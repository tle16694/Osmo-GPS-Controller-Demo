/* SPDX-License-Identifier: MIT */
/*
 * Copyright (C) 2025 SZ DJI Technology Co., Ltd.
 *  
 * All information contained herein is, and remains, the property of DJI.
 * The intellectual and technical concepts contained herein are proprietary
 * to DJI and may be covered by U.S. and foreign patents, patents in process,
 * and protected by trade secret or copyright law.  Dissemination of this
 * information, including but not limited to data and other proprietary
 * material(s) incorporated within the information, in any form, is strictly
 * prohibited without the express written consent of DJI.
 *
 * If you receive this source code without DJI’s authorization, you may not
 * further disseminate the information, and you must immediately remove the
 * source code and notify DJI of its removal. DJI reserves the right to pursue
 * legal actions against you for any loss(es) or damage(s) caused by your
 * failure to do so.
 */

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "connect_logic.h"
#include "status_logic.h"

#include "light_logic.h"
#include "product_config.h"

#define TAG "LOGIC_LIGHT"

// Single status LED (active-high)
#define STATUS_LED_GPIO PRODUCT_LED_GPIO

// Patterns (ms)
#define LED_BOOT_ON_MS 800
#define LED_BOOT_OFF_MS 200

#define LED_READY_ON_MS 120
#define LED_READY_OFF_MS 880

#define LED_CONNECTING_ON_MS 80
#define LED_CONNECTING_OFF_MS 120

#define LED_RECORDING_ON_MS 180
#define LED_RECORDING_OFF_MS 820

#define LED_ERROR_ON_MS 70
#define LED_ERROR_OFF_MS 70
#define LED_ERROR_PAUSE_MS 700

typedef enum {
    LED_MODE_READY = 0,
    LED_MODE_CONNECTING,
    LED_MODE_CONNECTED,
    LED_MODE_RECORDING,
    LED_MODE_ERROR,
} led_mode_t;

static TaskHandle_t s_led_task_handle = NULL;
static int64_t s_error_until_us = 0;

static void status_led_set(bool on) {
    gpio_set_level(STATUS_LED_GPIO, on ? 1 : 0);
}

static led_mode_t compute_led_mode(void) {
    const int64_t now_us = esp_timer_get_time();
    if (now_us < s_error_until_us) {
        return LED_MODE_ERROR;
    }

    const connect_state_t state = connect_logic_get_state();
    if (state == PROTOCOL_CONNECTED) {
        return is_camera_recording() ? LED_MODE_RECORDING : LED_MODE_CONNECTED;
    }

    if (state == BLE_SEARCHING || state == BLE_CONNECTED) {
        return LED_MODE_CONNECTING;
    }

    return LED_MODE_READY;
}

static bool delay_with_mode_check(uint32_t delay_ms, led_mode_t expected_mode) {
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(delay_ms);
    while (xTaskGetTickCount() < deadline) {
        if (compute_led_mode() != expected_mode) {
            return false;
        }
        const TickType_t remaining = deadline - xTaskGetTickCount();
        vTaskDelay(remaining > pdMS_TO_TICKS(50) ? pdMS_TO_TICKS(50) : remaining);
    }
    return true;
}

static void led_task(void *arg) {
    (void)arg;

    // BOOT: 800ms ON then 200ms OFF once
    status_led_set(true);
    vTaskDelay(pdMS_TO_TICKS(LED_BOOT_ON_MS));
    status_led_set(false);
    vTaskDelay(pdMS_TO_TICKS(LED_BOOT_OFF_MS));

    while (1) {
        const led_mode_t mode = compute_led_mode();

        switch (mode) {
            case LED_MODE_READY:
                status_led_set(true);
                if (!delay_with_mode_check(LED_READY_ON_MS, mode)) break;
                status_led_set(false);
                delay_with_mode_check(LED_READY_OFF_MS, mode);
                break;

            case LED_MODE_CONNECTING:
                status_led_set(true);
                if (!delay_with_mode_check(LED_CONNECTING_ON_MS, mode)) break;
                status_led_set(false);
                delay_with_mode_check(LED_CONNECTING_OFF_MS, mode);
                break;

            case LED_MODE_CONNECTED:
                status_led_set(true);
                delay_with_mode_check(200, mode);
                break;

            case LED_MODE_RECORDING:
                status_led_set(true);
                if (!delay_with_mode_check(LED_RECORDING_ON_MS, mode)) break;
                status_led_set(false);
                delay_with_mode_check(LED_RECORDING_OFF_MS, mode);
                break;

            case LED_MODE_ERROR:
                for (int i = 0; i < 3; i++) {
                    status_led_set(true);
                    if (!delay_with_mode_check(LED_ERROR_ON_MS, mode)) break;
                    status_led_set(false);
                    if (!delay_with_mode_check(LED_ERROR_OFF_MS, mode)) break;
                }
                status_led_set(false);
                delay_with_mode_check(LED_ERROR_PAUSE_MS, mode);
                break;

            default:
                status_led_set(false);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
        }
    }
}

void light_logic_signal_error(uint32_t duration_ms) {
    const int64_t now_us = esp_timer_get_time();
    const int64_t until_us = now_us + ((int64_t)duration_ms * 1000);
    if (until_us > s_error_until_us) {
        s_error_until_us = until_us;
    }
    if (s_led_task_handle) {
        xTaskNotifyGive(s_led_task_handle);
    }
}

int init_light_logic(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << STATUS_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    const esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %s", esp_err_to_name(ret));
        return -1;
    }
    status_led_set(false);

    if (xTaskCreate(led_task, "status_led", 2048, NULL, 2, &s_led_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LED task");
        return -1;
    }

    ESP_LOGI(TAG, "Single status LED initialized on GPIO%d", (int)STATUS_LED_GPIO);
    return 0;
}

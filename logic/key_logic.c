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
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "ble.h"
#include "command_logic.h"
#include "connect_logic.h"
#include "data.h"
#include "enums_logic.h"
#include "light_logic.h"
#include "product_config.h"
#include "product_nvs.h"
#include "status_logic.h"

#include "key_logic.h"

#define TAG "LOGIC_KEY"

typedef enum {
    BUTTON_EVENT_EDGE = 0,
    BUTTON_EVENT_FINALIZE,
} button_event_type_t;

typedef struct {
    button_event_type_t type;
    int level;            // 0=pressed, 1=released (active-low)
    TickType_t tick;
} button_event_t;

typedef enum {
    ACTION_NONE = 0,
    ACTION_RECORD_TOGGLE,
    ACTION_MODE_NEXT,
    ACTION_TAKE_PHOTO,
    ACTION_PAIR_OR_RECONNECT,
    ACTION_FACTORY_RESET_LINK,
} action_t;

static QueueHandle_t s_button_event_queue = NULL;
static QueueHandle_t s_action_queue = NULL;
static esp_timer_handle_t s_multiclick_timer = NULL;

static portMUX_TYPE s_activity_lock = portMUX_INITIALIZER_UNLOCKED;
static int64_t s_last_user_activity_us = 0;

static void mark_user_activity(void) {
    portENTER_CRITICAL(&s_activity_lock);
    s_last_user_activity_us = esp_timer_get_time();
    portEXIT_CRITICAL(&s_activity_lock);
}

static int64_t get_last_user_activity_us(void) {
    portENTER_CRITICAL(&s_activity_lock);
    const int64_t t = s_last_user_activity_us;
    portEXIT_CRITICAL(&s_activity_lock);
    return t;
}

static void post_action(action_t action) {
    if (!s_action_queue) {
        return;
    }
    (void)xQueueSend(s_action_queue, &action, 0);
}

static void multiclick_finalize_cb(void *arg) {
    (void)arg;
    if (!s_button_event_queue) {
        return;
    }
    const button_event_t event = {
        .type = BUTTON_EVENT_FINALIZE,
        .level = 1,
        .tick = xTaskGetTickCount(),
    };
    (void)xQueueSend(s_button_event_queue, &event, 0);
}

static void IRAM_ATTR button_isr_handler(void *arg) {
    (void)arg;
    if (!s_button_event_queue) {
        return;
    }
    const button_event_t event = {
        .type = BUTTON_EVENT_EDGE,
        .level = gpio_get_level(PRODUCT_BUTTON_GPIO),
        .tick = xTaskGetTickCountFromISR(),
    };
    BaseType_t high_task_woken = pdFALSE;
    (void)xQueueSendFromISR(s_button_event_queue, &event, &high_task_woken);
    if (high_task_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void maybe_enter_light_sleep(void) {
    const connect_state_t state = connect_logic_get_state();
    if (state == BLE_SEARCHING || state == BLE_CONNECTED || state == PROTOCOL_CONNECTED) {
        return;
    }
    if (gpio_get_level(PRODUCT_BUTTON_GPIO) == 0) {
        return;
    }

    const int64_t now_us = esp_timer_get_time();
    const int64_t idle_us = now_us - get_last_user_activity_us();
    if (idle_us < ((int64_t)PRODUCT_IDLE_LIGHT_SLEEP_MS * 1000)) {
        return;
    }

    ESP_LOGI(TAG, "Idle for %u ms -> entering light sleep", PRODUCT_IDLE_LIGHT_SLEEP_MS);

    // Turn LED off before sleep (state machine will resume after wake).
    gpio_set_level(PRODUCT_LED_GPIO, 0);

    ESP_ERROR_CHECK(gpio_wakeup_enable(PRODUCT_BUTTON_GPIO, GPIO_INTR_LOW_LEVEL));
    ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());

    const esp_sleep_wakeup_cause_t before = esp_sleep_get_wakeup_cause();
    (void)before;

    esp_light_sleep_start();

    ESP_LOGI(TAG, "Woke from light sleep, cause=%d", esp_sleep_get_wakeup_cause());
    gpio_wakeup_disable(PRODUCT_BUTTON_GPIO);
    mark_user_activity();
}

static void disconnect_if_connected(void) {
    const connect_state_t state = connect_logic_get_state();
    if (state == BLE_CONNECTED || state == PROTOCOL_CONNECTED || state == BLE_DISCONNECTING) {
        (void)connect_logic_ble_disconnect();
        for (int i = 0; i < 50; i++) { // ~5s
            if (connect_logic_get_state() == BLE_INIT_COMPLETE) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

static uint8_t choose_verify_mode(bool used_stored_camera_bda, bool force_pairing) {
    if (force_pairing) {
        return 1;
    }
    if (used_stored_camera_bda && product_nvs_get_paired()) {
        return 0;
    }
    return 1;
}

static int protocol_connect_and_prepare(bool used_stored_camera_bda, bool force_pairing) {
    const uint32_t device_id = product_nvs_get_or_create_device_id();

    uint8_t bt_mac_u8[6] = {0};
    esp_read_mac(bt_mac_u8, ESP_MAC_BT);
    int8_t bt_mac_i8[6] = {0};
    for (int i = 0; i < 6; i++) {
        bt_mac_i8[i] = (int8_t)bt_mac_u8[i];
    }

    const uint8_t verify_mode = choose_verify_mode(used_stored_camera_bda, force_pairing);
    const uint16_t verify_data = (uint16_t)(esp_random() % 10000);

    ESP_LOGI(TAG, "Protocol connect: verify_mode=%u verify_data=%u device_id=0x%08X", verify_mode, verify_data, (unsigned int)device_id);

    const int res = connect_logic_protocol_connect(
        device_id,
        6,
        bt_mac_i8,
        PRODUCT_FW_VERSION_U32,
        verify_mode,
        verify_data,
        0
    );
    if (res != 0) {
        ESP_LOGE(TAG, "Protocol connect failed");
        light_logic_signal_error(PRODUCT_ERROR_SIGNAL_MS);
        (void)connect_logic_ble_disconnect();
        return -1;
    }

    version_query_response_frame_t *version_resp = command_logic_get_version();
    if (version_resp) {
        free(version_resp);
    }

    const int sub_res = subscript_camera_status(PUSH_MODE_PERIODIC_WITH_STATE_CHANGE, PUSH_FREQ_2HZ);
    if (sub_res != 0) {
        ESP_LOGW(TAG, "Failed to subscribe camera status");
        light_logic_signal_error(PRODUCT_ERROR_SIGNAL_MS);
    }

    (void)product_nvs_set_last_camera_bda(s_ble_profile.remote_bda);
    (void)product_nvs_set_paired(true);

    ESP_LOGI(TAG, "Camera linked: %02X:%02X:%02X:%02X:%02X:%02X",
             s_ble_profile.remote_bda[0], s_ble_profile.remote_bda[1], s_ble_profile.remote_bda[2],
             s_ble_profile.remote_bda[3], s_ble_profile.remote_bda[4], s_ble_profile.remote_bda[5]);

    return 0;
}

static int connect_ble_and_protocol(bool prefer_last_camera, bool force_pairing) {
    if (connect_logic_get_state() == PROTOCOL_CONNECTED) {
        return 0;
    }

    disconnect_if_connected();

    esp_bd_addr_t last_bda = {0};
    const bool have_last = product_nvs_get_last_camera_bda(last_bda);

    if (prefer_last_camera && have_last) {
        memcpy(s_ble_profile.remote_bda, last_bda, ESP_BD_ADDR_LEN);
        ESP_LOGI(TAG, "Reconnect to last camera...");
        if (connect_logic_ble_connect(true) == 0) {
            if (protocol_connect_and_prepare(true, force_pairing) == 0) {
                return 0;
            }
        }
        disconnect_if_connected();
    }

    ESP_LOGI(TAG, "Scan/connect to nearest compatible camera...");
    if (connect_logic_ble_connect(false) != 0) {
        ESP_LOGE(TAG, "BLE connect failed");
        light_logic_signal_error(PRODUCT_ERROR_SIGNAL_MS);
        return -1;
    }
    return protocol_connect_and_prepare(false, force_pairing);
}

static void action_record_toggle(void) {
    if (connect_logic_get_state() != PROTOCOL_CONNECTED) {
        light_logic_signal_error(PRODUCT_ERROR_SIGNAL_MS);
        return;
    }

    if (is_camera_recording()) {
        record_control_response_frame_t *resp = command_logic_stop_record();
        if (resp) {
            free(resp);
            return;
        }
        light_logic_signal_error(PRODUCT_ERROR_SIGNAL_MS);
        return;
    }

    if ((camera_mode_t)current_camera_mode == CAMERA_MODE_PHOTO) {
        camera_mode_switch_response_frame_t *sw = command_logic_switch_camera_mode(CAMERA_MODE_NORMAL);
        if (sw) {
            free(sw);
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }

    record_control_response_frame_t *resp = command_logic_start_record();
    if (!resp) {
        (void)connect_logic_ble_wakeup();
        vTaskDelay(pdMS_TO_TICKS(250));
        resp = command_logic_start_record();
    }
    if (resp) {
        free(resp);
        return;
    }
    light_logic_signal_error(PRODUCT_ERROR_SIGNAL_MS);
}

static void action_mode_next(void) {
    if (connect_logic_get_state() != PROTOCOL_CONNECTED) {
        light_logic_signal_error(PRODUCT_ERROR_SIGNAL_MS);
        return;
    }
    key_report_response_frame_t *resp = command_logic_key_report_qs();
    if (resp) {
        free(resp);
        return;
    }
    light_logic_signal_error(PRODUCT_ERROR_SIGNAL_MS);
}

static void action_take_photo(void) {
    if (connect_logic_get_state() != PROTOCOL_CONNECTED) {
        light_logic_signal_error(PRODUCT_ERROR_SIGNAL_MS);
        return;
    }

    if ((camera_mode_t)current_camera_mode != CAMERA_MODE_PHOTO) {
        camera_mode_switch_response_frame_t *sw = command_logic_switch_camera_mode(CAMERA_MODE_PHOTO);
        if (sw) {
            free(sw);
        }
        vTaskDelay(pdMS_TO_TICKS(350));
    }

    key_report_response_frame_t *resp = command_logic_key_report_snapshot();
    if (resp) {
        free(resp);
        return;
    }

    // Fallback: force photo mode then retry shutter
    camera_mode_switch_response_frame_t *sw = command_logic_switch_camera_mode(CAMERA_MODE_PHOTO);
    if (sw) {
        free(sw);
    }
    vTaskDelay(pdMS_TO_TICKS(350));

    resp = command_logic_key_report_snapshot();
    if (resp) {
        free(resp);
        return;
    }
    light_logic_signal_error(PRODUCT_ERROR_SIGNAL_MS);
}

static void action_pair_or_reconnect(void) {
    (void)connect_ble_and_protocol(true, false);
}

static void action_factory_reset_link(void) {
    ESP_LOGW(TAG, "Factory reset link (NVS clear + force re-pair)");
    (void)connect_logic_ble_disconnect();
    (void)product_nvs_factory_reset();
    memset(s_ble_profile.remote_bda, 0, ESP_BD_ADDR_LEN);
    (void)connect_ble_and_protocol(false, true);
}

static void action_task(void *arg) {
    (void)arg;

    // Auto-reconnect on boot (only if a bonded device exists)
    vTaskDelay(pdMS_TO_TICKS(PRODUCT_AUTOCONNECT_DELAY_MS));
    esp_bd_addr_t last_bda = {0};
    if (product_nvs_get_last_camera_bda(last_bda)) {
        memcpy(s_ble_profile.remote_bda, last_bda, ESP_BD_ADDR_LEN);
        (void)connect_ble_and_protocol(true, false);
    }

    connect_state_t last_state = connect_logic_get_state();
    action_t action = ACTION_NONE;
    while (1) {
        if (xQueueReceive(s_action_queue, &action, pdMS_TO_TICKS(250)) == pdTRUE) {
            switch (action) {
                case ACTION_RECORD_TOGGLE:
                    action_record_toggle();
                    break;
                case ACTION_MODE_NEXT:
                    action_mode_next();
                    break;
                case ACTION_TAKE_PHOTO:
                    action_take_photo();
                    break;
                case ACTION_PAIR_OR_RECONNECT:
                    action_pair_or_reconnect();
                    break;
                case ACTION_FACTORY_RESET_LINK:
                    action_factory_reset_link();
                    break;
                default:
                    break;
            }
        }

        const connect_state_t state = connect_logic_get_state();
        if (state == BLE_CONNECTED && last_state != BLE_CONNECTED) {
            // BLE reconnected by lower layer; restore protocol link once.
            ESP_LOGI(TAG, "BLE connected without protocol, restoring protocol link...");
            (void)protocol_connect_and_prepare(true, false);
        }
        last_state = connect_logic_get_state();
        maybe_enter_light_sleep();
    }
}

static void button_task(void *arg) {
    (void)arg;

    bool pressed = (gpio_get_level(PRODUCT_BUTTON_GPIO) == 0);
    TickType_t press_tick = 0;
    TickType_t last_edge_tick = 0;
    uint8_t click_count = 0;

    button_event_t event;
    while (1) {
        if (xQueueReceive(s_button_event_queue, &event, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (event.type == BUTTON_EVENT_EDGE) {
            const TickType_t debounce_ticks = pdMS_TO_TICKS(PRODUCT_DEBOUNCE_MS);
            if ((event.tick - last_edge_tick) < debounce_ticks) {
                continue;
            }
            last_edge_tick = event.tick;
            mark_user_activity();

            if (event.level == 0 && !pressed) {
                pressed = true;
                press_tick = event.tick;
                (void)esp_timer_stop(s_multiclick_timer);
                continue;
            }

            if (event.level == 1 && pressed) {
                pressed = false;
                const TickType_t duration_ticks = event.tick - press_tick;
                const uint32_t duration_ms = (uint32_t)(duration_ticks * portTICK_PERIOD_MS);

                if (duration_ms >= PRODUCT_VERY_LONG_PRESS_MS) {
                    click_count = 0;
                    (void)esp_timer_stop(s_multiclick_timer);
                    post_action(ACTION_FACTORY_RESET_LINK);
                    continue;
                }

                if (duration_ms >= PRODUCT_LONG_PRESS_MS) {
                    click_count = 0;
                    (void)esp_timer_stop(s_multiclick_timer);
                    post_action(ACTION_PAIR_OR_RECONNECT);
                    continue;
                }

                if (duration_ms < PRODUCT_MIN_VALID_PRESS_MS) {
                    continue;
                }

                if (click_count < 3) {
                    click_count++;
                }

                (void)esp_timer_stop(s_multiclick_timer);
                (void)esp_timer_start_once(s_multiclick_timer, PRODUCT_MULTICLICK_FINALIZE_WINDOW_US);
                continue;
            }

            continue;
        }

        if (event.type == BUTTON_EVENT_FINALIZE) {
            const uint8_t final_clicks = click_count;
            click_count = 0;

            if (pressed) {
                continue;
            }

            switch (final_clicks) {
                case 1:
                    post_action(ACTION_RECORD_TOGGLE);
                    break;
                case 2:
                    post_action(ACTION_MODE_NEXT);
                    break;
                case 3:
                    post_action(ACTION_TAKE_PHOTO);
                    break;
                default:
                    break;
            }
            continue;
        }
    }
}

void key_logic_init(void) {
    mark_user_activity();

    ESP_ERROR_CHECK(product_nvs_init());
    esp_bd_addr_t last_bda = {0};
    if (product_nvs_get_last_camera_bda(last_bda)) {
        memcpy(s_ble_profile.remote_bda, last_bda, ESP_BD_ADDR_LEN);
    }

    if (!is_data_layer_initialized()) {
        data_init();
        data_register_status_update_callback(update_camera_state_handler);
        data_register_new_status_update_callback(update_new_camera_state_handler);
    }

    // Configure button GPIO as input with internal pull-up (active-low button)
    const gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PRODUCT_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Queues
    s_button_event_queue = xQueueCreate(16, sizeof(button_event_t));
    s_action_queue = xQueueCreate(8, sizeof(action_t));
    if (!s_button_event_queue || !s_action_queue) {
        ESP_LOGE(TAG, "Failed to create queues");
        return;
    }

    // esp_timer one-shot for multiclick finalization
    const esp_timer_create_args_t timer_args = {
        .callback = &multiclick_finalize_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "multiclick_finalize",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_multiclick_timer));

    // ISR service (ignore already-installed case)
    esp_err_t isr_ret = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    if (isr_ret != ESP_OK && isr_ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(isr_ret);
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(PRODUCT_BUTTON_GPIO, button_isr_handler, NULL));

    // Tasks
    if (xTaskCreate(button_task, "button_task", 2048, NULL, 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button_task");
        return;
    }
    if (xTaskCreate(action_task, "action_task", 6144, NULL, 2, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create action_task");
        return;
    }

    ESP_LOGI(TAG, "Single-button UI on GPIO%d (active-low, pull-up)", (int)PRODUCT_BUTTON_GPIO);
}

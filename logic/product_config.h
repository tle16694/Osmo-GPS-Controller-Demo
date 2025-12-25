/* SPDX-License-Identifier: MIT */
/*
 * Copyright (C) 2025 SZ DJI Technology Co., Ltd.
 *
 * Product configuration for ESP32 DevKit (ESP-WROOM-32) single-button BLE remote.
 */

#ifndef PRODUCT_CONFIG_H
#define PRODUCT_CONFIG_H

#include "driver/gpio.h"

// Semantic version (single source of truth)
#define PRODUCT_VERSION "3.1.0"

// Encoded firmware version for protocol fields (major.minor.patch -> 0xMMmmpp00)
#define PRODUCT_FW_VERSION_U32 ((3U << 24) | (1U << 16) | (0U << 8))

// Hardware (fixed pinout)
#define PRODUCT_BUTTON_GPIO GPIO_NUM_27
#define PRODUCT_LED_GPIO GPIO_NUM_33

// UX timing
#define PRODUCT_DEBOUNCE_MS 30
#define PRODUCT_MULTICLICK_FINALIZE_WINDOW_US 380000
#define PRODUCT_LONG_PRESS_MS 2000
#define PRODUCT_VERY_LONG_PRESS_MS 7000
#define PRODUCT_MIN_VALID_PRESS_MS 30

// Power
#define PRODUCT_IDLE_LIGHT_SLEEP_MS (5U * 60U * 1000U)

// Feedback
#define PRODUCT_ERROR_SIGNAL_MS 2200U

// Connection tuning
#define PRODUCT_AUTOCONNECT_DELAY_MS 300U

#endif


/* SPDX-License-Identifier: MIT */
/*
 * Product NVS storage (bonded camera address + pairing state).
 */

#ifndef PRODUCT_NVS_H
#define PRODUCT_NVS_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_bt_defs.h"

esp_err_t product_nvs_init(void);

bool product_nvs_get_last_camera_bda(esp_bd_addr_t out_bda);
esp_err_t product_nvs_set_last_camera_bda(const esp_bd_addr_t bda);
esp_err_t product_nvs_clear_last_camera_bda(void);

bool product_nvs_get_paired(void);
esp_err_t product_nvs_set_paired(bool paired);

uint32_t product_nvs_get_or_create_device_id(void);
esp_err_t product_nvs_factory_reset(void);

#endif


/* SPDX-License-Identifier: MIT */
/*
 * Product NVS storage (bonded camera address + pairing state).
 */

#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_system.h"

#include "product_nvs.h"

#define TAG "PRODUCT_NVS"

static const char *NVS_NS = "onebtn";
static const char *KEY_CAM_BDA = "cam_bda";
static const char *KEY_PAIRED = "paired";
static const char *KEY_DEVICE_ID = "dev_id";

static bool bda_is_zero(const esp_bd_addr_t bda) {
    static const uint8_t zero[ESP_BD_ADDR_LEN] = {0};
    return memcmp(bda, zero, ESP_BD_ADDR_LEN) == 0;
}

esp_err_t product_nvs_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS init returned %s, erasing NVS...", esp_err_to_name(ret));
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

bool product_nvs_get_last_camera_bda(esp_bd_addr_t out_bda) {
    if (!out_bda) {
        return false;
    }

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return false;
    }

    size_t len = ESP_BD_ADDR_LEN;
    ret = nvs_get_blob(handle, KEY_CAM_BDA, out_bda, &len);
    nvs_close(handle);

    if (ret != ESP_OK || len != ESP_BD_ADDR_LEN || bda_is_zero(out_bda)) {
        memset(out_bda, 0, ESP_BD_ADDR_LEN);
        return false;
    }
    return true;
}

esp_err_t product_nvs_set_last_camera_bda(const esp_bd_addr_t bda) {
    if (!bda || bda_is_zero(bda)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_set_blob(handle, KEY_CAM_BDA, bda, ESP_BD_ADDR_LEN);
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);
    return ret;
}

esp_err_t product_nvs_clear_last_camera_bda(void) {
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    (void)nvs_erase_key(handle, KEY_CAM_BDA);
    ret = nvs_commit(handle);
    nvs_close(handle);
    return ret;
}

bool product_nvs_get_paired(void) {
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return false;
    }
    uint8_t paired = 0;
    ret = nvs_get_u8(handle, KEY_PAIRED, &paired);
    nvs_close(handle);
    return (ret == ESP_OK) && (paired != 0);
}

esp_err_t product_nvs_set_paired(bool paired) {
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_set_u8(handle, KEY_PAIRED, paired ? 1 : 0);
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);
    return ret;
}

static uint32_t derive_device_id_from_bt_mac(void) {
    uint8_t bt_mac[6] = {0};
    esp_read_mac(bt_mac, ESP_MAC_BT);

    uint32_t id = ((uint32_t)bt_mac[2] << 24) |
                  ((uint32_t)bt_mac[3] << 16) |
                  ((uint32_t)bt_mac[4] << 8) |
                  ((uint32_t)bt_mac[5]);
    id ^= 0xA5A50000U;
    if (id == 0) {
        id = 0xA5A50001U;
    }
    return id;
}

uint32_t product_nvs_get_or_create_device_id(void) {
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return derive_device_id_from_bt_mac();
    }

    uint32_t device_id = 0;
    ret = nvs_get_u32(handle, KEY_DEVICE_ID, &device_id);
    if (ret == ESP_OK && device_id != 0) {
        nvs_close(handle);
        return device_id;
    }

    device_id = derive_device_id_from_bt_mac();
    (void)nvs_set_u32(handle, KEY_DEVICE_ID, device_id);
    (void)nvs_commit(handle);
    nvs_close(handle);
    return device_id;
}

esp_err_t product_nvs_factory_reset(void) {
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return ret;
    }

    (void)nvs_erase_key(handle, KEY_CAM_BDA);
    (void)nvs_erase_key(handle, KEY_PAIRED);
    (void)nvs_erase_key(handle, KEY_DEVICE_ID);

    ret = nvs_commit(handle);
    nvs_close(handle);
    return ret;
}


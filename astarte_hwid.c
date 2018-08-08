/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#include <astarte_hwid.h>

#include <string.h>

#include <esp_log.h>
#include <esp_system.h>

#include <mbedtls/md.h>

#define HWID_TAG "ASTARTE_HWID"

astarte_err_t astarte_hwid_get_id(uint8_t *hardware_id)
{
    uint8_t mac_addr[6];
    if (esp_efuse_mac_get_default(mac_addr)) {
        ESP_LOGE(HWID_TAG, "Cannot read MAC address.");
        return ASTARTE_ERR;
    }

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    char info_string[160];

    int embedded_flash = (chip_info.features & CHIP_FEATURE_EMB_FLASH) != 0;
    int bluetooth = (chip_info.features & CHIP_FEATURE_BT) != 0;
    int ble = (chip_info.features & CHIP_FEATURE_BLE) != 0;

    snprintf(info_string, 160,
            "ESP_MAC_WIFI_STA: %x:%x:%x:%x:%x:%x, model: %i, cores: %i, revision: %i "
            "embedded flash: %i, bluetooth: %i, BLE: %i.",
            (unsigned int) mac_addr[0], (unsigned int) mac_addr[1], (unsigned int) mac_addr[2],
            (unsigned int) mac_addr[3], (unsigned int) mac_addr[4], (unsigned int) mac_addr[5],
            chip_info.model, chip_info.cores, chip_info.revision, embedded_flash, bluetooth, ble);

    ESP_LOGD(HWID_TAG, "Astarte Device SDK running on: %s", info_string);

    uint8_t sha_result[32];

    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char *) info_string, strlen(info_string));
    mbedtls_md_finish(&ctx, sha_result);
    mbedtls_md_free(&ctx);

    memcpy(hardware_id, sha_result, 16);

    return ASTARTE_OK;
}
/*
 * (C) Copyright 2018-2021, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#include <astarte_hwid.h>

#include <string.h>
#include <uuid.h>

#include <esp_log.h>
#include <esp_system.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <esp_chip_info.h>
#include <esp_mac.h>
#endif

#include <mbedtls/base64.h>
#include <mbedtls/md.h>

#define HWID_TAG "ASTARTE_HWID"

astarte_err_t astarte_hwid_get_id(uint8_t *hardware_id)
{
    uint8_t mac_addr[6];
    if (esp_efuse_mac_get_default(mac_addr)) {
        ESP_LOGE(HWID_TAG, "Cannot read MAC address.");
        return ASTARTE_ERR_ESP_SDK;
    }

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    char info_string[160];

    // CHIP_FEATURE_EMB_FLASH, CHIP_FEATURE_BT and CHIP_FEATURE_BLE are part of the esp-idf lib.
    // NOLINTBEGIN(hicpp-signed-bitwise)
    int embedded_flash = (chip_info.features & CHIP_FEATURE_EMB_FLASH) != 0;
    int bluetooth = (chip_info.features & CHIP_FEATURE_BT) != 0;
    int ble = (chip_info.features & CHIP_FEATURE_BLE) != 0;
    // NOLINTEND(hicpp-signed-bitwise)

    // See changelog to v5.0 of ESP IDF: https://github.com/espressif/esp-idf/releases/tag/v5.0
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    uint16_t revision = chip_info.revision / 100U;
#else
    uint16_t revision = chip_info.revision;
#endif

    int res = snprintf(info_string, 160,
        "ESP_MAC_WIFI_STA: %02x:%02x:%02x:%02x:%02x:%02x, model: %i, cores: %i, revision: %i "
        "embedded flash: %i, bluetooth: %i, BLE: %i.",
        (unsigned int) mac_addr[0], (unsigned int) mac_addr[1], (unsigned int) mac_addr[2],
        (unsigned int) mac_addr[3], (unsigned int) mac_addr[4], (unsigned int) mac_addr[5],
        chip_info.model, chip_info.cores, revision, embedded_flash, bluetooth, ble);
    if ((res < 0) || (res >= 160)) {
        ESP_LOGE(HWID_TAG, "Error generating the encoding device specific info string.");
        return ASTARTE_ERR;
    }

    ESP_LOGD(HWID_TAG, "Astarte Device SDK running on: %s", info_string);

#ifdef CONFIG_ASTARTE_HWID_ENABLE_UUID
    uuid_t namespace_uuid;
    uuid_from_string(CONFIG_ASTARTE_HWID_UUID_NAMESPACE, namespace_uuid);

    uuid_t device_uuid;
    uuid_generate_v5(namespace_uuid, info_string, strlen(info_string), device_uuid);

    memcpy(hardware_id, device_uuid, 16);
#else
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
#endif // ASTARTE_HWID_ENABLE_UUID

    return ASTARTE_OK;
}

void astarte_hwid_encode(char *encoded, int dest_size, const uint8_t *hardware_id)
{
    size_t out_len = 0U;
    mbedtls_base64_encode((unsigned char *) encoded, dest_size, &out_len, hardware_id, 16);

    for (int i = 0; i < out_len; i++) {
        if (encoded[i] == '+') {
            encoded[i] = '-';

        } else if (encoded[i] == '/') {
            encoded[i] = '_';
        }
    }

    if (encoded[out_len - 2] == '=') {
        encoded[out_len - 2] = 0;
    }
}

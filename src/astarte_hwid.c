/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
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

#define TAG "ASTARTE_HWID"

astarte_err_t astarte_hwid_get_id(uint8_t *hardware_id)
{
    uint8_t mac_addr[6];
    if (esp_efuse_mac_get_default(mac_addr)) {
        ESP_LOGE(TAG, "Cannot read MAC address.");
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
        ESP_LOGE(TAG, "Error generating the encoding device specific info string.");
        return ASTARTE_ERR;
    }

    ESP_LOGD(TAG, "Astarte Device SDK running on: %s", info_string);

#ifdef CONFIG_ASTARTE_HWID_ENABLE_UUID
    uuid_t namespace_uuid;
    uuid_from_string(CONFIG_ASTARTE_HWID_UUID_NAMESPACE, namespace_uuid);

    uuid_t device_uuid;
    astarte_err_t uuid_err
        = uuid_generate_v5(namespace_uuid, info_string, strlen(info_string), device_uuid);
    if (uuid_err != ASTARTE_OK) {
        ESP_LOGE(TAG, "HWID generation failed.");
        return uuid_err;
    }

    memcpy(hardware_id, device_uuid, 16);
#else
    uint8_t sha_result[32];

    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    mbedtls_md_init(&ctx);
    int mbedtls_err = mbedtls_md_setup(&ctx, md_info, 0);
    // NOLINTBEGIN(hicpp-signed-bitwise) Only using the mbedtls_err to check if zero
    mbedtls_err |= mbedtls_md_starts(&ctx);
    mbedtls_err
        |= mbedtls_md_update(&ctx, (const unsigned char *) info_string, strlen(info_string));
    mbedtls_err |= mbedtls_md_finish(&ctx, sha_result);
    // NOLINTEND(hicpp-signed-bitwise)
    mbedtls_md_free(&ctx);
    if (mbedtls_err != 0) {
        ESP_LOGE(TAG, "HWID generation failed.");
        return ASTARTE_ERR;
    }

    memcpy(hardware_id, sha_result, 16);
#endif // ASTARTE_HWID_ENABLE_UUID

    return ASTARTE_OK;
}

astarte_err_t astarte_hwid_encode(char *encoded, int dest_size, const uint8_t *hardware_id)
{
    size_t out_len = 0U;
    int mbedtls_err
        = mbedtls_base64_encode((unsigned char *) encoded, dest_size, &out_len, hardware_id, 16);
    if (mbedtls_err != 0) {
        ESP_LOGE(TAG, "HWID encoding failed.");
        return ASTARTE_ERR;
    }

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
    return ASTARTE_OK;
}

/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "astarte_device_sdk/device_id.h"

#include <string.h>

#include <esp_chip_info.h>
#include <esp_err.h>
#include <esp_mac.h>

#include "log.h"
#include "uuid.h"

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

ASTARTE_LOG_MODULE_REGISTER("astarte-device-id");

#if defined(CONFIG_SOC_IEEE802154_SUPPORTED)
#define MAC_DEFAULT_LEN 8
#else
#define MAC_DEFAULT_LEN 6
#endif

#define CHIP_REVISION_MAJOR_DIVIDER 100U
#define HARDWARE_INFO_STRING_SIZE 160

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_result_t astarte_device_id_generate_random(char out[static ASTARTE_DEVICE_ID_LEN + 1])
{
    uuid_t uuid;
    uuid_generate_v4(uuid);
    return uuid_to_base64url(uuid, out, ASTARTE_DEVICE_ID_LEN + 1);
}

astarte_result_t astarte_device_id_generate_deterministic(
    const uint8_t namespace[static ASTARTE_DEVICE_ID_NAMESPACE_SIZE], const uint8_t *name,
    size_t name_size, char out[static ASTARTE_DEVICE_ID_LEN + 1])
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    uuid_t uuid = { 0 };

    ares = uuid_generate_v5(namespace, name, name_size, uuid);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("UUID V5 generation failed: %s", astarte_result_to_name(ares));
        return ares;
    }

    return uuid_to_base64url(uuid, out, ASTARTE_DEVICE_ID_LEN + 1);
}

astarte_result_t astarte_device_id_generate_unique(
    const uint8_t namespace[static ASTARTE_DEVICE_ID_NAMESPACE_SIZE],
    char out[static ASTARTE_DEVICE_ID_LEN + 1])
{
    esp_err_t esp_err = ESP_OK;

    // Get MAC address
    uint8_t mac_addr[MAC_DEFAULT_LEN] = { 0 };
    esp_err = esp_efuse_mac_get_default(mac_addr);
    if (esp_err != ESP_OK) {
        ASTARTE_LOG_ERR("Failed reading the mac address: %s", esp_err_to_name(esp_err));
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }

    // Get chip info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    // NOLINTBEGIN(hicpp-signed-bitwise)
    bool embedded_flash = (chip_info.features & CHIP_FEATURE_EMB_FLASH) != 0;
    bool bluetooth = (chip_info.features & CHIP_FEATURE_BT) != 0;
    bool ble = (chip_info.features & CHIP_FEATURE_BLE) != 0;
    // NOLINTEND(hicpp-signed-bitwise)
    uint16_t revision = chip_info.revision / CHIP_REVISION_MAJOR_DIVIDER;

    // Compose a device unique info string
    char info_string[HARDWARE_INFO_STRING_SIZE] = { 0 };
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    int res = snprintf(info_string, HARDWARE_INFO_STRING_SIZE,
        "ESP_MAC_WIFI_STA: %02x:%02x:%02x:%02x:%02x:%02x, model: %i, cores: %i, revision: %i "
        "embedded flash: %i, bluetooth: %i, BLE: %i.",
        (unsigned int) mac_addr[0], (unsigned int) mac_addr[1], (unsigned int) mac_addr[2],
        (unsigned int) mac_addr[3], (unsigned int) mac_addr[4], (unsigned int) mac_addr[5],
        chip_info.model, chip_info.cores, revision, embedded_flash, bluetooth, ble);
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    if ((res < 0) || (res >= HARDWARE_INFO_STRING_SIZE)) {
        ASTARTE_LOG_ERR("Error generating the encoding device specific info string.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }

    ASTARTE_LOG_DBG("Astarte Device SDK running on: %s", info_string);

    // Convert the info string in a UUID v5 using the provided namespace
    uuid_t uuid = { 0 };
    astarte_result_t ares = uuid_generate_v5(namespace, info_string, strlen(info_string), uuid);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("UUID V5 generation failed: %s", astarte_result_to_name(ares));
        return ares;
    }

    return uuid_to_base64url(uuid, out, ASTARTE_DEVICE_ID_LEN + 1);
}

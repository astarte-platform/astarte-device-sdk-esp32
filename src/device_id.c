/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "astarte_device_sdk/device_id.h"

#include "uuid.h"

#include <esp_log.h>

#define TAG "ASTARTE_DEVICE_ID"

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
        ESP_LOGE(TAG, "UUID V5 generation failed: %s", astarte_result_to_name(ares));
        return ares;
    }

    return uuid_to_base64url(uuid, out, ASTARTE_DEVICE_ID_LEN + 1);
}

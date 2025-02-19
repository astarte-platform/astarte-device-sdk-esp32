/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uuid.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <esp_random.h>

// #include <zephyr/posix/arpa/inet.h>
// #include <zephyr/random/random.h>
// #include <zephyr/sys/base64.h>

#include <mbedtls/base64.h>
#include <mbedtls/md.h>

#include <esp_log.h>

#define TAG "ASTARTE_UUID"

// All the macros below follow the standard for the Universally Unique Identifier as defined
// by the IETF in the RFC9562.
// The full specification can be found here: https://datatracker.ietf.org/doc/rfc9562/

// Position of the hyphens
#define UUID_STR_POSITION_FIRST_HYPHEN (8U)
#define UUID_STR_POSITION_SECOND_HYPHEN (13U)
#define UUID_STR_POSITION_THIRD_HYPHEN (18U)
#define UUID_STR_POSITION_FOURTH_HYPHEN (23U)

// Common positions, offsets and masks for all UUID versions
#define UUID_POSITION_VERSION (6U)
#define UUID_OFFSET_VERSION (4U)
#define UUID_MASK_VERSION (0b11110000U)
#define UUID_POSITION_VARIANT (8U)
#define UUID_OFFSET_VARIANT (6U)
#define UUID_MASK_VARIANT (0b11000000U)

#define UUID_V4_VERSION (4U)
#define UUID_V4_VARIANT (2U)
#define UUID_V5_VERSION (5U)
#define UUID_V5_VARIANT (2U)

/**
 * @brief Overwrite the 'ver' and 'var' fields in the input UUID.
 *
 * @param[inout] uuid The UUID to use for the operation.
 * @param[in] version The new version for the UUID. Only the first 4 bits will be used.
 * @param[in] variant The new variant for the UUID. Only the first 2 bits will be used.
 */
static void overwrite_uuid_version_and_variant(uuid_t uuid, uint8_t version, uint8_t variant);
/**
 * @brief Check if in a UUID string the char in the provided position should be an hyphen.
 *
 * @param[in] position The position to evaluate.
 * @return True when at the provided position there should be an hyphen, false otherwise.
 */
static bool should_be_hyphen(uint8_t position);

void uuid_generate_v4(uuid_t out)
{
    // Fill the whole UUID struct with a random number
    esp_fill_random(out, UUID_SIZE);
    // Update version and variant
    overwrite_uuid_version_and_variant(out, UUID_V4_VERSION, UUID_V4_VARIANT);
}

astarte_result_t uuid_generate_v5(
    const uuid_t namespace, const void *data, size_t data_size, uuid_t out)
{
    const size_t sha_1_bytes = 20;
    uint8_t sha_result[sha_1_bytes];

    mbedtls_md_context_t ctx = { 0 };
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);

    mbedtls_md_init(&ctx);
    int mbedtls_err = mbedtls_md_setup(&ctx, md_info, 0);
    // NOLINTBEGIN(hicpp-signed-bitwise) Only using the mbedtls_err to check if zero
    mbedtls_err |= mbedtls_md_starts(&ctx);
    mbedtls_err |= mbedtls_md_update(&ctx, namespace, UUID_SIZE);
    mbedtls_err |= mbedtls_md_update(&ctx, data, data_size);
    mbedtls_err |= mbedtls_md_finish(&ctx, sha_result);
    // NOLINTEND(hicpp-signed-bitwise)
    mbedtls_md_free(&ctx);
    if (mbedtls_err != 0) {
        ESP_LOGE(TAG, "UUID V5 generation failed.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }

    // Store the computed SHA1 in the out struct
    for (size_t i = 0; i < UUID_SIZE; i++) {
        out[i] = sha_result[i];
    }
    // Update version and variant
    overwrite_uuid_version_and_variant(out, UUID_V5_VERSION, UUID_V5_VARIANT);

    return ASTARTE_RESULT_OK;
}

astarte_result_t uuid_generate_v5_to_base64url(
    const uuid_t namespace, const void *data, size_t data_size, char *out, size_t out_size)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    uuid_t uuid = { 0 };

    ares = uuid_generate_v5(namespace, data, data_size, uuid);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "UUID V5 generation failed: %s", astarte_result_to_name(ares));
        return ares;
    }

    return uuid_to_base64url(uuid, out, out_size);
}

astarte_result_t uuid_from_string(const char *input, uuid_t out)
{
    // UUID string format is: 'XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX', where X is a hex char

    // Length check
    if (strlen(input) != UUID_STR_LEN) {
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    // Format check
    for (int i = 0; i < UUID_STR_LEN; i++) {
        char char_i = input[i];
        // Check that hyphens are in the right place
        if (should_be_hyphen(i)) {
            if (char_i != '-') {
                ESP_LOGW(TAG, "Found invalid character %c in hyphen position %d", char_i, i);
                return ASTARTE_RESULT_INVALID_PARAM;
            }
            continue;
        }
        // checking if the given input is not hexadecimal
        if (!isxdigit(char_i)) {
            ESP_LOGW(TAG, "Found invalid character %c in position %d", char_i, i);
            return ASTARTE_RESULT_INVALID_PARAM;
        }
    }

    // Content parsing
    size_t input_idx = 0U;
    size_t out_idx = 0U;
    const int uuid_str_base = 16;
    while (input_idx < UUID_STR_LEN) {
        if (should_be_hyphen(input_idx)) {
            input_idx += 1;
            continue;
        }
        char tmp[] = { input[input_idx], input[input_idx + 1], '\0' };
        out[out_idx] = strtoul(tmp, NULL, uuid_str_base);
        out_idx++;
        input_idx += 2;
    }

    return ASTARTE_RESULT_OK;
}

astarte_result_t uuid_to_string(const uuid_t uuid, char *out, size_t out_size)
{
    size_t min_out_size = UUID_STR_LEN + 1;
    if (out_size < min_out_size) {
        ESP_LOGE(TAG, "Output buffer should be at least %zu bytes long", min_out_size);
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    // NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
    int res = snprintf(out, min_out_size,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1],
        uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11],
        uuid[12], uuid[13], uuid[14], uuid[15]);
    // NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
    if ((res < 0) || (res >= min_out_size)) {
        ESP_LOGE(TAG, "Error converting UUID to string.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }

    return ASTARTE_RESULT_OK;
}

astarte_result_t uuid_to_base64(const uuid_t uuid, char *out, size_t out_size)
{
    size_t min_out_size = UUID_BASE64_LEN + 1;
    if (out_size < min_out_size) {
        ESP_LOGE(TAG, "Output buffer should be at least %zu bytes long", min_out_size);
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    size_t olen = 0;
    int res = mbedtls_base64_encode((unsigned char *) out, out_size, &olen, uuid, UUID_SIZE);
    if (res != 0) {
        ESP_LOGE(TAG, "Error converting UUID to base 64 string, rc: %d.", res);
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }

    return ASTARTE_RESULT_OK;
}

astarte_result_t uuid_to_base64url(const uuid_t uuid, char *out, size_t out_size)
{
    size_t min_out_size = UUID_BASE64URL_LEN + 1;
    if (out_size < min_out_size) {
        ESP_LOGE(TAG, "Output buffer should be at least %zu bytes long", min_out_size);
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    // Convert UUID to RFC 3548/4648 base 64 notation
    size_t olen = 0;
    char uuid_base64[UUID_BASE64_LEN + 1] = { 0 };
    int res = mbedtls_base64_encode(
        (unsigned char *) uuid_base64, UUID_BASE64_LEN + 1, &olen, uuid, UUID_SIZE);
    if (res != 0) {
        ESP_LOGE(TAG, "Error converting UUID to base 64 string, rc: %d.", res);
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }

    // Convert UUID to RFC 4648 sec. 5 URL and filename safe base 64 notation
    for (size_t i = 0; i < UUID_BASE64URL_LEN; i++) {
        if (uuid_base64[i] == '+') {
            uuid_base64[i] = '-';
        }
        if (uuid_base64[i] == '/') {
            uuid_base64[i] = '_';
        }
    }

    memcpy(out, uuid_base64, UUID_BASE64URL_LEN);
    out[UUID_BASE64URL_LEN] = 0;

    return ASTARTE_RESULT_OK;
}

static void overwrite_uuid_version_and_variant(uuid_t uuid, uint8_t version, uint8_t variant)
{
    // Clear the 'ver' and 'var' fields
    uuid[UUID_POSITION_VERSION] &= ~UUID_MASK_VERSION;
    uuid[UUID_POSITION_VARIANT] &= ~UUID_MASK_VARIANT;
    // Update the 'ver' and 'var' fields
    uuid[UUID_POSITION_VERSION] |= (uint8_t) (version << UUID_OFFSET_VERSION);
    uuid[UUID_POSITION_VARIANT] |= (uint8_t) (variant << UUID_OFFSET_VARIANT);
}

static bool should_be_hyphen(uint8_t position)
{
    switch (position) {
        case UUID_STR_POSITION_FIRST_HYPHEN:
        case UUID_STR_POSITION_SECOND_HYPHEN:
        case UUID_STR_POSITION_THIRD_HYPHEN:
        case UUID_STR_POSITION_FOURTH_HYPHEN:
            return true;
        default:
            return false;
    }
}

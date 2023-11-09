/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

#include "uuid.h"

#include <esp_log.h>
#include <esp_system.h>

#ifdef UNIT_TEST
#include <esp_random.h>
#endif

#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <mbedtls/md.h>

#define UUID_STR_LEN 36

#define TAG "uuid"

// All the macros below follow the standard for the Universally Unique IDentifier as defined
// by the IETF in the RFC 4122.
// The full specification can be found here: https://datatracker.ietf.org/doc/html/rfc4122

// Lengths of each component of the UUID in bytes
#define UUID_LEN_TIME_LOW (4)
#define UUID_LEN_TIME_MID (2)
#define UUID_LEN_TIME_HIGH_AND_VERSION (2)
#define UUID_LEN_CLOCK_SEQ_AND_RESERVED (1)
#define UUID_LEN_CLOCK_SEQ_LOW (1)
#define UUID_LEN_NODE (6)

// Offset of each component of the UUID in bytes
#define UUID_OFFSET_TIME_LOW (0)
#define UUID_OFFSET_TIME_MID (4)
#define UUID_OFFSET_TIME_HIGH_AND_VERSION (6)
#define UUID_OFFSET_CLOCK_SEQ_AND_RESERVED (8)
#define UUID_OFFSET_CLOCK_SEQ_LOW (9)
#define UUID_OFFSET_NODE (10)

// Offset of each component of the UUID in string format
#define UUID_STR_OFFSET_TIME_LOW (0)
#define UUID_STR_OFFSET_TIME_MID (9)
#define UUID_STR_OFFSET_TIME_HIGH_AND_VERSION (14)
#define UUID_STR_OFFSET_CLOCK_SEQ_AND_RESERVED (19)
#define UUID_STR_OFFSET_CLOCK_SEQ_LOW (21)
#define UUID_STR_OFFSET_NODE (24)

// Position of the
#define UUID_STR_POSITION_FIRST_HYPHEN (UUID_STR_OFFSET_TIME_MID - sizeof(char))
#define UUID_STR_POSITION_SECOND_HYPHEN (UUID_STR_OFFSET_TIME_HIGH_AND_VERSION - sizeof(char))
#define UUID_STR_POSITION_THIRD_HYPHEN (UUID_STR_OFFSET_CLOCK_SEQ_AND_RESERVED - sizeof(char))
#define UUID_STR_POSITION_FOURTH_HYPHEN (UUID_STR_OFFSET_NODE - sizeof(char))

// Internal element's masks and offsets
#define UUID_TIME_HI_AND_VERSION_MASK_TIME (0x0FFFU)

#define UUID_TIME_HI_AND_VERSION_OFFSET_VERSION (12U)

#define UUID_CLOCK_SEQ_AND_RESERVED_MASK_1 (0x3FU)
#define UUID_CLOCK_SEQ_AND_RESERVED_MASK_2 (0x80U)

// Helper struct to make it easier to access UUID fields
struct uuid
{
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_and_version;
    uint8_t clock_seq_hi_res;
    uint8_t clock_seq_low;
    uint8_t node[UUID_LEN_NODE];
};

static void uuid_from_struct(const struct uuid *input, uuid_t out)
{
    uint32_t tmp32 = 0U;
    uint16_t tmp16 = 0U;
    uint8_t *out_p = out;

    tmp32 = htonl(input->time_low);
    memcpy(out_p + UUID_OFFSET_TIME_LOW, &tmp32, UUID_LEN_TIME_LOW);

    tmp16 = htons(input->time_mid);
    memcpy(out_p + UUID_OFFSET_TIME_MID, &tmp16, UUID_LEN_TIME_MID);

    tmp16 = htons(input->time_hi_and_version);
    memcpy(out_p + UUID_OFFSET_TIME_HIGH_AND_VERSION, &tmp16, UUID_LEN_TIME_HIGH_AND_VERSION);

    out_p[UUID_OFFSET_CLOCK_SEQ_AND_RESERVED] = input->clock_seq_hi_res;
    out_p[UUID_OFFSET_CLOCK_SEQ_LOW] = input->clock_seq_low;

    memcpy(out_p + UUID_OFFSET_NODE, input->node, UUID_LEN_NODE);
}

static void uuid_to_struct(const uuid_t input, struct uuid *out)
{
    uint32_t tmp32 = 0U;
    uint16_t tmp16 = 0U;
    const uint8_t *in_p = input;

    memcpy(&tmp32, in_p + UUID_OFFSET_TIME_LOW, UUID_LEN_TIME_LOW);
    out->time_low = ntohl(tmp32);

    memcpy(&tmp16, in_p + UUID_OFFSET_TIME_MID, UUID_LEN_TIME_MID);
    out->time_mid = ntohs(tmp16);

    memcpy(&tmp16, in_p + UUID_OFFSET_TIME_HIGH_AND_VERSION, UUID_LEN_TIME_HIGH_AND_VERSION);
    out->time_hi_and_version = ntohs(tmp16);

    out->clock_seq_hi_res = in_p[UUID_OFFSET_CLOCK_SEQ_AND_RESERVED];
    out->clock_seq_low = in_p[UUID_OFFSET_CLOCK_SEQ_LOW];

    memcpy(out->node, in_p + UUID_OFFSET_NODE, UUID_LEN_NODE);
}

astarte_err_t uuid_generate_v5(const uuid_t namespace, const void *data, size_t length, uuid_t out)
{
    const size_t sha_256_bytes = 32;
    uint8_t sha_result[sha_256_bytes];

    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);

    mbedtls_md_init(&ctx);
    int mbedtls_err = mbedtls_md_setup(&ctx, md_info, 0);
    // NOLINTBEGIN(hicpp-signed-bitwise) Only using the mbedtls_err to check if zero
    mbedtls_err |= mbedtls_md_starts(&ctx);
    mbedtls_err |= mbedtls_md_update(&ctx, namespace, UUID_LEN);
    mbedtls_err |= mbedtls_md_update(&ctx, data, length);
    mbedtls_err |= mbedtls_md_finish(&ctx, sha_result);
    // NOLINTEND(hicpp-signed-bitwise)
    mbedtls_md_free(&ctx);
    if (mbedtls_err != 0) {
        ESP_LOGE(TAG, "UUID V5 generation failed.");
        return ASTARTE_ERR;
    }

    struct uuid sha_uuid_struct;
    // This will use the first 16 bytes of the SHA-256
    uuid_to_struct(sha_result, &sha_uuid_struct);

    const unsigned int version = 5U;
    sha_uuid_struct.time_hi_and_version &= UUID_TIME_HI_AND_VERSION_MASK_TIME;
    sha_uuid_struct.time_hi_and_version |= (version << UUID_TIME_HI_AND_VERSION_OFFSET_VERSION);
    sha_uuid_struct.clock_seq_hi_res &= UUID_CLOCK_SEQ_AND_RESERVED_MASK_1;
    sha_uuid_struct.clock_seq_hi_res |= UUID_CLOCK_SEQ_AND_RESERVED_MASK_2;

    uuid_from_struct(&sha_uuid_struct, out);

    return ASTARTE_OK;
}

astarte_err_t uuid_to_string(const uuid_t uuid, char *out)
{
    struct uuid uuid_struct;

    uuid_to_struct(uuid, &uuid_struct);

    int res = snprintf(out, UUID_STR_LEN + sizeof(char),
        "%08" PRIx32 "-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid_struct.time_low,
        uuid_struct.time_mid, uuid_struct.time_hi_and_version, uuid_struct.clock_seq_hi_res,
        uuid_struct.clock_seq_low, uuid_struct.node[0], uuid_struct.node[1], uuid_struct.node[2],
        uuid_struct.node[3], uuid_struct.node[4],
        uuid_struct.node[5]); // NOLINT(readability-magic-numbers)
    if ((res < 0) || (res >= UUID_STR_LEN + sizeof(char))) {
        ESP_LOGE(TAG, "Error converting UUID to string.");
        return ASTARTE_ERR;
    }
    return ASTARTE_OK;
}

int uuid_from_string(const char *input, uuid_t out)
{
    // Length check
    if (strlen(input) != UUID_STR_LEN) {
        return -1;
    }

    // Sanity check
    for (int i = 0; i < UUID_STR_LEN; i++) {
        char char_i = input[i];
        // Check that hyphens are in the right place
        if ((i == UUID_STR_POSITION_FIRST_HYPHEN) || (i == UUID_STR_POSITION_SECOND_HYPHEN)
            || (i == UUID_STR_POSITION_THIRD_HYPHEN) || (i == UUID_STR_POSITION_FOURTH_HYPHEN)) {
            if (char_i != '-') {
                ESP_LOGW(TAG, "Found invalid character %c in hyphen position %d", char_i, i);
                return -1;
            }
            continue;
        }

        // Check that everything else is an hexadecimal digit
        if (!((char_i >= '0') && (char_i <= '9')) && !((char_i >= 'a') && (char_i <= 'f'))
            && !((char_i >= 'A') && (char_i <= 'F'))) {
            ESP_LOGW(TAG, "Found invalid character %c in position %d", char_i, i);
            return -1;
        }
    }

    // Will be used to contain the string representation of a uint16_t (plus the NULL terminator)
    char tmp[sizeof(uint16_t) + sizeof(uint8_t)];
    struct uuid uuid_struct;
    const int strtoul_base = 16;

    uuid_struct.time_low = strtoul(input + UUID_STR_OFFSET_TIME_LOW, NULL, strtoul_base);
    uuid_struct.time_mid = strtoul(input + UUID_STR_OFFSET_TIME_MID, NULL, strtoul_base);
    uuid_struct.time_hi_and_version
        = strtoul(input + UUID_STR_OFFSET_TIME_HIGH_AND_VERSION, NULL, strtoul_base);

    tmp[0] = input[UUID_STR_OFFSET_CLOCK_SEQ_AND_RESERVED];
    tmp[1] = input[UUID_STR_OFFSET_CLOCK_SEQ_AND_RESERVED + sizeof(char)];
    uuid_struct.clock_seq_hi_res = strtoul(tmp, NULL, strtoul_base);

    tmp[0] = input[UUID_STR_OFFSET_CLOCK_SEQ_LOW];
    tmp[1] = input[UUID_STR_OFFSET_CLOCK_SEQ_LOW + sizeof(char)];
    uuid_struct.clock_seq_low = strtoul(tmp, NULL, strtoul_base);

    for (int i = 0; i < UUID_LEN_NODE; i++) {
        tmp[0] = input[UUID_STR_OFFSET_NODE + i * sizeof(uint16_t)];
        tmp[1] = input[UUID_STR_OFFSET_NODE + i * sizeof(uint16_t) + 1];
        uuid_struct.node[i] = strtoul(tmp, NULL, strtoul_base);
    }

    uuid_from_struct(&uuid_struct, out);
    return 0;
}

void uuid_generate_v4(uuid_t out)
{
    uint8_t random_result[UUID_LEN];
    esp_fill_random(random_result, UUID_LEN);

    struct uuid random_uuid_struct;
    uuid_to_struct(random_result, &random_uuid_struct);

    const unsigned int version = 4U;
    random_uuid_struct.time_hi_and_version &= UUID_TIME_HI_AND_VERSION_MASK_TIME;
    random_uuid_struct.time_hi_and_version |= (version << UUID_TIME_HI_AND_VERSION_OFFSET_VERSION);

    uuid_from_struct(&random_uuid_struct, out);
}

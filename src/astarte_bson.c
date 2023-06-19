/*
 * (C) Copyright 2016-2021, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#include <astarte_bson.h>

#include <astarte_bson_types.h>

#include <endian.h>
#include <stdio.h>
#include <string.h>

#include <esp_log.h>

#define TAG "ASTARTE_BSON"

static uint32_t read_uint32(const void *buff)
{
    const unsigned char *bytes = (const unsigned char *) buff;
    return le32toh(((uint32_t) bytes[0]) | (((uint32_t) bytes[1]) << 8U)
        | (((uint32_t) bytes[2]) << 16U) | (((uint32_t) bytes[3]) << 24U));
}

static uint64_t read_uint64(const void *buff)
{
    const unsigned char *bytes = (const unsigned char *) buff;
    return le64toh((uint64_t) bytes[0] | ((uint64_t) bytes[1] << 8U) | ((uint64_t) bytes[2] << 16U)
        | ((uint64_t) bytes[3] << 24U) | ((uint64_t) bytes[4] << 32U) | ((uint64_t) bytes[5] << 40U)
        | ((uint64_t) bytes[6] << 48U) | ((uint64_t) bytes[7] << 56U));
}

static unsigned int astarte_bson_next_item_offset(
    unsigned int offset, unsigned int key_len, const void *document)
{
    const char *doc_bytes = (const char *) document;
    uint8_t element_type = (uint8_t) doc_bytes[offset];

    /* offset <- type (uint8_t) + key (const char *) + '\0' (char) */
    offset += 1 + key_len + 1;

    switch (element_type) {
        case BSON_TYPE_STRING: {
            uint32_t string_len = read_uint32(doc_bytes + offset);
            offset += string_len + 4;
            break;
        }

        case BSON_TYPE_ARRAY:
        case BSON_TYPE_DOCUMENT: {
            uint32_t doc_len = read_uint32(doc_bytes + offset);
            offset += doc_len;
            break;
        }

        case BSON_TYPE_BINARY: {
            uint32_t bin_len = read_uint32(doc_bytes + offset);
            offset += 4 + 1 + bin_len; /* int32 (len) + byte (subtype) + bin_len */
            break;
        }

        case BSON_TYPE_INT32: {
            offset += sizeof(int32_t);
            break;
        }

        case BSON_TYPE_DOUBLE:
        case BSON_TYPE_DATETIME:
        case BSON_TYPE_INT64: {
            offset += sizeof(int64_t);
            break;
        }

        case BSON_TYPE_BOOLEAN: {
            offset += 1;
            break;
        }

        default: {
            ESP_LOGW(TAG, "unrecognized BSON type: %i", (int) element_type);
            return 0;
        }
    }

    return offset;
}

const void *astarte_bson_key_lookup(const char *key, const void *document, uint8_t *type)
{
    const char *doc_bytes = (const char *) document;
    uint32_t doc_len = read_uint32(document);

    /* TODO: it would be nice to check len validity here */

    unsigned int offset = 4;
    while (offset + 1 < doc_len) {
        uint8_t element_type = (uint8_t) doc_bytes[offset];
        size_t key_len = strnlen(doc_bytes + offset + 1, doc_len - offset);

        if (strncmp(key, doc_bytes + offset + 1, doc_len - offset) == 0) {
            if (type) {
                *type = element_type;
            }
            return (void *) (doc_bytes + offset + 1 + key_len + 1);
        }

        unsigned int new_offset = astarte_bson_next_item_offset(offset, key_len, document);
        if (!new_offset) {
            return NULL;
        }
        offset = new_offset;
    }

    return NULL;
}

void *astarte_bson_next_item(const void *document, const void *current_item)
{
    const char *doc_bytes = (const char *) document;
    uint32_t doc_len = read_uint32(document);
    unsigned int offset = ((const char *) current_item) - doc_bytes;

    if (offset + 1 >= doc_len) {
        return NULL;
    }

    size_t key_len = strnlen(doc_bytes + offset + 1, doc_len - offset);
    unsigned int new_offset = astarte_bson_next_item_offset(offset, key_len, document);

    if (!new_offset) {
        return NULL;
    }

    if (new_offset + 1 >= doc_len) {
        return NULL;
    }

    return ((char *) doc_bytes) + new_offset;
}

const void *astarte_bson_first_item(const void *document)
{
    const char *doc_bytes = (const char *) document;
    return doc_bytes + 4;
}

const char *astarte_bson_key(const void *item)
{
    return ((const char *) item) + 1;
}

const char *astarte_bson_value_to_string(const void *value_ptr, uint32_t *len)
{
    const char *value_bytes = (const char *) value_ptr;
    uint32_t string_len = read_uint32(value_bytes);

    if (len) {
        *len = string_len - 1;
    }

    return value_bytes + 4;
}

const char *astarte_bson_value_to_binary(const void *value_ptr, uint32_t *len)
{
    const char *value_bytes = (const char *) value_ptr;
    uint32_t bin_len = read_uint32(value_bytes);

    if (len) {
        *len = bin_len;
    }

    return value_bytes + 5;
}

const void *astarte_bson_value_to_document(const void *value_ptr, uint32_t *len)
{
    const char *value_bytes = (const char *) value_ptr;
    uint32_t bin_len = read_uint32(value_bytes);

    if (len) {
        *len = bin_len;
    }

    return value_bytes;
}

int8_t astarte_bson_value_to_int8(const void *value_ptr)
{
    return ((int8_t *) value_ptr)[0];
}

int32_t astarte_bson_value_to_int32(const void *value_ptr)
{
    return (int32_t) read_uint32(value_ptr);
}

int64_t astarte_bson_value_to_int64(const void *value_ptr)
{
    return (int64_t) read_uint64(value_ptr);
}

double astarte_bson_value_to_double(const void *value_ptr)
{
    union data64
    {
        uint64_t u64value;
        double dvalue;
    } value;
    value.u64value = read_uint64(value_ptr);
    return value.dvalue;
}

bool astarte_bson_check_validity(const void *document, unsigned int file_size)
{
    const char *doc_bytes = (const char *) document;
    uint32_t doc_len = read_uint32(document);

    if (!file_size) {
        ESP_LOGW(TAG, "Empty buffer: no BSON document found");
        return false;
    }

    if ((doc_len == 5) && (file_size >= 5) && (doc_bytes[4] == 0)) {
        // empty document
        return true;
    }

    if (file_size < 4 + 1 + 2 + 1) {
        ESP_LOGW(TAG, "BSON data too small");
        return false;
    }

    if (doc_len > file_size) {
        ESP_LOGW(TAG, "BSON document is bigger than data: data: %ui document: %i", file_size,
            (int) doc_len);
        return false;
    }

    if (doc_bytes[doc_len - 1] != 0) {
        ESP_LOGW(TAG, "BSON document is not terminated by null byte.");
        return false;
    }

    int offset = 4;
    switch (doc_bytes[offset]) {
        case BSON_TYPE_DOUBLE:
        case BSON_TYPE_STRING:
        case BSON_TYPE_DOCUMENT:
        case BSON_TYPE_ARRAY:
        case BSON_TYPE_BINARY:
        case BSON_TYPE_BOOLEAN:
        case BSON_TYPE_DATETIME:
        case BSON_TYPE_INT32:
        case BSON_TYPE_INT64:
            break;

        default:
            ESP_LOGW(TAG, "Unrecognized BSON document first type\n");
            return false;
    }

    return true;
}

uint32_t astarte_bson_document_size(const void *document)
{
    return read_uint32(document);
}

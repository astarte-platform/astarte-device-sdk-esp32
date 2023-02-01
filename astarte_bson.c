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

static uint32_t read_uint32(const void *u)
{
    const unsigned char *b = (const unsigned char *) u;
    return le32toh(((uint32_t) b[0]) | (((uint32_t) b[1]) << 8) | (((uint32_t) b[2]) << 16)
        | (((uint32_t) b[3]) << 24));
}

static uint64_t read_uint64(const void *u)
{
    const unsigned char *b = (const unsigned char *) u;
    return le64toh((uint64_t) b[0] | ((uint64_t) b[1] << 8) | ((uint64_t) b[2] << 16)
        | ((uint64_t) b[3] << 24) | ((uint64_t) b[4] << 32) | ((uint64_t) b[5] << 40)
        | ((uint64_t) b[6] << 48) | ((uint64_t) b[7] << 56));
}

static unsigned int astarte_bson_next_item_offset(
    unsigned int offset, unsigned int keyLen, const void *document)
{
    const char *docBytes = (const char *) document;
    uint8_t elementType = (uint8_t) docBytes[offset];

    /* offset <- type (uint8_t) + key (const char *) + '\0' (char) */
    offset += 1 + keyLen + 1;

    switch (elementType) {
        case BSON_TYPE_STRING: {
            uint32_t stringLen = read_uint32(docBytes + offset);
            offset += stringLen + 4;
            break;
        }

        case BSON_TYPE_ARRAY:
        case BSON_TYPE_DOCUMENT: {
            uint32_t docLen = read_uint32(docBytes + offset);
            offset += docLen;
            break;
        }

        case BSON_TYPE_BINARY: {
            uint32_t binLen = read_uint32(docBytes + offset);
            offset += 4 + 1 + binLen; /* int32 (len) + byte (subtype) + binLen */
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
            ESP_LOGW(TAG, "unrecognized BSON type: %i", (int) elementType);
            return 0;
        }
    }

    return offset;
}

const void *astarte_bson_key_lookup(const char *key, const void *document, uint8_t *type)
{
    const char *docBytes = (const char *) document;
    uint32_t docLen = read_uint32(document);

    /* TODO: it would be nice to check len validity here */

    unsigned int offset = 4;
    while (offset + 1 < docLen) {
        uint8_t elementType = (uint8_t) docBytes[offset];
        int keyLen = strnlen(docBytes + offset + 1, docLen - offset);

        if (!strncmp(key, docBytes + offset + 1, docLen - offset)) {
            if (type) {
                *type = elementType;
            }
            return (void *) (docBytes + offset + 1 + keyLen + 1);
        }

        unsigned int newOffset = astarte_bson_next_item_offset(offset, keyLen, document);
        if (!newOffset) {
            return NULL;
        }
        offset = newOffset;
    }

    return NULL;
}

void *astarte_bson_next_item(const void *document, const void *current_item)
{
    const char *docBytes = (const char *) document;
    uint32_t docLen = read_uint32(document);
    unsigned int offset = ((const char *) current_item) - docBytes;

    if (offset + 1 >= docLen) {
        return NULL;
    }

    int keyLen = strnlen(docBytes + offset + 1, docLen - offset);
    unsigned int newOffset = astarte_bson_next_item_offset(offset, keyLen, document);

    if (!newOffset) {
        return NULL;
    }

    if (newOffset + 1 >= docLen) {
        return NULL;
    }

    return ((char *) docBytes) + newOffset;
}

const void *astarte_bson_first_item(const void *document)
{
    const char *docBytes = (const char *) document;
    return docBytes + 4;
}

const char *astarte_bson_key(const void *item)
{
    return ((const char *) item) + 1;
}

const char *astarte_bson_value_to_string(const void *valuePtr, uint32_t *len)
{
    const char *valueBytes = (const char *) valuePtr;
    uint32_t stringLen = read_uint32(valueBytes);

    if (len) {
        *len = stringLen - 1;
    }

    return valueBytes + 4;
}

const char *astarte_bson_value_to_binary(const void *valuePtr, uint32_t *len)
{
    const char *valueBytes = (const char *) valuePtr;
    uint32_t binLen = read_uint32(valueBytes);

    if (len) {
        *len = binLen;
    }

    return valueBytes + 5;
}

const void *astarte_bson_value_to_document(const void *valuePtr, uint32_t *len)
{
    const char *valueBytes = (const char *) valuePtr;
    uint32_t binLen = read_uint32(valueBytes);

    if (len) {
        *len = binLen;
    }

    return valueBytes;
}

int8_t astarte_bson_value_to_int8(const void *valuePtr)
{
    return ((int8_t *) valuePtr)[0];
}

int32_t astarte_bson_value_to_int32(const void *valuePtr)
{
    return (int32_t) read_uint32(valuePtr);
}

int64_t astarte_bson_value_to_int64(const void *valuePtr)
{
    return (int64_t) read_uint64(valuePtr);
}

double astarte_bson_value_to_double(const void *valuePtr)
{
    union data64
    {
        uint64_t u64value;
        double dvalue;
    } v;
    v.u64value = read_uint64(valuePtr);
    return v.dvalue;
}

bool astarte_bson_check_validity(const void *document, unsigned int fileSize)
{
    const char *docBytes = (const char *) document;
    uint32_t docLen = read_uint32(document);
    int offset;

    if (!fileSize) {
        ESP_LOGW(TAG, "Empty buffer: no BSON document found");
        return false;
    }

    if ((docLen == 5) && (fileSize >= 5) && (docBytes[4] == 0)) {
        // empty document
        return true;
    }

    if (fileSize < 4 + 1 + 2 + 1) {
        ESP_LOGW(TAG, "BSON data too small");
        return false;
    }

    if (docLen > fileSize) {
        ESP_LOGW(TAG, "BSON document is bigger than data: data: %ui document: %i", fileSize,
            (int) docLen);
        return false;
    }

    if (docBytes[docLen - 1] != 0) {
        ESP_LOGW(TAG, "BSON document is not terminated by null byte.");
        return false;
    }

    offset = 4;
    switch (docBytes[offset]) {
        case BSON_TYPE_DOUBLE:
        case BSON_TYPE_STRING:
        case BSON_TYPE_DOCUMENT:
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

int32_t astarte_bson_document_size(const void *document)
{
    return read_uint32(document);
}

/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#include <astarte_bson_serializer.h>

#include <astarte_bson_types.h>

#include <esp_log.h>

#include <endian.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TAG "ASTARTE_BSON_SERIALIZER"

#define INT32_TO_BYTES(value, buf) \
    union data32 { \
        int64_t sval; \
        uint64_t uval; \
        uint8_t valBuf[4]; \
    }; \
    union data32 d32; \
    d32.sval = (value); \
    d32.uval = htole32(d32.uval); \
    buf = d32.valBuf;

#define INT64_TO_BYTES(value, buf) \
    union data64 { \
        int64_t sval; \
        uint64_t uval; \
        uint8_t valBuf[8]; \
    }; \
    union data64 d64; \
    d64.sval = (value); \
    d64.uval = htole64(d64.uval); \
    buf = d64.valBuf;

#define DOUBLE_TO_BYTES(value, buf) \
    union data64 { \
        double dval; \
        uint64_t uval; \
        uint8_t valBuf[8]; \
    }; \
    union data64 d64; \
    d64.dval = (value); \
    d64.uval = htole64(d64.uval); \
    buf = d64.valBuf;

static void astarte_byte_array_init(struct astarte_byte_array_t *ba, void *bytes, int size)
{
    ba->capacity = size;
    ba->size = size;
    ba->buf = malloc(size);

    if (!ba->buf) {
        ESP_LOGE(TAG, "Cannot allocate memory for BSON payload (size: %i)!", size);
        abort();
    }

    memcpy(ba->buf, bytes, size);
}

static void astarte_byte_array_destroy(struct astarte_byte_array_t *ba)
{
    ba->capacity = 0;
    ba->size = 0;
    free(ba->buf);
    ba->buf = 0;
}

static void astarte_byte_array_grow(struct astarte_byte_array_t *ba, int needed)
{
    if (ba->size + needed >= ba->capacity) {
        int new_capacity = ba->capacity * 2;
        if (new_capacity < ba->capacity + needed) {
            new_capacity = ba->capacity + needed;
        }
        ba->capacity = new_capacity;
        void *new_buf = malloc(new_capacity);
        memcpy(new_buf, ba->buf, ba->size);
        free(ba->buf);
        ba->buf = new_buf;
    }
}

static void astarte_byte_array_append_byte(struct astarte_byte_array_t *ba, uint8_t byte)
{
    astarte_byte_array_grow(ba, 1);
    ba->buf[ba->size] = byte;
    ba->size++;
}

static void astarte_byte_array_append(struct astarte_byte_array_t *ba, const void *bytes, int count)
{
    astarte_byte_array_grow(ba, count);

    memcpy(ba->buf + ba->size, bytes, count);
    ba->size += count;
}

static void astarte_byte_array_replace(struct astarte_byte_array_t *ba, int pos, int count, const uint8_t *bytes)
{
    memcpy(ba->buf + pos, bytes, count);
}


void astarte_bson_serializer_init(struct astarte_bson_serializer_t *bs)
{
    astarte_byte_array_init(&bs->ba, "\0\0\0\0", 4);
}

void astarte_bson_serializer_destroy(struct astarte_bson_serializer_t *bs)
{
    astarte_byte_array_destroy(&bs->ba);
}

const void *astarte_bson_serializer_get_document(const struct astarte_bson_serializer_t *bs, int *size)
{
    *size = bs->ba.size;
    return bs->ba.buf;
}

astarte_err_t astarte_bson_serializer_write_document(const struct astarte_bson_serializer_t *bs, void *out_buf, int out_buf_len, int *out_doc_size)
{
    int doc_size = bs->ba.size;
    *out_doc_size = doc_size;

    if (out_buf_len < bs->ba.size) {
        return ASTARTE_ERR;
    }

    memcpy(out_buf, bs->ba.buf, doc_size);

    return ASTARTE_OK;
}

int astarte_bson_serializer_document_size(const struct astarte_bson_serializer_t *bs)
{
    return bs->ba.size;
}

void astarte_bson_serializer_append_end_of_document(struct astarte_bson_serializer_t *bs)
{
    astarte_byte_array_append_byte(&bs->ba, '\0');

    uint8_t *sizeBuf;
    INT32_TO_BYTES(bs->ba.size, sizeBuf)

    astarte_byte_array_replace(&bs->ba, 0, sizeof(int32_t), sizeBuf);
}

void astarte_bson_serializer_append_double(struct astarte_bson_serializer_t *bs, const char *name, double value)
{
    uint8_t *valBuf;
    DOUBLE_TO_BYTES(value, valBuf)

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_DOUBLE);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bs->ba, valBuf, 8);
}

void astarte_bson_serializer_append_int32(struct astarte_bson_serializer_t *bs, const char *name, int32_t value)
{
    uint8_t *valBuf;
    INT32_TO_BYTES(value, valBuf)

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_INT32);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bs->ba, valBuf, sizeof(int32_t));
}

void astarte_bson_serializer_append_int64(struct astarte_bson_serializer_t *bs, const char *name, int64_t value)
{
    uint8_t *valBuf;
    INT64_TO_BYTES(value, valBuf)

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_INT64);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bs->ba, valBuf, sizeof(int64_t));
}

void astarte_bson_serializer_append_binary(struct astarte_bson_serializer_t *bs, const char *name, void *value, int size)
{
    uint8_t *lenBuf;
    INT32_TO_BYTES(size, lenBuf)

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_BINARY);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bs->ba, lenBuf, sizeof(int32_t));
    astarte_byte_array_append_byte(&bs->ba, BSON_SUBTYPE_DEFAULT_BINARY);
    astarte_byte_array_append(&bs->ba, value, size);
}

void astarte_bson_serializer_append_string(struct astarte_bson_serializer_t *bs, const char *name, const char *string)
{
    int string_len = strlen(string);

    uint8_t *lenBuf;
    INT32_TO_BYTES(string_len + 1, lenBuf)

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_STRING);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bs->ba, lenBuf, sizeof(int32_t));
    astarte_byte_array_append(&bs->ba, string, string_len + 1);
}

void astarte_bson_serializer_append_datetime(struct astarte_bson_serializer_t *bs, const char *name, uint64_t epoch_millis)
{
    uint8_t *valBuf;
    INT64_TO_BYTES(epoch_millis, valBuf)

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_DATETIME);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bs->ba, valBuf, sizeof(int64_t));
}

void astarte_bson_serializer_append_boolean(struct astarte_bson_serializer_t *bs, const char *name, int value)
{
    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_BOOLEAN);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append_byte(&bs->ba, value ? '\1' : '\0');
}

void astarte_bson_serializer_append_document(struct astarte_bson_serializer_t *bs, const char *name, const void *document)
{
    uint32_t size;
    memcpy(&size, document, sizeof(uint32_t));
    size = le64toh(size);

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_DOCUMENT);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);

    astarte_byte_array_append(&bs->ba, document, size);
}

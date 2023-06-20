/*
 * (C) Copyright 2018-2021, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#include <astarte_bson_serializer.h>

#include <astarte_bson_types.h>

#include <esp_log.h>

#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "ASTARTE_BSON_SERIALIZER"

static void uint32_to_bytes(uint32_t input, uint8_t out[static 4])
{
    uint32_t tmp = htole32(input);
    for (size_t i = 0; i < 4; i++) {
        out[i] = ((uint8_t *) &tmp)[i];
    }
}

static void int32_to_bytes(int32_t input, uint8_t out[static 4])
{
    uint32_t tmp = htole32(*((uint32_t *) &input));
    for (size_t i = 0; i < 4; i++) {
        out[i] = ((uint8_t *) &tmp)[i];
    }
}

static void uint64_to_bytes(uint64_t input, uint8_t out[static 8])
{
    uint64_t tmp = htole64(input);
    for (size_t i = 0; i < 8; i++) {
        out[i] = ((uint8_t *) &tmp)[i];
    }
}

static void int64_to_bytes(int64_t input, uint8_t out[static 8])
{
    uint64_t tmp = htole64(*((uint64_t *) &input));
    for (size_t i = 0; i < 8; i++) {
        out[i] = ((uint8_t *) &tmp)[i];
    }
}

static void double_to_bytes(double input, uint8_t out[static 8])
{
    uint64_t tmp = htole64(*((uint64_t *) &input));
    for (size_t i = 0; i < 8; i++) {
        out[i] = ((uint8_t *) &tmp)[i];
    }
}

static void astarte_byte_array_init(struct astarte_byte_array_t *ba, void *bytes, size_t size)
{
    ba->capacity = size;
    ba->size = size;
    ba->buf = malloc(size);

    if (!ba->buf) {
        ESP_LOGE(TAG, "Cannot allocate memory for BSON payload (size: %zu)!", size);
        abort();
    }

    memcpy(ba->buf, bytes, size);
}

static void astarte_byte_array_destroy(struct astarte_byte_array_t *ba)
{
    ba->capacity = 0;
    ba->size = 0;
    free(ba->buf);
    ba->buf = NULL;
}

static void astarte_byte_array_grow(struct astarte_byte_array_t *ba, size_t needed)
{
    if (ba->size + needed >= ba->capacity) {
        size_t new_capacity = ba->capacity * 2;
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
    astarte_byte_array_grow(ba, sizeof(uint8_t));
    ba->buf[ba->size] = byte;
    ba->size++;
}

static void astarte_byte_array_append(
    struct astarte_byte_array_t *ba, const void *bytes, size_t count)
{
    astarte_byte_array_grow(ba, count);

    memcpy(ba->buf + ba->size, bytes, count);
    ba->size += count;
}

static void astarte_byte_array_replace(
    struct astarte_byte_array_t *ba, unsigned int pos, size_t count, const uint8_t *bytes)
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

const void *astarte_bson_serializer_get_document(
    const struct astarte_bson_serializer_t *bs, size_t *size)
{
    if (size) {
        *size = bs->ba.size;
    }
    return bs->ba.buf;
}

astarte_err_t astarte_bson_serializer_write_document(const struct astarte_bson_serializer_t *bs,
    void *out_buf, int out_buf_len, size_t *out_doc_size)
{
    size_t doc_size = bs->ba.size;
    if (out_doc_size) {
        *out_doc_size = doc_size;
    }

    if (out_buf_len < bs->ba.size) {
        return ASTARTE_ERR;
    }

    memcpy(out_buf, bs->ba.buf, doc_size);

    return ASTARTE_OK;
}

size_t astarte_bson_serializer_document_size(const struct astarte_bson_serializer_t *bs)
{
    return bs->ba.size;
}

void astarte_bson_serializer_append_end_of_document(struct astarte_bson_serializer_t *bs)
{
    astarte_byte_array_append_byte(&bs->ba, '\0');

    uint8_t sizeBuf[4];
    uint32_to_bytes(bs->ba.size, sizeBuf);

    astarte_byte_array_replace(&bs->ba, 0, sizeof(int32_t), sizeBuf);
}

void astarte_bson_serializer_append_double(
    struct astarte_bson_serializer_t *bs, const char *name, double value)
{
    uint8_t valBuf[8];
    double_to_bytes(value, valBuf);

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_DOUBLE);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bs->ba, valBuf, 8);
}

void astarte_bson_serializer_append_int32(
    struct astarte_bson_serializer_t *bs, const char *name, int32_t value)
{
    uint8_t valBuf[4];
    int32_to_bytes(value, valBuf);

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_INT32);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bs->ba, valBuf, sizeof(int32_t));
}

void astarte_bson_serializer_append_int64(
    struct astarte_bson_serializer_t *bs, const char *name, int64_t value)
{
    uint8_t valBuf[8];
    int64_to_bytes(value, valBuf);

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_INT64);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bs->ba, valBuf, sizeof(int64_t));
}

void astarte_bson_serializer_append_binary(
    struct astarte_bson_serializer_t *bs, const char *name, const void *value, size_t size)
{
    uint8_t lenBuf[4];
    uint32_to_bytes(size, lenBuf);

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_BINARY);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bs->ba, lenBuf, sizeof(int32_t));
    astarte_byte_array_append_byte(&bs->ba, BSON_SUBTYPE_DEFAULT_BINARY);
    astarte_byte_array_append(&bs->ba, value, size);
}

void astarte_bson_serializer_append_string(
    struct astarte_bson_serializer_t *bs, const char *name, const char *string)
{
    size_t string_len = strlen(string);

    uint8_t lenBuf[4];
    uint32_to_bytes(string_len + 1, lenBuf);

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_STRING);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bs->ba, lenBuf, sizeof(int32_t));
    astarte_byte_array_append(&bs->ba, string, string_len + 1);
}

void astarte_bson_serializer_append_datetime(
    struct astarte_bson_serializer_t *bs, const char *name, uint64_t epoch_millis)
{
    uint8_t valBuf[8];
    uint64_to_bytes(epoch_millis, valBuf);

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_DATETIME);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bs->ba, valBuf, sizeof(int64_t));
}

void astarte_bson_serializer_append_boolean(
    struct astarte_bson_serializer_t *bs, const char *name, bool value)
{
    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_BOOLEAN);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);
    astarte_byte_array_append_byte(&bs->ba, value ? '\1' : '\0');
}

void astarte_bson_serializer_append_document(
    struct astarte_bson_serializer_t *bs, const char *name, const void *document)
{
    uint32_t size = 0U;
    memcpy(&size, document, sizeof(uint32_t));
    size = le32toh(size);

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_DOCUMENT);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);

    astarte_byte_array_append(&bs->ba, document, size);
}

#define IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(TYPE, TYPE_NAME)                       \
    void astarte_bson_serializer_append_##TYPE_NAME##_array(                                       \
        struct astarte_bson_serializer_t *bs, const char *name, TYPE arr, int count)               \
    {                                                                                              \
        struct astarte_bson_serializer_t array_ser;                                                \
        astarte_bson_serializer_init(&array_ser);                                                  \
        for (int i = 0; i < count; i++) {                                                          \
            char key[12];                                                                          \
            snprintf(key, 12, "%i", i);                                                            \
            astarte_bson_serializer_append_##TYPE_NAME(&array_ser, key, arr[i]);                   \
        }                                                                                          \
        astarte_bson_serializer_append_end_of_document(&array_ser);                                \
                                                                                                   \
        size_t size;                                                                               \
        const void *document = astarte_bson_serializer_get_document(&array_ser, &size);            \
                                                                                                   \
        astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_ARRAY);                                  \
        astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);                                \
                                                                                                   \
        astarte_byte_array_append(&bs->ba, document, size);                                        \
                                                                                                   \
        astarte_bson_serializer_destroy(&array_ser);                                               \
    }

IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const double *, double)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const int32_t *, int32)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const int64_t *, int64)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const char *const *, string)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const int64_t *, datetime)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const bool *, boolean)

void astarte_bson_serializer_append_binary_array(struct astarte_bson_serializer_t *bs,
    const char *name, const void *const *arr, const int *sizes, int count)
{
    struct astarte_bson_serializer_t array_ser;
    astarte_bson_serializer_init(&array_ser);
    for (int i = 0; i < count; i++) {
        char key[12];
        snprintf(key, 12, "%i", i);
        astarte_bson_serializer_append_binary(&array_ser, key, arr[i], sizes[i]);
    }
    astarte_bson_serializer_append_end_of_document(&array_ser);

    size_t size = 0;
    const void *document = astarte_bson_serializer_get_document(&array_ser, &size);

    astarte_byte_array_append_byte(&bs->ba, BSON_TYPE_ARRAY);
    astarte_byte_array_append(&bs->ba, name, strlen(name) + 1);

    astarte_byte_array_append(&bs->ba, document, size);

    astarte_bson_serializer_destroy(&array_ser);
}

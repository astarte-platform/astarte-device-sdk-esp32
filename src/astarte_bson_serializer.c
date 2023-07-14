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

// NOTE: this is a temporary typedef, once the structs astarte_byte_array_t and
// astarte_bson_serializer_t deprecation period ends they should be moved here and renamed to
// astarte_byte_array and astarte_bson_serializer respectively.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
// NOLINTBEGIN(readability-identifier-naming)
typedef struct astarte_byte_array_t astarte_byte_array;
typedef struct astarte_bson_serializer_t astarte_bson_serializer;
// NOLINTEND(readability-identifier-naming)
#pragma GCC diagnostic pop

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

static void astarte_byte_array_init(astarte_byte_array *byte_arr, void *bytes, size_t size)
{
    byte_arr->capacity = size;
    byte_arr->size = size;
    byte_arr->buf = malloc(size);

    if (!byte_arr->buf) {
        ESP_LOGE(TAG, "Cannot allocate memory for BSON payload (size: %zu)!", size);
        abort();
    }

    memcpy(byte_arr->buf, bytes, size);
}

static void astarte_byte_array_destroy(astarte_byte_array *byte_arr)
{
    byte_arr->capacity = 0;
    byte_arr->size = 0;
    free(byte_arr->buf);
    byte_arr->buf = NULL;
}

static void astarte_byte_array_grow(astarte_byte_array *byte_arr, size_t needed)
{
    if (byte_arr->size + needed >= byte_arr->capacity) {
        size_t new_capacity = byte_arr->capacity * 2;
        if (new_capacity < byte_arr->capacity + needed) {
            new_capacity = byte_arr->capacity + needed;
        }
        byte_arr->capacity = new_capacity;
        void *new_buf = malloc(new_capacity);
        memcpy(new_buf, byte_arr->buf, byte_arr->size);
        free(byte_arr->buf);
        byte_arr->buf = new_buf;
    }
}

static void astarte_byte_array_append_byte(astarte_byte_array *byte_arr, uint8_t byte)
{
    astarte_byte_array_grow(byte_arr, sizeof(uint8_t));
    byte_arr->buf[byte_arr->size] = byte;
    byte_arr->size++;
}

static void astarte_byte_array_append(astarte_byte_array *byte_arr, const void *bytes, size_t count)
{
    astarte_byte_array_grow(byte_arr, count);

    memcpy(byte_arr->buf + byte_arr->size, bytes, count);
    byte_arr->size += count;
}

static void astarte_byte_array_replace(
    astarte_byte_array *byte_arr, unsigned int pos, size_t count, const uint8_t *bytes)
{
    memcpy(byte_arr->buf + pos, bytes, count);
}

void astarte_bson_serializer_init(astarte_bson_serializer_handle_t bson)
{
    astarte_byte_array_init(&bson->ba, "\0\0\0\0", 4);
}

astarte_bson_serializer_handle_t astarte_bson_serializer_new(void)
{
    astarte_bson_serializer_handle_t bson = calloc(1, sizeof(astarte_bson_serializer));
    if (!bson) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        return NULL;
    }
    astarte_byte_array_init(&bson->ba, "\0\0\0\0", 4);
    return bson;
}

void astarte_bson_serializer_destroy(astarte_bson_serializer_handle_t bson)
{
    astarte_byte_array_destroy(&bson->ba);
    free(bson);
}

const void *astarte_bson_serializer_get_document(
    astarte_bson_serializer_handle_t bson, size_t *size)
{
    if (size) {
        *size = bson->ba.size;
    }
    return bson->ba.buf;
}

astarte_err_t astarte_bson_serializer_write_document(
    astarte_bson_serializer_handle_t bson, void *out_buf, int out_buf_len, size_t *out_doc_size)
{
    size_t doc_size = bson->ba.size;
    if (out_doc_size) {
        *out_doc_size = doc_size;
    }

    if (out_buf_len < bson->ba.size) {
        return ASTARTE_ERR;
    }

    memcpy(out_buf, bson->ba.buf, doc_size);

    return ASTARTE_OK;
}

size_t astarte_bson_serializer_document_size(astarte_bson_serializer_handle_t bson)
{
    return bson->ba.size;
}

void astarte_bson_serializer_append_end_of_document(astarte_bson_serializer_handle_t bson)
{
    astarte_byte_array_append_byte(&bson->ba, '\0');

    uint8_t size_buf[4];
    uint32_to_bytes(bson->ba.size, size_buf);

    astarte_byte_array_replace(&bson->ba, 0, sizeof(int32_t), size_buf);
}

void astarte_bson_serializer_append_double(
    astarte_bson_serializer_handle_t bson, const char *name, double value)
{
    uint8_t val_buf[8];
    double_to_bytes(value, val_buf);

    astarte_byte_array_append_byte(&bson->ba, BSON_TYPE_DOUBLE);
    astarte_byte_array_append(&bson->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bson->ba, val_buf, 8);
}

void astarte_bson_serializer_append_int32(
    astarte_bson_serializer_handle_t bson, const char *name, int32_t value)
{
    uint8_t val_buf[4];
    int32_to_bytes(value, val_buf);

    astarte_byte_array_append_byte(&bson->ba, BSON_TYPE_INT32);
    astarte_byte_array_append(&bson->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bson->ba, val_buf, sizeof(int32_t));
}

void astarte_bson_serializer_append_int64(
    astarte_bson_serializer_handle_t bson, const char *name, int64_t value)
{
    uint8_t val_buf[8];
    int64_to_bytes(value, val_buf);

    astarte_byte_array_append_byte(&bson->ba, BSON_TYPE_INT64);
    astarte_byte_array_append(&bson->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bson->ba, val_buf, sizeof(int64_t));
}

void astarte_bson_serializer_append_binary(
    astarte_bson_serializer_handle_t bson, const char *name, const void *value, size_t size)
{
    uint8_t len_buf[4];
    uint32_to_bytes(size, len_buf);

    astarte_byte_array_append_byte(&bson->ba, BSON_TYPE_BINARY);
    astarte_byte_array_append(&bson->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bson->ba, len_buf, sizeof(int32_t));
    astarte_byte_array_append_byte(&bson->ba, BSON_SUBTYPE_DEFAULT_BINARY);
    astarte_byte_array_append(&bson->ba, value, size);
}

void astarte_bson_serializer_append_string(
    astarte_bson_serializer_handle_t bson, const char *name, const char *string)
{
    size_t string_len = strlen(string);

    uint8_t len_buf[4];
    uint32_to_bytes(string_len + 1, len_buf);

    astarte_byte_array_append_byte(&bson->ba, BSON_TYPE_STRING);
    astarte_byte_array_append(&bson->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bson->ba, len_buf, sizeof(int32_t));
    astarte_byte_array_append(&bson->ba, string, string_len + 1);
}

void astarte_bson_serializer_append_datetime(
    astarte_bson_serializer_handle_t bson, const char *name, uint64_t epoch_millis)
{
    uint8_t val_buf[8];
    uint64_to_bytes(epoch_millis, val_buf);

    astarte_byte_array_append_byte(&bson->ba, BSON_TYPE_DATETIME);
    astarte_byte_array_append(&bson->ba, name, strlen(name) + 1);
    astarte_byte_array_append(&bson->ba, val_buf, sizeof(uint64_t));
}

void astarte_bson_serializer_append_boolean(
    astarte_bson_serializer_handle_t bson, const char *name, bool value)
{
    astarte_byte_array_append_byte(&bson->ba, BSON_TYPE_BOOLEAN);
    astarte_byte_array_append(&bson->ba, name, strlen(name) + 1);
    astarte_byte_array_append_byte(&bson->ba, value ? '\1' : '\0');
}

void astarte_bson_serializer_append_document(
    astarte_bson_serializer_handle_t bson, const char *name, const void *document)
{
    uint32_t size = 0U;
    memcpy(&size, document, sizeof(uint32_t));
    size = le32toh(size);

    astarte_byte_array_append_byte(&bson->ba, BSON_TYPE_DOCUMENT);
    astarte_byte_array_append(&bson->ba, name, strlen(name) + 1);

    astarte_byte_array_append(&bson->ba, document, size);
}

#define IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(TYPE, TYPE_NAME)                       \
    astarte_err_t astarte_bson_serializer_append_##TYPE_NAME##_array(                              \
        astarte_bson_serializer_handle_t bson, const char *name, TYPE arr, int count)              \
    {                                                                                              \
        astarte_err_t result = ASTARTE_OK;                                                         \
        astarte_bson_serializer_handle_t array_ser = astarte_bson_serializer_new();                \
        for (int i = 0; i < count; i++) {                                                          \
            char key[12];                                                                          \
            int ret = snprintf(key, 12, "%i", i);                                                  \
            if ((ret < 0) || (ret >= 12)) {                                                        \
                result = ASTARTE_ERR;                                                              \
            }                                                                                      \
            astarte_bson_serializer_append_##TYPE_NAME(array_ser, key, arr[i]);                    \
        }                                                                                          \
        astarte_bson_serializer_append_end_of_document(array_ser);                                 \
                                                                                                   \
        size_t size;                                                                               \
        const void *document = astarte_bson_serializer_get_document(array_ser, &size);             \
                                                                                                   \
        astarte_byte_array_append_byte(&bson->ba, BSON_TYPE_ARRAY);                                \
        astarte_byte_array_append(&bson->ba, name, strlen(name) + 1);                              \
                                                                                                   \
        astarte_byte_array_append(&bson->ba, document, size);                                      \
                                                                                                   \
        astarte_bson_serializer_destroy(array_ser);                                                \
                                                                                                   \
        return result;                                                                             \
    }

IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const double *, double)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const int32_t *, int32)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const int64_t *, int64)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const char *const *, string)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const int64_t *, datetime)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const bool *, boolean)

astarte_err_t astarte_bson_serializer_append_binary_array(astarte_bson_serializer_handle_t bson,
    const char *name, const void *const *arr, const int *sizes, int count)
{
    astarte_err_t result = ASTARTE_OK;
    astarte_bson_serializer_handle_t array_ser = astarte_bson_serializer_new();
    for (int i = 0; i < count; i++) {
        char key[12];
        int ret = snprintf(key, 12, "%i", i);
        if ((ret < 0) || (ret >= 12)) {
            result = ASTARTE_ERR;
        }
        astarte_bson_serializer_append_binary(array_ser, key, arr[i], sizes[i]);
    }
    astarte_bson_serializer_append_end_of_document(array_ser);

    size_t size = 0;
    const void *document = astarte_bson_serializer_get_document(array_ser, &size);

    astarte_byte_array_append_byte(&bson->ba, BSON_TYPE_ARRAY);
    astarte_byte_array_append(&bson->ba, name, strlen(name) + 1);

    astarte_byte_array_append(&bson->ba, document, size);

    astarte_bson_serializer_destroy(array_ser);

    return result;
}

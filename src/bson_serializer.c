/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bson_serializer.h"

#include <stdio.h>
#include <stdlib.h>

#include <endian.h>
#include <string.h>

#include <esp_log.h>

#include "bson_types.h"

#define TAG "ASTARTE_BSON_SERIALIZER"

// When serializing a C array into a BSON array, this is the maximum allowed size of the string
// field array length. 12 chars corresponding to 999999999999 elements.
#define BSON_ARRAY_SIZE_STR_LEN 12

static void uint32_to_bytes(uint32_t input, uint8_t out[static sizeof(uint32_t)])
{
    uint32_t tmp = htole32(input);
    for (size_t i = 0; i < sizeof(uint32_t); i++) {
        out[i] = ((uint8_t *) &tmp)[i];
    }
}

static void int32_to_bytes(int32_t input, uint8_t out[static sizeof(int32_t)])
{
    uint32_t tmp = htole32(*((uint32_t *) &input));
    for (size_t i = 0; i < sizeof(uint32_t); i++) {
        out[i] = ((uint8_t *) &tmp)[i];
    }
}

static void uint64_to_bytes(uint64_t input, uint8_t out[static sizeof(uint64_t)])
{
    uint64_t tmp = htole64(input);
    for (size_t i = 0; i < sizeof(uint64_t); i++) {
        out[i] = ((uint8_t *) &tmp)[i];
    }
}

static void int64_to_bytes(int64_t input, uint8_t out[static sizeof(int64_t)])
{
    uint64_t tmp = htole64(*((uint64_t *) &input));
    for (size_t i = 0; i < sizeof(uint64_t); i++) {
        out[i] = ((uint8_t *) &tmp)[i];
    }
}

static void double_to_bytes(double input, uint8_t out[static sizeof(double)])
{
    uint64_t tmp = htole64(*((uint64_t *) &input));
    for (size_t i = 0; i < sizeof(uint64_t); i++) {
        out[i] = ((uint8_t *) &tmp)[i];
    }
}

static astarte_result_t byte_array_init(astarte_bson_serializer_t *bson, void *bytes, size_t size)
{
    bson->capacity = size;
    bson->size = size;
    bson->buf = malloc(size);

    if (!bson->buf) {
        ESP_LOGE(TAG, "Cannot allocate memory for BSON payload (size: %zu)!", size);
        return ASTARTE_RESULT_OUT_OF_MEMORY;
    }

    memcpy(bson->buf, bytes, size);
    return ASTARTE_RESULT_OK;
}

static void byte_array_destroy(astarte_bson_serializer_t *bson)
{
    bson->capacity = 0;
    bson->size = 0;
    free(bson->buf);
    bson->buf = NULL;
}

static void byte_array_grow(astarte_bson_serializer_t *bson, size_t needed_size)
{
    if (bson->size + needed_size >= bson->capacity) {
        size_t new_capacity = bson->capacity * 2;
        if (new_capacity < bson->capacity + needed_size) {
            new_capacity = bson->capacity + needed_size;
        }
        bson->capacity = new_capacity;
        void *new_buf = malloc(new_capacity);
        if (!new_buf) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            abort();
        }
        memcpy(new_buf, bson->buf, bson->size);
        free(bson->buf);
        bson->buf = new_buf;
    }
}

static void byte_array_append_byte(astarte_bson_serializer_t *bson, uint8_t byte)
{
    byte_array_grow(bson, sizeof(uint8_t));
    bson->buf[bson->size] = byte;
    bson->size++;
}

static void byte_array_append(astarte_bson_serializer_t *bson, const void *bytes, size_t count)
{
    byte_array_grow(bson, count);

    memcpy(bson->buf + bson->size, bytes, count);
    bson->size += count;
}

static void byte_array_replace(
    astarte_bson_serializer_t *bson, unsigned int pos, size_t count, const uint8_t *bytes)
{
    memcpy(bson->buf + pos, bytes, count);
}

astarte_result_t astarte_bson_serializer_init(astarte_bson_serializer_t *bson)
{
    astarte_result_t ares = byte_array_init(bson, "\0\0\0\0", 4);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Unable to initialize byte_array");
    }
    return ares;
}

void astarte_bson_serializer_destroy(astarte_bson_serializer_t *bson)
{
    byte_array_destroy(bson);
}

const void *astarte_bson_serializer_get_serialized(astarte_bson_serializer_t bson, int *size)
{
    if (size) {
        *size = (int) bson.size;
    }
    return bson.buf;
}

astarte_result_t astarte_bson_serializer_get_serialized_copy(
    astarte_bson_serializer_t bson, void *out_buf, int out_buf_size, int *out_doc_size)
{
    size_t doc_size = bson.size;
    if (out_doc_size) {
        *out_doc_size = (int) doc_size;
    }

    if (out_buf_size < bson.size) {
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }

    memcpy(out_buf, bson.buf, doc_size);

    return ASTARTE_RESULT_OK;
}

size_t astarte_bson_serializer_get_serialized_size(astarte_bson_serializer_t bson)
{
    return bson.size;
}

void astarte_bson_serializer_append_end_of_document(astarte_bson_serializer_t *bson)
{
    byte_array_append_byte(bson, '\0');

    uint8_t size_buf[4] = { 0 };
    uint32_to_bytes(bson->size, size_buf);

    byte_array_replace(bson, 0, sizeof(int32_t), size_buf);
}

void astarte_bson_serializer_append_double(
    astarte_bson_serializer_t *bson, const char *name, double value)
{
    uint8_t val_buf[sizeof(double)] = { 0 };
    double_to_bytes(value, val_buf);

    byte_array_append_byte(bson, ASTARTE_BSON_TYPE_DOUBLE);
    byte_array_append(bson, name, strlen(name) + 1);
    byte_array_append(bson, val_buf, sizeof(double));
}

void astarte_bson_serializer_append_int32(
    astarte_bson_serializer_t *bson, const char *name, int32_t value)
{
    uint8_t val_buf[4] = { 0 };
    int32_to_bytes(value, val_buf);

    byte_array_append_byte(bson, ASTARTE_BSON_TYPE_INT32);
    byte_array_append(bson, name, strlen(name) + 1);
    byte_array_append(bson, val_buf, sizeof(int32_t));
}

void astarte_bson_serializer_append_int64(
    astarte_bson_serializer_t *bson, const char *name, int64_t value)
{
    uint8_t val_buf[sizeof(int64_t)] = { 0 };
    int64_to_bytes(value, val_buf);

    byte_array_append_byte(bson, ASTARTE_BSON_TYPE_INT64);
    byte_array_append(bson, name, strlen(name) + 1);
    byte_array_append(bson, val_buf, sizeof(int64_t));
}

void astarte_bson_serializer_append_binary(
    astarte_bson_serializer_t *bson, const char *name, const void *value, size_t size)
{
    uint8_t len_buf[4] = { 0 };
    uint32_to_bytes(size, len_buf);

    byte_array_append_byte(bson, ASTARTE_BSON_TYPE_BINARY);
    byte_array_append(bson, name, strlen(name) + 1);
    byte_array_append(bson, len_buf, sizeof(int32_t));
    byte_array_append_byte(bson, ASTARTE_BSON_SUBTYPE_DEFAULT_BINARY);
    byte_array_append(bson, value, size);
}

void astarte_bson_serializer_append_string(
    astarte_bson_serializer_t *bson, const char *name, const char *string)
{
    size_t string_len = strlen(string);

    uint8_t len_buf[4] = { 0 };
    uint32_to_bytes(string_len + 1, len_buf);

    byte_array_append_byte(bson, ASTARTE_BSON_TYPE_STRING);
    byte_array_append(bson, name, strlen(name) + 1);
    byte_array_append(bson, len_buf, sizeof(int32_t));
    byte_array_append(bson, string, string_len + 1);
}

void astarte_bson_serializer_append_datetime(
    astarte_bson_serializer_t *bson, const char *name, uint64_t epoch_millis)
{
    uint8_t val_buf[sizeof(uint64_t)] = { 0 };
    uint64_to_bytes(epoch_millis, val_buf);

    byte_array_append_byte(bson, ASTARTE_BSON_TYPE_DATETIME);
    byte_array_append(bson, name, strlen(name) + 1);
    byte_array_append(bson, val_buf, sizeof(uint64_t));
}

void astarte_bson_serializer_append_boolean(
    astarte_bson_serializer_t *bson, const char *name, bool value)
{
    byte_array_append_byte(bson, ASTARTE_BSON_TYPE_BOOLEAN);
    byte_array_append(bson, name, strlen(name) + 1);
    byte_array_append_byte(bson, value ? '\1' : '\0');
}

void astarte_bson_serializer_append_document(
    astarte_bson_serializer_t *bson, const char *name, const void *document)
{
    uint32_t size = 0U;
    memcpy(&size, document, sizeof(uint32_t));
    size = le32toh(size);

    byte_array_append_byte(bson, ASTARTE_BSON_TYPE_DOCUMENT);
    byte_array_append(bson, name, strlen(name) + 1);

    byte_array_append(bson, document, size);
}

#define IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(TYPE, TYPE_NAME)                       \
    astarte_result_t astarte_bson_serializer_append_##TYPE_NAME##_array(                           \
        astarte_bson_serializer_t *bson, const char *name, TYPE arr, int count)                    \
    {                                                                                              \
        astarte_bson_serializer_t array_ser = { 0 };                                               \
        astarte_result_t ares = astarte_bson_serializer_init(&array_ser);                          \
        if (ares != ASTARTE_RESULT_OK) {                                                           \
            return ares;                                                                           \
        }                                                                                          \
        for (int i = 0; i < count; i++) {                                                          \
            char key[BSON_ARRAY_SIZE_STR_LEN] = { 0 };                                             \
            int ret = snprintf(key, BSON_ARRAY_SIZE_STR_LEN, "%i", i);                             \
            if ((ret < 0) || (ret >= BSON_ARRAY_SIZE_STR_LEN)) {                                   \
                ares = ASTARTE_RESULT_INTERNAL_ERROR;                                              \
            }                                                                                      \
            astarte_bson_serializer_append_##TYPE_NAME(&array_ser, key, arr[i]);                   \
        }                                                                                          \
        astarte_bson_serializer_append_end_of_document(&array_ser);                                \
                                                                                                   \
        int size;                                                                                  \
        const void *document = astarte_bson_serializer_get_serialized(array_ser, &size);           \
                                                                                                   \
        byte_array_append_byte(bson, ASTARTE_BSON_TYPE_ARRAY);                                     \
        byte_array_append(bson, name, strlen(name) + 1);                                           \
                                                                                                   \
        byte_array_append(bson, document, size);                                                   \
                                                                                                   \
        astarte_bson_serializer_destroy(&array_ser);                                               \
                                                                                                   \
        return ares;                                                                               \
    }

IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const double *, double)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const int32_t *, int32)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const int64_t *, int64)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const char *const *, string)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const int64_t *, datetime)
IMPLEMENT_ASTARTE_BSON_SERIALIZER_APPEND_TYPE_ARRAY(const bool *, boolean)

astarte_result_t astarte_bson_serializer_append_binary_array(astarte_bson_serializer_t *bson,
    const char *name, const void *const *arr, const size_t *sizes, int count)
{
    astarte_bson_serializer_t array_ser = { 0 };
    astarte_result_t ares = astarte_bson_serializer_init(&array_ser);
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }
    for (int i = 0; i < count; i++) {
        char key[BSON_ARRAY_SIZE_STR_LEN] = { 0 };
        int ret = snprintf(key, BSON_ARRAY_SIZE_STR_LEN, "%i", i);
        if ((ret < 0) || (ret >= BSON_ARRAY_SIZE_STR_LEN)) {
            ares = ASTARTE_RESULT_INTERNAL_ERROR;
        }
        astarte_bson_serializer_append_binary(&array_ser, key, arr[i], sizes[i]);
    }
    astarte_bson_serializer_append_end_of_document(&array_ser);

    int size = 0;
    const void *document = astarte_bson_serializer_get_serialized(array_ser, &size);

    byte_array_append_byte(bson, ASTARTE_BSON_TYPE_ARRAY);
    byte_array_append(bson, name, strlen(name) + 1);
    byte_array_append(bson, document, size);

    astarte_bson_serializer_destroy(&array_ser);

    return ares;
}

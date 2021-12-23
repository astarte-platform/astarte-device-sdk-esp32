/*
 * (C) Copyright 2016, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

/**
 * @file astarte_bson_serializer.h
 * @brief Astarte BSON serializer functions.
 */

#ifndef _ASTARTE_BSON_SERIALIZER_H_
#define _ASTARTE_BSON_SERIALIZER_H_

#include "astarte.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct astarte_byte_array_t
{
    int capacity;
    int size;
    uint8_t *buf;
};

struct astarte_bson_serializer_t
{
    struct astarte_byte_array_t ba;
};

/**
 * @brief initialize given BSON serializer.
 *
 * @details This function has to be called to initialize and allocate memory for
 * astarte_bson_serializer_t.
 * @param bs an astarte_bson_serializer_t that will be initialized.
 */
void astarte_bson_serializer_init(struct astarte_bson_serializer_t *bs);

/**
 * @brief destroy given BSON serializer.
 *
 * @details This function has to be called to destroy and free astarte_bson_serializer_t memory.
 * @param bs an astarte_bson_serializer_t that will be destroyed.
 */
void astarte_bson_serializer_destroy(struct astarte_bson_serializer_t *bs);

/**
 * @brief return bson serializer internal buffer.
 *
 * @details This function might be used to get internal buffer without any data copy. Returned
 * buffer will be invalid after serializer destruction.
 * @param bs a astarte_bson_serializer_t.
 */
const void *astarte_bson_serializer_get_document(
    const struct astarte_bson_serializer_t *bs, int *size);

/**
 * @brief copy bson serializer internal buffer to a different buffer.
 *
 * @details This function should be used to get a copy of the BSON document data. end of document
 * should be called before calling this function.
 * @param bs a astarte_bson_serializer_t.
 * @param out_buf destination buffer.
 * @param out_buf_len destination buffer length.
 * @param out_doc_size BSON document size (that is <= out_buf_len).
 * @return astarte_err_t ASTARTE_OK on success, otherwise an error is returned.
 */
astarte_err_t astarte_bson_serializer_write_document(
    const struct astarte_bson_serializer_t *bs, void *out_buf, int out_buf_len, int *out_doc_size);

/**
 * @brief return document size
 *
 * @details This function returns BSON document size in bytes.
 * @param bs a astarte_bson_serializer_t.
 */
int astarte_bson_serializer_document_size(const struct astarte_bson_serializer_t *bs);

/**
 * @brief append end of document marker.
 *
 * @details BSON document MUST be manually terminated with an end of document marker.
 * @param bs a astarte_bson_serializer_t.
 */
void astarte_bson_serializer_append_end_of_document(struct astarte_bson_serializer_t *bs);

/**
 * @brief append a double value
 *
 * @details This function appends a double value to the document. Stored value can be fetched using
 * given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param value a double floating point value.
 */
void astarte_bson_serializer_append_double(
    struct astarte_bson_serializer_t *bs, const char *name, double value);

/**
 * @brief append an int32 value
 *
 * @details This function appends an int32 value to the document. Stored value can be fetched using
 * given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param value a 32 bits signed integer value.
 */
void astarte_bson_serializer_append_int32(
    struct astarte_bson_serializer_t *bs, const char *name, int32_t value);

/**
 * @brief append an int64 value
 *
 * @details This function appends an int64 value to the document. Stored value can be fetched using
 * given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param value a 64 bits signed integer value.
 */
void astarte_bson_serializer_append_int64(
    struct astarte_bson_serializer_t *bs, const char *name, int64_t value);

/**
 * @brief append a binary blob value
 *
 * @details This function appends a binary blob to the document. Stored value can be fetched using
 * given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param value the buffer that holds the binary blob.
 * @param binary blob size in bytes.
 */
void astarte_bson_serializer_append_binary(
    struct astarte_bson_serializer_t *bs, const char *name, const void *value, int size);

/**
 * @brief append an UTF-8 string
 *
 * @details This function appends an UTF-8 string to the document. Stored value can be fetched using
 * given key.
 *
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param value a 0 terminated UTF-8 string.
 */
void astarte_bson_serializer_append_string(
    struct astarte_bson_serializer_t *bs, const char *name, const char *string);

/**
 * @brief append a date time value
 *
 * @details This function appends a date time value to the document. Stored value can be fetched
 * using given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param value a 64 bits unsigned integer storing date time in milliseconds since epoch.
 */
void astarte_bson_serializer_append_datetime(
    struct astarte_bson_serializer_t *bs, const char *name, uint64_t epoch_millis);

/**
 * @brief append a boolean value
 *
 * @details This function appends a boolean value to the document. Stored value can be fetched using
 * given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param value 0 as false value, not 0 as true value.
 */
void astarte_bson_serializer_append_boolean(
    struct astarte_bson_serializer_t *bs, const char *name, bool value);

/**
 * @brief append a sub-BSON document.
 *
 * @details This function appends a BSON subdocument to the document. Stored value can be fetched
 * using given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param document a valid BSON document (that has been already terminated).
 */
void astarte_bson_serializer_append_document(
    struct astarte_bson_serializer_t *bs, const char *name, const void *document);

/**
 * @brief append a double array
 *
 * @details This function appends a double array to the document. Stored value can be fetched using
 * given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param double_array an array of doubles.
 * @param count the number of items stored in double_array.
 */
void astarte_bson_serializer_append_double_array(
    struct astarte_bson_serializer_t *bs, const char *name, const double *double_array, int count);

/**
 * @brief append an int32 array
 *
 * @details This function appends an int32 array to the document. Stored value can be fetched using
 * given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param int32_array an array of signed 32 bit integers.
 * @param count the number of items stored in int32_array.
 */
void astarte_bson_serializer_append_int32_array(
    struct astarte_bson_serializer_t *bs, const char *name, const int32_t *int32_array, int count);

/**
 * @brief append an int64 array
 *
 * @details This function appends an int64 array to the document. Stored value can be fetched using
 * given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param int64_array an array of signed 64 bit integers.
 * @param count the number of items stored in int64_array.
 */
void astarte_bson_serializer_append_int64_array(
    struct astarte_bson_serializer_t *bs, const char *name, const int64_t *int64_array, int count);

/**
 * @brief append a string array
 *
 * @details This function appends a string array to the document. Stored value can be fetched using
 * given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param string_array an array of 0 terminated UTF-8 strings.
 * @param count the number of items stored in string_array.
 */
void astarte_bson_serializer_append_string_array(struct astarte_bson_serializer_t *bs,
    const char *name, const char *const *string_array, int count);

/**
 * @brief append a binary blob array
 *
 * @details This function appends a binary blob array to the document. Stored value can be fetched
 * using given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param binarty_array an array of binary blobs (void *).
 * @param sizes an array with the sizes of each binary in binary_array parameter.
 * @param count the number of items stored in binary_array.
 */
void astarte_bson_serializer_append_binary_array(struct astarte_bson_serializer_t *bs,
    const char *name, const void *const *binary_array, const int *sizes, int count);

/**
 * @brief append a date time array
 *
 * @details This function appends a date time array to the document. Stored value can be fetched
 * using given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param epoch_millis_array an array of 64 bits unsigned integer storing date time in milliseconds
 * since epoch.
 * @param count the number of items stored in epoch_millis_array.
 */
void astarte_bson_serializer_append_datetime_array(struct astarte_bson_serializer_t *bs,
    const char *name, const int64_t *epoch_millis_array, int count);

/**
 * @brief append a boolean array
 *
 * @details This function appends a boolean array to the document. Stored value can be fetched using
 * given key.
 * @param bs a astarte_bson_serializer_t.
 * @param name BSON key, which is a C string.
 * @param boolean_array an array of stdbool booleans.
 * @param count the number of items stored in boolean_array.
 */
void astarte_bson_serializer_append_boolean_array(
    struct astarte_bson_serializer_t *bs, const char *name, const bool *boolean_array, int count);

#ifdef __cplusplus
}
#endif

#endif

/*
 * (C) Copyright 2016, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

/**
 * @file astarte_bson_deserializer.h
 * @brief Astarte BSON deserialization functions.
 *
 * @details This library follows the v1.1 of the BSON standard, but does not provide support
 * for the full specification, only for a smaller subset. For more information regarding the BSON
 * format specifications see: https://bsonspec.org/spec.html.
 */

#ifndef _ASTARTE_BSON_DESERIALIZER_H_
#define _ASTARTE_BSON_DESERIALIZER_H_

#include "astarte.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t size; /** Total size of the document in bytes */
    const void *list; /** Pointer to the head of the list of elements in the BSON document */
    uint32_t list_size; /** Size of the list in bytes */
} astarte_bson_document_t;

typedef struct
{
    uint8_t type; /** Element type, see astarte_bson_types.h for the available types */
    const char *name; /** String containing the element name */
    size_t name_len; /** Length in bytes of the element name, not including the null terminator */
    const void *value; /** Pointer to the element content */
} astarte_bson_element_t;

/**
 * @brief Perform some checks on the validity of the BSON.
 *
 * @note This function performs a very rough validation check. It is quite possible a malformed
 * bson file would still pass this check.
 *
 * @param[in] buffer Buffer containing the document to check.
 * @param[in] buffer_size Size of the allocated buffer containing the document.
 * @return True when BSON file is valid, false otherwise.
 */
bool astarte_bson_deserializer_check_validity(const void *buffer, int buffer_size);

/**
 * @brief Initialize a document type from a BSON data buffer.
 *
 * @param[in] buffer Buffer containing the BSON data.
 * @return Inizialized document struct.
 */
astarte_bson_document_t astarte_bson_deserializer_init_doc(const void *buffer);

/**
 * @brief Get the first element in a document's list.
 *
 * @param[in] document Document from which to extract the element.
 * @param[out] element Used to store the extracted element.
 * @return ASTARTE_OK if successful, ASTARTE_ERR_NOT_FOUND if the document is empty.
 */
astarte_err_t astarte_bson_deserializer_first_element(
    astarte_bson_document_t document, astarte_bson_element_t *element);

/**
 * @brief Get the next element in a list.
 *
 * @param[in] document Document containing the list.
 * @param[in] curr_element Pointer to the current element.
 * @param[in] next_element Used to store the extracted element.
 * @return ASTARTE_OK if successful, ASTARTE_ERR_NOT_FOUND if no next element exists.
 */
astarte_err_t astarte_bson_deserializer_next_element(astarte_bson_document_t document,
    astarte_bson_element_t curr_element, astarte_bson_element_t *next_element);

/**
 * @brief Extract the value from the passed element.
 *
 * @param[in] element Element to extract the value from.
 * @return Extracted value.
 */
double astarte_bson_deserializer_element_to_double(astarte_bson_element_t element);

/**
 * @brief Extract the value from the passed element.
 *
 * @param[in] element Element to extract the value from.
 * @param[in] len Returned string length.
 * @return Extracted value.
 */
const char *astarte_bson_deserializer_element_to_string(
    astarte_bson_element_t element, uint32_t *len);

/**
 * @brief Extract the value from the passed element.
 *
 * @param[in] element Element to extract the value from.
 * @return Extracted value.
 */
astarte_bson_document_t astarte_bson_deserializer_element_to_document(
    astarte_bson_element_t element);

/**
 * @brief Extract the value from the passed element.
 *
 * @note An array is encoded as a document, so a document type is returned.
 *
 * @param[in] element Element to extract the value from.
 * @return Extracted value.
 */
astarte_bson_document_t astarte_bson_deserializer_element_to_array(astarte_bson_element_t element);

/**
 * @brief Extract the value from the passed element.
 *
 * @param[in] element Element to extract the value from.
 * @param[in] len Returned bytes array length.
 * @return Extracted value.
 */
const uint8_t *astarte_bson_deserializer_element_to_binary(
    astarte_bson_element_t element, uint32_t *len);

/**
 * @brief Extract the value from the passed element.
 *
 * @note The actual returned value is an uint8_t.
 *
 * @param[in] element Element to extract the value from.
 * @return Extracted value.
 */
bool astarte_bson_deserializer_element_to_bool(astarte_bson_element_t element);

/**
 * @brief Extract the value from the passed element.
 *
 * @note UTC datetime is encoded as a timestamp in an int64_t.
 *
 * @param[in] element Element to extract the value from.
 * @return Extracted value.
 */
int64_t astarte_bson_deserializer_element_to_datetime(astarte_bson_element_t element);

/**
 * @brief Extract the value from the passed element.
 *
 * @param[in] element Element to extract the value from.
 * @return Extracted value.
 */
int32_t astarte_bson_deserializer_element_to_int32(astarte_bson_element_t element);

/**
 * @brief Extract the value from the passed element.
 *
 * @param[in] element Element to extract the value from.
 * @return Extracted value.
 */
int64_t astarte_bson_deserializer_element_to_int64(astarte_bson_element_t element);

/**
 * @brief Fetch the element with name corresponding to the specified key from the document.
 *
 * @param[in] document Document to use for the search.
 * @param[in] key Element name to use as key for the lookup.
 * @param[out] element Used to return the found element, should be a pointer to an uninitialized
 * element.
 * @return ASTARTE_OK if successful, ASTARTE_ERR_NOT_FOUND if the element does not exist.
 */
astarte_err_t astarte_bson_deserializer_element_lookup(
    astarte_bson_document_t document, const char *key, astarte_bson_element_t *element);

#ifdef __cplusplus
}
#endif

#endif // _ASTARTE_BSON_DESERIALIZER_H_

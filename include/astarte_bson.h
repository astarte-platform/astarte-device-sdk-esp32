/*
 * (C) Copyright 2016, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

/**
 * @file astarte_bson.h
 * @brief Astarte BSON deserialization functions.
 *
 * @details This library follows the v1.1 of the BSON standard, but does not provide support
 * for the full specification, only for a smaller subset. For more information regarding the BSON
 * format specifications see: https://bsonspec.org/spec.html.
 */

#ifndef _ASTARTE_BSON_H_
#define _ASTARTE_BSON_H_

#include "astarte.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Look up the key from the document and return a pointer to the appropriate entry.
 *
 * @details This function loops over all the elements in the document's list and return a pointer
 * to the first element with a name matching the specified key.
 * N.B. The return value points to the content of the element, past the element type and name.
 *
 * @note Deprecated, call astarte_bson_deserializer_element_lookup() instead.
 *
 * @param[in] key String representing the key to find in the document.
 * @param[in] document Document to use for the search.
 * @param[out] type Returned element type, see 'astarte_bson_types.h' for its possible values.
 * @return Pointer to the content of the element matched by the key. NULL when the key has not been
 * matched or the BSON file is malformed.
 */
const void *astarte_bson_key_lookup(const char *key, const void *document, uint8_t *type)
    __attribute__((deprecated("Please use the 'astarte_bson_deserializer_element_lookup' function "
                              "in astarte_bson_deserializer.h")));
/**
 * @brief Get a pointer to the first item in a document's list.
 *
 * @details N.B. The return value points to the beginning of the element, to the element type.
 *
 * @note Deprecated, call astarte_bson_deserializer_first_element() instead.
 *
 * @param[in] document Document containing the list.
 * @return Pointer to the first element. NULL when no next element exists.
 */
const void *astarte_bson_first_item(const void *document)
    __attribute__((deprecated("Please use the 'astarte_bson_deserializer_first_element' function "
                              "in astarte_bson_deserializer.h")));
/**
 * @brief Get a pointer to the next item in a document's list.
 *
 * @details N.B. The return value points to the beginning of the element, to the element type.
 *
 * @note Deprecated, call astarte_bson_deserializer_next_element() instead.
 *
 * @param[in] document Document containing the list.
 * @param[in] current_item Pointer to the current element.
 * @return Pointer to the next element. NULL when no next element exists.
 */
void *astarte_bson_next_item(const void *document, const void *current_item)
    __attribute__((deprecated("Please use the 'astarte_bson_deserializer_next_element' function "
                              "in astarte_bson_deserializer.h")));
/**
 * @brief Get a pointer to the string containing an element name.
 *
 * @note Deprecated, use the functions contained in astarte_bson_deserializer.h instead.
 *
 * @param[in] item Pointer to the element from which the key should be extracted.
 * @return Pointer to the beginning of the name of the element.
 */
const char *astarte_bson_key(const void *item) __attribute__((
    deprecated("Please use the deserialization functions in astarte_bson_deserializer.h")));
/**
 * @brief Parse a BSON string and return a pointer to the beginning of the UTF-8 string.
 *
 * @note Deprecated, call astarte_bson_deserializer_element_to_string() instead.
 *
 * @param[in] value_ptr Pointer to the value to parse.
 * @param[out] len Size of the parsed string.
 * @return Pointer to the begginning of the UTF-8 string.
 */
const char *astarte_bson_value_to_string(const void *value_ptr, uint32_t *len) __attribute__((
    deprecated("Please use the 'astarte_bson_deserializer_element_to_string' function "
               "in astarte_bson_deserializer.h")));
/**
 * @brief Parse a BSON binary and return a pointer to the beginning of the byte array.
 *
 * @note Deprecated, call astarte_bson_deserializer_element_to_binary() instead.
 *
 * @param[in] value_ptr Pointer to the value to parse.
 * @param[out] len Size of the parsed byte array.
 * @return Pointer to the beginning of the byte array.
 */
const char *astarte_bson_value_to_binary(const void *value_ptr, uint32_t *len) __attribute__((
    deprecated("Please use the 'astarte_bson_deserializer_element_to_binary' function "
               "in astarte_bson_deserializer.h")));
/**
 * @brief Parse a BSON document and return a pointer to the beginning of the document.
 *
 * @details This function does not perform actual parsing, using the original pointer as a document
 * would work in the exact same way.
 *
 * @note Deprecated, call astarte_bson_deserializer_element_to_document() instead.
 *
 * @param[in] value_ptr Pointer to the value to parse.
 * @param[out] len Size of the document.
 * @return Pointer to the beginning of the document.
 */
const void *astarte_bson_value_to_document(const void *value_ptr, uint32_t *len) __attribute__((
    deprecated("Please use the 'astarte_bson_deserializer_element_to_document' function "
               "in astarte_bson_deserializer.h")));
/**
 * @brief Cast the input element to a int8.
 *
 * @details This function does not perform actual parsing, performing a manual casting would
 * work in the same way.
 *
 * @note Deprecated, call astarte_bson_deserializer_element_to_bool() instead.
 *
 * @param[in] value_ptr Pointer to the value to parse.
 * @return Casted value.
 */
int8_t astarte_bson_value_to_int8(const void *value_ptr)
    __attribute__((deprecated("Please use the 'astarte_bson_deserializer_element_to_bool' function "
                              "in astarte_bson_deserializer.h")));
/**
 * @brief Cast the input element to a int32.
 *
 * @note Deprecated, call astarte_bson_deserializer_element_to_int32() instead.
 *
 * @param[in] value_ptr Pointer to the value to parse.
 * @return Casted value.
 */
int32_t astarte_bson_value_to_int32(const void *value_ptr) __attribute__((
    deprecated("Please use the 'astarte_bson_deserializer_element_to_int32' function "
               "in astarte_bson_deserializer.h")));
/**
 * @brief Cast the input element to a int64.
 *
 * @note Deprecated, call astarte_bson_deserializer_element_to_int64() instead.
 *
 * @param[in] value_ptr Pointer to the value to parse.
 * @return Casted value.
 */
int64_t astarte_bson_value_to_int64(const void *value_ptr) __attribute__((
    deprecated("Please use the 'astarte_bson_deserializer_element_to_int64' function "
               "in astarte_bson_deserializer.h")));
/**
 * @brief Cast the input element to a double.
 *
 * @note Deprecated, call astarte_bson_deserializer_element_to_double() instead.
 *
 * @param[in] value_ptr Pointer to the value to parse.
 * @return Casted value.
 */
double astarte_bson_value_to_double(const void *value_ptr) __attribute__((
    deprecated("Please use the 'astarte_bson_deserializer_element_to_double' function "
               "in astarte_bson_deserializer.h")));
/**
 * @brief Perform some checks on the validity of the BSON.
 *
 * @note This function performs a very rough validation check. It is quite possible a malformed
 * bson file would still pass this check.
 *
 * @note Deprecated, call astarte_bson_deserializer_check_validity() instead.
 *
 * @param[in] document Document for which to calculate the size.
 * @param[in] file_size Size of the allocated buffer containing document.
 * @return True when BSON file is valid, false otherwise.
 */
bool astarte_bson_check_validity(const void *document, unsigned int file_size)
    __attribute__((deprecated("Please use the 'astarte_bson_deserializer_check_validity' function "
                              "in astarte_bson_deserializer.h")));
/**
 * @brief Get the size of the document.
 *
 * @note Deprecated, use the functions contained in astarte_bson_deserializer.h instead.
 *
 * @param[in] document Document for which to calculate the size.
 * @return Size of the document.
 */
uint32_t astarte_bson_document_size(const void *document) __attribute__((
    deprecated("Please use the deserialization functions in astarte_bson_deserializer.h")));

#ifdef __cplusplus
}
#endif

#endif

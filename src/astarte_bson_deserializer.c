/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

#include <astarte_bson_deserializer.h>

#include <astarte_bson_types.h>

#include <endian.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <esp_log.h>

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define TAG "ASTARTE_BSON_DESERIALIZER"

#define NULL_TERM_SIZE 1

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Cast the first four bytes of a little-endian buffer to a uint32_t in the host byte order.
 *
 * @details This function expects the input buffer to be in little-endian order.
 * @param[in] buff Buffer containing the data to read.
 * @return Resulting uint32_t value.
 */
static uint32_t read_uint32(const void *buff);

/**
 * @brief Cast the first eight bytes of a little-endian buffer to a uint64_t in the host byte order.
 *
 * @details This function expects the input buffer to be in little-endian order.
 * @param[in] buff Buffer containing the data to read.
 * @return Resulting uint64_t value.
 */
static uint64_t read_uint64(const void *buff);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

bool astarte_bson_deserializer_check_validity(const void *buffer, int buffer_size)
{
    astarte_bson_document_t document;

    // Validate buffer size is at least 5, the size of an empty document.
    if (buffer_size < sizeof(document.size) + NULL_TERM_SIZE) {
        ESP_LOGW(TAG, "Buffer too small: no BSON document found");
        return false;
    }

    document = astarte_bson_deserializer_init_doc(buffer);

    // Ensure the buffer is larger or equal compared to the decoded document size
    if (buffer_size < document.size) {
        ESP_LOGW(TAG, "Allocated buffer size (%i) is smaller than BSON document size (%" PRIu32 ")",
            buffer_size, document.size);
        return false;
    }

    // Check document is terminated with 0x00
    if (*(const char *) (document.list + (document.size - sizeof(document.size)) - 1) != 0) {
        ESP_LOGW(TAG, "BSON document is not terminated by null byte.");
        return false;
    }

    // Validation for an empty document is over
    if (document.size == sizeof(document.size) + NULL_TERM_SIZE) {
        return true;
    }

    // Check on the minimum buffer size for a non empty document, composed of at least:
    // - 4 bytes for the document size
    // - 1 byte for the element index
    // - 1 byte for the element name (could be an empty string)
    // - 1 byte for the element content (for example a boolean)
    // - 1 byte for the trailing 0x00
    // NB this check could fail on the NULL value element described in the BSON specifications
    if (document.size < sizeof(document.size) + 3 + NULL_TERM_SIZE) {
        ESP_LOGW(TAG, "BSON data too small");
        return false;
    }

    // Check that the first element of the document has a supported index
    switch (*(const char *) document.list) {
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

astarte_bson_document_t astarte_bson_deserializer_init_doc(const void *buffer)
{
    astarte_bson_document_t document;
    document.size = read_uint32(buffer);
    document.list = buffer + sizeof(document.size);
    document.list_size = document.size - sizeof(document.size) - NULL_TERM_SIZE;
    return document;
}

astarte_err_t astarte_bson_deserializer_first_element(
    astarte_bson_document_t document, astarte_bson_element_t *element)
{
    // Document should not be empty
    if (document.size <= sizeof(document.size) + NULL_TERM_SIZE) {
        return ASTARTE_ERR_NOT_FOUND;
    }

    element->type = *(uint8_t *) document.list;
    element->name = (const char *) (document.list + sizeof(element->type));
    size_t max_name_str_len = document.list_size - sizeof(element->type);
    element->name_len = strnlen(element->name, max_name_str_len);
    element->value = document.list + sizeof(element->type) + element->name_len + NULL_TERM_SIZE;
    return ASTARTE_OK;
}

astarte_err_t astarte_bson_deserializer_next_element(astarte_bson_document_t document,
    astarte_bson_element_t curr_element, astarte_bson_element_t *next_element)
{
    // Get the size of the current element
    size_t element_value_size = 0U;
    switch (curr_element.type) {
        case BSON_TYPE_STRING: {
            element_value_size = sizeof(int32_t) + read_uint32(curr_element.value);
            break;
        }
        case BSON_TYPE_ARRAY:
        case BSON_TYPE_DOCUMENT: {
            element_value_size = read_uint32(curr_element.value);
            break;
        }
        case BSON_TYPE_BINARY: {
            element_value_size = sizeof(int32_t) + sizeof(int8_t) + read_uint32(curr_element.value);
            break;
        }
        case BSON_TYPE_INT32: {
            element_value_size = sizeof(int32_t);
            break;
        }
        case BSON_TYPE_DOUBLE:
        case BSON_TYPE_DATETIME:
        case BSON_TYPE_INT64: {
            element_value_size = sizeof(int64_t);
            break;
        }
        case BSON_TYPE_BOOLEAN: {
            element_value_size = sizeof(int8_t);
            break;
        }
        default: {
            ESP_LOGW(TAG, "unrecognized BSON type: %i", (int) curr_element.type);
            return ASTARTE_ERR;
        }
    }

    const void *next_element_start = curr_element.value + element_value_size;

    // Check if we are not looking past the end of the document
    const void *end_of_document = document.list + document.list_size;
    if (next_element_start >= end_of_document) {
        return ASTARTE_ERR_NOT_FOUND;
    }

    next_element->type = *(uint8_t *) next_element_start;
    next_element->name = (const char *) (next_element_start + sizeof(next_element->type));
    size_t max_name_str_len = document.list_size - (size_t) (document.list - next_element_start)
        - sizeof(next_element->type);
    next_element->name_len = strnlen(next_element->name, max_name_str_len);
    next_element->value
        = next_element_start + sizeof(next_element->type) + next_element->name_len + NULL_TERM_SIZE;
    return ASTARTE_OK;
}

astarte_err_t astarte_bson_deserializer_element_lookup(
    astarte_bson_document_t document, const char *key, astarte_bson_element_t *element)
{
    astarte_bson_element_t candidate_element;
    astarte_err_t err = astarte_bson_deserializer_first_element(document, &candidate_element);
    if (err != ASTARTE_OK) {
        return err;
    }

    while (strncmp(key, candidate_element.name, candidate_element.name_len + NULL_TERM_SIZE) != 0) {
        err = astarte_bson_deserializer_next_element(
            document, candidate_element, &candidate_element);
        if (err != ASTARTE_OK) {
            return err;
        }
    }

    // When match has been found copy the candidate element in the out struct
    element->type = candidate_element.type;
    element->name = candidate_element.name;
    element->name_len = candidate_element.name_len;
    element->value = candidate_element.value;

    return err;
}

double astarte_bson_deserializer_element_to_double(astarte_bson_element_t element)
{
    uint64_t value = read_uint64(element.value);
    return ((double *) &value)[0];
}

const char *astarte_bson_deserializer_element_to_string(
    astarte_bson_element_t element, uint32_t *len)
{
    if (len) {
        *len = read_uint32(element.value) - NULL_TERM_SIZE;
    }
    return (const char *) (element.value + sizeof(uint32_t));
}

astarte_bson_document_t astarte_bson_deserializer_element_to_document(
    astarte_bson_element_t element)
{
    return astarte_bson_deserializer_init_doc(element.value);
}

astarte_bson_document_t astarte_bson_deserializer_element_to_array(astarte_bson_element_t element)
{
    return astarte_bson_deserializer_init_doc(element.value);
}

const uint8_t *astarte_bson_deserializer_element_to_binary(
    astarte_bson_element_t element, uint32_t *len)
{
    if (len) {
        *len = read_uint32(element.value);
    }
    return (const uint8_t *) (element.value + sizeof(uint32_t) + sizeof(uint8_t));
}

bool astarte_bson_deserializer_element_to_bool(astarte_bson_element_t element)
{
    return *((bool *) element.value);
}

int64_t astarte_bson_deserializer_element_to_datetime(astarte_bson_element_t element)
{
    uint64_t value = read_uint64(element.value);
    return ((int64_t *) &value)[0];
}

int32_t astarte_bson_deserializer_element_to_int32(astarte_bson_element_t element)
{
    uint32_t value = read_uint32(element.value);
    return ((int32_t *) &value)[0];
}

int64_t astarte_bson_deserializer_element_to_int64(astarte_bson_element_t element)
{
    uint64_t value = read_uint64(element.value);
    return ((int64_t *) &value)[0];
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

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

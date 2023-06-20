/*
 * (C) Copyright 2016, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

/**
 * @file astarte_bson.h
 * @brief Astarte BSON manipulation functions.
 */

#ifndef _ASTARTE_BSON_H_
#define _ASTARTE_BSON_H_

#include "astarte.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const void *astarte_bson_key_lookup(const char *key, const void *document, uint8_t *type);
void *astarte_bson_next_item(const void *document, const void *current_item);
const void *astarte_bson_first_item(const void *document);
const char *astarte_bson_key(const void *item);
const char *astarte_bson_value_to_string(const void *valuePtr, uint32_t *len);
const char *astarte_bson_value_to_binary(const void *valuePtr, uint32_t *len);
const void *astarte_bson_value_to_document(const void *valuePtr, uint32_t *len);
int8_t astarte_bson_value_to_int8(const void *valuePtr);
int32_t astarte_bson_value_to_int32(const void *valuePtr);
int64_t astarte_bson_value_to_int64(const void *valuePtr);
double astarte_bson_value_to_double(const void *valuePtr);
bool astarte_bson_check_validity(const void *document, unsigned int fileSize);
uint32_t astarte_bson_document_size(const void *document);

#ifdef __cplusplus
}
#endif

#endif

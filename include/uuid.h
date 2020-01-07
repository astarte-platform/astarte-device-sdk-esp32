/*
 * (C) Copyright 2019, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#ifndef _UUID_H_
#define _UUID_H_

#include <stddef.h>
#include <stdint.h>

typedef uint8_t uuid_t[16];

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief generate a UUIDv5.
 *
 * @details This function computes a deterministic UUID starting from a namespace UUID and binary data.
 * @param ns The UUID to be used as namespace.
 * @param data A pointer to the data that will be hashed to produce the UUID.
 * @param len The lenght of the data.
 * @param len The UUID where the result will be written.
 */
void uuid_generate_v5(const uuid_t ns, const void *data, size_t len, uuid_t out);

/**
 * @brief convert a UUID to its string representation.
 *
 * @details Convert a UUID to its canonical (RFC4122) string representation.
 * @param uuid The UUID.
 * @param out A pointer to a previously allocated buffer where the result will be written.
 */
void uuid_to_string(const uuid_t uuid, char *out);

/**
 * @brief parse a UUID from its string representation.
 *
 * @details parse a UUID from its canonical (RFC4122) string representation.
 * @param in A pointer to the string to be parsed.
 * @param uuid The UUID where the result will be written.
 * @return 0 if the parsing was succesfull, -1 otherwise.
 */
int uuid_from_string(const char *in, uuid_t uuid);

#ifdef __cplusplus
}
#endif

#endif // _UUID_H_

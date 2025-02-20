/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

#ifndef _ASTARTE_UUID_H_
#define _ASTARTE_UUID_H_

#include <stddef.h>
#include <stdint.h>

#include "astarte.h"

#define ASTARTE_UUID_LEN 16

typedef uint8_t astarte_uuid_t[ASTARTE_UUID_LEN];

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief generate a UUIDv5.
 *
 * @details This function computes a deterministic UUID starting from a namespace UUID and binary
 * data.
 * @param namespace The string representation of an UUID to be used as namespace.
 * @param data A pointer to the data that will be hashed to produce the UUID.
 * @param len The lenght of the data.
 * @param out The UUID where the result will be written.
 * @return ASTARTE_OK when successfull, ASTARTE_ERR otherwise.
 */
astarte_err_t astarte_uuid_generate_v5(const astarte_uuid_t namespace, const void *data, size_t len, astarte_uuid_t out);

/**
 * @brief convert a UUID to its string representation.
 *
 * @details Convert a UUID to its canonical (RFC4122) string representation.
 * @param uuid The UUID.
 * @param out A pointer to a previously allocated buffer where the result will be written.
 * Should be at least 37 bytes large.
 * @return ASTARTE_ERR upon failure, ASTARTE_OK otherwise.
 */
astarte_err_t astarte_uuid_to_string(const astarte_uuid_t uuid, char *out);

/**
 * @brief parse a UUID from its string representation.
 *
 * @details parse a UUID from its canonical (RFC4122) string representation.
 * @param input A pointer to the string to be parsed.
 * @param out The UUID where the result will be written.
 * @return 0 if the parsing was succesfull, -1 otherwise.
 */
int astarte_uuid_from_string(const char *input, astarte_uuid_t out);

/**
 * @brief generate a UUIDv4.
 *
 * @details This function computes a random UUID using hardware RNG.
 * @param out The UUID where the result will be written.
 */
void astarte_uuid_generate_v4(astarte_uuid_t out);

#ifdef __cplusplus
}
#endif

#endif // _ASTARTE_UUID_H_

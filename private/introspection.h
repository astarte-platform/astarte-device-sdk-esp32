/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INTROSPECTION_H
#define INTROSPECTION_H

/**
 * @file introspection.h
 * @brief Astarte introspection representation
 * https://docs.astarte-platform.org/astarte/latest/080-mqtt-v1-protocol.html#introspection
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/interface.h"
#include "astarte_device_sdk/result.h"
#include "dlist.h"

/** @brief Introspection struct. */
typedef struct
{
    dlist_t list;
} introspection_t;

/** @brief Introspection iterator struct. */
typedef struct
{
    dlist_iterator_t iter;
} introspection_iterator_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new introspection instance
 *
 * @details The resulting instance should be deallocated by the user using #introspection_free
 *
 * @return The new introspection instance.
 */
introspection_t introspection_new(void);

/**
 * @brief Deallocate an introspection instance
 *
 * @details The struct must first get initialized using #introspection_new
 *
 * @param[in] introspection Introspection instance initialized using #introspection_new
 */
void introspection_free(introspection_t introspection);

/**
 * @brief Add an interface to the introspection list
 *
 * @details No update will be performed by this function. If an interface with the same name is
 * already present an error will be returned.
 *
 * @param[in,out] introspection Pointer to an introspection initialized using #introspection_new
 * @param[in] interface the pointer to an interface struct
 * @return ASTARTE_RESULT_OK on success, otherwise an error code.
 */
astarte_result_t introspection_add(
    introspection_t *introspection, const astarte_interface_t *interface);

/**
 * @brief Update or adds an interface in the introspection list
 *
 * @details If no interface with the same name as the one passed exists,
 * the function adds the interface to the introspection list. If an interface matching the name is
 * present, the function checks the new interface to ensure it is a valid interface update.
 *
 * @param[in,out] introspection Pointer to an introspection initialized using #introspection_new
 * @param[in] interface the pointer to an interface struct
 * @return ASTARTE_RESULT_OK on success, otherwise an error code.
 */
astarte_result_t introspection_update(
    introspection_t *introspection, const astarte_interface_t *interface);

/**
 * @brief Retrieve an interface from the introspection list using the name as a key
 *
 * @details A null pointer is returned if no interface is found for the @p interface_name
 *
 * @param[in] introspection Pointer to an introspection initialized using #introspection_new
 * @param[in] interface_name The name of the interface to get
 * @return ASTARTE_RESULT_OK on success, otherwise an error code.
 */
const astarte_interface_t *introspection_get(
    introspection_t *introspection, const char *interface_name);

/**
 * @brief Compute the introspection string length
 *
 * @details The returned length includes the byte for the terminating null character '\0'.
 * A buffer of the returned size in bytes can be allocated and passed to #introspection_fill_string
 *
 * @param[in] introspection Pointer to an introspection initialized using #introspection_new
 * @return size of the introspection string in bytes, including the NULL terminator.
 */
size_t introspection_get_string_size(introspection_t *introspection);

/**
 * @brief Return the introspection string as described in Astarte documentation
 *
 * @details An empty string is returned if no interfaces got added with #introspection_add
 * The ordering of the interface names is not guaranteed and it should't be relied on
 * https://docs.astarte-platform.org/astarte/latest/080-mqtt-v1-protocol.html#introspection
 *
 * @param[in] introspection Pointer to an introspection initialized using #introspection_new
 * @param[out] buffer Buffer where to store the string, should have at least the size returned by
 * #introspection_get_string_size
 * @param[in] buffer_size Size of the @p buffer
 */
void introspection_fill_string(introspection_t *introspection, char *buffer, size_t buffer_size);

/**
 * @brief Initialize an iterator over the introspection
 *
 * @details Once initialized the iterator will alreay point to the first element.
 *
 * @param[in] introspection Pointer to an introspection initialized using #introspection_new
 * @param[out] iter Iterator to initialize
 * @return One of the follwing error codes:
 * @retval ASTARTE_RESULT_NOT_FOUND when the iterator has reached the end of the introspection
 * @retval ASTARTE_RESULT_OK if operation has been successful
 */
astarte_result_t introspection_iterator_init(
    introspection_t *introspection, introspection_iterator_t *iter);

/**
 * @brief Advance the iterator of one element
 *
 * @param[in,out] iter Iterator initialized by #introspection_iterator_init
 * @return One of the follwing error codes:
 * @retval ASTARTE_RESULT_NOT_FOUND when the iterator has reached the end of the introspection
 * @retval ASTARTE_RESULT_OK if operation has been successful
 */
astarte_result_t introspection_iterator_advance(introspection_iterator_t *iter);

/**
 * @brief Get the interface pointed by the iterator
 *
 * @param[in,out] iter Iterator initialized by #introspection_iterator_init
 * @return Pointer to the returned interface
 */
const astarte_interface_t *introspection_iterator_get_interface(introspection_iterator_t *iter);

#ifdef __cplusplus
}
#endif

#endif // INTROSPECTION_H

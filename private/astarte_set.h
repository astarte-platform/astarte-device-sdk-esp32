/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

/**
 * @file astarte_set.h
 * @brief Utility module containing a basic set implementation.
 *
 * @details This set is implemented as an ordered list without duplicates.
 */

#ifndef _ASTARTE_SET_H_
#define _ASTARTE_SET_H_

#include <stdbool.h>
#include <stdlib.h>

#include "astarte.h"

typedef struct astarte_set_handle *astarte_set_handle_t;

typedef struct {
    void *value;
    size_t value_len;
} astarte_set_element_t;

/**
 * @brief Initializes a new set
 *
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_OUT_OF_MEMORY if memory allocation failed,
 * - ASTARTE_OK if intialization has been successful
 */
astarte_err_t astarte_set_init(astarte_set_handle_t *handle);

/**
 * @brief Checks if the set is empty
 *
 * @param handle Handle for the set on which to operate
 * @return true if the list is empty, false otherwise
 */
bool astarte_set_is_empty(astarte_set_handle_t handle);

/**
 * @brief Adds a set item to a set
 *
 * @param handle Handle for the set on which to operate
 * @param element Element to add to the set
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_OUT_OF_MEMORY if memory allocation failed,
 * - ASTARTE_OK if intialization has been successful
 */
astarte_err_t astarte_set_add(astarte_set_handle_t handle, astarte_set_element_t element);

/**
 * @brief Pops an item from the set
 *
 * @param handle Handle for the set on which to operate
 * @param element Element retrieved from the set. This could be any element from the set
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_NOT_FOUND if list was already empty,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_set_pop(astarte_set_handle_t handle, astarte_set_element_t *element);

/**
 * @brief Destroy the set
 *
 * @note After calling this function on a handle, the handle will become unvalid.
 * @param handle Handle to the set to destroy
 */
void astarte_set_destroy(astarte_set_handle_t handle);

#endif /* _ASTARTE_SET_H_ */

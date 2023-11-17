/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

/**
 * @file astarte_linked_list.h
 * @brief Utility module containing a basic linked list implementation.
 */

#ifndef _ASTARTE_LINKED_LIST_H_
#define _ASTARTE_LINKED_LIST_H_

#include <stdbool.h>
#include <stdlib.h>

#include "astarte.h"

typedef struct astarte_linked_list_handle *astarte_linked_list_handle_t;
typedef struct astarte_linked_list_iterator *astarte_linked_list_iterator_t;

typedef struct {
    void *value;
    size_t value_len;
} astarte_linked_list_item_t;

/**
 * @brief Initializes a new empty linked list
 *
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_OUT_OF_MEMORY if memory allocation failed,
 * - ASTARTE_OK if intialization has been successful
 */
astarte_err_t astarte_linked_list_init(astarte_linked_list_handle_t *handle);

/**
 * @brief Checks if the linked list is empty
 *
 * @param handle Handle for the linked list on which to operate
 * @return true if the list is empty, false otherwise
 */
bool astarte_linked_list_is_empty(astarte_linked_list_handle_t handle);

/**
 * @brief Append an item to the end of a linked list
 *
 * @param handle Handle for the linked list on which to operate
 * @param item Element to append to the linked list
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_OUT_OF_MEMORY if memory allocation failed,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_linked_list_append(astarte_linked_list_handle_t handle,
    astarte_linked_list_item_t item);

/**
 * @brief Remove and return the last item from a linked list
 *
 * @param handle Handle for the linked list on which to operate
 * @param item Tail item retrieved from the linked list.
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_NOT_FOUND if list was already empty,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_linked_list_remove_tail(astarte_linked_list_handle_t handle,
    astarte_linked_list_item_t *item);

/**
 * @brief Find an item in a linked list
 *
 * @param handle Handle for the linked list on which to operate
 * @param item Item that will be searched
 * @param result True if the item is found, false otherwise.
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_OUT_OF_MEMORY if memory allocation failed,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_linked_list_find(astarte_linked_list_handle_t handle,
    astarte_linked_list_item_t item, bool *result);

/**
 * @brief Destroy the linked list
 *
 * @note After calling this function on a list, its handle will become unvalid.
 *
 * @param handle Handle to the linked list to destroy
 */
void astarte_linked_list_destroy(astarte_linked_list_handle_t handle);

/**
 * @brief Initialize an iterator over a linked list
 *
 * @note After intialization the iterator will be pointing to the first item of the list.
 *
 * @note A successfully initialized iterator should always be destroyed with
 * astarte_linked_list_iterator_destroy. If this function returns an error code,
 * then calling the destroy function is not required.
 *
 * @param handle Handle to the linked list on which to iterate
 * @param iterator Iterator to initialize.
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_NOT_FOUND if list is empty,
 * - ASTARTE_ERR_OUT_OF_MEMORY if memory allocation failed,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_linked_list_iterator_init(astarte_linked_list_handle_t handle,
    astarte_linked_list_iterator_t *iterator);

/**
 * @brief Advance the iterator to the next item of the list
 *
 * @param iterator Iterator to advance.
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_NOT_FOUND if the end of the list has been reached,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_linked_list_iterator_advance(astarte_linked_list_iterator_t iterator);

/**
 * @brief Get the item pointed by the iterator
 *
 * @param iterator Iterator to use for the operation.
 * @param item Element retrieved from the linked list.
 */
void astarte_linked_list_iterator_get_item(
    astarte_linked_list_iterator_t iterator, astarte_linked_list_item_t *item);

/**
 * @brief Destroy a correctly initialized iterator
 *
 * @param iterator Iterator to be destroyed
 */
void astarte_linked_list_iterator_destroy(astarte_linked_list_iterator_t iterator);

#endif /* _ASTARTE_LINKED_LIST_H_ */

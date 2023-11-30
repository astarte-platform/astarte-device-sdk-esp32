/*
 * (C) Copyright 2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

/**
 * @file astarte_linked_list.h
 * @brief Utility module containing a doubly linked list implementation.
 *
 * @details This library does not perform deep copies when storing values.
 * The user should ensure that any value is freed correctly when appropriate.
 */

#ifndef _ASTARTE_LINKED_LIST_H_
#define _ASTARTE_LINKED_LIST_H_

#include <stdbool.h>
#include <stdlib.h>

#include "astarte.h"

typedef struct
{
    struct astarte_linked_list_node *head;
    struct astarte_linked_list_node *tail;
} astarte_linked_list_handle_t;

typedef struct
{
    astarte_linked_list_handle_t *handle;
    struct astarte_linked_list_node *node;
} astarte_linked_list_iterator_t;

/**
 * @brief Initializes a new empty linked list
 *
 * @return A handle to the newly initialized list
 */
astarte_linked_list_handle_t astarte_linked_list_init(void);

/**
 * @brief Checks if the linked list is empty
 *
 * @param[in] handle Linked list handle
 * @return true if the list is empty, false otherwise
 */
bool astarte_linked_list_is_empty(astarte_linked_list_handle_t *handle);

/**
 * @brief Appends an item to the end of a linked list
 *
 * @param[inout] handle Linked list handle
 * @param[in] value Item to append to the linked list
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_OUT_OF_MEMORY if memory allocation failed,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_linked_list_append(astarte_linked_list_handle_t *handle, void *value);

/**
 * @brief Removes and returns the last item from a linked list
 *
 * @param[inout] handle Linked list handle
 * @param[out] value Item removed from the list
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_NOT_FOUND if list is empty,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_linked_list_remove_tail(astarte_linked_list_handle_t *handle, void **value);

/**
 * @brief Destroys the list without de-allocating its content
 *
 * @note Must be called on an non-empty list when its use has ended. While it releases all the
 * internal structures of the list it does not free the content of each item.
 *
 * @param[inout] handle Linked list handle
 */
void astarte_linked_list_destroy(astarte_linked_list_handle_t *handle);

/**
 * @brief Destroys the list releasing with 'free()' its content
 *
 * @note Can be called on an non-empty list when its use has ended. It releases all the internal
 * structures of the list as well as each item placed into the list.
 *
 * @param[inout] handle Linked list handle
 */
void astarte_linked_list_destroy_and_release(astarte_linked_list_handle_t *handle);

/**
 * @brief Initializes an iterator over a linked list
 *
 * @note After intialization the iterator will be pointing to the first item of the list.
 *
 * @param[in] handle Linked list handle
 * @param[out] iterator Iterator to initialize
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_NOT_FOUND if list is empty,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_linked_list_iterator_init(
    astarte_linked_list_handle_t *handle, astarte_linked_list_iterator_t *iterator);

/**
 * @brief Advances the iterator to the next item of the list
 *
 * @param[inout] iterator Iterator to advance
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_NOT_FOUND if the end of the list has been reached,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_linked_list_iterator_advance(astarte_linked_list_iterator_t *iterator);

/**
 * @brief Gets the item pointed by the iterator
 *
 * @param[in] iterator Iterator to use for the operation
 * @param[out] value Item fetched from the list
 */
void astarte_linked_list_iterator_get_item(astarte_linked_list_iterator_t *iterator, void **value);

/**
 * @brief Replaces the item pointed by the iterator
 *
 * @note Does not de-allocate the old item content.
 *
 * @param[inout] iterator Iterator to use for the operation
 * @param[in] value Element to sobstitute from the linked list
 */
void astarte_linked_list_iterator_replace_item(
    astarte_linked_list_iterator_t *iterator, void *value);

#endif /* _ASTARTE_LINKED_LIST_H_ */

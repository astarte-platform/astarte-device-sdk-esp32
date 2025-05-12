/*
 * (C) Copyright 2023-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file dlist.h
 * @brief Utility module containing a doubly linked list implementation.
 *
 * @details This library does not perform deep copies when storing values.
 * The user should ensure that any value is freed correctly when appropriate.
 */

#ifndef DLIST_H
#define DLIST_H

#include <stdbool.h>
#include <stdlib.h>

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/result.h"

typedef struct
{
    struct dlist_node *head;
    struct dlist_node *tail;
} dlist_t;

typedef struct
{
    dlist_t *handle;
    struct dlist_node *node;
} dlist_iterator_t;

/**
 * @brief Initialize a new empty linked list
 *
 * @return A handle to the newly initialized list
 */
dlist_t dlist_init(void);

/**
 * @brief Check if the linked list is empty
 *
 * @param[in] handle Linked list handle
 * @return true if the list is empty, false otherwise
 */
bool dlist_is_empty(dlist_t *handle);

/**
 * @brief Append an item to the end of a linked list
 *
 * @param[inout] handle Linked list handle
 * @param[in] value Item to append to the linked list
 * @return One of the follwing error codes:
 * @retval ASTARTE_ERR_OUT_OF_MEMORY if memory allocation failed
 * @retval ASTARTE_RESULT_OK if operation has been successful
 */
astarte_result_t dlist_append(dlist_t *handle, void *value);

/**
 * @brief Append an integer item to the end of a linked list
 *
 * @note This function will allocate dynamically the space for the integer in addition to the usual
 * space required for a new item. This additional space will not be deallocated by #dlist_destroy.
 * The function #dlist_destroy_and_release should be used instead.
 *
 * @param[inout] handle Linked list handle
 * @param[in] value Item to append to the linked list
 * @return One of the follwing error codes:
 * @retval ASTARTE_ERR_OUT_OF_MEMORY if memory allocation failed
 * @retval ASTARTE_RESULT_OK if operation has been successful
 */
astarte_result_t dlist_append_int(dlist_t *handle, int value);

/**
 * @brief Remove and return the last item from a linked list
 *
 * @param[inout] handle Linked list handle
 * @param[out] value Item removed from the list
 * @return NULL when the list was empty an no item has been removed, the removed item otherwise
 */
void *dlist_remove_tail(dlist_t *handle);

/**
 * @brief Destroy the list without de-allocating its content
 *
 * @note Must be called on an non-empty list when its use has ended. While it releases all the
 * internal structures of the list it does not free the content of each item.
 *
 * @param[inout] handle Linked list handle
 */
void dlist_destroy(dlist_t *handle);

/**
 * @brief Destroy the list releasing with 'free()' its content
 *
 * @note Can be called on an non-empty list when its use has ended. It releases all the internal
 * structures of the list as well as each item placed into the list.
 *
 * @param[inout] handle Linked list handle
 */
void dlist_destroy_and_release(dlist_t *handle);

/**
 * @brief Initialize an iterator over a linked list
 *
 * @note After intialization the iterator will be pointing to the first item of the list.
 *
 * @param[in] handle Linked list handle
 * @param[out] iterator Iterator to initialize
 * @return One of the follwing error codes:
 * @retval ASTARTE_RESULT_NOT_FOUND if list is empty
 * @retval ASTARTE_RESULT_OK if operation has been successful
 */
astarte_result_t dlist_iterator_init(dlist_t *handle, dlist_iterator_t *iterator);

/**
 * @brief Advance the iterator to the next item of the list
 *
 * @param[inout] iterator Iterator to advance
 * @return One of the follwing error codes:
 * @retval ASTARTE_RESULT_NOT_FOUND if the end of the list has been reached
 * @retval ASTARTE_RESULT_OK if operation has been successful
 */
astarte_result_t dlist_iterator_advance(dlist_iterator_t *iterator);

/**
 * @brief Get the item pointed by the iterator
 *
 * @param[in] iterator Iterator to use for the operation
 * @return The list item pointed by the iterator
 */
void *dlist_iterator_get_item(dlist_iterator_t *iterator);

/**
 * @brief Replace the item pointed by the iterator
 *
 * @note Does not de-allocate the old item content.
 *
 * @param[inout] iterator Iterator to use for the operation
 * @param[in] value New element to insert in the linked list
 * @return The item that has been replaced
 */
void *dlist_iterator_replace_item(dlist_iterator_t *iterator, void *value);

/**
 * @brief Remove the item pointed by the iterator
 *
 * @note Does not de-allocate the old item content.
 *
 * @param[inout] iterator Iterator to use for the operation
 * @return The item that has been removed
 */
void *dlist_iterator_remove_item(dlist_iterator_t *iterator);

#endif // DLIST_H

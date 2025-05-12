/*
 * (C) Copyright 2023-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

#include <dlist.h>

#include <stdlib.h>
#include <string.h>

#include "astarte_device_sdk/result.h"

#include "log.h"

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

ASTARTE_LOG_MODULE_REGISTER("astarte-dlist");

struct dlist_node
{
    struct dlist_node *next;
    struct dlist_node *prev;
    void *value;
};

/************************************************
 *         Global functions definitions         *
 ***********************************************/

dlist_t dlist_init(void)
{
    dlist_t handle = { .head = NULL, .tail = NULL };
    return handle;
}

bool dlist_is_empty(dlist_t *handle)
{
    return (handle->head == NULL) && (handle->tail == NULL);
}

astarte_result_t dlist_append(dlist_t *handle, void *value)
{
    // Allocate a new node for the struct
    struct dlist_node *node = calloc(1, sizeof(struct dlist_node));
    if (!node) {
        ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return ASTARTE_RESULT_OUT_OF_MEMORY;
    }
    node->value = value;

    // If list is empty, add first node. Otherwise, add node at the end of list.
    node->next = NULL;
    if (dlist_is_empty(handle)) {
        node->prev = NULL;
        handle->head = node;
    } else {
        node->prev = handle->tail;
        handle->tail->next = node;
    }
    handle->tail = node;
    return ASTARTE_RESULT_OK;
}

astarte_result_t dlist_append_int(dlist_t *handle, int value)
{
    int *value_alloc = calloc(1, sizeof(int));
    if (!value_alloc) {
        ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return ASTARTE_RESULT_OUT_OF_MEMORY;
    }
    *value_alloc = value;
    return dlist_append(handle, value_alloc); // NOLINT(clang-analyzer-unix.Malloc)
}

void *dlist_remove_tail(dlist_t *handle)
{
    if (dlist_is_empty(handle)) {
        return NULL;
    }

    struct dlist_node *last_node = handle->tail;
    // Check if list contains a single node or multiple nodes
    if (handle->head == handle->tail) {
        handle->head = NULL;
        handle->tail = NULL;
    } else {
        last_node->prev->next = NULL;
        handle->tail = last_node->prev;
    }
    void *value = last_node->value;
    free(last_node);
    return value;
}

void dlist_destroy(dlist_t *handle)
{
    if (!dlist_is_empty(handle)) {
        struct dlist_node *node = handle->head;
        struct dlist_node *next_node = node->next;
        while (next_node) {
            free(node);
            node = next_node;
            next_node = node->next;
        }
        // Free the last node
        free(node);
        handle->head = NULL;
        handle->tail = NULL;
    }
}

void dlist_destroy_and_release(dlist_t *handle)
{
    if (!dlist_is_empty(handle)) {
        struct dlist_node *node = handle->head;
        struct dlist_node *next_node = node->next;
        while (next_node) {
            free(node->value);
            free(node);
            node = next_node;
            next_node = node->next;
        }
        // Free the last node
        free(node->value);
        free(node);
        handle->head = NULL;
        handle->tail = NULL;
    }
}

astarte_result_t dlist_iterator_init(dlist_t *handle, dlist_iterator_t *iterator)
{
    if (dlist_is_empty(handle)) {
        return ASTARTE_RESULT_NOT_FOUND;
    }
    iterator->handle = handle;
    iterator->node = handle->head;
    return ASTARTE_RESULT_OK;
}

astarte_result_t dlist_iterator_advance(dlist_iterator_t *iterator)
{
    if (iterator->node == iterator->handle->tail) {
        return ASTARTE_RESULT_NOT_FOUND;
    }
    iterator->node = iterator->node->next;
    return ASTARTE_RESULT_OK;
}

void *dlist_iterator_get_item(dlist_iterator_t *iterator)
{
    return iterator->node->value;
}

void *dlist_iterator_replace_item(dlist_iterator_t *iterator, void *value)
{
    void *old = iterator->node->value;
    iterator->node->value = value;
    return old;
}

void *dlist_iterator_remove_item(dlist_iterator_t *iterator)
{
    struct dlist_node *current = iterator->node;
    struct dlist_node *prev = current->prev;
    struct dlist_node *next = current->next;
    void *value = current->value;

    // Update the links of the surrounding nodes
    if (prev) {
        prev->next = next;
    } else {
        // If there is no previous node, we're at the head
        iterator->handle->head = next;
    }

    if (next) {
        next->prev = prev;
    } else {
        // If there is no next node, we're at the tail
        iterator->handle->tail = prev;
    }

    // Free the current node
    free(current);

    // Advance iterator to next node
    iterator->node = next;

    return value;
}

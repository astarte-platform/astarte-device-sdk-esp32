/*
 * (C) Copyright 2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

#include <astarte_linked_list.h>

#include <string.h>

#include <esp_log.h>

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define TAG "ASTARTE_LINKED_LIST"

struct astarte_linked_list_node
{
    struct astarte_linked_list_node *next;
    struct astarte_linked_list_node *prev;
    void *value;
};

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_linked_list_handle_t astarte_linked_list_init(void)
{
    astarte_linked_list_handle_t handle = { .head = NULL, .tail = NULL };
    return handle;
}

bool astarte_linked_list_is_empty(astarte_linked_list_handle_t *handle)
{
    return (handle->head == NULL) && (handle->tail == NULL);
}

astarte_err_t astarte_linked_list_append(astarte_linked_list_handle_t *handle, void *value)
{
    // Allocate a new node for the struct
    struct astarte_linked_list_node *node = calloc(1, sizeof(struct astarte_linked_list_node));
    if (!node) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        return ASTARTE_ERR_OUT_OF_MEMORY;
    }
    node->value = value;

    // If list is empty, add first node. Otherwise, add node at the end of list.
    node->next = NULL;
    if (astarte_linked_list_is_empty(handle)) {
        node->prev = NULL;
        handle->head = node;
    } else {
        node->prev = handle->tail;
        handle->tail->next = node;
    }
    handle->tail = node;
    return ASTARTE_OK;
}

astarte_err_t astarte_linked_list_remove_tail(astarte_linked_list_handle_t *handle, void **value)
{
    if (astarte_linked_list_is_empty(handle)) {
        return ASTARTE_ERR_NOT_FOUND;
    }

    struct astarte_linked_list_node *last_node = handle->tail;
    // Check if list contains a single node or multiple nodes
    if (handle->head == handle->tail) {
        handle->head = NULL;
        handle->tail = NULL;
    } else {
        last_node->prev->next = NULL;
        handle->tail = last_node->prev;
    }
    *value = last_node->value;
    free(last_node);
    return ASTARTE_OK;
}

void astarte_linked_list_destroy(astarte_linked_list_handle_t *handle)
{
    if (!astarte_linked_list_is_empty(handle)) {
        struct astarte_linked_list_node *node = handle->head;
        struct astarte_linked_list_node *next_node = node->next;
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

void astarte_linked_list_destroy_and_release(astarte_linked_list_handle_t *handle)
{
    if (!astarte_linked_list_is_empty(handle)) {
        struct astarte_linked_list_node *node = handle->head;
        struct astarte_linked_list_node *next_node = node->next;
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

astarte_err_t astarte_linked_list_iterator_init(
    astarte_linked_list_handle_t *handle, astarte_linked_list_iterator_t *iterator)
{
    if (astarte_linked_list_is_empty(handle)) {
        return ASTARTE_ERR_NOT_FOUND;
    }
    iterator->handle = handle;
    iterator->node = handle->head;
    return ASTARTE_OK;
}

astarte_err_t astarte_linked_list_iterator_advance(astarte_linked_list_iterator_t *iterator)
{
    if (iterator->node == iterator->handle->tail) {
        return ASTARTE_ERR_NOT_FOUND;
    }
    iterator->node = iterator->node->next;
    return ASTARTE_OK;
}

void astarte_linked_list_iterator_get_item(astarte_linked_list_iterator_t *iterator, void **value)
{
    *value = iterator->node->value;
}

void astarte_linked_list_iterator_replace_item(
    astarte_linked_list_iterator_t *iterator, void *value)
{
    iterator->node->value = value;
}

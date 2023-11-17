/*
 * (C) Copyright 2018-2023, SECO Mind Srl
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

struct astarte_linked_list_handle
{
    struct astarte_linked_list_node *head;
    struct astarte_linked_list_node *tail;
};

struct astarte_linked_list_node
{
    struct astarte_linked_list_node *next;
    struct astarte_linked_list_node *prev;
    void *value;
    size_t value_len;
};

struct astarte_linked_list_iterator
{
    struct astarte_linked_list_handle *handle;
    struct astarte_linked_list_node *node;
};

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_err_t astarte_linked_list_init(astarte_linked_list_handle_t *handle)
{
    struct astarte_linked_list_handle *handle_ref =
        calloc(1, sizeof(struct astarte_linked_list_handle));
    if (!handle_ref) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        return ASTARTE_ERR_OUT_OF_MEMORY;
    }
    handle_ref->head = NULL;
    handle_ref->tail = NULL;
    *handle = handle_ref;
    return ASTARTE_OK;
}

bool astarte_linked_list_is_empty(astarte_linked_list_handle_t handle)
{
    return (handle->head == NULL) && (handle->tail == NULL);
}

astarte_err_t astarte_linked_list_append(astarte_linked_list_handle_t handle,
    astarte_linked_list_item_t item)
{
    // Allocate a new node for the struct
    struct astarte_linked_list_node *node = calloc(1, sizeof(struct astarte_linked_list_node));
    if (!node) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        return ASTARTE_ERR_OUT_OF_MEMORY;
    }
    node->value = item.value;
    node->value_len = item.value_len;

    // If list is empty, add first node
    if (astarte_linked_list_is_empty(handle)) {
        node->next = node;
        node->prev = node;
        handle->head = node;
        handle->tail = node;
        return ASTARTE_OK;
    }

    // If list not empty, add normally
    node->next = handle->head;
    node->prev = handle->tail;
    handle->tail->next = node;
    handle->head->prev = node;
    handle->tail = node;

    return ASTARTE_OK;
}

astarte_err_t astarte_linked_list_remove_tail(astarte_linked_list_handle_t handle,
    astarte_linked_list_item_t *item)
{
    if (astarte_linked_list_is_empty(handle)) {
        return ASTARTE_ERR_NOT_FOUND;
    }

    // If list contains a single node
    if (handle->head == handle->tail) {
        struct astarte_linked_list_node *node = handle->head;
        handle->head = NULL;
        handle->tail = NULL;
        item->value = node->value;
        item->value_len = node->value_len;
        free(node);
        return ASTARTE_OK;
    }

    // If list contains multiple nodes
    struct astarte_linked_list_node *last_node = handle->tail;
    struct astarte_linked_list_node *second_last_node = last_node->prev;
    struct astarte_linked_list_node *first_node = handle->head;
    second_last_node->next = first_node;
    first_node->prev = second_last_node;
    handle->tail = second_last_node;
    item->value = last_node->value;
    item->value_len = last_node->value_len;
    free(last_node);

    return ASTARTE_OK;
}

astarte_err_t astarte_linked_list_find(astarte_linked_list_handle_t handle,
    astarte_linked_list_item_t item, bool *result)
{
    // Create an iterator
    astarte_linked_list_iterator_t iterator;
    astarte_err_t err = astarte_linked_list_iterator_init(handle, &iterator);
    // If list is empty return with false
    if (err == ASTARTE_ERR_NOT_FOUND) {
        *result = false;
        return ASTARTE_OK;
    }
    // Other iterator creationg errors
    if (err != ASTARTE_OK) {
        return err;
    }
    // Iterate through the list
    do {
        astarte_linked_list_item_t tmp_item;
        astarte_linked_list_iterator_get_item(iterator, &tmp_item);
        if ((tmp_item.value_len == item.value_len) &&
            (memcmp(tmp_item.value, item.value, item.value_len) == 0)) {
            *result = true;
            return ASTARTE_OK;
        }
    } while (ASTARTE_OK == astarte_linked_list_iterator_advance(iterator));
    // If not found
    *result = false;
    return ASTARTE_OK;
}

void astarte_linked_list_destroy(astarte_linked_list_handle_t handle)
{
    if (!astarte_linked_list_is_empty(handle)) {
        struct astarte_linked_list_node *node = handle->head;
        struct astarte_linked_list_node *tmp = node->next;
        do {
            free(node);
            node = tmp;
            tmp = node->next;
        } while (node != handle->head);
    }
    free(handle);
}

astarte_err_t astarte_linked_list_iterator_init(astarte_linked_list_handle_t handle,
    astarte_linked_list_iterator_t *iterator)
{
    *iterator = NULL;
    if (astarte_linked_list_is_empty(handle)) {
        return ASTARTE_ERR_NOT_FOUND;
    }
    struct astarte_linked_list_iterator *iterator_ref =
        calloc(1, sizeof(struct astarte_linked_list_iterator));
    if (!iterator_ref) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        return ASTARTE_ERR_OUT_OF_MEMORY;
    }
    iterator_ref->handle = handle;
    iterator_ref->node = handle->head;
    *iterator = iterator_ref;
    return ASTARTE_OK;
}

astarte_err_t astarte_linked_list_iterator_advance(astarte_linked_list_iterator_t iterator)
{
    if (iterator->node == iterator->handle->tail) {
        return ASTARTE_ERR_NOT_FOUND;
    }
    iterator->node = iterator->node->next;
    return ASTARTE_OK;
}

void astarte_linked_list_iterator_get_item(
    astarte_linked_list_iterator_t iterator, astarte_linked_list_item_t *item)
{
    item->value = iterator->node->value;
    item->value_len = iterator->node->value_len;
}

void astarte_linked_list_iterator_destroy(astarte_linked_list_iterator_t iterator)
{
    if (iterator) {
        free(iterator);
    }
}

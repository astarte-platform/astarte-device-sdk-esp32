/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

#include <astarte_set.h>

#include <string.h>

#include "astarte_linked_list.h"

#include <esp_log.h>

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define TAG "ASTARTE_SET"

struct astarte_set_handle
{
    astarte_linked_list_handle_t list_handle;
};

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_err_t astarte_set_init(astarte_set_handle_t *handle)
{
    struct astarte_set_handle *handle_ref = calloc(1, sizeof(struct astarte_set_handle));
    if (!handle_ref) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        return ASTARTE_ERR_OUT_OF_MEMORY;
    }
    astarte_err_t err = astarte_linked_list_init(&handle_ref->list_handle);
    if (err != ASTARTE_OK) {
        free(handle_ref);
        return err;
    }
    *handle = handle_ref;
    return ASTARTE_OK;
}

bool astarte_set_is_empty(astarte_set_handle_t handle)
{
    return astarte_linked_list_is_empty(handle->list_handle);
}

astarte_err_t astarte_set_add(astarte_set_handle_t handle, astarte_set_element_t element)
{
    // Ensure that the element is not already in the set
    bool find_res = false;
    astarte_linked_list_item_t item = {.value = element.value, .value_len = element.value_len};
    astarte_err_t err = astarte_linked_list_find(handle->list_handle, item, &find_res);
    if (err != ASTARTE_OK) {
        return err;
    }
    if (find_res) {
        ESP_LOGD(TAG, "Element already present in the set.");
        return ASTARTE_OK;
    }

    // Append element to the list
    return astarte_linked_list_append(handle->list_handle, item);
}

astarte_err_t astarte_set_pop(astarte_set_handle_t handle, astarte_set_element_t *element)
{
    astarte_linked_list_item_t item;
    astarte_err_t err = astarte_linked_list_remove_tail(handle->list_handle, &item);
    if (err == ASTARTE_OK) {
        element->value = item.value;
        element->value_len = item.value_len;
    }
    return err;
}

astarte_err_t astarte_set_has_element(astarte_set_handle_t handle, astarte_set_element_t element,
    bool *result)
{
    astarte_linked_list_item_t item = {.value = element.value, .value_len = element.value_len};
    return astarte_linked_list_find(handle->list_handle, item, result);
}

void astarte_set_destroy(astarte_set_handle_t handle)
{
    astarte_linked_list_destroy(handle->list_handle);
    free(handle);
}

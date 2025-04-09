/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "introspection.h"

#include <esp_log.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/interface.h"
#include "astarte_device_sdk/result.h"
#include "interface_private.h"

#include "dlist.h"
#include "log.h"

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

ASTARTE_LOG_MODULE_REGISTER("Astarte introspection");

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Function used to find an interface from its name
 *
 * @param[in] introspection a pointer to an introspection struct
 * @param[in] interface_name Interface name used in the comparison
 * @return The interface matching the name, NULL if not such interface exists
 */
static astarte_interface_t *find_interface_by_name(
    introspection_t *introspection, const char *interface_name);
/**
 * @brief Check whether an interface can be upgraded to a new one
 *
 * @details This function checks if the interfaces are the same, and if their major/minor versions
 * are compatible for an upgrade.
 *
 * @param[in] new The new interface
 * @param[in] old The old interface
 * @return True if the old interface can be substituted by the new one.
 */
static bool check_interface_upgradability(
    const astarte_interface_t *new, const astarte_interface_t *old);
/**
 * @brief Counts the number of digits of the passed paramter `num`
 *
 * @return The count of digits in num
 */
static uint8_t get_digit_count(uint32_t num);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

introspection_t introspection_new(void)
{
    return (introspection_t) { .list = dlist_init() };
}

void introspection_free(introspection_t introspection)
{
    dlist_destroy(&introspection.list);
}

astarte_result_t introspection_add(
    introspection_t *introspection, const astarte_interface_t *interface)
{
    astarte_result_t ares = interface_validate(interface);
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }

    astarte_interface_t *old_interface = find_interface_by_name(introspection, interface->name);
    if (old_interface) {
        return ASTARTE_RESULT_INTERFACE_ALREADY_PRESENT;
    }

    ares = dlist_append(&introspection->list, (void *) interface);
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }

    return ASTARTE_RESULT_OK;
}

astarte_result_t introspection_update(
    introspection_t *introspection, const astarte_interface_t *interface)
{

    astarte_result_t ares = interface_validate(interface);
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }

    dlist_iterator_t iterator;
    ares = dlist_iterator_init(&introspection->list, &iterator);
    while (ares != ASTARTE_RESULT_NOT_FOUND) {
        astarte_interface_t *fetched_interface = dlist_iterator_get_item(&iterator);
        if (check_interface_upgradability(interface, fetched_interface)) {
            dlist_iterator_replace_item(&iterator, (void *) interface);
            return ares;
        }
        ares = dlist_iterator_advance(&iterator);
    }

    ares = dlist_append(&introspection->list, (void *) interface);
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }

    return ASTARTE_RESULT_OK;
}

const astarte_interface_t *introspection_get(
    introspection_t *introspection, const char *interface_name)
{
    return find_interface_by_name(introspection, interface_name);
}

size_t introspection_get_string_size(introspection_t *introspection)
{
    size_t len = 0;

    dlist_iterator_t iterator;
    astarte_result_t ares = dlist_iterator_init(&introspection->list, &iterator);
    while (ares != ASTARTE_RESULT_NOT_FOUND) {
        astarte_interface_t *interface = dlist_iterator_get_item(&iterator);
        size_t name_len = strnlen(interface->name, ASTARTE_INTERFACE_NAME_MAX_SIZE);
        size_t major_len = get_digit_count(interface->major_version);
        size_t minor_len = get_digit_count(interface->minor_version);
        // size of the separators 3 (name:1:0; 2 ':' and 1 ';')
        // the separator ';' of the last interface is not present in an introspection
        // but we use it in the count as the byte needed for the null terminator char
        const static size_t separator_len = 3;

        len += name_len + major_len + minor_len + separator_len;
        ares = dlist_iterator_advance(&iterator);
    }

    // MAX to correctly handle the case of no interfaces
    len = MAX(1, len);

    // If introspection size is > 4KiB print a warning
    const size_t introspection_size_warn_level = 4096;
    if (len > introspection_size_warn_level) {
        ASTARTE_LOG_WRN("The introspection size is > 4KiB");
    }

    return len;
}

void introspection_fill_string(introspection_t *introspection, char *buffer, size_t buffer_size)
{
    size_t result_len = 0;

    dlist_iterator_t iterator;
    astarte_result_t ares = dlist_iterator_init(&introspection->list, &iterator);
    while (ares != ASTARTE_RESULT_NOT_FOUND) {
        astarte_interface_t *interface = dlist_iterator_get_item(&iterator);

        result_len += snprintf(buffer + result_len, buffer_size - result_len,
            "%s:%" PRIu32 ":%" PRIu32 ";", interface->name, interface->major_version,
            interface->minor_version);

        ares = dlist_iterator_advance(&iterator);
    }

    // Erase the last ';' char or null terminate the string when introspection is empty
    buffer[result_len] = '\0';
}

astarte_result_t introspection_iterator_init(
    introspection_t *introspection, introspection_iterator_t *iter)
{
    return dlist_iterator_init(&introspection->list, &iter->iter);
}

astarte_result_t introspection_iterator_advance(introspection_iterator_t *iter)
{
    return dlist_iterator_advance(&iter->iter);
}

const astarte_interface_t *introspection_iterator_get_interface(introspection_iterator_t *iter)
{
    return (astarte_interface_t *) dlist_iterator_get_item(&iter->iter);
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static astarte_interface_t *find_interface_by_name(
    introspection_t *introspection, const char *interface_name)
{
    dlist_iterator_t iterator;
    astarte_result_t ares = dlist_iterator_init(&introspection->list, &iterator);
    while (ares != ASTARTE_RESULT_NOT_FOUND) {
        astarte_interface_t *interface = dlist_iterator_get_item(&iterator);
        if ((strlen(interface_name) == strlen(interface->name))
            && (strcmp(interface_name, interface->name) == 0)) {
            return interface;
        }
        ares = dlist_iterator_advance(&iterator);
    }
    return NULL;
}

static bool check_interface_upgradability(
    const astarte_interface_t *new, const astarte_interface_t *old)
{
    // Check if interface names are the same
    if ((strlen(new->name) != strlen(old->name)) || (strcmp(new->name, old->name) != 0)) {
        ASTARTE_LOG_DBG("Interface names do not match");
        return false;
    }

    // Check if ownership and type are the same
    if ((new->ownership != old->ownership) || (new->type != old->type)) {
        ASTARTE_LOG_DBG("Interface ownership/type conflicts with the one in introspection");
        return false;
    }

    // Check if major versions align correctly
    if (new->major_version < old->major_version) {
        ASTARTE_LOG_DBG("Interface with smaller major version than one in introspection");
        return false;
    }

    // Check if minor versions aligns correctly
    if ((new->major_version == old->major_version) && (new->minor_version <= old->minor_version)) {
        ASTARTE_LOG_DBG("Interface with same major version and smaller or equal minor version than "
                        "the one in introspection");
        return false;
    }

    ASTARTE_LOG_DBG("Interface '%s' can be overwritten with new version '%" PRIu32 ".%" PRIu32 "'",
        old->name, new->major_version, new->minor_version);
    return true;
}

static uint8_t get_digit_count(uint32_t num)
{
    const uint8_t max_digit = 9;
    const uint8_t max_digit_plus_1 = max_digit + 1;

    uint8_t count = 1;

    while (num > max_digit) {
        num /= max_digit_plus_1;
        count += 1;
    }

    return count;
}

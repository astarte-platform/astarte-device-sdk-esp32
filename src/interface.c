/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "astarte_device_sdk/interface.h"
#include "interface_private.h"

#include <stdio.h>
#include <stdlib.h>

#include "mapping_private.h"

#include "log.h"

ASTARTE_LOG_MODULE_REGISTER(astarte_interface, CONFIG_ASTARTE_DEVICE_SDK_INTROSPECTION_LOG_LEVEL);

astarte_result_t astarte_interface_validate(const astarte_interface_t *interface)
{
    if (!interface) {
        ASTARTE_LOG_ERR("Received NULL interface reference");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    if ((interface->major_version == 0) && (interface->minor_version == 0)) {
        ASTARTE_LOG_ERR("Trying to add an interface with both major and minor version equal to 0");
        return ASTARTE_RESULT_INTERFACE_INVALID_VERSION;
    }

    return ASTARTE_RESULT_OK;
}

astarte_result_t astarte_interface_get_mapping_from_path(
    const astarte_interface_t *interface, const char *path, const astarte_mapping_t **mapping)
{
    for (size_t i = 0; i < interface->mappings_length; i++) {
        astarte_result_t ares = astarte_mapping_check_path(interface->mappings[i], path);
        if (ares == ASTARTE_RESULT_OK) {
            *mapping = &interface->mappings[i];
            return ASTARTE_RESULT_OK;
        }
        if (ares != ASTARTE_RESULT_MAPPING_PATH_MISMATCH) {
            return ASTARTE_RESULT_INTERNAL_ERROR;
        }
    }

    ASTARTE_LOG_DBG("Mapping not found in interface. Search path: %s.", path);
    return ASTARTE_RESULT_MAPPING_NOT_IN_INTERFACE;
}

astarte_result_t astarte_interface_get_mapping_from_paths(const astarte_interface_t *interface,
    const char *path1, const char *path2, const astarte_mapping_t **mapping)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    const size_t fullpath_size = strlen(path1) + 1 + strlen(path2) + 1;
    char *fullpath = calloc(fullpath_size, sizeof(char));
    if (!fullpath) {
        ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return ASTARTE_RESULT_OUT_OF_MEMORY;
    }
    if (snprintf(fullpath, fullpath_size, "%s/%s", path1, path2) != fullpath_size - 1) {
        ASTARTE_LOG_ERR("Failure in formatting the full path.");
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
        goto exit;
    }
    ares = astarte_interface_get_mapping_from_path(interface, fullpath, mapping);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR(
            "For path '%s' could not find mapping in interface '%s'.", fullpath, interface->name);
        ares = ASTARTE_RESULT_MAPPING_NOT_IN_INTERFACE;
        goto exit;
    }

exit:
    free(fullpath);
    return ares;
}

astarte_result_t astarte_interface_get_qos(
    const astarte_interface_t *interface, const char *path, int *qos)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    if (!qos) {
        ASTARTE_LOG_ERR("Missing QoS parameter in introspection_get_qos.");
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    const astarte_mapping_t *mapping = NULL;
    if (interface->aggregation == ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL) {
        ares = astarte_interface_get_mapping_from_path(interface, path, &mapping);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR(
                "Couldn't find mapping in interface %s for path %s.", interface->name, path);
            return ares;
        }
    } else {
        // All the QoS are the same in an aggregated interface, as such taking any of them works.
        mapping = interface->mappings;
    }

    *qos = mapping->reliability;

    return ASTARTE_RESULT_OK;
}

/**
 * @file generated_interfaces.c
 * @brief Contains automatically generated interfaces.
 *
 * @warning Do not modify this file manually.
 */

#include "generated_interfaces.h"

// Interface names should resemble as closely as possible their respective .json file names.
// NOLINTBEGIN(readability-identifier-naming)

static const astarte_mapping_t org_astarteplatform_esp32_examples_DeviceDatastream_mappings[1] = {

    {
        .endpoint = "/answer",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

const astarte_interface_t org_astarteplatform_esp32_examples_DeviceDatastream = {
    .name = "org.astarteplatform.esp32.examples.DeviceDatastream",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = org_astarteplatform_esp32_examples_DeviceDatastream_mappings,
    .mappings_length = 1U,
};

static const astarte_mapping_t org_astarteplatform_esp32_examples_ServerDatastream_mappings[1] = {

    {
        .endpoint = "/question",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

const astarte_interface_t org_astarteplatform_esp32_examples_ServerDatastream = {
    .name = "org.astarteplatform.esp32.examples.ServerDatastream",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = org_astarteplatform_esp32_examples_ServerDatastream_mappings,
    .mappings_length = 1U,
};

// NOLINTEND(readability-identifier-naming)

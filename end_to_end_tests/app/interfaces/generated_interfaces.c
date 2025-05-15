/**
 * @file generated_interfaces.c
 * @brief Contains automatically generated interfaces.
 *
 * @warning Do not modify this file manually.
 */

#include "generated_interfaces.h"

// Interface names should resemble as closely as possible their respective .json file names.
// NOLINTBEGIN(readability-identifier-naming)

static const astarte_mapping_t org_astarteplatform_end_to_end_DeviceAggregate_mappings[14] = {

    {
        .endpoint = "/%{sensor_id}/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/integer_endpoint",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/boolean_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/longinteger_endpoint",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/string_endpoint",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/binaryblob_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BINARYBLOB,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/datetime_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DATETIME,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/doublearray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLEARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/integerarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_INTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/booleanarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BOOLEANARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/longintegerarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/stringarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/binaryblobarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/datetimearray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DATETIMEARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

const astarte_interface_t org_astarteplatform_end_to_end_DeviceAggregate = {
    .name = "org.astarteplatform.end-to-end.DeviceAggregate",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = org_astarteplatform_end_to_end_DeviceAggregate_mappings,
    .mappings_length = 14U,
};

static const astarte_mapping_t org_astarteplatform_end_to_end_DeviceDatastream_mappings[14] = {

    {
        .endpoint = "/binaryblob_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BINARYBLOB,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/binaryblobarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/boolean_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/booleanarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BOOLEANARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/datetime_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DATETIME,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/datetimearray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DATETIMEARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/doublearray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLEARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/integer_endpoint",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/integerarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_INTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/longinteger_endpoint",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/longintegerarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/string_endpoint",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/stringarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

const astarte_interface_t org_astarteplatform_end_to_end_DeviceDatastream = {
    .name = "org.astarteplatform.end-to-end.DeviceDatastream",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = org_astarteplatform_end_to_end_DeviceDatastream_mappings,
    .mappings_length = 14U,
};

static const astarte_mapping_t org_astarteplatform_end_to_end_DeviceProperty_mappings[14] = {

    {
        .endpoint = "/%{sensor_id}/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/integer_endpoint",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/boolean_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/longinteger_endpoint",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/string_endpoint",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/binaryblob_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BINARYBLOB,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/datetime_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DATETIME,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/doublearray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLEARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/integerarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_INTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/booleanarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BOOLEANARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/longintegerarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/stringarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/binaryblobarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/datetimearray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DATETIMEARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
};

const astarte_interface_t org_astarteplatform_end_to_end_DeviceProperty = {
    .name = "org.astarteplatform.end-to-end.DeviceProperty",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = org_astarteplatform_end_to_end_DeviceProperty_mappings,
    .mappings_length = 14U,
};

static const astarte_mapping_t org_astarteplatform_end_to_end_ServerAggregate_mappings[14] = {

    {
        .endpoint = "/%{sensor_id}/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/integer_endpoint",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/boolean_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/longinteger_endpoint",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/string_endpoint",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/binaryblob_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BINARYBLOB,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/datetime_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DATETIME,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/doublearray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLEARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/integerarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_INTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/booleanarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BOOLEANARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/longintegerarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/stringarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/binaryblobarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{sensor_id}/datetimearray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DATETIMEARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

const astarte_interface_t org_astarteplatform_end_to_end_ServerAggregate = {
    .name = "org.astarteplatform.end-to-end.ServerAggregate",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = org_astarteplatform_end_to_end_ServerAggregate_mappings,
    .mappings_length = 14U,
};

static const astarte_mapping_t org_astarteplatform_end_to_end_ServerDatastream_mappings[14] = {

    {
        .endpoint = "/binaryblob_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BINARYBLOB,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/binaryblobarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/boolean_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/booleanarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BOOLEANARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/datetime_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DATETIME,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/datetimearray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DATETIMEARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/doublearray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLEARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/integer_endpoint",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/integerarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_INTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/longinteger_endpoint",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/longintegerarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/string_endpoint",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/stringarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

const astarte_interface_t org_astarteplatform_end_to_end_ServerDatastream = {
    .name = "org.astarteplatform.end-to-end.ServerDatastream",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = org_astarteplatform_end_to_end_ServerDatastream_mappings,
    .mappings_length = 14U,
};

static const astarte_mapping_t org_astarteplatform_end_to_end_ServerProperty_mappings[14] = {

    {
        .endpoint = "/%{sensor_id}/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/integer_endpoint",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/boolean_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/longinteger_endpoint",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/string_endpoint",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/binaryblob_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BINARYBLOB,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/datetime_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DATETIME,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/doublearray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLEARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/integerarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_INTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/booleanarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BOOLEANARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/longintegerarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/stringarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/binaryblobarray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{sensor_id}/datetimearray_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DATETIMEARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
};

const astarte_interface_t org_astarteplatform_end_to_end_ServerProperty = {
    .name = "org.astarteplatform.end-to-end.ServerProperty",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = org_astarteplatform_end_to_end_ServerProperty_mappings,
    .mappings_length = 14U,
};

// NOLINTEND(readability-identifier-naming)

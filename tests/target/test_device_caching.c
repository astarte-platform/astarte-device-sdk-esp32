/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_device_caching.h"
#include "device_caching.h"
#include "unity.h"

#include <esp_log.h>
#include <nvs_flash.h>
#include <string.h>

#define TAG "DEVICE CACHING TEST"

#define SYNCHRONIZATION_NAMESPACE "sync_namespace"
#define INTROSPECTION_NAMESPACE "intr_namespace"
#define PROPERTIES_NAMESPACE "prop_namespace"

// Some interfaces to use in the tests
static const astarte_mapping_t i1_mappings[2] = {
    {
        .endpoint = "/path_1",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/path_2",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};
static const astarte_interface_t interface1 = {
    .name = "interface_1_name",
    .major_version = 2,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = i1_mappings,
    .mappings_length = 2U,
};
static const astarte_mapping_t i2_mappings[2] = {
    {
        .endpoint = "/path_1",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/path_2",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};
static const astarte_interface_t interface2 = {
    .name = "interface_2_name",
    .major_version = 1,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = i2_mappings,
    .mappings_length = 2U,
};

#define I1_P1_PAYLOAD "iZDzYntqlF"
#define I1_P2_PAYLOAD "MZVqo9gM1K"
#define I2_P1_PAYLOAD "fMs7oUcFXC"
#define I2_P2_PAYLOAD "0dVXwKsxxf"

void test_device_caching_synchronization_set_get(void)
{
    bool sync = false;

    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Open storage
    device_caching_t device_caching;
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_open_namespace(&device_caching, SYNCHRONIZATION_NAMESPACE));

    // When not set
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_NOT_FOUND, device_caching_synchronization_get(device_caching, &sync));

    // Set true
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, device_caching_synchronization_set(device_caching, true));

    // Read true
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, device_caching_synchronization_get(device_caching, &sync));
    TEST_ASSERT_EQUAL(true, sync);

    // Set false
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, device_caching_synchronization_set(device_caching, false));

    // Read false
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, device_caching_synchronization_get(device_caching, &sync));
    TEST_ASSERT_EQUAL(false, sync);

    // Close storage
    device_caching_close_namespace(device_caching);
}

void test_device_caching_introspection_set_get(void)
{
    const char introspection[]
        = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque diam metus, viverra ac "
          "fringilla vitae, euismod ut neque. Pellentesque ornare, neque ut vestibulum auctor, "
          "dolor diam ultrices sem, in ultricies lectus elit consectetur urna. Suspendisse quis "
          "lorem augue. Sed varius dolor a orci condimentum porta. Ut accumsan finibus tortor eget "
          "ullamcorper. Integer nisl nulla, dapibus sed feugiat ac, eleifend nec leo. Pellentesque "
          "ullamcorper bibendum tellus, sed consequat nulla efficitur ac. Aenean ut fringilla "
          "orci, a lobortis mi. Duis ultricies nulla non vehicula commodo. Suspendisse pulvinar ex "
          "nunc. Proin laoreet dapibus dui non euismod. Morbi pretium faucibus ante id "
          "sollicitudin. Ut vitae neque accumsan, iaculis nisl ut, fringilla felis. Sed "
          "ullamcorper vehicula est non rutrum.";

    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Open storage
    device_caching_t device_caching;
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, device_caching_open_namespace(&device_caching, INTROSPECTION_NAMESPACE));

    // Check before storing the introspection
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_DEVICE_CACHING_OUTDATED_INTROSPECTION,
        device_caching_introspection_check(
            device_caching, "some introspection", sizeof("some introspection")));

    // Store the introspection
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_introspection_set(device_caching, introspection, sizeof(introspection)));

    // Check incorrect introspection
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_DEVICE_CACHING_OUTDATED_INTROSPECTION,
        device_caching_introspection_check(
            device_caching, "some introspection", sizeof("some introspection")));

    // Check correct introspection
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_introspection_check(device_caching, introspection, sizeof(introspection)));

    // Store the introspection
    const char updated_introspection[] = "Very simple introspection";
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_introspection_set(
            device_caching, updated_introspection, sizeof(updated_introspection)));

    // Check incorrect introspection
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_DEVICE_CACHING_OUTDATED_INTROSPECTION,
        device_caching_introspection_check(device_caching, introspection, sizeof(introspection)));

    // Check correct introspection
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_introspection_check(
            device_caching, updated_introspection, sizeof(updated_introspection)));

    // Close storage
    device_caching_close_namespace(device_caching);
}

void test_device_caching_property_store_load_delete_cycle(void)
{
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Variables where to store read payloads
    uint32_t i1_major_read = 0;
    astarte_data_t i1_p1_data_read = { 0 };
    const char *i1_p1_payload_read = NULL;
    astarte_data_t i1_p2_data_read = { 0 };
    const char *i1_p2_payload_read = NULL;
    uint32_t i2_major_read = 0;
    astarte_data_t i2_p1_data_read = { 0 };
    const char *i2_p1_payload_read = NULL;

    // Open storage
    device_caching_t device_caching;
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, device_caching_open_namespace(&device_caching, PROPERTIES_NAMESPACE));

    // Delete returns not found when property is not present
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_delete(
            device_caching, interface1.name, interface1.mappings[0].endpoint));

    // Load does not work when empty storage
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface1.name, interface1.mappings[0].endpoint, NULL, NULL));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface1.name, interface1.mappings[1].endpoint, NULL, NULL));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface2.name, interface2.mappings[0].endpoint, NULL, NULL));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface2.name, interface2.mappings[1].endpoint, NULL, NULL));

    // Store properties
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_store(device_caching, interface1.name,
            interface1.mappings[0].endpoint, interface1.major_version,
            astarte_data_from_string(I1_P1_PAYLOAD)));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_store(device_caching, interface1.name,
            interface1.mappings[1].endpoint, interface1.major_version,
            astarte_data_from_string(I1_P2_PAYLOAD)));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_store(device_caching, interface2.name,
            interface2.mappings[0].endpoint, interface2.major_version,
            astarte_data_from_string(I2_P1_PAYLOAD)));

    // Load properties
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_load(device_caching, interface1.name,
            interface1.mappings[0].endpoint, &i1_major_read, &i1_p1_data_read));
    TEST_ASSERT_EQUAL(interface1.major_version, i1_major_read);
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, astarte_data_to_string(i1_p1_data_read, &i1_p1_payload_read));
    TEST_ASSERT_EQUAL_STRING(I1_P1_PAYLOAD, i1_p1_payload_read);
    device_caching_property_destroy_loaded(i1_p1_data_read);

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_load(device_caching, interface1.name,
            interface1.mappings[1].endpoint, &i1_major_read, &i1_p2_data_read));
    TEST_ASSERT_EQUAL(interface1.major_version, i1_major_read);
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, astarte_data_to_string(i1_p2_data_read, &i1_p2_payload_read));
    TEST_ASSERT_EQUAL_STRING(I1_P2_PAYLOAD, i1_p2_payload_read);
    device_caching_property_destroy_loaded(i1_p2_data_read);

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_load(device_caching, interface2.name,
            interface2.mappings[0].endpoint, &i2_major_read, &i2_p1_data_read));
    TEST_ASSERT_EQUAL(interface2.major_version, i2_major_read);
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, astarte_data_to_string(i2_p1_data_read, &i2_p1_payload_read));
    TEST_ASSERT_EQUAL_STRING(I2_P1_PAYLOAD, i2_p1_payload_read);
    device_caching_property_destroy_loaded(i2_p1_data_read);

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface2.name, interface2.mappings[1].endpoint, NULL, NULL));

    // Delete property
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_delete(
            device_caching, interface1.name, interface1.mappings[1].endpoint));

    // Load updated properties
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_load(device_caching, interface1.name,
            interface1.mappings[0].endpoint, &i1_major_read, &i1_p1_data_read));
    TEST_ASSERT_EQUAL(interface1.major_version, i1_major_read);
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, astarte_data_to_string(i1_p1_data_read, &i1_p1_payload_read));
    TEST_ASSERT_EQUAL_STRING(I1_P1_PAYLOAD, i1_p1_payload_read);
    device_caching_property_destroy_loaded(i1_p1_data_read);

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface1.name, interface1.mappings[1].endpoint, NULL, NULL));

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_load(device_caching, interface2.name,
            interface2.mappings[0].endpoint, &i2_major_read, &i2_p1_data_read));
    TEST_ASSERT_EQUAL(interface2.major_version, i2_major_read);
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, astarte_data_to_string(i2_p1_data_read, &i2_p1_payload_read));
    TEST_ASSERT_EQUAL_STRING(I2_P1_PAYLOAD, i2_p1_payload_read);
    device_caching_property_destroy_loaded(i2_p1_data_read);

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface2.name, interface2.mappings[1].endpoint, NULL, NULL));

    // Delete property
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_delete(
            device_caching, interface1.name, interface1.mappings[0].endpoint));

    // Load updated properties
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface1.name, interface1.mappings[0].endpoint, NULL, NULL));

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface1.name, interface1.mappings[1].endpoint, NULL, NULL));

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_load(device_caching, interface2.name,
            interface2.mappings[0].endpoint, &i2_major_read, &i2_p1_data_read));
    TEST_ASSERT_EQUAL(interface2.major_version, i2_major_read);
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, astarte_data_to_string(i2_p1_data_read, &i2_p1_payload_read));
    TEST_ASSERT_EQUAL_STRING(I2_P1_PAYLOAD, i2_p1_payload_read);
    device_caching_property_destroy_loaded(i2_p1_data_read);

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface2.name, interface2.mappings[1].endpoint, NULL, NULL));

    // Delete property
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_delete(
            device_caching, interface2.name, interface2.mappings[0].endpoint));

    // Load updated properties
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface1.name, interface1.mappings[0].endpoint, NULL, NULL));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface1.name, interface1.mappings[1].endpoint, NULL, NULL));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface2.name, interface2.mappings[0].endpoint, NULL, NULL));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface2.name, interface2.mappings[1].endpoint, NULL, NULL));

    // Store property
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_store(device_caching, interface2.name,
            interface2.mappings[0].endpoint, interface2.major_version,
            astarte_data_from_string(I2_P1_PAYLOAD)));

    // Load updated properties
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface1.name, interface1.mappings[0].endpoint, NULL, NULL));

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface1.name, interface1.mappings[1].endpoint, NULL, NULL));

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_load(device_caching, interface2.name,
            interface2.mappings[0].endpoint, &i2_major_read, &i2_p1_data_read));
    TEST_ASSERT_EQUAL(interface2.major_version, i2_major_read);
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, astarte_data_to_string(i2_p1_data_read, &i2_p1_payload_read));
    TEST_ASSERT_EQUAL_STRING(I2_P1_PAYLOAD, i2_p1_payload_read);
    device_caching_property_destroy_loaded(i2_p1_data_read);

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND,
        device_caching_property_load(
            device_caching, interface2.name, interface2.mappings[1].endpoint, NULL, NULL));

    // Close storage
    device_caching_close_namespace(device_caching);
}

void test_device_caching_property_get_device_properties_string(void)
{
    const char expected_string[]
        = "interface_1_name/path_1;interface_1_name/path_2;interface_2_name/"
          "path_1;interface_2_name/path_2";

    size_t read_string_size = 0U;
    char read_string[sizeof(expected_string)] = { 0U };

    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Introspection to use for testing
    introspection_t introspection = introspection_new();
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_add(&introspection, &interface1));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_add(&introspection, &interface2));

    // Open storage
    device_caching_t device_caching;
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, device_caching_open_namespace(&device_caching, PROPERTIES_NAMESPACE));

    // Get the string when empty
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_get_device_properties_string(
            device_caching, &introspection, NULL, &read_string_size));
    TEST_ASSERT_EQUAL(0U, read_string_size);

    // Store properties
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_store(device_caching, interface1.name,
            interface1.mappings[0].endpoint, interface1.major_version,
            astarte_data_from_string(I1_P1_PAYLOAD)));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_store(device_caching, interface1.name,
            interface1.mappings[1].endpoint, interface1.major_version,
            astarte_data_from_string(I1_P2_PAYLOAD)));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_store(device_caching, interface2.name,
            interface2.mappings[0].endpoint, interface2.major_version,
            astarte_data_from_string(I2_P1_PAYLOAD)));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_store(device_caching, interface2.name,
            interface2.mappings[1].endpoint, interface2.major_version,
            astarte_data_from_string(I2_P2_PAYLOAD)));

    // Get the string
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_get_device_properties_string(
            device_caching, &introspection, NULL, &read_string_size));
    TEST_ASSERT_EQUAL(sizeof(expected_string), read_string_size);
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_get_device_properties_string(
            device_caching, &introspection, read_string, &read_string_size));
    TEST_ASSERT_EQUAL_STRING(expected_string, read_string);

    // Delete properties
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_delete(
            device_caching, interface1.name, interface1.mappings[0].endpoint));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_delete(
            device_caching, interface1.name, interface1.mappings[1].endpoint));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_delete(
            device_caching, interface2.name, interface2.mappings[0].endpoint));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_delete(
            device_caching, interface2.name, interface2.mappings[1].endpoint));

    // Get the string when empty
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_get_device_properties_string(
            device_caching, &introspection, NULL, &read_string_size));
    TEST_ASSERT_EQUAL(0U, read_string_size);

    // Close storage
    device_caching_close_namespace(device_caching);

    // Free the introspection
    introspection_free(introspection);
}

void test_device_caching_property_iteration(void)
{
    size_t read_interface_name_size = 0U;
    size_t read_path_size = 0U;
    char read_interface_name[17] = { 0U };
    char read_path[8] = { 0U };

    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Open storage
    device_caching_t device_caching;
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, device_caching_open_namespace(&device_caching, PROPERTIES_NAMESPACE));

    // Store properties
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_store(device_caching, interface1.name,
            interface1.mappings[0].endpoint, interface1.major_version,
            astarte_data_from_string(I1_P1_PAYLOAD)));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_store(device_caching, interface1.name,
            interface1.mappings[1].endpoint, interface1.major_version,
            astarte_data_from_string(I1_P2_PAYLOAD)));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_store(device_caching, interface2.name,
            interface2.mappings[0].endpoint, interface2.major_version,
            astarte_data_from_string(I2_P1_PAYLOAD)));

    // Create iterator
    device_caching_iterator_t iterator;
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, device_caching_property_iterator_init(device_caching, &iterator));

    // Fetch first property information
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_iterator_get(
            &iterator, NULL, &read_interface_name_size, NULL, &read_path_size));
    TEST_ASSERT_EQUAL(strlen(interface1.name) + 1, read_interface_name_size);
    TEST_ASSERT_EQUAL(strlen(interface1.mappings[0].endpoint) + 1, read_path_size);
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_iterator_get(
            &iterator, read_interface_name, &read_interface_name_size, read_path, &read_path_size));
    TEST_ASSERT_EQUAL_STRING(interface1.name, read_interface_name);
    TEST_ASSERT_EQUAL_STRING(interface1.mappings[0].endpoint, read_path);

    // Fetch next property
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, device_caching_property_iterator_next(&iterator));

    read_interface_name_size = 0U;
    read_path_size = 0U;
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_iterator_get(
            &iterator, NULL, &read_interface_name_size, NULL, &read_path_size));
    TEST_ASSERT_EQUAL(strlen(interface1.name) + 1, read_interface_name_size);
    TEST_ASSERT_EQUAL(strlen(interface1.mappings[1].endpoint) + 1, read_path_size);
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_iterator_get(
            &iterator, read_interface_name, &read_interface_name_size, read_path, &read_path_size));
    TEST_ASSERT_EQUAL_STRING(interface1.name, read_interface_name);
    TEST_ASSERT_EQUAL_STRING(interface1.mappings[1].endpoint, read_path);

    // Fetch next property
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, device_caching_property_iterator_next(&iterator));

    read_interface_name_size = 0U;
    read_path_size = 0U;
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_iterator_get(
            &iterator, NULL, &read_interface_name_size, NULL, &read_path_size));
    TEST_ASSERT_EQUAL(strlen(interface2.name) + 1, read_interface_name_size);
    TEST_ASSERT_EQUAL(strlen(interface2.mappings[0].endpoint) + 1, read_path_size);
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        device_caching_property_iterator_get(
            &iterator, read_interface_name, &read_interface_name_size, read_path, &read_path_size));
    TEST_ASSERT_EQUAL_STRING(interface2.name, read_interface_name);
    TEST_ASSERT_EQUAL_STRING(interface2.mappings[0].endpoint, read_path);

    // Fetch next property
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND, device_caching_property_iterator_next(&iterator));

    // Close storage
    device_caching_close_namespace(device_caching);
}

void test_device_caching_property_iteration_empty_memory(void)
{
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Open storage
    device_caching_t device_caching;
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, device_caching_open_namespace(&device_caching, PROPERTIES_NAMESPACE));

    // Create iterator
    device_caching_iterator_t iterator;
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_NOT_FOUND, device_caching_property_iterator_init(device_caching, &iterator));

    // Close storage
    device_caching_close_namespace(device_caching);
}

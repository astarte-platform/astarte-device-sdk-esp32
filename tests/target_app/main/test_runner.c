/**
 * This file is part of Astarte.
 *
 * Copyright 2018-2023 SECO Mind Srl
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 *
 **/

#include "unity.h"

#include <esp_log.h>

#include "test_bson_deserializer.h"
#include "test_bson_serializer.h"
#include "test_data.h"
#include "test_device_caching.h"
#include "test_dlist.h"
#include "test_interface.h"
#include "test_introspection.h"
#include "test_kv_storage.h"
#include "test_mapping.h"
#include "test_object.h"

void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bson_serializer_empty_document);
    RUN_TEST(test_bson_serializer_complete_document);

    RUN_TEST(test_bson_deserializer_check_validity);
    RUN_TEST(test_bson_deserializer_empty_bson_document);
    RUN_TEST(test_bson_deserializer_complete_bson_document);
    RUN_TEST(test_bson_deserializer_bson_document_lookup);

    RUN_TEST(test_data_serialize_integer);
    RUN_TEST(test_data_serialize_longinteger);
    RUN_TEST(test_data_serialize_double);
    RUN_TEST(test_data_serialize_boolean);
    RUN_TEST(test_data_serialize_string);
    RUN_TEST(test_data_serialize_integer_array);
    RUN_TEST(test_data_serialize_string_array);
    RUN_TEST(test_data_serialize_binaryblob_array);

    RUN_TEST(test_data_deserialize_astarte_data_from_incorrect_type);
    RUN_TEST(test_data_deserialize_astarte_data_from_binblob);
    RUN_TEST(test_data_deserialize_astarte_data_from_boolean);
    RUN_TEST(test_data_deserialize_astarte_data_from_datetime);
    RUN_TEST(test_data_deserialize_astarte_data_from_double);
    RUN_TEST(test_data_deserialize_astarte_data_from_integer);
    RUN_TEST(test_data_deserialize_astarte_data_from_longinteger);
    RUN_TEST(test_data_deserialize_astarte_data_from_string);
    RUN_TEST(test_data_deserialize_astarte_data_from_binblob_array);
    RUN_TEST(test_data_deserialize_astarte_data_from_boolean_array);
    RUN_TEST(test_data_deserialize_astarte_data_from_double_array);
    RUN_TEST(test_data_deserialize_astarte_data_from_datetime_array);
    RUN_TEST(test_data_deserialize_astarte_data_from_integer_array);
    RUN_TEST(test_data_deserialize_astarte_data_from_longinteger_array);
    RUN_TEST(test_data_deserialize_astarte_data_from_string_array);
    RUN_TEST(test_data_deserialize_astarte_data_from_empty_array);
    RUN_TEST(test_data_deserialize_astarte_data_from_mismatched_array_initial);
    RUN_TEST(test_data_deserialize_astarte_data_from_mismatched_array_final);

    RUN_TEST(test_dlist_is_empty);
    RUN_TEST(test_dlist_append_remove_tail);
    RUN_TEST(test_dlist_destroy);
    RUN_TEST(test_dlist_iterator);
    RUN_TEST(test_dlist_iterator_replace);

    RUN_TEST(test_interface_get_mapping);

    RUN_TEST(test_introspection_creation);
    RUN_TEST(test_introspection_add_get_update);
    RUN_TEST(test_introspection_get_string);
    RUN_TEST(test_introspection_iterator);

    RUN_TEST(test_device_caching_synchronization_set_get);
    RUN_TEST(test_device_caching_introspection_set_get);
    RUN_TEST(test_device_caching_property_store_load_delete_cycle);
    RUN_TEST(test_device_caching_property_get_device_properties_string);
    RUN_TEST(test_device_caching_property_iteration);
    RUN_TEST(test_device_caching_property_iteration_empty_memory);

    RUN_TEST(test_mapping_check_path_one_segment_no_pattern);
    RUN_TEST(test_astarte_mapping_check_path_multiple_segments_no_pattern);
    RUN_TEST(test_astarte_mapping_check_path_one_segment_single_pattern);
    RUN_TEST(test_astarte_mapping_check_path_multiple_segments_single_pattern);
    RUN_TEST(test_astarte_mapping_check_path_multiple_segments_three_patterns);
    RUN_TEST(test_astarte_mapping_check_data_double);
    RUN_TEST(test_astarte_mapping_check_data_doublearray);

    RUN_TEST(test_object_deserialize_astarte_object_from_aggregate);
    RUN_TEST(test_object_deserialize_astarte_object_from_empty_aggregate);

    RUN_TEST(test_kv_storage_set_get_cycle);
    RUN_TEST(test_kv_storage_erase_entry);
    RUN_TEST(test_kv_storage_iterator_to_empty_nvs);
    RUN_TEST(test_kv_storage_iterator);
    RUN_TEST(test_kv_storage_iterator_on_changing_memory_remove_first_and_only);
    RUN_TEST(test_kv_storage_iterator_on_changing_memory_remove_first);
    RUN_TEST(test_kv_storage_iterator_on_changing_memory_remove_last);
    RUN_TEST(test_kv_storage_iterator_on_changing_memory_remove_middle);
    UNITY_END();
}

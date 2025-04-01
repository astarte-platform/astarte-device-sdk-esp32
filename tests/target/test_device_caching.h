/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_DEVICE_CACHING_H
#define TEST_DEVICE_CACHING_H

#ifdef __cplusplus
extern "C" {
#endif

void test_device_caching_synchronization_set_get(void);
void test_device_caching_introspection_set_get(void);
void test_device_caching_property_store_load_delete_cycle(void);
void test_device_caching_property_get_device_properties_string(void);
void test_device_caching_property_iteration(void);
void test_device_caching_property_iteration_empty_memory(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_DEVICE_CACHING_H

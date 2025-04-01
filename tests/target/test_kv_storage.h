/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_KV_STORAGE_H
#define TEST_KV_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

void test_kv_storage_set_get_cycle(void);
void test_kv_storage_erase_entry(void);
void test_kv_storage_iterator_to_empty_nvs(void);
void test_kv_storage_iterator(void);
void test_kv_storage_iterator_on_changing_memory_remove_first_and_only(void);
void test_kv_storage_iterator_on_changing_memory_remove_first(void);
void test_kv_storage_iterator_on_changing_memory_remove_last(void);
void test_kv_storage_iterator_on_changing_memory_remove_middle(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_KV_STORAGE_H */

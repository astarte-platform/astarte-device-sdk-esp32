/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_DLIST_H
#define TEST_DLIST_H

#ifdef __cplusplus
extern "C" {
#endif

void test_dlist_is_empty(void);
void test_dlist_append_remove_tail(void);
void test_dlist_destroy(void);
void test_dlist_iterator(void);
void test_dlist_iterator_replace(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_DLIST_H

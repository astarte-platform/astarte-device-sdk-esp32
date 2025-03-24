/**
 * This file is part of Astarte.
 *
 * Copyright 2023 SECO Mind Srl
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

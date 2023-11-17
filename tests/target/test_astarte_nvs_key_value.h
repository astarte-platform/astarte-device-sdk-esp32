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

#ifndef _ASTARTE_TEST_NVS_KEY_VALUE_H_
#define _ASTARTE_TEST_NVS_KEY_VALUE_H_

#ifdef __cplusplus
extern "C" {
#endif

void test_astarte_nvs_key_value_set_get_cycle(void);
void test_astarte_nvs_key_value_erase_key(void);
void test_astarte_nvs_key_value_iterator(void);
void test_astarte_nvs_key_value_iterator_on_changing_memory_remove_first(void);
void test_astarte_nvs_key_value_iterator_on_changing_memory_remove_last(void);
void test_astarte_nvs_key_value_iterator_on_changing_memory_remove_middle(void);

#ifdef __cplusplus
}
#endif

#endif /* _ASTARTE_TEST_NVS_KEY_VALUE_H_ */

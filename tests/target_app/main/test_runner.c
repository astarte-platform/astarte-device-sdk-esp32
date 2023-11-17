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

#include "test_astarte_bson_deserializer.h"
#include "test_astarte_bson_serializer.h"
#include "test_astarte_nvs_key_value.h"

void app_main(void)
{
    // Disable logs for the bson deserializer to avoid printouts
    esp_log_level_set("ASTARTE_BSON_SERIALIZER", ESP_LOG_NONE);
    esp_log_level_set("ASTARTE_BSON_DESERIALIZER", ESP_LOG_NONE);
    // esp_log_level_set("NVS_KEY_VALUE", ESP_LOG_NONE);

    UNITY_BEGIN();
    RUN_TEST(test_astarte_bson_serializer_empty_document);
    RUN_TEST(test_astarte_bson_serializer_complete_document);

    RUN_TEST(test_astarte_bson_deserializer_check_validity);
    RUN_TEST(test_astarte_bson_deserializer_empty_bson_document);
    RUN_TEST(test_astarte_bson_deserializer_complete_bson_document);
    RUN_TEST(test_astarte_bson_deserializer_bson_document_lookup);

    RUN_TEST(test_astarte_nvs_key_value_set_get_cycle);
    RUN_TEST(test_astarte_nvs_key_value_erase_key);
    RUN_TEST(test_astarte_nvs_key_value_iterator);
    RUN_TEST(test_astarte_nvs_key_value_iterator_on_changing_memory_remove_first);
    RUN_TEST(test_astarte_nvs_key_value_iterator_on_changing_memory_remove_last);
    RUN_TEST(test_astarte_nvs_key_value_iterator_on_changing_memory_remove_middle);

    UNITY_END();
}

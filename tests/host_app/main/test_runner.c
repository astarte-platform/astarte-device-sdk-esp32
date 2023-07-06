// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "unity.h"

#include <esp_log.h>

#include "test_astarte_bson_serializer.h"
#include "test_uuid.h"

int main(int argc, char **argv)
{
    // Disable logs for the modules under test to avoid garbage prints
    esp_log_level_set("ASTARTE_BSON_SERIALIZER", ESP_LOG_NONE);
    esp_log_level_set("uuid", ESP_LOG_NONE);

    UNITY_BEGIN();
    RUN_TEST(test_astarte_bson_serializer_empty_document);
    RUN_TEST(test_astarte_bson_serializer_complete_document);

    RUN_TEST(test_uuid_from_string);
    RUN_TEST(test_uuid_to_string);
    RUN_TEST(test_uuid_generate_v4);
    RUN_TEST(test_uuid_generate_v5);
    int failures = UNITY_END();
    return failures;
}

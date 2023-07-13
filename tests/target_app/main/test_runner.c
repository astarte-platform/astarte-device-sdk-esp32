#include "unity.h"

#include <esp_log.h>

#include "test_astarte_bson_deserializer.h"
#include "test_astarte_bson_serializer.h"

void app_main(void)
{
    // Disable logs for the bson deserializer to avoid printouts
    esp_log_level_set("ASTARTE_BSON_SERIALIZER", ESP_LOG_NONE);
    esp_log_level_set("ASTARTE_BSON_DESERIALIZER", ESP_LOG_NONE);

    UNITY_BEGIN();
    RUN_TEST(test_astarte_bson_serializer_empty_document);
    RUN_TEST(test_astarte_bson_serializer_complete_document);

    RUN_TEST(test_astarte_bson_deserializer_check_validity);
    RUN_TEST(test_astarte_bson_deserializer_empty_bson_document);
    RUN_TEST(test_astarte_bson_deserializer_complete_bson_document);
    RUN_TEST(test_astarte_bson_deserializer_bson_document_lookup);
    UNITY_END();
}

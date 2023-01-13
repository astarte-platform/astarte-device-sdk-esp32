idf_component_register(SRCS "astarte_bson.c" "astarte_bson_serializer.c"
    "astarte_credentials.c" "astarte_device.c" "astarte_hwid.c"
    "astarte_pairing.c" "uuid.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES vfs mbedtls fatfs mqtt nvs_flash esp_http_client json wpa_supplicant)

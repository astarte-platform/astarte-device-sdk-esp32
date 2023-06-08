idf_component_register(SRCS "./src/astarte_bson.c" "./src/astarte_bson_serializer.c"
    "./src/astarte_credentials.c" "./src/astarte_device.c" "./src/astarte_hwid.c"
    "./src/astarte_pairing.c" "./src/uuid.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES vfs mbedtls fatfs mqtt nvs_flash esp_http_client json wpa_supplicant)

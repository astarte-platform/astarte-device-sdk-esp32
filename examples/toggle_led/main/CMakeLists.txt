idf_component_register(SRCS "./src/app_main.c" "./src/wifi_cfg.c" "./src/example_task.c"
                    "./src/led.c" "./src/button.c"
                    INCLUDE_DIRS "./inc"
                    REQUIRES astarte-device-sdk-esp32 nvs_flash esp_event esp_wifi driver)

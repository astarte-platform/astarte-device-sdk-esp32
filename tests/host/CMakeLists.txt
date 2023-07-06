idf_component_register(
    SRCS
        "test_uuid.c"
        "../../src/uuid.c"
    INCLUDE_DIRS
        "."
        "../../include"
    PRIV_REQUIRES unity cmock esp_system mbedtls esp_hw_support
)

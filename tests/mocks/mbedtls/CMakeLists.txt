# NOTE: This kind of mocking currently works on Linux targets only.
#       On Espressif chips, too many dependencies are missing at the moment.
message(STATUS "building MBEDTLS MOCKS")

idf_component_get_property(original_mbedtls_dir mbedtls COMPONENT_OVERRIDEN_DIR)

idf_component_mock(
    INCLUDE_DIRS
        "${original_mbedtls_dir}/mbedtls/include"
        "${original_mbedtls_dir}/mbedtls/include/mbedtls"
    REQUIRES esp_hw_support
    MOCK_HEADER_FILES
        "${original_mbedtls_dir}/mbedtls/include/mbedtls/md.h")

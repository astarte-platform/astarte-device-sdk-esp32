# NOTE: This kind of mocking currently works on Linux targets only.
#       On Espressif chips, too many dependencies are missing at the moment.
message(STATUS "building ESP SYSTEM MOCKS")

idf_component_get_property(original_esp_system_dir esp_system COMPONENT_OVERRIDEN_DIR)

idf_component_mock(INCLUDE_DIRS "${original_esp_system_dir}/include"
    REQUIRES esp_partition
    MOCK_HEADER_FILES ${original_esp_system_dir}/include/esp_system.h)

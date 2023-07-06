idf_component_register(
    SRCS
        "test_astarte_bson_serializer.c"
        "../../src/astarte_bson_serializer.c"
    INCLUDE_DIRS
        "."
        "../../include"
    PRIV_REQUIRES unity
)

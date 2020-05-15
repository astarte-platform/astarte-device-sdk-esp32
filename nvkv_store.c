#include "nvkv_store.h"

#include "include/astarte_bson.h"
#include "include/astarte_bson_serializer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NVS_GET_BLOB_SIZE(handle, key, out_len) nvs_get_blob(handle, key, NULL, out_len)
#define NVS_GET_BLOB(handle, key, out_value, out_len) nvs_get_blob(handle, key, out_value, out_len)
#define NVS_SET_BLOB(handle, key, value, len) nvs_set_blob(handle, key, value, len)

#define DUMMY
#ifdef DUMMY

#define ESP_OK 0

uint8_t temp_data[200];

static int nvs_get_blob(uint8_t *handle, const char *key, void *out, size_t *out_len)
{
    if (out) 
        memcpy(out, handle, 27);
    *out_len = 27;

    return ESP_OK;
}

static int nvs_set_blob(uint8_t *handle, const char *key, const void *value, size_t len)
{
    memcpy(handle, value, len);
    return ESP_OK;
}

#endif

static uint32_t sdbm_hash(const void *s, int len)
{
    const uint8_t *str = (const uint8_t *) s;

    uint32_t hash = 0;
    int c;

    for (int i = 0; i < len; i++) {
        c = *str++;
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

static void make_hash(const char *group, const char *key, char *out)
{
    unsigned long group_hash = sdbm_hash(group, strlen(group)) >> 4;
    unsigned long key_hash = sdbm_hash(key, strlen(key));

    snprintf(out, 16, "%7lx%8lx", group_hash, key_hash);
}

static nvkv_store_err_t nvkv_store_get_container_doc(NVS_HANDLE_TYPE nvs_handle,
        const char *group, const char *key, void **doc)
{
    char hash[16];
    make_hash(group, key, hash);

    size_t blob_size;

    if (NVS_GET_BLOB_SIZE(nvs_handle, hash, &blob_size) != ESP_OK) {
        return NVKV_STORE_ERROR;
    }

    void *document = malloc(blob_size);

    if (!document) {
        return NVKV_STORE_OUT_OF_MEMORY;
    }

    if (NVS_GET_BLOB(nvs_handle, hash, document, &blob_size) != ESP_OK) {
        free(document);
        return NVKV_STORE_ERROR;
    }

    *doc = document;
    return NVKV_STORE_OK;
}

#define IMPL_NVKV_STORE_GET_FUNCTION(type_name, type) \
    nvkv_store_err_t nvkv_store_get_##type_name(NVS_HANDLE_TYPE handle, \
            const char *group, const char *key, type *out_value) \
    { \
        void *container; \
        int result = nvkv_store_get_container_doc(handle, group, key, &container); \
        if (result != NVKV_STORE_OK) { \
            return result; \
        } \
\
        uint8_t bson_type; \
        const void *bson_kv = astarte_bson_key_lookup(key, container, &bson_type); \
        if (!bson_kv) { \
            free(container); \
            return NVKV_STORE_KEY_NOT_FOUND; \
        } \
\
        *out_value = astarte_bson_value_to_##type_name(bson_kv); \
\
        free(container);\
\
        return NVKV_STORE_OK; \
}

IMPL_NVKV_STORE_GET_FUNCTION(int8, int8_t)
IMPL_NVKV_STORE_GET_FUNCTION(int32, int32_t)
IMPL_NVKV_STORE_GET_FUNCTION(int64, int64_t)
IMPL_NVKV_STORE_GET_FUNCTION(double, double)

static nvkv_store_err_t nvkv_store_put_container_doc(NVS_HANDLE_TYPE nvs_handle,
        const char *group, const char *key, const void *document)
{
    char hash[16];
    make_hash(group, key, hash);

    size_t doc_size = astarte_bson_document_size(document);

    if (NVS_SET_BLOB(nvs_handle, hash, document, doc_size) != ESP_OK) {
        return NVKV_STORE_ERROR;
    }

    return NVKV_STORE_OK;
}

static void append_all_except(struct astarte_bson_serializer_t *bs, void *document, const char *key)
{
    //TODO
}

nvkv_store_err_t nvkv_store_put_int32(NVS_HANDLE_TYPE handle, const char *group, const char *key, int32_t value)
{
        void *container;
        int result = nvkv_store_get_container_doc(handle, group, key, &container);
        if (result != NVKV_STORE_OK) {
            return result;
        }

        struct astarte_bson_serializer_t bs;
        astarte_bson_serializer_init(&bs);

        append_all_except(&bs, container, key);
        astarte_bson_serializer_append_int32(&bs, key, value);
        astarte_bson_serializer_append_end_of_document(&bs);

        int size;
        const void *updated_container = astarte_bson_serializer_get_document(&bs, &size);

        nvkv_store_put_container_doc(handle, group, key, updated_container);
        
        astarte_bson_serializer_destroy(&bs);

        return NVKV_STORE_OK;
}

#ifdef DUMMY
int main(int argc, char **argv)
{
    char out[16];
    make_hash("com.astarte.SensorExample", "/this/is/my/path", out);
    printf("out: %s %li %li\n", out, strlen(out), (sizeof(unsigned long) * 8 / 4));

    nvkv_store_put_int32(temp_data, "group", "risafuranku", 0xCAFEBAB);
 
    int32_t value;
    if (nvkv_store_get_int32(temp_data, "group", "risafuranku", &value) == NVKV_STORE_OK) {
        printf("value: %x\n", value);
    }
}
#endif

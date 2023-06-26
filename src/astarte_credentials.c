/*
 * (C) Copyright 2018-2021, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#include <astarte_credentials.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_vfs.h>
#include <esp_vfs_fat.h>
#include <freertos/task.h>
#include <nvs.h>

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecp.h>
#include <mbedtls/entropy.h>
#include <mbedtls/oid.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define TAG "ASTARTE_CREDENTIALS"

#define PARTITION_NAME "astarte"
#define CREDENTIALS_MOUNTPOINT "/astarte"
#define CREDENTIALS_DIR_PATH CREDENTIALS_MOUNTPOINT "/ast_cred"
#define PRIVKEY_PATH CREDENTIALS_DIR_PATH "/device.key"
#define CSR_PATH CREDENTIALS_DIR_PATH "/device.csr"
#define CRT_PATH CREDENTIALS_DIR_PATH "/device.crt"

#define KEY_SIZE 2048
#define EXPONENT 65537
#define PRIVKEY_BUFFER_LENGTH 16000

#define CSR_BUFFER_LENGTH 4096

#define PAIRING_NAMESPACE "astarte_pairing"
#define CRED_SECRET_KEY "cred_secret"

#define CREDS_STORAGE_FUNCS(NAME)                                                                  \
    const astarte_credentials_storage_functions_t *NAME = creds_ctx.functions;

static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
static QueueHandle_t s_init_result_queue = NULL;

static astarte_err_t ensure_mounted();

static char *s_credentials_secret_partition_label = NVS_DEFAULT_PART_NAME;

static const astarte_credentials_storage_functions_t storage_funcs = {
    .astarte_credentials_store = astarte_credentials_store,
    .astarte_credentials_fetch = astarte_credentials_fetch,
    .astarte_credentials_exists = astarte_credentials_exists,
    .astarte_credentials_remove = astarte_credentials_remove,
};

static const astarte_credentials_storage_functions_t nvs_storage_funcs = {
    .astarte_credentials_store = astarte_credentials_nvs_store,
    .astarte_credentials_fetch = astarte_credentials_nvs_fetch,
    .astarte_credentials_exists = astarte_credentials_nvs_exists,
    .astarte_credentials_remove = astarte_credentials_nvs_remove,
};

static astarte_credentials_context_t creds_ctx = {
    .functions = &storage_funcs,
    .opaque = NULL,
};

void credentials_init_task(void *ctx)
{
    (void) ctx;

    astarte_err_t res = ASTARTE_ERR;
    if (!astarte_credentials_has_key()) {
        ESP_LOGD(TAG, "Private key not found, creating it.");
        res = astarte_credentials_create_key();
        if (res != ASTARTE_OK) {
            xQueueSend(s_init_result_queue, &res, portMAX_DELAY);
            vTaskDelete(NULL);
            return;
        }
    }

    if (!astarte_credentials_has_csr()) {
        ESP_LOGD(TAG, "CSR not found, creating it.");
        res = astarte_credentials_create_csr();
        if (res != ASTARTE_OK) {
            xQueueSend(s_init_result_queue, &res, portMAX_DELAY);
            vTaskDelete(NULL);
            return;
        }
    }

    res = ASTARTE_OK;
    xQueueSend(s_init_result_queue, &res, portMAX_DELAY);
    vTaskDelete(NULL);
}

astarte_err_t astarte_credentials_init()
{
    // astarte_credentials_is_initialized may mount filesystem as side effect
    if (astarte_credentials_is_initialized()) {
        return ASTARTE_OK;
    }

    if (!s_init_result_queue) {
        s_init_result_queue = xQueueCreate(1, sizeof(astarte_err_t));
        if (!s_init_result_queue) {
            ESP_LOGE(TAG, "Cannot initialize s_init_result_queue");
            return ASTARTE_ERR;
        }
    }

    TaskHandle_t task_handle = NULL;
    xTaskCreate(credentials_init_task, "credentials_init_task", 16384, NULL, tskIDLE_PRIORITY,
        &task_handle);
    if (!task_handle) {
        ESP_LOGE(TAG, "Cannot create credentials_init_task");
        return ASTARTE_ERR;
    }

    astarte_err_t result = ASTARTE_OK;
    xQueueReceive(s_init_result_queue, &result, portMAX_DELAY);

    return result;
}

bool astarte_credentials_is_initialized()
{
    // use automount when using default storage functions
    if (creds_ctx.functions == &storage_funcs) {
        // automount must be kept for compatibility reasons
        astarte_err_t err = ensure_mounted();
        if (err != ASTARTE_OK) {
            return false;
        }
    }

    return astarte_credentials_has_key() && astarte_credentials_has_csr();
}

astarte_err_t astarte_credentials_set_storage_context(astarte_credentials_context_t *creds_context)
{
    creds_ctx.functions = creds_context->functions;
    creds_ctx.opaque = creds_context->opaque;

    return ASTARTE_OK;
}

astarte_err_t astarte_credentials_use_nvs_storage(const char *partition_label)
{
    creds_ctx.functions = &nvs_storage_funcs;
    if (partition_label) {
        creds_ctx.opaque = strdup(partition_label);
        // Use the partition label also for the credentials secret
        s_credentials_secret_partition_label = creds_ctx.opaque;
    } else {
        creds_ctx.opaque = NVS_DEFAULT_PART_NAME;
    }

    return ASTARTE_OK;
}

static const char *astarte_credentials_filesystem_path(credential_type_t cred_type)
{
    switch (cred_type) {
        case ASTARTE_CREDENTIALS_CSR:
            return CSR_PATH;
        case ASTARTE_CREDENTIALS_KEY:
            return PRIVKEY_PATH;
        case ASTARTE_CREDENTIALS_CERTIFICATE:
            return CRT_PATH;
        default:
            return NULL;
    }
}

astarte_err_t astarte_credentials_store(
    void *opaque, credential_type_t cred_type, const void *credential, size_t length)
{
    (void) opaque;

    const char *path = astarte_credentials_filesystem_path(cred_type);
    if (!path) {
        return ASTARTE_ERR;
    }

    FILE *outf = fopen(path, "wb+");
    if (!outf) {
        ESP_LOGE(TAG, "Cannot open %s for writing", path);
        return ASTARTE_ERR_IO;
    }

    size_t written = fwrite(credential, sizeof(unsigned char), length, outf);
    if (written != length) {
        ESP_LOGE(TAG, "Cannot write credential to %s (len: %i, written: %zu)", path, (int) length,
            written);
        return ASTARTE_ERR_IO;
    }

    if (fclose(outf) != 0) {
        ESP_LOGE(TAG, "Cannot close %s", path);
        return ASTARTE_ERR_IO;
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_credentials_fetch(
    void *opaque, credential_type_t cred_type, char *out, size_t length)
{
    (void) opaque;

    const char *path = astarte_credentials_filesystem_path(cred_type);
    if (!path) {
        return ASTARTE_ERR;
    }

    FILE *infile = fopen(path, "rb");
    if (!infile) {
        ESP_LOGE(TAG, "Cannot open %s", path);
        return ASTARTE_ERR_NOT_FOUND;
    }

    (void) fread(out, 1, length, infile);
    if (ferror(infile)) {
        ESP_LOGE(TAG, "Error calling fread on %s", path);
        (void) fclose(infile);
        return ASTARTE_ERR_IO;
    }

    if (fclose(infile) != 0) {
        ESP_LOGE(TAG, "Cannot close %s", path);
        return ASTARTE_ERR_IO;
    }
    return ASTARTE_OK;
}

bool astarte_credentials_exists(void *opaque, credential_type_t cred_type)
{
    (void) opaque;

    const char *path = astarte_credentials_filesystem_path(cred_type);
    if (!path) {
        return false;
    }
    return access(path, R_OK) == 0;
}

astarte_err_t astarte_credentials_remove(void *opaque, credential_type_t cred_type)
{
    (void) opaque;

    const char *path = astarte_credentials_filesystem_path(cred_type);
    if (!path) {
        return ASTARTE_ERR;
    }
    return remove(path) == 0 ? ASTARTE_OK : ASTARTE_ERR;
}

static astarte_err_t ensure_mounted()
{
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 4,
        .format_if_mount_failed = true,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
    };
    esp_err_t err = ESP_OK;
    if (s_wl_handle == WL_INVALID_HANDLE) {
        ESP_LOGD(TAG, "Mounting FAT filesystem for credentials");
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        err = esp_vfs_fat_spiflash_mount_rw_wl(
            CREDENTIALS_MOUNTPOINT, PARTITION_NAME, &mount_config, &s_wl_handle);
#else
        err = esp_vfs_fat_spiflash_mount(
            CREDENTIALS_MOUNTPOINT, PARTITION_NAME, &mount_config, &s_wl_handle);
#endif
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        ESP_LOGE(TAG, "You have to add a partition named astarte to your partitions.csv file");
        return ASTARTE_ERR_PARTITION_SCHEME;
    }

    struct stat stats;
    if (stat(CREDENTIALS_DIR_PATH, &stats) < 0) {
        ESP_LOGD(TAG, "Directory %s doesn't exist, creating it", CREDENTIALS_DIR_PATH);
        if (mkdir(CREDENTIALS_DIR_PATH, 0700) < 0) {
            ESP_LOGE(TAG, "Cannot create %s directory", CREDENTIALS_DIR_PATH);
            return ASTARTE_ERR_IO;
        }
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_nvs_open_err_to_astarte(esp_err_t err)
{
    switch (err) {
        // NVS_NOT_FOUND is ok if we don't have credentials_secret yet
        case ESP_ERR_NVS_NOT_FOUND:
        case ESP_OK:
            return ASTARTE_OK;
        case ESP_ERR_NVS_NOT_INITIALIZED:
            ESP_LOGE(TAG, "Non-volatile storage not initialized");
            ESP_LOGE(TAG, "You have to call nvs_flash_init() in your initialization code");
            return ASTARTE_ERR_NVS_NOT_INITIALIZED;
        default:
            ESP_LOGE(
                TAG, "nvs_open error while reading credentials_secret: %s", esp_err_to_name(err));
            return ASTARTE_ERR_NVS;
    }
}

astarte_err_t astarte_nvs_rw_err_to_astarte(esp_err_t err)
{
    switch (err) {
        case ESP_OK:
            // Got it
            return ASTARTE_OK;

        // Here we come from NVS_NOT_FOUND above
        case ESP_ERR_NVS_INVALID_HANDLE:
        case ESP_ERR_NVS_NOT_FOUND:
            return ASTARTE_ERR_NOT_FOUND;

        default:
            ESP_LOGE(TAG, "nvs_get_str error: %s", esp_err_to_name(err));
            return ASTARTE_ERR_NVS;
    }
}

static const char *astarte_credentials_nvs_key(credential_type_t cred_type)
{
    switch (cred_type) {
        case ASTARTE_CREDENTIALS_CSR:
            return "device.csr";
        case ASTARTE_CREDENTIALS_KEY:
            return "device.key";
        case ASTARTE_CREDENTIALS_CERTIFICATE:
            return "device.crt";
        default:
            return NULL;
    }
}

astarte_err_t astarte_credentials_nvs_store(
    void *opaque, credential_type_t cred_type, const void *credential, size_t length)
{
    (void) length;

    const char *partition_label = opaque;
    const char *key = astarte_credentials_nvs_key(cred_type);
    if (!key) {
        return ASTARTE_ERR;
    }

    nvs_handle_t nvs = 0U;
    astarte_err_t res = astarte_nvs_open_err_to_astarte(
        nvs_open_from_partition(partition_label, PAIRING_NAMESPACE, NVS_READWRITE, &nvs));
    if (res != ASTARTE_OK) {
        goto err;
    }

    res = astarte_nvs_rw_err_to_astarte(nvs_set_str(nvs, key, credential));
    nvs_close(nvs);

err:
    if (res != ASTARTE_OK) {
        ESP_LOGE(TAG, "error while saving %s to NVS: %i", key, (int) res);
        return ASTARTE_ERR_NVS;
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_credentials_nvs_fetch(
    void *opaque, credential_type_t cred_type, char *out, size_t length)
{
    const char *partition_label = opaque;
    const char *key = astarte_credentials_nvs_key(cred_type);
    if (!key) {
        return ASTARTE_ERR;
    }

    nvs_handle_t nvs = 0U;
    astarte_err_t res = astarte_nvs_open_err_to_astarte(
        nvs_open_from_partition(partition_label, PAIRING_NAMESPACE, NVS_READONLY, &nvs));
    if (res != ASTARTE_OK) {
        goto err;
    }

    size_t len = 0U;
    nvs_get_str(nvs, key, NULL, &len);

    res = astarte_nvs_rw_err_to_astarte(nvs_get_str(nvs, key, out, &length));

err:
    nvs_close(nvs);
    return res;
}

bool astarte_credentials_nvs_exists(void *opaque, credential_type_t cred_type)
{
    const char *partition_label = opaque;
    const char *key = astarte_credentials_nvs_key(cred_type);
    if (!key) {
        return false;
    }

    nvs_handle_t nvs = 0U;
    astarte_err_t res = astarte_nvs_open_err_to_astarte(
        nvs_open_from_partition(partition_label, PAIRING_NAMESPACE, NVS_READONLY, &nvs));
    if (res != ASTARTE_OK) {
        goto err;
    }

    size_t length = 0U;
    res = astarte_nvs_rw_err_to_astarte(nvs_get_str(nvs, key, NULL, &length));

err:
    nvs_close(nvs);
    return res == ASTARTE_OK;
}

astarte_err_t astarte_credentials_nvs_remove(void *opaque, credential_type_t cred_type)
{
    const char *partition_label = opaque;
    const char *key = astarte_credentials_nvs_key(cred_type);
    if (!key) {
        return ASTARTE_ERR;
    }

    nvs_handle_t nvs = 0U;
    astarte_err_t res = astarte_nvs_open_err_to_astarte(
        nvs_open_from_partition(partition_label, PAIRING_NAMESPACE, NVS_READWRITE, &nvs));
    if (res != ASTARTE_OK) {
        goto err;
    }

    res = nvs_erase_key(nvs, key);

err:
    nvs_close(nvs);
    return res;
}

astarte_err_t astarte_credentials_create_key()
{
    astarte_err_t exit_code = ASTARTE_ERR_MBED_TLS;

    mbedtls_pk_context key;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    unsigned char *privkey_buffer = NULL;
    const char *pers = "astarte_credentials_create_key";

    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_pk_init(&key);
    mbedtls_entropy_init(&entropy);

    int ret = 0;
    ESP_LOGD(TAG, "Initializing entropy");
    if ((ret = mbedtls_ctr_drbg_seed(
             &ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen(pers)))
        != 0) {
        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
        goto exit;
    }

    ESP_LOGD(TAG, "Generating the EC key (using curve secp256r1)");

    if ((ret = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY))) != 0) {
        ESP_LOGE(TAG, "mbedtls_pk_setup returned %d", ret);
        goto exit;
    }

    if ((ret = mbedtls_ecp_gen_key(
             MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(key), mbedtls_ctr_drbg_random, &ctr_drbg))
        != 0) {
        ESP_LOGE(TAG, "mbedtls_ecp_gen_key returned %d", ret);
        goto exit;
    }

    ESP_LOGD(TAG, "Key succesfully generated");

    privkey_buffer = calloc(PRIVKEY_BUFFER_LENGTH, sizeof(unsigned char));
    if (!privkey_buffer) {
        exit_code = ASTARTE_ERR_OUT_OF_MEMORY;
        ESP_LOGE(TAG, "Cannot allocate private key buffer");
        goto exit;
    }

    if ((ret = mbedtls_pk_write_key_pem(&key, privkey_buffer, PRIVKEY_BUFFER_LENGTH)) != 0) {
        ESP_LOGE(TAG, "mbedtls_pk_write_key_pem returned %d", ret);
        goto exit;
    }
    size_t len = strlen((char *) privkey_buffer);

    ESP_LOGD(TAG, "Saving the private key");
    CREDS_STORAGE_FUNCS(funcs);
    astarte_err_t sres = funcs->astarte_credentials_store(
        creds_ctx.opaque, ASTARTE_CREDENTIALS_KEY, privkey_buffer, len);
    if (sres != ASTARTE_OK) {
        exit_code = sres;
        ESP_LOGE(TAG, "Cannot store private");
        goto exit;
    }

    ESP_LOGD(TAG, "Private key succesfully saved.");
    // TODO: this is useful in this phase, remove it later
    ESP_LOGD(TAG, "%.*s", len, privkey_buffer);
    exit_code = ASTARTE_OK;

    // Remove the CSR if present since the key is changed
    // We don't care if we fail since it could be not yet created
    if (funcs->astarte_credentials_remove(creds_ctx.opaque, ASTARTE_CREDENTIALS_CSR)
        == ASTARTE_OK) {
        ESP_LOGD(TAG, "Deleted old CSR");
    }

exit:
    free(privkey_buffer);

    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return exit_code;
}

astarte_err_t astarte_credentials_create_csr()
{
    astarte_err_t exit_code = ASTARTE_ERR_MBED_TLS;

    mbedtls_pk_context key;
    mbedtls_x509write_csr req;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    unsigned char *privkey_buffer = NULL;
    unsigned char *csr_buffer = NULL;
    const char *pers = "astarte_credentials_create_csr";

    mbedtls_x509write_csr_init(&req);
    mbedtls_pk_init(&key);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    mbedtls_x509write_csr_set_md_alg(&req, MBEDTLS_MD_SHA256);
    mbedtls_x509write_csr_set_ns_cert_type(&req, MBEDTLS_X509_NS_CERT_TYPE_SSL_CLIENT);

    int ret = 0;
    // We set the CN to a temporary value, it's just a placeholder since Pairing API will change it
    if ((ret = mbedtls_x509write_csr_set_subject_name(&req, "CN=temporary")) != 0) {
        ESP_LOGE(TAG, "mbedtls_x509write_csr_set_subject_name returned %d", ret);
        goto exit;
    }

    ESP_LOGD(TAG, "Initializing entropy");
    if ((ret = mbedtls_ctr_drbg_seed(
             &ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen(pers)))
        != 0) {
        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
        goto exit;
    }

    ESP_LOGD(TAG, "Loading the private key");
    privkey_buffer = calloc(PRIVKEY_BUFFER_LENGTH, sizeof(unsigned char));
    if (!privkey_buffer) {
        exit_code = ASTARTE_ERR_OUT_OF_MEMORY;
        ESP_LOGE(TAG, "Cannot allocate private key buffer");
        goto exit;
    }

    CREDS_STORAGE_FUNCS(funcs);
    astarte_err_t sres = funcs->astarte_credentials_fetch(
        creds_ctx.opaque, ASTARTE_CREDENTIALS_KEY, (char *) privkey_buffer, PRIVKEY_BUFFER_LENGTH);
    if (sres != ASTARTE_OK) {
        exit_code = sres;
        ESP_LOGE(TAG, "Cannot load the private key");
        goto exit;
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    if ((ret = mbedtls_pk_parse_key(&key, privkey_buffer, PRIVKEY_BUFFER_LENGTH, NULL, 0,
             mbedtls_ctr_drbg_random, &ctr_drbg))
        != 0) {
        ESP_LOGE(TAG, "mbedtls_pk_parse_key returned %d", ret);
        goto exit;
    }
#else
    if ((ret = mbedtls_pk_parse_key(&key, privkey_buffer, PRIVKEY_BUFFER_LENGTH, NULL, 0)) != 0) {
        ESP_LOGE(TAG, "mbedtls_pk_parse_key returned %d", ret);
        goto exit;
    }
#endif

    mbedtls_x509write_csr_set_key(&req, &key);

    csr_buffer = calloc(CSR_BUFFER_LENGTH, sizeof(unsigned char));
    if (!csr_buffer) {
        exit_code = ASTARTE_ERR_OUT_OF_MEMORY;
        ESP_LOGE(TAG, "Cannot allocate CSR buffer");
        goto exit;
    }

    if ((ret = mbedtls_x509write_csr_pem(
             &req, csr_buffer, CSR_BUFFER_LENGTH, mbedtls_ctr_drbg_random, &ctr_drbg))
        != 0) {
        ESP_LOGE(TAG, "mbedtls_x509write_csr_pem returned %d", ret);
        goto exit;
    }
    size_t len = strlen((char *) csr_buffer);

    ESP_LOGD(TAG, "Saving the CSR");
    sres = funcs->astarte_credentials_store(
        creds_ctx.opaque, ASTARTE_CREDENTIALS_CSR, csr_buffer, len);
    if (sres != ASTARTE_OK) {
        exit_code = sres;
        ESP_LOGE(TAG, "Cannot store the CSR");
        goto exit;
    }

    ESP_LOGD(TAG, "CSR succesfully created.");
    // TODO: this is useful in this phase, remove it later
    ESP_LOGD(TAG, "%.*s", len, csr_buffer);
    exit_code = ASTARTE_OK;

exit:
    free(csr_buffer);
    free(privkey_buffer);

    mbedtls_x509write_csr_free(&req);
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return exit_code;
}

astarte_err_t astarte_credentials_save_certificate(const char *cert_pem)
{
    if (!cert_pem) {
        ESP_LOGE(TAG, "cert_pem is NULL");
        return ASTARTE_ERR;
    }

    size_t len = strlen(cert_pem);

    ESP_LOGD(TAG, "Saving the certificate");
    CREDS_STORAGE_FUNCS(funcs);
    astarte_err_t sres = funcs->astarte_credentials_store(
        creds_ctx.opaque, ASTARTE_CREDENTIALS_CERTIFICATE, cert_pem, len);
    if (sres != ASTARTE_OK) {
        return sres;
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_credentials_delete_certificate()
{
    CREDS_STORAGE_FUNCS(funcs);

    astarte_err_t ret
        = funcs->astarte_credentials_remove(creds_ctx.opaque, ASTARTE_CREDENTIALS_CERTIFICATE);
    if (ret != ASTARTE_OK) {
        ESP_LOGE(TAG, "certificate remove failed");
    }

    return ret;
}

astarte_err_t astarte_credentials_get_csr(char *out, size_t length)
{
    CREDS_STORAGE_FUNCS(funcs);
    return funcs->astarte_credentials_fetch(creds_ctx.opaque, ASTARTE_CREDENTIALS_CSR, out, length);
}

astarte_err_t astarte_credentials_get_certificate(char *out, size_t length)
{
    CREDS_STORAGE_FUNCS(funcs);
    return funcs->astarte_credentials_fetch(
        creds_ctx.opaque, ASTARTE_CREDENTIALS_CERTIFICATE, out, length);
}

astarte_err_t astarte_credentials_get_certificate_common_name(
    const char *cert_pem, char *out, size_t length)
{
    astarte_err_t exit_code = ASTARTE_ERR_MBED_TLS;
    mbedtls_x509_crt crt;
    mbedtls_x509_crt_init(&crt);

    size_t cert_length = strlen(cert_pem) + 1; // + 1 for NULL terminator, as per documentation
    int ret = mbedtls_x509_crt_parse(&crt, (unsigned char *) cert_pem, cert_length);
    if (ret < 0) {
        ESP_LOGE(TAG, "mbedtls_x509_crt_parse_file returned %d", ret);
        goto exit;
    }

    mbedtls_x509_name *subj_it = &crt.subject;
    while (subj_it && (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_CN, &subj_it->oid) != 0)) {
        subj_it = subj_it->next;
    }

    if (!subj_it) {
        ESP_LOGE(TAG, "CN not found in certificate");
        exit_code = ASTARTE_ERR_NOT_FOUND;
        goto exit;
    }

    ret = snprintf(out, length, "%.*s", subj_it->val.len, subj_it->val.p);
    if ((ret < 0) || (ret >= length)) {
        ESP_LOGE(TAG, "Error encoding certificate common name");
        exit_code = ASTARTE_ERR;
        goto exit;
    }
    exit_code = ASTARTE_OK;

exit:
    mbedtls_x509_crt_free(&crt);

    return exit_code;
}

astarte_err_t astarte_credentials_get_key(char *out, size_t length)
{
    CREDS_STORAGE_FUNCS(funcs);
    return funcs->astarte_credentials_fetch(creds_ctx.opaque, ASTARTE_CREDENTIALS_KEY, out, length);
}

astarte_err_t astarte_credentials_get_stored_credentials_secret(char *out, size_t length)
{
    nvs_handle_t nvs = 0U;
    esp_err_t err = nvs_open_from_partition(
        s_credentials_secret_partition_label, PAIRING_NAMESPACE, NVS_READONLY, &nvs);
    switch (err) {
        // NVS_NOT_FOUND is ok if we don't have credentials_secret yet
        case ESP_ERR_NVS_NOT_FOUND:
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_INITIALIZED:
            ESP_LOGE(TAG, "Non-volatile storage not initialized");
            ESP_LOGE(TAG, "You have to call nvs_flash_init() in your initialization code");
            return ASTARTE_ERR_NVS_NOT_INITIALIZED;
        default:
            ESP_LOGE(
                TAG, "nvs_open error while reading credentials_secret: %s", esp_err_to_name(err));
            return ASTARTE_ERR_NVS;
    }

    err = nvs_get_str(nvs, CRED_SECRET_KEY, out, &length);
    nvs_close(nvs);

    switch (err) {
        case ESP_OK:
            // Got it
            return ASTARTE_OK;

        // Here we come from NVS_NOT_FOUND above
        case ESP_ERR_NVS_INVALID_HANDLE:
        case ESP_ERR_NVS_NOT_FOUND:
            return ASTARTE_ERR_NOT_FOUND;

        default:
            ESP_LOGE(TAG, "nvs_get_str error: %s", esp_err_to_name(err));
            return ASTARTE_ERR_NVS;
    }
}

astarte_err_t astarte_credentials_set_stored_credentials_secret(const char *credentials_secret)
{
    nvs_handle_t nvs = 0U;
    esp_err_t err = nvs_open_from_partition(
        s_credentials_secret_partition_label, PAIRING_NAMESPACE, NVS_READWRITE, &nvs);
    switch (err) {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_INITIALIZED:
            ESP_LOGE(TAG, "Non-volatile storage not initialized");
            ESP_LOGE(TAG, "You have to call nvs_flash_init() in your initialization code");
            return ASTARTE_ERR_NVS_NOT_INITIALIZED;
        default:
            ESP_LOGE(
                TAG, "nvs_open error while reading credentials_secret: %s", esp_err_to_name(err));
            return ASTARTE_ERR_NVS;
    }

    err = nvs_set_str(nvs, CRED_SECRET_KEY, credentials_secret);
    nvs_close(nvs);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG, "nvs_set_str error while saving credentials_secret: %s", esp_err_to_name(err));
        return ASTARTE_ERR_NVS;
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_credentials_erase_stored_credentials_secret()
{
    nvs_handle_t nvs = 0U;
    esp_err_t err = nvs_open_from_partition(
        s_credentials_secret_partition_label, PAIRING_NAMESPACE, NVS_READWRITE, &nvs);
    switch (err) {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_INITIALIZED:
            ESP_LOGE(TAG, "Non-volatile storage not initialized");
            ESP_LOGE(TAG, "You have to call nvs_flash_init() in your initialization code");
            return ASTARTE_ERR_NVS_NOT_INITIALIZED;
        default:
            ESP_LOGE(
                TAG, "nvs_open error while reading credentials_secret: %s", esp_err_to_name(err));
            return ASTARTE_ERR_NVS;
    }

    err = nvs_erase_key(nvs, CRED_SECRET_KEY);
    nvs_close(nvs);

    switch (err) {
        case ESP_OK:
            return ASTARTE_OK;

        // Here we come from NVS_NOT_FOUND above
        case ESP_ERR_NVS_INVALID_HANDLE:
        case ESP_ERR_NVS_NOT_FOUND:
            return ASTARTE_ERR_NOT_FOUND;

        default:
            ESP_LOGE(TAG, "nvs_erase_key error: %s", esp_err_to_name(err));
            return ASTARTE_ERR_NVS;
    }

    return ASTARTE_OK;
}

bool astarte_credentials_has_certificate()
{
    CREDS_STORAGE_FUNCS(funcs);
    if (!funcs->astarte_credentials_exists(creds_ctx.opaque, ASTARTE_CREDENTIALS_CERTIFICATE)) {
        return false;
    }

    astarte_err_t astarte_ret = ASTARTE_ERR;
    char *client_crt_cn = NULL;

    char *client_crt_pem = calloc(CERT_LENGTH, sizeof(char));
    if (!client_crt_pem) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        astarte_ret = ASTARTE_ERR_OUT_OF_MEMORY;
        goto exit;
    }

    astarte_ret = astarte_credentials_get_certificate(client_crt_pem, CERT_LENGTH);
    if (astarte_ret != ASTARTE_OK) {
        ESP_LOGE(TAG, "astarte_credentials_get_certificate returned %d", astarte_ret);
        goto exit;
    }

    client_crt_cn = calloc(CN_LENGTH, sizeof(char));
    if (!client_crt_cn) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto exit;
    }

    astarte_ret
        = astarte_credentials_get_certificate_common_name(client_crt_pem, client_crt_cn, CN_LENGTH);
    if (astarte_ret != ASTARTE_OK) {
        ESP_LOGE(TAG, "astarte_credentials_get_certificate_common_name returned %d", astarte_ret);
        goto exit;
    }

    astarte_ret = ASTARTE_OK;

exit:
    free(client_crt_pem);
    free(client_crt_cn);

    return astarte_ret == ASTARTE_OK;
}

bool astarte_credentials_has_csr()
{
    CREDS_STORAGE_FUNCS(funcs);
    return funcs->astarte_credentials_exists(creds_ctx.opaque, ASTARTE_CREDENTIALS_CSR);
}

bool astarte_credentials_has_key()
{
    CREDS_STORAGE_FUNCS(funcs);
    return funcs->astarte_credentials_exists(creds_ctx.opaque, ASTARTE_CREDENTIALS_KEY);
}

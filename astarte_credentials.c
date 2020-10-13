/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
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

static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
static QueueHandle_t s_init_result_queue = NULL;

static astarte_err_t ensure_mounted();

void credentials_init_task(void *ctx)
{
    struct stat st;
    astarte_err_t res = ASTARTE_ERR;
    if (stat(CREDENTIALS_DIR_PATH, &st) < 0) {
        ESP_LOGI(TAG, "Directory %s doesn't exist, creating it", CREDENTIALS_DIR_PATH);
        if (mkdir(CREDENTIALS_DIR_PATH, 0700) < 0) {
            ESP_LOGE(TAG, "Cannot create %s directory", CREDENTIALS_DIR_PATH);
            xQueueSend(s_init_result_queue, &res, portMAX_DELAY);
            vTaskDelete(NULL);
            return;
        }
    }

    if (!astarte_credentials_has_key()) {
        ESP_LOGI(TAG, "Private key not found, creating it.");
        res = astarte_credentials_create_key();
        if (res != ASTARTE_OK) {
            xQueueSend(s_init_result_queue, &res, portMAX_DELAY);
            vTaskDelete(NULL);
            return;
        }
    }

    if (!astarte_credentials_has_csr()) {
        ESP_LOGI(TAG, "CSR not found, creating it.");
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
    if (astarte_credentials_is_initialized()) {
        return ASTARTE_OK;
    }

    astarte_err_t err = ensure_mounted();
    if (err != ASTARTE_OK) {
        return err;
    }

    if (!s_init_result_queue) {
        s_init_result_queue = xQueueCreate(1, sizeof(astarte_err_t));
        if (!s_init_result_queue) {
            ESP_LOGE(TAG, "Cannot initialize s_init_result_queue");
            return ASTARTE_ERR;
        }
    }

    TaskHandle_t task_handle;
    xTaskCreate(credentials_init_task, "credentials_init_task", 16384, NULL, tskIDLE_PRIORITY,
        &task_handle);
    if (!task_handle) {
        ESP_LOGE(TAG, "Cannot create credentials_init_task");
        return ASTARTE_ERR;
    }

    astarte_err_t result;
    xQueueReceive(s_init_result_queue, &result, portMAX_DELAY);

    return result;
}

int astarte_credentials_is_initialized()
{
    astarte_err_t err = ensure_mounted();
    if (err != ASTARTE_OK) {
        return 0;
    }

    return astarte_credentials_has_key() && astarte_credentials_has_csr();
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
        ESP_LOGI(TAG, "Mounting FAT filesystem for credentials");
        err = esp_vfs_fat_spiflash_mount(
            CREDENTIALS_MOUNTPOINT, PARTITION_NAME, &mount_config, &s_wl_handle);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        ESP_LOGE(TAG, "You have to add a partition named astarte to your partitions.csv file");
        return ASTARTE_ERR_PARTITION_SCHEME;
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_credentials_create_key()
{
    astarte_err_t exit_code = ASTARTE_ERR_MBED_TLS;

    mbedtls_pk_context key;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    FILE *fpriv = NULL;
    unsigned char *privkey_buffer = NULL;
    const char *pers = "astarte_credentials_create_key";

    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_pk_init(&key);
    mbedtls_entropy_init(&entropy);

    int ret = 0;
    ESP_LOGI(TAG, "Initializing entropy");
    if ((ret = mbedtls_ctr_drbg_seed(
             &ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen(pers)))
        != 0) {
        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
        goto exit;
    }

    ESP_LOGI(TAG, "Generating the EC key (using curve secp256r1)");

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

    ESP_LOGI(TAG, "Key succesfully generated");

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

    ESP_LOGI(TAG, "Saving the private key in %s", PRIVKEY_PATH);
    fpriv = fopen(PRIVKEY_PATH, "wb+");
    if (!fpriv) {
        exit_code = ASTARTE_ERR_IO;
        ESP_LOGE(TAG, "Cannot open %s for writing", PRIVKEY_PATH);
        goto exit;
    }

    if (fwrite(privkey_buffer, sizeof(unsigned char), len, fpriv) != len) {
        exit_code = ASTARTE_ERR_IO;
        ESP_LOGE(TAG, "Cannot write private key to %s", PRIVKEY_PATH);
        goto exit;
    }

    ESP_LOGI(TAG, "Private key succesfully saved.");
    // TODO: this is useful in this phase, remove it later
    ESP_LOGI(TAG, "%.*s", len, privkey_buffer);
    exit_code = ASTARTE_OK;

    // Remove the CSR if present since the key is changed
    // We don't care if we fail since it could be not yet created
    if (remove(CSR_PATH) == 0) {
        ESP_LOGI(TAG, "Deleted old CSR");
    }

exit:
    free(privkey_buffer);

    if (fpriv != NULL) {
        fclose(fpriv);
    }

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
    FILE *fcsr = NULL;
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

    ESP_LOGI(TAG, "Initializing entropy");
    if ((ret = mbedtls_ctr_drbg_seed(
             &ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen(pers)))
        != 0) {
        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
        goto exit;
    }

    ESP_LOGI(TAG, "Loading the private key");
    if ((ret = mbedtls_pk_parse_keyfile(&key, PRIVKEY_PATH, NULL)) != 0) {
        ESP_LOGE(TAG, "mbedtls_pk_parse_key returned %d", ret);
        goto exit;
    }

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

    ESP_LOGI(TAG, "Saving the CSR in %s", CSR_PATH);
    fcsr = fopen(CSR_PATH, "wb+");
    if (!fcsr) {
        exit_code = ASTARTE_ERR_IO;
        ESP_LOGE(TAG, "Cannot open %s for writing", CSR_PATH);
        goto exit;
    }

    if (fwrite(csr_buffer, sizeof(unsigned char), len, fcsr) != len) {
        exit_code = ASTARTE_ERR_IO;
        ESP_LOGE(TAG, "Cannot write CSR to %s", CSR_PATH);
        goto exit;
    }

    ESP_LOGI(TAG, "CSR succesfully created.");
    // TODO: this is useful in this phase, remove it later
    ESP_LOGI(TAG, "%.*s", len, csr_buffer);
    exit_code = ASTARTE_OK;

exit:
    free(csr_buffer);

    if (fcsr) {
        fclose(fcsr);
    }

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

    FILE *fcert = fopen(CRT_PATH, "wb+");
    if (!fcert) {
        ESP_LOGE(TAG, "Cannot open %s", CRT_PATH);
        return ASTARTE_ERR_IO;
    }

    size_t len = strlen(cert_pem);
    int ret;
    if ((ret = fwrite(cert_pem, 1, len, fcert)) != len) {
        ESP_LOGE(TAG, "fwrite returned %d", ret);
        fclose(fcert);
        return ASTARTE_ERR_IO;
    }

    fclose(fcert);
    return ASTARTE_OK;
}

astarte_err_t astarte_credentials_delete_certificate()
{
    int ret;
    if ((ret = remove(CRT_PATH)) != 0) {
        ESP_LOGE(TAG, "remove returned %d", ret);
        return ASTARTE_ERR_IO;
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_credentials_get_csr(char *out, size_t length)
{
    FILE *fcsr = fopen(CSR_PATH, "rb");
    if (!fcsr) {
        ESP_LOGE(TAG, "Cannot open %s", CSR_PATH);
        return ASTARTE_ERR_NOT_FOUND;
    }

    int ret;
    if ((ret = fread(out, 1, length, fcsr)) < 0) {
        ESP_LOGE(TAG, "fread returned %d", ret);
        fclose(fcsr);
        return ASTARTE_ERR_IO;
    }

    fclose(fcsr);
    return ASTARTE_OK;
}

astarte_err_t astarte_credentials_get_certificate(char *out, size_t length)
{
    FILE *fcert = fopen(CRT_PATH, "rb");
    if (!fcert) {
        ESP_LOGE(TAG, "Cannot open %s", CRT_PATH);
        return ASTARTE_ERR_NOT_FOUND;
    }

    int ret;
    if ((ret = fread(out, 1, length, fcert)) < 0) {
        ESP_LOGE(TAG, "fread returned %d", ret);
        fclose(fcert);
        return ASTARTE_ERR_IO;
    }

    fclose(fcert);
    return ASTARTE_OK;
}

astarte_err_t astarte_credentials_get_certificate_common_name(
    const char *cert_pem, char *out, size_t length)
{
    astarte_err_t exit_code = ASTARTE_ERR_MBED_TLS;
    mbedtls_x509_crt crt;
    mbedtls_x509_crt_init(&crt);

    int ret;
    size_t cert_length = strlen(cert_pem) + 1; // + 1 for NULL terminator, as per documentation
    if ((ret = mbedtls_x509_crt_parse(&crt, (unsigned char *) cert_pem, cert_length)) < 0) {
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

    snprintf(out, length, "%.*s", subj_it->val.len, subj_it->val.p);
    exit_code = ASTARTE_OK;

exit:
    mbedtls_x509_crt_free(&crt);

    return exit_code;
}

astarte_err_t astarte_credentials_get_key(char *out, size_t length)
{
    FILE *fpriv = fopen(PRIVKEY_PATH, "rb");
    if (!fpriv) {
        ESP_LOGE(TAG, "Cannot open %s", CSR_PATH);
        return ASTARTE_ERR_NOT_FOUND;
    }

    int ret;
    if ((ret = fread(out, 1, length, fpriv)) < 0) {
        ESP_LOGE(TAG, "fread returned %d", ret);
        fclose(fpriv);
        return ASTARTE_ERR_IO;
    }

    fclose(fpriv);
    return ASTARTE_OK;
}

astarte_err_t astarte_credentials_get_stored_credentials_secret(char *out, size_t length)
{
    nvs_handle nvs;
    esp_err_t err = nvs_open(PAIRING_NAMESPACE, NVS_READONLY, &nvs);
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
    nvs_handle nvs;
    esp_err_t err = nvs_open(PAIRING_NAMESPACE, NVS_READWRITE, &nvs);
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
    nvs_handle nvs;
    esp_err_t err = nvs_open(PAIRING_NAMESPACE, NVS_READWRITE, &nvs);
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

int astarte_credentials_has_certificate()
{
    return access(CRT_PATH, R_OK) == 0;
}

int astarte_credentials_has_csr()
{
    return access(CSR_PATH, R_OK) == 0;
}

int astarte_credentials_has_key()
{
    return access(PRIVKEY_PATH, R_OK) == 0;
}

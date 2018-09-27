/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#include <astarte_credentials.h>

#include <esp_log.h>

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/x509_csr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define TAG "ASTARTE_CREDENTIALS"

#define CREDENTIALS_DIR_PATH "/spiflash/ast_cred"
#define PRIVKEY_PATH (CREDENTIALS_DIR_PATH "/device.key")
#define CSR_PATH (CREDENTIALS_DIR_PATH "/device.csr")
#define CRT_PATH (CREDENTIALS_DIR_PATH "/device.crt")

#define KEY_SIZE 2048
#define EXPONENT 65537
#define PRIVKEY_BUFFER_LENGTH 16000

#define CSR_BUFFER_LENGTH 4096

astarte_err_t astarte_credentials_init()
{
    struct stat st;
    if (stat(CREDENTIALS_DIR_PATH, &st) < 0) {
        ESP_LOGI(TAG, "Directory %s doesn't exist, creating it", CREDENTIALS_DIR_PATH);
        if (mkdir(CREDENTIALS_DIR_PATH, 0700) < 0) {
            ESP_LOGE(TAG, "Cannot create %s directory", CREDENTIALS_DIR_PATH);
            ESP_LOGE(TAG, "You have to mount a FAT fs on /spiflash");
            return ASTARTE_ERR;
        }
    }

    if (!astarte_credentials_has_key()) {
        ESP_LOGI(TAG, "Private key not found, creating it.");
        if (astarte_credentials_create_key() != ASTARTE_OK) {
            return ASTARTE_ERR;
        }
    }

    if (!astarte_credentials_has_csr()) {
        ESP_LOGI(TAG, "CSR not found, creating it.");
        if (astarte_credentials_create_csr() != ASTARTE_OK) {
            return ASTARTE_ERR;
        }
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_credentials_create_key()
{
    astarte_err_t exit_code = ASTARTE_ERR;

    mbedtls_pk_context key;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_mpi N, P, Q, D, E, DP, DQ, QP;
    FILE *fpriv = NULL;
    unsigned char *privkey_buffer = NULL;
    const char *pers = "astarte_credentials_create_key";

    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_pk_init(&key);
    mbedtls_mpi_init(&N);
    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&Q);
    mbedtls_mpi_init(&D);
    mbedtls_mpi_init(&E);
    mbedtls_mpi_init(&DP);
    mbedtls_mpi_init(&DQ);
    mbedtls_mpi_init(&QP);
    mbedtls_entropy_init(&entropy);

    int ret = 0;
    ESP_LOGI(TAG, "Initializing entropy");
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers))) != 0) {
        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
        goto exit;
    }

    ESP_LOGI(TAG, "Generating the RSA key [ %d-bit ]", KEY_SIZE);

    if ((ret = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA))) != 0) {
        ESP_LOGE(TAG, "mbedtls_pk_setup returned %d", ret);
        goto exit;
    }

    if ((ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(key), mbedtls_ctr_drbg_random, &ctr_drbg, KEY_SIZE, EXPONENT)) != 0) {
        ESP_LOGE(TAG, "mbedtls_rsa_gen_key returned %d", ret);
        goto exit;
    }

    mbedtls_rsa_context *rsa = mbedtls_pk_rsa(key);
    if ((ret = mbedtls_rsa_export(rsa, &N, &P, &Q, &D, &E)) != 0
            || (ret = mbedtls_rsa_export_crt(rsa, &DP, &DQ, &QP)) != 0 ) {
        ESP_LOGE(TAG, "Cannot export RSA parameters");
        goto exit;
    }

    ESP_LOGI(TAG, "Key succesfully generated");

    privkey_buffer = calloc(PRIVKEY_BUFFER_LENGTH, sizeof(unsigned char));
    if (!privkey_buffer) {
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
        ESP_LOGE(TAG, "Cannot open %s for writing", PRIVKEY_PATH);
        goto exit;
    }

    if (fwrite(privkey_buffer, sizeof(unsigned char), len, fpriv) != len) {
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

    mbedtls_mpi_free(&N);
    mbedtls_mpi_free(&P);
    mbedtls_mpi_free(&Q);
    mbedtls_mpi_free(&D);
    mbedtls_mpi_free(&E);
    mbedtls_mpi_free(&DP);
    mbedtls_mpi_free(&DQ);
    mbedtls_mpi_free(&QP);
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return exit_code;
}

astarte_err_t astarte_credentials_create_csr()
{
    astarte_err_t exit_code = ASTARTE_ERR;

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
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers))) != 0) {
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
        ESP_LOGE(TAG, "Cannot allocate CSR buffer");
        goto exit;
    }

    if ((ret = mbedtls_x509write_csr_pem(&req, csr_buffer, CSR_BUFFER_LENGTH, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
        ESP_LOGE(TAG, "mbedtls_x509write_csr_pem returned %d", ret);
        goto exit;
    }
    size_t len = strlen((char *) csr_buffer);

    ESP_LOGI(TAG, "Saving the CSR in %s", CSR_PATH);
    fcsr = fopen(CSR_PATH, "wb+");
    if (!fcsr) {
        ESP_LOGE(TAG, "Cannot open %s for writing", CSR_PATH);
        goto exit;
    }

    if (fwrite(csr_buffer, sizeof(unsigned char), len, fcsr) != len) {
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

    mbedtls_x509write_csr_free( &req );
    mbedtls_pk_free( &key );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

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
        return ASTARTE_ERR;
    }

    size_t len = strlen(cert_pem);
    int ret;
    if ((ret = fwrite(cert_pem, 1, len, fcert)) != len) {
        ESP_LOGE(TAG, "fwrite returned %d", ret);
        fclose(fcert);
        return ASTARTE_ERR;
    }

    fclose(fcert);
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
        return ASTARTE_ERR;
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
        return ASTARTE_ERR;
    }

    fclose(fcert);
    return ASTARTE_OK;
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
        return ASTARTE_ERR;
    }

    fclose(fpriv);
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

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define TAG "ASTARTE_CREDENTIALS"

#define CREDENTIALS_DIR_PATH "/spiflash/ast_cred"
#define PRIVKEY_PATH (CREDENTIALS_DIR_PATH "/device.key")
#define CSR_PATH (CREDENTIALS_DIR_PATH "/device.csr")

#define KEY_SIZE 2048
#define EXPONENT 65537
#define PRIVKEY_BUFFER_LENGTH 16000

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

    if (access(PRIVKEY_PATH, R_OK) < 0) {
        ESP_LOGI(TAG, "Private key not found, creating it.");
        if (astarte_credentials_create_key() != ASTARTE_OK) {
            return ASTARTE_ERR;
        }
    }

    if (access(CSR_PATH, R_OK) < 0) {
        ESP_LOGI(TAG, "CSR not found, creating it.");
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

    privkey_buffer = calloc(sizeof(unsigned char), PRIVKEY_BUFFER_LENGTH);
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

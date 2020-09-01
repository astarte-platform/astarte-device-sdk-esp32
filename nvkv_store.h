/*
 * (C) Copyright 2020, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

/**
 * @file astarte.h
 * @brief Astarte types and defines.
 */

#ifndef _NVKV_STORE_H_
#define _NVKV_STORE_H_

#include <stdint.h>

#define NVS_HANDLE_TYPE uint8_t *
//#define NVS_HANDLE_TYPE nvs_handle_t

/**
 * @brief Non-Volatile Key-Value Store return codes.
 *
 * @detail NVKV_STORE_OK is always returned when no errors occurred. Errors should be checked with != NVKV_STORE_OK
 */

enum nvkv_store_err_t
{
    NVKV_STORE_OK = 0,
    NVKV_STORE_ERROR = 1,
    NVKV_STORE_KEY_NOT_FOUND = 2,
    NVKV_STORE_OUT_OF_MEMORY = 3
};

typedef enum nvkv_store_err_t nvkv_store_err_t;

#ifdef __cplusplus
extern "C" {
#endif

nvkv_store_err_t nvkv_store_get_int8(NVS_HANDLE_TYPE handle, const char *group, const char *key, int8_t *out_value);
nvkv_store_err_t nvkv_store_get_int32(NVS_HANDLE_TYPE handle, const char *group, const char *key, int32_t *out_value);
nvkv_store_err_t nvkv_store_get_int64(NVS_HANDLE_TYPE handle, const char *group, const char *key, int64_t *out_value);
nvkv_store_err_t nvkv_store_get_double(NVS_HANDLE_TYPE handle, const char *group, const char *key, double *out_value);

#ifdef __cplusplus
}
#endif

#endif

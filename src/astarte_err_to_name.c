/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

#include <astarte.h>
#include <stdlib.h>

#define ERR_TBL_IT(err)                                                                            \
    {                                                                                              \
        err, #err                                                                                  \
    }

typedef struct
{
    astarte_err_t code;
    const char *msg;
} astarte_err_msg_t;

static const astarte_err_msg_t astarte_err_msg_table[] = {
    ERR_TBL_IT(ASTARTE_OK),
    ERR_TBL_IT(ASTARTE_ERR),
    ERR_TBL_IT(ASTARTE_ERR_NOT_FOUND),
    ERR_TBL_IT(ASTARTE_ERR_NO_JWT),
    ERR_TBL_IT(ASTARTE_ERR_OUT_OF_MEMORY),
    ERR_TBL_IT(ASTARTE_ERR_ESP_SDK),
    ERR_TBL_IT(ASTARTE_ERR_AUTH),
    ERR_TBL_IT(ASTARTE_ERR_ALREADY_EXISTS),
    ERR_TBL_IT(ASTARTE_ERR_API),
    ERR_TBL_IT(ASTARTE_ERR_HTTP),
    ERR_TBL_IT(ASTARTE_ERR_NVS),
    ERR_TBL_IT(ASTARTE_ERR_NVS_NOT_INITIALIZED),
    ERR_TBL_IT(ASTARTE_ERR_PARTITION_SCHEME),
    ERR_TBL_IT(ASTARTE_ERR_MBED_TLS),
    ERR_TBL_IT(ASTARTE_ERR_IO),
    ERR_TBL_IT(ASTARTE_ERR_INVALID_INTERFACE_PATH),
    ERR_TBL_IT(ASTARTE_ERR_INVALID_QOS),
    ERR_TBL_IT(ASTARTE_ERR_DEVICE_NOT_READY),
    ERR_TBL_IT(ASTARTE_ERR_PUBLISH),
    ERR_TBL_IT(ASTARTE_ERR_INVALID_INTROSPECTION),
    ERR_TBL_IT(ASTARTE_ERR_INVALID_INTERFACE_VERSION),
    ERR_TBL_IT(ASTARTE_ERR_CONFLICTING_INTERFACE),
};

static const char astarte_unknown_msg[] = "ERROR";

const char *astarte_err_to_name(astarte_err_t code)
{
    for (size_t i = 0; i < sizeof(astarte_err_msg_table) / sizeof(astarte_err_msg_table[0]); ++i) {
        if (astarte_err_msg_table[i].code == code) {
            return astarte_err_msg_table[i].msg;
        }
    }

    return astarte_unknown_msg;
}

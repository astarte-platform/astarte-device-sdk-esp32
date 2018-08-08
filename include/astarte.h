/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

/**
 * @file astarte.h
 * @brief Astarte types and defines.
 */

#ifndef _ASTARTE_H_
#define _ASTARTE_H_

/**
 * @brief Astarte return codes.
 *
 * @detail Astarte Device SDK return codes. ASTARTE_OK is always returned when no errors occurred.
 */
enum astarte_err_t
{
    ASTARTE_OK = 0, /**< No errors. */
    ASTARTE_ERR = 1 /**< A generic error occurred. */
};

typedef enum astarte_err_t astarte_err_t;

#endif

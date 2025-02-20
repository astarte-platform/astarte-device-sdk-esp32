/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ASTARTE_DEVICE_SDK_UTIL_H_
#define _ASTARTE_DEVICE_SDK_UTIL_H_

/**
 * @file util.h
 * @brief Utility macros and functions.
 */

/**
 * @defgroup utils Utils
 * @brief Utility macros and functions.
 * @ingroup astarte_device_sdk
 * @{
 */

/**
 * @brief Define a struct that contains a pointer of type TYPE and a length
 * @param[in] NAME name of the type
 * @param[in] TYPE type of an element of the array.
 */
// NOLINTBEGIN(bugprone-macro-parentheses) TYPE parameter can't be enclosed in parenthesis.
#define ASTARTE_UTIL_DEFINE_ARRAY(NAME, TYPE)                                                      \
    typedef struct                                                                                 \
    {                                                                                              \
        TYPE *buf;                                                                                 \
        size_t len;                                                                                \
    } NAME;
// NOLINTEND(bugprone-macro-parentheses)

/**
 * @}
 */
#endif /* _ASTARTE_DEVICE_SDK_UTIL_H_ */

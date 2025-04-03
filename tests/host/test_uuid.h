/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_UUID_H
#define TEST_UUID_H

#ifdef __cplusplus
extern "C" {
#endif

void test_uuid_generate_v4(void);
void test_uuid_generate_v5(void);
void test_uuid_from_string(void);
void test_uuid_to_string(void);
void test_uuid_to_base64(void);
void test_uuid_to_base64url(void);
void test_uuid_from_string_errors(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_UUID_H

/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_BSON_DESERIALIZER_H
#define TEST_BSON_DESERIALIZER_H

#ifdef __cplusplus
extern "C" {
#endif

void test_bson_deserializer_check_validity(void);
void test_bson_deserializer_empty_bson_document(void);
void test_bson_deserializer_complete_bson_document(void);
void test_bson_deserializer_bson_document_lookup(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_BSON_DESERIALIZER_H

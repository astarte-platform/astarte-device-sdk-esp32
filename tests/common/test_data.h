/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_DATA_H
#define TEST_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

void test_data_serialize_integer(void);
void test_data_serialize_longinteger(void);
void test_data_serialize_double(void);
void test_data_serialize_boolean(void);
void test_data_serialize_string(void);
void test_data_serialize_integer_array(void);
void test_data_serialize_string_array(void);
void test_data_serialize_binaryblob_array(void);

void test_data_deserialize_astarte_data_from_incorrect_type(void);
void test_data_deserialize_astarte_data_from_binblob(void);
void test_data_deserialize_astarte_data_from_boolean(void);
void test_data_deserialize_astarte_data_from_datetime(void);
void test_data_deserialize_astarte_data_from_double(void);
void test_data_deserialize_astarte_data_from_integer(void);
void test_data_deserialize_astarte_data_from_longinteger(void);
void test_data_deserialize_astarte_data_from_string(void);
void test_data_deserialize_astarte_data_from_binblob_array(void);
void test_data_deserialize_astarte_data_from_boolean_array(void);
void test_data_deserialize_astarte_data_from_double_array(void);
void test_data_deserialize_astarte_data_from_datetime_array(void);
void test_data_deserialize_astarte_data_from_integer_array(void);
void test_data_deserialize_astarte_data_from_longinteger_array(void);
void test_data_deserialize_astarte_data_from_string_array(void);
void test_data_deserialize_astarte_data_from_empty_array(void);
void test_data_deserialize_astarte_data_from_mismatched_array_initial(void);
void test_data_deserialize_astarte_data_from_mismatched_array_final(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_DATA_H

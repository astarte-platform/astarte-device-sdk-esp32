/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_DATA_VALIDATION_H
#define TEST_DATA_VALIDATION_H

#ifdef __cplusplus
extern "C" {
#endif

void test_data_validation_individual_datastream_ok(void);
void test_data_validation_individual_datastream_incorrect_path(void);
void test_data_validation_individual_datastream_incorrect_data(void);
void test_data_validation_individual_datastream_incorrect_timestamp_required(void);
void test_data_validation_individual_datastream_incorrect_timestamp_not_allowed(void);

void test_data_validation_aggregated_datastream_ok(void);
void test_data_validation_aggregated_datastream_incorrect_path(void);
void test_data_validation_aggregated_datastream_incorrect_data(void);
void test_data_validation_aggregated_datastream_incorrect_timestamp_required(void);
void test_data_validation_aggregated_datastream_timestamp_not_allowed(void);

void test_data_validation_unset_properties_ok(void);
void test_data_validation_unset_properties_incorrect_path(void);
void test_data_validation_unset_properties_not_allowed(void);

#ifdef __cplusplus
}
#endif
#endif // TEST_DATA_VALIDATION_H

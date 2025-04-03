/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_MAPPING_H
#define TEST_MAPPING_H

#ifdef __cplusplus
extern "C" {
#endif

void test_mapping_check_path_one_segment_no_pattern(void);
void test_astarte_mapping_check_path_multiple_segments_no_pattern(void);
void test_astarte_mapping_check_path_one_segment_single_pattern(void);
void test_astarte_mapping_check_path_multiple_segments_single_pattern(void);
void test_astarte_mapping_check_path_multiple_segments_three_patterns(void);
void test_astarte_mapping_check_data_double(void);
void test_astarte_mapping_check_data_doublearray(void);

#ifdef __cplusplus
}
#endif
#endif // TEST_MAPPING_H

#ifndef _TEST_UUID_H_
#define _TEST_UUID_H_

#ifdef __cplusplus
extern "C" {
#endif

void test_uuid_from_string(void);
void test_uuid_to_string(void);
void test_uuid_generate_v4(void);
void test_uuid_generate_v5(void);

#ifdef __cplusplus
}
#endif

#endif // _TEST_UUID_H_

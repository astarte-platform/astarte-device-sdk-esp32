#ifndef _TEST_ASTARTE_BSON_DESERIALIZER_H_
#define _TEST_ASTARTE_BSON_DESERIALIZER_H_

#ifdef __cplusplus
extern "C" {
#endif

void test_astarte_bson_deserializer_check_validity(void);
void test_astarte_bson_deserializer_empty_bson_document(void);
void test_astarte_bson_deserializer_complete_bson_document(void);
void test_astarte_bson_deserializer_bson_document_lookup(void);

#ifdef __cplusplus
}
#endif

#endif // _TEST_ASTARTE_BSON_DESERIALIZER_H_

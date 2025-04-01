/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ASTARTE_DEVICE_SDK_DATA_H
#define ASTARTE_DEVICE_SDK_DATA_H

/**
 * @file data.h
 * @brief Definitions for Astarte base data type
 */

/**
 * @defgroup data Base data
 * @brief Definitions for Astarte base data types.
 * @ingroup astarte_device_sdk
 * @{
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/mapping.h"
#include "astarte_device_sdk/result.h"
#include "astarte_device_sdk/util.h"

/** @private Astarte binary blob type. Do not inspect its content directly. */
ASTARTE_UTIL_DEFINE_ARRAY(astarte_data_binaryblob_t, void);

/** @private Astarte binary blob array type. Do not inspect its content directly. */
typedef struct
{
    /** @brief Array of binary blobs */
    const void **blobs;
    /** @brief Array of sizes of each binary blob */
    size_t *sizes;
    /** @brief Number of elements in both the array of binary blobs and array of sizes */
    size_t count;
} astarte_data_binaryblobarray_t;

/** @private Astarte bool array type. Do not inspect its content directly. */
ASTARTE_UTIL_DEFINE_ARRAY(astarte_data_booleanarray_t, bool);
/** @private Astarte double array type. Do not inspect its content directly. */
ASTARTE_UTIL_DEFINE_ARRAY(astarte_data_doublearray_t, double);
/** @private Astarte integer array type. Do not inspect its content directly. */
ASTARTE_UTIL_DEFINE_ARRAY(astarte_data_integerarray_t, int32_t);
/** @private Astarte long integer array type. Do not inspect its content directly. */
ASTARTE_UTIL_DEFINE_ARRAY(astarte_data_longintegerarray_t, int64_t);
/** @private Astarte string array type. Do not inspect its content directly. */
ASTARTE_UTIL_DEFINE_ARRAY(astarte_data_stringarray_t, const char *);

/** @private Union grouping all the Astarte data types. Do not inspect its content directly. */
typedef union
{
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_BINARYBLOB */
    astarte_data_binaryblob_t binaryblob;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_BOOLEAN */
    bool boolean;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_DATETIME */
    int64_t datetime;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_DOUBLE */
    double dbl;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_INTEGER */
    int32_t integer;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_LONGINTEGER */
    int64_t longinteger;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_STRING */
    const char *string;

    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY */
    astarte_data_binaryblobarray_t binaryblob_array;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_BOOLEANARRAY */
    astarte_data_booleanarray_t boolean_array;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_DATETIMEARRAY */
    astarte_data_longintegerarray_t datetime_array;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_DOUBLEARRAY */
    astarte_data_doublearray_t double_array;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_INTEGERARRAY */
    astarte_data_integerarray_t integer_array;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY */
    astarte_data_longintegerarray_t longinteger_array;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_STRINGARRAY */
    astarte_data_stringarray_t string_array;
} astarte_data_param_t;

/**
 * @brief Base Astarte data type.
 *
 * @warning Do not inspect its content directly. Use the provided functions to modify it.
 * @details This struct is a tagged enum and represents every possible type that can be sent to
 * Astarte individually. This struct should be initialized only by using one of the defined
 * `astarte_data_from_*` methods.
 *
 * See the following initialization methods:
 *  - #astarte_data_from_binaryblob
 *  - #astarte_data_from_boolean
 *  - #astarte_data_from_double
 *  - #astarte_data_from_datetime
 *  - #astarte_data_from_integer
 *  - #astarte_data_from_longinteger
 *  - #astarte_data_from_string
 *  - #astarte_data_from_binaryblob_array
 *  - #astarte_data_from_boolean_array
 *  - #astarte_data_from_datetime_array
 *  - #astarte_data_from_double_array
 *  - #astarte_data_from_integer_array
 *  - #astarte_data_from_longinteger_array
 *  - #astarte_data_from_string_array
 *
 * And the following getter methods:
 *  - #astarte_data_to_binaryblob
 *  - #astarte_data_to_boolean
 *  - #astarte_data_to_double
 *  - #astarte_data_to_datetime
 *  - #astarte_data_to_integer
 *  - #astarte_data_to_longinteger
 *  - #astarte_data_to_string
 *  - #astarte_data_to_binaryblob_array
 *  - #astarte_data_to_boolean_array
 *  - #astarte_data_to_datetime_array
 *  - #astarte_data_to_double_array
 *  - #astarte_data_to_integer_array
 *  - #astarte_data_to_longinteger_array
 *  - #astarte_data_to_string_array
 */
typedef struct
{
    /** @brief Data portion of the tagged enum */
    astarte_data_param_t data;
    /** @brief Tag of the tagged enum */
    astarte_mapping_type_t tag;
} astarte_data_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize an astarte data from the input binaryblob.
 *
 * @param[in] binaryblob Input type that will be inserted in the #astarte_data_t.
 * @param[in] len The length of the input array.
 * @return The astarte data that wraps the binaryblob input.
 */
astarte_data_t astarte_data_from_binaryblob(void *binaryblob, size_t len);
/**
 * @brief Initialize an astarte data from the input boolean.
 *
 * @param[in] boolean Input type that will be inserted in the #astarte_data_t.
 * @return The astarte data that wraps the boolean input.
 */
astarte_data_t astarte_data_from_boolean(bool boolean);
/**
 * @brief Initialize an astarte data from the input datetime.
 *
 * @param[in] datetime Input type that will be inserted in the #astarte_data_t.
 * @return The astarte data that wraps the datetime input.
 */
astarte_data_t astarte_data_from_datetime(int64_t datetime);
/**
 * @brief Initialize an astarte data from the input double.
 *
 * @param[in] dbl Input type that will be inserted in the #astarte_data_t.
 * @return The astarte data that wraps the double input.
 */
astarte_data_t astarte_data_from_double(double dbl);
/**
 * @brief Initialize an astarte data from the input integer.
 *
 * @param[in] integer Input type that will be inserted in the #astarte_data_t.
 * @return The astarte data that wraps the integer input.
 */
astarte_data_t astarte_data_from_integer(int32_t integer);
/**
 * @brief Initialize an astarte data from the input longinteger.
 *
 * @param[in] longinteger Input type that will be inserted in the #astarte_data_t.
 * @return The astarte data that wraps the longinteger input.
 */
astarte_data_t astarte_data_from_longinteger(int64_t longinteger);
/**
 * @brief Initialize an astarte data from the input string.
 *
 * @param[in] string Input type that will be inserted in the #astarte_data_t.
 * @return The astarte data that wraps the string input.
 */
astarte_data_t astarte_data_from_string(const char *string);

/**
 * @brief Initialize an astarte data from the input binaryblob array.
 *
 * @param[in] blobs A bi-dimensional array, the size of each array is contained in @p sizes while
 * the number of arrays is contained in @p count
 * @param[in] sizes Array of sizes, each entry will be the size of the corresponding array contained
 * in @p blobs
 * @param[in] count Number of arrays contained in @p blobs
 * @return The astarte data that wraps the binaryblob array input.
 */
astarte_data_t astarte_data_from_binaryblob_array(const void **blobs, size_t *sizes, size_t count);
/**
 * @brief Initialize an astarte data from the input boolean array.
 *
 * @param[in] boolean_array Input type that will be inserted in the #astarte_data_t.
 * @param[in] len The length of the input array.
 * @return The astarte data that wraps the boolean array input.
 */
astarte_data_t astarte_data_from_boolean_array(bool *boolean_array, size_t len);
/**
 * @brief Initialize an astarte data from the input datetime array.
 *
 * @param[in] datetime_array Input type that will be inserted in the #astarte_data_t.
 * @param[in] len The length of the input array.
 * @return The astarte data that wraps the datetime array input.
 */
astarte_data_t astarte_data_from_datetime_array(int64_t *datetime_array, size_t len);
/**
 * @brief Initialize an astarte data from the input double array.
 *
 * @param[in] double_array Input type that will be inserted in the #astarte_data_t.
 * @param[in] len The length of the input array.
 * @return The astarte data that wraps the double array input.
 */
astarte_data_t astarte_data_from_double_array(double *double_array, size_t len);
/**
 * @brief Initialize an astarte data from the input integer array.
 *
 * @param[in] integer_array Input type that will be inserted in the #astarte_data_t.
 * @param[in] len The length of the input array.
 * @return The astarte data that wraps the integer array input.
 */
astarte_data_t astarte_data_from_integer_array(int32_t *integer_array, size_t len);
/**
 * @brief Initialize an astarte data from the input longinteger array.
 *
 * @param[in] longinteger_array Input type that will be inserted in the #astarte_data_t.
 * @param[in] len The length of the input array.
 * @return The astarte data that wraps the longinteger array input.
 */
astarte_data_t astarte_data_from_longinteger_array(int64_t *longinteger_array, size_t len);
/**
 * @brief Initialize an astarte data from the input string array.
 *
 * @param[in] string_array Input type that will be inserted in the #astarte_data_t.
 * @param[in] len The length of the input array.
 * @return The astarte data that wraps the string array input.
 */
astarte_data_t astarte_data_from_string_array(const char **string_array, size_t len);

/**
 * @brief Get the type of Astarte data.
 *
 * @param[in] data Astarte data for which to check the type.
 * @return The astarte data type.
 */
astarte_mapping_type_t astarte_data_get_type(astarte_data_t data);

/**
 * @brief Convert Astarte data (of the binary blob type) to an array of binary blobs.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] binaryblob Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_binaryblob(astarte_data_t data, void **binaryblob, size_t *len);
/**
 * @brief Convert Astarte data (of the boolean type) to a bool.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] boolean Extracted data.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_boolean(astarte_data_t data, bool *boolean);
/**
 * @brief Convert Astarte data (of the datetime type) to an int64_t.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] datetime Extracted data.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_datetime(astarte_data_t data, int64_t *datetime);
/**
 * @brief Convert Astarte data (of double type) to a double.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] dbl Double where to store the extracted data.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_double(astarte_data_t data, double *dbl);
/**
 * @brief Convert Astarte data (of the integer type) to an int32_t.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] integer Extracted data.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_integer(astarte_data_t data, int32_t *integer);
/**
 * @brief Convert Astarte data (of the long integer type) to an int64_t.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] longinteger Extracted data.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_longinteger(astarte_data_t data, int64_t *longinteger);
/**
 * @brief Convert Astarte data (of the string type) to a const char *.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] string Extracted data.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_string(astarte_data_t data, const char **string);

/**
 * @brief Convert Astarte data (of binaryblob array type) to an array of binaryblob arrays.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] blobs Bi-dimensional array of binary blobs.
 * @param[out] sizes Array of sizes (second dimension) of @p blobs
 * @param[out] count Number or arrays (first dimension) of @p blobs
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_binaryblob_array(
    astarte_data_t data, const void ***blobs, size_t **sizes, size_t *count);
/**
 * @brief Convert Astarte data (of boolean array type) to a bool array.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] boolean_array Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_boolean_array(
    astarte_data_t data, bool **boolean_array, size_t *len);
/**
 * @brief Convert Astarte data (of datetime array type) to an int64_t array.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] datetime_array Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_datetime_array(
    astarte_data_t data, int64_t **datetime_array, size_t *len);
/**
 * @brief Convert Astarte data (of double array type) to a double array.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] double_array Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_double_array(
    astarte_data_t data, double **double_array, size_t *len);
/**
 * @brief Convert Astarte data (of integer array type) to an int32_t array.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] integer_array Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_integer_array(
    astarte_data_t data, int32_t **integer_array, size_t *len);
/**
 * @brief Convert Astarte data (of longinteger array type) to an int64_t array.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] longinteger_array Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_longinteger_array(
    astarte_data_t data, int64_t **longinteger_array, size_t *len);
/**
 * @brief Convert Astarte data (of string type) to a const char* array.
 *
 * @param[in] data Astarte data to use for the extraction.
 * @param[out] string_array Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_data_to_string_array(
    astarte_data_t data, const char ***string_array, size_t *len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // ASTARTE_DEVICE_SDK_DATA_H

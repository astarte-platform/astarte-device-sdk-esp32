/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ASTARTE_DEVICE_SDK_INDIVIDUAL_H_
#define _ASTARTE_DEVICE_SDK_INDIVIDUAL_H_

/**
 * @file individual.h
 * @brief Definitions for Astarte individual data types
 */

/**
 * @defgroup individuals Individuals
 * @brief Definitions for Astarte individual data types.
 * @ingroup astarte_device_sdk
 * @{
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/mapping.h"
#include "astarte_device_sdk/result.h"
#include "astarte_device_sdk/util.h"

/** @private Astarte binary blob type. Internal use, do not inspect its content directly. */
ASTARTE_UTIL_DEFINE_ARRAY(astarte_individual_binaryblob_t, void);

/** @private Astarte binary blob array type. Internal use, do not inspect its content directly. */
typedef struct
{
    /** @brief Array of binary blobs */
    const void **blobs;
    /** @brief Array of sizes of each binary blob */
    size_t *sizes;
    /** @brief Number of elements in both the array of binary blobs and array of sizes */
    size_t count;
} astarte_individual_binaryblobarray_t;

/** @private Astarte bool array type. Internal use, do not inspect its content directly. */
ASTARTE_UTIL_DEFINE_ARRAY(astarte_individual_booleanarray_t, bool);
/** @private Astarte double array type. Internal use, do not inspect its content directly. */
ASTARTE_UTIL_DEFINE_ARRAY(astarte_individual_doublearray_t, double);
/** @private Astarte integer array type. Internal use, do not inspect its content directly. */
ASTARTE_UTIL_DEFINE_ARRAY(astarte_individual_integerarray_t, int32_t);
/** @private Astarte long integer array type. Internal use, do not inspect its content directly. */
ASTARTE_UTIL_DEFINE_ARRAY(astarte_individual_longintegerarray_t, int64_t);
/** @private Astarte string array type. Internal use, do not inspect its content directly. */
ASTARTE_UTIL_DEFINE_ARRAY(astarte_individual_stringarray_t, const char *);

/** @private Union grouping all the individual Astarte types.
 * Internal use, do not inspect its content directly. */
typedef union
{
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_BINARYBLOB */
    astarte_individual_binaryblob_t binaryblob;
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
    astarte_individual_binaryblobarray_t binaryblob_array;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_BOOLEANARRAY */
    astarte_individual_booleanarray_t boolean_array;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_DATETIMEARRAY */
    astarte_individual_longintegerarray_t datetime_array;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_DOUBLEARRAY */
    astarte_individual_doublearray_t double_array;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_INTEGERARRAY */
    astarte_individual_integerarray_t integer_array;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY */
    astarte_individual_longintegerarray_t longinteger_array;
    /** @brief Union variant associated with tag #ASTARTE_MAPPING_TYPE_STRINGARRAY */
    astarte_individual_stringarray_t string_array;
} astarte_individual_param_t;

/**
 * @brief Base Astarte individual data type.
 *
 * @warning Do not inspect its content directly. Use the provided functions to modify it.
 * @details This struct is a tagged enum and represents every possible type that can be sent to
 * Astarte individually. This struct should be initialized only by using one of the defined
 * `astarte_individual_from_*` methods.
 *
 * See the following initialization methods:
 *  - #astarte_individual_from_binaryblob
 *  - #astarte_individual_from_boolean
 *  - #astarte_individual_from_double
 *  - #astarte_individual_from_datetime
 *  - #astarte_individual_from_integer
 *  - #astarte_individual_from_longinteger
 *  - #astarte_individual_from_string
 *  - #astarte_individual_from_binaryblob_array
 *  - #astarte_individual_from_boolean_array
 *  - #astarte_individual_from_datetime_array
 *  - #astarte_individual_from_double_array
 *  - #astarte_individual_from_integer_array
 *  - #astarte_individual_from_longinteger_array
 *  - #astarte_individual_from_string_array
 *
 * See the following getter methods:
 *  - #astarte_individual_to_binaryblob
 *  - #astarte_individual_to_boolean
 *  - #astarte_individual_to_double
 *  - #astarte_individual_to_datetime
 *  - #astarte_individual_to_integer
 *  - #astarte_individual_to_longinteger
 *  - #astarte_individual_to_string
 *  - #astarte_individual_to_binaryblob_array
 *  - #astarte_individual_to_boolean_array
 *  - #astarte_individual_to_datetime_array
 *  - #astarte_individual_to_double_array
 *  - #astarte_individual_to_integer_array
 *  - #astarte_individual_to_longinteger_array
 *  - #astarte_individual_to_string_array
 */
typedef struct
{
    /** @brief Data portion of the tagged enum */
    astarte_individual_param_t data;
    /** @brief Tag of the tagged enum */
    astarte_mapping_type_t tag;
} astarte_individual_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize an astarte individual from the input binaryblob.
 *
 * @param[in] binaryblob Input type that will be inserted in the #astarte_individual_t.
 * @param[in] len The length of the input array.
 * @return The astarte individual that wraps the binaryblob input.
 */
astarte_individual_t astarte_individual_from_binaryblob(void *binaryblob, size_t len);
/**
 * @brief Initialize an astarte individual from the input boolean.
 *
 * @param[in] boolean Input type that will be inserted in the #astarte_individual_t.
 * @return The astarte individual that wraps the boolean input.
 */
astarte_individual_t astarte_individual_from_boolean(bool boolean);
/**
 * @brief Initialize an astarte individual from the input datetime.
 *
 * @param[in] datetime Input type that will be inserted in the #astarte_individual_t.
 * @return The astarte individual that wraps the datetime input.
 */
astarte_individual_t astarte_individual_from_datetime(int64_t datetime);
/**
 * @brief Initialize an astarte individual from the input double.
 *
 * @param[in] dbl Input type that will be inserted in the #astarte_individual_t.
 * @return The astarte individual that wraps the double input.
 */
astarte_individual_t astarte_individual_from_double(double dbl);
/**
 * @brief Initialize an astarte individual from the input integer.
 *
 * @param[in] integer Input type that will be inserted in the #astarte_individual_t.
 * @return The astarte individual that wraps the integer input.
 */
astarte_individual_t astarte_individual_from_integer(int32_t integer);
/**
 * @brief Initialize an astarte individual from the input longinteger.
 *
 * @param[in] longinteger Input type that will be inserted in the #astarte_individual_t.
 * @return The astarte individual that wraps the longinteger input.
 */
astarte_individual_t astarte_individual_from_longinteger(int64_t longinteger);
/**
 * @brief Initialize an astarte individual from the input string.
 *
 * @param[in] string Input type that will be inserted in the #astarte_individual_t.
 * @return The astarte individual that wraps the string input.
 */
astarte_individual_t astarte_individual_from_string(const char *string);

/**
 * @brief Initialize an astarte individual from the input binaryblob array.
 *
 * @param[in] blobs A bi-dimensional array, the size of each array is contained in @p sizes while
 * the number of arrays is contained in @p count
 * @param[in] sizes Array of sizes, each entry will be the size of the corresponding array contained
 * in @p blobs
 * @param[in] count Number of arrays contained in @p blobs
 * @return The astarte individual that wraps the binaryblob array input.
 */
astarte_individual_t astarte_individual_from_binaryblob_array(
    const void **blobs, size_t *sizes, size_t count);
/**
 * @brief Initialize an astarte individual from the input boolean array.
 *
 * @param[in] boolean_array Input type that will be inserted in the #astarte_individual_t.
 * @param[in] len The length of the input array.
 * @return The astarte individual that wraps the boolean array input.
 */
astarte_individual_t astarte_individual_from_boolean_array(bool *boolean_array, size_t len);
/**
 * @brief Initialize an astarte individual from the input datetime array.
 *
 * @param[in] datetime_array Input type that will be inserted in the #astarte_individual_t.
 * @param[in] len The length of the input array.
 * @return The astarte individual that wraps the datetime array input.
 */
astarte_individual_t astarte_individual_from_datetime_array(int64_t *datetime_array, size_t len);
/**
 * @brief Initialize an astarte individual from the input double array.
 *
 * @param[in] double_array Input type that will be inserted in the #astarte_individual_t.
 * @param[in] len The length of the input array.
 * @return The astarte individual that wraps the double array input.
 */
astarte_individual_t astarte_individual_from_double_array(double *double_array, size_t len);
/**
 * @brief Initialize an astarte individual from the input integer array.
 *
 * @param[in] integer_array Input type that will be inserted in the #astarte_individual_t.
 * @param[in] len The length of the input array.
 * @return The astarte individual that wraps the integer array input.
 */
astarte_individual_t astarte_individual_from_integer_array(int32_t *integer_array, size_t len);
/**
 * @brief Initialize an astarte individual from the input longinteger array.
 *
 * @param[in] longinteger_array Input type that will be inserted in the #astarte_individual_t.
 * @param[in] len The length of the input array.
 * @return The astarte individual that wraps the longinteger array input.
 */
astarte_individual_t astarte_individual_from_longinteger_array(
    int64_t *longinteger_array, size_t len);
/**
 * @brief Initialize an astarte individual from the input string array.
 *
 * @param[in] string_array Input type that will be inserted in the #astarte_individual_t.
 * @param[in] len The length of the input array.
 * @return The astarte individual that wraps the string array input.
 */
astarte_individual_t astarte_individual_from_string_array(const char **string_array, size_t len);

/**
 * @brief Get the type of an Astarte individual.
 *
 * @param[in] individual Astarte individual for which to check the type.
 * @return The astarte individual type.
 */
astarte_mapping_type_t astarte_individual_get_type(astarte_individual_t individual);

/**
 * @brief Convert an Astarte individual (of the binary blob type) to an array of binary blobs.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] binaryblob Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_binaryblob(
    astarte_individual_t individual, void **binaryblob, size_t *len);
/**
 * @brief Convert an Astarte individual (of the boolean type) to a bool.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] boolean Extracted individual.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_boolean(astarte_individual_t individual, bool *boolean);
/**
 * @brief Convert an Astarte individual (of the datetime type) to an int64_t.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] datetime Extracted individual.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_datetime(astarte_individual_t individual, int64_t *datetime);
/**
 * @brief Convert an Astarte individual (of double type) to a double.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] dbl Double where to store the extracted individual.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_double(astarte_individual_t individual, double *dbl);
/**
 * @brief Convert an Astarte individual (of the integer type) to an int32_t.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] integer Extracted individual.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_integer(astarte_individual_t individual, int32_t *integer);
/**
 * @brief Convert an Astarte individual (of the long integer type) to an int64_t.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] longinteger Extracted individual.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_longinteger(
    astarte_individual_t individual, int64_t *longinteger);
/**
 * @brief Convert an Astarte individual (of the string type) to a const char *.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] string Extracted individual.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_string(astarte_individual_t individual, const char **string);

/**
 * @brief Convert an Astarte individual (of binaryblob array type) to an array of binaryblob arrays.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] blobs Bi-dimensional array of binary blobs.
 * @param[out] sizes Array of sizes (second dimension) of @p blobs
 * @param[out] count Number or arrays (first dimension) of @p blobs
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_binaryblob_array(
    astarte_individual_t individual, const void ***blobs, size_t **sizes, size_t *count);
/**
 * @brief Convert an Astarte individual (of boolean array type) to a bool array.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] boolean_array Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_boolean_array(
    astarte_individual_t individual, bool **boolean_array, size_t *len);
/**
 * @brief Convert an Astarte individual (of datetime array type) to an int64_t array.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] datetime_array Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_datetime_array(
    astarte_individual_t individual, int64_t **datetime_array, size_t *len);
/**
 * @brief Convert an Astarte individual (of double array type) to a double array.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] double_array Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_double_array(
    astarte_individual_t individual, double **double_array, size_t *len);
/**
 * @brief Convert an Astarte individual (of integer array type) to an int32_t array.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] integer_array Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_integer_array(
    astarte_individual_t individual, int32_t **integer_array, size_t *len);
/**
 * @brief Convert an Astarte individual (of longinteger array type) to an int64_t array.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] longinteger_array Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_longinteger_array(
    astarte_individual_t individual, int64_t **longinteger_array, size_t *len);
/**
 * @brief Convert an Astarte individual (of string type) to a const char* array.
 *
 * @param[in] individual Astarte individual to use for the extraction.
 * @param[out] string_array Array extracted.
 * @param[out] len Size of extracted array.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_individual_to_string_array(
    astarte_individual_t individual, const char ***string_array, size_t *len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _ASTARTE_DEVICE_SDK_INDIVIDUAL_H_ */

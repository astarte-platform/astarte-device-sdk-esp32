/*
 * (C) Copyright 2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

/**
 * @file astarte_zlib.h
 * @brief Soft wrappers for zlib functions.
 */

#ifndef _ASTARTE_ZLIB_H_
#define _ASTARTE_ZLIB_H_

#include <zlib.h>

/**
 * @brief Function equivalent to the `compress` function defined in zlib.h.
 *
 * @details The implementation is copied 1 to 1 from zlib, with a single difference.
 * `defalteInit` has been replaced by `deflateInit2`. This allows to reduce the memory usage of zlib
 * by setting custom values for the `windowBits` and `memLevel` parameters.
 * This could also have been done by changing the values of the defines `MAX_WBITS` and
 * `DEF_MEM_LEVEL` in zconf.h. However this would have required to change the source of a idf
 * component and has been avoided.
 *
 * @note Since this function is almost a copy and paste from zlib.c, it will not be linted as every
 * other function in this library.
 *
 * @param dest See docstrings for `compress` in zlib.h
 * @param destLen See docstrings for `compress` in zlib.h
 * @param source See docstrings for `compress` in zlib.h
 * @param sourceLen See docstrings for `compress` in zlib.h
 * @return See docstrings for `compress` in zlib.h
 */
int ZEXPORT astarte_zlib_compress(
    Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);

#endif /* _ASTARTE_ZLIB_H_ */

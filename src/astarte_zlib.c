/*
 * (C) Copyright 2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */
#include "astarte_zlib.h"

/************************************************
 *         Global functions definitions         *
 ***********************************************/

// NOLINTBEGIN: This function is pretty much identical to the one contained in zlib.
int ZEXPORT astarte_zlib_compress(
    Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
    z_stream stream;
    int err;
    const uInt max = (uInt) -1;
    uLong left;

    left = *destLen;
    *destLen = 0;

    stream.zalloc = (alloc_func) 0;
    stream.zfree = (free_func) 0;
    stream.opaque = (voidpf) 0;

    int windowBits = 9; // Smallest possible window
    int memLevel = 1; // Smallest possible memory usage
    err = deflateInit2(
        &stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits, memLevel, Z_DEFAULT_STRATEGY);
    if (err != Z_OK)
        return err;

    stream.next_out = dest;
    stream.avail_out = 0;
    stream.next_in = (z_const Bytef *) source;
    stream.avail_in = 0;

    do {
        if (stream.avail_out == 0) {
            stream.avail_out = left > (uLong) max ? max : (uInt) left;
            left -= stream.avail_out;
        }
        if (stream.avail_in == 0) {
            stream.avail_in = sourceLen > (uLong) max ? max : (uInt) sourceLen;
            sourceLen -= stream.avail_in;
        }
        err = deflate(&stream, sourceLen ? Z_NO_FLUSH : Z_FINISH);
    } while (err == Z_OK);

    *destLen = stream.total_out;
    deflateEnd(&stream);
    return err == Z_STREAM_END ? Z_OK : err;
}
// NOLINTEND

/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

/**
 * @file astarte_bson_types.h
 * @brief Astarte BSON types definitions.
 */

#ifndef _ASTARTE_BSON_TYPES_H_
#define _ASTARTE_BSON_TYPES_H_

// clang-format off

#define BSON_TYPE_DOUBLE    '\x01'
#define BSON_TYPE_STRING    '\x02'
#define BSON_TYPE_DOCUMENT  '\x03'
#define BSON_TYPE_ARRAY     '\x04'
#define BSON_TYPE_BINARY    '\x05'
#define BSON_TYPE_BOOLEAN   '\x08'
#define BSON_TYPE_DATETIME  '\x09'
#define BSON_TYPE_INT32     '\x10'
#define BSON_TYPE_INT64     '\x12'

#define BSON_SUBTYPE_DEFAULT_BINARY '\0'

// clang-format on

#endif

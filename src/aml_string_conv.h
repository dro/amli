#pragma once

#include "aml_platform.h"

//
// AmlStringConvParseIntegerU64 flags.
//
#define AML_STRING_CONV_FLAG_NONE            0
#define AML_STRING_CONV_FLAG_SKIP_LWS        (1 << 0) /* Skip leading whitespace, like strntox. */
#define AML_STRING_CONV_FLAG_NO_RADIX_PREFIX (1 << 1) /* Disallow optional prefixes, like "0x" for hex, etc. */
#define AML_STRING_CONV_FLAG_NO_SIGN_PREFIX  (1 << 2) /* Disallow optional sign prefix "+"/"-". */

//
// Attempt to parse an integer from string.
// If NumberRadix is 0, the number system base will be attempted to be automatically deduced by prefixes.
// Skips prefixed whitespace for compatibility with the behavior of some strntox implementations.
// Currently performs no error checking or overflow checking!
//
UINT64
AmlStringConvParseIntegerU64(
    _In_count_( StringLength ) const CHAR* String,
    _In_                       SIZE_T      StringLength,
    _In_                       INT         NumberRadix,
    _In_                       UINT        Flags,
    _Out_opt_                  SIZE_T*     pNumberLength
    );
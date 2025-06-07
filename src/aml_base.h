#pragma once

#include "aml_platform.h"
#include "aml_arena.h"
#include "aml_data.h"

//
// Read bit-granularity data from the given buffer field to the result data array.
// The given ResultDataSize is the max size of the ResultData array in *bytes*.
//
_Success_( return )
BOOLEAN
AmlCopyBits(
    _In_reads_bytes_( InputDataSize )   const VOID* InputData,
    _In_                                SIZE_T      InputDataSize,
    _Inout_bytecount_( ResultDataSize ) VOID*       ResultData,
    _In_                                SIZE_T      ResultDataSize,
    _In_                                SIZE_T      InputBitIndex,
    _In_                                SIZE_T      InputBitCount,
    _In_                                SIZE_T      OutputBitIndex
    );

//
// Convert a decimal number to a 4-bit digit BCD encoded value.
//
UINT64
AmlDecimalToBcd(
    _In_ UINT64 Input
    );

//
// Convert a 4-bit digit BCD encoded value to decimal.
//
UINT64
AmlBcdToDecimal(
    _In_ UINT64 Input
    );

//
// Performs no validation of the input string other than the length, is assumed to be a valid EISAID string
// in the following form: "UUUNNNN", where "U" is an uppercase letter and "N" is a hexadecimal digit.
// The results may not be a valid EISAID value if an invalid string is passed.
//
_Success_( return != 0 )
UINT32
AmlStringToEisaId(
    _In_count_( Length ) const CHAR* String,
    _In_                 SIZE_T      Length
    );

//
// Performs no validation of the input string other than the length, is assumed to be a valid EISAID string
// in the following form: "UUUNNNN", where "U" is an uppercase letter and "N" is a hexadecimal digit.
// The results may not be a valid EISAID value if an invalid string is passed.
//
_Success_( return != 0 )
UINT32
AmlStringZToEisaId(
    _In_z_ const CHAR* String
    );

//
// Convert a regular path string to a decoded AML_NAME_STRING.
//
_Success_( return )
BOOLEAN
AmlPathStringToNameString(
    _Inout_                  AML_ARENA*       Arena,
    _In_count_( PathLength ) const CHAR*      Path,
    _In_                     SIZE_T           PathLength,
    _Out_                    AML_NAME_STRING* NameString
    );

//
// Convert a regular null-terminated path string to a decoded AML_NAME_STRING.
//
_Success_( return )
BOOLEAN
AmlPathStringZToNameString(
    _Inout_ AML_ARENA*       Arena,
    _In_z_  const CHAR*      Path,
    _Out_   AML_NAME_STRING* NameString
    );
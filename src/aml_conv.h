#pragma once

#include "aml_platform.h"
#include "aml_data.h"
#include "aml_heap.h"

//
// AML_CONV_FLAGS ExplicitSubtype values.
//
#define AML_CONV_SUBTYPE_NONE              0
#define AML_CONV_SUBTYPE_TO_STRING         1 /* Special semantics for ToString (Buffer conversion only).*/
#define AML_CONV_SUBTYPE_TO_HEX_STRING     2 /* Special semantics for ToHexString (Integer, Buffer, String conversion) */
#define AML_CONV_SUBTYPE_TO_DECIMAL_STRING 3 /* Special semantics for ToDecimalString (Integer, Buffer, String conversion) */

//
// AML data-type simple conversion flags/parameters.
//
typedef UINT AML_CONV_FLAGS;

//
// AML_CONV_FLAGS bitfields/helpers.
//
#define AML_CONV_FLAGS_EXPLICIT                      (1 << 0)
#define AML_CONV_FLAGS_TEMPORARY                     (1 << 1)
#define AML_CONV_FLAGS_EXPLICIT_SUBTYPE_SHIFT        2
#define AML_CONV_FLAGS_EXPLICIT_SUBTYPE_MASK         ((1 << 3) - 1)
#define AML_CONV_FLAGS_EXPLICIT_SUBTYPE_SET(SubType) (((SubType) & AML_CONV_FLAGS_EXPLICIT_SUBTYPE_MASK) << AML_CONV_FLAGS_EXPLICIT_SUBTYPE_SHIFT)
#define AML_CONV_FLAGS_EXPLICIT_SUBTYPE_GET(Flags)   (((Flags) >> AML_CONV_FLAGS_EXPLICIT_SUBTYPE_SHIFT) & AML_CONV_FLAGS_EXPLICIT_SUBTYPE_MASK)
#define AML_CONV_FLAGS_IMPLICIT                      0
#define AML_CONV_FLAGS_EXPLICIT_TO_STRING            (AML_CONV_FLAGS_EXPLICIT | AML_CONV_FLAGS_EXPLICIT_SUBTYPE_SET(AML_CONV_SUBTYPE_TO_STRING))
#define AML_CONV_FLAGS_EXPLICIT_TO_HEX_STRING        (AML_CONV_FLAGS_EXPLICIT | AML_CONV_FLAGS_EXPLICIT_SUBTYPE_SET(AML_CONV_SUBTYPE_TO_HEX_STRING))
#define AML_CONV_FLAGS_EXPLICIT_TO_DECIMAL_STRING    (AML_CONV_FLAGS_EXPLICIT | AML_CONV_FLAGS_EXPLICIT_SUBTYPE_SET(AML_CONV_SUBTYPE_TO_DECIMAL_STRING))

//
// Convert string to buffer.
// If no buffer object exists, a new buffer object is created.
// If a buffer object already exists, it is completely overwritten.
// If the string is longer than the buffer, the string is truncated before copying.
// If the string is shorter than the buffer, the remaining buffer bytes are set to zero.
// In either case, the string is treated as a buffer, with each ASCII string character copied to one buffer byte,
// including the null terminator. A null (zero-length) string will be converted to a zero-length buffer.
//
_Success_( return )
BOOLEAN
AmlConvStringToBuffer(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Buffer,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Store a string to another existing string or empty object data.
//
_Success_( return )
BOOLEAN
AmlConvStringToString(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Attempts to convert a string to a field unit.
//
_Success_( return )
BOOLEAN
AmlConvStringToFieldUnit(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Convert string to integer.
// If no integer object exists, a new integer is created.
// The integer is initialized to the value zero and the ASCII string is interpreted as a hexadecimal constant.
// Each string character is interpreted as a hexadecimal value (‘0’-‘9’, ‘A’-‘F’, ‘a’-‘f’),
// starting with the first character as the most significant digit, and ending with the first non-hexadecimal character, end-of-string,
// or when the size of an integer is reached (8 characters for 32-bit integers and 16 characters for 64-bit integers).
// Note: the first non-hex character terminates the conversion without error, and a "0x" prefix is not allowed.
// Conversion of a null (zero-length) string to an integer is not allowed.
//
_Success_( return )
BOOLEAN
AmlConvStringToInteger(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      String,
    _Inout_ AML_DATA*            Integer,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Convert integer to buffer.
// If no buffer object exists, a new buffer object is created based on the size of the integer (4 bytes for 32-bit integers and 8 bytes for 64-bit integers).
// If a buffer object already exists, the Integer overwrites the entire Buffer object.
// If the integer requires more bits than the size of the Buffer, then the integer is truncated before being copied to the Buffer.
// If the integer contains fewer bits than the size of the buffer, the Integer is zero-extended to fill the entire buffer.
//
_Success_( return )
BOOLEAN
AmlConvIntegerToBuffer(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Integer,
    _Inout_ AML_DATA*            Buffer,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Convert integer to field unit (buffer field or op-region field).
//
_Success_( return )
BOOLEAN
AmlConvIntegerToFieldUnit(
    _Inout_ struct _AML_STATE*   State,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Convert integer to string.
// If no string object exists, a new string object is created based on the size of the integer
// (8 characters for 32-bit integers and 16 characters for 64-bit integers).
// If the string already exists, it is completely overwritten and truncated or extended to accommodate the converted integer exactly.
// In either case, the entire integer is converted to a string of hexadecimal ASCII characters.
//
_Success_( return )
BOOLEAN
AmlConvIntegerToString(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Integer,
    _Inout_ AML_DATA*            String,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Convert buffer to integer.
// If no integer object exists, a new integer is created.
// The contents of the buffer are copied to the Integer, starting with the least-significant bit
// and continuing until the buffer has been completely copied up to the maximum number of bits in an Integer.
// The size of an Integer is indicated by the Definition Block table header's Revision field.
// If the buffer is smaller than the size of an integer, it is zero extended.
// If the buffer is larger than the size of an integer, it is truncated.
// Conversion of a zero-length buffer to an integer is not allowed.
//
_Success_( return )
BOOLEAN
AmlConvBufferToInteger(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Buffer,
    _Inout_ AML_DATA*            Integer,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Convert from buffer to string.
// If no string object exists, a new string is created.
// If the string already exists, it is completely overwritten and truncated or extended to accommodate the converted buffer exactly.
// The entire contents of the buffer are converted to a string of two-character hexadecimal numbers, each separated by a space.
// A zero-length buffer will be converted to a null (zero-length) string.
//
_Success_( return )
BOOLEAN
AmlConvBufferToString(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Buffer,
    _Inout_ AML_DATA*            String,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Attempts to convert from a buffer to an empty object or another existing buffer.
//
_Success_( return )
BOOLEAN
AmlConvBufferToBuffer(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Attempts to convert from a buffer to a field unit.
//
_Success_( return )
BOOLEAN
AmlConvBufferToFieldUnit(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Attempts to store an integer to the given output object using the implicit store conversion rules.
//
_Success_( return )
BOOLEAN
AmlConvIntegerStore(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Attempts to store a buffer to the given output object using the implicit store conversion rules.
//
_Success_( return )
BOOLEAN
AmlConvBufferStore(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Attempts to store a string to the given output object using the implicit store conversion rules.
//
_Success_( return )
BOOLEAN
AmlConvStringStore(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Attempt to store from one object data value to another, performs the implicit store conversions.
//
_Success_( return )
BOOLEAN
AmlConvObjectStore(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    );

//
// Convert an object in-place to the given conversion type (convert, free old value, update with converted value).
// Passing a ConversionType of AML_DATA_TYPE_NONE performs no conversion.
//
_Success_( return )
BOOLEAN
AmlConvObjectInPlace(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _Inout_ AML_DATA*            Data,
    _In_    AML_DATA_TYPE        ConversionType,
    _In_    const AML_CONV_FLAGS ConvFlags
    );
#include "aml_state.h"
#include "aml_decoder.h"
#include "aml_debug.h"
#include "aml_base.h"
#include "aml_field.h"
#include "aml_state_snapshot.h"
#include "aml_buffer_field.h"
#include "aml_string_conv.h"
#include "aml_conv.h"

//
// Allocate backing buffer data according to the conversion flags (temporary/scoped or regular).
//
_Success_( return != NULL )
static
AML_BUFFER_DATA*
AmlConvCreateBufferData(
    _Inout_ struct _AML_STATE* State,
    _Inout_ AML_HEAP*          Heap,
    _In_    SIZE_T             Size,
    _In_    SIZE_T             MaxSize,
    _In_    AML_CONV_FLAGS     ConvFlags
    )
{
    if( ConvFlags & AML_CONV_FLAGS_TEMPORARY ) {
        return AmlStateSnapshotCreateBufferData( State, Heap, Size, MaxSize );
    } else {
        return AmlBufferDataCreate( Heap, Size, MaxSize );
    }
}

//
// String conversion.
//

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
    )
{
    AML_BUFFER_DATA* BufferResource;
    SIZE_T           i;
    SIZE_T           CopyLength;
    SIZE_T           Size;

    //
    // Input type must have already been deduced to be a string.
    //
    if( Input->Type != AML_DATA_TYPE_STRING ) {
        return AML_FALSE;
    }

    //
    // Output type must have already been deduced to be a buffer or none.
    //
    if( ( Buffer->Type != AML_DATA_TYPE_NONE ) && ( Buffer->Type != AML_DATA_TYPE_BUFFER ) ) {
        return AML_FALSE;
    }

    //
    // If no buffer object exists, a new buffer object is created.
    //
    if( ( Buffer->Type == AML_DATA_TYPE_NONE ) || ( Buffer->u.Buffer == NULL ) ) {
        //
        // Allocate memory for the entire string.
        //
        Size = ( Input->u.String->Size + 1 );
        if( ( Input->u.String->Size >= SIZE_MAX )
            || ( ( BufferResource = AmlConvCreateBufferData( State, Heap, Size, Size, ConvFlags ) ) == NULL ) )
        {
            return AML_FALSE;
        }

        //
        // Copy string data to buffer (null terminated).
        //
        for( i = 0; i < Input->u.String->Size; i++ ) {
            BufferResource->Data[ i ] = Input->u.String->Data[ i ];
        }
        BufferResource->Data[ Input->u.String->Size ] = '\0';

        //
        // Return new buffer.
        //
        *Buffer = ( AML_DATA ){
            .Type     = AML_DATA_TYPE_BUFFER,
            .u.Buffer = BufferResource,
        };

        return AML_TRUE;
    }

    //
    // If the string is longer than the buffer, the string is truncated before copying.
    //
    CopyLength = AML_MIN( Buffer->u.Buffer->Size, Input->u.String->Size );
    for( i = 0; i < CopyLength; i++ ) {
        Buffer->u.Buffer->Data[ i ] = Input->u.String->Data[ i ];
    }

    //
    // If the string is shorter than the buffer, the remaining buffer bytes are set to zero.
    //
    for( i = 0; i < ( Buffer->u.Buffer->Size - CopyLength ); i++ ) {
        Buffer->u.Buffer->Data[ CopyLength + i ] = 0;
    }

    return AML_TRUE;
}

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
    )
{
    AML_BUFFER_DATA* StringData;
    SIZE_T           i;
    SIZE_T           StringMaxSize;
    BOOLEAN          IsSmallerString;

    //
    // Input must have already been deduced to be a string.
    //
    if( Input->Type != AML_DATA_TYPE_STRING ) {
        return AML_FALSE;
    }

    //
    // If this is an existing string, determine if it isn't big enough for the new value.
    //
    IsSmallerString = AML_FALSE;
    if( Output->Type == AML_DATA_TYPE_STRING ) {
        if( Output->u.String != NULL ) {
            IsSmallerString = ( Output->u.String->Size < Input->u.String->Size );
        } else {
            IsSmallerString = AML_TRUE;
        }
    }

    //
    // If the destination is empty, or an existing string that isn't big enough, allocate memory for the new string value.
    //
    if( ( Output->Type == AML_DATA_TYPE_NONE ) || IsSmallerString ) {
        //
        // Free any existing output data (if this is a string).
        //
        AmlDataFree( Output );

        //
        // Allocate memory for the entire string.
        //
        StringMaxSize = ( Input->u.String->Size + 1 );
        if( ( Input->u.String->Size >= SIZE_MAX )
            || ( StringData = AmlConvCreateBufferData( State, Heap, Input->u.String->Size, StringMaxSize, ConvFlags ) ) == NULL )
        {
            return AML_FALSE;
        }

        //
        // Initialize new output string.
        //
        *Output = ( AML_DATA ){
            .Type     = AML_DATA_TYPE_STRING,
            .u.String = StringData
        };
    } else if( Output->Type != AML_DATA_TYPE_STRING ) {
        return AML_FALSE;
    }

    //
    // Copy input string value to output.
    //
    for( i = 0; i < AML_MIN( Output->u.String->Size, Input->u.String->Size ); i++ ) {
        Output->u.String->Data[ i ] = Input->u.String->Data[ i ];
    }
    Output->u.String->Data[ i ] = '\0';

    //
    // Truncate output string to new value.
    //
    Output->u.String->Size = Input->u.String->Size;
    return AML_TRUE;
}

//
// Attempts to convert from a string to a special BufferAcc field.
//
_Success_( return )
static
BOOLEAN
AmlConvStringToBufferAccField(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_OBJECT*          Object,
    _Inout_ AML_OBJECT_FIELD*    Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    )
{
    UINT8  DataBuffer[ sizeof( AML_REGION_ACCESS_DATA ) ];
    SIZE_T DataSize;

    //
    // Input type must have already been deduced.
    //
    if( ( Input->Type != AML_DATA_TYPE_STRING ) || ( Input->u.String == NULL ) ) {
        return AML_FALSE;
    }

    //
    // Truncate and copy all string data to the BufferAcc packet.
    //
    if( Input->u.String->Size > sizeof( DataBuffer ) ) {
        AML_DEBUG_WARNING(
            State,
            "Warning: Truncating string size (0x%"PRIx64") to maximum BufferAcc packet size (0x%"PRIx64").\n",
            ( UINT64 )Input->u.String->Size,
            ( UINT64 )sizeof( DataBuffer )
        );
    }
    DataSize = AML_MIN( Input->u.String->Size, sizeof( DataBuffer ) );
    AML_MEMCPY( DataBuffer, Input->u.String->Data, DataSize );

    //
    // Write the entire BufferAcc packet at once with no chunking/word-based ccess.
    //
    return AmlFieldUnitWrite( State, Object, DataBuffer, DataSize, AML_TRUE );
}

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
    )
{
    AML_OBJECT* FieldUnit;
    UINT64      i;

    //
    // Input type must have already been deduced.
    //
    if( Input->Type != AML_DATA_TYPE_STRING ) {
        return AML_FALSE;
    }

    //
    // Ensure that the output data is a valid field unit.
    //
    FieldUnit = Output->u.FieldUnit;
    if( ( Output->Type != AML_DATA_TYPE_FIELD_UNIT )
        || ( FieldUnit == NULL )
        || ( AmlObjectIsFieldUnit( FieldUnit ) == AML_FALSE ) )
    {
        return AML_FALSE;
    }

    //
    // Convert string to buffer field.
    // The string is treated as a buffer. If this buffer is smaller than the size of the buffer field, it is zero extended.
    // If the buffer is larger than the size of the buffer field, the upper bits are truncated.
    // Compatibility Note: This conversion was first introduced in ACPI 2.0. The behavior in ACPI 1.0 was undefined.
    //
    if( FieldUnit->Type == AML_OBJECT_TYPE_BUFFER_FIELD ) {
        return AmlBufferFieldWrite( State, &FieldUnit->u.BufferField,
                                    Input->u.String->Data, Input->u.String->Size,
                                    AML_TRUE );
    }

    //
    // For BufferAcc fields, directly pass through the string to the field.
    // Unsure if this is entirely correct, nothing specified by the spec.
    //
    switch( FieldUnit->Type ) {
    case AML_OBJECT_TYPE_FIELD:
        if( FieldUnit->u.Field.Element.AccessType != AML_FIELD_ACCESS_TYPE_BUFFER_ACC ) {
            break;
        }
        return AmlConvStringToBufferAccField( State, Heap, Input, FieldUnit, &FieldUnit->u.Field, ConvFlags );
    case AML_OBJECT_TYPE_BANK_FIELD:
        if( FieldUnit->u.BankField.Base.Element.AccessType != AML_FIELD_ACCESS_TYPE_BUFFER_ACC ) {
            break;
        }
        return AmlConvStringToBufferAccField( State, Heap, Input, FieldUnit, &FieldUnit->u.BankField.Base, ConvFlags );
    default:
        break;
    }

    //
    // Convert string to field unit.
    // Each character of the string is written, starting with the first, to the Field Unit.
    // If the Field Unit is less than eight bits, then the upper bits of each character are lost.
    // If the Field Unit is greater than eight bits, then the additional bits are zeroed.
    // TODO: This doesn't seem to be how ACPICA does it, need to verify and decide how to implement.
    // TODO: This needs to be fixed for BufferAcc!
    //
    for( i = 0; i < Input->u.String->Size; i++ ) {
        if( AmlFieldUnitWrite( State, FieldUnit, &Input->u.String->Data[ i ], sizeof( UINT8 ), AML_TRUE ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    return AML_TRUE;
}

//
// Attempts to convert a string to a debug object.
//
_Success_( return )
BOOLEAN
static
AmlConvStringToDebug(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    )
{
    if( ( Input->Type != AML_DATA_TYPE_STRING )
        || ( Output->Type != AML_DATA_TYPE_DEBUG ) 
        || ( Input->u.String == NULL ) )
    {
        return AML_FALSE;
    }
    AML_DEBUG_INFO( State, "Debug = \"%.*s\"\n", ( UINT )AML_MIN( Input->u.String->Size, UINT_MAX ), Input->u.String->Data );
    return AML_TRUE;
}

//
// Convert string to integer.
// If no integer object exists, a new integer is created.
// The integer is initialized to the value zero and the ASCII string is interpreted as a hexadecimal constant.
// Each string character is interpreted as a hexadecimal value ('0'-'9', 'A'-'F', 'a'-'f'),
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
    )
{
    SIZE_T StringLength;

    //
    // Conversion of a null (zero-length) string to an integer is not allowed.
    //
    if( String->Type != AML_DATA_TYPE_STRING || String->u.String->Size == 0 ) {
        return AML_FALSE;
    }

    //
    // 8 characters for 32-bit integers and 16 characters for 64-bit integers.
    //
    StringLength = AML_MIN( String->u.String->Size, ( State->IsIntegerSize64 ? 16 : 8 ) );

    //
    // Explicit conversion allows both decimal and hex strings with a "0x" radix prefix,
    // but implicit conversion only allows hex strings with no radix prefix!
    //
    if( ConvFlags & AML_CONV_FLAGS_EXPLICIT ) {
        *Integer = ( AML_DATA ){
            .Type      = AML_DATA_TYPE_INTEGER,
            .u.Integer = AmlStringConvParseIntegerU64(
                ( const CHAR* )String->u.String->Data,
                ( StringLength / sizeof( CHAR ) ),
                0,
                AML_STRING_CONV_FLAG_NO_SIGN_PREFIX,
                NULL
            )
        };
    } else {
        *Integer = ( AML_DATA ){
            .Type      = AML_DATA_TYPE_INTEGER,
            .u.Integer = AmlStringConvParseIntegerU64(
                ( const CHAR* )String->u.String->Data,
                ( StringLength / sizeof( CHAR ) ),
                16,
                ( AML_STRING_CONV_FLAG_NO_RADIX_PREFIX | AML_STRING_CONV_FLAG_NO_SIGN_PREFIX ),
                NULL
            )
        };
    }

    return AML_TRUE;
}

//
// Integer conversion.
//

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
    _Inout_  struct _AML_STATE*  State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Integer,
    _Inout_ AML_DATA*            Buffer,
    _In_    const AML_CONV_FLAGS ConvFlags
    )
{
    SIZE_T           IntegerSize;
    SIZE_T           UpdateSize;
    AML_BUFFER_DATA* BufferResource;
    SIZE_T           i;

    //
    // Type of input must have already been deduced to be an integer.
    //
    if( Integer->Type != AML_DATA_TYPE_INTEGER ) {
        return AML_FALSE;
    }

    //
    // Type of output must have already been deduced to be a buffer or none.
    //
    if( ( Buffer->Type != AML_DATA_TYPE_NONE ) && ( Buffer->Type != AML_DATA_TYPE_BUFFER ) ) {
        return AML_FALSE;
    }

    //
    // Integer size is 4 bytes for 32-bit integers and 8 bytes for 64-bit integers.
    //
    IntegerSize = ( State->IsIntegerSize64 ? 8 : 4 );

    //
    // If no buffer object exists, a new buffer object is created based on the size of the integer.
    //
    if( ( Buffer->Type == AML_DATA_TYPE_NONE ) || ( Buffer->u.Buffer == NULL ) ) {
        //
        // Allocate new buffer for the integer.
        //
        if( ( BufferResource = AmlConvCreateBufferData( State, Heap, IntegerSize, IntegerSize, ConvFlags ) ) == NULL ) {
            return AML_FALSE;
        }

        //
        // Set up empty integer-sized buffer.
        //
        *Buffer = ( AML_DATA ){
            .Type     = AML_DATA_TYPE_BUFFER,
            .u.Buffer = BufferResource,
        };
    }

    //
    // If the integer requires more bits than the size of the Buffer, then the integer is truncated before being copied to the Buffer.
    //
    _Analysis_assume_( Buffer->u.Buffer != NULL );
    UpdateSize = AML_MIN( IntegerSize, Buffer->u.Buffer->Size );
    for( i = 0; i < UpdateSize; i++ ) {
        Buffer->u.Buffer->Data[ i ] = ( UINT8 )( ( Integer->u.Integer >> ( 8 * i ) ) & 0xFF );
    }

    //
    // If the integer contains fewer bits than the size of the buffer, the Integer is zero-extended to fill the entire buffer.
    //
    for( i = 0; i < ( Buffer->u.Buffer->Size - UpdateSize ); i++ ) {
        Buffer->u.Buffer->Data[ UpdateSize + i ] = 0;
    }

    return AML_TRUE;
}

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
    )
{
    AML_OBJECT* FieldUnit;

    //
    // Input type must have already been deduced.
    //
    if( Input->Type != AML_DATA_TYPE_INTEGER ) {
        return AML_FALSE;
    }

    //
    // Ensure that the output data is a valid field unit.
    //
    FieldUnit = Output->u.FieldUnit;
    if( ( Output->Type != AML_DATA_TYPE_FIELD_UNIT )
        || ( FieldUnit == NULL )
        || ( AmlObjectIsFieldUnit( FieldUnit ) == AML_FALSE ) )
    {
        return AML_FALSE;
    }

    // 
    // Convert integer to buffer field.
    // The Integer overwrites the entire Buffer Field.
    // If the integer is smaller than the size of the buffer field, it is zero-extended.
    // If the integer is larger than the size of the buffer field, the upper bits are truncated.
    // Compatibility Note: This conversion was first introduced in ACPI 2.0. The behavior in ACPI 1.0 was undefined.
    //
    if( FieldUnit->Type == AML_OBJECT_TYPE_BUFFER_FIELD ) {
        return AmlBufferFieldWrite( State,
                                    &FieldUnit->u.BufferField,
                                    Input->u.IntegerRaw,
                                    sizeof( Input->u.IntegerRaw ),
                                    AML_TRUE );
    }

    //
    // Convert integer to op-region field object.
    // The Integer overwrites the entire Field Unit
    // If the integer is smaller than the size of the buffer field, it is zero-extended.
    // If the integer is larger than the size of the buffer field, the upper bits are truncated.
    //
    return AmlFieldUnitWrite( State,
                              FieldUnit,
                              Input->u.IntegerRaw,
                              sizeof( Input->u.IntegerRaw ),
                              AML_FALSE );
}

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
    _Inout_  struct _AML_STATE*  State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Integer,
    _Inout_ AML_DATA*            String,
    _In_    const AML_CONV_FLAGS ConvFlags
    )
{
    SIZE_T           IntegerSize;
    SIZE_T           IntegerStringSize;
    AML_BUFFER_DATA* StringData;
    SIZE_T           i;
    UINT64           IntegerScratch;
    CHAR             Nibble;
    CHAR             NibbleChar;
    CHAR             ConvString[ 32 + 1 ];
    SIZE_T           ConvStringSize;
    SIZE_T           ConvStringOffset;
    BOOLEAN          IsToDecimalString;

    //
    // Type of input must have already been deduced to be an integer.
    //
    if( Integer->Type != AML_DATA_TYPE_INTEGER ) {
        return AML_FALSE;
    }

    //
    // Type of output must have already been deduced to be a string or none.
    //
    if( ( String->Type != AML_DATA_TYPE_NONE ) && ( String->Type != AML_DATA_TYPE_STRING ) ) {
        return AML_FALSE;
    }

    //
    // Special semantics are required for explicit conversion by ToDecimalString.
    //
    IsToDecimalString = ( ( ConvFlags & AML_CONV_FLAGS_EXPLICIT )
                          && ( AML_CONV_FLAGS_EXPLICIT_SUBTYPE_GET( ConvFlags ) == AML_CONV_SUBTYPE_TO_DECIMAL_STRING ) );

    //
    // Get integer size dictated by revision.
    //
    IntegerSize = ( State->IsIntegerSize64 ? 8 : 4 );

    //
    // For decimal, 10 characters for 32-bit integers, and 20 characters for 64-bit integers.
    // For hexadecimal, 8 characters for 32-bit integers and 16 characters for 64-bit integers.
    //
    if( IsToDecimalString ) {
        IntegerStringSize = ( State->IsIntegerSize64 ? 20 : 10 );
    } else {
        IntegerStringSize = ( State->IsIntegerSize64 ? 16 : 8 );
    }

    //
    // If no string object exists, a new string object is created based on the size of the integer.
    // If the string already exists, it must be big enough to fit the string, otherwise, it will be extended and reallocated.
    //
    if( String->Type == AML_DATA_TYPE_NONE
        || ( String->u.String == NULL )
        || ( ( String->Type == AML_DATA_TYPE_STRING )
             && ( String->u.String->Size < IntegerStringSize ) ) )
    {
        //
        // Free previous string allocation (if any).
        //
        AmlDataFree( String );

        //
        // Allocate memory for the converted string.
        //
        if( ( StringData = AmlConvCreateBufferData( State, Heap, IntegerStringSize, ( IntegerStringSize + 1 ), ConvFlags ) ) == NULL ) {
            return AML_FALSE;
        }

        //
        // Set up empty integer-sized string.
        //
        *String = ( AML_DATA ){
            .Type     = AML_DATA_TYPE_STRING,
            .u.String = StringData
        };
    }

    //
    // String must be big enough to fit an encoded integer.
    //
    _Analysis_assume_( String->u.String != NULL );
    if( String->u.String->Size < IntegerSize ) {
        return AML_FALSE;
    }

    //
    // For explicit conversion by ToDecimalString,
    // the integer is converted to a string of decimal ASCII characters.
    //
    if( IsToDecimalString ) {
        IntegerScratch = Integer->u.Integer;
        for( i = 0; i < AML_MIN( IntegerStringSize, sizeof( ConvString ) ); i++ ) {
            if( ( IntegerScratch == 0 ) && ( i != 0 ) ) {
                break;
            }
            Nibble = ( CHAR )( IntegerScratch % 10 );
            NibbleChar = ( '0' + Nibble );
            ConvString[ IntegerStringSize - i - 1 ] = NibbleChar;
            IntegerScratch /= 10;
        }
        ConvStringSize = i;
        ConvStringOffset = ( IntegerStringSize - ConvStringSize );
    } else {
        //
        // For implicit conversion and hex conversion,
        // the entire integer is converted to a string of hexadecimal ASCII characters.
        //
        IntegerScratch = Integer->u.Integer;
        for( i = 0; i < AML_MIN( IntegerStringSize, sizeof( ConvString ) ); i++ ) {
            if( ( IntegerScratch == 0 ) && ( i != 0 ) ) {
                break;
            }
            Nibble = ( CHAR )( IntegerScratch & 0xF );
            NibbleChar = ( ( Nibble < 10 ) ? ( '0' + Nibble ) : ( 'a' + Nibble - 10 ) );
            ConvString[ IntegerStringSize - i - 1 ] = NibbleChar;
            IntegerScratch >>= 4;
        }
        ConvStringSize = i;
        ConvStringOffset = ( IntegerStringSize - ConvStringSize );
    }

    //
    // Copy hex string to output string (done in two parts, as the hex string is built backwards).
    //
    for( i = 0; i < ConvStringSize; i++ ) {
        String->u.String->Data[ i ] = ConvString[ ConvStringOffset + i ];
    }
    String->u.String->Data[ ConvStringSize ] = '\0';

    //
    // If the string already exists, it is completely overwritten and truncated or extended to accommodate the converted integer exactly.
    //
    String->u.String->Size = ConvStringSize;
    return AML_TRUE;
}

//
// Output integer to debug object.
//
_Success_( return )
static
BOOLEAN
AmlConvIntegerToDebug(
    _In_    const struct _AML_STATE* State,
    _In_    const AML_DATA*          Input,
    _Inout_ AML_DATA*                Output,
    _In_    const AML_CONV_FLAGS     ConvFlags
    )
{
    SIZE_T IntegerStringSize;
    SIZE_T i;
    UINT64 IntegerScratch;
    CHAR   Nibble;
    CHAR   NibbleChar;
    CHAR   ConvString[ 32 + 1 ];
    SIZE_T ConvStringSize;
    SIZE_T ConvStringOffset;

    //
    // Types must already have been deduced.
    //
    if( ( Input->Type != AML_DATA_TYPE_INTEGER )
        || ( Output->Type != AML_DATA_TYPE_DEBUG ) )
    {
        return AML_FALSE;
    }

    //
    // For hexadecimal, 8 characters for 32-bit integers and 16 characters for 64-bit integers.
    //
    IntegerStringSize = ( State->IsIntegerSize64 ? 16 : 8 );
    IntegerScratch = Input->u.Integer;
    for( i = 0; i < AML_MIN( IntegerStringSize, sizeof( ConvString ) ); i++ ) {
        if( ( IntegerScratch == 0 ) && ( i != 0 ) ) {
            break;
        }
        Nibble = ( CHAR )( IntegerScratch & 0xF );
        NibbleChar = ( ( Nibble < 10 ) ? ( '0' + Nibble ) : ( 'a' + Nibble - 10 ) );
        ConvString[ IntegerStringSize - i - 1 ] = NibbleChar;
        IntegerScratch >>= 4;
    }
    ConvStringSize = i;
    ConvStringOffset = ( IntegerStringSize - ConvStringSize );

    //
    // Print converted hexadecimal string.
    //
    AML_DEBUG_INFO( State, "Debug = 0x" );
    for( i = 0; i < ConvStringSize; i++ ) {
        AML_DEBUG_INFO( State, "%c", ConvString[ ConvStringOffset + i ] );
    }
    AML_DEBUG_INFO( State, "\n" );
    return AML_TRUE;
}

//
// Buffer conversion.
//

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
    )
{
    SIZE_T IntegerSize;
    SIZE_T UpdateSize;
    SIZE_T i;

    //
    // Conversion of a zero-length buffer to an integer is not allowed.
    //
    if( ( Buffer->Type != AML_DATA_TYPE_BUFFER ) || ( Buffer->u.Buffer->Size == 0 ) ) {
        return AML_FALSE;
    }

    //
    // If no integer object exists, a new integer is created.
    //
    if( Integer->Type == AML_DATA_TYPE_NONE ) {
        *Integer = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER };
    } else if( Integer->Type != AML_DATA_TYPE_INTEGER ) {
        return AML_FALSE;
    }

    //
    // Integer size is 4 bytes for 32-bit integers and 8 bytes for 64-bit integers.
    //
    IntegerSize = ( State->IsIntegerSize64 ? 8 : 4 );

    //
    // If the buffer is smaller than the size of an integer, it is zero extended.
    //
    Integer->u.Integer = 0;

    //
    // Accumulate little-endian integer bytes from buffer.
    // If the buffer is larger than the size of an integer, it is truncated.
    //
    UpdateSize = AML_MIN( IntegerSize, Buffer->u.Buffer->Size );
    for( i = 0; i < UpdateSize; i++ ) {
        Integer->u.Integer |= ( ( ( UINT64 )Buffer->u.Buffer->Data[ i ] ) << ( 8 * i ) );
    }

    return AML_TRUE;
}

//
// Convert from buffer to string.
// If no string object exists, a new string is created.
// If the string already exists, it is completely overwritten and truncated or extended to accommodate the converted buffer exactly.
// The entire contents of the buffer are converted to a string of two-character hexadecimal numbers, each separated by a space.
// A zero-length buffer will be converted to a null (zero-length) string.
// 
// TODO: Acpica diverges from the spec in regards to how hexadecimal numbers are converted to string (0x prefixed, etc).
//
_Success_( return )
BOOLEAN
AmlConvBufferToString(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Buffer,
    _Inout_ AML_DATA*            String,
    _In_    const AML_CONV_FLAGS ConvFlags
    )
{
    SIZE_T           BufferStringLength;
    AML_BUFFER_DATA* StringData;
    BOOLEAN          IsSmallerString;
    SIZE_T           i;
    UINT8            Byte;
    UINT8            Nibble1;
    UINT8            Nibble2;
    CHAR             NibbleChar1;
    CHAR             NibbleChar2;
    SIZE_T           OutputOffset;
    BOOLEAN          IsToHexString;
    BOOLEAN          IsToDecimalString;

    //
    // Input type must have already been deduced.
    //
    if( Buffer->Type != AML_DATA_TYPE_BUFFER ) {
        return AML_FALSE;
    }

    //
    // Explicit conversion by ToHexString and ToDecimalString require different semantics.
    //
    IsToHexString = ( ( ConvFlags & AML_CONV_FLAGS_EXPLICIT )
                      && ( AML_CONV_FLAGS_EXPLICIT_SUBTYPE_GET( ConvFlags ) == AML_CONV_SUBTYPE_TO_HEX_STRING ) );
    IsToDecimalString = ( ( ConvFlags & AML_CONV_FLAGS_EXPLICIT )
                          && ( AML_CONV_FLAGS_EXPLICIT_SUBTYPE_GET( ConvFlags ) == AML_CONV_SUBTYPE_TO_DECIMAL_STRING ) );

    //
    // For implicit/regular conversion, the entire contents of the buffer are converted to a string of
    // two-character hexadecimal numbers, each separated by a space. Therefor, 3 characters are required per buffer byte.
    // TODO: Acpica diverges from the spec in regards to how hexadecimal numbers are converted to string (0x prefixed, etc).
    //
    if( Buffer->u.Buffer->Size >= ( SIZE_MAX / 3 ) ) {
        return AML_FALSE;
    }
    BufferStringLength = ( Buffer->u.Buffer->Size * 3 );

    //
    // If this is an existing string, determine if it isn't big enough for the new value.
    //
    IsSmallerString = AML_FALSE;
    if( String->Type == AML_DATA_TYPE_STRING ) {
        if( String->u.String != NULL ) {
            IsSmallerString = ( BufferStringLength > String->u.String->Size );
        } else {
            IsSmallerString = AML_TRUE;
        }
    }

    //
    // If no string object exists, or the string must be extended, a new string is created.
    //
    if( ( String->Type == AML_DATA_TYPE_NONE ) || IsSmallerString ) {
        //
        // Free previous string allocation (if any).
        //
        AmlDataFree( String );

        //
        // Allocate memory for a new string.
        //
        if( ( StringData = AmlConvCreateBufferData( State, Heap, BufferStringLength,
                                                    ( BufferStringLength + 1 ), ConvFlags ) ) == NULL )
        {
            return AML_FALSE;
        }
        *String = ( AML_DATA ){ .Type = AML_DATA_TYPE_STRING, .u.String = StringData };
    } else if( String->Type != AML_DATA_TYPE_STRING ) {
        return AML_FALSE;
    }

    //
    // Existing strings are truncated (or increased in size).
    //
    OutputOffset = 0;
    String->u.String->Size = BufferStringLength;

    //
    // Conversion to string works entirely differently for explicit conversion by ToString.
    // Starting with the first byte, the contents of the buffer are copied into the string until the number of characters
    // specified by Length is reached or a null (0) character is found. 
    // If Length is not specified or is Ones, then the contents of the buffer are copied until a null (0) character is found. 
    // If the source buffer has a length of zero, a zero length (null terminator only) string will be created.
    //
    if( ( ConvFlags & AML_CONV_FLAGS_EXPLICIT )
        && ( AML_CONV_FLAGS_EXPLICIT_SUBTYPE_GET( ConvFlags ) == AML_CONV_SUBTYPE_TO_STRING ) )
    {
        for( i = 0; i < Buffer->u.Buffer->Size; i++ ) {
            Byte = Buffer->u.Buffer->Data[ i ];
            if( Byte == '\0' ) {
                break;
            }
            String->u.String->Data[ OutputOffset++ ] = Byte;
        }
    } else {
        //
        // For implicit conversions, the entire contents of the buffer are converted to
        // a string of two-character hexadecimal numbers, each separated by a space.
        // For explicit conversion by ToHexString, same as implicit conversion, but comma separated.
        // For explicit conversion by ToDecimalString, comma separated decimal values.
        //
        for( i = 0; i < Buffer->u.Buffer->Size; i++ ) {
            Byte = Buffer->u.Buffer->Data[ i ];
            if( IsToDecimalString ) {
                Nibble1 = ( Byte % 10 );
                Nibble2 = ( ( Byte / 10 ) % 10 );
            } else {
                Nibble1 = ( Byte & 0xF );
                Nibble2 = ( ( Byte >> 4 ) & 0xF );
            }
            NibbleChar1 = ( ( Nibble1 < 10 ) ? ( '0' + Nibble1 ) : ( 'a' + Nibble1 - 10 ) );
            NibbleChar2 = ( ( Nibble2 < 10 ) ? ( '0' + Nibble2 ) : ( 'a' + Nibble2 - 10 ) );

            //
            // Output nibble characters to string.
            //
            if( ( IsToDecimalString == AML_FALSE ) || ( Nibble2 != 0 ) ) { /* Don't zero pad decimal byte values. */
                String->u.String->Data[ OutputOffset++ ] = NibbleChar2;
            }
            String->u.String->Data[ OutputOffset++ ] = NibbleChar1;

            //
            // If this byte is followed by a another byte, separate them with a space,
            // or, for explicit conversion by ToHexString/ToDecimalString, a comma. 
            //
            if( i < ( Buffer->u.Buffer->Size - 1 ) ) {
                String->u.String->Data[ OutputOffset++ ] = ( ( IsToHexString || IsToDecimalString ) ? ',' : ' ' );
            }
        }
    }

    //
    // Null terminate string and set final length (may be shorter than our previous worst-case guess).
    //
    String->u.String->Data[ OutputOffset ] = '\0';
    String->u.String->Size = OutputOffset;
    return AML_TRUE;
}

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
    )
{
    AML_BUFFER_DATA* BufferResource;
    SIZE_T           i;

    //
    // Input type must have already been deduced.
    //
    if( Input->Type != AML_DATA_TYPE_BUFFER ) {
        return AML_FALSE;
    }

    //
    // If the output data is uninitialized, or an existing buffer that isn't big enough,
    // allocate sufficient memory for a new copy of the input buffer, and set up the output.
    //
    if( Output->Type == AML_DATA_TYPE_NONE
        || ( ( Output->Type == AML_DATA_TYPE_BUFFER )
             && ( Output->u.Buffer == NULL ) ) )
    {
        //
        // Create a new buffer for the output.
        //
        if( ( BufferResource = AmlConvCreateBufferData( State, Heap, Input->u.Buffer->Size,
                                                        Input->u.Buffer->Size, ConvFlags ) ) == NULL )
        {
            return AML_FALSE;
        }
        *Output = ( AML_DATA ){ .Type = AML_DATA_TYPE_BUFFER, .u.Buffer = BufferResource };
    } else if( Output->Type == AML_DATA_TYPE_BUFFER ) {
        //
        // Resize the output buffer if it isn't big enough to hold the input buffer contents.
        //
        if( Input->u.Buffer->Size > Output->u.Buffer->MaxSize ) {
            if( AmlBufferDataResize( Output->u.Buffer, Heap, Input->u.Buffer->Size ) == AML_FALSE ) {
                return AML_FALSE;
            }
        }
    } else {
        return AML_FALSE;
    }

    //
    // Update output buffer size (truncate if needed).
    //
    _Analysis_assume_( Output->u.Buffer != NULL );
    Output->u.Buffer->Size = Input->u.Buffer->Size;

    //
    // Copy input data to output buffer data.
    //
    for( i = 0; i < Input->u.Buffer->Size; i++ ) {
        Output->u.Buffer->Data[ i ] = Input->u.Buffer->Data[ i ];
    }

    return AML_TRUE;
}

//
// Attempts to convert from a buffer to a special BufferAcc field.
//
_Success_( return )
static
BOOLEAN
AmlConvBufferToBufferAccField(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_OBJECT*          Object,
    _Inout_ AML_OBJECT_FIELD*    Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    )
{
    UINT8  DataBuffer[ sizeof( AML_REGION_ACCESS_DATA ) ];
    SIZE_T DataSize;

    //
    // Input type must have already been deduced.
    //
    if( ( Input->Type != AML_DATA_TYPE_BUFFER ) || ( Input->u.Buffer == NULL ) ) {
        return AML_FALSE;
    }

    //
    // Truncate and copy all buffer data to the BufferAcc packet.
    //
    if( Input->u.Buffer->Size > sizeof( DataBuffer ) ) {
        AML_DEBUG_WARNING(
            State,
            "Warning: Truncating buffer size (0x%"PRIx64") to maximum BufferAcc packet size (0x%"PRIx64").\n",
            ( UINT64 )Input->u.Buffer->Size,
            ( UINT64 )sizeof( DataBuffer )
        );
    }
    DataSize = AML_MIN( Input->u.Buffer->Size, sizeof( DataBuffer ) );
    AML_MEMCPY( DataBuffer, Input->u.Buffer->Data, DataSize );

    //
    // Write the entire BufferAcc packet at once with no chunking/word-based ccess.
    //
    return AmlFieldUnitWrite( State, Object, DataBuffer, DataSize, AML_TRUE );
}

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
    )
{
    AML_OBJECT*        FieldUnit;
    UINT64             i;
    UINT64             FieldBitCount;
    UINT64             BufferBitCount;
    UINT64             ChunkByteCount;
    UINT8*             ChunkData;
    UINT64             CopyBitCount;
    UINT64             CopyByteCount;
    AML_ARENA_SNAPSHOT TempSnapshot;
    BOOLEAN            Success;

    //
    // Input type must have already been deduced.
    //
    if( ( Input->Type != AML_DATA_TYPE_BUFFER ) || ( Input->u.Buffer == NULL ) ) {
        return AML_FALSE;
    }

    //
    // Ensure that the output data is a valid field unit.
    //
    FieldUnit = Output->u.FieldUnit;
    if( ( Output->Type != AML_DATA_TYPE_FIELD_UNIT )
        || ( FieldUnit == NULL )
        || ( AmlObjectIsFieldUnit( FieldUnit ) == AML_FALSE ) )
    {
        return AML_FALSE;
    }

    //
    // BufferAcc fields require special handling, writes are truncated to packet size.
    //
    switch( FieldUnit->Type ) {
    case AML_OBJECT_TYPE_FIELD:
        if( FieldUnit->u.Field.Element.AccessType != AML_FIELD_ACCESS_TYPE_BUFFER_ACC ) {
            break;
        }
        return AmlConvBufferToBufferAccField( State, Heap, Input, FieldUnit, &FieldUnit->u.Field, ConvFlags );
    case AML_OBJECT_TYPE_BANK_FIELD:
        if( FieldUnit->u.BankField.Base.Element.AccessType != AML_FIELD_ACCESS_TYPE_BUFFER_ACC ) {
            break;
        }
        return AmlConvBufferToBufferAccField( State, Heap, Input, FieldUnit, &FieldUnit->u.BankField.Base, ConvFlags );
    default:
        break;
    }

    //
    // Convert from buffer to buffer field.
    // The contents of the buffer are copied to the Buffer Field.
    // If the buffer is smaller than the size of the buffer field, it is zero extended.
    // If the buffer is larger than the size of the buffer field, the upper bits are truncated.
    // Compatibility Note: This conversion was first introduced in ACPI 2.0. The behavior in ACPI 1.0 was undefined.
    //
    if( FieldUnit->Type == AML_OBJECT_TYPE_BUFFER_FIELD ) {
        return AmlBufferFieldWrite( State, &FieldUnit->u.BufferField,
                                    Input->u.Buffer->Data, Input->u.Buffer->Size,
                                    AML_TRUE );
    }

    //
    // If the buffer (or the last piece of the buffer, if broken up) is smaller than the size
    // of the Field Unit, it is zero extended before being written.
    // TODO: This needs to be fixed for BufferAcc!
    //
    FieldBitCount = AmlFieldUnitLengthBits( FieldUnit );
    BufferBitCount = ( ( UINT64 )Input->u.Buffer->Size * CHAR_BIT );
    if( BufferBitCount <= FieldBitCount ) {
        return AmlFieldUnitWrite( State, FieldUnit, Input->u.Buffer->Data,
                                  Input->u.Buffer->Size, AML_TRUE );
    }

    //
    // Allocate a temporary buffer to fit an entire field's worth of data.
    // TODO: Switch all usage of namespace's TempArena to state TempArena!.
    //
    ChunkByteCount = ( ( FieldBitCount + 63 ) / CHAR_BIT );
    if( FieldBitCount > ( SIZE_MAX - 63 ) ) {
        return AML_FALSE;
    }
    TempSnapshot = AmlArenaSnapshot( &State->Namespace.TempArena );
    ChunkData = AmlArenaAllocate( &State->Namespace.TempArena, ChunkByteCount );
    if( ChunkData == NULL ) {
        return AML_FALSE;
    }

    //
    // Zero out temporary write chunk bit buffer before copying bits to it.
    //
    for( i = 0; i < ChunkByteCount; i++ ) {
        ChunkData[ i ] = 0;
    }

    //
    // If the buffer is larger (in bits) than the size of the Field Unit,
    // it is broken into pieces and completely written to the Field Unit, lower chunks first.
    //
    Success = AML_TRUE;
    for( i = 0; i < BufferBitCount; i += FieldBitCount ) {
        CopyBitCount = AML_MIN( ( BufferBitCount - i ), FieldBitCount );
        CopyByteCount = ( ( CopyBitCount + 7 ) / CHAR_BIT );
        if( ( Success = AmlCopyBits( Input->u.Buffer->Data, Input->u.Buffer->Size,
                                     ChunkData, ChunkByteCount, i, CopyBitCount, 0 ) ) == AML_FALSE )
        {
            break;
        } else if( ( Success = AmlFieldUnitWrite( State, FieldUnit, ChunkData, CopyByteCount, AML_TRUE ) ) == AML_FALSE ) {
            break;
        }
    }

    AmlArenaSnapshotRollback( &State->Namespace.TempArena, &TempSnapshot );
    return Success;
}

//
// Output buffer to debug information.
//
_Success_( return )
static
BOOLEAN
AmlConvBufferToDebug(
    _In_    const struct _AML_STATE* State,
    _Inout_ AML_HEAP*                Heap,
    _In_    const AML_DATA*          Input,
    _Inout_ AML_DATA*                Output,
    _In_    const AML_CONV_FLAGS     ConvFlags
    )
{
    SIZE_T i;
    SIZE_T j;
    SIZE_T LineBytecount;
    CHAR   Char;

    //
    // Types must have already been deduced.
    //
    if( ( Input->Type != AML_DATA_TYPE_BUFFER )
        || ( Output->Type != AML_DATA_TYPE_DEBUG )
        || ( Input->u.Buffer == NULL ) )
    {
        return AML_FALSE;
    }

    //
    // Special handling for empty buffer, don't do hexdump formatting.
    //
    if( Input->u.Buffer->Size == 0 ) {
        AML_DEBUG_INFO( State, "Debug = Buffer [0]\n" );
        return AML_TRUE;
    }

    //
    // Hex dump of the buffer contents. 
    //
    AML_DEBUG_INFO( State, "Debug = Buffer [0x%"PRIx64"] {\n", ( UINT64 )Input->u.Buffer->Size );
    for( i = 0; i < Input->u.Buffer->Size; i += j ) {
        LineBytecount = AML_MIN( ( Input->u.Buffer->Size - i ), 16 );
        AML_DEBUG_INFO( State, "    %04"PRIx64": ", ( UINT64 )i );
        for( j = 0; j < LineBytecount; j++ ) {
            AML_DEBUG_INFO( State, "%02x ", ( UINT )Input->u.Buffer->Data[ i + j ] );
        }
        for( j = 0; j < ( ( 16 - LineBytecount ) * 3 ); j++ ) {
            AML_DEBUG_INFO( State, " " );
        }
        for( j = 0; j < LineBytecount; j++ ) {
            Char = Input->u.Buffer->Data[ i + j ];
            if( ( Char >= 32 ) && ( Char <= 126 ) ) {
                AML_DEBUG_INFO( State, "%c", Char );
            } else {
                AML_DEBUG_INFO( State, "." );
            }
        }
        AML_DEBUG_INFO( State, "\n" );
    }
    AML_DEBUG_INFO( State, "}\n" );
    return AML_TRUE;
}

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
    )
{
    //
    // Input type must have already been deduced to be an integer.
    //
    if( Input->Type != AML_DATA_TYPE_INTEGER ) {
        return AML_FALSE;
    }

    //
    // Attempt to perform implicit conversion to other supported destination object types.
    //
    switch( Output->Type ) {
    case AML_DATA_TYPE_STRING:
        return AmlConvIntegerToString( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_BUFFER:
        return AmlConvIntegerToBuffer( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_FIELD_UNIT:
        return AmlConvIntegerToFieldUnit( State, Input, Output, ConvFlags );
    case AML_DATA_TYPE_DEBUG:
        return AmlConvIntegerToDebug( State, Input, Output, ConvFlags );
    case AML_DATA_TYPE_NONE:
    case AML_DATA_TYPE_INTEGER:
        AmlDataFree( Output );
        *Output = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = Input->u.Integer };
        return AML_TRUE;
    default:
        break;
    }

    return AML_FALSE;
}

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
    )
{
    //
    // Input type must have already been deduced to be a buffer.
    //
    if( Input->Type != AML_DATA_TYPE_BUFFER ) {
        return AML_FALSE;
    }

    //
    // Attempt to perform implicit conversion to other supported destination object types.
    //
    switch( Output->Type ) {
    case AML_DATA_TYPE_STRING:
        return AmlConvBufferToString( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_INTEGER:
        return AmlConvBufferToInteger( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_FIELD_UNIT:
        return AmlConvBufferToFieldUnit( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_DEBUG:
        return AmlConvBufferToDebug( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_NONE:
    case AML_DATA_TYPE_BUFFER:
        return AmlConvBufferToBuffer( State, Heap, Input, Output, ConvFlags );
    default:
        break;
    }

    return AML_FALSE;
}

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
    )
{
    //
    // Input type must have already been deduced to be a string.
    //
    if( Input->Type != AML_DATA_TYPE_STRING ) {
        return AML_FALSE;
    }

    //
    // Attempt to perform implicit conversion to other supported destination object types.
    //
    switch( Output->Type ) {
    case AML_DATA_TYPE_BUFFER:
        return AmlConvStringToBuffer( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_INTEGER:
        return AmlConvStringToInteger( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_FIELD_UNIT:
        return AmlConvStringToFieldUnit( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_DEBUG:
        return AmlConvStringToDebug( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_NONE:
    case AML_DATA_TYPE_STRING:
        return AmlConvStringToString( State, Heap, Input, Output, ConvFlags );
    default:
        break;
    }

    return AML_FALSE;
}

//
// Store references.
//
_Success_( return )
BOOLEAN
AmlConvReferenceStore(
    _In_    const struct _AML_STATE* State,
    _Inout_ AML_HEAP*                Heap,
    _In_    const AML_DATA*          Input,
    _Inout_ AML_DATA*                Output,
    _In_    const AML_CONV_FLAGS     ConvFlags
    )
{
    //
    // No explicit conversions exist for references.
    //
    if( ConvFlags & AML_CONV_FLAGS_EXPLICIT ) {
        return AML_FALSE;
    }

    //
    // Handle stores to other types.
    //
    switch( Output->Type ) {
    case AML_DATA_TYPE_DEBUG:
        AML_DEBUG_INFO( State, "Debug = Reference" );
        if( Input->u.Reference.Object != NULL ) {
            AML_DEBUG_INFO( State, " %s (", AmlObjectToAcpiTypeName( Input->u.Reference.Object ) );
            AmlDebugPrintObjectName( State, AML_DEBUG_LEVEL_INFO, Input->u.Reference.Object );
            AML_DEBUG_INFO( State, ")" );
        } else {
            AML_DEBUG_INFO( State, " [Nil]" );
        }
        AML_DEBUG_INFO( State, "\n" );
        break;
    case AML_DATA_TYPE_NONE:
    case AML_DATA_TYPE_REFERENCE:
        //
        // Overwrite the existing reference.
        //
        AmlDataFree( Output );
        return AmlDataDuplicate( Input, Heap, Output );
    default:
        break;
    }

    return AML_FALSE;
}

//
// Store field unit data to another piece of data.
//
_Success_( return )
static
BOOLEAN
AmlConvFieldUnitStore(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    )
{
    AML_DATA         SubInputData;
    SIZE_T           FieldUnitBufferSize;
    AML_BUFFER_DATA* SubBuffer;
    BOOLEAN          Success;

    //
    // Ensure that the input data is a field unit type.
    //
    if( ( Input->Type != AML_DATA_TYPE_FIELD_UNIT ) || ( Input->u.FieldUnit == NULL ) ) {
        return AML_FALSE;
    }

    //
    // Can be implicitly or explicitly converted to these Data Types (In priority order):
    // Integer, Buffer, String, Debug Object.
    //
    switch( Output->Type ) {
    case AML_DATA_TYPE_NONE:
    case AML_DATA_TYPE_INTEGER:
    case AML_DATA_TYPE_BUFFER:
    case AML_DATA_TYPE_STRING:
    case AML_DATA_TYPE_DEBUG:
    case AML_DATA_TYPE_FIELD_UNIT:
        break;
    default:
        return AML_FALSE;
    }

    //
    // If the Buffer Field is smaller than or equal to the size of an Integer (in bits),
    // it will be treated as an Integer. Otherwise, it will be treated as a buffer.
    // 
    FieldUnitBufferSize = AmlFieldUnitAccessBufferSize( Input->u.FieldUnit );
    if( FieldUnitBufferSize < ( State->IsIntegerSize64 ? 8 : 4 ) ) {
        SubInputData = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER };
        if( AmlFieldUnitRead( State, Input->u.FieldUnit, SubInputData.u.IntegerRaw,
                              sizeof( SubInputData.u.IntegerRaw ), AML_TRUE, NULL ) == AML_FALSE )
        {
            return AML_FALSE;
        }
    } else {
        SubInputData = ( AML_DATA ){ .Type = AML_DATA_TYPE_BUFFER };
        if( ( SubBuffer = AmlConvCreateBufferData( State, Heap, FieldUnitBufferSize, FieldUnitBufferSize, ConvFlags ) ) == NULL ) {
            return AML_FALSE;
        } else if( AmlFieldUnitRead( State, Input->u.FieldUnit, SubBuffer->Data, SubBuffer->Size, AML_TRUE, NULL ) == AML_FALSE ) {
            AmlBufferDataRelease( SubBuffer );
            return AML_FALSE;
        }
        SubInputData.u.Buffer = SubBuffer;
    }

    //
    // Defer conversion to the handler for the sub-type we have chosen (integer or buffer).
    //
    Success = AmlConvObjectStore( State, Heap, &SubInputData, Output, ConvFlags );

    //
    // Release temporary copy of the data read from the field unit.
    //
    AmlDataFree( &SubInputData );
    return Success;
}

//
// Store package element data to another piece of data.
//
_Success_( return )
static
BOOLEAN
AmlConvPackageElementStore(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    )
{
    AML_PACKAGE_ELEMENT* Element;

    //
    // Lookup the referenced package element entry, and pass on the store to the element's type handler.
    //
    Element = AmlPackageDataLookupElement( Input->u.PackageElement.Package, Input->u.PackageElement.ElementIndex );
    if( Element == NULL ) {
        return AML_FALSE;
    }
    return AmlConvObjectStore( State, Heap, &Element->Value, Output, ConvFlags );
}

//
// Store package data to another piece of data.
// the ASL spec says that it isn't in the table of data type conversion table,
// but it is mentioned in data type converion rules. ACPICA allows storing to
// locals, args, and named objects, causes a deep-copy entirely overriding the type of the destination.
//
_Success_( return )
static
BOOLEAN
AmlConvPackageStore(
    _Inout_ struct _AML_STATE*   State,
    _Inout_ AML_HEAP*            Heap,
    _In_    const AML_DATA*      Input,
    _Inout_ AML_DATA*            Output,
    _In_    const AML_CONV_FLAGS ConvFlags
    )
{
    AML_PACKAGE_ELEMENT** ElementList;
    SIZE_T                i;
    AML_PACKAGE_ELEMENT*  Element;
    AML_PACKAGE_ELEMENT*  InputElement;

    //
    // Ensure that the input data is a package.
    //
    if( ( Input->Type != AML_DATA_TYPE_PACKAGE ) || ( Input->u.Package == NULL ) ) {
        return AML_FALSE;
    }

    //
    // A package can only be stored to another package (or uninitialized object).
    // The source package is duplicated, not referenced, and completely overwrites the existing package.
    //
    switch( Output->Type ) {
    case AML_DATA_TYPE_DEBUG:
        AML_DEBUG_INFO( State, "Debug = [Package]\n" ); /* TODO! */
        return AML_TRUE;
    case AML_DATA_TYPE_NONE:
    case AML_DATA_TYPE_PACKAGE:
        AmlDataFree( Output );
        break;
    default:
        return AML_FALSE;
    }

    //
    // If no package object exists, a new package object is created.
    //
    *Output = ( AML_DATA ){ .Type = AML_DATA_TYPE_PACKAGE };
    if( ( Output->u.Package = AmlHeapAllocate( Heap, sizeof( *Output->u.Package ) ) ) == NULL ) {
        return AML_FALSE;
    }
    *Output->u.Package = ( AML_PACKAGE_DATA ){ .ParentHeap = Heap, .ReferenceCount = 1 };

    //
    // Create a new set of package elements big enough to fit the source elements.
    //
    ElementList = AmlHeapAllocate( Heap, ( sizeof( *ElementList ) * Input->u.Package->ElementCount ) );
    if( ElementList == NULL ) {
        return AML_FALSE;
    }
    Output->u.Package->Elements = ElementList;
    Output->u.Package->ElementCount = Input->u.Package->ElementCount;
    Output->u.Package->ElementArrayHeap = Heap;
    for( i = 0; i < Input->u.Package->ElementCount; i++ ) {
        if( ( Element = AmlHeapAllocate( Heap, sizeof( *Element ) ) ) != NULL ) {
            *Element = ( AML_PACKAGE_ELEMENT ){ .ParentHeap = Heap, .Value = { .Type = AML_DATA_TYPE_NONE } };
        }
        ElementList[ i ] = Element;
    }

    //
    // Attempt to deep-copy over all the package elements from the source to the destination.
    // TODO: Deep-copying is not correct for fields or references!
    //
    for( i = 0; i < Input->u.Package->ElementCount; i++ ) {
        Element = Output->u.Package->Elements[ i ];
        InputElement = Input->u.Package->Elements[ i ];
        if( ( Element == NULL ) || ( InputElement == NULL ) ) {
            return AML_FALSE;
        }

        //
        // Only copy the element if the source element is initialized,
        // AmlConvObjectStore does not support stores with a NONE source.
        //
        if( InputElement->Value.Type != AML_DATA_TYPE_NONE ) {
            AmlDataFree( &Element->Value );
            if( AmlConvObjectStore( State, Heap, &InputElement->Value, &Element->Value, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
                return AML_FALSE;
            }
        }

        //
        // Debug output for package element types that shouldn't be possible.
        //
        switch( InputElement->Value.Type ) {
        case AML_DATA_TYPE_NONE:
        case AML_DATA_TYPE_DEBUG:
        case AML_DATA_TYPE_PACKAGE_ELEMENT:
        case AML_DATA_TYPE_FIELD_UNIT:
            AML_DEBUG_ERROR( State, "Error: Invalid package element type, report this!\n" );
            return AML_FALSE;
        default:
            break;
        }
    }

    return AML_TRUE;
}

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
    )
{
    AML_OBJECT*          RefObject;
    AML_PACKAGE_ELEMENT* PackageElement;

    //
    // Output to references to certain types are treated transparently (field units and package elements).
    //
    if( ( Output->Type == AML_DATA_TYPE_REFERENCE )
        && ( Output->u.Reference.Object != NULL ) )
    {
        RefObject = Output->u.Reference.Object;
        if( RefObject->Type == AML_OBJECT_TYPE_NAME ) {
            switch( RefObject->u.Name.Value.Type ) {
            case AML_DATA_TYPE_PACKAGE_ELEMENT:
            case AML_DATA_TYPE_FIELD_UNIT:
                Output = &RefObject->u.Name.Value;
                break;
            default:
                break;
            }
        }
    }

    //
    // Output to package elements are treated transparently.
    // Lookup the referenced package element entry, and pass through the store to the element value.
    //
    if( Output->Type == AML_DATA_TYPE_PACKAGE_ELEMENT ) {
        PackageElement = AmlPackageDataLookupElement( Output->u.PackageElement.Package, Output->u.PackageElement.ElementIndex );
        if( PackageElement == NULL ) {
            return AML_FALSE;
        }
        Output = &PackageElement->Value;
    }

    //
    // Dispatch to the handler for the given input type.
    //
    switch( Input->Type ) {
    case AML_DATA_TYPE_NONE:
        AML_DEBUG_ERROR( State, "Error: Uninitialized store source operand.\n" );
        return AML_FALSE;
    case AML_DATA_TYPE_BUFFER:
        return AmlConvBufferStore( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_STRING:
        return AmlConvStringStore( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_INTEGER:
        return AmlConvIntegerStore( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_REFERENCE:
        return AmlConvReferenceStore( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_FIELD_UNIT:
        return AmlConvFieldUnitStore( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_PACKAGE_ELEMENT:
        return AmlConvPackageElementStore( State, Heap, Input, Output, ConvFlags );
    case AML_DATA_TYPE_PACKAGE:
        return AmlConvPackageStore( State, Heap, Input, Output, ConvFlags );
    default:
        break;
    }

    return AML_FALSE;
}

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
    )
{
    AML_DATA NewData;

    //
    // Don't apply any conversion if conversion type is none.
    //
    if( ConversionType == AML_DATA_TYPE_NONE ) {
        return AML_TRUE;
    }

    //
    // Set up new data for conversion result.
    //
    NewData = ( AML_DATA ){ .Type = ConversionType };

    //
    // Attempt to convert the input data to the given type.
    //
    if( AmlConvObjectStore( State, Heap, Data, &NewData, ConvFlags ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Free old data.
    //
    AmlDataFree( Data );

    //
    // Update data with new converted value.
    //
    *Data = NewData;
    return AML_TRUE;
}
#include "aml_state.h"
#include "aml_decoder.h"
#include "aml_object.h"
#include "aml_conv.h"
#include "aml_mutex.h"
#include "aml_debug.h"
#include "aml_base.h"
#include "aml_buffer_field.h"
#include "aml_field.h"

//
// Perform a BufferAcc field read with special semantics.
//
_Success_( return )
static
BOOLEAN
AmlFieldBufferAccRead(
    _Inout_                              struct _AML_STATE* State,
    _In_                                 AML_OBJECT_FIELD*  Field,
    _Out_writes_bytes_( ResultDataSize ) VOID*              ResultData,
    _In_                                 SIZE_T             ResultDataSize,
    _In_                                 BOOLEAN            AllowTruncation,
    _Out_opt_                            SIZE_T*            pResultBitCount
    )
{
    AML_OBJECT_OPERATION_REGION* OpRegion;
    SIZE_T                       MaxPacketSize;
    SIZE_T                       i;
    SIZE_T                       OutputLength;

    //
    // Validate the field's referenced operation region object.
    //
    if( ( Field->OperationRegion == NULL )
        || ( Field->OperationRegion->Type != AML_OBJECT_TYPE_OPERATION_REGION ) )
    {
        return AML_FALSE;
    }
    OpRegion = &Field->OperationRegion->u.OpRegion;

    //
    // The result buffer must be large enough to contain a full packet for the region space type.
    //
    switch( OpRegion->SpaceType ) {
    case AML_REGION_SPACE_TYPE_SMBUS:
        MaxPacketSize = sizeof( AML_REGION_ACCESS_DATA_SMBUS );
        break;
    case AML_REGION_SPACE_TYPE_IPMI:
        MaxPacketSize = sizeof( AML_REGION_ACCESS_DATA_IPMI );
        break;
    case AML_REGION_SPACE_TYPE_GENERIC_SERIAL_BUS:
        MaxPacketSize = sizeof( AML_REGION_ACCESS_DATA_GSB );
        break;
    default:
        AML_DEBUG_ERROR( State, "Error: Unsupported BufferAcc region space type: 0x%x\n", ( UINT )OpRegion->SpaceType );
        return AML_FALSE;
    }

    //
    // Valdiate the output result buffer size for the packet type of the operation region.
    //
    if( ResultDataSize < MaxPacketSize ) {
        return AML_FALSE;
    }

    //
    // Default initialize the entire output buffer as a precaution.
    //
    for( i = 0; i < ResultDataSize; i++ ) {
        ( ( UINT8* )ResultData )[ i ] = 0;
    }

    //
    // Directly pass the packet along to the operation region access handler,
    // no chunked access is performed, and access must always be within the maximum buffer size of the type.
    //
    if( AmlOperationRegionRead( State,
                                OpRegion,
                                Field,
                                Field->Offset,
                                Field->Element.AccessType,
                                Field->Element.AccessAttributes,
                                ( MaxPacketSize * CHAR_BIT ),
                                ResultData,
                                ResultDataSize ) == AML_FALSE )
    {
        return AML_FALSE;
    }

    //
    // Optionally convert the packet-specific length field to an amount of bytes outputted.
    //
    if( pResultBitCount != NULL ) {
        switch( OpRegion->SpaceType ) {
        case AML_REGION_SPACE_TYPE_SMBUS:
        case AML_REGION_SPACE_TYPE_IPMI:
        case AML_REGION_SPACE_TYPE_GENERIC_SERIAL_BUS:
            OutputLength = ( 2 + ( ( UINT8* )ResultData )[ 1 ] );
            if( OutputLength > ResultDataSize ) {
                AML_DEBUG_PANIC( State, "Fatal: BufferAcc region read handler returned an invalid packet length (0x%x)!\n", ( UINT )OutputLength );
                return AML_FALSE;
            }
            *pResultBitCount = ( OutputLength * CHAR_BIT );
            break;
        default:
            *pResultBitCount = MaxPacketSize;
            break;
        }
    }

    return AML_TRUE;
}

//
// The given ResultDataSize is the max size of the ResultData array in *bytes*.
//
_Success_( return )
static
BOOLEAN
AmlFieldRead(
    _Inout_                              struct _AML_STATE* State,
    _In_                                 AML_OBJECT_FIELD*  Field,
    _Out_writes_bytes_( ResultDataSize ) VOID*              ResultData,
    _In_                                 SIZE_T             ResultDataSize,
    _In_                                 BOOLEAN            AllowTruncation,
    _Out_opt_                            SIZE_T*            pResultBitCount
    )
{
    UINT64                       AccessBitWidth;
    AML_OBJECT_OPERATION_REGION* OpRegion;
    UINT64                       AlignedFieldByteCount;
    UINT64                       i;
    UINT64                       OutputBitCount;
    UINT64                       ByteIndex;
    UINT8                        WordData[ 8 ];
    UINT64                       WordBitOffset;
    UINT64                       WordBitCount;

    //
    // BufferAcc access types require different semantics/handling.
    //
    if( Field->Element.AccessType == AML_FIELD_ACCESS_TYPE_BUFFER_ACC ) {
        return AmlFieldBufferAccRead( State, Field, ResultData, ResultDataSize, AllowTruncation, pResultBitCount );
    }

    //
    // Validate the field's referenced operation region object.
    //
    if( ( Field->OperationRegion == NULL )
        || ( Field->OperationRegion->Type != AML_OBJECT_TYPE_OPERATION_REGION ) )
    {
        return AML_FALSE;
    }
    OpRegion = &Field->OperationRegion->u.OpRegion;

    //
    // Ensure that the output buffer is big enough to fit the field data.
    // If truncation is enabled, truncate the field data size if not big enough.
    //
    OutputBitCount = Field->Element.Length;
    if( OutputBitCount > ( ResultDataSize * CHAR_BIT ) ) {
        if( AllowTruncation == AML_FALSE ) {
            return AML_FALSE;
        }
        OutputBitCount = ( ResultDataSize * CHAR_BIT );
    }

    //
    // Determine the bit-width for the given access type.
    //
    switch( Field->Element.AccessType ) {
    case AML_FIELD_ACCESS_TYPE_ANY_ACC:
        AccessBitWidth = 8;
        break;
    case AML_FIELD_ACCESS_TYPE_BYTE_ACC:
        AccessBitWidth = 8;
        break;
    case AML_FIELD_ACCESS_TYPE_WORD_ACC:
        AccessBitWidth = 16;
        break;
    case AML_FIELD_ACCESS_TYPE_DWORD_ACC:
        AccessBitWidth = 32;
        break;
    case AML_FIELD_ACCESS_TYPE_QWORD_ACC:
        AccessBitWidth = 64;
        break;
    default:
        return AML_FALSE;
    }

    //
    // Ensure that the region referenced by the field is large enough to contain the field's data.
    //
    AlignedFieldByteCount = ( ( ( Field->Element.Length + ( AccessBitWidth - 1 ) ) & ~( AccessBitWidth - 1 ) ) / CHAR_BIT );
    if( ( AlignedFieldByteCount > OpRegion->Length )
        || ( ( Field->Offset / CHAR_BIT ) > ( OpRegion->Length - AlignedFieldByteCount ) ) )
    {
        return AML_FALSE;
    }

    //
    // Zero out entire result by default, leaving any extra unread bits as 0.
    //
    for( i = 0; i < ResultDataSize; i++ ) {
        ( ( CHAR* )ResultData )[ i ] = 0;
    }

    //
    // Attempt to read in chunks of the access width until we have received the full field value.
    //
    for( i = 0; i < OutputBitCount; i += WordBitCount ) {
        //
        // Read an access-word worth of data from the current (word) offset.
        //
        ByteIndex = ( ( ( Field->Offset + i ) & ~( AccessBitWidth - 1 ) ) / CHAR_BIT );
        if( AmlOperationRegionRead( State,
                                    OpRegion,
                                    Field,
                                    ByteIndex,
                                    Field->Element.AccessType,
                                    Field->Element.AccessAttributes,
                                    AccessBitWidth,
                                    WordData,
                                    sizeof( WordData ) ) == AML_FALSE )
        {
            return AML_FALSE;
        }

        //
        // Copy the desired bits of the read word to the output buffer.
        //
        WordBitOffset = ( ( Field->Offset + i ) % AccessBitWidth );
        WordBitCount = AML_MIN( ( AccessBitWidth - WordBitOffset ), ( OutputBitCount - i ) );
        if( AmlCopyBits( WordData, sizeof( WordData ), ResultData,
                         ResultDataSize, WordBitOffset, WordBitCount, i ) == AML_FALSE )
        {
            return AML_FALSE;
        }
    }

    //
    // Return the read amount of bits.
    //
    if( pResultBitCount != NULL ) {
        *pResultBitCount = OutputBitCount;
    }

    return AML_TRUE;
}

//
// Perform a BufferAcc field write with special semantics.
//
_Success_( return )
static
BOOLEAN
AmlFieldBufferAccWrite(
    _Inout_                           struct _AML_STATE* State,
    _In_                              AML_OBJECT_FIELD*  Field,
    _In_reads_bytes_( InputDataSize ) const VOID*        InputData,
    _In_                              SIZE_T             InputDataSize,
    _In_                              BOOLEAN            AllowTruncation
    )
{
    AML_OBJECT_OPERATION_REGION* OpRegion;
    AML_REGION_ACCESS_DATA       Packet;
    SIZE_T                       PacketSize;

    //
    // Validate the field's referenced operation region object.
    //
    if( ( Field->OperationRegion == NULL )
        || ( Field->OperationRegion->Type != AML_OBJECT_TYPE_OPERATION_REGION ) )
    {
        return AML_FALSE;
    }
    OpRegion = &Field->OperationRegion->u.OpRegion;

    //
    // All supported BufferAcc payloads require a 2 byte header (status, length).
    //
    if( ( InputDataSize < 2 ) || ( InputDataSize > sizeof( Packet ) ) ) {
        return AML_FALSE;
    }

    //
    // Handle supported BufferAcc region space types.
    //
    switch( OpRegion->SpaceType ) {
    case AML_REGION_SPACE_TYPE_SMBUS:
        PacketSize = ( 2 + ( ( UINT8* )InputData )[ 1 ] );
        if( ( PacketSize > sizeof( Packet.SmBus ) ) || ( PacketSize > InputDataSize ) ) {
            return AML_FALSE;
        }
        AML_MEMCPY( &Packet.SmBus, InputData, PacketSize );
        break;
    case AML_REGION_SPACE_TYPE_IPMI:
        PacketSize = ( 2 + ( ( UINT8* )InputData )[ 1 ] );
        if( ( PacketSize > sizeof( Packet.Ipmi ) ) || ( PacketSize > InputDataSize ) ) {
            return AML_FALSE;
        }
        AML_MEMCPY( &Packet.Ipmi, InputData, PacketSize );
        break;
    case AML_REGION_SPACE_TYPE_GENERIC_SERIAL_BUS:
        PacketSize = ( 2 + ( ( UINT8* )InputData )[ 1 ] );
        if( ( PacketSize > sizeof( Packet.Gsb ) ) || ( PacketSize > InputDataSize ) ) {
            return AML_FALSE;
        }
        AML_MEMCPY( &Packet.Gsb, InputData, PacketSize );
        break;
    default:
        AML_DEBUG_ERROR( State, "Error: Unsupported BufferAcc region space type: 0x%x\n", ( UINT )OpRegion->SpaceType );
        return AML_FALSE;
    }

    //
    // Directly pass the packet along to the operation region access handler,
    // no chunked access is performed, and access must always be within the maximum buffer size of the type.
    //
    return AmlOperationRegionWrite( State,
                                    OpRegion,
                                    Field,
                                    Field->Offset,
                                    Field->Element.AccessType,
                                    Field->Element.AccessAttributes,
                                    ( PacketSize * CHAR_BIT ),
                                    &Packet,
                                    PacketSize );
}

//
// Write data to the given field from the input data array.
// The maximum input data size is passed in bytes, but the actual copy to the field is done at bit-granularity.
// AllowTruncation has different semantics for field writes, if enabled, when writing to a field that is
// greater in size than the input data, the upper bits of the field will be written as 0, instead of following
// the update rule of the field!
//
_Success_( return )
static
BOOLEAN
AmlFieldWrite(
    _Inout_                           struct _AML_STATE* State,
    _In_                              AML_OBJECT_FIELD*  Field,
    _In_reads_bytes_( InputDataSize ) const VOID*        InputData,
    _In_                              SIZE_T             InputDataSize,
    _In_                              BOOLEAN            AllowTruncation
    )
{
    UINT64                       AccessBitWidth;
    UINT64                       AccessByteWidth;
    AML_OBJECT_OPERATION_REGION* OpRegion;
    UINT64                       AlignedFieldByteCount;
    UINT64                       WordData;
    UINT64                       i;
    UINT64                       RegionByteIndex;
    UINT64                       WordBitCount;
    UINT64                       WordBitOffset;
    UINT64                       FieldWidthMask;
    UINT64                       AccessWidthMask;
    UINT64                       WordMask;
    UINT64                       InputWordData;
    UINT64                       InputBitCount;

    //
    // BufferAcc access types require different semantics/handling.
    //
    if( Field->Element.AccessType == AML_FIELD_ACCESS_TYPE_BUFFER_ACC ) {
        return AmlFieldBufferAccWrite( State, Field, InputData, InputDataSize, AllowTruncation );
    }

    //
    // Validate the field's referenced operation region object.
    //
    if( ( Field->OperationRegion == NULL )
        || ( Field->OperationRegion->Type != AML_OBJECT_TYPE_OPERATION_REGION ) )
    {
        return AML_FALSE;
    }
    OpRegion = &Field->OperationRegion->u.OpRegion;

    //
    // These region space types must only use BufferAcc access type, as they require special buffered semantics.
    //
    switch( OpRegion->SpaceType ) {
    case AML_REGION_SPACE_TYPE_SMBUS:
    case AML_REGION_SPACE_TYPE_IPMI:
    case AML_REGION_SPACE_TYPE_GENERIC_SERIAL_BUS:
        AML_DEBUG_ERROR( State, "Error: Invalid access type for BufferAcc region\n" );
        return AML_FALSE;
    }

    //
    // Determine the properties for the given access type.
    //
    switch( Field->Element.AccessType ) {
    case AML_FIELD_ACCESS_TYPE_ANY_ACC:
        AccessBitWidth = 8;
        break;
    case AML_FIELD_ACCESS_TYPE_BYTE_ACC:
        AccessBitWidth = 8;
        break;
    case AML_FIELD_ACCESS_TYPE_WORD_ACC:
        AccessBitWidth = 16;
        break;
    case AML_FIELD_ACCESS_TYPE_DWORD_ACC:
        AccessBitWidth = 32;
        break;
    case AML_FIELD_ACCESS_TYPE_QWORD_ACC:
        AccessBitWidth = 64;
        break;
    default:
        return AML_FALSE;
    }

    //
    // Byte-size of the access word width, used when interacting with the underlying operation region.
    //
    AccessByteWidth = ( AccessBitWidth / CHAR_BIT );

    //
    // Ensure that the region referenced by the field is large enough to contain the field's data.
    //
    AlignedFieldByteCount = ( ( ( Field->Element.Length + ( AccessBitWidth - 1 ) ) & ~( AccessBitWidth - 1 ) ) / CHAR_BIT );
    if( ( AlignedFieldByteCount > OpRegion->Length )
        || ( ( Field->Offset / CHAR_BIT ) > ( OpRegion->Length - AlignedFieldByteCount ) ) )
    {
        return AML_FALSE;
    }

    //
    // TODO: Handle truncation option?
    //
    // if( Field->Length > ( InputDataSize * CHAR_BIT ) ) {
    // 	return AML_FALSE;
    // }

    //
    // Attempt to write in chunks of the access width until we have written the full field value.
    //
    for( i = 0; i < Field->Element.Length; i += WordBitCount ) {
        //
        // Calculate the access-word-aligned byte index of the current word.
        //
        RegionByteIndex = ( ( ( Field->Offset + i ) & ~( AccessBitWidth - 1 ) ) / CHAR_BIT );

        //
        // Calculate the word-local bit offset and count, and determine the amount of input bits to use in this cycle.
        // It is possible that there are no more input bits left, but we still need to update the rest of the word.
        //
        WordBitOffset = ( ( Field->Offset + i ) % AccessBitWidth );
        WordBitCount = AML_MIN( ( Field->Element.Length - i ), ( AccessBitWidth - WordBitOffset ) );
        if( i < ( InputDataSize * CHAR_BIT ) ) {
            InputBitCount = AML_MIN( WordBitCount, ( ( InputDataSize * CHAR_BIT ) - i ) );
        } else {
            InputBitCount = 0;
        }

        //
        // Set up the bit values for word bits that don't belong to the current field.
        //
        switch( AML_FIELD_FLAGS_UPDATE_RULE_GET( Field->Flags ) ) {
        case AML_FIELD_UPDATE_RULE_PRESERVE:
            //
            // In preserve access mode, we must preserve any of the original unmodified bits of the word.
            //
            WordData = 0;
            if( AllowTruncation == AML_FALSE ) {
                if( ( ( ( Field->Offset + i ) & ( AccessBitWidth - 1 ) ) != 0 )
                    || ( ( Field->Element.Length - i ) < AccessBitWidth ) )
                {
                    if( AmlOperationRegionRead( State,
                                                OpRegion,
                                                Field,
                                                RegionByteIndex,
                                                Field->Element.AccessType,
                                                Field->Element.AccessAttributes,
                                                AccessBitWidth,
                                                &WordData,
                                                AccessByteWidth ) == AML_FALSE )
                    {
                        return AML_FALSE;
                    }
                }
            }
            break;
        case AML_FIELD_UPDATE_RULE_WRITE_AS_ONES:
            WordData = ~0ull;
            break;
        default:
            WordData = 0;
            break;
        }

        //
        // The word mask is the shifted mask of the word bits updated by this field write cycle.
        //
        AccessWidthMask = ( ( 1ull << ( AccessBitWidth - 1 ) ) | ( ( 1ull << ( AccessBitWidth - 1 ) ) - 1 ) );
        FieldWidthMask = ( ( 1ull << ( WordBitCount - 1 ) ) | ( ( 1ull << ( WordBitCount - 1 ) ) - 1 ) );
        WordMask = ( ( FieldWidthMask & AccessWidthMask ) << WordBitOffset );

        //
        // Read the input data for this write cycle.
        //
        InputWordData = 0;
        if( i < ( InputDataSize * CHAR_BIT ) ) {
            if( AmlCopyBits( InputData, InputDataSize, &InputWordData,
                             sizeof( InputWordData ), i, InputBitCount, 0 ) == AML_FALSE )
            {
                return AML_FALSE;
            }
        }

        //
        // Update the portion of the word being updated in this cycle,
        // leaves other bit values that depend on the update rule.
        //
        WordData &= ~WordMask;
        WordData |= ( ( InputWordData << WordBitOffset ) & WordMask );

        //
        // Perform the actual write to the operation region word.
        //
        if( AmlOperationRegionWrite( State,
                                     OpRegion,
                                     Field,
                                     RegionByteIndex,
                                     Field->Element.AccessType,
                                     Field->Element.AccessAttributes,
                                     AccessBitWidth,
                                     &WordData,
                                     AccessByteWidth ) == AML_FALSE )
        {
            return AML_FALSE;
        }
    }

    return AML_TRUE;
}

//
// The given ResultDataSize is the max size of the ResultData array in *bytes*.
//
_Success_( return )
static
BOOLEAN
AmlBankFieldRead(
    _Inout_                              struct _AML_STATE*     State,
    _In_                                 AML_OBJECT_BANK_FIELD* Field,
    _Out_writes_bytes_( ResultDataSize ) VOID*                  ResultData,
    _In_                                 SIZE_T                 ResultDataSize,
    _In_                                 BOOLEAN                AllowTruncation,
    _Out_opt_                            SIZE_T*                pResultBitCount
    )
{
    AML_DATA BankData;

    //
    // Write the bank value to the bank field, selecting the underlying banked register to access.
    //
    BankData = ( AML_DATA ){ .Type = AML_DATA_TYPE_FIELD_UNIT, .u.FieldUnit = Field->Bank };
    if( AmlConvIntegerToFieldUnit( State, &Field->BankValue, &BankData, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
        return AML_FALSE;
    }
    return AmlFieldRead( State, &Field->Base, ResultData, ResultDataSize, AllowTruncation, pResultBitCount );
}

//
// Write data to the given field from the input data array.
// The maximum input data size is passed in bytes, but the actual copy to the field is done at bit-granularity.
//
_Success_( return )
static
BOOLEAN
AmlBankFieldWrite(
    _Inout_                           struct _AML_STATE*     State,
    _In_                              AML_OBJECT_BANK_FIELD* Field,
    _In_reads_bytes_( InputDataSize ) const VOID*            InputData,
    _In_                              SIZE_T                 InputDataSize,
    _In_                              BOOLEAN                AllowTruncation
    )
{
    AML_DATA BankData;

    //
    // Write the bank value to the bank field, selecting the underlying banked register to access.
    //
    BankData = ( AML_DATA ){ .Type = AML_DATA_TYPE_FIELD_UNIT, .u.FieldUnit = Field->Bank };
    if( AmlConvIntegerToFieldUnit( State, &Field->BankValue, &BankData, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
        return AML_FALSE;
    }
    return AmlFieldWrite( State, &Field->Base, InputData, InputDataSize, AllowTruncation );
}

//
// The given ResultDataSize is the max size of the ResultData array in *bytes*.
//
_Success_( return )
static
BOOLEAN
AmlIndexFieldRead(
    _Inout_                              struct _AML_STATE*      State,
    _In_                                 AML_OBJECT_INDEX_FIELD* Field,
    _Out_writes_bytes_( ResultDataSize ) VOID*                   ResultData,
    _In_                                 SIZE_T                  ResultDataSize,
    _In_                                 BOOLEAN                 AllowTruncation,
    _Out_opt_                            SIZE_T*                 pResultBitCount
    )
{
    AML_DATA IndexValue;
    AML_DATA IndexField;

    //
    // Write the offset to the index register before performing the actual read.
    // The value written to the IndexName register is defined to be a byte offset that is aligned on an AccessType boundary
    //
    IndexValue = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = ( Field->Offset / 8 ) };
    IndexField = ( AML_DATA ){ .Type = AML_DATA_TYPE_FIELD_UNIT, .u.FieldUnit = Field->Index };
    if( AmlConvIntegerToFieldUnit( State, &IndexValue, &IndexField, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
        return AML_FALSE;
    }
    return AmlFieldUnitRead( State, Field->Data, ResultData, ResultDataSize, AllowTruncation, pResultBitCount );
}

//
// Write data to the given field from the input data array.
// The maximum input data size is passed in bytes, but the actual copy to the field is done at bit-granularity.
//
_Success_( return )
static
BOOLEAN
AmlIndexFieldWrite(
    _Inout_                           struct _AML_STATE*      State,
    _In_                              AML_OBJECT_INDEX_FIELD* Field,
    _In_reads_bytes_( InputDataSize ) const VOID*             InputData,
    _In_                              SIZE_T                  InputDataSize,
    _In_                              BOOLEAN                 AllowTruncation
    )
{
    AML_DATA IndexValue;
    AML_DATA IndexField;

    //
    // Write the offset to the index register before performing the actual write.
    //
    IndexValue = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = ( Field->Offset / 8 ) };
    IndexField = ( AML_DATA ){ .Type = AML_DATA_TYPE_FIELD_UNIT, .u.FieldUnit = Field->Index };
    if( AmlConvIntegerToFieldUnit( State, &IndexValue, &IndexField, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
        return AML_FALSE;
    }
    return AmlFieldUnitWrite( State, Field->Data, InputData, InputDataSize, AllowTruncation );
}

//
// Read data from a field unit object (field, buffer field, index field, bank field).
//
_Success_( return )
BOOLEAN
AmlFieldUnitRead(
    _Inout_                              struct _AML_STATE*  State,
    _Inout_                              struct _AML_OBJECT* FieldObject,
    _Out_writes_bytes_( ResultDataSize ) VOID*               ResultData,
    _In_                                 SIZE_T              ResultDataSize,
    _In_                                 BOOLEAN             AllowTruncation,
    _Out_opt_                            SIZE_T*             pResultBitCount
    )
{
    BOOLEAN Success;

    switch( FieldObject->Type ) {
    case AML_OBJECT_TYPE_BUFFER_FIELD:
        return AmlBufferFieldRead( State, &FieldObject->u.BufferField, ResultData,
                                   ResultDataSize, AllowTruncation, pResultBitCount );
    case AML_OBJECT_TYPE_FIELD:
        if( AML_FIELD_FLAGS_LOCK_RULE_GET( FieldObject->u.Field.Flags ) ) {
            if( AmlMutexAcquire( State, State->GlobalLock, 0xFFFF ) != AML_WAIT_STATUS_SUCCESS ) {
                return AML_FALSE;
            }
        }
        Success = AmlFieldRead( State, &FieldObject->u.Field, ResultData,
                                ResultDataSize, AllowTruncation, pResultBitCount );
        if( AML_FIELD_FLAGS_LOCK_RULE_GET( FieldObject->u.Field.Flags ) ) {
            AmlMutexRelease( State, State->GlobalLock );
        }
        return Success;
    case AML_OBJECT_TYPE_BANK_FIELD:
        if( AML_FIELD_FLAGS_LOCK_RULE_GET( FieldObject->u.BankField.Base.Flags ) ) {
            if( AmlMutexAcquire( State, State->GlobalLock, 0xFFFF ) != AML_WAIT_STATUS_SUCCESS ) {
                return AML_FALSE;
            }
        }
        Success = AmlBankFieldRead( State, &FieldObject->u.BankField, ResultData,
                                    ResultDataSize, AllowTruncation, pResultBitCount );
        if( AML_FIELD_FLAGS_LOCK_RULE_GET( FieldObject->u.BankField.Base.Flags ) ) {
            AmlMutexRelease( State, State->GlobalLock );
        }
        return Success;
    case AML_OBJECT_TYPE_INDEX_FIELD:
        if( AML_FIELD_FLAGS_LOCK_RULE_GET( FieldObject->u.IndexField.Flags ) ) {
            if( AmlMutexAcquire( State, State->GlobalLock, 0xFFFF ) != AML_WAIT_STATUS_SUCCESS ) {
                return AML_FALSE;
            }
        }
        Success = AmlIndexFieldRead( State, &FieldObject->u.IndexField, ResultData,
                                     ResultDataSize, AllowTruncation, pResultBitCount );
        if( AML_FIELD_FLAGS_LOCK_RULE_GET( FieldObject->u.IndexField.Flags ) ) {
            AmlMutexRelease( State, State->GlobalLock );
        }
        return Success;
    default:
        AML_DEBUG_ERROR( State, "Error: Unsupported field unit type!\n" );
        break;
    }

    return AML_FALSE;
}

//
// Write data to a field unit object (field, buffer field, index field, bank field).
// AllowTruncation has different semantics for OpRegion field writes, if enabled, when writing to a field that is
// greater in size than the input data, the upper bits of the field will be written as 0, instead of following
// the update rule of the field!
//
_Success_( return )
BOOLEAN
AmlFieldUnitWrite(
    _Inout_                           struct _AML_STATE*  State,
    _Inout_                           struct _AML_OBJECT* FieldObject,
    _In_reads_bytes_( InputDataSize ) const VOID*         InputData,
    _In_                              SIZE_T              InputDataSize,
    _In_                              BOOLEAN             AllowTruncation
    )
{
    BOOLEAN Success;

    switch( FieldObject->Type ) {
    case AML_OBJECT_TYPE_BUFFER_FIELD:
        return AmlBufferFieldWrite( State, &FieldObject->u.BufferField,
                                    InputData, InputDataSize, AllowTruncation );
    case AML_OBJECT_TYPE_FIELD:
        if( AML_FIELD_FLAGS_LOCK_RULE_GET( FieldObject->u.Field.Flags ) ) {
            if( AmlMutexAcquire( State, State->GlobalLock, 0xFFFF ) != AML_WAIT_STATUS_SUCCESS ) {
                return AML_FALSE;
            }
        }
        Success = AmlFieldWrite( State, &FieldObject->u.Field, InputData, InputDataSize, AllowTruncation );
        if( AML_FIELD_FLAGS_LOCK_RULE_GET( FieldObject->u.Field.Flags ) ) {
            AmlMutexRelease( State, State->GlobalLock );
        }
        return Success;
    case AML_OBJECT_TYPE_BANK_FIELD:
        if( AML_FIELD_FLAGS_LOCK_RULE_GET( FieldObject->u.BankField.Base.Flags ) ) {
            if( AmlMutexAcquire( State, State->GlobalLock, 0xFFFF ) != AML_WAIT_STATUS_SUCCESS ) {
                return AML_FALSE;
            }
        }
        Success = AmlBankFieldWrite( State, &FieldObject->u.BankField, InputData, InputDataSize, AllowTruncation );
        if( AML_FIELD_FLAGS_LOCK_RULE_GET( FieldObject->u.BankField.Base.Flags ) ) {
            AmlMutexRelease( State, State->GlobalLock );
        }
        return Success;
    case AML_OBJECT_TYPE_INDEX_FIELD:
        if( AML_FIELD_FLAGS_LOCK_RULE_GET( FieldObject->u.IndexField.Flags ) ) {
            if( AmlMutexAcquire( State, State->GlobalLock, 0xFFFF ) != AML_WAIT_STATUS_SUCCESS ) {
                return AML_FALSE;
            }
        }
        Success = AmlIndexFieldWrite( State, &FieldObject->u.IndexField, InputData, InputDataSize, AllowTruncation );
        if( AML_FIELD_FLAGS_LOCK_RULE_GET( FieldObject->u.IndexField.Flags ) ) {
            AmlMutexRelease( State, State->GlobalLock );
        }
        return Success;
    default:
        AML_DEBUG_ERROR( State, "Error: Unsupported field unit type!\n" );
        break;
    }

    return AML_FALSE;
}
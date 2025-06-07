#include "aml_state.h"
#include "aml_decoder.h"
#include "aml_debug.h"
#include "aml_object.h"
#include "aml_namespace.h"
#include "aml_conv.h"
#include "aml_compare.h"

//
// Pull in generated decoder tables, they are static and should only be included in this file.
// Them being static and only referenced may allow for further potential optimization by the compiler.
//
#include "aml_decoder_tables.inc.h"

//
// Our internal AML interpreter revision number, returned by RevisionOp.
//
#define AML_INTERPRETER_REVISION 1

//
// Peek a single byte of input data.
//
_Success_( return )
BOOLEAN
AmlDecoderPeekByte(
    _In_  const AML_STATE* State,
    _In_  SIZE_T           LookaheadOffset,
    _Out_ UINT8*           pPeekByte
    )
{
    if( State->DataCursor >= State->DataLength || LookaheadOffset >= ( State->DataLength - State->DataCursor ) ) {
        return AML_FALSE;
    }
    *pPeekByte = State->Data[ State->DataCursor + LookaheadOffset ];
    return AML_TRUE;
}

//
// Consume a single byte of input data.
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeByte(
    _Inout_   AML_STATE* State,
    _Out_opt_ UINT8*     pConsumedByte
    )
{
    SIZE_T Cursor;

    if( State->DataCursor >= State->DataLength ) {
        return AML_FALSE;
    }
    Cursor = State->DataCursor++;
    if( pConsumedByte != NULL ) {
        *pConsumedByte = State->Data[ Cursor ];
    }

    return AML_TRUE;
}

//
// Consume a span of input data bytes.
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeByteSpan(
    _Inout_                                 AML_STATE* State,
    _Out_writes_bytes_all_opt_( ByteCount ) UINT8*     ConsumedByteSpan,
    _In_                                    SIZE_T     ByteCount
    )
{
    SIZE_T i;

    if( State->DataCursor >= State->DataLength || ByteCount > ( State->DataLength - State->DataCursor ) ) {
        return AML_FALSE;
    }
    if( ConsumedByteSpan != NULL ) {
        for( i = 0; i < ByteCount; i++ ) {
            ConsumedByteSpan[ i ] = State->Data[ State->DataCursor + i ];
        }
    }
    State->DataCursor += ByteCount;

    return AML_TRUE;
}

//
// Consume the next input data byte if it matches the given byte.
//
BOOLEAN
AmlDecoderMatchByte(
    _Inout_ AML_STATE* State,
    _In_    UINT8      MatchByte
    )
{
    UINT8 PeekByte;

    if( AmlDecoderPeekByte( State, 0, &PeekByte ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( PeekByte == MatchByte ) {
        return AmlDecoderConsumeByte( State, NULL );
    }
    return AML_FALSE;
}

//
// WordData := ByteData[0:7] ByteData[8:15]
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeWord(
    _Inout_   AML_STATE* State,
    _Out_opt_ UINT16*    pConsumedWord
    )
{
    UINT8 ByteData[ 2 ];

    //
    // Consume two bytes.
    //
    if( AmlDecoderConsumeByteSpan( State, ByteData, sizeof( ByteData ) ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // WordData := ByteData[0:7] ByteData[8:15]
    //
    if( pConsumedWord != NULL ) {
        *pConsumedWord = ( ( ( UINT16 )ByteData[ 1 ] << 8 ) | ByteData[ 0 ] );
    }

    return AML_TRUE;
}

//
// DWordData := WordData[0:15] WordData[16:31]
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeDWord(
    _Inout_   AML_STATE* State,
    _Out_opt_ UINT32*    pConsumedDWord
    )
{
    UINT8 ByteData[ 4 ];

    //
    // Consume four bytes.
    //
    if( AmlDecoderConsumeByteSpan( State, ByteData, sizeof( ByteData ) ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // DWordData := WordData[0:15] WordData[16:31]
    //
    if( pConsumedDWord != NULL ) {
        *pConsumedDWord = ( 
              ( ( UINT32 )ByteData[ 3 ] << ( 8 * 3 ) )
            | ( ( UINT32 )ByteData[ 2 ] << ( 8 * 2 ) )
            | ( ( UINT32 )ByteData[ 1 ] << ( 8 * 1 ) )
            | ByteData[ 0 ]
        );
    }

    return AML_TRUE;
}

//
// QWordData := DWordData[0:31] DWordData[32:63]
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeQWord(
    _Inout_   AML_STATE* State,
    _Out_opt_ UINT64*    pConsumedQWord
    )
{
    UINT8 ByteData[ 8 ];

    //
    // Consume eight bytes.
    //
    if( AmlDecoderConsumeByteSpan( State, ByteData, sizeof( ByteData ) ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // QWordData := DWordData[0:31] DWordData[32:63]
    //
    if( pConsumedQWord != NULL ) {
        *pConsumedQWord = (
              ( ( UINT64 )ByteData[ 7 ] << ( 8 * 7 ) )
            | ( ( UINT64 )ByteData[ 6 ] << ( 8 * 6 ) )
            | ( ( UINT64 )ByteData[ 5 ] << ( 8 * 5 ) )
            | ( ( UINT64 )ByteData[ 4 ] << ( 8 * 4 ) )
            | ( ( UINT64 )ByteData[ 3 ] << ( 8 * 3 ) )
            | ( ( UINT64 )ByteData[ 2 ] << ( 8 * 2 ) )
            | ( ( UINT64 )ByteData[ 1 ] << ( 8 * 1 ) )
            | ByteData[ 0 ]
        );
    }

    return AML_TRUE;
}

//
// Check if the given window is valid (contained by the total data).
//
_Success_( return )
BOOLEAN
AmlDecoderIsValidDataWindow(
    _Inout_ AML_STATE* State,
    _In_    SIZE_T     DataOffset,
    _In_    SIZE_T     DataLength
    )
{
    return ( ( DataOffset <= State->DataTotalLength )
             && ( DataLength <= ( State->DataTotalLength - DataOffset ) ) );
}

//
// Peek information about the next entire opcode (including extended 2-byte opcodes).
//
_Success_( return )
BOOLEAN
AmlDecoderPeekOpcode(
    _In_  const AML_STATE*                State,
    _Out_ AML_DECODER_INSTRUCTION_OPCODE* OpcodeInfo
    )
{
    UINT8                                 ByteL1;
    UINT8                                 ByteL2;
    const AML_OPCODE_TABLE_ENTRY*         OpcodeEntryL1;
    const AML_OPCODE_SUBTABLE_LIST_ENTRY* SubTable;
    const AML_OPCODE_TABLE_ENTRY*         OpcodeEntryL2;
    BOOLEAN                               IsValid;
    BOOLEAN                               Is2ByteInstruction;
    UINT16                                OpcodeID;
    const AML_OPCODE_TABLE_ENTRY*         MainOpcode;

    //
    // Peek the first byte for the L1 opcode of the instruction.
    //
    if( AmlDecoderPeekByte( State, 0, &ByteL1 ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Look up the first byte in the opcode table. 
    //
    OpcodeEntryL1 = &AmlOpcodeL1Table[ ByteL1 % AML_COUNTOF( AmlOpcodeL1Table ) ];
    MainOpcode = OpcodeEntryL1;
    OpcodeID = OpcodeEntryL1->OpcodeByte;
    IsValid = ( OpcodeEntryL1->IsValid & ( OpcodeEntryL1->OpcodeByte == ByteL1 ) );
    Is2ByteInstruction = AML_FALSE;

    //
    // If this is a two-level opcode instruction, look up the second level table entry.
    //
    if( OpcodeEntryL1->SubTableIndex != 0 ) {
        //
        // Lookup the sub-table entry for the 2-level opcode,
        // contains meta information about the linked sub-table,
        // and an actual absolute pointer to its table entries.
        //
        SubTable = &AmlOpcodeL2SubTableList[ OpcodeEntryL1->SubTableIndex % AML_COUNTOF( AmlOpcodeL2SubTableList ) ];
        IsValid &= ( SubTable->IsValid & /* ( SubTable->OpcodeByteL1 == ByteL1 ) & */ ( SubTable->Entries != NULL ) );

        //
        // Attempt to peek the second opcode byte without consuming it yet.
        //
        IsValid &= AmlDecoderPeekByte( State, 1, &ByteL2 );

        //
        // Handle uncommon fatal error.
        // Either the table is invalid (malformed input or tables), or we have ran out of input AML bytecode for this to be valid. 
        //
        if( IsValid == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Lookup the second level opcode within the found sub-table.
        //
        _Analysis_assume_( SubTable->Entries != NULL );
        OpcodeEntryL2 = &SubTable->Entries[ ( ByteL2 % ( 0xFF + 1 ) ) ];

        //
        // Certain 2-byte opcodes are "optionally-matching", they are valid 1-byte instructions,
        // but become valid 2-byte instructions when followed by a certain subset of L2 bytes.
        // An example of this is LNotOp, which is a valid instruction on its own, but becomes 
        // LNotEqualOp when followed by an LEqualOp (same deal with lt and gt comparison ops, but inverts them).
        //
        Is2ByteInstruction = ( ( OpcodeEntryL2->OpcodeByte == ByteL2 ) & OpcodeEntryL2->IsValid );
        if( ( SubTable->IsOptionalMatch == AML_FALSE ) && ( Is2ByteInstruction == AML_FALSE ) ) {
            return AML_FALSE;
        }

        //
        // Form a single instruction ID from the 2 opcode bytes.
        //
        if( Is2ByteInstruction ) {
            MainOpcode = OpcodeEntryL2;
            OpcodeID |= ( ( ( UINT16 )OpcodeEntryL2->OpcodeByte ) << 8 );
        }
    }

    //
    // Return consumed instruction opcode information.
    //
    *OpcodeInfo = ( AML_DECODER_INSTRUCTION_OPCODE ){
        .OpcodeID                  = OpcodeID,
        .Is2ByteOpcode             = Is2ByteInstruction,
        .PrefixL1                  = ( Is2ByteInstruction ? OpcodeEntryL1->OpcodeByte : 0 ),
        .LeafOpcode                = *MainOpcode,
        .IsExpressionOpcode        = ( MainOpcode->IsExpressionOp == 1 ),
        .FixedArgumentTypes        = &AmlOpcodeFixedArgumentListSharedValueTable[ MainOpcode->FixedListArgTableOffset ],
        .FixedArgumentTypeCount    = MainOpcode->FixedListArgCount,
        .VariableArgumentTypes     = &AmlOpcodeVariableArgumentListSharedValueTable[ MainOpcode->VariableListArgTableOffset ],
        .VariableArgumentTypeCount = MainOpcode->VariableListArgCount,
    };

    return IsValid;
}

//
// Consume the full opcode bytes of the next instruction.
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeOpcode(
    _Inout_   AML_STATE*                      State,
    _Out_opt_ AML_DECODER_INSTRUCTION_OPCODE* OpcodeInfo
    )
{
    AML_DECODER_INSTRUCTION_OPCODE PeekOpcode;

    //
    // Peek the next opcode, and figure out the actual size of it.
    //
    if( AmlDecoderPeekOpcode( State, &PeekOpcode ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Consume instruction opcode bytes.
    //
    AmlDecoderConsumeByte( State, NULL );
    if( PeekOpcode.Is2ByteOpcode ) {
        AmlDecoderConsumeByte( State, NULL );
    }

    //
    // Return consumed opcode info if desired.
    //
    if( OpcodeInfo != NULL ) {
        *OpcodeInfo = PeekOpcode;
    }

    return AML_TRUE;
}

//
// Match and consume the next full opcode.
//
_Success_( return )
BOOLEAN
AmlDecoderMatchOpcode(
    _Inout_   AML_STATE*                      State,
    _In_      UINT16                          FullOpcodeID,
    _Out_opt_ AML_DECODER_INSTRUCTION_OPCODE* OpcodeInfo
    )
{
    AML_DECODER_INSTRUCTION_OPCODE PeekOpcode;

    //
    // The next full instruction opcode must match the desired full ID before consuming.
    //
    if( ( AmlDecoderPeekOpcode( State, &PeekOpcode ) == AML_FALSE ) || ( PeekOpcode.OpcodeID != FullOpcodeID ) ) {
        return AML_FALSE;
    }

    //
    // The opcode matches, consume all of it.
    //
    if( OpcodeInfo != NULL ) {
        *OpcodeInfo = PeekOpcode;
    }
    return AmlDecoderConsumeOpcode( State, NULL );
}

// 
// ConstObj := (ZeroOp | OneOp | OnesOp)
// ConstIntegerData := (ByteConst | WordConst | DWordConst | QWordConst | ConstObj).
//
_Success_( return )
BOOLEAN
AmlDecoderMatchConstIntegerData(
    _Inout_   AML_STATE*                      State,
    _Out_     UINT64*                         pValue,
    _Out_opt_ AML_DECODER_INSTRUCTION_OPCODE* pPrefix
    )
{
    AML_DECODER_INSTRUCTION_OPCODE Prefix;
    UINT8                          ByteConst;
    UINT16                         WordConst;
    UINT32                         DWordConst;
    UINT64                         QWordConst;
    UINT64                         Value;
    BOOLEAN                        Success;

    //
    // Peek first instruction.
    //
    if( ( Success = AmlDecoderPeekOpcode( State, &Prefix ) ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // ByteConst := BytePrefix ByteData
    // BytePrefix := 0x0A
    // 
    // WordConst := WordPrefix WordData
    // WordPrefix := 0x0B
    // 
    // DWordConst := DWordPrefix DWordData
    // DWordPrefix := 0x0C
    // 
    // QWordConst := QWordPrefix QWordData
    // QWordPrefix := 0x0E
    // 
    // ConstObj := ZeroOp | OneOp | OnesOp
    // 
    // ByteConst | WordConst | DWordConst | QWordConst | ConstObj
    //
    switch( Prefix.OpcodeID ) {
    case AML_OPCODE_ID_BYTE_PREFIX:
        AmlDecoderConsumeOpcode( State, NULL );
        Success = AmlDecoderConsumeByte( State, &ByteConst );
        Value = ByteConst;
        break;
    case AML_OPCODE_ID_WORD_PREFIX:
        AmlDecoderConsumeOpcode( State, NULL );
        Success = AmlDecoderConsumeWord( State, &WordConst );
        Value = WordConst;
        break;
    case AML_OPCODE_ID_DWORD_PREFIX:
        AmlDecoderConsumeOpcode( State, NULL );
        Success = AmlDecoderConsumeDWord( State, &DWordConst );
        Value = DWordConst;
        break;
    case AML_OPCODE_ID_QWORD_PREFIX:
        AmlDecoderConsumeOpcode( State, NULL );
        Success = AmlDecoderConsumeQWord( State, &QWordConst );
        Value = QWordConst;
        break;
    case AML_OPCODE_ID_ZERO_OP:
        AmlDecoderConsumeOpcode( State, NULL );
        Value = 0;
        break;
    case AML_OPCODE_ID_ONE_OP:
        AmlDecoderConsumeOpcode( State, NULL );
        Value = 1;
        break;
    case AML_OPCODE_ID_ONES_OP:
        AmlDecoderConsumeOpcode( State, NULL );
        Value = ~0ull;
        break;
    default:
        Success = AML_FALSE;
        break;
    }
    
    //
    // Unhandled opcode or invalid input data.
    //
    if( Success == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Return parsed value, and optionally the consumed instruction opcode (prefix).
    //
    *pValue = Value;
    if( pPrefix != NULL ) {
        *pPrefix = Prefix;
    }

    return AML_TRUE;
}

//
// AsciiCharList NullChar
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeAsciiCharListNullTerminated(
    _Inout_   AML_STATE*              State,
    _Out_opt_ AML_OPAQUE_STRING_DATA* StringData
    )
{
    UINT8  Byte;
    SIZE_T StringStart;
    SIZE_T StringLength;

    //
    // The actual ASCII data of the string follows the prefix, until the null terminator.
    //
    Byte = 0;
    StringStart = State->DataCursor;
    StringLength = 0;

    //
    // Consume ASCII characters until we hit a NullChar terminator.
    // AsciiCharList := Nothing | <asciichar asciicharlist>
    // AsciiChar := 0x01 - 0x7F
    // NullChar := 0x00
    // String := StringPrefix AsciiCharList NullChar
    //
    while( AML_TRUE ) {
        if( AmlDecoderConsumeByte( State, &Byte ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( Byte == 0 ) {
            break;
        } else if( ( Byte < 0x01 ) || ( Byte > 0x7f ) ) {
            return AML_FALSE;
        }
        StringLength++;
    }

    //
    // An entire string has been decoded successfully, return optional information.
    //
    if( StringData != NULL ) {
        *StringData = ( AML_OPAQUE_STRING_DATA ){ .Start = StringStart, .DataSize = StringLength };
    }

    return AML_TRUE;
}

//
// String := StringPrefix AsciiCharList NullChar
//
_Success_( return )
BOOLEAN
AmlDecoderMatchStringData(
    _Inout_   AML_STATE*              State,
    _Out_opt_ AML_OPAQUE_STRING_DATA* StringData
    )
{
    AML_DECODER_INSTRUCTION_OPCODE Prefix;

    //
    // Match the StringPrefix, ensures that this is actually a string data object.
    // StringPrefix := 0x0D
    //
    if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_STRING_PREFIX, &Prefix ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // The actual ASCII data of the string follows the prefix, until the null terminator.
    //
    return AmlDecoderConsumeAsciiCharListNullTerminated( State, StringData );
}

//
// Consume VLE PkgLength value.
// PkgLength :=
//     PkgLeadByte |
//     <pkgleadbyte bytedata> |
//     <pkgleadbyte bytedata bytedata> |
//     <pkgleadbyte bytedata bytedata bytedata>
//
// PkgLeadByte :=
//     <bit 7-6: bytedata count that follows (0-3)>
//     <bit 5-4: only used if pkglength < 63>
//     <bit 3-0: least significant package length nybble>
//
_Success_( return )
BOOLEAN
AmlDecoderConsumePackageLength(
    _Inout_   AML_STATE* State,
    _Out_opt_ SIZE_T*    pPackageLength
    )
{
    UINT8  FirstByte;
    UINT8  VleExtraByteCount;
    SIZE_T VleExtraByteAccumulator;
    UINT8  FirstByteMaskShift;
    SIZE_T PackageLength;
    SIZE_T i;
    UINT8  ExtraByte;

    //
    // Consume the first byte, there must always be one byte or more.
    //
    if( AmlDecoderConsumeByte( State, &FirstByte ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // The high 2 bits of the first byte reveal how many following bytes are in the PkgLength.
    //
    VleExtraByteCount = ( ( FirstByte >> 6 ) & 3 );
    VleExtraByteAccumulator = 0;

    //
    // If the PkgLength has only one byte, bit 0 through 5 are used to encode the package length (in other words, values 0-63).
    // If the multiple bytes encoding is used, bits 0-3 of the PkgLeadByte become the least significant 4 bits of the resulting package length value.
    //
    FirstByteMaskShift = ( ( VleExtraByteCount == 0 ) ? 6 : 4 );
    PackageLength = ( FirstByte & ( ( 1 << FirstByteMaskShift ) - 1 ) );

    //
    // The next ByteData will become the next least significant 8 bits
    // of the resulting value and so on, up to 3 ByteData bytes.
    // Thus, the maximum package length is 2*28.
    //
    _Static_assert( sizeof( SIZE_T ) >= 4, "Unsupported build SIZE_T" );
    for( i = 0; i < VleExtraByteCount; i++ ) {
        if( AmlDecoderConsumeByte( State, &ExtraByte ) == AML_FALSE ) {
            return AML_FALSE;
        }
        VleExtraByteAccumulator |= ( ( SIZE_T )ExtraByte << ( i * 8 ) );
    }

    //
    // Combine accumulated extra VLE bytes with the final package length.
    //
    PackageLength |= ( VleExtraByteAccumulator << 4 );

    //
    // Return the final decoded PkgLength value.
    //
    if( pPackageLength != NULL ) {
        *pPackageLength = PackageLength;
    }

    return AML_TRUE;
}

//
// DefBuffer := BufferOp PkgLength BufferSize ByteList
//
_Success_( return )
BOOLEAN
AmlDecoderMatchDefBuffer(
    _Inout_   AML_STATE*              State,
    _Out_opt_ AML_OPAQUE_BUFFER_DATA* BufferDataOutput
    )
{
    AML_DECODER_INSTRUCTION_OPCODE Prefix;
    SIZE_T                         PkgRawDataStart;
    SIZE_T                         PkgLength;
    SIZE_T                         BufferSizeStart;

    //
    // Decode the prefix/main opcode (BufferOp).
    //
    if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_BUFFER_OP, &Prefix ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // The data size encoded by the PkgLength includes the encoded PkgLength itself,
    // and all of the following parts of the DefBuffer instruction.
    //
    PkgRawDataStart = State->DataCursor;

    //
    // Decode the VLE PkgLength.
    //
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Validate PkgLength, must not cause us to go out of bounds.
    //
    if( PkgRawDataStart >= State->DataLength || PkgLength > ( State->DataLength - PkgRawDataStart ) ) {
        return AML_FALSE;
    }

    //
    // The actual BufferSize is a TermArg, and must be interpreted, so just save the address of it for later.
    //
    BufferSizeStart = State->DataCursor;

    //
    // Skip past all of the rest of the uninterpreted arguments, to the end of this instruction.
    //
    State->DataCursor = ( PkgRawDataStart + PkgLength );

    //
    // Return optional information about the consumed BufferOp instruction.
    //
    if( BufferDataOutput != NULL ) {
        *BufferDataOutput = ( AML_OPAQUE_BUFFER_DATA ){
            .RawDataSize  = ( PkgLength - ( BufferSizeStart - PkgRawDataStart ) ),
            .SizeArgStart = BufferSizeStart,
        };
    }

    return AML_TRUE;
}

//
// ComputationalData := ByteConst | WordConst | DWordConst | QWordConst | String | ConstObj | RevisionOp | DefBuffer
//
_Success_( return )
BOOLEAN
AmlDecoderMatchComputationalData(
    _Inout_   AML_STATE*       State,
    _Out_opt_ AML_OPAQUE_DATA* OutputData
    )
{
    AML_OPAQUE_DATA                Data;
    UINT64                         Integer;
    AML_OPAQUE_STRING_DATA         String;
    AML_OPAQUE_BUFFER_DATA         Buffer;
    AML_DECODER_INSTRUCTION_OPCODE Instruction;
    BOOLEAN                        Success;

    //
    // Try to match (ByteConst | WordConst | DWordConst | QWordConst | ConstObj).
    //
    if( AmlDecoderMatchConstIntegerData( State, &Integer, NULL ) ) {
        if( OutputData != NULL ) {
            *OutputData = ( AML_OPAQUE_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = Integer };
        }
        return AML_TRUE;
    }

    //
    // Peek the next full instruction opcode.
    //
    if( AmlDecoderPeekOpcode( State, &Instruction ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Try to match (String | RevisionOp | DefBuffer).
    //
    switch( Instruction.OpcodeID ) {
    case AML_OPCODE_ID_STRING_PREFIX:
        Success = AmlDecoderMatchStringData( State, &String );
        Data    = ( AML_OPAQUE_DATA ){ .Type = AML_DATA_TYPE_STRING, .u.String = String };
        break;
    case AML_OPCODE_ID_REVISION_OP:
        Success = AML_TRUE;
        Data    = ( AML_OPAQUE_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = AML_INTERPRETER_REVISION };
        break;
    case AML_OPCODE_ID_BUFFER_OP:
        Success = AmlDecoderMatchDefBuffer( State, &Buffer );
        Data    = ( AML_OPAQUE_DATA ){ .Type = AML_DATA_TYPE_BUFFER, .u.Buffer = Buffer };
        break;
    default:
        Success = AML_FALSE;
        break;
    }

    //
    // Failure to match a valid ComputationalData.
    //
    if( Success == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Return optional consumed ComputationalData.
    //
    if( OutputData != NULL ) {
        *OutputData = Data;
    }

    return AML_TRUE;
}

//
// 20.2.6.1. Arg Objects Encoding
// ArgObj := Arg0Op | Arg1Op | Arg2Op | Arg3Op | Arg4Op | Arg5Op | Arg6Op
//
_Success_( return )
BOOLEAN
AmlDecoderMatchArgObj(
    _Inout_   AML_STATE* State,
    _Out_opt_ UINT8*     pArgObjIndex
    )
{
    AML_DECODER_INSTRUCTION_OPCODE Peek;

    if( AmlDecoderPeekOpcode( State, &Peek ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( Peek.OpcodeID >= AML_OPCODE_ID_ARG0_OP && Peek.OpcodeID <= AML_OPCODE_ID_ARG6_OP ) {
        AmlDecoderConsumeOpcode( State, NULL );
        if( pArgObjIndex != NULL ) {
            *pArgObjIndex = ( Peek.OpcodeID - AML_OPCODE_ID_ARG0_OP );
        }
        return AML_TRUE;
    }

    return AML_FALSE;
}

//
// 20.2.6.2. Local Objects Encoding
// LocalObj := Local0Op | Local1Op | Local2Op | Local3Op | Local4Op | Local5Op | Local6Op | Local7Op
//
_Success_( return )
BOOLEAN
AmlDecoderMatchLocalObj(
    _Inout_   AML_STATE* State,
    _Out_opt_ UINT8*     pLocalObjIndex
    )
{
    AML_DECODER_INSTRUCTION_OPCODE Peek;

    if( AmlDecoderPeekOpcode( State, &Peek ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( Peek.OpcodeID >= AML_OPCODE_ID_LOCAL0_OP && Peek.OpcodeID <= AML_OPCODE_ID_LOCAL7_OP ) {
        AmlDecoderConsumeOpcode( State, NULL );
        if( pLocalObjIndex != NULL ) {
            *pLocalObjIndex = ( Peek.OpcodeID - AML_OPCODE_ID_LOCAL0_OP );
        }
        return AML_TRUE;
    }

    return AML_FALSE;
}

//
// 20.2.2. Name Objects Encoding
// 
// LeadNameChar := 'A'-'Z' | '_'
// DigitChar := '0' - '9'
// NameChar := DigitChar | LeadNameChar
// RootChar := '\'
// ParentPrefixChar := '^'
// 
// NameSeg :=
//    <leadnamechar namechar namechar namechar>
//    // Notice that NameSegs shorter than 4 characters are filled with trailing underscores ('_'s).
// 
// SegCount := ByteData
// 
// SegCount can be from 1 to 255. For example: MultiNamePrefix(35) is
// encoded as 0x2f 0x23 and followed by 35 NameSegs. So, the total
// encoding length will be 1 + 1 + 35*4 = 142. Notice that:
// DualNamePrefix NameSeg NameSeg has a smaller encoding than the
// encoding of: MultiNamePrefix(2) NameSeg NameSeg
// 
// MultiNamePrefix := 0x2F
// MultiNamePath := MultiNamePrefix SegCount NameSeg(SegCount)
// 
// DualNamePrefix := 0x2E
// DualNamePath := DualNamePrefix NameSeg NameSeg
// 
// NullName := 0x00
//
// NamePath := NameSeg | DualNamePath | MultiNamePath | NullName
// 
// PrefixPath := Nothing | <'^' prefixpath>
// NameString := <rootchar namepath> | <prefixpath namepath>
// 
// TODO: Refactor all uses of this, things persistently hold on to the returned name strings,
// this is completely reliant on the backing AML bytecode existing for the entire lifetime!
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeNameString(
    _Inout_   AML_STATE*       State,
    _Out_opt_ AML_NAME_STRING* NameStringOutput
    )
{
    UINT8           LastPrefixChar;
    AML_NAME_PREFIX Prefix;
    UINT8           FirstByte;
    UINT8           NameSegCount;
    SIZE_T          NameSegStart;
    SIZE_T          i;
    AML_NAME_SEG    NameSeg;

    //
    // Consume any prefix characters (\ and ^).
    //
    for( Prefix.Length = 0; ; ) {
        //
        // Peek the char before consuming, we only want to consume prefix chars in this loop.
        //
        if( AmlDecoderPeekByte( State, 0, &LastPrefixChar ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Disallow prefix strings that are too big.
        //
        if( Prefix.Length >= ( AML_COUNTOF( Prefix.Data ) - 1 ) ) {
            return AML_FALSE;
        }

        //
        // Break out of prefix consumption loop once we reach a non-prefix byte.
        //
        if( LastPrefixChar != '\\' && LastPrefixChar != '^' ) {
            break;
        }

        //
        // Consume any prefix bytes.
        //
        if( AmlDecoderConsumeByte( State, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
        Prefix.Data[ Prefix.Length++ ] = LastPrefixChar;
    }

    Prefix.Data[ Prefix.Length ] = '\0';

    //
    // Peek the first character after the RootChar or PrefixPath char,
    // this character allow us to figure out what kind of NamePath sub-type it is.
    //
    if( AmlDecoderPeekByte( State, 0, &FirstByte ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // NamePath := NameSeg | DualNamePath | MultiNamePath | NullName
    //
    if( ( ( FirstByte < 'A' || FirstByte > 'Z' ) && FirstByte != '_' ) ) {
        switch( FirstByte ) {
        case AML_L1_MULTI_NAME_PREFIX:
            //
            // MultiNamePrefix := 0x2F
            // MultiNamePath := MultiNamePrefix SegCount NameSeg(SegCount)
            //
            AmlDecoderConsumeByte( State, NULL );
            if( AmlDecoderConsumeByte( State, &NameSegCount ) == AML_FALSE ) {
                return AML_FALSE;
            }
            break;
        case AML_L1_DUAL_NAME_PREFIX:
            //
            // DualNamePrefix := 0x2E
            // DualNamePath := DualNamePrefix NameSeg NameSeg
            //
            NameSegCount = 2;
            AmlDecoderConsumeByte( State, NULL );
            break;
        case '\0':
            //
            // NullName := 0x00
            //
            NameSegCount = 0;
            AmlDecoderConsumeByte( State, NULL );
            break;
        default:
            //
            // Invalid first byte of NamePath, isn't a special prefix or OP, and isn't a valid LeadNameChar.
            //
            return AML_FALSE;
        }
    } else {
        //
        // NamePath := NameSeg
        //
        NameSegCount = 1;
    }

    //
    // Backup the input data offset to the start of the namesegments before consuming them.
    //
    NameSegStart = State->DataCursor;

    //
    // Consume and validate all NameSegs.
    // NameSeg :=
    //   <leadnamechar namechar namechar namechar>
    //   // Notice that NameSegs shorter than 4 characters are filled with trailing underscores ('_'s).
    //
    for( i = 0; i < NameSegCount; i++ ) {
        if( AmlDecoderConsumeByteSpan( State, NameSeg.Data, sizeof( NameSeg.Data ) ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( ( ( NameSeg.Data[ 0 ] < 'A' || NameSeg.Data[ 0 ] > 'Z' ) && NameSeg.Data[ 0 ] != '_' ) ) {
            return AML_FALSE;
        }
    }

    //
    // Return optional information about the consumed NameString.
    //
    if( NameStringOutput != NULL ) {
        *NameStringOutput = ( AML_NAME_STRING ){
            .Prefix       = Prefix,
            .Segments     = ( AML_NAME_SEG* )&State->Data[ NameSegStart ],
            .SegmentCount = NameSegCount,
        };
    }

    return AML_TRUE;
}

//
// Attempt to match a name string.
//
_Success_( return )
BOOLEAN
AmlDecoderMatchNameString(
    _Inout_   AML_STATE*       State,
    _In_      BOOLEAN          MatchNullName,
    _Out_opt_ AML_NAME_STRING* NameStringOutput
    )
{
    UINT8   Byte;
    BOOLEAN IsMatch;

    //
    // Peek next opcode, name characters are valid opcodes.
    // TODO: Just peek next opcode, name string values are valid opcode table entries.
    //
    if( AmlDecoderPeekByte( State, 0, &Byte ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // DualNamePrefix := 0x2E
    // MultiNamePrefix := 0x2F
    //
    IsMatch = ( Byte >= AML_L1_DUAL_NAME_PREFIX && Byte <= AML_L1_MULTI_NAME_PREFIX );

    //
    // LeadNameChar := 'A'-'Z' | '_'
    //
    IsMatch |= ( ( ( Byte >= 'A' ) && ( Byte <= 'Z' ) ) || Byte == '_' );

    //
    // DigitChar := '0' - '9'
    //
    IsMatch |= ( Byte >= '0' && Byte <= '9' );

    //
    // RootChar := '\'
    // ParentPrefixChar := '^'
    //
    IsMatch |= ( ( Byte == '\\' ) || ( Byte == '^' ) );

    //
    // NullName := 0x00
    //
    IsMatch |= ( MatchNullName && ( Byte == 0 ) );

    //
    // Consume the entire name string if we have a match.
    //
    if( IsMatch ) {
        return AmlDecoderConsumeNameString( State, NameStringOutput );
    }

    return AML_FALSE;
}

//
// SimpleName := NameString | ArgObj | LocalObj
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeSimpleName(
    _Inout_   AML_STATE*       State,
    _Out_opt_ AML_SIMPLE_NAME* SimpleNameOutput
    )
{
    UINT8           FirstByte;
    BOOLEAN         Success;
    AML_SIMPLE_NAME Name;

    //
    // Peek first byte to try to figure out what kind of sub-type this is.
    //
    if( AmlDecoderPeekByte( State, 0, &FirstByte ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // NameString | ArgObj | LocalObj.
    //
    if( ( Success = AmlDecoderMatchArgObj( State, &Name.u.ArgObj ) ) != AML_FALSE ) {
        Name.Type = AML_SIMPLE_NAME_TYPE_ARG_OBJ;
    } else if( ( Success = AmlDecoderMatchLocalObj( State, &Name.u.LocalObj ) ) != AML_FALSE ) {
        Name.Type = AML_SIMPLE_NAME_TYPE_LOCAL_OBJ;
    } else if( ( Success = AmlDecoderConsumeNameString( State, &Name.u.NameString ) ) != AML_FALSE ) {
        Name.Type = AML_SIMPLE_NAME_TYPE_STRING;
    } 

    //
    // If we have failed to match any valid children of SimpleName, don't write out any output to the caller.
    //
    if( Success == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Return optional consumed name information.
    //
    if( SimpleNameOutput != NULL ) {
        *SimpleNameOutput = Name;
    }

    return AML_TRUE;
}

//
// PackageOp := 0x12
// DefPackage := PackageOp PkgLength NumElements PackageElementList
//
_Success_( return )
BOOLEAN
AmlDecoderMatchDefPackage(
    _Inout_   AML_STATE*               State,
    _Out_opt_ AML_OPAQUE_PACKAGE_DATA* PackageDataOutput
    )
{
    SIZE_T PkgRawDataStart;
    SIZE_T PkgLength;
    SIZE_T PkgElementListStart;
    UINT8  NumElements;

    //
    // Must conditionally match PackageOp, everything after may be unconditionally consumed.
    //
    if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_PACKAGE_OP, NULL ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // The start of all raw VLE data encoded by this instruction (including the PkgLength itself).
    //
    PkgRawDataStart = State->DataCursor;

    //
    // Get PkgLength, contains the raw data size of everything following the PackageOp byte (including the VLE PkgLength itself).
    //
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Validate PkgLength, must not cause us to go out of bounds.
    //
    if( PkgRawDataStart >= State->DataLength || PkgLength > ( State->DataLength - PkgRawDataStart ) ) {
        return AML_FALSE;
    }

    //
    // NumElements is just a single ByteData.
    //
    if( AmlDecoderConsumeByte( State, &NumElements ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Save the offset to the start of the element list data, the underlying data can be parsed later.
    //
    PkgElementListStart = State->DataCursor;

    //
    // Element list must be contained by the package length.
    //
    if( PkgLength < ( PkgElementListStart - PkgRawDataStart ) ) {
        return AML_FALSE;
    }

    //
    // Skip past all of the element list data, to the end of this instruction.
    //
    State->DataCursor = ( PkgRawDataStart + PkgLength );

    //
    // Return optional information about the consumed PackageOp instruction.
    //
    if( PackageDataOutput != NULL ) {
        *PackageDataOutput = ( AML_OPAQUE_PACKAGE_DATA ){
            .ElementListByteSize = ( PkgLength - ( PkgElementListStart - PkgRawDataStart ) ),
            .NumElements         = NumElements,
            .ElementListStart    = PkgElementListStart,
        };
    }

    return AML_TRUE;
}

//
// VarPackageOp := 0x13
// VarNumElements := TermArg => Integer
// DefVarPackage := VarPackageOp PkgLength VarNumElements PackageElementList
//
_Success_( return )
BOOLEAN
AmlDecoderMatchDefVarPackage(
    _Inout_   AML_STATE*                   State,
    _Out_opt_ AML_OPAQUE_VAR_PACKAGE_DATA* VarPackageDataOutput
    )
{
    SIZE_T PkgRawDataStart;
    SIZE_T PkgLength;
    SIZE_T VarNumElementsStart;

    //
    // Must conditionally match VarPackageOp, everything after may be unconditionally consumed.
    //
    if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_VAR_PACKAGE_OP, NULL ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // The start of all raw VLE data encoded by this instruction (including the PkgLength itself).
    //
    PkgRawDataStart = State->DataCursor;

    //
    // Get PkgLength, contains the raw data size of everything following the PackageOp byte (including the VLE PkgLength itself).
    //
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Validate PkgLength, must not cause us to go out of bounds.
    //
    if( PkgRawDataStart >= State->DataLength || PkgLength >= ( State->DataLength - PkgRawDataStart ) ) {
        return AML_FALSE;
    }

    //
    // Save the offset to the start of the VarNumElementsData, it must be evaluated, cannot be decoded here.
    //
    VarNumElementsStart = State->DataCursor;

    //
    // Skip past the VarNumElements and PackageElementList.
    //
    State->DataCursor = ( PkgRawDataStart + PkgLength );

    //
    // Return optional information about the consumed VarPackageOp instruction.
    //
    if( VarPackageDataOutput != NULL ) {
        *VarPackageDataOutput = ( AML_OPAQUE_VAR_PACKAGE_DATA ){
            .ByteSize            = ( PkgLength - ( VarNumElementsStart - PkgRawDataStart ) ),
            .VarNumElementsStart = VarNumElementsStart,
        };
    }

    return AML_TRUE;
}

//
// DataObject := ComputationalData | DefPackage | DefVarPackage
// Note: does not fully evaluate all data, returns opaque package data!
//
_Success_( return )
BOOLEAN
AmlDecoderMatchDataObject(
    _Inout_   AML_STATE*       State,
    _Out_opt_ AML_OPAQUE_DATA* DataOutput
    )
{
    AML_OPAQUE_DATA Data;
    BOOLEAN         Success;

    //
    // Try matching (ComputationalData | DefPackage | DefVarPackage).
    //
    do {
        Data = ( AML_OPAQUE_DATA ){ .Type = AML_DATA_TYPE_NONE };
        if( ( Success = AmlDecoderMatchComputationalData( State, &Data ) ) != AML_FALSE ) {
            break;
        } else if( ( Success = AmlDecoderMatchDefPackage( State, &Data.u.Package ) ) != AML_FALSE ) {
            Data.Type = AML_DATA_TYPE_PACKAGE;
            break;
        } else if( ( Success = AmlDecoderMatchDefVarPackage( State, &Data.u.VarPackage ) ) != AML_FALSE ) {
            Data.Type = AML_DATA_TYPE_VAR_PACKAGE;
            break;
        }
    } while( AML_FALSE );

    //
    // Failed to match any valid DataObject children.
    //
    if( Success == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Return optional consumed data object information.
    //
    if( DataOutput != NULL ) {
        *DataOutput = Data;
    }

    return AML_TRUE;
}

//
// DataRefObject := DataObject | ObjectReference
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeDataRefObject(
    _Inout_ AML_STATE* State
    )
{
    AML_OPAQUE_DATA Data;

    //
    // Match DataObject, first case.
    //
    if( AmlDecoderMatchDataObject( State, &Data ) ) {
        return AML_TRUE;
    }

    //
    // Shouldn't happen, not sure this is even possible anymore.
    //
    AML_DEBUG_ERROR( State, "Error: AmlDecoderMatchDataRefObject/ObjectReference case" );
    return AML_FALSE;
}

//
// Consume the SuperName element without evaluating.
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeSuperName(
    _Inout_ AML_STATE* State,
    _In_    UINT       NameFlags
    )
{
    AML_DECODER_INSTRUCTION_OPCODE Peek;
    AML_NAME_STRING                NameString;
    AML_NAMESPACE_NODE*            NsNode;
    SIZE_T                         i;

    //
    // Peek the next full instruction opcode.
    //
    if( AmlDecoderPeekOpcode( State, &Peek ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // SuperName := SimpleName | DebugObj | ReferenceTypeOpcode
    // Try to match DebugObj first, then SimpleName, then ReferenceTypeOpcode,
    // as ReferenceTypeOpcode is greedy in the MethodInvocation case.
    //
    if( Peek.OpcodeID == AML_OPCODE_ID_DEBUG_OP ) {
        return AmlDecoderConsumeOpcode( State, NULL );
    } 

    //
    // Try to match and consume reference type opcodes.
    // ReferenceTypeOpcode := DefRefOf | DefDerefOf | DefIndex | MethodInvocation
    //
    switch( Peek.OpcodeID ) {
    case AML_OPCODE_ID_REF_OF_OP:
    case AML_OPCODE_ID_DEREF_OF_OP:
    case AML_OPCODE_ID_INDEX_OP:
        return AmlDecoderConsumeInstruction( State );
    }
    
    //
    // Try to match and consume non-namestring SimpleName opcodes.
    //
    if( ( Peek.OpcodeID >= AML_OPCODE_ID_LOCAL0_OP ) && ( Peek.OpcodeID <= AML_OPCODE_ID_LOCAL7_OP ) ) {
        return AML_TRUE;
    } else if( ( Peek.OpcodeID >= AML_OPCODE_ID_ARG0_OP ) && ( Peek.OpcodeID <= AML_OPCODE_ID_ARG6_OP ) ) {
        return AML_TRUE;
    }

    //
    // If NullName is allowed here (for example for Target), try to match and allow it.
    //
    if( ( NameFlags & AML_NAME_FLAG_ALLOW_NULL_NAME ) ) {
        if( AmlDecoderMatchByte( State, 0 ) ) {
            return AML_TRUE;
        }
    }

    //
    // Try to match a NameString (referencing an object or a MethodInvocation).
    //
    if( AmlDecoderConsumeNameString( State, &NameString ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // We have matched a SimpleName, it can either be a regular object or a MethodInvocation.
    // In the case that it is a method invocation, if we have yet to previously parse and create
    // a method namespace object, we have no way of knowing that it is a method invocation,
    // and no way of knowing the amount of arguments, parsing will fail shortly after.
    //
    if( AmlNamespaceSearch( &State->Namespace, NULL, &NameString, 0, &NsNode ) == AML_FALSE ) {
        return AML_TRUE;
    }

    //
    // We have found a namespace node for the simple name, if it is a method,
    // we must handle a MethodInvocation case (consume all arguments).
    // Certain cases like in CondRefOf do not cause actual MethodInvocation,
    // so the NO_METHOD_INVOCATION flag was added to support this case.
    // TODO: Ensure that method arguments can only be TermArgs.
    // TODO: A SuperName should never lead to a method invocation?
    //
    if( ( ( NameFlags & AML_NAME_FLAG_NO_METHOD_INVOCATION ) == 0 )
        && ( NsNode->Object->Type == AML_OBJECT_TYPE_METHOD ) )
    {
        for( i = 0; i < NsNode->Object->u.Method.ArgumentCount; i++ ) {
            if( AmlDecoderConsumeTermArg( State, NULL ) == AML_FALSE ) {
                return AML_FALSE;
            }
        }
    }

    return AML_TRUE;
}

//
// Generically attempt to consume all fixed arguments of the given instruction.
// The internal function does not handle updating the active recursion depth.
//
_Success_( return )
static
BOOLEAN
AmlDecoderConsumeInstructionArgsInternal(
    _Inout_ AML_STATE*                            State,
    _In_    const AML_DECODER_INSTRUCTION_OPCODE* Instruction
    )
{
    SIZE_T  PkgStart;
    SIZE_T  PkgLength;
    SIZE_T  i;
    UINT8   Type;
    UINT8   PeekByte;
    BOOLEAN Success;
    SIZE_T  StartCursor;

    //
    // Currently all variable argument instructions are encoded using a PkgLength containing all of their arguments.
    //
    if( Instruction->VariableArgumentTypeCount != 0 ) {
        PkgStart = State->DataCursor;
        if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlDecoderIsValidDataWindow( State, PkgStart, PkgLength ) == AML_FALSE ) {
            return AML_FALSE;
        }
        State->DataCursor = PkgStart;
        return AmlDecoderConsumeByteSpan( State, NULL, PkgLength );
    }

    //
    // Save the cursor position that we started the argument decoding at (currently used for lookback validation).
    //
    StartCursor = State->DataCursor;

    //
    // Attempt to decode and consume all fixed arguments.
    //
    for( i = 0; i < Instruction->FixedArgumentTypeCount; i++ ) {
        Type = Instruction->FixedArgumentTypes[ i ];
        switch( Type ) {
        case AML_ARGUMENT_FIXED_TYPE_NAME_STRING:
            Success = AmlDecoderConsumeNameString( State, NULL );
            break;
        case AML_ARGUMENT_FIXED_TYPE_DATA_REF_OBJECT:
            Success = AmlDecoderConsumeDataRefObject( State );
            break;
        case AML_ARGUMENT_FIXED_TYPE_BYTE_DATA:
            Success = AmlDecoderConsumeByte( State, NULL );
            break;
        case AML_ARGUMENT_FIXED_TYPE_WORD_DATA:
            Success = AmlDecoderConsumeWord( State, NULL );
            break;
        case AML_ARGUMENT_FIXED_TYPE_DWORD_DATA:
            Success = AmlDecoderConsumeDWord( State, NULL );
            break;
        case AML_ARGUMENT_FIXED_TYPE_QWORD_DATA:
            Success = AmlDecoderConsumeQWord( State, NULL );
            break;
        case AML_ARGUMENT_FIXED_TYPE_ASCII_CHAR_LIST:
            //
            // AsciiCharList arguments must be terminated by a NullChar argument.
            //
            if( ( i == ( Instruction->FixedArgumentTypeCount - 1 ) )
                || ( Instruction->FixedArgumentTypes[ i + 1 ] != AML_ARGUMENT_FIXED_TYPE_NULL_CHAR ) )
            {
                Success = AML_FALSE;
                break;
            }
            Success = AmlDecoderConsumeAsciiCharListNullTerminated( State, NULL );
            i++; /* Account for consumed NullChar argument, ignore the fixed arg entry for it. */
            break;
        case AML_ARGUMENT_FIXED_TYPE_NULL_CHAR:
            Success = AmlDecoderMatchByte( State, 0 );
            break;
        case AML_ARGUMENT_FIXED_TYPE_TERM_ARG:
            Success = AmlDecoderConsumeTermArg( State, NULL );
            break;
        case AML_ARGUMENT_FIXED_TYPE_NAME_SEG:
            Success = AmlDecoderConsumeByteSpan( State, NULL, sizeof( AML_NAME_SEG ) );
            break;
        case AML_ARGUMENT_FIXED_TYPE_NAME_SEG_N:
            //
            // NameSeg(N) arguments must be prefixed with a ByteData argument that specifies N.
            // TODO: Validate NameSeg LeadChars.
            //
            if( ( State->DataCursor - StartCursor ) < 1 ) {
                Success = AML_FALSE;
                break;
            }
            Success = AmlDecoderConsumeByteSpan( State, NULL, ( sizeof( AML_NAME_SEG ) * State->Data[ State->DataCursor - 1 ] ) );
            break;
        case AML_ARGUMENT_FIXED_TYPE_SUPER_NAME:
            Success = AmlDecoderConsumeSuperName( State, 0 );
            break;
        case AML_ARGUMENT_FIXED_TYPE_SUPER_NAME_NO_INVOCATION:
            Success = AmlDecoderConsumeSuperName( State, AML_NAME_FLAG_NO_METHOD_INVOCATION );
            break;
        case AML_ARGUMENT_FIXED_TYPE_TARGET:
            if( AmlDecoderPeekByte( State, 0, &PeekByte ) == AML_FALSE ) {
                Success = AML_FALSE;
                break;
            } else if( PeekByte == 0 ) {
                Success = AmlDecoderConsumeByte( State, NULL );
                break;
            }
            Success = AmlDecoderConsumeSuperName( State, 0 );
            break;
        case AML_ARGUMENT_FIXED_TYPE_SIMPLE_NAME:
            Success = AmlDecoderConsumeSimpleName( State, NULL );
            break;
        default:
            Success = AML_FALSE;
            break;
        }

        //
        // Failed to decode an argument, fatal.
        //
        if( Success == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    return AML_TRUE;
}

//
// Generically attempt to consume all fixed arguments of the given instruction.
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeInstructionArgs(
    _Inout_ AML_STATE*                            State,
    _In_    const AML_DECODER_INSTRUCTION_OPCODE* Instruction
    )
{
    SIZE_T  OldDepth;
    BOOLEAN Success;

    //
    // Cannot continue decoding another instruction argument if we have reached the recursion limit.
    //
    if( State->RecursionDepth >= AML_BUILD_MAX_RECURSION_DEPTH ) {
        AML_DEBUG_ERROR( State, "Error: Reached recursion depth limit!\n" );
        return AML_FALSE;
    }

    //
    // Update the recursion depth and perform the actual decoding of the arguments.
    //
    OldDepth = State->RecursionDepth;
    State->RecursionDepth += 1;
    Success = AmlDecoderConsumeInstructionArgsInternal( State, Instruction );
    State->RecursionDepth = OldDepth;
    return Success;
}

//
// Attempt to consume a single full instruction (opcode and arguments) without executing it.
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeInstruction(
    _Inout_ AML_STATE* State
    )
{
    AML_DECODER_INSTRUCTION_OPCODE Instruction;

    //
    // Consume the next opcode and get information.
    //
    if( AmlDecoderConsumeOpcode( State, &Instruction ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Consume all arguments.
    //
    return AmlDecoderConsumeInstructionArgs( State, &Instruction );
}

//
// Consume an expression opcode without executing it.
//
_Success_( return )
static
BOOLEAN
AmlDecoderConsumeExpressionOpcode(
    _Inout_ AML_STATE* State
    )
{
    AML_DECODER_INSTRUCTION_OPCODE Instruction;
    AML_NAME_STRING                NameString;
    AML_NAMESPACE_NODE*            NsNode;
    SIZE_T                         i;

    //
    // Peek the next opcode information.
    //
    if( AmlDecoderPeekOpcode( State, &Instruction ) ) {
        //
        // If this is a regular expression opcode, try to handle it generically.
        //
        if( Instruction.IsExpressionOpcode ) {
            //
            // Consume the expression opcode, move forward to the arguments of it.
            //
            AmlDecoderConsumeOpcode( State, NULL );

            //
            // Try to consume all of the instruction's arguments generically.
            //
            return AmlDecoderConsumeInstructionArgs( State, &Instruction );
        }
    }

    //
    // If we have failed to match any expression opcode, this must be a MethodInvocation.
    // This requires special handling, we can *only* parse this if we have previously encountered the definition of this method,
    // otherwise we have no way of knowing how many arguments there are, nor what their types are. Complete failure of the spec.
    // TODO: This requires namespace support in the case where it is actually parseable.
    //
    if( AmlDecoderConsumeNameString( State, &NameString ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // We have matched a SimpleName, it can either be a regular object or a MethodInvocation.
    // In the case that it is a method invocation, if we have yet to previously parse and create
    // a method namespace object, we have no way of knowing that it is a method invocation,
    // and no way of knowing the amount of arguments, parsing will fail shortly after.
    //
    if( AmlNamespaceSearch( &State->Namespace, NULL, &NameString, 0, &NsNode ) == AML_FALSE ) {
        return AML_TRUE;
    }

    //
    // If we have matched an object name, but it isn't a method invocation, move on.
    //
    if( NsNode->Object->Type != AML_OBJECT_TYPE_METHOD ) {
        return AML_TRUE;
    }

    //
    // Consume all MethodInvocation arguments.
    // TODO: Ensure that they can only be TermArgs.
    //
    for( i = 0; i < NsNode->Object->u.Method.ArgumentCount; i++ ) {
        if( AmlDecoderConsumeTermArg( State, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    return AML_TRUE;
}

//
// Attempts to consume an entire term arg, decoding until we figure out how long the span of term arg data is,
// then the offset to the raw bytes of the decoded span are returned to the caller (if desired).
// Does not actually evaluate the TermArg, only attempts to decode enough to figure out the size of the span.
// TermArg := ExpressionOpcode | DataObject | ArgObj | 
// The internal function does not handle updating the active recursion depth.
//
_Success_( return )
static
BOOLEAN
AmlDecoderConsumeTermArgInternal(
    _Inout_   AML_STATE*                State,
    _Out_opt_ AML_OPAQUE_TERM_ARG_DATA* OpaqueSpanOutput
    )
{
    SIZE_T          TermArgStart;
    AML_OPAQUE_DATA DataObj;
    UINT8           ArgObj;
    UINT8           LocalObj;
    BOOLEAN         Success;

    //
    // Save the start of the opaque TermArg data.
    //
    TermArgStart = State->DataCursor;

    //
    // Try cases that require a smaller range first, match DataObject | ArgObj | LocalObj. 
    //
    do {
        if( ( Success = AmlDecoderMatchDataObject( State, &DataObj ) != AML_FALSE ) ) {
            break;
        } else if( ( Success = AmlDecoderMatchArgObj( State, &ArgObj ) != AML_FALSE ) ) {
            break;
        } else if( ( Success = AmlDecoderMatchLocalObj( State, &LocalObj ) != AML_FALSE ) ) {
            break;
        }
    } while( AML_FALSE );

    //
    // If we have failed to match the easier cases, try to match any expression opcode.
    //
    if( Success == AML_FALSE ) {
        if( ( Success = AmlDecoderConsumeExpressionOpcode( State ) ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    //
    // Optionally return information about the consumed opaque span.
    //
    if( OpaqueSpanOutput != NULL ) {
        *OpaqueSpanOutput = ( AML_OPAQUE_TERM_ARG_DATA ){
            .Start = TermArgStart,
            .Size  = ( State->DataCursor - TermArgStart ),
        };
    }

    return AML_TRUE;
}

//
// May lead to recursion!
// Attempts to consume an entire term arg, decoding until we figure out how long the span of term arg data is,
// then the offset to the raw bytes of the decoded span are returned to the caller (if desired).
// Does not actually evaluate the TermArg, only attempts to decode enough to figure out the size of the span.
// TermArg := ExpressionOpcode | DataObject | ArgObj | LocalObj
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeTermArg(
    _Inout_   AML_STATE*                State,
    _Out_opt_ AML_OPAQUE_TERM_ARG_DATA* OpaqueSpanOutput
    )
{
    SIZE_T  OldDepth;
    BOOLEAN Success;

    //
    // Cannot continue decoding another instruction argument if we have reached the recursion limit.
    //
    if( State->RecursionDepth >= AML_BUILD_MAX_RECURSION_DEPTH ) {
        AML_DEBUG_ERROR( State, "Error: Reached recursion depth limit!\n" );
        return AML_FALSE;
    }

    //
    // Update the recursion depth and perform the actual decoding of the TermArg.
    //
    OldDepth = State->RecursionDepth;
    State->RecursionDepth += 1;
    Success = AmlDecoderConsumeTermArgInternal( State, OpaqueSpanOutput );
    State->RecursionDepth = OldDepth;
    return Success;
}

//
// ComputationalData := ByteConst | WordConst | DWordConst | QWordConst | String | ConstObj | RevisionOp | DefBuffer
// DataObject := ComputationalData | DefPackage | DefVarPackage
// TODO: Add all this classification stuff to the opcode table!
//
_Success_( return )
BOOLEAN
AmlDecoderIsDataObject(
    _Inout_ AML_STATE*                            State,
    _In_    const AML_DECODER_INSTRUCTION_OPCODE* Opcode
    )
{
    BOOLEAN IsMatch;

    //
    // ComputationalData := ByteConst | WordConst | DWordConst | QWordConst | String | ConstObj | RevisionOp | DefBuffer
    //
    IsMatch  = ( ( Opcode->OpcodeID >= AML_OPCODE_ID_BYTE_PREFIX ) && ( Opcode->OpcodeID <= AML_OPCODE_ID_QWORD_PREFIX ) );
    IsMatch |= ( ( Opcode->OpcodeID >= AML_OPCODE_ID_ZERO_OP ) && ( Opcode->OpcodeID <= AML_OPCODE_ID_ONE_OP ) );
    IsMatch |= ( Opcode->OpcodeID == AML_OPCODE_ID_ONES_OP );
    IsMatch |= ( Opcode->OpcodeID == AML_OPCODE_ID_REVISION_OP );
    IsMatch |= ( Opcode->OpcodeID == AML_OPCODE_ID_BUFFER_OP );

    //
    // DefPackage | DefVarPackage
    //
    IsMatch |= ( ( Opcode->OpcodeID >= AML_OPCODE_ID_PACKAGE_OP ) && ( Opcode->OpcodeID <= AML_OPCODE_ID_VAR_PACKAGE_OP ) );

    return IsMatch;
}

//
// Attempts to classify the given opcode and determine if it is a statement opcode.
// TODO: Add all this classification stuff to the opcode table!
//
_Success_( return )
BOOLEAN
AmlDecoderIsStatementOpcode(
    _Inout_ AML_STATE*                            State,
    _In_    const AML_DECODER_INSTRUCTION_OPCODE* Op
    )
{
    BOOLEAN IsStatement;

    //
    // 0x5B 0x21               , StallOp                , Term Object     , TermArg                                           , -                        , 0		
    // 0x5B 0x22               , SleepOp                , Term Object     , TermArg                                           , -                        , 0	
    // 0x5B 0x24               , SignalOp               , Term Object     , SuperName                                         , -                        , 0		
    // 0x5B 0x26               , ResetOp                , Term Object     , SuperName                                         , -                        , 0		
    // 0x5B 0x27               , ReleaseOp              , Term Object     , SuperName                                         , -                        , 0		
    // 0x5B 0x32               , FatalOp                , Term Object     , ByteData DWordData TermArg                        , -                        , 0	
    //
    switch( Op->OpcodeID ) {
    case AML_OPCODE_ID_STALL_OP:
    case AML_OPCODE_ID_SLEEP_OP:
    case AML_OPCODE_ID_SIGNAL_OP:
    case AML_OPCODE_ID_RESET_OP:
    case AML_OPCODE_ID_RELEASE_OP:
        IsStatement = AML_TRUE;
        break;
    default:
        IsStatement = AML_FALSE;
        break;
    }
    IsStatement |= ( Op->OpcodeID == AML_OPCODE_ID_FATAL_OP );

    //
    // 0xA0                    , IfOp                   , Term Object     , TermArg                                           , TermList                 , 0		
    // 0xA1                    , ElseOp                 , Term Object     , -                                                 , TermList                 , 0		
    // 0xA2                    , WhileOp                , Term Object     , TermArg                                           , TermList                 , 0		
    // 0xA3                    , NoopOp                 , Term Object     , -                                                 , -                        , 0		
    // 0xA4                    , ReturnOp               , Term Object     , TermArg                                           , -                        , 0		
    // 0xA5                    , BreakOp                , Term Object     , -                                                 , -                        , 0		
    //
    IsStatement |= ( ( Op->OpcodeID >= AML_OPCODE_ID_IF_OP ) && ( Op->OpcodeID <= AML_OPCODE_ID_BREAK_OP ) );

    //
    // 0x86                    , NotifyOp               , Term Object     , SuperName TermArg                                 , -                        , 0		
    // 0x9F                    , ContinueOp             , Term Object     , -                                                 , -                        , 0		
    // 0xCC                    , BreakPointOp           , Term Object     , -                                                 , -                        , 0
    //
    IsStatement |= ( Op->OpcodeID == AML_OPCODE_ID_NOTIFY_OP );
    IsStatement |= ( Op->OpcodeID == AML_OPCODE_ID_CONTINUE_OP );
    IsStatement |= ( Op->OpcodeID == AML_OPCODE_ID_BREAK_POINT_OP );

    return IsStatement;
}

//
// Attempts to classify the given opcode and determine if it is an expression opcode.
//
_Success_( return )
BOOLEAN
AmlDecoderIsExpressionOpcode(
    _Inout_ AML_STATE*                            State,
    _In_    const AML_DECODER_INSTRUCTION_OPCODE* Op
    )
{
    return Op->IsExpressionOpcode;
}

//
// Attempts to classify the given opcode and determine if it is a namespace modifier object opcode.
// TODO: Add all this classification stuff to the opcode table!
//
_Success_( return )
BOOLEAN
AmlDecoderIsNamespaceModifierObjectOpcode(
    _Inout_ AML_STATE*                            State,
    _In_    const AML_DECODER_INSTRUCTION_OPCODE* Op
    )
{
    //
    // NameSpaceModifierObj := DefAlias | DefName | DefScope
    //
    return ( ( Op->OpcodeID == AML_OPCODE_ID_ALIAS_OP )
             | ( Op->OpcodeID == AML_OPCODE_ID_NAME_OP )
             | ( Op->OpcodeID == AML_OPCODE_ID_SCOPE_OP ) );
}


//
// Attempts to classify the given opcode and determine if it is a named object opcode.
// NamedObj := DefBankField | DefCreateBitField | DefCreateByteField | DefCreateWordField
// | DefCreateDWordField | DefCreateQWordField | DefCreateField | DefDataRegion | DefExternal
// | DefOpRegion | DefPowerRes | DefThermalZone | DefDevice | DefEvent | DefField | DefIndexField
// | DefMethod
// TODO: Add all this classification stuff to the opcode table!
//
_Success_( return )
BOOLEAN
AmlDecoderIsNamedObjectOpcode(
    _Inout_ AML_STATE*                            State,
    _In_    const AML_DECODER_INSTRUCTION_OPCODE* Op
    )
{
    BOOLEAN IsMatch;

    //
    // 0x14                    , MethodOp               , Term Object     , NameString ByteData                               , TermList                 , 0	
    // 0x15                    , ExternalOp             , Name Object     , NameString ByteData ByteData                      , -                        , 0
    //
    IsMatch = ( ( Op->OpcodeID >= AML_OPCODE_ID_METHOD_OP ) && ( Op->OpcodeID <= AML_OPCODE_ID_EXTERNAL_OP ) );

    //
    // 0x8A                    , CreateDWordFieldOp     , Term Object     , TermArg TermArg NameString                        , -                        , 0		
    // 0x8B                    , CreateWordFieldOp      , Term Object     , TermArg TermArg NameString                        , -                        , 0		
    // 0x8C                    , CreateByteFieldOp      , Term Object     , TermArg TermArg NameString                        , -                        , 0		
    // 0x8D                    , CreateBitFieldOp       , Term Object     , TermArg TermArg NameString                        , -                        , 0
    //
    IsMatch |= ( ( Op->OpcodeID >= AML_OPCODE_ID_CREATE_DWORD_FIELD_OP ) && ( Op->OpcodeID <= AML_OPCODE_ID_CREATE_BIT_FIELD_OP ) );

    //
    // 0x8F                    , CreateQWordFieldOp     , Term Object     , TermArg TermArg NameString                        , -                        , 0
    //
    IsMatch |= ( Op->OpcodeID == AML_OPCODE_ID_CREATE_QWORD_FIELD_OP );

    //
    // 0x5B 0x13               , CreateFieldOp          , Term Object     , TermArg TermArg TermArg NameString                , -                        , 0	
    // 0x5B 0x02               , EventOp                , Term Object     , NameString                                        , -                        , 0		
    // 0x5B 0x01               , MutexOp                , Term Object     , NameString ByteData                               , -                        , 0		
    //
    IsMatch |= ( Op->OpcodeID == AML_OPCODE_ID_CREATE_FIELD_OP );
    IsMatch |= ( Op->OpcodeID == AML_OPCODE_ID_EVENT_OP );
    IsMatch |= ( Op->OpcodeID == AML_OPCODE_ID_MUTEX_OP );

    //
    // 0x5B 0x80               , OpRegionOp             , Term Object     , NameString ByteData TermArg TermArg               , -                        , 0		
    // 0x5B 0x81               , FieldOp                , Term Object     , NameString ByteData                               , FieldList                , 0	
    // 0x5B 0x82               , DeviceOp               , Term Object     , NameString                                        , TermList                 , 0	
    // 0x5B 0x83               , ProcessorOp            , Term Object     , NameString ByteData DWordData ByteData            , ObjectList               , 0		
    // 0x5B 0x84               , PowerResOp             , Term Object     , NameString ByteData WordData                      , TermList                 , 0		
    // 0x5B 0x85               , ThermalZoneOp          , Term Object     , NameString                                        , TermList                 , 0
    // 0x5B 0x86               , IndexFieldOp           , Term Object     , NameString NameString ByteData                    , FieldList                , 0		
    // 0x5B 0x87               , BankFieldOp            , Term Object     , NameString NameString TermArg ByteData            , FieldList                , 0
    // 0x5B 0x88               , DataRegionOp           , Term Object     , NameString TermArg TermArg TermArg                , -                        , 0	
    //
    IsMatch |= ( ( Op->OpcodeID >= AML_OPCODE_ID_OP_REGION_OP ) && ( Op->OpcodeID <= AML_OPCODE_ID_DATA_REGION_OP ) );

    return IsMatch;
}

//
// Sign extend the given integer value to the decoder revision integer size.
//
UINT64
AmlDecoderSignExtendInteger(
    _In_ const AML_STATE* State,
    _In_ UINT64           Value
    )
{
    if( State->IsIntegerSize64 ) {
        return Value;
    }
    return ( UINT64 )( INT64 )( INT32 )Value;
}
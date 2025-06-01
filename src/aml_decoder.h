#pragma once

#include "aml_heap.h"
#include "aml_state.h"
#include "aml_opcode.h"
#include "aml_data.h"
#include "aml_namespace.h"
#include "aml_host.h"
#include "aml_method.h"
#include "aml_platform.h"

//
// Name resolution flags.
//
#define AML_NAME_FLAG_NONE                 0
#define AML_NAME_FLAG_ALLOW_NULL_NAME      1
#define AML_NAME_FLAG_ALLOW_NON_EXISTENT   2
#define AML_NAME_FLAG_NO_METHOD_INVOCATION 3

//
// AML match instruction operator values.
//
#define AML_MATCH_OP_MTR 0
#define AML_MATCH_OP_MEQ 1
#define AML_MATCH_OP_MLE 2
#define AML_MATCH_OP_MLT 3
#define AML_MATCH_OP_MGE 4
#define AML_MATCH_OP_MGT 5

//
// Single full instruction (1 or 2 byte opcode) decoding result.
//
typedef struct _AML_DECODER_INSTRUCTION_OPCODE {
	UINT16                 OpcodeID;
	BOOLEAN                Is2ByteOpcode;
	BOOLEAN                IsExpressionOpcode;
	UINT8                  PrefixL1; /* 0 if no prefix/this is not a 2-byte instruction. */
	AML_OPCODE_TABLE_ENTRY LeafOpcode;
	const UINT8*           FixedArgumentTypes;
	SIZE_T                 FixedArgumentTypeCount;
	const UINT8*           VariableArgumentTypes;
	SIZE_T                 VariableArgumentTypeCount;
} AML_DECODER_INSTRUCTION_OPCODE;

#if 0
//
// Decoder table fixed argument type value.
//
typedef struct _AML_DECODER_ARGUMENT_FIXED {
	UINT8        Type;
	const UINT8* DataStart;
	SIZE_T       DataSize;
	union {
		AML_NAME_STRING            NameString;
		UINT8                      ByteData;
		AML_OPAQUE_DATA_REF_OBJECT DataRefObject;
		UINT16                     WordData;
		UINT32                     DWordData;
		UINT64                     QWordData;
		AML_OPAQUE_STRING_DATA     AsciiCharList;
		AML_OPAQUE_TERM_ARG_DATA   TermArg;
		AML_NAME_STRING            NameSeg;
		AML_NAME_STRING            NameSegN;
		AML_SUPER_NAME             SuperName;
		AML_SUPER_NAME             Target;
		AML_SIMPLE_NAME            SimpleName;
	} u;
} AML_DECODER_ARGUMENT_FIXED;

#define AML_DECODER_MAX_ARGUMENT_COUNT 9

//
// Decoder table fixed instruction argument values.
//
typedef struct _AML_DECODER_ARGUMENTS_FIXED {
	SIZE_T                     Count;
	AML_DECODER_ARGUMENT_FIXED Entries[ AML_DECODER_MAX_ARGUMENT_COUNT ];
} AML_DECODER_ARGUMENTS_FIXED;
#endif

//
// Opaque variable-length data that wasn't fully evaluated by the decoder.
//

typedef struct _AML_OPAQUE_REFERENCE_TYPE_OPCODE {
	SIZE_T Start;
	SIZE_T Size;
} AML_OPAQUE_REFERENCE_TYPE_OPCODE;

typedef struct _AML_OPAQUE_PACKAGE_DATA {
	UINT64 NumElements;
	SIZE_T ElementListStart;
	SIZE_T ElementListByteSize;
} AML_OPAQUE_PACKAGE_DATA;

typedef struct _AML_OPAQUE_STRING_DATA {
	SIZE_T Start;
	SIZE_T DataSize;
} AML_OPAQUE_STRING_DATA;

typedef struct _AML_OPAQUE_BUFFER_DATA {
	SIZE_T RawDataSize;
	SIZE_T SizeArgStart;
} AML_OPAQUE_BUFFER_DATA;

typedef struct _AML_OPAQUE_TERM_ARG_DATA {
	SIZE_T Start;
	SIZE_T Size;
} AML_OPAQUE_TERM_ARG_DATA;

typedef struct _AML_OPAQUE_METHOD_INVOCATION_DATA {
	AML_NAME_STRING Name;
	SIZE_T          TermListDataStart;
	SIZE_T          TermListDataSize;
} AML_OPAQUE_METHOD_INVOCATION_DATA;

typedef struct _AML_OPAQUE_VAR_PACKAGE_DATA {
	SIZE_T ByteSize;
	SIZE_T VarNumElementsStart;
} AML_OPAQUE_VAR_PACKAGE_DATA;

//
// Opaque unevaluated data types, generally only used for disassembly.
// TODO: Get rid of the decoder/eval split.
//
typedef struct _AML_OPAQUE_DATA {
	AML_DATA_TYPE Type;
	union {
		UINT64                      Integer;
		UINT32                      Integer32;
		AML_OPAQUE_PACKAGE_DATA     Package;
		AML_OPAQUE_STRING_DATA      String;
		AML_OPAQUE_BUFFER_DATA      Buffer;
		AML_OPAQUE_VAR_PACKAGE_DATA VarPackage;
	} u;
} AML_OPAQUE_DATA;

//
// Sign extend the given integer value to the decoder revision integer size.
//
UINT64
AmlDecoderSignExtendInteger(
	_In_ const AML_STATE* State,
	_In_ UINT64           Value
	);

//
// Peek a single byte of input data.
//
_Success_( return )
BOOLEAN
AmlDecoderPeekByte(
	_In_  const AML_STATE* State,
	_In_  SIZE_T           LookaheadOffset,
	_Out_ UINT8*           pPeekByte
	);

//
// Consume a single byte of input data.
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeByte(
	_Inout_   AML_STATE* State,
	_Out_opt_ UINT8*     pConsumedByte
	);

//
// Consume a span of input data bytes.
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeByteSpan(
	_Inout_                                 AML_STATE* State,
	_Out_writes_bytes_all_opt_( ByteCount ) UINT8*     ConsumedByteSpan,
	_In_                                    SIZE_T     ByteCount
	);

//
// Consume the next input data byte if it matches the given byte.
//
BOOLEAN
AmlDecoderMatchByte(
	_Inout_ AML_STATE* State,
	_In_    UINT8      MatchByte
	);

//
// WordData := ByteData[0:7] ByteData[8:15]
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeWord(
	_Inout_   AML_STATE* State,
	_Out_opt_ UINT16*    pConsumedWord
	);

//
// DWordData := WordData[0:15] WordData[16:31]
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeDWord(
	_Inout_   AML_STATE* State,
	_Out_opt_ UINT32*    pConsumedDWord
	);

//
// QWordData := DWordData[0:31] DWordData[32:63]
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeQWord(
	_Inout_   AML_STATE* State,
	_Out_opt_ UINT64*    pConsumedQWord
	);

//
// Check if the given window is valid (contained by the total data).
//
_Success_( return )
BOOLEAN
AmlDecoderIsValidDataWindow(
	_Inout_ AML_STATE* State,
	_In_    SIZE_T     DataOffset,
	_In_    SIZE_T     DataLength
	);

//
// Peek information about the next entire opcode (including extended 2-byte opcodes).
//
_Success_( return )
BOOLEAN
AmlDecoderPeekOpcode(
	_In_  const AML_STATE*                State,
	_Out_ AML_DECODER_INSTRUCTION_OPCODE* OpcodeInfo
	);

//
// Consume the full opcode bytes of the next instruction.
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeOpcode(
	_Inout_   AML_STATE*                      State,
	_Out_opt_ AML_DECODER_INSTRUCTION_OPCODE* OpcodeInfo
	);

//
// Match and consume the next full opcode.
//
_Success_( return )
BOOLEAN
AmlDecoderMatchOpcode(
	_Inout_   AML_STATE*                      State,
	_In_      UINT16                          FullOpcodeID,
	_Out_opt_ AML_DECODER_INSTRUCTION_OPCODE* OpcodeInfo
	);

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
	);

//
// AsciiCharList NullChar
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeAsciiCharListNullTerminated(
	_Inout_   AML_STATE*              State,
	_Out_opt_ AML_OPAQUE_STRING_DATA* StringData
	);

//
// String := StringPrefix AsciiCharList NullChar
//
_Success_( return )
BOOLEAN
AmlDecoderMatchStringData(
	_Inout_   AML_STATE*              State,
	_Out_opt_ AML_OPAQUE_STRING_DATA* StringData
	);

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
	);

//
// DefBuffer := BufferOp PkgLength BufferSize ByteList
//
_Success_( return )
BOOLEAN
AmlDecoderMatchDefBuffer(
	_Inout_   AML_STATE*              State,
	_Out_opt_ AML_OPAQUE_BUFFER_DATA* BufferDataOutput
	);

//
// ComputationalData := ByteConst | WordConst | DWordConst | QWordConst | String | ConstObj | RevisionOp | DefBuffer
//
_Success_( return )
BOOLEAN
AmlDecoderMatchComputationalData(
	_Inout_   AML_STATE*       State,
	_Out_opt_ AML_OPAQUE_DATA* OutputData
	);

//
// 20.2.6.1. Arg Objects Encoding
// ArgObj := Arg0Op | Arg1Op | Arg2Op | Arg3Op | Arg4Op | Arg5Op | Arg6Op
//
_Success_( return )
BOOLEAN
AmlDecoderMatchArgObj(
	_Inout_   AML_STATE* State,
	_Out_opt_ UINT8*     pArgObjIndex
	);

//
// 20.2.6.2. Local Objects Encoding
// LocalObj := Local0Op | Local1Op | Local2Op | Local3Op | Local4Op | Local5Op | Local6Op | Local7Op
//
_Success_( return )
BOOLEAN
AmlDecoderMatchLocalObj(
	_Inout_   AML_STATE* State,
	_Out_opt_ UINT8*     pLocalObjIndex
	);

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
_Success_( return )
BOOLEAN
AmlDecoderConsumeNameString(
	_Inout_   AML_STATE*       State,
	_Out_opt_ AML_NAME_STRING* NameStringOutput
	);

//
// Attempt to match a name string.
// NameString := <rootchar namepath> | <prefixpath namepath>
//
_Success_( return )
BOOLEAN
AmlDecoderMatchNameString(
	_Inout_   AML_STATE*       State,
	_In_      BOOLEAN          MatchNullName,
	_Out_opt_ AML_NAME_STRING* NameStringOutput
	);

//
// SimpleName := NameString | ArgObj | LocalObj
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeSimpleName(
	_Inout_   AML_STATE*       State,
	_Out_opt_ AML_SIMPLE_NAME* SimpleNameOutput
	);

//
// PackageOp := 0x12
// DefPackage := PackageOp PkgLength NumElements PackageElementList
//
_Success_( return )
BOOLEAN
AmlDecoderMatchDefPackage(
	_Inout_   AML_STATE*               State,
	_Out_opt_ AML_OPAQUE_PACKAGE_DATA* PackageDataOutput
	);

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
	);

//
// DataObject := ComputationalData | DefPackage | DefVarPackage
// Note: does not fully evaluate all data, returns opaque package data!
//
_Success_( return )
BOOLEAN
AmlDecoderMatchDataObject(
	_Inout_   AML_STATE*       State,
	_Out_opt_ AML_OPAQUE_DATA* DataOutput
	);

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
	);

//
// Generically attempt to consume all fixed arguments of the given instruction.
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeInstructionArgs(
	_Inout_ AML_STATE*                            State,
	_In_    const AML_DECODER_INSTRUCTION_OPCODE* Instruction
	);

//
// Attempt to consume a single full instruction (opcode and arguments) without executing it.
//
_Success_( return )
BOOLEAN
AmlDecoderConsumeInstruction(
	_Inout_ AML_STATE* State
	);

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
	);

//
// Attempts to classify the given opcode and determine if it is a statement opcode.
// TODO: Add all this classification stuff to the opcode table!
//
_Success_( return )
BOOLEAN
AmlDecoderIsStatementOpcode(
	_Inout_ AML_STATE*                            State,
	_In_    const AML_DECODER_INSTRUCTION_OPCODE* Op
	);

//
// Attempts to classify the given opcode and determine if it is an expression opcode.
//
_Success_( return )
BOOLEAN
AmlDecoderIsExpressionOpcode(
	_Inout_ AML_STATE*                            State,
	_In_    const AML_DECODER_INSTRUCTION_OPCODE* Op
	);

//
// Attempts to classify the given opcode and determine if it is a namespace modifier object opcode.
// TODO: Add all this classification stuff to the opcode table!
//
_Success_( return )
BOOLEAN
AmlDecoderIsNamespaceModifierObjectOpcode(
	_Inout_ AML_STATE*                            State,
	_In_    const AML_DECODER_INSTRUCTION_OPCODE* Op
	);

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
	);
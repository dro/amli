#include "aml_state.h"
#include "aml_eval.h"
#include "aml_host.h"
#include "aml_conv.h"
#include "aml_debug.h"
#include "aml_mutex.h"
#include "aml_base.h"
#include "aml_field.h"
#include "aml_compare.h"
#include "aml_eval_expression.h"

//
// Perform a single match instruction comparison using the given operator type.
//
_Success_( return )
static
BOOLEAN
AmlMatchCompareValue(
	_Inout_ AML_STATE*      State,
	_In_    const AML_DATA* ConvValue,
	_In_    UINT8           MatchOp,
	_In_    const AML_DATA* MatchData
	)
{
	//
	// Name | Semantics                           | ID | Macro
	// -----+-------------------------------------+----+-------
	// AML_TRUE | A don't care, always returns AML_TRUE   | 0  | MTR
	// EQ   | Returns AML_TRUE if P[i] == MatchObject | 1  | MEQ
	// LE   | Returns AML_TRUE if P[i] <= MatchObject | 2  | MLE
	// LT   | Returns AML_TRUE if P[i] < MatchObject  | 3  | MLT
	// GE   | Returns AML_TRUE if P[i] >= MatchObject | 4  | MGE
	// GT   | Returns AML_TRUE if P[i] > MatchObject  | 5  | MGT
	//
	switch( MatchOp ) {
	case AML_MATCH_OP_MTR:
		return AML_TRUE;
	case AML_MATCH_OP_MEQ:
		return ( AmlCompareData( State, &State->Heap, AML_COMPARISON_TYPE_EQUAL, ConvValue, MatchData ) == AML_COMPARISON_RESULT_AML_TRUE );
	case AML_MATCH_OP_MLE:
		return ( AmlCompareData( State, &State->Heap, AML_COMPARISON_TYPE_LESS_EQUAL, ConvValue, MatchData ) == AML_COMPARISON_RESULT_AML_TRUE );
	case AML_MATCH_OP_MLT:
		return ( AmlCompareData( State, &State->Heap, AML_COMPARISON_TYPE_LESS, ConvValue, MatchData ) == AML_COMPARISON_RESULT_AML_TRUE );
	case AML_MATCH_OP_MGE:
		return ( AmlCompareData( State, &State->Heap, AML_COMPARISON_TYPE_GREATER_EQUAL, ConvValue, MatchData ) == AML_COMPARISON_RESULT_AML_TRUE );
	case AML_MATCH_OP_MGT:
		return ( AmlCompareData( State, &State->Heap, AML_COMPARISON_TYPE_GREATER, ConvValue, MatchData ) == AML_COMPARISON_RESULT_AML_TRUE );
	}
	return AML_FALSE;
}

//
// Perform a full match comparison of a package element (includes conversion of element value).
//
_Success_( return )
static
BOOLEAN
AmlMatchPackageElement(
	_Inout_ AML_STATE*                 State,
	_In_    const AML_PACKAGE_ELEMENT* Element,
	_In_    UINT8                      MatchOp1,
	_In_    const AML_DATA*            MatchData1,
	_In_    UINT8                      MatchOp2,
	_In_    const AML_DATA*            MatchData2
	)
{
	AML_DATA ConvElement1;
	AML_DATA ConvElement2;
	BOOLEAN  IsMatch;

	//
	// The data type of the MatchObject dictates the required type of the package element.
	// If necessary, the package element is implicitly converted to match the type of the MatchObject.
	// If the implicit conversion fails for any reason, the package element is ignored (no match).
	// TODO: ACPICA doesn't seem to properly apply conversions, investigate more (?).
	//
	ConvElement1 = ( AML_DATA ){ .Type = MatchData1->Type };
	ConvElement2 = ( AML_DATA ){ .Type = MatchData2->Type };
	if( AmlConvObjectStore( State, &State->Heap, &Element->Value, &ConvElement1, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlConvObjectStore( State, &State->Heap, &Element->Value, &ConvElement2, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
		AmlDataFree( &ConvElement1 );
		return AML_FALSE;
	}

	//
	// A comparison is performed for each element of the package starting with the index value indicated by StartIndex.
	// If the element of SearchPackage being compared against is called P[i], then the comparison is:
	// If (P[i] Op1 MatchObject1) and (P[i] Op2 MatchObject2) then Match => i is returned.
	//
	IsMatch = ( AmlMatchCompareValue( State, &ConvElement1, MatchOp1, MatchData1 )
				&& AmlMatchCompareValue( State, &ConvElement2, MatchOp2, MatchData2 ) );

	//
	// Free temporary converted element values.
	//
	AmlDataFree( &ConvElement1 );
	AmlDataFree( &ConvElement2 );
	return IsMatch;
}

//
// DefMatch := MatchOp SearchPkg MatchOpcode Operand MatchOpcode Operand StartIndex
//
_Success_( return )
static
BOOLEAN
AmlEvalMatch(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode,
	_Out_   AML_DATA*  EvalResult
	)
{
	AML_DATA             SearchPkg;
	UINT8                MatchOpcode1;
	AML_DATA             MatchData1;
	UINT8                MatchOpcode2;
	AML_DATA             MatchData2;
	AML_DATA             StartIndex;
	UINT64               i;
	AML_PACKAGE_ELEMENT* Element;
	BOOLEAN              IsMatch;

	//
	// Consume the opcode if the caller hasn't already done it for us.
	//
	if( ConsumeOpcode ) {
		if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_MATCH_OP, NULL ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	//
	// SearchPkg := TermArg => Package
	// MatchOpcode := ByteData
	// MatchObject := TermArg => ComputationalData
	// StartIndex := TermArg => Integer
	// DefMatch := MatchOp SearchPkg MatchOpcode MatchObject MatchOpcode MatchObject StartIndex
	//
	if( AmlEvalTermArgToType( State, AML_EVAL_TERM_ARG_FLAG_TEMP, AML_DATA_TYPE_PACKAGE, &SearchPkg ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlDecoderConsumeByte( State, &MatchOpcode1 ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &MatchData1 ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlDecoderConsumeByte( State, &MatchOpcode2 ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &MatchData2 ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &StartIndex ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// SearchPackage is evaluated to a package object and is treated as a one-dimension array.
	// Each package element must evaluate to either an integer, a string, or a buffer.
	// Uninitialized package elements and elements that do not evaluate to integers, strings, or buffers are ignored. 
	//
	IsMatch = AML_FALSE;
	for( i = StartIndex.u.Integer; i < SearchPkg.u.Package->ElementCount; i++ ) {
		Element = SearchPkg.u.Package->Elements[ i ];
		if( Element == NULL ) {
			continue;
		}

		//
		// Uninitialized package elements and elements that do not evaluate to integers, strings, or buffers are ignored.
		// TODO: Verify exactly what this means. Are references followed? are field units valid?
		//
		switch( Element->Value.Type ) {
		case AML_DATA_TYPE_INTEGER:
		case AML_DATA_TYPE_STRING:
		case AML_DATA_TYPE_BUFFER:
			break;
		default:
			continue;
		}

		//
		// Perform the full match comparison for this element (including conversions).
		// If (P[i] Op1 MatchObject1) and (P[i] Op2 MatchObject2)
		//
		IsMatch = AmlMatchPackageElement( State, Element, MatchOpcode1, &MatchData1, MatchOpcode2, &MatchData2 );
		if( IsMatch ) {
			break;
		}		
	}

	//
	// If the comparison succeeds, the index of the element that succeeded is returned;
	// otherwise, the constant object Ones is returned.
	//
	*EvalResult = ( AML_DATA ){
		.Type      = AML_DATA_TYPE_INTEGER,
		.u.Integer = ( IsMatch ? i : ~0ull )
	};

	//
	// Release all evaluated resources.
	//
	AmlDataFree( &SearchPkg );
	AmlDataFree( &MatchData1 );
	AmlDataFree( &MatchData2 );
	AmlDataFree( &StartIndex );
	return AML_TRUE;
}

//
// Convert a null-terminated type-name constant to an AML_DATA string.
// TODO: Move this out of here and into aml_data.
//
_Success_( return )
static
BOOLEAN
AmlTypeNameToStringData(
	_Inout_ AML_STATE*  State,
	_In_z_  const CHAR* TypeName,
	_Out_   AML_DATA*   Data
	)
{
	SIZE_T           TypeNameLength;
	AML_BUFFER_DATA* TypeNameBuffer;
	SIZE_T           i;

	//
	// Calculate the length of the type name string.
	//
	for( TypeNameLength = 0; TypeNameLength < ( SIZE_MAX - 1 ); TypeNameLength++ ) {
		if( TypeName[ TypeNameLength ] == '\0' ) {
			break;
		}
	}

	//
	// Allocate a buffer for the typename.
	//
	TypeNameBuffer = AmlBufferDataCreate( &State->Heap, TypeNameLength, ( TypeNameLength + 1 ) );
	if( TypeNameBuffer == NULL ) {
		return AML_FALSE;
	}

	//
	// Copy over the constant string to the new buffer allocation.
	//
	for( i = 0; i < TypeNameLength; i++ ) {
		TypeNameBuffer->Data[ i ] = TypeName[ i ];
	}
	TypeNameBuffer->Data[ TypeNameLength ] = '\0';

	//
	// Return a string containing the converted string type-name of the object.
	//
	*Data = ( AML_DATA ){
		.Type     = AML_DATA_TYPE_STRING,
		.u.String = TypeNameBuffer
	};

	return AML_TRUE;
}

//
// Get the concat value for the input data.
// This is the underlying data if an the type is Integer, String, or Buffer.
// Otherwise, this is a stringified name of the data type.
// TODO: Move this out of here and into aml_data.
//
_Success_( return )
static
BOOLEAN
AmlDataToConcatValue(
	_Inout_ AML_STATE*      State,
	_In_    const AML_DATA* Input,
	_Out_   AML_DATA*       Output
	)
{
	//
	// If this is a named object and the underlying data type is Integer|String|Buffer, return the value directly.
	//
	switch( Input->Type ) {
	case AML_DATA_TYPE_INTEGER:
	case AML_DATA_TYPE_STRING:
	case AML_DATA_TYPE_BUFFER:
		return AmlDataDuplicate( Input, &State->Heap, Output );
	default:
		break;
	}

	//
	// This was not an integer, string, or buffer. Return a string of the name of the data type.
	//
	return AmlTypeNameToStringData( State, AmlDataToAcpiTypeName( Input ), Output );
}

//
// Evaluate a concat operand to a regular data type.
// For the basic data object types (Integer, String, or Buffer), the value is returned.
// For all other object types, a string object is created that contains the name (type) of the object. 
//
_Success_( return )
static
BOOLEAN
AmlEvalConcatOperand(
	_Inout_ AML_STATE* State,
	_Out_   AML_DATA*  EvalResult
	)
{
	AML_NAME_STRING     NameString;
	AML_DATA            TempValue;
	AML_NAMESPACE_NODE* NsNode;

	//
	// First try to match a name string, for the case where we are referring to an object (that isn't a named value).
	//
	if( AmlDecoderMatchNameString( State, AML_FALSE, &NameString ) ) {
		//
		// Attempt to resolve the given named object.
		//
		if( AmlNamespaceSearch( &State->Namespace, NULL, &NameString, 0, &NsNode ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// If this is a method, we must evaluate an actual MethodInvocation and use the return value.
		//
		if( NsNode->Object->Type == AML_OBJECT_TYPE_METHOD ) {
			if( AmlEvalMethodInvocationInternal( State, NsNode->Object, 0, &TempValue ) == AML_FALSE ) {
				return AML_FALSE;
			} else if( AmlDataToConcatValue( State, &TempValue, EvalResult ) == AML_FALSE ) {
				return AML_FALSE;
			}
			AmlDataFree( &TempValue );
			return AML_TRUE;
		}

		//
		// If this is a named value object, mark this as regular data, and combine behavior with the method return value path.
		//
		if( NsNode->Object->Type == AML_OBJECT_TYPE_NAME ) {
			return AmlDataToConcatValue( State, &NsNode->Object->u.Name.Value, EvalResult );
		}

		//
		// This is an object without an underlying data type that is usable for concatenation.
		// In this case, the name of the object type should be converted to a string and used.
		//
		return AmlTypeNameToStringData( State, AmlObjectToAcpiTypeName( NsNode->Object ), EvalResult );
	}

	//
	// A name string wasn't matched, try to evaluate a regular TermArg to data.
	//
	if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &TempValue ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlDataToConcatValue( State, &TempValue, EvalResult ) == AML_FALSE ) {
		return AML_FALSE;
	}

	AmlDataFree( &TempValue );
	return AML_TRUE;
}

//
// DefConcat := ConcatOp Data Data Target
//
_Success_( return )
static
BOOLEAN
AmlEvalConcat(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode,
	_Out_   AML_DATA*  EvalResult
	)
{
	AML_DATA         Source1;
	AML_DATA         Source2;
	AML_OBJECT*      Target;
	AML_DATA         ConvSource2;
	SIZE_T           BufferSize;
	AML_BUFFER_DATA* Buffer;

	//
	// Consume the opcode if the caller hasn't already done it for us.
	//
	if( ConsumeOpcode ) {
		if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_CONCAT_OP, NULL ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}
	
	//
	// Data := TermArg => ComputationalData
	// DefConcat := ConcatOp Data Data Target
	// The AML spec says that the data arguments should evaluate to ComputationalData,
	// but in practice this doesn't seem to be correct. The ASL spec says they can be
	// any valid ACPI object, and in practice, this is how it works in ACPICA.
	// For example: (Device(DEV0){} Concatenate("String: ", DEV0)) => "String: [Device Object]".
	//
	if( AmlEvalConcatOperand( State, &Source1 ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalConcatOperand( State, &Source2 ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// The data type of Source1 dictates the required type of Source2 and the type of the result object.
	// Source2 is implicitly converted if necessary (and possible) to match the type of Source1.
	// 
	// Source1 Type     | Source2 Converted Type           | Result Type
	// -----------------+----------------------------------+-------------
	// Integer          | Integer/String/Buffer => Integer | Buffer
	// String           | All types => String              | String
	// Buffer           | All types => Buffer              | Buffer
	// Others => String | All types => String              | String
	//
	switch( Source1.Type ) {
	case AML_DATA_TYPE_INTEGER:
		//
		// Convert the second source operand to an integer to match the type of the first source operand.
		//
		ConvSource2 = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER };
		if( AmlConvObjectStore( State, &State->Heap, &Source2, &ConvSource2, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Create a buffer with space for two integers.
		//
		BufferSize = ( State->IsIntegerSize64 ? 16 : 8 );
		if( ( Buffer = AmlBufferDataCreate( &State->Heap, BufferSize, BufferSize ) ) == NULL ) {
			return AML_FALSE;
		}

		//
		// Concatenate the two integer values to the buffer data.
		//
		if( State->IsIntegerSize64 ) {
			AML_MEMCPY( &Buffer[ 0 ], &Source1.u.Integer, sizeof( UINT64 ) );
			AML_MEMCPY( &Buffer[ 8 ], &ConvSource2.u.Integer, sizeof( UINT64 ) );
		} else {
			AML_MEMCPY( &Buffer[ 0 ], &( UINT32 ){ ( UINT32 )Source1.u.Integer }, sizeof( UINT32 ) );
			AML_MEMCPY( &Buffer[ 4 ], &( UINT32 ){ ( UINT32 )ConvSource2.u.Integer }, sizeof( UINT32 ) );
		}

		//
		// Output the concatenated result.
		//
		*EvalResult = ( AML_DATA ){ .Type = AML_DATA_TYPE_BUFFER, .u.Buffer = Buffer };
		break;
	case AML_DATA_TYPE_STRING:
		//
		// Convert the second source operand to an string to match the type of the first source operand.
		//
		ConvSource2 = ( AML_DATA ){ .Type = AML_DATA_TYPE_STRING };
		if( AmlConvObjectStore( State, &State->Heap, &Source2, &ConvSource2, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( Source1.u.String->Size >= SIZE_MAX) {
			return AML_FALSE;
		} else if( ConvSource2.u.String->Size > ( SIZE_MAX - 1 - Source1.u.String->Size ) ) {
			return AML_FALSE;
		}

		//
		// Create a buffer with space for the two strings.
		//
		BufferSize = ( Source1.u.String->Size + ConvSource2.u.String->Size );
		if( ( Buffer = AmlBufferDataCreate( &State->Heap, BufferSize, ( BufferSize + 1 ) ) ) == NULL ) {
			return AML_FALSE;
		}

		//
		// Concatenate the two string values to the buffer data.
		//
		AML_MEMCPY( &Buffer->Data[ 0 ], &Source1.u.String->Data[ 0 ], Source1.u.String->Size );
		AML_MEMCPY( &Buffer->Data[ Source1.u.String->Size ], &ConvSource2.u.String->Data[ 0 ], ConvSource2.u.String->Size );
		Buffer->Data[ BufferSize ] = '\0';

		//
		// Output the concatenated result.
		//
		*EvalResult = ( AML_DATA ){ .Type = AML_DATA_TYPE_STRING, .u.Buffer = Buffer };
		break;
	case AML_DATA_TYPE_BUFFER:
		//
		// Convert the second source operand to a buffer to match the type of the first source operand.
		//
		ConvSource2 = ( AML_DATA ){ .Type = AML_DATA_TYPE_BUFFER };
		if( AmlConvObjectStore( State, &State->Heap, &Source2, &ConvSource2, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( ConvSource2.u.Buffer->Size > ( SIZE_MAX - Source1.u.Buffer->Size ) ) {
			return AML_FALSE;
		}

		//
		// Create a buffer with space for the two buffers.
		//
		BufferSize = ( Source1.u.Buffer->Size + ConvSource2.u.Buffer->Size );
		if( ( Buffer = AmlBufferDataCreate( &State->Heap, BufferSize, BufferSize ) ) == NULL ) {
			return AML_FALSE;
		}

		//
		// Concatenate the two buffer values to the buffer data.
		//
		AML_MEMCPY( &Buffer->Data[ 0 ], &Source1.u.Buffer->Data[ 0 ], Source1.u.Buffer->Size );
		AML_MEMCPY( &Buffer->Data[ Source1.u.Buffer->Size ], &ConvSource2.u.Buffer->Data[ 0 ], ConvSource2.u.Buffer->Size );

		//
		// Output the concatenated result.
		//
		*EvalResult = ( AML_DATA ){ .Type = AML_DATA_TYPE_BUFFER, .u.Buffer = Buffer };
		break;
	default:
		AML_DEBUG_ERROR( State, "Error: Invalid AmlEvalConcat case, all types should be converted to Integer/Buffer/String." );
		return AML_FALSE;
	}

	//
	// Release all source values.
	//
	AmlDataFree( &ConvSource2 );
	AmlDataFree( &Source2 );
	AmlDataFree( &Source1 );

	//
	// Optionally write output to the target if given.
	//
	if( Target != NULL ) {
		if( AmlOperandStore( State, &State->Heap, EvalResult, Target, AML_TRUE ) == AML_FALSE ) {
			return AML_FALSE;
		}
		AmlObjectRelease( Target );
	}

	return AML_TRUE;
}

//
// DefConcatRes := ConcatResOp BufData BufData Target
//
_Success_( return )
static
BOOLEAN
AmlEvalConcatRes(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode,
	_Out_   AML_DATA*  EvalResult
	)
{
	AML_DATA         Source1;
	AML_DATA         Source2;
	AML_OBJECT*      Target;
	SIZE_T           Length1;
	SIZE_T           Length2;
	AML_BUFFER_DATA* Buffer;
	BOOLEAN          Success;

	//
	// Consume the opcode if the caller hasn't already done it for us.
	//
	if( ConsumeOpcode ) {
		if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_CONCAT_RES_OP, NULL ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	//
	// BufData := TermArg => Buffer
	// DefConcatRes := ConcatResOp BufData BufData Target
	//
	if( AmlEvalTermArgToType( State, AML_EVAL_TERM_ARG_FLAG_TEMP, AML_DATA_TYPE_BUFFER, &Source1 ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTermArgToType( State, AML_EVAL_TERM_ARG_FLAG_TEMP, AML_DATA_TYPE_BUFFER, &Source2 ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// The resource descriptors from Source2 are appended to the resource descriptors from Source1.
	// Then a new end tag and checksum are appended and the result is stored in Result, if specified
	// If either Source1 or Source2 is exactly 1 byte in length, a run-time error occurs.
	// An empty buffer is treated as a resource template with only an end tag.
	// We diverge from the spec here and treat buffers with 1 byte as empty.
	//
	Length1 = ( ( Source1.u.Buffer->Size >= 2 ) ? ( Source1.u.Buffer->Size - 2 ) : 0 );
	Length2 = ( ( Source2.u.Buffer->Size >= 2 ) ? ( Source2.u.Buffer->Size - 2 ) : 0 );

	//
	// Create a new buffer large enough to hold both concatenated buffers and one end tag.
	//
	if( ( Buffer = AmlStateSnapshotCreateBufferData( State, &State->Heap, ( Length1 + Length2 + 2 ), 0 ) ) == NULL ) {
		return AML_FALSE;
	}

	//
	// Concatenate both buffers (not including their end tags).
	// If the checksum field is zero, the resource data is treated as if the checksum operation succeeded.
	// Configuration proceeds normally.
	//
	AML_MEMCPY( &Buffer->Data[ 0 ], Source1.u.Buffer->Data, Length1 );
	AML_MEMCPY( &Buffer->Data[ Length1 ], Source2.u.Buffer->Data, Length2 );
	Buffer->Data[ Length1 + Length2 + 0 ] = 0x79; /* Type 0, Small Item Name 0xF, Length = 1 */
	Buffer->Data[ Length1 + Length2 + 1 ] = 0;    /* Zero checksum, ignore. */

	//
	// Return the concatenated resource descriptor buffers.
	//
	*EvalResult = ( AML_DATA ){ .Type = AML_DATA_TYPE_BUFFER, .u.Buffer = Buffer };

	//
	// Release all source values.
	//
	AmlDataFree( &Source2 );
	AmlDataFree( &Source1 );

	//
	// Optionally write output to the target if given.
	//
	Success = AML_TRUE;
	if( Target != NULL ) {
		Success = AmlOperandStore( State, &State->Heap, EvalResult, Target, AML_TRUE );
		AmlObjectRelease( Target );
	}

	return Success;
}

//
// DefMid := MidOp MidObj TermArg TermArg Target
//
_Success_( return )
static
BOOLEAN
AmlEvalMid(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode,
	_Out_   AML_DATA*  EvalResult
	)
{
	AML_DATA         MidObj;
	AML_DATA         Index;
	AML_DATA         Length;
	AML_OBJECT*      Target;
	SIZE_T           ResultSize;
	AML_DATA         Result;
	AML_BUFFER_DATA* BufferData;
	SIZE_T           i;
	BOOLEAN          Success;

	//
	// Consume the opcode if the caller hasn't already done it for us.
	//
	if( ConsumeOpcode ) {
		if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_MID_OP, NULL ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	//
	// MidObj := TermArg => Buffer | String
	// Index := TermArg => Integer
	// Length := TermArg => Integer
	// DefMid := MidOp MidObj Index Length Target
	//
	if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &MidObj ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Index ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Length ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Length bytes, starting with the Indexth byte (zero-based) are optionally copied into Result.
	//
	switch( MidObj.Type ) {
	case AML_DATA_TYPE_BUFFER:
		//
		// If Index is greater than or equal to the length of the buffer, then the result is an empty buffer.
		// Otherwise, if Index + Length is greater than or equal to the length of the buffer,
		// then only bytes up to and including the last byte are included in the result.
		//
		Length.u.Integer = AML_MIN( Length.u.Integer, SIZE_MAX );
		if( Index.u.Integer >= MidObj.u.Buffer->Size ) {
			ResultSize = 0;
		} else {
			ResultSize = ( SIZE_T )AML_MIN( Length.u.Integer, ( MidObj.u.Buffer->Size - ( SIZE_T )Index.u.Integer ) );
		}

		//
		// Allocate new buffer data for the mid result.
		//
		if( ( BufferData = AmlBufferDataCreate( &State->Heap, ResultSize, ResultSize ) ) == NULL ) {
			return AML_FALSE;
		}

		//
		// Copy data into the result buffer.
		//
		for( i = 0; i < ResultSize; i++ ) {
			BufferData->Data[ i ] = MidObj.u.Buffer->Data[ Index.u.Integer + i ];
		}
		Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_BUFFER, .u.Buffer = BufferData };
		break;
	case AML_DATA_TYPE_STRING:
		//
		// If Index is greater than or equal to the length of the buffer, then the result is an empty string.
		// Otherwise, if Index + Length is greater than or equal to the length of the string,
		// then only bytes up to an including the last character are included in the result.
		//
		Length.u.Integer = AML_MIN( Length.u.Integer, SIZE_MAX );
		if( Index.u.Integer >= MidObj.u.String->Size ) {
			ResultSize = 0;
		} else {
			ResultSize = ( SIZE_T )AML_MIN( Length.u.Integer, ( MidObj.u.String->Size - ( SIZE_T )Index.u.Integer ) );
		}

		//
		// Allocate new buffer data for the mid result.
		//
		if( ( BufferData = AmlBufferDataCreate( &State->Heap, ResultSize, ResultSize ) ) == NULL ) {
			return AML_FALSE;
		}

		//
		// Copy data into the result buffer.
		//
		for( i = 0; i < ResultSize; i++ ) {
			BufferData->Data[ i ] = MidObj.u.String->Data[ Index.u.Integer + i ];
		}
		Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_STRING, .u.String = BufferData };
		break;
	default:
		AML_DEBUG_ERROR( State, "Error: unsupported MidObj type.\n" );
		AmlDataFree( &MidObj );
		AmlObjectRelease( Target );
		return AML_FALSE;
	}

	//
	// Optionally store the created result to the target (if given).
	//
	if( Target != NULL ) {
		Success = AmlOperandStore( State, &State->Heap, &Result, Target, AML_TRUE );
	} else {
		Success = AML_TRUE;
	}

	//
	// Return created result.
	//
	*EvalResult = Result;

	//
	// Release evaluated resources.
	//
	AmlDataFree( &MidObj );
	AmlObjectRelease( Target );
	return Success;
}

//
// DefObjectType := ObjectTypeOp <SimpleName | DebugObj | DefRefOf | DefDerefOf | DefIndex>
//
_Success_( return )
static
BOOLEAN
AmlEvalObjectType(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode,
	_Out_   AML_DATA*  EvalResult
	)
{
	AML_OBJECT* Object;

	//
	// Consume the opcode if the caller hasn't already done it for us.
	//
	if( ConsumeOpcode ) {
		if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_OBJECT_TYPE_OP, NULL ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	//
	// Handle debug objects specially, as they are not an actual AML_OBJECT in our system.
	// 16 = Debug Object.
	//
	if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_DEBUG_OP, NULL ) ) {
		*EvalResult = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = 16 };
		return AML_TRUE;
	}

	//
	// <SimpleName | DefRefOf | DefDerefOf | DefIndex>
	//
	if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_REF_OF_OP, NULL ) ) {
		if( AmlEvalRefOf( State, AML_FALSE, &Object ) == AML_FALSE ) {
			return AML_FALSE;
		}
	} else if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_DEREF_OF_OP, NULL ) ) {
		if( AmlEvalDerefOf( State, AML_FALSE, &Object ) == AML_FALSE ) {
			return AML_FALSE;
		}
	} else if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_INDEX_OP, NULL ) ) {
		if( AmlEvalIndex( State, AML_FALSE, &Object ) == AML_FALSE ) {
			return AML_FALSE;
		}
	} else if( AmlEvalSimpleName( State, 0, 0, &Object ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Attempt to resolve the ACPI object type value for the given object.
	// TODO: returns 0/uninitialized for unsupported types, maybe change in the future.
	//
	*EvalResult = ( AML_DATA ){ 
		.Type      = AML_DATA_TYPE_INTEGER,
		.u.Integer = AmlObjectToAcpiObjectType( Object )
	};

	return AML_TRUE;
}

//
// Find the first set bit starting from the MSB of the input value.
// The result of 0 means no bit was set.
// 1 means the left-most bit set is the first bit,
// 2 means the left-most bit set is the second bit, and so on.
//
static
UINT64
AmlFindSetLeftBit(
	_In_ UINT64  Value,
	_In_ BOOLEAN IsInteger32
	)
{
	UINT64 IsInteger32Mask;
	UINT64 LzCount;
	UINT64 IsZeroValueMask;

	//
	// Create a mask of all bits set if IsInteger32, 0 if unset.
	// (IsInteger32 ? MAXUINT64 : 0)
	//
	IsInteger32 = ( IsInteger32 ? 1 : 0 );
	IsInteger32Mask = ( 1 + ~( ( UINT64 )IsInteger32 & 1 ) );

	//
	// A result of 0 means no bit was set.
	//
	if( ( Value & IsInteger32Mask ) == 0 ) {
		return 0;
	}

	//
	// Strip the upper 32 bits of the 64 bit value if IsInteger32 is enabled.
	//
	Value &= ~( ( 0xFFFFFFFFull << 32ull ) & IsInteger32Mask );

	//
	// Count leading (from msb) zero bits, if IsInteger32, the return value begins at 32.
	//
	LzCount = AML_LZCNT64( Value );

	//
	// Create a mask of all bits set if the value is non-zero, otherwise, a mask of zero.
	// This is used to disregard the LZ count if the input value is zero, and return zero instead.
	//
	IsZeroValueMask = ( 1 + ~( Value ? 1 : 0 ) );

	//
	// The result of 0 means no bit was set.
	// 1 means the left-most bit set is the first bit,
	// 2 means the left-most bit set is the second bit, and so on.
	//
	return ( ( 64 - LzCount ) & IsZeroValueMask );
}

//
// Find the first set bit starting from the LSB of the input value.
// The result of 0 means no bit was set.
// 1 means the left-most bit set is the first bit,
// 2 means the left-most bit set is the second bit, and so on.
//
static
UINT64
AmlFindSetRightBit(
	_In_ UINT64  Value,
	_In_ BOOLEAN IsInteger32
	)
{
	UINT64 IsInteger32Mask;
	UINT64 TzCount;
	UINT64 IsZeroValueMask;

	//
	// Create a mask of all bits set if IsInteger32, 0 if unset.
	// (IsInteger32 ? MAXUINT64 : 0)
	//
	IsInteger32 = ( IsInteger32 ? 1 : 0 );
	IsInteger32Mask = ( ~( ( UINT64 )IsInteger32 & 1 ) + 1 );

	//
	// A result of 0 means no bit was set.
	//
	if( ( Value & IsInteger32Mask ) == 0 ) {
		return 0;
	}

	//
	// Force the upper 32 bits of the 64 bit value to be set if IsInteger32 is enabled.
	//
	Value |= ( ( 0xFFFFFFFFull << 32ull ) & IsInteger32Mask );

	//
	// Count trailing (from lsb) zero bits.
	//
	TzCount = AML_TZCNT64( Value );

	//
	// Create a mask of all bits set if the value is non-zero, otherwise, a mask of zero.
	// This is used to disregard the TZ count if the input value is zero, and return zero instead.
	//
	IsZeroValueMask = ( ~0ull & ~( ( 0xFFFFFFFFull << 32ull ) & IsInteger32 ) ); /* ( IsInteger32 ? MAXUINT64 : MAXUINT32 ) */
	IsZeroValueMask = ( Value & IsZeroValueMask );
	IsZeroValueMask = ( 1 + ~( IsZeroValueMask ? 1 : 0 ) );

	//
	// The result of 0 means no bit was set.
	// 1 means the left-most bit set is the first bit,
	// 2 means the left-most bit set is the second bit, and so on.
	//
	return ( TzCount & IsZeroValueMask );
}

//
// Attempt to evaluate 2 TermArg source operands, and one optional Target SuperName.
// Output operands must be freed by the caller, and the TargetObject must be released by the caller.
//
_Success_( return )
static
BOOLEAN
AmlEvalBinaryIntegerArithmeticArguments(
	_Inout_  AML_STATE*   State,
	_Out_    AML_DATA*    Operand1,
	_Out_    AML_DATA*    Operand2,
	_Outptr_ AML_OBJECT** ppTarget
	)
{
	//
	// Target := SuperName | NullName
	// Operand Operand Target
	//
	if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, Operand1 ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, Operand2 ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTarget( State, 0, ppTarget ) == AML_FALSE ) {
		return AML_FALSE;
	}
	return AML_TRUE;
}

//
// Read data from all source object types supported by the Load instruction.
// Operation Region, Operation Region Field, Buffer.
//
_Success_( return )
static
BOOLEAN
AmlReadFromLoadSource(
	_Inout_                            AML_STATE*  State,
	_In_                               AML_OBJECT* Source,
	_Out_writes_bytes_all_( ReadSize ) VOID*       ReadData,
	_In_                               SIZE_T      ReadSize
	)
{
	AML_OBJECT_OPERATION_REGION* OpRegion;
	SIZE_T                       i;
	AML_BUFFER_DATA*             Buffer;
	SIZE_T                       ReadBitCount;

	//
	// Read data from all source object types supported by the Load instruction.
	//
	switch( Source->Type ) {
	case AML_OBJECT_TYPE_OPERATION_REGION:
		OpRegion = &Source->u.OpRegion;
		if( OpRegion->SpaceType != AML_REGION_SPACE_TYPE_SYSTEM_MEMORY ) {
			return AML_FALSE;
		} else if( OpRegion->Length < ReadSize ) {
			return AML_FALSE;
		}
		for( i = 0; i < ReadSize; i++ ) {
			if( AmlOperationRegionRead( State,
										OpRegion,
										NULL,
										i,
										AML_FIELD_ACCESS_TYPE_ANY_ACC,
										0,
										( sizeof( UINT8 ) * CHAR_BIT ),
										( ( UINT8* )ReadData + i ),
										sizeof( UINT8 ) ) == AML_FALSE )
			{
				return AML_FALSE;
			}
		}
		break;
	case AML_OBJECT_TYPE_NAME:
		if( Source->u.Name.Value.Type != AML_DATA_TYPE_BUFFER ) {
			return AML_FALSE;
		}
		Buffer = Source->u.Name.Value.u.Buffer;
		if( ( Buffer == NULL ) || ( Buffer->Size < ReadSize ) ) {
			return AML_FALSE;
		}
		for( i = 0; i < ReadSize; i++ ) {
			( ( CHAR* )ReadData )[ i ] = Buffer->Data[ i ];
		}
		break;
	case AML_OBJECT_TYPE_FIELD:
	case AML_OBJECT_TYPE_BANK_FIELD:
	case AML_OBJECT_TYPE_INDEX_FIELD:
		if( AmlFieldUnitRead( State, Source, ReadData, ReadSize, AML_TRUE, &ReadBitCount ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( ( ReadBitCount / CHAR_BIT ) != ReadSize ) {
			return AML_FALSE;
		}
		break;
	default:
		return AML_FALSE;
	}

	return AML_TRUE;
}

//
// DefLoad := LoadOp NameString Target
//
_Success_( return )
static
BOOLEAN
AmlEvalLoad(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode,
	_Out_   AML_DATA*  Result
	)
{
	AML_NAME_STRING        NameString;
	AML_NAMESPACE_NODE*    NsNode;
	AML_OBJECT*            Object;
	AML_OBJECT*            Target;
	BOOLEAN                IsValidType;
	AML_DESCRIPTION_HEADER Header;
	VOID*                  TableData;
	SIZE_T                 CodeSize;
	const UINT8*           CodeData;

	//
	// Consume the opcode if the caller hasn't already done it for us.
	//
	if( ConsumeOpcode ) {
		if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_LOAD_OP, NULL ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	//
	// DefLoad := LoadOp NameString Target
	//
	if( AmlDecoderConsumeNameString( State, &NameString ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// The NameString specifies the path to an object.
	//
	if( AmlNamespaceSearch( &State->Namespace, NULL, &NameString, 0, &NsNode ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( ( Object = NsNode->Object ) == NULL ) {
		return AML_FALSE;
	}

	//
	// The Object parameter can refer to one of the following object types:
    //  1. An operation region field (TODO: Support IndexField?)
    //  2. An operation region directly
    //  3. A Buffer object
	// If the object is an operation region, the operation region must be in SystemMemory space.
	//
	switch( Object->Type ) {
	case AML_OBJECT_TYPE_OPERATION_REGION:
		IsValidType = ( Object->u.OpRegion.SpaceType == AML_REGION_SPACE_TYPE_SYSTEM_MEMORY );
		break;
	case AML_OBJECT_TYPE_FIELD:
	case AML_OBJECT_TYPE_BANK_FIELD:
		IsValidType = AML_TRUE;
		break;
	case AML_OBJECT_TYPE_NAME:
		IsValidType = ( Object->u.Name.Value.Type == AML_DATA_TYPE_BUFFER );
		break;
	default:
		IsValidType = AML_FALSE;
		break;
	}

	//
	// Ensure that the referenced object is of a valid type for the Load operation.
	//
	if( IsValidType == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: Unsupported object type in Load (%u).\n", ( UINT )Object->Type );
		return AML_FALSE;
	}

	//
	// Initialize output result to unsucessful by default, we allow some cases below to gracefully fail.
	// These graceful failure cases result in internal eval success but write out a unsucessful result value to AML.
	// TODO: This is not true yet, implement graceful failure (resources must be properly freed in this case).
	//
	*Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = 0 };

	//
	// The Definition Block should contain an ACPI DESCRIPTION_HEADER of type SSDT.
	//
	if( AmlReadFromLoadSource( State, Object, &Header, sizeof( Header ) ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( Header.Signature != 0x54445353 ) {
		AML_DEBUG_ERROR( State, "Error: Invalid table signature in Load (0x%x)\n", Header.Signature );
		return AML_FALSE;
	}

	//
	// Allocate space for the entire definition block.
	//
	TableData = AmlHeapAllocate( &State->Heap, Header.Length );
	if( TableData == NULL ) {
		AML_DEBUG_ERROR( State, "Error: Failed to allocate memory for table in Load (size: 0x%x)\n", Header.Length );
		return AML_FALSE;
	}

	//
	// Attempt to load the entire definition block from the source object.
	//
	if( AmlReadFromLoadSource( State, Object, TableData, Header.Length ) == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: Failed to read entire table to memory in Load (size: 0x%x)\n", Header.Length );
		AmlHeapFree( &State->Heap, TableData );
		return AML_FALSE;
	}

	//
	// Change the decoder data to point to the table's data,
	// and attempt to fully execute the given table.
	//
	if( Header.Length > sizeof( Header ) ) {
		CodeSize = ( Header.Length - sizeof( Header ) );
		CodeData = ( ( const UINT8* )TableData + sizeof( Header ) );
		if( AmlEvalLoadedTableCode( State, CodeData, CodeSize, NULL ) == AML_FALSE ) {
			AML_DEBUG_ERROR( State, "Error: Failed to load table - OemID=%.6s\n", Header.OemId.Data );
			AmlHeapFree( &State->Heap, TableData );
			return AML_FALSE;
		}
	}

	//
	// Table was successfully loaded, or there were no table contents to load, indicate success to the caller.
	//
	*Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = ~0ull };
	if( Target != NULL ) {
		AmlOperandStore( State, &State->Heap, Result, Target, AML_TRUE );
	}
	AML_DEBUG_TRACE( State, "Loaded table! - %p - OemID=%.6s - Length=0x%x\n", TableData, Header.OemId.Data, Header.Length );

	//
	// Release the evaluated arguments.
	//
	AmlObjectRelease( Target );
	return AML_TRUE;
}


//
// DefLoadTable := LoadTableOp TermArg TermArg TermArg TermArg TermArg TermArg
//
_Success_( return )
static
BOOLEAN
AmlEvalLoadTableInternal(
	_Inout_ AML_STATE*      State,
	_Inout_ AML_ARENA*      TempArena,
	_In_    const AML_DATA* Signature,
	_In_    const AML_DATA* OemId,
	_In_    const AML_DATA* OemTableId,
	_In_    const AML_DATA* RootPath,
	_In_    const AML_DATA* ParameterPath,
	_In_    const AML_DATA* ParameterData,
	_Out_   AML_DATA*       Result
	)
{
	AML_DESCRIPTION_HEADER  Search;
	AML_NAME_STRING         RootPathNameString;
	AML_DESCRIPTION_HEADER* InputTable;
	VOID*                   LoadedTable;
	BOOLEAN                 LoadSuccess;
	SIZE_T                  CodeSize;
	const UINT8*            CodeData;
	AML_NAME_STRING         ParameterPathNameString;
	AML_NAMESPACE_NODE*     ParameterNsNode;

	//
	// Default to an unsucessful result, graceful failure is possible here.
	//
	*Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = 0 };

	//
	// Validate all input string sizes.
	//
	if( ( Signature->u.String->Size > sizeof( Search.Signature ) )
		|| ( OemId->u.String->Size > sizeof( Search.OemId ) )
		|| ( OemTableId->u.String->Size > sizeof( Search.OemTableId ) ) )
	{
		return AML_TRUE;
	}
	
	//
	// Attempt to parse the input root path string to a proper AML_NAME_STRING, will also validate format of the path string.
	// If RootPathString is not specified, "\" is assumed.
	//
	if( RootPath->u.String->Size != 0 ) {
		if( AmlPathStringToNameString( TempArena,
									   ( const CHAR* )RootPath->u.String->Data,
									   ( RootPath->u.String->Size / sizeof( CHAR ) ),
									   &RootPathNameString ) == AML_FALSE )
		{
			AML_DEBUG_ERROR( State, "Error: Failed to parse RootPathString in LoadTable (%s)\n", RootPath->u.String->Data );
			return AML_TRUE;
		}
	} else {
		RootPathNameString = ( AML_NAME_STRING ){ .Prefix = { .Data = { '\\' }, .Length = 1 } };
	}

	//
	// Convert the input strings to the actual ACPI table header format.
	// Zero-pads strings that are a lower size then the actual fields.
	//
	Search = ( AML_DESCRIPTION_HEADER ){ 0 };
	AML_MEMCPY( &Search.Signature, Signature->u.String->Data, Signature->u.String->Size );
	AML_MEMCPY( &Search.OemId, OemId->u.String->Data, OemId->u.String->Size );
	AML_MEMCPY( &Search.OemTableId, OemTableId->u.String->Data, OemTableId->u.String->Size );

	//
	// Use the host interface to search for the given table information.
	// Validate the properties of the found table header, must always at least have space for a DESCRIPTION_HEADER.
	//
	InputTable = AmlHostSearchAcpiTableEx( State->Host, Search.Signature, Search.OemId, Search.OemTableId );
	if( ( InputTable == NULL ) || ( InputTable->Length < sizeof( AML_DESCRIPTION_HEADER ) ) ) {
		AML_DEBUG_ERROR(
			State,
			"Error: Table not found in LoadTable (Signature=%s) (OemID=%s) (OemTableID=%s)\n",
			Signature->u.String->Data,
			OemId->u.String->Data,
			OemTableId->u.String->Data
		);
		return AML_TRUE;
	}

	//
	// Allocate space for the entire definition block, as the block must remain loaded for the lifetime of the interpreter.
	// Copy all of the input table data into the persistent allocation for the table.
	// TODO: Code/table object system + allow unloading/releasing later.
	//
	LoadedTable = AmlHeapAllocate( &State->Heap, InputTable->Length );
	if( LoadedTable == NULL ) {
		return AML_FALSE;
	}
	AML_MEMCPY( LoadedTable, InputTable, InputTable->Length );

	//
	// Attempt to actually execute/evaluate the loaded table contents.
	//
	LoadSuccess = AML_FALSE;
	if( InputTable->Length > sizeof( AML_DESCRIPTION_HEADER ) ) {
		CodeSize = ( InputTable->Length - sizeof( AML_DESCRIPTION_HEADER ) );
		CodeData = ( ( const UINT8* )LoadedTable + sizeof( AML_DESCRIPTION_HEADER ) );
		if( ( LoadSuccess = AmlEvalLoadedTableCode( State, CodeData, CodeSize, &RootPathNameString ) ) == AML_FALSE ) {
			AML_DEBUG_ERROR(
				State,
				"Error: Failed to load table in LoadTable (Signature=%s) (OemID=%s) (OemTableID=%s)\n",
				Signature->u.String->Data,
				OemId->u.String->Data,
				OemTableId->u.String->Data
			);
			AmlHeapFree( &State->Heap, LoadedTable );
		}
	}

	//
	// Print information about the loaded table.
	//
	if( LoadSuccess ) {
		AML_DEBUG_INFO(
			State,
			"LoadTable - %p - RootPath=%s - OemID=%.6s - Length=0x%x\n",
			LoadedTable, RootPath->u.String->Data, InputTable->OemId.Data, InputTable->Length
		);
	}

	//
	// Store ParameterData to optional ParameterPath upon success.
	// If the first character of ParameterPathString is a backslash ('\') or caret ('^') character,
	// then the path of the object is ParameterPathString. Otherwise, it is RootPathString.ParameterPathString.
	// TODO: Clean this up.
	//
	if( ( LoadSuccess != AML_FALSE ) && ( ParameterPath->u.String->Size != 0 ) ) {
		if( AmlPathStringToNameString( TempArena,
									   ( const CHAR* )ParameterPath->u.String->Data,
									   ( ParameterPath->u.String->Size / sizeof( CHAR ) ),
									   &ParameterPathNameString ) )
		{
			if( ParameterPathNameString.Prefix.Length != 0 ) {
				if( AmlNamespacePushScope( &State->Namespace, &RootPathNameString, AML_SCOPE_FLAG_SWITCH ) == AML_FALSE ) {
					return AML_FALSE;
				}
			}
			if( AmlNamespaceSearch( &State->Namespace, NULL, &ParameterPathNameString, 0, &ParameterNsNode ) ) {
				if( ( ParameterNsNode->Object != NULL ) && ( ParameterNsNode->Object->Type == AML_OBJECT_TYPE_NAME ) ) {
					AmlConvObjectStore( State, &State->Heap, ParameterData, &ParameterNsNode->Object->u.Name.Value, AML_CONV_FLAGS_IMPLICIT );
				}
			}
			if( ParameterPathNameString.Prefix.Length != 0 ) {
				if( AmlNamespacePopScope( &State->Namespace ) == AML_FALSE ) {
					return AML_FALSE;
				}
			}
		} else {
			AML_DEBUG_ERROR(
				State,
				"Error: Failed to parse ParameterPath in LoadTable: %s\n",
				ParameterPath->u.String->Data
			);
		}
	}

	//
	// Return result indicating table load success (graceful failure is possible in this path).
	//
	*Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = ( LoadSuccess ? ~0ull : 0 ) };
	return AML_TRUE;
}

//
// DefLoadTable := LoadTableOp TermArg TermArg TermArg TermArg TermArg TermArg
//
_Success_( return )
static
BOOLEAN
AmlEvalLoadTable(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode,
	_Out_   AML_DATA*  Result
	)
{
	AML_DATA           Signature;
	AML_DATA           OemId;
	AML_DATA           OemTableId;
	AML_DATA           RootPath;
	AML_DATA           ParameterPath;
	AML_DATA           ParameterData;
	AML_ARENA_SNAPSHOT TempSnapshot;
	BOOLEAN            Success;

	//
	// SignatureString := TermArg => String
	// OEMIDString := TermArg => String
	// OEMTableIDString := TermArg => String
	// RootPathString := TermArg => String
	// ParameterPathString := TermArg => String
	// ParameterData := TermArg
	// DefLoadTable := LoadTableOp SignatureString OEMIDString OEMTableIDString RootPathString ParameterPathString ParameterData
	//
	if( AmlEvalTermArgToType( State, AML_EVAL_TERM_ARG_FLAG_TEMP, AML_DATA_TYPE_STRING, &Signature ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTermArgToType( State, AML_EVAL_TERM_ARG_FLAG_TEMP, AML_DATA_TYPE_STRING, &OemId ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTermArgToType( State, AML_EVAL_TERM_ARG_FLAG_TEMP, AML_DATA_TYPE_STRING, &OemTableId ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTermArgToType( State, AML_EVAL_TERM_ARG_FLAG_TEMP, AML_DATA_TYPE_STRING, &RootPath ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTermArgToType( State, AML_EVAL_TERM_ARG_FLAG_TEMP, AML_DATA_TYPE_STRING, &ParameterPath ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &ParameterData ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Attempt the actual table load using a temporary arena allocation for pathname conversions.
	// TODO: Add an AML state temp arena instead of using the namespace temp arena.
	//
	TempSnapshot = AmlArenaSnapshot( &State->Namespace.TempArena );
	Success = AmlEvalLoadTableInternal(
		State, &State->Namespace.TempArena, &Signature,
		&OemId, &OemTableId, &RootPath, &ParameterPath,
		&ParameterData, Result
	);
	AmlArenaSnapshotRollback( &State->Namespace.TempArena, &TempSnapshot );

	//
	// Release all evaluated parameters.
	//
	AmlDataFree( &Signature );
	AmlDataFree( &OemId );
	AmlDataFree( &OemTableId );
	AmlDataFree( &RootPath );
	AmlDataFree( &ParameterPath );
	AmlDataFree( &ParameterData );
	return Success;
}

//
// Evaluate an ExpressionOpcode.
// Result data value must be freed by the caller.
//
_Success_( return )
BOOLEAN
AmlEvalExpressionOpcode(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    EvalMethodInvocation,
	_Out_   AML_DATA*  EvalResult
	)
{
	AML_DECODER_INSTRUCTION_OPCODE Next;
	SIZE_T                         CursorOpcodeStart;
	AML_DATA                       Operand1;
	AML_DATA                       Operand2;
	AML_OBJECT*                    Target;
	AML_OBJECT*                    RemainderTarget;
	AML_OBJECT*                    SuperName;
	AML_DATA*                      TargetValue;
	AML_DATA                       Result;
	UINT64                         IntegerMaxValue;
	UINT64                         Comparison;
	SIZE_T                         DataSizeBackup;
	BOOLEAN                        Success;
	UINT16                         TimeoutWord;
	AML_WAIT_STATUS                WaitStatus;

	//
	// Try to match an expression opcode.
	//
	if( AmlDecoderPeekOpcode( State, &Next ) == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: AmlEvalExpressionOpcode PeekOpcode failed!" );
		return AML_FALSE;
	} else if( Next.IsExpressionOpcode == AML_FALSE ) {
		return AML_FALSE;
	}
	CursorOpcodeStart = State->DataCursor;
	AmlDecoderConsumeOpcode( State, NULL );

	//
	// Default to no evaluated result.
	//
	Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_NONE };

	//
	// Default to all empty operands, simplifies cleanup/shared usage.
	//
	Operand1  = ( AML_DATA ){ .Type = AML_DATA_TYPE_NONE };
	Operand2  = ( AML_DATA ){ .Type = AML_DATA_TYPE_NONE };
	Target    = NULL;
	SuperName = NULL;

	//
	// Used by logical comparison instructions, values must be defined the same as the AML spec boolean values (Ones/Zero).
	//
	_Static_assert( AML_COMPARISON_RESULT_AML_TRUE  == ~0ull, "Invalid AML_COMPARISON_RESULT values" );
	_Static_assert( AML_COMPARISON_RESULT_AML_FALSE == 0ull,  "Invalid AML_COMPARISON_RESULT values" );

	//
	// Handle expression opcodes.
	// Done:
	//  DefAdd | DefAnd | DefSubtract | DefDivide | DefMod | DefMultiply
	//  | DefNAnd | DefNOr | DefOr | DefOr | DefShiftLeft | DefShiftRight
	//  | DefXOr | DefNot | DefIncrement | DefDecrement | DefCopyObject
	//  | DefLEqual | DefLNotEqual | DefLGreater | DefLGreaterEqual
	//  | DefLLess | DefLLessEqual | DefLAnd | DefLNot | DefLOr
	//  | SizeOf | DefToBuffer | DefToInteger | DefToString | DefToHexString
	//  | DefToDecimalString | RefOfOp | DefDerefOf | DefIndex | DefObjectType
	//  | DefCondRefOf | FindSetLeftBit | FindSetRightBit | DefTimer
	//  | DefAcquire | DefWait | DefMid | DefFromBCD | DefToBCD | DefConcat
	//  | DefConcatRes | DefMatch | DefBuffer | DefVarPackage | DefPackage
	//  | DefLoad | DefLoadTable | DefBuffer | DefPackage | DefVarPackage
	// 
	// Remaining but handled elsewhere (?):
	//   MethodInvocation (!)
	//
	switch( Next.OpcodeID ) {
	case AML_OPCODE_ID_BUFFER_OP:
	case AML_OPCODE_ID_VAR_PACKAGE_OP:
	case AML_OPCODE_ID_PACKAGE_OP:
		//
		// Handle DefBuffer/DefVarPackage/DefPackage, should typically be handled elsewhere,
		// but is included in the definition of an expression, so this is added here nonetheless.
		// The data cursor must be rewound to the start of the instruction opcode, as EvalDataObject
		// expects to consume the opcode itself.
		//
		State->DataCursor = CursorOpcodeStart;
		if( AmlEvalDataObject( State, &Result, AML_TRUE ) == AML_FALSE ) {
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_LOAD_TABLE_OP:
		//
		// DefLoadTable := LoadTableOp TermArg TermArg TermArg TermArg TermArg TermArg
		//
		if( AmlEvalLoadTable( State, AML_FALSE, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_LOAD_OP:
		//
		// DefLoad := LoadOp NameString Target
		//
		if( AmlEvalLoad( State, AML_FALSE, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_MATCH_OP:
		//
		// DefMatch := MatchOp SearchPkg MatchOpcode Operand MatchOpcode Operand StartIndex
		//
		if( AmlEvalMatch( State, AML_FALSE, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_CONCAT_RES_OP:
		//
		// DefConcatRes := ConcatResOp BufData BufData Target
		//
		if( AmlEvalConcatRes( State, AML_FALSE, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_CONCAT_OP:
		//
		// DefConcat := ConcatOp Data Data Target
		//
		if( AmlEvalConcat( State, AML_FALSE, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_TO_BCD:
		//
		// Operand := TermArg => Integer
		// DefToBCD := ToBCDOp Operand Target
		//
		if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = AmlDecimalToBcd( Operand1.u.Integer ),
		};
		break;
	case AML_OPCODE_ID_FROM_BCD_OP:
		//
		// BCDValue := TermArg => Integer
		// DefFromBCD := FromBCDOp BCDValue Target
		//
		if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = AmlBcdToDecimal( Operand1.u.Integer ),
		};
		break;
	case AML_OPCODE_ID_MID_OP:
		//
		// DefMid := MidOp MidObj TermArg TermArg Target
		//
		if( AmlEvalMid( State, AML_FALSE, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_WAIT_OP:
		// 
		// EventObject := SuperName
		// Operand := TermArg => Integer
		// DefWait := WaitOp EventObject Operand
		//
		if( AmlEvalSuperName( State, 0, 0, &SuperName ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( SuperName->Type != AML_OBJECT_TYPE_EVENT ) {
			AmlObjectRelease( SuperName );
			return AML_FALSE;
		}

		//
		// Attempt to await signalling of the given event.
		//
		WaitStatus = AmlHostEventAwait( SuperName->u.Event.Host, SuperName->u.Event.HostHandle, Operand1.u.Integer );
		if( WaitStatus == AML_WAIT_STATUS_ERROR ) {
			AmlObjectRelease( SuperName );
			return AML_FALSE;
		}

		//
		// This operation returns a non-zero value if a timeout occurred and a signal was not acquired.
		//
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( ( WaitStatus == AML_WAIT_STATUS_TIMEOUT ) ? 1 : 0 ),
		};

		break;
	case AML_OPCODE_ID_ACQUIRE_OP:
		//
		// MutexObject := SuperName
		// Timeout := WordData
		// DefAcquire := AcquireOp MutexObject Timeout
		//
		if( AmlEvalSuperName( State, 0, 0, &SuperName ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlDecoderConsumeWord( State, &TimeoutWord ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( SuperName->Type != AML_OBJECT_TYPE_MUTEX ) {
			AmlObjectRelease( SuperName );
			return AML_FALSE;
		}

		//
		// Attempt to acquire the given mutex.
		//
		WaitStatus = AmlMutexAcquire( State, SuperName, TimeoutWord );

		//
		// This operation returns True if a timeout occurred and the mutex ownership was not acquired.
		//
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( ( WaitStatus == AML_WAIT_STATUS_TIMEOUT ) ? ~0ul : 0 ),
		};

		break;
	case AML_OPCODE_ID_TIMER_OP:
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = AmlHostMonotonicTimer( State->Host )
		};
		break;
	case AML_OPCODE_ID_OBJECT_TYPE_OP:   
		//
		// DefObjectType := ObjectTypeOp <SimpleName | DebugObj | DefRefOf | DefDerefOf | DefIndex>
		//
		if( AmlEvalObjectType( State, AML_FALSE, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_FIND_SET_LEFT_BIT_OP:
		//
		// Source operand is evaluated as an Integer.
		// The one-based bit location of the first MSb (most significant set bit) is optionally stored into the Target.
		// DefFindSetLeftBit := FindSetLeftBitOp Operand Target
		//
		if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// The result of 0 means no bit was set,
		// 1 means the left-most bit set is the first bit,
		// 2 means the left-most bit set is the second bit, and so on.
		//
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = AmlFindSetLeftBit( Operand1.u.Integer, ( State->IsIntegerSize64 == 0 ) )
		};

		break;
	case AML_OPCODE_ID_FIND_SET_RIGHT_BIT_OP:
		//
		// Source operand is evaluated as an Integer.
		// The one-based bit location of the first LSb (least significant set bit) is optionally stored into the Target.
		// DefFindSetRightBit := FindSetRightBitOp Operand Target
		//
		if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// The result of 0 means no bit was set,
		// 1 means the left-most bit set is the first bit,
		// 2 means the left-most bit set is the second bit, and so on.
		//
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = AmlFindSetRightBit( Operand1.u.Integer, ( State->IsIntegerSize64 == 0 ) )
		};

		break;
	case AML_OPCODE_ID_REF_OF_OP:
		//
		// DefRefOf := RefOfOp SuperName
		// Object can be any object type (for example, a package, a device object, and so on).
		// TODO: Replace this case with a call to AmlEvalRefOf!
		//
		if( AmlEvalSuperName( State, 0, 0, &SuperName ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Set up new reference to the given target.
		// Returns an object reference to Object. If the Object does not exist,
		// the result of a RefOf operation is fatal. Use the CondRefOf term in cases where the Object might not exist.
		//
		AmlObjectReference( SuperName );
		Result = ( AML_DATA ){
			.Type        = AML_DATA_TYPE_REFERENCE,
			.u.Reference = { .Object = SuperName }
		};

		break;
	case AML_OPCODE_ID_COND_REF_OF_OP:
		//
		// DefCondRefOf := CondRefOfOp SuperName Target
		// Attempts to create a reference to the Source object.
		// The Source of this operation can be any object type (for example, data package, device object, and so on),
		// and the result data is optionally stored into the result target.
		//
		if( AmlEvalSuperName( State, 0, AML_NAME_FLAG_ALLOW_NON_EXISTENT, &SuperName ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// If the referenced object doesn't exist, a NilObject/None type will be returned.
		//
		Success = ( SuperName->Type != AML_OBJECT_TYPE_NONE );

		//
		// On success, the Destination object is set to refer to Source and the execution result of this operation is the value True.
		// On failure, Destination is unchanged and the execution result of this operation is the value False.
		// This can be used to reference items in the namespace that may appear dynamically,
		// for example, from a dynamically loaded definition block.
		//
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( Success ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE )
		};

		//
		// On failure, the destination target is left unchanged.
		//
		if( Success == AML_FALSE ) {
			AmlObjectRelease( Target );
			Target = NULL;
		}

		break;
	case AML_OPCODE_ID_DEREF_OF_OP:
		//
		// Evaluate and handle a DerefOf expression.
		// Must currently be a name/value object, the expression handling function only operates on data/TermArgs, not objects.
		//
		if( AmlEvalDerefOf( State, AML_FALSE, &SuperName ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( SuperName->Type != AML_OBJECT_TYPE_NAME ) {
			return AML_FALSE;
		}

		//
		// Duplicate the resolved value and return it as the result.
		//
		if( AmlDataDuplicate( &SuperName->u.Name.Value, &State->Heap, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}

		break;
	case AML_OPCODE_ID_INDEX_OP:
		//
		// Evaluate and handle a DefIndex expression.
		// Must currently be a name/value object, the expression handling function only operates on data/TermArgs, not objects.
		//
		if( AmlEvalIndex( State, AML_FALSE, &SuperName ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( SuperName->Type != AML_OBJECT_TYPE_NAME ) {
			return AML_FALSE;
		}

		//
		// Duplicate the resolved value and return it as the result.
		//
		if( AmlDataDuplicate( &SuperName->u.Name.Value, &State->Heap, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}

		break;
	case AML_OPCODE_ID_SIZE_OF_OP:
		//
		// DefSizeOf := SizeOfOp SuperName
		// Object referenced by the SuperName must be a buffer, string or package object,
		// or a reference to any of the aformentioned types.
		//
		if( AmlEvalSuperName( State, 0, 0, &SuperName ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Must be a name/value object.
		//
		if( SuperName->Type != AML_OBJECT_TYPE_NAME ) {
			return AML_FALSE;
		}

		//
		// For a buffer, it returns the size in bytes of the data
		// For a string, it returns the size in bytes of the string, not counting the trailing NULL.
		// For a package, it returns the number of elements.
		// For an object reference, the size of the referenced object is returned.
		// Other data types cause a fatal run-time error.
		//
		TargetValue = &SuperName->u.Name.Value;
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = AmlDataSizeOf( TargetValue )
		};

		break;
	case AML_OPCODE_ID_TO_BUFFER_OP:
		//
		// DefToBuffer := ToBufferOp Operand Target
		// Operand must be an Integer, String, or Buffer data type.
		// Data is converted to buffer type and the result is optionally stored into Target. 
		//
		if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// If Data is an integer, it is converted into n bytes of buffer (where n is 4 or 8 depending on revision),
		// taking the least significant byte of integer as the first byte of buffer.
		// If Data is a buffer, no conversion is performed.
		// If Data is a string, each ASCII string character is copied to one buffer byte, including the string null terminator.
		// A null (zero-length) string will be converted to a zero-length buffer.
		// Follows all of the same conversion semantics as our existing implicit converison code (AmlConv) for buffers.
		//
		if( AmlDataDuplicate( &Operand1, &State->Heap, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlConvObjectInPlace( State, &State->Heap, &Result, AML_DATA_TYPE_BUFFER, AML_CONV_FLAGS_EXPLICIT ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Explicit conversion operations do not apply implicit conversion to stores to the target result,
		// instead, they operate like the CopyObject instruction, overriding the existing object.
		//
		if( Target->Type != AML_OBJECT_TYPE_NONE ) {
			if( AmlOperandStore( State, &State->Heap, &Result, Target, AML_FALSE ) == AML_FALSE ) {
				AmlDataFree( &Result );
				return AML_FALSE;
			}
		}

		//
		// Disable automatic implicit conversion target/result store, as we have manually written to the target.
		//
		AmlObjectRelease( Target );
		Target = NULL;
		break;
	case AML_OPCODE_ID_TO_INTEGER_OP:
		//
		// DefToInteger := ToIntegerOp Operand Target
		// Data must be an Integer, String, or Buffer data type.
		// Data is converted to integer type and the result is optionally stored into the target.
		//
		if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// If Data is a string, it must be either a decimal or hexadecimal numeric string
		// (in other words, prefixed by "0x") and the value must not exceed the maximum of an integer value.
		// If Data is a Buffer, the first 8 bytes of the buffer are converted to an integer,
		// taking the first byte as the least significant byte of the integer.
		// If Data is an integer, no action is performed.
		//
		if( AmlDataDuplicate( &Operand1, &State->Heap, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlConvObjectInPlace( State, &State->Heap, &Result, AML_DATA_TYPE_INTEGER, AML_CONV_FLAGS_EXPLICIT ) == AML_FALSE ) {
			AmlDataFree( &Result );
			return AML_FALSE;
		}

		//
		// Explicit conversion operations do not apply implicit conversion to stores to the target result,
		// instead, they operate like the CopyObject operation, overriding the existing object.
		//
		if( Target->Type != AML_OBJECT_TYPE_NONE ) {
			if( AmlOperandStore( State, &State->Heap, &Result, Target, AML_FALSE ) == AML_FALSE ) {
				AmlDataFree( &Result );
				return AML_FALSE;
			}
		}

		//
		// Disable automatic implicit conversion target/result store, as we have manually written to the target.
		//
		AmlObjectRelease( Target );
		Target = NULL;
		break;
	case AML_OPCODE_ID_TO_STRING_OP:
		//
		// DefToString := ToStringOp TermArg LengthArg Target
		// Source is evaluated as a buffer. Length is evaluated as an integer data type.
		//
		if( AmlEvalTermArgToType( State, AML_EVAL_TERM_ARG_FLAG_TEMP, AML_DATA_TYPE_BUFFER, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTermArgToType( State, AML_EVAL_TERM_ARG_FLAG_TEMP, AML_DATA_TYPE_INTEGER, &Operand2 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Starting with the first byte, the contents of the buffer are copied into the string until the number of characters
		// specified by Length is reached or a null (0) character is found. 
		// If Length is not specified or is Ones, then the contents of the buffer are copied until a null (0) character is found. 
		// If the source buffer has a length of zero, a zero length (null terminator only) string will be created. The result is copied into the Target.
		//
		DataSizeBackup = Operand1.u.Buffer->Size;
		Operand1.u.Buffer->Size = AML_MIN( Operand1.u.Buffer->Size, Operand2.u.Integer );
		if( ( Success = AmlDataDuplicate( &Operand1, &State->Heap, &Result ) ) != AML_FALSE ) {
			Success = AmlConvObjectInPlace( State, &State->Heap, &Result, AML_DATA_TYPE_STRING, AML_CONV_FLAGS_EXPLICIT_TO_STRING );
		}
		Operand1.u.Buffer->Size = DataSizeBackup;

		//
		// Handle failure to convert object.
		//
		if( Success == AML_FALSE ) {
			AmlDataFree( &Result );
			return AML_FALSE;
		}

		//
		// Explicit conversion operations do not apply implicit conversion to stores to the target result,
		// instead, they operate like the CopyObject operation, overriding the existing object.
		//
		if( Target->Type != AML_OBJECT_TYPE_NONE ) {
			if( AmlOperandStore( State, &State->Heap, &Result, Target, AML_FALSE ) == AML_FALSE ) {
				AmlDataFree( &Result );
				return AML_FALSE;
			}
		}

		//
		// Disable automatic implicit conversion target/result store, as we have manually written to the target.
		//
		AmlObjectRelease( Target );
		Target = NULL;
		break;
	case AML_OPCODE_ID_TO_HEX_STRING_OP:
		//
		// DefToHexString := ToHexStringOp Operand Target
		// Data must be an Integer, String, or Buffer data type.
		//
		if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		
		//
		// Data is converted to a hexadecimal string, and the result is optionally stored into Result.
		// If Data is already a string, no action is performed.
		// If Data is a buffer, it is converted to a string of hexadecimal values separated by commas.
		// A zero-length buffer will be converted to a null (zero-length) string.
		//
		if( AmlDataDuplicate( &Operand1, &State->Heap, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlConvObjectInPlace( State, &State->Heap, &Result, AML_DATA_TYPE_STRING, AML_CONV_FLAGS_EXPLICIT_TO_HEX_STRING ) == AML_FALSE ) {
			AmlDataFree( &Result );
			return AML_FALSE;
		}

		//
		// Explicit conversion operations do not apply implicit conversion to stores to the target result,
		// instead, they operate like the CopyObject operation, overriding the existing object.
		//
		if( Target->Type != AML_OBJECT_TYPE_NONE ) {
			if( AmlOperandStore( State, &State->Heap, &Result, Target, AML_FALSE ) == AML_FALSE ) {
				AmlDataFree( &Result );
				return AML_FALSE;
			}
		}

		//
		// Disable automatic implicit conversion target/result store, as we have manually written to the target.
		//
		AmlObjectRelease( Target );
		Target = NULL;
		break;
	case AML_OPCODE_ID_TO_DECIMAL_STRING_OP:
		//
		// DefToDecimalString := ToDecimalStringOp Operand Target
		// Data must be an Integer, String, or Buffer data type.
		//
		if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// If operand is an integer, it is converted to a string of decimal ASCII characters.
		// If Data is already a string, no action is performed.
		// If Data is a buffer, it is converted to a string of decimal values separated by commas. 
		// (Each byte of the buffer is converted to a single decimal value.) 
		//
		if( AmlDataDuplicate( &Operand1, &State->Heap, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlConvObjectInPlace( State,
										 &State->Heap,
										 &Result,
										 AML_DATA_TYPE_STRING,
										 AML_CONV_FLAGS_EXPLICIT_TO_DECIMAL_STRING ) == AML_FALSE )
		{
			AmlDataFree( &Result );
			return AML_FALSE;
		}

		//
		// Explicit conversion operations do not apply implicit conversion to stores to the target result,
		// instead, they operate like the CopyObject operation, overriding the existing object.
		//
		if( Target->Type != AML_OBJECT_TYPE_NONE ) {
			if( AmlOperandStore( State, &State->Heap, &Result, Target, AML_FALSE ) == AML_FALSE ) {
				AmlDataFree( &Result );
				return AML_FALSE;
			}
		}

		//
		// Disable automatic implicit conversion target/result store, as we have manually written to the target.
		//
		AmlObjectRelease( Target );
		Target = NULL;
		break;
	case AML_OPCODE_ID_STORE_OP:
		//
		// DefStore := StoreOp TermArg SuperName
		//
		if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalSuperName( State, 0, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Set up the result to be stored to the target using implicit conversion.
		//
		if( AmlDataDuplicate( &Operand1, &State->Heap, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}

		break;
	case AML_OPCODE_ID_COPY_OBJECT_OP:
		//
		// DefCopyObject := CopyObjectOp TermArg SimpleName
		//
		if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalSimpleName( State, 0, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Explicitly store a copy of the operand object to the target name.
		// No implicit type conversion is performed. This operator is used to avoid
		// the implicit conversion inherent in the ASL Store operator.
		//
		if( AmlOperandStore( State, &State->Heap, &Operand1, Target, AML_FALSE ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Return the copied object value.
		//
		if( AmlDataDuplicate( &Operand1, &State->Heap, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Disable automatic implicit conversion target/result store, as we have manually written to the target.
		//
		AmlObjectRelease( Target );
		Target = NULL;
		break;
	case AML_OPCODE_ID_LAND_OP:
		//
		// DefLAnd := LandOp Operand Operand
		// If both values are non-zero, True is returned: otherwise, False is returned.
		//
		if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Operand2 ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( ( Operand1.u.Integer && Operand2.u.Integer )
						   ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE )
		};
		break;
	case AML_OPCODE_ID_LOR_OP:
		//
		// DefLOr := LorOp Operand Operand
		// If either value is non-zero, True is returned; otherwise, False is returned.
		//
		if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Operand2 ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( ( Operand1.u.Integer || Operand2.u.Integer )
						   ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE )
		};
		break;
	case AML_OPCODE_ID_LNOT_OP:
		//
		// DefLNot := LnotOp Operand
		// If the value is zero True is returned; otherwise, False is returned.
		//
		if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( ( Operand1.u.Integer == 0 ) ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE )
		};
		break;
	case AML_OPCODE_ID_LEQUAL_OP:
		//
		// DefLEqual := LequalOp Operand Operand
		// Note: AML spec says "Operand" (which is a TermArg => Integer),
		// but the ASL spec says it can be an integer, buffer, or string,
		// so for now, allow buffers and strings, with the comparison
		// semantics defined by the ASL spec.
		//
		if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand2 ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Comparison = AmlCompareData( State, &State->Heap, AML_COMPARISON_TYPE_EQUAL, &Operand1, &Operand2 );
		Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = Comparison };
		if( Comparison == AML_COMPARISON_RESULT_ERROR ) {
			AML_DEBUG_ERROR( State, "Error: Invalid logical comparison types." );
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_LNOT_EQUAL_OP:
		//
		// DefLEqual := LequalOp Operand Operand
		// See LEQUAL_OP case notes.
		//
		if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand2 ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Comparison = AmlCompareData( State, &State->Heap, AML_COMPARISON_TYPE_EQUAL, &Operand1, &Operand2 );
		Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = ~Comparison };
		if( Comparison == AML_COMPARISON_RESULT_ERROR ) {
			AML_DEBUG_ERROR( State, "Error: Invalid logical comparison types." );
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_LGREATER_OP:
		//
		// DefLGreater := LgreaterOp Operand Operand
		// See LEQUAL_OP case notes.
		//
		if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand2 ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Comparison = AmlCompareData( State, &State->Heap, AML_COMPARISON_TYPE_GREATER, &Operand1, &Operand2 );
		Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = Comparison };
		if( Comparison == AML_COMPARISON_RESULT_ERROR ) {
			AML_DEBUG_ERROR( State, "Error: Invalid logical comparison types." );
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_LLESS_OP:
		//
		// DefLLess := LlessOp Operand Operand
		// See LEQUAL_OP case notes.
		//
		if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand2 ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Comparison = AmlCompareData( State, &State->Heap, AML_COMPARISON_TYPE_LESS, &Operand1, &Operand2 );
		Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = Comparison };
		if( Comparison == AML_COMPARISON_RESULT_ERROR ) {
			AML_DEBUG_ERROR( State, "Error: Invalid logical comparison types." );
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_LGREATER_EQUAL_OP:
		//
		// DefLGreaterEqual := LgreaterEqualOp Operand Operand
		// See LEQUAL_OP case notes.
		//
		if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand2 ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Comparison = AmlCompareData( State, &State->Heap, AML_COMPARISON_TYPE_GREATER_EQUAL, &Operand1, &Operand2 );
		Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = Comparison };
		if( Comparison == AML_COMPARISON_RESULT_ERROR ) {
			AML_DEBUG_ERROR( State, "Error: Invalid logical comparison types." );
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_LLESS_EQUAL_OP:
		//
		// DefLLessEqual := LlessEqualOp Operand Operand
		// See LEQUAL_OP case notes.
		//
		if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Operand2 ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Comparison = AmlCompareData( State, &State->Heap, AML_COMPARISON_TYPE_LESS_EQUAL, &Operand1, &Operand2 );
		Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = Comparison };
		if( Comparison == AML_COMPARISON_RESULT_ERROR ) {
			AML_DEBUG_ERROR( State, "Error: Invalid logical comparison types." );
			return AML_FALSE;
		}
		break;
	case AML_OPCODE_ID_DIVIDE_OP:
		//
		// Remainder := Target
		// Quotient := Target
		// DefDivide := DivideOp Dividend Divisor Remainder Quotient
		//
		if( AmlEvalTermArgToType( State, AML_EVAL_TERM_ARG_FLAG_TEMP, AML_DATA_TYPE_INTEGER, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTermArgToType( State, AML_EVAL_TERM_ARG_FLAG_TEMP, AML_DATA_TYPE_INTEGER, &Operand2 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &RemainderTarget ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Divide-by-zero exceptions are fatal.
		//
		if( Operand2.u.Integer == 0 ) {
			AML_DEBUG_ERROR( State, "Error: Division by zero.\n" );
			return AML_FALSE;
		}

		//
		// Calculate quotient as main result, and write it out to the target.
		//
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( Operand1.u.Integer / Operand2.u.Integer )
		};

		//
		// Optionally allow a remainder to be calculated and outputted.
		//
		if( RemainderTarget->Type != AML_OBJECT_TYPE_NONE ) {
			if( AmlOperandStore( State,
								 &State->Heap,
								 &( AML_DATA ){
								 	.Type      = AML_DATA_TYPE_INTEGER,
								 	.u.Integer = ( Operand1.u.Integer % Operand2.u.Integer ),
								 },
								 RemainderTarget,
								 AML_TRUE ) == AML_FALSE )
			{
				return AML_FALSE;
			}
		}

		AmlDebugPrintArithmetic( State, AML_DEBUG_LEVEL_TRACE, "/", Target, Operand1, Operand2, Result );
		AmlObjectRelease( RemainderTarget );
		RemainderTarget = NULL;
		break;
	case AML_OPCODE_ID_ADD_OP:
		if( AmlEvalBinaryIntegerArithmeticArguments( State, &Operand1, &Operand2, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( Operand1.u.Integer + Operand2.u.Integer )
		};
		AmlDebugPrintArithmetic( State, AML_DEBUG_LEVEL_TRACE, "+", Target, Operand1, Operand2, Result );
		break;
	case AML_OPCODE_ID_SUBTRACT_OP:
		if( AmlEvalBinaryIntegerArithmeticArguments( State, &Operand1, &Operand2, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( Operand1.u.Integer - Operand2.u.Integer )
		};
		AmlDebugPrintArithmetic( State, AML_DEBUG_LEVEL_TRACE, "-", Target, Operand1, Operand2, Result );
		break;
	case AML_OPCODE_ID_MULTIPLY_OP:
		if( AmlEvalBinaryIntegerArithmeticArguments( State, &Operand1, &Operand2, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( Operand1.u.Integer * Operand2.u.Integer )
		};
		AmlDebugPrintArithmetic( State, AML_DEBUG_LEVEL_TRACE, "*", Target, Operand1, Operand2, Result );
		break;
	case AML_OPCODE_ID_MOD_OP:
		if( AmlEvalBinaryIntegerArithmeticArguments( State, &Operand1, &Operand2, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( Operand2.u.Integer == 0 ) {
			AML_DEBUG_ERROR( State, "Error: Mod divisor is zero.\n" );
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( Operand1.u.Integer % Operand2.u.Integer )
		};
		AmlDebugPrintArithmetic( State, AML_DEBUG_LEVEL_TRACE, "%", Target, Operand1, Operand2, Result );
		break;
	case AML_OPCODE_ID_AND_OP:
		if( AmlEvalBinaryIntegerArithmeticArguments( State, &Operand1, &Operand2, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( Operand1.u.Integer & Operand2.u.Integer )
		};
		AmlDebugPrintArithmetic( State, AML_DEBUG_LEVEL_TRACE, "&", Target, Operand1, Operand2, Result );
		break;
	case AML_OPCODE_ID_NAND_OP:
		if( AmlEvalBinaryIntegerArithmeticArguments( State, &Operand1, &Operand2, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ~( Operand1.u.Integer & Operand2.u.Integer )
		};
		AmlDebugPrintArithmetic( State, AML_DEBUG_LEVEL_TRACE, "~&", Target, Operand1, Operand2, Result );
		break;
	case AML_OPCODE_ID_OR_OP:
		if( AmlEvalBinaryIntegerArithmeticArguments( State, &Operand1, &Operand2, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( Operand1.u.Integer | Operand2.u.Integer )
		};
		AmlDebugPrintArithmetic( State, AML_DEBUG_LEVEL_TRACE, "|", Target, Operand1, Operand2, Result );
		break;
	case AML_OPCODE_ID_NOR_OP:
		if( AmlEvalBinaryIntegerArithmeticArguments( State, &Operand1, &Operand2, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ~( Operand1.u.Integer | Operand2.u.Integer )
		};
		AmlDebugPrintArithmetic( State, AML_DEBUG_LEVEL_TRACE, "~|", Target, Operand1, Operand2, Result );
		break;
	case AML_OPCODE_ID_SHIFT_LEFT_OP:
		if( AmlEvalBinaryIntegerArithmeticArguments( State, &Operand1, &Operand2, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( Operand1.u.Integer << AML_MIN( Operand2.u.Integer, 63 ) )
		};
		AmlDebugPrintArithmetic( State, AML_DEBUG_LEVEL_TRACE, "<<", Target, Operand1, Operand2, Result );
		break;
	case AML_OPCODE_ID_SHIFT_RIGHT_OP:
		if( AmlEvalBinaryIntegerArithmeticArguments( State, &Operand1, &Operand2, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( Operand1.u.Integer >> AML_MIN( Operand2.u.Integer, 63 ) )
		};
		AmlDebugPrintArithmetic( State, AML_DEBUG_LEVEL_TRACE, ">>", Target, Operand1, Operand2, Result );
		break;
	case AML_OPCODE_ID_XOR_OP:
		if( AmlEvalBinaryIntegerArithmeticArguments( State, &Operand1, &Operand2, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ( Operand1.u.Integer ^ Operand2.u.Integer )
		};
		AmlDebugPrintArithmetic( State, AML_DEBUG_LEVEL_TRACE, "^", Target, Operand1, Operand2, Result );
		break;
	case AML_OPCODE_ID_NOT_OP:
		//
		// DefNot := NotOp Operand Target
		//
		if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Operand1 ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}
		Result = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_INTEGER,
			.u.Integer = ~Operand1.u.Integer
		};
		break;
	case AML_OPCODE_ID_INCREMENT_OP:
		//
		// DefIncrement := IncrementOp SuperName
		// Add one to the Addend and place the result back in Addend.
		// Equivalent to Add (Addend, 1, Addend).
		// Attempts to convert the current value of the target to an integer.
		//
		if( AmlEvalSuperName( State, 0, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// The addend object must be a name/data object
		//
		if( Target->Type != AML_OBJECT_TYPE_NAME ) {
			return AML_FALSE;
		}
		TargetValue = &Target->u.Name.Value;

		//
		// Convert the current value of the addend to an integer (and output to result var).
		//
		Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER };
		if( AmlConvObjectStore( State, &State->Heap, TargetValue, &Result, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Overflow conditions are ignored and the result of an overflow is zero.
		// TODO: This code might be weird with the automatic integer sign extension!
		//
		IntegerMaxValue = ( State->IsIntegerSize64 ? UINT64_MAX : UINT32_MAX );
		if( ( Result.u.Integer & IntegerMaxValue ) != IntegerMaxValue ) {
			Result.u.Integer = ( ( Result.u.Integer & IntegerMaxValue ) + 1 );
		} else {
			Result.u.Integer = 0;
		}

		break;
	case AML_OPCODE_ID_DECREMENT_OP:
		//
		// DefDecrement := DecrementOp SuperName
		// This operation decrements the Minuend by one and the result is stored back to Minuend.
		// Equivalent to Subtract(Minuend, 1, Minuend).
		// Attempts to convert the current value of the target to an integer.
		//
		Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER };
		if( AmlEvalSuperName( State, 0, 0, &Target ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// The addend object must be a name/data object
		//
		if( Target->Type != AML_OBJECT_TYPE_NAME ) {
			return AML_FALSE;
		}
		TargetValue = &Target->u.Name.Value;

		//
		// Convert the current value of the addend to an integer (and output to result var).
		//
		Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER };
		if( AmlConvObjectStore( State, &State->Heap, &Target->u.Name.Value, &Result, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Underflow conditions are ignored and the result is Ones.
		//
		IntegerMaxValue = ( State->IsIntegerSize64 ? UINT64_MAX : UINT32_MAX );
		if( Result.u.Integer != 0 ) {
			Result.u.Integer = ( ( Result.u.Integer & IntegerMaxValue ) - 1 );
		} else {
			Result.u.Integer = ( State->IsIntegerSize64 ? UINT64_MAX : UINT32_MAX );
		}

		break;
	default:
		AML_DEBUG_ERROR( State, "Error: Invalid expression opcode!" );
		return AML_FALSE;
	}

	//
	// Apply sign extension to maintain a correct internal value for differing spec integer sizes.
	// TODO: This might not work out how I was expecting!
	//
	if( Result.Type == AML_DATA_TYPE_INTEGER ) {
		Result.u.Integer = AmlDecoderSignExtendInteger( State, Result.u.Integer );
	}

	//
	// If a target variable was specified, the evaluated result must also be written to the target variable.
	// Output (target) parameters for all operators except the explicit data conversion operators are subject
	// to implicit data type conversion (also known as implicit result object conversion) whenever the target
	// is an existing named object or named field that is of a different type than the object to be stored.
	//
	if( Target != NULL ) {
		Success = AmlOperandStore( State, &State->Heap, &Result, Target, AML_TRUE );
		if( Success ) {
			AmlDebugPrintObjectName( State, AML_DEBUG_LEVEL_TRACE, Target );
			AML_DEBUG_TRACE( State, " = " );
			AmlDebugPrintDataValue( State, AML_DEBUG_LEVEL_TRACE, &Result );
			AML_DEBUG_TRACE( State, "\n" );
		} else {
			AML_DEBUG_ERROR( State, "Error: AmlOperandStore failed!\n" );
		}
		AmlObjectRelease( Target );
		Target = NULL;
	} else {
		Success = AML_TRUE;
	}

	//
	// Free any temporary operand data values.
	//
	AmlDataFree( &Operand1 );
	AmlDataFree( &Operand2 );

	//
	// Release evaluated SuperNames.
	//
	if( SuperName != NULL ) {
		AmlObjectRelease( SuperName );
		SuperName = NULL;
	}

	//
	// Free result data value upon late failure (due to target store).
	//
	if( Success == AML_FALSE ) {
		AmlDataFree( &Result );
		return AML_FALSE;
	}

	//
	// Write out final evaluated result.
	//
	*EvalResult = Result;
	return AML_TRUE;
}
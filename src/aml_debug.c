#include "aml_state.h"
#include "aml_namespace.h"
#include "aml_host.h"
#include "aml_debug.h"
#include "aml_object.h"

//
// Debug print to host at a certain log level.
//
VOID
AmlDebugPrint(
	_In_   const struct _AML_STATE* State,
	_In_   INT                      LogLevel,
	_In_z_ const CHAR*              Format,
	...
	)
{
	va_list VaList;

	//
	// Pass along the actual print and varargs to the host implementation.
	//
	if( LogLevel >= AML_BUILD_DEBUG_LEVEL ) {
		va_start( VaList, Format );
		AmlHostDebugPrintV( State->Host, LogLevel, Format, VaList );
		va_end( VaList );
	}
}

//
// Debug print the value of an AML_DATA.
//
VOID
AmlDebugPrintDataValue(
	_In_ const struct _AML_STATE* State,
	_In_ INT                      LogLevel,
	_In_ const AML_DATA*          Value
	)
{
	SIZE_T i;

	switch( Value->Type ) {
	case AML_DATA_TYPE_NONE:
		AmlDebugPrint( State, LogLevel, "[None]" );
		break;
	case AML_DATA_TYPE_INTEGER:
		AmlDebugPrint( State, LogLevel, "Integer(0x%"PRIx64")", Value->u.Integer );
		break;
	case AML_DATA_TYPE_STRING:
		AmlDebugPrint( State, LogLevel, "String(\"%s\")", Value->u.String->Data );
		break;
	case AML_DATA_TYPE_BUFFER:
		AmlDebugPrint( State, LogLevel, "Buffer(" );
		for( i = 0; i < Value->u.Buffer->Size; i++ ) {
			AmlDebugPrint( State, LogLevel, "%02x", Value->u.Buffer->Data[ i ] );
			if( i < ( Value->u.Buffer->Size - 1 ) ) {
				AmlDebugPrint( State, LogLevel, ", " );
			}
		}
		AmlDebugPrint( State, LogLevel, ")" );
		break;
	case AML_DATA_TYPE_PACKAGE:
		AmlDebugPrint( State, LogLevel, "[Package]" );
		break;
	case AML_DATA_TYPE_VAR_PACKAGE:
		AmlDebugPrint( State, LogLevel, "[VarPackage]" );
		break;
	case AML_DATA_TYPE_REFERENCE:
		if( Value->u.Reference.Object->NamespaceNode != NULL ) {
			AmlDebugPrint( State, LogLevel, "RefOf(" );
			AmlDebugPrintObjectName( State, LogLevel, Value->u.Reference.Object );
			AmlDebugPrint( State, LogLevel, ")" );
		} else {
			AmlDebugPrint( State, LogLevel, "[Reference]" );
		}
		break;
	default:
		AmlDebugPrint( State, LogLevel, "[Unknown]" );
		break;
	}
}

//
// Debug print an AML name string.
//
VOID
AmlDebugPrintNameString(
	_In_ const struct _AML_STATE*       State,
	_In_ INT                            LogLevel,
	_In_ const struct _AML_NAME_STRING* NameString
	)
{
	SIZE_T i;

	//
	// Print all prefixes.
	//
	for( i = 0; i < NameString->Prefix.Length; i++ ) {
		AmlDebugPrint( State, LogLevel, "%c", NameString->Prefix.Data[ i ] );
	}

	//
	// Print all name segments.
	//
	for( i = 0; i < NameString->SegmentCount; i++ ) {
		if( i != 0 ) {
			AmlDebugPrint( State, LogLevel, "." );
		}
		AmlDebugPrint( State, LogLevel, "%.4s", NameString->Segments[ i ].Data );
	}
}

//
// Debug print an AML object's name (if any known).
//
VOID
AmlDebugPrintObjectName(
	_In_ const struct _AML_STATE*  State,
	_In_ INT                       LogLevel,
	_In_ const struct _AML_OBJECT* Object
	)
{
	AML_NAME_STRING* NameString;
	SIZE_T           i;

	switch( Object->SuperType ) {
	case AML_OBJECT_SUPERTYPE_ARG:
		AmlDebugPrint( State, LogLevel, "Arg%i", ( INT )Object->SuperIndex );
		break;
	case AML_OBJECT_SUPERTYPE_LOCAL:
		AmlDebugPrint( State, LogLevel, "Local%i", ( INT )Object->SuperIndex );
		break;
	case AML_OBJECT_SUPERTYPE_DEBUG:
		AmlDebugPrint( State, LogLevel, "Debug" );
		break;
	default:
		if( Object->NamespaceNode != NULL ) {
			NameString = &Object->NamespaceNode->AbsolutePath;
			for( i = 0; i < NameString->Prefix.Length; i++ ) {
				AmlDebugPrint( State, LogLevel, "%c", NameString->Prefix.Data[ i ] );
			}
			for( i = 0; i < NameString->SegmentCount; i++ ) {
				if( i != 0 ) {
					AmlDebugPrint( State, LogLevel, "." );
				}
				AmlDebugPrint( State, LogLevel, "%.4s", NameString->Segments[ i ].Data );
			}
		} else {
			AmlDebugPrint( State, LogLevel, "Unnamed" );
		}
		break;
	}
}

//
// Debug print a binary arithmetic instruction.
//
VOID
AmlDebugPrintArithmetic(
	_In_ const struct _AML_STATE*  State,
	_In_ INT                       LogLevel,
	_In_ const CHAR*               OperatorSymbol,
	_In_ const struct _AML_OBJECT* Target,
	_In_ AML_DATA                  Operand1,
	_In_ AML_DATA                  Operand2,
	_In_ AML_DATA                  Result
	)
{
	AmlDebugPrintObjectName( State, LogLevel, Target );
	AmlDebugPrint( State, LogLevel, " = ((" );
	AmlDebugPrintDataValue( State, LogLevel, &Operand1 );
	AmlDebugPrint( State, LogLevel, " %s ", OperatorSymbol );
	AmlDebugPrintDataValue( State, LogLevel, &Operand2 );
	AmlDebugPrint( State, LogLevel, ") = " );
	AmlDebugPrintDataValue( State, LogLevel, &Result );
	AmlDebugPrint( State, LogLevel, ")\n" );
}
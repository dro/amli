#include "aml_eval.h"
#include "aml_eval_namespace.h"
#include "aml_debug.h"

//
// Attempt to evaluate a name instruction.
//
_Success_( return )
BOOLEAN
AmlEvalName(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode
	)
{
	AML_NAMESPACE_NODE*            Node;
	AML_OBJECT*                    Object;
	SIZE_T                         i;

	//
	// Consume the opcode if the caller hasn't done it for us.
	//
	if( ConsumeOpcode ) {
		if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_NAME_OP, NULL ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	//
	// Attempt to create a new reference counted object.
	// Don't worry about releasing our reference/memoryupon failure here,
	// as failure is fatal to the entire state, and all memory is backed by
	// an underlying internal arena + heap.
	//
	if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_NAME, &Object ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// DefName := NameOp NameString DataRefObject
	// TODO: The AML spec says DataRefObject, but the ASL spec says DataObject, and iasl wont let me compile a RefOf here.
	//
	if( AmlDecoderConsumeNameString( State, &Object->u.Name.String ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlEvalDataObject( State, &Object->u.Name.Value, AML_FALSE ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Attempt to create and link a new namespace node for this object.
	//
	if( AmlStateSnapshotCreateNode( State, State->Namespace.ScopeLast, &Object->u.Name.String, &Node ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Print debug information.
	//
	if( Object->u.Name.String.SegmentCount > 0 ) {
		AML_DEBUG_TRACE( State, "Created named object: \"" );
		AML_DEBUG_TRACE( State, "%.*s", ( UINT )Node->AbsolutePath.Prefix.Length, Node->AbsolutePath.Prefix.Data );
		for( i = 0; i < Node->AbsolutePath.SegmentCount; i++ ) {
			if( i != 0 ) {
				AML_DEBUG_TRACE( State, "." );
			}
			AML_DEBUG_TRACE( State, "%.4s", Node->AbsolutePath.Segments[ i ].Data );
		}
		AML_DEBUG_TRACE( State, "\", Value: " );
		AmlDebugPrintDataValue( State, AML_DEBUG_LEVEL_TRACE, &Object->u.Name.Value );
		AML_DEBUG_TRACE( State, "\n" );
	}

	//
	// Save parsed object value to node, holds on to our initial reference given to us by AmlObjectCreate.
	//
	Object->NamespaceNode = Node;
	Node->Object = Object;
	return AML_TRUE;
}

//
// Attempt to evaluate a scope instruction.
//
_Success_( return )
BOOLEAN
AmlEvalScope(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode
	)
{
	AML_NAMESPACE_NODE* Node;
	AML_NAME_STRING     ScopeLocation;
	SIZE_T              ScopePkgLength;
	SIZE_T              ScopePkgStart;
	SIZE_T              ScopePkgCodeOffset;
	SIZE_T              ScopeCodeStart;
	SIZE_T              ScopeCodeSize;
	AML_NAME_STRING*    ScopePath;

	//
	// Consume the opcode if the caller hasn't already.
	//
	if( ConsumeOpcode ) {
		if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_SCOPE_OP, NULL ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}
	
	//
	// Opens and assigns a base namespace scope to a collection of objects.
	// All object names defined within the scope are created relative to Location.
	// Note that Location does not have to be below the surrounding scope, but can refer to any location within the namespace.
	// The Scope term itself does not create objects, but only locates objects within the namespace; the actual objects are created by other ASL terms.
	// DefScope := ScopeOp PkgLength NameString TermList
	//
	ScopePkgStart = State->DataCursor;
	if( AmlDecoderConsumePackageLength( State, &ScopePkgLength ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlDecoderConsumeNameString( State, &ScopeLocation ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Print debug information about the scope name.
	//
	AML_DEBUG_TRACE( State, "Scope(" );
	AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_TRACE, &ScopeLocation );
	AML_DEBUG_TRACE( State, ")\n" );

	//
	// The scope location refers to an *existing* object with an associated scope.
	//
	if( AmlNamespaceSearch( &State->Namespace, State->Namespace.ScopeLast, &ScopeLocation, 0, &Node ) == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: Failed to find and enter scope!\n" );
		return AML_FALSE;
	}

	//
	// The object referred to by Location must already exist in the namespace, and be
	// one of the following object types that has a namespace scope associated with it:
	//  - A predefined scope such as: (root), _SB, GPE, _PR, _TZ, etc.
	//  - Device
	//  - Processor
	//  - Thermal Zone
	//  - Power Resource
	//
	if( Node->Object != NULL ) {
		switch( Node->Object->Type ) {
		case AML_OBJECT_TYPE_SCOPE:
		case AML_OBJECT_TYPE_DEVICE:
		case AML_OBJECT_TYPE_PROCESSOR:
		case AML_OBJECT_TYPE_THERMAL_ZONE:
		case AML_OBJECT_TYPE_POWER_RESOURCE:
			break;
		default:
			return AML_FALSE;
		}	
	}

	//
	// Determine the path to use when changing to the scope.
	// Generally we always want to use an absolute path here, so that we don't
	// erroneously push a scope local to the current scope when we shouldn't.
	//
	ScopePath = &Node->AbsolutePath;

	//
	// Push and enter a new namespace scope level with the given name.
	//
	if( AmlNamespacePushScope( &State->Namespace, ScopePath, AML_SCOPE_FLAG_SWITCH ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Get offset and length of the start of the TermList code embedded inside of the package.
	//
	ScopeCodeStart = State->DataCursor;
	ScopePkgCodeOffset = ( ScopeCodeStart - ScopePkgStart );
	if( ScopePkgCodeOffset > ScopePkgLength ) {
		return AML_FALSE;
	}
	ScopeCodeSize = ( ScopePkgLength - ScopePkgCodeOffset );

	//
	// Recursively evaluate all code of the scope.
	//
	if( AmlEvalTermListCode( State, ScopeCodeStart, ScopeCodeSize, AML_FALSE ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Pop and exit the namespace scope level.
	//
	if( AmlNamespacePopScope( &State->Namespace ) == AML_FALSE ) {
		return AML_FALSE;
	}

	return AML_TRUE;
}

//
// Attempt to evaluate an alias instruction.
//
_Success_( return )
BOOLEAN
AmlEvalAlias(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode
	)
{
	AML_NAME_STRING     DestinationName;
	AML_NAME_STRING     AliasName;
	AML_NAMESPACE_NODE* Node;
	AML_OBJECT*         Object;

	//
	// Consume the opcode if the caller hasn't done it for us.
	//
	if( ConsumeOpcode ) {
		if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_ALIAS_OP, NULL ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	//
	// DefAlias := AliasOp NameString NameString
	//
	if( AmlDecoderConsumeNameString( State, &DestinationName ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( AmlDecoderConsumeNameString( State, &AliasName ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// If we are in full evaluation mode, first check if the namespace pre-pass has already processed this alias.
	//
	if( State->PassType == AML_PASS_TYPE_FULL ) {
		if( AmlNamespaceSearch( &State->Namespace,
								NULL,
								&AliasName,
								AML_SEARCH_FLAG_NAME_CREATION,
								&Node ) )
		{
			if( Node->IsPreParsed && Node->Object && Node->Object->Type == AML_OBJECT_TYPE_ALIAS ) {
				if( AmlNamespaceCompareNameString( &State->Namespace,
												   NULL,
												   &DestinationName,
												   &Node->Object->u.Alias.DestinationName ) )
				{
					AML_DEBUG_TRACE( State, "Skipped alias creation due to pre-pass.\n" );
					return AML_TRUE;
				}
			}
		}
	}

	//
	// Attempt to create and link a new namespace node for this object.
	//
	if( AmlStateSnapshotCreateNode( State, NULL, &AliasName, &Node ) == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: AML name collision, alias \"" );
		AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_ERROR, &AliasName );
		AML_DEBUG_ERROR( State, "\" already exists!\n" );
		return AML_FALSE;
	}

	//
	// Attempt to create a new reference counted object.
	// See note in AML_OPCODE_ID_NAME_OP case about not releasing upon failure.
	//
	if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_ALIAS, &Object ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Initialize the alias object.
	// TODO: Copy name strings, right now the input bytecode for the object needs to stay loaded forever.
	//
	Object->u.Alias = ( AML_OBJECT_ALIAS ){
		.Name            = AliasName,
		.DestinationName = DestinationName,
	};

	//
	// Save parsed object value to node.
	//
	Object->NamespaceNode = Node;
	Node->Object = Object;
	Node->IsPreParsed = ( State->PassType == AML_PASS_TYPE_NAMESPACE );
	return AML_TRUE;
}

//
// Attempt to evaluate a namespace modifier object opcode.
// NameSpaceModifierObj := DefAlias | DefName | DefScope
//
_Success_( return )
BOOLEAN
AmlEvalNamespaceModifierObjectOpcode(
	_Inout_ AML_STATE* State
	)
{
	AML_DECODER_INSTRUCTION_OPCODE Next;

	//
	// Consume input instruction opcode, this function should only be called if the
	// caller has already deduced that the opcode is a namespace modifier object opcode.
	//
	if( AmlDecoderConsumeOpcode( State, &Next ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Handle namespace modifier object opcodes.
	//
	switch( Next.OpcodeID ) {
	case AML_OPCODE_ID_NAME_OP:
		//
		// Evaluate a name instruction.
		// DefName := NameOp NameString DataRefObject
		//
		return AmlEvalName( State, AML_FALSE );
	case AML_OPCODE_ID_ALIAS_OP:
		//
		// Evaluate an alias instruction.
		// DefAlias := AliasOp NameString NameString
		//
		return AmlEvalAlias( State, AML_FALSE );
	case AML_OPCODE_ID_SCOPE_OP:
		//
		// Evaluate a scope instruction.
		// DefScope := ScopeOp PkgLength NameString TermList
		//
		return AmlEvalScope( State, AML_FALSE );
	}

	AML_DEBUG_ERROR( State, "Unsupported namespace modifier opcode!" );
	return AML_FALSE;
}
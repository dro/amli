#include "aml_platform.h"
#include "aml_state.h"
#include "aml_data.h"
#include "aml_heap.h"
#include "aml_decoder.h"
#include "aml_debug.h"
#include "aml_conv.h"
#include "aml_compare.h"
#include "aml_object.h"
#include "aml_namespace.h"
#include "aml_host.h"
#include "aml_osi.h"
#include "aml_device_id.h"
#include "aml_method.h"
#include "aml_base.h"
#include "aml_eval.h"
#include "aml_eval_named.h"
#include "aml_eval_expression.h"
#include "aml_eval_statement.h"
#include "aml_eval_namespace.h"
#include "aml_eval_reference.h"

//
// TODO:
// Feature/Functionality:
//  [DONE] Proper package definition evaluation support.
//  [DONE] Full package support throughout rest of codebase/interactions.
//  [DONE] Figure out index system (buffer fields referencing strings?!)
//  [DONE] Fix/investigate object reference leaks (especially in index handler/target operand parsing!!)
//  [DONE] (Multi-pass) Support out-of-order definitions, real code references objects before they are created.
//  [DONE] Host interface.
//  [DONE] Host interface OpRegion field unit interactions.
//  [DONE] Fix method scope system, need to change namespace scope to that of the method when calling into it!
//  [DONE] Fix scope traversal system to use the path segments of the parent scope instead of incorrectly using the stack!
//  [DONE] Field writes.
//  [DONE] Properly test field reads that cross word boundaries.
//  [DONE] Temporary NS scope for methods.
//  [DONE] PowerResource
//  [DONE] Support or skip deprecated ProcessorOp in eval code.
//  [DONE] ThermalZone
//  [DONE] Index field access. 
//  [DONE] Test bank field access.
//  [DONE] Finish all remaining statement instructions.
//  [DONE] Externals (just ignore?)
//  [DONE] Test event/mutex instructions.
//  [DONE] Finish all remaining expression instructions except Load/LoadTable.
//  [DONE] Add cases for remaining expression instructions that are implemented elsewhere.
//  [DONE] Stubbed out ConnectField support, is skipped, need support for GPIO/SB I/O.
//  [DONE] Host interface synchronization objects.
//  [DONE] Synchronization instructions.
//  [DONE] Allow OSPM/interpreter to call into AML methods with native API.
//  [DONE] Allow OSPM/interpreter to define native methods.
//  [DONE] Predefined objects (\_OS, \_OSI, \_REV).
//  [DONE] Initial _OSI implementation.
//  [DONE] Compare full names in namespace code instead of only hashes.
//  [DONE] Build actual hierarchical tree of the namespace.
//  [DONE] Load expression support.
//  [DONE] LoadTable expression support.
//  [DONE] Add host physical-virtual memory map/unmap interface.
//  [DONE] Add code to map memory/MMIO operation regions, switch all uses of AmlHostMmio* to use mapped addresses.
//  [DONE] Evaluating PCI device SBDF address/finding parent root bus.
//  [DONE] PCI interaction for operation regions
//  [DONE] Support operation region device accesses over PCI-to-PCI bridges.
//  [DONE] Move PCI functions to own files.
//  [DONE] Add remaining required host interface OpRegion types.
//  [DONE] Package conversion support (package to package store), this is not valid (?).
//  [DONE] Predefined \_GL object.
//  [DONE] Special _GL mutex access semantics.
//  [DONE] ACPI global lock interaction.
//  [DONE] Follow proper locking semantics for opregions/fields.
//  [DONE] Fix package stores, they are somewhat allowed.
//  [DONE] Ownership of a Mutex must be relinquished before completion of any invocation!
//  [DONE] Remove default method scope creation in AmlDecoderCreate.
//  [DONE] Remove old sentinel MethodScopeRoot and change to actual per-execution-context first scope pointer.
//  [DONE] Fix or remove AML_DATA_TYPE_NONE store source case!
//  [DONE] Fix partial buffer write system for field units bigger than 64 bits.
//  [DONE] Support underscore padding in ASL object path decoding.
//  [DONE] Support for host-implemented OpRegion types.
//  [DONE] Support BufferAcc field access.
//  [DONE] Fix Buffer stores to BufferAcc fields using wrong FieldBitLength semantics for splitting up reads/writes!
//  [DONE] Test BufferAcc field access.
//  [DONE] Debug output support for all types.
//  [DONE] Fix String stores to BufferAcc fields?
//  [DONE] Support for _REG broadcast for registration of certain OpRegion types.
//  [DONE] Integrate and test _REG broadcast.
//  [DONE] Fix unallocated/NULL string/buffer problems in conv stores (maybe introduce a nil sentinel in the future).
//  [DONE] Proper _STA/_INI broadcast helper.
//  [DONE] Test device initialization/_INI broadcast.
//  [DONE] (?) Need to support field ACCESS_ATTRIBUTE_BYTES/pass along optional field to OperationRegionRead/Write?
//  [DONE] Field connection support.
//  [DONE] Fix connection under-referencing and leak in field parsing!
//  [DONE] _INI re-broadcast for tables that are loaded after the main initial pass.
//  [DONE] (kind of, links to existing tree parents) Namespace tree building for tables that are loaded after the main initial pass.
//  [DONE] _REG re-broadcast for tables that are loaded after the main initial pass.
//  [DONE] Better memory management for conditional error handling (full state snapshots?).
//  [DONE] Remove the temporary malloc/free usage in internal memory management when not in ASAN debug build!
//  Better error reporting/debug info system.
//  SyncLevel support.
//  Ensure that SyncLevel works properly with the automatic mutex release system.
//  Gracefully back out of the current scope/if statement scope if we hit an unknown method call in the namespace pass.
//  Ensure that a referenced object has been fully evaluated when in the full pass (now needed due to conditional exploration in ns path).
//  Track mutex acquire count inside mutex object, and only call real AmlMutexAcquire/Release accordingly.
// 
// Validation/bug checking/uncertainty:
//   Fix/investigate method invocation case in AmlEvalSuperName!
//   Fix/investigate deep-copy vs reference system for string/buffer in AmlDataDuplicate!
//   Look into PendingInterruptionEvent behavior with mismatched keywords (break, return, etc, look into note in DefWhile handler).
//   While loop body in namespace pass?
//   Full namespace tree rebuild for tables that are loaded after the main initial pass?
// 
// Codebase:
//  [DONE] Rename AmlDecoderEval* To AmlEval*
//  [DONE] Split up aml_eval into multiple files.
//  [DONE] Rename AmlDecoderOperand* to AmlOperand*
//  [DONE] Make all eval functions that aren't exposed static.
//  [DONE] Rename any AmlDecoder* to Aml* that aren't relevant to the decoder.
//  [DONE] Move AmlMethod* functions into own file.
//  [DONE] Tidy up aml_data.
//  [DONE] Remove leftover ifdeffed decoder functions.
//  [DONE] Remove leftover unused opaque data decoder system (decided to leave core for disassembler/ns pass).
//  [DONE] Remove AmlMethodInvoke, unify to one AmlMethodInvoke, remove requirement for NS node being passed.
//  [DONE] Move mutex and global lock handling to separate functions/file.
//  [DONE] Use proper method code for initialization of root method scope in AmlDecoderCreate.
//  [DONE] Rename AML_DECODER_STATE to AML_STATE
//  [DONE] Move constructor from aml_decoder to aml_state.
//  [DONE] Move generic state-related functions from aml_eval to aml_state or other files.
//  [DONE] Integrate/remove external dependency on stringconv helper files.
//  [DONE] Move arena into aml code, optionally allow override of implementaiton.
//  [DONE] Add allocator interface to aml code.
//  [DONE] Move hashing implementation to aml code.
//  [DONE] (?) Change MSVC-specific defines (MAX*, _Interlocked, etc)
//  [DONE] Type definitions for all of our MSVC-specific types.
//  Add TempArena to main state, change all uses of namespace TempArena to it.
//  Split off operand handling to own files?
//  Split out functions with many cases in aml_eval_* into sub-functions.
//  Move PCI eval functions to aml_pci? 
//  Refactor reference counted data/object system?
//  Remove pointless name strings that point into bytecode.
//

//
// Resolve the given AML_SIMPLE_NAME to an operand meta structure.
// The returned object reference counter is increased, and must be released by the caller.
//
_Success_( return )
BOOLEAN
AmlResolveSimpleName(
	_Inout_  AML_STATE*             State,
	_In_opt_ AML_METHOD_SCOPE*      MethodScope,
	_In_     const AML_SIMPLE_NAME* Target,
	_In_     UINT                   SearchFlags,
	_In_     UINT                   NameFlags,
	_Outptr_ AML_OBJECT**           ppObject
	)
{
	AML_NAMESPACE_NODE* NamespaceNode;
	AML_OBJECT*         Object;

	//
	// If no method scope is given, use the current method.
	//
	MethodScope = ( ( MethodScope != NULL ) ? MethodScope : State->MethodScopeLast );

	//
	// Handle all simple name target types.
	//
	switch( Target->Type ) {
	case AML_SIMPLE_NAME_TYPE_ARG_OBJ:
		//
		// Ensure that parsed argument index is valid (should always be, unless code is changed).
		//
		if( Target->u.ArgObj >= AML_COUNTOF( MethodScope->ArgObjects ) ) {
			AML_DEBUG_FATAL( State, "Fatal: ArgObj index out of bounds!\n" );
			return AML_FALSE;
		}

		//
		// Must be within the scope of a method call.
		// Acpica acpiexec seems to just allow accesses as if there was a function scope level.
		//
		if( MethodScope == State->MethodScopeRoot ) {
			AML_DEBUG_WARNING( State, "Warning: ArgObj operand used outside of method scope.\n" );
		}

		Object = MethodScope->ArgObjects[ Target->u.ArgObj ];
		break;
	case AML_SIMPLE_NAME_TYPE_LOCAL_OBJ:
		//
		// Ensure that the parsed local variable index is valid (should always be, unless code is changed).
		//
		if( ( Target->u.LocalObj >= AML_COUNTOF( MethodScope->LocalObjects ) ) ) {
			AML_DEBUG_FATAL( State, "Fatal: LocalObj index out of bounds!\n" );
			return AML_FALSE;
		}

		//
		// Must be within the scope of a method call.
		// Acpica acpiexec seems to just allow accesses as if there was a function scope level.
		//
		if( MethodScope == State->MethodScopeRoot ) {
			AML_DEBUG_WARNING( State, "Warning: LocalObj operand used outside of method scope.\n" );
		}
		
		Object = MethodScope->LocalObjects[ Target->u.ArgObj ];
		break;
	case AML_SIMPLE_NAME_TYPE_STRING:
		//
		// Search the given name (and resolve/follow aliases to final object).
		// Return special nil object if we are allowed to not find anything.
		//
		if( AmlNamespaceSearch( &State->Namespace, NULL, &Target->u.NameString, SearchFlags, &NamespaceNode ) == AML_FALSE ) {
			if( ( NameFlags & AML_NAME_FLAG_ALLOW_NON_EXISTENT ) == 0 ) {
				return AML_FALSE;
			}
			Object = &State->Namespace.NilObject;
		} else {
			Object = NamespaceNode->Object;
		}
		break;
	default:
		return AML_FALSE;
	}

	//
	// Raise the reference counter of the object, must be appropriately released by the caller.
	//
	AmlObjectReference( Object );

	//
	// Follow object references.
	// Note: not sure this is going to be used as planned anymore with new operand system, leaving it here temporarily for now.
	//
	// if( ( Value->Type == AML_DATA_TYPE_REFERENCE )
	// 	&& ( SearchFlags & AML_SEARCH_FLAG_FOLLOW_REFERENCE ) )
	// {
	// 	AML_DEBUG_PANIC( "TODO: AmlDecoderLookupSimpleNameOperandValue reference following" );
	// }

	*ppObject = Object;
	return AML_TRUE;
}

//
// Decode and resolve a SimpleName from the input stream.
//
_Success_( return )
BOOLEAN
AmlEvalSimpleName(
	_Inout_  AML_STATE*   State,
	_In_     UINT         SearchFlags,
	_In_     UINT         NameFlags,
	_Outptr_ AML_OBJECT** ppObject
	)
{
	AML_SIMPLE_NAME SimpleName;

	//
	// Consume a full SimpleName from the input code.
	//
	if( AmlDecoderConsumeSimpleName( State, &SimpleName ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Attempt to resolve the SimpleName to an object.
	//
	return AmlResolveSimpleName( State, NULL, &SimpleName, SearchFlags, NameFlags, ppObject );
}

//
// Store to a target simple name operand, with implicit conversion rules applied.
// TODO: Convert to use new operand resolution system.
//
_Success_( return )
BOOLEAN
AmlOperandStore(
	_Inout_ AML_STATE*      State,
	_Inout_ AML_HEAP*       Heap,
	_In_    const AML_DATA* Value,
	_Inout_ AML_OBJECT*     Target,
	_In_    BOOLEAN         ImplicitConversion
	)
{
	AML_DATA* TargetData;
	AML_DATA  FieldData;

	//
	// Treat stores to nothing as a no-op.
	//
	if( Target == &State->Namespace.NilObject ) {
		return AML_TRUE;
	}

	//
	// Stores have different rules depending on the kind of target operand.
	// TODO: Remove switch and fold all into one path, can now be done with new operand system.
	//
	switch( Target->SuperType ) {
	case AML_OBJECT_SUPERTYPE_ARG:
		//
		// Must be within the scope of a method call.
		// Acpica acpiexec seems to just allow accesses, as if there was a function scope level.
		//
		if( State->MethodScopeLast == State->MethodScopeRoot ) {
			AML_DEBUG_WARNING( State, "Warning: ArgObj operand used outside of method scope.\n" );
		}

		//
		// Internal local objects are always treated as named objects.
		// TODO: Check that the we are still within the scope of this object.
		//
		if( Target->Type != AML_OBJECT_TYPE_NAME ) {
			return AML_FALSE;
		}

		//
		// Store to ArgX parameters:
		// ObjectReference objects - Automatic dereference, copy the object and overwrite the final target.
		// All other object types - Copy the object and overwrite the ArgX variable.
		// Direct writes to buffer or package ArgX parameters will also simply overwrite ArgX.
		//
		TargetData = &Target->u.Name.Value;
		if( TargetData->Type == AML_DATA_TYPE_REFERENCE ) {
			//
			// Can only store to a reference to a name/value object.
			//
			if( TargetData->u.Reference.Object->Type != AML_OBJECT_TYPE_NAME ) {
				AML_DEBUG_ERROR( State, "Error: Store to an ArgX reference to a non-name/value object.\n" );
				return AML_FALSE;
			}
			TargetData = &TargetData->u.Reference.Object->u.Name.Value;
		}

		//
		// Free any previous value of the ArgX object, then store new value, this way, no implicit conversion is applied.
		// TODO: Why are these not just using AmlDataDuplicate, stop using the conv system.
		// Note: This has now become required to move the value outside of the current snapshot.
		//
		AmlDataFree( TargetData );
		return AmlConvObjectStore( State, Heap, Value, TargetData, AML_CONV_FLAGS_IMPLICIT );
	case AML_OBJECT_SUPERTYPE_LOCAL:
		//
		// Must be within the scope of a method call.
		// Acpica acpiexec seems to just allow accesses, as if there was a function scope level.
		//
		if( State->MethodScopeLast == State->MethodScopeRoot ) {
			AML_DEBUG_WARNING( State, "Warning: LocalObj operand used outside of method scope.\n" );
		}

		//
		// Internal local objects are always treated as named objects.
		// TODO: Check that the we are still within the scope of this object.
		//
		if( Target->Type != AML_OBJECT_TYPE_NAME ) {
			return AML_FALSE;
		}

		//
		// Store to LocalX variables:
		// All object types - Delete any existing object in LocalX first, then store a copy of the object.
		// Even if LocalX contains an Object Reference, it is overwritten.
		//
		TargetData = &Target->u.Name.Value;

		//
		// Free any previous value of the LocalX object, then store new value, this way, no implicit conversion is applied.
		//
		AmlDataFree( TargetData );
		return AmlConvObjectStore( State, Heap, Value, TargetData, AML_CONV_FLAGS_IMPLICIT );
	case AML_OBJECT_SUPERTYPE_DEBUG:
		AmlConvObjectStore( State, Heap, Value, &( AML_DATA ){ .Type = AML_DATA_TYPE_DEBUG }, AML_CONV_FLAGS_IMPLICIT );
		return AML_TRUE;
	case AML_OBJECT_SUPERTYPE_NONE:
		//
		// The store destination must be a named object or a field unit.
		// TODO: Figure out a way to remove these checks entirely,
		// AmlConvObjectStore should decide what it wants to support!
		// (Store to object function inside conversion implementation?)
		//
		if( Target->Type == AML_OBJECT_TYPE_NAME ) {
			//
			// Output to the value of the named object.
			// If implicit conversion isn't desired, free the original value and make type NONE,
			// skips implicit conversion, causes AmlConvObjectStore to operate like CopyObject.
			//
			if( ImplicitConversion == AML_FALSE ) {
				AmlDataFree( &Target->u.Name.Value );
			}
			return AmlConvObjectStore( State, Heap, Value, &Target->u.Name.Value, AML_CONV_FLAGS_IMPLICIT );
		} else if( AmlObjectIsFieldUnit( Target ) ) {
			//
			// Create a temporary datum referencing the field unit and attempt to store to it.
			//
			FieldData = ( AML_DATA ){ .Type = AML_DATA_TYPE_FIELD_UNIT, .u.FieldUnit = Target };
			return AmlConvObjectStore( State, Heap, Value, &FieldData, AML_CONV_FLAGS_IMPLICIT );
		}
	}

	AML_DEBUG_ERROR( State, "Error: Unsupported destination object type in AmlOperandStore!" );
	return AML_FALSE;
}

//
// Evaluate and resolve the given AML_SUPER_NAME to an object.
// The returned object is referenced and must be released by the caller.
//
_Success_( return )
BOOLEAN
AmlEvalSuperName(
	_Inout_  AML_STATE*   State,
	_In_     UINT         SearchFlags,
	_In_     UINT         NameFlags,
	_Outptr_ AML_OBJECT** ppObject
	)
{
	AML_DECODER_INSTRUCTION_OPCODE Peek;
	AML_SIMPLE_NAME                SimpleName;

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
	// Either way, the precedence here is still kind of unclear...
	//
	if( Peek.OpcodeID == AML_OPCODE_ID_DEBUG_OP ) {
		if( AmlDecoderConsumeOpcode( State, NULL ) == AML_FALSE ) {
			return AML_FALSE;
		}
		AmlObjectReference( &State->DebugObject );
		*ppObject = &State->DebugObject;
		return AML_TRUE;
	} 
	
	//
	// Try to match a SimpleName.
	// Hack to turn NullChar case of simple name into empty operand.
	// TODO: Fix up how AmlDecoderConsumeSimpleName works in regards to NullChar names.
	// TODO: This will currently match over MethodInvocations, and will lead to ResolveSimpleName failing,
	// or being incorrect! Must fix soon!
	//
	if( AmlDecoderConsumeSimpleName( State, &SimpleName ) ) {
		if( ( SimpleName.Type == AML_SIMPLE_NAME_TYPE_STRING )
			&& ( SimpleName.u.NameString.Prefix.Length == 0 )
			&& ( SimpleName.u.NameString.SegmentCount == 0 ) )
		{
			if( ( NameFlags & AML_NAME_FLAG_ALLOW_NULL_NAME ) == 0 ) {
				return AML_FALSE;
			}
			AmlObjectReference( &State->Namespace.NilObject ); /* TODO: Move nil object to eval/decoder state? */
			*ppObject = &State->Namespace.NilObject;
			return AML_TRUE;
		}
		return AmlResolveSimpleName( State, NULL, &SimpleName, SearchFlags, NameFlags, ppObject );
	}

	//
	// Finally, try to match a ReferenceTypeOpcode, which may include a user MethodInvocation, which is greedy.
	//
	return AmlEvalReferenceTypeOpcode( State, SearchFlags, ppObject );
}

//
// Evaluate and resolve the given SuperName or NullName to an object.
// The returned object is referenced and must be released by the caller.
// Target := SuperName | NullName
//
_Success_( return )
BOOLEAN
AmlEvalTarget(
	_Inout_  AML_STATE*   State,
	_In_     UINT         SearchFlags,
	_Outptr_ AML_OBJECT** ppObject
	)
{
	return AmlEvalSuperName( State, SearchFlags, AML_NAME_FLAG_ALLOW_NULL_NAME, ppObject );
}

//
// Evaluate a single package element.
// PackageElement := DataRefObject | NameString
//
_Success_( return )
static
BOOLEAN
AmlEvalPackageElement(
	_Inout_ AML_STATE* State,
	_Out_   AML_DATA*  Data
	)
{   
	AML_NAME_STRING     NameString;
	AML_NAMESPACE_NODE* NsNode;
	AML_OBJECT_NAME*    NameObject;
	AML_DATA            FieldUnit;

	//
    // First try to match and handle an object reference by name,
	//
	if( AmlDecoderMatchNameString( State, AML_FALSE, &NameString ) ) {
		//
		// Lookup the named object referenced by this package element.
		//
		if( AmlNamespaceSearch( &State->Namespace, NULL, &NameString, 0, &NsNode ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// These Named References to Data Objects are resolved to actual data by the AML Interpreter at runtime:
		//  - Integer reference
		//  - String reference
		//  - Buffer reference
		//  - Buffer Field reference
		//  - Field Unit reference
		//  - Package reference
		//
		if( NsNode->Object->Type == AML_OBJECT_TYPE_NAME ) {
			NameObject = &NsNode->Object->u.Name;
			switch( NameObject->Value.Type ) {
			case AML_DATA_TYPE_INTEGER:
			case AML_DATA_TYPE_STRING:
			case AML_DATA_TYPE_BUFFER:
			case AML_DATA_TYPE_PACKAGE:
				return AmlDataDuplicate( &NameObject->Value, &State->Heap, Data );
			default:
				AML_DEBUG_ERROR( State, "Error: Unsupported named object reference in PackageElement.\n" );
				return AML_FALSE;
			}
		} else if( AmlObjectIsFieldUnit( NsNode->Object ) ) {
			*Data = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER };
			FieldUnit = ( AML_DATA ){ .Type = AML_DATA_TYPE_FIELD_UNIT, .u.FieldUnit = NsNode->Object };
			return AmlConvObjectStore( State, &State->Heap, &FieldUnit, Data, AML_CONV_FLAGS_IMPLICIT );
		}

		//
		// These Named References to non-Data Objects cannot be resolved to values.
		// They are instead returned in the package as references:
		//  - Device reference
		//  - Event reference
		//  - Method reference
		//  - Mutex reference
		//  - Operation Region reference
		//  - Power Resource reference
		//  - Processor reference
		//  - Thermal Zone reference
		//  - Predefined Scope reference
		//
		switch( NsNode->Object->Type ) {
		case AML_OBJECT_TYPE_DEVICE:
		case AML_OBJECT_TYPE_EVENT:
		case AML_OBJECT_TYPE_METHOD:
		case AML_OBJECT_TYPE_MUTEX:
		case AML_OBJECT_TYPE_OPERATION_REGION:
		case AML_OBJECT_TYPE_POWER_RESOURCE:
		case AML_OBJECT_TYPE_PROCESSOR:
		case AML_OBJECT_TYPE_THERMAL_ZONE:
		case AML_OBJECT_TYPE_SCOPE:
			*Data = ( AML_DATA ){ .Type = AML_DATA_TYPE_REFERENCE, .u.Reference.Object = NsNode->Object };
			AmlObjectReference( NsNode->Object );
			return AML_TRUE;
		default:
			break;
		}

		//
		// Doesn't seem to be any supported named data or object type.
		//
		AML_DEBUG_ERROR( State, "Error: Unsupported object reference in PackageElement.\n" );
		return AML_FALSE;
	}

	//
	// We weren't able to match an object reference by name, must be plain data.
	//
	return AmlEvalDataObject( State, Data, AML_FALSE );
}

//
// Evaluate a full package element list.
//
_Success_( return )
static
BOOLEAN
AmlEvalPackageElementList(
	_Inout_ AML_STATE*        State,
	_Inout_ AML_PACKAGE_DATA* Package,
	_In_    SIZE_T            ListEndOffset
	)
{
	AML_DATA              ElementData;
	AML_PACKAGE_ELEMENT*  Element;
	SIZE_T                ElementCount;
	SIZE_T                i;

	//
	// Process elements until we reach the end of the list element data.
	//
	ElementCount = 0;
	while( State->DataCursor < ListEndOffset ) {
		if( ElementCount >= Package->ElementCount ) {
			return AML_FALSE;
		} else if( AmlEvalPackageElement( State, &ElementData ) == AML_FALSE ) {
			return AML_FALSE;
		} if( ( Element = AmlHeapAllocate( &State->Heap, sizeof( *Element ) ) ) == NULL ) {
			return AML_FALSE;
		}
		*Element = ( AML_PACKAGE_ELEMENT ){
			.ParentHeap = &State->Heap,
			.Value      = ElementData
		};
		Package->Elements[ ElementCount++ ] = Element;
	}

	//
	// If NumElements is present and greater than the number of elements in the PackageList
	// the default entry of type Uninitialized (see ObjectType) is used to initialize the
	// package elements beyond those initialized from the PackageList.
	//
	for( i = 0; i < ( Package->ElementCount - ElementCount ); i++ ) {
		if( ( Element = AmlHeapAllocate( &State->Heap, sizeof( *Element ) ) ) == NULL ) {
			return AML_FALSE;
		}
		*Element = ( AML_PACKAGE_ELEMENT ){
			.ParentHeap = &State->Heap,
			.Value.Type = AML_DATA_TYPE_NONE
		};
		Package->Elements[ ElementCount++ ] = Element;
	}

	return AML_TRUE;
}

//
// Evaluate a DataObject.
// DataObject := ComputationalData | DefPackage | DefVarPackage
// TODO: Remove old AmlDecoderMatchDataObject/opaque system entirely, and evaluate it ourselves instead.
//
_Success_( return )
BOOLEAN
AmlEvalDataObject(
	_Inout_ AML_STATE* State,
	_Out_   AML_DATA*  Data,
	_In_    BOOLEAN    IsTemporary
	)
{
	AML_OPAQUE_DATA       OpaqueData;
	AML_BUFFER_DATA*      StringData;
	AML_DATA              BufferSize;
	AML_DATA              VarNumElements;
	SIZE_T                OldDataCursor;
	SIZE_T                OldDataLength;
	SIZE_T                OpaqueDataOffset;
	SIZE_T                InitDataSize;
	AML_BUFFER_DATA*      BufferResource;
	SIZE_T                i;
	AML_PACKAGE_DATA*     Package;
	AML_PACKAGE_ELEMENT** PackageElementList;

	//
	// Perform initial match of a DataObject using the decoder, constant data is decoded,
	// and variable data is opaque, and must be evaluated after this.
	// This opaque matching function will place us at the end of the instruction.
	//
	if( AmlDecoderMatchDataObject( State, &OpaqueData ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Default to no evaluated result.
	//
	*Data = ( AML_DATA ){ .Type = AML_DATA_TYPE_NONE };

	//
	// Evaluate opaque decoded data.
	//
	switch( OpaqueData.Type ) {
	case AML_DATA_TYPE_INTEGER:
		*Data = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = OpaqueData.u.Integer };
		break;
	case AML_DATA_TYPE_STRING:
		//
		// Check if string bounds are valid, and contained by the input bytecode.
		//
		if( ( AmlDecoderIsValidDataWindow( State, OpaqueData.u.String.Start, OpaqueData.u.String.DataSize ) == AML_FALSE )
			|| ( OpaqueData.u.String.DataSize >= SIZE_MAX ) )
		{
			return AML_FALSE;
		}

		//
		// Allocate memory for string data.
		//
		if( IsTemporary ) {
			if( ( StringData = AmlStateSnapshotCreateBufferData( State,
																 &State->Heap,
																 OpaqueData.u.String.DataSize,
																 ( OpaqueData.u.String.DataSize + 1 ) ) ) == NULL )
			{
				return AML_FALSE;
			}
		} else {
			if( ( StringData = AmlBufferDataCreate( &State->Heap,
													OpaqueData.u.String.DataSize,
													( OpaqueData.u.String.DataSize + 1 ) ) ) == NULL )
			{
				return AML_FALSE;
			}
		}

		//
		// Copy initialization data from the input bytecode to the allocation.
		//
		for( i = 0; i < OpaqueData.u.String.DataSize; i++ ) {
			StringData->Data[ i ] = State->Data[ OpaqueData.u.String.Start + i ];
		}

		//
		// Null terminate string data.
		//
		StringData->Data[ OpaqueData.u.String.DataSize ] = '\0';

		//
		// Initialize and return string object.
		//
		*Data = ( AML_DATA ){ 
			.Type     = AML_DATA_TYPE_STRING,
			.u.String = StringData
		};

		break;
	case AML_DATA_TYPE_BUFFER:
		//
		// Backup old decoder window (at the end of the instruction).
		//
		OldDataCursor = State->DataCursor;
		OldDataLength = State->DataLength;

		//
		// Move the decoder to the start of unevaluated opaque data.
		//
		State->DataCursor = OpaqueData.u.Buffer.SizeArgStart;
		State->DataLength = ( State->DataCursor + OpaqueData.u.Buffer.RawDataSize );

		//
		// BufferSize := TermArg => Integer
		// DefBuffer := BufferOp PkgLength | BufferSize ByteList
		//                                 ^-- Opaque data begins here.
		//
		if( AmlEvalTermArg( State, 0, &BufferSize ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( BufferSize.Type != AML_DATA_TYPE_INTEGER || BufferSize.u.Integer >= SIZE_MAX ) {
			return AML_FALSE;
		}

		//
		// We have consumed and evaluated the opaque BufferSize TermArg, we are now at the ByteList portion of DefBuffer.
		//
		OpaqueDataOffset = ( State->DataCursor - OpaqueData.u.Buffer.SizeArgStart );
		InitDataSize = ( OpaqueData.u.Buffer.RawDataSize - OpaqueDataOffset );

		//
		// Validate data parameters.
		//
		if( OpaqueDataOffset > OpaqueData.u.VarPackage.ByteSize ) {
			return AML_FALSE;
		} else if( AmlDecoderIsValidDataWindow( State, State->DataCursor, InitDataSize ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Allocate a new buffer resource.
		//
		if( IsTemporary ) {
			if( ( BufferResource = AmlStateSnapshotCreateBufferData( State, &State->Heap,
																	 ( SIZE_T )BufferSize.u.Integer, 0 ) ) == NULL )
			{
				return AML_FALSE;
			}
		} else {
			if( ( BufferResource = AmlBufferDataCreate( &State->Heap, ( SIZE_T )BufferSize.u.Integer, 0 ) ) == NULL ) {
				return AML_FALSE;
			}
		}

		//
		// Initialize evaluated buffer information.
		//
		*Data = ( AML_DATA ){
			.Type     = AML_DATA_TYPE_BUFFER,
			.u.Buffer = BufferResource,
		};

		//
		// Copy any initializer data to the buffer.
		//
		InitDataSize = AML_MIN( InitDataSize, Data->u.Buffer->Size );
		for( i = 0; i < InitDataSize; i++ ) {
			Data->u.Buffer->Data[ i ] = State->Data[ State->DataCursor + i ];
		}

		//
		// Uninitialized buffer elements are zero by default.
		//
		for( i = 0; i < ( Data->u.Buffer->Size - InitDataSize ); i++ ) {
			Data->u.Buffer->Data[ InitDataSize + i ] = 0;
		}
		
		//
		// Restore the old window (to the end of the instruction).
		//
		State->DataCursor = OldDataCursor;
		State->DataLength = OldDataLength;
		break;
	case AML_DATA_TYPE_PACKAGE:
		//
		// Backup old decoder window (at the end of the instruction).
		//
		OldDataCursor = State->DataCursor;
		OldDataLength = State->DataLength;

		//
		// Validate package element count.
		//
		if( OpaqueData.u.Package.NumElements >= ( SIZE_MAX / sizeof( *PackageElementList ) ) ) {
			return AML_FALSE;
		}

		//
		// Move the decoder to the start of unevaluated PackageElementList.
		//
		State->DataCursor = OpaqueData.u.Package.ElementListStart;
		State->DataLength = ( State->DataCursor + OpaqueData.u.Package.ElementListByteSize );

		//
		// Allocate a new internal package data object.
		//
		if( ( Package = AmlHeapAllocate( &State->Heap, sizeof( *Package ) ) ) == NULL ) {
			return AML_FALSE;
		}

		//
		// Allocate element array for the package.
		//
		PackageElementList = AmlHeapAllocate( &State->Heap, ( sizeof( *PackageElementList ) * OpaqueData.u.Package.NumElements ) );
		if( PackageElementList == NULL ) {
			AmlHeapFree( &State->Heap, Package );
			return AML_FALSE;
		}

		//
		// Zero initialize all package elements for safe releasing.
		//
		for( i = 0; i < OpaqueData.u.Package.NumElements; i++ ) {
			PackageElementList[ i ] = NULL;
		}

		//
		// Initialize the reference package object.
		//
		*Package = ( AML_PACKAGE_DATA ){
			.ReferenceCount   = 1,
			.ParentHeap       = &State->Heap,
			.ElementArrayHeap = &State->Heap,
			.ElementCount     = OpaqueData.u.Package.NumElements,
			.Elements         = PackageElementList
		};

		//
		// Evaluate and initialize all package elements.
		//
		if( AmlEvalPackageElementList( State,
									   Package,
									   ( State->DataCursor + OpaqueData.u.Package.ElementListByteSize ) ) == AML_FALSE )
		{
			AmlHeapFree( &State->Heap, Package );
			return AML_FALSE;
		}

		//
		// Return created package.
		//
		*Data = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_PACKAGE,
			.u.Package = Package,
		};

		//
		// Restore the old window (to the end of the instruction).
		//
		State->DataCursor = OldDataCursor;
		State->DataLength = OldDataLength;
		break;
	case AML_DATA_TYPE_VAR_PACKAGE:
		//
		// Backup old decoder window (at the end of the instruction).
		//
		OldDataCursor = State->DataCursor;
		OldDataLength = State->DataLength;

		//
		// Move the decoder to the start of unevaluated opaque data.
		//
		State->DataCursor = OpaqueData.u.VarPackage.VarNumElementsStart;
		State->DataLength = ( State->DataCursor + OpaqueData.u.VarPackage.ByteSize );

		//
		// VarNumElements := TermArg => Integer
		// DefVarPackage := VarPackageOp PkgLength | VarNumElements PackageElementList
		//                                         ^-- Opaque data begins here.
		//
		if( AmlEvalTermArg( State, 0, &VarNumElements ) == AML_FALSE ) {
			return AML_FALSE;
		} else if( VarNumElements.Type != AML_DATA_TYPE_INTEGER ) {
			return AML_FALSE;
		}

		//
		// Validate package element count.
		//
		if( OpaqueData.u.VarPackage.VarNumElementsStart > ( SIZE_MAX / sizeof( PackageElementList ) ) ) {
			return AML_FALSE;
		}

		//
		// We have consumed and evaluated the opaque VarNumElements TermArg, we are now at the PackageElementList portion of DefVarPackage.
		//
		OpaqueDataOffset = ( State->DataCursor - OpaqueData.u.VarPackage.VarNumElementsStart );
		InitDataSize = ( OpaqueData.u.VarPackage.ByteSize - OpaqueDataOffset );

		//
		// Validate data parameters.
		//
		if( OpaqueDataOffset > OpaqueData.u.VarPackage.ByteSize ) {
			return AML_FALSE;
		} else if( AmlDecoderIsValidDataWindow( State, State->DataCursor, InitDataSize ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// Allocate a new internal package data object.
		//
		if( ( Package = AmlHeapAllocate( &State->Heap, sizeof( *Package ) ) ) == NULL ) {
			return AML_FALSE;
		}

		//
		// Allocate element array for the package.
		//
		PackageElementList = AmlHeapAllocate( &State->Heap, ( sizeof( *PackageElementList ) * VarNumElements.u.Integer ) );
		if( PackageElementList == NULL ) {
			AmlHeapFree( &State->Heap, Package );
			return AML_FALSE;
		}

		//
		// Zero initialize all package elements for safe releasing.
		//
		for( i = 0; i < VarNumElements.u.Integer; i++ ) {
			PackageElementList[ i ] = NULL;
		}

		//
		// Initialize the package object.
		//
		*Package = ( AML_PACKAGE_DATA ){
			.ReferenceCount   = 1,
			.ParentHeap       = &State->Heap,
			.ElementArrayHeap = &State->Heap,
			.ElementCount     = VarNumElements.u.Integer,
			.Elements         = PackageElementList
		};

		//
		// Evaluate and initialize all package elements.
		//
		if( AmlEvalPackageElementList( State,
									   Package,
									   ( State->DataCursor + InitDataSize ) ) == AML_FALSE )
		{
			AmlPackageDataRelease( Package );
			return AML_FALSE;
		}

		//
		// Return created package.
		//
		*Data = ( AML_DATA ){
			.Type      = AML_DATA_TYPE_PACKAGE,
			.u.Package = Package,
		};

		//
		// Restore the old window (to the end of the instruction).
		//
		State->DataCursor = OldDataCursor;
		State->DataLength = OldDataLength;
		break;
	default:
		return AML_FALSE;
	}

	return AML_TRUE;
}

//
// Evaluate a method invocation to a created method object,
// assumes that the NameString has already been consumed from the input stream,
// that the data cursor is currently pointing to the call's arguments.
//
_Success_( return )
BOOLEAN
AmlEvalMethodInvocationInternal(
	_Inout_   AML_STATE*  State,
	_Inout_   AML_OBJECT* MethodObject,
	_In_      UINT        Flags,
	_Out_opt_ AML_DATA*   pReturnValueOutput
	)
{
	AML_NAMESPACE_NODE* MethodNsNode;
	AML_DATA            Arguments[ 8 ];
	SIZE_T              i;
	BOOLEAN             Success;

	//
	// The object must have a namespace node attached (for scope information).
	//
	MethodNsNode = MethodObject->NamespaceNode;
	if( MethodNsNode == NULL ) {
		return AML_FALSE;
	}

	//
	// Must have already been determined to be a method object.
	//
	if( MethodObject->Type != AML_OBJECT_TYPE_METHOD ) {
		return AML_FALSE;
	}

	//
	// Validate argument count.
	//
	if( MethodObject->u.Method.ArgumentCount > AML_COUNTOF( Arguments ) ) {
		return AML_FALSE;
	}

	//
	// Pre-initialize all arguments to empty by default for simpler freeing logic.
	//
	for( i = 0; i < AML_COUNTOF( Arguments ); i++ ) {
		Arguments[ i ] = ( AML_DATA ){ .Type = AML_DATA_TYPE_NONE };
	}

	//
	// Attempt to parse and evaluate all arguments.
	//
	Success = AML_TRUE;
	for( i = 0; i < MethodObject->u.Method.ArgumentCount; i++ ) {
		if( ( Success = AmlEvalTermArg( State, 0, &Arguments[ i ] ) ) == AML_FALSE ) {
			break;
		}
	}

	//
	// Attempt to perform the actual method invocation using the evaluated arguments.
	//
	if( Success ) {
		Success = AmlMethodInvoke( State,
								   MethodObject,
								   Flags,
								   Arguments,
								   MethodObject->u.Method.ArgumentCount,
								   pReturnValueOutput );
	}

	//
	// Release all parsed arguments.
	//
	for( i = 0; i < AML_COUNTOF( Arguments ); i++ ) {
		AmlDataFree( &Arguments[ i ] );
	}

	return Success;
}

//
// Evaluate a method invocation to a created method object.
// Assumes that the IP is currently pointing to the NameString start of the MethodInvocation, and consumes it.
//
_Success_( return )
BOOLEAN
AmlEvalMethodInvocation(
	_Inout_   AML_STATE* State,
	_In_      UINT       Flags,
	_Out_opt_ AML_DATA*  pReturnValueOutput
	)
{
	AML_NAME_STRING     NameString;
	SIZE_T              OldCursor;
	AML_NAMESPACE_NODE* NsNode;

	//
	// Backup old IP to restore if we couldn't match a MethodInvocation.
	//
	OldCursor = State->DataCursor;

	//
	// Attempt to match and consume the method name.
	//
	if( AmlDecoderMatchNameString( State, AML_FALSE, &NameString ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Search the name and try to figure out what it is.
	//
	if( AmlNamespaceSearch( &State->Namespace, State->Namespace.ScopeLast, &NameString, 0, &NsNode ) == AML_FALSE ) {
		State->DataCursor = OldCursor;
		return AML_FALSE;
	}

	//
	// If the found namespace node object isn't a method, bail out and restore original cursor.
	//
	if( NsNode->Object->Type != AML_OBJECT_TYPE_METHOD ) {
		State->DataCursor = OldCursor;
		return AML_FALSE;
	}

	//
	// Execute the MethodInvocation using the found method object.
	//
	return AmlEvalMethodInvocationInternal( State, NsNode->Object, Flags, pReturnValueOutput );
}

//
// Evaluate a TermArg.
// TermArg := ExpressionOpcode | DataObject | ArgObj | LocalObj
//
_Success_( return )
BOOLEAN
AmlEvalTermArg(
	_Inout_ AML_STATE* State,
	_In_    UINT       TermArgFlags,
	_Out_   AML_DATA*  ValueData
	)
{
	AML_DECODER_INSTRUCTION_OPCODE Opcode;
	UINT8                          ArgIndex;
	UINT8                          LocalIndex;
	BOOLEAN                        IsTemporary;
	AML_DATA*                      ReadData;
	AML_NAME_STRING                NameString;
	AML_NAMESPACE_NODE*            NsNode;

	//
	// Handle ArgObj, reads the corresponding argument for the current method call scope.
	// 
	if( AmlDecoderMatchArgObj( State, &ArgIndex ) ) {
		//
		// Ensure that parsed argument index is valid (should always be, unless code is changed).
		//
		if( ArgIndex >= AML_COUNTOF( State->MethodScopeLast->ArgObjects ) ) {
			return AML_FALSE;
		}

		//
		// Must be within the scope of a method call.
		// Acpica acpiexec seems to trigger error but continue execution and allows access to Arg/Locals, as if there was a parent scope.
		//
		if( State->MethodScopeLast == State->MethodScopeRoot ) {
			AML_DEBUG_WARNING( State, "Warning: ArgObj operand used outside of method scope.\n" );
		}

		//
		// Return the value of this argument.
		// 
		// Read from ArgX parameters:
		// ObjectReference        - Automatic dereference, return the target of the reference. Use of DeRefOf returns the same.
		// Buffer                 - Return the Buffer. Can create an Index, Field, or Reference to the buffer.
		// Package                - Return the Package. Can create an Index or Reference to the package
		// All other object types - Return the object.
		// 
		// If this function is being called for DerefOf, skip automatic dereferencing,
		// as DerefOf is expecting an ObjectReference TermArg!
		// 
		// Note: it is possible that this method doesn't have that many arguments,
		// in this case, OBJECT_TYPE_NONE is returned, rather than a fatal error.
		// 
		ReadData = &State->MethodScopeLast->ArgObjects[ ArgIndex ]->u.Name.Value;
		if( ( ReadData->Type == AML_DATA_TYPE_REFERENCE )
			&& ( ( TermArgFlags & AML_EVAL_TERM_ARG_FLAG_IS_DEREFOF ) == 0 ) )
		{
			if( ReadData->u.Reference.Object->Type != AML_OBJECT_TYPE_NAME ) {
				AML_DEBUG_ERROR( State, "Error: Attempting to read from a reference to non-name/data.\n" );
				return AML_FALSE;
			}
			ReadData = &ReadData->u.Reference.Object->u.Name.Value;
		}

		//
		// Duplicate the data, the returned copy must be freed by the caller.
		//
		return AmlDataDuplicate( ReadData, &State->Heap, ValueData );
	}

	//
	// Handle LocalObj, reads the corresponding local variable for the current method call scope.
	//
	if( AmlDecoderMatchLocalObj( State, &LocalIndex ) ) {
		//
		// Must be within the scope of a method call.
		// Acpica acpiexec seems to trigger error but continue execution and allows access to Arg/Locals, as if there was a parent scope.
		//
		if( State->MethodScopeLast == State->MethodScopeRoot ) {
			AML_DEBUG_WARNING( State, "Warning: LocalObj operand used outside of method scope.\n" );
		}

		//
		// Ensure that the parsed local variable index is valid (should always be, unless code is changed).
		//
		if( ( LocalIndex >= AML_COUNTOF( State->MethodScopeLast->LocalObjects ) ) ) {
			return AML_FALSE;
		}

		//
		// Return the current value of the local variable (may be NONE).
		// 
		// Read from LocalX variables
        // ObjectReference        - If performing a DeRefOf return the target of the reference. Otherwise, return the reference.
        // All other object types - Return the object.
		//
		ReadData = &State->MethodScopeLast->LocalObjects[ LocalIndex ]->u.Name.Value;

		//
		// Duplicate the data, the returned copy must be freed by the caller.
		//
		return AmlDataDuplicate( ReadData, &State->Heap, ValueData );
	}

	//
	// Peek the next instruction opcode, AmlEval* functions are not the same as match,
	// they may partially evaluate/match data and error out, an eval function returning false
	// can either mean no match, or evaluation error.
	//
	if( AmlDecoderPeekOpcode( State, &Opcode ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Handle DataObject.
	//
	if( AmlDecoderIsDataObject( State, &Opcode ) ) {
		IsTemporary = ( ( TermArgFlags & AML_EVAL_TERM_ARG_FLAG_TEMP ) != 0 );
		if( AmlEvalDataObject( State, ValueData, IsTemporary ) == AML_FALSE ) {
			AML_DEBUG_ERROR( State, "Error: AmlEvalTermArg: AmlEvalDataObject failed!\n" );
			return AML_FALSE;
		}
		return AML_TRUE;
	}

	//
	// Match an ExpressionOpcode, but don't try to match a MethodInvocation,
	// as we need special logic for here for handling named objects and method invocations.
	//
	if( AmlDecoderIsExpressionOpcode( State, &Opcode ) ) {
		if( AmlEvalExpressionOpcode( State, AML_FALSE, ValueData ) == AML_FALSE ) {
			AML_DEBUG_ERROR( State, "Error: AmlEvalTermArg: AmlEvalExpressionOpcode failed!\n" );
			return AML_FALSE;
		}
		return AML_TRUE;
	}

	//
	// Nothing else was matched, we must now try to match an object name or MethodInvocation (both start with NameStrings).
	//
	if( AmlDecoderMatchNameString( State, AML_FALSE, &NameString ) ) {
		//
		// Search the name and try to figure out what it is.
		//
		if( AmlNamespaceSearch( &State->Namespace, State->Namespace.ScopeLast, &NameString, 0, &NsNode ) == AML_FALSE ) {
			AML_DEBUG_ERROR( State, "Error: AmlEvalTermArg: namespace search failed for name: \"" );
			AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_ERROR, &NameString );
			AML_DEBUG_ERROR( State, "\"\n" );
			return AML_FALSE;
		}

		//
		// Handle named object types.
		//
		switch( NsNode->Object->Type ) {
		case AML_OBJECT_TYPE_NAME:
			//
			// Read from Named object.
			// ObjectReference - If performing a DeRefOf return the target of the reference. Otherwise, return the reference.
			// All other object types - Return the object
			// TODO: (?) Handle ObjectReference + Alias.
			//
			return AmlDataDuplicate( &NsNode->Object->u.Name.Value, &State->Heap, ValueData );
		case AML_OBJECT_TYPE_BUFFER_FIELD:
		case AML_OBJECT_TYPE_INDEX_FIELD:
		case AML_OBJECT_TYPE_BANK_FIELD:
		case AML_OBJECT_TYPE_FIELD:
			//
			// Return data referencing the field unit object.
			//
			*ValueData = ( AML_DATA ){
				.Type        = AML_DATA_TYPE_FIELD_UNIT,
				.u.FieldUnit = NsNode->Object
			};
			AmlObjectReference( NsNode->Object );
			return AML_TRUE;
		case AML_OBJECT_TYPE_METHOD:
			//
			// Handle method invocation.
			//
			return AmlEvalMethodInvocationInternal( State, NsNode->Object, 0, ValueData );
		default:
			AML_DEBUG_ERROR( State, "Error: AmlEvalTermArg: unsupported named object type!\n" );
			return AML_FALSE;
		}
	}

	AML_DEBUG_ERROR( State, "Error: AmlEvalTermArg: no valid TermArg matched!\n" );
	return AML_FALSE;
}

//
// Attempt to evaluate a TermArg and then apply implicit conversion to the given type.
// Passing a ConversionType of AML_DATA_TYPE_NONE performs no conversion.
//
_Success_( return )
BOOLEAN
AmlEvalTermArgToType(
	_Inout_ AML_STATE*    State,
	_In_    UINT          TermArgFlags,
	_In_    AML_DATA_TYPE ConversionType,
	_Out_   AML_DATA*     ValueData
	)
{
	AML_CONV_FLAGS ConvFlags;

	//
	// Attempt to evaluate a TermArg.
	//
	if( AmlEvalTermArg( State, TermArgFlags, ValueData ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Attempt to convert the evaluated TermArg to the given ConversionType.
	//
	if( ( ConversionType != AML_DATA_TYPE_NONE ) && ( ValueData->Type != ConversionType ) ) {
		ConvFlags  = AML_CONV_FLAGS_IMPLICIT;
		ConvFlags |= ( ( TermArgFlags & AML_EVAL_TERM_ARG_FLAG_TEMP ) ? AML_CONV_FLAGS_TEMPORARY : 0 );
		if( AmlConvObjectInPlace( State, &State->Heap, ValueData, ConversionType, ConvFlags ) == AML_FALSE ) {
			AmlDataFree( ValueData );
			return AML_FALSE;
		}
	}

	return AML_TRUE;
}

//
// Attempt to evaluate/execute the next opcode/term of the input window.
//
_Success_( return )
static
BOOLEAN
AmlEvalTerm(
	_Inout_ AML_STATE* State
	)
{
	AML_DECODER_INSTRUCTION_OPCODE Next;
	AML_DATA                       Result;

	//
	// Peek next full opcode.
	//
	if( AmlDecoderPeekOpcode( State, &Next ) == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: AmlDecoderPeekOpcode failed!\n" );
		return AML_FALSE;
	}
	AML_DEBUG_TRACE( State, "Executing opcode 0x%x @ +0x%08"PRIx64"\n", ( UINT )Next.OpcodeID, ( UINT64 )State->DataCursor );

	//
	// Evaluate statement opcodes.
	//
	if( AmlDecoderIsStatementOpcode( State, &Next ) ) {
		return AmlEvalStatementOpcode( State );
	}

	//
	// Evaluate namespace modifier object opcodes.
	//
	if( AmlDecoderIsNamespaceModifierObjectOpcode( State, &Next ) ) {
		return AmlEvalNamespaceModifierObjectOpcode( State );
	}

	//
	// Named object opcodes.
	//
	if( AmlDecoderIsNamedObjectOpcode( State, &Next ) ) {
		return AmlEvalNamedObjectOpcode( State );
	}

	//
	// Evaluate expression opcodes.
	// Temp hack to free any results.
	//
	if( AmlDecoderIsExpressionOpcode( State, &Next ) ) {
		if( AmlEvalExpressionOpcode( State, AML_TRUE, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}
		AmlDataFree( &Result );
		return AML_TRUE;
	}

	//
	// Ignore data in the regular instruction stream.
	//
	if( AmlDecoderMatchDataObject( State, NULL ) ) {
		AML_DEBUG_TRACE( State, "Skipping pointless data object in TermList instruction stream.\n" );
		return AML_TRUE;
	}

	//
	// Nothing else was matched, try to match and evaluate a MethodInvocation.
	//
	if( AmlEvalMethodInvocation( State, 0, &Result ) ) {
		AmlDataFree( &Result );
		return AML_TRUE;
	}

	AML_DEBUG_ERROR( State, "Error: AmlEvalTerm failed!\n" );
	return AML_FALSE;
}

//
// EvalTerm equivalent for the initial namespace pass,
// skips the majority of instructions, handling only namespace/scope definition.
//
_Success_( return )
static
BOOLEAN
AmlEvalTermNamespacePass(
	_Inout_ AML_STATE* State
	)
{
	AML_DECODER_INSTRUCTION_OPCODE Instruction;

	//
	// Peek the next instruction's information to determine if it should be evaluated or skipped.
	//
	if( AmlDecoderPeekOpcode( State, &Instruction ) == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: AmlDecoderPeekOpcode failed!\n" );
		return AML_FALSE;
	}
	AML_DEBUG_TRACE( State, "Executing opcode 0x%x @ +0x%08"PRIx64"\n", ( UINT )Instruction.OpcodeID, ( UINT64 )State->DataCursor );

	//
	// Handle instructions that we actually want to evaluate in the namespace pass.
	//
	switch( Instruction.OpcodeID ) {
	case AML_OPCODE_ID_METHOD_OP:
		return AmlEvalMethod( State, AML_TRUE );
	case AML_OPCODE_ID_SCOPE_OP:
		return AmlEvalScope( State, AML_TRUE );
	case AML_OPCODE_ID_ALIAS_OP:
		return AmlEvalAlias( State, AML_TRUE );
	case AML_OPCODE_ID_DEVICE_OP:
		return AmlEvalDevice( State, AML_TRUE );
	case AML_OPCODE_ID_EXTERNAL_OP:
		return AmlEvalExternal( State, AML_TRUE );
	case AML_OPCODE_ID_PROCESSOR_OP:
		return AmlEvalProcessor( State, AML_TRUE );
	case AML_OPCODE_ID_POWER_RES_OP:
		return AmlEvalPowerRes( State, AML_TRUE );
	case AML_OPCODE_ID_THERMAL_ZONE_OP:
		return AmlEvalThermalZone( State, AML_TRUE );
	case AML_OPCODE_ID_IF_OP:
		return AmlEvalIfElse( State, AML_TRUE );
	}

	//
	// Consume and skip the entire instruction, we aren't interested in it in this pass.
	//
	return AmlDecoderConsumeInstruction( State );
}

//
// Attempt to evaluate an entire block of code until reaching the end.
//
_Success_( return )
BOOLEAN
AmlEvalTermListCode(
	_Inout_ AML_STATE* State,
	_In_    SIZE_T     CodeStart,
	_In_    SIZE_T     CodeSize,
	_In_    BOOLEAN    RestoreDataCursor
	)
{
	SIZE_T OldCursor;
	SIZE_T OldSize;

	//
	// The given code block must be within the bounds of the input data.
	//
	if( CodeStart >= State->DataTotalLength || CodeSize > ( State->DataTotalLength - CodeStart ) ) {
		return AML_FALSE;
	}

	//
	// Save old decoder cursor/window size before jumping to the given start offset, will be restored after executing the chunk of input code.
	//
	OldCursor = State->DataCursor;
	OldSize = State->DataLength;

	//
	// Move interpreter data window to input code block.
	// Note: this may possibly allow data to be read below the code block,
	// as the lower bound of the interpreter window is currently always 0, but never out of bounds.
	//
	State->DataCursor = CodeStart;
	State->DataLength = ( CodeStart + CodeSize );

	//
	// Recursively execute terms until we reach the end of the input code block.
	//
	while( State->DataCursor < State->DataLength ) {
		//
		// If we are within an interruptible code block, stop executing if we hit an interruption event,
		// so that the outer caller can handle the event accordingly.
		// TODO: While loop events are per-method, a call to a method containing a break inside of a while loop
		// should not cause the outer while loop to break!
		//
		if( State->PendingInterruptionEvent != AML_INTERRUPTION_EVENT_NONE ) {
			break;
		}

		//
		// Process/evaluate the next term.
		//
		if( State->PassType == AML_PASS_TYPE_NAMESPACE ) {
			if( AmlEvalTermNamespacePass( State ) == AML_FALSE ) {
				return AML_FALSE;
			}
		} else if( AmlEvalTerm( State ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	//
	// Restore old interpreter data window.
	//
	if( RestoreDataCursor ) {
		State->DataCursor = OldCursor;
	}
	State->DataLength = OldSize; 
	return AML_TRUE;
}

//
// Evaluates a separate table data block and loads it to the namespace.
// This is used for SSDTs/Load/LoadTable. Overrides the current decoder state to use the given code during loading.
// The given table code must remain loaded for the entirety of execution proceeding this call!
// Note: This function is internal and is designed to be used within a state snapshot,
// this function also does not call _REG/_INI handlers for the loaded table!
//
_Success_( return )
static
BOOLEAN
AmlEvalLoadedTableCodeInternal(
	_Inout_                           AML_STATE*             State,
	_In_reads_bytes_( TableCodeSize ) const VOID*            TableCode,
	_In_                              SIZE_T                 TableCodeSize,
	_In_opt_                          const AML_NAME_STRING* TableRootPath
	)
{
	AML_NAME_STRING RootPath;

	//
	// The default namespace location to load the Definition Block is relative to the root of the namespace. 
	//
	if( TableRootPath != NULL ) {
		RootPath = *TableRootPath;
	} else {
		RootPath = ( AML_NAME_STRING ){ .Prefix = { .Data = { '\\' }, .Length = 1 } };
	}

	//
	// Use the new namespace scope with the given path while evaluating the table's code.
	//
	if( AmlNamespacePushScope( &State->Namespace, &RootPath, ( AML_SCOPE_FLAG_SWITCH | AML_SCOPE_FLAG_BOUNDARY ) ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Begin a new default method scope for the table,
	// ensures that the new table cannot modify the existing method context.
	//
	if( AmlMethodPushScope( State ) == AML_FALSE ) {
		AmlNamespacePopScope( &State->Namespace );
		return AML_FALSE;
	}

	//
	// Change to the new decoder state to begin decoding the input table.
	//
	State->Data = TableCode;
	State->DataCursor = 0;
	State->DataLength = TableCodeSize;
	State->DataTotalLength = TableCodeSize;
	State->PendingInterruptionEvent = AML_INTERRUPTION_EVENT_NONE;
	State->WhileLoopLevel = 0;
	State->MethodScopeLevelStart = State->MethodScopeLevel;

	//
	// Perform the initial namespace construction pass, will perform a light pass over code to build the namespace.
	// Does not perform full evaluation of any expressions or values, but only attempts to discover named objects
	// that may be forward referenced during the full evaluation pass.
	//
	State->PassType = AML_PASS_TYPE_NAMESPACE;
	if( AmlEvalTermListCode( State, 0, TableCodeSize, AML_TRUE ) == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: AmlEvalLoadedTableCode namespace pass failed!\n" );
		return AML_FALSE;
	}

	//
	// Perform the full evaluation pass, executes all code and builds the full namespace.
	//
	State->PassType = AML_PASS_TYPE_FULL;
	if( AmlEvalTermListCode( State, 0, TableCodeSize, AML_TRUE ) == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: AmlEvalLoadedTableCode full evaluation pass failed!\n" );
		return AML_FALSE;
	}

	return AML_TRUE;
}

//
// Evaluates a separate table data block and loads it to the namespace.
// This is used for SSDTs/Load/LoadTable. Overrides the current decoder state to use the given code during loading.
// The given table code must remain loaded for the entirety of execution proceeding this call!
//
_Success_( return )
BOOLEAN
AmlEvalLoadedTableCode(
	_Inout_                           AML_STATE*             State,
	_In_reads_bytes_( TableCodeSize ) const VOID*            TableCode,
	_In_                              SIZE_T                 TableCodeSize,
	_In_opt_                          const AML_NAME_STRING* TableRootPath
	)
{
	UINT8                           i;
	AML_REGION_ACCESS_REGISTRATION* Handler;

	//
	// Since we are switching the decoder to use a new block of code,
	// we must backup and restore the old decoder state. Otherwise,
	// any code currently in progress of execution will fail (for example: the Load or LoadTable cases).
	//
	if( AmlStateSnapshotBegin( State ) == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: AmlEvalLoadedTableCode state snapshot push failure!\n" );
		return AML_FALSE;
	}

	//
	// Attempt to execute the actual table code, restore state upon success, rollback certain changes upon failure.
	//
	if( AmlEvalLoadedTableCodeInternal( State, TableCode, TableCodeSize, TableRootPath ) == AML_FALSE ) {
		AmlStateSnapshotRollback( State );
		return AML_FALSE;
	}
	AmlStateSnapshotCommit( State, AML_TRUE );

	//
	// If this is after the initial table load (dynamically loaded code),
	// attempt to broadcast the state of any registered region space handlers,
	// and attempt to call any possible newly loaded device initializers.
	//
	if( State->IsInitialLoadComplete ) {
		//
		// Call any region space registration handlers that have yet to be informed of a state update.
		// This is only necessary to handle any state updates performed before completion of the initial load.
		// TODO: Do this in one pass instead of traversing the tree for every single handler type!
		//
		for( i = 0; i < AML_COUNTOF( State->RegionSpaceHandlers ); i++ ) {
			Handler = &State->RegionSpaceHandlers[ i ];
			if( Handler->EnableState == AML_FALSE ) {
				continue;
			}
			AmlBroadcastRegionSpaceStateUpdate( State, i, AML_TRUE, AML_TRUE );
		}

		//
		// Query the status of all devices present in the namespace and attempt to call their initializers if present.
		//
		if( AmlInitializeDevices( State, &State->Namespace.TreeRoot, AML_TRUE ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	return AML_TRUE;
}

//
// Attempt to evaluate the given object to data.
// If ToPrimitive is set, we will attempt to resolve the data to its lowest primitive value,
// that is, package elements will be dereferenced to their underlying element, and field units
// will be actually read from. This function will call methods without any arguments.
//
_Success_( return )
BOOLEAN
AmlEvalObject(
	_Inout_     AML_STATE*  State,
	_Inout_opt_ AML_OBJECT* Object,
	_Out_       AML_DATA*   Output,
	_In_        BOOLEAN     ToPrimitive
	)
{
	AML_DATA Result;
	BOOLEAN  Success;

	//
	// Validate input object pointer.
	//
	if( Object == NULL ) {
		return AML_FALSE;
	}

	//
	// If this is a named object, just return the value as is.
	// Other than named values and methods, only fields need special handling.
	//
	if( Object->Type == AML_OBJECT_TYPE_NAME ) {
		if( AmlDataDuplicate( &Object->u.Name.Value, &State->Heap, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	//
	// If the object is a method, we can call it if it has no arguments,
	// for example: _ADR, things that can be a value or method returning a value).
	//
	if( Object->Type == AML_OBJECT_TYPE_METHOD ) {
		if( Object->u.Method.ArgumentCount != 0 ) {
			return AML_FALSE;
		} else if( AmlMethodInvoke( State, Object, 0, NULL, 0, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	//
	// Special case for field objects, convert them to a plain data reference and try to evaluate the underlying value.
	//
	switch( Object->Type ) {
	case AML_OBJECT_TYPE_NAME:
		if( AmlDataDuplicate( &Object->u.Name.Value, &State->Heap, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}
		break;
	case AML_OBJECT_TYPE_METHOD:
		if( Object->u.Method.ArgumentCount != 0 ) {
			return AML_FALSE;
		} else if( AmlMethodInvoke( State, Object, 0, NULL, 0, &Result ) == AML_FALSE ) {
			return AML_FALSE;
		}
		break;
	case AML_OBJECT_TYPE_FIELD:
	case AML_OBJECT_TYPE_BANK_FIELD:
	case AML_OBJECT_TYPE_INDEX_FIELD:
	case AML_OBJECT_TYPE_BUFFER_FIELD:
		Result = ( AML_DATA ){ .Type = AML_DATA_TYPE_FIELD_UNIT, .u.FieldUnit = Object };
		AmlObjectReference( Object );
		break;
	default:
		return AML_FALSE;
	}

	//
	// If desired by the user, attempt to use the conv system to output/read the actual primitive value of the data.
	// For example: Read from the field, access the underlying package element, etc.
	//
	if( ToPrimitive ) {
		*Output = ( AML_DATA ){ .Type = AML_DATA_TYPE_NONE };
		Success = AmlConvObjectStore( State, &State->Heap, &Result, Output, AML_CONV_FLAGS_IMPLICIT );
		AmlDataFree( &Result );
		return Success;
	}

	//
	// No lowering to primitive type desired, return the data as-is (package elements and fields remain references).
	//
	*Output = Result;
	return AML_TRUE;
}

//
// Attempt to evaluate the given object to data of the given type.
// Does not implicitly convert the result to the input type, only verifies that the evaluated output matches the type.
// If ToPrimitive is set, we will attempt to resolve the data to its lowest primitive value,
// that is, package elements will be dereferenced to their underlying element, and field units
// will be actually read from. This function will call methods without any arguments.
//
_Success_( return )
BOOLEAN
AmlEvalObjectOfType(
	_Inout_     AML_STATE*    State,
	_Inout_opt_ AML_OBJECT*   Object,
	_Out_       AML_DATA*     Output,
	_In_        BOOLEAN       ToPrimitive,
	_In_        AML_DATA_TYPE TypeConstraint
	)
{
	AML_DATA Result;

	//
	// Attempt to evaluate the object's underlying value and ensure that it matches the desired type.
	//
	if( AmlEvalObject( State, Object, &Result, ToPrimitive ) == AML_FALSE ) {
		return AML_FALSE;
	} else if( Result.Type != TypeConstraint ) {
		AmlDataFree( &Result );
		return AML_FALSE;
	}
	*Output = Result;
	return AML_TRUE;
}

//
// Attempt to evaluate the _HID and _CID values of the given node.
//
_Success_( return )
BOOLEAN
AmlEvalNodeDeviceId(
	_Inout_   AML_STATE*          State,
	_In_      AML_NAMESPACE_NODE* Node,
	_Out_opt_ AML_DATA*           HidValue,
	_Out_opt_ AML_DATA*           CidValue,
	_In_      BOOLEAN             SearchAncestors
	)
{
	BOOLEAN             Success;
	AML_NAME_STRING     Name;
	AML_NAMESPACE_NODE* Child;

	//
	// Default to failure, only return successfully if at least one of the desired HID/CID values was found.
	//
	Success = AML_FALSE;

	//
	// Search for an _HID node that tells us the actual ID of the device.
	//
	if( HidValue != NULL ) {
		*HidValue = ( AML_DATA ){ .Type = AML_DATA_TYPE_NONE };
		Name = ( AML_NAME_STRING ){ .Segments = ( AML_NAME_SEG[] ){ { .Data = { '_', 'H', 'I', 'D' } } }, .SegmentCount = 1 };
		if( ( Child = AmlNamespaceSearchRelative( &State->Namespace, Node, &Name, SearchAncestors ) ) != NULL ) {
			if( AmlEvalObject( State, Child->Object, HidValue, AML_TRUE ) ) {
				Success |= AML_TRUE;
			}
		}
	}

	//
	// Search for a _CID node that tells us the device ID that this device is compatible with.
	//
	if( CidValue != NULL ) {
		*CidValue = ( AML_DATA ){ .Type = AML_DATA_TYPE_NONE };
		Name = ( AML_NAME_STRING ){ .Segments = ( AML_NAME_SEG[] ){ { .Data = { '_', 'C', 'I', 'D' } } }, .SegmentCount = 1 };
		if( ( Child = AmlNamespaceSearchRelative( &State->Namespace, Node, &Name, SearchAncestors ) ) != NULL ) {
			if( AmlEvalObject( State, Child->Object, CidValue, AML_TRUE ) ) {
				Success |= AML_TRUE;
			}
		}
	}

	return Success;
}

//
// Attempt to search for the given ChildName relative to the input Node,
// if the child is found, attempts to evaluate the found child.
// To be evaluated, the Child must be a named data object or a method with no arguments.
// ToPrimitive will attempt to evaluate to the most primitive type possible,
// for example: PackageElement references will be resolved, fields will be read from.
//
_Success_( return )
BOOLEAN
AmlEvalNodeChild(
	_Inout_   AML_STATE*             State,
	_In_      AML_NAMESPACE_NODE*    Node,
	_In_      const AML_NAME_STRING* ChildName,
	_In_opt_  AML_DATA_TYPE          TypeRequirement,
	_In_      BOOLEAN                SearchAncestors,
	_In_      BOOLEAN                ToPrimitive,
	_Out_opt_ AML_DATA*              Result
	)
{
	AML_NAMESPACE_NODE* Child;
	AML_DATA            EvalData;

	//
	// Attempt to search for the ChildName relative to the input node.
	//
	Child = AmlNamespaceSearchRelative( &State->Namespace, Node, ChildName, SearchAncestors );
	if( Child == NULL ) {
		return AML_FALSE;
	}

	//
	// Attempt to evaluate the found node to data.
	//
	if( AmlEvalObject( State, Child->Object, &EvalData, ToPrimitive ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// If the evaluated type doesn't match the requirement, free the data and bail out.
	// TODO: Support implicit conversion if the types dont match?
	//
	if( ( TypeRequirement != AML_DATA_TYPE_NONE ) && ( EvalData.Type != TypeRequirement ) ) {
		AmlDataFree( &EvalData );
		return AML_FALSE;
	}

	//
	// Disregard the evaluated data if the caller doesn't care about the final evaluated value.
	//
	if( Result == NULL ) {
		AmlDataFree( &EvalData );
		return AML_TRUE;
	}

	*Result = EvalData;
	return AML_TRUE;
}

//
// Attempt to search for the given ChildName relative to the input Node,
// if the child is found, attempts to evaluate the found child.
// To be evaluated, the Child must be a named data object or a method with no arguments.
// ToPrimitive will attempt to evaluate to the most primitive type possible,
// for example: PackageElement references will be resolved, fields will be read from.
//
_Success_( return )
BOOLEAN
AmlEvalNodeChildZ(
	_Inout_   AML_STATE*          State,
	_In_      AML_NAMESPACE_NODE* Node,
	_In_      const CHAR*         ChildName,
	_In_opt_  AML_DATA_TYPE       TypeRequirement,
	_In_      BOOLEAN             SearchAncestors,
	_In_      BOOLEAN             ToPrimitive,
	_Out_opt_ AML_DATA*           Result
	)
{
	AML_ARENA_SNAPSHOT Snapshot;
	AML_NAME_STRING    Name;
	BOOLEAN            Success;

	//
	// Attempt to parse a null-terminated path string to an AML_NAME_STRING and search like normal.
	// TODO: Use separate TempArena instead of the namespace one.
	//
	Success = AML_FALSE;
	Snapshot = AmlArenaSnapshot( &State->Namespace.TempArena );
	if( AmlPathStringZToNameString( &State->Namespace.TempArena, ChildName, &Name ) ) {
		Success = AmlEvalNodeChild( State, Node, &Name, TypeRequirement, SearchAncestors, ToPrimitive, Result );
	}
	AmlArenaSnapshotRollback( &State->Namespace.TempArena, &Snapshot );
	return Success;
}

//
// Attempt to search for the given node's parent PCI(e) root bus.
// Searches ancestors for a _HID or _CID matching the PCI(e) root bus EISAIDs.
//
_Success_( return != NULL )
AML_NAMESPACE_NODE*
AmlEvalNodePciParentRootBus(
	_Inout_ AML_STATE*          State,
	_In_    AML_NAMESPACE_NODE* Node
	)
{
	AML_NAMESPACE_NODE* Ancestor;
	AML_DATA            HidValue;
	AML_DATA            CidValue;
	AML_NAMESPACE_NODE* RootBus;

	//
	// Search all ancestors of the given node for the parent PCI(e) root bus (if any).
	//
	RootBus = NULL;
	for( Ancestor = Node; ( Ancestor != NULL ) && ( RootBus == NULL ); ) {
		if( AmlEvalNodeDeviceId( State, Ancestor, &HidValue, &CidValue, AML_FALSE ) ) {
			if( AmlDataCompareEisaId( &HidValue, AML_EISA_ID_PCI_ROOT_BUS )
				|| AmlDataCompareEisaId( &HidValue, AML_EISA_ID_PCIE_ROOT_BUS )
				|| AmlDataCompareEisaId( &CidValue, AML_EISA_ID_PCI_ROOT_BUS )
				|| AmlDataCompareEisaId( &CidValue, AML_EISA_ID_PCIE_ROOT_BUS ) )
			{
				RootBus = Ancestor;
			}
			AmlDataFree( &HidValue );
			AmlDataFree( &CidValue );
		}
		Ancestor = AmlNamespaceParentNode( &State->Namespace, Ancestor );
	}

	return RootBus;
}

//
// Attempt to search for and evalute the full SBDF address for the given PCI device object.
// Attempts to evaluate the _ADR of the device, and the _BBN and _SEG of the device's parent PCI root bus.
//
_Success_( return )
BOOLEAN
AmlEvalNodePciAddressLocal(
	_Inout_      AML_STATE*            State,
	_In_opt_     AML_NAMESPACE_NODE*   Node,
	_Out_        AML_PCI_SBDF_ADDRESS* Output,
	_Outptr_opt_ AML_NAMESPACE_NODE**  ppParentDevice,
	_Outptr_opt_ AML_NAMESPACE_NODE**  ppRootBus
	)
{
	AML_NAME_STRING      Name;
	AML_PCI_SBDF_ADDRESS Bdf;
	AML_DATA             Value;
	AML_NAMESPACE_NODE*  Adr;
	AML_NAMESPACE_NODE*  Device;
	AML_NAMESPACE_NODE*  RootBus;

	//
	// Handle NULL input node for cases where the return value is passed directly from a traversal/search result.
	//
	if( Node == NULL ) {
		return AML_FALSE;
	}

	//
	// Zero initialize all SBDF fields by default, update them as we go.
	//
	Bdf = ( AML_PCI_SBDF_ADDRESS ){ 0 };

	//
	// Search for and attempt to evaluate the _ADR node of the device.
	// The _ADR value for a PCI device contains the PCI device number and function number.
	// The low 16-bits contain the function number, and the high 16-bits contain the device number.
	// Searches up all ancestors for the first _ADR node, not strictly just the parent of the node.
	// TODO: Smaller masks that properly match the maximum device/function values?
	// TODO: Default to 0 if no _ADR node is found?
	//
	Name = ( AML_NAME_STRING ){ .Segments = ( AML_NAME_SEG[] ){ { .Data = { '_', 'A', 'D', 'R' } } }, .SegmentCount = 1 };
	if( ( Adr = AmlNamespaceSearchRelative( &State->Namespace, Node, &Name, AML_TRUE ) ) == NULL ) {
		return AML_FALSE;
	} else if( ( Device = AmlNamespaceParentNode( &State->Namespace, Adr ) ) == NULL ) {
		return AML_FALSE;
	} else if( AmlEvalObjectOfType( State, Adr->Object, &Value, AML_TRUE, AML_DATA_TYPE_INTEGER ) == AML_FALSE ) {
		return AML_FALSE;
	}
	Bdf.Device = ( UINT8 )( ( Value.u.Integer >> 16 ) & 0xFF );
	Bdf.Function = ( UINT8 )( Value.u.Integer & 0xFF );

	//
	// Search up through the device's ancestors for the parent PCI root bus.
	//
	if( ( RootBus = AmlEvalNodePciParentRootBus( State, Device ) ) == NULL ) {
		return AML_FALSE;
	}

	//
	// Search for and attempt to evaluate the _BBN node of the root bus.
	// The _BBN value for a PCI root bus contains the bus number.
	// TODO: Fall back to _CRS?
	//
	Name = ( AML_NAME_STRING ){ .Segments = ( AML_NAME_SEG[] ){ { .Data = { '_', 'B', 'B', 'N' } } }, .SegmentCount = 1 };
	if( AmlEvalNodeChild( State, RootBus, &Name, AML_DATA_TYPE_INTEGER, AML_FALSE, AML_TRUE, &Value ) == AML_FALSE ) {
		return AML_FALSE;
	}
	Bdf.Bus = ( UINT8 )( Value.u.Integer & 0xFF );

	//
	// Search for and attempt to evaluate the _SEG node of the root bus.
	// If _SEG does not exist, OSPM assumes that all PCI bus segments are in PCI Segment Group 0.
	//
	Name = ( AML_NAME_STRING ){ .Segments = ( AML_NAME_SEG[] ){ { .Data = { '_', 'S', 'E', 'G' } } }, .SegmentCount = 1 };
	if( AmlEvalNodeChild( State, RootBus, &Name, AML_DATA_TYPE_INTEGER, AML_FALSE, AML_TRUE, &Value ) ) {
		Bdf.Segment = ( UINT16 )( Value.u.Integer & 0xFFFF );
	}

	//
	// Optionally return the found parent PCI device of the node.
	//
	if( ppParentDevice != NULL ) {
		*ppParentDevice = Device;
	}

	//
	// Optionally return the found PCI root bus of the device.
	//
	if( ppRootBus != NULL ) {
		*ppRootBus = RootBus;
	}

	//
	// Return parsed PCI device information.
	//
	*Output = Bdf;
	return AML_TRUE;
}

//
// Attempt to evaluate the full PCI hierarchy of the given node.
// Includes all PCI-to-PCI bridges along the way from the root bus to the device.
// Does not handle resolving the final address using the bridges.
//
_Success_( return )
BOOLEAN
AmlEvalNodePciInformation(
	_Inout_  AML_STATE*           State,
	_In_opt_ AML_NAMESPACE_NODE*  Node,
	_Out_    AML_PCI_INFORMATION* Output
	)
{
	AML_PCI_SBDF_ADDRESS Address;
	AML_NAMESPACE_NODE*  Device;
	AML_NAMESPACE_NODE*  RootBus;
	AML_NAMESPACE_NODE*  Current;
	AML_PCI_BRIDGE*      BridgeListHead;
	AML_NAME_STRING      Name;
	AML_NAMESPACE_NODE*  Adr;
	AML_DATA             Value;
	AML_PCI_BRIDGE*      Bridge;

	//
	// Query the bottom-level PCI address of the node, and root bus information.
	// This will also determine if the node is a valid PCI device.
	// This doesn't account for any PCI-to-PCI bridges encountered along the path up to the root bus.
	//
	if( AmlEvalNodePciAddressLocal( State, Node, &Address, &Device, &RootBus ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Traverse back up the hierarchy from the node to the root bus,
	// keeping track of all PCI-to-PCI bridge connections along the way.
	//
	BridgeListHead = NULL;
	for( Current = AmlNamespaceParentNode( &State->Namespace, Device );
		 ( ( Current != RootBus ) && ( Current != NULL ) );
		 Current = AmlNamespaceParentNode( &State->Namespace, Current ) )
	{
		//
		// Only consider device ancestors in the bridge discovery pass.
		// TODO: Change AmlEvalNodePciAddressLocal to only consider devices when searching for _ADR?
		//
		if( ( Current->Object == NULL ) || ( Current->Object->Type != AML_OBJECT_TYPE_DEVICE ) ) {
			continue;
		}

		//
		// If the found ancestor device has an _ADR value, this is probably a PCI-to-PCI bridge.
		//
		Name = ( AML_NAME_STRING ){ .Segments = ( AML_NAME_SEG[] ){ { .Data = { '_', 'A', 'D', 'R' } } }, .SegmentCount = 1 };
		if( ( Adr = AmlNamespaceSearchRelative( &State->Namespace, Current, &Name, AML_FALSE ) ) == NULL ) {
			continue;
		}

		//
		// Attempt to evaluate the ancestor's _ADR value, fatal error if it exists but we can't evaluate it.
		//
		if( AmlEvalObjectOfType( State, Adr->Object, &Value, AML_TRUE, AML_DATA_TYPE_INTEGER ) == AML_FALSE ) {
			return AML_FALSE;
		}

		//
		// We have discovered a PCI-to-PCI bridge, attempt to allocate and link a new bridge to the list.
		// The actual bus numbers/addresses of the bridges will be resolved upon usage.
		//
		if( ( Bridge = AmlHeapAllocate( &State->Heap, sizeof( *BridgeListHead ) ) ) == NULL ) {
			return AML_FALSE;
		}
		*Bridge = ( AML_PCI_BRIDGE ){
			.Next     = BridgeListHead,
			.Device   = ( UINT8 )( ( Value.u.Integer >> 16 ) & 0xFF ),
			.Function = ( UINT8 )( Value.u.Integer & 0xFF ),
		};
		BridgeListHead = Bridge;
	}

	//
	// Return evaluated PCI device information.
	//
	*Output = ( AML_PCI_INFORMATION ){
		.Heap           = &State->Heap,
		.BridgeHead     = BridgeListHead,
		.Address        = Address,
		.RootBusAddress = { .Segment = Address.Segment, .Bus = Address.Bus },
	};
	return AML_TRUE;
}

//
// Attempt to evaluate the _STA of the given device node.
// If a device object does not have an _STA object then OSPM assumes that all of the above bits are set
// (i.e., the device is present, enabled, shown in the UI, and functioning).
//
_Success_( return )
BOOLEAN
AmlEvalNodeDeviceStatus(
	_Inout_ AML_STATE*          State,
	_In_    AML_NAMESPACE_NODE* DeviceNode,
	_Out_   UINT32*             Result
	)
{
	AML_NAME_STRING     Name;
	AML_NAMESPACE_NODE* StaNode;
	AML_DATA            Value;

	//
	// Attempt to find the device's _STA node.
	// If a device object does not have an _STA object then OSPM assumes that all of the above bits are set
	// (i.e., the device is present, enabled, shown in the UI, and functioning).
	//
	Name = ( AML_NAME_STRING ){ .Segments = ( AML_NAME_SEG[] ){ { .Data = { '_', 'S', 'T', 'A' } } }, .SegmentCount = 1 };
	if( ( StaNode = AmlNamespaceChildNode( &State->Namespace, DeviceNode, &Name ) ) != NULL ) {
		if( AmlEvalObjectOfType( State, StaNode->Object, &Value, AML_TRUE, AML_DATA_TYPE_INTEGER ) == AML_FALSE ) {
			return AML_FALSE;
		}
		*Result = ( UINT32 )Value.u.Integer;
	} else {
		*Result = ~AML_STA_FLAG_RESERVED;
	}

	return AML_TRUE;
}
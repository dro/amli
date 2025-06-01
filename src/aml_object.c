#include "aml_state.h"
#include "aml_heap.h"
#include "aml_object.h"
#include "aml_debug.h"
#include "aml_host.h"
#include "aml_conv.h"
#include "aml_decoder.h"
#include "aml_mutex.h"
#include "aml_pci.h"

//
// Allocate and default-initialize a reference counted AML object.
//
_Success_( return )
BOOLEAN
AmlObjectCreate(
	_Inout_  struct _AML_HEAP* Heap,
	_In_     AML_OBJECT_TYPE   Type,
	_Outptr_ AML_OBJECT**      ppObject
	)
{
	AML_OBJECT* Object;

	//
	// Allocate new object.
	// TODO: Switch to object free-list, requires separate state for list.
	//
	if( ( Object = AmlHeapAllocate( Heap, sizeof( AML_OBJECT ) ) ) == NULL ) {
		return AML_FALSE;
	}

	//
	// Default initialize object, with a reference for the caller.
	//
	*Object = ( AML_OBJECT ){
		.ParentHeap     = Heap,
		.Type           = Type,
		.ReferenceCount = 1
	};
	*ppObject = Object;
	return AML_TRUE;
}

//
// Raise the reference counter of an object.
//
VOID
AmlObjectReference(
	_Inout_ AML_OBJECT* Object
	)
{
	//
	// Objects without a set ParentHeap are persistent/managed internally, and are not reference counted.
	//
	if( ( Object == NULL ) || ( Object->ParentHeap == NULL ) ) {
		return;
	}

	//
	// Raise the reference counter of the object.
	//
	if( Object->ReferenceCount >= SIZE_MAX ) {
		AML_TRAP_STRING( "Object reference count overflow." );
	}
	++Object->ReferenceCount;
}


//
// Free internal resources of an object, used internally when the reference counter hits zero.
//
static
VOID
AmlObjectFree(
	_In_ _Frees_ptr_ AML_OBJECT* Object
	)
{
	//
	// Free internal type-specific resources.
	//
	switch( Object->Type ) {
	case AML_OBJECT_TYPE_NAME:
		AmlDataFree( &Object->u.Name.Value );
		break;
	case AML_OBJECT_TYPE_BUFFER_FIELD:
		AmlDataFree( &Object->u.BufferField.SourceBuf );
		break;
	case AML_OBJECT_TYPE_FIELD:
		AmlDataFree( &Object->u.Field.Element.ConnectionResource );
		AmlObjectRelease( Object->u.Field.OperationRegion );
		break;
	case AML_OBJECT_TYPE_BANK_FIELD:
		AmlDataFree( &Object->u.BankField.Base.Element.ConnectionResource );
		AmlObjectRelease( Object->u.BankField.Base.OperationRegion );
		AmlObjectRelease( Object->u.BankField.Bank );
		break;
	case AML_OBJECT_TYPE_INDEX_FIELD:
		AmlDataFree( &Object->u.IndexField.Element.ConnectionResource );
		AmlObjectRelease( Object->u.IndexField.Data );
		AmlObjectRelease( Object->u.IndexField.Index );
		break;
	case AML_OBJECT_TYPE_MUTEX:
		AmlHostMutexFree( Object->u.Mutex.Host, Object->u.Mutex.HostHandle );
		break;
	case AML_OBJECT_TYPE_EVENT:
		AmlHostEventFree( Object->u.Event.Host, Object->u.Event.HostHandle );
		break;
	case AML_OBJECT_TYPE_OPERATION_REGION:
		if( Object->u.OpRegion.IsMapped ) {
			AmlHostMemoryUnmap( Object->u.OpRegion.Host, Object->u.OpRegion.MappedBase, Object->u.OpRegion.Length );
		}
		AmlPciInformationFree( &Object->u.OpRegion.PciInfo );
		break;
	default:
		break;
	}

	//
	// If the object is linked to a namespace node that is somehow still alive,
	// point the namespace node's object to the nil sentinel object just in case.
	//
	if( Object->NamespaceNode != NULL ) {
		Object->NamespaceNode->Object = &Object->NamespaceNode->ParentState->NilObject;
		Object->NamespaceNode = NULL;
	}

	//
	// Free the actual object allocation.
	// TODO: Ensure that the object is not still linked to a namespace node,
	// shouldn't be possible since the namespace node should still hold its reference.
	//
	if( Object->ParentHeap != NULL ) {
		if( Object->NamespaceNode != NULL ) {
			AML_TRAP_STRING( "Object still linked to a namespace node!" );
		}
		AmlHeapFree( Object->ParentHeap, Object );
	}
}

//
// Release object reference, and free if last held reference.
//
VOID
AmlObjectRelease(
	_In_opt_ _Post_invalid_ AML_OBJECT* Object
	)
{
	//
	// Objects without a set ParentHeap are persistent/managed internally, and are not reference counted.
	//
	if( ( Object == NULL ) || ( Object->ParentHeap == NULL ) ) {
		return;
	}

	//
	// Lower the reference counter of the object.
	//
	if( Object->ReferenceCount <= 0 ) {
		AML_TRAP_STRING( "Object reference count underflow." );
	} else if( --Object->ReferenceCount == 0 ) {
		AmlObjectFree( Object );
	}
}


//
// Check if the given object is a field unit object type.
//
BOOLEAN
AmlObjectIsFieldUnit(
	_In_ const AML_OBJECT* Object
	)
{
	switch( Object->Type ) {
	case AML_OBJECT_TYPE_FIELD:
	case AML_OBJECT_TYPE_BANK_FIELD:
	case AML_OBJECT_TYPE_BUFFER_FIELD:
	case AML_OBJECT_TYPE_INDEX_FIELD:
		return AML_TRUE;
	default:
		break;
	}
	return AML_FALSE;
}

//
// Get the length (in bits) of a field unit.
// Returns 0 if this is not a valid field unit type.
//
SIZE_T
AmlFieldUnitLengthBits(
	_In_ const AML_OBJECT* Object
	)
{
	switch( Object->Type ) {
	case AML_OBJECT_TYPE_BUFFER_FIELD:
		return Object->u.BufferField.BitCount;
	case AML_OBJECT_TYPE_FIELD:
		return Object->u.Field.Element.Length;
	case AML_OBJECT_TYPE_INDEX_FIELD:
		return Object->u.IndexField.Element.Length;
	case AML_OBJECT_TYPE_BANK_FIELD:
		return Object->u.BankField.Base.Element.Length;
	default:
		break;
	}
	return 0;
}

//
// Get the maximum access buffer size required for a field.
//
static
SIZE_T
AmlFieldAccessBufferSize(
	_In_ const AML_OBJECT_FIELD* Field
	)
{
	//
	// Handle BufferAcc fields with special semantics.
	// Returns the maximum buffer size required for all types of BufferAcc regions.
	//
	if( Field->Element.AccessType == AML_FIELD_ACCESS_TYPE_BUFFER_ACC ) {
		return sizeof( AML_REGION_ACCESS_DATA );
	}

	//
	// Non-BufferAcc region types use the actual length of the field.
	//
	return ( ( Field->Element.Length + 7 ) / 8 );
}

//
// Get the maximum buffer length (in bytes) required to service any read of the given field unit length.
// Used mainly to accomodate BufferAcc reads. Returns 0 for unsupported object types,
// the return value should always be validated by any functions using this as an output size.
//
SIZE_T
AmlFieldUnitAccessBufferSize(
	_In_ const AML_OBJECT* Object
	)
{
	switch( Object->Type ) {
	case AML_OBJECT_TYPE_BUFFER_FIELD:
		return ( ( AmlFieldUnitLengthBits( Object ) + 7 ) / 8 );
	case AML_OBJECT_TYPE_INDEX_FIELD:
		return ( ( AmlFieldUnitLengthBits( Object ) + 7 ) / 8 );
	case AML_OBJECT_TYPE_FIELD:
		return AmlFieldAccessBufferSize( &Object->u.Field );
	case AML_OBJECT_TYPE_BANK_FIELD:
		return AmlFieldAccessBufferSize( &Object->u.BankField.Base );
	default:
		break;
	}
	return 0;
}

//
// Return the ACPI object type name defined for the Concatenate instruction.
// Does not handle returning the data typenames of named objects.
//
const CHAR*
AmlObjectToAcpiTypeName(
	_In_opt_ const AML_OBJECT* Object
	)
{
	//
	// Handle uninitialized object separate from main case (due to it being possible NULL).
	//
	if( ( Object == NULL ) || ( Object->Type == AML_OBJECT_TYPE_NONE ) ) {
		return "[Uninitialized Object]";
	}

	//
	// Handle internal referenced counted object types.
	// Not all real AML objects are reference counted internal objects.
	//
	switch( Object->Type ) {
	case AML_OBJECT_TYPE_FIELD:
	case AML_OBJECT_TYPE_INDEX_FIELD:
	case AML_OBJECT_TYPE_BANK_FIELD:
		return "[Field]";
	case AML_OBJECT_TYPE_DEVICE:
		return "[Device]";
	case AML_OBJECT_TYPE_EVENT:
		return "[Event]";
	case AML_OBJECT_TYPE_METHOD:
		return "[Control Method]";
	case AML_OBJECT_TYPE_MUTEX:
		return "[Mutex]";
	case AML_OBJECT_TYPE_OPERATION_REGION:
		return "[Operation Region]";
	case AML_OBJECT_TYPE_POWER_RESOURCE:
		return "[Power Resource]";
	case AML_OBJECT_TYPE_PROCESSOR:
		return "[Processor]";
	case AML_OBJECT_TYPE_THERMAL_ZONE:
		return "[Thermal Zone]";
	case AML_OBJECT_TYPE_BUFFER_FIELD:
		return "[Buffer Field]";
	case AML_OBJECT_TYPE_NAME:
		return AmlDataToAcpiTypeName( &Object->u.Name.Value ); /* Handle names specially, using the underlying data type */
	default:
		break;
	}

	return "[Unknown]"; /* Special value for unsupported object types, not spec compliant. */
}

//
// Return the ACPI object type returned by the ObjectType operator.
// No correlation with our internal object type/data type enum values.
// Returns 0/uninitialized on unsupported data type.
//
UINT64
AmlObjectToAcpiObjectType(
	_In_opt_ const AML_OBJECT* Object
	)
{
	//
	// Return uninitialized for invalid object pointer.
	//
	if( Object == NULL ) {
		return 0;
	}

	//
	// Value | Object Type
	// ------+-----------------
	// 0     | Uninitialized
	// 1     | Integer
	// 2     | String
	// 3     | Buffer
	// 4     | Package
	// 5     | Field Unit
	// 6     | Device
	// 7     | Event
	// 8     | Method
	// 9     | Mutex
	// 10    | Operation Region
	// 11    | Power Resource
	// 12    | Reserved
	// 13    | Thermal Zone
	// 14    | Buffer Field
	// 15    | Reserved
	// 16    | Debug Object
	//
	switch( Object->Type ) {
	case AML_OBJECT_TYPE_NAME:
		return AmlDataToAcpiObjectType( &Object->u.Name.Value );
	case AML_OBJECT_TYPE_FIELD:
	case AML_OBJECT_TYPE_INDEX_FIELD:
	case AML_OBJECT_TYPE_BANK_FIELD:
		return 5;
	case AML_OBJECT_TYPE_DEVICE:
		return 6;
	case AML_OBJECT_TYPE_EVENT:
		return 7;
	case AML_OBJECT_TYPE_METHOD:
		return 8;
	case AML_OBJECT_TYPE_MUTEX:
		return 9;
	case AML_OBJECT_TYPE_OPERATION_REGION:
		return 10;
	case AML_OBJECT_TYPE_POWER_RESOURCE:
		return 11;
	case AML_OBJECT_TYPE_THERMAL_ZONE:
		return 13;
	case AML_OBJECT_TYPE_BUFFER_FIELD:
		return 14;
	default:
		break;
	}
	return 0;
}
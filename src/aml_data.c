#include "aml_data.h"
#include "aml_object.h"
#include "aml_heap.h"
#include "aml_debug.h"
#include "aml_platform.h"
#include "aml_base.h"

//
// Raise reference counter of a buffer data resource.
//
VOID
AmlBufferDataReference(
    _Inout_ AML_BUFFER_DATA* Buffer
    )
{
    AmlStateSnapshotItemPushAction( Buffer->StateItem, AML_TRUE );
    Buffer->ReferenceCount++;
}

//
// Release reference to a buffer data resource.
//
VOID
AmlBufferDataRelease(
    _Inout_ _Post_invalid_ AML_BUFFER_DATA* Buffer
    )
{
    AmlStateSnapshotItemPushAction( Buffer->StateItem, AML_FALSE );
    if( Buffer->ReferenceCount-- == 1 ) {
        if( Buffer->StateItem != NULL ) {
            Buffer->StateItem->Valid = 0;
        }
        if( Buffer->Data != NULL ) {
            AmlHeapFree( Buffer->DataHeap, Buffer->Data );
        }
        AmlHeapFree( Buffer->ParentHeap, Buffer );
    }
}

//
// Create a new reference counted buffer data resource.
//
_Success_( return != NULL )
AML_BUFFER_DATA*
AmlBufferDataCreate(
    _Inout_ AML_HEAP* Heap,
    _In_    SIZE_T    Size,
    _In_    SIZE_T    MaxSize
    )
{
    AML_BUFFER_DATA* Buffer;
    UINT8*           Data;
    SIZE_T           i;

    //
    // Force valid size parameters.
    //
    if( Size > MaxSize ) {
        MaxSize = Size;
    }
    
    //
    // Allocate a new buffer resource instance.
    //
    if( ( Buffer = AmlHeapAllocate( Heap, sizeof( *Buffer ) ) ) == NULL ) {
        return NULL;
    } 
    
    //
    // Allocate the backing buffer data.
    // For 0 size, don't try to allocate a backing buffer for the data.
    //
    if( MaxSize != 0 ) {
        if( ( Data = AmlHeapAllocate( Heap, MaxSize ) ) == NULL ) {
            AmlHeapFree( Heap, Buffer );
            return NULL;
        }
    } else {
        Data = NULL;
    }

    //
    // Initialize the new reference counted buffer resource.
    //
    *Buffer = ( AML_BUFFER_DATA ){
        .ParentHeap     = Heap,
        .DataHeap       = Heap,
        .ReferenceCount = 1,
        .Data           = Data,
        .Size           = Size,
        .MaxSize        = MaxSize,
    };

    //
    // Zero-initialize buffer contents by default.
    //
    for( i = 0; i < MaxSize; i++ ) {
        Data[ i ] = 0;
    }

    //
    // Return created buffer resource.
    //
    return Buffer;
}

//
// Create an AML buffer/string of a null-terminated input string.
//
_Success_( return != NULL )
AML_BUFFER_DATA*
AmlBufferDataCreateZ(
    _Inout_ AML_HEAP*   Heap,
    _In_z_  const CHAR* String
    )
{
    SIZE_T           Length;
    AML_BUFFER_DATA* Buffer;
    SIZE_T           i;

    //
    // Calculate null-terminated string length.
    //
    for( Length = 0; Length < ( SIZE_MAX - 1 ); Length++ ) {
        if( String[ Length ] == '\0' ) {
            break;
        }
    }

    //
    // Allocate a new buffer with space for the entire string.
    //
    Buffer = AmlBufferDataCreate( Heap, Length, ( Length + 1 ) );
    if( Buffer == NULL ) {
        return NULL;
    }

    //
    // Copy input string data to the new buffer and null terminate.
    //
    for( i = 0; i < Length; i++ ) {
        Buffer->Data[ i ] = String[ i ];
    }
    Buffer->Data[ Length ] = '\0';
    return Buffer;
}

//
// Resize the internal data allocation capacity of the buffer.
//
_Success_( return )
BOOLEAN
AmlBufferDataResize(
    _Inout_ AML_BUFFER_DATA*  Buffer,
    _Inout_ struct _AML_HEAP* Heap,
    _In_    SIZE_T            NewMaxSize
    )
{
    UINT8* NewData;
    SIZE_T i;

    //
    // Simple uncommon case for resizing a buffer,
    // don't bother reallocating, just lower the buffer size fields.
    //
    if( NewMaxSize <= Buffer->MaxSize ) {
        Buffer->Size = AML_MIN( Buffer->Size, NewMaxSize );
        Buffer->MaxSize = NewMaxSize;
        return AML_TRUE;
    }

    //
    // Buffer needs to be grown, allocate new backing data allocation for the new size.
    //
    if( ( NewData = AmlHeapAllocate( Heap, NewMaxSize ) ) == NULL ) {
        return AML_FALSE;
    }

    //
    // Copy old buffer data contents to the new data allocation.
    //
    for( i = 0; i < Buffer->MaxSize; i++ ) {
        NewData[ i ] = Buffer->Data[ i ];
    }

    //
    // Zero initialize newly allocated space in the buffer.
    //
    for( i = 0; i < ( NewMaxSize - Buffer->MaxSize ); i++ ) {
        NewData[ Buffer->MaxSize + i ] = 0;
    }

    //
    // Free the old buffer data allocation and update the buffer with the resized information.
    //
    if( Buffer->Data != NULL ) {
        AmlHeapFree( Buffer->DataHeap, Buffer->Data );
    }
    Buffer->Data = NewData;
    Buffer->MaxSize = NewMaxSize;
    Buffer->DataHeap = Heap;
    return AML_TRUE;
}

//
// Raise the reference counter of an AML package.
//
VOID
AmlPackageDataReference(
    _Inout_ _Post_invalid_ AML_PACKAGE_DATA* Package
    )
{
    Package->ReferenceCount++;
}

//
// Free all elements and the element array itself.
//
VOID
AmlPackageDataFreeElementsInternal(
    _Inout_ AML_PACKAGE_DATA* Package
    )
{
    UINT64               i;
    AML_PACKAGE_ELEMENT* Element;

    //
    // Free all elements and the element array itself.
    //
    if( Package->Elements != NULL ) {
        for( i = 0; i < Package->ElementCount; i++ ) {
            Element = Package->Elements[ i ];
            if( Element != NULL ) {
                AmlDataFree( &Element->Value );
                AmlHeapFree( Element->ParentHeap, Element );
                Package->Elements[ i ] = NULL;
            }
        }
        AmlHeapFree( Package->ElementArrayHeap, Package->Elements );
        Package->Elements = NULL;
        Package->ElementCount = 0;
        Package->ElementArrayHeap = NULL;
    }
}

//
// Release a reference to an AML package.
//
VOID
AmlPackageDataRelease(
    _Inout_ _Post_invalid_ AML_PACKAGE_DATA* Package
    )
{
    //
    // Decrease the reference counter, only free if we hit zero.
    //
    if( --Package->ReferenceCount != 0 ) {
        return;
    }

    //
    // Free all elements and the element array itself.
    //
    AmlPackageDataFreeElementsInternal( Package );

    //
    // Free the actual package allocation.
    //
    AmlHeapFree( Package->ParentHeap, Package );
}

//
// Validate and lookup the package element entry for the given index.
//
_Success_( return != NULL )
AML_PACKAGE_ELEMENT*
AmlPackageDataLookupElement(
    _In_opt_ AML_PACKAGE_DATA* Package,
    _In_     UINT64            ElementIndex
    )
{
    //
    // Validate input package element against the current state of the referenced package.
    //
    if( Package == NULL ) {
        return NULL;
    } else if( ElementIndex >= Package->ElementCount ) {
        return NULL;
    }
    return Package->Elements[ ElementIndex ];
}

//
// Validate and lookup the package element index data-type.
//
_Success_( return != NULL )
AML_PACKAGE_ELEMENT*
AmlPackageElementIndexDataResolve(
    _In_ const AML_PACKAGE_ELEMENT_INDEX_DATA* Data
    )
{
    return AmlPackageDataLookupElement( Data->Package, Data->ElementIndex );
}

//
// Release any backing allocations for the given AML data object.
//
VOID
AmlDataFree(
    _Inout_ AML_DATA* Data
    )
{
    switch( Data->Type ) {
    case AML_DATA_TYPE_STRING:
        if( Data->u.String != NULL ) {
            AmlBufferDataRelease( Data->u.String );
            Data->u.String = NULL;
        }
        break;
    case AML_DATA_TYPE_BUFFER:
        if( Data->u.Buffer != NULL ) {
            AmlBufferDataRelease( Data->u.Buffer );
            Data->u.Buffer = NULL;
        }
        break;
    case AML_DATA_TYPE_REFERENCE:
        if( Data->u.Reference.Object != NULL ) {
            AmlObjectRelease( Data->u.Reference.Object );
            Data->u.Reference.Object = NULL;
        }
        break;
    case AML_DATA_TYPE_FIELD_UNIT:
        if( Data->u.FieldUnit != NULL ) {
            AmlObjectRelease( Data->u.FieldUnit );
            Data->u.FieldUnit = NULL;
        }
        break;
    case AML_DATA_TYPE_PACKAGE:
        if( Data->u.Package != NULL ) {
            AmlPackageDataRelease( Data->u.Package );
            Data->u.Package = NULL;
        }
        break;
    case AML_DATA_TYPE_PACKAGE_ELEMENT:
        if( Data->u.PackageElement.Package != NULL ) {
            AmlPackageDataRelease( Data->u.PackageElement.Package );
            Data->u.PackageElement.Package = NULL;
        }
        break;
    default:
        break;
    }
    *Data = ( AML_DATA ){ .Type = AML_DATA_TYPE_NONE };
}

//
// Duplicate the given template to the given heap/output.
// Does not deep-copy reference counted types (string/buffer/package),
// simply raises the reference counter to account for the copy.
//
_Success_( return )
BOOLEAN
AmlDataDuplicate(
    _In_    const AML_DATA* TemplateData,
    _Inout_ AML_HEAP*       DuplicateHeap,
    _Out_   AML_DATA*       DuplicateData
    )
{
    AML_DATA Copy;

    //
    // Shallow copy value fields to the copy.
    //
    Copy = *TemplateData;

    //
    // Some data types require deep copying.
    // TODO: !! This is probably not always correct now for buffers/strings! in some cases it should really be deep copying!
    // Need to look into all cases in the codebase where AmlDataDuplicate is used instead of a true deep-copy (if any).
    //
    switch( Copy.Type ) {
    case AML_DATA_TYPE_STRING:
        if( Copy.u.String->Size > Copy.u.String->MaxSize ) {
            return AML_FALSE;
        }
        AmlBufferDataReference( Copy.u.String );
        break;
    case AML_DATA_TYPE_BUFFER:
        if( Copy.u.Buffer->Size > Copy.u.Buffer->MaxSize ) {
            return AML_FALSE;
        }
        AmlBufferDataReference( Copy.u.Buffer );
        break;
    case AML_DATA_TYPE_REFERENCE:
        if( Copy.u.Reference.Object != NULL ) {
            AmlObjectReference( Copy.u.Reference.Object );
        }
        break;
    case AML_DATA_TYPE_FIELD_UNIT:
        if( Copy.u.FieldUnit != NULL ) {
            AmlObjectReference( Copy.u.FieldUnit );
        }
        break;
    case AML_DATA_TYPE_PACKAGE:
        if( Copy.u.Package != NULL ) {
            AmlPackageDataReference( Copy.u.Package );
        }
        break;
    case AML_DATA_TYPE_PACKAGE_ELEMENT:
        if( Copy.u.PackageElement.Package != NULL ) {
            AmlPackageDataReference( Copy.u.PackageElement.Package );
        }
        break;
    default:
        break;
    }

    //
    // Return deep copied duplicate.
    //
    *DuplicateData = Copy;
    return AML_TRUE;
}

//
// Return the ACPI type name for a given internal data object.
//
const CHAR*
AmlDataToAcpiTypeName(
    _In_opt_ const AML_DATA* Value
    )
{
    AML_PACKAGE_ELEMENT* PackageElement;

    //
    // Handle uninitialized data separate from main case (due to it being possible NULL).
    //
    if( ( Value == NULL ) || ( Value->Type == AML_DATA_TYPE_NONE ) ) {
        return "[Uninitialized Object]";
    }

    //
    // Try to convert the underlying data-type to a string.
    // Some of these cases are not defined in the spec, in this case, the values from ACPICA are used.
    //
    switch( Value->Type ) {
    case AML_DATA_TYPE_BUFFER:
        return "[Buffer Object]";
    case AML_DATA_TYPE_INTEGER:
        return "[Integer Object]";
    case AML_DATA_TYPE_STRING:
        return "[String Object]";
    case AML_DATA_TYPE_PACKAGE:
    case AML_DATA_TYPE_VAR_PACKAGE:
        return "[Package Object]";
    case AML_DATA_TYPE_REFERENCE:
        return "[Reference Object]";
    case AML_DATA_TYPE_DEBUG:
        return "[Debug Object]";
    case AML_DATA_TYPE_FIELD_UNIT:
        return AmlObjectToAcpiTypeName( Value->u.FieldUnit );
    case AML_DATA_TYPE_PACKAGE_ELEMENT:
        if( ( PackageElement = AmlPackageElementIndexDataResolve( &Value->u.PackageElement ) ) == NULL ) {
            return "[Uninitialized Object]";
        }
        return AmlDataToAcpiTypeName( &PackageElement->Value );
    default:
        break;
    }

    //
    // Unsupported data type.
    // This value is not spec compliant.
    //
    return "[Unknown]";
}

//
// Return the ACPI object type returned by the ObjectType operator.
// No correlation with our internal object type/data type enum values.
// Returns 0/uninitialized on unsupported data type.
//
UINT64
AmlDataToAcpiObjectType(
    _In_ const AML_DATA* Data
    )
{
    AML_PACKAGE_ELEMENT* Element;

    //
    // Value | Object Type
    // ------+-----------------
    // 0     | Uninitialized
    // 1     | Integer
    // 2     | String
    // 3     | Buffer
    // 4     | Package
    // 5     | Field Unit
    // 14    | Buffer Field
    //
    switch( Data->Type ) {
    case AML_DATA_TYPE_INTEGER:
        return 1;
    case AML_DATA_TYPE_STRING:
        return 2;
    case AML_DATA_TYPE_BUFFER:
        return 3;
    case AML_DATA_TYPE_PACKAGE:
        return 4;
    case AML_DATA_TYPE_FIELD_UNIT:
        if( Data->u.FieldUnit == NULL ) {
            return 0;
        } else if( Data->u.FieldUnit->Type == AML_OBJECT_TYPE_BUFFER_FIELD ) {
            return 14;
        }
        return 5;
    case AML_DATA_TYPE_PACKAGE_ELEMENT:
        Element = AmlPackageDataLookupElement( Data->u.PackageElement.Package, Data->u.PackageElement.ElementIndex );
        if( Element == NULL ) {
            return 0;
        }
        return AmlDataToAcpiObjectType( &Element->Value );
    case AML_DATA_TYPE_REFERENCE:
        return AmlObjectToAcpiObjectType( Data->u.Reference.Object );
    default:
        break;
    }
    return 0;
}

//
// Calculate the size of the given input for evaluation of the SizeOf opcode.
//
UINT64
AmlDataSizeOf(
    _In_ const AML_DATA* TargetValue
    )
{
    AML_PACKAGE_ELEMENT* Element;

    //
    // Package elements must be treated transparently, follow them to their underlying value.
    //
    if( TargetValue->Type == AML_DATA_TYPE_PACKAGE_ELEMENT ) {
        Element = AmlPackageElementIndexDataResolve( &TargetValue->u.PackageElement );
        if( Element != NULL ) {
            TargetValue = &Element->Value;
        }
    }

    //
    // For a buffer, it returns the size in bytes of the data
    // For a string, it returns the size in bytes of the string, not counting the trailing NULL.
    // For a package, it returns the number of elements.
    // For an object reference, the size of the referenced object is returned.
    // Other data types cause a fatal run-time error (not fatal in ACPICA, error message and return 0).
    //
    switch( TargetValue->Type ) {
    case AML_DATA_TYPE_BUFFER:
        return ( ( TargetValue->u.Buffer != NULL ) ? TargetValue->u.Buffer->Size : 0 );
    case AML_DATA_TYPE_STRING:
        return ( ( TargetValue->u.String != NULL ) ? TargetValue->u.String->Size : 0 );
    case AML_DATA_TYPE_PACKAGE:
        return ( ( TargetValue->u.Package != NULL ) ? TargetValue->u.Package->ElementCount : 0 );
    case AML_DATA_TYPE_REFERENCE:
        if( ( TargetValue->u.Reference.Object == NULL )
            || ( TargetValue->u.Reference.Object->Type != AML_OBJECT_TYPE_NAME )
            || ( TargetValue->u.Reference.Object->u.Name.Value.Type == AML_DATA_TYPE_REFERENCE ) )
        {
            // AML_DEBUG_ERROR( State, "Error: Invalid reference destination in SizeOf.\n" );
            break;
        }
        return AmlDataSizeOf( &TargetValue->u.Reference.Object->u.Name.Value );
    default:
        // AML_DEBUG_ERROR( State, "Error: Unsupported SizeOf data type.\n" );
        break;
    }

    return 0;
}

//
// Compare a string EISAID or integer encoded EISAID with the given integer encoded EISAID value.
//
_Success_( return )
BOOLEAN
AmlDataCompareEisaId(
    _In_ const AML_DATA* Data,
    _In_ UINT32          EisaId
    )
{
    if( Data->Type == AML_DATA_TYPE_INTEGER ) {
        return ( EisaId == ( Data->u.Integer & ~0ul ) );
    } else if( Data->Type == AML_DATA_TYPE_STRING ) {
        return ( EisaId == AmlStringToEisaId( ( const CHAR* )Data->u.String->Data, ( Data->u.String->Size / sizeof( CHAR ) ) ) );
    }
    return AML_FALSE;
}
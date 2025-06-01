#pragma once

#include "aml_platform.h"
#include "aml_heap.h"

//
// The maximum amount of prefixes allowed in a name string.
//
#ifndef AML_NAME_MAX_PREFIX_COUNT
#define AML_NAME_MAX_PREFIX_COUNT 16
#endif

//
// NameSeg :=
//   <leadnamechar namechar namechar namechar>
// NameSegs shorter than 4 characters are filled with trailing underscores ('_'s).
//
#pragma pack(push, 1)
typedef struct _AML_NAME_SEG {
	union {
		UINT8  Data[ 4 ];
		UINT32 AsUInt32;
	};
} AML_NAME_SEG;
#pragma pack(pop)

//
// A string of AML name prefixes, either '\' or a sequence of '^' characters.
//
typedef struct _AML_NAME_PREFIX {
	UINT8  Data[ AML_NAME_MAX_PREFIX_COUNT + 1 ];
	SIZE_T Length;
} AML_NAME_PREFIX;

//
// A (possibly) prefixed AML name string formed out of name segments.
//
typedef struct _AML_NAME_STRING {
	AML_NAME_PREFIX     Prefix;
	const AML_NAME_SEG* Segments; /* TODO: Add opaque vs input stream name string/segments? */
	SIZE_T              SegmentCount;
} AML_NAME_STRING;

//
// SimpleName := NameString | ArgObj | LocalObj
//
typedef enum _AML_SIMPLE_NAME_TYPE {
	AML_SIMPLE_NAME_TYPE_NONE = 0,
	AML_SIMPLE_NAME_TYPE_STRING,
	AML_SIMPLE_NAME_TYPE_ARG_OBJ,
	AML_SIMPLE_NAME_TYPE_LOCAL_OBJ,
} AML_SIMPLE_NAME_TYPE;

//
// SimpleName := NameString | ArgObj | LocalObj
//
typedef struct _AML_SIMPLE_NAME {
	AML_SIMPLE_NAME_TYPE Type;
	union {
		AML_NAME_STRING NameString;
		UINT8           ArgObj;
		UINT8           LocalObj;
	} u;
} AML_SIMPLE_NAME;

//
// SuperName := SimpleName | DebugObj | ReferenceTypeOpcode
//
typedef enum _AML_SUPER_NAME_TYPE {
	AML_SUPER_NAME_TYPE_NONE = 0,
	AML_SUPER_NAME_TYPE_SIMPLE,
	AML_SUPER_NAME_TYPE_DEBUG_OBJ,
	AML_SUPER_NAME_TYPE_REFERENCE_TYPE_OPCODE,
} AML_SUPER_NAME_TYPE;

//
// Value data types.
//
typedef enum _AML_DATA_TYPE {
	AML_DATA_TYPE_NONE = 0,
	AML_DATA_TYPE_INTEGER,
	AML_DATA_TYPE_STRING,
	AML_DATA_TYPE_BUFFER,
	AML_DATA_TYPE_PACKAGE,
	AML_DATA_TYPE_VAR_PACKAGE, /* TODO: Remove, used internally for opaque data. */
	AML_DATA_TYPE_REFERENCE,
	AML_DATA_TYPE_FIELD_UNIT,
	AML_DATA_TYPE_PACKAGE_ELEMENT,
	AML_DATA_TYPE_DEBUG
} AML_DATA_TYPE;

_Static_assert( AML_DATA_TYPE_NONE == 0, "AML_DATA_TYPE_NONE value must remain 0, used internally." );

//
// AML package created by DefPackage/DefVarPackage.
//
typedef struct _AML_PACKAGE_DATA {
	struct _AML_HEAP*             ParentHeap;
	struct _AML_HEAP*             ElementArrayHeap;
	SIZE_T                        ReferenceCount;
	UINT64                        ElementCount;
	struct _AML_PACKAGE_ELEMENT** Elements;
} AML_PACKAGE_DATA;

//
// AML buffer resource.
//
typedef struct _AML_BUFFER_DATA {
	struct _AML_HEAP*                ParentHeap;
	struct _AML_HEAP*                DataHeap;
	struct _AML_STATE_SNAPSHOT_ITEM* StateItem;
	UINT8*                           Data;
	SIZE_T                           ReferenceCount;
	SIZE_T                           Size;
	SIZE_T                           MaxSize;
} AML_BUFFER_DATA;

//
// AML reference data value.
//
typedef struct _AML_REFERENCE {
	struct _AML_OBJECT* Object;
} AML_REFERENCE;

//
// Indexed reference to a package element.
//
typedef struct _AML_PACKAGE_ELEMENT_INDEX_DATA {
	AML_PACKAGE_DATA* Package;
	UINT64            ElementIndex;
} AML_PACKAGE_ELEMENT_INDEX_DATA;

//
// Fully evaluated AML data value.
//
typedef struct _AML_DATA {
	AML_DATA_TYPE Type;
	union {
		UINT64                         Integer;
		CHAR                           IntegerRaw[ 8 ];
		AML_BUFFER_DATA*               String;
		AML_PACKAGE_DATA*              Package;
		AML_BUFFER_DATA*               Buffer;
		AML_REFERENCE                  Reference;
		struct _AML_OBJECT*            FieldUnit;
		AML_PACKAGE_ELEMENT_INDEX_DATA PackageElement;
	} u;
} AML_DATA;

//
// AML package element created by DefPackage/DefVarPackage.
//
typedef struct _AML_PACKAGE_ELEMENT {
	struct _AML_HEAP* ParentHeap;
	AML_DATA          Value;
} AML_PACKAGE_ELEMENT;

//
// Create a new reference counted buffer data resource.
//
_Success_( return != NULL )
AML_BUFFER_DATA*
AmlBufferDataCreate(
	_Inout_ AML_HEAP* Heap,
	_In_    SIZE_T    Size,
	_In_    SIZE_T    MaxSize
	);

//
// Create an AML buffer/string of a null-terminated input string.
//
_Success_( return != NULL )
AML_BUFFER_DATA*
AmlBufferDataCreateZ(
	_Inout_ AML_HEAP*   Heap,
	_In_z_  const CHAR* String
	);

//
// Resize the internal data allocation capacity of the buffer.
//
_Success_( return )
BOOLEAN
AmlBufferDataResize(
	_Inout_ AML_BUFFER_DATA*  Buffer,
	_Inout_ struct _AML_HEAP* Heap,
	_In_    SIZE_T            NewMaxSize
	);

//
// Raise reference counter of a buffer data resource.
//
VOID
AmlBufferDataReference(
	_Inout_ AML_BUFFER_DATA* Buffer
	);

//
// Release reference to a buffer data resource.
//
VOID
AmlBufferDataRelease(
	_Inout_ _Post_invalid_ AML_BUFFER_DATA* Buffer
	);

//
// Raise the reference counter of an AML package.
//
VOID
AmlPackageDataReference(
	_Inout_ _Post_invalid_ AML_PACKAGE_DATA* Package
	);

//
// Free all elements and the element array itself.
//
VOID
AmlPackageDataFreeElementsInternal(
	_Inout_ AML_PACKAGE_DATA* Package
	);

//
// Release a reference to an AML package.
//
VOID
AmlPackageDataRelease(
	_Inout_ _Post_invalid_ AML_PACKAGE_DATA* Package
	);

//
// Validate and lookup the package element entry for the given index.
//
_Success_( return != NULL )
AML_PACKAGE_ELEMENT*
AmlPackageDataLookupElement(
	_In_opt_ AML_PACKAGE_DATA* Package,
	_In_     UINT64            ElementIndex
	);

//
// Validate and lookup the package element index data-type.
//
_Success_( return != NULL )
AML_PACKAGE_ELEMENT*
AmlPackageElementIndexDataResolve(
	_In_ const AML_PACKAGE_ELEMENT_INDEX_DATA* Data
	);

//
// Release any backing allocations for the given AML data object.
//
VOID
AmlDataFree(
	_Inout_ AML_DATA* DataObj
	);

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
	);

//
// Return the ACPI type name for a given internal data object.
//
const CHAR*
AmlDataToAcpiTypeName(
	_In_opt_ const AML_DATA* Value
	);

//
// Return the ACPI object type returned by the ObjectType operator.
// No correlation with our internal object type/data type enum values.
// Returns 0/uninitialized on unsupported data type.
//
UINT64
AmlDataToAcpiObjectType(
	_In_ const AML_DATA* Data
	);

//
// Calculate the size of the given input for evaluation of the SizeOf opcode.
//
UINT64
AmlDataSizeOf(
	_In_ const AML_DATA* TargetValue
	);

//
// Compare a string EISAID or integer encoded EISAID with the given integer encoded EISAID value.
//
_Success_( return )
BOOLEAN
AmlDataCompareEisaId(
	_In_ const AML_DATA* Data,
	_In_ UINT32          EisaId
	);
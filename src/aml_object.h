#pragma once

#include "aml_platform.h"
#include "aml_data.h"
#include "aml_pci_info.h"
#include "aml_state_snapshot.h"
#include "aml_object_type.h"

//
// Object created by DefAlias.
// Forwards accesses of "Name" to the destination named object name.
//
typedef struct _AML_OBJECT_ALIAS {
    AML_NAME_STRING Name;
    AML_NAME_STRING DestinationName;
} AML_OBJECT_ALIAS;

//
// Object created by DefName.
//
typedef struct _AML_OBJECT_NAME {
    AML_NAME_STRING String;
    AML_DATA        Value;
} AML_OBJECT_NAME;

//
// Object created by DefScope.
//
typedef struct _AML_OBJECT_SCOPE {
    AML_NAME_STRING Location;
} AML_OBJECT_SCOPE;

//
// Object created by DefDevice.
//
typedef struct _AML_OBJECT_DEVICE {
    AML_NAME_STRING Name;
} AML_OBJECT_DEVICE;

//
// Per-method-scope mutex state.
//
typedef struct _AML_MUTEX_SCOPE_LEVEL {
    struct _AML_MUTEX_SCOPE_LEVEL* PreviousLevel; /* Links to the previous level in the per-scope stack for this mutex. */
    struct _AML_OBJECT*            Object;
    SIZE_T                         AcquireCount;
    struct _AML_METHOD_SCOPE*      MethodScope;
    struct _AML_MUTEX_SCOPE_LEVEL* NextSibling; /* Links to the next sibling in the current method scope. */
} AML_MUTEX_SCOPE_LEVEL;

//
// Object created by DefMutex.
//
typedef struct _AML_OBJECT_MUTEX {
    struct _AML_HOST_CONTEXT* Host;
    UINT64                    HostHandle;
    AML_MUTEX_SCOPE_LEVEL*    ScopeEntry;
    UINT8                     SyncLevel;
} AML_OBJECT_MUTEX;

//
// MethodFlags used by DefMethod.
// bit 0-2: ArgCount (0-7)
// bit 3: SerializeFlag
// 0 NotSerialized
// 1 Serialized
// bit 4-7: SyncLevel (0x00-0x0f)
// Note: Masks are unshifted, and begin at bit 0.
//
#define AML_METHOD_FLAGS_ARG_COUNT_SHIFT 0
#define AML_METHOD_FLAGS_ARG_COUNT_MASK  ( ( 1 << ( ( 2 - 0 ) + 1 ) ) - 1 )
#define AML_METHOD_FLAGS_ARG_COUNT(MethodFlags) \
    ( ( (MethodFlags) >> AML_METHOD_FLAGS_ARG_COUNT_SHIFT ) & AML_METHOD_FLAGS_ARG_COUNT_MASK )
#define AML_METHOD_FLAGS_SERIALIZE_SHIFT 3
#define AML_METHOD_FLAGS_SERIALIZE_MASK  1
#define AML_METHOD_FLAGS_SERIALIZE(MethodFlags) \
    ( ( (MethodFlags) >> AML_METHOD_FLAGS_SERIALIZE_SHIFT ) & AML_METHOD_FLAGS_SERIALIZE_MASK )
#define AML_METHOD_FLAGS_SYNC_LEVEL_SHIFT 4
#define AML_METHOD_FLAGS_SYNC_LEVEL_MASK  ( ( 1 << ( ( 7 - 4 ) + 1 ) ) - 1 )
#define AML_METHOD_FLAGS_SYNC_LEVEL(MethodFlags) \
    ( ( (MethodFlags) >> AML_METHOD_FLAGS_SYNC_LEVEL_SHIFT ) & AML_METHOD_FLAGS_SYNC_LEVEL_MASK )

//
// Native method callback, allows native code to implement a method callable by AML.
//
typedef
_Success_( return )
BOOLEAN
( *AML_METHOD_USER_ROUTINE )(
    _Inout_                        struct _AML_STATE* State,
    _In_                           VOID*              UserContext,
    _Inout_count_( ArgumentCount ) AML_DATA*          Arguments,
    _In_                           SIZE_T             ArgumentCount,
    _Inout_                        AML_DATA*          ReturnValue
    );

//
// Method created by DefMethod (or by native code).
//
typedef struct _AML_OBJECT_METHOD {
    AML_NAME_STRING         Name;
    const VOID*             CodeDataBlock;
    SIZE_T                  CodeDataBlockSize;
    SIZE_T                  CodeStart;
    SIZE_T                  CodeSize;
    AML_METHOD_USER_ROUTINE UserRoutine;
    VOID*                   UserContext;
    UINT8                   ArgumentCount : 3;
    UINT8                   IsSerialized : 1;
    UINT8                   SyncLevel : 4;
} AML_OBJECT_METHOD;

//
// ACPI region space types.
//
#define AML_REGION_SPACE_TYPE_SYSTEM_MEMORY      ( ( UINT8 )0x00 )
#define AML_REGION_SPACE_TYPE_SYSTEM_IO          ( ( UINT8 )0x01 )
#define AML_REGION_SPACE_TYPE_PCI_CONFIG         ( ( UINT8 )0x02 )
#define AML_REGION_SPACE_TYPE_EMBEDDED_CONTROL   ( ( UINT8 )0x03 )
#define AML_REGION_SPACE_TYPE_SMBUS              ( ( UINT8 )0x04 )
#define AML_REGION_SPACE_TYPE_SYSTEM_CMOS        ( ( UINT8 )0x05 )
#define AML_REGION_SPACE_TYPE_PCI_BAR_TARGET     ( ( UINT8 )0x06 )
#define AML_REGION_SPACE_TYPE_IPMI               ( ( UINT8 )0x07 )
#define AML_REGION_SPACE_TYPE_GENERAL_PURPOSE_IO ( ( UINT8 )0x08 )
#define AML_REGION_SPACE_TYPE_GENERIC_SERIAL_BUS ( ( UINT8 )0x09 )
#define AML_REGION_SPACE_TYPE_PCC                ( ( UINT8 )0x0A )
#define AML_REGION_SPACE_TYPE_PLATFORM_RT        ( ( UINT8 )0x0B )
#define AML_MAX_SPEC_REGION_SPACE_TYPE_COUNT     ( AML_REGION_SPACE_TYPE_PLATFORM_RT + 1 )
#define AML_REGION_SPACE_OEM_START               ( ( UINT8 )0x80 )

//
// Operation region object.
//
typedef struct _AML_OBJECT_OPERATION_REGION {
    struct _AML_HOST_CONTEXT* Host;
    AML_NAME_STRING           Name;
    VOID*                     UserContext;
    UINT64                    Offset;         /* Offset in bytes. */
    UINT64                    Length;         /* Length in bytes. */
    VOID*                     MappedBase;     /* For memory-mapped space types. */
    AML_PCI_INFORMATION       PciInfo;        /* For PCI space types (CFG/BAR). */
    UINT8                     SpaceType;      /* AML_REGION_SPACE_TYPE */
    BOOLEAN                   IsMapped : 1;	  /* For memory-mapped and PCI (CFG/BAR) space types. */
} AML_OBJECT_OPERATION_REGION;

//
// FieldFlags AccessType (bit 0-3) enum values.
//
#define AML_FIELD_ACCESS_TYPE_ANY_ACC    0
#define AML_FIELD_ACCESS_TYPE_BYTE_ACC   1
#define AML_FIELD_ACCESS_TYPE_WORD_ACC   2
#define AML_FIELD_ACCESS_TYPE_DWORD_ACC  3
#define AML_FIELD_ACCESS_TYPE_QWORD_ACC  4
#define AML_FIELD_ACCESS_TYPE_BUFFER_ACC 5

//
// FieldFlags UpdateRule (bit 5-6) enum values. 
//
#define AML_FIELD_UPDATE_RULE_PRESERVE       0
#define AML_FIELD_UPDATE_RULE_WRITE_AS_ONES  1
#define AML_FIELD_UPDATE_RULE_WRITE_AS_ZEROS 2

//
// FieldFlags bitfield definitions.
// Note: The masks here are unshifted, beginning at bit 0.
//
#define AML_FIELD_FLAGS_ACCESS_TYPE_SHIFT 0 /* Bit 0-3: AccessType (AML_FIELD_ACCESS_TYPE_) */
#define AML_FIELD_FLAGS_ACCESS_TYPE_MASK  ( ( 1 << ( ( 3 - 0 ) + 1 ) ) - 1 )
#define AML_FIELD_FLAGS_ACCESS_TYPE_GET(FieldFlags) \
    ( ( (FieldFlags) >> AML_FIELD_FLAGS_ACCESS_TYPE_SHIFT ) & AML_FIELD_FLAGS_ACCESS_TYPE_MASK )
#define AML_FIELD_FLAGS_LOCK_RULE_SHIFT 4 /* Bit 4: LockRule (0: NoLock, 1: Lock) */
#define AML_FIELD_FLAGS_LOCK_RULE_MASK  1
#define AML_FIELD_FLAGS_LOCK_RULE_GET(FieldFlags) \
    ( ( (FieldFlags) >> AML_FIELD_FLAGS_LOCK_RULE_SHIFT ) & AML_FIELD_FLAGS_LOCK_RULE_MASK )
#define AML_FIELD_FLAGS_UPDATE_RULE_SHIFT 5 /* Bit 5-6: UpdateRule */
#define AML_FIELD_FLAGS_UPDATE_RULE_MASK ( ( 1 << ( ( 6 - 5 ) + 1 ) ) - 1 )
#define AML_FIELD_FLAGS_UPDATE_RULE_GET(FieldFlags) \
    ( ( (FieldFlags) >> AML_FIELD_FLAGS_UPDATE_RULE_SHIFT ) & AML_FIELD_FLAGS_UPDATE_RULE_MASK )
#define AML_FIELD_FLAGS_RESERVED_SHIFT 7 /* Bit 7: Reserved (must be 0) */
#define AML_FIELD_FLAGS_RESERVED_MASK  1
#define AML_FIELD_FLAGS_RESERVED_GET(FieldFlags) \
    ( ( (FieldFlags) >> AML_FIELD_FLAGS_RESERVED_SHIFT ) & AML_FIELD_FLAGS_RESERVED_MASK )

//
// AML field AccessType bitfield definitions.
// Note: The masks here are unshifted, beginning at bit 0.
//
#define AML_FIELD_ACCESS_TYPE_ACCESS_ATTRIB_SHIFT 6 /* Bits 7:6 - 0 = AccessAttrib */
#define AML_FIELD_ACCESS_TYPE_ACCESS_ATTRIB_MASK ( ( 1 << ( ( 7 - 6 ) + 1 ) ) - 1 )
#define AML_FIELD_ACCESS_TYPE_ACCESS_ATTRIB_GET(AccessType) \
    ( ( (FieldFlags) >> AML_FIELD_ACCESS_TYPE_ACCESS_ATTRIB_SHIFT ) & AML_FIELD_ACCESS_TYPE_ACCESS_ATTRIB_MASK )

//
// AML field AccessType AccessAttribType values.
// These are the values held by bits 7:6 of the AccessType byte.
//
#define AML_FIELD_ACCESS_ATTRIB_TYPE_NORMAL            0
#define AML_FIELD_ACCESS_ATTRIB_TYPE_BYTES             1
#define AML_FIELD_ACCESS_ATTRIB_TYPE_RAW_BYTES         2
#define AML_FIELD_ACCESS_ATTRIB_TYPE_RAW_PROCESS_BYTES 3

//
// AML field AccessAttrib values, values of the actual AccessAttrib byte.
//
#define AML_FIELD_ACCESS_ATTRIB_NONE               ( ( UINT8 )0x00 )
#define AML_FIELD_ACCESS_ATTRIB_QUICK              ( ( UINT8 )0x02 )
#define AML_FIELD_ACCESS_ATTRIB_SEND_RECEIVE       ( ( UINT8 )0x04 )
#define AML_FIELD_ACCESS_ATTRIB_BYTE               ( ( UINT8 )0x06 )
#define AML_FIELD_ACCESS_ATTRIB_WORD               ( ( UINT8 )0x08 )
#define AML_FIELD_ACCESS_ATTRIB_BLOCK              ( ( UINT8 )0x0A )
#define AML_FIELD_ACCESS_ATTRIB_PROCESS_CALL       ( ( UINT8 )0x0C )
#define AML_FIELD_ACCESS_ATTRIB_BLOCK_PROCESS_CALL ( ( UINT8 )0x0D )

//
// AML field list element type.
//
#define AML_FIELD_ELEMENT_TYPE_RESERVED        0x00
#define AML_FIELD_ELEMENT_TYPE_ACCESS          0x01
#define AML_FIELD_ELEMENT_TYPE_CONNECT         0x02
#define AML_FIELD_ELEMENT_TYPE_EXTENDED_ACCESS 0x03
#define AML_FIELD_ELEMENT_TYPE_NAMED           0xFF

//
// AML field element access attributes.
//
#define AML_ACCESS_ATTRIBUTE_QUICK              0x02
#define AML_ACCESS_ATTRIBUTE_SEND_RECEIVE       0x04
#define AML_ACCESS_ATTRIBUTE_BYTE               0x06
#define AML_ACCESS_ATTRIBUTE_WORD               0x08
#define AML_ACCESS_ATTRIBUTE_BLOCK              0x0A
#define AML_ACCESS_ATTRIBUTE_PROCESS_CALL       0x0C
#define AML_ACCESS_ATTRIBUTE_BLOCK_PROCESS_CALL 0x0D

//
// Decoded AML field list element.
// 
typedef struct _AML_FIELD_ELEMENT {
    UINT8           Type;
    UINT8           AccessType;
    UINT8           AccessAttributes;   /* AML_ACCESS_ATTRIBUTE */
    UINT8           AccessByteLength;   /* Only used for serial/buffer protocols. */
    BOOLEAN         IsConnection;       /* Is connected to a GPIO or SBC region/descriptor. */
    AML_DATA        ConnectionResource;
    UINT64          Length;             /* Bit-length of the field. */
    AML_NAME_STRING Name;               /* If (Type == AML_FIELD_ELEMENT_TYPE_NAMED). */
} AML_FIELD_ELEMENT;

//
// Object created by (DefCreateBitField | DefCreateByteField | DefCreateWordField | DefCreateDWordField | DefCreateQWordField | DefCreateField).
//
typedef struct _AML_OBJECT_BUFFER_FIELD {
    UINT64           BitIndex; // TermArg => Integer
    UINT64           BitCount; // TermArg => Integer
    AML_NAME_STRING  Name;     // NameString
    AML_DATA         SourceBuf;
} AML_OBJECT_BUFFER_FIELD;

//
// Object created by DefField.
//
typedef struct _AML_OBJECT_FIELD {
    AML_FIELD_ELEMENT   Element;
    UINT8               Flags; // AML_FIELD_FLAGS
    UINT64              Offset;
    struct _AML_OBJECT* OperationRegion;
} AML_OBJECT_FIELD;

//
// Object created by DefBankField.
//
typedef struct _AML_OBJECT_BANK_FIELD {
    AML_OBJECT_FIELD    Base;
    struct _AML_OBJECT* Bank;
    AML_DATA            BankValue;
} AML_OBJECT_BANK_FIELD;

//
// Object created by DefIndexField.
//
typedef struct _AML_OBJECT_INDEX_FIELD {
    AML_FIELD_ELEMENT   Element;
    UINT8               Flags; // AML_FIELD_FLAGS
    UINT64              Offset;
    struct _AML_OBJECT* Index;
    struct _AML_OBJECT* Data;
} AML_OBJECT_INDEX_FIELD;

//
// Data region object.
//
typedef struct _AML_OBJECT_DATA_REGION {
    AML_NAME_STRING Name;
    AML_DATA        SignatureString;
    AML_DATA        OemIDString;
    AML_DATA        OemTableIDString;
} AML_OBJECT_DATA_REGION;

//
// Event object.
//
typedef struct _AML_OBJECT_EVENT {
    struct _AML_HOST_CONTEXT* Host;
    UINT64                    HostHandle;
    volatile ULONG            State;
} AML_OBJECT_EVENT;

//
// Deprecated processor object defined by DefProcessor.
//
typedef struct _AML_OBJECT_PROCESSOR {
    UINT32 PBLKAddress;
    UINT32 PBLKLength;
    UINT8  ID;
} AML_OBJECT_PROCESSOR;

//
// Power resource object.
//
typedef struct _AML_OBJECT_POWER_RESOURCE {
    UINT8  SystemLevel;
    UINT16 ResourceOrder;
} AML_OBJECT_POWER_RESOURCE;

//
// Object supertype with special semantics (Normal, ArgObj, LocalObj, DebugObj).
//
typedef enum _AML_OBJECT_SUPERTYPE {
    AML_OBJECT_SUPERTYPE_NONE = 0,
    AML_OBJECT_SUPERTYPE_ARG,
    AML_OBJECT_SUPERTYPE_LOCAL,
    AML_OBJECT_SUPERTYPE_DEBUG
} AML_OBJECT_SUPERTYPE;

//
// Object := NameSpaceModifierObj | NamedObj
//
typedef struct _AML_OBJECT {
    //
    // Memory management metadata.
    //
    struct _AML_HEAP* ParentHeap;
    SIZE_T            ReferenceCount;

    //
    // Link to a parent namespace node for this object (if any).
    //
    struct _AML_NAMESPACE_NODE* NamespaceNode;

    //
    // Super-type semantics for the object (None, LocalObj, ArgObj, DebugObj).
    //
    AML_OBJECT_SUPERTYPE SuperType;
    SIZE_T               SuperIndex; /* Local[X], Arg[X] */

    //
    // Underlying object union type tag.
    //
    AML_OBJECT_TYPE Type;

    //
    // Indicates if the _REG handler child of this object has already been called for each address space type.
    //
    UINT32 RegCallBitmap;

    //
    // Indicates if this object was already processed in a device initialization pass.
    //
    BOOLEAN IsInitializedDevice : 1;

    //
    // Underlying object type structures.
    //
    union {
        AML_OBJECT_FIELD            Field;
        AML_OBJECT_BUFFER_FIELD     BufferField;
        AML_OBJECT_BANK_FIELD       BankField;
        AML_OBJECT_INDEX_FIELD      IndexField;
        AML_OBJECT_ALIAS            Alias;
        AML_OBJECT_NAME             Name;
        AML_OBJECT_SCOPE            Scope;
        AML_OBJECT_DEVICE           Device;
        AML_OBJECT_METHOD           Method;
        AML_OBJECT_OPERATION_REGION OpRegion;
        AML_OBJECT_DATA_REGION      DataRegion;
        AML_OBJECT_EVENT            Event;
        AML_OBJECT_MUTEX            Mutex;
        AML_OBJECT_PROCESSOR        Processor;
        AML_OBJECT_POWER_RESOURCE   PowerResource;
    } u;
} AML_OBJECT;

//
// Allocate and default-initialize a reference counted AML object.
//
_Success_( return )
BOOLEAN
AmlObjectCreate(
    _Inout_  struct _AML_HEAP* Heap,
    _In_     AML_OBJECT_TYPE   Type,
    _Outptr_ AML_OBJECT**      ppObject
    );

//
// Raise the reference counter of an object.
//
VOID
AmlObjectReference(
    _Inout_ AML_OBJECT* Object
    );

//
// Release object reference, and free if last held reference.
//
VOID
AmlObjectRelease(
    _In_opt_ _Post_invalid_ AML_OBJECT* Object
    );

//
// Check if the given object is a field unit object type.
//
BOOLEAN
AmlObjectIsFieldUnit(
    _In_ const AML_OBJECT* Object
    );

//
// Get the length (in bits) of a field unit.
// Returns 0 if this is not a valid field unit type.
//
SIZE_T
AmlFieldUnitLengthBits(
    _In_ const AML_OBJECT* Object
    );

//
// Get the maximum buffer length (in bytes) required to service any read of the given field unit length.
// Used mainly to accomodate BufferAcc reads. Returns 0 for unsupported object types,
// the return value should always be validated by any functions using this as an output size.
//
SIZE_T
AmlFieldUnitAccessBufferSize(
    _In_ const AML_OBJECT* Object
    );

//
// Return the ACPI object type name defined for the Concatenate instruction.
// Does not handle returning the data typenames of named objects.
//
const CHAR*
AmlObjectToAcpiTypeName(
    _In_opt_ const AML_OBJECT* Object
    );

//
// Return the ACPI object type returned by the ObjectType operator.
// No correlation with our internal object type/data type enum values.
// Returns 0/uninitialized on unsupported data type.
//
UINT64
AmlObjectToAcpiObjectType(
    _In_opt_ const AML_OBJECT* Object
    );
#pragma once

#include "aml_platform.h"
#include "aml_decoder.h"

//
// Default string used for _OS operating system name value.
//
#ifndef AML_OS_NAME
 #define AML_OS_NAME "Microsoft Windows NT"
#endif

//
// Special semantics for TermArg evaluation.
//
#define AML_EVAL_TERM_ARG_FLAG_NONE       0
#define AML_EVAL_TERM_ARG_FLAG_IS_DEREFOF (1 << 0)
#define AML_EVAL_TERM_ARG_FLAG_TEMP       (1 << 1)

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
    );

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
    );

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
    );

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
    );

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
    );

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
    );

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
    );

//
// Evaluate a DataObject.
// DataObject := ComputationalData | DefPackage | DefVarPackage
//
_Success_( return )
BOOLEAN
AmlEvalDataObject(
    _Inout_ AML_STATE* State,
    _Out_   AML_DATA*  Data,
    _In_    BOOLEAN    IsTemporary
    );

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
    );

//
// Evaluate a RefOf instruction to an object.
//
_Success_( return )
BOOLEAN
AmlEvalRefOf(
    _Inout_  AML_STATE*   State,
    _In_     BOOLEAN      ConsumeOpcode,
    _Outptr_ AML_OBJECT** ppObject
    );

//
// Evaluate DerefOf instruction to an object.
// DefDerefOf := DerefOfOp ObjReference
// ObjReference := TermArg => ObjectReference | String
//
_Success_( return )
BOOLEAN
AmlEvalDerefOf(
    _Inout_  AML_STATE*   State,
    _In_     BOOLEAN      ConsumeOpcode,
    _Outptr_ AML_OBJECT** ppObject
    );

//
// Evaluate IndexOp.
// BuffPkgStrObj := TermArg => Buffer, Package, or String
// IndexValue := TermArg => Integer
// DefIndex := IndexOp BuffPkgStrObj IndexValue Target
// 
// TODO: Fix up the object/data system, this function is a great showcase of how it is lacking.
//
_Success_( return )
BOOLEAN
AmlEvalIndex(
    _Inout_  AML_STATE*   State,
    _In_     BOOLEAN      ConsumeOpcode,
    _Outptr_ AML_OBJECT** ppObject
    );

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
    );

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
    );

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
    );

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
    );

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
    );

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
    );

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
    );

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
    );

//
// Attempt to search for the given node's parent PCI(e) root bus.
// Searches ancestors for a _HID or _CID matching the PCI(e) root bus EISAIDs.
//
_Success_( return != NULL )
AML_NAMESPACE_NODE*
AmlEvalNodePciParentRootBus(
    _Inout_ AML_STATE*          State,
    _In_    AML_NAMESPACE_NODE* Node
    );

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
    );

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
    );

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
    );
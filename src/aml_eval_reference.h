#pragma once

#include "aml_decoder.h"

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
// MethodInvocation := NameString TermArgList
// ReferenceTypeOpcode := DefRefOf | DefDerefOf | DefIndex | MethodInvocation
// The returned object is referenced and must be released by the caller.
//
_Success_( return )
BOOLEAN
AmlEvalReferenceTypeOpcode(
    _Inout_  AML_STATE*   State,
    _In_     UINT         SearchFlags,
    _Outptr_ AML_OBJECT** ppObject
    );
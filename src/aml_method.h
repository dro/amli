#pragma once

#include "aml_platform.h"
#include "aml_data.h"
#include "aml_object.h"
#include "aml_namespace.h"

//
// A temporary namespace node created by the current method that must be destroyed upon the end of the method's scope.
//
typedef struct _AML_METHOD_NAMESPACE_NODE {
	struct _AML_METHOD_NAMESPACE_NODE* Next;
	AML_NAMESPACE_NODE*                Node;
} AML_METHOD_NAMESPACE_NODE;

//
// Method call stack scope.
//
typedef struct _AML_METHOD_SCOPE {
	//
	// Method scope stack links.
	//
	struct _AML_METHOD_SCOPE* Parent;
	struct _AML_METHOD_SCOPE* Child;

	//
	// Scope arena snapshot to rollback to when popping this scope level.
	//
	AML_ARENA_SNAPSHOT ArenaSnapshot;

	//
	// Head of the temporary namespace node list for the current method scope.
	// Any namespace nodes/objects created inside of a method are temporary,
	// and are destroyed upon exit of the method's scope.
	//
	AML_METHOD_NAMESPACE_NODE* TempNodeHead;

	//
	// Up to 8 local objects can be referenced in a control method.
	// On entry to a control method, these objects are uninitialized and
	// cannot be used until some value or reference is stored into the object.
	// Once initialized, these objects are preserved in the scope of execution for that control method.
	//
	AML_OBJECT* LocalObjects[ 8 ];

	//
	// Arguments passed to the current method.
	//
	AML_OBJECT* ArgObjects[ 7 ];

	//
	// Mutex acquisition list for the current method scope.
	// Any mutex acquired inside the scope of the method must be released upon exit of the scope.
	//
	struct _AML_MUTEX_SCOPE_LEVEL* MutexHead;
	struct _AML_MUTEX_SCOPE_LEVEL* MutexTail;

	//
	// Current return value of the method scope.
	//
	AML_DATA ReturnValue;
} AML_METHOD_SCOPE;

//
// Push a new method scope stack level.
//
_Success_( return )
BOOLEAN
AmlMethodPushScope(
	_Inout_ struct _AML_STATE* State
	);

//
// Pop the last method scope stack level.
//
_Success_( return )
BOOLEAN
AmlMethodPopScope(
	_Inout_ struct _AML_STATE* State
	);

//
// Track an acquisition of the given mutex within the current method scope.
//
_Success_( return )
BOOLEAN
AmlMethodScopeMutexAcquire(
	_Inout_     struct _AML_STATE* State,
	_Inout_     AML_METHOD_SCOPE*  Scope,
	_Inout_opt_ AML_OBJECT*        MutexObject
	);

//
// Track a release of the given mutex within the current method scope.
//
_Success_( return )
BOOLEAN
AmlMethodScopeMutexRelease(
	_Inout_     struct _AML_STATE* State,
	_Inout_     AML_METHOD_SCOPE*  Scope,
	_Inout_opt_ AML_OBJECT*        MutexObject
	);

//
// Evaluate a method invocation to a created method object.
// Does not consume any code from the state (for arguments),
// requires all arguments to be setup and passed here.
//
_Success_( return )
BOOLEAN
AmlMethodInvoke(
	_Inout_                     struct _AML_STATE* State,
	_Inout_                     AML_OBJECT*        MethodObject,
	_In_                        UINT               Flags,
	_In_count_( ArgumentCount ) AML_DATA*          Arguments,
	_In_                        SIZE_T             ArgumentCount,
	_Out_opt_                   AML_DATA*          pReturnValueOutput
	);

//
// Create a native method, implemented by the host OS, callable through AML.
//
_Success_( return )
BOOLEAN
AmlMethodCreateNative(
	_Inout_  struct _AML_STATE*      State,
	_In_     const AML_NAME_STRING*  PathName,
	_In_     AML_METHOD_USER_ROUTINE UserRoutine,
	_In_opt_ VOID*                   UserContext,
	_In_     SIZE_T                  ArgumentCount,
	_In_     UINT8                   SyncLevel
	);
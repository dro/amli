#pragma once

#include "aml_platform.h"

//
// Attempt to acquire the given mutex object.
// Handles _GL mutex with special semantics, acquires the ACPI global lock.
//
_Success_( return == AML_WAIT_STATUS_SUCCESS )
AML_WAIT_STATUS
AmlMutexAcquire(
	_Inout_ struct _AML_STATE*  State,
	_Inout_ struct _AML_OBJECT* MutexObject,
	_In_    UINT16              TimeoutMs
	);

//
// Attempt to release the given mutex object previously acquired using AmlMutexAcquire.
//
VOID
AmlMutexRelease(
	_Inout_ struct _AML_STATE*  State,
	_Inout_ struct _AML_OBJECT* Mutex
	);
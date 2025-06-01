#include "aml_platform.h"
#include "aml_object.h"
#include "aml_decoder.h"
#include "aml_debug.h"
#include "aml_host.h"
#include "aml_mutex.h"

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
	)
{
	AML_OBJECT_MUTEX* Mutex;
	AML_WAIT_STATUS   WaitStatus;
	UINT64            EndTime;

	//
	// Validate input mutex object.
	//
	if( ( MutexObject == NULL ) || ( MutexObject->Type != AML_OBJECT_TYPE_MUTEX ) ) {
		return AML_WAIT_STATUS_ERROR;
	}
	Mutex = &MutexObject->u.Mutex;

	//
	// Attempt to acquire the given mutex.
	//
	WaitStatus = AmlHostMutexAcquire( State->Host, Mutex->HostHandle, TimeoutMs );
	if( WaitStatus == AML_WAIT_STATUS_ERROR ) {
		return WaitStatus;
	}

	//
	// If this is the the special _GL mutex, handle acquiring the actual APCI global lock.
	// TODO: Add deadlock check.
	// TODO: Use host event/semaphore.
	//
	if( ( MutexObject == State->GlobalLock ) && ( WaitStatus == AML_WAIT_STATUS_SUCCESS ) ) {
		//
		// Attempt to spin and acquire the ACPI global lock until we timeout (if one is given).
		// If we are already holding the global lock, raise the counter and avoid deadlock
		// caused by trying to acquire from ourselves.
		//
		if( State->GlobalLockHoldCount == 0 ) {
			EndTime = ( AmlHostMonotonicTimer( State->Host ) + ( ( UINT64 )TimeoutMs * 10000 ) );
			WaitStatus = AML_WAIT_STATUS_TIMEOUT;
			AML_DEBUG_TRACE( State, "Attempting to acquire ACPI global lock.\n" );
			do {
				if( AmlHostGlobalLockTryAcquire( State->Host ) ) {
					WaitStatus = AML_WAIT_STATUS_SUCCESS;
					State->GlobalLockHoldCount = 1;
					break;
				}
				AML_PAUSE(); AML_PAUSE(); AML_PAUSE(); AML_PAUSE();
			} while( ( TimeoutMs == 0xFFFF ) || ( AmlHostMonotonicTimer( State->Host ) < EndTime ) );
		} else {
			State->GlobalLockHoldCount += 1;
		}
	}

	//
	// If we have acquired the mutex, inform the method layer to keep track of all acquisitions per-scope.
	//
	if( WaitStatus == AML_WAIT_STATUS_SUCCESS ) {
		if( AmlMethodScopeMutexAcquire( State, State->MethodScopeLast, MutexObject ) == AML_FALSE ) {
			AML_DEBUG_PANIC( State, "Fatal: AmlMethodScopeMutexAcquire failed (OOM or implementation bug)!" );
			return AML_WAIT_STATUS_ERROR;
		}
	}

	return WaitStatus;
}

//
// Attempt to release the given mutex object previously acquired using AmlMutexAcquire.
// TODO: Validate hold count for all mutexes, make over-releasing impossible!
//
VOID
AmlMutexRelease(
	_Inout_ struct _AML_STATE*  State,
	_Inout_ struct _AML_OBJECT* MutexObject
	)
{
	AML_OBJECT_MUTEX* Mutex;

	//
	// Validate input mutex object.
	//
	if( ( MutexObject == NULL ) || ( MutexObject->Type != AML_OBJECT_TYPE_MUTEX ) ) {
		return;
	}
	Mutex = &MutexObject->u.Mutex;

	//
	// Inform the method layer to keep track of all releases per-scope.
	//
	if( AmlMethodScopeMutexRelease( State, State->MethodScopeLast, MutexObject ) == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: AmlMethodScopeMutexRelease failed.\n" );
		return;
	}

	//
	// If this is the special _GL mutex, handle releasing the actual ACPI global lock.
	//
	if( ( MutexObject == State->GlobalLock ) && ( State->GlobalLockHoldCount > 0 ) ) {
		if( State->GlobalLockHoldCount == 1 ) {
			AML_DEBUG_TRACE( State, "Releasing ACPI global lock.\n" );
		}
		AmlHostGlobalLockRelease( State->Host );
		State->GlobalLockHoldCount -= 1;
	}

	//
	// Release the actual host mutex object.
	//
	AmlHostMutexRelease( State->Host, Mutex->HostHandle );
}
#include "aml_method.h"
#include "aml_eval.h"
#include "aml_debug.h"
#include "aml_state.h"
#include "aml_mutex.h"

//
// Track an acquisition of the given mutex within the current method scope.
//
_Success_( return )
BOOLEAN
AmlMethodScopeMutexAcquire(
	_Inout_     struct _AML_STATE* State,
	_Inout_     AML_METHOD_SCOPE*  Scope,
	_Inout_opt_ AML_OBJECT*        MutexObject
	)
{
	AML_OBJECT_MUTEX*      Mutex;
	AML_MUTEX_SCOPE_LEVEL* MutexScope;

	//
	// A mutex acquire/release must only happen for the current scope level. 
	//
	if( ( Scope == NULL ) || ( Scope != State->MethodScopeLast ) ) {
		return AML_FALSE;
	}

	//
	// Ensure that a valid mutex object was passed.
	//
	if( ( MutexObject == NULL ) || ( MutexObject->Type != AML_OBJECT_TYPE_MUTEX ) ) {
		return AML_TRUE;
	}
	Mutex = &MutexObject->u.Mutex;

	//
	// If there isn't already a mutex scope level entry for the current method scope, push a new one.
	//
	if( ( Mutex->ScopeEntry == NULL ) || ( Mutex->ScopeEntry->MethodScope != Scope ) ) {
		//
		// Allocate and initialize a new scope level for the mutex.
		//
		MutexScope = AmlArenaAllocate( &State->MethodScopeArena, sizeof( *MutexScope ) );
		if( MutexScope == NULL ) {
			return AML_FALSE;
		}
		*MutexScope = ( AML_MUTEX_SCOPE_LEVEL ){
			.Object        = MutexObject,
			.AcquireCount  = 0,
			.MethodScope   = Scope,
			.PreviousLevel = Mutex->ScopeEntry
		};
		AmlObjectReference( MutexObject );
		Mutex->ScopeEntry = MutexScope;

		//
		// Link the new mutex scope level to the current method scope.
		//
		if( Scope->MutexTail != NULL ) {
			Scope->MutexTail->NextSibling = MutexScope;
		}
		Scope->MutexTail = MutexScope;
		Scope->MutexHead = ( Scope->MutexHead ? Scope->MutexHead : MutexScope );
	}

	//
	// Raise the acquisition counter of the current mutex scope level.
	//
	if( Mutex->ScopeEntry->AcquireCount >= ~( SIZE_T )0 ) {
		AML_DEBUG_ERROR( State, "Error: Exceeded maximum mutex acquire count in current method scope!\n" );
		return AML_FALSE;
	}
	AML_DEBUG_TRACE(
		State,
		"Raised method mutex AcquireCount: (%"PRId64") -> (%"PRId64")\n",
		Mutex->ScopeEntry->AcquireCount,
		( Mutex->ScopeEntry->AcquireCount + 1 )
	);
	Mutex->ScopeEntry->AcquireCount += 1;
	return AML_TRUE;
}

//
// Track a release of the given mutex within the current method scope.
//
_Success_( return )
BOOLEAN
AmlMethodScopeMutexRelease(
	_Inout_     struct _AML_STATE* State,
	_Inout_     AML_METHOD_SCOPE*  Scope,
	_Inout_opt_ AML_OBJECT*        MutexObject
	)
{
	AML_OBJECT_MUTEX* Mutex;

	//
	// A mutex acquire/release must only happen for the current scope level. 
	//
	if( ( Scope == NULL ) || ( Scope != State->MethodScopeLast ) ) {
		return AML_FALSE;
	}

	//
	// Ensure that a valid mutex object was passed.
	//
	if( ( MutexObject == NULL ) || ( MutexObject->Type != AML_OBJECT_TYPE_MUTEX ) ) {
		return AML_TRUE;
	}
	Mutex = &MutexObject->u.Mutex;

	//
	// A valid mutex scope level must have already been created for the mutex.
	// TODO: Check method scope generation index.
	//
	if( ( Mutex->ScopeEntry == NULL ) || ( Mutex->ScopeEntry->MethodScope != Scope ) ) {
		return AML_FALSE;
	}

	//
	// Lower the acquisition count of the mutex in the current scope.
	//
	if( Mutex->ScopeEntry->AcquireCount == 0 ) {
		AML_DEBUG_ERROR( State, "Error: Exceeded maximum mutex release count in current method scope!\n" );
		return AML_FALSE;
	}
	AML_DEBUG_TRACE(
		State,
		"Lowered method mutex AcquireCount: (%"PRId64") -> (%"PRId64")\n",
		Mutex->ScopeEntry->AcquireCount,
		( Mutex->ScopeEntry->AcquireCount - 1 )
	);
	Mutex->ScopeEntry->AcquireCount -= 1;
	return AML_TRUE;
}

//
// Release any child resources/references held by the scope level.
// Does not rollback the arena snapshot.
//
static
VOID
AmlMethodScopeReleaseInternal(
	_Inout_ AML_STATE*        State,
	_Inout_ AML_METHOD_SCOPE* Scope
	)
{
	SIZE_T                 i;
	AML_MUTEX_SCOPE_LEVEL* MutexScope;
	SIZE_T                 AcquireCount;

	//
	// Release arg objects.
	//
	for( i = 0; i < AML_COUNTOF( Scope->ArgObjects ); i++ ) {
		if( Scope->ArgObjects[ i ] != NULL ) {
			AmlObjectRelease( Scope->ArgObjects[ i ] );
			Scope->ArgObjects[ i ] = NULL;
		}
	}

	//
	// Release local objects.
	//
	for( i = 0; i < AML_COUNTOF( Scope->LocalObjects ); i++ ) {
		if( Scope->LocalObjects[ i ] != NULL ) {
			AmlObjectRelease( Scope->LocalObjects[ i ] );
			Scope->LocalObjects[ i ] = NULL;
		}
	}

	//
	// Release mutex scope entries.
	//
	for( MutexScope = Scope->MutexHead; MutexScope != NULL; MutexScope = MutexScope->NextSibling ) {
		if( ( MutexScope->Object != NULL ) && ( MutexScope->Object->Type == AML_OBJECT_TYPE_MUTEX ) ) {
			//
			// Release all acquisitions of the mutex performed during this method scope.
			//
			AcquireCount = MutexScope->AcquireCount;
			for( i = 0; i < AcquireCount; i++ ) {
				AmlMutexRelease( State, MutexScope->Object );
			}
			if( MutexScope->AcquireCount != 0 ) {
				AML_DEBUG_PANIC( State, "Fatal: Failed to release all per-method-scope mutex acquires!" );
			}
			MutexScope->AcquireCount = 0;

			//
			// Pop the scope level of the mutex to that of the previous method scope.
			//
			MutexScope->Object->u.Mutex.ScopeEntry = MutexScope->PreviousLevel;
		}
		AmlObjectRelease( MutexScope->Object );
		MutexScope->Object = NULL;
	}

	//
	// Free return value.
	// TODO: Make the return value an object instead of plain data?
	//
	AmlDataFree( &Scope->ReturnValue );
}

//
// Push a new method scope stack level.
//
_Success_( return )
BOOLEAN
AmlMethodPushScope(
	_Inout_ struct _AML_STATE* State
	)
{
	AML_ARENA_SNAPSHOT ScopeArenaSnapshot;
	AML_METHOD_SCOPE*  Scope;
	SIZE_T             i;
	AML_OBJECT*        Object;
	BOOLEAN            Success;

	//
	// Attempt to create an arena snapshot, and allocate a new method scope.
	// Allows us to roll back all allocations local to this scope.
	//
	ScopeArenaSnapshot = AmlArenaSnapshot( &State->MethodScopeArena );
	Scope = AmlArenaAllocate( &State->MethodScopeArena, sizeof( AML_METHOD_SCOPE ) );
	if( Scope == NULL ) {
		return AML_FALSE;
	}

	//
	// Default initialize method scope level.
	//
	*Scope = ( AML_METHOD_SCOPE ){ .ArenaSnapshot = ScopeArenaSnapshot };

	//
	// Create initial arg objects.
	//
	for( i = 0; i < AML_COUNTOF( Scope->ArgObjects ); i++ ) {
		if( ( Success = AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_NAME, &Object ) ) == AML_FALSE ) {
			break;
		}
		Object->SuperType = AML_OBJECT_SUPERTYPE_ARG;
		Scope->ArgObjects[ i ] = Object;
	}

	//
	// Create initial local objects.
	//
	for( i = 0; i < AML_COUNTOF( Scope->LocalObjects ); i++ ) {
		if( ( Success = AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_NAME, &Object ) ) == AML_FALSE ) {
			break;
		}
		Object->SuperType = AML_OBJECT_SUPERTYPE_LOCAL;
		Scope->LocalObjects[ i ] = Object;
	}

	//
	// If we have failed to create any initial objects, release any created ones, and rollback stack snapshot.
	//
	if( Success == AML_FALSE ) {
		AmlMethodScopeReleaseInternal( State, Scope );
		AmlArenaSnapshotRollback( &State->MethodScopeArena, &ScopeArenaSnapshot );
		return AML_FALSE;
	}

	//
	// Push the created method scope level to the stack.
	//
	Scope->Parent = State->MethodScopeLast;
	if( State->MethodScopeLast != NULL ) {
		State->MethodScopeLast->Child = Scope;
	}
	State->MethodScopeLast  = Scope;
	State->MethodScopeFirst = ( ( State->MethodScopeFirst != NULL ) ? State->MethodScopeFirst : Scope );

	//
	// Increase the current method scope level.
	//
	State->MethodScopeLevel += 1;
	return AML_TRUE;
}

//
// Pop the last method scope stack level.
//
_Success_( return )
BOOLEAN
AmlMethodPopScope(
	_Inout_ struct _AML_STATE* State
	)
{
	AML_METHOD_SCOPE* PoppedScope;

	//
	// Push/pop mismatch count, pop has been called more than push.
	//
	if( ( State->MethodScopeLast == NULL ) || ( State->MethodScopeLevel <= State->MethodScopeLevelStart ) ) {
		AML_DEBUG_ERROR( State, "Error: AmlMethodPopScope count mismatch!\n" );
		return AML_FALSE;
	}

	//
	// Release any child resources/references held by the popped scope level.
	//
	AmlMethodScopeReleaseInternal( State, State->MethodScopeLast );

	//
	// Pop the last scope entry from the stack.
	//
	PoppedScope = State->MethodScopeLast;
	if( PoppedScope->Parent != NULL ) {
		PoppedScope->Parent->Child = NULL;
	}
	State->MethodScopeLast = PoppedScope->Parent;
	State->MethodScopeFirst = ( ( State->MethodScopeFirst != PoppedScope ) ? State->MethodScopeFirst : NULL );

	//
	// Decrease the current scope level index.
	//
	State->MethodScopeLevel -= 1;

	//
	// Rollback the allocation of the scope entry and all of its suballocations.
	//
	return AmlArenaSnapshotRollback( &State->MethodScopeArena, &PoppedScope->ArenaSnapshot );
}

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
	)
{
	AML_NAMESPACE_NODE*    MethodNsNode;
	AML_METHOD_SCOPE*      Scope;
	AML_OBJECT_METHOD*     MethodInfo;
	SIZE_T                 i;
	BOOLEAN                Success;
	AML_DATA               ReturnValue;
	AML_INTERRUPTION_EVENT OldPendingEvent;
	AML_INTERRUPTION_EVENT PendingEvent;

	//
	// The given object must have a namespace node attached (for scope informaiton).
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
	// Validate number of method arguments.
	//
	MethodInfo = &MethodObject->u.Method;
	if( ( MethodInfo->ArgumentCount > AML_COUNTOF( Scope->ArgObjects ) )
		|| ( ArgumentCount < MethodInfo->ArgumentCount ) )
	{
		return AML_FALSE;
	}

	//
	// Push a new method scope stack level to be used for the method call.
	//
	if( AmlMethodPushScope( State ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Setup initial internal variables before executing the method.
	//
	Scope = State->MethodScopeLast;
	Success = AML_TRUE;
	ReturnValue = ( AML_DATA ){ .Type = AML_DATA_TYPE_NONE };
	do {
		//
		// Copy all input argument values to the current method scope.
		// TODO: Ensure that Duplicate works here, it should probably be a deep copy, need to investigate these cases.
		//
		for( i = 0; i < MethodInfo->ArgumentCount; i++ ) {
			if( ( Success = AmlDataDuplicate( &Arguments[ i ], &State->Heap, &Scope->ArgObjects[ i ]->u.Name.Value ) ) == AML_FALSE ) {
				break;
			}
		}

		//
		// Abort and clean up if we have failed to evaluate/decode any of the call's arguments.
		//
		if( Success == AML_FALSE ) {
			break;
		}

		//
		// Backup old pending interruption event state.
		//
		OldPendingEvent = State->PendingInterruptionEvent;

		//
		// Begin a new state snapshot, allowing us to rollback any leftover allocations/objects/nodes upon failure.
		//
		if( ( Success = AmlStateSnapshotBegin( State ) ) == AML_FALSE ) {
			break;
		}

		//
		// Push a new namespace scope level with the scope of the method to be used for the method call.
		//
		if( ( Success = AmlNamespacePushScope( &State->Namespace, &MethodNsNode->AbsolutePath, AML_SCOPE_FLAG_TEMPORARY ) ) == AML_FALSE ) {
			break;
		}

		//
		// Begin executing the body of the method until we hit a return or the end of the method.
		// We pass AML_TRUE for RestoreDataCursor, this will cause the IP to be restored to the original place
		// (following the call instruction + args), after execution of the method.
		// Uses a temporary hack to update the temporary scope of the method.
		// Will not function if multithreading is implemented.
		// TODO: Figure out a better way to do this!
		//
		MethodNsNode->TempScope = State->Namespace.ScopeLast;
		if( MethodInfo->UserRoutine == NULL ) {
			State->Data = MethodInfo->CodeDataBlock;
			State->DataTotalLength = MethodInfo->CodeDataBlockSize;
			Success = AmlEvalTermListCode( State, MethodInfo->CodeStart, MethodInfo->CodeSize, AML_TRUE );
		} else {
			Success = MethodInfo->UserRoutine( State, MethodInfo->UserContext, Arguments, ArgumentCount, &Scope->ReturnValue );
		}

		//
		// Pop the namespace scope level of the method.
		//
		Success &= AmlNamespacePopScope( &State->Namespace );

		//
		// Either rollback or commit pending objects depending on if the full method invocation succeeded,
		//
		if( Success ) {
			AmlStateSnapshotCommit( State, AML_TRUE );
		} else {
			AmlStateSnapshotRollback( State );
		}

		//
		// Clear pending handled return interruption event.
		// No control flow interruption events other than return should be triggered,
		// this means a break or continue was executed inside of a method, but outside of a loop body.
		//
		PendingEvent = State->PendingInterruptionEvent;
		if( ( PendingEvent != AML_INTERRUPTION_EVENT_NONE )
			&& ( PendingEvent != AML_INTERRUPTION_EVENT_RETURN ) )
		{
			AML_DEBUG_ERROR( State, "Error: Invalid interruption event inside of a method (continue/break outside of loop body)\n" );
			Success = AML_FALSE;
			break;
		}
		State->PendingInterruptionEvent = OldPendingEvent;
		if( Success == AML_FALSE ) {
			break;
		}

		//
		// Copy the return value of the method back to the caller.
		//
		Success = AmlDataDuplicate( &Scope->ReturnValue, &State->Heap, &ReturnValue );
	} while( 0 );

	//
	// Pop the method scope level (and release method scope resources).
	//
	if( Scope != State->MethodScopeLast ) {
		AML_TRAP_STRING( "Method scope stack out of sync!" );
	}
	Success &= AmlMethodPopScope( State );

	//
	// Free the return value (if any) upon failure.
	//
	if( Success == AML_FALSE ) {
		AmlDataFree( &ReturnValue );
		return AML_FALSE;
	}

	//
	// Pass the method's return value back to the caller if desired.
	//
	if( pReturnValueOutput != NULL ) {
		*pReturnValueOutput = ReturnValue;
	} else {
		AmlDataFree( &ReturnValue );
	}

	return AML_TRUE;
}

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
	)
{
	AML_NAMESPACE_NODE* NsNode;
	AML_OBJECT*         Object;

	//
	// Validate input arguments.
	//
	if( ArgumentCount > AML_COUNTOF( ( ( AML_METHOD_SCOPE* )0 )->ArgObjects ) ) {
		return AML_FALSE;
	}

	//
	// Attempt to create a namespace node with the given path.
	// An object with this path/name must not already exist.
	//
	if( AmlStateSnapshotCreateNode( State, NULL, PathName, &NsNode ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Attempt to create a new object for the method.
	//
	if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_METHOD, &Object ) == AML_FALSE ) {
		AmlNamespaceReleaseNode( &State->Namespace, NsNode );
		return AML_FALSE;
	}

	//
	// Initialize new method object.
	//
	Object->u.Method = ( AML_OBJECT_METHOD ){
		.UserRoutine   = UserRoutine,
		.UserContext   = UserContext,
		.ArgumentCount = ( UINT8 )ArgumentCount,
		.SyncLevel     = SyncLevel,
	};

	//
	// Link the namespace node and object together.
	//
	Object->NamespaceNode = NsNode;
	NsNode->Object = Object;
	return AML_TRUE;
}
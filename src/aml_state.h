#pragma once

#include "aml_platform.h"
#include "aml_arena.h"
#include "aml_heap.h"
#include "aml_host.h"
#include "aml_object.h"
#include "aml_namespace.h"
#include "aml_method.h"
#include "aml_operation_region.h"
#include "aml_state_snapshot.h"
#include "aml_state_pass.h"

//
// Maximum recursion depth limit.
//
#ifndef AML_BUILD_MAX_RECURSION_DEPTH
 #ifdef AML_BUILD_FUZZER
  #define AML_BUILD_MAX_RECURSION_DEPTH 128
 #else
  #define AML_BUILD_MAX_RECURSION_DEPTH 2048
 #endif
#endif

//
// There is currently a problem with this feature, forced to be disabled for now.
//
#define AML_BUILD_NO_SNAPSHOT_ITEMS

//
// AML state.
//
typedef struct _AML_STATE {
	//
	// Permanent arena/state.
	//
	AML_ARENA Arena;
	AML_HEAP  Heap;

	//
	// Host interface context.
	//
	AML_HOST_CONTEXT* Host;

	//
	// Indicates if the main DSDT/SSDT initial evaluation is complete.
	//
	BOOLEAN IsInitialLoadComplete;

	//
	// ASL integer sizes depend on the ComplianceRevision field of a DSDT. If this is greater than 1, the
	// integer size will be 64 bits in size. Otherwise, integers are 32 bits in size.
	//
	BOOLEAN IsIntegerSize64;

	//
	// Global namespace state.
	//
	AML_NAMESPACE_STATE Namespace;

	//
	// Method scope state.
	//
	AML_ARENA         MethodScopeArena;
	AML_METHOD_SCOPE* MethodScopeFirst;
	AML_METHOD_SCOPE* MethodScopeLast;
	AML_METHOD_SCOPE* MethodScopeRoot; /* Root method scope, the first of the current execution environment. */

	//
	// Current evaluation and decoder state.
	//
	struct {
		//
		// Pass type, indicates if we are fully evaluating code,
		// or if we are doing the initial namespace discovery pass.
		//
		AML_PASS_TYPE PassType;

		//
		// Per-method scope state.
		//
		SIZE_T MethodScopeLevel;
		SIZE_T MethodScopeLevelStart;

		//
		// The input AML bytecode being decoded, and our current cursor offset into it.
		//
		const UINT8* Data;
		SIZE_T       DataTotalLength; /* Length of all input data. */
		SIZE_T       DataLength;      /* The length is the absolute input data length, relative to start of Data, not the cursor. */
		SIZE_T       DataCursor;      /* Cursor position of the current window of the interpreter. */

		//
		// While loop state.
		//
		SIZE_T WhileLoopLevel;

		//
		// Active recursion depth.
		//
		SIZE_T RecursionDepth;

		//
		// Control flow interruption event state (continue, break, return, etc).
		//
		AML_INTERRUPTION_EVENT PendingInterruptionEvent;
	};

	//
	// Internal DebugObj sentinel object, can never be free'd.
	//
	AML_OBJECT DebugObject;

	//
	// Internal global lock mutex with special semantics.
	//
	AML_OBJECT* GlobalLock;
	UINT64      GlobalLockHoldCount;

	//
	// IO access handler table.
	// Used to dispatch to a region-space-specific handler for operation region access.
	//
	AML_REGION_ACCESS_REGISTRATION RegionSpaceHandlers[ AML_MAX_SPEC_REGION_SPACE_TYPE_COUNT ];

	//
	// State snapshot stack, allows rollback of certain state items upon error.
	//
	AML_ARENA           StateSnapshotArena;
	AML_STATE_SNAPSHOT* StateSnapshotHead;
	AML_STATE_SNAPSHOT* StateSnapshotTail;
	UINT64              StateSnapshotLevelIndex;
} AML_STATE;

//
// User-provided state creation parameters.
//
typedef struct _AML_STATE_PARAMETERS {
	BOOLEAN           Use64BitInteger;
	AML_HOST_CONTEXT* Host;
} AML_STATE_PARAMETERS;

//
// Initialize the AML decoder/evaluation state.
// Note: The given allocator and host context must exist for the entire lifetime of the state.
//
_Success_( return )
BOOLEAN
AmlStateCreate(
	_Out_ AML_STATE*                  State,
	_In_  AML_ALLOCATOR               Allocator,
	_In_  const AML_STATE_PARAMETERS* Parameters
	);

//
// Release all underlying state state resources.
//
VOID
AmlStateFree(
	_Inout_ _Post_invalid_ AML_STATE* State
	);

//
// Create all predefined namespaces.
//
_Success_( return )
BOOLEAN
AmlCreatePredefinedNamespaces(
	_Inout_ AML_STATE* State
	);

//
// Create all predefined objects.
//
VOID
AmlCreatePredefinedObjects(
	_Inout_ AML_STATE* State
	);

//
// Call the _INI methods for all functioning devices that advertise their presence through _STA.
// CallRootInitializers determines if the unconditional root \_INI and \_SB_._INI methods
// should be called to adhere with the typical expected behavior of other interpreters.
// Typically AmlCompleteInitialLoad should be used instead of this function directly.
//
_Success_( return )
BOOLEAN
AmlInitializeDevices(
	_Inout_ AML_STATE*               State,
	_Inout_ AML_NAMESPACE_TREE_NODE* StartTreeNode,
	_In_    BOOLEAN                  CallRootInitializers
	);

//
// Complete the initial load, build the hierarchical namespace tree, initialize all device objects.
//
_Success_( return )
BOOLEAN
AmlCompleteInitialLoad(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    InitializeDevices
	);

//
// Call all device _REG handlers for the given region space type (if valid for _REG).
//
_Success_( return )
BOOLEAN
AmlBroadcastRegionSpaceStateUpdate(
	_Inout_ AML_STATE* State,
	_In_    UINT8      RegionSpaceType,
	_In_    BOOLEAN    EnableState,
	_In_    BOOLEAN    IsTrueUpdate
	);

//
// Attempt to register a region-space access handler.
// Registering a handler for a region-space type that has already had a handler register
// will overwrite the existing handler, and the caller must take their precaution to free
// any resources associated with the old registration's user data.
// Routine remain valid along with the given UserContext until the end of the AML state usage.
//
_Success_( return )
BOOLEAN
AmlRegisterRegionSpaceAccessHandler(
	_Inout_  AML_STATE*                State,
	_In_     UINT8                     RegionSpaceType,
	_In_opt_ AML_REGION_ACCESS_ROUTINE UserRoutine,
	_In_opt_ VOID*                     UserContext,
	_In_     BOOLEAN                   CallRegMethods
	);

//
// Lookup the access handler registration entry for the given region space tyhpe.
//
_Success_( return != NULL )
AML_REGION_ACCESS_REGISTRATION*
AmlLookupRegionSpaceAccessHandler(
	_In_ AML_STATE* State,
	_In_ UINT8      RegionSpaceType
	);
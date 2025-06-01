#pragma once

#include "aml_platform.h"
#include "aml_arena.h"
#include "aml_object_type.h"
#include "aml_state_pass.h"

//
// Underlying data type referred to by the snapshot item.
//
typedef enum _AML_STATE_SNAPSHOT_ITEM_TYPE {
	AML_STATE_SNAPSHOT_ITEM_TYPE_BUFFER,
	AML_STATE_SNAPSHOT_ITEM_TYPE_PACKAGE,
	AML_STATE_SNAPSHOT_ITEM_TYPE_NODE,
	AML_STATE_SNAPSHOT_ITEM_TYPE_MUTEX,
} AML_STATE_SNAPSHOT_ITEM_TYPE;

//
// Each item frame corresponds to all actions performed on this item during a certain snapshot stack level.
//
typedef struct _AML_STATE_SNAPSHOT_ITEM_FRAME {
	//
	// Stack of all actions performed on this item during the current snapshot.
	//
	struct _AML_STATE_SNAPSHOT_ACTION* ActionFirst;
	struct _AML_STATE_SNAPSHOT_ACTION* ActionLast;

	//
	// Links to the previous/next frame in the frame stack of the parent item.
	//
	struct _AML_STATE_SNAPSHOT_ITEM*       Item;
	struct _AML_STATE_SNAPSHOT_ITEM_FRAME* FramePrevious;
	struct _AML_STATE_SNAPSHOT_ITEM_FRAME* FrameNext;

	//
	// Links to the previous/next item frames referenced in the corresponding snapshot.
	//
	struct _AML_STATE_SNAPSHOT_ITEM_FRAME* SnapFramePrevious;
	struct _AML_STATE_SNAPSHOT_ITEM_FRAME* SnapFrameNext;

	//
	// Per-snapshot frame use counter.
	//
	UINT64 Counter;
	UINT64 LevelIndex; /* Debug. */
} AML_STATE_SNAPSHOT_ITEM_FRAME;

//
// A piece of underlying state data referenced by an item.
//
typedef union _AML_STATE_SNAPSHOT_ITEM_DATA {
	struct _AML_BUFFER_DATA*    Buffer;
	struct _AML_PACKAGE_DATA*   Package;
	struct _AML_NAMESPACE_NODE* Node;
	struct _AML_OBJECT*         Mutex;
} AML_STATE_SNAPSHOT_ITEM_DATA;

//
// An item references the underlying state datum and maintains snapshot state
// specific to the item itself. Items contain a stack of frames that correspond
// to a cached list of actions performed on the item during a certain snapshot level.
//
typedef struct _AML_STATE_SNAPSHOT_ITEM {
	struct _AML_STATE*                     State;
	struct _AML_STATE_SNAPSHOT_ITEM_FRAME* FrameFirst;
	struct _AML_STATE_SNAPSHOT_ITEM_FRAME* FrameLast;
	UINT32                                 Valid;
	AML_STATE_SNAPSHOT_ITEM_TYPE           Type;
	AML_STATE_SNAPSHOT_ITEM_DATA           u; /* Item data union. */
	UINT64                                 LevelIndex;
	UINT64                                 ReferenceCount;
} AML_STATE_SNAPSHOT_ITEM;

//
// An action is a cached operation performed on an item during a certain snapshot level.
// Actions either increase or decrease the item's use counter for that snapshot level.
// When an item's use counter is above 0 when rolling back a snapshot, the item
// is considered still live, and will be automatically released upon rollback.
//
typedef struct _AML_STATE_SNAPSHOT_ACTION {
	struct _AML_STATE_SNAPSHOT_ACTION* Next;
	AML_STATE_SNAPSHOT_ITEM*           Item;
	BOOLEAN                            IsRaise;
} AML_STATE_SNAPSHOT_ACTION;

//
// A snapshot of all actions performed to applicable items.
//
typedef struct _AML_STATE_SNAPSHOT {
	//
	// Internal snapshot management state.
	//
	AML_ARENA_SNAPSHOT                     ArenaSnapshot;
	struct _AML_STATE_SNAPSHOT*            StatePrevious;
	struct _AML_STATE_SNAPSHOT*            StateNext;
	struct _AML_STATE_SNAPSHOT_ITEM_FRAME* FrameFirst;
	struct _AML_STATE_SNAPSHOT_ITEM_FRAME* FrameLast;
	UINT64                                 LevelIndex;

	//
	// Backup of core AML evaluation state at time of the snapshot.
	// See AML_STATE for a proper description of these fields.
	//
	AML_PASS_TYPE                PassType;
	SIZE_T                       MethodScopeLevel;
	SIZE_T                       MethodScopeLevelStart;
	const UINT8*                 Data;
	SIZE_T                       DataTotalLength;
	SIZE_T                       DataLength;
	SIZE_T                       DataCursor;
	SIZE_T                       WhileLoopLevel;
	AML_INTERRUPTION_EVENT       PendingInterruptionEvent;
	struct _AML_NAMESPACE_SCOPE* NamespaceScopeLast;
} AML_STATE_SNAPSHOT;

//
// Allocate and initialize a new state snapshot item referencing an actionable piece of data.
//
_Success_( return != NULL )
AML_STATE_SNAPSHOT_ITEM*
AmlStateSnapshotItemCreate(
	_Inout_ struct _AML_STATE*           State,
	_In_    AML_STATE_SNAPSHOT_ITEM_TYPE Type,
	_In_    AML_STATE_SNAPSHOT_ITEM_DATA Data
	);

//
// Raise the reference counter of a snapshot item.
//
VOID
AmlStateSnapshotItemReference(
	_Inout_ AML_STATE_SNAPSHOT_ITEM* Item
	);

//
// Lower the reference counter of a snapshot item.
//
VOID
AmlStateSnapshotItemRelease(
	_Inout_ AML_STATE_SNAPSHOT_ITEM* Item
	);

//
// Push a performed action to the current snapshot frame of the given item.
//
_Success_( return )
BOOLEAN
AmlStateSnapshotItemPushAction(
	_Inout_opt_ AML_STATE_SNAPSHOT_ITEM* Item,
	_In_        BOOLEAN                  IsRaise
	);

//
// Begin a full state snapshot stack entry.
//
_Success_( return )
BOOLEAN
AmlStateSnapshotBegin(
	_Inout_ struct _AML_STATE* State
	);

//
// Commit the previously pushed state snapshot entry, allows all state updates/actions to remain performed.
//
VOID
AmlStateSnapshotCommit(
	_Inout_ struct _AML_STATE* State,
	_In_    BOOLEAN            RestoreEvalState
	);

//
// Pop the last state snapshot stack entry and rllback any state snapshot actions that happened during it.
//
VOID
AmlStateSnapshotRollback(
	_Inout_ struct _AML_STATE* State
	);

//
// Create a namespace node as part of the current snapshot state.
//
_Success_( return )
BOOLEAN
AmlStateSnapshotCreateNode(
	_Inout_     struct _AML_STATE*             State,
	_Inout_opt_ struct _AML_NAMESPACE_SCOPE*   ActiveScope,
	_In_        const struct _AML_NAME_STRING* Name,
	_Outptr_    struct _AML_NAMESPACE_NODE**   ppCreatedNode
	);

//
// Create a new reference counted buffer data resource within the current snapshot.
//
_Success_( return != NULL )
AML_BUFFER_DATA*
AmlStateSnapshotCreateBufferData(
	_Inout_ struct _AML_STATE* State,
	_Inout_ struct _AML_HEAP*  Heap,
	_In_    SIZE_T             Size,
	_In_    SIZE_T             MaxSize
	);
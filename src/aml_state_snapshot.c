#include "aml_state.h"
#include "aml_mutex.h"
#include "aml_debug.h"
#include "aml_namespace.h"
#include "aml_state_snapshot.h"

//
//                          each item frame corresponds to an                                                                
//                          action performed during the linked                                                               
//                          snapshot stack level's lifetime.                                                          
//                                                                                                                    
//                                 item 0 frame list                        item 0 frame 0 action list                
//                              +---+-------------+----------------------------------------------------+              
//                              |   | use counter |frame 0                 | inc/dec frame use counter |              
//          item n              |   | action list |            ------------+---------------------------+              
//    +-------------------------+   +-------------+-----------/            |           ....            |              
//    | underlying data |           |    ....     |frame (1..n-1)          +---------------------------+              
//    | cur level index |           |    ....     |                                                                   
//    +-------------------------+   +-------------+                                    ....                           
//    | underlying data |       |   | use counter |frame n                                                            
//    | cur level index |       |   | action list |             (all item frames have their own separate action lists)
//    +-----------------+-----+ |   +-------------+                   item (1..n) frame (0..n) action lists           
//    | underlying data |     | |  item 1 frame list                      +---------------------------+               
//    | cur level index |     | +---+-------------+-----------------------+ inc/dec frame use counter |               
//    +-----------------+     |     | use counter |frame 0    +-----------+---------------------------+               
//  items reference a state   |     | action list |           |   +-------|           ....            |               
//  datum like an allocation, |     +-------------+-----------+   | +-----+------|--------------------+               
//  mutex, object, etc.       |     |    ....     |frame (1..n-1) | |     |      |                                    
//                            |     |    ....     |               | | +---+      |                                    
//                            |     +-------------+---------------+ | |          |                                    
//                            |     | use counter |frame n          | | +--------+                                    
//                            |     | action list |                 | | | an action either increases or decreases     
//                            |     +-------------+                 | | | the linked item frame's use counter.        
//                            |    item 2 frame list                | | | when a use counter is above 0 at the end    
//                            +-----+-------------+-----------------+ | | of a snapshot, the item is considered still 
//                                  | use counter |frame 0            | | live, and must be released accordingly if   
//                                  | action list |                   | | the snapshot is rolled back.                
//                                  +-------------+-------------------+ |                                             
//                                  |    ....     |frame (1..n-1)       |                                             
//                                  |    ....     |                     |                                             
//                                  +-------------+---------------------+                                             
//                                  | use counter |frame n                                                            
//                                  | action list |                                                                   
//                                  +-------------+                                                                   
//                 rolling back a snapshot will automatically release                                                 
//                 any item data for all linked frames with live data.                                                
//

//
// Allocate and initialize a new state snapshot item referencing an actionable piece of data.
//
_Success_( return != NULL )
AML_STATE_SNAPSHOT_ITEM*
AmlStateSnapshotItemCreate(
	_Inout_ struct _AML_STATE*           State,
	_In_    AML_STATE_SNAPSHOT_ITEM_TYPE Type,
	_In_    AML_STATE_SNAPSHOT_ITEM_DATA Data
	)
{
	AML_STATE_SNAPSHOT_ITEM* Item;

	//
	// Allocate and initialize a new snapshot item.
	// This maintains all snapshot metadata for an associated piece of state data.
	//
	if( ( Item = AmlHeapAllocate( &State->Heap, sizeof( *Item ) ) ) == NULL ) {
		return NULL;
	}
	*Item = ( AML_STATE_SNAPSHOT_ITEM ){ .Valid = 1, .State = State, .Type = Type, .u = Data, .ReferenceCount = 1 };
	return Item;
}

//
// Raise the reference counter of a snapshot item.
//
VOID
AmlStateSnapshotItemReference(
	_Inout_ AML_STATE_SNAPSHOT_ITEM* Item
	)
{
	if( ++Item->ReferenceCount == 0 ) {
		AML_TRAP_STRING( "State snapshot item reference counter overflow!" );
	}
}

//
// Lower the reference counter of a snapshot item.
//
VOID
AmlStateSnapshotItemRelease(
	_Inout_ AML_STATE_SNAPSHOT_ITEM* Item
	)
{
	if( Item->ReferenceCount-- == UINT64_MAX ) {
		AML_TRAP_STRING( "State snapshot item reference counter underflow!" );
	} else if( Item->ReferenceCount == 0 ) {
		AmlHeapFree( &Item->State->Heap, Item );
	}
}

//
// Push a performed action to the current snapshot frame of the given item.
//
_Success_( return )
BOOLEAN
AmlStateSnapshotItemPushAction(
	_Inout_opt_ AML_STATE_SNAPSHOT_ITEM* Item,
	_In_        BOOLEAN                  IsRaise
	)
{
	AML_STATE*                     State;
	AML_STATE_SNAPSHOT_ACTION*     Action;
	AML_STATE_SNAPSHOT_ITEM_FRAME* Frame;
	AML_STATE_SNAPSHOT*            Snapshot;

	//
	// The given snapshot item state must be initialized and valid.
	// It is possible for not all objects to be properly set up for snapshots.
	//
	if( ( Item == NULL ) || ( Item->State == NULL ) ) {
		return AML_TRUE;
	}

	//
	// Must currently be within a snapshot.
	//
	State = Item->State;
	if( ( State->StateSnapshotTail == NULL ) || ( State->StateSnapshotLevelIndex == 0 ) ) {
		return AML_FALSE;
	}

	//
	// If the current state snapshot level differs from the last snapshot that the item was referenced in,
	// we must allocate and set up a new snapshot frame for the item. If the last snapshot level of the item
	// is greater than the current level index, this indicates that something has gone wrong. All item frames
	// should be popped when exiting a snapshot scope (via rollback or commit).
	//
	if( State->StateSnapshotLevelIndex > Item->LevelIndex ) {
		Frame = AmlArenaAllocate( &State->StateSnapshotArena, sizeof( *Frame ) );
		if( Frame == NULL ) {
			return AML_FALSE;
		}
		*Frame = ( AML_STATE_SNAPSHOT_ITEM_FRAME ){ .Item = Item, .LevelIndex = State->StateSnapshotLevelIndex };
		AmlStateSnapshotItemReference( Item );
		AML_DEBUG_TRACE(
			State,
			"Creating item (%p) frame (%p) for level index (%"PRId64") (SSLI: %"PRId64")\n",
			Item, Frame, Frame->LevelIndex, State->StateSnapshotTail->LevelIndex
		);

		//
		// Push the new frame to the item's frame stack.
		//
		Frame->FramePrevious = Item->FrameLast;
		if( Item->FrameLast != NULL ) {
			Item->FrameLast->FrameNext = Frame;
		}
		Item->FrameLast = Frame;
		Item->FrameFirst = ( ( Item->FrameFirst != NULL ) ? Item->FrameFirst : Frame );
		Item->LevelIndex = State->StateSnapshotLevelIndex;

		//
		// Push the new frame to the snapshot level's frame stack.
		//
		Snapshot = State->StateSnapshotTail;
		Frame->SnapFramePrevious = Snapshot->FrameLast;
		if( Snapshot->FrameLast != NULL ) {
			Snapshot->FrameLast->SnapFrameNext = Frame;
		}
		Snapshot->FrameLast = Frame;
		Snapshot->FrameFirst = ( ( Snapshot->FrameFirst != NULL ) ? Snapshot->FrameFirst : Frame );
	} else if( State->StateSnapshotLevelIndex < Item->LevelIndex ) {
		return AML_FALSE;
	}

	//
	// We must always push to the last frame of the item.
	//
	if( Item->FrameLast == NULL ) {
		return AML_FALSE;
	}

	//
	// Allocate and initialize a new action entry for the given item.
	//
	Action = AmlArenaAllocate( &State->StateSnapshotArena, sizeof( *Action ) );
	if( Action == NULL ) {
		return AML_FALSE;
	}
	*Action = ( AML_STATE_SNAPSHOT_ACTION ){ .Item = Item, .IsRaise = IsRaise };
	AmlStateSnapshotItemReference( Item );

	//
	// Link a new action to the tail of the frame's action list.
	//
	Frame = Item->FrameLast;
	if( Frame->ActionLast != NULL ) {
		Frame->ActionLast->Next = Action;
	}
	Frame->ActionLast  = Action;
	Frame->ActionFirst = ( Frame->ActionFirst ? Frame->ActionFirst : Action );
	return AML_TRUE;
}

//
// Begin a full state snapshot stack entry.
//
_Success_( return )
BOOLEAN
AmlStateSnapshotBegin(
	_Inout_ struct _AML_STATE* State
	)
{
	AML_ARENA_SNAPSHOT  StateArenaSnapshot;
	AML_STATE_SNAPSHOT* StateSnapshot;

	//
	// Attempt to allocate all resources for the new snapshot stack level.
	//
	StateArenaSnapshot = AmlArenaSnapshot( &State->StateSnapshotArena );
	StateSnapshot = AmlArenaAllocate( &State->StateSnapshotArena, sizeof( *StateSnapshot ) );
	if( StateSnapshot == NULL ) {
		AML_DEBUG_FATAL( State, "Fatal: State snapshot backing arena allocation failed!" );
		return AML_FALSE;
	}

	//
	// Default initialize state snapshot and backup part of the state.
	//
	State->StateSnapshotLevelIndex += 1;
	*StateSnapshot = ( AML_STATE_SNAPSHOT ){
		.ArenaSnapshot      = StateArenaSnapshot,
		.LevelIndex         = State->StateSnapshotLevelIndex,
		.NamespaceScopeLast = State->Namespace.ScopeLast,
	};

	//
	// Backup the current pass state.
	// TODO: We also need to backup and restore the namespace scope!
	//
	StateSnapshot->PassType                 = State->PassType;
	StateSnapshot->MethodScopeLevel         = State->MethodScopeLevel;
	StateSnapshot->MethodScopeLevelStart    = State->MethodScopeLevelStart;
	StateSnapshot->Data                     = State->Data;
	StateSnapshot->DataTotalLength          = State->DataTotalLength;
	StateSnapshot->DataLength               = State->DataLength;
	StateSnapshot->DataCursor               = State->DataCursor;
	StateSnapshot->WhileLoopLevel           = State->WhileLoopLevel;
	StateSnapshot->PendingInterruptionEvent = State->PendingInterruptionEvent;

	//
	// Push a new snapshot level to the stack.
	//
	StateSnapshot->StatePrevious = State->StateSnapshotTail;
	if( State->StateSnapshotTail != NULL ) {
		State->StateSnapshotTail->StateNext = StateSnapshot;
	}
	if( State->StateSnapshotHead == NULL ) {
		State->StateSnapshotHead = StateSnapshot;
	}
	State->StateSnapshotTail = StateSnapshot;
	return AML_TRUE;
}

//
// Commit the previously pushed state snapshot entry, allows all state updates/actions to remain performed.
//
VOID
AmlStateSnapshotCommit(
	_Inout_ struct _AML_STATE* State,
	_In_    BOOLEAN            RestoreEvalState
	)
{
	AML_STATE_SNAPSHOT*            Snapshot;
	AML_STATE_SNAPSHOT_ITEM_FRAME* Frame;
	AML_STATE_SNAPSHOT_ITEM*       Item;
	SIZE_T                         PopCount;
	SIZE_T                         i;

	//
	// We can only ever operate on the last snapshot created.
	//
	Snapshot = State->StateSnapshotTail;
	if( ( Snapshot == NULL ) || ( State->StateSnapshotLevelIndex == 0 ) ) {
		return;
	}

	//
	// All linked items need to have their frames corresponding to this snapshot popped.
	//
	for( Frame = Snapshot->FrameFirst; Frame != NULL; Frame = Frame->SnapFrameNext ) {
		//
		// The snapshot must correspond to the last frame pushed for this item,
		// otherwise the stacks have somehow become desynchronized.
		//
		Item = Frame->Item;
		if( ( Frame != Item->FrameLast ) || ( State->StateSnapshotLevelIndex != Item->LevelIndex ) ) {
			AML_DEBUG_PANIC( State, "Fatal: State snapshot stacks desynchronized!" );
			return;
		}

		//
		// Pop the frame from the item's frame stack.
		//
		if( Frame->FramePrevious != NULL ) {
			Frame->FramePrevious->FrameNext = Frame->FrameNext;
		}
		if( Frame->FrameNext != NULL ) {
			Frame->FrameNext->FramePrevious = Frame->FramePrevious;
		}
		Item->FrameFirst = ( ( Item->FrameFirst != Frame ) ? Item->FrameFirst : NULL );
		Item->FrameLast = ( ( Item->FrameLast != Frame ) ? Item->FrameLast : Frame->FramePrevious );

		//
		// Update the current highest referenced level index of the item.
		//
		if( Item->FrameLast != NULL ) {
			Item->LevelIndex = Item->FrameLast->LevelIndex;
		} else {
			Item->LevelIndex = 0;
		}

		//
		// All item snapshot frames hold a reference to their parent item, release it.
		//
		AmlStateSnapshotItemRelease( Item );
	}

	//
	// Remove the snapshot from the state snapshot list.
	//
	if( Snapshot->StatePrevious != NULL ) {
		Snapshot->StatePrevious->StateNext = Snapshot->StateNext;
	}
	if( Snapshot->StateNext != NULL ) {
		Snapshot->StateNext->StatePrevious = Snapshot->StatePrevious;
	}
	State->StateSnapshotHead = ( ( State->StateSnapshotHead != Snapshot ) ? State->StateSnapshotHead : NULL );
	State->StateSnapshotTail = ( ( State->StateSnapshotTail != Snapshot ) ? State->StateSnapshotTail : Snapshot->StatePrevious );

	//
	// Decrease the total nested snapshot level index.
	//
	State->StateSnapshotLevelIndex -= 1;

	//
	// The user may only want to restore the eval state while committing all actions.
	// For instance, when loading a separate table of AML code, or invoking a function natively.
	// TODO: Restore namespace state.
	//
	if( RestoreEvalState ) {
		//
		// The code executed within the snapshot should never be able to pop the scope out of the bounds of the snapshot.
		//
		if( ( State->MethodScopeLevel < Snapshot->MethodScopeLevel )
			|| ( State->MethodScopeLevelStart < Snapshot->MethodScopeLevelStart ) )
		{
			AML_DEBUG_PANIC( State, "Fatal: Code within a snapshot must never pop scope beyond the beginning of the snapshot!" );
			return;
		}

		//
		// Attempt to rollback the method scope to that of the snapshot.
		//
		State->MethodScopeLevelStart = Snapshot->MethodScopeLevelStart;
		PopCount = ( State->MethodScopeLevel - Snapshot->MethodScopeLevel );
		for( i = 0; i < PopCount; i++ ) {
			if( AmlMethodPopScope( State ) == AML_FALSE ) {
				AML_DEBUG_PANIC( State, "Fatal: Snapshot method scope restore pop failed!" );
				return;
			}
		}

		//
		// Attempt to rollback the namespace scope to that of the snapshot.
		// TODO: Add a mechanism like MethodScopeLevelStart to ensure that the code
		// within the snapshot never pops past the scope level of the snapshot.
		//
		while( State->Namespace.ScopeLast != Snapshot->NamespaceScopeLast ) {
			if( AmlNamespacePopScope( &State->Namespace ) == AML_FALSE ) {
				AML_DEBUG_PANIC( State, "Fatal: Namespace scope stack out of sync!" );
				return;
			}
		}

		//
		// Restore the rest of the pass state.
		//
		State->PassType                 = Snapshot->PassType;
		State->MethodScopeLevel         = Snapshot->MethodScopeLevel;
		State->MethodScopeLevelStart    = Snapshot->MethodScopeLevelStart;
		State->Data                     = Snapshot->Data;
		State->DataTotalLength          = Snapshot->DataTotalLength;
		State->DataLength               = Snapshot->DataLength;
		State->DataCursor               = Snapshot->DataCursor;
		State->WhileLoopLevel           = Snapshot->WhileLoopLevel;
		State->PendingInterruptionEvent = Snapshot->PendingInterruptionEvent;
	}

	//
	// Rollback the state snapshot arena to release all internal snapshot memory.
	//
	AmlArenaSnapshotRollback( &State->StateSnapshotArena, &Snapshot->ArenaSnapshot );
}

//
// Pop the last state snapshot stack entry and rllback any state snapshot actions that happened during it.
//
VOID
AmlStateSnapshotRollback(
	_Inout_ struct _AML_STATE* State
	)
{
	AML_STATE_SNAPSHOT*            Snapshot;
	AML_STATE_SNAPSHOT_ACTION*     Action;
	AML_STATE_SNAPSHOT_ITEM*       Item;
	AML_STATE_SNAPSHOT_ITEM_FRAME* Frame;
	UINT64                         i;

	//
	// We can only ever operate on the last snapshot created.
	//
	Snapshot = State->StateSnapshotTail;
	if( ( Snapshot == NULL ) || ( State->StateSnapshotLevelIndex == 0 ) ) {
		return;
	}

	//
	// Process queued actions and build the final reference counter for all items.
	// Adjusts the reference counter of the item based on the action raise type.
	// For example, referencing an object raises, dereferencing lowers.
	// TODO: Check against counter overflow/underflow.
	//
	for( Frame = Snapshot->FrameFirst; Frame != NULL; Frame = Frame->SnapFrameNext ) {
		for( Action = Frame->ActionFirst; Action != NULL; Action = Action->Next ) {
			if( Action->Item->Valid ) {
				AML_DEBUG_TRACE(
					State,
					"Processing action for item (%p) frame (%p) level (%"PRId64") (%"PRId64", SSLI: %"PRId64")\n",
					Action->Item, Frame, Action->Item->LevelIndex, Snapshot->LevelIndex, State->StateSnapshotLevelIndex
				);
			}

			//
			// Validate the action properties and ensure that the action doesn't lead to a counter overflow.
			//
			if( ( Action->Item->LevelIndex != Snapshot->LevelIndex ) || ( Action->Item->FrameLast == NULL ) ) {
				AML_DEBUG_PANIC( State, "Fatal: Snapshot action item level index mismatch, stack out of sync!" );
				return;
			} else if( ( Action->IsRaise != AML_FALSE ) && ( Action->Item->FrameLast->Counter >= UINT64_MAX ) ) {
				AML_DEBUG_PANIC( State, "Fatal: Snapshot action item use counter overflow!" );
				return;
			}

			//
			// Raise or lower the item's use counter.
			// Skip lowering the counter if the counter for this frame is already at 0,
			// this is skipped gracefully because it is possible to release an object
			// that was referenced outside of the current snapshot scope.
			//
			if( Action->Item->Valid && ( Action->IsRaise || ( Action->Item->FrameLast->Counter > 0 ) ) ) {
				Action->Item->FrameLast->Counter += ( Action->IsRaise ? 1ull : ~0ull );
			}

			//
			// All snapshot action entries raise the reference counter of the item, release our reference.
			//
			AmlStateSnapshotItemRelease( Action->Item );
		}
	}

	//
	// Process all queued items and release any that haven't had their reference counts decreased back to 0 during the snapshot.
	// TODO: Does release order matter for any items?
	//
	for( Frame = Snapshot->FrameFirst; Frame != NULL; Frame = Frame->SnapFrameNext ) {
		Item = Frame->Item;
		if( ( Item == NULL ) || ( Item->Valid == 0 ) ) {
			continue;
		}

		//
		// If the item being rolled back has outstanding references,
		// perform the type-specific release call for the underlying item datum.
		//
		for( i = 0; i < Frame->Counter; i++ ) {
			if( ( Item == NULL ) || ( Item->Valid == 0 ) ) {
				break;
			}
#ifndef AML_BUILD_NO_SNAPSHOT_ITEMS
			switch( Item->Type ) {
			case AML_STATE_SNAPSHOT_ITEM_TYPE_BUFFER:
				AML_DEBUG_TRACE( State, "Snapshot [%"PRId64"] releasing namespace buffer: %p\n", Frame->Item->LevelIndex, Item->u.Buffer );
				AmlBufferDataRelease( Item->u.Buffer );
				break;
			case AML_STATE_SNAPSHOT_ITEM_TYPE_PACKAGE:
				AmlPackageDataRelease( Item->u.Package );
				break;
			case AML_STATE_SNAPSHOT_ITEM_TYPE_NODE:
				AML_DEBUG_TRACE( State, "Snapshot [%"PRId64"] releasing namespace node: \"", Frame->Item->LevelIndex ); 
				AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_TRACE, &Item->u.Node->AbsolutePath );
				AML_DEBUG_TRACE( State, "\"\n" );
				AmlNamespaceReleaseNode( &State->Namespace, Item->u.Node );
				break;
			case AML_STATE_SNAPSHOT_ITEM_TYPE_MUTEX:
				AmlMutexRelease( State, Item->u.Mutex );
				break;
			default:
				AML_DEBUG_ERROR( State, "Error: Unsupported state snapshot item type!" );
				break;
			}
#endif
		}
	}

	//
	// Use commit to pop and free the snapshot level.
	//
	AmlStateSnapshotCommit( State, AML_TRUE );
}

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
	)
{
	AML_NAMESPACE_NODE*          Node;
	AML_STATE_SNAPSHOT_ITEM_DATA ItemData;

	//
	// Create the base namespace node before setting up a state item for it.
	//
	if( AmlNamespaceCreateNode( &State->Namespace, ActiveScope, Name, &Node ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Create a snapshot item for this node, allows the node to be referenced in snapshot actions.
	//
	ItemData = ( AML_STATE_SNAPSHOT_ITEM_DATA ){ .Node = Node };
	Node->StateItem = AmlStateSnapshotItemCreate( State, AML_STATE_SNAPSHOT_ITEM_TYPE_NODE, ItemData );
	if( Node->StateItem == NULL ) {
		AmlNamespaceReleaseNode( &State->Namespace, Node );
		return AML_FALSE;
	}

	//
	// Push the initial reference action to the current snapshot.
	//
	if( AmlStateSnapshotItemPushAction( Node->StateItem, AML_TRUE ) == AML_FALSE ) {
		AmlNamespaceReleaseNode( &State->Namespace, Node );
		return AML_FALSE;
	}

	*ppCreatedNode = Node;
	return AML_TRUE;
}

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
	)
{
	AML_BUFFER_DATA*             Buffer;
	AML_STATE_SNAPSHOT_ITEM_DATA ItemData;

	//
	// Create the initial buffer data allocation.
	//
	if( ( Buffer = AmlBufferDataCreate( Heap, Size, MaxSize ) ) == NULL ) {
		return NULL;
	}

	//
	// Create a snapshot item for this buffer, allows the buffer to be referenced in snapshot actions.
	//
	ItemData = ( AML_STATE_SNAPSHOT_ITEM_DATA ){ .Buffer = Buffer };
	Buffer->StateItem = AmlStateSnapshotItemCreate( State, AML_STATE_SNAPSHOT_ITEM_TYPE_BUFFER, ItemData );
	if( Buffer->StateItem == NULL ) {
		AmlBufferDataRelease( Buffer );
		return NULL;
	}

	//
	// Push the initial reference action to the current snapshot.
	//
	if( AmlStateSnapshotItemPushAction( Buffer->StateItem, AML_TRUE ) == AML_FALSE ) {
		AmlBufferDataRelease( Buffer );
		return NULL;
	}

	return Buffer;
}
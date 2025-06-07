#include "aml_arena.h"

//
// Allow the user to override the arena implementation with their own.
// Public structs/functions in this header must be defined/implemented by the user.
//
#ifndef AML_BUILD_OVERRIDE_ARENA

//
// Private arena chunk header.
//
typedef struct _AML_ARENA_CHUNK {
    struct _AML_ARENA_CHUNK* Next;
    struct _AML_ARENA_CHUNK* Previous;
    SIZE_T                   UsedSize;
    SIZE_T                   TotalSize;
    SIZE_T                   AllocationSize;
    _Alignas( AML_ARENA_MIN_ALIGNMENT )
    _Field_size_part_( TotalSize, UsedSize ) UINT8 Data[ 0 ];
} AML_ARENA_CHUNK;

//
// Minimum allocation alignment must be a power of 2.
//
_Static_assert(
    ( ( AML_ARENA_MIN_ALIGNMENT & ( AML_ARENA_MIN_ALIGNMENT - 1 ) ) == 0 ),
    "Minimum allocation alignment must be a power of 2."
    );

//
// Align a pointer upwards by the given alignment (must be a power of 2).
//
#define AML_ARENA_PTR_ALIGN_UP(Value, AlignmentPow2) \
    (VOID*)((((UINT_PTR)(Value)) + ((AlignmentPow2) - 1)) & ~((AlignmentPow2) - 1))

//
// Link a chunk to the tail of the given list.
// Note: the given chunk should not already be linked to a list or any other entries!
//
static
VOID
AmlArenaChunkListInsert(
    _Inout_ AML_ARENA_CHUNK_LIST* List,
    _Inout_ AML_ARENA_CHUNK*      Entry
    )
{
    //
    // Link the chunk to the tail of the list.
    //
    if( List->Last != NULL ) {
        List->Last->Next = Entry;
    }
    Entry->Previous = List->Last;
    List->Last = Entry;

    //
    // Update the head of the list if the list is empty.
    //
    List->First = ( ( List->First == NULL ) ? Entry : List->First );
}

//
// Unlink a chunk from the given list.
//
static
VOID
AmlArenaChunkListRemove(
    _Inout_ AML_ARENA_CHUNK_LIST* List,
    _Inout_ AML_ARENA_CHUNK*      Entry
    )
{
    //
    // Unlink the chunk from its previous link.
    //
    if( Entry->Previous != NULL ) {
        Entry->Previous->Next = Entry->Next;
    }

    //
    // Unlink the chunk from its next link.
    //
    if( Entry->Next != NULL ) {
        Entry->Next->Previous = Entry->Previous;
    }

    //
    // If the entry is the current head of the list, update list head.
    //
    if( List->First == Entry ) {
        List->First = Entry->Next;
    }

    //
    // If the entry is the current tail of the list, update list tail.
    //
    if( List->Last == Entry ) {
        List->Last = Entry->Previous;
    }

    //
    // Reset entry links to unlinked state.
    //
    Entry->Previous = NULL;
    Entry->Next = NULL;
}

//
// Initialize an arena that allocates ontop of the given backend allocator.
//
VOID
AmlArenaInitialize(
    _Out_ AML_ARENA*    Arena,
    _In_  AML_ALLOCATOR Backend,
    _In_  SIZE_T        ChunkSize,
    _In_  UINT32        Flags
    )
{
    //
    // Initialize base fields.
    //
    *Arena = ( AML_ARENA ){
        //
        // Initialize the backend allocator used to allocate arena chunk memory.
        //
        .Backend = Backend,
        
        //
        // Initialize user-provided configuration.
        //
        .BaseChunkSize = ChunkSize
    };
}

//
// Attempt to reclaim a chunk from the chunk free list (allows resetting an existing arena).
//
_Success_( return != NULL )
static
AML_ARENA_CHUNK*
AmlArenaTryReclaimFreeChunk(
    _Inout_ AML_ARENA* Arena,
    _In_    SIZE_T     MinimumSize
    )
{
    AML_ARENA_CHUNK* Chunk;

    //
    // Attempt to pop an entry from the free list.
    // This pops from the tail end of the list, as if we have increased the
    // new chunk size, the tail will usually be the biggest free chunk.
    // If the free list is empty, the entry will point to the list head.
    //
    for( Chunk = Arena->ChunkFreeList.Last; Chunk != NULL; Chunk = Chunk->Previous ) {
        //
        // Skip this entry if it isn't big enough for the given required size.
        // Note: this is not a common case, and may only happen if the arena is suboptimally configured, 
        // and is serving an individual allocation size that exceeds the base chunk size of the arena.
        //
        if( Chunk->TotalSize < MinimumSize ) {
            continue;
        }

        //
        // Reset chunk metadata.
        //
        Chunk->UsedSize = 0;

        //
        // Unlink and return the chunk from the free list.
        // Note: for any future editing sake, the chunk loop *must not* continue after this, as Chunk->Previous will now be NULL.
        //
        AmlArenaChunkListRemove( &Arena->ChunkFreeList, Chunk );
        return Chunk;
    }

    return NULL;
}

//
// Allocate an entirely new chunk using the backend allocator.
//
_Success_( return != NULL )
static
AML_ARENA_CHUNK*
AmlArenaBackendAllocateNewChunk(
    _Inout_ AML_ARENA* Arena,
    _In_    SIZE_T     ChunkDataSize
    )
{
    AML_ARENA_CHUNK* Chunk;
    SIZE_T           AllocationSize;

    //
    // Ensure that the given data capacity summed with the chunk header size does not overflow.
    //
    if( ChunkDataSize >= ( SIZE_MAX - sizeof( AML_ARENA_CHUNK ) ) ) {
        return NULL;
    }

    //
    // Attempt to allocate the backing memory for a new chunk using the user-supplied allocator.
    //
    AllocationSize = ( sizeof( AML_ARENA_CHUNK ) + ChunkDataSize );
    if( ( Chunk = Arena->Backend.Allocate( Arena->Backend.Context, AllocationSize ) ) == NULL ) {
        return NULL;
    }

    //
    // Initialize persistent chunk metadata.
    //
    Chunk->Next           = NULL;
    Chunk->Previous       = NULL;
    Chunk->UsedSize       = 0;
    Chunk->TotalSize      = ChunkDataSize;
    Chunk->AllocationSize = AllocationSize;
    return Chunk;
}

//
// Attempt to reclaim a free chunk, if not possible, allocate and initialize new chunk using the backend allocator.
//
_Success_( return != NULL )
static
AML_ARENA_CHUNK*
AmlArenaCreateChunk(
    _Inout_ AML_ARENA* Arena,
    _In_    SIZE_T     MinimumSize
    )
{
    AML_ARENA_CHUNK* Chunk;
    SIZE_T           ChunkDataSize;

    //
    // Determine the new chunk's data size.
    // TODO: Add other methods, exponential increase mode, etc.
    //
    ChunkDataSize = AML_MAX( MinimumSize, Arena->BaseChunkSize );

    //
    // Ensure that the given data capacity summed with the chunk header size does not overflow.
    //
    if( ChunkDataSize >= ( SIZE_MAX - sizeof( AML_ARENA_CHUNK ) ) ) {
        return NULL;
    }

    //
    // If this is a large allocation, meaning that it exceeds the base chunk size
    // due to suboptimal configuration of the arena (individual allocation size above base chunk size),
    // then it is handled specially, without considering the free list, to avoid linear search for a big enough entry.
    //
    if( ChunkDataSize <= Arena->BaseChunkSize ) {
        Chunk = AmlArenaTryReclaimFreeChunk( Arena, MinimumSize );
    } else {
        Chunk = NULL;
    }

    //
    // If we have failed to reclaim a free chunk, we must allocate a new one using the backend allocator.
    //
    if( Chunk == NULL ) {
        if( ( Chunk = AmlArenaBackendAllocateNewChunk( Arena, ChunkDataSize ) ) == NULL ) {
            return NULL;
        }
    }

    //
    // Reset used chunk data size.
    //
    Chunk->UsedSize = 0;

    //
    // Reset global information about the current chunk.
    //
    Arena->CurrentChunk = Chunk;

    //
    // Add the chunk to the chunk list.
    //
    AmlArenaChunkListInsert( &Arena->ChunkList, Chunk );
    return Chunk;
}

//
// Reset used space of the arena allocator,
// allows chunks to be re-used again.
// Frees all large private allocations.
//
VOID
AmlArenaReset(
    _Inout_ AML_ARENA* Arena
    )
{
    AML_ARENA_CHUNK_LIST* ActiveList;
    AML_ARENA_CHUNK_LIST* FreeList;
    AML_ARENA_CHUNK*      TransplantHead;

    //
    // Initialize shorthand pointers to chunk lists.
    //
    ActiveList = &Arena->ChunkList;
    FreeList = &Arena->ChunkFreeList;

    //
    // Move all active chunks to the reclaimable free chunk list.
    //
    if( ActiveList->First != NULL ) {
        //
        // Add all active chunks into the tail of the free chunk list.
        //
        TransplantHead = ActiveList->First;
        TransplantHead->Previous = FreeList->Last;
        if( FreeList->Last != NULL ) {
            FreeList->Last->Next = TransplantHead;
        }
        FreeList->First = ( ( FreeList->First != NULL ) ? FreeList->First : TransplantHead );
        FreeList->Last = ActiveList->Last;

        //
        // Reset active chunk list links.
        //
        ActiveList->First = NULL;
        ActiveList->Last = NULL;
    }

    //
    // Reset the current chunk information.
    //
    Arena->CurrentChunk = NULL;
}

//
// Free all ARENA_CHUNKs in the given doubly-linked list.
// Resets the given list.
//
static
VOID
AmlArenaFreeChunkList(
    _In_    AML_ARENA*            Arena,
    _Inout_ AML_ARENA_CHUNK_LIST* List
    )
{
    AML_ARENA_CHUNK* Chunk;

    //
    // Free all list entries.
    //
    while( List->First != NULL ) {
        Chunk = List->First;
        List->First = Chunk->Next;
        Arena->Backend.Free( Arena->Backend.Context, Chunk, Chunk->AllocationSize );
    }

    //
    // Reset list head/tail links.
    //
    List->First = NULL;
    List->Last = NULL;
}

//
// Free all backing memory allocated by the arena, and reset arena chunk state.
//
VOID
AmlArenaRelease(
    _Inout_ AML_ARENA* Arena
    )
{
    Arena->CurrentChunk = NULL;

    //
    // Free all chunk list entries, and reset all chunk lists.
    //
    AmlArenaFreeChunkList( Arena, &Arena->ChunkList );
    AmlArenaFreeChunkList( Arena, &Arena->ChunkFreeList );
}

//
// Allocate memory using the given arena.
//
_Success_( return != NULL )
VOID*
AmlArenaAllocate(
    _Inout_ AML_ARENA* Arena,
    _In_    SIZE_T     DataSize
    )
{
    SIZE_T  ChunkFreeSpace;
    BOOLEAN ChunkAllocatable;
    SIZE_T  AllocationSize;
    UINT8*  AllocationData;

    //
    // Ensure that adding required space for alignment does not overflow.
    //
    if( DataSize >= ( SIZE_MAX - AML_ARENA_MIN_ALIGNMENT ) ) {
        return NULL;
    }

    //
    // Total required allocation size to align the allocation base.
    // TODO: Don't add alignment if chunk data is already aligned!!!
    //
    AllocationSize = ( DataSize + AML_ARENA_MIN_ALIGNMENT );
    ChunkAllocatable = AML_FALSE;

    //
    // Determine if we should linearly allocate from the current existing chunk.
    //
    if( Arena->CurrentChunk != NULL ) {
        ChunkFreeSpace = ( Arena->CurrentChunk->TotalSize - Arena->CurrentChunk->UsedSize );
        ChunkAllocatable = ( ChunkFreeSpace >= AllocationSize );
    }

    //
    // If we weren't able to linearly allocate from the current chunk, we must create/reclaim a new one.
    //
    if( ChunkAllocatable == AML_FALSE ) {
        if( ( Arena->CurrentChunk = AmlArenaCreateChunk( Arena, AllocationSize ) ) == NULL ) {
            return NULL;
        }
        ChunkFreeSpace = ( Arena->CurrentChunk->TotalSize - Arena->CurrentChunk->UsedSize );
        ChunkAllocatable = ( ChunkFreeSpace >= AllocationSize );
    }

    //
    // Should never happen, left to ensure safety when modifying this code in the future.
    //
    if( ChunkAllocatable == AML_FALSE ) {
        return NULL;
    }

    //
    // Linearly allocate from the current chunk.
    //
    AllocationData                 = &Arena->CurrentChunk->Data[ Arena->CurrentChunk->UsedSize ];
    Arena->CurrentChunk->UsedSize += AllocationSize;

    //
    // Align up the allocation base by the minimum memory allocator alignment.
    //
    return AML_ARENA_PTR_ALIGN_UP( AllocationData, AML_ARENA_MIN_ALIGNMENT );
}

//
// Allocate memory using the given arena and zero initialize it.
//
_Success_( return != NULL )
VOID*
AmlArenaAllocateZeroInitialized(
    _Inout_ AML_ARENA* Arena,
    _In_    SIZE_T     DataSize
    )
{
    VOID* Allocation;

    //
    // Allocate and zero initialize.
    //
    if( ( Allocation = AmlArenaAllocate( Arena, DataSize ) ) == NULL ) {
        return NULL;
    }
    AML_MEMSET( Allocation, 0, DataSize );
    return Allocation;
}

//
// Allocate memory using the given arena, and copy the contents of the input buffer to it.
//
_Success_( return != NULL )
VOID*
AmlArenaAllocateCopy(
    _Inout_                      AML_ARENA*  Arena,
    _In_reads_bytes_( DataSize ) const VOID* DataInput,
    _In_                         SIZE_T      DataSize
    )
{
    VOID* Allocation;

    //
    // Allocate and copy.
    //
    if( ( Allocation = AmlArenaAllocate( Arena, DataSize ) ) == NULL ) {
        return NULL;
    }
    AML_MEMCPY( Allocation, DataInput, DataSize );
    return Allocation;
}

//
// Take a snapshot of the current arena, must be committed or rolled back.
//
AML_ARENA_SNAPSHOT
AmlArenaSnapshot(
    _Inout_ AML_ARENA* Arena
    )
{
    return ( AML_ARENA_SNAPSHOT ) {
        .Index         = Arena->SnapshotCount++,
        .Chunk         = Arena->CurrentChunk,
        .ChunkUsedSize = ( ( Arena->CurrentChunk != NULL ) ? Arena->CurrentChunk->UsedSize : 0 ),
    };
}

//
// Rollback arena state to the given snapshot.
// Renders any allocations made after this snapshot point invalid!
// Note: Snapshots must be committed and rolled back in the proper order.
//
_Success_( return )
BOOLEAN
AmlArenaSnapshotRollback(
    _Inout_ AML_ARENA*                Arena,
    _In_    const AML_ARENA_SNAPSHOT* Snapshot
    )
{
    AML_ARENA_CHUNK_LIST* ActiveList;
    AML_ARENA_CHUNK_LIST* FreeList;
    AML_ARENA_CHUNK*      TransplantHead;

    //
    // Snapshots must be rolled back in the proper order.
    //
    if( Snapshot->Index != ( Arena->SnapshotCount - 1 ) ) {
        AML_TRAP_STRING( "Arena snapshot pop order mismatch!" );
        return AML_FALSE;
    }

    //
    // If we are completely resetting the arena, move all chunks back to the free list.
    //
    if( Snapshot->Chunk == NULL ) {
        //
        // Move all active chunks to the reclaimable free chunk list.
        //
        AmlArenaReset( Arena );

        //
        // Rollback chunk/snapshot information to no current chunk.
        //
        Arena->CurrentChunk   = NULL;
        Arena->SnapshotCount -= 1;
        return AML_TRUE;
    }

    //
    // Initialize shorthand pointers to chunk lists.
    //
    ActiveList = &Arena->ChunkList;
    FreeList = &Arena->ChunkFreeList;

    //
    // Unlink all chunks that have been allocated *after* this snapshot from the active list,
    // but keep them linked together, so we can transplant all of them to the free list at once.
    //
    if( Snapshot->Chunk->Next != NULL ) {
        TransplantHead = Snapshot->Chunk->Next;
        if( TransplantHead->Previous != NULL ) {
            TransplantHead->Previous->Next = NULL;
            TransplantHead->Previous = NULL;
        }
        ActiveList->First = ( ( ActiveList->First != TransplantHead ) ? ActiveList->First : NULL );
        ActiveList->Last = ( ( ActiveList->Last != TransplantHead ) ? ActiveList->Last : NULL );

        //
        // Move any chunks that have been allocated after this snapshot to the free list.
        //
        if( FreeList->Last != NULL ) {
            TransplantHead->Previous = FreeList->Last;
            FreeList->Last->Next = TransplantHead;
        }
        FreeList->First = ( ( FreeList->First != NULL ) ? FreeList->First : TransplantHead );
        FreeList->Last = ActiveList->Last;
    }

    //
    // Rollback chunk/snapshot information.
    //
    Arena->CurrentChunk           = Snapshot->Chunk;
    Arena->CurrentChunk->UsedSize = Snapshot->ChunkUsedSize;
    Arena->SnapshotCount         -= 1;
    return AML_TRUE;
}

//
// Commit the current snapshot.
// Note: Snapshots must be committed and rolled back in the proper order.
//
_Success_( return )
BOOLEAN
AmlArenaSnapshotCommit(
    _Inout_ AML_ARENA*                Arena,
    _In_    const AML_ARENA_SNAPSHOT* Snapshot
    )
{
    //
    // Snapshots must be committed in the proper order.
    //
    if( Snapshot->Index != ( Arena->SnapshotCount - 1 ) ) {
        AML_TRAP_STRING( "Arena snapshot pop order mismatch!" );
        return AML_FALSE;
    }
    Arena->SnapshotCount -= 1;
    return AML_TRUE;
}

#endif
#pragma once

#include "aml_platform.h"
#include "aml_allocator.h"

//
// Allow the user to override the arena implementation with their own.
// Public structs/functions in this header must be defined/implemented by the user.
//
#ifndef AML_BUILD_OVERRIDE_ARENA

//
// Minimum alignment for the start of the arena chunk data, and currently the start of all arena chunk sub-allocations.
// TODO: Allow alignment to be specified on a per-sub-allocation basis.
//
#define AML_ARENA_MIN_ALIGNMENT 16

//
// Arena snapshot state.
//
typedef struct _AML_ARENA_SNAPSHOT {
    UINT64                   Index;
    SIZE_T                   ChunkUsedSize;
    struct _AML_ARENA_CHUNK* Chunk;
} AML_ARENA_SNAPSHOT;

//
// Arena doubly linked chunk list.
//
typedef struct _AML_ARENA_CHUNK_LIST {
    struct _AML_ARENA_CHUNK* First;
    struct _AML_ARENA_CHUNK* Last;
} AML_ARENA_CHUNK_LIST;

//
// Chunk freelist arena allocator.
//
typedef struct _AML_ARENA {
    //
    // The backend allocator used to allocate arena chunks.
    //
    AML_ALLOCATOR Backend;

    //
    // Chunk information/configuration.
    //
    SIZE_T                   BaseChunkSize;
    AML_ARENA_CHUNK_LIST     ChunkList;
    AML_ARENA_CHUNK_LIST     ChunkFreeList;
    struct _AML_ARENA_CHUNK* CurrentChunk;

    //
    // Snapshot stack state.
    //
    SIZE_T SnapshotCount;
} AML_ARENA;

#endif

//
// Initialize an arena allocator that allocates ontop of the given backend allocator.
//
VOID
AmlArenaInitialize(
    _Out_ AML_ARENA*          Arena,
    _In_  const AML_ALLOCATOR Backend,
    _In_  SIZE_T              ChunkSize,
    _In_  UINT32              Flags
    );

//
// Allocate memory using the given arena.
//
_Success_( return != NULL )
VOID*
AmlArenaAllocate(
    _Inout_ AML_ARENA* Arena,
    _In_    SIZE_T     DataSize
    );

//
// Allocate memory using the given arena and zero initialize it.
//
_Success_( return != NULL )
VOID*
AmlArenaAllocateZeroInitialized(
    _Inout_ AML_ARENA* Arena,
    _In_    SIZE_T     DataSize
    );

//
// Allocate memory using the given arena, and copy the contents of the input buffer to it.
//
_Success_( return != NULL )
VOID*
AmlArenaAllocateCopy(
    _Inout_                      AML_ARENA*  Arena,
    _In_reads_bytes_( DataSize ) const VOID* DataInput,
    _In_                         SIZE_T      DataSize
    );

//
// Reset used space of the arena allocator,
// allows chunks to be re-used again.
// Frees all large private allocations.
//
VOID
AmlArenaReset(
    _Inout_ AML_ARENA* Arena
    );

//
// Free all backing memory allocated by the arena.
//
VOID
AmlArenaRelease(
    _Inout_ AML_ARENA* Arena
    );

//
// Take a snapshot of the current arena, must be committed or rolled back.
//
AML_ARENA_SNAPSHOT
AmlArenaSnapshot(
    _Inout_ AML_ARENA* Arena
    );

//
// Rollback arena state to the given snapshot.
// Renders any allocations made after this snapshot point invalid!
// Note: Snapshots must be committed in the proper order.
//
_Success_( return )
BOOLEAN
AmlArenaSnapshotRollback(
    _Inout_ AML_ARENA*                Arena,
    _In_    const AML_ARENA_SNAPSHOT* Snapshot
    );

//
// Commit the current snapshot.
// Note: Snapshots must be committed in the proper order.
//
_Success_( return )
BOOLEAN
AmlArenaSnapshotCommit(
    _Inout_ AML_ARENA*                Arena,
    _In_    const AML_ARENA_SNAPSHOT* Snapshot
    );
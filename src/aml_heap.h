#pragma once

#include "aml_platform.h"
#include "aml_arena.h"

//
// SIZE_T bit count used by AML_HEAP.
//
#define AML_HEAP_SIZE_BITS ( sizeof( SIZE_T ) * 8 )

//
// Simple binned heap allocator chunk header.
//
typedef struct _AML_HEAP_BIN_ENTRY {
    struct _AML_HEAP_BIN_ENTRY* Next;
    SIZE_T                      DataSize;
    _Alignas( 16 ) UINT8        Data[ 0 ];
} AML_HEAP_BIN_ENTRY;

//
// Simple binned heap allocator for modifiable object values.
//
typedef struct _AML_HEAP {
    AML_ARENA*          Arena;
    AML_HEAP_BIN_ENTRY* Bins[ ( AML_HEAP_SIZE_BITS + 1 ) ];
} AML_HEAP;

//
// Initialize an AML heap allocator.
//
VOID
AmlHeapInitialize(
    _Out_ AML_HEAP*  Heap,
    _In_  AML_ARENA* BackingArena
    );

//
// Allocate a block of memory of Size or larger.
// The returned allocation is freed using AmlHeapFree.
//
_Success_( return != NULL )
VOID*
AmlHeapAllocate(
    _Inout_ AML_HEAP* Heap,
    _In_    SIZE_T    Size
    );

//
// Free an allocated block of memory returned by AmlHeapFree.
// Should never be called multiple times on the same allocation.
//
VOID
AmlHeapFree(
    _Inout_          AML_HEAP* Heap,
    _In_ _Frees_ptr_ VOID*     AllocationData
    );
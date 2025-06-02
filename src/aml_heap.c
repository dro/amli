#include "aml_heap.h"
#include <stdlib.h>

//
// TODO: Improve heap allocator, add more bins inbetween pow2 values, implement splitting/coalescing of halves.
//

//
// Initialize an AML heap allocator.
//
VOID
AmlHeapInitialize(
	_Out_ AML_HEAP*  Heap,
	_In_  AML_ARENA* BackingArena
	)
{
	*Heap = ( AML_HEAP ){ .Arena = BackingArena };
}

//
// Calculate the bin index that the given size fulls under.
// Values will be placed into the tightest fitting pow2 bit indexed bin.
//
static
SIZE_T
AmlHeapBinIndexForSize(
	_Inout_ AML_HEAP* Heap,
	_In_    SIZE_T    Size
	)
{
	SIZE_T LzCount;

	//
	// Calculate the closest bin index that equals or encompasses the input size.
	// The result of LZCNT64 must be clamped to the maximum heap size bit count,
	// for the case where AML_HEAP_SIZE_BITS < 64.
	//
	if( Size >= ( ( SIZE_T )1 << ( AML_HEAP_SIZE_BITS - 1 ) ) ) {
		return ( AML_HEAP_SIZE_BITS - 1 );
	} else if( Size == 0 ) {
		return 0;
	}
	LzCount = AML_LZCNT64( AML_MIN( Size, 2 ) );
	LzCount = AML_MIN( LzCount, AML_HEAP_SIZE_BITS );
	return ( AML_HEAP_SIZE_BITS - LzCount );
}

//
// Allocate a block of memory of Size or larger.
// The returned allocation is freed using AmlHeapFree.
//
_Success_( return != NULL )
VOID*
AmlHeapAllocate(
	_Inout_ AML_HEAP* Heap,
	_In_    SIZE_T    Size
	)
{
	SIZE_T              BinIndex;
	AML_HEAP_BIN_ENTRY* BinEntry;
	SIZE_T              BlockSize;

#if AML_BUILD_FUZZER
	return malloc( Size );
#endif

	//
	// Get closest fitting upward bin for the given size
	// and round up size to the bin-size for this index.
	//
	_Static_assert(
		AML_HEAP_SIZE_BITS <= AML_COUNTOF( Heap->Bins ),
		"Insufficient heap bin count for platform SIZE_T"
	);
	BinIndex = AmlHeapBinIndexForSize( Heap, Size );
	Size = ( ( SIZE_T )1 << BinIndex );

	//
	// If there is an existing freelist entry in the bin, pop it and use it.
	//
	if( ( BinEntry = Heap->Bins[ BinIndex ] ) != NULL ) {
		BinEntry->Next = NULL;
		Heap->Bins[ BinIndex ] = BinEntry->Next;
		return BinEntry->Data;
	}

	//
	// Allocate a new heap block for the given size.
	//
	if( Size > ( SIZE_MAX - sizeof( *BinEntry ) ) ) {
		return NULL;
	}
	BlockSize = ( sizeof( *BinEntry ) + Size );
	if( ( BinEntry = AmlArenaAllocate( Heap->Arena, BlockSize ) ) == NULL ) {
		return NULL;
	}

	//
	// Initialize block header.
	//
	BinEntry->Next     = NULL;
	BinEntry->DataSize = Size;

	//
	// Return allocated block data.
	//
	return &BinEntry->Data[ 0 ];
}

//
// Free an allocated block of memory returned by AmlHeapFree.
// Should never be called multiple times on the same allocation.
//
VOID
AmlHeapFree(
	_Inout_          AML_HEAP* Heap,
	_In_ _Frees_ptr_ VOID*     AllocationData
	)
{
	AML_HEAP_BIN_ENTRY* BinEntry;
	SIZE_T              LzCount;
	SIZE_T              BinIndex;

#ifdef AML_BUILD_FUZZER
	free( AllocationData );
	return;
#endif

	//
	// Get the chunk header of the given allocation data.
	// Note: this is only valid if AllocationData was returned by AmlHeapAllocate.
	//
	BinEntry = AML_CONTAINING_RECORD( AllocationData, AML_HEAP_BIN_ENTRY, Data );

	//
	// Get closest fitting bin for the allocation block size.
	//
	_Static_assert(
		AML_HEAP_SIZE_BITS <= AML_COUNTOF( Heap->Bins ),
		"Insufficient heap bin count for platform SIZE_T"
	);
	BinIndex = AmlHeapBinIndexForSize( Heap, BinEntry->DataSize );

	//
	// Add the block to the free-list of the fitting bin.
	//
	BinEntry->Next = Heap->Bins[ BinIndex ];
	Heap->Bins[ BinIndex ] = BinEntry;
}
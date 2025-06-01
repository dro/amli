#include "aml_heap.h"

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
	SIZE_T              LzCount;
	SIZE_T              BinIndex;
	AML_HEAP_BIN_ENTRY* BinEntry;
	SIZE_T              BlockSize;

	//
	// Get closest fitting upward bin for the given size
	// and round up size to the bin-size for this index.
	//
	_Static_assert(
		AML_HEAP_SIZE_BITS <= AML_COUNTOF( Heap->Bins ),
		"Insufficient heap bin count for platform SIZE_T"
	);
	LzCount  = ( Size ? AML_LZCNT64( Size ) : ( AML_HEAP_SIZE_BITS - 1 ) );
	LzCount  = AML_MIN( LzCount, AML_HEAP_SIZE_BITS );
	BinIndex = ( AML_HEAP_SIZE_BITS - LzCount );
	Size     = ( ( SIZE_T )1 << BinIndex );

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
	SIZE_T              BinIndex;

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
	BinIndex = ( AML_HEAP_SIZE_BITS - AML_LZCNT64( BinEntry->DataSize ) );

	//
	// Add the block to the free-list of the fitting bin.
	//
	BinEntry->Next = Heap->Bins[ BinIndex ];
	Heap->Bins[ BinIndex ] = BinEntry;
}
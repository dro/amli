#include "aml_hash.h"

//
// Allows the user to override the hashing implementation with their own.
//
#ifndef AML_BUILD_OVERRIDE_HASH

//
// Read murmur3 input block.
// If platform/implementation requires endian swapping, do it here.
//
static
UINT32
AmlMurmur3GetBlock(
    _In_ const UCHAR* Data,
    _In_ SIZE_T       BlockIndex
    )
{
    UINT32 BlockValue;

    //
    // Avoid unaligned/aliased/UB access to the input data by using memcpy instead of a UINT32 dereference.
    //
    AML_MEMCPY(
        &BlockValue,
        &Data[ BlockIndex * sizeof( UINT32 ) ],
        sizeof( BlockValue )
    );

    return BlockValue;
}

//
// Force all bits of a 32-bit hash sum to avalanche.
//
static
UINT32
AmlMurmur3FinalizationMix32(
    _In_ UINT32 Hash
    )
{
    Hash ^= ( Hash >> 16 );
    Hash *= 0x85ebca6b;
    Hash ^= ( Hash >> 13 );
    Hash *= 0xc2b2ae35;
    Hash ^= ( Hash >> 16 );
    return Hash;
}

//
// Calculate murmur3 32-bit hash of input key using given seed.
//
static
UINT32
AmlMurmur3Hash32(
    _In_reads_bytes_( KeyLength ) const VOID* Key,
    _In_                          SIZE_T      KeyLength,
    _In_                          UINT32      Seed
    )
{
    const UCHAR*  Data;
    SIZE_T        BlockCount;
    UINT32        K1;
    UINT32        Hash;
    SIZE_T        i;
    const UCHAR*  DataTail;

    //
    // Key the hash with the seed.
    //
    Hash = Seed;

    //
    // Convert KeyLength to block count.
    //
    BlockCount = ( KeyLength / sizeof( UINT32 ) );

    //
    // Body, hash multiples of 32 bits.
    //
    Data = Key;
    for( i = 0; i < BlockCount; i++ ) {
        K1    = AmlMurmur3GetBlock( Data, i );
        K1   *= 0xcc9e2d51;
        K1    = AML_ROL32( K1, 15 );
        K1   *= 0x1b873593;
        Hash ^= K1;
        Hash  = AML_ROL32( Hash, 13 );
        Hash  = Hash * 5 + 0xe6546b64;
    }

    //
    // Tail, hash remaining data byte-by-byte.
    //
    K1       = 0;
    DataTail = &Data[ BlockCount * sizeof( UINT32 ) ];

    //
    // Hash up to 3 tail bytes.
    //
    switch( KeyLength & 3 ) {
    case 3:
        K1 ^= ( DataTail[ 2 ] << 16 );
    case 2:
        K1 ^= ( DataTail[ 1 ] << 8 );
    case 1:
        K1   ^= DataTail[ 0 ];
        K1   *= 0xcc9e2d51;
        K1    = AML_ROL32( K1, 15 );
        K1   *= 0x1b873593;
        Hash ^= K1;
    }

    //
    // Finalize and mix hash.
    //
    Hash ^= KeyLength;
    Hash  = AmlMurmur3FinalizationMix32( Hash );
    return Hash;
}

//
// Calculate a 32-bit hash sum of the full input key using the given seed.
//
UINT32
AmlHashKey32(
    _In_reads_bytes_( KeyLength ) const VOID* Key,
    _In_                          SIZE_T      KeyLength,
    _In_                          UINT32      Seed
    )
{
    return AmlMurmur3Hash32( Key, KeyLength, Seed );
}

#endif
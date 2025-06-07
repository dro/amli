#include "aml_platform.h"

//
// Simple naive memcpy for builds that don't define their own.
//
VOID*
AmlMemoryCopy(
    _Out_writes_bytes_all_( Size ) VOID*       Destination,
    _In_reads_bytes_( Size )       const VOID* Source,
    _In_                           SIZE_T      Size
    )
{
    SIZE_T i;

    //
    // Naively copy byte-by-byte, shouldn't violate strict aliasing when using UCHAR.
    //
    for( i = 0; i < Size; i++ ) {
        ( ( UCHAR* )Destination )[ i ] = ( ( const UCHAR* )Source )[ i ];
    }

    return Destination;
}

//
// Simple naive memset for builds that don't define their own.
//
VOID* 
AmlMemorySet(
    _Out_writes_bytes_all_( Size ) VOID*  Destination,
    _In_                           INT    Value,
    _In_                           SIZE_T Size
    )
{
    SIZE_T i;

    //
    // Naively set byte-by-byte, shouldn't violate strict aliasing when using UCHAR.
    //
    for( i = 0; i < Size; i++ ) {
        ( ( UCHAR* )Destination )[ i ] = ( UCHAR )Value;
    }

    return Destination;
}

//
// RotateLeft32 helper for platforms where an intrinsic couldn't be found.
//
UINT32
AmlRotateLeft32(
    _In_ UINT32 Value,
    _In_ UINT32 Shift
    )
{
    return ( ( Value << Shift ) | ( Value >> ( 32 - Shift ) ) );
}
#pragma once

#include "aml_platform.h"

//
// User-provided allocator interface callback to allocate memory.
//
typedef
_Success_( return != NULL )
VOID*
( *AML_MEMORY_ALLOCATE )(
    _Inout_ VOID*  Context,
    _In_    SIZE_T Size
    );

//
// User-provided allocator interface callback to free memory previously allocated using AML_MEMORY_ALLOCATE.
//
typedef
_Success_( return )
BOOLEAN
( *AML_MEMORY_FREE )(
    _Inout_          VOID*  Context,
    _In_ _Frees_ptr_ VOID*  Allocation,
    _In_             SIZE_T AllocationSize
    );

//
// Memory allocator interface.
//
typedef struct _AML_ALLOCATOR {
    VOID*               Context;
    AML_MEMORY_ALLOCATE Allocate;
    AML_MEMORY_FREE     Free;
} AML_ALLOCATOR;
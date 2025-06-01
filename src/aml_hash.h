#pragma once

#include "aml_platform.h"

//
// Calculate a 32-bit hash sum of the full input key using the given seed.
//
UINT32
AmlHashKey32(
	_In_reads_bytes_( KeyLength ) const VOID* Key,
	_In_                          SIZE_T      KeyLength,
	_In_                          UINT32      Seed
	);
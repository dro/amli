#pragma once

#include "aml_platform.h"

//
// Read bit-granularity data from the given buffer field to the result data array.
// The given ResultDataSize is the max size of the ResultData array in *bytes*.
//
_Success_( return )
BOOLEAN
AmlBufferFieldRead(
	_In_                                 const struct _AML_STATE*         State,
	_In_                                 struct _AML_OBJECT_BUFFER_FIELD* Field,
	_Out_writes_bytes_( ResultDataSize ) VOID*                            ResultData,
	_In_                                 SIZE_T                           ResultDataSize,
	_In_                                 BOOLEAN                          AllowTruncation,
	_Out_opt_                            SIZE_T*                          pResultBitCount
	);

//
// Write data to the given buffer field from the input data array.
// The maximum input data size is passed in bytes, but the actual copy to the field is done at bit-granularity.
//
_Success_( return )
BOOLEAN
AmlBufferFieldWrite(
	_In_                              const struct _AML_STATE*         State,
	_In_                              struct _AML_OBJECT_BUFFER_FIELD* Field,
	_In_reads_bytes_( InputDataSize ) const VOID*                      InputData,
	_In_                              SIZE_T                           InputDataSize,
	_In_                              BOOLEAN                          AllowTruncation
	);
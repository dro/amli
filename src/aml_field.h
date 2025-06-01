#pragma once

#include "aml_platform.h"

//
// Read data from a field unit object (field, buffer field, index field, bank field).
//
_Success_( return )
BOOLEAN
AmlFieldUnitRead(
	_Inout_                              struct _AML_STATE*  State,
	_Inout_                              struct _AML_OBJECT* FieldObject,
	_Out_writes_bytes_( ResultDataSize ) VOID*               ResultData,
	_In_                                 SIZE_T              ResultDataSize,
	_In_                                 BOOLEAN             AllowTruncation,
	_Out_opt_                            SIZE_T*             pResultBitCount
	);

//
// Write data to a field unit object (field, buffer field, index field, bank field).
// AllowTruncation has different semantics for OpRegion field writes, if enabled, when writing to a field that is
// greater in size than the input data, the upper bits of the field will be written as 0, instead of following
// the update rule of the field!
//
_Success_( return )
BOOLEAN
AmlFieldUnitWrite(
	_Inout_                           struct _AML_STATE*  State,
	_Inout_                           struct _AML_OBJECT* FieldObject,
	_In_reads_bytes_( InputDataSize ) const VOID*         InputData,
	_In_                              SIZE_T              InputDataSize,
	_In_                              BOOLEAN             AllowTruncation
	);
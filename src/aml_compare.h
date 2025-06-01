#pragma once

#include "aml_platform.h"
#include "aml_data.h"

//
// Types of AML logical comparisons.
//
typedef enum _AML_COMPARISON_TYPE {
	AML_COMPARISON_TYPE_EQUAL,
	AML_COMPARISON_TYPE_LESS,
	AML_COMPARISON_TYPE_LESS_EQUAL,
	AML_COMPARISON_TYPE_GREATER,
	AML_COMPARISON_TYPE_GREATER_EQUAL,
} AML_COMPARISON_TYPE;

//
// Result of an AML comparison, should only ever be returned to AML code if not ERROR.
//
#define AML_COMPARISON_RESULT_AML_TRUE  (~0ull)
#define AML_COMPARISON_RESULT_AML_FALSE (0ull)
#define AML_COMPARISON_RESULT_ERROR (1ull)

//
// Compare integer value to another (implcitly converted) integer value.
//
_Success_( return != AML_COMPARISON_RESULT_ERROR )
UINT64
AmlCompareInteger(
	_Inout_ struct _AML_STATE*  State,
	_Inout_ AML_HEAP*           TempHeap,
	_In_    AML_COMPARISON_TYPE ComparisonType,
	_In_    const AML_DATA*     Operand1,
	_In_    const AML_DATA*     Operand2
	);

//
// Lexicographic compare raw byte span to another raw byte span.
//
_Success_( return != AML_COMPARISON_RESULT_ERROR )
UINT64
AmlCompareRawBytes(
	_In_                            const struct _AML_STATE* State,
	_Inout_                         AML_HEAP*                TempHeap,
	_In_                            AML_COMPARISON_TYPE      ComparisonType,
	_In_reads_bytes_( LhsDataSize ) const UINT8*             LhsData,
	_In_                            SIZE_T                   LhsDataSize,
	_In_reads_bytes_( RhsDataSize ) const UINT8*             RhsData,
	_In_                            SIZE_T                   RhsDataSize
	);

//
// Compare string value to another (implcitly converted) string value.
//
_Success_( return != AML_COMPARISON_RESULT_ERROR )
UINT64
AmlCompareString(
	_Inout_ struct _AML_STATE*  State,
	_Inout_ AML_HEAP*           TempHeap,
	_In_    AML_COMPARISON_TYPE ComparisonType,
	_In_    const AML_DATA*     Operand1,
	_In_    const AML_DATA*     Operand2
	);

//
// Compare buffer value to another (implcitly converted) buffer value.
//
_Success_( return != AML_COMPARISON_RESULT_ERROR )
UINT64
AmlCompareBuffer(
	_Inout_ struct _AML_STATE*  State,
	_Inout_ AML_HEAP*           TempHeap,
	_In_    AML_COMPARISON_TYPE ComparisonType,
	_In_    const AML_DATA*     Operand1,
	_In_    const AML_DATA*     Operand2
	);

//
// Compare field unit value to another (implcitly converted) integer value.
//
_Success_( return != AML_COMPARISON_RESULT_ERROR )
UINT64
AmlCompareFieldUnit(
	_Inout_ struct _AML_STATE*  State,
	_Inout_ AML_HEAP*           TempHeap,
	_In_    AML_COMPARISON_TYPE ComparisonType,
	_In_    const AML_DATA*     Operand1,
	_In_    const AML_DATA*     Operand2
	);

//
// Compare two data values, the comparison type is dictated by the type of the first operand,
// and the second operand is implicitly converted to the type of the first operand.
// The first operand must be a valid comparable type (integer, string, buffer).
//
_Success_( return != AML_COMPARISON_RESULT_ERROR )
UINT64
AmlCompareData(
	_Inout_ struct _AML_STATE*  State,
	_Inout_ AML_HEAP*           TempHeap,
	_In_    AML_COMPARISON_TYPE ComparisonType,
	_In_    const AML_DATA*     Operand1,
	_In_    const AML_DATA*     Operand2
	);
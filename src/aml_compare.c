#include "aml_state.h"
#include "aml_decoder.h"
#include "aml_compare.h"
#include "aml_conv.h"

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
    )
{
    AML_DATA ConvOperand2;
    UINT64   IntegerSizeMask;
    UINT64   Value1;
    UINT64   Value2;
    UINT64   Result;

    //
    // First operand must already have been deduced to be an integer.
    //
    if( Operand1->Type != AML_DATA_TYPE_INTEGER ) {
        return AML_COMPARISON_RESULT_ERROR;
    }

    //
    // The data type of Source1 dictates the required type of Source2.
    // Source2 is implicitly converted if necessary to match the type of Source1.
    // TODO: Only convert if the second operand isn't already the same type.
    //
    ConvOperand2 = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER };
    if( AmlConvObjectStore( State, TempHeap, Operand2, &ConvOperand2, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
        return AML_COMPARISON_RESULT_ERROR;
    }

    //
    // Get both integer operand values (masked to the revision's integer size).
    //
    IntegerSizeMask = ( State->IsIntegerSize64 ? UINT64_MAX : UINT32_MAX );
    Value1 = ( Operand1->u.Integer & IntegerSizeMask );
    Value2 = ( ConvOperand2.u.Integer & IntegerSizeMask );

    //
    // Compare integer values.
    //
    switch( ComparisonType ) {
    case AML_COMPARISON_TYPE_EQUAL:
        Result = ( ( Value1 == Value2 ) ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE );
        break;
    case AML_COMPARISON_TYPE_LESS:
        Result = ( ( Value1 < Value2 ) ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE );
        break;
    case AML_COMPARISON_TYPE_LESS_EQUAL:
        Result = ( ( Value1 <= Value2 ) ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE );
        break;
    case AML_COMPARISON_TYPE_GREATER:
        Result = ( ( Value1 > Value2 ) ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE );
        break;
    case AML_COMPARISON_TYPE_GREATER_EQUAL:
        Result = ( ( Value1 >= Value2 ) ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE );
        break;
    default:
        Result = AML_COMPARISON_RESULT_ERROR;
        break;
    }

    //
    // Free any temporary converted value data.
    //
    AmlDataFree( &ConvOperand2 );
    return Result;
}

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
    )
{
    UINT64 Result;
    SIZE_T i;
    UINT8  Byte1;
    UINT8  Byte2;
    INT16  ByteCmp;

    //
    // Default match equality to false, and byte-wise comparison to successful (for case of 0-sized data).
    //
    Result = AML_COMPARISON_RESULT_AML_FALSE;
    ByteCmp = 0;

    //
    // Perform a byte-wise comparison of the two spans.
    // Skip for equality comparisons if sizes don't match.
    // ByteCmp  < 0: (Byte1 < Byte2)
    // Bytecmp  > 0: (Byte1 > Byte2)
    // ByteCmp == 0: (Byte1 == Byte2)
    //
    if( ( ComparisonType != AML_COMPARISON_TYPE_EQUAL )
        || ( LhsDataSize == RhsDataSize ) )
    {
        for( i = 0; i < AML_MIN( LhsDataSize, RhsDataSize ); i++ ) {
            _Analysis_assume_( ( i < LhsDataSize ) && ( i < RhsDataSize ) );
            Byte1 = LhsData[ i ];
            Byte2 = RhsData[ i ];
            ByteCmp = ( ( INT16 )Byte1 - ( INT16 )Byte2 );
            if( ByteCmp != 0 ) {
                break;
            }
        }
    } else {
        ByteCmp = -1; /* Less/greater doesn't matter for regular equal comparison. */
    }

    //
    // Perform all types of comparisons.
    //
    switch( ComparisonType ) {
    case AML_COMPARISON_TYPE_EQUAL:
        //
        // True is returned only if both lengths are the same and the result of a byte-wise compare indicates exact equality.
        //
        Result = ( ( ( LhsDataSize == RhsDataSize ) && ( ByteCmp == 0 ) )
                   ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE );
        break;
    case AML_COMPARISON_TYPE_GREATER:
    case AML_COMPARISON_TYPE_GREATER_EQUAL:
        //
        // True is returned if a byte-wise (unsigned) compare discovers at least one byte in Source1 that is
        // numerically greater than the corresponding byte in Source2. 
        // False is returned if at least one byte in Source1 is numerically less than the corresponding byte in Source2.
        // 
        // In the case of byte-wise equality, True is returned if the length of Source1 is greater than Source2,
        // False is returned if the length of Source1 is less than or equal to Source2.
        //
        if( ByteCmp > 0 ) {
            Result = AML_COMPARISON_RESULT_AML_TRUE;
        } else if( ByteCmp < 0 ) {
            Result = AML_COMPARISON_RESULT_AML_FALSE;
        } else {
            if( ComparisonType == AML_COMPARISON_TYPE_GREATER_EQUAL ) {
                Result = ( ( LhsDataSize >= RhsDataSize )
                           ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE );
            } else {
                Result = ( ( LhsDataSize > RhsDataSize )
                           ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE );
            }
        }
        break;
    case AML_COMPARISON_TYPE_LESS:
    case AML_COMPARISON_TYPE_LESS_EQUAL:
        //
        // True is returned if a byte-wise (unsigned) compare discovers at least one byte in Source1 that is
        // numerically less than the corresponding byte in Source2.
        // False is returned if at least one byte in Source1 is numerically greater than the corresponding byte in Source2.
        // 
        // In the case of byte-wise equality, True is returned if the length of Source1 is less than Source2,
        // False is returned if the length of Source1 is greater than or equal to Source2.
        //
        if( ByteCmp < 0 ) {
            Result = AML_COMPARISON_RESULT_AML_TRUE;
        } else if( ByteCmp > 0 ) {
            Result = AML_COMPARISON_RESULT_AML_FALSE;
        } else {
            if( ComparisonType == AML_COMPARISON_TYPE_LESS_EQUAL ) {
                Result = ( ( LhsDataSize <= RhsDataSize )
                           ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE );
            } else {
                Result = ( ( LhsDataSize < RhsDataSize )
                           ? AML_COMPARISON_RESULT_AML_TRUE : AML_COMPARISON_RESULT_AML_FALSE );
            }
        }
        break;
    default:
        Result = AML_COMPARISON_RESULT_ERROR;
        break;
    }

    return Result;
}

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
    )
{
    AML_DATA ConvOperand2;
    UINT64   Result;

    //
    // First operand must already have been deduced to be a string.
    //
    if( Operand1->Type != AML_DATA_TYPE_STRING ) {
        return AML_COMPARISON_RESULT_ERROR;
    }

    //
    // The data type of Source1 dictates the required type of Source2.
    // Source2 is implicitly converted if necessary to match the type of Source1.
    // TODO: Only convert if the second operand isn't already the same type.
    //
    ConvOperand2 = ( AML_DATA ){ .Type = AML_DATA_TYPE_STRING };
    if( AmlConvObjectStore( State, TempHeap, Operand2, &ConvOperand2, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
        return AML_COMPARISON_RESULT_ERROR;
    }

    //
    // Perform lexicographic comparison of the raw bytes of both strings following
    // the semantics for string/buffer comparisons in the ACPI spec.
    //
    Result = AmlCompareRawBytes( State,
                                 TempHeap,
                                 ComparisonType,
                                 Operand1->u.String->Data,
                                 Operand1->u.String->Size,
                                 ConvOperand2.u.String->Data,
                                 ConvOperand2.u.String->Size );

    //
    // Free temporary converted second operand.
    //
    AmlDataFree( &ConvOperand2 );
    return Result;
}

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
    )
{
    AML_DATA ConvOperand2;
    UINT64   Result;

    //
    // First operand must already have been deduced to be a buffer.
    //
    if( Operand1->Type != AML_DATA_TYPE_BUFFER ) {
        return AML_COMPARISON_RESULT_ERROR;
    }

    //
    // The data type of Source1 dictates the required type of Source2.
    // Source2 is implicitly converted if necessary to match the type of Source1.
    // TODO: Only convert if the second operand isn't already the same type.
    //
    ConvOperand2 = ( AML_DATA ){ .Type = AML_DATA_TYPE_BUFFER };
    if( AmlConvObjectStore( State, TempHeap, Operand2, &ConvOperand2, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
        return AML_COMPARISON_RESULT_ERROR;
    }

    //
    // Perform lexicographic comparison of the raw bytes of both buffers following
    // the semantics for string/buffer comparisons in the ACPI spec.
    //
    Result = AmlCompareRawBytes( State,
                                 TempHeap,
                                 ComparisonType,
                                 Operand1->u.Buffer->Data,
                                 Operand1->u.Buffer->Size,
                                 ConvOperand2.u.Buffer->Data,
                                 ConvOperand2.u.Buffer->Size );

    //
    // Free temporary converted second operand.
    //
    AmlDataFree( &ConvOperand2 );
    return Result;
}

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
    )
{
    AML_DATA ConvValue;

    //
    // First operand must already have been deduced to be a field unit.
    //
    if( Operand1->Type != AML_DATA_TYPE_FIELD_UNIT ) {
        return AML_COMPARISON_RESULT_ERROR;
    }

    //
    // Convert the field unit to the highest priority conversion type (integer).
    //
    ConvValue = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER };
    if( AmlConvObjectStore( State, TempHeap, Operand1, &ConvValue, AML_CONV_FLAGS_IMPLICIT ) == AML_FALSE ) {
        return AML_COMPARISON_RESULT_ERROR;
    }

    //
    // Forward to the integer handler.
    //
    return AmlCompareInteger( State, TempHeap, ComparisonType, &ConvValue, Operand2 );
}

//
// Compare package element data value to another (implcitly converted) value.
//
_Success_( return != AML_COMPARISON_RESULT_ERROR )
static
UINT64
AmlComparePackageElement(
    _Inout_ struct _AML_STATE*  State,
    _Inout_ AML_HEAP*           TempHeap,
    _In_    AML_COMPARISON_TYPE ComparisonType,
    _In_    const AML_DATA*     Operand1,
    _In_    const AML_DATA*     Operand2
    )
{
    AML_PACKAGE_ELEMENT* Element;

    //
    // First operand must already have been deduced to be a package element.
    //
    if( Operand1->Type != AML_DATA_TYPE_PACKAGE_ELEMENT ) {
        return AML_COMPARISON_RESULT_ERROR;
    }

    //
    // Pass through the comparison to the actual underlying value of the package element.
    //
    Element = AmlPackageDataLookupElement( Operand1->u.PackageElement.Package, Operand1->u.PackageElement.ElementIndex );
    if( Element == NULL ) {
        return AML_COMPARISON_RESULT_ERROR;
    }
    return AmlCompareData( State, TempHeap, ComparisonType, &Element->Value, Operand2 );
}

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
    )
{
    switch( Operand1->Type ) {
    case AML_DATA_TYPE_INTEGER:
        return AmlCompareInteger( State, TempHeap, ComparisonType, Operand1, Operand2 );
    case AML_DATA_TYPE_STRING:
        return AmlCompareString( State, TempHeap, ComparisonType, Operand1, Operand2 );
    case AML_DATA_TYPE_BUFFER:
        return AmlCompareBuffer( State, TempHeap, ComparisonType, Operand1, Operand2 );
    case AML_DATA_TYPE_FIELD_UNIT:
        return AmlCompareFieldUnit( State, TempHeap, ComparisonType, Operand1, Operand2 );
    case AML_DATA_TYPE_PACKAGE_ELEMENT:
        return AmlComparePackageElement( State, TempHeap, ComparisonType, Operand1, Operand2 );
    default:
        break;
    }
    return AML_COMPARISON_RESULT_ERROR;
}
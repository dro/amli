#include "aml_eval.h"
#include "aml_eval_reference.h"
#include "aml_debug.h"

//
// Evaluate a RefOf instruction to an object.
//
_Success_( return )
BOOLEAN
AmlEvalRefOf(
    _Inout_  AML_STATE*   State,
    _In_     BOOLEAN      ConsumeOpcode,
    _Outptr_ AML_OBJECT** ppObject
    )
{
    AML_OBJECT* Object;
    AML_OBJECT* DestObject;

    //
    // DefRefOf := RefOfOp SuperName
    //
    if( ConsumeOpcode ) {
        if( AmlDecoderConsumeOpcode( State, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }
    if( AmlEvalSuperName( State, 0, 0, &DestObject ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Create a new temporary object for the created reference.
    //
    if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_NAME, &Object ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Create an immediate reference operand.
    // No reference is added here, as we pass on our reference returned by EvalSuperName.
    //
    Object->u.Name.Value = ( AML_DATA ){
        .Type        = AML_DATA_TYPE_REFERENCE,
        .u.Reference = { .Object = DestObject }
    };
    *ppObject = Object;
    return AML_TRUE;
}

//
// Evaluate DerefOf instruction to an object.
// DefDerefOf := DerefOfOp ObjReference
// ObjReference := TermArg => ObjectReference | String
//
_Success_( return )
BOOLEAN
AmlEvalDerefOf(
    _Inout_  AML_STATE*   State,
    _In_     BOOLEAN      ConsumeOpcode,
    _Outptr_ AML_OBJECT** ppObject
    )
{
    AML_DATA        TermArg;
    AML_OBJECT*     Object;
    AML_SIMPLE_NAME SimpleName;
    SIZE_T          i;

    //
    // DefDerefOf := DerefOfOp ObjReference
    // ObjReference := TermArg => ObjectReference | String
    //
    if( ConsumeOpcode ) {
        if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_DEREF_OF_OP, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }
    if( AmlEvalTermArg( State, ( AML_EVAL_TERM_ARG_FLAG_TEMP | AML_EVAL_TERM_ARG_FLAG_IS_DEREFOF ), &TermArg ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Target of a dereference must be an ObjectReference or a string.
    //
    switch( TermArg.Type ) {
    case AML_DATA_TYPE_REFERENCE:
        //
        // If the Source evaluates to an object reference, the actual contents of the object referred to are returned.
        //
        Object = TermArg.u.Reference.Object;
        AmlObjectReference( Object );
        break;
    case AML_DATA_TYPE_STRING:
        //
        // If the Source evaluates to a string, the string is evaluated as an ASL name
        // (relative to the current scope) and the contents of that object are returned.
        //
        SimpleName = ( AML_SIMPLE_NAME ){ .Type = AML_SIMPLE_NAME_TYPE_STRING };
        for( i = 0; i < TermArg.u.String->Size; i++ ) {
            if( ( TermArg.u.String->Data[ i ] != '\\' ) && ( TermArg.u.String->Data[ i ] != '^' ) ) {
                break;
            }
            SimpleName.u.NameString.Prefix.Data[ SimpleName.u.NameString.Prefix.Length++ ] = TermArg.u.String->Data[ i ];
        }
        SimpleName.u.NameString.SegmentCount = ( i / sizeof( AML_NAME_SEG ) );
        SimpleName.u.NameString.Segments = ( AML_NAME_SEG* )&TermArg.u.String->Data[ i ];

        //
        // Resolve operand and return it as the result of this function.
        //
        if( AmlResolveSimpleName( State, NULL, &SimpleName, 0, 0, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    default:
        AML_DEBUG_ERROR( State, "Error: Invalid target type of DerefOf, must be ObjectReference|String\n" );
        return AML_FALSE;
    }

    //
    // Free temporary deref target TermArg.
    //
    AmlDataFree( &TermArg );

    //
    // Return resolved object.
    //
    *ppObject = Object;
    return AML_TRUE;
}

//
// Evaluate IndexOp.
// BuffPkgStrObj := TermArg => Buffer, Package, or String
// IndexValue := TermArg => Integer
// DefIndex := IndexOp BuffPkgStrObj IndexValue Target
// 
// TODO: Fix up the object/data system, this function is a great showcase of how it is lacking.
//
_Success_( return )
BOOLEAN
AmlEvalIndex(
    _Inout_  AML_STATE*   State,
    _In_     BOOLEAN      ConsumeOpcode,
    _Outptr_ AML_OBJECT** ppObject
    )
{
    AML_DATA             Source;
    AML_DATA             NewSource;
    AML_DATA             Index;
    AML_OBJECT*          Target;
    AML_PACKAGE_ELEMENT* PackageElement;
    AML_OBJECT*          SourceObject;
    AML_OBJECT*          ReferenceObject;
    AML_OBJECT*          BufferField;
    AML_DATA             Reference;

    //
    // Consume the IndexOp byte if the caller hasn't done it for us.
    //
    if( ConsumeOpcode ) {
        if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_INDEX_OP, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    //
    // DefIndex := IndexOp BuffPkgStrObj IndexValue Target
    // Source is evaluated to a buffer, string, or package data type. Index is evaluated to an integer.
    // The reference to the nth object (where n = Index) within Source is optionally stored as a reference into Destination.
    //
    if( AmlEvalTermArg( State, AML_EVAL_TERM_ARG_FLAG_TEMP, &Source ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Index ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlEvalTarget( State, 0, &Target ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // We must follow package elements to their underlying value.
    // TODO: Figure out if there is anywhere else where this must be done, should only be relevant for reference type instructions.
    //
    if( Source.Type == AML_DATA_TYPE_PACKAGE_ELEMENT ) {
        PackageElement = AmlPackageElementIndexDataResolve( &Source.u.PackageElement );
        if( PackageElement == NULL ) {
            AML_DEBUG_ERROR( State, "Error: invalid index source operand package element.\n" );
            return AML_FALSE;
        } else if( AmlDataDuplicate( &PackageElement->Value, &State->Heap, &NewSource ) == AML_FALSE ) {
            return AML_FALSE;
        }
        AmlDataFree( &Source );
        Source = NewSource;
    }

    //
    // Must be a buffer, string, or package data type.
    //
    switch( Source.Type ) {
    case AML_DATA_TYPE_BUFFER:
    case AML_DATA_TYPE_STRING:
    case AML_DATA_TYPE_PACKAGE:
        break;
    default:
        AML_DEBUG_ERROR( State, "Error: invalid index source operand (must be buffer, string, package).\n" );
        return AML_FALSE;
    }

    //
    // Validate the given index against the size of the source.
    //
    if( Index.u.Integer >= AmlDataSizeOf( &Source ) ) {
        AML_DEBUG_ERROR( State, "Error: index value out of bounds.\n" );
        return AML_FALSE;
    }

    //
    // Create a new temporary internal object for the referenced source data.
    // TODO: This is hacky, should probably start combining reference counted data with object system.
    //
    if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_NAME, &SourceObject ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Create a temporary internal object for the returned reference.
    // TODO: This is hacky, should probably start combining reference counted data with object system.
    //
    if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_NAME, &ReferenceObject ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Setup the temporary source data object with the referenced source data.
    //
    SourceObject->u.Name = ( AML_OBJECT_NAME ){ 0 };
    switch( Source.Type ) {
    case AML_DATA_TYPE_BUFFER:
    case AML_DATA_TYPE_STRING:
        //
        // Create an internal buffer field object.
        //
        if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_BUFFER_FIELD, &BufferField ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Set up the internal buffer field object to reference the byte at the specified index.
        // TODO: Will need to change once duplicate/object/data system is refactored.
        //
        BufferField->u.BufferField = ( AML_OBJECT_BUFFER_FIELD ){
            .BitIndex  = ( Index.u.Integer * 8 ),
            .BitCount  = 8,
            .SourceBuf = Source
        };
        AmlBufferDataReference( Source.u.Buffer );

        //
        // Set up the temporary source object with a buffer field data value.
        //
        SourceObject->u.Name.Value = ( AML_DATA ){
            .Type = AML_DATA_TYPE_FIELD_UNIT,
            .u.FieldUnit = BufferField
        };

        //
        // Setup a reference to the created buffer field data value object.
        //
        Reference = ( AML_DATA ){
            .Type = AML_DATA_TYPE_REFERENCE,
            .u.Reference = {
                .Object = SourceObject,
            }
        };
        break;
    case AML_DATA_TYPE_PACKAGE:
        //
        // Set up the temporary source object with a package element data value.
        //
        SourceObject->u.Name.Value = ( AML_DATA ){
            .Type = AML_DATA_TYPE_PACKAGE_ELEMENT,
            .u.PackageElement = {
                .Package = Source.u.Package,
                .ElementIndex = Index.u.Integer,
            }
        };
        AmlPackageDataReference( Source.u.Package );

        //
        // Setup a reference to the created package element data value.
        //
        Reference = ( AML_DATA ){
            .Type = AML_DATA_TYPE_REFERENCE,
            .u.Reference = {
                .Object = SourceObject,
            }
        };
        break;
    default:
        AML_DEBUG_ERROR( State, "Error: invalid index source operand (must be buffer, string, package).\n" );
        return AML_FALSE;
    }

    //
    // Optionally output the reference to the target destination operand.
    //
    if( AmlOperandStore( State, &State->Heap, &Reference, Target, AML_FALSE ) == AML_FALSE ) {
        return AML_FALSE;
    }
    AmlObjectRelease( Target );

    //
    // Release our local copy of the source operand.
    //
    AmlDataFree( &Source );

    //
    // Set up the temporary reference object return value.
    //
    ReferenceObject->u.Name = ( AML_OBJECT_NAME ){ .Value = Reference };

    //
    // Return the created temporary reference object.
    //
    *ppObject = ReferenceObject;
    return AML_TRUE;
}

//
// MethodInvocation := NameString TermArgList
// ReferenceTypeOpcode := DefRefOf | DefDerefOf | DefIndex | MethodInvocation
// The returned object is referenced and must be released by the caller.
//
_Success_( return )
BOOLEAN
AmlEvalReferenceTypeOpcode(
    _Inout_  AML_STATE*   State,
    _In_     UINT         SearchFlags,
    _Outptr_ AML_OBJECT** ppObject
    )
{
    AML_DECODER_INSTRUCTION_OPCODE Peek;
    AML_DATA                       MethodReturn;

    //
    // Peek the next full instruction opcode.
    //
    if( AmlDecoderPeekOpcode( State, &Peek ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // ReferenceTypeOpcode := DefRefOf | DefDerefOf | DefIndex | MethodInvocation
    //
    switch( Peek.OpcodeID ) {
    case AML_OPCODE_ID_REF_OF_OP:
        if( AmlEvalRefOf( State, AML_TRUE, ppObject ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case AML_OPCODE_ID_DEREF_OF_OP:
        if( AmlEvalDerefOf( State, AML_TRUE, ppObject ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case AML_OPCODE_ID_INDEX_OP:
        if( AmlEvalIndex( State, AML_TRUE, ppObject ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    default:
        //
        // Last chance to match something, try evaluating a method invocation, it must return a reference to be used here.
        //
        if( AmlEvalMethodInvocation( State, 0, &MethodReturn ) ) {
            if( MethodReturn.Type != AML_DATA_TYPE_REFERENCE ) {
                AmlDataFree( &MethodReturn );
                return AML_FALSE;
            } else if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_NAME, ppObject ) == AML_FALSE ) {
                AmlDataFree( &MethodReturn );
                return AML_FALSE;
            }
            ( *ppObject )->u.Name = ( AML_OBJECT_NAME ){ .Value = MethodReturn };
            return AML_TRUE;
        }
        AML_DEBUG_ERROR( State, "Error: Unsupported EvalReferenceTypeOpcode case.\n" );
        return AML_FALSE;
    }

    return AML_TRUE;
}
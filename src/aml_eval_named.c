#include "aml_state.h"
#include "aml_eval.h"
#include "aml_host.h"
#include "aml_conv.h"
#include "aml_compare.h"
#include "aml_debug.h"

//
// Evaluate a named buffer field CreateXFieldOp instruction.
//
_Success_( return )
static
BOOLEAN
AmlEvalCreateBufferField(
    _Inout_ AML_STATE* State,
    _In_    UINT16     OpcodeID
    )
{
    AML_OBJECT*         Object;
    AML_DATA            SourceBuf;
    AML_DATA            IndexData;
    AML_NAME_STRING     Name;
    AML_DATA            BitCountData;
    UINT64              BitCount;
    BOOLEAN             IsBitIndexed;
    UINT64              BitIndex;
    AML_NAMESPACE_NODE* Node;

    //
    // Deduce the actual bit count and index type for the field by opcode type.
    // In the case of CreateFieldOp, it is variable and will be decoded after.
    //
    IsBitIndexed = AML_FALSE;
    switch( OpcodeID ) {
    case AML_OPCODE_ID_CREATE_FIELD_OP:
        BitCount = 0;
        IsBitIndexed = AML_TRUE;
        break;
    case AML_OPCODE_ID_CREATE_BIT_FIELD_OP:
        BitCount = 1;
        IsBitIndexed = AML_TRUE;
        break;
    case AML_OPCODE_ID_CREATE_BYTE_FIELD_OP:
        BitCount = 8;
        break;
    case AML_OPCODE_ID_CREATE_WORD_FIELD_OP:
        BitCount = 16;
        break;
    case AML_OPCODE_ID_CREATE_DWORD_FIELD_OP:
        BitCount = 32;
        break;
    case AML_OPCODE_ID_CREATE_QWORD_FIELD_OP:
        BitCount = 64;
        break;
    default:
        return AML_FALSE;
    }

    //
    // Generic field creation opcodes:
    // SourceBuff := TermArg => Buffer
    // BitIndex := TermArg => Integer
    // NumBits := TermArg => Integer
    // CreateFieldOp SourceBuff BitIndex ?(NumBits) NameString
    // 
    // All buffer field creation opcodes other than CreateFieldOp follow the same
    // opcode definition, with only CreateFieldOp having an extra NumBits field.
    // 
    // TODO: Restructure code to allow releasing of SourceBuf on failure.
    //
    if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_BUFFER, &SourceBuf ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &IndexData ) == AML_FALSE ) {
        return AML_FALSE;
    } 
    
    //
    // Only CreateFieldOp has an extra variable BitCount field.
    //
    if( OpcodeID == AML_OPCODE_ID_CREATE_FIELD_OP ) {
        if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &BitCountData ) == AML_FALSE ) {
            return AML_FALSE;
        }
        BitCount = BitCountData.u.Integer;
    }

    //
    // All buffer field opcodes end with the field name.
    //
    if( AmlDecoderConsumeNameString( State, &Name ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Update evaluated region bit index.
    // CreateFieldOp and CreateBitFieldOp are bit-indexed, others are byte-indexed.
    //
    BitIndex = IndexData.u.Integer;
    if( IsBitIndexed == AML_FALSE ){
        if( IndexData.u.Integer >= ( UINT64_MAX / 8 ) ) {
            AML_DEBUG_ERROR( State, "Error: Buffer field bit index leads to overflow.\n" );
            return AML_FALSE;
        }
        BitIndex *= 8;
    }

    //
    // Ensure that the end of the buffer field doesn't lead to overflow.
    //
    if( BitCount > ( UINT64_MAX - BitIndex ) ) {
        AML_DEBUG_ERROR( State, "Error: Buffer field end bit index leads to overflow.\n" );
        return AML_FALSE;
    }

    //
    // Attempt to create a new reference counted object.
    // Don't worry about releasing our reference/memory upon failure here,
    // as failure is fatal to the entire state, and all memory is backed by
    // an underlying internal arena + heap.
    //
    if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_BUFFER_FIELD, &Object ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Initialize new buffer field object.
    //
    Object->u.BufferField = ( AML_OBJECT_BUFFER_FIELD ){
        .Name      = Name,
        .BitIndex  = BitIndex,
        .BitCount  = BitCount,
        .SourceBuf = SourceBuf,
    };

    //
    // Create a namespace node for the field.
    //
    if( AmlStateSnapshotCreateNode( State, NULL, &Object->u.BufferField.Name, &Node ) == AML_FALSE ) {
        AmlObjectRelease( Object );
        return AML_FALSE;
    }

    //
    // Copy field object to created node.
    //
    Object->NamespaceNode = Node;
    Node->Object = Object;
    return AML_TRUE;
}

//
// Evaluate a single field element of a field list.
//
_Success_( return )
static
BOOLEAN
AmlEvalFieldElement(
    _Inout_ AML_STATE*         State,
    _In_    AML_FIELD_ELEMENT  LastFieldElement,
    _Out_   AML_FIELD_ELEMENT* Element
    )
{
    UINT8               FieldElementType;
    UINT8               PeekNameByte;
    AML_DATA            ConnectionBuffer;
    AML_NAME_STRING     ConnectionName;
    AML_NAMESPACE_NODE* ConnectionNode;

    //
    // Peek the next byte to try to determine what kind of field element this is.
    //
    if( AmlDecoderPeekByte( State, 0, &FieldElementType ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Default initialize element fields.
    //
    *Element = ( AML_FIELD_ELEMENT ){
        .Type               = FieldElementType,
        .AccessType         = LastFieldElement.AccessType,
        .AccessAttributes   = LastFieldElement.AccessAttributes,
        .AccessByteLength   = LastFieldElement.AccessByteLength,
        .IsConnection       = LastFieldElement.IsConnection,
        .ConnectionResource = LastFieldElement.ConnectionResource,
    };

    //
    // NamedField := NameSeg PkgLength
    // ReservedField := 0x00 PkgLength
    // AccessField := 0x01 AccessType AccessAttrib
    // ConnectField := <0x02 NameString> | <0x02 BufferData>
    // ExtendedAccessField := 0x03 AccessType ExtendedAccessAttrib AccessLength
    // FieldElement := NamedField | ReservedField | AccessField | ExtendedAccessField | ConnectField
    //
    switch( FieldElementType ) {
    case 0:
        //
        // ReservedField := 0x00 PkgLength
        //
        AmlDecoderConsumeByte( State, NULL );
        if( AmlDecoderConsumePackageLength( State, &Element->Length ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case 1:
        //
        // AccessType := ByteData
        // AccessAttrib := ByteData
        // AccessField := 0x01 AccessType AccessAttrib
        //
        AmlDecoderConsumeByte( State, NULL );
        if( AmlDecoderConsumeByte( State, &Element->AccessType ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlDecoderConsumeByte( State, &Element->AccessAttributes ) == AML_FALSE ) {
            return AML_FALSE;
        }
        AML_DEBUG_TRACE( 
            State,
            "AccessField = AccessType=(0x%x), AccessAttrib=(0x%x)\n",
            ( UINT32 )Element->AccessType, ( UINT32 )Element->AccessAttributes
        );
        break;
    case 2:
        //
        // ConnectField := <0x02 NameString> | <0x02 BufferData>
        //
        AmlDecoderConsumeByte( State, NULL );
        if( AmlDecoderMatchNameString( State, AML_FALSE, &ConnectionName ) ) {
            ConnectionNode = NULL;
            if( ( AmlNamespaceSearch( &State->Namespace, NULL, &ConnectionName, 0, &ConnectionNode ) == AML_FALSE )
                || ( ConnectionNode == NULL )
                || ( ConnectionNode->Object == NULL )
                || ( ConnectionNode->Object->Type != AML_OBJECT_TYPE_NAME )
                || ( ConnectionNode->Object->u.Name.Value.Type != AML_DATA_TYPE_BUFFER ) 
                || ( AmlDataDuplicate( &ConnectionNode->Object->u.Name.Value, &State->Heap, &ConnectionBuffer ) == AML_FALSE ) )
            {
                AML_DEBUG_ERROR( State, "Error: Invalid field connection, name: \"" );
                AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_ERROR, &ConnectionName );
                AML_DEBUG_ERROR( State, "\"\n" );
                return AML_FALSE;
            }
        } else if( ( AmlEvalTermArg( State, 0, &ConnectionBuffer ) == AML_FALSE )
                   || ( ConnectionBuffer.Type != AML_DATA_TYPE_BUFFER ) )
        {
            AmlDataFree( &ConnectionBuffer );
            return AML_FALSE;
        }

        //
        // If the previous element already contained a connection, release it and replace it with the new one.
        //
        AmlDataFree( &Element->ConnectionResource );
        Element->ConnectionResource = ConnectionBuffer;
        Element->IsConnection = AML_TRUE;
        break;
    case 3:
        //
        // AccessType := ByteData
        // ExtendedAccessAttrib := ByteData
        //	 (0x0B AttribBytes, 0x0E AttribRawBytes, 0x0F AttribRawProcess)
        // ExtendedAccessField := 0x03 AccessType ExtendedAccessAttrib AccessLength
        //
        AmlDecoderConsumeByte( State, NULL );
        if( AmlDecoderConsumeByte( State, &Element->AccessType ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlDecoderConsumeByte( State, &Element->AccessAttributes ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlDecoderConsumeByte( State, &Element->AccessByteLength ) == AML_FALSE ) {
            return AML_FALSE;
        }
        AML_DEBUG_TRACE( 
            State,
            "ExtendedAccessField = AccessType=(0x%x), AccessAttrib=(0x%x), AccessLength=(0x%x)\n",
            ( UINT32 )Element->AccessType, ( UINT32 )Element->AccessAttributes, ( UINT32 )Element->AccessByteLength
        );
        break;
    default:
        //
        // NamedField := NameSeg PkgLength
        //
        if( AmlDecoderPeekByte( State, 0, &PeekNameByte ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( ( PeekNameByte < 'A' || PeekNameByte > 'Z' ) && PeekNameByte != '_' ) {
            return AML_FALSE;
        } else if( AmlDecoderConsumeNameString( State, &Element->Name ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlDecoderConsumePackageLength( State, &Element->Length ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Override junk FieldElementType with our own internal value.
        //
        Element->Type = AML_FIELD_ELEMENT_TYPE_NAMED;
        break;
    }

    return AML_TRUE;
}

//
// Evaluate a DefField instruction.
//
_Success_( return )
static
BOOLEAN
AmlEvalFieldOpcode(
    _Inout_ AML_STATE* State
    )
{
    SIZE_T              OldDataCursor;
    SIZE_T              OldDataLength;
    SIZE_T              PkgLength;
    AML_NAME_STRING     RegionName;
    AML_NAMESPACE_NODE* OpRegionNsNode;
    UINT8               FieldFlags;
    UINT64              FieldOffset;
    AML_FIELD_ELEMENT   Element;
    AML_OBJECT*         Object;
    AML_NAMESPACE_NODE* Node;

    //
    // Backup old decoder window.
    //
    OldDataCursor = State->DataCursor;
    OldDataLength = State->DataLength;

    //
    // Read package length.
    // DefField := FieldOp PkgLength NameString FieldFlags FieldList
    //
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Validate the package length.
    //
    if( AmlDecoderIsValidDataWindow( State, OldDataCursor, PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Update decoder window to use the decoded package length.
    //
    State->DataLength = ( State->DataCursor + ( PkgLength - ( State->DataCursor - OldDataCursor ) ) );

    //
    // RegionName := NameString => OperationRegion
    // FieldFlags := ByteData => AccessTypeKeyword
    //
    if( AmlDecoderConsumeNameString( State, &RegionName ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeByte( State, &FieldFlags ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Lookup the operation region referenced by this set of fields.
    //
    if( AmlNamespaceSearch( &State->Namespace, NULL, &RegionName, 0, &OpRegionNsNode ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( OpRegionNsNode->Object->Type != AML_OBJECT_TYPE_OPERATION_REGION ) {
        return AML_FALSE;
    }

    //
    // The current field properties will change as we parse fields.
    //
    Element = ( AML_FIELD_ELEMENT ){ .AccessType = AML_FIELD_FLAGS_ACCESS_TYPE_GET( FieldFlags ) };
    FieldOffset = 0;

    //
    // FieldList := Nothing | <fieldelement fieldlist>
    //
    while( State->DataCursor < State->DataLength ) {
        //
        // Decode and consume a single field element.
        //
        if( AmlEvalFieldElement( State, Element, &Element ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // If we have decoded an actual named field element, create a new field with the current properties.
        //
        if( Element.Type == AML_FIELD_ELEMENT_TYPE_NAMED ) {
            //
            // Ensure that the end bit index of the element doesn't lead to overflow.
            //
            if( Element.Length > ( UINT64_MAX - FieldOffset ) ) {
                AML_DEBUG_ERROR( State, "Error: Field element length would lead to overflow!\n" );
                return AML_FALSE;
            }

            //
            // Allocate new field object.
            //
            if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_FIELD, &Object ) == AML_FALSE ) {
                return AML_FALSE;
            }

            //
            // Initialize field object and reference the parent operation region.
            //
            Object->u.Field = ( AML_OBJECT_FIELD ){
                .Flags           = FieldFlags,
                .Offset          = FieldOffset,
                .Element         = Element,
                .OperationRegion = OpRegionNsNode->Object
            };
            AmlObjectReference( OpRegionNsNode->Object );

            //
            // Raise the reference counter of the element's connection (if any).
            //
            if( AmlDataDuplicate( &Element.ConnectionResource, &State->Heap, &Object->u.Field.Element.ConnectionResource ) == AML_FALSE ) {
                AmlObjectRelease( Object );
                return AML_FALSE;
            }

            //
            // Create a namespace node for the object.
            //
            if( AmlStateSnapshotCreateNode( State, NULL, &Element.Name, &Node ) == AML_FALSE ) {
                AmlObjectRelease( Object );
                return AML_FALSE;
            }

            //
            // Update namespace node with our created object.
            //
            Object->NamespaceNode = Node;
            Node->Object = Object;
        }

        //
        // Move the current field offset forward by the length of the decoded field.
        //
        if( Element.Length >= ( UINT64_MAX - FieldOffset ) ) {
            AML_DEBUG_ERROR( State, "Error: Field element offset would lead to overflow.\n" );
            return AML_FALSE;
        }
        FieldOffset += Element.Length;
    }

    //
    // Release the referenced field element's connection (if any).
    //
    AmlDataFree( &Element.ConnectionResource );

    //
    // Restore the old data window length.
    //
    State->DataLength = OldDataLength;
    return AML_TRUE;
}

//
// Evaluate a DefBankField instruction.
//
_Success_( return )
static
BOOLEAN
AmlEvalBankFieldOpcode(
    _Inout_ AML_STATE* State
    )
{
    SIZE_T              OldDataCursor;
    SIZE_T              OldDataLength;
    SIZE_T              PkgLength;
    AML_NAME_STRING     OpRegionName;
    AML_NAME_STRING     BankName;
    AML_DATA            BankValueData;
    UINT8               FieldFlags;
    AML_NAMESPACE_NODE* OpRegionNsNode;
    AML_NAMESPACE_NODE* BankNsNode;
    UINT64              FieldOffset;
    AML_FIELD_ELEMENT   Element;
    AML_OBJECT*         Object;
    AML_NAMESPACE_NODE* Node;

    //
    // Backup old decoder window.
    //
    OldDataCursor = State->DataCursor;
    OldDataLength = State->DataLength;

    //
    // Decode the PkgLength of the DefBankField instruction and move the window accordingly.
    //
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }
    
    //
    // Validate the package length.
    //
    if( AmlDecoderIsValidDataWindow( State, OldDataCursor, PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Update decoder window to use the decoded package length.
    //
    State->DataLength = ( State->DataCursor + ( PkgLength - ( State->DataCursor - OldDataCursor ) ) );

    //
    // RegionName := NameString => OperationRegion
    // BankName := NameString => FieldUnit
    // BankValue := TermArg => Integer
    // DefBankField := BankFieldOp PkgLength RegionName BankName BankValue FieldFlags FieldList
    //
    if( AmlDecoderConsumeNameString( State, &OpRegionName ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeNameString( State, &BankName ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &BankValueData ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeByte( State, &FieldFlags ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Lookup the operation region referenced by this set of bank fields.
    //
    if( AmlNamespaceSearch( &State->Namespace, NULL, &OpRegionName, 0, &OpRegionNsNode ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( OpRegionNsNode->Object->Type != AML_OBJECT_TYPE_OPERATION_REGION ) {
        return AML_FALSE;
    }

    //
    // Lookup the bank referenced by this set of bank fields.
    // Ensure that the referenced bank is a field unit object.
    //
    if( AmlNamespaceSearch( &State->Namespace, NULL, &BankName, 0, &BankNsNode ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlObjectIsFieldUnit( BankNsNode->Object ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // The current field properties will change as we parse fields.
    //
    Element = ( AML_FIELD_ELEMENT ){ .AccessType = AML_FIELD_FLAGS_ACCESS_TYPE_GET( FieldFlags ) };
    FieldOffset = 0;

    //
    // FieldList := Nothing | <fieldelement fieldlist>
    //
    while( State->DataCursor < State->DataLength ) {
        //
        // Decode and consume a single field element.
        //
        if( AmlEvalFieldElement( State, Element, &Element ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // If we have decoded an actual named field element, create a new field with the current properties.
        //
        if( Element.Type == AML_FIELD_ELEMENT_TYPE_NAMED ) {
            //
            // Ensure that the end bit index of the element doesn't lead to overflow.
            //
            if( Element.Length > ( UINT64_MAX - FieldOffset ) ) {
                AML_DEBUG_ERROR( State, "Error: Field element length would lead to overflow!\n" );
                return AML_FALSE;
            }

            //
            // Allocate new bank field object.
            //
            if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_BANK_FIELD, &Object ) == AML_FALSE ) {
                return AML_FALSE;
            }

            //
            // Initialize bank field object and reference the parent operation region and bank.
            //
            Object->u.BankField = ( AML_OBJECT_BANK_FIELD ){
                .Base = {
                    .Flags           = FieldFlags,
                    .Offset          = FieldOffset,
                    .Element         = Element,
                    .OperationRegion = OpRegionNsNode->Object,
                },
                .Bank      = BankNsNode->Object,
                .BankValue = BankValueData,
            };
            AmlObjectReference( OpRegionNsNode->Object );
            AmlObjectReference( BankNsNode->Object );

            //
            // Raise the reference counter of the element's connection (if any).
            //
            if( AmlDataDuplicate( &Element.ConnectionResource, &State->Heap, &Object->u.BankField.Base.Element.ConnectionResource ) == AML_FALSE ) {
                AmlObjectRelease( Object );
                return AML_FALSE;
            }

            //
            // Create a namespace node for the object.
            //
            if( AmlStateSnapshotCreateNode( State, NULL, &Element.Name, &Node ) == AML_FALSE ) {
                AmlObjectRelease( Object );
                return AML_FALSE;
            }

            //
            // Update namespace node with our created object.
            //
            Object->NamespaceNode = Node;
            Node->Object = Object;
        }

        //
        // Move the current field offset forward by the length of the decoded field.
        //
        if( Element.Length >= ( SIZE_MAX - FieldOffset ) ) {
            AML_DEBUG_ERROR( State, "Error: Field element offset would lead to overflow.\n" );
            return AML_FALSE;
        }
        FieldOffset += Element.Length;
    }

    //
    // Release the referenced field element's connection (if any).
    //
    AmlDataFree( &Element.ConnectionResource );

    //
    // Restore the old data window length.
    //
    State->DataLength = OldDataLength;
    return AML_TRUE;
}

//
// Evaluate a DefIndexField instruction.
//
_Success_( return )
static
BOOLEAN
AmlEvalIndexFieldOpcode(
    _Inout_ AML_STATE* State
    )
{
    SIZE_T              OldDataCursor;
    SIZE_T              OldDataLength;
    SIZE_T              PkgLength;
    AML_NAME_STRING     IndexName;
    AML_NAME_STRING     DataName;
    AML_NAMESPACE_NODE* IndexNsNode;
    AML_NAMESPACE_NODE* DataNsNode;
    UINT8               FieldFlags;
    UINT64              FieldOffset;
    AML_FIELD_ELEMENT   Element;
    AML_OBJECT*         Object;
    AML_NAMESPACE_NODE* Node;

    //
    // Backup old decoder window.
    //
    OldDataCursor = State->DataCursor;
    OldDataLength = State->DataLength;

    //
    // Decode the PkgLength of the DefIndexField instruction and move the window accordingly.
    //
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Validate the package length.
    //
    if( AmlDecoderIsValidDataWindow( State, OldDataCursor, PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Update decoder window to use the decoded package length.
    //
    State->DataLength = ( State->DataCursor + ( PkgLength - ( State->DataCursor - OldDataCursor ) ) );

    //
    // IndexName := NameString => FieldUnit
    // DataName := NameString => FieldUnit
    // DefIndexField := IndexFieldOp PkgLength IndexName DataName FieldFlags FieldList
    //
    if( AmlDecoderConsumeNameString( State, &IndexName ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeNameString( State, &DataName ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeByte( State, &FieldFlags ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Lookup the index and data namespace nodes and ensure that they are field unit objects.
    //
    if( AmlNamespaceSearch( &State->Namespace, NULL, &IndexName, 0, &IndexNsNode ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlNamespaceSearch( &State->Namespace, NULL, &DataName, 0, &DataNsNode ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlObjectIsFieldUnit( IndexNsNode->Object ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlObjectIsFieldUnit( DataNsNode->Object ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // The current field properties will change as we parse fields.
    //
    Element = ( AML_FIELD_ELEMENT ){ .AccessType = AML_FIELD_FLAGS_ACCESS_TYPE_GET( FieldFlags ) };
    FieldOffset = 0;

    //
    // FieldList := Nothing | <fieldelement fieldlist>
    //
    while( State->DataCursor < State->DataLength ) {
        //
        // Decode and consume a single field element.
        //
        if( AmlEvalFieldElement( State, Element, &Element ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // If we have decoded an actual named field element, create a new field with the current properties.
        //
        if( Element.Type == AML_FIELD_ELEMENT_TYPE_NAMED ) {
            //
            // Ensure that the end bit index of the element doesn't lead to overflow.
            //
            if( Element.Length > ( UINT64_MAX - FieldOffset ) ) {
                AML_DEBUG_ERROR( State, "Error: Field element length would lead to overflow!\n" );
                return AML_FALSE;
            }

            //
            // Allocate new index field object.
            //
            if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_INDEX_FIELD, &Object ) == AML_FALSE ) {
                return AML_FALSE;
            }

            //
            // Initialize index field object and reference the index and data objects.
            //
            Object->u.IndexField = ( AML_OBJECT_INDEX_FIELD ){
                .Flags   = FieldFlags,
                .Offset  = FieldOffset,
                .Element = Element,
                .Index   = IndexNsNode->Object,
                .Data    = DataNsNode->Object
            };
            AmlObjectReference( IndexNsNode->Object );
            AmlObjectReference( DataNsNode->Object );

            //
            // Raise the reference counter of the element's connection (if any).
            //
            if( AmlDataDuplicate( &Element.ConnectionResource, &State->Heap, &Object->u.IndexField.Element.ConnectionResource ) == AML_FALSE ) {
                AmlObjectRelease( Object );
                return AML_FALSE;
            }

            //
            // Create a namespace node for the object.
            //
            if( AmlStateSnapshotCreateNode( State, NULL, &Element.Name, &Node ) == AML_FALSE ) {
                AmlObjectRelease( Object );
                return AML_FALSE;
            }

            //
            // Update namespace node with our created object.
            //
            Object->NamespaceNode = Node;
            Node->Object = Object;
        }

        //
        // Move the current field offset forward by the length of the decoded field.
        //
        if( Element.Length >= ( SIZE_MAX - FieldOffset ) ) {
            AML_DEBUG_ERROR( State, "Error: Field element offset would lead to overflow.\n" );
            return AML_FALSE;
        }
        FieldOffset += Element.Length;
    }

    //
    // Release the referenced field element's connection (if any).
    //
    AmlDataFree( &Element.ConnectionResource );

    //
    // Restore the old data window length.
    //
    State->DataLength = OldDataLength;
    return AML_TRUE;
}

//
// Evaluate a DefDevice instruction.
// DefDevice := DeviceOp PkgLength NameString TermList
// TODO: Use the PkgLength when decoding the rest of the operands.
//
_Success_( return )
BOOLEAN
AmlEvalDevice(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    )
{
    AML_NAME_STRING     DeviceName;
    AML_NAMESPACE_NODE* SearchNode;
    SIZE_T              PkgLength;
    SIZE_T              PkgStart;
    SIZE_T              PkgCodeOffset;
    SIZE_T              CodeStart;
    SIZE_T              CodeSize;
    AML_OBJECT*         Object;
    AML_NAMESPACE_NODE* Node;

    //
    // Consume the instruction opcode if the caller hasn't already.
    //
    if( ConsumeOpcode ) {
        if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_DEVICE_OP, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    //
    // DefDevice := DeviceOp PkgLength NameString TermList
    //
    PkgStart = State->DataCursor;
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeNameString( State, &DeviceName ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Check if the namespace pre-pass has already processed this device.
    //
    Node = NULL;
    if( AmlNamespaceSearch( &State->Namespace, NULL, &DeviceName, AML_SEARCH_FLAG_NAME_CREATION, &SearchNode ) ) {
        if( SearchNode->IsPreParsed && SearchNode->Object && SearchNode->Object->Type == AML_OBJECT_TYPE_DEVICE ) {
            AML_DEBUG_TRACE( State, "Found pre-parsed device.\n" );
            Node = SearchNode;
        }
    }

    //
    // Print debug information about the device name.
    //
    AML_DEBUG_TRACE( State, "Device(" );
    AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_TRACE, &DeviceName );
    AML_DEBUG_TRACE( State, ")\n" );

    //
    // Create the device if it hasn't already been created by a namespace pass.
    //
    if( Node == NULL ) {
        //
        // Create a namespace node for the device.
        //
        if( AmlStateSnapshotCreateNode( State, NULL, &DeviceName, &Node ) == AML_FALSE ) {
            AML_DEBUG_ERROR( State, "Error: Name collision for device: " );
            AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_ERROR, &DeviceName );
            AML_DEBUG_ERROR( State, "\n" );
            return AML_FALSE;
        }

        //
        // Attempt to create a new reference counted object.
        // Don't worry about releasing our reference/memory upon failure here,
        // as failure is fatal to the entire state, and all memory is backed by
        // an underlying internal arena + heap.
        //
        if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_DEVICE, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Initialize device object.
        // TODO: Deep-copy name string in the future, currently all bytecode needs to stay loaded.
        //
        Object->u.Device = ( AML_OBJECT_DEVICE ){ .Name = DeviceName };

        //
        // Copy device object to created node.
        //
        Object->NamespaceNode = Node;
        Node->Object = Object;
        Node->IsPreParsed = ( State->PassType == AML_PASS_TYPE_NAMESPACE );
    }

    //
    // Mark this node as fully evaluated if this is the real evaluation pass.
    //
    Node->IsEvaluated = ( State->PassType == AML_PASS_TYPE_FULL );

    //
    // Push and enter a new namespace scope level with the given name.
    //
    if( AmlNamespacePushScope( &State->Namespace, &Node->AbsolutePath, 0 ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Get offset and length of the start of the TermList code embedded inside of the package.
    //
    CodeStart = State->DataCursor;
    PkgCodeOffset = ( CodeStart - PkgStart );
    if( PkgCodeOffset > PkgLength ) {
        return AML_FALSE;
    }
    CodeSize = ( PkgLength - PkgCodeOffset );

    //
    // Recursively evaluate all code of the scope.
    //
    if( AmlEvalTermListCode( State, CodeStart, CodeSize, AML_FALSE ) == AML_FALSE ) {
        return AML_FALSE;
    }
          
    //
    // Pop and exit the namespace scope level.
    //
    if( AmlNamespacePopScope( &State->Namespace ) == AML_FALSE ) {
        return AML_FALSE;
    }

    return AML_TRUE;
}

//
// Evaluate a DefMethod instruction.
// DefDevice := DeviceOp PkgLength NameString TermList
//
_Success_( return )
BOOLEAN
AmlEvalMethod(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    )
{
    UINT8               PeekByte;
    SIZE_T              PkgLength;
    SIZE_T              PkgStart;
    AML_NAME_STRING     MethodName;
    AML_OBJECT_METHOD   Method;
    SIZE_T              CodeStart;
    SIZE_T              CodeSize;
    SIZE_T              i;
    SIZE_T              MaxTypeBytes;
    UINT8               MethodFlags;
    AML_OBJECT*         Object;
    AML_NAMESPACE_NODE* SearchNode;
    AML_NAMESPACE_NODE* Node;

    //
    // Consume the instruction opcode if the caller hasn't already.
    //
    if( ConsumeOpcode ) {
        if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_METHOD_OP, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    //
    // DefMethod := MethodOp PkgLength NameString MethodFlags TermList
    //
    PkgStart = State->DataCursor;
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeNameString( State, &MethodName ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeByte( State, &MethodFlags ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Print debug information about the method name.
    //
    AML_DEBUG_TRACE( State, "Method(" );
    AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_TRACE, &MethodName );
    AML_DEBUG_TRACE( State, ")\n" );

    //
    // If we are in full evaluation mode, first check if the namespace pre-pass has already processed this method.
    // TODO: Validate the rest of the properties of the method before skipping?
    //
    Node = NULL;
    if( AmlNamespaceSearch( &State->Namespace, NULL, &MethodName, AML_SEARCH_FLAG_NAME_CREATION, &SearchNode ) ) {
        if( SearchNode->IsPreParsed && SearchNode->Object && SearchNode->Object->Type == AML_OBJECT_TYPE_METHOD ) {
            AML_DEBUG_TRACE( State, "Found pre-parsed method, skipping.\n" );
            Node = SearchNode;
        }
    }

    //
    // Set up method value.
    // TODO: Copy MethodName, currently bytecode has to remain loaded forever.
    //
    Method = ( AML_OBJECT_METHOD ){ .Name = MethodName };

    //
    // Decode MethodFlags values to struct fields.
    //
    Method.ArgumentCount = AML_METHOD_FLAGS_ARG_COUNT( MethodFlags );
    Method.IsSerialized  = AML_METHOD_FLAGS_SERIALIZE( MethodFlags );
    Method.SyncLevel     = AML_METHOD_FLAGS_SYNC_LEVEL( MethodFlags );

    //
    // Not defined by the AML spec whatsoever, ACPICA compiler/ASL allows defining methods with return type/parameter type sets,
    // which are then encoded as a series of ObjectType byte values following the MethodFlags, they are ambiguous and overlap with
    // existing instruction opcodes, as well as breaking the PkgLength (they aren't included by the PkgLength?!). It never ends.
    // 
    // A more typical case with both a ReturnType and ParameterTypes specified, somewhat usable,
    // though a mismatch will still lead to breaking the rest of decoding:
    // 
    // Method(MTH0, 1, NotSerialized, 0, StrObj, IntObj) { ... }
    // = 14 XX 4D 54 48 30 01 [02] [01] { ... }
    //      ^                  ^    ^ Parameter types (0 or ArgumentCount extra bytes).
    //      |                  + Return type (0 or 1 extra byte).
    //      + PkgLength that doesn't include any of the extra type bytes.
    // 
    // 
    // Weirder cases due to allowing Nothing for both fields, completely ambiguous,
    // why even encode this information when it is unusable due to complete ambiguity?:
    // 
    // Method(MTH0, 1, NotSerialized, 0, , IntObj) { ... }
    // = 14 XX 4D 54 48 30 01 [01]
    //                         ^ Parameter types
    // 
    // Method(MTH0, 1, NotSerialized, 0, IntObj, ) { ... }
    // = 14 XX 4D 54 48 30 01 [01]
    //                         ^ Return type
    // 
    // Due to the ambiguity caused by allowing "Nothing" for both fields,
    // it is impossible to tell if the first type is a return type or parameter type, 
    // so even if you do manage to parse these type bytes without breaking the decoder stream,
    // they are useless, as you have no way of knowing which is which for sure.
    // 
    // For clarification in terms of how the decoder stream can be broken due to the
    // ambiguous encoding and overlap of ObjectType values and real instruction opcodes,
    // MethodObj == 8, NAME_OP == 8:
    // 
    // Method(MTH0, 1, NotSerialized, 0, IntObj, MethodObj) {
    //	Name(NOM0, 0x72727272)
    //	Return(Add(Arg0, NOM0))
    // }
    // 
    // Method(MTH1, 1, NotSerialized, 0, , MethodObj) {
    //	Return(Add(Arg0, 1000))
    //}
    // 
    // The above snippets cause ACPICA itself to implode:
    // 
    // ACPI Warning: Invalid character(s) in name (0x4D4F4E08) 001FFA68, repaired: [_NOM] (20230628/utstring-339)
    // ACPI Error: Aborting method \MTH0 due to previous error (AE_AML_NO_OPERAND) (20230628/psparse-689)
    // ACPI Error: Aborting method \ due to previous error (AE_AML_NO_OPERAND) (20230628/psparse-689)
    // ACPI Error: Invalid zero thread count in method (20230628/dsmethod-978)
    // ACPI Error: Invalid OwnerId: 0x000 (20230628/utownerid-318)
    // ACPI Error: AE_AML_NO_OPERAND, (SSDT:       Y) while loading table (20230628/tbxfload-353)
    // 
    // ACPI Warning: Invalid character(s) in name (0x0B6872A4) 0083F788, repaired: [____] (20230628/utstring-339)
    // ACPI Error: Result stack is empty! State=02FA44C0 (20230628/dswstate-218)
    // ACPI Error: AE_AML_NO_RETURN_VALUE, Missing or null operand (20230628/dsutils-805)
    // ACPI Error: AE_AML_NO_RETURN_VALUE, While creating Arg 0 (20230628/dsutils-931)
    //
    MaxTypeBytes = AML_MIN( ( Method.ArgumentCount + 1 ), ( State->DataLength - State->DataCursor ) );
    for( i = 0; i < MaxTypeBytes; i++ ) {
        //
        // Stop trying to shave off pointless type bytes if we have hit a byte
        // that can't be an ObjectType value (as of ACPI Spec 6.4).
        //
        if( AmlDecoderPeekByte( State, 0, &PeekByte ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( PeekByte > 16 ) {
            break;
        }

        //
        // Attempt to avoid false-positive cases due to ObjectType overlaps with AliasOp, NameOp, and ScopeOp.
        //
        if( PeekByte == AML_L1_ALIAS_OP || PeekByte == AML_L1_NAME_OP || PeekByte == AML_L1_SCOPE_OP ) {
            if( AmlDecoderPeekByte( State, 1, &PeekByte ) ) {
                if( ( PeekByte >= 'A' && PeekByte <= 'Z' )
                    || ( PeekByte == '_' )
                    || ( PeekByte == '^' )
                    || ( PeekByte == '\\' )
                    || ( PeekByte == AML_L1_DUAL_NAME_PREFIX ) 
                    || ( PeekByte == AML_L1_MULTI_NAME_PREFIX ) )
                {
                    break;
                }
            }
        }

        //
        // Skip the possible ObjectType byte.
        //
        if( AmlDecoderConsumeByte( State, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Include the extra bytes in the PkgLength, makes getting to the end of the TermList easier.
        //
        PkgLength++;
    }

    //
    // Get offset and length of the start of the TermList code embedded inside of the package.
    //
    CodeStart = State->DataCursor;
    if( PkgStart > State->DataLength
        || PkgLength > ( State->DataLength - PkgStart ) )
    {
        return AML_FALSE;
    }
    CodeSize = ( PkgLength - ( CodeStart - PkgStart ) );
    if( CodeSize > ( State->DataLength - State->DataCursor ) ) {
        return AML_FALSE;
    }

    //
    // Save the start offset and size of the method's code, to be called later, do not immediately eval contents.
    //
    Method.CodeStart         = CodeStart; 
    Method.CodeSize          = CodeSize;
    Method.CodeDataBlock     = State->Data;
    Method.CodeDataBlockSize = State->DataTotalLength;

    //
    // Skip past the method code termlist.
    //
    State->DataCursor += CodeSize;

    //
    // Create a new object and namespace node for the method if we haven't already made one in the namespace pre-pass.
    //
    if( Node == NULL ) {
        //
        // Attempt to create a new reference counted nethod object.
        //
        if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_METHOD, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Initialize method object value.
        //
        Object->u.Method = Method;

        //
        // Create namespace node for the method.
        //
        if( AmlStateSnapshotCreateNode( State, State->Namespace.ScopeLast, &MethodName, &Node ) == AML_FALSE ) {
            AmlObjectRelease( Object );
            return AML_FALSE;
        }

        //
        // Mark the scope of this node as temporary, any sub-nodes will be released upon exit of this node's scope.
        //
        Node->ScopeFlags |= AML_SCOPE_FLAG_TEMPORARY;

        //
        // Save decoded method information.
        //
        Object->NamespaceNode = Node;
        Node->Object = Object;
        Node->IsPreParsed = ( State->PassType == AML_PASS_TYPE_NAMESPACE );
    }

    //
    // Mark this node as fully evaluated if this is the real evaluation pass.
    //
    Node->IsEvaluated = ( State->PassType == AML_PASS_TYPE_FULL );
    return AML_TRUE;
}

//
// DefExternal := ExternalOp NameString ObjectType ArgumentCount
//
_Success_( return )
BOOLEAN
AmlEvalExternal(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    )
{
    AML_NAME_STRING Name;
    UINT8           ObjectType;
    UINT8           ArgumentCount;

    //
    // Consume the instruction opcode if the caller has yet to do it.
    //
    if( ConsumeOpcode ) {
        if( AmlDecoderConsumeOpcode( State, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    //
    // ObjectType := ByteData
    // ArgumentCount := ByteData
    // DefExternal := ExternalOp NameString ObjectType ArgumentCount
    //
    if( AmlDecoderConsumeNameString( State, &Name ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeByte( State, &ObjectType ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeByte( State, &ArgumentCount ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Print debug trace information about the external.
    // TODO: This may give us extra information to use for parsing method calls in the namespace pass.
    //
    AML_DEBUG_TRACE( State, "External(" );
    AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_TRACE, &Name );
    AML_DEBUG_TRACE( State, ", %i, %i)\n", ( INT )ObjectType, ( INT )ArgumentCount);

    return AML_TRUE;
}

//
// DefProcessor := ProcessorOp PkgLength NameString ProcID PblkAddr PblkLen TermList
//
_Success_( return )
BOOLEAN
AmlEvalProcessor(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    )
{
    SIZE_T              OriginalLength;
    SIZE_T              PkgStart;
    SIZE_T              PkgLength;
    AML_NAME_STRING     Name;
    UINT8               ProcID;
    UINT32              PblkAddr;
    UINT8               PblkLen;
    AML_OBJECT*         Object;
    AML_NAMESPACE_NODE* SearchNode;
    AML_NAMESPACE_NODE* Node;
    SIZE_T              PkgCodeOffset;
    SIZE_T              PkgCodeSize;

    //
    // Consume the instruction opcode if the caller has yet to do it.
    //
    if( ConsumeOpcode ) {
        if( AmlDecoderConsumeOpcode( State, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    //
    // ProcID := ByteData
    // PblkAddr := DWordData
    // PblkLen := ByteData
    // DefProcessor := ProcessorOp PkgLength NameString ProcID PblkAddr PblkLen TermList
    //
    PkgStart = State->DataCursor;
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Validate the package size of the instruction (includes the entire TermList).
    //
    if( AmlDecoderIsValidDataWindow( State, PkgStart, PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Continue decoding the rest of the instruction using the read package length.
    //
    OriginalLength = State->DataLength;
    State->DataLength = ( State->DataCursor + ( PkgLength - ( State->DataCursor - PkgStart ) ) );

    //
    // Decode the rest of the instruction operands.
    //
    if( AmlDecoderConsumeNameString( State, &Name ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeByte( State, &ProcID ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeDWord( State, &PblkAddr ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeByte( State, &PblkLen ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Print debug information about the processor name.
    //
    AML_DEBUG_TRACE( State, "Processor(" );
    AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_TRACE, &Name );
    AML_DEBUG_TRACE( State, ")\n" );

    //
    // Check if the namespace pre-pass has already processed this processor.
    //
    Node = NULL;
    if( AmlNamespaceSearch( &State->Namespace, NULL, &Name, AML_SEARCH_FLAG_NAME_CREATION, &SearchNode ) ) {
        if( SearchNode->IsPreParsed && SearchNode->Object && SearchNode->Object->Type == AML_OBJECT_TYPE_PROCESSOR ) {
            AML_DEBUG_TRACE( State, "Found pre-parsed processor, skipping.\n" );
            Node = SearchNode;
        }
    }

    //
    // Create and setup a new namespace node/object for this processor if we haven't already.
    //
    if( Node == NULL ) {
        if( AmlStateSnapshotCreateNode( State, NULL, &Name, &Node ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_PROCESSOR, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        }
        Object->u.Processor = ( AML_OBJECT_PROCESSOR ){ .PBLKAddress = PblkAddr, .PBLKLength = PblkLen, .ID = ProcID };

        //
        // Update the namespace node with the created object.
        //
        Object->NamespaceNode = Node;
        Node->Object = Object;
        Node->IsPreParsed = ( State->PassType == AML_PASS_TYPE_NAMESPACE );
    }

    //
    // If this was a full evaluation pass, mark the node as evaluated.
    //
    Node->IsEvaluated = ( State->PassType == AML_PASS_TYPE_FULL );

    //
    // Get offset and length of the start of the TermList code embedded inside of the package.
    //
    PkgCodeOffset = ( State->DataCursor - PkgStart );
    if( PkgCodeOffset > PkgLength ) {
        return AML_FALSE;
    }
    PkgCodeSize = ( PkgLength - PkgCodeOffset );

    //
    // Push a new scope level for the processor TermList, and attempt to evaluate it.
    //
    if( AmlNamespacePushScope( &State->Namespace, &Node->AbsolutePath, 0 ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlEvalTermListCode( State, State->DataCursor, PkgCodeSize, AML_FALSE ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlNamespacePopScope( &State->Namespace ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Restore the original data window length.
    //
    State->DataLength = OriginalLength;
    return AML_TRUE;
}

//
// DefPowerRes := PowerResOp PkgLength NameString SystemLevel ResourceOrder TermList
//
_Success_( return )
BOOLEAN
AmlEvalPowerRes(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    )
{
    SIZE_T              OriginalLength;
    SIZE_T              PkgStart;
    SIZE_T              PkgLength;
    AML_NAME_STRING     Name;
    UINT8               SystemLevel;
    UINT16              ResourceOrder;
    AML_OBJECT*         Object;
    AML_NAMESPACE_NODE* SearchNode;
    AML_NAMESPACE_NODE* Node;
    SIZE_T              PkgCodeOffset;
    SIZE_T              PkgCodeSize;

    //
    // Consume the instruction opcode if the caller has yet to do it.
    //
    if( ConsumeOpcode ) {
        if( AmlDecoderConsumeOpcode( State, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    //
    // Decode the PkgLength that encapsulates the rest of the operands and the TermList.
    //
    PkgStart = State->DataCursor;
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Validate the package size of the instruction (includes the entire TermList).
    //
    if( AmlDecoderIsValidDataWindow( State, PkgStart, PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Continue decoding the rest of the instruction using the read package length.
    //
    OriginalLength = State->DataLength;
    State->DataLength = ( State->DataCursor + ( PkgLength - ( State->DataCursor - PkgStart ) ) );

    //
    // Decode the rest of the instruction operands.
    // SystemLevel := ByteData
    // ResourceOrder := WordData
    // DefPowerRes := PowerResOp PkgLength NameString SystemLevel ResourceOrder TermList
    //
    if( AmlDecoderConsumeNameString( State, &Name ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeByte( State, &SystemLevel ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeWord( State, &ResourceOrder ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Print debug information about the name.
    //
    AML_DEBUG_TRACE( State, "PowerResource(" );
    AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_TRACE, &Name );
    AML_DEBUG_TRACE( State, ")\n" );

    //
    // Check if the namespace pre-pass has already processed this.
    //
    Node = NULL;
    if( AmlNamespaceSearch( &State->Namespace, NULL, &Name, AML_SEARCH_FLAG_NAME_CREATION, &SearchNode ) ) {
        if( SearchNode->IsPreParsed && SearchNode->Object && SearchNode->Object->Type == AML_OBJECT_TYPE_POWER_RESOURCE ) {
            AML_DEBUG_TRACE( State, "Found pre-parsed power resource, skipping.\n" );
            Node = SearchNode;
        }
    }

    //
    // Create and setup a new namespace node/object if we haven't already.
    //
    if( Node == NULL ) {
        if( AmlStateSnapshotCreateNode( State, NULL, &Name, &Node ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_POWER_RESOURCE, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        }
        Object->u.PowerResource = ( AML_OBJECT_POWER_RESOURCE ){ .SystemLevel = SystemLevel, .ResourceOrder = ResourceOrder };

        //
        // Update the namespace node with the created object.
        //
        Object->NamespaceNode = Node;
        Node->Object = Object;
        Node->IsPreParsed = ( State->PassType == AML_PASS_TYPE_NAMESPACE );
    }

    //
    // If this was a full evaluation pass, mark the node as evaluated.
    //
    Node->IsEvaluated = ( State->PassType == AML_PASS_TYPE_FULL );

    //
    // Get offset and length of the start of the TermList code embedded inside of the package.
    //
    PkgCodeOffset = ( State->DataCursor - PkgStart );
    if( PkgCodeOffset > PkgLength ) {
        return AML_FALSE;
    }
    PkgCodeSize = ( PkgLength - PkgCodeOffset );

    //
    // Push a new scope level for the TermList, and attempt to evaluate it.
    //
    if( AmlNamespacePushScope( &State->Namespace, &Node->AbsolutePath, 0 ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlEvalTermListCode( State, State->DataCursor, PkgCodeSize, AML_FALSE ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlNamespacePopScope( &State->Namespace ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Restore the original data window length.
    //
    State->DataLength = OriginalLength;
    return AML_TRUE;
}

//
// DefThermalZone := ThermalZoneOp PkgLength NameString TermList
//
_Success_( return )
BOOLEAN
AmlEvalThermalZone(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    )
{
    SIZE_T              OriginalLength;
    SIZE_T              PkgStart;
    SIZE_T              PkgLength;
    AML_NAME_STRING     Name;
    AML_OBJECT*         Object;
    AML_NAMESPACE_NODE* SearchNode;
    AML_NAMESPACE_NODE* Node;
    SIZE_T              PkgCodeOffset;
    SIZE_T              PkgCodeSize;

    //
    // Consume the instruction opcode if the caller has yet to do it.
    //
    if( ConsumeOpcode ) {
        if( AmlDecoderConsumeOpcode( State, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    //
    // Decode the PkgLength that encapsulates the rest of the operands and the TermList.
    //
    PkgStart = State->DataCursor;
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Validate the package size of the instruction (includes the entire TermList).
    //
    if( AmlDecoderIsValidDataWindow( State, PkgStart, PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Continue decoding the rest of the instruction using the read package length.
    //
    OriginalLength = State->DataLength;
    State->DataLength = ( State->DataCursor + ( PkgLength - ( State->DataCursor - PkgStart ) ) );

    //
    // Decode the rest of the instruction operands.
    // DefThermalZone: = ThermalZoneOp PkgLength NameString TermList
    //
    if( AmlDecoderConsumeNameString( State, &Name ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Print debug information about the name.
    //
    AML_DEBUG_TRACE( State, "ThermalZone(" );
    AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_TRACE, &Name );
    AML_DEBUG_TRACE( State, ")\n" );

    //
    // Check if the namespace pre-pass has already processed this.
    //
    Node = NULL;
    if( AmlNamespaceSearch( &State->Namespace, NULL, &Name, AML_SEARCH_FLAG_NAME_CREATION, &SearchNode ) ) {
        if( SearchNode->IsPreParsed && SearchNode->Object && SearchNode->Object->Type == AML_OBJECT_TYPE_THERMAL_ZONE ) {
            AML_DEBUG_TRACE( State, "Found pre-parsed thermal zone, skipping.\n" );
            Node = SearchNode;
        }
    }

    //
    // Create and setup a new namespace node/object if we haven't already.
    //
    if( Node == NULL ) {
        if( AmlStateSnapshotCreateNode( State, NULL, &Name, &Node ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_THERMAL_ZONE, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Update the namespace node with the created object.
        //
        Object->NamespaceNode = Node;
        Node->Object = Object;
        Node->IsPreParsed = ( State->PassType == AML_PASS_TYPE_NAMESPACE );
    }

    //
    // If this was a full evaluation pass, mark the node as evaluated.
    //
    Node->IsEvaluated = ( State->PassType == AML_PASS_TYPE_FULL );

    //
    // Get offset and length of the start of the TermList code embedded inside of the package.
    //
    PkgCodeOffset = ( State->DataCursor - PkgStart );
    if( PkgCodeOffset > PkgLength ) {
        return AML_FALSE;
    }
    PkgCodeSize = ( PkgLength - PkgCodeOffset );

    //
    // Push a new scope level for the TermList, and attempt to evaluate it.
    //
    if( AmlNamespacePushScope( &State->Namespace, &Node->AbsolutePath, 0 ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlEvalTermListCode( State, State->DataCursor, PkgCodeSize, AML_FALSE ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlNamespacePopScope( &State->Namespace ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Restore the original data window length.
    //
    State->DataLength = OriginalLength;
    return AML_TRUE;
}

//
// DefOpRegion := OpRegionOp NameString RegionSpace RegionOffset RegionLen
//
_Success_( return )
BOOLEAN
AmlEvalOpRegion(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    )
{
    AML_OBJECT*                  Object;
    AML_NAME_STRING              Name;
    UINT8                        SpaceType;
    AML_DATA                     RegionOffset;
    AML_DATA                     RegionLength;
    AML_OBJECT_OPERATION_REGION* OpRegion;
    AML_NAMESPACE_NODE*          Node;

    //
    // RegionSpace := ByteData
    // RegionOffset := TermArg => Integer
    // RegionLen := TermArg => Integer
    // DefOpRegion := OpRegionOp NameString RegionSpace RegionOffset RegionLen
    //
    if( AmlDecoderConsumeNameString( State, &Name ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlDecoderConsumeByte( State, &SpaceType ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &RegionOffset ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &RegionLength ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Create a namespace node for the region.
    //
    if( AmlStateSnapshotCreateNode( State, NULL, &Name, &Node ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Attempt to create a new reference counted object.
    // Don't worry about releasing our reference/memory upon failure here,
    // as failure is fatal to the entire state, and all memory is backed by
    // an underlying internal arena + heap.
    //
    if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_OPERATION_REGION, &Object ) == AML_FALSE ) {
        return AML_FALSE;
    }
    OpRegion = &Object->u.OpRegion;

    //
    // Initialize the operation region object.
    //
    *OpRegion = ( AML_OBJECT_OPERATION_REGION ){
        .Host      = State->Host,
        .Name      = Name,
        .Offset    = RegionOffset.u.Integer,
        .Length    = RegionLength.u.Integer,
        .SpaceType = SpaceType,
    };

    //
    // Copy region object to created node.
    //
    Object->NamespaceNode = Node;
    Node->Object = Object;
    return AML_TRUE;
}

//
// Evalute named object opcodes.
// NamedObj := DefBankField | DefCreateBitField | DefCreateByteField | DefCreateWordField
// | DefCreateDWordField | DefCreateQWordField | DefCreateField | DefDataRegion | DefExternal
// | DefOpRegion | DefPowerRes | DefThermalZone | DefDevice | DefEvent | DefField | DefIndexField
// | DefMethod | DefMutex | DefProcessor (deprecated)
//
_Success_( return )
BOOLEAN
AmlEvalNamedObjectOpcode(
    _Inout_ AML_STATE* State
    )
{
    AML_DECODER_INSTRUCTION_OPCODE Next;
    AML_OBJECT*                    Object;
    AML_NAMESPACE_NODE*            Node;
    AML_NAME_STRING                NameString;
    BOOLEAN                        Success;
    UINT8                          ByteData;
    UINT64                         HostHandle;

    //
    // Consume next instruction, this function should only be called if the next opcode was determined to be a named object opcode.
    //
    if( AmlDecoderConsumeOpcode( State, &Next ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Handle named object instructions.
    // TODO: PowerRes, ThermalZone.
    //
    switch( Next.OpcodeID ) {
    case AML_OPCODE_ID_POWER_RES_OP:
        //
        // DefPowerRes := PowerResOp PkgLength NameString SystemLevel ResourceOrder TermList
        //
        if( AmlEvalPowerRes( State, AML_FALSE ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case AML_OPCODE_ID_THERMAL_ZONE_OP:
        //
        // DefThermalZone := ThermalZoneOp PkgLength NameString TermList
        //
        if( AmlEvalThermalZone( State, AML_FALSE ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case AML_OPCODE_ID_EXTERNAL_OP:
        //
        // DefExternal := ExternalOp NameString ObjectType ArgumentCount
        //
        if( AmlEvalExternal( State, AML_FALSE ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case AML_OPCODE_ID_MUTEX_OP:
        //
        // SyncFlags := ByteData // bits 0-3: SyncLevel (0x00-0x0f), bits 4-7: Reserved (must be 0)
        // DefMutex := MutexOp NameString SyncFlags
        //
        if( AmlDecoderConsumeNameString( State, &NameString ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlDecoderConsumeByte( State, &ByteData ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Create an object and namespace node for the mutex.
        // TODO: Combine these two.
        //
        if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_MUTEX, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlStateSnapshotCreateNode( State, NULL, &NameString, &Node ) == AML_FALSE ) {
            AmlObjectRelease( Object );
            return AML_FALSE;
        }

        //
        // Attempt to create a host-internal mutex object.
        //
        if( AmlHostMutexCreate( State->Host, &HostHandle ) == AML_FALSE ) {
            AmlObjectRelease( Object );
            return AML_FALSE;
        }

        //
        // Initialize mutex object.
        //
        Object->u.Mutex = ( AML_OBJECT_MUTEX ){
            .Host       = State->Host,
            .HostHandle = HostHandle,
            .SyncLevel  = ( ByteData & 0xF )
        };

        //
        // Update namespace node with the created object.
        // TODO: Make a function that does this alongside creation of a namespace node,
        // currently this is left to work like this as we pass along our initial reference from AmlObjectCreate.
        //
        Object->NamespaceNode = Node;
        Node->Object = Object;
        break;
    case AML_OPCODE_ID_EVENT_OP:
        //
        // DefEvent := EventOp NameString
        //
        if( AmlDecoderConsumeNameString( State, &NameString ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Create an object and namespace node for the event.
        // TODO: Combine these two.
        //
        if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_EVENT, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlStateSnapshotCreateNode( State, NULL, &NameString, &Node ) == AML_FALSE ) {
            AmlObjectRelease( Object );
            return AML_FALSE;
        }

        //
        // Attempt to create a host-internal event object.
        //
        if( AmlHostEventCreate( State->Host, &HostHandle ) == AML_FALSE ) {
            AmlObjectRelease( Object );
            return AML_FALSE;
        }

        //
        // Initialize event object.
        //
        Object->u.Event = ( AML_OBJECT_EVENT ){
            .Host       = State->Host,
            .HostHandle = HostHandle,
            .State      = 0
        };

        //
        // Update namespace node with the created object.
        // TODO: Make a function that does this alongside creation of a namespace node,
        // currently this is left to work like this as we pass along our initial reference from AmlObjectCreate.
        //
        Object->NamespaceNode = Node;
        Node->Object = Object;
        break;
    case AML_OPCODE_ID_CREATE_BIT_FIELD_OP:
    case AML_OPCODE_ID_CREATE_BYTE_FIELD_OP:
    case AML_OPCODE_ID_CREATE_WORD_FIELD_OP:
    case AML_OPCODE_ID_CREATE_DWORD_FIELD_OP:
    case AML_OPCODE_ID_CREATE_QWORD_FIELD_OP:
    case AML_OPCODE_ID_CREATE_FIELD_OP:
        //
        // SourceBuff := TermArg => Buffer
        // BitIndex := TermArg => Integer
        // NumBits := TermArg => Integer
        // CreateXFieldOp SourceBuff BitIndex ?(NumBits) NameString
        //
        if( AmlEvalCreateBufferField( State, Next.OpcodeID ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case AML_OPCODE_ID_FIELD_OP:
        //
        // DefField := FieldOp PkgLength NameString FieldFlags FieldList
        //
        if( AmlEvalFieldOpcode( State ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case AML_OPCODE_ID_BANK_FIELD_OP:
        //
        // DefBankField := BankFieldOp PkgLength NameString NameString BankValue FieldFlags FieldList
        //
        if( AmlEvalBankFieldOpcode( State ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case AML_OPCODE_ID_INDEX_FIELD_OP:
        //
        // DefIndexField := IndexFieldOp PkgLength NameString NameString FieldFlags FieldList
        //
        if( AmlEvalIndexFieldOpcode( State ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case AML_OPCODE_ID_OP_REGION_OP:
        //
        // DefOpRegion := OpRegionOp NameString RegionSpace RegionOffset RegionLen
        //
        if( AmlEvalOpRegion( State, AML_FALSE ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case AML_OPCODE_ID_DATA_REGION_OP:
        //
        // Attempt to create a new reference counted object.
        //
        if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_DATA_REGION, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // RegionName := NameString
        // SignatureString := TermArg => String
        // OemIDString := TermArg => String
        // OemTableIDString := TermArg => String
        // DefDataRegion := DataRegionOp RegionName SignatureString OemIDString OemTableIDString
        //
        do {
            if( ( Success = AmlDecoderConsumeNameString( State, &Object->u.DataRegion.Name ) ) == AML_FALSE ) {
                break;
            } else if( ( Success = AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_STRING, &Object->u.DataRegion.SignatureString ) ) == AML_FALSE ) {
                break;
            } else if( ( Success = AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_STRING, &Object->u.DataRegion.OemIDString ) ) == AML_FALSE ) {
                break;
            } else if( ( Success = AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_STRING, &Object->u.DataRegion.OemTableIDString ) ) == AML_FALSE ) {
                break;
            }
        } while( 0 );

        //
        // Free object upon failure to parse any of the strings (in turn will free any parsed strings).
        //
        if( Success == AML_FALSE ) {
            AmlObjectRelease( Object );
            return AML_FALSE;
        }

        //
        // Create a namespace node for the region.
        //
        if( AmlStateSnapshotCreateNode( State, NULL, &Object->u.Device.Name, &Node ) == AML_FALSE ) {
            AmlObjectRelease( Object );
            return AML_FALSE;
        }

        //
        // Copy region to created node.
        //
        Object->NamespaceNode = Node;
        Node->Object = Object;
        break;
    case AML_OPCODE_ID_DEVICE_OP:
        //
        // Evaluate a DefDevice instruction.
        // DefDevice := DeviceOp PkgLength NameString TermList
        //
        if( AmlEvalDevice( State, AML_FALSE ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case AML_OPCODE_ID_METHOD_OP:
        //
        // Evaluate a DefMethod instruction.
        // DefMethod := MethodOp PkgLength NameString MethodFlags TermList
        //
        if( AmlEvalMethod( State, AML_FALSE ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    case AML_OPCODE_ID_PROCESSOR_OP:
        //
        // Evaluate a DefProcessor instruction.
        // DefProcessor := ProcessorOp PkgLength NameString ProcID PblkAddr PblkLen TermList
        //
        if( AmlEvalProcessor( State, AML_FALSE ) == AML_FALSE ) {
            return AML_FALSE;
        }
        break;
    default:
        AML_DEBUG_ERROR( State, "Error: Unsupported named object opcode!" );
        return AML_FALSE;
    }

    return AML_TRUE;
}
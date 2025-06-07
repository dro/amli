#include "aml_resource.h"

//
// Initialize a resource view/decoder referencing the given input buffer data.
//
VOID
AmlResourceViewInitialize(
    _Out_                        AML_RESOURCE_VIEW* View,
    _In_reads_bytes_( DataSize ) const UINT8*       Data,
    _In_                         SIZE_T             DataSize
    )
{
    *View = ( AML_RESOURCE_VIEW ){ .Data = Data, .DataSize = DataSize };
}

//
// Returns true once the resource view has reached the end of the input buffer.
//
BOOLEAN
AmlResourceViewEnd(
    _In_ const AML_RESOURCE_VIEW* View
    )
{
    return ( View->DataCursor >= View->DataSize );
}

//
// Peek information about the next descriptor in the view (tag, length),
// and validate that the descriptor length is fully contained by the view.
//
_Success_( return )
BOOLEAN
AmlResourceViewPeekHeader(
    _Inout_ AML_RESOURCE_VIEW* View,
    _Out_   UINT8*             pDescriptorTag,
    _Out_   SIZE_T*            pDescriptorHeaderSize,
    _Out_   SIZE_T*            pDescriptorDataSize
    )
{
    UINT8   Tag;
    SIZE_T  DescriptorStart;
    SIZE_T  HeaderSize;
    SIZE_T  DataSize;

    //
    // Attempt to peek the next resource descriptor's tag.
    //
    DescriptorStart = View->DataCursor;
    if( View->DataCursor >= View->DataSize ) {
        return AML_FALSE;
    }
    Tag = View->Data[ View->DataCursor ];

    //
    // Determine the size of the resource descriptor data payload.
    // The maximum size and encoding of the length differs depending on the type of descriptor.
    // Small descriptors encode a 3-bit length directly in their tag, large descriptors encode
    // a 16-bit length in the 2 bytes following the tag.
    //
    HeaderSize = 1;
    if( ( ( Tag & AML_RESOURCE_SMALL_TAG_TYPE( Tag ) ) != 0 ) ) {
        //
        // Byte 1 - Length of data items bits [7:0].
        // Byte 2 - Length of data items bits [15:8].
        //
        HeaderSize = 3;
        if( ( View->DataSize - View->DataCursor ) < HeaderSize ) {
            return AML_FALSE;
        }
        DataSize  = View->Data[ View->DataCursor + 1 ];
        DataSize |= ( ( ( SIZE_T )View->Data[ View->DataCursor + 2 ] ) << 8 );
    } else {
        //
        // Tag Bits [2:0] - Length (n) bytes.
        //
        DataSize = AML_RESOURCE_SMALL_TAG_LENGTH( Tag );
    }

    //
    // Ensure that the input data contains the entire resource description.
    //
    if( DataSize > ( View->DataSize - ( DescriptorStart + HeaderSize ) ) ) {
        return AML_FALSE;
    }

    //
    // Return the decoded tag and size of the next descriptor.
    //
    *pDescriptorTag        = Tag;
    *pDescriptorDataSize   = DataSize;
    *pDescriptorHeaderSize = HeaderSize;
    return AML_TRUE;
}

//
// Attempt to decode and move past the next full descriptor entry in the view.
//
_Success_( return )
BOOLEAN
AmlResourceViewRead(
    _Inout_ AML_RESOURCE_VIEW* View,
    _Out_   AML_RESOURCE*      Resource
    )
{
    UINT8  Tag;
    SIZE_T HeaderSize;
    SIZE_T DataSize;
    SIZE_T Size;
    SIZE_T CopySize;
    SIZE_T VldSize;
    SIZE_T VldOffset;

    //
    // Peek the tag and length of the next resource descriptor.
    // This function also validates the size and ensures that it is in bounds of the view.
    //
    if( AmlResourceViewPeekHeader( View, &Tag, &HeaderSize, &DataSize ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Initialize sizes, Size is the actual total amount of data in the descriptor,
    // including attached variable-length data. CopySize is the size of the fixed-length
    // data that has been directly copied to the AML_RESOURCE union. The VLD thus begins at (Start + CopySize).
    //
    Size = ( HeaderSize + DataSize );
    CopySize = 0;

    //
    // Default initialize all resource fields, necessary for certain variable-length descriptors.
    //
    *Resource = ( AML_RESOURCE ){ .Offset = View->DataCursor, .Size = Size };

    //
    // Handle specific resource types, copied using memcpy to the corresponding union field
    // individually to limit copy size, and to comply with strict aliasing rules.
    // Other resource-specific semantics are also handled here, like types with variable-length data.
    //
    switch( Tag ) {
    case AML_RESOURCE_TAG_IRQ_2:
        if( Size < sizeof( Resource->u.Irq2 ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.Irq2 );
        AML_MEMCPY( &Resource->u.Irq2, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_IRQ_3:
        if( Size < sizeof( Resource->u.Irq3 ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.Irq3 );
        AML_MEMCPY( &Resource->u.Irq3, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_DMA:
        if( Size < sizeof( Resource->u.Dma ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.Dma );
        AML_MEMCPY( &Resource->u.Dma, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_DMA_FIXED:
        if( Size < sizeof( Resource->u.DmaFixed ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.DmaFixed );
        AML_MEMCPY( &Resource->u.DmaFixed, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_START_DPF_PRIORITY:
        if( Size < sizeof( Resource->u.StartDpfPriority ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.StartDpfPriority );
        AML_MEMCPY( &Resource->u.StartDpfPriority, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_IO_PORT:
        if( Size < sizeof( Resource->u.IoPort ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.IoPort );
        AML_MEMCPY( &Resource->u.IoPort, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_IO_PORT_FIXED:
        if( Size < sizeof( Resource->u.IoPortFixed ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.IoPortFixed );
        AML_MEMCPY( &Resource->u.IoPortFixed, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_END_TAG:
        if( Size < sizeof( Resource->u.EndTag ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.EndTag );
        AML_MEMCPY( &Resource->u.EndTag, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_START_DPF:
        break;
    case AML_RESOURCE_TAG_END_DPF:
        break;

    //
    // Large resource descriptors.
    //
    case AML_RESOURCE_TAG_GENERIC_REGISTER:
        if( Size < sizeof( Resource->u.GenericRegister ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.GenericRegister );
        AML_MEMCPY( &Resource->u.GenericRegister, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_LARGE_VENDOR: /* VLA */
        if( Size < sizeof( Resource->u.VendorDefinedLarge ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.VendorDefinedLarge );
        AML_MEMCPY( &Resource->u.VendorDefinedLarge, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_MEMORY24:
        if( Size < sizeof( Resource->u.Memory24 ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.Memory24 );
        AML_MEMCPY( &Resource->u.Memory24, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_MEMORY32:
        if( Size < sizeof( Resource->u.Memory32 ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.Memory32 );
        AML_MEMCPY( &Resource->u.Memory32, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_MEMORY32_FIXED:
        if( Size < sizeof( Resource->u.Memory32Fixed ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.Memory32Fixed );
        AML_MEMCPY( &Resource->u.Memory32Fixed, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_EXTENDED_ADDRESS64: /* ResourceSource */
        if( Size < sizeof( Resource->u.ExtendedAddress64 ) ) {
            return AML_FALSE;
        }
        CopySize = sizeof( Resource->u.ExtendedAddress64 );
        AML_MEMCPY( &Resource->u.ExtendedAddress64, &View->Data[ View->DataCursor ], CopySize );
        break;

    //
    // Large resource descriptors with spec-defined minimum sizes.
    //
    case AML_RESOURCE_TAG_ADDRESS16: /* ResourceSource */
        if( DataSize < AML_RESOURCE_MIN_LENGTH_ADDRESS16 ) {
            return AML_FALSE;
        }
        CopySize = AML_MIN( Size, sizeof( Resource->u.Address16 ) );
        AML_MEMCPY( &Resource->u.Address16, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_ADDRESS32: /* ResourceSource */
        if( DataSize < AML_RESOURCE_MIN_LENGTH_ADDRESS32 ) {
            return AML_FALSE;
        }
        CopySize = AML_MIN( Size, sizeof( Resource->u.Address32 ) );
        AML_MEMCPY( &Resource->u.Address32, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_ADDRESS64: /* ResourceSource  */
        if( DataSize < AML_RESOURCE_MIN_LENGTH_ADDRESS64 ) {
            return AML_FALSE;
        }
        CopySize = AML_MIN( Size, sizeof( Resource->u.Address64 ) );
        AML_MEMCPY( &Resource->u.Address64, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_EXTENDED_INTERRUPT: /* VLA + ResourceSource */
        if( DataSize < AML_RESOURCE_MIN_LENGTH_EXTENDED_INTERRUPT ) {
            return AML_FALSE;
        }
        CopySize = AML_MIN( Size, sizeof( Resource->u.ExtendedInterrupt ) );
        AML_MEMCPY( &Resource->u.ExtendedInterrupt, &View->Data[ View->DataCursor ], CopySize );
        break;
    case AML_RESOURCE_TAG_PIN_FUNCTION:
        if( DataSize < AML_RESOURCE_MIN_LENGTH_PIN_FUNCTION ) {
            return AML_FALSE;
        }
        CopySize = AML_MIN( Size, sizeof( Resource->u.PinFunction ) );
        AML_MEMCPY( &Resource->u.PinFunction, &View->Data[ View->DataCursor ], CopySize );
        if( ( Resource->u.PinFunction.PinTableOffset >= Size )
            || ( Resource->u.PinFunction.VendorDataOffset >= Size )
            || ( Resource->u.PinFunction.ResourceSourceNameOffset >= Size )
            || ( Resource->u.PinFunction.VendorDataLength > ( Size - Resource->u.PinFunction.VendorDataOffset ) ) )
        {
            return AML_FALSE;
        }
        break;
    case AML_RESOURCE_TAG_PIN_CONFIGURATION:
        if( DataSize < AML_RESOURCE_MIN_LENGTH_PIN_CONFIGURATION ) {
            return AML_FALSE;
        }
        CopySize = AML_MIN( Size, sizeof( Resource->u.PinConfig ) );
        AML_MEMCPY( &Resource->u.PinConfig, &View->Data[ View->DataCursor ], CopySize );
        if( ( Resource->u.PinConfig.PinTableOffset >= Size )
            || ( Resource->u.PinConfig.VendorDataOffset >= Size )
            || ( Resource->u.PinConfig.ResourceSourceNameOffset >= Size )
            || ( Resource->u.PinConfig.VendorDataLength > ( Size - Resource->u.PinConfig.VendorDataOffset ) ) )
        {
            return AML_FALSE;
        }
        break;
    case AML_RESOURCE_TAG_PIN_GROUP:
        if( DataSize < AML_RESOURCE_MIN_LENGTH_PIN_GROUP ) {
            return AML_FALSE;
        }
        CopySize = AML_MIN( Size, sizeof( Resource->u.PinGroup ) );
        AML_MEMCPY( &Resource->u.PinGroup, &View->Data[ View->DataCursor ], CopySize );
        if( ( Resource->u.PinGroup.PinTableOffset >= Size )
            || ( Resource->u.PinGroup.VendorDataOffset >= Size )
            || ( Resource->u.PinGroup.ResourceLabelOffset >= Size )
            || ( Resource->u.PinGroup.VendorDataLength > ( Size - Resource->u.PinGroup.VendorDataOffset ) ) )
        {
            return AML_FALSE;
        }
        break;
    case AML_RESOURCE_TAG_PIN_GROUP_FUNCTION:
        if( DataSize < AML_RESOURCE_MIN_LENGTH_PIN_GROUP_FUNCTION ) {
            return AML_FALSE;
        }
        CopySize = AML_MIN( Size, sizeof( Resource->u.PinGroupFunction ) );
        AML_MEMCPY( &Resource->u.PinGroupFunction, &View->Data[ View->DataCursor ], CopySize );
        if( ( Resource->u.PinGroupFunction.ResourceSourceNameOffset >= Size )
            || ( Resource->u.PinGroupFunction.ResourceSourceLabelOffset >= Size )
            || ( Resource->u.PinGroupFunction.VendorDataOffset >= Size )
            || ( Resource->u.PinGroupFunction.VendorDataLength > ( Size - Resource->u.PinGroupFunction.VendorDataOffset ) ) )
        {
            return AML_FALSE;
        }
        break;
    case AML_RESOURCE_TAG_PIN_GROUP_CONFIG:
        if( DataSize < AML_RESOURCE_MIN_LENGTH_PIN_GROUP_CONFIG ) {
            return AML_FALSE;
        }
        CopySize = AML_MIN( Size, sizeof( Resource->u.PinGroupConfig ) );
        AML_MEMCPY( &Resource->u.PinGroupConfig, &View->Data[ View->DataCursor ], CopySize );
        if( ( Resource->u.PinGroupConfig.ResourceSourceNameOffset >= Size )
            || ( Resource->u.PinGroupConfig.ResourceSourceLabelOffset >= Size )
            || ( Resource->u.PinGroupConfig.VendorDataOffset >= Size )
            || ( Resource->u.PinGroupConfig.VendorDataLength > ( Size - Resource->u.PinGroupConfig.VendorDataOffset ) ) )
        {
            return AML_FALSE;
        }
        break;
    case AML_RESOURCE_TAG_GPIO_CONNECTION:
        if( DataSize < AML_RESOURCE_MIN_LENGTH_GPIO_CONNECTION ) {
            return AML_FALSE;
        }
        CopySize = AML_MIN( Size, sizeof( Resource->u.Gpio ) );
        AML_MEMCPY( &Resource->u.Gpio, &View->Data[ View->DataCursor ], CopySize );
        if( Resource->u.Gpio.PinTableOffset >= Size
            || Resource->u.Gpio.ResourceSourceNameOffset >= Size
            || ( Resource->u.Gpio.VendorDataOffset >= Size )
            || ( Resource->u.Gpio.VendorDataLength > ( Size - Resource->u.Gpio.VendorDataOffset ) ) )
        {
            return AML_FALSE;
        }
        break;
    case AML_RESOURCE_TAG_GSB_CONNECTION:
        if( DataSize < AML_RESOURCE_MIN_LENGTH_SERIAL_BUS ) {
            return AML_FALSE;
        }
        CopySize = AML_MIN( Size, sizeof( Resource->u.SerialBus ) );
        AML_MEMCPY( &Resource->u.SerialBus, &View->Data[ View->DataCursor ], CopySize );
        switch( Resource->u.SerialBus.SerialBusType ) { /* (1=I2C, 2=SPI, 3=UART, 4=CSI-2) */
        case AML_RESOURCE_SERIAL_BUS_TYPE_I2C:
            if( DataSize < AML_RESOURCE_MIN_LENGTH_I2C ) {
                return AML_FALSE;
            }
            CopySize = AML_MIN( Size, sizeof( Resource->u.I2c ) );
            AML_MEMCPY( &Resource->u.I2c, &View->Data[ View->DataCursor ], CopySize );
            break;
        case AML_RESOURCE_SERIAL_BUS_TYPE_SPI:
            if( DataSize < AML_RESOURCE_MIN_LENGTH_SPI ) {
                return AML_FALSE;
            }
            CopySize = AML_MIN( Size, sizeof( Resource->u.Spi ) );
            AML_MEMCPY( &Resource->u.Spi, &View->Data[ View->DataCursor ], CopySize );
            break;
        case AML_RESOURCE_SERIAL_BUS_TYPE_UART:
            if( DataSize < AML_RESOURCE_MIN_LENGTH_UART ) {
                return AML_FALSE;
            }
            CopySize = AML_MIN( Size, sizeof( Resource->u.Uart ) );
            AML_MEMCPY( &Resource->u.Uart, &View->Data[ View->DataCursor ], CopySize );
            break;
        case AML_RESOURCE_SERIAL_BUS_TYPE_CSI2:
            if( DataSize < AML_RESOURCE_MIN_LENGTH_CSI2 ) {
                return AML_FALSE;
            }
            CopySize = AML_MIN( Size, sizeof( Resource->u.Csi2 ) );
            AML_MEMCPY( &Resource->u.Csi2, &View->Data[ View->DataCursor ], CopySize );
            break;
        }
        break;
    case AML_RESOURCE_TAG_CLOCK_INPUT:
        break;
    }

    //
    // Determine the amount of extra variable-length data attached to the descriptor.
    //
    VldSize = ( Size - CopySize );
    VldOffset = CopySize;
    if( Size < CopySize ) {
        return AML_FALSE;
    }

    //
    // Update information about any possible variable-length data attached to the descriptor.
    //
    Resource->VldOffset = VldOffset;
    Resource->VldSize = VldSize;

    //
    // Move the view forward past the entire descriptor.
    //
    View->DataCursor += Size;
    return AML_TRUE;
}
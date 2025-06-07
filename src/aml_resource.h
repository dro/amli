#pragma once

#include "aml_resource_spec.h"

//
// Current decoder view of an AML resource buffer.
//
typedef struct _AML_RESOURCE_VIEW {
    SIZE_T       DataCursor;
    const UINT8* Data;
    SIZE_T       DataSize;
} AML_RESOURCE_VIEW;

//
// Decoded resource information, contains the parsed fixed-length descriptor portion, 
// any variable-length data is not directly included in this structure, but can be accessed
// by the user using the VldSize and VldOffset fields to access the original source buffer.
//
typedef struct _AML_RESOURCE {
    //
    // Decoded resource information.
    // Size includes any attached variable-length data following the fixed portion of the descriptor.
    //
    SIZE_T Offset;
    SIZE_T Size;

    //
    // Variable-length data information.
    // This is any amount of data that follows the the fixed-length structure that has been decoded to the union.
    //
    SIZE_T VldSize;
    SIZE_T VldOffset;

    //
    // Decoded resource descriptor data without attached variable-length data.
    //
    union {
        UINT8                           Tag;
        AML_RESOURCE_IRQ_2              Irq2;
        AML_RESOURCE_IRQ_3              Irq3;
        AML_RESOURCE_DMA                Dma;
        AML_RESOURCE_DMA_FIXED          DmaFixed;
        AML_RESOURCE_START_DPF_PRIORITY StartDpfPriority;
        AML_RESOURCE_IO_PORT            IoPort;
        AML_RESOURCE_IO_PORT_FIXED      IoPortFixed;
        AML_RESOURCE_END_TAG            EndTag;
        AML_RESOURCE_GPIO_CONNECTION    Gpio;
        AML_RESOURCE_MEMORY24           Memory24;
        AML_RESOURCE_MEMORY32           Memory32;
        AML_RESOURCE_MEMORY32_FIXED     Memory32Fixed;
        AML_RESOURCE_VENDOR_LARGE       VendorDefinedLarge;
        AML_RESOURCE_ADDRESS16          Address16;
        AML_RESOURCE_ADDRESS32          Address32;
        AML_RESOURCE_ADDRESS64          Address64;
        AML_RESOURCE_EXTENDED_ADDRESS64 ExtendedAddress64;
        AML_RESOURCE_EXTENDED_INTERRUPT ExtendedInterrupt;
        AML_RESOURCE_GENERIC_REGISTER   GenericRegister;
        AML_RESOURCE_SERIAL_BUS         SerialBus;
        AML_RESOURCE_I2C                I2c;
        AML_RESOURCE_SPI                Spi;
        AML_RESOURCE_UART               Uart;
        AML_RESOURCE_CSI2               Csi2;
        AML_RESOURCE_PIN_FUNCTION       PinFunction;
        AML_RESOURCE_PIN_CONFIGURATION  PinConfig;
        AML_RESOURCE_PIN_GROUP          PinGroup;
        AML_RESOURCE_PIN_GROUP_FUNCTION PinGroupFunction;
        AML_RESOURCE_PIN_GROUP_CONFIG   PinGroupConfig;
    } u;
} AML_RESOURCE;

//
// Initialize a resource view/decoder referencing the given input buffer data.
//
VOID
AmlResourceViewInitialize(
    _Out_                        AML_RESOURCE_VIEW* View,
    _In_reads_bytes_( DataSize ) const UINT8*       Data,
    _In_                         SIZE_T             DataSize
    );

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
    );

//
// Attempt to decode and move past the next full descriptor entry in the view.
//
_Success_( return )
BOOLEAN
AmlResourceViewRead(
    _Inout_ AML_RESOURCE_VIEW* View,
    _Out_   AML_RESOURCE*      Resource
    );
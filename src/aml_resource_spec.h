#pragma once

#include "aml_platform.h"

//
// Small Resource Data Type.
// Byte 0:
//  Tag Bits [2:0] - Length-n bytes.
//  Tag Bits [6:3] - Small item name.
//  Tag Bit  [7]   - Type-0 (Small item)
// 
// Bytes 1 to n:
//  Data bytes (Length 0 - 7)
//
#define AML_RESOURCE_SMALL_TAG_LENGTH_SHIFT 0
#define AML_RESOURCE_SMALL_TAG_LENGTH_MASK  ((1 << ((2-0)+1)) - 1)
#define AML_RESOURCE_SMALL_TAG_LENGTH(Tag) \
    (((Tag) >> AML_RESOURCE_SMALL_TAG_LENGTH_SHIFT) & AML_RESOURCE_SMALL_TAG_LENGTH_MASK)
#define AML_RESOURCE_SMALL_TAG_NAME_SHIFT 3
#define AML_RESOURCE_SMALL_TAG_NAME_MASK  ((1 << ((6-3)+1)) - 1)
#define AML_RESOURCE_SMALL_TAG_NAME(Tag) \
    (((Tag) >> AML_RESOURCE_SMALL_TAG_NAME_SHIFT) & AML_RESOURCE_SMALL_TAG_NAME_MASK)
#define AML_RESOURCE_SMALL_TAG_TYPE_SHIFT 7
#define AML_RESOURCE_SMALL_TAG_TYPE_MASK  1
#define AML_RESOURCE_SMALL_TAG_TYPE(Tag) \
    (((Tag) >> AML_RESOURCE_SMALL_TAG_TYPE_SHIFT) & AML_RESOURCE_SMALL_TAG_TYPE_MASK)

//
// Small resource item names.
// 
#define AML_RESOURCE_SMALL_NAME_IRQ_FORMAT     0x04 /* IRQ Format Descriptor*/
#define AML_RESOURCE_SMALL_NAME_DMA_FORMAT     0x05 /* DMA Format Descriptor */
#define AML_RESOURCE_SMALL_NAME_START_DPF      0x06 /* Start Dependent Functions Descriptor */
#define AML_RESOURCE_SMALL_NAME_END_DPF        0x07 /* End Dependent Functions Descriptor */
#define AML_RESOURCE_SMALL_NAME_IO_PORT        0x08 /* I/O Port Descriptor */
#define AML_RESOURCE_SMALL_NAME_IO_PORT_FIXED  0x09 /* Fixed Location I/O Port Descriptor */
#define AML_RESOURCE_SMALL_NAME_DMA_FIXED      0x0A /* Fixed DMA Descriptor */
#define AML_RESOURCE_SMALL_NAME_VENDOR_DEFINED 0x0E /* Vendor Defined Descriptor */
#define AML_RESOURCE_SMALL_NAME_END_TAG        0x0F /* End Tag Descriptor */

//
// IRQ descriptor (length 2).
// Value = 0x22 (Type = 0, Small item name = 0x4, Length = 2).
// 
// The IRQ data structure indicates that the device uses an interrupt level and supplies a mask
// with bits set indicating the levels implemented in this device.
// For standard PC-AT implementation there are 15 possible interrupts so a two-byte field is used.
// This structure is repeated for each separate interrupt required.
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_IRQ_2 {
    UINT8 Tag;
    UINT8 Mask0; /* IRQ mask bits[7:0], _INT Bit [0] represents IRQ0, bit[1] is IRQ1, and so on. */
    UINT8 Mask1; /* IRQ mask bits[15:8], _INT Bit [0] represents IRQ8, bit[1] is IRQ9, and so on. */
} AML_RESOURCE_IRQ_2;
#pragma pack(pop)

//
// IRQ descriptor (length 3).
// Value = 0x23 (Type = 0, Small item name = 0x4, Length = 3).
// 
// The IRQ data structure indicates that the device uses an interrupt level and supplies a mask
// with bits set indicating the levels implemented in this device.
// For standard PC-AT implementation there are 15 possible interrupts so a two-byte field is used.
// This structure is repeated for each separate interrupt required.
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_IRQ_3 {
    UINT8 Tag;
    UINT8 Mask0; /* IRQ mask bits[7:0], _INT Bit [0] represents IRQ0, bit[1] is IRQ1, and so on. */
    UINT8 Mask1; /* IRQ mask bits[15:8], _INT Bit [0] represents IRQ8, bit[1] is IRQ9, and so on. */
    UINT8 Info;  /* IRQ Information. Each bit, when set, indicates this device is capable of driving a certain type of interrupt. */
} AML_RESOURCE_IRQ_3;
#pragma pack(pop)

//
// AML_RESOURCE_IRQ_3 information bitfields.
//
#define AML_RESOURCE_IRQ_MODE_SHIFT 0 /* Bit [0] Interrupt Mode, _HE */
#define AML_RESOURCE_IRQ_MODE_MASK  1
#define AML_RESOURCE_IRQ_MODE(Info) (((Info) >> AML_RESOURCE_IRQ_MODE_SHIFT) & AML_RESOURCE_IRQ_MODE_MASK)
    #define AML_RESOURCE_IRQ_MODE_LEVEL 0 /* Level-Triggered */
    #define AML_RESOURCE_IRQ_MODE_EDGE  1 /* Edge-Triggered */
#define AML_RESOURCE_IRQ_POLARITY_SHIFT 3 /* Bit [3] Interrupt Polarity, _LL */
#define AML_RESOURCE_IRQ_POLARITY_MASK  1
#define AML_RESOURCE_IRQ_POLARITY(Info) (((Info) >> AML_RESOURCE_IRQ_POLARITY_SHIFT) & AML_RESOURCE_IRQ_POLARITY_MASK)
    #define AML_RESOURCE_IRQ_POLARITY_HIGH 0 /* Active-High */
    #define AML_RESOURCE_IRQ_POLARITY_LOW  1 /* Active-Low */
#define AML_RESOURCE_IRQ_SHARING_SHIFT 4 /* Bit [4] Interrupt Sharing, _SHR */
#define AML_RESOURCE_IRQ_SHARING_MASK  1
#define AML_RESOURCE_IRQ_SHARING(Info) (((Info) >> AML_RESOURCE_IRQ_SHARING_SHIFT) & AML_RESOURCE_IRQ_SHARING_MASK)
    #define AML_RESOURCE_IRQ_EXCLUSIVE 0 /* Not shared with other devices */
    #define AML_RESOURCE_IRQ_SHARED    1 /* Shared with other devices */
#define AML_RESOURCE_IRQ_WAKE_SHIFT 5 /* Bit [5] Wake Capability, _WKC */
#define AML_RESOURCE_IRQ_WAKE_MASK  1
#define AML_RESOURCE_IRQ_WAKE(Info) (((Info) >> AML_RESOURCE_IRQ_WAKE_SHIFT) & AML_RESOURCE_IRQ_WAKE_MASK)
    #define AML_RESOURCE_IRQ_NOT_WAKE_CAPABLE 0 /* Not capable of waking the system */
    #define AML_RESOURCE_IRQ_WAKE_CAPABLE     1 /* Capable of waking the system */

//
// DMA Descriptor.
// Value = 0x2A (Type = 0, Small item name = 0x5, Length = 2).
// 
// The DMA data structure indicates that the device uses a DMA channel and supplies
// a mask with bits set indicating the channels actually implemented in this device.
// This structure is repeated for each separate channel required.
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_DMA {
    UINT8 Tag;
    UINT8 ChannelMask; /* DMA channel mask bits [7:0] - _DMA Bit [0] is channel 0, etc. */
    UINT8 Info;        /* DMA Information byte */
} AML_RESOURCE_DMA;
#pragma pack(pop)

//
// DMA descriptor information bitfields.
//
#define AML_RESOURCE_DMA_TRANSFER_TYPE_SHIFT 0 /* Bits [1:0] DMA transfer type preference, _SIZ */
#define AML_RESOURCE_DMA_TRANSFER_TYPE_MASK  ((1 << ((1-0)+1)) - 1)
#define AML_RESOURCE_DMA_TRANSFER_TYPE(Info) (((Info) >> AML_RESOURCE_DMA_TRANSFER_TYPE_SHIFT) & AML_RESOURCE_DMA_TRANSFER_TYPE_MASK)
    #define AML_RESOURCE_DMA_8BIT_ONLY      0x00  /* 8-bit only */
    #define AML_RESOURCE_DMA_8BIT_AND_16BIT 0x01  /* 8-bit and 16-bit */
    #define AML_RESOURCE_DMA_16BIT_ONLY     0x02  /* 16-bit only */
#define AML_RESOURCE_DMA_BUS_MASTER_SHIFT 2 /* Bit [2] Logical device bus master status, _BM */
#define AML_RESOURCE_DMA_BUS_MASTER_MASK  1
#define AML_RESOURCE_DMA_BUS_MASTER(Info) (((Info) >> AML_RESOURCE_DMA_BUS_MASTER_SHIFT) & AML_RESOURCE_DMA_BUS_MASTER_MASK)
    #define AML_RESOURCE_DMA_NOT_BUS_MASTER 0  /* Logical device is not a bus master */
    #define AML_RESOURCE_DMA_IS_BUS_MASTER  1  /* Logical device is a bus master */
#define AML_RESOURCE_DMA_SPEED_SHIFT 5 /* Bits [6:5] DMA channel speed supported, _TYP */
#define AML_RESOURCE_DMA_SPEED_MASK  ((1 << ((6-5)+1)) - 1)
#define AML_RESOURCE_DMA_SPEED(Info) (((Info) >> AML_RESOURCE_DMA_SPEED_SHIFT) & AML_RESOURCE_DMA_SPEED_MASK)
    #define AML_RESOURCE_DMA_COMPATIBILITY 0x00  /* Compatibility mode */
    #define AML_RESOURCE_DMA_TYPE_A        0x01  /* Type A DMA */
    #define AML_RESOURCE_DMA_TYPE_B        0x02  /* Type B DMA */
    #define AML_RESOURCE_DMA_TYPE_F        0x03  /* Type F */

//
// Start Dependent Functions Descriptor with Priority.
// Value = 0x31 (Type = 0, small item name = 0x6, Length = 1)
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_START_DPF_PRIORITY {
    UINT8 Tag;
    UINT8 Priority;
} AML_RESOURCE_START_DPF_PRIORITY;
#pragma pack(pop)

//
// Dependent functions priority bitfields.
//
#define AML_RESOURCE_PRIORITY_COMPATIBILITY_SHIFT 0 /* Bits [1:0] Compatibility priority */
#define AML_RESOURCE_PRIORITY_COMPATIBILITY_MASK  ((1 << ((1-0)+1)) - 1)
#define AML_RESOURCE_PRIORITY_COMPATIBILITY(Priority) \
    (((Priority) >> AML_RESOURCE_PRIORITY_COMPATIBILITY_SHIFT) & AML_RESOURCE_PRIORITY_COMPATIBILITY_MASK)
#define AML_RESOURCE_PRIORITY_PERFORMANCE_SHIFT 2 /* Bits [3:2] Performance/robustness */
#define AML_RESOURCE_PRIORITY_PERFORMANCE_MASK  ((1 << ((3-2)+1)) - 1)
#define AML_RESOURCE_PRIORITY_PERFORMANCE(Priority) \
    (((Priority) >> AML_RESOURCE_PRIORITY_PERFORMANCE_SHIFT) & AML_RESOURCE_PRIORITY_PERFORMANCE_MASK)
    #define AML_RESOURCE_PRIORITY_GOOD        0 /* Good configuration: Highest Priority and preferred */
    #define AML_RESOURCE_PRIORITY_ACCEPTABLE  1 /* Acceptable configuration: Lower Priority but acceptable */
    #define AML_RESOURCE_PRIORITY_SUB_OPTIMAL 2 /* Sub-optimal configuration: Functional but not optimal */

//
// I/O Port Descriptor.
// Value = 0x47 (Type = 0, Small item name = 0x8, Length = 7).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_IO_PORT {
    UINT8  Tag;
    UINT8  Info;
    UINT16 RangeMinimum;  /* Range minimum base address, _MIN - Address bits [15:0] */
    UINT16 RangeMaximum;  /* Range maximum base address, _MAX - Address bits [15:0] */
    UINT8  BaseAlignment; /* Base alignment, _ALN - Alignment for minimum base address */
    UINT8  RangeLength;   /* Range length, _LEN - Number of contiguous I/O ports requested */
} AML_RESOURCE_IO_PORT;
#pragma pack(pop)

//
// I/O port information bitfields.
//
#define AML_RESOURCE_IO_DECODE_SHIFT 0 /* Bit [0] (_DEC) Address decode */
#define AML_RESOURCE_IO_DECODE_MASK  1
#define AML_RESOURCE_IO_DECODE(Info) (((Info) >> AML_RESOURCE_IO_DECODE_SHIFT) & AML_RESOURCE_IO_DECODE_MASK)
    #define AML_RESOURCE_IO_DECODE_10BIT 0 /* Logical device only decodes address bits [9:0] */
    #define AML_RESOURCE_IO_DECODE_16BIT 1 /* Logical device decodes 16-bit addresses */

//
// Fixed Location I/O Port Descriptor.
// Value = 0x4B (Type = 0, Small item name = 0x9, Length = 3).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_IO_PORT_FIXED {
    UINT8  Tag;
    UINT16 BaseAddress;   /* Range base address, _BAS - Address bits [9:0] for 10-bit ISA decode */
    UINT8  RangeLength;   /* Range length, _LEN - Number of contiguous I/O ports requested */
} AML_RESOURCE_IO_PORT_FIXED;
#pragma pack(pop)

//
// Fixed DMA Descriptor.
// Value = 0x55 (Type = 0, Small item name = 0xA, Length = 0x5).
//
#pragma pack(push, 1)
typedef struct {
    UINT8  Tag;
    UINT16 DmaRequestLine; /* DMA Request Line bits [15:0] _DMA[15:0] */
    UINT16 DmaChannel;     /* DMA Channel bits[15:0] _TYP[15:0] */
    UINT8  TransferWidth;  /* DMA Transfer Width, _SIZ */
} AML_RESOURCE_DMA_FIXED;
#pragma pack(pop)

//
// Fixed DMA transfer width values.
//
#define AML_RESOURCE_DMA_FIXED_TRW_8BIT   0x00
#define AML_RESOURCE_DMA_FIXED_TRW_16BIT  0x01
#define AML_RESOURCE_DMA_FIXED_TRW_32BIT  0x02
#define AML_RESOURCE_DMA_FIXED_TRW_64BIT  0x03
#define AML_RESOURCE_DMA_FIXED_TRW_128BIT 0x04
#define AML_RESOURCE_DMA_FIXED_TRW_256BIT 0x05

//
// End Tag Descriptor.
// Value = 0x79 (Type = 0, Small item name = 0xF, Length = 1).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_END_TAG {
    UINT8 Tag;
    UINT8 Checksum; /* Checksum covering all resource data after the serial identifier */
} AML_RESOURCE_END_TAG;
#pragma pack(pop)

//
// Common predefined small tag values, does not include vendor defined tag,
// users should manually check the type/name and use the provided length.
//
#define AML_RESOURCE_MAKE_SMALL_TAG(Name, Length) \
    (((Name) << AML_RESOURCE_SMALL_TAG_NAME_SHIFT) | ((Length) << AML_RESOURCE_SMALL_TAG_LENGTH_SHIFT))
#define AML_RESOURCE_TAG_IRQ_2              AML_RESOURCE_MAKE_SMALL_TAG(0x4, 0x2)
#define AML_RESOURCE_TAG_IRQ_3              AML_RESOURCE_MAKE_SMALL_TAG(0x4, 0x3)
#define AML_RESOURCE_TAG_DMA                AML_RESOURCE_MAKE_SMALL_TAG(0x5, 0x2)
#define AML_RESOURCE_TAG_START_DPF          AML_RESOURCE_MAKE_SMALL_TAG(0x6, 0x0)
#define AML_RESOURCE_TAG_START_DPF_PRIORITY AML_RESOURCE_MAKE_SMALL_TAG(0x6, 0x1)
#define AML_RESOURCE_TAG_END_DPF            AML_RESOURCE_MAKE_SMALL_TAG(0x7, 0x0)
#define AML_RESOURCE_TAG_IO_PORT            AML_RESOURCE_MAKE_SMALL_TAG(0x8, 0x7)
#define AML_RESOURCE_TAG_IO_PORT_FIXED      AML_RESOURCE_MAKE_SMALL_TAG(0x9, 0x3)
#define AML_RESOURCE_TAG_DMA_FIXED          AML_RESOURCE_MAKE_SMALL_TAG(0xA, 0x5)
#define AML_RESOURCE_TAG_SMALL_VENDOR       AML_RESOURCE_MAKE_SMALL_TAG(0xE, 0x0)
#define AML_RESOURCE_TAG_END_TAG            AML_RESOURCE_MAKE_SMALL_TAG(0xF, 0x1)

//
// Common large resource header.
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_LARGE_HEADER {
    UINT8  Tag;
    UINT16 Length;
} MAL_RESOURCE_LARGE_HEADER;
#pragma pack(pop)

//
// GPIO Connection Descriptor, Type 1.
// Value = 0x8C (Type = 1, Large item name = 0x0C).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_GPIO_CONNECTION {
    UINT8  Tag;                       // GPIO Connection Descriptor (0x8C)
    UINT16 Length;                    // Variable length, minimum 0x16 + L
    UINT8  RevisionId;                // Revision ID (must be 1)
    UINT8  ConnectionType;            // GPIO Connection Type (0x00=Interrupt, 0x01=IO)
    UINT16 GeneralFlags;              // General flags
    UINT16 InterruptIoFlags;          // Interrupt and IO flags
    UINT8  PinConfiguration;          // Pin Configuration (_PPI)
    UINT16 OutputDriveStrength;       // Output drive strength in hundredths of mA
    UINT16 DebounceTimeout;           // Debounce timeout in hundredths of ms
    UINT16 PinTableOffset;            // Offset to pin table
    UINT8  ResourceSourceIndex;       // Reserved (must be 0)
    UINT16 ResourceSourceNameOffset;  // Offset to resource source name
    UINT16 VendorDataOffset;          // Offset to vendor data
    UINT16 VendorDataLength;          // Length of vendor data
//  UINT16 PinTable[ 0 ];             // Pin numbers (starting at PinTableOffset)
//  CHAR8  ResourceSourceName[ 0 ];   // GPIO controller name (at ResourceSourceNameOffset)
//  UINT8  VendorData[ 0 ];           // Vendor-defined data (at VendorDataOffset)
} AML_RESOURCE_GPIO_CONNECTION;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_GPIO_CONNECTION 22

//
// GPIO Connection Type values
//
#define AML_GPIO_CONNECTION_TYPE_INTERRUPT  0x00
#define AML_GPIO_CONNECTION_TYPE_IO         0x01

//
// General Flags bitfields
//
#define AML_GPIO_GENERAL_FLAGS_CONSUMER_PRODUCER_SHIFT  0
#define AML_GPIO_GENERAL_FLAGS_CONSUMER_PRODUCER_MASK   0x01
#define AML_GPIO_GENERAL_FLAGS_CONSUMER                 0x01
#define AML_GPIO_GENERAL_FLAGS_PRODUCER                 0x00

//
// Interrupt Connection Flags (when ConnectionType = 0x00)
//
#define AML_GPIO_INTERRUPT_MODE_SHIFT           0   /* Bit 0 */
#define AML_GPIO_INTERRUPT_MODE_MASK            0x01
#define AML_GPIO_INTERRUPT_MODE_LEVEL           0x00
#define AML_GPIO_INTERRUPT_MODE_EDGE            0x01
#define AML_GPIO_INTERRUPT_POLARITY_SHIFT       1   /* Bits 2:1 */
#define AML_GPIO_INTERRUPT_POLARITY_MASK        0x03
#define AML_GPIO_INTERRUPT_POLARITY_HIGH        0x00
#define AML_GPIO_INTERRUPT_POLARITY_LOW         0x01
#define AML_GPIO_INTERRUPT_POLARITY_BOTH        0x02
#define AML_GPIO_INTERRUPT_SHARING_SHIFT        3   /* Bit 3 */
#define AML_GPIO_INTERRUPT_SHARING_MASK         0x01
#define AML_GPIO_INTERRUPT_SHARING_EXCLUSIVE    0x00
#define AML_GPIO_INTERRUPT_SHARING_SHARED       0x01
#define AML_GPIO_INTERRUPT_WAKE_SHIFT           4   /* Bit 4 */
#define AML_GPIO_INTERRUPT_WAKE_MASK            0x01
#define AML_GPIO_INTERRUPT_WAKE_NOT_CAPABLE     0x00
#define AML_GPIO_INTERRUPT_WAKE_CAPABLE         0x01

//
// IO Connection Flags (when ConnectionType = 0x01)
//
#define AML_GPIO_IO_RESTRICTION_SHIFT           0   /* Bits 1:0 */
#define AML_GPIO_IO_RESTRICTION_MASK            0x03
#define AML_GPIO_IO_RESTRICTION_INPUT_OUTPUT    0x00
#define AML_GPIO_IO_RESTRICTION_INPUT_ONLY      0x01
#define AML_GPIO_IO_RESTRICTION_OUTPUT_ONLY     0x02
#define AML_GPIO_IO_RESTRICTION_PRESERVE        0x03

#define AML_GPIO_IO_SHARING_SHIFT               3   /* Bit 3 */
#define AML_GPIO_IO_SHARING_MASK                0x01
#define AML_GPIO_IO_SHARING_EXCLUSIVE           0x00
#define AML_GPIO_IO_SHARING_SHARED              0x01

//
// Pin Configuration values
//
#define AML_GPIO_PIN_CONFIG_DEFAULT             0x00
#define AML_GPIO_PIN_CONFIG_PULL_UP             0x01
#define AML_GPIO_PIN_CONFIG_PULL_DOWN           0x02
#define AML_GPIO_PIN_CONFIG_NO_PULL             0x03
//
// Special pin number value
//
#define AML_GPIO_PIN_NUMBER_NO_PIN              0xFFFF

//
// 24-Bit Memory Range Descriptor.
// Value = 0x81 (Type = 1, Large item name = 0x01).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_MEMORY24 {
    UINT8  Tag;
    UINT16 Length;        /* Value = 0x0009 */
    UINT8  Information;
    UINT16 RangeMinimum;  /* Range minimum base address, _MIN - Address bits [23:8] */
    UINT16 RangeMaximum;  /* Range maximum base address, _MAX - Address bits [23:8] */
    UINT16 BaseAlignment; /* Base alignment, _ALN - Alignment for minimum base address */
    UINT16 RangeLength;   /* Range length, _LEN - Length in 256 byte blocks */
} AML_RESOURCE_MEMORY24;
#pragma pack(pop)

//
// 24-bit memory information bitfields.
//
#define AML_RESOURCE_MEMORY24_WRITE_STATUS_SHIFT 0 /* Bit [0] (_RW) Write status */
#define AML_RESOURCE_MEMORY24_WRITE_STATUS_MASK  1
#define AML_RESOURCE_MEMORY24_WRITE_STATUS(Info) (((Info) >> AML_RESOURCE_MEMORY24_WRITE_STATUS_SHIFT) & AML_RESOURCE_MEMORY24_WRITE_STATUS_MASK)
    #define AML_RESOURCE_MEMORY24_READ_ONLY  0 /* Non-writeable (read-only) */
    #define AML_RESOURCE_MEMORY24_READ_WRITE 1 /* Writeable (read/write) */

//
// Vendor-Defined Descriptor, Type 1.
// Value = 0x84 (Type = 1, Large item name = 0x04).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_VENDOR_LARGE {
    UINT8  Tag;
    UINT16 Length;          /* Variable length (UUID and vendor data) */
    UINT8  SubType;         /* UUID specific descriptor sub type */
    UINT8  UUID[ 16 ];      /* UUID Value */
//  UINT8  VendorData[ 0 ]; /* Vendor defined data bytes */
} AML_RESOURCE_VENDOR_LARGE;
#pragma pack(pop)

//
// 32-Bit Memory Range Descriptor.
// Value = 0x85 (Type = 1, Large item name = 0x05).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_MEMORY32 {
    UINT8  Tag;
    UINT16 Length;        /* Value = 0x0011 (17) */
    UINT8  Information;
    UINT32 RangeMinimum;  /* Range minimum base address, _MIN - Address bits [31:0] */
    UINT32 RangeMaximum;  /* Range maximum base address, _MAX - Address bits [31:0] */
    UINT32 BaseAlignment; /* Base alignment, _ALN - Alignment for minimum base address */
    UINT32 RangeLength;   /* Range length, _LEN - Length in 1-byte blocks */
} AML_RESOURCE_MEMORY32;
#pragma pack(pop)

//
// 32-bit memory information bitfields.
//
#define AML_RESOURCE_MEMORY32_WRITE_STATUS_SHIFT 0 /* Bit [0] (_RW) Write status */
#define AML_RESOURCE_MEMORY32_WRITE_STATUS_MASK  1
#define AML_RESOURCE_MEMORY32_WRITE_STATUS(Info) \
    (((Info) >> AML_RESOURCE_MEMORY32_WRITE_STATUS_SHIFT) & AML_RESOURCE_MEMORY32_WRITE_STATUS_MASK)
    #define AML_RESOURCE_MEMORY32_READ_ONLY  0 /* Non-writeable (read-only) */
    #define AML_RESOURCE_MEMORY32_READ_WRITE 1 /* Writeable (read/write) */

//
// 32-Bit Fixed Memory Range Descriptor.
// Value = 0x86 (Type = 1, Large item name = 0x06).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_MEMORY32_FIXED {
    UINT8  Tag;
    UINT16 Length;      /* Value = 0x0009 (9) */
    UINT8  Information;
    UINT32 BaseAddress; /* Range base address, _BAS - Address bits [31:0] */
    UINT32 RangeLength; /* Range length, _LEN - Length in 1-byte blocks */
} AML_RESOURCE_MEMORY32_FIXED;
#pragma pack(pop)

//
// 32-bit fixed memory information bitfields.
//
#define AML_RESOURCE_MEMORY32_FIXED_WRITE_STATUS_SHIFT 0 /* Bit [0] (_RW) Write status */
#define AML_RESOURCE_MEMORY32_FIXED_WRITE_STATUS_MASK  1
#define AML_RESOURCE_MEMORY32_FIXED_WRITE_STATUS(Info) \
    (((Info) >> AML_RESOURCE_MEMORY32_FIXED_WRITE_STATUS_SHIFT) & AML_RESOURCE_MEMORY32_FIXED_WRITE_STATUS_MASK)
    #define AML_RESOURCE_MEMORY32_FIXED_READ_ONLY  0 /* Non-writeable (read-only) */
    #define AML_RESOURCE_MEMORY32_FIXED_READ_WRITE 1 /* Writeable (read/write) */

//
// DWORD Address Space Descriptor.
// Value = 0x87 (Type = 1, Large item name = 0x07).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_ADDRESS32 {
    UINT8  Tag;
    UINT16 Length;                   /* Variable length, minimum = 23 */
    UINT8  ResourceType;
    UINT8  GeneralFlags;
    UINT8  TypeSpecificFlags;
    UINT32 AddressSpaceGranularity;  /* Address space granularity, _GRA */
    UINT32 AddressRangeMinimum;      /* Address range minimum, _MIN */
    UINT32 AddressRangeMaximum;      /* Address range maximum, _MAX */
    UINT32 AddressTranslationOffset; /* Address translation offset, _TRA */
    UINT32 AddressLength;            /* Address length, _LEN */
//  UINT8  ResourceSourceIndex;      /* Resource source index (optional) */
//  CHAR   ResourceSource[ 0 ];      /* Resource source string (optional) */
} AML_RESOURCE_ADDRESS32;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_ADDRESS32 23

//
// WORD Address Space Descriptor.
// Value = 0x88 (Type = 1, Large item name = 0x08).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_ADDRESS16 {
    UINT8  Tag;
    UINT16 Length;                   /* Variable length, minimum = 13 */
    UINT8  ResourceType;
    UINT8  GeneralFlags;
    UINT8  TypeSpecificFlags;
    UINT16 AddressSpaceGranularity;  /* Address space granularity, _GRA */
    UINT16 AddressRangeMinimum;      /* Address range minimum, _MIN */
    UINT16 AddressRangeMaximum;      /* Address range maximum, _MAX */
    UINT16 AddressTranslationOffset; /* Address translation offset, _TRA */
    UINT16 AddressLength;            /* Address length, _LEN */
//  UINT8  ResourceSourceIndex;      /* Resource source index (optional) */
//  CHAR   ResourceSource[ 0 ];      /* Resource source string (optional) */
} AML_RESOURCE_ADDRESS16;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_ADDRESS16 13

//
// Extended Interrupt Descriptor.
// Value = 0x89 (Type = 1, Large item name = 0x09).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_EXTENDED_INTERRUPT {
    UINT8  Tag;
    UINT16 Length;                    /* Variable length, minimum = 6 */
    UINT8  InterruptVectorFlags;
    UINT8  InterruptTableLength;      /* Number of interrupt numbers that follow */
//  UINT32 InterruptNumber[ 0 ];      /* Array of interrupt numbers, _INT */
//  UINT8  ResourceSourceIndex;       /* Resource source index (optional) */
//  CHAR   ResourceSource[ 0 ];       /* Resource source string (optional) */
} AML_RESOURCE_EXTENDED_INTERRUPT;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_EXTENDED_INTERRUPT 6

//
// Extended interrupt vector flags bitfields.
//
#define AML_RESOURCE_EXTENDED_INTERRUPT_CONSUMER_PRODUCER_SHIFT 0 /* Bit [0] Consumer/Producer */
#define AML_RESOURCE_EXTENDED_INTERRUPT_CONSUMER_PRODUCER_MASK  1
#define AML_RESOURCE_EXTENDED_INTERRUPT_CONSUMER_PRODUCER(Flags) \
    (((Flags) >> AML_RESOURCE_EXTENDED_INTERRUPT_CONSUMER_PRODUCER_SHIFT) & AML_RESOURCE_EXTENDED_INTERRUPT_CONSUMER_PRODUCER_MASK)
    #define AML_RESOURCE_EXTENDED_INTERRUPT_PRODUCER 0 /* This device produces this resource */
    #define AML_RESOURCE_EXTENDED_INTERRUPT_CONSUMER 1 /* This device consumes this resource */
#define AML_RESOURCE_EXTENDED_INTERRUPT_MODE_SHIFT 1 /* Bit [1] (_HE) Interrupt Mode */
#define AML_RESOURCE_EXTENDED_INTERRUPT_MODE_MASK  1
#define AML_RESOURCE_EXTENDED_INTERRUPT_MODE(Flags) \
    (((Flags) >> AML_RESOURCE_EXTENDED_INTERRUPT_MODE_SHIFT) & AML_RESOURCE_EXTENDED_INTERRUPT_MODE_MASK)
    #define AML_RESOURCE_EXTENDED_INTERRUPT_LEVEL_TRIGGERED 0 /* Level-triggered */
    #define AML_RESOURCE_EXTENDED_INTERRUPT_EDGE_TRIGGERED  1 /* Edge-triggered */
#define AML_RESOURCE_EXTENDED_INTERRUPT_POLARITY_SHIFT 2 /* Bit [2] (_LL) Interrupt Polarity */
#define AML_RESOURCE_EXTENDED_INTERRUPT_POLARITY_MASK  1
#define AML_RESOURCE_EXTENDED_INTERRUPT_POLARITY(Flags) \
    (((Flags) >> AML_RESOURCE_EXTENDED_INTERRUPT_POLARITY_SHIFT) & AML_RESOURCE_EXTENDED_INTERRUPT_POLARITY_MASK)
    #define AML_RESOURCE_EXTENDED_INTERRUPT_ACTIVE_HIGH 0 /* Active-high */
    #define AML_RESOURCE_EXTENDED_INTERRUPT_ACTIVE_LOW  1 /* Active-low */
#define AML_RESOURCE_EXTENDED_INTERRUPT_SHARING_SHIFT 3 /* Bit [3] (_SHR) Interrupt Sharing */
#define AML_RESOURCE_EXTENDED_INTERRUPT_SHARING_MASK  1
#define AML_RESOURCE_EXTENDED_INTERRUPT_SHARING(Flags) \
    (((Flags) >> AML_RESOURCE_EXTENDED_INTERRUPT_SHARING_SHIFT) & AML_RESOURCE_EXTENDED_INTERRUPT_SHARING_MASK)
    #define AML_RESOURCE_EXTENDED_INTERRUPT_EXCLUSIVE 0 /* Not shared with other devices */
    #define AML_RESOURCE_EXTENDED_INTERRUPT_SHARED    1 /* Shared with other devices */
#define AML_RESOURCE_EXTENDED_INTERRUPT_WAKE_CAPABILITY_SHIFT 4 /* Bit [4] (_WKC) Wake Capability */
#define AML_RESOURCE_EXTENDED_INTERRUPT_WAKE_CAPABILITY_MASK  1
#define AML_RESOURCE_EXTENDED_INTERRUPT_WAKE_CAPABILITY(Flags) \
    (((Flags) >> AML_RESOURCE_EXTENDED_INTERRUPT_WAKE_CAPABILITY_SHIFT) & AML_RESOURCE_EXTENDED_INTERRUPT_WAKE_CAPABILITY_MASK)
    #define AML_RESOURCE_EXTENDED_INTERRUPT_NOT_WAKE_CAPABLE 0 /* Not capable of waking the system */
    #define AML_RESOURCE_EXTENDED_INTERRUPT_WAKE_CAPABLE     1 /* Capable of waking the system */

//
// QWORD Address Space Descriptor.
// Value = 0x8A (Type = 1, Large item name = 0x0A).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_ADDRESS64 {
    UINT8  Tag;
    UINT16 Length;                   /* Variable length, minimum = 43 */
    UINT8  ResourceType;
    UINT8  GeneralFlags;
    UINT8  TypeSpecificFlags;
    UINT64 AddressSpaceGranularity;  /* Address space granularity, _GRA */
    UINT64 AddressRangeMinimum;      /* Address range minimum, _MIN */
    UINT64 AddressRangeMaximum;      /* Address range maximum, _MAX */
    UINT64 AddressTranslationOffset; /* Address translation offset, _TRA */
    UINT64 AddressLength;            /* Address length, _LEN */
    UINT8  ResourceSourceIndex;      /* Resource source index (optional) */
//  CHAR   ResourceSource[ 0 ];      /* Resource source string (optional) */
} AML_RESOURCE_ADDRESS64;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_ADDRESS64 43

//
// Extended Address Space Descriptor.
// Value = 0x8B (Type = 1, Large item name = 0x0B).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_EXTENDED_ADDRESS64 {
    UINT8  Tag;
    UINT16 Length;                   /* Value = 0x0035 (53) */
    UINT8  ResourceType;
    UINT8  GeneralFlags;
    UINT8  TypeSpecificFlags;
    UINT8  RevisionId;               /* Revision ID, value = 1 for ACPI 3.0 */
    UINT8  Reserved;                 /* Must be 0 */
    UINT64 AddressSpaceGranularity;  /* Address space granularity, _GRA */
    UINT64 AddressRangeMinimum;      /* Address range minimum, _MIN */
    UINT64 AddressRangeMaximum;      /* Address range maximum, _MAX */
    UINT64 AddressTranslationOffset; /* Address translation offset, _TRA */
    UINT64 AddressLength;            /* Address length, _LEN */
    UINT64 TypeSpecificAttribute;    /* Type specific attribute, _ATT */
} AML_RESOURCE_EXTENDED_ADDRESS64;
#pragma pack(pop)

//
// Generic Register Descriptor.
// Value = 0x82 (Type = 1, Large item name = 0x02).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_GENERIC_REGISTER {
    UINT8  Tag;
    UINT16 Length;            /* Value = 0x000C (12) */
    UINT8  AddressSpaceId;    /* Address Space ID, _ASI */
    UINT8  RegisterBitWidth;  /* Register Bit Width, _RBW */
    UINT8  RegisterBitOffset; /* Register Bit Offset, _RBO */
    UINT8  AccessSize;        /* Access Size, _ASZ */
    UINT64 RegisterAddress;   /* Register Address, _ADR */
} AML_RESOURCE_GENERIC_REGISTER;
#pragma pack(pop)

//
// Address space resource type definitions.
//
#define AML_RESOURCE_ADDRESS_TYPE_MEMORY 0 /* Memory range */
#define AML_RESOURCE_ADDRESS_TYPE_IO     1 /* I/O range */
#define AML_RESOURCE_ADDRESS_TYPE_BUS    2 /* Bus number range */

//
// Address space general flags bitfields (common to all address space descriptors).
//
#define AML_RESOURCE_ADDRESS_MIN_ADDRESS_FIXED_SHIFT 2 /* Bit [2] (_MIF) Min Address Fixed */
#define AML_RESOURCE_ADDRESS_MIN_ADDRESS_FIXED_MASK  1
#define AML_RESOURCE_ADDRESS_MIN_ADDRESS_FIXED(Flags) (((Flags) >> AML_RESOURCE_ADDRESS_MIN_ADDRESS_FIXED_SHIFT) & AML_RESOURCE_ADDRESS_MIN_ADDRESS_FIXED_MASK)
    #define AML_RESOURCE_ADDRESS_MIN_ADDRESS_NOT_FIXED 0 /* Minimum address is not fixed */
    #define AML_RESOURCE_ADDRESS_MIN_ADDRESS_IS_FIXED  1 /* Minimum address is fixed */
#define AML_RESOURCE_ADDRESS_MAX_ADDRESS_FIXED_SHIFT 3 /* Bit [3] (_MAF) Max Address Fixed */
#define AML_RESOURCE_ADDRESS_MAX_ADDRESS_FIXED_MASK  1
#define AML_RESOURCE_ADDRESS_MAX_ADDRESS_FIXED(Flags) (((Flags) >> AML_RESOURCE_ADDRESS_MAX_ADDRESS_FIXED_SHIFT) & AML_RESOURCE_ADDRESS_MAX_ADDRESS_FIXED_MASK)
    #define AML_RESOURCE_ADDRESS_MAX_ADDRESS_NOT_FIXED 0 /* Maximum address is not fixed */
    #define AML_RESOURCE_ADDRESS_MAX_ADDRESS_IS_FIXED  1 /* Maximum address is fixed */
#define AML_RESOURCE_ADDRESS_DECODE_TYPE_SHIFT 1 /* Bit [1] (_DEC) Decode Type */
#define AML_RESOURCE_ADDRESS_DECODE_TYPE_MASK  1
#define AML_RESOURCE_ADDRESS_DECODE_TYPE(Flags) (((Flags) >> AML_RESOURCE_ADDRESS_DECODE_TYPE_SHIFT) & AML_RESOURCE_ADDRESS_DECODE_TYPE_MASK)
    #define AML_RESOURCE_ADDRESS_POSITIVE_DECODE     0 /* Bridge positively decodes this address */
    #define AML_RESOURCE_ADDRESS_SUBTRACTIVE_DECODE  1 /* Bridge subtractively decodes this address */

//
// Extended address space general flags additional bitfield.
//
#define AML_RESOURCE_EXTENDED_ADDRESS_CONSUMER_PRODUCER_SHIFT 0 /* Bit [0] Consumer/Producer */
#define AML_RESOURCE_EXTENDED_ADDRESS_CONSUMER_PRODUCER_MASK  1
#define AML_RESOURCE_EXTENDED_ADDRESS_CONSUMER_PRODUCER(Flags) (((Flags) >> AML_RESOURCE_EXTENDED_ADDRESS_CONSUMER_PRODUCER_SHIFT) & AML_RESOURCE_EXTENDED_ADDRESS_CONSUMER_PRODUCER_MASK)
    #define AML_RESOURCE_EXTENDED_ADDRESS_PRODUCER 0 /* This device produces and consumes this resource */
    #define AML_RESOURCE_EXTENDED_ADDRESS_CONSUMER 1 /* This device consumes this resource */

//
// Memory resource type specific flags bitfields (Resource Type = 0).
//
#define AML_RESOURCE_MEMORY_WRITE_STATUS_SHIFT 0 /* Bit [0] (_RW) Write status */
#define AML_RESOURCE_MEMORY_WRITE_STATUS_MASK  1
#define AML_RESOURCE_MEMORY_WRITE_STATUS(Flags) (((Flags) >> AML_RESOURCE_MEMORY_WRITE_STATUS_SHIFT) & AML_RESOURCE_MEMORY_WRITE_STATUS_MASK)
    #define AML_RESOURCE_MEMORY_READ_ONLY  0 /* Read-only */
    #define AML_RESOURCE_MEMORY_READ_WRITE 1 /* Read-write */
#define AML_RESOURCE_MEMORY_ATTRIBUTES_SHIFT 1 /* Bits [2:1] (_MEM) Memory attributes */
#define AML_RESOURCE_MEMORY_ATTRIBUTES_MASK  3
#define AML_RESOURCE_MEMORY_ATTRIBUTES(Flags) (((Flags) >> AML_RESOURCE_MEMORY_ATTRIBUTES_SHIFT) & AML_RESOURCE_MEMORY_ATTRIBUTES_MASK)
    #define AML_RESOURCE_MEMORY_NON_CACHEABLE      0 /* Non-cacheable */
    #define AML_RESOURCE_MEMORY_CACHEABLE          1 /* Cacheable (deprecated) */
    #define AML_RESOURCE_MEMORY_WRITE_COMBINING    2 /* Write combining (deprecated) */
    #define AML_RESOURCE_MEMORY_PREFETCHABLE       3 /* Prefetchable */
#define AML_RESOURCE_MEMORY_TYPE_SHIFT 3 /* Bits [4:3] (_MTP) Memory type */
#define AML_RESOURCE_MEMORY_TYPE_MASK  3
#define AML_RESOURCE_MEMORY_TYPE(Flags) (((Flags) >> AML_RESOURCE_MEMORY_TYPE_SHIFT) & AML_RESOURCE_MEMORY_TYPE_MASK)
    #define AML_RESOURCE_MEMORY_TYPE_MEMORY   0 /* AddressRangeMemory */
    #define AML_RESOURCE_MEMORY_TYPE_RESERVED 1 /* AddressRangeReserved */
    #define AML_RESOURCE_MEMORY_TYPE_ACPI     2 /* AddressRangeACPI */
    #define AML_RESOURCE_MEMORY_TYPE_NVS      3 /* AddressRangeNVS */
#define AML_RESOURCE_MEMORY_TRANSLATION_SHIFT 5 /* Bit [5] (_TTP) Memory to I/O Translation */
#define AML_RESOURCE_MEMORY_TRANSLATION_MASK  1
#define AML_RESOURCE_MEMORY_TRANSLATION(Flags) (((Flags) >> AML_RESOURCE_MEMORY_TRANSLATION_SHIFT) & AML_RESOURCE_MEMORY_TRANSLATION_MASK)
    #define AML_RESOURCE_MEMORY_TYPE_STATIC      0 /* Memory on both sides of bridge */
    #define AML_RESOURCE_MEMORY_TYPE_TRANSLATION 1 /* Memory on secondary, I/O on primary */

//
// I/O resource type specific flags bitfields (Resource Type = 1).
//
#define AML_RESOURCE_IO_RANGE_SHIFT 0 /* Bits [1:0] (_RNG) Range type */
#define AML_RESOURCE_IO_RANGE_MASK  3
#define AML_RESOURCE_IO_RANGE(Flags) (((Flags) >> AML_RESOURCE_IO_RANGE_SHIFT) & AML_RESOURCE_IO_RANGE_MASK)
    #define AML_RESOURCE_IO_RANGE_ENTIRE      3 /* Entire range */
    #define AML_RESOURCE_IO_RANGE_ISA_ONLY    2 /* ISA ranges only */
    #define AML_RESOURCE_IO_RANGE_NON_ISA     1 /* Non-ISA ranges only */
#define AML_RESOURCE_IO_TRANSLATION_TYPE_SHIFT 4 /* Bit [4] (_TTP) I/O to Memory Translation */
#define AML_RESOURCE_IO_TRANSLATION_TYPE_MASK  1
#define AML_RESOURCE_IO_TRANSLATION_TYPE(Flags) (((Flags) >> AML_RESOURCE_IO_TRANSLATION_TYPE_SHIFT) & AML_RESOURCE_IO_TRANSLATION_TYPE_MASK)
    #define AML_RESOURCE_IO_TYPE_STATIC      0 /* I/O on both sides of bridge */
    #define AML_RESOURCE_IO_TYPE_TRANSLATION 1 /* I/O on secondary, memory on primary */
#define AML_RESOURCE_IO_SPARSE_TRANSLATION_SHIFT 5 /* Bit [5] (_TRS) Sparse Translation */
#define AML_RESOURCE_IO_SPARSE_TRANSLATION_MASK  1
#define AML_RESOURCE_IO_SPARSE_TRANSLATION(Flags) (((Flags) >> AML_RESOURCE_IO_SPARSE_TRANSLATION_SHIFT) & AML_RESOURCE_IO_SPARSE_TRANSLATION_MASK)
    #define AML_RESOURCE_IO_DENSE_TRANSLATION  0 /* Dense translation */
    #define AML_RESOURCE_IO_SPARSE_TRANSLATION_VAL 1 /* Sparse translation */

//
// Generic register address space ID definitions.
//
#define AML_RESOURCE_GENERIC_REGISTER_SYSTEM_MEMORY        0x00 /* System Memory */
#define AML_RESOURCE_GENERIC_REGISTER_SYSTEM_IO            0x01 /* System I/O */
#define AML_RESOURCE_GENERIC_REGISTER_PCI_CONFIG           0x02 /* PCI Configuration Space */
#define AML_RESOURCE_GENERIC_REGISTER_EMBEDDED_CONTROLLER  0x03 /* Embedded Controller */
#define AML_RESOURCE_GENERIC_REGISTER_SMBUS                0x04 /* SMBus */
#define AML_RESOURCE_GENERIC_REGISTER_SYSTEM_CMOS          0x05 /* SystemCMOS */
#define AML_RESOURCE_GENERIC_REGISTER_PCI_BAR_TARGET       0x06 /* PciBarTarget */
#define AML_RESOURCE_GENERIC_REGISTER_IPMI                 0x07 /* IPMI */
#define AML_RESOURCE_GENERIC_REGISTER_GPIO                 0x08 /* GeneralPurposeIO */
#define AML_RESOURCE_GENERIC_REGISTER_GENERIC_SERIAL_BUS   0x09 /* GenericSerialBus */
#define AML_RESOURCE_GENERIC_REGISTER_PCC                  0x0A /* PCC */
#define AML_RESOURCE_GENERIC_REGISTER_FUNCTIONAL_FIXED_HW  0x7F /* Functional Fixed Hardware */

//
// Generic register access size definitions.
//
#define AML_RESOURCE_GENERIC_REGISTER_ACCESS_UNDEFINED 0 /* Undefined (legacy) */
#define AML_RESOURCE_GENERIC_REGISTER_ACCESS_BYTE      1 /* Byte access */
#define AML_RESOURCE_GENERIC_REGISTER_ACCESS_WORD      2 /* Word access */
#define AML_RESOURCE_GENERIC_REGISTER_ACCESS_DWORD     3 /* Dword access */
#define AML_RESOURCE_GENERIC_REGISTER_ACCESS_QWORD     4 /* QWord access */

//
// GenericSerialBus Connection Descriptor.
// Value = 0x8E (Type = 1, Large item name = 0x0E).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_SERIAL_BUS {
    UINT8  Tag;                     /* Value = 0x8E */
    UINT16 Length;                  /* Variable length, minimum = 0x09 + L (9 + ResourceSource string length) */
    UINT8  RevisionId;              /* Value = 2 */
    UINT8  ResourceSourceIndex;     /* Resource Connection Instance */
    UINT8  SerialBusType;           /* Serial bus type (1=I2C, 2=SPI, 3=UART, 4=CSI-2) */
    UINT8  GeneralFlags;
    UINT8  TypeSpecificFlags[ 2 ];  /* Type-specific flags [15:0] */
    UINT8  TypeSpecificRevisionId;
    UINT16 TypeDataLength;          /* Variable length */
//  UINT8  TypeSpecificData[ 0 ];   /* Optional type-specific data */
//  CHAR   ResourceSource[ 0 ];
} AML_RESOURCE_SERIAL_BUS;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_SERIAL_BUS 9

//
// Specific type of serial bus resource (SerialBusType field of AML_RESOURCE_SERIAL_BUS).
//
#define AML_RESOURCE_SERIAL_BUS_TYPE_I2C  1
#define AML_RESOURCE_SERIAL_BUS_TYPE_SPI  2
#define AML_RESOURCE_SERIAL_BUS_TYPE_UART 3
#define AML_RESOURCE_SERIAL_BUS_TYPE_CSI2 4

//
// GenericSerialBus general flags bitfields.
//
#define AML_RESOURCE_SERIAL_BUS_SLAVE_MODE_SHIFT 0 /* Bit [0] (_SLV) Slave Mode */
#define AML_RESOURCE_SERIAL_BUS_SLAVE_MODE_MASK  1
#define AML_RESOURCE_SERIAL_BUS_SLAVE_MODE(Flags) \
    (((Flags) >> AML_RESOURCE_SERIAL_BUS_SLAVE_MODE_SHIFT) & AML_RESOURCE_SERIAL_BUS_SLAVE_MODE_MASK)
    #define AML_RESOURCE_SERIAL_BUS_CONTROLLER_INITIATED 0 /* Communication initiated by controller */
    #define AML_RESOURCE_SERIAL_BUS_DEVICE_INITIATED     1 /* Communication initiated by device */
#define AML_RESOURCE_SERIAL_BUS_CONSUMER_PRODUCER_SHIFT 1 /* Bit [1] Consumer/Producer */
#define AML_RESOURCE_SERIAL_BUS_CONSUMER_PRODUCER_MASK  1
#define AML_RESOURCE_SERIAL_BUS_CONSUMER_PRODUCER(Flags) \
    (((Flags) >> AML_RESOURCE_SERIAL_BUS_CONSUMER_PRODUCER_SHIFT) & AML_RESOURCE_SERIAL_BUS_CONSUMER_PRODUCER_MASK)
    #define AML_RESOURCE_SERIAL_BUS_PRODUCER_CONSUMER 0 /* Device produces and consumes resource */
    #define AML_RESOURCE_SERIAL_BUS_CONSUMER          1 /* Device consumes resource */
#define AML_RESOURCE_SERIAL_BUS_CONNECTION_SHARING_SHIFT 2 /* Bit [2] (_SHR) Connection Sharing */
#define AML_RESOURCE_SERIAL_BUS_CONNECTION_SHARING_MASK  1
#define AML_RESOURCE_SERIAL_BUS_CONNECTION_SHARING(Flags) \
    (((Flags) >> AML_RESOURCE_SERIAL_BUS_CONNECTION_SHARING_SHIFT) & AML_RESOURCE_SERIAL_BUS_CONNECTION_SHARING_MASK)
    #define AML_RESOURCE_SERIAL_BUS_EXCLUSIVE 0 /* Exclusive connection */
    #define AML_RESOURCE_SERIAL_BUS_SHARED    1 /* Shared connection */

//
// I2C Serial Bus Connection Resource Descriptor.
// Value = 0x8E (Type = 1, Large item name = 0x0E), SerialBusType = 1.
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_I2C {
    UINT8  Tag;                     /* Value = 0x8E */
    UINT16 Length;                  /* Variable length, minimum = 0x0F + L */
    UINT8  RevisionId;              /* Value = 2 */
    UINT8  ResourceSourceIndex;     /* Master Instance */
    UINT8  SerialBusType;           /* Value = 1 for I2C */
    UINT8  GeneralFlags;
    UINT8  TypeSpecificFlags;       /* I2C-specific flags [7:0] */
    UINT8  LegacyVirtualRegister;   /* I3C Legacy Virtual Register [15:8] */
    UINT8  TypeSpecificRevisionId;  /* Value = 1 */
    UINT16 TypeDataLength;          /* Minimum value = 0x0006 */
    UINT32 ConnectionSpeed;         /* Maximum speed in Hz (_SPE) */
    UINT16 SlaveAddress;            /* I2C bus address (_ADR) */
//  UINT8  VendorData[ 0 ];         /* Optional vendor-defined data */
//  CHAR   ResourceSource[ 0 ];
} AML_RESOURCE_I2C;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_I2C 15

//
// I2C type-specific flags bitfields.
//
#define AML_RESOURCE_I2C_10BIT_ADDRESSING_SHIFT 0 /* Bit [0] (_MOD) 10-bit addressing mode */
#define AML_RESOURCE_I2C_10BIT_ADDRESSING_MASK  1
#define AML_RESOURCE_I2C_10BIT_ADDRESSING(Flags) \
    (((Flags) >> AML_RESOURCE_I2C_10BIT_ADDRESSING_SHIFT) & AML_RESOURCE_I2C_10BIT_ADDRESSING_MASK)

//
// I2C slave address bitfields.
//
#define AML_RESOURCE_I2C_SLAVE_ADDRESS_7BIT_SHIFT       0 /* Bits [6:0] 7-bit address */
#define AML_RESOURCE_I2C_SLAVE_ADDRESS_7BIT_MASK        0x7F
#define AML_RESOURCE_I2C_SLAVE_ADDRESS_10BIT_HIGH_SHIFT 8 /* Bits [9:8] 10-bit address high bits */
#define AML_RESOURCE_I2C_SLAVE_ADDRESS_10BIT_HIGH_MASK  0x03

//
// SPI Serial Bus Connection Resource Descriptor.
// Value = 0x8E (Type = 1, Large item name = 0x0E), SerialBusType = 2.
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_SPI {
    UINT8  Tag;                     /* Value = 0x8E */
    UINT16 Length;                  /* Variable length, minimum = 0x12 + L */
    UINT8  RevisionId;              /* Value = 1 */
    UINT8  ResourceSourceIndex;     /* Reserved, must be 0 */
    UINT8  SerialBusType;           /* Value = 2 for SPI */
    UINT8  GeneralFlags;
    UINT8  TypeSpecificFlags;       /* SPI-specific flags [7:0] */
    UINT8  Reserved;                /* Reserved, must be 0 */
    UINT8  TypeSpecificRevisionId;  /* Value = 1 */
    UINT16 TypeDataLength;          /* Minimum value = 0x0009 */
    UINT32 ConnectionSpeed;         /* Maximum speed in Hz (_SPE) */
    UINT8  DataBitLength;           /* Size in bits of smallest transfer unit (_LEN) */
    UINT8  Phase;                   /* Clock phase (_PHA) */
    UINT8  Polarity;                /* Clock polarity (_POL) */
    UINT16 DeviceSelection;         /* Device selection value (_ADR) */
//  UINT8  VendorData[ 0 ];         /* Optional vendor-defined data */
//  CHAR   ResourceSource[ 0 ];
} AML_RESOURCE_SPI;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_SPI 18

//
// SPI type-specific flags bitfields.
//
#define AML_RESOURCE_SPI_WIRE_MODE_SHIFT 0 /* Bit [0] (_MOD) Wire Mode */
#define AML_RESOURCE_SPI_WIRE_MODE_MASK  1
#define AML_RESOURCE_SPI_WIRE_MODE(Flags) \
    (((Flags) >> AML_RESOURCE_SPI_WIRE_MODE_SHIFT) & AML_RESOURCE_SPI_WIRE_MODE_MASK)
    #define AML_RESOURCE_SPI_4_WIRE 0 /* 4-wire connection */
    #define AML_RESOURCE_SPI_3_WIRE 1 /* 3-wire connection */
#define AML_RESOURCE_SPI_DEVICE_POLARITY_SHIFT 1 /* Bit [1] (_DPL) Device Polarity */
#define AML_RESOURCE_SPI_DEVICE_POLARITY_MASK  1
#define AML_RESOURCE_SPI_DEVICE_POLARITY(Flags) \
    (((Flags) >> AML_RESOURCE_SPI_DEVICE_POLARITY_SHIFT) & AML_RESOURCE_SPI_DEVICE_POLARITY_MASK)
    #define AML_RESOURCE_SPI_ACTIVE_LOW  0 /* Device selection line active low */
    #define AML_RESOURCE_SPI_ACTIVE_HIGH 1 /* Device selection line active high */

//
// SPI phase values.
//
#define AML_RESOURCE_SPI_FIRST_PHASE  0 /* First phase */
#define AML_RESOURCE_SPI_SECOND_PHASE 1 /* Second phase */

//
// SPI polarity values.
//
#define AML_RESOURCE_SPI_START_LOW  0 /* Start low */
#define AML_RESOURCE_SPI_START_HIGH 1 /* Start high */

//
// UART Serial Bus Connection Resource Descriptor.
// Value = 0x8E (Type = 1, Large item name = 0x0E), SerialBusType = 3.
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_UART {
    UINT8  Tag;                     /* Value = 0x8E */
    UINT16 Length;                  /* Variable length, minimum = 0x13 + L */
    UINT8  RevisionId;              /* Value = 1 */
    UINT8  ResourceSourceIndex;     /* Reserved, must be 0 */
    UINT8  SerialBusType;           /* Value = 3 for UART */
    UINT8  GeneralFlags;
    UINT8  TypeSpecificFlags;       /* UART-specific flags [7:0] */
    UINT8  Reserved;                /* Reserved, must be 0 */
    UINT8  TypeSpecificRevisionId;  /* Value = 1 */
    UINT16 TypeDataLength;          /* Minimum value = 0x000A */
    UINT32 DefaultBaudRate;         /* Default baud rate in bits-per-second (_SPE) */
    UINT16 RxFifo;                  /* Maximum receive buffer in bytes (_RXL) */
    UINT16 TxFifo;                  /* Maximum transmit buffer in bytes (_TXL) */
    UINT8  Parity;                  /* Parity (_PAR) */
    UINT8  SerialLinesEnabled;      /* Serial lines enabled (_LIN) */
//  UINT8  VendorData[ 0 ];         /* Optional vendor-defined data */
//  CHAR   ResourceSource[ 0 ];
} AML_RESOURCE_UART;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_UART (0x13 + 0x0A)

//
// UART type-specific flag bitfields.
//
#define AML_RESOURCE_UART_FLOW_CONTROL_SHIFT 0 /* Bits [1:0] (_FLC) Flow control */
#define AML_RESOURCE_UART_FLOW_CONTROL_MASK  0x03
#define AML_RESOURCE_UART_FLOW_CONTROL(Flags) \
    (((Flags) >> AML_RESOURCE_UART_FLOW_CONTROL_SHIFT) & AML_RESOURCE_UART_FLOW_CONTROL_MASK)
    #define AML_RESOURCE_UART_FLOW_CONTROL_NONE     0 /* None */
    #define AML_RESOURCE_UART_FLOW_CONTROL_HARDWARE 1 /* Hardware flow control */
    #define AML_RESOURCE_UART_FLOW_CONTROL_XON_XOFF 2 /* XON/XOFF */
#define AML_RESOURCE_UART_STOP_BITS_SHIFT 2 /* Bits [3:2] (_STB) Stop bits */
#define AML_RESOURCE_UART_STOP_BITS_MASK  0x03
#define AML_RESOURCE_UART_STOP_BITS(Flags) \
    (((Flags) >> AML_RESOURCE_UART_STOP_BITS_SHIFT) & AML_RESOURCE_UART_STOP_BITS_MASK)
    #define AML_RESOURCE_UART_STOP_BITS_NONE 0 /* None */
    #define AML_RESOURCE_UART_STOP_BITS_1    1 /* 1 stop bit */
    #define AML_RESOURCE_UART_STOP_BITS_1_5  2 /* 1.5 stop bits */
    #define AML_RESOURCE_UART_STOP_BITS_2    3 /* 2 stop bits */
#define AML_RESOURCE_UART_DATA_BITS_SHIFT 4 /* Bits [6:4] (_LEN) Data bits */
#define AML_RESOURCE_UART_DATA_BITS_MASK  0x07
#define AML_RESOURCE_UART_DATA_BITS(Flags) \
    (((Flags) >> AML_RESOURCE_UART_DATA_BITS_SHIFT) & AML_RESOURCE_UART_DATA_BITS_MASK)
    #define AML_RESOURCE_UART_DATA_BITS_5 0 /* 5 bits */
    #define AML_RESOURCE_UART_DATA_BITS_6 1 /* 6 bits */
    #define AML_RESOURCE_UART_DATA_BITS_7 2 /* 7 bits */
    #define AML_RESOURCE_UART_DATA_BITS_8 3 /* 8 bits */
    #define AML_RESOURCE_UART_DATA_BITS_9 4 /* 9 bits */
#define AML_RESOURCE_UART_ENDIANNESS_SHIFT 7 /* Bit [7] (_END) Endianness */
#define AML_RESOURCE_UART_ENDIANNESS_MASK  1
#define AML_RESOURCE_UART_ENDIANNESS(Flags) \
    (((Flags) >> AML_RESOURCE_UART_ENDIANNESS_SHIFT) & AML_RESOURCE_UART_ENDIANNESS_MASK)
    #define AML_RESOURCE_UART_LITTLE_ENDIAN 0 /* Little endian */
    #define AML_RESOURCE_UART_BIG_ENDIAN    1 /* Big endian */

//
// UART parity values.
//
#define AML_RESOURCE_UART_PARITY_NONE  0x00
#define AML_RESOURCE_UART_PARITY_EVEN  0x01
#define AML_RESOURCE_UART_PARITY_ODD   0x02
#define AML_RESOURCE_UART_PARITY_MARK  0x03
#define AML_RESOURCE_UART_PARITY_SPACE 0x04

//
// UART serial lines enabled bitfields.
//
#define AML_RESOURCE_UART_DCD_SHIFT 2 /* Bit [2] Data Carrier Detect */
#define AML_RESOURCE_UART_DCD_MASK  1
#define AML_RESOURCE_UART_DCD_FLAG  (AML_RESOURCE_UART_DCD_MASK << AML_RESOURCE_UART_DCD_SHIFT)
#define AML_RESOURCE_UART_RI_SHIFT  3 /* Bit [3] Ring Indicator */
#define AML_RESOURCE_UART_RI_MASK   1
#define AML_RESOURCE_UART_RI_FLAG   (AML_RESOURCE_UART_RI_SMASK << AML_RESOURCE_UART_RI_SHIFT )
#define AML_RESOURCE_UART_DSR_SHIFT 4 /* Bit [4] Data Set Ready */
#define AML_RESOURCE_UART_DSR_MASK  1
#define AML_RESOURCE_UART_DSR_FLAG  (AML_RESOURCE_UART_DSR_MASK << AML_RESOURCE_UART_DSR_SHIFT)
#define AML_RESOURCE_UART_DTR_SHIFT 5 /* Bit [5] Data Terminal Ready */
#define AML_RESOURCE_UART_DTR_MASK  1
#define AML_RESOURCE_UART_DTR_FLAG  (AML_RESOURCE_UART_DTR_MASK << AML_RESOURCE_UART_DTR_SHIFT)
#define AML_RESOURCE_UART_CTS_SHIFT 6 /* Bit [6] Clear to Send */
#define AML_RESOURCE_UART_CTS_MASK  1
#define AML_RESOURCE_UART_CTS_FLAG  (AML_RESOURCE_UART_CTS_MASK << AML_RESOURCE_UART_CTS_SHIFT)
#define AML_RESOURCE_UART_RTS_SHIFT 7 /* Bit [7] Request to Send */
#define AML_RESOURCE_UART_RTS_MASK  1
#define AML_RESOURCE_UART_RTS_FLAG  (AML_RESOURCE_UART_RTS_MASK << AML_RESOURCE_UART_RTS_SHIFT)

//
// CSI-2 Connection Resource Descriptor.
// Value = 0x8E (Type = 1, Large item name = 0x0E), SerialBusType = 4.
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_CSI2 {
    UINT8  Tag;                     /* Value = 0x8E */
    UINT16 Length;                  /* Variable length, minimum = 0x0F + L */
    UINT8  RevisionId;              /* Value = 1 */
    UINT8  ResourceSourceIndex;     /* Remote Port Instance */
    UINT8  SerialBusType;           /* Value = 4 for CSI-2 */
    UINT8  GeneralFlags;
    UINT8  TypeSpecificFlags;       /* CSI-2-specific flags [7:0] */
    UINT8  Reserved;                /* Reserved, must be 0 */
    UINT8  TypeSpecificRevisionId;  /* Value = 1 */
    UINT16 TypeDataLength;          /* Variable length, minimum = 0 */
//  UINT8  VendorData[ 0 ];         /* Optional vendor-defined data (_VEN) */
//  CHAR   ResourceSource[ 0 ];
} AML_RESOURCE_CSI2;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_CSI2 0x0F

//
// CSI-2 type-specific flags bitfields.
//
#define AML_RESOURCE_CSI2_PHY_TYPE_SHIFT 0 /* Bits [1:0] (_PHY) PHY Type */
#define AML_RESOURCE_CSI2_PHY_TYPE_MASK  0x03
#define AML_RESOURCE_CSI2_PHY_TYPE(Flags) \
    (((Flags) >> AML_RESOURCE_CSI2_PHY_TYPE_SHIFT) & AML_RESOURCE_CSI2_PHY_TYPE_MASK)
    #define AML_RESOURCE_CSI2_PHY_C_PHY 0 /* C-PHY */
    #define AML_RESOURCE_CSI2_PHY_D_PHY 1 /* D-PHY */
#define AML_RESOURCE_CSI2_LOCAL_PORT_SHIFT 2 /* Bits [7:2] (_PRT) Local Port Instance */
#define AML_RESOURCE_CSI2_LOCAL_PORT_MASK  0x3F
#define AML_RESOURCE_CSI2_LOCAL_PORT(Flags) \
    (((Flags) >> AML_RESOURCE_CSI2_LOCAL_PORT_SHIFT) & AML_RESOURCE_CSI2_LOCAL_PORT_MASK)

//
// Pin Function Descriptor.
// Value = 0x8D (Type = 1, Large item name = 0x0D).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_PIN_FUNCTION {
    UINT8  Tag;                      /* Value = 0x8D */
    UINT16 Length;                   /* Variable length, minimum = 0x0F + L */
    UINT8  RevisionId;               /* Value = 1 */
    UINT8  Flags;                    /* Flags [7:0] */
    UINT8  Reserved;                 /* Reserved, must be 0 */
    UINT8  PinPullConfiguration;     /* Pin pull configuration */
    UINT16 FunctionNumber;           /* Function number */
    UINT16 PinTableOffset;           /* Offset to pin table */
    UINT8  ResourceSourceIndex;      /* Reserved, must be 0 */
    UINT16 ResourceSourceNameOffset; /* Offset to resource source name */
    UINT16 VendorDataOffset;         /* Offset to vendor data */
    UINT16 VendorDataLength;         /* Length of vendor data */
//  UINT16 PinData[ 0 ];
//  UINT8  VendorData[ 0 ];
} AML_RESOURCE_PIN_FUNCTION;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_PIN_FUNCTION 15

//
// Pin Function flags bitfields.
//
#define AML_RESOURCE_PIN_FUNCTION_IO_SHARING_SHIFT 0 /* Bit [0] (_SHR) IO Sharing */
#define AML_RESOURCE_PIN_FUNCTION_IO_SHARING_MASK  1
#define AML_RESOURCE_PIN_FUNCTION_IO_SHARING(Flags) \
    (((Flags) >> AML_RESOURCE_PIN_FUNCTION_IO_SHARING_SHIFT) & AML_RESOURCE_PIN_FUNCTION_IO_SHARING_MASK)
    #define AML_RESOURCE_PIN_FUNCTION_EXCLUSIVE 0 /* Exclusive */
    #define AML_RESOURCE_PIN_FUNCTION_SHARED    1 /* Shared */

//
// Pin Configuration Descriptor.
// Value = 0x8F (Type = 1, Large item name = 0x0F).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_PIN_CONFIGURATION {
    UINT8  Tag;                      /* Value = 0x8F */
    UINT16 Length;                   /* Variable length, minimum = 0x13 + L */
    UINT8  RevisionId;               /* Value = 1 */
    UINT8  Flags;                    /* Flags [7:0] */
    UINT8  Reserved;                 /* Reserved, must be 0 */
    UINT8  PinConfigurationType;     /* Pin configuration type (_TYP) */
    UINT32 PinConfigurationValue;    /* Pin configuration value (_VAL) */
    UINT16 PinTableOffset;           /* Offset to pin table */
    UINT8  ResourceSourceIndex;      /* Reserved, must be 0 */
    UINT16 ResourceSourceNameOffset; /* Offset to resource source name */
    UINT16 VendorDataOffset;         /* Offset to vendor data */
    UINT16 VendorDataLength;         /* Length of vendor data */
//  UINT16 PinData[ 0 ];
//  UINT8  VendorData[ 0 ];
} AML_RESOURCE_PIN_CONFIGURATION;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_PIN_CONFIGURATION 19

//
// Pin Configuration flags bitfields.
//
#define AML_RESOURCE_PIN_CONFIG_IO_SHARING_SHIFT 0 /* Bit [0] (_SHR) IO Sharing */
#define AML_RESOURCE_PIN_CONFIG_IO_SHARING_MASK  1
#define AML_RESOURCE_PIN_CONFIG_IO_SHARING(Flags) \
    (((Flags) >> AML_RESOURCE_PIN_CONFIG_IO_SHARING_SHIFT) & AML_RESOURCE_PIN_CONFIG_IO_SHARING_MASK)
    #define AML_RESOURCE_PIN_CONFIG_EXCLUSIVE 0 /* Exclusive */
    #define AML_RESOURCE_PIN_CONFIG_SHARED    1 /* Shared */
#define AML_RESOURCE_PIN_CONFIG_CONSUMER_PRODUCER_SHIFT 1 /* Bit [1] Consumer/Producer */
#define AML_RESOURCE_PIN_CONFIG_CONSUMER_PRODUCER_MASK  1
#define AML_RESOURCE_PIN_CONFIG_CONSUMER_PRODUCER(Flags) \
    (((Flags) >> AML_RESOURCE_PIN_CONFIG_CONSUMER_PRODUCER_SHIFT) & AML_RESOURCE_PIN_CONFIG_CONSUMER_PRODUCER_MASK)
    #define AML_RESOURCE_PIN_CONFIG_PRODUCER_CONSUMER 0 /* Device produces and consumes resource */
    #define AML_RESOURCE_PIN_CONFIG_CONSUMER          1 /* Device consumes resource */

//
// Pin Group Descriptor.
// Value = 0x90 (Type = 1, Large item name = 0x10).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_PIN_GROUP {
    UINT8  Tag;                 /* Value = 0x90 */
    UINT16 Length;              /* Variable length, minimum = 0x0B + L */
    UINT8  RevisionId;          /* Value = 1 */
    UINT8  Flags;               /* Flags [7:0] */
    UINT8  Reserved;            /* Reserved, must be 0 */
    UINT16 PinTableOffset;      /* Offset to pin table */
    UINT16 ResourceLabelOffset; /* Offset to resource label */
    UINT16 VendorDataOffset;    /* Offset to vendor data */
    UINT16 VendorDataLength;    /* Length of vendor data */
//  UINT16 PinData[ 0 ];
//  UINT8  ResourceLabel[ 0 ];
//  UINT8  VendorData[ 0 ];
} AML_RESOURCE_PIN_GROUP;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_PIN_GROUP 11

//
// Pin Group flags bitfields.
//
#define AML_RESOURCE_PIN_GROUP_CONSUMER_PRODUCER_SHIFT 0 /* Bit [0] Consumer/Producer */
#define AML_RESOURCE_PIN_GROUP_CONSUMER_PRODUCER_MASK  1
#define AML_RESOURCE_PIN_GROUP_CONSUMER_PRODUCER(Flags) \
    (((Flags) >> AML_RESOURCE_PIN_GROUP_CONSUMER_PRODUCER_SHIFT) & AML_RESOURCE_PIN_GROUP_CONSUMER_PRODUCER_MASK)
    #define AML_RESOURCE_PIN_GROUP_PRODUCER_CONSUMER 0 /* Device produces and consumes resource */
    #define AML_RESOURCE_PIN_GROUP_CONSUMER          1 /* Device consumes resource */

//
// Pin Group Function Descriptor.
// Value = 0x91 (Type = 1, Large item name = 0x11).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_PIN_GROUP_FUNCTION {
    UINT8  Tag;                       /* Value = 0x91 */
    UINT16 Length;                    /* Variable length, minimum = 0x0E + L1 + L2 */
    UINT8  RevisionId;                /* Value = 1 */
    UINT8  Flags;                     /* Flags [7:0] */
    UINT8  Reserved;                  /* Reserved, must be 0 */
    UINT16 FunctionNumber;            /* Function number (_FUN) */
    UINT8  ResourceSourceIndex;       /* Reserved, must be 0 */
    UINT16 ResourceSourceNameOffset;  /* Offset to resource source name */
    UINT16 ResourceSourceLabelOffset; /* Offset to resource source label */
    UINT16 VendorDataOffset;          /* Offset to vendor data */
    UINT16 VendorDataLength;          /* Length of vendor data */
//  UINT8  ResourceSourceName[ 0 ];
//  UINT8  ResourceSourceLabel[ 0 ];
//  UINt8  VendorData[ 0 ];
} AML_RESOURCE_PIN_GROUP_FUNCTION;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_PIN_GROUP_FUNCTION 14

//
// Pin Group Function flags bitfields.
//
#define AML_RESOURCE_PIN_GROUP_FUNC_IO_SHARING_SHIFT 0 /* Bit [0] (_SHR) IO Sharing */
#define AML_RESOURCE_PIN_GROUP_FUNC_IO_SHARING_MASK  1
#define AML_RESOURCE_PIN_GROUP_FUNC_IO_SHARING(Flags) \
    (((Flags) >> AML_RESOURCE_PIN_GROUP_FUNC_IO_SHARING_SHIFT) & AML_RESOURCE_PIN_GROUP_FUNC_IO_SHARING_MASK)
    #define AML_RESOURCE_PIN_GROUP_FUNC_EXCLUSIVE 0 /* Exclusive */
    #define AML_RESOURCE_PIN_GROUP_FUNC_SHARED    1 /* Shared */

#define AML_RESOURCE_PIN_GROUP_FUNC_CONSUMER_PRODUCER_SHIFT 1 /* Bit [1] Consumer/Producer */
#define AML_RESOURCE_PIN_GROUP_FUNC_CONSUMER_PRODUCER_MASK  1
#define AML_RESOURCE_PIN_GROUP_FUNC_CONSUMER_PRODUCER(Flags) \
    (((Flags) >> AML_RESOURCE_PIN_GROUP_FUNC_CONSUMER_PRODUCER_SHIFT) & AML_RESOURCE_PIN_GROUP_FUNC_CONSUMER_PRODUCER_MASK)
    #define AML_RESOURCE_PIN_GROUP_FUNC_PRODUCER_CONSUMER 0 /* Device produces and consumes resource */
    #define AML_RESOURCE_PIN_GROUP_FUNC_CONSUMER          1 /* Device consumes resource */

//
// Pin Group Configuration Descriptor, Type 1.
// Value = 0x92 (Type = 1, Large item name = 0x12).
//
#pragma pack(push, 1)
typedef struct _AML_RESOURCE_PIN_GROUP_CONFIG {
    UINT8  Tag;                         // Resource Identifier (0x92)
    UINT16 Length;                      // Variable length, minimum 0x11 + L1 + L2
    UINT8  RevisionId;                  // Revision ID (must be 1)
    UINT16 Flags;                       // Configuration flags
    UINT8  PinConfigurationType;        // Pin Configuration Type (_TYP)
    UINT32 PinConfigurationValue;       // Pin Configuration Value (_VAL)
    UINT8  ResourceSourceIndex;         // Reserved (must be 0)
    UINT16 ResourceSourceNameOffset;    // Offset to resource source name
    UINT16 ResourceSourceLabelOffset;   // Offset to resource source label
    UINT16 VendorDataOffset;            // Offset to vendor data
    UINT16 VendorDataLength;            // Length of vendor data
//  CHAR   ResourceSourceName[ 0 ];     // Pin controller name (at ResourceSourceNameOffset)
//  CHAR   ResourceSourceLabel[ 0 ];    // PinGroup resource label (at ResourceSourceLabelOffset)
//  UINT8  VendorData[ 0 ];             // Vendor-defined data (at VendorDataOffset)
} AML_RESOURCE_PIN_GROUP_CONFIG;
#pragma pack(pop)
#define AML_RESOURCE_MIN_LENGTH_PIN_GROUP_CONFIG 17

//
// Flags bitfields
//
#define AML_PIN_GROUP_FLAGS_CONSUMER_PRODUCER_SHIFT     1   // Bit 1
#define AML_PIN_GROUP_FLAGS_CONSUMER_PRODUCER_MASK      0x01
#define AML_PIN_GROUP_FLAGS_CONSUMER                    0x01
#define AML_PIN_GROUP_FLAGS_PRODUCER                    0x00

#define AML_PIN_GROUP_FLAGS_IO_SHARING_SHIFT            0   // Bit 0
#define AML_PIN_GROUP_FLAGS_IO_SHARING_MASK             0x01
#define AML_PIN_GROUP_FLAGS_IO_SHARING_EXCLUSIVE        0x00
#define AML_PIN_GROUP_FLAGS_IO_SHARING_SHARED           0x01

//
// Common predefined large tag values.
//
#define AML_RESOURCE_MAKE_LARGE_TAG(Name)     (0x80 | ((Name) & 0x7F))
#define AML_RESOURCE_TAG_MEMORY24             AML_RESOURCE_MAKE_LARGE_TAG(0x01)
#define AML_RESOURCE_TAG_GENERIC_REGISTER     AML_RESOURCE_MAKE_LARGE_TAG(0x02)
#define AML_RESOURCE_TAG_LARGE_VENDOR         AML_RESOURCE_MAKE_LARGE_TAG(0x04)
#define AML_RESOURCE_TAG_MEMORY32             AML_RESOURCE_MAKE_LARGE_TAG(0x05)
#define AML_RESOURCE_TAG_MEMORY32_FIXED       AML_RESOURCE_MAKE_LARGE_TAG(0x06)
#define AML_RESOURCE_TAG_ADDRESS32            AML_RESOURCE_MAKE_LARGE_TAG(0x07)
#define AML_RESOURCE_TAG_ADDRESS16            AML_RESOURCE_MAKE_LARGE_TAG(0x08)
#define AML_RESOURCE_TAG_EXTENDED_INTERRUPT   AML_RESOURCE_MAKE_LARGE_TAG(0x09)
#define AML_RESOURCE_TAG_ADDRESS64            AML_RESOURCE_MAKE_LARGE_TAG(0x0A)
#define AML_RESOURCE_TAG_EXTENDED_ADDRESS64   AML_RESOURCE_MAKE_LARGE_TAG(0x0B)
#define AML_RESOURCE_TAG_GPIO_CONNECTION      AML_RESOURCE_MAKE_LARGE_TAG(0x0C)
#define AML_RESOURCE_TAG_PIN_FUNCTION         AML_RESOURCE_MAKE_LARGE_TAG(0x0D)
#define AML_RESOURCE_TAG_GSB_CONNECTION       AML_RESOURCE_MAKE_LARGE_TAG(0x0E)
#define AML_RESOURCE_TAG_PIN_CONFIGURATION    AML_RESOURCE_MAKE_LARGE_TAG(0x0F)
#define AML_RESOURCE_TAG_PIN_GROUP            AML_RESOURCE_MAKE_LARGE_TAG(0x10)
#define AML_RESOURCE_TAG_PIN_GROUP_FUNCTION   AML_RESOURCE_MAKE_LARGE_TAG(0x11)
#define AML_RESOURCE_TAG_PIN_GROUP_CONFIG     AML_RESOURCE_MAKE_LARGE_TAG(0x12)
#define AML_RESOURCE_TAG_CLOCK_INPUT          AML_RESOURCE_MAKE_LARGE_TAG(0x13)
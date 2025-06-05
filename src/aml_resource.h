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
#define AML_RESOURCE_SMALL_TAG_TYPE_FLAG \
	(AML_RESOURCE_SMALL_TAG_TYPE_MASK << AML_RESOURCE_SMALL_TAG_TYPE_SHIFT)

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
    #define AML_RESOURCE_IRQ_MODE_LEVEL 0  /* Level-Triggered */
    #define AML_RESOURCE_IRQ_MODE_EDGE  1  /* Edge-Triggered */
#define AML_RESOURCE_IRQ_POLARITY_SHIFT 3 /* Bit [3] Interrupt Polarity, _LL */
#define AML_RESOURCE_IRQ_POLARITY_MASK  1
#define AML_RESOURCE_IRQ_POLARITY(Info) (((Info) >> AML_RESOURCE_IRQ_POLARITY_SHIFT) & AML_RESOURCE_IRQ_POLARITY_MASK)
    #define AML_RESOURCE_IRQ_POLARITY_HIGH 0  /* Active-High */
    #define AML_RESOURCE_IRQ_POLARITY_LOW  1  /* Active-Low */
#define AML_RESOURCE_IRQ_SHARING_SHIFT 4 /* Bit [4] Interrupt Sharing, _SHR */
#define AML_RESOURCE_IRQ_SHARING_MASK  1
#define AML_RESOURCE_IRQ_SHARING(Info) (((Info) >> AML_RESOURCE_IRQ_SHARING_SHIFT) & AML_RESOURCE_IRQ_SHARING_MASK)
    #define AML_RESOURCE_IRQ_EXCLUSIVE 0  /* Not shared with other devices */
    #define AML_RESOURCE_IRQ_SHARED    1  /* Shared with other devices */
#define AML_RESOURCE_IRQ_WAKE_SHIFT 5 /* Bit [5] Wake Capability, _WKC */
#define AML_RESOURCE_IRQ_WAKE_MASK  1
#define AML_RESOURCE_IRQ_WAKE(Info) (((Info) >> AML_RESOURCE_IRQ_WAKE_SHIFT) & AML_RESOURCE_IRQ_WAKE_MASK)
    #define AML_RESOURCE_IRQ_NOT_WAKE_CAPABLE 0  /* Not capable of waking the system */
    #define AML_RESOURCE_IRQ_WAKE_CAPABLE     1  /* Capable of waking the system */

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
     ((Name) << AML_RESOURCE_SMALL_TAG_NAME_SHIFT) | ((Length) << AML_RESOURCE_SMALL_TAG_LENGTH_SHIFT))
#define AML_RESOURCE_TAG_IRQ_2              AML_RESOURCE_MAKE_SMALL_TAG(0x4, 0x2)
#define AML_RESOURCE_TAG_IRQ_3              AML_RESOURCE_MAKE_SMALL_TAG(0x4, 0x3)
#define AML_RESOURCE_TAG_DMA                AML_RESOURCE_MAKE_SMALL_TAG(0x5, 0x2)
#define AML_RESOURCE_TAG_START_DPF          AML_RESOURCE_MAKE_SMALL_TAG(0x6, 0x0)
#define AML_RESOURCE_TAG_START_DPF_PRIORITY AML_RESOURCE_MAKE_SMALL_TAG(0x6, 0x1)
#define AML_RESOURCE_TAG_END_DPF            AML_RESOURCE_MAKE_SMALL_TAG(0x7, 0x0)
#define AML_RESOURCE_TAG_IO_PORT            AML_RESOURCE_MAKE_SMALL_TAG(0x8, 0x7)
#define AML_RESOURCE_TAG_IO_PORT_FIXED      AML_RESOURCE_MAKE_SMALL_TAG(0x9, 0x3)
#define AML_RESOURCE_TAG_DMA_FIXED          AML_RESOURCE_MAKE_SMALL_TAG(0xA, 0x5)
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
    UINT8  VendorData[ 0 ]; /* Vendor defined data bytes */
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
    UINT8  ResourceSourceIndex;      /* Resource source index (optional) */
    CHAR   ResourceSource[ 0 ];      /* Resource source string (optional) */
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
    UINT8  ResourceSourceIndex;      /* Resource source index (optional) */
    CHAR   ResourceSource[ 0 ];      /* Resource source string (optional) */
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
    UINT32 InterruptNumber[ 0 ];      /* Array of interrupt numbers, _INT */
//  UINT8  ResourceSourceIndex;       /* Resource source index (optional) */
//  CHAR   ResourceSource[];          /* Resource source string (optional) */
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
    CHAR   ResourceSource[ 0 ];      /* Resource source string (optional) */
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
// Common predefined large tag values.
//
#define AML_RESOURCE_MAKE_LARGE_TAG(Name)     (0x80 | ((Name) & 0x7F))
#define AML_RESOURCE_TAG_MEMORY24             AML_RESOURCE_MAKE_LARGE_TAG(0x01)
#define AML_RESOURCE_TAG_GENERIC_REGISTER     AML_RESOURCE_MAKE_LARGE_TAG(0x02)
#define AML_RESOURCE_TAG_LARGE_VENDOR_DEFINED AML_RESOURCE_MAKE_LARGE_TAG(0x04)
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
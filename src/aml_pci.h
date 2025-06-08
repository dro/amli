#pragma once

#include "aml_platform.h"
#include "aml_heap.h"
#include "aml_pci_info.h"
#include "aml_host.h"

//
// Generic PCI configuration space offsets.
// 
#define AML_PCI_CFS_OFFSET_HEADER_TYPE 0x0E

//
// PCI configuration space header type field layout-code mask.
//
#define AML_PCI_CFS_MASK_HEADER_TYPE_LAYOUT 0x7F

//
// PCI-to-PCI bridge type-specific configuration space offsets.
//
#define AML_PCI_CFS_OFFSET_BRIDGE_PRIMARY_BUS     0x18
#define AML_PCI_CFS_OFFSET_BRIDGE_SECONDARY_BUS   0x19
#define AML_PCI_CFS_OFFSET_BRIDGE_SUBORDINATE_BUS 0x1A

//
// PCI configuration space header types.
//
#define AML_PCI_CFS_HEADER_TYPE_PCI_TO_PCI_BRIDGE     1
#define AML_PCI_CFS_HEADER_TYPE_PCI_TO_CARDBUS_BRIDGE 2

//
// PCI interrupt routing table entry.
// Note: The PCI function number in the Address field of the _PRT packages must be 0xFFFF,
// indicating "any" function number or "all functions".
//
typedef struct _AML_PCI_PRT_ENTRY {
    //
    // The address of the device (uses the same format as _ADR).
    // Function should always be 0xFFFF indicating all functions.
    //
    UINT32 Address;

    //
    // The PCI pin number of the device (0-INTA, 1-INTB, 2-INTC, 3-INTD).
    //
    UINT32 Pin;
    
    //
    // Index that indicates which resource descriptor in the resource template of
    // the device pointed to in the Source field this interrupt is allocated from.
    // If the Source field is the Byte value zero, then this field is the
    // global system interrupt number to which the pin is connected.
    //
    UINT32 SourceIndex;

    //
    // The current legacy IRQ or GSI that the pin is connected to.
    //
    UINT32 CrsNumber;
    UINT8  CrsInfo;
    UINT8  CrsIsShared : 1;
    UINT8  CrsIsPolarityLow : 1;
    UINT8  CrsIsEdge : 1;
    UINT8  CrsIsLegacyIrq : 1; /* Indicates if CrsNumber is a legacy IRQ or GSi. */

    //
    // The device that allocates the interrupt to which the above pin is connected,
    // if the pointer is NULL, then allocate the interrupt from the global interrupt pool.
    //
    struct _AML_NAMESPACE_NODE* Source;
} AML_PCI_PRT_ENTRY;

//
// Free PCI device information.
//
VOID
AmlPciInformationFree(
    _Inout_ AML_PCI_INFORMATION* Info
    );

//
// Resolve the current address of a PCI device, taking into account any PCI bridge links.
//
_Success_( return )
BOOLEAN
AmlPciResolveDeviceAddress(
    _Inout_ AML_HOST_CONTEXT*     Host,
    _Inout_ AML_PCI_INFORMATION*  Info,
    _Out_   AML_PCI_SBDF_ADDRESS* Address
    );
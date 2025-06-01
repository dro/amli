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
#pragma once

#include "aml_platform.h"
#include "aml_heap.h"

//
// PCI Bus/Device/Function pair address.
//
typedef struct _AML_PCI_SBDF_ADDRESS {
    UINT16 Segment;
    UINT8  Bus;
    UINT8  Device;
    UINT8  Function;
} AML_PCI_SBDF_ADDRESS;

//
// PCI-to-PCI bridge link level between a root bus and a device.
//
typedef struct _AML_PCI_BRIDGE {
    struct _AML_PCI_BRIDGE* Next;
    UINT8                   Device;
    UINT8                   Function;
    AML_PCI_SBDF_ADDRESS    ResolvedAddress;
} AML_PCI_BRIDGE;

//
// Evaluated PCI information of an AML device.
// Contains all bridge links from the root bus to the device (if any).
//
typedef struct _AML_PCI_INFORMATION {
    AML_HEAP*            Heap;
    AML_PCI_BRIDGE*      BridgeHead;
    AML_PCI_SBDF_ADDRESS RootBusAddress;
    AML_PCI_SBDF_ADDRESS Address;
} AML_PCI_INFORMATION;
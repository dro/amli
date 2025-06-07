#include "aml_host.h"
#include "aml_pci.h"
#include "aml_debug.h"

//
// Free PCI device information.
//
VOID
AmlPciInformationFree(
    _Inout_ AML_PCI_INFORMATION* Info
    )
{
    AML_PCI_BRIDGE* Bridge;
    AML_PCI_BRIDGE* BridgeNext;

    //
    // Free all bridge allocations.
    //
    for( Bridge = Info->BridgeHead; Bridge != NULL; Bridge = BridgeNext ) {
        BridgeNext = Bridge->Next;
        AmlHeapFree( Info->Heap, Bridge );
    }
    Info->BridgeHead = NULL;
}

//
// Resolve the current address of a PCI device connected over a bridge.
//
_Success_( return )
static
BOOLEAN
AmlPciResolveBridgedAddress(
    _Inout_  AML_HOST_CONTEXT*     Host,
    _In_     AML_PCI_SBDF_ADDRESS  RootBusAddress,
    _In_opt_ AML_PCI_BRIDGE*       LastBridge,
    _In_     AML_PCI_SBDF_ADDRESS  LocalAddress,
    _Out_    AML_PCI_SBDF_ADDRESS* ResolvedAddress
    )
{
    UINT8 HeaderType;
    UINT8 HeaderLayout;
    UINT8 SecondaryBus;

    //
    // If this is the link, use the address on the root bus as-is.
    //
    if( LastBridge == NULL ) {
        *ResolvedAddress = ( AML_PCI_SBDF_ADDRESS ){
            .Segment  = RootBusAddress.Segment,
            .Bus      = RootBusAddress.Bus,
            .Device   = LocalAddress.Device,
            .Function = LocalAddress.Function,
        };
        return AML_TRUE;
    }

    //
    // Read from the configuration space of the last bridge to determine the bus of the current bridge.
    //
    HeaderType = AmlHostPciConfigRead8( Host, LastBridge->ResolvedAddress, AML_PCI_CFS_OFFSET_HEADER_TYPE );
    HeaderLayout = ( HeaderType & AML_PCI_CFS_MASK_HEADER_TYPE_LAYOUT );
    if( ( HeaderLayout != AML_PCI_CFS_HEADER_TYPE_PCI_TO_PCI_BRIDGE )
        && ( HeaderLayout != AML_PCI_CFS_HEADER_TYPE_PCI_TO_CARDBUS_BRIDGE ) )
    {
        return AML_FALSE;
    }
    SecondaryBus = AmlHostPciConfigRead8( Host, LastBridge->ResolvedAddress, AML_PCI_CFS_OFFSET_BRIDGE_SECONDARY_BUS );

    //
    // Use the resolved secondary bus index from the last bridge as the bus for the current address.
    //
    *ResolvedAddress = ( AML_PCI_SBDF_ADDRESS ){
        .Segment  = RootBusAddress.Segment,
        .Bus      = SecondaryBus,
        .Device   = LocalAddress.Device,
        .Function = LocalAddress.Function,
    };

    return AML_TRUE;
}

//
// Resolve the current address of a PCI device, taking into account any PCI bridge links.
//
_Success_( return )
BOOLEAN
AmlPciResolveDeviceAddress(
    _Inout_ AML_HOST_CONTEXT*     Host,
    _Inout_ AML_PCI_INFORMATION*  Info,
    _Out_   AML_PCI_SBDF_ADDRESS* Address
    )
{
    AML_PCI_BRIDGE*      LastBridge;
    AML_PCI_BRIDGE*      Bridge;
    AML_PCI_SBDF_ADDRESS LocalAddress;

    //
    // If the PCI device that the region is referencing is connected over bridges,
    // attempt to resolve all the current bus numbers of each linked bridge.
    //
    LastBridge = NULL;
    for( Bridge = Info->BridgeHead; Bridge != NULL; Bridge = Bridge->Next ) {
        LocalAddress = ( AML_PCI_SBDF_ADDRESS ){ .Device = Bridge->Device, .Function = Bridge->Function };
        if( AmlPciResolveBridgedAddress( Host, Info->RootBusAddress, LastBridge,
                                         LocalAddress, &Bridge->ResolvedAddress ) == AML_FALSE )
        {
            return AML_FALSE;
        }
        LastBridge = Bridge;
    }

    //
    // If this is a bridged connection, read the final bus number from the last bridge.
    //
    if( LastBridge != NULL ) {
        if( AmlPciResolveBridgedAddress( Host, Info->RootBusAddress, LastBridge, Info->Address, Address ) == AML_FALSE ) {
            return AML_FALSE;
        }
    } else {
        *Address = Info->Address;
    }

    return AML_TRUE;
}
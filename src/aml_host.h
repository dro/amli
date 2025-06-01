#pragma once

#include "aml_platform.h"
#include "aml_pci_info.h"
#include "aml_object.h"

//
// User-provided host context type.
//
typedef struct _AML_HOST_CONTEXT AML_HOST_CONTEXT;

//
// Attempt to acquire the global lock.
// Returns AML_TRUE if the global lock was acquired and we have taken ownership.
// Returns AML_FALSE if the global lock was already owned, and we have set the pending bit.
//
BOOLEAN
AmlHostGlobalLockTryAcquire(
	_Inout_ AML_HOST_CONTEXT* Host
	);

//
// Release ownership of the global lock.
// Returns AML_TRUE if the lock was originally pending, meaning the caller should signal pending waiters.
//
BOOLEAN
AmlHostGlobalLockRelease(
	_Inout_ AML_HOST_CONTEXT* Host
	);

//
// Called upon successful initialization of a device through the _INI method.
//
VOID
AmlHostOnDeviceInitialized(
	_Inout_ AML_HOST_CONTEXT* Host,
	_Inout_ AML_OBJECT*       Object,
	_In_    UINT32            DeviceStatus
	);

//
// Allows the host to process/output debug logs produced by the interpreter.
//
VOID
AmlHostDebugPrintV(
	_In_   struct _AML_HOST_CONTEXT* Host,
	_In_   INT                       LogLevel,
	_In_z_ const CHAR*               Format,
	_In_   va_list                   VaList
	);

//
// Map a region of physical memory to virtual address space.
// The given PhysicalAddress is not page aligned, it should be rounded down to page granularity before being mapped.
// The returned virtual address must not be the rounded address, but rather the corresponding offset into the virtual page.
//
_Success_( return )
BOOLEAN
AmlHostMemoryMap(
	_Inout_  AML_HOST_CONTEXT* Host,
	_In_     UINT64            PhysicalAddress,
	_In_     UINT64            Size,
	_In_     UINT32            Flags,
	_Outptr_ VOID**            ppMappedAddress
	);

//
// Unmap a region of virtual address space previously returned by AmlHostMemoryMap.
//
_Success_( return )
BOOLEAN
AmlHostMemoryUnmap(
	_Inout_  AML_HOST_CONTEXT* Host,
	_In_     VOID*             MappedAddress,
	_In_     UINT64            Size
	);

//
// Search for the ACPI table with the given properties.
// Returns NULL if the given table wasn't found.
//
_Success_( return != NULL )
AML_DESCRIPTION_HEADER*
AmlHostSearchAcpiTableEx(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT32            Signature,
	_In_    AML_OEM_ID        OemId,
	_In_    UINT64            OemTableId
	);

//
// Create an internal OS mutex object.
//
_Success_( return )
BOOLEAN
AmlHostMutexCreate(
	_Inout_ AML_HOST_CONTEXT* Host,
	_Out_   UINT64*           MutexHandleOutput
	);

//
// Await signal of an internal OS mutex object for the given timeout.
// A TimeoutValue of 0xFFFF (or greater) indicates that there is no timeout and the operation will wait indefinitely.
//
AML_WAIT_STATUS
AmlHostMutexAcquire(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MutexHandle,
	_In_    UINT64            TimeoutMs
	);

//
// Release an internal OS mutex object that was previously acquired using AmlHostMutexAcquire.
//
VOID
AmlHostMutexRelease(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MutexHandle
	);

//
// Release an internal OS mutex object.
//
VOID
AmlHostMutexFree(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MutexHandle
	);

//
// Create an internal OS event object.
//
_Success_( return )
BOOLEAN
AmlHostEventCreate(
	_Inout_ AML_HOST_CONTEXT* Host,
	_Out_   UINT64*           EventHandleOutput
	);

//
// Free an internal OS event object.
//
VOID
AmlHostEventFree(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            EventHandle
	);

//
// Signal an internal OS event object.
//
VOID
AmlHostEventSignal(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            EventHandle
	);

//
// Reset an internal OS event object.
//
VOID
AmlHostEventReset(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            EventHandle
	);

//
// Await signal of an internal OS event object for the given timeout.
// A TimeoutValue of 0xFFFF (or greater) indicates that there is no timeout and the operation will wait indefinitely.
//
AML_WAIT_STATUS
AmlHostEventAwait(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            EventHandle,
	_In_    UINT64            TimeoutMs
	);

//
// Notify the OSPM of an event occurring for the given object.
//
VOID
AmlHostObjectNotification(
	_Inout_ AML_HOST_CONTEXT* Host,
	_Inout_ AML_OBJECT*       Object,
	_In_    UINT64            NotificationValue
	);

//
// Implements the sleep instruction, sleep for a long term amount of milliseconds,
// relinquishing control of the processor if performing multithreaded execution.
// Execution is delayed for at least the required number of milliseconds.
//
VOID
AmlHostSleep(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            Milliseconds
	);

//
// Implements the stall instruction, stall for the given amount of microseconds (should be below 100us),
// without relinquishing control of the current processor if multithreaded.
//
VOID
AmlHostStall(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MicroSeconds
	);

//
// 64-bit OS-provided monotonically increasing counter.
// The period of this timer is 100 nanoseconds (is not in default stub implementation).
//
UINT64
AmlHostMonotonicTimer(
	_Inout_ AML_HOST_CONTEXT* Host
	);

//
// Read from an 8-bit IO port.
//
UINT8
AmlHostIoPortRead8(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT16            PortIndex
	);

//
// Read from a 16-bit IO port.
//
UINT16
AmlHostIoPortRead16(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT16            PortIndex
	);

//
// Read from a 32-bit IO port.
//
UINT32
AmlHostIoPortRead32(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT16            PortIndex
	);

//
// Write to an 8-bit IO port.
//
VOID
AmlHostIoPortWrite8(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT16            PortIndex,
	_In_    UINT8             Value
	);

//
// Write to a 16-bit IO port.
//
VOID
AmlHostIoPortWrite16(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT16            PortIndex,
	_In_    UINT16            Value
	);

//
// Write to a 32-bit IO port.
//
VOID
AmlHostIoPortWrite32(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT16            PortIndex,
	_In_    UINT32            Value
	);

//
// Read from an 8-bit MMIO address.
//
UINT8
AmlHostMmioRead8(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress
	);

//
// Read from a 16-bit MMIO address.
//
UINT16
AmlHostMmioRead16(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress
	);

//
// Read from a 32-bit MMIO address.
//
UINT32
AmlHostMmioRead32(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress
	);

//
// Read from a 64-bit MMIO address.
//
UINT64
AmlHostMmioRead64(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress
	);

//
// Write to an 8-bit MMIO address.
//
VOID
AmlHostMmioWrite8(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress,
	_In_    UINT8             Value
	);

//
// Write to a 16-bit MMIO address.
//
VOID
AmlHostMmioWrite16(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress,
	_In_    UINT16            Value
	);

//
// Write to a 32-bit MMIO address.
//
VOID
AmlHostMmioWrite32(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress,
	_In_    UINT32            Value
	);

//
// Write to a 64-bit MMIO address.
//
VOID
AmlHostMmioWrite64(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress,
	_In_    UINT64            Value
	);

//
// Read from an 8-bit PCI configuration space register.
//
UINT8
AmlHostPciConfigRead8(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset
	);

//
// Read from a 16-bit PCI configuration space register.
//
UINT16
AmlHostPciConfigRead16(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset
	);

//
// Read from a 32-bit PCI configuration space register.
//
UINT32
AmlHostPciConfigRead32(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset
	);

//
// Read from a 64-bit PCI configuration space register.
//
UINT64
AmlHostPciConfigRead64(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset
	);

//
// Write to an 8-bit PCI configuration space register.
//
VOID
AmlHostPciConfigWrite8(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset,
	_In_    UINT8                Value
	);

//
// Write to a 16-bit PCI configuration space register.
//
VOID
AmlHostPciConfigWrite16(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset,
	_In_    UINT16                Value
	);

//
// Write to a 32-bit PCI configuration space register.
//
VOID
AmlHostPciConfigWrite32(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset,
	_In_    UINT32               Value
	);

//
// Write to a 64-bit PCI configuration space register.
//
VOID
AmlHostPciConfigWrite64(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset,
	_In_    UINT64               Value
	);
#include "aml_host.h"
#include "aml_debug.h"
#include "aml_namespace.h"

//
// Example stub implementation of an AML host interface.c.
//
#if 0

//
// Attempt to acquire the global lock.
// Returns AML_TRUE if the global lock was acquired and we have taken ownership.
// Returns AML_FALSE if the global lock was already owned, and we have set the pending bit.
//
BOOLEAN
AmlHostGlobalLockTryAcquire(
	_Inout_ AML_HOST_CONTEXT* Host
	)
{
	ULONG Current;
	ULONG Desired;

	//
	// If no global lock is present, assume that we always have access.
	//
	if( Host->GlobalLock == NULL ) {
		return AML_TRUE;
	}

	// Read the current value of the global lock, and set it up with our desired value.
	// If the lock is already owned, the pending bit will be set.
	// If the lock isn't owned, we will take ownership of the lock.
	//
	do {
		Current  = *Host->GlobalLock;
		Desired  = ( Current & ~ACPI_GLOBAL_LOCK_PENDING_FLAG );       /* Clear pending bit. */
		Desired |= ACPI_GLOBAL_LOCK_OWNED_FLAG;                        /* Set owned bit in desired value. */
		Desired |= ( ( Current & ACPI_GLOBAL_LOCK_OWNED_FLAG ) >> 1 ); /* If already owned, set pending bit. */
	} while( _InterlockedCompareExchange( Host->GlobalLock, Desired, Current ) != Desired );

	//
	// Indicate to the caller if we have taken ownership or just set the pending bit. 
	//
	return ( ( Desired & ( ACPI_GLOBAL_LOCK_OWNED_FLAG | ACPI_GLOBAL_LOCK_PENDING_FLAG ) ) == ACPI_GLOBAL_LOCK_OWNED_FLAG );
}

//
// Release ownership of the global lock.
// Returns AML_TRUE if the lock was originally pending, meaning the caller should signal pending waiters.
//
BOOLEAN
AmlHostGlobalLockRelease(
	_Inout_ AML_HOST_CONTEXT* Host
	)
{
	ULONG Current;
	ULONG Desired;

	//
	// If no global lock is present, assume that we always have access.
	//
	if( Host->GlobalLock == NULL ) {
		return AML_TRUE;
	}

	//
	// Attempt to clear the owned and pending field, fully releasing the lock.
	//
	do {
		Current = *Host->GlobalLock;
		Desired = ( Current & ~( ACPI_GLOBAL_LOCK_PENDING_FLAG | ACPI_GLOBAL_LOCK_OWNED_FLAG ) ); /* Clear owned and pending field. */
	} while( _InterlockedCompareExchange( Host->GlobalLock, Desired, Current ) != Desired );

	//
	// Return AML_TRUE if the lock was originally pending, in this case,
	// the caller should signal that the lock was released, so that
	// any pending code waiting on the lock can attempt to acquire it.
	// (GBL_RLS or BIOS_RLS).
	//
	return ( ( Current & ACPI_GLOBAL_LOCK_PENDING_FLAG ) != 0 );
}

//
// Called upon successful initialization of a device through the _INI method.
//
VOID
AmlHostOnDeviceInitialized(
	_Inout_ AML_HOST_CONTEXT* Host,
	_Inout_ AML_OBJECT*       Object,
	_In_    UINT32            DeviceStatus
	)
{

}

//
// Allows the host to process/output debug logs produced by the interpreter.
//
VOID
AmlHostDebugPrintV(
	_In_   struct _AML_HOST_CONTEXT* Host,
	_In_   AML_DEBUG_LEVEL           LogLevel,
	_In_z_ const CHAR*               Format,
	_In_   va_list                   VaList
	)
{

}

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
	)
{
	*ppMappedAddress = ( VOID* )PhysicalAddress;
	return AML_TRUE;
}

//
// Unmap a region of virtual address space previously returned by AmlHostMemoryMap.
//
_Success_( return )
BOOLEAN
AmlHostMemoryUnmap(
	_Inout_  AML_HOST_CONTEXT* Host,
	_In_     VOID*             MappedAddress,
	_In_     UINT64            Size
	)
{
	return AML_TRUE;
}

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
	)
{
	return NULL;
}

//
// Create an internal OS mutex object.
//
_Success_( return )
BOOLEAN
AmlHostMutexCreate(
	_Inout_ AML_HOST_CONTEXT* Host,
	_Out_   UINT64*           MutexHandleOutput
	)
{
	*MutexHandleOutput = MAXUINT64;
	return AML_TRUE;
}

//
// Await signal of an internal OS mutex object for the given timeout.
// A TimeoutValue of 0xFFFF (or greater) indicates that there is no timeout and the operation will wait indefinitely.
//
AML_WAIT_STATUS
AmlHostMutexAcquire(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MutexHandle,
	_In_    UINT64            TimeoutMs
	)
{
	return AML_WAIT_STATUS_SUCCESS;
}

//
// Release an internal OS mutex object that was previously acquired using AmlHostMutexAcquire.
//
VOID
AmlHostMutexRelease(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MutexHandle
	)
{
	
}

//
// Release an internal OS mutex object.
//
VOID
AmlHostMutexFree(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MutexHandle
	)
{
	
}


//
// Create an internal OS event object.
//
_Success_( return )
BOOLEAN
AmlHostEventCreate(
	_Inout_ AML_HOST_CONTEXT* Host,
	_Out_   UINT64*           EventHandleOutput
	)
{
	*EventHandleOutput = MAXUINT64;
	return AML_TRUE;
}

//
// Free an internal OS event object.
//
VOID
AmlHostEventFree(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            EventHandle
	)
{

}

//
// Signal an internal OS event object.
//
VOID
AmlHostEventSignal(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            EventHandle
	)
{

}

//
// Reset an internal OS event object.
//
VOID
AmlHostEventReset(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            EventHandle
	)
{

}

//
// Await signal of an internal OS event object for the given timeout.
// A TimeoutValue of 0xFFFF (or greater) indicates that there is no timeout and the operation will wait indefinitely.
//
AML_WAIT_STATUS
AmlHostEventAwait(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            EventHandle,
	_In_    UINT64            TimeoutMs
	)
{
	return AML_WAIT_STATUS_SUCCESS;
}


//
// Notify the OSPM of an event occurring for the given object.
//
VOID
AmlHostObjectNotification(
	_Inout_ AML_HOST_CONTEXT* Host,
	_Inout_ AML_OBJECT*       Object,
	_In_    UINT64            NotificationValue
	)
{
	
}

//
// Implements the sleep instruction, sleep for a long term amount of milliseconds,
// relinquishing control of the processor if performing multithreaded execution.
// Execution is delayed for at least the required number of milliseconds.
//
VOID
AmlHostSleep(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            Milliseconds
	)
{

}

//
// Implements the stall instruction, stall for the given amount of microseconds (should be below 100us),
// without relinquishing control of the current processor if multithreaded.
//
VOID
AmlHostStall(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MicroSeconds
	)
{
	UINT64 End;

	//
	// Attempt to stall the current processor for the given amount of microseconds (or a reasonable cap).
	//
	MicroSeconds = AML_MIN( MicroSeconds, ( 100 + 50 ) );
	End = ( AmlHostMonotonicTimer( Host ) + ( MicroSeconds * 1000 ) );
	while( AmlHostMonotonicTimer( Host ) < End ) {
		AML_PAUSE();
	}
}

//
// 64-bit OS-provided monotonically increasing counter.
// The period of this timer is 100 nanoseconds (is not in default stub implementation).
//
UINT64
AmlHostMonotonicTimer(
	_Inout_ AML_HOST_CONTEXT* Host
	)
{
	//
	// Read stub timer value, not spec adherent (not 100ns).
	//
	return ( __rdtsc() / 400 );
}

//
// Read from an 8-bit IO port.
//
UINT8
AmlHostIoPortRead8(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT16            PortIndex
	)
{
	return __inbyte( PortIndex );
}

//
// Read from a 16-bit IO port.
//
UINT16
AmlHostIoPortRead16(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT16            PortIndex
	)
{
	return __inword( PortIndex );
}

//
// Read from a 32-bit IO port.
//
UINT32
AmlHostIoPortRead32(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT16            PortIndex
	)
{
	return __indword( PortIndex );
}

//
// Write to an 8-bit IO port.
//
VOID
AmlHostIoPortWrite8(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT16            PortIndex,
	_In_    UINT8             Value
	)
{
	__outbyte( PortIndex, Value );
}

//
// Write to a 16-bit IO port.
//
VOID
AmlHostIoPortWrite16(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT16            PortIndex,
	_In_    UINT16            Value
	)
{
	__outword( PortIndex, Value );
}

//
// Write to a 32-bit IO port.
//
VOID
AmlHostIoPortWrite32(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT16            PortIndex,
	_In_    UINT32            Value
	)
{
	__outdword( PortIndex, Value );
}

//
// Read from an 8-bit MMIO address.
//
UINT8
AmlHostMmioRead8(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress
	)
{
	return *( volatile UINT8* )MmioAddress;
}

//
// Read from a 16-bit MMIO address.
//
UINT16
AmlHostMmioRead16(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress
	)
{
	return *( volatile UINT16* )MmioAddress;
}

//
// Read from a 32-bit MMIO address.
//
UINT32
AmlHostMmioRead32(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress
	)
{
	return *( volatile UINT32* )MmioAddress;
}

//
// Read from a 64-bit MMIO address.
//
UINT64
AmlHostMmioRead64(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress
	)
{
	return *( volatile UINT64* )MmioAddress;
}

//
// Write to an 8-bit MMIO address.
//
VOID
AmlHostMmioWrite8(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress,
	_In_    UINT8             Value
	)
{
	*( volatile UINT8* )MmioAddress = Value;
}

//
// Write to a 16-bit MMIO address.
//
VOID
AmlHostMmioWrite16(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress,
	_In_    UINT16            Value
	)
{
	*( volatile UINT16* )MmioAddress = Value;
}

//
// Write to a 32-bit MMIO address.
//
VOID
AmlHostMmioWrite32(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress,
	_In_    UINT32            Value
	)
{
	*( volatile UINT32* )MmioAddress = Value;
}

//
// Write to a 64-bit MMIO address.
//
VOID
AmlHostMmioWrite64(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            MmioAddress,
	_In_    UINT64            Value
	)
{
	*( volatile UINT64* )MmioAddress = Value;
}

//
// Read from an 8-bit PCI configuration space register.
//
UINT8
AmlHostPciConfigRead8(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset
	)	
{
	return 0xFF;
}

//
// Read from a 16-bit PCI configuration space register.
//
UINT16
AmlHostPciConfigRead16(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset
	)
{
	return 0xFFFF;
}

//
// Read from a 32-bit PCI configuration space register.
//
UINT32
AmlHostPciConfigRead32(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset
	)
{
	return 0xFFFFFFFF;
}

//
// Read from a 64-bit PCI configuration space register.
//
UINT64
AmlHostPciConfigRead64(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset
	)
{
	return 0xFFFFFFFFFFFFFFFF;
}

//
// Write to an 8-bit PCI configuration space register.
//
VOID
AmlHostPciConfigWrite8(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset,
	_In_    UINT8                Value
	)
{

}

//
// Write to a 16-bit PCI configuration space register.
//
VOID
AmlHostPciConfigWrite16(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset,
	_In_    UINT16                Value
	)
{

}

//
// Write to a 32-bit PCI configuration space register.
//
VOID
AmlHostPciConfigWrite32(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset,
	_In_    UINT32               Value
	)
{

}

//
// Write to a 64-bit PCI configuration space register.
//
VOID
AmlHostPciConfigWrite64(
	_Inout_ AML_HOST_CONTEXT*    Host,
	_In_    AML_PCI_SBDF_ADDRESS Address,
	_In_    UINT64               Offset,
	_In_    UINT64               Value
	)
{

}

#endif
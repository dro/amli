#include <stdio.h>
#include "runtest_host.h"
#include "aml_host.h"
#include "aml_debug.h"
#include "aml_namespace.h"

//
// ACPI global lock format.
//
#define ACPI_GLOBAL_LOCK_PENDING_SHIFT 0ul
#define ACPI_GLOBAL_LOCK_PENDING_FLAG  (1ul << 0) /* Non-zero indicates that a request for ownership of the Global Lock is pending. */
#define ACPI_GLOBAL_LOCK_OWNED_SHIFT   1ul
#define ACPI_GLOBAL_LOCK_OWNED_FLAG    (1ul << 1) /* Non-zero indicates that the Global Lock is Owned. */

//
// Interlocked/atomic compare and exchange intrinsic.
// Behaves like _InterlockedCompareExchange from MSVC.
//
#ifdef __has_builtin
 #if __has_builtin(__sync_val_compare_and_swap)
  #define AML_ATOMIC_COMPARE_EXCHANGE_32(pLongDestination, LongDesired, LongComparand) \
   __sync_val_compare_and_swap((pLongDestination), (LongComparand), (LongDesired))
 #endif
#endif
#ifndef AML_ATOMIC_COMPARE_EXCHANGE_32
 #ifdef _MSC_VER
  #define AML_ATOMIC_COMPARE_EXCHANGE_32(pLongDestination, LongDesired, LongComparand) \
   _InterlockedCompareExchange((pLongDestination), (LongDesired), (LongComparand))
 #endif
#endif

//
// RDTSC intrinsic.
//
#ifndef AML_RDTSC
 #ifdef __has_builtin
  #if __has_builtin(__builtin_ia32_rdtsc)
   #define AML_RDTSC() __builtin_ia32_rdtsc()
  #endif
 #endif
#endif
#ifndef AML_RDTSC
 #ifdef _MSC_VER
  #define AML_RDTSC() __rdtsc()
 #endif
#endif

//
// Host debug print helpers.
//
#ifndef AML_HOST_PRINTF
 #ifndef AML_BUILD_FUZZER
  #define AML_HOST_PRINTF(...) printf(__VA_ARGS__)
 #else
  #define AML_HOST_PRINTF(...) ((VOID)0)
 #endif
#endif
#ifndef AML_HOST_VPRINTF
 #ifndef AML_BUILD_FUZZER
  #define AML_HOST_VPRINTF(...) vprintf(__VA_ARGS__)
 #else
  #define AML_HOST_VPRINTF(...) ((VOID)0)
 #endif
#endif

//
// Debug ACPI table returned by AmlHostSearchAcpiTableEx.
//
static unsigned char AmlDebugTable0[ ] = {
	0x53, 0x53, 0x44, 0x54, 0xC7, 0x01, 0x00, 0x00, 0x01, 0x3B, 0x50, 0x6D,
	0x52, 0x65, 0x66, 0x00, 0x4C, 0x61, 0x6B, 0x65, 0x54, 0x69, 0x6E, 0x79,
	0x00, 0x30, 0x00, 0x00, 0x49, 0x4E, 0x54, 0x4C, 0x17, 0x11, 0x05, 0x20,
	0x10, 0x4D, 0x0C, 0x5C, 0x2F, 0x03, 0x5F, 0x53, 0x42, 0x5F, 0x50, 0x43,
	0x49, 0x30, 0x53, 0x41, 0x54, 0x30, 0x14, 0x32, 0x53, 0x4C, 0x54, 0x31,
	0x08, 0xA0, 0x29, 0x5B, 0x12, 0x5C, 0x2F, 0x03, 0x5F, 0x50, 0x52, 0x5F,
	0x43, 0x50, 0x55, 0x30, 0x47, 0x45, 0x41, 0x52, 0x00, 0x70, 0x00, 0x5C,
	0x2F, 0x03, 0x5F, 0x50, 0x52, 0x5F, 0x43, 0x50, 0x55, 0x30, 0x47, 0x45,
	0x41, 0x52, 0x5C, 0x50, 0x4E, 0x4F, 0x54, 0xA4, 0x00, 0x14, 0x32, 0x53,
	0x4C, 0x54, 0x32, 0x08, 0xA0, 0x29, 0x5B, 0x12, 0x5C, 0x2F, 0x03, 0x5F,
	0x50, 0x52, 0x5F, 0x43, 0x50, 0x55, 0x30, 0x47, 0x45, 0x41, 0x52, 0x00,
	0x70, 0x01, 0x5C, 0x2F, 0x03, 0x5F, 0x50, 0x52, 0x5F, 0x43, 0x50, 0x55,
	0x30, 0x47, 0x45, 0x41, 0x52, 0x5C, 0x50, 0x4E, 0x4F, 0x54, 0xA4, 0x00,
	0x14, 0x33, 0x53, 0x4C, 0x54, 0x33, 0x08, 0xA0, 0x2A, 0x5B, 0x12, 0x5C,
	0x2F, 0x03, 0x5F, 0x50, 0x52, 0x5F, 0x43, 0x50, 0x55, 0x30, 0x47, 0x45,
	0x41, 0x52, 0x00, 0x70, 0x0A, 0x02, 0x5C, 0x2F, 0x03, 0x5F, 0x50, 0x52,
	0x5F, 0x43, 0x50, 0x55, 0x30, 0x47, 0x45, 0x41, 0x52, 0x5C, 0x50, 0x4E,
	0x4F, 0x54, 0xA4, 0x00, 0x14, 0x21, 0x47, 0x4C, 0x54, 0x53, 0x08, 0x70,
	0x5C, 0x2F, 0x03, 0x5F, 0x50, 0x52, 0x5F, 0x43, 0x50, 0x55, 0x30, 0x47,
	0x45, 0x41, 0x52, 0x60, 0x79, 0x60, 0x01, 0x60, 0x7D, 0x60, 0x01, 0x60,
	0xA4, 0x60, 0x10, 0x44, 0x0D, 0x5C, 0x2F, 0x03, 0x5F, 0x53, 0x42, 0x5F,
	0x50, 0x43, 0x49, 0x30, 0x53, 0x41, 0x54, 0x31, 0x14, 0x32, 0x53, 0x4C,
	0x54, 0x31, 0x08, 0xA0, 0x29, 0x5B, 0x12, 0x5C, 0x2F, 0x03, 0x5F, 0x50,
	0x52, 0x5F, 0x43, 0x50, 0x55, 0x30, 0x47, 0x45, 0x41, 0x52, 0x00, 0x70,
	0x00, 0x5C, 0x2F, 0x03, 0x5F, 0x50, 0x52, 0x5F, 0x43, 0x50, 0x55, 0x30,
	0x47, 0x45, 0x41, 0x52, 0x5C, 0x50, 0x4E, 0x4F, 0x54, 0xA4, 0x00, 0x14,
	0x32, 0x53, 0x4C, 0x54, 0x32, 0x08, 0xA0, 0x29, 0x5B, 0x12, 0x5C, 0x2F,
	0x03, 0x5F, 0x50, 0x52, 0x5F, 0x43, 0x50, 0x55, 0x30, 0x47, 0x45, 0x41,
	0x52, 0x00, 0x70, 0x01, 0x5C, 0x2F, 0x03, 0x5F, 0x50, 0x52, 0x5F, 0x43,
	0x50, 0x55, 0x30, 0x47, 0x45, 0x41, 0x52, 0x5C, 0x50, 0x4E, 0x4F, 0x54,
	0xA4, 0x00, 0x14, 0x33, 0x53, 0x4C, 0x54, 0x33, 0x08, 0xA0, 0x2A, 0x5B,
	0x12, 0x5C, 0x2F, 0x03, 0x5F, 0x50, 0x52, 0x5F, 0x43, 0x50, 0x55, 0x30,
	0x47, 0x45, 0x41, 0x52, 0x00, 0x70, 0x0A, 0x02, 0x5C, 0x2F, 0x03, 0x5F,
	0x50, 0x52, 0x5F, 0x43, 0x50, 0x55, 0x30, 0x47, 0x45, 0x41, 0x52, 0x5C,
	0x50, 0x4E, 0x4F, 0x54, 0xA4, 0x00, 0x14, 0x28, 0x47, 0x4C, 0x54, 0x53,
	0x08, 0x70, 0x5C, 0x2F, 0x03, 0x5F, 0x50, 0x52, 0x5F, 0x43, 0x50, 0x55,
	0x30, 0x47, 0x45, 0x41, 0x52, 0x60, 0x79, 0x60, 0x01, 0x60, 0x7B, 0x4D,
	0x50, 0x4D, 0x46, 0x01, 0x61, 0x7D, 0x60, 0x61, 0x60, 0xA4, 0x60
};

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

	//
	// Read the current value of the global lock, and set it up with our desired value.
	// If the lock is already owned, the pending bit will be set.
	// If the lock isn't owned, we will take ownership of the lock.
	//
	do {
		Current  = *Host->GlobalLock;
		Desired  = ( Current & ~ACPI_GLOBAL_LOCK_PENDING_FLAG );       /* Clear pending bit. */
		Desired |= ACPI_GLOBAL_LOCK_OWNED_FLAG;                        /* Set owned bit in desired value. */
		Desired |= ( ( Current & ACPI_GLOBAL_LOCK_OWNED_FLAG ) >> 1 ); /* If already owned, set pending bit. */
	} while( AML_ATOMIC_COMPARE_EXCHANGE_32( Host->GlobalLock, Desired, Current ) != Current );

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
	} while( AML_ATOMIC_COMPARE_EXCHANGE_32( Host->GlobalLock, Desired, Current ) != Current );

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
	AML_HOST_PRINTF( "Host: AmlHostOnDeviceInitialized: " );
	if( Object->NamespaceNode != NULL ) {
		AML_HOST_PRINTF( "%.4s", Object->NamespaceNode->LocalName.Data );
	} else {
		AML_HOST_PRINTF( "[Unknown]" );
	}
	AML_HOST_PRINTF( " (Status=0x%x)\n", DeviceStatus );
}

//
// Allows the host to process/output debug logs produced by the interpreter.
//
VOID
AmlHostDebugPrintV(
	_In_   struct _AML_HOST_CONTEXT* Host,
	_In_   INT                       LogLevel,
	_In_z_ const CHAR*               Format,
	_In_   va_list                   VaList
	)
{
	const CHAR* LogLevelPrefixes[] = {
		[ AML_DEBUG_LEVEL_TRACE ]   = "\033[39m",
		[ AML_DEBUG_LEVEL_INFO ]    = "\033[39m",
		[ AML_DEBUG_LEVEL_WARNING ] = "\033[33m",
		[ AML_DEBUG_LEVEL_ERROR ]   = "\033[31m",
		[ AML_DEBUG_LEVEL_FATAL ]   = "\033[31;1;4m",
	};
	if( ( LogLevel >= 0 ) && ( LogLevel < AML_COUNTOF( LogLevelPrefixes ) ) ) {
		// AML_HOST_PRINTF( "%s", LogLevelPrefixes[ LogLevel ] );
	}
	AML_HOST_VPRINTF( Format, VaList );
	// AML_HOST_PRINTF( "\033[39m\033[49m" );
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
	AML_HOST_PRINTF( "Host: Mapping physical address 0x%"PRIx64" (size=0x%"PRIx64") (virtual=0x%"PRIx64")\n", PhysicalAddress, Size, PhysicalAddress );
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
	AML_HOST_PRINTF( "Host: Unmapping virtual address 0x%p (size: 0x%"PRIx64")\n", MappedAddress, Size );
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
	return ( AML_DESCRIPTION_HEADER* )AmlDebugTable0;
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
	*MutexHandleOutput = AML_RDTSC();
	AML_HOST_PRINTF( "Host: Creating internal mutex object: 0x%"PRIx64"\n", *MutexHandleOutput );
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
	AML_HOST_PRINTF( "Host: Awaiting acquire of internal mutex object: 0x%"PRIx64" (timeout: 0x%"PRIx64")\n", MutexHandle, TimeoutMs );
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
	AML_HOST_PRINTF( "Host: Releasing internal mutex object: 0x%"PRIx64"\n", MutexHandle );
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
	AML_HOST_PRINTF( "Host: Freeing internal mutex object: 0x%"PRIx64"\n", MutexHandle );
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
	*EventHandleOutput = AML_RDTSC();
	AML_HOST_PRINTF( "Host: Creating internal event object: 0x%"PRIx64"\n", *EventHandleOutput );
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
	AML_HOST_PRINTF( "Host: Freeing internal event object: 0x%"PRIx64"\n", EventHandle );
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
	AML_HOST_PRINTF( "Host: Signalling internal event object: 0x%"PRIx64"\n", EventHandle );
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
	AML_HOST_PRINTF( "Host: Resetting internal event object: 0x%"PRIx64"\n", EventHandle );
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
	AML_HOST_PRINTF( "Host: Awaiting signal of internal event object: 0x%"PRIx64" (timeout: 0x%"PRIx64")\n", EventHandle, TimeoutMs );
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
	AML_HOST_PRINTF( "Host: Received notification value (%"PRId64") for object \"", NotificationValue );
	if( Object->NamespaceNode != NULL ) {
		AML_HOST_PRINTF( "%.4s", Object->NamespaceNode->LocalName.Data );
	} else {
		AML_HOST_PRINTF( "[Unknown]" );
	}
	AML_HOST_PRINTF( "\"\n" );
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
	Milliseconds = AML_MIN( Milliseconds, 5000 );
	AML_HOST_PRINTF( "Host: Sleeping for %"PRIu64" millisecond(s).\n", Milliseconds );
	// Sleep( ( DWORD )Milliseconds );
}

//
// Implements the stall instruction, stall for the given amount of microseconds (should be below 100us),
// without relinquishing control of the current processor if multithreaded.
//
VOID
AmlHostStall(
	_Inout_ AML_HOST_CONTEXT* Host,
	_In_    UINT64            Microseconds
	)
{
	UINT64 End;

	//
	// Attempt to stall the current processor for the given amount of microseconds (or a reasonable cap).
	//
	AML_HOST_PRINTF( "Host: Stalling for %"PRId64" microsecond(s).\n", Microseconds );
	Microseconds = AML_MIN( Microseconds, ( 100 + 50 ) );
	End = ( AmlHostMonotonicTimer( Host ) + ( Microseconds * 1000 ) );
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
	UINT64 Value;

	//
	// Read stub timer value, not spec adherent (not 100ns).
	//
	Value = ( AML_RDTSC() / 400 );
	// AML_HOST_PRINTF( "Host: Read from monotonic timer: 0x%"PRIx64"\n", Value );
	return Value;
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
	AML_HOST_PRINTF( "Host: Read8 from IO port 0x%x\n", ( UINT )PortIndex );
	return 0xFF;
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
	AML_HOST_PRINTF( "Host: Read16 from IO port 0x%x\n", ( UINT )PortIndex );
	return 0xFFFF;
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
	AML_HOST_PRINTF( "Host: Read32 from IO port 0x%x\n", ( UINT )PortIndex );
	return 0xFFFFFFFF;
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
	AML_HOST_PRINTF( "Host: Write8 0x%"PRIx64" to IO port 0x%x\n", ( UINT64 )Value, ( UINT )PortIndex );
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
	AML_HOST_PRINTF( "Host: Write16 0x%"PRIx64" to IO port 0x%x\n", ( UINT64 )Value, ( UINT )PortIndex );
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
	AML_HOST_PRINTF( "Host: Write32 0x%"PRIx64" to IO port 0x%x\n", ( UINT64 )Value, ( UINT )PortIndex );
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
	AML_HOST_PRINTF( "Host: Read8 from MMIO address 0x%"PRIx64"\n", MmioAddress );
	return 0xFF;
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
	AML_HOST_PRINTF( "Host: Read16 from MMIO address 0x%"PRIx64"\n", MmioAddress );
	return 0xFFFF;
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
	AML_HOST_PRINTF( "Host: Read32 from MMIO address 0x%"PRIx64"\n", MmioAddress );
	return 0xFFFFFFFF;
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
	AML_HOST_PRINTF( "Host: Read64 from MMIO address 0x%"PRIx64"\n", MmioAddress );
	return 0xFFFFFFFFFFFFFFFF;
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
	AML_HOST_PRINTF( "Host: Write8 0x%"PRIx64" to MMIO address 0x%"PRIx64"\n", ( UINT64 )Value, MmioAddress );
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
	AML_HOST_PRINTF( "Host: Write16 0x%"PRIx64" to MMIO address 0x%"PRIx64"\n", ( UINT64 )Value, MmioAddress );
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
	AML_HOST_PRINTF( "Host: Write32 0x%"PRIx64" to MMIO address 0x%"PRIx64"\n", ( UINT64 )Value, MmioAddress );
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
	AML_HOST_PRINTF( "Host: Write64 0x%"PRIx64" to MMIO address 0x%"PRIx64"\n", ( UINT64 )Value, MmioAddress );
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
	AML_HOST_PRINTF(
		"Host: Read8 from PCI configuration space (%u:%u:%u:%u) offset 0x%"PRIx64"\n",
		( UINT )Address.Segment,
		( UINT )Address.Bus,
		( UINT )Address.Device,
		( UINT )Address.Function,
		Offset
	);
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
	AML_HOST_PRINTF(
		"Host: Read16 from PCI configuration space (%u:%u:%u:%u) offset 0x%"PRIx64"\n",
		( UINT )Address.Segment,
		( UINT )Address.Bus,
		( UINT )Address.Device,
		( UINT )Address.Function,
		Offset
	);
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
	AML_HOST_PRINTF(
		"Host: Read32 from PCI configuration space (%u:%u:%u:%u) offset 0x%"PRIx64"\n",
		( UINT )Address.Segment,
		( UINT )Address.Bus,
		( UINT )Address.Device,
		( UINT )Address.Function,
		Offset
	);
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
	AML_HOST_PRINTF(
		"Host: Read64 from PCI configuration space (%u:%u:%u:%u) offset 0x%"PRIx64"\n",
		( UINT )Address.Segment,
		( UINT )Address.Bus,
		( UINT )Address.Device,
		( UINT )Address.Function,
		Offset
	);
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
	AML_HOST_PRINTF(
		"Host: Write8(0x%x) to PCI configuration space (%u:%u:%u:%u) offset 0x%"PRIx64"\n",
		( UINT )Value,
		( UINT )Address.Segment,
		( UINT )Address.Bus,
		( UINT )Address.Device,
		( UINT )Address.Function,
		Offset
	);
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
	AML_HOST_PRINTF(
		"Host: Write16(0x%x) to PCI configuration space (%u:%u:%u:%u) offset 0x%"PRIx64"\n",
		( UINT )Value,
		( UINT )Address.Segment,
		( UINT )Address.Bus,
		( UINT )Address.Device,
		( UINT )Address.Function,
		Offset
	);
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
	AML_HOST_PRINTF(
		"Host: Write32(0x%x) to PCI configuration space (%u:%u:%u:%u) offset 0x%"PRIx64"\n",
		( UINT )Value,
		( UINT )Address.Segment,
		( UINT )Address.Bus,
		( UINT )Address.Device,
		( UINT )Address.Function,
		Offset
	);
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
	AML_HOST_PRINTF(
		"Host: Write32(0x%"PRIx64") to PCI configuration space (%u:%u:%u:%u) offset 0x%"PRIx64"\n",
		Value,
		( UINT )Address.Segment,
		( UINT )Address.Bus,
		( UINT )Address.Device,
		( UINT )Address.Function,
		Offset
	);
}
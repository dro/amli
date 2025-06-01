#include "aml_state.h"
#include "aml_debug.h"
#include "aml_pci.h"
#include "aml_operation_region.h"

//
// Validate the access parameters for the given operation region.
// Some region types have different semantics and meanings to the region information,
// and thus must be validated against the access parameters differently.
//
_Success_( return )
static
BOOLEAN
AmlOperationRegionValidateAccess(
	_Inout_ struct _AML_STATE*           State,
	_Inout_ AML_OBJECT_OPERATION_REGION* Region,
	_In_    UINT64                       ByteOffset,
	_In_    UINT8                        AccessType,
	_In_    UINT64                       AccessBitWidth,
	_In_    SIZE_T                       DataBufferSize
	)
{
	//
	// Handle space types with different semantics, offload the responsibility of validation to the type-specific handlers.
	// Otherwise, the region access is validated generically against the bounds of the region.
	//
	switch( Region->SpaceType ) {
	case AML_REGION_SPACE_TYPE_GENERIC_SERIAL_BUS:
	case AML_REGION_SPACE_TYPE_SMBUS:
	case AML_REGION_SPACE_TYPE_IPMI:
	case AML_REGION_SPACE_TYPE_GENERAL_PURPOSE_IO:
		return AML_TRUE; /* Special semantic, implement in the handlers. */
	}

	//
	// Regular regions must not be BufferAcc.
	//
	if( AccessType == AML_FIELD_ACCESS_TYPE_BUFFER_ACC ) {
		return AML_FALSE;
	}

	//
	// Access bit-width must be a power-of-2 from 8,64 inclusive.
	//
	if( ( ( AccessBitWidth & ( AccessBitWidth - 1 ) ) != 0 )
		|| ( AccessBitWidth < 8 ) || ( AccessBitWidth > 64 ) )
	{
		return AML_FALSE;
	}

	//
	// Result buffer must be large enough to hold the given access width.
	//
	if( ( AccessBitWidth / CHAR_BIT ) > DataBufferSize ) {
		return AML_FALSE;
	}

	//
	// The operation region must be large enough to contain the given data.
	// The region properties (length, offset) are in bytes.
	//
	if( ( ByteOffset >= Region->Length ) || ( ( AccessBitWidth / CHAR_BIT ) > ( Region->Length - ByteOffset ) ) ) {
		return AML_FALSE;
	}

	return AML_TRUE;
}

//
// Ensures that the given operation region is mapped and present to be accessed.
// For an MMIO region this consists of mapping the physical base to virtual.
//
_Success_( return )
static
BOOLEAN
AmlOperationRegionEnsureMapped(
	_In_    const struct _AML_STATE*     State,
	_Inout_ AML_OBJECT_OPERATION_REGION* Region
	)
{
	//
	// The backing region is only mapped once, on-demand/lazily upon the first access, and unmapped upon destruction of the object.
	//
	if( Region->IsMapped ) {
		return AML_TRUE;
	}

	//
	// Currently only SystemMemory requires extra mapping semantics (may change soon for PCI access).
	//
	switch( Region->SpaceType ) {
	case AML_REGION_SPACE_TYPE_SYSTEM_MEMORY:
		Region->IsMapped = AmlHostMemoryMap( State->Host, Region->Offset, Region->Length, 0, &Region->MappedBase );
		return Region->IsMapped;
	}

	return AML_TRUE;
}

//
// Read from an operation region at byte-granularity.
// Access bit-width must be a power-of-2 from 8,64 inclusive.
//
_Success_( return )
BOOLEAN
AmlOperationRegionRead(
	_Inout_                                  struct _AML_STATE*           State,
	_Inout_                                  AML_OBJECT_OPERATION_REGION* Region,
	_Inout_opt_                              AML_OBJECT_FIELD*            Field,
	_In_                                     UINT64                       ByteOffset,
	_In_                                     UINT8                        AccessType,
	_In_                                     UINT8                        AccessAttribute,
	_In_                                     UINT64                       AccessBitWidth,
	_Out_writes_bytes_all_( ResultDataSize ) VOID*                        ResultData,
	_In_                                     SIZE_T                       ResultDataSize
	)
{
	SIZE_T                          i;
	AML_REGION_ACCESS_REGISTRATION* Handler;

	//
	// Validate the input parameters of the read for this region.
	//
	if( AmlOperationRegionValidateAccess( State, Region, ByteOffset, AccessType, AccessBitWidth, ResultDataSize ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Ensure that the backing data of the operation region is mapped and accessible.
	//
	if( AmlOperationRegionEnsureMapped( State, Region ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Zero out the entire result buffer if greater than the access size.
	// Done to adhere to the _Inout_ specification of the dispatcher/handler routines.
	//
	for( i = 0; i < ResultDataSize; i++ ) {
		( ( CHAR* )ResultData )[ i ] = 0;
	}

	//
	// See if an access handler has been registered for this type of region.
	//
	Handler = AmlLookupRegionSpaceAccessHandler( State, Region->SpaceType );
	if( ( Handler == NULL ) || ( Handler->UserRoutine == NULL ) ) {
		AML_DEBUG_ERROR( State, "Error: No handler registered for region space type: 0x%x.\n", ( UINT )Region->SpaceType );
		return AML_FALSE;
	}

	//
	// Allow the registered handler to service the read for this operation region.
	//
	return Handler->UserRoutine( State, Region, Handler->UserContext, Field,
								 AML_REGION_ACCESS_TYPE_READ, AccessAttribute, ByteOffset,
								 AccessBitWidth, ( AML_REGION_ACCESS_DATA* )ResultData );
}

//
// Write to an operation region at byte-granularity.
// Access bit-width must be a power-of-2 from 8,64 inclusive.
//
_Success_( return )
BOOLEAN
AmlOperationRegionWrite(
	_Inout_                      struct _AML_STATE*           State,
	_Inout_                      AML_OBJECT_OPERATION_REGION* Region,
	_Inout_opt_                  AML_OBJECT_FIELD*            Field,
	_In_                         UINT64                       ByteOffset,
	_In_                         UINT8                        AccessType,
	_In_                         UINT8                        AccessAttribute,
	_In_                         UINT64                       AccessBitWidth,
	_In_reads_bytes_( DataSize ) VOID*                        Data,
	_In_                         SIZE_T                       DataSize
	)
{
	AML_REGION_ACCESS_REGISTRATION* Handler;

	//
	// Validate the input parameters of the read for this region.
	//
	if( AmlOperationRegionValidateAccess( State, Region, ByteOffset, AccessType, AccessBitWidth, DataSize ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Ensure that the backing data of the operation region is mapped and accessible.
	//
	if( AmlOperationRegionEnsureMapped( State, Region ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// See if an access handler has been registered for this type of region.
	//
	Handler = AmlLookupRegionSpaceAccessHandler( State, Region->SpaceType );
	if( ( Handler == NULL ) || ( Handler->UserRoutine == NULL ) ) {
		AML_DEBUG_ERROR( State, "Error: No handler registered for region space type: 0x%x.\n", ( UINT )Region->SpaceType );
		return AML_FALSE;
	}

	//
	// Allow the registered handler to service the read for this operation region.
	//
	return Handler->UserRoutine( State, Region, Handler->UserContext, Field,
								 AML_REGION_ACCESS_TYPE_WRITE, AccessAttribute, ByteOffset,
								 AccessBitWidth, ( AML_REGION_ACCESS_DATA* )Data );
}

//
// Read from an I/O space operation region at byte granularity.
//
_Success_( return )
static
BOOLEAN
AmlOperationRegionReadSystemIo(
	_In_                                 const struct _AML_STATE*           State,
	_In_                                 const AML_OBJECT_OPERATION_REGION* Region,
	_In_                                 UINT64                             Offset,
	_In_                                 UINT64                             AccessBitWidth,
	_Out_writes_bytes_( ResultDataSize ) VOID*                              ResultData,
	_In_                                 SIZE_T                             ResultDataSize
	)
{
	UINT16 PortIndex;

	//
	// Validate input parameters.
	// Parameters are not checked against the bounds of the region,
	// as this is an internal function, it is intended for the caller to properly validate.
	//
	if( ( Region->Offset > UINT16_MAX )
		|| ( Offset > ( UINT16_MAX - Region->Offset ) )
		|| ( ( AccessBitWidth / CHAR_BIT ) > ResultDataSize ) )
	{
		return AML_FALSE;
	}

	//
	// Calculate actual absolute port index, the input offset is relative to the region's offset.
	//
	PortIndex = ( UINT16 )( Region->Offset + Offset );

	//
	// Perform the actual I/O read.
	//
	switch( AccessBitWidth ) {
	case 8:
		*( UINT8* )ResultData = AmlHostIoPortRead8( State->Host, PortIndex );
		break;
	case 16:
		*( UINT16* )ResultData = AmlHostIoPortRead16( State->Host, PortIndex );
		break;
	case 32:
		*( UINT32* )ResultData = AmlHostIoPortRead32( State->Host, PortIndex );
		break;
	case 64:
		*( UINT64* )ResultData = AmlHostIoPortRead32( State->Host, PortIndex );
		AML_DEBUG_ERROR( State, "Error: Invalid I/O space access size 64 truncated to 32.\n" );
		break;
	default:
		return AML_FALSE;
	}

	return AML_TRUE;
}

//
// Write to an I/O space operation region at byte granularity.
//
_Success_( return )
static
BOOLEAN
AmlOperationRegionWriteSystemIo(
	_In_                         const struct _AML_STATE*           State,
	_In_                         const AML_OBJECT_OPERATION_REGION* Region,
	_In_                         UINT64                             Offset,
	_In_                         UINT64                             AccessBitWidth,
	_In_reads_bytes_( DataSize ) VOID*                              Data,
	_In_                         SIZE_T                             DataSize
	)
{
	UINT16 PortIndex;

	//
	// Validate input parameters.
	// Parameters are not checked against the bounds of the region,
	// as this is an internal function, it is intended for the caller to properly validate.
	//
	if( ( Region->Offset > UINT16_MAX )
		|| ( Offset > ( UINT16_MAX - Region->Offset ) )
		|| ( ( AccessBitWidth / CHAR_BIT ) > DataSize ) )
	{
		return AML_FALSE;
	}

	//
	// Calculate actual absolute port index, the input offset is relative to the region's offset.
	//
	PortIndex = ( UINT16 )( Region->Offset + Offset );

	//
	// Perform the actual I/O write.
	//
	switch( AccessBitWidth ) {
	case 8:
		AmlHostIoPortWrite8( State->Host, PortIndex, *( UINT8* )Data );
		break;
	case 16:
		AmlHostIoPortWrite16( State->Host, PortIndex, *( UINT16* )Data );
		break;
	case 32:
		AmlHostIoPortWrite32( State->Host, PortIndex, *( UINT32* )Data );
		break;
	case 64:
		AmlHostIoPortWrite32( State->Host, PortIndex, ( UINT32 )( *( UINT64* )Data ) );
		AML_DEBUG_ERROR( State, "Error: Invalid I/O space access size 64 truncated to 32.\n" );
		break;
	default:
		return AML_FALSE;
	}

	return AML_TRUE;
}

//
// Default region-space handler for system IO space.
//
_Success_( return )
BOOLEAN
AmlOperationRegionHandlerDefaultSystemIo(
	_Inout_     struct _AML_STATE*           State,
	_Inout_     AML_OBJECT_OPERATION_REGION* Region,
	_Inout_     VOID*                        UserContext,
	_Inout_opt_ AML_OBJECT_FIELD*            Field,
	_In_        AML_REGION_ACCESS_TYPE       AccessType,
	_In_        UINT8                        AccessAttribute,
	_In_        UINT64                       AccessOffset,
	_In_        UINT64                       AccessBitWidth,
	_Inout_     AML_REGION_ACCESS_DATA*      Data
	)
{
	switch( AccessType ) {
	case AML_REGION_ACCESS_TYPE_READ:
		return AmlOperationRegionReadSystemIo( State, Region, AccessOffset, AccessBitWidth, &Data->Word, sizeof( Data->Word ) );
	case AML_REGION_ACCESS_TYPE_WRITE:
		return AmlOperationRegionWriteSystemIo( State, Region, AccessOffset, AccessBitWidth, &Data->Word, sizeof( Data->Word ) );
	}
	return AML_FALSE;
}

//
// Read from an MMIO space operation region at byte granularity.
//
_Success_( return )
static
BOOLEAN
AmlOperationRegionReadSystemMemory(
	_In_                                 const struct _AML_STATE*           State,
	_In_                                 const AML_OBJECT_OPERATION_REGION* Region,
	_In_                                 UINT64                             Offset,
	_In_                                 UINT64                             AccessBitWidth,
	_Out_writes_bytes_( ResultDataSize ) VOID*                              ResultData,
	_In_                                 SIZE_T                             ResultDataSize
	)
{
	UINT64 MmioAddress;

	//
	// Validate input parameters.
	// Parameters are not checked against the bounds of the region,
	// as this is an internal function, it is intended for the caller to properly validate.
	// TODO: Validate region.
	//
	if( ( ( AccessBitWidth / CHAR_BIT ) > ResultDataSize ) ) {
		return AML_FALSE;
	}

	//
	// SystemMemory always requires the physical address to be mapped to virtual address space.
	//
	if( Region->IsMapped == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Calculate absolute MMIO address, the input offset is relative to the region start offset.
	//
	MmioAddress = ( ( UINT64 )Region->MappedBase + Offset );

	//
	// Perform the actual I/O read.
	//
	switch( AccessBitWidth ) {
	case 8:
		*( UINT8* )ResultData = AmlHostMmioRead8( State->Host, MmioAddress );
		break;
	case 16:
		*( UINT16* )ResultData = AmlHostMmioRead16( State->Host, MmioAddress );
		break;
	case 32:
		*( UINT32* )ResultData = AmlHostMmioRead32( State->Host, MmioAddress );
		break;
	case 64:
		*( UINT64* )ResultData = AmlHostMmioRead64( State->Host, MmioAddress );
		break;
	default:
		return AML_FALSE;
	}

	return AML_TRUE;
}

//
// Write to an MMIO space operation region at byte granularity.
//
_Success_( return )
static
BOOLEAN
AmlOperationRegionWriteSystemMemory(
	_In_                         const struct _AML_STATE*           State,
	_In_                         const AML_OBJECT_OPERATION_REGION* Region,
	_In_                         UINT64                             Offset,
	_In_                         UINT64                             AccessBitWidth,
	_In_reads_bytes_( DataSize ) VOID*                              Data,
	_In_                         SIZE_T                             DataSize
	)
{
	UINT64 MmioAddress;

	//
	// Validate input parameters.
	// Parameters are not checked against the bounds of the region,
	// as this is an internal function, it is intended for the caller to properly validate.
	//
	if( ( ( AccessBitWidth / CHAR_BIT ) > DataSize ) ) {
		return AML_FALSE;
	}

	//
	// SystemMemory always requires the physical address to be mapped to virtual address space.
	//
	if( Region->IsMapped == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Calculate absolute MMIO address, the input offset is relative to the region start offset.
	//
	MmioAddress = ( ( UINT64 )Region->MappedBase + Offset );

	//
	// Perform the actual I/O read.
	//
	switch( AccessBitWidth ) {
	case 8:
		AmlHostMmioWrite8( State->Host, MmioAddress, *( UINT8* )Data );
		break;
	case 16:
		AmlHostMmioWrite16( State->Host, MmioAddress, *( UINT16* )Data );
		break;
	case 32:
		AmlHostMmioWrite32( State->Host, MmioAddress, *( UINT32* )Data );
		break;
	case 64:
		AmlHostMmioWrite64( State->Host, MmioAddress, *( UINT64* )Data );
		break;
	default:
		return AML_FALSE;
	}

	return AML_TRUE;
}

//
// Default region-space handler for system memory space.
//
_Success_( return )
BOOLEAN
AmlOperationRegionHandlerDefaultSystemMemory(
	_Inout_     struct _AML_STATE*           State,
	_Inout_     AML_OBJECT_OPERATION_REGION* Region,
	_Inout_     VOID*                        UserContext,
	_Inout_opt_ AML_OBJECT_FIELD*            Field,
	_In_        AML_REGION_ACCESS_TYPE       AccessType,
	_In_        UINT8                        AccessAttribute,
	_In_        UINT64                       AccessOffset,
	_In_        UINT64                       AccessBitWidth,
	_Inout_     AML_REGION_ACCESS_DATA*      Data
	)
{
	switch( AccessType ) {
	case AML_REGION_ACCESS_TYPE_READ:
		return AmlOperationRegionReadSystemMemory( State, Region, AccessOffset, AccessBitWidth, &Data->Word, sizeof( Data->Word ) );
	case AML_REGION_ACCESS_TYPE_WRITE:
		return AmlOperationRegionWriteSystemMemory( State, Region, AccessOffset, AccessBitWidth, &Data->Word, sizeof( Data->Word ) );
	}
	return AML_FALSE;
}

//
// Read from a PCI configuration space operation region at byte granularity.
//
_Success_( return )
static
BOOLEAN
AmlOperationRegionReadPciConfig(
	_In_                                 const struct _AML_STATE*     State,
	_Inout_                              AML_OBJECT_OPERATION_REGION* Region,
	_In_                                 UINT64                       Offset,
	_In_                                 UINT64                       AccessBitWidth,
	_Out_writes_bytes_( ResultDataSize ) VOID*                        ResultData,
	_In_                                 SIZE_T                       ResultDataSize
	)
{
	AML_PCI_SBDF_ADDRESS Address;

	//
	// Validate input parameters.
	// Parameters are not checked against the bounds of the region,
	// as this is an internal function, it is intended for the caller to properly validate.
	//
	if( ( ( AccessBitWidth / CHAR_BIT ) > ResultDataSize ) ) {
		return AML_FALSE;
	}

	//
	// We must have evaluated PCI information for the region's device.
	//
	if( Region->IsPciValid == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: No valid parsed PCI device information for operation region!\n" );
		return AML_FALSE;
	}

	//
	// Resolve the actual current PCI address (S/B/D/F) of the region's device.
	// Takes into account any PCI bridges that the device is linked over.
	//
	Address = Region->PciInfo.Address;
	if( AmlPciResolveDeviceAddress( State->Host, &Region->PciInfo, &Address ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Perform the actual PCI configuration space write to the final resolved PCI address.
	//
	switch( AccessBitWidth ) {
	case 8:
		*( UINT8* )ResultData = AmlHostPciConfigRead8( State->Host, Address, ( Region->Offset + Offset ) );
		break;
	case 16:
		*( UINT16* )ResultData = AmlHostPciConfigRead16( State->Host, Address, ( Region->Offset + Offset ) );
		break;
	case 32:
		*( UINT32* )ResultData = AmlHostPciConfigRead32( State->Host, Address, ( Region->Offset + Offset ) );
		break;
	case 64:
		*( UINT64* )ResultData = AmlHostPciConfigRead64( State->Host, Address, ( Region->Offset + Offset ) );
		break;
	default:
		return AML_FALSE;
	}

	return AML_TRUE;
}

//
// Write to a PCI configuration space operation region at byte granularity.
//
_Success_( return )
static
BOOLEAN
AmlOperationRegionWritePciConfig(
	_In_                         const struct _AML_STATE*     State,
	_Inout_                      AML_OBJECT_OPERATION_REGION* Region,
	_In_                         UINT64                       Offset,
	_In_                         UINT64                       AccessBitWidth,
	_In_reads_bytes_( DataSize ) VOID*                        Data,
	_In_                         SIZE_T                       DataSize
	)
{
	AML_PCI_SBDF_ADDRESS Address;

	//
	// Validate input parameters.
	// Parameters are not checked against the bounds of the region,
	// as this is an internal function, it is intended for the caller to properly validate.
	//
	if( ( ( AccessBitWidth / CHAR_BIT ) > DataSize ) ) {
		return AML_FALSE;
	}

	//
	// We must have evaluated PCI information for the region's device.
	//
	if( Region->IsPciValid == AML_FALSE ) {
		AML_DEBUG_ERROR( State, "Error: No valid parsed PCI device information for operation region!\n" );
		return AML_FALSE;
	}

	//
	// Resolve the actual current PCI address (S/B/D/F) of the region's device.
	// Takes into account any PCI bridges that the device is linked over.
	//
	Address = Region->PciInfo.Address;
	if( AmlPciResolveDeviceAddress( State->Host, &Region->PciInfo, &Address ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Perform the actual PCI configuration space write to the final resolved PCI address.
	//
	switch( AccessBitWidth ) {
	case 8:
		AmlHostPciConfigWrite8( State->Host, Address, ( Region->Offset + Offset ), *( UINT8* )Data );
		break;
	case 16:
		AmlHostPciConfigWrite16( State->Host, Address, ( Region->Offset + Offset ), *( UINT16* )Data );
		break;
	case 32:
		AmlHostPciConfigWrite32( State->Host, Address, ( Region->Offset + Offset ), *( UINT32* )Data );
		break;
	case 64:
		AmlHostPciConfigWrite64( State->Host, Address, ( Region->Offset + Offset ), *( UINT64* )Data );
		break;
	default:
		return AML_FALSE;
	}

	return AML_TRUE;
}

//
// Default region-space handler for PCI configuration space.
//
_Success_( return )
BOOLEAN
AmlOperationRegionHandlerDefaultPciConfig(
	_Inout_     struct _AML_STATE*           State,
	_Inout_     AML_OBJECT_OPERATION_REGION* Region,
	_Inout_     VOID*                        UserContext,
	_Inout_opt_ AML_OBJECT_FIELD*            Field,
	_In_        AML_REGION_ACCESS_TYPE       AccessType,
	_In_        UINT8                        AccessAttribute,
	_In_        UINT64                       AccessOffset,
	_In_        UINT64                       AccessBitWidth,
	_Inout_     AML_REGION_ACCESS_DATA*      Data
	)
{
	switch( AccessType ) {
	case AML_REGION_ACCESS_TYPE_READ:
		return AmlOperationRegionReadPciConfig( State, Region, AccessOffset, AccessBitWidth, &Data->Word, sizeof( Data->Word ) );
	case AML_REGION_ACCESS_TYPE_WRITE:
		return AmlOperationRegionWritePciConfig( State, Region, AccessOffset, AccessBitWidth, &Data->Word, sizeof( Data->Word ) );
	}
	return AML_FALSE;
}

//
// Default region-space handler for all optional/unsupported region space types, does nothing.
//
_Success_( return )
BOOLEAN
AmlOperationRegionHandlerDefaultNull(
	_Inout_     struct _AML_STATE*           State,
	_Inout_     AML_OBJECT_OPERATION_REGION* Region,
	_Inout_     VOID*                        UserContext,
	_Inout_opt_ AML_OBJECT_FIELD*            Field,
	_In_        AML_REGION_ACCESS_TYPE       AccessType,
	_In_        UINT8                        AccessAttribute,
	_In_        UINT64                       AccessOffset,
	_In_        UINT64                       AccessBitWidth,
	_Inout_     AML_REGION_ACCESS_DATA*      Data
	)
{
	AML_DEBUG_WARNING(
		State,
		"Warning: Null operation region handler called for space type 0x%x, ignoring (type=%s).\n",
		Region->SpaceType,
		( ( AccessType == AML_REGION_ACCESS_TYPE_READ ) ? "read" : "write" )
	);

	return AML_TRUE;
}
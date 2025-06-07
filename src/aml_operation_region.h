#pragma once

#include "aml_platform.h"
#include "aml_object.h"

//
// AML operation-region access type, used by the host to distinguish
// what operation to actually perform inside of an access handler.
//
typedef enum _AML_REGION_ACCESS_TYPE {
    AML_REGION_ACCESS_TYPE_READ,
    AML_REGION_ACCESS_TYPE_WRITE,
} AML_REGION_ACCESS_TYPE;

//
// Special SMBUS data transfer buffer.
//
#pragma pack(push, 1)
typedef struct _AML_REGION_ACCESS_DATA_SMBUS {
    UINT8 Status;
    UINT8 Length;
    UINT8 Data[ 32 ];
} AML_REGION_ACCESS_DATA_SMBUS;
#pragma pack(pop)

//
// Special IPMI data transfer buffer.
//
#pragma pack(push, 1)
typedef struct _AML_REGION_ACCESS_DATA_IPMI {
    UINT8 Status;
    UINT8 Length;
    UINT8 Data[ 64 ];
} AML_REGION_ACCESS_DATA_IPMI;
#pragma pack(pop)

//
// Special GSB data transfer buffer.
//
#pragma pack(push, 1)
typedef struct _AML_REGION_ACCESS_DATA_GSB {
    UINT8 Status;
    UINT8 Length;
    UINT8 Data[ 128 ];
} AML_REGION_ACCESS_DATA_GSB;
#pragma pack(pop)

//
// AML region access data passed to a region access handler.
// For BufferAcc types, their corresponding union fields are used.
// For all other region space types, the regular Word of data is used.
//
#pragma pack(push, 1)
typedef union _AML_REGION_ACCESS_DATA {
    AML_REGION_ACCESS_DATA_SMBUS SmBus;
    AML_REGION_ACCESS_DATA_IPMI  Ipmi;
    AML_REGION_ACCESS_DATA_GSB   Gsb;
    UINT64                       Word;
} AML_REGION_ACCESS_DATA;
#pragma pack(pop)

//
// AML operation-region access handler callback.
// Used by the host to implement access to a certain type of region space.
//
typedef
_Success_( return )
BOOLEAN
( *AML_REGION_ACCESS_ROUTINE )(
    _Inout_     struct _AML_STATE*                   State,
    _Inout_     AML_OBJECT_OPERATION_REGION*         Region,
    _Inout_     VOID*                                UserContext,
    _Inout_opt_ AML_OBJECT_FIELD*                    Field,
    _In_        AML_REGION_ACCESS_TYPE               AccessType,
    _In_        UINT8                                AccessAttribute,
    _In_        UINT64                               AccessOffset,
    _In_        UINT64                               AccessBitWidth,
    _Inout_     AML_REGION_ACCESS_DATA*              Data
    );

//
// Registered information about a region access handler.
//
typedef struct _AML_REGION_ACCESS_REGISTRATION {
    AML_REGION_ACCESS_ROUTINE UserRoutine;
    VOID*                     UserContext;
    BOOLEAN                   BroadcastPending;
    BOOLEAN                   EnableState;
} AML_REGION_ACCESS_REGISTRATION;

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
    );

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
    );

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
    );

//
// Default region-space handler for system memory space.
//
_Success_( return )
BOOLEAN
AmlOperationRegionHandlerDefaultSystemMemory(
    _Inout_     struct _AML_STATE*                   State,
    _Inout_     AML_OBJECT_OPERATION_REGION*         Region,
    _Inout_     VOID*                                UserContext,
    _Inout_opt_ AML_OBJECT_FIELD*                    Field,
    _In_        AML_REGION_ACCESS_TYPE               AccessType,
    _In_        UINT8                                AccessAttribute,
    _In_        UINT64                               AccessOffset,
    _In_        UINT64                               AccessBitWidth,
    _Inout_     AML_REGION_ACCESS_DATA*              Data
    );

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
    );

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
    );
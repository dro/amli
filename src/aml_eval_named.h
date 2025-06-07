#pragma once

#include "aml_data.h"
#include "aml_decoder.h"

//
// Evaluate a DefDevice instruction.
// DefDevice := DeviceOp PkgLength NameString TermList
//
_Success_( return )
BOOLEAN
AmlEvalDevice(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    );

//
// Evaluate a DefMethod instruction.
// DefDevice := DeviceOp PkgLength NameString TermList
//
_Success_( return )
BOOLEAN
AmlEvalMethod(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    );

//
// DefExternal := ExternalOp NameString ObjectType ArgumentCount
//
_Success_( return )
BOOLEAN
AmlEvalExternal(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    );

//
// DefProcessor := ProcessorOp PkgLength NameString ProcID PblkAddr PblkLen TermList
//
_Success_( return )
BOOLEAN
AmlEvalProcessor(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    );

//
// DefPowerRes := PowerResOp PkgLength NameString SystemLevel ResourceOrder TermList
//
_Success_( return )
BOOLEAN
AmlEvalPowerRes(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    );

//
// DefThermalZone := ThermalZoneOp PkgLength NameString TermList
//
_Success_( return )
BOOLEAN
AmlEvalThermalZone(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    );

//
// Evalute named object opcodes.
// NamedObj := DefBankField | DefCreateBitField | DefCreateByteField | DefCreateWordField
// | DefCreateDWordField | DefCreateQWordField | DefCreateField | DefDataRegion | DefExternal
// | DefOpRegion | DefPowerRes | DefThermalZone | DefDevice | DefEvent | DefField | DefIndexField
// | DefMethod | DefMutex | DefProcessor (deprecated)
//
_Success_( return )
BOOLEAN
AmlEvalNamedObjectOpcode(
    _Inout_ AML_STATE* State
    );
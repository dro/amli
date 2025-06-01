#pragma once

#include "aml_decoder.h"
#include "aml_eval.h"
#include "aml_host.h"
#include "aml_debug.h"

//
// Attempt to evaluate a name instruction.
//
_Success_( return )
BOOLEAN
AmlEvalName(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode
	);

//
// Attempt to evaluate a scope instruction.
//
_Success_( return )
BOOLEAN
AmlEvalScope(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode
	);

//
// Attempt to evaluate an alias instruction.
//
_Success_( return )
BOOLEAN
AmlEvalAlias(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode
	);

//
// Attempt to evaluate a namespace modifier object opcode.
// NameSpaceModifierObj := DefAlias | DefName | DefScope
//
_Success_( return )
BOOLEAN
AmlEvalNamespaceModifierObjectOpcode(
	_Inout_ AML_STATE* State
	);
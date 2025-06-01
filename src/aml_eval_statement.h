#pragma once

#include "aml_decoder.h"

//
// Evaluate a DefIfElse instruction.
// Predicate := TermArg => Integer
// DefElse := Nothing | <elseop pkglength termlist>
// DefIfElse := IfOp PkgLength Predicate TermList DefElse
//
_Success_( return )
BOOLEAN
AmlEvalIfElse(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode
	);

//
// Predicate := TermArg => Integer
// DefWhile := WhileOp PkgLength Predicate TermList
//
_Success_( return )
BOOLEAN
AmlEvalWhile(
	_Inout_ AML_STATE* State,
	_In_    BOOLEAN    ConsumeOpcode
	);

//
// Evaluate a StatementOpcode.
// StatementOpcode := DefBreak | DefBreakPoint | DefContinue | DefFatal | DefIfElse
// 	| DefNoop | DefNotify | DefRelease | DefReset | DefReturn | DefSignal | DefSleep | DefStall | DefWhile
//
_Success_( return )
BOOLEAN
AmlEvalStatementOpcode(
	_Inout_ AML_STATE* State
	);
#pragma once

#include "aml_decoder.h"

//
// Controls the maximum amount of iterations of a while loop before forcefully terminating it.
//
#ifndef AML_BUILD_MAX_LOOP_ITERATIONS
 #ifdef AML_BUILD_FUZZER
  #define AML_BUILD_MAX_LOOP_ITERATIONS 100
 #else 
  #define AML_BUILD_MAX_LOOP_ITERATIONS 1000000
 #endif
#endif

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
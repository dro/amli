#pragma once

#include "aml_decoder.h"

//
// Evaluate an ExpressionOpcode.
// Result data value must be freed by the caller.
//
_Success_( return )
BOOLEAN
AmlEvalExpressionOpcode(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    EvalMethodInvocation,
    _Out_   AML_DATA*  EvalResult
    );
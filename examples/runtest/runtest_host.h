#include "aml_platform.h"

//
// User-provided host context type.
//
struct _AML_HOST_CONTEXT {
    volatile LONG* GlobalLock;
};
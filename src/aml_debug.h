#pragma once

#include "aml_platform.h"
#include "aml_data.h"

//
// Predefined internal debug log levels.
//
#define AML_DEBUG_LEVEL_TRACE	0
#define AML_DEBUG_LEVEL_INFO	1
#define AML_DEBUG_LEVEL_WARNING	2
#define AML_DEBUG_LEVEL_ERROR	3
#define AML_DEBUG_LEVEL_FATAL	4

//
// If no log level build option was set, default to only showing warning levels and higher.
//
#ifndef AML_BUILD_DEBUG_LEVEL
 #define AML_BUILD_DEBUG_LEVEL AML_DEBUG_LEVEL_TRACE
#endif

//
// Log level print helpers that can be removed at compile-time based on build log level.
//
#if AML_BUILD_DEBUG_LEVEL <= AML_DEBUG_LEVEL_TRACE
 #define AML_DEBUG_TRACE(State, ...)   AmlDebugPrint((State), AML_DEBUG_LEVEL_TRACE,   __VA_ARGS__)
#else
 #define AML_DEBUG_TRACE(State, ...)   ((VOID)0)
#endif
#if AML_BUILD_DEBUG_LEVEL <= AML_DEBUG_LEVEL_INFO
 #define AML_DEBUG_INFO(State, ...)    AmlDebugPrint((State), AML_DEBUG_LEVEL_INFO,    __VA_ARGS__)
#else
 #define AML_DEBUG_INFO(State, ...)    ((VOID)0)
#endif
#if AML_BUILD_DEBUG_LEVEL <= AML_DEBUG_LEVEL_WARNING
 #define AML_DEBUG_WARNING(State, ...) AmlDebugPrint((State), AML_DEBUG_LEVEL_WARNING, __VA_ARGS__)
#else
 #define AML_DEBUG_WARNING(State, ...) ((VOID)0)
#endif
#if AML_BUILD_DEBUG_LEVEL <= AML_DEBUG_LEVEL_ERROR
 #define AML_DEBUG_ERROR(State, ...)   AmlDebugPrint((State), AML_DEBUG_LEVEL_ERROR,   __VA_ARGS__)
#else
 #define AML_DEBUG_ERROR(State, ...)   ((VOID)0)
#endif
#if AML_BUILD_DEBUG_LEVEL <= AML_DEBUG_LEVEL_FATAL
 #define AML_DEBUG_FATAL(State, ...)   AmlDebugPrint((State), AML_DEBUG_LEVEL_FATAL,   __VA_ARGS__)
#else
 #define AML_DEBUG_FATAL(State, ...)   ((VOID)0)
#endif

//
// Debug panic (fatal print + trap).
//
#ifndef AML_DEBUG_PANIC
 #define AML_DEBUG_PANIC(State, ...) ( AML_DEBUG_FATAL((State), __VA_ARGS__), AML_TRAP() )
#endif

//
// Debug print to host at a certain log level.
//
VOID
AmlDebugPrint(
    _In_   const struct _AML_STATE* State,
    _In_   INT                      LogLevel,
    _In_z_ const CHAR*              Format,
    ...
    );

//
// Debug print the value of an AML_DATA.
//
VOID
AmlDebugPrintDataValue(
    _In_ const struct _AML_STATE* State,
    _In_ INT                      LogLevel,
    _In_ const AML_DATA*          Value
    );

//
// Debug print an AML name string.
//
VOID
AmlDebugPrintNameString(
    _In_ const struct _AML_STATE*       State,
    _In_ INT                            LogLevel,
    _In_ const struct _AML_NAME_STRING* NameString
    );

//
// Debug print an AML object's name (if any known).
//
VOID
AmlDebugPrintObjectName(
    _In_ const struct _AML_STATE*  State,
    _In_ INT                       LogLevel,
    _In_ const struct _AML_OBJECT* Object
    );

//
// Debug print a binary arithmetic instruction.
//
VOID
AmlDebugPrintArithmetic(
    _In_ const struct _AML_STATE*  State,
    _In_ INT                       LogLevel,
    _In_ const CHAR*               OperatorSymbol,
    _In_ const struct _AML_OBJECT* Target,
    _In_ AML_DATA                  Operand1,
    _In_ AML_DATA                  Operand2,
    _In_ AML_DATA                  Result
    );
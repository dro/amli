#pragma once

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>

//
// Include compiler intrinsic definitions on MSVC build.
//
#if defined(_MSC_VER)
 #include <intrin.h>
#endif

//
// Include stub SAL annotation definitions if the build environment doesn't support SAL.
//
#if defined(AML_BUILD_NO_SAL) || !defined(_SAL_VERSION)
 #include "aml_nosal.h"
#endif

//
// Use __builtin_ia32_pause for AML_PAUSE if advertised through __has_builtin (GCC/clang).
//
#ifndef AML_PAUSE
 #ifdef __has_builtin
  #if __has_builtin(__builtin_ia32_pause)
   #define AML_PAUSE() __builtin_ia32_pause()
  #endif
 #endif
#endif

//
// Use _mm_pause or __yield for x86 and ARM builds respectively if this is an MSVC build.
//
#ifndef AML_PAUSE
 #ifdef _MSC_VER
  #if defined(_M_ARM) || defined(_M_ARM64)
   #define AML_PAUSE() __yield()
  #else
   #define AML_PAUSE() _mm_pause()
  #endif
 #endif
#endif

//
// If there was still no pause/yield intrinsic found, just do nothing.
//
#ifndef AML_PAUSE
 #define AML_PAUSE() ((VOID)0)
#endif

//
// Use __builtin_clzll for AML_LZCNT64 if advertised through __has_builtin (GCC/clang).
//
#ifndef AML_LZCNT64
 #ifdef __has_builtin
  #if __has_builtin(__builtin_clzll)
   #define AML_LZCNT64(Value) __builtin_clzll((Value))
  #endif
 #endif
#endif

//
// Use _lzcnt_u64 for AML_LZCNT if this is an MSVC build.
// TODO: Support ARM MSVC build.
//
#ifndef AML_LZCNT64
 #ifdef _MSC_VER
  #define AML_LZCNT64(Value) _lzcnt_u64((Value))
 #endif
#endif

//
// Use __builtin_ctzll for AML_TZCNT64 if advertised through __has_builtin (GCC/clang).
//
#ifndef AML_TZCNT64
 #ifdef __has_builtin
  #if __has_builtin(__builtin_ctzll)
   #define AML_TZCNT64(Value) __builtin_ctzll((Value))
  #endif
 #endif
#endif

//
// Use _tzcnt_u64 for AML_TZCNT if this is an MSVC build.
// TODO: Support ARM MSVC build.
//
#ifndef AML_TZCNT64
 #ifdef _MSC_VER
  #define AML_TZCNT64(Value) _tzcnt_u64((Value))
 #endif
#endif

//
// Implement AML_ROL32 using __builtin_rotateleft32 if advertised through __has_builtin (GCC/clang).
//
#ifndef AML_ROL32
 #ifdef __has_builtin
  #if __has_builtin(__builtin_rotateleft32)
   #define AML_ROL32(Value, Shift) __builtin_rotateleft32((Value), (Shift))
  #endif
 #endif
#endif

//
// Implement AML_ROL32 using _rotl if this is an MSVC build.
//
#ifndef AML_ROL32
 #ifdef _MSC_VER
  #define AML_ROL32(Value, Shift) _rotl((Value), (Shift))
 #endif
#endif

//
// No rotate left intrinsic found, use internal implementation.
//
#ifndef AML_ROL32
 #define AML_ROL32(Value, Shift) AmlRotateLeft32((Value), (Shift))
#endif

//
// Implement AML_BSWAP32 using __builtin_bswap32 if advertised through __has_builtin (GCC/clang).
//
#ifndef AML_BSWAP32
 #ifdef __has_builtin
  #if __has_builtin(__builtin_bswap32)
   #define AML_BSWAP32(Value) __builtin_bswap32((Value))
  #endif
 #endif
#endif

//
// Implement AML_BSWAP32 using _byteswap_ulong if this is an MSVC build.
//
#ifndef AML_BSWAP32
 #ifdef _MSC_VER
  #define AML_BSWAP32(Value) _byteswap_ulong((Value))
 #endif
#endif

//
// Internal debugger fail/trap helper, use __builtin_trap if advertised as available (GCC/clang).
//
#ifndef AML_TRAP
 #ifdef __has_builtin
  #if __has_builtin(__builtin_trap)
   #define AML_TRAP() __builtin_trap()
  #endif
 #endif
#endif

//
// Internal debugger fail/trap helper, uses fastfail on MSVC.
//
#ifndef AML_TRAP
 #ifdef _MSC_VER
  #define AML_TRAP() __fastfail(56)
 #endif
#endif

//
// No supported abort/trap intrinsic, fall back to doing nothing.
//
#ifndef AML_TRAP
 #define AML_TRAP() ((VOID)0)
#endif

//
// Helper to show a string in source code for the associated trap.
//
#ifndef AML_TRAP_STRING
 #define AML_TRAP_STRING(String) ((VOID)0)
#endif

//
// Internal runtime assertion helper.
//
#define AML_ASSERT(x) (((x) == 0) ? AML_TRAP() : (VOID)0)

//
// Fix missing SAL annotation for certain build configurations.
//
#ifndef _Frees_ptr_
 #define _Frees_ptr_
#endif

//
// Base integer types.
//
#ifndef AML_BUILD_OVERRIDE_INTEGER_TYPES
 typedef int8_t        INT8;
 typedef int16_t       INT16;
 typedef int32_t       INT32;
 typedef int64_t       INT64;
 typedef uint8_t       UINT8;
 typedef uint16_t      UINT16;
 typedef uint32_t      UINT32;
 typedef uint64_t      UINT64;
 typedef int           INT;
 typedef unsigned int  UINT;
 typedef size_t        SIZE_T;
 typedef uintptr_t     UINT_PTR;
 typedef char          CHAR;
 typedef unsigned char UCHAR;
 typedef UINT8         BOOLEAN;
 typedef long          LONG;
 typedef unsigned long ULONG;
 #ifndef VOID
  typedef void         VOID;
 #endif
#endif

//
// AML boolean values.
//
#define AML_TRUE  ((BOOLEAN)(1 == 1))
#define AML_FALSE ((BOOLEAN)(0 == 1))

//
// Helper macros.
// 
#define AML_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define AML_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define AML_COUNTOF(Array) (sizeof((Array)) / sizeof((Array[0])))
#define AML_CONTAINING_RECORD(Address, Type, Field) ((Type*)((CHAR*)(Address) - (UINT_PTR)(&((Type*)NULL)->Field)))

//
// Copy memory from two non-overlapping regions, mainly used for type punning.
//
#ifndef AML_MEMCPY
 #define AML_MEMCPY AmlMemoryCopy
#endif

//
// Set all bytes of a region of memory to the given byte value.
//
#ifndef AML_MEMSET
 #define AML_MEMSET AmlMemorySet
#endif

//
// Misc ACPI/AML definitions.
// TODO: Move these!
//

//
// ACPI table OEM ID.
//
#pragma pack(push, 1)
typedef struct _AML_OEM_ID {
	UINT8 Data[ 6 ];
} AML_OEM_ID;
#pragma pack(pop)

//
// ACPI table description header, used internally by AML.
//
#pragma pack(push, 1)
typedef struct _AML_DESCRIPTION_HEADER {
	UINT32     Signature;
	UINT32     Length;
	UINT8      Revision;
	UINT8      Checksum;
	AML_OEM_ID OemId;
	UINT64     OemTableId;
	UINT32     OemRevision;
	UINT32     CreatorId;
	UINT32     CreatorRevision;
} AML_DESCRIPTION_HEADER;
#pragma pack(pop)

//
// Event/mutex await status.
//
typedef enum _AML_WAIT_STATUS {
	AML_WAIT_STATUS_SUCCESS,
	AML_WAIT_STATUS_TIMEOUT,
	AML_WAIT_STATUS_ERROR,
} AML_WAIT_STATUS;

//
// AML _STA return value format.
//
#define AML_STA_FLAG_PRESENT         ((UINT32)1 << 0) /* Bit [0] - Set if the device is present. */
#define AML_STA_FLAG_ENABLED         ((UINT32)1 << 1) /* Bit [1] - Set if the device is enabled and decoding its resources. */
#define AML_STA_FLAG_UI_SHOW         ((UINT32)1 << 2) /* Bit [2] - Set if the device should be shown in the UI. */
#define AML_STA_FLAG_FUNCTIONING     ((UINT32)1 << 3) /* Bit [3] - Set if the device is functioning properly (cleared if device failed its diagnostics). */
#define AML_STA_FLAG_BATTERY_PRESENT ((UINT32)1 << 4) /* Bit [4] - Set if the battery is present. */
#define AML_STA_FLAG_RESERVED        ((((UINT32)1 << (31-5+1)) - 1) << 5) /* Bits [31:5] - Reserved (must be cleared). */

//
// Simple naive memcpy for builds that don't define their own.
//
VOID*
AmlMemoryCopy(
    _Out_writes_bytes_all_( Size ) VOID*       Destination,
    _In_reads_bytes_( Size )       const VOID* Source,
    _In_                           SIZE_T      Size
    );

//
// Simple naive memset for builds that don't define their own.
//
VOID* 
AmlMemorySet(
    _Out_writes_bytes_all_( Size ) VOID*  Destination,
    _In_                           INT    Value,
    _In_                           SIZE_T Size
    );

//
// RotateLeft32 helper for platforms where an intrinsic couldn't be found.
//
UINT32
AmlRotateLeft32(
	_In_ UINT32 Value,
	_In_ UINT32 Shift
	);
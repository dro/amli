#pragma once

//
// Stubs for the subset of SAL annotations for build environments without SAL.
//
#ifndef _SAL_VERSION
#define _Inout_
#define _Inout_opt_
#define _Inout_count_(x)
#define _Inout_bytecount_(x)
#define _In_
#define _In_count_(x)
#define _In_z_
#define _In_reads_bytes_(x)
#define _In_opt_
#define _Out_writes_bytes_(x)
#define _Out_writes_bytes_all_(x)
#define _Out_writes_bytes_all_opt_(x)
#define _Outptr_
#define _Outptr_opt_
#define _Out_
#define _Out_opt_
#define _Success_(x)
#define _Analysis_assume_(x)
#define _Frees_ptr_
#define _Post_invalid_
#define _Field_size_part_(x,y)
#endif
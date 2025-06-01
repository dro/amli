#pragma once

#include "aml_platform.h"
#include "aml_data.h"

//
// Predefined AML OSI query name strings.
//
#define AML_OSI_NAME_WINDOWS_2000                "Windows 2000"		                 /* Windows 2000				                                    */
#define AML_OSI_NAME_WINDOWS_2001                "Windows 2001"		                 /* Windows XP					                                    */
#define AML_OSI_NAME_WINDOWS_2001_SP1            "Windows 2001 SP1"	                 /* Windows XP SP1				                                    */
#define AML_OSI_NAME_WINDOWS_2001_1              "Windows 2001.1"	                 /* Windows Server 2003			                                    */
#define AML_OSI_NAME_WINDOWS_2001_SP2            "Windows 2001 SP2"	                 /* Windows XP SP2				                                    */
#define AML_OSI_NAME_WINDOWS_2001_1_SP1          "Windows 2001.1 SP1"                /* Windows Server 2003 SP1		                                    */
#define AML_OSI_NAME_WINDOWS_2006                "Windows 2006"		                 /* Windows Vista				                                    */
#define AML_OSI_NAME_WINDOWS_2006_SP1            "Windows 2006 SP1"	                 /* Windows Vista SP1			                                    */
#define AML_OSI_NAME_WINDOWS_2006_1              "Windows 2006.1"	                 /* Windows Server 2008			                                    */
#define AML_OSI_NAME_WINDOWS_2009                "Windows 2009"		                 /* Windows 7, Win Server 2008 R2                                   */
#define AML_OSI_NAME_WINDOWS_2012                "Windows 2012"		                 /* Windows 8, Win Server 2012	                                    */
#define AML_OSI_NAME_WINDOWS_2013                "Windows 2013"		                 /* Windows 8.1					                                    */
#define AML_OSI_NAME_WINDOWS_2015                "Windows 2015"		                 /* Windows 10					                                    */
#define AML_OSI_NAME_WINDOWS_2016                "Windows 2016"		                 /* Windows 10, version 1607	                                    */
#define AML_OSI_NAME_WINDOWS_2017                "Windows 2017"		                 /* Windows 10, version 1703	                                    */
#define AML_OSI_NAME_WINDOWS_2017_2              "Windows 2017.2"	                 /* Windows 10, version 1709	                                    */
#define AML_OSI_NAME_WINDOWS_2018                "Windows 2018"		                 /* Windows 10, version 1803	                                    */
#define AML_OSI_NAME_WINDOWS_2018_2              "Windows 2018.2"	                 /* Windows 10, version 1809	                                    */
#define AML_OSI_NAME_WINDOWS_2019                "Windows 2019"		                 /* Windows 10, version 1903	                                    */
#define AML_OSI_NAME_WINDOWS_2020                "Windows 2020"		                 /* Windows 10, version 2004	                                    */
#define AML_OSI_NAME_WINDOWS_2021                "Windows 2021"		                 /* Windows 11					                                    */
#define AML_OSI_NAME_WINDOWS_2022                "Windows 2022"		                 /* Windows 11, version 22H2	                                    */
#define AML_OSI_NAME_MODULE_DEVICE               "Module Device"                     /* ACPI-defined module device (ACPI0004)                           */
#define AML_OSI_NAME_PROCESSOR_DEVICE            "Processor Device"                  /* ACPI-defined processor device (ACPI0007)                        */
#define AML_OSI_NAME_3_THERMAL_MODEL             "3.0 Thermal Model"                 /* ACPI-defined extensions to the thermal model in Revision 3.0    */
#define AML_OSI_NAME_ESA_DESCRIPTOR              "Extended Address Space Descriptor" /* ACPI-defined Extended Address Space Descriptor                  */
#define AML_OSI_NAME_3_SCP_EXTENSIONS            "3.0 _SCP Extensions"               /* ACPI-defined _SCP with the additional arguments in Revision 3.0 */
#define AML_OSI_NAME_PROCESSOR_AGGREGATOR_DEVICE "Processor Aggregator Device"       /* ACPI-defined processor aggregator device (ACPI000C)             */

//
// Supported OSI entry, will return successfully if queried by AML bytecode using _OSI(Name).
//
typedef struct _AML_OSI_ENTRY {
	const CHAR* Name;
	SIZE_T      NameLength;
} AML_OSI_ENTRY;

//
// Native method/upcall allowing AML code to query interfaces supported by the host operating system.
//
_Success_( return )
BOOLEAN
AmlOsiQueryNativeMethod(
	_Inout_                        struct _AML_STATE* State,
	_In_                           VOID*              UserContext,
	_Inout_count_( ArgumentCount ) AML_DATA*          Arguments,
	_In_                           SIZE_T             ArgumentCount,
	_Inout_                        AML_DATA*          ReturnValue
	);
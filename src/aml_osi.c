#include "aml_state.h"
#include "aml_osi.h"
#include "aml_debug.h"

//
// Default supported OSI values/interfaces.
//
#define AML_DEFINE_OSI_ENTRY_CONST(ConstNameString) { .Name = (ConstNameString), .NameLength = AML_COUNTOF( (ConstNameString) ) - 1 }
static const AML_OSI_ENTRY AmlOsiDefaultInterfaceList[] = {
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2000                ),  /* Windows 2000				                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2001                ),  /* Windows XP					                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2001_SP1            ),  /* Windows XP SP1				                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2001_1              ),  /* Windows Server 2003			                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2001_SP2            ),  /* Windows XP SP2				                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2001_1_SP1          ),  /* Windows Server 2003 SP1		                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2006                ),  /* Windows Vista				                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2006_SP1            ),  /* Windows Vista SP1			                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2006_1              ),  /* Windows Server 2008			                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2009                ),  /* Windows 7, Win Server 2008 R2                                   */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2012                ),  /* Windows 8, Win Server 2012	                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2013                ),  /* Windows 8.1					                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2015                ),  /* Windows 10					                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2016                ),  /* Windows 10, version 1607	                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2017                ),  /* Windows 10, version 1703	                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2017_2              ),  /* Windows 10, version 1709	                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2018                ),  /* Windows 10, version 1803	                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2018_2              ),  /* Windows 10, version 1809	                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2019                ),  /* Windows 10, version 1903	                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2020                ),  /* Windows 10, version 2004	                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2021                ),  /* Windows 11					                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_WINDOWS_2022                ),  /* Windows 11, version 22H2	                                    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_MODULE_DEVICE               ),  /* ACPI-defined module device (ACPI0004)                           */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_PROCESSOR_DEVICE            ),  /* ACPI-defined processor device (ACPI0007)                        */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_3_THERMAL_MODEL             ),  /* ACPI-defined extensions to the thermal model in Revision 3.0    */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_ESA_DESCRIPTOR              ),  /* ACPI-defined Extended Address Space Descriptor                  */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_3_SCP_EXTENSIONS            ),  /* ACPI-defined _SCP with the additional arguments in Revision 3.0 */
    AML_DEFINE_OSI_ENTRY_CONST( AML_OSI_NAME_PROCESSOR_AGGREGATOR_DEVICE ),  /* ACPI-defined processor aggregator device (ACPI000C)             */
};

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
    )
{
    SIZE_T               i;
    SIZE_T               j;
    BOOLEAN              Match;
    const AML_OSI_ENTRY* Entry;
    AML_BUFFER_DATA*     InputString;

    //
    // Validate input parameters, return unsupported if invalid arguments.
    //
    if( ( ArgumentCount < 1 )
        || ( Arguments[ 0 ].Type != AML_DATA_TYPE_STRING )
        || ( Arguments[ 0 ].u.String == NULL ) )
    {
        *ReturnValue = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = 0 };
        return AML_TRUE;
    }
    InputString = Arguments[ 0 ].u.String;

    //
    // Search the default OSI list for a matching interface.
    //
    Match = AML_FALSE;
    for( i = 0; ( ( i < AML_COUNTOF( AmlOsiDefaultInterfaceList ) ) && ( Match == AML_FALSE ) ); i++ ) {
        Entry = &AmlOsiDefaultInterfaceList[ i ];
        if( ( Entry->NameLength == InputString->Size ) && ( Entry->NameLength > 0 ) ) {
            for( j = 0; j < Entry->NameLength; j++ ) {
                if( ( Match = ( Entry->Name[ j ] == InputString->Data[ j ] ) ) == AML_FALSE ) {
                    break;
                }
            }
        }
    }

    //
    // Print information about the query and if the interface is supported.
    //
    AML_DEBUG_TRACE(
        State,
        "OSI query: %.*s (supported: %i)\n",
        ( UINT )AML_MIN( UINT_MAX, Arguments[ 0 ].u.String->Size ),
        Arguments[ 0 ].u.String->Data,
        ( INT )Match
    );

    //
    // Return True/False (Ones/Zero) based on the supported status of hte interface.
    //
    *ReturnValue = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = ( Match ? ~0ull : 0 ) };
    return AML_TRUE;
}

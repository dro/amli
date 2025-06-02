#include <stdio.h>
#include <stdlib.h>
#include "runtest_host.h"
#include "aml_platform.h"
#include "aml_eval.h"
#include "aml_base.h"
#include "aml_debug.h"

//
// User-provided allocator interface callback to allocate memory.
//
_Success_( return != NULL )
static
VOID*
AmlTestMemoryAllocate(
	_Inout_ VOID*  Context,
	_In_    SIZE_T Size
	)
{
	return malloc( Size );
}

//
// User-provided allocator interface callback to free memory previously allocated using AML_MEMORY_ALLOCATE.
//
_Success_( return )
static
BOOLEAN
AmlTestMemoryFree(
	_Inout_          VOID*  Context,
	_In_ _Frees_ptr_ VOID*  Allocation,
	_In_             SIZE_T AllocationSize
	)
{
	free( Allocation );
	return AML_TRUE;
}

//
// Recursively debug print a namespace tree node and all of its children.
//
VOID
AmlTestrintNamespaceTreeNode(
	_In_     const AML_STATE*         State,
	_In_opt_ AML_NAMESPACE_TREE_NODE* TreeNode,
	_In_     UINT64                   LevelIndex
	)
{
	AML_NAMESPACE_TREE_NODE* TreeChild;
	AML_NAMESPACE_NODE*      NsNode;
	UINT64                   i;

	//
	// Stop once we have reached the end of this path.
	//
	if( TreeNode == NULL ) {
		return;
	}

	//
	// Print information about the current visited node.
	//
	if( TreeNode != &State->Namespace.TreeRoot ) {
		NsNode = AML_CONTAINING_RECORD( TreeNode, AML_NAMESPACE_NODE, TreeEntry );
		if( LevelIndex > 0 ) {
			for( i = 0; i < ( LevelIndex - 1 ); i++ ) {
				printf( "  " );
			}
		}
		printf( " \\ %.4s %s\n", NsNode->LocalName.Data, AmlObjectToAcpiTypeName( NsNode->Object ) );
	}

	//
	// Process all children.
	//
	for( TreeChild = TreeNode->ChildFirst; TreeChild != NULL; TreeChild = TreeChild->Next ) {
		AmlTestrintNamespaceTreeNode( State, TreeChild, ( LevelIndex + 1 ) );
	}
}

//
// Attempt to load and evaluate a table from the given file path.
//
_Success_( return == EXIT_SUCCESS )
static
INT
AmlTestMain(
	_In_z_ const CHAR* FileName
	)
{
	AML_DESCRIPTION_HEADER TableHeader;
	FILE*                  TableFile;
	SIZE_T                 ReadSize;
	VOID*                  TableData;
	UINT32                 TableDataSize;
	BOOLEAN                Success;
	AML_ALLOCATOR          Allocator;
	AML_STATE              State;
	volatile LONG          AcpiGlobalLock;
	AML_HOST_CONTEXT       Host;
	AML_NAMESPACE_NODE*    TsfiNode;
	const AML_DATA*        TsfiValue;

	//
	// Attempt to open the given ACPI table file to execute (should be a DSDT or SSDT).
	//
#ifdef _MSC_VER
	if( fopen_s( &TableFile, FileName, "rb" ) != 0 ) {
#else
	if( ( TableFile = fopen( FileName, "rb" ) ) == NULL ) {
#endif
		printf( "Error: Failed to open input file.\n" );
		return EXIT_FAILURE;
	}

	//
	// Attempt to validate and load the full ACPI table.
	//
	TableData = NULL;
	do {
		//
		// Read and validate the ACPI table header.
		//
		Success = AML_FALSE;
		ReadSize = fread( &TableHeader, 1, sizeof( TableHeader ), TableFile );
		if( ReadSize != sizeof( TableHeader ) ) {
			printf( "Error: Failed to read input table header.\n" );
			break;
		} else if( ( TableHeader.Signature != 0x54445344 ) && ( TableHeader.Signature != 0x54445353 ) ) {
			printf( "Error: Invalid input table signature (must be DSDT or SSDT).\n" );
			break;
		} else if( TableHeader.Length <= sizeof( TableHeader ) ) {
			printf( "Error: Invalid input table size.\n" );
			break;
		}

		//
		// Attempt to read the rest of the data attached to the table.
		//
		TableDataSize = ( TableHeader.Length - sizeof( TableHeader ) );
		TableData = malloc( TableDataSize );
		if( TableData == NULL ) {
			printf( "Error: Failed to allocate memory for full input table data.\n" );
			break;
		} else if( fread( TableData, 1, TableDataSize, TableFile ) != TableDataSize ) {
			printf( "Error: Failed to read input table data.\n" );
			break;
		}
		Success = AML_TRUE;
	} while( 0 );
	fclose( TableFile );

	//
	// Handle failure to load the input table.
	//
	if( Success == AML_FALSE ) {
		return EXIT_FAILURE;
	}

	//
	// Set up decoder and host interface.
	//
	AcpiGlobalLock = 0;
	Host = ( AML_HOST_CONTEXT ){ .GlobalLock = &AcpiGlobalLock };
	Allocator = ( AML_ALLOCATOR ){ .Allocate = AmlTestMemoryAllocate, .Free = AmlTestMemoryFree };
	if( AmlStateCreate(
		&State,
		Allocator,
		&( AML_STATE_PARAMETERS ){
			.Host            = &Host,
			.Use64BitInteger = ( TableHeader.Revision > 1 ),
		} ) == AML_FALSE )
	{
		printf( "Error: AmlStateCreate failed!\n" );
		return EXIT_FAILURE;
	}

	//
	// Create all predefined ACPI root namespaces and objects.
	//
	if( AmlCreatePredefinedNamespaces( &State ) == AML_FALSE ) {
		printf( "Error: AmlDecoderCreatePredefinedNamespaces failed!\n" );
		return EXIT_FAILURE;
	}
	AmlCreatePredefinedObjects( &State );

	//
	// Attempt to execute the input code.
	//
	_Analysis_assume_( TableData != NULL );
	if( AmlEvalLoadedTableCode( &State, TableData, TableDataSize, NULL ) == AML_FALSE ) {
		printf( "Error: AmlEvalLoadedTableCode failed!\n" );
		return EXIT_FAILURE;
	}

	//
	// Mark the initial table loads as complete, broadcast any pending _REG notifications,
	// and attempt to call the _INI initializers for all devices with an applicable _STA value.
	//
	AmlCompleteInitialLoad( &State, AML_TRUE );

	//
	// Dump the entire created namespace in hierarchical tree format.
	//
	AmlTestrintNamespaceTreeNode( &State, &State.Namespace.TreeRoot, 0 );

	//
	// Test broadcasting _REG.
	//
	AmlRegisterRegionSpaceAccessHandler( &State, AML_REGION_SPACE_TYPE_SMBUS, AmlOperationRegionHandlerDefaultNull, NULL, AML_TRUE );

	//
	// Check for any test case failures (TSFI object will contain the index of the last test case failure).
	//
	if( AmlNamespaceSearchZ( &State.Namespace, NULL, "TSFI", 0, &TsfiNode ) ) {
		if( TsfiNode->Object->Type == AML_OBJECT_TYPE_NAME ) {
			TsfiValue = &TsfiNode->Object->u.Name.Value;
			if( ( TsfiValue->Type != AML_DATA_TYPE_INTEGER ) || ( TsfiValue->u.Integer != 0 ) ) {
				AML_DEBUG_ERROR( &State, "Error: One or more test cases failed! Last failed TSFI value: " );
				if( TsfiValue->Type == AML_DATA_TYPE_INTEGER ) {
					AML_DEBUG_ERROR( &State, "%"PRId64"", TsfiValue->u.Integer );
				} else {
					AmlDebugPrintDataValue( &State, AML_DEBUG_LEVEL_ERROR, TsfiValue );
				}
				AML_DEBUG_ERROR( &State, "\n" );
				return EXIT_FAILURE;
			}
		}
	}

	printf( "\n\nAll test cases completed successfully.\n" );
	return EXIT_SUCCESS;
}

_Success_( return == EXIT_SUCCESS )
INT
main(
	_In_               INT    ArgC,
	_In_count_( ArgC ) CHAR** ArgV
	)
{
#if 0
	if( ArgC < 2 ) {
		printf(
			"Invalid arguments.\n"
			"Usage: runtest <table path>\n"
		);
		return EXIT_FAILURE;
	}
	return AmlTestMain( ArgV[ 1 ] );
#else
	return AmlTestMain( "C:\\iasl\\test2.aml" );
#endif
}
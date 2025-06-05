#include "aml_namespace.h"
#include "aml_debug.h"
#include "aml_hash.h"
#include "aml_base.h"

//
// Initialize global namespace state.
// The given allocator and heap must remain valid for the entire lifetime of the namespace state.
// TODO: Take in PermanentArena as an argument instead of a regular Allocator,
// then set up the scope arena to use the PermanentArena as a backend.
//
VOID
AmlNamespaceStateInitialize(
	_Out_   AML_NAMESPACE_STATE* State,
	_In_    AML_ALLOCATOR        Allocator,
	_Inout_ AML_HEAP*            Heap
	)
{
	//
	// Default initialize fields.
	// Memset is used here to avoid a large empty copy of AML_NAMESPACE_STATE on the stack.
	//
	AML_MEMSET( State, 0, sizeof( *State ) );
	State->Heap = Heap;
	State->TreeMaxDepth = 256;

	//
	// Initialize arenas.
	//
	AmlArenaInitialize( &State->PermanentArena, Allocator, ( 4096 * 4 ), 0 );
	AmlArenaInitialize( &State->TempArena, Allocator, 4096, 0 );
	AmlArenaInitialize( &State->ScopeArena, Allocator, 8192, 0 );

	//
	// Set up the root scope node.
	//
	State->ScopeRoot = ( AML_NAMESPACE_SCOPE ){
		.ArenaSnapshot = AmlArenaSnapshot( &State->ScopeArena ),
		.AbsolutePath = {
			.Prefix = { .Data = { '\\' }, .Length = 1 }
		},
	};
	State->ScopeFirst = &State->ScopeRoot;
	State->ScopeLast = &State->ScopeRoot;
}

//
// Release all underlying namespace state resources.
// TODO: Remove this once namespace allocates ontop of a parent arena.
//
VOID
AmlNamespaceStateRelease(
	_Inout_ _Post_invalid_ AML_NAMESPACE_STATE* State
	)
{
	//
	// Release all allocated memory.
	//
	AmlArenaRelease( &State->TempArena );
	AmlArenaRelease( &State->ScopeArena );
	AmlArenaRelease( &State->PermanentArena );

	//
	// Zero fields for debugging.
	//
	AML_MEMSET( State, 0, sizeof( *State ) );
}

//
// Convert a possibly-relative name string to an absolute path, relative to the given active namespace scope.
// Note: this does not apply any search rules, if the path is relative, it assumes that it is relative to the current scope!
//
_Success_( return )
static
BOOLEAN
AmlNamespaceConvertNameStringToAbsolutePath(
	_In_    const AML_NAMESPACE_SCOPE* ActiveScope,
	_Inout_ AML_ARENA*                 Arena,
	_In_    const AML_NAME_STRING*     Input,
	_Out_   AML_NAME_STRING*           AbsoluteOutput
	)
{
	SIZE_T        i;
	AML_NAME_SEG* OutputSegments;
	SIZE_T        BackAncestorCount;
	SIZE_T        OutputSegmentCount;
	SIZE_T        ParentSegmentCount;

	//
	// An empty name is not a valid name.
	// TODO: Is it ever valid to refer to just a parent namespace without any object names?
	//
	if( ( Input->SegmentCount > ( SIZE_MAX / sizeof( AML_NAME_SEG ) ) ) ) {
		return AML_FALSE;
	}

	//
	// If the input name is already absolute, and begins with the RootChar, nothing needs to be done, just deep copy the input.
	//
	if( ( Input->Prefix.Length > 0 ) && ( Input->Prefix.Data[ 0 ] == '\\' ) ) {
		OutputSegments = AmlArenaAllocate( Arena, ( sizeof( AML_NAME_SEG ) * Input->SegmentCount ) );
		if( OutputSegments == NULL ) {
			return AML_FALSE;
		}

		//
		// Copy all input name segments to the new output copy allocation.
		//
		for( i = 0; i < Input->SegmentCount; i++ ) {
			OutputSegments[ i ] = Input->Segments[ i ];
		}

		//
		// Return the deep-copied absolute name string.
		//
		*AbsoluteOutput = ( AML_NAME_STRING ){
			.Prefix       = Input->Prefix,
			.SegmentCount = Input->SegmentCount,
			.Segments     = OutputSegments
		};

		return AML_TRUE;
	}

	//
	// Since the root prefix has already been handled, there can only be '^' parent prefixes.
	// Each '^' prefix moves up a namespace parent from the current scope,
	// and correspondingly, one less name segment that the final absolute output path will need.
	//
	BackAncestorCount = 0;
	for( i = 0; i < Input->Prefix.Length; i++ ) {
		if( ( Input->Prefix.Data[ i ] != '^' ) ) {
			return AML_FALSE;
		}
		BackAncestorCount++;
	}

	//
	// If this is an ancestor prefixed path, the parent scope's path must
	// be large enough to move back by the desired amount of ancestors.
	//
	if( BackAncestorCount > ActiveScope->AbsolutePath.SegmentCount ) {
		return AML_FALSE;
	}

	//
	// Use the stored absolute path of the found parent namespace as the base of our new path.
	// This avoids having to iterate ancestor namespaces upwards until root to build the path.
	//
	ParentSegmentCount = ( ActiveScope->AbsolutePath.SegmentCount - BackAncestorCount );
	if( ParentSegmentCount > ( SIZE_MAX - Input->SegmentCount ) ) {
		return AML_FALSE;
	}
	OutputSegmentCount = ( ParentSegmentCount + Input->SegmentCount );

	//
	// Allocate memory for the full output path.
	//
	OutputSegments = AmlArenaAllocate( Arena, ( sizeof( AML_NAME_SEG ) * OutputSegmentCount ) );
	if( OutputSegments == NULL ) {
		return AML_FALSE;
	}

	//
	// Copy in full absolute path of the selected parent namespace.
	//
	for( i = 0; i < ParentSegmentCount; i++ ) {
		OutputSegments[ i ] = ActiveScope->AbsolutePath.Segments[ i ];
	}

	//
	// Append the relative part of the input path to the absolute path of the selected ancestor.
	//
	for( i = 0; i < Input->SegmentCount; i++ ) {
		OutputSegments[ ParentSegmentCount + i ] = Input->Segments[ i ];
	}

	//
	// Write out the converted path.
	// The prefixes are taken from the ancestor node that we took the AbsolutePath from, and are assumed to be valid ('\').
	//
	*AbsoluteOutput = ( AML_NAME_STRING ){
		.Prefix       = ActiveScope->AbsolutePath.Prefix,
		.SegmentCount = OutputSegmentCount,
		.Segments     = OutputSegments
	};

	return AML_TRUE;
}

//
// Resolve and hash the absolute path of the given prefixed name string.
//
_Success_( return )
static
BOOLEAN
AmlNamespaceHashPrefixedNameString(
	_In_  const AML_NAMESPACE_SCOPE* ActiveScope,
	_In_  const AML_NAME_STRING*     Name,
	_Out_ UINT32*                    pHashSum
	)
{
	UINT32  Seed;
	SIZE_T  i;
	SIZE_T  BackAncestorCount;
	BOOLEAN IsRelative;

	//
	// Starting hash seed.
	//
	Seed = AML_NAMESPACE_HASH_SEED;

	//
	// Handle prefixed names.
	// If this is a parent scope prefix, find the namespace ancestor that this relative name is referring to.
	//
	IsRelative = AML_TRUE;
	BackAncestorCount = 0;
	if( Name->Prefix.Length > 0 ) {
		if( Name->Prefix.Data[ 0 ] == '^' ) {
			for( i = 0; i < Name->Prefix.Length; i++ ) {
				if( ( Name->Prefix.Data[ i ] != '^' ) ) {
					return AML_FALSE;
				}
				BackAncestorCount++;
			}
		} else if( Name->Prefix.Data[ 0 ] != '\\' ) {
			return AML_FALSE;
		}
		IsRelative = ( Name->Prefix.Data[ 0 ] != '\\' );
	}

	//
	// If this is an ancestor prefixed path, the parent scope's path must
	// be large enough to move back by the desired amount of ancestors.
	//
	if( BackAncestorCount > ActiveScope->AbsolutePath.SegmentCount ) {
		return AML_FALSE;
	}
	
	//
	// Accumulate the hash of the path of the ancestor that this name is relative to.
	//
	if( IsRelative ) {
		for( i = 0; i < ( ActiveScope->AbsolutePath.SegmentCount - BackAncestorCount ); i++ ) {
			Seed = AmlHashKey32( ActiveScope->AbsolutePath.Segments[ i ].Data,
								 sizeof( ActiveScope->AbsolutePath.Segments[ i ].Data ),
								 Seed );
		}
	}

	//
	// Accumulate the hash of the input name.
	//
	for( i = 0; i < Name->SegmentCount; i++ ) {
		Seed = AmlHashKey32( Name->Segments[ i ].Data, sizeof( Name->Segments[ i ].Data ), Seed );
	}

	//
	// Return the calculated absolute path hash.
	//
	*pHashSum = Seed;
	return AML_TRUE;
}

//
// Compare two AML_NAME_STRING values without any resolution.
// For example, two names that may resolve to the same absolute path/object,
// but are written differently to depend on scope will not compare as equal,
// AmlNamespaceCompareNameString should be used instead.
//
static
BOOLEAN
AmlNamespaceCompareNameStringLocal(
	_In_ const AML_NAME_STRING* String1,
	_In_ const AML_NAME_STRING* String2
	)
{
	SIZE_T i;

	//
	// Early-out by comparing lengths first.
	//
	if( ( String1->Prefix.Length != String2->Prefix.Length ) || ( String1->SegmentCount != String2->SegmentCount ) ) {
		return AML_FALSE;
	}

	//
	// Compare all prefix characters.
	//
	for( i = 0; i < String1->Prefix.Length; i++ ) {
		if( String1->Prefix.Data[ i ] != String2->Prefix.Data[ i ] ) {
			return AML_FALSE;
		}
	}

	//
	// Compare all segments.
	//
	for( i = 0; i < String1->SegmentCount; i++ ) {
		if( String1->Segments[ i ].AsUInt32 != String2->Segments[ i ].AsUInt32 ) {
			return AML_FALSE;
		}
	}

	return AML_TRUE;
}

//
// Fully expand and compare two AML_NAME_STRING values.
//
BOOLEAN
AmlNamespaceCompareNameString(
	_Inout_  AML_NAMESPACE_STATE*       State,
	_In_opt_ const AML_NAMESPACE_SCOPE* ActiveScope,
	_In_     const AML_NAME_STRING*     String1,
	_In_     const AML_NAME_STRING*     String2
	)
{
	AML_ARENA_SNAPSHOT Snapshot;
	AML_NAME_STRING    AbsString1;
	AML_NAME_STRING    AbsString2;
	BOOLEAN            Match;

	//
	// Snapshot temporary arena to rollback all allocations.
	//
	Snapshot = AmlArenaSnapshot( &State->TempArena );

	//
	// Use the current namespace scope if none is given.
	//
	ActiveScope = ( ( ActiveScope == NULL ) ? State->ScopeLast : ActiveScope );

	//
	// Attempt to convert both strings to absolute paths relative to the given scope,
	// and compare the resolved absolute name strings.
	//
	Match = AML_FALSE;
	do {
		if( AmlNamespaceConvertNameStringToAbsolutePath( ActiveScope, &State->TempArena, String1, &AbsString1 ) == AML_FALSE ) {
			break;
		} else if( AmlNamespaceConvertNameStringToAbsolutePath( ActiveScope, &State->TempArena, String2, &AbsString2 ) == AML_FALSE ) {
			break;
		}
		Match = AmlNamespaceCompareNameStringLocal( &AbsString1, &AbsString2 );
	} while( 0 );

	//
	// Rollback all temporary allocations.
	//
	AmlArenaSnapshotRollback( &State->TempArena, &Snapshot );
	return Match;
}

//
// For relative name paths that contain multiple NameSegs or Parent Prefixes, '^', the search rules do not apply.
// If the search rules do not apply to a relative namespace path, the namespace object is looked up relative to the current namespace.
//
_Success_( return != NULL )
static
AML_NAMESPACE_NODE*
AmlNamespaceSearchPrefixedName(
	_In_ AML_NAMESPACE_STATE*   State,
	_In_ AML_NAMESPACE_SCOPE*   ActiveScope,
	_In_ const AML_NAME_STRING* Name
	)
{
	UINT32              Hash;
	AML_ARENA_SNAPSHOT  Snapshot;
	AML_NAME_STRING     AbsoluteName;
	AML_NAMESPACE_NODE* BucketEntry;
	AML_NAMESPACE_NODE* FoundNode;

	//
	// Must be a prefixed name, or a name with multiple name segments.
	//
	if( ( Name->Prefix.Length <= 0 ) && ( Name->SegmentCount <= 1 ) ) {
		return NULL;
	}

	//
	// Resolve and hash the absolute path of the input node and attempt to find the given path in the hash table.
	//
	Snapshot = AmlArenaSnapshot( &State->TempArena );
	FoundNode = NULL;
	do {
		//
		// Attempt to resolve the absolute path of the input name.
		//
		if( AmlNamespaceConvertNameStringToAbsolutePath( ActiveScope,
														 &State->TempArena,
														 Name,
														 &AbsoluteName ) == AML_FALSE )
		{
			break;
		}

		//
		// Attempt to hash the input absolute path.
		//
		if( AmlNamespaceHashPrefixedNameString( ActiveScope, Name, &Hash ) == AML_FALSE ) {
			break;
		}

		//
		// Search for a node with this full path in the hash-table.
		//
		BucketEntry = State->AbsolutePathMap[ Hash % AML_COUNTOF( State->AbsolutePathMap ) ];
		for( ; BucketEntry != NULL; BucketEntry = BucketEntry->BucketNext ) {
			if( ( BucketEntry->AbsolutePathHash == Hash )
				&& AmlNamespaceCompareNameStringLocal( &BucketEntry->AbsolutePath, &AbsoluteName ) )
			{
				FoundNode = BucketEntry;
			}
		}
	} while( 0 );
	AmlArenaSnapshotRollback( &State->TempArena, &Snapshot );

	return FoundNode;
}

//
// Search an unprefixed single segment path relative to all possible scope ancestors.
//
_Success_( return != NULL )
static
AML_NAMESPACE_NODE*
AmlNamespaceSearchRelativeName(
	_In_ AML_NAMESPACE_STATE*   State,
	_In_ AML_NAMESPACE_SCOPE*   ActiveScope,
	_In_ const AML_NAME_STRING* Name,
	_In_ UINT                   SearchFlags
	)
{
	SIZE_T              i;
	UINT32              Hash;
	SIZE_T              j;
	const AML_NAME_SEG* Segment;
	AML_NAMESPACE_NODE* BucketEntry;
	AML_NAMESPACE_NODE* FoundNode;
	AML_NAME_STRING     PathPrefix1;
	AML_NAME_STRING     PathPrefix2;

	//
	// Must be an unprefixed name with a single relative name segment.
	//
	if( ( Name->Prefix.Length > 0 ) || ( Name->SegmentCount != 1 ) ) {
		return NULL;
	}

	//
	// Print debug information about the search.
	//
#if 0
	AML_DEBUG_PRINT( "Scanning up parent scope \"" );
	AmlDebugPrintNameString( &ActiveScope->AbsolutePath );
	AML_DEBUG_PRINT( "\" for name \"" );
	AmlDebugPrintNameString( Name );
	AML_DEBUG_PRINT( "\":\n" );
#endif

	//
	// A name is located by finding the matching name in the current namespace, and then in the parent namespace.
	// If the parent namespace does not contain the name, the search continues recursively upwards until either the
	// name is found or the namespace does not have a parent (the root of the namespace).
	//
	FoundNode = NULL;
	for( i = 0; ( i < ( ActiveScope->AbsolutePath.SegmentCount + 1 ) ) && ( FoundNode == NULL ); i++ ) {
		//
		// If we are attempting to create a new node with this name,
		// we only need to check the scope that the name is being declared in,
		// otherwise we will traverse up the scope tree and possibly match an existing
		// name within a higher scope level.
		//
		if( ( SearchFlags & AML_SEARCH_FLAG_NAME_CREATION ) && ( i != 0 ) ) {
			break;
		}

		//
		// Accumulate the scope segment hashes up to the current search ancestor index.
		//
		Hash = AML_NAMESPACE_HASH_SEED;
		if( i < ActiveScope->AbsolutePath.SegmentCount ) {
			for( j = 0; j < ( ActiveScope->AbsolutePath.SegmentCount - i ); j++ ) {
				Segment = &ActiveScope->AbsolutePath.Segments[ j ];
				Hash = AmlHashKey32( Segment->Data, sizeof( Segment->Data ), Hash );
			}
		}

		//
		// Accumulate the hash of the relative search name.
		//
		Hash = AmlHashKey32( Name->Segments[ 0 ].Data, sizeof( Name->Segments[ 0 ].Data ), Hash );

		//
		// Search for a node with this full path in the hash-table.
		//
		BucketEntry = State->AbsolutePathMap[ Hash % AML_COUNTOF( State->AbsolutePathMap ) ];
		for( ; BucketEntry != NULL; BucketEntry = BucketEntry->BucketNext ) {
			//
			// Typical case, early out if the hashes of the two paths don't match.
			// In case of hash collisions, the actual absolute paths are compared after this.
			// 
			if( BucketEntry->AbsolutePathHash != Hash ) {
				continue;
			}

			//
			// If this isn't the root of the search path, compare the path of the found node and the search path.
			// This doesn't include the local name being searched for, but the part of the path that comes before it.
			//
			if( i < ActiveScope->AbsolutePath.SegmentCount ) {
				PathPrefix1 = BucketEntry->AbsolutePath;
				if( PathPrefix1.SegmentCount > 0 ) {
					PathPrefix1.SegmentCount -= 1;
				}
				PathPrefix2 = ActiveScope->AbsolutePath;
				PathPrefix2.SegmentCount -= i;
				if( AmlNamespaceCompareNameStringLocal( &PathPrefix1, &PathPrefix2 ) == AML_FALSE ) {
					continue;
				}
			}

			//
			// Hash and path prefixes match, finally attempt to compare the local name being searched.
			//
			if( BucketEntry->LocalName.AsUInt32 == Name->Segments[ 0 ].AsUInt32 ) {
				FoundNode = BucketEntry;
				break;
			}
		}
	}

	return FoundNode;
}

//
// Search for an existing node with the given name.
// Searches for the given name/path according to the rules in 5.3. ACPI Namespace.
//
_Success_( return )
BOOLEAN
AmlNamespaceSearch(
	_In_     AML_NAMESPACE_STATE*   State,
	_In_opt_ AML_NAMESPACE_SCOPE*   ActiveScope,
	_In_     const AML_NAME_STRING* Name,
	_In_     UINT                   SearchFlags,
	_Outptr_ AML_NAMESPACE_NODE**   ppFoundNode
	)
{
	AML_NAMESPACE_NODE* FoundNode;

	//
	// Use last scope if no specific active scope was given.
	//
	ActiveScope = ( ( ActiveScope != NULL ) ? ActiveScope : State->ScopeLast );

	//
	// Default to no found node, simplifies control flow for both cases.
	//
	FoundNode = NULL;

	//
	// For relative name paths that contain multiple NameSegs or Parent Prefixes, '^', the search rules do not apply.
	// If the search rules do not apply to a relative namespace path, the namespace object is looked up relative to the current namespace.
	// Calculate the hash of the input name using prefix path rules.
	//
	if( ( Name->Prefix.Length > 0 ) || ( Name->SegmentCount > 1 ) ) {
		FoundNode = AmlNamespaceSearchPrefixedName( State, ActiveScope, Name );
	} else {
		FoundNode = AmlNamespaceSearchRelativeName( State, ActiveScope, Name, SearchFlags );
	}

	//
	// Failed to locate the given name.
	//
	if( FoundNode == NULL ) {
		return AML_FALSE;
	}

	//
	// If alias resolution is enabled, we must try to recursively find the final non-alias destination node.
	// TODO: Keep track of recursion count, or prevent infinite alias resolution loops by visited stack.
	//
	while( ( FoundNode->Object->Type == AML_OBJECT_TYPE_ALIAS )
		   && ( ( SearchFlags & ( AML_SEARCH_FLAG_NAME_CREATION | AML_SEARCH_FLAG_NO_ALIAS_RESOLUTION ) ) == 0 ) )
	{
		if( AmlNamespaceSearch( State, NULL, &FoundNode->Object->u.Alias.DestinationName, SearchFlags, &FoundNode ) == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	//
	// Failed to locate the given name.
	//
	if( FoundNode == NULL ) {
		return AML_FALSE;
	}

	//
	// Return the final found and fully resolved node.
	//
	*ppFoundNode = FoundNode;
	return AML_TRUE;
}

//
// Search for an existing node with the given null-terminated path string.
// Searches for the given name/path according to the rules in 5.3. ACPI Namespace.
//
_Success_( return )
BOOLEAN
AmlNamespaceSearchZ(
	_In_     AML_NAMESPACE_STATE* State,
	_In_opt_ AML_NAMESPACE_SCOPE* ActiveScope,
	_In_z_   const CHAR*          PathString,
	_In_     UINT                 SearchFlags,
	_Outptr_ AML_NAMESPACE_NODE** ppFoundNode
	)
{
	AML_ARENA_SNAPSHOT Snapshot;
	BOOLEAN            Success;
	AML_NAME_STRING    Name;

	//
	// Attempt to parse a null-terminated path string to an AML_NAME_STRING and search it like normal.
	//
	Snapshot = AmlArenaSnapshot( &State->TempArena );
	if( ( Success = AmlPathStringZToNameString( &State->TempArena, PathString, &Name ) ) != AML_FALSE ) {
		Success = AmlNamespaceSearch( State, ActiveScope, &Name, SearchFlags, ppFoundNode );
	}
	AmlArenaSnapshotRollback( &State->TempArena, &Snapshot );
	return Success;
}

//
// Push new namespace scope level.
//
_Success_( return )
BOOLEAN
AmlNamespacePushScope(
	_Inout_ AML_NAMESPACE_STATE*   State,
	_In_    const AML_NAME_STRING* Name,
	_In_    UINT                   ScopeFlags
	)
{
	AML_ARENA_SNAPSHOT   ScopeArenaSnapshot;
	AML_NAMESPACE_SCOPE* Scope;
	AML_NAME_STRING      AbsolutePath;
	AML_NAME_STRING      ParentPath;
	AML_NAMESPACE_NODE*  ParentNode;
	UINT32               PathHash;

	//
	// Attempt to allocate a new namespace scope stack entry.
	//
	ScopeArenaSnapshot = AmlArenaSnapshot( &State->ScopeArena );
	Scope = AmlArenaAllocate( &State->ScopeArena, sizeof( AML_NAMESPACE_SCOPE ) );
	if( Scope == NULL ) {
		return AML_FALSE;
	}

	//
	// Attempt to resolve the input name string to an absolute path, and keep a copy of it.
	//
	if( Name->SegmentCount != 0 ) {
		if( AmlNamespaceConvertNameStringToAbsolutePath( State->ScopeLast,
														 &State->ScopeArena,
														 Name,
														 &AbsolutePath ) == AML_FALSE )
		{
			AmlArenaSnapshotRollback( &State->ScopeArena, &ScopeArenaSnapshot );
			return AML_FALSE;
		}
	} else {
		AbsolutePath = ( AML_NAME_STRING ){ .Prefix = { .Data = { '\\' }, .Length = 1 } };
	}

	//
	// Attempt to hash the absolute path.
	//
	if( AmlNamespaceHashPrefixedNameString( State->ScopeLast, &AbsolutePath, &PathHash ) == AML_FALSE ) {
		AmlArenaSnapshotRollback( &State->ScopeArena, &ScopeArenaSnapshot );
		return AML_FALSE;
	}

	//
	// Default initialize scope level.
	//
	*Scope = ( AML_NAMESPACE_SCOPE ){
		.ArenaSnapshot    = ScopeArenaSnapshot,
		.AbsolutePath     = AbsolutePath,
		.AbsolutePathHash = PathHash,
		.Flags            = ScopeFlags,
	};

	//
	// Attempt to look up the parent namespace node (if any) to apply parent-specific properties to the new child scope.
	// TODO: Scan back for the first valid node in the parent path.
	//
	if( AbsolutePath.SegmentCount > 0 ) {
		ParentPath = ( AML_NAME_STRING ){
			.Prefix       = AbsolutePath.Prefix,
			.Segments     = AbsolutePath.Segments,
			.SegmentCount = ( AbsolutePath.SegmentCount - 1 )
		};

		//
		// If we arent explicitly switching to a completely different scope, propagate the flags of the parent!
		//
		if( ( ScopeFlags & AML_SCOPE_FLAG_SWITCH ) == 0 ) {
			if( AmlNamespaceSearch( State, NULL, &ParentPath, 0, &ParentNode ) ) {
				Scope->Flags |= ParentNode->ScopeFlags;
			}
		}
	}

	//
	// Push scope, link to previous scope levels.
	//
	Scope->Parent = State->ScopeLast;
	if( State->ScopeLast != NULL ) {
		Scope->LevelIndex = ( State->ScopeLast->LevelIndex + 1 );
		State->ScopeLast->Child = Scope;
	}
	State->ScopeLast  = Scope;
	State->ScopeFirst = ( ( State->ScopeFirst != NULL ) ? State->ScopeFirst : Scope );
	return AML_TRUE;
}

//
// Pop last namespace scope level.
//
_Success_( return )
BOOLEAN
AmlNamespacePopScope(
	_Inout_ AML_NAMESPACE_STATE* State
	)
{
	AML_NAMESPACE_SCOPE* PoppedScope;
	AML_NAMESPACE_NODE*  Node;
	AML_NAMESPACE_NODE*  NextNode;

	//
	// Cannot pop the first special root scope level!
	//
	if( State->ScopeLast == &State->ScopeRoot ) {
		return AML_FALSE;
	}

	//
	// Pop the last scope entry from the stack.
	//
	PoppedScope = State->ScopeLast;
	if( PoppedScope->Parent != NULL ) {
		PoppedScope->Parent->Child = NULL;
	}
	State->ScopeLast  = PoppedScope->Parent;
	State->ScopeFirst = ( ( State->ScopeFirst != PoppedScope ) ? State->ScopeFirst : NULL );

	//
	// If this was a temporary scope level, release all temporary nodes.
	//
	if( ( PoppedScope->Flags & AML_SCOPE_FLAG_TEMPORARY ) ) {
		// AML_DEBUG_TRACE( State, "Destroying temporary scope: " );
		// AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_TRACE, &PoppedScope->AbsolutePath );
		// AML_DEBUG_TRACE( State, "\n" );
		for( Node = PoppedScope->TempNodeHead; Node != NULL; Node = NextNode ) {
			NextNode = Node->TempNext;
			// AML_DEBUG_TRACE( State, "Freeing temporary namespace node: " );
			// AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_TRACE, &Node->AbsolutePath );
			// AML_DEBUG_TRACE( State, "\n" );
			AmlNamespaceReleaseNode( State, Node );
		}
	}

	//
	// Rollback the allocation of the scope entry and all of its suballocations.
	//
	return AmlArenaSnapshotRollback( &State->ScopeArena, &PoppedScope->ArenaSnapshot );
}

//
// Create and link a new namespace node (does not initialize the node's object/value).
// TODO: Release resources upon failure!
//
_Success_( return )
BOOLEAN
AmlNamespaceCreateNode(
	_Inout_     AML_NAMESPACE_STATE*   State,
	_Inout_opt_ AML_NAMESPACE_SCOPE*   ActiveScope,
	_In_        const AML_NAME_STRING* Name,
	_Outptr_    AML_NAMESPACE_NODE**   ppCreatedNode
	)
{
	AML_NAMESPACE_NODE*  Node;
	AML_NAMESPACE_NODE** ppBucketHead;
	AML_NAMESPACE_NODE*  BucketHead;
	AML_NAMESPACE_NODE*  BucketEntry;
	AML_NAME_STRING      ParentPath;
	AML_NAMESPACE_NODE*  ParentNode;

	//
	// Use the last level of scope if none given.
	//
	ActiveScope = ( ( ActiveScope != NULL ) ? ActiveScope : State->ScopeLast );

	//
	// Allocate permanent node.
	// TODO: This should be released upon failure in this function.
	//
	if( ( Node = AmlHeapAllocate( State->Heap, sizeof( *Node ) ) ) == NULL ) {
		return AML_FALSE;
	}

	//
	// Default initialize empty node.
	//
	*Node = ( AML_NAMESPACE_NODE ){ .ReferenceCount = 1, .Object = &State->NilObject, .ParentState = State };

	//
	// Resolve and allocate a copy of the absolute path to the given name.
	//
	if( AmlNamespaceConvertNameStringToAbsolutePath( ActiveScope, &State->PermanentArena, Name, &Node->AbsolutePath ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Calculate path hash of the absolute path.
	//
	if( AmlNamespaceHashPrefixedNameString( ActiveScope, &Node->AbsolutePath, &Node->AbsolutePathHash ) == AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// Save local name segment of the node.
	//
	if( Node->AbsolutePath.SegmentCount > 0 ) {
		Node->LocalName = Node->AbsolutePath.Segments[ Node->AbsolutePath.SegmentCount - 1 ];
	}

	//
	// Lookup the hash-table bucket that the absolute path belongs to.
	//
	ppBucketHead = &State->AbsolutePathMap[ Node->AbsolutePathHash % AML_COUNTOF( State->AbsolutePathMap ) ];
	BucketHead   = *ppBucketHead;

	//
	// Insert the node to the hash-table bucket.
	//
	if( BucketHead != NULL ) {
		//
		// Ensure that there are no name collisions.
		//
		for( BucketEntry = BucketHead;
			 BucketEntry != NULL;
			 BucketEntry = BucketEntry->BucketNext )
		{
			if( ( BucketEntry->AbsolutePathHash == Node->AbsolutePathHash )
				|| AmlNamespaceCompareNameStringLocal( &BucketEntry->AbsolutePath, &Node->AbsolutePath ) )
			{
				return AML_FALSE;
			}
		}

		//
		// Link the new node to the head end of the bucket.
		//
		Node->BucketNext = BucketHead;
		BucketHead->BucketPrevious = Node;
	}

	//
	// Attempt to look up the parent namespace node (if any) to apply parent-specific properties to the new child scope.
	// We don't scan backwards up through the whole path to handle paths with a missing parent link, as this may be happening out of order.
	//
	if( Node->AbsolutePath.SegmentCount > 0 ) {
		ParentPath = ( AML_NAME_STRING ){
			.Prefix       = Node->AbsolutePath.Prefix,
			.Segments     = Node->AbsolutePath.Segments,
			.SegmentCount = ( Node->AbsolutePath.SegmentCount - 1 )
		};

		//
		// Apply parent-specific properties if the parent exists.
		//
		if( AmlNamespaceSearch( State, NULL, &ParentPath, 0, &ParentNode ) ) {
			//
			// Propogate the flags and temporary scope of the parent to the new child.
			//
			Node->ScopeFlags |= ParentNode->ScopeFlags;
			Node->TempScope   = ParentNode->TempScope;

			//
			// If the found parent node is already present in the hierarchical tree, link the child to it.
			//
			if( ParentNode->TreeEntry.IsPresent ) {
				AmlNamespaceTreeLinkChildNode( State, &ParentNode->TreeEntry, &Node->TreeEntry );
			}
		}
	}

	//
	// If the scope that this node is being created in is temporary, link it to the scope's temporary node list.
	//
	if( Node->TempScope != NULL ) {
		Node->TempNext = Node->TempScope->TempNodeHead;
		if( Node->TempScope->TempNodeHead != NULL ) {
			Node->TempScope->TempNodeHead->TempPrevious = Node;
		}
		Node->TempScope->TempNodeHead = Node;
		Node->TempScope->TempNodeTail = ( ( Node->TempScope->TempNodeTail == NULL )
										  ? Node : Node->TempScope->TempNodeTail );
	}

	//
	// Insert the node to the evaluation-order node list.
	//
	if( State->InOrderNodeTail != NULL ) {
		Node->InOrderPrevious = State->InOrderNodeTail;
		State->InOrderNodeTail->InOrderNext = Node;
	}
	State->InOrderNodeTail = Node;
	if( State->InOrderNodeHead == NULL ) {
		State->InOrderNodeHead = Node;
	}

	//
	// Make the inserted node the new head of the bucket.
	//
	*ppBucketHead = Node;

	//
	// Return created and linked namespace node.
	//
	*ppCreatedNode = Node;
	return AML_TRUE;
}

//
// Attempt to find the parent node of a namespace node.
//
_Success_( return != NULL )
AML_NAMESPACE_NODE*
AmlNamespaceParentNode(
	_In_ AML_NAMESPACE_STATE* State,
	_In_ AML_NAMESPACE_NODE*  Node
	)
{
	AML_NAMESPACE_NODE* Parent;
	SIZE_T              i;
	AML_NAME_STRING     AncestorPath;
	AML_NAMESPACE_NODE* AncestorNode;

	//
	// If the hierarchical tree has already been built, first try to use it, otherwise,
	// fall bak to manually scanning the path segments using the hash table.
	//
	if( Node->TreeEntry.IsPresent ) {
		if( Node->TreeEntry.Parent == &State->TreeRoot ) {
			return NULL;
		}
		return AML_CONTAINING_RECORD( Node->TreeEntry.Parent, AML_NAMESPACE_NODE, TreeEntry );
	}

	//
	// The root node has no parent.
	//
	if( Node->AbsolutePath.SegmentCount < 1 ) {
		return NULL;
	}

	//
	// Scan the node's path segments for the highest created object in the path hierarchy.
	//
	Parent = NULL;
	for( i = 0; ( i <= ( Node->AbsolutePath.SegmentCount - 1 ) ) && ( Parent == NULL ); i++ ) {
		AncestorPath = Node->AbsolutePath;
		AncestorPath.SegmentCount = ( Node->AbsolutePath.SegmentCount - 1 - i );
		if( AmlNamespaceSearch( State, NULL, &AncestorPath, 0, &AncestorNode ) ) {
			Parent = AncestorNode;
		}
	}

	return Parent;
}

//
// Search for the given name relative to the input node.
// Attempts to find the given Name relative to the node or any of its ancestors.
//
_Success_( return != NULL )
AML_NAMESPACE_NODE*
AmlNamespaceSearchRelative(
	_In_ AML_NAMESPACE_STATE*      State,
	_In_ const AML_NAMESPACE_NODE* Node,
	_In_ const AML_NAME_STRING*    Name,
	_In_ BOOLEAN                   SearchAncestors
	)
{
	BOOLEAN             Success;
	AML_NAMESPACE_NODE* FoundNode;

	//
	// Temporarily switch to the scope of the given parent node and search for the name.
	//
	if( AmlNamespacePushScope( State, &Node->AbsolutePath, AML_SCOPE_FLAG_SWITCH ) == AML_FALSE ) {
		return NULL;
	}
	Success = AmlNamespaceSearch( State, NULL, Name, ( SearchAncestors ? 0 : AML_SEARCH_FLAG_NAME_CREATION ), &FoundNode );
	if( AmlNamespacePopScope( State ) == AML_FALSE ) {
		return NULL;
	}
	return ( Success ? FoundNode : NULL );
}

//
// Search for the given name relative to the input node.
// Attempts to find the given Name relative to the node or any of its ancestors.
//
_Success_( return != NULL )
AML_NAMESPACE_NODE*
AmlNamespaceSearchRelativeZ(
	_In_   AML_NAMESPACE_STATE*      State,
	_In_   const AML_NAMESPACE_NODE* Node,
	_In_z_ const CHAR*               NameString,
	_In_   BOOLEAN                   SearchAncestors
	)
{
	AML_ARENA_SNAPSHOT  Snapshot;
	AML_NAME_STRING     Name;
	AML_NAMESPACE_NODE* FoundNode;

	//
	// Attempt to parse a null-terminated path string to an AML_NAME_STRING and search like normal.
	//
	FoundNode = NULL;
	Snapshot = AmlArenaSnapshot( &State->TempArena );
	if( AmlPathStringZToNameString( &State->TempArena, NameString, &Name ) ) {
		FoundNode = AmlNamespaceSearchRelative( State, Node, &Name, SearchAncestors );
	}
	AmlArenaSnapshotRollback( &State->TempArena, &Snapshot );
	return FoundNode;
}

//
// Search for a child of the given node with the ChildName.
//
_Success_( return != NULL )
AML_NAMESPACE_NODE*
AmlNamespaceChildNode(
	_In_ AML_NAMESPACE_STATE*      State,
	_In_ const AML_NAMESPACE_NODE* Node,
	_In_ const AML_NAME_STRING*    ChildName
	)
{
	return AmlNamespaceSearchRelative( State, Node, ChildName, AML_FALSE );
}

//
// Search for a child of the given node with the null-terminated ChildName path string.
//
_Success_( return != NULL )
AML_NAMESPACE_NODE*
AmlNamespaceChildNodeZ(
	_In_   AML_NAMESPACE_STATE*      State,
	_In_   const AML_NAMESPACE_NODE* Node,
	_In_z_ const CHAR*               NameString
	)
{
	return AmlNamespaceSearchRelativeZ( State, Node, NameString, AML_FALSE );
}

//
// Release a reference to a namespace node, and free resources if this was the last held reference.
//
VOID
AmlNamespaceReleaseNode(
	_Inout_                AML_NAMESPACE_STATE* State,
	_Inout_ _Post_invalid_ AML_NAMESPACE_NODE*  Node
	)
{
	UINT32 BucketIndex;

	//
	// Push a release action to the current snapshot if this node is within one.
	//
	AmlStateSnapshotItemPushAction( Node->StateItem, AML_FALSE );

	//
	// Lower the reference count.
	//
	if( --Node->ReferenceCount != 0 ) {
		return;
	}

	//
	// Release the linked object (if any).
	//
	if( Node->Object != &State->NilObject ) {
		Node->Object->NamespaceNode = NULL;
		AmlObjectRelease( Node->Object );
	}

	//
	// Unlink the node from the path hash-table sibling nodes.
	//
	if( Node->BucketPrevious != NULL ) {
		Node->BucketPrevious->BucketNext = Node->BucketNext;
	}
	if( Node->BucketNext != NULL ) {
		Node->BucketNext->BucketPrevious = Node->BucketPrevious;
	}

	//
	// If this node was the head of the hash-table bucket, update the new head.
	//
	BucketIndex = ( Node->AbsolutePathHash % AML_COUNTOF( State->AbsolutePathMap ) );
	if( State->AbsolutePathMap[ BucketIndex ] == Node ) {
		State->AbsolutePathMap[ BucketIndex ] = Node->BucketNext;
	}

	//
	// Unlink the node from the in-order list.
	//
	if( Node->InOrderPrevious != NULL ) {
		Node->InOrderPrevious->InOrderNext = Node->InOrderNext;
	}
	if( Node->InOrderNext != NULL ) {
		Node->InOrderNext->InOrderPrevious = Node->InOrderPrevious;
	}
	State->InOrderNodeTail = ( ( State->InOrderNodeTail != Node ) ? State->InOrderNodeTail : Node->InOrderPrevious );
	State->InOrderNodeHead = ( ( State->InOrderNodeHead != Node ) ? State->InOrderNodeHead : NULL );

	//
	// Ensure that the node is no longer present in the hierarchical tree.
	//
	AmlNamespaceTreeUnlinkNode( State, &Node->TreeEntry );

	//
	// Free the namespace node allocation.
	//
	AmlHeapFree( State->Heap, Node );
}

//
// Unlink the given node from the tree, transplanting its children to the parent.
//
VOID
AmlNamespaceTreeUnlinkNode(
	_Inout_ AML_NAMESPACE_STATE*     State,
	_Inout_ AML_NAMESPACE_TREE_NODE* Node
	)
{
	AML_NAMESPACE_TREE_NODE* ChildNode;

	//
	// Nothing to do if the node is already unlinked from the tree.
	//
	if( Node->IsPresent == AML_FALSE ) {
		return;
	}

	//
	// Unlink the tree node from its siblings.
	//
	if( Node->Next != NULL ) {
		Node->Next->Previous = Node->Previous;
	}
	if( Node->Previous != NULL ) {
		Node->Previous->Next = Node->Next;
	}
	
	//
	// Unlink the tree node from its children.
	//
	if( Node->ChildFirst != NULL ) {
		//
		// Update the parent of all child nodes to the parent of the removed node.
		//
		for( ChildNode = Node->ChildFirst; ChildNode != NULL; ChildNode = ChildNode->Next ) {
			ChildNode->Parent = Node->Parent;
		}

		//
		// Transplant the child list to the child list of the parent of the removed node.
		//
		if( Node->Parent != NULL ) {
			if( Node->Parent->ChildLast != NULL ) {
				Node->Parent->ChildLast->Next = Node->ChildFirst;
				Node->ChildFirst->Previous = Node->Parent->ChildLast;
			}
			Node->Parent->ChildLast = Node->ChildLast;
			if( Node->Parent->ChildFirst == NULL ) {
				Node->Parent->ChildFirst = Node->ChildFirst;
			}
		}
	}

	//
	// Unlink the tree node from its parent.
	//
	if( Node->Parent != NULL ) {
		if( Node->Parent->ChildFirst == Node ) {
			Node->Parent->ChildFirst = Node->Next;
		}
		if( Node->Parent->ChildLast == Node ) {
			Node->Parent->ChildLast = Node->Previous;
		}
	}

	//
	// Reset the node's information.
	//
	*Node = ( AML_NAMESPACE_TREE_NODE ){ .IsPresent = AML_FALSE };
}

//
// Link the given node as a child to the parent tree node.
// The node being inserted as a child should not be linked to anything.
//
VOID
AmlNamespaceTreeLinkChildNode(
	_Inout_ AML_NAMESPACE_STATE*     State,
	_Inout_ AML_NAMESPACE_TREE_NODE* TreeNode,
	_Inout_ AML_NAMESPACE_TREE_NODE* ChildTreeNode
	)
{
	//
	// The given child tree node must not already be present in the tree!
	//
	if( ChildTreeNode->IsPresent ) {
		AML_TRAP_STRING( "AmlNamespaceTreeLinkChildNode: Child already present in the tree!" );
	}
	
	//
	// Reset child sibling links.
	// The child being inserted should not be already linked to anything.
	//
	ChildTreeNode->Next = NULL;
	ChildTreeNode->Previous = NULL;

	//
	// Link the child to the end of the parent's child list.
	//
	if( TreeNode->ChildLast != NULL ) {
		TreeNode->ChildLast->Next = ChildTreeNode;
		ChildTreeNode->Previous = TreeNode->ChildLast;
	}
	TreeNode->ChildLast = ChildTreeNode;
	if( TreeNode->ChildFirst == NULL ) {
		TreeNode->ChildFirst = ChildTreeNode;
	}

	//
	// Mark the child as present in the tree.
	//
	ChildTreeNode->Parent = TreeNode;
	ChildTreeNode->IsPresent = AML_TRUE;

	//
	// Update the tree depth of the linked node (how many ancestors it takes to reach this node).
	// Then use the tree node's depth to possibly update the maximum depth of the entire tree (deepest path).
	//
	ChildTreeNode->Depth = ( TreeNode->Depth + ( ( TreeNode->Depth < UINT64_MAX ) ? 1 : 0 ) );
	State->TreeMaxDepth = AML_MAX( State->TreeMaxDepth, ChildTreeNode->Depth );
}

//
// Insert the given namespace node to the hierarchical tree,
// ensures that the entire path along the way to the given node is inserted and present.
//
VOID
AmlNamespaceTreeInsertNode(
	_Inout_ AML_NAMESPACE_STATE* State,
	_Inout_ AML_NAMESPACE_NODE*  Node
	)
{
	SIZE_T              j;
	AML_NAME_STRING     AncestorPath;
	AML_NAMESPACE_NODE* AncestorNode;
	AML_NAMESPACE_NODE* LastAncestorNode;

	//
	// Nothing to do if this node is already present in the tree.
	//
	if( Node->TreeEntry.IsPresent ) {
		return;
	}

	//
	// Start from the root of the node's path and attempt to construct all missing levels,
	// then link the current node to the last level existing of the tree.
	//
	LastAncestorNode = NULL;
	for( j = 0; j <= Node->AbsolutePath.SegmentCount; j++ ) {
		//
		// Search for each level of the path of the node we are inserting
		// ensuring that each level is present in the tree.
		//
		AncestorPath = Node->AbsolutePath;
		AncestorPath.SegmentCount = j;
		if( AmlNamespaceSearch( State, NULL, &AncestorPath, 0, &AncestorNode ) == AML_FALSE ) {
			continue;
		} 

		//
		// If this level of the path isn't present in the tree yet, link it to the last ancestor.
		//
		if( AncestorNode->TreeEntry.IsPresent == AML_FALSE ) {
			if( LastAncestorNode != NULL ) {
				AmlNamespaceTreeLinkChildNode( State, &LastAncestorNode->TreeEntry, &AncestorNode->TreeEntry );
			} else {
				AmlNamespaceTreeLinkChildNode( State, &State->TreeRoot, &AncestorNode->TreeEntry );
			}
		}

		//
		// Update the new last ancestor node, the next child level can link back to the previous ancestor.
		//
		LastAncestorNode = AncestorNode;
	}
}

//
// Build the actual hierarchical namespace tree from the unsorted list of all namespace objects.
// TODO: Allow rebuilding the tree, also allow rebuilding from a specific node.
//
VOID
AmlNamespaceTreeBuild(
	_Inout_ AML_NAMESPACE_STATE* State	
	)
{
	AML_NAMESPACE_NODE* Node;

	//
	// Iterate and insert all namespace nodes in unsorted order.
	//
	State->TreeRoot.Depth = 1;
	State->TreeRoot.IsPresent = AML_TRUE;
	for( Node = State->InOrderNodeHead; Node != NULL; Node = Node->InOrderNext ) {
		AmlNamespaceTreeInsertNode( State, Node );
	}
}
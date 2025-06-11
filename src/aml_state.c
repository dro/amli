#include "aml_state.h"
#include "aml_debug.h"
#include "aml_eval.h"
#include "aml_osi.h"

//
// Iterative namespace tree DFS traversal frame.
// TODO: Move this out of here, make generic iterator helpers in namespace code.
//
typedef struct _AML_NAMESPACE_TREE_DFS_FRAME {
    AML_NAMESPACE_TREE_NODE* Node;
    AML_NAMESPACE_TREE_NODE* NextChild;
} AML_NAMESPACE_TREE_DFS_FRAME;

//
// Initialize the AML decoder/evaluation state.
// Note: The given allocator and host context must exist for the entire lifetime of the state.
//
_Success_( return )
BOOLEAN
AmlStateCreate(
    _Out_ AML_STATE*                  State,
    _In_  AML_ALLOCATOR               Allocator,
    _In_  const AML_STATE_PARAMETERS* Parameters
    )
{
    SIZE_T i;

    //
    // Default initialize fields.
    // Memset is used here to avoid a large copy of the default AML_STATE structure on the stack.
    //
    AML_MEMSET( State, 0, sizeof( *State ) );
    State->PassType        = AML_PASS_TYPE_FULL;
    State->IsIntegerSize64 = Parameters->Use64BitInteger;
    State->Host            = Parameters->Host;

    //
    // Set up the default operation region space access handlers.
    //
    for( i = 0; i < AML_COUNTOF( State->RegionSpaceHandlers ); i++ ) {
        State->RegionSpaceHandlers[ i ].UserRoutine = AmlOperationRegionHandlerDefaultNull;
    }
    State->RegionSpaceHandlers[ AML_REGION_SPACE_TYPE_SYSTEM_IO ].UserRoutine = AmlOperationRegionHandlerDefaultSystemIo;
    State->RegionSpaceHandlers[ AML_REGION_SPACE_TYPE_SYSTEM_MEMORY ].UserRoutine = AmlOperationRegionHandlerDefaultSystemMemory;
    State->RegionSpaceHandlers[ AML_REGION_SPACE_TYPE_PCI_CONFIG ].UserRoutine = AmlOperationRegionHandlerDefaultPciConfig;

    //
    // Initialize decoder permanent lifetime allocator and heap.
    //
    AmlArenaInitialize( &State->Arena, Allocator, ( 4096 * 8 ), 0 );
    AmlHeapInitialize( &State->Heap, &State->Arena );

    //
    // Initialize per-method-call scope arena.
    //
    AmlArenaInitialize( &State->MethodScopeArena, Allocator, 4096, 0 );

    //
    // Initialize namespace state.
    //
    AmlNamespaceStateInitialize( &State->Namespace, Allocator, &State->Heap );

    //
    // Initialize snapshot state stack arena.
    //
    AmlArenaInitialize( &State->StateSnapshotArena, Allocator, 4096, 0 );

    //
    // Initialize root method scope level.
    //
    State->MethodScopeFirst = NULL;
    State->MethodScopeLast  = NULL;
    State->MethodScopeLevel = 0;

    //
    // Set up the DebugObj sentinel object.
    //
    State->DebugObject = ( AML_OBJECT ){ .SuperType = AML_OBJECT_SUPERTYPE_DEBUG, .Type = AML_OBJECT_TYPE_DEBUG };

    //
    // Begin default state snapshot stack level.
    //
    AmlStateSnapshotBegin( State );

    //
    // Begin default method scope root level.
    //
    AmlMethodPushScope( State );
    State->MethodScopeRoot = State->MethodScopeLast;
    return AML_TRUE;
}

//
// Release all underlying state resources.
//
VOID
AmlStateFree(
    _Inout_ _Post_invalid_ AML_STATE* State
    )
{
    //
    // Release all allocated namespace state memory.
    //
    AmlNamespaceStateRelease( &State->Namespace );

    //
    // Release all allocated state memory.
    //
    AmlArenaRelease( &State->StateSnapshotArena );
    AmlArenaRelease( &State->MethodScopeArena );
    AmlArenaRelease( &State->Arena );

    //
    // Zero fields for debugging.
    //
    AML_MEMSET( State, 0, sizeof( *State ) );
}

//
// Create a namespace.
//
_Success_( return )
static
BOOLEAN
AmlCreateNamespace(
    _Inout_ AML_STATE*             State,
    _In_    const AML_NAME_STRING* NameString
    )
{
    AML_OBJECT*         Object;
    AML_NAMESPACE_NODE* Node;

    //
    // Create a scope object.
    //
    if( AmlObjectCreate( &State->Heap, AML_OBJECT_TYPE_SCOPE, &Object ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Create a namespace node for the scope.
    //
    if( AmlStateSnapshotCreateNode( State, NULL, NameString, &Node ) == AML_FALSE ) {
        AmlObjectRelease( Object );
        return AML_FALSE;
    }
    
    //
    // Initialize empty scope object.
    // Empty because the unused name field inside of the scope object will be removed soon.
    //
    Object->u.Scope = ( AML_OBJECT_SCOPE ){ 0 };

    //
    // Linkobject to created node.
    //
    Object->NamespaceNode = Node;
    Node->Object = Object;
    return AML_TRUE;
}

//
// Create all predefined namespaces.
//
_Success_( return )
BOOLEAN
AmlCreatePredefinedNamespaces(
    _Inout_ AML_STATE* State
    )
{
    AML_NAME_PREFIX RootPrefix;
    BOOLEAN         Success;

    //
    // Root namespace prefix bytes.
    //
    RootPrefix = ( AML_NAME_PREFIX ){ .Data = { '\\' }, .Length = 1 };

    //
    // \ - Root namespace.
    //
    Success = AmlCreateNamespace(
        State,
        &( AML_NAME_STRING ){
            .Prefix = RootPrefix,
        }
    );

    //
    // \_GPE - General events in GPE register block.
    //
    Success &= AmlCreateNamespace(
        State,
        &( AML_NAME_STRING ){
            .Prefix       = RootPrefix,
            .Segments     = ( AML_NAME_SEG[] ){ { .Data = { '_', 'G', 'P', 'E' } } },
            .SegmentCount = 1,
        }
    );

    //
    // \_PR - ACPI 1.0 Processor Namespace.
    //
    Success &= AmlCreateNamespace(
        State,
        &( AML_NAME_STRING ){
            .Prefix       = RootPrefix,
            .Segments     = ( AML_NAME_SEG[] ){ { .Data = { '_', 'P', 'R', '_' } } },
            .SegmentCount = 1,
        }
    );

    //
    // \_SB - All Device/Bus Objects are defined under this namespace.
    //
    Success &= AmlCreateNamespace(
        State,
        &( AML_NAME_STRING ){
            .Prefix       = RootPrefix,
            .Segments     = ( AML_NAME_SEG[] ){ { .Data = { '_', 'S', 'B', '_' } } },
            .SegmentCount = 1,
        }
    );

    //
    // \_SI - System indicator objects are defined under this namespace.
    //
    Success &= AmlCreateNamespace(
        State,
        &( AML_NAME_STRING ){
            .Prefix       = RootPrefix,
            .Segments     = ( AML_NAME_SEG[] ){ { .Data = { '_', 'S', 'I', '_' } } },
            .SegmentCount = 1,
        }
    );

    //
    // \_TZ_ - ACPI 1.0 Thermal Zone namespace.
    //
    Success &= AmlCreateNamespace(
        State,
        &( AML_NAME_STRING ){
            .Prefix       = RootPrefix,
            .Segments     = ( AML_NAME_SEG[] ){ { .Data = { '_', 'T', 'Z', '_' } } },
            .SegmentCount = 1,
        }
    );

    return Success;
}

//
// Create a new linked namespace node and object pair of the given type.
// Does not perform any object initialization for the specific type given.
// Does not add any references to the output object or namespace nodes,
// they will both be returned at 1 (general namespace reference amount).
//
_Success_( return )
static
BOOLEAN
AmlCreateNamedObject(
    _Inout_      AML_STATE*             State,
    _In_         const AML_NAME_STRING* PathName,
    _In_         AML_OBJECT_TYPE        ObjectType,
    _Outptr_     AML_OBJECT**           ppObject,
    _Outptr_opt_ AML_NAMESPACE_NODE**   ppNsNode
    )
{
    AML_OBJECT*         Object;
    AML_NAMESPACE_NODE* NsNode;

    //
    // Create a namespace node.
    //
    if( AmlStateSnapshotCreateNode( State, NULL, PathName, &NsNode ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Create an object.
    //
    if( AmlObjectCreate( &State->Heap, ObjectType, &Object ) == AML_FALSE ) {
        AmlNamespaceReleaseNode( &State->Namespace, NsNode );
        return AML_FALSE;
    }

    //
    // Link object to created node.
    //
    Object->NamespaceNode = NsNode;
    NsNode->Object = Object;

    //
    // Return the created object and optionally the namespace node.
    //
    *ppObject = Object;
    if( ppNsNode != NULL ) {
        *ppNsNode = NsNode;
    }

    return AML_TRUE;
}

//
// Create all predefined objects.
//
VOID
AmlCreatePredefinedObjects(
    _Inout_ AML_STATE* State
    )
{
    AML_NAME_PREFIX RootPrefix;
    AML_NAME_STRING Name;
    AML_OBJECT*     Object;

    //
    // Root namespace prefix bytes.
    //
    RootPrefix = ( AML_NAME_PREFIX ){ .Data = { '\\' }, .Length = 1 };

    //
    // \_GL - Global Lock mutex.
    //
    Name = ( AML_NAME_STRING ){
        .Prefix       = RootPrefix,
        .Segments     = ( AML_NAME_SEG[] ){ { .Data = { '_', 'G', 'L', '_' } } },
        .SegmentCount = 1,
    };
    if( AmlCreateNamedObject( State, &Name, AML_OBJECT_TYPE_MUTEX, &Object, NULL ) ) {
        Object->u.Mutex = ( AML_OBJECT_MUTEX ){ .Host = State->Host };
        if( AmlHostMutexCreate( State->Host, &Object->u.Mutex.HostHandle ) ) {
            State->GlobalLock = Object;
            AmlObjectReference( Object );
        } else {
            AmlObjectRelease( Object );
        }
    }

    //
    // \_OS - Name of the operating system.
    // A String containing the operating system name.
    //
    Name = ( AML_NAME_STRING ){
        .Prefix       = RootPrefix,
        .Segments     = ( AML_NAME_SEG[] ){ { .Data = { '_', 'O', 'S', '_' } } },
        .SegmentCount = 1,
    };
    if( AmlCreateNamedObject( State, &Name, AML_OBJECT_TYPE_NAME, &Object, NULL ) ) {
        Object->u.Name.Value = ( AML_DATA ){
            .Type     = AML_DATA_TYPE_STRING,
            .u.String = AmlBufferDataCreateZ( &State->Heap, AML_OS_NAME )
        };
    }

    //
    // \_OSI - Operating System Interface support.
    // This method is used by the system firmware to query OSPM about interfaces and features that are supported by the host operating system.
    //
    Name = ( AML_NAME_STRING ){
        .Prefix       = RootPrefix,
        .Segments     = ( AML_NAME_SEG[] ){ { .Data = { '_', 'O', 'S', 'I' } } },
        .SegmentCount = 1,
    };
    AmlMethodCreateNative( State, &Name, AmlOsiQueryNativeMethod, NULL, 1, 0 );

    //
    // \_REV - Revision of the ACPI specification that is implemented.
    // An Integer representing the revision of the currently executing ACPI implementation.
    //  1. Only ACPI 1 is supported, only 32-bit integers.
    //  2. ACPI 2 or greater is supported. Both 32-bit and 64-bit integers are supported.
    //
    Name = ( AML_NAME_STRING ){
        .Prefix       = RootPrefix,
        .Segments     = ( AML_NAME_SEG[] ){ { .Data = { '_', 'R', 'E', 'V' } } },
        .SegmentCount = 1,
    };
    if( AmlCreateNamedObject( State, &Name, AML_OBJECT_TYPE_NAME, &Object, NULL ) ) {
        Object->u.Name.Value = ( AML_DATA ){
            .Type      = AML_DATA_TYPE_INTEGER,
            .u.Integer = 2
        };
    }
}

//
// Returns AML_TRUE if the given object might be a scoped object that has children.
//
static
BOOLEAN
AmlIsObjectScopeCandidate(
    _In_opt_ const AML_OBJECT* Object
    )
{
    if( Object != NULL ) {
        switch( Object->Type ) {
        case AML_OBJECT_TYPE_NONE:
        case AML_OBJECT_TYPE_SCOPE:
        case AML_OBJECT_TYPE_DEVICE:
        case AML_OBJECT_TYPE_PROCESSOR:
        case AML_OBJECT_TYPE_THERMAL_ZONE:
        case AML_OBJECT_TYPE_POWER_RESOURCE:
            return AML_TRUE;
        default:
            break;
        }
    }

    return AML_FALSE;
}

//
// Call the _INI function for a single device object if allowed by the _STA value.
//
_Success_( return )
static
BOOLEAN
AmlInitializeDevice(
    _Inout_   AML_STATE*          State,
    _Inout_   AML_NAMESPACE_NODE* NsNode,
    _Out_opt_ BOOLEAN*            SkipChildren
    )
{
    UINT32              Status;
    UINT32              ShouldCallInit;
    AML_NAMESPACE_NODE* NsIniNode;

    //
    // Keep exploring the children down this path by default.
    //
    if( SkipChildren != NULL ) {
        *SkipChildren = AML_FALSE;
    }

    //
    // Nothing to do if the device node is not properly initialized with an object.
    //
    if( NsNode->Object == NULL ) {
        return AML_TRUE;
    }

    //
    // Skip this object if it has already had its initializer called.
    //
    if( NsNode->Object->IsInitializedDevice ) {
        return AML_TRUE;
    }

    //
    // Only handle spec-designated _INI object types in this function,
    // special handlers like \_INI and \_SB_._INI must be called unconditionally elsewhere,
    // as they don't require the same considerations as a regular device in regards to _STA behavior.
    //
    switch( NsNode->Object->Type ) {
    case AML_OBJECT_TYPE_DEVICE:
    case AML_OBJECT_TYPE_PROCESSOR:
    case AML_OBJECT_TYPE_THERMAL_ZONE:
        break;
    default:
        return AML_TRUE;
    }

    //
    // Call the device status method, returns information about the connection state of the device.
    // Determines if we should keep trying to process children down this path.
    //
    if( AmlEvalNodeDeviceStatus( State, NsNode, &Status ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Only run _INI on this device if it is present.
    // If the device is not present but is functioning, keep exploring children.
    // If the device is not marked functioning, stop exploring this path entirely.
    //
    ShouldCallInit = ( Status & ( AML_STA_FLAG_PRESENT | AML_STA_FLAG_FUNCTIONING ) );
    ShouldCallInit = ( ShouldCallInit == ( AML_STA_FLAG_PRESENT | AML_STA_FLAG_FUNCTIONING ) );
    if( ShouldCallInit ) {
        NsIniNode = AmlNamespaceChildNodeZ( &State->Namespace, NsNode, "_INI" );
        if( ( NsIniNode != NULL )
            && ( NsIniNode->Object != NULL )
            && ( NsIniNode->Object->Type == AML_OBJECT_TYPE_METHOD ) )
        {
            if( AmlMethodInvoke( State, NsIniNode->Object, 0, NULL, 0, NULL ) == AML_FALSE ) {
                return AML_FALSE;
            }
            AmlHostOnDeviceInitialized( State->Host, NsNode->Object, Status );
            NsNode->Object->IsInitializedDevice = AML_TRUE;
        }
    }

    //
    // If the device wasn't functional, stop exploring this path, don't call initializers for any children.
    //
    if( SkipChildren != NULL ) {
        *SkipChildren = ( ( Status & AML_STA_FLAG_FUNCTIONING ) == 0 );
    }

    return AML_TRUE;
}

//
// Call the _INI methods for all functioning devices that advertise their presence through _STA.
// CallRootInitializers determines if the unconditional root \_INI and \_SB_._INI methods
// should be called to adhere with the typical expected behavior of other interpreters.
//
_Success_( return )
BOOLEAN
AmlInitializeDevices(
    _Inout_ AML_STATE*               State,
    _Inout_ AML_NAMESPACE_TREE_NODE* StartTreeNode,
    _In_    BOOLEAN                  CallRootInitializers
    )
{
    AML_ARENA_SNAPSHOT            TempSnapshot;
    AML_NAMESPACE_TREE_DFS_FRAME* Stack;
    SIZE_T                        StackTail;
    AML_NAMESPACE_TREE_DFS_FRAME* Frame;
    AML_NAMESPACE_NODE*           NsNode;
    AML_NAMESPACE_NODE*           NsIniNode;
    BOOLEAN                       SkipChildren;

    //
    // The namespace tree must have been built for the start node.
    //
    if( StartTreeNode->IsPresent == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Call the root _INI methods if desired by the user (not strictly spec compliant).
    //
    if( CallRootInitializers ) {
        if( AmlNamespaceSearchZ( &State->Namespace, NULL, "\\_INI", 0, &NsIniNode ) ) {
            if( ( NsIniNode->Object != NULL )
                && ( NsIniNode->Object->Type == AML_OBJECT_TYPE_METHOD )
                && ( NsIniNode->Object->IsInitializedDevice == AML_FALSE ) )
            {
                if( AmlMethodInvoke( State, NsIniNode->Object, 0, NULL, 0, NULL ) == AML_FALSE ) {
                    AML_DEBUG_ERROR( State, "Error: Method call failed for special root \\_INI initializer.\n" );
                }
                NsIniNode->Object->IsInitializedDevice = AML_TRUE;
            }
        }
        if( AmlNamespaceSearchZ( &State->Namespace, NULL, "\\_SB_._INI", 0, &NsIniNode ) ) {
            if( ( NsIniNode->Object != NULL )
                && ( NsIniNode->Object->Type == AML_OBJECT_TYPE_METHOD )
                && ( NsIniNode->Object->IsInitializedDevice == AML_FALSE ) )
            {
                if( AmlMethodInvoke( State, NsIniNode->Object, 0, NULL, 0, NULL ) == AML_FALSE ) {
                    AML_DEBUG_ERROR( State, "Error: Method call failed for special root \\_SB_._INI initializer.\n" );
                }
                NsIniNode->Object->IsInitializedDevice = AML_TRUE;
            }
        }
    }

    //
    // Attempt to allocate a pending visit stack for the traversal.
    // There is no visiting order requirement between siblings, only that a parent must be visited before its children.
    //
    TempSnapshot = AmlArenaSnapshot( &State->Namespace.TempArena );
    Stack = AmlArenaAllocate( &State->Namespace.TempArena, ( sizeof( Stack[ 0 ] ) * State->Namespace.TreeMaxDepth ) );
    if( Stack == NULL ) {
        return AML_FALSE;
    }

    //
    // Push the initial root node to the visit stack.
    //
    StackTail = 0;
    Stack[ StackTail++ ] = ( AML_NAMESPACE_TREE_DFS_FRAME ){
        .Node      = StartTreeNode,
        .NextChild = StartTreeNode->ChildFirst
    };

    //
    // Visit nodes until we have completely ran out of pushed stack items.
    //
    while( StackTail != 0 ) {
        //
        // Lookup the next frame to process on the stack.
        //
        Frame = &Stack[ StackTail - 1 ];

        //
        // If this is the first time processing the node,
        // and we aren't here because we are processing a child of it,
        // perform the actual processing for the node.
        //
        SkipChildren = AML_FALSE;
        if( ( Frame->Node != &State->Namespace.TreeRoot )
            && ( Frame->NextChild == Frame->Node->ChildFirst ) )
        {
            NsNode = AML_CONTAINING_RECORD( Frame->Node, AML_NAMESPACE_NODE, TreeEntry );
            if( NsNode->Object != NULL ) {
                if( AmlInitializeDevice( State, NsNode, &SkipChildren ) == AML_FALSE ) {
                    AML_DEBUG_ERROR( State, "Error: Device initialization failed for node: \"" );
                    AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_ERROR, &NsNode->AbsolutePath );
                    AML_DEBUG_ERROR( State, "\"\n" );
                }
            }
        }

        //
        // If we want to end processing of children down the current path,
        // or if there are no children left to process for the current frame,
        // pop the frame from the stack.
        //
        if( ( Frame->NextChild == NULL ) || ( SkipChildren != AML_FALSE ) ) {
            StackTail -= 1;
            continue;
        }

        //
        // This should never happen unless the tree is somehow out of date or invalid (or the traversal algorithm is broken).
        //
        if( StackTail >= State->Namespace.TreeMaxDepth ) {
            AML_DEBUG_PANIC( State, "Fatal: Namespace node visit stack size exceeded maximum tree depth!" );
            return AML_FALSE;
        }

        //
        // If there is still a child to process for the current frame, queue it up to be processed.
        //
        Stack[ StackTail++ ] = ( AML_NAMESPACE_TREE_DFS_FRAME ){
            .Node      = Frame->NextChild,
            .NextChild = Frame->NextChild->ChildFirst
        };

        //
        // Advance the child iterator for the current parent level.
        //
        Frame->NextChild = Frame->NextChild->Next;
    }

    //
    // Rollback temporary visit stack allocations.
    //
    AmlArenaSnapshotRollback( &State->Namespace.TempArena, &TempSnapshot );
    return AML_TRUE;
}

//
// Complete the initial load, build the hierarchical namespace tree, initialize all device objects.
//
_Success_( return )
BOOLEAN
AmlCompleteInitialLoad(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    InitializeDevices
    )
{
    UINT8                           i;
    AML_REGION_ACCESS_REGISTRATION* Handler;

    //
    // Build hierarchical tree from the unsorted list of all namespace nodes.
    //
    AmlNamespaceTreeBuild( &State->Namespace );

    //
    // Mark the initial load stage as complete, all table/code loading proceeding this point
    // will require reconstruction of the tree, and rebroadcast of registered region space types.
    //
    State->IsInitialLoadComplete = AML_TRUE;

    //
    // Call any region space registration handlers that have yet to be informed of a state update.
    // This is only necessary to handle any state updates performed before completion of the initial load.
    // TODO: Do this in one pass instead of traversing the tree for every single handler type!
    //
    for( i = 0; i < AML_COUNTOF( State->RegionSpaceHandlers ); i++ ) {
        Handler = &State->RegionSpaceHandlers[ i ];
        if( Handler->BroadcastPending == AML_FALSE ) {
            continue;
        }
        AmlBroadcastRegionSpaceStateUpdate( State, i, Handler->EnableState, AML_TRUE );
        Handler->BroadcastPending = AML_FALSE;
    }

    //
    // Query the status of all devices present in the namespace and attempt to call their initializers if present.
    //
    if( InitializeDevices ) {
        if( AmlInitializeDevices( State, &State->Namespace.TreeRoot, AML_TRUE ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    return AML_TRUE;
}

//
// Call all device _REG handlers for the given region space type (if valid for _REG).
//
_Success_( return )
BOOLEAN
AmlBroadcastRegionSpaceStateUpdate(
    _Inout_ AML_STATE* State,
    _In_    UINT8      RegionSpaceType,
    _In_    BOOLEAN    EnableState,
    _In_    BOOLEAN    IsTrueUpdate
    )
{
    AML_ARENA_SNAPSHOT            TempSnapshot;
    AML_NAMESPACE_TREE_DFS_FRAME* Stack;
    SIZE_T                        StackTail;
    AML_NAMESPACE_TREE_DFS_FRAME* Frame;
    AML_DATA                      RegArgs[ 2 ];
    BOOLEAN                       SkipObject;
    AML_NAMESPACE_NODE*           NsNode;
    AML_NAMESPACE_NODE*           NsRegNode;

    //
    // If this is a region space type that isn't supported by _REG,
    // due to uncoditionally needing to be supported, just ignore it.
    //
    switch( RegionSpaceType ) {
    case AML_REGION_SPACE_TYPE_SYSTEM_IO:
    case AML_REGION_SPACE_TYPE_SYSTEM_MEMORY:
    case AML_REGION_SPACE_TYPE_PCI_CONFIG:
        return AML_TRUE;
    }

    //
    // The namespace tree must have been built.
    //
    if( State->Namespace.TreeRoot.IsPresent == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Attempt to allocate a pending visit stack for the traversal.
    // There is no visiting order requirement between siblings, only that a parent must be visited before its children.
    //
    TempSnapshot = AmlArenaSnapshot( &State->Namespace.TempArena );
    Stack = AmlArenaAllocate( &State->Namespace.TempArena, ( sizeof( Stack[ 0 ] ) * State->Namespace.TreeMaxDepth ) );
    if( Stack == NULL ) {
        return AML_FALSE;
    }

    //
    // Set up the arguments passed to all invoked _REG functions.
    //
    EnableState = ( EnableState ? 1 : 0 );
    RegArgs[ 0 ] = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = RegionSpaceType };
    RegArgs[ 1 ] = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = EnableState };

    //
    // Push the initial root node to the visit stack.
    //
    StackTail = 0;
    Stack[ StackTail++ ] = ( AML_NAMESPACE_TREE_DFS_FRAME ){
        .Node      = &State->Namespace.TreeRoot,
        .NextChild = State->Namespace.TreeRoot.ChildFirst
    };

    //
    // Visit nodes until we have completely ran out of pushed stack items.
    //
    while( StackTail != 0 ) {
        //
        // Lookup the next frame to process on the stack.
        //
        Frame = &Stack[ StackTail - 1 ];

        //
        // If this is the first time processing the node,
        // and we aren't here because we are processing a child of it,
        // perform the actual processing for the node.
        //
        if( ( Frame->Node != &State->Namespace.TreeRoot )
            && ( Frame->NextChild == Frame->Node->ChildFirst ) )
        {
            NsNode = AML_CONTAINING_RECORD( Frame->Node, AML_NAMESPACE_NODE, TreeEntry );
            if( NsNode->Object != NULL ) {
                //
                // If the _REG method has already been called, and we are just re-broadcasting states
                // to dynamically loaded code, ignore any objects that have already had their _REG
                // called for the given space type.
                //
                SkipObject = AML_FALSE;
                if( IsTrueUpdate == AML_FALSE ) {
                    _Static_assert(
                        AML_MAX_SPEC_REGION_SPACE_TYPE_COUNT <= 32,
                        "Exceeded maximum region space bitmap size."
                    );
                    SkipObject = ( NsNode->Object->RegCallBitmap & ( 1ul << RegionSpaceType ) );
                }

                //
                // If this object has any children, check for a _REG method to invoke.
                //
                if( SkipObject == AML_FALSE ) {
                    if( AmlIsObjectScopeCandidate( NsNode->Object )
                        || ( NsNode->TreeEntry.ChildFirst != NULL ) )
                    {
                        NsRegNode = AmlNamespaceChildNodeZ( &State->Namespace, NsNode, "_REG" );
                        if( ( NsRegNode != NULL )
                            && ( NsRegNode->Object != NULL )
                            && ( NsRegNode->Object->Type == AML_OBJECT_TYPE_METHOD )
                            && ( NsRegNode->Object->u.Method.ArgumentCount == 2 ) )
                        {
                            if( AmlMethodInvoke( State, NsRegNode->Object, 0, RegArgs, AML_COUNTOF( RegArgs ), NULL ) == AML_FALSE ) {
                                AML_DEBUG_ERROR( State, "Error: Failed to invoke _REG method for object \"" );
                                AmlDebugPrintNameString( State, AML_DEBUG_LEVEL_ERROR, &NsRegNode->AbsolutePath );
                                AML_DEBUG_ERROR( State, "\"\n" );
                            }
                            NsNode->Object->RegCallBitmap |= ( 1ul << RegionSpaceType );
                        }
                    }
                }
            }
        }

        //
        // If there are no children left to process for the current frame, pop the frame from the stack.
        //
        if( Frame->NextChild == NULL ) {
            StackTail -= 1;
            continue;
        }

        //
        // This should never happen unless the tree is somehow out of date or invalid (or the traversal algorithm is broken).
        //
        if( StackTail >= State->Namespace.TreeMaxDepth ) {
            AML_DEBUG_PANIC( State, "Fatal: Namespace node visit stack size exceeded maximum tree depth!" );
            return AML_FALSE;
        }

        //
        // If there is still a child to process for the current frame, queue it up to be processed.
        //
        Stack[ StackTail++ ] = ( AML_NAMESPACE_TREE_DFS_FRAME ){
            .Node      = Frame->NextChild,
            .NextChild = Frame->NextChild->ChildFirst
        };

        //
        // Advance the child iterator for the current parent level.
        //
        Frame->NextChild = Frame->NextChild->Next;
    }

    //
    // Rollback temporary visit stack allocations.
    //
    AmlArenaSnapshotRollback( &State->Namespace.TempArena, &TempSnapshot );
    return AML_TRUE;
}

//
// Visit all namespace nodes of the given object type in DFS traversal order.
// If VisitObjectType is AML_OBJECT_TYPE_NONE, any object type is visited.
//
_Success_( return )
BOOLEAN
AmlIterateNamespaceObjects(
    _Inout_     AML_STATE*                     State,
    _Inout_opt_ AML_NAMESPACE_TREE_NODE*       StartTreeNode,
    _In_        AML_ITERATOR_NAMESPACE_ROUTINE UserRoutine,
    _In_opt_    VOID*                          UserContext,
    _In_opt_    AML_OBJECT_TYPE                VisitObjectType
    )
{
    AML_ARENA_SNAPSHOT            TempSnapshot;
    AML_NAMESPACE_TREE_DFS_FRAME* Stack;
    SIZE_T                        StackTail;
    AML_NAMESPACE_TREE_DFS_FRAME* Frame;
    AML_NAMESPACE_NODE*           NsNode;
    AML_ITERATOR_ACTION           Action;

    //
    // The namespace tree must have been built for the start node.
    //
    StartTreeNode = ( ( StartTreeNode == NULL ) ? &State->Namespace.TreeRoot : StartTreeNode );
    if( StartTreeNode->IsPresent == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Attempt to allocate a pending visit stack for the traversal.
    // There is no visiting order requirement between siblings, only that a parent must be visited before its children.
    //
    TempSnapshot = AmlArenaSnapshot( &State->Namespace.TempArena );
    Stack = AmlArenaAllocate( &State->Namespace.TempArena, ( sizeof( Stack[ 0 ] ) * State->Namespace.TreeMaxDepth ) );
    if( Stack == NULL ) {
        return AML_FALSE;
    }

    //
    // Push the initial root node to the visit stack.
    //
    StackTail = 0;
    Stack[ StackTail++ ] = ( AML_NAMESPACE_TREE_DFS_FRAME ){
        .Node      = StartTreeNode,
        .NextChild = StartTreeNode->ChildFirst
    };

    //
    // Visit nodes until we have completely ran out of pushed stack items.
    //
    Action = AML_ITERATOR_ACTION_CONTINUE;
    while( StackTail != 0 ) {
        //
        // Lookup the next frame to process on the stack.
        //
        Frame = &Stack[ StackTail - 1 ];

        //
        // If this is the first time processing the node,
        // and we aren't here because we are processing a child of it,
        // perform the actual processing for the node.
        //
        Action = AML_ITERATOR_ACTION_CONTINUE;
        if( ( Frame->Node != &State->Namespace.TreeRoot )
            && ( Frame->NextChild == Frame->Node->ChildFirst ) )
        {
            NsNode = AML_CONTAINING_RECORD( Frame->Node, AML_NAMESPACE_NODE, TreeEntry );
            if( NsNode->Object != NULL ) {
                if( ( VisitObjectType == AML_OBJECT_TYPE_NONE ) || ( NsNode->Object->Type == VisitObjectType ) ) {
                    Action = UserRoutine( UserContext, State, NsNode );
                    if( ( Action == AML_ITERATOR_ACTION_STOP ) || ( Action == AML_ITERATOR_ACTION_ERROR ) ) {
                        break;
                    }
                }
            }
        }

        //
        // If we want to end processing of children down the current path,
        // or if there are no children left to process for the current frame,
        // pop the frame from the stack.
        //
        if( ( Frame->NextChild == NULL ) || ( Action == AML_ITERATOR_ACTION_SKIP ) ) {
            StackTail -= 1;
            continue;
        }

        //
        // This should never happen unless the tree is somehow out of date or invalid (or the traversal algorithm is broken).
        //
        if( StackTail >= State->Namespace.TreeMaxDepth ) {
            AML_DEBUG_PANIC( State, "Fatal: Namespace node visit stack size exceeded maximum tree depth!" );
            return AML_FALSE;
        }

        //
        // If there is still a child to process for the current frame, queue it up to be processed.
        //
        Stack[ StackTail++ ] = ( AML_NAMESPACE_TREE_DFS_FRAME ){
            .Node      = Frame->NextChild,
            .NextChild = Frame->NextChild->ChildFirst
        };

        //
        // Advance the child iterator for the current parent level.
        //
        Frame->NextChild = Frame->NextChild->Next;
    }

    //
    // Rollback temporary visit stack allocations.
    //
    AmlArenaSnapshotRollback( &State->Namespace.TempArena, &TempSnapshot );
    return ( Action != AML_ITERATOR_ACTION_ERROR );
}

//
// Attempt to register a region-space access handler.
// Registering a handler for a region-space type that has already had a handler register
// will overwrite the existing handler, and the caller must take their precaution to free
// any resources associated with the old registration's user data.
// Routine remain valid along with the given UserContext until the end of the AML state usage.
//
_Success_( return )
BOOLEAN
AmlRegisterRegionSpaceAccessHandler(
    _Inout_  AML_STATE*                State,
    _In_     UINT8                     RegionSpaceType,
    _In_opt_ AML_REGION_ACCESS_ROUTINE UserRoutine,
    _In_opt_ VOID*                     UserContext,
    _In_     BOOLEAN                   CallRegMethods
    )
{
    //
    // Currently only allow the spec defined region spaces.
    //
    if( RegionSpaceType >= AML_COUNTOF( State->RegionSpaceHandlers ) ) {
        return AML_FALSE;
    }

    //
    // Register the new access handler.
    // Must remain valid along with the given UserContext until the end of the AML state usage.
    //
    State->RegionSpaceHandlers[ RegionSpaceType ] = ( AML_REGION_ACCESS_REGISTRATION ){
        .UserContext = UserContext,
        .UserRoutine = ( ( UserRoutine != NULL ) ? UserRoutine : AmlOperationRegionHandlerDefaultNull ),
        .EnableState = ( UserRoutine != NULL )
    };

    //
    // If the caller is deferring calling of the reg methods, or we have yet to complete initialization,
    // the _REG broadcast for this handler needs to be marked as pending, so that we can ensure that
    // all handler updates have been broadcast to all _REG methods at the end of complete initialization!
    //
    if( ( CallRegMethods == AML_FALSE ) || ( State->IsInitialLoadComplete == AML_FALSE ) ) {
        State->RegionSpaceHandlers[ RegionSpaceType ].BroadcastPending = AML_TRUE;
    }

    //
    // Broadcast the change of support for the region space type to all _REG methods.
    // For types that do not require _REG, the broadcast request will be ignored.
    //
    if( CallRegMethods ) {
        if( AmlBroadcastRegionSpaceStateUpdate( State, RegionSpaceType, ( UserRoutine != NULL ), AML_TRUE ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    return AML_TRUE;
}

//
// Lookup the access handler registration entry for the given region space tyhpe.
//
_Success_( return != NULL )
AML_REGION_ACCESS_REGISTRATION*
AmlLookupRegionSpaceAccessHandler(
    _In_ AML_STATE* State,
    _In_ UINT8      RegionSpaceType
    )
{
    if( RegionSpaceType >= AML_COUNTOF( State->RegionSpaceHandlers ) ) {
        return NULL;
    }
    return &State->RegionSpaceHandlers[ RegionSpaceType ];
}
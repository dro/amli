#pragma once

#include "aml_platform.h"
#include "aml_data.h"
#include "aml_arena.h"
#include "aml_object.h"

//
// Seed used for namespace hashtables.
//
#define AML_NAMESPACE_HASH_SEED 'AmlH'

//
// AML namespace tree node.
// Constructed after full evaluation of the namestate.
//
typedef struct _AML_NAMESPACE_TREE_NODE {
    //
    // Links to the previous and next children of the parent of this node.
    //
    struct _AML_NAMESPACE_TREE_NODE* Previous;
    struct _AML_NAMESPACE_TREE_NODE* Next;

    //
    // Links to the first and last children of this node.
    //
    struct _AML_NAMESPACE_TREE_NODE* ChildFirst;
    struct _AML_NAMESPACE_TREE_NODE* ChildLast;

    //
    // Links to the parent of this node.
    //
    struct _AML_NAMESPACE_TREE_NODE* Parent;

    //
    // The depth of this node in the tree (how many ancestors it has).
    //
    UINT64 Depth;

    //
    // Set once the node has actually been inserted to the tree in the right location.
    //
    BOOLEAN IsPresent : 1;
} AML_NAMESPACE_TREE_NODE;

//
// AML namespace/object node.
//
typedef struct _AML_NAMESPACE_NODE {
    //
    // Tree node, only valid once the tree building post-pass is done.
    //
    AML_NAMESPACE_TREE_NODE TreeEntry;

    //
    // Allows the node to be referenced as part of a state snapshot.
    //
    struct _AML_STATE_SNAPSHOT_ITEM* StateItem;

    //
    // Reference counter, some namespace nodes are temporary (method-scoped nodes).
    //
    UINT64 ReferenceCount;

    //
    // FullPath hash-table bucket links.
    //
    struct _AML_NAMESPACE_NODE* BucketNext;
    struct _AML_NAMESPACE_NODE* BucketPrevious;

    //
    // Evaluation-order list links.
    //
    struct _AML_NAMESPACE_NODE* InOrderNext;
    struct _AML_NAMESPACE_NODE* InOrderPrevious;

    //
    // Temporary scope node links.
    // Only used if the parent scope is temporary.
    //
    struct _AML_NAMESPACE_NODE* TempNext;
    struct _AML_NAMESPACE_NODE* TempPrevious;

    //
    // The absolute full path to the node, multiple name segments, including the final local node name.
    //
    AML_NAME_STRING AbsolutePath;
    UINT32          AbsolutePathHash;

    //
    // The local node name, the last name segment of the path.
    //
    AML_NAME_SEG LocalName;

    //
    // The underlying object that this node was created for.
    //
    struct _AML_OBJECT* Object;

    //
    // The parent namespace state that this node belongs to.
    //
    struct _AML_NAMESPACE_STATE* ParentState;

    //
    // Flags propagated to the node from its parent scope (for example, if it is temporary).
    //
    UINT                         ScopeFlags;
    struct _AML_NAMESPACE_SCOPE* TempScope;

    //
    // Indicates if this namespace node was pre-parsed as a result of the namespace pass.
    //
    BOOLEAN IsPreParsed : 1;

    //
    // Indicates if this namespace node was actually evaluated as a result of the full pass.
    //
    BOOLEAN IsEvaluated : 1;
} AML_NAMESPACE_NODE;

//
// Namespace name scope stack entry.
//
typedef struct _AML_NAMESPACE_SCOPE {
    //
    // Scope stack level index.
    //
    SIZE_T LevelIndex;

    //
    // Arena snapshot to rollback scope stack arena with when popping the scope.
    //
    AML_ARENA_SNAPSHOT ArenaSnapshot;

    //
    // Links to the parent and child scopes of this level.
    // We currently only link to one single child, it is intended for these
    // scope structures to only exist for the current active scope path, so there
    // are no siblings pointed to, as only one scope level within the parent scope
    // is able to be active at a time.
    //
    struct _AML_NAMESPACE_SCOPE* Parent;
    struct _AML_NAMESPACE_SCOPE* Child;

    //
    // Absolute path to this namespace, starting from root (\), including the name of itself.
    //
    AML_NAME_STRING AbsolutePath;
    UINT32          AbsolutePathHash;

    //
    // Scope behavior flags (AML_SCOPE_FLAG).
    //
    UINT Flags;

    //
    // If this is a temporary scope level, we need to keep track of all namespace nodes allocated in it.
    // All temporary namespace nodes will be removed and released upon exit of this scope.
    //
    struct _AML_NAMESPACE_NODE* TempNodeHead;
    struct _AML_NAMESPACE_NODE* TempNodeTail;
} AML_NAMESPACE_SCOPE;

//
// Namespace object hash table bucket count.
//
#define AML_NAMESPACE_PATH_MAP_BUCKET_COUNT 1024

//
// Namespace scope hash table bucket count.
//
#define AML_NAMESPACE_SCOPE_MAP_BUCKET_COUNT 128

//
// Global namespace state.
//
typedef struct _AML_NAMESPACE_STATE {
    //
    // Parent heap used for dynamic allocations.
    //
    AML_HEAP* Heap;

    //
    // Permanent node arena, currently nodes are never released.
    //
    AML_ARENA PermanentArena;

    //
    // Internal temporary arena.
    //
    AML_ARENA TempArena;

    //
    // Namespace absolute/full-path node hash-table.
    //
    struct _AML_NAMESPACE_NODE* AbsolutePathMap[ AML_NAMESPACE_PATH_MAP_BUCKET_COUNT ];

    //
    // Empty sentinel object used by namespace nodes that have yet to be pointed to an object.
    //
    AML_OBJECT NilObject;

    //
    // Namespace root scope, shouldn't usually be accessed directly.
    //
    AML_NAMESPACE_SCOPE ScopeRoot;

    //
    // Currently active namespace scopes.
    //
    AML_ARENA            ScopeArena;
    AML_NAMESPACE_SCOPE* ScopeFirst;
    AML_NAMESPACE_SCOPE* ScopeLast;

    //
    // Tree root of the full namespace tree built after evaluation.
    //
    AML_NAMESPACE_TREE_NODE TreeRoot;
    UINT64                  TreeMaxDepth;

    //
    // Evaluation-order node list, used to build the final tree in order of how objects are laid out in code.
    //
    AML_NAMESPACE_NODE* InOrderNodeHead;
    AML_NAMESPACE_NODE* InOrderNodeTail;
} AML_NAMESPACE_STATE;

//
// SearchFlags used by AmlNamespaceSearch.
//
#define AML_SEARCH_FLAG_NONE                0
#define AML_SEARCH_FLAG_NO_ALIAS_RESOLUTION (1 << 0)
#define AML_SEARCH_FLAG_FOLLOW_REFERENCE    (1 << 1)
#define AML_SEARCH_FLAG_NAME_CREATION       (1 << 2)

//
// Flags used by AmlNamespacePushScope.
//
#define AML_SCOPE_FLAG_NONE      0
#define AML_SCOPE_FLAG_TEMPORARY (1 << 0) /* Nodes in this scope are temporary. */
#define AML_SCOPE_FLAG_SWITCH    (1 << 1) /* Switching to an existing scope, should not inherit current parent properties. */
#define AML_SCOPE_FLAG_BOUNDARY  (1 << 2)

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
    );

//
// Release all underlying namespace state resources.
// TODO: Remove this once namespace allocates ontop of a parent arena.
//
VOID
AmlNamespaceStateRelease(
    _Inout_ _Post_invalid_ AML_NAMESPACE_STATE* State
    );

//
// Fully expand and compare two AML_NAME_STRING values.
//
BOOLEAN
AmlNamespaceCompareNameString(
    _Inout_  AML_NAMESPACE_STATE*       State,
    _In_opt_ const AML_NAMESPACE_SCOPE* ActiveScope,
    _In_     const AML_NAME_STRING*     String1,
    _In_     const AML_NAME_STRING*     String2
    );

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
    );

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
    );

//
// Attempt to find the parent node of a namespace node.
//
_Success_( return != NULL )
AML_NAMESPACE_NODE*
AmlNamespaceParentNode(
    _In_ AML_NAMESPACE_STATE* State,
    _In_ AML_NAMESPACE_NODE*  Node
    );

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
    );

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
    );

//
// Search for a child of the given node with the ChildName.
//
_Success_( return != NULL )
AML_NAMESPACE_NODE*
AmlNamespaceChildNode(
    _In_ AML_NAMESPACE_STATE*      State,
    _In_ const AML_NAMESPACE_NODE* Node,
    _In_ const AML_NAME_STRING*    ChildName
    );

//
// Search for a child of the given node with the null-terminated ChildName path string.
//
_Success_( return != NULL )
AML_NAMESPACE_NODE*
AmlNamespaceChildNodeZ(
    _In_   AML_NAMESPACE_STATE*      State,
    _In_   const AML_NAMESPACE_NODE* Node,
    _In_z_ const CHAR*               NameString
    );

//
// Push new namespace scope level.
//
_Success_( return )
BOOLEAN
AmlNamespacePushScope(
    _Inout_ AML_NAMESPACE_STATE*   State,
    _In_    const AML_NAME_STRING* Name,
    _In_    UINT                   ScopeFlags
    );

//
// Pop last namespace scope level.
//
_Success_( return )
BOOLEAN
AmlNamespacePopScope(
    _Inout_ AML_NAMESPACE_STATE* State
    );

//
// Create and link a new namespace node (does not initialize the node's object/value).
//
_Success_( return )
BOOLEAN
AmlNamespaceCreateNode(
    _Inout_     AML_NAMESPACE_STATE*   State,
    _Inout_opt_ AML_NAMESPACE_SCOPE*   ActiveScope,
    _In_        const AML_NAME_STRING* Name,
    _Outptr_    AML_NAMESPACE_NODE**   ppCreatedNode
    );

//
// Release a reference to a namespace node, and free resources if this was the last held reference.
//
VOID
AmlNamespaceReleaseNode(
    _Inout_                AML_NAMESPACE_STATE* State,
    _Inout_ _Post_invalid_ AML_NAMESPACE_NODE*  Node
    );

//
// Unlink the given node from the tree, transplanting its children to the parent.
//
VOID
AmlNamespaceTreeUnlinkNode(
    _Inout_ AML_NAMESPACE_STATE*     State,
    _Inout_ AML_NAMESPACE_TREE_NODE* Node
    );

//
// Link the given node as a child to the parent tree node.
// The node being inserted as a child should not be linked to anything.
//
VOID
AmlNamespaceTreeLinkChildNode(
    _Inout_ AML_NAMESPACE_STATE*     State,
    _Inout_ AML_NAMESPACE_TREE_NODE* TreeNode,
    _Inout_ AML_NAMESPACE_TREE_NODE* ChildTreeNode
    );

//
// Insert the given namespace node to the hierarchical tree,
// ensures that the entire path along the way to the given node is inserted and present.
//
VOID
AmlNamespaceTreeInsertNode(
    _Inout_ AML_NAMESPACE_STATE* State,
    _Inout_ AML_NAMESPACE_NODE*  Node
    );

//
// Build the actual hierarchical namespace tree from the unsorted list of all namespace objects.
//
VOID
AmlNamespaceTreeBuild(
    _Inout_ AML_NAMESPACE_STATE* State	
    );
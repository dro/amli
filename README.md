# AMLI [![CI](https://github.com/dro/amli/actions/workflows/main.yml/badge.svg)](https://github.com/dro/amli/actions/workflows/main.yml)

AMLI is a lightweight feature-complete ACPI 6.5 AML interpreter.
The code is designed for hypervisors or OSes that require AML evaluation support, but without an entire ACPI driver (or wish to implement their own).
The project is in active development, any errors should be reported via issues, ideally alongside a copy of the AML bytecode/tables that trigger the issue.

---

## Features
- Implements all ACPI 6.5 defined opcodes/instructions (and deprecated ones like ProcessorOp).
- Internal memory management using arenas and a custom heap, users only require a page-granularity allocator.
- Support for BufferAcc region types.
- Support for devices connected over PCI-to-PCI bridges.
- As spec-compliant as possible without knowingly breaking support for real-world code.
- Written to be easy to follow along as a reference implementation for instructions.
- Zero dependencies (including LibC) other than a few standard freestanding headers for base integer types and values.
- GlobalLock protected event and field access.
- All platform-specific code is handled by the host (including GlobalLock interaction, table access).
- Lazy mapping of entire MMIO operation regions upon first access (no map/unmap for each field access).
- Helpers for device initialization, PCI information.

## Limitations
- Currently a recursive implementation, users should take care to properly have a guard-page protected stack (and possibly grow the stack).
- Single threaded (the complexity was not yet deemed worth it).

## Building as a dependency
Meson is currently the only actual build system supported, 
add amli as a subproject and add the "src" variable to your build source files, 
and the "inc" variable to your include directories.
For all other build systems, just add all of the files in the src directory to your build.

## Building examples/test cases
Examples and test cases will be built when invoking building/compiling the meson.build file directly, and not as a subproject.
The main runtest example can be executed like so:
`runtest <path_to_dsdt_or_ssdt>`


## Implementing
All functions within aml_host.h should be implemented by the host environment/user.
An AML_ALLOCATOR interface should be implemented to be used as the backend allocator, it is only ever used to back internal allocators, and can be a very simplistic page-granularity allocator.
Some currently may be stubbed out and not used, such as mutex and event support, due to lack of multithreaded support.
Most of the initialization follows the typical ACPI initialization process, but is abstracted away if desired.
After creating the eval state, the user should create predefined namespaces and objects.
Once all predefined state has been created, the DSDT should be executed first using `AmlEvalLoadedTableCode`, followed by all SSDTs.
Once all tables have been loaded, the user can call `AmlCompleteInitialLoad` to finalize the loading process, this will build the hierarchical namespace tree, broadcast any pending region-space handlers (pending _REG invocations), and optionally initialize all applicable devices in the namespace (_STA, _INI).
The host will be informed of devices that have been successfully initialized will be through `AmlHostOnDeviceInitialized`.
For more information, see the runtest example application.

```c
//
// Setup the evaluation state and create all predefined namespace nodes.
//
if( AmlStateCreate( &State, HostBackendAllocator, CreationParameters ) == AML_FALSE ) {
    return AML_FALSE;
}
AmlCreatePredefinedNamespaces( &State );
AmlCreatePredefinedObjects( &State );

//
// Load DSDT and all SSDTs.
//
if( AmlEvalLoadedTableCode( &State, DsdtCode, DsdtCodeSize, NULL ) == AML_FALSE ) {
    AmlStateFree( &State );
    return AML_FALSE;
}

//
// Finalize the load, build hierarchical namespace tree, broadcast pending _REGs, perform device initialization (_STA, _INI).
//
AmlCompleteInitialLoad( &State, AML_TRUE );
```

## Build configuration

All platform-specific intrinsics/helpers defined in `aml_platform.h` can be overridden. 
Other than these, some special preprocessor defines exist to control the build and certain features at compile-time.

### `AML_BUILD_HOST_PLATFORM_HEADER`
Causes `aml_platform.h` to include a user-supplied `aml_host_platform.h`, allowing the user to override any desired platform definitions.

### `AML_BUILD_DEBUG_LEVEL`
Change the minimum debug logging verbosity level at compile-time. Possible values:
- `AML_DEBUG_LEVEL_TRACE`
- `AML_DEBUG_LEVEL_INFO`
- `AML_DEBUG_LEVEL_WARNING`
- `AML_DEBUG_LEVEL_ERROR`
- `AML_DEBUG_LEVEL_FATAL`

### `AML_BUILD_OVERRIDE_ARENA`
Allows the user to integrate their own arena allocator. Defines away the internal arena implementation, the user should implement all functions defined in `aml_arena.h`.

### `AML_BUILD_OVERRIDE_HASH`
Allows the user to integrate their own hashing function, the user must implement `AmlHashKey32` defined in `aml_hash.h`.

### `AML_BUILD_OVERRIDE_INTEGER_TYPES`
Allows the user to override the base integer types used in `aml_platform.h`

### `AML_BUILD_OVERRIDE_STRING_CONV`
Allows the user to integrate their own string conversion functions, the user must implement all functions defined in `aml_string_conv.h`.

### `AML_BUILD_NO_SNAPSHOT_ITEMS`
Disables snapshot rollback of state items, can be useful to help debugging in some cases.

### `AML_BUILD_MAX_RECURSION_DEPTH`
Specifies the maximum evaluation and decoder recursion depth before producing an error.

### `AML_BUILD_MAX_LOOP_ITERATIONS`
Specifies the maximum iterations in a while loop before producing an error (presumably due to an infinite loop).

### `AML_BUILD_FUZZER`
Enable libfuzzer interfaces and remove debug prints.


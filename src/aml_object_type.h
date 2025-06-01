#pragma once

#include "aml_platform.h"

//
// (NameSpaceModifierObj | NamedObj) type enum.
//
typedef enum _AML_OBJECT_TYPE {
	AML_OBJECT_TYPE_NONE = 0,

	//
	// Misc named object types.
	//
	AML_OBJECT_TYPE_BUFFER_FIELD,
	AML_OBJECT_TYPE_FIELD,
	AML_OBJECT_TYPE_BANK_FIELD,
	AML_OBJECT_TYPE_INDEX_FIELD,
	AML_OBJECT_TYPE_OPERATION_REGION,
	AML_OBJECT_TYPE_DATA_REGION,
	AML_OBJECT_TYPE_METHOD,
	AML_OBJECT_TYPE_EVENT,
	AML_OBJECT_TYPE_MUTEX,
	AML_OBJECT_TYPE_DEBUG,

	//
	// Objects with associated namespace scope.
	// 	- A predefined scope such as: (root), _SB, GPE, _PR, _TZ, etc.
	//  - Device
	//  - Processor
	//  - Thermal Zone
	//  - Power Resource
	//
	AML_OBJECT_TYPE_SCOPE,
	AML_OBJECT_TYPE_DEVICE,
	AML_OBJECT_TYPE_PROCESSOR,
	AML_OBJECT_TYPE_THERMAL_ZONE,
	AML_OBJECT_TYPE_POWER_RESOURCE,

	//
	// Namespace modifier object types.
	//
	AML_OBJECT_TYPE_ALIAS,
	AML_OBJECT_TYPE_NAME,
} AML_OBJECT_TYPE;
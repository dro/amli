#pragma once

#include "aml_platform.h"

//
// APCI AML level 1 (single-byte) opcode list.
//
#define AML_L1_ZERO_OP					  0x00 /* ZeroOp             */
#define AML_L1_ONE_OP					  0x01 /* OneOp              */
#define AML_L1_ALIAS_OP					  0x06 /* AliasOp            */
#define AML_L1_NAME_OP					  0x08 /* NameOp             */
#define AML_L1_BYTE_PREFIX				  0x0A /* BytePrefix         */
#define AML_L1_WORD_PREFIX				  0x0B /* WordPrefix         */
#define AML_L1_DWORD_PREFIX			      0x0C /* DWordPrefix        */
#define AML_L1_STRING_PREFIX			  0x0D /* StringPrefix       */
#define AML_L1_QWORD_PREFIX			      0x0E /* QWordPrefix        */
#define AML_L1_SCOPE_OP					  0x10 /* ScopeOp            */
#define AML_L1_BUFFER_OP				  0x11 /* BufferOp           */
#define AML_L1_PACKAGE_OP				  0x12 /* PackageOp          */
#define AML_L1_VAR_PACKAGE_OP			  0x13 /* VarPackageOp       */
#define AML_L1_METHOD_OP				  0x14 /* MethodOp           */
#define AML_L1_EXTERNAL_OP				  0x15 /* ExternalOp         */
#define AML_L1_DUAL_NAME_PREFIX			  0x2E /* DualNamePrefix     */
#define AML_L1_MULTI_NAME_PREFIX		  0x2F /* MultiNamePrefix    */
#define AML_L1_DIGIT_CHAR_FIRST			  0x30 /* DigitCharFirst     */
#define AML_L1_DIGIT_CHAR_LAST			  0x39 /* DigitCharLast      */
#define AML_L1_NAME_CHAR_ALPHA_FIRST	  0x41 /* NameCharAlphaFirst */
#define AML_L1_NAME_CHAR_ALPHA_LAST		  0x5A /* NameCharAlphaLast  */
#define AML_L1_EXT_OP_PREFIX			  0x5B /* ExtOpPrefix        */
#define AML_L1_ROOT_CHAR				  0x5C /* RootChar           */
#define AML_L1_PARENT_PREFIX_CHAR		  0x5E /* ParentPrefixChar   */
#define AML_L1_NAME_CHAR_UNDERSCORE		  0x5F /* NameCharUnderscore */
#define AML_L1_LOCAL0_OP				  0x60 /* Local0Op           */
#define AML_L1_LOCAL1_OP				  0x61 /* Local1Op           */
#define AML_L1_LOCAL2_OP				  0x62 /* Local2Op           */
#define AML_L1_LOCAL3_OP				  0x63 /* Local3Op           */
#define AML_L1_LOCAL4_OP				  0x64 /* Local4Op           */
#define AML_L1_LOCAL5_OP				  0x65 /* Local5Op           */
#define AML_L1_LOCAL6_OP				  0x66 /* Local6Op           */
#define AML_L1_LOCAL7_OP				  0x67 /* Local7Op           */
#define AML_L1_ARG0_OP					  0x68 /* Arg0Op             */
#define AML_L1_ARG1_OP					  0x69 /* Arg1Op             */
#define AML_L1_ARG2_OP					  0x6A /* Arg2Op             */
#define AML_L1_ARG3_OP					  0x6B /* Arg3Op             */
#define AML_L1_ARG4_OP					  0x6C /* Arg4Op             */
#define AML_L1_ARG5_OP					  0x6D /* Arg5Op             */
#define AML_L1_ARG6_OP					  0x6E /* Arg6Op             */
#define AML_L1_STORE_OP					  0x70 /* StoreOp            */
#define AML_L1_REF_OF_OP				  0x71 /* RefOfOp            */
#define AML_L1_ADD_OP					  0x72 /* AddOp              */
#define AML_L1_CONCAT_OP				  0x73 /* ConcatOp           */
#define AML_L1_SUBTRACT_OP				  0x74 /* SubtractOp         */
#define AML_L1_INCREMENT_OP				  0x75 /* IncrementOp        */
#define AML_L1_DECREMENT_OP				  0x76 /* DecrementOp        */
#define AML_L1_MULTIPLY_OP				  0x77 /* MultiplyOp         */
#define AML_L1_DIVIDE_OP				  0x78 /* DivideOp           */
#define AML_L1_SHIFT_LEFT_OP			  0x79 /* ShiftLeftOp        */
#define AML_L1_SHIFT_RIGHT_OP			  0x7A /* ShiftRightOp       */
#define AML_L1_AND_OP					  0x7B /* AndOp              */
#define AML_L1_NAND_OP					  0x7C /* NandOp             */
#define AML_L1_OR_OP					  0x7D /* OrOp               */
#define AML_L1_NOR_OP					  0x7E /* NorOp              */
#define AML_L1_XOR_OP					  0x7F /* XorOp              */
#define AML_L1_NOT_OP					  0x80 /* NotOp              */
#define AML_L1_FIND_SET_LEFT_BIT_OP		  0x81 /* FindSetLeftBitOp   */
#define AML_L1_FIND_SET_RIGHT_BIT_OP	  0x82 /* FindSetRightBitOp  */
#define AML_L1_DEREF_OF_OP				  0x83 /* DerefOfOp          */
#define AML_L1_CONCAT_RES_OP			  0x84 /* ConcatResOp        */
#define AML_L1_MOD_OP					  0x85 /* ModOp              */
#define AML_L1_NOTIFY_OP				  0x86 /* NotifyOp           */
#define AML_L1_SIZE_OF_OP				  0x87 /* SizeOfOp           */
#define AML_L1_INDEX_OP					  0x88 /* IndexOp            */
#define AML_L1_MATCH_OP					  0x89 /* MatchOp            */
#define AML_L1_CREATE_DWORD_FIELD_OP	  0x8A /* CreateDWordFieldOp */
#define AML_L1_CREATE_WORD_FIELD_OP		  0x8B /* CreateWordFieldOp  */
#define AML_L1_CREATE_BYTE_FIELD_OP		  0x8C /* CreateByteFieldOp  */
#define AML_L1_CREATE_BIT_FIELD_OP		  0x8D /* CreateBitFieldOp   */
#define AML_L1_OBJECT_TYPE_OP			  0x8E /* ObjectTypeOp       */
#define AML_L1_CREATE_QWORD_FIELD_OP	  0x8F /* CreateQWordFieldOp */
#define AML_L1_LAND_OP					  0x90 /* LandOp             */
#define AML_L1_LOR_OP					  0x91 /* LorOp              */
#define AML_L1_LNOT_OP					  0x92 /* LnotOp             */
#define AML_L1_LEQUAL_OP				  0x93 /* LEqualOp           */
#define AML_L1_LGREATER_OP				  0x94 /* LGreaterOp         */
#define AML_L1_LLESS_OP				      0x95 /* LLessOp            */
#define AML_L1_TO_BUFFER_OP				  0x96 /* ToBufferOp         */
#define AML_L1_TO_DECIMAL_STRING_OP		  0x97 /* ToDecimalStringOp  */
#define AML_L1_TO_HEX_STRING_OP			  0x98 /* ToHexStringOp      */
#define AML_L1_TO_INTEGER_OP			  0x99 /* ToIntegerOp        */
#define AML_L1_TO_STRING_OP				  0x9C /* ToStringOp         */
#define AML_L1_COPY_OBJECT_OP			  0x9D /* CopyObjectOp       */
#define AML_L1_MID_OP					  0x9E /* MidOp              */
#define AML_L1_CONTINUE_OP				  0x9F /* ContinueOp         */
#define AML_L1_IF_OP					  0xA0 /* IfOp               */
#define AML_L1_ELSE_OP					  0xA1 /* ElseOp             */
#define AML_L1_WHILE_OP					  0xA2 /* WhileOp            */
#define AML_L1_NOOP_OP					  0xA3 /* NoopOp             */
#define AML_L1_RETURN_OP				  0xA4 /* ReturnOp           */
#define AML_L1_BREAK_OP					  0xA5 /* BreakOp            */
#define AML_L1_BREAK_POINT_OP			  0xCC /* BreakPointOp       */
#define AML_L1_ONES_OP					  0xFF /* OnesOp             */

//
// Level 2 sub-opcodes of L1 opcode ExtOpPrefix (0x5B), second byte of the full two-byte instruction.
//    
#define AML_L2_EXT_MUTEX_OP			 0x01 /* MutexOp       */
#define AML_L2_EXT_EVENT_OP			 0x02 /* EventOp       */
#define AML_L2_EXT_COND_REF_OF_OP	 0x12 /* CondRefOfOp   */
#define AML_L2_EXT_CREATE_FIELD_OP	 0x13 /* CreateFieldOp */
#define AML_L2_EXT_LOAD_TABLE_OP	 0x1F /* LoadTableOp   */
#define AML_L2_EXT_LOAD_OP			 0x20 /* LoadOp        */
#define AML_L2_EXT_STALL_OP			 0x21 /* StallOp       */
#define AML_L2_EXT_SLEEP_OP			 0x22 /* SleepOp       */
#define AML_L2_EXT_ACQUIRE_OP		 0x23 /* AcquireOp     */
#define AML_L2_EXT_SIGNAL_OP		 0x24 /* SignalOp      */
#define AML_L2_EXT_WAIT_OP			 0x25 /* WaitOp        */
#define AML_L2_EXT_RESET_OP			 0x26 /* ResetOp       */
#define AML_L2_EXT_RELEASE_OP		 0x27 /* ReleaseOp     */
#define AML_L2_EXT_FROM_BCD_OP		 0x28 /* FromBCDOp     */
#define AML_L2_EXT_TO_BCD			 0x29 /* ToBCD         */
#define AML_L2_EXT_RESERVED			 0x2A /* Reserved      */
#define AML_L2_EXT_REVISION_OP		 0x30 /* RevisionOp    */
#define AML_L2_EXT_DEBUG_OP			 0x31 /* DebugOp       */
#define AML_L2_EXT_FATAL_OP			 0x32 /* FatalOp       */
#define AML_L2_EXT_TIMER_OP			 0x33 /* TimerOp       */
#define AML_L2_EXT_OP_REGION_OP		 0x80 /* OpRegionOp    */
#define AML_L2_EXT_FIELD_OP			 0x81 /* FieldOp       */
#define AML_L2_EXT_DEVICE_OP		 0x82 /* DeviceOp      */
#define AML_L2_EXT_POWER_RES_OP		 0x84 /* PowerResOp    */
#define AML_L2_EXT_THERMAL_ZONE_OP	 0x85 /* ThermalZoneOp */
#define AML_L2_EXT_INDEX_FIELD_OP	 0x86 /* IndexFieldOp  */
#define AML_L2_EXT_BANK_FIELD_OP	 0x87 /* BankFieldOp   */
#define AML_L2_EXT_DATA_REGION_OP	 0x88 /* DataRegionOp  */

//
// Level 2 sub-opcodes of L1 opcode LnotOp (0x92).
//          
#define AML_L2_NOT_LNOT_EQUAL_OP     0x93 /* LNotEqualOp    */
#define AML_L2_NOT_LLESS_EQUAL_OP    0x94 /* LLessEqualOp   */
#define AML_L2_NOT_LGREATER_EQUAL_OP 0x95 /* LGreaterEqualO */

//
// Full 2-byte combined internal decoded opcodes IDs.
//
#define AML_OPCODE_ID_ZERO_OP					  ( ( UINT16 )AML_L1_ZERO_OP				)
#define AML_OPCODE_ID_ONE_OP					  ( ( UINT16 )AML_L1_ONE_OP					)
#define AML_OPCODE_ID_ALIAS_OP					  ( ( UINT16 )AML_L1_ALIAS_OP				)
#define AML_OPCODE_ID_NAME_OP					  ( ( UINT16 )AML_L1_NAME_OP				)
#define AML_OPCODE_ID_BYTE_PREFIX				  ( ( UINT16 )AML_L1_BYTE_PREFIX			)
#define AML_OPCODE_ID_WORD_PREFIX				  ( ( UINT16 )AML_L1_WORD_PREFIX			)
#define AML_OPCODE_ID_DWORD_PREFIX			      ( ( UINT16 )AML_L1_DWORD_PREFIX			)
#define AML_OPCODE_ID_STRING_PREFIX			      ( ( UINT16 )AML_L1_STRING_PREFIX			)
#define AML_OPCODE_ID_QWORD_PREFIX			      ( ( UINT16 )AML_L1_QWORD_PREFIX			)
#define AML_OPCODE_ID_SCOPE_OP					  ( ( UINT16 )AML_L1_SCOPE_OP				)
#define AML_OPCODE_ID_BUFFER_OP				      ( ( UINT16 )AML_L1_BUFFER_OP				)
#define AML_OPCODE_ID_PACKAGE_OP				  ( ( UINT16 )AML_L1_PACKAGE_OP				)
#define AML_OPCODE_ID_VAR_PACKAGE_OP			  ( ( UINT16 )AML_L1_VAR_PACKAGE_OP			)
#define AML_OPCODE_ID_METHOD_OP				      ( ( UINT16 )AML_L1_METHOD_OP				)
#define AML_OPCODE_ID_EXTERNAL_OP				  ( ( UINT16 )AML_L1_EXTERNAL_OP			)
#define AML_OPCODE_ID_DUAL_NAME_PREFIX			  ( ( UINT16 )AML_L1_DUAL_NAME_PREFIX		)
#define AML_OPCODE_ID_MULTI_NAME_PREFIX		      ( ( UINT16 )AML_L1_MULTI_NAME_PREFIX		)
#define AML_OPCODE_ID_DIGIT_CHAR_FIRST			  ( ( UINT16 )AML_L1_DIGIT_CHAR_FIRST		)
#define AML_OPCODE_ID_DIGIT_CHAR_LAST			  ( ( UINT16 )AML_L1_DIGIT_CHAR_LAST		)
#define AML_OPCODE_ID_NAME_CHAR_ALPHA_FIRST	      ( ( UINT16 )AML_L1_NAME_CHAR_ALPHA_FIRST	)
#define AML_OPCODE_ID_NAME_CHAR_ALPHA_LAST		  ( ( UINT16 )AML_L1_NAME_CHAR_ALPHA_LAST	)
#define AML_OPCODE_ID_EXT_OP_PREFIX			      ( ( UINT16 )AML_L1_EXT_OP_PREFIX			)
	#define AML_OPCODE_ID_MUTEX_OP			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x01 << 8 ) ) /* MutexOp       */
	#define AML_OPCODE_ID_EVENT_OP			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x02 << 8 ) ) /* EventOp       */
	#define AML_OPCODE_ID_COND_REF_OF_OP	      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x12 << 8 ) ) /* CondRefOfOp   */
	#define AML_OPCODE_ID_CREATE_FIELD_OP	      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x13 << 8 ) ) /* CreateFieldOp */
	#define AML_OPCODE_ID_LOAD_TABLE_OP	          ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x1F << 8 ) ) /* LoadTableOp   */
	#define AML_OPCODE_ID_LOAD_OP			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x20 << 8 ) ) /* LoadOp        */
	#define AML_OPCODE_ID_STALL_OP			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x21 << 8 ) ) /* StallOp       */
	#define AML_OPCODE_ID_SLEEP_OP			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x22 << 8 ) ) /* SleepOp       */
	#define AML_OPCODE_ID_ACQUIRE_OP		      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x23 << 8 ) ) /* AcquireOp     */
	#define AML_OPCODE_ID_SIGNAL_OP		          ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x24 << 8 ) ) /* SignalOp      */
	#define AML_OPCODE_ID_WAIT_OP			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x25 << 8 ) ) /* WaitOp        */
	#define AML_OPCODE_ID_RESET_OP			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x26 << 8 ) ) /* ResetOp       */
	#define AML_OPCODE_ID_RELEASE_OP		      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x27 << 8 ) ) /* ReleaseOp     */
	#define AML_OPCODE_ID_FROM_BCD_OP		      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x28 << 8 ) ) /* FromBCDOp     */
	#define AML_OPCODE_ID_TO_BCD			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x29 << 8 ) ) /* ToBCD         */
	#define AML_OPCODE_ID_RESERVED			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x2A << 8 ) ) /* Reserved      */
	#define AML_OPCODE_ID_REVISION_OP		      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x30 << 8 ) ) /* RevisionOp    */
	#define AML_OPCODE_ID_DEBUG_OP			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x31 << 8 ) ) /* DebugOp       */
	#define AML_OPCODE_ID_FATAL_OP			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x32 << 8 ) ) /* FatalOp       */
	#define AML_OPCODE_ID_TIMER_OP			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x33 << 8 ) ) /* TimerOp       */
	#define AML_OPCODE_ID_OP_REGION_OP		      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x80 << 8 ) ) /* OpRegionOp    */
	#define AML_OPCODE_ID_FIELD_OP			      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x81 << 8 ) ) /* FieldOp       */
	#define AML_OPCODE_ID_DEVICE_OP		          ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x82 << 8 ) ) /* DeviceOp      */
	#define AML_OPCODE_ID_PROCESSOR_OP		      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x83 << 8 ) ) /* ProcessorOp   */
	#define AML_OPCODE_ID_POWER_RES_OP		      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x84 << 8 ) ) /* PowerResOp    */
	#define AML_OPCODE_ID_THERMAL_ZONE_OP	      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x85 << 8 ) ) /* ThermalZoneOp */
	#define AML_OPCODE_ID_INDEX_FIELD_OP	      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x86 << 8 ) ) /* IndexFieldOp  */
	#define AML_OPCODE_ID_BANK_FIELD_OP	          ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x87 << 8 ) ) /* BankFieldOp   */
	#define AML_OPCODE_ID_DATA_REGION_OP	      ( AML_OPCODE_ID_EXT_OP_PREFIX | ( ( UINT16 )0x88 << 8 ) ) /* DataRegionOp  */
#define AML_OPCODE_ID_ROOT_CHAR				      ( ( UINT16 )AML_L1_ROOT_CHAR				)
#define AML_OPCODE_ID_PARENT_PREFIX_CHAR		  ( ( UINT16 )AML_L1_PARENT_PREFIX_CHAR		)
#define AML_OPCODE_ID_NAME_CHAR_UNDERSCORE		  ( ( UINT16 )AML_L1_NAME_CHAR_UNDERSCORE	)
#define AML_OPCODE_ID_LOCAL0_OP				      ( ( UINT16 )AML_L1_LOCAL0_OP				)
#define AML_OPCODE_ID_LOCAL1_OP				      ( ( UINT16 )AML_L1_LOCAL1_OP				)
#define AML_OPCODE_ID_LOCAL2_OP				      ( ( UINT16 )AML_L1_LOCAL2_OP				)
#define AML_OPCODE_ID_LOCAL3_OP				      ( ( UINT16 )AML_L1_LOCAL3_OP				)
#define AML_OPCODE_ID_LOCAL4_OP				      ( ( UINT16 )AML_L1_LOCAL4_OP				)
#define AML_OPCODE_ID_LOCAL5_OP				      ( ( UINT16 )AML_L1_LOCAL5_OP				)
#define AML_OPCODE_ID_LOCAL6_OP				      ( ( UINT16 )AML_L1_LOCAL6_OP				)
#define AML_OPCODE_ID_LOCAL7_OP				      ( ( UINT16 )AML_L1_LOCAL7_OP				)
#define AML_OPCODE_ID_ARG0_OP					  ( ( UINT16 )AML_L1_ARG0_OP				)
#define AML_OPCODE_ID_ARG1_OP					  ( ( UINT16 )AML_L1_ARG1_OP				)
#define AML_OPCODE_ID_ARG2_OP					  ( ( UINT16 )AML_L1_ARG2_OP				)
#define AML_OPCODE_ID_ARG3_OP					  ( ( UINT16 )AML_L1_ARG3_OP				)
#define AML_OPCODE_ID_ARG4_OP					  ( ( UINT16 )AML_L1_ARG4_OP				)
#define AML_OPCODE_ID_ARG5_OP					  ( ( UINT16 )AML_L1_ARG5_OP				)
#define AML_OPCODE_ID_ARG6_OP					  ( ( UINT16 )AML_L1_ARG6_OP				)
#define AML_OPCODE_ID_STORE_OP					  ( ( UINT16 )AML_L1_STORE_OP				)
#define AML_OPCODE_ID_REF_OF_OP				      ( ( UINT16 )AML_L1_REF_OF_OP				)
#define AML_OPCODE_ID_ADD_OP					  ( ( UINT16 )AML_L1_ADD_OP					)
#define AML_OPCODE_ID_CONCAT_OP				      ( ( UINT16 )AML_L1_CONCAT_OP				)
#define AML_OPCODE_ID_SUBTRACT_OP				  ( ( UINT16 )AML_L1_SUBTRACT_OP			)
#define AML_OPCODE_ID_INCREMENT_OP				  ( ( UINT16 )AML_L1_INCREMENT_OP			)
#define AML_OPCODE_ID_DECREMENT_OP				  ( ( UINT16 )AML_L1_DECREMENT_OP			)
#define AML_OPCODE_ID_MULTIPLY_OP				  ( ( UINT16 )AML_L1_MULTIPLY_OP			)
#define AML_OPCODE_ID_DIVIDE_OP				      ( ( UINT16 )AML_L1_DIVIDE_OP				)
#define AML_OPCODE_ID_SHIFT_LEFT_OP			      ( ( UINT16 )AML_L1_SHIFT_LEFT_OP			)
#define AML_OPCODE_ID_SHIFT_RIGHT_OP			  ( ( UINT16 )AML_L1_SHIFT_RIGHT_OP			)
#define AML_OPCODE_ID_AND_OP					  ( ( UINT16 )AML_L1_AND_OP					)
#define AML_OPCODE_ID_NAND_OP					  ( ( UINT16 )AML_L1_NAND_OP				)
#define AML_OPCODE_ID_OR_OP					      ( ( UINT16 )AML_L1_OR_OP					)
#define AML_OPCODE_ID_NOR_OP					  ( ( UINT16 )AML_L1_NOR_OP					)
#define AML_OPCODE_ID_XOR_OP					  ( ( UINT16 )AML_L1_XOR_OP					)
#define AML_OPCODE_ID_NOT_OP					  ( ( UINT16 )AML_L1_NOT_OP					)
#define AML_OPCODE_ID_FIND_SET_LEFT_BIT_OP		  ( ( UINT16 )AML_L1_FIND_SET_LEFT_BIT_OP	)
#define AML_OPCODE_ID_FIND_SET_RIGHT_BIT_OP	      ( ( UINT16 )AML_L1_FIND_SET_RIGHT_BIT_OP	)
#define AML_OPCODE_ID_DEREF_OF_OP				  ( ( UINT16 )AML_L1_DEREF_OF_OP			)
#define AML_OPCODE_ID_CONCAT_RES_OP			      ( ( UINT16 )AML_L1_CONCAT_RES_OP			)
#define AML_OPCODE_ID_MOD_OP					  ( ( UINT16 )AML_L1_MOD_OP					)
#define AML_OPCODE_ID_NOTIFY_OP				      ( ( UINT16 )AML_L1_NOTIFY_OP				)
#define AML_OPCODE_ID_SIZE_OF_OP				  ( ( UINT16 )AML_L1_SIZE_OF_OP				)
#define AML_OPCODE_ID_INDEX_OP				      ( ( UINT16 )AML_L1_INDEX_OP				)
#define AML_OPCODE_ID_MATCH_OP				      ( ( UINT16 )AML_L1_MATCH_OP				)
#define AML_OPCODE_ID_CREATE_DWORD_FIELD_OP	      ( ( UINT16 )AML_L1_CREATE_DWORD_FIELD_OP	)
#define AML_OPCODE_ID_CREATE_WORD_FIELD_OP	      ( ( UINT16 )AML_L1_CREATE_WORD_FIELD_OP	)
#define AML_OPCODE_ID_CREATE_BYTE_FIELD_OP	      ( ( UINT16 )AML_L1_CREATE_BYTE_FIELD_OP	)
#define AML_OPCODE_ID_CREATE_BIT_FIELD_OP		  ( ( UINT16 )AML_L1_CREATE_BIT_FIELD_OP	)
#define AML_OPCODE_ID_OBJECT_TYPE_OP			  ( ( UINT16 )AML_L1_OBJECT_TYPE_OP			)
#define AML_OPCODE_ID_CREATE_QWORD_FIELD_OP	      ( ( UINT16 )AML_L1_CREATE_QWORD_FIELD_OP	)
#define AML_OPCODE_ID_LAND_OP					  ( ( UINT16 )AML_L1_LAND_OP				)
#define AML_OPCODE_ID_LOR_OP					  ( ( UINT16 )AML_L1_LOR_OP					)
#define AML_OPCODE_ID_LNOT_OP					  ( ( UINT16 )AML_L1_LNOT_OP				)
	#define AML_OPCODE_ID_LNOT_EQUAL_OP           ( AML_OPCODE_ID_LNOT_OP | ( ( UINT16 )0x93 << 8 ) ) /* LNotEqualOp    */
	#define AML_OPCODE_ID_LLESS_EQUAL_OP          ( AML_OPCODE_ID_LNOT_OP | ( ( UINT16 )0x94 << 8 ) ) /* LLessEqualOp   */
	#define AML_OPCODE_ID_LGREATER_EQUAL_OP       ( AML_OPCODE_ID_LNOT_OP | ( ( UINT16 )0x95 << 8 ) ) /* LGreaterEqualO */
#define AML_OPCODE_ID_LEQUAL_OP				      ( ( UINT16 )AML_L1_LEQUAL_OP				)
#define AML_OPCODE_ID_LGREATER_OP				  ( ( UINT16 )AML_L1_LGREATER_OP			)
#define AML_OPCODE_ID_LLESS_OP				      ( ( UINT16 )AML_L1_LLESS_OP				)
#define AML_OPCODE_ID_TO_BUFFER_OP			      ( ( UINT16 )AML_L1_TO_BUFFER_OP			)
#define AML_OPCODE_ID_TO_DECIMAL_STRING_OP	      ( ( UINT16 )AML_L1_TO_DECIMAL_STRING_OP	)
#define AML_OPCODE_ID_TO_HEX_STRING_OP		      ( ( UINT16 )AML_L1_TO_HEX_STRING_OP		)
#define AML_OPCODE_ID_TO_INTEGER_OP			      ( ( UINT16 )AML_L1_TO_INTEGER_OP			)
#define AML_OPCODE_ID_TO_STRING_OP			      ( ( UINT16 )AML_L1_TO_STRING_OP			)
#define AML_OPCODE_ID_COPY_OBJECT_OP			  ( ( UINT16 )AML_L1_COPY_OBJECT_OP			)
#define AML_OPCODE_ID_MID_OP					  ( ( UINT16 )AML_L1_MID_OP					)
#define AML_OPCODE_ID_CONTINUE_OP				  ( ( UINT16 )AML_L1_CONTINUE_OP			)
#define AML_OPCODE_ID_IF_OP					      ( ( UINT16 )AML_L1_IF_OP					)
#define AML_OPCODE_ID_ELSE_OP					  ( ( UINT16 )AML_L1_ELSE_OP				)
#define AML_OPCODE_ID_WHILE_OP				      ( ( UINT16 )AML_L1_WHILE_OP				)
#define AML_OPCODE_ID_NOOP_OP					  ( ( UINT16 )AML_L1_NOOP_OP				)
#define AML_OPCODE_ID_RETURN_OP				      ( ( UINT16 )AML_L1_RETURN_OP				)
#define AML_OPCODE_ID_BREAK_OP				      ( ( UINT16 )AML_L1_BREAK_OP				)
#define AML_OPCODE_ID_BREAK_POINT_OP			  ( ( UINT16 )AML_L1_BREAK_POINT_OP			)
#define AML_OPCODE_ID_ONES_OP					  ( ( UINT16 )AML_L1_ONES_OP				)

//
// AML opcode encoding groups.
// Note: these are internal, the values have no meaning or relation to the AML spec.
//
#define AML_ENCODING_GROUP_NONE         0
#define AML_ENCODING_GROUP_DATA_OBJECT  1
#define AML_ENCODING_GROUP_TERM_OBJECT  2
#define AML_ENCODING_GROUP_NAME_OBJECT  3
#define AML_ENCODING_GROUP_DEBUG_OBJECT 4
#define AML_ENCODING_GROUP_LOCAL_OBJECT 5
#define AML_ENCODING_GROUP_ARG_OBJECT   6

//
// AML argument entry types.
//
#define AML_ARGUMENT_FIXED_TYPE_NONE		             0
#define AML_ARGUMENT_FIXED_TYPE_NAME_STRING		         1
#define AML_ARGUMENT_FIXED_TYPE_DATA_REF_OBJECT	         2
#define AML_ARGUMENT_FIXED_TYPE_BYTE_DATA		         3
#define AML_ARGUMENT_FIXED_TYPE_WORD_DATA		         4
#define AML_ARGUMENT_FIXED_TYPE_DWORD_DATA		         5
#define AML_ARGUMENT_FIXED_TYPE_QWORD_DATA		         6
#define AML_ARGUMENT_FIXED_TYPE_ASCII_CHAR_LIST	         7
#define AML_ARGUMENT_FIXED_TYPE_NULL_CHAR		         8
#define AML_ARGUMENT_FIXED_TYPE_TERM_ARG	             9
#define AML_ARGUMENT_FIXED_TYPE_NAME_SEG	             10
#define AML_ARGUMENT_FIXED_TYPE_NAME_SEG_N		         11
#define AML_ARGUMENT_FIXED_TYPE_SUPER_NAME		         12
#define AML_ARGUMENT_FIXED_TYPE_SUPER_NAME_NO_INVOCATION 13
#define AML_ARGUMENT_FIXED_TYPE_TARGET		             14
#define AML_ARGUMENT_FIXED_TYPE_SIMPLE_NAME		         15

//
// AML opcode variable list argument types.
//
#define AML_ARGUMENT_VARIABLE_TYPE_NONE       0
#define AML_ARGUMENT_VARIABLE_TYPE_TERM_LIST  1
#define AML_ARGUMENT_VARIABLE_TYPE_BYTE_LIST  2
#define AML_ARGUMENT_VARIABLE_TYPE_PACKAGE    3
#define AML_ARGUMENT_VARIABLE_TYPE_FIELD_LIST 4

//
// AML instruction opcode table entry.
//
typedef struct _AML_OPCODE_TABLE_ENTRY {
	UINT8 IsValid : 1;
	UINT8 IsExpressionOp : 1;
	UINT8 OpcodeByte;
	UINT8 EncodingGroup;
	UINT8 SubTableIndex;
	UINT8 FixedListArgTableOffset;
	UINT8 FixedListArgCount;
	UINT8 VariableListArgTableOffset;
	UINT8 VariableListArgCount;
} AML_OPCODE_TABLE_ENTRY;

//
// AML opcode sub-table indirection table definition,
// contains information about the subtable, and an actual pointer to the subtable's entries.
//
typedef struct _AML_OPCODE_SUBTABLE_LIST_ENTRY {
	BOOLEAN                       IsValid;
	BOOLEAN                       IsOptionalMatch;
	UINT8                         OpcodeByteL1;
	const AML_OPCODE_TABLE_ENTRY* Entries;
} AML_OPCODE_SUBTABLE_LIST_ENTRY;
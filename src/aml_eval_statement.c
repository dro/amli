#include "aml_eval.h"
#include "aml_eval_statement.h"
#include "aml_debug.h"
#include "aml_mutex.h"

//
// Evaluate a DefIfElse instruction.
// Predicate := TermArg => Integer
// DefElse := Nothing | <elseop pkglength termlist>
// DefIfElse := IfOp PkgLength Predicate TermList DefElse
//
_Success_( return )
BOOLEAN
AmlEvalIfElse(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    )
{
    SIZE_T   PkgStart;
    SIZE_T   PkgLength;
    SIZE_T   TermListStart;
    SIZE_T   TermListSize;
    AML_DATA Predicate;

    //
    // Consume the instruction opcode if the caller has yet to do it.
    //
    if( ConsumeOpcode ) {
        if( AmlDecoderConsumeOpcode( State, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    //
    // Predicate := TermArg => Integer
    // DefElse := Nothing | <elseop pkglength termlist>
    // DefIfElse := IfOp PkgLength Predicate TermList DefElse
    // 
    PkgStart = State->DataCursor;
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( PkgStart >= State->DataLength || PkgLength > ( State->DataLength - PkgStart ) ) {
        return AML_FALSE;
    }

    //
    // Evaluate the statement predicate, predicate is always evaluated as an Integer.
    // Skip evaluation of the predicate if we are in namespace evaluation mode.
    // TODO: Change window to package length to ensure that the predicate and termlist are contained by the package.
    //
    if( State->PassType == AML_PASS_TYPE_FULL ) {
        if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Predicate ) == AML_FALSE ) {
            return AML_FALSE;
        }
    } else if( State->PassType == AML_PASS_TYPE_NAMESPACE ) {
        if( AmlDecoderConsumeTermArg( State, NULL ) == AML_FALSE ) {
            return AML_FALSE;
        }
        Predicate = ( AML_DATA ){ .Type = AML_DATA_TYPE_INTEGER, .u.Integer = 0 };
    } else {
        return AML_FALSE;
    }

    //
    // Size of the term list must not exceed the outer package length.
    //
    if( ( State->DataCursor - PkgStart ) > PkgLength ) {
        return AML_FALSE;
    }

    //
    // Calculate start and size of the term list.
    //
    TermListStart = State->DataCursor;
    TermListSize  = ( PkgLength - ( TermListStart - PkgStart ) );

    //
    // If we are in namespace evaluation mode, scan the contents of both paths,
    // irregardless of the predicate, the namespace path may require all possible object definitions.
    // If the Predicate is non-zero, the term list of the If term is executed, otherwise, skip it.
    // TODO: This doesn't necessarily need to be done recursively, it is only done currently to change the
    // window size to the TermListSize, otherwise, we would just let the eval loop keep processing the next
    // instructions naturally.
    //
    if( ( Predicate.u.Integer != 0 ) || ( State->PassType == AML_PASS_TYPE_NAMESPACE ) ) {
        if( AmlEvalTermListCode( State, State->DataCursor, TermListSize, AML_FALSE ) == AML_FALSE ) {
            return AML_FALSE;
        }
    } else {
        State->DataCursor = ( PkgStart + PkgLength );
    }

    //
    // Check if the statement has an Else portion.
    // Else := ElseOp PkgLength TermList
    //
    if( AmlDecoderMatchOpcode( State, AML_OPCODE_ID_ELSE_OP, NULL ) ) {
        //
        // Save start of the Else statement package data.
        //
        PkgStart = State->DataCursor;

        //
        // Consume and validate package length.
        //
        if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( PkgStart >= State->DataLength || PkgLength > ( State->DataLength - PkgStart ) ) {
            return AML_FALSE;
        }

        //
        // If the If statement predicate was zero, execute the term list of the else statement, otherwise, skip it.
        // Just like above, if this is the namespace pre-pass, explore all paths for all possible object names.
        // TODO: Same as the above call to AmlEvalTermListCode, doesn't necessarily need to be recursive.
        //
        if( ( Predicate.u.Integer == 0 ) || ( State->PassType == AML_PASS_TYPE_NAMESPACE ) ) {
            TermListSize = ( PkgLength - ( State->DataCursor - PkgStart ) );
            if( AmlEvalTermListCode( State, State->DataCursor, TermListSize, AML_FALSE ) == AML_FALSE ) {
                return AML_FALSE;
            }
        } else {
            State->DataCursor = ( PkgStart + PkgLength );
        }
    }

    return AML_TRUE;
}

//
// Predicate := TermArg => Integer
// DefWhile := WhileOp PkgLength Predicate TermList
//
_Success_( return )
BOOLEAN
AmlEvalWhile(
    _Inout_ AML_STATE* State,
    _In_    BOOLEAN    ConsumeOpcode
    )
{
    SIZE_T                 PredicateStart;
    SIZE_T                 PkgStart;
    SIZE_T                 PkgLength;
    SIZE_T                 TermListStart;
    SIZE_T                 TermListSize;
    AML_DATA               Predicate;
    SIZE_T                 i;
    AML_INTERRUPTION_EVENT OldPendingEvent;
    AML_INTERRUPTION_EVENT PendingEvent;

    //
    // Predicate := TermArg => Integer
    // DefWhile := WhileOp PkgLength Predicate TermList
    // Predicate is evaluated as an integer.
    //
    PkgStart = State->DataCursor;
    if( AmlDecoderConsumePackageLength( State, &PkgLength ) == AML_FALSE ) {
        return AML_FALSE;
    } else if( PkgStart >= State->DataLength || PkgLength > ( State->DataLength - PkgStart ) ) {
        return AML_FALSE;
    }

    //
    // Save the start of the while loop predicate, allows us to re-evaluate it after each iteration.
    //
    PredicateStart = State->DataCursor;

    //
    // Evaluate the statement predicate, predicate is always evaluated as an Integer.
    // TODO: Change window to package length to ensure that the predicate and termlist are contained by the package.
    //
    if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Predicate ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Size of the term list must not exceed the outer package length.
    //
    if( ( State->DataCursor - PkgStart ) > PkgLength ) {
        return AML_FALSE;
    }

    //
    // Calculate start and size of the term list.
    //
    TermListStart = State->DataCursor;
    TermListSize  = ( PkgLength - ( TermListStart - PkgStart ) );

    //
    // While the predicate remains true, keep executing the term list until reaching a break or continue statement.
    //
    for( i = 0; Predicate.u.Integer != 0; i++ ) {
        //
        // Infinite Break out of the loop if we have possibly encountered an infinite loop.
        //
        if( i >= AML_BUILD_MAX_LOOP_ITERATIONS ) {
            AML_DEBUG_ERROR( State, "Error: Exceeded maximum loop iteration count (possible infinite loop).\n" );
            return AML_FALSE;
        }

        //
        // Increase while loop nesting level.
        //
        if( State->WhileLoopLevel == SIZE_MAX ) {
            AML_DEBUG_ERROR( State, "Error: Reached maximum while loop nesting level.\n" );
            return AML_FALSE;
        }
        State->WhileLoopLevel++;

        //
        // Back up old control flow interruption state, generally these should never stay set between nesting levels, but just make sure.
        // Clear control flow interruption state, and then break out of term list execution if a while loop break event becomes set.
        // TODO: This whole system needs a refactor now due to the return instruction (?)
        //
        OldPendingEvent = State->PendingInterruptionEvent;
        State->PendingInterruptionEvent = AML_INTERRUPTION_EVENT_NONE;

        //
        // Execute the body term list of the while loop.
        //
        if( AmlEvalTermListCode( State, TermListStart, TermListSize, AML_TRUE ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Save the pending event results of this iteration.
        //
        PendingEvent = State->PendingInterruptionEvent;

        //
        // Decrease while loop nesting level count.
        //
        State->WhileLoopLevel--;

        //
        // If there is a pending event that wasn't for us, we must pass it on until it is handled by the intended code.
        //
        if( ( PendingEvent != AML_INTERRUPTION_EVENT_NONE )
            && ( PendingEvent != AML_INTERRUPTION_EVENT_BREAK )
            && ( PendingEvent != AML_INTERRUPTION_EVENT_CONTINUE ) )
        {
            break;
        }

        //
        // Restore old control flow interruption state, and decrease nesting level.
        //
        State->PendingInterruptionEvent = OldPendingEvent;

        //
        // Break out of the loop if we have hit a break instruction.
        //
        if( PendingEvent == AML_INTERRUPTION_EVENT_BREAK ) {
            break;
        }

        //
        // Re-evaluate the predicate's value after this iteration.
        //
        State->DataCursor = PredicateStart;
        if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Predicate ) == AML_FALSE ) {
            return AML_FALSE;
        }
    }

    //
    // We have finished executing the while loop, move the IP past the while loop instruction and body.
    //
    State->DataCursor = ( PkgStart + PkgLength );
    return AML_TRUE;
}

//
// Evaluate a StatementOpcode.
// StatementOpcode := DefBreak | DefBreakPoint | DefContinue | DefFatal | DefIfElse
// 	| DefNoop | DefNotify | DefRelease | DefReset | DefReturn | DefSignal | DefSleep | DefStall | DefWhile
//
_Success_( return )
BOOLEAN
AmlEvalStatementOpcode(
    _Inout_ AML_STATE* State
    )
{
    AML_DECODER_INSTRUCTION_OPCODE Statement;
    UINT8                          FatalType;
    UINT32                         FatalCode;
    AML_DATA                       FatalArg;
    AML_OBJECT*                    Object;
    AML_DATA                       NotifyValue;
    AML_DATA                       Time;

    //
    // Consume next full instruction opcode, must have already been deduced to be a StatementOpcode by the caller.
    //
    if( AmlDecoderConsumeOpcode( State, &Statement ) == AML_FALSE ) {
        return AML_FALSE;
    }

    //
    // Attempt to evaluate all supported StatementOpcodes.
    //
    switch( Statement.OpcodeID ) {
    case AML_OPCODE_ID_RELEASE_OP:
        //
        // MutexObject := SuperName
        // DefRelease := ReleaseOp MutexObject
        //
        if( AmlEvalSuperName( State, 0, 0, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        } if( Object->Type != AML_OBJECT_TYPE_MUTEX ) {
            AmlObjectRelease( Object );
            return AML_FALSE;
        }

        //
        // Release mutex.
        //
        AmlMutexRelease( State, Object );
        AmlObjectRelease( Object );
        return AML_TRUE;
    case AML_OPCODE_ID_RESET_OP:
        //
        // EventObject := SuperName
        // DefReset := ResetOp EventObject
        //
        if( AmlEvalSuperName( State, 0, 0, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        } if( Object->Type != AML_OBJECT_TYPE_EVENT ) {
            AmlObjectRelease( Object );
            return AML_FALSE;
        }
        AmlHostEventReset( Object->u.Event.Host, Object->u.Event.HostHandle );
        AmlObjectRelease( Object );
        return AML_TRUE;
    case AML_OPCODE_ID_SIGNAL_OP:
        //
        // EventObject := SuperName
        // DefSignal := SignalOp EventObject
        //
        if( AmlEvalSuperName( State, 0, 0, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        } if( Object->Type != AML_OBJECT_TYPE_EVENT ) {
            AmlObjectRelease( Object );
            return AML_FALSE;
        }
        AmlHostEventSignal( Object->u.Event.Host, Object->u.Event.HostHandle );
        AmlObjectRelease( Object );
        return AML_TRUE;
    case AML_OPCODE_ID_SLEEP_OP:
        //
        // DefSleep := SleepOp MsecTime
        // MsecTime := TermArg => Integer
        //
        if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Time ) == AML_FALSE ) {
            return AML_FALSE;
        }
        AmlHostSleep( State->Host, Time.u.Integer );
        return AML_TRUE;
    case AML_OPCODE_ID_STALL_OP:
        //
        // UsecTime := TermArg => ByteData
        // DefStall := StallOp UsecTime
        //
        if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &Time ) == AML_FALSE ) {
            return AML_FALSE;
        }
        AmlHostStall( State->Host, Time.u.Integer );
        return AML_TRUE;
    case AML_OPCODE_ID_NOTIFY_OP:
        //
        // NotifyObject := SuperName => ThermalZone | Processor | Device
        // NotifyValue := TermArg => Integer
        // DefNotify := NotifyOp NotifyObject NotifyValue
        //
        if( AmlEvalSuperName( State, 0, 0, &Object ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &NotifyValue ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Validate the type of the NotifyObject.
        //
        switch( Object->Type ) {
        case AML_OBJECT_TYPE_THERMAL_ZONE:
        case AML_OBJECT_TYPE_PROCESSOR:
        case AML_OBJECT_TYPE_DEVICE:
            AmlHostObjectNotification( State->Host, Object, NotifyValue.u.Integer );
            break;
        default:
            AML_DEBUG_ERROR( State, "Error: Unsupported NotifyObject type.\n" );
            break;
        }

        AmlObjectRelease( Object );
        return AML_TRUE;
    case AML_OPCODE_ID_IF_OP:
        //
        // Predicate := TermArg => Integer
        // DefElse := Nothing | <elseop pkglength termlist>
        // DefIfElse := IfOp PkgLength Predicate TermList DefElse
        // 
        return AmlEvalIfElse( State, AML_FALSE );
    case AML_OPCODE_ID_WHILE_OP:
        //
        // Predicate := TermArg => Integer
        // DefWhile := WhileOp PkgLength Predicate TermList
        //
        return AmlEvalWhile( State, AML_FALSE );
    case AML_OPCODE_ID_ELSE_OP:
        AML_DEBUG_ERROR( State, "Error: Else statement opcode without matching If statement!\n" );
        return AML_FALSE;
    case AML_OPCODE_ID_NOOP_OP:
        return AML_TRUE;
    case AML_OPCODE_ID_BREAK_OP:
        if( State->WhileLoopLevel <= 0 ) {
            AML_DEBUG_WARNING( State, "Warning: Ignoring break instruction outside of a while loop body\n" );
            return AML_TRUE;
        } else if( State->PendingInterruptionEvent != AML_INTERRUPTION_EVENT_NONE ) {
            AML_DEBUG_ERROR( State, "Error: Interruption event already pending during break\n" );
            return AML_FALSE;
        }
        State->PendingInterruptionEvent = AML_INTERRUPTION_EVENT_BREAK;
        return AML_TRUE;
    case AML_OPCODE_ID_CONTINUE_OP:
        if( State->WhileLoopLevel <= 0 ) {
            AML_DEBUG_WARNING( State, "Warning: Ignoring continue instruction outside of a while loop body\n" );
            return AML_TRUE;
        } else if( State->PendingInterruptionEvent != AML_INTERRUPTION_EVENT_NONE ) {
            AML_DEBUG_ERROR( State, "Error: Interruption event already pending during continue\n" );
            return AML_FALSE;
        }
        State->PendingInterruptionEvent = AML_INTERRUPTION_EVENT_CONTINUE;
        return AML_TRUE;
    case AML_OPCODE_ID_RETURN_OP:
        if( State->MethodScopeLast == State->MethodScopeRoot ) {
            /* TODO: Make this non-fatal, just consume the arg and do nothing. */
            AML_DEBUG_ERROR( State, "Error: Return outside of method body\n" );
            return AML_FALSE;
        } else if( State->PendingInterruptionEvent != AML_INTERRUPTION_EVENT_NONE ) {
            AML_DEBUG_ERROR( State, "Error: interruption event already pending during return\n" );
            return AML_FALSE;
        }

        //
        // ArgObject := TermArg => DataRefObject
        // DefReturn := ReturnOp ArgObject
        //
        AmlDataFree( &State->MethodScopeLast->ReturnValue );
        if( AmlEvalTermArg( State, 0, &State->MethodScopeLast->ReturnValue ) == AML_FALSE ) {
            return AML_FALSE;
        }

        //
        // Set up return interruption event, will cause us to break out of execution of the current method scope.
        //
        State->PendingInterruptionEvent = AML_INTERRUPTION_EVENT_RETURN;
        return AML_TRUE;
    case AML_OPCODE_ID_FATAL_OP:
        //
        // FatalType := ByteData
        // FatalCode := DWordData
        // FatalArg := TermArg => Integer
        // DefFatal := FatalOp FatalType FatalCode FatalArg
        //
        if( AmlDecoderConsumeByte( State, &FatalType ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlDecoderConsumeDWord( State, &FatalCode ) == AML_FALSE ) {
            return AML_FALSE;
        } else if( AmlEvalTermArgToType( State, 0, AML_DATA_TYPE_INTEGER, &FatalArg ) == AML_FALSE ) {
            return AML_FALSE;
        }
        AML_DEBUG_FATAL(
            State,
            "Fatal OEM-defined error: Type=(0x%x) FatalCode=(0x%x) FatalArg=(0x%"PRIx64")\n",
            ( UINT )FatalType, ( UINT )FatalCode, FatalArg.u.Integer
        );
        return AML_FALSE;
    case AML_OPCODE_ID_BREAK_POINT_OP:
        AML_DEBUG_INFO( State, "Debug breakpoint hit @ 0x%"PRIx64"\n", ( ( UINT64 )State->DataCursor - 1 ) );
        break;
    default:
        AML_DEBUG_ERROR( State, "Error: Unsupported statement opcode!" );
        return AML_FALSE;
    }

    return AML_FALSE;
}
#include "aml_string_conv.h"

//
// Allows the user to override the implementation of string conversion functions with their own.
// In this case, the user should implement all functions defined in the header.
//
#ifndef AML_BUILD_OVERRIDE_STRING_CONV

//
// String/integer conversion utilities, assumes ANSI compatible C strings/chars.
//

//
// Convert a single digit character to integer.
// Return value is < 0 upon failure/non-digit character.
//
_Success_( return >= 0 )
static
INT
AmlStringConvDigitCharToInteger(
	_In_ CHAR Char,
	_In_ INT  NumberRadix
	)
{
	CHAR CharAlphaLower;

	//
	// Reject invalid radix base values.
	// Considers a number system base of 0 as invalid, though we could only match zeroes instead,
	// there is no point, and zero being passed here probably indicates a programmer error.
	//
	if( NumberRadix <= 0 || NumberRadix >= ( 9 + ( 'z' - 'a' ) ) ) {
		return -1;
	}

	//
	// Most common case, match regular digits shared by all number system bases.
	//
	if( Char >= '0' && Char < ( '0' + AML_MIN( NumberRadix, 10 ) ) ) {
		return ( Char - '0' );
	}

	//
	// Less standard case, match alphabetical characters as digits for number system bases above 10.
	//
	if( NumberRadix > 10 ) {
		CharAlphaLower = ( Char | ( 1 << 5 ) ); /* Force ANSI alphabet characters to lowercase. */
		if( CharAlphaLower >= 'a' && CharAlphaLower <= ( 'a' + AML_MIN( ( NumberRadix - 10 ), ( 'z' - 'a' ) ) ) ) {
			return 10 + ( CharAlphaLower - 'a' );
		}
	}

	return -1;
}

//
// Compare whitespace characters that are skipped in certain conversion cases.
//
static
inline
BOOLEAN
AmlStringConvIsWhiteSpace(
	_In_ CHAR Char
	)
{
	return( ( Char == ' ' ) || ( Char == '\r' )
			|| ( Char == '\n' ) || ( Char == '\t' )
			|| ( Char == '\v' ) || ( Char == '\f' ) );
}

//
// Peek the next character without consuming it.
//
static
CHAR
AmlStringConvPeek8(
	_In_count_( StringLength ) const CHAR* String,
	_In_                       SIZE_T      StringLength,
	_Inout_                    SIZE_T*     pCursor
	)
{
	return ( ( *pCursor < StringLength ) ? String[ *pCursor ] : '\0' );
}

//
// Unconditionally consume the next character.
//
static
CHAR
AmlStringConvConsume8(
	_In_count_( StringLength ) const CHAR* String,
	_In_                       SIZE_T      StringLength,
	_Inout_                    SIZE_T*     pCursor
	)
{
	if( *pCursor >= StringLength ) {
		return '\0';
	}
	*pCursor += 1;
	return String[ ( *pCursor - 1 ) ];
}

//
// Peek and consume the next character if it matches the desired character.
//
static
BOOLEAN
AmlStringConvMatch8(
	_In_count_( StringLength ) const CHAR* String,
	_In_                       SIZE_T      StringLength,
	_Inout_                    SIZE_T*     pCursor,
	_In_                       CHAR        MatchChar
	)
{
	if( AmlStringConvPeek8( String, StringLength, pCursor ) != MatchChar ) {
		return AML_FALSE;
	}
	*pCursor += 1;
	return AML_TRUE;
}

//
// Attempt to parse an integer from string.
// If NumberRadix is 0, the number system base will be attempted to be automatically deduced by prefixes.
// Skips prefixed whitespace for compatibility with the behavior of some strntox implementations.
// Currently performs no error checking or overflow checking!
//
UINT64
AmlStringConvParseIntegerU64(
	_In_count_( StringLength ) const CHAR* String,
	_In_                       SIZE_T      StringLength,
	_In_                       INT         NumberRadix,
	_In_                       UINT        Flags,
	_Out_opt_                  SIZE_T*     pNumberLength
	)
{
	CHAR    Char;
	SIZE_T  Cursor;
	BOOLEAN IsNegated;
	UINT64  Value;
	INT     Digit;

	//
	// Convert empty strings to zero integer temporarily.
	// TODO: Add a flag for this behavior.
	//
	if( StringLength == 0 ) {
		if( pNumberLength != NULL ) {
			*pNumberLength = 0;
		}
		return 0;
	}

	//
	// Default to zero, if nothing is able to be parsed, it will be returned as such.
	//
	Value = 0;

	//
	// Current string cursor offset, points to the next character to be processed.
	//
	Cursor = 0;

	//
	// Skip prefixed spaces.
	//
	if( ( Flags & AML_STRING_CONV_FLAG_SKIP_LWS ) != 0 ) {
		while( AmlStringConvIsWhiteSpace( AmlStringConvPeek8( String, StringLength, &Cursor ) ) ) {
			AmlStringConvConsume8( String, StringLength, &Cursor );
		}
	}

	//
	// Single optional '+' or '-' prefix.
	//
	if( ( Flags & AML_STRING_CONV_FLAG_NO_SIGN_PREFIX ) == 0 ) {
		IsNegated = AmlStringConvMatch8( String, StringLength, &Cursor, '-' );
		if( IsNegated == AML_FALSE ) {
			AmlStringConvMatch8( String, StringLength, &Cursor, '+' );
		}
	} else {
		IsNegated = AML_FALSE;
	}

	//
	// Attempt to automatically deduce the base from prefixes if 0 is passed.
	//
	if( NumberRadix == 0 ) {
		//
		// Inferred number system bases by prefix are matched with the following precedence:
		//  0x = base-16 (hex).
		//  0  = base-8  (octal).
		//  *  = base-10 (decimal).
		//
		NumberRadix = 10;
		if( AmlStringConvMatch8( String, StringLength, &Cursor, '0' ) ) {
			NumberRadix = ( AmlStringConvMatch8( String, StringLength, &Cursor, 'x' ) ? 16 : 8 );
		}
	} else if( NumberRadix == 16 ) {
		//
		// Skip 0x prefix for base-16 numbers.
		//
		if( ( Flags & AML_STRING_CONV_FLAG_NO_RADIX_PREFIX ) == 0 ) {
			if( AmlStringConvMatch8( String, StringLength, &Cursor, '0' ) ) {
				AmlStringConvMatch8( String, StringLength, &Cursor, 'x' );
			}
		}
	}

	//
	// Accumulate digits until EOF or non-digit character.
	// TODO: Handle overflow/consuming more than the maximum amount of digits for the number integer type.
	//
	while( AML_TRUE ) {
		Char = AmlStringConvConsume8( String, StringLength, &Cursor );
		Digit = AmlStringConvDigitCharToInteger( Char, NumberRadix );
		if( Digit < 0 || Digit >= NumberRadix ) {
			break;
		}
		Value = ( ( Value * NumberRadix ) + Digit );
	}

	//
	// Invert number if there was a minus prefix.
	// TODO: This should only be applied to base10?
	//
	Value = ( IsNegated ? ( ~Value + 1 ) : Value );

	//
	// Optionally write out the parsed number length.
	//
	if( pNumberLength != NULL ) {
		*pNumberLength = Cursor;
	}
	
	return Value;
}

#endif
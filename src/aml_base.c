#include "aml_data.h"
#include "aml_arena.h"
#include "aml_base.h"

//
// Read bit-granularity data from the given buffer field to the result data array.
// The given ResultDataSize is the max size of the ResultData array in *bytes*.
//
_Success_( return )
BOOLEAN
AmlCopyBits(
	_In_reads_bytes_( InputDataSize )   const VOID* InputData,
	_In_                                SIZE_T      InputDataSize,
	_Inout_bytecount_( ResultDataSize ) VOID*       ResultData,
	_In_                                SIZE_T      ResultDataSize,
	_In_                                SIZE_T      InputBitIndex,
	_In_                                SIZE_T      InputBitCount,
	_In_                                SIZE_T      OutputBitIndex
	)
{
	SIZE_T BitOffset;
	SIZE_T OutByteIndex;
	SIZE_T OutByteLocalBitIndex;
	SIZE_T RemainingBitCount;
	SIZE_T InputByteIndex;
	SIZE_T InputByteLocalBitIndex;
	UINT8  InputBitValue;

	//
	// Ensure that the input data is large enough to contain the input bit span.
	//
	if( ( ( InputBitIndex + InputBitCount ) / CHAR_BIT ) > InputDataSize ) {
		return AML_FALSE;
	}

	//
	// Ensure that the output data is large enough to receive the output bit span.
	//
	if( ( ( OutputBitIndex + InputBitCount ) / CHAR_BIT ) > ResultDataSize ) {
		return AML_FALSE;
	}

	//
	// Copy byte-by-byte until we have reached a final byte containing a lesser number of bits.
	//
	for( BitOffset = 0; BitOffset < InputBitCount; ) {
		//
		// Calculate the output data byte index and local bit index for the current bit offset.
		//
		OutByteIndex = ( ( OutputBitIndex + BitOffset ) / CHAR_BIT );
		OutByteLocalBitIndex = ( ( OutputBitIndex + BitOffset ) % CHAR_BIT );
		RemainingBitCount = ( InputBitCount - BitOffset );

		//
		// Calculate the input byte index and local bit index for the current bit offset.
		//
		InputByteIndex = ( ( InputBitIndex + BitOffset ) / CHAR_BIT );
		InputByteLocalBitIndex = ( ( InputBitIndex + BitOffset ) % CHAR_BIT );
		_Analysis_assume_( OutByteIndex < ResultDataSize );

		//
		// Copy an entire byte at once if there are more than 8 bits remaining, and we are aligned.
		//
		if( ( ( OutByteLocalBitIndex | InputByteLocalBitIndex ) == 0 ) && ( RemainingBitCount >= CHAR_BIT ) ) {
			( ( CHAR* )ResultData )[ OutByteIndex ] = ( ( const CHAR* )InputData )[ InputByteIndex ];
			BitOffset += CHAR_BIT;
			continue;
		}

		//
		// Just copy bit-by-bit for the last few bits (or until we are byte-aligned).
		// Read the bit value from the buffer data.
		// TODO: Improve this to copy the largest span of bits possible within the current word.
		//
		InputBitValue = ( ( ( const CHAR* )InputData )[ InputByteIndex ] >> InputByteLocalBitIndex );
		InputBitValue = ( InputBitValue & 1 );

		//
		// Copy the bit value to the result data.
		//
		( ( CHAR* )ResultData )[ OutByteIndex ] &= ~( 1 << OutByteLocalBitIndex );
		( ( CHAR* )ResultData )[ OutByteIndex ] |= ( InputBitValue << OutByteLocalBitIndex );
		BitOffset += 1;
	}

	return AML_TRUE;
}

//
// Convert a decimal number to a 4-bit digit BCD encoded value.
//
UINT64
AmlDecimalToBcd(
	_In_ UINT64 Input
	)
{
	UINT64 Output;
	UINT64 i;
	UINT64 Digit;

	//
	// Convert all base-10 digits into 4-bit binary codes.
	//
	Output = 0;
	for( i = 0; Input != 0; i++ ) {
		Digit   = ( Input % 10 );
		Output |= ( ( Digit & 0xF ) << ( i * 4 ) );
		Input  /= 10;
	}

	return Output;
}

//
// Convert a 4-bit digit BCD encoded value to decimal.
//
UINT64
AmlBcdToDecimal(
	_In_ UINT64 Input
	)
{
	UINT64 Output;
	INT64  i;
	UINT64 Digit;

	//
	// Convert all 4-bit binary coded digits back to decimal digits.
	//
	Output = 0;
	for( i = ( 64 - 4 ); i >= 0; i -= 4 ) {
		Digit   = ( ( Input >> i ) & 0xF );
		Output *= 10;
		Output += Digit;
	}

	return Output;
}

//
// Performs no validation of the input string other than the length, is assumed to be a valid EISAID string
// in the following form: "UUUNNNN", where "U" is an uppercase letter and "N" is a hexadecimal digit.
// The results may not be a valid EISAID value if an invalid string is passed.
//
_Success_( return != 0 )
UINT32
AmlStringToEisaId(
	_In_count_( Length ) const CHAR* String,
	_In_                 SIZE_T      Length
	)
{
	SIZE_T i;
	UINT32 Value;
	CHAR   AlphaLower;
	UINT32 Nibble;

	//
	// A valid EISAID string is always 7 characters (3 characters, 4 hex digits).
	//
	if( Length != 7 ) {
		return 0;
	}

	//
	// The String must be of the form "UUUNNNN", where "U" is an uppercase letter and "N" is a hexadecimal digit.
	// No asterisks or other characters are allowed in the string.
	//
	for( i = 0, Value = 0; i <= 3; i++ ) {
		AlphaLower = ( String[ 3 + i ] | ( 1 << 5 ) ); /* Force ANSI alphabet characters to lowercase. */
		Nibble = ( ( AlphaLower <= 0x39 ) ? ( AlphaLower - 0x30 ) : ( 0xA + ( AlphaLower - 0x61 ) ) );
		Value |= ( ( Nibble & 0xF ) << ( ( 3 - i ) * 4 ) );
	}
	Value |= ( ( ( ( UINT32 )String[ 2 ] - 0x40ul ) & 0x1F ) << ( 16ul + ( 5 * 0 ) ) );
	Value |= ( ( ( ( UINT32 )String[ 1 ] - 0x40ul ) & 0x1F ) << ( 16ul + ( 5 * 1 ) ) );
	Value |= ( ( ( ( UINT32 )String[ 0 ] - 0x40ul ) & 0x1F ) << ( 16ul + ( 5 * 2 ) ) );
	return AML_BSWAP32( Value );
}

//
// Performs no validation of the input string other than the length, is assumed to be a valid EISAID string
// in the following form: "UUUNNNN", where "U" is an uppercase letter and "N" is a hexadecimal digit.
// The results may not be a valid EISAID value if an invalid string is passed.
//
_Success_( return != 0 )
UINT32
AmlStringZToEisaId(
	_In_z_ const CHAR* String
	)
{
	SIZE_T Length;

	//
	// Calculate the length of the string and pass it along to the regular implementation.
	//
	for( Length = 0; Length < ( SIZE_MAX - 1 ); Length++ ) {
		if( String[ Length ] == '\0' ) {
			break;
		}
	}
	return AmlStringToEisaId( String, Length );
}

//
// Internal function to convert a regular path string to a decoded AML_NAME_STRING.
// This function is internal because it doesn't handle snapshotting the input arena
// to rollback the output allocations upon failure.
//
_Success_( return )
static
BOOLEAN
AmlPathStringToNameStringInternal(
	_Inout_                  AML_ARENA*       Arena,
	_In_count_( PathLength ) const CHAR*      Path,
	_In_                     SIZE_T           PathLength,
	_Out_                    AML_NAME_STRING* NameString
	)
{
	AML_NAME_STRING Name;
	SIZE_T          Cursor;
	SIZE_T          MaxSegCount;
	SIZE_T          SegLength;
	AML_NAME_SEG*   Segments;
	SIZE_T          i;
	AML_NAME_SEG*   Segment;
	BOOLEAN         ExpectNameSeg;
	BOOLEAN         EndNameSeg;
	BOOLEAN         IsLastSegChar;

	//
	// A name string must either contain a prefix or a segment.
	//
	if( PathLength <= 0 ) {
		return AML_FALSE;
	}

	//
	// Default initialize an empty name string, fields will be updated as we process each part of the input path.
	//
	Name = ( AML_NAME_STRING ){ .SegmentCount = 0 };

	//
	// Consume prefixes.
	//
	Name.Prefix.Length = 0;
	for( Cursor = 0; Cursor < PathLength; ) {
		if( Path[ Cursor ] == '\\' ) {
			Name.Prefix.Data[ Name.Prefix.Length++ ] = Path[ Cursor++ ];
			break;
		} else if( Path[ Cursor ] == '^' ) {
			if( Name.Prefix.Length >= AML_COUNTOF( Name.Prefix.Data ) ) {
				return AML_FALSE;
			}
			Name.Prefix.Data[ Name.Prefix.Length++ ] = Path[ Cursor++ ];
		} else {
			break;
		}
	}

	//
	// Allocate the maximum amount of segments required for the input path.
	//
	if( ( PathLength - Cursor ) > ( SIZE_MAX - sizeof( AML_NAME_SEG ) ) ) {
		return AML_FALSE;
	}
	MaxSegCount = ( ( ( ( PathLength - Cursor ) + ( sizeof( AML_NAME_SEG ) - 1 ) ) / sizeof( AML_NAME_SEG ) ) );
	if( MaxSegCount > 0 ) {
		Segments = AmlArenaAllocate( Arena, ( MaxSegCount * sizeof( AML_NAME_SEG ) ) );
		if( Segments == NULL ) {
			return AML_FALSE;
		}
	} else {
		Segments = NULL;
	}
	Name.Segments = Segments;

	//
	// If we the path doesn't have enough space left for a full name segment,
	// but we haven't reach the end of the path, this is an invalid path.
	//
	if( ( MaxSegCount == 0 ) && ( Cursor < PathLength ) ) {
		return AML_FALSE;
	}

	//
	// Consume and validate all name segments of the input path.
	//
	SegLength = 0;
	EndNameSeg = AML_FALSE;
	ExpectNameSeg = AML_FALSE;
	while( Cursor < PathLength ) {
		//
		// Consume a character of the path.
		// A segment must be 4 characters long composed of NameChars and DigitChars.
		// A segment must begin with a NameChar ('A'-'Z' | '_') and not a DigitChar.
		// Multiple name segments must be chained using the '.' character.
		//
		if( ( Path[ Cursor ] >= '0' ) && ( Path[ Cursor ] <= '9' ) ) {
			if( SegLength == 0 ) {
				return AML_FALSE;
			}
			SegLength++;
		} else if( ( Path[ Cursor ] >= 'A' ) && ( Path[ Cursor ] <= 'Z' ) ) {
			SegLength++;
		} else if( Path[ Cursor ] == '_' ) {
			SegLength++;
		} else if( Path[ Cursor ] == '.' ) {
			if( ( SegLength == 0 ) || ( Cursor == Name.Prefix.Length ) ) {
				return AML_FALSE;
			}
			SegLength = 0;
			ExpectNameSeg = AML_TRUE;
		} else {
			return AML_FALSE;
		}

		//
		// Handle invalid segment length. A segment must not exceed 4 characters.
		//
		if( SegLength > AML_COUNTOF( Segments[ 0 ].Data ) ) {
			return AML_FALSE;
		}

		//
		// If we have consumed a full segment, push the information to the name string.
		//
		IsLastSegChar = ( ( ( PathLength - Cursor ) >= 2 ) && ( Path[ Cursor + 1 ] == '.' ) );
		EndNameSeg = ( IsLastSegChar || ( ( PathLength - Cursor ) <= 1 ) || ( SegLength >= AML_COUNTOF( Segments[ 0 ].Data ) ) );
		if( EndNameSeg ) {
			//
			// A completely empty name segment is not valid.
			//
			if( SegLength == 0 ) {
				return AML_FALSE;
			}

			//
			// Accumulate the parsed characters of the current name segment.
			//
			Segment = &Segments[ Name.SegmentCount++ ];
			for( i = 0; i < SegLength; i++ ) {
				Segment->Data[ i ] = Path[ ( Cursor - ( SegLength - 1 - i ) ) ];
			}

			//
			// If the current parsed name segment is below name segment size, pad it with underscores.
			//
			if( SegLength < AML_COUNTOF( Segments[ 0 ].Data ) ) {
				for( i = 0; i < ( AML_COUNTOF( Segments[ 0 ].Data ) - SegLength ); i++ ) {
					Segment->Data[ SegLength + i ] = '_';
				}
			}

			//
			// Reset segment parsing state.
			//
			ExpectNameSeg = AML_FALSE;

			//
			// If there are multiple segments in a name, they must be delimited by a '.' character.
			//
			if( ( ( PathLength - Cursor ) >= 2 ) && ( Path[ Cursor + 1 ] != '.' ) ) {
				return AML_FALSE;
			}
		}

		//
		// Consume the current input path character and move on to the next one.
		//
		Cursor += 1;
	}

	//
	// The string should always end in a name segment, and not a delimiter.
	//
	if( ExpectNameSeg != AML_FALSE ) {
		return AML_FALSE;
	}

	//
	// All segments have been parsed, output the new name string.
	//
	*NameString = Name;
	return AML_TRUE;
}

//
// Convert a regular path string to a decoded AML_NAME_STRING.
//
_Success_( return )
BOOLEAN
AmlPathStringToNameString(
	_Inout_                  AML_ARENA*       Arena,
	_In_count_( PathLength ) const CHAR*      Path,
	_In_                     SIZE_T           PathLength,
	_Out_                    AML_NAME_STRING* NameString
	)
{
	AML_ARENA_SNAPSHOT Snapshot;
	BOOLEAN            Success;

	//
	// Snapshot the arena state and rollback any allocations if we have failed to parse the full path.
	//
	Snapshot = AmlArenaSnapshot( Arena );
	Success  = AmlPathStringToNameStringInternal( Arena, Path, PathLength, NameString );
	Success &= ( Success ? AmlArenaSnapshotCommit( Arena, &Snapshot ) : AmlArenaSnapshotRollback( Arena, &Snapshot ) );
	return Success;
}

//
// Convert a regular null-terminated path string to a decoded AML_NAME_STRING.
//
_Success_( return )
BOOLEAN
AmlPathStringZToNameString(
	_Inout_ AML_ARENA*       Arena,
	_In_z_  const CHAR*      Path,
	_Out_   AML_NAME_STRING* NameString
	)
{
	SIZE_T Length;

	//
	// Calculate the length of the null-terminated string and pass it on the real parser function.
	//
	for( Length = 0; Length < ( SIZE_MAX - 1 ); Length++ ) {
		if( Path[ Length ] == '\0' ) {
			break;
		}
	}
	return AmlPathStringToNameString( Arena, Path, Length, NameString );
}
#include "aml_state.h"
#include "aml_data.h"
#include "aml_object.h"
#include "aml_buffer_field.h"
#include "aml_base.h"

//
// Read bit-granularity data from the given buffer field to the result data array.
// The given ResultDataSize is the max size of the ResultData array in *bytes*.
//
_Success_( return )
BOOLEAN
AmlBufferFieldRead(
	_In_                                 const struct _AML_STATE*         State,
	_In_                                 struct _AML_OBJECT_BUFFER_FIELD* Field,
	_Out_writes_bytes_( ResultDataSize ) VOID*                            ResultData,
	_In_                                 SIZE_T                           ResultDataSize,
	_In_                                 BOOLEAN                          AllowTruncation,
	_Out_opt_                            SIZE_T*                          pResultBitCount
	)
{
	SIZE_T           OutputBitCount;
	SIZE_T           i;
	AML_BUFFER_DATA* Buffer;

	//
	// Ensure that the buffer object referenced by the buffer field is valid.
	//
	if( ( ( Field->SourceBuf.Type != AML_DATA_TYPE_BUFFER )
		  && ( Field->SourceBuf.Type != AML_DATA_TYPE_STRING ) )
		|| ( Field->SourceBuf.u.Buffer == NULL ) )
	{
		return AML_FALSE;
	}

	//
	// Ensure that the buffer object referenced by the field is still large enough to contain the field's data.
	//
	if( ( ( Field->BitIndex + Field->BitCount + ( CHAR_BIT - 1 ) ) / CHAR_BIT ) > Field->SourceBuf.u.Buffer->Size ) {
		return AML_FALSE;
	}

	//
	// Ensure that the output buffer is big enough to fit the field data.
	// If truncation is enabled, truncate the field data size if not big enough.
	//
	OutputBitCount = Field->BitCount;
	if( OutputBitCount > ( ResultDataSize * CHAR_BIT ) ) {
		if( AllowTruncation == AML_FALSE ) {
			return AML_FALSE;
		}
		OutputBitCount = ( ResultDataSize * CHAR_BIT );
	}

	//
	// Zero initialize output by default.
	//
	for( i = 0; i < ResultDataSize; i++ ) {
		( ( CHAR* )ResultData )[ i ] = 0;
	}

	//
	// Copy the possibly truncated amount of bits to the output buffer.
	//
	Buffer = Field->SourceBuf.u.Buffer;
	if( AmlCopyBits( Buffer->Data, Buffer->Size, ResultData,
					 ResultDataSize, Field->BitIndex, Field->BitCount, 0 ) == AML_FALSE )
	{
		return AML_FALSE;
	}

	//
	// Return the possibly truncated amount of bits read.
	//
	if( pResultBitCount != NULL ) {
		*pResultBitCount = OutputBitCount;
	}

	return AML_TRUE;
}

//
// Write data to the given buffer field from the input data array.
// The maximum input data size is passed in bytes, but the actual copy to the field is done at bit-granularity.
//
_Success_( return )
BOOLEAN
AmlBufferFieldWrite(
	_In_                              const struct _AML_STATE*         State,
	_In_                              struct _AML_OBJECT_BUFFER_FIELD* Field,
	_In_reads_bytes_( InputDataSize ) const VOID*                      InputData,
	_In_                              SIZE_T                           InputDataSize,
	_In_                              BOOLEAN                          AllowTruncation
	)
{
	UINT64           InputBitCount;
	AML_BUFFER_DATA* Buffer;
	CHAR             ZeroBuffer[ 16 ];
	UINT64           i;
	UINT64           PadBitCount;
	UINT64           PadChunkSize;

	//
	// Ensure that the buffer object referenced by the buffer field is valid.
	//
	if( ( ( Field->SourceBuf.Type != AML_DATA_TYPE_BUFFER )
		  && ( Field->SourceBuf.Type != AML_DATA_TYPE_STRING ) )
		|| ( Field->SourceBuf.u.Buffer == NULL ) )
	{
		return AML_FALSE;
	}

	//
	// Ensure that the buffer object referenced by the field is still large enough to contain the field's data.
	//
	if( ( Field->SourceBuf.u.Buffer->Size > ( SIZE_MAX / CHAR_BIT ) )
		|| ( ( Field->BitIndex + Field->BitCount ) > ( Field->SourceBuf.u.Buffer->Size * CHAR_BIT ) ) )
	{
		return AML_FALSE;
	}

	//
	// Ensure that the input buffer is big enough to fit the field data.
	// If truncation is enabled, truncate the field data size and zero pad if not big enough.
	//
	InputBitCount = AML_MIN( Field->BitCount, ( InputDataSize * CHAR_BIT ) );
	if( InputBitCount != Field->BitCount ) {
		if( AllowTruncation == AML_FALSE ) {
			return AML_FALSE;
		}
	}

	//
	// Copy input data to the buffer field.
	//
	Buffer = Field->SourceBuf.u.Buffer;
	if( AmlCopyBits( InputData, InputDataSize, Buffer->Data,
					 Buffer->Size, 0, InputBitCount, Field->BitIndex ) == AML_FALSE )
	{
		return AML_FALSE;
	}

	//
	// If truncation is enabled, and we haven't written the entire field, zero pad the rest of the field value.
	//
	if( InputBitCount < Field->BitCount ) {
		for( i = 0; i < AML_COUNTOF( ZeroBuffer ); i++ ) {
			ZeroBuffer[ i ] = 0;
		}
		PadBitCount = ( Field->BitCount - InputBitCount );
		for( i = 0; i < PadBitCount; i += PadChunkSize ) {
			PadChunkSize = AML_MIN( ( PadBitCount - i ), ( sizeof( ZeroBuffer ) * CHAR_BIT ) );
			if( AmlCopyBits( ZeroBuffer, sizeof( ZeroBuffer ),
							 Buffer->Data, Buffer->Size, 0, PadChunkSize,
							 ( Field->BitIndex + InputBitCount + i ) ) == AML_FALSE )
			{
				return AML_FALSE;
			}
		}
	}

	return AML_TRUE;
}
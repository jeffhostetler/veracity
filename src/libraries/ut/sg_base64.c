/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <sg.h>

/*
cencode.h - c header for a base64 encoding algorithm

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

typedef enum
{
	step_A, 
    step_B, 
    step_C
} base64_encodestep;

typedef struct
{
	base64_encodestep step;
	char result;
	int stepcount;
} base64_encodestate;

void base64_init_encodestate( base64_encodestate* state_in);

int base64_encode_block( const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in);

int base64_encode_blockend( char* code_out, base64_encodestate* state_in);

/*
cdecode.h - c header for a base64 decoding algorithm

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

typedef enum
{
	step_a, 
    step_b, 
    step_c, 
    step_d
} base64_decodestep;

typedef struct
{
	base64_decodestep step;
	char plainchar;
} base64_decodestate;

void base64_init_decodestate( base64_decodestate* state_in);

int base64_decode_block(const char* code_in, const int length_in, char* plaintext_out, base64_decodestate* state_in);

END_EXTERN_C

/*
cencoder.c - c source to a base64 encoding algorithm implementation

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

void base64_init_encodestate(base64_encodestate* state_in)
{
	state_in->step = step_A;
	state_in->result = 0;
	state_in->stepcount = 0;
}

char base64_encode_value(char value_in)
{
	static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	if (value_in > 63) return '=';
	return encoding[(int)value_in];
}

int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in)
{
	const char* plainchar = plaintext_in;
	const char* const plaintextend = plaintext_in + length_in;
	char* codechar = code_out;
	char result;
	char fragment;
	
	result = state_in->result;
	
	switch (state_in->step)
	{
		while (1)
		{
	case step_A:
			if (plainchar == plaintextend)
			{
				state_in->result = result;
				state_in->step = step_A;
				return (int) (codechar - code_out);
			}
			fragment = *plainchar++;
			result = (fragment & 0x0fc) >> 2;
			*codechar++ = base64_encode_value(result);
			result = (fragment & 0x003) << 4;
	case step_B:
			if (plainchar == plaintextend)
			{
				state_in->result = result;
				state_in->step = step_B;
				return (int) (codechar - code_out);
			}
			fragment = *plainchar++;
			result |= (fragment & 0x0f0) >> 4;
			*codechar++ = base64_encode_value(result);
			result = (fragment & 0x00f) << 2;
	case step_C:
			if (plainchar == plaintextend)
			{
				state_in->result = result;
				state_in->step = step_C;
				return (int) (codechar - code_out);
			}
			fragment = *plainchar++;
			result |= (fragment & 0x0c0) >> 6;
			*codechar++ = base64_encode_value(result);
			result  = (fragment & 0x03f) >> 0;
			*codechar++ = base64_encode_value(result);
			
			++(state_in->stepcount);
		}
	}
	/* control should not reach here */
    return (int) (codechar - code_out);
}

int base64_encode_blockend(char* code_out, base64_encodestate* state_in)
{
	char* codechar = code_out;
	
	switch (state_in->step)
	{
	case step_B:
		*codechar++ = base64_encode_value(state_in->result);
		*codechar++ = '=';
		*codechar++ = '=';
		break;
	case step_C:
		*codechar++ = base64_encode_value(state_in->result);
		*codechar++ = '=';
		break;
	case step_A:
		break;
	}
	
    return (int) (codechar - code_out);
}

/*
cdecoder.c - c source to a base64 decoding algorithm implementation

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

int base64_decode_value(char value_in)
{
	static const char decoding[] = {62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51};
	static const char decoding_size = sizeof(decoding);
	value_in -= 43;
	if (value_in < 0 || value_in > decoding_size) return -1;
	return decoding[(int)value_in];
}

void base64_init_decodestate(base64_decodestate* state_in)
{
	state_in->step = step_a;
	state_in->plainchar = 0;
}

int base64_decode_block(const char* code_in, const int length_in, char* plaintext_out, base64_decodestate* state_in)
{
	const char* codechar = code_in;
	char* plainchar = plaintext_out;
	char fragment;
	
	*plainchar = state_in->plainchar;
	
	switch (state_in->step)
	{
		while (1)
		{
	case step_a:
			do {
				if (codechar == code_in+length_in)
				{
					state_in->step = step_a;
					state_in->plainchar = *plainchar;
					return (int) (plainchar - plaintext_out);
				}
				fragment = (char)base64_decode_value(*codechar++);
			} while (fragment < 0);
			*plainchar    = (fragment & 0x03f) << 2;
	case step_b:
			do {
				if (codechar == code_in+length_in)
				{
					state_in->step = step_b;
					state_in->plainchar = *plainchar;
					return (int) (plainchar - plaintext_out);
				}
				fragment = (char)base64_decode_value(*codechar++);
			} while (fragment < 0);
			*plainchar++ |= (fragment & 0x030) >> 4;
			*plainchar    = (fragment & 0x00f) << 4;
	case step_c:
			do {
				if (codechar == code_in+length_in)
				{
					state_in->step = step_c;
					state_in->plainchar = *plainchar;
					return (int) (plainchar - plaintext_out);
				}
				fragment = (char)base64_decode_value(*codechar++);
			} while (fragment < 0);
			*plainchar++ |= (fragment & 0x03c) >> 2;
			*plainchar    = (fragment & 0x003) << 6;
	case step_d:
			do {
				if (codechar == code_in+length_in)
				{
					state_in->step = step_d;
					state_in->plainchar = *plainchar;
					return (int) (plainchar - plaintext_out);
				}
				fragment = (char)base64_decode_value(*codechar++);
			} while (fragment < 0);
			*plainchar++   |= (fragment & 0x03f);
		}
	}
	/* control should not reach here */
	return (int) (plainchar - plaintext_out);
}

void SG_base64__space_needed_for_encode(
    SG_context* pCtx,
    SG_uint32 len_binary,
    SG_uint32* p
    )
{
    SG_NULLARGCHECK_RETURN(p);

    *p = 1 + (4 * ((len_binary + 2) / 3));
}

void SG_base64__encode(
    SG_context* pCtx,
    const SG_byte* p_binary,
    SG_uint32 len_binary,
    char* p_encoded_buf,
    SG_uint32 len_encoded_buf
    )
{
    base64_encodestate es;
    char* p = (char*) p_encoded_buf;
    SG_uint32 space_needed = 0;

    SG_NULLARGCHECK_RETURN(p_binary);

    SG_ERR_CHECK_RETURN(  SG_base64__space_needed_for_encode(pCtx, len_binary, &space_needed)  );
    if (len_encoded_buf < space_needed)
    {
        SG_ERR_THROW_RETURN(SG_ERR_BUFFERTOOSMALL);
    }

    base64_init_encodestate(&es);
    p += base64_encode_block((const char*) p_binary, len_binary, p, &es);
    p += base64_encode_blockend(p, &es);
    *p = 0;
}

void SG_base64__space_needed_for_decode(
    SG_context* pCtx,
    const char* p_encoded,
    SG_uint32* pi
    )
{
    SG_uint32 len = 0;

    SG_NULLARGCHECK_RETURN(p_encoded);

    // TODO this could be fancier and check for padding at
    // the end.  also, it could (and perhaps should) verify that
    // the string has no whitespace.

    len = (SG_uint32) strlen(p_encoded);
    *pi = len / 4 * 3;
}

void SG_base64__decode(
    SG_context* pCtx,
    const char* p_encoded,
    SG_byte* p_binary_buf,
    SG_uint32 len_binary_buf,
    SG_uint32* p_actual_len_binary
    )
{
    base64_decodestate ds;
    SG_uint32 len = 0;
    SG_uint32 len_encoded = (SG_uint32) strlen(p_encoded);
    SG_uint32 space_needed = 0;

    SG_ERR_CHECK_RETURN(  SG_base64__space_needed_for_decode(pCtx, p_encoded, &space_needed)  );
    if (len_binary_buf < space_needed)
    {
        SG_ERR_THROW_RETURN(SG_ERR_BUFFERTOOSMALL);
    }

    base64_init_decodestate(&ds);
    len = base64_decode_block(p_encoded, len_encoded, (char*) p_binary_buf, &ds);
    *p_actual_len_binary = len;
}


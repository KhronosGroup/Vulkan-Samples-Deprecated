/*
================================================================================================

Description	:	Base64 encoding/decoding.
Author		:	J.M.P. van Waveren
Date		:	08/26/2016
Language	:	C99
Format		:	Real tabs with the tab size equal to 4 spaces.
Copyright	:	Copyright (c) 2016 Oculus VR, LLC. All Rights reserved.


LICENSE
=======

Copyright (c) 2016 Oculus VR, LLC.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

BASE64
======

Base64 is a binary-to-text encoding scheme that represents binary
data in an ASCII string format by translating it into a radix-64
representation. Every three bytes (3 x 8 = 24 bits) are converted
to four radix-64 numbers (4 x 6 = 24 bits) and stored in the Base64
alphabet as four ASCII characters. Base64 encoding of data is used
in many situations to store or to transfer data in environments that
are restricted to ASCII data.

The commonly used Base64, Base32, and Base16 encoding schemes are
described in RFC 4648. It also discusses the use of line-feeds in
encoded data, use of padding in encoded data, use of non-alphabet
characters in encoded data, use of different encoding alphabets,
and canonical encodings.

https://www.ietf.org/rfc/rfc4648.txt


INTERFACE
=========

int Base64_EncodeSizeInBytes( int dataSizeInBytes );
int Base64_DecodeSizeInBytes( const char * base64, const int base64SizeInBytes );
int Base64_Encode( char * base64, const unsigned char * data, const int dataSizeInBytes );
int Base64_Decode( unsigned char * data, const char * base64, const int base64SizeInBytes );

================================================================================================================================
*/

#if !defined( BASE64_H )
#define BASE64_H

// alphabet character for a radix-64 number
static const char base64_alphabet[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"	// [ 0, 25]
	"abcdefghijklmnopqrstuvwxyz"	// [26, 51]
	"0123456789"					// [52, 61]
	"+/";							// [62, 63]

// radix-64 number for an alphabet character
static const char base64_radix64[] =
{
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 63,  0,  0,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,
	 0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,
	 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static inline int Base64_EncodeSizeInBytes( int dataSizeInBytes )
{
	return ( dataSizeInBytes + 2 ) / 3 * 4;
}

static inline int Base64_DecodeSizeInBytes( const char * base64, const int base64SizeInBytes )
{
	int padding = 0;
	for ( int i = base64SizeInBytes - 1; i >= 0 && base64[i] == '='; i-- )
	{
		padding++;
	}
	return ( ( 3 * base64SizeInBytes + 3 ) / 4 ) - padding;
}

static inline int Base64_Encode( char * base64, const unsigned char * data, const int dataSizeInBytes )
{
	int base64SizeInBytes = 0;
	unsigned char bytes[3];
	int byteCount = 0;

	for ( int i = 0; i < dataSizeInBytes; i++ )
	{
		bytes[byteCount++] = data[i];
		if ( byteCount == 3 || i == dataSizeInBytes - 1 )
		{
			// Pad the input data with zeros up to three bytes.
			for ( int j = byteCount; j < 3; j++ )
			{
				bytes[j] = 0;
			}

			// Convert from three bytes to four radix-64 numbers.
			unsigned char radix64[4];
			radix64[0] = (unsigned char)( ( ( bytes[0] & 0xFC ) >> 2 ) );
			radix64[1] = (unsigned char)( ( ( bytes[0] & 0x03 ) << 4 ) + ( ( bytes[1] & 0xF0 ) >> 4 ) );
			radix64[2] = (unsigned char)( ( ( bytes[1] & 0x0F ) << 2 ) + ( ( bytes[2] & 0xC0 ) >> 6 ) );
			radix64[3] = (unsigned char)( ( ( bytes[2] & 0x3F ) >> 0 ) );

			// Convert from radix-64 to the base64 alphabet.
			for ( int j = 0; j < byteCount + 1; j++ )
			{
				base64[base64SizeInBytes++] = base64_alphabet[radix64[j]];
			}

			// Pad the base64 data with '='.
			for ( int j = byteCount + 1; j < 4; j++ )
			{
				base64[base64SizeInBytes++] = '=';
			}
			byteCount = 0;
		}
	}
	return base64SizeInBytes;
}

static inline int Base64_Decode( unsigned char * data, const char * base64, const int base64SizeInBytes )
{
	int dataSizeInBytes = 0;
	unsigned char alphabet[4];
	int alphabetCount = 0;

	for ( int i = 0; i < base64SizeInBytes; i++ )
	{
		alphabet[alphabetCount++] = base64[i];
		if ( alphabetCount == 4 || i == base64SizeInBytes - 1 )
		{
			// Pad the base64 data with '=' in case there is no padding.
			for ( int j = alphabetCount; j < 4; j++ )
			{
				alphabet[j] = '=';
			}

			// Don't consider any stored padding as alphabet characters.
			while ( alphabetCount > 0 && alphabet[alphabetCount - 1] == '=' )
			{
				alphabetCount--;
			}

			// Convert from the base64 alphabet to radix-64.
			char radix64[4];
			for ( int j = 0; j < 4; j++ )
			{
				radix64[j] = base64_radix64[alphabet[j]];
			}

			// Convert from four radix-64 numbers to three bytes.
			unsigned char bytes[3];
			bytes[0] = (unsigned char)( ( ( radix64[0] & 0xFF ) << 2 ) + ( ( radix64[1] & 0x30 ) >> 4 ) );
			bytes[1] = (unsigned char)( ( ( radix64[1] & 0x0F ) << 4 ) + ( ( radix64[2] & 0x3C ) >> 2 ) );
			bytes[2] = (unsigned char)( ( ( radix64[2] & 0x03 ) << 6 ) + radix64[3] );

			// Store output data.
			for ( int j = 0; j < alphabetCount - 1; j++ )
			{
				data[dataSizeInBytes++] = bytes[j];
			}
			alphabetCount = 0;
		}
	}
	return dataSizeInBytes;
}

#endif // !BASE64_H

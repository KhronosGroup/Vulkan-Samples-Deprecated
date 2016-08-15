/*
================================================================================================

Description	:	Lightweight JSON reader/writer.
Author		:	J.M.P. van Waveren
Date		:	08/07/2016
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


The JavaScript Object Notation (JSON) Data Interchange Format
=============================================================

http://www.json.org/
http://rfc7159.net/rfc7159
http://www.ecma-international.org/publications/files/ECMA-ST/ECMA-404.pdf


DESCRIPTION
===========

This is a Document Object Model (DOM) style JSON implementation where
JSON data is represented in memory as an object tree. The code is written
in straight C99 for maximum portability.

There are various syntactically pretty C++ JSON parsers and writers,
but the syntactical sugar tends to come at a performance cost.
For instance, this JSON implementation is on the order of 3 to 6 times
faster than a std::map based JSON implementation.

It is generally questionable whether a DOM-style JSON implementation
should use a map or hash table to access the members of an object.
An array may have many elements, but these elements are never looked
up by name. The members of an object are looked up by name, but objects
rarely have more than a dozen to a couple of dozen members. Building a
map or hash table for relatively few members tends to be more costly
than just iterating over the members and comparing the names.

As a trivial optimization this implementation continues iterating over
the members of an object where it left off during the last lookup.
If the DOM is traversed in the order it is stored, then this results
in only a single string comparison per lookup. This implementation
keeps objects in the same order in the DOM as they appear in the
JSON text.

This implementation stores the children of an object, or the elements
of an array, as a single flat array in memory. While this may require
re-allocation during construction or parsing, with geometric growth
there are only O(log(N)) allocations and the re-allocation is not nearly
as costly as iterating a linked list of objects that are all separately
allocated. Having a flat array is not only more cache friendly but
also allows direct indexing of an array.

This implementation is more lenient than the JSON specification and
technically invalid JSON text may still parse correctly.

This implementation was designed to be robust, allowing completely
invalid JSON text to be parsed without crashing. All interface
functions continue to work without fatal runtime exceptions, even
if a NULL Json_t pointer is passed in.


INTERFACE
=========

Json_t

Json_t *		Json_Create();
void			Json_Destroy( Json_t * rootNode );

bool			Json_ReadFromBuffer( Json_t * rootNode, const char * buffer, const char ** errorStringOut );
bool			Json_ReadFromFile( Json_t * rootNode, const char * fileName, const char ** errorStringOut );
bool			Json_WriteToBuffer( const Json_t * rootNode, char ** bufferOut, int * lengthOut );
bool			Json_WriteToFile( const Json_t * rootNode, const char * fileName );

int				Json_GetChildCount( const Json_t * parentNode );							// For both objects and arrays.
Json_t *		Json_GetChildByIndex( const Json_t * parentNode, const int index );			// For both objects and arrays.
Json_t *		Json_GetChildByName( const Json_t * parentNode, const char * childName );	// Only for objects with case-sensitive lookups.
Json_t *		Json_AddChild( Json_t * parentNode, const char * childName );				// For both objects and arrays where 'childName' is ignored for arrays.

bool			Json_IsNull( const Json_t * node );
bool			Json_IsBoolean( const Json_t * node );
bool			Json_IsNumber( const Json_t * node );
bool			Json_IsInteger( const Json_t * node );
bool			Json_IsFloatingPoint( const Json_t * node );
bool			Json_IsString( const Json_t * node );
bool			Json_IsObject( const Json_t * node );
bool			Json_IsArray( const Json_t * node );

bool			Json_GetBoolean( const Json_t * node, const bool defaultValue );
int				Json_GetInt32( const Json_t * node, const int defaultValue );
long long		Json_GetInt64( const Json_t * node, const long long defaultValue );
float			Json_GetFloat( const Json_t * node, const float defaultValue );
double			Json_GetDouble( const Json_t * node, const double defaultValue );
const char *	Json_GetString( const Json_t * node, const char * defaultValue );

Json_t *		Json_SetBoolean( Json_t * node, const bool value );
Json_t *		Json_SetInt32( Json_t * node, const int value );
Json_t *		Json_SetInt64( Json_t * node, const long long value );
Json_t *		Json_SetFloat( Json_t * node, const float value );
Json_t *		Json_SetDouble( Json_t * node, const double value );
Json_t *		Json_SetString( Json_t * node, const char * value );
Json_t *		Json_SetObject( Json_t * node );
Json_t *		Json_SetArray( Json_t * node );


EXAMPLES
========

char * buffer = NULL;
int length = 0;

{
	Json_t * rootNode = Json_SetObject( Json_Create() );
	Json_t * vertices = Json_SetArray( Json_AddChild( rootNode, "vertices" ) );
	for ( int i = 0; i < 3; i++ )
	{
		Json_t * vertex = Json_SetObject( Json_AddChild( vertices, NULL ) );

		Json_t * position = Json_SetObject( Json_AddChild( vertex, "position" ) );
		Json_SetFloat( Json_AddChild( position, "x" ), (float)( ( i & 1 ) >> 0 ) );
		Json_SetFloat( Json_AddChild( position, "y" ), (float)( ( i & 2 ) >> 1 ) );
		Json_SetFloat( Json_AddChild( position, "z" ), (float)( ( i & 4 ) >> 2 ) );

		Json_t * normal = Json_SetObject( Json_AddChild( vertex, "normal" ) );
		Json_SetFloat( Json_AddChild( normal, "x" ), 0.5f );
		Json_SetFloat( Json_AddChild( normal, "y" ), 0.6f );
		Json_SetFloat( Json_AddChild( normal, "z" ), 0.7f );
	}
	Json_t * indices = Json_SetArray( Json_AddChild( rootNode, "indices" ) );
	for ( int i = 0; i < 3; i++ )
	{
		Json_SetInt32( Json_AddChild( indices, NULL ), i );
	}
	Json_WriteToBuffer( rootNode, &buffer, &length );
	Json_Destroy( rootNode );
}

{
	Json_t * rootNode = Json_Create();
	if ( Json_ReadFromBuffer( rootNode, buffer, NULL ) )
	{
		const Json_t * vertices = Json_GetChildByName( rootNode, "vertices" );
		if ( Json_IsArray( vertices ) )
		{
			for ( int i = 0; i < Json_GetChildCount( vertices ); i++ )
			{
				const Json_t * vertex = Json_GetChildByIndex( vertices, i );

				const Json_t * position = Json_GetChildByName( vertex, "position" );
				const float position_x = Json_GetFloat( Json_GetChildByName( position, "x" ), 0.0f );
				const float position_y = Json_GetFloat( Json_GetChildByName( position, "y" ), 0.0f );
				const float position_z = Json_GetFloat( Json_GetChildByName( position, "z" ), 0.0f );
				(void)position_x; (void)position_y; (void)position_z;

				const Json_t * normal = Json_GetChildByName( vertex, "normal" );
				const float normal_x = Json_GetFloat( Json_GetChildByName( normal, "x" ), 0.0f );
				const float normal_y = Json_GetFloat( Json_GetChildByName( normal, "y" ), 0.0f );
				const float normal_z = Json_GetFloat( Json_GetChildByName( normal, "z" ), 0.0f );
				(void)normal_x; (void)normal_y; (void)normal_z;
			}
		}
		const Json_t * indices = Json_GetChildByName( rootNode, "indices" );
		if ( Json_IsArray( indices ) )
		{
			for ( int i = 0; i < Json_GetChildCount( indices ); i++ )
			{
				const int index = Json_GetInt32( Json_GetChildByIndex( indices, i ), 0 );
				(void)index;
			}
		}
	}
	Json_Destroy( rootNode );
}


BENCHMARK
=========

The following implementations were tested:
                                                              Case       Supports  Supports  Member       Reorders
Name       Source                                   Language  sensitive  longlong  double    storage      members
------------------------------------------------------------------------------------------------------------------
Json_t     this file                                C99       yes        yes       yes       array        no
rapidjson  https://github.com/miloyip/rapidjson     C++11     yes        yes       yes       array        no
cJSON      https://github.com/DaveGamble/cJSON      C99       no         no        yes       linked-list  no
OVR::JSON  https://developer3.oculus.com/downloads  C++0x     yes        no        yes       linked-list  no
jsoncons   http://danielaparker.github.io/jsoncons  C++11     yes        yes       yes       std::vector  no
json11     https://github.com/dropbox/json11        C++11     yes        yes       yes       std::map     no
nlohmann   https://github.com/nlohmann/json         C++11     yes        yes       yes       std::map     no

Minor changes had to made to some of these implementations to compile with GCC and Clang.
- Exception handling was disabled in rapidjson.
- Alternate implementations for std::to_string, std::stoi, std::strtof, std::strtold in nlohmann.

This benchmark shows the time in milliseconds it takes to parse 'twitter_public_timeline.json' 1000 times.

  https://github.com/TouchCode/TouchJSON/blob/master/Support/Test%20Data/twitter_public_timeline.json

----------------------------------------------------------------------------------------------------------
Text to DOM     2.6 GHz        2.6 GHz        2.1 GHz        2.1 GHz        2.1 GHz        2.1 GHz 
                Intel Core i7  Intel Core i7  Qualcomm Kryo  Qualcomm Kryo  Qualcomm Kryo  Qualcomm Kryo
                win32          win64          armeabi-v7a    arm64-v8a      armeabi-v7a    arm64-v8a
                VS2015-up3     VS2015-up3     GCC 4.9        GCC 4.9        Clang 3.8      Clang 3.8
				c++11          c++11          gnustl_static  gnustl_static  c++_static     c++_static
----------------------------------------------------------------------------------------------------------
  Json_t        121.930 ms     107.169 ms      260.471 ms     229.713 ms     295.828 ms     259.748 ms
  rapidjson     111.411 ms     112.965 ms      294.813 ms     217.756 ms     322.271 ms     267.120 ms
  cJSON         188.701 ms     169.897 ms      363.644 ms     315.248 ms     423.802 ms     319.888 ms
  OVR::JSON     329.450 ms     294.698 ms      791.750 ms     883.271 ms     812.664 ms     879.435 ms
  jsoncons      289.972 ms     230.264 ms      616.342 ms     582.109 ms     618.514 ms     451.056 ms
  json11        507.506 ms     458.889 ms     2592.904 ms    2327.689 ms    2004.852 ms    1476.452 ms
  nlohmann      470.255 ms     419.529 ms     1305.589 ms    1296.296 ms    1410.566 ms     988.472 ms

----------------------------------------------------------------------------------------------------------
Traverse DOM    2.6 GHz        2.6 GHz        2.1 GHz        2.1 GHz        2.1 GHz        2.1 GHz 
                Intel Core i7  Intel Core i7  Qualcomm Kryo  Qualcomm Kryo  Qualcomm Kryo  Qualcomm Kryo
                win32          win64          armeabi-v7a    arm64-v8a      armeabi-v7a    arm64-v8a
                VS2015-up3     VS2015-up3     GCC 4.9        GCC 4.9        Clang 3.8      Clang 3.8
				c++11          c++11          gnustl_static  gnustl_static  c++_static     c++_static
----------------------------------------------------------------------------------------------------------
  Json_t         22.131 ms      19.727 ms      106.081 ms      39.828 ms      87.568 ms      42.936 ms
  rapidjson      40.618 ms      29.887 ms       82.376 ms      48.170 ms      75.268 ms      49.578 ms
  cJSON         271.376 ms     330.203 ms      522.007 ms     468.991 ms     600.157 ms     571.723 ms
  OVR::JSON      55.858 ms      55.793 ms      334.563 ms     205.061 ms     332.576 ms     202.970 ms
  jsoncons      150.714 ms     137.073 ms      542.477 ms     421.545 ms     963.076 ms     796.742 ms
  json11        139.426 ms     123.035 ms      576.560 ms     449.121 ms     462.324 ms     292.826 ms
  nlohmann      102.583 ms      95.926 ms      401.062 ms     501.726 ms     267.339 ms     169.525 ms

================================================================================================================================
*/

#if !defined( JSON_H )
#define JSON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>

// JSON value type
typedef enum
{
	JSON_NONE		= 0,
	JSON_NULL		= 1,	// null
	JSON_BOOLEAN	= 2,	// true | false
	JSON_INTEGER	= 3,	// integer
	JSON_FLOAT		= 4,	// integer[.fraction][exponent]
	JSON_STRING		= 5,	// "string"
	JSON_OBJECT		= 6,	// { "name" : <value>, "name" : <value>, ... }
	JSON_ARRAY		= 7		// [ <value>, <value>, ... ]
} JsonType_t;

// JSON value
// 32-bit sizeof( Json_t ) = 32
// 64-bit sizeof( Json_t ) = 32
typedef struct Json_t
{
	union
	{
		char *			name;			// only != NULL for named members
		long long		pad;			// make the structure size the same between 32-bit and 64-bit
	};
	union
	{
		long long		valueInt64;		// 64-bit integer value
		double			valueDouble;	// 64-bit floating-point value
		char *			valueString;	// string value
		struct Json_t *	children;		// object/array children
	};
	JsonType_t		type;				// type of value
	int				childrenAllocated;	// number of allocated children
	int				childCount;			// number of actual children
	int				childIndex;			// mutable child index for faster lookups
} Json_t;

Json_t * Json_Create()
{
	Json_t * json = (Json_t *) malloc( sizeof( Json_t ) );
	memset( json, 0, sizeof( Json_t ) );
	return json;
}

static Json_t * Json_AllocChild( Json_t * parentNode )
{
	if ( parentNode->childCount >= parentNode->childrenAllocated )
	{
		const int newAllocated = ( parentNode->childrenAllocated <= 0 ) ? 8 : parentNode->childrenAllocated * 2;
		Json_t * newChildren = (Json_t *) malloc( newAllocated * sizeof( Json_t ) );
		if ( parentNode->children != NULL )
		{
			memcpy( newChildren, parentNode->children, parentNode->childrenAllocated * sizeof( Json_t ) );
			free( parentNode->children );
		}
		parentNode->children = newChildren;
		parentNode->childrenAllocated = newAllocated;
	}
	Json_t * child = &parentNode->children[parentNode->childCount++];
	child->name = NULL;
	child->valueInt64 = 0;
	child->type = JSON_NONE;
	child->childrenAllocated = 0;
	child->childCount = 0;
	child->childIndex = 0;
	return child;
}

static void Json_FreeNode( Json_t * node, const bool freeName )
{
	if ( freeName )
	{
		free( node->name );
		node->name = NULL;
	}
	if ( node->type == JSON_OBJECT || node->type == JSON_ARRAY )
	{
		for ( int c = 0; c < node->childCount; c++ )
		{
			Json_FreeNode( &node->children[c], true );
		}
		free( node->children );
	}
	else if ( node->type == JSON_STRING )
	{
		free( node->valueString );
	}
	node->valueInt64 = 0;
	node->type = JSON_NONE;
	node->childrenAllocated = 0;
	node->childCount = 0;
	node->childIndex = 0;
}

static void Json_Destroy( Json_t * rootNode )
{
	if ( rootNode != NULL /* && is an actual root */ )
	{
		Json_FreeNode( rootNode, true );
		free( rootNode );
	}
}

// Parses white space.
// Returns a pointer to the first character after the white space.
static const char * Json_ParseWhiteSpace( const char * buffer )
{
	while ( buffer[0] != '\0' && (unsigned char)buffer[0] <= ' ' )
	{
		buffer++;
	}
	return buffer;
}

// Parses a hexadecimal string up to the specified number of digits and stores the integer result in 'value'.
// Returns a pointer to the first character after the string.
static const char * Json_ParseHex( unsigned int * value, const unsigned int digitCount, const char * buffer )
{
	int v = 0;
	for( unsigned int digit = 0; digit < digitCount; digit++, buffer++ )
	{
		unsigned int d = buffer[0];

		if ( ( d >= '0' ) && ( d <= '9' ) ) d = d - '0';
		else if ( ( d >= 'a') && ( d <= 'f' ) ) d = 10 + d - 'a';
		else if ( ( d >= 'A' ) && ( d <= 'F' ) ) d = 10 + d - 'A';
		else break;

		v = v * 16 + d;
	}
	*value = v;
	return buffer;
}

static const char json_escape[256] =
{
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,'\"',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '/',
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,'\\',   0,   0,   0,
	0,   0,'\b',   0,   0,   0,'\f',   0,   0,   0,   0,   0,   0,   0,'\n',   0,
	0,   0,'\r',   0,'\t',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

static const unsigned char json_firstByteMark[7] =
{
	0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};

// Parses a string and stores the result in 'value'.
// Returns a pointer to the first character after the string.
static const char * Json_ParseString( char ** value, const char * buffer, const char ** errorStringOut )
{
	assert( buffer[0] == '\"' );
	buffer++;

	int length = 0;
	for ( const char * str = buffer; *str != '\"' && *str != '\0'; length++ )
	{
		// Collapse escaped characters and skip escaped quotes.
		if ( str[0] == '\\' && str[1] != '\0' ) str++;
		str++;
	}

	char * out = (char *) malloc( length + 1 );
	char * outPtr = out;

	while ( buffer[0] != '\"' && buffer[0] != '\0' )
	{
		if ( buffer[0] != '\\' )
		{
			*outPtr++ = *buffer++;
		}
		else if ( json_escape[(unsigned char)buffer[1]] )
		{
			*outPtr++ = json_escape[(unsigned char)buffer[1]];
			buffer += 2;
		}
		else if ( buffer[1] == 'u' )
		{
			// Transcode UTF16 to UTF8.
			unsigned int uc = 0;
			buffer = Json_ParseHex( &uc, 4, buffer + 2 );
			if ( ( uc >= 0xDC00 && uc <= 0xDFFF ) || uc == 0 )
			{
				free( out );
				*errorStringOut = "invalid unicode";
				return buffer;
			}

			// UTF16 surrogate pairs.
			if ( uc >= 0xD800 && uc <= 0xDBFF )
			{
				// Second-half of surrogate.
				if ( buffer[0] == '\\' && buffer[1] == 'u' )
				{
					unsigned int uc2 = 0;
					buffer = Json_ParseHex( &uc2, 4, buffer + 2 );
					if ( ( uc2 >= 0xDC00 && uc2 <= 0xDFFF ) || uc2 == 0 )
					{
						free( out );
						*errorStringOut = "invalid unicode";
						return buffer;
					}
					uc = 0x10000 + ( ( ( uc & 0x3FF ) << 10 ) | ( uc2 & 0x3FF ) );
				}
			}

			int outLength = 4;
                    
			if ( uc < 0x80 )
			{
				outLength = 1;
			}
			else if ( uc < 0x800 )
			{
				outLength = 2;
			}
			else if ( uc < 0x10000 )
			{
				outLength = 3;
			}
			outPtr += outLength;
                    
			switch ( outLength )
			{
				case 4: *--outPtr = (char)( ( uc | 0x80 ) & 0xBF ); uc >>= 6;	// fall through
				case 3: *--outPtr = (char)( ( uc | 0x80 ) & 0xBF ); uc >>= 6;	// fall through
				case 2: *--outPtr = (char)( ( uc | 0x80 ) & 0xBF ); uc >>= 6;	// fall through
				case 1: *--outPtr = (char)( uc | json_firstByteMark[outLength] );
			}
			outPtr += outLength;
		}
		else
		{
			*outPtr++ = *buffer++;
		}
	}

	*outPtr = 0;
	if ( *buffer != '\"' )
	{
		free( out );
		*errorStringOut = "missing trailing quote";
		return buffer;
	}
	buffer++;

	*value = out;

	return buffer;
}

// from 10^-308 to 10^+308
static const double json_pow10[] =
{
	1e-308,1e-307,1e-306,1e-305,
	1e-304,1e-303,1e-302,1e-301,1e-300,1e-299,1e-298,1e-297,1e-296,1e-295,1e-294,1e-293,1e-292,1e-291,1e-290,1e-289,
	1e-288,1e-287,1e-286,1e-285,1e-284,1e-283,1e-282,1e-281,1e-280,1e-279,1e-278,1e-277,1e-276,1e-275,1e-274,1e-273,
	1e-272,1e-271,1e-270,1e-269,1e-268,1e-267,1e-266,1e-265,1e-264,1e-263,1e-262,1e-261,1e-260,1e-259,1e-258,1e-257,
	1e-256,1e-255,1e-254,1e-253,1e-252,1e-251,1e-250,1e-249,1e-248,1e-247,1e-246,1e-245,1e-244,1e-243,1e-242,1e-241,
	1e-240,1e-239,1e-238,1e-237,1e-236,1e-235,1e-234,1e-233,1e-232,1e-231,1e-230,1e-229,1e-228,1e-227,1e-226,1e-225,
	1e-224,1e-223,1e-222,1e-221,1e-220,1e-219,1e-218,1e-217,1e-216,1e-215,1e-214,1e-213,1e-212,1e-211,1e-210,1e-209,
	1e-208,1e-207,1e-206,1e-205,1e-204,1e-203,1e-202,1e-201,1e-200,1e-199,1e-198,1e-197,1e-196,1e-195,1e-194,1e-193,
	1e-192,1e-191,1e-190,1e-189,1e-188,1e-187,1e-186,1e-185,1e-184,1e-183,1e-182,1e-181,1e-180,1e-179,1e-178,1e-177,
	1e-176,1e-175,1e-174,1e-173,1e-172,1e-171,1e-170,1e-169,1e-168,1e-167,1e-166,1e-165,1e-164,1e-163,1e-162,1e-161,
	1e-160,1e-159,1e-158,1e-157,1e-156,1e-155,1e-154,1e-153,1e-152,1e-151,1e-150,1e-149,1e-148,1e-147,1e-146,1e-145,
	1e-144,1e-143,1e-142,1e-141,1e-140,1e-139,1e-138,1e-137,1e-136,1e-135,1e-134,1e-133,1e-132,1e-131,1e-130,1e-129,
	1e-128,1e-127,1e-126,1e-125,1e-124,1e-123,1e-122,1e-121,1e-120,1e-119,1e-118,1e-117,1e-116,1e-115,1e-114,1e-113,
	1e-112,1e-111,1e-110,1e-109,1e-108,1e-107,1e-106,1e-105,1e-104,1e-103,1e-102,1e-101,1e-100, 1e-99, 1e-98, 1e-97,
	 1e-96, 1e-95, 1e-94, 1e-93, 1e-92, 1e-91, 1e-90, 1e-89, 1e-88, 1e-87, 1e-86, 1e-85, 1e-84, 1e-83, 1e-82, 1e-81,
	 1e-80, 1e-79, 1e-78, 1e-77, 1e-76, 1e-75, 1e-74, 1e-73, 1e-72, 1e-71, 1e-70, 1e-69, 1e-68, 1e-67, 1e-66, 1e-65,
	 1e-64, 1e-63, 1e-62, 1e-61, 1e-60, 1e-59, 1e-58, 1e-57, 1e-56, 1e-55, 1e-54, 1e-53, 1e-52, 1e-51, 1e-50, 1e-49,
	 1e-48, 1e-47, 1e-46, 1e-45, 1e-44, 1e-43, 1e-42, 1e-41, 1e-40, 1e-39, 1e-38, 1e-37, 1e-36, 1e-35, 1e-34, 1e-33,
	 1e-32, 1e-31, 1e-30, 1e-29, 1e-28, 1e-27, 1e-26, 1e-25, 1e-24, 1e-23, 1e-22, 1e-21, 1e-20, 1e-19, 1e-18, 1e-17,
	 1e-16, 1e-15, 1e-14, 1e-13, 1e-12, 1e-11, 1e-10,  1e-9,  1e-8,  1e-7,  1e-6,  1e-5,  1e-4,  1e-3,  1e-2,  1e-1,
	  1e+0,  1e+1,  1e+2,  1e+3,  1e+4,  1e+5,  1e+6,  1e+7,  1e+8,  1e+9, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15,
	 1e+16, 1e+17, 1e+18, 1e+19, 1e+20, 1e+21, 1e+22, 1e+23, 1e+24, 1e+25, 1e+26, 1e+27, 1e+28, 1e+29, 1e+30, 1e+31,
	 1e+32, 1e+33, 1e+34, 1e+35, 1e+36, 1e+37, 1e+38, 1e+39, 1e+40, 1e+41, 1e+42, 1e+43, 1e+44, 1e+45, 1e+46, 1e+47,
	 1e+48, 1e+49, 1e+50, 1e+51, 1e+52, 1e+53, 1e+54, 1e+55, 1e+56, 1e+57, 1e+58, 1e+59, 1e+60, 1e+61, 1e+62, 1e+63,
	 1e+64, 1e+65, 1e+66, 1e+67, 1e+68, 1e+69, 1e+70, 1e+71, 1e+72, 1e+73, 1e+74, 1e+75, 1e+76, 1e+77, 1e+78, 1e+79,
	 1e+80, 1e+81, 1e+82, 1e+83, 1e+84, 1e+85, 1e+86, 1e+87, 1e+88, 1e+89, 1e+90, 1e+91, 1e+92, 1e+93, 1e+94, 1e+95,
	 1e+96, 1e+97, 1e+98, 1e+99,1e+100,1e+101,1e+102,1e+103,1e+104,1e+105,1e+106,1e+107,1e+108,1e+109,1e+110,1e+111,
	1e+112,1e+113,1e+114,1e+115,1e+116,1e+117,1e+118,1e+119,1e+120,1e+121,1e+122,1e+123,1e+124,1e+125,1e+126,1e+127,
	1e+128,1e+129,1e+130,1e+131,1e+132,1e+133,1e+134,1e+135,1e+136,1e+137,1e+138,1e+139,1e+140,1e+141,1e+142,1e+143,
	1e+144,1e+145,1e+146,1e+147,1e+148,1e+149,1e+150,1e+151,1e+152,1e+153,1e+154,1e+155,1e+156,1e+157,1e+158,1e+159,
	1e+160,1e+161,1e+162,1e+163,1e+164,1e+165,1e+166,1e+167,1e+168,1e+169,1e+170,1e+171,1e+172,1e+173,1e+174,1e+175,
	1e+176,1e+177,1e+178,1e+179,1e+180,1e+181,1e+182,1e+183,1e+184,1e+185,1e+186,1e+187,1e+188,1e+189,1e+190,1e+191,
	1e+192,1e+193,1e+194,1e+195,1e+196,1e+197,1e+198,1e+199,1e+200,1e+201,1e+202,1e+203,1e+204,1e+205,1e+206,1e+207,
	1e+208,1e+209,1e+210,1e+211,1e+212,1e+213,1e+214,1e+215,1e+216,1e+217,1e+218,1e+219,1e+220,1e+221,1e+222,1e+223,
	1e+224,1e+225,1e+226,1e+227,1e+228,1e+229,1e+230,1e+231,1e+232,1e+233,1e+234,1e+235,1e+236,1e+237,1e+238,1e+239,
	1e+240,1e+241,1e+242,1e+243,1e+244,1e+245,1e+246,1e+247,1e+248,1e+249,1e+250,1e+251,1e+252,1e+253,1e+254,1e+255,
	1e+256,1e+257,1e+258,1e+259,1e+260,1e+261,1e+262,1e+263,1e+264,1e+265,1e+266,1e+267,1e+268,1e+269,1e+270,1e+271,
	1e+272,1e+273,1e+274,1e+275,1e+276,1e+277,1e+278,1e+279,1e+280,1e+281,1e+282,1e+283,1e+284,1e+285,1e+286,1e+287,
	1e+288,1e+289,1e+290,1e+291,1e+292,1e+293,1e+294,1e+295,1e+296,1e+297,1e+298,1e+299,1e+300,1e+301,1e+302,1e+303,
	1e+304,1e+305,1e+306,1e+307,1e+308
};

// Parses a number and stores the result in 'valueInt64' or 'valueDouble'.
// Returns a pointer to the first character after the number.
static const char * Json_ParseNumber( JsonType_t * type, long long * valueInt64, double * valueDouble, const char * buffer, const char ** errorStringOut )
{
	(void)errorStringOut;

	int sign = 1;
	int fractionalDigits = 0;
	int exponentSign = 1;
	int exponentValue = 0;
	long long int64Value = 0;
	double doubleValue = 0.0;

	if ( buffer[0] == '-' )
	{
		sign = -1;
		buffer++;
	}
	else if ( buffer[0] == '+' )
	{
		sign = 1;
		buffer++;
	}

	// Skip leading zeros and parse integer digits.
	while ( buffer[0] >= '0' && buffer[0] <= '9' )
	{
		int64Value = ( int64Value * 10 ) + ( buffer[0] - '0' );
		doubleValue = ( doubleValue * 10.0 ) + ( buffer[0] - '0' );
		buffer++;
	}
	// Parse fractional digits.
	if ( buffer[0] == '.' )
	{
		buffer++;
		while ( buffer[0] >= '0' && buffer[0] <= '9' )
		{
			doubleValue = ( doubleValue * 10.0 ) + ( buffer[0] - '0' );
			fractionalDigits++;
			buffer++;
		}
	}
	// Parse exponent.
	if ( buffer[0] == 'e' || buffer[0] == 'E' )
	{
		buffer++;
		exponentSign = 1;
		if ( buffer[0] == '+' )
		{
			exponentSign = 1;
			buffer++;
		}
		else if ( buffer[0] == '-' )
		{
			exponentSign = -1;
			buffer++;
		}
		while ( buffer[0] >= '0' && buffer[0] <= '9' )
		{
			exponentValue = ( exponentValue * 10 ) + ( buffer[0] - '0' );
			buffer++;
		}
	}

	if ( fractionalDigits != 0 || exponentValue != 0 )
	{
		const int exp = exponentSign * exponentValue - fractionalDigits;
		*type = JSON_FLOAT;
		*valueDouble = sign * doubleValue * json_pow10[308 + ( exp >= -308 ? ( exp <= 308 ? exp : 308 ) : -308 )];
	}
	else
	{
		*type = JSON_INTEGER;
		*valueInt64 = sign * int64Value;
	}

	return buffer;
}

static const char * Json_ParseValue( Json_t * json, const char * buffer, const char ** errorStringOut )
{
	assert( errorStringOut != NULL );

	buffer = Json_ParseWhiteSpace( buffer );

	if ( buffer[0] == 'n' )
	{
		assert( strncmp( buffer, "null", 4 ) == 0 );
		json->type = JSON_NULL;
		json->valueString = (char *)"null";
		return buffer + 4;
	}
	else if ( buffer[0] == 'f' )
	{ 
		assert( strncmp( buffer, "false", 5 ) == 0 );
		json->type = JSON_BOOLEAN;
		json->valueString = (char *)"false";
		return buffer + 5;
	}
	else if ( buffer[0] == 't' )
	{
		assert( strncmp( buffer, "true", 4 ) == 0 );
		json->type = JSON_BOOLEAN;
		json->valueString = (char *)"true";
		return buffer + 4;
	}
	else if ( buffer[0] == '\"' )
	{
		json->type = JSON_STRING;
		return Json_ParseString( &json->valueString, buffer, errorStringOut );
	}
	else if ( buffer[0] == '{' )
	{
		json->type = JSON_OBJECT;

		buffer++;
		while ( *errorStringOut == NULL )
		{
			buffer = Json_ParseWhiteSpace( buffer );
			if ( buffer[0] == '}' )
			{
				buffer++;
				break;
			}
			if ( buffer[0] == ',' )
			{
				buffer++;
			}
			Json_t * child = Json_AllocChild( json );
			buffer = Json_ParseWhiteSpace( buffer );
			buffer = Json_ParseString( &child->name, buffer, errorStringOut );
			buffer = Json_ParseWhiteSpace( buffer );
			if ( buffer[0] != ':' )
			{
				*errorStringOut = "missing colon";
				return buffer;
			}
			buffer++;
			buffer = Json_ParseValue( child, buffer, errorStringOut );
		}
		return buffer;
	}
	else if ( buffer[0] == '[' )
	{ 
		json->type = JSON_ARRAY;

		buffer++;
		while ( *errorStringOut == NULL )
		{
			buffer = Json_ParseWhiteSpace( buffer );
			if ( buffer[0] == ']' )
			{
				buffer++;
				break;
			}
			if ( buffer[0] == ',' )
			{
				buffer++;
			}
			Json_t * child = Json_AllocChild( json );
			buffer = Json_ParseValue( child, buffer, errorStringOut );
		}
		return buffer;
	}
	else
	{
		return Json_ParseNumber( &json->type, &json->valueInt64, &json->valueDouble, buffer, errorStringOut );
	}

	*errorStringOut = "invalid value";
	return buffer;
}

static bool Json_ReadFromBuffer( Json_t * rootNode, const char * buffer, const char ** errorStringOut )
{
	if ( rootNode == NULL || buffer == NULL )
	{
		return false;
	}
	if ( errorStringOut != NULL )
	{
		*errorStringOut = NULL;
	}
	Json_FreeNode( rootNode, true );

	const char * error = NULL;
	Json_ParseValue( rootNode, buffer, &error );
	if ( error != NULL )
	{
		if ( errorStringOut != NULL )
		{
			*errorStringOut = error;
		}
		Json_FreeNode( rootNode, true );
		return false;
	}
	return true;
}

static bool Json_ReadFromFile( Json_t * rootNode, const char * fileName, const char ** errorStringOut )
{
	if ( rootNode == NULL || fileName == NULL )
	{
		return false;
	}
	if ( errorStringOut != NULL )
	{
		*errorStringOut = NULL;
	}
	Json_FreeNode( rootNode, true );

	FILE * file = fopen( fileName, "rb" );
	if ( file == NULL )
	{
		if ( errorStringOut != NULL )
		{
			*errorStringOut = "failed to open file";
		}
		return false;
	}

	fseek( file, 0L, SEEK_END );
	size_t bufferSize = ftell( file );
	fseek( file, 0L, SEEK_SET );

	char * buffer = (char *) malloc( bufferSize + 1 );
	if ( fread( buffer, 1, bufferSize, file ) != bufferSize )
	{
		if ( errorStringOut != NULL )
		{
			*errorStringOut = "failed to read file";
		}
		free( buffer );
		fclose( file );
		return false;
	}
	buffer[bufferSize] = '\0';	// make sure the buffer is zero terminated
	fclose( file );

	const char * error = NULL;
	Json_ParseValue( rootNode, buffer, &error );
	if ( error != NULL )
	{
		if ( errorStringOut != NULL )
		{
			*errorStringOut = error;
		}
		Json_FreeNode( rootNode, true );
		free( buffer );
		return false;
	}

	free( buffer );
	return true;
}

static void Json_Printf( char ** bufferInOut, int * lengthInOut, int * offsetInOut, const int extraLength, const char * format, ... )
{
	if ( *offsetInOut + extraLength + 1 > *lengthInOut )
	{
		int newLength = ( *lengthInOut <= 0 ) ? 128 : *lengthInOut * 2;
		while ( newLength < *offsetInOut + extraLength + 1 )
		{
			newLength *= 2;
		}
		char * newBuffer = (char *) malloc( newLength );
		memcpy( newBuffer, *bufferInOut, *offsetInOut );
		free( *bufferInOut );
		*bufferInOut = newBuffer;
		*lengthInOut = newLength;
	}
	va_list args;
	va_start( args, format );
	*offsetInOut += vsprintf( &(*bufferInOut)[*offsetInOut], format, args );
	va_end( args );
	assert( *offsetInOut <= *lengthInOut );
}

static void Json_WriteValue( const Json_t * node, char ** bufferInOut, int * lengthInOut, int * offsetInOut, const int indent, const bool lastChild )
{
	const int maxIndent = 32;
	const char * indentTable = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	assert( indent <= maxIndent );

	if ( node->type == JSON_NULL || node->type == JSON_BOOLEAN )
	{
		Json_Printf( bufferInOut, lengthInOut, offsetInOut, (int)strlen( node->valueString ) + 2, "%s%s\n", node->valueString, lastChild ? "" : "," );
	}
	else if ( node->type == JSON_INTEGER )
	{
		char temp[1024];
		const int length = sprintf( temp, "%lld", node->valueInt64 );
		Json_Printf( bufferInOut, lengthInOut, offsetInOut, length + 2, "%s%s\n", temp, lastChild ? "" : "," );
	}
	else if ( node->type == JSON_FLOAT )
	{
		char temp[1024];
		const int length = sprintf( temp, "%f", node->valueDouble );
		Json_Printf( bufferInOut, lengthInOut, offsetInOut, length + 2, "%s%s\n", temp, lastChild ? "" : "," );
	}
	else if ( node->type == JSON_STRING )
	{
		for ( const char * ptr = node->valueString; ptr[0] != '\0'; ptr++ )
		{
			if ( (unsigned char)ptr[0] < ' ' || ptr[0] == '\"' || ptr[0] == '\\' )
			{
				switch ( ptr[0] )
				{
					case '\\': Json_Printf( bufferInOut, lengthInOut, offsetInOut, 2, "\\\\" ); break;
					case '\"': Json_Printf( bufferInOut, lengthInOut, offsetInOut, 2, "\\\"" ); break;
					case '\b': Json_Printf( bufferInOut, lengthInOut, offsetInOut, 2, "\\b" ); break;
					case '\f': Json_Printf( bufferInOut, lengthInOut, offsetInOut, 2, "\\f" ); break;
					case '\n': Json_Printf( bufferInOut, lengthInOut, offsetInOut, 2, "\\n" ); break;
					case '\r': Json_Printf( bufferInOut, lengthInOut, offsetInOut, 2, "\\r" ); break;
					case '\t': Json_Printf( bufferInOut, lengthInOut, offsetInOut, 2, "\\t" ); break;
					default: Json_Printf( bufferInOut, lengthInOut, offsetInOut, 6, "\\u%04x", ptr[0] ); break;
				}
			}
			else
			{
				Json_Printf( bufferInOut, lengthInOut, offsetInOut, 1, "%c", ptr[0] );
			}
		}
		Json_Printf( bufferInOut, lengthInOut, offsetInOut, 2, "%s\n", lastChild ? "" : "," );
	}
	else if ( node->type == JSON_OBJECT )
	{
		Json_Printf( bufferInOut, lengthInOut, offsetInOut, 2, "{\n" );
		for ( int i = 0; i < node->childCount; i++ )
		{
			const Json_t * child = &node->children[i];
			Json_Printf( bufferInOut, lengthInOut, offsetInOut, indent + 1 + (int)strlen( child->name ) + 5, "%s\"%s\" : ", &indentTable[maxIndent - ( indent + 1 )], child->name );
			Json_WriteValue( child, bufferInOut, lengthInOut, offsetInOut, indent + 1, ( i == node->childCount - 1 ) );
		}
		Json_Printf( bufferInOut, lengthInOut, offsetInOut, indent + 2, "%s}%s\n", &indentTable[maxIndent - indent], lastChild ? "" : "," );
	}
	else if ( node->type == JSON_ARRAY )
	{
		Json_Printf( bufferInOut, lengthInOut, offsetInOut, 2, "[\n" );
		for ( int i = 0; i < node->childCount; i++ )
		{
			const Json_t * child = &node->children[i];
			Json_Printf( bufferInOut, lengthInOut, offsetInOut, indent + 1, "%s", &indentTable[maxIndent - ( indent + 1 )] );
			Json_WriteValue( child, bufferInOut, lengthInOut, offsetInOut, indent + 1, ( i == node->childCount - 1 ) );
		}
		Json_Printf( bufferInOut, lengthInOut, offsetInOut, indent + 2, "%s]%s\n", &indentTable[maxIndent - indent], lastChild ? "" : "," );
	}
}

// 'lengthOut' is the length of 'bufferOut' without trailing zero.
static bool Json_WriteToBuffer( const Json_t * rootNode, char ** bufferOut, int * lengthOut )
{
	if ( rootNode == NULL || bufferOut == NULL || lengthOut == NULL )
	{
		return false;
	}
	*bufferOut = NULL;
	*lengthOut = 0;
	int offset = 0;
	Json_WriteValue( rootNode, bufferOut, lengthOut, &offset, 0, true );
	*lengthOut = offset;
	return true;
}

static bool Json_WriteToFile( const Json_t * rootNode, const char * fileName )
{
	if ( rootNode == NULL || fileName == NULL )
	{
		return false;
	}
	char * buffer = NULL;
	int length = 0;
	int offset = 0;
	Json_WriteValue( rootNode, &buffer, &length, &offset, 0, true );

	FILE * file = fopen( fileName, "wb" );
	if ( file == NULL )
	{
		free( buffer );
		return false;
	}
	if ( fwrite( buffer, 1, offset, file ) != (size_t) offset )
	{
		free( buffer );
		fclose( file );
		return false;
	}
	fclose( file );
	free( buffer );
	return true;
}

static int Json_GetChildCount( const Json_t * parentNode )
{
	return ( parentNode != NULL ) ? parentNode->childCount : 0;
}

static Json_t * Json_GetChildByIndex( const Json_t * parentNode, const int index )
{
	if ( parentNode != NULL )
	{
		if ( parentNode->type == JSON_OBJECT || parentNode->type == JSON_ARRAY )
		{
			if ( index >= 0 && index < parentNode->childCount )
			{
				return &parentNode->children[index];
			}
		}
	}
	return NULL;
}

static Json_t * Json_GetChildByName( const Json_t * parentNode, const char * childName )
{
	if ( parentNode != NULL && parentNode->type == JSON_OBJECT )
	{
		assert( childName != NULL );
		for ( int i = 0; i < parentNode->childCount; i++ )
		{
			Json_t * child = &parentNode->children[( parentNode->childIndex + i ) % parentNode->childCount];
			if ( strcmp( child->name, childName ) == 0 )
			{
				*(int *)&parentNode->childIndex = ( parentNode->childIndex + i + 1 ) % parentNode->childCount;	// mutable
				return child;
			}
			// Set a breakpoint here to find cases where the JSON is not parsed in order.
		}
	}
	return NULL;
}

static Json_t * Json_AddChild( Json_t * parentNode, const char * childName )
{
	if ( parentNode != NULL )
	{
		if ( parentNode->type == JSON_OBJECT )
		{
			assert( childName != NULL );
			Json_t * child = Json_AllocChild( parentNode );
			const size_t length = strlen( childName );
			child->name = (char *) malloc( length + 1 );
			strcpy( child->name, childName );
			child->name[length] = '\0';
			return child;
		}
		else if ( parentNode->type == JSON_ARRAY )
		{
			return Json_AllocChild( parentNode );
		}
	}
	return NULL;
}

static inline bool Json_IsNull( const Json_t * node )
{
	return ( node != NULL && node->type == JSON_NULL );
}

static inline bool Json_IsBoolean( const Json_t * node )
{
	return ( node != NULL && node->type == JSON_BOOLEAN );
}

static inline bool Json_IsNumber( const Json_t * node )
{
	return ( node != NULL && ( node->type == JSON_INTEGER || node->type == JSON_FLOAT ) );
}

static inline bool Json_IsInteger( const Json_t * node )
{
	return ( node != NULL && node->type == JSON_INTEGER );
}

static inline bool Json_IsFloatingPoint( const Json_t * node )
{
	return ( node != NULL && node->type == JSON_FLOAT );
}

static inline bool Json_IsString( const Json_t * node )
{
	return ( node != NULL && node->type == JSON_STRING );
}

static inline bool Json_IsObject( const Json_t * node )
{
	return ( node != NULL && node->type == JSON_OBJECT );
}

static inline bool Json_IsArray( const Json_t * node )
{
	return ( node != NULL && node->type == JSON_ARRAY );
}

static inline bool Json_GetBool( const Json_t * node, const bool defaultValue )
{
	return Json_IsBoolean( node ) ? strcmp( node->valueString, "false" ) != 0 : defaultValue;
}

static inline int Json_GetInt32( const Json_t * node, const int defaultValue )
{
	return Json_IsInteger( node ) ? (int)node->valueInt64 : ( Json_IsFloatingPoint( node ) ? (int)node->valueDouble : defaultValue );
}

static inline long long Json_GetInt64( const Json_t * node, const long long defaultValue )
{
	return Json_IsInteger( node ) ? node->valueInt64 : ( Json_IsFloatingPoint( node ) ? (long long)node->valueDouble : defaultValue );
}

static inline float Json_GetFloat( const Json_t * node, const float defaultValue )
{
	return Json_IsInteger( node ) ? (float)node->valueInt64 : ( Json_IsFloatingPoint( node ) ? (float)node->valueDouble : defaultValue );
}

static inline double Json_GetDouble( const Json_t * node, const double defaultValue )
{
	return Json_IsInteger( node ) ? (double)node->valueInt64 : ( Json_IsFloatingPoint( node ) ? node->valueDouble : defaultValue );
}

static inline const char * Json_GetString( const Json_t * node, const char * defaultValue )
{
	return Json_IsString( node ) ? node->valueString : defaultValue;
}

static inline Json_t * Json_SetBoolean( Json_t * node, const bool value )
{
	if ( node != NULL )
	{
		Json_FreeNode( node, false );
		node->type = JSON_BOOLEAN;
		node->valueString = value ? (char *)"true" : (char *)"false";
	}
	return node;
}

static inline Json_t * Json_SetInt32( Json_t * node, const int value )
{
	if ( node != NULL )
	{
		Json_FreeNode( node, false );
		node->type = JSON_INTEGER;
		node->valueInt64 = value;
	}
	return node;
}

static inline Json_t * Json_SetInt64( Json_t * node, const long long value )
{
	if ( node != NULL )
	{
		Json_FreeNode( node, false );
		node->type = JSON_INTEGER;
		node->valueInt64 = value;
	}
	return node;
}

static inline Json_t * Json_SetFloat( Json_t * node, const float value )
{
	if ( node != NULL )
	{
		Json_FreeNode( node, false );
		node->type = JSON_FLOAT;
		node->valueDouble = value;
	}
	return node;
}

static inline Json_t * Json_SetDouble( Json_t * node, const double value )
{
	if ( node != NULL )
	{
		Json_FreeNode( node, false );
		node->type = JSON_FLOAT;
		node->valueDouble = value;
	}
	return node;
}

static inline Json_t * Json_SetString( Json_t * node, const char * value )
{
	if ( node != NULL )
	{
		assert( value != NULL );
		Json_FreeNode( node, false );
		node->type = JSON_STRING;
		const int length = (int)strlen( value );
		node->valueString = (char *) malloc( length + 1 );
		strcpy( node->valueString, value );
	}
	return node;
}

static inline Json_t * Json_SetObject( Json_t * node )
{
	if ( node != NULL )
	{
		Json_FreeNode( node, false );
		node->type = JSON_OBJECT;
	}
	return node;
}

static inline Json_t * Json_SetArray( Json_t * node )
{
	if ( node != NULL )
	{
		Json_FreeNode( node, false );
		node->type = JSON_ARRAY;
	}
	return node;
}

#endif // !JSON_H

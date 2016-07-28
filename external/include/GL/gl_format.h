/*
================================================================================================

Description	:	OpenGL formats/types and properties.
Author		:	J.M.P. van Waveren
Date		:	07/17/2016
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


DESCRIPTION
===========

This header stores the OpenGL formats/types and two simple routines
to derive the format/type from an internal format. These routines
are useful to verify the data in a KTX container filer.


IMPLEMENTATION
==============

This file does not include OpenGL / OpenGL ES headers because:

  1. Including OpenGL / OpenGL ES headers is a platform dependent mess.
  2. The OpenGL format/type constants are the same between extensions and core.
  3. The OpenGL format/type constants are the same between OpenGL and OpenGL ES.
  4. The OpenGL constants in this header are also used to derive Vulkan formats
     from the OpenGL formats/types stored in files like KTX and glTF. These file
	 formats may use OpenGL formats/types that are not supported by the OpenGL
	 implementation on the platform but are supported by the Vulkan implementation.


ENTRY POINTS
============

static inline GLenum glGetFormatFromInternalFormat( const GLenum internalFormat );
static inline GLenum glGetTypeFromInternalFormat( const GLenum internalFormat );
static inline void glGetFormatSize( const GLenum internalFormat, GlFormatSize * pFormatSize );

================================================================================================
*/

#include <assert.h>

#if defined(_WIN32)
#define NOMINMAX
#ifndef __cplusplus
#undef inline
#define inline __inline
#endif // __cplusplus
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLuint;

/*
================================================================================================================================

Format to glTexImage2D and glTexImage3D.

================================================================================================================================
*/

#if !defined( GL_RED )
#define GL_RED											0x1903	// same as GL_RED_EXT
#endif
#if !defined( GL_GREEN )
#define GL_GREEN										0x1904	// deprecated
#endif
#if !defined( GL_BLUE )
#define GL_BLUE											0x1905	// deprecated
#endif
#if !defined( GL_ALPHA )
#define GL_ALPHA										0x1906	// deprecated
#endif
#if !defined( GL_LUMINANCE )
#define GL_LUMINANCE									0x1909	// deprecated
#endif
#if !defined( GL_LUMINANCE_ALPHA )
#define GL_LUMINANCE_ALPHA								0x190A	// deprecated
#endif
#if !defined( GL_RG )
#define GL_RG											0x8227	// same as GL_RG_EXT  
#endif
#if !defined( GL_RGB )
#define GL_RGB											0x1907
#endif
#if !defined( GL_BGR )
#define GL_BGR											0x80E0	// same as GL_BGR_EXT
#endif
#if !defined( GL_RGBA )
#define GL_RGBA											0x1908
#endif
#if !defined( GL_BGRA )
#define GL_BGRA											0x80E1	// same as GL_BGRA_EXT
#endif
#if !defined( GL_RED_INTEGER )
#define GL_RED_INTEGER									0x8D94	// same as GL_RED_INTEGER_EXT
#endif
#if !defined( GL_GREEN_INTEGER )
#define GL_GREEN_INTEGER								0x8D95	// deprecated, same as GL_GREEN_INTEGER_EXT
#endif
#if !defined( GL_BLUE_INTEGER )
#define GL_BLUE_INTEGER									0x8D96	// deprecated, same as GL_BLUE_INTEGER_EXT
#endif
#if !defined( GL_ALPHA_INTEGER )
#define GL_ALPHA_INTEGER								0x8D97	// deprecated, same as GL_ALPHA_INTEGER_EXT
#endif
#if !defined( GL_LUMINANCE_INTEGER )
#define GL_LUMINANCE_INTEGER							0x8D9C	// deprecated, same as GL_LUMINANCE_INTEGER_EXT
#endif
#if !defined( GL_LUMINANCE_ALPHA_INTEGER )
#define GL_LUMINANCE_ALPHA_INTEGER						0x8D9D	// deprecated, same as GL_LUMINANCE_ALPHA_INTEGER_EXT
#endif
#if !defined( GL_RG_INTEGER )
#define GL_RG_INTEGER									0x8228	// same as GL_RG_INTEGER_EXT
#endif
#if !defined( GL_RGB_INTEGER )
#define GL_RGB_INTEGER									0x8D98	// same as GL_RGB_INTEGER_EXT
#endif
#if !defined( GL_BGR_INTEGER )
#define GL_BGR_INTEGER									0x8D9A	// same as GL_BGR_INTEGER_EXT
#endif
#if !defined( GL_RGBA_INTEGER )
#define GL_RGBA_INTEGER									0x8D99	// same as GL_RGBA_INTEGER_EXT
#endif
#if !defined( GL_BGRA_INTEGER )
#define GL_BGRA_INTEGER									0x8D9B	// same as GL_BGRA_INTEGER_EXT
#endif
#if !defined( GL_STENCIL_INDEX )
#define GL_STENCIL_INDEX								0x1901
#endif
#if !defined( GL_DEPTH_COMPONENT )
#define GL_DEPTH_COMPONENT								0x1902
#endif
#if !defined( GL_DEPTH_STENCIL )
#define GL_DEPTH_STENCIL								0x84F9	// same as GL_DEPTH_STENCIL_EXT and GL_DEPTH_STENCIL_NV
#endif

/*
================================================================================================================================

Type to glTexImage2D, glTexImage3D and glVertexAttribPointer.

================================================================================================================================
*/

#if !defined( GL_BYTE )
#define GL_BYTE											0x1400
#endif
#if !defined( GL_UNSIGNED_BYTE )
#define GL_UNSIGNED_BYTE								0x1401
#endif
#if !defined( GL_SHORT )
#define GL_SHORT										0x1402
#endif
#if !defined( GL_UNSIGNED_SHORT )
#define GL_UNSIGNED_SHORT								0x1403
#endif
#if !defined( GL_INT )
#define GL_INT											0x1404
#endif
#if !defined( GL_UNSIGNED_INT )
#define GL_UNSIGNED_INT									0x1405
#endif
#if !defined( GL_HALF_FLOAT )
#define GL_HALF_FLOAT									0x140B	// same as GL_HALF_FLOAT_NV and GL_HALF_FLOAT_ARB
#endif
#if !defined( GL_HALF_FLOAT_OES )
#define GL_HALF_FLOAT_OES								0x8D61	// FIXME: why is this different?
#endif
#if !defined( GL_FLOAT )
#define GL_FLOAT										0x1406
#endif
#if !defined( GL_DOUBLE )
#define GL_DOUBLE										0x140A	// same as GL_DOUBLE_EXT
#endif
#if !defined( GL_UNSIGNED_BYTE_3_3_2 )
#define GL_UNSIGNED_BYTE_3_3_2							0x8032	// same as GL_UNSIGNED_BYTE_3_3_2_EXT
#endif
#if !defined( GL_UNSIGNED_BYTE_2_3_3_REV )
#define GL_UNSIGNED_BYTE_2_3_3_REV						0x8362	// same as GL_UNSIGNED_BYTE_2_3_3_REV_EXT
#endif
#if !defined( GL_UNSIGNED_SHORT_5_6_5 )
#define GL_UNSIGNED_SHORT_5_6_5							0x8363	// same as GL_UNSIGNED_SHORT_5_6_5_EXT
#endif
#if !defined( GL_UNSIGNED_SHORT_5_6_5_REV )
#define GL_UNSIGNED_SHORT_5_6_5_REV						0x8364	// same as GL_UNSIGNED_SHORT_5_6_5_REV_EXT
#endif
#if !defined( GL_UNSIGNED_SHORT_4_4_4_4 )
#define GL_UNSIGNED_SHORT_4_4_4_4						0x8033	// same as GL_UNSIGNED_SHORT_4_4_4_4_EXT
#endif
#if !defined( GL_UNSIGNED_SHORT_4_4_4_4_REV )
#define GL_UNSIGNED_SHORT_4_4_4_4_REV					0x8365	// same as  GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT and GL_UNSIGNED_SHORT_4_4_4_4_REV_IMG
#endif
#if !defined( GL_UNSIGNED_SHORT_5_5_5_1 )
#define GL_UNSIGNED_SHORT_5_5_5_1						0x8034	// same as GL_UNSIGNED_SHORT_5_5_5_1_EXT
#endif
#if !defined( GL_UNSIGNED_SHORT_1_5_5_5_REV )
#define GL_UNSIGNED_SHORT_1_5_5_5_REV					0x8366	// same as GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT
#endif
#if !defined( GL_UNSIGNED_INT_8_8_8_8 )
#define GL_UNSIGNED_INT_8_8_8_8							0x8035	// same as GL_UNSIGNED_INT_8_8_8_8_EXT
#endif
#if !defined( GL_UNSIGNED_INT_8_8_8_8_REV )
#define GL_UNSIGNED_INT_8_8_8_8_REV						0x8367	// same as GL_UNSIGNED_INT_8_8_8_8_REV_EXT
#endif
#if !defined( GL_UNSIGNED_INT_10_10_10_2 )
#define GL_UNSIGNED_INT_10_10_10_2						0x8036	// same as GL_UNSIGNED_INT_10_10_10_2_EXT
#endif
#if !defined( GL_UNSIGNED_INT_2_10_10_10_REV )
#define GL_UNSIGNED_INT_2_10_10_10_REV					0x8368	// same as GL_UNSIGNED_INT_2_10_10_10_REV_EXT
#endif
#if !defined( GL_UNSIGNED_INT_10F_11F_11F_REV )
#define GL_UNSIGNED_INT_10F_11F_11F_REV					0x8C3B	// same as GL_UNSIGNED_INT_10F_11F_11F_REV_EXT
#endif
#if !defined( GL_UNSIGNED_INT_5_9_9_9_REV )
#define GL_UNSIGNED_INT_5_9_9_9_REV						0x8C3E	// same as GL_UNSIGNED_INT_5_9_9_9_REV_EXT
#endif
#if !defined( GL_UNSIGNED_INT_24_8 )
#define GL_UNSIGNED_INT_24_8							0x84FA	// same as GL_UNSIGNED_INT_24_8_EXT and GL_UNSIGNED_INT_24_8_NV
#endif
#if !defined( GL_FLOAT_32_UNSIGNED_INT_24_8_REV )
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV				0x8DAD	// same as GL_FLOAT_32_UNSIGNED_INT_24_8_REV_NV
#endif

/*
================================================================================================================================

Internal format to glTexImage2D, glTexImage3D, glCompressedTexImage2D, glCompressedTexImage3D, glTexStorage2D, glTexStorage3D

================================================================================================================================
*/

//
// 8 bits per component
//

#if !defined( GL_R8 )
#define GL_R8											0x8229	// same as GL_R8_EXT
#endif
#if !defined( GL_RG8 )
#define GL_RG8											0x822B	// same as GL_RG8_EXT
#endif
#if !defined( GL_RGB8 )
#define GL_RGB8											0x8051	// same as GL_RGB8_EXT and GL_RGB8_OES
#endif
#if !defined( GL_RGBA8 )
#define GL_RGBA8										0x8058	// same as GL_RGBA8_EXT and GL_RGBA8_OES
#endif

#if !defined( GL_R8_SNORM )
#define GL_R8_SNORM										0x8F94
#endif
#if !defined( GL_RG8_SNORM )
#define GL_RG8_SNORM									0x8F95
#endif
#if !defined( GL_RGB8_SNORM )
#define GL_RGB8_SNORM									0x8F96
#endif
#if !defined( GL_RGBA8_SNORM )
#define GL_RGBA8_SNORM									0x8F97
#endif

#if !defined( GL_R8UI )
#define GL_R8UI											0x8232
#endif
#if !defined( GL_RG8UI )
#define GL_RG8UI										0x8238
#endif
#if !defined( GL_RGB8UI )
#define GL_RGB8UI										0x8D7D	// same as GL_RGB8UI_EXT
#endif
#if !defined( GL_RGBA8UI )
#define GL_RGBA8UI										0x8D7C	// same as GL_RGBA8UI_EXT
#endif

#if !defined( GL_R8I )
#define GL_R8I											0x8231
#endif
#if !defined( GL_RG8I )
#define GL_RG8I											0x8237
#endif
#if !defined( GL_RGB8I )
#define GL_RGB8I										0x8D8F	// same as GL_RGB8I_EXT
#endif
#if !defined( GL_RGBA8I )
#define GL_RGBA8I										0x8D8E	// same as GL_RGBA8I_EXT
#endif

#if !defined( GL_SR8 )
#define GL_SR8											0x8FBD	// same as GL_SR8_EXT
#endif
#if !defined( GL_SRG8 )
#define GL_SRG8											0x8FBE	// same as GL_SRG8_EXT
#endif
#if !defined( GL_SRGB8 )
#define GL_SRGB8										0x8C41	// same as GL_SRGB8_EXT
#endif
#if !defined( GL_SRGB8_ALPHA8 )
#define GL_SRGB8_ALPHA8									0x8C43	// same as GL_SRGB8_ALPHA8_EXT
#endif

//
// 16 bits per component
//

#if !defined( GL_R16 )
#define GL_R16											0x822A	// same as GL_R16_EXT
#endif
#if !defined( GL_RG16 )
#define GL_RG16											0x822C	// same as GL_RG16_EXT
#endif
#if !defined( GL_RGB16 )
#define GL_RGB16										0x8054	// same as GL_RGB16_EXT
#endif
#if !defined( GL_RGBA16 )
#define GL_RGBA16										0x805B	// same as GL_RGBA16_EXT
#endif

#if !defined( GL_R16_SNORM )
#define GL_R16_SNORM									0x8F98	// same as GL_R16_SNORM_EXT
#endif
#if !defined( GL_RG16_SNORM )
#define GL_RG16_SNORM									0x8F99	// same as GL_RG16_SNORM_EXT
#endif
#if !defined( GL_RGB16_SNORM )
#define GL_RGB16_SNORM									0x8F9A	// same as GL_RGB16_SNORM_EXT
#endif
#if !defined( GL_RGBA16_SNORM )
#define GL_RGBA16_SNORM									0x8F9B	// same as GL_RGBA16_SNORM_EXT
#endif

#if !defined( GL_R16UI )
#define GL_R16UI										0x8234
#endif
#if !defined( GL_RG16UI )
#define GL_RG16UI										0x823A
#endif
#if !defined( GL_RGB16UI )
#define GL_RGB16UI										0x8D77	// same as GL_RGB16UI_EXT
#endif
#if !defined( GL_RGBA16UI )
#define GL_RGBA16UI										0x8D76	// same as GL_RGBA16UI_EXT
#endif

#if !defined( GL_R16I )
#define GL_R16I											0x8233
#endif
#if !defined( GL_RG16I )
#define GL_RG16I										0x8239
#endif
#if !defined( GL_RGB16I )
#define GL_RGB16I										0x8D89	// same as GL_RGB16I_EXT
#endif
#if !defined( GL_RGBA16I )
#define GL_RGBA16I										0x8D88	// same as GL_RGBA16I_EXT
#endif

#if !defined( GL_R16F )
#define GL_R16F											0x822D	// same as GL_R16F_EXT
#endif
#if !defined( GL_RG16F )
#define GL_RG16F										0x822F	// same as GL_RG16F_EXT
#endif
#if !defined( GL_RGB16F )
#define GL_RGB16F										0x881B	// same as GL_RGB16F_EXT and GL_RGB16F_ARB
#endif
#if !defined( GL_RGBA16F )
#define GL_RGBA16F										0x881A	// sama as GL_RGBA16F_EXT and GL_RGBA16F_ARB
#endif

//
// 32 bits per component
//

#if !defined( GL_R32UI )
#define GL_R32UI										0x8236
#endif
#if !defined( GL_RG32UI )
#define GL_RG32UI										0x823C
#endif
#if !defined( GL_RGB32UI )
#define GL_RGB32UI										0x8D71	// same as GL_RGB32UI_EXT
#endif
#if !defined( GL_RGBA32UI )
#define GL_RGBA32UI										0x8D70	// same as GL_RGBA32UI_EXT
#endif

#if !defined( GL_R32I )
#define GL_R32I											0x8235
#endif
#if !defined( GL_RG32I )
#define GL_RG32I										0x823B
#endif
#if !defined( GL_RGB32I )
#define GL_RGB32I										0x8D83	// same as GL_RGB32I_EXT 
#endif
#if !defined( GL_RGBA32I )
#define GL_RGBA32I										0x8D82	// same as GL_RGBA32I_EXT
#endif

#if !defined( GL_R32F )
#define GL_R32F											0x822E	// same as GL_R32F_EXT
#endif
#if !defined( GL_RG32F )
#define GL_RG32F										0x8230	// same as GL_RG32F_EXT
#endif
#if !defined( GL_RGB32F )
#define GL_RGB32F										0x8815	// same as GL_RGB32F_EXT and GL_RGB32F_ARB
#endif
#if !defined( GL_RGBA32F )
#define GL_RGBA32F										0x8814	// same as GL_RGBA32F_EXT and GL_RGBA32F_ARB
#endif

//
// Odd bits per component
//

#if !defined( GL_R3_G3_B2 )
#define GL_R3_G3_B2										0x2A10
#endif
#if !defined( GL_RGB4 )
#define GL_RGB4											0x804F	// same as GL_RGB4_EXT
#endif
#if !defined( GL_RGB5 )
#define GL_RGB5											0x8050	// same as GL_RGB5_EXT
#endif
#if !defined( GL_RGB565 )
#define GL_RGB565										0x8D62	// same as GL_RGB565_EXT and GL_RGB565_OES
#endif
#if !defined( GL_RGB10 )
#define GL_RGB10										0x8052	// same as GL_RGB10_EXT
#endif
#if !defined( GL_RGB12 )
#define GL_RGB12										0x8053	// same as GL_RGB12_EXT
#endif
#if !defined( GL_RGBA2 )
#define GL_RGBA2										0x8055	// same as GL_RGBA2_EXT
#endif
#if !defined( GL_RGBA4 )
#define GL_RGBA4										0x8056	// same as GL_RGBA4_EXT and GL_RGBA4_OES
#endif
#if !defined( GL_RGBA12 )
#define GL_RGBA12										0x805A	// same as GL_RGBA12_EXT
#endif
#if !defined( GL_RGB5_A1 )
#define GL_RGB5_A1										0x8057	// same as GL_RGB5_A1_EXT and GL_RGB5_A1_OES
#endif
#if !defined( GL_RGB10_A2 )
#define GL_RGB10_A2										0x8059	// same as GL_RGB10_A2_EXT
#endif
#if !defined( GL_RGB10_A2UI )
#define GL_RGB10_A2UI									0x906F
#endif
#if !defined( GL_R11F_G11F_B10F )
#define GL_R11F_G11F_B10F								0x8C3A	// same as GL_R11F_G11F_B10F_EXT and GL_R11F_G11F_B10F_APPLE
#endif
#if !defined( GL_RGB9_E5 )
#define GL_RGB9_E5										0x8C3D	// same as GL_RGB9_E5_EXT and GL_RGB9_E5_APPLE
#endif

//
// S3TC/DXT/BC
//

#if !defined( GL_COMPRESSED_RGB_S3TC_DXT1_EXT )
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT					0x83F0
#endif
#if !defined( GL_COMPRESSED_RGBA_S3TC_DXT1_EXT )
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT				0x83F1
#endif
#if !defined( GL_COMPRESSED_RGBA_S3TC_DXT3_EXT )
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT				0x83F2
#endif
#if !defined( GL_COMPRESSED_RGBA_S3TC_DXT5_EXT )
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT				0x83F3
#endif

#if !defined( GL_COMPRESSED_SRGB_S3TC_DXT1_EXT )
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT				0x8C4C
#endif
#if !defined( GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT )
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT			0x8C4D
#endif
#if !defined( GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT )
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT			0x8C4E
#endif
#if !defined( GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT )
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT			0x8C4F
#endif

#if !defined( GL_COMPRESSED_LUMINANCE_LATC1_EXT )
#define GL_COMPRESSED_LUMINANCE_LATC1_EXT				0x8C70
#endif
#if !defined( GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT )
#define GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT			0x8C72
#endif
#if !defined( GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT )
#define GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT		0x8C71
#endif
#if !defined( GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT )
#define GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT	0x8C73
#endif

#if !defined( GL_COMPRESSED_RED_RGTC1 )
#define GL_COMPRESSED_RED_RGTC1							0x8DBB	// same as GL_COMPRESSED_RED_RGTC1_EXT
#endif
#if !defined( GL_COMPRESSED_RG_RGTC2 )
#define GL_COMPRESSED_RG_RGTC2							0x8DBD	// same as GL_COMPRESSED_RG_RGTC2_EXT
#endif
#if !defined( GL_COMPRESSED_SIGNED_RED_RGTC1 )
#define GL_COMPRESSED_SIGNED_RED_RGTC1					0x8DBC	// same as GL_COMPRESSED_SIGNED_RED_RGTC1_EXT
#endif
#if !defined( GL_COMPRESSED_SIGNED_RG_RGTC2 )
#define GL_COMPRESSED_SIGNED_RG_RGTC2					0x8DBE	// same as GL_COMPRESSED_SIGNED_RG_RGTC2_EXT
#endif

#if !defined( GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT )
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT				0x8E8E	// same as GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB
#endif
#if !defined( GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT )
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT			0x8E8F	// same as GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB
#endif
#if !defined( GL_COMPRESSED_RGBA_BPTC_UNORM )
#define GL_COMPRESSED_RGBA_BPTC_UNORM					0x8E8C	// same as GL_COMPRESSED_RGBA_BPTC_UNORM_ARB	
#endif
#if !defined( GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM )
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM				0x8E8D	// same as GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB
#endif

//
// ETC
//

#if !defined( GL_ETC1_RGB8_OES )
#define GL_ETC1_RGB8_OES								0x8D64
#endif

#if !defined( GL_COMPRESSED_RGB8_ETC2 )
#define GL_COMPRESSED_RGB8_ETC2							0x9274
#endif
#if !defined( GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 )
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2		0x9276
#endif
#if !defined( GL_COMPRESSED_RGBA8_ETC2_EAC )
#define GL_COMPRESSED_RGBA8_ETC2_EAC					0x9278
#endif

#if !defined( GL_COMPRESSED_SRGB8_ETC2 )
#define GL_COMPRESSED_SRGB8_ETC2						0x9275
#endif
#if !defined( GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 )
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2	0x9277
#endif
#if !defined( GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC )
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC				0x9279
#endif

#if !defined( GL_COMPRESSED_R11_EAC )
#define GL_COMPRESSED_R11_EAC							0x9270
#endif
#if !defined( GL_COMPRESSED_RG11_EAC )
#define GL_COMPRESSED_RG11_EAC							0x9272
#endif
#if !defined( GL_COMPRESSED_SIGNED_R11_EAC )
#define GL_COMPRESSED_SIGNED_R11_EAC					0x9271
#endif
#if !defined( GL_COMPRESSED_SIGNED_RG11_EAC )
#define GL_COMPRESSED_SIGNED_RG11_EAC					0x9273
#endif

//
// ASTC
//

#if !defined( GL_COMPRESSED_RGBA_ASTC_4x4_KHR )
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR					0x93B0
#define GL_COMPRESSED_RGBA_ASTC_5x4_KHR					0x93B1
#define GL_COMPRESSED_RGBA_ASTC_5x5_KHR					0x93B2
#define GL_COMPRESSED_RGBA_ASTC_6x5_KHR					0x93B3
#define GL_COMPRESSED_RGBA_ASTC_6x6_KHR					0x93B4
#define GL_COMPRESSED_RGBA_ASTC_8x5_KHR					0x93B5
#define GL_COMPRESSED_RGBA_ASTC_8x6_KHR					0x93B6
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR					0x93B7
#define GL_COMPRESSED_RGBA_ASTC_10x5_KHR				0x93B8
#define GL_COMPRESSED_RGBA_ASTC_10x6_KHR				0x93B9
#define GL_COMPRESSED_RGBA_ASTC_10x8_KHR				0x93BA
#define GL_COMPRESSED_RGBA_ASTC_10x10_KHR				0x93BB
#define GL_COMPRESSED_RGBA_ASTC_12x10_KHR				0x93BC
#define GL_COMPRESSED_RGBA_ASTC_12x12_KHR				0x93BD
#endif

#if !defined( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR )
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR			0x93D0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR			0x93D1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR			0x93D2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR			0x93D3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR			0x93D4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR			0x93D5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR			0x93D6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR			0x93D7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR		0x93D8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR		0x93D9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR		0x93DA
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR		0x93DB
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR		0x93DC
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR		0x93DD
#endif

#if !defined( GL_COMPRESSED_RGBA_ASTC_3x3x3_OES )
#define GL_COMPRESSED_RGBA_ASTC_3x3x3_OES				0x93C0
#define GL_COMPRESSED_RGBA_ASTC_4x3x3_OES				0x93C1
#define GL_COMPRESSED_RGBA_ASTC_4x4x3_OES				0x93C2
#define GL_COMPRESSED_RGBA_ASTC_4x4x4_OES				0x93C3
#define GL_COMPRESSED_RGBA_ASTC_5x4x4_OES				0x93C4
#define GL_COMPRESSED_RGBA_ASTC_5x5x4_OES				0x93C5
#define GL_COMPRESSED_RGBA_ASTC_5x5x5_OES				0x93C6
#define GL_COMPRESSED_RGBA_ASTC_6x5x5_OES				0x93C7
#define GL_COMPRESSED_RGBA_ASTC_6x6x5_OES				0x93C8
#define GL_COMPRESSED_RGBA_ASTC_6x6x6_OES				0x93C9
#endif

#if !defined( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES )
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES		0x93E0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES		0x93E1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES		0x93E2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES		0x93E3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES		0x93E4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES		0x93E5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES		0x93E6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES		0x93E7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES		0x93E8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES		0x93E9
#endif

//
// ATC
//

#if !defined( GL_ATC_RGB_AMD )
#define GL_ATC_RGB_AMD									0x8C92
#define GL_ATC_RGBA_EXPLICIT_ALPHA_AMD					0x8C93
#define GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD				0x87EE
#endif

//
// Palletized
//

#if !defined( GL_PALETTE4_RGB8_OES )
#define GL_PALETTE4_RGB8_OES							0x8B90
#define GL_PALETTE4_RGBA8_OES							0x8B91
#define GL_PALETTE4_R5_G6_B5_OES						0x8B92
#define GL_PALETTE4_RGBA4_OES							0x8B93
#define GL_PALETTE4_RGB5_A1_OES							0x8B94
#define GL_PALETTE8_RGB8_OES							0x8B95
#define GL_PALETTE8_RGBA8_OES							0x8B96
#define GL_PALETTE8_R5_G6_B5_OES						0x8B97
#define GL_PALETTE8_RGBA4_OES							0x8B98
#define GL_PALETTE8_RGB5_A1_OES							0x8B99
#endif


//
// Luminance/alpha/intensity
//

#if !defined( GL_ALPHA4 )
#define GL_ALPHA4										0x803B	// deprecated, same as GL_ALPHA4_EXT
#define GL_ALPHA8										0x803C	// deprecated, same as GL_ALPHA8_EXT
#define GL_ALPHA12										0x803D	// deprecated, same as GL_ALPHA12_EXT
#define GL_ALPHA16										0x803E	// deprecated, same as GL_ALPHA16_EXT
#define GL_LUMINANCE4									0x803F	// deprecated, same as GL_LUMINANCE4_EXT
#define GL_LUMINANCE8									0x8040	// deprecated, same as GL_LUMINANCE8_EXT
#define GL_LUMINANCE12									0x8041	// deprecated, same as GL_LUMINANCE12_EXT
#define GL_LUMINANCE16									0x8042	// deprecated, same as GL_LUMINANCE16_EXT
#define GL_SLUMINANCE8									0x8C47	// deprecated, same as GL_SLUMINANCE8_EXT
#define GL_LUMINANCE4_ALPHA4							0x8043	// deprecated, same as GL_LUMINANCE4_ALPHA4_EXT
#define GL_LUMINANCE6_ALPHA2							0x8044	// deprecated, same as GL_LUMINANCE6_ALPHA2_EXT
#define GL_LUMINANCE8_ALPHA8							0x8045	// deprecated, same as GL_LUMINANCE8_ALPHA8_EXT
#define GL_LUMINANCE12_ALPHA4							0x8046	// deprecated, same as GL_LUMINANCE12_ALPHA4_EXT
#define GL_LUMINANCE12_ALPHA12							0x8047	// deprecated, same as GL_LUMINANCE12_ALPHA12_EXT
#define GL_LUMINANCE16_ALPHA16							0x8048	// deprecated, same as GL_LUMINANCE16_ALPHA16_EXT
#define GL_SLUMINANCE8_ALPHA8							0x8C45	// deprecated, same as GL_SLUMINANCE8_ALPHA8_EXT
#define GL_INTENSITY									0x8049	// deprecated, same as GL_INTENSITY_EXT
#define GL_INTENSITY4									0x804A	// deprecated, same as GL_INTENSITY4_EXT
#define GL_INTENSITY8									0x804B	// deprecated, same as GL_INTENSITY8_EXT
#define GL_INTENSITY12									0x804C	// deprecated, same as GL_INTENSITY12_EXT
#define GL_INTENSITY16									0x804D	// deprecated, same as GL_INTENSITY16_EXT
#endif


static inline GLenum glGetFormatFromInternalFormat( const GLenum internalFormat )
{
	switch ( internalFormat )
	{
		//
		// 8 bits per component
		//
		case GL_R8:												return GL_RED;		// 1-component, 8-bit unsigned normalized
		case GL_RG8:											return GL_RG;		// 2-component, 8-bit unsigned normalized
		case GL_RGB8:											return GL_RGB;		// 3-component, 8-bit unsigned normalized
		case GL_RGBA8:											return GL_RGBA;		// 4-component, 8-bit unsigned normalized

		case GL_R8_SNORM:										return GL_RED;		// 1-component, 8-bit signed normalized
		case GL_RG8_SNORM:										return GL_RG;		// 2-component, 8-bit signed normalized
		case GL_RGB8_SNORM:										return GL_RGB;		// 3-component, 8-bit signed normalized
		case GL_RGBA8_SNORM:									return GL_RGBA;		// 4-component, 8-bit signed normalized

		case GL_R8UI:											return GL_RED;		// 1-component, 8-bit unsigned integer
		case GL_RG8UI:											return GL_RG;		// 2-component, 8-bit unsigned integer
		case GL_RGB8UI:											return GL_RGB;		// 3-component, 8-bit unsigned integer
		case GL_RGBA8UI:										return GL_RGBA;		// 4-component, 8-bit unsigned integer

		case GL_R8I:											return GL_RED;		// 1-component, 8-bit signed integer
		case GL_RG8I:											return GL_RG;		// 2-component, 8-bit signed integer
		case GL_RGB8I:											return GL_RGB;		// 3-component, 8-bit signed integer
		case GL_RGBA8I:											return GL_RGBA;		// 4-component, 8-bit signed integer

		case GL_SR8:											return GL_RED;		// 1-component, 8-bit sRGB
		case GL_SRG8:											return GL_RG;		// 2-component, 8-bit sRGB
		case GL_SRGB8:											return GL_RGB;		// 3-component, 8-bit sRGB
		case GL_SRGB8_ALPHA8:									return GL_RGBA;		// 4-component, 8-bit sRGB

		//
		// 16 bits per component
		//
		case GL_R16:											return GL_RED;		// 1-component, 16-bit unsigned normalized
		case GL_RG16:											return GL_RG;		// 2-component, 16-bit unsigned normalized
		case GL_RGB16:											return GL_RGB;		// 3-component, 16-bit unsigned normalized
		case GL_RGBA16:											return GL_RGBA;		// 4-component, 16-bit unsigned normalized

		case GL_R16_SNORM:										return GL_RED;		// 1-component, 16-bit signed normalized
		case GL_RG16_SNORM:										return GL_RG;		// 2-component, 16-bit signed normalized
		case GL_RGB16_SNORM:									return GL_RGB;		// 3-component, 16-bit signed normalized
		case GL_RGBA16_SNORM:									return GL_RGBA;		// 4-component, 16-bit signed normalized

		case GL_R16UI:											return GL_RED;		// 1-component, 16-bit unsigned integer
		case GL_RG16UI:											return GL_RG;		// 2-component, 16-bit unsigned integer
		case GL_RGB16UI:										return GL_RGB;		// 3-component, 16-bit unsigned integer
		case GL_RGBA16UI:										return GL_RGBA;		// 4-component, 16-bit unsigned integer

		case GL_R16I:											return GL_RED;		// 1-component, 16-bit signed integer
		case GL_RG16I:											return GL_RG;		// 2-component, 16-bit signed integer
		case GL_RGB16I:											return GL_RGB;		// 3-component, 16-bit signed integer
		case GL_RGBA16I:										return GL_RGBA;		// 4-component, 16-bit signed integer

		case GL_R16F:											return GL_RED;		// 1-component, 16-bit floating-point
		case GL_RG16F:											return GL_RG;		// 2-component, 16-bit floating-point
		case GL_RGB16F:											return GL_RGB;		// 3-component, 16-bit floating-point
		case GL_RGBA16F:										return GL_RGBA;		// 4-component, 16-bit floating-point

		//
		// 32 bits per component
		//
		case GL_R32UI:											return GL_RED;		// 1-component, 32-bit unsigned integer
		case GL_RG32UI:											return GL_RG;		// 2-component, 32-bit unsigned integer
		case GL_RGB32UI:										return GL_RGB;		// 3-component, 32-bit unsigned integer
		case GL_RGBA32UI:										return GL_RGBA;		// 4-component, 32-bit unsigned integer

		case GL_R32I:											return GL_RED;		// 1-component, 32-bit signed integer
		case GL_RG32I:											return GL_RG;		// 2-component, 32-bit signed integer
		case GL_RGB32I:											return GL_RGB;		// 3-component, 32-bit signed integer
		case GL_RGBA32I:										return GL_RGBA;		// 4-component, 32-bit signed integer

		case GL_R32F:											return GL_RED;		// 1-component, 32-bit floating-point
		case GL_RG32F:											return GL_RG;		// 2-component, 32-bit floating-point
		case GL_RGB32F:											return GL_RGB;		// 3-component, 32-bit floating-point
		case GL_RGBA32F:										return GL_RGBA;		// 4-component, 32-bit floating-point

		//
		// Odd bits per component
		//
		case GL_R3_G3_B2:										return GL_RGB;		// 3-component 3:3:2,       unsigned normalized
		case GL_RGB4:											return GL_RGB;		// 3-component 4:4:4,       unsigned normalized
		case GL_RGB5:											return GL_RGB;		// 3-component 5:5:5,       unsigned normalized
		case GL_RGB565:											return GL_RGB;		// 3-component 5:6:5,       unsigned normalized
		case GL_RGB10:											return GL_RGB;		// 3-component 10:10:10,    unsigned normalized
		case GL_RGB12:											return GL_RGB;		// 3-component 12:12:12,    unsigned normalized
		case GL_RGBA2:											return GL_RGBA;		// 4-component 2:2:2:2,     unsigned normalized
		case GL_RGBA4:											return GL_RGBA;		// 4-component 4:4:4:4,     unsigned normalized
		case GL_RGBA12:											return GL_RGBA;		// 4-component 12:12:12:12, unsigned normalized
		case GL_RGB5_A1:										return GL_RGBA;		// 4-component 5:5:5:1,     unsigned normalized
		case GL_RGB10_A2:										return GL_RGBA;		// 4-component 10:10:10:2,  unsigned normalized
		case GL_RGB10_A2UI:										return GL_RGBA;		// 4-component 10:10:10:2,  unsigned integer
		case GL_R11F_G11F_B10F:									return GL_RGB;		// 3-component 11:11:10,    floating-point
		case GL_RGB9_E5:										return GL_RGB;		// 3-component/exp 9:9:9/5, floating-point

		//
		// S3TC/DXT/BC
		//

		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:					return GL_RGB;		// line through 3D space, unsigned normalized
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:					return GL_RGBA;		// line through 3D space plus 1-bit alpha, unsigned normalized
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:					return GL_RGBA;		// line through 3D space plus line through 1D space, unsigned normalized
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:					return GL_RGBA;		// line through 3D space plus 4-bit alpha, unsigned normalized

		case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:					return GL_RGB;		// line through 3D space, sRGB
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:			return GL_RGBA;		// line through 3D space plus 1-bit alpha, sRGB
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:			return GL_RGBA;		// line through 3D space plus line through 1D space, sRGB
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:			return GL_RGBA;		// line through 3D space plus 4-bit alpha, sRGB

		case GL_COMPRESSED_LUMINANCE_LATC1_EXT:					return GL_RED;		// line through 1D space, unsigned normalized
		case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:			return GL_RG;		// two lines through 1D space, unsigned normalized
		case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:			return GL_RED;		// line through 1D space, signed normalized
		case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:	return GL_RG;		// two lines through 1D space, signed normalized

		case GL_COMPRESSED_RED_RGTC1:							return GL_RED;		// line through 1D space, unsigned normalized
		case GL_COMPRESSED_RG_RGTC2:							return GL_RG;		// two lines through 1D space, unsigned normalized
		case GL_COMPRESSED_SIGNED_RED_RGTC1:					return GL_RED;		// line through 1D space, signed normalized
		case GL_COMPRESSED_SIGNED_RG_RGTC2:						return GL_RG;		// two lines through 1D space, signed normalized

		case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:				return GL_RGB;		// 3-component, unsigned floating-point
		case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:				return GL_RGB;		// 3-component, signed floating-point
		case GL_COMPRESSED_RGBA_BPTC_UNORM:						return GL_RGBA;		// 4-component, unsigned normalized
		case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:				return GL_RGBA;		// 4-component, sRGB

		//
		// ETC
		//
		case GL_ETC1_RGB8_OES:									return GL_RGB;		// 3-component ETC1, unsigned normalized" ),

		case GL_COMPRESSED_RGB8_ETC2:							return GL_RGB;		// 3-component ETC2, unsigned normalized
		case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:		return GL_RGBA;		// 4-component ETC2 with 1-bit alpha, unsigned normalized
		case GL_COMPRESSED_RGBA8_ETC2_EAC:						return GL_RGBA;		// 4-component ETC2, unsigned normalized

		case GL_COMPRESSED_SRGB8_ETC2:							return GL_RGB;		// 3-component ETC2, sRGB
		case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:		return GL_RGBA;		// 4-component ETC2 with 1-bit alpha, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:				return GL_RGBA;		// 4-component ETC2, sRGB

		case GL_COMPRESSED_R11_EAC:								return GL_RED;		// 1-component ETC, unsigned normalized
		case GL_COMPRESSED_RG11_EAC:							return GL_RG;		// 2-component ETC, unsigned normalized
		case GL_COMPRESSED_SIGNED_R11_EAC:						return GL_RED;		// 1-component ETC, signed normalized
		case GL_COMPRESSED_SIGNED_RG11_EAC:						return GL_RG;		// 2-component ETC, signed normalized

		//
		// ASTC
		//
		case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:					return GL_RGBA;		// 4-component ASTC, 4x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:					return GL_RGBA;		// 4-component ASTC, 5x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:					return GL_RGBA;		// 4-component ASTC, 5x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:					return GL_RGBA;		// 4-component ASTC, 6x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:					return GL_RGBA;		// 4-component ASTC, 6x6 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:					return GL_RGBA;		// 4-component ASTC, 8x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:					return GL_RGBA;		// 4-component ASTC, 8x6 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:					return GL_RGBA;		// 4-component ASTC, 8x8 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:					return GL_RGBA;		// 4-component ASTC, 10x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:					return GL_RGBA;		// 4-component ASTC, 10x6 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:					return GL_RGBA;		// 4-component ASTC, 10x8 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:					return GL_RGBA;		// 4-component ASTC, 10x10 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:					return GL_RGBA;		// 4-component ASTC, 12x10 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:					return GL_RGBA;		// 4-component ASTC, 12x12 blocks, unsigned normalized

		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:			return GL_RGBA;		// 4-component ASTC, 4x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:			return GL_RGBA;		// 4-component ASTC, 5x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:			return GL_RGBA;		// 4-component ASTC, 5x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:			return GL_RGBA;		// 4-component ASTC, 6x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:			return GL_RGBA;		// 4-component ASTC, 6x6 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:			return GL_RGBA;		// 4-component ASTC, 8x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:			return GL_RGBA;		// 4-component ASTC, 8x6 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:			return GL_RGBA;		// 4-component ASTC, 8x8 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:			return GL_RGBA;		// 4-component ASTC, 10x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:			return GL_RGBA;		// 4-component ASTC, 10x6 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:			return GL_RGBA;		// 4-component ASTC, 10x8 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:			return GL_RGBA;		// 4-component ASTC, 10x10 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:			return GL_RGBA;		// 4-component ASTC, 12x10 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:			return GL_RGBA;		// 4-component ASTC, 12x12 blocks, sRGB

		case GL_COMPRESSED_RGBA_ASTC_3x3x3_OES:					return GL_RGBA;		// 4-component ASTC, 3x3x3 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_4x3x3_OES:					return GL_RGBA;		// 4-component ASTC, 4x3x3 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_4x4x3_OES:					return GL_RGBA;		// 4-component ASTC, 4x4x3 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_4x4x4_OES:					return GL_RGBA;		// 4-component ASTC, 4x4x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x4x4_OES:					return GL_RGBA;		// 4-component ASTC, 5x4x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x5x4_OES:					return GL_RGBA;		// 4-component ASTC, 5x5x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x5x5_OES:					return GL_RGBA;		// 4-component ASTC, 5x5x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x5x5_OES:					return GL_RGBA;		// 4-component ASTC, 6x5x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x6x5_OES:					return GL_RGBA;		// 4-component ASTC, 6x6x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x6x6_OES:					return GL_RGBA;		// 4-component ASTC, 6x6x6 blocks, unsigned normalized

		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES:			return GL_RGBA;		// 4-component ASTC, 3x3x3 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES:			return GL_RGBA;		// 4-component ASTC, 4x3x3 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES:			return GL_RGBA;		// 4-component ASTC, 4x4x3 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES:			return GL_RGBA;		// 4-component ASTC, 4x4x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES:			return GL_RGBA;		// 4-component ASTC, 5x4x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES:			return GL_RGBA;		// 4-component ASTC, 5x5x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES:			return GL_RGBA;		// 4-component ASTC, 5x5x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES:			return GL_RGBA;		// 4-component ASTC, 6x5x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES:			return GL_RGBA;		// 4-component ASTC, 6x6x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES:			return GL_RGBA;		// 4-component ASTC, 6x6x6 blocks, sRGB

		//
		// ATC
		//
		case GL_ATC_RGB_AMD:									return GL_RGB;		// 3-component, unsigned normalized
		case GL_ATC_RGBA_EXPLICIT_ALPHA_AMD:					return GL_RGBA;		// 4-component, unsigned normalized
		case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD:				return GL_RGBA;		// 4-component, unsigned normalized

		//
		// Palletized
		//
		case GL_PALETTE4_RGB8_OES:								return GL_RGB;		// 3-component 8:8:8,   4-bit palette, unsigned normalized
		case GL_PALETTE4_RGBA8_OES:								return GL_RGBA;		// 4-component 8:8:8:8, 4-bit palette, unsigned normalized
		case GL_PALETTE4_R5_G6_B5_OES:							return GL_RGB;		// 3-component 5:6:5,   4-bit palette, unsigned normalized
		case GL_PALETTE4_RGBA4_OES:								return GL_RGBA;		// 4-component 4:4:4:4, 4-bit palette, unsigned normalized
		case GL_PALETTE4_RGB5_A1_OES:							return GL_RGBA;		// 4-component 5:5:5:1, 4-bit palette, unsigned normalized
		case GL_PALETTE8_RGB8_OES:								return GL_RGB;		// 3-component 8:8:8,   8-bit palette, unsigned normalized
		case GL_PALETTE8_RGBA8_OES:								return GL_RGBA;		// 4-component 8:8:8:8, 8-bit palette, unsigned normalized
		case GL_PALETTE8_R5_G6_B5_OES:							return GL_RGB;		// 3-component 5:6:5,   8-bit palette, unsigned normalized
		case GL_PALETTE8_RGBA4_OES:								return GL_RGBA;		// 4-component 4:4:4:4, 8-bit palette, unsigned normalized
		case GL_PALETTE8_RGB5_A1_OES:							return GL_RGBA;		// 4-component 5:5:5:1, 8-bit palette, unsigned normalized

		default:												return GL_RGBA;
	}
}

static inline GLenum glGetTypeFromInternalFormat( const GLenum internalFormat )
{
	switch ( internalFormat )
	{
		//
		// 8 bits per component
		//
		case GL_R8:												return GL_UNSIGNED_BYTE;	// 1-component, 8-bit unsigned normalized
		case GL_RG8:											return GL_UNSIGNED_BYTE;	// 2-component, 8-bit unsigned normalized
		case GL_RGB8:											return GL_UNSIGNED_BYTE;	// 3-component, 8-bit unsigned normalized
		case GL_RGBA8:											return GL_UNSIGNED_BYTE;	// 4-component, 8-bit unsigned normalized

		case GL_R8_SNORM:										return GL_BYTE;				// 1-component, 8-bit signed normalized
		case GL_RG8_SNORM:										return GL_BYTE;				// 2-component, 8-bit signed normalized
		case GL_RGB8_SNORM:										return GL_BYTE;				// 3-component, 8-bit signed normalized
		case GL_RGBA8_SNORM:									return GL_BYTE;				// 4-component, 8-bit signed normalized

		case GL_R8UI:											return GL_UNSIGNED_BYTE;	// 1-component, 8-bit unsigned integer
		case GL_RG8UI:											return GL_UNSIGNED_BYTE;	// 2-component, 8-bit unsigned integer
		case GL_RGB8UI:											return GL_UNSIGNED_BYTE;	// 3-component, 8-bit unsigned integer
		case GL_RGBA8UI:										return GL_UNSIGNED_BYTE;	// 4-component, 8-bit unsigned integer

		case GL_R8I:											return GL_BYTE;				// 1-component, 8-bit signed integer
		case GL_RG8I:											return GL_BYTE;				// 2-component, 8-bit signed integer
		case GL_RGB8I:											return GL_BYTE;				// 3-component, 8-bit signed integer
		case GL_RGBA8I:											return GL_BYTE;				// 4-component, 8-bit signed integer

		case GL_SR8:											return GL_UNSIGNED_BYTE;	// 1-component, 8-bit sRGB
		case GL_SRG8:											return GL_UNSIGNED_BYTE;	// 2-component, 8-bit sRGB
		case GL_SRGB8:											return GL_UNSIGNED_BYTE;	// 3-component, 8-bit sRGB
		case GL_SRGB8_ALPHA8:									return GL_UNSIGNED_BYTE;	// 4-component, 8-bit sRGB

		//
		// 16 bits per component
		//
		case GL_R16:											return GL_UNSIGNED_SHORT;	// 1-component, 16-bit unsigned normalized
		case GL_RG16:											return GL_UNSIGNED_SHORT;	// 2-component, 16-bit unsigned normalized
		case GL_RGB16:											return GL_UNSIGNED_SHORT;	// 3-component, 16-bit unsigned normalized
		case GL_RGBA16:											return GL_UNSIGNED_SHORT;	// 4-component, 16-bit unsigned normalized

		case GL_R16_SNORM:										return GL_SHORT;			// 1-component, 16-bit signed normalized
		case GL_RG16_SNORM:										return GL_SHORT;			// 2-component, 16-bit signed normalized
		case GL_RGB16_SNORM:									return GL_SHORT;			// 3-component, 16-bit signed normalized
		case GL_RGBA16_SNORM:									return GL_SHORT;			// 4-component, 16-bit signed normalized

		case GL_R16UI:											return GL_UNSIGNED_SHORT;	// 1-component, 16-bit unsigned integer
		case GL_RG16UI:											return GL_UNSIGNED_SHORT;	// 2-component, 16-bit unsigned integer
		case GL_RGB16UI:										return GL_UNSIGNED_SHORT;	// 3-component, 16-bit unsigned integer
		case GL_RGBA16UI:										return GL_UNSIGNED_SHORT;	// 4-component, 16-bit unsigned integer

		case GL_R16I:											return GL_SHORT;			// 1-component, 16-bit signed integer
		case GL_RG16I:											return GL_SHORT;			// 2-component, 16-bit signed integer
		case GL_RGB16I:											return GL_SHORT;			// 3-component, 16-bit signed integer
		case GL_RGBA16I:										return GL_SHORT;			// 4-component, 16-bit signed integer

		case GL_R16F:											return GL_HALF_FLOAT;		// 1-component, 16-bit floating-point
		case GL_RG16F:											return GL_HALF_FLOAT;		// 2-component, 16-bit floating-point
		case GL_RGB16F:											return GL_HALF_FLOAT;		// 3-component, 16-bit floating-point
		case GL_RGBA16F:										return GL_HALF_FLOAT;		// 4-component, 16-bit floating-point

		//
		// 32 bits per component
		//
		case GL_R32UI:											return GL_UNSIGNED_INT;		// 1-component, 32-bit unsigned integer
		case GL_RG32UI:											return GL_UNSIGNED_INT;		// 2-component, 32-bit unsigned integer
		case GL_RGB32UI:										return GL_UNSIGNED_INT;		// 3-component, 32-bit unsigned integer
		case GL_RGBA32UI:										return GL_UNSIGNED_INT;		// 4-component, 32-bit unsigned integer

		case GL_R32I:											return GL_INT;				// 1-component, 32-bit signed integer
		case GL_RG32I:											return GL_INT;				// 2-component, 32-bit signed integer
		case GL_RGB32I:											return GL_INT;				// 3-component, 32-bit signed integer
		case GL_RGBA32I:										return GL_INT;				// 4-component, 32-bit signed integer

		case GL_R32F:											return GL_FLOAT;			// 1-component, 32-bit floating-point
		case GL_RG32F:											return GL_FLOAT;			// 2-component, 32-bit floating-point
		case GL_RGB32F:											return GL_FLOAT;			// 3-component, 32-bit floating-point
		case GL_RGBA32F:										return GL_FLOAT;			// 4-component, 32-bit floating-point

		//
		// Odd bits per component
		//
		case GL_R3_G3_B2:										return GL_UNSIGNED_BYTE;	// 3-component 3:3:2,       unsigned normalized
		case GL_RGB4:											return GL_UNSIGNED_BYTE;	// 3-component 4:4:4,       unsigned normalized
		case GL_RGB5:											return GL_UNSIGNED_BYTE;	// 3-component 5:5:5,       unsigned normalized
		case GL_RGB565:											return GL_UNSIGNED_BYTE;	// 3-component 5:6:5,       unsigned normalized
		case GL_RGB10:											return GL_UNSIGNED_BYTE;	// 3-component 10:10:10,    unsigned normalized
		case GL_RGB12:											return GL_UNSIGNED_BYTE;	// 3-component 12:12:12,    unsigned normalized
		case GL_RGBA2:											return GL_UNSIGNED_BYTE;	// 4-component 2:2:2:2,     unsigned normalized
		case GL_RGBA4:											return GL_UNSIGNED_BYTE;	// 4-component 4:4:4:4,     unsigned normalized
		case GL_RGBA12:											return GL_UNSIGNED_BYTE;	// 4-component 12:12:12:12, unsigned normalized
		case GL_RGB5_A1:										return GL_UNSIGNED_BYTE;	// 4-component 5:5:5:1,     unsigned normalized
		case GL_RGB10_A2:										return GL_UNSIGNED_BYTE;	// 4-component 10:10:10:2,  unsigned normalized
		case GL_RGB10_A2UI:										return GL_UNSIGNED_BYTE;	// 4-component 10:10:10:2,  unsigned integer
		case GL_R11F_G11F_B10F:									return GL_FLOAT;			// 3-component 11:11:10,    floating-point
		case GL_RGB9_E5:										return GL_FLOAT;			// 3-component/exp 9:9:9/5, floating-point

		//
		// S3TC/DXT/BC
		//

		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:					return GL_UNSIGNED_BYTE;	// line through 3D space, unsigned normalized
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:					return GL_UNSIGNED_BYTE;	// line through 3D space plus 1-bit alpha, unsigned normalized
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:					return GL_UNSIGNED_BYTE;	// line through 3D space plus line through 1D space, unsigned normalized
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:					return GL_UNSIGNED_BYTE;	// line through 3D space plus 4-bit alpha, unsigned normalized

		case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:					return GL_UNSIGNED_BYTE;	// line through 3D space, sRGB
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:			return GL_UNSIGNED_BYTE;	// line through 3D space plus 1-bit alpha, sRGB
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:			return GL_UNSIGNED_BYTE;	// line through 3D space plus line through 1D space, sRGB
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:			return GL_UNSIGNED_BYTE;	// line through 3D space plus 4-bit alpha, sRGB

		case GL_COMPRESSED_LUMINANCE_LATC1_EXT:					return GL_UNSIGNED_BYTE;	// line through 1D space, unsigned normalized
		case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:			return GL_UNSIGNED_BYTE;	// two lines through 1D space, unsigned normalized
		case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:			return GL_UNSIGNED_BYTE;	// line through 1D space, signed normalized
		case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:	return GL_UNSIGNED_BYTE;	// two lines through 1D space, signed normalized

		case GL_COMPRESSED_RED_RGTC1:							return GL_UNSIGNED_BYTE;	// line through 1D space, unsigned normalized
		case GL_COMPRESSED_RG_RGTC2:							return GL_UNSIGNED_BYTE;	// two lines through 1D space, unsigned normalized
		case GL_COMPRESSED_SIGNED_RED_RGTC1:					return GL_UNSIGNED_BYTE;	// line through 1D space, signed normalized
		case GL_COMPRESSED_SIGNED_RG_RGTC2:						return GL_UNSIGNED_BYTE;	// two lines through 1D space, signed normalized

		case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:				return GL_FLOAT;			// 3-component, unsigned floating-point
		case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:				return GL_FLOAT;			// 3-component, signed floating-point
		case GL_COMPRESSED_RGBA_BPTC_UNORM:						return GL_UNSIGNED_BYTE;	// 4-component, unsigned normalized
		case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:				return GL_UNSIGNED_BYTE;	// 4-component, sRGB

		//
		// ETC
		//
		case GL_ETC1_RGB8_OES:									return GL_UNSIGNED_BYTE;	// 3-component ETC1, unsigned normalized" ),

		case GL_COMPRESSED_RGB8_ETC2:							return GL_UNSIGNED_BYTE;	// 3-component ETC2, unsigned normalized
		case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:		return GL_UNSIGNED_BYTE;	// 4-component ETC2 with 1-bit alpha, unsigned normalized
		case GL_COMPRESSED_RGBA8_ETC2_EAC:						return GL_UNSIGNED_BYTE;	// 4-component ETC2, unsigned normalized

		case GL_COMPRESSED_SRGB8_ETC2:							return GL_UNSIGNED_BYTE;	// 3-component ETC2, sRGB
		case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:		return GL_UNSIGNED_BYTE;	// 4-component ETC2 with 1-bit alpha, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:				return GL_UNSIGNED_BYTE;	// 4-component ETC2, sRGB

		case GL_COMPRESSED_R11_EAC:								return GL_UNSIGNED_BYTE;	// 1-component ETC, unsigned normalized
		case GL_COMPRESSED_RG11_EAC:							return GL_UNSIGNED_BYTE;	// 2-component ETC, unsigned normalized
		case GL_COMPRESSED_SIGNED_R11_EAC:						return GL_UNSIGNED_BYTE;	// 1-component ETC, signed normalized
		case GL_COMPRESSED_SIGNED_RG11_EAC:						return GL_UNSIGNED_BYTE;	// 2-component ETC, signed normalized

		//
		// ASTC
		//
		case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 4x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 5x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 5x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 6x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 6x6 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 8x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 8x6 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 8x8 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 10x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 10x6 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 10x8 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 10x10 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 12x10 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 12x12 blocks, unsigned normalized

		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 4x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 5x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 5x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 6x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 6x6 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 8x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 8x6 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 8x8 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 10x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 10x6 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 10x8 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 10x10 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 12x10 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 12x12 blocks, sRGB

		case GL_COMPRESSED_RGBA_ASTC_3x3x3_OES:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 3x3x3 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_4x3x3_OES:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 4x3x3 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_4x4x3_OES:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 4x4x3 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_4x4x4_OES:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 4x4x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x4x4_OES:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 5x4x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x5x4_OES:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 5x5x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x5x5_OES:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 5x5x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x5x5_OES:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 6x5x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x6x5_OES:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 6x6x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x6x6_OES:					return GL_UNSIGNED_BYTE;	// 4-component ASTC, 6x6x6 blocks, unsigned normalized

		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 3x3x3 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 4x3x3 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 4x4x3 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 4x4x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 5x4x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 5x5x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 5x5x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 6x5x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 6x6x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES:			return GL_UNSIGNED_BYTE;	// 4-component ASTC, 6x6x6 blocks, sRGB

		//
		// ATC
		//
		case GL_ATC_RGB_AMD:									return GL_UNSIGNED_BYTE;	// 3-component, unsigned normalized
		case GL_ATC_RGBA_EXPLICIT_ALPHA_AMD:					return GL_UNSIGNED_BYTE;	// 4-component, unsigned normalized
		case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD:				return GL_UNSIGNED_BYTE;	// 4-component, unsigned normalized

		//
		// Palletized
		//
		case GL_PALETTE4_RGB8_OES:								return GL_UNSIGNED_BYTE;	// 3-component 8:8:8,   4-bit palette, unsigned normalized
		case GL_PALETTE4_RGBA8_OES:								return GL_UNSIGNED_BYTE;	// 4-component 8:8:8:8, 4-bit palette, unsigned normalized
		case GL_PALETTE4_R5_G6_B5_OES:							return GL_UNSIGNED_BYTE;	// 3-component 5:6:5,   4-bit palette, unsigned normalized
		case GL_PALETTE4_RGBA4_OES:								return GL_UNSIGNED_BYTE;	// 4-component 4:4:4:4, 4-bit palette, unsigned normalized
		case GL_PALETTE4_RGB5_A1_OES:							return GL_UNSIGNED_BYTE;	// 4-component 5:5:5:1, 4-bit palette, unsigned normalized
		case GL_PALETTE8_RGB8_OES:								return GL_UNSIGNED_BYTE;	// 3-component 8:8:8,   8-bit palette, unsigned normalized
		case GL_PALETTE8_RGBA8_OES:								return GL_UNSIGNED_BYTE;	// 4-component 8:8:8:8, 8-bit palette, unsigned normalized
		case GL_PALETTE8_R5_G6_B5_OES:							return GL_UNSIGNED_BYTE;	// 3-component 5:6:5,   8-bit palette, unsigned normalized
		case GL_PALETTE8_RGBA4_OES:								return GL_UNSIGNED_BYTE;	// 4-component 4:4:4:4, 8-bit palette, unsigned normalized
		case GL_PALETTE8_RGB5_A1_OES:							return GL_UNSIGNED_BYTE;	// 4-component 5:5:5:1, 8-bit palette, unsigned normalized

		default:												return GL_UNSIGNED_BYTE;
	}
}

typedef enum GlFormatSizeFlagBits {
	GL_FORMAT_SIZE_PACKED_BIT = 0x00000001,
	GL_FORMAT_SIZE_COMPRESSED_BIT = 0x00000002,
	GL_FORMAT_SIZE_PALETTIZED_BIT = 0x00000004,
} GlFormatSizeFlagBits;

typedef unsigned int GlFormatSizeFlags;

typedef struct GlFormatSize {
	GlFormatSizeFlags	flags;
	unsigned int		paletteSizeInBits;
	unsigned int		blockSizeInBits;
	unsigned int		blockWidth;			// in texels
	unsigned int		blockHeight;		// in texels
	unsigned int		blockDepth;			// in texels
} GlFormatSize;

static inline void glGetFormatSize( const GLenum internalFormat, GlFormatSize * pFormatSize )
{
	switch ( internalFormat )
	{
		//
		// 8 bits per component
		//
		case GL_R8:												// 1-component, 8-bit unsigned normalized
		case GL_R8_SNORM:										// 1-component, 8-bit signed normalized
		case GL_R8UI:											// 1-component, 8-bit unsigned integer
		case GL_R8I:											// 1-component, 8-bit signed integer
		case GL_SR8:											// 1-component, 8-bit sRGB
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 1 * 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RG8:											// 2-component, 8-bit unsigned normalized
		case GL_RG8_SNORM:										// 2-component, 8-bit signed normalized
		case GL_RG8UI:											// 2-component, 8-bit unsigned integer
		case GL_RG8I:											// 2-component, 8-bit signed integer
		case GL_SRG8:											// 2-component, 8-bit sRGB
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 2 * 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGB8:											// 3-component, 8-bit unsigned normalized
		case GL_RGB8_SNORM:										// 3-component, 8-bit signed normalized
		case GL_RGB8UI:											// 3-component, 8-bit unsigned integer
		case GL_RGB8I:											// 3-component, 8-bit signed integer
		case GL_SRGB8:											// 3-component, 8-bit sRGB
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 3 * 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGBA8:											// 4-component, 8-bit unsigned normalized
		case GL_RGBA8_SNORM:									// 4-component, 8-bit signed normalized
		case GL_RGBA8UI:										// 4-component, 8-bit unsigned integer
		case GL_RGBA8I:											// 4-component, 8-bit signed integer
		case GL_SRGB8_ALPHA8:									// 4-component, 8-bit sRGB
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 4 * 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;

		//
		// 16 bits per component
		//
		case GL_R16:											// 1-component, 16-bit unsigned normalized
		case GL_R16_SNORM:										// 1-component, 16-bit signed normalized
		case GL_R16UI:											// 1-component, 16-bit unsigned integer
		case GL_R16I:											// 1-component, 16-bit signed integer
		case GL_R16F:											// 1-component, 16-bit floating-point
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 2 * 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RG16:											// 2-component, 16-bit unsigned normalized
		case GL_RG16_SNORM:										// 2-component, 16-bit signed normalized
		case GL_RG16UI:											// 2-component, 16-bit unsigned integer
		case GL_RG16I:											// 2-component, 16-bit signed integer
		case GL_RG16F:											// 2-component, 16-bit floating-point
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 4 * 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGB16:											// 3-component, 16-bit unsigned normalized
		case GL_RGB16_SNORM:									// 3-component, 16-bit signed normalized
		case GL_RGB16UI:										// 3-component, 16-bit unsigned integer
		case GL_RGB16I:											// 3-component, 16-bit signed integer
		case GL_RGB16F:											// 3-component, 16-bit floating-point
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 6 * 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGBA16:											// 4-component, 16-bit unsigned normalized
		case GL_RGBA16_SNORM:									// 4-component, 16-bit signed normalized
		case GL_RGBA16UI:										// 4-component, 16-bit unsigned integer
		case GL_RGBA16I:										// 4-component, 16-bit signed integer
		case GL_RGBA16F:										// 4-component, 16-bit floating-point
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 8 * 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;

		//
		// 32 bits per component
		//
		case GL_R32UI:											// 1-component, 32-bit unsigned integer
		case GL_R32I:											// 1-component, 32-bit signed integer
		case GL_R32F:											// 1-component, 32-bit floating-point
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 4 * 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RG32UI:											// 2-component, 32-bit unsigned integer
		case GL_RG32I:											// 2-component, 32-bit signed integer
		case GL_RG32F:											// 2-component, 32-bit floating-point
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 8 * 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGB32UI:										// 3-component, 32-bit unsigned integer
		case GL_RGB32I:											// 3-component, 32-bit signed integer
		case GL_RGB32F:											// 3-component, 32-bit floating-point
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 12 * 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGBA32UI:										// 4-component, 32-bit unsigned integer
		case GL_RGBA32I:										// 4-component, 32-bit signed integer
		case GL_RGBA32F:										// 4-component, 32-bit floating-point
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 16 * 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;

		//
		// Odd bits per component
		//
		case GL_R3_G3_B2:										// 3-component 3:3:2, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGB4:											// 3-component 4:4:4, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 12;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGB5:											// 3-component 5:5:5, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 15;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGB565:											// 3-component 5:6:5, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 16;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGB10:											// 3-component 10:10:10, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 30;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGB12:											// 3-component 12:12:12, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 36;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGBA2:											// 4-component 2:2:2:2, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGBA4:											// 4-component 4:4:4:4, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 16;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGBA12:											// 4-component 12:12:12:12, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 16;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGB5_A1:										// 4-component 5:5:5:1, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 32;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGB10_A2:										// 4-component 10:10:10:2, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 32;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_RGB10_A2UI:										// 4-component 10:10:10:2, unsigned integer
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 16;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_R11F_G11F_B10F:									// 3-component 11:11:10, floating-point
		case GL_RGB9_E5:										// 3-component/exp 9:9:9/5, floating-point
			pFormatSize->flags = GL_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 32;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;

		//
		// S3TC/DXT/BC
		//

		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:					// line through 3D space, unsigned normalized
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:					// line through 3D space plus 1-bit alpha, unsigned normalized
		case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:					// line through 3D space, sRGB
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:			// line through 3D space plus 1-bit alpha, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 64;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:					// line through 3D space plus line through 1D space, unsigned normalized
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:					// line through 3D space plus 4-bit alpha, unsigned normalized
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:			// line through 3D space plus line through 1D space, sRGB
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:			// line through 3D space plus 4-bit alpha, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;

		case GL_COMPRESSED_LUMINANCE_LATC1_EXT:					// line through 1D space, unsigned normalized
		case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:			// line through 1D space, signed normalized
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 64;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:			// two lines through 1D space, unsigned normalized
		case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:	// two lines through 1D space, signed normalized
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;

		case GL_COMPRESSED_RED_RGTC1:							// line through 1D space, unsigned normalized
		case GL_COMPRESSED_SIGNED_RED_RGTC1:					// line through 1D space, signed normalized
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 64;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RG_RGTC2:							// two lines through 1D space, unsigned normalized
		case GL_COMPRESSED_SIGNED_RG_RGTC2:						// two lines through 1D space, signed normalized
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;

		case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:				// 3-component, unsigned floating-point
		case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:				// 3-component, signed floating-point
		case GL_COMPRESSED_RGBA_BPTC_UNORM:						// 4-component, unsigned normalized
		case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:				// 4-component, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;

		//
		// ETC
		//
		case GL_ETC1_RGB8_OES:									// 3-component ETC1, unsigned normalized" ),
		case GL_COMPRESSED_RGB8_ETC2:							// 3-component ETC2, unsigned normalized
		case GL_COMPRESSED_SRGB8_ETC2:							// 3-component ETC2, sRGB
		case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:		// 4-component ETC2 with 1-bit alpha, unsigned normalized
		case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:		// 4-component ETC2 with 1-bit alpha, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 64;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA8_ETC2_EAC:						// 4-component ETC2, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:				// 4-component ETC2, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;

		case GL_COMPRESSED_R11_EAC:								// 1-component ETC, unsigned normalized
		case GL_COMPRESSED_SIGNED_R11_EAC:						// 1-component ETC, signed normalized
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 64;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RG11_EAC:							// 2-component ETC, unsigned normalized
		case GL_COMPRESSED_SIGNED_RG11_EAC:						// 2-component ETC, signed normalized
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;

		//
		// ASTC
		//
		case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:					// 4-component ASTC, 4x4 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:			// 4-component ASTC, 4x4 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:					// 4-component ASTC, 5x4 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:			// 4-component ASTC, 5x4 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 5;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:					// 4-component ASTC, 5x5 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:			// 4-component ASTC, 5x5 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 5;
			pFormatSize->blockHeight = 6;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:					// 4-component ASTC, 6x5 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:			// 4-component ASTC, 6x5 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 6;
			pFormatSize->blockHeight = 5;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:					// 4-component ASTC, 6x6 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:			// 4-component ASTC, 6x6 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 6;
			pFormatSize->blockHeight = 6;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:					// 4-component ASTC, 8x5 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:			// 4-component ASTC, 8x5 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 8;
			pFormatSize->blockHeight = 5;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:					// 4-component ASTC, 8x6 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:			// 4-component ASTC, 8x6 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 8;
			pFormatSize->blockHeight = 6;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:					// 4-component ASTC, 8x8 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:			// 4-component ASTC, 8x8 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 8;
			pFormatSize->blockHeight = 8;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:					// 4-component ASTC, 10x5 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:			// 4-component ASTC, 10x5 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 10;
			pFormatSize->blockHeight = 5;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:					// 4-component ASTC, 10x6 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:			// 4-component ASTC, 10x6 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 10;
			pFormatSize->blockHeight = 6;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:					// 4-component ASTC, 10x8 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:			// 4-component ASTC, 10x8 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 10;
			pFormatSize->blockHeight = 8;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:					// 4-component ASTC, 10x10 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:			// 4-component ASTC, 10x10 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 10;
			pFormatSize->blockHeight = 10;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:					// 4-component ASTC, 12x10 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:			// 4-component ASTC, 12x10 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 12;
			pFormatSize->blockHeight = 10;
			pFormatSize->blockDepth = 1;
			break;
		case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:					// 4-component ASTC, 12x12 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:			// 4-component ASTC, 12x12 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 12;
			pFormatSize->blockHeight = 12;
			pFormatSize->blockDepth = 1;
			break;

		case GL_COMPRESSED_RGBA_ASTC_3x3x3_OES:					// 4-component ASTC, 3x3x3 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES:			// 4-component ASTC, 3x3x3 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 3;
			pFormatSize->blockHeight = 3;
			pFormatSize->blockDepth = 3;
			break;
		case GL_COMPRESSED_RGBA_ASTC_4x3x3_OES:					// 4-component ASTC, 4x3x3 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES:			// 4-component ASTC, 4x3x3 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 3;
			pFormatSize->blockDepth = 3;
			break;
		case GL_COMPRESSED_RGBA_ASTC_4x4x3_OES:					// 4-component ASTC, 4x4x3 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES:			// 4-component ASTC, 4x4x3 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 3;
			break;
		case GL_COMPRESSED_RGBA_ASTC_4x4x4_OES:					// 4-component ASTC, 4x4x4 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES:			// 4-component ASTC, 4x4x4 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 4;
			break;
		case GL_COMPRESSED_RGBA_ASTC_5x4x4_OES:					// 4-component ASTC, 5x4x4 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES:			// 4-component ASTC, 5x4x4 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 5;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 4;
			break;
		case GL_COMPRESSED_RGBA_ASTC_5x5x4_OES:					// 4-component ASTC, 5x5x4 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES:			// 4-component ASTC, 5x5x4 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 5;
			pFormatSize->blockHeight = 5;
			pFormatSize->blockDepth = 4;
			break;
		case GL_COMPRESSED_RGBA_ASTC_5x5x5_OES:					// 4-component ASTC, 5x5x5 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES:			// 4-component ASTC, 5x5x5 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 5;
			pFormatSize->blockHeight = 5;
			pFormatSize->blockDepth = 5;
			break;
		case GL_COMPRESSED_RGBA_ASTC_6x5x5_OES:					// 4-component ASTC, 6x5x5 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES:			// 4-component ASTC, 6x5x5 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 6;
			pFormatSize->blockHeight = 5;
			pFormatSize->blockDepth = 5;
			break;
		case GL_COMPRESSED_RGBA_ASTC_6x6x5_OES:					// 4-component ASTC, 6x6x5 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES:			// 4-component ASTC, 6x6x5 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 6;
			pFormatSize->blockHeight = 6;
			pFormatSize->blockDepth = 5;
			break;
		case GL_COMPRESSED_RGBA_ASTC_6x6x6_OES:					// 4-component ASTC, 6x6x6 blocks, unsigned normalized
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES:			// 4-component ASTC, 6x6x6 blocks, sRGB
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 6;
			pFormatSize->blockHeight = 6;
			pFormatSize->blockDepth = 6;
			break;

		//
		// ATC
		//
		case GL_ATC_RGB_AMD:									// 3-component, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 64;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case GL_ATC_RGBA_EXPLICIT_ALPHA_AMD:					// 4-component, unsigned normalized
		case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD:				// 4-component, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 128;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;

		//
		// Palletized
		//
		case GL_PALETTE4_RGB8_OES:								// 3-component 8:8:8,   4-bit palette, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PALETTIZED_BIT;
			pFormatSize->paletteSizeInBits = 16 * 24;
			pFormatSize->blockSizeInBits = 4;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_PALETTE4_RGBA8_OES:								// 4-component 8:8:8:8, 4-bit palette, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PALETTIZED_BIT;
			pFormatSize->paletteSizeInBits = 16 * 32;
			pFormatSize->blockSizeInBits = 4;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_PALETTE4_R5_G6_B5_OES:							// 3-component 5:6:5,   4-bit palette, unsigned normalized
		case GL_PALETTE4_RGBA4_OES:								// 4-component 4:4:4:4, 4-bit palette, unsigned normalized
		case GL_PALETTE4_RGB5_A1_OES:							// 4-component 5:5:5:1, 4-bit palette, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PALETTIZED_BIT;
			pFormatSize->paletteSizeInBits = 16 * 16;
			pFormatSize->blockSizeInBits = 4;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_PALETTE8_RGB8_OES:								// 3-component 8:8:8,   8-bit palette, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PALETTIZED_BIT;
			pFormatSize->paletteSizeInBits = 256 * 24;
			pFormatSize->blockSizeInBits = 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_PALETTE8_RGBA8_OES:								// 4-component 8:8:8:8, 8-bit palette, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PALETTIZED_BIT;
			pFormatSize->paletteSizeInBits = 256 * 32;
			pFormatSize->blockSizeInBits = 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case GL_PALETTE8_R5_G6_B5_OES:							// 3-component 5:6:5,   8-bit palette, unsigned normalized
		case GL_PALETTE8_RGBA4_OES:								// 4-component 4:4:4:4, 8-bit palette, unsigned normalized
		case GL_PALETTE8_RGB5_A1_OES:							// 4-component 5:5:5:1, 8-bit palette, unsigned normalized
			pFormatSize->flags = GL_FORMAT_SIZE_PALETTIZED_BIT;
			pFormatSize->paletteSizeInBits = 256 * 16;
			pFormatSize->blockSizeInBits = 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;

		default:
			pFormatSize->flags = 0;
			pFormatSize->paletteSizeInBits = 0;
			pFormatSize->blockSizeInBits = 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
	}
}

/*
================================================================================================

Description	:	Vulkan format properties and conversion from OpenGL.
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


IMPLEMENTATION
==============

This file does not include OpenGL / OpenGL ES headers because:
  1. including OpenGL / OpenGL ES headers is a platform dependent mess
  2. file formats like KTX and glTF may use OpenGL formats and types that
     on the platform are not supported in OpenGL but are supported in Vulkan


ENTRY POINTS
============

inline VkFormat vkGetVulkanFormatFromOpenGLFormat( const GLenum format, const GLenum type );
inline VkFormat vkGetVulkanFormatFromOpenGLType( const GLenum type, const GLuint numComponents );
inline VkFormat vkGetVulkanFormatFromOpenGLInternalFormat( const GLenum internalFormat );
inline void vkGetVulkanFormatSize( const VkFormat format, VkFormatSize * pFormatSize );

================================================================================================
*/

#if defined(_WIN32)
#define NOMINMAX
#ifndef __cplusplus
#undef inline
#define inline __inline
#endif // __cplusplus
#endif

typedef unsigned int GLenum;
typedef unsigned int GLuint;

// format to glTexImage2D / glTexImage3D
#if !defined( GL_RED )
#define GL_RED											0x1903
#endif
#if !defined( GL_RG )
#define GL_RG											0x8227
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
#define GL_RED_INTEGER									0x8D94
#endif
#if !defined( GL_RG_INTEGER )
#define GL_RG_INTEGER									0x8228
#endif
#if !defined( GL_RGB_INTEGER )
#define GL_RGB_INTEGER									0x8D98
#endif
#if !defined( GL_BGR_INTEGER )
#define GL_BGR_INTEGER									0x8D9A
#endif
#if !defined( GL_RGBA_INTEGER )
#define GL_RGBA_INTEGER									0x8D99
#endif
#if !defined( GL_BGRA_INTEGER )
#define GL_BGRA_INTEGER									0x8D9B
#endif
#if !defined( GL_STENCIL_INDEX )
#define GL_STENCIL_INDEX								0x1901
#endif
#if !defined( GL_DEPTH_COMPONENT )
#define GL_DEPTH_COMPONENT								0x1902
#endif
#if !defined( GL_DEPTH_STENCIL )
#define GL_DEPTH_STENCIL								0x84F9	// same as GL_DEPTH_STENCIL_NV and GL_DEPTH_STENCIL_EXT
#endif

// type to glTexImage2D / glTexImage3D
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
#if !defined( GL_FLOAT )
#define GL_FLOAT										0x1406
#endif
#if !defined( GL_HALF_FLOAT )
#define GL_HALF_FLOAT									0x140B	// same as GL_HALF_FLOAT_NV and GL_HALF_FLOAT_ARB
#endif
#if !defined( GL_UNSIGNED_BYTE_3_3_2 )
#define GL_UNSIGNED_BYTE_3_3_2							0x8032	// same as GL_UNSIGNED_BYTE_3_3_2_EXT
#endif
#if !defined( GL_UNSIGNED_BYTE_2_3_3_REV )
#define GL_UNSIGNED_BYTE_2_3_3_REV						0x8362
#endif
#if !defined( GL_UNSIGNED_SHORT_5_6_5 )
#define GL_UNSIGNED_SHORT_5_6_5							0x8363
#endif
#if !defined( GL_UNSIGNED_SHORT_5_6_5_REV )
#define GL_UNSIGNED_SHORT_5_6_5_REV						0x8364
#endif
#if !defined( GL_UNSIGNED_SHORT_4_4_4_4 )
#define GL_UNSIGNED_SHORT_4_4_4_4						0x8033	// same as GL_UNSIGNED_SHORT_4_4_4_4_EXT
#endif
#if !defined( GL_UNSIGNED_SHORT_4_4_4_4_REV )
#define GL_UNSIGNED_SHORT_4_4_4_4_REV					0x8365
#endif
#if !defined( GL_UNSIGNED_SHORT_5_5_5_1 )
#define GL_UNSIGNED_SHORT_5_5_5_1						0x8034	// same as GL_UNSIGNED_SHORT_5_5_5_1_EXT
#endif
#if !defined( GL_UNSIGNED_SHORT_1_5_5_5_REV )
#define GL_UNSIGNED_SHORT_1_5_5_5_REV					0x8366
#endif
#if !defined( GL_UNSIGNED_INT_8_8_8_8 )
#define GL_UNSIGNED_INT_8_8_8_8							0x8035	// same as GL_UNSIGNED_INT_8_8_8_8_EXT
#endif
#if !defined( GL_UNSIGNED_INT_8_8_8_8_REV )
#define GL_UNSIGNED_INT_8_8_8_8_REV						0x8367
#endif
#if !defined( GL_UNSIGNED_INT_10_10_10_2 )
#define GL_UNSIGNED_INT_10_10_10_2						0x8036	// same as GL_UNSIGNED_INT_10_10_10_2_EXT
#endif
#if !defined( GL_UNSIGNED_INT_2_10_10_10_REV )
#define GL_UNSIGNED_INT_2_10_10_10_REV					0x8368
#endif

inline VkFormat vkGetVulkanFormatFromOpenGLFormat( const GLenum format, const GLenum type )
{
	switch ( type )
	{
		//
		// 8 bits per component
		//
		case GL_UNSIGNED_BYTE:
		{
			switch ( format )
			{
				case GL_RED:					return VK_FORMAT_R8_UNORM;
				case GL_RG:						return VK_FORMAT_R8G8_UNORM;
				case GL_RGB:					return VK_FORMAT_R8G8B8_UNORM;
				case GL_BGR:					return VK_FORMAT_B8G8R8_UNORM;
				case GL_RGBA:					return VK_FORMAT_R8G8B8A8_UNORM;
				case GL_BGRA:					return VK_FORMAT_B8G8R8A8_UNORM;
				case GL_RED_INTEGER:			return VK_FORMAT_R8_UINT;
				case GL_RG_INTEGER:				return VK_FORMAT_R8G8_UINT;
				case GL_RGB_INTEGER:			return VK_FORMAT_R8G8B8_UINT;
				case GL_BGR_INTEGER:			return VK_FORMAT_B8G8R8_UINT;
				case GL_RGBA_INTEGER:			return VK_FORMAT_R8G8B8A8_UINT;
				case GL_BGRA_INTEGER:			return VK_FORMAT_B8G8R8A8_UINT;
				case GL_STENCIL_INDEX:			return VK_FORMAT_S8_UINT;
				case GL_DEPTH_COMPONENT:		return VK_FORMAT_X8_D24_UNORM_PACK32;
				case GL_DEPTH_STENCIL:			return VK_FORMAT_D24_UNORM_S8_UINT;
			}
			break;
		}
		case GL_BYTE:
		{
			switch ( format )
			{
				case GL_RED:					return VK_FORMAT_R8_SNORM;
				case GL_RG:						return VK_FORMAT_R8G8_SNORM;
				case GL_RGB:					return VK_FORMAT_R8G8B8_SNORM;
				case GL_BGR:					return VK_FORMAT_B8G8R8_SNORM;
				case GL_RGBA:					return VK_FORMAT_R8G8B8A8_SNORM;
				case GL_BGRA:					return VK_FORMAT_B8G8R8A8_SNORM;
				case GL_RED_INTEGER:			return VK_FORMAT_R8_SINT;
				case GL_RG_INTEGER:				return VK_FORMAT_R8G8_SINT;
				case GL_RGB_INTEGER:			return VK_FORMAT_R8G8B8_SINT;
				case GL_BGR_INTEGER:			return VK_FORMAT_B8G8R8_SINT;
				case GL_RGBA_INTEGER:			return VK_FORMAT_R8G8B8A8_SINT;
				case GL_BGRA_INTEGER:			return VK_FORMAT_B8G8R8A8_SINT;
				case GL_STENCIL_INDEX:			return VK_FORMAT_S8_UINT;
				case GL_DEPTH_COMPONENT:		return VK_FORMAT_X8_D24_UNORM_PACK32;
				case GL_DEPTH_STENCIL:			return VK_FORMAT_D24_UNORM_S8_UINT;
			}
			break;
		}

		//
		// 16 bits per component
		//
		case GL_UNSIGNED_SHORT:
		{
			switch ( format )
			{
				case GL_RED:					return VK_FORMAT_R16_UNORM;
				case GL_RG:						return VK_FORMAT_R16G16_UNORM;
				case GL_RGB:					return VK_FORMAT_R16G16B16_UNORM;
				case GL_BGR:					return VK_FORMAT_UNDEFINED;
				case GL_RGBA:					return VK_FORMAT_R16G16B16A16_UNORM;
				case GL_BGRA:					return VK_FORMAT_UNDEFINED;
				case GL_RED_INTEGER:			return VK_FORMAT_R16_UINT;
				case GL_RG_INTEGER:				return VK_FORMAT_R16G16_UINT;
				case GL_RGB_INTEGER:			return VK_FORMAT_R16G16B16_UINT;
				case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
				case GL_RGBA_INTEGER:			return VK_FORMAT_R16G16B16A16_UINT;
				case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
				case GL_STENCIL_INDEX:			return VK_FORMAT_S8_UINT;
				case GL_DEPTH_COMPONENT:		return VK_FORMAT_D16_UNORM;
				case GL_DEPTH_STENCIL:			return VK_FORMAT_D16_UNORM_S8_UINT;
			}
			break;
		}
		case GL_SHORT:
		{
			switch ( format )
			{
				case GL_RED:					return VK_FORMAT_R16_SNORM;
				case GL_RG:						return VK_FORMAT_R16G16_SNORM;
				case GL_RGB:					return VK_FORMAT_R16G16B16_SNORM;
				case GL_BGR:					return VK_FORMAT_UNDEFINED;
				case GL_RGBA:					return VK_FORMAT_R16G16B16A16_SNORM;
				case GL_BGRA:					return VK_FORMAT_UNDEFINED;
				case GL_RED_INTEGER:			return VK_FORMAT_R16_SINT;
				case GL_RG_INTEGER:				return VK_FORMAT_R16G16_SINT;
				case GL_RGB_INTEGER:			return VK_FORMAT_R16G16B16_SINT;
				case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
				case GL_RGBA_INTEGER:			return VK_FORMAT_R16G16B16A16_SINT;
				case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
				case GL_STENCIL_INDEX:			return VK_FORMAT_S8_UINT;
				case GL_DEPTH_COMPONENT:		return VK_FORMAT_D16_UNORM;
				case GL_DEPTH_STENCIL:			return VK_FORMAT_D16_UNORM_S8_UINT;
			}
			break;
		}
		case GL_HALF_FLOAT:
		{
			switch ( format )
			{
				case GL_RED:					return VK_FORMAT_R16_SFLOAT;
				case GL_RG:						return VK_FORMAT_R16G16_SFLOAT;
				case GL_RGB:					return VK_FORMAT_R16G16B16_SFLOAT;
				case GL_BGR:					return VK_FORMAT_UNDEFINED;
				case GL_RGBA:					return VK_FORMAT_R16G16B16A16_SFLOAT;
				case GL_BGRA:					return VK_FORMAT_UNDEFINED;
				case GL_RED_INTEGER:			return VK_FORMAT_R16_SFLOAT;
				case GL_RG_INTEGER:				return VK_FORMAT_R16G16_SFLOAT;
				case GL_RGB_INTEGER:			return VK_FORMAT_R16G16B16_SFLOAT;
				case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
				case GL_RGBA_INTEGER:			return VK_FORMAT_R16G16B16A16_SFLOAT;
				case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
				case GL_STENCIL_INDEX:			return VK_FORMAT_S8_UINT;
				case GL_DEPTH_COMPONENT:		return VK_FORMAT_D16_UNORM;
				case GL_DEPTH_STENCIL:			return VK_FORMAT_D16_UNORM_S8_UINT;
			}
			break;
		}

		//
		// 32 bits per component
		//
		case GL_UNSIGNED_INT:
		{
			switch ( format )
			{
				case GL_RED:					return VK_FORMAT_R32_UINT;
				case GL_RG:						return VK_FORMAT_R32G32_UINT;
				case GL_RGB:					return VK_FORMAT_R32G32B32_UINT;
				case GL_BGR:					return VK_FORMAT_UNDEFINED;
				case GL_RGBA:					return VK_FORMAT_R32G32B32A32_UINT;
				case GL_BGRA:					return VK_FORMAT_UNDEFINED;
				case GL_RED_INTEGER:			return VK_FORMAT_R32_UINT;
				case GL_RG_INTEGER:				return VK_FORMAT_R32G32_UINT;
				case GL_RGB_INTEGER:			return VK_FORMAT_R32G32B32_UINT;
				case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
				case GL_RGBA_INTEGER:			return VK_FORMAT_R32G32B32A32_UINT;
				case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
				case GL_STENCIL_INDEX:			return VK_FORMAT_S8_UINT;
				case GL_DEPTH_COMPONENT:		return VK_FORMAT_X8_D24_UNORM_PACK32;
				case GL_DEPTH_STENCIL:			return VK_FORMAT_D24_UNORM_S8_UINT;
			}
			break;
		}
		case GL_INT:
		{
			switch ( format )
			{
				case GL_RED:					return VK_FORMAT_R32_SINT;
				case GL_RG:						return VK_FORMAT_R32G32_SINT;
				case GL_RGB:					return VK_FORMAT_R32G32B32_SINT;
				case GL_BGR:					return VK_FORMAT_UNDEFINED;
				case GL_RGBA:					return VK_FORMAT_R32G32B32A32_SINT;
				case GL_BGRA:					return VK_FORMAT_UNDEFINED;
				case GL_RED_INTEGER:			return VK_FORMAT_R32_SINT;
				case GL_RG_INTEGER:				return VK_FORMAT_R32G32_SINT;
				case GL_RGB_INTEGER:			return VK_FORMAT_R32G32B32_SINT;
				case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
				case GL_RGBA_INTEGER:			return VK_FORMAT_R32G32B32A32_SINT;
				case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
				case GL_STENCIL_INDEX:			return VK_FORMAT_S8_UINT;
				case GL_DEPTH_COMPONENT:		return VK_FORMAT_X8_D24_UNORM_PACK32;
				case GL_DEPTH_STENCIL:			return VK_FORMAT_D24_UNORM_S8_UINT;
			}
			break;
		}
		case GL_FLOAT:
		{
			switch ( format )
			{
				case GL_RED:					return VK_FORMAT_R32_SFLOAT;
				case GL_RG:						return VK_FORMAT_R32G32_SFLOAT;
				case GL_RGB:					return VK_FORMAT_R32G32B32_SFLOAT;
				case GL_BGR:					return VK_FORMAT_UNDEFINED;
				case GL_RGBA:					return VK_FORMAT_R32G32B32A32_SFLOAT;
				case GL_BGRA:					return VK_FORMAT_UNDEFINED;
				case GL_RED_INTEGER:			return VK_FORMAT_R32_SFLOAT;
				case GL_RG_INTEGER:				return VK_FORMAT_R32G32_SFLOAT;
				case GL_RGB_INTEGER:			return VK_FORMAT_R32G32B32_SFLOAT;
				case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
				case GL_RGBA_INTEGER:			return VK_FORMAT_R32G32B32A32_SFLOAT;
				case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
				case GL_STENCIL_INDEX:			return VK_FORMAT_S8_UINT;
				case GL_DEPTH_COMPONENT:		return VK_FORMAT_D32_SFLOAT;
				case GL_DEPTH_STENCIL:			return VK_FORMAT_D32_SFLOAT_S8_UINT;
			}
			break;
		}

		//
		// Odd bits per component
		//
		case GL_UNSIGNED_BYTE_3_3_2:			return VK_FORMAT_UNDEFINED;
		case GL_UNSIGNED_BYTE_2_3_3_REV:		return VK_FORMAT_UNDEFINED;
		case GL_UNSIGNED_SHORT_5_6_5:			return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case GL_UNSIGNED_SHORT_5_6_5_REV:		return VK_FORMAT_B5G6R5_UNORM_PACK16;
		case GL_UNSIGNED_SHORT_4_4_4_4:			return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:		return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
		case GL_UNSIGNED_SHORT_5_5_5_1:			return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:		return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		case GL_UNSIGNED_INT_8_8_8_8:			return VK_FORMAT_R8G8B8A8_UNORM;
		case GL_UNSIGNED_INT_8_8_8_8_REV:		return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
		case GL_UNSIGNED_INT_10_10_10_2:		return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case GL_UNSIGNED_INT_2_10_10_10_REV:	return VK_FORMAT_A2B10G10R10_UNORM_PACK32;

		default:								return VK_FORMAT_UNDEFINED;
	}
}

inline VkFormat vkGetVulkanFormatFromOpenGLType( const GLenum type, const GLuint numComponents )
{
	switch ( type )
	{
		//
		// 8 bits per component
		//
		case GL_UNSIGNED_BYTE:
		{
			switch ( numComponents )
			{
				case 1:							return VK_FORMAT_R8_UNORM;
				case 2:							return VK_FORMAT_R8G8_UNORM;
				case 3:							return VK_FORMAT_R8G8B8_UNORM;
				case 4:							return VK_FORMAT_R8G8B8A8_UNORM;
			}
			break;
		}
		case GL_BYTE:
		{
			switch ( numComponents )
			{
				case 1:							return VK_FORMAT_R8_SNORM;
				case 2:							return VK_FORMAT_R8G8_SNORM;
				case 3:							return VK_FORMAT_R8G8B8_SNORM;
				case 4:							return VK_FORMAT_R8G8B8A8_SNORM;
			}
			break;
		}

		//
		// 16 bits per component
		//
		case GL_UNSIGNED_SHORT:
		{
			switch ( numComponents )
			{
				case 1:							return VK_FORMAT_R16_UNORM;
				case 2:							return VK_FORMAT_R16G16_UNORM;
				case 3:							return VK_FORMAT_R16G16B16_UNORM;
				case 4:							return VK_FORMAT_R16G16B16A16_UNORM;
			}
			break;
		}
		case GL_SHORT:
		{
			switch ( numComponents )
			{
				case 1:							return VK_FORMAT_R16_SNORM;
				case 2:							return VK_FORMAT_R16G16_SNORM;
				case 3:							return VK_FORMAT_R16G16B16_SNORM;
				case 4:							return VK_FORMAT_R16G16B16A16_SNORM;
			}
			break;
		}
		case GL_HALF_FLOAT:
		{
			switch ( numComponents )
			{
				case 1:							return VK_FORMAT_R16_SFLOAT;
				case 2:							return VK_FORMAT_R16G16_SFLOAT;
				case 3:							return VK_FORMAT_R16G16B16_SFLOAT;
				case 4:							return VK_FORMAT_R16G16B16A16_SFLOAT;
			}
			break;
		}

		//
		// 32 bits per component
		//
		case GL_UNSIGNED_INT:
		{
			switch ( numComponents )
			{
				case 1:							return VK_FORMAT_R32_UINT;
				case 2:							return VK_FORMAT_R32G32_UINT;
				case 3:							return VK_FORMAT_R32G32B32_UINT;
				case 4:							return VK_FORMAT_R32G32B32A32_UINT;
			}
			break;
		}
		case GL_INT:
		{
			switch ( numComponents )
			{
				case 1:							return VK_FORMAT_R32_SINT;
				case 2:							return VK_FORMAT_R32G32_SINT;
				case 3:							return VK_FORMAT_R32G32B32_SINT;
				case 4:							return VK_FORMAT_R32G32B32A32_SINT;
			}
			break;
		}
		case GL_FLOAT:
		{
			switch ( numComponents )
			{
				case 1:							return VK_FORMAT_R32_SFLOAT;
				case 2:							return VK_FORMAT_R32G32_SFLOAT;
				case 3:							return VK_FORMAT_R32G32B32_SFLOAT;
				case 4:							return VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			break;
		}

		//
		// Odd bits per component
		//
		case GL_UNSIGNED_BYTE_3_3_2:			return VK_FORMAT_UNDEFINED;
		case GL_UNSIGNED_BYTE_2_3_3_REV:		return VK_FORMAT_UNDEFINED;
		case GL_UNSIGNED_SHORT_5_6_5:			return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case GL_UNSIGNED_SHORT_5_6_5_REV:		return VK_FORMAT_B5G6R5_UNORM_PACK16;
		case GL_UNSIGNED_SHORT_4_4_4_4:			return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:		return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
		case GL_UNSIGNED_SHORT_5_5_5_1:			return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:		return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		case GL_UNSIGNED_INT_8_8_8_8:			return VK_FORMAT_R8G8B8A8_UNORM;
		case GL_UNSIGNED_INT_8_8_8_8_REV:		return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
		case GL_UNSIGNED_INT_10_10_10_2:		return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case GL_UNSIGNED_INT_2_10_10_10_REV:	return VK_FORMAT_A2B10G10R10_UNORM_PACK32;

		default:								return VK_FORMAT_UNDEFINED;
	}
}

//
// 8 bits per component
//

#if !defined( GL_R8 )
#define GL_R8											0x8229
#endif
#if !defined( GL_RG8 )
#define GL_RG8											0x822B
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

#if !defined( GL_SR8_EXT )
#define GL_SR8_EXT										0x8FBD
#endif
#if !defined( GL_SRG8_EXT )
#define GL_SRG8_EXT										0x8FBE
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
#define GL_R16F											0x822D
#endif
#if !defined( GL_RG16F )
#define GL_RG16F										0x822F
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
#define GL_R32F											0x822E
#endif
#if !defined( GL_RG32F )
#define GL_RG32F										0x8230
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
#define GL_RGB4											0x804F
#endif
#if !defined( GL_RGB5 )
#define GL_RGB5											0x8050
#endif
#if !defined( GL_RGB565 )
#define GL_RGB565										0x8D62	// same as GL_RGB565_OES
#endif
#if !defined( GL_RGB10 )
#define GL_RGB10										0x8052
#endif
#if !defined( GL_RGB12 )
#define GL_RGB12										0x8053
#endif
#if !defined( GL_RGBA2 )
#define GL_RGBA2										0x8055
#endif
#if !defined( GL_RGBA4 )
#define GL_RGBA4										0x8056	// same as GL_RGBA4_OES
#endif
#if !defined( GL_RGBA12 )
#define GL_RGBA12										0x805A
#endif
#if !defined( GL_RGB5_A1 )
#define GL_RGB5_A1										0x8057	// same as GL_RGB5_A1_OES
#endif
#if !defined( GL_RGB10_A2 )
#define GL_RGB10_A2										0x8059
#endif
#if !defined( GL_RGB10_A2UI )
#define GL_RGB10_A2UI									0x906F
#endif
#if !defined( GL_R11F_G11F_B10F )
#define GL_R11F_G11F_B10F								0x8C3A	// same as GL_R11F_G11F_B10F_APPLE and GL_R11F_G11F_B10F_EXT
#endif
#if !defined( GL_RGB9_E5 )
#define GL_RGB9_E5										0x8C3D	// same as GL_RGB9_E5_APPLE and GL_RGB9_E5_EXT
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
#define GL_COMPRESSED_RED_RGTC1							0x8DBB
#endif
#if !defined( GL_COMPRESSED_RG_RGTC2 )
#define GL_COMPRESSED_RG_RGTC2							0x8DBD
#endif
#if !defined( GL_COMPRESSED_SIGNED_RED_RGTC1 )
#define GL_COMPRESSED_SIGNED_RED_RGTC1					0x8DBC
#endif
#if !defined( GL_COMPRESSED_SIGNED_RG_RGTC2 )
#define GL_COMPRESSED_SIGNED_RG_RGTC2					0x8DBE
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
// Generic compression
//

#if !defined( GL_COMPRESSED_RED )
#define GL_COMPRESSED_RED								0x8225
#define GL_COMPRESSED_RG								0x8226
#define GL_COMPRESSED_RGB								0x84ED
#define GL_COMPRESSED_RGBA								0x84EE
#define GL_COMPRESSED_SRGB								0x8C48
#define GL_COMPRESSED_SRGB_ALPHA						0x8C49
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

inline VkFormat vkGetVulkanFormatFromOpenGLInternalFormat( const GLenum internalFormat )
{
	switch ( internalFormat )
	{
		//
		// 8 bits per component
		//
		case GL_R8:												return VK_FORMAT_R8_UNORM;					// 1-component; 8-bit unsigned normalized
		case GL_RG8:											return VK_FORMAT_R8G8_UNORM;				// 2-component; 8-bit unsigned normalized
		case GL_RGB8:											return VK_FORMAT_R8G8B8_UNORM;				// 3-component; 8-bit unsigned normalized
		case GL_RGBA8:											return VK_FORMAT_R8G8B8A8_UNORM;			// 4-component; 8-bit unsigned normalized

		case GL_R8_SNORM:										return VK_FORMAT_R8_SNORM;					// 1-component; 8-bit signed normalized
		case GL_RG8_SNORM:										return VK_FORMAT_R8G8_SNORM;				// 2-component; 8-bit signed normalized
		case GL_RGB8_SNORM:										return VK_FORMAT_R8G8B8_SNORM;				// 3-component; 8-bit signed normalized
		case GL_RGBA8_SNORM:									return VK_FORMAT_R8G8B8A8_SNORM;			// 4-component; 8-bit signed normalized

		case GL_R8UI:											return VK_FORMAT_R8_UINT;					// 1-component; 8-bit unsigned integer
		case GL_RG8UI:											return VK_FORMAT_R8G8_UINT;					// 2-component; 8-bit unsigned integer
		case GL_RGB8UI:											return VK_FORMAT_R8G8B8_UINT;				// 3-component; 8-bit unsigned integer
		case GL_RGBA8UI:										return VK_FORMAT_R8G8B8A8_UINT;				// 4-component; 8-bit unsigned integer

		case GL_R8I:											return VK_FORMAT_R8_SINT;					// 1-component; 8-bit signed integer
		case GL_RG8I:											return VK_FORMAT_R8G8_SINT;					// 2-component; 8-bit signed integer
		case GL_RGB8I:											return VK_FORMAT_R8G8B8_SINT;				// 3-component; 8-bit signed integer
		case GL_RGBA8I:											return VK_FORMAT_R8G8B8A8_SINT;				// 4-component; 8-bit signed integer

		case GL_SR8_EXT:										return VK_FORMAT_R8_SRGB;					// 1-component; 8-bit sRGB
		case GL_SRG8_EXT:										return VK_FORMAT_R8G8_SRGB;					// 2-component; 8-bit sRGB
		case GL_SRGB8:											return VK_FORMAT_R8G8B8_SRGB;				// 3-component; 8-bit sRGB
		case GL_SRGB8_ALPHA8:									return VK_FORMAT_R8G8B8A8_SRGB;				// 4-component; 8-bit sRGB

		//
		// 16 bits per component
		//
		case GL_R16:											return VK_FORMAT_R16_UNORM;					// 1-component, 16-bit unsigned normalized
		case GL_RG16:											return VK_FORMAT_R16G16_UNORM;				// 2-component, 16-bit unsigned normalized
		case GL_RGB16:											return VK_FORMAT_R16G16B16_UNORM;			// 3-component, 16-bit unsigned normalized
		case GL_RGBA16:											return VK_FORMAT_R16G16B16A16_UNORM;		// 4-component, 16-bit unsigned normalized

		case GL_R16_SNORM:										return VK_FORMAT_R16_SNORM;					// 1-component, 16-bit signed normalized
		case GL_RG16_SNORM:										return VK_FORMAT_R16G16_SNORM;				// 2-component, 16-bit signed normalized
		case GL_RGB16_SNORM:									return VK_FORMAT_R16G16B16_SNORM;			// 3-component, 16-bit signed normalized
		case GL_RGBA16_SNORM:									return VK_FORMAT_R16G16B16A16_SNORM;		// 4-component, 16-bit signed normalized

		case GL_R16UI:											return VK_FORMAT_R16_UINT;					// 1-component; 16-bit unsigned integer
		case GL_RG16UI:											return VK_FORMAT_R16G16_UINT;				// 2-component; 16-bit unsigned integer
		case GL_RGB16UI:										return VK_FORMAT_R16G16B16_UINT;			// 3-component; 16-bit unsigned integer
		case GL_RGBA16UI:										return VK_FORMAT_R16G16B16A16_UINT;			// 4-component; 16-bit unsigned integer

		case GL_R16I:											return VK_FORMAT_R16_SINT;					// 1-component; 16-bit signed integer
		case GL_RG16I:											return VK_FORMAT_R16G16_SINT;				// 2-component; 16-bit signed integer
		case GL_RGB16I:											return VK_FORMAT_R16G16B16_SINT;			// 3-component; 16-bit signed integer
		case GL_RGBA16I:										return VK_FORMAT_R16G16B16A16_SINT;			// 4-component; 16-bit signed integer

		case GL_R16F:											return VK_FORMAT_R16_SFLOAT;				// 1-component; 16-bit floating-point
		case GL_RG16F:											return VK_FORMAT_R16G16_SFLOAT;				// 2-component; 16-bit floating-point
		case GL_RGB16F:											return VK_FORMAT_R16G16B16_SFLOAT;			// 3-component; 16-bit floating-point
		case GL_RGBA16F:										return VK_FORMAT_R16G16B16A16_SFLOAT;		// 4-component; 16-bit floating-point

		//
		// 32 bits per component
		//
		case GL_R32UI:											return VK_FORMAT_R32_UINT;					// 1-component: 32-bit unsigned integer
		case GL_RG32UI:											return VK_FORMAT_R32G32_UINT;				// 2-component: 32-bit unsigned integer
		case GL_RGB32UI:										return VK_FORMAT_R32G32B32_UINT;			// 3-component: 32-bit unsigned integer
		case GL_RGBA32UI:										return VK_FORMAT_R32G32B32A32_UINT;			// 4-component: 32-bit unsigned integer

		case GL_R32I:											return VK_FORMAT_R32_SINT;					// 1-component: 32-bit signed integer
		case GL_RG32I:											return VK_FORMAT_R32G32_SINT;				// 2-component: 32-bit signed integer
		case GL_RGB32I:											return VK_FORMAT_R32G32B32_SINT;			// 3-component: 32-bit signed integer
		case GL_RGBA32I:										return VK_FORMAT_R32G32B32A32_SINT;			// 4-component: 32-bit signed integer

		case GL_R32F:											return VK_FORMAT_R32_SFLOAT;				// 1-component: 32-bit floating-point
		case GL_RG32F:											return VK_FORMAT_R32G32_SFLOAT;				// 2-component: 32-bit floating-point
		case GL_RGB32F:											return VK_FORMAT_R32G32B32_SFLOAT;			// 3-component: 32-bit floating-point
		case GL_RGBA32F:										return VK_FORMAT_R32G32B32A32_SFLOAT;		// 4-component: 32-bit floating-point

		//
		// Odd bits per component
		//
		case GL_R3_G3_B2:										return VK_FORMAT_UNDEFINED;					// 3-component 3:3:2,       unsigned normalized
		case GL_RGB4:											return VK_FORMAT_UNDEFINED;					// 3-component 4:4:4,       unsigned normalized
		case GL_RGB5:											return VK_FORMAT_R5G5B5A1_UNORM_PACK16;		// 3-component 5:5:5,       unsigned normalized
		case GL_RGB565:											return VK_FORMAT_R5G6B5_UNORM_PACK16;		// 3-component 5:6:5,       unsigned normalized
		case GL_RGB10:											return VK_FORMAT_A2R10G10B10_UNORM_PACK32;	// 3-component 10:10:10,    unsigned normalized
		case GL_RGB12:											return VK_FORMAT_UNDEFINED;					// 3-component 12:12:12,    unsigned normalized
		case GL_RGBA2:											return VK_FORMAT_UNDEFINED;					// 4-component 2:2:2:2,     unsigned normalized
		case GL_RGBA4:											return VK_FORMAT_R4G4B4A4_UNORM_PACK16;		// 4-component 4:4:4:4,     unsigned normalized
		case GL_RGBA12:											return VK_FORMAT_UNDEFINED;					// 4-component 12:12:12:12, unsigned normalized
		case GL_RGB5_A1:										return VK_FORMAT_A1R5G5B5_UNORM_PACK16;		// 4-component 5:5:5:1,     unsigned normalized
		case GL_RGB10_A2:										return VK_FORMAT_A2R10G10B10_UNORM_PACK32;	// 4-component 10:10:10:2,  unsigned normalized
		case GL_RGB10_A2UI:										return VK_FORMAT_A2R10G10B10_UINT_PACK32;	// 4-component 10:10:10:2,  unsigned integer
		case GL_R11F_G11F_B10F:									return VK_FORMAT_B10G11R11_UFLOAT_PACK32;	// 3-component 11:11:10,    floating-point
		case GL_RGB9_E5:										return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;	// 3-component/exp 9:9:9/5, floating-point

		//
		// S3TC/DXT/BC
		//

		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:					return VK_FORMAT_BC1_RGB_UNORM_BLOCK;		// line through 3D space, unsigned normalized
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:					return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;		// line through 3D space plus 1-bit alpha, unsigned normalized
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:					return VK_FORMAT_BC2_UNORM_BLOCK;			// line through 3D space plus line through 1D space, unsigned normalized
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:					return VK_FORMAT_BC3_UNORM_BLOCK;			// line through 3D space plus 4-bit alpha, unsigned normalized

		case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:					return VK_FORMAT_BC1_RGB_SRGB_BLOCK;		// line through 3D space, sRGB
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;		// line through 3D space plus 1-bit alpha, sRGB
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:			return VK_FORMAT_BC2_SRGB_BLOCK;			// line through 3D space plus line through 1D space, sRGB
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:			return VK_FORMAT_BC3_SRGB_BLOCK;			// line through 3D space plus 4-bit alpha, sRGB

		case GL_COMPRESSED_LUMINANCE_LATC1_EXT:					return VK_FORMAT_BC4_UNORM_BLOCK;			// line through 1D space, unsigned normalized
		case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:			return VK_FORMAT_BC5_UNORM_BLOCK;			// two lines through 1D space, unsigned normalized
		case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:			return VK_FORMAT_BC4_SNORM_BLOCK;			// line through 1D space, signed normalized
		case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:	return VK_FORMAT_BC5_SNORM_BLOCK;			// two lines through 1D space, signed normalized

		case GL_COMPRESSED_RED_RGTC1:							return VK_FORMAT_BC4_UNORM_BLOCK;			// line through 1D space, unsigned normalized
		case GL_COMPRESSED_RG_RGTC2:							return VK_FORMAT_BC5_UNORM_BLOCK;			// two lines through 1D space, unsigned normalized
		case GL_COMPRESSED_SIGNED_RED_RGTC1:					return VK_FORMAT_BC4_SNORM_BLOCK;			// line through 1D space, signed normalized
		case GL_COMPRESSED_SIGNED_RG_RGTC2:						return VK_FORMAT_BC5_SNORM_BLOCK;			// two lines through 1D space, signed normalized

		case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:				return VK_FORMAT_BC6H_UFLOAT_BLOCK;			// 3-component, unsigned floating-point
		case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:				return VK_FORMAT_BC6H_SFLOAT_BLOCK;			// 3-component, signed floating-point
		case GL_COMPRESSED_RGBA_BPTC_UNORM:						return VK_FORMAT_BC7_UNORM_BLOCK;			// 4-component, unsigned normalized
		case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:				return VK_FORMAT_BC7_SRGB_BLOCK;			// 4-component, sRGB

		//
		// ETC
		//
		case GL_ETC1_RGB8_OES:									return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;	// 3-component ETC1, unsigned normalized" ),

		case GL_COMPRESSED_RGB8_ETC2:							return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;	// 3-component ETC2, unsigned normalized
		case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:		return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;	// 4-component with 1-bit alpha ETC2, unsigned normalized
		case GL_COMPRESSED_RGBA8_ETC2_EAC:						return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;	// 4-component ETC2, unsigned normalized

		case GL_COMPRESSED_SRGB8_ETC2:							return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;	// 3-component ETC2, sRGB
		case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:		return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;	// 4-component with 1-bit alpha ETC2, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:				return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;	// 4-component ETC2, sRGB

		case GL_COMPRESSED_R11_EAC:								return VK_FORMAT_EAC_R11_UNORM_BLOCK;		// 1-component ETC, unsigned normalized
		case GL_COMPRESSED_RG11_EAC:							return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;	// 2-component ETC, unsigned normalized
		case GL_COMPRESSED_SIGNED_R11_EAC:						return VK_FORMAT_EAC_R11_SNORM_BLOCK;		// 1-component ETC, signed normalized
		case GL_COMPRESSED_SIGNED_RG11_EAC:						return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;	// 2-component ETC, signed normalized

		//
		// ASTC
		//
		case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:					return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;		// 4-component ASTC, 4x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:					return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;		// 4-component ASTC, 5x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:					return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;		// 4-component ASTC, 5x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:					return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;		// 4-component ASTC, 6x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:					return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;		// 4-component ASTC, 6x6 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:					return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;		// 4-component ASTC, 8x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:					return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;		// 4-component ASTC, 8x6 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:					return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;		// 4-component ASTC, 8x8 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:					return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;		// 4-component ASTC, 10x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:					return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;		// 4-component ASTC, 10x6 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:					return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;		// 4-component ASTC, 10x8 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:					return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;	// 4-component ASTC, 10x10 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:					return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;	// 4-component ASTC, 12x10 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:					return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;	// 4-component ASTC, 12x12 blocks, unsigned normalized

		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:			return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;		// 4-component ASTC, 4x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:			return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;		// 4-component ASTC, 5x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:			return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;		// 4-component ASTC, 5x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:			return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;		// 4-component ASTC, 6x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:			return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;		// 4-component ASTC, 6x6 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:			return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;		// 4-component ASTC, 8x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:			return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;		// 4-component ASTC, 8x6 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:			return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;		// 4-component ASTC, 8x8 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:			return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;		// 4-component ASTC, 10x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:			return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;		// 4-component ASTC, 10x6 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:			return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;		// 4-component ASTC, 10x8 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:			return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;		// 4-component ASTC, 10x10 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:			return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;		// 4-component ASTC, 12x10 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:			return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;		// 4-component ASTC, 12x12 blocks, sRGB

		case GL_COMPRESSED_RGBA_ASTC_3x3x3_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 3x3x3 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_4x3x3_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 4x3x3 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_4x4x3_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 4x4x3 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_4x4x4_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 4x4x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x4x4_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 5x4x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x5x4_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 5x5x4 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_5x5x5_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 5x5x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x5x5_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 6x5x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x6x5_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 6x6x5 blocks, unsigned normalized
		case GL_COMPRESSED_RGBA_ASTC_6x6x6_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 6x6x6 blocks, unsigned normalized

		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 3x3x3 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 4x3x3 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 4x4x3 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 4x4x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 5x4x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 5x5x4 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 5x5x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 6x5x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 6x6x5 blocks, sRGB
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 6x6x6 blocks, sRGB

		//
		// Generic compression
		//
		case GL_COMPRESSED_RED:									return VK_FORMAT_UNDEFINED;					// 1-component, generic, unsigned normalized
		case GL_COMPRESSED_RG:									return VK_FORMAT_UNDEFINED;					// 2-component, generic, unsigned normalized
		case GL_COMPRESSED_RGB:									return VK_FORMAT_UNDEFINED;					// 3-component, generic, unsigned normalized
		case GL_COMPRESSED_RGBA:								return VK_FORMAT_UNDEFINED;					// 4-component, generic, unsigned normalized
		case GL_COMPRESSED_SRGB:								return VK_FORMAT_UNDEFINED;					// 3-component, generic, sRGB
		case GL_COMPRESSED_SRGB_ALPHA:							return VK_FORMAT_UNDEFINED;					// 4-component, generic, sRGB

		//
		// ATC
		//
		case GL_ATC_RGB_AMD:									return VK_FORMAT_UNDEFINED;					// 3-component, unsigned normalized
		case GL_ATC_RGBA_EXPLICIT_ALPHA_AMD:					return VK_FORMAT_UNDEFINED;					// 4-component, unsigned normalized
		case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD:				return VK_FORMAT_UNDEFINED;					// 4-component, unsigned normalized

		//
		// Palletized
		//
		case GL_PALETTE4_RGB8_OES:								return VK_FORMAT_UNDEFINED;					// 3-component 8:8:8,   4-bit palette, unsigned normalized
		case GL_PALETTE4_RGBA8_OES:								return VK_FORMAT_UNDEFINED;					// 4-component 8:8:8:8, 4-bit palette, unsigned normalized
		case GL_PALETTE4_R5_G6_B5_OES:							return VK_FORMAT_UNDEFINED;					// 3-component 5:6:5,   4-bit palette, unsigned normalized
		case GL_PALETTE4_RGBA4_OES:								return VK_FORMAT_UNDEFINED;					// 4-component 4:4:4:4, 4-bit palette, unsigned normalized
		case GL_PALETTE4_RGB5_A1_OES:							return VK_FORMAT_UNDEFINED;					// 4-component 5:5:5:1, 4-bit palette, unsigned normalized
		case GL_PALETTE8_RGB8_OES:								return VK_FORMAT_UNDEFINED;					// 3-component 8:8:8,   8-bit palette, unsigned normalized
		case GL_PALETTE8_RGBA8_OES:								return VK_FORMAT_UNDEFINED;					// 4-component 8:8:8:8, 8-bit palette, unsigned normalized
		case GL_PALETTE8_R5_G6_B5_OES:							return VK_FORMAT_UNDEFINED;					// 3-component 5:6:5,   8-bit palette, unsigned normalized
		case GL_PALETTE8_RGBA4_OES:								return VK_FORMAT_UNDEFINED;					// 4-component 4:4:4:4, 8-bit palette, unsigned normalized
		case GL_PALETTE8_RGB5_A1_OES:							return VK_FORMAT_UNDEFINED;					// 4-component 5:5:5:1, 8-bit palette, unsigned normalized

		default:												return VK_FORMAT_UNDEFINED;
	}
}

typedef enum VkFormatSizeFlagBits {
	VK_FORMAT_SIZE_PACKED_BIT = 0x00000001,
	VK_FORMAT_SIZE_COMPRESSED_BIT = 0x00000002,
} VkFormatSizeFlagBits;

typedef VkFlags VkFormatSizeFlags;

typedef struct VkFormatSize {
	VkFormatSizeFlags	flags;
	uint32_t			blockSize;
	uint32_t			blockWidth;
	uint32_t			blockHeight;
	uint32_t			blockDepth;
} VkFormatSize;

inline void vkGetVulkanFormatSize( const VkFormat format, VkFormatSize * pFormatSize )
{
	memset( pFormatSize, 0, sizeof( VkFormatSize ) );

	switch ( format )
	{
		case VK_FORMAT_R4G4_UNORM_PACK8:
			pFormatSize->flags = VK_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->blockSize = 1;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		case VK_FORMAT_B5G6R5_UNORM_PACK16:
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
			pFormatSize->flags = VK_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->blockSize = 2;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_SRGB:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 1;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8_USCALED:
		case VK_FORMAT_R8G8_SSCALED:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_SRGB:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 2;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_R8G8B8_USCALED:
		case VK_FORMAT_R8G8B8_SSCALED:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_B8G8R8_SRGB:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 3;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 4;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_UINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
			pFormatSize->flags = VK_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->blockSize = 4;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
			pFormatSize->flags = VK_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->blockSize = 4;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16_USCALED:
		case VK_FORMAT_R16_SSCALED:
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 2;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16_USCALED:
		case VK_FORMAT_R16G16_SSCALED:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 4;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R16G16B16_UNORM:
		case VK_FORMAT_R16G16B16_SNORM:
		case VK_FORMAT_R16G16B16_USCALED:
		case VK_FORMAT_R16G16B16_SSCALED:
		case VK_FORMAT_R16G16B16_UINT:
		case VK_FORMAT_R16G16B16_SINT:
		case VK_FORMAT_R16G16B16_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 6;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
		case VK_FORMAT_R16G16B16A16_USCALED:
		case VK_FORMAT_R16G16B16A16_SSCALED:
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R16G16B16A16_SINT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 4;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32_SINT:
		case VK_FORMAT_R32G32B32_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 12;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R32G32B32A32_UINT:
		case VK_FORMAT_R32G32B32A32_SINT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R64_UINT:
		case VK_FORMAT_R64_SINT:
		case VK_FORMAT_R64_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 8;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R64G64_UINT:
		case VK_FORMAT_R64G64_SINT:
		case VK_FORMAT_R64G64_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R64G64B64_UINT:
		case VK_FORMAT_R64G64B64_SINT:
		case VK_FORMAT_R64G64B64_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 24;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_R64G64B64A64_UINT:
		case VK_FORMAT_R64G64B64A64_SINT:
		case VK_FORMAT_R64G64B64A64_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 32;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			pFormatSize->flags = VK_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->blockSize = 4;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_D16_UNORM:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 2;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_X8_D24_UNORM_PACK32:
			pFormatSize->flags = VK_FORMAT_SIZE_PACKED_BIT;
			pFormatSize->blockSize = 4;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_D32_SFLOAT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 4;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_S8_UINT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 1;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_D16_UNORM_S8_UINT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 3;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_D24_UNORM_S8_UINT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 4;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			pFormatSize->flags = 0;
			pFormatSize->blockSize = 5;
			pFormatSize->blockWidth = 1;
			pFormatSize->blockHeight = 1;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 8;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_BC2_UNORM_BLOCK:
		case VK_FORMAT_BC2_SRGB_BLOCK:
		case VK_FORMAT_BC3_UNORM_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
		case VK_FORMAT_BC4_UNORM_BLOCK:
		case VK_FORMAT_BC4_SNORM_BLOCK:
		case VK_FORMAT_BC5_UNORM_BLOCK:
		case VK_FORMAT_BC5_SNORM_BLOCK:
		case VK_FORMAT_BC6H_UFLOAT_BLOCK:
		case VK_FORMAT_BC6H_SFLOAT_BLOCK:
		case VK_FORMAT_BC7_UNORM_BLOCK:
		case VK_FORMAT_BC7_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 8;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
		case VK_FORMAT_EAC_R11_UNORM_BLOCK:
		case VK_FORMAT_EAC_R11_SNORM_BLOCK:
		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
		case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
		case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 4;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
		case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 5;
			pFormatSize->blockHeight = 4;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 5;
			pFormatSize->blockHeight = 5;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 6;
			pFormatSize->blockHeight = 5;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 6;
			pFormatSize->blockHeight = 6;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 8;
			pFormatSize->blockHeight = 5;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 8;
			pFormatSize->blockHeight = 6;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 8;
			pFormatSize->blockHeight = 8;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 10;
			pFormatSize->blockHeight = 5;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 10;
			pFormatSize->blockHeight = 6;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: 
		case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 10;
			pFormatSize->blockHeight = 8;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 10;
			pFormatSize->blockHeight = 10;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
		case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 12;
			pFormatSize->blockHeight = 10;
			pFormatSize->blockDepth = 1;
			break;
		case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
		case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
			pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
			pFormatSize->blockSize = 16;
			pFormatSize->blockWidth = 12;
			pFormatSize->blockHeight = 12;
			pFormatSize->blockDepth = 1;
			break;
	}
}

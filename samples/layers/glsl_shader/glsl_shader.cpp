/*
================================================================================================

Description	:	This layer can be used during development to load GLSL shaders directly.
Author		:	J.M.P. van Waveren
Date		:	06/24/2016
Language	:	C++
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

This layer can be used during development to load GLSL shaders directly
without first converting them to SPIR-V.


REQUIREMENTS
============

A collocated glslang repository (from https://github.com/KhronosGroup/glslang),
and either the Vulkan SDK (from https://lunarg.com/vulkan-sdk/) or a collocated
Vulkan-LoaderAndValidationLayers repository (from https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers)
are needed. The paths for the collocated repositories will look as follows:

	<path>/Vulkan-Samples/
	<path>/Vulkan-LoaderAndValidationLayers/
	<path>/glslang/

On Windows make sure that the <path> is no more than one folder deep to
avoid running into maximum path depth compilation issues.


VERSION HISTORY
===============

1.0		Initial version.

================================================================================================
*/

#if defined( WIN32 ) || defined( _WIN32 ) || defined( WIN64 ) || defined( _WIN64 )
	#if !defined( _CRT_SECURE_NO_WARNINGS )
		#define _CRT_SECURE_NO_WARNINGS
	#endif
#endif

#ifdef _MSC_VER
#pragma warning( disable : 4100 )	// unreferenced formal parameter
#pragma warning( disable : 4191 )	// 'type cast' : unsafe conversion from 'PFN_vkVoidFunction' to 'PFN_vkCmdCopyImage'
#pragma warning( disable : 4255 )	// 'FARPROC' : no function prototype given: converting '()' to '(void)'
#pragma warning( disable : 4350 )	// behavior change: 'member1' called instead of 'member2'
#pragma warning( disable : 4505 )	// unreferenced local function has been removed
#pragma warning( disable : 4668 )	// 'NTDDI_WIN7SP1' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning( disable : 4710 )	// function not inlined
#pragma warning( disable : 4711 )	// function '<name>' selected for automatic inline expansion
#pragma warning( disable : 4820 )	// '<name>' : 'X' bytes padding added after data member '<member>'

// glslang
#pragma warning( disable : 4061 )	// enumerator 'EvqLast' in switch of enum 'glslang::TStorageQualifier' is not explicitly handled by a case label
#pragma warning( disable : 4365 )	// 'argument' : conversion from '__int64' to 'size_t', signed/unsigned mismatch
#pragma warning( disable : 4625 )	// 'glslang::TIntermTyped' : copy constructor could not be generated because a base class copy constructor is inaccessible or deleted
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <vulkan/vulkan.h>				// {SDK}/Include/       | Vulkan-LoaderAndValidationLayers/include/      | /usr/include/
#include <vulkan/vk_layer.h>			// {SDK}/Include/       | Vulkan-LoaderAndValidationLayers/include/      | /usr/include/
#include <vk_dispatch_table_helper.h>	// {SDK}/Source/layers/ | Vulkan-LoaderAndValidationLayers/build/layers/

#include "SPIRV/GlslangToSpv.h"

#if defined( __ANDROID__ )
#include <android/log.h>			// for __android_log_print()
#define Print( ... )				__android_log_print( ANDROID_LOG_INFO, "qm", __VA_ARGS__ )
#else
#define Print( ... )				printf( __VA_ARGS__ )
#endif

#define LAYER_NAME					"VK_LAYER_OCULUS_glsl_shader"

/*
================================================================================================================================

HashMap_t

static void HashMap_Create( HashMap_t * map );
static void HashMap_Destroy( HashMap_t * map );
static void HashMap_Add( HashMap_t * map, void * key, void * data );
static void HashMap_Remove( HashMap_t * map, void * key );
static void * HashMap_Find( HashMap_t * map, void * key );

================================================================================================================================
*/

// This layer does not track many different objects, typically 1 instance, 1 device and up to 16 queues.
#define HASH_TABLE_SIZE		256

typedef struct
{
	int				table[HASH_TABLE_SIZE];
	void **			key;
	void **			data;
	int *			next;
	int *			empty;
	int				count;
	int				nextEmpty;
} HashMap_t;

static void HashMap_Grow( HashMap_t * map, const int newCount )
{
	assert( newCount > map->count );

	void ** key = (void **) malloc( newCount * sizeof( map->key[0] ) );
	void ** data = (void **) malloc( newCount * sizeof( map->data[0] ) );
	int * next = (int *) malloc( newCount * sizeof( map->next[0] ) );
	int * empty = (int *) malloc( newCount * sizeof( map->empty[0] ) );

	memcpy( key, map->key, map->count * sizeof( key[0] ) );
	memcpy( data, map->data, map->count * sizeof( data[0] ) );
	memcpy( next, map->next, map->count * sizeof( next[0] ) );
	memcpy( empty, map->empty, map->count * sizeof( empty[0] ) );

	memset( key + map->count, 0, ( newCount - map->count ) * sizeof( key[0] ) );
	memset( data + map->count, 0, ( newCount - map->count ) * sizeof( data[0] ) );
	memset( next + map->count, -1, ( newCount - map->count ) * sizeof( next[0] ) );
	for ( int i = map->count; i < newCount; i++ ) { empty[i] = i; }

	free( map->key );
	free( map->data );
	free( map->next );
	free( map->empty );

	map->key = key;
	map->data = data;
	map->next = next;
	map->empty = empty;
	map->count = newCount;
}

static void HashMap_Create( HashMap_t * map )
{
	memset( map, 0, sizeof( HashMap_t ) );
	memset( map->table, -1, sizeof( map->table ) );
	HashMap_Grow( map, 16 );
}

static void HashMap_Destroy( HashMap_t * map )
{
	free( map->key );
	free( map->data );
	free( map->next );
	free( map->empty );
	memset( map, 0, sizeof( HashMap_t ) );
}

static unsigned int HashMap_HashPointer( HashMap_t * map, const void * key )
{
	// HASH_TABLE_SIZE must be a power of two.
	assert( ( HASH_TABLE_SIZE > 0 ) && ( HASH_TABLE_SIZE & ( HASH_TABLE_SIZE - 1 ) ) == 0 );
	const size_t value = (size_t)key;
	return (unsigned int)( ( ( value >> 3 ) ^ ( value >> 11 ) ^ ( value >> 19 ) ) & ( HASH_TABLE_SIZE - 1 ) );
}

static void HashMap_Add( HashMap_t * map, void * key, void * data )
{
	const unsigned int hash = HashMap_HashPointer( map, key );
	int * index = NULL;
	for ( index = &map->table[hash]; *index >= 0; index = &map->next[*index] )
	{
		if ( map->key[*index] == key )
		{
			assert( 0 );
			return;
		}
	}
	if ( map->nextEmpty >= map->count )
	{
		HashMap_Grow( map, map->count * 2 );
	}
	*index = map->empty[map->nextEmpty++];
	map->key[*index] = key;
	map->data[*index] = data;
	map->next[*index] = -1;
}

static void HashMap_Remove( HashMap_t * map, void * key )
{
	const unsigned int hash = HashMap_HashPointer( map, key );
	for ( int * index = &map->table[hash]; *index >= 0; index = &map->next[*index] )
	{
		if ( map->key[*index] == key )
		{
			map->key[*index] = NULL;
			map->data[*index] = NULL;
			map->next[*index] = -1;
			map->empty[--map->nextEmpty] = *index;
			*index = map->next[*index];
			return;
		}
	}
	assert( 0 );
}

static void * HashMap_Find( HashMap_t * map, void * key )
{
	const unsigned int hash = HashMap_HashPointer( map, key );
	for ( int index = map->table[hash]; index >= 0; index = map->next[index] )
	{
		if ( map->key[index] == key )
		{
			return map->data[index];
		}
	}
	return NULL;
}

/*
================================================================================================================================

Instance/Device/Queue data maps

================================================================================================================================
*/

typedef struct
{
	VkLayerInstanceDispatchTable *	instanceDispatchTable;
} InstanceData_t;

typedef struct
{
	InstanceData_t *				instanceData;
	VkLayerDispatchTable *			deviceDispatchTable;
} DeviceData_t;

static HashMap_t instance_data_map;
static HashMap_t device_data_map;

static VkLayerDispatchTable * GetDispatchTable( const void * object ) { return *(VkLayerDispatchTable **)object; }

/*
================================================================================================================================

Instance

================================================================================================================================
*/

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
		const VkInstanceCreateInfo *	pCreateInfo,
		const VkAllocationCallbacks *	pAllocator,
		VkInstance *					pInstance )
{
	VkLayerInstanceCreateInfo * chain_info = (VkLayerInstanceCreateInfo *)pCreateInfo->pNext;
	while ( chain_info != NULL &&
			!( chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO &&
				chain_info->function == VK_LAYER_LINK_INFO ) )
	{
		chain_info = (VkLayerInstanceCreateInfo *)chain_info->pNext;
	}
	assert( chain_info != NULL );
	assert( chain_info->u.pLayerInfo );

	PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
	PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance) pfnGetInstanceProcAddr( NULL, "vkCreateInstance" );
	if ( fpCreateInstance == NULL )
	{
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	// Advance the link info for the next element on the chain
	chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

	VkResult result = fpCreateInstance( pCreateInfo, pAllocator, pInstance );
	if ( result != VK_SUCCESS )
	{
		return result;
	}

	if ( instance_data_map.nextEmpty == 0 )
	{
		HashMap_Create( &instance_data_map );
		HashMap_Create( &device_data_map );
	}

	InstanceData_t * my_instance_data = (InstanceData_t *)malloc( sizeof( InstanceData_t ) );
	my_instance_data->instanceDispatchTable = (VkLayerInstanceDispatchTable *)malloc( sizeof( VkLayerInstanceDispatchTable ) );
	layer_init_instance_dispatch_table( *pInstance, my_instance_data->instanceDispatchTable, pfnGetInstanceProcAddr );

	HashMap_Add( &instance_data_map, GetDispatchTable( *pInstance ), my_instance_data );
		
	return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
		VkInstance						instance,
		const VkAllocationCallbacks *	pAllocator)
{
	void * key = GetDispatchTable( instance );
	InstanceData_t * my_instance_data = (InstanceData_t *)HashMap_Find( &instance_data_map, key );

	my_instance_data->instanceDispatchTable->DestroyInstance( instance, pAllocator );

	HashMap_Remove( &instance_data_map, key );

	free( my_instance_data->instanceDispatchTable );
	free( my_instance_data );

	if ( instance_data_map.nextEmpty == 0 )
	{
		HashMap_Destroy( &instance_data_map );
		HashMap_Destroy( &device_data_map );
	}
}

/*
================================================================================================================================

Device

================================================================================================================================
*/

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
		VkPhysicalDevice				physicalDevice,
		const VkDeviceCreateInfo *		pCreateInfo,
		const VkAllocationCallbacks *	pAllocator,
		VkDevice *						pDevice )
{
	VkLayerDeviceCreateInfo * chain_info = (VkLayerDeviceCreateInfo *)pCreateInfo->pNext;
	while ( chain_info != NULL &&
			!( chain_info->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO &&
				chain_info->function == VK_LAYER_LINK_INFO ) )
	{
		chain_info = (VkLayerDeviceCreateInfo *)chain_info->pNext;
	}
	assert( chain_info != NULL );
	assert( chain_info->u.pLayerInfo );

	PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
	PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
	PFN_vkCreateDevice pfnCreateDevice = (PFN_vkCreateDevice) pfnGetInstanceProcAddr( NULL, "vkCreateDevice" );
	if ( pfnCreateDevice == NULL )
	{
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	// Advance the link info for the next element on the chain
	chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

	VkResult result = pfnCreateDevice( physicalDevice, pCreateInfo, pAllocator, pDevice );
	if ( result != VK_SUCCESS )
	{
		return result;
	}

	// Setup device data.
	DeviceData_t * my_device_data = (DeviceData_t *)malloc( sizeof( DeviceData_t ) );
	my_device_data->instanceData = (InstanceData_t *)HashMap_Find( &instance_data_map, GetDispatchTable( physicalDevice ) );
	my_device_data->deviceDispatchTable = (VkLayerDispatchTable *)malloc( sizeof( VkLayerDispatchTable ) );
	layer_init_device_dispatch_table( *pDevice, my_device_data->deviceDispatchTable, pfnGetDeviceProcAddr );

	HashMap_Add( &device_data_map, GetDispatchTable( *pDevice ), my_device_data );

	return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
		VkDevice						device,
		const VkAllocationCallbacks *	pAllocator )
{
	void * key = GetDispatchTable( device );
	DeviceData_t * my_device_data = (DeviceData_t *)HashMap_Find( &device_data_map, key );

	my_device_data->deviceDispatchTable->DestroyDevice( device, pAllocator );

	HashMap_Remove( &device_data_map, key );

	free( my_device_data->deviceDispatchTable );
	free( my_device_data );
}

/*
================================================================================================================================

Shader Module

================================================================================================================================
*/

bool CompileSPIRV(
		const char *				shaderSource,
		const size_t				shaderLength,
		const VkShaderStageFlagBits	stage,
		std::vector<unsigned int> &	spirv )
{
	// Enable SPIR-V and Vulkan rules when parsing GLSL
	EShMessages messages = (EShMessages)( EShMsgSpvRules | EShMsgVulkanRules );
	EShLanguage language = EShLangVertex;
	switch ( stage )
	{
		case VK_SHADER_STAGE_VERTEX_BIT:					language = EShLangVertex; break;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:		language = EShLangTessControl; break;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:	language = EShLangTessEvaluation; break;
		case VK_SHADER_STAGE_GEOMETRY_BIT:					language = EShLangGeometry; break;
		case VK_SHADER_STAGE_FRAGMENT_BIT:					language = EShLangFragment; break;
		case VK_SHADER_STAGE_COMPUTE_BIT:					language = EShLangCompute; break;
		default:											language = EShLangVertex; break;
	}

	TBuiltInResource resources;
	resources.maxLights = 32;
	resources.maxClipPlanes = 6;
	resources.maxTextureUnits = 32;
	resources.maxTextureCoords = 32;
	resources.maxVertexAttribs = 64;
	resources.maxVertexUniformComponents = 4096;
	resources.maxVaryingFloats = 64;
	resources.maxVertexTextureImageUnits = 32;
	resources.maxCombinedTextureImageUnits = 80;
	resources.maxTextureImageUnits = 32;
	resources.maxFragmentUniformComponents = 4096;
	resources.maxDrawBuffers = 32;
	resources.maxVertexUniformVectors = 128;
	resources.maxVaryingVectors = 8;
	resources.maxFragmentUniformVectors = 16;
	resources.maxVertexOutputVectors = 16;
	resources.maxFragmentInputVectors = 15;
	resources.minProgramTexelOffset = -8;
	resources.maxProgramTexelOffset = 7;
	resources.maxClipDistances = 8;
	resources.maxComputeWorkGroupCountX = 65535;
	resources.maxComputeWorkGroupCountY = 65535;
	resources.maxComputeWorkGroupCountZ = 65535;
	resources.maxComputeWorkGroupSizeX = 1024;
	resources.maxComputeWorkGroupSizeY = 1024;
	resources.maxComputeWorkGroupSizeZ = 64;
	resources.maxComputeUniformComponents = 1024;
	resources.maxComputeTextureImageUnits = 16;
	resources.maxComputeImageUniforms = 8;
	resources.maxComputeAtomicCounters = 8;
	resources.maxComputeAtomicCounterBuffers = 1;
	resources.maxVaryingComponents = 60;
	resources.maxVertexOutputComponents = 64;
	resources.maxGeometryInputComponents = 64;
	resources.maxGeometryOutputComponents = 128;
	resources.maxFragmentInputComponents = 128;
	resources.maxImageUnits = 8;
	resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
	resources.maxCombinedShaderOutputResources = 8;
	resources.maxImageSamples = 0;
	resources.maxVertexImageUniforms = 0;
	resources.maxTessControlImageUniforms = 0;
	resources.maxTessEvaluationImageUniforms = 0;
	resources.maxGeometryImageUniforms = 0;
	resources.maxFragmentImageUniforms = 8;
	resources.maxCombinedImageUniforms = 8;
	resources.maxGeometryTextureImageUnits = 16;
	resources.maxGeometryOutputVertices = 256;
	resources.maxGeometryTotalOutputComponents = 1024;
	resources.maxGeometryUniformComponents = 1024;
	resources.maxGeometryVaryingComponents = 64;
	resources.maxTessControlInputComponents = 128;
	resources.maxTessControlOutputComponents = 128;
	resources.maxTessControlTextureImageUnits = 16;
	resources.maxTessControlUniformComponents = 1024;
	resources.maxTessControlTotalOutputComponents = 4096;
	resources.maxTessEvaluationInputComponents = 128;
	resources.maxTessEvaluationOutputComponents = 128;
	resources.maxTessEvaluationTextureImageUnits = 16;
	resources.maxTessEvaluationUniformComponents = 1024;
	resources.maxTessPatchComponents = 120;
	resources.maxPatchVertices = 32;
	resources.maxTessGenLevel = 64;
	resources.maxViewports = 16;
	resources.maxVertexAtomicCounters = 0;
	resources.maxTessControlAtomicCounters = 0;
	resources.maxTessEvaluationAtomicCounters = 0;
	resources.maxGeometryAtomicCounters = 0;
	resources.maxFragmentAtomicCounters = 8;
	resources.maxCombinedAtomicCounters = 8;
	resources.maxAtomicCounterBindings = 1;
	resources.maxVertexAtomicCounterBuffers = 0;
	resources.maxTessControlAtomicCounterBuffers = 0;
	resources.maxTessEvaluationAtomicCounterBuffers = 0;
	resources.maxGeometryAtomicCounterBuffers = 0;
	resources.maxFragmentAtomicCounterBuffers = 1;
	resources.maxCombinedAtomicCounterBuffers = 1;
	resources.maxAtomicCounterBufferSize = 16384;
	resources.maxTransformFeedbackBuffers = 4;
	resources.maxTransformFeedbackInterleavedComponents = 64;
	resources.maxCullDistances = 8;
	resources.maxCombinedClipAndCullDistances = 8;
	resources.maxSamples = 4;
	resources.limits.nonInductiveForLoops = 1;
	resources.limits.whileLoops = 1;
	resources.limits.doWhileLoops = 1;
	resources.limits.generalUniformIndexing = 1;
	resources.limits.generalAttributeMatrixVectorIndexing = 1;
	resources.limits.generalVaryingIndexing = 1;
	resources.limits.generalSamplerIndexing = 1;
	resources.limits.generalVariableIndexing = 1;
	resources.limits.generalConstantMatrixVectorIndexing = 1;

	glslang::InitializeProcess();

	glslang::TShader * shader = new glslang::TShader( language );
	const char * shaderStrings[1] = { shaderSource };
	shader->setStrings( shaderStrings, 1 );

	if ( !shader->parse( &resources, 100, false, messages ) )
	{
		puts( shader->getInfoLog() );
		puts( shader->getInfoDebugLog() );
		glslang::FinalizeProcess();
		return false;
	}

	glslang::TProgram * program = new glslang::TProgram;
	program->addShader( shader );

	if ( !program->link( messages ) )
	{
		puts( shader->getInfoLog() );
		puts( shader->getInfoDebugLog() );
		glslang::FinalizeProcess();
		return false;
	}

	glslang::GlslangToSpv( *program->getIntermediate( language ), spirv );

	glslang::FinalizeProcess();

	return true;
}

#define ICD_SPV_MAGIC		0x07230203

VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(
		VkDevice							device,
		const VkShaderModuleCreateInfo *	pCreateInfo,
		const VkAllocationCallbacks *		pAllocator,
		VkShaderModule *					pShaderModule )
{
	DeviceData_t * my_device_data = (DeviceData_t *)HashMap_Find( &device_data_map, GetDispatchTable( device ) );

	const unsigned int * header = (unsigned int *)pCreateInfo->pCode;
	if ( header[0] == ICD_SPV_MAGIC && header[1] == 0 )
	{
		const VkShaderStageFlagBits stage = (VkShaderStageFlagBits) header[2];
		const char * shaderSource = (const char *)&header[3];
		const size_t shaderLength = pCreateInfo->codeSize - 3 * sizeof( unsigned int );

		std::vector<unsigned int> spirv;
		if ( !CompileSPIRV( shaderSource, shaderLength, stage, spirv ) )
		{
			return VK_ERROR_INVALID_SHADER_NV;
		}

		VkShaderModuleCreateInfo moduleCreateInfo;
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pNext = NULL;
		moduleCreateInfo.flags = 0;
		moduleCreateInfo.codeSize = spirv.size() * sizeof( spirv[0] );
		moduleCreateInfo.pCode = spirv.data();

		return my_device_data->deviceDispatchTable->CreateShaderModule( device, &moduleCreateInfo, pAllocator, pShaderModule );
	}

	return my_device_data->deviceDispatchTable->CreateShaderModule( device, pCreateInfo, pAllocator, pShaderModule );
}

/*
================================================================================================================================

Hookup

For a layer to be recognized by the Android loader the layer .so must export:

	vkEnumerateInstanceLayerProperties
	vkEnumerateInstanceExtensionProperties
	vkEnumerateDeviceLayerProperties
	vkEnumerateDeviceExtensionProperties
	vkGetInstanceProcAddr
	vkGetDeviceProcAddr

================================================================================================================================
*/

#define ARRAY_SIZE( a )		( sizeof( (a) ) / sizeof( (a)[0] ) )
#define ADD_HOOK( fn )		if ( strcmp( #fn, funcName ) == 0 ) \
							{ \
								return (PFN_vkVoidFunction) fn; \
							}

static VkResult GetLayerProperties(
		const uint32_t				count,
		const VkLayerProperties *	layer_properties,
		uint32_t *					pCount,
		VkLayerProperties *			pProperties )
{
	if ( pProperties == NULL || layer_properties == NULL )
	{
		*pCount = count;
		return VK_SUCCESS;
	}

	uint32_t copy_size = ( *pCount < count ) ? *pCount : count;
	memcpy( pProperties, layer_properties, copy_size * sizeof( VkLayerProperties ) );
	*pCount = copy_size;

	return ( copy_size < count ) ? VK_INCOMPLETE : VK_SUCCESS;
}

static VkResult GetExtensionProperties(
		const uint32_t					count,
		const VkExtensionProperties *	layer_extensions,
		uint32_t *						pCount,
		VkExtensionProperties *			pProperties )
{
	if ( pProperties == NULL || layer_extensions == NULL )
	{
		*pCount = count;
		return VK_SUCCESS;
	}

	uint32_t copy_size = *pCount < count ? *pCount : count;
	memcpy( pProperties, layer_extensions, copy_size * sizeof( VkExtensionProperties ) );
	*pCount = copy_size;

	return ( copy_size < count ) ? VK_INCOMPLETE : VK_SUCCESS;
}

static const VkLayerProperties instanceLayerProps[] =
{
	{
		LAYER_NAME,
		VK_MAKE_VERSION( 1, 0, 0 ),
		1,
		"Oculus GLSL Shader",
	}
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
		uint32_t *			pCount,
		VkLayerProperties *	pProperties )
{
	return GetLayerProperties( ARRAY_SIZE( instanceLayerProps ), instanceLayerProps, pCount, pProperties );
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
		const char *			pLayerName,
		uint32_t *				pCount,
		VkExtensionProperties *	pProperties)
{
	if ( pLayerName != NULL && strcmp( pLayerName, LAYER_NAME ) == 0 )
	{
		// This layer does not implement any instance extensions.
		return GetExtensionProperties( 0, NULL, pCount, pProperties );
	}
	return VK_ERROR_LAYER_NOT_PRESENT;
}

static const VkLayerProperties deviceLayerProps[] =
{
	{
		LAYER_NAME,
		VK_MAKE_VERSION( 1, 0, 0 ),
		1,
		"Oculus GLSL Shader",
	}
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
			VkPhysicalDevice	physicalDevice,
			uint32_t *			pCount,
			VkLayerProperties *	pProperties )
{
	return GetLayerProperties( ARRAY_SIZE( deviceLayerProps ), deviceLayerProps, pCount, pProperties );
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
		VkPhysicalDevice		physicalDevice,
		const char *			pLayerName,
		uint32_t *				pCount,
		VkExtensionProperties *	pProperties )
{
	if ( pLayerName != NULL && strcmp( pLayerName, LAYER_NAME ) == 0 )
	{
		// This layer does not implement any device extensions.
		return GetExtensionProperties( 0, NULL, pCount, pProperties );
	}

	assert( physicalDevice );

	InstanceData_t * my_instance_data = (InstanceData_t *)HashMap_Find( &instance_data_map, GetDispatchTable( physicalDevice ) );

	return my_instance_data->instanceDispatchTable->EnumerateDeviceExtensionProperties( physicalDevice, NULL, pCount, pProperties );
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr( VkInstance instance, const char * funcName )
{
	ADD_HOOK( vkEnumerateInstanceLayerProperties );
	ADD_HOOK( vkEnumerateInstanceExtensionProperties );
	ADD_HOOK( vkEnumerateDeviceLayerProperties );
	ADD_HOOK( vkGetInstanceProcAddr );
	ADD_HOOK( vkCreateInstance );
	ADD_HOOK( vkDestroyInstance );
	ADD_HOOK( vkCreateDevice );

	if ( instance == NULL )
	{
		return NULL;
	}

	InstanceData_t * my_instance_data = (InstanceData_t *)HashMap_Find( &instance_data_map, GetDispatchTable( instance ) );

	VkLayerInstanceDispatchTable * pInstanceTable = my_instance_data->instanceDispatchTable;
	if ( pInstanceTable->GetInstanceProcAddr == NULL )
	{
		return NULL;
	}
	return pInstanceTable->GetInstanceProcAddr( instance, funcName );
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr( VkDevice device, const char * funcName )
{
	ADD_HOOK( vkEnumerateDeviceExtensionProperties );
	ADD_HOOK( vkGetDeviceProcAddr );
	ADD_HOOK( vkDestroyDevice );
	ADD_HOOK( vkCreateShaderModule );

	if ( device == NULL )
	{
		return NULL;
	}

	DeviceData_t * my_device_data = (DeviceData_t *)HashMap_Find( &device_data_map, GetDispatchTable( device ) );

	VkLayerDispatchTable * pDeviceTable = my_device_data->deviceDispatchTable;
	if ( pDeviceTable->GetDeviceProcAddr == NULL )
	{
		return NULL;
	}
	return pDeviceTable->GetDeviceProcAddr( device, funcName );
}

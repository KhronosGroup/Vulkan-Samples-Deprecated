/*
================================================================================================

Description	:	Queue Multiplexer for drivers that support too few queues per family.
Author		:	J.M.P. van Waveren
Date		:	05/12/2016
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

This layer implements queue multiplexing.

Some Vulkan drivers support only a single queue family with a single queue.
Applications that use more than one queue will not work with these Vulkan drivers.

This layer makes all Vulkan devices look like they have at least 16 queues
per family. There is virtually no impact on performance when the application
only uses the queues that are exposed by the Vulkan driver. If an application
uses more queues than are exposed by the Vulkan driver, then additional virtual
queues are used that all map to the last physical queue. When this happens there
may be a noticeable impact on performance, but at least the application will work.
The impact on performance automatically goes away when new Vulkan drivers are
installed with support for more queues, even if an application continues to use
this layer.


REQUIREMENTS
============

Either the Vulkan SDK (from https://lunarg.com/vulkan-sdk/) or a collocated Vulkan-LoaderAndValidationLayers
repository (from https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers) right next to the
Vulkan-Samples repository is needed to compile this layer. The paths for the collocated repositories
will look as follows:

	<path>/Vulkan-Samples/
	<path>/Vulkan-LoaderAndValidationLayers/

On Windows make sure that the path is no more than one folder deep to
avoid running into maximum path depth compilation issues.


VERSION HISTORY
===============

1.0		Initial version.

================================================================================================
*/

#if defined( WIN32 ) || defined( _WIN32 ) || defined( WIN64 ) || defined( _WIN64 )
	#define OS_WINDOWS
#elif defined( __ANDROID__ )
	#define OS_ANDROID
#elif defined( __APPLE__ )
	#define OS_APPLE
	#include <Availability.h>
	#if __IPHONE_OS_VERSION_MAX_ALLOWED
		#define OS_APPLE_IOS
	#elif __MAC_OS_X_VERSION_MAX_ALLOWED
		#define OS_APPLE_MACOS
	#endif
#elif defined( __linux__ )
	#define OS_LINUX
	//#define OS_LINUX_XLIB
	#define OS_LINUX_XCB
#else
	#error "unknown platform"
#endif

#if defined( OS_WINDOWS )
	#if !defined( _CRT_SECURE_NO_WARNINGS )
		#define _CRT_SECURE_NO_WARNINGS
	#endif
	#define VK_USE_PLATFORM_WIN32_KHR
#elif defined( OS_APPLE )

#elif defined( OS_LINUX_XLIB )
	#define VK_USE_PLATFORM_XLIB_KHR
#elif defined( OS_LINUX_XCB )
	#define VK_USE_PLATFORM_XCB_KHR
#elif defined( OS_ANDROID )
	#define VK_USE_PLATFORM_ANDROID_KHR
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
#endif

#define inline

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>				// {SDK}/Include/       | Vulkan-LoaderAndValidationLayers/include/                  | /usr/include/
#include <vulkan/vk_layer.h>			// {SDK}/Include/       | Vulkan-LoaderAndValidationLayers/include/                  | /usr/include/
#include <vk_dispatch_table_helper.h>	// {SDK}/Source/layers/ | Vulkan-LoaderAndValidationLayers/build/generated/include/

#if defined( OS_ANDROID )
#include <android/log.h>			// for __android_log_print()
#define Print( ... )				__android_log_print( ANDROID_LOG_INFO, "qm", __VA_ARGS__ )
#else
#define Print( ... )				printf( __VA_ARGS__ )
#endif

#define MIN_QUEUES_PER_FAMILY		16
#define LAYER_NAME					"VK_LAYER_OCULUS_queue_muxer"

/*
================================================================================================================================

ksMutex

static void ksMutex_Create( ksMutex * pMutex );
static void ksMutex_Destroy( ksMutex * pMutex );
static void ksMutex_Lock( ksMutex * pMutex );
static void ksMutex_Unlock( ksMutex * pMutex );

================================================================================================================================
*/

#if defined( WIN32 ) || defined( _WIN32 ) || defined( WIN64 ) || defined( _WIN64 )

#include <Windows.h>

typedef CRITICAL_SECTION ksMutex;

static void ksMutex_Create( ksMutex * mutex ) { InitializeCriticalSection( mutex ); }
static void ksMutex_Destroy( ksMutex * mutex ) { DeleteCriticalSection( mutex ); }
static void ksMutex_Lock( ksMutex * mutex ) { EnterCriticalSection( mutex ); }
static void ksMutex_Unlock( ksMutex * mutex ) { LeaveCriticalSection( mutex ); }

#else

#include <pthread.h>

typedef pthread_mutex_t ksMutex;

static void ksMutex_Create( ksMutex * mutex ) { pthread_mutex_init( mutex, NULL ); }
static void ksMutex_Destroy( ksMutex * mutex ) { pthread_mutex_destroy( mutex ); }
static void ksMutex_Lock( ksMutex * mutex ) { pthread_mutex_lock( mutex ); }
static void ksMutex_Unlock( ksMutex * mutex ) { pthread_mutex_unlock( mutex ); }

#endif

/*
================================================================================================================================

ksHashMap

static void ksHashMap_Create( ksHashMap * map );
static void ksHashMap_Destroy( ksHashMap * map );
static void ksHashMap_Add( ksHashMap * map, void * key, void * data );
static void ksHashMap_Remove( ksHashMap * map, void * key );
static void * ksHashMap_Find( ksHashMap * map, void * key );

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
} ksHashMap;

static void ksHashMap_Grow( ksHashMap * map, const int newCount )
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

static void ksHashMap_Create( ksHashMap * map )
{
	memset( map, 0, sizeof( ksHashMap ) );
	memset( map->table, -1, sizeof( map->table ) );
	ksHashMap_Grow( map, 16 );
}

static void ksHashMap_Destroy( ksHashMap * map )
{
	free( map->key );
	free( map->data );
	free( map->next );
	free( map->empty );
	memset( map, 0, sizeof( ksHashMap ) );
}

static unsigned int ksHashMap_HashPointer( ksHashMap * map, const void * key )
{
	// HASH_TABLE_SIZE must be a power of two.
	assert( ( HASH_TABLE_SIZE > 0 ) && ( HASH_TABLE_SIZE & ( HASH_TABLE_SIZE - 1 ) ) == 0 );
	const size_t value = (size_t)key;
	return (unsigned int)( ( ( value >> 3 ) ^ ( value >> 11 ) ^ ( value >> 19 ) ) & ( HASH_TABLE_SIZE - 1 ) );
}

static void ksHashMap_Add( ksHashMap * map, void * key, void * data )
{
	const unsigned int hash = ksHashMap_HashPointer( map, key );
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
		ksHashMap_Grow( map, map->count * 2 );
	}
	*index = map->empty[map->nextEmpty++];
	map->key[*index] = key;
	map->data[*index] = data;
	map->next[*index] = -1;
}

static void ksHashMap_Remove( ksHashMap * map, void * key )
{
	const unsigned int hash = ksHashMap_HashPointer( map, key );
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

static void * ksHashMap_Find( ksHashMap * map, void * key )
{
	const unsigned int hash = ksHashMap_HashPointer( map, key );
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
} ksInstanceData;

typedef struct
{
	ksInstanceData *				instanceData;
	VkLayerDispatchTable *			deviceDispatchTable;
	ksMutex							deviceMutex;
	uint32_t						queueFamilyCount;
	VkQueueFamilyProperties *		queueFamilyProperties;
} ksDeviceData;

typedef struct
{
	ksDeviceData *					deviceData;
	ksMutex							queueMutex;
} ksQueueData;

static ksHashMap instance_data_map;
static ksHashMap device_data_map;
static ksHashMap queue_data_map;

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
		ksHashMap_Create( &instance_data_map );
		ksHashMap_Create( &device_data_map );
		ksHashMap_Create( &queue_data_map );
	}

	ksInstanceData * my_instance_data = (ksInstanceData *)malloc( sizeof( ksInstanceData ) );
	my_instance_data->instanceDispatchTable = (VkLayerInstanceDispatchTable *)malloc( sizeof( VkLayerInstanceDispatchTable ) );
	layer_init_instance_dispatch_table( *pInstance, my_instance_data->instanceDispatchTable, pfnGetInstanceProcAddr );

	ksHashMap_Add( &instance_data_map, GetDispatchTable( *pInstance ), my_instance_data );
		
	return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
		VkInstance						instance,
		const VkAllocationCallbacks *	pAllocator)
{
	void * key = GetDispatchTable( instance );
	ksInstanceData * my_instance_data = (ksInstanceData *)ksHashMap_Find( &instance_data_map, key );

	my_instance_data->instanceDispatchTable->DestroyInstance( instance, pAllocator );

	ksHashMap_Remove( &instance_data_map, key );

	free( my_instance_data->instanceDispatchTable );
	free( my_instance_data );

	if ( instance_data_map.nextEmpty == 0 )
	{
		ksHashMap_Destroy( &instance_data_map );
		ksHashMap_Destroy( &device_data_map );
		ksHashMap_Destroy( &queue_data_map );
	}
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
		VkPhysicalDevice			physicalDevice,
		uint32_t *					pQueueFamilyPropertyCount,
		VkQueueFamilyProperties *	pQueueFamilyProperties )
{
	ksInstanceData * my_instance_data = (ksInstanceData *)ksHashMap_Find( &instance_data_map, GetDispatchTable( physicalDevice ) );

	my_instance_data->instanceDispatchTable->GetPhysicalDeviceQueueFamilyProperties( physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties );

	if ( pQueueFamilyProperties != NULL )
	{
		// Set minimum device queues.
		for ( uint32_t i = 0; i < *pQueueFamilyPropertyCount; i++ )
		{
			if ( pQueueFamilyProperties[i].queueCount < MIN_QUEUES_PER_FAMILY )
			{
				Print( "vkGetPhysicalDeviceQueueFamilyProperties: " LAYER_NAME " increased queueu family %d queue count from %d to %d",
						i, pQueueFamilyProperties[i].queueCount, MIN_QUEUES_PER_FAMILY );
				pQueueFamilyProperties[i].queueCount = MIN_QUEUES_PER_FAMILY;
			}
		}
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

	ksInstanceData * my_instance_data = (ksInstanceData *)ksHashMap_Find( &instance_data_map, GetDispatchTable( physicalDevice ) );

	// Get the queue family properties.
	uint32_t queueFamilyCount = 0;
	my_instance_data->instanceDispatchTable->GetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, NULL );

	VkQueueFamilyProperties * queueFamilyProperties = (VkQueueFamilyProperties *) malloc( queueFamilyCount * sizeof( VkQueueFamilyProperties ) );
	my_instance_data->instanceDispatchTable->GetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, queueFamilyProperties );

	// Bound the queue count per family.
	VkDeviceQueueCreateInfo * queueCreateInfos = (VkDeviceQueueCreateInfo *) malloc( pCreateInfo->queueCreateInfoCount * sizeof( pCreateInfo->pQueueCreateInfos[0] ) );
	for ( uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++ )
	{
		queueCreateInfos[i] = pCreateInfo->pQueueCreateInfos[i];
		if ( queueCreateInfos[i].queueCount > queueFamilyProperties[queueCreateInfos[i].queueFamilyIndex].queueCount )
		{
			queueCreateInfos[i].queueCount = queueFamilyProperties[queueCreateInfos[i].queueFamilyIndex].queueCount;
		}
	}
	VkDeviceCreateInfo deviceCreateInfo = *pCreateInfo;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;

	VkResult result = pfnCreateDevice( physicalDevice, &deviceCreateInfo, pAllocator, pDevice );
	if ( result != VK_SUCCESS )
	{
		free( queueCreateInfos );
		free( queueFamilyProperties );
		return result;
	}

	free( queueCreateInfos );

	VkLayerDispatchTable * pDeviceTable = (VkLayerDispatchTable *)malloc( sizeof( VkLayerDispatchTable ) );
	layer_init_device_dispatch_table( *pDevice, pDeviceTable, pfnGetDeviceProcAddr );

	// Setup device data.
	ksDeviceData * my_device_data = (ksDeviceData *)malloc( sizeof( ksDeviceData ) );
	my_device_data->instanceData = my_instance_data;
	my_device_data->deviceDispatchTable = pDeviceTable;
	my_device_data->queueFamilyCount = queueFamilyCount;
	my_device_data->queueFamilyProperties = queueFamilyProperties;
	ksMutex_Create( &my_device_data->deviceMutex );

	ksHashMap_Add( &device_data_map, GetDispatchTable( *pDevice ), my_device_data );

	return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
		VkDevice						device,
		const VkAllocationCallbacks *	pAllocator )
{
	void * key = GetDispatchTable( device );
	ksDeviceData * my_device_data = (ksDeviceData *)ksHashMap_Find( &device_data_map, key );

	my_device_data->deviceDispatchTable->DestroyDevice( device, pAllocator );

	// Free all queue objects associated with this device.
	ksMutex_Lock( &my_device_data->deviceMutex );
	for ( int i = 0; i < queue_data_map.count; i++ )
	{
		if ( queue_data_map.data[i] != NULL && ((ksQueueData *)queue_data_map.data[i])->deviceData == my_device_data )
		{
			free( queue_data_map.data[i] );
			ksHashMap_Remove( &queue_data_map, queue_data_map.key[i] );
		}
	}
	ksMutex_Unlock( &my_device_data->deviceMutex );

	ksHashMap_Remove( &device_data_map, key );

	ksMutex_Destroy( &my_device_data->deviceMutex );
	free( my_device_data->deviceDispatchTable );
	free( my_device_data->queueFamilyProperties );
	free( my_device_data );
}

/*
================================================================================================================================

Queues

================================================================================================================================
*/

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(
		VkDevice	device,
		uint32_t	queueFamilyIndex,
		uint32_t	queueIndex,
		VkQueue *	pQueue )
{
	ksDeviceData * my_device_data = (ksDeviceData *)ksHashMap_Find( &device_data_map, GetDispatchTable( device ) );

	// Direct all virtual queues to the last physical queue.
	if ( queueIndex >= my_device_data->queueFamilyProperties[queueFamilyIndex].queueCount )
	{
		queueIndex = my_device_data->queueFamilyProperties[queueFamilyIndex].queueCount - 1;
	}

	my_device_data->deviceDispatchTable->GetDeviceQueue( device, queueFamilyIndex, queueIndex, pQueue );

	// Create the queue mutex here without thread contention.
	ksMutex_Lock( &my_device_data->deviceMutex );
	if ( ksHashMap_Find( &queue_data_map, *pQueue ) == NULL )
	{
		ksQueueData * my_queue_data = (ksQueueData *)malloc( sizeof( ksQueueData ) );
		my_queue_data->deviceData = my_device_data;
		ksMutex_Create( &my_queue_data->queueMutex );
		ksHashMap_Add( &queue_data_map, *pQueue, my_queue_data );
	}
	ksMutex_Unlock( &my_device_data->deviceMutex );
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(
		VkQueue					queue,
		uint32_t				submitCount,
		const VkSubmitInfo *	pSubmits,
		VkFence					fence )
{
	ksDeviceData * my_device_data = (ksDeviceData *)ksHashMap_Find( &device_data_map, GetDispatchTable( queue ) );
	ksQueueData * my_queue_data = (ksQueueData *)ksHashMap_Find( &queue_data_map, queue );

	ksMutex_Lock( &my_queue_data->queueMutex );
	VkResult result = my_device_data->deviceDispatchTable->QueueSubmit( queue, submitCount, pSubmits, fence );
	ksMutex_Unlock( &my_queue_data->queueMutex );

	return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(
		VkQueue	queue )
{
	ksDeviceData * my_device_data = (ksDeviceData *)ksHashMap_Find( &device_data_map, GetDispatchTable( queue ) );
	ksQueueData * my_queue_data = (ksQueueData *)ksHashMap_Find( &queue_data_map, queue );

	ksMutex_Lock( &my_queue_data->queueMutex );
	VkResult result = my_device_data->deviceDispatchTable->QueueWaitIdle( queue );
	ksMutex_Unlock( &my_queue_data->queueMutex );

	return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(
		VkQueue						queue,
		const VkPresentInfoKHR *	pPresentInfo )
{
	ksDeviceData * my_device_data = (ksDeviceData *)ksHashMap_Find( &device_data_map, GetDispatchTable( queue ) );
	ksQueueData * my_queue_data = (ksQueueData *)ksHashMap_Find( &queue_data_map, queue );

	ksMutex_Lock( &my_queue_data->queueMutex );
	VkResult result = my_device_data->deviceDispatchTable->QueuePresentKHR( queue, pPresentInfo );
	ksMutex_Unlock( &my_queue_data->queueMutex );

	return result;
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
		"Oculus Queue Muxer",
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
		"Oculus Queue Muxer",
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

	ksInstanceData * my_instance_data = (ksInstanceData *)ksHashMap_Find( &instance_data_map, GetDispatchTable( physicalDevice ) );

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
	ADD_HOOK( vkGetPhysicalDeviceQueueFamilyProperties );
	ADD_HOOK( vkCreateDevice );

	if ( instance == NULL )
	{
		return NULL;
	}

	ksInstanceData * my_instance_data = (ksInstanceData *)ksHashMap_Find( &instance_data_map, GetDispatchTable( instance ) );

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
	ADD_HOOK( vkGetDeviceQueue );
	ADD_HOOK( vkQueueSubmit );
	ADD_HOOK( vkQueueWaitIdle );
	ADD_HOOK( vkQueuePresentKHR );
	ADD_HOOK( vkDestroyDevice );

	if ( device == NULL )
	{
		return NULL;
	}

	ksDeviceData * my_device_data = (ksDeviceData *)ksHashMap_Find( &device_data_map, GetDispatchTable( device ) );

	VkLayerDispatchTable * pDeviceTable = my_device_data->deviceDispatchTable;
	if ( pDeviceTable->GetDeviceProcAddr == NULL )
	{
		return NULL;
	}
	return pDeviceTable->GetDeviceProcAddr( device, funcName );
}

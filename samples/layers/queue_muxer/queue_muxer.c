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
Vulkan-Samples repository is needed to compile this layer.


COMMAND-LINE COMPILATION
========================

Microsoft Windows: Visual Studio 2013 Compiler:
	"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
	cl /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /EHsc /I%VK_SDK_PATH%\Include /I%VK_SDK_PATH%\Source\layers /I%VK_SDK_PATH%\Source\loader queue_muxer.cpp /link /DLL /OUT:VkLayer_queue_muxer.dll

Microsoft Windows: Intel Compiler 14.0
	"C:\Program Files (x86)\Intel\Composer XE\bin\iclvars.bat" intel64
	icl /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /EHsc /I%VK_SDK_PATH%\Include /I%VK_SDK_PATH%\Source\layers /I%VK_SDK_PATH%\Source\loader queue_muxer.cpp /link /DLL /OUT:VkLayer_queue_muxer.dll

Linux: GCC 4.8.2:
	gcc -std=c++11 -march=native -Wall -g -O2 -m64 -fPIC -shared -o VkLayer_queue_muxer.so -I${VK_SDK_PATH}\Source\layers -I${VK_SDK_PATH}\Source\loader queue_muxer.cpp

Android for ARM from Windows: NDK Revision 11c - Android 21 - ANT/Gradle
	ANT:
		cd projects/android/ant/
		build
	Gradle:
		cd projects/android/gradle/
		build


INSTALLATION
============

Windows:

	Add a reference to VkLayer_queue_muxer.json to the registry key:

	    HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ExplicitLayers

	Each key name must be a full path to the JSON manifest file.
	The Vulkan loader opens the JSON manifest file specified by the key name.
	The value of the key is a DWORD data set to 0.

	Alternatively, use the VK_LAYER_PATH environment variable to specify where the layer library resides.

Linux:

	Place the VkLayer_queue_muxer.json and libVkLayer_queue_muxer.so in one of the following folders:

		/usr/share/vulkan/icd.d
		/etc/vulkan/icd.d
		$HOME/.local/share/vulkan/icd.d

	Where $HOME is the current home directory of the application's user id; this path will be ignored for suid programs.
	Alternatively, use the VK_LAYER_PATH environment variable to specify where the layer library resides.

Android:

	On Android copy the libVkLayer_queue_muxer.so file to the application's lib folder:

		src/main/jniLibs/
		    arm64-v8a/
		        libVkLayer_queue_muxer.so
		        ...
		    armeabi-v7a/
		        libVkLayer_queue_muxer.so
		        ...

	Recompile the application and use the jar tool to verify that the libraries are actually in the APK:

		jar -tf <filename>.apk

	Alternatively, on a device with root access, the libVkLayer_queue_muxer.so can be placed in

		/data/local/debug/vulkan/

	The Android loader queries layer and extension information directly from the respective libraries,
	and does not use JSON manifest files as used by the Windows and Linux loaders.


ACTIVATION
==========

To enable the layer, the name of the layer ("VK_LAYER_OCULUS_queue_muxer") should be added
to the ppEnabledLayerNames member of VkInstanceCreateInfo when creating a VkInstance,
and the ppEnabledLayerNames member of VkDeviceCreateInfo when creating a VkDevice.

Windows:

	Alternatively, on Windows the layer can be enabled for all applications by adding the layer name
	("VK_LAYER_OCULUS_queue_muxer") to the VK_INSTANCE_LAYERS and VK_DEVICE_LAYERS environment variables.

		set VK_INSTANCE_LAYERS=VK_LAYER_OCULUS_queue_muxer
		set VK_DEVICE_LAYERS=VK_LAYER_OCULUS_queue_muxer

	Multiple layers can be enabled simultaneously by separating them with semi-colons.

		set VK_INSTANCE_LAYERS=VK_LAYER_OCULUS_queue_muxer;VK_LAYER_LUNARG_core_validation
		set VK_DEVICE_LAYERS=VK_LAYER_OCULUS_queue_muxer;VK_LAYER_LUNARG_core_validation

Linux:

	Alternatively, on Linux the layer can be enabled for all applications by adding the layer name
	("VK_LAYER_OCULUS_queue_muxer") to the VK_INSTANCE_LAYERS and VK_DEVICE_LAYERS environment variables.

		export VK_INSTANCE_LAYERS=VK_LAYER_OCULUS_queue_muxer
		export VK_DEVICE_LAYERS=VK_LAYER_OCULUS_queue_muxer

	Multiple layers can be enabled simultaneously by separating them with colons.

		export VK_INSTANCE_LAYERS=VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation
		export VK_DEVICE_LAYERS=VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation

Android:

	Alternatively on Android the layer can be enabled for all applications using
	the debug.vulkan.layers system property:

		adb shell setprop debug.vulkan.layers VK_LAYER_OCULUS_queue_muxer

	Multiple layers can be enabled simultaneously by separating them with colons.

		adb shell setprop debug.vulkan.layers VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation


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
#endif

#define inline

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>				// {SDK}/Include/       | Vulkan-LoaderAndValidationLayers/include/      | /usr/include/
#include <vulkan/vk_layer.h>			// {SDK}/Include/       | Vulkan-LoaderAndValidationLayers/include/      | /usr/include/
#include <vk_dispatch_table_helper.h>	// {SDK}/Source/layers/ | Vulkan-LoaderAndValidationLayers/build/layers/

#if defined( __ANDROID__ )
#include <android/log.h>			// for __android_log_print()
#define Print( ... )				__android_log_print( ANDROID_LOG_INFO, "qm", __VA_ARGS__ )
#else
#define Print( ... )				printf( __VA_ARGS__ )
#endif

#define MIN_QUEUES_PER_FAMILY		16
#define LAYER_NAME					"VK_LAYER_OCULUS_queue_muxer"

/*
================================================================================================================================

Mutex_t

static void Mutex_Create( Mutex_t * pMutex );
static void Mutex_Destroy( Mutex_t * pMutex );
static void Mutex_Lock( Mutex_t * pMutex );
static void Mutex_Unlock( Mutex_t * pMutex );

================================================================================================================================
*/

#if defined( WIN32 ) || defined( _WIN32 ) || defined( WIN64 ) || defined( _WIN64 )

#include <Windows.h>

typedef CRITICAL_SECTION Mutex_t;

static void Mutex_Create( Mutex_t * mutex ) { InitializeCriticalSection( mutex ); }
static void Mutex_Destroy( Mutex_t * mutex ) { DeleteCriticalSection( mutex ); }
static void Mutex_Lock( Mutex_t * mutex ) { EnterCriticalSection( mutex ); }
static void Mutex_Unlock( Mutex_t * mutex ) { LeaveCriticalSection( mutex ); }

#else

#include <pthread.h>

typedef pthread_mutex_t Mutex_t;

static void Mutex_Create( Mutex_t * mutex ) { pthread_mutex_init( mutex, NULL ); }
static void Mutex_Destroy( Mutex_t * mutex ) { pthread_mutex_destroy( mutex ); }
static void Mutex_Lock( Mutex_t * mutex ) { pthread_mutex_lock( mutex ); }
static void Mutex_Unlock( Mutex_t * mutex ) { pthread_mutex_unlock( mutex ); }

#endif

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
	Mutex_t							deviceMutex;
	uint32_t						queueFamilyCount;
	VkQueueFamilyProperties *		queueFamilyProperties;
} DeviceData_t;

typedef struct
{
	DeviceData_t *					deviceData;
	Mutex_t							queueMutex;
} QueueData_t;

static HashMap_t instance_data_map;
static HashMap_t device_data_map;
static HashMap_t queue_data_map;

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
		HashMap_Create( &queue_data_map );
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
		HashMap_Destroy( &queue_data_map );
	}
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
		VkPhysicalDevice			physicalDevice,
		uint32_t *					pQueueFamilyPropertyCount,
		VkQueueFamilyProperties *	pQueueFamilyProperties )
{
	InstanceData_t * my_instance_data = (InstanceData_t *)HashMap_Find( &instance_data_map, GetDispatchTable( physicalDevice ) );

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

	InstanceData_t * my_instance_data = (InstanceData_t *)HashMap_Find( &instance_data_map, GetDispatchTable( physicalDevice ) );

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
	pDeviceTable->CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)pDeviceTable->GetDeviceProcAddr( *pDevice, "vkCreateSwapchainKHR" );
	pDeviceTable->DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)pDeviceTable->GetDeviceProcAddr( *pDevice, "vkDestroySwapchainKHR" );
	pDeviceTable->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)pDeviceTable->GetDeviceProcAddr( *pDevice, "vkGetSwapchainImagesKHR" );
	pDeviceTable->AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)pDeviceTable->GetDeviceProcAddr( *pDevice, "vkAcquireNextImageKHR" );
	pDeviceTable->QueuePresentKHR = (PFN_vkQueuePresentKHR)pDeviceTable->GetDeviceProcAddr( *pDevice, "vkQueuePresentKHR" );

	// Setup device data.
	DeviceData_t * my_device_data = (DeviceData_t *)malloc( sizeof( DeviceData_t ) );
	my_device_data->instanceData = my_instance_data;
	my_device_data->deviceDispatchTable = pDeviceTable;
	my_device_data->queueFamilyCount = queueFamilyCount;
	my_device_data->queueFamilyProperties = queueFamilyProperties;
	Mutex_Create( &my_device_data->deviceMutex );

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

	// Free all queue objects associated with this device.
	Mutex_Lock( &my_device_data->deviceMutex );
	for ( int i = 0; i < queue_data_map.count; i++ )
	{
		if ( queue_data_map.data[i] != NULL && ((QueueData_t *)queue_data_map.data[i])->deviceData == my_device_data )
		{
			free( queue_data_map.data[i] );
			HashMap_Remove( &queue_data_map, queue_data_map.key[i] );
		}
	}
	Mutex_Unlock( &my_device_data->deviceMutex );

	HashMap_Remove( &device_data_map, key );

	Mutex_Destroy( &my_device_data->deviceMutex );
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
	DeviceData_t * my_device_data = (DeviceData_t *)HashMap_Find( &device_data_map, GetDispatchTable( device ) );

	// Direct all virtual queues to the last physical queue.
	if ( queueIndex >= my_device_data->queueFamilyProperties[queueFamilyIndex].queueCount )
	{
		queueIndex = my_device_data->queueFamilyProperties[queueFamilyIndex].queueCount - 1;
	}

	my_device_data->deviceDispatchTable->GetDeviceQueue( device, queueFamilyIndex, queueIndex, pQueue );

	// Create the queue mutex here without thread contention.
	Mutex_Lock( &my_device_data->deviceMutex );
	if ( HashMap_Find( &queue_data_map, *pQueue ) == NULL )
	{
		QueueData_t * my_queue_data = (QueueData_t *)malloc( sizeof( QueueData_t ) );
		my_queue_data->deviceData = my_device_data;
		Mutex_Create( &my_queue_data->queueMutex );
		HashMap_Add( &queue_data_map, *pQueue, my_queue_data );
	}
	Mutex_Unlock( &my_device_data->deviceMutex );
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(
		VkQueue					queue,
		uint32_t				submitCount,
		const VkSubmitInfo *	pSubmits,
		VkFence					fence )
{
	DeviceData_t * my_device_data = (DeviceData_t *)HashMap_Find( &device_data_map, GetDispatchTable( queue ) );
	QueueData_t * my_queue_data = (QueueData_t *)HashMap_Find( &queue_data_map, queue );

	Mutex_Lock( &my_queue_data->queueMutex );
	VkResult result = my_device_data->deviceDispatchTable->QueueSubmit( queue, submitCount, pSubmits, fence );
	Mutex_Unlock( &my_queue_data->queueMutex );

	return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(
		VkQueue	queue )
{
	DeviceData_t * my_device_data = (DeviceData_t *)HashMap_Find( &device_data_map, GetDispatchTable( queue ) );
	QueueData_t * my_queue_data = (QueueData_t *)HashMap_Find( &queue_data_map, queue );

	Mutex_Lock( &my_queue_data->queueMutex );
	VkResult result = my_device_data->deviceDispatchTable->QueueWaitIdle( queue );
	Mutex_Unlock( &my_queue_data->queueMutex );

	return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(
		VkQueue						queue,
		const VkPresentInfoKHR *	pPresentInfo )
{
	DeviceData_t * my_device_data = (DeviceData_t *)HashMap_Find( &device_data_map, GetDispatchTable( queue ) );
	QueueData_t * my_queue_data = (QueueData_t *)HashMap_Find( &queue_data_map, queue );

	Mutex_Lock( &my_queue_data->queueMutex );
	VkResult result = my_device_data->deviceDispatchTable->QueuePresentKHR( queue, pPresentInfo );
	Mutex_Unlock( &my_queue_data->queueMutex );

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
	ADD_HOOK( vkGetPhysicalDeviceQueueFamilyProperties );
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
	ADD_HOOK( vkGetDeviceQueue );
	ADD_HOOK( vkQueueSubmit );
	ADD_HOOK( vkQueueWaitIdle );
	ADD_HOOK( vkQueuePresentKHR );
	ADD_HOOK( vkDestroyDevice );

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

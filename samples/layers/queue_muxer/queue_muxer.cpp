/*
================================================================================================

Description	:	Queue Multiplexer for drivers that support too few queues per family.
Author		:	J.M.P. van Waveren
Date		:	05/12/2016
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
	gcc -std=c++11 -march=native -Wall -g -O2 -m64 -fPIC -shared -o VkLayer_queue_mutex.so -I${VK_SDK_PATH}\Source\layers -I${VK_SDK_PATH}\Source\loader queue_mutex.cpp

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

	Multiple layers can be enabled simultaneously by separating them with colons.

		set VK_INSTANCE_LAYERS=VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation
		set VK_DEVICE_LAYERS=VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation

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
#pragma warning( disable : 4350 )	// behavior change: 'member1' called instead of 'member2'
#pragma warning( disable : 4505 )	// unreferenced local function has been removed
#pragma warning( disable : 4668 )	// 'NTDDI_WIN7SP1' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning( disable : 4710 )	// function not inlined
#pragma warning( disable : 4711 )	// function '<name>' selected for automatic inline expansion
#pragma warning( disable : 4820 )	// '<name>' : 'X' bytes padding added after data member '<member>'
#endif

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>
#include <vk_loader_platform.h>
#include <vk_dispatch_table_helper.h>
#include <vk_layer_data.h>
#include <vk_layer_extension_utils.h>

#if defined( __ANDROID__ )
#include <android/log.h>			// for __android_log_print()
#define Print( ... )				__android_log_print( ANDROID_LOG_INFO, "qm", __VA_ARGS__ )
#else
#define Print( ... )				printf( __VA_ARGS__ )
#endif

#define MIN_QUEUES_PER_FAMILY		16


struct instance_data
{
									instance_data() :
										instanceDispatchTable( NULL ) {}

	VkLayerInstanceDispatchTable *	instanceDispatchTable;
};

struct device_data
{
									device_data() :
										instance( NULL ),
										deviceDispatchTable( NULL ),
										queueFamilyCount( 0 ),
										queueFamilyProperties( NULL ),
										pfnQueuePresentKHR( NULL ) {}

	instance_data *					instance;
	VkLayerDispatchTable *			deviceDispatchTable;
	loader_platform_thread_mutex	deviceLock;
	uint32_t						queueFamilyCount;
	VkQueueFamilyProperties *		queueFamilyProperties;
	PFN_vkQueuePresentKHR			pfnQueuePresentKHR;
};

struct queue_data
{
									queue_data() :
										device( NULL ) {}

	device_data *					device;
	loader_platform_thread_mutex	queueLock;
};

static std::unordered_map<void *, instance_data *> instance_data_map;
static std::unordered_map<void *, device_data *> device_data_map;
static std::unordered_map<void *, queue_data *> queue_data_map;

// Explicit instantiation.
template instance_data *get_my_data_ptr<instance_data>( void * data_key, std::unordered_map< void *, instance_data * > & data_map );
template device_data *get_my_data_ptr<device_data>( void * data_key, std::unordered_map< void *, device_data * > & data_map );
template queue_data *get_my_data_ptr<queue_data>( void * data_key, std::unordered_map< void *, queue_data * > & data_map );

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
	while ( chain_info != NULL && !( chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO && chain_info->function == VK_LAYER_LINK_INFO ) )
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

	instance_data * my_instance_data = get_my_data_ptr( get_dispatch_key( *pInstance ), instance_data_map );
	assert( my_instance_data->instanceDispatchTable == NULL );

	VkLayerInstanceDispatchTable * pInstanceTable = new VkLayerInstanceDispatchTable;
	layer_init_instance_dispatch_table( *pInstance, pInstanceTable, pfnGetInstanceProcAddr );

	my_instance_data->instanceDispatchTable = pInstanceTable;

	return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
		VkInstance						instance,
		const VkAllocationCallbacks *	pAllocator)
{
	dispatch_key key = get_dispatch_key( instance );
	instance_data * my_instance_data = get_my_data_ptr( key, instance_data_map );
	assert( my_instance_data->instanceDispatchTable != NULL );

	my_instance_data->instanceDispatchTable->DestroyInstance( instance, pAllocator );

	delete my_instance_data->instanceDispatchTable;
	my_instance_data->instanceDispatchTable = NULL;

	instance_data_map.erase( key );
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
		VkPhysicalDevice			physicalDevice,
		uint32_t *					pQueueFamilyPropertyCount,
		VkQueueFamilyProperties *	pQueueFamilyProperties )
{
	instance_data * my_instance_data = get_my_data_ptr( get_dispatch_key( physicalDevice ), instance_data_map );
	assert( my_instance_data->instanceDispatchTable != NULL );

	my_instance_data->instanceDispatchTable->GetPhysicalDeviceQueueFamilyProperties( physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties );

	if ( pQueueFamilyProperties != NULL )
	{
		// Set minimum device queues.
		for ( uint32_t i = 0; i < *pQueueFamilyPropertyCount; i++ )
		{
			if ( pQueueFamilyProperties[i].queueCount < MIN_QUEUES_PER_FAMILY )
			{
				Print( "vkGetPhysicalDeviceQueueFamilyProperties: VK_LAYER_OCULUS_queue_muxer increased queueu family %d queue count from %d to %d",
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
	while ( chain_info != NULL && !( chain_info->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO && chain_info->function == VK_LAYER_LINK_INFO ) )
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

	instance_data * my_instance_data = get_my_data_ptr( get_dispatch_key( physicalDevice ), instance_data_map );
	assert( my_instance_data->instanceDispatchTable != NULL );

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

	device_data * my_device_data = get_my_data_ptr( get_dispatch_key( *pDevice ), device_data_map );
	assert( my_device_data->deviceDispatchTable == NULL );

	// Setup device dispatch table
	VkLayerDispatchTable * pDeviceTable = new VkLayerDispatchTable;
	layer_init_device_dispatch_table( *pDevice, pDeviceTable, pfnGetDeviceProcAddr );

	my_device_data->instance = my_instance_data;
	my_device_data->deviceDispatchTable = pDeviceTable;
	my_device_data->queueFamilyCount = queueFamilyCount;
	my_device_data->queueFamilyProperties = queueFamilyProperties;
	my_device_data->pfnQueuePresentKHR = (PFN_vkQueuePresentKHR)pDeviceTable->GetDeviceProcAddr( *pDevice, "vkQueuePresentKHR" );
	loader_platform_thread_create_mutex( &my_device_data->deviceLock );

	return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
		VkDevice						device,
		const VkAllocationCallbacks *	pAllocator )
{
	dispatch_key key = get_dispatch_key( device );
	device_data * my_device_data = get_my_data_ptr( key, device_data_map );
	assert( my_device_data->deviceDispatchTable != NULL );

	my_device_data->deviceDispatchTable->DestroyDevice( device, pAllocator );

	// Free all queue objects associated with this device.
	loader_platform_thread_lock_mutex( &my_device_data->deviceLock );
	for ( std::unordered_map<void *, queue_data *>::const_iterator cur = queue_data_map.begin(); cur != queue_data_map.end(); )
	{
		if ( cur->second->device == my_device_data )
		{
			queue_data_map.erase( cur );
			cur = queue_data_map.begin();
		}
		else
		{
			cur++;
		}
	}
	loader_platform_thread_unlock_mutex( &my_device_data->deviceLock );

	my_device_data->instance = NULL;
	delete my_device_data->deviceDispatchTable;
	my_device_data->deviceDispatchTable = NULL;
	my_device_data->queueFamilyCount = 0;
	free( my_device_data->queueFamilyProperties );
	my_device_data->queueFamilyProperties = NULL;
	loader_platform_thread_delete_mutex( &my_device_data->deviceLock );

	device_data_map.erase( key );
}

/*
================================================================================================================================

Queues

================================================================================================================================
*/

static loader_platform_thread_mutex * GetQueueLock( device_data * my_device_data, VkQueue queue )
{
	// NOTE: this fetches or creates a queue_data for each unique queue.
	queue_data * my_queue_data = get_my_data_ptr( (void *)queue, queue_data_map );
	if ( my_queue_data->device == NULL )
	{
		my_queue_data->device = my_device_data;
		loader_platform_thread_create_mutex( &my_queue_data->queueLock );
	}
	return &my_queue_data->queueLock;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(
		VkDevice	device,
		uint32_t	queueFamilyIndex,
		uint32_t	queueIndex,
		VkQueue *	pQueue )
{
	device_data * my_device_data = get_my_data_ptr( get_dispatch_key( device ), device_data_map );
	assert( my_device_data->deviceDispatchTable != NULL );

	// Direct all virtual queues to the last physical queue.
	if ( queueIndex >= my_device_data->queueFamilyProperties[queueFamilyIndex].queueCount )
	{
		queueIndex = my_device_data->queueFamilyProperties[queueFamilyIndex].queueCount - 1;
	}

	my_device_data->deviceDispatchTable->GetDeviceQueue( device, queueFamilyIndex, queueIndex, pQueue );

	// Create the queue lock here without thread contention.
	loader_platform_thread_lock_mutex( &my_device_data->deviceLock );
	GetQueueLock( my_device_data, *pQueue );
	loader_platform_thread_unlock_mutex( &my_device_data->deviceLock );
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(
		VkQueue					queue,
		uint32_t				submitCount,
		const VkSubmitInfo *	pSubmits,
		VkFence					fence )
{
	device_data * my_device_data = get_my_data_ptr( get_dispatch_key( queue ), device_data_map );
	assert( my_device_data->deviceDispatchTable != NULL );

	loader_platform_thread_mutex * lock = GetQueueLock( my_device_data, queue );

	loader_platform_thread_lock_mutex( lock );
	VkResult result = my_device_data->deviceDispatchTable->QueueSubmit( queue, submitCount, pSubmits, fence );
	loader_platform_thread_unlock_mutex( lock );

	return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(
		VkQueue	queue )
{
	device_data * my_device_data = get_my_data_ptr( get_dispatch_key( queue ), device_data_map );
	assert( my_device_data->deviceDispatchTable != NULL );

	loader_platform_thread_mutex * lock = GetQueueLock( my_device_data, queue );

	loader_platform_thread_lock_mutex( lock );
	VkResult result = my_device_data->deviceDispatchTable->QueueWaitIdle( queue );
	loader_platform_thread_unlock_mutex( lock );

	return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(
		VkQueue						queue,
		const VkPresentInfoKHR *	pPresentInfo )
{
	device_data * my_device_data = get_my_data_ptr( get_dispatch_key( queue ), device_data_map );
	assert( my_device_data->deviceDispatchTable != NULL );

	loader_platform_thread_mutex * lock = GetQueueLock( my_device_data, queue );

	loader_platform_thread_lock_mutex( lock );
	VkResult result = my_device_data->pfnQueuePresentKHR( queue, pPresentInfo );
	loader_platform_thread_unlock_mutex( lock );

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
		"VK_LAYER_OCULUS_queue_muxer",
		VK_MAKE_VERSION( 1, 0, 3 ),
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
	// This layer does not implement any instance extensions.
	return GetExtensionProperties( 0, NULL, pCount, pProperties );
}

static const VkLayerProperties deviceLayerProps[] =
{
	{
		"VK_LAYER_OCULUS_queue_muxer",
		VK_MAKE_VERSION( 1, 0, 3 ),
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
	if ( pLayerName != NULL && strcmp( pLayerName, deviceLayerProps[0].layerName ) == 0 )
	{
		// This layer does not implement any device extensions.
		return GetExtensionProperties( 0, NULL, pCount, pProperties );
	}

	assert( physicalDevice );

	instance_data * my_instance_data = get_my_data_ptr( get_dispatch_key( physicalDevice ), instance_data_map );
	assert( my_instance_data->instanceDispatchTable != NULL );

	return my_instance_data->instanceDispatchTable->EnumerateDeviceExtensionProperties( physicalDevice, NULL, pCount, pProperties );
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr( VkInstance instance, const char * funcName )
{
#define ADD_HOOK( fn ) \
	if ( !strncmp( #fn, funcName, sizeof( #fn ) ) ) \
		return (PFN_vkVoidFunction) fn

	ADD_HOOK( vkEnumerateInstanceLayerProperties );
	ADD_HOOK( vkEnumerateInstanceExtensionProperties );
	ADD_HOOK( vkEnumerateDeviceLayerProperties );
	ADD_HOOK( vkGetInstanceProcAddr );
	ADD_HOOK( vkCreateInstance );
	ADD_HOOK( vkDestroyInstance );
	ADD_HOOK( vkGetPhysicalDeviceQueueFamilyProperties );
	ADD_HOOK( vkCreateDevice );
#undef ADD_HOOK

	if ( instance == NULL )
	{
		return NULL;
	}

	instance_data * my_instance_data = get_my_data_ptr( get_dispatch_key( instance ), instance_data_map );
	assert( my_instance_data->instanceDispatchTable != NULL );

	VkLayerInstanceDispatchTable * pInstanceTable = my_instance_data->instanceDispatchTable;
	if ( pInstanceTable->GetInstanceProcAddr == NULL )
	{
		return NULL;
	}
	return pInstanceTable->GetInstanceProcAddr( instance, funcName );
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr( VkDevice device, const char * funcName )
{
#define ADD_HOOK( fn ) \
	if ( !strncmp( #fn, funcName, sizeof( #fn ) ) ) \
		return (PFN_vkVoidFunction) fn

	ADD_HOOK( vkEnumerateDeviceExtensionProperties );
	ADD_HOOK( vkGetDeviceProcAddr );
	ADD_HOOK( vkGetDeviceQueue );
	ADD_HOOK( vkQueueSubmit );
	ADD_HOOK( vkQueueWaitIdle );
	ADD_HOOK( vkQueuePresentKHR );
	ADD_HOOK( vkDestroyDevice );
#undef ADD_HOOK

	if ( device == NULL )
	{
		return NULL;
	}

	device_data * my_device_data = get_my_data_ptr( get_dispatch_key( device ), device_data_map );
	assert( my_device_data->deviceDispatchTable != NULL );

	VkLayerDispatchTable * pDeviceTable = my_device_data->deviceDispatchTable;
	if ( pDeviceTable->GetDeviceProcAddr == NULL )
	{
		return NULL;
	}
	return pDeviceTable->GetDeviceProcAddr( device, funcName );
}

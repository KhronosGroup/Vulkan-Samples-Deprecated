/*
================================================================================================

Description	:	Vulkan driver information.
Author		:	J.M.P. van Waveren
Date		:	06/12/2016
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

This sample displays Vulkan driver information.


IMPLEMENTATION
==============

The code is written against version 1.0.0 of the Vulkan Specification.

Supported platforms are:

	- Microsoft Windows 7 or later
	- Apple Mac OS X 10.9 or later
	- Ubuntu Linux 14.04 or later
	- Android 5.0 or later


COMMAND-LINE COMPILATION
========================

Microsoft Windows: Visual Studio 2013 Compiler:
	"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
	cl /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /I%VK_SDK_PATH%\Include driverinfo_vulkan.c /link user32.lib gdi32.lib Advapi32.lib Shlwapi.lib

Microsoft Windows: Intel Compiler 14.0
	"C:\Program Files (x86)\Intel\Composer XE\bin\iclvars.bat" intel64
	icl /Qstd=c99 /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /I%VK_SDK_PATH%\Include driverinfo_vulkan.c /link user32.lib gdi32.lib Advapi32.lib Shlwapi.lib

Apple Mac OS X: Apple LLVM 6.0:
	clang -std=c99 -x objective-c -fno-objc-arc -Wall -g -O2 -m64 -o driverinfo_vulkan driverinfo_vulkan.c -framework Cocoa

Linux: GCC 4.8.2 Xlib:
	sudo apt-get install libx11-dev
	sudo apt-get install libxxf86vm-dev
	sudo apt-get install libxrandr-dev
	gcc -std=c99 -Wall -g -O2 -m64 -o driverinfo_vulkan driverinfo_vulkan.c -lm -lpthread -ldl -lX11 -lXxf86vm -lXrandr

Linux: GCC 4.8.2 XCB:
	sudo apt-get install libxcb1-dev
	sudo apt-get install libxcb-keysyms1-dev
	sudo apt-get install libxcb-icccm4-dev
	gcc -std=c99 -Wall -g -O2 -m64 -o driverinfo_vulkan driverinfo_vulkan.c -lm -lpthread -ldl -lxcb -lxcb-keysyms -lxcb-randr

Android for ARM from Windows: NDK Revision 11c - Android 21 - ANT/Gradle
	ANT:
		cd projects/android/ant/driverinfo_vulkan
		ndk-build
		ant debug
		adb install -r bin/driverinfo_vulkan-debug.apk
	Gradle:
		cd projects/android/gradle/driverinfo_vulkan
		gradlew build
		adb install -r build/outputs/apk/driverinfo_vulkan-all-debug.apk


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

/*
================================
Platform headers / declarations
================================
*/

#if defined( OS_WINDOWS )

	#if !defined( _CRT_SECURE_NO_WARNINGS )
		#define _CRT_SECURE_NO_WARNINGS
	#endif

	#if defined( _MSC_VER )
		#pragma warning( disable : 4204 )	// nonstandard extension used : non-constant aggregate initializer
		#pragma warning( disable : 4255 )	// '<name>' : no function prototype given: converting '()' to '(void)'
		#pragma warning( disable : 4668 )	// '__cplusplus' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
		#pragma warning( disable : 4710	)	// 'int printf(const char *const ,...)': function not inlined
		#pragma warning( disable : 4711 )	// function '<name>' selected for automatic inline expansion
		#pragma warning( disable : 4738 )	// storing 32-bit float result in memory, possible loss of performance
		#pragma warning( disable : 4820 )	// '<name>' : 'X' bytes padding added after data member '<member>'
	#endif

	#if _MSC_VER >= 1900
		#pragma warning( disable : 4464	)	// relative include path contains '..'
		#pragma warning( disable : 4774	)	// 'printf' : format string expected in argument 1 is not a string literal
	#endif

	#include <conio.h>			// for _getch()
	#include <windows.h>

	#include "vulkan/vulkan.h"
	#include "vulkan/vk_sdk_platform.h"

	#define VULKAN_LOADER	"vulkan-1.dll"

#elif defined( OS_APPLE )

	#include <sys/param.h>
	#include <sys/sysctl.h>
	#include <sys/time.h>
	#include <pthread.h>
	#include <dlfcn.h>						// for dlopen
	#include <Cocoa/Cocoa.h>

	#include "vulkan/vulkan.h"
	#include "vulkan/vk_sdk_platform.h"

	#if defined( OS_APPLE_IOS )
		#include <UIKit/UIKit.h>
		#if defined( VK_USE_PLATFORM_IOS_MVK )
			#include <QuartzCore/CAMetalLayer.h>
			#include <MoltenVK/vk_mvk_moltenvk.h>
			#include <MoltenVK/vk_mvk_ios_surface.h>
			#define VkIOSSurfaceCreateInfoKHR						VkIOSSurfaceCreateInfoMVK
			#define VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_KHR	VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK
			#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME			VK_MVK_IOS_SURFACE_EXTENSION_NAME
			#define PFN_vkCreateSurfaceKHR							PFN_vkCreateIOSSurfaceMVK
			#define vkCreateSurfaceKHR								vkCreateIOSSurfaceMVK
		#else
			// Only here to make the code compile.
			typedef VkFlags VkIOSSurfaceCreateFlagsKHR;
			typedef struct VkIOSSurfaceCreateInfoKHR {
				VkStructureType				sType;
				const void *				pNext;
				VkIOSSurfaceCreateFlagsKHR	flags;
				UIView *					pView;
			} VkIOSSurfaceCreateInfoKHR;
			#define VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_KHR	1000015000
			#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME			"VK_KHR_ios_surface"
			typedef VkResult (VKAPI_PTR *PFN_vkCreateIOSSurfaceKHR)(VkInstance instance, const VkIOSSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
			#define PFN_vkCreateSurfaceKHR							PFN_vkCreateIOSSurfaceKHR
			#define vkCreateSurfaceKHR								vkCreateIOSSurfaceKHR
			#define VULKAN_LOADER									"libvulkan.dylib"
		#endif
	#endif

	#if defined( OS_APPLE_MACOS )
		#include <AppKit/AppKit.h>
		#if defined( VK_USE_PLATFORM_MACOS_MVK )
			#include <QuartzCore/CAMetalLayer.h>
			#include <MoltenVK/vk_mvk_moltenvk.h>
			#include <MoltenVK/vk_mvk_macos_surface.h>
			#define VkMacOSSurfaceCreateInfoKHR						VkMacOSSurfaceCreateInfoMVK
			#define VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_KHR	VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK
			#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME			VK_MVK_MACOS_SURFACE_EXTENSION_NAME
			#define PFN_vkCreateSurfaceKHR							PFN_vkCreateMacOSSurfaceMVK
			#define vkCreateSurfaceKHR								vkCreateMacOSSurfaceMVK
		#else
			// Only here to make the code compile.
			typedef VkFlags VkMacOSSurfaceCreateFlagsKHR;
			typedef struct VkMacOSSurfaceCreateInfoKHR {
				VkStructureType					sType;
				const void *					pNext;
				VkMacOSSurfaceCreateFlagsKHR	flags;
				NSView *						pView;
			} VkMacOSSurfaceCreateInfoKHR;
			#define VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_KHR	1000015000
			#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME			"VK_KHR_macos_surface"
			typedef VkResult (VKAPI_PTR *PFN_vkCreateMacOSSurfaceKHR)(VkInstance instance, const VkMacOSSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
			#define PFN_vkCreateSurfaceKHR							PFN_vkCreateMacOSSurfaceKHR
			#define vkCreateSurfaceKHR								vkCreateMacOSSurfaceKHR
			#define VULKAN_LOADER									"libvulkan.dylib"
		#endif
	#endif

	#pragma clang diagnostic ignored "-Wunused-function"
	#pragma clang diagnostic ignored "-Wunused-const-variable"

#elif defined( OS_LINUX )

	#include <sys/time.h>
	#define __USE_UNIX98	// for pthread_mutexattr_settype
	#include <pthread.h>
	#include <malloc.h>                     // for memalign
	#include <dlfcn.h>						// for dlopen
	#if defined( OS_LINUX_XLIB )
		#include <X11/Xlib.h>
		#include <X11/Xatom.h>
		#include <X11/extensions/xf86vmode.h>	// for fullscreen video mode
		#include <X11/extensions/Xrandr.h>		// for resolution changes
	#elif defined( OS_LINUX_XCB )
		#include <X11/keysym.h>
		#include <xcb/xcb.h>
		#include <xcb/xcb_keysyms.h>
		#include <xcb/xcb_icccm.h>
		#include <xcb/randr.h>
	#endif

	#include "vulkan/vulkan.h"
	#include "vulkan/vk_sdk_platform.h"

	#define VULKAN_LOADER	"libvulkan-1.so"

	#pragma GCC diagnostic ignored "-Wunused-function"

#elif defined( OS_ANDROID )

	#include <time.h>
	#include <unistd.h>
	#include <pthread.h>
	#include <ctype.h>							// for isdigit, isspace
	#include <malloc.h>							// for memalign
	#include <dlfcn.h>							// for dlopen
	#include <sys/prctl.h>						// for prctl( PR_SET_NAME )
	#include <sys/stat.h>						// for gettid
	#include <sys/syscall.h>					// for syscall
	#include <android/log.h>					// for __android_log_print
	#include <android/input.h>					// for AKEYCODE_ etc.
	#include <android/window.h>					// for AWINDOW_FLAG_KEEP_SCREEN_ON
	#include <android/native_window_jni.h>		// for native window JNI
	#include <android_native_app_glue.h>

	#include "vulkan/vulkan.h"
	#include "vulkan/vk_sdk_platform.h"

	#define VULKAN_LOADER	"libvulkan.so"

	#pragma GCC diagnostic ignored "-Wunused-function"

#endif

/*
================================
Common headers
================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <string.h>			// for memset
#include <errno.h>			// for EBUSY, ETIMEDOUT etc.

/*
================================
Common defines
================================
*/

#define UNUSED_PARM( x )				{ (void)(x); }
#define ARRAY_SIZE( a )					( sizeof( (a) ) / sizeof( (a)[0] ) )
#define OFFSETOF_MEMBER( type, member )	(size_t)&((type *)0)->member
#define SIZEOF_MEMBER( type, member )	sizeof( ((type *)0)->member )
#define BIT( x )						( 1 << (x) )
#define ROUNDUP( x, granularity )		( ( (x) + (granularity) - 1 ) & ~( (granularity) - 1 ) )
#define CLAMP( x, min, max )			( ( (x) < (min) ) ? (min) : ( ( (x) > (max) ) ? (max) : (x) ) )
#define STRINGIFY_EXPANDED( a )			#a
#define STRINGIFY( a )					STRINGIFY_EXPANDED(a)
#define ENUM_STRING_CASE( e )			case (e): return #e

#define APPLICATION_NAME				"DriverInfo"

#define VK_ALLOCATOR					NULL

#if !defined( VK_API_VERSION_1_0 )
#define VK_API_VERSION_1_0 VK_API_VERSION
#endif

#if !defined( VK_VERSION_MAJOR )
#define VK_VERSION_MAJOR(version) ((uint32_t)(version) >> 22)
#define VK_VERSION_MINOR(version) (((uint32_t)(version) >> 12) & 0x3ff)
#define VK_VERSION_PATCH(version) ((uint32_t)(version) & 0xfff)
#endif

/*
================================================================================================================================

System level functionality

================================================================================================================================
*/

static void Print( const char * format, ... )
{
#if defined( OS_WINDOWS )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	vsnprintf_s( buffer, 4096, _TRUNCATE, format, args );
	va_end( args );

	printf( buffer );
	OutputDebugString( buffer );
#elif defined( OS_APPLE )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	vsnprintf( buffer, 4096, format, args );
	va_end( args );

	NSLog( @"%s", buffer );
#elif defined( OS_LINUX )
	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );
	fflush( stdout );
#elif defined( OS_ANDROID )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	vsnprintf( buffer, 4096, format, args );
	va_end( args );

	__android_log_print( ANDROID_LOG_VERBOSE, "DriverInfo", "%s", buffer );
#endif
}

static void Error( const char * format, ... )
{
#if defined( OS_WINDOWS )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	vsnprintf_s( buffer, 4096, _TRUNCATE, format, args );
	va_end( args );

	OutputDebugString( buffer );

	MessageBox( NULL, buffer, "ERROR", MB_OK | MB_ICONINFORMATION );
#elif defined( OS_APPLE_IOS )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	int length = vsnprintf( buffer, 4096, format, args );
	va_end( args );

	NSLog( @"%s\n", buffer );

	if ( [NSThread isMainThread] )
	{
		NSString * string = [[NSString alloc] initWithBytes:buffer length:length encoding:NSASCIIStringEncoding];
		UIAlertController* alert = [UIAlertController alertControllerWithTitle: @"Error"
																	   message: string
																preferredStyle: UIAlertControllerStyleAlert];
		[alert addAction: [UIAlertAction actionWithTitle: @"OK"
												   style: UIAlertActionStyleDefault
												 handler: ^(UIAlertAction * action) {}]];
		[UIApplication.sharedApplication.keyWindow.rootViewController presentViewController: alert animated: YES completion: nil];
	}
#elif defined( OS_APPLE_MACOS )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	int length = vsnprintf( buffer, 4096, format, args );
	va_end( args );

	NSLog( @"%s\n", buffer );

	if ( [NSThread isMainThread] )
	{
		NSString * string = [[NSString alloc] initWithBytes:buffer length:length encoding:NSASCIIStringEncoding];
		NSAlert * alert = [[NSAlert alloc] init];
		[alert addButtonWithTitle:@"OK"];
		[alert setMessageText:@"Error"];
		[alert setInformativeText:string];
		[alert setAlertStyle:NSWarningAlertStyle];
		[alert runModal];
	}
#elif defined( OS_LINUX )
	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );
	printf( "\n" );
	fflush( stdout );
#elif defined( OS_ANDROID )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	vsnprintf( buffer, 4096, format, args );
	va_end( args );

	__android_log_print( ANDROID_LOG_ERROR, "DriverInfo", "%s", buffer );
#endif
	// Without exiting, the application will likely crash.
	if ( format != NULL )
	{
		exit( 0 );
	}
}

static const char * GetOSVersion()
{
#if defined( OS_WINDOWS )
	HKEY hKey = 0;
	if ( RegOpenKey( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", &hKey ) == ERROR_SUCCESS )
	{
		static char version[1024];
		DWORD version_length = sizeof( version );
		DWORD dwType = REG_SZ;
		if ( RegQueryValueEx( hKey, "ProductName", NULL, &dwType, (LPBYTE)&version, &version_length ) == ERROR_SUCCESS )
		{
			return version;
		}
	}

	return "Microsoft Windows";
#elif defined( OS_APPLE_IOS )
	return [NSString stringWithFormat: @"Apple iOS %@", NSProcessInfo.processInfo.operatingSystemVersionString].UTF8String;
#elif defined( OS_APPLE_MACOS )
	return [NSString stringWithFormat: @"Apple macOS %@", NSProcessInfo.processInfo.operatingSystemVersionString].UTF8String;
#elif defined( OS_LINUX )
	static char buffer[1024];

	FILE * os_release = fopen( "/etc/os-release", "r" );
	if ( os_release != NULL )
	{
		while ( fgets( buffer, sizeof( buffer ), os_release ) )
		{
			if ( strncmp( buffer, "PRETTY_NAME=", 12 ) == 0 )
			{
				char * pretty_name = buffer + 12;

				// remove newline and enclosing quotes
				while(	pretty_name[0] == ' ' ||
						pretty_name[0] == '\t' ||
						pretty_name[0] == ':' ||
						pretty_name[0] == '\'' ||
						pretty_name[0] == '\"' )
				{
					pretty_name++;
				}
				int last = strlen( pretty_name ) - 1;
				while(	last >= 0 && (
						pretty_name[last] == '\n' ||
						pretty_name[last] == '\'' ||
						pretty_name[last] == '\"' ) )
				{
					pretty_name[last--] = '\0';
				}
				return pretty_name;
			}
		}

		fclose( os_release );
	}

	return "Linux";
#elif defined( OS_ANDROID )
	static char version[1024];

	#define PROP_NAME_MAX   32
	#define PROP_VALUE_MAX  92

	char release[PROP_VALUE_MAX] = { 0 };
	char build[PROP_VALUE_MAX] = { 0 };

	void * handle = dlopen( "libc.so", RTLD_NOLOAD );
	if ( handle != NULL )
	{
		typedef int (*PFN_SYSTEM_PROP_GET)(const char *, char *);
		PFN_SYSTEM_PROP_GET __my_system_property_get = (PFN_SYSTEM_PROP_GET)dlsym( handle, "__system_property_get" );
		if ( __my_system_property_get != NULL )
		{
			__my_system_property_get( "ro.build.version.release", release );
			__my_system_property_get( "ro.build.version.incremental", build );
		}
	}

	snprintf( version, sizeof( version ), "Android %s (%s)", release, build );

	return version;
#endif
}

static const char * GetCPUVersion()
{
#if defined( OS_WINDOWS )
	HKEY hKey = 0;
	if ( RegOpenKey( HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", &hKey ) == ERROR_SUCCESS )
	{
		static char processor[1024];
		DWORD processor_length = sizeof( processor );
		DWORD dwType = REG_SZ;
		if ( RegQueryValueEx( hKey, "ProcessorNameString", NULL, &dwType, (LPBYTE)&processor, &processor_length ) == ERROR_SUCCESS )
		{
			return processor;
		}
	}
#elif defined( OS_APPLE )
	static char processor[1024];
	size_t processor_length = sizeof( processor );
	sysctlbyname( "machdep.cpu.brand_string", &processor, &processor_length, NULL, 0 );
	return processor;
#elif defined( OS_LINUX ) || defined( OS_ANDROID )
	struct
	{
		const char * key;
		char value[1024];
	} keyValues[] =
	{
		{ "model name", "" },
		{ "Processor", "" },
		{ "Hardware", "" }
	};
	static char name[1024];

	FILE * cpuinfo = fopen( "/proc/cpuinfo", "r" );
	if ( cpuinfo != NULL )
	{
		char buffer[1024];
		while ( fgets( buffer, sizeof( buffer ), cpuinfo ) )
		{
			for ( int i = 0; i < (int)ARRAY_SIZE( keyValues ); i++ )
			{
				const size_t length = strlen( keyValues[i].key );
				if ( strncmp( buffer, keyValues[i].key, length ) == 0 )
				{
					char * pretty_name = buffer + length;

					// remove newline and enclosing quotes
					while(	pretty_name[0] == ' ' ||
							pretty_name[0] == '\t' ||
							pretty_name[0] == ':' ||
							pretty_name[0] == '\'' ||
							pretty_name[0] == '\"' )
					{
						pretty_name++;
					}
					int last = strlen( pretty_name ) - 1;
					while(	last >= 0 && (
							pretty_name[last] == '\n' ||
							pretty_name[last] == '\'' ||
							pretty_name[last] == '\"' ) )
					{
						pretty_name[last--] = '\0';
					}

					strcpy( keyValues[i].value, pretty_name );
					break;
				}
			}
		}

		fclose( cpuinfo );

		sprintf( name, "%s%s%s", keyValues[2].value,
				( keyValues[2].value[0] != '\0' ) ? " - " : "",
				( keyValues[0].value[0] != '\0' ) ? keyValues[0].value : keyValues[1].value );
		return name;
	}
#endif
	return "unknown";
}

/*
================================================================================================================================

Console

================================================================================================================================
*/

static void Console_Resize( const short numLines, const short numColumns )
{
#if defined( OS_WINDOWS )
	DWORD pids[2];
	DWORD num_pids = GetConsoleProcessList( pids, ARRAY_SIZE( pids ) );
	if ( num_pids > 1 )
	{
		return;
	}

	HANDLE consoleHandle = GetStdHandle( STD_OUTPUT_HANDLE );

	// Set the console window size.
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if ( GetConsoleScreenBufferInfo( consoleHandle, &csbi ) )
	{
		COORD bufferSize;
		bufferSize.X = numColumns > csbi.dwSize.X ? numColumns : csbi.dwSize.X;
		bufferSize.Y = numLines > csbi.dwSize.Y ? numLines : csbi.dwSize.Y;
		SetConsoleScreenBufferSize( consoleHandle, bufferSize );

		// Position the console window.
		SMALL_RECT rect;
		rect.Left = 0;
		rect.Top = 0;
		rect.Right = ( numColumns > csbi.dwSize.X ? numColumns : csbi.dwSize.X ) - 1;
		rect.Bottom = 100 - 1;
		SetConsoleWindowInfo( consoleHandle, TRUE, &rect );
	}
#else
	UNUSED_PARM( numLines );
	UNUSED_PARM( numColumns );
#endif
}

/*
================================================================================================================================

Vulkan error checking.

================================================================================================================================
*/

#if defined( _DEBUG )
	#define VK( func )		VkCheckErrors( func, #func );
	#define VC( func )		func;
#else
	#define VK( func )		VkCheckErrors( func, #func );
	#define VC( func )		func;
#endif

#define VK_ERROR_INVALID_SHADER_NV		-1002

const char * VkErrorString( VkResult result )
{
	switch( result )
	{
		ENUM_STRING_CASE( VK_SUCCESS );
		ENUM_STRING_CASE( VK_NOT_READY );
		ENUM_STRING_CASE( VK_TIMEOUT );
		ENUM_STRING_CASE( VK_EVENT_SET );
		ENUM_STRING_CASE( VK_EVENT_RESET );
		ENUM_STRING_CASE( VK_INCOMPLETE );
		ENUM_STRING_CASE( VK_ERROR_OUT_OF_HOST_MEMORY );
		ENUM_STRING_CASE( VK_ERROR_OUT_OF_DEVICE_MEMORY );
		ENUM_STRING_CASE( VK_ERROR_INITIALIZATION_FAILED );
		ENUM_STRING_CASE( VK_ERROR_DEVICE_LOST );
		ENUM_STRING_CASE( VK_ERROR_MEMORY_MAP_FAILED );
		ENUM_STRING_CASE( VK_ERROR_LAYER_NOT_PRESENT );
		ENUM_STRING_CASE( VK_ERROR_EXTENSION_NOT_PRESENT );
		ENUM_STRING_CASE( VK_ERROR_FEATURE_NOT_PRESENT );
		ENUM_STRING_CASE( VK_ERROR_INCOMPATIBLE_DRIVER );
		ENUM_STRING_CASE( VK_ERROR_TOO_MANY_OBJECTS );
		ENUM_STRING_CASE( VK_ERROR_FORMAT_NOT_SUPPORTED );
		ENUM_STRING_CASE( VK_ERROR_SURFACE_LOST_KHR );
		ENUM_STRING_CASE( VK_SUBOPTIMAL_KHR );
		ENUM_STRING_CASE( VK_ERROR_OUT_OF_DATE_KHR );
		ENUM_STRING_CASE( VK_ERROR_INCOMPATIBLE_DISPLAY_KHR );
		ENUM_STRING_CASE( VK_ERROR_NATIVE_WINDOW_IN_USE_KHR );
		ENUM_STRING_CASE( VK_ERROR_VALIDATION_FAILED_EXT );
		default:
		{
			if ( result == VK_ERROR_INVALID_SHADER_NV )
			{
				return "VK_ERROR_INVALID_SHADER_NV";
			}
			return "unknown";
		}
	}
}

static void VkCheckErrors( VkResult result, const char * function )
{
	if ( result != VK_SUCCESS )
	{
		Error( "Vulkan error: %s: %s\n", function, VkErrorString( result ) );
	}
}

/*
================================================================================================================================

Driver Instance.

ksDriverInstance

static bool ksDriverInstance_Create( ksDriverInstance * intance );
static void ksDriverInstance_Destroy( ksDriverInstance * instance );

================================================================================================================================
*/

typedef struct
{
	void *												loader;
	VkInstance											instance;

	// Global functions.
	PFN_vkGetInstanceProcAddr							vkGetInstanceProcAddr;
	PFN_vkEnumerateInstanceLayerProperties				vkEnumerateInstanceLayerProperties;
	PFN_vkEnumerateInstanceExtensionProperties			vkEnumerateInstanceExtensionProperties;
	PFN_vkCreateInstance								vkCreateInstance;

	// Instance functions.
	PFN_vkDestroyInstance								vkDestroyInstance;
	PFN_vkEnumeratePhysicalDevices						vkEnumeratePhysicalDevices;
	PFN_vkGetPhysicalDeviceFeatures						vkGetPhysicalDeviceFeatures;
	PFN_vkGetPhysicalDeviceProperties					vkGetPhysicalDeviceProperties;
	PFN_vkGetPhysicalDeviceMemoryProperties				vkGetPhysicalDeviceMemoryProperties;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties		vkGetPhysicalDeviceQueueFamilyProperties;
	PFN_vkGetPhysicalDeviceFormatProperties				vkGetPhysicalDeviceFormatProperties;
	PFN_vkGetPhysicalDeviceImageFormatProperties		vkGetPhysicalDeviceImageFormatProperties;
	PFN_vkGetPhysicalDeviceSparseImageFormatProperties	vkGetPhysicalDeviceSparseImageFormatProperties;
	PFN_vkEnumerateDeviceExtensionProperties			vkEnumerateDeviceExtensionProperties;
	PFN_vkEnumerateDeviceLayerProperties				vkEnumerateDeviceLayerProperties;
} ksDriverInstance;

#define GET_INSTANCE_PROC_ADDR_EXP( function )	instance->function = (PFN_##function)( instance->vkGetInstanceProcAddr( instance->instance, #function ) ); assert( instance->function != NULL );
#define GET_INSTANCE_PROC_ADDR( function )		GET_INSTANCE_PROC_ADDR_EXP( function )

static bool ksDriverInstance_Create( ksDriverInstance * instance )
{
	memset( instance, 0, sizeof( ksDriverInstance ) );

#if defined( OS_WINDOWS )
	instance->loader = LoadLibrary( TEXT( VULKAN_LOADER ) );
	if ( instance->loader == NULL )
	{
		char buffer[1024];
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), buffer, sizeof( buffer ), NULL );
		Error( "%s not available: %s", VULKAN_LOADER, buffer );
		return false;
	}
	instance->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress( instance->loader, "vkGetInstanceProcAddr" );
	instance->vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)GetProcAddress( instance->loader, "vkEnumerateInstanceLayerProperties" );
	instance->vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)GetProcAddress( instance->loader, "vkEnumerateInstanceExtensionProperties" );
	instance->vkCreateInstance = (PFN_vkCreateInstance)GetProcAddress( instance->loader, "vkCreateInstance" );
#elif defined( OS_LINUX ) || defined( OS_ANDROID ) || defined( OS_APPLE )
	instance->loader = dlopen( VULKAN_LOADER, RTLD_NOW | RTLD_LOCAL );
	if ( instance->loader == NULL )
	{
		Error( "%s not available: %s", VULKAN_LOADER, dlerror() );
		return false;
	}
	instance->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym( instance->loader, "vkGetInstanceProcAddr" );
	instance->vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)dlsym( instance->loader, "vkEnumerateInstanceLayerProperties" );
	instance->vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)dlsym( instance->loader, "vkEnumerateInstanceExtensionProperties" );
	instance->vkCreateInstance = (PFN_vkCreateInstance)dlsym( instance->loader, "vkCreateInstance" );
#else
	instance->vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	GET_INSTANCE_PROC_ADDR( vkEnumerateInstanceLayerProperties );
	GET_INSTANCE_PROC_ADDR( vkEnumerateInstanceExtensionProperties );
	GET_INSTANCE_PROC_ADDR( vkCreateInstance );
#endif

	VkApplicationInfo app;
	app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pNext = NULL;
	app.pApplicationName = APPLICATION_NAME;
	app.applicationVersion = 0;
	app.pEngineName = APPLICATION_NAME;
	app.engineVersion = 0;
	app.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &app;
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.ppEnabledLayerNames = NULL;
	instanceCreateInfo.enabledExtensionCount = 0;
	instanceCreateInfo.ppEnabledExtensionNames = NULL;

	VK( instance->vkCreateInstance( &instanceCreateInfo, VK_ALLOCATOR, &instance->instance ) );

	GET_INSTANCE_PROC_ADDR( vkDestroyInstance );
	GET_INSTANCE_PROC_ADDR( vkEnumeratePhysicalDevices );
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceFeatures );
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceProperties );
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceMemoryProperties );
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceQueueFamilyProperties );
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceFormatProperties );
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceImageFormatProperties );
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceSparseImageFormatProperties );
	GET_INSTANCE_PROC_ADDR( vkEnumerateDeviceExtensionProperties );
	GET_INSTANCE_PROC_ADDR( vkEnumerateDeviceLayerProperties );

	return true;
}


static void ksDriverInstance_Destroy( ksDriverInstance * instance )
{
	VC( instance->vkDestroyInstance( instance->instance, VK_ALLOCATOR ) );

	if ( instance->loader != NULL )
	{
#if defined( OS_WINDOWS )
		FreeLibrary( instance->loader );
#elif defined( OS_LINUX ) || defined( OS_ANDROID )
		dlclose( instance->loader );
#endif
	}
}

/*
================================================================================================================================

Print Driver Info

================================================================================================================================
*/

#define COLUMN_WIDTH				"50"

#define FORMAT_int32_t				"%d"
#define FORMAT_int64_t				"%lld"
#define FORMAT_uint32_t				"%u"
#define FORMAT_uint64_t				"%llu"
#define FORMAT_size_t				"%d"
#define FORMAT_float				"%f"
#define FORMAT_VkBool32				"%u"	// uint32_t
#define FORMAT_VkDeviceSize			"%u"	// uint32_t
#define FORMAT_VkSampleCountFlags	"%u"	// uint32_t

#define PRINT_STRUCT_MEMBER( struct_, member_, format_ ) \
	Print( "%-"COLUMN_WIDTH"s: "format_"\n", #member_, struct_->member_ );

static const char * GetFormatString( VkFormat format )
{
	switch ( format )
	{
		ENUM_STRING_CASE( VK_FORMAT_UNDEFINED );
		ENUM_STRING_CASE( VK_FORMAT_R4G4_UNORM_PACK8 );
		ENUM_STRING_CASE( VK_FORMAT_R4G4B4A4_UNORM_PACK16 );
		ENUM_STRING_CASE( VK_FORMAT_B4G4R4A4_UNORM_PACK16 );
		ENUM_STRING_CASE( VK_FORMAT_R5G6B5_UNORM_PACK16 );
		ENUM_STRING_CASE( VK_FORMAT_B5G6R5_UNORM_PACK16 );
		ENUM_STRING_CASE( VK_FORMAT_R5G5B5A1_UNORM_PACK16 );
		ENUM_STRING_CASE( VK_FORMAT_B5G5R5A1_UNORM_PACK16 );
		ENUM_STRING_CASE( VK_FORMAT_A1R5G5B5_UNORM_PACK16 );
		ENUM_STRING_CASE( VK_FORMAT_R8_UNORM );
		ENUM_STRING_CASE( VK_FORMAT_R8_SNORM );
		ENUM_STRING_CASE( VK_FORMAT_R8_USCALED );
		ENUM_STRING_CASE( VK_FORMAT_R8_SSCALED );
		ENUM_STRING_CASE( VK_FORMAT_R8_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R8_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R8_SRGB );
		ENUM_STRING_CASE( VK_FORMAT_R8G8_UNORM );
		ENUM_STRING_CASE( VK_FORMAT_R8G8_SNORM );
		ENUM_STRING_CASE( VK_FORMAT_R8G8_USCALED );
		ENUM_STRING_CASE( VK_FORMAT_R8G8_SSCALED );
		ENUM_STRING_CASE( VK_FORMAT_R8G8_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R8G8_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R8G8_SRGB );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8_UNORM );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8_SNORM );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8_USCALED );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8_SSCALED );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8_SRGB );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8_UNORM );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8_SNORM );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8_USCALED );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8_SSCALED );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8_UINT );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8_SINT );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8_SRGB );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8A8_UNORM );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8A8_SNORM );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8A8_USCALED );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8A8_SSCALED );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8A8_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8A8_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R8G8B8A8_SRGB );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8A8_UNORM );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8A8_SNORM );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8A8_USCALED );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8A8_SSCALED );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8A8_UINT );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8A8_SINT );
		ENUM_STRING_CASE( VK_FORMAT_B8G8R8A8_SRGB );
		ENUM_STRING_CASE( VK_FORMAT_A8B8G8R8_UNORM_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A8B8G8R8_SNORM_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A8B8G8R8_USCALED_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A8B8G8R8_SSCALED_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A8B8G8R8_UINT_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A8B8G8R8_SINT_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A8B8G8R8_SRGB_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A2R10G10B10_UNORM_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A2R10G10B10_SNORM_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A2R10G10B10_USCALED_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A2R10G10B10_SSCALED_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A2R10G10B10_UINT_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A2R10G10B10_SINT_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A2B10G10R10_UNORM_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A2B10G10R10_SNORM_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A2B10G10R10_USCALED_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A2B10G10R10_SSCALED_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A2B10G10R10_UINT_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_A2B10G10R10_SINT_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_R16_UNORM );
		ENUM_STRING_CASE( VK_FORMAT_R16_SNORM );
		ENUM_STRING_CASE( VK_FORMAT_R16_USCALED );
		ENUM_STRING_CASE( VK_FORMAT_R16_SSCALED );
		ENUM_STRING_CASE( VK_FORMAT_R16_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R16_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R16_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_R16G16_UNORM );
		ENUM_STRING_CASE( VK_FORMAT_R16G16_SNORM );
		ENUM_STRING_CASE( VK_FORMAT_R16G16_USCALED );
		ENUM_STRING_CASE( VK_FORMAT_R16G16_SSCALED );
		ENUM_STRING_CASE( VK_FORMAT_R16G16_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R16G16_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R16G16_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16_UNORM );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16_SNORM );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16_USCALED );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16_SSCALED );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16A16_UNORM );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16A16_SNORM );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16A16_USCALED );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16A16_SSCALED );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16A16_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16A16_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R16G16B16A16_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_R32_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R32_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R32_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_R32G32_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R32G32_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R32G32_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_R32G32B32_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R32G32B32_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R32G32B32_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_R32G32B32A32_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R32G32B32A32_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R32G32B32A32_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_R64_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R64_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R64_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_R64G64_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R64G64_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R64G64_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_R64G64B64_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R64G64B64_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R64G64B64_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_R64G64B64A64_UINT );
		ENUM_STRING_CASE( VK_FORMAT_R64G64B64A64_SINT );
		ENUM_STRING_CASE( VK_FORMAT_R64G64B64A64_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_B10G11R11_UFLOAT_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_D16_UNORM );
		ENUM_STRING_CASE( VK_FORMAT_X8_D24_UNORM_PACK32 );
		ENUM_STRING_CASE( VK_FORMAT_D32_SFLOAT );
		ENUM_STRING_CASE( VK_FORMAT_S8_UINT );
		ENUM_STRING_CASE( VK_FORMAT_D16_UNORM_S8_UINT );
		ENUM_STRING_CASE( VK_FORMAT_D24_UNORM_S8_UINT );
		ENUM_STRING_CASE( VK_FORMAT_D32_SFLOAT_S8_UINT );
		ENUM_STRING_CASE( VK_FORMAT_BC1_RGB_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC1_RGB_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC1_RGBA_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC1_RGBA_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC2_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC2_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC3_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC3_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC4_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC4_SNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC5_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC5_SNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC6H_UFLOAT_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC6H_SFLOAT_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC7_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_BC7_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_EAC_R11_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_EAC_R11_SNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_EAC_R11G11_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_EAC_R11G11_SNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_4x4_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_4x4_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_5x4_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_5x4_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_5x5_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_5x5_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_6x5_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_6x5_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_6x6_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_6x6_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_8x5_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_8x5_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_8x6_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_8x6_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_8x8_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_8x8_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_10x5_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_10x5_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_10x6_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_10x6_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_10x8_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_10x8_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_10x10_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_10x10_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_12x10_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_12x10_SRGB_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_12x12_UNORM_BLOCK );
		ENUM_STRING_CASE( VK_FORMAT_ASTC_12x12_SRGB_BLOCK );
		default: return NULL;
	}
}

static const char * GetFormatFeatureFlagString( VkFormatFeatureFlags flag )
{
	switch ( flag )
	{
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_BLIT_SRC_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_BLIT_DST_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT );
		ENUM_STRING_CASE( VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG );
		default: return NULL;
	}
}

static const char * GetMemoryPropertyFlagString( const VkMemoryPropertyFlagBits flag )
{
	switch ( flag )
	{
		ENUM_STRING_CASE( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		ENUM_STRING_CASE( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
		ENUM_STRING_CASE( VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
		ENUM_STRING_CASE( VK_MEMORY_PROPERTY_HOST_CACHED_BIT );
		ENUM_STRING_CASE( VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT );
		ENUM_STRING_CASE( VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM );
		default: return NULL;
	}
}

static const char * GetMemoryHeapFlagString( const VkMemoryHeapFlags flag )
{
	switch ( flag )
	{
		ENUM_STRING_CASE( VK_MEMORY_HEAP_DEVICE_LOCAL_BIT );
		default: return NULL;
	}
}

static void PrintDeviceLimits( const VkPhysicalDeviceLimits * limit )
{
	PRINT_STRUCT_MEMBER( limit, maxImageDimension1D, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxImageDimension2D, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxImageDimension3D, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxImageDimensionCube, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxImageArrayLayers, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxTexelBufferElements, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxUniformBufferRange, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxStorageBufferRange, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxPushConstantsSize, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxMemoryAllocationCount, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxSamplerAllocationCount, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, bufferImageGranularity, FORMAT_VkDeviceSize );
	PRINT_STRUCT_MEMBER( limit, sparseAddressSpaceSize, FORMAT_VkDeviceSize );
	PRINT_STRUCT_MEMBER( limit, maxBoundDescriptorSets, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxPerStageDescriptorSamplers, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxPerStageDescriptorUniformBuffers, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxPerStageDescriptorStorageBuffers, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxPerStageDescriptorSampledImages, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxPerStageDescriptorStorageImages, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxPerStageDescriptorInputAttachments, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxPerStageResources, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxDescriptorSetSamplers, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxDescriptorSetUniformBuffers, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxDescriptorSetUniformBuffersDynamic, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxDescriptorSetStorageBuffers, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxDescriptorSetStorageBuffersDynamic, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxDescriptorSetSampledImages, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxDescriptorSetStorageImages, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxDescriptorSetInputAttachments, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxVertexInputAttributes, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxVertexInputBindings, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxVertexInputAttributeOffset, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxVertexInputBindingStride, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxVertexOutputComponents, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxTessellationGenerationLevel, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxTessellationPatchSize, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxTessellationControlPerVertexInputComponents, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxTessellationControlPerVertexOutputComponents, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxTessellationControlPerPatchOutputComponents, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxTessellationControlTotalOutputComponents, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxTessellationEvaluationInputComponents, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxTessellationEvaluationOutputComponents, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxGeometryShaderInvocations, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxGeometryInputComponents, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxGeometryOutputComponents, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxGeometryOutputVertices, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxGeometryTotalOutputComponents, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxFragmentInputComponents, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxFragmentOutputAttachments, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxFragmentDualSrcAttachments, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxFragmentCombinedOutputResources, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxComputeSharedMemorySize, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxComputeWorkGroupCount[0], FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxComputeWorkGroupCount[1], FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxComputeWorkGroupCount[2], FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxComputeWorkGroupInvocations, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxComputeWorkGroupSize[0], FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxComputeWorkGroupSize[1], FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxComputeWorkGroupSize[2], FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, subPixelPrecisionBits, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, subTexelPrecisionBits, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, mipmapPrecisionBits, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxDrawIndexedIndexValue, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxDrawIndirectCount, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxSamplerLodBias, FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, maxSamplerAnisotropy, FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, maxViewports, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxViewportDimensions[0], FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxViewportDimensions[1], FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, viewportBoundsRange[0], FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, viewportBoundsRange[1], FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, viewportSubPixelBits, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, minMemoryMapAlignment, FORMAT_size_t );
	PRINT_STRUCT_MEMBER( limit, minTexelBufferOffsetAlignment, FORMAT_VkDeviceSize );
	PRINT_STRUCT_MEMBER( limit, minUniformBufferOffsetAlignment, FORMAT_VkDeviceSize );
	PRINT_STRUCT_MEMBER( limit, minStorageBufferOffsetAlignment, FORMAT_VkDeviceSize );
	PRINT_STRUCT_MEMBER( limit, minTexelOffset, FORMAT_int32_t );
	PRINT_STRUCT_MEMBER( limit, maxTexelOffset, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, minTexelGatherOffset, FORMAT_int32_t );
	PRINT_STRUCT_MEMBER( limit, maxTexelGatherOffset, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, minInterpolationOffset, FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, maxInterpolationOffset, FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, subPixelInterpolationOffsetBits, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxFramebufferWidth, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxFramebufferHeight, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxFramebufferLayers, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, framebufferColorSampleCounts, FORMAT_VkSampleCountFlags );
	PRINT_STRUCT_MEMBER( limit, framebufferDepthSampleCounts, FORMAT_VkSampleCountFlags );
	PRINT_STRUCT_MEMBER( limit, framebufferStencilSampleCounts, FORMAT_VkSampleCountFlags );
	PRINT_STRUCT_MEMBER( limit, framebufferNoAttachmentsSampleCounts, FORMAT_VkSampleCountFlags );
	PRINT_STRUCT_MEMBER( limit, maxColorAttachments, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, sampledImageColorSampleCounts, FORMAT_VkSampleCountFlags );
	PRINT_STRUCT_MEMBER( limit, sampledImageIntegerSampleCounts, FORMAT_VkSampleCountFlags );
	PRINT_STRUCT_MEMBER( limit, sampledImageDepthSampleCounts, FORMAT_VkSampleCountFlags );
	PRINT_STRUCT_MEMBER( limit, sampledImageStencilSampleCounts, FORMAT_VkSampleCountFlags );
	PRINT_STRUCT_MEMBER( limit, storageImageSampleCounts, FORMAT_VkSampleCountFlags );
	PRINT_STRUCT_MEMBER( limit, maxSampleMaskWords, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, timestampComputeAndGraphics, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( limit, timestampPeriod, FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, maxClipDistances, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxCullDistances, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, maxCombinedClipAndCullDistances, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, discreteQueuePriorities, FORMAT_uint32_t );
	PRINT_STRUCT_MEMBER( limit, pointSizeRange[0], FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, pointSizeRange[1], FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, lineWidthRange[0], FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, lineWidthRange[1], FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, pointSizeGranularity, FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, lineWidthGranularity, FORMAT_float );
	PRINT_STRUCT_MEMBER( limit, strictLines, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( limit, standardSampleLocations, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( limit, optimalBufferCopyOffsetAlignment, FORMAT_VkDeviceSize );
	PRINT_STRUCT_MEMBER( limit, optimalBufferCopyRowPitchAlignment, FORMAT_VkDeviceSize );
	PRINT_STRUCT_MEMBER( limit, nonCoherentAtomSize, FORMAT_VkDeviceSize );
}

static void PrintDeviceSparseProperties( const VkPhysicalDeviceSparseProperties * sparseProperties )
{
	PRINT_STRUCT_MEMBER( sparseProperties, residencyStandard2DBlockShape, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( sparseProperties, residencyStandard2DMultisampleBlockShape, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( sparseProperties, residencyStandard3DBlockShape, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( sparseProperties, residencyAlignedMipSize, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( sparseProperties, residencyNonResidentStrict, FORMAT_VkBool32 );
}

static void PrintDeviceFeatures( const VkPhysicalDeviceFeatures * deviceFeatures )
{
	PRINT_STRUCT_MEMBER( deviceFeatures, robustBufferAccess, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, fullDrawIndexUint32, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, imageCubeArray, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, independentBlend, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, geometryShader, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, tessellationShader, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, sampleRateShading, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, dualSrcBlend, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, logicOp, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, multiDrawIndirect, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, drawIndirectFirstInstance, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, depthClamp, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, depthBiasClamp, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, fillModeNonSolid, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, depthBounds, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, wideLines, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, largePoints, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, alphaToOne, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, multiViewport, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, samplerAnisotropy, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, textureCompressionETC2, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, textureCompressionASTC_LDR, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, textureCompressionBC, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, occlusionQueryPrecise, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, pipelineStatisticsQuery, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, vertexPipelineStoresAndAtomics, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, fragmentStoresAndAtomics, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderTessellationAndGeometryPointSize, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderImageGatherExtended, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderStorageImageExtendedFormats, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderStorageImageMultisample, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderStorageImageReadWithoutFormat, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderStorageImageWriteWithoutFormat, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderUniformBufferArrayDynamicIndexing, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderSampledImageArrayDynamicIndexing, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderStorageBufferArrayDynamicIndexing, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderStorageImageArrayDynamicIndexing, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderClipDistance, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderCullDistance, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderFloat64, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderInt64, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderInt16, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderResourceResidency, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, shaderResourceMinLod, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, sparseBinding, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, sparseResidencyBuffer, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, sparseResidencyImage2D, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, sparseResidencyImage3D, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, sparseResidency2Samples, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, sparseResidency4Samples, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, sparseResidency8Samples, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, sparseResidency16Samples, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, sparseResidencyAliased, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, variableMultisampleRate, FORMAT_VkBool32 );
	PRINT_STRUCT_MEMBER( deviceFeatures, inheritedQueries, FORMAT_VkBool32 );
}

int main( int argc, char * argv[] )
{
	UNUSED_PARM( argc );
	UNUSED_PARM( argv );

	Console_Resize( 4096, 120 );

	ksDriverInstance instance;
	ksDriverInstance_Create( &instance );

	const uint32_t headerMajor = VK_VERSION_MAJOR( VK_API_VERSION_1_0 );
	const uint32_t headerMinor = VK_VERSION_MINOR( VK_API_VERSION_1_0 );
	const uint32_t headerPatch = VK_VERSION_PATCH( VK_API_VERSION_1_0 );

	Print( "--------------------------------\n" );
	Print( "%-"COLUMN_WIDTH"s: %s\n", "OS", GetOSVersion() );
	Print( "%-"COLUMN_WIDTH"s: %s\n", "CPU", GetCPUVersion() );
	Print( "%-"COLUMN_WIDTH"s: %d.%d.%d\n", "Instance API version", headerMajor, headerMinor, headerPatch );

	// Instance Extensions.
	{
		uint32_t availableExtensionCount = 0;
		VK( instance.vkEnumerateInstanceExtensionProperties( NULL, &availableExtensionCount, NULL ) );

		VkExtensionProperties * availableExtensions = malloc( availableExtensionCount * sizeof( VkExtensionProperties ) );
		VK( instance.vkEnumerateInstanceExtensionProperties( NULL, &availableExtensionCount, availableExtensions ) );

		for ( uint32_t i = 0; i < availableExtensionCount; i++ )
		{
			Print( "%-"COLUMN_WIDTH"s%c %s\n", ( i == 0 ? "Instance Extensions" : "" ), ( i == 0 ? ':' : ' ' ), availableExtensions[i].extensionName );
		}

		free( availableExtensions );
	}

	// Instance Layers.
	{
		uint32_t availableLayerCount = 0;
		VK( instance.vkEnumerateInstanceLayerProperties( &availableLayerCount, NULL ) );

		VkLayerProperties * availableLayers = malloc( availableLayerCount * sizeof( VkLayerProperties ) );
		VK( instance.vkEnumerateInstanceLayerProperties( &availableLayerCount, availableLayers ) );

		for ( uint32_t i = 0; i < availableLayerCount; i++ )
		{
			Print( "%-"COLUMN_WIDTH"s%c %s\n", ( i == 0 ? "Instance Layers" : "" ), ( i == 0 ? ':' : ' ' ), availableLayers[i].layerName );
		}

		free( availableLayers );
	}

	// Physical Devices.
	{
		uint32_t physicalDeviceCount = 0;
		VK( instance.vkEnumeratePhysicalDevices( instance.instance, &physicalDeviceCount, NULL ) );

		VkPhysicalDevice * physicalDevices = (VkPhysicalDevice *) malloc( physicalDeviceCount * sizeof( VkPhysicalDevice ) );
		VK( instance.vkEnumeratePhysicalDevices( instance.instance, &physicalDeviceCount, physicalDevices ) );

		for ( uint32_t physicalDeviceIndex = 0; physicalDeviceIndex < physicalDeviceCount; physicalDeviceIndex++ )
		{
			VkPhysicalDeviceFeatures physicalDeviceFeatures;
			VC( instance.vkGetPhysicalDeviceFeatures( physicalDevices[physicalDeviceIndex], &physicalDeviceFeatures ) );

			VkPhysicalDeviceProperties physicalDeviceProperties;
			VC( instance.vkGetPhysicalDeviceProperties( physicalDevices[physicalDeviceIndex], &physicalDeviceProperties ) );

			VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
			VC( instance.vkGetPhysicalDeviceMemoryProperties( physicalDevices[physicalDeviceIndex], &physicalDeviceMemoryProperties ) );

			const uint32_t driverMajor = VK_VERSION_MAJOR( physicalDeviceProperties.driverVersion );
			const uint32_t driverMinor = VK_VERSION_MINOR( physicalDeviceProperties.driverVersion );
			const uint32_t driverPatch = VK_VERSION_PATCH( physicalDeviceProperties.driverVersion );

			const uint32_t apiMajor = VK_VERSION_MAJOR( physicalDeviceProperties.apiVersion );
			const uint32_t apiMinor = VK_VERSION_MINOR( physicalDeviceProperties.apiVersion );
			const uint32_t apiPatch = VK_VERSION_PATCH( physicalDeviceProperties.apiVersion );

			// Device Info.
			Print( "--------------------------------\n" );
			Print( "%-"COLUMN_WIDTH"s: %s\n", "Device Name", physicalDeviceProperties.deviceName );
			Print( "%-"COLUMN_WIDTH"s: %s\n", "Device Type",	( ( physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ) ? "integrated GPU" :
																( ( physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) ? "discrete GPU" :
																( ( physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ) ? "virtual GPU" :
																( ( physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ) ? "CPU" : "unknown" ) ) ) ) );
			Print( "%-"COLUMN_WIDTH"s: 0x%04X\n", "Vendor ID", physicalDeviceProperties.vendorID );		// http://pcidatabase.com
			Print( "%-"COLUMN_WIDTH"s: 0x%04X\n", "Device ID", physicalDeviceProperties.deviceID );
			Print( "%-"COLUMN_WIDTH"s: %d.%d.%d\n", "Driver Version", driverMajor, driverMinor, driverPatch );
			Print( "%-"COLUMN_WIDTH"s: %d.%d.%d\n", "API Version", apiMajor, apiMinor, apiPatch );

			// Device Queue Families.
			{
				uint32_t queueFamilyCount = 0;
				VC( instance.vkGetPhysicalDeviceQueueFamilyProperties( physicalDevices[physicalDeviceIndex], &queueFamilyCount, NULL ) );

				VkQueueFamilyProperties * queueFamilyProperties = (VkQueueFamilyProperties *) malloc( queueFamilyCount * sizeof( VkQueueFamilyProperties ) );
				VC( instance.vkGetPhysicalDeviceQueueFamilyProperties( physicalDevices[physicalDeviceIndex], &queueFamilyCount, queueFamilyProperties ) );

				for ( uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++ )
				{
					const VkQueueFlags queueFlags = queueFamilyProperties[queueFamilyIndex].queueFlags;
					Print( "%-"COLUMN_WIDTH"s%c %d =%s%s%s (%d queues, %d priorities)\n",
									( queueFamilyIndex == 0 ? "Queue Families" : "" ),
									( queueFamilyIndex == 0 ? ':' : ' ' ),
									queueFamilyIndex,
									( queueFlags & VK_QUEUE_GRAPHICS_BIT ) ? " graphics" : "",
									( queueFlags & VK_QUEUE_COMPUTE_BIT )  ? " compute"  : "",
									( queueFlags & VK_QUEUE_TRANSFER_BIT ) ? " transfer" : "",
									queueFamilyProperties[queueFamilyIndex].queueCount,
									physicalDeviceProperties.limits.discreteQueuePriorities );
				}

				free( queueFamilyProperties );
			}

			// Device Extensions.
			{
				uint32_t availableExtensionCount = 0;
				VK( instance.vkEnumerateDeviceExtensionProperties( physicalDevices[physicalDeviceIndex], NULL, &availableExtensionCount, NULL ) );

				VkExtensionProperties * availableExtensions = malloc( availableExtensionCount * sizeof( VkExtensionProperties ) );
				VK( instance.vkEnumerateDeviceExtensionProperties( physicalDevices[physicalDeviceIndex], NULL, &availableExtensionCount, availableExtensions ) );

				for ( uint32_t i = 0; i < availableExtensionCount; i++ )
				{
					Print( "%-"COLUMN_WIDTH"s%c %s\n", ( i == 0 ? "Device Extensions" : "" ), ( i == 0 ? ':' : ' ' ), availableExtensions[i].extensionName );
				}

				free( availableExtensions );
			}

			// Device Layers.
			{
				uint32_t availableLayerCount = 0;
				VK( instance.vkEnumerateDeviceLayerProperties( physicalDevices[physicalDeviceIndex], &availableLayerCount, NULL ) );

				VkLayerProperties * availableLayers = malloc( availableLayerCount * sizeof( VkLayerProperties ) );
				VK( instance.vkEnumerateDeviceLayerProperties( physicalDevices[physicalDeviceIndex], &availableLayerCount, availableLayers ) );

				for ( uint32_t i = 0; i < availableLayerCount; i++ )
				{
					Print( "%-"COLUMN_WIDTH"s%c %s\n", ( i == 0 ? "Device Layers" : "" ), ( i == 0 ? ':' : ' ' ), availableLayers[i].layerName );
				}

				free( availableLayers );
			}

			Print( "--------------------------------\n" );

			// Device Limits.
			PrintDeviceLimits( &physicalDeviceProperties.limits );

			Print( "--------------------------------\n" );

			// Device Sparse Properties.
			PrintDeviceSparseProperties( &physicalDeviceProperties.sparseProperties );

			Print( "--------------------------------\n" );

			// Device Features.
			PrintDeviceFeatures( &physicalDeviceFeatures );

			Print( "--------------------------------\n" );

			// Device Memory Types.
			for ( uint32_t typeIndex = 0; typeIndex < physicalDeviceMemoryProperties.memoryTypeCount; typeIndex++ )
			{
				char label[128] = { 0 };

				sprintf( label, "memoryTypes[%u].heapIndex", typeIndex );
				Print( "%-"COLUMN_WIDTH"s: %u\n", label, physicalDeviceMemoryProperties.memoryTypes[typeIndex].heapIndex );

				sprintf( label, "memoryTypes[%u].propertyFlags", typeIndex );
				bool first = true;
				for ( uint32_t bit = 0; bit < 32; bit++ )
				{
					const VkFlags flag = ( 1 << bit );
					const char * propertyString = GetMemoryPropertyFlagString( (VkMemoryPropertyFlagBits) flag );
					if ( propertyString != NULL && ( physicalDeviceMemoryProperties.memoryTypes[typeIndex].propertyFlags & flag ) != 0 )
					{
						Print( "%-"COLUMN_WIDTH"s%c %s\n", ( first ? label : "" ), ( first ? ':' : ' ' ), propertyString );
						first = false;
					}
				}
				if ( first )
				{
					Print( "%-"COLUMN_WIDTH"s: %s\n", label, "-" );
				}
			}

			Print( "--------------------------------\n" );

			// Device Memory Heaps.
			for ( uint32_t heapIndex = 0; heapIndex < physicalDeviceMemoryProperties.memoryHeapCount; heapIndex++ )
			{
				char label[128] = { 0 };

				sprintf( label, "memoryHeaps[%u].size", heapIndex );
				Print( "%-"COLUMN_WIDTH"s: %u\n", label, physicalDeviceMemoryProperties.memoryHeaps[heapIndex].size );

				sprintf( label, "memoryHeaps[%u].flags", heapIndex );
				bool first = true;
				for ( uint32_t bit = 0; bit < 32; bit++ )
				{
					const VkFlags flag = ( 1 << bit );
					const char * heapFlagString = GetMemoryHeapFlagString( (VkMemoryHeapFlags) flag );
					if ( heapFlagString != NULL && ( physicalDeviceMemoryProperties.memoryHeaps[heapIndex].flags & ( 1 << flag ) ) != 0 )
					{
						Print( "%-"COLUMN_WIDTH"s%c %s\n", ( first ? label : "" ), ( first ? ':' : ' ' ), heapFlagString );
						first = false;
					}
				}
				if ( first )
				{
					Print( "%-"COLUMN_WIDTH"s: %s\n", label, "-" );
				}
			}

			Print( "--------------------------------\n" );

			// Device Format properties.
			for ( uint32_t format = VK_FORMAT_BEGIN_RANGE + 1; format <= VK_FORMAT_END_RANGE; format++ )
			{
				VkFormatProperties formatProperties;
				VC( instance.vkGetPhysicalDeviceFormatProperties( physicalDevices[physicalDeviceIndex], format, &formatProperties ) );

				const char * formatString = GetFormatString( format );
				assert( formatString != NULL );
				if ( formatString == NULL )
				{
					continue;
				}

				for ( uint32_t bit = 0; bit < 32; bit++ )
				{
					const VkFlags flag = ( 1 << bit );
					const char * flagString = GetFormatFeatureFlagString( (VkFormatFeatureFlags) flag );
					if ( flagString != NULL )
					{
						Print( "%-"COLUMN_WIDTH"s%c %-54s  %-6s  %-6s  %-6s\n",
								( bit == 0 ? formatString : "" ),
								( bit == 0 ? ':' : ' ' ),
								flagString,
								( ( formatProperties.optimalTilingFeatures & flag ) != 0 ? "tiling" : "-" ),
								( ( formatProperties.linearTilingFeatures & flag ) != 0 ? "linear" : "-" ),
								( ( formatProperties.bufferFeatures & flag ) != 0 ? "buffer" : "-" ) );
					}
				}
			}
			
			// vkGetPhysicalDeviceImageFormatProperties
			// vkGetPhysicalDeviceSparseImageFormatProperties
		}

		free( physicalDevices );
	}

	Print( "--------------------------------\n" );

	ksDriverInstance_Destroy( &instance );

#if defined( OS_WINDOWS )
	Print( "Press any key to continue.\n" );
	_getch();
#endif
}

#if defined( OS_ANDROID )

static void app_handle_cmd( struct android_app * app, int32_t cmd )
{
	switch ( cmd )
	{
		// There is no APP_CMD_CREATE. The ANativeActivity creates the
		// application thread from onCreate(). The application thread
		// then calls android_main().
		case APP_CMD_START:
		{
			Print( "onStart()" );
			Print( "    APP_CMD_START" );
			break;
		}
		case APP_CMD_RESUME:
		{
			Print( "onResume()" );
			Print( "    APP_CMD_RESUME" );
			main( 0, NULL );
			ANativeActivity_finish( app->activity );
			break;
		}
		case APP_CMD_PAUSE:
		{
			Print( "onPause()" );
			Print( "    APP_CMD_PAUSE" );
			break;
		}
		case APP_CMD_STOP:
		{
			Print( "onStop()" );
			Print( "    APP_CMD_STOP" );
			break;
		}
		case APP_CMD_DESTROY:
		{
			Print( "onDestroy()" );
			Print( "    APP_CMD_DESTROY" );
			break;
		}
		case APP_CMD_INIT_WINDOW:
		{
			Print( "surfaceCreated()" );
			Print( "    APP_CMD_INIT_WINDOW" );
			ANativeActivity_setWindowFlags( app->activity, AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON, 0 );
			break;
		}
		case APP_CMD_TERM_WINDOW:
		{
			Print( "surfaceDestroyed()" );
			Print( "    APP_CMD_TERM_WINDOW" );
			break;
		}
    }
}

void android_main( struct android_app * app )
{
	// Make sure the native app glue is not stripped.
	app_dummy();

	app->userData = NULL;
	app->onAppCmd = app_handle_cmd;
	app->onInputEvent = NULL;

	for ( ; ; )
	{
		int events;
		struct android_poll_source * source;
		const int timeoutMilliseconds = ( app->destroyRequested == 0 ) ? -1 : 0;
		if ( ALooper_pollAll( timeoutMilliseconds, NULL, &events, (void **)&source ) < 0 )
		{
			break;
		}

		if ( source != NULL )
		{
			source->process( app, source );
		}
	}
}

#endif

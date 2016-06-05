/*
================================================================================================

Description	:	Asynchronous Time Warp test utility for Vulkan.
Author		:	J.M.P. van Waveren
Date		:	08/11/2015
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

This implements the simplest form of time warp transform for Vulkan.
This transform corrects for optical aberration of the optics used in a virtual
reality headset, and only rotates the stereoscopic images based on the very latest
head orientation to reduce the motion-to-photon delay (or end-to-end latency).

This utility can be used to test whether or not a particular combination of hardware,
operating system and graphics driver is capable of rendering stereoscopic pairs of
images, while asynchronously (and ideally concurrently) warping the latest pair of
images onto the display, synchronized with the display refresh without dropping any
frames. Under high system load, the rendering of the stereoscopic images is allowed
to drop frames, but the asynchronous time warp must be able to warp the latest
stereoscopic images onto the display, synchronized with the display refresh
*without ever* dropping any frames.

There is one thread that renders the stereoscopic pairs of images by rendering a scene
to two textures, one for each eye. These eye textures are then handed over to the
asynchronous time warp in a thread safe manner. The asynchronous time warp runs in
another thread and continuously takes the last completed eye textures and warps them
onto the display.

Even though rendering commands are issued concurrently from two separate threads,
most current hardware and drivers serialize these rendering commands because the
hardware cannot actually execute multiple graphics/compute tasks concurrently.
Based on how the rendering commands are prioritized and serialized, the asynchronous
time warp may, or may not be able to stay synchronized with the display refresh.

On hardware that cannot execute multiple graphics/compute tasks concurrently, the
following is required to keep the asynchronous time warp synchronized with the
display refresh without dropping frames:

	- Queue priorities.
	- Fine-grained and low latency priority based task switching.

To significantly reduce the latency in a virtual reality simulation, the asynchronous
time warp needs to be scheduled as close as possible to the display refresh.
In addition to the above requirements, the following is required to achieve this:

	- Accurate timing of the display refresh.
	- Predictable latency and throughput of the time warp execution.


PERFORMANCE
===========

When the frame rate drops, it can be hard to tell whether the stereoscopic rendering,
or the time warp rendering drops frames. Therefore four scrolling bar graphs are
drawn at the bottom left of the screen. Each bar represents a frame. New frames scroll
in on the right and old frames scroll out on the left.

The left-most bar graph represent the frame rate of the sterescopic rendering (pink).
The next bar graph represents the frame rate of time warp rendering (green). Each bar
that is pink or green respectively reaches the top of the graph and represents a frame
rendered at the display refresh rate. When the frame rate drops, the bars turn red
and become shorter proportional to how much the frame rate drops.

The next two bar graph shows the CPU and GPU time of the stereoscopic rendering (pink),
the time warp rendering (green) and the bar graph rendering (yellow). The times are
stacked in each graph. The full height of a graph represents a full frame time.
For instance, with a 60Hz display refresh rate, the full graph height represents 16.7
milliseconds.


SCENE WORKLOAD
==============

The graphics work load of the scene that is rendered fer each eye can be changed by
adjusting the number of draw calls, the number of triangles per draw call, and the
fragment program complexity. For each of these there are four levels:

Number of draw calls:
	0: 8
	1: 64
	2: 512
	3: 4096

Number of triangles per draw call:
	0: 12
	1: 128
	2: 512
	3: 2048

Fragment program complexity:
	0: flat-shaded with 1 light
	1: normal-mapped with 100 lights
	2: normal-mapped with 1000 lights
	3: normal-mapped with 2000 lights

In the lower right corner of the screen there are three indicators that show
the current level for each. The levels are colored: 0 = green, 1 = blue,
2 = yellow and 3 = red.

The scene is normally rendered separately for each eye. However, there is also
an option to render the scene only once for both eyes (multi-view). The left-most
small indicator in the middle-bottom of the screen shows whether or not multi-view
is enabled: gray = off and red = on.


TIMEWARP SETTINGS
=================

The time warp can run in two modes. The first mode only corrects for spatial
aberration and the second mode also corrects for chromatic aberration.
The middle small indicator in the middle-bottom of the screen shows which mode
is used: gray = spatial and red = chromatic.

There are two implementations of the time warp. The first implementation uses
the conventional graphics pipeline and the second implementation uses compute.
The right-most small indicator in the middle-bottom of the screen shows which
implementation is used: gray = graphics and red = compute.


COMMAND-LINE INPUT
==================

The following command-line options can be used to change various settings.

	-f			start fullscreen
	-v <s>		start with V-Sync disabled for this many seconds
	-h			start with head rotation disabled
	-p			start with the simulation paused
	-q <0-3>	set per eye draw calls level
	-w <0-3>	set per eye triangles per draw call level
	-e <0-3>	set per eye fragment program complexity level
	-m <0-1>    enable/disable multi-view
	-c <0-1>	enable/disable correction for chromatic aberration
	-r <name>	set the render mode: atw, tw, scene
	-i <name>	set time warp implementation: graphics, compute
	-g			hide graphs
	-l <s>		log 10 frames of Vulkan commands after this many seconds
	-d			dump GLSL to files for conversion to SPIR-V


KEYBOARD INPUT
==============

The following keys can be used at run-time to change various settings.

	[F]		= toggle between windowed and fullscreen
	[V]		= toggle V-Sync
	[H]		= toggle head rotation
	[P]		= pause/unpause the simulation
	[Q]		= cycle per eye draw calls level
	[W]		= cycle per eye triangles per draw call level
	[E]		= cycle per eye fragment program complexity level
	[M]		= toggle multi-view
	[C]		= toggle correction for chromatic aberration
	[R]		= set the render mode: atw, tw, scene
	[I]		= cycle time warp implementations: graphics, compute
	[G]		= cycle between showing graphs, showing paused graphs and hiding graphs
	[L]		= log 10 frames of Vulkan commands
	[D]		= dump GLSL to files for conversion to SPIR-V
	[Esc]	= exit


IMPLEMENTATION
==============

The code is written in an object-oriented style with a focus on minimizing state
and side effects. The majority of the functions manipulate self-contained objects
without modifying any global state. The types introduced in this file have no
circular dependencies, and there are no forward declarations.

Even though an object-oriented style is used, the code is written in straight C99 for
maximum portability and readability. To further improve portability and to simplify
compilation, all source code is in a single file without any dependencies on third-
party code or non-standard libraries. Instead, the code provides direct access to
window creation for driver extension work.

The code is written against version 1.0.2 of the Vulkan Specification.

Supported platforms are:

	- Microsoft Windows 7 or later
	- Apple Mac OS X 10.9 or later
	- Ubuntu Linux 14.04 or later
	- Android 5.0 or later


COMMAND-LINE COMPILATION
========================

Microsoft Windows: Visual Studio 2013 Compiler:
	"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
	cl /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /I%VK_SDK_PATH%\Include atw_vulkan.c /link user32.lib gdi32.lib Advapi32.lib Shlwapi.lib

Microsoft Windows: Intel Compiler 14.0
	"C:\Program Files (x86)\Intel\Composer XE\bin\iclvars.bat" intel64
	icl /Qstd=c99 /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /I%VK_SDK_PATH%\Include atw_vulkan.c /link user32.lib gdi32.lib Advapi32.lib Shlwapi.lib

Apple Mac OS X: Apple LLVM 6.0:
	clang -std=c99 -x objective-c -fno-objc-arc -Wall -g -O2 -m64 -o atw_vulkan atw_vulkan.c -framework Cocoa

Linux: GCC 4.8.2 Xlib:
	sudo apt-get install libx11-dev
	sudo apt-get install libxxf86vm-dev
	sudo apt-get install libxrandr-dev
	gcc -std=c99 -Wall -g -O2 -m64 -o atw_vulkan atw_vulkan.c -lm -lpthread -ldl -lX11 -lXxf86vm -lXrandr

Linux: GCC 4.8.2 XCB:
	sudo apt-get install libxcb1-dev
	sudo apt-get install libxcb-keysyms1-dev
	sudo apt-get install libxcb-icccm4-dev
	gcc -std=c99 -Wall -g -O2 -m64 -o atw_vulkan atw_vulkan.c -lm -lpthread -ldl -lxcb -lxcb-keysyms -lxcb-randr

Android for ARM from Windows: NDK Revision 11c - Android 21 - ANT/Gradle
	ANT:
		cd projects/android/ant/atw_vulkan
		ndk-build
		ant debug
		adb install -r bin/atw_vulkan-debug.apk
	Gradle:
		cd projects/android/gradle/atw_vulkan
		gradlew build
		adb install -r build/outputs/apk/atw_vulkan-arm7-debug.apk


KNOWN ISSUES
============

OS     : Android 6.0.1
GPU    : Adreno (TM) 530
DRIVER : Vulkan 1.0.3 (9.292.2983)
-----------------------------------------------
- Only one queue family with a single queue is supported.


WORK ITEMS
==========

- Implement an extension that provides accurate display refresh timing.
- Improve GPU task switching granularity.


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
	#define OS_MAC
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

	#ifdef _MSC_VER
		#pragma warning( disable : 4204 )	// nonstandard extension used : non-constant aggregate initializer
		#pragma warning( disable : 4255 )	// '<name>' : no function prototype given: converting '()' to '(void)'
		#pragma warning( disable : 4668 )	// '__cplusplus' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
		#pragma warning( disable : 4711 )	// function '<name>' selected for automatic inline expansion
		#pragma warning( disable : 4738 )	// storing 32-bit float result in memory, possible loss of performance
		#pragma warning( disable : 4820 )	// '<name>' : 'X' bytes padding added after data member '<member>'
	#endif

	#include <windows.h>

	#define VK_USE_PLATFORM_WIN32_KHR
	#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME	VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	#define PFN_vkCreateSurfaceKHR					PFN_vkCreateWin32SurfaceKHR
	#define vkCreateSurfaceKHR						vkCreateWin32SurfaceKHR

	#include "vulkan/vulkan.h"
	#include "vulkan/vk_sdk_platform.h"

	#define VULKAN_LOADER	"vulkan-1.dll"
	#define OUTPUT_PATH		""

	#define __thread	__declspec( thread )

#elif defined( OS_MAC )

	#include <sys/param.h>
	#include <sys/sysctl.h>
	#include <sys/time.h>
	#include <pthread.h>
	#include <dlfcn.h>						// for dlopen
	#include <Cocoa/Cocoa.h>

	#include "TargetConditionals.h"
	#if TARGET_OS_IPHONE
		#define VK_USE_PLATFORM_IOS_KHR
		#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME	VK_KHR_IOS_SURFACE_EXTENSION_NAME
		#define PFN_vkCreateSurfaceKHR					PFN_vkCreateIosSurfaceKHR
		#define vkCreateSurfaceKHR						vkCreateIosSurfaceKHR
	#else
		#define VK_USE_PLATFORM_OSX_KHR
		#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME	VK_KHR_OSX_SURFACE_EXTENSION_NAME
		#define PFN_vkCreateSurfaceKHR					PFN_vkCreateOsxSurfaceKHR
		#define vkCreateSurfaceKHR						vkCreateOsxSurfaceKHR
	#endif

	#include "vulkan/vulkan.h"
	#include "vulkan/vk_sdk_platform.h"

	//#define MOLTEN_VK
	#if defined( MOLTEN_VK )
		#import <QuartzCore/CAMetalLayer.h>
	#elif TARGET_OS_IPHONE
		typedef VkFlags VkIosSurfaceCreateFlagsKHR;
		typedef struct VkIos2SurfaceCreateInfoKHR {
			VkStructureType				sType;
			const void *				pNext;
			VkIosSurfaceCreateFlagsKHR	flags;
			NSView *					nsview;
		} VkIosSurfaceCreateInfoKHR;
		#define VK_KHR_IOS_SURFACE_EXTENSION_NAME				"VK_KHR_ios_surface"
		#define VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_KHR	1000015000
		typedef VkResult (VKAPI_PTR *PFN_vkCreateIosSurfaceKHR)(VkInstance instance, const VkIosSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
		#define VULKAN_LOADER									"libvulkan.dylib"
	#else
		typedef VkFlags VkOsxSurfaceCreateFlagsKHR;
		typedef struct VkOsxSurfaceCreateInfoKHR {
			VkStructureType				sType;
			const void *				pNext;
			VkOsxSurfaceCreateFlagsKHR	flags;
			NSView *					nsview;
		} VkOsxSurfaceCreateInfoKHR;
		#define VK_KHR_OSX_SURFACE_EXTENSION_NAME				"VK_KHR_osx_surface"
		#define VK_STRUCTURE_TYPE_OSX_SURFACE_CREATE_INFO_KHR	1000015000
		typedef VkResult (VKAPI_PTR *PFN_vkCreateOsxSurfaceKHR)(VkInstance instance, const VkOsxSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
		#define VULKAN_LOADER									"libvulkan.dylib"
	#endif

	#define OUTPUT_PATH		""

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

	#if defined( OS_LINUX_XLIB )
		#define VK_USE_PLATFORM_XLIB_KHR
		#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME	VK_KHR_XLIB_SURFACE_EXTENSION_NAME
		#define PFN_vkCreateSurfaceKHR					PFN_vkCreateXlibSurfaceKHR
		#define vkCreateSurfaceKHR						vkCreateXlibSurfaceKHR
	#elif defined( OS_LINUX_XCB )
		#define VK_USE_PLATFORM_XCB_KHR
		#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME	VK_KHR_XCB_SURFACE_EXTENSION_NAME
		#define PFN_vkCreateSurfaceKHR					PFN_vkCreateXcbSurfaceKHR
		#define vkCreateSurfaceKHR						vkCreateXcbSurfaceKHR
	#endif

	#include "vulkan/vulkan.h"
	#include "vulkan/vk_sdk_platform.h"

	#define VULKAN_LOADER	"libvulkan-1.so"
	#define OUTPUT_PATH		""

	// prototype is only included when __USE_GNU is defined but that causes other compile errors
	extern int pthread_setname_np( pthread_t __target_thread, __const char *__name );

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

	#define VK_USE_PLATFORM_ANDROID_KHR
	#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME	VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
	#define PFN_vkCreateSurfaceKHR					PFN_vkCreateAndroidSurfaceKHR
	#define vkCreateSurfaceKHR						vkCreateAndroidSurfaceKHR

	#include "vulkan/vulkan.h"
	#include "vulkan/vk_sdk_platform.h"

	#define VULKAN_LOADER	"libvulkan.so"
	#define OUTPUT_PATH		"/sdcard/"

	#pragma GCC diagnostic ignored "-Wunused-function"

	typedef struct
	{
		JavaVM *	vm;			// Java Virtual Machine
		JNIEnv *	env;		// Thread specific environment
		jobject		activity;	// Java activity object
	} Java_t;

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

#define MATH_PI							3.14159265358979323846f

#define UNUSED_PARM( x )				{ (void)(x); }
#define ARRAY_SIZE( a )					( sizeof( (a) ) / sizeof( (a)[0] ) )
#define OFFSETOF_MEMBER( type, member )	(size_t)&((type *)0)->member
#define SIZEOF_MEMBER( type, member )	sizeof( ((type *)0)->member )
#define BIT( x )						( 1 << (x) )
#define ROUNDUP( x, granularity )		( ( (x) + (granularity) - 1 ) & ~( (granularity) - 1 ) )
#define CLAMP( x, min, max )			( ( (x) < (min) ) ? (min) : ( ( (x) > (max) ) ? (max) : (x) ) )
#define STRINGIFY_EXPANDED( a )			#a
#define STRINGIFY( a )					STRINGIFY_EXPANDED(a)

#define APPLICATION_NAME				"Vulkan ATW"
#define WINDOW_TITLE					"Asynchronous Time Warp - Vulkan"

#define GRAPHICS_API_VULKAN				1

#define VK_ALLOCATOR					NULL

#define USE_SPIRV						1
#define USE_PM_MULTIVIEW				1
#define USE_API_DUMP					0	// place vk_layer_settings.txt in the executable folder and change APIDumpFile = TRUE

#if USE_SPIRV == 1
	#define PROGRAM( name )				name##SPIRV
#else
	#define ICD_SPV_MAGIC				0x07230203
	#define PROGRAM( name )				name##GLSL
#endif

#define GLSL_PROGRAM_VERSION			"310 es"	// maintain precision decorations "440 core"
#define GLSL_EXTENSIONS					"#extension GL_EXT_shader_io_blocks : enable\n"	\
										"#extension GL_ARB_enhanced_layouts : enable\n"

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

static void * AllocAlignedMemory( size_t size, size_t alignment )
{
	alignment = ( alignment < sizeof( void * ) ) ? sizeof( void * ) : alignment;
#if defined( OS_WINDOWS )
	return _aligned_malloc( size, alignment );
#elif defined( OS_MAC )
	void * ptr = NULL;
	return ( posix_memalign( &ptr, alignment, size ) == 0 ) ? ptr : NULL;
#else
	return memalign( alignment, size );
#endif
}

static void FreeAlignedMemory( void * ptr )
{
#if defined( OS_WINDOWS )
	_aligned_free( ptr );
#else
	free( ptr );
#endif
}

static void Print( const char * format, ... )
{
#if defined( OS_WINDOWS )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	vsnprintf_s( buffer, 4096, _TRUNCATE, format, args );
	va_end( args );

	OutputDebugString( buffer );
#elif defined( OS_MAC )
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

	__android_log_print( ANDROID_LOG_VERBOSE, "atw", "%s", buffer );
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
#elif defined( OS_MAC )
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

	__android_log_print( ANDROID_LOG_ERROR, "atw", "%s", buffer );
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
#elif defined( OS_MAC )
	static char version[1024];
	size_t len;
	int mib[2] = { CTL_KERN, KERN_OSRELEASE };
    
	if ( sysctl( mib, 2, version, &len, NULL, 0 ) == 0 )
	{
		const char * dot = strstr( version, "." );
		if ( dot != NULL )
		{
			const int kernelMajor = (int)strtol( version, (char **)NULL, 10 );
			const int kernelMinor = (int)strtol( dot + 1, (char **)NULL, 10 );
			const int osxMajor = 10;
			const int osxMinor = kernelMajor - 4;
			const int osxSub = kernelMinor + 1;
			snprintf( version, sizeof( version ), "Apple Mac OS X %d.%d.%d", osxMajor, osxMinor, osxSub );
			return version;
		}
	}

	return "Apple Mac OS X";
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

	char propval[PROP_VALUE_MAX] = { 0 };

	void * handle = dlopen( "libc.so", RTLD_NOLOAD );
	if ( handle != NULL )
	{
		typedef int (*PFN_SYSTEM_PROP_GET)(const char *, char *);
		PFN_SYSTEM_PROP_GET __my_system_property_get = (PFN_SYSTEM_PROP_GET)dlsym( handle, "__system_property_get" );
		if ( __my_system_property_get != NULL )
		{
			__my_system_property_get( "ro.build.version.release", propval );
		}
	}

	snprintf( version, sizeof( version ), "Android %s", propval );

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
#elif defined( OS_MAC )
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

typedef unsigned long long Microseconds_t;

static Microseconds_t GetTimeMicroseconds()
{
#if defined( OS_WINDOWS )
	static Microseconds_t ticksPerSecond = 0;
	static Microseconds_t timeBase = 0;

	if ( ticksPerSecond == 0 )
	{
		LARGE_INTEGER li;
		QueryPerformanceFrequency( &li );
		ticksPerSecond = (Microseconds_t) li.QuadPart;
		QueryPerformanceCounter( &li );
		timeBase = (Microseconds_t) li.LowPart + 0xFFFFFFFFULL * li.HighPart;
	}

	LARGE_INTEGER li;
	QueryPerformanceCounter( &li );
	Microseconds_t counter = (Microseconds_t) li.LowPart + 0xFFFFFFFFULL * li.HighPart;
	return ( counter - timeBase ) * 1000ULL * 1000ULL / ticksPerSecond;
#elif defined( OS_ANDROID )
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts );
	return (Microseconds_t) ts.tv_sec * 1000ULL * 1000ULL + ts.tv_nsec / 1000ULL;
#else
	static Microseconds_t timeBase = 0;

	struct timeval tv;
	gettimeofday( &tv, 0 );

	if ( timeBase == 0 )
	{
		timeBase = (Microseconds_t) tv.tv_sec * 1000ULL * 1000ULL;
	}

	return (Microseconds_t) tv.tv_sec * 1000ULL * 1000ULL + tv.tv_usec - timeBase;
#endif
}

/*
================================================================================================================================

Mutex for mutual exclusion on shared resources within a single process.

Equivalent to a Windows Critical Section Object which allows recursive access. This mutex cannot be
used for mutual-exclusion synchronization between threads from different processes.

Mutex_t

static void Mutex_Create( Mutex_t * mutex );
static void Mutex_Destroy( Mutex_t * mutex );
static bool Mutex_Lock( Mutex_t * mutex, const bool blocking );
static void Mutex_Unlock( Mutex_t * mutex );

================================================================================================================================
*/

typedef struct
{
#if defined( OS_WINDOWS )
	CRITICAL_SECTION	handle;
#else
	pthread_mutex_t		mutex;
#endif
} Mutex_t;

static void Mutex_Create( Mutex_t * mutex )
{
#if defined( OS_WINDOWS )
	InitializeCriticalSection( &mutex->handle );
#else
	pthread_mutexattr_t attr;
	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
	pthread_mutex_init( &mutex->mutex, &attr );
#endif
}

static void Mutex_Destroy( Mutex_t * mutex )
{
#if defined( OS_WINDOWS )
	DeleteCriticalSection( &mutex->handle );
#else
	pthread_mutex_destroy( &mutex->mutex );
#endif
}

static bool Mutex_Lock( Mutex_t * mutex, const bool blocking )
{
#if defined( OS_WINDOWS )
	if ( TryEnterCriticalSection( &mutex->handle ) == 0 )
	{
		if ( !blocking )
		{
			return false;
		}
		EnterCriticalSection( &mutex->handle );
	}
	return true;
#else
	if ( pthread_mutex_trylock( &mutex->mutex ) == EBUSY )
	{
		if ( !blocking )
		{
			return false;
		}
		pthread_mutex_lock( &mutex->mutex );
	}
	return true;
#endif
}

static void Mutex_Unlock( Mutex_t * mutex )
{
#if defined( OS_WINDOWS )
	LeaveCriticalSection( &mutex->handle );
#else
	pthread_mutex_unlock( &mutex->mutex );
#endif
}

/*
================================================================================================================================

Signal for thread synchronization, similar to a Windows event object which only supports SetEvent.

Windows event objects come in two types: auto-reset events and manual-reset events. A Windows event object
can be signalled by calling either SetEvent or PulseEvent.

When a manual-reset event is signaled by calling SetEvent, it sets the event into the signaled state and
wakes up all threads waiting on the event. The manual-reset event remains in the signalled state until
the event is manually reset. When an auto-reset event is signaled by calling SetEvent and there are any
threads waiting, it wakes up only one thread and resets the event to the non-signaled state. If there are
no threads waiting for an auto-reset event, then the event remains signaled until a single waiting thread
waits on it and is released.

When a manual-reset event is signaled by calling PulseEvent, it wakes up all waiting threads and atomically
resets the event. When an auto-reset event is signaled by calling PulseEvent, and there are any threads
waiting, it wakes up only one thread and resets the event to the non-signaled state. If there are no threads
waiting, then no threads are released and the event is set to the non-signaled state.

A Windows event object has limited functionality compared to a POSIX condition variable. Unlike a
Windows event object, the expression waited upon by a POSIX condition variable can be arbitrarily complex.
Furthermore, there is no way to release just one waiting thread with a manual-reset Windows event object.
Similarly there is no way to release all waiting threads with an auto-reset Windows event object.
These limitations make it difficult to simulate a POSIX condition variable using Windows event objects.

Windows Vista and later implement PCONDITION_VARIABLE, but as Douglas C. Schmidt and Irfan Pyarali point
out, it is complicated to simulate a POSIX condition variable on prior versions of Windows without causing
unfair or even incorrect behavior:

	1. "Strategies for Implementing POSIX Condition Variables on Win32"
	   http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
	2. "Patterns for POSIX Condition Variables on Win32"
	   http://www.cs.wustl.edu/~schmidt/win32-cv-2.html
	
Even using SignalObjectAndWait is not safe because as per the Microsoft documentation: "Note that the 'signal'
and 'wait' are not guaranteed to be performed as an atomic operation. Threads executing on other processors
can observe the signaled state of the first object before the thread calling SignalObjectAndWait begins its
wait on the second object."

Simulating a Windows event object using a POSIX condition variable is fairly straight forward, which
is done here. However, this implementation does not support the equivalent of PulseEvent, because
PulseEvent is unreliable. On Windows, a thread waiting on an event object can be momentarily removed
from the wait state by a kernel-mode Asynchronous Procedure Call (APC), and then returned to the wait
state after the APC is complete. If a call to PulseEvent occurs during the time when the thread has
been temporarily removed from the wait state, then the thread will not be released, because PulseEvent
releases only those threads that are in the wait state at the moment PulseEvent is called.

Signal_t

static void Signal_Create( Signal_t * signal, const bool autoReset );
static void Signal_Destroy( Signal_t * signal );
static bool Signal_Wait( Signal_t * signal, const int timeOutMilliseconds );
static void Signal_Raise( Signal_t * signal );
static void Signal_Clear( Signal_t * signal );

================================================================================================================================
*/

typedef struct
{
#if defined( OS_WINDOWS )
	HANDLE			handle;
#else
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	int				waitCount;		// number of threads waiting on the signal
	bool			autoReset;		// automatically clear the signalled state when a single thread is released
	bool			signaled;		// in the signalled state if true
#endif
} Signal_t;

static void Signal_Create( Signal_t * signal, const bool autoReset )
{
#if defined( OS_WINDOWS )
	signal->handle = CreateEvent( NULL, !autoReset, FALSE, NULL );
#else
	pthread_mutex_init( &signal->mutex, NULL );
	pthread_cond_init( &signal->cond, NULL );
	signal->waitCount = 0;
	signal->autoReset = autoReset;
	signal->signaled = false;
#endif
}

static void Signal_Destroy( Signal_t * signal )
{
#if defined( OS_WINDOWS )
	CloseHandle( signal->handle );
#else
	pthread_cond_destroy( &signal->cond );
	pthread_mutex_destroy( &signal->mutex );
#endif
}

// Waits for the object to enter the signalled state and returns true if this state is reached within the time-out period.
// If 'autoReset' is true then the first thread that reaches the signalled state within the time-out period will clear the signalled state.
// If 'timeOutMilliseconds' is negative then this will wait indefinitely until the signalled state is reached.
// Returns true if the thread was released because the object entered the signalled state, returns false if the time-out is reached first.
static bool Signal_Wait( Signal_t * signal, const int timeOutMilliseconds )
{
#if defined( OS_WINDOWS )
	DWORD result = WaitForSingleObject( signal->handle, timeOutMilliseconds < 0 ? INFINITE : timeOutMilliseconds );
	assert( result == WAIT_OBJECT_0 || ( timeOutMilliseconds >= 0 && result == WAIT_TIMEOUT ) );
	return ( result == WAIT_OBJECT_0 );
#else
	bool released = false;
	pthread_mutex_lock( &signal->mutex );
	if ( signal->signaled )
	{
		released = true;
	}
	else
	{
		signal->waitCount++;
		if ( timeOutMilliseconds < 0 )
		{
			do
			{
				pthread_cond_wait( &signal->cond, &signal->mutex );
				// Must re-check condition because pthread_cond_wait may spuriously wake up.
			} while ( signal->signaled == false );
		}
		else if ( timeOutMilliseconds > 0 )
		{
			struct timeval tp;
			gettimeofday( &tp, NULL );
			struct timespec ts;
			ts.tv_sec = tp.tv_sec + timeOutMilliseconds / 1000;
			ts.tv_nsec = tp.tv_usec * 1000 + ( timeOutMilliseconds % 1000 ) * 1000000;
			do
			{
				if ( pthread_cond_timedwait( &signal->cond, &signal->mutex, &ts ) == ETIMEDOUT )
				{
					break;
				}
				// Must re-check condition because pthread_cond_timedwait may spuriously wake up.
			} while ( signal->signaled == false );
		}
		released = signal->signaled;
		signal->waitCount--;
	}
	if ( released && signal->autoReset )
	{
		signal->signaled = false;
	}
	pthread_mutex_unlock( &signal->mutex );
	return released;
#endif
}

// Enter the signalled state.
// Note that if 'autoReset' is true then this will only release a single thread.
static void Signal_Raise( Signal_t * signal )
{
#if defined( OS_WINDOWS )
	SetEvent( signal->handle );
#else
	pthread_mutex_lock( &signal->mutex );
	signal->signaled = true;
	if ( signal->waitCount > 0 )
	{
		pthread_cond_broadcast( &signal->cond );
	}
	pthread_mutex_unlock( &signal->mutex );
#endif
}

// Clear the signalled state.
// Should not be needed for auto-reset signals (autoReset == true).
static void Signal_Clear( Signal_t * signal )
{
#if defined( OS_WINDOWS )
	ResetEvent( signal->handle );
#else
	pthread_mutex_lock( &signal->mutex );
	signal->signaled = false;
	pthread_mutex_unlock( &signal->mutex );
#endif
}

/*
================================================================================================================================

Worker thread.

When the thread is first created, it will be in a suspended state. The thread function will be
called as soon as the thread is signalled. If the thread is not signalled again, then the thread
will return to a suspended state as soon as the thread function returns. The thread function will
be called again by signalling the thread again. The thread function will be called again right
away, when the thread is signalled during the execution of the thread function. Signalling the
thread more than once during the execution of the thread function does not cause the thread
function to be called multiple times. The thread can be joined to wait for the thread function
to return.

This worker thread will function as a normal thread by immediately signalling the thread after creation.
Once the thread function returns, the thread can be destroyed. Destroying the thread always waits
for the thread function to return first.

Thread_t

static bool Thread_Create( Thread_t * thread, const char * threadName, threadFunction_t threadFunction, void * threadData );
static void Thread_Destroy( Thread_t * thread );
static void Thread_Signal( Thread_t * thread );
static void Thread_Join( Thread_t * thread );
static void Thread_Submit( Thread_t * thread, threadFunction_t threadFunction, void * threadData );

static void Thread_SetName( const char * name );
static void Thread_SetAffinity( int mask );
static void Thread_SetRealTimePriority( int priority );

================================================================================================================================
*/

typedef void (*threadFunction_t)( void * data );

#if defined( OS_WINDOWS )
#define THREAD_HANDLE			HANDLE
#define THREAD_RETURN_TYPE		int
#define THREAD_RETURN_VALUE		0
#else
#define THREAD_HANDLE			pthread_t
#define THREAD_RETURN_TYPE		void *
#define THREAD_RETURN_VALUE		0
#endif

#define THREAD_AFFINITY_BIG_CORES		-1

typedef struct
{
	char				threadName[128];
	threadFunction_t	threadFunction;
	void *				threadData;

	void *				stack;
	THREAD_HANDLE		handle;
	Signal_t			workIsDone;
	Signal_t			workIsAvailable;
	Mutex_t				workMutex;
	volatile bool		terminate;
} Thread_t;

// Note that on Android AttachCurrentThread will reset the thread name.
static void Thread_SetName( const char * name )
{
#if defined( OS_WINDOWS )
	static const unsigned int MS_VC_EXCEPTION = 0x406D1388;

	typedef struct
	{
		DWORD dwType;		// Must be 0x1000.
		LPCSTR szName;		// Pointer to name (in user address space).
		DWORD dwThreadID;	// Thread ID (-1 = caller thread).
		DWORD dwFlags;		// Reserved for future use, must be zero.
	} THREADNAME_INFO;

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = (DWORD)( -1 );
	info.dwFlags = 0;
	__try
	{
		RaiseException( MS_VC_EXCEPTION, 0, sizeof( info ) / sizeof( DWORD ), (const ULONG_PTR *)&info );
	}
	__except( GetExceptionCode() == MS_VC_EXCEPTION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
	{
		info.dwFlags = 0;
	}
#elif defined( OS_MAC )
	pthread_setname_np( name );
#elif defined( OS_LINUX )
	pthread_setname_np( pthread_self(), name );
#elif defined( OS_ANDROID )
	prctl( PR_SET_NAME, (long)name, 0, 0, 0 );
#endif
}

static void Thread_SetAffinity( int mask )
{
#if defined( OS_WINDOWS )
	if ( mask == THREAD_AFFINITY_BIG_CORES )
	{
		return;
	}
	HANDLE thread = GetCurrentThread();
	if ( !SetThreadAffinityMask( thread, mask ) )
	{
		char buffer[1024];
		DWORD error = GetLastError();
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), buffer, sizeof( buffer ), NULL );
		Print( "Failed to set thread %p affinity: %s(%d)\n", thread, buffer, error );
	}
	else
	{
		Print( "Thread %p affinity set to 0x%02X\n", thread, mask );
	}
#elif defined( OS_MAC )
	// OS X does not export interfaces that identify processors or control thread placement.
	// Explicit thread to processor binding is not supported.
	UNUSED_PARM( mask );
#elif defined( OS_ANDROID )
	// Optionally use the faster cores of a heterogeneous CPU.
	if ( mask == THREAD_AFFINITY_BIG_CORES )
	{
		mask = 0;
		unsigned int bestFrequency = 0;
		for ( int i = 0; i < 16; i++ )
		{
			int maxFrequency = 0;
			const char * files[] =
			{
				"scaling_available_frequencies",	// not available on all devices
				"scaling_max_freq",					// no user read permission on all devices
				"cpuinfo_max_freq",					// could be set lower than the actual max, but better than nothing
			};
			for ( int j = 0; j < ARRAY_SIZE( files ); j++ )
			{
				char fileName[1024];
				sprintf( fileName, "/sys/devices/system/cpu/cpu%d/cpufreq/%s", i, files[j] );
				FILE * fp = fopen( fileName, "r" );
				if ( fp == NULL )
				{
					continue;
				}
				char buffer[1024];
				if ( fgets( buffer, sizeof( buffer ), fp ) == NULL )
				{
					fclose( fp );
					continue;
				}
				for ( int index = 0; buffer[index] != '\0'; )
				{
					const unsigned int frequency = atoi( buffer + index );
					maxFrequency = ( frequency > maxFrequency ) ? frequency : maxFrequency;
					while ( isspace( buffer[index] ) ) { index++; }
					while ( isdigit( buffer[index] ) ) { index++; }
				}
				fclose( fp );
				break;
			}
			if ( maxFrequency == 0 )
			{
				break;
			}

			if ( maxFrequency == bestFrequency )
			{
				mask |= ( 1 << i );
			}
			else if ( maxFrequency > bestFrequency )
			{
				mask = ( 1 << i );
				bestFrequency = maxFrequency;
			}
		}

		if ( mask == 0 )
		{
			return;
		}
	}

	// Set the thread affinity.
	pid_t pid = gettid();
	int syscallres = syscall( __NR_sched_setaffinity, pid, sizeof( mask ), &mask );
	if ( syscallres )
	{
		int err = errno;
		Print( "    Error sched_setaffinity(%d): thread=(%d) mask=0x%X err=%s(%d)\n", __NR_sched_setaffinity, pid, mask, strerror( err ), err );
	}
	else
	{
		Print( "    Thread %d affinity 0x%02X\n", pid, mask );
	}
#else
	UNUSED_PARM( mask );
#endif
}

static void Thread_SetRealTimePriority( int priority )
{
#if defined( OS_WINDOWS )
	UNUSED_PARM( priority );
	HANDLE process = GetCurrentProcess();
	if( !SetPriorityClass( process, REALTIME_PRIORITY_CLASS ) )
	{
		char buffer[1024];
		DWORD error = GetLastError();
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), buffer, sizeof( buffer ), NULL );
		Print( "Failed to set process %p priority class: %s(%d)\n", process, buffer, error );
	}
	else
	{
		Print( "Process %p priority class set to real-time.\n", process );
	}
	HANDLE thread = GetCurrentThread();
	if ( !SetThreadPriority( thread, THREAD_PRIORITY_TIME_CRITICAL ) )
	{
		char buffer[1024];
		DWORD error = GetLastError();
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), buffer, sizeof( buffer ), NULL );
		Print( "Failed to set thread %p priority: %s(%d)\n", thread, buffer, error );
	}
	else
	{
		Print( "Thread %p priority set to critical.\n", thread );
	}
#elif defined( OS_MAC ) || defined( OS_LINUX )
	struct sched_param sp;
	memset( &sp, 0, sizeof( struct sched_param ) );
	sp.sched_priority = priority;
	if ( pthread_setschedparam( pthread_self(), SCHED_FIFO, &sp ) == -1 )
	{
		Print( "Failed to change thread %d priority.\n", (unsigned int)pthread_self() );
	}
	else
	{
		Print( "Thread %d set to SCHED_FIFO, priority=%d\n", (unsigned int)pthread_self(), priority );
	}
#elif defined( OS_ANDROID )
	struct sched_attr
	{
		uint32_t size;
		uint32_t sched_policy;
		uint64_t sched_flags;
		int32_t  sched_nice;
		uint32_t sched_priority;
		uint64_t sched_runtime;
		uint64_t sched_deadline;
		uint64_t sched_period;
	} attr;

	memset( &attr, 0, sizeof( attr ) );
	attr.size = sizeof( attr );
	attr.sched_policy = SCHED_FIFO;
	attr.sched_flags = SCHED_FLAG_RESET_ON_FORK;
	attr.sched_nice = 0;				// (SCHED_OTHER, SCHED_BATCH)
	attr.sched_priority = priority;		// (SCHED_FIFO, SCHED_RR)
	attr.sched_runtime = 0;				// (SCHED_DEADLINE)
	attr.sched_deadline = 0;			// (SCHED_DEADLINE)
	attr.sched_period = 0;				// (SCHED_DEADLINE)

	unsigned int flags = 0;

	pid_t pid = gettid();
	int syscallres = syscall( __NR_sched_setattr, pid, &attr, flags );
	if ( syscallres )
	{
		int err = errno;
		Print( "    Error sched_setattr(%d): thread=%d err=%s(%d)\n", __NR_sched_setattr, pid, strerror( err ), err );
	}
	else
	{
		Print( "    Thread %d set to SCHED_FIFO, priority=%d\n", pid, priority );
	}
#else
	UNUSED_PARM( priority );
#endif
}

static THREAD_RETURN_TYPE ThreadFunctionInternal( void * data )
{
	Thread_t * thread = (Thread_t *)data;

	Thread_SetName( thread->threadName );

	for ( ; ; )
	{
		Mutex_Lock( &thread->workMutex, true );
		if ( Signal_Wait( &thread->workIsAvailable, 0 ) )
		{
			Mutex_Unlock( &thread->workMutex );
		}
		else
		{
			Signal_Raise( &thread->workIsDone );
			Mutex_Unlock( &thread->workMutex );
			Signal_Wait( &thread->workIsAvailable, -1 );
		}
		if ( thread->terminate )
		{
			Signal_Raise( &thread->workIsDone );
			break;
		}
		thread->threadFunction( thread->threadData );
	}
	return THREAD_RETURN_VALUE;
}

static bool Thread_Create( Thread_t * thread, const char * threadName, threadFunction_t threadFunction, void * threadData )
{
	strncpy( thread->threadName, threadName, sizeof( thread->threadName ) );
	thread->threadName[sizeof( thread->threadName ) - 1] = '\0';
	thread->threadFunction = threadFunction;
	thread->threadData = threadData;
	thread->stack = NULL;
	Signal_Create( &thread->workIsDone, false );
	Signal_Create( &thread->workIsAvailable, true );
	Mutex_Create( &thread->workMutex );
	thread->terminate = false;

#if defined( OS_WINDOWS )
	const int stackSize = 512 * 1024;
	DWORD threadID;
	thread->handle = CreateThread( NULL, stackSize, (LPTHREAD_START_ROUTINE)ThreadFunctionInternal, thread, STACK_SIZE_PARAM_IS_A_RESERVATION, &threadID );
	if ( thread->handle == 0 )
	{
		return false;
	}
#else
	const int stackSize = 512 * 1024;
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_attr_setstacksize( &attr, stackSize );
	int ret = pthread_create( &thread->handle, &attr, ThreadFunctionInternal, thread );
	if ( ret != 0 )
	{
		return false;
	}
	pthread_attr_destroy( &attr );
#endif

	Signal_Wait( &thread->workIsDone, -1 );
	return true;
}

static void Thread_Destroy( Thread_t * thread )
{
	Mutex_Lock( &thread->workMutex, true );
	Signal_Clear( &thread->workIsDone );
	thread->terminate = true;
	Signal_Raise( &thread->workIsAvailable );
	Mutex_Unlock( &thread->workMutex );
	Signal_Wait( &thread->workIsDone, -1 );
	Mutex_Destroy( &thread->workMutex );
	Signal_Destroy( &thread->workIsDone );
	Signal_Destroy( &thread->workIsAvailable );
#if defined( OS_WINDOWS )
	WaitForSingleObject( thread->handle, INFINITE );
	CloseHandle( thread->handle );
#else
	pthread_join( thread->handle, NULL );
#endif
}

static void Thread_Signal( Thread_t * thread )
{
	Mutex_Lock( &thread->workMutex, true );
	Signal_Clear( &thread->workIsDone );
	Signal_Raise( &thread->workIsAvailable );
	Mutex_Unlock( &thread->workMutex );
}

static void Thread_Join( Thread_t * thread )
{
	Signal_Wait( &thread->workIsDone, -1 );
}

static void Thread_Submit( Thread_t * thread, threadFunction_t threadFunction, void * threadData )
{
	Thread_Join( thread );
	thread->threadFunction = threadFunction;
	thread->threadData = threadData;
	Thread_Signal( thread );
}

/*
================================================================================================================================

Frame logging.

Each thread that calls FrameLog_Open will open its own log.
A frame log is always opened for a specified number of frames, and will
automatically close after the specified number of frames have been recorded.
The CPU and GPU times for the recorded frames will be listed at the end of the log.

FrameLog_t

static void FrameLog_Open( const char * fileName, const int numFrames );
static void FrameLog_Write( const char * fileName, const int lineNumber, const char * function );
static void FrameLog_BeginFrame();
static void FrameLog_EndFrame( const float cpuTimeMilliseconds, const float gpuTimeMilliseconds, const int gpuTimeFramesDelayed );

================================================================================================================================
*/

typedef struct
{
	FILE *		fp;
	float *		frameCpuTimes;
	float *		frameGpuTimes;
	int			numFrames;
	int			frame;
} FrameLog_t;

__thread FrameLog_t * threadFrameLog;

static FrameLog_t * FrameLog_Get()
{
	FrameLog_t * l = threadFrameLog;
	if ( l == NULL )
	{
		l = (FrameLog_t *) malloc( sizeof( FrameLog_t ) );
		memset( l, 0, sizeof( FrameLog_t ) );
		threadFrameLog = l;
	}
	return l;
}

static void FrameLog_Open( const char * fileName, const int numFrames )
{
	FrameLog_t * l = FrameLog_Get();
	if ( l != NULL && l->fp == NULL )
	{
		l->fp = fopen( fileName, "wb" );
		if ( l->fp == NULL )
		{
			Print( "Failed to open %s\n", fileName );
		}
		else
		{
			Print( "Opened frame log %s for %d frames.\n", fileName, numFrames );
			l->frameCpuTimes = (float *) malloc( numFrames * sizeof( l->frameCpuTimes[0] ) );
			l->frameGpuTimes = (float *) malloc( numFrames * sizeof( l->frameGpuTimes[0] ) );
			memset( l->frameCpuTimes, 0, numFrames * sizeof( l->frameCpuTimes[0] ) );
			memset( l->frameGpuTimes, 0, numFrames * sizeof( l->frameGpuTimes[0] ) );
			l->numFrames = numFrames;
			l->frame = 0;
		}
	}
}

static void FrameLog_Write( const char * fileName, const int lineNumber, const char * function )
{
	FrameLog_t * l = FrameLog_Get();
	if ( l != NULL && l->fp != NULL )
	{
		if ( l->frame < l->numFrames )
		{
			fprintf( l->fp, "%s(%d): %s\r\n", fileName, lineNumber, function );
		}
	}
}

static void FrameLog_BeginFrame()
{
	FrameLog_t * l = FrameLog_Get();
	if ( l != NULL && l->fp != NULL )
	{
		if ( l->frame < l->numFrames )
		{
#if defined( _DEBUG )
			fprintf( l->fp, "================ BEGIN FRAME %d ================\r\n", l->frame );
#endif
		}
	}
}

static void FrameLog_EndFrame( const float cpuTimeMilliseconds, const float gpuTimeMilliseconds, const int gpuTimeFramesDelayed )
{
	FrameLog_t * l = FrameLog_Get();
	if ( l != NULL && l->fp != NULL )
	{
		if ( l->frame < l->numFrames )
		{
			l->frameCpuTimes[l->frame] = cpuTimeMilliseconds;
#if defined( _DEBUG )
			fprintf( l->fp, "================ END FRAME %d ================\r\n", l->frame );
#endif
		}
		if ( l->frame >= gpuTimeFramesDelayed && l->frame < l->numFrames + gpuTimeFramesDelayed )
		{
			l->frameGpuTimes[l->frame - gpuTimeFramesDelayed] = gpuTimeMilliseconds;
		}

		l->frame++;

		if ( l->frame >= l->numFrames + gpuTimeFramesDelayed )
		{
			for ( int i = 0; i < l->numFrames; i++ )
			{
				fprintf( l->fp, "frame %d: CPU = %1.1f ms, GPU = %1.1f ms\r\n", i, l->frameCpuTimes[i], l->frameGpuTimes[i] );
			}

			Print( "Closing frame log file (%d frames).\n", l->numFrames );
			fclose( l->fp );
			free( l->frameCpuTimes );
			free( l->frameGpuTimes );
			memset( l, 0, sizeof( FrameLog_t ) );
		}
	}
}

/*
================================================================================================================================

Vectors and matrices.

Vector2i_t
Vector3i_t
Vector4i_t
Vector2f_t
Vector3f_t
Vector4f_t
Matrix4x4f_t		// This is a column-major matrix.

static void Vector3f_Zero( Vector3f_t * v );
static void Vector3f_Normalize( Vector3f_t * v );

static void Matrix4x4f_CreateIdentity( Matrix4x4f_t * matrix );
static void Matrix4x4f_CreateTranslation( Matrix4x4f_t * matrix, const float x, const float y, const float z );
static void Matrix4x4f_CreateRotation( Matrix4x4f_t * matrix, const float degreesX, const float degreesY, const float degreesZ );
static void Matrix4x4f_CreateProjection( Matrix4x4f_t * matrix, const float minX, const float maxX,
											float const minY, const float maxY, const float nearZ, const float farZ );
static void Matrix4x4f_CreateProjectionFov( Matrix4x4f_t * matrix, const float fovDegreesX, const float fovDegreesY,
											const float offsetX, const float offsetY, const float nearZ, const float farZ );
static void Matrix4x4f_Multiply( Matrix4x4f_t * out, const Matrix4x4f_t * a, const Matrix4x4f_t * b );
static void Matrix4x4f_Invert( Matrix4x4f_t * result, const Matrix4x4f_t * src );
static void Matrix4x4f_InvertHomogeneous( Matrix4x4f_t * result, const Matrix4x4f_t * src );

================================================================================================================================
*/

// 2D integer vector
typedef struct
{
	int x;
	int y;
} Vector2i_t;

// 3D integer vector
typedef struct
{
	int x;
	int y;
	int z;
} Vector3i_t;

// 4D integer vector
typedef struct
{
	int x;
	int y;
	int z;
	int w;
} Vector4i_t;

// 2D float vector
typedef struct
{
	float x;
	float y;
} Vector2f_t;

// 3D float vector
typedef struct
{
	float x;
	float y;
	float z;
} Vector3f_t;

// 4D float vector
typedef struct
{
	float x;
	float y;
	float z;
	float w;
} Vector4f_t;

// Column-major 4x4 matrix
typedef struct
{
	float m[4][4];
} Matrix4x4f_t;

static const Vector4f_t colorRed		= { 1.0f, 0.0f, 0.0f, 1.0f };
static const Vector4f_t colorGreen		= { 0.0f, 1.0f, 0.0f, 1.0f };
static const Vector4f_t colorBlue		= { 0.0f, 0.0f, 1.0f, 1.0f };
static const Vector4f_t colorYellow		= { 1.0f, 1.0f, 0.0f, 1.0f };
static const Vector4f_t colorPurple		= { 1.0f, 0.0f, 1.0f, 1.0f };
static const Vector4f_t colorCyan		= { 0.0f, 1.0f, 1.0f, 1.0f };
static const Vector4f_t colorLightGrey	= { 0.7f, 0.7f, 0.7f, 1.0f };
static const Vector4f_t colorDarkGrey	= { 0.3f, 0.3f, 0.3f, 1.0f };

static void Vector3f_Zero( Vector3f_t * v )
{
	v->x = 0.0f;
	v->y = 0.0f;
	v->z = 0.0f;
}

static void Vector3f_Normalize( Vector3f_t * v )
{
	const float SMALLEST_NON_DENORMAL = 1.1754943508222875e-038f;	// ( 1U << 23 )
	const float lengthSqr = v->x * v->x + v->y * v->y + v->z * v->z;
	const float lengthRcp = ( lengthSqr >= SMALLEST_NON_DENORMAL ) ? 1.0f / sqrtf( lengthSqr ) : 1.0f;
	v->x *= lengthRcp;
	v->y *= lengthRcp;
	v->z *= lengthRcp;
}

// Use left-multiplication to accumulate transformations.
static void Matrix4x4f_Multiply( Matrix4x4f_t * out, const Matrix4x4f_t * a, const Matrix4x4f_t * b )
{
	out->m[0][0] = a->m[0][0] * b->m[0][0] + a->m[1][0] * b->m[0][1] + a->m[2][0] * b->m[0][2] + a->m[3][0] * b->m[0][3];
	out->m[0][1] = a->m[0][1] * b->m[0][0] + a->m[1][1] * b->m[0][1] + a->m[2][1] * b->m[0][2] + a->m[3][1] * b->m[0][3];
	out->m[0][2] = a->m[0][2] * b->m[0][0] + a->m[1][2] * b->m[0][1] + a->m[2][2] * b->m[0][2] + a->m[3][2] * b->m[0][3];
	out->m[0][3] = a->m[0][3] * b->m[0][0] + a->m[1][3] * b->m[0][1] + a->m[2][3] * b->m[0][2] + a->m[3][3] * b->m[0][3];

	out->m[1][0] = a->m[0][0] * b->m[1][0] + a->m[1][0] * b->m[1][1] + a->m[2][0] * b->m[1][2] + a->m[3][0] * b->m[1][3];
	out->m[1][1] = a->m[0][1] * b->m[1][0] + a->m[1][1] * b->m[1][1] + a->m[2][1] * b->m[1][2] + a->m[3][1] * b->m[1][3];
	out->m[1][2] = a->m[0][2] * b->m[1][0] + a->m[1][2] * b->m[1][1] + a->m[2][2] * b->m[1][2] + a->m[3][2] * b->m[1][3];
	out->m[1][3] = a->m[0][3] * b->m[1][0] + a->m[1][3] * b->m[1][1] + a->m[2][3] * b->m[1][2] + a->m[3][3] * b->m[1][3];

	out->m[2][0] = a->m[0][0] * b->m[2][0] + a->m[1][0] * b->m[2][1] + a->m[2][0] * b->m[2][2] + a->m[3][0] * b->m[2][3];
	out->m[2][1] = a->m[0][1] * b->m[2][0] + a->m[1][1] * b->m[2][1] + a->m[2][1] * b->m[2][2] + a->m[3][1] * b->m[2][3];
	out->m[2][2] = a->m[0][2] * b->m[2][0] + a->m[1][2] * b->m[2][1] + a->m[2][2] * b->m[2][2] + a->m[3][2] * b->m[2][3];
	out->m[2][3] = a->m[0][3] * b->m[2][0] + a->m[1][3] * b->m[2][1] + a->m[2][3] * b->m[2][2] + a->m[3][3] * b->m[2][3];

	out->m[3][0] = a->m[0][0] * b->m[3][0] + a->m[1][0] * b->m[3][1] + a->m[2][0] * b->m[3][2] + a->m[3][0] * b->m[3][3];
	out->m[3][1] = a->m[0][1] * b->m[3][0] + a->m[1][1] * b->m[3][1] + a->m[2][1] * b->m[3][2] + a->m[3][1] * b->m[3][3];
	out->m[3][2] = a->m[0][2] * b->m[3][0] + a->m[1][2] * b->m[3][1] + a->m[2][2] * b->m[3][2] + a->m[3][2] * b->m[3][3];
	out->m[3][3] = a->m[0][3] * b->m[3][0] + a->m[1][3] * b->m[3][1] + a->m[2][3] * b->m[3][2] + a->m[3][3] * b->m[3][3];
}

// Returns a 3x3 minor of a 4x4 matrix.
static float Matrix4x4f_Minor( const Matrix4x4f_t * src, int r0, int r1, int r2, int c0, int c1, int c2 )
{
	return	src->m[r0][c0] * ( src->m[r1][c1] * src->m[r2][c2] - src->m[r2][c1] * src->m[r1][c2] ) -
			src->m[r0][c1] * ( src->m[r1][c0] * src->m[r2][c2] - src->m[r2][c0] * src->m[r1][c2] ) +
			src->m[r0][c2] * ( src->m[r1][c0] * src->m[r2][c1] - src->m[r2][c0] * src->m[r1][c1] );
}
 
// Calculates the inverse of a 4x4 matrix.
static void Matrix4x4f_Invert( Matrix4x4f_t * result, const Matrix4x4f_t * src )
{
	const float rcpDet = 1.0f / (	src->m[0][0] * Matrix4x4f_Minor( src, 1, 2, 3, 1, 2, 3 ) -
									src->m[0][1] * Matrix4x4f_Minor( src, 1, 2, 3, 0, 2, 3 ) +
									src->m[0][2] * Matrix4x4f_Minor( src, 1, 2, 3, 0, 1, 3 ) -
									src->m[0][3] * Matrix4x4f_Minor( src, 1, 2, 3, 0, 1, 2 ) );

	result->m[0][0] =  Matrix4x4f_Minor( src, 1, 2, 3, 1, 2, 3 ) * rcpDet;
	result->m[0][1] = -Matrix4x4f_Minor( src, 0, 2, 3, 1, 2, 3 ) * rcpDet;
	result->m[0][2] =  Matrix4x4f_Minor( src, 0, 1, 3, 1, 2, 3 ) * rcpDet;
	result->m[0][3] = -Matrix4x4f_Minor( src, 0, 1, 2, 1, 2, 3 ) * rcpDet;
	result->m[1][0] = -Matrix4x4f_Minor( src, 1, 2, 3, 0, 2, 3 ) * rcpDet;
	result->m[1][1] =  Matrix4x4f_Minor( src, 0, 2, 3, 0, 2, 3 ) * rcpDet;
	result->m[1][2] = -Matrix4x4f_Minor( src, 0, 1, 3, 0, 2, 3 ) * rcpDet;
	result->m[1][3] =  Matrix4x4f_Minor( src, 0, 1, 2, 0, 2, 3 ) * rcpDet;
	result->m[2][0] =  Matrix4x4f_Minor( src, 1, 2, 3, 0, 1, 3 ) * rcpDet;
	result->m[2][1] = -Matrix4x4f_Minor( src, 0, 2, 3, 0, 1, 3 ) * rcpDet;
	result->m[2][2] =  Matrix4x4f_Minor( src, 0, 1, 3, 0, 1, 3 ) * rcpDet;
	result->m[2][3] = -Matrix4x4f_Minor( src, 0, 1, 2, 0, 1, 3 ) * rcpDet;
	result->m[3][0] = -Matrix4x4f_Minor( src, 1, 2, 3, 0, 1, 2 ) * rcpDet;
	result->m[3][1] =  Matrix4x4f_Minor( src, 0, 2, 3, 0, 1, 2 ) * rcpDet;
	result->m[3][2] = -Matrix4x4f_Minor( src, 0, 1, 3, 0, 1, 2 ) * rcpDet;
	result->m[3][3] =  Matrix4x4f_Minor( src, 0, 1, 2, 0, 1, 2 ) * rcpDet;
}

// Calculates the inverse of a 4x4 homogeneous matrix.
static void Matrix4x4f_InvertHomogeneous( Matrix4x4f_t * result, const Matrix4x4f_t * src )
{
	result->m[0][0] = src->m[0][0];
	result->m[0][1] = src->m[1][0];
	result->m[0][2] = src->m[2][0];
	result->m[0][3] = 0.0f;
	result->m[1][0] = src->m[0][1];
	result->m[1][1] = src->m[1][1];
	result->m[1][2] = src->m[2][1];
	result->m[1][3] = 0.0f;
	result->m[2][0] = src->m[0][2];
	result->m[2][1] = src->m[1][2];
	result->m[2][2] = src->m[2][2];
	result->m[2][3] = 0.0f;
	result->m[3][0] = -( src->m[0][0] * src->m[3][0] + src->m[0][1] * src->m[3][1] + src->m[0][2] * src->m[3][2] );
	result->m[3][1] = -( src->m[1][0] * src->m[3][0] + src->m[1][1] * src->m[3][1] + src->m[1][2] * src->m[3][2] );
	result->m[3][2] = -( src->m[2][0] * src->m[3][0] + src->m[2][1] * src->m[3][1] + src->m[2][2] * src->m[3][2] );
	result->m[3][3] = 1.0f;
}

// Creates an identity matrix.
static void Matrix4x4f_CreateIdentity( Matrix4x4f_t * matrix )
{
	matrix->m[0][0] = 1.0f; matrix->m[0][1] = 0.0f; matrix->m[0][2] = 0.0f; matrix->m[0][3] = 0.0f;
	matrix->m[1][0] = 0.0f; matrix->m[1][1] = 1.0f; matrix->m[1][2] = 0.0f; matrix->m[1][3] = 0.0f;
	matrix->m[2][0] = 0.0f; matrix->m[2][1] = 0.0f; matrix->m[2][2] = 1.0f; matrix->m[2][3] = 0.0f;
	matrix->m[3][0] = 0.0f; matrix->m[3][1] = 0.0f; matrix->m[3][2] = 0.0f; matrix->m[3][3] = 1.0f;
}

// Creates a translation matrix.
static void Matrix4x4f_CreateTranslation( Matrix4x4f_t * matrix, const float x, const float y, const float z )
{
	matrix->m[0][0] = 1.0f; matrix->m[0][1] = 0.0f; matrix->m[0][2] = 0.0f; matrix->m[0][3] = 0.0f;
	matrix->m[1][0] = 0.0f; matrix->m[1][1] = 1.0f; matrix->m[1][2] = 0.0f; matrix->m[1][3] = 0.0f;
	matrix->m[2][0] = 0.0f; matrix->m[2][1] = 0.0f; matrix->m[2][2] = 1.0f; matrix->m[2][3] = 0.0f;
	matrix->m[3][0] =    x; matrix->m[3][1] =    y; matrix->m[3][2] =    z; matrix->m[3][3] = 1.0f;
}

// Creates a rotation matrix.
// If -Z=forward, +Y=up, +X=right, then degreesX=pitch, degreesY=yaw, degreesZ=roll.
static void Matrix4x4f_CreateRotation( Matrix4x4f_t * matrix, const float degreesX, const float degreesY, const float degreesZ )
{
	const float sinX = sinf( degreesX * ( MATH_PI / 180.0f ) );
	const float cosX = cosf( degreesX * ( MATH_PI / 180.0f ) );
	const Matrix4x4f_t rotationX =
	{ {
		{ 1,     0,    0, 0 },
		{ 0,  cosX, sinX, 0 },
		{ 0, -sinX, cosX, 0 },
		{ 0,     0,    0, 1 }
	} };
	const float sinY = sinf( degreesY * ( MATH_PI / 180.0f ) );
	const float cosY = cosf( degreesY * ( MATH_PI / 180.0f ) );
	const Matrix4x4f_t rotationY =
	{ {
		{ cosY, 0, -sinY, 0 },
		{    0, 1,     0, 0 },
		{ sinY, 0,  cosY, 0 },
		{    0, 0,     0, 1 }
	} };
	const float sinZ = sinf( degreesZ * ( MATH_PI / 180.0f ) );
	const float cosZ = cosf( degreesZ * ( MATH_PI / 180.0f ) );
	const Matrix4x4f_t rotationZ =
	{ {
		{  cosZ, sinZ, 0, 0 },
		{ -sinZ, cosZ, 0, 0 },
		{     0,    0, 1, 0 },
		{     0,    0, 0, 1 }
	} };
	Matrix4x4f_t rotationXY;
	Matrix4x4f_Multiply( &rotationXY, &rotationY, &rotationX );
	Matrix4x4f_Multiply( matrix, &rotationZ, &rotationXY );
}

// Creates a projection matrix based on the specified dimensions.
// The projection matrix transforms -Z=forward, +Y=up, +X=right to the appropriate clip space for the graphics API.
// The far plane is placed at infinity if farZ <= nearZ.
// An infinite projection matrix is preferred for rasterization because, except for
// things *right* up against the near plane, it always provides better precision:
//		"Tightening the Precision of Perspective Rendering"
//		Paul Upchurch, Mathieu Desbrun
//		Journal of Graphics Tools, Volume 16, Issue 1, 2012
static void Matrix4x4f_CreateProjection( Matrix4x4f_t * matrix, const float minX, const float maxX,
											float const minY, const float maxY, const float nearZ, const float farZ )
{
	const float width = maxX - minX;

#if defined( GRAPHICS_API_VULKAN )
	// Set to minY - maxY for a clip space with positive Y down (Vulkan).
	const float height = minY - maxY;
#else
	// Set to maxY - minY for a clip space with positive Y up (OpenGL / D3D).
	const float height = maxY - minY;
#endif

#if defined( GRAPHICS_API_OPENGL )
	// Set to nearZ for a [-1,1] Z clip space (OpenGL).
	const float offsetZ = nearZ;
#else
	// Set to zero for a [0,1] Z clip space (D3D / Vulkan).
	const float offsetZ = 0;
#endif

	if ( farZ <= nearZ )
	{
		// place the far plane at infinity
		matrix->m[0][0] = 2 * nearZ / width;
		matrix->m[1][0] = 0;
		matrix->m[2][0] = ( maxX + minX ) / width;
		matrix->m[3][0] = 0;

		matrix->m[0][1] = 0;
		matrix->m[1][1] = 2 * nearZ / height;
		matrix->m[2][1] = ( maxY + minY ) / height;
		matrix->m[3][1] = 0;

		matrix->m[0][2] = 0;
		matrix->m[1][2] = 0;
		matrix->m[2][2] = -1;
		matrix->m[3][2] = -( nearZ + offsetZ );

		matrix->m[0][3] = 0;
		matrix->m[1][3] = 0;
		matrix->m[2][3] = -1;
		matrix->m[3][3] = 0;
	}
	else
	{
		// normal projection
		matrix->m[0][0] = 2 * nearZ / width;
		matrix->m[1][0] = 0;
		matrix->m[2][0] = ( maxX + minX ) / width;
		matrix->m[3][0] = 0;

		matrix->m[0][1] = 0;
		matrix->m[1][1] = 2 * nearZ / height;
		matrix->m[2][1] = ( maxY + minY ) / height;
		matrix->m[3][1] = 0;

		matrix->m[0][2] = 0;
		matrix->m[1][2] = 0;
		matrix->m[2][2] = -( farZ + offsetZ ) / ( farZ - nearZ );
		matrix->m[3][2] = -( farZ * ( nearZ + offsetZ ) ) / ( farZ - nearZ );

		matrix->m[0][3] = 0;
		matrix->m[1][3] = 0;
		matrix->m[2][3] = -1;
		matrix->m[3][3] = 0;
	}
}

// Creates a projection matrix based on the specified FOV.
static void Matrix4x4f_CreateProjectionFov( Matrix4x4f_t * matrix, const float fovDegreesX, const float fovDegreesY,
												const float offsetX, const float offsetY, const float nearZ, const float farZ )
{
	const float halfWidth = nearZ * tanf( fovDegreesX * ( 0.5f * MATH_PI / 180.0f ) );
	const float halfHeight = nearZ * tanf( fovDegreesY * ( 0.5f * MATH_PI / 180.0f ) );

	const float minX = offsetX - halfWidth;
	const float maxX = offsetX + halfWidth;

	const float minY = offsetY - halfHeight;
	const float maxY = offsetY + halfHeight;

	Matrix4x4f_CreateProjection( matrix, minX, maxX, minY, maxY, nearZ, farZ );
}

/*
================================================================================================================================
 
Rectangles.

ScreenRect_t
ClipRect_t

ScreenRect_t is specified in pixels with 0,0 at the left-bottom.
ClipRect_t is specified in clip space in the range [-1,1], with -1,-1 at the left-bottom.

static ClipRect_t ScreenRect_ToClipRect( const ScreenRect_t * screenRect, const int resolutionX, const int resolutionY );
static ScreenRect_t ClipRect_ToScreenRect( const ClipRect_t * clipRect, const int resolutionX, const int resolutionY );

================================================================================================================================
*/

typedef struct
{
	int x;
	int y;
	int width;
	int height;
} ScreenRect_t;

typedef struct
{
	float x;
	float y;
	float width;
	float height;
} ClipRect_t;

static ClipRect_t ScreenRect_ToClipRect( const ScreenRect_t * screenRect, const int resolutionX, const int resolutionY )
{
	ClipRect_t clipRect;
	clipRect.x = 2.0f * screenRect->x / resolutionX - 1.0f;
	clipRect.y = 2.0f * screenRect->y / resolutionY - 1.0f;
	clipRect.width = 2.0f * screenRect->width / resolutionX;
	clipRect.height = 2.0f * screenRect->height / resolutionY;
	return clipRect;
}

static ScreenRect_t ClipRect_ToScreenRect( const ClipRect_t * clipRect, const int resolutionX, const int resolutionY )
{
	ScreenRect_t screenRect;
	screenRect.x = (int)( ( clipRect->x * 0.5f + 0.5f ) * resolutionX + 0.5f );
	screenRect.y = (int)( ( clipRect->y * 0.5f + 0.5f ) * resolutionY + 0.5f );
	screenRect.width = (int)( clipRect->width * 0.5f * resolutionX + 0.5f );
	screenRect.height = (int)( clipRect->height * 0.5f * resolutionY + 0.5f );
	return screenRect;
}

/*
================================================================================================================================

Vulkan error checking.

================================================================================================================================
*/

#if defined( _DEBUG )
	#define VK( func )		VkCheckErrors( func, #func ); FrameLog_Write( __FILE__, __LINE__, #func );
	#define VC( func )		func; FrameLog_Write( __FILE__, __LINE__, #func );
#else
	#define VK( func )		VkCheckErrors( func, #func );
	#define VC( func )		func;
#endif

#define VK_ERROR_INVALID_SHADER_NV		-1002

const char * VkErrorString( VkResult result )
{
	switch( result )
	{
		case VK_SUCCESS:						return "VK_SUCCESS";
		case VK_NOT_READY:						return "VK_NOT_READY";
		case VK_TIMEOUT:						return "VK_TIMEOUT";
		case VK_EVENT_SET:						return "VK_EVENT_SET";
		case VK_EVENT_RESET:					return "VK_EVENT_RESET";
		case VK_INCOMPLETE:						return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:		return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:		return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:	return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:				return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:		return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:		return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:	return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:		return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:		return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:			return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:		return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_SURFACE_LOST_KHR:			return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_SUBOPTIMAL_KHR:					return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:			return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:	return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:	return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:	return "VK_ERROR_VALIDATION_FAILED_EXT";
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

DriverInstance_t

static bool DriverInstance_Create( DriverInstance_t * intance );
static void DriverInstance_Destroy( DriverInstance_t * instance );

================================================================================================================================
*/

typedef struct
{
	VkBool32											validate;
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
	PFN_vkCreateDevice									vkCreateDevice;
	PFN_vkGetDeviceProcAddr								vkGetDeviceProcAddr;

	// Instance extensions.
	PFN_vkCreateSurfaceKHR								vkCreateSurfaceKHR;
	PFN_vkDestroySurfaceKHR								vkDestroySurfaceKHR;
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR			vkGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR		vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR			vkGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR		vkGetPhysicalDeviceSurfacePresentModesKHR;

	// Debug callback.
	PFN_vkCreateDebugReportCallbackEXT					vkCreateDebugReportCallbackEXT;
	PFN_vkDestroyDebugReportCallbackEXT					vkDestroyDebugReportCallbackEXT;
	VkDebugReportCallbackEXT							debugReportCallback;
} DriverInstance_t;

// Match strings except for hexadecimal numbers.
static bool MatchStrings( const char * str1, const char * str2 )
{
	while ( str1[0] != '\0' && str2[0] != '\0' )
	{
		if ( str1[0] != str2[0] )
		{
			for ( ; str1[0] == 'x' ||
									( str1[0] >= '0' && str1[0] <= '9' ) ||
									( str1[0] >= 'a' && str1[0] <= 'f' ) ||
									( str1[0] >= 'A' && str1[0] <= 'F' ); str1++ ) {}
			for ( ; str2[0] == 'x' ||
									( str2[0] >= '0' && str2[0] <= '9' ) ||
									( str2[0] >= 'a' && str2[0] <= 'f' ) ||
									( str2[0] >= 'A' && str2[0] <= 'F' ); str2++ ) {}
			if ( str1[0] != str2[0] )
			{
				return false;
			}
		}
		str1++;
		str2++;
	}
	return true;
}

typedef VkBool32 (VKAPI_PTR *PFN_vkDebugReportCallbackEXT)(
    VkDebugReportFlagsEXT                       flags,
    VkDebugReportObjectTypeEXT                  objectType,
    uint64_t                                    object,
    size_t                                      location,
    int32_t                                     messageCode,
    const char*                                 pLayerPrefix,
    const char*                                 pMessage,
    void*                                       pUserData);

VkBool32 DebugReportCallback( VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location,
									int32_t msgCode, const char * pLayerPrefix, const char * pMsg, void * pUserData )
{
	UNUSED_PARM( objType );
	UNUSED_PARM( srcObject );
	UNUSED_PARM( location );
	UNUSED_PARM( pUserData );

	// This performance warning is valid but this is how the the secondary command buffer is used.
	// [DS] "vkBeginCommandBuffer(): Secondary Command Buffers (00000039460DB2F8) may perform better if a valid framebuffer parameter is specified."
	if ( MatchStrings( pMsg, "vkBeginCommandBuffer(): Secondary Command Buffers (00000039460DB2F8) may perform better if a valid framebuffer parameter is specified." ) )
	{
		return VK_FALSE;
	}
	// Using GLSL results in the following error (which is fine but GLSL is supported by some drivers):
	// [SC] "Shader is not SPIR-V"
	if ( MatchStrings( pMsg, "Shader is not SPIR-V" ) )
	{
		return VK_FALSE;
	}

	// Validation layer fixes are pending for these:

	// The draw state validation layer incorrectly reports layout errors when submitting a command buffer.
	// https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/46
	// [DS] "Cannot submit cmd buffer using image (0x6) [sub-resource: aspectMask 0x1 array layer 0, mip level 0], with layout VK_IMAGE_LAYOUT_UNDEFINED when first use is VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
	if ( MatchStrings( pMsg, "Cannot submit cmd buffer using image (0x6) [sub-resource: aspectMask 0x1 array layer 0, mip level 0], with layout" ) )
	{
		return VK_FALSE;
	}
	// The draw state validation layer incorrectly reports that a present semaphore is still in use.
	// https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/100
	// [DS] "Cannot delete semaphore 38d1ea00d0 which is in use."
	if ( MatchStrings( pMsg, "Cannot delete semaphore 38d1ea00d0 which is in use." ) )
	{
		return VK_FALSE;
	}
	// The mem tracker validation layer reports a memory leak when calling vkDestroyDebugReportCallbackEXT after vkDestroyDevice,
	// but before vkDestroyInstance, even though VkDebugReportCallbackEXT is an instance object.
	// https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/365
	// [MEM] "Mem Object 148617029248 has not been freed. You should clean up this memory by calling vkFreeMemory(148617029248) prior to vkDestroyDevice()."
	// [OBJTRACK] "OBJ ERROR : VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT object 0x7a613498a0 has not been destroyed."
	// [OBJTRACK] "OBJ ERROR : VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT object 0x8ea68afdf0 has not been destroyed."
	// [OBJTRACK] "OBJ ERROR : VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT object 0x6313f2bc50 has not been destroyed."
	// [OBJTRACK] "OBJ ERROR : VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT object 0x146da272a0 has not been destroyed."
	if (	MatchStrings( pMsg, "Mem Object 148617029248 has not been freed. You should clean up this memory by calling vkFreeMemory(148617029248) prior to vkDestroyDevice()." ) ||
			MatchStrings( pMsg, "OBJ ERROR : VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT object 0x7a613498a0 has not been destroyed." ) ||
			MatchStrings( pMsg, "OBJ ERROR : VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT object 0x8ea68afdf0 has not been destroyed." ) ||
			MatchStrings( pMsg, "OBJ ERROR : VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT object 0x6313f2bc50 has not been destroyed." ) ||
			MatchStrings( pMsg, "OBJ ERROR : VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT object 0x146da272a0 has not been destroyed." ) )
	{
		return VK_FALSE;
	}
	// The draw state validation layer incorrectly reports that a query is unavailable.
	// https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/367
	// [DS] Cannot get query results on queryPool 408912669008 with index 4 which is unavailable.
	if ( MatchStrings( pMsg, "Cannot get query results on queryPool 408912669008 with index 4 which is unavailable." ) )
	{
		return VK_FALSE;
	}
	// The core validation layer incorrectly states that layouts are not compatible.
	// https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/450
	// [DS] "Error: [DS] Code 42 : descriptorSet #0 being bound is not compatible with overlapping descriptorSetLayout at index 0 of pipelineLayout 0x42 due to: Binding 0 for DescriptorSetLayout 0000000000000041 has a descriptorCount of 1 but binding 0 for DescriptorSetLayout 000000000000003D has a descriptorCount of 4"
	if ( MatchStrings( pMsg, "Error: [DS] Code 42 : descriptorSet #0 being bound is not compatible with overlapping descriptorSetLayout at index 0 of pipelineLayout 0x42 due to: Binding 0 for DescriptorSetLayout 0000000000000041 has a descriptorCount of 1 but binding 0 for DescriptorSetLayout 000000000000003D has a descriptorCount of 4" ) )
	{
		return VK_FALSE;
	}

	const bool warning = ( msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) == 0 ? true : false;
	Error( "%s: [%s] Code %d : %s", warning ? "Warning" : "Error", pLayerPrefix, msgCode, pMsg );
	return VK_FALSE;
}

#define GET_INSTANCE_PROC_ADDR_EXP( function )	instance->function = (PFN_##function)( instance->vkGetInstanceProcAddr( instance->instance, #function ) ); assert( instance->function != NULL );
#define GET_INSTANCE_PROC_ADDR( function )		GET_INSTANCE_PROC_ADDR_EXP( function )

typedef struct
{
	const char *	name;
	bool			validationOnly;
	bool			required;
} DriverFeature_t;

// Returns true if all the required features are present.
static bool CheckFeatures( const char * label, const bool validationEnabled, const bool extensions,
							const DriverFeature_t * requested, const uint32_t requestedCount,
							const void * available, const uint32_t availableCount,
							const char * enabledNames[], uint32_t * enabledCount )
{
	bool foundAllRequired = true;
	*enabledCount = 0;
	for ( uint32_t i = 0; i < requestedCount; i++ )
	{
		bool found = false;
		const char * result = requested[i].required ? "(required, not found)" : "(not found)";
		for ( uint32_t j = 0; j < availableCount; j++ )
		{
			const char * name = extensions ?
									((const VkExtensionProperties *)available)[j].extensionName :
									((const VkLayerProperties *)available)[j].layerName;
			if ( strcmp( requested[i].name, name ) == 0 )
			{
				found = true;
				if ( requested[i].validationOnly && !validationEnabled )
				{
					result = "(not enabled)";
					break;
				}
				enabledNames[(*enabledCount)++] = requested[i].name;
				result = requested[i].required ? "(required, enabled)" : "(enabled)";
				break;
			}
		}
		foundAllRequired &= ( found || !requested[i].required );
		Print( "%-21s%c %s %s\n", ( i == 0 ? label : "" ), ( i == 0 ? ':' : ' ' ), requested[i].name, result );
	}
	return foundAllRequired;
}

static bool DriverInstance_Create( DriverInstance_t * instance )
{
	memset( instance, 0, sizeof( DriverInstance_t ) );

#if defined( _DEBUG )
	instance->validate = VK_TRUE;
#else
	instance->validate = VK_FALSE;
#endif

	// Get the global functions.
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
#elif defined( OS_LINUX ) || defined( OS_ANDROID ) || defined( OS_MAC )
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

	Print( "--------------------------------\n" );

	// Get the instance extensions.
	const DriverFeature_t requestedExtensions[] =
	{
		{ VK_KHR_SURFACE_EXTENSION_NAME,			false, true },
		{ VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME,	false, true },
		{ VK_EXT_DEBUG_REPORT_EXTENSION_NAME,		true, false },
	};

	const char * enabledExtensionNames[32] = { 0 };
	uint32_t enabledExtensionCount = 0;
	bool requiedExtensionsAvailable = true;
	{
		uint32_t availableExtensionCount = 0;
		VK( instance->vkEnumerateInstanceExtensionProperties( NULL, &availableExtensionCount, NULL ) );

		VkExtensionProperties * availableExtensions = malloc( availableExtensionCount * sizeof( VkExtensionProperties ) );
		VK( instance->vkEnumerateInstanceExtensionProperties( NULL, &availableExtensionCount, availableExtensions ) );

		requiedExtensionsAvailable = CheckFeatures( "Instance Extensions", instance->validate, true,
													requestedExtensions, ARRAY_SIZE( requestedExtensions ),
													availableExtensions, availableExtensionCount,
													enabledExtensionNames, &enabledExtensionCount );

		free( availableExtensions );
	}
	if ( !requiedExtensionsAvailable )
	{
		Print( "Required instance extensions not supported.\n" );
	}

	// Get the instance layers.
	const DriverFeature_t requestedLayers[] =
	{
		{ "VK_LAYER_OCULUS_queue_muxer",			false, false },
		{ "VK_LAYER_GOOGLE_threading",				true, false },
		{ "VK_LAYER_LUNARG_parameter_validation",	true, false },
		{ "VK_LAYER_LUNARG_object_tracker",			true, false },
		{ "VK_LAYER_LUNARG_core_validation",		true, false },
		{ "VK_LAYER_LUNARG_device_limits",			true, false },
		{ "VK_LAYER_LUNARG_image",					true, false },
		{ "VK_LAYER_LUNARG_swapchain",				true, false },
		{ "VK_LAYER_GOOGLE_unique_objects",			true, false },
#if USE_API_DUMP == 1
		{ "VK_LAYER_LUNARG_api_dump",				true, false },
#endif
	};

	const char * enabledLayerNames[32] = { 0 };
	uint32_t enabledLayerCount = 0;
	bool requiredLayersAvailable = true;
	{
		uint32_t availableLayerCount = 0;
		VK( instance->vkEnumerateInstanceLayerProperties( &availableLayerCount, NULL ) );

		VkLayerProperties * availableLayers = malloc( availableLayerCount * sizeof( VkLayerProperties ) );
		VK( instance->vkEnumerateInstanceLayerProperties( &availableLayerCount, availableLayers ) );

		requiredLayersAvailable = CheckFeatures( "Instance Layers", instance->validate, false,
													requestedLayers, ARRAY_SIZE( requestedLayers ),
													availableLayers, availableLayerCount,
													enabledLayerNames, &enabledLayerCount );

		free( availableLayers );
	}
	if ( !requiredLayersAvailable )
	{
		Print( "Required instance layers not supported.\n" );
	}

	const uint32_t apiMajor = VK_VERSION_MAJOR( VK_API_VERSION_1_0 );
	const uint32_t apiMinor = VK_VERSION_MINOR( VK_API_VERSION_1_0 );
	const uint32_t apiPatch = VK_VERSION_PATCH( VK_API_VERSION_1_0 );
	Print( "Instance API version : %d.%d.%d\n", apiMajor, apiMinor, apiPatch );

	Print( "--------------------------------\n" );

	// Create the instance.
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
	instanceCreateInfo.enabledLayerCount = enabledLayerCount;
	instanceCreateInfo.ppEnabledLayerNames = (const char * const *) ( enabledLayerCount != 0 ? enabledLayerNames : NULL );
	instanceCreateInfo.enabledExtensionCount = enabledExtensionCount;
	instanceCreateInfo.ppEnabledExtensionNames = (const char * const *) ( enabledExtensionCount != 0 ? enabledExtensionNames : NULL );

	VK( instance->vkCreateInstance( &instanceCreateInfo, VK_ALLOCATOR, &instance->instance ) );

	// Get the instance functions.
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
	GET_INSTANCE_PROC_ADDR( vkCreateDevice );
	GET_INSTANCE_PROC_ADDR( vkGetDeviceProcAddr );

	// Get the surface extension functions.
	GET_INSTANCE_PROC_ADDR( vkCreateSurfaceKHR );
	GET_INSTANCE_PROC_ADDR( vkDestroySurfaceKHR );
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceSurfaceSupportKHR );
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceSurfaceCapabilitiesKHR );
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceSurfaceFormatsKHR );
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceSurfacePresentModesKHR );

	if ( instance->validate )
	{
		GET_INSTANCE_PROC_ADDR( vkCreateDebugReportCallbackEXT );
		GET_INSTANCE_PROC_ADDR( vkDestroyDebugReportCallbackEXT );
		if ( instance->vkCreateDebugReportCallbackEXT != NULL )
		{
			VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo;
			debugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
			debugReportCallbackCreateInfo.pNext = NULL;
			debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			debugReportCallbackCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)DebugReportCallback;
			debugReportCallbackCreateInfo.pUserData = NULL;

			VK( instance->vkCreateDebugReportCallbackEXT( instance->instance, &debugReportCallbackCreateInfo, VK_ALLOCATOR, &instance->debugReportCallback ) );
		}
	}

	return true;
}

static void DriverInstance_Destroy( DriverInstance_t * instance )
{
	if ( instance->validate && instance->vkDestroyDebugReportCallbackEXT != NULL )
	{
		VC( instance->vkDestroyDebugReportCallbackEXT( instance->instance, instance->debugReportCallback, VK_ALLOCATOR ) );
	}

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

GPU device.

GpuQueueProperty_t
GpuQueuePriority_t
GpuQueueInfo_t
GpuDevice_t

static bool GpuDevice_Create( GpuDevice_t * device, DriverInstance_t * instance,
							const GpuQueueInfo_t * queueInfo, const VkSurfaceKHR presentSurface );
static void GpuDevice_Destroy( GpuDevice_t * device );

================================================================================================================================
*/

typedef enum
{
	GPU_QUEUE_PROPERTY_GRAPHICS		= BIT( 0 ),
	GPU_QUEUE_PROPERTY_COMPUTE		= BIT( 1 ),
	GPU_QUEUE_PROPERTY_TRANSFER		= BIT( 2 )
} GpuQueueProperty_t;

typedef enum
{
	GPU_QUEUE_PRIORITY_LOW,
	GPU_QUEUE_PRIORITY_MEDIUM,
	GPU_QUEUE_PRIORITY_HIGH
} GpuQueuePriority_t;

#define MAX_QUEUES	16

typedef struct
{
	int					queueCount;						// number of queues
	GpuQueueProperty_t	queueProperties;				// desired queue family properties
	GpuQueuePriority_t	queuePriorities[MAX_QUEUES];	// individual queue priorities
} GpuQueueInfo_t;

typedef struct
{
	VkBool32								foundSwapchainExtension;
	DriverInstance_t *						instance;
	VkDevice								device;
	VkPhysicalDevice						physicalDevice;
	VkPhysicalDeviceFeatures				physicalDeviceFeatures;
	VkPhysicalDeviceProperties				physicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties		physicalDeviceMemoryProperties;
	uint32_t								queueFamilyCount;
	VkQueueFamilyProperties *				queueFamilyProperties;
	uint32_t *								queueFamilyUsedQueues;
	Mutex_t									queueFamilyMutex;
	int										workQueueFamilyIndex;
	int										presentQueueFamilyIndex;

	// Device functions.
	PFN_vkDestroyDevice						vkDestroyDevice;
	PFN_vkGetDeviceQueue					vkGetDeviceQueue;
	PFN_vkQueueSubmit						vkQueueSubmit;
	PFN_vkQueueWaitIdle						vkQueueWaitIdle;
	PFN_vkDeviceWaitIdle					vkDeviceWaitIdle;
	PFN_vkAllocateMemory					vkAllocateMemory;
	PFN_vkFreeMemory						vkFreeMemory;
	PFN_vkMapMemory							vkMapMemory;
	PFN_vkUnmapMemory						vkUnmapMemory;
	PFN_vkFlushMappedMemoryRanges			vkFlushMappedMemoryRanges;
	PFN_vkInvalidateMappedMemoryRanges		vkInvalidateMappedMemoryRanges;
	PFN_vkGetDeviceMemoryCommitment			vkGetDeviceMemoryCommitment;
	PFN_vkBindBufferMemory					vkBindBufferMemory;
	PFN_vkBindImageMemory					vkBindImageMemory;
	PFN_vkGetBufferMemoryRequirements		vkGetBufferMemoryRequirements;
	PFN_vkGetImageMemoryRequirements		vkGetImageMemoryRequirements;
	PFN_vkGetImageSparseMemoryRequirements	vkGetImageSparseMemoryRequirements;
	PFN_vkQueueBindSparse					vkQueueBindSparse;
	PFN_vkCreateFence						vkCreateFence;
	PFN_vkDestroyFence						vkDestroyFence;
	PFN_vkResetFences						vkResetFences;
	PFN_vkGetFenceStatus					vkGetFenceStatus;
	PFN_vkWaitForFences						vkWaitForFences;
	PFN_vkCreateSemaphore					vkCreateSemaphore;
	PFN_vkDestroySemaphore					vkDestroySemaphore;
	PFN_vkCreateEvent						vkCreateEvent;
	PFN_vkDestroyEvent						vkDestroyEvent;
	PFN_vkGetEventStatus					vkGetEventStatus;
	PFN_vkSetEvent							vkSetEvent;
	PFN_vkResetEvent						vkResetEvent;
	PFN_vkCreateQueryPool					vkCreateQueryPool;
	PFN_vkDestroyQueryPool					vkDestroyQueryPool;
	PFN_vkGetQueryPoolResults				vkGetQueryPoolResults;
	PFN_vkCreateBuffer						vkCreateBuffer;
	PFN_vkDestroyBuffer						vkDestroyBuffer;
	PFN_vkCreateBufferView					vkCreateBufferView;
	PFN_vkDestroyBufferView					vkDestroyBufferView;
	PFN_vkCreateImage						vkCreateImage;
	PFN_vkDestroyImage						vkDestroyImage;
	PFN_vkGetImageSubresourceLayout			vkGetImageSubresourceLayout;
	PFN_vkCreateImageView					vkCreateImageView;
	PFN_vkDestroyImageView					vkDestroyImageView;
	PFN_vkCreateShaderModule				vkCreateShaderModule;
	PFN_vkDestroyShaderModule				vkDestroyShaderModule;
	PFN_vkCreatePipelineCache				vkCreatePipelineCache;
	PFN_vkDestroyPipelineCache				vkDestroyPipelineCache;
	PFN_vkGetPipelineCacheData				vkGetPipelineCacheData;
	PFN_vkMergePipelineCaches				vkMergePipelineCaches;
	PFN_vkCreateGraphicsPipelines			vkCreateGraphicsPipelines;
	PFN_vkCreateComputePipelines			vkCreateComputePipelines;
	PFN_vkDestroyPipeline					vkDestroyPipeline;
	PFN_vkCreatePipelineLayout				vkCreatePipelineLayout;
	PFN_vkDestroyPipelineLayout				vkDestroyPipelineLayout;
	PFN_vkCreateSampler						vkCreateSampler;
	PFN_vkDestroySampler					vkDestroySampler;
	PFN_vkCreateDescriptorSetLayout			vkCreateDescriptorSetLayout;
	PFN_vkDestroyDescriptorSetLayout		vkDestroyDescriptorSetLayout;
	PFN_vkCreateDescriptorPool				vkCreateDescriptorPool;
	PFN_vkDestroyDescriptorPool				vkDestroyDescriptorPool;
	PFN_vkResetDescriptorPool				vkResetDescriptorPool;
	PFN_vkAllocateDescriptorSets			vkAllocateDescriptorSets;
	PFN_vkFreeDescriptorSets				vkFreeDescriptorSets;
	PFN_vkUpdateDescriptorSets				vkUpdateDescriptorSets;
	PFN_vkCreateFramebuffer					vkCreateFramebuffer;
	PFN_vkDestroyFramebuffer				vkDestroyFramebuffer;
	PFN_vkCreateRenderPass					vkCreateRenderPass;
	PFN_vkDestroyRenderPass					vkDestroyRenderPass;
	PFN_vkGetRenderAreaGranularity			vkGetRenderAreaGranularity;
	PFN_vkCreateCommandPool					vkCreateCommandPool;
	PFN_vkDestroyCommandPool				vkDestroyCommandPool;
	PFN_vkResetCommandPool					vkResetCommandPool;
	PFN_vkAllocateCommandBuffers			vkAllocateCommandBuffers;
	PFN_vkFreeCommandBuffers				vkFreeCommandBuffers;
	PFN_vkBeginCommandBuffer				vkBeginCommandBuffer;
	PFN_vkEndCommandBuffer					vkEndCommandBuffer;
	PFN_vkResetCommandBuffer				vkResetCommandBuffer;
	PFN_vkCmdBindPipeline					vkCmdBindPipeline;
	PFN_vkCmdSetViewport					vkCmdSetViewport;
	PFN_vkCmdSetScissor						vkCmdSetScissor;
	PFN_vkCmdSetLineWidth					vkCmdSetLineWidth;
	PFN_vkCmdSetDepthBias					vkCmdSetDepthBias;
	PFN_vkCmdSetBlendConstants				vkCmdSetBlendConstants;
	PFN_vkCmdSetDepthBounds					vkCmdSetDepthBounds;
	PFN_vkCmdSetStencilCompareMask			vkCmdSetStencilCompareMask;
	PFN_vkCmdSetStencilWriteMask			vkCmdSetStencilWriteMask;
	PFN_vkCmdSetStencilReference			vkCmdSetStencilReference;
	PFN_vkCmdBindDescriptorSets				vkCmdBindDescriptorSets;
	PFN_vkCmdBindIndexBuffer				vkCmdBindIndexBuffer;
	PFN_vkCmdBindVertexBuffers				vkCmdBindVertexBuffers;
	PFN_vkCmdDraw							vkCmdDraw;
	PFN_vkCmdDrawIndexed					vkCmdDrawIndexed;
	PFN_vkCmdDrawIndirect					vkCmdDrawIndirect;
	PFN_vkCmdDrawIndexedIndirect			vkCmdDrawIndexedIndirect;
	PFN_vkCmdDispatch						vkCmdDispatch;
	PFN_vkCmdDispatchIndirect				vkCmdDispatchIndirect;
	PFN_vkCmdCopyBuffer						vkCmdCopyBuffer;
	PFN_vkCmdCopyImage						vkCmdCopyImage;
	PFN_vkCmdBlitImage						vkCmdBlitImage;
	PFN_vkCmdCopyBufferToImage				vkCmdCopyBufferToImage;
	PFN_vkCmdCopyImageToBuffer				vkCmdCopyImageToBuffer;
	PFN_vkCmdUpdateBuffer					vkCmdUpdateBuffer;
	PFN_vkCmdFillBuffer						vkCmdFillBuffer;
	PFN_vkCmdClearColorImage				vkCmdClearColorImage;
	PFN_vkCmdClearDepthStencilImage			vkCmdClearDepthStencilImage;
	PFN_vkCmdClearAttachments				vkCmdClearAttachments;
	PFN_vkCmdResolveImage					vkCmdResolveImage;
	PFN_vkCmdSetEvent						vkCmdSetEvent;
	PFN_vkCmdResetEvent						vkCmdResetEvent;
	PFN_vkCmdWaitEvents						vkCmdWaitEvents;
	PFN_vkCmdPipelineBarrier				vkCmdPipelineBarrier;
	PFN_vkCmdBeginQuery						vkCmdBeginQuery;
	PFN_vkCmdEndQuery						vkCmdEndQuery;
	PFN_vkCmdResetQueryPool					vkCmdResetQueryPool;
	PFN_vkCmdWriteTimestamp					vkCmdWriteTimestamp;
	PFN_vkCmdCopyQueryPoolResults			vkCmdCopyQueryPoolResults;
	PFN_vkCmdPushConstants					vkCmdPushConstants;
	PFN_vkCmdBeginRenderPass				vkCmdBeginRenderPass;
	PFN_vkCmdNextSubpass					vkCmdNextSubpass;
	PFN_vkCmdEndRenderPass					vkCmdEndRenderPass;
	PFN_vkCmdExecuteCommands				vkCmdExecuteCommands;

	// Device extensions.
	PFN_vkCreateSwapchainKHR				vkCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR				vkDestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR				vkGetSwapchainImagesKHR;
	PFN_vkAcquireNextImageKHR				vkAcquireNextImageKHR;
	PFN_vkQueuePresentKHR					vkQueuePresentKHR;
} GpuDevice_t;

#define GET_DEVICE_PROC_ADDR_EXP( function )	device->function = (PFN_##function)( device->instance->vkGetDeviceProcAddr( device->device, #function ) ); assert( device->function != NULL );
#define GET_DEVICE_PROC_ADDR( function )		GET_DEVICE_PROC_ADDR_EXP( function )

static bool GpuDevice_Create( GpuDevice_t * device, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const VkSurfaceKHR presentSurface )
{
	memset( device, 0, sizeof( GpuDevice_t ) );

	device->instance = instance;

	//
	// Select an appropriate physical device
	//

	const VkQueueFlags requiredQueueFlags =
			( ( ( queueInfo->queueProperties & GPU_QUEUE_PROPERTY_GRAPHICS ) != 0 ) ? VK_QUEUE_GRAPHICS_BIT : 0 ) |
			( ( ( queueInfo->queueProperties & GPU_QUEUE_PROPERTY_COMPUTE ) != 0 ) ? VK_QUEUE_COMPUTE_BIT : 0 ) |
			( ( ( queueInfo->queueProperties & GPU_QUEUE_PROPERTY_TRANSFER ) != 0 &&
				( queueInfo->queueProperties & ( GPU_QUEUE_PROPERTY_GRAPHICS | GPU_QUEUE_PROPERTY_COMPUTE ) ) == 0 ) ? VK_QUEUE_TRANSFER_BIT : 0 );

	const char * enabledExtensionNames[32] = { 0 };
	uint32_t enabledExtensionCount = 0;

	const char * enabledLayerNames[32] = { 0 };
	uint32_t enabledLayerCount = 0;

	uint32_t physicalDeviceCount = 0;
	VK( instance->vkEnumeratePhysicalDevices( instance->instance, &physicalDeviceCount, NULL ) );

	VkPhysicalDevice * physicalDevices = (VkPhysicalDevice *) malloc( physicalDeviceCount * sizeof( VkPhysicalDevice ) );
	VK( instance->vkEnumeratePhysicalDevices( instance->instance, &physicalDeviceCount, physicalDevices ) );

	for ( uint32_t physicalDeviceIndex = 0; physicalDeviceIndex < physicalDeviceCount; physicalDeviceIndex++ )
	{
		// Get the device properties.
		VkPhysicalDeviceProperties physicalDeviceProperties;
		VC( instance->vkGetPhysicalDeviceProperties( physicalDevices[physicalDeviceIndex], &physicalDeviceProperties ) );

		const uint32_t driverMajor = VK_VERSION_MAJOR( physicalDeviceProperties.driverVersion );
		const uint32_t driverMinor = VK_VERSION_MINOR( physicalDeviceProperties.driverVersion );
		const uint32_t driverPatch = VK_VERSION_PATCH( physicalDeviceProperties.driverVersion );

		const uint32_t apiMajor = VK_VERSION_MAJOR( physicalDeviceProperties.apiVersion );
		const uint32_t apiMinor = VK_VERSION_MINOR( physicalDeviceProperties.apiVersion );
		const uint32_t apiPatch = VK_VERSION_PATCH( physicalDeviceProperties.apiVersion );

		Print( "--------------------------------\n" );
		Print( "Device Name          : %s\n", physicalDeviceProperties.deviceName );
		Print( "Device Type          : %s\n",	( ( physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ) ? "integrated GPU" :
												( ( physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) ? "discrete GPU" :
												( ( physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ) ? "virtual GPU" :
												( ( physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ) ? "CPU" : "unknown" ) ) ) ) );
		Print( "Vendor ID            : 0x%04X\n", physicalDeviceProperties.vendorID );		// http://pcidatabase.com
		Print( "Device ID            : 0x%04X\n", physicalDeviceProperties.deviceID );
		Print( "Driver Version       : %d.%d.%d\n", driverMajor, driverMinor, driverPatch );
		Print( "API Version          : %d.%d.%d\n", apiMajor, apiMinor, apiPatch );

		// Get the queue families.
		uint32_t queueFamilyCount = 0;
		VC( instance->vkGetPhysicalDeviceQueueFamilyProperties( physicalDevices[physicalDeviceIndex], &queueFamilyCount, NULL ) );

		VkQueueFamilyProperties * queueFamilyProperties = (VkQueueFamilyProperties *) malloc( queueFamilyCount * sizeof( VkQueueFamilyProperties ) );
		VC( instance->vkGetPhysicalDeviceQueueFamilyProperties( physicalDevices[physicalDeviceIndex], &queueFamilyCount, queueFamilyProperties ) );

		for ( uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++ )
		{
			const VkQueueFlags queueFlags = queueFamilyProperties[queueFamilyIndex].queueFlags;
			Print( "%-21s%c %d =%s%s%s (%d queues, %d priorities)\n",
							( queueFamilyIndex == 0 ? "Queue Families" : "" ),
							( queueFamilyIndex == 0 ? ':' : ' ' ),
							queueFamilyIndex,
							( queueFlags & VK_QUEUE_GRAPHICS_BIT ) ? " graphics" : "",
							( queueFlags & VK_QUEUE_COMPUTE_BIT )  ? " compute"  : "",
							( queueFlags & VK_QUEUE_TRANSFER_BIT ) ? " transfer" : "",
							queueFamilyProperties[queueFamilyIndex].queueCount,
							physicalDeviceProperties.limits.discreteQueuePriorities );
		}

		// Check if this physical device supports the required queue families.
		int workQueueFamilyIndex = -1;
		int presentQueueFamilyIndex = -1;
		for ( uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++ )
		{
			if ( ( queueFamilyProperties[queueFamilyIndex].queueFlags & requiredQueueFlags ) == requiredQueueFlags )
			{
				if ( (int)queueFamilyProperties[queueFamilyIndex].queueCount >= queueInfo->queueCount )
				{
					workQueueFamilyIndex = queueFamilyIndex;
				}
			}
			if ( presentSurface != VK_NULL_HANDLE )
			{
				VkBool32 supportsPresent = VK_FALSE;
				VK( instance->vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevices[physicalDeviceIndex], queueFamilyIndex, presentSurface, &supportsPresent ) );
				if ( supportsPresent )
				{
					presentQueueFamilyIndex = queueFamilyIndex;
				}
			}
			if ( workQueueFamilyIndex != -1 && ( presentQueueFamilyIndex != -1 || presentSurface == VK_NULL_HANDLE ) )
			{
				break;
			}
		}
#if defined( OS_ANDROID )
		// On Android all devices must be able to present to the system compositor, and all queue families
		// must support the necessary image layout transitions and synchronization operations.
		presentQueueFamilyIndex = workQueueFamilyIndex;
#endif

		if ( workQueueFamilyIndex == -1 )
		{
			Print( "Required work queue family not supported.\n" );
			free( queueFamilyProperties );
			continue;
		}

		if ( presentQueueFamilyIndex == -1 && presentSurface != VK_NULL_HANDLE )
		{
			Print( "Required present queue family not supported.\n" );
			free( queueFamilyProperties );
			continue;
		}

		Print( "Work Queue Family    : %d\n", workQueueFamilyIndex );
		Print( "Present Queue Family : %d\n", presentQueueFamilyIndex );

		const DriverFeature_t requestedExtensions[] =
		{
			{ VK_KHR_SWAPCHAIN_EXTENSION_NAME,	false, true },
			{ "VK_NV_glsl_shader",				false, false },
		};

		// Check the device extensions.
		bool requiedExtensionsAvailable = true;
		{
			uint32_t availableExtensionCount = 0;
			VK( instance->vkEnumerateDeviceExtensionProperties( physicalDevices[physicalDeviceIndex], NULL, &availableExtensionCount, NULL ) );

			VkExtensionProperties * availableExtensions = malloc( availableExtensionCount * sizeof( VkExtensionProperties ) );
			VK( instance->vkEnumerateDeviceExtensionProperties( physicalDevices[physicalDeviceIndex], NULL, &availableExtensionCount, availableExtensions ) );

			requiedExtensionsAvailable = CheckFeatures( "Device Extensions", instance->validate, true,
														requestedExtensions, ARRAY_SIZE( requestedExtensions ),
														availableExtensions, availableExtensionCount,
														enabledExtensionNames, &enabledExtensionCount );

			free( availableExtensions );
		}

		if ( !requiedExtensionsAvailable )
		{
			Print( "Required device extensions not supported.\n" );
			free( queueFamilyProperties );
			continue;
		}

		// Check the device layers.
		const DriverFeature_t requestedLayers[] =
		{
			{ "VK_LAYER_OCULUS_queue_muxer",			false, false },
			{ "VK_LAYER_GOOGLE_threading",				true, false },
			{ "VK_LAYER_LUNARG_parameter_validation",	true, false },
			{ "VK_LAYER_LUNARG_object_tracker",			true, false },
			{ "VK_LAYER_LUNARG_core_validation",		true, false },
			{ "VK_LAYER_LUNARG_device_limits",			true, false },
			{ "VK_LAYER_LUNARG_image",					true, false },
			{ "VK_LAYER_LUNARG_swapchain",				true, false },
			{ "VK_LAYER_GOOGLE_unique_objects",			true, false },
#if USE_API_DUMP == 1
			{ "VK_LAYER_LUNARG_api_dump",				true, false },
#endif
		};

		bool requiredLayersAvailable = true;
		{
			uint32_t availableLayerCount = 0;
			VK( instance->vkEnumerateDeviceLayerProperties( physicalDevices[physicalDeviceIndex], &availableLayerCount, NULL ) );

			VkLayerProperties * availableLayers = malloc( availableLayerCount * sizeof( VkLayerProperties ) );
			VK( instance->vkEnumerateDeviceLayerProperties( physicalDevices[physicalDeviceIndex], &availableLayerCount, availableLayers ) );

			requiredLayersAvailable = CheckFeatures( "Device Layers", instance->validate, false,
													requestedLayers, ARRAY_SIZE( requestedLayers ),
													availableLayers, availableLayerCount,
													enabledLayerNames, &enabledLayerCount );

			free( availableLayers );
		}

		if ( !requiredLayersAvailable )
		{
			Print( "Required device layers not supported.\n" );
			free( queueFamilyProperties );
			continue;
		}

		device->foundSwapchainExtension = requiredLayersAvailable;
		device->physicalDevice = physicalDevices[physicalDeviceIndex];
		device->queueFamilyCount = queueFamilyCount;
		device->queueFamilyProperties = queueFamilyProperties;
		device->workQueueFamilyIndex = workQueueFamilyIndex;
		device->presentQueueFamilyIndex = presentQueueFamilyIndex;

		VC( instance->vkGetPhysicalDeviceFeatures( physicalDevices[physicalDeviceIndex], &device->physicalDeviceFeatures ) );
		VC( instance->vkGetPhysicalDeviceProperties( physicalDevices[physicalDeviceIndex], &device->physicalDeviceProperties ) );
		VC( instance->vkGetPhysicalDeviceMemoryProperties( physicalDevices[physicalDeviceIndex], &device->physicalDeviceMemoryProperties ) );

		break;
	}

	Print( "--------------------------------\n" );

	if ( device->physicalDevice == VK_NULL_HANDLE )
	{
		Error( "No capable Vulkan physical device found." );
		return false;
	}

	// Allocate a bit mask for the available queues per family.
	device->queueFamilyUsedQueues = (uint32_t *) malloc( device->queueFamilyCount * sizeof( uint32_t ) );
	for ( uint32_t queueFamilyIndex = 0; queueFamilyIndex < device->queueFamilyCount; queueFamilyIndex++ )
	{
		device->queueFamilyUsedQueues[queueFamilyIndex] = 0xFFFFFFFF << device->queueFamilyProperties[queueFamilyIndex].queueCount;
	}

	Mutex_Create( &device->queueFamilyMutex );

	//
	// Create the logical device
	//

	float floatPriorities[MAX_QUEUES];
	for ( int i = 0; i < queueInfo->queueCount; i++ )
	{
		const uint32_t discreteQueuePriorities = device->physicalDeviceProperties.limits.discreteQueuePriorities;
		switch ( queueInfo->queuePriorities[i] )
		{
			case GPU_QUEUE_PRIORITY_LOW:	floatPriorities[i] = 0.0f; break;
			case GPU_QUEUE_PRIORITY_MEDIUM:	floatPriorities[i] = ( discreteQueuePriorities <= 2 ) ? 0.0f : 0.5f; break;
			case GPU_QUEUE_PRIORITY_HIGH:	floatPriorities[i] = 1.0f; break;
		}
	}

	// Create the device.
	VkDeviceQueueCreateInfo deviceQueueCreateInfo[2];
	deviceQueueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo[0].pNext = NULL;
	deviceQueueCreateInfo[0].flags = 0;
	deviceQueueCreateInfo[0].queueFamilyIndex = device->workQueueFamilyIndex;
	deviceQueueCreateInfo[0].queueCount = queueInfo->queueCount;
	deviceQueueCreateInfo[0].pQueuePriorities = floatPriorities;

	deviceQueueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo[1].pNext = NULL;
	deviceQueueCreateInfo[1].flags = 0;
	deviceQueueCreateInfo[1].queueFamilyIndex = device->presentQueueFamilyIndex;
	deviceQueueCreateInfo[1].queueCount = 1;
	deviceQueueCreateInfo[1].pQueuePriorities = NULL;

	VkDeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = NULL;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = 1 + ( device->presentQueueFamilyIndex != -1 && device->presentQueueFamilyIndex != device->workQueueFamilyIndex );
	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfo;
	deviceCreateInfo.enabledLayerCount = enabledLayerCount;
	deviceCreateInfo.ppEnabledLayerNames = (const char * const *) ( enabledLayerCount != 0 ? enabledLayerNames : NULL );
	deviceCreateInfo.enabledExtensionCount = enabledExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = (const char * const *) ( enabledExtensionCount != 0 ? enabledExtensionNames : NULL );
	deviceCreateInfo.pEnabledFeatures = NULL;

	VK( instance->vkCreateDevice( device->physicalDevice, &deviceCreateInfo, VK_ALLOCATOR, &device->device ) );

	//
	// Setup the device specific function pointers
	//

	// Get the device functions.
	GET_DEVICE_PROC_ADDR( vkDestroyDevice );
	GET_DEVICE_PROC_ADDR( vkGetDeviceQueue );
	GET_DEVICE_PROC_ADDR( vkQueueSubmit );
	GET_DEVICE_PROC_ADDR( vkQueueWaitIdle );
	GET_DEVICE_PROC_ADDR( vkDeviceWaitIdle );
	GET_DEVICE_PROC_ADDR( vkAllocateMemory );
	GET_DEVICE_PROC_ADDR( vkFreeMemory );
	GET_DEVICE_PROC_ADDR( vkMapMemory );
	GET_DEVICE_PROC_ADDR( vkUnmapMemory );
	GET_DEVICE_PROC_ADDR( vkFlushMappedMemoryRanges );
	GET_DEVICE_PROC_ADDR( vkInvalidateMappedMemoryRanges );
	GET_DEVICE_PROC_ADDR( vkGetDeviceMemoryCommitment );
	GET_DEVICE_PROC_ADDR( vkBindBufferMemory );
	GET_DEVICE_PROC_ADDR( vkBindImageMemory );
	GET_DEVICE_PROC_ADDR( vkGetBufferMemoryRequirements );
	GET_DEVICE_PROC_ADDR( vkGetImageMemoryRequirements );
	GET_DEVICE_PROC_ADDR( vkGetImageSparseMemoryRequirements );
	GET_DEVICE_PROC_ADDR( vkQueueBindSparse );
	GET_DEVICE_PROC_ADDR( vkCreateFence );
	GET_DEVICE_PROC_ADDR( vkDestroyFence );
	GET_DEVICE_PROC_ADDR( vkResetFences );
	GET_DEVICE_PROC_ADDR( vkGetFenceStatus );
	GET_DEVICE_PROC_ADDR( vkWaitForFences );
	GET_DEVICE_PROC_ADDR( vkCreateSemaphore );
	GET_DEVICE_PROC_ADDR( vkDestroySemaphore );
	GET_DEVICE_PROC_ADDR( vkCreateEvent );
	GET_DEVICE_PROC_ADDR( vkDestroyEvent );
	GET_DEVICE_PROC_ADDR( vkGetEventStatus );
	GET_DEVICE_PROC_ADDR( vkSetEvent );
	GET_DEVICE_PROC_ADDR( vkResetEvent );
	GET_DEVICE_PROC_ADDR( vkCreateQueryPool );
	GET_DEVICE_PROC_ADDR( vkDestroyQueryPool );
	GET_DEVICE_PROC_ADDR( vkGetQueryPoolResults );
	GET_DEVICE_PROC_ADDR( vkCreateBuffer );
	GET_DEVICE_PROC_ADDR( vkDestroyBuffer );
	GET_DEVICE_PROC_ADDR( vkCreateBufferView );
	GET_DEVICE_PROC_ADDR( vkDestroyBufferView );
	GET_DEVICE_PROC_ADDR( vkCreateImage );
	GET_DEVICE_PROC_ADDR( vkDestroyImage );
	GET_DEVICE_PROC_ADDR( vkGetImageSubresourceLayout );
	GET_DEVICE_PROC_ADDR( vkCreateImageView );
	GET_DEVICE_PROC_ADDR( vkDestroyImageView );
	GET_DEVICE_PROC_ADDR( vkCreateShaderModule );
	GET_DEVICE_PROC_ADDR( vkDestroyShaderModule );
	GET_DEVICE_PROC_ADDR( vkCreatePipelineCache );
	GET_DEVICE_PROC_ADDR( vkDestroyPipelineCache );
	GET_DEVICE_PROC_ADDR( vkGetPipelineCacheData );
	GET_DEVICE_PROC_ADDR( vkMergePipelineCaches );
	GET_DEVICE_PROC_ADDR( vkCreateGraphicsPipelines );
	GET_DEVICE_PROC_ADDR( vkCreateComputePipelines );
	GET_DEVICE_PROC_ADDR( vkDestroyPipeline );
	GET_DEVICE_PROC_ADDR( vkCreatePipelineLayout );
	GET_DEVICE_PROC_ADDR( vkDestroyPipelineLayout );
	GET_DEVICE_PROC_ADDR( vkCreateSampler );
	GET_DEVICE_PROC_ADDR( vkDestroySampler );
	GET_DEVICE_PROC_ADDR( vkCreateDescriptorSetLayout );
	GET_DEVICE_PROC_ADDR( vkDestroyDescriptorSetLayout );
	GET_DEVICE_PROC_ADDR( vkCreateDescriptorPool );
	GET_DEVICE_PROC_ADDR( vkDestroyDescriptorPool );
	GET_DEVICE_PROC_ADDR( vkResetDescriptorPool );
	GET_DEVICE_PROC_ADDR( vkAllocateDescriptorSets );
	GET_DEVICE_PROC_ADDR( vkFreeDescriptorSets );
	GET_DEVICE_PROC_ADDR( vkUpdateDescriptorSets );
	GET_DEVICE_PROC_ADDR( vkCreateFramebuffer );
	GET_DEVICE_PROC_ADDR( vkDestroyFramebuffer );
	GET_DEVICE_PROC_ADDR( vkCreateRenderPass );
	GET_DEVICE_PROC_ADDR( vkDestroyRenderPass );
	GET_DEVICE_PROC_ADDR( vkGetRenderAreaGranularity );
	GET_DEVICE_PROC_ADDR( vkCreateCommandPool );
	GET_DEVICE_PROC_ADDR( vkDestroyCommandPool );
	GET_DEVICE_PROC_ADDR( vkResetCommandPool );
	GET_DEVICE_PROC_ADDR( vkAllocateCommandBuffers );
	GET_DEVICE_PROC_ADDR( vkFreeCommandBuffers );
	GET_DEVICE_PROC_ADDR( vkBeginCommandBuffer );
	GET_DEVICE_PROC_ADDR( vkEndCommandBuffer );
	GET_DEVICE_PROC_ADDR( vkResetCommandBuffer );
	GET_DEVICE_PROC_ADDR( vkCmdBindPipeline );
	GET_DEVICE_PROC_ADDR( vkCmdSetViewport );
	GET_DEVICE_PROC_ADDR( vkCmdSetScissor );
	GET_DEVICE_PROC_ADDR( vkCmdSetLineWidth );
	GET_DEVICE_PROC_ADDR( vkCmdSetDepthBias );
	GET_DEVICE_PROC_ADDR( vkCmdSetBlendConstants );
	GET_DEVICE_PROC_ADDR( vkCmdSetDepthBounds );
	GET_DEVICE_PROC_ADDR( vkCmdSetStencilCompareMask );
	GET_DEVICE_PROC_ADDR( vkCmdSetStencilWriteMask );
	GET_DEVICE_PROC_ADDR( vkCmdSetStencilReference );
	GET_DEVICE_PROC_ADDR( vkCmdBindDescriptorSets );
	GET_DEVICE_PROC_ADDR( vkCmdBindIndexBuffer );
	GET_DEVICE_PROC_ADDR( vkCmdBindVertexBuffers );
	GET_DEVICE_PROC_ADDR( vkCmdDraw );
	GET_DEVICE_PROC_ADDR( vkCmdDrawIndexed );
	GET_DEVICE_PROC_ADDR( vkCmdDrawIndirect );
	GET_DEVICE_PROC_ADDR( vkCmdDrawIndexedIndirect );
	GET_DEVICE_PROC_ADDR( vkCmdDispatch );
	GET_DEVICE_PROC_ADDR( vkCmdDispatchIndirect );
	GET_DEVICE_PROC_ADDR( vkCmdCopyBuffer );
	GET_DEVICE_PROC_ADDR( vkCmdCopyImage );
	GET_DEVICE_PROC_ADDR( vkCmdBlitImage );
	GET_DEVICE_PROC_ADDR( vkCmdCopyBufferToImage );
	GET_DEVICE_PROC_ADDR( vkCmdCopyImageToBuffer );
	GET_DEVICE_PROC_ADDR( vkCmdUpdateBuffer );
	GET_DEVICE_PROC_ADDR( vkCmdFillBuffer );
	GET_DEVICE_PROC_ADDR( vkCmdClearColorImage );
	GET_DEVICE_PROC_ADDR( vkCmdClearDepthStencilImage );
	GET_DEVICE_PROC_ADDR( vkCmdClearAttachments );
	GET_DEVICE_PROC_ADDR( vkCmdResolveImage );
	GET_DEVICE_PROC_ADDR( vkCmdSetEvent );
	GET_DEVICE_PROC_ADDR( vkCmdResetEvent );
	GET_DEVICE_PROC_ADDR( vkCmdWaitEvents );
	GET_DEVICE_PROC_ADDR( vkCmdPipelineBarrier );
	GET_DEVICE_PROC_ADDR( vkCmdBeginQuery );
	GET_DEVICE_PROC_ADDR( vkCmdEndQuery );
	GET_DEVICE_PROC_ADDR( vkCmdResetQueryPool );
	GET_DEVICE_PROC_ADDR( vkCmdWriteTimestamp );
	GET_DEVICE_PROC_ADDR( vkCmdCopyQueryPoolResults );
	GET_DEVICE_PROC_ADDR( vkCmdPushConstants );
	GET_DEVICE_PROC_ADDR( vkCmdBeginRenderPass );
	GET_DEVICE_PROC_ADDR( vkCmdNextSubpass );
	GET_DEVICE_PROC_ADDR( vkCmdEndRenderPass );
	GET_DEVICE_PROC_ADDR( vkCmdExecuteCommands );

	// Get the swapchain extension functions.
	if ( device->foundSwapchainExtension )
	{
		GET_DEVICE_PROC_ADDR( vkCreateSwapchainKHR );
		GET_DEVICE_PROC_ADDR( vkCreateSwapchainKHR );
		GET_DEVICE_PROC_ADDR( vkDestroySwapchainKHR );
		GET_DEVICE_PROC_ADDR( vkGetSwapchainImagesKHR );
		GET_DEVICE_PROC_ADDR( vkAcquireNextImageKHR );
		GET_DEVICE_PROC_ADDR( vkQueuePresentKHR );
	}

	return true;
}

static void GpuDevice_Destroy( GpuDevice_t * device )
{
	VK( device->vkDeviceWaitIdle( device->device ) );

	free( device->queueFamilyProperties );
	free( device->queueFamilyUsedQueues );

	Mutex_Destroy( &device->queueFamilyMutex );

	VC( device->vkDestroyDevice( device->device, VK_ALLOCATOR ) );
}

static uint32_t GpuDevice_GetMemoryTypeIndex( GpuDevice_t * device, const uint32_t typeBits,
											const VkMemoryPropertyFlags requiredProperties )
{
	// Search memory types to find the index with the requested properties.
	for ( uint32_t type = 0; type < device->physicalDeviceMemoryProperties.memoryTypeCount; type++ )
	{
		if ( ( typeBits & ( 1 << type ) ) != 0 )
		{
			// Test if this memory type has the required properties.
			const VkFlags propertyFlags = device->physicalDeviceMemoryProperties.memoryTypes[type].propertyFlags;
			if ( ( propertyFlags & requiredProperties ) == requiredProperties )
			{
				return type;
			}
		}
	}
	Error( "Memory type %d with properties %d not found.", typeBits, requiredProperties );
	return 0;
}

static void GpuDevice_CreateShader( GpuDevice_t * device, VkShaderModule * shaderModule,
									const VkShaderStageFlagBits stage, const void * code, size_t codeSize )
{
	UNUSED_PARM( stage );

	VkShaderModuleCreateInfo moduleCreateInfo;
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.pNext = NULL;
	moduleCreateInfo.flags = 0;
	moduleCreateInfo.codeSize = 0;
	moduleCreateInfo.pCode = NULL;

#if USE_SPIRV == 1
	moduleCreateInfo.codeSize = codeSize;
	moduleCreateInfo.pCode = code;

	VK( device->vkCreateShaderModule( device->device, &moduleCreateInfo, VK_ALLOCATOR, shaderModule ) );
#else
	// Create fake SPV structure to feed GLSL to the driver "under the covers".
	// Try version 0 first: one of the VkShaderStageFlagBits followed by the GLSL text
	size_t tempCodeSize = 3 * sizeof( uint32_t ) + codeSize + 1;
	uint32_t * tempCode = (uint32_t *) malloc( tempCodeSize );
	tempCode[0] = ICD_SPV_MAGIC;
	tempCode[1] = 0;
	tempCode[2] = stage;
	memcpy( tempCode + 3, code, codeSize + 1 );

	moduleCreateInfo.codeSize = tempCodeSize;
	moduleCreateInfo.pCode = tempCode;

	VK( device->vkCreateShaderModule( device->device, &moduleCreateInfo, VK_ALLOCATOR, shaderModule ) );

	free( tempCode );
#endif
}

/*
================================================================================================================================

GPU context.

A context encapsulates a queue that is used to submit command buffers.
A context can only be used by a single thread.
For optimal performance, a context should only be created at load time, not at runtime.

GpuContext_t

static bool GpuContext_CreateShared( GpuContext_t * context, const GpuContext_t * other, const int queueIndex );
static void GpuContext_Destroy( GpuContext_t * context );
static void GpuContext_WaitIdle( GpuContext_t * context );

static bool GpuContext_Create( GpuContext_t * context, GpuDevice_t * device, const int queueIndex );

================================================================================================================================
*/

typedef struct
{
	GpuDevice_t *	device;
	uint32_t		queueFamilyIndex;
	uint32_t		queueIndex;
	VkQueue			queue;
	VkCommandPool	commandPool;
	VkPipelineCache	pipelineCache;
	VkCommandBuffer	setupCommandBuffer;
} GpuContext_t;

static bool GpuContext_Create( GpuContext_t * context, GpuDevice_t * device, const int queueIndex )
{
	memset( context, 0, sizeof( GpuContext_t ) );

	Mutex_Lock( &device->queueFamilyMutex, true );
	assert( ( device->queueFamilyUsedQueues[device->workQueueFamilyIndex] & ( 1 << queueIndex ) ) == 0 );
	device->queueFamilyUsedQueues[device->workQueueFamilyIndex] |= ( 1 << queueIndex );
	Mutex_Unlock( &device->queueFamilyMutex );

	context->device = device;
	context->queueFamilyIndex = device->workQueueFamilyIndex;
	context->queueIndex = queueIndex;

	VC( device->vkGetDeviceQueue( device->device, context->queueFamilyIndex, context->queueIndex, &context->queue ) );

	VkCommandPoolCreateInfo commandPoolCreateInfo;
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = NULL;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = context->queueFamilyIndex;

	VK( device->vkCreateCommandPool( device->device, &commandPoolCreateInfo, VK_ALLOCATOR, &context->commandPool ) );

	VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.pNext = NULL;
	pipelineCacheCreateInfo.flags = 0;
	pipelineCacheCreateInfo.initialDataSize = 0;
	pipelineCacheCreateInfo.pInitialData = NULL;

	VK( device->vkCreatePipelineCache( device->device, &pipelineCacheCreateInfo, VK_ALLOCATOR, &context->pipelineCache ) );

	return true;
}

static bool GpuContext_CreateShared( GpuContext_t * context, const GpuContext_t * other, const int queueIndex )
{
	return GpuContext_Create( context, other->device, queueIndex );
}

static void GpuContext_Destroy( GpuContext_t * context )
{
	if ( context->device == NULL )
	{
		return;
	}

	// Mark the queue as no longer in use.
	Mutex_Lock( &context->device->queueFamilyMutex, true );
	assert( ( context->device->queueFamilyUsedQueues[context->queueFamilyIndex] & ( 1 << context->queueIndex ) ) != 0 );
	context->device->queueFamilyUsedQueues[context->queueFamilyIndex] &= ~( 1 << context->queueIndex );
	Mutex_Unlock( &context->device->queueFamilyMutex );

	if ( context->setupCommandBuffer )
	{
		VC( context->device->vkFreeCommandBuffers( context->device->device, context->commandPool, 1, &context->setupCommandBuffer ) );
	}
	VC( context->device->vkDestroyCommandPool( context->device->device, context->commandPool, VK_ALLOCATOR ) );
	VC( context->device->vkDestroyPipelineCache( context->device->device, context->pipelineCache, VK_ALLOCATOR ) );
}

static void GpuContext_WaitIdle( GpuContext_t * context )
{
	VK( context->device->vkQueueWaitIdle( context->queue ) );
}

static void GpuContext_CreateSetupCmdBuffer( GpuContext_t * context )
{
	if ( context->setupCommandBuffer != VK_NULL_HANDLE )
	{
		return;
	}

	VkCommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = NULL;
	commandBufferAllocateInfo.commandPool = context->commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VK( context->device->vkAllocateCommandBuffers( context->device->device, &commandBufferAllocateInfo, &context->setupCommandBuffer ) );

	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = NULL;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = NULL;

	VK( context->device->vkBeginCommandBuffer( context->setupCommandBuffer, &commandBufferBeginInfo ) );
}

static void GpuContext_FlushSetupCmdBuffer( GpuContext_t * context )
{
    if ( context->setupCommandBuffer == VK_NULL_HANDLE )
	{
        return;
	}

	VK( context->device->vkEndCommandBuffer( context->setupCommandBuffer ) );

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = NULL;
	submitInfo.pWaitDstStageMask = NULL;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &context->setupCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = NULL;

	VK( context->device->vkQueueSubmit( context->queue, 1, &submitInfo, VK_NULL_HANDLE ) );
	VK( context->device->vkQueueWaitIdle( context->queue ) );

	VC( context->device->vkFreeCommandBuffers( context->device->device, context->commandPool, 1, &context->setupCommandBuffer ) );
	context->setupCommandBuffer = VK_NULL_HANDLE;
}

/*
================================================================================================================================

GPU swapchain.

This encapsulates a platform agnostic swapchain.
For optimal performance, a swapchain should only be created at load time, not at runtime.

GpuSwapchain_t

static bool GpuSwapchain_Create( GpuContext_t * context, GpuSwapchain_t * swapchain, const VkSurfaceKHR surface,
								const GpuSurfaceColorFormat_t colorFormat, const int width, const int height, const int swapInterval );
static void GpuSwapchain_Destroy( GpuContext_t * context, GpuSwapchain_t * swapchain );
static Microseconds_t GpuSwapchain_SwapBuffers( GpuContext_t * context, GpuSwapchain_t * swapchain );

================================================================================================================================
*/

typedef enum
{
	GPU_SURFACE_COLOR_FORMAT_R5G6B5,
	GPU_SURFACE_COLOR_FORMAT_B5G6R5,
	GPU_SURFACE_COLOR_FORMAT_R8G8B8A8,
	GPU_SURFACE_COLOR_FORMAT_B8G8R8A8,
	GPU_SURFACE_COLOR_FORMAT_MAX
} GpuSurfaceColorFormat_t;

typedef struct
{
	uint32_t		imageIndex;
	VkSemaphore		presentCompleteSemaphore;
	VkSemaphore		renderingCompleteSemaphore;
} GpuSwapchainBuffer_t;

typedef struct
{
	GpuSurfaceColorFormat_t	format;
	VkFormat				internalFormat;
	VkColorSpaceKHR			colorSpace;
	int						width;
	int						height;
	VkQueue					presentQueue;
	VkSwapchainKHR			swapchain;
	uint32_t				imageCount;
	VkImage *				images;
	VkImageView *			views;
	uint32_t				bufferCount;
	uint32_t				currentBuffer;
	GpuSwapchainBuffer_t *	buffers;
} GpuSwapchain_t;

static VkFormat GpuSwapchain_InternalSurfaceColorFormat( const GpuSurfaceColorFormat_t colorFormat )
{
	return	( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 ) ? VK_FORMAT_R8G8B8A8_UNORM :
			( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 ) ? VK_FORMAT_B8G8R8A8_UNORM :
			( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R5G6B5 ) ? VK_FORMAT_R5G6B5_UNORM_PACK16 :
			( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B5G6R5 ) ? VK_FORMAT_B5G6R5_UNORM_PACK16 :
			( ( VK_FORMAT_UNDEFINED ) ) ) ) ) );
}

static bool GpuSwapchain_Create( GpuContext_t * context, GpuSwapchain_t * swapchain, const VkSurfaceKHR surface,
								const GpuSurfaceColorFormat_t colorFormat, const int width, const int height, const int swapInterval )
{
	memset( swapchain, 0, sizeof( GpuSwapchain_t ) );

	GpuDevice_t * device = context->device;

	if ( !device->foundSwapchainExtension )
	{
		return false;
	}

	// Get the list of formats that are supported.
	uint32_t formatCount;
	VK( device->instance->vkGetPhysicalDeviceSurfaceFormatsKHR( device->physicalDevice, surface, &formatCount, NULL ) );

	VkSurfaceFormatKHR * surfaceFormats = (VkSurfaceFormatKHR *)malloc( formatCount * sizeof( VkSurfaceFormatKHR ) );
	VK( device->instance->vkGetPhysicalDeviceSurfaceFormatsKHR( device->physicalDevice, surface, &formatCount, surfaceFormats ) );

	const GpuSurfaceColorFormat_t desiredFormatTable[GPU_SURFACE_COLOR_FORMAT_MAX][GPU_SURFACE_COLOR_FORMAT_MAX] =
	{
		{ GPU_SURFACE_COLOR_FORMAT_R5G6B5, GPU_SURFACE_COLOR_FORMAT_B5G6R5, GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 },
		{ GPU_SURFACE_COLOR_FORMAT_B5G6R5, GPU_SURFACE_COLOR_FORMAT_R5G6B5, GPU_SURFACE_COLOR_FORMAT_B8G8R8A8, GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 },
		{ GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_COLOR_FORMAT_B8G8R8A8, GPU_SURFACE_COLOR_FORMAT_R5G6B5, GPU_SURFACE_COLOR_FORMAT_B5G6R5 },
		{ GPU_SURFACE_COLOR_FORMAT_B8G8R8A8, GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_COLOR_FORMAT_B5G6R5, GPU_SURFACE_COLOR_FORMAT_R5G6B5 }
	};
	assert( GPU_SURFACE_COLOR_FORMAT_R5G6B5 == 0 );
	assert( GPU_SURFACE_COLOR_FORMAT_B5G6R5 == 1 );
	assert( GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 == 2 );
	assert( GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 == 3 );
	assert( colorFormat >= 0 && colorFormat < GPU_SURFACE_COLOR_FORMAT_MAX );

	const GpuSurfaceColorFormat_t * desiredFormat = desiredFormatTable[colorFormat];
	const VkColorSpaceKHR desiredColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

	// If the format list includes just one entry of VK_FORMAT_UNDEFINED, then the surface has no preferred format.
	// Otherwise, at least one supported format will be returned.
	if ( formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED )
	{
		swapchain->format = colorFormat;
		swapchain->internalFormat = GpuSwapchain_InternalSurfaceColorFormat( desiredFormat[0] );
		swapchain->colorSpace = desiredColorSpace;
	}
	else
	{
		// Select the best matching surface format.
		assert( formatCount >= 1 );
		for ( uint32_t desired = 0; desired < GPU_SURFACE_COLOR_FORMAT_MAX; desired++ )
		{
			const VkFormat internalFormat = GpuSwapchain_InternalSurfaceColorFormat( desiredFormat[desired] );
			for ( uint32_t available = 0; available < formatCount; available++ )
			{
				if ( surfaceFormats[available].format == internalFormat &&
						surfaceFormats[available].colorSpace == desiredColorSpace )
				{
					swapchain->format = desiredFormat[desired];
					swapchain->internalFormat = internalFormat;
					swapchain->colorSpace = desiredColorSpace;
					break;
				}
			}
			if ( swapchain->internalFormat != VK_FORMAT_UNDEFINED )
			{
				break;
			}
		}
	}

	Print( "--------------------------------\n" );

	for ( uint32_t i = 0; i < formatCount; i++ )
	{
		const char * formatString = NULL;
		switch ( surfaceFormats[i].format )
		{
			case VK_FORMAT_R5G6B5_UNORM_PACK16:	formatString = "VK_FORMAT_R5G6B5_UNORM_PACK16"; break;
			case VK_FORMAT_B5G6R5_UNORM_PACK16:	formatString = "VK_FORMAT_B5G6R5_UNORM_PACK16"; break;
			case VK_FORMAT_R8G8B8A8_UNORM:		formatString = "VK_FORMAT_R8G8B8A8_UNORM"; break;
			case VK_FORMAT_R8G8B8A8_SRGB:		formatString = "VK_FORMAT_R8G8B8A8_SRGB"; break;
			case VK_FORMAT_B8G8R8A8_UNORM:		formatString = "VK_FORMAT_B8G8R8A8_UNORM"; break;
			case VK_FORMAT_B8G8R8A8_SRGB:		formatString = "VK_FORMAT_B8G8R8A8_SRGB"; break;
			default:
			{
				static char number[32];
				sprintf( number, "%d", surfaceFormats[i].format );
				formatString = number;
				break;
			}
		}
		Print( "%s %s%s\n", ( i == 0 ? "Surface Formats      :" : "                      " ),
				formatString, ( swapchain->internalFormat == surfaceFormats[i].format ) ? " (used)" : "" );
	}

	free( surfaceFormats );
	surfaceFormats = NULL;

	// Check the surface proprties and formats.
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VK( device->instance->vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device->physicalDevice, surface, &surfaceCapabilities ) );

	assert( ( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) != 0 );
	assert( ( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT ) != 0 );
	//assert( ( surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR ) != 0 );

	uint32_t presentModeCount;
	VK( device->instance->vkGetPhysicalDeviceSurfacePresentModesKHR( device->physicalDevice, surface, &presentModeCount, NULL ) );

	VkPresentModeKHR *presentModes = (VkPresentModeKHR *)malloc( presentModeCount * sizeof( VkPresentModeKHR ) );
	VK( device->instance->vkGetPhysicalDeviceSurfacePresentModesKHR( device->physicalDevice, surface, &presentModeCount, presentModes ) );

	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	VkPresentModeKHR desiredPresendMode =	( ( swapInterval == 0 ) ?	VK_PRESENT_MODE_IMMEDIATE_KHR :
											( ( swapInterval == -1 ) ?	VK_PRESENT_MODE_FIFO_RELAXED_KHR :
																		VK_PRESENT_MODE_FIFO_KHR ) );
	if ( swapchainPresentMode != desiredPresendMode  )
	{
		for ( uint32_t i = 0; i < presentModeCount; i++ )
		{
			if ( presentModes[i] == desiredPresendMode )
			{
				swapchainPresentMode = desiredPresendMode;
				break;
			}
		}
	}

	for ( uint32_t i = 0; i < presentModeCount; i++ )
	{
		const char * formatString = NULL;
		switch ( presentModes[i] )
		{
			case VK_PRESENT_MODE_IMMEDIATE_KHR:		formatString = "VK_PRESENT_MODE_IMMEDIATE_KHR"; break;
			case VK_PRESENT_MODE_MAILBOX_KHR:		formatString = "VK_PRESENT_MODE_MAILBOX_KHR"; break;
			case VK_PRESENT_MODE_FIFO_KHR:			formatString = "VK_PRESENT_MODE_FIFO_KHR"; break;
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR:	formatString = "VK_PRESENT_MODE_FIFO_RELAXED_KHR"; break;
			default:
			{
				static char number[32];
				sprintf( number, "%d", surfaceFormats[i].format );
				formatString = number;
				break;
			}
		}
		Print( "%s %s%s\n", ( i == 0 ? "Present Modes        :" : "                      " ),
				formatString, ( presentModes[i] == swapchainPresentMode ) ? " (used)" : "" );
	}

	free( presentModes );
	presentModes = NULL;

	VkExtent2D swapchainExtent =
	{
		CLAMP( (uint32_t)width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width ),
		CLAMP( (uint32_t)height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height ),
	};

#if !defined( OS_ANDROID )
	// The width and height are either both -1, or both not -1.
	if ( surfaceCapabilities.currentExtent.width != (uint32_t) -1 )
	{
		// If the surface size is defined, the swapchain size must match.
		swapchainExtent = surfaceCapabilities.currentExtent;
	}
#endif

	swapchain->width = swapchainExtent.width;
	swapchain->height = swapchainExtent.height;

	// Determine the number of VkImage's to use in the swapchain (we desire to
	// own only 1 image at a time, besides the images being displayed and
	// queued for display):
	uint32_t desiredNumberOfSwapChainImages = surfaceCapabilities.minImageCount + 1;
	if ( ( surfaceCapabilities.maxImageCount > 0 ) && ( desiredNumberOfSwapChainImages > surfaceCapabilities.maxImageCount ) )
	{
		// Application must settle for fewer images than desired:
		desiredNumberOfSwapChainImages = surfaceCapabilities.maxImageCount;
	}

	Print( "Swapchain Images     : %d\n", desiredNumberOfSwapChainImages );
	Print( "--------------------------------\n" );

	VkSurfaceTransformFlagBitsKHR preTransform;
	if ( surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
	{
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = surfaceCapabilities.currentTransform;
	}

	const bool separatePresentQueue = ( device->presentQueueFamilyIndex != device->workQueueFamilyIndex );
	const uint32_t queueFamilyIndices[2] = { device->workQueueFamilyIndex, device->presentQueueFamilyIndex };

	VkSwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = NULL;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = desiredNumberOfSwapChainImages;
	swapchainCreateInfo.imageFormat = swapchain->internalFormat;
	swapchainCreateInfo.imageColorSpace = swapchain->colorSpace;
	swapchainCreateInfo.imageExtent.width = swapchainExtent.width;
	swapchainCreateInfo.imageExtent.height = swapchainExtent.height;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage =	VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
										VK_IMAGE_USAGE_STORAGE_BIT;
	swapchainCreateInfo.imageSharingMode = separatePresentQueue ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = separatePresentQueue ? 2 : 0;
	swapchainCreateInfo.pQueueFamilyIndices = separatePresentQueue ? queueFamilyIndices : NULL;
	swapchainCreateInfo.preTransform = preTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = swapchainPresentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	VK( device->vkCreateSwapchainKHR( device->device, &swapchainCreateInfo, VK_ALLOCATOR, &swapchain->swapchain ) );

	VK( device->vkGetSwapchainImagesKHR( device->device, swapchain->swapchain,
											&swapchain->imageCount, NULL ) );

	swapchain->images = (VkImage *)malloc( swapchain->imageCount * sizeof( VkImage ) );
	VK( device->vkGetSwapchainImagesKHR( device->device, swapchain->swapchain,
											&swapchain->imageCount, swapchain->images ) );

	swapchain->views = (VkImageView *)malloc( swapchain->imageCount * sizeof( VkImageView ) );

	for ( uint32_t i = 0; i < swapchain->imageCount; i++ )
	{
		VkImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = NULL;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = swapchain->images[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = swapchain->internalFormat;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VK( device->vkCreateImageView( device->device, &imageViewCreateInfo, VK_ALLOCATOR, &swapchain->views[i] ) );
	}

	swapchain->bufferCount = swapchain->imageCount;
	swapchain->buffers = (GpuSwapchainBuffer_t *) malloc( swapchain->bufferCount * sizeof( GpuSwapchainBuffer_t ) );
	for ( uint32_t i = 0; i < swapchain->bufferCount; i++ )
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo;
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = NULL;
		semaphoreCreateInfo.flags = 0;

		swapchain->buffers[i].imageIndex = 0;
		VK( device->vkCreateSemaphore( device->device, &semaphoreCreateInfo, VK_ALLOCATOR, &swapchain->buffers[i].presentCompleteSemaphore ) );
		VK( device->vkCreateSemaphore( device->device, &semaphoreCreateInfo, VK_ALLOCATOR, &swapchain->buffers[i].renderingCompleteSemaphore ) );
	}

	swapchain->currentBuffer = 0;
	VK( device->vkAcquireNextImageKHR( device->device, swapchain->swapchain, UINT64_MAX,
			swapchain->buffers[swapchain->currentBuffer].presentCompleteSemaphore, VK_NULL_HANDLE,
			&swapchain->buffers[swapchain->currentBuffer].imageIndex ) );

	VC( device->vkGetDeviceQueue( device->device, device->presentQueueFamilyIndex, 0, &swapchain->presentQueue ) );

	assert( separatePresentQueue || swapchain->presentQueue == context->queue );

	return true;
}

static void GpuSwapchain_Destroy( GpuContext_t * context, GpuSwapchain_t * swapchain )
{
	GpuDevice_t * device = context->device;

	if ( !device->foundSwapchainExtension )
	{
		return;
	}

	for ( uint32_t i = 0; i < swapchain->imageCount; i++ )
	{
		VC( device->vkDestroyImageView( device->device, swapchain->views[i], VK_ALLOCATOR ) );
	}

	VC( device->vkDestroySwapchainKHR( device->device, swapchain->swapchain, VK_ALLOCATOR ) );

	for ( uint32_t i = 0; i < swapchain->bufferCount; i++ )
	{
		VC( device->vkDestroySemaphore( device->device, swapchain->buffers[i].renderingCompleteSemaphore, VK_ALLOCATOR ) );
		VC( device->vkDestroySemaphore( device->device, swapchain->buffers[i].presentCompleteSemaphore, VK_ALLOCATOR ) );
	}

	free( swapchain->buffers );
	free( swapchain->images );
	free( swapchain->views );

	memset( swapchain, 0, sizeof( GpuSwapchain_t ) );
}

static Microseconds_t GpuSwapchain_SwapBuffers( GpuContext_t * context, GpuSwapchain_t * swapchain )
{
	GpuDevice_t * device = context->device;

	if ( !device->foundSwapchainExtension )
	{
		return 0;
	}

	//
	// Present the current image from the swapchain once rendering has completed.
	//

	VkPresentInfoKHR presentInfo;
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &swapchain->buffers[swapchain->currentBuffer].renderingCompleteSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain->swapchain;
	presentInfo.pImageIndices = &swapchain->buffers[swapchain->currentBuffer].imageIndex;
	presentInfo.pResults = NULL;

	// There should be no need to handle VK_SUBOPTIMAL_WSI and VK_ERROR_OUT_OF_DATE_WSI because the window size is fixed.
	VK( device->vkQueuePresentKHR( swapchain->presentQueue, &presentInfo ) );

	const Microseconds_t swapTime = GetTimeMicroseconds();

	//
	// Fetch the next image from the swapchain.
	//

	swapchain->currentBuffer = ( swapchain->currentBuffer + 1 ) % swapchain->bufferCount;

	// There should be no need to handle VK_SUBOPTIMAL_WSI and VK_ERROR_OUT_OF_DATE_WSI because the window size is fixed.
	VK( device->vkAcquireNextImageKHR( device->device, swapchain->swapchain, UINT64_MAX,
			swapchain->buffers[swapchain->currentBuffer].presentCompleteSemaphore, VK_NULL_HANDLE,
			&swapchain->buffers[swapchain->currentBuffer].imageIndex ) );

	return swapTime;
}

/*
================================================================================================================================

GPU depth buffer.

This encapsulates a platform agnostic depth buffer.
For optimal performance, a depth buffer should only be created at load time, not at runtime.

GpuDepthBuffer_t

static void GpuDepthBuffer_Create( GpuContext_t * context, GpuDepthBuffer_t * depthBuffer,
									const GpuSurfaceDepthFormat_t depthFormat, const int width, const int height, const int numLayers );
static void GpuDepthBuffer_Destroy( GpuContext_t * context, GpuDepthBuffer_t * depthBuffer );

================================================================================================================================
*/

typedef enum
{
	GPU_SURFACE_DEPTH_FORMAT_NONE,
	GPU_SURFACE_DEPTH_FORMAT_D16,
	GPU_SURFACE_DEPTH_FORMAT_D24,
	GPU_SURFACE_DEPTH_FORMAT_MAX
} GpuSurfaceDepthFormat_t;

typedef struct
{
	GpuSurfaceDepthFormat_t	format;
	VkFormat				internalFormat;
	VkImageLayout			imageLayout;
	VkImage					image;
	VkDeviceMemory			memory;
	VkImageView *			views;
	int						numViews;
} GpuDepthBuffer_t;

static VkFormat GpuDepthBuffer_InternalSurfaceDepthFormat( const GpuSurfaceDepthFormat_t depthFormat )
{
	return	( ( depthFormat == GPU_SURFACE_DEPTH_FORMAT_D16 ) ? VK_FORMAT_D16_UNORM :
			( ( depthFormat == GPU_SURFACE_DEPTH_FORMAT_D24 ) ? VK_FORMAT_D24_UNORM_S8_UINT :
			VK_FORMAT_UNDEFINED ) );
}
static void GpuDepthBuffer_Create( GpuContext_t * context, GpuDepthBuffer_t * depthBuffer,
									const GpuSurfaceDepthFormat_t depthFormat, const int width, const int height, const int numLayers )
{
	assert( width >= 1 );
	assert( height >= 1 );
	assert( numLayers >= 1 );

	memset( depthBuffer, 0, sizeof( GpuDepthBuffer_t ) );

	depthBuffer->format = depthFormat;

	if ( depthFormat == GPU_SURFACE_DEPTH_FORMAT_NONE )
	{
		depthBuffer->internalFormat = VK_FORMAT_UNDEFINED;
		return;
	}

	depthBuffer->internalFormat = GpuDepthBuffer_InternalSurfaceDepthFormat( depthFormat );

	VkImageCreateInfo imageCreateInfo;
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = NULL;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = depthBuffer->internalFormat;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = numLayers;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = NULL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VK( context->device->vkCreateImage( context->device->device, &imageCreateInfo, VK_ALLOCATOR, &depthBuffer->image ) );

	VkMemoryRequirements memoryRequirements;
	VC( context->device->vkGetImageMemoryRequirements( context->device->device, depthBuffer->image, &memoryRequirements ) );

	VkMemoryAllocateInfo memoryAllocateInfo;
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = NULL;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = GpuDevice_GetMemoryTypeIndex( context->device, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	VK( context->device->vkAllocateMemory( context->device->device, &memoryAllocateInfo, VK_ALLOCATOR, &depthBuffer->memory ) );
	VK( context->device->vkBindImageMemory( context->device->device, depthBuffer->image, depthBuffer->memory, 0 ) );

	depthBuffer->views = (VkImageView *) malloc( numLayers * sizeof( VkImageView ) );
	depthBuffer->numViews = numLayers;

	for ( int layerIndex = 0; layerIndex < numLayers; layerIndex++ )
	{
		VkImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = NULL;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = depthBuffer->image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = depthBuffer->internalFormat;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_ZERO;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_ZERO;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_ZERO;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_ZERO;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = layerIndex;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VK( context->device->vkCreateImageView( context->device->device, &imageViewCreateInfo, VK_ALLOCATOR, &depthBuffer->views[layerIndex] ) );
	}

	//
	// Set optimal image layout
	//

	{
		GpuContext_CreateSetupCmdBuffer( context );

		VkImageMemoryBarrier imageMemoryBarrier;
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = NULL;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = depthBuffer->image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = numLayers;

		const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		const VkDependencyFlags flags = 0;

		VC( context->device->vkCmdPipelineBarrier( context->setupCommandBuffer, src_stages, dst_stages, flags, 0, NULL, 0, NULL, 1, &imageMemoryBarrier ) );

		GpuContext_FlushSetupCmdBuffer( context );
	}

	depthBuffer->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

static void GpuDepthBuffer_Destroy( GpuContext_t * context, GpuDepthBuffer_t * depthBuffer )
{
	if ( depthBuffer->internalFormat == VK_FORMAT_UNDEFINED )
	{
		return;
	}

	for ( int viewIndex = 0; viewIndex < depthBuffer->numViews; viewIndex++ )
	{
		VC( context->device->vkDestroyImageView( context->device->device, depthBuffer->views[viewIndex], VK_ALLOCATOR ) );
	}
	VC( context->device->vkDestroyImage( context->device->device, depthBuffer->image, VK_ALLOCATOR ) );
	VC( context->device->vkFreeMemory( context->device->device, depthBuffer->memory, VK_ALLOCATOR ) );

	free( depthBuffer->views );
}

/*
================================================================================================================================

GPU Window.

Window with associated GPU context for GPU accelerated rendering.
For optimal performance, a window should only be created at load time, not at runtime.

GpuWindow_t
GpuWindowEvent_t
KeyboardKey_t
MouseButton_t

static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const int width, const int height, const bool fullscreen );
static void GpuWindow_Destroy( GpuWindow_t * window );
static void GpuWindow_Exit( GpuWindow_t * window );
static bool GpuWindow_ProcessEvents( GpuWindow_t * window );
static void GpuWindow_SwapInterval( GpuWindow_t * window, const int swapInterval );
static void GpuWindow_SwapBuffers( GpuWindow_t * window );
static Microseconds_t GpuWindow_GetNextSwapTime( GpuWindow_t * window );
static bool GpuWindow_CheckKeyboardKey( GpuWindow_t * window, const KeyboardKey_t key );
static bool GpuWindow_CheckMouseButton( GpuWindow_t * window, const MouseButton_t button );

================================================================================================================================
*/

typedef enum
{
	GPU_WINDOW_EVENT_NONE,
	GPU_WINDOW_EVENT_ACTIVATED,
	GPU_WINDOW_EVENT_DEACTIVATED,
	GPU_WINDOW_EVENT_EXIT
} GpuWindowEvent_t;

typedef struct
{
	GpuDevice_t				device;
	GpuContext_t			context;
	GpuSurfaceColorFormat_t	colorFormat;
	GpuSurfaceDepthFormat_t	depthFormat;
	int						windowWidth;
	int						windowHeight;
	int						windowSwapInterval;
	float					windowRefreshRate;
	bool					windowFullscreen;
	bool					windowActive;
	bool					windowExit;
	bool					keyInput[256];
	bool					mouseInput[8];
	int						mouseInputX[8];
	int						mouseInputY[8];
	Microseconds_t			lastSwapTime;

	// The swapchain and depth buffer could be stored on the context like OpenGL but this makes more sense.
	VkSurfaceKHR			surface;
	int						swapchainCreateCount;
	GpuSwapchain_t			swapchain;
	GpuDepthBuffer_t		depthBuffer;

#if defined( OS_WINDOWS )
	HINSTANCE				hInstance;
	HWND					hWnd;
	bool					windowActiveState;
#elif defined( OS_MAC )
	CGDirectDisplayID		display;
	CGDisplayModeRef		desktopDisplayMode;
	NSWindow *				nsWindow;
	NSView *				nsView;
#elif defined( OS_LINUX_XLIB )
	Display *				xDisplay;
	int						xScreen;
	Window					xRoot;
	Window					xWindow;
	int						desktopWidth;
	int						desktopHeight;
	float					desktopRefreshRate;
#elif defined( OS_LINUX_XCB )
	xcb_connection_t *		connection;
	xcb_screen_t *			screen;
	xcb_window_t			window;
	xcb_atom_t				wm_delete_window_atom;
	xcb_key_symbols_t *		key_symbols;
	int						desktopWidth;
	int						desktopHeight;
	float					desktopRefreshRate;
#elif defined( OS_ANDROID )
	struct android_app *	app;
	Java_t					java;
	ANativeWindow *			nativeWindow;
	bool					resumed;
#endif
} GpuWindow_t;

static void GpuWindow_CreateFromSurface( GpuWindow_t * window, const VkSurfaceKHR surface )
{
	GpuSwapchain_Create( &window->context, &window->swapchain, surface, window->colorFormat, window->windowWidth, window->windowHeight, window->windowSwapInterval );
	GpuDepthBuffer_Create( &window->context, &window->depthBuffer, window->depthFormat, window->windowWidth, window->windowHeight, 1 );

	assert( window->swapchain.width == window->windowWidth && window->swapchain.height == window->windowHeight );

	window->surface = surface;
	window->colorFormat = window->swapchain.format;		// May not have acquired the desired format.
	window->depthFormat = window->depthBuffer.format;
	window->swapchainCreateCount++;
}

static void GpuWindow_DestroySurface( GpuWindow_t * window )
{
	GpuDepthBuffer_Destroy( &window->context, &window->depthBuffer );
	GpuSwapchain_Destroy( &window->context, &window->swapchain );
	VC( window->device.instance->vkDestroySurfaceKHR( window->device.instance->instance, window->surface, VK_ALLOCATOR ) );
}

#if defined( OS_WINDOWS )

typedef enum
{
	KEY_ESCAPE		= 0x1B,
	KEY_A			= 0x61,
	KEY_B			= 0x62,
	KEY_C			= 0x63,
	KEY_D			= 0x64,
	KEY_E			= 0x65,
	KEY_F			= 0x66,
	KEY_G			= 0x67,
	KEY_H			= 0x68,
	KEY_I			= 0x69,
	KEY_J			= 0x6A,
	KEY_K			= 0x6B,
	KEY_L			= 0x6C,
	KEY_M			= 0x6D,
	KEY_N			= 0x6E,
	KEY_O			= 0x6F,
	KEY_P			= 0x70,
	KEY_Q			= 0x71,
	KEY_R			= 0x72,
	KEY_S			= 0x73,
	KEY_T			= 0x74,
	KEY_U			= 0x75,
	KEY_V			= 0x76,
	KEY_W			= 0x77,
	KEY_X			= 0x78,
	KEY_Y			= 0x79,
	KEY_Z			= 0x7A
} KeyboardKey_t;

typedef enum
{
	MOUSE_LEFT		= 0,
	MOUSE_RIGHT		= 1
} MouseButton_t;

LRESULT APIENTRY WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	GpuWindow_t * window = (GpuWindow_t *) GetWindowLongPtr( hWnd, GWLP_USERDATA );

	switch ( message )
	{
		case WM_SIZE:
		{
			if ( window != NULL )
			{
				window->windowWidth = (int) LOWORD( lParam );
				window->windowHeight = (int) HIWORD( lParam );
			}
			return 0;
		}
		case WM_ACTIVATE:
		{
			if ( window != NULL )
			{
				window->windowActiveState = !HIWORD( wParam );
			}
			return 0;
		}
		case WM_ERASEBKGND:
		{
			return 0;
		}
		case WM_CLOSE:
		{
			PostQuitMessage( 0 );
			return 0;
		}
		case WM_CHAR:
		{
			if ( window != NULL )
			{
				if ( (int)wParam >= 0 && (int)wParam < 256 )
				{
					window->keyInput[(int)wParam] = true;
				}
			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			window->mouseInput[MOUSE_LEFT] = true;
			window->mouseInputX[MOUSE_LEFT] = LOWORD( lParam );
			window->mouseInputY[MOUSE_LEFT] = window->windowHeight - HIWORD( lParam );
			break;
		}
		case WM_RBUTTONDOWN:
		{
			window->mouseInput[MOUSE_RIGHT] = true;
			window->mouseInputX[MOUSE_RIGHT] = LOWORD( lParam );
			window->mouseInputY[MOUSE_RIGHT] = window->windowHeight - HIWORD( lParam );
			break;
		}
	}
	return DefWindowProc( hWnd, message, wParam, lParam );
}

static void GpuWindow_Destroy( GpuWindow_t * window )
{
	GpuWindow_DestroySurface( window );
	GpuContext_Destroy( &window->context );
	GpuDevice_Destroy( &window->device );

	if ( window->windowFullscreen )
	{
		ChangeDisplaySettings( NULL, 0 );
		ShowCursor( TRUE );
	}

	if ( window->hWnd )
	{
		if ( !DestroyWindow( window->hWnd ) )
		{
			Error( "Failed to destroy the window." );
		}
		window->hWnd = NULL;
	}

	if ( window->hInstance )
	{
		if ( !UnregisterClass( APPLICATION_NAME, window->hInstance ) )
		{
			Error( "Failed to unregister window class." );
		}
		window->hInstance = NULL;
	}
}

static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( GpuWindow_t ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->windowActiveState = false;
	window->lastSwapTime = GetTimeMicroseconds();

	if ( window->windowFullscreen )
	{
		DEVMODE dmScreenSettings;
		memset( &dmScreenSettings, 0, sizeof( dmScreenSettings ) );
		dmScreenSettings.dmSize			= sizeof( dmScreenSettings );
		dmScreenSettings.dmPelsWidth	= width;
		dmScreenSettings.dmPelsHeight	= height;
		dmScreenSettings.dmBitsPerPel	= 32;
		dmScreenSettings.dmFields		= DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

		if ( ChangeDisplaySettings( &dmScreenSettings, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL )
		{
			Error( "The requested fullscreen mode is not supported." );
			return false;
		}
	}

	DEVMODE lpDevMode;
	memset( &lpDevMode, 0, sizeof( DEVMODE ) );
	lpDevMode.dmSize = sizeof( DEVMODE );
	lpDevMode.dmDriverExtra = 0;

	if ( EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &lpDevMode ) != FALSE )
	{
		window->windowRefreshRate = (float)lpDevMode.dmDisplayFrequency;
	}

	window->hInstance = GetModuleHandle( NULL );

	WNDCLASS wc;
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc		= (WNDPROC) WndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= window->hInstance;
	wc.hIcon			= LoadIcon( NULL, IDI_WINLOGO );
	wc.hCursor			= LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground	= NULL;
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= APPLICATION_NAME;

	if ( !RegisterClass( &wc ) )
	{
		Error( "Failed to register window class." );
		return false;
	}
	
	DWORD dwExStyle = 0;
	DWORD dwStyle = 0;
	if ( window->windowFullscreen )
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP;
		ShowCursor( FALSE );
	}
	else
	{
		// Fixed size window.
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	}

	RECT windowRect;
	windowRect.left = (long)0;
	windowRect.right = (long)width;
	windowRect.top = (long)0;
	windowRect.bottom = (long)height;

	AdjustWindowRectEx( &windowRect, dwStyle, FALSE, dwExStyle );

	if ( !window->windowFullscreen )
	{
		RECT desktopRect;
		GetWindowRect( GetDesktopWindow(), &desktopRect );

		const int offsetX = ( desktopRect.right - ( windowRect.right - windowRect.left ) ) / 2;
		const int offsetY = ( desktopRect.bottom - ( windowRect.bottom - windowRect.top ) ) / 2;

		windowRect.left += offsetX;
		windowRect.right += offsetX;
		windowRect.top += offsetY;
		windowRect.bottom += offsetY;
	}

	window->hWnd = CreateWindowEx( dwExStyle,						// Extended style for the window
								APPLICATION_NAME,					// Class name
								WINDOW_TITLE,						// Window title
								dwStyle |							// Defined window style
								WS_CLIPSIBLINGS |					// Required window style
								WS_CLIPCHILDREN,					// Required window style
								windowRect.left,					// Window X position
								windowRect.top,						// Window Y position
								windowRect.right - windowRect.left,	// Window width
								windowRect.bottom - windowRect.top,	// Window height
								NULL,								// No parent window
								NULL,								// No menu
								window->hInstance,					// Instance
								NULL );
	if ( !window->hWnd )
	{
		GpuWindow_Destroy( window );
		Error( "Failed to create window." );
		return false;
	}

	SetWindowLongPtr( window->hWnd, GWLP_USERDATA, (LONG_PTR) window );

	VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo;
	win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	win32SurfaceCreateInfo.pNext = NULL;
	win32SurfaceCreateInfo.flags = 0;
	win32SurfaceCreateInfo.hinstance = window->hInstance;
	win32SurfaceCreateInfo.hwnd = window->hWnd;

	VkSurfaceKHR surface;
	VK( instance->vkCreateSurfaceKHR( instance->instance, &win32SurfaceCreateInfo, VK_ALLOCATOR, &surface ) );

	GpuDevice_Create( &window->device, instance, queueInfo, surface );
	GpuContext_Create( &window->context, &window->device, queueIndex );
	GpuWindow_CreateFromSurface( window, surface );

	ShowWindow( window->hWnd, SW_SHOW );
	SetForegroundWindow( window->hWnd );
	SetFocus( window->hWnd );

	return true;
}

static void GpuWindow_Exit( GpuWindow_t * window )
{
	window->windowExit = true;
}

static GpuWindowEvent_t GpuWindow_ProcessEvents( GpuWindow_t * window )
{
	UNUSED_PARM( window );

	MSG msg;
	while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) > 0 )
	{
		if ( msg.message == WM_QUIT )
		{
			window->windowExit = true;
		}
		else
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}

	if ( window->windowExit )
	{
		return GPU_WINDOW_EVENT_EXIT;
	}
	if ( window->windowActiveState != window->windowActive )
	{
		window->windowActive = window->windowActiveState;
		return ( window->windowActiveState ) ? GPU_WINDOW_EVENT_ACTIVATED : GPU_WINDOW_EVENT_DEACTIVATED;
	}
	return GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_MAC )

typedef enum
{
	KEY_ESCAPE			= 0x35,
	KEY_A				= 0x00,
	KEY_B				= 0x0B,
	KEY_C				= 0x08,
	KEY_D				= 0x02,
	KEY_E				= 0x0E,
	KEY_F				= 0x03,
	KEY_G				= 0x05,
	KEY_H				= 0x04,
	KEY_I				= 0x22,
	KEY_J				= 0x26,
	KEY_K				= 0x28,
	KEY_L				= 0x25,
	KEY_M				= 0x2E,
	KEY_N				= 0x2D,
	KEY_O				= 0x1F,
	KEY_P				= 0x23,
	KEY_Q				= 0x0C,
	KEY_R				= 0x0F,
	KEY_S				= 0x01,
	KEY_T				= 0x11,
	KEY_U				= 0x20,
	KEY_V				= 0x09,
	KEY_W				= 0x0D,
	KEY_X				= 0x07,
	KEY_Y				= 0x10,
	KEY_Z				= 0x06,
} KeyboardKey_t;

typedef enum
{
	MOUSE_LEFT			= 0,
	MOUSE_RIGHT			= 1
} MouseButton_t;

NSAutoreleasePool * autoReleasePool;

@interface MyNSWindow : NSWindow
- (BOOL)canBecomeMainWindow;
- (BOOL)canBecomeKeyWindow;
- (BOOL)acceptsFirstResponder;
- (void)keyDown:(NSEvent *)event;
@end

@implementation MyNSWindow
- (BOOL)canBecomeMainWindow { return YES; }
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (void)keyDown:(NSEvent *)event {}
@end

@interface MyNSView : NSView
- (BOOL)acceptsFirstResponder;
- (void)keyDown:(NSEvent *)event;
@end

@implementation MyNSView
- (BOOL)acceptsFirstResponder { return YES; }
- (void)keyDown:(NSEvent *)event {}

#if defined( MOLTEN_VK )

-(instancetype) initWithFrame:(NSRect)frameRect {
	self = [super initWithFrame: frameRect];
	if ( self ) {
		self.wantsLayer = YES;		// Back the view with a layer created by the makeBackingLayer method.
	}
	return self;
}

// Indicates that the view wants to draw using the backing layer instead of using drawRect.
-(BOOL) wantsUpdateLayer { return YES; }

// If the wantsLayer property is set to YES, this method will be invoked to return a layer instance.
-(CALayer *) makeBackingLayer {
	CALayer * layer = [[CAMetalLayer class] layer];
	CGSize viewScale = [self convertSizeToBacking: CGSizeMake( 1.0f, 1.0f )];
	layer.contentsScale = MIN( viewScale.width, viewScale.height );
	return layer;
}

#endif

@end

static void GpuWindow_Destroy( GpuWindow_t * window )
{
	GpuWindow_DestroySurface( window );
	GpuContext_Destroy( &window->context );
	GpuDevice_Destroy( &window->device );

	if ( window->windowFullscreen )
	{
		CGDisplaySetDisplayMode( window->display, window->desktopDisplayMode, NULL );
		CGDisplayModeRelease( window->desktopDisplayMode );
		window->desktopDisplayMode = NULL;
	}
	if ( window->nsWindow )
	{
		[window->nsWindow release];
		window->nsWindow = nil;
	}
	if ( window->nsView )
	{
		[window->nsView release];
		window->nsView = nil;
	}
}

static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( GpuWindow_t ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeMicroseconds();

	// Get a list of all available displays.
	CGDirectDisplayID displays[32];
	CGDisplayCount displayCount = 0;
	CGDisplayErr err = CGGetActiveDisplayList( 32, displays, &displayCount );
	if ( err != CGDisplayNoErr )
	{
		return false;
	}
	// Use the main display.
	window->display = displays[0];
	window->desktopDisplayMode = CGDisplayCopyDisplayMode( window->display );

	// If fullscreen then switch to the best matching display mode.
	if ( window->windowFullscreen )
	{
		CFArrayRef displayModes = CGDisplayCopyAllDisplayModes( window->display, NULL );
		CFIndex displayModeCount = CFArrayGetCount( displayModes );
		CGDisplayModeRef bestDisplayMode = nil;
		size_t bestDisplayWidth = 0;
		size_t bestDisplayHeight = 0;
		float bestDisplayRefreshRate = 0;
		size_t bestError = 0x7FFFFFFF;
		for ( CFIndex i = 0; i < displayModeCount; i++ )
		{
			CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex( displayModes, i );

			const size_t modeWidth = CGDisplayModeGetWidth( mode );
			const size_t modeHeight = CGDisplayModeGetHeight( mode );
			const double modeRefreshRate = CGDisplayModeGetRefreshRate( mode );
			CFStringRef modePixelEncoding = CGDisplayModeCopyPixelEncoding( mode );
			const bool modeBitsPerPixelIs32 = ( CFStringCompare( modePixelEncoding, CFSTR( IO32BitDirectPixels ), 0) == kCFCompareEqualTo );
			CFRelease( modePixelEncoding );

			if ( modeBitsPerPixelIs32 )
			{
				const size_t dw = modeWidth - width;
				const size_t dh = modeHeight - height;
				const size_t error = dw * dw + dh * dh;
				if ( error < bestError )
				{
					bestError = error;
					bestDisplayMode = mode;
					bestDisplayWidth = modeWidth;
					bestDisplayHeight = modeHeight;
					bestDisplayRefreshRate = (float)modeRefreshRate;
				}
			}
		}
		CGDisplayErr err = CGDisplaySetDisplayMode( window->display, bestDisplayMode, NULL );
		if ( err != CGDisplayNoErr )
		{
			CFRelease( displayModes );
			return false;
		}
		CFRelease( displayModes );
		window->windowWidth = (int)bestDisplayWidth;
		window->windowHeight = (int)bestDisplayHeight;
		window->windowRefreshRate = ( bestDisplayRefreshRate > 0.0f ) ? bestDisplayRefreshRate : 60.0f;
	}
	else
	{
		const float desktopDisplayRefreshRate = (float)CGDisplayModeGetRefreshRate( window->desktopDisplayMode );
		window->windowRefreshRate = ( desktopDisplayRefreshRate > 0.0f ) ? desktopDisplayRefreshRate : 60.0f;
	}

	if ( window->windowFullscreen )
	{
		NSScreen * screen = [NSScreen mainScreen];
		NSRect screenRect = [screen frame];
		
		window->nsView = [MyNSView alloc];
		[window->nsView initWithFrame:screenRect];

		const int style = NSBorderlessWindowMask;

		window->nsWindow = [MyNSWindow alloc];
		[window->nsWindow initWithContentRect:screenRect styleMask:style backing:NSBackingStoreBuffered defer:NO screen:screen];
		[window->nsWindow setOpaque:YES];
		[window->nsWindow setLevel:NSMainMenuWindowLevel+1];
		[window->nsWindow setContentView:window->nsView];
		[window->nsWindow makeMainWindow];
		[window->nsWindow makeKeyAndOrderFront:nil];
		[window->nsWindow makeFirstResponder:nil];
	}
	else
	{
		NSScreen * screen = [NSScreen mainScreen];
		NSRect screenRect = [screen frame];

		NSRect windowRect;
		windowRect.origin.x = ( screenRect.size.width - width ) / 2;
		windowRect.origin.y = ( screenRect.size.height - height ) / 2;
		windowRect.size.width = width;
		windowRect.size.height = height;

		window->nsView = [MyNSView alloc];
		[window->nsView initWithFrame:windowRect];

		// Fixed size window.
		const int style = NSTitledWindowMask;// | NSClosableWindowMask | NSResizableWindowMask;

		window->nsWindow = [MyNSWindow alloc];
		[window->nsWindow initWithContentRect:windowRect styleMask:style backing:NSBackingStoreBuffered defer:NO screen:screen];
		[window->nsWindow setTitle:@WINDOW_TITLE];
		[window->nsWindow setOpaque:YES];
		[window->nsWindow setContentView:window->nsView];
		[window->nsWindow makeMainWindow];
		[window->nsWindow makeKeyAndOrderFront:nil];
		[window->nsWindow makeFirstResponder:nil];
	}

	VkOsxSurfaceCreateInfoKHR osxSurfaceCreateInfo;
	osxSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_OSX_SURFACE_CREATE_INFO_KHR;
	osxSurfaceCreateInfo.pNext = NULL;
	osxSurfaceCreateInfo.flags = 0;
	osxSurfaceCreateInfo.nsview = window->nsView;

	VkSurfaceKHR surface;
	VK( instance->vkCreateSurfaceKHR( instance->instance, &osxSurfaceCreateInfo, VK_ALLOCATOR, &surface ) );

	GpuDevice_Create( &window->device, instance, queueInfo, surface );
	GpuContext_Create( &window->context, &window->device, queueIndex );
	GpuWindow_CreateFromSurface( window, surface );

	return true;
}

static void GpuWindow_Exit( GpuWindow_t * window )
{
	window->windowExit = true;
}

static GpuWindowEvent_t GpuWindow_ProcessEvents( GpuWindow_t * window )
{
	[autoReleasePool release];
	autoReleasePool = [[NSAutoreleasePool alloc] init];

	for ( ; ; )
	{
		NSEvent * event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
		if ( event == nil )
		{
			break;
		}

		if ( event.type == NSKeyDown )
		{
			unsigned short key = [event keyCode];
			if ( key >= 0 && key < 256 )
			{
				window->keyInput[key] = true;
			}
		}
		else if ( event.type == NSLeftMouseDown )
		{
			NSPoint point = [event locationInWindow];
			window->mouseInput[MOUSE_LEFT] = true;
			window->mouseInputX[MOUSE_LEFT] = point.x;
			window->mouseInputY[MOUSE_LEFT] = point.y - 1;	// change to zero-based
		}
		else if ( event.type == NSRightMouseDown )
		{
			NSPoint point = [event locationInWindow];
			window->mouseInput[MOUSE_RIGHT] = true;
			window->mouseInputX[MOUSE_RIGHT] = point.x;
			window->mouseInputY[MOUSE_RIGHT] = point.y - 1;	// change to zero-based
		}

		[NSApp sendEvent:event];
	}

	if ( window->windowExit )
	{
		return GPU_WINDOW_EVENT_EXIT;
	}

	if ( window->windowActive == false )
	{
		window->windowActive = true;
		return GPU_WINDOW_EVENT_ACTIVATED;
	}

	return GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_LINUX_XLIB )

typedef enum	// keysym.h
{
	KEY_ESCAPE		= ( XK_Escape & 255 ),
	KEY_A			= XK_a,
	KEY_B			= XK_b,
	KEY_C			= XK_c,
	KEY_D			= XK_d,
	KEY_E			= XK_e,
	KEY_F			= XK_f,
	KEY_G			= XK_g,
	KEY_H			= XK_h,
	KEY_I			= XK_i,
	KEY_J			= XK_j,
	KEY_K			= XK_k,
	KEY_L			= XK_l,
	KEY_M			= XK_m,
	KEY_N			= XK_n,
	KEY_O			= XK_o,
	KEY_P			= XK_p,
	KEY_Q			= XK_q,
	KEY_R			= XK_r,
	KEY_S			= XK_s,
	KEY_T			= XK_t,
	KEY_U			= XK_u,
	KEY_V			= XK_v,
	KEY_W			= XK_w,
	KEY_X			= XK_x,
	KEY_Y			= XK_y,
	KEY_Z			= XK_z
} KeyboardKey_t;

typedef enum
{
	MOUSE_LEFT		= Button1,
	MOUSE_RIGHT		= Button2
} MouseButton_t;

/*
	Change video mode using the XFree86-VidMode X extension.

	While the XFree86-VidMode X extension should be superseded by the XRandR X extension,
	this still appears to be the most reliable way to change video modes for a single
	monitor configuration.
*/
static bool ChangeVideoMode_XF86VidMode( Display * xDisplay, int xScreen, Window xWindow,
								int * currentWidth, int * currentHeight, float * currentRefreshRate,
								int * desiredWidth, int * desiredHeight, float * desiredRefreshRate )
{
	int videoModeCount;
	XF86VidModeModeInfo ** videoModeInfos;

	XF86VidModeGetAllModeLines( xDisplay, xScreen, &videoModeCount, &videoModeInfos );

	if ( currentWidth != NULL && currentHeight != NULL && currentRefreshRate != NULL )
	{
		XF86VidModeModeInfo * mode = videoModeInfos[0];
		*currentWidth = mode->hdisplay;
		*currentHeight = mode->vdisplay;
		*currentRefreshRate = ( mode->dotclock * 1000.0f ) / ( mode->htotal * mode->vtotal );
	}

	if ( desiredWidth != NULL && desiredHeight != NULL && desiredRefreshRate != NULL )
	{
		XF86VidModeModeInfo * bestMode = NULL;
		int bestModeWidth = 0;
		int bestModeHeight = 0;
		float bestModeRefreshRate = 0.0f;
		int bestSizeError = 0x7FFFFFFF;
		float bestRefreshRateError = 1e6f;
		for ( int j = 0; j < videoModeCount; j++ )
		{
			XF86VidModeModeInfo * mode = videoModeInfos[j];
			const int modeWidth = mode->hdisplay;
			const int modeHeight = mode->vdisplay;
			const float modeRefreshRate = ( mode->dotclock * 1000.0f ) / ( mode->htotal * mode->vtotal );

			const int dw = modeWidth - *desiredWidth;
			const int dh = modeHeight - *desiredHeight;
			const int sizeError = dw * dw + dh * dh;
			const float refreshRateError = fabs( modeRefreshRate - *desiredRefreshRate );
			if ( sizeError < bestSizeError || ( sizeError == bestSizeError && refreshRateError < bestRefreshRateError ) )
			{
				bestSizeError = sizeError;
				bestRefreshRateError = refreshRateError;
				bestMode = mode;
				bestModeWidth = modeWidth;
				bestModeHeight = modeHeight;
				bestModeRefreshRate = modeRefreshRate;
			}
		}

		XF86VidModeSwitchToMode( xDisplay, xScreen, bestMode );
		XF86VidModeSetViewPort( xDisplay, xScreen, 0, 0 );

		*desiredWidth = bestModeWidth;
		*desiredHeight = bestModeHeight;
		*desiredRefreshRate = bestModeRefreshRate;
	}

	for ( int i = 0; i < videoModeCount; i++ )
	{
		if ( videoModeInfos[i]->privsize > 0 )
		{
			XFree( videoModeInfos[i]->private );
		}
	}
	XFree( videoModeInfos );

	return true;
}

/*
	Change video mode using the XRandR X extension version 1.1

	This does not work using NVIDIA drivers because the NVIDIA drivers by default dynamically
	configure TwinView, known as DynamicTwinView. When DynamicTwinView is enabled (the default),
	the refresh rate of a mode reported through XRandR is not the actual refresh rate, but
	instead is an unique number such that each MetaMode has a different value. This is to
	guarantee that MetaModes can be uniquely identified by XRandR.

	To get XRandR to report accurate refresh rates, DynamicTwinView needs to be disabled, but
	then NV-CONTROL clients, such as nvidia-settings, will not be able to dynamically manipulate
	the X screen's MetaModes.
*/
static bool ChangeVideoMode_XRandR_1_1( Display * xDisplay, int xScreen, Window xWindow,
								int * currentWidth, int * currentHeight, float * currentRefreshRate,
								int * desiredWidth, int * desiredHeight, float * desiredRefreshRate )
{
	int major_version;
	int minor_version;
	XRRQueryVersion( xDisplay, &major_version, &minor_version );

	XRRScreenConfiguration * screenInfo = XRRGetScreenInfo( xDisplay, xWindow );
	if ( screenInfo == NULL )
	{
		Error( "Cannot get screen info." );
		return false;
	}

	if ( currentWidth != NULL && currentHeight != NULL && currentRefreshRate != NULL )
	{
		XRRScreenConfiguration * screenInfo = XRRGetScreenInfo( xDisplay, xWindow );

		Rotation rotation;
		int size_index = XRRConfigCurrentConfiguration( screenInfo, &rotation );

		int nsizes;
		XRRScreenSize * sizes = XRRConfigSizes( screenInfo, &nsizes );

		*currentWidth = sizes[size_index].width;
		*currentHeight = sizes[size_index].height;
		*currentRefreshRate = XRRConfigCurrentRate( screenInfo );
	}

	if ( desiredWidth != NULL && desiredHeight != NULL && desiredRefreshRate != NULL )
	{
		int nsizes = 0;
		XRRScreenSize * sizes = XRRConfigSizes( screenInfo, &nsizes );

		int size_index = -1;
		int bestSizeError = 0x7FFFFFFF;
		for ( int i = 0; i < nsizes; i++ )
		{
			const int dw = sizes[i].width - *desiredWidth;
			const int dh = sizes[i].height - *desiredHeight;
			const int error = dw * dw + dh * dh;
			if ( error < bestSizeError )
			{
				bestSizeError = error;
				size_index = i;
			}
		}
		if ( size_index == -1 )
		{
			Error( "%dx%d resolution not available.", *desiredWidth, *desiredHeight );
			XRRFreeScreenConfigInfo( screenInfo );
			return false;
		}

		int nrates = 0;
		short * rates = XRRConfigRates( screenInfo, size_index, &nrates );

		int rate_index = -1;
		float bestRateError = 1e6f;
		for ( int i = 0; i < nrates; i++ )
		{
			const float error = fabs( rates[i] - *desiredRefreshRate );
			if ( error < bestRateError )
			{
				bestRateError = error;
				rate_index = i;
			}
		}

		*desiredWidth = sizes[size_index].width;
		*desiredHeight = sizes[size_index].height;
		*desiredRefreshRate = rates[rate_index];

		XSelectInput( xDisplay, xWindow, StructureNotifyMask );
		XRRSelectInput( xDisplay, xWindow, RRScreenChangeNotifyMask );

		Rotation rotation = 1;
		int reflection = 0;

		Status status = XRRSetScreenConfigAndRate( xDisplay, screenInfo, xWindow,
							(SizeID) size_index,
							(Rotation) (rotation | reflection),
							rates[rate_index],
							CurrentTime );

		if ( status != RRSetConfigSuccess)
		{
			Error( "Failed to change resolution to %dx%d", *desiredWidth, *desiredHeight );
			XRRFreeScreenConfigInfo( screenInfo );
			return false;
		}

		int eventbase;
		int errorbase;
		XRRQueryExtension( xDisplay, &eventbase, &errorbase );

		bool receivedScreenChangeNotify = false;
		bool receivedConfigNotify = false;
		while ( 1 )
		{
			XEvent event;
			XNextEvent( xDisplay, (XEvent *) &event );
			XRRUpdateConfiguration( &event );
			if ( event.type - eventbase == RRScreenChangeNotify )
			{
				receivedScreenChangeNotify = true;
			}
			else if ( event.type == ConfigureNotify )
			{
				receivedConfigNotify = true ;
			}
			if ( receivedScreenChangeNotify && receivedConfigNotify )
			{
				break;
			}
		}
	}

	XRRFreeScreenConfigInfo( screenInfo );

	return true;
}

/*
	Change video mode using the XRandR X extension version 1.2

	The following code does not necessarily work out of the box, because on
	some configurations the modes list returned by XRRGetScreenResources()
	is populated with nothing other than the maximum display resolution,
	even though XF86VidModeGetAllModeLines() and XRRConfigSizes() *will*
	list all resolutions for the same display.

	The user can manually add new modes from the command-line using the
	xrandr utility:

	xrandr --newmode <modeline>

	Where <modeline> is generated with a utility that implements either
	the General Timing Formula (GTF) or the Coordinated Video Timing (CVT)
	standard put forth by the Video Electronics Standards Association (VESA):

	gft <width> <height> <Hz>	// http://gtf.sourceforge.net/
	cvt <width> <height> <Hz>	// http://www.uruk.org/~erich/projects/cvt/

	Alternatively, new modes can be added in code using XRRCreateMode().
	However, this requires calculating all the timing information in code
	because there is no standard library that implements the GTF or CVT.
*/
static bool ChangeVideoMode_XRandR_1_2( Display * xDisplay, int xScreen, Window xWindow,
								int * currentWidth, int * currentHeight, float * currentRefreshRate,
								int * desiredWidth, int * desiredHeight, float * desiredRefreshRate )
{
	int major_version;
	int minor_version;
	XRRQueryVersion( xDisplay, &major_version, &minor_version );

	/*
		Screen	- virtual screenspace which may be covered by multiple CRTCs
		CRTC	- display controller
		Output	- display/monitor connected to a CRTC
		Clones	- outputs that are simultaneously connected to the same CRTC
	*/

	const int PRIMARY_CRTC_INDEX = 0;
	const int PRIMARY_OUTPUT_INDEX = 0;

	XRRScreenResources * screenResources = XRRGetScreenResources( xDisplay, xWindow );
	XRRCrtcInfo * primaryCrtcInfo = XRRGetCrtcInfo( xDisplay, screenResources, screenResources->crtcs[PRIMARY_CRTC_INDEX] );
	XRROutputInfo * primaryOutputInfo = XRRGetOutputInfo( xDisplay, screenResources, primaryCrtcInfo->outputs[PRIMARY_OUTPUT_INDEX] );

	if ( currentWidth != NULL && currentHeight != NULL && currentRefreshRate != NULL )
	{
		for ( int i = 0; i < screenResources->nmode; i++ )
		{
			const XRRModeInfo * modeInfo = &screenResources->modes[i];
			if ( modeInfo->id == primaryCrtcInfo->mode )
			{
				*currentWidth = modeInfo->width;
				*currentHeight = modeInfo->height;
				*currentRefreshRate = modeInfo->dotClock / ( (float)modeInfo->hTotal * (float)modeInfo->vTotal );
				break;
			}
		}
	}

	if ( desiredWidth != NULL && desiredHeight != NULL && desiredRefreshRate != NULL )
	{
		RRMode bestMode = 0;
		int bestModeWidth = 0;
		int bestModeHeight = 0;
		float bestModeRefreshRate = 0.0f;
		int bestSizeError = 0x7FFFFFFF;
		float bestRefreshRateError = 1e6f;

		for ( int i = 0; i < screenResources->nmode; i++ )
		{
			const XRRModeInfo * modeInfo = &screenResources->modes[i];

			if ( modeInfo->modeFlags & RR_Interlace )
			{
				continue;
			}

			bool validOutputMode = false;
			for ( int j = 0; j < primaryOutputInfo->nmode; j++ )
			{
				if ( modeInfo->id == primaryOutputInfo->modes[j] )
				{
					validOutputMode = true;
					break;
				}
			}
			if ( !validOutputMode )
			{
				continue;
			}

			const int modeWidth = modeInfo->width;
			const int modeHeight = modeInfo->height;
			const float modeRefreshRate = modeInfo->dotClock / ( (float)modeInfo->hTotal * (float)modeInfo->vTotal );

			const int dw = modeWidth - *desiredWidth;
			const int dh = modeHeight - *desiredHeight;
			const int sizeError = dw * dw + dh * dh;
			const float refreshRateError = fabs( modeRefreshRate - *desiredRefreshRate );
			if ( sizeError < bestSizeError || ( sizeError == bestSizeError && refreshRateError < bestRefreshRateError ) )
			{
				bestSizeError = sizeError;
				bestRefreshRateError = refreshRateError;
				bestMode = modeInfo->id;
				bestModeWidth = modeWidth;
				bestModeHeight = modeHeight;
				bestModeRefreshRate = modeRefreshRate;
			}
		}

		XRRSetCrtcConfig( xDisplay, screenResources, primaryOutputInfo->crtc, CurrentTime,
							primaryCrtcInfo->x, primaryCrtcInfo->y, bestMode, primaryCrtcInfo->rotation,
							primaryCrtcInfo->outputs, primaryCrtcInfo->noutput );

		*desiredWidth = bestModeWidth;
		*desiredHeight = bestModeHeight;
		*desiredRefreshRate = bestModeRefreshRate;
	}

	XRRFreeOutputInfo( primaryOutputInfo );
	XRRFreeCrtcInfo( primaryCrtcInfo );
	XRRFreeScreenResources( screenResources );

	return true;
}

static void GpuWindow_Destroy( GpuWindow_t * window )
{
	GpuWindow_DestroySurface( window );
	GpuContext_Destroy( &window->context );
	GpuDevice_Destroy( &window->device );

	if ( window->windowFullscreen )
	{
		ChangeVideoMode_XF86VidMode( window->xDisplay, window->xScreen, window->xRoot,
									NULL, NULL, NULL,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate );

		XUngrabPointer( window->xDisplay, CurrentTime );
		XUngrabKeyboard( window->xDisplay, CurrentTime );
	}

	if ( window->xWindow )
	{
		XUnmapWindow( window->xDisplay, window->xWindow );
		XDestroyWindow( window->xDisplay, window->xWindow );
		window->xWindow = 0;
	}

	XFlush( window->xDisplay );
	XCloseDisplay( window->xDisplay );
	window->xDisplay = NULL;
}

static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( GpuWindow_t ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeMicroseconds();

	const char * displayName = NULL;
	window->xDisplay = XOpenDisplay( displayName );
	if ( !window->xDisplay )
	{
		Error( "Unable to open X Display." );
		return false;
	}

	window->xScreen = XDefaultScreen( window->xDisplay );
	window->xRoot = XRootWindow( window->xDisplay, window->xScreen );

	if ( window->windowFullscreen )
	{
		ChangeVideoMode_XF86VidMode( window->xDisplay, window->xScreen, window->xRoot,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate,
									&window->windowWidth, &window->windowHeight, &window->windowRefreshRate );
	}
	else
	{
		ChangeVideoMode_XF86VidMode( window->xDisplay, window->xScreen, window->xRoot,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate,
									NULL, NULL, NULL );
		window->windowRefreshRate = window->desktopRefreshRate;
	}

	Colormap defaultColorMap = XDefaultColormap( window->xDisplay, window->xScreen );

	const unsigned long wamask = CWColormap | CWEventMask | ( window->windowFullscreen ? 0 : CWBorderPixel );

	XSetWindowAttributes wa;
	memset( &wa, 0, sizeof( wa ) );
	wa.colormap = defaultColorMap;
	wa.border_pixel = 0;
	wa.event_mask = StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask |
					KeyPressMask | KeyReleaseMask |
					ButtonPressMask | ButtonReleaseMask |
					FocusChangeMask | ExposureMask | VisibilityChangeMask |
					EnterWindowMask | LeaveWindowMask;

	window->xWindow = XCreateWindow(	window->xDisplay,		// Display * display
										window->xRoot,			// Window parent
										0,						// int x
										0,						// int y
										window->windowWidth,	// unsigned int width
										window->windowHeight,	// unsigned int height
										0,						// unsigned int border_width
										CopyFromParent,			// int depth
										InputOutput,			// unsigned int class
										CopyFromParent,			// Visual * visual
										wamask,					// unsigned long valuemask
										&wa );					// XSetWindowAttributes * attributes

	if ( !window->xWindow )
	{
		Error( "Failed to create window." );
		GpuWindow_Destroy( window );
		return false;
	}

	// Change the window title.
	Atom _NET_WM_NAME = XInternAtom( window->xDisplay, "_NET_WM_NAME", False );
	XChangeProperty( window->xDisplay, window->xWindow, _NET_WM_NAME,
						XA_STRING, 8, PropModeReplace,
						(const unsigned char *)WINDOW_TITLE, strlen( WINDOW_TITLE ) );

	if ( window->windowFullscreen )
	{
		// Bypass the compositor in fullscreen mode.
		const unsigned long bypass = 1;
		Atom _NET_WM_BYPASS_COMPOSITOR = XInternAtom( window->xDisplay, "_NET_WM_BYPASS_COMPOSITOR", False );
		XChangeProperty( window->xDisplay, window->xWindow, _NET_WM_BYPASS_COMPOSITOR,
							XA_CARDINAL, 32, PropModeReplace, (const unsigned char*)&bypass, 1 );

		// Completely dissasociate window from window manager.
		XSetWindowAttributes attributes;
		attributes.override_redirect = True;
		XChangeWindowAttributes( window->xDisplay, window->xWindow, CWOverrideRedirect, &attributes );

		// Make the window visible.
		XMapRaised( window->xDisplay, window->xWindow );
		XMoveResizeWindow( window->xDisplay, window->xWindow, 0, 0, window->windowWidth, window->windowHeight );
		XFlush( window->xDisplay );

		// Grab mouse and keyboard input now that the window is disassociated from the window manager.
		XGrabPointer( window->xDisplay, window->xWindow, True, 0, GrabModeAsync, GrabModeAsync, window->xWindow, 0L, CurrentTime );
		XGrabKeyboard( window->xDisplay, window->xWindow, True, GrabModeAsync, GrabModeAsync, CurrentTime );
	}
	else
	{
		// Make the window fixed size.
		XSizeHints * hints = XAllocSizeHints();
		hints->flags = ( PMinSize | PMaxSize );
		hints->min_width = window->windowWidth;
		hints->max_width = window->windowWidth;
		hints->min_height = window->windowHeight;
		hints->max_height = window->windowHeight;
		XSetWMNormalHints( window->xDisplay, window->xWindow, hints );
		XFree( hints );

		// First map the window and then center the window on the screen.
		XMapRaised( window->xDisplay, window->xWindow );
		const int x = ( window->desktopWidth - window->windowWidth ) / 2;
		const int y = ( window->desktopHeight - window->windowHeight ) / 2;
		XMoveResizeWindow( window->xDisplay, window->xWindow, x, y, window->windowWidth, window->windowHeight );
		XFlush( window->xDisplay );
	}

	VkXlibSurfaceCreateInfoKHR xlibSurfaceCreateInfo;
	xlibSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	xlibSurfaceCreateInfo.pNext = NULL;
	xlibSurfaceCreateInfo.flags = 0;
	xlibSurfaceCreateInfo.dpy = window->xDisplay;
	xlibSurfaceCreateInfo.window = window->xWindow;

	VkSurfaceKHR surface;
	VK( instance->vkCreateSurfaceKHR( instance->instance, &xlibSurfaceCreateInfo, VK_ALLOCATOR, &surface ) );

	GpuDevice_Create( &window->device, instance, queueInfo, surface );
	GpuContext_Create( &window->context, &window->device, queueIndex );
	GpuWindow_CreateFromSurface( window, surface );

	return true;
}

static void GpuWindow_Exit( GpuWindow_t * window )
{
	window->windowExit = true;
}

static GpuWindowEvent_t GpuWindow_ProcessEvents( GpuWindow_t * window )
{
	int count = XPending( window->xDisplay );
	for ( int i = 0; i < count; i++ )
	{
		XEvent event;
		XNextEvent( window->xDisplay, &event );

		switch ( event.type )
		{
			case KeyPress:
			{
				KeySym key = XLookupKeysym( &event.xkey, 0 );
				if ( key < 256 || key == XK_Escape )
				{
					window->keyInput[key & 255] = true;
				}
				break;
			}
			case KeyRelease:
			{
				break;
			}
			case ButtonPress:
			{
				window->mouseInput[event.xbutton.button] = true;
				window->mouseInputX[event.xbutton.button] = event.xbutton.x;
				window->mouseInputY[event.xbutton.button] = event.xbutton.y;
			}
			case ButtonRelease:
			{
				break;
			}
			// StructureNotifyMask
			case ConfigureNotify:
			case MapNotify:
			case UnmapNotify:
			case DestroyNotify:
			// PropertyChangeMask
			case PropertyNotify:
			// ResizeRedirectMask
			case ResizeRequest:
			// EnterWindowMask | LeaveWindowMask
			case EnterNotify:
			case LeaveNotify:
			// FocusChangeMask
			case FocusIn:
			case FocusOut:
			// ExposureMask
			case Expose:
			// VisibilityChangeMask
			case VisibilityNotify:

			case GenericEvent:
			default: break;
		}
	}

	if ( window->windowExit )
	{
		return GPU_WINDOW_EVENT_EXIT;
	}

	if ( window->windowActive == false )
	{
		window->windowActive = true;
		return GPU_WINDOW_EVENT_ACTIVATED;
	}

	return GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_LINUX_XCB )

typedef enum	// keysym.h
{
	KEY_ESCAPE		= ( XK_Escape & 255 ),
	KEY_A			= XK_a,
	KEY_B			= XK_b,
	KEY_C			= XK_c,
	KEY_D			= XK_d,
	KEY_E			= XK_e,
	KEY_F			= XK_f,
	KEY_G			= XK_g,
	KEY_H			= XK_h,
	KEY_I			= XK_i,
	KEY_J			= XK_j,
	KEY_K			= XK_k,
	KEY_L			= XK_l,
	KEY_M			= XK_m,
	KEY_N			= XK_n,
	KEY_O			= XK_o,
	KEY_P			= XK_p,
	KEY_Q			= XK_q,
	KEY_R			= XK_r,
	KEY_S			= XK_s,
	KEY_T			= XK_t,
	KEY_U			= XK_u,
	KEY_V			= XK_v,
	KEY_W			= XK_w,
	KEY_X			= XK_x,
	KEY_Y			= XK_y,
	KEY_Z			= XK_z
} KeyboardKey_t;

typedef enum
{
	MOUSE_LEFT		= 0,
	MOUSE_RIGHT		= 1
} MouseButton_t;

typedef enum
{
	XCB_SIZE_HINT_US_POSITION	= 1 << 0,
	XCB_SIZE_HINT_US_SIZE		= 1 << 1,
	XCB_SIZE_HINT_P_POSITION	= 1 << 2,
	XCB_SIZE_HINT_P_SIZE		= 1 << 3,
	XCB_SIZE_HINT_P_MIN_SIZE	= 1 << 4,
	XCB_SIZE_HINT_P_MAX_SIZE	= 1 << 5,
	XCB_SIZE_HINT_P_RESIZE_INC	= 1 << 6,
	XCB_SIZE_HINT_P_ASPECT		= 1 << 7,
	XCB_SIZE_HINT_BASE_SIZE		= 1 << 8,
	XCB_SIZE_HINT_P_WIN_GRAVITY	= 1 << 9
} xcb_size_hints_flags_t;

static const int _NET_WM_STATE_REMOVE	= 0;	// remove/unset property
static const int _NET_WM_STATE_ADD		= 1;	// add/set property
static const int _NET_WM_STATE_TOGGLE	= 2;	// toggle property

/*
	Change video mode using the RandR X extension version 1.4

	The following code does not necessarily work out of the box, because on
	some configurations the modes list returned by XRRGetScreenResources()
	is populated with nothing other than the maximum display resolution,
	even though XF86VidModeGetAllModeLines() and XRRConfigSizes() *will*
	list all resolutions for the same display.

	The user can manually add new modes from the command-line using the
	xrandr utility:

	xrandr --newmode <modeline>

	Where <modeline> is generated with a utility that implements either
	the General Timing Formula (GTF) or the Coordinated Video Timing (CVT)
	standard put forth by the Video Electronics Standards Association (VESA):

	gft <width> <height> <Hz>	// http://gtf.sourceforge.net/
	cvt <width> <height> <Hz>	// http://www.uruk.org/~erich/projects/cvt/

	Alternatively, new modes can be added in code using XRRCreateMode().
	However, this requires calculating all the timing information in code
	because there is no standard library that implements the GTF or CVT.
*/
static bool ChangeVideoMode_XcbRandR_1_4( xcb_connection_t * connection, xcb_screen_t * screen,
								int * currentWidth, int * currentHeight, float * currentRefreshRate,
								int * desiredWidth, int * desiredHeight, float * desiredRefreshRate )
{
	/*
		Screen	- virtual screenspace which may be covered by multiple CRTCs
		CRTC	- display controller
		Output	- display/monitor connected to a CRTC
		Clones	- outputs that are simultaneously connected to the same CRTC
	*/

	xcb_randr_get_screen_resources_cookie_t screen_resources_cookie = xcb_randr_get_screen_resources( connection, screen->root );
	xcb_randr_get_screen_resources_reply_t * screen_resources_reply = xcb_randr_get_screen_resources_reply( connection, screen_resources_cookie, 0 );
	if ( screen_resources_reply == NULL )
	{
		return false;
	}

	xcb_randr_mode_info_t * mode_info = xcb_randr_get_screen_resources_modes( screen_resources_reply );
	const int modes_length = xcb_randr_get_screen_resources_modes_length( screen_resources_reply );
	assert( modes_length > 0 );
	
	xcb_randr_crtc_t * crtcs = xcb_randr_get_screen_resources_crtcs( screen_resources_reply );
	const int crtcs_length = xcb_randr_get_screen_resources_crtcs_length( screen_resources_reply );
	assert( crtcs_length > 0 );
	UNUSED_PARM( crtcs_length );

	const int PRIMARY_CRTC_INDEX = 0;
	const int PRIMARY_OUTPUT_INDEX = 0;

	xcb_randr_get_crtc_info_cookie_t primary_crtc_info_cookie = xcb_randr_get_crtc_info( connection, crtcs[PRIMARY_CRTC_INDEX], 0 );
	xcb_randr_get_crtc_info_reply_t * primary_crtc_info_reply = xcb_randr_get_crtc_info_reply( connection, primary_crtc_info_cookie, NULL );

	xcb_randr_output_t * crtc_outputs = xcb_randr_get_crtc_info_outputs( primary_crtc_info_reply );

	xcb_randr_get_output_info_cookie_t primary_output_info_cookie = xcb_randr_get_output_info( connection, crtc_outputs[PRIMARY_OUTPUT_INDEX], 0 );
	xcb_randr_get_output_info_reply_t * primary_output_info_reply = xcb_randr_get_output_info_reply( connection, primary_output_info_cookie, NULL );

	if ( currentWidth != NULL && currentHeight != NULL && currentRefreshRate != NULL )
	{
		for ( int i = 0; i < modes_length; i++ )
		{
			if ( mode_info[i].id == primary_crtc_info_reply->mode )
			{
				*currentWidth = mode_info[i].width;
				*currentHeight = mode_info[i].height;
				*currentRefreshRate = mode_info[i].dot_clock / ( (float)mode_info[i].htotal * (float)mode_info[i].vtotal );
				break;
			}
		}
	}

	if ( desiredWidth != NULL && desiredHeight != NULL && desiredRefreshRate != NULL )
	{
		xcb_randr_mode_t bestMode = 0;
		int bestModeWidth = 0;
		int bestModeHeight = 0;
		float bestModeRefreshRate = 0.0f;
		int bestSizeError = 0x7FFFFFFF;
		float bestRefreshRateError = 1e6f;
		for ( int i = 0; i < modes_length; i++ )
		{
			if ( mode_info[i].mode_flags & XCB_RANDR_MODE_FLAG_INTERLACE )
			{
				continue;
			}

			xcb_randr_mode_t * primary_output_info_modes = xcb_randr_get_output_info_modes( primary_output_info_reply );
			int primary_output_info_modes_length = xcb_randr_get_output_info_modes_length( primary_output_info_reply );

			bool validOutputMode = false;
			for ( int j = 0; j < primary_output_info_modes_length; j++ )
			{
				if ( mode_info[i].id == primary_output_info_modes[j] )
				{
					validOutputMode = true;
					break;
				}
			}
			if ( !validOutputMode )
			{
				continue;
			}

			const int modeWidth = mode_info[i].width;
			const int modeHeight = mode_info[i].height;
			const float modeRefreshRate = mode_info[i].dot_clock / ( (float)mode_info[i].htotal * (float)mode_info[i].vtotal );

			const int dw = modeWidth - *desiredWidth;
			const int dh = modeHeight - *desiredHeight;
			const int sizeError = dw * dw + dh * dh;
			const float refreshRateError = fabs( modeRefreshRate - *desiredRefreshRate );
			if ( sizeError < bestSizeError || ( sizeError == bestSizeError && refreshRateError < bestRefreshRateError ) )
			{
				bestSizeError = sizeError;
				bestRefreshRateError = refreshRateError;
				bestMode = mode_info[i].id;
				bestModeWidth = modeWidth;
				bestModeHeight = modeHeight;
				bestModeRefreshRate = modeRefreshRate;
			}
		}

		xcb_randr_output_t * primary_crtc_info_outputs = xcb_randr_get_crtc_info_outputs( primary_crtc_info_reply );
		int primary_crtc_info_outputs_length = xcb_randr_get_crtc_info_outputs_length( primary_crtc_info_reply );

		xcb_randr_set_crtc_config( connection, primary_output_info_reply->crtc, XCB_TIME_CURRENT_TIME, XCB_TIME_CURRENT_TIME,
									primary_crtc_info_reply->x, primary_crtc_info_reply->y, bestMode, primary_crtc_info_reply->rotation,
									primary_crtc_info_outputs_length, primary_crtc_info_outputs );

		*desiredWidth = bestModeWidth;
		*desiredHeight = bestModeHeight;
		*desiredRefreshRate = bestModeRefreshRate;
	}

	free( primary_output_info_reply );
	free( primary_crtc_info_reply );
	free( screen_resources_reply );

	return true;
}

static void GpuWindow_Destroy( GpuWindow_t * window )
{
	GpuWindow_DestroySurface( window );
	GpuContext_Destroy( &window->context );
	GpuDevice_Destroy( &window->device );

	if ( window->windowFullscreen )
	{
		ChangeVideoMode_XcbRandR_1_4( window->connection, window->screen,
									NULL, NULL, NULL,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate );
	}

	xcb_destroy_window( window->connection, window->window );
	xcb_flush( window->connection );
	xcb_disconnect( window->connection );
	xcb_key_symbols_free( window->key_symbols );
}

static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( GpuWindow_t ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeMicroseconds();

	const char * displayName = NULL;
	int screen_number = 0;
	window->connection = xcb_connect( displayName, &screen_number );
	if ( xcb_connection_has_error( window->connection ) )
	{
		GpuWindow_Destroy( window );
		Error( "Failed to open XCB connection." );
		return false;
	}

	const xcb_setup_t * setup = xcb_get_setup( window->connection );
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator( setup );
	for ( int i = 0; i < screen_number; i++ )
	{
		xcb_screen_next( &iter );
	}
	window->screen = iter.data;

	if ( window->windowFullscreen )
	{
		ChangeVideoMode_XcbRandR_1_4( window->connection, window->screen,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate,
									&window->windowWidth, &window->windowHeight, &window->windowRefreshRate );
	}
	else
	{
		ChangeVideoMode_XcbRandR_1_4( window->connection, window->screen,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate,
									NULL, NULL, NULL );
		window->windowRefreshRate = window->desktopRefreshRate;
	}

	// Create the window.
	uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
	uint32_t value_list[5];
	value_list[0] = window->screen->black_pixel;
	value_list[1] = 0;
	value_list[2] = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_BUTTON_PRESS;

	window->window = xcb_generate_id( window->connection );
	xcb_create_window(	window->connection,				// xcb_connection_t *	connection
						XCB_COPY_FROM_PARENT,			// uint8_t				depth
						window->window,					// xcb_window_t			wid
						window->screen->root,			// xcb_window_t			parent
						0,								// int16_t				x
						0,								// int16_t				y
						window->windowWidth,			// uint16_t				width
						window->windowHeight,			// uint16_t				height
						0,								// uint16_t				border_width
						XCB_WINDOW_CLASS_INPUT_OUTPUT,	// uint16_t				_class
						window->screen->root_visual,	// xcb_visualid_t		visual
						value_mask,						// uint32_t				value_mask
						value_list );					// const uint32_t *		value_list

	// Change the window title.
	xcb_change_property( window->connection, XCB_PROP_MODE_REPLACE, window->window,
						XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
						8, strlen( WINDOW_TITLE ), WINDOW_TITLE );

	// Setup code that will send a notification when the window is destroyed.
	xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom( window->connection, 1, 12, "WM_PROTOCOLS" );
	xcb_intern_atom_cookie_t wm_delete_window_cookie = xcb_intern_atom( window->connection, 0, 16, "WM_DELETE_WINDOW" );
	xcb_intern_atom_reply_t * wm_protocols_reply = xcb_intern_atom_reply( window->connection, wm_protocols_cookie, 0 );
	xcb_intern_atom_reply_t * wm_delete_window_reply = xcb_intern_atom_reply( window->connection, wm_delete_window_cookie, 0 );

	window->wm_delete_window_atom = wm_delete_window_reply->atom;
	xcb_change_property( window->connection, XCB_PROP_MODE_REPLACE, window->window,
						wm_protocols_reply->atom, XCB_ATOM_ATOM,
						32, 1, &wm_delete_window_reply->atom );

	free( wm_protocols_reply );
	free( wm_delete_window_reply );

	if ( window->windowFullscreen )
	{
		// Change the window to fullscreen
		xcb_intern_atom_cookie_t wm_state_cookie = xcb_intern_atom( window->connection, 0, 13, "_NET_WM_STATE" );
		xcb_intern_atom_cookie_t wm_state_fullscreen_cookie = xcb_intern_atom( window->connection, 0, 24, "_NET_WM_STATE_FULLSCREEN" );
		xcb_intern_atom_reply_t * wm_state_reply = xcb_intern_atom_reply( window->connection, wm_state_cookie, 0 );
		xcb_intern_atom_reply_t * wm_state_fullscreen_reply = xcb_intern_atom_reply( window->connection, wm_state_fullscreen_cookie, 0 );

		xcb_client_message_event_t ev;
		ev.response_type = XCB_CLIENT_MESSAGE;
		ev.format = 32;
		ev.sequence = 0;
		ev.window = window->window;
		ev.type = wm_state_reply->atom;
		ev.data.data32[0] = _NET_WM_STATE_ADD;
		ev.data.data32[1] = wm_state_fullscreen_reply->atom;
		ev.data.data32[2] = XCB_ATOM_NONE;
		ev.data.data32[3] = 0;
		ev.data.data32[4] = 0;

		xcb_send_event(	window->connection, 1, window->window,
						XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
						(const char*)(&ev) );

		free( wm_state_reply );
		free( wm_state_fullscreen_reply );

		xcb_map_window( window->connection, window->window );
		xcb_flush( window->connection );
	}
	else
	{
		// Make the window fixed size.
		xcb_size_hints_t hints;
		memset( &hints, 0, sizeof( hints ) );
		hints.flags = XCB_SIZE_HINT_US_SIZE | XCB_SIZE_HINT_P_SIZE | XCB_SIZE_HINT_P_MIN_SIZE | XCB_SIZE_HINT_P_MAX_SIZE;
		hints.min_width = window->windowWidth;
		hints.max_width = window->windowWidth;
		hints.min_height = window->windowHeight;
		hints.max_height = window->windowHeight;

		xcb_change_property( window->connection, XCB_PROP_MODE_REPLACE, window->window,
							XCB_ATOM_WM_NORMAL_HINTS, XCB_ATOM_WM_SIZE_HINTS,
							32, sizeof( hints ) / 4, &hints );

		// First map the window and then center the window on the screen.
		xcb_map_window( window->connection, window->window );
		const uint32_t coords[] =
		{
			( window->desktopWidth - window->windowWidth ) / 2,
			( window->desktopHeight - window->windowHeight ) / 2
		};
		xcb_configure_window( window->connection, window->window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords );
		xcb_flush( window->connection );
	}

	window->key_symbols = xcb_key_symbols_alloc( window->connection );

	VkXcbSurfaceCreateInfoKHR xcbSurfaceCreateInfo;
	xcbSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	xcbSurfaceCreateInfo.pNext = NULL;
	xcbSurfaceCreateInfo.flags = 0;
	xcbSurfaceCreateInfo.connection = window->connection;
	xcbSurfaceCreateInfo.window = window->window;

	VkSurfaceKHR surface;
	VK( instance->vkCreateSurfaceKHR( instance->instance, &xcbSurfaceCreateInfo, VK_ALLOCATOR, &surface ) );

	GpuDevice_Create( &window->device, instance, queueInfo, surface );
	GpuContext_Create( &window->context, &window->device, queueIndex );
	GpuWindow_CreateFromSurface( window, surface );

	return true;
}

static void GpuWindow_Exit( GpuWindow_t * window )
{
	window->windowExit = true;
}

static GpuWindowEvent_t GpuWindow_ProcessEvents( GpuWindow_t * window )
{
	xcb_generic_event_t * event = xcb_poll_for_event( window->connection );
	if ( event != NULL )
	{
		const uint8_t event_code = ( event->response_type & 0x7f );
		switch ( event_code )
		{
			case XCB_CLIENT_MESSAGE:
			{
				const xcb_client_message_event_t * client_message_event = (const xcb_client_message_event_t *) event;
				if ( client_message_event->data.data32[0] == window->wm_delete_window_atom )
				{
					free( event );
					return GPU_WINDOW_EVENT_EXIT;
				}
				break;
			}
			case XCB_KEY_PRESS:
			{
				xcb_key_press_event_t * key_press_event = (xcb_key_press_event_t *) event;
				const xcb_keysym_t keysym = xcb_key_press_lookup_keysym( window->key_symbols, key_press_event, 0 );
				if ( keysym < 256 || keysym == XK_Escape )
				{
					window->keyInput[keysym & 255] = true;
				}
				break;
			}
			case XCB_BUTTON_PRESS:
			{
				const xcb_button_press_event_t * button_press_event = (const xcb_button_press_event_t *) event;
				const int masks[5] = { XCB_BUTTON_MASK_1, XCB_BUTTON_MASK_2, XCB_BUTTON_MASK_3, XCB_BUTTON_MASK_4, XCB_BUTTON_MASK_5 };
				for ( int i = 0; i < 5; i++ )
				{
					if ( ( button_press_event->state & masks[i] ) != 0 )
					{
						window->mouseInput[i] = true;
						window->mouseInputX[i] = button_press_event->event_x;
						window->mouseInputY[i] = button_press_event->event_y;
					}
				}
				break;
			}
			default:
			{
				break;
			}
		}
		free( event );
	}

	if ( window->windowExit )
	{
		return GPU_WINDOW_EVENT_EXIT;
	}

	if ( window->windowActive == false )
	{
		window->windowActive = true;
		return GPU_WINDOW_EVENT_ACTIVATED;
	}

	return GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_ANDROID )

typedef enum
{
	KEY_ESCAPE		= AKEYCODE_ESCAPE,
	KEY_A			= AKEYCODE_A,
	KEY_B			= AKEYCODE_B,
	KEY_C			= AKEYCODE_C,
	KEY_D			= AKEYCODE_D,
	KEY_E			= AKEYCODE_E,
	KEY_F			= AKEYCODE_F,
	KEY_G			= AKEYCODE_G,
	KEY_H			= AKEYCODE_H,
	KEY_I			= AKEYCODE_I,
	KEY_J			= AKEYCODE_J,
	KEY_K			= AKEYCODE_K,
	KEY_L			= AKEYCODE_L,
	KEY_M			= AKEYCODE_M,
	KEY_N			= AKEYCODE_N,
	KEY_O			= AKEYCODE_O,
	KEY_P			= AKEYCODE_P,
	KEY_Q			= AKEYCODE_Q,
	KEY_R			= AKEYCODE_R,
	KEY_S			= AKEYCODE_S,
	KEY_T			= AKEYCODE_T,
	KEY_U			= AKEYCODE_U,
	KEY_V			= AKEYCODE_V,
	KEY_W			= AKEYCODE_W,
	KEY_X			= AKEYCODE_X,
	KEY_Y			= AKEYCODE_Y,
	KEY_Z			= AKEYCODE_Z
} KeyboardKey_t;

typedef enum
{
	MOUSE_LEFT		= 0,
	MOUSE_RIGHT		= 1
} MouseButton_t;

static void app_handle_cmd( struct android_app * app, int32_t cmd )
{
	GpuWindow_t * window = (GpuWindow_t *)app->userData;

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
			window->resumed = true;
			break;
		}
		case APP_CMD_PAUSE:
		{
			Print( "onPause()" );
			Print( "    APP_CMD_PAUSE" );
			window->resumed = false;
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
			window->nativeWindow = NULL;
			break;
		}
		case APP_CMD_INIT_WINDOW:
		{
			Print( "surfaceCreated()" );
			Print( "    APP_CMD_INIT_WINDOW" );
			window->nativeWindow = app->window;
			break;
		}
		case APP_CMD_TERM_WINDOW:
		{
			Print( "surfaceDestroyed()" );
			Print( "    APP_CMD_TERM_WINDOW" );
			window->nativeWindow = NULL;
			break;
		}
    }
}

static int32_t app_handle_input( struct android_app * app, AInputEvent * event )
{
	GpuWindow_t * window = (GpuWindow_t *)app->userData;

	const int type = AInputEvent_getType( event );
	if ( type == AINPUT_EVENT_TYPE_KEY )
	{
		int keyCode = AKeyEvent_getKeyCode( event );
		const int action = AKeyEvent_getAction( event );
		if ( action == AKEY_EVENT_ACTION_DOWN )
		{
			// Translate controller input to useful keys.
			switch ( keyCode )
			{
				case AKEYCODE_BUTTON_A: keyCode = AKEYCODE_Q; break;
				case AKEYCODE_BUTTON_B: keyCode = AKEYCODE_W; break;
				case AKEYCODE_BUTTON_X: keyCode = AKEYCODE_E; break;
				case AKEYCODE_BUTTON_Y: keyCode = AKEYCODE_M; break;
				case AKEYCODE_BUTTON_START: keyCode = AKEYCODE_L; break;
				case AKEYCODE_BUTTON_SELECT: keyCode = AKEYCODE_ESCAPE; break;
			}
			if ( keyCode >= 0 && keyCode < 256 )
			{
				window->keyInput[keyCode] = true;
				return 1;
			}
		}
		return 0;
	}
	else if ( type == AINPUT_EVENT_TYPE_MOTION )
	{
		const int source = AInputEvent_getSource( event );
		// Events with source == AINPUT_SOURCE_TOUCHSCREEN come from the phone's builtin touch screen.
		// Events with source == AINPUT_SOURCE_MOUSE come from the trackpad on the right side of the GearVR.
		if ( source == AINPUT_SOURCE_TOUCHSCREEN || source == AINPUT_SOURCE_MOUSE )
		{
			const int action = AKeyEvent_getAction( event ) & AMOTION_EVENT_ACTION_MASK;
			const float x = AMotionEvent_getRawX( event, 0 );
			const float y = AMotionEvent_getRawY( event, 0 );
			if ( action == AMOTION_EVENT_ACTION_UP )
			{
				window->mouseInput[MOUSE_LEFT] = true;
				window->mouseInputX[MOUSE_LEFT] = (int)x;
				window->mouseInputY[MOUSE_LEFT] = (int)y;
				return 1;
			}
			return 0;
		}
	}
	return 0;
}

static void GpuWindow_Destroy( GpuWindow_t * window )
{
	GpuContext_Destroy( &window->context );
	GpuDevice_Destroy( &window->device );

	if ( window->app != NULL )
	{
		(*window->java.vm)->DetachCurrentThread( window->java.vm );
		window->java.vm = NULL;
		window->java.env = NULL;
		window->java.activity = 0;
	}
}

static float GetDisplayRefreshRate( const Java_t * java )
{
	// Retrieve Context.WINDOW_SERVICE.
	jclass contextClass = (*java->env)->FindClass( java->env, "android/content/Context" );
	jfieldID field_WINDOW_SERVICE = (*java->env)->GetStaticFieldID( java->env, contextClass, "WINDOW_SERVICE", "Ljava/lang/String;" );
	jobject WINDOW_SERVICE = (*java->env)->GetStaticObjectField( java->env, contextClass, field_WINDOW_SERVICE );
	(*java->env)->DeleteLocalRef( java->env, contextClass );

	// WindowManager windowManager = (WindowManager) activity.getSystemService( Context.WINDOW_SERVICE );
	const jclass activityClass = (*java->env)->GetObjectClass( java->env, java->activity );
	const jmethodID getSystemServiceMethodId = (*java->env)->GetMethodID( java->env, activityClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
	const jobject windowManager = (*java->env)->CallObjectMethod( java->env, java->activity, getSystemServiceMethodId, WINDOW_SERVICE );
	(*java->env)->DeleteLocalRef( java->env, activityClass );

	// Display display = windowManager.getDefaultDisplay();
	const jclass windowManagerClass = (*java->env)->GetObjectClass( java->env, windowManager );
	const jmethodID getDefaultDisplayMethodId = (*java->env)->GetMethodID( java->env, windowManagerClass, "getDefaultDisplay", "()Landroid/view/Display;" );
	const jobject display = (*java->env)->CallObjectMethod( java->env, windowManager, getDefaultDisplayMethodId );
	(*java->env)->DeleteLocalRef( java->env, windowManagerClass );

	// float refreshRate = display.getRefreshRate();
	const jclass displayClass = (*java->env)->GetObjectClass( java->env, display );
	const jmethodID getRefreshRateMethodId = (*java->env)->GetMethodID( java->env, displayClass, "getRefreshRate", "()F" );
	const float refreshRate = (*java->env)->CallFloatMethod( java->env, display, getRefreshRateMethodId );
	(*java->env)->DeleteLocalRef( java->env, displayClass );

	(*java->env)->DeleteLocalRef( java->env, display );
	(*java->env)->DeleteLocalRef( java->env, windowManager );
	(*java->env)->DeleteLocalRef( java->env, WINDOW_SERVICE );

	return refreshRate;
}

struct android_app * global_app;

static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( GpuWindow_t ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = true;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeMicroseconds();

	window->app = global_app;
	window->nativeWindow = NULL;
	window->resumed = false;

	if ( window->app != NULL )
	{
		window->app->userData = window;
		window->app->onAppCmd = app_handle_cmd;
		window->app->onInputEvent = app_handle_input;
		window->java.vm = window->app->activity->vm;
		(*window->java.vm)->AttachCurrentThread( window->java.vm, &window->java.env, NULL );
		window->java.activity = window->app->activity->clazz;

		window->windowRefreshRate = GetDisplayRefreshRate( &window->java );

		// Keep the display on and bright.
		// Also make sure there is only one "HWC" next to the "FB TARGET" (adb shell dumpsys SurfaceFlinger).
		ANativeActivity_setWindowFlags( window->app->activity, AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON, 0 );
	}

	GpuDevice_Create( &window->device, instance, queueInfo, VK_NULL_HANDLE );
	GpuContext_Create( &window->context, &window->device, queueIndex );

	return true;
}

static void GpuWindow_Exit( GpuWindow_t * window )
{
	// Call finish() on the activity and GpuWindow_ProcessEvents will handle the rest.
	ANativeActivity_finish( window->app->activity );
}

static GpuWindowEvent_t GpuWindow_ProcessEvents( GpuWindow_t * window )
{
	if ( window->app == NULL )
	{
		return GPU_WINDOW_EVENT_NONE;
	}

	const bool windowWasActive = window->windowActive;

	for ( ; ; )
	{
		int events;
		struct android_poll_source * source;
		const int timeoutMilliseconds = ( window->windowActive == false && window->app->destroyRequested == 0 ) ? -1 : 0;
		if ( ALooper_pollAll( timeoutMilliseconds, NULL, &events, (void **)&source ) < 0 )
		{
			break;
		}

		if ( source != NULL )
		{
			source->process( window->app, source );
		}

		if ( window->nativeWindow != NULL && window->swapchain.swapchain == VK_NULL_HANDLE )
		{
			Print( "        ANativeWindow_setBuffersGeometry %d x %d", window->windowWidth, window->windowHeight );
			ANativeWindow_setBuffersGeometry( window->nativeWindow, window->windowWidth, window->windowHeight, 0 );

			VkAndroidSurfaceCreateInfoKHR androidSurfaceCreateInfo;
			androidSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
			androidSurfaceCreateInfo.pNext = NULL;
			androidSurfaceCreateInfo.flags = 0;
			androidSurfaceCreateInfo.window = window->nativeWindow;

			VkSurfaceKHR surface;
			VK( window->device.instance->vkCreateSurfaceKHR( window->device.instance->instance, &androidSurfaceCreateInfo, VK_ALLOCATOR, &surface ) );

			GpuWindow_CreateFromSurface( window, surface );
		}

		if ( window->resumed != false && window->nativeWindow != NULL )
		{
			window->windowActive = true;
		}
		else
		{
			window->windowActive = false;
		}

		if ( window->nativeWindow == NULL && window->swapchain.swapchain != VK_NULL_HANDLE )
		{
			GpuWindow_DestroySurface( window );
		}
	}

	if ( window->app->destroyRequested != 0 )
	{
		return GPU_WINDOW_EVENT_EXIT;
	}
	if ( windowWasActive != window->windowActive )
	{
		return ( window->windowActive ) ? GPU_WINDOW_EVENT_ACTIVATED : GPU_WINDOW_EVENT_DEACTIVATED;
	}
	return GPU_WINDOW_EVENT_NONE;
}

#endif

static void GpuWindow_SwapInterval( GpuWindow_t * window, const int swapInterval )
{
	if ( swapInterval != window->windowSwapInterval )
	{
		GpuContext_WaitIdle( &window->context );
		GpuSwapchain_Destroy( &window->context, &window->swapchain );
		GpuSwapchain_Create( &window->context, &window->swapchain, window->surface, window->colorFormat, window->windowWidth, window->windowHeight, swapInterval );
		window->windowSwapInterval = swapInterval;
		window->swapchainCreateCount++;
	}
}

static void GpuWindow_SwapBuffers( GpuWindow_t * window )
{
	Microseconds_t newTimeMicroseconds = GpuSwapchain_SwapBuffers( &window->context, &window->swapchain );

	// Even with smoothing, this is not particularly accurate.
	const float frameTimeMicroseconds = 1000.0f * 1000.0f / window->windowRefreshRate;
	const float deltaTimeMicroseconds = (float)newTimeMicroseconds - window->lastSwapTime - frameTimeMicroseconds;
	if ( fabs( deltaTimeMicroseconds ) < frameTimeMicroseconds * 0.75f )
	{
		newTimeMicroseconds = (Microseconds_t)( window->lastSwapTime + frameTimeMicroseconds + 0.025f * deltaTimeMicroseconds );
	}
	//const float smoothDeltaMicroseconds = (float)( newTimeMicroseconds - window->lastSwapTime );
	//Print( "frame delta = %1.3f (error = %1.3f)\n", smoothDeltaMicroseconds * ( 1.0f / 1000.0f ),
	//					( smoothDeltaMicroseconds - frameTimeMicroseconds ) * ( 1.0f / 1000.0f ) );
	window->lastSwapTime = newTimeMicroseconds;
}

static Microseconds_t GpuWindow_GetNextSwapTime( GpuWindow_t * window )
{
	const float frameTimeMicroseconds = 1000.0f * 1000.0f / window->windowRefreshRate;
	return window->lastSwapTime + (Microseconds_t)( frameTimeMicroseconds );
}

static bool GpuWindow_CheckKeyboardKey( GpuWindow_t * window, const KeyboardKey_t key )
{
	if ( window->keyInput[key] )
	{
		window->keyInput[key] = false;
		return true;
	}
	return false;
}

static bool GpuWindow_CheckMouseButton( GpuWindow_t * window, const MouseButton_t button )
{
	if ( window->mouseInput[button] )
	{
		window->mouseInput[button] = false;
		return true;
	}
	return false;
}

/*
================================================================================================================================

GPU buffer.

A buffer maintains a block of memory for a specific use by GPU programs (vertex, index, uniform, storage).
For optimal performance, a buffer should only be created at load time, not at runtime.
The best performance is typically achieved when the buffer is not host visible.

GpuBufferType_t
GpuBuffer_t

static bool GpuBuffer_Create( GpuContext_t * context, GpuBuffer_t * buffer, const GpuBufferType_t type,
							const size_t dataSize, const void * data, const bool hostVisible );
static void GpuBuffer_Destroy( GpuContext_t * context, GpuBuffer_t * buffer );

================================================================================================================================
*/

typedef enum
{
	GPU_BUFFER_TYPE_VERTEX,
	GPU_BUFFER_TYPE_INDEX,
	GPU_BUFFER_TYPE_UNIFORM,
	GPU_BUFFER_TYPE_STORAGE
} GpuBufferType_t;

typedef struct GpuBuffer_s
{
	struct GpuBuffer_s *	next;
	int						unusedCount;
	GpuBufferType_t			type;
	size_t					size;
	VkMemoryPropertyFlags	flags;
	VkBuffer				buffer;
	VkDeviceMemory			memory;
	void *					mapped;
} GpuBuffer_t;

static VkBufferUsageFlags GpuBuffer_GetBufferUsage( const GpuBufferType_t type )
{
	return	( ( type == GPU_BUFFER_TYPE_VERTEX ) ?	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT :
			( ( type == GPU_BUFFER_TYPE_INDEX ) ?	VK_BUFFER_USAGE_INDEX_BUFFER_BIT :
			( ( type == GPU_BUFFER_TYPE_UNIFORM ) ?	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT :
			( ( type == GPU_BUFFER_TYPE_STORAGE ) ?	VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0 ) ) ) );
}

static VkAccessFlags GpuBuffer_GetBufferAccess( const GpuBufferType_t type )
{
	return	( ( type == GPU_BUFFER_TYPE_INDEX ) ?	VK_ACCESS_INDEX_READ_BIT :
			( ( type == GPU_BUFFER_TYPE_VERTEX ) ?	VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT :
			( ( type == GPU_BUFFER_TYPE_UNIFORM ) ?	VK_ACCESS_UNIFORM_READ_BIT :
			( ( type == GPU_BUFFER_TYPE_STORAGE ) ?	( VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT ) : 0 ) ) ) );
}

static void GpuBuffer_StagedCopy( GpuContext_t * context, GpuBuffer_t * destBuffer, const size_t dataSize, const void * data )
{
	VkBufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = dataSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = NULL;

	VkBuffer srcBuffer;
	VK( context->device->vkCreateBuffer( context->device->device, &bufferCreateInfo, VK_ALLOCATOR, &srcBuffer ) );

	VkMemoryRequirements memoryRequirements;
	VC( context->device->vkGetBufferMemoryRequirements( context->device->device, srcBuffer, &memoryRequirements ) );

	VkMemoryAllocateInfo memoryAllocateInfo;
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = NULL;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = GpuDevice_GetMemoryTypeIndex( context->device, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );

	VkDeviceMemory srcMemory;
	VK( context->device->vkAllocateMemory( context->device->device, &memoryAllocateInfo, VK_ALLOCATOR, &srcMemory ) );
	VK( context->device->vkBindBufferMemory( context->device->device, srcBuffer, srcMemory, 0 ) );

	void * mapped;
	VK( context->device->vkMapMemory( context->device->device, srcMemory, 0, memoryRequirements.size, 0, &mapped ) );
	memcpy( mapped, data, dataSize );
	VC( context->device->vkUnmapMemory( context->device->device, srcMemory ) );

	GpuContext_CreateSetupCmdBuffer( context );

	VkBufferCopy bufferCopy;
	bufferCopy.srcOffset = 0;
	bufferCopy.dstOffset = 0;
	bufferCopy.size = dataSize;

	VC( context->device->vkCmdCopyBuffer( context->setupCommandBuffer, srcBuffer, destBuffer->buffer, 1, &bufferCopy ) );

	GpuContext_FlushSetupCmdBuffer( context );

	VC( context->device->vkDestroyBuffer( context->device->device, srcBuffer, VK_ALLOCATOR ) );
	VC( context->device->vkFreeMemory( context->device->device, srcMemory, VK_ALLOCATOR ) );
}

static bool GpuBuffer_Create( GpuContext_t * context, GpuBuffer_t * buffer, const GpuBufferType_t type,
							const size_t dataSize, const void * data, const bool hostVisible )
{
	memset( buffer, 0, sizeof( GpuBuffer_t ) );

	assert( dataSize <= context->device->physicalDeviceProperties.limits.maxStorageBufferRange );

	buffer->type = type;
	buffer->size = dataSize;

	VkBufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = dataSize;
	bufferCreateInfo.usage = GpuBuffer_GetBufferUsage( type ) |
							( hostVisible ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : VK_BUFFER_USAGE_TRANSFER_DST_BIT );
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = NULL;

	VK( context->device->vkCreateBuffer( context->device->device, &bufferCreateInfo, VK_ALLOCATOR, &buffer->buffer ) );

	buffer->flags = hostVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkMemoryRequirements memoryRequirements;
	VC( context->device->vkGetBufferMemoryRequirements( context->device->device, buffer->buffer, &memoryRequirements ) );

	VkMemoryAllocateInfo memoryAllocateInfo;
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = NULL;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = GpuDevice_GetMemoryTypeIndex( context->device, memoryRequirements.memoryTypeBits, buffer->flags );

	VK( context->device->vkAllocateMemory( context->device->device, &memoryAllocateInfo, VK_ALLOCATOR, &buffer->memory ) );
	VK( context->device->vkBindBufferMemory( context->device->device, buffer->buffer, buffer->memory, 0 ) );

	if ( data != NULL )
	{
		if ( hostVisible )
		{
			void * mapped;
			VK( context->device->vkMapMemory( context->device->device, buffer->memory, 0, memoryRequirements.size, 0, &mapped ) );
			memcpy( mapped, data, dataSize );
			VC( context->device->vkUnmapMemory( context->device->device, buffer->memory ) );
		}
		else
		{
			GpuBuffer_StagedCopy( context, buffer, dataSize, data );
		}
	}

	return true;
}

static void GpuBuffer_Destroy( GpuContext_t * context, GpuBuffer_t * buffer )
{
	if ( buffer->mapped != NULL )
	{
		VC( context->device->vkUnmapMemory( context->device->device, buffer->memory ) );
	}
	VC( context->device->vkDestroyBuffer( context->device->device, buffer->buffer, VK_ALLOCATOR ) );
	VC( context->device->vkFreeMemory( context->device->device, buffer->memory, VK_ALLOCATOR ) );
}

/*
================================================================================================================================

GPU texture.

Supports loading textures from raw data.
For optimal performance, a texture should only be created or modified at load time, not at runtime.
Note that the geometry code assumes the texture origin 0,0 = left-top as opposed to left-bottom.
In other words, textures are expected to be stored top-down as opposed to bottom-up.

GpuTextureFormat_t
GpuTextureUsage_t
GpuTextureWrapMode_t
GpuTextureFilter_t
GpuTextureDefault_t
GpuTexture_t

static bool GpuTexture_Create2D( GpuContext_t * context, GpuTexture_t * texture,
								const GpuTextureFormat_t format, const int width, const int height,
								const int numberOfMipmapLevels,
								const void * data, const size_t dataSize );
static bool GpuTexture_Create2DArray( GpuContext_t * context, GpuTexture_t * texture,
								const GpuTextureFormat_t format, const int width, const int height,
								const int numberOfArrayElements, const int numberOfMipmapLevels,
								const void * data, const size_t dataSize );
static bool GpuTexture_CreateDefault( GpuContext_t * context, GpuTexture_t * texture, const GpuTextureDefault_t defaultType,
								const int width, const int height, const int depth,
								const int numberOfArrayElements, const int numberOfFaces,
								const bool mipmaps, const bool border );
static bool GpuTexture_CreateFromFile( GpuContext_t * context, GpuTexture_t * texture, const char * fileName );
static bool GpuTexture_CreateFromSwapChain( GpuContext_t * context, GpuTexture_t * texture, const GpuWindow_t * window, int index );
static void GpuTexture_Destroy( GpuContext_t * context, GpuTexture_t * texture );

static void GpuTexture_SetFilter( GpuContext_t * context, GpuTexture_t * texture, const GpuTextureFilter_t filter );
static void GpuTexture_SetAniso( GpuContext_t * context, GpuTexture_t * texture, const float maxAniso );
static void GpuTexture_SetWrapMode( GpuContext_t * context, GpuTexture_t * texture, const GpuTextureWrapMode_t wrapMode );

================================================================================================================================
*/

// Note that the channel listed first in the name shall occupy the least significant bit.
typedef enum
{
	GPU_TEXTURE_FORMAT_R8_UNORM				= VK_FORMAT_R8_UNORM,
	GPU_TEXTURE_FORMAT_R8G8_UNORM			= VK_FORMAT_R8G8_UNORM,
	GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM		= VK_FORMAT_R8G8B8A8_UNORM,

	GPU_TEXTURE_FORMAT_R8_SRGB				= VK_FORMAT_R8_SRGB,
	GPU_TEXTURE_FORMAT_R8G8_SRGB			= VK_FORMAT_R8G8_SRGB,
	GPU_TEXTURE_FORMAT_R8G8B8A8_SRGB		= VK_FORMAT_R8G8B8A8_SRGB,

	GPU_TEXTURE_FORMAT_R16_SFLOAT			= VK_FORMAT_R16_SFLOAT,
	GPU_TEXTURE_FORMAT_R16G16_SFLOAT		= VK_FORMAT_R16G16_SFLOAT,
	GPU_TEXTURE_FORMAT_R16G16B16A16_SFLOAT	= VK_FORMAT_R16G16B16A16_SFLOAT,

	GPU_TEXTURE_FORMAT_R32_SFLOAT			= VK_FORMAT_R32_SFLOAT,
	GPU_TEXTURE_FORMAT_R32G32_SFLOAT		= VK_FORMAT_R32G32_SFLOAT,
	GPU_TEXTURE_FORMAT_R32G32B32A32_SFLOAT	= VK_FORMAT_R32G32B32A32_SFLOAT,

	GPU_TEXTURE_FORMAT_BC1_R8G8B8_UNORM		= VK_FORMAT_BC1_RGB_UNORM_BLOCK,		// line through 3D space, unsigned
	GPU_TEXTURE_FORMAT_BC1_R8G8B8A1_UNORM	= VK_FORMAT_BC1_RGBA_UNORM_BLOCK,		// line through 3D space plus 1-bit alpha, unsigned
	GPU_TEXTURE_FORMAT_BC2_R8G8B8A8_UNORM	= VK_FORMAT_BC2_UNORM_BLOCK,			// line through 3D space plus line through 1D space, unsigned
	GPU_TEXTURE_FORMAT_BC3_R8G8B8A4_UNORM	= VK_FORMAT_BC3_UNORM_BLOCK,			// line through 3D space plus 4-bit alpha, unsigned

	GPU_TEXTURE_FORMAT_BC1_R8G8B8_SRGB		= VK_FORMAT_BC1_RGB_SRGB_BLOCK,			// line through 3D space, sRGB
	GPU_TEXTURE_FORMAT_BC1_R8G8B8A1_SRGB	= VK_FORMAT_BC1_RGBA_SRGB_BLOCK,		// line through 3D space plus 1-bit alpha, sRGB
	GPU_TEXTURE_FORMAT_BC2_R8G8B8A8_SRGB	= VK_FORMAT_BC2_SRGB_BLOCK,				// line through 3D space plus line through 1D space, sRGB
	GPU_TEXTURE_FORMAT_BC3_R8G8B8A4_SRGB	= VK_FORMAT_BC3_SRGB_BLOCK,				// line through 3D space plus 4-bit alpha, sRGB
    
	GPU_TEXTURE_FORMAT_BC4_R8_UNORM			= VK_FORMAT_BC4_UNORM_BLOCK,			// line through 1D space, unsigned
	GPU_TEXTURE_FORMAT_BC5_R8G8_UNORM		= VK_FORMAT_BC5_UNORM_BLOCK,			// line through 2D space, unsigned

	GPU_TEXTURE_FORMAT_BC4_R8_SNORM			= VK_FORMAT_BC4_SNORM_BLOCK,			// line through 1D space, signed
	GPU_TEXTURE_FORMAT_BC5_R8G8_SNORM		= VK_FORMAT_BC5_SNORM_BLOCK,			// line through 2D space, signed

	GPU_TEXTURE_FORMAT_ETC2_R8G8B8_UNORM	= VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
	GPU_TEXTURE_FORMAT_ETC2_R8G8B8A1_UNORM	= VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
	GPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_UNORM	= VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,

	GPU_TEXTURE_FORMAT_ETC2_R8G8B8_SRGB		= VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
	GPU_TEXTURE_FORMAT_ETC2_R8G8B8A1_SRGB	= VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
	GPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_SRGB	= VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,

	GPU_TEXTURE_FORMAT_ASTC_4x4_UNORM		= VK_FORMAT_ASTC_4x4_UNORM_BLOCK,		// four-components ASTC 4x4 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_5x4_UNORM		= VK_FORMAT_ASTC_5x4_UNORM_BLOCK,		// four-components ASTC 5x4 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_5x5_UNORM		= VK_FORMAT_ASTC_5x5_UNORM_BLOCK,		// four-components ASTC 5x5 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_6x5_UNORM		= VK_FORMAT_ASTC_6x5_UNORM_BLOCK,		// four-components ASTC 6x5 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_6x6_UNORM		= VK_FORMAT_ASTC_6x6_UNORM_BLOCK,		// four-components ASTC 6x6 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_8x5_UNORM		= VK_FORMAT_ASTC_8x5_UNORM_BLOCK,		// four-components ASTC 8x5 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_8x6_UNORM		= VK_FORMAT_ASTC_8x6_UNORM_BLOCK,		// four-components ASTC 8x6 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_8x8_UNORM		= VK_FORMAT_ASTC_8x8_UNORM_BLOCK,		// four-components ASTC 8x8 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_10x5_UNORM		= VK_FORMAT_ASTC_10x5_UNORM_BLOCK,		// four-components ASTC 10x5 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_10x6_UNORM		= VK_FORMAT_ASTC_10x6_UNORM_BLOCK,		// four-components ASTC 10x6 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_10x8_UNORM		= VK_FORMAT_ASTC_10x8_UNORM_BLOCK,		// four-components ASTC 10x8 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_10x10_UNORM		= VK_FORMAT_ASTC_10x10_UNORM_BLOCK,		// four-components ASTC 10x10 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_12x10_UNORM		= VK_FORMAT_ASTC_12x10_UNORM_BLOCK,		// four-components ASTC 12x10 block compressed, unsigned
	GPU_TEXTURE_FORMAT_ASTC_12x12_UNORM		= VK_FORMAT_ASTC_12x12_UNORM_BLOCK,		// four-components ASTC 12x12 block compressed, unsigned
	
	GPU_TEXTURE_FORMAT_ASTC_4x4_SRGB		= VK_FORMAT_ASTC_4x4_SRGB_BLOCK,		// four-components ASTC 4x4 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_5x4_SRGB		= VK_FORMAT_ASTC_5x4_SRGB_BLOCK,		// four-components ASTC 5x4 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_5x5_SRGB		= VK_FORMAT_ASTC_5x5_SRGB_BLOCK,		// four-components ASTC 5x5 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_6x5_SRGB		= VK_FORMAT_ASTC_6x5_SRGB_BLOCK,		// four-components ASTC 6x5 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_6x6_SRGB		= VK_FORMAT_ASTC_6x6_SRGB_BLOCK,		// four-components ASTC 6x6 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_8x5_SRGB		= VK_FORMAT_ASTC_8x5_SRGB_BLOCK,		// four-components ASTC 8x5 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_8x6_SRGB		= VK_FORMAT_ASTC_8x6_SRGB_BLOCK,		// four-components ASTC 8x6 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_8x8_SRGB		= VK_FORMAT_ASTC_8x8_SRGB_BLOCK,		// four-components ASTC 8x8 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_10x5_SRGB		= VK_FORMAT_ASTC_10x5_SRGB_BLOCK,		// four-components ASTC 10x5 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_10x6_SRGB		= VK_FORMAT_ASTC_10x6_SRGB_BLOCK,		// four-components ASTC 10x6 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_10x8_SRGB		= VK_FORMAT_ASTC_10x8_SRGB_BLOCK,		// four-components ASTC 10x8 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_10x10_SRGB		= VK_FORMAT_ASTC_10x10_SRGB_BLOCK,		// four-components ASTC 10x10 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_12x10_SRGB		= VK_FORMAT_ASTC_12x10_SRGB_BLOCK,		// four-components ASTC 12x10 block compressed, sRGB
	GPU_TEXTURE_FORMAT_ASTC_12x12_SRGB		= VK_FORMAT_ASTC_12x12_SRGB_BLOCK,		// four-components ASTC 12x12 block compressed, sRGB
} GpuTextureFormat_t;

typedef enum
{
	GPU_TEXTURE_USAGE_UNDEFINED,
	GPU_TEXTURE_USAGE_SAMPLED,
	GPU_TEXTURE_USAGE_STORAGE,
	GPU_TEXTURE_USAGE_COLOR_ATTACHMENT,
	GPU_TEXTURE_USAGE_PRESENTATION
} GpuTextureUsage_t;

typedef enum
{
	GPU_TEXTURE_WRAP_MODE_REPEAT,
	GPU_TEXTURE_WRAP_MODE_CLAMP_TO_EDGE,
	GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER
} GpuTextureWrapMode_t;

typedef enum
{
	GPU_TEXTURE_FILTER_NEAREST,
	GPU_TEXTURE_FILTER_LINEAR,
	GPU_TEXTURE_FILTER_BILINEAR
} GpuTextureFilter_t;

typedef enum
{
	GPU_TEXTURE_DEFAULT_CHECKERBOARD,	// 16x16 checkerboard pattern (GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM)
	GPU_TEXTURE_DEFAULT_CIRCLES,		// 32x32 block pattern with circles (GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM)
	GPU_TEXTURE_DEFAULT_PYRAMIDS		// 16x16 block pattern of pyramids (GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM)
} GpuTextureDefault_t;

typedef struct
{
	int						width;
	int						height;
	int						depth;
	int						layerCount;
	int						mipCount;
	GpuTextureUsage_t		usage;
	GpuTextureWrapMode_t	wrapMode;
	GpuTextureFilter_t		filter;
	float					maxAnisotropy;
	VkFormat				format;
	VkImageLayout			imageLayout;
	VkImage					image;
	VkDeviceMemory			memory;
	VkImageView				view;
	VkSampler				sampler;
} GpuTexture_t;

static void GpuTexture_UpdateSampler( GpuContext_t * context, GpuTexture_t * texture )
{
	if ( texture->sampler != VK_NULL_HANDLE )
	{
		VC( context->device->vkDestroySampler( context->device->device, texture->sampler, VK_ALLOCATOR ) );
	}

	const VkSamplerMipmapMode mipmapMode =	( ( texture->filter == GPU_TEXTURE_FILTER_NEAREST ) ? VK_SAMPLER_MIPMAP_MODE_NEAREST :
											( ( texture->filter == GPU_TEXTURE_FILTER_LINEAR  ) ? VK_SAMPLER_MIPMAP_MODE_NEAREST :
											( VK_SAMPLER_MIPMAP_MODE_LINEAR ) ) );
	const VkSamplerAddressMode addressMode =	( ( texture->wrapMode == GPU_TEXTURE_WRAP_MODE_CLAMP_TO_EDGE ) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE :
												( ( texture->wrapMode == GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER ) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER :
												( VK_SAMPLER_ADDRESS_MODE_REPEAT ) ) );

	VkSamplerCreateInfo samplerCreateInfo;
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext = NULL;
	samplerCreateInfo.flags = 0;
	samplerCreateInfo.magFilter = ( texture->filter == GPU_TEXTURE_FILTER_NEAREST ) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = ( texture->filter == GPU_TEXTURE_FILTER_NEAREST ) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = mipmapMode;
	samplerCreateInfo.addressModeU = addressMode;
	samplerCreateInfo.addressModeV = addressMode;
	samplerCreateInfo.addressModeW = addressMode;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.anisotropyEnable = ( texture->maxAnisotropy > 1.0f );
	samplerCreateInfo.maxAnisotropy = texture->maxAnisotropy;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	VK( context->device->vkCreateSampler( context->device->device, &samplerCreateInfo, VK_ALLOCATOR, &texture->sampler ) );
}

static int IntegerLog2( int i )
{
	int r = 0;
	int t;
	t = ( (~( ( i >> 16 ) + ~0U ) ) >> 27 ) & 0x10; r |= t; i >>= t;
	t = ( (~( ( i >>  8 ) + ~0U ) ) >> 28 ) & 0x08; r |= t; i >>= t;
	t = ( (~( ( i >>  4 ) + ~0U ) ) >> 29 ) & 0x04; r |= t; i >>= t;
	t = ( (~( ( i >>  2 ) + ~0U ) ) >> 30 ) & 0x02; r |= t; i >>= t;
	return ( r | ( i >> 1 ) );
}

// 'width' must be >= 1 and <= 32768.
// 'height' must be >= 1 and <= 32768.
// 'depth' must be >= 1 and <= 32768.
// 'numberOfArrayElements' must be >= 1.
// 'numberOfFaces' must be either 1 or 6.
// 'numberOfMipmapLevels' must be >= 1.
// 'numberOfMipmapLevels' includes the finest level.
// 'numberOfMipmapLevels' set to -1 will allocate the full mip chain.
// 'data' may be NULL to allocate a texture without initialization.
// 'dataSize' is the full data size in bytes.
// The 'data' is expected to be stored packed on a per mip level basis.
// If 'data' != NULL and 'numberOfMipmapLevels' == -1, then the full mip chain will be generated from the finest data level.
static bool GpuTexture_CreateInternal( GpuContext_t * context, GpuTexture_t * texture, const char * fileName,
										const VkFormat format, const int width, const int height, const int depth,
										const int numberOfArrayElements, const int numberOfFaces, const int numberOfMipmapLevels,
										const void * data, const size_t dataSize, const bool mipSizeStored )
{
	UNUSED_PARM( dataSize );
	UNUSED_PARM( mipSizeStored );	// Only used for KTX files.

	memset( texture, 0, sizeof( GpuTexture_t ) );

	assert( depth >= 1 );
	assert( numberOfFaces >= 1 );
	assert( numberOfArrayElements >= 1 );

	if ( width < 1 || width > 32768 || height < 1 || height > 32768 || depth < 1 || depth > 32768 )
	{
		Error( "%s: Invalid texture size (%dx%dx%d)", fileName, width, height, depth );
		return false;
	}

	if ( numberOfFaces != 1 && numberOfFaces != 6 )
	{
		Error( "%s: Cube maps must have 6 faces (%d)", fileName, numberOfFaces );
		return false;
	}

	if ( numberOfFaces != 1 && width != height )
	{
		Error( "%s: Cube maps must be square (%dx%d)", fileName, width, height );
		return false;
	}

	if ( depth > 1 && numberOfArrayElements > 1 )
	{
		Error( "%s: 3D array textures not supported", fileName );
		return false;
	}

	const int maxDimension = width > height ? ( width > depth ? width : depth ) : ( height > depth ? height : depth );
	const int maxMipLevels = ( 1 + IntegerLog2( maxDimension ) );

	if ( numberOfMipmapLevels > maxMipLevels )
	{
		Error( "%s: Too many mip levels (%d > %d)", fileName, numberOfMipmapLevels, maxMipLevels );
		return false;
	}

	VkFormatProperties props;
	VC( context->device->instance->vkGetPhysicalDeviceFormatProperties( context->device->physicalDevice, format, &props ) );

	if (	// Copy from linear staging image.
			( props.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT ) == 0 &&
			// Blit from tiled image to create mip maps.
			( props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT ) == 0 &&
			// Blit to tiled image from linear staging image, or to create mip maps.
			( props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT ) == 0 &&
			// Sample texture during rendering.
			( props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) == 0 )
	{
		Error( "%s: Unsupported texture format %d", fileName, format );
		return false;
	}

	const int numStorageLevels = ( numberOfMipmapLevels >= 1 ) ? numberOfMipmapLevels : maxMipLevels;
	const int arrayLayerCount = numberOfFaces * numberOfArrayElements;

	texture->width = width;
	texture->height = height;
	texture->depth = depth;
	texture->layerCount = arrayLayerCount;
	texture->mipCount = numStorageLevels;
	texture->wrapMode = GPU_TEXTURE_WRAP_MODE_REPEAT;
	texture->filter = ( numStorageLevels > 1 ) ? GPU_TEXTURE_FILTER_BILINEAR : GPU_TEXTURE_FILTER_LINEAR;
	texture->maxAnisotropy = 1.0f;
	texture->format = format;

	// Create tiled image.
	VkImageCreateInfo imageCreateInfo;
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = NULL;
	imageCreateInfo.flags = ( numberOfFaces == 6 ) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	imageCreateInfo.imageType = ( depth > 1 ) ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = depth;
	imageCreateInfo.mipLevels = numStorageLevels;
	imageCreateInfo.arrayLayers = arrayLayerCount;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
							VK_IMAGE_USAGE_TRANSFER_DST_BIT |
							VK_IMAGE_USAGE_SAMPLED_BIT |
							VK_IMAGE_USAGE_STORAGE_BIT |
							VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = NULL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VK( context->device->vkCreateImage( context->device->device, &imageCreateInfo, VK_ALLOCATOR, &texture->image ) );

	VkMemoryRequirements memoryRequirements;
	VC( context->device->vkGetImageMemoryRequirements( context->device->device, texture->image, &memoryRequirements ) );

	VkMemoryAllocateInfo memoryAllocateInfo;
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = NULL;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = GpuDevice_GetMemoryTypeIndex( context->device, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	VK( context->device->vkAllocateMemory( context->device->device, &memoryAllocateInfo, VK_ALLOCATOR, &texture->memory ) );
	VK( context->device->vkBindImageMemory( context->device->device, texture->image, texture->memory, 0 ) );

	if ( data == NULL )
	{
		GpuContext_CreateSetupCmdBuffer( context );

		// Set optimal image layout for shader read access.
		VkImageMemoryBarrier imageMemoryBarrier;
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = NULL;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = texture->image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = numStorageLevels;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = arrayLayerCount;

		const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		const VkDependencyFlags flags = 0;

		VC( context->device->vkCmdPipelineBarrier( context->setupCommandBuffer, src_stages, dst_stages, flags, 0, NULL, 0, NULL, 1, &imageMemoryBarrier ) );

		GpuContext_FlushSetupCmdBuffer( context );
	}
	else	// Copy source data through a linear image.
	{
		GpuContext_CreateSetupCmdBuffer( context );

		// Set optimal image layout for transfer destination.
		VkImageMemoryBarrier imageMemoryBarrier;
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = NULL;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = texture->image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = numStorageLevels;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = arrayLayerCount;

		const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		const VkDependencyFlags flags = 0;

		VC( context->device->vkCmdPipelineBarrier( context->setupCommandBuffer, src_stages, dst_stages, flags, 0, NULL, 0, NULL, 1, &imageMemoryBarrier ) );

		const int numDataLevels = ( numberOfMipmapLevels >= 1 ) ? numberOfMipmapLevels : 1;

		VkImage * linearImage = (VkImage *) malloc( numDataLevels * arrayLayerCount * depth * sizeof( VkImage ) );
		VkDeviceMemory * linearMemory = (VkDeviceMemory *) malloc( numDataLevels * arrayLayerCount * depth * sizeof( VkDeviceMemory ) );

		int dataOffset = 0;
		int imageIndex = 0;
		bool compressed = false;
		for ( int mipLevel = 0; mipLevel < numDataLevels; mipLevel++ )
		{
			const int mipWidth = ( width >> mipLevel ) >= 1 ? ( width >> mipLevel ) : 1;
			const int mipHeight = ( height >> mipLevel ) >= 1 ? ( height >> mipLevel ) : 1;
			const int mipDepth = ( depth >> mipLevel ) >= 1 ? ( depth >> mipLevel ) : 1;

			for ( int layerIndex = 0; layerIndex < arrayLayerCount; layerIndex++ )
			{
				for ( int depthIndex = 0; depthIndex < mipDepth; depthIndex++ )
				{
					// Create linear image for each mip/layer/depth.
					VkImageCreateInfo imageCreateInfo;
					imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
					imageCreateInfo.pNext = NULL;
					imageCreateInfo.flags = 0;
					imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
					imageCreateInfo.format = format;
					imageCreateInfo.extent.width = mipWidth;
					imageCreateInfo.extent.height = mipHeight;
					imageCreateInfo.extent.depth = 1;
					imageCreateInfo.mipLevels = 1;
					imageCreateInfo.arrayLayers = 1;
					imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
					imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
					imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
					imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					imageCreateInfo.queueFamilyIndexCount = 0;
					imageCreateInfo.pQueueFamilyIndices = NULL;
					imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

					VK( context->device->vkCreateImage( context->device->device, &imageCreateInfo, VK_ALLOCATOR, &linearImage[imageIndex] ) );

					VkMemoryRequirements memoryRequirements;
					VC( context->device->vkGetImageMemoryRequirements( context->device->device, linearImage[imageIndex], &memoryRequirements ) );

					VkMemoryAllocateInfo memoryAllocateInfo;
					memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
					memoryAllocateInfo.pNext = NULL;
					memoryAllocateInfo.allocationSize = memoryRequirements.size;
					memoryAllocateInfo.memoryTypeIndex = GpuDevice_GetMemoryTypeIndex( context->device, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );

					VK( context->device->vkAllocateMemory( context->device->device, &memoryAllocateInfo, VK_ALLOCATOR, &linearMemory[imageIndex] ) );
					VK( context->device->vkBindImageMemory( context->device->device, linearImage[imageIndex], linearMemory[imageIndex], 0 ) );

					// Get the source data row size.
					int dataRowCount = 0;
					int dataRowSize = 0;
					switch ( format )
					{
						case VK_FORMAT_R8_UNORM:					{ dataRowCount = mipHeight; dataRowSize = mipWidth * 1 * sizeof( unsigned char ); break; }
						case VK_FORMAT_R8G8_UNORM:					{ dataRowCount = mipHeight; dataRowSize = mipWidth * 2 * sizeof( unsigned char ); break; }
						case VK_FORMAT_R8G8B8A8_UNORM:				{ dataRowCount = mipHeight; dataRowSize = mipWidth * 4 * sizeof( unsigned char ); break; }

						case VK_FORMAT_R8_SRGB:						{ dataRowCount = mipHeight; dataRowSize = mipWidth * 1 * sizeof( unsigned char ); break; }
						case VK_FORMAT_R8G8_SRGB:					{ dataRowCount = mipHeight; dataRowSize = mipWidth * 2 * sizeof( unsigned char ); break; }
						case VK_FORMAT_R8G8B8A8_SRGB:				{ dataRowCount = mipHeight; dataRowSize = mipWidth * 4 * sizeof( unsigned char ); break; }

						case VK_FORMAT_R16_SFLOAT:					{ dataRowCount = mipHeight; dataRowSize = mipWidth * 1 * sizeof( unsigned short ); break; }
						case VK_FORMAT_R16G16_SFLOAT:				{ dataRowCount = mipHeight; dataRowSize = mipWidth * 2 * sizeof( unsigned short ); break; }
						case VK_FORMAT_R16G16B16A16_SFLOAT:			{ dataRowCount = mipHeight; dataRowSize = mipWidth * 4 * sizeof( unsigned short ); break; }

						case VK_FORMAT_R32_SFLOAT:					{ dataRowCount = mipHeight; dataRowSize = mipWidth * 1 * sizeof( float ); break; }
						case VK_FORMAT_R32G32_SFLOAT:				{ dataRowCount = mipHeight; dataRowSize = mipWidth * 2 * sizeof( float ); break; }
						case VK_FORMAT_R32G32B32A32_SFLOAT:			{ dataRowCount = mipHeight; dataRowSize = mipWidth * 4 * sizeof( float ); break; }

						case VK_FORMAT_BC1_RGB_UNORM_BLOCK:			{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_BC2_UNORM_BLOCK:				{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 16; compressed = true; break; }
						case VK_FORMAT_BC3_UNORM_BLOCK:				{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 16; compressed = true; break; }

						case VK_FORMAT_BC1_RGB_SRGB_BLOCK:			{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:			{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_BC2_SRGB_BLOCK:				{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 16; compressed = true; break; }
						case VK_FORMAT_BC3_SRGB_BLOCK:				{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 16; compressed = true; break; }

						case VK_FORMAT_BC4_UNORM_BLOCK:			
						case VK_FORMAT_BC5_UNORM_BLOCK:				{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 8; compressed = true; break; }

						case VK_FORMAT_BC4_SNORM_BLOCK:			
						case VK_FORMAT_BC5_SNORM_BLOCK:				{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 16; compressed = true; break; }

						case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:	{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:	{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 16; compressed = true; break; }

						case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:		{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:	{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:	{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 16; compressed = true; break; }

						case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+4)/5) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+4)/5); dataRowSize = ((mipWidth+4)/5) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+4)/5); dataRowSize = ((mipWidth+5)/6) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+5)/6); dataRowSize = ((mipWidth+5)/6) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+4)/5); dataRowSize = ((mipWidth+7)/8) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+5)/6); dataRowSize = ((mipWidth+7)/8) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+7)/8); dataRowSize = ((mipWidth+7)/8) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+4)/5); dataRowSize = ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+5)/6); dataRowSize = ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+7)/8); dataRowSize = ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+9)/10); dataRowSize = ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+9)/10); dataRowSize = ((mipWidth+11)/12) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:		{ dataRowCount = ((mipHeight+11)/12); dataRowSize = ((mipWidth+11)/12) * 16; compressed = true; break; }

						case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:			{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+3)/4) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:			{ dataRowCount = ((mipHeight+3)/4); dataRowSize = ((mipWidth+4)/5) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:			{ dataRowCount = ((mipHeight+4)/5); dataRowSize = ((mipWidth+4)/5) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:			{ dataRowCount = ((mipHeight+4)/5); dataRowSize = ((mipWidth+5)/6) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:			{ dataRowCount = ((mipHeight+5)/6); dataRowSize = ((mipWidth+5)/6) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:			{ dataRowCount = ((mipHeight+4)/5); dataRowSize = ((mipWidth+7)/8) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:			{ dataRowCount = ((mipHeight+5)/6); dataRowSize = ((mipWidth+7)/8) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:			{ dataRowCount = ((mipHeight+7)/8); dataRowSize = ((mipWidth+7)/8) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:		{ dataRowCount = ((mipHeight+4)/5); dataRowSize = ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:		{ dataRowCount = ((mipHeight+5)/6); dataRowSize = ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:		{ dataRowCount = ((mipHeight+7)/8); dataRowSize = ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:		{ dataRowCount = ((mipHeight+9)/10); dataRowSize = ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:		{ dataRowCount = ((mipHeight+9)/10); dataRowSize = ((mipWidth+11)/12) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:		{ dataRowCount = ((mipHeight+11)/12); dataRowSize = ((mipWidth+11)/12) * 16; compressed = true; break; }

						default: Error( "%s: Unsupported texture format %d", fileName, format );
					}

					// Copy the source data into the linear image.
					VkImageSubresource subres;
					subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					subres.mipLevel = 0;
					subres.arrayLayer = 0;

					VkSubresourceLayout layout;
					VC( context->device->vkGetImageSubresourceLayout( context->device->device, linearImage[imageIndex], &subres, &layout ) );

					assert( (VkDeviceSize)( dataRowCount * dataRowSize ) <= layout.size );
					assert( (size_t)( dataOffset + dataRowCount * dataRowSize ) <= dataSize );

					void * mapped;
					VK( context->device->vkMapMemory( context->device->device, linearMemory[imageIndex], 0, memoryRequirements.size, 0, &mapped ) );

					const size_t copyBytes = (size_t)( ( (VkDeviceSize) dataRowSize < layout.rowPitch ) ? (VkDeviceSize) dataRowSize : layout.rowPitch );
					for ( int y = 0; y < dataRowCount; y++ )
					{
						memcpy( (char *)mapped + layout.offset + y * layout.rowPitch,
									(char *)data + dataOffset + y * dataRowSize, copyBytes );
					}

					VC( context->device->vkUnmapMemory( context->device->device, linearMemory[imageIndex] ) );

					// Make sure the CPU writes to the linear image are flushed and set optimal image layout for transfer source.
					{
						VkImageMemoryBarrier imageMemoryBarrier;
						imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						imageMemoryBarrier.pNext = NULL;
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
						imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						imageMemoryBarrier.image = linearImage[imageIndex];
						imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
						imageMemoryBarrier.subresourceRange.levelCount = 1;
						imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
						imageMemoryBarrier.subresourceRange.layerCount = 1;

						const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
						const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
						const VkDependencyFlags flags = 0;

						VC( context->device->vkCmdPipelineBarrier( context->setupCommandBuffer, src_stages, dst_stages, flags, 0, NULL, 0, NULL, 1, &imageMemoryBarrier ) );
					}

					// Copy from the linear image to the tiled image.
					{
						VkImageCopy imageCopy;
						imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						imageCopy.srcSubresource.mipLevel = 0;
						imageCopy.srcSubresource.baseArrayLayer = 0;
						imageCopy.srcSubresource.layerCount = 1;
						imageCopy.srcOffset.x = 0;
						imageCopy.srcOffset.y = 0;
						imageCopy.srcOffset.z = 0;
						imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						imageCopy.dstSubresource.mipLevel = 0;
						imageCopy.dstSubresource.baseArrayLayer = layerIndex;
						imageCopy.dstSubresource.layerCount = 1;
						imageCopy.dstOffset.x = 0;
						imageCopy.dstOffset.y = 0;
						imageCopy.dstOffset.z = depthIndex;
						imageCopy.extent.width = mipWidth;
						imageCopy.extent.height = mipHeight;
						imageCopy.extent.depth = 1;

						VC( context->device->vkCmdCopyImage( context->setupCommandBuffer,
										linearImage[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
										texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
										1, &imageCopy ) );
					}

					dataOffset += dataRowCount * dataRowSize;
					imageIndex++;
				}
			}
		}

		if ( numberOfMipmapLevels < 1 )
		{
			// Can ony generate mip levels for uncompressed textures.
			assert( compressed == false );
			UNUSED_PARM( compressed );

			// Generate mip levels for the tiled image in place.
			for ( int mipLevel = 1; mipLevel <= numStorageLevels; mipLevel++ )
			{
				const int prevMipLevel = mipLevel - 1;

				// Make sure any copies to the image are flushed and set optimal image layout for transfer source.
				{
					VkImageMemoryBarrier imageMemoryBarrier;
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.pNext = NULL;
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					imageMemoryBarrier.image = texture->image;
					imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageMemoryBarrier.subresourceRange.baseMipLevel = prevMipLevel;
					imageMemoryBarrier.subresourceRange.levelCount = 1;
					imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
					imageMemoryBarrier.subresourceRange.layerCount = arrayLayerCount;

					const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					const VkDependencyFlags flags = 0;

					VC( context->device->vkCmdPipelineBarrier( context->setupCommandBuffer, src_stages, dst_stages, flags, 0, NULL, 0, NULL, 1, &imageMemoryBarrier ) );
				}

				// Blit from the previous mip level to the current mip level.
				if ( mipLevel < numStorageLevels )
				{
					VkImageBlit imageBlit;
					imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlit.srcSubresource.mipLevel = prevMipLevel;
					imageBlit.srcSubresource.baseArrayLayer = 0;
					imageBlit.srcSubresource.layerCount = arrayLayerCount;
					imageBlit.srcOffsets[0].x = 0;
					imageBlit.srcOffsets[0].y = 0;
					imageBlit.srcOffsets[0].z = 0;
					imageBlit.srcOffsets[1].x = ( width >> prevMipLevel ) >= 1 ? ( width >> prevMipLevel ) - 1 : 0;
					imageBlit.srcOffsets[1].y = ( height >> prevMipLevel ) >= 1 ? ( height >> prevMipLevel ) - 1 : 0;
					imageBlit.srcOffsets[1].z = ( depth >> prevMipLevel ) >= 1 ? ( depth >> prevMipLevel ) - 1 : 0;
					imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlit.dstSubresource.mipLevel = mipLevel;
					imageBlit.dstSubresource.baseArrayLayer = 0;
					imageBlit.dstSubresource.layerCount = arrayLayerCount;
					imageBlit.dstOffsets[0].x = 0;
					imageBlit.dstOffsets[0].y = 0;
					imageBlit.dstOffsets[0].z = 0;
					imageBlit.dstOffsets[1].x = ( width >> mipLevel ) >= 1 ? ( width >> mipLevel ) - 1 : 0;
					imageBlit.dstOffsets[1].y = ( height >> mipLevel ) >= 1 ? ( height >> mipLevel ) - 1 : 0;
					imageBlit.dstOffsets[1].z = ( depth >> mipLevel ) >= 1 ? ( depth >> mipLevel ) - 1 : 0;

					VC( context->device->vkCmdBlitImage( context->setupCommandBuffer,
									texture->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
									texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
									1, &imageBlit, VK_FILTER_LINEAR ) );
				}
			}
		}

		// Make sure any copies or blits to the image are flushed and set optimal image layout for shader read access.
		{
			VkImageMemoryBarrier imageMemoryBarrier;
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.pNext = NULL;
			imageMemoryBarrier.srcAccessMask = ( numberOfMipmapLevels >= 1 ) ? VK_ACCESS_TRANSFER_WRITE_BIT : VK_ACCESS_TRANSFER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
			imageMemoryBarrier.oldLayout = ( numberOfMipmapLevels >= 1 ) ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.image = texture->image;
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
			imageMemoryBarrier.subresourceRange.levelCount = numStorageLevels;
			imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
			imageMemoryBarrier.subresourceRange.layerCount = arrayLayerCount;

			const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			const VkDependencyFlags flags = 0;

			VC( context->device->vkCmdPipelineBarrier( context->setupCommandBuffer, src_stages, dst_stages, flags, 0, NULL, 0, NULL, 1, &imageMemoryBarrier ) );
		}

		GpuContext_FlushSetupCmdBuffer( context );

		// Free the linear images.
		for ( int i = 0; i < imageIndex; i++ )
		{
			VC( context->device->vkDestroyImage( context->device->device, linearImage[i], VK_ALLOCATOR ) );
			VC( context->device->vkFreeMemory( context->device->device, linearMemory[i], VK_ALLOCATOR ) );
		}

		free( linearImage );
		free( linearMemory );
	}

	texture->usage = GPU_TEXTURE_USAGE_SAMPLED;
	texture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	const VkImageViewType viewType =	( ( depth > 1 ) ? VK_IMAGE_VIEW_TYPE_3D :
										( ( numberOfFaces > 1 ) ?
										( ( numberOfArrayElements > 1 ) ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE ) :
										( ( numberOfArrayElements > 1 ) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D ) ) );

	VkImageViewCreateInfo imageViewCreateInfo;
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = NULL;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.image = texture->image;
	imageViewCreateInfo.viewType = viewType;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = numStorageLevels;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = arrayLayerCount;

	VK( context->device->vkCreateImageView( context->device->device, &imageViewCreateInfo, VK_ALLOCATOR, &texture->view ) );

	GpuTexture_UpdateSampler( context, texture );

	return true;
}

static bool GpuTexture_Create2D( GpuContext_t * context, GpuTexture_t * texture,
								const GpuTextureFormat_t format, const int width, const int height,
								const int numberOfMipmapLevels,
								const void * data, const size_t dataSize )
{
	const int depth = 1;
	const int numberOfArrayElements = 1;
	const int numberOfFaces = 1;
	return GpuTexture_CreateInternal( context, texture, "data", (VkFormat)format, width, height, depth,
										numberOfArrayElements, numberOfFaces, numberOfMipmapLevels,
										data, dataSize, false );
}

static bool GpuTexture_Create2DArray( GpuContext_t * context, GpuTexture_t * texture,
								const GpuTextureFormat_t format, const int width, const int height,
								const int numberOfArrayElements, const int numberOfMipmapLevels,
								const void * data, const size_t dataSize )
{
	const int depth = 1;
	const int numberOfFaces = 1;
	return GpuTexture_CreateInternal( context, texture, "data", (VkFormat)format, width, height, depth,
										numberOfArrayElements, numberOfFaces, numberOfMipmapLevels,
										data, dataSize, false );
}

static bool GpuTexture_CreateDefault( GpuContext_t * context, GpuTexture_t * texture, const GpuTextureDefault_t defaultType,
										const int width, const int height, const int depth,
										const int numberOfArrayElements, const int numberOfFaces,
										const bool mipmaps, const bool border )
{
	const int TEXEL_SIZE = 4;
	const int layerSize = width * height * TEXEL_SIZE;
	const int dataSize = depth * numberOfArrayElements * numberOfFaces * layerSize;
	unsigned char * data = (unsigned char *) malloc( dataSize );

	if ( defaultType == GPU_TEXTURE_DEFAULT_CHECKERBOARD )
	{
		const int sp = 4;
		for ( int layer = 0; layer < depth * numberOfArrayElements * numberOfFaces; layer++ )
		{
			for ( int y = 0; y < height; y++ )
			{
				for ( int x = 0; x < width; x++ )
				{
					if ( ( ( ( x >> sp ) ^ ( y >> sp ) ) & 1 ) == 0 )
					{
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 0] = ( layer & 1 ) == 0 ? 96 : 160;
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 1] = 64;
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 2] = ( layer & 1 ) == 0 ? 255 : 96;
					}
					else
					{
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 0] = ( layer & 1 ) == 0 ? 64 : 160;
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 1] = 32;
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 2] = ( layer & 1 ) == 0 ? 255 : 64;
					}
					data[layer * layerSize + ( y * 128 + x ) * TEXEL_SIZE + 3] = 255;
				}
			}
		}
	}
	else if ( defaultType == GPU_TEXTURE_DEFAULT_CIRCLES )
	{
		const unsigned char colors[4][4] =
		{
			{ 0xFF, 0x00, 0x00, 0xFF },
			{ 0x00, 0xFF, 0x00, 0xFF },
			{ 0x00, 0x00, 0xFF, 0xFF },
			{ 0xFF, 0xFF, 0x00, 0xFF }
		};
		for ( int layer = 0; layer < depth * numberOfArrayElements * numberOfFaces; layer++ )
		{
			for ( int y = 0; y < height; y++ )
			{
				for ( int x = 0; x < width; x++ )
				{
					// Pick a color per 32x32 block of texels.
					const int index =	( ( ( y >> 4 ) & 2 ) ^ ( ( x >> 5 ) & 2 ) ) |
										( ( ( x >> 5 ) & 1 ) ^ ( ( y >> 6 ) & 1 ) );

					// Draw a circle with radius 10 centered inside each 32x32 block of texels.
					const int dX = ( x & ~31 ) + 16 - x;
					const int dY = ( y & ~31 ) + 16 - y;
					const int dS = abs( dX * dX + dY * dY - 10 * 10 );
					const int scale = ( dS <= 32 ) ? dS : 32;

					for ( int c = 0; c < TEXEL_SIZE - 1; c++ )
					{
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + c] = (unsigned char)( ( colors[index][c] * scale ) >> 5 );
					}
					data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + TEXEL_SIZE - 1] = 255;
				}
			}
		}
	}
	else if ( defaultType == GPU_TEXTURE_DEFAULT_PYRAMIDS )
	{
		const int sp = 4;
		for ( int layer = 0; layer < depth * numberOfArrayElements * numberOfFaces; layer++ )
		{
			for ( int y = 0; y < height; y++ )
			{
				for ( int x = 0; x < width; x++ )
				{
					const int mask = ( 1 << sp ) - 1;
					const int lx = x & mask;
					const int ly = y & mask;
					const int rx = mask - lx;
					const int ry = mask - ly;

					char cx = 0;
					char cy = 0;
					if ( lx != ly && lx != ry )
					{
						int m = 1 << sp;
						if ( lx < m ) { m = lx; cx = -96; cy =   0; }
						if ( ly < m ) { m = ly; cx =   0; cy = -96; }
						if ( rx < m ) { m = rx; cx = +96; cy =   0; }
						if ( ry < m ) { m = ry; cx =   0; cy = +96; }
					}
					data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 0] = 128 + cx;
					data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 1] = 128 + cy;
					data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 2] = 128 + 85;
					data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 3] = 255;
				}
			}
		}
	}

	if ( border )
	{
		for ( int layer = 0; layer < depth * numberOfArrayElements * numberOfFaces; layer++ )
		{
			for ( int x = 0; x < width; x++ )
			{
				data[layer * layerSize + ( 0 * width + x ) * TEXEL_SIZE + 0] = 0;
				data[layer * layerSize + ( 0 * width + x ) * TEXEL_SIZE + 1] = 0;
				data[layer * layerSize + ( 0 * width + x ) * TEXEL_SIZE + 2] = 0;
				data[layer * layerSize + ( 0 * width + x ) * TEXEL_SIZE + 3] = 255;

				data[layer * layerSize + ( ( height - 1 ) * width + x ) * TEXEL_SIZE + 0] = 0;
				data[layer * layerSize + ( ( height - 1 ) * width + x ) * TEXEL_SIZE + 1] = 0;
				data[layer * layerSize + ( ( height - 1 ) * width + x ) * TEXEL_SIZE + 2] = 0;
				data[layer * layerSize + ( ( height - 1 ) * width + x ) * TEXEL_SIZE + 3] = 255;
			}
			for ( int y = 0; y < height; y++ )
			{
				data[layer * layerSize + ( y * width + 0 ) * TEXEL_SIZE + 0] = 0;
				data[layer * layerSize + ( y * width + 0 ) * TEXEL_SIZE + 1] = 0;
				data[layer * layerSize + ( y * width + 0 ) * TEXEL_SIZE + 2] = 0;
				data[layer * layerSize + ( y * width + 0 ) * TEXEL_SIZE + 3] = 255;

				data[layer * layerSize + ( y * width + width - 1 ) * TEXEL_SIZE + 0] = 0;
				data[layer * layerSize + ( y * width + width - 1 ) * TEXEL_SIZE + 1] = 0;
				data[layer * layerSize + ( y * width + width - 1 ) * TEXEL_SIZE + 2] = 0;
				data[layer * layerSize + ( y * width + width - 1 ) * TEXEL_SIZE + 3] = 255;
			}
		}
	}

	const int numberOfMipmapLevels = ( mipmaps ) ? -1 : 1;
	bool success = GpuTexture_CreateInternal( context, texture, "data", VK_FORMAT_R8G8B8A8_UNORM, width, height, depth,
												numberOfArrayElements, numberOfFaces, numberOfMipmapLevels,
												data, dataSize, false );

	free( data );

	return success;
}

static bool GpuTexture_CreateFromSwapChain( GpuContext_t * context, GpuTexture_t * texture, const GpuWindow_t * window, int index )
{
	assert( index >= 0 && index < (int)window->swapchain.imageCount );

	texture->width = window->swapchain.width;
	texture->height = window->swapchain.height;
	texture->depth = 1;
	texture->layerCount = 1;
	texture->mipCount = 1;
	texture->usage = GPU_TEXTURE_USAGE_UNDEFINED;
	texture->wrapMode = GPU_TEXTURE_WRAP_MODE_REPEAT;
	texture->filter = GPU_TEXTURE_FILTER_LINEAR;
	texture->maxAnisotropy = 1.0f;
	texture->format = window->swapchain.internalFormat;
	texture->imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	texture->image = window->swapchain.images[index];
	texture->memory = VK_NULL_HANDLE;
	texture->view = window->swapchain.views[index];
	texture->sampler = VK_NULL_HANDLE;
	GpuTexture_UpdateSampler( context, texture );

	return true;
}

static void GpuTexture_Destroy( GpuContext_t * context, GpuTexture_t * texture )
{
	VC( context->device->vkDestroySampler( context->device->device, texture->sampler, VK_ALLOCATOR ) );
	// A texture created from a swapchain does not own the view, image or memory.
	if ( texture->memory != VK_NULL_HANDLE )
	{
		VC( context->device->vkDestroyImageView( context->device->device, texture->view, VK_ALLOCATOR ) );
		VC( context->device->vkDestroyImage( context->device->device, texture->image, VK_ALLOCATOR ) );
		VC( context->device->vkFreeMemory( context->device->device, texture->memory, VK_ALLOCATOR ) );
	}
	memset( texture, 0, sizeof( GpuTexture_t ) );
}

static void GpuTexture_SetWrapMode( GpuContext_t * context, GpuTexture_t * texture, const GpuTextureWrapMode_t wrapMode )
{
	texture->wrapMode = wrapMode;

	GpuTexture_UpdateSampler( context, texture );
}

static void GpuTexture_SetFilter( GpuContext_t * context, GpuTexture_t * texture, const GpuTextureFilter_t filter )
{
	texture->filter = filter;

	GpuTexture_UpdateSampler( context, texture );
}

static void GpuTexture_SetAniso( GpuContext_t * context, GpuTexture_t * texture, const float maxAniso )
{
	texture->maxAnisotropy = maxAniso;

	GpuTexture_UpdateSampler( context, texture );
}

/*
================================================================================================================================

GPU geometry.

For optimal performance, geometry should only be created at load time, not at runtime.
The vertex, index and instance buffers are placed in device memory for optimal performance.
The vertex attributes are not packed. Each attribute is stored in a separate array for
optimal binning on tiling GPUs that only transform the vertex position for the binning pass.
Storing each attribute in a saparate array is preferred even on immediate-mode GPUs to avoid
wasting cache space for attributes that are not used by a particular vertex shader.

GpuVertexAttributeFlags_t
GpuVertexAttributeArrays_t
GpuTriangleIndex_t
GpuGeometry_t

static void GpuVertexAttributeArrays_Alloc( GpuVertexAttributeArrays_t * attribs, const int numVertices, const int attribsFlags );
static void GpuVertexAttributeArrays_Free( GpuVertexAttributeArrays_t * attribs );
static void GpuVertexAttributeArrays_CalculateTangents( GpuVertexAttributeArrays_t * attribs, const int numVertices,
														const GpuTriangleIndex_t * indices, const int numIndices );

static void GpuGeometry_Create( GpuContext_t * context, GpuGeometry_t * geometry,
								const GpuVertexAttributeArrays_t * attribs, const int numVertices,
								const GpuTriangleIndex_t * indices, const int numIndices );
static void GpuGeometry_CreateQuad( GpuContext_t * context, GpuGeometry_t * geometry, const float offset, const float scale );
static void GpuGeometry_CreateCube( GpuContext_t * context, GpuGeometry_t * geometry, const float offset, const float scale );
static void GpuGeometry_CreateTorus( GpuContext_t * context, GpuGeometry_t * geometry, const int tesselation, const float offset, const float scale );
static void GpuGeometry_Destroy( GpuContext_t * context, GpuGeometry_t * geometry );

static void GpuGeometry_AddInstanceAttributes( GpuContext_t * context, GpuGeometry_t * geometry, const int numInstances, const int instanceAttribsFlags );

================================================================================================================================
*/

typedef struct
{
	int				attributeFlag;		// VERTEX_ATTRIBUTE_FLAG_
	size_t			attributeOffset;	// Offset in bytes to the pointer in GpuVertexAttributeArrays_t
	size_t			attributeSize;		// Size in bytes of a single attribute
	int				attributeFormat;	// VkFormat of the attribute
	int				locationCount;		// Number of attribute locations
} GpuVertexAttribute_t;

typedef enum
{
	VERTEX_ATTRIBUTE_FLAG_POSITION	= BIT( 0 ),		// vec3 vertexPosition
	VERTEX_ATTRIBUTE_FLAG_NORMAL	= BIT( 1 ),		// vec3 vertexNormal
	VERTEX_ATTRIBUTE_FLAG_TANGENT	= BIT( 2 ),		// vec3 vertexTangent
	VERTEX_ATTRIBUTE_FLAG_BINORMAL	= BIT( 3 ),		// vec3 vertexBinormal
	VERTEX_ATTRIBUTE_FLAG_COLOR		= BIT( 4 ),		// vec4 vertexColor
	VERTEX_ATTRIBUTE_FLAG_UV0		= BIT( 5 ),		// vec2 vertexUv0
	VERTEX_ATTRIBUTE_FLAG_UV1		= BIT( 6 ),		// vec2 vertexUv1
	VERTEX_ATTRIBUTE_FLAG_UV2		= BIT( 7 ),		// vec2 vertexUv2
	VERTEX_ATTRIBUTE_FLAG_TRANSFORM	= BIT( 8 )		// mat4 vertexTransform (NOTE this mat4 takes up 4 attribute locations)
} GpuVertexAttributeFlags_t;

typedef struct
{
	Vector3f_t *	position;
	Vector3f_t *	normal;
	Vector3f_t *	tangent;
	Vector3f_t *	binormal;
	Vector4f_t *	color;
	Vector2f_t *	uv0;
	Vector2f_t *	uv1;
	Vector2f_t *	uv2;
	Matrix4x4f_t *	transform;
} GpuVertexAttributeArrays_t;

static const GpuVertexAttribute_t VertexAttributeLayout[] =
{
	{ VERTEX_ATTRIBUTE_FLAG_POSITION,	OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, position ),	SIZEOF_MEMBER( GpuVertexAttributeArrays_t, position[0] ),	VK_FORMAT_R32G32B32_SFLOAT,		1 },
	{ VERTEX_ATTRIBUTE_FLAG_NORMAL,		OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, normal ),		SIZEOF_MEMBER( GpuVertexAttributeArrays_t, normal[0] ),		VK_FORMAT_R32G32B32_SFLOAT,		1 },
	{ VERTEX_ATTRIBUTE_FLAG_TANGENT,	OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, tangent ),		SIZEOF_MEMBER( GpuVertexAttributeArrays_t, tangent[0] ),	VK_FORMAT_R32G32B32_SFLOAT,		1 },
	{ VERTEX_ATTRIBUTE_FLAG_BINORMAL,	OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, binormal ),	SIZEOF_MEMBER( GpuVertexAttributeArrays_t, binormal[0] ),	VK_FORMAT_R32G32B32_SFLOAT,		1 },
	{ VERTEX_ATTRIBUTE_FLAG_COLOR,		OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, color ),		SIZEOF_MEMBER( GpuVertexAttributeArrays_t, color[0] ),		VK_FORMAT_R32G32B32A32_SFLOAT,	1 },
	{ VERTEX_ATTRIBUTE_FLAG_UV0,		OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, uv0 ),			SIZEOF_MEMBER( GpuVertexAttributeArrays_t, uv0[0] ),		VK_FORMAT_R32G32_SFLOAT,		1 },
	{ VERTEX_ATTRIBUTE_FLAG_UV1,		OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, uv1 ),			SIZEOF_MEMBER( GpuVertexAttributeArrays_t, uv1[0] ),		VK_FORMAT_R32G32_SFLOAT,		1 },
	{ VERTEX_ATTRIBUTE_FLAG_UV2,		OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, uv2 ),			SIZEOF_MEMBER( GpuVertexAttributeArrays_t, uv2[0] ),		VK_FORMAT_R32G32_SFLOAT,		1 },
	{ VERTEX_ATTRIBUTE_FLAG_TRANSFORM,	OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, transform ),	SIZEOF_MEMBER( GpuVertexAttributeArrays_t, transform[0] ),	VK_FORMAT_R32G32B32A32_SFLOAT,	4 }
};

typedef unsigned int GpuTriangleIndex_t;

typedef struct
{
	int			vertexCount;
	int			instanceCount;
	int 		indexCount;
	int			vertexAttribsFlags;
	int			instanceAttribsFlags;
	GpuBuffer_t	vertexBuffer;
	GpuBuffer_t	instanceBuffer;
	GpuBuffer_t	indexBuffer;
} GpuGeometry_t;

static size_t GpuVertexAttributeArrays_GetDataSize( const int numVertices, const int attribsFlags )
{
	size_t totalSize = 0;
	for ( int i = 0; i < (int)( ARRAY_SIZE( VertexAttributeLayout ) ); i++ )
	{
		const GpuVertexAttribute_t * v = &VertexAttributeLayout[i];
		if ( ( v->attributeFlag & attribsFlags ) != 0 )
		{
			totalSize += v->attributeSize;
		}
	}
	return numVertices * totalSize;
}

static void * GpuVertexAttributeArrays_GetDataPointer( const GpuVertexAttributeArrays_t * attribs )
{
	for ( int i = 0; i < (int)( ARRAY_SIZE( VertexAttributeLayout ) ); i++ )
	{
		const GpuVertexAttribute_t * v = &VertexAttributeLayout[i];
		void * attribPtr = *(void **) ( ((char *)attribs) + v->attributeOffset );
		if ( attribPtr != NULL )
		{
			return attribPtr;
		}
	}
	return NULL;
}

static int GpuVertexAttributeArrays_GetAttribsFlags( const GpuVertexAttributeArrays_t * attribs )
{
	int attribsFlags = 0;
	for ( int i = 0; i < (int)( ARRAY_SIZE( VertexAttributeLayout ) ); i++ )
	{
		const GpuVertexAttribute_t * v = &VertexAttributeLayout[i];
		void * attribPtr = *(void **) ( ((char *)attribs) + v->attributeOffset );
		if ( attribPtr != NULL )
		{
			attribsFlags |= v->attributeFlag;
		}
	}
	return attribsFlags;
}

static void GpuVertexAttributeArrays_Map( GpuVertexAttributeArrays_t * attribs, void * data, const size_t dataSize, const int numVertices, const int attribsFlags )
{
	memset( attribs, 0, sizeof( GpuVertexAttributeArrays_t ) );

	unsigned char * dataBytePtr = (unsigned char *) data;
	size_t offset = 0;

	for ( int i = 0; i < (int)( ARRAY_SIZE( VertexAttributeLayout ) ); i++ )
	{
		const GpuVertexAttribute_t * v = &VertexAttributeLayout[i];
		if ( ( v->attributeFlag & attribsFlags ) != 0 )
		{
			void ** attribPtr = (void **) ( ((char *)attribs) + v->attributeOffset );
			*attribPtr = ( dataBytePtr + offset );
			offset += numVertices * v->attributeSize;
		}
	}

	assert( offset == dataSize );
	UNUSED_PARM( dataSize );
}

static void GpuVertexAttributeArrays_Alloc( GpuVertexAttributeArrays_t * attribs, const int numVertices, const int attribsFlags )
{
	const size_t dataSize = GpuVertexAttributeArrays_GetDataSize( numVertices, attribsFlags );
	void * data = malloc( dataSize );

	GpuVertexAttributeArrays_Map( attribs, data, dataSize, numVertices, attribsFlags );
}

static void GpuVertexAttributeArrays_Free( GpuVertexAttributeArrays_t * attribs )
{
	void * data = GpuVertexAttributeArrays_GetDataPointer( attribs );
	free( data );
	memset( attribs, 0, sizeof( GpuVertexAttributeArrays_t ) );
}

static void GpuVertexAttributeArrays_CalculateTangents( GpuVertexAttributeArrays_t * attribs, const int numVertices,
												const GpuTriangleIndex_t * indices, const int numIndices )
{
	assert( attribs->position != NULL );
	assert( attribs->normal != NULL );
	assert( attribs->tangent != NULL );
	assert( attribs->binormal != NULL );
	assert( attribs->uv0 != NULL );

	for ( int i = 0; i < numVertices; i++ )
	{
		Vector3f_Zero( &attribs->tangent[i] );
		Vector3f_Zero( &attribs->binormal[i] );
	}

	for ( int i = 0; i < numIndices; i += 3 )
	{
		const GpuTriangleIndex_t * v = indices + i;
		const Vector3f_t * pos = attribs->position;
		const Vector2f_t * uv0 = attribs->uv0;

		const Vector3f_t delta0 = { pos[v[1]].x - pos[v[0]].x, pos[v[1]].y - pos[v[0]].y, pos[v[1]].z - pos[v[0]].z };
		const Vector3f_t delta1 = { pos[v[2]].x - pos[v[1]].x, pos[v[2]].y - pos[v[1]].y, pos[v[2]].z - pos[v[1]].z };
		const Vector3f_t delta2 = { pos[v[0]].x - pos[v[2]].x, pos[v[0]].y - pos[v[2]].y, pos[v[0]].z - pos[v[2]].z };

		const float l0 = delta0.x * delta0.x + delta0.y * delta0.y + delta0.z * delta0.z;
		const float l1 = delta1.x * delta1.x + delta1.y * delta1.y + delta1.z * delta1.z;
		const float l2 = delta2.x * delta2.x + delta2.y * delta2.y + delta2.z * delta2.z;

		const int i0 = ( l0 > l1 ) ? ( l0 > l2 ? 2 : 1 ) : ( l1 > l2 ? 0 : 1 );
		const int i1 = ( i0 + 1 ) % 3;
		const int i2 = ( i0 + 2 ) % 3;

		const Vector3f_t d0 = { pos[v[i1]].x - pos[v[i0]].x, pos[v[i1]].y - pos[v[i0]].y, pos[v[i1]].z - pos[v[i0]].z };
		const Vector3f_t d1 = { pos[v[i2]].x - pos[v[i0]].x, pos[v[i2]].y - pos[v[i0]].y, pos[v[i2]].z - pos[v[i0]].z };

		const Vector2f_t s0 = { uv0[v[i1]].x - uv0[v[i0]].x, uv0[v[i1]].y - uv0[v[i0]].y };
		const Vector2f_t s1 = { uv0[v[i2]].x - uv0[v[i0]].x, uv0[v[i2]].y - uv0[v[i0]].y };

		const float sign = ( s0.x * s1.y - s0.y * s1.x ) < 0.0f ? -1.0f : 1.0f;

		Vector3f_t tangent  = { ( d0.x * s1.y - d1.x * s0.y ) * sign, ( d0.y * s1.y - d1.y * s0.y ) * sign, ( d0.z * s1.y - d1.z * s0.y ) * sign };
		Vector3f_t binormal = { ( d1.x * s0.x - d0.x * s1.x ) * sign, ( d1.y * s0.x - d0.y * s1.x ) * sign, ( d1.z * s0.x - d0.z * s1.x ) * sign };

		Vector3f_Normalize( &tangent );
		Vector3f_Normalize( &binormal );

		for ( int j = 0; j < 3; j++ )
		{
			attribs->tangent[v[j]].x += tangent.x;
			attribs->tangent[v[j]].y += tangent.y;
			attribs->tangent[v[j]].z += tangent.z;

			attribs->binormal[v[j]].x += binormal.x;
			attribs->binormal[v[j]].y += binormal.y;
			attribs->binormal[v[j]].z += binormal.z;
		}
	}

	for ( int i = 0; i < numVertices; i++ )
	{
		Vector3f_Normalize( &attribs->tangent[i] );
		Vector3f_Normalize( &attribs->binormal[i] );
	}
}

static void GpuGeometry_Create( GpuContext_t * context, GpuGeometry_t * geometry,
								const GpuVertexAttributeArrays_t * attribs, const int numVertices,
								const GpuTriangleIndex_t * indices, const int numIndices )
{
	memset( geometry, 0, sizeof( GpuGeometry_t ) );

	geometry->vertexCount = numVertices;
	geometry->indexCount = numIndices;
	geometry->vertexAttribsFlags = GpuVertexAttributeArrays_GetAttribsFlags( attribs );

	const void * data = GpuVertexAttributeArrays_GetDataPointer( attribs );
	const size_t dataSize = GpuVertexAttributeArrays_GetDataSize( geometry->vertexCount, geometry->vertexAttribsFlags );

	GpuBuffer_Create( context, &geometry->vertexBuffer, GPU_BUFFER_TYPE_VERTEX, dataSize, data, false );
	GpuBuffer_Create( context, &geometry->indexBuffer, GPU_BUFFER_TYPE_INDEX, numIndices * sizeof( indices[0] ), indices, false );
}

// The quad is centered about the origin and without offset/scale spans the [-1, 1] X-Y range.
static void GpuGeometry_CreateQuad( GpuContext_t * context, GpuGeometry_t * geometry, const float offset, const float scale )
{
	Vector3f_t quadPositions[4] =
	{
		{ -1.0f, -1.0f, 0.0f }, { +1.0f, -1.0f, 0.0f }, { +1.0f, +1.0f, 0.0f }, { -1.0f, +1.0f, 0.0f }
	};

	Vector3f_t quadNormals[4] =
	{
		{ 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }
	};

	Vector2f_t quadUvs[4] =
	{
		{ 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f },	{ 0.0f, 0.0f }
	};

	GpuTriangleIndex_t quadIndices[6] =
	{
		 0,  1,  2,  2,  3,  0
	};

	GpuVertexAttributeArrays_t quadAttribs;
	GpuVertexAttributeArrays_Alloc( &quadAttribs, 4,
									VERTEX_ATTRIBUTE_FLAG_POSITION |
									VERTEX_ATTRIBUTE_FLAG_NORMAL |
									VERTEX_ATTRIBUTE_FLAG_TANGENT |
									VERTEX_ATTRIBUTE_FLAG_BINORMAL |
									VERTEX_ATTRIBUTE_FLAG_UV0 );

	for ( int i = 0; i < 4; i++ )
	{
		quadAttribs.position[i].x = ( quadPositions[i].x + offset ) * scale;
		quadAttribs.position[i].y = ( quadPositions[i].y + offset ) * scale;
		quadAttribs.position[i].z = ( quadPositions[i].z + offset ) * scale;
		quadAttribs.normal[i].x = quadNormals[i].x;
		quadAttribs.normal[i].y = quadNormals[i].y;
		quadAttribs.normal[i].z = quadNormals[i].z;
		quadAttribs.uv0[i].x = quadUvs[i].x;
		quadAttribs.uv0[i].y = quadUvs[i].y;
	}

	GpuVertexAttributeArrays_CalculateTangents( &quadAttribs, 4, quadIndices, 6 );

	GpuGeometry_Create( context, geometry, &quadAttribs, 4, quadIndices, 6 );

	GpuVertexAttributeArrays_Free( &quadAttribs );
}

// The cube is centered about the origin and without offset/scale spans the [-1, 1] X-Y-Z range.
static void GpuGeometry_CreateCube( GpuContext_t * context, GpuGeometry_t * geometry, const float offset, const float scale )
{
	Vector3f_t cubePositions[24] =
	{
		{ +1.0f, -1.0f, -1.0f }, { +1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, +1.0f }, { +1.0f, -1.0f, +1.0f },
		{ -1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, +1.0f }, { -1.0f, +1.0f, +1.0f }, { -1.0f, +1.0f, -1.0f },

		{ -1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, +1.0f }, { -1.0f, +1.0f, +1.0f },
		{ -1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, +1.0f }, { +1.0f, -1.0f, +1.0f }, { +1.0f, -1.0f, -1.0f },

		{ -1.0f, -1.0f, +1.0f }, { +1.0f, -1.0f, +1.0f }, { +1.0f, +1.0f, +1.0f }, { -1.0f, +1.0f, +1.0f },
		{ -1.0f, -1.0f, -1.0f }, { -1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, -1.0f }, { +1.0f, -1.0f, -1.0f }
	};

	Vector3f_t cubeNormals[24] =
	{
		{ +1.0f, 0.0f, 0.0f }, { +1.0f, 0.0f, 0.0f }, { +1.0f, 0.0f, 0.0f }, { +1.0f, 0.0f, 0.0f },
		{ -1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f },

		{ 0.0f, +1.0f, 0.0f }, { 0.0f, +1.0f, 0.0f }, { 0.0f, +1.0f, 0.0f }, { 0.0f, +1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f },

		{ 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f },
		{ 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }
	};

	Vector2f_t cubeUvs[24] =
	{
		{ 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f },
		{ 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f },	{ 0.0f, 1.0f },

		{ 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f },	{ 0.0f, 0.0f },
		{ 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f },	{ 0.0f, 1.0f },

		{ 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f },	{ 0.0f, 0.0f },
		{ 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f },	{ 0.0f, 1.0f },
	};

	GpuTriangleIndex_t cubeIndices[36] =
	{
		 0,  1,  2,  2,  3,  0,
		 4,  5,  6,  6,  7,  4,
		 8, 10,  9, 10,  8, 11,
		12, 14, 13, 14, 12, 15,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20
	};

	GpuVertexAttributeArrays_t cubeAttribs;
	GpuVertexAttributeArrays_Alloc( &cubeAttribs, 24,
									VERTEX_ATTRIBUTE_FLAG_POSITION |
									VERTEX_ATTRIBUTE_FLAG_NORMAL |
									VERTEX_ATTRIBUTE_FLAG_TANGENT |
									VERTEX_ATTRIBUTE_FLAG_BINORMAL |
									VERTEX_ATTRIBUTE_FLAG_UV0 );

	for ( int i = 0; i < 24; i++ )
	{
		cubeAttribs.position[i].x = ( cubePositions[i].x + offset ) * scale;
		cubeAttribs.position[i].y = ( cubePositions[i].y + offset ) * scale;
		cubeAttribs.position[i].z = ( cubePositions[i].z + offset ) * scale;
		cubeAttribs.normal[i].x = cubeNormals[i].x;
		cubeAttribs.normal[i].y = cubeNormals[i].y;
		cubeAttribs.normal[i].z = cubeNormals[i].z;
		cubeAttribs.uv0[i].x = cubeUvs[i].x;
		cubeAttribs.uv0[i].y = cubeUvs[i].y;
	}

	GpuVertexAttributeArrays_CalculateTangents( &cubeAttribs, 24, cubeIndices, 36 );

	GpuGeometry_Create( context, geometry, &cubeAttribs, 24, cubeIndices, 36 );

	GpuVertexAttributeArrays_Free( &cubeAttribs );
}

// The torus is centered about the origin and without offset/scale spans the [-1, 1] X-Y range and the [-0.3, 0.3] Z range.
static void GpuGeometry_CreateTorus( GpuContext_t * context, GpuGeometry_t * geometry, const int tesselation, const float offset, const float scale )
{
	const int minorTesselation = tesselation;
	const int majorTesselation = tesselation;
	const float tubeRadius = 0.3f;
	const float tubeCenter = 0.7f;
	const int numVertices = ( majorTesselation + 1 ) * ( minorTesselation + 1 );
	const int numIndices = majorTesselation * minorTesselation * 6;

	GpuVertexAttributeArrays_t torusAttribs;
	GpuVertexAttributeArrays_Alloc( &torusAttribs, numVertices,
									VERTEX_ATTRIBUTE_FLAG_POSITION |
									VERTEX_ATTRIBUTE_FLAG_NORMAL |
									VERTEX_ATTRIBUTE_FLAG_TANGENT |
									VERTEX_ATTRIBUTE_FLAG_BINORMAL |
									VERTEX_ATTRIBUTE_FLAG_UV0 );

	GpuTriangleIndex_t * torusIndices = (GpuTriangleIndex_t *) malloc( numIndices * sizeof( torusIndices[0] ) );

	for ( int u = 0; u <= majorTesselation; u++ )
	{
		const float ua = 2.0f * MATH_PI * u / majorTesselation;
		const float majorCos = cosf( ua );
		const float majorSin = sinf( ua );

		for ( int v = 0; v <= minorTesselation; v++ )
		{
			const float va = MATH_PI + 2.0f * MATH_PI * v / minorTesselation;
			const float minorCos = cosf( va );
			const float minorSin = sinf( va );

			const float minorX = tubeCenter + tubeRadius * minorCos;
			const float minorZ = tubeRadius * minorSin;

			const int index = u * ( minorTesselation + 1 ) + v;
			torusAttribs.position[index].x = ( minorX * majorCos * scale ) + offset;
			torusAttribs.position[index].y = ( minorX * majorSin * scale ) + offset;
			torusAttribs.position[index].z = ( minorZ * scale ) + offset;
			torusAttribs.normal[index].x = minorCos * majorCos;
			torusAttribs.normal[index].y = minorCos * majorSin;
			torusAttribs.normal[index].z = minorSin;
			torusAttribs.uv0[index].x = (float) u / majorTesselation;
			torusAttribs.uv0[index].y = (float) v / minorTesselation;
		}
	}

	for ( int u = 0; u < majorTesselation; u++ )
	{
		for ( int v = 0; v < minorTesselation; v++ )
		{
			const int index = ( u * minorTesselation + v ) * 6;
			torusIndices[index + 0] = ( u + 0 ) * ( minorTesselation + 1 ) + ( v + 0 );
			torusIndices[index + 1] = ( u + 1 ) * ( minorTesselation + 1 ) + ( v + 0 );
			torusIndices[index + 2] = ( u + 1 ) * ( minorTesselation + 1 ) + ( v + 1 );
			torusIndices[index + 3] = ( u + 1 ) * ( minorTesselation + 1 ) + ( v + 1 );
			torusIndices[index + 4] = ( u + 0 ) * ( minorTesselation + 1 ) + ( v + 1 );
			torusIndices[index + 5] = ( u + 0 ) * ( minorTesselation + 1 ) + ( v + 0 );
		}
	}

	GpuVertexAttributeArrays_CalculateTangents( &torusAttribs, numVertices, torusIndices, numIndices );

	GpuGeometry_Create( context, geometry, &torusAttribs, numVertices, torusIndices, numIndices );

	GpuVertexAttributeArrays_Free( &torusAttribs );
	free( torusIndices );
}

static void GpuGeometry_Destroy( GpuContext_t * context, GpuGeometry_t * geometry )
{
	GpuBuffer_Destroy( context, &geometry->indexBuffer );
	GpuBuffer_Destroy( context, &geometry->vertexBuffer );
	if ( geometry->instanceBuffer.size != 0 )
	{
		GpuBuffer_Destroy( context, &geometry->instanceBuffer );
	}

	memset( geometry, 0, sizeof( GpuGeometry_t ) );
}

static void GpuGeometry_AddInstanceAttributes( GpuContext_t * context, GpuGeometry_t * geometry, const int numInstances, const int instanceAttribsFlags )
{
	assert( ( geometry->vertexAttribsFlags & instanceAttribsFlags ) == 0 );

	geometry->instanceCount = numInstances;
	geometry->instanceAttribsFlags = instanceAttribsFlags;

	const size_t dataSize = GpuVertexAttributeArrays_GetDataSize( numInstances, geometry->instanceAttribsFlags );

	GpuBuffer_Create( context, &geometry->instanceBuffer, GPU_BUFFER_TYPE_VERTEX, dataSize, NULL, false );
}

/*
================================================================================================================================

GPU render pass.

A render pass encapsulates a sequence of graphics commands that can be executed in a single tiling pass.
For optimal performance, a render pass should only be created at load time, not at runtime.
Render passes cannot overlap and cannot be nested.

GpuRenderPassType_t
GpuRenderPassFlags_t
GpuRenderPass_t

static bool GpuRenderPass_Create( GpuContext_t * context, GpuRenderPass_t * renderPass,
									const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
									const GpuRenderPassType_t type, const uint32_t flags );
static void GpuRenderPass_Destroy( GpuContext_t * context, GpuRenderPass_t * renderPass );

================================================================================================================================
*/

typedef enum
{
	GPU_RENDERPASS_TYPE_INLINE,
	GPU_RENDERPASS_TYPE_SECONDARY_COMMAND_BUFFERS
} GpuRenderPassType_t;

typedef enum
{
	GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER		= BIT( 0 ),
	GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER		= BIT( 1 )
} GpuRenderPassFlags_t;

typedef struct
{
	GpuRenderPassType_t		type;
	int						flags;
	GpuSurfaceColorFormat_t	colorFormat;
	GpuSurfaceDepthFormat_t	depthFormat;
	VkFormat				internalColorFormat;
	VkFormat				internalDepthFormat;
	VkRenderPass			renderPass;
} GpuRenderPass_t;

static bool GpuRenderPass_Create( GpuContext_t * context, GpuRenderPass_t * renderPass,
									const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
									const GpuRenderPassType_t type, const int flags )
{
	renderPass->type = type;
	renderPass->flags = flags;
	renderPass->colorFormat = colorFormat;
	renderPass->depthFormat = depthFormat;
	renderPass->internalColorFormat = GpuSwapchain_InternalSurfaceColorFormat( colorFormat );
	renderPass->internalDepthFormat = GpuDepthBuffer_InternalSurfaceDepthFormat( depthFormat );

	VkAttachmentDescription attachments[2];

	attachments[0].flags = 0;
	attachments[0].format = renderPass->internalColorFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = ( flags & GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER ) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachments[1].flags = 0;
	attachments[1].format = renderPass->internalDepthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = ( flags & GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER ) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentReference;
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference;
	depthAttachmentReference.attachment = ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED ) ? 1 : VK_ATTACHMENT_UNUSED;
	depthAttachmentReference.layout = ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED ) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;

	VkSubpassDescription subpassDescription;
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = NULL;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;
	subpassDescription.pResolveAttachments = NULL;
	subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = NULL;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED ) ? 2 : 1;
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 0;
	renderPassCreateInfo.pDependencies = NULL;

	VK( context->device->vkCreateRenderPass( context->device->device, &renderPassCreateInfo, VK_ALLOCATOR, &renderPass->renderPass ) );

	return true;
}

static void GpuRenderPass_Destroy( GpuContext_t * context, GpuRenderPass_t * renderPass )
{
	VC( context->device->vkDestroyRenderPass( context->device->device, renderPass->renderPass, VK_ALLOCATOR ) );
}

/*
================================================================================================================================

GPU framebuffer.

A framebuffer encapsulates either a swapchain or a buffered set of textures.
For optimal performance, a framebuffer should only be created at load time, not at runtime.

GpuFramebuffer_t

static bool GpuFramebuffer_CreateFromSwapchain( GpuWindow_t * window, GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass );
static bool GpuFramebuffer_CreateFromTextures( GpuContext_t * context, GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass,
												const int width, const int height, const int numBuffers );
static bool GpuFramebuffer_CreateFromTextureArrays( GpuContext_t * context, GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass,
												const int width, const int height, const int numLayers, const int numBuffers, const bool multiview );
static void GpuFramebuffer_Destroy( GpuContext_t * context, GpuFramebuffer_t * framebuffer );

static int GpuFramebuffer_GetWidth( const GpuFramebuffer_t * framebuffer );
static int GpuFramebuffer_GetHeight( const GpuFramebuffer_t * framebuffer );
static ScreenRect_t GpuFramebuffer_GetRect( const GpuFramebuffer_t * framebuffer );
static int GpuFramebuffer_GetBufferCount( const GpuFramebuffer_t * framebuffer );
static GpuTexture_t * GpuFramebuffer_GetColorTexture( const GpuFramebuffer_t * framebuffer );

================================================================================================================================
*/

typedef struct
{
	GpuTexture_t *		colorTextures;
	GpuDepthBuffer_t	depthBuffer;
	VkImageView *		views;
	VkFramebuffer *		framebuffers;
	GpuRenderPass_t *	renderPass;
	GpuWindow_t *		window;
	int					swapchainCreateCount;
	int					width;
	int					height;
	int					numLayers;
	int					numBuffers;
	int					currentBuffer;
	int					currentLayer;
} GpuFramebuffer_t;

static bool GpuFramebuffer_CreateFromSwapchain( GpuWindow_t * window, GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass )
{
	assert( window->windowWidth >= 1 && window->windowWidth <= (int)window->context.device->physicalDeviceProperties.limits.maxFramebufferWidth );
	assert( window->windowHeight >= 1 && window->windowHeight <= (int)window->context.device->physicalDeviceProperties.limits.maxFramebufferHeight );

	memset( framebuffer, 0, sizeof( GpuFramebuffer_t ) );

	framebuffer->renderPass = renderPass;
	framebuffer->window = window;
	framebuffer->swapchainCreateCount = window->swapchainCreateCount;
	framebuffer->width = window->windowWidth;
	framebuffer->height = window->windowHeight;
	framebuffer->numLayers = 1;
	framebuffer->numBuffers = 3;	// Default to 3 for late swapchain creation on Android.
	framebuffer->currentBuffer = 0;
	framebuffer->currentLayer = 0;

	if ( window->swapchain.swapchain == VK_NULL_HANDLE )
	{
		return false;
	}

	assert( renderPass->internalColorFormat == window->swapchain.internalFormat );
	assert( renderPass->internalDepthFormat == window->depthBuffer.internalFormat );
	assert( framebuffer->numBuffers >= (int)window->swapchain.imageCount );

	framebuffer->colorTextures = (GpuTexture_t *) malloc( window->swapchain.imageCount * sizeof( GpuTexture_t ) );
	framebuffer->views = NULL;
	framebuffer->framebuffers = (VkFramebuffer *) malloc( window->swapchain.imageCount * sizeof( VkFramebuffer ) );
	framebuffer->numBuffers = window->swapchain.imageCount;

	for ( uint32_t imageIndex = 0; imageIndex < window->swapchain.imageCount; imageIndex++ )
	{
		assert( renderPass->colorFormat == window->colorFormat );
		assert( renderPass->depthFormat == window->depthFormat );

		GpuTexture_CreateFromSwapChain( &window->context, &framebuffer->colorTextures[imageIndex], window, imageIndex );

		assert( window->windowWidth == framebuffer->colorTextures[imageIndex].width );
		assert( window->windowHeight == framebuffer->colorTextures[imageIndex].height );

		VkImageView attachments[2];
		attachments[0] = window->swapchain.views[imageIndex];
		attachments[1] = ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED ) ? window->depthBuffer.views[0] : VK_NULL_HANDLE;

		VkFramebufferCreateInfo framebufferCreateInfo;
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.pNext = NULL;
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.renderPass = renderPass->renderPass;
		framebufferCreateInfo.attachmentCount = ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED ) ? 2 : 1;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = window->windowWidth;
		framebufferCreateInfo.height = window->windowHeight;
		framebufferCreateInfo.layers = 1;

		VK( window->context.device->vkCreateFramebuffer( window->context.device->device, &framebufferCreateInfo, VK_ALLOCATOR, &framebuffer->framebuffers[imageIndex] ) );
	}

	return true;
}

static bool GpuFramebuffer_CreateFromTextures( GpuContext_t * context, GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass,
												const int width, const int height, const int numBuffers )
{
	assert( width >= 1 && width <= (int)context->device->physicalDeviceProperties.limits.maxFramebufferWidth );
	assert( height >= 1 && height <= (int)context->device->physicalDeviceProperties.limits.maxFramebufferHeight );

	memset( framebuffer, 0, sizeof( GpuFramebuffer_t ) );

	framebuffer->colorTextures = (GpuTexture_t *) malloc( numBuffers * sizeof( GpuTexture_t ) );
	framebuffer->views = NULL;
	framebuffer->framebuffers = (VkFramebuffer *) malloc( numBuffers * sizeof( VkFramebuffer ) );
	framebuffer->renderPass = renderPass;
	framebuffer->window = NULL;
	framebuffer->swapchainCreateCount = 0;
	framebuffer->width = width;
	framebuffer->height = height;
	framebuffer->numLayers = 1;
	framebuffer->numBuffers = numBuffers;
	framebuffer->currentBuffer = 0;
	framebuffer->currentLayer = 0;

	if ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED )
	{
		GpuDepthBuffer_Create( context, &framebuffer->depthBuffer, renderPass->depthFormat, width, height, 1 );
	}

	for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
	{
		GpuTexture_Create2D( context, &framebuffer->colorTextures[bufferIndex], (GpuTextureFormat_t)renderPass->internalColorFormat, width, height, 1, NULL, 0 );
		GpuTexture_SetWrapMode( context, &framebuffer->colorTextures[bufferIndex], GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER );

		VkImageView attachments[2];
		attachments[0] = framebuffer->colorTextures[bufferIndex].view;
		attachments[1] = ( renderPass->depthFormat != GPU_SURFACE_DEPTH_FORMAT_NONE ) ? framebuffer->depthBuffer.views[0] : VK_NULL_HANDLE;

		VkFramebufferCreateInfo framebufferCreateInfo;
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.pNext = NULL;
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.renderPass = renderPass->renderPass;
		framebufferCreateInfo.attachmentCount = ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED ) ? 2 : 1;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = width;
		framebufferCreateInfo.height = height;
		framebufferCreateInfo.layers = 1;

		VK( context->device->vkCreateFramebuffer( context->device->device, &framebufferCreateInfo, VK_ALLOCATOR, &framebuffer->framebuffers[bufferIndex] ) );
	}

	return true;
}

static bool GpuFramebuffer_CreateFromTextureArrays( GpuContext_t * context, GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass,
												const int width, const int height, const int numLayers, const int numBuffers, const bool multiview )
{
	UNUSED_PARM( multiview );

	assert( width >= 1 && width <= (int)context->device->physicalDeviceProperties.limits.maxFramebufferWidth );
	assert( height >= 1 && height <= (int)context->device->physicalDeviceProperties.limits.maxFramebufferHeight );
	assert( numLayers >= 1 && numLayers <= (int)context->device->physicalDeviceProperties.limits.maxFramebufferLayers );

	memset( framebuffer, 0, sizeof( GpuFramebuffer_t ) );

	framebuffer->colorTextures = (GpuTexture_t *) malloc( numBuffers * sizeof( GpuTexture_t ) );
	framebuffer->views = (VkImageView *) malloc( numBuffers * numLayers * sizeof( VkImageView ) );
	framebuffer->framebuffers = (VkFramebuffer *) malloc( numBuffers * numLayers * sizeof( VkFramebuffer ) );
	framebuffer->renderPass = renderPass;
	framebuffer->window = NULL;
	framebuffer->swapchainCreateCount = 0;
	framebuffer->width = width;
	framebuffer->height = height;
	framebuffer->numLayers = numLayers;
	framebuffer->numBuffers = numBuffers;
	framebuffer->currentBuffer = 0;
	framebuffer->currentLayer = 0;

	if ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED )
	{
		// Note: share a single depth buffer between all array layers.
		GpuDepthBuffer_Create( context, &framebuffer->depthBuffer, renderPass->depthFormat, width, height, 1 );
	}

	for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
	{
		GpuTexture_Create2DArray( context, &framebuffer->colorTextures[bufferIndex], (GpuTextureFormat_t)renderPass->internalColorFormat, width, height, numLayers, 1, NULL, 0 );
		GpuTexture_SetWrapMode( context, &framebuffer->colorTextures[bufferIndex], GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER );

		for ( int layerIndex = 0; layerIndex < numLayers; layerIndex++ )
		{
			// Create a view for a single array layer.
			VkImageViewCreateInfo imageViewCreateInfo;
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.pNext = NULL;
			imageViewCreateInfo.flags = 0;
			imageViewCreateInfo.image = framebuffer->colorTextures[bufferIndex].image;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = framebuffer->colorTextures[bufferIndex].format;
			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = layerIndex;
			imageViewCreateInfo.subresourceRange.layerCount = 1;

			VK( context->device->vkCreateImageView( context->device->device, &imageViewCreateInfo, VK_ALLOCATOR, &framebuffer->views[bufferIndex * numLayers + layerIndex] ) );

			VkImageView attachments[2];
			attachments[0] = framebuffer->views[bufferIndex * numLayers + layerIndex];
			attachments[1] = ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED ) ? framebuffer->depthBuffer.views[0] : VK_NULL_HANDLE;

			// Create a framebuffer for a single array layer.
			VkFramebufferCreateInfo framebufferCreateInfo;
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.pNext = NULL;
			framebufferCreateInfo.flags = 0;
			framebufferCreateInfo.renderPass = renderPass->renderPass;
			framebufferCreateInfo.attachmentCount = ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED ) ? 2 : 1;
			framebufferCreateInfo.pAttachments = attachments;
			framebufferCreateInfo.width = width;
			framebufferCreateInfo.height = height;
			framebufferCreateInfo.layers = 1;

			VK( context->device->vkCreateFramebuffer( context->device->device, &framebufferCreateInfo, VK_ALLOCATOR, &framebuffer->framebuffers[bufferIndex * numLayers + layerIndex] ) );
		}
	}

	return true;
}

static void GpuFramebuffer_Destroy( GpuContext_t * context, GpuFramebuffer_t * framebuffer )
{
	for ( int bufferIndex = 0; bufferIndex < framebuffer->numBuffers; bufferIndex++ )
	{
		for ( int layerIndex = 0; layerIndex < framebuffer->numLayers; layerIndex++ )
		{
			if ( framebuffer->framebuffers != NULL )
			{
				VC( context->device->vkDestroyFramebuffer( context->device->device, framebuffer->framebuffers[bufferIndex * framebuffer->numLayers + layerIndex], VK_ALLOCATOR ) );
			}
			if ( framebuffer->views != NULL )
			{
				VC( context->device->vkDestroyImageView( context->device->device, framebuffer->views[bufferIndex * framebuffer->numLayers + layerIndex], VK_ALLOCATOR ) );
			}
		}
		if ( framebuffer->colorTextures != NULL )
		{
			GpuTexture_Destroy( context, &framebuffer->colorTextures[bufferIndex] );
		}
	}

	if ( framebuffer->window == NULL )
	{
		GpuDepthBuffer_Destroy( context, &framebuffer->depthBuffer );
	}

	free( framebuffer->framebuffers );
	free( framebuffer->views );
	free( framebuffer->colorTextures );

	memset( framebuffer, 0, sizeof( GpuFramebuffer_t ) );
}

static int GpuFramebuffer_GetWidth( const GpuFramebuffer_t * framebuffer )
{
	return framebuffer->width;
}

static int GpuFramebuffer_GetHeight( const GpuFramebuffer_t * framebuffer )
{
	return framebuffer->height;
}

static ScreenRect_t GpuFramebuffer_GetRect( const GpuFramebuffer_t * framebuffer )
{
	ScreenRect_t rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = framebuffer->width;
	rect.height = framebuffer->height;
	return rect;
}

static int GpuFramebuffer_GetBufferCount( const GpuFramebuffer_t * framebuffer )
{
	return framebuffer->numBuffers;
}

static GpuTexture_t * GpuFramebuffer_GetColorTexture( const GpuFramebuffer_t * framebuffer )
{
	assert( framebuffer->colorTextures != NULL );
	return &framebuffer->colorTextures[framebuffer->currentBuffer];
}

/*
================================================================================================================================

GPU program parms and layout.

GpuProgramStage_t
GpuProgramParmType_t
GpuProgramParmAccess_t
GpuProgramParm_t
GpuProgramParmLayout_t

static void GpuProgramParmLayout_Create( GpuContext_t * context, GpuProgramParmLayout_t * layout,
										const GpuProgramParm_t * parms, const int numParms );
static void GpuProgramParmLayout_Destroy( GpuContext_t * context, GpuProgramParmLayout_t * layout );

================================================================================================================================
*/

#define MAX_PROGRAM_PARMS	16

typedef enum
{
	GPU_PROGRAM_STAGE_VERTEX,
	GPU_PROGRAM_STAGE_FRAGMENT,
	GPU_PROGRAM_STAGE_COMPUTE,
	GPU_PROGRAM_STAGE_MAX
} GpuProgramStage_t;

typedef enum
{
	GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,					// texture plus sampler bound together (GLSL: sampler)
	GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,					// not sampled, direct read-write storage (GLSL: image)
	GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM,					// read-only uniform buffer (GLSL: uniform)
	GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE,					// read-write storage buffer (GLSL: buffer)
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				// int
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,		// int[2]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3,		// int[3]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4,		// int[4]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT,				// float
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,		// float[2]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3,		// float[3]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4,		// float[4]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	// float[4][4]
	GPU_PROGRAM_PARM_TYPE_MAX
} GpuProgramParmType_t;

typedef enum
{
	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,
	GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,
	GPU_PROGRAM_PARM_ACCESS_READ_WRITE
} GpuProgramParmAccess_t;

typedef struct
{
	GpuProgramStage_t			stage;		// vertex, fragment or compute
	GpuProgramParmType_t		type;		// texture, buffer or push constant
	GpuProgramParmAccess_t		access;		// read and/or write
	int							index;		// index into GpuProgramParmState_t::parms
	const char * 				name;		// GLSL name
	int							binding;	// texture/buffer binding, or push constant offset
											// Note that Vulkan bindings must be unique per descriptor set across the whole pipeline.
											// Note that Vulkan push constant ranges must be unique across the whole pipeline.
} GpuProgramParm_t;

typedef struct
{
	int							numParms;
	const GpuProgramParm_t *	parms;
	VkDescriptorSetLayout		descriptorSetLayout;
	VkPipelineLayout			pipelineLayout;
	int							offsetForIndex[MAX_PROGRAM_PARMS];	// push constant offsets into GpuProgramParmState_t::data based on GpuProgramParm_t::index
	const GpuProgramParm_t *	bindings[MAX_PROGRAM_PARMS];		// descriptor bindings
	const GpuProgramParm_t *	pushConstants[MAX_PROGRAM_PARMS];	// push constants
	int							numBindings;
	int							numPushConstants;
	unsigned int				hash;
} GpuProgramParmLayout_t;

static bool GpuProgramParm_IsDescriptor( const GpuProgramParmType_t type )
{
	return	( ( type == GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED ) ?	true :
			( ( type == GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE ) ?	true :
			( ( type == GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM ) ?	true :
			( ( type == GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE ) ?	true :
																	false ) ) ) );
}

static VkDescriptorType GpuProgramParm_GetDescriptorType( const GpuProgramParmType_t type )
{
	return	( ( type == GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED ) ?	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER :
			( ( type == GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE ) ?	VK_DESCRIPTOR_TYPE_STORAGE_IMAGE :
			( ( type == GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM ) ?	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
			( ( type == GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE ) ?	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
																	VK_DESCRIPTOR_TYPE_MAX_ENUM ) ) ) );
}

static int GpuProgramParm_GetPushConstantSize( GpuProgramParmType_t type )
{
	static const int parmSize[] =
	{
		(unsigned int)0,
		(unsigned int)0,
		(unsigned int)0,
		(unsigned int)0,
		(unsigned int)sizeof( int ),
		(unsigned int)sizeof( int[2] ),
		(unsigned int)sizeof( int[3] ),
		(unsigned int)sizeof( int[4] ),
		(unsigned int)sizeof( float ),
		(unsigned int)sizeof( float[2] ),
		(unsigned int)sizeof( float[3] ),
		(unsigned int)sizeof( float[4] ),
		(unsigned int)sizeof( float[4][4] )
	};
	return parmSize[type];
}

static VkShaderStageFlags GpuProgramParm_GetShaderStageFlags( const GpuProgramStage_t stage )
{
	return	( ( stage == GPU_PROGRAM_STAGE_VERTEX ) ?	VK_SHADER_STAGE_VERTEX_BIT :
			( ( stage == GPU_PROGRAM_STAGE_FRAGMENT ) ?	VK_SHADER_STAGE_FRAGMENT_BIT :
			( ( stage == GPU_PROGRAM_STAGE_COMPUTE ) ?	VK_SHADER_STAGE_COMPUTE_BIT :
														0 ) ) );
}

static void GpuProgramParmLayout_Create( GpuContext_t * context, GpuProgramParmLayout_t * layout,
										const GpuProgramParm_t * parms, const int numParms )
{
	memset( layout, 0, sizeof( GpuProgramParmLayout_t ) );

	layout->numParms = numParms;
	layout->parms = parms;

	int numSampledTextureBindings[GPU_PROGRAM_STAGE_MAX] = { 0 };
	int numStorageTextureBindings[GPU_PROGRAM_STAGE_MAX] = { 0 };
	int numUniformBufferBindings[GPU_PROGRAM_STAGE_MAX] = { 0 };
	int numStorageBufferBindings[GPU_PROGRAM_STAGE_MAX] = { 0 };

	int offset = 0;
	memset( layout->offsetForIndex, -1, sizeof( layout->offsetForIndex ) );

	for ( int i = 0; i < numParms; i++ )
	{
		if ( parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED ||
				parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE ||
					parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM ||
						parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
		{
			numSampledTextureBindings[parms[i].stage] += ( parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED );
			numStorageTextureBindings[parms[i].stage] += ( parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE );
			numUniformBufferBindings[parms[i].stage] += ( parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM );
			numStorageBufferBindings[parms[i].stage] += ( parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE );

			assert( parms[i].binding >= 0 && parms[i].binding < MAX_PROGRAM_PARMS );

			// Make sure each binding location is only used once.
			assert( layout->bindings[parms[i].binding] == NULL );

			layout->bindings[parms[i].binding] = &parms[i];
			if ( (int)parms[i].binding >= layout->numBindings )
			{
				layout->numBindings = (int)parms[i].binding + 1;
			}
		}
		else
		{
			assert( layout->numPushConstants < MAX_PROGRAM_PARMS );
			layout->pushConstants[layout->numPushConstants++] = &parms[i];

			layout->offsetForIndex[parms[i].index] = offset;
			offset += GpuProgramParm_GetPushConstantSize( parms[i].type );
		}
	}

	// Make sure the descriptor bindings are packed.
	for ( int binding = 0; binding < layout->numBindings; binding++ )
	{
		assert( layout->bindings[binding] != NULL );
	}

	// Make sure no push constants overlap.
	for ( int push0 = 0; push0 < layout->numPushConstants; push0++ )
	{
		for ( int push1 = push0 + 1; push1 < layout->numPushConstants; push1++ )
		{
			assert( layout->pushConstants[push0]->binding >= layout->pushConstants[push1]->binding + GpuProgramParm_GetPushConstantSize( layout->pushConstants[push1]->type ) ||
					layout->pushConstants[push0]->binding + GpuProgramParm_GetPushConstantSize( layout->pushConstants[push0]->type ) <= layout->pushConstants[push1]->binding );
		}
	}

	// Check the descriptor limits.
	int numTotalSampledTextureBindings = 0;
	int numTotalStorageTextureBindings = 0;
	int numTotalUniformBufferBindings = 0;
	int numTotalStorageBufferBindings = 0;
	for ( int stage = 0; stage < GPU_PROGRAM_STAGE_MAX; stage++ )
	{
		assert( numSampledTextureBindings[stage] <= (int)context->device->physicalDeviceProperties.limits.maxPerStageDescriptorSampledImages );
		assert( numStorageTextureBindings[stage] <= (int)context->device->physicalDeviceProperties.limits.maxPerStageDescriptorStorageImages );
		assert( numUniformBufferBindings[stage] <= (int)context->device->physicalDeviceProperties.limits.maxPerStageDescriptorUniformBuffers );
		assert( numStorageBufferBindings[stage] <= (int)context->device->physicalDeviceProperties.limits.maxPerStageDescriptorStorageBuffers );

		numTotalSampledTextureBindings += numSampledTextureBindings[stage];
		numTotalStorageTextureBindings += numStorageTextureBindings[stage];
		numTotalUniformBufferBindings += numUniformBufferBindings[stage];
		numTotalStorageBufferBindings += numStorageBufferBindings[stage];
	}

	assert( numTotalSampledTextureBindings <= (int)context->device->physicalDeviceProperties.limits.maxDescriptorSetSampledImages );
	assert( numTotalStorageTextureBindings <= (int)context->device->physicalDeviceProperties.limits.maxDescriptorSetStorageImages );
	assert( numTotalUniformBufferBindings <= (int)context->device->physicalDeviceProperties.limits.maxDescriptorSetUniformBuffers );
	assert( numTotalStorageBufferBindings <= (int)context->device->physicalDeviceProperties.limits.maxDescriptorSetStorageBuffers );

	//
	// Create descriptor set layout and pipeline layout
	//

	{
		VkDescriptorSetLayoutBinding descriptorSetBindings[MAX_PROGRAM_PARMS];
		VkPushConstantRange pushConstantRanges[MAX_PROGRAM_PARMS];

		int numDescriptorSetBindings = 0;
		int numPushConstantRanges = 0;
		for ( int i = 0; i < numParms; i++ )
		{
			if ( GpuProgramParm_IsDescriptor( parms[i].type ) )
			{
				descriptorSetBindings[numDescriptorSetBindings].binding = parms[i].binding;
				descriptorSetBindings[numDescriptorSetBindings].descriptorType = GpuProgramParm_GetDescriptorType( parms[i].type );
				descriptorSetBindings[numDescriptorSetBindings].descriptorCount = 1;
				descriptorSetBindings[numDescriptorSetBindings].stageFlags = GpuProgramParm_GetShaderStageFlags( parms[i].stage );
				descriptorSetBindings[numDescriptorSetBindings].pImmutableSamplers = NULL;
				numDescriptorSetBindings++;
			}
			else // push constant
			{
				pushConstantRanges[numPushConstantRanges].stageFlags = GpuProgramParm_GetShaderStageFlags( parms[i].stage );
				pushConstantRanges[numPushConstantRanges].offset = parms[i].binding;
				pushConstantRanges[numPushConstantRanges].size = GpuProgramParm_GetPushConstantSize( parms[i].type );
				numPushConstantRanges++;
			}
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = NULL;
		descriptorSetLayoutCreateInfo.flags = 0;
		descriptorSetLayoutCreateInfo.bindingCount = numDescriptorSetBindings;
		descriptorSetLayoutCreateInfo.pBindings = ( numDescriptorSetBindings != 0 ) ? descriptorSetBindings : NULL;

		VK( context->device->vkCreateDescriptorSetLayout( context->device->device, &descriptorSetLayoutCreateInfo, VK_ALLOCATOR, &layout->descriptorSetLayout ) );

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext = NULL;
		pipelineLayoutCreateInfo.flags = 0;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &layout->descriptorSetLayout;
		pipelineLayoutCreateInfo.pushConstantRangeCount = numPushConstantRanges;
		pipelineLayoutCreateInfo.pPushConstantRanges = ( numPushConstantRanges != 0 ) ? pushConstantRanges : NULL;

		VK( context->device->vkCreatePipelineLayout( context->device->device, &pipelineLayoutCreateInfo, VK_ALLOCATOR, &layout->pipelineLayout ) );
	}

	// Calculate a hash of the layout.
	unsigned int hash = 5381;
	for ( int i = 0; i < numParms * (int)sizeof( parms[0] ); i++ )
	{
		hash = ( ( hash << 5 ) - hash ) + ((const char *)parms)[i];
	}
	layout->hash = hash;
}

static void GpuProgramParmLayout_Destroy( GpuContext_t * context, GpuProgramParmLayout_t * layout )
{
	VC( context->device->vkDestroyPipelineLayout( context->device->device, layout->pipelineLayout, VK_ALLOCATOR ) );
	VC( context->device->vkDestroyDescriptorSetLayout( context->device->device, layout->descriptorSetLayout, VK_ALLOCATOR ) );
}

/*
================================================================================================================================

GPU graphics program.

A graphics program encapsulates a vertex and fragment program that are used to render geometry.
For optimal performance, a graphics program should only be created at load time, not at runtime.

GpuGraphicsProgram_t

static bool GpuGraphicsProgram_Create( GpuContext_t * context, GpuGraphicsProgram_t * program,
										const void * vertexSourceData, const size_t vertexSourceSize,
										const void * fragmentSourceData, const size_t fragmentSourceSize,
										const GpuProgramParm_t * parms, const int numParms,
										const int vertexAttribsFlags );
static void GpuGraphicsProgram_Destroy( GpuContext_t * context, GpuGraphicsProgram_t * program );

================================================================================================================================
*/

typedef struct
{
	VkShaderModule						vertexShaderModule;
	VkShaderModule						fragmentShaderModule;
	VkPipelineShaderStageCreateInfo		pipelineStages[2];
	GpuProgramParmLayout_t				parmLayout;
	int									vertexAttribsFlags;
} GpuGraphicsProgram_t;

static bool GpuGraphicsProgram_Create( GpuContext_t * context, GpuGraphicsProgram_t * program,
										const void * vertexSourceData, const size_t vertexSourceSize,
										const void * fragmentSourceData, const size_t fragmentSourceSize,
										const GpuProgramParm_t * parms, const int numParms,
										const int vertexAttribsFlags )
{
	program->vertexAttribsFlags = vertexAttribsFlags;

	GpuDevice_CreateShader( context->device, &program->vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT, vertexSourceData, vertexSourceSize );
	GpuDevice_CreateShader( context->device, &program->fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, fragmentSourceData, fragmentSourceSize );

	program->pipelineStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	program->pipelineStages[0].pNext = NULL;
	program->pipelineStages[0].flags = 0;
	program->pipelineStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	program->pipelineStages[0].module = program->vertexShaderModule;
	program->pipelineStages[0].pName = "main";
	program->pipelineStages[0].pSpecializationInfo = NULL;

	program->pipelineStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	program->pipelineStages[1].pNext = NULL;
	program->pipelineStages[1].flags = 0;
	program->pipelineStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	program->pipelineStages[1].module = program->fragmentShaderModule;
	program->pipelineStages[1].pName = "main";
	program->pipelineStages[1].pSpecializationInfo = NULL;

	GpuProgramParmLayout_Create( context, &program->parmLayout, parms, numParms );

	return true;
}

static void GpuGraphicsProgram_Destroy( GpuContext_t * context, GpuGraphicsProgram_t * program )
{
	GpuProgramParmLayout_Destroy( context, &program->parmLayout );

	VC( context->device->vkDestroyShaderModule( context->device->device, program->vertexShaderModule, VK_ALLOCATOR ) );
	VC( context->device->vkDestroyShaderModule( context->device->device, program->fragmentShaderModule, VK_ALLOCATOR ) );
}

/*
================================================================================================================================

GPU compute program.

For optimal performance, a compute program should only be created at load time, not at runtime.

GpuComputeProgram_t

static bool GpuComputeProgram_Create( GpuContext_t * context, GpuComputeProgram_t * program,
									const void * computeSourceData, const size_t computeSourceSize,
									const GpuProgramParm_t * parms, const int numParms );
static void GpuComputeProgram_Destroy( GpuContext_t * context, GpuComputeProgram_t * program );

================================================================================================================================
*/

typedef struct
{
	VkShaderModule						computeShaderModule;
	VkPipelineShaderStageCreateInfo		pipelineStage;
	GpuProgramParmLayout_t				parmLayout;
} GpuComputeProgram_t;

static bool GpuComputeProgram_Create( GpuContext_t * context, GpuComputeProgram_t * program,
									const void * computeSourceData, const size_t computeSourceSize,
									const GpuProgramParm_t * parms, const int numParms )
{
	GpuDevice_CreateShader( context->device, &program->computeShaderModule, VK_SHADER_STAGE_COMPUTE_BIT, computeSourceData, computeSourceSize );

	program->pipelineStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	program->pipelineStage.pNext = NULL;
	program->pipelineStage.flags = 0;
	program->pipelineStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	program->pipelineStage.module = program->computeShaderModule;
	program->pipelineStage.pName = "main";
	program->pipelineStage.pSpecializationInfo = NULL;

	GpuProgramParmLayout_Create( context, &program->parmLayout, parms, numParms );

	return true;
}

static void GpuComputeProgram_Destroy( GpuContext_t * context, GpuComputeProgram_t * program )
{
	GpuProgramParmLayout_Destroy( context, &program->parmLayout );

	VC( context->device->vkDestroyShaderModule( context->device->device, program->computeShaderModule, VK_ALLOCATOR ) );
}

/*
================================================================================================================================

GPU graphics pipeline.

A graphics pipeline encapsulates the geometry, program and ROP state that is used to render.
For optimal performance, a graphics pipeline should only be created at load time, not at runtime.
The vertex attribute locations are assigned here, when both the geometry and program are known,
to avoid binding vertex attributes that are not used by the vertex shader, and to avoid binding
to a discontinuous set of vertex attribute locations.

GpuBlendFactor_t
GpuBlendOp_t
GpuCompareOp_t
GpuRasterOperations_t
GpuGraphicsPipelineParms_t
GpuGraphicsPipeline_t

static bool GpuGraphicsPipeline_Create( GpuContext_t * context, GpuGraphicsPipeline_t * pipeline, const GpuGraphicsPipelineParms_t * parms );
static void GpuGraphicsPipeline_Destroy( GpuContext_t * context, GpuGraphicsPipeline_t * pipeline );

================================================================================================================================
*/

typedef enum
{
	GPU_BLEND_FACTOR_ZERO					= VK_BLEND_FACTOR_ZERO,
	GPU_BLEND_FACTOR_ONE					= VK_BLEND_FACTOR_ONE,
	GPU_BLEND_FACTOR_SRC_COLOR				= VK_BLEND_FACTOR_SRC_COLOR,
	GPU_BLEND_FACTOR_ONE_MINUS_SRC_COLOR	= VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	GPU_BLEND_FACTOR_DST_COLOR				= VK_BLEND_FACTOR_DST_COLOR,
	GPU_BLEND_FACTOR_ONE_MINUS_DST_COLOR	= VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
	GPU_BLEND_FACTOR_SRC_ALPHA				= VK_BLEND_FACTOR_SRC_ALPHA,
	GPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA	= VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	GPU_BLEND_FACTOR_DST_ALPHA				= VK_BLEND_FACTOR_DST_ALPHA,
	GPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA	= VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA
} GpuBlendFactor_t;

typedef enum
{
	GPU_BLEND_OP_ADD						= VK_BLEND_OP_ADD,
	GPU_BLEND_OP_SUBTRACT					= VK_BLEND_OP_SUBTRACT,
	GPU_BLEND_OP_REVERSE_SUBTRACT			= VK_BLEND_OP_REVERSE_SUBTRACT,
	GPU_BLEND_OP_MIN						= VK_BLEND_OP_MIN,
	GPU_BLEND_OP_MAX						= VK_BLEND_OP_MAX
} GpuBlendOp_t;

typedef enum
{
	GPU_COMPARE_OP_NEVER					= VK_COMPARE_OP_NEVER,
	GPU_COMPARE_OP_LESS						= VK_COMPARE_OP_LESS,
	GPU_COMPARE_OP_EQUAL					= VK_COMPARE_OP_EQUAL,
	GPU_COMPARE_OP_LESS_OR_EQUAL			= VK_COMPARE_OP_LESS_OR_EQUAL,
	GPU_COMPARE_OP_GREATER					= VK_COMPARE_OP_GREATER,
	GPU_COMPARE_OP_NOT_EQUAL				= VK_COMPARE_OP_NOT_EQUAL,
	GPU_COMPARE_OP_GREATER_OR_EQUAL			= VK_COMPARE_OP_GREATER_OR_EQUAL,
	GPU_COMPARE_OP_ALWAYS					= VK_COMPARE_OP_ALWAYS
} GpuCompareOp_t;

typedef struct
{
	bool							blendEnable;
	bool							alphaWriteEnable;
	bool							depthTestEnable;
	bool							depthWriteEnable;
	GpuBlendFactor_t				blendSrcColor;
	GpuBlendFactor_t				blendDstColor;
	GpuBlendOp_t					blendOpColor;
	GpuBlendFactor_t				blendSrcAlpha;
	GpuBlendFactor_t				blendDstAlpha;
	GpuBlendOp_t					blendOpAlpha;
	GpuCompareOp_t					depthCompare;
} GpuRasterOperations_t;

typedef struct
{
	GpuRasterOperations_t			rop;
	const GpuRenderPass_t *			renderPass;
	const GpuGraphicsProgram_t *	program;
	const GpuGeometry_t *			geometry;
} GpuGraphicsPipelineParms_t;

#define MAX_VERTEX_ATTRIBUTES		16

typedef struct
{
	GpuRasterOperations_t					rop;
	const GpuGraphicsProgram_t *			program;
	const GpuGeometry_t *					geometry;
	int										vertexAttributeCount;
	int										vertexBindingCount;
	int										firstInstanceBinding;
	VkVertexInputAttributeDescription		vertexAttributes[MAX_VERTEX_ATTRIBUTES];
	VkVertexInputBindingDescription			vertexBindings[MAX_VERTEX_ATTRIBUTES];
	VkDeviceSize							vertexBindingOffsets[MAX_VERTEX_ATTRIBUTES];
	VkPipelineVertexInputStateCreateInfo	vertexInputState;
	VkPipelineInputAssemblyStateCreateInfo	inputAssemblyState;
	VkPipeline								pipeline;
} GpuGraphicsPipeline_t;

static void GpuGraphicsPipelineParms_Init( GpuGraphicsPipelineParms_t * parms )
{
	parms->rop.blendEnable = false;
	parms->rop.alphaWriteEnable = false;
	parms->rop.depthTestEnable = true;
	parms->rop.depthWriteEnable = true;
	parms->rop.blendSrcColor = GPU_BLEND_FACTOR_ONE;
	parms->rop.blendDstColor = GPU_BLEND_FACTOR_ZERO;
	parms->rop.blendOpColor = GPU_BLEND_OP_ADD;
	parms->rop.blendSrcAlpha = GPU_BLEND_FACTOR_ONE;
	parms->rop.blendDstAlpha = GPU_BLEND_FACTOR_ZERO;
	parms->rop.blendOpAlpha = GPU_BLEND_OP_ADD;
	parms->rop.depthCompare = GPU_COMPARE_OP_LESS_OR_EQUAL;
	parms->renderPass = NULL;
	parms->program = NULL;
	parms->geometry = NULL;
}

static void InitVertexAttributes( const bool instance, const int numAttribs,
								const int storedAttribsFlags, const int usedAttribsFlags,
								VkVertexInputAttributeDescription * attributes, int * attributeCount,
								VkVertexInputBindingDescription * bindings, int * bindingCount,
								VkDeviceSize * bindingOffsets )
{
	size_t offset = 0;
	for ( int i = 0; i < (int)( ARRAY_SIZE( VertexAttributeLayout ) ); i++ )
	{
		const GpuVertexAttribute_t * v = &VertexAttributeLayout[i];
		if ( ( v->attributeFlag & storedAttribsFlags ) != 0 )
		{
			if ( ( v->attributeFlag & usedAttribsFlags ) != 0 )
			{
				for ( int i = 0; i < v->locationCount; i++ )
				{
					attributes[*attributeCount + i].location = *attributeCount + i;
					attributes[*attributeCount + i].binding = *bindingCount;
					attributes[*attributeCount + i].format = v->attributeFormat;
					attributes[*attributeCount + i].offset = (uint32_t)( i * v->attributeSize / v->locationCount );	// limited offset used for packed vertex data
				}

				bindings[*bindingCount].binding = *bindingCount;
				bindings[*bindingCount].stride = (uint32_t) v->attributeSize;
				bindings[*bindingCount].inputRate = instance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;

				bindingOffsets[*bindingCount] = (VkDeviceSize) offset;	// memory offset within vertex buffer

				*attributeCount += v->locationCount;
				*bindingCount += 1;
			}
			offset += numAttribs * v->attributeSize;
		}
	}
}

static bool GpuGraphicsPipeline_Create( GpuContext_t * context, GpuGraphicsPipeline_t * pipeline, const GpuGraphicsPipelineParms_t * parms )
{
	pipeline->rop = parms->rop;
	pipeline->program = parms->program;
	pipeline->geometry = parms->geometry;
	pipeline->vertexAttributeCount = 0;
	pipeline->vertexBindingCount = 0;

	InitVertexAttributes( false, parms->geometry->vertexCount, parms->geometry->vertexAttribsFlags,
							parms->program->vertexAttribsFlags,
							pipeline->vertexAttributes, &pipeline->vertexAttributeCount,
							pipeline->vertexBindings, &pipeline->vertexBindingCount,
							pipeline->vertexBindingOffsets );

	pipeline->firstInstanceBinding = pipeline->vertexBindingCount;

	InitVertexAttributes( true, parms->geometry->instanceCount, parms->geometry->instanceAttribsFlags,
							parms->program->vertexAttribsFlags,
							pipeline->vertexAttributes, &pipeline->vertexAttributeCount,
							pipeline->vertexBindings, &pipeline->vertexBindingCount,
							pipeline->vertexBindingOffsets );

	pipeline->vertexInputState.sType							= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipeline->vertexInputState.pNext							= NULL;
	pipeline->vertexInputState.flags							= 0;
	pipeline->vertexInputState.vertexBindingDescriptionCount	= pipeline->vertexBindingCount;
	pipeline->vertexInputState.pVertexBindingDescriptions		= pipeline->vertexBindings;
	pipeline->vertexInputState.vertexAttributeDescriptionCount	= pipeline->vertexAttributeCount;
	pipeline->vertexInputState.pVertexAttributeDescriptions		= pipeline->vertexAttributes;

	pipeline->inputAssemblyState.sType							= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipeline->inputAssemblyState.pNext							= NULL;
	pipeline->inputAssemblyState.flags							= 0;
	pipeline->inputAssemblyState.topology						= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipeline->inputAssemblyState.primitiveRestartEnable			= VK_FALSE;

	VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo;
	tessellationStateCreateInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessellationStateCreateInfo.pNext							= NULL;
	tessellationStateCreateInfo.flags							= 0;
	tessellationStateCreateInfo.patchControlPoints				= 0;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
	viewportStateCreateInfo.sType								= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.pNext								= NULL;
	viewportStateCreateInfo.flags								= 0;
	viewportStateCreateInfo.viewportCount						= 1;
	viewportStateCreateInfo.pViewports							= NULL;
	viewportStateCreateInfo.scissorCount						= 1;
	viewportStateCreateInfo.pScissors							= NULL;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
	rasterizationStateCreateInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.pNext							= NULL;
	rasterizationStateCreateInfo.flags							= 0;
	rasterizationStateCreateInfo.depthClampEnable				= VK_TRUE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable		= VK_FALSE;
	rasterizationStateCreateInfo.polygonMode					= VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.cullMode						= VK_CULL_MODE_BACK_BIT;
	rasterizationStateCreateInfo.frontFace						= VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable				= VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor		= 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp					= 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor			= 0.0f;
	rasterizationStateCreateInfo.lineWidth						= 1.0f;

	uint32_t sampleMask = 0x1;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
	multisampleStateCreateInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.pNext							= NULL;
	multisampleStateCreateInfo.flags							= 0;
	multisampleStateCreateInfo.rasterizationSamples				= 1;
	multisampleStateCreateInfo.sampleShadingEnable				= VK_FALSE;
	multisampleStateCreateInfo.minSampleShading					= 1.0f;
	multisampleStateCreateInfo.pSampleMask						= &sampleMask;
	multisampleStateCreateInfo.alphaToCoverageEnable			= VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable					= VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
	depthStencilStateCreateInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.pNext							= NULL;
	depthStencilStateCreateInfo.flags							= 0;
	depthStencilStateCreateInfo.depthTestEnable					= parms->rop.depthTestEnable ? VK_TRUE : VK_FALSE;
	depthStencilStateCreateInfo.depthWriteEnable				= parms->rop.depthWriteEnable ? VK_TRUE : VK_FALSE;
	depthStencilStateCreateInfo.depthCompareOp					= (VkCompareOp)parms->rop.depthCompare;
	depthStencilStateCreateInfo.depthBoundsTestEnable			= VK_FALSE;
	depthStencilStateCreateInfo.stencilTestEnable				= VK_FALSE;
	depthStencilStateCreateInfo.front.failOp					= VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.front.passOp					= VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.front.depthFailOp				= VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.front.compareOp					= VK_COMPARE_OP_ALWAYS;
	depthStencilStateCreateInfo.back.failOp						= VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.back.passOp						= VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.back.depthFailOp				= VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.back.compareOp					= VK_COMPARE_OP_ALWAYS;
	depthStencilStateCreateInfo.minDepthBounds					= 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds					= 1.0f;

	VkPipelineColorBlendAttachmentState colorBlendAttachementState[1];
	colorBlendAttachementState[0].blendEnable					= parms->rop.blendEnable ? VK_TRUE : VK_FALSE;
	colorBlendAttachementState[0].srcColorBlendFactor			= (VkBlendFactor)parms->rop.blendSrcColor;
	colorBlendAttachementState[0].dstColorBlendFactor			= (VkBlendFactor)parms->rop.blendDstColor;
	colorBlendAttachementState[0].colorBlendOp					= (VkBlendOp)parms->rop.blendOpColor;
	colorBlendAttachementState[0].srcAlphaBlendFactor			= (VkBlendFactor)parms->rop.blendSrcAlpha;
	colorBlendAttachementState[0].dstAlphaBlendFactor			= (VkBlendFactor)parms->rop.blendDstAlpha;
	colorBlendAttachementState[0].alphaBlendOp					= (VkBlendOp)parms->rop.blendOpAlpha;
	colorBlendAttachementState[0].colorWriteMask				=	VK_COLOR_COMPONENT_R_BIT |
																	VK_COLOR_COMPONENT_G_BIT |
																	VK_COLOR_COMPONENT_B_BIT |
																	( parms->rop.alphaWriteEnable ? VK_COLOR_COMPONENT_A_BIT : 0 );

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
	colorBlendStateCreateInfo.sType								= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.pNext								= NULL;
	colorBlendStateCreateInfo.flags								= 0;
	colorBlendStateCreateInfo.logicOpEnable						= VK_FALSE;
	colorBlendStateCreateInfo.logicOp							= VK_LOGIC_OP_CLEAR;
	colorBlendStateCreateInfo.attachmentCount					= 1;
	colorBlendStateCreateInfo.pAttachments						= colorBlendAttachementState;
	colorBlendStateCreateInfo.blendConstants[0]					= 1.0f;
	colorBlendStateCreateInfo.blendConstants[1]					= 1.0f;
	colorBlendStateCreateInfo.blendConstants[2]					= 1.0f;
	colorBlendStateCreateInfo.blendConstants[3]					= 1.0f;

	VkDynamicState dynamicStateEnables[] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
	pipelineDynamicStateCreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineDynamicStateCreateInfo.pNext						= NULL;
	pipelineDynamicStateCreateInfo.flags						= 0;
	pipelineDynamicStateCreateInfo.dynamicStateCount			= ARRAY_SIZE( dynamicStateEnables );
	pipelineDynamicStateCreateInfo.pDynamicStates				= dynamicStateEnables;

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
	graphicsPipelineCreateInfo.sType							= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.pNext							= NULL;
	graphicsPipelineCreateInfo.flags							= 0;
	graphicsPipelineCreateInfo.stageCount						= 2;
	graphicsPipelineCreateInfo.pStages							= parms->program->pipelineStages;
	graphicsPipelineCreateInfo.pVertexInputState				= &pipeline->vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState				= &pipeline->inputAssemblyState;
	graphicsPipelineCreateInfo.pTessellationState				= &tessellationStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState					= &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState				= &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState				= &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState				= ( parms->renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED ) ? &depthStencilStateCreateInfo : NULL;
	graphicsPipelineCreateInfo.pColorBlendState					= &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState					= &pipelineDynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout							= parms->program->parmLayout.pipelineLayout;
	graphicsPipelineCreateInfo.renderPass						= parms->renderPass->renderPass;
	graphicsPipelineCreateInfo.subpass							= 0;
	graphicsPipelineCreateInfo.basePipelineHandle				= VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex				= 0;

	VK( context->device->vkCreateGraphicsPipelines( context->device->device, context->pipelineCache, 1, &graphicsPipelineCreateInfo, VK_ALLOCATOR, &pipeline->pipeline ) );

	return true;
}

static void GpuGraphicsPipeline_Destroy( GpuContext_t * context, GpuGraphicsPipeline_t * pipeline )
{
	VC( context->device->vkDestroyPipeline( context->device->device, pipeline->pipeline, VK_ALLOCATOR ) );

	memset( pipeline, 0, sizeof( GpuGraphicsPipeline_t ) );
}

/*
================================================================================================================================

GPU compute pipeline.

A compute pipeline encapsulates a compute program.
For optimal performance, a compute pipeline should only be created at load time, not at runtime.

GpuComputePipeline_t

static bool GpuComputePipeline_Create( GpuContext_t * context, GpuComputePipeline_t * pipeline, const GpuComputeProgram_t * program );
static void GpuComputePipeline_Destroy( GpuContext_t * context, GpuComputePipeline_t * pipeline );

================================================================================================================================
*/

typedef struct
{
	const GpuComputeProgram_t *	program;
	VkPipeline					pipeline;
} GpuComputePipeline_t;

static bool GpuComputePipeline_Create( GpuContext_t * context, GpuComputePipeline_t * pipeline, const GpuComputeProgram_t * program )
{
	pipeline->program = program;

	VkComputePipelineCreateInfo computePipelineCreateInfo;
	computePipelineCreateInfo.sType					= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext					= NULL;
	computePipelineCreateInfo.stage					= program->pipelineStage;
	computePipelineCreateInfo.flags					= 0;
	computePipelineCreateInfo.layout				= program->parmLayout.pipelineLayout;
	computePipelineCreateInfo.basePipelineHandle	= VK_NULL_HANDLE;
	computePipelineCreateInfo.basePipelineIndex		= 0;

	VK( context->device->vkCreateComputePipelines( context->device->device, context->pipelineCache, 1, &computePipelineCreateInfo, VK_ALLOCATOR, &pipeline->pipeline ) );

	return true;
}

static void GpuComputePipeline_Destroy( GpuContext_t * context, GpuComputePipeline_t * pipeline )
{
	VC( context->device->vkDestroyPipeline( context->device->device, pipeline->pipeline, VK_ALLOCATOR ) );

	memset( pipeline, 0, sizeof( GpuComputePipeline_t ) );
}

/*
================================================================================================================================

GPU fence.

A fence is used to notify completion of a command buffer.
For optimal performance, a fence should only be created at load time, not at runtime.

GpuFence_t

static void GpuFence_Create( GpuContext_t * context, GpuFence_t * fence );
static void GpuFence_Destroy( GpuContext_t * context, GpuFence_t * fence );
static void GpuFence_Submit( GpuContext_t * context, GpuFence_t * fence );
static void GpuFence_IsSignalled( GpuContext_t * context, GpuFence_t * fence );

================================================================================================================================
*/

typedef struct
{
	VkFence			fence;
	bool			submitted;
} GpuFence_t;

static void GpuFence_Create( GpuContext_t * context, GpuFence_t * fence )
{
	VkFenceCreateInfo fenceCreateInfo;
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = NULL;
	fenceCreateInfo.flags = 0;

	VK( context->device->vkCreateFence( context->device->device, &fenceCreateInfo, VK_ALLOCATOR, &fence->fence ) );

	fence->submitted = false;
}

static void GpuFence_Destroy( GpuContext_t * context, GpuFence_t * fence )
{
	VC( context->device->vkDestroyFence( context->device->device, fence->fence, VK_ALLOCATOR ) );
	fence->fence = VK_NULL_HANDLE;
	fence->submitted = false;
}

static void GpuFence_Submit( GpuContext_t * context, GpuFence_t * fence )
{
	UNUSED_PARM( context );
	fence->submitted = true;
}

static bool GpuFence_IsSignalled( GpuContext_t * context, GpuFence_t * fence )
{
	if ( fence == NULL || !fence->submitted )
	{
		return false;
	}
	VC( VkResult res = context->device->vkGetFenceStatus( context->device->device, fence->fence ) );
	return ( res == VK_SUCCESS );
}

/*
================================================================================================================================

GPU timer.

A timer is used to measure the amount of time it takes to complete GPU commands.
For optimal performance, a timer should only be created at load time, not at runtime.
To avoid synchronization, GpuTimer_GetMilliseconds() reports the time from GPU_TIMER_FRAMES_DELAYED frames ago.
Timer queries are allowed to overlap and can be nested.
Timer queries that are issued inside a render pass may not produce accurate times on tiling GPUs.

GpuTimer_t

static void GpuTimer_Create( GpuContext_t * context, GpuTimer_t * timer );
static void GpuTimer_Destroy( GpuContext_t * context, GpuTimer_t * timer );
static float GpuTimer_GetMilliseconds( GpuTimer_t * timer );

================================================================================================================================
*/

#define GPU_TIMER_FRAMES_DELAYED	2

typedef struct
{
	VkBool32			supported;
	float				period;
	VkQueryPool			pool;
	uint32_t			init;
	uint32_t			index;
	uint64_t			data[2];
} GpuTimer_t;

static void GpuTimer_Create( GpuContext_t * context, GpuTimer_t * timer )
{
	memset( timer, 0, sizeof( GpuTimer_t ) );

	timer->supported = context->device->queueFamilyProperties[context->queueFamilyIndex].timestampValidBits != 0;
	timer->period = context->device->physicalDeviceProperties.limits.timestampPeriod;

	const uint32_t queryCount = ( GPU_TIMER_FRAMES_DELAYED + 1 ) * 2;

	VkQueryPoolCreateInfo queryPoolCreateInfo;
	queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	queryPoolCreateInfo.pNext = NULL;
	queryPoolCreateInfo.flags = 0;
	queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	queryPoolCreateInfo.queryCount = queryCount;
	queryPoolCreateInfo.pipelineStatistics = 0;

	VK( context->device->vkCreateQueryPool( context->device->device, &queryPoolCreateInfo, VK_ALLOCATOR, &timer->pool ) );

	GpuContext_CreateSetupCmdBuffer( context );
	VC( context->device->vkCmdResetQueryPool( context->setupCommandBuffer, timer->pool, 0, queryCount ) );
	GpuContext_FlushSetupCmdBuffer( context );
}

static void GpuTimer_Destroy( GpuContext_t * context, GpuTimer_t * timer )
{
	VC( context->device->vkDestroyQueryPool( context->device->device, timer->pool, VK_ALLOCATOR ) );
}

static float GpuTimer_GetMilliseconds( GpuTimer_t * timer )
{
	return ( timer->data[1] - timer->data[0] ) * timer->period * ( 1.0f / 1000.0f / 1000.0f );
}

/*
================================================================================================================================

GPU program parm state.

GpuProgramParmState_t

================================================================================================================================
*/

#define SAVE_PUSH_CONSTANT_STATE 	1

typedef struct
{
	const void *	parms[MAX_PROGRAM_PARMS];
#if SAVE_PUSH_CONSTANT_STATE == 1
	unsigned char	data[MAX_PROGRAM_PARMS * sizeof( float[4] )];
#endif
} GpuProgramParmState_t;

static void GpuProgramParmState_SetParm( GpuProgramParmState_t * parmState, const GpuProgramParmLayout_t * parmLayout,
											const int index, const GpuProgramParmType_t parmType, const void * pointer )
{
	assert( index >= 0 && index < MAX_PROGRAM_PARMS );
	if ( pointer != NULL )
	{
		bool found = false;
		for ( int i = 0; i < parmLayout->numParms; i++ )
		{
			if ( parmLayout->parms[i].index == index )
			{
				assert( parmLayout->parms[i].type == parmType );
				found = true;
				break;
			}
		}
		// Currently parms can be set even if they are not used by the program.
		//assert( found );
		UNUSED_PARM( found );
	}

	parmState->parms[index] = pointer;

#if SAVE_PUSH_CONSTANT_STATE == 1
	const int pushConstantSize = GpuProgramParm_GetPushConstantSize( parmType );
	if ( pushConstantSize > 0 )
	{
		assert( parmLayout->offsetForIndex[index] + pushConstantSize <= MAX_PROGRAM_PARMS * sizeof( float[4] ) );
		memcpy( &parmState->data[parmLayout->offsetForIndex[index]], pointer, pushConstantSize );
	}
#endif
}

static const void * GpuProgramParmState_NewPushConstantData( const GpuProgramParmLayout_t * newLayout, const int newPushConstantIndex, const GpuProgramParmState_t * newParmState,
													const GpuProgramParmLayout_t * oldLayout, const int oldPushConstantIndex, const GpuProgramParmState_t * oldParmState,
													const bool force )
{
#if SAVE_PUSH_CONSTANT_STATE == 1
	const GpuProgramParm_t * newParm = newLayout->pushConstants[newPushConstantIndex];
	const unsigned char * newData = &newParmState->data[newLayout->offsetForIndex[newParm->index]];
	if ( force || oldLayout == NULL || oldPushConstantIndex >= oldLayout->numPushConstants )
	{
		return newData;
	}
	const GpuProgramParm_t * oldParm = oldLayout->pushConstants[oldPushConstantIndex];
	const unsigned char * oldData = &oldParmState->data[oldLayout->offsetForIndex[oldParm->index]];
	if ( newParm->type != oldParm->type || newParm->binding != oldParm->binding )
	{
		return newData;
	}
	const int pushConstantSize = GpuProgramParm_GetPushConstantSize( newParm->type );
	if ( memcmp( newData, oldData, pushConstantSize ) != 0 )
	{
		return newData;
	}
	return NULL;
#else
	if ( force || oldLayout == NULL || oldPushConstantIndex >= oldLayout->numPushConstants ||
			newLayout->pushConstants[newPushConstantIndex]->binding != oldLayout->pushConstants[oldPushConstantIndex]->binding ||
				newLayout->pushConstants[newPushConstantIndex]->type != oldLayout->pushConstants[oldPushConstantIndex]->type ||
					newParmState->parms[newLayout->pushConstants[newPushConstantIndex]->index] != oldParmState->parms[oldLayout->pushConstants[oldPushConstantIndex]->index] )
	{
		return newParmState->parms[newLayout->pushConstants[newPushConstantIndex]->index];
	}
	return NULL;
#endif
}

static bool GpuProgramParmState_DescriptorsMatch( const GpuProgramParmLayout_t * layout1, const GpuProgramParmState_t * parmState1,
													const GpuProgramParmLayout_t * layout2, const GpuProgramParmState_t * parmState2 )
{
	if ( layout1 == NULL || layout2 == NULL )
	{
		return false;
	}
	if ( layout1->hash != layout2->hash )
	{
		return false;
	}
	for ( int i = 0; i < layout1->numBindings; i++ )
	{
		if ( parmState1->parms[layout1->bindings[i]->index] != parmState2->parms[layout2->bindings[i]->index] )
		{
			return false;
		}
	}
	return true;
}

/*
================================================================================================================================

GPU graphics commands.

A graphics command encapsulates all Vulkan state associated with a single draw call.
The pointers passed in as parameters are expected to point to unique objects that persist
at least past the submission of the command buffer into which the graphics command is
submitted. Because pointers are maintained as state, DO NOT use pointers to local
variables that will go out of scope before the command buffer is submitted.

GpuGraphicsCommand_t

static void GpuGraphicsCommand_Init( GpuGraphicsCommand_t * command );
static void GpuGraphicsCommand_SetPipeline( GpuGraphicsCommand_t * command, const GpuGraphicsPipeline_t * pipeline );
static void GpuGraphicsCommand_SetVertexBuffer( GpuGraphicsCommand_t * command, const GpuBuffer_t * vertexBuffer );
static void GpuGraphicsCommand_SetInstanceBuffer( GpuGraphicsCommand_t * command, const GpuBuffer_t * instanceBuffer );
static void GpuGraphicsCommand_SetParmTextureSampled( GpuGraphicsCommand_t * command, const int index, const GpuTexture_t * texture );
static void GpuGraphicsCommand_SetParmTextureStorage( GpuGraphicsCommand_t * command, const int index, const GpuTexture_t * texture );
static void GpuGraphicsCommand_SetParmBufferUniform( GpuGraphicsCommand_t * command, const int index, const GpuBuffer_t * buffer );
static void GpuGraphicsCommand_SetParmBufferStorage( GpuGraphicsCommand_t * command, const int index, const GpuBuffer_t * buffer );
static void GpuGraphicsCommand_SetParmInt( GpuGraphicsCommand_t * command, const int index, const int * value );
static void GpuGraphicsCommand_SetParmIntVector2( GpuGraphicsCommand_t * command, const int index, const Vector2i_t * value );
static void GpuGraphicsCommand_SetParmIntVector3( GpuGraphicsCommand_t * command, const int index, const Vector3i_t * value );
static void GpuGraphicsCommand_SetParmIntVector4( GpuGraphicsCommand_t * command, const int index, const Vector4i_t * value );
static void GpuGraphicsCommand_SetParmFloat( GpuGraphicsCommand_t * command, const int index, const float * value );
static void GpuGraphicsCommand_SetParmFloatVector2( GpuGraphicsCommand_t * command, const int index, const Vector2f_t * value );
static void GpuGraphicsCommand_SetParmFloatVector3( GpuGraphicsCommand_t * command, const int index, const Vector3f_t * value );
static void GpuGraphicsCommand_SetParmFloatVector4( GpuGraphicsCommand_t * command, const int index, const Vector3f_t * value );
static void GpuGraphicsCommand_SetNumInstances( GpuGraphicsCommand_t * command, const int numInstances );

================================================================================================================================
*/

typedef struct
{
	const GpuGraphicsPipeline_t *	pipeline;
	const GpuBuffer_t *				vertexBuffer;		// vertex buffer returned by GpuCommandBuffer_MapVertexAttributes
	const GpuBuffer_t *				instanceBuffer;		// instance buffer returned by GpuCommandBuffer_MapInstanceAttributes
	GpuProgramParmState_t			parmState;
	int								numInstances;
} GpuGraphicsCommand_t;

static void GpuGraphicsCommand_Init( GpuGraphicsCommand_t * command )
{
	command->pipeline = NULL;
	command->vertexBuffer = NULL;
	command->instanceBuffer = NULL;
	memset( (void *)&command->parmState, 0, sizeof( command->parmState ) );
	command->numInstances = 1;
}

static void GpuGraphicsCommand_SetPipeline( GpuGraphicsCommand_t * command, const GpuGraphicsPipeline_t * pipeline )
{
	command->pipeline = pipeline;
}

static void GpuGraphicsCommand_SetVertexBuffer( GpuGraphicsCommand_t * command, const GpuBuffer_t * vertexBuffer )
{
	command->vertexBuffer = vertexBuffer;
}

static void GpuGraphicsCommand_SetInstanceBuffer( GpuGraphicsCommand_t * command, const GpuBuffer_t * instanceBuffer )
{
	command->instanceBuffer = instanceBuffer;
}

static void GpuGraphicsCommand_SetParmTextureSampled( GpuGraphicsCommand_t * command, const int index, const GpuTexture_t * texture )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED, texture );
}

static void GpuGraphicsCommand_SetParmTextureStorage( GpuGraphicsCommand_t * command, const int index, const GpuTexture_t * texture )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE, texture );
}

static void GpuGraphicsCommand_SetParmBufferUniform( GpuGraphicsCommand_t * command, const int index, const GpuBuffer_t * buffer )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM, buffer );
}

static void GpuGraphicsCommand_SetParmBufferStorage( GpuGraphicsCommand_t * command, const int index, const GpuBuffer_t * buffer )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE, buffer );
}

static void GpuGraphicsCommand_SetParmInt( GpuGraphicsCommand_t * command, const int index, const int * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT, value );
}

static void GpuGraphicsCommand_SetParmIntVector2( GpuGraphicsCommand_t * command, const int index, const Vector2i_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2, value );
}

static void GpuGraphicsCommand_SetParmIntVector3( GpuGraphicsCommand_t * command, const int index, const Vector3i_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3, value );
}

static void GpuGraphicsCommand_SetParmIntVector4( GpuGraphicsCommand_t * command, const int index, const Vector4i_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4, value );
}

static void GpuGraphicsCommand_SetParmFloat( GpuGraphicsCommand_t * command, const int index, const float * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT, value );
}

static void GpuGraphicsCommand_SetParmFloatVector2( GpuGraphicsCommand_t * command, const int index, const Vector2f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2, value );
}

static void GpuGraphicsCommand_SetParmFloatVector3( GpuGraphicsCommand_t * command, const int index, const Vector3f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3, value );
}

static void GpuGraphicsCommand_SetParmFloatVector4( GpuGraphicsCommand_t * command, const int index, const Vector4f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4, value );
}

static void GpuGraphicsCommand_SetParmFloatMatrix4x4( GpuGraphicsCommand_t * command, const int index, const Matrix4x4f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4, value );
}

static void GpuGraphicsCommand_SetNumInstances( GpuGraphicsCommand_t * command, const int numInstances )
{
	command->numInstances = numInstances;
}

/*
================================================================================================================================

GPU compute commands.

A compute command encapsulates all Vulkan state associated with a single dispatch.
The pointers passed in as parameters are expected to point to unique objects that persist
at least past the submission of the command buffer into which the compute command is
submitted. Because various pointer are maintained as state, DO NOT use pointers to local
variables that will go out of scope before the command buffer is submitted.

GpuComputeCommand_t

static void GpuComputeCommand_Init( GpuComputeCommand_t * command );
static void GpuComputeCommand_SetPipeline( GpuComputeCommand_t * command, const GpuComputePipeline_t * pipeline );
static void GpuComputeCommand_SetParmTextureSampled( GpuComputeCommand_t * command, const int index, const GpuTexture_t * texture );
static void GpuComputeCommand_SetParmTextureStorage( GpuComputeCommand_t * command, const int index, const GpuTexture_t * texture );
static void GpuComputeCommand_SetParmBufferUniform( GpuComputeCommand_t * command, const int index, const GpuBuffer_t * buffer );
static void GpuComputeCommand_SetParmBufferStorage( GpuComputeCommand_t * command, const int index, const GpuBuffer_t * buffer );
static void GpuComputeCommand_SetParmInt( GpuComputeCommand_t * command, const int index, const int * value );
static void GpuComputeCommand_SetParmIntVector2( GpuComputeCommand_t * command, const int index, const Vector2i_t * value );
static void GpuComputeCommand_SetParmIntVector3( GpuComputeCommand_t * command, const int index, const Vector3i_t * value );
static void GpuComputeCommand_SetParmIntVector4( GpuComputeCommand_t * command, const int index, const Vector4i_t * value );
static void GpuComputeCommand_SetParmFloat( GpuComputeCommand_t * command, const int index, const float * value );
static void GpuComputeCommand_SetParmFloatVector2( GpuComputeCommand_t * command, const int index, const Vector2f_t * value );
static void GpuComputeCommand_SetParmFloatVector3( GpuComputeCommand_t * command, const int index, const Vector3f_t * value );
static void GpuComputeCommand_SetParmFloatVector4( GpuComputeCommand_t * command, const int index, const Vector3f_t * value );
static void GpuComputeCommand_SetDimensions( GpuComputeCommand_t * command, const int x, const int y, const int z );

================================================================================================================================
*/

typedef struct
{
	const GpuComputePipeline_t *	pipeline;
	GpuProgramParmState_t			parmState;
	int								x;
	int								y;
	int								z;
} GpuComputeCommand_t;

static void GpuComputeCommand_Init( GpuComputeCommand_t * command )
{
	command->pipeline = NULL;
	memset( (void *)&command->parmState, 0, sizeof( command->parmState ) );
	command->x = 1;
	command->y = 1;
	command->z = 1;
}

static void GpuComputeCommand_SetPipeline( GpuComputeCommand_t * command, const GpuComputePipeline_t * pipeline )
{
	command->pipeline = pipeline;
}

static void GpuComputeCommand_SetParmTextureSampled( GpuComputeCommand_t * command, const int index, const GpuTexture_t * texture )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED, texture );
}

static void GpuComputeCommand_SetParmTextureStorage( GpuComputeCommand_t * command, const int index, const GpuTexture_t * texture )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE, texture );
}

static void GpuComputeCommand_SetParmBufferUniform( GpuComputeCommand_t * command, const int index, const GpuBuffer_t * buffer )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM, buffer );
}

static void GpuComputeCommand_SetParmBufferStorage( GpuComputeCommand_t * command, const int index, const GpuBuffer_t * buffer )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE, buffer );
}

static void GpuComputeCommand_SetParmInt( GpuComputeCommand_t * command, const int index, const int * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT, value );
}

static void GpuComputeCommand_SetParmIntVector2( GpuComputeCommand_t * command, const int index, const Vector2i_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2, value );
}

static void GpuComputeCommand_SetParmIntVector3( GpuComputeCommand_t * command, const int index, const Vector3i_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3, value );
}

static void GpuComputeCommand_SetParmIntVector4( GpuComputeCommand_t * command, const int index, const Vector4i_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4, value );
}

static void GpuComputeCommand_SetParmFloat( GpuComputeCommand_t * command, const int index, const float * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT, value );
}

static void GpuComputeCommand_SetParmFloatVector2( GpuComputeCommand_t * command, const int index, const Vector2f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2, value );
}

static void GpuComputeCommand_SetParmFloatVector3( GpuComputeCommand_t * command, const int index, const Vector3f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3, value );
}

static void GpuComputeCommand_SetParmFloatVector4( GpuComputeCommand_t * command, const int index, const Vector4f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4, value );
}

static void GpuComputeCommand_SetParmFloatMatrix4x4( GpuComputeCommand_t * command, const int index, const Matrix4x4f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4, value );
}

static void GpuComputeCommand_SetDimensions( GpuComputeCommand_t * command, const int x, const int y, const int z )
{
	command->x = x;
	command->y = y;
	command->z = z;
}

/*
================================================================================================================================

GPU pipeline resources.

Resources, like texture and uniform buffer descriptions, that are used by a graphics or compute pipeline.

GpuPipelineResources_t

static void GpuPipelineResources_Create( GpuContext_t * context, GpuPipelineResources_t * resources,
										const GpuProgramParmLayout_t * parmLayout,
										const GpuProgramParmState_t * parms );
static void GpuPipelineResources_Destroy( GpuContext_t * context, GpuPipelineResources_t * resources );

================================================================================================================================
*/

typedef struct GpuPipelineResources_s
{
	struct GpuPipelineResources_s *	next;
	int								unusedCount;			// Number of frames these resources have not been used.
	const GpuProgramParmLayout_t *	parmLayout;
	GpuProgramParmState_t			parms;
	VkDescriptorPool				descriptorPool;
	VkDescriptorSet					descriptorSet;
} GpuPipelineResources_t;

static void GpuPipelineResources_Create( GpuContext_t * context, GpuPipelineResources_t * resources,
										const GpuProgramParmLayout_t * parmLayout,
										const GpuProgramParmState_t * parms )
{
	memset( resources, 0, sizeof( GpuPipelineResources_t ) );

	resources->parmLayout = parmLayout;
	memcpy( (void *)&resources->parms, parms, sizeof( GpuProgramParmState_t ) );

	//
	// Create descriptor pool.
	//

	{
		VkDescriptorPoolSize typeCounts[MAX_PROGRAM_PARMS];

		int count = 0;
		for ( int i = 0; i < parmLayout->numBindings; i++ )
		{
			VkDescriptorType type = GpuProgramParm_GetDescriptorType( parmLayout->bindings[i]->type );
			for ( int j = 0; j < count; j++ )
			{
				if ( typeCounts[j].type == type )
				{
					typeCounts[j].descriptorCount++;
					type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
					break;
				}
			}
			if ( type != VK_DESCRIPTOR_TYPE_MAX_ENUM )
			{
				typeCounts[count].type = type;
				typeCounts[count].descriptorCount = 1;
				count++;
			}
		}
		if ( count == 0 )
		{
			typeCounts[count].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			typeCounts[count].descriptorCount = 1;
			count++;
		}

		VkDescriptorPoolCreateInfo destriptorPoolCreateInfo;
		destriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		destriptorPoolCreateInfo.pNext = NULL;
		destriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		destriptorPoolCreateInfo.maxSets = 1;
		destriptorPoolCreateInfo.poolSizeCount = count;
		destriptorPoolCreateInfo.pPoolSizes = ( count != 0 ) ? typeCounts : NULL;

		VK( context->device->vkCreateDescriptorPool( context->device->device, &destriptorPoolCreateInfo, VK_ALLOCATOR, &resources->descriptorPool ) );
	}

	//
	// Allocated and update a descriptor set.
	//

	{
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pNext = NULL;
		descriptorSetAllocateInfo.descriptorPool = resources->descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &parmLayout->descriptorSetLayout;

		VK( context->device->vkAllocateDescriptorSets( context->device->device, &descriptorSetAllocateInfo, &resources->descriptorSet ) );

		VkWriteDescriptorSet writes[MAX_PROGRAM_PARMS] = { { 0 } };
		VkDescriptorImageInfo imageInfo[MAX_PROGRAM_PARMS] = { { 0 } };
		VkDescriptorBufferInfo bufferInfo[MAX_PROGRAM_PARMS] = { { 0 } };

		int numWrites = 0;
		for ( int i = 0; i < parmLayout->numBindings; i++ )
		{
			const GpuProgramParm_t * binding = parmLayout->bindings[i];

			writes[numWrites].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[numWrites].pNext = NULL;
			writes[numWrites].dstSet = resources->descriptorSet;
			writes[numWrites].dstBinding = binding->binding;
			writes[numWrites].dstArrayElement = 0;
			writes[numWrites].descriptorCount = 1;
			writes[numWrites].descriptorType = GpuProgramParm_GetDescriptorType( parmLayout->bindings[i]->type );
			writes[numWrites].pImageInfo = &imageInfo[numWrites];
			writes[numWrites].pBufferInfo = &bufferInfo[numWrites];
			writes[numWrites].pTexelBufferView = NULL;

			if ( binding->type == GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED )
			{
				const GpuTexture_t * texture = (const GpuTexture_t *)parms->parms[binding->index];
				assert( texture->usage == GPU_TEXTURE_USAGE_SAMPLED );
				assert( texture->imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

				imageInfo[numWrites].sampler = texture->sampler;
				imageInfo[numWrites].imageView = texture->view;
				imageInfo[numWrites].imageLayout = texture->imageLayout;
			}
			else if ( binding->type == GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE )
			{
				const GpuTexture_t * texture = (const GpuTexture_t *)parms->parms[binding->index];
				assert( texture->usage == GPU_TEXTURE_USAGE_STORAGE );
				assert( texture->imageLayout == VK_IMAGE_LAYOUT_GENERAL );

				imageInfo[numWrites].sampler = VK_NULL_HANDLE;
				imageInfo[numWrites].imageView = texture->view;
				imageInfo[numWrites].imageLayout = texture->imageLayout;
			}
			else if ( binding->type == GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM )
			{
				const GpuBuffer_t * buffer = (const GpuBuffer_t *)parms->parms[binding->index];
				assert( buffer->type == GPU_BUFFER_TYPE_UNIFORM );

				bufferInfo[numWrites].buffer = buffer->buffer;
				bufferInfo[numWrites].offset = 0;
				bufferInfo[numWrites].range = buffer->size;
			}
			else if ( binding->type == GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
			{
				const GpuBuffer_t * buffer = (const GpuBuffer_t *)parms->parms[binding->index];
				assert( buffer->type == GPU_BUFFER_TYPE_STORAGE );

				bufferInfo[numWrites].buffer = buffer->buffer;
				bufferInfo[numWrites].offset = 0;
				bufferInfo[numWrites].range = buffer->size;
			}

			numWrites++;
		}

		if ( numWrites > 0 )
		{
			VC( context->device->vkUpdateDescriptorSets( context->device->device, numWrites, writes, 0, NULL ) );
		}
	}
}

static void GpuPipelineResources_Destroy( GpuContext_t * context, GpuPipelineResources_t * resources )
{
	VC( context->device->vkFreeDescriptorSets( context->device->device, resources->descriptorPool, 1, &resources->descriptorSet ) );
	VC( context->device->vkDestroyDescriptorPool( context->device->device, resources->descriptorPool, VK_ALLOCATOR ) );

	memset( resources, 0, sizeof( GpuPipelineResources_t ) );
}

/*
================================================================================================================================

GPU command buffer.

A command buffer is used to record graphics and compute commands.
For optimal performance, a command buffer should only be created at load time, not at runtime.
When a command is submitted, the state of the command is compared with the currently saved state,
and only the state that has changed translates into Vulkan function calls.

GpuCommandBuffer_t
GpuCommandBufferType_t
GpuBufferUnmapType_t

static void GpuCommandBuffer_Create( GpuContext_t * context, GpuCommandBuffer_t * commandBuffer, const GpuCommandBufferType_t type, const int numBuffers );
static void GpuCommandBuffer_Destroy( GpuContext_t * context, GpuCommandBuffer_t * commandBuffer );

static void GpuCommandBuffer_BeginPrimary( GpuCommandBuffer_t * commandBuffer );
static void GpuCommandBuffer_EndPrimary( GpuCommandBuffer_t * commandBuffer );
static GpuFence_t * GpuCommandBuffer_SubmitPrimary( GpuCommandBuffer_t * commandBuffer );

static void GpuCommandBuffer_BeginSecondary( GpuCommandBuffer_t * commandBuffer, GpuRenderPass_t * renderPass, GpuFramebuffer_t * framebuffer );
static void GpuCommandBuffer_EndSecondary( GpuCommandBuffer_t * commandBuffer );
static void GpuCommandBuffer_SubmitSecondary( GpuCommandBuffer_t * commandBuffer, GpuCommandBuffer_t * primary );

static void GpuCommandBuffer_ChangeTextureUsage( GpuCommandBuffer_t * commandBuffer, GpuTexture_t * texture, const GpuTextureUsage_t usage );

static void GpuCommandBuffer_BeginFramebuffer( GpuCommandBuffer_t * commandBuffer, GpuFramebuffer_t * framebuffer, const int arrayLayer, const GpuTextureUsage_t usage );
static void GpuCommandBuffer_EndFramebuffer( GpuCommandBuffer_t * commandBuffer, GpuFramebuffer_t * framebuffer, const int arrayLayer, const GpuTextureUsage_t usage );

static void GpuCommandBuffer_BeginTimer( GpuCommandBuffer_t * commandBuffer, GpuTimer_t * timer );
static void GpuCommandBuffer_EndTimer( GpuCommandBuffer_t * commandBuffer, GpuTimer_t * timer );

static void GpuCommandBuffer_BeginRenderPass( GpuCommandBuffer_t * commandBuffer, GpuRenderPass_t * renderPass, GpuFramebuffer_t * framebuffer, const ScreenRect_t * rect );
static void GpuCommandBuffer_EndRenderPass( GpuCommandBuffer_t * commandBuffer, GpuRenderPass_t * renderPass );

static void GpuCommandBuffer_SetViewport( GpuCommandBuffer_t * commandBuffer, const ScreenRect_t * rect );
static void GpuCommandBuffer_SetScissor( GpuCommandBuffer_t * commandBuffer, const ScreenRect_t * rect );

static void GpuCommandBuffer_SubmitGraphicsCommand( GpuCommandBuffer_t * commandBuffer, const GpuGraphicsCommand_t * command );
static void GpuCommandBuffer_SubmitComputeCommand( GpuCommandBuffer_t * commandBuffer, const GpuComputeCommand_t * command );

static GpuBuffer_t * GpuCommandBuffer_MapBuffer( GpuCommandBuffer_t * commandBuffer, GpuBuffer_t * buffer, void ** data );
static void GpuCommandBuffer_UnmapBuffer( GpuCommandBuffer_t * commandBuffer, GpuBuffer_t * buffer, GpuBuffer_t * mappedBuffer, const GpuBufferUnmapType_t type );

static GpuBuffer_t * GpuCommandBuffer_MapVertexAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuVertexAttributeArrays_t * attribs );
static void GpuCommandBuffer_UnmapVertexAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuBuffer_t * mappedVertexBuffer, const GpuBufferUnmapType_t type );

static GpuBuffer_t * GpuCommandBuffer_MapInstanceAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuVertexAttributeArrays_t * attribs );
static void GpuCommandBuffer_UnmapInstanceAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuBuffer_t * mappedInstanceBuffer, const GpuBufferUnmapType_t type );

================================================================================================================================
*/

typedef enum
{
	GPU_BUFFER_UNMAP_TYPE_USE_ALLOCATED,			// use the newly allocated (host visible) buffer
	GPU_BUFFER_UNMAP_TYPE_COPY_BACK					// copy back to the original buffer
} GpuBufferUnmapType_t;

typedef enum
{
	GPU_COMMAND_BUFFER_TYPE_PRIMARY,
	GPU_COMMAND_BUFFER_TYPE_SECONDARY,
	GPU_COMMAND_BUFFER_TYPE_SECONDARY_CONTINUE_RENDER_PASS
} GpuCommandBufferType_t;

#define MAX_COMMAND_BUFFER_TIMERS	16

typedef struct
{
	GpuCommandBufferType_t		type;
	int							numBuffers;
	int							currentBuffer;
	VkCommandBuffer *			cmdBuffers;
	GpuContext_t *				context;
	GpuFence_t *				fences;
	GpuBuffer_t **				mappedBuffers;
	GpuBuffer_t **				oldMappedBuffers;
	GpuPipelineResources_t **	pipelineResources;
	GpuSwapchainBuffer_t *		swapchainBuffer;
	GpuGraphicsCommand_t		currentGraphicsState;
	GpuComputeCommand_t			currentComputeState;
	GpuFramebuffer_t *			currentFramebuffer;
	GpuRenderPass_t *			currentRenderPass;
	GpuTimer_t *				currentTimers[MAX_COMMAND_BUFFER_TIMERS];
	int							currentTimerCount;
} GpuCommandBuffer_t;

#define MAX_VERTEX_BUFFER_UNUSED_COUNT			16
#define MAX_PIPELINE_RESOURCES_UNUSED_COUNT		16

static void GpuCommandBuffer_Create( GpuContext_t * context, GpuCommandBuffer_t * commandBuffer, const GpuCommandBufferType_t type, const int numBuffers )
{
	memset( commandBuffer, 0, sizeof( GpuCommandBuffer_t ) );

	commandBuffer->type = type;
	commandBuffer->numBuffers = numBuffers;
	commandBuffer->currentBuffer = 0;
	commandBuffer->context = context;
	commandBuffer->cmdBuffers = (VkCommandBuffer *) malloc( numBuffers * sizeof( VkCommandBuffer ) );
	commandBuffer->fences = (GpuFence_t *) malloc( numBuffers * sizeof( GpuFence_t ) );
	commandBuffer->mappedBuffers = (GpuBuffer_t **) malloc( numBuffers * sizeof( GpuBuffer_t * ) );
	commandBuffer->oldMappedBuffers = (GpuBuffer_t **) malloc( numBuffers * sizeof( GpuBuffer_t * ) );
	commandBuffer->pipelineResources = (GpuPipelineResources_t **) malloc( numBuffers * sizeof( GpuPipelineResources_t * ) );

	for ( int i = 0; i < numBuffers; i++ )
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.pNext = NULL;
		commandBufferAllocateInfo.commandPool = context->commandPool;
		commandBufferAllocateInfo.level = ( type == GPU_COMMAND_BUFFER_TYPE_PRIMARY ) ?
												VK_COMMAND_BUFFER_LEVEL_PRIMARY :
												VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		commandBufferAllocateInfo.commandBufferCount = 1;

		VK( context->device->vkAllocateCommandBuffers( context->device->device, &commandBufferAllocateInfo, &commandBuffer->cmdBuffers[i] ) );

		GpuFence_Create( context, &commandBuffer->fences[i] );

		commandBuffer->mappedBuffers[i] = NULL;
		commandBuffer->oldMappedBuffers[i] = NULL;
		commandBuffer->pipelineResources[i] = NULL;
	}
}

static void GpuCommandBuffer_Destroy( GpuContext_t * context, GpuCommandBuffer_t * commandBuffer )
{
	assert( context == commandBuffer->context );

	for ( int i = 0; i < commandBuffer->numBuffers; i++ )
	{
		VC( context->device->vkFreeCommandBuffers( context->device->device, context->commandPool, 1, &commandBuffer->cmdBuffers[i] ) );

		GpuFence_Destroy( context, &commandBuffer->fences[i] );

		for ( GpuBuffer_t * b = commandBuffer->mappedBuffers[i], * next = NULL; b != NULL; b = next )
		{
			next = b->next;
			GpuBuffer_Destroy( context, b );
			free( b );
		}
		commandBuffer->mappedBuffers[i] = NULL;

		for ( GpuBuffer_t * b = commandBuffer->oldMappedBuffers[i], * next = NULL; b != NULL; b = next )
		{
			next = b->next;
			GpuBuffer_Destroy( context, b );
			free( b );
		}
		commandBuffer->oldMappedBuffers[i] = NULL;

		for ( GpuPipelineResources_t * r = commandBuffer->pipelineResources[i], * next = NULL; r != NULL; r = next )
		{
			next = r->next;
			GpuPipelineResources_Destroy( context, r );
			free( r );
		}
		commandBuffer->pipelineResources[i] = NULL;
	}

	free( commandBuffer->pipelineResources );
	free( commandBuffer->oldMappedBuffers );
	free( commandBuffer->mappedBuffers );
	free( commandBuffer->fences );
	free( commandBuffer->cmdBuffers );

	memset( commandBuffer, 0, sizeof( GpuCommandBuffer_t ) );
}

static void GpuCommandBuffer_ManageBuffers( GpuCommandBuffer_t * commandBuffer )
{
	//
	// Manage buffers.
	//

	{
		// Free any old buffers that were not reused for a number of frames.
		for ( GpuBuffer_t ** b = &commandBuffer->oldMappedBuffers[commandBuffer->currentBuffer]; *b != NULL; )
		{
			if ( (*b)->unusedCount++ >= MAX_VERTEX_BUFFER_UNUSED_COUNT )
			{
				GpuBuffer_t * next = (*b)->next;
				GpuBuffer_Destroy( commandBuffer->context, *b );
				free( *b );
				*b = next;
			}
			else
			{
				b = &(*b)->next;
			}
		}

		// Move the last used buffers to the list with old buffers.
		for ( GpuBuffer_t * b = commandBuffer->mappedBuffers[commandBuffer->currentBuffer], * next = NULL; b != NULL; b = next )
		{
			next = b->next;
			b->next = commandBuffer->oldMappedBuffers[commandBuffer->currentBuffer];
			commandBuffer->oldMappedBuffers[commandBuffer->currentBuffer] = b;
		}
		commandBuffer->mappedBuffers[commandBuffer->currentBuffer] = NULL;
	}

	//
	// Manage pipeline resources.
	//

	{
		// Free any pipeline resources that were not reused for a number of frames.
		for ( GpuPipelineResources_t ** r = &commandBuffer->pipelineResources[commandBuffer->currentBuffer]; *r != NULL; )
		{
			if ( (*r)->unusedCount++ >= MAX_PIPELINE_RESOURCES_UNUSED_COUNT )
			{
				GpuPipelineResources_t * next = (*r)->next;
				GpuPipelineResources_Destroy( commandBuffer->context, *r );
				free( *r );
				*r = next;
			}
			else
			{
				r = &(*r)->next;
			}
		}
	}
}

static void GpuCommandBuffer_ManageTimers( GpuCommandBuffer_t * commandBuffer )
{
	GpuDevice_t * device = commandBuffer->context->device;

	for ( int i = 0; i < commandBuffer->currentTimerCount; i++ )
	{
		GpuTimer_t * timer = commandBuffer->currentTimers[i];
		timer->index = ( timer->index + 1 ) % ( GPU_TIMER_FRAMES_DELAYED + 1 );
		if ( timer->init >= GPU_TIMER_FRAMES_DELAYED )
		{
			VC( device->vkGetQueryPoolResults( commandBuffer->context->device->device, timer->pool, timer->index * 2, 2,
				2 * sizeof( uint64_t ), timer->data, sizeof( uint64_t ), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT ) );
		}
		else
		{
			timer->init++;
		}
		VC( device->vkCmdResetQueryPool( commandBuffer->cmdBuffers[commandBuffer->currentBuffer], timer->pool, timer->index * 2, 2 ) );
	}
	commandBuffer->currentTimerCount = 0;
}

static void GpuCommandBuffer_BeginPrimary( GpuCommandBuffer_t * commandBuffer )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );

	GpuDevice_t * device = commandBuffer->context->device;

	commandBuffer->currentBuffer = ( commandBuffer->currentBuffer + 1 ) % commandBuffer->numBuffers;

	GpuFence_t * fence = &commandBuffer->fences[commandBuffer->currentBuffer];
	if ( fence->submitted )
	{
		VK( device->vkWaitForFences( device->device, 1, &fence->fence, VK_TRUE, 1ULL * 1000 * 1000 * 1000 ) );
		VK( device->vkResetFences( device->device, 1, &fence->fence ) );
		fence->submitted = false;
	}

	GpuCommandBuffer_ManageBuffers( commandBuffer );

	GpuGraphicsCommand_Init( &commandBuffer->currentGraphicsState );
	GpuComputeCommand_Init( &commandBuffer->currentComputeState );

	VK( device->vkResetCommandBuffer( commandBuffer->cmdBuffers[commandBuffer->currentBuffer], 0 ) );

	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = NULL;
	commandBufferBeginInfo.flags = 0;
	commandBufferBeginInfo.pInheritanceInfo = NULL;

	VK( device->vkBeginCommandBuffer( commandBuffer->cmdBuffers[commandBuffer->currentBuffer], &commandBufferBeginInfo ) );

	// Make sure any CPU writes are flushed.
	{
		VkMemoryBarrier memoryBarrier;
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = NULL;
		memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		memoryBarrier.dstAccessMask = 0;

		const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		const VkDependencyFlags flags = 0;

		VC( device->vkCmdPipelineBarrier( commandBuffer->cmdBuffers[commandBuffer->currentBuffer],
											src_stages, dst_stages, flags, 1, &memoryBarrier, 0, NULL, 0, NULL ) );
	}
}

static void GpuCommandBuffer_EndPrimary( GpuCommandBuffer_t * commandBuffer )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );

	GpuCommandBuffer_ManageTimers( commandBuffer );

	GpuDevice_t * device = commandBuffer->context->device;
	VK( device->vkEndCommandBuffer( commandBuffer->cmdBuffers[commandBuffer->currentBuffer] ) );
}

static GpuFence_t * GpuCommandBuffer_SubmitPrimary( GpuCommandBuffer_t * commandBuffer )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );

	GpuDevice_t * device = commandBuffer->context->device;

	const VkPipelineStageFlags stageFlags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = ( commandBuffer->swapchainBuffer != NULL ) ? 1 : 0;
	submitInfo.pWaitSemaphores = ( commandBuffer->swapchainBuffer != NULL ) ? &commandBuffer->swapchainBuffer->presentCompleteSemaphore : NULL;
	submitInfo.pWaitDstStageMask = ( commandBuffer->swapchainBuffer != NULL ) ? stageFlags : NULL;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer->cmdBuffers[commandBuffer->currentBuffer];
	submitInfo.signalSemaphoreCount = ( commandBuffer->swapchainBuffer != NULL ) ? 1 : 0;
	submitInfo.pSignalSemaphores = ( commandBuffer->swapchainBuffer != NULL ) ? &commandBuffer->swapchainBuffer->renderingCompleteSemaphore : NULL;

	GpuFence_t * fence = &commandBuffer->fences[commandBuffer->currentBuffer];
	VK( device->vkQueueSubmit( commandBuffer->context->queue, 1, &submitInfo, fence->fence ) );
	GpuFence_Submit( commandBuffer->context, fence );

	commandBuffer->swapchainBuffer = NULL;

	return fence;
}

static void GpuCommandBuffer_BeginSecondary( GpuCommandBuffer_t * commandBuffer, GpuRenderPass_t * renderPass, GpuFramebuffer_t * framebuffer )
{
	assert( commandBuffer->type != GPU_COMMAND_BUFFER_TYPE_PRIMARY );

	GpuDevice_t * device = commandBuffer->context->device;

	commandBuffer->currentBuffer = ( commandBuffer->currentBuffer + 1 ) % commandBuffer->numBuffers;

	GpuCommandBuffer_ManageBuffers( commandBuffer );

	GpuGraphicsCommand_Init( &commandBuffer->currentGraphicsState );
	GpuComputeCommand_Init( &commandBuffer->currentComputeState );

	VK( device->vkResetCommandBuffer( commandBuffer->cmdBuffers[commandBuffer->currentBuffer], 0 ) );

	VkCommandBufferInheritanceInfo commandBufferInheritanceInfo;
	commandBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	commandBufferInheritanceInfo.pNext = NULL;
	commandBufferInheritanceInfo.renderPass = (renderPass != NULL ) ? renderPass->renderPass : VK_NULL_HANDLE;
	commandBufferInheritanceInfo.subpass = 0;
	commandBufferInheritanceInfo.framebuffer = ( framebuffer != NULL ) ? framebuffer->framebuffers[framebuffer->currentBuffer] : VK_NULL_HANDLE;
	commandBufferInheritanceInfo.occlusionQueryEnable = VK_FALSE;
	commandBufferInheritanceInfo.queryFlags = 0;
	commandBufferInheritanceInfo.pipelineStatistics = 0;

	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = NULL;
	commandBufferBeginInfo.flags = ( ( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_SECONDARY_CONTINUE_RENDER_PASS ) ?
										VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0 ) |
										VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	commandBufferBeginInfo.pInheritanceInfo = &commandBufferInheritanceInfo;

	VK( device->vkBeginCommandBuffer( commandBuffer->cmdBuffers[commandBuffer->currentBuffer], &commandBufferBeginInfo ) );

	commandBuffer->currentRenderPass = renderPass;
}

static void GpuCommandBuffer_EndSecondary( GpuCommandBuffer_t * commandBuffer )
{
	assert( commandBuffer->type != GPU_COMMAND_BUFFER_TYPE_PRIMARY );

	GpuCommandBuffer_ManageTimers( commandBuffer );
	commandBuffer->currentRenderPass = NULL;

	GpuDevice_t * device = commandBuffer->context->device;
	VK( device->vkEndCommandBuffer( commandBuffer->cmdBuffers[commandBuffer->currentBuffer] ) );
}

static void GpuCommandBuffer_SubmitSecondary( GpuCommandBuffer_t * commandBuffer, GpuCommandBuffer_t * primary )
{
	assert( commandBuffer->type != GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( primary->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( ( primary->currentRenderPass != NULL ) == ( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_SECONDARY_CONTINUE_RENDER_PASS ) );

	GpuDevice_t * device = commandBuffer->context->device;

	VC( device->vkCmdExecuteCommands( primary->cmdBuffers[primary->currentBuffer], 1, &commandBuffer->cmdBuffers[commandBuffer->currentBuffer] ) );
}

static VkImageLayout LayoutForTextureUsage( const GpuTextureUsage_t usage )
{
	return	( ( usage == GPU_TEXTURE_USAGE_UNDEFINED ) ?		VK_IMAGE_LAYOUT_UNDEFINED :
			( ( usage == GPU_TEXTURE_USAGE_SAMPLED ) ?			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :
			( ( usage == GPU_TEXTURE_USAGE_STORAGE ) ?			VK_IMAGE_LAYOUT_GENERAL :
			( ( usage == GPU_TEXTURE_USAGE_COLOR_ATTACHMENT ) ?	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
			( ( usage == GPU_TEXTURE_USAGE_PRESENTATION ) ?		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : 0 ) ) ) ) );
}

static VkAccessFlags AccessForTextureUsage( const GpuTextureUsage_t usage )
{
	return	( ( usage == GPU_TEXTURE_USAGE_UNDEFINED ) ?		( 0 ) :
			( ( usage == GPU_TEXTURE_USAGE_SAMPLED ) ?			( VK_ACCESS_SHADER_READ_BIT ) :
			( ( usage == GPU_TEXTURE_USAGE_STORAGE ) ?			( VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT ) :
			( ( usage == GPU_TEXTURE_USAGE_COLOR_ATTACHMENT ) ?	( VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT ) :
			( ( usage == GPU_TEXTURE_USAGE_PRESENTATION ) ?		( VK_ACCESS_MEMORY_READ_BIT ) : 0 ) ) ) ) );
}

static VkPipelineStageFlags PipelineStagesForTextureUsage( const GpuTextureUsage_t usage, const bool from )
{
	return	( ( usage == GPU_TEXTURE_USAGE_UNDEFINED ) ?		( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT ) :
			( ( usage == GPU_TEXTURE_USAGE_SAMPLED ) ?			( VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ) :
			( ( usage == GPU_TEXTURE_USAGE_STORAGE ) ?			( VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ) :
			( ( usage == GPU_TEXTURE_USAGE_COLOR_ATTACHMENT ) ?	( VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT ) :
			( ( usage == GPU_TEXTURE_USAGE_PRESENTATION ) ?		( from ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT ) : 0 ) ) ) ) );
}

static void GpuCommandBuffer_ChangeTextureUsage( GpuCommandBuffer_t * commandBuffer, GpuTexture_t * texture, const GpuTextureUsage_t usage )
{
	GpuDevice_t * device = commandBuffer->context->device;

	const VkImageLayout newImageLayout = LayoutForTextureUsage( usage );

	VkImageMemoryBarrier imageMemoryBarrier;
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.pNext = NULL;
	imageMemoryBarrier.srcAccessMask = AccessForTextureUsage( texture->usage );
	imageMemoryBarrier.dstAccessMask = AccessForTextureUsage( usage );
	imageMemoryBarrier.oldLayout = texture->imageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = texture->image;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = texture->mipCount;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = texture->layerCount;

	const VkPipelineStageFlags src_stages = PipelineStagesForTextureUsage( texture->usage, true );
	const VkPipelineStageFlags dst_stages = PipelineStagesForTextureUsage( usage, false );
	const VkDependencyFlags flags = 0;

	VC( device->vkCmdPipelineBarrier( commandBuffer->cmdBuffers[commandBuffer->currentBuffer],
										src_stages, dst_stages, flags, 0, NULL, 0, NULL, 1, &imageMemoryBarrier ) );

	texture->usage = usage;
	texture->imageLayout = newImageLayout;
}

static void GpuCommandBuffer_BeginFramebuffer( GpuCommandBuffer_t * commandBuffer, GpuFramebuffer_t * framebuffer, const int arrayLayer, const GpuTextureUsage_t usage )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( arrayLayer >= 0 && arrayLayer < framebuffer->numLayers );

	if ( framebuffer->window != NULL )
	{
		assert( framebuffer->window->swapchain.swapchain != VK_NULL_HANDLE );
		if ( framebuffer->swapchainCreateCount != framebuffer->window->swapchainCreateCount )
		{
			GpuWindow_t * window = framebuffer->window;
			GpuRenderPass_t * renderPass = framebuffer->renderPass;
			GpuFramebuffer_Destroy( commandBuffer->context, framebuffer );
			GpuFramebuffer_CreateFromSwapchain( window, framebuffer, renderPass );
		}

		// Keep track of the current swapchain buffer to handle the swapchain semaphores.
		assert( commandBuffer->swapchainBuffer == NULL );
		commandBuffer->swapchainBuffer = &framebuffer->window->swapchain.buffers[framebuffer->window->swapchain.currentBuffer];

		framebuffer->currentBuffer = commandBuffer->swapchainBuffer->imageIndex;
		framebuffer->currentLayer = 0;
	}
	else
	{
		// Only advance when rendering to the first layer.
		if ( arrayLayer == 0 )
		{
			framebuffer->currentBuffer = ( framebuffer->currentBuffer + 1 ) % framebuffer->numBuffers;
		}
		framebuffer->currentLayer = arrayLayer;
	}

	assert( framebuffer->depthBuffer.internalFormat == VK_FORMAT_UNDEFINED ||
			framebuffer->depthBuffer.imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );

	GpuCommandBuffer_ChangeTextureUsage( commandBuffer, GpuFramebuffer_GetColorTexture( framebuffer ), usage );

	commandBuffer->currentFramebuffer = framebuffer;
}

static void GpuCommandBuffer_EndFramebuffer( GpuCommandBuffer_t * commandBuffer, GpuFramebuffer_t * framebuffer, const int arrayLayer, const GpuTextureUsage_t usage )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == framebuffer );
	assert( arrayLayer >= 0 && arrayLayer < framebuffer->numLayers );

	UNUSED_PARM( arrayLayer );

	GpuCommandBuffer_ChangeTextureUsage( commandBuffer, GpuFramebuffer_GetColorTexture( framebuffer ), usage );

	commandBuffer->currentFramebuffer = NULL;
}

static void GpuCommandBuffer_BeginTimer( GpuCommandBuffer_t * commandBuffer, GpuTimer_t * timer )
{
	GpuDevice_t * device = commandBuffer->context->device;

	// Make sure this timer has not already been used.
	for ( int i = 0; i < commandBuffer->currentTimerCount; i++ )
	{
		assert( commandBuffer->currentTimers[i] != timer );
	}

	VC( device->vkCmdWriteTimestamp( commandBuffer->cmdBuffers[commandBuffer->currentBuffer], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, timer->pool, timer->index * 2 + 0 ) );
}

static void GpuCommandBuffer_EndTimer( GpuCommandBuffer_t * commandBuffer, GpuTimer_t * timer )
{
	GpuDevice_t * device = commandBuffer->context->device;

	VC( device->vkCmdWriteTimestamp( commandBuffer->cmdBuffers[commandBuffer->currentBuffer], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, timer->pool, timer->index * 2 + 1 ) );

	assert( commandBuffer->currentTimerCount < MAX_COMMAND_BUFFER_TIMERS );
	commandBuffer->currentTimers[commandBuffer->currentTimerCount++] = timer;
}

static void GpuCommandBuffer_BeginRenderPass( GpuCommandBuffer_t * commandBuffer, GpuRenderPass_t * renderPass, GpuFramebuffer_t * framebuffer, const ScreenRect_t * rect )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentRenderPass == NULL );
	assert( commandBuffer->currentFramebuffer == framebuffer );

	GpuDevice_t * device = commandBuffer->context->device;

	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];

	VkClearValue clearValues[2];
	memset( clearValues, 0, sizeof( clearValues ) );
	clearValues[0].color.float32[0] = 0.0f;
	clearValues[0].color.float32[1] = 0.0f;
	clearValues[0].color.float32[2] = 0.0f;
	clearValues[0].color.float32[3] = 1.0f;
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;

	VkRenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = NULL;
	renderPassBeginInfo.renderPass = renderPass->renderPass;
	renderPassBeginInfo.framebuffer = framebuffer->framebuffers[framebuffer->currentBuffer * framebuffer->numLayers + framebuffer->currentLayer ];
	renderPassBeginInfo.renderArea.offset.x = rect->x;
	renderPassBeginInfo.renderArea.offset.y = rect->y;
	renderPassBeginInfo.renderArea.extent.width = rect->width;
	renderPassBeginInfo.renderArea.extent.height = rect->height;
	renderPassBeginInfo.clearValueCount = ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED ) ? 2 : 1;
	renderPassBeginInfo.pClearValues = clearValues;

	VkSubpassContents contents = ( renderPass->type == GPU_RENDERPASS_TYPE_INLINE ) ?
									VK_SUBPASS_CONTENTS_INLINE :
									VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;

	VC( device->vkCmdBeginRenderPass( cmdBuffer, &renderPassBeginInfo, contents ) );

	commandBuffer->currentRenderPass = renderPass;
}

static void GpuCommandBuffer_EndRenderPass( GpuCommandBuffer_t * commandBuffer, GpuRenderPass_t * renderPass )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentRenderPass == renderPass );

	UNUSED_PARM( renderPass );

	GpuDevice_t * device = commandBuffer->context->device;

	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];

	VC( device->vkCmdEndRenderPass( cmdBuffer ) );

	commandBuffer->currentRenderPass = NULL;
}

static void GpuCommandBuffer_SetViewport( GpuCommandBuffer_t * commandBuffer, const ScreenRect_t * rect )
{
	GpuDevice_t * device = commandBuffer->context->device;

	VkViewport viewport;
	viewport.x = (float) rect->x;
	viewport.y = (float) rect->y;
	viewport.width = (float) rect->width;
	viewport.height = (float) rect->height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];
	VC( device->vkCmdSetViewport( cmdBuffer, 0, 1, &viewport ) );
}

static void GpuCommandBuffer_SetScissor( GpuCommandBuffer_t * commandBuffer, const ScreenRect_t * rect )
{
	GpuDevice_t * device = commandBuffer->context->device;

	VkRect2D scissor;
	scissor.offset.x = rect->x;
	scissor.offset.y = rect->y;
	scissor.extent.width = rect->width;
	scissor.extent.height = rect->height;

	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];
	VC( device->vkCmdSetScissor( cmdBuffer, 0, 1, &scissor ) );
}

static void GpuCommandBuffer_UpdateProgramParms( GpuCommandBuffer_t * commandBuffer,
												const GpuProgramParmLayout_t * newLayout,
												const GpuProgramParmLayout_t * oldLayout,
												const GpuProgramParmState_t * newParmState,
												const GpuProgramParmState_t * oldParmState,
												VkPipelineBindPoint bindPoint )
{
	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];
	GpuDevice_t * device = commandBuffer->context->device;

	const bool descriptorsMatch = GpuProgramParmState_DescriptorsMatch( newLayout, newParmState, oldLayout, oldParmState );
	if ( !descriptorsMatch )
	{
		// Try to find existing resources that match.
		GpuPipelineResources_t * resources = NULL;
		for ( GpuPipelineResources_t * r = commandBuffer->pipelineResources[commandBuffer->currentBuffer]; r != NULL; r = r->next )
		{
			if ( GpuProgramParmState_DescriptorsMatch( newLayout, newParmState, r->parmLayout, &r->parms ) )
			{
				r->unusedCount = 0;
				resources = r;
				break;
			}
		}

		// Create new resources if none were found.
		if ( resources == NULL )
		{
			resources = (GpuPipelineResources_t *) malloc( sizeof( GpuPipelineResources_t ) );
			GpuPipelineResources_Create( commandBuffer->context, resources, newLayout, newParmState );
			resources->next = commandBuffer->pipelineResources[commandBuffer->currentBuffer];
			commandBuffer->pipelineResources[commandBuffer->currentBuffer] = resources;	
		}

		VC( device->vkCmdBindDescriptorSets( cmdBuffer, bindPoint, newLayout->pipelineLayout,
										0, 1, &resources->descriptorSet, 0, NULL ) );
	}

	for ( int i = 0; i < newLayout->numPushConstants; i++ )
	{
		const void * data = GpuProgramParmState_NewPushConstantData( newLayout, i, newParmState, oldLayout, i, oldParmState, false );
		if ( data != NULL )
		{
			const GpuProgramParm_t * newParm = newLayout->pushConstants[i];
			const VkShaderStageFlags stageFlags = GpuProgramParm_GetShaderStageFlags( newParm->stage );
			const uint32_t offset = (uint32_t) newParm->binding;
			const uint32_t size = (uint32_t) GpuProgramParm_GetPushConstantSize( newParm->type );
			const void * data = newParmState->parms[newParm->index];
			VC( device->vkCmdPushConstants( cmdBuffer, newLayout->pipelineLayout, stageFlags, offset, size, data ) );
		}
	}
}

static void GpuCommandBuffer_SubmitGraphicsCommand( GpuCommandBuffer_t * commandBuffer, const GpuGraphicsCommand_t * command )
{
	assert( commandBuffer->currentRenderPass != NULL );

	GpuDevice_t * device = commandBuffer->context->device;

	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];
	const GpuGraphicsCommand_t * state = &commandBuffer->currentGraphicsState;

	// If the pipeline has changed.
	if ( command->pipeline != state->pipeline )
	{
		VC( device->vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, command->pipeline->pipeline ) );
	}

	const GpuProgramParmLayout_t * commandLayout = &command->pipeline->program->parmLayout;
	const GpuProgramParmLayout_t * stateLayout = ( state->pipeline != NULL ) ? &state->pipeline->program->parmLayout : NULL;

	GpuCommandBuffer_UpdateProgramParms( commandBuffer, commandLayout, stateLayout, &command->parmState, &state->parmState, VK_PIPELINE_BIND_POINT_GRAPHICS );

	const GpuGeometry_t * geometry = command->pipeline->geometry;

	// If the geometry has changed.
	if ( state->pipeline == NULL || geometry != state->pipeline->geometry || command->vertexBuffer != state->vertexBuffer || command->instanceBuffer != state->instanceBuffer )
	{
		const VkBuffer vertexBuffer = ( command->vertexBuffer != NULL ) ? command->vertexBuffer->buffer : geometry->vertexBuffer.buffer;
		for ( int i = 0; i < command->pipeline->firstInstanceBinding; i++ )
		{
			VC( device->vkCmdBindVertexBuffers( cmdBuffer, i, 1, &vertexBuffer, &command->pipeline->vertexBindingOffsets[i] ) );
		}

		const VkBuffer instanceBuffer = ( command->instanceBuffer != NULL ) ? command->instanceBuffer->buffer : geometry->instanceBuffer.buffer;
		for ( int i = command->pipeline->firstInstanceBinding; i < command->pipeline->vertexBindingCount; i++ )
		{
			VC( device->vkCmdBindVertexBuffers( cmdBuffer, i, 1, &instanceBuffer, &command->pipeline->vertexBindingOffsets[i] ) );
		}

		const VkIndexType indexType = ( sizeof( GpuTriangleIndex_t ) == sizeof( unsigned int ) ) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
		VC( device->vkCmdBindIndexBuffer( cmdBuffer, geometry->indexBuffer.buffer, 0, indexType ) );
	}

	VC( device->vkCmdDrawIndexed( cmdBuffer, geometry->indexCount, command->numInstances, 0, 0, 0 ) );

	commandBuffer->currentGraphicsState = *command;
}

static void GpuCommandBuffer_SubmitComputeCommand( GpuCommandBuffer_t * commandBuffer, const GpuComputeCommand_t * command )
{
	assert( commandBuffer->currentRenderPass == NULL );

	GpuDevice_t * device = commandBuffer->context->device;

	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];
	const GpuComputeCommand_t * state = &commandBuffer->currentComputeState;

	// If the pipeline has changed.
	if ( command->pipeline != state->pipeline )
	{
		VC( device->vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, command->pipeline->pipeline ) );
	}

	const GpuProgramParmLayout_t * commandLayout = &command->pipeline->program->parmLayout;
	const GpuProgramParmLayout_t * stateLayout = ( state->pipeline != NULL ) ? &state->pipeline->program->parmLayout : NULL;

	GpuCommandBuffer_UpdateProgramParms( commandBuffer, commandLayout, stateLayout, &command->parmState, &state->parmState, VK_PIPELINE_BIND_POINT_COMPUTE );

	VC( device->vkCmdDispatch( cmdBuffer, command->x, command->y, command->z ) );

	commandBuffer->currentComputeState = *command;
}

static GpuBuffer_t * GpuCommandBuffer_MapBuffer( GpuCommandBuffer_t * commandBuffer, GpuBuffer_t * buffer, void ** data )
{
	assert( commandBuffer->currentRenderPass == NULL );

	GpuDevice_t * device = commandBuffer->context->device;

	GpuBuffer_t * newBuffer = NULL;
	for ( GpuBuffer_t ** b = &commandBuffer->oldMappedBuffers[commandBuffer->currentBuffer]; *b != NULL; b = &(*b)->next )
	{
		if ( (*b)->size == buffer->size && (*b)->type == buffer->type )
		{
			newBuffer = *b;
			*b = (*b)->next;
			break;
		}
	}
	if ( newBuffer == NULL )
	{
		newBuffer = (GpuBuffer_t *) malloc( sizeof( GpuBuffer_t ) );
		GpuBuffer_Create( commandBuffer->context, newBuffer, buffer->type, buffer->size, NULL, true );
	}

	newBuffer->unusedCount = 0;
	newBuffer->next = commandBuffer->mappedBuffers[commandBuffer->currentBuffer];
	commandBuffer->mappedBuffers[commandBuffer->currentBuffer] = newBuffer;

	assert( newBuffer->mapped == NULL );
	VK( device->vkMapMemory( commandBuffer->context->device->device, newBuffer->memory, 0, newBuffer->size, 0, &newBuffer->mapped ) );

	*data = newBuffer->mapped;

	return newBuffer;
}

static void GpuCommandBuffer_UnmapBuffer( GpuCommandBuffer_t * commandBuffer, GpuBuffer_t * buffer, GpuBuffer_t * mappedBuffer, const GpuBufferUnmapType_t type )
{
	// Can only copy or issue memory barrier outside a render pass.
	assert( commandBuffer->currentRenderPass == NULL );

	GpuDevice_t * device = commandBuffer->context->device;

	VC( device->vkUnmapMemory( commandBuffer->context->device->device, mappedBuffer->memory ) );
	mappedBuffer->mapped = NULL;

	// Optionally copy the mapped buffer back to the original buffer. While the copy is not for free,
	// there may be a performance benefit from using the original buffer if it lives in device local memory.
	if ( type == GPU_BUFFER_UNMAP_TYPE_COPY_BACK )
	{
		assert( buffer->size == mappedBuffer->size );

		{
			// Add a memory barrier for the mapped buffer from host write to DMA read.
			VkBufferMemoryBarrier bufferMemoryBarrier;
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.pNext = NULL;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.buffer = mappedBuffer->buffer;
			bufferMemoryBarrier.offset = 0;
			bufferMemoryBarrier.size = mappedBuffer->size;

			const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			const VkDependencyFlags flags = 0;

			VC( device->vkCmdPipelineBarrier( commandBuffer->cmdBuffers[commandBuffer->currentBuffer],
												src_stages, dst_stages, flags, 0, NULL, 1, &bufferMemoryBarrier, 0, NULL ) );
		}

		{
			// Copy back to the original buffer.
			VkBufferCopy bufferCopy;
			bufferCopy.srcOffset = 0;
			bufferCopy.dstOffset = 0;
			bufferCopy.size = buffer->size;

			VC( device->vkCmdCopyBuffer( commandBuffer->cmdBuffers[commandBuffer->currentBuffer], mappedBuffer->buffer, buffer->buffer, 1, &bufferCopy ) );
		}

		{
			// Add a memory barrier for the original buffer from DMA write to the buffer access.
			VkBufferMemoryBarrier bufferMemoryBarrier;
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.pNext = NULL;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = GpuBuffer_GetBufferAccess( buffer->type );
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.buffer = buffer->buffer;
			bufferMemoryBarrier.offset = 0;
			bufferMemoryBarrier.size = buffer->size;

			const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			const VkDependencyFlags flags = 0;

			VC( device->vkCmdPipelineBarrier( commandBuffer->cmdBuffers[commandBuffer->currentBuffer],
												src_stages, dst_stages, flags, 0, NULL, 1, &bufferMemoryBarrier, 0, NULL ) );
		}
	}
	else
	{
		{
			// Add a memory barrier for the mapped buffer from host write to buffer access.
			VkBufferMemoryBarrier bufferMemoryBarrier;
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.pNext = NULL;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = GpuBuffer_GetBufferAccess( mappedBuffer->type );
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.buffer = mappedBuffer->buffer;
			bufferMemoryBarrier.offset = 0;
			bufferMemoryBarrier.size = mappedBuffer->size;

			const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			const VkDependencyFlags flags = 0;

			VC( device->vkCmdPipelineBarrier( commandBuffer->cmdBuffers[commandBuffer->currentBuffer],
												src_stages, dst_stages, flags, 0, NULL, 1, &bufferMemoryBarrier, 0, NULL ) );
		}
	}
}

static GpuBuffer_t * GpuCommandBuffer_MapVertexAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuVertexAttributeArrays_t * attribs )
{
	void * data = NULL;
	GpuBuffer_t * buffer = GpuCommandBuffer_MapBuffer( commandBuffer, &geometry->vertexBuffer, &data );

	GpuVertexAttributeArrays_Map( attribs, data, buffer->size, geometry->vertexCount, geometry->vertexAttribsFlags );

	return buffer;
}

static void GpuCommandBuffer_UnmapVertexAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuBuffer_t * mappedVertexBuffer, const GpuBufferUnmapType_t type )
{
	GpuCommandBuffer_UnmapBuffer( commandBuffer, &geometry->vertexBuffer, mappedVertexBuffer, type );
}

static GpuBuffer_t * GpuCommandBuffer_MapInstanceAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuVertexAttributeArrays_t * attribs )
{
	void * data = NULL;
	GpuBuffer_t * buffer = GpuCommandBuffer_MapBuffer( commandBuffer, &geometry->instanceBuffer, &data );

	GpuVertexAttributeArrays_Map( attribs, data, buffer->size, geometry->instanceCount, geometry->instanceAttribsFlags );

	return buffer;
}

static void GpuCommandBuffer_UnmapInstanceAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuBuffer_t * mappedInstanceBuffer, const GpuBufferUnmapType_t type )
{
	GpuCommandBuffer_UnmapBuffer( commandBuffer, &geometry->instanceBuffer, mappedInstanceBuffer, type );
}

/*
================================================================================================================================

Bar graph.

Real-time bar graph where new bars scroll in on the right and old bars scroll out on the left.
Optionally supports stacking of bars. A bar value is in the range [0, 1] where 1 is a full height bar.
The bar graph position x,y,width,height is specified in clip coordinates in the range [-1, 1].

BarGraph_t

static void BarGraph_Create( GpuContext_t * context, BarGraph_t * barGraph, GpuRenderPass_t * renderPass,
								const float x, const float y, const float width, const float height,
								const int numBars, const int numStacked, const Vector4f_t * backgroundColor );
static void BarGraph_Destroy( GpuContext_t * context, BarGraph_t * barGraph );
static void BarGraph_AddBar( BarGraph_t * barGraph, const int stackedBar, const float value, const Vector4f_t * color, const bool advance );

static void BarGraph_UpdateGraphics( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph );
static void BarGraph_RenderGraphics( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph );

static void BarGraph_UpdateCompute( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph );
static void BarGraph_RenderCompute( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph, GpuFramebuffer_t * framebuffer );

================================================================================================================================
*/

typedef struct
{
	ClipRect_t			clipRect;
	int					numBars;
	int					numStacked;
	int					barIndex;
	float *				barValues;
	Vector4f_t *		barColors;
	Vector4f_t			backgroundColor;
	struct
	{
		GpuGeometry_t			quad;
		GpuGraphicsProgram_t	program;
		GpuGraphicsPipeline_t	pipeline;
		int						numInstances;
	} graphics;
	struct
	{
		GpuBuffer_t				barValueBuffer;
		GpuBuffer_t				barColorBuffer;
		Vector2i_t				barGraphOffset;
		GpuComputeProgram_t		program;
		GpuComputePipeline_t	pipeline;
	} compute;
} BarGraph_t;

static const GpuProgramParm_t barGraphGraphicsProgramParms[] =
{
	{ 0 }
};

static const char barGraphVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( location = 0 ) in vec3 vertexPosition;\n"
	"layout( location = 1 ) in mat4 vertexTransform;\n"
	"layout( location = 0 ) out vec4 fragmentColor;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"vec3 multiply3x4( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[1].x * v.y + m[2].x * v.z + m[3].x,\n"
	"		m[0].y * v.x + m[1].y * v.y + m[2].y * v.z + m[3].y,\n"
	"		m[0].z * v.x + m[1].z * v.y + m[2].z * v.z + m[3].z );\n"
	"}\n"
	"void main( void )\n"
	"{\n"
	"	gl_Position.xyz = multiply3x4( vertexTransform, vertexPosition );\n"
	"	gl_Position.w = 1.0;\n"
	"	fragmentColor.r = vertexTransform[0][3];\n"
	"	fragmentColor.g = vertexTransform[1][3];\n"
	"	fragmentColor.b = vertexTransform[2][3];\n"
	"	fragmentColor.a = vertexTransform[3][3];\n"
	"}\n";

static const unsigned int barGraphVertexProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x0000007c,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0009000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000005c,0x0000005e,0x00000060,
	0x0000006e,0x00030003,0x00000002,0x000001b8,0x00040005,0x00000004,0x6e69616d,0x00000000,
	0x00080005,0x0000000f,0x746c756d,0x796c7069,0x28347833,0x3434666d,0x3366763b,0x0000003b,
	0x00030005,0x0000000d,0x0000006d,0x00030005,0x0000000e,0x00000076,0x00060005,0x0000005a,
	0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x0000005a,0x00000000,0x505f6c67,
	0x7469736f,0x006e6f69,0x00030005,0x0000005c,0x00000000,0x00060005,0x0000005e,0x74726576,
	0x72547865,0x66736e61,0x006d726f,0x00060005,0x00000060,0x74726576,0x6f507865,0x69746973,
	0x00006e6f,0x00040005,0x00000061,0x61726170,0x0000006d,0x00040005,0x00000063,0x61726170,
	0x0000006d,0x00060005,0x0000006e,0x67617266,0x746e656d,0x6f6c6f43,0x00000072,0x00050048,
	0x0000005a,0x00000000,0x0000000b,0x00000000,0x00030047,0x0000005a,0x00000002,0x00040047,
	0x0000005e,0x0000001e,0x00000001,0x00040047,0x00000060,0x0000001e,0x00000000,0x00040047,
	0x0000006e,0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
	0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040018,
	0x00000008,0x00000007,0x00000004,0x00040020,0x00000009,0x00000007,0x00000008,0x00040017,
	0x0000000a,0x00000006,0x00000003,0x00040020,0x0000000b,0x00000007,0x0000000a,0x00050021,
	0x0000000c,0x0000000a,0x00000009,0x0000000b,0x00040015,0x00000011,0x00000020,0x00000001,
	0x0004002b,0x00000011,0x00000012,0x00000000,0x00040015,0x00000013,0x00000020,0x00000000,
	0x0004002b,0x00000013,0x00000014,0x00000000,0x00040020,0x00000015,0x00000007,0x00000006,
	0x0004002b,0x00000011,0x0000001b,0x00000001,0x0004002b,0x00000013,0x0000001e,0x00000001,
	0x0004002b,0x00000011,0x00000023,0x00000002,0x0004002b,0x00000013,0x00000026,0x00000002,
	0x0004002b,0x00000011,0x0000002b,0x00000003,0x0003001e,0x0000005a,0x00000007,0x00040020,
	0x0000005b,0x00000003,0x0000005a,0x0004003b,0x0000005b,0x0000005c,0x00000003,0x00040020,
	0x0000005d,0x00000001,0x00000008,0x0004003b,0x0000005d,0x0000005e,0x00000001,0x00040020,
	0x0000005f,0x00000001,0x0000000a,0x0004003b,0x0000005f,0x00000060,0x00000001,0x00040020,
	0x00000066,0x00000003,0x00000007,0x0004002b,0x00000006,0x0000006a,0x3f800000,0x0004002b,
	0x00000013,0x0000006b,0x00000003,0x00040020,0x0000006c,0x00000003,0x00000006,0x0004003b,
	0x00000066,0x0000006e,0x00000003,0x00040020,0x0000006f,0x00000001,0x00000006,0x00050036,
	0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000009,
	0x00000061,0x00000007,0x0004003b,0x0000000b,0x00000063,0x00000007,0x0004003d,0x00000008,
	0x00000062,0x0000005e,0x0003003e,0x00000061,0x00000062,0x0004003d,0x0000000a,0x00000064,
	0x00000060,0x0003003e,0x00000063,0x00000064,0x00060039,0x0000000a,0x00000065,0x0000000f,
	0x00000061,0x00000063,0x00050041,0x00000066,0x00000067,0x0000005c,0x00000012,0x0004003d,
	0x00000007,0x00000068,0x00000067,0x0009004f,0x00000007,0x00000069,0x00000068,0x00000065,
	0x00000004,0x00000005,0x00000006,0x00000003,0x0003003e,0x00000067,0x00000069,0x00060041,
	0x0000006c,0x0000006d,0x0000005c,0x00000012,0x0000006b,0x0003003e,0x0000006d,0x0000006a,
	0x00060041,0x0000006f,0x00000070,0x0000005e,0x00000012,0x0000006b,0x0004003d,0x00000006,
	0x00000071,0x00000070,0x00050041,0x0000006c,0x00000072,0x0000006e,0x00000014,0x0003003e,
	0x00000072,0x00000071,0x00060041,0x0000006f,0x00000073,0x0000005e,0x0000001b,0x0000006b,
	0x0004003d,0x00000006,0x00000074,0x00000073,0x00050041,0x0000006c,0x00000075,0x0000006e,
	0x0000001e,0x0003003e,0x00000075,0x00000074,0x00060041,0x0000006f,0x00000076,0x0000005e,
	0x00000023,0x0000006b,0x0004003d,0x00000006,0x00000077,0x00000076,0x00050041,0x0000006c,
	0x00000078,0x0000006e,0x00000026,0x0003003e,0x00000078,0x00000077,0x00060041,0x0000006f,
	0x00000079,0x0000005e,0x0000002b,0x0000006b,0x0004003d,0x00000006,0x0000007a,0x00000079,
	0x00050041,0x0000006c,0x0000007b,0x0000006e,0x0000006b,0x0003003e,0x0000007b,0x0000007a,
	0x000100fd,0x00010038,0x00050036,0x0000000a,0x0000000f,0x00000000,0x0000000c,0x00030037,
	0x00000009,0x0000000d,0x00030037,0x0000000b,0x0000000e,0x000200f8,0x00000010,0x00060041,
	0x00000015,0x00000016,0x0000000d,0x00000012,0x00000014,0x0004003d,0x00000006,0x00000017,
	0x00000016,0x00050041,0x00000015,0x00000018,0x0000000e,0x00000014,0x0004003d,0x00000006,
	0x00000019,0x00000018,0x00050085,0x00000006,0x0000001a,0x00000017,0x00000019,0x00060041,
	0x00000015,0x0000001c,0x0000000d,0x0000001b,0x00000014,0x0004003d,0x00000006,0x0000001d,
	0x0000001c,0x00050041,0x00000015,0x0000001f,0x0000000e,0x0000001e,0x0004003d,0x00000006,
	0x00000020,0x0000001f,0x00050085,0x00000006,0x00000021,0x0000001d,0x00000020,0x00050081,
	0x00000006,0x00000022,0x0000001a,0x00000021,0x00060041,0x00000015,0x00000024,0x0000000d,
	0x00000023,0x00000014,0x0004003d,0x00000006,0x00000025,0x00000024,0x00050041,0x00000015,
	0x00000027,0x0000000e,0x00000026,0x0004003d,0x00000006,0x00000028,0x00000027,0x00050085,
	0x00000006,0x00000029,0x00000025,0x00000028,0x00050081,0x00000006,0x0000002a,0x00000022,
	0x00000029,0x00060041,0x00000015,0x0000002c,0x0000000d,0x0000002b,0x00000014,0x0004003d,
	0x00000006,0x0000002d,0x0000002c,0x00050081,0x00000006,0x0000002e,0x0000002a,0x0000002d,
	0x00060041,0x00000015,0x0000002f,0x0000000d,0x00000012,0x0000001e,0x0004003d,0x00000006,
	0x00000030,0x0000002f,0x00050041,0x00000015,0x00000031,0x0000000e,0x00000014,0x0004003d,
	0x00000006,0x00000032,0x00000031,0x00050085,0x00000006,0x00000033,0x00000030,0x00000032,
	0x00060041,0x00000015,0x00000034,0x0000000d,0x0000001b,0x0000001e,0x0004003d,0x00000006,
	0x00000035,0x00000034,0x00050041,0x00000015,0x00000036,0x0000000e,0x0000001e,0x0004003d,
	0x00000006,0x00000037,0x00000036,0x00050085,0x00000006,0x00000038,0x00000035,0x00000037,
	0x00050081,0x00000006,0x00000039,0x00000033,0x00000038,0x00060041,0x00000015,0x0000003a,
	0x0000000d,0x00000023,0x0000001e,0x0004003d,0x00000006,0x0000003b,0x0000003a,0x00050041,
	0x00000015,0x0000003c,0x0000000e,0x00000026,0x0004003d,0x00000006,0x0000003d,0x0000003c,
	0x00050085,0x00000006,0x0000003e,0x0000003b,0x0000003d,0x00050081,0x00000006,0x0000003f,
	0x00000039,0x0000003e,0x00060041,0x00000015,0x00000040,0x0000000d,0x0000002b,0x0000001e,
	0x0004003d,0x00000006,0x00000041,0x00000040,0x00050081,0x00000006,0x00000042,0x0000003f,
	0x00000041,0x00060041,0x00000015,0x00000043,0x0000000d,0x00000012,0x00000026,0x0004003d,
	0x00000006,0x00000044,0x00000043,0x00050041,0x00000015,0x00000045,0x0000000e,0x00000014,
	0x0004003d,0x00000006,0x00000046,0x00000045,0x00050085,0x00000006,0x00000047,0x00000044,
	0x00000046,0x00060041,0x00000015,0x00000048,0x0000000d,0x0000001b,0x00000026,0x0004003d,
	0x00000006,0x00000049,0x00000048,0x00050041,0x00000015,0x0000004a,0x0000000e,0x0000001e,
	0x0004003d,0x00000006,0x0000004b,0x0000004a,0x00050085,0x00000006,0x0000004c,0x00000049,
	0x0000004b,0x00050081,0x00000006,0x0000004d,0x00000047,0x0000004c,0x00060041,0x00000015,
	0x0000004e,0x0000000d,0x00000023,0x00000026,0x0004003d,0x00000006,0x0000004f,0x0000004e,
	0x00050041,0x00000015,0x00000050,0x0000000e,0x00000026,0x0004003d,0x00000006,0x00000051,
	0x00000050,0x00050085,0x00000006,0x00000052,0x0000004f,0x00000051,0x00050081,0x00000006,
	0x00000053,0x0000004d,0x00000052,0x00060041,0x00000015,0x00000054,0x0000000d,0x0000002b,
	0x00000026,0x0004003d,0x00000006,0x00000055,0x00000054,0x00050081,0x00000006,0x00000056,
	0x00000053,0x00000055,0x00060050,0x0000000a,0x00000057,0x0000002e,0x00000042,0x00000056,
	0x000200fe,0x00000057,0x00010038
};

static const char barGraphFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( location = 0 ) in lowp vec4 fragmentColor;\n"
	"layout( location = 0 ) out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	outColor = fragmentColor;\n"
	"}\n";

static const unsigned int barGraphFragmentProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x0000000d,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000b,0x00030010,
	0x00000004,0x00000007,0x00030003,0x00000002,0x000001b8,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00050005,0x00000009,0x4374756f,0x726f6c6f,0x00000000,0x00060005,0x0000000b,
	0x67617266,0x746e656d,0x6f6c6f43,0x00000072,0x00040047,0x00000009,0x0000001e,0x00000000,
	0x00040047,0x0000000b,0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,
	0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,
	0x00040020,0x00000008,0x00000003,0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,
	0x00040020,0x0000000a,0x00000001,0x00000007,0x0004003b,0x0000000a,0x0000000b,0x00000001,
	0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003d,
	0x00000007,0x0000000c,0x0000000b,0x0003003e,0x00000009,0x0000000c,0x000100fd,0x00010038
};

enum
{
	COMPUTE_PROGRAM_TEXTURE_BAR_GRAPH_DEST,
	COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_VALUES,
	COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_COLORS,
	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_BARS,
	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_STACKED,
	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_INDEX,
	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_GRAPH_OFFSET,
	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BACK_GROUND_COLOR
};

static const GpuProgramParm_t barGraphComputeProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,				GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_BAR_GRAPH_DEST,					"dest",				0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_VALUES,			"barValueBuffer",	1 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_COLORS,			"barColorBuffer",	2 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BACK_GROUND_COLOR,	"backgroundColor",	0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_GRAPH_OFFSET,		"barGraphOffset",	16 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_BARS,				"numBars",			24 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_STACKED,			"numStacked",		28 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_INDEX,			"barIndex",			32 }
};

#define BARGRAPH_LOCAL_SIZE_X	8
#define BARGRAPH_LOCAL_SIZE_Y	8

static const char barGraphComputeProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"\n"
	"layout( local_size_x = " STRINGIFY( BARGRAPH_LOCAL_SIZE_X ) ", local_size_y = " STRINGIFY( BARGRAPH_LOCAL_SIZE_Y ) " ) in;\n"
	"\n"
	"layout( rgba8, binding = 0 ) uniform writeonly image2D dest;\n"
	"layout( std430, binding = 1 ) buffer barValueBuffer { float barValues[]; };\n"
	"layout( std430, binding = 2 ) buffer barColorBuffer { vec4 barColors[]; };\n"
	"layout( std140, push_constant ) uniform buffer0\n"
	"{\n"
	"	layout( offset =  0 ) lowp vec4 backgroundColor;\n"
	"	layout( offset = 16 ) ivec2 barGraphOffset;\n"
	"	layout( offset = 24 ) int numBars;\n"
	"	layout( offset = 28 ) int numStacked;\n"
	"	layout( offset = 32 ) int barIndex;\n"
	"} pc;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	ivec2 barGraph = ivec2( gl_GlobalInvocationID.xy );\n"
	"	ivec2 barGraphSize = ivec2( gl_NumWorkGroups.xy * gl_WorkGroupSize.xy );\n"
	"\n"
	"	int index = barGraph.x * pc.numBars / barGraphSize.x;\n"
	"	int barOffset = ( ( pc.barIndex + index ) % pc.numBars ) * pc.numStacked;\n"
	"	float barColorScale = ( ( index & 1 ) != 0 ) ? 0.75f : 1.0f;\n"
	"\n"
	"	vec4 rgba = pc.backgroundColor;\n"
	"	float localY = float( barGraph.y );\n"
	"	float stackedBarValue = 0.0f;\n"
	"	for ( int i = 0; i < pc.numStacked; i++ )\n"
	"	{\n"
	"		stackedBarValue += barValues[barOffset + i];\n"
	"		if ( localY < stackedBarValue * float( barGraphSize.y ) )\n"
	"		{\n"
	"			rgba = barColors[barOffset + i] * barColorScale;\n"
	"			break;\n"
	"		}\n"
	"	}\n"
	"\n"
	"	imageStore( dest, pc.barGraphOffset + ivec2( barGraph.x, -barGraph.y ), rgba );\n"
	"}\n";

static const unsigned int barGraphComputeProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x00000092,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000005,0x00000004,0x6e69616d,0x00000000,0x0000000d,0x00000013,0x00060010,
	0x00000004,0x00000011,0x00000008,0x00000008,0x00000001,0x00030003,0x00000002,0x000001b8,
	0x00040005,0x00000004,0x6e69616d,0x00000000,0x00050005,0x00000009,0x47726162,0x68706172,
	0x00000000,0x00080005,0x0000000d,0x475f6c67,0x61626f6c,0x766e496c,0x7461636f,0x496e6f69,
	0x00000044,0x00060005,0x00000012,0x47726162,0x68706172,0x657a6953,0x00000000,0x00070005,
	0x00000013,0x4e5f6c67,0x6f576d75,0x72476b72,0x7370756f,0x00000000,0x00040005,0x0000001b,
	0x65646e69,0x00000078,0x00040005,0x00000021,0x66667562,0x00307265,0x00070006,0x00000021,
	0x00000000,0x6b636162,0x756f7267,0x6f43646e,0x00726f6c,0x00070006,0x00000021,0x00000001,
	0x47726162,0x68706172,0x7366664f,0x00007465,0x00050006,0x00000021,0x00000002,0x426d756e,
	0x00737261,0x00060006,0x00000021,0x00000003,0x536d756e,0x6b636174,0x00006465,0x00060006,
	0x00000021,0x00000004,0x49726162,0x7865646e,0x00000000,0x00030005,0x00000023,0x00006370,
	0x00050005,0x0000002c,0x4f726162,0x65736666,0x00000074,0x00060005,0x0000003a,0x43726162,
	0x726f6c6f,0x6c616353,0x00000065,0x00040005,0x00000049,0x61626772,0x00000000,0x00040005,
	0x0000004d,0x61636f6c,0x0000596c,0x00060005,0x00000052,0x63617473,0x4264656b,0x61567261,
	0x0065756c,0x00030005,0x00000054,0x00000069,0x00060005,0x0000005f,0x56726162,0x65756c61,
	0x66667542,0x00007265,0x00060006,0x0000005f,0x00000000,0x56726162,0x65756c61,0x00000073,
	0x00030005,0x00000061,0x00000000,0x00060005,0x00000074,0x43726162,0x726f6c6f,0x66667542,
	0x00007265,0x00060006,0x00000074,0x00000000,0x43726162,0x726f6c6f,0x00000073,0x00030005,
	0x00000076,0x00000000,0x00040005,0x00000084,0x74736564,0x00000000,0x00040047,0x0000000d,
	0x0000000b,0x0000001c,0x00040047,0x00000013,0x0000000b,0x00000018,0x00050048,0x00000021,
	0x00000000,0x00000023,0x00000000,0x00050048,0x00000021,0x00000001,0x00000023,0x00000010,
	0x00050048,0x00000021,0x00000002,0x00000023,0x00000018,0x00050048,0x00000021,0x00000003,
	0x00000023,0x0000001c,0x00050048,0x00000021,0x00000004,0x00000023,0x00000020,0x00030047,
	0x00000021,0x00000002,0x00040047,0x00000023,0x00000022,0x00000000,0x00040047,0x0000005e,
	0x00000006,0x00000004,0x00050048,0x0000005f,0x00000000,0x00000023,0x00000000,0x00030047,
	0x0000005f,0x00000003,0x00040047,0x00000061,0x00000022,0x00000000,0x00040047,0x00000061,
	0x00000021,0x00000001,0x00040047,0x00000073,0x00000006,0x00000010,0x00050048,0x00000074,
	0x00000000,0x00000023,0x00000000,0x00030047,0x00000074,0x00000003,0x00040047,0x00000076,
	0x00000022,0x00000000,0x00040047,0x00000076,0x00000021,0x00000002,0x00040047,0x00000084,
	0x00000022,0x00000000,0x00040047,0x00000084,0x00000021,0x00000000,0x00040047,0x00000091,
	0x0000000b,0x00000019,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00040015,
	0x00000006,0x00000020,0x00000001,0x00040017,0x00000007,0x00000006,0x00000002,0x00040020,
	0x00000008,0x00000007,0x00000007,0x00040015,0x0000000a,0x00000020,0x00000000,0x00040017,
	0x0000000b,0x0000000a,0x00000003,0x00040020,0x0000000c,0x00000001,0x0000000b,0x0004003b,
	0x0000000c,0x0000000d,0x00000001,0x00040017,0x0000000e,0x0000000a,0x00000002,0x0004003b,
	0x0000000c,0x00000013,0x00000001,0x0004002b,0x0000000a,0x00000016,0x00000008,0x0005002c,
	0x0000000e,0x00000017,0x00000016,0x00000016,0x00040020,0x0000001a,0x00000007,0x00000006,
	0x0004002b,0x0000000a,0x0000001c,0x00000000,0x00030016,0x0000001f,0x00000020,0x00040017,
	0x00000020,0x0000001f,0x00000004,0x0007001e,0x00000021,0x00000020,0x00000007,0x00000006,
	0x00000006,0x00000006,0x00040020,0x00000022,0x00000009,0x00000021,0x0004003b,0x00000022,
	0x00000023,0x00000009,0x0004002b,0x00000006,0x00000024,0x00000002,0x00040020,0x00000025,
	0x00000009,0x00000006,0x0004002b,0x00000006,0x0000002d,0x00000004,0x0004002b,0x00000006,
	0x00000035,0x00000003,0x00040020,0x00000039,0x00000007,0x0000001f,0x0004002b,0x00000006,
	0x0000003d,0x00000001,0x0004002b,0x00000006,0x0000003f,0x00000000,0x00020014,0x00000040,
	0x0004002b,0x0000001f,0x00000044,0x3f400000,0x0004002b,0x0000001f,0x00000046,0x3f800000,
	0x00040020,0x00000048,0x00000007,0x00000020,0x00040020,0x0000004a,0x00000009,0x00000020,
	0x0004002b,0x0000000a,0x0000004e,0x00000001,0x0004002b,0x0000001f,0x00000053,0x00000000,
	0x0003001d,0x0000005e,0x0000001f,0x0003001e,0x0000005f,0x0000005e,0x00040020,0x00000060,
	0x00000002,0x0000005f,0x0004003b,0x00000060,0x00000061,0x00000002,0x00040020,0x00000065,
	0x00000002,0x0000001f,0x0003001d,0x00000073,0x00000020,0x0003001e,0x00000074,0x00000073,
	0x00040020,0x00000075,0x00000002,0x00000074,0x0004003b,0x00000075,0x00000076,0x00000002,
	0x00040020,0x0000007a,0x00000002,0x00000020,0x00090019,0x00000082,0x0000001f,0x00000001,
	0x00000000,0x00000000,0x00000000,0x00000002,0x00000004,0x00040020,0x00000083,0x00000000,
	0x00000082,0x0004003b,0x00000083,0x00000084,0x00000000,0x00040020,0x00000086,0x00000009,
	0x00000007,0x0006002c,0x0000000b,0x00000091,0x00000016,0x00000016,0x0000004e,0x00050036,
	0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000008,
	0x00000009,0x00000007,0x0004003b,0x00000008,0x00000012,0x00000007,0x0004003b,0x0000001a,
	0x0000001b,0x00000007,0x0004003b,0x0000001a,0x0000002c,0x00000007,0x0004003b,0x00000039,
	0x0000003a,0x00000007,0x0004003b,0x00000039,0x0000003b,0x00000007,0x0004003b,0x00000048,
	0x00000049,0x00000007,0x0004003b,0x00000039,0x0000004d,0x00000007,0x0004003b,0x00000039,
	0x00000052,0x00000007,0x0004003b,0x0000001a,0x00000054,0x00000007,0x0004003d,0x0000000b,
	0x0000000f,0x0000000d,0x0007004f,0x0000000e,0x00000010,0x0000000f,0x0000000f,0x00000000,
	0x00000001,0x0004007c,0x00000007,0x00000011,0x00000010,0x0003003e,0x00000009,0x00000011,
	0x0004003d,0x0000000b,0x00000014,0x00000013,0x0007004f,0x0000000e,0x00000015,0x00000014,
	0x00000014,0x00000000,0x00000001,0x00050084,0x0000000e,0x00000018,0x00000015,0x00000017,
	0x0004007c,0x00000007,0x00000019,0x00000018,0x0003003e,0x00000012,0x00000019,0x00050041,
	0x0000001a,0x0000001d,0x00000009,0x0000001c,0x0004003d,0x00000006,0x0000001e,0x0000001d,
	0x00050041,0x00000025,0x00000026,0x00000023,0x00000024,0x0004003d,0x00000006,0x00000027,
	0x00000026,0x00050084,0x00000006,0x00000028,0x0000001e,0x00000027,0x00050041,0x0000001a,
	0x00000029,0x00000012,0x0000001c,0x0004003d,0x00000006,0x0000002a,0x00000029,0x00050087,
	0x00000006,0x0000002b,0x00000028,0x0000002a,0x0003003e,0x0000001b,0x0000002b,0x00050041,
	0x00000025,0x0000002e,0x00000023,0x0000002d,0x0004003d,0x00000006,0x0000002f,0x0000002e,
	0x0004003d,0x00000006,0x00000030,0x0000001b,0x00050080,0x00000006,0x00000031,0x0000002f,
	0x00000030,0x00050041,0x00000025,0x00000032,0x00000023,0x00000024,0x0004003d,0x00000006,
	0x00000033,0x00000032,0x0005008b,0x00000006,0x00000034,0x00000031,0x00000033,0x00050041,
	0x00000025,0x00000036,0x00000023,0x00000035,0x0004003d,0x00000006,0x00000037,0x00000036,
	0x00050084,0x00000006,0x00000038,0x00000034,0x00000037,0x0003003e,0x0000002c,0x00000038,
	0x0004003d,0x00000006,0x0000003c,0x0000001b,0x000500c7,0x00000006,0x0000003e,0x0000003c,
	0x0000003d,0x000500ab,0x00000040,0x00000041,0x0000003e,0x0000003f,0x000300f7,0x00000043,
	0x00000000,0x000400fa,0x00000041,0x00000042,0x00000045,0x000200f8,0x00000042,0x0003003e,
	0x0000003b,0x00000044,0x000200f9,0x00000043,0x000200f8,0x00000045,0x0003003e,0x0000003b,
	0x00000046,0x000200f9,0x00000043,0x000200f8,0x00000043,0x0004003d,0x0000001f,0x00000047,
	0x0000003b,0x0003003e,0x0000003a,0x00000047,0x00050041,0x0000004a,0x0000004b,0x00000023,
	0x0000003f,0x0004003d,0x00000020,0x0000004c,0x0000004b,0x0003003e,0x00000049,0x0000004c,
	0x00050041,0x0000001a,0x0000004f,0x00000009,0x0000004e,0x0004003d,0x00000006,0x00000050,
	0x0000004f,0x0004006f,0x0000001f,0x00000051,0x00000050,0x0003003e,0x0000004d,0x00000051,
	0x0003003e,0x00000052,0x00000053,0x0003003e,0x00000054,0x0000003f,0x000200f9,0x00000055,
	0x000200f8,0x00000055,0x000400f6,0x00000057,0x00000058,0x00000000,0x000200f9,0x00000059,
	0x000200f8,0x00000059,0x0004003d,0x00000006,0x0000005a,0x00000054,0x00050041,0x00000025,
	0x0000005b,0x00000023,0x00000035,0x0004003d,0x00000006,0x0000005c,0x0000005b,0x000500b1,
	0x00000040,0x0000005d,0x0000005a,0x0000005c,0x000400fa,0x0000005d,0x00000056,0x00000057,
	0x000200f8,0x00000056,0x0004003d,0x00000006,0x00000062,0x0000002c,0x0004003d,0x00000006,
	0x00000063,0x00000054,0x00050080,0x00000006,0x00000064,0x00000062,0x00000063,0x00060041,
	0x00000065,0x00000066,0x00000061,0x0000003f,0x00000064,0x0004003d,0x0000001f,0x00000067,
	0x00000066,0x0004003d,0x0000001f,0x00000068,0x00000052,0x00050081,0x0000001f,0x00000069,
	0x00000068,0x00000067,0x0003003e,0x00000052,0x00000069,0x0004003d,0x0000001f,0x0000006a,
	0x0000004d,0x0004003d,0x0000001f,0x0000006b,0x00000052,0x00050041,0x0000001a,0x0000006c,
	0x00000012,0x0000004e,0x0004003d,0x00000006,0x0000006d,0x0000006c,0x0004006f,0x0000001f,
	0x0000006e,0x0000006d,0x00050085,0x0000001f,0x0000006f,0x0000006b,0x0000006e,0x000500b8,
	0x00000040,0x00000070,0x0000006a,0x0000006f,0x000300f7,0x00000072,0x00000000,0x000400fa,
	0x00000070,0x00000071,0x00000072,0x000200f8,0x00000071,0x0004003d,0x00000006,0x00000077,
	0x0000002c,0x0004003d,0x00000006,0x00000078,0x00000054,0x00050080,0x00000006,0x00000079,
	0x00000077,0x00000078,0x00060041,0x0000007a,0x0000007b,0x00000076,0x0000003f,0x00000079,
	0x0004003d,0x00000020,0x0000007c,0x0000007b,0x0004003d,0x0000001f,0x0000007d,0x0000003a,
	0x0005008e,0x00000020,0x0000007e,0x0000007c,0x0000007d,0x0003003e,0x00000049,0x0000007e,
	0x000200f9,0x00000057,0x000200f8,0x00000072,0x000200f9,0x00000058,0x000200f8,0x00000058,
	0x0004003d,0x00000006,0x00000080,0x00000054,0x00050080,0x00000006,0x00000081,0x00000080,
	0x0000003d,0x0003003e,0x00000054,0x00000081,0x000200f9,0x00000055,0x000200f8,0x00000057,
	0x0004003d,0x00000082,0x00000085,0x00000084,0x00050041,0x00000086,0x00000087,0x00000023,
	0x0000003d,0x0004003d,0x00000007,0x00000088,0x00000087,0x00050041,0x0000001a,0x00000089,
	0x00000009,0x0000001c,0x0004003d,0x00000006,0x0000008a,0x00000089,0x00050041,0x0000001a,
	0x0000008b,0x00000009,0x0000004e,0x0004003d,0x00000006,0x0000008c,0x0000008b,0x0004007e,
	0x00000006,0x0000008d,0x0000008c,0x00050050,0x00000007,0x0000008e,0x0000008a,0x0000008d,
	0x00050080,0x00000007,0x0000008f,0x00000088,0x0000008e,0x0004003d,0x00000020,0x00000090,
	0x00000049,0x00040063,0x00000085,0x0000008f,0x00000090,0x000100fd,0x00010038
};

static void BarGraph_Create( GpuContext_t * context, BarGraph_t * barGraph, GpuRenderPass_t * renderPass,
								const float x, const float y, const float width, const float height,
								const int numBars, const int numStacked, const Vector4f_t * backgroundColor )
{
	barGraph->clipRect.x = x;
	barGraph->clipRect.y = y;
	barGraph->clipRect.width = width;
	barGraph->clipRect.height = height;
	barGraph->numBars = numBars;
	barGraph->numStacked = numStacked;
	barGraph->barIndex = 0;
	barGraph->barValues = (float *) AllocAlignedMemory( numBars * numStacked * sizeof( barGraph->barValues[0] ), sizeof( void * ) );
	barGraph->barColors = (Vector4f_t *) AllocAlignedMemory( numBars * numStacked * sizeof( barGraph->barColors[0] ), sizeof( Vector4f_t ) );

	for ( int i = 0; i < numBars * numStacked; i++ )
	{
		barGraph->barValues[i] = 0.0f;
		barGraph->barColors[i] = colorGreen;
	}

	barGraph->backgroundColor = *backgroundColor;

	// graphics
	{
		GpuGeometry_CreateQuad( context, &barGraph->graphics.quad, 1.0f, 0.5f );
		GpuGeometry_AddInstanceAttributes( context, &barGraph->graphics.quad, numBars * numStacked + 1, VERTEX_ATTRIBUTE_FLAG_TRANSFORM );

		GpuGraphicsProgram_Create( context, &barGraph->graphics.program,
									PROGRAM( barGraphVertexProgram ), sizeof( PROGRAM( barGraphVertexProgram ) ),
									PROGRAM( barGraphFragmentProgram ), sizeof( PROGRAM( barGraphFragmentProgram ) ),
									barGraphGraphicsProgramParms, 0,
									VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_TRANSFORM );

		GpuGraphicsPipelineParms_t pipelineParms;
		GpuGraphicsPipelineParms_Init( &pipelineParms );

		pipelineParms.rop.depthTestEnable = false;
		pipelineParms.rop.depthWriteEnable = false;
		pipelineParms.renderPass = renderPass;
		pipelineParms.program = &barGraph->graphics.program;
		pipelineParms.geometry = &barGraph->graphics.quad;

		GpuGraphicsPipeline_Create( context, &barGraph->graphics.pipeline, &pipelineParms );

		barGraph->graphics.numInstances = 0;
	}

	// compute
	{
		GpuBuffer_Create( context, &barGraph->compute.barValueBuffer, GPU_BUFFER_TYPE_STORAGE,
							barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barValues[0] ), NULL, false );
		GpuBuffer_Create( context, &barGraph->compute.barColorBuffer, GPU_BUFFER_TYPE_STORAGE,
							barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barColors[0] ), NULL, false );

		GpuComputeProgram_Create( context, &barGraph->compute.program,
									PROGRAM( barGraphComputeProgram ), sizeof( PROGRAM( barGraphComputeProgram ) ),
									barGraphComputeProgramParms, ARRAY_SIZE( barGraphComputeProgramParms ) );

		GpuComputePipeline_Create( context, &barGraph->compute.pipeline, &barGraph->compute.program );
	}
}

static void BarGraph_Destroy( GpuContext_t * context, BarGraph_t * barGraph )
{
	FreeAlignedMemory( barGraph->barValues );
	FreeAlignedMemory( barGraph->barColors );

	// graphics
	{
		GpuGraphicsPipeline_Destroy( context, &barGraph->graphics.pipeline );
		GpuGraphicsProgram_Destroy( context, &barGraph->graphics.program );
		GpuGeometry_Destroy( context, &barGraph->graphics.quad );
	}

	// compute
	{
		GpuComputePipeline_Destroy( context, &barGraph->compute.pipeline );
		GpuComputeProgram_Destroy( context, &barGraph->compute.program );
		GpuBuffer_Destroy( context, &barGraph->compute.barValueBuffer );
		GpuBuffer_Destroy( context, &barGraph->compute.barColorBuffer );
	}
}

static void BarGraph_AddBar( BarGraph_t * barGraph, const int stackedBar, const float value, const Vector4f_t * color, const bool advance )
{
	assert( stackedBar >= 0 && stackedBar < barGraph->numStacked );
	barGraph->barValues[barGraph->barIndex * barGraph->numStacked + stackedBar] = value;
	barGraph->barColors[barGraph->barIndex * barGraph->numStacked + stackedBar] = *color;
	if ( advance )
	{
		barGraph->barIndex = ( barGraph->barIndex + 1 ) % barGraph->numBars;
	}
}

static void BarGraph_UpdateGraphics( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph )
{
	GpuVertexAttributeArrays_t attribs;
	GpuBuffer_t * instanceBuffer = GpuCommandBuffer_MapInstanceAttributes( commandBuffer, &barGraph->graphics.quad, &attribs );

#if defined( GRAPHICS_API_VULKAN )
	const float flipY = -1.0f;
#else
	const float flipY = 1.0f;
#endif

	int numInstances = 0;
	Matrix4x4f_t * backgroundMatrix = &attribs.transform[numInstances++];

	// Write in order to write-combined memory.
	backgroundMatrix->m[0][0] = barGraph->clipRect.width;
	backgroundMatrix->m[0][1] = 0.0f;
	backgroundMatrix->m[0][2] = 0.0f;
	backgroundMatrix->m[0][3] = barGraph->backgroundColor.x;

	backgroundMatrix->m[1][0] = 0.0f;
	backgroundMatrix->m[1][1] = barGraph->clipRect.height * flipY;
	backgroundMatrix->m[1][2] = 0.0f;
	backgroundMatrix->m[1][3] = barGraph->backgroundColor.y;

	backgroundMatrix->m[2][0] = 0.0f;
	backgroundMatrix->m[2][1] = 0.0f;
	backgroundMatrix->m[2][2] = 0.0f;
	backgroundMatrix->m[2][3] = barGraph->backgroundColor.z;

	backgroundMatrix->m[3][0] = barGraph->clipRect.x;
	backgroundMatrix->m[3][1] = barGraph->clipRect.y * flipY;
	backgroundMatrix->m[3][2] = 0.0f;
	backgroundMatrix->m[3][3] = barGraph->backgroundColor.w;

	const float barWidth = barGraph->clipRect.width / barGraph->numBars;

	for ( int i = 0; i < barGraph->numBars; i++ )
	{
		const int barIndex = ( ( barGraph->barIndex + i ) % barGraph->numBars ) * barGraph->numStacked;
		const float barColorScale = ( i & 1 ) ? 0.75f : 1.0f;

		float stackedBarValue = 0.0f;
		for ( int j = 0; j < barGraph->numStacked; j++ )
		{
			float value = barGraph->barValues[barIndex + j];
			if ( stackedBarValue + value > 1.0f )
			{
				value = 1.0f - stackedBarValue;
			}
			if ( value <= 0.0f )
			{
				continue;
			}

			Matrix4x4f_t * barMatrix = &attribs.transform[numInstances++];

			// Write in order to write-combined memory.
			barMatrix->m[0][0] = barWidth;
			barMatrix->m[0][1] = 0.0f;
			barMatrix->m[0][2] = 0.0f;
			barMatrix->m[0][3] = barGraph->barColors[barIndex + j].x * barColorScale;

			barMatrix->m[1][0] = 0.0f;
			barMatrix->m[1][1] = value * barGraph->clipRect.height * flipY;
			barMatrix->m[1][2] = 0.0f;
			barMatrix->m[1][3] = barGraph->barColors[barIndex + j].y * barColorScale;

			barMatrix->m[2][0] = 0.0f;
			barMatrix->m[2][1] = 0.0f;
			barMatrix->m[2][2] = 1.0f;
			barMatrix->m[2][3] = barGraph->barColors[barIndex + j].z * barColorScale;

			barMatrix->m[3][0] = barGraph->clipRect.x + i * barWidth;
			barMatrix->m[3][1] = ( barGraph->clipRect.y + stackedBarValue * barGraph->clipRect.height ) * flipY;
			barMatrix->m[3][2] = 0.0f;
			barMatrix->m[3][3] = barGraph->barColors[barIndex + j].w;

			stackedBarValue += value;
		}
	}

	GpuCommandBuffer_UnmapInstanceAttributes( commandBuffer, &barGraph->graphics.quad, instanceBuffer, GPU_BUFFER_UNMAP_TYPE_COPY_BACK );

	assert( numInstances <= barGraph->numBars * barGraph->numStacked + 1 );
	barGraph->graphics.numInstances = numInstances;
}

static void BarGraph_RenderGraphics( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph )
{
	GpuGraphicsCommand_t command;
	GpuGraphicsCommand_Init( &command );
	GpuGraphicsCommand_SetPipeline( &command, &barGraph->graphics.pipeline );
	GpuGraphicsCommand_SetNumInstances( &command, barGraph->graphics.numInstances );

	GpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
}

static void BarGraph_UpdateCompute( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph )
{
	void * barValues = NULL;
	GpuBuffer_t * mappedBarValueBuffer = GpuCommandBuffer_MapBuffer( commandBuffer, &barGraph->compute.barValueBuffer, &barValues );
	memcpy( barValues, barGraph->barValues, barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barValues[0] ) );
	GpuCommandBuffer_UnmapBuffer( commandBuffer, &barGraph->compute.barValueBuffer, mappedBarValueBuffer, GPU_BUFFER_UNMAP_TYPE_COPY_BACK );

	void * barColors = NULL;
	GpuBuffer_t * mappedBarColorBuffer = GpuCommandBuffer_MapBuffer( commandBuffer, &barGraph->compute.barColorBuffer, &barColors );
	memcpy( barColors, barGraph->barColors, barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barColors[0] ) );
	GpuCommandBuffer_UnmapBuffer( commandBuffer, &barGraph->compute.barColorBuffer, mappedBarColorBuffer, GPU_BUFFER_UNMAP_TYPE_COPY_BACK );
}

static void BarGraph_RenderCompute( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph, GpuFramebuffer_t * framebuffer )
{
	const int screenWidth = GpuFramebuffer_GetWidth( framebuffer );
	const int screenHeight = GpuFramebuffer_GetHeight( framebuffer );
	ScreenRect_t screenRect = ClipRect_ToScreenRect( &barGraph->clipRect, screenWidth, screenHeight );
	barGraph->compute.barGraphOffset.x = screenRect.x;
#if defined( GRAPHICS_API_VULKAN )
	barGraph->compute.barGraphOffset.y = screenHeight - 1 - screenRect.y;
#else
	barGraph->compute.barGraphOffset.y = screenRect.y;
#endif

	screenRect.width = ROUNDUP( screenRect.width, 8 );
	screenRect.height = ROUNDUP( screenRect.height, 8 );

	assert( screenRect.width % BARGRAPH_LOCAL_SIZE_X == 0 );
	assert( screenRect.height % BARGRAPH_LOCAL_SIZE_Y == 0 );

	GpuComputeCommand_t command;
	GpuComputeCommand_Init( &command );
	GpuComputeCommand_SetPipeline( &command, &barGraph->compute.pipeline );
	GpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_BAR_GRAPH_DEST, GpuFramebuffer_GetColorTexture( framebuffer ) );
	GpuComputeCommand_SetParmBufferStorage( &command, COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_VALUES, &barGraph->compute.barValueBuffer );
	GpuComputeCommand_SetParmBufferStorage( &command, COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_COLORS, &barGraph->compute.barColorBuffer );
	GpuComputeCommand_SetParmFloatVector4( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BACK_GROUND_COLOR, &barGraph->backgroundColor );
	GpuComputeCommand_SetParmIntVector2( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_GRAPH_OFFSET, &barGraph->compute.barGraphOffset );
	GpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_BARS, &barGraph->numBars );
	GpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_STACKED, &barGraph->numStacked );
	GpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_INDEX, &barGraph->barIndex );
	GpuComputeCommand_SetDimensions( &command, screenRect.width / BARGRAPH_LOCAL_SIZE_X, screenRect.height / BARGRAPH_LOCAL_SIZE_Y, 1 );

	GpuCommandBuffer_SubmitComputeCommand( commandBuffer, &command );
}

/*
================================================================================================================================

Time warp bar graphs.

TimeWarpBarGraphs_t

static void TimeWarpBarGraphs_Create( GpuContext_t * context, TimeWarpBarGraphs_t * bargraphs, GpuRenderPass_t * renderPass );
static void TimeWarpBarGraphs_Destroy( GpuContext_t * context, TimeWarpBarGraphs_t * bargraphs );

static void TimeWarpBarGraphs_UpdateGraphics( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs );
static void TimeWarpBarGraphs_RenderGraphics( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs );

static void TimeWarpBarGraphs_UpdateCompute( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs );
static void TimeWarpBarGraphs_RenderCompute( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs, GpuFramebuffer_t * framebuffer );

static float TimeWarpBarGraphs_GetGpuMillisecondsGraphics( TimeWarpBarGraphs_t * bargraphs );
static float TimeWarpBarGraphs_GetGpuMillisecondsCompute( TimeWarpBarGraphs_t * bargraphs );

================================================================================================================================
*/

#define BARGRAPH_VIRTUAL_PIXELS_WIDE		1920
#define BARGRAPH_VIRTUAL_PIXELS_HIGH		1080

#if defined( OS_ANDROID )
#define BARGRAPH_INSET						64
#else
#define BARGRAPH_INSET						16
#endif

static const ScreenRect_t eyeTextureFrameRateBarGraphRect			= { BARGRAPH_INSET + 0 * 264, BARGRAPH_INSET, 256, 128 };
static const ScreenRect_t timeWarpFrameRateBarGraphRect				= { BARGRAPH_INSET + 1 * 264, BARGRAPH_INSET, 256, 128 };
static const ScreenRect_t frameCpuTimeBarGraphRect					= { BARGRAPH_INSET + 2 * 264, BARGRAPH_INSET, 256, 128 };
static const ScreenRect_t frameGpuTimeBarGraphRect					= { BARGRAPH_INSET + 3 * 264, BARGRAPH_INSET, 256, 128 };

static const ScreenRect_t multiViewBarGraphRect						= { 3 * BARGRAPH_VIRTUAL_PIXELS_WIDE / 4 + 0 * 40, BARGRAPH_INSET, 32, 32 };
static const ScreenRect_t correctChromaticAberrationBarGraphRect	= { 3 * BARGRAPH_VIRTUAL_PIXELS_WIDE / 4 + 1 * 40, BARGRAPH_INSET, 32, 32 };
static const ScreenRect_t timeWarpImplementationBarGraphRect		= { 3 * BARGRAPH_VIRTUAL_PIXELS_WIDE / 4 + 2 * 40, BARGRAPH_INSET, 32, 32 };

static const ScreenRect_t sceneDrawCallLevelBarGraphRect			= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 3 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };
static const ScreenRect_t sceneTriangleLevelBarGraphRect			= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 2 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };
static const ScreenRect_t sceneFragmentLevelBarGraphRect			= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 1 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };

typedef enum
{
	BAR_GRAPH_HIDDEN,
	BAR_GRAPH_VISIBLE,
	BAR_GRAPH_PAUSED
} BarGraphState_t;

typedef struct
{
	BarGraphState_t	barGraphState;

	BarGraph_t		eyeTexturesFrameRateGraph;
	BarGraph_t		timeWarpFrameRateGraph;
	BarGraph_t		frameCpuTimeBarGraph;
	BarGraph_t		frameGpuTimeBarGraph;

	BarGraph_t		multiViewBarGraph;
	BarGraph_t		correctChromaticAberrationBarGraph;
	BarGraph_t		timeWarpImplementationBarGraph;

	BarGraph_t		sceneDrawCallLevelBarGraph;
	BarGraph_t		sceneTriangleLevelBarGraph;
	BarGraph_t		sceneFragmentLevelBarGraph;

	GpuTimer_t		barGraphTimer;
} TimeWarpBarGraphs_t;

enum
{
	PROFILE_TIME_EYE_TEXTURES,
	PROFILE_TIME_TIME_WARP,
	PROFILE_TIME_BAR_GRAPHS,
	PROFILE_TIME_BLIT,
	PROFILE_TIME_OVERFLOW,
	PROFILE_TIME_MAX
};

static const Vector4f_t * profileTimeBarColors[] =
{
	&colorPurple,
	&colorGreen,
	&colorYellow,
	&colorBlue,
	&colorRed
};

static void BarGraph_CreateVirtualRect( GpuContext_t * context, BarGraph_t * barGraph, GpuRenderPass_t * renderPass,
						const ScreenRect_t * virtualRect, const int numBars, const int numStacked, const Vector4f_t * backgroundColor )
{
	const ClipRect_t clipRect = ScreenRect_ToClipRect( virtualRect, BARGRAPH_VIRTUAL_PIXELS_WIDE, BARGRAPH_VIRTUAL_PIXELS_HIGH );
	BarGraph_Create( context, barGraph, renderPass, clipRect.x, clipRect.y, clipRect.width, clipRect.height, numBars, numStacked, backgroundColor );
}

static void TimeWarpBarGraphs_Create( GpuContext_t * context, TimeWarpBarGraphs_t * bargraphs, GpuRenderPass_t * renderPass )
{
	bargraphs->barGraphState = BAR_GRAPH_VISIBLE;

	BarGraph_CreateVirtualRect( context, &bargraphs->eyeTexturesFrameRateGraph, renderPass, &eyeTextureFrameRateBarGraphRect, 64, 1, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->timeWarpFrameRateGraph, renderPass, &timeWarpFrameRateBarGraphRect, 64, 1, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->frameCpuTimeBarGraph, renderPass, &frameCpuTimeBarGraphRect, 64, PROFILE_TIME_MAX, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->frameGpuTimeBarGraph, renderPass, &frameGpuTimeBarGraphRect, 64, PROFILE_TIME_MAX, &colorDarkGrey );

	BarGraph_CreateVirtualRect( context, &bargraphs->multiViewBarGraph, renderPass, &multiViewBarGraphRect, 1, 1, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->correctChromaticAberrationBarGraph, renderPass, &correctChromaticAberrationBarGraphRect, 1, 1, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->timeWarpImplementationBarGraph, renderPass, &timeWarpImplementationBarGraphRect, 1, 1, &colorDarkGrey );

	BarGraph_CreateVirtualRect( context, &bargraphs->sceneDrawCallLevelBarGraph, renderPass, &sceneDrawCallLevelBarGraphRect, 1, 4, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->sceneTriangleLevelBarGraph, renderPass, &sceneTriangleLevelBarGraphRect, 1, 4, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->sceneFragmentLevelBarGraph, renderPass, &sceneFragmentLevelBarGraphRect, 1, 4, &colorDarkGrey );

	BarGraph_AddBar( &bargraphs->sceneDrawCallLevelBarGraph, 0, 0.25f, &colorBlue, false );
	BarGraph_AddBar( &bargraphs->sceneTriangleLevelBarGraph, 0, 0.25f, &colorBlue, false );
	BarGraph_AddBar( &bargraphs->sceneFragmentLevelBarGraph, 0, 0.25f, &colorBlue, false );

	GpuTimer_Create( context, &bargraphs->barGraphTimer );
}

static void TimeWarpBarGraphs_Destroy( GpuContext_t * context, TimeWarpBarGraphs_t * bargraphs )
{
	BarGraph_Destroy( context, &bargraphs->eyeTexturesFrameRateGraph );
	BarGraph_Destroy( context, &bargraphs->timeWarpFrameRateGraph );
	BarGraph_Destroy( context, &bargraphs->frameCpuTimeBarGraph );
	BarGraph_Destroy( context, &bargraphs->frameGpuTimeBarGraph );

	BarGraph_Destroy( context, &bargraphs->multiViewBarGraph );
	BarGraph_Destroy( context, &bargraphs->correctChromaticAberrationBarGraph );
	BarGraph_Destroy( context, &bargraphs->timeWarpImplementationBarGraph );

	BarGraph_Destroy( context, &bargraphs->sceneDrawCallLevelBarGraph );
	BarGraph_Destroy( context, &bargraphs->sceneTriangleLevelBarGraph );
	BarGraph_Destroy( context, &bargraphs->sceneFragmentLevelBarGraph );

	GpuTimer_Destroy( context, &bargraphs->barGraphTimer );
}

static void TimeWarpBarGraphs_UpdateGraphics( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->eyeTexturesFrameRateGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->timeWarpFrameRateGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->frameCpuTimeBarGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->frameGpuTimeBarGraph );

		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->multiViewBarGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->correctChromaticAberrationBarGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->timeWarpImplementationBarGraph );

		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->sceneDrawCallLevelBarGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->sceneTriangleLevelBarGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->sceneFragmentLevelBarGraph );
	}
}

static void TimeWarpBarGraphs_RenderGraphics( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		GpuCommandBuffer_BeginTimer( commandBuffer, &bargraphs->barGraphTimer );

		BarGraph_RenderGraphics( commandBuffer, &bargraphs->eyeTexturesFrameRateGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->timeWarpFrameRateGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->frameCpuTimeBarGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->frameGpuTimeBarGraph );

		BarGraph_RenderGraphics( commandBuffer, &bargraphs->multiViewBarGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->correctChromaticAberrationBarGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->timeWarpImplementationBarGraph );

		BarGraph_RenderGraphics( commandBuffer, &bargraphs->sceneDrawCallLevelBarGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->sceneTriangleLevelBarGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->sceneFragmentLevelBarGraph );

		GpuCommandBuffer_EndTimer( commandBuffer, &bargraphs->barGraphTimer );
	}
}

static void TimeWarpBarGraphs_UpdateCompute( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->eyeTexturesFrameRateGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->timeWarpFrameRateGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->frameCpuTimeBarGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->frameGpuTimeBarGraph );

		BarGraph_UpdateCompute( commandBuffer, &bargraphs->multiViewBarGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->correctChromaticAberrationBarGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->timeWarpImplementationBarGraph );

		BarGraph_UpdateCompute( commandBuffer, &bargraphs->sceneDrawCallLevelBarGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->sceneTriangleLevelBarGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->sceneFragmentLevelBarGraph );
	}
}

static void TimeWarpBarGraphs_RenderCompute( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs, GpuFramebuffer_t * framebuffer )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		GpuCommandBuffer_BeginTimer( commandBuffer, &bargraphs->barGraphTimer );

		BarGraph_RenderCompute( commandBuffer, &bargraphs->eyeTexturesFrameRateGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->timeWarpFrameRateGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->frameCpuTimeBarGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->frameGpuTimeBarGraph, framebuffer );

		BarGraph_RenderCompute( commandBuffer, &bargraphs->multiViewBarGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->correctChromaticAberrationBarGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->timeWarpImplementationBarGraph, framebuffer );

		BarGraph_RenderCompute( commandBuffer, &bargraphs->sceneDrawCallLevelBarGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->sceneTriangleLevelBarGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->sceneFragmentLevelBarGraph, framebuffer );

		GpuCommandBuffer_EndTimer( commandBuffer, &bargraphs->barGraphTimer );
	}
}

static float TimeWarpBarGraphs_GetGpuMillisecondsGraphics( TimeWarpBarGraphs_t * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		return GpuTimer_GetMilliseconds( &bargraphs->barGraphTimer );
	}
	return 0.0f;
}

static float TimeWarpBarGraphs_GetGpuMillisecondsCompute( TimeWarpBarGraphs_t * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		return GpuTimer_GetMilliseconds( &bargraphs->barGraphTimer );
	}
	return 0.0f;
}

/*
================================================================================================================================

HMD

HmdInfo_t

================================================================================================================================
*/

// Typical 16:9 resolutions: 1920 x 1080, 2560 x 1440, 3840 x 2160, 7680 x 4320
#define DISPLAY_PIXELS_WIDE		1920
#define DISPLAY_PIXELS_HIGH		1080

#define NUM_EYES				2
#define NUM_COLOR_CHANNELS		3

#define TILE_PIXELS_WIDE		32
#define TILE_PIXELS_HIGH		32

#define EYE_TILES_WIDE			( DISPLAY_PIXELS_WIDE / TILE_PIXELS_WIDE / NUM_EYES )	// 30*32*2 = 1920
#define EYE_TILES_HIGH			( DISPLAY_PIXELS_HIGH / TILE_PIXELS_HIGH )				// 33*32   = 1056 leaving 24 pixels untouched

typedef struct
{
	int		widthInPixels;
	int		heightInPixels;
	float	widthInMeters;
	float	heightInMeters;
	float	lensSeparationInMeters;
	float	metersPerTanAngleAtCenter;
	int		numKnots;
	float	K[11];
	float	chromaticAberration[4];
} HmdInfo_t;

typedef struct
{
	float	interpupillaryDistance;
} BodyInfo_t;

static const HmdInfo_t * GetDefaultHmdInfo()
{
	static HmdInfo_t hmdInfo;
	hmdInfo.widthInPixels = EYE_TILES_WIDE * TILE_PIXELS_WIDE * NUM_EYES;
	hmdInfo.heightInPixels = EYE_TILES_HIGH * TILE_PIXELS_HIGH;
	hmdInfo.widthInMeters = 0.11047f * ( EYE_TILES_WIDE * TILE_PIXELS_WIDE * NUM_EYES ) / DISPLAY_PIXELS_WIDE;
	hmdInfo.heightInMeters = 0.06214f * ( EYE_TILES_HIGH * TILE_PIXELS_HIGH ) / DISPLAY_PIXELS_HIGH;
	hmdInfo.lensSeparationInMeters = hmdInfo.widthInMeters / NUM_EYES;//0.062f;
	hmdInfo.metersPerTanAngleAtCenter = 0.037f;
	hmdInfo.numKnots = 11;
	hmdInfo.K[0] = 1.0f;
	hmdInfo.K[1] = 1.021f;
	hmdInfo.K[2] = 1.051f;
	hmdInfo.K[3] = 1.086f;
	hmdInfo.K[4] = 1.128f;
	hmdInfo.K[5] = 1.177f;
	hmdInfo.K[6] = 1.232f;
	hmdInfo.K[7] = 1.295f;
	hmdInfo.K[8] = 1.368f;
	hmdInfo.K[9] = 1.452f;
	hmdInfo.K[10] = 1.560f;
	hmdInfo.chromaticAberration[0] = -0.006f;
	hmdInfo.chromaticAberration[1] =  0.0f;
	hmdInfo.chromaticAberration[2] =  0.014f;
	hmdInfo.chromaticAberration[3] =  0.0f;
	return &hmdInfo;
}

static const BodyInfo_t * GetDefaultBodyInfo()
{
	static BodyInfo_t bodyInfo;
	bodyInfo.interpupillaryDistance	= 0.0640f;	// average interpupillary distance
	return &bodyInfo;
}

static bool hmd_headRotationDisabled = false;

static void GetHmdViewMatrixForTime( Matrix4x4f_t * viewMatrix, const Microseconds_t time )
{
	if ( hmd_headRotationDisabled )
	{
		Matrix4x4f_CreateIdentity( viewMatrix );
		return;
	}

	const float offset = time * ( MATH_PI / 1000.0f / 1000.0f );
	const float degrees = 10.0f;
	const float degreesX = sinf( offset ) * degrees;
	const float degreesY = cosf( offset ) * degrees;

	Matrix4x4f_CreateRotation( viewMatrix, degreesX, degreesY, 0.0f );
}

static void CalculateTimeWarpTransform( Matrix4x4f_t * transform, const Matrix4x4f_t * renderProjectionMatrix,
										const Matrix4x4f_t * renderViewMatrix, const Matrix4x4f_t * newViewMatrix )
{
	// Convert the projection matrix from [-1, 1] space to [0, 1] space.
	const Matrix4x4f_t texCoordProjection =
	{ {
		{ 0.5f * renderProjectionMatrix->m[0][0],        0.0f,                                           0.0f,  0.0f },
		{ 0.0f,                                          0.5f * renderProjectionMatrix->m[1][1],         0.0f,  0.0f },
		{ 0.5f * renderProjectionMatrix->m[2][0] - 0.5f, 0.5f * renderProjectionMatrix->m[2][1] - 0.5f, -1.0f,  0.0f },
		{ 0.0f,                                          0.0f,                                           0.0f,  1.0f }
	} };

	// Calculate the delta between the view matrix used for rendering and
	// a more recent or predicted view matrix based on new sensor input.
	Matrix4x4f_t inverseRenderViewMatrix;
	Matrix4x4f_InvertHomogeneous( &inverseRenderViewMatrix, renderViewMatrix );

	Matrix4x4f_t deltaViewMatrix;
	Matrix4x4f_Multiply( &deltaViewMatrix, &inverseRenderViewMatrix, newViewMatrix );

	Matrix4x4f_t inverseDeltaViewMatrix;
	Matrix4x4f_InvertHomogeneous( &inverseDeltaViewMatrix, &deltaViewMatrix );

	// Make the delta rotation only.
	inverseDeltaViewMatrix.m[3][0] = 0.0f;
	inverseDeltaViewMatrix.m[3][1] = 0.0f;
	inverseDeltaViewMatrix.m[3][2] = 0.0f;

	// Accumulate the transforms.
	Matrix4x4f_Multiply( transform, &texCoordProjection, &inverseDeltaViewMatrix );
}

/*
================================================================================================================================

Distortion meshes.

MeshCoord_t

================================================================================================================================
*/

typedef struct
{
	float x;
	float y;
} MeshCoord_t;

static float MaxFloat( float x, float y ) { return ( x > y ) ? x : y; }
static float MinFloat( float x, float y ) { return ( x < y ) ? x : y; }

// A Catmull-Rom spline through the values K[0], K[1], K[2] ... K[numKnots-1] evenly spaced from 0.0 to 1.0
static float EvaluateCatmullRomSpline( const float value, float const * K, const int numKnots )
{
	const float scaledValue = (float)( numKnots - 1 ) * value;
	const float scaledValueFloor = MaxFloat( 0.0f, MinFloat( (float)( numKnots - 1 ), floorf( scaledValue ) ) );
	const float t = scaledValue - scaledValueFloor;
	const int k = (int)scaledValueFloor;

	float p0 = 0.0f;
	float p1 = 0.0f;
	float m0 = 0.0f;
	float m1 = 0.0f;

	if ( k == 0 )
	{
		p0 = K[0];
		m0 = K[1] - K[0];
		p1 = K[1];
		m1 = 0.5f * ( K[2] - K[0] );
	}
	else if ( k < numKnots - 2 )
	{
		p0 = K[k];
		m0 = 0.5f * ( K[k+1] - K[k-1] );
		p1 = K[k+1];
		m1 = 0.5f * ( K[k+2] - K[k] );
	}
	else if ( k == numKnots - 2 )
	{
		p0 = K[k];
		m0 = 0.5f * ( K[k+1] - K[k-1] );
		p1 = K[k+1];
		m1 = K[k+1] - K[k];
	}
	else if ( k == numKnots - 1 )
	{
		p0 = K[k];
		m0 = K[k] - K[k-1];
		p1 = p0 + m0;
		m1 = m0;
	}

	const float omt = 1.0f - t;
	const float res = ( p0 * ( 1.0f + 2.0f *   t ) + m0 *   t ) * omt * omt
					+ ( p1 * ( 1.0f + 2.0f * omt ) - m1 * omt ) *   t *   t;
	return res;
}

static void BuildDistortionMeshes( MeshCoord_t * meshCoords[NUM_EYES][NUM_COLOR_CHANNELS], const int eyeTilesWide, const int eyeTilesHigh, const HmdInfo_t * hmdInfo )
{
	const float horizontalShiftMeters = ( hmdInfo->lensSeparationInMeters / 2 ) - ( hmdInfo->widthInMeters / 4 );
	const float horizontalShiftView = horizontalShiftMeters / ( hmdInfo->widthInMeters / 2 );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		for ( int y = 0; y <= eyeTilesHigh; y++ )
		{
			const float yf = (float)y / (float)eyeTilesHigh;

			for ( int x = 0; x <= eyeTilesWide; x++ )
			{
				const float xf = (float)x / (float)eyeTilesWide;

				const float in[2] = { ( eye ? -horizontalShiftView : horizontalShiftView ) + xf, yf };
				const float ndcToPixels[2] = { hmdInfo->widthInPixels * 0.25f, hmdInfo->heightInPixels * 0.5f };
				const float pixelsToMeters[2] = { hmdInfo->widthInMeters / hmdInfo->widthInPixels, hmdInfo->heightInMeters / hmdInfo->heightInPixels };

				float theta[2];
				for ( int i = 0; i < 2; i++ )
				{
					const float unit = in[i];
					const float ndc = 2.0f * unit - 1.0f;
					const float pixels = ndc * ndcToPixels[i];
					const float meters = pixels * pixelsToMeters[i];
					const float tanAngle = meters / hmdInfo->metersPerTanAngleAtCenter;
					theta[i] = tanAngle;
				}

				const float rsq = theta[0] * theta[0] + theta[1] * theta[1];
				const float scale = EvaluateCatmullRomSpline( rsq, hmdInfo->K, hmdInfo->numKnots );
				const float chromaScale[NUM_COLOR_CHANNELS] =
				{
					scale * ( 1.0f + hmdInfo->chromaticAberration[0] + rsq * hmdInfo->chromaticAberration[1] ),
					scale,
					scale * ( 1.0f + hmdInfo->chromaticAberration[2] + rsq * hmdInfo->chromaticAberration[3] )
				};

				const int vertNum = y * ( eyeTilesWide + 1 ) + x;
				for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
				{
					meshCoords[eye][channel][vertNum].x = chromaScale[channel] * theta[0];
					meshCoords[eye][channel][vertNum].y = chromaScale[channel] * theta[1];
				}
			}
		}
	}
}

/*
================================================================================================================================

Time warp graphics rendering.

TimeWarpGraphics_t

static void TimeWarpGraphics_Create( GpuContext_t * context, TimeWarpGraphics_t * graphics, GpuRenderPass_t * renderPass );
static void TimeWarpGraphics_Destroy( GpuContext_t * context, TimeWarpGraphics_t * graphics );
static void TimeWarpGraphics_Render( GpuCommandBuffer_t * commandBuffer, TimeWarpGraphics_t * graphics,
									GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass,
									const Microseconds_t refreshStartTime, const Microseconds_t refreshEndTime,
									const Matrix4x4f_t * projectionMatrix, const Matrix4x4f_t * viewMatrix,
									GpuTexture_t * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, TimeWarpBarGraphs_t * bargraphs,
									float cpuTimes[PROFILE_TIME_MAX], float gpuTimes[PROFILE_TIME_MAX] );

================================================================================================================================
*/

typedef struct
{
	GpuGeometry_t			distortionMesh[NUM_EYES];
	GpuGraphicsProgram_t	timeWarpSpatialProgram;
	GpuGraphicsProgram_t	timeWarpChromaticProgram;
	GpuGraphicsPipeline_t	timeWarpSpatialPipeline[NUM_EYES];
	GpuGraphicsPipeline_t	timeWarpChromaticPipeline[NUM_EYES];
	GpuTimer_t				timeWarpGpuTime;
} TimeWarpGraphics_t;

enum
{
	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,
	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,
	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER,
	GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE
};

static const GpuProgramParm_t timeWarpSpatialGraphicsProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_VERTEX,		GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,	"TimeWarpStartTransform",	0 },
	{ GPU_PROGRAM_STAGE_VERTEX,		GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,	"TimeWarpEndTransform",		64 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER,		"ArrayLayer",				128 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE,			"Texture",					0 }
};

static const char timeWarpSpatialVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( std140, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =  0 ) highp mat4 TimeWarpStartTransform;\n"
	"	layout( offset = 64 ) highp mat4 TimeWarpEndTransform;\n"
	"} pc;\n"
	"layout( location = 0 ) in highp vec3 vertexPosition;\n"
	"layout( location = 1 ) in highp vec2 vertexUv1;\n"
	"layout( location = 0 ) out mediump vec2 fragmentUv1;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"void main( void )\n"
	"{\n"
	"	gl_Position = vec4( vertexPosition, 1.0 );\n"
	"\n"
	"	float displayFraction = vertexPosition.x * 0.5 + 0.5;\n"	// landscape left-to-right
	"\n"
	"	vec3 startUv1 = vec3( pc.TimeWarpStartTransform * vec4( vertexUv1, -1, 1 ) );\n"
	"	vec3 endUv1 = vec3( pc.TimeWarpEndTransform * vec4( vertexUv1, -1, 1 ) );\n"
	"	vec3 curUv1 = mix( startUv1, endUv1, displayFraction );\n"
	"	fragmentUv1 = curUv1.xy * ( 1.0 / max( curUv1.z, 0.00001 ) );\n"
	"}\n";

static const unsigned int timeWarpSpatialVertexProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x00000056,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0009000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000a,0x0000000f,0x0000002d,
	0x0000004c,0x00030003,0x00000002,0x000001b8,0x00040005,0x00000004,0x6e69616d,0x00000000,
	0x00060005,0x00000008,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000008,
	0x00000000,0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000000a,0x00000000,0x00060005,
	0x0000000f,0x74726576,0x6f507865,0x69746973,0x00006e6f,0x00060005,0x00000019,0x70736964,
	0x4679616c,0x74636172,0x006e6f69,0x00050005,0x00000023,0x72617473,0x31765574,0x00000000,
	0x00060005,0x00000025,0x68737550,0x736e6f43,0x746e6174,0x00000073,0x00090006,0x00000025,
	0x00000000,0x656d6954,0x70726157,0x72617453,0x61725474,0x6f66736e,0x00006d72,0x00090006,
	0x00000025,0x00000001,0x656d6954,0x70726157,0x54646e45,0x736e6172,0x6d726f66,0x00000000,
	0x00030005,0x00000027,0x00006370,0x00050005,0x0000002d,0x74726576,0x76557865,0x00000031,
	0x00040005,0x00000038,0x55646e65,0x00003176,0x00040005,0x00000045,0x55727563,0x00003176,
	0x00050005,0x0000004c,0x67617266,0x746e656d,0x00317655,0x00050048,0x00000008,0x00000000,
	0x0000000b,0x00000000,0x00030047,0x00000008,0x00000002,0x00040047,0x0000000f,0x0000001e,
	0x00000000,0x00040048,0x00000025,0x00000000,0x00000005,0x00050048,0x00000025,0x00000000,
	0x00000023,0x00000000,0x00050048,0x00000025,0x00000000,0x00000007,0x00000010,0x00040048,
	0x00000025,0x00000001,0x00000005,0x00050048,0x00000025,0x00000001,0x00000023,0x00000040,
	0x00050048,0x00000025,0x00000001,0x00000007,0x00000010,0x00030047,0x00000025,0x00000002,
	0x00040047,0x00000027,0x00000022,0x00000000,0x00040047,0x0000002d,0x0000001e,0x00000001,
	0x00040047,0x0000004c,0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,
	0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,
	0x0003001e,0x00000008,0x00000007,0x00040020,0x00000009,0x00000003,0x00000008,0x0004003b,
	0x00000009,0x0000000a,0x00000003,0x00040015,0x0000000b,0x00000020,0x00000001,0x0004002b,
	0x0000000b,0x0000000c,0x00000000,0x00040017,0x0000000d,0x00000006,0x00000003,0x00040020,
	0x0000000e,0x00000001,0x0000000d,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x0004002b,
	0x00000006,0x00000011,0x3f800000,0x00040020,0x00000016,0x00000003,0x00000007,0x00040020,
	0x00000018,0x00000007,0x00000006,0x00040015,0x0000001a,0x00000020,0x00000000,0x0004002b,
	0x0000001a,0x0000001b,0x00000000,0x00040020,0x0000001c,0x00000001,0x00000006,0x0004002b,
	0x00000006,0x0000001f,0x3f000000,0x00040020,0x00000022,0x00000007,0x0000000d,0x00040018,
	0x00000024,0x00000007,0x00000004,0x0004001e,0x00000025,0x00000024,0x00000024,0x00040020,
	0x00000026,0x00000009,0x00000025,0x0004003b,0x00000026,0x00000027,0x00000009,0x00040020,
	0x00000028,0x00000009,0x00000024,0x00040017,0x0000002b,0x00000006,0x00000002,0x00040020,
	0x0000002c,0x00000001,0x0000002b,0x0004003b,0x0000002c,0x0000002d,0x00000001,0x0004002b,
	0x00000006,0x0000002f,0xbf800000,0x0004002b,0x0000000b,0x00000039,0x00000001,0x00040020,
	0x0000004b,0x00000003,0x0000002b,0x0004003b,0x0000004b,0x0000004c,0x00000003,0x0004002b,
	0x0000001a,0x0000004f,0x00000002,0x0004002b,0x00000006,0x00000052,0x3727c5ac,0x00050036,
	0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000018,
	0x00000019,0x00000007,0x0004003b,0x00000022,0x00000023,0x00000007,0x0004003b,0x00000022,
	0x00000038,0x00000007,0x0004003b,0x00000022,0x00000045,0x00000007,0x0004003d,0x0000000d,
	0x00000010,0x0000000f,0x00050051,0x00000006,0x00000012,0x00000010,0x00000000,0x00050051,
	0x00000006,0x00000013,0x00000010,0x00000001,0x00050051,0x00000006,0x00000014,0x00000010,
	0x00000002,0x00070050,0x00000007,0x00000015,0x00000012,0x00000013,0x00000014,0x00000011,
	0x00050041,0x00000016,0x00000017,0x0000000a,0x0000000c,0x0003003e,0x00000017,0x00000015,
	0x00050041,0x0000001c,0x0000001d,0x0000000f,0x0000001b,0x0004003d,0x00000006,0x0000001e,
	0x0000001d,0x00050085,0x00000006,0x00000020,0x0000001e,0x0000001f,0x00050081,0x00000006,
	0x00000021,0x00000020,0x0000001f,0x0003003e,0x00000019,0x00000021,0x00050041,0x00000028,
	0x00000029,0x00000027,0x0000000c,0x0004003d,0x00000024,0x0000002a,0x00000029,0x0004003d,
	0x0000002b,0x0000002e,0x0000002d,0x00050051,0x00000006,0x00000030,0x0000002e,0x00000000,
	0x00050051,0x00000006,0x00000031,0x0000002e,0x00000001,0x00070050,0x00000007,0x00000032,
	0x00000030,0x00000031,0x0000002f,0x00000011,0x00050091,0x00000007,0x00000033,0x0000002a,
	0x00000032,0x00050051,0x00000006,0x00000034,0x00000033,0x00000000,0x00050051,0x00000006,
	0x00000035,0x00000033,0x00000001,0x00050051,0x00000006,0x00000036,0x00000033,0x00000002,
	0x00060050,0x0000000d,0x00000037,0x00000034,0x00000035,0x00000036,0x0003003e,0x00000023,
	0x00000037,0x00050041,0x00000028,0x0000003a,0x00000027,0x00000039,0x0004003d,0x00000024,
	0x0000003b,0x0000003a,0x0004003d,0x0000002b,0x0000003c,0x0000002d,0x00050051,0x00000006,
	0x0000003d,0x0000003c,0x00000000,0x00050051,0x00000006,0x0000003e,0x0000003c,0x00000001,
	0x00070050,0x00000007,0x0000003f,0x0000003d,0x0000003e,0x0000002f,0x00000011,0x00050091,
	0x00000007,0x00000040,0x0000003b,0x0000003f,0x00050051,0x00000006,0x00000041,0x00000040,
	0x00000000,0x00050051,0x00000006,0x00000042,0x00000040,0x00000001,0x00050051,0x00000006,
	0x00000043,0x00000040,0x00000002,0x00060050,0x0000000d,0x00000044,0x00000041,0x00000042,
	0x00000043,0x0003003e,0x00000038,0x00000044,0x0004003d,0x0000000d,0x00000046,0x00000023,
	0x0004003d,0x0000000d,0x00000047,0x00000038,0x0004003d,0x00000006,0x00000048,0x00000019,
	0x00060050,0x0000000d,0x00000049,0x00000048,0x00000048,0x00000048,0x0008000c,0x0000000d,
	0x0000004a,0x00000001,0x0000002e,0x00000046,0x00000047,0x00000049,0x0003003e,0x00000045,
	0x0000004a,0x0004003d,0x0000000d,0x0000004d,0x00000045,0x0007004f,0x0000002b,0x0000004e,
	0x0000004d,0x0000004d,0x00000000,0x00000001,0x00050041,0x00000018,0x00000050,0x00000045,
	0x0000004f,0x0004003d,0x00000006,0x00000051,0x00000050,0x0007000c,0x00000006,0x00000053,
	0x00000001,0x00000028,0x00000051,0x00000052,0x00050088,0x00000006,0x00000054,0x00000011,
	0x00000053,0x0005008e,0x0000002b,0x00000055,0x0000004e,0x00000054,0x0003003e,0x0000004c,
	0x00000055,0x000100fd,0x00010038
};

static const char timeWarpSpatialFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( std140, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset = 128 ) int ArrayLayer;\n"
	"} pc;\n"
	"layout( binding = 0 ) uniform highp sampler2DArray Texture;\n"
	"layout( location = 0 ) in mediump vec2 fragmentUv1;\n"
	"layout( location = 0 ) out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	outColor = texture( Texture, vec3( fragmentUv1, pc.ArrayLayer ) );\n"
	"}\n";

static const unsigned int timeWarpSpatialFragmentProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x00000021,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x00000011,0x00030010,
	0x00000004,0x00000007,0x00030003,0x00000002,0x000001b8,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00050005,0x00000009,0x4374756f,0x726f6c6f,0x00000000,0x00040005,0x0000000d,
	0x74786554,0x00657275,0x00050005,0x00000011,0x67617266,0x746e656d,0x00317655,0x00060005,
	0x00000014,0x68737550,0x736e6f43,0x746e6174,0x00000073,0x00060006,0x00000014,0x00000000,
	0x61727241,0x79614c79,0x00007265,0x00030005,0x00000016,0x00006370,0x00040047,0x00000009,
	0x0000001e,0x00000000,0x00040047,0x0000000d,0x00000022,0x00000000,0x00040047,0x0000000d,
	0x00000021,0x00000000,0x00040047,0x00000011,0x0000001e,0x00000000,0x00050048,0x00000014,
	0x00000000,0x00000023,0x00000080,0x00030047,0x00000014,0x00000002,0x00040047,0x00000016,
	0x00000022,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,
	0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,
	0x00000003,0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00090019,0x0000000a,
	0x00000006,0x00000001,0x00000000,0x00000001,0x00000000,0x00000001,0x00000000,0x0003001b,
	0x0000000b,0x0000000a,0x00040020,0x0000000c,0x00000000,0x0000000b,0x0004003b,0x0000000c,
	0x0000000d,0x00000000,0x00040017,0x0000000f,0x00000006,0x00000002,0x00040020,0x00000010,
	0x00000001,0x0000000f,0x0004003b,0x00000010,0x00000011,0x00000001,0x00040015,0x00000013,
	0x00000020,0x00000001,0x0003001e,0x00000014,0x00000013,0x00040020,0x00000015,0x00000009,
	0x00000014,0x0004003b,0x00000015,0x00000016,0x00000009,0x0004002b,0x00000013,0x00000017,
	0x00000000,0x00040020,0x00000018,0x00000009,0x00000013,0x00040017,0x0000001c,0x00000006,
	0x00000003,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
	0x0004003d,0x0000000b,0x0000000e,0x0000000d,0x0004003d,0x0000000f,0x00000012,0x00000011,
	0x00050041,0x00000018,0x00000019,0x00000016,0x00000017,0x0004003d,0x00000013,0x0000001a,
	0x00000019,0x0004006f,0x00000006,0x0000001b,0x0000001a,0x00050051,0x00000006,0x0000001d,
	0x00000012,0x00000000,0x00050051,0x00000006,0x0000001e,0x00000012,0x00000001,0x00060050,
	0x0000001c,0x0000001f,0x0000001d,0x0000001e,0x0000001b,0x00050057,0x00000007,0x00000020,
	0x0000000e,0x0000001f,0x0003003e,0x00000009,0x00000020,0x000100fd,0x00010038
};

static const GpuProgramParm_t timeWarpChromaticGraphicsProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_VERTEX,		GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,	"TimeWarpStartTransform",	0 },
	{ GPU_PROGRAM_STAGE_VERTEX,		GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,	"TimeWarpEndTransform",		64 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER,		"ArrayLayer",				128 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE,			"Texture",					0 }
};

static const char timeWarpChromaticVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( std140, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =  0 ) highp mat4 TimeWarpStartTransform;\n"
	"	layout( offset = 64 ) highp mat4 TimeWarpEndTransform;\n"
	"} pc;\n"
	"layout( location = 0 ) in highp vec3 vertexPosition;\n"
	"layout( location = 1 ) in highp vec2 vertexUv0;\n"
	"layout( location = 2 ) in highp vec2 vertexUv1;\n"
	"layout( location = 3 ) in highp vec2 vertexUv2;\n"
	"layout( location = 0 ) out mediump vec2 fragmentUv0;\n"
	"layout( location = 1 ) out mediump vec2 fragmentUv1;\n"
	"layout( location = 2 ) out mediump vec2 fragmentUv2;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"void main( void )\n"
	"{\n"
	"	gl_Position = vec4( vertexPosition, 1.0 );\n"
	"\n"
	"	float displayFraction = vertexPosition.x * 0.5 + 0.5;\n"	// landscape left-to-right
	"\n"
	"	vec3 startUv0 = vec3( pc.TimeWarpStartTransform * vec4( vertexUv0, -1, 1 ) );\n"
	"	vec3 startUv1 = vec3( pc.TimeWarpStartTransform * vec4( vertexUv1, -1, 1 ) );\n"
	"	vec3 startUv2 = vec3( pc.TimeWarpStartTransform * vec4( vertexUv2, -1, 1 ) );\n"
	"\n"
	"	vec3 endUv0 = vec3( pc.TimeWarpEndTransform * vec4( vertexUv0, -1, 1 ) );\n"
	"	vec3 endUv1 = vec3( pc.TimeWarpEndTransform * vec4( vertexUv1, -1, 1 ) );\n"
	"	vec3 endUv2 = vec3( pc.TimeWarpEndTransform * vec4( vertexUv2, -1, 1 ) );\n"
	"\n"
	"	vec3 curUv0 = mix( startUv0, endUv0, displayFraction );\n"
	"	vec3 curUv1 = mix( startUv1, endUv1, displayFraction );\n"
	"	vec3 curUv2 = mix( startUv2, endUv2, displayFraction );\n"
	"\n"
	"	fragmentUv0 = curUv0.xy * ( 1.0 / max( curUv0.z, 0.00001 ) );\n"
	"	fragmentUv1 = curUv1.xy * ( 1.0 / max( curUv1.z, 0.00001 ) );\n"
	"	fragmentUv2 = curUv2.xy * ( 1.0 / max( curUv2.z, 0.00001 ) );\n"
	"}\n";

static const unsigned int timeWarpChromaticVertexProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x000000a4,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000d000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000a,0x0000000f,0x0000002d,
	0x0000003b,0x00000048,0x0000008a,0x00000094,0x0000009c,0x00030003,0x00000002,0x000001b8,
	0x00040005,0x00000004,0x6e69616d,0x00000000,0x00060005,0x00000008,0x505f6c67,0x65567265,
	0x78657472,0x00000000,0x00060006,0x00000008,0x00000000,0x505f6c67,0x7469736f,0x006e6f69,
	0x00030005,0x0000000a,0x00000000,0x00060005,0x0000000f,0x74726576,0x6f507865,0x69746973,
	0x00006e6f,0x00060005,0x00000019,0x70736964,0x4679616c,0x74636172,0x006e6f69,0x00050005,
	0x00000023,0x72617473,0x30765574,0x00000000,0x00060005,0x00000025,0x68737550,0x736e6f43,
	0x746e6174,0x00000073,0x00090006,0x00000025,0x00000000,0x656d6954,0x70726157,0x72617453,
	0x61725474,0x6f66736e,0x00006d72,0x00090006,0x00000025,0x00000001,0x656d6954,0x70726157,
	0x54646e45,0x736e6172,0x6d726f66,0x00000000,0x00030005,0x00000027,0x00006370,0x00050005,
	0x0000002d,0x74726576,0x76557865,0x00000030,0x00050005,0x00000038,0x72617473,0x31765574,
	0x00000000,0x00050005,0x0000003b,0x74726576,0x76557865,0x00000031,0x00050005,0x00000045,
	0x72617473,0x32765574,0x00000000,0x00050005,0x00000048,0x74726576,0x76557865,0x00000032,
	0x00040005,0x00000052,0x55646e65,0x00003076,0x00040005,0x0000005f,0x55646e65,0x00003176,
	0x00040005,0x0000006b,0x55646e65,0x00003276,0x00040005,0x00000077,0x55727563,0x00003076,
	0x00040005,0x0000007d,0x55727563,0x00003176,0x00040005,0x00000083,0x55727563,0x00003276,
	0x00050005,0x0000008a,0x67617266,0x746e656d,0x00307655,0x00050005,0x00000094,0x67617266,
	0x746e656d,0x00317655,0x00050005,0x0000009c,0x67617266,0x746e656d,0x00327655,0x00050048,
	0x00000008,0x00000000,0x0000000b,0x00000000,0x00030047,0x00000008,0x00000002,0x00040047,
	0x0000000f,0x0000001e,0x00000000,0x00040048,0x00000025,0x00000000,0x00000005,0x00050048,
	0x00000025,0x00000000,0x00000023,0x00000000,0x00050048,0x00000025,0x00000000,0x00000007,
	0x00000010,0x00040048,0x00000025,0x00000001,0x00000005,0x00050048,0x00000025,0x00000001,
	0x00000023,0x00000040,0x00050048,0x00000025,0x00000001,0x00000007,0x00000010,0x00030047,
	0x00000025,0x00000002,0x00040047,0x00000027,0x00000022,0x00000000,0x00040047,0x0000002d,
	0x0000001e,0x00000001,0x00040047,0x0000003b,0x0000001e,0x00000002,0x00040047,0x00000048,
	0x0000001e,0x00000003,0x00040047,0x0000008a,0x0000001e,0x00000000,0x00040047,0x00000094,
	0x0000001e,0x00000001,0x00040047,0x0000009c,0x0000001e,0x00000002,0x00020013,0x00000002,
	0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,
	0x00000006,0x00000004,0x0003001e,0x00000008,0x00000007,0x00040020,0x00000009,0x00000003,
	0x00000008,0x0004003b,0x00000009,0x0000000a,0x00000003,0x00040015,0x0000000b,0x00000020,
	0x00000001,0x0004002b,0x0000000b,0x0000000c,0x00000000,0x00040017,0x0000000d,0x00000006,
	0x00000003,0x00040020,0x0000000e,0x00000001,0x0000000d,0x0004003b,0x0000000e,0x0000000f,
	0x00000001,0x0004002b,0x00000006,0x00000011,0x3f800000,0x00040020,0x00000016,0x00000003,
	0x00000007,0x00040020,0x00000018,0x00000007,0x00000006,0x00040015,0x0000001a,0x00000020,
	0x00000000,0x0004002b,0x0000001a,0x0000001b,0x00000000,0x00040020,0x0000001c,0x00000001,
	0x00000006,0x0004002b,0x00000006,0x0000001f,0x3f000000,0x00040020,0x00000022,0x00000007,
	0x0000000d,0x00040018,0x00000024,0x00000007,0x00000004,0x0004001e,0x00000025,0x00000024,
	0x00000024,0x00040020,0x00000026,0x00000009,0x00000025,0x0004003b,0x00000026,0x00000027,
	0x00000009,0x00040020,0x00000028,0x00000009,0x00000024,0x00040017,0x0000002b,0x00000006,
	0x00000002,0x00040020,0x0000002c,0x00000001,0x0000002b,0x0004003b,0x0000002c,0x0000002d,
	0x00000001,0x0004002b,0x00000006,0x0000002f,0xbf800000,0x0004003b,0x0000002c,0x0000003b,
	0x00000001,0x0004003b,0x0000002c,0x00000048,0x00000001,0x0004002b,0x0000000b,0x00000053,
	0x00000001,0x00040020,0x00000089,0x00000003,0x0000002b,0x0004003b,0x00000089,0x0000008a,
	0x00000003,0x0004002b,0x0000001a,0x0000008d,0x00000002,0x0004002b,0x00000006,0x00000090,
	0x3727c5ac,0x0004003b,0x00000089,0x00000094,0x00000003,0x0004003b,0x00000089,0x0000009c,
	0x00000003,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
	0x0004003b,0x00000018,0x00000019,0x00000007,0x0004003b,0x00000022,0x00000023,0x00000007,
	0x0004003b,0x00000022,0x00000038,0x00000007,0x0004003b,0x00000022,0x00000045,0x00000007,
	0x0004003b,0x00000022,0x00000052,0x00000007,0x0004003b,0x00000022,0x0000005f,0x00000007,
	0x0004003b,0x00000022,0x0000006b,0x00000007,0x0004003b,0x00000022,0x00000077,0x00000007,
	0x0004003b,0x00000022,0x0000007d,0x00000007,0x0004003b,0x00000022,0x00000083,0x00000007,
	0x0004003d,0x0000000d,0x00000010,0x0000000f,0x00050051,0x00000006,0x00000012,0x00000010,
	0x00000000,0x00050051,0x00000006,0x00000013,0x00000010,0x00000001,0x00050051,0x00000006,
	0x00000014,0x00000010,0x00000002,0x00070050,0x00000007,0x00000015,0x00000012,0x00000013,
	0x00000014,0x00000011,0x00050041,0x00000016,0x00000017,0x0000000a,0x0000000c,0x0003003e,
	0x00000017,0x00000015,0x00050041,0x0000001c,0x0000001d,0x0000000f,0x0000001b,0x0004003d,
	0x00000006,0x0000001e,0x0000001d,0x00050085,0x00000006,0x00000020,0x0000001e,0x0000001f,
	0x00050081,0x00000006,0x00000021,0x00000020,0x0000001f,0x0003003e,0x00000019,0x00000021,
	0x00050041,0x00000028,0x00000029,0x00000027,0x0000000c,0x0004003d,0x00000024,0x0000002a,
	0x00000029,0x0004003d,0x0000002b,0x0000002e,0x0000002d,0x00050051,0x00000006,0x00000030,
	0x0000002e,0x00000000,0x00050051,0x00000006,0x00000031,0x0000002e,0x00000001,0x00070050,
	0x00000007,0x00000032,0x00000030,0x00000031,0x0000002f,0x00000011,0x00050091,0x00000007,
	0x00000033,0x0000002a,0x00000032,0x00050051,0x00000006,0x00000034,0x00000033,0x00000000,
	0x00050051,0x00000006,0x00000035,0x00000033,0x00000001,0x00050051,0x00000006,0x00000036,
	0x00000033,0x00000002,0x00060050,0x0000000d,0x00000037,0x00000034,0x00000035,0x00000036,
	0x0003003e,0x00000023,0x00000037,0x00050041,0x00000028,0x00000039,0x00000027,0x0000000c,
	0x0004003d,0x00000024,0x0000003a,0x00000039,0x0004003d,0x0000002b,0x0000003c,0x0000003b,
	0x00050051,0x00000006,0x0000003d,0x0000003c,0x00000000,0x00050051,0x00000006,0x0000003e,
	0x0000003c,0x00000001,0x00070050,0x00000007,0x0000003f,0x0000003d,0x0000003e,0x0000002f,
	0x00000011,0x00050091,0x00000007,0x00000040,0x0000003a,0x0000003f,0x00050051,0x00000006,
	0x00000041,0x00000040,0x00000000,0x00050051,0x00000006,0x00000042,0x00000040,0x00000001,
	0x00050051,0x00000006,0x00000043,0x00000040,0x00000002,0x00060050,0x0000000d,0x00000044,
	0x00000041,0x00000042,0x00000043,0x0003003e,0x00000038,0x00000044,0x00050041,0x00000028,
	0x00000046,0x00000027,0x0000000c,0x0004003d,0x00000024,0x00000047,0x00000046,0x0004003d,
	0x0000002b,0x00000049,0x00000048,0x00050051,0x00000006,0x0000004a,0x00000049,0x00000000,
	0x00050051,0x00000006,0x0000004b,0x00000049,0x00000001,0x00070050,0x00000007,0x0000004c,
	0x0000004a,0x0000004b,0x0000002f,0x00000011,0x00050091,0x00000007,0x0000004d,0x00000047,
	0x0000004c,0x00050051,0x00000006,0x0000004e,0x0000004d,0x00000000,0x00050051,0x00000006,
	0x0000004f,0x0000004d,0x00000001,0x00050051,0x00000006,0x00000050,0x0000004d,0x00000002,
	0x00060050,0x0000000d,0x00000051,0x0000004e,0x0000004f,0x00000050,0x0003003e,0x00000045,
	0x00000051,0x00050041,0x00000028,0x00000054,0x00000027,0x00000053,0x0004003d,0x00000024,
	0x00000055,0x00000054,0x0004003d,0x0000002b,0x00000056,0x0000002d,0x00050051,0x00000006,
	0x00000057,0x00000056,0x00000000,0x00050051,0x00000006,0x00000058,0x00000056,0x00000001,
	0x00070050,0x00000007,0x00000059,0x00000057,0x00000058,0x0000002f,0x00000011,0x00050091,
	0x00000007,0x0000005a,0x00000055,0x00000059,0x00050051,0x00000006,0x0000005b,0x0000005a,
	0x00000000,0x00050051,0x00000006,0x0000005c,0x0000005a,0x00000001,0x00050051,0x00000006,
	0x0000005d,0x0000005a,0x00000002,0x00060050,0x0000000d,0x0000005e,0x0000005b,0x0000005c,
	0x0000005d,0x0003003e,0x00000052,0x0000005e,0x00050041,0x00000028,0x00000060,0x00000027,
	0x00000053,0x0004003d,0x00000024,0x00000061,0x00000060,0x0004003d,0x0000002b,0x00000062,
	0x0000003b,0x00050051,0x00000006,0x00000063,0x00000062,0x00000000,0x00050051,0x00000006,
	0x00000064,0x00000062,0x00000001,0x00070050,0x00000007,0x00000065,0x00000063,0x00000064,
	0x0000002f,0x00000011,0x00050091,0x00000007,0x00000066,0x00000061,0x00000065,0x00050051,
	0x00000006,0x00000067,0x00000066,0x00000000,0x00050051,0x00000006,0x00000068,0x00000066,
	0x00000001,0x00050051,0x00000006,0x00000069,0x00000066,0x00000002,0x00060050,0x0000000d,
	0x0000006a,0x00000067,0x00000068,0x00000069,0x0003003e,0x0000005f,0x0000006a,0x00050041,
	0x00000028,0x0000006c,0x00000027,0x00000053,0x0004003d,0x00000024,0x0000006d,0x0000006c,
	0x0004003d,0x0000002b,0x0000006e,0x00000048,0x00050051,0x00000006,0x0000006f,0x0000006e,
	0x00000000,0x00050051,0x00000006,0x00000070,0x0000006e,0x00000001,0x00070050,0x00000007,
	0x00000071,0x0000006f,0x00000070,0x0000002f,0x00000011,0x00050091,0x00000007,0x00000072,
	0x0000006d,0x00000071,0x00050051,0x00000006,0x00000073,0x00000072,0x00000000,0x00050051,
	0x00000006,0x00000074,0x00000072,0x00000001,0x00050051,0x00000006,0x00000075,0x00000072,
	0x00000002,0x00060050,0x0000000d,0x00000076,0x00000073,0x00000074,0x00000075,0x0003003e,
	0x0000006b,0x00000076,0x0004003d,0x0000000d,0x00000078,0x00000023,0x0004003d,0x0000000d,
	0x00000079,0x00000052,0x0004003d,0x00000006,0x0000007a,0x00000019,0x00060050,0x0000000d,
	0x0000007b,0x0000007a,0x0000007a,0x0000007a,0x0008000c,0x0000000d,0x0000007c,0x00000001,
	0x0000002e,0x00000078,0x00000079,0x0000007b,0x0003003e,0x00000077,0x0000007c,0x0004003d,
	0x0000000d,0x0000007e,0x00000038,0x0004003d,0x0000000d,0x0000007f,0x0000005f,0x0004003d,
	0x00000006,0x00000080,0x00000019,0x00060050,0x0000000d,0x00000081,0x00000080,0x00000080,
	0x00000080,0x0008000c,0x0000000d,0x00000082,0x00000001,0x0000002e,0x0000007e,0x0000007f,
	0x00000081,0x0003003e,0x0000007d,0x00000082,0x0004003d,0x0000000d,0x00000084,0x00000045,
	0x0004003d,0x0000000d,0x00000085,0x0000006b,0x0004003d,0x00000006,0x00000086,0x00000019,
	0x00060050,0x0000000d,0x00000087,0x00000086,0x00000086,0x00000086,0x0008000c,0x0000000d,
	0x00000088,0x00000001,0x0000002e,0x00000084,0x00000085,0x00000087,0x0003003e,0x00000083,
	0x00000088,0x0004003d,0x0000000d,0x0000008b,0x00000077,0x0007004f,0x0000002b,0x0000008c,
	0x0000008b,0x0000008b,0x00000000,0x00000001,0x00050041,0x00000018,0x0000008e,0x00000077,
	0x0000008d,0x0004003d,0x00000006,0x0000008f,0x0000008e,0x0007000c,0x00000006,0x00000091,
	0x00000001,0x00000028,0x0000008f,0x00000090,0x00050088,0x00000006,0x00000092,0x00000011,
	0x00000091,0x0005008e,0x0000002b,0x00000093,0x0000008c,0x00000092,0x0003003e,0x0000008a,
	0x00000093,0x0004003d,0x0000000d,0x00000095,0x0000007d,0x0007004f,0x0000002b,0x00000096,
	0x00000095,0x00000095,0x00000000,0x00000001,0x00050041,0x00000018,0x00000097,0x0000007d,
	0x0000008d,0x0004003d,0x00000006,0x00000098,0x00000097,0x0007000c,0x00000006,0x00000099,
	0x00000001,0x00000028,0x00000098,0x00000090,0x00050088,0x00000006,0x0000009a,0x00000011,
	0x00000099,0x0005008e,0x0000002b,0x0000009b,0x00000096,0x0000009a,0x0003003e,0x00000094,
	0x0000009b,0x0004003d,0x0000000d,0x0000009d,0x00000083,0x0007004f,0x0000002b,0x0000009e,
	0x0000009d,0x0000009d,0x00000000,0x00000001,0x00050041,0x00000018,0x0000009f,0x00000083,
	0x0000008d,0x0004003d,0x00000006,0x000000a0,0x0000009f,0x0007000c,0x00000006,0x000000a1,
	0x00000001,0x00000028,0x000000a0,0x00000090,0x00050088,0x00000006,0x000000a2,0x00000011,
	0x000000a1,0x0005008e,0x0000002b,0x000000a3,0x0000009e,0x000000a2,0x0003003e,0x0000009c,
	0x000000a3,0x000100fd,0x00010038
};

static const char timeWarpChromaticFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( std140, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset = 128 ) int ArrayLayer;\n"
	"} pc;\n"
	"layout( binding = 0 ) uniform highp sampler2DArray Texture;\n"
	"layout( location = 0 ) in mediump vec2 fragmentUv0;\n"
	"layout( location = 1 ) in mediump vec2 fragmentUv1;\n"
	"layout( location = 2 ) in mediump vec2 fragmentUv2;\n"
	"layout( location = 0 ) out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	outColor.r = texture( Texture, vec3( fragmentUv0, pc.ArrayLayer ) ).r;\n"
	"	outColor.g = texture( Texture, vec3( fragmentUv1, pc.ArrayLayer ) ).g;\n"
	"	outColor.b = texture( Texture, vec3( fragmentUv2, pc.ArrayLayer ) ).b;\n"
	"	outColor.a = 1.0;\n"
	"}\n";

static const unsigned int timeWarpChromaticFragmentProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x00000043,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0009000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x00000011,0x00000027,
	0x00000034,0x00030010,0x00000004,0x00000007,0x00030003,0x00000002,0x000001b8,0x00040005,
	0x00000004,0x6e69616d,0x00000000,0x00050005,0x00000009,0x4374756f,0x726f6c6f,0x00000000,
	0x00040005,0x0000000d,0x74786554,0x00657275,0x00050005,0x00000011,0x67617266,0x746e656d,
	0x00307655,0x00060005,0x00000014,0x68737550,0x736e6f43,0x746e6174,0x00000073,0x00060006,
	0x00000014,0x00000000,0x61727241,0x79614c79,0x00007265,0x00030005,0x00000016,0x00006370,
	0x00050005,0x00000027,0x67617266,0x746e656d,0x00317655,0x00050005,0x00000034,0x67617266,
	0x746e656d,0x00327655,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,
	0x00000022,0x00000000,0x00040047,0x0000000d,0x00000021,0x00000000,0x00040047,0x00000011,
	0x0000001e,0x00000000,0x00050048,0x00000014,0x00000000,0x00000023,0x00000080,0x00030047,
	0x00000014,0x00000002,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000027,
	0x0000001e,0x00000001,0x00040047,0x00000034,0x0000001e,0x00000002,0x00020013,0x00000002,
	0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,
	0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,0x00000007,0x0004003b,0x00000008,
	0x00000009,0x00000003,0x00090019,0x0000000a,0x00000006,0x00000001,0x00000000,0x00000001,
	0x00000000,0x00000001,0x00000000,0x0003001b,0x0000000b,0x0000000a,0x00040020,0x0000000c,
	0x00000000,0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000000,0x00040017,0x0000000f,
	0x00000006,0x00000002,0x00040020,0x00000010,0x00000001,0x0000000f,0x0004003b,0x00000010,
	0x00000011,0x00000001,0x00040015,0x00000013,0x00000020,0x00000001,0x0003001e,0x00000014,
	0x00000013,0x00040020,0x00000015,0x00000009,0x00000014,0x0004003b,0x00000015,0x00000016,
	0x00000009,0x0004002b,0x00000013,0x00000017,0x00000000,0x00040020,0x00000018,0x00000009,
	0x00000013,0x00040017,0x0000001c,0x00000006,0x00000003,0x00040015,0x00000021,0x00000020,
	0x00000000,0x0004002b,0x00000021,0x00000022,0x00000000,0x00040020,0x00000024,0x00000003,
	0x00000006,0x0004003b,0x00000010,0x00000027,0x00000001,0x0004002b,0x00000021,0x00000030,
	0x00000001,0x0004003b,0x00000010,0x00000034,0x00000001,0x0004002b,0x00000021,0x0000003d,
	0x00000002,0x0004002b,0x00000006,0x00000040,0x3f800000,0x0004002b,0x00000021,0x00000041,
	0x00000003,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
	0x0004003d,0x0000000b,0x0000000e,0x0000000d,0x0004003d,0x0000000f,0x00000012,0x00000011,
	0x00050041,0x00000018,0x00000019,0x00000016,0x00000017,0x0004003d,0x00000013,0x0000001a,
	0x00000019,0x0004006f,0x00000006,0x0000001b,0x0000001a,0x00050051,0x00000006,0x0000001d,
	0x00000012,0x00000000,0x00050051,0x00000006,0x0000001e,0x00000012,0x00000001,0x00060050,
	0x0000001c,0x0000001f,0x0000001d,0x0000001e,0x0000001b,0x00050057,0x00000007,0x00000020,
	0x0000000e,0x0000001f,0x00050051,0x00000006,0x00000023,0x00000020,0x00000000,0x00050041,
	0x00000024,0x00000025,0x00000009,0x00000022,0x0003003e,0x00000025,0x00000023,0x0004003d,
	0x0000000b,0x00000026,0x0000000d,0x0004003d,0x0000000f,0x00000028,0x00000027,0x00050041,
	0x00000018,0x00000029,0x00000016,0x00000017,0x0004003d,0x00000013,0x0000002a,0x00000029,
	0x0004006f,0x00000006,0x0000002b,0x0000002a,0x00050051,0x00000006,0x0000002c,0x00000028,
	0x00000000,0x00050051,0x00000006,0x0000002d,0x00000028,0x00000001,0x00060050,0x0000001c,
	0x0000002e,0x0000002c,0x0000002d,0x0000002b,0x00050057,0x00000007,0x0000002f,0x00000026,
	0x0000002e,0x00050051,0x00000006,0x00000031,0x0000002f,0x00000001,0x00050041,0x00000024,
	0x00000032,0x00000009,0x00000030,0x0003003e,0x00000032,0x00000031,0x0004003d,0x0000000b,
	0x00000033,0x0000000d,0x0004003d,0x0000000f,0x00000035,0x00000034,0x00050041,0x00000018,
	0x00000036,0x00000016,0x00000017,0x0004003d,0x00000013,0x00000037,0x00000036,0x0004006f,
	0x00000006,0x00000038,0x00000037,0x00050051,0x00000006,0x00000039,0x00000035,0x00000000,
	0x00050051,0x00000006,0x0000003a,0x00000035,0x00000001,0x00060050,0x0000001c,0x0000003b,
	0x00000039,0x0000003a,0x00000038,0x00050057,0x00000007,0x0000003c,0x00000033,0x0000003b,
	0x00050051,0x00000006,0x0000003e,0x0000003c,0x00000002,0x00050041,0x00000024,0x0000003f,
	0x00000009,0x0000003d,0x0003003e,0x0000003f,0x0000003e,0x00050041,0x00000024,0x00000042,
	0x00000009,0x00000041,0x0003003e,0x00000042,0x00000040,0x000100fd,0x00010038
};

static void TimeWarpGraphics_Create( GpuContext_t * context, TimeWarpGraphics_t * graphics, GpuRenderPass_t * renderPass )
{
	const int numVertices = ( EYE_TILES_HIGH + 1 ) * ( EYE_TILES_WIDE + 1 );
	const int numIndices = EYE_TILES_HIGH * EYE_TILES_WIDE * 6;

	GpuTriangleIndex_t * indices = (GpuTriangleIndex_t *) malloc( numIndices * sizeof( indices[0] ) );
	for ( int y = 0; y < EYE_TILES_HIGH; y++ )
	{
		for ( int x = 0; x < EYE_TILES_WIDE; x++ )
		{
			const int offset = ( y * EYE_TILES_WIDE + x ) * 6;

			indices[offset + 0] = ( y + 0 ) * ( EYE_TILES_WIDE + 1 ) + ( x + 0 );
			indices[offset + 1] = ( y + 1 ) * ( EYE_TILES_WIDE + 1 ) + ( x + 0 );
			indices[offset + 2] = ( y + 0 ) * ( EYE_TILES_WIDE + 1 ) + ( x + 1 );

			indices[offset + 3] = ( y + 0 ) * ( EYE_TILES_WIDE + 1 ) + ( x + 1 );
			indices[offset + 4] = ( y + 1 ) * ( EYE_TILES_WIDE + 1 ) + ( x + 0 );
			indices[offset + 5] = ( y + 1 ) * ( EYE_TILES_WIDE + 1 ) + ( x + 1 );
		}
	}

	GpuVertexAttributeArrays_t vertexAttribs;
	GpuVertexAttributeArrays_Alloc( &vertexAttribs, numVertices,
							VERTEX_ATTRIBUTE_FLAG_POSITION |
							VERTEX_ATTRIBUTE_FLAG_UV0 |
							VERTEX_ATTRIBUTE_FLAG_UV1 |
							VERTEX_ATTRIBUTE_FLAG_UV2 );

	const int numMeshCoords = ( EYE_TILES_WIDE + 1 ) * ( EYE_TILES_HIGH + 1 );
	MeshCoord_t * meshCoordsBasePtr = (MeshCoord_t *) malloc( NUM_EYES * NUM_COLOR_CHANNELS * numMeshCoords * sizeof( MeshCoord_t ) );
	MeshCoord_t * meshCoords[NUM_EYES][NUM_COLOR_CHANNELS] =
	{
		{ meshCoordsBasePtr + 0 * numMeshCoords, meshCoordsBasePtr + 1 * numMeshCoords, meshCoordsBasePtr + 2 * numMeshCoords },
		{ meshCoordsBasePtr + 3 * numMeshCoords, meshCoordsBasePtr + 4 * numMeshCoords, meshCoordsBasePtr + 5 * numMeshCoords }
	};
	BuildDistortionMeshes( meshCoords, EYE_TILES_WIDE, EYE_TILES_HIGH, GetDefaultHmdInfo() );

#if defined( GRAPHICS_API_VULKAN )
	const float flipY = -1.0f;
#else
	const float flipY = 1.0f;
#endif

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		for ( int y = 0; y <= EYE_TILES_HIGH; y++ )
		{
			for ( int x = 0; x <= EYE_TILES_WIDE; x++ )
			{
				const int index = y * ( EYE_TILES_WIDE + 1 ) + x;
				vertexAttribs.position[index].x = ( -1.0f + eye + ( (float)x / EYE_TILES_WIDE ) );
				vertexAttribs.position[index].y = ( -1.0f + 2.0f * ( ( EYE_TILES_HIGH - (float)y ) / EYE_TILES_HIGH ) *
													( (float)( EYE_TILES_HIGH * TILE_PIXELS_HIGH ) / DISPLAY_PIXELS_HIGH ) ) * flipY;
				vertexAttribs.position[index].z = 0.0f;
				vertexAttribs.uv0[index].x = meshCoords[eye][0][index].x;
				vertexAttribs.uv0[index].y = meshCoords[eye][0][index].y;
				vertexAttribs.uv1[index].x = meshCoords[eye][1][index].x;
				vertexAttribs.uv1[index].y = meshCoords[eye][1][index].y;
				vertexAttribs.uv2[index].x = meshCoords[eye][2][index].x;
				vertexAttribs.uv2[index].y = meshCoords[eye][2][index].y;
			}
		}

		GpuGeometry_Create( context, &graphics->distortionMesh[eye], &vertexAttribs, numVertices, indices, numIndices );
	}

	free( meshCoordsBasePtr );
	GpuVertexAttributeArrays_Free( &vertexAttribs );
	free( indices );

	GpuGraphicsProgram_Create( context, &graphics->timeWarpSpatialProgram,
								PROGRAM( timeWarpSpatialVertexProgram ), sizeof( PROGRAM( timeWarpSpatialVertexProgram ) ),
								PROGRAM( timeWarpSpatialFragmentProgram ), sizeof( PROGRAM( timeWarpSpatialFragmentProgram ) ),
								timeWarpSpatialGraphicsProgramParms, ARRAY_SIZE( timeWarpSpatialGraphicsProgramParms ),
								VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_UV0 );
	GpuGraphicsProgram_Create( context, &graphics->timeWarpChromaticProgram,
								PROGRAM( timeWarpChromaticVertexProgram ), sizeof( PROGRAM( timeWarpChromaticVertexProgram ) ),
								PROGRAM( timeWarpChromaticFragmentProgram ), sizeof( PROGRAM( timeWarpChromaticFragmentProgram ) ),
								timeWarpChromaticGraphicsProgramParms, ARRAY_SIZE( timeWarpChromaticGraphicsProgramParms ),
								VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_UV0 |
								VERTEX_ATTRIBUTE_FLAG_UV1 | VERTEX_ATTRIBUTE_FLAG_UV2 );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		GpuGraphicsPipelineParms_t pipelineParms;
		GpuGraphicsPipelineParms_Init( &pipelineParms );

		pipelineParms.rop.depthTestEnable = false;
		pipelineParms.rop.depthWriteEnable = false;
		pipelineParms.renderPass = renderPass;
		pipelineParms.program = &graphics->timeWarpSpatialProgram;
		pipelineParms.geometry = &graphics->distortionMesh[eye];

		GpuGraphicsPipeline_Create( context, &graphics->timeWarpSpatialPipeline[eye], &pipelineParms );

		pipelineParms.program = &graphics->timeWarpChromaticProgram;
		pipelineParms.geometry = &graphics->distortionMesh[eye];

		GpuGraphicsPipeline_Create( context, &graphics->timeWarpChromaticPipeline[eye], &pipelineParms );
	}

	GpuTimer_Create( context, &graphics->timeWarpGpuTime );
}

static void TimeWarpGraphics_Destroy( GpuContext_t * context, TimeWarpGraphics_t * graphics )
{
	GpuTimer_Destroy( context, &graphics->timeWarpGpuTime );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		GpuGraphicsPipeline_Destroy( context, &graphics->timeWarpSpatialPipeline[eye] );
		GpuGraphicsPipeline_Destroy( context, &graphics->timeWarpChromaticPipeline[eye] );
	}

	GpuGraphicsProgram_Destroy( context, &graphics->timeWarpSpatialProgram );
	GpuGraphicsProgram_Destroy( context, &graphics->timeWarpChromaticProgram );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		GpuGeometry_Destroy( context, &graphics->distortionMesh[eye] );
	}
}

static void TimeWarpGraphics_Render( GpuCommandBuffer_t * commandBuffer, TimeWarpGraphics_t * graphics,
									GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass,
									const Microseconds_t refreshStartTime, const Microseconds_t refreshEndTime,
									const Matrix4x4f_t * projectionMatrix, const Matrix4x4f_t * viewMatrix,
									GpuTexture_t * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, TimeWarpBarGraphs_t * bargraphs,
									float cpuTimes[PROFILE_TIME_MAX], float gpuTimes[PROFILE_TIME_MAX] )
{
	const Microseconds_t t0 = GetTimeMicroseconds();

	Matrix4x4f_t displayRefreshStartViewMatrix;
	Matrix4x4f_t displayRefreshEndViewMatrix;
	GetHmdViewMatrixForTime( &displayRefreshStartViewMatrix, refreshStartTime );
	GetHmdViewMatrixForTime( &displayRefreshEndViewMatrix, refreshEndTime );

	Matrix4x4f_t timeWarpStartTransform;
	Matrix4x4f_t timeWarpEndTransform;
	CalculateTimeWarpTransform( &timeWarpStartTransform, projectionMatrix, viewMatrix, &displayRefreshStartViewMatrix );
	CalculateTimeWarpTransform( &timeWarpEndTransform, projectionMatrix, viewMatrix, &displayRefreshEndViewMatrix );

	const ScreenRect_t screenRect = GpuFramebuffer_GetRect( framebuffer );

	GpuCommandBuffer_BeginPrimary( commandBuffer );
	GpuCommandBuffer_BeginFramebuffer( commandBuffer, framebuffer, 0, GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );

	TimeWarpBarGraphs_UpdateGraphics( commandBuffer, bargraphs );

	GpuCommandBuffer_BeginTimer( commandBuffer, &graphics->timeWarpGpuTime );
	GpuCommandBuffer_BeginRenderPass( commandBuffer, renderPass, framebuffer, &screenRect );

	GpuCommandBuffer_SetViewport( commandBuffer, &screenRect );
	GpuCommandBuffer_SetScissor( commandBuffer, &screenRect );

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		GpuGraphicsCommand_t command;
		GpuGraphicsCommand_Init( &command );
		GpuGraphicsCommand_SetPipeline( &command, correctChromaticAberration ? &graphics->timeWarpChromaticPipeline[eye] : &graphics->timeWarpSpatialPipeline[eye] );
		GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM, &timeWarpStartTransform );
		GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM, &timeWarpEndTransform );
		GpuGraphicsCommand_SetParmInt( &command, GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER, &eyeArrayLayer[eye] );
		GpuGraphicsCommand_SetParmTextureSampled( &command, GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE, eyeTexture[eye] );

		GpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
	}

	const Microseconds_t t1 = GetTimeMicroseconds();

	TimeWarpBarGraphs_RenderGraphics( commandBuffer, bargraphs );

	GpuCommandBuffer_EndRenderPass( commandBuffer, renderPass );
	GpuCommandBuffer_EndTimer( commandBuffer, &graphics->timeWarpGpuTime );

	GpuCommandBuffer_EndFramebuffer( commandBuffer, framebuffer, 0, GPU_TEXTURE_USAGE_PRESENTATION );
	GpuCommandBuffer_EndPrimary( commandBuffer );

	GpuCommandBuffer_SubmitPrimary( commandBuffer );

	const Microseconds_t t2 = GetTimeMicroseconds();

	cpuTimes[PROFILE_TIME_TIME_WARP] = ( t1 - t0 ) * ( 1.0f / 1000.0f );
	cpuTimes[PROFILE_TIME_BAR_GRAPHS] = ( t2 - t1 ) * ( 1.0f / 1000.0f );
	cpuTimes[PROFILE_TIME_BLIT] = 0.0f;

	const float barGraphGpuTime = TimeWarpBarGraphs_GetGpuMillisecondsGraphics( bargraphs );

	gpuTimes[PROFILE_TIME_TIME_WARP] = GpuTimer_GetMilliseconds( &graphics->timeWarpGpuTime ) - barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BAR_GRAPHS] = barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BLIT] = 0.0f;
}

/*
================================================================================================================================

Time warp compute rendering.

TimeWarpCompute_t

static void TimeWarpCompute_Create( GpuContext_t * context, TimeWarpCompute_t * compute, GpuRenderPass_t * renderPass );
static void TimeWarpCompute_Destroy( GpuContext_t * context, TimeWarpCompute_t * compute );
static void TimeWarpCompute_Render( GpuCommandBuffer_t * commandBuffer, TimeWarpCompute_t * compute,
									GpuFramebuffer_t * framebuffer,
									const Microseconds_t refreshStartTime, const Microseconds_t refreshEndTime,
									const Matrix4x4f_t * projectionMatrix, const Matrix4x4f_t * viewMatrix,
									GpuTexture_t * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, TimeWarpBarGraphs_t * bargraphs,
									float cpuTimes[PROFILE_TIME_MAX], float gpuTimes[PROFILE_TIME_MAX] );

================================================================================================================================
*/

typedef struct
{
	GpuTexture_t			distortionImage[NUM_EYES][NUM_COLOR_CHANNELS];
	GpuTexture_t			timeWarpImage[NUM_EYES][NUM_COLOR_CHANNELS];
	GpuComputeProgram_t		timeWarpTransformProgram;
	GpuComputeProgram_t		timeWarpSpatialProgram;
	GpuComputeProgram_t		timeWarpChromaticProgram;
	GpuComputePipeline_t	timeWarpTransformPipeline;
	GpuComputePipeline_t	timeWarpSpatialPipeline;
	GpuComputePipeline_t	timeWarpChromaticPipeline;
	GpuTimer_t				timeWarpGpuTime;
} TimeWarpCompute_t;

enum
{
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_DST,
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_SRC,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_DIMENSIONS,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM
};

static const GpuProgramParm_t timeWarpTransformComputeProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,					GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_DST,		"dst",						0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_SRC,		"src",						1 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,		GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_DIMENSIONS,		"dimensions",				128 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE,				"eye",						136 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,	"timeWarpStartTransform",	0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,		"timeWarpEndTransform",		64 }
};

#define TRANSFORM_LOCAL_SIZE_X		8
#define TRANSFORM_LOCAL_SIZE_Y		8

static const char timeWarpTransformComputeProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"\n"
	"layout( local_size_x = " STRINGIFY( TRANSFORM_LOCAL_SIZE_X ) ", local_size_y = " STRINGIFY( TRANSFORM_LOCAL_SIZE_Y ) " ) in;\n"
	"\n"
	"layout( rgba16f, binding = 0 ) uniform writeonly image2D dst;\n"
	"layout( rgba32f, binding = 1 ) uniform readonly image2D src;\n"
	"layout( std140, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =   0 ) highp mat4 timeWarpStartTransform;\n"
	"	layout( offset =  64 ) highp mat4 timeWarpEndTransform;\n"
	"	layout( offset = 128 ) ivec2 dimensions;\n"
	"	layout( offset = 136 ) int eye;\n"
	"} pc;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	ivec2 mesh = ivec2( gl_GlobalInvocationID.xy );\n"
	"	if ( mesh.x >= pc.dimensions.x || mesh.y >= pc.dimensions.y )\n"
	"	{\n"
	"		return;\n"
	"	}\n"
	"	int eyeTilesWide = int( gl_NumWorkGroups.x * gl_WorkGroupSize.x ) - 1;\n"
	"	int eyeTilesHigh = int( gl_NumWorkGroups.y * gl_WorkGroupSize.y ) - 1;\n"
	"\n"
	"	vec2 coords = imageLoad( src, mesh ).xy;\n"
	"\n"
	"	float displayFraction = float( pc.eye * eyeTilesWide + mesh.x ) / ( float( eyeTilesWide ) * 2.0f );\n"		// landscape left-to-right
	"	vec3 start = vec3( pc.timeWarpStartTransform * vec4( coords, -1.0f, 1.0f ) );\n"
	"	vec3 end = vec3( pc.timeWarpEndTransform * vec4( coords, -1.0f, 1.0f ) );\n"
	"	vec3 cur = start + displayFraction * ( end - start );\n"
	"	float rcpZ = 1.0f / cur.z;\n"
	"\n"
	"	imageStore( dst, mesh, vec4( cur.xy * rcpZ, 0.0f, 0.0f ) );\n"
	"}\n";

static const unsigned int timeWarpTransformComputeProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x00000092,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000005,0x00000004,0x6e69616d,0x00000000,0x0000000d,0x00000030,0x00060010,
	0x00000004,0x00000011,0x00000008,0x00000008,0x00000001,0x00030003,0x00000002,0x000001b8,
	0x00040005,0x00000004,0x6e69616d,0x00000000,0x00040005,0x00000009,0x6873656d,0x00000000,
	0x00080005,0x0000000d,0x475f6c67,0x61626f6c,0x766e496c,0x7461636f,0x496e6f69,0x00000044,
	0x00060005,0x0000001a,0x68737550,0x736e6f43,0x746e6174,0x00000073,0x00090006,0x0000001a,
	0x00000000,0x656d6974,0x70726157,0x72617453,0x61725474,0x6f66736e,0x00006d72,0x00090006,
	0x0000001a,0x00000001,0x656d6974,0x70726157,0x54646e45,0x736e6172,0x6d726f66,0x00000000,
	0x00060006,0x0000001a,0x00000002,0x656d6964,0x6f69736e,0x0000736e,0x00040006,0x0000001a,
	0x00000003,0x00657965,0x00030005,0x0000001c,0x00006370,0x00060005,0x0000002f,0x54657965,
	0x73656c69,0x65646957,0x00000000,0x00070005,0x00000030,0x4e5f6c67,0x6f576d75,0x72476b72,
	0x7370756f,0x00000000,0x00060005,0x00000039,0x54657965,0x73656c69,0x68676948,0x00000000,
	0x00040005,0x00000041,0x726f6f63,0x00007364,0x00030005,0x00000044,0x00637273,0x00060005,
	0x0000004a,0x70736964,0x4679616c,0x74636172,0x006e6f69,0x00040005,0x0000005b,0x72617473,
	0x00000074,0x00030005,0x0000006b,0x00646e65,0x00030005,0x00000077,0x00727563,0x00040005,
	0x0000007f,0x5a706372,0x00000000,0x00030005,0x00000086,0x00747364,0x00040047,0x0000000d,
	0x0000000b,0x0000001c,0x00040048,0x0000001a,0x00000000,0x00000005,0x00050048,0x0000001a,
	0x00000000,0x00000023,0x00000000,0x00050048,0x0000001a,0x00000000,0x00000007,0x00000010,
	0x00040048,0x0000001a,0x00000001,0x00000005,0x00050048,0x0000001a,0x00000001,0x00000023,
	0x00000040,0x00050048,0x0000001a,0x00000001,0x00000007,0x00000010,0x00050048,0x0000001a,
	0x00000002,0x00000023,0x00000080,0x00050048,0x0000001a,0x00000003,0x00000023,0x00000088,
	0x00030047,0x0000001a,0x00000002,0x00040047,0x0000001c,0x00000022,0x00000000,0x00040047,
	0x00000030,0x0000000b,0x00000018,0x00040047,0x00000044,0x00000022,0x00000000,0x00040047,
	0x00000044,0x00000021,0x00000001,0x00040047,0x00000086,0x00000022,0x00000000,0x00040047,
	0x00000086,0x00000021,0x00000000,0x00040047,0x00000091,0x0000000b,0x00000019,0x00020013,
	0x00000002,0x00030021,0x00000003,0x00000002,0x00040015,0x00000006,0x00000020,0x00000001,
	0x00040017,0x00000007,0x00000006,0x00000002,0x00040020,0x00000008,0x00000007,0x00000007,
	0x00040015,0x0000000a,0x00000020,0x00000000,0x00040017,0x0000000b,0x0000000a,0x00000003,
	0x00040020,0x0000000c,0x00000001,0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,
	0x00040017,0x0000000e,0x0000000a,0x00000002,0x00020014,0x00000012,0x0004002b,0x0000000a,
	0x00000013,0x00000000,0x00040020,0x00000014,0x00000007,0x00000006,0x00030016,0x00000017,
	0x00000020,0x00040017,0x00000018,0x00000017,0x00000004,0x00040018,0x00000019,0x00000018,
	0x00000004,0x0006001e,0x0000001a,0x00000019,0x00000019,0x00000007,0x00000006,0x00040020,
	0x0000001b,0x00000009,0x0000001a,0x0004003b,0x0000001b,0x0000001c,0x00000009,0x0004002b,
	0x00000006,0x0000001d,0x00000002,0x00040020,0x0000001e,0x00000009,0x00000006,0x0004002b,
	0x0000000a,0x00000025,0x00000001,0x0004003b,0x0000000c,0x00000030,0x00000001,0x00040020,
	0x00000031,0x00000001,0x0000000a,0x0004002b,0x0000000a,0x00000034,0x00000008,0x0004002b,
	0x00000006,0x00000037,0x00000001,0x00040017,0x0000003f,0x00000017,0x00000002,0x00040020,
	0x00000040,0x00000007,0x0000003f,0x00090019,0x00000042,0x00000017,0x00000001,0x00000000,
	0x00000000,0x00000000,0x00000002,0x00000001,0x00040020,0x00000043,0x00000000,0x00000042,
	0x0004003b,0x00000043,0x00000044,0x00000000,0x00040020,0x00000049,0x00000007,0x00000017,
	0x0004002b,0x00000006,0x0000004b,0x00000003,0x0004002b,0x00000017,0x00000056,0x40000000,
	0x00040017,0x00000059,0x00000017,0x00000003,0x00040020,0x0000005a,0x00000007,0x00000059,
	0x0004002b,0x00000006,0x0000005c,0x00000000,0x00040020,0x0000005d,0x00000009,0x00000019,
	0x0004002b,0x00000017,0x00000061,0xbf800000,0x0004002b,0x00000017,0x00000062,0x3f800000,
	0x0004002b,0x0000000a,0x00000080,0x00000002,0x00090019,0x00000084,0x00000017,0x00000001,
	0x00000000,0x00000000,0x00000000,0x00000002,0x00000002,0x00040020,0x00000085,0x00000000,
	0x00000084,0x0004003b,0x00000085,0x00000086,0x00000000,0x0004002b,0x00000017,0x0000008d,
	0x00000000,0x0006002c,0x0000000b,0x00000091,0x00000034,0x00000034,0x00000025,0x00050036,
	0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000008,
	0x00000009,0x00000007,0x0004003b,0x00000014,0x0000002f,0x00000007,0x0004003b,0x00000014,
	0x00000039,0x00000007,0x0004003b,0x00000040,0x00000041,0x00000007,0x0004003b,0x00000049,
	0x0000004a,0x00000007,0x0004003b,0x0000005a,0x0000005b,0x00000007,0x0004003b,0x0000005a,
	0x0000006b,0x00000007,0x0004003b,0x0000005a,0x00000077,0x00000007,0x0004003b,0x00000049,
	0x0000007f,0x00000007,0x0004003d,0x0000000b,0x0000000f,0x0000000d,0x0007004f,0x0000000e,
	0x00000010,0x0000000f,0x0000000f,0x00000000,0x00000001,0x0004007c,0x00000007,0x00000011,
	0x00000010,0x0003003e,0x00000009,0x00000011,0x00050041,0x00000014,0x00000015,0x00000009,
	0x00000013,0x0004003d,0x00000006,0x00000016,0x00000015,0x00060041,0x0000001e,0x0000001f,
	0x0000001c,0x0000001d,0x00000013,0x0004003d,0x00000006,0x00000020,0x0000001f,0x000500af,
	0x00000012,0x00000021,0x00000016,0x00000020,0x000400a8,0x00000012,0x00000022,0x00000021,
	0x000300f7,0x00000024,0x00000000,0x000400fa,0x00000022,0x00000023,0x00000024,0x000200f8,
	0x00000023,0x00050041,0x00000014,0x00000026,0x00000009,0x00000025,0x0004003d,0x00000006,
	0x00000027,0x00000026,0x00060041,0x0000001e,0x00000028,0x0000001c,0x0000001d,0x00000025,
	0x0004003d,0x00000006,0x00000029,0x00000028,0x000500af,0x00000012,0x0000002a,0x00000027,
	0x00000029,0x000200f9,0x00000024,0x000200f8,0x00000024,0x000700f5,0x00000012,0x0000002b,
	0x00000021,0x00000005,0x0000002a,0x00000023,0x000300f7,0x0000002d,0x00000000,0x000400fa,
	0x0000002b,0x0000002c,0x0000002d,0x000200f8,0x0000002c,0x000100fd,0x000200f8,0x0000002d,
	0x00050041,0x00000031,0x00000032,0x00000030,0x00000013,0x0004003d,0x0000000a,0x00000033,
	0x00000032,0x00050084,0x0000000a,0x00000035,0x00000033,0x00000034,0x0004007c,0x00000006,
	0x00000036,0x00000035,0x00050082,0x00000006,0x00000038,0x00000036,0x00000037,0x0003003e,
	0x0000002f,0x00000038,0x00050041,0x00000031,0x0000003a,0x00000030,0x00000025,0x0004003d,
	0x0000000a,0x0000003b,0x0000003a,0x00050084,0x0000000a,0x0000003c,0x0000003b,0x00000034,
	0x0004007c,0x00000006,0x0000003d,0x0000003c,0x00050082,0x00000006,0x0000003e,0x0000003d,
	0x00000037,0x0003003e,0x00000039,0x0000003e,0x0004003d,0x00000042,0x00000045,0x00000044,
	0x0004003d,0x00000007,0x00000046,0x00000009,0x00050062,0x00000018,0x00000047,0x00000045,
	0x00000046,0x0007004f,0x0000003f,0x00000048,0x00000047,0x00000047,0x00000000,0x00000001,
	0x0003003e,0x00000041,0x00000048,0x00050041,0x0000001e,0x0000004c,0x0000001c,0x0000004b,
	0x0004003d,0x00000006,0x0000004d,0x0000004c,0x0004003d,0x00000006,0x0000004e,0x0000002f,
	0x00050084,0x00000006,0x0000004f,0x0000004d,0x0000004e,0x00050041,0x00000014,0x00000050,
	0x00000009,0x00000013,0x0004003d,0x00000006,0x00000051,0x00000050,0x00050080,0x00000006,
	0x00000052,0x0000004f,0x00000051,0x0004006f,0x00000017,0x00000053,0x00000052,0x0004003d,
	0x00000006,0x00000054,0x0000002f,0x0004006f,0x00000017,0x00000055,0x00000054,0x00050085,
	0x00000017,0x00000057,0x00000055,0x00000056,0x00050088,0x00000017,0x00000058,0x00000053,
	0x00000057,0x0003003e,0x0000004a,0x00000058,0x00050041,0x0000005d,0x0000005e,0x0000001c,
	0x0000005c,0x0004003d,0x00000019,0x0000005f,0x0000005e,0x0004003d,0x0000003f,0x00000060,
	0x00000041,0x00050051,0x00000017,0x00000063,0x00000060,0x00000000,0x00050051,0x00000017,
	0x00000064,0x00000060,0x00000001,0x00070050,0x00000018,0x00000065,0x00000063,0x00000064,
	0x00000061,0x00000062,0x00050091,0x00000018,0x00000066,0x0000005f,0x00000065,0x00050051,
	0x00000017,0x00000067,0x00000066,0x00000000,0x00050051,0x00000017,0x00000068,0x00000066,
	0x00000001,0x00050051,0x00000017,0x00000069,0x00000066,0x00000002,0x00060050,0x00000059,
	0x0000006a,0x00000067,0x00000068,0x00000069,0x0003003e,0x0000005b,0x0000006a,0x00050041,
	0x0000005d,0x0000006c,0x0000001c,0x00000037,0x0004003d,0x00000019,0x0000006d,0x0000006c,
	0x0004003d,0x0000003f,0x0000006e,0x00000041,0x00050051,0x00000017,0x0000006f,0x0000006e,
	0x00000000,0x00050051,0x00000017,0x00000070,0x0000006e,0x00000001,0x00070050,0x00000018,
	0x00000071,0x0000006f,0x00000070,0x00000061,0x00000062,0x00050091,0x00000018,0x00000072,
	0x0000006d,0x00000071,0x00050051,0x00000017,0x00000073,0x00000072,0x00000000,0x00050051,
	0x00000017,0x00000074,0x00000072,0x00000001,0x00050051,0x00000017,0x00000075,0x00000072,
	0x00000002,0x00060050,0x00000059,0x00000076,0x00000073,0x00000074,0x00000075,0x0003003e,
	0x0000006b,0x00000076,0x0004003d,0x00000059,0x00000078,0x0000005b,0x0004003d,0x00000017,
	0x00000079,0x0000004a,0x0004003d,0x00000059,0x0000007a,0x0000006b,0x0004003d,0x00000059,
	0x0000007b,0x0000005b,0x00050083,0x00000059,0x0000007c,0x0000007a,0x0000007b,0x0005008e,
	0x00000059,0x0000007d,0x0000007c,0x00000079,0x00050081,0x00000059,0x0000007e,0x00000078,
	0x0000007d,0x0003003e,0x00000077,0x0000007e,0x00050041,0x00000049,0x00000081,0x00000077,
	0x00000080,0x0004003d,0x00000017,0x00000082,0x00000081,0x00050088,0x00000017,0x00000083,
	0x00000062,0x00000082,0x0003003e,0x0000007f,0x00000083,0x0004003d,0x00000084,0x00000087,
	0x00000086,0x0004003d,0x00000007,0x00000088,0x00000009,0x0004003d,0x00000059,0x00000089,
	0x00000077,0x0007004f,0x0000003f,0x0000008a,0x00000089,0x00000089,0x00000000,0x00000001,
	0x0004003d,0x00000017,0x0000008b,0x0000007f,0x0005008e,0x0000003f,0x0000008c,0x0000008a,
	0x0000008b,0x00050051,0x00000017,0x0000008e,0x0000008c,0x00000000,0x00050051,0x00000017,
	0x0000008f,0x0000008c,0x00000001,0x00070050,0x00000018,0x00000090,0x0000008e,0x0000008f,
	0x0000008d,0x0000008d,0x00040063,0x00000087,0x00000088,0x00000090,0x000100fd,0x00010038
};

enum
{
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST,
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE,
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_R,
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G,
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_B,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET
};

static const GpuProgramParm_t timeWarpSpatialComputeProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,				GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST,				"dest",				0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE,			"eyeImage",			1 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G,		"warpImageG",		2 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE,		"imageScale",		0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS,		"imageBias",		8 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER,		"imageLayer",		16 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET,	"eyePixelOffset",	24 }
};

#define SPATIAL_LOCAL_SIZE_X		8
#define SPATIAL_LOCAL_SIZE_Y		8

static const char timeWarpSpatialComputeProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"\n"
	"layout( local_size_x = " STRINGIFY( SPATIAL_LOCAL_SIZE_X ) ", local_size_y = " STRINGIFY( SPATIAL_LOCAL_SIZE_Y ) " ) in;\n"
	"\n"
	"// imageScale = {	eyeTilesWide / ( eyeTilesWide + 1 ) / eyePixelsWide,\n"
	"//					eyeTilesHigh / ( eyeTilesHigh + 1 ) / eyePixelsHigh };\n"
	"// imageBias  = {	0.5f / ( eyeTilesWide + 1 ),\n"
	"//					0.5f / ( eyeTilesHigh + 1 ) };\n"
	"layout( rgba8, binding = 0 ) uniform writeonly image2D dest;\n"
	"layout( binding = 1 ) uniform highp sampler2DArray eyeImage;\n"
	"layout( binding = 2 ) uniform highp sampler2D warpImageG;\n"
	"layout( std140, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =  0 ) highp vec2 imageScale;\n"
	"	layout( offset =  8 ) highp vec2 imageBias;\n"
	"	layout( offset = 16 ) int imageLayer;\n"
	"	layout( offset = 24 ) ivec2 eyePixelOffset;\n"
	"} pc;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	vec2 tile = ( vec2( gl_GlobalInvocationID.xy ) + vec2( 0.5f ) ) * pc.imageScale + pc.imageBias;\n"
	"\n"
	"	vec2 eyeCoords = texture( warpImageG, tile ).xy;\n"
	"\n"
	"	vec4 rgba = texture( eyeImage, vec3( eyeCoords, pc.imageLayer ) );\n"
	"\n"
	"	imageStore( dest, ivec2( gl_GlobalInvocationID.xy ) + pc.eyePixelOffset, rgba );\n"
	"}\n";

static const unsigned int timeWarpSpatialComputeProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x00000050,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0006000f,0x00000005,0x00000004,0x6e69616d,0x00000000,0x0000000d,0x00060010,0x00000004,
	0x00000011,0x00000008,0x00000008,0x00000001,0x00030003,0x00000002,0x000001b8,0x00040005,
	0x00000004,0x6e69616d,0x00000000,0x00040005,0x00000009,0x656c6974,0x00000000,0x00080005,
	0x0000000d,0x475f6c67,0x61626f6c,0x766e496c,0x7461636f,0x496e6f69,0x00000044,0x00060005,
	0x00000017,0x68737550,0x736e6f43,0x746e6174,0x00000073,0x00060006,0x00000017,0x00000000,
	0x67616d69,0x61635365,0x0000656c,0x00060006,0x00000017,0x00000001,0x67616d69,0x61694265,
	0x00000073,0x00060006,0x00000017,0x00000002,0x67616d69,0x79614c65,0x00007265,0x00070006,
	0x00000017,0x00000003,0x50657965,0x6c657869,0x7366664f,0x00007465,0x00030005,0x00000019,
	0x00006370,0x00050005,0x00000023,0x43657965,0x64726f6f,0x00000073,0x00050005,0x00000027,
	0x70726177,0x67616d49,0x00004765,0x00040005,0x0000002f,0x61626772,0x00000000,0x00050005,
	0x00000033,0x49657965,0x6567616d,0x00000000,0x00040005,0x00000042,0x74736564,0x00000000,
	0x00040047,0x0000000d,0x0000000b,0x0000001c,0x00050048,0x00000017,0x00000000,0x00000023,
	0x00000000,0x00050048,0x00000017,0x00000001,0x00000023,0x00000008,0x00050048,0x00000017,
	0x00000002,0x00000023,0x00000010,0x00050048,0x00000017,0x00000003,0x00000023,0x00000018,
	0x00030047,0x00000017,0x00000002,0x00040047,0x00000019,0x00000022,0x00000000,0x00040047,
	0x00000027,0x00000022,0x00000000,0x00040047,0x00000027,0x00000021,0x00000002,0x00040047,
	0x00000033,0x00000022,0x00000000,0x00040047,0x00000033,0x00000021,0x00000001,0x00040047,
	0x00000042,0x00000022,0x00000000,0x00040047,0x00000042,0x00000021,0x00000000,0x00040047,
	0x0000004f,0x0000000b,0x00000019,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
	0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000002,0x00040020,
	0x00000008,0x00000007,0x00000007,0x00040015,0x0000000a,0x00000020,0x00000000,0x00040017,
	0x0000000b,0x0000000a,0x00000003,0x00040020,0x0000000c,0x00000001,0x0000000b,0x0004003b,
	0x0000000c,0x0000000d,0x00000001,0x00040017,0x0000000e,0x0000000a,0x00000002,0x0004002b,
	0x00000006,0x00000012,0x3f000000,0x0005002c,0x00000007,0x00000013,0x00000012,0x00000012,
	0x00040015,0x00000015,0x00000020,0x00000001,0x00040017,0x00000016,0x00000015,0x00000002,
	0x0006001e,0x00000017,0x00000007,0x00000007,0x00000015,0x00000016,0x00040020,0x00000018,
	0x00000009,0x00000017,0x0004003b,0x00000018,0x00000019,0x00000009,0x0004002b,0x00000015,
	0x0000001a,0x00000000,0x00040020,0x0000001b,0x00000009,0x00000007,0x0004002b,0x00000015,
	0x0000001f,0x00000001,0x00090019,0x00000024,0x00000006,0x00000001,0x00000000,0x00000000,
	0x00000000,0x00000001,0x00000000,0x0003001b,0x00000025,0x00000024,0x00040020,0x00000026,
	0x00000000,0x00000025,0x0004003b,0x00000026,0x00000027,0x00000000,0x00040017,0x0000002a,
	0x00000006,0x00000004,0x0004002b,0x00000006,0x0000002b,0x00000000,0x00040020,0x0000002e,
	0x00000007,0x0000002a,0x00090019,0x00000030,0x00000006,0x00000001,0x00000000,0x00000001,
	0x00000000,0x00000001,0x00000000,0x0003001b,0x00000031,0x00000030,0x00040020,0x00000032,
	0x00000000,0x00000031,0x0004003b,0x00000032,0x00000033,0x00000000,0x0004002b,0x00000015,
	0x00000036,0x00000002,0x00040020,0x00000037,0x00000009,0x00000015,0x00040017,0x0000003b,
	0x00000006,0x00000003,0x00090019,0x00000040,0x00000006,0x00000001,0x00000000,0x00000000,
	0x00000000,0x00000002,0x00000004,0x00040020,0x00000041,0x00000000,0x00000040,0x0004003b,
	0x00000041,0x00000042,0x00000000,0x0004002b,0x00000015,0x00000047,0x00000003,0x00040020,
	0x00000048,0x00000009,0x00000016,0x0004002b,0x0000000a,0x0000004d,0x00000008,0x0004002b,
	0x0000000a,0x0000004e,0x00000001,0x0006002c,0x0000000b,0x0000004f,0x0000004d,0x0000004d,
	0x0000004e,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
	0x0004003b,0x00000008,0x00000009,0x00000007,0x0004003b,0x00000008,0x00000023,0x00000007,
	0x0004003b,0x0000002e,0x0000002f,0x00000007,0x0004003d,0x0000000b,0x0000000f,0x0000000d,
	0x0007004f,0x0000000e,0x00000010,0x0000000f,0x0000000f,0x00000000,0x00000001,0x00040070,
	0x00000007,0x00000011,0x00000010,0x00050081,0x00000007,0x00000014,0x00000011,0x00000013,
	0x00050041,0x0000001b,0x0000001c,0x00000019,0x0000001a,0x0004003d,0x00000007,0x0000001d,
	0x0000001c,0x00050085,0x00000007,0x0000001e,0x00000014,0x0000001d,0x00050041,0x0000001b,
	0x00000020,0x00000019,0x0000001f,0x0004003d,0x00000007,0x00000021,0x00000020,0x00050081,
	0x00000007,0x00000022,0x0000001e,0x00000021,0x0003003e,0x00000009,0x00000022,0x0004003d,
	0x00000025,0x00000028,0x00000027,0x0004003d,0x00000007,0x00000029,0x00000009,0x00070058,
	0x0000002a,0x0000002c,0x00000028,0x00000029,0x00000002,0x0000002b,0x0007004f,0x00000007,
	0x0000002d,0x0000002c,0x0000002c,0x00000000,0x00000001,0x0003003e,0x00000023,0x0000002d,
	0x0004003d,0x00000031,0x00000034,0x00000033,0x0004003d,0x00000007,0x00000035,0x00000023,
	0x00050041,0x00000037,0x00000038,0x00000019,0x00000036,0x0004003d,0x00000015,0x00000039,
	0x00000038,0x0004006f,0x00000006,0x0000003a,0x00000039,0x00050051,0x00000006,0x0000003c,
	0x00000035,0x00000000,0x00050051,0x00000006,0x0000003d,0x00000035,0x00000001,0x00060050,
	0x0000003b,0x0000003e,0x0000003c,0x0000003d,0x0000003a,0x00070058,0x0000002a,0x0000003f,
	0x00000034,0x0000003e,0x00000002,0x0000002b,0x0003003e,0x0000002f,0x0000003f,0x0004003d,
	0x00000040,0x00000043,0x00000042,0x0004003d,0x0000000b,0x00000044,0x0000000d,0x0007004f,
	0x0000000e,0x00000045,0x00000044,0x00000044,0x00000000,0x00000001,0x0004007c,0x00000016,
	0x00000046,0x00000045,0x00050041,0x00000048,0x00000049,0x00000019,0x00000047,0x0004003d,
	0x00000016,0x0000004a,0x00000049,0x00050080,0x00000016,0x0000004b,0x00000046,0x0000004a,
	0x0004003d,0x0000002a,0x0000004c,0x0000002f,0x00040063,0x00000043,0x0000004b,0x0000004c,
	0x000100fd,0x00010038
};

static const GpuProgramParm_t timeWarpChromaticComputeProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,				GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST,				"dest",				0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE,			"eyeImage",			1 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_R,		"warpImageR",		2 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G,		"warpImageG",		3 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_B,		"warpImageB",		4 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE,		"imageScale",		0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS,		"imageBias",		8 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER,		"imageLayer",		16 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET,	"eyePixelOffset",	24 }
};

#define CHROMATIC_LOCAL_SIZE_X		8
#define CHROMATIC_LOCAL_SIZE_Y		8

static const char timeWarpChromaticComputeProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"\n"
	"layout( local_size_x = " STRINGIFY( CHROMATIC_LOCAL_SIZE_X ) ", local_size_y = " STRINGIFY( CHROMATIC_LOCAL_SIZE_Y ) " ) in;\n"
	"\n"
	"// imageScale = {	eyeTilesWide / ( eyeTilesWide + 1 ) / eyePixelsWide,\n"
	"//					eyeTilesHigh / ( eyeTilesHigh + 1 ) / eyePixelsHigh };\n"
	"// imageBias  = {	0.5f / ( eyeTilesWide + 1 ),\n"
	"//					0.5f / ( eyeTilesHigh + 1 ) };\n"
	"layout( rgba8, binding = 0 ) uniform writeonly image2D dest;\n"
	"layout( binding = 1 ) uniform highp sampler2DArray eyeImage;\n"
	"layout( binding = 2 ) uniform highp sampler2D warpImageR;\n"
	"layout( binding = 3 ) uniform highp sampler2D warpImageG;\n"
	"layout( binding = 4 ) uniform highp sampler2D warpImageB;\n"
	"layout( std140, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =  0 ) highp vec2 imageScale;\n"
	"	layout( offset =  8 ) highp vec2 imageBias;\n"
	"	layout( offset = 16 ) int imageLayer;\n"
	"	layout( offset = 24 ) ivec2 eyePixelOffset;\n"
	"} pc;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	vec2 tile = ( vec2( gl_GlobalInvocationID.xy ) + vec2( 0.5f ) ) * pc.imageScale + pc.imageBias;\n"
	"\n"
	"	vec2 eyeCoordsR = texture( warpImageR, tile ).xy;\n"
	"	vec2 eyeCoordsG = texture( warpImageG, tile ).xy;\n"
	"	vec2 eyeCoordsB = texture( warpImageB, tile ).xy;\n"
	"\n"
	"	vec4 rgba;\n"
	"	rgba.x = texture( eyeImage, vec3( eyeCoordsR, pc.imageLayer ) ).x;\n"
	"	rgba.y = texture( eyeImage, vec3( eyeCoordsG, pc.imageLayer ) ).y;\n"
	"	rgba.z = texture( eyeImage, vec3( eyeCoordsB, pc.imageLayer ) ).z;\n"
	"	rgba.w = 1.0f;\n"
	"\n"
	"	imageStore( dest, ivec2( gl_GlobalInvocationID.xy ) + pc.eyePixelOffset, rgba );\n"
	"}\n";

static const unsigned int timeWarpChromaticComputeProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x0000007a,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0006000f,0x00000005,0x00000004,0x6e69616d,0x00000000,0x0000000d,0x00060010,0x00000004,
	0x00000011,0x00000008,0x00000008,0x00000001,0x00030003,0x00000002,0x000001b8,0x00040005,
	0x00000004,0x6e69616d,0x00000000,0x00040005,0x00000009,0x656c6974,0x00000000,0x00080005,
	0x0000000d,0x475f6c67,0x61626f6c,0x766e496c,0x7461636f,0x496e6f69,0x00000044,0x00060005,
	0x00000017,0x68737550,0x736e6f43,0x746e6174,0x00000073,0x00060006,0x00000017,0x00000000,
	0x67616d69,0x61635365,0x0000656c,0x00060006,0x00000017,0x00000001,0x67616d69,0x61694265,
	0x00000073,0x00060006,0x00000017,0x00000002,0x67616d69,0x79614c65,0x00007265,0x00070006,
	0x00000017,0x00000003,0x50657965,0x6c657869,0x7366664f,0x00007465,0x00030005,0x00000019,
	0x00006370,0x00050005,0x00000023,0x43657965,0x64726f6f,0x00005273,0x00050005,0x00000027,
	0x70726177,0x67616d49,0x00005265,0x00050005,0x0000002e,0x43657965,0x64726f6f,0x00004773,
	0x00050005,0x0000002f,0x70726177,0x67616d49,0x00004765,0x00050005,0x00000034,0x43657965,
	0x64726f6f,0x00004273,0x00050005,0x00000035,0x70726177,0x67616d49,0x00004265,0x00040005,
	0x0000003b,0x61626772,0x00000000,0x00050005,0x0000003f,0x49657965,0x6567616d,0x00000000,
	0x00040005,0x0000006d,0x74736564,0x00000000,0x00040047,0x0000000d,0x0000000b,0x0000001c,
	0x00050048,0x00000017,0x00000000,0x00000023,0x00000000,0x00050048,0x00000017,0x00000001,
	0x00000023,0x00000008,0x00050048,0x00000017,0x00000002,0x00000023,0x00000010,0x00050048,
	0x00000017,0x00000003,0x00000023,0x00000018,0x00030047,0x00000017,0x00000002,0x00040047,
	0x00000019,0x00000022,0x00000000,0x00040047,0x00000027,0x00000022,0x00000000,0x00040047,
	0x00000027,0x00000021,0x00000002,0x00040047,0x0000002f,0x00000022,0x00000000,0x00040047,
	0x0000002f,0x00000021,0x00000003,0x00040047,0x00000035,0x00000022,0x00000000,0x00040047,
	0x00000035,0x00000021,0x00000004,0x00040047,0x0000003f,0x00000022,0x00000000,0x00040047,
	0x0000003f,0x00000021,0x00000001,0x00040047,0x0000006d,0x00000022,0x00000000,0x00040047,
	0x0000006d,0x00000021,0x00000000,0x00040047,0x00000079,0x0000000b,0x00000019,0x00020013,
	0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,
	0x00000007,0x00000006,0x00000002,0x00040020,0x00000008,0x00000007,0x00000007,0x00040015,
	0x0000000a,0x00000020,0x00000000,0x00040017,0x0000000b,0x0000000a,0x00000003,0x00040020,
	0x0000000c,0x00000001,0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040017,
	0x0000000e,0x0000000a,0x00000002,0x0004002b,0x00000006,0x00000012,0x3f000000,0x0005002c,
	0x00000007,0x00000013,0x00000012,0x00000012,0x00040015,0x00000015,0x00000020,0x00000001,
	0x00040017,0x00000016,0x00000015,0x00000002,0x0006001e,0x00000017,0x00000007,0x00000007,
	0x00000015,0x00000016,0x00040020,0x00000018,0x00000009,0x00000017,0x0004003b,0x00000018,
	0x00000019,0x00000009,0x0004002b,0x00000015,0x0000001a,0x00000000,0x00040020,0x0000001b,
	0x00000009,0x00000007,0x0004002b,0x00000015,0x0000001f,0x00000001,0x00090019,0x00000024,
	0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x0003001b,
	0x00000025,0x00000024,0x00040020,0x00000026,0x00000000,0x00000025,0x0004003b,0x00000026,
	0x00000027,0x00000000,0x00040017,0x0000002a,0x00000006,0x00000004,0x0004002b,0x00000006,
	0x0000002b,0x00000000,0x0004003b,0x00000026,0x0000002f,0x00000000,0x0004003b,0x00000026,
	0x00000035,0x00000000,0x00040020,0x0000003a,0x00000007,0x0000002a,0x00090019,0x0000003c,
	0x00000006,0x00000001,0x00000000,0x00000001,0x00000000,0x00000001,0x00000000,0x0003001b,
	0x0000003d,0x0000003c,0x00040020,0x0000003e,0x00000000,0x0000003d,0x0004003b,0x0000003e,
	0x0000003f,0x00000000,0x0004002b,0x00000015,0x00000042,0x00000002,0x00040020,0x00000043,
	0x00000009,0x00000015,0x00040017,0x00000047,0x00000006,0x00000003,0x0004002b,0x0000000a,
	0x0000004c,0x00000000,0x00040020,0x0000004e,0x00000007,0x00000006,0x0004002b,0x0000000a,
	0x00000059,0x00000001,0x0004002b,0x0000000a,0x00000065,0x00000002,0x0004002b,0x00000006,
	0x00000068,0x3f800000,0x0004002b,0x0000000a,0x00000069,0x00000003,0x00090019,0x0000006b,
	0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,0x00000002,0x00000004,0x00040020,
	0x0000006c,0x00000000,0x0000006b,0x0004003b,0x0000006c,0x0000006d,0x00000000,0x0004002b,
	0x00000015,0x00000072,0x00000003,0x00040020,0x00000073,0x00000009,0x00000016,0x0004002b,
	0x0000000a,0x00000078,0x00000008,0x0006002c,0x0000000b,0x00000079,0x00000078,0x00000078,
	0x00000059,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
	0x0004003b,0x00000008,0x00000009,0x00000007,0x0004003b,0x00000008,0x00000023,0x00000007,
	0x0004003b,0x00000008,0x0000002e,0x00000007,0x0004003b,0x00000008,0x00000034,0x00000007,
	0x0004003b,0x0000003a,0x0000003b,0x00000007,0x0004003d,0x0000000b,0x0000000f,0x0000000d,
	0x0007004f,0x0000000e,0x00000010,0x0000000f,0x0000000f,0x00000000,0x00000001,0x00040070,
	0x00000007,0x00000011,0x00000010,0x00050081,0x00000007,0x00000014,0x00000011,0x00000013,
	0x00050041,0x0000001b,0x0000001c,0x00000019,0x0000001a,0x0004003d,0x00000007,0x0000001d,
	0x0000001c,0x00050085,0x00000007,0x0000001e,0x00000014,0x0000001d,0x00050041,0x0000001b,
	0x00000020,0x00000019,0x0000001f,0x0004003d,0x00000007,0x00000021,0x00000020,0x00050081,
	0x00000007,0x00000022,0x0000001e,0x00000021,0x0003003e,0x00000009,0x00000022,0x0004003d,
	0x00000025,0x00000028,0x00000027,0x0004003d,0x00000007,0x00000029,0x00000009,0x00070058,
	0x0000002a,0x0000002c,0x00000028,0x00000029,0x00000002,0x0000002b,0x0007004f,0x00000007,
	0x0000002d,0x0000002c,0x0000002c,0x00000000,0x00000001,0x0003003e,0x00000023,0x0000002d,
	0x0004003d,0x00000025,0x00000030,0x0000002f,0x0004003d,0x00000007,0x00000031,0x00000009,
	0x00070058,0x0000002a,0x00000032,0x00000030,0x00000031,0x00000002,0x0000002b,0x0007004f,
	0x00000007,0x00000033,0x00000032,0x00000032,0x00000000,0x00000001,0x0003003e,0x0000002e,
	0x00000033,0x0004003d,0x00000025,0x00000036,0x00000035,0x0004003d,0x00000007,0x00000037,
	0x00000009,0x00070058,0x0000002a,0x00000038,0x00000036,0x00000037,0x00000002,0x0000002b,
	0x0007004f,0x00000007,0x00000039,0x00000038,0x00000038,0x00000000,0x00000001,0x0003003e,
	0x00000034,0x00000039,0x0004003d,0x0000003d,0x00000040,0x0000003f,0x0004003d,0x00000007,
	0x00000041,0x00000023,0x00050041,0x00000043,0x00000044,0x00000019,0x00000042,0x0004003d,
	0x00000015,0x00000045,0x00000044,0x0004006f,0x00000006,0x00000046,0x00000045,0x00050051,
	0x00000006,0x00000048,0x00000041,0x00000000,0x00050051,0x00000006,0x00000049,0x00000041,
	0x00000001,0x00060050,0x00000047,0x0000004a,0x00000048,0x00000049,0x00000046,0x00070058,
	0x0000002a,0x0000004b,0x00000040,0x0000004a,0x00000002,0x0000002b,0x00050051,0x00000006,
	0x0000004d,0x0000004b,0x00000000,0x00050041,0x0000004e,0x0000004f,0x0000003b,0x0000004c,
	0x0003003e,0x0000004f,0x0000004d,0x0004003d,0x0000003d,0x00000050,0x0000003f,0x0004003d,
	0x00000007,0x00000051,0x0000002e,0x00050041,0x00000043,0x00000052,0x00000019,0x00000042,
	0x0004003d,0x00000015,0x00000053,0x00000052,0x0004006f,0x00000006,0x00000054,0x00000053,
	0x00050051,0x00000006,0x00000055,0x00000051,0x00000000,0x00050051,0x00000006,0x00000056,
	0x00000051,0x00000001,0x00060050,0x00000047,0x00000057,0x00000055,0x00000056,0x00000054,
	0x00070058,0x0000002a,0x00000058,0x00000050,0x00000057,0x00000002,0x0000002b,0x00050051,
	0x00000006,0x0000005a,0x00000058,0x00000001,0x00050041,0x0000004e,0x0000005b,0x0000003b,
	0x00000059,0x0003003e,0x0000005b,0x0000005a,0x0004003d,0x0000003d,0x0000005c,0x0000003f,
	0x0004003d,0x00000007,0x0000005d,0x00000034,0x00050041,0x00000043,0x0000005e,0x00000019,
	0x00000042,0x0004003d,0x00000015,0x0000005f,0x0000005e,0x0004006f,0x00000006,0x00000060,
	0x0000005f,0x00050051,0x00000006,0x00000061,0x0000005d,0x00000000,0x00050051,0x00000006,
	0x00000062,0x0000005d,0x00000001,0x00060050,0x00000047,0x00000063,0x00000061,0x00000062,
	0x00000060,0x00070058,0x0000002a,0x00000064,0x0000005c,0x00000063,0x00000002,0x0000002b,
	0x00050051,0x00000006,0x00000066,0x00000064,0x00000002,0x00050041,0x0000004e,0x00000067,
	0x0000003b,0x00000065,0x0003003e,0x00000067,0x00000066,0x00050041,0x0000004e,0x0000006a,
	0x0000003b,0x00000069,0x0003003e,0x0000006a,0x00000068,0x0004003d,0x0000006b,0x0000006e,
	0x0000006d,0x0004003d,0x0000000b,0x0000006f,0x0000000d,0x0007004f,0x0000000e,0x00000070,
	0x0000006f,0x0000006f,0x00000000,0x00000001,0x0004007c,0x00000016,0x00000071,0x00000070,
	0x00050041,0x00000073,0x00000074,0x00000019,0x00000072,0x0004003d,0x00000016,0x00000075,
	0x00000074,0x00050080,0x00000016,0x00000076,0x00000071,0x00000075,0x0004003d,0x0000002a,
	0x00000077,0x0000003b,0x00040063,0x0000006e,0x00000076,0x00000077,0x000100fd,0x00010038
};

static void TimeWarpCompute_Create( GpuContext_t * context, TimeWarpCompute_t * compute, GpuRenderPass_t * renderPass )
{
	UNUSED_PARM( renderPass );

	memset( compute, 0, sizeof( TimeWarpCompute_t ) );

	const int numMeshCoords = ( EYE_TILES_WIDE + 1 ) * ( EYE_TILES_HIGH + 1 );
	MeshCoord_t * meshCoordsBasePtr = (MeshCoord_t *) malloc( NUM_EYES * NUM_COLOR_CHANNELS * numMeshCoords * sizeof( MeshCoord_t ) );
	MeshCoord_t * meshCoords[NUM_EYES][NUM_COLOR_CHANNELS] =
	{
		{ meshCoordsBasePtr + 0 * numMeshCoords, meshCoordsBasePtr + 1 * numMeshCoords, meshCoordsBasePtr + 2 * numMeshCoords },
		{ meshCoordsBasePtr + 3 * numMeshCoords, meshCoordsBasePtr + 4 * numMeshCoords, meshCoordsBasePtr + 5 * numMeshCoords }
	};
	BuildDistortionMeshes( meshCoords, EYE_TILES_WIDE, EYE_TILES_HIGH, GetDefaultHmdInfo() );

	float * rgbaFloat = (float *) malloc( numMeshCoords * 4 * sizeof( float ) );
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			for ( int i = 0; i < numMeshCoords; i++ )
			{
				rgbaFloat[i * 4 + 0] = meshCoords[eye][channel][i].x;
				rgbaFloat[i * 4 + 1] = meshCoords[eye][channel][i].y;
				rgbaFloat[i * 4 + 2] = 0.0f;
				rgbaFloat[i * 4 + 3] = 0.0f;
			}
			const size_t rgbaSize = numMeshCoords * 4 * sizeof( float );
			GpuTexture_Create2D( context, &compute->distortionImage[eye][channel],
								GPU_TEXTURE_FORMAT_R32G32B32A32_SFLOAT, EYE_TILES_WIDE + 1, EYE_TILES_HIGH + 1, 1,
								rgbaFloat, rgbaSize );
			GpuTexture_Create2D( context, &compute->timeWarpImage[eye][channel],
								GPU_TEXTURE_FORMAT_R16G16B16A16_SFLOAT, EYE_TILES_WIDE + 1, EYE_TILES_HIGH + 1, 1,
								NULL, 0 );
		}
	}
	free( rgbaFloat );

	free( meshCoordsBasePtr );

	GpuComputeProgram_Create( context, &compute->timeWarpTransformProgram,
								PROGRAM( timeWarpTransformComputeProgram ), sizeof( PROGRAM( timeWarpTransformComputeProgram ) ),
								timeWarpTransformComputeProgramParms, ARRAY_SIZE( timeWarpTransformComputeProgramParms ) );
	GpuComputeProgram_Create( context, &compute->timeWarpSpatialProgram,
								PROGRAM( timeWarpSpatialComputeProgram ), sizeof( PROGRAM( timeWarpSpatialComputeProgram ) ),
								timeWarpSpatialComputeProgramParms, ARRAY_SIZE( timeWarpSpatialComputeProgramParms ) );
	GpuComputeProgram_Create( context, &compute->timeWarpChromaticProgram,
								PROGRAM( timeWarpChromaticComputeProgram ), sizeof( PROGRAM( timeWarpChromaticComputeProgram ) ),
								timeWarpChromaticComputeProgramParms, ARRAY_SIZE( timeWarpChromaticComputeProgramParms ) );

	GpuComputePipeline_Create( context, &compute->timeWarpTransformPipeline, &compute->timeWarpTransformProgram );
	GpuComputePipeline_Create( context, &compute->timeWarpSpatialPipeline, &compute->timeWarpSpatialProgram );
	GpuComputePipeline_Create( context, &compute->timeWarpChromaticPipeline, &compute->timeWarpChromaticProgram );

	GpuTimer_Create( context, &compute->timeWarpGpuTime );
}

static void TimeWarpCompute_Destroy( GpuContext_t * context, TimeWarpCompute_t * compute )
{
	GpuTimer_Destroy( context, &compute->timeWarpGpuTime );

	GpuComputePipeline_Destroy( context, &compute->timeWarpTransformPipeline );
	GpuComputePipeline_Destroy( context, &compute->timeWarpSpatialPipeline );
	GpuComputePipeline_Destroy( context, &compute->timeWarpChromaticPipeline );

	GpuComputeProgram_Destroy( context, &compute->timeWarpTransformProgram );
	GpuComputeProgram_Destroy( context, &compute->timeWarpSpatialProgram );
	GpuComputeProgram_Destroy( context, &compute->timeWarpChromaticProgram );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			GpuTexture_Destroy( context, &compute->distortionImage[eye][channel] );
			GpuTexture_Destroy( context, &compute->timeWarpImage[eye][channel] );
		}
	}

	memset( compute, 0, sizeof( TimeWarpCompute_t ) );
}

static void TimeWarpCompute_Render( GpuCommandBuffer_t * commandBuffer, TimeWarpCompute_t * compute,
									GpuFramebuffer_t * framebuffer,
									const Microseconds_t refreshStartTime, const Microseconds_t refreshEndTime,
									const Matrix4x4f_t * projectionMatrix, const Matrix4x4f_t * viewMatrix,
									GpuTexture_t * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, TimeWarpBarGraphs_t * bargraphs,
									float cpuTimes[PROFILE_TIME_MAX], float gpuTimes[PROFILE_TIME_MAX] )
{
	const Microseconds_t t0 = GetTimeMicroseconds();

	Matrix4x4f_t displayRefreshStartViewMatrix;
	Matrix4x4f_t displayRefreshEndViewMatrix;
	GetHmdViewMatrixForTime( &displayRefreshStartViewMatrix, refreshStartTime );
	GetHmdViewMatrixForTime( &displayRefreshEndViewMatrix, refreshEndTime );

	Matrix4x4f_t timeWarpStartTransform;
	Matrix4x4f_t timeWarpEndTransform;
	CalculateTimeWarpTransform( &timeWarpStartTransform, projectionMatrix, viewMatrix, &displayRefreshStartViewMatrix );
	CalculateTimeWarpTransform( &timeWarpEndTransform, projectionMatrix, viewMatrix, &displayRefreshEndViewMatrix );

	GpuCommandBuffer_BeginPrimary( commandBuffer );
	GpuCommandBuffer_BeginFramebuffer( commandBuffer, framebuffer, 0, GPU_TEXTURE_USAGE_STORAGE );

	GpuCommandBuffer_BeginTimer( commandBuffer, &compute->timeWarpGpuTime );

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			GpuCommandBuffer_ChangeTextureUsage( commandBuffer, &compute->timeWarpImage[eye][channel], GPU_TEXTURE_USAGE_STORAGE );
			GpuCommandBuffer_ChangeTextureUsage( commandBuffer, &compute->distortionImage[eye][channel], GPU_TEXTURE_USAGE_STORAGE );
		}
	}

	const Vector2i_t dimensions = { EYE_TILES_WIDE + 1, EYE_TILES_HIGH + 1 };
	const int eyeIndex[NUM_EYES] = { 0, 1 };

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			GpuComputeCommand_t command;
			GpuComputeCommand_Init( &command );
			GpuComputeCommand_SetPipeline( &command, &compute->timeWarpTransformPipeline );
			GpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_DST, &compute->timeWarpImage[eye][channel] );
			GpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_SRC, &compute->distortionImage[eye][channel] );
			GpuComputeCommand_SetParmFloatMatrix4x4( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM, &timeWarpStartTransform );
			GpuComputeCommand_SetParmFloatMatrix4x4( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM, &timeWarpEndTransform );
			GpuComputeCommand_SetParmIntVector2( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_DIMENSIONS, &dimensions );
			GpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE, &eyeIndex[eye] );
			GpuComputeCommand_SetDimensions( &command, ( dimensions.x + TRANSFORM_LOCAL_SIZE_X - 1 ) / TRANSFORM_LOCAL_SIZE_X, 
														( dimensions.y + TRANSFORM_LOCAL_SIZE_Y - 1 ) / TRANSFORM_LOCAL_SIZE_Y, 1 );

			GpuCommandBuffer_SubmitComputeCommand( commandBuffer, &command );
		}
	}

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			GpuCommandBuffer_ChangeTextureUsage( commandBuffer, &compute->timeWarpImage[eye][channel], GPU_TEXTURE_USAGE_SAMPLED );
		}
	}

	const int screenWidth = GpuFramebuffer_GetWidth( framebuffer );
	const int screenHeight = GpuFramebuffer_GetHeight( framebuffer );
	const int eyePixelsWide = screenWidth / NUM_EYES;
	const int eyePixelsHigh = screenHeight * EYE_TILES_HIGH * TILE_PIXELS_HIGH / DISPLAY_PIXELS_HIGH;
	const Vector2f_t imageScale =
	{
		(float)EYE_TILES_WIDE / ( EYE_TILES_WIDE + 1 ) / eyePixelsWide,
		(float)EYE_TILES_HIGH / ( EYE_TILES_HIGH + 1 ) / eyePixelsHigh
	};
	const Vector2f_t imageBias =
	{
		0.5f / ( EYE_TILES_WIDE + 1 ),
		0.5f / ( EYE_TILES_HIGH + 1 )
	};
	const Vector2i_t eyePixelOffset[NUM_EYES] =
	{
#if defined( GRAPHICS_API_VULKAN )
		{ 0 * eyePixelsWide, screenHeight - eyePixelsHigh },
		{ 1 * eyePixelsWide, screenHeight - eyePixelsHigh }
#else
		{ 0 * eyePixelsWide, eyePixelsHigh },
		{ 1 * eyePixelsWide, eyePixelsHigh }
#endif
	};

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		assert( screenWidth % ( correctChromaticAberration ? CHROMATIC_LOCAL_SIZE_X : SPATIAL_LOCAL_SIZE_X ) == 0 );
		assert( screenHeight % ( correctChromaticAberration ? CHROMATIC_LOCAL_SIZE_Y : SPATIAL_LOCAL_SIZE_Y ) == 0 );

		GpuComputeCommand_t command;
		GpuComputeCommand_Init( &command );
		GpuComputeCommand_SetPipeline( &command, correctChromaticAberration ? &compute->timeWarpChromaticPipeline : &compute->timeWarpSpatialPipeline );
		GpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST, GpuFramebuffer_GetColorTexture( framebuffer ) );
		GpuComputeCommand_SetParmTextureSampled( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE, eyeTexture[eye] );
		GpuComputeCommand_SetParmTextureSampled( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_R, &compute->timeWarpImage[eye][0] );
		GpuComputeCommand_SetParmTextureSampled( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G, &compute->timeWarpImage[eye][1] );
		GpuComputeCommand_SetParmTextureSampled( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_B, &compute->timeWarpImage[eye][2] );
		GpuComputeCommand_SetParmFloatVector2( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE, &imageScale );
		GpuComputeCommand_SetParmFloatVector2( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS, &imageBias );
		GpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER, &eyeArrayLayer[eye] );
		GpuComputeCommand_SetParmIntVector2( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET, &eyePixelOffset[eye] );
		GpuComputeCommand_SetDimensions( &command, screenWidth / ( correctChromaticAberration ? CHROMATIC_LOCAL_SIZE_X : SPATIAL_LOCAL_SIZE_X ) / 2,
													screenHeight / ( correctChromaticAberration ? CHROMATIC_LOCAL_SIZE_Y : SPATIAL_LOCAL_SIZE_Y ), 1 );

		GpuCommandBuffer_SubmitComputeCommand( commandBuffer, &command );
	}

	const Microseconds_t t1 = GetTimeMicroseconds();

	TimeWarpBarGraphs_UpdateCompute( commandBuffer, bargraphs );
	TimeWarpBarGraphs_RenderCompute( commandBuffer, bargraphs, framebuffer );

	GpuCommandBuffer_EndTimer( commandBuffer, &compute->timeWarpGpuTime );

	GpuCommandBuffer_EndFramebuffer( commandBuffer, framebuffer, 0, GPU_TEXTURE_USAGE_PRESENTATION );
	GpuCommandBuffer_EndPrimary( commandBuffer );

	GpuCommandBuffer_SubmitPrimary( commandBuffer );

	const Microseconds_t t2 = GetTimeMicroseconds();

	cpuTimes[PROFILE_TIME_TIME_WARP] = ( t1 - t0 ) * ( 1.0f / 1000.0f );
	cpuTimes[PROFILE_TIME_BAR_GRAPHS] = ( t2 - t1 ) * ( 1.0f / 1000.0f );
	cpuTimes[PROFILE_TIME_BLIT] = 0.0f;

	const float barGraphGpuTime = TimeWarpBarGraphs_GetGpuMillisecondsCompute( bargraphs );

	gpuTimes[PROFILE_TIME_TIME_WARP] = GpuTimer_GetMilliseconds( &compute->timeWarpGpuTime ) - barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BAR_GRAPHS] = barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BLIT] = 0.0f;
}

/*
================================================================================================================================

Time warp rendering.

TimeWarp_t

static void TimeWarp_Create( TimeWarp_t * timeWarp, GpuWindow_t * window );
static void TimeWarp_Destroy( TimeWarp_t * timeWarp, GpuWindow_t * window );
static void TimeWarp_Reset( TimeWarp_t * timeWarp );

static void TimeWarp_SetBarGraphState( TimeWarp_t * timeWarp, const BarGraphState_t state );
static void TimeWarp_CycleBarGraphState( TimeWarp_t * timeWarp );
static void TimeWarp_SetImplementation( TimeWarp_t * timeWarp, const TimeWarpImplementation_t implementation );
static void TimeWarp_CycleImplementation( TimeWarp_t * timeWarp );
static void TimeWarp_SetChromaticAberrationCorrection( TimeWarp_t * timeWarp, const bool set );
static void TimeWarp_ToggleChromaticAberrationCorrection( TimeWarp_t * timeWarp );
static void TimeWarp_SetMultiView( TimeWarp_t * timeWarp, const bool enabled );
static void TimeWarp_SetDrawCallLevel( TimeWarp_t * timeWarp, const int level );
static void TimeWarp_SetTriangleLevel( TimeWarp_t * timeWarp, const int level );
static void TimeWarp_SetFragmentLevel( TimeWarp_t * timeWarp, const int level );

static void TimeWarp_PresentNewEyeTextures( TimeWarp_t * timeWarp,
											const Matrix4x4_t * projectionMatrix, const Matrix4x4f_t * viewMatrix,
											GpuTexture_t * eyeTexture[2], GpuFence_t * eyeCompletionFence[2],
											int eyeArrayLayer[2], float eyeTexturesCpuTime, float eyeTexturesGpuTime );
static void TimeWarp_Render( TimeWarp_t * timeWarp, GpuWindow_t * window );

================================================================================================================================
*/

#define AVARGE_FRAME_RATE_FRAMES		20

typedef enum
{
	TIMEWARP_IMPLEMENTATION_GRAPHICS,
	TIMEWARP_IMPLEMENTATION_COMPUTE,
	TIMEWARP_IMPLEMENTATION_MAX
} TimeWarpImplementation_t;

typedef struct
{
	int							index;
	Matrix4x4f_t				projectionMatrix;
	Matrix4x4f_t				viewMatrix;
	GpuTexture_t *				texture[NUM_EYES];
	GpuFence_t *				completionFence[NUM_EYES];
	int							arrayLayer[NUM_EYES];
	float						cpuTime;
	float						gpuTime;
} EyeTextures_t;

typedef struct
{
	GpuTexture_t				defaultTexture;
	GpuTexture_t *				eyeTexture[NUM_EYES];
	int							eyeArrayLayer[NUM_EYES];
	Matrix4x4f_t				projectionMatrix;
	Matrix4x4f_t				viewMatrix;

	Mutex_t						newEyeTexturesMutex;
	Signal_t					newEyeTexturesConsumed;
	EyeTextures_t				newEyeTextures;

	int							eyeTexturesPresentIndex;
	int							eyeTexturesConsumedIndex;

	float						refreshRate;
	Microseconds_t				frameCpuTime[AVARGE_FRAME_RATE_FRAMES];
	int							eyeTexturesFrames[AVARGE_FRAME_RATE_FRAMES];
	int							timeWarpFrames;
	float						cpuTimes[PROFILE_TIME_MAX];
	float						gpuTimes[PROFILE_TIME_MAX];

	GpuRenderPass_t				renderPass;
	GpuFramebuffer_t			framebuffer;
	GpuCommandBuffer_t			commandBuffer;
	bool						correctChromaticAberration;
	TimeWarpImplementation_t	implementation;
	TimeWarpGraphics_t			graphics;
	TimeWarpCompute_t			compute;
	TimeWarpBarGraphs_t			bargraphs;
} TimeWarp_t;

static void TimeWarp_Reset( TimeWarp_t * timeWarp )
{
	Matrix4x4f_CreateIdentity( &timeWarp->viewMatrix );
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		timeWarp->eyeTexture[eye] = &timeWarp->defaultTexture;
		timeWarp->eyeArrayLayer[eye] = eye;
	}

	Mutex_Create( &timeWarp->newEyeTexturesMutex );
	Signal_Create( &timeWarp->newEyeTexturesConsumed, true );
	Signal_Raise( &timeWarp->newEyeTexturesConsumed );

	timeWarp->newEyeTextures.index = 0;
	Matrix4x4f_CreateIdentity( &timeWarp->newEyeTextures.viewMatrix );
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		timeWarp->newEyeTextures.texture[eye] = &timeWarp->defaultTexture;
		timeWarp->newEyeTextures.completionFence[eye] = NULL;
	}
	timeWarp->newEyeTextures.cpuTime = 0.0f;
	timeWarp->newEyeTextures.gpuTime = 0.0f;

	timeWarp->eyeTexturesPresentIndex = 1;
	timeWarp->eyeTexturesConsumedIndex = 0;
}

static void TimeWarp_Create( TimeWarp_t * timeWarp, GpuWindow_t * window )
{
	GpuTexture_CreateDefault( &window->context, &timeWarp->defaultTexture, GPU_TEXTURE_DEFAULT_CIRCLES, 1024, 1024, 1, 2, 1, false, true );
	GpuTexture_SetWrapMode( &window->context, &timeWarp->defaultTexture, GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER );

	TimeWarp_Reset( timeWarp );

	timeWarp->refreshRate = window->windowRefreshRate;
	for ( int i = 0; i < AVARGE_FRAME_RATE_FRAMES; i++ )
	{
		timeWarp->frameCpuTime[i] = 0;
		timeWarp->eyeTexturesFrames[i] = 0;
	}
	timeWarp->timeWarpFrames = 0;

	GpuRenderPass_Create( &window->context, &timeWarp->renderPass, window->colorFormat, window->depthFormat,
							GPU_RENDERPASS_TYPE_INLINE,
							GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER );
	GpuFramebuffer_CreateFromSwapchain( window, &timeWarp->framebuffer, &timeWarp->renderPass );
	GpuCommandBuffer_Create( &window->context, &timeWarp->commandBuffer, GPU_COMMAND_BUFFER_TYPE_PRIMARY, GpuFramebuffer_GetBufferCount( &timeWarp->framebuffer ) );

	timeWarp->correctChromaticAberration = false;
	timeWarp->implementation = TIMEWARP_IMPLEMENTATION_GRAPHICS;
	TimeWarpGraphics_Create( &window->context, &timeWarp->graphics, &timeWarp->renderPass );
	TimeWarpCompute_Create( &window->context, &timeWarp->compute, &timeWarp->renderPass );
	TimeWarpBarGraphs_Create( &window->context, &timeWarp->bargraphs, &timeWarp->renderPass );

	memset( timeWarp->cpuTimes, 0, sizeof( timeWarp->cpuTimes ) );
	memset( timeWarp->gpuTimes, 0, sizeof( timeWarp->gpuTimes ) );
}

static void TimeWarp_Destroy( TimeWarp_t * timeWarp, GpuWindow_t * window )
{
	GpuContext_WaitIdle( &window->context );

	TimeWarpGraphics_Destroy( &window->context, &timeWarp->graphics );
	TimeWarpCompute_Destroy( &window->context, &timeWarp->compute );
	TimeWarpBarGraphs_Destroy( &window->context, &timeWarp->bargraphs );

	GpuCommandBuffer_Destroy( &window->context, &timeWarp->commandBuffer );
	GpuFramebuffer_Destroy( &window->context, &timeWarp->framebuffer );
	GpuRenderPass_Destroy( &window->context, &timeWarp->renderPass );

	Mutex_Destroy( &timeWarp->newEyeTexturesMutex );
	Signal_Destroy( &timeWarp->newEyeTexturesConsumed );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		timeWarp->eyeTexture[eye] = NULL;
		timeWarp->eyeArrayLayer[eye] = 0;
	}
	GpuTexture_Destroy( &window->context, &timeWarp->defaultTexture );
}

static void TimeWarp_SetBarGraphState( TimeWarp_t * timeWarp, const BarGraphState_t state )
{
	timeWarp->bargraphs.barGraphState = state;
}

static void TimeWarp_CycleBarGraphState( TimeWarp_t * timeWarp )
{
	timeWarp->bargraphs.barGraphState = (BarGraphState_t)( ( timeWarp->bargraphs.barGraphState + 1 ) % 3 );
}

static void TimeWarp_SetImplementation( TimeWarp_t * timeWarp, const TimeWarpImplementation_t implementation )
{
	timeWarp->implementation = implementation;
	const float delta = ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_GRAPHICS ) ? 0.0f : 1.0f;
	BarGraph_AddBar( &timeWarp->bargraphs.timeWarpImplementationBarGraph, 0, delta, &colorRed, false );
}

static void TimeWarp_CycleImplementation( TimeWarp_t * timeWarp )
{
	timeWarp->implementation = (TimeWarpImplementation_t)( ( timeWarp->implementation + 1 ) % TIMEWARP_IMPLEMENTATION_MAX );
	const float delta = ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_GRAPHICS ) ? 0.0f : 1.0f;
	BarGraph_AddBar( &timeWarp->bargraphs.timeWarpImplementationBarGraph, 0, delta, &colorRed, false );
}

static void TimeWarp_SetChromaticAberrationCorrection( TimeWarp_t * timeWarp, const bool set )
{
	timeWarp->correctChromaticAberration = set;
	BarGraph_AddBar( &timeWarp->bargraphs.correctChromaticAberrationBarGraph, 0, timeWarp->correctChromaticAberration ? 1.0f : 0.0f, &colorRed, false );
}

static void TimeWarp_ToggleChromaticAberrationCorrection( TimeWarp_t * timeWarp )
{
	timeWarp->correctChromaticAberration = !timeWarp->correctChromaticAberration;
	BarGraph_AddBar( &timeWarp->bargraphs.correctChromaticAberrationBarGraph, 0, timeWarp->correctChromaticAberration ? 1.0f : 0.0f, &colorRed, false );
}

static void TimeWarp_SetMultiView( TimeWarp_t * timeWarp, const bool enabled )
{
	BarGraph_AddBar( &timeWarp->bargraphs.multiViewBarGraph, 0, enabled ? 1.0f : 0.0f, &colorRed, false );
}

static void TimeWarp_SetDrawCallLevel( TimeWarp_t * timeWarp, const int level )
{
	const Vector4f_t * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		BarGraph_AddBar( &timeWarp->bargraphs.sceneDrawCallLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void TimeWarp_SetTriangleLevel( TimeWarp_t * timeWarp, const int level )
{
	const Vector4f_t * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		BarGraph_AddBar( &timeWarp->bargraphs.sceneTriangleLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void TimeWarp_SetFragmentLevel( TimeWarp_t * timeWarp, const int level )
{
	const Vector4f_t * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		BarGraph_AddBar( &timeWarp->bargraphs.sceneFragmentLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void TimeWarp_PresentNewEyeTextures( TimeWarp_t * timeWarp,
											const Matrix4x4f_t * projectionMatrix, const Matrix4x4f_t * viewMatrix,
											GpuTexture_t * eyeTexture[2], GpuFence_t * eyeCompletionFence[2],
											int eyeArrayLayer[2], float eyeTexturesCpuTime, float eyeTexturesGpuTime )
{
	EyeTextures_t newEyeTextures;
	newEyeTextures.index = timeWarp->eyeTexturesPresentIndex++;
	newEyeTextures.projectionMatrix = *projectionMatrix;
	newEyeTextures.viewMatrix = *viewMatrix;
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		newEyeTextures.texture[eye] = eyeTexture[eye];
		newEyeTextures.completionFence[eye] = eyeCompletionFence[eye];
		newEyeTextures.arrayLayer[eye] = eyeArrayLayer[eye];
	}
	newEyeTextures.cpuTime = eyeTexturesCpuTime;
	newEyeTextures.gpuTime = eyeTexturesGpuTime;

	Signal_Wait( &timeWarp->newEyeTexturesConsumed, -1 );

	Mutex_Lock( &timeWarp->newEyeTexturesMutex, true );
	timeWarp->newEyeTextures = newEyeTextures;
	Mutex_Unlock( &timeWarp->newEyeTexturesMutex );
}

static void TimeWarp_ConsumeNewEyeTextures( GpuContext_t * context, TimeWarp_t * timeWarp )
{
	timeWarp->eyeTexturesFrames[timeWarp->timeWarpFrames % AVARGE_FRAME_RATE_FRAMES] = 0;

	// Never block the time warp thread.
	if ( !Mutex_Lock( &timeWarp->newEyeTexturesMutex, false ) )
	{
		return;
	}
	EyeTextures_t newEyeTextures = timeWarp->newEyeTextures;
	Mutex_Unlock( &timeWarp->newEyeTexturesMutex );

	if ( newEyeTextures.index > timeWarp->eyeTexturesConsumedIndex &&
			GpuFence_IsSignalled( context, newEyeTextures.completionFence[0] ) &&
				GpuFence_IsSignalled( context, newEyeTextures.completionFence[1] ) )
	{
		assert( newEyeTextures.index == timeWarp->eyeTexturesConsumedIndex + 1 );
		timeWarp->eyeTexturesConsumedIndex = newEyeTextures.index;
		timeWarp->projectionMatrix = newEyeTextures.projectionMatrix;
		timeWarp->viewMatrix = newEyeTextures.viewMatrix;
		for ( int eye = 0; eye < NUM_EYES; eye++ )
		{
			timeWarp->eyeTexture[eye] = newEyeTextures.texture[eye];
			timeWarp->eyeArrayLayer[eye] = newEyeTextures.arrayLayer[eye];
		}
		timeWarp->cpuTimes[PROFILE_TIME_EYE_TEXTURES] = newEyeTextures.cpuTime;
		timeWarp->gpuTimes[PROFILE_TIME_EYE_TEXTURES] = newEyeTextures.gpuTime;
		timeWarp->eyeTexturesFrames[timeWarp->timeWarpFrames % AVARGE_FRAME_RATE_FRAMES] = 1;
		Signal_Raise( &timeWarp->newEyeTexturesConsumed );
	}
}

static void TimeWarp_Render( TimeWarp_t * timeWarp, GpuWindow_t * window )
{
	TimeWarp_ConsumeNewEyeTextures( &window->context, timeWarp );

	// Calculate the eye texture and time warp frame rates.
	float timeWarpFrameRate = timeWarp->refreshRate;
	float eyeTexturesFrameRate = timeWarp->refreshRate;
	{
		Microseconds_t lastTime = timeWarp->frameCpuTime[timeWarp->timeWarpFrames % AVARGE_FRAME_RATE_FRAMES];
		Microseconds_t time = GpuWindow_GetNextSwapTime( window );
		timeWarp->frameCpuTime[timeWarp->timeWarpFrames % AVARGE_FRAME_RATE_FRAMES] = time;
		timeWarp->timeWarpFrames++;
		if ( timeWarp->timeWarpFrames > AVARGE_FRAME_RATE_FRAMES )
		{
			int timeWarpFrames = AVARGE_FRAME_RATE_FRAMES;
			int eyeTexturesFrames = 0;
			for ( int i = 0; i < AVARGE_FRAME_RATE_FRAMES; i++ )
			{
				eyeTexturesFrames += timeWarp->eyeTexturesFrames[i];
			}

			timeWarpFrameRate = timeWarpFrames * 1000.0f * 1000.0f / ( time - lastTime );
			eyeTexturesFrameRate = eyeTexturesFrames * 1000.0f * 1000.0f / ( time - lastTime );
		}
	}

	// Update the bar graphs if not paused.
	if ( timeWarp->bargraphs.barGraphState == BAR_GRAPH_VISIBLE )
	{
		const Vector4f_t * eyeTexturesFrameRateColor = ( eyeTexturesFrameRate > timeWarp->refreshRate - 0.5f ) ? &colorPurple : &colorRed;
		const Vector4f_t * timeWarpFrameRateColor = ( timeWarpFrameRate > timeWarp->refreshRate - 0.5f ) ? &colorGreen : &colorRed;

		BarGraph_AddBar( &timeWarp->bargraphs.eyeTexturesFrameRateGraph, 0, eyeTexturesFrameRate / timeWarp->refreshRate, eyeTexturesFrameRateColor, true );
		BarGraph_AddBar( &timeWarp->bargraphs.timeWarpFrameRateGraph, 0, timeWarpFrameRate / timeWarp->refreshRate, timeWarpFrameRateColor, true );

		for ( int i = 0; i < 2; i++ )
		{
			const float * times = ( i == 0 ) ? timeWarp->cpuTimes : timeWarp->gpuTimes;
			float barHeights[PROFILE_TIME_MAX];
			float totalBarHeight = 0.0f;
			for ( int i = 0; i < PROFILE_TIME_MAX; i++ )
			{
				barHeights[i] = times[i] * timeWarp->refreshRate * ( 1.0f / 1000.0f );
				totalBarHeight += barHeights[i];
			}

			const float limit = 0.9f;
			if ( totalBarHeight > limit )
			{
				totalBarHeight = 0.0f;
				for ( int i = 0; i < PROFILE_TIME_MAX; i++ )
				{
					barHeights[i] = ( totalBarHeight + barHeights[i] > limit ) ? ( limit - totalBarHeight ) : barHeights[i];
					totalBarHeight += barHeights[i];
				}
				barHeights[PROFILE_TIME_OVERFLOW] = 1.0f - limit;
			}

			BarGraph_t * barGraph = ( i == 0 ) ? &timeWarp->bargraphs.frameCpuTimeBarGraph : &timeWarp->bargraphs.frameGpuTimeBarGraph;
			for ( int i = 0; i < PROFILE_TIME_MAX; i++ )
			{
				BarGraph_AddBar( barGraph, i, barHeights[i], profileTimeBarColors[i], ( i == PROFILE_TIME_MAX - 1 ) );
			}
		}
	}

	FrameLog_BeginFrame();

	const Microseconds_t refreshStartTime = GpuWindow_GetNextSwapTime( window );
	const Microseconds_t refreshEndTime = refreshStartTime /* + refresh time for incremental display refresh */;

	if ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_GRAPHICS )
	{
		TimeWarpGraphics_Render( &timeWarp->commandBuffer, &timeWarp->graphics, &timeWarp->framebuffer, &timeWarp->renderPass,
								refreshStartTime, refreshEndTime,
								&timeWarp->projectionMatrix, &timeWarp->viewMatrix,
								timeWarp->eyeTexture, timeWarp->eyeArrayLayer, timeWarp->correctChromaticAberration,
								&timeWarp->bargraphs, timeWarp->cpuTimes, timeWarp->gpuTimes );
	}
	else if ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_COMPUTE )
	{
		TimeWarpCompute_Render( &timeWarp->commandBuffer, &timeWarp->compute, &timeWarp->framebuffer,
								refreshStartTime, refreshEndTime,
								&timeWarp->projectionMatrix, &timeWarp->viewMatrix,
								timeWarp->eyeTexture, timeWarp->eyeArrayLayer, timeWarp->correctChromaticAberration,
								&timeWarp->bargraphs, timeWarp->cpuTimes, timeWarp->gpuTimes );
	}

	const int gpuTimeFramesDelayed = ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_GRAPHICS ) ? GPU_TIMER_FRAMES_DELAYED : 0;

	FrameLog_EndFrame(	timeWarp->cpuTimes[PROFILE_TIME_TIME_WARP] +
						timeWarp->cpuTimes[PROFILE_TIME_BAR_GRAPHS] +
						timeWarp->cpuTimes[PROFILE_TIME_BLIT],
						timeWarp->gpuTimes[PROFILE_TIME_TIME_WARP] +
						timeWarp->gpuTimes[PROFILE_TIME_BAR_GRAPHS] +
						timeWarp->gpuTimes[PROFILE_TIME_BLIT], gpuTimeFramesDelayed );
}

/*
================================================================================================================================

Scene rendering.

Scene_t
SceneSettings_t

static void Scene_Create( GpuContext_t * context, Scene_t * scene, SceneSettings_t * settings, GpuRenderPass_t * renderPass );
static void Scene_Destroy( GpuContext_t * context, Scene_t * scene );
static void Scene_UpdateSettings( Scene_t * scene );
static void Scene_Simulate( Scene_t * scene, const Microseconds_t time );
static void Scene_UpdateMatrices( GpuCommandBuffer_t * commandBuffer, Scene_t * scene,
									const Matrix4x4f_t * viewMatrix, const Matrix4x4f_t * projectionMatrix );
static void Scene_Render( GpuCommandBuffer_t * commandBuffer, const Scene_t * scene );

static void SceneSettings_ToggleSimulationPaused( SceneSettings_t * settings );
static void SceneSettings_ToggleMultiView( SceneSettings_t * settings );
static void SceneSettings_SetSimulationPaused( SceneSettings_t * settings, const bool set );
static void SceneSettings_SetMultiView( SceneSettings_t * settings, const bool set );
static bool SceneSettings_GetSimulationPaused( SceneSettings_t * settings );
static bool SceneSettings_GetMultiView( SceneSettings_t * settings );
static void SceneSettings_CycleDrawCallLevel( SceneSettings_t * settings );
static void SceneSettings_CycleTriangleLevel( SceneSettings_t * settings );
static void SceneSettings_CycleFragmentLevel( SceneSettings_t * settings );
static void SceneSettings_SetDrawCallLevel( SceneSettings_t * settings, const int level );
static void SceneSettings_SetTriangleLevel( SceneSettings_t * settings, const int level );
static void SceneSettings_SetFragmentLevel( SceneSettings_t * settings, const int level );
static int SceneSettings_GetDrawCallLevel( const SceneSettings_t * settings );
static int SceneSettings_GetTriangleLevel( const SceneSettings_t * settings );
static int SceneSettings_GetFragmentLevel( const SceneSettings_t * settings );

================================================================================================================================
*/

#define MAX_SCENE_DRAWCALL_LEVELS	4
#define MAX_SCENE_TRIANGLE_LEVELS	4
#define MAX_SCENE_FRAGMENT_LEVELS	4

typedef struct
{
	bool	simulationPaused;
	bool	useMultiView;
	int		drawCallLevel;
	int		triangleLevel;
	int		fragmentLevel;
} SceneSettings_t;

static void SceneSettings_Init( SceneSettings_t * settings )
{
	settings->simulationPaused = false;
	settings->useMultiView = false;
	settings->drawCallLevel = 0;
	settings->triangleLevel = 0;
	settings->fragmentLevel = 0;
}

static void CycleLevel( int * x, const int max ) { (*x) = ( (*x) + 1 ) % max; }

static void SceneSettings_ToggleSimulationPaused( SceneSettings_t * settings ) { settings->simulationPaused = !settings->simulationPaused; }
static void SceneSettings_ToggleMultiView( SceneSettings_t * settings ) { settings->useMultiView = !settings->useMultiView; }

static void SceneSettings_SetSimulationPaused( SceneSettings_t * settings, const bool set ) { settings->simulationPaused = set; }
static void SceneSettings_SetMultiView( SceneSettings_t * settings, const bool set ) { settings->useMultiView = set; }

static bool SceneSettings_GetSimulationPaused( SceneSettings_t * settings ) { return settings->simulationPaused; }
static bool SceneSettings_GetMultiView( SceneSettings_t * settings ) { return settings->useMultiView; }

static void SceneSettings_CycleDrawCallLevel( SceneSettings_t * settings ) { CycleLevel( &settings->drawCallLevel, MAX_SCENE_DRAWCALL_LEVELS ); }
static void SceneSettings_CycleTriangleLevel( SceneSettings_t * settings ) { CycleLevel( &settings->triangleLevel, MAX_SCENE_TRIANGLE_LEVELS ); }
static void SceneSettings_CycleFragmentLevel( SceneSettings_t * settings ) { CycleLevel( &settings->fragmentLevel, MAX_SCENE_FRAGMENT_LEVELS ); }

static void SceneSettings_SetDrawCallLevel( SceneSettings_t * settings, const int level ) { settings->drawCallLevel = level; }
static void SceneSettings_SetTriangleLevel( SceneSettings_t * settings, const int level ) { settings->triangleLevel = level; }
static void SceneSettings_SetFragmentLevel( SceneSettings_t * settings, const int level ) { settings->fragmentLevel = level; }

static int SceneSettings_GetDrawCallLevel( const SceneSettings_t * settings ) { return settings->drawCallLevel; }
static int SceneSettings_GetTriangleLevel( const SceneSettings_t * settings ) { return settings->triangleLevel; }
static int SceneSettings_GetFragmentLevel( const SceneSettings_t * settings ) { return settings->fragmentLevel; }

typedef struct
{
	// assets
	GpuGeometry_t			geometry[MAX_SCENE_TRIANGLE_LEVELS];
	GpuGraphicsProgram_t	program[MAX_SCENE_FRAGMENT_LEVELS];
	GpuGraphicsPipeline_t	pipelines[MAX_SCENE_TRIANGLE_LEVELS][MAX_SCENE_FRAGMENT_LEVELS];
	GpuBuffer_t				sceneMatrices;
	GpuTexture_t			diffuseTexture;
	GpuTexture_t			specularTexture;
	GpuTexture_t			normalTexture;
	SceneSettings_t			settings;
	SceneSettings_t *		newSettings;
	// simulation state
	float					bigRotationX;
	float					bigRotationY;
	float					smallRotationX;
	float					smallRotationY;
	Matrix4x4f_t *			modelMatrix;
} Scene_t;

enum
{
	PROGRAM_UNIFORM_MODEL_MATRIX,
	PROGRAM_UNIFORM_SCENE_MATRICES,
	PROGRAM_TEXTURE_0,
	PROGRAM_TEXTURE_1,
	PROGRAM_TEXTURE_2
};

static GpuProgramParm_t flatShadedProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_VERTEX,	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_UNIFORM_MODEL_MATRIX,		"ModelMatrix",		0 },
	{ GPU_PROGRAM_STAGE_VERTEX,	GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_UNIFORM_SCENE_MATRICES,		"SceneMatrices",	0 }
};

static const char flatShadedVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( std140, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =   0 ) mat4 ModelMatrix;\n"
	"} pc;\n"
	"layout( std140, binding = 0 ) uniform SceneMatrices\n"
	"{\n"
	"	layout( offset =   0 ) mat4 ViewMatrix;\n"
	"	layout( offset =  64 ) mat4 ProjectionMatrix;\n"
	"} ub;\n"
	"layout( location = 0 ) in vec3 vertexPosition;\n"
	"layout( location = 1 ) in vec3 vertexNormal;\n"
	"layout( location = 0 ) out vec3 fragmentEyeDir;\n"
	"layout( location = 1 ) out vec3 fragmentNormal;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"vec3 multiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,\n"
	"		m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,\n"
	"		m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );\n"
	"}\n"
	"vec3 transposeMultiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[0].y * v.y + m[0].z * v.z,\n"
	"		m[1].x * v.x + m[1].y * v.y + m[1].z * v.z,\n"
	"		m[2].x * v.x + m[2].y * v.y + m[2].z * v.z );\n"
	"}\n"
	"void main( void )\n"
	"{\n"
	"	vec4 vertexWorldPos = pc.ModelMatrix * vec4( vertexPosition, 1.0 );\n"
	"	vec3 eyeWorldPos = transposeMultiply3x3( ub.ViewMatrix, -vec3( ub.ViewMatrix[3] ) );\n"
	"	gl_Position = ub.ProjectionMatrix * ( ub.ViewMatrix * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( pc.ModelMatrix, vertexNormal );\n"
	"}\n";

static const unsigned int flatShadedVertexProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x000000cb,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x00000093,0x000000b0,0x000000bb,
	0x000000c3,0x000000c4,0x00030003,0x00000001,0x00000136,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00080005,0x0000000f,0x746c756d,0x796c7069,0x28337833,0x3434666d,0x3366763b,
	0x0000003b,0x00030005,0x0000000d,0x0000006d,0x00030005,0x0000000e,0x00000076,0x000a0005,
	0x00000013,0x6e617274,0x736f7073,0x6c754d65,0x6c706974,0x33783379,0x34666d28,0x66763b34,
	0x00003b33,0x00030005,0x00000011,0x0000006d,0x00030005,0x00000012,0x00000076,0x00060005,
	0x0000008b,0x74726576,0x6f577865,0x50646c72,0x0000736f,0x00060005,0x0000008c,0x68737550,
	0x736e6f43,0x746e6174,0x00000073,0x00060006,0x0000008c,0x00000000,0x65646f4d,0x74614d6c,
	0x00786972,0x00030005,0x0000008e,0x00006370,0x00060005,0x00000093,0x74726576,0x6f507865,
	0x69746973,0x00006e6f,0x00050005,0x0000009b,0x57657965,0x646c726f,0x00736f50,0x00060005,
	0x0000009c,0x6e656353,0x74614d65,0x65636972,0x00000073,0x00060006,0x0000009c,0x00000000,
	0x77656956,0x7274614d,0x00007869,0x00080006,0x0000009c,0x00000001,0x6a6f7250,0x69746365,
	0x614d6e6f,0x78697274,0x00000000,0x00030005,0x0000009e,0x00006275,0x00040005,0x000000a8,
	0x61726170,0x0000006d,0x00040005,0x000000ac,0x61726170,0x0000006d,0x00060005,0x000000ae,
	0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x000000ae,0x00000000,0x505f6c67,
	0x7469736f,0x006e6f69,0x00070006,0x000000ae,0x00000001,0x505f6c67,0x746e696f,0x657a6953,
	0x00000000,0x00030005,0x000000b0,0x00000000,0x00060005,0x000000bb,0x67617266,0x746e656d,
	0x44657945,0x00007269,0x00060005,0x000000c3,0x67617266,0x746e656d,0x6d726f4e,0x00006c61,
	0x00060005,0x000000c4,0x74726576,0x6f4e7865,0x6c616d72,0x00000000,0x00040005,0x000000c5,
	0x61726170,0x0000006d,0x00040005,0x000000c8,0x61726170,0x0000006d,0x00040048,0x0000008c,
	0x00000000,0x00000005,0x00050048,0x0000008c,0x00000000,0x00000023,0x00000000,0x00050048,
	0x0000008c,0x00000000,0x00000007,0x00000010,0x00030047,0x0000008c,0x00000002,0x00040047,
	0x00000093,0x0000001e,0x00000000,0x00040048,0x0000009c,0x00000000,0x00000005,0x00050048,
	0x0000009c,0x00000000,0x00000023,0x00000000,0x00050048,0x0000009c,0x00000000,0x00000007,
	0x00000010,0x00040048,0x0000009c,0x00000001,0x00000005,0x00050048,0x0000009c,0x00000001,
	0x00000023,0x00000040,0x00050048,0x0000009c,0x00000001,0x00000007,0x00000010,0x00030047,
	0x0000009c,0x00000002,0x00040047,0x0000009e,0x00000022,0x00000000,0x00040047,0x0000009e,
	0x00000021,0x00000000,0x00050048,0x000000ae,0x00000000,0x0000000b,0x00000000,0x00050048,
	0x000000ae,0x00000001,0x0000000b,0x00000001,0x00030047,0x000000ae,0x00000002,0x00040047,
	0x000000bb,0x0000001e,0x00000000,0x00040047,0x000000c3,0x0000001e,0x00000001,0x00040047,
	0x000000c4,0x0000001e,0x00000001,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
	0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040018,
	0x00000008,0x00000007,0x00000004,0x00040020,0x00000009,0x00000007,0x00000008,0x00040017,
	0x0000000a,0x00000006,0x00000003,0x00040020,0x0000000b,0x00000007,0x0000000a,0x00050021,
	0x0000000c,0x0000000a,0x00000009,0x0000000b,0x00040015,0x00000015,0x00000020,0x00000001,
	0x0004002b,0x00000015,0x00000016,0x00000000,0x00040015,0x00000017,0x00000020,0x00000000,
	0x0004002b,0x00000017,0x00000018,0x00000000,0x00040020,0x00000019,0x00000007,0x00000006,
	0x0004002b,0x00000015,0x0000001f,0x00000001,0x0004002b,0x00000017,0x00000022,0x00000001,
	0x0004002b,0x00000015,0x00000027,0x00000002,0x0004002b,0x00000017,0x0000002a,0x00000002,
	0x00040020,0x0000008a,0x00000007,0x00000007,0x0003001e,0x0000008c,0x00000008,0x00040020,
	0x0000008d,0x00000009,0x0000008c,0x0004003b,0x0000008d,0x0000008e,0x00000009,0x00040020,
	0x0000008f,0x00000009,0x00000008,0x00040020,0x00000092,0x00000001,0x0000000a,0x0004003b,
	0x00000092,0x00000093,0x00000001,0x0004002b,0x00000006,0x00000095,0x3f800000,0x0004001e,
	0x0000009c,0x00000008,0x00000008,0x00040020,0x0000009d,0x00000002,0x0000009c,0x0004003b,
	0x0000009d,0x0000009e,0x00000002,0x0004002b,0x00000015,0x0000009f,0x00000003,0x00040020,
	0x000000a0,0x00000002,0x00000007,0x00040020,0x000000a9,0x00000002,0x00000008,0x0004001e,
	0x000000ae,0x00000007,0x00000006,0x00040020,0x000000af,0x00000003,0x000000ae,0x0004003b,
	0x000000af,0x000000b0,0x00000003,0x00040020,0x000000b8,0x00000003,0x00000007,0x00040020,
	0x000000ba,0x00000003,0x0000000a,0x0004003b,0x000000ba,0x000000bb,0x00000003,0x0004003b,
	0x000000ba,0x000000c3,0x00000003,0x0004003b,0x00000092,0x000000c4,0x00000001,0x00050036,
	0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x0000008a,
	0x0000008b,0x00000007,0x0004003b,0x0000000b,0x0000009b,0x00000007,0x0004003b,0x00000009,
	0x000000a8,0x00000007,0x0004003b,0x0000000b,0x000000ac,0x00000007,0x0004003b,0x00000009,
	0x000000c5,0x00000007,0x0004003b,0x0000000b,0x000000c8,0x00000007,0x00050041,0x0000008f,
	0x00000090,0x0000008e,0x00000016,0x0004003d,0x00000008,0x00000091,0x00000090,0x0004003d,
	0x0000000a,0x00000094,0x00000093,0x00050051,0x00000006,0x00000096,0x00000094,0x00000000,
	0x00050051,0x00000006,0x00000097,0x00000094,0x00000001,0x00050051,0x00000006,0x00000098,
	0x00000094,0x00000002,0x00070050,0x00000007,0x00000099,0x00000096,0x00000097,0x00000098,
	0x00000095,0x00050091,0x00000007,0x0000009a,0x00000091,0x00000099,0x0003003e,0x0000008b,
	0x0000009a,0x00060041,0x000000a0,0x000000a1,0x0000009e,0x00000016,0x0000009f,0x0004003d,
	0x00000007,0x000000a2,0x000000a1,0x00050051,0x00000006,0x000000a3,0x000000a2,0x00000000,
	0x00050051,0x00000006,0x000000a4,0x000000a2,0x00000001,0x00050051,0x00000006,0x000000a5,
	0x000000a2,0x00000002,0x00060050,0x0000000a,0x000000a6,0x000000a3,0x000000a4,0x000000a5,
	0x0004007f,0x0000000a,0x000000a7,0x000000a6,0x00050041,0x000000a9,0x000000aa,0x0000009e,
	0x00000016,0x0004003d,0x00000008,0x000000ab,0x000000aa,0x0003003e,0x000000a8,0x000000ab,
	0x0003003e,0x000000ac,0x000000a7,0x00060039,0x0000000a,0x000000ad,0x00000013,0x000000a8,
	0x000000ac,0x0003003e,0x0000009b,0x000000ad,0x00050041,0x000000a9,0x000000b1,0x0000009e,
	0x0000001f,0x0004003d,0x00000008,0x000000b2,0x000000b1,0x00050041,0x000000a9,0x000000b3,
	0x0000009e,0x00000016,0x0004003d,0x00000008,0x000000b4,0x000000b3,0x0004003d,0x00000007,
	0x000000b5,0x0000008b,0x00050091,0x00000007,0x000000b6,0x000000b4,0x000000b5,0x00050091,
	0x00000007,0x000000b7,0x000000b2,0x000000b6,0x00050041,0x000000b8,0x000000b9,0x000000b0,
	0x00000016,0x0003003e,0x000000b9,0x000000b7,0x0004003d,0x0000000a,0x000000bc,0x0000009b,
	0x0004003d,0x00000007,0x000000bd,0x0000008b,0x00050051,0x00000006,0x000000be,0x000000bd,
	0x00000000,0x00050051,0x00000006,0x000000bf,0x000000bd,0x00000001,0x00050051,0x00000006,
	0x000000c0,0x000000bd,0x00000002,0x00060050,0x0000000a,0x000000c1,0x000000be,0x000000bf,
	0x000000c0,0x00050083,0x0000000a,0x000000c2,0x000000bc,0x000000c1,0x0003003e,0x000000bb,
	0x000000c2,0x00050041,0x0000008f,0x000000c6,0x0000008e,0x00000016,0x0004003d,0x00000008,
	0x000000c7,0x000000c6,0x0003003e,0x000000c5,0x000000c7,0x0004003d,0x0000000a,0x000000c9,
	0x000000c4,0x0003003e,0x000000c8,0x000000c9,0x00060039,0x0000000a,0x000000ca,0x0000000f,
	0x000000c5,0x000000c8,0x0003003e,0x000000c3,0x000000ca,0x000100fd,0x00010038,0x00050036,
	0x0000000a,0x0000000f,0x00000000,0x0000000c,0x00030037,0x00000009,0x0000000d,0x00030037,
	0x0000000b,0x0000000e,0x000200f8,0x00000010,0x00060041,0x00000019,0x0000001a,0x0000000d,
	0x00000016,0x00000018,0x0004003d,0x00000006,0x0000001b,0x0000001a,0x00050041,0x00000019,
	0x0000001c,0x0000000e,0x00000018,0x0004003d,0x00000006,0x0000001d,0x0000001c,0x00050085,
	0x00000006,0x0000001e,0x0000001b,0x0000001d,0x00060041,0x00000019,0x00000020,0x0000000d,
	0x0000001f,0x00000018,0x0004003d,0x00000006,0x00000021,0x00000020,0x00050041,0x00000019,
	0x00000023,0x0000000e,0x00000022,0x0004003d,0x00000006,0x00000024,0x00000023,0x00050085,
	0x00000006,0x00000025,0x00000021,0x00000024,0x00050081,0x00000006,0x00000026,0x0000001e,
	0x00000025,0x00060041,0x00000019,0x00000028,0x0000000d,0x00000027,0x00000018,0x0004003d,
	0x00000006,0x00000029,0x00000028,0x00050041,0x00000019,0x0000002b,0x0000000e,0x0000002a,
	0x0004003d,0x00000006,0x0000002c,0x0000002b,0x00050085,0x00000006,0x0000002d,0x00000029,
	0x0000002c,0x00050081,0x00000006,0x0000002e,0x00000026,0x0000002d,0x00060041,0x00000019,
	0x0000002f,0x0000000d,0x00000016,0x00000022,0x0004003d,0x00000006,0x00000030,0x0000002f,
	0x00050041,0x00000019,0x00000031,0x0000000e,0x00000018,0x0004003d,0x00000006,0x00000032,
	0x00000031,0x00050085,0x00000006,0x00000033,0x00000030,0x00000032,0x00060041,0x00000019,
	0x00000034,0x0000000d,0x0000001f,0x00000022,0x0004003d,0x00000006,0x00000035,0x00000034,
	0x00050041,0x00000019,0x00000036,0x0000000e,0x00000022,0x0004003d,0x00000006,0x00000037,
	0x00000036,0x00050085,0x00000006,0x00000038,0x00000035,0x00000037,0x00050081,0x00000006,
	0x00000039,0x00000033,0x00000038,0x00060041,0x00000019,0x0000003a,0x0000000d,0x00000027,
	0x00000022,0x0004003d,0x00000006,0x0000003b,0x0000003a,0x00050041,0x00000019,0x0000003c,
	0x0000000e,0x0000002a,0x0004003d,0x00000006,0x0000003d,0x0000003c,0x00050085,0x00000006,
	0x0000003e,0x0000003b,0x0000003d,0x00050081,0x00000006,0x0000003f,0x00000039,0x0000003e,
	0x00060041,0x00000019,0x00000040,0x0000000d,0x00000016,0x0000002a,0x0004003d,0x00000006,
	0x00000041,0x00000040,0x00050041,0x00000019,0x00000042,0x0000000e,0x00000018,0x0004003d,
	0x00000006,0x00000043,0x00000042,0x00050085,0x00000006,0x00000044,0x00000041,0x00000043,
	0x00060041,0x00000019,0x00000045,0x0000000d,0x0000001f,0x0000002a,0x0004003d,0x00000006,
	0x00000046,0x00000045,0x00050041,0x00000019,0x00000047,0x0000000e,0x00000022,0x0004003d,
	0x00000006,0x00000048,0x00000047,0x00050085,0x00000006,0x00000049,0x00000046,0x00000048,
	0x00050081,0x00000006,0x0000004a,0x00000044,0x00000049,0x00060041,0x00000019,0x0000004b,
	0x0000000d,0x00000027,0x0000002a,0x0004003d,0x00000006,0x0000004c,0x0000004b,0x00050041,
	0x00000019,0x0000004d,0x0000000e,0x0000002a,0x0004003d,0x00000006,0x0000004e,0x0000004d,
	0x00050085,0x00000006,0x0000004f,0x0000004c,0x0000004e,0x00050081,0x00000006,0x00000050,
	0x0000004a,0x0000004f,0x00060050,0x0000000a,0x00000051,0x0000002e,0x0000003f,0x00000050,
	0x000200fe,0x00000051,0x00010038,0x00050036,0x0000000a,0x00000013,0x00000000,0x0000000c,
	0x00030037,0x00000009,0x00000011,0x00030037,0x0000000b,0x00000012,0x000200f8,0x00000014,
	0x00060041,0x00000019,0x00000054,0x00000011,0x00000016,0x00000018,0x0004003d,0x00000006,
	0x00000055,0x00000054,0x00050041,0x00000019,0x00000056,0x00000012,0x00000018,0x0004003d,
	0x00000006,0x00000057,0x00000056,0x00050085,0x00000006,0x00000058,0x00000055,0x00000057,
	0x00060041,0x00000019,0x00000059,0x00000011,0x00000016,0x00000022,0x0004003d,0x00000006,
	0x0000005a,0x00000059,0x00050041,0x00000019,0x0000005b,0x00000012,0x00000022,0x0004003d,
	0x00000006,0x0000005c,0x0000005b,0x00050085,0x00000006,0x0000005d,0x0000005a,0x0000005c,
	0x00050081,0x00000006,0x0000005e,0x00000058,0x0000005d,0x00060041,0x00000019,0x0000005f,
	0x00000011,0x00000016,0x0000002a,0x0004003d,0x00000006,0x00000060,0x0000005f,0x00050041,
	0x00000019,0x00000061,0x00000012,0x0000002a,0x0004003d,0x00000006,0x00000062,0x00000061,
	0x00050085,0x00000006,0x00000063,0x00000060,0x00000062,0x00050081,0x00000006,0x00000064,
	0x0000005e,0x00000063,0x00060041,0x00000019,0x00000065,0x00000011,0x0000001f,0x00000018,
	0x0004003d,0x00000006,0x00000066,0x00000065,0x00050041,0x00000019,0x00000067,0x00000012,
	0x00000018,0x0004003d,0x00000006,0x00000068,0x00000067,0x00050085,0x00000006,0x00000069,
	0x00000066,0x00000068,0x00060041,0x00000019,0x0000006a,0x00000011,0x0000001f,0x00000022,
	0x0004003d,0x00000006,0x0000006b,0x0000006a,0x00050041,0x00000019,0x0000006c,0x00000012,
	0x00000022,0x0004003d,0x00000006,0x0000006d,0x0000006c,0x00050085,0x00000006,0x0000006e,
	0x0000006b,0x0000006d,0x00050081,0x00000006,0x0000006f,0x00000069,0x0000006e,0x00060041,
	0x00000019,0x00000070,0x00000011,0x0000001f,0x0000002a,0x0004003d,0x00000006,0x00000071,
	0x00000070,0x00050041,0x00000019,0x00000072,0x00000012,0x0000002a,0x0004003d,0x00000006,
	0x00000073,0x00000072,0x00050085,0x00000006,0x00000074,0x00000071,0x00000073,0x00050081,
	0x00000006,0x00000075,0x0000006f,0x00000074,0x00060041,0x00000019,0x00000076,0x00000011,
	0x00000027,0x00000018,0x0004003d,0x00000006,0x00000077,0x00000076,0x00050041,0x00000019,
	0x00000078,0x00000012,0x00000018,0x0004003d,0x00000006,0x00000079,0x00000078,0x00050085,
	0x00000006,0x0000007a,0x00000077,0x00000079,0x00060041,0x00000019,0x0000007b,0x00000011,
	0x00000027,0x00000022,0x0004003d,0x00000006,0x0000007c,0x0000007b,0x00050041,0x00000019,
	0x0000007d,0x00000012,0x00000022,0x0004003d,0x00000006,0x0000007e,0x0000007d,0x00050085,
	0x00000006,0x0000007f,0x0000007c,0x0000007e,0x00050081,0x00000006,0x00000080,0x0000007a,
	0x0000007f,0x00060041,0x00000019,0x00000081,0x00000011,0x00000027,0x0000002a,0x0004003d,
	0x00000006,0x00000082,0x00000081,0x00050041,0x00000019,0x00000083,0x00000012,0x0000002a,
	0x0004003d,0x00000006,0x00000084,0x00000083,0x00050085,0x00000006,0x00000085,0x00000082,
	0x00000084,0x00050081,0x00000006,0x00000086,0x00000080,0x00000085,0x00060050,0x0000000a,
	0x00000087,0x00000064,0x00000075,0x00000086,0x000200fe,0x00000087,0x00010038
};

static const char flatShadedFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( location = 0 ) in lowp vec3 fragmentEyeDir;\n"
	"layout( location = 1 ) in lowp vec3 fragmentNormal;\n"
	"layout( location = 0 ) out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	lowp vec3 diffuseMap = vec3( 0.2, 0.2, 1.0 );\n"
	"	lowp vec3 specularMap = vec3( 0.5, 0.5, 0.5 );\n"
	"	lowp float specularPower = 10.0;\n"
	"	lowp vec3 eyeDir = normalize( fragmentEyeDir );\n"
	"	lowp vec3 normal = normalize( fragmentNormal );\n"
	"\n"
	"	lowp vec3 lightDir = normalize( vec3( -1.0, 1.0, 1.0 ) );\n"
	"	lowp vec3 lightReflection = normalize( 2.0 * dot( lightDir, normal ) * normal - lightDir );\n"
	"	lowp vec3 lightDiffuse = diffuseMap * ( max( dot( normal, lightDir ), 0.0 ) * 0.5 + 0.5 );\n"
	"	lowp vec3 lightSpecular = specularMap * pow( max( dot( lightReflection, eyeDir ), 0.0 ), specularPower );\n"
	"\n"
	"	outColor.xyz = lightDiffuse + lightSpecular;\n"
	"	outColor.w = 1.0;\n"
	"}\n";

static const unsigned int flatShadedFragmentProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x0000004a,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0008000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000015,0x00000019,0x00000040,
	0x00030010,0x00000004,0x00000007,0x00030003,0x00000001,0x00000136,0x00040005,0x00000004,
	0x6e69616d,0x00000000,0x00050005,0x00000009,0x66666964,0x4d657375,0x00007061,0x00050005,
	0x0000000d,0x63657073,0x72616c75,0x0070614d,0x00060005,0x00000011,0x63657073,0x72616c75,
	0x65776f50,0x00000072,0x00040005,0x00000013,0x44657965,0x00007269,0x00060005,0x00000015,
	0x67617266,0x746e656d,0x44657945,0x00007269,0x00040005,0x00000018,0x6d726f6e,0x00006c61,
	0x00060005,0x00000019,0x67617266,0x746e656d,0x6d726f4e,0x00006c61,0x00050005,0x0000001c,
	0x6867696c,0x72694474,0x00000000,0x00060005,0x00000020,0x6867696c,0x66655274,0x7463656c,
	0x006e6f69,0x00060005,0x0000002b,0x6867696c,0x66694474,0x65737566,0x00000000,0x00060005,
	0x00000035,0x6867696c,0x65705374,0x616c7563,0x00000072,0x00050005,0x00000040,0x4374756f,
	0x726f6c6f,0x00000000,0x00030047,0x00000009,0x00000000,0x00030047,0x0000000d,0x00000000,
	0x00030047,0x00000011,0x00000000,0x00030047,0x00000013,0x00000000,0x00030047,0x00000015,
	0x00000000,0x00040047,0x00000015,0x0000001e,0x00000000,0x00030047,0x00000016,0x00000000,
	0x00030047,0x00000017,0x00000000,0x00030047,0x00000018,0x00000000,0x00030047,0x00000019,
	0x00000000,0x00040047,0x00000019,0x0000001e,0x00000001,0x00030047,0x0000001a,0x00000000,
	0x00030047,0x0000001b,0x00000000,0x00030047,0x0000001c,0x00000000,0x00030047,0x00000020,
	0x00000000,0x00030047,0x00000022,0x00000000,0x00030047,0x00000023,0x00000000,0x00030047,
	0x00000024,0x00000000,0x00030047,0x00000025,0x00000000,0x00030047,0x00000026,0x00000000,
	0x00030047,0x00000027,0x00000000,0x00030047,0x00000028,0x00000000,0x00030047,0x00000029,
	0x00000000,0x00030047,0x0000002a,0x00000000,0x00030047,0x0000002b,0x00000000,0x00030047,
	0x0000002c,0x00000000,0x00030047,0x0000002d,0x00000000,0x00030047,0x0000002e,0x00000000,
	0x00030047,0x0000002f,0x00000000,0x00030047,0x00000031,0x00000000,0x00030047,0x00000032,
	0x00000000,0x00030047,0x00000033,0x00000000,0x00030047,0x00000034,0x00000000,0x00030047,
	0x00000035,0x00000000,0x00030047,0x00000036,0x00000000,0x00030047,0x00000037,0x00000000,
	0x00030047,0x00000038,0x00000000,0x00030047,0x00000039,0x00000000,0x00030047,0x0000003a,
	0x00000000,0x00030047,0x0000003b,0x00000000,0x00030047,0x0000003c,0x00000000,0x00030047,
	0x0000003d,0x00000000,0x00030047,0x00000040,0x00000000,0x00040047,0x00000040,0x0000001e,
	0x00000000,0x00030047,0x00000041,0x00000000,0x00030047,0x00000042,0x00000000,0x00030047,
	0x00000043,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,
	0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000003,0x00040020,0x00000008,
	0x00000007,0x00000007,0x0004002b,0x00000006,0x0000000a,0x3e4ccccd,0x0004002b,0x00000006,
	0x0000000b,0x3f800000,0x0006002c,0x00000007,0x0000000c,0x0000000a,0x0000000a,0x0000000b,
	0x0004002b,0x00000006,0x0000000e,0x3f000000,0x0006002c,0x00000007,0x0000000f,0x0000000e,
	0x0000000e,0x0000000e,0x00040020,0x00000010,0x00000007,0x00000006,0x0004002b,0x00000006,
	0x00000012,0x41200000,0x00040020,0x00000014,0x00000001,0x00000007,0x0004003b,0x00000014,
	0x00000015,0x00000001,0x0004003b,0x00000014,0x00000019,0x00000001,0x0004002b,0x00000006,
	0x0000001d,0xbf13cd3a,0x0004002b,0x00000006,0x0000001e,0x3f13cd3a,0x0006002c,0x00000007,
	0x0000001f,0x0000001d,0x0000001e,0x0000001e,0x0004002b,0x00000006,0x00000021,0x40000000,
	0x0004002b,0x00000006,0x00000030,0x00000000,0x00040017,0x0000003e,0x00000006,0x00000004,
	0x00040020,0x0000003f,0x00000003,0x0000003e,0x0004003b,0x0000003f,0x00000040,0x00000003,
	0x00040015,0x00000046,0x00000020,0x00000000,0x0004002b,0x00000046,0x00000047,0x00000003,
	0x00040020,0x00000048,0x00000003,0x00000006,0x00050036,0x00000002,0x00000004,0x00000000,
	0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000008,0x00000009,0x00000007,0x0004003b,
	0x00000008,0x0000000d,0x00000007,0x0004003b,0x00000010,0x00000011,0x00000007,0x0004003b,
	0x00000008,0x00000013,0x00000007,0x0004003b,0x00000008,0x00000018,0x00000007,0x0004003b,
	0x00000008,0x0000001c,0x00000007,0x0004003b,0x00000008,0x00000020,0x00000007,0x0004003b,
	0x00000008,0x0000002b,0x00000007,0x0004003b,0x00000008,0x00000035,0x00000007,0x0003003e,
	0x00000009,0x0000000c,0x0003003e,0x0000000d,0x0000000f,0x0003003e,0x00000011,0x00000012,
	0x0004003d,0x00000007,0x00000016,0x00000015,0x0006000c,0x00000007,0x00000017,0x00000001,
	0x00000045,0x00000016,0x0003003e,0x00000013,0x00000017,0x0004003d,0x00000007,0x0000001a,
	0x00000019,0x0006000c,0x00000007,0x0000001b,0x00000001,0x00000045,0x0000001a,0x0003003e,
	0x00000018,0x0000001b,0x0003003e,0x0000001c,0x0000001f,0x0004003d,0x00000007,0x00000022,
	0x0000001c,0x0004003d,0x00000007,0x00000023,0x00000018,0x00050094,0x00000006,0x00000024,
	0x00000022,0x00000023,0x00050085,0x00000006,0x00000025,0x00000021,0x00000024,0x0004003d,
	0x00000007,0x00000026,0x00000018,0x0005008e,0x00000007,0x00000027,0x00000026,0x00000025,
	0x0004003d,0x00000007,0x00000028,0x0000001c,0x00050083,0x00000007,0x00000029,0x00000027,
	0x00000028,0x0006000c,0x00000007,0x0000002a,0x00000001,0x00000045,0x00000029,0x0003003e,
	0x00000020,0x0000002a,0x0004003d,0x00000007,0x0000002c,0x00000009,0x0004003d,0x00000007,
	0x0000002d,0x00000018,0x0004003d,0x00000007,0x0000002e,0x0000001c,0x00050094,0x00000006,
	0x0000002f,0x0000002d,0x0000002e,0x0007000c,0x00000006,0x00000031,0x00000001,0x00000028,
	0x0000002f,0x00000030,0x00050085,0x00000006,0x00000032,0x00000031,0x0000000e,0x00050081,
	0x00000006,0x00000033,0x00000032,0x0000000e,0x0005008e,0x00000007,0x00000034,0x0000002c,
	0x00000033,0x0003003e,0x0000002b,0x00000034,0x0004003d,0x00000007,0x00000036,0x0000000d,
	0x0004003d,0x00000007,0x00000037,0x00000020,0x0004003d,0x00000007,0x00000038,0x00000013,
	0x00050094,0x00000006,0x00000039,0x00000037,0x00000038,0x0007000c,0x00000006,0x0000003a,
	0x00000001,0x00000028,0x00000039,0x00000030,0x0004003d,0x00000006,0x0000003b,0x00000011,
	0x0007000c,0x00000006,0x0000003c,0x00000001,0x0000001a,0x0000003a,0x0000003b,0x0005008e,
	0x00000007,0x0000003d,0x00000036,0x0000003c,0x0003003e,0x00000035,0x0000003d,0x0004003d,
	0x00000007,0x00000041,0x0000002b,0x0004003d,0x00000007,0x00000042,0x00000035,0x00050081,
	0x00000007,0x00000043,0x00000041,0x00000042,0x0004003d,0x0000003e,0x00000044,0x00000040,
	0x0009004f,0x0000003e,0x00000045,0x00000044,0x00000043,0x00000004,0x00000005,0x00000006,
	0x00000003,0x0003003e,0x00000040,0x00000045,0x00050041,0x00000048,0x00000049,0x00000040,
	0x00000047,0x0003003e,0x00000049,0x0000000b,0x000100fd,0x00010038
};

static GpuProgramParm_t normalMappedProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_VERTEX,		GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_UNIFORM_MODEL_MATRIX,		"ModelMatrix",		0 },
	{ GPU_PROGRAM_STAGE_VERTEX,		GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_UNIFORM_SCENE_MATRICES,		"SceneMatrices",	0 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_TEXTURE_0,					"Texture0",			1 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_TEXTURE_1,					"Texture1",			2 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_TEXTURE_2,					"Texture2",			3 }
};

static const char normalMappedVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( std140, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =   0 ) mat4 ModelMatrix;\n"
	"} pc;\n"
	"layout( std140, binding = 0 ) uniform SceneMatrices\n"
	"{\n"
	"	layout( offset =   0 ) mat4 ViewMatrix;\n"
	"	layout( offset =  64 ) mat4 ProjectionMatrix;\n"
	"} ub;\n"
	"layout( location = 0 ) in vec3 vertexPosition;\n"
	"layout( location = 1 ) in vec3 vertexNormal;\n"
	"layout( location = 2 ) in vec3 vertexTangent;\n"
	"layout( location = 3 ) in vec3 vertexBinormal;\n"
	"layout( location = 4 ) in vec2 vertexUv0;\n"
	"layout( location = 0 ) out vec3 fragmentEyeDir;\n"
	"layout( location = 1 ) out vec3 fragmentNormal;\n"
	"layout( location = 2 ) out vec3 fragmentTangent;\n"
	"layout( location = 3 ) out vec3 fragmentBinormal;\n"
	"layout( location = 4 ) out vec2 fragmentUv0;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"vec3 multiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,\n"
	"		m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,\n"
	"		m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );\n"
	"}\n"
	"vec3 transposeMultiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[0].y * v.y + m[0].z * v.z,\n"
	"		m[1].x * v.x + m[1].y * v.y + m[1].z * v.z,\n"
	"		m[2].x * v.x + m[2].y * v.y + m[2].z * v.z );\n"
	"}\n"
	"void main( void )\n"
	"{\n"
	"	vec4 vertexWorldPos = pc.ModelMatrix * vec4( vertexPosition, 1.0 );\n"
	"	vec3 eyeWorldPos = transposeMultiply3x3( ub.ViewMatrix, -vec3( ub.ViewMatrix[3] ) );\n"
	"	gl_Position = ub.ProjectionMatrix * ( ub.ViewMatrix * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( pc.ModelMatrix, vertexNormal );\n"
	"	fragmentTangent = multiply3x3( pc.ModelMatrix, vertexTangent );\n"
	"	fragmentBinormal = multiply3x3( pc.ModelMatrix, vertexBinormal );\n"
	"	fragmentUv0 = vertexUv0;\n"
	"}\n";

static const unsigned int normalMappedVertexProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x000000e1,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0010000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x00000093,0x000000b0,0x000000bb,
	0x000000c3,0x000000c4,0x000000cb,0x000000cc,0x000000d3,0x000000d4,0x000000dd,0x000000df,
	0x00030003,0x00000001,0x00000136,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00080005,
	0x0000000f,0x746c756d,0x796c7069,0x28337833,0x3434666d,0x3366763b,0x0000003b,0x00030005,
	0x0000000d,0x0000006d,0x00030005,0x0000000e,0x00000076,0x000a0005,0x00000013,0x6e617274,
	0x736f7073,0x6c754d65,0x6c706974,0x33783379,0x34666d28,0x66763b34,0x00003b33,0x00030005,
	0x00000011,0x0000006d,0x00030005,0x00000012,0x00000076,0x00060005,0x0000008b,0x74726576,
	0x6f577865,0x50646c72,0x0000736f,0x00060005,0x0000008c,0x68737550,0x736e6f43,0x746e6174,
	0x00000073,0x00060006,0x0000008c,0x00000000,0x65646f4d,0x74614d6c,0x00786972,0x00030005,
	0x0000008e,0x00006370,0x00060005,0x00000093,0x74726576,0x6f507865,0x69746973,0x00006e6f,
	0x00050005,0x0000009b,0x57657965,0x646c726f,0x00736f50,0x00060005,0x0000009c,0x6e656353,
	0x74614d65,0x65636972,0x00000073,0x00060006,0x0000009c,0x00000000,0x77656956,0x7274614d,
	0x00007869,0x00080006,0x0000009c,0x00000001,0x6a6f7250,0x69746365,0x614d6e6f,0x78697274,
	0x00000000,0x00030005,0x0000009e,0x00006275,0x00040005,0x000000a8,0x61726170,0x0000006d,
	0x00040005,0x000000ac,0x61726170,0x0000006d,0x00060005,0x000000ae,0x505f6c67,0x65567265,
	0x78657472,0x00000000,0x00060006,0x000000ae,0x00000000,0x505f6c67,0x7469736f,0x006e6f69,
	0x00070006,0x000000ae,0x00000001,0x505f6c67,0x746e696f,0x657a6953,0x00000000,0x00030005,
	0x000000b0,0x00000000,0x00060005,0x000000bb,0x67617266,0x746e656d,0x44657945,0x00007269,
	0x00060005,0x000000c3,0x67617266,0x746e656d,0x6d726f4e,0x00006c61,0x00060005,0x000000c4,
	0x74726576,0x6f4e7865,0x6c616d72,0x00000000,0x00040005,0x000000c5,0x61726170,0x0000006d,
	0x00040005,0x000000c8,0x61726170,0x0000006d,0x00060005,0x000000cb,0x67617266,0x746e656d,
	0x676e6154,0x00746e65,0x00060005,0x000000cc,0x74726576,0x61547865,0x6e65676e,0x00000074,
	0x00040005,0x000000cd,0x61726170,0x0000006d,0x00040005,0x000000d0,0x61726170,0x0000006d,
	0x00070005,0x000000d3,0x67617266,0x746e656d,0x6f6e6942,0x6c616d72,0x00000000,0x00060005,
	0x000000d4,0x74726576,0x69427865,0x6d726f6e,0x00006c61,0x00040005,0x000000d5,0x61726170,
	0x0000006d,0x00040005,0x000000d8,0x61726170,0x0000006d,0x00050005,0x000000dd,0x67617266,
	0x746e656d,0x00307655,0x00050005,0x000000df,0x74726576,0x76557865,0x00000030,0x00040048,
	0x0000008c,0x00000000,0x00000005,0x00050048,0x0000008c,0x00000000,0x00000023,0x00000000,
	0x00050048,0x0000008c,0x00000000,0x00000007,0x00000010,0x00030047,0x0000008c,0x00000002,
	0x00040047,0x00000093,0x0000001e,0x00000000,0x00040048,0x0000009c,0x00000000,0x00000005,
	0x00050048,0x0000009c,0x00000000,0x00000023,0x00000000,0x00050048,0x0000009c,0x00000000,
	0x00000007,0x00000010,0x00040048,0x0000009c,0x00000001,0x00000005,0x00050048,0x0000009c,
	0x00000001,0x00000023,0x00000040,0x00050048,0x0000009c,0x00000001,0x00000007,0x00000010,
	0x00030047,0x0000009c,0x00000002,0x00040047,0x0000009e,0x00000022,0x00000000,0x00040047,
	0x0000009e,0x00000021,0x00000000,0x00050048,0x000000ae,0x00000000,0x0000000b,0x00000000,
	0x00050048,0x000000ae,0x00000001,0x0000000b,0x00000001,0x00030047,0x000000ae,0x00000002,
	0x00040047,0x000000bb,0x0000001e,0x00000000,0x00040047,0x000000c3,0x0000001e,0x00000001,
	0x00040047,0x000000c4,0x0000001e,0x00000001,0x00040047,0x000000cb,0x0000001e,0x00000002,
	0x00040047,0x000000cc,0x0000001e,0x00000002,0x00040047,0x000000d3,0x0000001e,0x00000003,
	0x00040047,0x000000d4,0x0000001e,0x00000003,0x00040047,0x000000dd,0x0000001e,0x00000004,
	0x00040047,0x000000df,0x0000001e,0x00000004,0x00020013,0x00000002,0x00030021,0x00000003,
	0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,
	0x00040018,0x00000008,0x00000007,0x00000004,0x00040020,0x00000009,0x00000007,0x00000008,
	0x00040017,0x0000000a,0x00000006,0x00000003,0x00040020,0x0000000b,0x00000007,0x0000000a,
	0x00050021,0x0000000c,0x0000000a,0x00000009,0x0000000b,0x00040015,0x00000015,0x00000020,
	0x00000001,0x0004002b,0x00000015,0x00000016,0x00000000,0x00040015,0x00000017,0x00000020,
	0x00000000,0x0004002b,0x00000017,0x00000018,0x00000000,0x00040020,0x00000019,0x00000007,
	0x00000006,0x0004002b,0x00000015,0x0000001f,0x00000001,0x0004002b,0x00000017,0x00000022,
	0x00000001,0x0004002b,0x00000015,0x00000027,0x00000002,0x0004002b,0x00000017,0x0000002a,
	0x00000002,0x00040020,0x0000008a,0x00000007,0x00000007,0x0003001e,0x0000008c,0x00000008,
	0x00040020,0x0000008d,0x00000009,0x0000008c,0x0004003b,0x0000008d,0x0000008e,0x00000009,
	0x00040020,0x0000008f,0x00000009,0x00000008,0x00040020,0x00000092,0x00000001,0x0000000a,
	0x0004003b,0x00000092,0x00000093,0x00000001,0x0004002b,0x00000006,0x00000095,0x3f800000,
	0x0004001e,0x0000009c,0x00000008,0x00000008,0x00040020,0x0000009d,0x00000002,0x0000009c,
	0x0004003b,0x0000009d,0x0000009e,0x00000002,0x0004002b,0x00000015,0x0000009f,0x00000003,
	0x00040020,0x000000a0,0x00000002,0x00000007,0x00040020,0x000000a9,0x00000002,0x00000008,
	0x0004001e,0x000000ae,0x00000007,0x00000006,0x00040020,0x000000af,0x00000003,0x000000ae,
	0x0004003b,0x000000af,0x000000b0,0x00000003,0x00040020,0x000000b8,0x00000003,0x00000007,
	0x00040020,0x000000ba,0x00000003,0x0000000a,0x0004003b,0x000000ba,0x000000bb,0x00000003,
	0x0004003b,0x000000ba,0x000000c3,0x00000003,0x0004003b,0x00000092,0x000000c4,0x00000001,
	0x0004003b,0x000000ba,0x000000cb,0x00000003,0x0004003b,0x00000092,0x000000cc,0x00000001,
	0x0004003b,0x000000ba,0x000000d3,0x00000003,0x0004003b,0x00000092,0x000000d4,0x00000001,
	0x00040017,0x000000db,0x00000006,0x00000002,0x00040020,0x000000dc,0x00000003,0x000000db,
	0x0004003b,0x000000dc,0x000000dd,0x00000003,0x00040020,0x000000de,0x00000001,0x000000db,
	0x0004003b,0x000000de,0x000000df,0x00000001,0x00050036,0x00000002,0x00000004,0x00000000,
	0x00000003,0x000200f8,0x00000005,0x0004003b,0x0000008a,0x0000008b,0x00000007,0x0004003b,
	0x0000000b,0x0000009b,0x00000007,0x0004003b,0x00000009,0x000000a8,0x00000007,0x0004003b,
	0x0000000b,0x000000ac,0x00000007,0x0004003b,0x00000009,0x000000c5,0x00000007,0x0004003b,
	0x0000000b,0x000000c8,0x00000007,0x0004003b,0x00000009,0x000000cd,0x00000007,0x0004003b,
	0x0000000b,0x000000d0,0x00000007,0x0004003b,0x00000009,0x000000d5,0x00000007,0x0004003b,
	0x0000000b,0x000000d8,0x00000007,0x00050041,0x0000008f,0x00000090,0x0000008e,0x00000016,
	0x0004003d,0x00000008,0x00000091,0x00000090,0x0004003d,0x0000000a,0x00000094,0x00000093,
	0x00050051,0x00000006,0x00000096,0x00000094,0x00000000,0x00050051,0x00000006,0x00000097,
	0x00000094,0x00000001,0x00050051,0x00000006,0x00000098,0x00000094,0x00000002,0x00070050,
	0x00000007,0x00000099,0x00000096,0x00000097,0x00000098,0x00000095,0x00050091,0x00000007,
	0x0000009a,0x00000091,0x00000099,0x0003003e,0x0000008b,0x0000009a,0x00060041,0x000000a0,
	0x000000a1,0x0000009e,0x00000016,0x0000009f,0x0004003d,0x00000007,0x000000a2,0x000000a1,
	0x00050051,0x00000006,0x000000a3,0x000000a2,0x00000000,0x00050051,0x00000006,0x000000a4,
	0x000000a2,0x00000001,0x00050051,0x00000006,0x000000a5,0x000000a2,0x00000002,0x00060050,
	0x0000000a,0x000000a6,0x000000a3,0x000000a4,0x000000a5,0x0004007f,0x0000000a,0x000000a7,
	0x000000a6,0x00050041,0x000000a9,0x000000aa,0x0000009e,0x00000016,0x0004003d,0x00000008,
	0x000000ab,0x000000aa,0x0003003e,0x000000a8,0x000000ab,0x0003003e,0x000000ac,0x000000a7,
	0x00060039,0x0000000a,0x000000ad,0x00000013,0x000000a8,0x000000ac,0x0003003e,0x0000009b,
	0x000000ad,0x00050041,0x000000a9,0x000000b1,0x0000009e,0x0000001f,0x0004003d,0x00000008,
	0x000000b2,0x000000b1,0x00050041,0x000000a9,0x000000b3,0x0000009e,0x00000016,0x0004003d,
	0x00000008,0x000000b4,0x000000b3,0x0004003d,0x00000007,0x000000b5,0x0000008b,0x00050091,
	0x00000007,0x000000b6,0x000000b4,0x000000b5,0x00050091,0x00000007,0x000000b7,0x000000b2,
	0x000000b6,0x00050041,0x000000b8,0x000000b9,0x000000b0,0x00000016,0x0003003e,0x000000b9,
	0x000000b7,0x0004003d,0x0000000a,0x000000bc,0x0000009b,0x0004003d,0x00000007,0x000000bd,
	0x0000008b,0x00050051,0x00000006,0x000000be,0x000000bd,0x00000000,0x00050051,0x00000006,
	0x000000bf,0x000000bd,0x00000001,0x00050051,0x00000006,0x000000c0,0x000000bd,0x00000002,
	0x00060050,0x0000000a,0x000000c1,0x000000be,0x000000bf,0x000000c0,0x00050083,0x0000000a,
	0x000000c2,0x000000bc,0x000000c1,0x0003003e,0x000000bb,0x000000c2,0x00050041,0x0000008f,
	0x000000c6,0x0000008e,0x00000016,0x0004003d,0x00000008,0x000000c7,0x000000c6,0x0003003e,
	0x000000c5,0x000000c7,0x0004003d,0x0000000a,0x000000c9,0x000000c4,0x0003003e,0x000000c8,
	0x000000c9,0x00060039,0x0000000a,0x000000ca,0x0000000f,0x000000c5,0x000000c8,0x0003003e,
	0x000000c3,0x000000ca,0x00050041,0x0000008f,0x000000ce,0x0000008e,0x00000016,0x0004003d,
	0x00000008,0x000000cf,0x000000ce,0x0003003e,0x000000cd,0x000000cf,0x0004003d,0x0000000a,
	0x000000d1,0x000000cc,0x0003003e,0x000000d0,0x000000d1,0x00060039,0x0000000a,0x000000d2,
	0x0000000f,0x000000cd,0x000000d0,0x0003003e,0x000000cb,0x000000d2,0x00050041,0x0000008f,
	0x000000d6,0x0000008e,0x00000016,0x0004003d,0x00000008,0x000000d7,0x000000d6,0x0003003e,
	0x000000d5,0x000000d7,0x0004003d,0x0000000a,0x000000d9,0x000000d4,0x0003003e,0x000000d8,
	0x000000d9,0x00060039,0x0000000a,0x000000da,0x0000000f,0x000000d5,0x000000d8,0x0003003e,
	0x000000d3,0x000000da,0x0004003d,0x000000db,0x000000e0,0x000000df,0x0003003e,0x000000dd,
	0x000000e0,0x000100fd,0x00010038,0x00050036,0x0000000a,0x0000000f,0x00000000,0x0000000c,
	0x00030037,0x00000009,0x0000000d,0x00030037,0x0000000b,0x0000000e,0x000200f8,0x00000010,
	0x00060041,0x00000019,0x0000001a,0x0000000d,0x00000016,0x00000018,0x0004003d,0x00000006,
	0x0000001b,0x0000001a,0x00050041,0x00000019,0x0000001c,0x0000000e,0x00000018,0x0004003d,
	0x00000006,0x0000001d,0x0000001c,0x00050085,0x00000006,0x0000001e,0x0000001b,0x0000001d,
	0x00060041,0x00000019,0x00000020,0x0000000d,0x0000001f,0x00000018,0x0004003d,0x00000006,
	0x00000021,0x00000020,0x00050041,0x00000019,0x00000023,0x0000000e,0x00000022,0x0004003d,
	0x00000006,0x00000024,0x00000023,0x00050085,0x00000006,0x00000025,0x00000021,0x00000024,
	0x00050081,0x00000006,0x00000026,0x0000001e,0x00000025,0x00060041,0x00000019,0x00000028,
	0x0000000d,0x00000027,0x00000018,0x0004003d,0x00000006,0x00000029,0x00000028,0x00050041,
	0x00000019,0x0000002b,0x0000000e,0x0000002a,0x0004003d,0x00000006,0x0000002c,0x0000002b,
	0x00050085,0x00000006,0x0000002d,0x00000029,0x0000002c,0x00050081,0x00000006,0x0000002e,
	0x00000026,0x0000002d,0x00060041,0x00000019,0x0000002f,0x0000000d,0x00000016,0x00000022,
	0x0004003d,0x00000006,0x00000030,0x0000002f,0x00050041,0x00000019,0x00000031,0x0000000e,
	0x00000018,0x0004003d,0x00000006,0x00000032,0x00000031,0x00050085,0x00000006,0x00000033,
	0x00000030,0x00000032,0x00060041,0x00000019,0x00000034,0x0000000d,0x0000001f,0x00000022,
	0x0004003d,0x00000006,0x00000035,0x00000034,0x00050041,0x00000019,0x00000036,0x0000000e,
	0x00000022,0x0004003d,0x00000006,0x00000037,0x00000036,0x00050085,0x00000006,0x00000038,
	0x00000035,0x00000037,0x00050081,0x00000006,0x00000039,0x00000033,0x00000038,0x00060041,
	0x00000019,0x0000003a,0x0000000d,0x00000027,0x00000022,0x0004003d,0x00000006,0x0000003b,
	0x0000003a,0x00050041,0x00000019,0x0000003c,0x0000000e,0x0000002a,0x0004003d,0x00000006,
	0x0000003d,0x0000003c,0x00050085,0x00000006,0x0000003e,0x0000003b,0x0000003d,0x00050081,
	0x00000006,0x0000003f,0x00000039,0x0000003e,0x00060041,0x00000019,0x00000040,0x0000000d,
	0x00000016,0x0000002a,0x0004003d,0x00000006,0x00000041,0x00000040,0x00050041,0x00000019,
	0x00000042,0x0000000e,0x00000018,0x0004003d,0x00000006,0x00000043,0x00000042,0x00050085,
	0x00000006,0x00000044,0x00000041,0x00000043,0x00060041,0x00000019,0x00000045,0x0000000d,
	0x0000001f,0x0000002a,0x0004003d,0x00000006,0x00000046,0x00000045,0x00050041,0x00000019,
	0x00000047,0x0000000e,0x00000022,0x0004003d,0x00000006,0x00000048,0x00000047,0x00050085,
	0x00000006,0x00000049,0x00000046,0x00000048,0x00050081,0x00000006,0x0000004a,0x00000044,
	0x00000049,0x00060041,0x00000019,0x0000004b,0x0000000d,0x00000027,0x0000002a,0x0004003d,
	0x00000006,0x0000004c,0x0000004b,0x00050041,0x00000019,0x0000004d,0x0000000e,0x0000002a,
	0x0004003d,0x00000006,0x0000004e,0x0000004d,0x00050085,0x00000006,0x0000004f,0x0000004c,
	0x0000004e,0x00050081,0x00000006,0x00000050,0x0000004a,0x0000004f,0x00060050,0x0000000a,
	0x00000051,0x0000002e,0x0000003f,0x00000050,0x000200fe,0x00000051,0x00010038,0x00050036,
	0x0000000a,0x00000013,0x00000000,0x0000000c,0x00030037,0x00000009,0x00000011,0x00030037,
	0x0000000b,0x00000012,0x000200f8,0x00000014,0x00060041,0x00000019,0x00000054,0x00000011,
	0x00000016,0x00000018,0x0004003d,0x00000006,0x00000055,0x00000054,0x00050041,0x00000019,
	0x00000056,0x00000012,0x00000018,0x0004003d,0x00000006,0x00000057,0x00000056,0x00050085,
	0x00000006,0x00000058,0x00000055,0x00000057,0x00060041,0x00000019,0x00000059,0x00000011,
	0x00000016,0x00000022,0x0004003d,0x00000006,0x0000005a,0x00000059,0x00050041,0x00000019,
	0x0000005b,0x00000012,0x00000022,0x0004003d,0x00000006,0x0000005c,0x0000005b,0x00050085,
	0x00000006,0x0000005d,0x0000005a,0x0000005c,0x00050081,0x00000006,0x0000005e,0x00000058,
	0x0000005d,0x00060041,0x00000019,0x0000005f,0x00000011,0x00000016,0x0000002a,0x0004003d,
	0x00000006,0x00000060,0x0000005f,0x00050041,0x00000019,0x00000061,0x00000012,0x0000002a,
	0x0004003d,0x00000006,0x00000062,0x00000061,0x00050085,0x00000006,0x00000063,0x00000060,
	0x00000062,0x00050081,0x00000006,0x00000064,0x0000005e,0x00000063,0x00060041,0x00000019,
	0x00000065,0x00000011,0x0000001f,0x00000018,0x0004003d,0x00000006,0x00000066,0x00000065,
	0x00050041,0x00000019,0x00000067,0x00000012,0x00000018,0x0004003d,0x00000006,0x00000068,
	0x00000067,0x00050085,0x00000006,0x00000069,0x00000066,0x00000068,0x00060041,0x00000019,
	0x0000006a,0x00000011,0x0000001f,0x00000022,0x0004003d,0x00000006,0x0000006b,0x0000006a,
	0x00050041,0x00000019,0x0000006c,0x00000012,0x00000022,0x0004003d,0x00000006,0x0000006d,
	0x0000006c,0x00050085,0x00000006,0x0000006e,0x0000006b,0x0000006d,0x00050081,0x00000006,
	0x0000006f,0x00000069,0x0000006e,0x00060041,0x00000019,0x00000070,0x00000011,0x0000001f,
	0x0000002a,0x0004003d,0x00000006,0x00000071,0x00000070,0x00050041,0x00000019,0x00000072,
	0x00000012,0x0000002a,0x0004003d,0x00000006,0x00000073,0x00000072,0x00050085,0x00000006,
	0x00000074,0x00000071,0x00000073,0x00050081,0x00000006,0x00000075,0x0000006f,0x00000074,
	0x00060041,0x00000019,0x00000076,0x00000011,0x00000027,0x00000018,0x0004003d,0x00000006,
	0x00000077,0x00000076,0x00050041,0x00000019,0x00000078,0x00000012,0x00000018,0x0004003d,
	0x00000006,0x00000079,0x00000078,0x00050085,0x00000006,0x0000007a,0x00000077,0x00000079,
	0x00060041,0x00000019,0x0000007b,0x00000011,0x00000027,0x00000022,0x0004003d,0x00000006,
	0x0000007c,0x0000007b,0x00050041,0x00000019,0x0000007d,0x00000012,0x00000022,0x0004003d,
	0x00000006,0x0000007e,0x0000007d,0x00050085,0x00000006,0x0000007f,0x0000007c,0x0000007e,
	0x00050081,0x00000006,0x00000080,0x0000007a,0x0000007f,0x00060041,0x00000019,0x00000081,
	0x00000011,0x00000027,0x0000002a,0x0004003d,0x00000006,0x00000082,0x00000081,0x00050041,
	0x00000019,0x00000083,0x00000012,0x0000002a,0x0004003d,0x00000006,0x00000084,0x00000083,
	0x00050085,0x00000006,0x00000085,0x00000082,0x00000084,0x00050081,0x00000006,0x00000086,
	0x00000080,0x00000085,0x00060050,0x0000000a,0x00000087,0x00000064,0x00000075,0x00000086,
	0x000200fe,0x00000087,0x00010038
};

static const char normalMapped100LightsFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( binding = 1 ) uniform sampler2D Texture0;\n"
	"layout( binding = 2 ) uniform sampler2D Texture1;\n"
	"layout( binding = 3 ) uniform sampler2D Texture2;\n"
	"layout( location = 0 ) in lowp vec3 fragmentEyeDir;\n"
	"layout( location = 1 ) in lowp vec3 fragmentNormal;\n"
	"layout( location = 2 ) in lowp vec3 fragmentTangent;\n"
	"layout( location = 3 ) in lowp vec3 fragmentBinormal;\n"
	"layout( location = 4 ) in lowp vec2 fragmentUv0;\n"
	"layout( location = 0 ) out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	lowp vec3 diffuseMap = texture( Texture0, fragmentUv0 ).xyz;\n"
	"	lowp vec3 specularMap = texture( Texture1, fragmentUv0 ).xyz * 2.0;\n"
	"	lowp vec3 normalMap = texture( Texture2, fragmentUv0 ).xyz * 2.0 - 1.0;\n"
	"	lowp float specularPower = 10.0;\n"
	"	lowp vec3 eyeDir = normalize( fragmentEyeDir );\n"
	"	lowp vec3 normal = normalize( normalMap.x * fragmentTangent + normalMap.y * fragmentBinormal + normalMap.z * fragmentNormal );\n"
	"\n"
	"	lowp vec3 color = vec3( 0 );\n"
	"	for ( int i = 0; i < 100; i++ )\n"
	"	{\n"
	"		lowp vec3 lightDir = normalize( vec3( -1.0, 1.0, 1.0 ) );\n"
	"		lowp vec3 lightReflection = normalize( 2.0 * dot( lightDir, normal ) * normal - lightDir );\n"
	"		lowp vec3 lightDiffuse = diffuseMap * ( max( dot( normal, lightDir ), 0.0 ) * 0.5 + 0.5 );\n"
	"		lowp vec3 lightSpecular = specularMap * pow( max( dot( lightReflection, eyeDir ), 0.0 ), specularPower );\n"
	"		color += ( lightDiffuse + lightSpecular ) * ( 1.0 / 100.0 );\n"
	"	}\n"
	"\n"
	"	outColor.xyz = color;\n"
	"	outColor.w = 1.0;\n"
	"}\n";

static const unsigned int normalMapped100LightsFragmentProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x0000008a,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000b000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000011,0x0000002d,0x00000035,
	0x0000003b,0x00000042,0x00000083,0x00030010,0x00000004,0x00000007,0x00030003,0x00000001,
	0x00000136,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00050005,0x00000009,0x66666964,
	0x4d657375,0x00007061,0x00050005,0x0000000d,0x74786554,0x30657275,0x00000000,0x00050005,
	0x00000011,0x67617266,0x746e656d,0x00307655,0x00050005,0x00000016,0x63657073,0x72616c75,
	0x0070614d,0x00050005,0x00000017,0x74786554,0x31657275,0x00000000,0x00050005,0x0000001e,
	0x6d726f6e,0x614d6c61,0x00000070,0x00050005,0x0000001f,0x74786554,0x32657275,0x00000000,
	0x00060005,0x00000029,0x63657073,0x72616c75,0x65776f50,0x00000072,0x00040005,0x0000002b,
	0x44657965,0x00007269,0x00060005,0x0000002d,0x67617266,0x746e656d,0x44657945,0x00007269,
	0x00040005,0x00000030,0x6d726f6e,0x00006c61,0x00060005,0x00000035,0x67617266,0x746e656d,
	0x676e6154,0x00746e65,0x00070005,0x0000003b,0x67617266,0x746e656d,0x6f6e6942,0x6c616d72,
	0x00000000,0x00060005,0x00000042,0x67617266,0x746e656d,0x6d726f4e,0x00006c61,0x00040005,
	0x00000047,0x6f6c6f63,0x00000072,0x00030005,0x0000004c,0x00000069,0x00050005,0x00000057,
	0x6867696c,0x72694474,0x00000000,0x00060005,0x0000005b,0x6867696c,0x66655274,0x7463656c,
	0x006e6f69,0x00060005,0x00000065,0x6867696c,0x66694474,0x65737566,0x00000000,0x00060005,
	0x0000006f,0x6867696c,0x65705374,0x616c7563,0x00000072,0x00050005,0x00000083,0x4374756f,
	0x726f6c6f,0x00000000,0x00030047,0x00000009,0x00000000,0x00030047,0x0000000d,0x00000000,
	0x00040047,0x0000000d,0x00000022,0x00000000,0x00040047,0x0000000d,0x00000021,0x00000001,
	0x00030047,0x0000000e,0x00000000,0x00030047,0x00000011,0x00000000,0x00040047,0x00000011,
	0x0000001e,0x00000004,0x00030047,0x00000012,0x00000000,0x00030047,0x00000014,0x00000000,
	0x00030047,0x00000015,0x00000000,0x00030047,0x00000016,0x00000000,0x00030047,0x00000017,
	0x00000000,0x00040047,0x00000017,0x00000022,0x00000000,0x00040047,0x00000017,0x00000021,
	0x00000002,0x00030047,0x00000018,0x00000000,0x00030047,0x00000019,0x00000000,0x00030047,
	0x0000001a,0x00000000,0x00030047,0x0000001b,0x00000000,0x00030047,0x0000001d,0x00000000,
	0x00030047,0x0000001e,0x00000000,0x00030047,0x0000001f,0x00000000,0x00040047,0x0000001f,
	0x00000022,0x00000000,0x00040047,0x0000001f,0x00000021,0x00000003,0x00030047,0x00000020,
	0x00000000,0x00030047,0x00000021,0x00000000,0x00030047,0x00000022,0x00000000,0x00030047,
	0x00000023,0x00000000,0x00030047,0x00000024,0x00000000,0x00030047,0x00000026,0x00000000,
	0x00030047,0x00000027,0x00000000,0x00030047,0x00000029,0x00000000,0x00030047,0x0000002b,
	0x00000000,0x00030047,0x0000002d,0x00000000,0x00040047,0x0000002d,0x0000001e,0x00000000,
	0x00030047,0x0000002e,0x00000000,0x00030047,0x0000002f,0x00000000,0x00030047,0x00000030,
	0x00000000,0x00030047,0x00000034,0x00000000,0x00030047,0x00000035,0x00000000,0x00040047,
	0x00000035,0x0000001e,0x00000002,0x00030047,0x00000036,0x00000000,0x00030047,0x00000037,
	0x00000000,0x00030047,0x0000003a,0x00000000,0x00030047,0x0000003b,0x00000000,0x00040047,
	0x0000003b,0x0000001e,0x00000003,0x00030047,0x0000003c,0x00000000,0x00030047,0x0000003d,
	0x00000000,0x00030047,0x0000003e,0x00000000,0x00030047,0x00000041,0x00000000,0x00030047,
	0x00000042,0x00000000,0x00040047,0x00000042,0x0000001e,0x00000001,0x00030047,0x00000043,
	0x00000000,0x00030047,0x00000044,0x00000000,0x00030047,0x00000045,0x00000000,0x00030047,
	0x00000046,0x00000000,0x00030047,0x00000047,0x00000000,0x00030047,0x0000004c,0x00000000,
	0x00030047,0x00000053,0x00000000,0x00030047,0x00000057,0x00000000,0x00030047,0x0000005b,
	0x00000000,0x00030047,0x0000005c,0x00000000,0x00030047,0x0000005d,0x00000000,0x00030047,
	0x0000005e,0x00000000,0x00030047,0x0000005f,0x00000000,0x00030047,0x00000060,0x00000000,
	0x00030047,0x00000061,0x00000000,0x00030047,0x00000062,0x00000000,0x00030047,0x00000063,
	0x00000000,0x00030047,0x00000064,0x00000000,0x00030047,0x00000065,0x00000000,0x00030047,
	0x00000066,0x00000000,0x00030047,0x00000067,0x00000000,0x00030047,0x00000068,0x00000000,
	0x00030047,0x00000069,0x00000000,0x00030047,0x0000006a,0x00000000,0x00030047,0x0000006c,
	0x00000000,0x00030047,0x0000006d,0x00000000,0x00030047,0x0000006e,0x00000000,0x00030047,
	0x0000006f,0x00000000,0x00030047,0x00000070,0x00000000,0x00030047,0x00000071,0x00000000,
	0x00030047,0x00000072,0x00000000,0x00030047,0x00000073,0x00000000,0x00030047,0x00000074,
	0x00000000,0x00030047,0x00000075,0x00000000,0x00030047,0x00000076,0x00000000,0x00030047,
	0x00000077,0x00000000,0x00030047,0x00000078,0x00000000,0x00030047,0x00000079,0x00000000,
	0x00030047,0x0000007a,0x00000000,0x00030047,0x0000007c,0x00000000,0x00030047,0x0000007d,
	0x00000000,0x00030047,0x0000007e,0x00000000,0x00030047,0x0000007f,0x00000000,0x00030047,
	0x00000081,0x00000000,0x00030047,0x00000083,0x00000000,0x00040047,0x00000083,0x0000001e,
	0x00000000,0x00030047,0x00000084,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,
	0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000003,
	0x00040020,0x00000008,0x00000007,0x00000007,0x00090019,0x0000000a,0x00000006,0x00000001,
	0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x0003001b,0x0000000b,0x0000000a,
	0x00040020,0x0000000c,0x00000000,0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000000,
	0x00040017,0x0000000f,0x00000006,0x00000002,0x00040020,0x00000010,0x00000001,0x0000000f,
	0x0004003b,0x00000010,0x00000011,0x00000001,0x00040017,0x00000013,0x00000006,0x00000004,
	0x0004003b,0x0000000c,0x00000017,0x00000000,0x0004002b,0x00000006,0x0000001c,0x40000000,
	0x0004003b,0x0000000c,0x0000001f,0x00000000,0x0004002b,0x00000006,0x00000025,0x3f800000,
	0x00040020,0x00000028,0x00000007,0x00000006,0x0004002b,0x00000006,0x0000002a,0x41200000,
	0x00040020,0x0000002c,0x00000001,0x00000007,0x0004003b,0x0000002c,0x0000002d,0x00000001,
	0x00040015,0x00000031,0x00000020,0x00000000,0x0004002b,0x00000031,0x00000032,0x00000000,
	0x0004003b,0x0000002c,0x00000035,0x00000001,0x0004002b,0x00000031,0x00000038,0x00000001,
	0x0004003b,0x0000002c,0x0000003b,0x00000001,0x0004002b,0x00000031,0x0000003f,0x00000002,
	0x0004003b,0x0000002c,0x00000042,0x00000001,0x0004002b,0x00000006,0x00000048,0x00000000,
	0x0006002c,0x00000007,0x00000049,0x00000048,0x00000048,0x00000048,0x00040015,0x0000004a,
	0x00000020,0x00000001,0x00040020,0x0000004b,0x00000007,0x0000004a,0x0004002b,0x0000004a,
	0x0000004d,0x00000000,0x0004002b,0x0000004a,0x00000054,0x00000064,0x00020014,0x00000055,
	0x0004002b,0x00000006,0x00000058,0xbf13cd3a,0x0004002b,0x00000006,0x00000059,0x3f13cd3a,
	0x0006002c,0x00000007,0x0000005a,0x00000058,0x00000059,0x00000059,0x0004002b,0x00000006,
	0x0000006b,0x3f000000,0x0004002b,0x00000006,0x0000007b,0x3c23d70a,0x0004002b,0x0000004a,
	0x00000080,0x00000001,0x00040020,0x00000082,0x00000003,0x00000013,0x0004003b,0x00000082,
	0x00000083,0x00000003,0x0004002b,0x00000031,0x00000087,0x00000003,0x00040020,0x00000088,
	0x00000003,0x00000006,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
	0x00000005,0x0004003b,0x00000008,0x00000009,0x00000007,0x0004003b,0x00000008,0x00000016,
	0x00000007,0x0004003b,0x00000008,0x0000001e,0x00000007,0x0004003b,0x00000028,0x00000029,
	0x00000007,0x0004003b,0x00000008,0x0000002b,0x00000007,0x0004003b,0x00000008,0x00000030,
	0x00000007,0x0004003b,0x00000008,0x00000047,0x00000007,0x0004003b,0x0000004b,0x0000004c,
	0x00000007,0x0004003b,0x00000008,0x00000057,0x00000007,0x0004003b,0x00000008,0x0000005b,
	0x00000007,0x0004003b,0x00000008,0x00000065,0x00000007,0x0004003b,0x00000008,0x0000006f,
	0x00000007,0x0004003d,0x0000000b,0x0000000e,0x0000000d,0x0004003d,0x0000000f,0x00000012,
	0x00000011,0x00050057,0x00000013,0x00000014,0x0000000e,0x00000012,0x0008004f,0x00000007,
	0x00000015,0x00000014,0x00000014,0x00000000,0x00000001,0x00000002,0x0003003e,0x00000009,
	0x00000015,0x0004003d,0x0000000b,0x00000018,0x00000017,0x0004003d,0x0000000f,0x00000019,
	0x00000011,0x00050057,0x00000013,0x0000001a,0x00000018,0x00000019,0x0008004f,0x00000007,
	0x0000001b,0x0000001a,0x0000001a,0x00000000,0x00000001,0x00000002,0x0005008e,0x00000007,
	0x0000001d,0x0000001b,0x0000001c,0x0003003e,0x00000016,0x0000001d,0x0004003d,0x0000000b,
	0x00000020,0x0000001f,0x0004003d,0x0000000f,0x00000021,0x00000011,0x00050057,0x00000013,
	0x00000022,0x00000020,0x00000021,0x0008004f,0x00000007,0x00000023,0x00000022,0x00000022,
	0x00000000,0x00000001,0x00000002,0x0005008e,0x00000007,0x00000024,0x00000023,0x0000001c,
	0x00060050,0x00000007,0x00000026,0x00000025,0x00000025,0x00000025,0x00050083,0x00000007,
	0x00000027,0x00000024,0x00000026,0x0003003e,0x0000001e,0x00000027,0x0003003e,0x00000029,
	0x0000002a,0x0004003d,0x00000007,0x0000002e,0x0000002d,0x0006000c,0x00000007,0x0000002f,
	0x00000001,0x00000045,0x0000002e,0x0003003e,0x0000002b,0x0000002f,0x00050041,0x00000028,
	0x00000033,0x0000001e,0x00000032,0x0004003d,0x00000006,0x00000034,0x00000033,0x0004003d,
	0x00000007,0x00000036,0x00000035,0x0005008e,0x00000007,0x00000037,0x00000036,0x00000034,
	0x00050041,0x00000028,0x00000039,0x0000001e,0x00000038,0x0004003d,0x00000006,0x0000003a,
	0x00000039,0x0004003d,0x00000007,0x0000003c,0x0000003b,0x0005008e,0x00000007,0x0000003d,
	0x0000003c,0x0000003a,0x00050081,0x00000007,0x0000003e,0x00000037,0x0000003d,0x00050041,
	0x00000028,0x00000040,0x0000001e,0x0000003f,0x0004003d,0x00000006,0x00000041,0x00000040,
	0x0004003d,0x00000007,0x00000043,0x00000042,0x0005008e,0x00000007,0x00000044,0x00000043,
	0x00000041,0x00050081,0x00000007,0x00000045,0x0000003e,0x00000044,0x0006000c,0x00000007,
	0x00000046,0x00000001,0x00000045,0x00000045,0x0003003e,0x00000030,0x00000046,0x0003003e,
	0x00000047,0x00000049,0x0003003e,0x0000004c,0x0000004d,0x000200f9,0x0000004e,0x000200f8,
	0x0000004e,0x000400f6,0x00000050,0x00000051,0x00000000,0x000200f9,0x00000052,0x000200f8,
	0x00000052,0x0004003d,0x0000004a,0x00000053,0x0000004c,0x000500b1,0x00000055,0x00000056,
	0x00000053,0x00000054,0x000400fa,0x00000056,0x0000004f,0x00000050,0x000200f8,0x0000004f,
	0x0003003e,0x00000057,0x0000005a,0x0004003d,0x00000007,0x0000005c,0x00000057,0x0004003d,
	0x00000007,0x0000005d,0x00000030,0x00050094,0x00000006,0x0000005e,0x0000005c,0x0000005d,
	0x00050085,0x00000006,0x0000005f,0x0000001c,0x0000005e,0x0004003d,0x00000007,0x00000060,
	0x00000030,0x0005008e,0x00000007,0x00000061,0x00000060,0x0000005f,0x0004003d,0x00000007,
	0x00000062,0x00000057,0x00050083,0x00000007,0x00000063,0x00000061,0x00000062,0x0006000c,
	0x00000007,0x00000064,0x00000001,0x00000045,0x00000063,0x0003003e,0x0000005b,0x00000064,
	0x0004003d,0x00000007,0x00000066,0x00000009,0x0004003d,0x00000007,0x00000067,0x00000030,
	0x0004003d,0x00000007,0x00000068,0x00000057,0x00050094,0x00000006,0x00000069,0x00000067,
	0x00000068,0x0007000c,0x00000006,0x0000006a,0x00000001,0x00000028,0x00000069,0x00000048,
	0x00050085,0x00000006,0x0000006c,0x0000006a,0x0000006b,0x00050081,0x00000006,0x0000006d,
	0x0000006c,0x0000006b,0x0005008e,0x00000007,0x0000006e,0x00000066,0x0000006d,0x0003003e,
	0x00000065,0x0000006e,0x0004003d,0x00000007,0x00000070,0x00000016,0x0004003d,0x00000007,
	0x00000071,0x0000005b,0x0004003d,0x00000007,0x00000072,0x0000002b,0x00050094,0x00000006,
	0x00000073,0x00000071,0x00000072,0x0007000c,0x00000006,0x00000074,0x00000001,0x00000028,
	0x00000073,0x00000048,0x0004003d,0x00000006,0x00000075,0x00000029,0x0007000c,0x00000006,
	0x00000076,0x00000001,0x0000001a,0x00000074,0x00000075,0x0005008e,0x00000007,0x00000077,
	0x00000070,0x00000076,0x0003003e,0x0000006f,0x00000077,0x0004003d,0x00000007,0x00000078,
	0x00000065,0x0004003d,0x00000007,0x00000079,0x0000006f,0x00050081,0x00000007,0x0000007a,
	0x00000078,0x00000079,0x0005008e,0x00000007,0x0000007c,0x0000007a,0x0000007b,0x0004003d,
	0x00000007,0x0000007d,0x00000047,0x00050081,0x00000007,0x0000007e,0x0000007d,0x0000007c,
	0x0003003e,0x00000047,0x0000007e,0x000200f9,0x00000051,0x000200f8,0x00000051,0x0004003d,
	0x0000004a,0x0000007f,0x0000004c,0x00050080,0x0000004a,0x00000081,0x0000007f,0x00000080,
	0x0003003e,0x0000004c,0x00000081,0x000200f9,0x0000004e,0x000200f8,0x00000050,0x0004003d,
	0x00000007,0x00000084,0x00000047,0x0004003d,0x00000013,0x00000085,0x00000083,0x0009004f,
	0x00000013,0x00000086,0x00000085,0x00000084,0x00000004,0x00000005,0x00000006,0x00000003,
	0x0003003e,0x00000083,0x00000086,0x00050041,0x00000088,0x00000089,0x00000083,0x00000087,
	0x0003003e,0x00000089,0x00000025,0x000100fd,0x00010038
};

static const char normalMapped1000LightsFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( binding = 1 ) uniform sampler2D Texture0;\n"
	"layout( binding = 2 ) uniform sampler2D Texture1;\n"
	"layout( binding = 3 ) uniform sampler2D Texture2;\n"
	"layout( location = 0 ) in lowp vec3 fragmentEyeDir;\n"
	"layout( location = 1 ) in lowp vec3 fragmentNormal;\n"
	"layout( location = 2 ) in lowp vec3 fragmentTangent;\n"
	"layout( location = 3 ) in lowp vec3 fragmentBinormal;\n"
	"layout( location = 4 ) in lowp vec2 fragmentUv0;\n"
	"layout( location = 0 ) out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	lowp vec3 diffuseMap = texture( Texture0, fragmentUv0 ).xyz;\n"
	"	lowp vec3 specularMap = texture( Texture1, fragmentUv0 ).xyz * 2.0;\n"
	"	lowp vec3 normalMap = texture( Texture2, fragmentUv0 ).xyz * 2.0 - 1.0;\n"
	"	lowp float specularPower = 10.0;\n"
	"	lowp vec3 eyeDir = normalize( fragmentEyeDir );\n"
	"	lowp vec3 normal = normalize( normalMap.x * fragmentTangent + normalMap.y * fragmentBinormal + normalMap.z * fragmentNormal );\n"
	"\n"
	"	lowp vec3 color = vec3( 0 );\n"
	"	for ( int i = 0; i < 1000; i++ )\n"
	"	{\n"
	"		lowp vec3 lightDir = normalize( vec3( -1.0, 1.0, 1.0 ) );\n"
	"		lowp vec3 lightReflection = normalize( 2.0 * dot( lightDir, normal ) * normal - lightDir );\n"
	"		lowp vec3 lightDiffuse = diffuseMap * ( max( dot( normal, lightDir ), 0.0 ) * 0.5 + 0.5 );\n"
	"		lowp vec3 lightSpecular = specularMap * pow( max( dot( lightReflection, eyeDir ), 0.0 ), specularPower );\n"
	"		color += ( lightDiffuse + lightSpecular ) * ( 1.0 / 1000.0 );\n"
	"	}\n"
	"\n"
	"	outColor.xyz = color;\n"
	"	outColor.w = 1.0;\n"
	"}\n";

static const unsigned int normalMapped1000LightsFragmentProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x0000008a,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000b000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000011,0x0000002d,0x00000035,
	0x0000003b,0x00000042,0x00000083,0x00030010,0x00000004,0x00000007,0x00030003,0x00000001,
	0x00000136,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00050005,0x00000009,0x66666964,
	0x4d657375,0x00007061,0x00050005,0x0000000d,0x74786554,0x30657275,0x00000000,0x00050005,
	0x00000011,0x67617266,0x746e656d,0x00307655,0x00050005,0x00000016,0x63657073,0x72616c75,
	0x0070614d,0x00050005,0x00000017,0x74786554,0x31657275,0x00000000,0x00050005,0x0000001e,
	0x6d726f6e,0x614d6c61,0x00000070,0x00050005,0x0000001f,0x74786554,0x32657275,0x00000000,
	0x00060005,0x00000029,0x63657073,0x72616c75,0x65776f50,0x00000072,0x00040005,0x0000002b,
	0x44657965,0x00007269,0x00060005,0x0000002d,0x67617266,0x746e656d,0x44657945,0x00007269,
	0x00040005,0x00000030,0x6d726f6e,0x00006c61,0x00060005,0x00000035,0x67617266,0x746e656d,
	0x676e6154,0x00746e65,0x00070005,0x0000003b,0x67617266,0x746e656d,0x6f6e6942,0x6c616d72,
	0x00000000,0x00060005,0x00000042,0x67617266,0x746e656d,0x6d726f4e,0x00006c61,0x00040005,
	0x00000047,0x6f6c6f63,0x00000072,0x00030005,0x0000004c,0x00000069,0x00050005,0x00000057,
	0x6867696c,0x72694474,0x00000000,0x00060005,0x0000005b,0x6867696c,0x66655274,0x7463656c,
	0x006e6f69,0x00060005,0x00000065,0x6867696c,0x66694474,0x65737566,0x00000000,0x00060005,
	0x0000006f,0x6867696c,0x65705374,0x616c7563,0x00000072,0x00050005,0x00000083,0x4374756f,
	0x726f6c6f,0x00000000,0x00030047,0x00000009,0x00000000,0x00030047,0x0000000d,0x00000000,
	0x00040047,0x0000000d,0x00000022,0x00000000,0x00040047,0x0000000d,0x00000021,0x00000001,
	0x00030047,0x0000000e,0x00000000,0x00030047,0x00000011,0x00000000,0x00040047,0x00000011,
	0x0000001e,0x00000004,0x00030047,0x00000012,0x00000000,0x00030047,0x00000014,0x00000000,
	0x00030047,0x00000015,0x00000000,0x00030047,0x00000016,0x00000000,0x00030047,0x00000017,
	0x00000000,0x00040047,0x00000017,0x00000022,0x00000000,0x00040047,0x00000017,0x00000021,
	0x00000002,0x00030047,0x00000018,0x00000000,0x00030047,0x00000019,0x00000000,0x00030047,
	0x0000001a,0x00000000,0x00030047,0x0000001b,0x00000000,0x00030047,0x0000001d,0x00000000,
	0x00030047,0x0000001e,0x00000000,0x00030047,0x0000001f,0x00000000,0x00040047,0x0000001f,
	0x00000022,0x00000000,0x00040047,0x0000001f,0x00000021,0x00000003,0x00030047,0x00000020,
	0x00000000,0x00030047,0x00000021,0x00000000,0x00030047,0x00000022,0x00000000,0x00030047,
	0x00000023,0x00000000,0x00030047,0x00000024,0x00000000,0x00030047,0x00000026,0x00000000,
	0x00030047,0x00000027,0x00000000,0x00030047,0x00000029,0x00000000,0x00030047,0x0000002b,
	0x00000000,0x00030047,0x0000002d,0x00000000,0x00040047,0x0000002d,0x0000001e,0x00000000,
	0x00030047,0x0000002e,0x00000000,0x00030047,0x0000002f,0x00000000,0x00030047,0x00000030,
	0x00000000,0x00030047,0x00000034,0x00000000,0x00030047,0x00000035,0x00000000,0x00040047,
	0x00000035,0x0000001e,0x00000002,0x00030047,0x00000036,0x00000000,0x00030047,0x00000037,
	0x00000000,0x00030047,0x0000003a,0x00000000,0x00030047,0x0000003b,0x00000000,0x00040047,
	0x0000003b,0x0000001e,0x00000003,0x00030047,0x0000003c,0x00000000,0x00030047,0x0000003d,
	0x00000000,0x00030047,0x0000003e,0x00000000,0x00030047,0x00000041,0x00000000,0x00030047,
	0x00000042,0x00000000,0x00040047,0x00000042,0x0000001e,0x00000001,0x00030047,0x00000043,
	0x00000000,0x00030047,0x00000044,0x00000000,0x00030047,0x00000045,0x00000000,0x00030047,
	0x00000046,0x00000000,0x00030047,0x00000047,0x00000000,0x00030047,0x0000004c,0x00000000,
	0x00030047,0x00000053,0x00000000,0x00030047,0x00000057,0x00000000,0x00030047,0x0000005b,
	0x00000000,0x00030047,0x0000005c,0x00000000,0x00030047,0x0000005d,0x00000000,0x00030047,
	0x0000005e,0x00000000,0x00030047,0x0000005f,0x00000000,0x00030047,0x00000060,0x00000000,
	0x00030047,0x00000061,0x00000000,0x00030047,0x00000062,0x00000000,0x00030047,0x00000063,
	0x00000000,0x00030047,0x00000064,0x00000000,0x00030047,0x00000065,0x00000000,0x00030047,
	0x00000066,0x00000000,0x00030047,0x00000067,0x00000000,0x00030047,0x00000068,0x00000000,
	0x00030047,0x00000069,0x00000000,0x00030047,0x0000006a,0x00000000,0x00030047,0x0000006c,
	0x00000000,0x00030047,0x0000006d,0x00000000,0x00030047,0x0000006e,0x00000000,0x00030047,
	0x0000006f,0x00000000,0x00030047,0x00000070,0x00000000,0x00030047,0x00000071,0x00000000,
	0x00030047,0x00000072,0x00000000,0x00030047,0x00000073,0x00000000,0x00030047,0x00000074,
	0x00000000,0x00030047,0x00000075,0x00000000,0x00030047,0x00000076,0x00000000,0x00030047,
	0x00000077,0x00000000,0x00030047,0x00000078,0x00000000,0x00030047,0x00000079,0x00000000,
	0x00030047,0x0000007a,0x00000000,0x00030047,0x0000007c,0x00000000,0x00030047,0x0000007d,
	0x00000000,0x00030047,0x0000007e,0x00000000,0x00030047,0x0000007f,0x00000000,0x00030047,
	0x00000081,0x00000000,0x00030047,0x00000083,0x00000000,0x00040047,0x00000083,0x0000001e,
	0x00000000,0x00030047,0x00000084,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,
	0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000003,
	0x00040020,0x00000008,0x00000007,0x00000007,0x00090019,0x0000000a,0x00000006,0x00000001,
	0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x0003001b,0x0000000b,0x0000000a,
	0x00040020,0x0000000c,0x00000000,0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000000,
	0x00040017,0x0000000f,0x00000006,0x00000002,0x00040020,0x00000010,0x00000001,0x0000000f,
	0x0004003b,0x00000010,0x00000011,0x00000001,0x00040017,0x00000013,0x00000006,0x00000004,
	0x0004003b,0x0000000c,0x00000017,0x00000000,0x0004002b,0x00000006,0x0000001c,0x40000000,
	0x0004003b,0x0000000c,0x0000001f,0x00000000,0x0004002b,0x00000006,0x00000025,0x3f800000,
	0x00040020,0x00000028,0x00000007,0x00000006,0x0004002b,0x00000006,0x0000002a,0x41200000,
	0x00040020,0x0000002c,0x00000001,0x00000007,0x0004003b,0x0000002c,0x0000002d,0x00000001,
	0x00040015,0x00000031,0x00000020,0x00000000,0x0004002b,0x00000031,0x00000032,0x00000000,
	0x0004003b,0x0000002c,0x00000035,0x00000001,0x0004002b,0x00000031,0x00000038,0x00000001,
	0x0004003b,0x0000002c,0x0000003b,0x00000001,0x0004002b,0x00000031,0x0000003f,0x00000002,
	0x0004003b,0x0000002c,0x00000042,0x00000001,0x0004002b,0x00000006,0x00000048,0x00000000,
	0x0006002c,0x00000007,0x00000049,0x00000048,0x00000048,0x00000048,0x00040015,0x0000004a,
	0x00000020,0x00000001,0x00040020,0x0000004b,0x00000007,0x0000004a,0x0004002b,0x0000004a,
	0x0000004d,0x00000000,0x0004002b,0x0000004a,0x00000054,0x000003e8,0x00020014,0x00000055,
	0x0004002b,0x00000006,0x00000058,0xbf13cd3a,0x0004002b,0x00000006,0x00000059,0x3f13cd3a,
	0x0006002c,0x00000007,0x0000005a,0x00000058,0x00000059,0x00000059,0x0004002b,0x00000006,
	0x0000006b,0x3f000000,0x0004002b,0x00000006,0x0000007b,0x3a83126f,0x0004002b,0x0000004a,
	0x00000080,0x00000001,0x00040020,0x00000082,0x00000003,0x00000013,0x0004003b,0x00000082,
	0x00000083,0x00000003,0x0004002b,0x00000031,0x00000087,0x00000003,0x00040020,0x00000088,
	0x00000003,0x00000006,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
	0x00000005,0x0004003b,0x00000008,0x00000009,0x00000007,0x0004003b,0x00000008,0x00000016,
	0x00000007,0x0004003b,0x00000008,0x0000001e,0x00000007,0x0004003b,0x00000028,0x00000029,
	0x00000007,0x0004003b,0x00000008,0x0000002b,0x00000007,0x0004003b,0x00000008,0x00000030,
	0x00000007,0x0004003b,0x00000008,0x00000047,0x00000007,0x0004003b,0x0000004b,0x0000004c,
	0x00000007,0x0004003b,0x00000008,0x00000057,0x00000007,0x0004003b,0x00000008,0x0000005b,
	0x00000007,0x0004003b,0x00000008,0x00000065,0x00000007,0x0004003b,0x00000008,0x0000006f,
	0x00000007,0x0004003d,0x0000000b,0x0000000e,0x0000000d,0x0004003d,0x0000000f,0x00000012,
	0x00000011,0x00050057,0x00000013,0x00000014,0x0000000e,0x00000012,0x0008004f,0x00000007,
	0x00000015,0x00000014,0x00000014,0x00000000,0x00000001,0x00000002,0x0003003e,0x00000009,
	0x00000015,0x0004003d,0x0000000b,0x00000018,0x00000017,0x0004003d,0x0000000f,0x00000019,
	0x00000011,0x00050057,0x00000013,0x0000001a,0x00000018,0x00000019,0x0008004f,0x00000007,
	0x0000001b,0x0000001a,0x0000001a,0x00000000,0x00000001,0x00000002,0x0005008e,0x00000007,
	0x0000001d,0x0000001b,0x0000001c,0x0003003e,0x00000016,0x0000001d,0x0004003d,0x0000000b,
	0x00000020,0x0000001f,0x0004003d,0x0000000f,0x00000021,0x00000011,0x00050057,0x00000013,
	0x00000022,0x00000020,0x00000021,0x0008004f,0x00000007,0x00000023,0x00000022,0x00000022,
	0x00000000,0x00000001,0x00000002,0x0005008e,0x00000007,0x00000024,0x00000023,0x0000001c,
	0x00060050,0x00000007,0x00000026,0x00000025,0x00000025,0x00000025,0x00050083,0x00000007,
	0x00000027,0x00000024,0x00000026,0x0003003e,0x0000001e,0x00000027,0x0003003e,0x00000029,
	0x0000002a,0x0004003d,0x00000007,0x0000002e,0x0000002d,0x0006000c,0x00000007,0x0000002f,
	0x00000001,0x00000045,0x0000002e,0x0003003e,0x0000002b,0x0000002f,0x00050041,0x00000028,
	0x00000033,0x0000001e,0x00000032,0x0004003d,0x00000006,0x00000034,0x00000033,0x0004003d,
	0x00000007,0x00000036,0x00000035,0x0005008e,0x00000007,0x00000037,0x00000036,0x00000034,
	0x00050041,0x00000028,0x00000039,0x0000001e,0x00000038,0x0004003d,0x00000006,0x0000003a,
	0x00000039,0x0004003d,0x00000007,0x0000003c,0x0000003b,0x0005008e,0x00000007,0x0000003d,
	0x0000003c,0x0000003a,0x00050081,0x00000007,0x0000003e,0x00000037,0x0000003d,0x00050041,
	0x00000028,0x00000040,0x0000001e,0x0000003f,0x0004003d,0x00000006,0x00000041,0x00000040,
	0x0004003d,0x00000007,0x00000043,0x00000042,0x0005008e,0x00000007,0x00000044,0x00000043,
	0x00000041,0x00050081,0x00000007,0x00000045,0x0000003e,0x00000044,0x0006000c,0x00000007,
	0x00000046,0x00000001,0x00000045,0x00000045,0x0003003e,0x00000030,0x00000046,0x0003003e,
	0x00000047,0x00000049,0x0003003e,0x0000004c,0x0000004d,0x000200f9,0x0000004e,0x000200f8,
	0x0000004e,0x000400f6,0x00000050,0x00000051,0x00000000,0x000200f9,0x00000052,0x000200f8,
	0x00000052,0x0004003d,0x0000004a,0x00000053,0x0000004c,0x000500b1,0x00000055,0x00000056,
	0x00000053,0x00000054,0x000400fa,0x00000056,0x0000004f,0x00000050,0x000200f8,0x0000004f,
	0x0003003e,0x00000057,0x0000005a,0x0004003d,0x00000007,0x0000005c,0x00000057,0x0004003d,
	0x00000007,0x0000005d,0x00000030,0x00050094,0x00000006,0x0000005e,0x0000005c,0x0000005d,
	0x00050085,0x00000006,0x0000005f,0x0000001c,0x0000005e,0x0004003d,0x00000007,0x00000060,
	0x00000030,0x0005008e,0x00000007,0x00000061,0x00000060,0x0000005f,0x0004003d,0x00000007,
	0x00000062,0x00000057,0x00050083,0x00000007,0x00000063,0x00000061,0x00000062,0x0006000c,
	0x00000007,0x00000064,0x00000001,0x00000045,0x00000063,0x0003003e,0x0000005b,0x00000064,
	0x0004003d,0x00000007,0x00000066,0x00000009,0x0004003d,0x00000007,0x00000067,0x00000030,
	0x0004003d,0x00000007,0x00000068,0x00000057,0x00050094,0x00000006,0x00000069,0x00000067,
	0x00000068,0x0007000c,0x00000006,0x0000006a,0x00000001,0x00000028,0x00000069,0x00000048,
	0x00050085,0x00000006,0x0000006c,0x0000006a,0x0000006b,0x00050081,0x00000006,0x0000006d,
	0x0000006c,0x0000006b,0x0005008e,0x00000007,0x0000006e,0x00000066,0x0000006d,0x0003003e,
	0x00000065,0x0000006e,0x0004003d,0x00000007,0x00000070,0x00000016,0x0004003d,0x00000007,
	0x00000071,0x0000005b,0x0004003d,0x00000007,0x00000072,0x0000002b,0x00050094,0x00000006,
	0x00000073,0x00000071,0x00000072,0x0007000c,0x00000006,0x00000074,0x00000001,0x00000028,
	0x00000073,0x00000048,0x0004003d,0x00000006,0x00000075,0x00000029,0x0007000c,0x00000006,
	0x00000076,0x00000001,0x0000001a,0x00000074,0x00000075,0x0005008e,0x00000007,0x00000077,
	0x00000070,0x00000076,0x0003003e,0x0000006f,0x00000077,0x0004003d,0x00000007,0x00000078,
	0x00000065,0x0004003d,0x00000007,0x00000079,0x0000006f,0x00050081,0x00000007,0x0000007a,
	0x00000078,0x00000079,0x0005008e,0x00000007,0x0000007c,0x0000007a,0x0000007b,0x0004003d,
	0x00000007,0x0000007d,0x00000047,0x00050081,0x00000007,0x0000007e,0x0000007d,0x0000007c,
	0x0003003e,0x00000047,0x0000007e,0x000200f9,0x00000051,0x000200f8,0x00000051,0x0004003d,
	0x0000004a,0x0000007f,0x0000004c,0x00050080,0x0000004a,0x00000081,0x0000007f,0x00000080,
	0x0003003e,0x0000004c,0x00000081,0x000200f9,0x0000004e,0x000200f8,0x00000050,0x0004003d,
	0x00000007,0x00000084,0x00000047,0x0004003d,0x00000013,0x00000085,0x00000083,0x0009004f,
	0x00000013,0x00000086,0x00000085,0x00000084,0x00000004,0x00000005,0x00000006,0x00000003,
	0x0003003e,0x00000083,0x00000086,0x00050041,0x00000088,0x00000089,0x00000083,0x00000087,
	0x0003003e,0x00000089,0x00000025,0x000100fd,0x00010038
};

static const char normalMapped2000LightsFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( binding = 1 ) uniform sampler2D Texture0;\n"
	"layout( binding = 2 ) uniform sampler2D Texture1;\n"
	"layout( binding = 3 ) uniform sampler2D Texture2;\n"
	"layout( location = 0 ) in lowp vec3 fragmentEyeDir;\n"
	"layout( location = 1 ) in lowp vec3 fragmentNormal;\n"
	"layout( location = 2 ) in lowp vec3 fragmentTangent;\n"
	"layout( location = 3 ) in lowp vec3 fragmentBinormal;\n"
	"layout( location = 4 ) in lowp vec2 fragmentUv0;\n"
	"layout( location = 0 ) out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	lowp vec3 diffuseMap = texture( Texture0, fragmentUv0 ).xyz;\n"
	"	lowp vec3 specularMap = texture( Texture1, fragmentUv0 ).xyz * 2.0;\n"
	"	lowp vec3 normalMap = texture( Texture2, fragmentUv0 ).xyz * 2.0 - 1.0;\n"
	"	lowp float specularPower = 10.0;\n"
	"	lowp vec3 eyeDir = normalize( fragmentEyeDir );\n"
	"	lowp vec3 normal = normalize( normalMap.x * fragmentTangent + normalMap.y * fragmentBinormal + normalMap.z * fragmentNormal );\n"
	"\n"
	"	lowp vec3 color = vec3( 0 );\n"
	"	for ( int i = 0; i < 2000; i++ )\n"
	"	{\n"
	"		lowp vec3 lightDir = normalize( vec3( -1.0, 1.0, 1.0 ) );\n"
	"		lowp vec3 lightReflection = normalize( 2.0 * dot( lightDir, normal ) * normal - lightDir );\n"
	"		lowp vec3 lightDiffuse = diffuseMap * ( max( dot( normal, lightDir ), 0.0 ) * 0.5 + 0.5 );\n"
	"		lowp vec3 lightSpecular = specularMap * pow( max( dot( lightReflection, eyeDir ), 0.0 ), specularPower );\n"
	"		color += ( lightDiffuse + lightSpecular ) * ( 1.0 / 2000.0 );\n"
	"	}\n"
	"\n"
	"	outColor.xyz = color;\n"
	"	outColor.w = 1.0;\n"
	"}\n";

static const unsigned int normalMapped2000LightsFragmentProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x0000008a,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000b000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000011,0x0000002d,0x00000035,
	0x0000003b,0x00000042,0x00000083,0x00030010,0x00000004,0x00000007,0x00030003,0x00000001,
	0x00000136,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00050005,0x00000009,0x66666964,
	0x4d657375,0x00007061,0x00050005,0x0000000d,0x74786554,0x30657275,0x00000000,0x00050005,
	0x00000011,0x67617266,0x746e656d,0x00307655,0x00050005,0x00000016,0x63657073,0x72616c75,
	0x0070614d,0x00050005,0x00000017,0x74786554,0x31657275,0x00000000,0x00050005,0x0000001e,
	0x6d726f6e,0x614d6c61,0x00000070,0x00050005,0x0000001f,0x74786554,0x32657275,0x00000000,
	0x00060005,0x00000029,0x63657073,0x72616c75,0x65776f50,0x00000072,0x00040005,0x0000002b,
	0x44657965,0x00007269,0x00060005,0x0000002d,0x67617266,0x746e656d,0x44657945,0x00007269,
	0x00040005,0x00000030,0x6d726f6e,0x00006c61,0x00060005,0x00000035,0x67617266,0x746e656d,
	0x676e6154,0x00746e65,0x00070005,0x0000003b,0x67617266,0x746e656d,0x6f6e6942,0x6c616d72,
	0x00000000,0x00060005,0x00000042,0x67617266,0x746e656d,0x6d726f4e,0x00006c61,0x00040005,
	0x00000047,0x6f6c6f63,0x00000072,0x00030005,0x0000004c,0x00000069,0x00050005,0x00000057,
	0x6867696c,0x72694474,0x00000000,0x00060005,0x0000005b,0x6867696c,0x66655274,0x7463656c,
	0x006e6f69,0x00060005,0x00000065,0x6867696c,0x66694474,0x65737566,0x00000000,0x00060005,
	0x0000006f,0x6867696c,0x65705374,0x616c7563,0x00000072,0x00050005,0x00000083,0x4374756f,
	0x726f6c6f,0x00000000,0x00030047,0x00000009,0x00000000,0x00030047,0x0000000d,0x00000000,
	0x00040047,0x0000000d,0x00000022,0x00000000,0x00040047,0x0000000d,0x00000021,0x00000001,
	0x00030047,0x0000000e,0x00000000,0x00030047,0x00000011,0x00000000,0x00040047,0x00000011,
	0x0000001e,0x00000004,0x00030047,0x00000012,0x00000000,0x00030047,0x00000014,0x00000000,
	0x00030047,0x00000015,0x00000000,0x00030047,0x00000016,0x00000000,0x00030047,0x00000017,
	0x00000000,0x00040047,0x00000017,0x00000022,0x00000000,0x00040047,0x00000017,0x00000021,
	0x00000002,0x00030047,0x00000018,0x00000000,0x00030047,0x00000019,0x00000000,0x00030047,
	0x0000001a,0x00000000,0x00030047,0x0000001b,0x00000000,0x00030047,0x0000001d,0x00000000,
	0x00030047,0x0000001e,0x00000000,0x00030047,0x0000001f,0x00000000,0x00040047,0x0000001f,
	0x00000022,0x00000000,0x00040047,0x0000001f,0x00000021,0x00000003,0x00030047,0x00000020,
	0x00000000,0x00030047,0x00000021,0x00000000,0x00030047,0x00000022,0x00000000,0x00030047,
	0x00000023,0x00000000,0x00030047,0x00000024,0x00000000,0x00030047,0x00000026,0x00000000,
	0x00030047,0x00000027,0x00000000,0x00030047,0x00000029,0x00000000,0x00030047,0x0000002b,
	0x00000000,0x00030047,0x0000002d,0x00000000,0x00040047,0x0000002d,0x0000001e,0x00000000,
	0x00030047,0x0000002e,0x00000000,0x00030047,0x0000002f,0x00000000,0x00030047,0x00000030,
	0x00000000,0x00030047,0x00000034,0x00000000,0x00030047,0x00000035,0x00000000,0x00040047,
	0x00000035,0x0000001e,0x00000002,0x00030047,0x00000036,0x00000000,0x00030047,0x00000037,
	0x00000000,0x00030047,0x0000003a,0x00000000,0x00030047,0x0000003b,0x00000000,0x00040047,
	0x0000003b,0x0000001e,0x00000003,0x00030047,0x0000003c,0x00000000,0x00030047,0x0000003d,
	0x00000000,0x00030047,0x0000003e,0x00000000,0x00030047,0x00000041,0x00000000,0x00030047,
	0x00000042,0x00000000,0x00040047,0x00000042,0x0000001e,0x00000001,0x00030047,0x00000043,
	0x00000000,0x00030047,0x00000044,0x00000000,0x00030047,0x00000045,0x00000000,0x00030047,
	0x00000046,0x00000000,0x00030047,0x00000047,0x00000000,0x00030047,0x0000004c,0x00000000,
	0x00030047,0x00000053,0x00000000,0x00030047,0x00000057,0x00000000,0x00030047,0x0000005b,
	0x00000000,0x00030047,0x0000005c,0x00000000,0x00030047,0x0000005d,0x00000000,0x00030047,
	0x0000005e,0x00000000,0x00030047,0x0000005f,0x00000000,0x00030047,0x00000060,0x00000000,
	0x00030047,0x00000061,0x00000000,0x00030047,0x00000062,0x00000000,0x00030047,0x00000063,
	0x00000000,0x00030047,0x00000064,0x00000000,0x00030047,0x00000065,0x00000000,0x00030047,
	0x00000066,0x00000000,0x00030047,0x00000067,0x00000000,0x00030047,0x00000068,0x00000000,
	0x00030047,0x00000069,0x00000000,0x00030047,0x0000006a,0x00000000,0x00030047,0x0000006c,
	0x00000000,0x00030047,0x0000006d,0x00000000,0x00030047,0x0000006e,0x00000000,0x00030047,
	0x0000006f,0x00000000,0x00030047,0x00000070,0x00000000,0x00030047,0x00000071,0x00000000,
	0x00030047,0x00000072,0x00000000,0x00030047,0x00000073,0x00000000,0x00030047,0x00000074,
	0x00000000,0x00030047,0x00000075,0x00000000,0x00030047,0x00000076,0x00000000,0x00030047,
	0x00000077,0x00000000,0x00030047,0x00000078,0x00000000,0x00030047,0x00000079,0x00000000,
	0x00030047,0x0000007a,0x00000000,0x00030047,0x0000007c,0x00000000,0x00030047,0x0000007d,
	0x00000000,0x00030047,0x0000007e,0x00000000,0x00030047,0x0000007f,0x00000000,0x00030047,
	0x00000081,0x00000000,0x00030047,0x00000083,0x00000000,0x00040047,0x00000083,0x0000001e,
	0x00000000,0x00030047,0x00000084,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,
	0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000003,
	0x00040020,0x00000008,0x00000007,0x00000007,0x00090019,0x0000000a,0x00000006,0x00000001,
	0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x0003001b,0x0000000b,0x0000000a,
	0x00040020,0x0000000c,0x00000000,0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000000,
	0x00040017,0x0000000f,0x00000006,0x00000002,0x00040020,0x00000010,0x00000001,0x0000000f,
	0x0004003b,0x00000010,0x00000011,0x00000001,0x00040017,0x00000013,0x00000006,0x00000004,
	0x0004003b,0x0000000c,0x00000017,0x00000000,0x0004002b,0x00000006,0x0000001c,0x40000000,
	0x0004003b,0x0000000c,0x0000001f,0x00000000,0x0004002b,0x00000006,0x00000025,0x3f800000,
	0x00040020,0x00000028,0x00000007,0x00000006,0x0004002b,0x00000006,0x0000002a,0x41200000,
	0x00040020,0x0000002c,0x00000001,0x00000007,0x0004003b,0x0000002c,0x0000002d,0x00000001,
	0x00040015,0x00000031,0x00000020,0x00000000,0x0004002b,0x00000031,0x00000032,0x00000000,
	0x0004003b,0x0000002c,0x00000035,0x00000001,0x0004002b,0x00000031,0x00000038,0x00000001,
	0x0004003b,0x0000002c,0x0000003b,0x00000001,0x0004002b,0x00000031,0x0000003f,0x00000002,
	0x0004003b,0x0000002c,0x00000042,0x00000001,0x0004002b,0x00000006,0x00000048,0x00000000,
	0x0006002c,0x00000007,0x00000049,0x00000048,0x00000048,0x00000048,0x00040015,0x0000004a,
	0x00000020,0x00000001,0x00040020,0x0000004b,0x00000007,0x0000004a,0x0004002b,0x0000004a,
	0x0000004d,0x00000000,0x0004002b,0x0000004a,0x00000054,0x000007d0,0x00020014,0x00000055,
	0x0004002b,0x00000006,0x00000058,0xbf13cd3a,0x0004002b,0x00000006,0x00000059,0x3f13cd3a,
	0x0006002c,0x00000007,0x0000005a,0x00000058,0x00000059,0x00000059,0x0004002b,0x00000006,
	0x0000006b,0x3f000000,0x0004002b,0x00000006,0x0000007b,0x3a03126f,0x0004002b,0x0000004a,
	0x00000080,0x00000001,0x00040020,0x00000082,0x00000003,0x00000013,0x0004003b,0x00000082,
	0x00000083,0x00000003,0x0004002b,0x00000031,0x00000087,0x00000003,0x00040020,0x00000088,
	0x00000003,0x00000006,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
	0x00000005,0x0004003b,0x00000008,0x00000009,0x00000007,0x0004003b,0x00000008,0x00000016,
	0x00000007,0x0004003b,0x00000008,0x0000001e,0x00000007,0x0004003b,0x00000028,0x00000029,
	0x00000007,0x0004003b,0x00000008,0x0000002b,0x00000007,0x0004003b,0x00000008,0x00000030,
	0x00000007,0x0004003b,0x00000008,0x00000047,0x00000007,0x0004003b,0x0000004b,0x0000004c,
	0x00000007,0x0004003b,0x00000008,0x00000057,0x00000007,0x0004003b,0x00000008,0x0000005b,
	0x00000007,0x0004003b,0x00000008,0x00000065,0x00000007,0x0004003b,0x00000008,0x0000006f,
	0x00000007,0x0004003d,0x0000000b,0x0000000e,0x0000000d,0x0004003d,0x0000000f,0x00000012,
	0x00000011,0x00050057,0x00000013,0x00000014,0x0000000e,0x00000012,0x0008004f,0x00000007,
	0x00000015,0x00000014,0x00000014,0x00000000,0x00000001,0x00000002,0x0003003e,0x00000009,
	0x00000015,0x0004003d,0x0000000b,0x00000018,0x00000017,0x0004003d,0x0000000f,0x00000019,
	0x00000011,0x00050057,0x00000013,0x0000001a,0x00000018,0x00000019,0x0008004f,0x00000007,
	0x0000001b,0x0000001a,0x0000001a,0x00000000,0x00000001,0x00000002,0x0005008e,0x00000007,
	0x0000001d,0x0000001b,0x0000001c,0x0003003e,0x00000016,0x0000001d,0x0004003d,0x0000000b,
	0x00000020,0x0000001f,0x0004003d,0x0000000f,0x00000021,0x00000011,0x00050057,0x00000013,
	0x00000022,0x00000020,0x00000021,0x0008004f,0x00000007,0x00000023,0x00000022,0x00000022,
	0x00000000,0x00000001,0x00000002,0x0005008e,0x00000007,0x00000024,0x00000023,0x0000001c,
	0x00060050,0x00000007,0x00000026,0x00000025,0x00000025,0x00000025,0x00050083,0x00000007,
	0x00000027,0x00000024,0x00000026,0x0003003e,0x0000001e,0x00000027,0x0003003e,0x00000029,
	0x0000002a,0x0004003d,0x00000007,0x0000002e,0x0000002d,0x0006000c,0x00000007,0x0000002f,
	0x00000001,0x00000045,0x0000002e,0x0003003e,0x0000002b,0x0000002f,0x00050041,0x00000028,
	0x00000033,0x0000001e,0x00000032,0x0004003d,0x00000006,0x00000034,0x00000033,0x0004003d,
	0x00000007,0x00000036,0x00000035,0x0005008e,0x00000007,0x00000037,0x00000036,0x00000034,
	0x00050041,0x00000028,0x00000039,0x0000001e,0x00000038,0x0004003d,0x00000006,0x0000003a,
	0x00000039,0x0004003d,0x00000007,0x0000003c,0x0000003b,0x0005008e,0x00000007,0x0000003d,
	0x0000003c,0x0000003a,0x00050081,0x00000007,0x0000003e,0x00000037,0x0000003d,0x00050041,
	0x00000028,0x00000040,0x0000001e,0x0000003f,0x0004003d,0x00000006,0x00000041,0x00000040,
	0x0004003d,0x00000007,0x00000043,0x00000042,0x0005008e,0x00000007,0x00000044,0x00000043,
	0x00000041,0x00050081,0x00000007,0x00000045,0x0000003e,0x00000044,0x0006000c,0x00000007,
	0x00000046,0x00000001,0x00000045,0x00000045,0x0003003e,0x00000030,0x00000046,0x0003003e,
	0x00000047,0x00000049,0x0003003e,0x0000004c,0x0000004d,0x000200f9,0x0000004e,0x000200f8,
	0x0000004e,0x000400f6,0x00000050,0x00000051,0x00000000,0x000200f9,0x00000052,0x000200f8,
	0x00000052,0x0004003d,0x0000004a,0x00000053,0x0000004c,0x000500b1,0x00000055,0x00000056,
	0x00000053,0x00000054,0x000400fa,0x00000056,0x0000004f,0x00000050,0x000200f8,0x0000004f,
	0x0003003e,0x00000057,0x0000005a,0x0004003d,0x00000007,0x0000005c,0x00000057,0x0004003d,
	0x00000007,0x0000005d,0x00000030,0x00050094,0x00000006,0x0000005e,0x0000005c,0x0000005d,
	0x00050085,0x00000006,0x0000005f,0x0000001c,0x0000005e,0x0004003d,0x00000007,0x00000060,
	0x00000030,0x0005008e,0x00000007,0x00000061,0x00000060,0x0000005f,0x0004003d,0x00000007,
	0x00000062,0x00000057,0x00050083,0x00000007,0x00000063,0x00000061,0x00000062,0x0006000c,
	0x00000007,0x00000064,0x00000001,0x00000045,0x00000063,0x0003003e,0x0000005b,0x00000064,
	0x0004003d,0x00000007,0x00000066,0x00000009,0x0004003d,0x00000007,0x00000067,0x00000030,
	0x0004003d,0x00000007,0x00000068,0x00000057,0x00050094,0x00000006,0x00000069,0x00000067,
	0x00000068,0x0007000c,0x00000006,0x0000006a,0x00000001,0x00000028,0x00000069,0x00000048,
	0x00050085,0x00000006,0x0000006c,0x0000006a,0x0000006b,0x00050081,0x00000006,0x0000006d,
	0x0000006c,0x0000006b,0x0005008e,0x00000007,0x0000006e,0x00000066,0x0000006d,0x0003003e,
	0x00000065,0x0000006e,0x0004003d,0x00000007,0x00000070,0x00000016,0x0004003d,0x00000007,
	0x00000071,0x0000005b,0x0004003d,0x00000007,0x00000072,0x0000002b,0x00050094,0x00000006,
	0x00000073,0x00000071,0x00000072,0x0007000c,0x00000006,0x00000074,0x00000001,0x00000028,
	0x00000073,0x00000048,0x0004003d,0x00000006,0x00000075,0x00000029,0x0007000c,0x00000006,
	0x00000076,0x00000001,0x0000001a,0x00000074,0x00000075,0x0005008e,0x00000007,0x00000077,
	0x00000070,0x00000076,0x0003003e,0x0000006f,0x00000077,0x0004003d,0x00000007,0x00000078,
	0x00000065,0x0004003d,0x00000007,0x00000079,0x0000006f,0x00050081,0x00000007,0x0000007a,
	0x00000078,0x00000079,0x0005008e,0x00000007,0x0000007c,0x0000007a,0x0000007b,0x0004003d,
	0x00000007,0x0000007d,0x00000047,0x00050081,0x00000007,0x0000007e,0x0000007d,0x0000007c,
	0x0003003e,0x00000047,0x0000007e,0x000200f9,0x00000051,0x000200f8,0x00000051,0x0004003d,
	0x0000004a,0x0000007f,0x0000004c,0x00050080,0x0000004a,0x00000081,0x0000007f,0x00000080,
	0x0003003e,0x0000004c,0x00000081,0x000200f9,0x0000004e,0x000200f8,0x00000050,0x0004003d,
	0x00000007,0x00000084,0x00000047,0x0004003d,0x00000013,0x00000085,0x00000083,0x0009004f,
	0x00000013,0x00000086,0x00000085,0x00000084,0x00000004,0x00000005,0x00000006,0x00000003,
	0x0003003e,0x00000083,0x00000086,0x00050041,0x00000088,0x00000089,0x00000083,0x00000087,
	0x0003003e,0x00000089,0x00000025,0x000100fd,0x00010038
};

static void Scene_Create( GpuContext_t * context, Scene_t * scene, SceneSettings_t * settings, GpuRenderPass_t * renderPass )
{
	memset( scene, 0, sizeof( Scene_t ) );

	GpuGeometry_CreateCube( context, &scene->geometry[0], 0.0f, 0.5f );			// 12 triangles
	GpuGeometry_CreateTorus( context, &scene->geometry[1], 8, 0.0f, 1.0f );		// 128 triangles
	GpuGeometry_CreateTorus( context, &scene->geometry[2], 16, 0.0f, 1.0f );	// 512 triangles
	GpuGeometry_CreateTorus( context, &scene->geometry[3], 32, 0.0f, 1.0f );	// 2048 triangles

	GpuGraphicsProgram_Create( context, &scene->program[0],
								PROGRAM( flatShadedVertexProgram ),
								sizeof( PROGRAM( flatShadedVertexProgram ) ),
								PROGRAM( flatShadedFragmentProgram ),
								sizeof( PROGRAM( flatShadedFragmentProgram ) ),
								flatShadedProgramParms, ARRAY_SIZE( flatShadedProgramParms ),
								VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL );
	GpuGraphicsProgram_Create( context, &scene->program[1],
								PROGRAM( normalMappedVertexProgram ),
								sizeof( PROGRAM( normalMappedVertexProgram ) ),
								PROGRAM( normalMapped100LightsFragmentProgram ),
								sizeof( PROGRAM( normalMapped100LightsFragmentProgram ) ),
								normalMappedProgramParms, ARRAY_SIZE( normalMappedProgramParms ),
								VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL |
								VERTEX_ATTRIBUTE_FLAG_TANGENT | VERTEX_ATTRIBUTE_FLAG_BINORMAL |
								VERTEX_ATTRIBUTE_FLAG_UV0 );
	GpuGraphicsProgram_Create( context, &scene->program[2],
								PROGRAM( normalMappedVertexProgram ),
								sizeof( PROGRAM( normalMappedVertexProgram ) ),
								PROGRAM( normalMapped1000LightsFragmentProgram ),
								sizeof( PROGRAM( normalMapped1000LightsFragmentProgram ) ),
								normalMappedProgramParms, ARRAY_SIZE( normalMappedProgramParms ),
								VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL |
								VERTEX_ATTRIBUTE_FLAG_TANGENT | VERTEX_ATTRIBUTE_FLAG_BINORMAL |
								VERTEX_ATTRIBUTE_FLAG_UV0 );
	GpuGraphicsProgram_Create( context, &scene->program[3],
								PROGRAM( normalMappedVertexProgram ),
								sizeof( PROGRAM( normalMappedVertexProgram ) ),
								PROGRAM( normalMapped2000LightsFragmentProgram ),
								sizeof( PROGRAM( normalMapped2000LightsFragmentProgram ) ),
								normalMappedProgramParms, ARRAY_SIZE( normalMappedProgramParms ),
								VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL |
								VERTEX_ATTRIBUTE_FLAG_TANGENT | VERTEX_ATTRIBUTE_FLAG_BINORMAL |
								VERTEX_ATTRIBUTE_FLAG_UV0 );

	for ( int i = 0; i < MAX_SCENE_TRIANGLE_LEVELS; i++ )
	{
		for ( int j = 0; j < MAX_SCENE_FRAGMENT_LEVELS; j++ )
		{
			GpuGraphicsPipelineParms_t pipelineParms;
			GpuGraphicsPipelineParms_Init( &pipelineParms );

			pipelineParms.renderPass = renderPass;
			pipelineParms.program = &scene->program[j];
			pipelineParms.geometry = &scene->geometry[i];

			GpuGraphicsPipeline_Create( context, &scene->pipelines[i][j], &pipelineParms );
		}
	}

	GpuBuffer_Create( context, &scene->sceneMatrices, GPU_BUFFER_TYPE_UNIFORM, 2 * sizeof( Matrix4x4f_t ), NULL, false );

	GpuTexture_CreateDefault( context, &scene->diffuseTexture, GPU_TEXTURE_DEFAULT_CHECKERBOARD, 256, 256, 1, 1, 1, true, false );
	GpuTexture_CreateDefault( context, &scene->specularTexture, GPU_TEXTURE_DEFAULT_CHECKERBOARD, 256, 256, 1, 1, 1, true, false );
	GpuTexture_CreateDefault( context, &scene->normalTexture, GPU_TEXTURE_DEFAULT_PYRAMIDS, 256, 256, 1, 1, 1, true, false );

	scene->settings = *settings;
	scene->newSettings = settings;

	const int maxDimension = 2 * ( 1 << ( MAX_SCENE_DRAWCALL_LEVELS - 1 ) );

	scene->bigRotationX = 0.0f;
	scene->bigRotationY = 0.0f;
	scene->smallRotationX = 0.0f;
	scene->smallRotationY = 0.0f;

	scene->modelMatrix = (Matrix4x4f_t *) AllocAlignedMemory( maxDimension * maxDimension * maxDimension * sizeof( Matrix4x4f_t ), sizeof( Matrix4x4f_t ) );
}

static void Scene_Destroy( GpuContext_t * context, Scene_t * scene )
{
	for ( int i = 0; i < MAX_SCENE_TRIANGLE_LEVELS; i++ )
	{
		for ( int j = 0; j < MAX_SCENE_FRAGMENT_LEVELS; j++ )
		{
			GpuGraphicsPipeline_Destroy( context, &scene->pipelines[i][j] );
		}
	}

	for ( int i = 0; i < MAX_SCENE_TRIANGLE_LEVELS; i++ )
	{
		GpuGeometry_Destroy( context, &scene->geometry[i] );
	}

	for ( int i = 0; i < MAX_SCENE_FRAGMENT_LEVELS; i++ )
	{
		GpuGraphicsProgram_Destroy( context, &scene->program[i] );
	}

	GpuBuffer_Destroy( context, &scene->sceneMatrices );

	GpuTexture_Destroy( context, &scene->diffuseTexture );
	GpuTexture_Destroy( context, &scene->specularTexture );
	GpuTexture_Destroy( context, &scene->normalTexture );

	FreeAlignedMemory( scene->modelMatrix );
	scene->modelMatrix = NULL;
}

static void Scene_UpdateSettings( Scene_t * scene )
{
	scene->settings = *scene->newSettings;
}

static void Scene_Simulate( Scene_t * scene, const Microseconds_t time )
{
	if ( scene->settings.simulationPaused )
	{
		return;
	}

	const float offset = time * ( MATH_PI / 1000.0f / 1000.0f );
	scene->bigRotationX = 20.0f * offset;
	scene->bigRotationY = 10.0f * offset;
	scene->smallRotationX = -60.0f * offset;
	scene->smallRotationY = -40.0f * offset;
}

static void Scene_UpdateMatrices( GpuCommandBuffer_t * commandBuffer, Scene_t * scene,
									const Matrix4x4f_t * viewMatrix, const Matrix4x4f_t * projectionMatrix )
{
	void * sceneMatrices = NULL;
	GpuBuffer_t * sceneMatricesBuffer = GpuCommandBuffer_MapBuffer( commandBuffer, &scene->sceneMatrices, &sceneMatrices );
	const int numMatrices = 1;
	memcpy( (char *)sceneMatrices + 0 * numMatrices * sizeof( Matrix4x4f_t ), viewMatrix, numMatrices * sizeof( Matrix4x4f_t ) );
	memcpy( (char *)sceneMatrices + 1 * numMatrices * sizeof( Matrix4x4f_t ), projectionMatrix, numMatrices * sizeof( Matrix4x4f_t ) );
	GpuCommandBuffer_UnmapBuffer( commandBuffer, &scene->sceneMatrices, sceneMatricesBuffer, GPU_BUFFER_UNMAP_TYPE_COPY_BACK );
}

static void Scene_Render( GpuCommandBuffer_t * commandBuffer, const Scene_t * scene )
{
	const int dimension = 2 * ( 1 << scene->settings.drawCallLevel );
	const float cubeOffset = ( dimension - 1.0f ) * 0.5f;
	const float cubeScale = 2.0f;

	Matrix4x4f_t bigRotationMatrix;
	Matrix4x4f_CreateRotation( &bigRotationMatrix, scene->bigRotationX, scene->bigRotationY, 0.0f );

	Matrix4x4f_t bigTranslationMatrix;
	Matrix4x4f_CreateTranslation( &bigTranslationMatrix, 0.0f, 0.0f, - 2.5f * dimension );

	Matrix4x4f_t bigTransformMatrix;
	Matrix4x4f_Multiply( &bigTransformMatrix, &bigTranslationMatrix, &bigRotationMatrix );

	Matrix4x4f_t smallRotationMatrix;
	Matrix4x4f_CreateRotation( &smallRotationMatrix, scene->smallRotationX, scene->smallRotationY, 0.0f );

	GpuGraphicsCommand_t command;
	GpuGraphicsCommand_Init( &command );
	GpuGraphicsCommand_SetPipeline( &command, &scene->pipelines[scene->settings.triangleLevel][scene->settings.fragmentLevel] );
	GpuGraphicsCommand_SetParmBufferUniform( &command, PROGRAM_UNIFORM_SCENE_MATRICES, &scene->sceneMatrices );
	GpuGraphicsCommand_SetParmTextureSampled( &command, PROGRAM_TEXTURE_0, ( scene->settings.fragmentLevel >= 1 ) ? &scene->diffuseTexture : NULL );
	GpuGraphicsCommand_SetParmTextureSampled( &command, PROGRAM_TEXTURE_1, ( scene->settings.fragmentLevel >= 1 ) ? &scene->specularTexture : NULL );
	GpuGraphicsCommand_SetParmTextureSampled( &command, PROGRAM_TEXTURE_2, ( scene->settings.fragmentLevel >= 1 ) ? &scene->normalTexture : NULL );

	for ( int x = 0; x < dimension; x++ )
	{
		for ( int y = 0; y < dimension; y++ )
		{
			for ( int z = 0; z < dimension; z++ )
			{
				Matrix4x4f_t smallTranslationMatrix;
				Matrix4x4f_CreateTranslation( &smallTranslationMatrix, cubeScale * ( x - cubeOffset ), cubeScale * ( y - cubeOffset ), cubeScale * ( z - cubeOffset ) );

				Matrix4x4f_t smallTransformMatrix;
				Matrix4x4f_Multiply( &smallTransformMatrix, &smallTranslationMatrix, &smallRotationMatrix );

				Matrix4x4f_t * modelMatrix = &scene->modelMatrix[( x * dimension + y ) * dimension + z];
				Matrix4x4f_Multiply( modelMatrix, &bigTransformMatrix, &smallTransformMatrix );

				GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, PROGRAM_UNIFORM_MODEL_MATRIX, modelMatrix );

				GpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
			}
		}
	}
}

/*
================================================================================================================================

Stats

================================================================================================================================
*/

static void PrintStats( const GpuWindow_t * window )
{
	const uint32_t version = window->context.device->physicalDeviceProperties.apiVersion;
	const uint32_t major = VK_VERSION_MAJOR( version );
	const uint32_t minor = VK_VERSION_MINOR( version );
	const uint32_t patch = VK_VERSION_PATCH( version );

	Print( "--------------------------------\n" );
	Print( "OS     : %s\n", GetOSVersion() );
	Print( "CPU    : %s\n", GetCPUVersion() );
	Print( "GPU    : %s\n", window->context.device->physicalDeviceProperties.deviceName );
	Print( "Vulkan : %d.%d.%d\n", major, minor, patch );
	Print( "Mode   : %s %dx%d %1.0f Hz\n", window->windowFullscreen ? "fullscreen" : "windowed",
					window->windowWidth, window->windowHeight, window->windowRefreshRate );
	Print( "--------------------------------\n" );
}

/*
================================================================================================================================

Dump GLSL

================================================================================================================================
*/

static void WriteTextFile( const char * path, const char * text )
{
	FILE * fp = fopen( path, "wb" );
	if ( fp == NULL )
	{
		Print( "Failed to write %s\n", path );
		return;
	}
	fwrite( text, strlen( text ), 1, fp );
	fclose( fp );
	Print( "Wrote %s\n", path );
}

typedef struct
{
	const char * fileName;
	const char * extension;
	const char * glsl;
} glsl_t;

static void DumpGLSL()
{
	glsl_t glsl[] =
	{
		{ "barGraphVertexProgram",					"vert",	barGraphVertexProgramGLSL },
		{ "barGraphFragmentProgram",				"frag",	barGraphFragmentProgramGLSL },
		{ "timeWarpSpatialVertexProgram",			"vert",	timeWarpSpatialVertexProgramGLSL },
		{ "timeWarpSpatialFragmentProgram",			"frag",	timeWarpSpatialFragmentProgramGLSL },
		{ "timeWarpChromaticVertexProgram",			"vert",	timeWarpChromaticVertexProgramGLSL },
		{ "timeWarpChromaticFragmentProgram",		"frag",	timeWarpChromaticFragmentProgramGLSL },
		{ "flatShadedVertexProgram",				"vert",	flatShadedVertexProgramGLSL },
		{ "flatShadedFragmentProgram",				"frag",	flatShadedFragmentProgramGLSL },
		{ "normalMappedVertexProgram",				"vert",	normalMappedVertexProgramGLSL },
		{ "normalMapped100LightsFragmentProgram",	"frag",	normalMapped100LightsFragmentProgramGLSL },
		{ "normalMapped1000LightsFragmentProgram",	"frag",	normalMapped1000LightsFragmentProgramGLSL },
		{ "normalMapped2000LightsFragmentProgram",	"frag",	normalMapped2000LightsFragmentProgramGLSL },

		{ "barGraphComputeProgram",					"comp", barGraphComputeProgramGLSL },
		{ "timeWarpTransformComputeProgram",		"comp", timeWarpTransformComputeProgramGLSL },
		{ "timeWarpSpatialComputeProgram",			"comp", timeWarpSpatialComputeProgramGLSL },
		{ "timeWarpChromaticComputeProgram",		"comp", timeWarpChromaticComputeProgramGLSL },
	};

	char path[1024];
	char batchFileBin[4096];
	char batchFileHex[4096];
	size_t batchFileBinLength = 0;
	size_t batchFileHexLength = 0;
	for ( size_t i = 0; i < ARRAY_SIZE( glsl ); i++ )
	{
		sprintf( path, "glsl/%sGLSL.%s", glsl[i].fileName, glsl[i].extension );
		WriteTextFile( path, glsl[i].glsl );

		batchFileBinLength += sprintf( batchFileBin + batchFileBinLength,
									"glslangValidator -V -o %sSPIRV.spv %sGLSL.%s\r\n",
									glsl[i].fileName, glsl[i].fileName, glsl[i].extension );
		batchFileHexLength += sprintf( batchFileHex + batchFileHexLength,
									"glslangValidator -V -x %sSPIRV.h %sGLSL.%s\r\n",
									glsl[i].fileName, glsl[i].fileName, glsl[i].extension );
	}

	WriteTextFile( "glsl/spirv_bin.bat", batchFileBin );
	WriteTextFile( "glsl/spirv_hex.bat", batchFileHex );
}

/*
================================================================================================================================

Startup settings.

StartupSettings_t

static int StartupSettings_StringToLevel( const char * string, const int maxLevels );
static int StartupSettings_StringToRenderMode( const char * string );
static int StartupSettings_StringToTimeWarpImplementation( const char * string );

================================================================================================================================
*/

typedef enum
{
	RENDER_MODE_ASYNC_TIME_WARP,
	RENDER_MODE_TIME_WARP,
	RENDER_MODE_SCENE,
	RENDER_MODE_MAX
} RenderMode_t;

typedef struct
{
	bool						fullscreen;
	bool						simulationPaused;
	bool						headRotationDisabled;
	int							drawCallLevel;
	int							triangleLevel;
	int							fragmentLevel;
	bool						useMultiView;
	bool						correctChromaticAberration;
	bool						hideGraphs;
	RenderMode_t				renderMode;
	TimeWarpImplementation_t	timeWarpImplementation;
	Microseconds_t				startupTimeMicroseconds;
	Microseconds_t				noVSyncMicroseconds;
	Microseconds_t				noLogMicroseconds;
} StartupSettings_t;

static int StartupSettings_StringToLevel( const char * string, const int maxLevels )
{
	const int level = atoi( string );
	return ( level >= 0 ) ? ( ( level < maxLevels ) ? level : maxLevels - 1 ) : 0;
}

static int StartupSettings_StringToRenderMode( const char * string )
{
	return	( ( strcmp( string, "atw" ) == 0 ) ? RENDER_MODE_ASYNC_TIME_WARP:
			( ( strcmp( string, "tw"  ) == 0 ) ? RENDER_MODE_TIME_WARP :
			RENDER_MODE_SCENE ) );
}

static int StartupSettings_StringToTimeWarpImplementation( const char * string )
{
	return	( ( strcmp( string, "graphics" ) == 0 ) ? TIMEWARP_IMPLEMENTATION_GRAPHICS :
			( ( strcmp( string, "compute"  ) == 0 ) ? TIMEWARP_IMPLEMENTATION_COMPUTE :
			TIMEWARP_IMPLEMENTATION_GRAPHICS ) );
}

/*
================================================================================================================================

Asynchronous time warp.

================================================================================================================================
*/

enum
{
	QUEUE_INDEX_TIMEWARP	= 0,
	QUEUE_INDEX_SCENE		= 1
};

// Two should be enough but use three to make absolutely sure there are no stalls due to buffer locking.
#define NUM_EYE_BUFFERS			3

#if defined( OS_ANDROID )
#define WINDOWED_PIXELS_WIDE	DISPLAY_PIXELS_WIDE
#define WINDOWED_PIXELS_HIGH	DISPLAY_PIXELS_HIGH
#else
#define WINDOWED_PIXELS_WIDE	ROUNDUP( DISPLAY_PIXELS_WIDE / 2, 8 )
#define WINDOWED_PIXELS_HIGH	ROUNDUP( DISPLAY_PIXELS_HIGH / 2, 8 )
#endif

typedef struct
{
	Signal_t				initialized;
	GpuContext_t *			shareContext;
	TimeWarp_t *			timeWarp;
	SceneSettings_t *		sceneSettings;

	volatile Microseconds_t	nextSwapTime;
	volatile bool			terminate;
	volatile bool			openFrameLog;
} SceneThreadData_t;

void SceneThread_Render( SceneThreadData_t * threadData )
{
	static const int EYE_WIDTH = 1024;
	static const int EYE_HEIGHT = 1024;

	Thread_SetAffinity( THREAD_AFFINITY_BIG_CORES );

	GpuContext_t context;
	GpuContext_CreateShared( &context, threadData->shareContext, QUEUE_INDEX_SCENE );

	GpuRenderPass_t renderPassSingleView;
	GpuRenderPass_Create( &context, &renderPassSingleView, GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_DEPTH_FORMAT_D24,
							GPU_RENDERPASS_TYPE_INLINE,
							GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER |
							GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER );

	GpuRenderPass_t renderPassMultiView;
	GpuRenderPass_Create( &context, &renderPassMultiView, GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_DEPTH_FORMAT_D24,
							GPU_RENDERPASS_TYPE_SECONDARY_COMMAND_BUFFERS,
							GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER |
							GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER );

	GpuFramebuffer_t framebuffer;
	GpuFramebuffer_CreateFromTextureArrays( &context, &framebuffer, &renderPassSingleView, EYE_WIDTH, EYE_HEIGHT, NUM_EYES, NUM_EYE_BUFFERS, false );

	GpuCommandBuffer_t eyeCommandBuffer[NUM_EYES];
	GpuTimer_t eyeTimer[NUM_EYES];
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		GpuCommandBuffer_Create( &context, &eyeCommandBuffer[eye], GPU_COMMAND_BUFFER_TYPE_PRIMARY, NUM_EYE_BUFFERS );
		GpuTimer_Create( &context, &eyeTimer[eye] );
	}

	GpuCommandBuffer_t sceneCommandBuffer;
	GpuCommandBuffer_Create( &context, &sceneCommandBuffer, GPU_COMMAND_BUFFER_TYPE_SECONDARY_CONTINUE_RENDER_PASS, NUM_EYE_BUFFERS );

	Scene_t scene;
	Scene_Create( &context, &scene, threadData->sceneSettings, &renderPassSingleView );

	const BodyInfo_t * bodyInfo = GetDefaultBodyInfo();

	Signal_Raise( &threadData->initialized );

	while ( !threadData->terminate )
	{
		if ( threadData->openFrameLog )
		{
			threadData->openFrameLog = false;
			FrameLog_Open( OUTPUT_PATH "framelog_scene.txt", 10 );
		}

		Scene_UpdateSettings( &scene );

		Scene_Simulate( &scene, threadData->nextSwapTime );

		Matrix4x4f_t hmdViewMatrix;
		GetHmdViewMatrixForTime( &hmdViewMatrix, threadData->nextSwapTime );

		Matrix4x4f_t eyeViewMatrix[2];
		Matrix4x4f_t eyeProjectionMatrix[2];
		for ( int eye = 0; eye < NUM_EYES; eye++ )
		{
			const float eyeOffset = ( eye ? -0.5f : 0.5f ) * bodyInfo->interpupillaryDistance;
			Matrix4x4f_t eyeOffsetMatrix;
			Matrix4x4f_CreateTranslation( &eyeOffsetMatrix, eyeOffset, 0.0f, 0.0f );
			Matrix4x4f_Multiply( &eyeViewMatrix[eye], &eyeOffsetMatrix, &hmdViewMatrix );
			Matrix4x4f_CreateProjectionFov( &eyeProjectionMatrix[eye], 90.0f, 72.0f, 0.0f, 0.0f, 0.1f, 0.0f );
		}

		FrameLog_BeginFrame();

		const Microseconds_t t0 = GetTimeMicroseconds();

		if ( scene.settings.useMultiView )
		{
			const ScreenRect_t sceneRect = { 0, 0, EYE_WIDTH, EYE_HEIGHT };
			GpuCommandBuffer_BeginSecondary( &sceneCommandBuffer, &renderPassMultiView, NULL );

			GpuCommandBuffer_SetViewport( &sceneCommandBuffer, &sceneRect );
			GpuCommandBuffer_SetScissor( &sceneCommandBuffer, &sceneRect );
			Scene_Render( &sceneCommandBuffer, &scene );

			GpuCommandBuffer_EndSecondary( &sceneCommandBuffer );
		}

		GpuTexture_t * eyeTexture[NUM_EYES] = { 0 };
		GpuFence_t * eyeCompletionFence[NUM_EYES] = { 0 };
		int eyeArrayLayer[NUM_EYES] = { 0, 1 };

		for ( int eye = 0; eye < NUM_EYES; eye++ )
		{
			const ScreenRect_t screenRect = GpuFramebuffer_GetRect( &framebuffer );

			GpuCommandBuffer_BeginPrimary( &eyeCommandBuffer[eye] );
			GpuCommandBuffer_BeginFramebuffer( &eyeCommandBuffer[eye], &framebuffer, eye, GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );

			Scene_UpdateMatrices( &eyeCommandBuffer[eye], &scene, eyeViewMatrix + eye, eyeProjectionMatrix + eye );

			GpuRenderPass_t * renderPass = scene.settings.useMultiView ? &renderPassMultiView : &renderPassSingleView;

			GpuCommandBuffer_BeginTimer( &eyeCommandBuffer[eye], &eyeTimer[eye] );
			GpuCommandBuffer_BeginRenderPass( &eyeCommandBuffer[eye], renderPass, &framebuffer, &screenRect );

			if ( scene.settings.useMultiView )
			{
				GpuCommandBuffer_SubmitSecondary( &sceneCommandBuffer, &eyeCommandBuffer[eye] );
			}
			else
			{
				GpuCommandBuffer_SetViewport( &eyeCommandBuffer[eye], &screenRect );
				GpuCommandBuffer_SetScissor( &eyeCommandBuffer[eye], &screenRect );
				Scene_Render( &eyeCommandBuffer[eye], &scene );
			}

			GpuCommandBuffer_EndRenderPass( &eyeCommandBuffer[eye], renderPass );
			GpuCommandBuffer_EndTimer( &eyeCommandBuffer[eye], &eyeTimer[eye] );

			GpuCommandBuffer_EndFramebuffer( &eyeCommandBuffer[eye], &framebuffer, eye, GPU_TEXTURE_USAGE_SAMPLED );
			GpuCommandBuffer_EndPrimary( &eyeCommandBuffer[eye] );

			eyeTexture[eye] = GpuFramebuffer_GetColorTexture( &framebuffer );
			eyeCompletionFence[eye] = GpuCommandBuffer_SubmitPrimary( &eyeCommandBuffer[eye] );
		}

		const Microseconds_t t1 = GetTimeMicroseconds();

		const float eyeTexturesCpuTime = ( t1 - t0 ) * ( 1.0f / 1000.0f );
		const float eyeTexturesGpuTime = GpuTimer_GetMilliseconds( &eyeTimer[0] ) + GpuTimer_GetMilliseconds( &eyeTimer[1] );

		FrameLog_EndFrame( eyeTexturesCpuTime, eyeTexturesGpuTime, GPU_TIMER_FRAMES_DELAYED );

		Matrix4x4f_t projectionMatrix;
		Matrix4x4f_CreateProjectionFov( &projectionMatrix, 80.0f, 80.0f, 0.0f, 0.0f, 0.1f, 0.0f );

		TimeWarp_PresentNewEyeTextures( threadData->timeWarp, &projectionMatrix, &hmdViewMatrix,
										eyeTexture, eyeCompletionFence, eyeArrayLayer,
										eyeTexturesCpuTime, eyeTexturesGpuTime );
	}

	GpuContext_WaitIdle( &context );

	Scene_Destroy( &context, &scene );

	GpuCommandBuffer_Destroy( &context, &sceneCommandBuffer );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		GpuTimer_Destroy( &context, &eyeTimer[eye] );
		GpuCommandBuffer_Destroy( &context, &eyeCommandBuffer[eye] );
	}

	GpuFramebuffer_Destroy( &context, &framebuffer );
	GpuRenderPass_Destroy( &context, &renderPassMultiView );
	GpuRenderPass_Destroy( &context, &renderPassSingleView );
	GpuContext_Destroy( &context );
}

void SceneThread_Create( Thread_t * sceneThread, SceneThreadData_t * sceneThreadData,
							GpuContext_t * shareContext, TimeWarp_t * timeWarp, SceneSettings_t * sceneSettings )
{
	Signal_Create( &sceneThreadData->initialized, true );
	sceneThreadData->shareContext = shareContext;
	sceneThreadData->timeWarp = timeWarp;
	sceneThreadData->sceneSettings = sceneSettings;
	sceneThreadData->nextSwapTime = GetTimeMicroseconds();
	sceneThreadData->terminate = false;
	sceneThreadData->openFrameLog = false;

	Thread_Create( sceneThread, "atw:scene", (threadFunction_t) SceneThread_Render, sceneThreadData );
	Thread_Signal( sceneThread );
	Signal_Wait( &sceneThreadData->initialized, -1 );
}

void SceneThread_Destroy( Thread_t * sceneThread, SceneThreadData_t * sceneThreadData )
{
	sceneThreadData->terminate = true;
	Signal_Raise( &sceneThreadData->timeWarp->newEyeTexturesConsumed );
	Signal_Destroy( &sceneThreadData->initialized );
	Thread_Destroy( sceneThread );
}

bool RenderAsyncTimeWarp( const StartupSettings_t * startupSettings )
{
	Thread_SetAffinity( THREAD_AFFINITY_BIG_CORES );
	Thread_SetRealTimePriority( 1 );

	DriverInstance_t instance;
	DriverInstance_Create( &instance );

	const GpuQueueInfo_t queueInfo =
	{
		2,
		GPU_QUEUE_PROPERTY_GRAPHICS | GPU_QUEUE_PROPERTY_COMPUTE,
		{ GPU_QUEUE_PRIORITY_HIGH, GPU_QUEUE_PRIORITY_MEDIUM }
	};

	GpuWindow_t window;
	GpuWindow_Create( &window, &instance, &queueInfo, QUEUE_INDEX_TIMEWARP,
						GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_DEPTH_FORMAT_NONE,
						startupSettings->fullscreen ? DISPLAY_PIXELS_WIDE : WINDOWED_PIXELS_WIDE,
						startupSettings->fullscreen ? DISPLAY_PIXELS_HIGH : WINDOWED_PIXELS_HIGH,
						startupSettings->fullscreen );

	int swapInterval = ( startupSettings->noVSyncMicroseconds <= 0 );
	GpuWindow_SwapInterval( &window, swapInterval );

	TimeWarp_t timeWarp;
	TimeWarp_Create( &timeWarp, &window );
	TimeWarp_SetBarGraphState( &timeWarp, startupSettings->hideGraphs ? BAR_GRAPH_HIDDEN : BAR_GRAPH_VISIBLE );
	TimeWarp_SetImplementation( &timeWarp, startupSettings->timeWarpImplementation );
	TimeWarp_SetChromaticAberrationCorrection( &timeWarp, startupSettings->correctChromaticAberration );
	TimeWarp_SetMultiView( &timeWarp, startupSettings->useMultiView );
	TimeWarp_SetDrawCallLevel( &timeWarp, startupSettings->drawCallLevel );
	TimeWarp_SetTriangleLevel( &timeWarp, startupSettings->triangleLevel );
	TimeWarp_SetFragmentLevel( &timeWarp, startupSettings->fragmentLevel );

	SceneSettings_t sceneSettings;
	SceneSettings_Init( &sceneSettings );
	SceneSettings_SetSimulationPaused( &sceneSettings, startupSettings->simulationPaused );
	SceneSettings_SetMultiView( &sceneSettings, startupSettings->useMultiView );
	SceneSettings_SetDrawCallLevel( &sceneSettings, startupSettings->drawCallLevel );
	SceneSettings_SetTriangleLevel( &sceneSettings, startupSettings->triangleLevel );
	SceneSettings_SetFragmentLevel( &sceneSettings, startupSettings->fragmentLevel );

	Thread_t sceneThread;
	SceneThreadData_t sceneThreadData;
	SceneThread_Create( &sceneThread, &sceneThreadData, &window.context, &timeWarp, &sceneSettings );

	hmd_headRotationDisabled = startupSettings->headRotationDisabled;

	Microseconds_t startupTimeMicroseconds = startupSettings->startupTimeMicroseconds;
	Microseconds_t noVSyncMicroseconds = startupSettings->noVSyncMicroseconds;
	Microseconds_t noLogMicroseconds = startupSettings->noLogMicroseconds;

	Thread_SetName( "atw:timewarp" );

	bool exit = false;
	while ( !exit )
	{
		const Microseconds_t time = GetTimeMicroseconds();

		const GpuWindowEvent_t handleEvent = GpuWindow_ProcessEvents( &window );
		if ( handleEvent == GPU_WINDOW_EVENT_ACTIVATED )
		{
			PrintStats( &window );
		}
		else if ( handleEvent == GPU_WINDOW_EVENT_EXIT )
		{
			exit = true;
		}

		if ( GpuWindow_CheckKeyboardKey( &window, KEY_ESCAPE ) )
		{
			GpuWindow_Exit( &window );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_R ) )
		{
			break;	// change render mode
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_F ) )
		{
			const bool fullscreen = !window.windowFullscreen;
			SceneThread_Destroy( &sceneThread, &sceneThreadData );
			TimeWarp_Destroy( &timeWarp, &window );
			GpuWindow_Destroy( &window );
			GpuWindow_Create( &window, &instance, &queueInfo, QUEUE_INDEX_TIMEWARP,
							GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_DEPTH_FORMAT_NONE,
							fullscreen ? DISPLAY_PIXELS_WIDE : WINDOWED_PIXELS_WIDE,
							fullscreen ? DISPLAY_PIXELS_HIGH : WINDOWED_PIXELS_HIGH,
							fullscreen );
			TimeWarp_Create( &timeWarp, &window );
			TimeWarp_SetDrawCallLevel( &timeWarp, SceneSettings_GetDrawCallLevel( &sceneSettings ) );
			TimeWarp_SetTriangleLevel( &timeWarp, SceneSettings_GetTriangleLevel( &sceneSettings ) );
			TimeWarp_SetFragmentLevel( &timeWarp, SceneSettings_GetFragmentLevel( &sceneSettings ) );
			SceneThread_Create( &sceneThread, &sceneThreadData, &window.context, &timeWarp, &sceneSettings );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_V ) ||
			( noVSyncMicroseconds > 0 && time - startupTimeMicroseconds > noVSyncMicroseconds ) )
		{
			swapInterval = !swapInterval;
			GpuWindow_SwapInterval( &window, swapInterval );
			noVSyncMicroseconds = 0;
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_L ) ||
			( noLogMicroseconds > 0 && time - startupTimeMicroseconds > noLogMicroseconds ) )
		{
			FrameLog_Open( OUTPUT_PATH "framelog_timewarp.txt", 10 );
			sceneThreadData.openFrameLog = true;
			noLogMicroseconds = 0;
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_H ) )
		{
			hmd_headRotationDisabled = !hmd_headRotationDisabled;
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_P ) )
		{
			SceneSettings_ToggleSimulationPaused( &sceneSettings );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_G ) )
		{
			TimeWarp_CycleBarGraphState( &timeWarp );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_Q ) )
		{
			SceneSettings_CycleDrawCallLevel( &sceneSettings );
			TimeWarp_SetDrawCallLevel( &timeWarp, SceneSettings_GetDrawCallLevel( &sceneSettings ) );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_W ) )
		{
			SceneSettings_CycleTriangleLevel( &sceneSettings );
			TimeWarp_SetTriangleLevel( &timeWarp, SceneSettings_GetTriangleLevel( &sceneSettings ) );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_E ) )
		{
			SceneSettings_CycleFragmentLevel( &sceneSettings );
			TimeWarp_SetFragmentLevel( &timeWarp, SceneSettings_GetFragmentLevel( &sceneSettings ) );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_I ) )
		{
			TimeWarp_CycleImplementation( &timeWarp );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_C ) )
		{
			TimeWarp_ToggleChromaticAberrationCorrection( &timeWarp );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_M ) )
		{
			SceneSettings_ToggleMultiView( &sceneSettings );
			TimeWarp_SetMultiView( &timeWarp, SceneSettings_GetMultiView( &sceneSettings ) );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_D ) )
		{
			DumpGLSL();
		}

		if ( window.windowActive )
		{
			TimeWarp_Render( &timeWarp, &window );
			GpuWindow_SwapBuffers( &window );

			sceneThreadData.nextSwapTime = GpuWindow_GetNextSwapTime( &window );
		}
	}

	GpuContext_WaitIdle( &window.context );
	SceneThread_Destroy( &sceneThread, &sceneThreadData );
	TimeWarp_Destroy( &timeWarp, &window );
	GpuWindow_Destroy( &window );
	DriverInstance_Destroy( &instance );

	return exit;
}

/*
================================================================================================================================

Time warp rendering test.

================================================================================================================================
*/

bool RenderTimeWarp( const StartupSettings_t * startupSettings )
{
	Thread_SetAffinity( THREAD_AFFINITY_BIG_CORES );

	DriverInstance_t instance;
	DriverInstance_Create( &instance );

	const GpuQueueInfo_t queueInfo =
	{
		1,
		GPU_QUEUE_PROPERTY_GRAPHICS | GPU_QUEUE_PROPERTY_COMPUTE,
		{ GPU_QUEUE_PRIORITY_MEDIUM }
	};

	GpuWindow_t window;
	GpuWindow_Create( &window, &instance, &queueInfo, 0,
						GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_DEPTH_FORMAT_NONE,
						startupSettings->fullscreen ? DISPLAY_PIXELS_WIDE : WINDOWED_PIXELS_WIDE,
						startupSettings->fullscreen ? DISPLAY_PIXELS_HIGH : WINDOWED_PIXELS_HIGH,
						startupSettings->fullscreen );

	int swapInterval = ( startupSettings->noVSyncMicroseconds <= 0 );
	GpuWindow_SwapInterval( &window, swapInterval );

	TimeWarp_t timeWarp;
	TimeWarp_Create( &timeWarp, &window );
	TimeWarp_SetBarGraphState( &timeWarp, startupSettings->hideGraphs ? BAR_GRAPH_HIDDEN : BAR_GRAPH_VISIBLE );
	TimeWarp_SetImplementation( &timeWarp, startupSettings->timeWarpImplementation );
	TimeWarp_SetChromaticAberrationCorrection( &timeWarp, startupSettings->correctChromaticAberration );

	hmd_headRotationDisabled = startupSettings->headRotationDisabled;

	Microseconds_t startupTimeMicroseconds = startupSettings->startupTimeMicroseconds;
	Microseconds_t noVSyncMicroseconds = startupSettings->noVSyncMicroseconds;
	Microseconds_t noLogMicroseconds = startupSettings->noLogMicroseconds;

	Thread_SetName( "atw:timewarp" );

	bool exit = false;
	while ( !exit )
	{
		const Microseconds_t time = GetTimeMicroseconds();

		const GpuWindowEvent_t handleEvent = GpuWindow_ProcessEvents( &window );
		if ( handleEvent == GPU_WINDOW_EVENT_ACTIVATED )
		{
			PrintStats( &window );
		}
		else if ( handleEvent == GPU_WINDOW_EVENT_EXIT )
		{
			exit = true;
		}

		if ( GpuWindow_CheckKeyboardKey( &window, KEY_ESCAPE ) )
		{
			GpuWindow_Exit( &window );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_R ) )
		{
			break;	// change render mode
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_F ) )
		{
			const bool fullscreen = !window.windowFullscreen;
			TimeWarp_Destroy( &timeWarp, &window );
			GpuWindow_Destroy( &window );
			GpuWindow_Create( &window, &instance, &queueInfo, 0,
							GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_DEPTH_FORMAT_NONE,
							fullscreen ? DISPLAY_PIXELS_WIDE : WINDOWED_PIXELS_WIDE,
							fullscreen ? DISPLAY_PIXELS_HIGH : WINDOWED_PIXELS_HIGH,
							fullscreen );
			TimeWarp_Create( &timeWarp, &window );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_V ) ||
			( noVSyncMicroseconds > 0 && time - startupTimeMicroseconds > noVSyncMicroseconds ) )
		{
			swapInterval = !swapInterval;
			GpuWindow_SwapInterval( &window, swapInterval );
			noVSyncMicroseconds = 0;
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_L ) ||
			( noLogMicroseconds > 0 && time - startupTimeMicroseconds > noLogMicroseconds ) )
		{
			FrameLog_Open( OUTPUT_PATH "framelog_timewarp.txt", 10 );
			noLogMicroseconds = 0;
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_H ) )
		{
			hmd_headRotationDisabled = !hmd_headRotationDisabled;
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_G ) )
		{
			TimeWarp_CycleBarGraphState( &timeWarp );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_I ) )
		{
			TimeWarp_CycleImplementation( &timeWarp );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_C ) )
		{
			TimeWarp_ToggleChromaticAberrationCorrection( &timeWarp );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_D ) )
		{
			DumpGLSL();
		}

		if ( window.windowActive )
		{
			TimeWarp_Render( &timeWarp, &window );
			GpuWindow_SwapBuffers( &window );
		}
	}

	GpuContext_WaitIdle( &window.context );
	TimeWarp_Destroy( &timeWarp, &window );
	GpuWindow_Destroy( &window );
	DriverInstance_Destroy( &instance );

	return exit;
}

/*
================================================================================================================================

Scene rendering test.

================================================================================================================================
*/

bool RenderScene( const StartupSettings_t * startupSettings )
{
	Thread_SetAffinity( THREAD_AFFINITY_BIG_CORES );

	DriverInstance_t instance;
	DriverInstance_Create( &instance );

	const GpuQueueInfo_t queueInfo =
	{
		1,
		GPU_QUEUE_PROPERTY_GRAPHICS,
		{ GPU_QUEUE_PRIORITY_MEDIUM }
	};

	GpuWindow_t window;
	GpuWindow_Create( &window, &instance, &queueInfo, 0,
						GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_DEPTH_FORMAT_D24,
						startupSettings->fullscreen ? DISPLAY_PIXELS_WIDE : WINDOWED_PIXELS_WIDE,
						startupSettings->fullscreen ? DISPLAY_PIXELS_HIGH : WINDOWED_PIXELS_HIGH,
						startupSettings->fullscreen );

	int swapInterval = ( startupSettings->noVSyncMicroseconds <= 0 );
	GpuWindow_SwapInterval( &window, swapInterval );

	GpuRenderPass_t renderPass;
	GpuRenderPass_Create( &window.context, &renderPass, window.colorFormat, window.depthFormat,
							GPU_RENDERPASS_TYPE_INLINE,
							GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER |
							GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER );

	GpuFramebuffer_t framebuffer;
	GpuFramebuffer_CreateFromSwapchain( &window, &framebuffer, &renderPass );

	GpuCommandBuffer_t commandBuffer;
	GpuCommandBuffer_Create( &window.context, &commandBuffer, GPU_COMMAND_BUFFER_TYPE_PRIMARY, GpuFramebuffer_GetBufferCount( &framebuffer ) );

	GpuTimer_t timer;
	GpuTimer_Create( &window.context, &timer );

	SceneSettings_t sceneSettings;
	SceneSettings_Init( &sceneSettings );
	SceneSettings_SetSimulationPaused( &sceneSettings, startupSettings->simulationPaused );
	SceneSettings_SetDrawCallLevel( &sceneSettings, startupSettings->drawCallLevel );
	SceneSettings_SetTriangleLevel( &sceneSettings, startupSettings->triangleLevel );
	SceneSettings_SetFragmentLevel( &sceneSettings, startupSettings->fragmentLevel );

	Scene_t scene;
	Scene_Create( &window.context, &scene, &sceneSettings, &renderPass );

	BarGraph_t frameCpuTimeBarGraph;
	BarGraph_CreateVirtualRect( &window.context, &frameCpuTimeBarGraph, &renderPass, &frameCpuTimeBarGraphRect, 64, 1, &colorDarkGrey );

	BarGraph_t frameGpuTimeBarGraph;
	BarGraph_CreateVirtualRect( &window.context, &frameGpuTimeBarGraph, &renderPass, &frameGpuTimeBarGraphRect, 64, 1, &colorDarkGrey );

	hmd_headRotationDisabled = startupSettings->headRotationDisabled;

	Microseconds_t startupTimeMicroseconds = startupSettings->startupTimeMicroseconds;
	Microseconds_t noVSyncMicroseconds = startupSettings->noVSyncMicroseconds;
	Microseconds_t noLogMicroseconds = startupSettings->noLogMicroseconds;

	Thread_SetName( "atw:scene" );

	bool exit = false;
	while ( !exit )
	{
		const Microseconds_t time = GetTimeMicroseconds();

		const GpuWindowEvent_t handleEvent = GpuWindow_ProcessEvents( &window );
		if ( handleEvent == GPU_WINDOW_EVENT_ACTIVATED )
		{
			PrintStats( &window );
		}
		else if ( handleEvent == GPU_WINDOW_EVENT_EXIT )
		{
			exit = true;
		}

		if ( GpuWindow_CheckKeyboardKey( &window, KEY_ESCAPE ) )
		{
			GpuWindow_Exit( &window );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_R ) )
		{
			break;	// change render mode
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_F ) )
		{
			const bool fullscreen = !window.windowFullscreen;
			Scene_Destroy( &window.context, &scene );
			GpuTimer_Destroy( &window.context, &timer );
			GpuCommandBuffer_Destroy( &window.context, &commandBuffer );
			GpuFramebuffer_Destroy( &window.context, &framebuffer );
			GpuRenderPass_Destroy( &window.context, &renderPass );
			GpuWindow_Destroy( &window );
			GpuWindow_Create( &window, &instance, &queueInfo, 0,
							GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_DEPTH_FORMAT_D24,
							fullscreen ? DISPLAY_PIXELS_WIDE : WINDOWED_PIXELS_WIDE,
							fullscreen ? DISPLAY_PIXELS_HIGH : WINDOWED_PIXELS_HIGH,
							fullscreen );
			GpuRenderPass_Create( &window.context, &renderPass, window.colorFormat, window.depthFormat,
									GPU_RENDERPASS_TYPE_INLINE,
									GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER |
									GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER );
			GpuFramebuffer_CreateFromSwapchain( &window, &framebuffer, &renderPass );
			GpuCommandBuffer_Create( &window.context, &commandBuffer, GPU_COMMAND_BUFFER_TYPE_PRIMARY, GpuFramebuffer_GetBufferCount( &framebuffer ) );
			GpuTimer_Create( &window.context, &timer );
			Scene_Create( &window.context, &scene, &sceneSettings, &renderPass );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_V ) ||
			( noVSyncMicroseconds > 0 && time - startupTimeMicroseconds > noVSyncMicroseconds ) )
		{
			swapInterval = !swapInterval;
			GpuWindow_SwapInterval( &window, swapInterval );
			noVSyncMicroseconds = 0;
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_L ) ||
			( noLogMicroseconds > 0 && time - startupTimeMicroseconds > noLogMicroseconds ) )
		{
			FrameLog_Open( OUTPUT_PATH "framelog_scene.txt", 10 );
			noLogMicroseconds = 0;
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_H ) )
		{
			hmd_headRotationDisabled = !hmd_headRotationDisabled;
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_P ) )
		{
			SceneSettings_ToggleSimulationPaused( &sceneSettings );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_Q ) )
		{
			SceneSettings_CycleDrawCallLevel( &sceneSettings );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_W ) )
		{
			SceneSettings_CycleTriangleLevel( &sceneSettings );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_E ) )
		{
			SceneSettings_CycleFragmentLevel( &sceneSettings );
		}
		if ( GpuWindow_CheckKeyboardKey( &window, KEY_D ) )
		{
			DumpGLSL();
		}

		if ( window.windowActive )
		{
			Scene_UpdateSettings( &scene );

			Scene_Simulate( &scene, GpuWindow_GetNextSwapTime( &window ) );

			Matrix4x4f_t viewMatrix;
			Matrix4x4f_CreateIdentity( &viewMatrix );

			Matrix4x4f_t projectionMatrix;
			Matrix4x4f_CreateProjectionFov( &projectionMatrix, 90.0f, 72.0f, 0.0f, 0.0f, 0.1f, 0.0f );

			FrameLog_BeginFrame();

			const Microseconds_t t0 = GetTimeMicroseconds();

			const ScreenRect_t screenRect = GpuFramebuffer_GetRect( &framebuffer );

			GpuCommandBuffer_BeginPrimary( &commandBuffer );
			GpuCommandBuffer_BeginFramebuffer( &commandBuffer, &framebuffer, 0, GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );

			Scene_UpdateMatrices( &commandBuffer, &scene, &viewMatrix, &projectionMatrix );

			BarGraph_UpdateGraphics( &commandBuffer, &frameCpuTimeBarGraph );
			BarGraph_UpdateGraphics( &commandBuffer, &frameGpuTimeBarGraph );

			GpuCommandBuffer_BeginTimer( &commandBuffer, &timer );
			GpuCommandBuffer_BeginRenderPass( &commandBuffer, &renderPass, &framebuffer, &screenRect );

			GpuCommandBuffer_SetViewport( &commandBuffer, &screenRect );
			GpuCommandBuffer_SetScissor( &commandBuffer, &screenRect );
			Scene_Render( &commandBuffer, &scene );

			BarGraph_RenderGraphics( &commandBuffer, &frameCpuTimeBarGraph );
			BarGraph_RenderGraphics( &commandBuffer, &frameGpuTimeBarGraph );

			GpuCommandBuffer_EndRenderPass( &commandBuffer, &renderPass );
			GpuCommandBuffer_EndTimer( &commandBuffer, &timer );

			GpuCommandBuffer_EndFramebuffer( &commandBuffer, &framebuffer, 0, GPU_TEXTURE_USAGE_PRESENTATION );
			GpuCommandBuffer_EndPrimary( &commandBuffer );

			GpuCommandBuffer_SubmitPrimary( &commandBuffer );

			const Microseconds_t t1 = GetTimeMicroseconds();

			const float sceneCpuTimeMilliseconds = ( t1 - t0 ) * ( 1.0f / 1000.0f );
			const float sceneGpuTimeMilliseconds = GpuTimer_GetMilliseconds( &timer );

			FrameLog_EndFrame( sceneCpuTimeMilliseconds, sceneGpuTimeMilliseconds, GPU_TIMER_FRAMES_DELAYED );

			BarGraph_AddBar( &frameCpuTimeBarGraph, 0, sceneCpuTimeMilliseconds * window.windowRefreshRate * ( 1.0f / 1000.0f ), &colorGreen, true );
			BarGraph_AddBar( &frameGpuTimeBarGraph, 0, sceneGpuTimeMilliseconds * window.windowRefreshRate * ( 1.0f / 1000.0f ), &colorGreen, true );

			GpuWindow_SwapBuffers( &window );
		}
	}

	GpuContext_WaitIdle( &window.context );
	BarGraph_Destroy( &window.context, &frameGpuTimeBarGraph );
	BarGraph_Destroy( &window.context, &frameCpuTimeBarGraph );
	Scene_Destroy( &window.context, &scene );
	GpuTimer_Destroy( &window.context, &timer );
	GpuCommandBuffer_Destroy( &window.context, &commandBuffer );
	GpuFramebuffer_Destroy( &window.context, &framebuffer );
	GpuRenderPass_Destroy( &window.context, &renderPass );
	GpuWindow_Destroy( &window );
	DriverInstance_Destroy( &instance );

	return exit;
}

/*
================================================================================================================================

Startup

================================================================================================================================
*/

static int StartApplication( int argc, char * argv[] )
{
	StartupSettings_t startupSettings;
	memset( &startupSettings, 0, sizeof( startupSettings ) );
	startupSettings.startupTimeMicroseconds = GetTimeMicroseconds();
	
	for ( int i = 1; i < argc; i++ )
	{
		const char * arg = argv[i];
		if ( arg[0] == '-' ) { arg++; }

		if ( strcmp( arg, "f" ) == 0 && i + 0 < argc )		{ startupSettings.fullscreen = true; }
		else if ( strcmp( arg, "v" ) == 0 && i + 1 < argc )	{ startupSettings.noVSyncMicroseconds = (Microseconds_t)( atof( argv[++i] ) * 1000 * 1000 ); }
		else if ( strcmp( arg, "h" ) == 0 && i + 0 < argc )	{ startupSettings.headRotationDisabled = true; }
		else if ( strcmp( arg, "p" ) == 0 && i + 0 < argc )	{ startupSettings.simulationPaused = true; }
		else if ( strcmp( arg, "q" ) == 0 && i + 1 < argc )	{ startupSettings.drawCallLevel = StartupSettings_StringToLevel( argv[++i], MAX_SCENE_DRAWCALL_LEVELS ); }
		else if ( strcmp( arg, "w" ) == 0 && i + 1 < argc )	{ startupSettings.triangleLevel = StartupSettings_StringToLevel( argv[++i], MAX_SCENE_TRIANGLE_LEVELS ); }
		else if ( strcmp( arg, "e" ) == 0 && i + 1 < argc )	{ startupSettings.fragmentLevel = StartupSettings_StringToLevel( argv[++i], MAX_SCENE_FRAGMENT_LEVELS ); }
		else if ( strcmp( arg, "m" ) == 0 && i + 0 < argc )	{ startupSettings.useMultiView = ( atoi( argv[++i] ) != 0 ); }
		else if ( strcmp( arg, "c" ) == 0 && i + 1 < argc )	{ startupSettings.correctChromaticAberration = ( atoi( argv[++i] ) != 0 ); }
		else if ( strcmp( arg, "r" ) == 0 && i + 1 < argc )	{ startupSettings.renderMode = StartupSettings_StringToRenderMode( argv[++i] ); }
		else if ( strcmp( arg, "i" ) == 0 && i + 1 < argc )	{ startupSettings.timeWarpImplementation = (TimeWarpImplementation_t)StartupSettings_StringToTimeWarpImplementation( argv[++i] ); }
		else if ( strcmp( arg, "g" ) == 0 && i + 0 < argc )	{ startupSettings.hideGraphs = true; }
		else if ( strcmp( arg, "l" ) == 0 && i + 1 < argc )	{ startupSettings.noLogMicroseconds = (Microseconds_t)( atof( argv[++i] ) * 1000 * 1000 ); }
		else if ( strcmp( arg, "d" ) == 0 && i + 0 < argc )	{ DumpGLSL(); exit( 0 ); }
		else
		{
			Print( "Unknown option: %s\n"
				   "atw_vulkan [options]\n"
				   "options:\n"
				   "   -f         start fullscreen\n"
				   "   -v <s>     start with V-Sync disabled for this many seconds\n"
				   "   -h         start with head rotation disabled\n"
				   "   -p         start with the simulation paused\n"
				   "   -q <0-3>   set per eye draw calls level\n"
				   "   -w <0-3>   set per eye triangles per draw call level\n"
				   "   -e <0-3>   set per eye fragment program complexity level\n"
				   "   -m <0-1>   enable/disable multi-view\n"
				   "   -c <0-1>   enable/disable correction for chromatic aberration\n"
				   "   -r <name>  set the render mode: atw, tw, scene\n"
				   "   -i <name>  set time warp implementation: graphics, compute\n"
				   "   -g         hide graphs\n"
				   "   -l <s>     log 10 frames of Vulkan commands after this many seconds\n"
				   "   -d         dump GLSL to files for conversion to SPIR-V\n",
				   arg );
			return 1;
		}
	}

	//startupSettings.headRotationDisabled = true;
	//startupSettings.simulationPaused = true;
	//startupSettings.useMultiView = true;
	//startupSettings.correctChromaticAberration = true;
	//startupSettings.renderMode = 1;
	//startupSettings.timeWarpImplementation = TIMEWARP_IMPLEMENTATION_COMPUTE;

	Print( "    fullscreen = %d\n",					startupSettings.fullscreen );
	Print( "    noVSyncMicroseconds = %lld\n",		startupSettings.noVSyncMicroseconds );
	Print( "    headRotationDisabled = %d\n",		startupSettings.headRotationDisabled );
	Print( "    simulationPaused = %d\n",			startupSettings.simulationPaused );
	Print( "    drawCallLevel = %d\n",				startupSettings.drawCallLevel );
	Print( "    triangleLevel = %d\n",				startupSettings.triangleLevel );
	Print( "    fragmentLevel = %d\n",				startupSettings.fragmentLevel );
	Print( "    useMultiView = %d\n",				startupSettings.useMultiView );
	Print( "    correctChromaticAberration = %d\n",	startupSettings.correctChromaticAberration );
	Print( "    renderMode = %d\n",					startupSettings.renderMode );
	Print( "    timeWarpImplementation = %d\n",		startupSettings.timeWarpImplementation );
	Print( "    hideGraphs = %d\n",					startupSettings.hideGraphs );
	Print( "    noLogMicroseconds = %lld\n",		startupSettings.noLogMicroseconds );

	for ( bool exit = false; !exit; )
	{
		if ( startupSettings.renderMode == RENDER_MODE_ASYNC_TIME_WARP )
		{
			exit = RenderAsyncTimeWarp( &startupSettings );
		}
		else if ( startupSettings.renderMode == RENDER_MODE_TIME_WARP )
		{
			exit = RenderTimeWarp( &startupSettings );
		}
		else if ( startupSettings.renderMode == RENDER_MODE_SCENE )
		{
			exit = RenderScene( &startupSettings );
		}
		if ( !exit )
		{
			startupSettings.renderMode = (RenderMode_t) ( ( startupSettings.renderMode + 1 ) % RENDER_MODE_MAX );
		}
	}

	return 0;
}

#if defined( OS_WINDOWS )

int APIENTRY WinMain( HINSTANCE hCurrentInst, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow )
{
	UNUSED_PARM( hCurrentInst );
	UNUSED_PARM( hPreviousInst );
	UNUSED_PARM( nCmdShow );

	int		argc = 0;
	char *	argv[32];

	char filename[_MAX_PATH];
	GetModuleFileNameA( NULL, filename, _MAX_PATH );
	argv[argc++] = filename;

	while ( argc < 32 )
	{
		while ( lpszCmdLine[0] != '\0' && lpszCmdLine[0] == ' ' ) { lpszCmdLine++; }
		if ( lpszCmdLine[0] == '\0' ) { break; }

		argv[argc++] = lpszCmdLine;
        
		while ( lpszCmdLine[0] != '\0' && lpszCmdLine[0] != ' ' ) { lpszCmdLine++; }
		if ( lpszCmdLine[0] == '\0' ) { break; }
        
		*lpszCmdLine++ = '\0';
	}

	return StartApplication( argc, argv );
}

#elif defined( OS_MAC )

static const char * FormatString( const char * format, ... )
{
	static int index = 0;
	static char buffer[2][4096];
	index ^= 1;
	va_list args;
	va_start( args, format );
	vsnprintf( buffer[index], sizeof( buffer[index] ), format, args );
	va_end( args );
	return buffer[index];
}

void SystemCommandVerbose( const char * command )
{
	int result = system( command );
	printf( "%s : %s\n", command, ( result == 0 ) ? "\e[0;32msuccessful\e[0m" : "\e[0;31mfailed\e[0m" );
}

void WriteTextFileVerbose( const char * fileName, const char * text )
{
	FILE * file = fopen( fileName, "wb" );
	int elem = 0;
	if ( file != NULL )
	{
		elem = fwrite( text, strlen( text ), 1, file );
		fclose( file );
	}
	printf( "write %s %s\n", fileName, ( elem == 1 ) ? "\e[0;32msuccessful\e[0m" : "\e[0;31mfailed\e[0m" );
}

void CreateBundle( const char * exePath )
{
	const char * bundleIdentifier = "ATW";
	const char * bundleName = "ATW";
	const char * bundleSignature = "atwx";

	const char * infoPlistText =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<plist version=\"1.0\">\n"
		"<dict>\n"
		"	<key>BuildMachineOSBuild</key>\n"
		"	<string>13F34</string>\n"
		"	<key>CFBundleDevelopmentRegion</key>\n"
		"	<string>en</string>\n"
		"	<key>CFBundleExecutable</key>\n"
		"	<string>%s</string>\n"					// %s for executable name
		"	<key>CFBundleIdentifier</key>\n"
		"	<string>%s</string>\n"					// %s for bundleIdentifier
		"	<key>CFBundleInfoDictionaryVersion</key>\n"
		"	<string>6.0</string>\n"
		"	<key>CFBundleName</key>\n"
		"	<string>%s</string>\n"					// %s for bundleName
		"	<key>CFBundlePackageType</key>\n"
		"	<string>APPL</string>\n"
		"	<key>CFBundleShortVersionString</key>\n"
		"	<string>1.0</string>\n"
		"	<key>CFBundleSignature</key>\n"
		"	<string>atwx</string>\n"				// %s for bundleSignature
		"	<key>CFBundleVersion</key>\n"
		"	<string>1</string>\n"
		"	<key>DTCompiler</key>\n"
		"	<string>com.apple.compilers.llvm.clang.1_0</string>\n"
		"	<key>DTPlatformBuild</key>\n"
		"	<string>6A2008a</string>\n"
		"	<key>DTPlatformVersion</key>\n"
		"	<string>GM</string>\n"
		"	<key>DTSDKBuild</key>\n"
		"	<string>14A382</string>\n"
		"	<key>DTSDKName</key>\n"
		"	<string>macosx10.10</string>\n"
		"	<key>DTXcode</key>\n"
		"	<string>0611</string>\n"
		"	<key>DTXcodeBuild</key>\n"
		"	<string>6A2008a</string>\n"
		"	<key>LSMinimumSystemVersion</key>\n"
		"	<string>10.9</string>\n"
		"	<key>NSMainNibFile</key>\n"
		"	<string>MainMenu</string>\n"
		"	<key>NSPrincipalClass</key>\n"
		"	<string>NSApplication</string>\n"
		"</dict>\n"
		"</plist>\n";

	const char * exeName = exePath + strlen( exePath ) - 1;
	for ( ; exeName > exePath && exeName[-1] != '/'; exeName-- ) {}

	SystemCommandVerbose( FormatString( "rm -r %s.app", exePath ) );
	SystemCommandVerbose( FormatString( "mkdir %s.app", exePath ) );
	SystemCommandVerbose( FormatString( "mkdir %s.app/Contents", exePath ) );
	SystemCommandVerbose( FormatString( "mkdir %s.app/Contents/MacOS", exePath ) );
	SystemCommandVerbose( FormatString( "cp %s %s.app/Contents/MacOS", exePath, exePath ) );
	WriteTextFileVerbose( FormatString( "%s.app/Contents/Info.plist", exePath ),
							FormatString( infoPlistText, exeName, bundleIdentifier, bundleName, bundleSignature ) );
}

void LaunchBundle( int argc, char * argv[] )
{
	// Print command to open the bundled application.
	char command[2048];
	int length = snprintf( command, sizeof( command ), "open %s.app", argv[0] );

	// Append all the original command-line arguments.
	const char * argsParm = " --args";
	const int argsParmLength = strlen( argsParm );
	if ( argc > 1 && argsParmLength + 1 < sizeof( command ) - length )
	{
		strcpy( command + length, argsParm );
		length += argsParmLength;
		for ( int i = 1; i < argc; i++ )
		{
			const int argLength = strlen( argv[i] );
			if ( argLength + 2 > sizeof( command ) - length )
			{
				break;
			}
			strcpy( command + length + 0, " " );
			strcpy( command + length + 1, argv[i] );
			length += 1 + argLength;
		}
	}

	// Launch the bundled application with the original command-line arguments.
	SystemCommandVerbose( command );
}

void SetBundleCWD( const char * bundledExecutablePath )
{
	// Inside a bundle, an executable lives three folders and
	// four forward slashes deep: /name.app/Contents/MacOS/name
	char cwd[1024];
	strcpy( cwd, bundledExecutablePath );
	for ( int i = strlen( cwd ) - 1, slashes = 0; i >= 0 && slashes < 4; i-- )
	{
		slashes += ( cwd[i] == '/' );
		cwd[i] = '\0';
	}
	int result = chdir( cwd );
	Print( "chdir( \"%s\" ) %s", cwd, ( result == 0 ) ? "successful" : "failed" );
}

int main( int argc, char * argv[] )
{
	/*
		When an application executable is not launched from a bundle, Mac OS X
		considers the application to be a console application with only text output
		and console keyboard input. As a result, an application will not receive
		keyboard events unless the application is launched from a bundle.
		Programmatically created graphics windows are also unable to properly
		acquire focus unless the application is launched from a bundle.

		If the executable was not launched from a bundle then automatically create
		a bundle right here and then launch the bundled application.
	*/
	if ( strstr( argv[0], "/Contents/MacOS/" ) == NULL )
	{
		CreateBundle( argv[0] );
		LaunchBundle( argc, argv );
		return 0;
	}

	SetBundleCWD( argv[0] );

	autoReleasePool = [[NSAutoreleasePool alloc] init];
	
	[NSApplication sharedApplication];
	[NSApp finishLaunching];
	[NSApp activateIgnoringOtherApps:YES];

	return StartApplication( argc, argv );
}

#elif defined( OS_LINUX )

int main( int argc, char * argv[] )
{
	return StartApplication( argc, argv );
}

#elif defined( OS_ANDROID )

#define MAX_ARGS		32
#define MAX_ARGS_BUFFER	1024

typedef struct
{
	char	buffer[MAX_ARGS_BUFFER];
	char *	argv[MAX_ARGS];
	int		argc;
} AndroidParm_t;

// adb shell am start -n com.vulkansamples.atw_vulkan/android.app.NativeActivity -a "android.intent.action.MAIN" --es "args" "\"-r tw\""
void GetIntentParms( AndroidParm_t * parms )
{
	parms->buffer[0] = '\0';
	parms->argv[0] = "atw_vulkan";
	parms->argc = 1;

	Java_t java;
	java.vm = global_app->activity->vm;
	(*java.vm)->AttachCurrentThread( java.vm, &java.env, NULL );
	java.activity = global_app->activity->clazz;

	jclass activityClass = (*java.env)->GetObjectClass( java.env, java.activity );
	jmethodID getIntenMethodId = (*java.env)->GetMethodID( java.env, activityClass, "getIntent", "()Landroid/content/Intent;" );
	jobject intent = (*java.env)->CallObjectMethod( java.env, java.activity, getIntenMethodId );
	(*java.env)->DeleteLocalRef( java.env, activityClass );

	jclass intentClass = (*java.env)->GetObjectClass( java.env, intent );
	jmethodID getStringExtraMethodId = (*java.env)->GetMethodID( java.env, intentClass, "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;" );

	jstring argsJstring = (*java.env)->NewStringUTF( java.env, "args" );
	jstring extraJstring = (*java.env)->CallObjectMethod( java.env, intent, getStringExtraMethodId, argsJstring );
	if ( extraJstring != NULL )
	{
		const char * args = (*java.env)->GetStringUTFChars( java.env, extraJstring, 0 );
		strncpy( parms->buffer, args, sizeof( parms->buffer ) - 1 );
		parms->buffer[sizeof( parms->buffer ) - 1] = '\0';
		(*java.env)->ReleaseStringUTFChars( java.env, extraJstring, args );

		Print( "    args = %s\n", args );

		char * ptr = parms->buffer;
		while ( parms->argc < MAX_ARGS )
		{
			while ( ptr[0] != '\0' && ptr[0] == ' ' ) { ptr++; }
			if ( ptr[0] == '\0' ) { break; }

			parms->argv[parms->argc++] = ptr;

			while ( ptr[0] != '\0' && ptr[0] != ' ' ) { ptr++; }
			if ( ptr[0] == '\0' ) { break; }

			*ptr++ = '\0';
		}
	}

	(*java.env)->DeleteLocalRef( java.env, argsJstring );
	(*java.env)->DeleteLocalRef( java.env, intentClass );
	(*java.vm)->DetachCurrentThread( java.vm );
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events.
 */
void android_main( struct android_app * app )
{
	Print( "----------------------------------------------------------------" );
	Print( "onCreate()" );
	Print( "    android_main()" );

	// Make sure the native app glue is not stripped.
	app_dummy();

	global_app = app;

	AndroidParm_t parms;
	GetIntentParms( &parms );

	StartApplication( parms.argc, parms.argv );
}

#endif

/*
================================================================================================

Description	:	Asynchronous Time Warp test utility for Vulkan.
Author		:	J.M.P. van Waveren
Date		:	08/11/2015
Language	:	C99
Format		:	Real tabs with the tab size equal to 4 spaces.
Copyright	:	Copyright (c) 2016 Oculus VR, LLC. All Rights reserved.
			:	Portions copyright (c) 2016 The Brenwill Workshop Ltd. All Rights reserved.


LICENSE
=======

Copyright (c) 2016 Oculus VR, LLC.
Portions of macOS, iOS, and MoltenVK functionality copyright (c) 2016 The Brenwill Workshop Ltd.

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
Based on the task switching granularity of the GPU, and on how the rendering
commands are prioritized and serialized, the asynchronous time warp may, or may
not be able to stay synchronized with the display refresh.

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

The left-most bar graph represent the frame rate of the stereoscopic rendering (pink).
The next bar graph represents the frame rate of time warp rendering (green). Each bar
that is pink or green respectively reaches the top of the graph and represents a frame
rendered at the display refresh rate. When the frame rate drops, the bars turn red
and become shorter proportional to how much the frame rate drops.

The next two bar graphs shows the CPU and GPU time of the stereoscopic rendering (pink),
the time warp rendering (green) and the bar graph rendering (yellow). The times are
stacked in each graph. The full height of a graph represents a full frame time.
For instance, with a 60Hz display refresh rate, the full graph height represents 16.7
milliseconds.


RESOLUTIONS
===========

The rendering resolutions can be changed by adjusting the display resolution, the
eye image resolution, and the eye image MSAA. For each of these there are four levels:

Display Resolution:
	0: 1920 x 1080
	1: 2560 x 1440
	2: 3840 x 2160
	3: 7680 x 4320

Eye image resolution:
	0: 1024 x 1024
	1: 1536 x 1536
	2: 2048 x 2048
	3: 4096 x 4096

Eye image multi-sampling:
	0: 1x MSAA
	1: 2x MSAA
	2: 4x MSAA
	3: 8x MSAA


SCENE WORKLOAD
==============

The graphics work load of the scene that is rendered for each eye can be changed by
adjusting the number of draw calls, the number of triangles per draw call, the fragment
program complexity and the number of samples. For each of these there are four levels:

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

In the lower right corner of the screen there are four indicators that show
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

	-a <.json>	load glTF scene
	-f			start fullscreen
	-v <s>		start with V-Sync disabled for this many seconds
	-h			start with head rotation disabled
	-p			start with the simulation paused
	-r <0-3>	set display resolution level
	-b <0-3>	set eye image resolution level
	-s <0-3>	set eye image multi-sampling level
	-q <0-3>	set per eye draw calls level
	-w <0-3>	set per eye triangles per draw call level
	-e <0-3>	set per eye fragment program complexity level
	-m <0-1>	enable/disable multi-view
	-c <0-1>	enable/disable correction for chromatic aberration
	-i <name>	set time warp implementation: graphics, compute
	-z <name>	set the render mode: atw, tw, scene
	-g			hide graphs
	-l <s>		log 10 frames of Vulkan commands after this many seconds
	-d			dump GLSL to files for conversion to SPIR-V


KEYBOARD INPUT
==============

The following keys can be used at run-time to change various settings.

	[F]		= toggle between windowed and fullscreen
	[V]		= toggle V-Sync on/off
	[H]		= toggle head rotation on/off
	[P]		= pause/resume the simulation
	[R]		= cycle screen resolution level
	[B]		= cycle eye buffer resolution level
	[S]		= cycle multi-sampling level
	[Q]		= cycle per eye draw calls level
	[W]		= cycle per eye triangles per draw call level
	[E]		= cycle per eye fragment program complexity level
	[M]		= toggle multi-view
	[C]		= toggle correction for chromatic aberration
	[I]		= toggle time warp implementation: graphics, compute
	[Z]		= cycle the render mode: atw, tw, scene
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
	- Ubuntu Linux 14.04 or later
	- Apple macOS 10.11 or later
	- Apple iOS 9.0 or later
	- Android 5.0 or later


GRAPHICS API WRAPPER
====================

The code wraps the Vulkan API with a convenient wrapper that takes care of all
the intricacies of memory synchronization and memory barriers. This wrapper
does not expose the full Vulkan API but can be easily extended to support more
features. Some of the current limitations are:

- The wrapper is setup for forward rendering with a single render pass. This
  can be easily extended if more complex rendering algorithms are desired.

- A pipeline can only use 256 bytes worth of plain integer and floating-point
  uniforms, including vectors and matrices. If more uniforms are needed then 
  it is advised to use a uniform buffer, which is the preferred approach for
  exposing large amounts of data anyway.

- Graphics programs currently consist of only a vertex and fragment shader.
  This can be easily extended if there is a need for geometry shaders etc.


COMMAND-LINE COMPILATION
========================

Microsoft Windows: Visual Studio 2013 Compiler:
	"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
	cl /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /I%VK_SDK_PATH%\Include atw_vulkan.c /link user32.lib gdi32.lib Advapi32.lib Shlwapi.lib

Microsoft Windows: Intel Compiler 14.0
	"C:\Program Files (x86)\Intel\Composer XE\bin\iclvars.bat" intel64
	icl /Qstd=c99 /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /I%VK_SDK_PATH%\Include atw_vulkan.c /link user32.lib gdi32.lib Advapi32.lib Shlwapi.lib

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

Apple macOS: Apple LLVM 6.0:
	clang -std=c99 -x objective-c -fno-objc-arc -Wall -g -O2 -m64 -o atw_vulkan atw_vulkan.c -framework Cocoa

Android for ARM from Windows: NDK Revision 11c - Android 21 - ANT/Gradle
	ANT:
		cd projects/android/ant/atw_vulkan
		ndk-build
		ant debug
		adb install -r bin/atw_vulkan-debug.apk
	Gradle:
		cd projects/android/gradle/atw_vulkan
		gradlew build
		adb install -r build/outputs/apk/atw_vulkan-all-debug.apk


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
	#define OS_APPLE
	#include <Availability.h>
	#if __IPHONE_OS_VERSION_MAX_ALLOWED
		#define OS_APPLE_IOS
	#elif __MAC_OS_X_VERSION_MAX_ALLOWED
		#define OS_APPLE_MACOS
	#endif
#elif defined( __linux__ )
	#define OS_LINUX
	#if defined( SUPPORT_X )
		//#define OS_LINUX_XLIB
		#define OS_LINUX_XCB
	#endif
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
		#pragma warning( disable : 4201 )	// nonstandard extension used: nameless struct/union
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

	#include <windows.h>

	#define VK_USE_PLATFORM_WIN32_KHR
	#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME	VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	#define PFN_vkCreateSurfaceKHR					PFN_vkCreateWin32SurfaceKHR
	#define vkCreateSurfaceKHR						vkCreateWin32SurfaceKHR

	#include "vulkan/vulkan.h"
	#include "vulkan/vk_sdk_platform.h"
	#include "vulkan/vk_format.h"

	#define VULKAN_LOADER	"vulkan-1.dll"
	#define OUTPUT_PATH		""

	#define __thread	__declspec( thread )

#elif defined( OS_LINUX )

	#if __STDC_VERSION__ >= 199901L
	#define _XOPEN_SOURCE 600
	#else
	#define _XOPEN_SOURCE 500
	#endif

	#include <time.h>							// for timespec
	#include <sys/time.h>						// for gettimeofday()
	#if !defined( __USE_UNIX98 )
		#define __USE_UNIX98	1				// for pthread_mutexattr_settype
	#endif
	#include <pthread.h>						// for pthread_create() etc.
	#include <malloc.h>							// for memalign
	#include <dlfcn.h>							// for dlopen
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
	#include "vulkan/vk_format.h"

	#define VULKAN_LOADER	"libvulkan-1.so"
	#define OUTPUT_PATH		""

	// These prototypes are only included when __USE_GNU is defined but that causes other compile errors.
	extern int pthread_setname_np( pthread_t __target_thread, __const char *__name );
	extern int pthread_setaffinity_np( pthread_t thread, size_t cpusetsize, const cpu_set_t * cpuset );

	#pragma GCC diagnostic ignored "-Wunused-function"

#elif defined( OS_APPLE )

	#include <sys/param.h>
	#include <sys/sysctl.h>
	#include <sys/time.h>
	#include <pthread.h>
	#include <dlfcn.h>						// for dlopen

	#include "vulkan/vulkan.h"
	#include "vulkan/vk_format.h"

#if !(defined( VK_USE_PLATFORM_IOS_MVK ) || defined( VK_USE_PLATFORM_MACOS_MVK ))
	#include "vulkan/vk_sdk_platform.h"
#endif

	#if defined( OS_APPLE_IOS )
		#include <UIKit/UIKit.h>
		#if defined( VK_USE_PLATFORM_IOS_MVK )
			#include <QuartzCore/CAMetalLayer.h>
			#include <MoltenVK/vk_mvk_moltenvk.h>
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
			#define VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_KHR	1000122000
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
			#define VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_KHR	1000123000
			#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME			"VK_KHR_macos_surface"
			typedef VkResult (VKAPI_PTR *PFN_vkCreateMacOSSurfaceKHR)(VkInstance instance, const VkMacOSSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
			#define PFN_vkCreateSurfaceKHR							PFN_vkCreateMacOSSurfaceKHR
			#define vkCreateSurfaceKHR								vkCreateMacOSSurfaceKHR
			#define VULKAN_LOADER									"libvulkan.dylib"
		#endif
	#endif

	#undef MAX
	#undef MIN
	#define OUTPUT_PATH		""

	#pragma clang diagnostic ignored "-Wunused-function"
	#pragma clang diagnostic ignored "-Wunused-const-variable"

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
	#include "vulkan/vk_format.h"

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

#if defined( OS_NEUTRAL_DISPLAY_SURFACE )
	#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME	VK_KHR_DISPLAY_EXTENSION_NAME
	#define PFN_vkCreateSurfaceKHR					PFN_vkCreateDisplayPlaneSurfaceKHR
	#define vkCreateSurfaceKHR						vkCreateDisplayPlaneSurfaceKHR
#endif

#define GRAPHICS_API_VULKAN				1

/*
================================
Common headers
================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>			// for memset
#include <errno.h>			// for EBUSY, ETIMEDOUT etc.
#include <ctype.h>			// for isspace, isdigit

#include <utils/sysinfo.h>
#include <utils/nanoseconds.h>
#include <utils/threading.h>
#include <utils/algebra.h>

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
#define MAX( x, y )						( ( x > y ) ? ( x ) : ( y ) )
#define MIN( x, y )						( ( x < y ) ? ( x ) : ( y ) )
#define CLAMP( x, min, max )			( ( (x) < (min) ) ? (min) : ( ( (x) > (max) ) ? (max) : (x) ) )
#define STRINGIFY_EXPANDED( a )			#a
#define STRINGIFY( a )					STRINGIFY_EXPANDED(a)

#define APPLICATION_NAME				"Vulkan ATW"
#define WINDOW_TITLE					"Asynchronous Time Warp - Vulkan"

#define VK_ALLOCATOR					NULL

#define USE_VALIDATION					0
#define USE_SPIRV						1
#define USE_PM_MULTIVIEW				1
#define USE_API_DUMP					0	// place vk_layer_settings.txt in the executable folder and change APIDumpFile = TRUE

#define ICD_SPV_MAGIC					0x07230203

#if USE_SPIRV == 1
	#define PROGRAM( name )				name##SPIRV
#else
	#define PROGRAM( name )				name##GLSL
#endif

#define SPIRV_VERSION					"99"
#define GLSL_VERSION					"440 core"	// maintain precision decorations: "310 es"
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
#elif defined( OS_APPLE )
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

	OutputDebugStringA( buffer );
#elif defined( OS_LINUX )
	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );
	fflush( stdout );
#elif defined( OS_APPLE )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	vsnprintf( buffer, 4096, format, args );
	va_end( args );

	NSLog( @"%s", buffer );
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

	OutputDebugStringA( buffer );

	MessageBoxA( NULL, buffer, "ERROR", MB_OK | MB_ICONINFORMATION );
#elif defined( OS_LINUX )
	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );
	printf( "\n" );
	fflush( stdout );
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

/*
================================================================================================================================

String Hash.

ksStringHash

================================================================================================================================
*/

typedef uint32_t ksStringHash;

void ksStringHash_Init( ksStringHash * hash )
{
	*hash = 5381;
}

void ksStringHash_Update( ksStringHash * hash, const char * string )
{
	ksStringHash value = *hash;
	for ( int i = 0; string[i] != '\0'; i++ )
	{
		value = ( ( value << 5 ) - value ) + string[i];
	}
	*hash = value;
}

/*
================================================================================================================================

Frame logging.

Each thread that calls ksFrameLog_Open will open its own log.
A frame log is always opened for a specified number of frames, and will
automatically close after the specified number of frames have been recorded.
The CPU and GPU times for the recorded frames will be listed at the end of the log.

ksFrameLog

static void ksFrameLog_Open( const char * fileName, const int frameCount );
static void ksFrameLog_Write( const char * fileName, const int lineNumber, const char * function );
static void ksFrameLog_BeginFrame();
static void ksFrameLog_EndFrame( const ksNanoseconds cpuTimeNanoseconds, const ksNanoseconds gpuTimeNanoseconds, const int gpuTimeFramesDelayed );

================================================================================================================================
*/

typedef struct
{
	FILE *			fp;
	ksNanoseconds *	frameCpuTimes;
	ksNanoseconds *	frameGpuTimes;
	int				frameCount;
	int				frame;
} ksFrameLog;

__thread ksFrameLog * threadFrameLog;

static ksFrameLog * ksFrameLog_Get()
{
	ksFrameLog * l = threadFrameLog;
	if ( l == NULL )
	{
		l = (ksFrameLog *) malloc( sizeof( ksFrameLog ) );
		memset( l, 0, sizeof( ksFrameLog ) );
		threadFrameLog = l;
	}
	return l;
}

static void ksFrameLog_Open( const char * fileName, const int frameCount )
{
	ksFrameLog * l = ksFrameLog_Get();
	if ( l != NULL && l->fp == NULL )
	{
		l->fp = fopen( fileName, "wb" );
		if ( l->fp == NULL )
		{
			Print( "Failed to open %s\n", fileName );
		}
		else
		{
			Print( "Opened frame log %s for %d frames.\n", fileName, frameCount );
			l->frameCpuTimes = (ksNanoseconds *) malloc( frameCount * sizeof( l->frameCpuTimes[0] ) );
			l->frameGpuTimes = (ksNanoseconds *) malloc( frameCount * sizeof( l->frameGpuTimes[0] ) );
			memset( l->frameCpuTimes, 0, frameCount * sizeof( l->frameCpuTimes[0] ) );
			memset( l->frameGpuTimes, 0, frameCount * sizeof( l->frameGpuTimes[0] ) );
			l->frameCount = frameCount;
			l->frame = 0;
		}
	}
}

static void ksFrameLog_Write( const char * fileName, const int lineNumber, const char * function )
{
	ksFrameLog * l = ksFrameLog_Get();
	if ( l != NULL && l->fp != NULL )
	{
		if ( l->frame < l->frameCount )
		{
			fprintf( l->fp, "%s(%d): %s\r\n", fileName, lineNumber, function );
		}
	}
}

static void ksFrameLog_BeginFrame()
{
	ksFrameLog * l = ksFrameLog_Get();
	if ( l != NULL && l->fp != NULL )
	{
		if ( l->frame < l->frameCount )
		{
#if defined( _DEBUG )
			fprintf( l->fp, "================ BEGIN FRAME %d ================\r\n", l->frame );
#endif
		}
	}
}

static void ksFrameLog_EndFrame( const ksNanoseconds cpuTimeNanoseconds, const ksNanoseconds gpuTimeNanoseconds, const int gpuTimeFramesDelayed )
{
	ksFrameLog * l = ksFrameLog_Get();
	if ( l != NULL && l->fp != NULL )
	{
		if ( l->frame < l->frameCount )
		{
			l->frameCpuTimes[l->frame] = cpuTimeNanoseconds;
#if defined( _DEBUG )
			fprintf( l->fp, "================ END FRAME %d ================\r\n", l->frame );
#endif
		}
		if ( l->frame >= gpuTimeFramesDelayed && l->frame < l->frameCount + gpuTimeFramesDelayed )
		{
			l->frameGpuTimes[l->frame - gpuTimeFramesDelayed] = gpuTimeNanoseconds;
		}

		l->frame++;

		if ( l->frame >= l->frameCount + gpuTimeFramesDelayed )
		{
			for ( int i = 0; i < l->frameCount; i++ )
			{
				fprintf( l->fp, "frame %d: CPU = %1.1f ms, GPU = %1.1f ms\r\n", i, l->frameCpuTimes[i] * 1e-6f, l->frameGpuTimes[i] * 1e-6f );
			}

			Print( "Closing frame log file (%d frames).\n", l->frameCount );
			fclose( l->fp );
			free( l->frameCpuTimes );
			free( l->frameGpuTimes );
			memset( l, 0, sizeof( ksFrameLog ) );
		}
	}
}

/*
================================================================================================================================

Rectangles.

ksScreenRect
ksClipRect

ksScreenRect is specified in pixels with 0,0 at the left-bottom.
ksClipRect is specified in clip space in the range [-1,1], with -1,-1 at the left-bottom.

static ksClipRect ksScreenRect_ToClipRect( const ksScreenRect * screenRect, const int resolutionX, const int resolutionY );
static ksScreenRect ksClipRect_ToScreenRect( const ksClipRect * clipRect, const int resolutionX, const int resolutionY );

================================================================================================================================
*/

typedef struct
{
	int x;
	int y;
	int width;
	int height;
} ksScreenRect;

typedef struct
{
	float x;
	float y;
	float width;
	float height;
} ksClipRect;

static ksClipRect ksScreenRect_ToClipRect( const ksScreenRect * screenRect, const int resolutionX, const int resolutionY )
{
	ksClipRect clipRect;
	clipRect.x = 2.0f * screenRect->x / resolutionX - 1.0f;
	clipRect.y = 2.0f * screenRect->y / resolutionY - 1.0f;
	clipRect.width = 2.0f * screenRect->width / resolutionX;
	clipRect.height = 2.0f * screenRect->height / resolutionY;
	return clipRect;
}

static ksScreenRect ksClipRect_ToScreenRect( const ksClipRect * clipRect, const int resolutionX, const int resolutionY )
{
	ksScreenRect screenRect;
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
	#define VK( func )		VkCheckErrors( func, #func ); ksFrameLog_Write( __FILE__, __LINE__, #func );
	#define VC( func )		func; ksFrameLog_Write( __FILE__, __LINE__, #func );
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

ksDriverInstance

static bool ksDriverInstance_Create( ksDriverInstance * intance );
static void ksDriverInstance_Destroy( ksDriverInstance * instance );

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

#if defined( OS_NEUTRAL_DISPLAY_SURFACE )
	PFN_vkGetPhysicalDeviceDisplayPropertiesKHR			vkGetPhysicalDeviceDisplayPropertiesKHR;
	PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR	vkGetPhysicalDeviceDisplayPlanePropertiesKHR;
	PFN_vkGetDisplayPlaneSupportedDisplaysKHR			vkGetDisplayPlaneSupportedDisplaysKHR;
	PFN_vkGetDisplayModePropertiesKHR					vkGetDisplayModePropertiesKHR;
#endif

	// Debug callback.
	PFN_vkCreateDebugReportCallbackEXT					vkCreateDebugReportCallbackEXT;
	PFN_vkDestroyDebugReportCallbackEXT					vkDestroyDebugReportCallbackEXT;
	VkDebugReportCallbackEXT							debugReportCallback;
} ksDriverInstance;

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

	// [DS] "Cannot get query results on queryPool 0x1b3 with index 4 which is in flight."
	if ( MatchStrings( pMsg, "Cannot get query results on queryPool 0x1b3 with index 4 which is in flight." ) )
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
} ksDriverFeature;

// Returns true if all the required features are present.
static bool CheckFeatures( const char * label, const bool validationEnabled, const bool extensions,
							const ksDriverFeature * requested, const uint32_t requestedCount,
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

static bool ksDriverInstance_Create( ksDriverInstance * instance )
{
	memset( instance, 0, sizeof( ksDriverInstance ) );

#if defined( _DEBUG ) && USE_VALIDATION == 1
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
#elif defined( OS_LINUX ) || defined( OS_ANDROID ) || ( defined( OS_APPLE ) && !defined( VK_USE_PLATFORM_IOS_MVK ) && !defined( VK_USE_PLATFORM_MACOS_MVK ) )
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
	const ksDriverFeature requestedExtensions[] =
	{
		{ VK_KHR_SURFACE_EXTENSION_NAME,			false, true },
#if defined( OS_NEUTRAL_DISPLAY_SURFACE )
		{ VK_KHR_DISPLAY_EXTENSION_NAME,			false, true },
#endif
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
	const ksDriverFeature requestedLayers[] =
	{
		{ "VK_LAYER_OCULUS_glsl_shader",			false, false },
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

#if defined( OS_NEUTRAL_DISPLAY_SURFACE )
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceDisplayPropertiesKHR )
	GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceDisplayPlanePropertiesKHR )
	GET_INSTANCE_PROC_ADDR( vkGetDisplayPlaneSupportedDisplaysKHR )
	GET_INSTANCE_PROC_ADDR( vkGetDisplayModePropertiesKHR )
#endif

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
			debugReportCallbackCreateInfo.flags =
					VK_DEBUG_REPORT_ERROR_BIT_EXT |
					VK_DEBUG_REPORT_WARNING_BIT_EXT |
					VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
					//VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
					//VK_DEBUG_REPORT_DEBUG_BIT_EXT;
			debugReportCallbackCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)DebugReportCallback;
			debugReportCallbackCreateInfo.pUserData = NULL;

			VK( instance->vkCreateDebugReportCallbackEXT( instance->instance, &debugReportCallbackCreateInfo, VK_ALLOCATOR, &instance->debugReportCallback ) );
		}
	}

	return true;
}

static void ksDriverInstance_Destroy( ksDriverInstance * instance )
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

ksGpuQueueProperty
ksGpuQueuePriority
ksGpuQueueInfo
ksGpuDevice

static bool ksGpuDevice_SelectPhysicalDevice( ksGpuDevice * device, ksDriverInstance * instance,
									const ksGpuQueueInfo * queueInfo, const VkSurfaceKHR presentSurface );
static bool ksGpuDevice_Create( ksGpuDevice * device, ksDriverInstance * instance, const ksGpuQueueInfo * queueInfo );
static void ksGpuDevice_Destroy( ksGpuDevice * device );

================================================================================================================================
*/

typedef enum
{
	KS_GPU_QUEUE_PROPERTY_GRAPHICS		= BIT( 0 ),
	KS_GPU_QUEUE_PROPERTY_COMPUTE		= BIT( 1 ),
	KS_GPU_QUEUE_PROPERTY_TRANSFER		= BIT( 2 )
} ksGpuQueueProperty;

typedef enum
{
	KS_GPU_QUEUE_PRIORITY_LOW,
	KS_GPU_QUEUE_PRIORITY_MEDIUM,
	KS_GPU_QUEUE_PRIORITY_HIGH
} ksGpuQueuePriority;

#define MAX_QUEUES	16

typedef struct
{
	int					queueCount;						// number of queues
	ksGpuQueueProperty	queueProperties;				// desired queue family properties
	ksGpuQueuePriority	queuePriorities[MAX_QUEUES];	// individual queue priorities
} ksGpuQueueInfo;

typedef struct
{
	VkBool32								foundSwapchainExtension;
	ksDriverInstance *						instance;
	uint32_t								enabledExtensionCount;
	const char *							enabledExtensionNames[32];
	uint32_t								enabledLayerCount;
	const char *							enabledLayerNames[32];
	VkPhysicalDevice						physicalDevice;
	VkPhysicalDeviceFeatures				physicalDeviceFeatures;
	VkPhysicalDeviceProperties				physicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties		physicalDeviceMemoryProperties;
	uint32_t								queueFamilyCount;
	VkQueueFamilyProperties *				queueFamilyProperties;
	uint32_t *								queueFamilyUsedQueues;
	ksMutex									queueFamilyMutex;
	int										workQueueFamilyIndex;
	int										presentQueueFamilyIndex;

	// The logical device.
	VkDevice								device;

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
} ksGpuDevice;

#define GET_DEVICE_PROC_ADDR_EXP( function )	device->function = (PFN_##function)( device->instance->vkGetDeviceProcAddr( device->device, #function ) ); assert( device->function != NULL );
#define GET_DEVICE_PROC_ADDR( function )		GET_DEVICE_PROC_ADDR_EXP( function )

static bool ksGpuDevice_SelectPhysicalDevice( ksGpuDevice * device, ksDriverInstance * instance,
											const ksGpuQueueInfo * queueInfo, const VkSurfaceKHR presentSurface )
{
	memset( device, 0, sizeof( ksGpuDevice ) );

	device->instance = instance;

	//
	// Select an appropriate physical device
	//

	const VkQueueFlags requiredQueueFlags =
		( ( ( queueInfo->queueProperties & KS_GPU_QUEUE_PROPERTY_GRAPHICS ) != 0 ) ? VK_QUEUE_GRAPHICS_BIT : 0 ) |
		( ( ( queueInfo->queueProperties & KS_GPU_QUEUE_PROPERTY_COMPUTE ) != 0 ) ? VK_QUEUE_COMPUTE_BIT : 0 ) |
		( ( ( queueInfo->queueProperties & KS_GPU_QUEUE_PROPERTY_TRANSFER ) != 0 &&
			( queueInfo->queueProperties & ( KS_GPU_QUEUE_PROPERTY_GRAPHICS | KS_GPU_QUEUE_PROPERTY_COMPUTE ) ) == 0 ) ? VK_QUEUE_TRANSFER_BIT : 0 );

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

		const ksDriverFeature requestedExtensions[] =
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
					device->enabledExtensionNames, &device->enabledExtensionCount );

			free( availableExtensions );
		}

		if ( !requiedExtensionsAvailable )
		{
			Print( "Required device extensions not supported.\n" );
			free( queueFamilyProperties );
			continue;
		}

		// Check the device layers.
		const ksDriverFeature requestedLayers[] =
		{
			{ "VK_LAYER_OCULUS_glsl_shader",			false, false },
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
					device->enabledLayerNames, &device->enabledLayerCount );

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

	ksMutex_Create( &device->queueFamilyMutex );

	return true;
}

static bool ksGpuDevice_Create( ksGpuDevice * device, ksDriverInstance * instance, const ksGpuQueueInfo * queueInfo )
{
	//
	// Create the logical device
	//

	float floatPriorities[MAX_QUEUES];
	for ( int i = 0; i < queueInfo->queueCount; i++ )
	{
		const uint32_t discreteQueuePriorities = device->physicalDeviceProperties.limits.discreteQueuePriorities;
		switch ( queueInfo->queuePriorities[i] )
		{
			case KS_GPU_QUEUE_PRIORITY_LOW:		floatPriorities[i] = 0.0f; break;
			case KS_GPU_QUEUE_PRIORITY_MEDIUM:	floatPriorities[i] = ( discreteQueuePriorities <= 2 ) ? 0.0f : 0.5f; break;
			case KS_GPU_QUEUE_PRIORITY_HIGH:	floatPriorities[i] = 1.0f; break;
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
	deviceCreateInfo.enabledLayerCount = device->enabledLayerCount;
	deviceCreateInfo.ppEnabledLayerNames = (const char * const *) ( device->enabledLayerCount != 0 ? device->enabledLayerNames : NULL );
	deviceCreateInfo.enabledExtensionCount = device->enabledExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = (const char * const *) ( device->enabledExtensionCount != 0 ? device->enabledExtensionNames : NULL );
	deviceCreateInfo.pEnabledFeatures = NULL;

	VK( instance->vkCreateDevice( device->physicalDevice, &deviceCreateInfo, VK_ALLOCATOR, &device->device ) );

#if defined( VK_USE_PLATFORM_IOS_MVK ) || defined( VK_USE_PLATFORM_MACOS_MVK )
	// Specify some helpful MoltenVK extension configuration, such as performance logging and shader conversion logging.
	MVKDeviceConfiguration mvkConfig;
	vkGetMoltenVKDeviceConfigurationMVK( device->device, &mvkConfig );
	mvkConfig.performanceTracking = true;
	mvkConfig.performanceLoggingFrameCount = 300;	// Log once every 5 seconds (300 frames)
#ifdef DEBUG
	mvkConfig.debugMode = true;
#endif
	vkSetMoltenVKDeviceConfigurationMVK( device->device, &mvkConfig );
#endif

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

static void ksGpuDevice_Destroy( ksGpuDevice * device )
{
	VK( device->vkDeviceWaitIdle( device->device ) );

	free( device->queueFamilyProperties );
	free( device->queueFamilyUsedQueues );

	ksMutex_Destroy( &device->queueFamilyMutex );

	VC( device->vkDestroyDevice( device->device, VK_ALLOCATOR ) );
}

static uint32_t ksGpuDevice_GetMemoryTypeIndex( ksGpuDevice * device, const uint32_t typeBits,
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

static void ksGpuDevice_CreateShader( ksGpuDevice * device, VkShaderModule * shaderModule,
									const VkShaderStageFlagBits stage, const void * code, size_t codeSize )
{
	UNUSED_PARM( stage );

	VkShaderModuleCreateInfo moduleCreateInfo;
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.pNext = NULL;
	moduleCreateInfo.flags = 0;
	moduleCreateInfo.codeSize = 0;
	moduleCreateInfo.pCode = NULL;


	if ( *(uint32_t *)code == ICD_SPV_MAGIC )
	{
		moduleCreateInfo.codeSize = codeSize;
		moduleCreateInfo.pCode = code;

		VK( device->vkCreateShaderModule( device->device, &moduleCreateInfo, VK_ALLOCATOR, shaderModule ) );
	}
	else
	{
		// Create fake SPV structure to feed GLSL to the driver "under the covers".
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
	}
}

/*
================================================================================================================================

GPU context.

A context encapsulates a queue that is used to submit command buffers.
A context can only be used by a single thread.
For optimal performance a context should only be created at load time, not at runtime.

ksGpuContext
ksGpuSurfaceColorFormat
ksGpuSurfaceDepthFormat
ksGpuSampleCount

static bool ksGpuContext_CreateShared( ksGpuContext * context, const ksGpuContext * other, const int queueIndex );
static void ksGpuContext_Destroy( ksGpuContext * context );
static void ksGpuContext_WaitIdle( ksGpuContext * context );
static void ksGpuContext_GetLimits( ksGpuContext * context, ksGpuLimits * limits );

static bool ksGpuContext_Create( ksGpuContext * context, ksGpuDevice * device, const int queueIndex );

================================================================================================================================
*/

typedef enum
{
	KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5,
	KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5,
	KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8,
	KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8,
	KS_GPU_SURFACE_COLOR_FORMAT_MAX
} ksGpuSurfaceColorFormat;

typedef enum
{
	KS_GPU_SURFACE_DEPTH_FORMAT_NONE,
	KS_GPU_SURFACE_DEPTH_FORMAT_D16,
	KS_GPU_SURFACE_DEPTH_FORMAT_D24,
	KS_GPU_SURFACE_DEPTH_FORMAT_MAX
} ksGpuSurfaceDepthFormat;

typedef enum
{
	KS_GPU_SAMPLE_COUNT_1		= VK_SAMPLE_COUNT_1_BIT,
	KS_GPU_SAMPLE_COUNT_2		= VK_SAMPLE_COUNT_2_BIT,
	KS_GPU_SAMPLE_COUNT_4		= VK_SAMPLE_COUNT_4_BIT,
	KS_GPU_SAMPLE_COUNT_8		= VK_SAMPLE_COUNT_8_BIT,
	KS_GPU_SAMPLE_COUNT_16		= VK_SAMPLE_COUNT_16_BIT,
	KS_GPU_SAMPLE_COUNT_32		= VK_SAMPLE_COUNT_32_BIT,
	KS_GPU_SAMPLE_COUNT_64		= VK_SAMPLE_COUNT_64_BIT,
} ksGpuSampleCount;

typedef struct ksGpuLimits
{
	size_t					maxPushConstantsSize;
	int						maxSamples;
} ksGpuLimits;

typedef struct
{
	ksGpuDevice *	device;
	uint32_t		queueFamilyIndex;
	uint32_t		queueIndex;
	VkQueue			queue;
	VkCommandPool	commandPool;
	VkPipelineCache	pipelineCache;
	VkCommandBuffer	setupCommandBuffer;
} ksGpuContext;

static bool ksGpuContext_Create( ksGpuContext * context, ksGpuDevice * device, const int queueIndex )
{
	memset( context, 0, sizeof( ksGpuContext ) );

	ksMutex_Lock( &device->queueFamilyMutex, true );
	assert( ( device->queueFamilyUsedQueues[device->workQueueFamilyIndex] & ( 1 << queueIndex ) ) == 0 );
	device->queueFamilyUsedQueues[device->workQueueFamilyIndex] |= ( 1 << queueIndex );
	ksMutex_Unlock( &device->queueFamilyMutex );

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

static bool ksGpuContext_CreateShared( ksGpuContext * context, const ksGpuContext * other, const int queueIndex )
{
	return ksGpuContext_Create( context, other->device, queueIndex );
}

static void ksGpuContext_Destroy( ksGpuContext * context )
{
	if ( context->device == NULL )
	{
		return;
	}

	// Mark the queue as no longer in use.
	ksMutex_Lock( &context->device->queueFamilyMutex, true );
	assert( ( context->device->queueFamilyUsedQueues[context->queueFamilyIndex] & ( 1 << context->queueIndex ) ) != 0 );
	context->device->queueFamilyUsedQueues[context->queueFamilyIndex] &= ~( 1 << context->queueIndex );
	ksMutex_Unlock( &context->device->queueFamilyMutex );

	if ( context->setupCommandBuffer )
	{
		VC( context->device->vkFreeCommandBuffers( context->device->device, context->commandPool, 1, &context->setupCommandBuffer ) );
	}
	VC( context->device->vkDestroyCommandPool( context->device->device, context->commandPool, VK_ALLOCATOR ) );
	VC( context->device->vkDestroyPipelineCache( context->device->device, context->pipelineCache, VK_ALLOCATOR ) );
}

static void ksGpuContext_WaitIdle( ksGpuContext * context )
{
	VK( context->device->vkQueueWaitIdle( context->queue ) );
}

static void ksGpuContext_GetLimits( ksGpuContext * context, ksGpuLimits * limits )
{
	limits->maxPushConstantsSize = context->device->physicalDeviceProperties.limits.maxPushConstantsSize;
	const VkSampleCountFlags availableSampleCounts = context->device->physicalDeviceProperties.limits.framebufferColorSampleCounts &
														context->device->physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	limits->maxSamples = 0;
	for ( int bit = VK_SAMPLE_COUNT_1_BIT; bit <= VK_SAMPLE_COUNT_64_BIT; bit <<= 1 )
	{
		if ( ( availableSampleCounts & bit ) == 0 )
		{
			break;
		}
		limits->maxSamples = bit;
	}
}

static void ksGpuContext_CreateSetupCmdBuffer( ksGpuContext * context )
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

static void ksGpuContext_FlushSetupCmdBuffer( ksGpuContext * context )
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
For optimal performance a swapchain should only be created at load time, not at runtime.
Swapchain images are never multi-sampled.
For multi-sampled rendering, a regular multi-sampled texture is resolved to a swapchain image

ksGpuSwapchain

static bool ksGpuSwapchain_Create( ksGpuContext * context, ksGpuSwapchain * swapchain, const VkSurfaceKHR surface,
								const ksGpuSurfaceColorFormat colorFormat, const int width, const int height, const int swapInterval );
static void ksGpuSwapchain_Destroy( ksGpuContext * context, ksGpuSwapchain * swapchain );
static ksNanoseconds ksGpuSwapchain_SwapBuffers( ksGpuContext * context, ksGpuSwapchain * swapchain );

================================================================================================================================
*/

typedef struct
{
	uint32_t		imageIndex;
	VkSemaphore		presentCompleteSemaphore;
	VkSemaphore		renderingCompleteSemaphore;
} ksGpuSwapchainBuffer;

typedef struct
{
	ksGpuSurfaceColorFormat	format;
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
	ksGpuSwapchainBuffer *	buffers;
} ksGpuSwapchain;

static VkFormat ksGpuSwapchain_InternalSurfaceColorFormat( const ksGpuSurfaceColorFormat colorFormat )
{
	return	( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 ) ? VK_FORMAT_R8G8B8A8_UNORM :
			( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 ) ? VK_FORMAT_B8G8R8A8_UNORM :
			( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5 ) ? VK_FORMAT_R5G6B5_UNORM_PACK16 :
			( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5 ) ? VK_FORMAT_B5G6R5_UNORM_PACK16 :
			( ( VK_FORMAT_UNDEFINED ) ) ) ) ) );
}

static bool ksGpuSwapchain_Create( ksGpuContext * context, ksGpuSwapchain * swapchain, const VkSurfaceKHR surface,
								const ksGpuSurfaceColorFormat colorFormat, const int width, const int height, const int swapInterval )
{
	memset( swapchain, 0, sizeof( ksGpuSwapchain ) );

	ksGpuDevice * device = context->device;

	if ( !device->foundSwapchainExtension )
	{
		return false;
	}

	// Get the list of formats that are supported.
	uint32_t formatCount;
	VK( device->instance->vkGetPhysicalDeviceSurfaceFormatsKHR( device->physicalDevice, surface, &formatCount, NULL ) );

	VkSurfaceFormatKHR * surfaceFormats = (VkSurfaceFormatKHR *)malloc( formatCount * sizeof( VkSurfaceFormatKHR ) );
	VK( device->instance->vkGetPhysicalDeviceSurfaceFormatsKHR( device->physicalDevice, surface, &formatCount, surfaceFormats ) );

	const ksGpuSurfaceColorFormat desiredFormatTable[KS_GPU_SURFACE_COLOR_FORMAT_MAX][KS_GPU_SURFACE_COLOR_FORMAT_MAX] =
	{
		{ KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5, KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5, KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 },
		{ KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5, KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5, KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8, KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 },
		{ KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8, KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5, KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5 },
		{ KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8, KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5, KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5 }
	};
	assert( KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5 == 0 );
	assert( KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5 == 1 );
	assert( KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 == 2 );
	assert( KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 == 3 );
	assert( colorFormat >= 0 && colorFormat < KS_GPU_SURFACE_COLOR_FORMAT_MAX );

	const ksGpuSurfaceColorFormat * desiredFormat = desiredFormatTable[colorFormat];
	const VkColorSpaceKHR desiredColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

	// If the format list includes just one entry of VK_FORMAT_UNDEFINED, then the surface has no preferred format.
	// Otherwise, at least one supported format will be returned.
	if ( formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED )
	{
		swapchain->format = colorFormat;
		swapchain->internalFormat = ksGpuSwapchain_InternalSurfaceColorFormat( desiredFormat[0] );
		swapchain->colorSpace = desiredColorSpace;
	}
	else
	{
		// Select the best matching surface format.
		assert( formatCount >= 1 );
		for ( uint32_t desired = 0; desired < KS_GPU_SURFACE_COLOR_FORMAT_MAX; desired++ )
		{
			const VkFormat internalFormat = ksGpuSwapchain_InternalSurfaceColorFormat( desiredFormat[desired] );
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
	swapchain->buffers = (ksGpuSwapchainBuffer *) malloc( swapchain->bufferCount * sizeof( ksGpuSwapchainBuffer ) );
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

static void ksGpuSwapchain_Destroy( ksGpuContext * context, ksGpuSwapchain * swapchain )
{
	ksGpuDevice * device = context->device;

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

	memset( swapchain, 0, sizeof( ksGpuSwapchain ) );
}

static ksNanoseconds ksGpuSwapchain_SwapBuffers( ksGpuContext * context, ksGpuSwapchain * swapchain )
{
	ksGpuDevice * device = context->device;

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

	const ksNanoseconds swapTime = GetTimeNanoseconds();

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
For optimal performance a depth buffer should only be created at load time, not at runtime.

ksGpuDepthBuffer

static void ksGpuDepthBuffer_Create( ksGpuContext * context, ksGpuDepthBuffer * depthBuffer,
									const ksGpuSurfaceDepthFormat depthFormat, const ksGpuSampleCount sampleCount,
									const int width, const int height, const int numLayers );
static void ksGpuDepthBuffer_Destroy( ksGpuContext * context, ksGpuDepthBuffer * depthBuffer );

================================================================================================================================
*/

typedef struct
{
	ksGpuSurfaceDepthFormat	format;
	VkFormat				internalFormat;
	VkImageLayout			imageLayout;
	VkImage					image;
	VkDeviceMemory			memory;
	VkImageView *			views;
	int						numViews;
} ksGpuDepthBuffer;

static VkFormat ksGpuDepthBuffer_InternalSurfaceDepthFormat( const ksGpuSurfaceDepthFormat depthFormat )
{
	return	( ( depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_D16 ) ? VK_FORMAT_D16_UNORM :
			( ( depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_D24 ) ? VK_FORMAT_D24_UNORM_S8_UINT :
			VK_FORMAT_UNDEFINED ) );
}
static void ksGpuDepthBuffer_Create( ksGpuContext * context, ksGpuDepthBuffer * depthBuffer,
									const ksGpuSurfaceDepthFormat depthFormat, const ksGpuSampleCount sampleCount,
									const int width, const int height, const int numLayers )
{
	assert( width >= 1 );
	assert( height >= 1 );
	assert( numLayers >= 1 );

	memset( depthBuffer, 0, sizeof( ksGpuDepthBuffer ) );

	depthBuffer->format = depthFormat;

	if ( depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_NONE )
	{
		depthBuffer->internalFormat = VK_FORMAT_UNDEFINED;
		return;
	}

	depthBuffer->internalFormat = ksGpuDepthBuffer_InternalSurfaceDepthFormat( depthFormat );

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
	imageCreateInfo.samples = (VkSampleCountFlagBits)sampleCount;
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
	memoryAllocateInfo.memoryTypeIndex = ksGpuDevice_GetMemoryTypeIndex( context->device, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

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
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
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
		ksGpuContext_CreateSetupCmdBuffer( context );

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

		ksGpuContext_FlushSetupCmdBuffer( context );
	}

	depthBuffer->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

static void ksGpuDepthBuffer_Destroy( ksGpuContext * context, ksGpuDepthBuffer * depthBuffer )
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
For optimal performance a window should only be created at load time, not at runtime.

ksGpuWindow
ksGpuWindowEvent
ksGpuWindowInput
ksKeyboardKey
ksMouseButton

static bool ksGpuWindow_SupportedResolution( const int width, const int height );
static bool ksGpuWindow_Create( ksGpuWindow * window, ksDriverInstance * instance,
								const ksGpuQueueInfo * queueInfo, const int queueIndex,
								const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
								const ksGpuSampleCount sampleCount, const int width, const int height, const bool fullscreen );
static void ksGpuWindow_Destroy( ksGpuWindow * window );
static void ksGpuWindow_Exit( ksGpuWindow * window );
static ksGpuWindowEvent ksGpuWindow_ProcessEvents( ksGpuWindow * window );
static void ksGpuWindow_SwapInterval( ksGpuWindow * window, const int swapInterval );
static void ksGpuWindow_SwapBuffers( ksGpuWindow * window );
static ksNanoseconds ksGpuWindow_GetNextSwapTimeNanoseconds( ksGpuWindow * window );
static ksNanoseconds ksGpuWindow_GetFrameTimeNanoseconds( ksGpuWindow * window );

static bool ksGpuWindowInput_ConsumeKeyboardKey( ksGpuWindowInput * input, const ksKeyboardKey key );
static bool ksGpuWindowInput_ConsumeMouseButton( ksGpuWindowInput * input, const ksMouseButton button );
static bool ksGpuWindowInput_CheckKeyboardKey( ksGpuWindowInput * input, const ksKeyboardKey key );

================================================================================================================================
*/

typedef enum
{
	KS_GPU_WINDOW_EVENT_NONE,
	KS_GPU_WINDOW_EVENT_ACTIVATED,
	KS_GPU_WINDOW_EVENT_DEACTIVATED,
	KS_GPU_WINDOW_EVENT_EXIT
} ksGpuWindowEvent;

typedef struct
{
	bool					keyInput[256];
	bool					mouseInput[8];
	int						mouseInputX[8];
	int						mouseInputY[8];
} ksGpuWindowInput;

typedef struct
{
	ksGpuDevice				device;
	ksGpuContext			context;
	ksGpuSurfaceColorFormat	colorFormat;
	ksGpuSurfaceDepthFormat	depthFormat;
	ksGpuSampleCount		sampleCount;
	int						windowWidth;
	int						windowHeight;
	int						windowSwapInterval;
	float					windowRefreshRate;
	bool					windowFullscreen;
	bool					windowActive;
	bool					windowExit;
	ksGpuWindowInput		input;
	ksNanoseconds			lastSwapTime;

	// The swapchain and depth buffer could be stored on the context like OpenGL but this makes more sense.
	VkSurfaceKHR			surface;
	int						swapchainCreateCount;
	ksGpuSwapchain			swapchain;
	ksGpuDepthBuffer		depthBuffer;

#if defined( OS_WINDOWS )
	HINSTANCE				hInstance;
	HWND					hWnd;
	bool					windowActiveState;
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
#elif defined( OS_APPLE_MACOS )
	CGDirectDisplayID		display;
	CGDisplayModeRef		desktopDisplayMode;
	NSWindow *				nsWindow;
	NSView *				nsView;
#elif defined( OS_APPLE_IOS )
	UIWindow *				uiWindow;
	UIView *				uiView;
#elif defined( OS_ANDROID )
	struct android_app *	app;
	Java_t					java;
	ANativeWindow *			nativeWindow;
	bool					resumed;
#elif defined( OS_NEUTRAL_DISPLAY_SURFACE )
	VkDisplayKHR			display;
	VkDisplayModeKHR		mode;
#endif
} ksGpuWindow;

static void ksGpuWindow_CreateFromSurface( ksGpuWindow * window, const VkSurfaceKHR surface )
{
	ksGpuSwapchain_Create( &window->context, &window->swapchain, surface, window->colorFormat, window->windowWidth, window->windowHeight, window->windowSwapInterval );
	ksGpuDepthBuffer_Create( &window->context, &window->depthBuffer, window->depthFormat, window->sampleCount, window->windowWidth, window->windowHeight, 1 );

#if defined( OS_APPLE )
	window->windowWidth = window->swapchain.width;			// iOS/macOS patch for Retina displays
	window->windowHeight = window->swapchain.height;		// iOS/macOS patch for Retina displays
#endif

	assert( window->swapchain.width == window->windowWidth && window->swapchain.height == window->windowHeight );

	window->surface = surface;
	window->colorFormat = window->swapchain.format;		// May not have acquired the desired format.
	window->depthFormat = window->depthBuffer.format;
	window->swapchainCreateCount++;
}

static void ksGpuWindow_DestroySurface( ksGpuWindow * window )
{
	ksGpuDepthBuffer_Destroy( &window->context, &window->depthBuffer );
	ksGpuSwapchain_Destroy( &window->context, &window->swapchain );
	VC( window->device.instance->vkDestroySurfaceKHR( window->device.instance->instance, window->surface, VK_ALLOCATOR ) );
}

#if defined( OS_WINDOWS )

typedef enum
{
	KEY_A				= 0x41,
	KEY_B				= 0x42,
	KEY_C				= 0x43,
	KEY_D				= 0x44,
	KEY_E				= 0x45,
	KEY_F				= 0x46,
	KEY_G				= 0x47,
	KEY_H				= 0x48,
	KEY_I				= 0x49,
	KEY_J				= 0x4A,
	KEY_K				= 0x4B,
	KEY_L				= 0x4C,
	KEY_M				= 0x4D,
	KEY_N				= 0x4E,
	KEY_O				= 0x4F,
	KEY_P				= 0x50,
	KEY_Q				= 0x51,
	KEY_R				= 0x52,
	KEY_S				= 0x53,
	KEY_T				= 0x54,
	KEY_U				= 0x55,
	KEY_V				= 0x56,
	KEY_W				= 0x57,
	KEY_X				= 0x58,
	KEY_Y				= 0x59,
	KEY_Z				= 0x5A,
	KEY_RETURN			= VK_RETURN,
	KEY_TAB				= VK_TAB,
	KEY_ESCAPE			= VK_ESCAPE,
	KEY_SHIFT_LEFT		= VK_LSHIFT,
	KEY_CTRL_LEFT		= VK_LCONTROL,
	KEY_ALT_LEFT		= VK_LMENU,
	KEY_CURSOR_UP		= VK_UP,
	KEY_CURSOR_DOWN		= VK_DOWN,
	KEY_CURSOR_LEFT		= VK_LEFT,
	KEY_CURSOR_RIGHT	= VK_RIGHT
} ksKeyboardKey;

typedef enum
{
	MOUSE_LEFT		= 0,
	MOUSE_RIGHT		= 1
} ksMouseButton;

LRESULT APIENTRY WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	ksGpuWindow * window = (ksGpuWindow *) GetWindowLongPtrA( hWnd, GWLP_USERDATA );

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
		case WM_KEYDOWN:
		{
			if ( window != NULL )
			{
				if ( (int)wParam >= 0 && (int)wParam < 256 )
				{
					if ( 	(int)wParam != KEY_SHIFT_LEFT &&
							(int)wParam != KEY_CTRL_LEFT &&
							(int)wParam != KEY_ALT_LEFT &&
							(int)wParam != KEY_CURSOR_UP &&
							(int)wParam != KEY_CURSOR_DOWN &&
							(int)wParam != KEY_CURSOR_LEFT &&
							(int)wParam != KEY_CURSOR_RIGHT )
					{
						window->input.keyInput[(int)wParam] = true;
					}
				}
			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			window->input.mouseInput[MOUSE_LEFT] = true;
			window->input.mouseInputX[MOUSE_LEFT] = LOWORD( lParam );
			window->input.mouseInputY[MOUSE_LEFT] = window->windowHeight - HIWORD( lParam );
			break;
		}
		case WM_RBUTTONDOWN:
		{
			window->input.mouseInput[MOUSE_RIGHT] = true;
			window->input.mouseInputX[MOUSE_RIGHT] = LOWORD( lParam );
			window->input.mouseInputY[MOUSE_RIGHT] = window->windowHeight - HIWORD( lParam );
			break;
		}
	}
	return DefWindowProcA( hWnd, message, wParam, lParam );
}

static void ksGpuWindow_Destroy( ksGpuWindow * window )
{
	ksGpuWindow_DestroySurface( window );
	ksGpuContext_Destroy( &window->context );
	ksGpuDevice_Destroy( &window->device );

	if ( window->windowFullscreen )
	{
		ChangeDisplaySettingsA( NULL, 0 );
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
		if ( !UnregisterClassA( APPLICATION_NAME, window->hInstance ) )
		{
			Error( "Failed to unregister window class." );
		}
		window->hInstance = NULL;
	}
}

static bool ksGpuWindow_Create( ksGpuWindow * window, ksDriverInstance * instance,
								const ksGpuQueueInfo * queueInfo, const int queueIndex,
								const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
								const ksGpuSampleCount sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( ksGpuWindow ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->windowActiveState = false;
	window->lastSwapTime = GetTimeNanoseconds();

	const LPCSTR displayDevice = NULL;

	if ( window->windowFullscreen )
	{
		DEVMODEA dmScreenSettings;
		memset( &dmScreenSettings, 0, sizeof( dmScreenSettings ) );
		dmScreenSettings.dmSize			= sizeof( dmScreenSettings );
		dmScreenSettings.dmPelsWidth	= width;
		dmScreenSettings.dmPelsHeight	= height;
		dmScreenSettings.dmBitsPerPel	= 32;
		dmScreenSettings.dmFields		= DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

		if ( ChangeDisplaySettingsExA( displayDevice, &dmScreenSettings, NULL, CDS_FULLSCREEN, NULL ) != DISP_CHANGE_SUCCESSFUL )
		{
			Error( "The requested fullscreen mode is not supported." );
			return false;
		}
	}

	DEVMODEA lpDevMode;
	memset( &lpDevMode, 0, sizeof( DEVMODEA ) );
	lpDevMode.dmSize = sizeof( DEVMODEA );
	lpDevMode.dmDriverExtra = 0;

	if ( EnumDisplaySettingsA( displayDevice, ENUM_CURRENT_SETTINGS, &lpDevMode ) != FALSE )
	{
		window->windowRefreshRate = (float)lpDevMode.dmDisplayFrequency;
	}

	window->hInstance = GetModuleHandleA( NULL );

	WNDCLASSA wc;
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

	if ( !RegisterClassA( &wc ) )
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

	window->hWnd = CreateWindowExA( dwExStyle,						// Extended style for the window
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
								NULL );								// No WM_CREATE parameter
	if ( !window->hWnd )
	{
		ksGpuWindow_Destroy( window );
		Error( "Failed to create window." );
		return false;
	}

	SetWindowLongPtrA( window->hWnd, GWLP_USERDATA, (LONG_PTR) window );

	VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo;
	win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	win32SurfaceCreateInfo.pNext = NULL;
	win32SurfaceCreateInfo.flags = 0;
	win32SurfaceCreateInfo.hinstance = window->hInstance;
	win32SurfaceCreateInfo.hwnd = window->hWnd;

	VkSurfaceKHR surface;
	VK( instance->vkCreateSurfaceKHR( instance->instance, &win32SurfaceCreateInfo, VK_ALLOCATOR, &surface ) );

	ksGpuDevice_SelectPhysicalDevice( &window->device, instance, queueInfo, surface );
	ksGpuDevice_Create( &window->device, instance, queueInfo );
	ksGpuContext_Create( &window->context, &window->device, queueIndex );
	ksGpuWindow_CreateFromSurface( window, surface );

	ShowWindow( window->hWnd, SW_SHOW );
	SetForegroundWindow( window->hWnd );
	SetFocus( window->hWnd );

	return true;
}

static bool ksGpuWindow_SupportedResolution( const int width, const int height )
{
	DEVMODE dm = { 0 };
	dm.dmSize = sizeof( dm );
	for ( int modeIndex = 0; EnumDisplaySettings( NULL, modeIndex, &dm ) != 0; modeIndex++ )
	{
		if ( dm.dmPelsWidth == (DWORD)width && dm.dmPelsHeight == (DWORD)height )
		{
			return true;
		}
	}
	return false;
}

static void ksGpuWindow_Exit( ksGpuWindow * window )
{
	window->windowExit = true;
}

static ksGpuWindowEvent ksGpuWindow_ProcessEvents( ksGpuWindow * window )
{
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

	window->input.keyInput[KEY_SHIFT_LEFT]		= GetAsyncKeyState( KEY_SHIFT_LEFT );
	window->input.keyInput[KEY_CTRL_LEFT]		= GetAsyncKeyState( KEY_CTRL_LEFT );
	window->input.keyInput[KEY_ALT_LEFT]		= GetAsyncKeyState( KEY_ALT_LEFT );
	window->input.keyInput[KEY_CURSOR_UP]		= GetAsyncKeyState( KEY_CURSOR_UP );
	window->input.keyInput[KEY_CURSOR_DOWN]		= GetAsyncKeyState( KEY_CURSOR_DOWN );
	window->input.keyInput[KEY_CURSOR_LEFT]		= GetAsyncKeyState( KEY_CURSOR_LEFT );
	window->input.keyInput[KEY_CURSOR_RIGHT]	= GetAsyncKeyState( KEY_CURSOR_RIGHT );

	if ( window->windowExit )
	{
		return KS_GPU_WINDOW_EVENT_EXIT;
	}
	if ( window->windowActiveState != window->windowActive )
	{
		window->windowActive = window->windowActiveState;
		return ( window->windowActiveState ) ? KS_GPU_WINDOW_EVENT_ACTIVATED : KS_GPU_WINDOW_EVENT_DEACTIVATED;
	}
	return KS_GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_LINUX_XLIB )

typedef enum	// keysym.h
{
	KEY_A				= XK_a,
	KEY_B				= XK_b,
	KEY_C				= XK_c,
	KEY_D				= XK_d,
	KEY_E				= XK_e,
	KEY_F				= XK_f,
	KEY_G				= XK_g,
	KEY_H				= XK_h,
	KEY_I				= XK_i,
	KEY_J				= XK_j,
	KEY_K				= XK_k,
	KEY_L				= XK_l,
	KEY_M				= XK_m,
	KEY_N				= XK_n,
	KEY_O				= XK_o,
	KEY_P				= XK_p,
	KEY_Q				= XK_q,
	KEY_R				= XK_r,
	KEY_S				= XK_s,
	KEY_T				= XK_t,
	KEY_U				= XK_u,
	KEY_V				= XK_v,
	KEY_W				= XK_w,
	KEY_X				= XK_x,
	KEY_Y				= XK_y,
	KEY_Z				= XK_z,
	KEY_RETURN			= ( XK_Return & 0xFF ),
	KEY_TAB				= ( XK_Tab & 0xFF ),
	KEY_ESCAPE			= ( XK_Escape & 0xFF ),
	KEY_SHIFT_LEFT		= ( XK_Shift_L & 0xFF ),
	KEY_CTRL_LEFT		= ( XK_Control_L & 0xFF ),
	KEY_ALT_LEFT		= ( XK_Alt_L & 0xFF ),
	KEY_CURSOR_UP		= ( XK_Up & 0xFF ),
	KEY_CURSOR_DOWN		= ( XK_Down & 0xFF ),
	KEY_CURSOR_LEFT		= ( XK_Left & 0xFF ),
	KEY_CURSOR_RIGHT	= ( XK_Right & 0xFF )
} ksKeyboardKey;

typedef enum
{
	MOUSE_LEFT		= Button1,
	MOUSE_RIGHT		= Button2
} ksMouseButton;

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

static void ksGpuWindow_Destroy( ksGpuWindow * window )
{
	ksGpuWindow_DestroySurface( window );
	ksGpuContext_Destroy( &window->context );
	ksGpuDevice_Destroy( &window->device );

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

static bool ksGpuWindow_Create( ksGpuWindow * window, ksDriverInstance * instance,
								const ksGpuQueueInfo * queueInfo, const int queueIndex,
								const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
								const ksGpuSampleCount sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( ksGpuWindow ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeNanoseconds();

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
		ksGpuWindow_Destroy( window );
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

	ksGpuDevice_SelectPhysicalDevice( &window->device, instance, queueInfo, surface );
	ksGpuDevice_Create( &window->device, instance, queueInfo );
	ksGpuContext_Create( &window->context, &window->device, queueIndex );
	ksGpuWindow_CreateFromSurface( window, surface );

	return true;
}

static bool ksGpuWindow_SupportedResolution( const int width, const int height )
{
	UNUSED_PARM( width );
	UNUSED_PARM( height );

	return true;
}

static void ksGpuWindow_Exit( ksGpuWindow * window )
{
	window->windowExit = true;
}

static ksGpuWindowEvent ksGpuWindow_ProcessEvents( ksGpuWindow * window )
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
					window->input.keyInput[key & 255] = true;
				}
				break;
			}
			case KeyRelease:
			{
				break;
			}
			case ButtonPress:
			{
				window->input.mouseInput[event.xbutton.button] = true;
				window->input.mouseInputX[event.xbutton.button] = event.xbutton.x;
				window->input.mouseInputY[event.xbutton.button] = event.xbutton.y;
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
		return KS_GPU_WINDOW_EVENT_EXIT;
	}

	if ( window->windowActive == false )
	{
		window->windowActive = true;
		return KS_GPU_WINDOW_EVENT_ACTIVATED;
	}

	return KS_GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_LINUX_XCB )

typedef enum	// keysym.h
{
	KEY_A				= XK_a,
	KEY_B				= XK_b,
	KEY_C				= XK_c,
	KEY_D				= XK_d,
	KEY_E				= XK_e,
	KEY_F				= XK_f,
	KEY_G				= XK_g,
	KEY_H				= XK_h,
	KEY_I				= XK_i,
	KEY_J				= XK_j,
	KEY_K				= XK_k,
	KEY_L				= XK_l,
	KEY_M				= XK_m,
	KEY_N				= XK_n,
	KEY_O				= XK_o,
	KEY_P				= XK_p,
	KEY_Q				= XK_q,
	KEY_R				= XK_r,
	KEY_S				= XK_s,
	KEY_T				= XK_t,
	KEY_U				= XK_u,
	KEY_V				= XK_v,
	KEY_W				= XK_w,
	KEY_X				= XK_x,
	KEY_Y				= XK_y,
	KEY_Z				= XK_z,
	KEY_RETURN			= ( XK_Return & 0xFF ),
	KEY_TAB				= ( XK_Tab & 0xFF ),
	KEY_ESCAPE			= ( XK_Escape & 0xFF ),
	KEY_SHIFT_LEFT		= ( XK_Shift_L & 0xFF ),
	KEY_CTRL_LEFT		= ( XK_Control_L & 0xFF ),
	KEY_ALT_LEFT		= ( XK_Alt_L & 0xFF ),
	KEY_CURSOR_UP		= ( XK_Up & 0xFF ),
	KEY_CURSOR_DOWN		= ( XK_Down & 0xFF ),
	KEY_CURSOR_LEFT		= ( XK_Left & 0xFF ),
	KEY_CURSOR_RIGHT	= ( XK_Right & 0xFF )
} ksKeyboardKey;

typedef enum
{
	MOUSE_LEFT		= 0,
	MOUSE_RIGHT		= 1
} ksMouseButton;

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

static void ksGpuWindow_Destroy( ksGpuWindow * window )
{
	ksGpuWindow_DestroySurface( window );
	ksGpuContext_Destroy( &window->context );
	ksGpuDevice_Destroy( &window->device );

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

static bool ksGpuWindow_Create( ksGpuWindow * window, ksDriverInstance * instance,
								const ksGpuQueueInfo * queueInfo, const int queueIndex,
								const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
								const ksGpuSampleCount sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( ksGpuWindow ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeNanoseconds();

	const char * displayName = NULL;
	int screen_number = 0;
	window->connection = xcb_connect( displayName, &screen_number );
	if ( xcb_connection_has_error( window->connection ) )
	{
		ksGpuWindow_Destroy( window );
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

	ksGpuDevice_SelectPhysicalDevice( &window->device, instance, queueInfo, surface );
	ksGpuDevice_Create( &window->device, instance, queueInfo );
	ksGpuContext_Create( &window->context, &window->device, queueIndex );
	ksGpuWindow_CreateFromSurface( window, surface );

	return true;
}

static bool ksGpuWindow_SupportedResolution( const int width, const int height )
{
	UNUSED_PARM( width );
	UNUSED_PARM( height );

	return true;
}

static void ksGpuWindow_Exit( ksGpuWindow * window )
{
	window->windowExit = true;
}

static ksGpuWindowEvent ksGpuWindow_ProcessEvents( ksGpuWindow * window )
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
					return KS_GPU_WINDOW_EVENT_EXIT;
				}
				break;
			}
			case XCB_KEY_PRESS:
			{
				xcb_key_press_event_t * key_press_event = (xcb_key_press_event_t *) event;
				const xcb_keysym_t keysym = xcb_key_press_lookup_keysym( window->key_symbols, key_press_event, 0 );
				if ( keysym < 256 || keysym == XK_Escape )
				{
					window->input.keyInput[keysym & 255] = true;
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
						window->input.mouseInput[i] = true;
						window->input.mouseInputX[i] = button_press_event->event_x;
						window->input.mouseInputY[i] = button_press_event->event_y;
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
		return KS_GPU_WINDOW_EVENT_EXIT;
	}

	if ( window->windowActive == false )
	{
		window->windowActive = true;
		return KS_GPU_WINDOW_EVENT_ACTIVATED;
	}

	return KS_GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_APPLE_MACOS )

typedef enum
{
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
	KEY_RETURN			= 0x24,
	KEY_TAB				= 0x30,
	KEY_ESCAPE			= 0x35,
	KEY_SHIFT_LEFT		= 0x38,
	KEY_CTRL_LEFT		= 0x3B,
	KEY_ALT_LEFT		= 0x3A,
	KEY_CURSOR_UP		= 0x7E,
	KEY_CURSOR_DOWN		= 0x7D,
	KEY_CURSOR_LEFT		= 0x7B,
	KEY_CURSOR_RIGHT	= 0x7C
} ksKeyboardKey;

typedef enum
{
	MOUSE_LEFT			= 0,
	MOUSE_RIGHT			= 1
} ksMouseButton;

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

#if defined( VK_USE_PLATFORM_MACOS_MVK )

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

static void ksGpuWindow_Destroy( ksGpuWindow * window )
{
	ksGpuWindow_DestroySurface( window );
	ksGpuContext_Destroy( &window->context );
	ksGpuDevice_Destroy( &window->device );

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

static bool ksGpuWindow_Create( ksGpuWindow * window, ksDriverInstance * instance,
								const ksGpuQueueInfo * queueInfo, const int queueIndex,
								const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
								const ksGpuSampleCount sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( ksGpuWindow ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeNanoseconds();

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

	VkMacOSSurfaceCreateInfoKHR macosSurfaceCreateInfo;
	macosSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_KHR;
	macosSurfaceCreateInfo.pNext = NULL;
	macosSurfaceCreateInfo.flags = 0;
	macosSurfaceCreateInfo.pView = window->nsView;

	VkSurfaceKHR surface;
	VK( instance->vkCreateSurfaceKHR( instance->instance, &macosSurfaceCreateInfo, VK_ALLOCATOR, &surface ) );

	ksGpuDevice_SelectPhysicalDevice( &window->device, instance, queueInfo, surface );
	ksGpuDevice_Create( &window->device, instance, queueInfo );
	ksGpuContext_Create( &window->context, &window->device, queueIndex );
	ksGpuWindow_CreateFromSurface( window, surface );

	return true;
}

static bool ksGpuWindow_SupportedResolution( const int width, const int height )
{
	UNUSED_PARM( width );
	UNUSED_PARM( height );

	return true;
}

static void ksGpuWindow_Exit( ksGpuWindow * window )
{
	window->windowExit = true;
}

static ksGpuWindowEvent ksGpuWindow_ProcessEvents( ksGpuWindow * window )
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
				window->input.keyInput[key] = true;
			}
		}
		else if ( event.type == NSLeftMouseDown )
		{
			NSPoint point = [event locationInWindow];
			window->input.mouseInput[MOUSE_LEFT] = true;
			window->input.mouseInputX[MOUSE_LEFT] = point.x;
			window->input.mouseInputY[MOUSE_LEFT] = point.y - 1;	// change to zero-based
		}
		else if ( event.type == NSRightMouseDown )
		{
			NSPoint point = [event locationInWindow];
			window->input.mouseInput[MOUSE_RIGHT] = true;
			window->input.mouseInputX[MOUSE_RIGHT] = point.x;
			window->input.mouseInputY[MOUSE_RIGHT] = point.y - 1;	// change to zero-based
		}

		[NSApp sendEvent:event];
	}

	if ( window->windowExit )
	{
		return KS_GPU_WINDOW_EVENT_EXIT;
	}

	if ( window->windowActive == false )
	{
		window->windowActive = true;
		return KS_GPU_WINDOW_EVENT_ACTIVATED;
	}

	return KS_GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_APPLE_IOS )

typedef enum
{
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
	KEY_RETURN			= 0x24,
	KEY_TAB				= 0x30,
	KEY_ESCAPE			= 0x35,
	KEY_SHIFT_LEFT		= 0x38,
	KEY_CTRL_LEFT		= 0x3B,
	KEY_ALT_LEFT		= 0x3A,
	KEY_CURSOR_UP		= 0x7E,
	KEY_CURSOR_DOWN		= 0x7D,
	KEY_CURSOR_LEFT		= 0x7B,
	KEY_CURSOR_RIGHT	= 0x7C
} ksKeyboardKey;

typedef enum
{
	MOUSE_LEFT			= 0,
	MOUSE_RIGHT			= 1
} ksMouseButton;

static NSAutoreleasePool * autoReleasePool;
static UIView* myUIView;
static UIWindow* myUIWindow;

@interface MyUIView : UIView
@end

@implementation MyUIView

-(instancetype) initWithFrame:(CGRect)frameRect {
	self = [super initWithFrame: frameRect];
	if ( self ) {
		self.contentScaleFactor = UIScreen.mainScreen.nativeScale;
	}
	return self;
}

+(Class) layerClass { return [CAMetalLayer class]; }

@end

@interface MyUIViewController : UIViewController
@end

@implementation MyUIViewController

-(UIInterfaceOrientationMask) supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }

-(BOOL) shouldAutorotate { return TRUE; }

@end

static void ksGpuWindow_Destroy( ksGpuWindow * window )
{
	ksGpuWindow_DestroySurface( window );
	ksGpuContext_Destroy( &window->context );
	ksGpuDevice_Destroy( &window->device );

	if ( window->uiWindow )
	{
		[window->uiWindow release];
		window->uiWindow = nil;
	}
	if ( window->uiView )
	{
		[window->uiView release];
		window->uiView = nil;
	}
}

static bool ksGpuWindow_Create( ksGpuWindow * window, ksDriverInstance * instance,
								const ksGpuQueueInfo * queueInfo, const int queueIndex,
								const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
								const ksGpuSampleCount sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( ksGpuWindow ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeNanoseconds();
	window->uiView = myUIView;
	window->uiWindow = myUIWindow;

	VkIOSSurfaceCreateInfoKHR iosSurfaceCreateInfo;
	iosSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_KHR;
	iosSurfaceCreateInfo.pNext = NULL;
	iosSurfaceCreateInfo.flags = 0;
	iosSurfaceCreateInfo.pView = window->uiView;

	VkSurfaceKHR surface;
	VK( instance->vkCreateSurfaceKHR( instance->instance, &iosSurfaceCreateInfo, VK_ALLOCATOR, &surface ) );

	ksGpuDevice_SelectPhysicalDevice( &window->device, instance, queueInfo, surface );
	ksGpuDevice_Create( &window->device, instance, queueInfo );
	ksGpuContext_Create( &window->context, &window->device, queueIndex );
	ksGpuWindow_CreateFromSurface( window, surface );

	return true;
}

static bool ksGpuWindow_SupportedResolution( const int width, const int height )
{
	UNUSED_PARM( width );
	UNUSED_PARM( height );

	return true;
}

static void ksGpuWindow_Exit( ksGpuWindow * window )
{
	window->windowExit = true;
}

static ksGpuWindowEvent ksGpuWindow_ProcessEvents( ksGpuWindow * window )
{
	[autoReleasePool release];
	autoReleasePool = [[NSAutoreleasePool alloc] init];

	if ( window->windowExit )
	{
		return KS_GPU_WINDOW_EVENT_EXIT;
	}

	if ( window->windowActive == false )
	{
		window->windowActive = true;
		return KS_GPU_WINDOW_EVENT_ACTIVATED;
	}
	
	return KS_GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_ANDROID )

typedef enum	// https://developer.android.com/ndk/reference/group___input.html
{
	KEY_A				= AKEYCODE_A,
	KEY_B				= AKEYCODE_B,
	KEY_C				= AKEYCODE_C,
	KEY_D				= AKEYCODE_D,
	KEY_E				= AKEYCODE_E,
	KEY_F				= AKEYCODE_F,
	KEY_G				= AKEYCODE_G,
	KEY_H				= AKEYCODE_H,
	KEY_I				= AKEYCODE_I,
	KEY_J				= AKEYCODE_J,
	KEY_K				= AKEYCODE_K,
	KEY_L				= AKEYCODE_L,
	KEY_M				= AKEYCODE_M,
	KEY_N				= AKEYCODE_N,
	KEY_O				= AKEYCODE_O,
	KEY_P				= AKEYCODE_P,
	KEY_Q				= AKEYCODE_Q,
	KEY_R				= AKEYCODE_R,
	KEY_S				= AKEYCODE_S,
	KEY_T				= AKEYCODE_T,
	KEY_U				= AKEYCODE_U,
	KEY_V				= AKEYCODE_V,
	KEY_W				= AKEYCODE_W,
	KEY_X				= AKEYCODE_X,
	KEY_Y				= AKEYCODE_Y,
	KEY_Z				= AKEYCODE_Z,
	KEY_RETURN			= AKEYCODE_ENTER,
	KEY_TAB				= AKEYCODE_TAB,
	KEY_ESCAPE			= AKEYCODE_ESCAPE,
	KEY_SHIFT_LEFT		= AKEYCODE_SHIFT_LEFT,
	KEY_CTRL_LEFT		= AKEYCODE_CTRL_LEFT,
	KEY_ALT_LEFT		= AKEYCODE_ALT_LEFT,
	KEY_CURSOR_UP		= AKEYCODE_DPAD_UP,
	KEY_CURSOR_DOWN		= AKEYCODE_DPAD_DOWN,
	KEY_CURSOR_LEFT		= AKEYCODE_DPAD_LEFT,
	KEY_CURSOR_RIGHT	= AKEYCODE_DPAD_RIGHT
} ksKeyboardKey;

typedef enum
{
	MOUSE_LEFT		= 0,
	MOUSE_RIGHT		= 1
} ksMouseButton;

static void app_handle_cmd( struct android_app * app, int32_t cmd )
{
	ksGpuWindow * window = (ksGpuWindow *)app->userData;

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
	ksGpuWindow * window = (ksGpuWindow *)app->userData;

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
				window->input.keyInput[keyCode] = true;
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
				window->input.mouseInput[MOUSE_LEFT] = true;
				window->input.mouseInputX[MOUSE_LEFT] = (int)x;
				window->input.mouseInputY[MOUSE_LEFT] = (int)y;
				return 1;
			}
			return 0;
		}
	}
	return 0;
}

static void ksGpuWindow_Destroy( ksGpuWindow * window )
{
	ksGpuContext_Destroy( &window->context );
	ksGpuDevice_Destroy( &window->device );

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

static bool ksGpuWindow_Create( ksGpuWindow * window, ksDriverInstance * instance,
								const ksGpuQueueInfo * queueInfo, const int queueIndex,
								const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
								const ksGpuSampleCount sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( ksGpuWindow ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = true;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeNanoseconds();

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

	ksGpuDevice_SelectPhysicalDevice( &window->device, instance, queueInfo, VK_NULL_HANDLE );
	ksGpuDevice_Create( &window->device, instance, queueInfo );
	ksGpuContext_Create( &window->context, &window->device, queueIndex );

	return true;
}

static bool ksGpuWindow_SupportedResolution( const int width, const int height )
{
	UNUSED_PARM( width );
	UNUSED_PARM( height );

	// Assume the HWC can handle any window size.
	return true;
}

static void ksGpuWindow_Exit( ksGpuWindow * window )
{
	// Call finish() on the activity and ksGpuWindow_ProcessEvents will handle the rest.
	ANativeActivity_finish( window->app->activity );
}

static ksGpuWindowEvent ksGpuWindow_ProcessEvents( ksGpuWindow * window )
{
	if ( window->app == NULL )
	{
		return KS_GPU_WINDOW_EVENT_NONE;
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

			ksGpuWindow_CreateFromSurface( window, surface );
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
			ksGpuWindow_DestroySurface( window );
		}
	}

	if ( window->app->destroyRequested != 0 )
	{
		return KS_GPU_WINDOW_EVENT_EXIT;
	}
	if ( windowWasActive != window->windowActive )
	{
		return ( window->windowActive ) ? KS_GPU_WINDOW_EVENT_ACTIVATED : KS_GPU_WINDOW_EVENT_DEACTIVATED;
	}
	return KS_GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_NEUTRAL_DISPLAY_SURFACE )

//These are empty enums in neutral display surface since we have no input devices.
//To make this work we need to disconnect the concepts of OS/input from presentation -
//though this is more difficult since most of the time presentation *is* input.

typedef enum
{
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_RETURN,
	KEY_TAB,
	KEY_ESCAPE,
	KEY_SHIFT_LEFT,
	KEY_CTRL_LEFT,
	KEY_ALT_LEFT,
	KEY_CURSOR_UP,
	KEY_CURSOR_DOWN,
	KEY_CURSOR_LEFT,
	KEY_CURSOR_RIGHT
} ksKeyboardKey;

typedef enum
{
	MOUSE_LEFT,
	MOUSE_RIGHT
} ksMouseButton;

static void ksGpuWindow_Destroy( ksGpuWindow * window )
{
	ksGpuWindow_DestroySurface( window );
	ksGpuContext_Destroy( &window->context );
	ksGpuDevice_Destroy( &window->device );
}

static bool ksGpuWindow_Create( ksGpuWindow * window, ksDriverInstance * instance,
								const ksGpuQueueInfo * queueInfo, const int queueIndex,
								const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
								const ksGpuSampleCount sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( ksGpuWindow ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = true;
	window->windowExit = false;
	window->lastSwapTime = GetTimeNanoseconds();

	ksGpuDevice_SelectPhysicalDevice( &window->device, instance, queueInfo, VK_NULL_HANDLE );

	uint32_t displayPropertiesCount = 0;
	VK( instance->vkGetPhysicalDeviceDisplayPropertiesKHR( window->device.physicalDevice, &displayPropertiesCount, NULL ) );

	VkDisplayPropertiesKHR * displayProperties = (VkDisplayPropertiesKHR *) malloc( displayPropertiesCount * sizeof( VkDisplayPropertiesKHR ) );
	VK( instance->vkGetPhysicalDeviceDisplayPropertiesKHR( window->device.physicalDevice, &displayPropertiesCount, displayProperties ) );

	for ( int i = 0; i < displayPropertiesCount; i++)
	{
		Printf( "Display %d:\n", i);
		Printf( "Name             : %s\n", displayProperties[i].displayName );
		Printf( "Dimensions       : %ux%u\n", displayProperties[i].physicalDimensions.width, displayProperties[i].physicalDimensions.height );
		Printf( "Resolution       : %ux%u\n", displayProperties[i].physicalResolution.width, displayProperties[i].physicalResolution.height );
		Printf( "Transforms       : %x\n", displayProperties[i].supportedTransforms );
		Printf( "Plane reordering : %s\n", displayProperties[i].planeReorderPossible ? "YES" : "NO" );
		Printf( "Persistent       : %s\n", displayProperties[i].persistentContent ? "YES" : "NO" );
	}

	// FIXME: Make mode and display selection configurable.
	VkDisplayKHR display = displayProperties[0].display;

	uint32_t displayModeCount = 0;
	VK( instance->vkGetDisplayModePropertiesKHR( window->device.physicalDevice, display, &displayModeCount, NULL ) );

	VkDisplayModePropertiesKHR * displayModes = (VkDisplayModePropertiesKHR *) malloc( displayModeCount * sizeof( VkDisplayModePropertiesKHR ) );
	VK( instance->vkGetDisplayModePropertiesKHR( window->device.physicalDevice, display, &displayModeCount, displayModes ) );

	for ( int i = 0; i < displayModeCount; i++)
	{
		Printf( "Mode %d:\n", i);
		Printf( "Visible Region : %ux%u\n", displayModes[i].parameters.visibleRegion.width, displayModes[i].parameters.visibleRegion.height );
		Printf( "Refresh Rate   : %u\n", displayModes[i].parameters.refreshRate );
	}

	VkDisplaySurfaceCreateInfoKHR displaySurfaceCreateInfo;
	displaySurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
	displaySurfaceCreateInfo.pNext = NULL;
	displaySurfaceCreateInfo.flags = 0;
	displaySurfaceCreateInfo.displayMode = displayModes[0].displayMode;
	displaySurfaceCreateInfo.planeIndex = 0;
	displaySurfaceCreateInfo.planeStackIndex = 0;
	displaySurfaceCreateInfo.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	displaySurfaceCreateInfo.globalAlpha = 0.0f;
	displaySurfaceCreateInfo.alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR;
	displaySurfaceCreateInfo.imageExtent.width = displayModes[0].parameters.visibleRegion.width;
	displaySurfaceCreateInfo.imageExtent.height = displayModes[0].parameters.visibleRegion.height;

	window->windowWidth = displaySurfaceCreateInfo.imageExtent.width;
	window->windowHeight = displaySurfaceCreateInfo.imageExtent.height;

	VkSurfaceKHR surface;
	VK( instance->vkCreateSurfaceKHR( instance->instance, &displaySurfaceCreateInfo, VK_ALLOCATOR, &surface ) );

	ksGpuDevice_Create( &window->device, instance, queueInfo );
	ksGpuContext_Create( &window->context, &window->device, queueIndex );
	ksGpuWindow_CreateFromSurface( window, surface );

	return true;
}

static bool ksGpuWindow_SupportedResolution( const int width, const int height )
{
	UNUSED_PARM( width );
	UNUSED_PARM( height );

	return true;
}

static void ksGpuWindow_Exit( ksGpuWindow * window )
{
	window->windowExit = true;
}

static ksGpuWindowEvent ksGpuWindow_ProcessEvents( ksGpuWindow * window )
{
	return KS_GPU_WINDOW_EVENT_NONE;
}

#endif

static void ksGpuWindow_SwapInterval( ksGpuWindow * window, const int swapInterval )
{
	if ( swapInterval != window->windowSwapInterval )
	{
		ksGpuContext_WaitIdle( &window->context );
		ksGpuSwapchain_Destroy( &window->context, &window->swapchain );
		ksGpuSwapchain_Create( &window->context, &window->swapchain, window->surface, window->colorFormat, window->windowWidth, window->windowHeight, swapInterval );
		window->windowSwapInterval = swapInterval;
		window->swapchainCreateCount++;
	}
}

static void ksGpuWindow_SwapBuffers( ksGpuWindow * window )
{
	ksNanoseconds newTimeNanoseconds = ksGpuSwapchain_SwapBuffers( &window->context, &window->swapchain );

	// Even with smoothing, this is not particularly accurate.
	const float frameTimeNanoseconds = 1000.0f * 1000.0f * 1000.0f / window->windowRefreshRate;
	const float deltaTimeNanoseconds = (float)newTimeNanoseconds - window->lastSwapTime - frameTimeNanoseconds;
	if ( fabs( deltaTimeNanoseconds ) < frameTimeNanoseconds * 0.75f )
	{
		newTimeNanoseconds = (ksNanoseconds)( window->lastSwapTime + frameTimeNanoseconds + 0.025f * deltaTimeNanoseconds );
	}
	//const float smoothDeltaNanoseconds = (float)( newTimeNanoseconds - window->lastSwapTime );
	//Print( "frame delta = %1.3f (error = %1.3f)\n", smoothDeltaNanoseconds * 1e-6f,
	//					( smoothDeltaNanoseconds - frameTimeNanoseconds ) * 1e-6f );
	window->lastSwapTime = newTimeNanoseconds;
}

static ksNanoseconds ksGpuWindow_GetNextSwapTimeNanoseconds( ksGpuWindow * window )
{
	const float frameTimeNanoseconds = 1000.0f * 1000.0f * 1000.0f / window->windowRefreshRate;
	return window->lastSwapTime + (ksNanoseconds)( frameTimeNanoseconds );
}

static ksNanoseconds ksGpuWindow_GetFrameTimeNanoseconds( ksGpuWindow * window )
{
	const float frameTimeNanoseconds = 1000.0f * 1000.0f * 1000.0f / window->windowRefreshRate;
	return (ksNanoseconds)( frameTimeNanoseconds );
}

static void ksGpuWindow_DelayBeforeSwap( ksGpuWindow * window, const ksNanoseconds delay )
{
	UNUSED_PARM( window );
	UNUSED_PARM( delay );
}

static bool ksGpuWindowInput_ConsumeKeyboardKey( ksGpuWindowInput * input, const ksKeyboardKey key )
{
	if ( input->keyInput[key] )
	{
		input->keyInput[key] = false;
		return true;
	}
	return false;
}

static bool ksGpuWindowInput_ConsumeMouseButton( ksGpuWindowInput * input, const ksMouseButton button )
{
	if ( input->mouseInput[button] )
	{
		input->mouseInput[button] = false;
		return true;
	}
	return false;
}

static bool ksGpuWindowInput_CheckKeyboardKey( ksGpuWindowInput * input, const ksKeyboardKey key )
{
	return ( input->keyInput[key] != false );
}

/*
================================================================================================================================

GPU buffer.

A buffer maintains a block of memory for a specific use by GPU programs (vertex, index, uniform, storage).
For optimal performance a buffer should only be created at load time, not at runtime.
The best performance is typically achieved when the buffer is not host visible.

ksGpuBufferType
ksGpuBuffer

static bool ksGpuBuffer_Create( ksGpuContext * context, ksGpuBuffer * buffer, const ksGpuBufferType type,
							const size_t dataSize, const void * data, const bool hostVisible );
static void ksGpuBuffer_CreateReference( ksGpuContext * context, ksGpuBuffer * buffer, const ksGpuBuffer * other );
static void ksGpuBuffer_Destroy( ksGpuContext * context, ksGpuBuffer * buffer );

================================================================================================================================
*/

typedef enum
{
	KS_GPU_BUFFER_TYPE_VERTEX,
	KS_GPU_BUFFER_TYPE_INDEX,
	KS_GPU_BUFFER_TYPE_UNIFORM,
	KS_GPU_BUFFER_TYPE_STORAGE
} ksGpuBufferType;

typedef struct ksGpuBuffer_s
{
	struct ksGpuBuffer_s *	next;
	int						unusedCount;
	ksGpuBufferType			type;
	size_t					size;
	VkMemoryPropertyFlags	flags;
	VkBuffer				buffer;
	VkDeviceMemory			memory;
	void *					mapped;
	bool			owner;
} ksGpuBuffer;

static VkBufferUsageFlags ksGpuBuffer_GetBufferUsage( const ksGpuBufferType type )
{
	return	( ( type == KS_GPU_BUFFER_TYPE_VERTEX ) ?	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT :
			( ( type == KS_GPU_BUFFER_TYPE_INDEX ) ?	VK_BUFFER_USAGE_INDEX_BUFFER_BIT :
			( ( type == KS_GPU_BUFFER_TYPE_UNIFORM ) ?	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT :
			( ( type == KS_GPU_BUFFER_TYPE_STORAGE ) ?	VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0 ) ) ) );
}

static VkAccessFlags ksGpuBuffer_GetBufferAccess( const ksGpuBufferType type )
{
	return	( ( type == KS_GPU_BUFFER_TYPE_INDEX ) ?	VK_ACCESS_INDEX_READ_BIT :
			( ( type == KS_GPU_BUFFER_TYPE_VERTEX ) ?	VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT :
			( ( type == KS_GPU_BUFFER_TYPE_UNIFORM ) ?	VK_ACCESS_UNIFORM_READ_BIT :
			( ( type == KS_GPU_BUFFER_TYPE_STORAGE ) ?	( VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT ) : 0 ) ) ) );
}

static bool ksGpuBuffer_Create( ksGpuContext * context, ksGpuBuffer * buffer, const ksGpuBufferType type,
							const size_t dataSize, const void * data, const bool hostVisible )
{
	memset( buffer, 0, sizeof( ksGpuBuffer ) );

	assert( dataSize <= context->device->physicalDeviceProperties.limits.maxStorageBufferRange );

	buffer->type = type;
	buffer->size = dataSize;
	buffer->owner = true;

	VkBufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = dataSize;
	bufferCreateInfo.usage = ksGpuBuffer_GetBufferUsage( type ) |
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
	memoryAllocateInfo.memoryTypeIndex = ksGpuDevice_GetMemoryTypeIndex( context->device, memoryRequirements.memoryTypeBits, buffer->flags );

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

			VkMappedMemoryRange mappedMemoryRange;
			mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedMemoryRange.pNext = NULL;
			mappedMemoryRange.memory = buffer->memory;
			mappedMemoryRange.offset = 0;
			mappedMemoryRange.size = VK_WHOLE_SIZE;
			VC( context->device->vkFlushMappedMemoryRanges( context->device->device, 1, &mappedMemoryRange ) );
		}
		else
		{
			VkBufferCreateInfo stagingBufferCreateInfo;
			stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			stagingBufferCreateInfo.pNext = NULL;
			stagingBufferCreateInfo.flags = 0;
			stagingBufferCreateInfo.size = dataSize;
			stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			stagingBufferCreateInfo.queueFamilyIndexCount = 0;
			stagingBufferCreateInfo.pQueueFamilyIndices = NULL;

			VkBuffer srcBuffer;
			VK( context->device->vkCreateBuffer( context->device->device, &stagingBufferCreateInfo, VK_ALLOCATOR, &srcBuffer ) );

			VkMemoryRequirements stagingMemoryRequirements;
			VC( context->device->vkGetBufferMemoryRequirements( context->device->device, srcBuffer, &stagingMemoryRequirements ) );

			VkMemoryAllocateInfo stagingMemoryAllocateInfo;
			stagingMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			stagingMemoryAllocateInfo.pNext = NULL;
			stagingMemoryAllocateInfo.allocationSize = stagingMemoryRequirements.size;
			stagingMemoryAllocateInfo.memoryTypeIndex = ksGpuDevice_GetMemoryTypeIndex( context->device, stagingMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );

			VkDeviceMemory srcMemory;
			VK( context->device->vkAllocateMemory( context->device->device, &stagingMemoryAllocateInfo, VK_ALLOCATOR, &srcMemory ) );
			VK( context->device->vkBindBufferMemory( context->device->device, srcBuffer, srcMemory, 0 ) );

			void * mapped;
			VK( context->device->vkMapMemory( context->device->device, srcMemory, 0, stagingMemoryRequirements.size, 0, &mapped ) );
			memcpy( mapped, data, dataSize );
			VC( context->device->vkUnmapMemory( context->device->device, srcMemory ) );

			VkMappedMemoryRange mappedMemoryRange;
			mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedMemoryRange.pNext = NULL;
			mappedMemoryRange.memory = srcMemory;
			mappedMemoryRange.offset = 0;
			mappedMemoryRange.size = VK_WHOLE_SIZE;
			VC( context->device->vkFlushMappedMemoryRanges( context->device->device, 1, &mappedMemoryRange ) );

			ksGpuContext_CreateSetupCmdBuffer( context );

			VkBufferCopy bufferCopy;
			bufferCopy.srcOffset = 0;
			bufferCopy.dstOffset = 0;
			bufferCopy.size = dataSize;

			VC( context->device->vkCmdCopyBuffer( context->setupCommandBuffer, srcBuffer, buffer->buffer, 1, &bufferCopy ) );

			ksGpuContext_FlushSetupCmdBuffer( context );

			VC( context->device->vkDestroyBuffer( context->device->device, srcBuffer, VK_ALLOCATOR ) );
			VC( context->device->vkFreeMemory( context->device->device, srcMemory, VK_ALLOCATOR ) );
		}
	}

	return true;
}

static void ksGpuBuffer_CreateReference( ksGpuContext * context, ksGpuBuffer * buffer, const ksGpuBuffer * other )
{
	UNUSED_PARM( context );

	buffer->next = NULL;
	buffer->unusedCount = 0;
	buffer->type = other->type;
	buffer->size = other->size;
	buffer->flags = other->flags;
	buffer->buffer = other->buffer;
	buffer->memory = other->memory;
	buffer->mapped = NULL;
	buffer->owner = false;
}

static void ksGpuBuffer_Destroy( ksGpuContext * context, ksGpuBuffer * buffer )
{
	if ( buffer->mapped != NULL )
	{
		VC( context->device->vkUnmapMemory( context->device->device, buffer->memory ) );
	}
	if ( buffer->owner )
	{
		VC( context->device->vkDestroyBuffer( context->device->device, buffer->buffer, VK_ALLOCATOR ) );
		VC( context->device->vkFreeMemory( context->device->device, buffer->memory, VK_ALLOCATOR ) );
	}
}

/*
================================================================================================================================

GPU texture.

Supports loading textures from raw data or KTX container files.
For optimal performance a texture should only be created or modified at load time, not at runtime.
Note that the geometry code assumes the texture origin 0,0 = left-top as opposed to left-bottom.
In other words, textures are expected to be stored top-down as opposed to bottom-up.

ksGpuTextureFormat
ksGpuTextureUsage
ksGpuTextureWrapMode
ksGpuTextureFilter
ksGpuTextureDefault
ksGpuTexture

static bool ksGpuTexture_Create2D( ksGpuContext * context, ksGpuTexture * texture,
									const ksGpuTextureFormat format, const ksGpuSampleCount sampleCount,
									const int width, const int height, const int mipCount,
									const ksGpuTextureUsageFlags usageFlags, const void * data, const size_t dataSize );
static bool ksGpuTexture_Create2DArray( ksGpuContext * context, ksGpuTexture * texture,
									const ksGpuTextureFormat format, const ksGpuSampleCount sampleCount,
									const int width, const int height, const int layerCount, const int mipCount,
									const ksGpuTextureUsageFlags usageFlags, const void * data, const size_t dataSize );
static bool ksGpuTexture_CreateDefault( ksGpuContext * context, ksGpuTexture * texture, const ksGpuTextureDefault defaultType,
									const int width, const int height, const int depth,
									const int layerCount, const int faceCount, const bool mipmaps, const bool border );
static bool ksGpuTexture_CreateFromSwapchain( ksGpuContext * context, ksGpuTexture * texture, const ksGpuWindow * window, int index );
static bool ksGpuTexture_CreateFromFile( ksGpuContext * context, ksGpuTexture * texture, const char * fileName );
static void ksGpuTexture_Destroy( ksGpuContext * context, ksGpuTexture * texture );

static void ksGpuTexture_SetFilter( ksGpuContext * context, ksGpuTexture * texture, const ksGpuTextureFilter filter );
static void ksGpuTexture_SetAniso( ksGpuContext * context, ksGpuTexture * texture, const float maxAniso );
static void ksGpuTexture_SetWrapMode( ksGpuContext * context, ksGpuTexture * texture, const ksGpuTextureWrapMode wrapMode );

================================================================================================================================
*/

// Note that the channel listed first in the name shall occupy the least significant bit.
typedef enum
{
	//
	// 8 bits per component
	//
	KS_GPU_TEXTURE_FORMAT_R8_UNORM				= VK_FORMAT_R8_UNORM,					// 1-component, 8-bit unsigned normalized
	KS_GPU_TEXTURE_FORMAT_R8G8_UNORM			= VK_FORMAT_R8G8_UNORM,					// 2-component, 8-bit unsigned normalized
	KS_GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM		= VK_FORMAT_R8G8B8A8_UNORM,				// 4-component, 8-bit unsigned normalized

	KS_GPU_TEXTURE_FORMAT_R8_SNORM				= VK_FORMAT_R8_SNORM,					// 1-component, 8-bit signed normalized
	KS_GPU_TEXTURE_FORMAT_R8G8_SNORM			= VK_FORMAT_R8G8_SNORM,					// 2-component, 8-bit signed normalized
	KS_GPU_TEXTURE_FORMAT_R8G8B8A8_SNORM		= VK_FORMAT_R8G8B8A8_SNORM,				// 4-component, 8-bit signed normalized

	KS_GPU_TEXTURE_FORMAT_R8_UINT				= VK_FORMAT_R8_UINT,					// 1-component, 8-bit unsigned integer
	KS_GPU_TEXTURE_FORMAT_R8G8_UINT				= VK_FORMAT_R8G8_UINT,					// 2-component, 8-bit unsigned integer
	KS_GPU_TEXTURE_FORMAT_R8G8B8A8_UINT			= VK_FORMAT_R8G8B8A8_UINT,				// 4-component, 8-bit unsigned integer

	KS_GPU_TEXTURE_FORMAT_R8_SINT				= VK_FORMAT_R8_SINT,					// 1-component, 8-bit signed integer
	KS_GPU_TEXTURE_FORMAT_R8G8_SINT				= VK_FORMAT_R8G8_SINT,					// 2-component, 8-bit signed integer
	KS_GPU_TEXTURE_FORMAT_R8G8B8A8_SINT			= VK_FORMAT_R8G8B8A8_SINT,				// 4-component, 8-bit signed integer

	KS_GPU_TEXTURE_FORMAT_R8_SRGB				= VK_FORMAT_R8_SRGB,					// 1-component, 8-bit sRGB
	KS_GPU_TEXTURE_FORMAT_R8G8_SRGB				= VK_FORMAT_R8G8_SRGB,					// 2-component, 8-bit sRGB
	KS_GPU_TEXTURE_FORMAT_R8G8B8A8_SRGB			= VK_FORMAT_R8G8B8A8_SRGB,				// 4-component, 8-bit sRGB

	//
	// 16 bits per component
	//
	KS_GPU_TEXTURE_FORMAT_R16_UNORM				= VK_FORMAT_R16_UNORM,					// 1-component, 16-bit unsigned normalized
	KS_GPU_TEXTURE_FORMAT_R16G16_UNORM			= VK_FORMAT_R16G16_UNORM,				// 2-component, 16-bit unsigned normalized
	KS_GPU_TEXTURE_FORMAT_R16G16B16A16_UNORM	= VK_FORMAT_R16G16B16A16_UNORM,			// 4-component, 16-bit unsigned normalized

	KS_GPU_TEXTURE_FORMAT_R16_SNORM				= VK_FORMAT_R16_SNORM,					// 1-component, 16-bit signed normalized
	KS_GPU_TEXTURE_FORMAT_R16G16_SNORM			= VK_FORMAT_R16G16_SNORM,				// 2-component, 16-bit signed normalized
	KS_GPU_TEXTURE_FORMAT_R16G16B16A16_SNORM	= VK_FORMAT_R16G16B16A16_SNORM,			// 4-component, 16-bit signed normalized

	KS_GPU_TEXTURE_FORMAT_R16_UINT				= VK_FORMAT_R16_UINT,					// 1-component, 16-bit unsigned integer
	KS_GPU_TEXTURE_FORMAT_R16G16_UINT			= VK_FORMAT_R16G16_UINT,				// 2-component, 16-bit unsigned integer
	KS_GPU_TEXTURE_FORMAT_R16G16B16A16_UINT		= VK_FORMAT_R16G16B16A16_UINT,			// 4-component, 16-bit unsigned integer

	KS_GPU_TEXTURE_FORMAT_R16_SINT				= VK_FORMAT_R16_SINT,					// 1-component, 16-bit signed integer
	KS_GPU_TEXTURE_FORMAT_R16G16_SINT			= VK_FORMAT_R16G16_SINT,				// 2-component, 16-bit signed integer
	KS_GPU_TEXTURE_FORMAT_R16G16B16A16_SINT		= VK_FORMAT_R16G16B16A16_SINT,			// 4-component, 16-bit signed integer

	KS_GPU_TEXTURE_FORMAT_R16_SFLOAT			= VK_FORMAT_R16_SFLOAT,					// 1-component, 16-bit floating-point
	KS_GPU_TEXTURE_FORMAT_R16G16_SFLOAT			= VK_FORMAT_R16G16_SFLOAT,				// 2-component, 16-bit floating-point
	KS_GPU_TEXTURE_FORMAT_R16G16B16A16_SFLOAT	= VK_FORMAT_R16G16B16A16_SFLOAT,		// 4-component, 16-bit floating-point

	//
	// 32 bits per component
	//
	KS_GPU_TEXTURE_FORMAT_R32_UINT				= VK_FORMAT_R32_UINT,					// 1-component, 32-bit unsigned integer
	KS_GPU_TEXTURE_FORMAT_R32G32_UINT			= VK_FORMAT_R32G32_UINT,				// 2-component, 32-bit unsigned integer
	KS_GPU_TEXTURE_FORMAT_R32G32B32A32_UINT		= VK_FORMAT_R32G32B32A32_UINT,			// 4-component, 32-bit unsigned integer

	KS_GPU_TEXTURE_FORMAT_R32_SINT				= VK_FORMAT_R32_SINT,					// 1-component, 32-bit signed integer
	KS_GPU_TEXTURE_FORMAT_R32G32_SINT			= VK_FORMAT_R32G32_SINT,				// 2-component, 32-bit signed integer
	KS_GPU_TEXTURE_FORMAT_R32G32B32A32_SINT		= VK_FORMAT_R32G32B32A32_SINT,			// 4-component, 32-bit signed integer

	KS_GPU_TEXTURE_FORMAT_R32_SFLOAT			= VK_FORMAT_R32_SFLOAT,					// 1-component, 32-bit floating-point
	KS_GPU_TEXTURE_FORMAT_R32G32_SFLOAT			= VK_FORMAT_R32G32_SFLOAT,				// 2-component, 32-bit floating-point
	KS_GPU_TEXTURE_FORMAT_R32G32B32A32_SFLOAT	= VK_FORMAT_R32G32B32A32_SFLOAT,		// 4-component, 32-bit floating-point

	//
	// S3TC/DXT/BC
	//
	KS_GPU_TEXTURE_FORMAT_BC1_R8G8B8_UNORM		= VK_FORMAT_BC1_RGB_UNORM_BLOCK,		// 3-component, line through 3D space, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_BC1_R8G8B8A1_UNORM	= VK_FORMAT_BC1_RGBA_UNORM_BLOCK,		// 4-component, line through 3D space plus 1-bit alpha, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_BC2_R8G8B8A8_UNORM	= VK_FORMAT_BC2_UNORM_BLOCK,			// 4-component, line through 3D space plus line through 1D space, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_BC3_R8G8B8A4_UNORM	= VK_FORMAT_BC3_UNORM_BLOCK,			// 4-component, line through 3D space plus 4-bit alpha, unsigned normalized

	KS_GPU_TEXTURE_FORMAT_BC1_R8G8B8_SRGB		= VK_FORMAT_BC1_RGB_SRGB_BLOCK,			// 3-component, line through 3D space, sRGB
	KS_GPU_TEXTURE_FORMAT_BC1_R8G8B8A1_SRGB		= VK_FORMAT_BC1_RGBA_SRGB_BLOCK,		// 4-component, line through 3D space plus 1-bit alpha, sRGB
	KS_GPU_TEXTURE_FORMAT_BC2_R8G8B8A8_SRGB		= VK_FORMAT_BC2_SRGB_BLOCK,				// 4-component, line through 3D space plus line through 1D space, sRGB
	KS_GPU_TEXTURE_FORMAT_BC3_R8G8B8A4_SRGB		= VK_FORMAT_BC3_SRGB_BLOCK,				// 4-component, line through 3D space plus 4-bit alpha, sRGB
    
	KS_GPU_TEXTURE_FORMAT_BC4_R8_UNORM			= VK_FORMAT_BC4_UNORM_BLOCK,			// 1-component, line through 1D space, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_BC5_R8G8_UNORM		= VK_FORMAT_BC5_UNORM_BLOCK,			// 2-component, two lines through 1D space, unsigned normalized

	KS_GPU_TEXTURE_FORMAT_BC4_R8_SNORM			= VK_FORMAT_BC4_SNORM_BLOCK,			// 1-component, line through 1D space, signed normalized
	KS_GPU_TEXTURE_FORMAT_BC5_R8G8_SNORM		= VK_FORMAT_BC5_SNORM_BLOCK,			// 2-component, two lines through 1D space, signed normalized

	//
	// ETC
	//
	KS_GPU_TEXTURE_FORMAT_ETC2_R8G8B8_UNORM		= VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,	// 3-component ETC2, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ETC2_R8G8B8A1_UNORM	= VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,	// 3-component with 1-bit alpha ETC2, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_UNORM	= VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,	// 4-component ETC2, unsigned normalized

	KS_GPU_TEXTURE_FORMAT_ETC2_R8G8B8_SRGB		= VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,		// 3-component ETC2, sRGB
	KS_GPU_TEXTURE_FORMAT_ETC2_R8G8B8A1_SRGB	= VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,	// 3-component with 1-bit alpha ETC2, sRGB
	KS_GPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_SRGB	= VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,	// 4-component ETC2, sRGB

	KS_GPU_TEXTURE_FORMAT_EAC_R11_UNORM			= VK_FORMAT_EAC_R11_UNORM_BLOCK,		// 1-component ETC, line through 1D space, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_EAC_R11G11_UNORM		= VK_FORMAT_EAC_R11G11_UNORM_BLOCK,		// 2-component ETC, two lines through 1D space, unsigned normalized

	KS_GPU_TEXTURE_FORMAT_EAC_R11_SNORM			= VK_FORMAT_EAC_R11_SNORM_BLOCK,		// 1-component ETC, line through 1D space, signed normalized
	KS_GPU_TEXTURE_FORMAT_EAC_R11G11_SNORM		= VK_FORMAT_EAC_R11G11_SNORM_BLOCK,		// 2-component ETC, two lines through 1D space, signed normalized

	//
	// ASTC
	//
	KS_GPU_TEXTURE_FORMAT_ASTC_4x4_UNORM		= VK_FORMAT_ASTC_4x4_UNORM_BLOCK,		// 4-component ASTC, 4x4 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_5x4_UNORM		= VK_FORMAT_ASTC_5x4_UNORM_BLOCK,		// 4-component ASTC, 5x4 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_5x5_UNORM		= VK_FORMAT_ASTC_5x5_UNORM_BLOCK,		// 4-component ASTC, 5x5 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_6x5_UNORM		= VK_FORMAT_ASTC_6x5_UNORM_BLOCK,		// 4-component ASTC, 6x5 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_6x6_UNORM		= VK_FORMAT_ASTC_6x6_UNORM_BLOCK,		// 4-component ASTC, 6x6 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_8x5_UNORM		= VK_FORMAT_ASTC_8x5_UNORM_BLOCK,		// 4-component ASTC, 8x5 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_8x6_UNORM		= VK_FORMAT_ASTC_8x6_UNORM_BLOCK,		// 4-component ASTC, 8x6 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_8x8_UNORM		= VK_FORMAT_ASTC_8x8_UNORM_BLOCK,		// 4-component ASTC, 8x8 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_10x5_UNORM		= VK_FORMAT_ASTC_10x5_UNORM_BLOCK,		// 4-component ASTC, 10x5 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_10x6_UNORM		= VK_FORMAT_ASTC_10x6_UNORM_BLOCK,		// 4-component ASTC, 10x6 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_10x8_UNORM		= VK_FORMAT_ASTC_10x8_UNORM_BLOCK,		// 4-component ASTC, 10x8 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_10x10_UNORM		= VK_FORMAT_ASTC_10x10_UNORM_BLOCK,		// 4-component ASTC, 10x10 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_12x10_UNORM		= VK_FORMAT_ASTC_12x10_UNORM_BLOCK,		// 4-component ASTC, 12x10 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_12x12_UNORM		= VK_FORMAT_ASTC_12x12_UNORM_BLOCK,		// 4-component ASTC, 12x12 blocks, unsigned normalized

	KS_GPU_TEXTURE_FORMAT_ASTC_4x4_SRGB			= VK_FORMAT_ASTC_4x4_SRGB_BLOCK,		// 4-component ASTC, 4x4 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_5x4_SRGB			= VK_FORMAT_ASTC_5x4_SRGB_BLOCK,		// 4-component ASTC, 5x4 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_5x5_SRGB			= VK_FORMAT_ASTC_5x5_SRGB_BLOCK,		// 4-component ASTC, 5x5 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_6x5_SRGB			= VK_FORMAT_ASTC_6x5_SRGB_BLOCK,		// 4-component ASTC, 6x5 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_6x6_SRGB			= VK_FORMAT_ASTC_6x6_SRGB_BLOCK,		// 4-component ASTC, 6x6 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_8x5_SRGB			= VK_FORMAT_ASTC_8x5_SRGB_BLOCK,		// 4-component ASTC, 8x5 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_8x6_SRGB			= VK_FORMAT_ASTC_8x6_SRGB_BLOCK,		// 4-component ASTC, 8x6 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_8x8_SRGB			= VK_FORMAT_ASTC_8x8_SRGB_BLOCK,		// 4-component ASTC, 8x8 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_10x5_SRGB		= VK_FORMAT_ASTC_10x5_SRGB_BLOCK,		// 4-component ASTC, 10x5 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_10x6_SRGB		= VK_FORMAT_ASTC_10x6_SRGB_BLOCK,		// 4-component ASTC, 10x6 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_10x8_SRGB		= VK_FORMAT_ASTC_10x8_SRGB_BLOCK,		// 4-component ASTC, 10x8 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_10x10_SRGB		= VK_FORMAT_ASTC_10x10_SRGB_BLOCK,		// 4-component ASTC, 10x10 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_12x10_SRGB		= VK_FORMAT_ASTC_12x10_SRGB_BLOCK,		// 4-component ASTC, 12x10 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_12x12_SRGB		= VK_FORMAT_ASTC_12x12_SRGB_BLOCK,		// 4-component ASTC, 12x12 blocks, sRGB
} ksGpuTextureFormat;

typedef enum
{
	KS_GPU_TEXTURE_USAGE_UNDEFINED			= BIT( 0 ),
	KS_GPU_TEXTURE_USAGE_GENERAL			= BIT( 1 ),
	KS_GPU_TEXTURE_USAGE_TRANSFER_SRC		= BIT( 2 ),
	KS_GPU_TEXTURE_USAGE_TRANSFER_DST		= BIT( 3 ),
	KS_GPU_TEXTURE_USAGE_SAMPLED			= BIT( 4 ),
	KS_GPU_TEXTURE_USAGE_STORAGE			= BIT( 5 ),
	KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT	= BIT( 6 ),
	KS_GPU_TEXTURE_USAGE_PRESENTATION		= BIT( 7 )
} ksGpuTextureUsage;

typedef unsigned int ksGpuTextureUsageFlags;

typedef enum
{
	KS_GPU_TEXTURE_WRAP_MODE_REPEAT,
	KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_EDGE,
	KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER
} ksGpuTextureWrapMode;

typedef enum
{
	KS_GPU_TEXTURE_FILTER_NEAREST,
	KS_GPU_TEXTURE_FILTER_LINEAR,
	KS_GPU_TEXTURE_FILTER_BILINEAR
} ksGpuTextureFilter;

typedef enum
{
	KS_GPU_TEXTURE_DEFAULT_CHECKERBOARD,	// 32x32 checkerboard pattern (KS_GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM)
	KS_GPU_TEXTURE_DEFAULT_PYRAMIDS,		// 32x32 block pattern of pyramids (KS_GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM)
	KS_GPU_TEXTURE_DEFAULT_CIRCLES			// 32x32 block pattern with circles (KS_GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM)
} ksGpuTextureDefault;

typedef struct
{
	int						width;
	int						height;
	int						depth;
	int						layerCount;
	int						mipCount;
	ksGpuSampleCount		sampleCount;
	ksGpuTextureUsage		usage;
	ksGpuTextureUsageFlags	usageFlags;
	ksGpuTextureWrapMode	wrapMode;
	ksGpuTextureFilter		filter;
	float					maxAnisotropy;
	VkFormat				format;
	VkImageLayout			imageLayout;
	VkImage					image;
	VkDeviceMemory			memory;
	VkImageView				view;
	VkSampler				sampler;
} ksGpuTexture;

static void ksGpuTexture_UpdateSampler( ksGpuContext * context, ksGpuTexture * texture )
{
	if ( texture->sampler != VK_NULL_HANDLE )
	{
		VC( context->device->vkDestroySampler( context->device->device, texture->sampler, VK_ALLOCATOR ) );
	}

	const VkSamplerMipmapMode mipmapMode =	( ( texture->filter == KS_GPU_TEXTURE_FILTER_NEAREST ) ? VK_SAMPLER_MIPMAP_MODE_NEAREST :
											( ( texture->filter == KS_GPU_TEXTURE_FILTER_LINEAR  ) ? VK_SAMPLER_MIPMAP_MODE_NEAREST :
											( VK_SAMPLER_MIPMAP_MODE_LINEAR ) ) );
	const VkSamplerAddressMode addressMode =	( ( texture->wrapMode == KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_EDGE ) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE :
												( ( texture->wrapMode == KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER ) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER :
												( VK_SAMPLER_ADDRESS_MODE_REPEAT ) ) );

	VkSamplerCreateInfo samplerCreateInfo;
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext = NULL;
	samplerCreateInfo.flags = 0;
	samplerCreateInfo.magFilter = ( texture->filter == KS_GPU_TEXTURE_FILTER_NEAREST ) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = ( texture->filter == KS_GPU_TEXTURE_FILTER_NEAREST ) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
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
	samplerCreateInfo.maxLod = (float)texture->mipCount;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	VK( context->device->vkCreateSampler( context->device->device, &samplerCreateInfo, VK_ALLOCATOR, &texture->sampler ) );
}

static VkImageLayout LayoutForTextureUsage( const ksGpuTextureUsage usage )
{
	return	( ( usage == KS_GPU_TEXTURE_USAGE_UNDEFINED ) ?			VK_IMAGE_LAYOUT_UNDEFINED :
			( ( usage == KS_GPU_TEXTURE_USAGE_GENERAL ) ?			VK_IMAGE_LAYOUT_GENERAL :
			( ( usage == KS_GPU_TEXTURE_USAGE_TRANSFER_SRC ) ?		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL :
			( ( usage == KS_GPU_TEXTURE_USAGE_TRANSFER_DST ) ?		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL :
			( ( usage == KS_GPU_TEXTURE_USAGE_SAMPLED ) ?			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :
			( ( usage == KS_GPU_TEXTURE_USAGE_STORAGE ) ?			VK_IMAGE_LAYOUT_GENERAL :
			( ( usage == KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT ) ?	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
			( ( usage == KS_GPU_TEXTURE_USAGE_PRESENTATION ) ?		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : 0 ) ) ) ) ) ) ) );
}

static VkAccessFlags AccessForTextureUsage( const ksGpuTextureUsage usage )
{
	return	( ( usage == KS_GPU_TEXTURE_USAGE_UNDEFINED ) ?			( 0 ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_GENERAL ) ?			( 0 ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_TRANSFER_SRC ) ?		( VK_ACCESS_TRANSFER_READ_BIT ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_TRANSFER_DST ) ?		( VK_ACCESS_TRANSFER_WRITE_BIT ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_SAMPLED ) ?			( VK_ACCESS_SHADER_READ_BIT ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_STORAGE ) ?			( VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT ) ?	( VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_PRESENTATION ) ?		( VK_ACCESS_MEMORY_READ_BIT ) : 0 ) ) ) ) ) ) ) );
}

static VkPipelineStageFlags PipelineStagesForTextureUsage( const ksGpuTextureUsage usage, const bool from )
{
	return	( ( usage == KS_GPU_TEXTURE_USAGE_UNDEFINED ) ?			( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_GENERAL ) ?			( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_TRANSFER_SRC ) ?		( VK_PIPELINE_STAGE_TRANSFER_BIT ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_TRANSFER_DST ) ?		( VK_PIPELINE_STAGE_TRANSFER_BIT ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_SAMPLED ) ?			( VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_STORAGE ) ?			( VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT ) ?	( VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT ) :
			( ( usage == KS_GPU_TEXTURE_USAGE_PRESENTATION ) ?		( from ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT ) : 0 ) ) ) ) ) ) ) );
}

static void ksGpuTexture_ChangeUsage( ksGpuContext * context, VkCommandBuffer cmdBuffer, ksGpuTexture * texture, const ksGpuTextureUsage usage )
{
	assert( ( texture->usageFlags & usage ) != 0 );

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

	VC( context->device->vkCmdPipelineBarrier( cmdBuffer, src_stages, dst_stages, flags, 0, NULL, 0, NULL, 1, &imageMemoryBarrier ) );

	texture->usage = usage;
	texture->imageLayout = newImageLayout;
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
// 'depth' must be >= 0 and <= 32768.
// 'layerCount' must be >= 0.
// 'faceCount' must be either 1 or 6.
// 'mipCount' must be -1 or >= 1.
// 'mipCount' includes the finest level.
// 'mipCount' set to -1 will allocate the full mip chain.
// 'data' may be NULL to allocate a texture without initialization.
// 'dataSize' is the full data size in bytes.
// The 'data' is expected to be stored packed on a per mip level basis.
// If 'data' != NULL and 'mipCount' <= 0, then the full mip chain will be generated from the finest data level.
// mipSizeStored indicates that the first uint32 of data is the size in bytes of the following mip.
static bool ksGpuTexture_CreateInternal( ksGpuContext * context, ksGpuTexture * texture, const char * fileName,
										const VkFormat format, const ksGpuSampleCount sampleCount,
										const int width, const int height, const int depth,
										const int layerCount, const int faceCount, const int mipCount,
										const ksGpuTextureUsageFlags usageFlags,
										const void * data, const size_t dataSize, const bool mipSizeStored )
{
	UNUSED_PARM( dataSize );

	memset( texture, 0, sizeof( ksGpuTexture ) );

	assert( depth >= 0 );
	assert( layerCount >= 0 );
	assert( faceCount == 1 || faceCount == 6 );

	if ( width < 1 || width > 32768 || height < 1 || height > 32768 || depth < 0 || depth > 32768 )
	{
		Error( "%s: Invalid texture size (%dx%dx%d)", fileName, width, height, depth );
		return false;
	}

	if ( faceCount != 1 && faceCount != 6 )
	{
		Error( "%s: Cube maps must have 6 faces (%d)", fileName, faceCount );
		return false;
	}

	if ( faceCount == 6 && width != height )
	{
		Error( "%s: Cube maps must be square (%dx%d)", fileName, width, height );
		return false;
	}

	if ( depth > 0 && layerCount > 0 )
	{
		Error( "%s: 3D array textures not supported", fileName );
		return false;
	}

	const int maxDimension = width > height ? ( width > depth ? width : depth ) : ( height > depth ? height : depth );
	const int maxMipLevels = ( 1 + IntegerLog2( maxDimension ) );

	if ( mipCount > maxMipLevels )
	{
		Error( "%s: Too many mip levels (%d > %d)", fileName, mipCount, maxMipLevels );
		return false;
	}

	VkFormatProperties props;
	VC( context->device->instance->vkGetPhysicalDeviceFormatProperties( context->device->physicalDevice, format, &props ) );

	// If this image is sampled.
	if ( ( usageFlags & KS_GPU_TEXTURE_USAGE_SAMPLED ) != 0 )
	{
		if ( ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) == 0 )
		{
			Error( "%s: Texture format %d cannot be sampled", fileName, format );
			return false;
		}
	}
	// If this image is rendered to.
	if ( ( usageFlags & KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT ) != 0 )
	{
		if ( ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT ) == 0 )
		{
			Error( "%s: Texture format %d cannot be rendered to", fileName, format );
			return false;
		}
	}
	// If this image is used for storage.
	if ( ( usageFlags & KS_GPU_TEXTURE_USAGE_STORAGE ) != 0 )
	{
		if ( ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT ) == 0 )
		{
			Error( "%s: Texture format %d cannot be used for storage", fileName, format );
			return false;
		}
	}

	const int numStorageLevels = ( mipCount >= 1 ) ? mipCount : maxMipLevels;
	const int arrayLayerCount = faceCount * MAX( layerCount, 1 );

	texture->width = width;
	texture->height = height;
	texture->depth = depth;
	texture->layerCount = arrayLayerCount;
	texture->mipCount = numStorageLevels;
	texture->sampleCount = sampleCount;
	texture->usage = KS_GPU_TEXTURE_USAGE_UNDEFINED;
	texture->usageFlags = usageFlags;
	texture->wrapMode = KS_GPU_TEXTURE_WRAP_MODE_REPEAT;
	texture->filter = ( numStorageLevels > 1 ) ? KS_GPU_TEXTURE_FILTER_BILINEAR : KS_GPU_TEXTURE_FILTER_LINEAR;
	texture->maxAnisotropy = 1.0f;
	texture->format = format;

	const VkImageUsageFlags usage =
		// Must be able to copy to the image for initialization.
		( ( usageFlags & KS_GPU_TEXTURE_USAGE_TRANSFER_DST ) != 0 || data != NULL ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0 ) |
		// Must be able to blit from the image to create mip maps.
		( ( usageFlags & KS_GPU_TEXTURE_USAGE_TRANSFER_SRC ) != 0 || ( data != NULL && mipCount < 1 ) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0 ) |
		// If this image is sampled.
		( ( usageFlags & KS_GPU_TEXTURE_USAGE_SAMPLED ) != 0 ? VK_IMAGE_USAGE_SAMPLED_BIT : 0 ) |
		// If this image is rendered to.
		( ( usageFlags & KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT ) != 0 ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0 ) |
		// If this image is used for storage.
		( ( usageFlags & KS_GPU_TEXTURE_USAGE_STORAGE ) != 0 ? VK_IMAGE_USAGE_STORAGE_BIT : 0 );

	// Create tiled image.
	VkImageCreateInfo imageCreateInfo;
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = NULL;
	imageCreateInfo.flags = ( faceCount == 6 ) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	imageCreateInfo.imageType = ( depth > 0 ) ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = MAX( depth, 1 );
	imageCreateInfo.mipLevels = numStorageLevels;
	imageCreateInfo.arrayLayers = arrayLayerCount;
	imageCreateInfo.samples = (VkSampleCountFlagBits)sampleCount;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = usage;
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
	memoryAllocateInfo.memoryTypeIndex = ksGpuDevice_GetMemoryTypeIndex( context->device, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	VK( context->device->vkAllocateMemory( context->device->device, &memoryAllocateInfo, VK_ALLOCATOR, &texture->memory ) );
	VK( context->device->vkBindImageMemory( context->device->device, texture->image, texture->memory, 0 ) );

	if ( data == NULL )
	{
		ksGpuContext_CreateSetupCmdBuffer( context );

		// Set optimal image layout for shader read access.
		{
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
		}

		ksGpuContext_FlushSetupCmdBuffer( context );
	}
	else	// Copy source data through a staging buffer.
	{
		assert( sampleCount == KS_GPU_SAMPLE_COUNT_1 );

		ksGpuContext_CreateSetupCmdBuffer( context );

		// Set optimal image layout for transfer destination.
		{
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
		}

		const int numDataLevels = ( mipCount >= 1 ) ? mipCount : 1;
		bool compressed = false;

		// Using a staging buffer to initialize the tiled image.
		VkBufferCreateInfo stagingBufferCreateInfo;
		stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferCreateInfo.pNext = NULL;
		stagingBufferCreateInfo.flags = 0;
		stagingBufferCreateInfo.size = dataSize;
		stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		stagingBufferCreateInfo.queueFamilyIndexCount = 0;
		stagingBufferCreateInfo.pQueueFamilyIndices = NULL;

		VkBuffer stagingBuffer;
		VK( context->device->vkCreateBuffer( context->device->device, &stagingBufferCreateInfo, VK_ALLOCATOR, &stagingBuffer ) );

		VkMemoryRequirements stagingMemoryRequirements;
		VC( context->device->vkGetBufferMemoryRequirements( context->device->device, stagingBuffer, &stagingMemoryRequirements ) );

		VkMemoryAllocateInfo stagingMemoryAllocateInfo;
		stagingMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		stagingMemoryAllocateInfo.pNext = NULL;
		stagingMemoryAllocateInfo.allocationSize = stagingMemoryRequirements.size;
		stagingMemoryAllocateInfo.memoryTypeIndex = ksGpuDevice_GetMemoryTypeIndex( context->device, stagingMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );

		VkDeviceMemory stagingMemory;
		VK( context->device->vkAllocateMemory( context->device->device, &stagingMemoryAllocateInfo, VK_ALLOCATOR, &stagingMemory ) );
		VK( context->device->vkBindBufferMemory( context->device->device, stagingBuffer, stagingMemory, 0 ) );

		uint8_t * mapped;
		VK( context->device->vkMapMemory( context->device->device, stagingMemory, 0, stagingMemoryRequirements.size, 0, (void **)&mapped ) );
		memcpy( mapped, data, dataSize );
		VC( context->device->vkUnmapMemory( context->device->device, stagingMemory ) );

		// Make sure the CPU writes to the buffer are flushed.
		{
			VkBufferMemoryBarrier bufferMemoryBarrier;
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.pNext = NULL;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.buffer = stagingBuffer;
			bufferMemoryBarrier.offset = 0;
			bufferMemoryBarrier.size = dataSize;

			const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			const VkDependencyFlags flags = 0;

			VC( context->device->vkCmdPipelineBarrier( context->setupCommandBuffer, src_stages, dst_stages, flags, 0, NULL, 1, &bufferMemoryBarrier, 0, NULL ) );
		}

		VkBufferImageCopy * bufferImageCopy = (VkBufferImageCopy *) malloc( numDataLevels * arrayLayerCount * MAX( depth, 1 ) * sizeof( VkBufferImageCopy ) );
		uint32_t bufferImageCopyIndex = 0;
		uint32_t dataOffset = 0;
		for ( int mipLevel = 0; mipLevel < numDataLevels; mipLevel++ )
		{
			const int mipWidth = ( width >> mipLevel ) >= 1 ? ( width >> mipLevel ) : 1;
			const int mipHeight = ( height >> mipLevel ) >= 1 ? ( height >> mipLevel ) : 1;
			const int mipDepth = ( depth >> mipLevel ) >= 1 ? ( depth >> mipLevel ) : 1;

			uint32_t totalMipSize = 0;
			uint32_t storedMipSize = 0;
			if ( mipSizeStored )
			{
				assert( dataOffset + 4 <= dataSize );
				storedMipSize = *(uint32_t *)&(((uint8_t *)data)[dataOffset]);
				dataOffset += 4;
			}
			UNUSED_PARM( storedMipSize );

			for ( int layerIndex = 0; layerIndex < arrayLayerCount; layerIndex++ )
			{
				for ( int depthIndex = 0; depthIndex < mipDepth; depthIndex++ )
				{
					bufferImageCopy[bufferImageCopyIndex].bufferOffset = dataOffset;
					bufferImageCopy[bufferImageCopyIndex].bufferRowLength = 0;
					bufferImageCopy[bufferImageCopyIndex].bufferImageHeight = 0;
					bufferImageCopy[bufferImageCopyIndex].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					bufferImageCopy[bufferImageCopyIndex].imageSubresource.mipLevel = mipLevel;
					bufferImageCopy[bufferImageCopyIndex].imageSubresource.baseArrayLayer = layerIndex;
					bufferImageCopy[bufferImageCopyIndex].imageSubresource.layerCount = 1;
					bufferImageCopy[bufferImageCopyIndex].imageOffset.x = 0;
					bufferImageCopy[bufferImageCopyIndex].imageOffset.y = 0;
					bufferImageCopy[bufferImageCopyIndex].imageOffset.z = depthIndex;
					bufferImageCopy[bufferImageCopyIndex].imageExtent.width = mipWidth;
					bufferImageCopy[bufferImageCopyIndex].imageExtent.height = mipHeight;
					bufferImageCopy[bufferImageCopyIndex].imageExtent.depth = 1;
					bufferImageCopyIndex++;

					uint32_t mipSize = 0;
					switch ( format )
					{
						//
						// 8 bits per component
						//
						case VK_FORMAT_R8_UNORM:					{ mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned char ); break; }
						case VK_FORMAT_R8G8_UNORM:					{ mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned char ); break; }
						case VK_FORMAT_R8G8B8A8_UNORM:				{ mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned char ); break; }

						case VK_FORMAT_R8_SNORM:					{ mipSize = mipHeight * mipWidth * 1 * sizeof( char ); break; }
						case VK_FORMAT_R8G8_SNORM:					{ mipSize = mipHeight * mipWidth * 2 * sizeof( char ); break; }
						case VK_FORMAT_R8G8B8_SNORM:				{ mipSize = mipHeight * mipWidth * 4 * sizeof( char ); break; }

						case VK_FORMAT_R8_UINT:						{ mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned char ); break; }
						case VK_FORMAT_R8G8_UINT:					{ mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned char ); break; }
						case VK_FORMAT_R8G8B8_UINT:					{ mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned char ); break; }

						case VK_FORMAT_R8_SINT:						{ mipSize = mipHeight * mipWidth * 1 * sizeof( char ); break; }
						case VK_FORMAT_R8G8_SINT:					{ mipSize = mipHeight * mipWidth * 2 * sizeof( char ); break; }
						case VK_FORMAT_R8G8B8_SINT:					{ mipSize = mipHeight * mipWidth * 4 * sizeof( char ); break; }

						case VK_FORMAT_R8_SRGB:						{ mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned char ); break; }
						case VK_FORMAT_R8G8_SRGB:					{ mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned char ); break; }
						case VK_FORMAT_R8G8B8A8_SRGB:				{ mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned char ); break; }

						//
						// 16 bits per component
						//
						case VK_FORMAT_R16_UNORM:					{ mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned short ); break; }
						case VK_FORMAT_R16G16_UNORM:				{ mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned short ); break; }
						case VK_FORMAT_R16G16B16A16_UNORM:			{ mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned short ); break; }

						case VK_FORMAT_R16_SNORM:					{ mipSize = mipHeight * mipWidth * 1 * sizeof( short ); break; }
						case VK_FORMAT_R16G16_SNORM:				{ mipSize = mipHeight * mipWidth * 2 * sizeof( short ); break; }
						case VK_FORMAT_R16G16B16A16_SNORM:			{ mipSize = mipHeight * mipWidth * 4 * sizeof( short ); break; }

						case VK_FORMAT_R16_UINT:					{ mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned short ); break; }
						case VK_FORMAT_R16G16_UINT:					{ mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned short ); break; }
						case VK_FORMAT_R16G16B16A16_UINT:			{ mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned short ); break; }

						case VK_FORMAT_R16_SINT:					{ mipSize = mipHeight * mipWidth * 1 * sizeof( short ); break; }
						case VK_FORMAT_R16G16_SINT:					{ mipSize = mipHeight * mipWidth * 2 * sizeof( short ); break; }
						case VK_FORMAT_R16G16B16A16_SINT:			{ mipSize = mipHeight * mipWidth * 4 * sizeof( short ); break; }

						case VK_FORMAT_R16_SFLOAT:					{ mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned short ); break; }
						case VK_FORMAT_R16G16_SFLOAT:				{ mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned short ); break; }
						case VK_FORMAT_R16G16B16A16_SFLOAT:			{ mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned short ); break; }

						//
						// 32 bits per component
						//
						case VK_FORMAT_R32_UINT:					{ mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned int ); break; }
						case VK_FORMAT_R32G32_UINT:					{ mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned int ); break; }
						case VK_FORMAT_R32G32B32A32_UINT:			{ mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned int ); break; }

						case VK_FORMAT_R32_SINT:					{ mipSize = mipHeight * mipWidth * 1 * sizeof( int ); break; }
						case VK_FORMAT_R32G32_SINT:					{ mipSize = mipHeight * mipWidth * 2 * sizeof( int ); break; }
						case VK_FORMAT_R32G32B32A32_SINT:			{ mipSize = mipHeight * mipWidth * 4 * sizeof( int ); break; }

						case VK_FORMAT_R32_SFLOAT:					{ mipSize = mipHeight * mipWidth * 1 * sizeof( float ); break; }
						case VK_FORMAT_R32G32_SFLOAT:				{ mipSize = mipHeight * mipWidth * 2 * sizeof( float ); break; }
						case VK_FORMAT_R32G32B32A32_SFLOAT:			{ mipSize = mipHeight * mipWidth * 4 * sizeof( float ); break; }

						//
						// S3TC/DXT/BC
						//
						case VK_FORMAT_BC1_RGB_UNORM_BLOCK:			{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:		{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_BC2_UNORM_BLOCK:				{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 16; compressed = true; break; }
						case VK_FORMAT_BC3_UNORM_BLOCK:				{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 16; compressed = true; break; }

						case VK_FORMAT_BC1_RGB_SRGB_BLOCK:			{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:			{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_BC2_SRGB_BLOCK:				{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 16; compressed = true; break; }
						case VK_FORMAT_BC3_SRGB_BLOCK:				{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 16; compressed = true; break; }

						case VK_FORMAT_BC4_UNORM_BLOCK:				{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_BC5_UNORM_BLOCK:				{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 16; compressed = true; break; }

						case VK_FORMAT_BC4_SNORM_BLOCK:				{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_BC5_SNORM_BLOCK:				{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 16; compressed = true; break; }

						//
						// ETC
						//
						case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:		{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:	{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:	{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 16; compressed = true; break; }

						case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:		{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:	{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:	{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 16; compressed = true; break; }

						case VK_FORMAT_EAC_R11_UNORM_BLOCK:			{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:		{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 16; compressed = true; break; }

						case VK_FORMAT_EAC_R11_SNORM_BLOCK:			{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 8; compressed = true; break; }
						case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:		{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 16; compressed = true; break; }

						//
						// ASTC
						//
						case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:		{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:		{ mipSize = ((mipHeight+3)/4) * ((mipWidth+4)/5) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:		{ mipSize = ((mipHeight+4)/5) * ((mipWidth+4)/5) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:		{ mipSize = ((mipHeight+4)/5) * ((mipWidth+5)/6) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:		{ mipSize = ((mipHeight+5)/6) * ((mipWidth+5)/6) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:		{ mipSize = ((mipHeight+4)/5) * ((mipWidth+7)/8) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:		{ mipSize = ((mipHeight+5)/6) * ((mipWidth+7)/8) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:		{ mipSize = ((mipHeight+7)/8) * ((mipWidth+7)/8) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:		{ mipSize = ((mipHeight+4)/5) * ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:		{ mipSize = ((mipHeight+5)/6) * ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:		{ mipSize = ((mipHeight+7)/8) * ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:		{ mipSize = ((mipHeight+9)/10) * ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:		{ mipSize = ((mipHeight+9)/10) * ((mipWidth+11)/12) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:		{ mipSize = ((mipHeight+11)/12) * ((mipWidth+11)/12) * 16; compressed = true; break; }

						case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:			{ mipSize = ((mipHeight+3)/4) * ((mipWidth+3)/4) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:			{ mipSize = ((mipHeight+3)/4) * ((mipWidth+4)/5) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:			{ mipSize = ((mipHeight+4)/5) * ((mipWidth+4)/5) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:			{ mipSize = ((mipHeight+4)/5) * ((mipWidth+5)/6) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:			{ mipSize = ((mipHeight+5)/6) * ((mipWidth+5)/6) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:			{ mipSize = ((mipHeight+4)/5) * ((mipWidth+7)/8) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:			{ mipSize = ((mipHeight+5)/6) * ((mipWidth+7)/8) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:			{ mipSize = ((mipHeight+7)/8) * ((mipWidth+7)/8) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:		{ mipSize = ((mipHeight+4)/5) * ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:		{ mipSize = ((mipHeight+5)/6) * ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:		{ mipSize = ((mipHeight+7)/8) * ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:		{ mipSize = ((mipHeight+9)/10) * ((mipWidth+9)/10) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:		{ mipSize = ((mipHeight+9)/10) * ((mipWidth+11)/12) * 16; compressed = true; break; }
						case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:		{ mipSize = ((mipHeight+11)/12) * ((mipWidth+11)/12) * 16; compressed = true; break; }

						default:
						{
							Error( "%s: Unsupported texture format %d", fileName, format );
							return false;
						}
					}

					assert( dataOffset + mipSize <= dataSize );

					totalMipSize += mipSize;
					dataOffset += mipSize;
					if ( mipSizeStored && ( depth <= 0 && layerCount <= 0 ) )
					{
						assert( mipSize == storedMipSize );
						dataOffset += 3 - ( ( mipSize + 3 ) % 4 );
					}
				}
			}
			if ( mipSizeStored && ( depth > 0 || layerCount > 0 ) )
			{
				assert( totalMipSize == storedMipSize );
				dataOffset += 3 - ( ( totalMipSize + 3 ) % 4 );
			}
		}

		assert( dataOffset == dataSize );

		VC( context->device->vkCmdCopyBufferToImage( context->setupCommandBuffer, stagingBuffer, texture->image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, numDataLevels * arrayLayerCount * MAX( depth, 1 ), bufferImageCopy ) );

		if ( mipCount < 1 )
		{
			assert( !compressed );
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
					imageBlit.srcOffsets[1].x = ( width >> prevMipLevel ) >= 1 ? ( width >> prevMipLevel ) : 0;
					imageBlit.srcOffsets[1].y = ( height >> prevMipLevel ) >= 1 ? ( height >> prevMipLevel ) : 0;
					imageBlit.srcOffsets[1].z = ( depth >> prevMipLevel ) >= 1 ? ( depth >> prevMipLevel ) : 1;
					imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlit.dstSubresource.mipLevel = mipLevel;
					imageBlit.dstSubresource.baseArrayLayer = 0;
					imageBlit.dstSubresource.layerCount = arrayLayerCount;
					imageBlit.dstOffsets[0].x = 0;
					imageBlit.dstOffsets[0].y = 0;
					imageBlit.dstOffsets[0].z = 0;
					imageBlit.dstOffsets[1].x = ( width >> mipLevel ) >= 1 ? ( width >> mipLevel ) : 0;
					imageBlit.dstOffsets[1].y = ( height >> mipLevel ) >= 1 ? ( height >> mipLevel ) : 0;
					imageBlit.dstOffsets[1].z = ( depth >> mipLevel ) >= 1 ? ( depth >> mipLevel ) : 1;

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
			imageMemoryBarrier.srcAccessMask = ( mipCount >= 1 ) ? VK_ACCESS_TRANSFER_WRITE_BIT : VK_ACCESS_TRANSFER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
			imageMemoryBarrier.oldLayout = ( mipCount >= 1 ) ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
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

		ksGpuContext_FlushSetupCmdBuffer( context );

		VC( context->device->vkFreeMemory( context->device->device, stagingMemory, VK_ALLOCATOR ) );
		VC( context->device->vkDestroyBuffer( context->device->device, stagingBuffer, VK_ALLOCATOR ) );
		free( bufferImageCopy );
	}

	texture->usage = KS_GPU_TEXTURE_USAGE_SAMPLED;
	texture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	const VkImageViewType viewType =	( ( depth > 0 ) ? VK_IMAGE_VIEW_TYPE_3D :
										( ( faceCount == 6 ) ?
										( ( layerCount > 0 ) ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE ) :
										( ( layerCount > 0 ) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D ) ) );

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

	ksGpuTexture_UpdateSampler( context, texture );

	return true;
}

static bool ksGpuTexture_Create2D( ksGpuContext * context, ksGpuTexture * texture,
									const ksGpuTextureFormat format, const ksGpuSampleCount sampleCount,
									const int width, const int height, const int mipCount,
									const ksGpuTextureUsageFlags usageFlags, const void * data, const size_t dataSize )
{
	const int depth = 0;
	const int layerCount = 0;
	const int faceCount = 1;
	return ksGpuTexture_CreateInternal( context, texture, "data", (VkFormat)format, sampleCount, width, height, depth,
										layerCount, faceCount, mipCount,
										usageFlags, data, dataSize, false );
}

static bool ksGpuTexture_Create2DArray( ksGpuContext * context, ksGpuTexture * texture,
										const ksGpuTextureFormat format, const ksGpuSampleCount sampleCount,
										const int width, const int height, const int layerCount, const int mipCount,
										const ksGpuTextureUsageFlags usageFlags, const void * data, const size_t dataSize )
{
	const int depth = 0;
	const int faceCount = 1;
	return ksGpuTexture_CreateInternal( context, texture, "data", (VkFormat)format, sampleCount, width, height, depth,
										layerCount, faceCount, mipCount,
										usageFlags, data, dataSize, false );
}

static bool ksGpuTexture_CreateDefault( ksGpuContext * context, ksGpuTexture * texture, const ksGpuTextureDefault defaultType,
										const int width, const int height, const int depth,
										const int layerCount, const int faceCount,
										const bool mipmaps, const bool border )
{
	const int TEXEL_SIZE = 4;
	const int layerSize = width * height * TEXEL_SIZE;
	const int dataSize = MAX( depth, 1 ) * MAX( layerCount, 1 ) * faceCount * layerSize;
	unsigned char * data = (unsigned char *) malloc( dataSize );

	if ( defaultType == KS_GPU_TEXTURE_DEFAULT_CHECKERBOARD )
	{
		const int blockSize = 32;	// must be a power of two
		for ( int layer = 0; layer < MAX( depth, 1 ) * MAX( layerCount, 1 ) * faceCount; layer++ )
		{
			for ( int y = 0; y < height; y++ )
			{
				for ( int x = 0; x < width; x++ )
				{
					if ( ( ( ( x / blockSize ) ^ ( y / blockSize ) ) & 1 ) == 0 )
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
	else if ( defaultType == KS_GPU_TEXTURE_DEFAULT_PYRAMIDS )
	{
		const int blockSize = 32;	// must be a power of two
		for ( int layer = 0; layer < MAX( depth, 1 ) * MAX( layerCount, 1 ) * faceCount; layer++ )
		{
			for ( int y = 0; y < height; y++ )
			{
				for ( int x = 0; x < width; x++ )
				{
					const int mask = blockSize - 1;
					const int lx = x & mask;
					const int ly = y & mask;
					const int rx = mask - lx;
					const int ry = mask - ly;

					char cx = 0;
					char cy = 0;
					if ( lx != ly && lx != ry )
					{
						int m = blockSize;
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
	else if ( defaultType == KS_GPU_TEXTURE_DEFAULT_CIRCLES )
	{
		const int blockSize = 32;	// must be a power of two
		const int radius = 10;
		const unsigned char colors[4][4] =
		{
			{ 0xFF, 0x00, 0x00, 0xFF },
			{ 0x00, 0xFF, 0x00, 0xFF },
			{ 0x00, 0x00, 0xFF, 0xFF },
			{ 0xFF, 0xFF, 0x00, 0xFF }
		};
		for ( int layer = 0; layer < MAX( depth, 1 ) * MAX( layerCount, 1 ) * faceCount; layer++ )
		{
			for ( int y = 0; y < height; y++ )
			{
				for ( int x = 0; x < width; x++ )
				{
					// Pick a color per block of texels.
					const int index =	( ( ( y / ( blockSize / 2 ) ) & 2 ) ^ ( ( x / ( blockSize * 1 ) ) & 2 ) ) |
										( ( ( x / ( blockSize * 1 ) ) & 1 ) ^ ( ( y / ( blockSize * 2 ) ) & 1 ) );

					// Draw a circle with radius 10 centered inside each 32x32 block of texels.
					const int dX = ( x & ~( blockSize - 1 ) ) + ( blockSize / 2 ) - x;
					const int dY = ( y & ~( blockSize - 1 ) ) + ( blockSize / 2 ) - y;
					const int dS = abs( dX * dX + dY * dY - radius * radius );
					const int scale = ( dS <= blockSize ) ? dS : blockSize;

					for ( int c = 0; c < TEXEL_SIZE - 1; c++ )
					{
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + c] = (unsigned char)( ( colors[index][c] * scale ) / blockSize );
					}
					data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + TEXEL_SIZE - 1] = 255;
				}
			}
		}
	}

	if ( border )
	{
		for ( int layer = 0; layer < MAX( depth, 1 ) * MAX( layerCount, 1 ) * faceCount; layer++ )
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

	const int mipCount = ( mipmaps ) ? -1 : 1;
	bool success = ksGpuTexture_CreateInternal( context, texture, "data", VK_FORMAT_R8G8B8A8_UNORM, KS_GPU_SAMPLE_COUNT_1,
												width, height, depth,
												layerCount, faceCount, mipCount,
												KS_GPU_TEXTURE_USAGE_SAMPLED, data, dataSize, false );

	free( data );

	return success;
}

static bool ksGpuTexture_CreateFromSwapchain( ksGpuContext * context, ksGpuTexture * texture, const ksGpuWindow * window, int index )
{
	assert( index >= 0 && index < (int)window->swapchain.imageCount );

	texture->width = window->swapchain.width;
	texture->height = window->swapchain.height;
	texture->depth = 1;
	texture->layerCount = 1;
	texture->mipCount = 1;
	texture->sampleCount = KS_GPU_SAMPLE_COUNT_1;
	texture->usage = KS_GPU_TEXTURE_USAGE_UNDEFINED;
	texture->usageFlags = KS_GPU_TEXTURE_USAGE_STORAGE | KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT | KS_GPU_TEXTURE_USAGE_PRESENTATION;
	texture->wrapMode = KS_GPU_TEXTURE_WRAP_MODE_REPEAT;
	texture->filter = KS_GPU_TEXTURE_FILTER_LINEAR;
	texture->maxAnisotropy = 1.0f;
	texture->format = window->swapchain.internalFormat;
	texture->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	texture->image = window->swapchain.images[index];
	texture->memory = VK_NULL_HANDLE;
	texture->view = window->swapchain.views[index];
	texture->sampler = VK_NULL_HANDLE;
	ksGpuTexture_UpdateSampler( context, texture );

	return true;
}

// KTX is a format for storing textures for OpenGL and OpenGL ES applications.
// https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
// This KTX loader does not do any conversions. In other words, the format
// stored in the KTX file must be the same as the glInternalFormat.
static bool ksGpuTexture_CreateFromKTX( ksGpuContext * context, ksGpuTexture * texture, const char * fileName,
									const unsigned char * buffer, const size_t bufferSize )
{
	memset( texture, 0, sizeof( ksGpuTexture ) );

#pragma pack(1)
	typedef struct
	{
		unsigned char	identifier[12];
		unsigned int	endianness;
		unsigned int	glType;
		unsigned int	glTypeSize;
		unsigned int	glFormat;
		unsigned int	glInternalFormat;
		unsigned int	glBaseInternalFormat;
		unsigned int	pixelWidth;
		unsigned int	pixelHeight;
		unsigned int	pixelDepth;
		unsigned int	numberOfArrayElements;
		unsigned int	numberOfFaces;
		unsigned int	numberOfMipmapLevels;
		unsigned int	bytesOfKeyValueData;
	} GlHeaderKTX_t;
#pragma pack()

	if ( bufferSize < sizeof( GlHeaderKTX_t ) )
	{
    	Error( "%s: Invalid KTX file", fileName );
        return false;
	}

	const unsigned char fileIdentifier[12] =
	{
		(unsigned char)'\xAB', 'K', 'T', 'X', ' ', '1', '1', (unsigned char)'\xBB', '\r', '\n', '\x1A', '\n'
	};

	const GlHeaderKTX_t * header = (GlHeaderKTX_t *)buffer;
	if ( memcmp( header->identifier, fileIdentifier, sizeof( fileIdentifier ) ) != 0 )
	{
		Error( "%s: Invalid KTX file", fileName );
		return false;
	}
	// only support little endian
	if ( header->endianness != 0x04030201 )
	{
		Error( "%s: KTX file has wrong endianess", fileName );
		return false;
	}
	// skip the key value data
	const size_t startTex = sizeof( GlHeaderKTX_t ) + header->bytesOfKeyValueData;
	if ( ( startTex < sizeof( GlHeaderKTX_t ) ) || ( startTex >= bufferSize ) )
	{
		Error( "%s: Invalid KTX header sizes", fileName );
		return false;
	}

	const unsigned int derivedFormat = glGetFormatFromInternalFormat( header->glInternalFormat );
	const unsigned int derivedType = glGetTypeFromInternalFormat( header->glInternalFormat );

	UNUSED_PARM( derivedFormat );
	UNUSED_PARM( derivedType );

	// The glFormat and glType must either both be zero or both be non-zero.
	assert( ( header->glFormat == 0 ) == ( header->glType == 0 ) );
	// Uncompressed glTypeSize must be 1, 2, 4 or 8.
	assert( header->glFormat == 0 || header->glTypeSize == 1 || header->glTypeSize == 2 || header->glTypeSize == 4 || header->glTypeSize == 8 );
	// Uncompressed glFormat must match the format derived from glInternalFormat.
	assert( header->glFormat == 0 || header->glFormat == derivedFormat );
	// Uncompressed glType must match the type derived from glInternalFormat.
	assert( header->glFormat == 0 || header->glType == derivedType );
	// Uncompressed glBaseInternalFormat must be the same as glFormat.
	assert( header->glFormat == 0 || header->glBaseInternalFormat == header->glFormat );
	// Compressed glTypeSize must be 1.
	assert( header->glFormat != 0 || header->glTypeSize == 1 );
	// Compressed glBaseInternalFormat must match the format drived from glInternalFormat.
	assert( header->glFormat != 0 || header->glBaseInternalFormat == derivedFormat );

	const int numberOfFaces = ( header->numberOfFaces >= 1 ) ? header->numberOfFaces : 1;
	const VkFormat format = vkGetFormatFromOpenGLInternalFormat( header->glInternalFormat );

	return ksGpuTexture_CreateInternal( context, texture, fileName,
									format, KS_GPU_SAMPLE_COUNT_1,
									header->pixelWidth, header->pixelHeight, header->pixelDepth,
									header->numberOfArrayElements, numberOfFaces, header->numberOfMipmapLevels,
									KS_GPU_TEXTURE_USAGE_SAMPLED, buffer + startTex, bufferSize - startTex, true );
}

static bool ksGpuTexture_CreateFromFile( ksGpuContext * context, ksGpuTexture * texture, const char * fileName )
{
	memset( texture, 0, sizeof( ksGpuTexture ) );

	FILE * fp = fopen( fileName, "rb" );
	if ( fp == NULL )
	{
		Error( "Failed to open %s", fileName );
		return false;
	}

	fseek( fp, 0L, SEEK_END );
	size_t bufferSize = ftell( fp );
	fseek( fp, 0L, SEEK_SET );

	unsigned char * buffer = (unsigned char *) malloc( bufferSize );
	if ( fread( buffer, 1, bufferSize, fp ) != bufferSize )
	{
		Error( "Failed to read %s", fileName );
		free( buffer );
		fclose( fp );
		return false;
	}
	fclose( fp );

	bool success = ksGpuTexture_CreateFromKTX( context, texture, fileName, buffer, bufferSize );

	free( buffer );

	return success;
}

static void ksGpuTexture_Destroy( ksGpuContext * context, ksGpuTexture * texture )
{
	VC( context->device->vkDestroySampler( context->device->device, texture->sampler, VK_ALLOCATOR ) );
	// A texture created from a swapchain does not own the view, image or memory.
	if ( texture->memory != VK_NULL_HANDLE )
	{
		VC( context->device->vkDestroyImageView( context->device->device, texture->view, VK_ALLOCATOR ) );
		VC( context->device->vkDestroyImage( context->device->device, texture->image, VK_ALLOCATOR ) );
		VC( context->device->vkFreeMemory( context->device->device, texture->memory, VK_ALLOCATOR ) );
	}
	memset( texture, 0, sizeof( ksGpuTexture ) );
}

static void ksGpuTexture_SetWrapMode( ksGpuContext * context, ksGpuTexture * texture, const ksGpuTextureWrapMode wrapMode )
{
	texture->wrapMode = wrapMode;

	ksGpuTexture_UpdateSampler( context, texture );
}

static void ksGpuTexture_SetFilter( ksGpuContext * context, ksGpuTexture * texture, const ksGpuTextureFilter filter )
{
	texture->filter = filter;

	ksGpuTexture_UpdateSampler( context, texture );
}

static void ksGpuTexture_SetAniso( ksGpuContext * context, ksGpuTexture * texture, const float maxAniso )
{
	texture->maxAnisotropy = maxAniso;

	ksGpuTexture_UpdateSampler( context, texture );
}

/*
================================================================================================================================

GPU indices and vertex attributes.

ksGpuTriangleIndex
ksGpuTriangleIndexArray
ksGpuVertexAttribute
ksGpuVertexAttributeArrays

================================================================================================================================
*/

typedef unsigned short ksGpuTriangleIndex;

typedef struct
{
	const ksGpuBuffer *		buffer;
	ksGpuTriangleIndex *	indexArray;
	int						indexCount;
} ksGpuTriangleIndexArray;

typedef enum
{
	KS_GPU_ATTRIBUTE_FORMAT_R32_SFLOAT			= VK_FORMAT_R32_SFLOAT,
	KS_GPU_ATTRIBUTE_FORMAT_R32G32_SFLOAT		= VK_FORMAT_R32G32_SFLOAT,
	KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT	= VK_FORMAT_R32G32B32_SFLOAT,
	KS_GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT	= VK_FORMAT_R32G32B32A32_SFLOAT
} ksGpuAttributeFormat;

typedef struct
{
	int						attributeFlag;		// VERTEX_ATTRIBUTE_FLAG_
	size_t					attributeOffset;	// Offset in bytes to the pointer in ksGpuVertexAttributeArrays
	size_t					attributeSize;		// Size in bytes of a single attribute
	ksGpuAttributeFormat	attributeFormat;	// Format of the attribute
	int						locationCount;		// Number of attribute locations
	const char *			name;				// Name in vertex program
} ksGpuVertexAttribute;

typedef struct
{
	const ksGpuBuffer *				buffer;
	const ksGpuVertexAttribute *	layout;
	void *							data;
	size_t							dataSize;
	int								vertexCount;
	int								attribsFlags;
} ksGpuVertexAttributeArrays;

static void ksGpuTriangleIndexArray_CreateFromBuffer( ksGpuTriangleIndexArray * indices, const int indexCount, const ksGpuBuffer * buffer )
{
	indices->indexCount = indexCount;
	indices->indexArray = NULL;
	indices->buffer = buffer;
}

static void ksGpuTriangleIndexArray_Alloc( ksGpuTriangleIndexArray * indices, const int indexCount, const ksGpuTriangleIndex * data )
{
	indices->indexCount = indexCount;
	indices->indexArray = (ksGpuTriangleIndex *) malloc( indexCount * sizeof( ksGpuTriangleIndex ) );
	if ( data != NULL )
	{
		memcpy( indices->indexArray, data, indexCount * sizeof( ksGpuTriangleIndex ) );
	}
	indices->buffer = NULL;
}

static void ksGpuTriangleIndexArray_Free( ksGpuTriangleIndexArray * indices )
{
	free( indices->indexArray );
	memset( indices, 0, sizeof( ksGpuTriangleIndexArray ) );
}

static size_t ksGpuVertexAttributeArrays_GetDataSize( const ksGpuVertexAttribute * layout, const int vertexCount, const int attribsFlags )
{
	size_t totalSize = 0;
	for ( int i = 0; layout[i].attributeFlag != 0; i++ )
	{
		const ksGpuVertexAttribute * v = &layout[i];
		if ( ( v->attributeFlag & attribsFlags ) != 0 )
		{
			totalSize += v->attributeSize;
		}
	}
	return vertexCount * totalSize;
}

static void ksGpuVertexAttributeArrays_Map( ksGpuVertexAttributeArrays * attribs, void * data, const size_t dataSize, const int vertexCount, const int attribsFlags )
{
	unsigned char * dataBytePtr = (unsigned char *) data;
	size_t offset = 0;

	for ( int i = 0; attribs->layout[i].attributeFlag != 0; i++ )
	{
		const ksGpuVertexAttribute * v = &attribs->layout[i];
		void ** attribPtr = (void **) ( ((char *)attribs) + v->attributeOffset );
		if ( ( v->attributeFlag & attribsFlags ) != 0 )
		{
			*attribPtr = ( dataBytePtr + offset );
			offset += vertexCount * v->attributeSize;
		}
		else
		{
			*attribPtr = NULL;
		}
	}

	assert( offset == dataSize );
	UNUSED_PARM( dataSize );
}

static void ksGpuVertexAttributeArrays_CreateFromBuffer( ksGpuVertexAttributeArrays * attribs, const ksGpuVertexAttribute * layout,
															const int vertexCount, const int attribsFlags, const ksGpuBuffer * buffer )
{
	attribs->buffer = buffer;
	attribs->layout = layout;
	attribs->data = NULL;
	attribs->dataSize = 0;
	attribs->vertexCount = vertexCount;
	attribs->attribsFlags = attribsFlags;
}

static void ksGpuVertexAttributeArrays_Alloc( ksGpuVertexAttributeArrays * attribs, const ksGpuVertexAttribute * layout, const int vertexCount, const int attribsFlags )
{
	const size_t dataSize = ksGpuVertexAttributeArrays_GetDataSize( layout, vertexCount, attribsFlags );
	void * data = malloc( dataSize );
	attribs->buffer = NULL;
	attribs->layout = layout;
	attribs->data = data;
	attribs->dataSize = dataSize;
	attribs->vertexCount = vertexCount;
	attribs->attribsFlags = attribsFlags;
	ksGpuVertexAttributeArrays_Map( attribs, data, dataSize, vertexCount, attribsFlags );
}

static void ksGpuVertexAttributeArrays_Free( ksGpuVertexAttributeArrays * attribs )
{
	free( attribs->data );
	memset( attribs, 0, sizeof( ksGpuVertexAttributeArrays ) );
}

static void * ksGpuVertexAttributeArrays_FindAtribute( ksGpuVertexAttributeArrays * attribs, const char * name, const ksGpuAttributeFormat format )
{
	for ( int i = 0; attribs->layout[i].attributeFlag != 0; i++ )
	{
		const ksGpuVertexAttribute * v = &attribs->layout[i];
		if ( v->attributeFormat == format && strcmp( v->name, name ) == 0 )
		{
			void ** attribPtr = (void **) ( ((char *)attribs) + v->attributeOffset );
			return *attribPtr;
		}
	}
	return NULL;
}

static void ksGpuVertexAttributeArrays_CalculateTangents( ksGpuVertexAttributeArrays * attribs,
														const ksGpuTriangleIndexArray * indices )
{
	ksVector3f * vertexPosition	= (ksVector3f *)ksGpuVertexAttributeArrays_FindAtribute( attribs, "vertexPosition", KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT );
	ksVector3f * vertexNormal	= (ksVector3f *)ksGpuVertexAttributeArrays_FindAtribute( attribs, "vertexNormal", KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT );
	ksVector3f * vertexTangent	= (ksVector3f *)ksGpuVertexAttributeArrays_FindAtribute( attribs, "vertexTangent", KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT );
	ksVector3f * vertexBinormal	= (ksVector3f *)ksGpuVertexAttributeArrays_FindAtribute( attribs, "vertexBinormal", KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT );
	ksVector2f * vertexUv0		= (ksVector2f *)ksGpuVertexAttributeArrays_FindAtribute( attribs, "vertexUv0", KS_GPU_ATTRIBUTE_FORMAT_R32G32_SFLOAT );

	if ( vertexPosition == NULL || vertexNormal == NULL || vertexTangent == NULL || vertexBinormal == NULL || vertexUv0 == NULL )
	{
		assert( false );
		return;
	}

	for ( int i = 0; i < attribs->vertexCount; i++ )
	{
		ksVector3f_Set( &vertexTangent[i], 0.0f );
		ksVector3f_Set( &vertexBinormal[i], 0.0f );
	}

	for ( int i = 0; i < indices->indexCount; i += 3 )
	{
		const ksGpuTriangleIndex * v = indices->indexArray + i;
		const ksVector3f * pos = vertexPosition;
		const ksVector2f * uv0 = vertexUv0;

		const ksVector3f delta0 = { pos[v[1]].x - pos[v[0]].x, pos[v[1]].y - pos[v[0]].y, pos[v[1]].z - pos[v[0]].z };
		const ksVector3f delta1 = { pos[v[2]].x - pos[v[1]].x, pos[v[2]].y - pos[v[1]].y, pos[v[2]].z - pos[v[1]].z };
		const ksVector3f delta2 = { pos[v[0]].x - pos[v[2]].x, pos[v[0]].y - pos[v[2]].y, pos[v[0]].z - pos[v[2]].z };

		const float l0 = delta0.x * delta0.x + delta0.y * delta0.y + delta0.z * delta0.z;
		const float l1 = delta1.x * delta1.x + delta1.y * delta1.y + delta1.z * delta1.z;
		const float l2 = delta2.x * delta2.x + delta2.y * delta2.y + delta2.z * delta2.z;

		const int i0 = ( l0 > l1 ) ? ( l0 > l2 ? 2 : 1 ) : ( l1 > l2 ? 0 : 1 );
		const int i1 = ( i0 + 1 ) % 3;
		const int i2 = ( i0 + 2 ) % 3;

		const ksVector3f d0 = { pos[v[i1]].x - pos[v[i0]].x, pos[v[i1]].y - pos[v[i0]].y, pos[v[i1]].z - pos[v[i0]].z };
		const ksVector3f d1 = { pos[v[i2]].x - pos[v[i0]].x, pos[v[i2]].y - pos[v[i0]].y, pos[v[i2]].z - pos[v[i0]].z };

		const ksVector2f s0 = { uv0[v[i1]].x - uv0[v[i0]].x, uv0[v[i1]].y - uv0[v[i0]].y };
		const ksVector2f s1 = { uv0[v[i2]].x - uv0[v[i0]].x, uv0[v[i2]].y - uv0[v[i0]].y };

		const float sign = ( s0.x * s1.y - s0.y * s1.x ) < 0.0f ? -1.0f : 1.0f;

		ksVector3f tangent  = { ( d0.x * s1.y - d1.x * s0.y ) * sign, ( d0.y * s1.y - d1.y * s0.y ) * sign, ( d0.z * s1.y - d1.z * s0.y ) * sign };
		ksVector3f binormal = { ( d1.x * s0.x - d0.x * s1.x ) * sign, ( d1.y * s0.x - d0.y * s1.x ) * sign, ( d1.z * s0.x - d0.z * s1.x ) * sign };

		ksVector3f_Normalize( &tangent );
		ksVector3f_Normalize( &binormal );

		for ( int j = 0; j < 3; j++ )
		{
			vertexTangent[v[j]].x += tangent.x;
			vertexTangent[v[j]].y += tangent.y;
			vertexTangent[v[j]].z += tangent.z;

			vertexBinormal[v[j]].x += binormal.x;
			vertexBinormal[v[j]].y += binormal.y;
			vertexBinormal[v[j]].z += binormal.z;
		}
	}

	for ( int i = 0; i < attribs->vertexCount; i++ )
	{
		ksVector3f_Normalize( &vertexTangent[i] );
		ksVector3f_Normalize( &vertexBinormal[i] );
	}
}

/*
================================================================================================================================

GPU default vertex attribute layout.

ksDefaultVertexAttributeFlags
ksDefaultVertexAttributeArrays

================================================================================================================================
*/

typedef enum
{
	VERTEX_ATTRIBUTE_FLAG_POSITION		= BIT( 0 ),		// vec3 vertexPosition
	VERTEX_ATTRIBUTE_FLAG_NORMAL		= BIT( 1 ),		// vec3 vertexNormal
	VERTEX_ATTRIBUTE_FLAG_TANGENT		= BIT( 2 ),		// vec3 vertexTangent
	VERTEX_ATTRIBUTE_FLAG_BINORMAL		= BIT( 3 ),		// vec3 vertexBinormal
	VERTEX_ATTRIBUTE_FLAG_COLOR			= BIT( 4 ),		// vec4 vertexColor
	VERTEX_ATTRIBUTE_FLAG_UV0			= BIT( 5 ),		// vec2 vertexUv0
	VERTEX_ATTRIBUTE_FLAG_UV1			= BIT( 6 ),		// vec2 vertexUv1
	VERTEX_ATTRIBUTE_FLAG_UV2			= BIT( 7 ),		// vec2 vertexUv2
	VERTEX_ATTRIBUTE_FLAG_JOINT_INDICES	= BIT( 8 ),		// vec4 jointIndices
	VERTEX_ATTRIBUTE_FLAG_JOINT_WEIGHTS	= BIT( 9 ),		// vec4 jointWeights
	VERTEX_ATTRIBUTE_FLAG_TRANSFORM		= BIT( 10 )		// mat4 vertexTransform (NOTE this mat4 takes up 4 attribute locations)
} ksDefaultVertexAttributeFlags;

typedef struct
{
	ksGpuVertexAttributeArrays	base;
	ksVector3f *				position;
	ksVector3f *				normal;
	ksVector3f *				tangent;
	ksVector3f *				binormal;
	ksVector4f *				color;
	ksVector2f *				uv0;
	ksVector2f *				uv1;
	ksVector2f *				uv2;
	ksVector4f *				jointIndices;
	ksVector4f *				jointWeights;
	ksMatrix4x4f *				transform;
} ksDefaultVertexAttributeArrays;

static const ksGpuVertexAttribute DefaultVertexAttributeLayout[] =
{
	{ VERTEX_ATTRIBUTE_FLAG_POSITION,		OFFSETOF_MEMBER( ksDefaultVertexAttributeArrays, position ),		SIZEOF_MEMBER( ksDefaultVertexAttributeArrays, position[0] ),		KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT,		1,	"vertexPosition" },
	{ VERTEX_ATTRIBUTE_FLAG_NORMAL,			OFFSETOF_MEMBER( ksDefaultVertexAttributeArrays, normal ),			SIZEOF_MEMBER( ksDefaultVertexAttributeArrays, normal[0] ),			KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT,		1,	"vertexNormal" },
	{ VERTEX_ATTRIBUTE_FLAG_TANGENT,		OFFSETOF_MEMBER( ksDefaultVertexAttributeArrays, tangent ),			SIZEOF_MEMBER( ksDefaultVertexAttributeArrays, tangent[0] ),		KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT,		1,	"vertexTangent" },
	{ VERTEX_ATTRIBUTE_FLAG_BINORMAL,		OFFSETOF_MEMBER( ksDefaultVertexAttributeArrays, binormal ),		SIZEOF_MEMBER( ksDefaultVertexAttributeArrays, binormal[0] ),		KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT,		1,	"vertexBinormal" },
	{ VERTEX_ATTRIBUTE_FLAG_COLOR,			OFFSETOF_MEMBER( ksDefaultVertexAttributeArrays, color ),			SIZEOF_MEMBER( ksDefaultVertexAttributeArrays, color[0] ),			KS_GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT,	1,	"vertexColor" },
	{ VERTEX_ATTRIBUTE_FLAG_UV0,			OFFSETOF_MEMBER( ksDefaultVertexAttributeArrays, uv0 ),				SIZEOF_MEMBER( ksDefaultVertexAttributeArrays, uv0[0] ),			KS_GPU_ATTRIBUTE_FORMAT_R32G32_SFLOAT,			1,	"vertexUv0" },
	{ VERTEX_ATTRIBUTE_FLAG_UV1,			OFFSETOF_MEMBER( ksDefaultVertexAttributeArrays, uv1 ),				SIZEOF_MEMBER( ksDefaultVertexAttributeArrays, uv1[0] ),			KS_GPU_ATTRIBUTE_FORMAT_R32G32_SFLOAT,			1,	"vertexUv1" },
	{ VERTEX_ATTRIBUTE_FLAG_UV2,			OFFSETOF_MEMBER( ksDefaultVertexAttributeArrays, uv2 ),				SIZEOF_MEMBER( ksDefaultVertexAttributeArrays, uv2[0] ),			KS_GPU_ATTRIBUTE_FORMAT_R32G32_SFLOAT,			1,	"vertexUv2" },
	{ VERTEX_ATTRIBUTE_FLAG_JOINT_INDICES,	OFFSETOF_MEMBER( ksDefaultVertexAttributeArrays, jointIndices ),	SIZEOF_MEMBER( ksDefaultVertexAttributeArrays, jointIndices[0] ),	KS_GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT,	1,	"vertexJointIndices" },
	{ VERTEX_ATTRIBUTE_FLAG_JOINT_WEIGHTS,	OFFSETOF_MEMBER( ksDefaultVertexAttributeArrays, jointWeights ),	SIZEOF_MEMBER( ksDefaultVertexAttributeArrays, jointWeights[0] ),	KS_GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT,	1,	"vertexJointWeights" },
	{ VERTEX_ATTRIBUTE_FLAG_TRANSFORM,		OFFSETOF_MEMBER( ksDefaultVertexAttributeArrays, transform ),		SIZEOF_MEMBER( ksDefaultVertexAttributeArrays, transform[0] ),		KS_GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT,	4,	"vertexTransform" },
	{ 0, 0, 0, 0, 0, "" }
};

/*
================================================================================================================================

GPU geometry.

For optimal performance geometry should only be created at load time, not at runtime.
The vertex, index and instance buffers are placed in device memory for optimal performance.
The vertex attributes are not packed. Each attribute is stored in a separate array for
optimal binning on tiling GPUs that only transform the vertex position for the binning pass.
Storing each attribute in a saparate array is preferred even on immediate-mode GPUs to avoid
wasting cache space for attributes that are not used by a particular vertex shader.

ksGpuGeometry

static void ksGpuGeometry_Create( ksGpuContext * context, ksGpuGeometry * geometry,
								const ksGpuVertexAttributeArrays * attribs,
								const ksGpuTriangleIndexArray * indices );
static void ksGpuGeometry_CreateQuad( ksGpuContext * context, ksGpuGeometry * geometry, const float offset, const float scale );
static void ksGpuGeometry_CreateCube( ksGpuContext * context, ksGpuGeometry * geometry, const float offset, const float scale );
static void ksGpuGeometry_CreateTorus( ksGpuContext * context, ksGpuGeometry * geometry, const int tesselation, const float offset, const float scale );
static void ksGpuGeometry_Destroy( ksGpuContext * context, ksGpuGeometry * geometry );

static void ksGpuGeometry_AddInstanceAttributes( ksGpuContext * context, ksGpuGeometry * geometry, const int numInstances, const int instanceAttribsFlags );

================================================================================================================================
*/

typedef struct
{
	const ksGpuVertexAttribute *	layout;
	int								vertexAttribsFlags;
	int								instanceAttribsFlags;
	int								vertexCount;
	int								instanceCount;
	int 							indexCount;
	ksGpuBuffer						vertexBuffer;
	ksGpuBuffer						instanceBuffer;
	ksGpuBuffer						indexBuffer;
} ksGpuGeometry;

static void ksGpuGeometry_Create( ksGpuContext * context, ksGpuGeometry * geometry,
								const ksGpuVertexAttributeArrays * attribs,
								const ksGpuTriangleIndexArray * indices )
{
	memset( geometry, 0, sizeof( ksGpuGeometry ) );

	geometry->layout = attribs->layout;
	geometry->vertexAttribsFlags = attribs->attribsFlags;
	geometry->vertexCount = attribs->vertexCount;
	geometry->indexCount = indices->indexCount;

	if ( attribs->buffer != NULL )
	{
		ksGpuBuffer_CreateReference( context, &geometry->vertexBuffer, attribs->buffer );
	}
	else
	{
		ksGpuBuffer_Create( context, &geometry->vertexBuffer, KS_GPU_BUFFER_TYPE_VERTEX, attribs->dataSize, attribs->data, false );
	}
	if ( indices->buffer != NULL )
	{
		ksGpuBuffer_CreateReference( context, &geometry->indexBuffer, indices->buffer );
	}
	else
	{
		ksGpuBuffer_Create( context, &geometry->indexBuffer, KS_GPU_BUFFER_TYPE_INDEX, indices->indexCount * sizeof( indices->indexArray[0] ), indices->indexArray, false );
	}
}

// The quad is centered about the origin and without offset/scale spans the [-1, 1] X-Y range.
static void ksGpuGeometry_CreateQuad( ksGpuContext * context, ksGpuGeometry * geometry, const float offset, const float scale )
{
	const ksVector3f quadPositions[4] =
	{
		{ -1.0f, -1.0f, 0.0f }, { +1.0f, -1.0f, 0.0f }, { +1.0f, +1.0f, 0.0f }, { -1.0f, +1.0f, 0.0f }
	};

	const ksVector3f quadNormals[4] =
	{
		{ 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }
	};

	const ksVector2f quadUvs[4] =
	{
		{ 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f },	{ 0.0f, 0.0f }
	};

	const ksGpuTriangleIndex quadIndices[6] =
	{
		 0,  1,  2,  2,  3,  0
	};

	ksDefaultVertexAttributeArrays quadAttributeArrays;
	ksGpuVertexAttributeArrays_Alloc( &quadAttributeArrays.base,
									DefaultVertexAttributeLayout, 4,
									VERTEX_ATTRIBUTE_FLAG_POSITION |
									VERTEX_ATTRIBUTE_FLAG_NORMAL |
									VERTEX_ATTRIBUTE_FLAG_TANGENT |
									VERTEX_ATTRIBUTE_FLAG_BINORMAL |
									VERTEX_ATTRIBUTE_FLAG_UV0 );

	for ( int i = 0; i < 4; i++ )
	{
		quadAttributeArrays.position[i].x = ( quadPositions[i].x + offset ) * scale;
		quadAttributeArrays.position[i].y = ( quadPositions[i].y + offset ) * scale;
		quadAttributeArrays.position[i].z = ( quadPositions[i].z + offset ) * scale;
		quadAttributeArrays.normal[i].x = quadNormals[i].x;
		quadAttributeArrays.normal[i].y = quadNormals[i].y;
		quadAttributeArrays.normal[i].z = quadNormals[i].z;
		quadAttributeArrays.uv0[i].x = quadUvs[i].x;
		quadAttributeArrays.uv0[i].y = quadUvs[i].y;
	}

	ksGpuTriangleIndexArray quadIndexArray;
	ksGpuTriangleIndexArray_Alloc( &quadIndexArray, 6, quadIndices );

	ksGpuVertexAttributeArrays_CalculateTangents( &quadAttributeArrays.base, &quadIndexArray );

	ksGpuGeometry_Create( context, geometry, &quadAttributeArrays.base, &quadIndexArray );

	ksGpuVertexAttributeArrays_Free( &quadAttributeArrays.base );
	ksGpuTriangleIndexArray_Free( &quadIndexArray );
}

// The cube is centered about the origin and without offset/scale spans the [-1, 1] X-Y-Z range.
static void ksGpuGeometry_CreateCube( ksGpuContext * context, ksGpuGeometry * geometry, const float offset, const float scale )
{
	const ksVector3f cubePositions[24] =
	{
		{ +1.0f, -1.0f, -1.0f }, { +1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, +1.0f }, { +1.0f, -1.0f, +1.0f },
		{ -1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, +1.0f }, { -1.0f, +1.0f, +1.0f }, { -1.0f, +1.0f, -1.0f },

		{ -1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, +1.0f }, { -1.0f, +1.0f, +1.0f },
		{ -1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, +1.0f }, { +1.0f, -1.0f, +1.0f }, { +1.0f, -1.0f, -1.0f },

		{ -1.0f, -1.0f, +1.0f }, { +1.0f, -1.0f, +1.0f }, { +1.0f, +1.0f, +1.0f }, { -1.0f, +1.0f, +1.0f },
		{ -1.0f, -1.0f, -1.0f }, { -1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, -1.0f }, { +1.0f, -1.0f, -1.0f }
	};

	const ksVector3f cubeNormals[24] =
	{
		{ +1.0f, 0.0f, 0.0f }, { +1.0f, 0.0f, 0.0f }, { +1.0f, 0.0f, 0.0f }, { +1.0f, 0.0f, 0.0f },
		{ -1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f },

		{ 0.0f, +1.0f, 0.0f }, { 0.0f, +1.0f, 0.0f }, { 0.0f, +1.0f, 0.0f }, { 0.0f, +1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f },

		{ 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f },
		{ 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }
	};

	const ksVector2f cubeUvs[24] =
	{
		{ 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f },
		{ 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f },	{ 0.0f, 1.0f },

		{ 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f },	{ 0.0f, 0.0f },
		{ 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f },	{ 0.0f, 1.0f },

		{ 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f },	{ 0.0f, 0.0f },
		{ 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f },	{ 0.0f, 1.0f },
	};

	const ksGpuTriangleIndex cubeIndices[36] =
	{
		 0,  1,  2,  2,  3,  0,
		 4,  5,  6,  6,  7,  4,
		 8, 10,  9, 10,  8, 11,
		12, 14, 13, 14, 12, 15,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20
	};

	ksDefaultVertexAttributeArrays cubeAttributeArrays;
	ksGpuVertexAttributeArrays_Alloc( &cubeAttributeArrays.base,
									DefaultVertexAttributeLayout, 24,
									VERTEX_ATTRIBUTE_FLAG_POSITION |
									VERTEX_ATTRIBUTE_FLAG_NORMAL |
									VERTEX_ATTRIBUTE_FLAG_TANGENT |
									VERTEX_ATTRIBUTE_FLAG_BINORMAL |
									VERTEX_ATTRIBUTE_FLAG_UV0 );

	for ( int i = 0; i < 24; i++ )
	{
		cubeAttributeArrays.position[i].x = ( cubePositions[i].x + offset ) * scale;
		cubeAttributeArrays.position[i].y = ( cubePositions[i].y + offset ) * scale;
		cubeAttributeArrays.position[i].z = ( cubePositions[i].z + offset ) * scale;
		cubeAttributeArrays.normal[i].x = cubeNormals[i].x;
		cubeAttributeArrays.normal[i].y = cubeNormals[i].y;
		cubeAttributeArrays.normal[i].z = cubeNormals[i].z;
		cubeAttributeArrays.uv0[i].x = cubeUvs[i].x;
		cubeAttributeArrays.uv0[i].y = cubeUvs[i].y;
	}

	ksGpuTriangleIndexArray cubeIndexArray;
	ksGpuTriangleIndexArray_Alloc( &cubeIndexArray, 36, cubeIndices );

	ksGpuVertexAttributeArrays_CalculateTangents( &cubeAttributeArrays.base, &cubeIndexArray );

	ksGpuGeometry_Create( context, geometry, &cubeAttributeArrays.base, &cubeIndexArray );

	ksGpuVertexAttributeArrays_Free( &cubeAttributeArrays.base );
	ksGpuTriangleIndexArray_Free( &cubeIndexArray );
}

// The torus is centered about the origin and without offset/scale spans the [-1, 1] X-Y range and the [-0.3, 0.3] Z range.
static void ksGpuGeometry_CreateTorus( ksGpuContext * context, ksGpuGeometry * geometry, const int tesselation, const float offset, const float scale )
{
	const int minorTesselation = tesselation;
	const int majorTesselation = tesselation;
	const float tubeRadius = 0.3f;
	const float tubeCenter = 0.7f;
	const int vertexCount = ( majorTesselation + 1 ) * ( minorTesselation + 1 );
	const int indexCount = majorTesselation * minorTesselation * 6;

	ksDefaultVertexAttributeArrays torusAttributeArrays;
	ksGpuVertexAttributeArrays_Alloc( &torusAttributeArrays.base,
									DefaultVertexAttributeLayout, vertexCount,
									VERTEX_ATTRIBUTE_FLAG_POSITION |
									VERTEX_ATTRIBUTE_FLAG_NORMAL |
									VERTEX_ATTRIBUTE_FLAG_TANGENT |
									VERTEX_ATTRIBUTE_FLAG_BINORMAL |
									VERTEX_ATTRIBUTE_FLAG_UV0 );

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
			torusAttributeArrays.position[index].x = ( minorX * majorCos * scale ) + offset;
			torusAttributeArrays.position[index].y = ( minorX * majorSin * scale ) + offset;
			torusAttributeArrays.position[index].z = ( minorZ * scale ) + offset;
			torusAttributeArrays.normal[index].x = minorCos * majorCos;
			torusAttributeArrays.normal[index].y = minorCos * majorSin;
			torusAttributeArrays.normal[index].z = minorSin;
			torusAttributeArrays.uv0[index].x = (float) u / majorTesselation;
			torusAttributeArrays.uv0[index].y = (float) v / minorTesselation;
		}
	}

	ksGpuTriangleIndexArray torusIndexArray;
	ksGpuTriangleIndexArray_Alloc( &torusIndexArray, indexCount, NULL );

	for ( int u = 0; u < majorTesselation; u++ )
	{
		for ( int v = 0; v < minorTesselation; v++ )
		{
			const int index = ( u * minorTesselation + v ) * 6;
			torusIndexArray.indexArray[index + 0] = (ksGpuTriangleIndex)( ( u + 0 ) * ( minorTesselation + 1 ) + ( v + 0 ) );
			torusIndexArray.indexArray[index + 1] = (ksGpuTriangleIndex)( ( u + 1 ) * ( minorTesselation + 1 ) + ( v + 0 ) );
			torusIndexArray.indexArray[index + 2] = (ksGpuTriangleIndex)( ( u + 1 ) * ( minorTesselation + 1 ) + ( v + 1 ) );
			torusIndexArray.indexArray[index + 3] = (ksGpuTriangleIndex)( ( u + 1 ) * ( minorTesselation + 1 ) + ( v + 1 ) );
			torusIndexArray.indexArray[index + 4] = (ksGpuTriangleIndex)( ( u + 0 ) * ( minorTesselation + 1 ) + ( v + 1 ) );
			torusIndexArray.indexArray[index + 5] = (ksGpuTriangleIndex)( ( u + 0 ) * ( minorTesselation + 1 ) + ( v + 0 ) );
		}
	}

	ksGpuVertexAttributeArrays_CalculateTangents( &torusAttributeArrays.base, &torusIndexArray );

	ksGpuGeometry_Create( context, geometry, &torusAttributeArrays.base, &torusIndexArray );

	ksGpuVertexAttributeArrays_Free( &torusAttributeArrays.base );
	ksGpuTriangleIndexArray_Free( &torusIndexArray );
}

static void ksGpuGeometry_Destroy( ksGpuContext * context, ksGpuGeometry * geometry )
{
	ksGpuBuffer_Destroy( context, &geometry->indexBuffer );
	ksGpuBuffer_Destroy( context, &geometry->vertexBuffer );
	if ( geometry->instanceBuffer.size != 0 )
	{
		ksGpuBuffer_Destroy( context, &geometry->instanceBuffer );
	}

	memset( geometry, 0, sizeof( ksGpuGeometry ) );
}

static void ksGpuGeometry_AddInstanceAttributes( ksGpuContext * context, ksGpuGeometry * geometry, const int numInstances, const int instanceAttribsFlags )
{
	assert( geometry->layout != NULL );
	assert( ( geometry->vertexAttribsFlags & instanceAttribsFlags ) == 0 );

	geometry->instanceCount = numInstances;
	geometry->instanceAttribsFlags = instanceAttribsFlags;

	const size_t dataSize = ksGpuVertexAttributeArrays_GetDataSize( geometry->layout, numInstances, geometry->instanceAttribsFlags );

	ksGpuBuffer_Create( context, &geometry->instanceBuffer, KS_GPU_BUFFER_TYPE_VERTEX, dataSize, NULL, false );
}

/*
================================================================================================================================

GPU render pass.

A render pass encapsulates a sequence of graphics commands that can be executed in a single tiling pass.
For optimal performance a render pass should only be created at load time, not at runtime.
Render passes cannot overlap and cannot be nested.

ksGpuRenderPassType
ksGpuRenderPassFlags
ksGpuRenderPass

static bool ksGpuRenderPass_Create( ksGpuContext * context, ksGpuRenderPass * renderPass,
									const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
									const ksGpuSampleCount sampleCount, const ksGpuRenderPassType type, const uint32_t flags );
static void ksGpuRenderPass_Destroy( ksGpuContext * context, ksGpuRenderPass * renderPass );

================================================================================================================================
*/

#define EXPLICIT_RESOLVE		0

typedef enum
{
	KS_GPU_RENDERPASS_TYPE_INLINE,
	KS_GPU_RENDERPASS_TYPE_SECONDARY_COMMAND_BUFFERS
} ksGpuRenderPassType;

typedef enum
{
	KS_GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER		= BIT( 0 ),
	KS_GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER		= BIT( 1 )
} ksGpuRenderPassFlags;

typedef struct
{
	ksGpuRenderPassType			type;
	int							flags;
	ksGpuSurfaceColorFormat		colorFormat;
	ksGpuSurfaceDepthFormat		depthFormat;
	ksGpuSampleCount			sampleCount;
	VkFormat					internalColorFormat;
	VkFormat					internalDepthFormat;
	VkRenderPass				renderPass;
} ksGpuRenderPass;

static bool ksGpuRenderPass_Create( ksGpuContext * context, ksGpuRenderPass * renderPass,
									const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
									const ksGpuSampleCount sampleCount, const ksGpuRenderPassType type, const int flags )
{
	assert( ( context->device->physicalDeviceProperties.limits.framebufferColorSampleCounts & (VkSampleCountFlags) sampleCount ) != 0 );
	assert( ( context->device->physicalDeviceProperties.limits.framebufferDepthSampleCounts & (VkSampleCountFlags) sampleCount ) != 0 );

	renderPass->type = type;
	renderPass->flags = flags;
	renderPass->colorFormat = colorFormat;
	renderPass->depthFormat = depthFormat;
	renderPass->sampleCount = sampleCount;
	renderPass->internalColorFormat = ksGpuSwapchain_InternalSurfaceColorFormat( colorFormat );
	renderPass->internalDepthFormat = ksGpuDepthBuffer_InternalSurfaceDepthFormat( depthFormat );

	uint32_t attachmentCount = 0;
	VkAttachmentDescription attachments[3];

	// Optionally use a multi-sampled attachment.
	if ( sampleCount > KS_GPU_SAMPLE_COUNT_1 )
	{
		attachments[attachmentCount].flags = 0;
		attachments[attachmentCount].format = renderPass->internalColorFormat;
		attachments[attachmentCount].samples = (VkSampleCountFlagBits)sampleCount;
		attachments[attachmentCount].loadOp = ( ( flags & KS_GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER ) != 0 ) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[attachmentCount].storeOp = ( EXPLICIT_RESOLVE != 0 ) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[attachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[attachmentCount].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[attachmentCount].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachmentCount++;
	}
	// Either render directly to, or resolve to the single-sample attachment.
	if ( sampleCount <= KS_GPU_SAMPLE_COUNT_1 || EXPLICIT_RESOLVE == 0 )
	{
		attachments[attachmentCount].flags = 0;
		attachments[attachmentCount].format = renderPass->internalColorFormat;
		attachments[attachmentCount].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[attachmentCount].loadOp = ( ( flags & KS_GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER ) != 0 && sampleCount <= KS_GPU_SAMPLE_COUNT_1 ) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[attachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[attachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[attachmentCount].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[attachmentCount].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachmentCount++;
	}
	// Optionally use a depth buffer.
	if ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED )
	{
		attachments[attachmentCount].flags = 0;
		attachments[attachmentCount].format = renderPass->internalDepthFormat;
		attachments[attachmentCount].samples = (VkSampleCountFlagBits)sampleCount;
		attachments[attachmentCount].loadOp = ( ( flags & KS_GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER ) != 0 ) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[attachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[attachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[attachmentCount].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[attachmentCount].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachmentCount++;
	}

	VkAttachmentReference colorAttachmentReference;
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference resolveAttachmentReference;
	resolveAttachmentReference.attachment = 1;
	resolveAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference;
	depthAttachmentReference.attachment = ( sampleCount > KS_GPU_SAMPLE_COUNT_1 && EXPLICIT_RESOLVE == 0 ) ? 2 : 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription;
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = NULL;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;
	subpassDescription.pResolveAttachments = ( sampleCount > KS_GPU_SAMPLE_COUNT_1 && EXPLICIT_RESOLVE == 0 ) ? &resolveAttachmentReference : NULL;
	subpassDescription.pDepthStencilAttachment = ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED ) ? &depthAttachmentReference : NULL;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = NULL;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = attachmentCount;
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 0;
	renderPassCreateInfo.pDependencies = NULL;

	VK( context->device->vkCreateRenderPass( context->device->device, &renderPassCreateInfo, VK_ALLOCATOR, &renderPass->renderPass ) );

	return true;
}

static void ksGpuRenderPass_Destroy( ksGpuContext * context, ksGpuRenderPass * renderPass )
{
	VC( context->device->vkDestroyRenderPass( context->device->device, renderPass->renderPass, VK_ALLOCATOR ) );
}

/*
================================================================================================================================

GPU framebuffer.

A framebuffer encapsulates either a swapchain or a buffered set of textures.
For optimal performance a framebuffer should only be created at load time, not at runtime.

ksGpuFramebuffer

static bool ksGpuFramebuffer_CreateFromSwapchain( ksGpuWindow * window, ksGpuFramebuffer * framebuffer, ksGpuRenderPass * renderPass );
static bool ksGpuFramebuffer_CreateFromTextures( ksGpuContext * context, ksGpuFramebuffer * framebuffer, ksGpuRenderPass * renderPass,
												const int width, const int height, const int numBuffers );
static bool ksGpuFramebuffer_CreateFromTextureArrays( ksGpuContext * context, ksGpuFramebuffer * framebuffer, ksGpuRenderPass * renderPass,
												const int width, const int height, const int numLayers, const int numBuffers, const bool multiview );
static void ksGpuFramebuffer_Destroy( ksGpuContext * context, ksGpuFramebuffer * framebuffer );

static int ksGpuFramebuffer_GetWidth( const ksGpuFramebuffer * framebuffer );
static int ksGpuFramebuffer_GetHeight( const ksGpuFramebuffer * framebuffer );
static ksScreenRect ksGpuFramebuffer_GetRect( const ksGpuFramebuffer * framebuffer );
static int ksGpuFramebuffer_GetBufferCount( const ksGpuFramebuffer * framebuffer );
static ksGpuTexture * ksGpuFramebuffer_GetColorTexture( const ksGpuFramebuffer * framebuffer );

================================================================================================================================
*/

typedef struct
{
	ksGpuTexture *		colorTextures;
	ksGpuTexture		renderTexture;
	ksGpuDepthBuffer	depthBuffer;
	VkImageView *		textureViews;
	VkImageView *		renderViews;
	VkFramebuffer *		framebuffers;
	ksGpuRenderPass *	renderPass;
	ksGpuWindow *		window;
	int					swapchainCreateCount;
	int					width;
	int					height;
	int					numLayers;
	int					numBuffers;
	int					currentBuffer;
	int					currentLayer;
} ksGpuFramebuffer;

static bool ksGpuFramebuffer_CreateFromSwapchain( ksGpuWindow * window, ksGpuFramebuffer * framebuffer, ksGpuRenderPass * renderPass )
{
	assert( window->windowWidth >= 1 && window->windowWidth <= (int)window->context.device->physicalDeviceProperties.limits.maxFramebufferWidth );
	assert( window->windowHeight >= 1 && window->windowHeight <= (int)window->context.device->physicalDeviceProperties.limits.maxFramebufferHeight );
	assert( window->sampleCount == renderPass->sampleCount );

	memset( framebuffer, 0, sizeof( ksGpuFramebuffer ) );

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

	framebuffer->colorTextures = (ksGpuTexture *) malloc( window->swapchain.imageCount * sizeof( ksGpuTexture ) );
	framebuffer->textureViews = NULL;
	framebuffer->renderViews = NULL;
	framebuffer->framebuffers = (VkFramebuffer *) malloc( window->swapchain.imageCount * sizeof( VkFramebuffer ) );
	framebuffer->numBuffers = window->swapchain.imageCount;

	if ( renderPass->sampleCount > KS_GPU_SAMPLE_COUNT_1 )
	{
		ksGpuTexture_Create2D( &window->context, &framebuffer->renderTexture, (ksGpuTextureFormat)renderPass->internalColorFormat, renderPass->sampleCount,
			window->windowWidth, window->windowHeight, 1, KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT, NULL, 0 );
		ksGpuContext_CreateSetupCmdBuffer( &window->context );
		ksGpuTexture_ChangeUsage( &window->context, window->context.setupCommandBuffer, &framebuffer->renderTexture, KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );
		ksGpuContext_FlushSetupCmdBuffer( &window->context );
	}

	for ( uint32_t imageIndex = 0; imageIndex < window->swapchain.imageCount; imageIndex++ )
	{
		assert( renderPass->colorFormat == window->colorFormat );
		assert( renderPass->depthFormat == window->depthFormat );

		ksGpuTexture_CreateFromSwapchain( &window->context, &framebuffer->colorTextures[imageIndex], window, imageIndex );

		assert( window->windowWidth == framebuffer->colorTextures[imageIndex].width );
		assert( window->windowHeight == framebuffer->colorTextures[imageIndex].height );

		uint32_t attachmentCount = 0;
		VkImageView attachments[3];

		if ( renderPass->sampleCount > KS_GPU_SAMPLE_COUNT_1 )
		{
			attachments[attachmentCount++] = framebuffer->renderTexture.view;
		}
		if ( renderPass->sampleCount <= KS_GPU_SAMPLE_COUNT_1 || EXPLICIT_RESOLVE == 0 )
		{
			attachments[attachmentCount++] = framebuffer->colorTextures[imageIndex].view;
		}
		if ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED )
		{
			attachments[attachmentCount++] = window->depthBuffer.views[0];
		}

		VkFramebufferCreateInfo framebufferCreateInfo;
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.pNext = NULL;
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.renderPass = renderPass->renderPass;
		framebufferCreateInfo.attachmentCount = attachmentCount;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = window->windowWidth;
		framebufferCreateInfo.height = window->windowHeight;
		framebufferCreateInfo.layers = 1;

		VK( window->context.device->vkCreateFramebuffer( window->context.device->device, &framebufferCreateInfo, VK_ALLOCATOR, &framebuffer->framebuffers[imageIndex] ) );
	}

	return true;
}

static bool ksGpuFramebuffer_CreateFromTextures( ksGpuContext * context, ksGpuFramebuffer * framebuffer, ksGpuRenderPass * renderPass,
												const int width, const int height, const int numBuffers )
{
	assert( width >= 1 && width <= (int)context->device->physicalDeviceProperties.limits.maxFramebufferWidth );
	assert( height >= 1 && height <= (int)context->device->physicalDeviceProperties.limits.maxFramebufferHeight );

	memset( framebuffer, 0, sizeof( ksGpuFramebuffer ) );

	framebuffer->colorTextures = (ksGpuTexture *) malloc( numBuffers * sizeof( ksGpuTexture ) );
	framebuffer->textureViews = NULL;
	framebuffer->renderViews = NULL;
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

	for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
	{
		ksGpuTexture_Create2D( context, &framebuffer->colorTextures[bufferIndex], (ksGpuTextureFormat)renderPass->internalColorFormat, KS_GPU_SAMPLE_COUNT_1,
			width, height, 1, KS_GPU_TEXTURE_USAGE_SAMPLED | KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT | KS_GPU_TEXTURE_USAGE_STORAGE, NULL, 0 );
		ksGpuTexture_SetWrapMode( context, &framebuffer->colorTextures[bufferIndex], KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER );
	}

	if ( renderPass->sampleCount > KS_GPU_SAMPLE_COUNT_1 )
	{
		ksGpuTexture_Create2D( context, &framebuffer->renderTexture, (ksGpuTextureFormat)renderPass->internalColorFormat, renderPass->sampleCount,
			width, height, 1, KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT, NULL, 0 );
		ksGpuContext_CreateSetupCmdBuffer( context );
		ksGpuTexture_ChangeUsage( context, context->setupCommandBuffer, &framebuffer->renderTexture, KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );
		ksGpuContext_FlushSetupCmdBuffer( context );
	}

	if ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED )
	{
		ksGpuDepthBuffer_Create( context, &framebuffer->depthBuffer, renderPass->depthFormat, renderPass->sampleCount, width, height, 1 );
	}

	for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
	{
		uint32_t attachmentCount = 0;
		VkImageView attachments[3];

		if ( renderPass->sampleCount > KS_GPU_SAMPLE_COUNT_1 )
		{
			attachments[attachmentCount++] = framebuffer->renderTexture.view;
		}
		if ( renderPass->sampleCount <= KS_GPU_SAMPLE_COUNT_1 || EXPLICIT_RESOLVE == 0 )
		{
			attachments[attachmentCount++] = framebuffer->colorTextures[bufferIndex].view;
		}
		if ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED )
		{
			attachments[attachmentCount++] = framebuffer->depthBuffer.views[0];
		}

		VkFramebufferCreateInfo framebufferCreateInfo;
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.pNext = NULL;
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.renderPass = renderPass->renderPass;
		framebufferCreateInfo.attachmentCount = attachmentCount;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = width;
		framebufferCreateInfo.height = height;
		framebufferCreateInfo.layers = 1;

		VK( context->device->vkCreateFramebuffer( context->device->device, &framebufferCreateInfo, VK_ALLOCATOR, &framebuffer->framebuffers[bufferIndex] ) );
	}

	return true;
}

static bool ksGpuFramebuffer_CreateFromTextureArrays( ksGpuContext * context, ksGpuFramebuffer * framebuffer, ksGpuRenderPass * renderPass,
												const int width, const int height, const int numLayers, const int numBuffers, const bool multiview )
{
	UNUSED_PARM( multiview );

	assert( width >= 1 && width <= (int)context->device->physicalDeviceProperties.limits.maxFramebufferWidth );
	assert( height >= 1 && height <= (int)context->device->physicalDeviceProperties.limits.maxFramebufferHeight );
	assert( numLayers >= 1 && numLayers <= (int)context->device->physicalDeviceProperties.limits.maxFramebufferLayers );

	memset( framebuffer, 0, sizeof( ksGpuFramebuffer ) );

	framebuffer->colorTextures = (ksGpuTexture *) malloc( numBuffers * sizeof( ksGpuTexture ) );
	framebuffer->textureViews = NULL;
	framebuffer->renderViews = NULL;
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

	for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
	{
		ksGpuTexture_Create2DArray( context, &framebuffer->colorTextures[bufferIndex], (ksGpuTextureFormat)renderPass->internalColorFormat, KS_GPU_SAMPLE_COUNT_1,
			width, height, numLayers, 1, KS_GPU_TEXTURE_USAGE_SAMPLED | KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT | KS_GPU_TEXTURE_USAGE_STORAGE, NULL, 0 );
		ksGpuTexture_SetWrapMode( context, &framebuffer->colorTextures[bufferIndex], KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER );
	}

	if ( renderPass->sampleCount <= KS_GPU_SAMPLE_COUNT_1 || EXPLICIT_RESOLVE == 0 )
	{
		framebuffer->textureViews = (VkImageView *) malloc( numBuffers * numLayers * sizeof( VkImageView ) );
	}

	if ( renderPass->sampleCount > KS_GPU_SAMPLE_COUNT_1 )
	{
		framebuffer->renderViews = (VkImageView *) malloc( numBuffers * numLayers * sizeof( VkImageView ) );

		ksGpuTexture_Create2DArray( context, &framebuffer->renderTexture, (ksGpuTextureFormat)renderPass->internalColorFormat, renderPass->sampleCount,
			width, height, numLayers, 1, KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT, NULL, 0 );
		ksGpuContext_CreateSetupCmdBuffer( context );
		ksGpuTexture_ChangeUsage( context, context->setupCommandBuffer, &framebuffer->renderTexture, KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );
		ksGpuContext_FlushSetupCmdBuffer( context );
	}

	if ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED )
	{
		// Note: share a single depth buffer between all array layers.
		ksGpuDepthBuffer_Create( context, &framebuffer->depthBuffer, renderPass->depthFormat, renderPass->sampleCount, width, height, 1 );
	}

	for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
	{
		for ( int layerIndex = 0; layerIndex < numLayers; layerIndex++ )
		{
			uint32_t attachmentCount = 0;
			VkImageView attachments[3];

			if ( renderPass->sampleCount > KS_GPU_SAMPLE_COUNT_1 )
			{
				// Create a view for a single array layer.
				VkImageViewCreateInfo imageViewCreateInfo;
				imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewCreateInfo.pNext = NULL;
				imageViewCreateInfo.flags = 0;
				imageViewCreateInfo.image = framebuffer->renderTexture.image;
				imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageViewCreateInfo.format = framebuffer->renderTexture.format;
				imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
				imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
				imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
				imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
				imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
				imageViewCreateInfo.subresourceRange.levelCount = 1;
				imageViewCreateInfo.subresourceRange.baseArrayLayer = layerIndex;
				imageViewCreateInfo.subresourceRange.layerCount = 1;

				VK( context->device->vkCreateImageView( context->device->device, &imageViewCreateInfo, VK_ALLOCATOR, &framebuffer->renderViews[bufferIndex * numLayers + layerIndex] ) );

				attachments[attachmentCount++] = framebuffer->renderViews[bufferIndex * numLayers + layerIndex];
			}
			if ( renderPass->sampleCount <= KS_GPU_SAMPLE_COUNT_1 || EXPLICIT_RESOLVE == 0 )
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

				VK( context->device->vkCreateImageView( context->device->device, &imageViewCreateInfo, VK_ALLOCATOR, &framebuffer->textureViews[bufferIndex * numLayers + layerIndex] ) );

				attachments[attachmentCount++] = framebuffer->textureViews[bufferIndex * numLayers + layerIndex];
			}
			if ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED )
			{
				attachments[attachmentCount++] = framebuffer->depthBuffer.views[0];
			}

			// Create a framebuffer for a single array layer.
			VkFramebufferCreateInfo framebufferCreateInfo;
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.pNext = NULL;
			framebufferCreateInfo.flags = 0;
			framebufferCreateInfo.renderPass = renderPass->renderPass;
			framebufferCreateInfo.attachmentCount = attachmentCount;
			framebufferCreateInfo.pAttachments = attachments;
			framebufferCreateInfo.width = width;
			framebufferCreateInfo.height = height;
			framebufferCreateInfo.layers = 1;

			VK( context->device->vkCreateFramebuffer( context->device->device, &framebufferCreateInfo, VK_ALLOCATOR, &framebuffer->framebuffers[bufferIndex * numLayers + layerIndex] ) );
		}
	}

	return true;
}

static void ksGpuFramebuffer_Destroy( ksGpuContext * context, ksGpuFramebuffer * framebuffer )
{
	for ( int bufferIndex = 0; bufferIndex < framebuffer->numBuffers; bufferIndex++ )
	{
		for ( int layerIndex = 0; layerIndex < framebuffer->numLayers; layerIndex++ )
		{
			if ( framebuffer->framebuffers != NULL )
			{
				VC( context->device->vkDestroyFramebuffer( context->device->device, framebuffer->framebuffers[bufferIndex * framebuffer->numLayers + layerIndex], VK_ALLOCATOR ) );
			}
			if ( framebuffer->textureViews != NULL )
			{
				VC( context->device->vkDestroyImageView( context->device->device, framebuffer->textureViews[bufferIndex * framebuffer->numLayers + layerIndex], VK_ALLOCATOR ) );
			}
			if ( framebuffer->renderViews != NULL )
			{
				VC( context->device->vkDestroyImageView( context->device->device, framebuffer->renderViews[bufferIndex * framebuffer->numLayers + layerIndex], VK_ALLOCATOR ) );
			}
		}
	}
	if ( framebuffer->depthBuffer.image != VK_NULL_HANDLE )
	{
		ksGpuDepthBuffer_Destroy( context, &framebuffer->depthBuffer );
	}
	if ( framebuffer->renderTexture.image != VK_NULL_HANDLE )
	{
		ksGpuTexture_Destroy( context, &framebuffer->renderTexture );
	}
	for ( int bufferIndex = 0; bufferIndex < framebuffer->numBuffers; bufferIndex++ )
	{
		if ( framebuffer->colorTextures != NULL )
		{
			ksGpuTexture_Destroy( context, &framebuffer->colorTextures[bufferIndex] );
		}
	}

	free( framebuffer->framebuffers );
	free( framebuffer->renderViews );
	free( framebuffer->textureViews );
	free( framebuffer->colorTextures );

	memset( framebuffer, 0, sizeof( ksGpuFramebuffer ) );
}

static int ksGpuFramebuffer_GetWidth( const ksGpuFramebuffer * framebuffer )
{
	return framebuffer->width;
}

static int ksGpuFramebuffer_GetHeight( const ksGpuFramebuffer * framebuffer )
{
	return framebuffer->height;
}

static ksScreenRect ksGpuFramebuffer_GetRect( const ksGpuFramebuffer * framebuffer )
{
	ksScreenRect rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = framebuffer->width;
	rect.height = framebuffer->height;
	return rect;
}

static int ksGpuFramebuffer_GetBufferCount( const ksGpuFramebuffer * framebuffer )
{
	return framebuffer->numBuffers;
}

static ksGpuTexture * ksGpuFramebuffer_GetColorTexture( const ksGpuFramebuffer * framebuffer )
{
	assert( framebuffer->colorTextures != NULL );
	return &framebuffer->colorTextures[framebuffer->currentBuffer];
}

/*
================================================================================================================================

GPU program parms and layout.

ksGpuProgramStageFlags
ksGpuProgramParmType
ksGpuProgramParmAccess
ksGpuProgramParm
ksGpuProgramParmLayout

static void ksGpuProgramParmLayout_Create( ksGpuContext * context, ksGpuProgramParmLayout * layout,
										const ksGpuProgramParm * parms, const int numParms );
static void ksGpuProgramParmLayout_Destroy( ksGpuContext * context, ksGpuProgramParmLayout * layout );

================================================================================================================================
*/

#define MAX_PROGRAM_PARMS			16

typedef enum
{
	KS_GPU_PROGRAM_STAGE_FLAG_VERTEX		= BIT( 0 ),
	KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT		= BIT( 1 ),
	KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE		= BIT( 2 ),
	KS_GPU_PROGRAM_STAGE_MAX				= 3
} ksGpuProgramStageFlags;

typedef enum
{
	KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				// texture plus sampler bound together		(GLSL: sampler*, isampler*, usampler*)
	KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,				// not sampled, direct read-write storage	(GLSL: image*, iimage*, uimage*)
	KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM,				// read-only uniform buffer					(GLSL: uniform)
	KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE,				// read-write storage buffer				(GLSL: buffer)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				// int										(GLSL: int) 
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,		// int[2]									(GLSL: ivec2)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3,		// int[3]									(GLSL: ivec3)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4,		// int[4]									(GLSL: ivec4)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT,			// float									(GLSL: float)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	// float[2]									(GLSL: vec2)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3,	// float[3]									(GLSL: vec3)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4,	// float[4]									(GLSL: vec4)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2,	// float[2][2]								(GLSL: mat2x2 or mat2)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3,	// float[2][3]								(GLSL: mat2x3)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4,	// float[2][4]								(GLSL: mat2x4)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2,	// float[3][2]								(GLSL: mat3x2)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3,	// float[3][3]								(GLSL: mat3x3 or mat3)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	// float[3][4]								(GLSL: mat3x4)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2,	// float[4][2]								(GLSL: mat4x2)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3,	// float[4][3]								(GLSL: mat4x3)
	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	// float[4][4]								(GLSL: mat4x4 or mat4)
	KS_GPU_PROGRAM_PARM_TYPE_MAX
} ksGpuProgramParmType;

typedef enum
{
	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,
	KS_GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,
	KS_GPU_PROGRAM_PARM_ACCESS_READ_WRITE
} ksGpuProgramParmAccess;

typedef struct
{
	int							stageFlags;	// vertex, fragment and/or compute
	ksGpuProgramParmType		type;		// texture, buffer or push constant
	ksGpuProgramParmAccess		access;		// read and/or write
	int							index;		// index into ksGpuProgramParmState::parms
	const char * 				name;		// GLSL name
	int							binding;	// Vulkan texture/buffer binding, or push constant offset
											// Note that all Vulkan bindings must be unique per descriptor set across all stages of the pipeline.
											// Note that all Vulkan push constant ranges must be unique across all stages of the pipeline.
} ksGpuProgramParm;

typedef struct
{
	int							numParms;
	const ksGpuProgramParm *	parms;
	VkDescriptorSetLayout		descriptorSetLayout;
	VkPipelineLayout			pipelineLayout;
	int							offsetForIndex[MAX_PROGRAM_PARMS];	// push constant offsets into ksGpuProgramParmState::data based on ksGpuProgramParm::index
	const ksGpuProgramParm *	bindings[MAX_PROGRAM_PARMS];		// descriptor bindings
	const ksGpuProgramParm *	pushConstants[MAX_PROGRAM_PARMS];	// push constants
	int							numBindings;
	int							numPushConstants;
	unsigned int				hash;
} ksGpuProgramParmLayout;

static bool ksGpuProgramParm_IsOpaqueBinding( const ksGpuProgramParmType type )
{
	return	( ( type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED ) ?	true :
			( ( type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE ) ?	true :
			( ( type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM ) ?		true :
			( ( type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE ) ?		true :
																		false ) ) ) );
}

static VkDescriptorType ksGpuProgramParm_GetDescriptorType( const ksGpuProgramParmType type )
{
	return	( ( type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED ) ?	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER :
			( ( type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE ) ?	VK_DESCRIPTOR_TYPE_STORAGE_IMAGE :
			( ( type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM ) ?		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
			( ( type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE ) ?		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
																		VK_DESCRIPTOR_TYPE_MAX_ENUM ) ) ) );
}

static int ksGpuProgramParm_GetPushConstantSize( ksGpuProgramParmType type )
{
	static const int parmSize[KS_GPU_PROGRAM_PARM_TYPE_MAX] =
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
		(unsigned int)sizeof( float[2][2] ),
		(unsigned int)sizeof( float[2][3] ),
		(unsigned int)sizeof( float[2][4] ),
		(unsigned int)sizeof( float[3][2] ),
		(unsigned int)sizeof( float[3][3] ),
		(unsigned int)sizeof( float[3][4] ),
		(unsigned int)sizeof( float[4][2] ),
		(unsigned int)sizeof( float[4][3] ),
		(unsigned int)sizeof( float[4][4] )
	};
	assert( ARRAY_SIZE( parmSize ) == KS_GPU_PROGRAM_PARM_TYPE_MAX );
	return parmSize[type];
}

static const char * ksGpuProgramParm_GetPushConstantGlslType( const ksGpuProgramParmType type )
{
	static const char * glslType[KS_GPU_PROGRAM_PARM_TYPE_MAX] =
	{
		"",
		"",
		"",
		"",
		"int",
		"ivec2",
		"ivec3",
		"ivec4",
		"float",
		"vec2",
		"vec3",
		"vec4",
		"mat2",
		"mat2x3",
		"mat2x4",
		"mat3x2",
		"mat3",
		"mat3x4",
		"mat4x2",
		"mat4x3",
		"mat4"
	};
	assert( ARRAY_SIZE( glslType ) == KS_GPU_PROGRAM_PARM_TYPE_MAX );
	return glslType[type];
}

static VkShaderStageFlags ksGpuProgramParm_GetShaderStageFlags( const ksGpuProgramStageFlags stageFlags )
{
	return	( ( stageFlags & KS_GPU_PROGRAM_STAGE_FLAG_VERTEX ) ?	VK_SHADER_STAGE_VERTEX_BIT : 0 ) |
			( ( stageFlags & KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT ) ?	VK_SHADER_STAGE_FRAGMENT_BIT : 0 ) |
			( ( stageFlags & KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE ) ?	VK_SHADER_STAGE_COMPUTE_BIT : 0 );
}

static void ksGpuProgramParmLayout_Create( ksGpuContext * context, ksGpuProgramParmLayout * layout,
										const ksGpuProgramParm * parms, const int numParms )
{
	memset( layout, 0, sizeof( ksGpuProgramParmLayout ) );

	layout->numParms = numParms;
	layout->parms = parms;

	int numSampledTextureBindings[KS_GPU_PROGRAM_STAGE_MAX] = { 0 };
	int numStorageTextureBindings[KS_GPU_PROGRAM_STAGE_MAX] = { 0 };
	int numUniformBufferBindings[KS_GPU_PROGRAM_STAGE_MAX] = { 0 };
	int numStorageBufferBindings[KS_GPU_PROGRAM_STAGE_MAX] = { 0 };

	int offset = 0;
	memset( layout->offsetForIndex, -1, sizeof( layout->offsetForIndex ) );

	for ( int i = 0; i < numParms; i++ )
	{
		if ( parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED ||
				parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE ||
					parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM ||
						parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
		{
			for ( int stageIndex = 0; stageIndex < KS_GPU_PROGRAM_STAGE_MAX; stageIndex++ )
			{
				if ( ( parms[i].stageFlags & ( 1 << stageIndex ) ) != 0 )
				{
					numSampledTextureBindings[stageIndex] += ( parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED );
					numStorageTextureBindings[stageIndex] += ( parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE );
					numUniformBufferBindings[stageIndex] += ( parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM );
					numStorageBufferBindings[stageIndex] += ( parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE );
				}
			}

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
			offset += ksGpuProgramParm_GetPushConstantSize( parms[i].type );
		}
	}

	// Make sure the descriptor bindings are packed.
	for ( int binding = 0; binding < layout->numBindings; binding++ )
	{
		assert( layout->bindings[binding] != NULL );
	}

	// Verify the push constant layout.
	for ( int push0 = 0; push0 < layout->numPushConstants; push0++ )
	{
		// The push constants for a pipeline cannot use more than 'maxPushConstantsSize' bytes.
		assert( layout->pushConstants[push0]->binding + ksGpuProgramParm_GetPushConstantSize( layout->pushConstants[push0]->type ) <= (int)context->device->physicalDeviceProperties.limits.maxPushConstantsSize );
		// Make sure no push constants overlap.
		for ( int push1 = push0 + 1; push1 < layout->numPushConstants; push1++ )
		{
			assert( layout->pushConstants[push0]->binding >= layout->pushConstants[push1]->binding + ksGpuProgramParm_GetPushConstantSize( layout->pushConstants[push1]->type ) ||
					layout->pushConstants[push0]->binding + ksGpuProgramParm_GetPushConstantSize( layout->pushConstants[push0]->type ) <= layout->pushConstants[push1]->binding );
		}
	}

	// Check the descriptor limits.
	int numTotalSampledTextureBindings = 0;
	int numTotalStorageTextureBindings = 0;
	int numTotalUniformBufferBindings = 0;
	int numTotalStorageBufferBindings = 0;
	for ( int stage = 0; stage < KS_GPU_PROGRAM_STAGE_MAX; stage++ )
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
			if ( ksGpuProgramParm_IsOpaqueBinding( parms[i].type ) )
			{
				descriptorSetBindings[numDescriptorSetBindings].binding = parms[i].binding;
				descriptorSetBindings[numDescriptorSetBindings].descriptorType = ksGpuProgramParm_GetDescriptorType( parms[i].type );
				descriptorSetBindings[numDescriptorSetBindings].descriptorCount = 1;
				descriptorSetBindings[numDescriptorSetBindings].stageFlags = ksGpuProgramParm_GetShaderStageFlags( parms[i].stageFlags );
				descriptorSetBindings[numDescriptorSetBindings].pImmutableSamplers = NULL;
				numDescriptorSetBindings++;
			}
			else // push constant
			{
				pushConstantRanges[numPushConstantRanges].stageFlags = ksGpuProgramParm_GetShaderStageFlags( parms[i].stageFlags );
				pushConstantRanges[numPushConstantRanges].offset = parms[i].binding;
				pushConstantRanges[numPushConstantRanges].size = ksGpuProgramParm_GetPushConstantSize( parms[i].type );
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

static void ksGpuProgramParmLayout_Destroy( ksGpuContext * context, ksGpuProgramParmLayout * layout )
{
	VC( context->device->vkDestroyPipelineLayout( context->device->device, layout->pipelineLayout, VK_ALLOCATOR ) );
	VC( context->device->vkDestroyDescriptorSetLayout( context->device->device, layout->descriptorSetLayout, VK_ALLOCATOR ) );
}

/*
================================================================================================================================

GPU graphics program.

A graphics program encapsulates a vertex and fragment program that are used to render geometry.
For optimal performance a graphics program should only be created at load time, not at runtime.

ksGpuGraphicsProgram

static bool ksGpuGraphicsProgram_Create( ksGpuContext * context, ksGpuGraphicsProgram * program,
										const void * vertexSourceData, const size_t vertexSourceSize,
										const void * fragmentSourceData, const size_t fragmentSourceSize,
										const ksGpuProgramParm * parms, const int numParms,
										const ksGpuVertexAttribute * vertexLayout, const int vertexAttribsFlags );
static void ksGpuGraphicsProgram_Destroy( ksGpuContext * context, ksGpuGraphicsProgram * program );

================================================================================================================================
*/

typedef struct
{
	VkShaderModule						vertexShaderModule;
	VkShaderModule						fragmentShaderModule;
	VkPipelineShaderStageCreateInfo		pipelineStages[2];
	ksGpuProgramParmLayout				parmLayout;
	int									vertexAttribsFlags;
} ksGpuGraphicsProgram;

static bool ksGpuGraphicsProgram_Create( ksGpuContext * context, ksGpuGraphicsProgram * program,
										const void * vertexSourceData, const size_t vertexSourceSize,
										const void * fragmentSourceData, const size_t fragmentSourceSize,
										const ksGpuProgramParm * parms, const int numParms,
										const ksGpuVertexAttribute * vertexLayout, const int vertexAttribsFlags )
{
	UNUSED_PARM( vertexLayout );

	program->vertexAttribsFlags = vertexAttribsFlags;

	ksGpuDevice_CreateShader( context->device, &program->vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT, vertexSourceData, vertexSourceSize );
	ksGpuDevice_CreateShader( context->device, &program->fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, fragmentSourceData, fragmentSourceSize );

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

	ksGpuProgramParmLayout_Create( context, &program->parmLayout, parms, numParms );

	return true;
}

static void ksGpuGraphicsProgram_Destroy( ksGpuContext * context, ksGpuGraphicsProgram * program )
{
	ksGpuProgramParmLayout_Destroy( context, &program->parmLayout );

	VC( context->device->vkDestroyShaderModule( context->device->device, program->vertexShaderModule, VK_ALLOCATOR ) );
	VC( context->device->vkDestroyShaderModule( context->device->device, program->fragmentShaderModule, VK_ALLOCATOR ) );
}

/*
================================================================================================================================

GPU compute program.

For optimal performance a compute program should only be created at load time, not at runtime.

ksGpuComputeProgram

static bool ksGpuComputeProgram_Create( ksGpuContext * context, ksGpuComputeProgram * program,
										const void * computeSourceData, const size_t computeSourceSize,
										const ksGpuProgramParm * parms, const int numParms );
static void ksGpuComputeProgram_Destroy( ksGpuContext * context, ksGpuComputeProgram * program );

================================================================================================================================
*/

typedef struct
{
	VkShaderModule						computeShaderModule;
	VkPipelineShaderStageCreateInfo		pipelineStage;
	ksGpuProgramParmLayout				parmLayout;
} ksGpuComputeProgram;

static bool ksGpuComputeProgram_Create( ksGpuContext * context, ksGpuComputeProgram * program,
									const void * computeSourceData, const size_t computeSourceSize,
									const ksGpuProgramParm * parms, const int numParms )
{
	ksGpuDevice_CreateShader( context->device, &program->computeShaderModule, VK_SHADER_STAGE_COMPUTE_BIT, computeSourceData, computeSourceSize );

	program->pipelineStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	program->pipelineStage.pNext = NULL;
	program->pipelineStage.flags = 0;
	program->pipelineStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	program->pipelineStage.module = program->computeShaderModule;
	program->pipelineStage.pName = "main";
	program->pipelineStage.pSpecializationInfo = NULL;

	ksGpuProgramParmLayout_Create( context, &program->parmLayout, parms, numParms );

	return true;
}

static void ksGpuComputeProgram_Destroy( ksGpuContext * context, ksGpuComputeProgram * program )
{
	ksGpuProgramParmLayout_Destroy( context, &program->parmLayout );

	VC( context->device->vkDestroyShaderModule( context->device->device, program->computeShaderModule, VK_ALLOCATOR ) );
}

/*
================================================================================================================================

GPU graphics pipeline.

A graphics pipeline encapsulates the geometry, program and ROP state that is used to render.
For optimal performance a graphics pipeline should only be created at load time, not at runtime.
The vertex attribute locations are assigned here, when both the geometry and program are known,
to avoid binding vertex attributes that are not used by the vertex shader, and to avoid binding
to a discontinuous set of vertex attribute locations.

ksGpuFrontFace
ksGpuCullMode
ksGpuCompareOp
ksGpuBlendOp
ksGpuBlendFactor
ksGpuRasterOperations
ksGpuGraphicsPipelineParms
ksGpuGraphicsPipeline

static bool ksGpuGraphicsPipeline_Create( ksGpuContext * context, ksGpuGraphicsPipeline * pipeline, const ksGpuGraphicsPipelineParms * parms );
static void ksGpuGraphicsPipeline_Destroy( ksGpuContext * context, ksGpuGraphicsPipeline * pipeline );

================================================================================================================================
*/

typedef enum
{
	KS_GPU_FRONT_FACE_COUNTER_CLOCKWISE				= VK_FRONT_FACE_COUNTER_CLOCKWISE,
	KS_GPU_FRONT_FACE_CLOCKWISE						= VK_FRONT_FACE_CLOCKWISE
} ksGpuFrontFace;

typedef enum
{
	KS_GPU_CULL_MODE_NONE							= 0,
	KS_GPU_CULL_MODE_FRONT							= VK_CULL_MODE_FRONT_BIT,
	KS_GPU_CULL_MODE_BACK							= VK_CULL_MODE_BACK_BIT
} ksGpuCullMode;

typedef enum
{
	KS_GPU_COMPARE_OP_NEVER							= VK_COMPARE_OP_NEVER,
	KS_GPU_COMPARE_OP_LESS							= VK_COMPARE_OP_LESS,
	KS_GPU_COMPARE_OP_EQUAL							= VK_COMPARE_OP_EQUAL,
	KS_GPU_COMPARE_OP_LESS_OR_EQUAL					= VK_COMPARE_OP_LESS_OR_EQUAL,
	KS_GPU_COMPARE_OP_GREATER						= VK_COMPARE_OP_GREATER,
	KS_GPU_COMPARE_OP_NOT_EQUAL						= VK_COMPARE_OP_NOT_EQUAL,
	KS_GPU_COMPARE_OP_GREATER_OR_EQUAL				= VK_COMPARE_OP_GREATER_OR_EQUAL,
	KS_GPU_COMPARE_OP_ALWAYS						= VK_COMPARE_OP_ALWAYS
} ksGpuCompareOp;

typedef enum
{
	KS_GPU_BLEND_OP_ADD								= VK_BLEND_OP_ADD,
	KS_GPU_BLEND_OP_SUBTRACT						= VK_BLEND_OP_SUBTRACT,
	KS_GPU_BLEND_OP_REVERSE_SUBTRACT				= VK_BLEND_OP_REVERSE_SUBTRACT,
	KS_GPU_BLEND_OP_MIN								= VK_BLEND_OP_MIN,
	KS_GPU_BLEND_OP_MAX								= VK_BLEND_OP_MAX
} ksGpuBlendOp;

typedef enum
{
	KS_GPU_BLEND_FACTOR_ZERO						= VK_BLEND_FACTOR_ZERO,
	KS_GPU_BLEND_FACTOR_ONE							= VK_BLEND_FACTOR_ONE,
	KS_GPU_BLEND_FACTOR_SRC_COLOR					= VK_BLEND_FACTOR_SRC_COLOR,
	KS_GPU_BLEND_FACTOR_ONE_MINUS_SRC_COLOR			= VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	KS_GPU_BLEND_FACTOR_DST_COLOR					= VK_BLEND_FACTOR_DST_COLOR,
	KS_GPU_BLEND_FACTOR_ONE_MINUS_DST_COLOR			= VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
	KS_GPU_BLEND_FACTOR_SRC_ALPHA					= VK_BLEND_FACTOR_SRC_ALPHA,
	KS_GPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA			= VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	KS_GPU_BLEND_FACTOR_DST_ALPHA					= VK_BLEND_FACTOR_DST_ALPHA,
	KS_GPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA			= VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
	KS_GPU_BLEND_FACTOR_CONSTANT_COLOR				= VK_BLEND_FACTOR_CONSTANT_COLOR,				
	KS_GPU_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR	= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
	KS_GPU_BLEND_FACTOR_CONSTANT_ALPHA				= VK_BLEND_FACTOR_CONSTANT_ALPHA,
	KS_GPU_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA	= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
	KS_GPU_BLEND_FACTOR_SRC_ALPHA_SATURATE			= VK_BLEND_FACTOR_SRC_ALPHA_SATURATE
} ksGpuBlendFactor;

typedef struct
{
	bool							blendEnable;
	bool							redWriteEnable;
	bool							blueWriteEnable;
	bool							greenWriteEnable;
	bool							alphaWriteEnable;
	bool							depthTestEnable;
	bool							depthWriteEnable;
	ksGpuFrontFace					frontFace;
	ksGpuCullMode					cullMode;
	ksGpuCompareOp					depthCompare;
	ksVector4f						blendColor;
	ksGpuBlendOp					blendOpColor;
	ksGpuBlendFactor				blendSrcColor;
	ksGpuBlendFactor				blendDstColor;
	ksGpuBlendOp					blendOpAlpha;
	ksGpuBlendFactor				blendSrcAlpha;
	ksGpuBlendFactor				blendDstAlpha;
} ksGpuRasterOperations;

typedef struct
{
	ksGpuRasterOperations			rop;
	const ksGpuRenderPass *			renderPass;
	const ksGpuGraphicsProgram *	program;
	const ksGpuGeometry *			geometry;
} ksGpuGraphicsPipelineParms;

#define MAX_VERTEX_ATTRIBUTES		16

typedef struct
{
	ksGpuRasterOperations					rop;
	const ksGpuGraphicsProgram *			program;
	const ksGpuGeometry *					geometry;
	int										vertexAttributeCount;
	int										vertexBindingCount;
	int										firstInstanceBinding;
	VkVertexInputAttributeDescription		vertexAttributes[MAX_VERTEX_ATTRIBUTES];
	VkVertexInputBindingDescription			vertexBindings[MAX_VERTEX_ATTRIBUTES];
	VkDeviceSize							vertexBindingOffsets[MAX_VERTEX_ATTRIBUTES];
	VkPipelineVertexInputStateCreateInfo	vertexInputState;
	VkPipelineInputAssemblyStateCreateInfo	inputAssemblyState;
	VkPipeline								pipeline;
} ksGpuGraphicsPipeline;

static void ksGpuGraphicsPipelineParms_Init( ksGpuGraphicsPipelineParms * parms )
{
	parms->rop.blendEnable = false;
	parms->rop.redWriteEnable = true;
	parms->rop.blueWriteEnable = true;
	parms->rop.greenWriteEnable = true;
	parms->rop.alphaWriteEnable = false;
	parms->rop.depthTestEnable = true;
	parms->rop.depthWriteEnable = true;
	parms->rop.frontFace = KS_GPU_FRONT_FACE_COUNTER_CLOCKWISE;
	parms->rop.cullMode = KS_GPU_CULL_MODE_BACK;
	parms->rop.depthCompare = KS_GPU_COMPARE_OP_LESS_OR_EQUAL;
	parms->rop.blendColor.x = 0.0f;
	parms->rop.blendColor.y = 0.0f;
	parms->rop.blendColor.z = 0.0f;
	parms->rop.blendColor.w = 0.0f;
	parms->rop.blendOpColor = KS_GPU_BLEND_OP_ADD;
	parms->rop.blendSrcColor = KS_GPU_BLEND_FACTOR_ONE;
	parms->rop.blendDstColor = KS_GPU_BLEND_FACTOR_ZERO;
	parms->rop.blendOpAlpha = KS_GPU_BLEND_OP_ADD;
	parms->rop.blendSrcAlpha = KS_GPU_BLEND_FACTOR_ONE;
	parms->rop.blendDstAlpha = KS_GPU_BLEND_FACTOR_ZERO;
	parms->renderPass = NULL;
	parms->program = NULL;
	parms->geometry = NULL;
}

static void InitVertexAttributes( const bool instance,
								const ksGpuVertexAttribute * vertexLayout, const int numAttribs,
								const int storedAttribsFlags, const int usedAttribsFlags,
								VkVertexInputAttributeDescription * attributes, int * attributeCount,
								VkVertexInputBindingDescription * bindings, int * bindingCount,
								VkDeviceSize * bindingOffsets )
{
	size_t offset = 0;
	for ( int i = 0; vertexLayout[i].attributeFlag != 0; i++ )
	{
		const ksGpuVertexAttribute * v = &vertexLayout[i];
		if ( ( v->attributeFlag & storedAttribsFlags ) != 0 )
		{
			if ( ( v->attributeFlag & usedAttribsFlags ) != 0 )
			{
				for ( int location = 0; location < v->locationCount; location++ )
				{
					attributes[*attributeCount + location].location = *attributeCount + location;
					attributes[*attributeCount + location].binding = *bindingCount;
					attributes[*attributeCount + location].format = (VkFormat) v->attributeFormat;
					attributes[*attributeCount + location].offset = (uint32_t)( location * v->attributeSize / v->locationCount );	// limited offset used for packed vertex data
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

static bool ksGpuGraphicsPipeline_Create( ksGpuContext * context, ksGpuGraphicsPipeline * pipeline, const ksGpuGraphicsPipelineParms * parms )
{
	// Make sure the geometry provides all the attributes needed by the program.
	assert( ( ( parms->geometry->vertexAttribsFlags | parms->geometry->instanceAttribsFlags ) & parms->program->vertexAttribsFlags ) == parms->program->vertexAttribsFlags );

	pipeline->rop = parms->rop;
	pipeline->program = parms->program;
	pipeline->geometry = parms->geometry;
	pipeline->vertexAttributeCount = 0;
	pipeline->vertexBindingCount = 0;

	InitVertexAttributes( false, parms->geometry->layout, parms->geometry->vertexCount,
							parms->geometry->vertexAttribsFlags, parms->program->vertexAttribsFlags,
							pipeline->vertexAttributes, &pipeline->vertexAttributeCount,
							pipeline->vertexBindings, &pipeline->vertexBindingCount,
							pipeline->vertexBindingOffsets );

	pipeline->firstInstanceBinding = pipeline->vertexBindingCount;

	InitVertexAttributes( true, parms->geometry->layout, parms->geometry->instanceCount,
							parms->geometry->instanceAttribsFlags, parms->program->vertexAttribsFlags,
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
	rasterizationStateCreateInfo.cullMode						= (VkCullModeFlags)parms->rop.cullMode;
	rasterizationStateCreateInfo.frontFace						= (VkFrontFace)parms->rop.frontFace;
	rasterizationStateCreateInfo.depthBiasEnable				= VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor		= 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp					= 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor			= 0.0f;
	rasterizationStateCreateInfo.lineWidth						= 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
	multisampleStateCreateInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.pNext							= NULL;
	multisampleStateCreateInfo.flags							= 0;
	multisampleStateCreateInfo.rasterizationSamples				= (VkSampleCountFlagBits)parms->renderPass->sampleCount;
	multisampleStateCreateInfo.sampleShadingEnable				= VK_FALSE;
	multisampleStateCreateInfo.minSampleShading					= 1.0f;
	multisampleStateCreateInfo.pSampleMask						= NULL;
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
	colorBlendAttachementState[0].colorWriteMask				=	( parms->rop.redWriteEnable ? VK_COLOR_COMPONENT_R_BIT : 0 ) |
																	( parms->rop.blueWriteEnable ? VK_COLOR_COMPONENT_G_BIT : 0 ) |
																	( parms->rop.greenWriteEnable ? VK_COLOR_COMPONENT_B_BIT : 0 ) |
																	( parms->rop.alphaWriteEnable ? VK_COLOR_COMPONENT_A_BIT : 0 );

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
	colorBlendStateCreateInfo.sType								= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.pNext								= NULL;
	colorBlendStateCreateInfo.flags								= 0;
	colorBlendStateCreateInfo.logicOpEnable						= VK_FALSE;
	colorBlendStateCreateInfo.logicOp							= VK_LOGIC_OP_CLEAR;
	colorBlendStateCreateInfo.attachmentCount					= 1;
	colorBlendStateCreateInfo.pAttachments						= colorBlendAttachementState;
	colorBlendStateCreateInfo.blendConstants[0]					= parms->rop.blendColor.x;
	colorBlendStateCreateInfo.blendConstants[1]					= parms->rop.blendColor.y;
	colorBlendStateCreateInfo.blendConstants[2]					= parms->rop.blendColor.z;
	colorBlendStateCreateInfo.blendConstants[3]					= parms->rop.blendColor.w;

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

static void ksGpuGraphicsPipeline_Destroy( ksGpuContext * context, ksGpuGraphicsPipeline * pipeline )
{
	VC( context->device->vkDestroyPipeline( context->device->device, pipeline->pipeline, VK_ALLOCATOR ) );

	memset( pipeline, 0, sizeof( ksGpuGraphicsPipeline ) );
}

/*
================================================================================================================================

GPU compute pipeline.

A compute pipeline encapsulates a compute program.
For optimal performance a compute pipeline should only be created at load time, not at runtime.

ksGpuComputePipeline

static bool ksGpuComputePipeline_Create( ksGpuContext * context, ksGpuComputePipeline * pipeline, const ksGpuComputeProgram * program );
static void ksGpuComputePipeline_Destroy( ksGpuContext * context, ksGpuComputePipeline * pipeline );

================================================================================================================================
*/

typedef struct
{
	const ksGpuComputeProgram *	program;
	VkPipeline					pipeline;
} ksGpuComputePipeline;

static bool ksGpuComputePipeline_Create( ksGpuContext * context, ksGpuComputePipeline * pipeline, const ksGpuComputeProgram * program )
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

static void ksGpuComputePipeline_Destroy( ksGpuContext * context, ksGpuComputePipeline * pipeline )
{
	VC( context->device->vkDestroyPipeline( context->device->device, pipeline->pipeline, VK_ALLOCATOR ) );

	memset( pipeline, 0, sizeof( ksGpuComputePipeline ) );
}

/*
================================================================================================================================

GPU fence.

A fence is used to notify completion of a command buffer.
For optimal performance a fence should only be created at load time, not at runtime.

ksGpuFence

static void ksGpuFence_Create( ksGpuContext * context, ksGpuFence * fence );
static void ksGpuFence_Destroy( ksGpuContext * context, ksGpuFence * fence );
static void ksGpuFence_Submit( ksGpuContext * context, ksGpuFence * fence );
static void ksGpuFence_IsSignalled( ksGpuContext * context, ksGpuFence * fence );

================================================================================================================================
*/

typedef struct
{
	VkFence			fence;
	bool			submitted;
} ksGpuFence;

static void ksGpuFence_Create( ksGpuContext * context, ksGpuFence * fence )
{
	VkFenceCreateInfo fenceCreateInfo;
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = NULL;
	fenceCreateInfo.flags = 0;

	VK( context->device->vkCreateFence( context->device->device, &fenceCreateInfo, VK_ALLOCATOR, &fence->fence ) );

	fence->submitted = false;
}

static void ksGpuFence_Destroy( ksGpuContext * context, ksGpuFence * fence )
{
	VC( context->device->vkDestroyFence( context->device->device, fence->fence, VK_ALLOCATOR ) );
	fence->fence = VK_NULL_HANDLE;
	fence->submitted = false;
}

static void ksGpuFence_Submit( ksGpuContext * context, ksGpuFence * fence )
{
	UNUSED_PARM( context );
	fence->submitted = true;
}

static bool ksGpuFence_IsSignalled( ksGpuContext * context, ksGpuFence * fence )
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
For optimal performance a timer should only be created at load time, not at runtime.
To avoid synchronization, ksGpuTimer_GetNanoseconds() reports the time from KS_GPU_TIMER_FRAMES_DELAYED frames ago.
Timer queries are allowed to overlap and can be nested.
Timer queries that are issued inside a render pass may not produce accurate times on tiling GPUs.

ksGpuTimer

static void ksGpuTimer_Create( ksGpuContext * context, ksGpuTimer * timer );
static void ksGpuTimer_Destroy( ksGpuContext * context, ksGpuTimer * timer );
static ksNanoseconds ksGpuTimer_GetNanoseconds( ksGpuTimer * timer );

================================================================================================================================
*/

#define KS_GPU_TIMER_FRAMES_DELAYED	2

typedef struct
{
	VkBool32			supported;
	ksNanoseconds		period;
	VkQueryPool			pool;
	uint32_t			init;
	uint32_t			index;
	uint64_t			data[2];
} ksGpuTimer;

static void ksGpuTimer_Create( ksGpuContext * context, ksGpuTimer * timer )
{
	memset( timer, 0, sizeof( ksGpuTimer ) );

	timer->supported = context->device->queueFamilyProperties[context->queueFamilyIndex].timestampValidBits != 0;
	if ( !timer->supported )
	{
		return;
	}

	timer->period = (ksNanoseconds) context->device->physicalDeviceProperties.limits.timestampPeriod;

	const uint32_t queryCount = ( KS_GPU_TIMER_FRAMES_DELAYED + 1 ) * 2;

	VkQueryPoolCreateInfo queryPoolCreateInfo;
	queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	queryPoolCreateInfo.pNext = NULL;
	queryPoolCreateInfo.flags = 0;
	queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	queryPoolCreateInfo.queryCount = queryCount;
	queryPoolCreateInfo.pipelineStatistics = 0;

	VK( context->device->vkCreateQueryPool( context->device->device, &queryPoolCreateInfo, VK_ALLOCATOR, &timer->pool ) );

	ksGpuContext_CreateSetupCmdBuffer( context );
	VC( context->device->vkCmdResetQueryPool( context->setupCommandBuffer, timer->pool, 0, queryCount ) );
	ksGpuContext_FlushSetupCmdBuffer( context );
}

static void ksGpuTimer_Destroy( ksGpuContext * context, ksGpuTimer * timer )
{
	if ( timer->supported )
	{
		VC( context->device->vkDestroyQueryPool( context->device->device, timer->pool, VK_ALLOCATOR ) );
	}
}

static ksNanoseconds ksGpuTimer_GetNanoseconds( ksGpuTimer * timer )
{
	if ( timer->supported )
	{
		return ( timer->data[1] - timer->data[0] ) * timer->period;
	}
	else
	{
		return 0;
	}
}

/*
================================================================================================================================

GPU program parm state.

ksGpuProgramParmState

================================================================================================================================
*/

#define SAVE_PUSH_CONSTANT_STATE 			1
#define MAX_SAVED_PUSH_CONSTANT_BYTES		512

typedef struct
{
	const void *	parms[MAX_PROGRAM_PARMS];
#if SAVE_PUSH_CONSTANT_STATE == 1
	unsigned char	data[MAX_SAVED_PUSH_CONSTANT_BYTES];
#endif
} ksGpuProgramParmState;

static void ksGpuProgramParmState_SetParm( ksGpuProgramParmState * parmState, const ksGpuProgramParmLayout * parmLayout,
											const int index, const ksGpuProgramParmType parmType, const void * pointer )
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
	const int pushConstantSize = ksGpuProgramParm_GetPushConstantSize( parmType );
	if ( pushConstantSize > 0 )
	{
		assert( parmLayout->offsetForIndex[index] >= 0 );
		assert( parmLayout->offsetForIndex[index] + pushConstantSize <= MAX_SAVED_PUSH_CONSTANT_BYTES );
		memcpy( &parmState->data[parmLayout->offsetForIndex[index]], pointer, pushConstantSize );
	}
#endif
}

static const void * ksGpuProgramParmState_NewPushConstantData( const ksGpuProgramParmLayout * newLayout, const int newPushConstantIndex, const ksGpuProgramParmState * newParmState,
													const ksGpuProgramParmLayout * oldLayout, const int oldPushConstantIndex, const ksGpuProgramParmState * oldParmState,
													const bool force )
{
#if SAVE_PUSH_CONSTANT_STATE == 1
	const ksGpuProgramParm * newParm = newLayout->pushConstants[newPushConstantIndex];
	const unsigned char * newData = &newParmState->data[newLayout->offsetForIndex[newParm->index]];
	if ( force || oldLayout == NULL || oldPushConstantIndex >= oldLayout->numPushConstants )
	{
		return newData;
	}
	const ksGpuProgramParm * oldParm = oldLayout->pushConstants[oldPushConstantIndex];
	const unsigned char * oldData = &oldParmState->data[oldLayout->offsetForIndex[oldParm->index]];
	if ( newParm->type != oldParm->type || newParm->binding != oldParm->binding )
	{
		return newData;
	}
	const int pushConstantSize = ksGpuProgramParm_GetPushConstantSize( newParm->type );
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

static bool ksGpuProgramParmState_DescriptorsMatch( const ksGpuProgramParmLayout * layout1, const ksGpuProgramParmState * parmState1,
													const ksGpuProgramParmLayout * layout2, const ksGpuProgramParmState * parmState2 )
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

A graphics command encapsulates all GPU state associated with a single draw call.
The pointers passed in as parameters are expected to point to unique objects that persist
at least past the submission of the command buffer into which the graphics command is
submitted. Because pointers are maintained as state, DO NOT use pointers to local
variables that will go out of scope before the command buffer is submitted.

ksGpuGraphicsCommand

static void ksGpuGraphicsCommand_Init( ksGpuGraphicsCommand * command );
static void ksGpuGraphicsCommand_SetPipeline( ksGpuGraphicsCommand * command, const ksGpuGraphicsPipeline * pipeline );
static void ksGpuGraphicsCommand_SetVertexBuffer( ksGpuGraphicsCommand * command, const ksGpuBuffer * vertexBuffer );
static void ksGpuGraphicsCommand_SetInstanceBuffer( ksGpuGraphicsCommand * command, const ksGpuBuffer * instanceBuffer );
static void ksGpuGraphicsCommand_SetParmTextureSampled( ksGpuGraphicsCommand * command, const int index, const ksGpuTexture * texture );
static void ksGpuGraphicsCommand_SetParmTextureStorage( ksGpuGraphicsCommand * command, const int index, const ksGpuTexture * texture );
static void ksGpuGraphicsCommand_SetParmBufferUniform( ksGpuGraphicsCommand * command, const int index, const ksGpuBuffer * buffer );
static void ksGpuGraphicsCommand_SetParmBufferStorage( ksGpuGraphicsCommand * command, const int index, const ksGpuBuffer * buffer );
static void ksGpuGraphicsCommand_SetParmInt( ksGpuGraphicsCommand * command, const int index, const int * value );
static void ksGpuGraphicsCommand_SetParmIntVector2( ksGpuGraphicsCommand * command, const int index, const ksVector2i * value );
static void ksGpuGraphicsCommand_SetParmIntVector3( ksGpuGraphicsCommand * command, const int index, const ksVector3i * value );
static void ksGpuGraphicsCommand_SetParmIntVector4( ksGpuGraphicsCommand * command, const int index, const ksVector4i * value );
static void ksGpuGraphicsCommand_SetParmFloat( ksGpuGraphicsCommand * command, const int index, const float * value );
static void ksGpuGraphicsCommand_SetParmFloatVector2( ksGpuGraphicsCommand * command, const int index, const ksVector2f * value );
static void ksGpuGraphicsCommand_SetParmFloatVector3( ksGpuGraphicsCommand * command, const int index, const ksVector3f * value );
static void ksGpuGraphicsCommand_SetParmFloatVector4( ksGpuGraphicsCommand * command, const int index, const ksVector3f * value );
static void ksGpuGraphicsCommand_SetParmFloatMatrix2x2( ksGpuGraphicsCommand * command, const int index, const ksMatrix2x2f * value );
static void ksGpuGraphicsCommand_SetParmFloatMatrix2x3( ksGpuGraphicsCommand * command, const int index, const ksMatrix2x3f * value );
static void ksGpuGraphicsCommand_SetParmFloatMatrix2x4( ksGpuGraphicsCommand * command, const int index, const ksMatrix2x4f * value );
static void ksGpuGraphicsCommand_SetParmFloatMatrix3x2( ksGpuGraphicsCommand * command, const int index, const ksMatrix3x2f * value );
static void ksGpuGraphicsCommand_SetParmFloatMatrix3x3( ksGpuGraphicsCommand * command, const int index, const ksMatrix3x3f * value );
static void ksGpuGraphicsCommand_SetParmFloatMatrix3x4( ksGpuGraphicsCommand * command, const int index, const ksMatrix3x4f * value );
static void ksGpuGraphicsCommand_SetParmFloatMatrix4x2( ksGpuGraphicsCommand * command, const int index, const ksMatrix4x2f * value );
static void ksGpuGraphicsCommand_SetParmFloatMatrix4x3( ksGpuGraphicsCommand * command, const int index, const ksMatrix4x3f * value );
static void ksGpuGraphicsCommand_SetParmFloatMatrix4x4( ksGpuGraphicsCommand * command, const int index, const ksMatrix4x4f * value );
static void ksGpuGraphicsCommand_SetNumInstances( ksGpuGraphicsCommand * command, const int numInstances );

================================================================================================================================
*/

typedef struct
{
	const ksGpuGraphicsPipeline *	pipeline;
	const ksGpuBuffer *				vertexBuffer;		// vertex buffer returned by ksGpuCommandBuffer_MapVertexAttributes
	const ksGpuBuffer *				instanceBuffer;		// instance buffer returned by ksGpuCommandBuffer_MapInstanceAttributes
	ksGpuProgramParmState			parmState;
	int								numInstances;
} ksGpuGraphicsCommand;

static void ksGpuGraphicsCommand_Init( ksGpuGraphicsCommand * command )
{
	command->pipeline = NULL;
	command->vertexBuffer = NULL;
	command->instanceBuffer = NULL;
	memset( (void *)&command->parmState, 0, sizeof( command->parmState ) );
	command->numInstances = 1;
}

static void ksGpuGraphicsCommand_SetPipeline( ksGpuGraphicsCommand * command, const ksGpuGraphicsPipeline * pipeline )
{
	command->pipeline = pipeline;
}

static void ksGpuGraphicsCommand_SetVertexBuffer( ksGpuGraphicsCommand * command, const ksGpuBuffer * vertexBuffer )
{
	command->vertexBuffer = vertexBuffer;
}

static void ksGpuGraphicsCommand_SetInstanceBuffer( ksGpuGraphicsCommand * command, const ksGpuBuffer * instanceBuffer )
{
	command->instanceBuffer = instanceBuffer;
}

static void ksGpuGraphicsCommand_SetParmTextureSampled( ksGpuGraphicsCommand * command, const int index, const ksGpuTexture * texture )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED, texture );
}

static void ksGpuGraphicsCommand_SetParmTextureStorage( ksGpuGraphicsCommand * command, const int index, const ksGpuTexture * texture )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE, texture );
}

static void ksGpuGraphicsCommand_SetParmBufferUniform( ksGpuGraphicsCommand * command, const int index, const ksGpuBuffer * buffer )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM, buffer );
}

static void ksGpuGraphicsCommand_SetParmBufferStorage( ksGpuGraphicsCommand * command, const int index, const ksGpuBuffer * buffer )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE, buffer );
}

static void ksGpuGraphicsCommand_SetParmInt( ksGpuGraphicsCommand * command, const int index, const int * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT, value );
}

static void ksGpuGraphicsCommand_SetParmIntVector2( ksGpuGraphicsCommand * command, const int index, const ksVector2i * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2, value );
}

static void ksGpuGraphicsCommand_SetParmIntVector3( ksGpuGraphicsCommand * command, const int index, const ksVector3i * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3, value );
}

static void ksGpuGraphicsCommand_SetParmIntVector4( ksGpuGraphicsCommand * command, const int index, const ksVector4i * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4, value );
}

static void ksGpuGraphicsCommand_SetParmFloat( ksGpuGraphicsCommand * command, const int index, const float * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT, value );
}

static void ksGpuGraphicsCommand_SetParmFloatVector2( ksGpuGraphicsCommand * command, const int index, const ksVector2f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2, value );
}

static void ksGpuGraphicsCommand_SetParmFloatVector3( ksGpuGraphicsCommand * command, const int index, const ksVector3f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3, value );
}

static void ksGpuGraphicsCommand_SetParmFloatVector4( ksGpuGraphicsCommand * command, const int index, const ksVector4f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4, value );
}

static void ksGpuGraphicsCommand_SetParmFloatMatrix2x2( ksGpuGraphicsCommand * command, const int index, const ksMatrix2x2f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2, value );
}

static void ksGpuGraphicsCommand_SetParmFloatMatrix2x3( ksGpuGraphicsCommand * command, const int index, const ksMatrix2x3f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3, value );
}

static void ksGpuGraphicsCommand_SetParmFloatMatrix2x4( ksGpuGraphicsCommand * command, const int index, const ksMatrix2x4f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4, value );
}

static void ksGpuGraphicsCommand_SetParmFloatMatrix3x2( ksGpuGraphicsCommand * command, const int index, const ksMatrix3x2f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2, value );
}

static void ksGpuGraphicsCommand_SetParmFloatMatrix3x3( ksGpuGraphicsCommand * command, const int index, const ksMatrix3x3f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3, value );
}

static void ksGpuGraphicsCommand_SetParmFloatMatrix3x4( ksGpuGraphicsCommand * command, const int index, const ksMatrix3x4f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4, value );
}

static void ksGpuGraphicsCommand_SetParmFloatMatrix4x2( ksGpuGraphicsCommand * command, const int index, const ksMatrix4x2f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2, value );
}

static void ksGpuGraphicsCommand_SetParmFloatMatrix4x3( ksGpuGraphicsCommand * command, const int index, const ksMatrix4x3f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3, value );
}

static void ksGpuGraphicsCommand_SetParmFloatMatrix4x4( ksGpuGraphicsCommand * command, const int index, const ksMatrix4x4f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4, value );
}

static void ksGpuGraphicsCommand_SetNumInstances( ksGpuGraphicsCommand * command, const int numInstances )
{
	command->numInstances = numInstances;
}

/*
================================================================================================================================

GPU compute commands.

A compute command encapsulates all GPU state associated with a single dispatch.
The pointers passed in as parameters are expected to point to unique objects that persist
at least past the submission of the command buffer into which the compute command is
submitted. Because various pointer are maintained as state, DO NOT use pointers to local
variables that will go out of scope before the command buffer is submitted.

ksGpuComputeCommand

static void ksGpuComputeCommand_Init( ksGpuComputeCommand * command );
static void ksGpuComputeCommand_SetPipeline( ksGpuComputeCommand * command, const ksGpuComputePipeline * pipeline );
static void ksGpuComputeCommand_SetParmTextureSampled( ksGpuComputeCommand * command, const int index, const ksGpuTexture * texture );
static void ksGpuComputeCommand_SetParmTextureStorage( ksGpuComputeCommand * command, const int index, const ksGpuTexture * texture );
static void ksGpuComputeCommand_SetParmBufferUniform( ksGpuComputeCommand * command, const int index, const ksGpuBuffer * buffer );
static void ksGpuComputeCommand_SetParmBufferStorage( ksGpuComputeCommand * command, const int index, const ksGpuBuffer * buffer );
static void ksGpuComputeCommand_SetParmInt( ksGpuComputeCommand * command, const int index, const int * value );
static void ksGpuComputeCommand_SetParmIntVector2( ksGpuComputeCommand * command, const int index, const ksVector2i * value );
static void ksGpuComputeCommand_SetParmIntVector3( ksGpuComputeCommand * command, const int index, const ksVector3i * value );
static void ksGpuComputeCommand_SetParmIntVector4( ksGpuComputeCommand * command, const int index, const ksVector4i * value );
static void ksGpuComputeCommand_SetParmFloat( ksGpuComputeCommand * command, const int index, const float * value );
static void ksGpuComputeCommand_SetParmFloatVector2( ksGpuComputeCommand * command, const int index, const ksVector2f * value );
static void ksGpuComputeCommand_SetParmFloatVector3( ksGpuComputeCommand * command, const int index, const ksVector3f * value );
static void ksGpuComputeCommand_SetParmFloatVector4( ksGpuComputeCommand * command, const int index, const ksVector3f * value );
static void ksGpuComputeCommand_SetParmFloatMatrix2x2( ksGpuComputeCommand * command, const int index, const ksMatrix2x2f * value );
static void ksGpuComputeCommand_SetParmFloatMatrix2x3( ksGpuComputeCommand * command, const int index, const ksMatrix2x3f * value );
static void ksGpuComputeCommand_SetParmFloatMatrix2x4( ksGpuComputeCommand * command, const int index, const ksMatrix2x4f * value );
static void ksGpuComputeCommand_SetParmFloatMatrix3x2( ksGpuComputeCommand * command, const int index, const ksMatrix3x2f * value );
static void ksGpuComputeCommand_SetParmFloatMatrix3x3( ksGpuComputeCommand * command, const int index, const ksMatrix3x3f * value );
static void ksGpuComputeCommand_SetParmFloatMatrix3x4( ksGpuComputeCommand * command, const int index, const ksMatrix3x4f * value );
static void ksGpuComputeCommand_SetParmFloatMatrix4x2( ksGpuComputeCommand * command, const int index, const ksMatrix4x2f * value );
static void ksGpuComputeCommand_SetParmFloatMatrix4x3( ksGpuComputeCommand * command, const int index, const ksMatrix4x3f * value );
static void ksGpuComputeCommand_SetParmFloatMatrix4x4( ksGpuComputeCommand * command, const int index, const ksMatrix4x4f * value );
static void ksGpuComputeCommand_SetDimensions( ksGpuComputeCommand * command, const int x, const int y, const int z );

================================================================================================================================
*/

typedef struct
{
	const ksGpuComputePipeline *	pipeline;
	ksGpuProgramParmState			parmState;
	int								x;
	int								y;
	int								z;
} ksGpuComputeCommand;

static void ksGpuComputeCommand_Init( ksGpuComputeCommand * command )
{
	command->pipeline = NULL;
	memset( (void *)&command->parmState, 0, sizeof( command->parmState ) );
	command->x = 1;
	command->y = 1;
	command->z = 1;
}

static void ksGpuComputeCommand_SetPipeline( ksGpuComputeCommand * command, const ksGpuComputePipeline * pipeline )
{
	command->pipeline = pipeline;
}

static void ksGpuComputeCommand_SetParmTextureSampled( ksGpuComputeCommand * command, const int index, const ksGpuTexture * texture )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED, texture );
}

static void ksGpuComputeCommand_SetParmTextureStorage( ksGpuComputeCommand * command, const int index, const ksGpuTexture * texture )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE, texture );
}

static void ksGpuComputeCommand_SetParmBufferUniform( ksGpuComputeCommand * command, const int index, const ksGpuBuffer * buffer )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM, buffer );
}

static void ksGpuComputeCommand_SetParmBufferStorage( ksGpuComputeCommand * command, const int index, const ksGpuBuffer * buffer )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE, buffer );
}

static void ksGpuComputeCommand_SetParmInt( ksGpuComputeCommand * command, const int index, const int * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT, value );
}

static void ksGpuComputeCommand_SetParmIntVector2( ksGpuComputeCommand * command, const int index, const ksVector2i * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2, value );
}

static void ksGpuComputeCommand_SetParmIntVector3( ksGpuComputeCommand * command, const int index, const ksVector3i * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3, value );
}

static void ksGpuComputeCommand_SetParmIntVector4( ksGpuComputeCommand * command, const int index, const ksVector4i * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4, value );
}

static void ksGpuComputeCommand_SetParmFloat( ksGpuComputeCommand * command, const int index, const float * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT, value );
}

static void ksGpuComputeCommand_SetParmFloatVector2( ksGpuComputeCommand * command, const int index, const ksVector2f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2, value );
}

static void ksGpuComputeCommand_SetParmFloatVector3( ksGpuComputeCommand * command, const int index, const ksVector3f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3, value );
}

static void ksGpuComputeCommand_SetParmFloatVector4( ksGpuComputeCommand * command, const int index, const ksVector4f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4, value );
}

static void ksGpuComputeCommand_SetParmFloatMatrix2x2( ksGpuComputeCommand * command, const int index, const ksMatrix2x2f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2, value );
}

static void ksGpuComputeCommand_SetParmFloatMatrix2x3( ksGpuComputeCommand * command, const int index, const ksMatrix2x3f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3, value );
}

static void ksGpuComputeCommand_SetParmFloatMatrix2x4( ksGpuComputeCommand * command, const int index, const ksMatrix2x4f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4, value );
}

static void ksGpuComputeCommand_SetParmFloatMatrix3x2( ksGpuComputeCommand * command, const int index, const ksMatrix3x2f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2, value );
}

static void ksGpuComputeCommand_SetParmFloatMatrix3x3( ksGpuComputeCommand * command, const int index, const ksMatrix3x3f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3, value );
}

static void ksGpuComputeCommand_SetParmFloatMatrix3x4( ksGpuComputeCommand * command, const int index, const ksMatrix3x4f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4, value );
}

static void ksGpuComputeCommand_SetParmFloatMatrix4x2( ksGpuComputeCommand * command, const int index, const ksMatrix4x2f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2, value );
}

static void ksGpuComputeCommand_SetParmFloatMatrix4x3( ksGpuComputeCommand * command, const int index, const ksMatrix4x3f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3, value );
}

static void ksGpuComputeCommand_SetParmFloatMatrix4x4( ksGpuComputeCommand * command, const int index, const ksMatrix4x4f * value )
{
	ksGpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4, value );
}

static void ksGpuComputeCommand_SetDimensions( ksGpuComputeCommand * command, const int x, const int y, const int z )
{
	command->x = x;
	command->y = y;
	command->z = z;
}

/*
================================================================================================================================

GPU pipeline resources.

Resources, like texture and uniform buffer descriptions, that are used by a graphics or compute pipeline.

ksGpuPipelineResources

static void ksGpuPipelineResources_Create( ksGpuContext * context, ksGpuPipelineResources * resources,
										const ksGpuProgramParmLayout * parmLayout,
										const ksGpuProgramParmState * parms );
static void ksGpuPipelineResources_Destroy( ksGpuContext * context, ksGpuPipelineResources * resources );

================================================================================================================================
*/

typedef struct ksGpuPipelineResources_s
{
	struct ksGpuPipelineResources_s *	next;
	int									unusedCount;			// Number of frames these resources have not been used.
	const ksGpuProgramParmLayout *		parmLayout;
	ksGpuProgramParmState				parms;
	VkDescriptorPool					descriptorPool;
	VkDescriptorSet						descriptorSet;
} ksGpuPipelineResources;

static void ksGpuPipelineResources_Create( ksGpuContext * context, ksGpuPipelineResources * resources,
										const ksGpuProgramParmLayout * parmLayout,
										const ksGpuProgramParmState * parms )
{
	memset( resources, 0, sizeof( ksGpuPipelineResources ) );

	resources->parmLayout = parmLayout;
	memcpy( (void *)&resources->parms, parms, sizeof( ksGpuProgramParmState ) );

	//
	// Create descriptor pool.
	//

	{
		VkDescriptorPoolSize typeCounts[MAX_PROGRAM_PARMS];

		int count = 0;
		for ( int i = 0; i < parmLayout->numBindings; i++ )
		{
			VkDescriptorType type = ksGpuProgramParm_GetDescriptorType( parmLayout->bindings[i]->type );
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
			const ksGpuProgramParm * binding = parmLayout->bindings[i];

			writes[numWrites].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[numWrites].pNext = NULL;
			writes[numWrites].dstSet = resources->descriptorSet;
			writes[numWrites].dstBinding = binding->binding;
			writes[numWrites].dstArrayElement = 0;
			writes[numWrites].descriptorCount = 1;
			writes[numWrites].descriptorType = ksGpuProgramParm_GetDescriptorType( parmLayout->bindings[i]->type );
			writes[numWrites].pImageInfo = &imageInfo[numWrites];
			writes[numWrites].pBufferInfo = &bufferInfo[numWrites];
			writes[numWrites].pTexelBufferView = NULL;

			if ( binding->type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED )
			{
				const ksGpuTexture * texture = (const ksGpuTexture *)parms->parms[binding->index];
				assert( texture->usage == KS_GPU_TEXTURE_USAGE_SAMPLED );
				assert( texture->imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

				imageInfo[numWrites].sampler = texture->sampler;
				imageInfo[numWrites].imageView = texture->view;
				imageInfo[numWrites].imageLayout = texture->imageLayout;
			}
			else if ( binding->type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE )
			{
				const ksGpuTexture * texture = (const ksGpuTexture *)parms->parms[binding->index];
				assert( texture->usage == KS_GPU_TEXTURE_USAGE_STORAGE );
				assert( texture->imageLayout == VK_IMAGE_LAYOUT_GENERAL );

				imageInfo[numWrites].sampler = VK_NULL_HANDLE;
				imageInfo[numWrites].imageView = texture->view;
				imageInfo[numWrites].imageLayout = texture->imageLayout;
			}
			else if ( binding->type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM )
			{
				const ksGpuBuffer * buffer = (const ksGpuBuffer *)parms->parms[binding->index];
				assert( buffer->type == KS_GPU_BUFFER_TYPE_UNIFORM );

				bufferInfo[numWrites].buffer = buffer->buffer;
				bufferInfo[numWrites].offset = 0;
				bufferInfo[numWrites].range = buffer->size;
			}
			else if ( binding->type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
			{
				const ksGpuBuffer * buffer = (const ksGpuBuffer *)parms->parms[binding->index];
				assert( buffer->type == KS_GPU_BUFFER_TYPE_STORAGE );

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

static void ksGpuPipelineResources_Destroy( ksGpuContext * context, ksGpuPipelineResources * resources )
{
	VC( context->device->vkFreeDescriptorSets( context->device->device, resources->descriptorPool, 1, &resources->descriptorSet ) );
	VC( context->device->vkDestroyDescriptorPool( context->device->device, resources->descriptorPool, VK_ALLOCATOR ) );

	memset( resources, 0, sizeof( ksGpuPipelineResources ) );
}

/*
================================================================================================================================

GPU command buffer.

A command buffer is used to record graphics and compute commands.
For optimal performance a command buffer should only be created at load time, not at runtime.
When a command is submitted, the state of the command is compared with the currently saved state,
and only the state that has changed translates into graphics API function calls.

ksGpuCommandBuffer
ksGpuCommandBufferType
ksGpuBufferUnmapType

static void ksGpuCommandBuffer_Create( ksGpuContext * context, ksGpuCommandBuffer * commandBuffer, const ksGpuCommandBufferType type, const int numBuffers );
static void ksGpuCommandBuffer_Destroy( ksGpuContext * context, ksGpuCommandBuffer * commandBuffer );

static void ksGpuCommandBuffer_BeginPrimary( ksGpuCommandBuffer * commandBuffer );
static void ksGpuCommandBuffer_EndPrimary( ksGpuCommandBuffer * commandBuffer );
static ksGpuFence * ksGpuCommandBuffer_SubmitPrimary( ksGpuCommandBuffer * commandBuffer );

static void ksGpuCommandBuffer_BeginSecondary( ksGpuCommandBuffer * commandBuffer, ksGpuRenderPass * renderPass, ksGpuFramebuffer * framebuffer );
static void ksGpuCommandBuffer_EndSecondary( ksGpuCommandBuffer * commandBuffer );
static void ksGpuCommandBuffer_SubmitSecondary( ksGpuCommandBuffer * commandBuffer, ksGpuCommandBuffer * primary );

static void ksGpuCommandBuffer_ChangeTextureUsage( ksGpuCommandBuffer * commandBuffer, ksGpuTexture * texture, const ksGpuTextureUsage usage );

static void ksGpuCommandBuffer_BeginFramebuffer( ksGpuCommandBuffer * commandBuffer, ksGpuFramebuffer * framebuffer, const int arrayLayer, const ksGpuTextureUsage usage );
static void ksGpuCommandBuffer_EndFramebuffer( ksGpuCommandBuffer * commandBuffer, ksGpuFramebuffer * framebuffer, const int arrayLayer, const ksGpuTextureUsage usage );

static void ksGpuCommandBuffer_BeginTimer( ksGpuCommandBuffer * commandBuffer, ksGpuTimer * timer );
static void ksGpuCommandBuffer_EndTimer( ksGpuCommandBuffer * commandBuffer, ksGpuTimer * timer );

static void ksGpuCommandBuffer_BeginRenderPass( ksGpuCommandBuffer * commandBuffer, ksGpuRenderPass * renderPass, ksGpuFramebuffer * framebuffer, const ksScreenRect * rect );
static void ksGpuCommandBuffer_EndRenderPass( ksGpuCommandBuffer * commandBuffer, ksGpuRenderPass * renderPass );

static void ksGpuCommandBuffer_SetViewport( ksGpuCommandBuffer * commandBuffer, const ksScreenRect * rect );
static void ksGpuCommandBuffer_SetScissor( ksGpuCommandBuffer * commandBuffer, const ksScreenRect * rect );

static void ksGpuCommandBuffer_SubmitGraphicsCommand( ksGpuCommandBuffer * commandBuffer, const ksGpuGraphicsCommand * command );
static void ksGpuCommandBuffer_SubmitComputeCommand( ksGpuCommandBuffer * commandBuffer, const ksGpuComputeCommand * command );

static ksGpuBuffer * ksGpuCommandBuffer_MapBuffer( ksGpuCommandBuffer * commandBuffer, ksGpuBuffer * buffer, void ** data );
static void ksGpuCommandBuffer_UnmapBuffer( ksGpuCommandBuffer * commandBuffer, ksGpuBuffer * buffer, ksGpuBuffer * mappedBuffer, const ksGpuBufferUnmapType type );

static ksGpuBuffer * ksGpuCommandBuffer_MapVertexAttributes( ksGpuCommandBuffer * commandBuffer, ksGpuGeometry * geometry, ksGpuVertexAttributeArrays * attribs );
static void ksGpuCommandBuffer_UnmapVertexAttributes( ksGpuCommandBuffer * commandBuffer, ksGpuGeometry * geometry, ksGpuBuffer * mappedVertexBuffer, const ksGpuBufferUnmapType type );

static ksGpuBuffer * ksGpuCommandBuffer_MapInstanceAttributes( ksGpuCommandBuffer * commandBuffer, ksGpuGeometry * geometry, ksGpuVertexAttributeArrays * attribs );
static void ksGpuCommandBuffer_UnmapInstanceAttributes( ksGpuCommandBuffer * commandBuffer, ksGpuGeometry * geometry, ksGpuBuffer * mappedInstanceBuffer, const ksGpuBufferUnmapType type );

================================================================================================================================
*/

typedef enum
{
	KS_GPU_BUFFER_UNMAP_TYPE_USE_ALLOCATED,		// use the newly allocated (host visible) buffer
	KS_GPU_BUFFER_UNMAP_TYPE_COPY_BACK			// copy back to the original buffer
} ksGpuBufferUnmapType;

typedef enum
{
	KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY,
	KS_GPU_COMMAND_BUFFER_TYPE_SECONDARY,
	KS_GPU_COMMAND_BUFFER_TYPE_SECONDARY_CONTINUE_RENDER_PASS
} ksGpuCommandBufferType;

#define MAX_COMMAND_BUFFER_TIMERS	16

typedef struct
{
	ksGpuCommandBufferType		type;
	int							numBuffers;
	int							currentBuffer;
	VkCommandBuffer *			cmdBuffers;
	ksGpuContext *				context;
	ksGpuFence *				fences;
	ksGpuBuffer **				mappedBuffers;
	ksGpuBuffer **				oldMappedBuffers;
	ksGpuPipelineResources **	pipelineResources;
	ksGpuSwapchainBuffer *		swapchainBuffer;
	ksGpuGraphicsCommand		currentGraphicsState;
	ksGpuComputeCommand			currentComputeState;
	ksGpuFramebuffer *			currentFramebuffer;
	ksGpuRenderPass *			currentRenderPass;
	ksGpuTimer *				currentTimers[MAX_COMMAND_BUFFER_TIMERS];
	int							currentTimerCount;
} ksGpuCommandBuffer;

#define MAX_VERTEX_BUFFER_UNUSED_COUNT			16
#define MAX_PIPELINE_RESOURCES_UNUSED_COUNT		16

static void ksGpuCommandBuffer_Create( ksGpuContext * context, ksGpuCommandBuffer * commandBuffer, const ksGpuCommandBufferType type, const int numBuffers )
{
	memset( commandBuffer, 0, sizeof( ksGpuCommandBuffer ) );

	commandBuffer->type = type;
	commandBuffer->numBuffers = numBuffers;
	commandBuffer->currentBuffer = 0;
	commandBuffer->context = context;
	commandBuffer->cmdBuffers = (VkCommandBuffer *) malloc( numBuffers * sizeof( VkCommandBuffer ) );
	commandBuffer->fences = (ksGpuFence *) malloc( numBuffers * sizeof( ksGpuFence ) );
	commandBuffer->mappedBuffers = (ksGpuBuffer **) malloc( numBuffers * sizeof( ksGpuBuffer * ) );
	commandBuffer->oldMappedBuffers = (ksGpuBuffer **) malloc( numBuffers * sizeof( ksGpuBuffer * ) );
	commandBuffer->pipelineResources = (ksGpuPipelineResources **) malloc( numBuffers * sizeof( ksGpuPipelineResources * ) );

	for ( int i = 0; i < numBuffers; i++ )
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.pNext = NULL;
		commandBufferAllocateInfo.commandPool = context->commandPool;
		commandBufferAllocateInfo.level = ( type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY ) ?
												VK_COMMAND_BUFFER_LEVEL_PRIMARY :
												VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		commandBufferAllocateInfo.commandBufferCount = 1;

		VK( context->device->vkAllocateCommandBuffers( context->device->device, &commandBufferAllocateInfo, &commandBuffer->cmdBuffers[i] ) );

		ksGpuFence_Create( context, &commandBuffer->fences[i] );

		commandBuffer->mappedBuffers[i] = NULL;
		commandBuffer->oldMappedBuffers[i] = NULL;
		commandBuffer->pipelineResources[i] = NULL;
	}
}

static void ksGpuCommandBuffer_Destroy( ksGpuContext * context, ksGpuCommandBuffer * commandBuffer )
{
	assert( context == commandBuffer->context );

	for ( int i = 0; i < commandBuffer->numBuffers; i++ )
	{
		VC( context->device->vkFreeCommandBuffers( context->device->device, context->commandPool, 1, &commandBuffer->cmdBuffers[i] ) );

		ksGpuFence_Destroy( context, &commandBuffer->fences[i] );

		for ( ksGpuBuffer * b = commandBuffer->mappedBuffers[i], * next = NULL; b != NULL; b = next )
		{
			next = b->next;
			ksGpuBuffer_Destroy( context, b );
			free( b );
		}
		commandBuffer->mappedBuffers[i] = NULL;

		for ( ksGpuBuffer * b = commandBuffer->oldMappedBuffers[i], * next = NULL; b != NULL; b = next )
		{
			next = b->next;
			ksGpuBuffer_Destroy( context, b );
			free( b );
		}
		commandBuffer->oldMappedBuffers[i] = NULL;

		for ( ksGpuPipelineResources * r = commandBuffer->pipelineResources[i], * next = NULL; r != NULL; r = next )
		{
			next = r->next;
			ksGpuPipelineResources_Destroy( context, r );
			free( r );
		}
		commandBuffer->pipelineResources[i] = NULL;
	}

	free( commandBuffer->pipelineResources );
	free( commandBuffer->oldMappedBuffers );
	free( commandBuffer->mappedBuffers );
	free( commandBuffer->fences );
	free( commandBuffer->cmdBuffers );

	memset( commandBuffer, 0, sizeof( ksGpuCommandBuffer ) );
}

static void ksGpuCommandBuffer_ManageBuffers( ksGpuCommandBuffer * commandBuffer )
{
	//
	// Manage buffers.
	//

	{
		// Free any old buffers that were not reused for a number of frames.
		for ( ksGpuBuffer ** b = &commandBuffer->oldMappedBuffers[commandBuffer->currentBuffer]; *b != NULL; )
		{
			if ( (*b)->unusedCount++ >= MAX_VERTEX_BUFFER_UNUSED_COUNT )
			{
				ksGpuBuffer * next = (*b)->next;
				ksGpuBuffer_Destroy( commandBuffer->context, *b );
				free( *b );
				*b = next;
			}
			else
			{
				b = &(*b)->next;
			}
		}

		// Move the last used buffers to the list with old buffers.
		for ( ksGpuBuffer * b = commandBuffer->mappedBuffers[commandBuffer->currentBuffer], * next = NULL; b != NULL; b = next )
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
		for ( ksGpuPipelineResources ** r = &commandBuffer->pipelineResources[commandBuffer->currentBuffer]; *r != NULL; )
		{
			if ( (*r)->unusedCount++ >= MAX_PIPELINE_RESOURCES_UNUSED_COUNT )
			{
				ksGpuPipelineResources * next = (*r)->next;
				ksGpuPipelineResources_Destroy( commandBuffer->context, *r );
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

static void ksGpuCommandBuffer_ManageTimers( ksGpuCommandBuffer * commandBuffer )
{
	ksGpuDevice * device = commandBuffer->context->device;

	for ( int i = 0; i < commandBuffer->currentTimerCount; i++ )
	{
		ksGpuTimer * timer = commandBuffer->currentTimers[i];
		if (timer->supported)
		{
			timer->index = ( timer->index + 1 ) % ( KS_GPU_TIMER_FRAMES_DELAYED + 1 );
			if ( timer->init >= KS_GPU_TIMER_FRAMES_DELAYED )
			{
				VC( device->vkGetQueryPoolResults( commandBuffer->context->device->device, timer->pool, timer->index * 2, 2,
							2 * sizeof( uint64_t ), timer->data, sizeof( uint64_t ), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT ) );
			}
			else
			{
				timer->index = ( timer->index + 1 ) % ( KS_GPU_TIMER_FRAMES_DELAYED + 1 );
				if ( timer->init >= KS_GPU_TIMER_FRAMES_DELAYED )
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
		}
	}
	commandBuffer->currentTimerCount = 0;
}

static void ksGpuCommandBuffer_BeginPrimary( ksGpuCommandBuffer * commandBuffer )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );

	ksGpuDevice * device = commandBuffer->context->device;

	commandBuffer->currentBuffer = ( commandBuffer->currentBuffer + 1 ) % commandBuffer->numBuffers;

	ksGpuFence * fence = &commandBuffer->fences[commandBuffer->currentBuffer];
	if ( fence->submitted )
	{
		VK( device->vkWaitForFences( device->device, 1, &fence->fence, VK_TRUE, 1ULL * 1000 * 1000 * 1000 ) );
		VK( device->vkResetFences( device->device, 1, &fence->fence ) );
		fence->submitted = false;
	}

	ksGpuCommandBuffer_ManageBuffers( commandBuffer );

	ksGpuGraphicsCommand_Init( &commandBuffer->currentGraphicsState );
	ksGpuComputeCommand_Init( &commandBuffer->currentComputeState );

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

static void ksGpuCommandBuffer_EndPrimary( ksGpuCommandBuffer * commandBuffer )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );

	ksGpuCommandBuffer_ManageTimers( commandBuffer );

	ksGpuDevice * device = commandBuffer->context->device;
	VK( device->vkEndCommandBuffer( commandBuffer->cmdBuffers[commandBuffer->currentBuffer] ) );
}

static ksGpuFence * ksGpuCommandBuffer_SubmitPrimary( ksGpuCommandBuffer * commandBuffer )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );

	ksGpuDevice * device = commandBuffer->context->device;

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

	ksGpuFence * fence = &commandBuffer->fences[commandBuffer->currentBuffer];
	VK( device->vkQueueSubmit( commandBuffer->context->queue, 1, &submitInfo, fence->fence ) );
	ksGpuFence_Submit( commandBuffer->context, fence );

	commandBuffer->swapchainBuffer = NULL;

	return fence;
}

static void ksGpuCommandBuffer_BeginSecondary( ksGpuCommandBuffer * commandBuffer, ksGpuRenderPass * renderPass, ksGpuFramebuffer * framebuffer )
{
	assert( commandBuffer->type != KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );

	ksGpuDevice * device = commandBuffer->context->device;

	commandBuffer->currentBuffer = ( commandBuffer->currentBuffer + 1 ) % commandBuffer->numBuffers;

	ksGpuCommandBuffer_ManageBuffers( commandBuffer );

	ksGpuGraphicsCommand_Init( &commandBuffer->currentGraphicsState );
	ksGpuComputeCommand_Init( &commandBuffer->currentComputeState );

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
	commandBufferBeginInfo.flags = ( ( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_SECONDARY_CONTINUE_RENDER_PASS ) ?
										VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0 ) |
										VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	commandBufferBeginInfo.pInheritanceInfo = &commandBufferInheritanceInfo;

	VK( device->vkBeginCommandBuffer( commandBuffer->cmdBuffers[commandBuffer->currentBuffer], &commandBufferBeginInfo ) );

	commandBuffer->currentRenderPass = renderPass;
}

static void ksGpuCommandBuffer_EndSecondary( ksGpuCommandBuffer * commandBuffer )
{
	assert( commandBuffer->type != KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );

	ksGpuCommandBuffer_ManageTimers( commandBuffer );
	commandBuffer->currentRenderPass = NULL;

	ksGpuDevice * device = commandBuffer->context->device;
	VK( device->vkEndCommandBuffer( commandBuffer->cmdBuffers[commandBuffer->currentBuffer] ) );
}

static void ksGpuCommandBuffer_SubmitSecondary( ksGpuCommandBuffer * commandBuffer, ksGpuCommandBuffer * primary )
{
	assert( commandBuffer->type != KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( primary->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( ( primary->currentRenderPass != NULL ) == ( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_SECONDARY_CONTINUE_RENDER_PASS ) );

	ksGpuDevice * device = commandBuffer->context->device;

	VC( device->vkCmdExecuteCommands( primary->cmdBuffers[primary->currentBuffer], 1, &commandBuffer->cmdBuffers[commandBuffer->currentBuffer] ) );
}

static void ksGpuCommandBuffer_ChangeTextureUsage( ksGpuCommandBuffer * commandBuffer, ksGpuTexture * texture, const ksGpuTextureUsage usage )
{
	ksGpuTexture_ChangeUsage( commandBuffer->context, commandBuffer->cmdBuffers[commandBuffer->currentBuffer], texture, usage );
}

static void ksGpuCommandBuffer_BeginFramebuffer( ksGpuCommandBuffer * commandBuffer, ksGpuFramebuffer * framebuffer, const int arrayLayer, const ksGpuTextureUsage usage )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );
	assert( arrayLayer >= 0 && arrayLayer < framebuffer->numLayers );

	if ( framebuffer->window != NULL )
	{
		assert( framebuffer->window->swapchain.swapchain != VK_NULL_HANDLE );
		if ( framebuffer->swapchainCreateCount != framebuffer->window->swapchainCreateCount )
		{
			ksGpuWindow * window = framebuffer->window;
			ksGpuRenderPass * renderPass = framebuffer->renderPass;
			ksGpuFramebuffer_Destroy( commandBuffer->context, framebuffer );
			ksGpuFramebuffer_CreateFromSwapchain( window, framebuffer, renderPass );
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

	ksGpuCommandBuffer_ChangeTextureUsage( commandBuffer, &framebuffer->colorTextures[framebuffer->currentBuffer], usage );

	commandBuffer->currentFramebuffer = framebuffer;
}

static void ksGpuCommandBuffer_EndFramebuffer( ksGpuCommandBuffer * commandBuffer, ksGpuFramebuffer * framebuffer, const int arrayLayer, const ksGpuTextureUsage usage )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == framebuffer );
	assert( commandBuffer->currentRenderPass == NULL );
	assert( arrayLayer >= 0 && arrayLayer < framebuffer->numLayers );

	UNUSED_PARM( arrayLayer );

#if EXPLICIT_RESOLVE != 0
	if ( framebuffer->renderTexture.image != VK_NULL_HANDLE )
	{
		ksGpuCommandBuffer_ChangeTextureUsage( commandBuffer, &framebuffer->renderTexture, KS_GPU_TEXTURE_USAGE_TRANSFER_SRC );
		ksGpuCommandBuffer_ChangeTextureUsage( commandBuffer, &framebuffer->colorTextures[framebuffer->currentBuffer], KS_GPU_TEXTURE_USAGE_TRANSFER_DST );

		VkImageResolve region;
		region.srcOffset.x = 0;
		region.srcOffset.y = 0;
		region.srcOffset.z = 0;
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = arrayLayer;
		region.srcSubresource.layerCount = 1;
		region.dstOffset.x = 0;
		region.dstOffset.y = 0;
		region.dstOffset.z = 0;
		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.mipLevel = 0;
		region.dstSubresource.baseArrayLayer = arrayLayer;
		region.dstSubresource.layerCount = 1;
		region.extent.width = framebuffer->renderTexture.width;
		region.extent.height = framebuffer->renderTexture.height;
		region.extent.depth = framebuffer->renderTexture.depth;

		commandBuffer->context->device->vkCmdResolveImage( commandBuffer->cmdBuffers[commandBuffer->currentBuffer],
					framebuffer->renderTexture.image, framebuffer->renderTexture.imageLayout,
					framebuffer->colorTextures[framebuffer->currentBuffer].image, framebuffer->colorTextures[framebuffer->currentBuffer].imageLayout,
					1, &region );

		ksGpuCommandBuffer_ChangeTextureUsage( commandBuffer, &framebuffer->renderTexture, KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );
	}
#endif

	ksGpuCommandBuffer_ChangeTextureUsage( commandBuffer, &framebuffer->colorTextures[framebuffer->currentBuffer], usage );

	commandBuffer->currentFramebuffer = NULL;
}

static void ksGpuCommandBuffer_BeginTimer( ksGpuCommandBuffer * commandBuffer, ksGpuTimer * timer )
{
	ksGpuDevice * device = commandBuffer->context->device;

	if ( !timer->supported )
	{
		return;
	}

	// Make sure this timer has not already been used.
	for ( int i = 0; i < commandBuffer->currentTimerCount; i++ )
	{
		assert( commandBuffer->currentTimers[i] != timer );
	}

	VC( device->vkCmdWriteTimestamp( commandBuffer->cmdBuffers[commandBuffer->currentBuffer], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, timer->pool, timer->index * 2 + 0 ) );
}

static void ksGpuCommandBuffer_EndTimer( ksGpuCommandBuffer * commandBuffer, ksGpuTimer * timer )
{
	ksGpuDevice * device = commandBuffer->context->device;

	if ( !timer->supported )
	{
		return;
	}

	VC( device->vkCmdWriteTimestamp( commandBuffer->cmdBuffers[commandBuffer->currentBuffer], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, timer->pool, timer->index * 2 + 1 ) );

	assert( commandBuffer->currentTimerCount < MAX_COMMAND_BUFFER_TIMERS );
	commandBuffer->currentTimers[commandBuffer->currentTimerCount++] = timer;
}

static void ksGpuCommandBuffer_BeginRenderPass( ksGpuCommandBuffer * commandBuffer, ksGpuRenderPass * renderPass, ksGpuFramebuffer * framebuffer, const ksScreenRect * rect )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentRenderPass == NULL );
	assert( commandBuffer->currentFramebuffer == framebuffer );

	ksGpuDevice * device = commandBuffer->context->device;

	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];

	uint32_t clearValueCount = 0;
	VkClearValue clearValues[3];
	memset( clearValues, 0, sizeof( clearValues ) );

	if ( renderPass->sampleCount > KS_GPU_SAMPLE_COUNT_1 )
	{
		clearValues[clearValueCount].color.float32[0] = 0.0f;
		clearValues[clearValueCount].color.float32[1] = 0.0f;
		clearValues[clearValueCount].color.float32[2] = 0.0f;
		clearValues[clearValueCount].color.float32[3] = 1.0f;
		clearValueCount++;
	}
	if ( renderPass->sampleCount <= KS_GPU_SAMPLE_COUNT_1 || EXPLICIT_RESOLVE == 0 )
	{
		clearValues[clearValueCount].color.float32[0] = 0.0f;
		clearValues[clearValueCount].color.float32[1] = 0.0f;
		clearValues[clearValueCount].color.float32[2] = 0.0f;
		clearValues[clearValueCount].color.float32[3] = 1.0f;
		clearValueCount++;
	}
	if ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED )
	{
		clearValues[clearValueCount].depthStencil.depth = 1.0f;
		clearValues[clearValueCount].depthStencil.stencil = 0;
		clearValueCount++;
	}

	VkRenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = NULL;
	renderPassBeginInfo.renderPass = renderPass->renderPass;
	renderPassBeginInfo.framebuffer = framebuffer->framebuffers[framebuffer->currentBuffer * framebuffer->numLayers + framebuffer->currentLayer ];
	renderPassBeginInfo.renderArea.offset.x = rect->x;
	renderPassBeginInfo.renderArea.offset.y = rect->y;
	renderPassBeginInfo.renderArea.extent.width = rect->width;
	renderPassBeginInfo.renderArea.extent.height = rect->height;
	renderPassBeginInfo.clearValueCount = clearValueCount;
	renderPassBeginInfo.pClearValues = clearValues;

	VkSubpassContents contents = ( renderPass->type == KS_GPU_RENDERPASS_TYPE_INLINE ) ?
									VK_SUBPASS_CONTENTS_INLINE :
									VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;

	VC( device->vkCmdBeginRenderPass( cmdBuffer, &renderPassBeginInfo, contents ) );

	commandBuffer->currentRenderPass = renderPass;
}

static void ksGpuCommandBuffer_EndRenderPass( ksGpuCommandBuffer * commandBuffer, ksGpuRenderPass * renderPass )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentRenderPass == renderPass );

	UNUSED_PARM( renderPass );

	ksGpuDevice * device = commandBuffer->context->device;

	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];

	VC( device->vkCmdEndRenderPass( cmdBuffer ) );

	commandBuffer->currentRenderPass = NULL;
}

static void ksGpuCommandBuffer_SetViewport( ksGpuCommandBuffer * commandBuffer, const ksScreenRect * rect )
{
	ksGpuDevice * device = commandBuffer->context->device;

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

static void ksGpuCommandBuffer_SetScissor( ksGpuCommandBuffer * commandBuffer, const ksScreenRect * rect )
{
	ksGpuDevice * device = commandBuffer->context->device;

	VkRect2D scissor;
	scissor.offset.x = rect->x;
	scissor.offset.y = rect->y;
	scissor.extent.width = rect->width;
	scissor.extent.height = rect->height;

	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];
	VC( device->vkCmdSetScissor( cmdBuffer, 0, 1, &scissor ) );
}

static void ksGpuCommandBuffer_UpdateProgramParms( ksGpuCommandBuffer * commandBuffer,
												const ksGpuProgramParmLayout * newLayout,
												const ksGpuProgramParmLayout * oldLayout,
												const ksGpuProgramParmState * newParmState,
												const ksGpuProgramParmState * oldParmState,
												VkPipelineBindPoint bindPoint )
{
	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];
	ksGpuDevice * device = commandBuffer->context->device;

	const bool descriptorsMatch = ksGpuProgramParmState_DescriptorsMatch( newLayout, newParmState, oldLayout, oldParmState );
	if ( !descriptorsMatch )
	{
		// Try to find existing resources that match.
		ksGpuPipelineResources * resources = NULL;
		for ( ksGpuPipelineResources * r = commandBuffer->pipelineResources[commandBuffer->currentBuffer]; r != NULL; r = r->next )
		{
			if ( ksGpuProgramParmState_DescriptorsMatch( newLayout, newParmState, r->parmLayout, &r->parms ) )
			{
				r->unusedCount = 0;
				resources = r;
				break;
			}
		}

		// Create new resources if none were found.
		if ( resources == NULL )
		{
			resources = (ksGpuPipelineResources *) malloc( sizeof( ksGpuPipelineResources ) );
			ksGpuPipelineResources_Create( commandBuffer->context, resources, newLayout, newParmState );
			resources->next = commandBuffer->pipelineResources[commandBuffer->currentBuffer];
			commandBuffer->pipelineResources[commandBuffer->currentBuffer] = resources;	
		}

		VC( device->vkCmdBindDescriptorSets( cmdBuffer, bindPoint, newLayout->pipelineLayout,
										0, 1, &resources->descriptorSet, 0, NULL ) );
	}

	for ( int i = 0; i < newLayout->numPushConstants; i++ )
	{
		const void * data = ksGpuProgramParmState_NewPushConstantData( newLayout, i, newParmState, oldLayout, i, oldParmState, false );
		if ( data != NULL )
		{
			const ksGpuProgramParm * newParm = newLayout->pushConstants[i];
			const VkShaderStageFlags stageFlags = ksGpuProgramParm_GetShaderStageFlags( newParm->stageFlags );
			const uint32_t offset = (uint32_t) newParm->binding;
			const uint32_t size = (uint32_t) ksGpuProgramParm_GetPushConstantSize( newParm->type );
			VC( device->vkCmdPushConstants( cmdBuffer, newLayout->pipelineLayout, stageFlags, offset, size, data ) );
		}
	}
}

static void ksGpuCommandBuffer_SubmitGraphicsCommand( ksGpuCommandBuffer * commandBuffer, const ksGpuGraphicsCommand * command )
{
	assert( commandBuffer->currentRenderPass != NULL );

	ksGpuDevice * device = commandBuffer->context->device;

	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];
	const ksGpuGraphicsCommand * state = &commandBuffer->currentGraphicsState;

	// If the pipeline has changed.
	if ( command->pipeline != state->pipeline )
	{
		VC( device->vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, command->pipeline->pipeline ) );
	}

	const ksGpuProgramParmLayout * commandLayout = &command->pipeline->program->parmLayout;
	const ksGpuProgramParmLayout * stateLayout = ( state->pipeline != NULL ) ? &state->pipeline->program->parmLayout : NULL;

	ksGpuCommandBuffer_UpdateProgramParms( commandBuffer, commandLayout, stateLayout, &command->parmState, &state->parmState, VK_PIPELINE_BIND_POINT_GRAPHICS );

	const ksGpuGeometry * geometry = command->pipeline->geometry;

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

		const VkIndexType indexType = ( sizeof( ksGpuTriangleIndex ) == sizeof( unsigned int ) ) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
		VC( device->vkCmdBindIndexBuffer( cmdBuffer, geometry->indexBuffer.buffer, 0, indexType ) );
	}

	VC( device->vkCmdDrawIndexed( cmdBuffer, geometry->indexCount, command->numInstances, 0, 0, 0 ) );

	commandBuffer->currentGraphicsState = *command;
}

static void ksGpuCommandBuffer_SubmitComputeCommand( ksGpuCommandBuffer * commandBuffer, const ksGpuComputeCommand * command )
{
	assert( commandBuffer->currentRenderPass == NULL );

	ksGpuDevice * device = commandBuffer->context->device;

	VkCommandBuffer cmdBuffer = commandBuffer->cmdBuffers[commandBuffer->currentBuffer];
	const ksGpuComputeCommand * state = &commandBuffer->currentComputeState;

	// If the pipeline has changed.
	if ( command->pipeline != state->pipeline )
	{
		VC( device->vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, command->pipeline->pipeline ) );
	}

	const ksGpuProgramParmLayout * commandLayout = &command->pipeline->program->parmLayout;
	const ksGpuProgramParmLayout * stateLayout = ( state->pipeline != NULL ) ? &state->pipeline->program->parmLayout : NULL;

	ksGpuCommandBuffer_UpdateProgramParms( commandBuffer, commandLayout, stateLayout, &command->parmState, &state->parmState, VK_PIPELINE_BIND_POINT_COMPUTE );

	VC( device->vkCmdDispatch( cmdBuffer, command->x, command->y, command->z ) );

	commandBuffer->currentComputeState = *command;
}

static ksGpuBuffer * ksGpuCommandBuffer_MapBuffer( ksGpuCommandBuffer * commandBuffer, ksGpuBuffer * buffer, void ** data )
{
	assert( commandBuffer->currentRenderPass == NULL );

	ksGpuDevice * device = commandBuffer->context->device;

	ksGpuBuffer * newBuffer = NULL;
	for ( ksGpuBuffer ** b = &commandBuffer->oldMappedBuffers[commandBuffer->currentBuffer]; *b != NULL; b = &(*b)->next )
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
		newBuffer = (ksGpuBuffer *) malloc( sizeof( ksGpuBuffer ) );
		ksGpuBuffer_Create( commandBuffer->context, newBuffer, buffer->type, buffer->size, NULL, true );
	}

	newBuffer->unusedCount = 0;
	newBuffer->next = commandBuffer->mappedBuffers[commandBuffer->currentBuffer];
	commandBuffer->mappedBuffers[commandBuffer->currentBuffer] = newBuffer;

	assert( newBuffer->mapped == NULL );
	VK( device->vkMapMemory( commandBuffer->context->device->device, newBuffer->memory, 0, newBuffer->size, 0, &newBuffer->mapped ) );

	*data = newBuffer->mapped;

	return newBuffer;
}

static void ksGpuCommandBuffer_UnmapBuffer( ksGpuCommandBuffer * commandBuffer, ksGpuBuffer * buffer, ksGpuBuffer * mappedBuffer, const ksGpuBufferUnmapType type )
{
	// Can only copy or issue memory barrier outside a render pass.
	assert( commandBuffer->currentRenderPass == NULL );

	ksGpuDevice * device = commandBuffer->context->device;

	VC( device->vkUnmapMemory( commandBuffer->context->device->device, mappedBuffer->memory ) );
	mappedBuffer->mapped = NULL;

	// Optionally copy the mapped buffer back to the original buffer. While the copy is not for free,
	// there may be a performance benefit from using the original buffer if it lives in device local memory.
	if ( type == KS_GPU_BUFFER_UNMAP_TYPE_COPY_BACK )
	{
		assert( buffer->size == mappedBuffer->size );

		{
			// Add a memory barrier for the destination buffer from shader access to DMA write
			VkBufferMemoryBarrier bufferMemoryBarrier;
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.pNext = NULL;
			bufferMemoryBarrier.srcAccessMask = ksGpuBuffer_GetBufferAccess( buffer->type );
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.buffer = buffer->buffer;
			bufferMemoryBarrier.offset = 0;
			bufferMemoryBarrier.size = buffer->size;

			const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
			const VkDependencyFlags flags = 0;

			device->vkCmdPipelineBarrier( commandBuffer->cmdBuffers[commandBuffer->currentBuffer],
											src_stages, dst_stages, flags, 0, NULL, 1, &bufferMemoryBarrier, 0, NULL );
		}

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

			const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_HOST_BIT;
			const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
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
			bufferMemoryBarrier.dstAccessMask = ksGpuBuffer_GetBufferAccess( buffer->type );
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.buffer = buffer->buffer;
			bufferMemoryBarrier.offset = 0;
			bufferMemoryBarrier.size = buffer->size;

			const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
			const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
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
			bufferMemoryBarrier.dstAccessMask = ksGpuBuffer_GetBufferAccess( mappedBuffer->type );
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

static ksGpuBuffer * ksGpuCommandBuffer_MapVertexAttributes( ksGpuCommandBuffer * commandBuffer, ksGpuGeometry * geometry, ksGpuVertexAttributeArrays * attribs )
{
	void * data = NULL;
	ksGpuBuffer * buffer = ksGpuCommandBuffer_MapBuffer( commandBuffer, &geometry->vertexBuffer, &data );

	attribs->layout = geometry->layout;
	ksGpuVertexAttributeArrays_Map( attribs, data, buffer->size, geometry->vertexCount, geometry->vertexAttribsFlags );

	return buffer;
}

static void ksGpuCommandBuffer_UnmapVertexAttributes( ksGpuCommandBuffer * commandBuffer, ksGpuGeometry * geometry, ksGpuBuffer * mappedVertexBuffer, const ksGpuBufferUnmapType type )
{
	ksGpuCommandBuffer_UnmapBuffer( commandBuffer, &geometry->vertexBuffer, mappedVertexBuffer, type );
}

static ksGpuBuffer * ksGpuCommandBuffer_MapInstanceAttributes( ksGpuCommandBuffer * commandBuffer, ksGpuGeometry * geometry, ksGpuVertexAttributeArrays * attribs )
{
	void * data = NULL;
	ksGpuBuffer * buffer = ksGpuCommandBuffer_MapBuffer( commandBuffer, &geometry->instanceBuffer, &data );

	attribs->layout = geometry->layout;
	ksGpuVertexAttributeArrays_Map( attribs, data, buffer->size, geometry->instanceCount, geometry->instanceAttribsFlags );

	return buffer;
}

static void ksGpuCommandBuffer_UnmapInstanceAttributes( ksGpuCommandBuffer * commandBuffer, ksGpuGeometry * geometry, ksGpuBuffer * mappedInstanceBuffer, const ksGpuBufferUnmapType type )
{
	ksGpuCommandBuffer_UnmapBuffer( commandBuffer, &geometry->instanceBuffer, mappedInstanceBuffer, type );
}

/*
================================================================================================================================

Bar graph.

Real-time bar graph where new bars scroll in on the right and old bars scroll out on the left.
Optionally supports stacking of bars. A bar value is in the range [0, 1] where 1 is a full height bar.
The bar graph position x,y,width,height is specified in clip coordinates in the range [-1, 1].

ksBarGraph

static void ksBarGraph_Create( ksGpuContext * context, ksBarGraph * barGraph, ksGpuRenderPass * renderPass,
								const float x, const float y, const float width, const float height,
								const int numBars, const int numStacked, const ksVector4f * backgroundColor );
static void ksBarGraph_Destroy( ksGpuContext * context, ksBarGraph * barGraph );
static void ksBarGraph_AddBar( ksBarGraph * barGraph, const int stackedBar, const float value, const ksVector4f * color, const bool advance );

static void ksBarGraph_UpdateGraphics( ksGpuCommandBuffer * commandBuffer, ksBarGraph * barGraph );
static void ksBarGraph_RenderGraphics( ksGpuCommandBuffer * commandBuffer, ksBarGraph * barGraph );

static void ksBarGraph_UpdateCompute( ksGpuCommandBuffer * commandBuffer, ksBarGraph * barGraph );
static void ksBarGraph_RenderCompute( ksGpuCommandBuffer * commandBuffer, ksBarGraph * barGraph, ksGpuFramebuffer * framebuffer );

================================================================================================================================
*/

typedef struct
{
	ksClipRect			clipRect;
	int					numBars;
	int					numStacked;
	int					barIndex;
	float *				barValues;
	ksVector4f *		barColors;
	ksVector4f			backgroundColor;
	struct
	{
		ksGpuGeometry			quad;
		ksGpuGraphicsProgram	program;
		ksGpuGraphicsPipeline	pipeline;
		int						numInstances;
	} graphics;
	struct
	{
		ksGpuBuffer				barValueBuffer;
		ksGpuBuffer				barColorBuffer;
		ksVector2i				barGraphOffset;
		ksGpuComputeProgram		program;
		ksGpuComputePipeline	pipeline;
	} compute;
} ksBarGraph;

static const ksGpuProgramParm barGraphGraphicsProgramParms[] =
{
	{ 0 }
};

static const char barGraphVertexProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( location = 0 ) in vec3 vertexPosition;\n"
	"layout( location = 1 ) in mat4 vertexTransform;\n"
	"layout( location = 0 ) out vec4 fragmentColor;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"vec3 multiply4x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[1].x * v.y + m[2].x * v.z + m[3].x,\n"
	"		m[0].y * v.x + m[1].y * v.y + m[2].y * v.z + m[3].y,\n"
	"		m[0].z * v.x + m[1].z * v.y + m[2].z * v.z + m[3].z );\n"
	"}\n"
	"void main( void )\n"
	"{\n"
	"	gl_Position.xyz = multiply4x3( vertexTransform, vertexPosition );\n"
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
	"#version " GLSL_VERSION "\n"
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

static const ksGpuProgramParm barGraphComputeProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,				KS_GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_BAR_GRAPH_DEST,					"dest",				0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_VALUES,			"barValueBuffer",	1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_COLORS,			"barColorBuffer",	2 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BACK_GROUND_COLOR,	"backgroundColor",	0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_GRAPH_OFFSET,		"barGraphOffset",	16 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_BARS,				"numBars",			24 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_STACKED,			"numStacked",		28 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_INDEX,			"barIndex",			32 }
};

#define BARGRAPH_LOCAL_SIZE_X	8
#define BARGRAPH_LOCAL_SIZE_Y	8

static const char barGraphComputeProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"\n"
	"layout( local_size_x = " STRINGIFY( BARGRAPH_LOCAL_SIZE_X ) ", local_size_y = " STRINGIFY( BARGRAPH_LOCAL_SIZE_Y ) " ) in;\n"
	"\n"
	"layout( rgba8, binding = 0 ) uniform highp writeonly image2D dest;\n"
	"layout( std430, binding = 1 ) buffer barValueBuffer { float barValues[]; };\n"
	"layout( std430, binding = 2 ) buffer barColorBuffer { vec4 barColors[]; };\n"
	"layout( std430, push_constant ) uniform buffer0\n"
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

static void ksBarGraph_Create( ksGpuContext * context, ksBarGraph * barGraph, ksGpuRenderPass * renderPass,
								const float x, const float y, const float width, const float height,
								const int numBars, const int numStacked, const ksVector4f * backgroundColor )
{
	barGraph->clipRect.x = x;
	barGraph->clipRect.y = y;
	barGraph->clipRect.width = width;
	barGraph->clipRect.height = height;
	barGraph->numBars = numBars;
	barGraph->numStacked = numStacked;
	barGraph->barIndex = 0;
	barGraph->barValues = (float *) AllocAlignedMemory( numBars * numStacked * sizeof( barGraph->barValues[0] ), sizeof( void * ) );
	barGraph->barColors = (ksVector4f *) AllocAlignedMemory( numBars * numStacked * sizeof( barGraph->barColors[0] ), sizeof( ksVector4f ) );

	for ( int i = 0; i < numBars * numStacked; i++ )
	{
		barGraph->barValues[i] = 0.0f;
		barGraph->barColors[i] = colorGreen;
	}

	barGraph->backgroundColor = *backgroundColor;

	// graphics
	{
		ksGpuGeometry_CreateQuad( context, &barGraph->graphics.quad, 1.0f, 0.5f );
		ksGpuGeometry_AddInstanceAttributes( context, &barGraph->graphics.quad, numBars * numStacked + 1, VERTEX_ATTRIBUTE_FLAG_TRANSFORM );

		ksGpuGraphicsProgram_Create( context, &barGraph->graphics.program,
									PROGRAM( barGraphVertexProgram ), sizeof( PROGRAM( barGraphVertexProgram ) ),
									PROGRAM( barGraphFragmentProgram ), sizeof( PROGRAM( barGraphFragmentProgram ) ),
									barGraphGraphicsProgramParms, 0,
									barGraph->graphics.quad.layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_TRANSFORM );

		ksGpuGraphicsPipelineParms pipelineParms;
		ksGpuGraphicsPipelineParms_Init( &pipelineParms );

		pipelineParms.rop.depthTestEnable = false;
		pipelineParms.rop.depthWriteEnable = false;
		pipelineParms.renderPass = renderPass;
		pipelineParms.program = &barGraph->graphics.program;
		pipelineParms.geometry = &barGraph->graphics.quad;

		ksGpuGraphicsPipeline_Create( context, &barGraph->graphics.pipeline, &pipelineParms );

		barGraph->graphics.numInstances = 0;
	}

	// compute
	{
		ksGpuBuffer_Create( context, &barGraph->compute.barValueBuffer, KS_GPU_BUFFER_TYPE_STORAGE,
							barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barValues[0] ), NULL, false );
		ksGpuBuffer_Create( context, &barGraph->compute.barColorBuffer, KS_GPU_BUFFER_TYPE_STORAGE,
							barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barColors[0] ), NULL, false );

		ksGpuComputeProgram_Create( context, &barGraph->compute.program,
									PROGRAM( barGraphComputeProgram ), sizeof( PROGRAM( barGraphComputeProgram ) ),
									barGraphComputeProgramParms, ARRAY_SIZE( barGraphComputeProgramParms ) );

		ksGpuComputePipeline_Create( context, &barGraph->compute.pipeline, &barGraph->compute.program );
	}
}

static void ksBarGraph_Destroy( ksGpuContext * context, ksBarGraph * barGraph )
{
	FreeAlignedMemory( barGraph->barValues );
	FreeAlignedMemory( barGraph->barColors );

	// graphics
	{
		ksGpuGraphicsPipeline_Destroy( context, &barGraph->graphics.pipeline );
		ksGpuGraphicsProgram_Destroy( context, &barGraph->graphics.program );
		ksGpuGeometry_Destroy( context, &barGraph->graphics.quad );
	}

	// compute
	{
		ksGpuComputePipeline_Destroy( context, &barGraph->compute.pipeline );
		ksGpuComputeProgram_Destroy( context, &barGraph->compute.program );
		ksGpuBuffer_Destroy( context, &barGraph->compute.barValueBuffer );
		ksGpuBuffer_Destroy( context, &barGraph->compute.barColorBuffer );
	}
}

static void ksBarGraph_AddBar( ksBarGraph * barGraph, const int stackedBar, const float value, const ksVector4f * color, const bool advance )
{
	assert( stackedBar >= 0 && stackedBar < barGraph->numStacked );
	barGraph->barValues[barGraph->barIndex * barGraph->numStacked + stackedBar] = value;
	barGraph->barColors[barGraph->barIndex * barGraph->numStacked + stackedBar] = *color;
	if ( advance )
	{
		barGraph->barIndex = ( barGraph->barIndex + 1 ) % barGraph->numBars;
	}
}

static void ksBarGraph_UpdateGraphics( ksGpuCommandBuffer * commandBuffer, ksBarGraph * barGraph )
{
	ksDefaultVertexAttributeArrays attribs;
	ksGpuBuffer * instanceBuffer = ksGpuCommandBuffer_MapInstanceAttributes( commandBuffer, &barGraph->graphics.quad, &attribs.base );

#if defined( GRAPHICS_API_VULKAN )
	const float flipY = -1.0f;
#else
	const float flipY = 1.0f;
#endif

	int numInstances = 0;
	ksMatrix4x4f * backgroundMatrix = &attribs.transform[numInstances++];

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

			ksMatrix4x4f * barMatrix = &attribs.transform[numInstances++];

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

	ksGpuCommandBuffer_UnmapInstanceAttributes( commandBuffer, &barGraph->graphics.quad, instanceBuffer, KS_GPU_BUFFER_UNMAP_TYPE_COPY_BACK );

	assert( numInstances <= barGraph->numBars * barGraph->numStacked + 1 );
	barGraph->graphics.numInstances = numInstances;
}

static void ksBarGraph_RenderGraphics( ksGpuCommandBuffer * commandBuffer, ksBarGraph * barGraph )
{
	ksGpuGraphicsCommand command;
	ksGpuGraphicsCommand_Init( &command );
	ksGpuGraphicsCommand_SetPipeline( &command, &barGraph->graphics.pipeline );
	ksGpuGraphicsCommand_SetNumInstances( &command, barGraph->graphics.numInstances );

	ksGpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
}

static void ksBarGraph_UpdateCompute( ksGpuCommandBuffer * commandBuffer, ksBarGraph * barGraph )
{
	void * barValues = NULL;
	ksGpuBuffer * mappedBarValueBuffer = ksGpuCommandBuffer_MapBuffer( commandBuffer, &barGraph->compute.barValueBuffer, &barValues );
	memcpy( barValues, barGraph->barValues, barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barValues[0] ) );
	ksGpuCommandBuffer_UnmapBuffer( commandBuffer, &barGraph->compute.barValueBuffer, mappedBarValueBuffer, KS_GPU_BUFFER_UNMAP_TYPE_COPY_BACK );

	void * barColors = NULL;
	ksGpuBuffer * mappedBarColorBuffer = ksGpuCommandBuffer_MapBuffer( commandBuffer, &barGraph->compute.barColorBuffer, &barColors );
	memcpy( barColors, barGraph->barColors, barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barColors[0] ) );
	ksGpuCommandBuffer_UnmapBuffer( commandBuffer, &barGraph->compute.barColorBuffer, mappedBarColorBuffer, KS_GPU_BUFFER_UNMAP_TYPE_COPY_BACK );
}

static void ksBarGraph_RenderCompute( ksGpuCommandBuffer * commandBuffer, ksBarGraph * barGraph, ksGpuFramebuffer * framebuffer )
{
	const int screenWidth = ksGpuFramebuffer_GetWidth( framebuffer );
	const int screenHeight = ksGpuFramebuffer_GetHeight( framebuffer );
	ksScreenRect screenRect = ksClipRect_ToScreenRect( &barGraph->clipRect, screenWidth, screenHeight );
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

	ksGpuComputeCommand command;
	ksGpuComputeCommand_Init( &command );
	ksGpuComputeCommand_SetPipeline( &command, &barGraph->compute.pipeline );
	ksGpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_BAR_GRAPH_DEST, ksGpuFramebuffer_GetColorTexture( framebuffer ) );
	ksGpuComputeCommand_SetParmBufferStorage( &command, COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_VALUES, &barGraph->compute.barValueBuffer );
	ksGpuComputeCommand_SetParmBufferStorage( &command, COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_COLORS, &barGraph->compute.barColorBuffer );
	ksGpuComputeCommand_SetParmFloatVector4( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BACK_GROUND_COLOR, &barGraph->backgroundColor );
	ksGpuComputeCommand_SetParmIntVector2( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_GRAPH_OFFSET, &barGraph->compute.barGraphOffset );
	ksGpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_BARS, &barGraph->numBars );
	ksGpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_STACKED, &barGraph->numStacked );
	ksGpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_INDEX, &barGraph->barIndex );
	ksGpuComputeCommand_SetDimensions( &command, screenRect.width / BARGRAPH_LOCAL_SIZE_X, screenRect.height / BARGRAPH_LOCAL_SIZE_Y, 1 );

	ksGpuCommandBuffer_SubmitComputeCommand( commandBuffer, &command );
}

/*
================================================================================================================================

Time warp bar graphs.

ksTimeWarpBarGraphs

static void ksTimeWarpBarGraphs_Create( ksGpuContext * context, ksTimeWarpBarGraphs * bargraphs, ksGpuRenderPass * renderPass );
static void ksTimeWarpBarGraphs_Destroy( ksGpuContext * context, ksTimeWarpBarGraphs * bargraphs );

static void ksTimeWarpBarGraphs_UpdateGraphics( ksGpuCommandBuffer * commandBuffer, ksTimeWarpBarGraphs * bargraphs );
static void ksTimeWarpBarGraphs_RenderGraphics( ksGpuCommandBuffer * commandBuffer, ksTimeWarpBarGraphs * bargraphs );

static void ksTimeWarpBarGraphs_UpdateCompute( ksGpuCommandBuffer * commandBuffer, ksTimeWarpBarGraphs * bargraphs );
static void ksTimeWarpBarGraphs_RenderCompute( ksGpuCommandBuffer * commandBuffer, ksTimeWarpBarGraphs * bargraphs, ksGpuFramebuffer * framebuffer );

static ksNanoseconds ksTimeWarpBarGraphs_GetGpuNanosecondsGraphics( ksTimeWarpBarGraphs * bargraphs );
static ksNanoseconds ksTimeWarpBarGraphs_GetGpuNanosecondsCompute( ksTimeWarpBarGraphs * bargraphs );

================================================================================================================================
*/

#define BARGRAPH_VIRTUAL_PIXELS_WIDE		1920
#define BARGRAPH_VIRTUAL_PIXELS_HIGH		1080

#if defined( OS_ANDROID )
#define BARGRAPH_INSET						64
#else
#define BARGRAPH_INSET						16
#endif

static const ksScreenRect eyeTextureFrameRateBarGraphRect			= { BARGRAPH_INSET + 0 * 264, BARGRAPH_INSET, 256, 128 };
static const ksScreenRect timeWarpFrameRateBarGraphRect				= { BARGRAPH_INSET + 1 * 264, BARGRAPH_INSET, 256, 128 };
static const ksScreenRect frameCpuTimeBarGraphRect					= { BARGRAPH_INSET + 2 * 264, BARGRAPH_INSET, 256, 128 };
static const ksScreenRect frameGpuTimeBarGraphRect					= { BARGRAPH_INSET + 3 * 264, BARGRAPH_INSET, 256, 128 };

static const ksScreenRect multiViewBarGraphRect						= { 2 * BARGRAPH_VIRTUAL_PIXELS_WIDE / 3 + 0 * 40, BARGRAPH_INSET, 32, 32 };
static const ksScreenRect correctChromaticAberrationBarGraphRect	= { 2 * BARGRAPH_VIRTUAL_PIXELS_WIDE / 3 + 1 * 40, BARGRAPH_INSET, 32, 32 };
static const ksScreenRect timeWarpImplementationBarGraphRect		= { 2 * BARGRAPH_VIRTUAL_PIXELS_WIDE / 3 + 2 * 40, BARGRAPH_INSET, 32, 32 };

static const ksScreenRect displayResolutionLevelBarGraphRect		= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 7 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };
static const ksScreenRect eyeImageResolutionLevelBarGraphRect		= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 6 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };
static const ksScreenRect eyeImageSamplesLevelBarGraphRect			= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 5 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };

static const ksScreenRect sceneDrawCallLevelBarGraphRect			= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 3 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };
static const ksScreenRect sceneTriangleLevelBarGraphRect			= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 2 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };
static const ksScreenRect sceneFragmentLevelBarGraphRect			= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 1 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };

typedef enum
{
	BAR_GRAPH_HIDDEN,
	BAR_GRAPH_VISIBLE,
	BAR_GRAPH_PAUSED
} ksBarGraphState;

typedef struct
{
	ksBarGraphState	barGraphState;

	ksBarGraph		applicationFrameRateGraph;
	ksBarGraph		timeWarpFrameRateGraph;
	ksBarGraph		frameCpuTimeBarGraph;
	ksBarGraph		frameGpuTimeBarGraph;

	ksBarGraph		multiViewBarGraph;
	ksBarGraph		correctChromaticAberrationBarGraph;
	ksBarGraph		timeWarpImplementationBarGraph;

	ksBarGraph		displayResolutionLevelBarGraph;
	ksBarGraph		eyeImageResolutionLevelBarGraph;
	ksBarGraph		eyeImageSamplesLevelBarGraph;

	ksBarGraph		sceneDrawCallLevelBarGraph;
	ksBarGraph		sceneTriangleLevelBarGraph;
	ksBarGraph		sceneFragmentLevelBarGraph;

	ksGpuTimer		barGraphTimer;
} ksTimeWarpBarGraphs;

enum
{
	PROFILE_TIME_APPLICATION,
	PROFILE_TIME_TIME_WARP,
	PROFILE_TIME_BAR_GRAPHS,
	PROFILE_TIME_BLIT,
	PROFILE_TIME_OVERFLOW,
	PROFILE_TIME_MAX
};

static const ksVector4f * profileTimeBarColors[] =
{
	&colorPurple,
	&colorGreen,
	&colorYellow,
	&colorBlue,
	&colorRed
};

static void ksBarGraph_CreateVirtualRect( ksGpuContext * context, ksBarGraph * barGraph, ksGpuRenderPass * renderPass,
						const ksScreenRect * virtualRect, const int numBars, const int numStacked, const ksVector4f * backgroundColor )
{
	const ksClipRect clipRect = ksScreenRect_ToClipRect( virtualRect, BARGRAPH_VIRTUAL_PIXELS_WIDE, BARGRAPH_VIRTUAL_PIXELS_HIGH );
	ksBarGraph_Create( context, barGraph, renderPass, clipRect.x, clipRect.y, clipRect.width, clipRect.height, numBars, numStacked, backgroundColor );
}

static void ksTimeWarpBarGraphs_Create( ksGpuContext * context, ksTimeWarpBarGraphs * bargraphs, ksGpuRenderPass * renderPass )
{
	bargraphs->barGraphState = BAR_GRAPH_VISIBLE;

	ksBarGraph_CreateVirtualRect( context, &bargraphs->applicationFrameRateGraph, renderPass, &eyeTextureFrameRateBarGraphRect, 64, 1, &colorDarkGrey );
	ksBarGraph_CreateVirtualRect( context, &bargraphs->timeWarpFrameRateGraph, renderPass, &timeWarpFrameRateBarGraphRect, 64, 1, &colorDarkGrey );
	ksBarGraph_CreateVirtualRect( context, &bargraphs->frameCpuTimeBarGraph, renderPass, &frameCpuTimeBarGraphRect, 64, PROFILE_TIME_MAX, &colorDarkGrey );
	ksBarGraph_CreateVirtualRect( context, &bargraphs->frameGpuTimeBarGraph, renderPass, &frameGpuTimeBarGraphRect, 64, PROFILE_TIME_MAX, &colorDarkGrey );

	ksBarGraph_CreateVirtualRect( context, &bargraphs->multiViewBarGraph, renderPass, &multiViewBarGraphRect, 1, 1, &colorDarkGrey );
	ksBarGraph_CreateVirtualRect( context, &bargraphs->correctChromaticAberrationBarGraph, renderPass, &correctChromaticAberrationBarGraphRect, 1, 1, &colorDarkGrey );
	ksBarGraph_CreateVirtualRect( context, &bargraphs->timeWarpImplementationBarGraph, renderPass, &timeWarpImplementationBarGraphRect, 1, 1, &colorDarkGrey );

	ksBarGraph_CreateVirtualRect( context, &bargraphs->displayResolutionLevelBarGraph, renderPass, &displayResolutionLevelBarGraphRect, 1, 4, &colorDarkGrey );
	ksBarGraph_CreateVirtualRect( context, &bargraphs->eyeImageResolutionLevelBarGraph, renderPass, &eyeImageResolutionLevelBarGraphRect, 1, 4, &colorDarkGrey );
	ksBarGraph_CreateVirtualRect( context, &bargraphs->eyeImageSamplesLevelBarGraph, renderPass, &eyeImageSamplesLevelBarGraphRect, 1, 4, &colorDarkGrey );

	ksBarGraph_CreateVirtualRect( context, &bargraphs->sceneDrawCallLevelBarGraph, renderPass, &sceneDrawCallLevelBarGraphRect, 1, 4, &colorDarkGrey );
	ksBarGraph_CreateVirtualRect( context, &bargraphs->sceneTriangleLevelBarGraph, renderPass, &sceneTriangleLevelBarGraphRect, 1, 4, &colorDarkGrey );
	ksBarGraph_CreateVirtualRect( context, &bargraphs->sceneFragmentLevelBarGraph, renderPass, &sceneFragmentLevelBarGraphRect, 1, 4, &colorDarkGrey );

	ksBarGraph_AddBar( &bargraphs->displayResolutionLevelBarGraph, 0, 0.25f, &colorBlue, false );
	ksBarGraph_AddBar( &bargraphs->eyeImageResolutionLevelBarGraph, 0, 0.25f, &colorBlue, false );
	ksBarGraph_AddBar( &bargraphs->eyeImageSamplesLevelBarGraph, 0, 0.25f, &colorBlue, false );

	ksBarGraph_AddBar( &bargraphs->sceneDrawCallLevelBarGraph, 0, 0.25f, &colorBlue, false );
	ksBarGraph_AddBar( &bargraphs->sceneTriangleLevelBarGraph, 0, 0.25f, &colorBlue, false );
	ksBarGraph_AddBar( &bargraphs->sceneFragmentLevelBarGraph, 0, 0.25f, &colorBlue, false );

	ksGpuTimer_Create( context, &bargraphs->barGraphTimer );
}

static void ksTimeWarpBarGraphs_Destroy( ksGpuContext * context, ksTimeWarpBarGraphs * bargraphs )
{
	ksBarGraph_Destroy( context, &bargraphs->applicationFrameRateGraph );
	ksBarGraph_Destroy( context, &bargraphs->timeWarpFrameRateGraph );
	ksBarGraph_Destroy( context, &bargraphs->frameCpuTimeBarGraph );
	ksBarGraph_Destroy( context, &bargraphs->frameGpuTimeBarGraph );

	ksBarGraph_Destroy( context, &bargraphs->multiViewBarGraph );
	ksBarGraph_Destroy( context, &bargraphs->correctChromaticAberrationBarGraph );
	ksBarGraph_Destroy( context, &bargraphs->timeWarpImplementationBarGraph );

	ksBarGraph_Destroy( context, &bargraphs->displayResolutionLevelBarGraph );
	ksBarGraph_Destroy( context, &bargraphs->eyeImageResolutionLevelBarGraph );
	ksBarGraph_Destroy( context, &bargraphs->eyeImageSamplesLevelBarGraph );

	ksBarGraph_Destroy( context, &bargraphs->sceneDrawCallLevelBarGraph );
	ksBarGraph_Destroy( context, &bargraphs->sceneTriangleLevelBarGraph );
	ksBarGraph_Destroy( context, &bargraphs->sceneFragmentLevelBarGraph );

	ksGpuTimer_Destroy( context, &bargraphs->barGraphTimer );
}

static void ksTimeWarpBarGraphs_UpdateGraphics( ksGpuCommandBuffer * commandBuffer, ksTimeWarpBarGraphs * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->applicationFrameRateGraph );
		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->timeWarpFrameRateGraph );
		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->frameCpuTimeBarGraph );
		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->frameGpuTimeBarGraph );

		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->multiViewBarGraph );
		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->correctChromaticAberrationBarGraph );
		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->timeWarpImplementationBarGraph );

		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->displayResolutionLevelBarGraph );
		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->eyeImageResolutionLevelBarGraph );
		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->eyeImageSamplesLevelBarGraph );

		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->sceneDrawCallLevelBarGraph );
		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->sceneTriangleLevelBarGraph );
		ksBarGraph_UpdateGraphics( commandBuffer, &bargraphs->sceneFragmentLevelBarGraph );
	}
}

static void ksTimeWarpBarGraphs_RenderGraphics( ksGpuCommandBuffer * commandBuffer, ksTimeWarpBarGraphs * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		ksGpuCommandBuffer_BeginTimer( commandBuffer, &bargraphs->barGraphTimer );

		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->applicationFrameRateGraph );
		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->timeWarpFrameRateGraph );
		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->frameCpuTimeBarGraph );
		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->frameGpuTimeBarGraph );

		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->multiViewBarGraph );
		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->correctChromaticAberrationBarGraph );
		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->timeWarpImplementationBarGraph );

		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->displayResolutionLevelBarGraph );
		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->eyeImageResolutionLevelBarGraph );
		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->eyeImageSamplesLevelBarGraph );

		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->sceneDrawCallLevelBarGraph );
		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->sceneTriangleLevelBarGraph );
		ksBarGraph_RenderGraphics( commandBuffer, &bargraphs->sceneFragmentLevelBarGraph );

		ksGpuCommandBuffer_EndTimer( commandBuffer, &bargraphs->barGraphTimer );
	}
}

static void ksTimeWarpBarGraphs_UpdateCompute( ksGpuCommandBuffer * commandBuffer, ksTimeWarpBarGraphs * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->applicationFrameRateGraph );
		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->timeWarpFrameRateGraph );
		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->frameCpuTimeBarGraph );
		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->frameGpuTimeBarGraph );

		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->multiViewBarGraph );
		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->correctChromaticAberrationBarGraph );
		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->timeWarpImplementationBarGraph );

		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->displayResolutionLevelBarGraph );
		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->eyeImageResolutionLevelBarGraph );
		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->eyeImageSamplesLevelBarGraph );

		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->sceneDrawCallLevelBarGraph );
		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->sceneTriangleLevelBarGraph );
		ksBarGraph_UpdateCompute( commandBuffer, &bargraphs->sceneFragmentLevelBarGraph );
	}
}

static void ksTimeWarpBarGraphs_RenderCompute( ksGpuCommandBuffer * commandBuffer, ksTimeWarpBarGraphs * bargraphs, ksGpuFramebuffer * framebuffer )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		ksGpuCommandBuffer_BeginTimer( commandBuffer, &bargraphs->barGraphTimer );

		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->applicationFrameRateGraph, framebuffer );
		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->timeWarpFrameRateGraph, framebuffer );
		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->frameCpuTimeBarGraph, framebuffer );
		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->frameGpuTimeBarGraph, framebuffer );

		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->multiViewBarGraph, framebuffer );
		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->correctChromaticAberrationBarGraph, framebuffer );
		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->timeWarpImplementationBarGraph, framebuffer );

		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->displayResolutionLevelBarGraph, framebuffer );
		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->eyeImageResolutionLevelBarGraph, framebuffer );
		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->eyeImageSamplesLevelBarGraph, framebuffer );

		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->sceneDrawCallLevelBarGraph, framebuffer );
		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->sceneTriangleLevelBarGraph, framebuffer );
		ksBarGraph_RenderCompute( commandBuffer, &bargraphs->sceneFragmentLevelBarGraph, framebuffer );

		ksGpuCommandBuffer_EndTimer( commandBuffer, &bargraphs->barGraphTimer );
	}
}

static ksNanoseconds ksTimeWarpBarGraphs_GetGpuNanosecondsGraphics( ksTimeWarpBarGraphs * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		return ksGpuTimer_GetNanoseconds( &bargraphs->barGraphTimer );
	}
	return 0;
}

static ksNanoseconds ksTimeWarpBarGraphs_GetGpuNanosecondsCompute( ksTimeWarpBarGraphs * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		return ksGpuTimer_GetNanoseconds( &bargraphs->barGraphTimer );
	}
	return 0;
}

/*
================================================================================================================================

HMD

ksHmdInfo
ksBodyInfo

================================================================================================================================
*/

#define NUM_EYES				2
#define NUM_COLOR_CHANNELS		3

typedef struct
{
	int		displayPixelsWide;
	int		displayPixelsHigh;
	int		tilePixelsWide;
	int		tilePixelsHigh;
	int		eyeTilesWide;
	int		eyeTilesHigh;
	int		visiblePixelsWide;
	int		visiblePixelsHigh;
	float	visibleMetersWide;
	float	visibleMetersHigh;
	float	lensSeparationInMeters;
	float	metersPerTanAngleAtCenter;
	int		numKnots;
	float	K[11];
	float	chromaticAberration[4];
} ksHmdInfo;

typedef struct
{
	float	interpupillaryDistance;
} ksBodyInfo;

static const ksHmdInfo * GetDefaultHmdInfo( const int displayPixelsWide, const int displayPixelsHigh )
{
	static ksHmdInfo hmdInfo;
	hmdInfo.displayPixelsWide = displayPixelsWide;
	hmdInfo.displayPixelsHigh = displayPixelsHigh;
	hmdInfo.tilePixelsWide = 32;
	hmdInfo.tilePixelsHigh = 32;
	hmdInfo.eyeTilesWide = displayPixelsWide / hmdInfo.tilePixelsWide / NUM_EYES;
	hmdInfo.eyeTilesHigh = displayPixelsHigh / hmdInfo.tilePixelsHigh;
	hmdInfo.visiblePixelsWide = hmdInfo.eyeTilesWide * hmdInfo.tilePixelsWide * NUM_EYES;
	hmdInfo.visiblePixelsHigh = hmdInfo.eyeTilesHigh * hmdInfo.tilePixelsHigh;
	hmdInfo.visibleMetersWide = 0.11047f * ( hmdInfo.eyeTilesWide * hmdInfo.tilePixelsWide * NUM_EYES ) / displayPixelsWide;
	hmdInfo.visibleMetersHigh = 0.06214f * ( hmdInfo.eyeTilesHigh * hmdInfo.tilePixelsHigh ) / displayPixelsHigh;
	hmdInfo.lensSeparationInMeters = hmdInfo.visibleMetersWide / NUM_EYES;
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

static const ksBodyInfo * GetDefaultBodyInfo()
{
	static ksBodyInfo bodyInfo;
	bodyInfo.interpupillaryDistance	= 0.0640f;	// average interpupillary distance
	return &bodyInfo;
}

static bool hmd_headRotationDisabled = false;

static void GetHmdViewMatrixForTime( ksMatrix4x4f * viewMatrix, const ksNanoseconds time )
{
	if ( hmd_headRotationDisabled )
	{
		ksMatrix4x4f_CreateIdentity( viewMatrix );
		return;
	}

	// FIXME: use double?
	const float offset = time * ( MATH_PI / 1000.0f / 1000.0f / 1000.0f );
	const float degrees = 10.0f;
	const float degreesX = sinf( offset ) * degrees;
	const float degreesY = cosf( offset ) * degrees;

	ksMatrix4x4f_CreateRotation( viewMatrix, degreesX, degreesY, 0.0f );
}

static void CalculateTimeWarpTransform( ksMatrix4x4f * transform, const ksMatrix4x4f * renderProjectionMatrix,
										const ksMatrix4x4f * renderViewMatrix, const ksMatrix4x4f * newViewMatrix )
{
	// Convert the projection matrix from [-1, 1] space to [0, 1] space.
	const ksMatrix4x4f texCoordProjection =
	{ {
		{ 0.5f * renderProjectionMatrix->m[0][0],        0.0f,                                           0.0f,  0.0f },
		{ 0.0f,                                          0.5f * renderProjectionMatrix->m[1][1],         0.0f,  0.0f },
		{ 0.5f * renderProjectionMatrix->m[2][0] - 0.5f, 0.5f * renderProjectionMatrix->m[2][1] - 0.5f, -1.0f,  0.0f },
		{ 0.0f,                                          0.0f,                                           0.0f,  1.0f }
	} };

	// Calculate the delta between the view matrix used for rendering and
	// a more recent or predicted view matrix based on new sensor input.
	ksMatrix4x4f inverseRenderViewMatrix;
	ksMatrix4x4f_InvertHomogeneous( &inverseRenderViewMatrix, renderViewMatrix );

	ksMatrix4x4f deltaViewMatrix;
	ksMatrix4x4f_Multiply( &deltaViewMatrix, &inverseRenderViewMatrix, newViewMatrix );

	ksMatrix4x4f inverseDeltaViewMatrix;
	ksMatrix4x4f_InvertHomogeneous( &inverseDeltaViewMatrix, &deltaViewMatrix );

	// Make the delta rotation only.
	inverseDeltaViewMatrix.m[3][0] = 0.0f;
	inverseDeltaViewMatrix.m[3][1] = 0.0f;
	inverseDeltaViewMatrix.m[3][2] = 0.0f;

	// Accumulate the transforms.
	ksMatrix4x4f_Multiply( transform, &texCoordProjection, &inverseDeltaViewMatrix );
}

/*
================================================================================================================================

Distortion meshes.

ksMeshCoord

================================================================================================================================
*/

typedef struct
{
	float x;
	float y;
} ksMeshCoord;

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

static void BuildDistortionMeshes( ksMeshCoord * meshCoords[NUM_EYES][NUM_COLOR_CHANNELS], const ksHmdInfo * hmdInfo )
{
	const float horizontalShiftMeters = ( hmdInfo->lensSeparationInMeters / 2 ) - ( hmdInfo->visibleMetersWide / 4 );
	const float horizontalShiftView = horizontalShiftMeters / ( hmdInfo->visibleMetersWide / 2 );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		for ( int y = 0; y <= hmdInfo->eyeTilesHigh; y++ )
		{
			const float yf = 1.0f - (float)y / (float)hmdInfo->eyeTilesHigh;

			for ( int x = 0; x <= hmdInfo->eyeTilesWide; x++ )
			{
				const float xf = (float)x / (float)hmdInfo->eyeTilesWide;

				const float in[2] = { ( eye ? -horizontalShiftView : horizontalShiftView ) + xf, yf };
				const float ndcToPixels[2] = { hmdInfo->visiblePixelsWide * 0.25f, hmdInfo->visiblePixelsHigh * 0.5f };
				const float pixelsToMeters[2] = { hmdInfo->visibleMetersWide / hmdInfo->visiblePixelsWide, hmdInfo->visibleMetersHigh / hmdInfo->visiblePixelsHigh };

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

				const int vertNum = y * ( hmdInfo->eyeTilesWide + 1 ) + x;
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

ksTimeWarpGraphics

static void ksTimeWarpGraphics_Create( ksGpuContext * context, ksTimeWarpGraphics * graphics,
									const ksHmdInfo * hmdInfo, ksGpuRenderPass * renderPass );
static void ksTimeWarpGraphics_Destroy( ksGpuContext * context, ksTimeWarpGraphics * graphics );
static void ksTimeWarpGraphics_Render( ksGpuCommandBuffer * commandBuffer, ksTimeWarpGraphics * graphics,
									ksGpuFramebuffer * framebuffer, ksGpuRenderPass * renderPass,
									const ksNanoseconds refreshStartTime, const ksNanoseconds refreshEndTime,
									const ksMatrix4x4f * projectionMatrix, const ksMatrix4x4f * viewMatrix,
									ksGpuTexture * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, ksTimeWarpBarGraphs * bargraphs,
									ksNanoseconds cpuTimes[PROFILE_TIME_MAX], ksNanoseconds gpuTimes[PROFILE_TIME_MAX] );

================================================================================================================================
*/

typedef struct
{
	ksHmdInfo				hmdInfo;
	ksGpuGeometry			distortionMesh[NUM_EYES];
	ksGpuGraphicsProgram	timeWarpSpatialProgram;
	ksGpuGraphicsProgram	timeWarpChromaticProgram;
	ksGpuGraphicsPipeline	timeWarpSpatialPipeline[NUM_EYES];
	ksGpuGraphicsPipeline	timeWarpChromaticPipeline[NUM_EYES];
	ksGpuTimer				timeWarpGpuTime;
} ksTimeWarpGraphics;

enum
{
	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,
	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,
	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER,
	GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE
};

static const ksGpuProgramParm timeWarpSpatialGraphicsProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,		KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,	"TimeWarpStartTransform",	0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,		KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,	"TimeWarpEndTransform",		48 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER,		"ArrayLayer",				96 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT,	KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE,			"Texture",					0 }
};

static const char timeWarpSpatialVertexProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( std430, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =  0 ) highp mat3x4 TimeWarpStartTransform;\n"
	"	layout( offset = 48 ) highp mat3x4 TimeWarpEndTransform;\n"
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
	"	vec3 startUv1 = vec4( vertexUv1, -1, 1 ) * pc.TimeWarpStartTransform;\n"
	"	vec3 endUv1 = vec4( vertexUv1, -1, 1 ) * pc.TimeWarpEndTransform;\n"
	"	vec3 curUv1 = mix( startUv1, endUv1, displayFraction );\n"
	"	fragmentUv1 = curUv1.xy * ( 1.0 / max( curUv1.z, 0.00001 ) );\n"
	"}\n";

static const unsigned int timeWarpSpatialVertexProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x0000004e,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0009000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000a,0x0000000f,0x00000026,
	0x00000044,0x00030003,0x00000001,0x00000136,0x00070004,0x415f4c47,0x655f4252,0x6e61686e,
	0x5f646563,0x6f79616c,0x00737475,0x00070004,0x455f4c47,0x735f5458,0x65646168,0x6f695f72,
	0x6f6c625f,0x00736b63,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00060005,0x00000008,
	0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000008,0x00000000,0x505f6c67,
	0x7469736f,0x006e6f69,0x00030005,0x0000000a,0x00000000,0x00060005,0x0000000f,0x74726576,
	0x6f507865,0x69746973,0x00006e6f,0x00060005,0x00000019,0x70736964,0x4679616c,0x74636172,
	0x006e6f69,0x00050005,0x00000023,0x72617473,0x31765574,0x00000000,0x00050005,0x00000026,
	0x74726576,0x76557865,0x00000031,0x00060005,0x0000002d,0x68737550,0x736e6f43,0x746e6174,
	0x00000073,0x00090006,0x0000002d,0x00000000,0x656d6954,0x70726157,0x72617453,0x61725474,
	0x6f66736e,0x00006d72,0x00090006,0x0000002d,0x00000001,0x656d6954,0x70726157,0x54646e45,
	0x736e6172,0x6d726f66,0x00000000,0x00030005,0x0000002f,0x00006370,0x00040005,0x00000034,
	0x55646e65,0x00003176,0x00040005,0x0000003d,0x55727563,0x00003176,0x00050005,0x00000044,
	0x67617266,0x746e656d,0x00317655,0x00050048,0x00000008,0x00000000,0x0000000b,0x00000000,
	0x00030047,0x00000008,0x00000002,0x00040047,0x0000000f,0x0000001e,0x00000000,0x00040047,
	0x00000026,0x0000001e,0x00000001,0x00040048,0x0000002d,0x00000000,0x00000005,0x00050048,
	0x0000002d,0x00000000,0x00000023,0x00000000,0x00050048,0x0000002d,0x00000000,0x00000007,
	0x00000010,0x00040048,0x0000002d,0x00000001,0x00000005,0x00050048,0x0000002d,0x00000001,
	0x00000023,0x00000030,0x00050048,0x0000002d,0x00000001,0x00000007,0x00000010,0x00030047,
	0x0000002d,0x00000002,0x00030047,0x00000044,0x00000000,0x00040047,0x00000044,0x0000001e,
	0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
	0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x0003001e,0x00000008,0x00000007,
	0x00040020,0x00000009,0x00000003,0x00000008,0x0004003b,0x00000009,0x0000000a,0x00000003,
	0x00040015,0x0000000b,0x00000020,0x00000001,0x0004002b,0x0000000b,0x0000000c,0x00000000,
	0x00040017,0x0000000d,0x00000006,0x00000003,0x00040020,0x0000000e,0x00000001,0x0000000d,
	0x0004003b,0x0000000e,0x0000000f,0x00000001,0x0004002b,0x00000006,0x00000011,0x3f800000,
	0x00040020,0x00000016,0x00000003,0x00000007,0x00040020,0x00000018,0x00000007,0x00000006,
	0x00040015,0x0000001a,0x00000020,0x00000000,0x0004002b,0x0000001a,0x0000001b,0x00000000,
	0x00040020,0x0000001c,0x00000001,0x00000006,0x0004002b,0x00000006,0x0000001f,0x3f000000,
	0x00040020,0x00000022,0x00000007,0x0000000d,0x00040017,0x00000024,0x00000006,0x00000002,
	0x00040020,0x00000025,0x00000001,0x00000024,0x0004003b,0x00000025,0x00000026,0x00000001,
	0x0004002b,0x00000006,0x00000028,0xbf800000,0x00040018,0x0000002c,0x00000007,0x00000003,
	0x0004001e,0x0000002d,0x0000002c,0x0000002c,0x00040020,0x0000002e,0x00000009,0x0000002d,
	0x0004003b,0x0000002e,0x0000002f,0x00000009,0x00040020,0x00000030,0x00000009,0x0000002c,
	0x0004002b,0x0000000b,0x00000039,0x00000001,0x00040020,0x00000043,0x00000003,0x00000024,
	0x0004003b,0x00000043,0x00000044,0x00000003,0x0004002b,0x0000001a,0x00000047,0x00000002,
	0x0004002b,0x00000006,0x0000004a,0x3727c5ac,0x00050036,0x00000002,0x00000004,0x00000000,
	0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000018,0x00000019,0x00000007,0x0004003b,
	0x00000022,0x00000023,0x00000007,0x0004003b,0x00000022,0x00000034,0x00000007,0x0004003b,
	0x00000022,0x0000003d,0x00000007,0x0004003d,0x0000000d,0x00000010,0x0000000f,0x00050051,
	0x00000006,0x00000012,0x00000010,0x00000000,0x00050051,0x00000006,0x00000013,0x00000010,
	0x00000001,0x00050051,0x00000006,0x00000014,0x00000010,0x00000002,0x00070050,0x00000007,
	0x00000015,0x00000012,0x00000013,0x00000014,0x00000011,0x00050041,0x00000016,0x00000017,
	0x0000000a,0x0000000c,0x0003003e,0x00000017,0x00000015,0x00050041,0x0000001c,0x0000001d,
	0x0000000f,0x0000001b,0x0004003d,0x00000006,0x0000001e,0x0000001d,0x00050085,0x00000006,
	0x00000020,0x0000001e,0x0000001f,0x00050081,0x00000006,0x00000021,0x00000020,0x0000001f,
	0x0003003e,0x00000019,0x00000021,0x0004003d,0x00000024,0x00000027,0x00000026,0x00050051,
	0x00000006,0x00000029,0x00000027,0x00000000,0x00050051,0x00000006,0x0000002a,0x00000027,
	0x00000001,0x00070050,0x00000007,0x0000002b,0x00000029,0x0000002a,0x00000028,0x00000011,
	0x00050041,0x00000030,0x00000031,0x0000002f,0x0000000c,0x0004003d,0x0000002c,0x00000032,
	0x00000031,0x00050090,0x0000000d,0x00000033,0x0000002b,0x00000032,0x0003003e,0x00000023,
	0x00000033,0x0004003d,0x00000024,0x00000035,0x00000026,0x00050051,0x00000006,0x00000036,
	0x00000035,0x00000000,0x00050051,0x00000006,0x00000037,0x00000035,0x00000001,0x00070050,
	0x00000007,0x00000038,0x00000036,0x00000037,0x00000028,0x00000011,0x00050041,0x00000030,
	0x0000003a,0x0000002f,0x00000039,0x0004003d,0x0000002c,0x0000003b,0x0000003a,0x00050090,
	0x0000000d,0x0000003c,0x00000038,0x0000003b,0x0003003e,0x00000034,0x0000003c,0x0004003d,
	0x0000000d,0x0000003e,0x00000023,0x0004003d,0x0000000d,0x0000003f,0x00000034,0x0004003d,
	0x00000006,0x00000040,0x00000019,0x00060050,0x0000000d,0x00000041,0x00000040,0x00000040,
	0x00000040,0x0008000c,0x0000000d,0x00000042,0x00000001,0x0000002e,0x0000003e,0x0000003f,
	0x00000041,0x0003003e,0x0000003d,0x00000042,0x0004003d,0x0000000d,0x00000045,0x0000003d,
	0x0007004f,0x00000024,0x00000046,0x00000045,0x00000045,0x00000000,0x00000001,0x00050041,
	0x00000018,0x00000048,0x0000003d,0x00000047,0x0004003d,0x00000006,0x00000049,0x00000048,
	0x0007000c,0x00000006,0x0000004b,0x00000001,0x00000028,0x00000049,0x0000004a,0x00050088,
	0x00000006,0x0000004c,0x00000011,0x0000004b,0x0005008e,0x00000024,0x0000004d,0x00000046,
	0x0000004c,0x0003003e,0x00000044,0x0000004d,0x000100fd,0x00010038
};

static const char timeWarpSpatialFragmentProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( std430, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset = 96 ) int ArrayLayer;\n"
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
	0x00000004,0x00000007,0x00030003,0x00000001,0x00000136,0x00070004,0x415f4c47,0x655f4252,
	0x6e61686e,0x5f646563,0x6f79616c,0x00737475,0x00070004,0x455f4c47,0x735f5458,0x65646168,
	0x6f695f72,0x6f6c625f,0x00736b63,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00050005,
	0x00000009,0x4374756f,0x726f6c6f,0x00000000,0x00040005,0x0000000d,0x74786554,0x00657275,
	0x00050005,0x00000011,0x67617266,0x746e656d,0x00317655,0x00060005,0x00000014,0x68737550,
	0x736e6f43,0x746e6174,0x00000073,0x00060006,0x00000014,0x00000000,0x61727241,0x79614c79,
	0x00007265,0x00030005,0x00000016,0x00006370,0x00030047,0x00000009,0x00000000,0x00040047,
	0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x00000022,0x00000000,0x00040047,
	0x0000000d,0x00000021,0x00000000,0x00030047,0x00000011,0x00000000,0x00040047,0x00000011,
	0x0000001e,0x00000000,0x00030047,0x00000012,0x00000000,0x00040048,0x00000014,0x00000000,
	0x00000000,0x00050048,0x00000014,0x00000000,0x00000023,0x00000060,0x00030047,0x00000014,
	0x00000002,0x00030047,0x0000001a,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,
	0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,
	0x00040020,0x00000008,0x00000003,0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,
	0x00090019,0x0000000a,0x00000006,0x00000001,0x00000000,0x00000001,0x00000000,0x00000001,
	0x00000000,0x0003001b,0x0000000b,0x0000000a,0x00040020,0x0000000c,0x00000000,0x0000000b,
	0x0004003b,0x0000000c,0x0000000d,0x00000000,0x00040017,0x0000000f,0x00000006,0x00000002,
	0x00040020,0x00000010,0x00000001,0x0000000f,0x0004003b,0x00000010,0x00000011,0x00000001,
	0x00040015,0x00000013,0x00000020,0x00000001,0x0003001e,0x00000014,0x00000013,0x00040020,
	0x00000015,0x00000009,0x00000014,0x0004003b,0x00000015,0x00000016,0x00000009,0x0004002b,
	0x00000013,0x00000017,0x00000000,0x00040020,0x00000018,0x00000009,0x00000013,0x00040017,
	0x0000001c,0x00000006,0x00000003,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,
	0x000200f8,0x00000005,0x0004003d,0x0000000b,0x0000000e,0x0000000d,0x0004003d,0x0000000f,
	0x00000012,0x00000011,0x00050041,0x00000018,0x00000019,0x00000016,0x00000017,0x0004003d,
	0x00000013,0x0000001a,0x00000019,0x0004006f,0x00000006,0x0000001b,0x0000001a,0x00050051,
	0x00000006,0x0000001d,0x00000012,0x00000000,0x00050051,0x00000006,0x0000001e,0x00000012,
	0x00000001,0x00060050,0x0000001c,0x0000001f,0x0000001d,0x0000001e,0x0000001b,0x00050057,
	0x00000007,0x00000020,0x0000000e,0x0000001f,0x0003003e,0x00000009,0x00000020,0x000100fd,
	0x00010038
};

static const ksGpuProgramParm timeWarpChromaticGraphicsProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,		KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,	"TimeWarpStartTransform",	0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,		KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,	"TimeWarpEndTransform",		48 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER,		"ArrayLayer",				96 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT,	KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE,			"Texture",					0 }
};

static const char timeWarpChromaticVertexProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( std430, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =  0 ) highp mat3x4 TimeWarpStartTransform;\n"
	"	layout( offset = 48 ) highp mat3x4 TimeWarpEndTransform;\n"
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
	"	vec3 startUv0 = vec4( vertexUv0, -1, 1 ) * pc.TimeWarpStartTransform;\n"
	"	vec3 startUv1 = vec4( vertexUv1, -1, 1 ) * pc.TimeWarpStartTransform;\n"
	"	vec3 startUv2 = vec4( vertexUv2, -1, 1 ) * pc.TimeWarpStartTransform;\n"
	"\n"
	"	vec3 endUv0 = vec4( vertexUv0, -1, 1 ) * pc.TimeWarpEndTransform;\n"
	"	vec3 endUv1 = vec4( vertexUv1, -1, 1 ) * pc.TimeWarpEndTransform;\n"
	"	vec3 endUv2 = vec4( vertexUv2, -1, 1 ) * pc.TimeWarpEndTransform;\n"
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
	0x07230203,0x00010000,0x00080001,0x0000008c,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000d000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000a,0x0000000f,0x00000026,
	0x00000035,0x0000003e,0x00000072,0x0000007c,0x00000084,0x00030003,0x00000001,0x00000136,
	0x00070004,0x415f4c47,0x655f4252,0x6e61686e,0x5f646563,0x6f79616c,0x00737475,0x00070004,
	0x455f4c47,0x735f5458,0x65646168,0x6f695f72,0x6f6c625f,0x00736b63,0x00040005,0x00000004,
	0x6e69616d,0x00000000,0x00060005,0x00000008,0x505f6c67,0x65567265,0x78657472,0x00000000,
	0x00060006,0x00000008,0x00000000,0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000000a,
	0x00000000,0x00060005,0x0000000f,0x74726576,0x6f507865,0x69746973,0x00006e6f,0x00060005,
	0x00000019,0x70736964,0x4679616c,0x74636172,0x006e6f69,0x00050005,0x00000023,0x72617473,
	0x30765574,0x00000000,0x00050005,0x00000026,0x74726576,0x76557865,0x00000030,0x00060005,
	0x0000002d,0x68737550,0x736e6f43,0x746e6174,0x00000073,0x00090006,0x0000002d,0x00000000,
	0x656d6954,0x70726157,0x72617453,0x61725474,0x6f66736e,0x00006d72,0x00090006,0x0000002d,
	0x00000001,0x656d6954,0x70726157,0x54646e45,0x736e6172,0x6d726f66,0x00000000,0x00030005,
	0x0000002f,0x00006370,0x00050005,0x00000034,0x72617473,0x31765574,0x00000000,0x00050005,
	0x00000035,0x74726576,0x76557865,0x00000031,0x00050005,0x0000003d,0x72617473,0x32765574,
	0x00000000,0x00050005,0x0000003e,0x74726576,0x76557865,0x00000032,0x00040005,0x00000046,
	0x55646e65,0x00003076,0x00040005,0x0000004f,0x55646e65,0x00003176,0x00040005,0x00000057,
	0x55646e65,0x00003276,0x00040005,0x0000005f,0x55727563,0x00003076,0x00040005,0x00000065,
	0x55727563,0x00003176,0x00040005,0x0000006b,0x55727563,0x00003276,0x00050005,0x00000072,
	0x67617266,0x746e656d,0x00307655,0x00050005,0x0000007c,0x67617266,0x746e656d,0x00317655,
	0x00050005,0x00000084,0x67617266,0x746e656d,0x00327655,0x00050048,0x00000008,0x00000000,
	0x0000000b,0x00000000,0x00030047,0x00000008,0x00000002,0x00040047,0x0000000f,0x0000001e,
	0x00000000,0x00040047,0x00000026,0x0000001e,0x00000001,0x00040048,0x0000002d,0x00000000,
	0x00000005,0x00050048,0x0000002d,0x00000000,0x00000023,0x00000000,0x00050048,0x0000002d,
	0x00000000,0x00000007,0x00000010,0x00040048,0x0000002d,0x00000001,0x00000005,0x00050048,
	0x0000002d,0x00000001,0x00000023,0x00000030,0x00050048,0x0000002d,0x00000001,0x00000007,
	0x00000010,0x00030047,0x0000002d,0x00000002,0x00040047,0x00000035,0x0000001e,0x00000002,
	0x00040047,0x0000003e,0x0000001e,0x00000003,0x00030047,0x00000072,0x00000000,0x00040047,
	0x00000072,0x0000001e,0x00000000,0x00030047,0x0000007c,0x00000000,0x00040047,0x0000007c,
	0x0000001e,0x00000001,0x00030047,0x00000084,0x00000000,0x00040047,0x00000084,0x0000001e,
	0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
	0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x0003001e,0x00000008,0x00000007,
	0x00040020,0x00000009,0x00000003,0x00000008,0x0004003b,0x00000009,0x0000000a,0x00000003,
	0x00040015,0x0000000b,0x00000020,0x00000001,0x0004002b,0x0000000b,0x0000000c,0x00000000,
	0x00040017,0x0000000d,0x00000006,0x00000003,0x00040020,0x0000000e,0x00000001,0x0000000d,
	0x0004003b,0x0000000e,0x0000000f,0x00000001,0x0004002b,0x00000006,0x00000011,0x3f800000,
	0x00040020,0x00000016,0x00000003,0x00000007,0x00040020,0x00000018,0x00000007,0x00000006,
	0x00040015,0x0000001a,0x00000020,0x00000000,0x0004002b,0x0000001a,0x0000001b,0x00000000,
	0x00040020,0x0000001c,0x00000001,0x00000006,0x0004002b,0x00000006,0x0000001f,0x3f000000,
	0x00040020,0x00000022,0x00000007,0x0000000d,0x00040017,0x00000024,0x00000006,0x00000002,
	0x00040020,0x00000025,0x00000001,0x00000024,0x0004003b,0x00000025,0x00000026,0x00000001,
	0x0004002b,0x00000006,0x00000028,0xbf800000,0x00040018,0x0000002c,0x00000007,0x00000003,
	0x0004001e,0x0000002d,0x0000002c,0x0000002c,0x00040020,0x0000002e,0x00000009,0x0000002d,
	0x0004003b,0x0000002e,0x0000002f,0x00000009,0x00040020,0x00000030,0x00000009,0x0000002c,
	0x0004003b,0x00000025,0x00000035,0x00000001,0x0004003b,0x00000025,0x0000003e,0x00000001,
	0x0004002b,0x0000000b,0x0000004b,0x00000001,0x00040020,0x00000071,0x00000003,0x00000024,
	0x0004003b,0x00000071,0x00000072,0x00000003,0x0004002b,0x0000001a,0x00000075,0x00000002,
	0x0004002b,0x00000006,0x00000078,0x3727c5ac,0x0004003b,0x00000071,0x0000007c,0x00000003,
	0x0004003b,0x00000071,0x00000084,0x00000003,0x00050036,0x00000002,0x00000004,0x00000000,
	0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000018,0x00000019,0x00000007,0x0004003b,
	0x00000022,0x00000023,0x00000007,0x0004003b,0x00000022,0x00000034,0x00000007,0x0004003b,
	0x00000022,0x0000003d,0x00000007,0x0004003b,0x00000022,0x00000046,0x00000007,0x0004003b,
	0x00000022,0x0000004f,0x00000007,0x0004003b,0x00000022,0x00000057,0x00000007,0x0004003b,
	0x00000022,0x0000005f,0x00000007,0x0004003b,0x00000022,0x00000065,0x00000007,0x0004003b,
	0x00000022,0x0000006b,0x00000007,0x0004003d,0x0000000d,0x00000010,0x0000000f,0x00050051,
	0x00000006,0x00000012,0x00000010,0x00000000,0x00050051,0x00000006,0x00000013,0x00000010,
	0x00000001,0x00050051,0x00000006,0x00000014,0x00000010,0x00000002,0x00070050,0x00000007,
	0x00000015,0x00000012,0x00000013,0x00000014,0x00000011,0x00050041,0x00000016,0x00000017,
	0x0000000a,0x0000000c,0x0003003e,0x00000017,0x00000015,0x00050041,0x0000001c,0x0000001d,
	0x0000000f,0x0000001b,0x0004003d,0x00000006,0x0000001e,0x0000001d,0x00050085,0x00000006,
	0x00000020,0x0000001e,0x0000001f,0x00050081,0x00000006,0x00000021,0x00000020,0x0000001f,
	0x0003003e,0x00000019,0x00000021,0x0004003d,0x00000024,0x00000027,0x00000026,0x00050051,
	0x00000006,0x00000029,0x00000027,0x00000000,0x00050051,0x00000006,0x0000002a,0x00000027,
	0x00000001,0x00070050,0x00000007,0x0000002b,0x00000029,0x0000002a,0x00000028,0x00000011,
	0x00050041,0x00000030,0x00000031,0x0000002f,0x0000000c,0x0004003d,0x0000002c,0x00000032,
	0x00000031,0x00050090,0x0000000d,0x00000033,0x0000002b,0x00000032,0x0003003e,0x00000023,
	0x00000033,0x0004003d,0x00000024,0x00000036,0x00000035,0x00050051,0x00000006,0x00000037,
	0x00000036,0x00000000,0x00050051,0x00000006,0x00000038,0x00000036,0x00000001,0x00070050,
	0x00000007,0x00000039,0x00000037,0x00000038,0x00000028,0x00000011,0x00050041,0x00000030,
	0x0000003a,0x0000002f,0x0000000c,0x0004003d,0x0000002c,0x0000003b,0x0000003a,0x00050090,
	0x0000000d,0x0000003c,0x00000039,0x0000003b,0x0003003e,0x00000034,0x0000003c,0x0004003d,
	0x00000024,0x0000003f,0x0000003e,0x00050051,0x00000006,0x00000040,0x0000003f,0x00000000,
	0x00050051,0x00000006,0x00000041,0x0000003f,0x00000001,0x00070050,0x00000007,0x00000042,
	0x00000040,0x00000041,0x00000028,0x00000011,0x00050041,0x00000030,0x00000043,0x0000002f,
	0x0000000c,0x0004003d,0x0000002c,0x00000044,0x00000043,0x00050090,0x0000000d,0x00000045,
	0x00000042,0x00000044,0x0003003e,0x0000003d,0x00000045,0x0004003d,0x00000024,0x00000047,
	0x00000026,0x00050051,0x00000006,0x00000048,0x00000047,0x00000000,0x00050051,0x00000006,
	0x00000049,0x00000047,0x00000001,0x00070050,0x00000007,0x0000004a,0x00000048,0x00000049,
	0x00000028,0x00000011,0x00050041,0x00000030,0x0000004c,0x0000002f,0x0000004b,0x0004003d,
	0x0000002c,0x0000004d,0x0000004c,0x00050090,0x0000000d,0x0000004e,0x0000004a,0x0000004d,
	0x0003003e,0x00000046,0x0000004e,0x0004003d,0x00000024,0x00000050,0x00000035,0x00050051,
	0x00000006,0x00000051,0x00000050,0x00000000,0x00050051,0x00000006,0x00000052,0x00000050,
	0x00000001,0x00070050,0x00000007,0x00000053,0x00000051,0x00000052,0x00000028,0x00000011,
	0x00050041,0x00000030,0x00000054,0x0000002f,0x0000004b,0x0004003d,0x0000002c,0x00000055,
	0x00000054,0x00050090,0x0000000d,0x00000056,0x00000053,0x00000055,0x0003003e,0x0000004f,
	0x00000056,0x0004003d,0x00000024,0x00000058,0x0000003e,0x00050051,0x00000006,0x00000059,
	0x00000058,0x00000000,0x00050051,0x00000006,0x0000005a,0x00000058,0x00000001,0x00070050,
	0x00000007,0x0000005b,0x00000059,0x0000005a,0x00000028,0x00000011,0x00050041,0x00000030,
	0x0000005c,0x0000002f,0x0000004b,0x0004003d,0x0000002c,0x0000005d,0x0000005c,0x00050090,
	0x0000000d,0x0000005e,0x0000005b,0x0000005d,0x0003003e,0x00000057,0x0000005e,0x0004003d,
	0x0000000d,0x00000060,0x00000023,0x0004003d,0x0000000d,0x00000061,0x00000046,0x0004003d,
	0x00000006,0x00000062,0x00000019,0x00060050,0x0000000d,0x00000063,0x00000062,0x00000062,
	0x00000062,0x0008000c,0x0000000d,0x00000064,0x00000001,0x0000002e,0x00000060,0x00000061,
	0x00000063,0x0003003e,0x0000005f,0x00000064,0x0004003d,0x0000000d,0x00000066,0x00000034,
	0x0004003d,0x0000000d,0x00000067,0x0000004f,0x0004003d,0x00000006,0x00000068,0x00000019,
	0x00060050,0x0000000d,0x00000069,0x00000068,0x00000068,0x00000068,0x0008000c,0x0000000d,
	0x0000006a,0x00000001,0x0000002e,0x00000066,0x00000067,0x00000069,0x0003003e,0x00000065,
	0x0000006a,0x0004003d,0x0000000d,0x0000006c,0x0000003d,0x0004003d,0x0000000d,0x0000006d,
	0x00000057,0x0004003d,0x00000006,0x0000006e,0x00000019,0x00060050,0x0000000d,0x0000006f,
	0x0000006e,0x0000006e,0x0000006e,0x0008000c,0x0000000d,0x00000070,0x00000001,0x0000002e,
	0x0000006c,0x0000006d,0x0000006f,0x0003003e,0x0000006b,0x00000070,0x0004003d,0x0000000d,
	0x00000073,0x0000005f,0x0007004f,0x00000024,0x00000074,0x00000073,0x00000073,0x00000000,
	0x00000001,0x00050041,0x00000018,0x00000076,0x0000005f,0x00000075,0x0004003d,0x00000006,
	0x00000077,0x00000076,0x0007000c,0x00000006,0x00000079,0x00000001,0x00000028,0x00000077,
	0x00000078,0x00050088,0x00000006,0x0000007a,0x00000011,0x00000079,0x0005008e,0x00000024,
	0x0000007b,0x00000074,0x0000007a,0x0003003e,0x00000072,0x0000007b,0x0004003d,0x0000000d,
	0x0000007d,0x00000065,0x0007004f,0x00000024,0x0000007e,0x0000007d,0x0000007d,0x00000000,
	0x00000001,0x00050041,0x00000018,0x0000007f,0x00000065,0x00000075,0x0004003d,0x00000006,
	0x00000080,0x0000007f,0x0007000c,0x00000006,0x00000081,0x00000001,0x00000028,0x00000080,
	0x00000078,0x00050088,0x00000006,0x00000082,0x00000011,0x00000081,0x0005008e,0x00000024,
	0x00000083,0x0000007e,0x00000082,0x0003003e,0x0000007c,0x00000083,0x0004003d,0x0000000d,
	0x00000085,0x0000006b,0x0007004f,0x00000024,0x00000086,0x00000085,0x00000085,0x00000000,
	0x00000001,0x00050041,0x00000018,0x00000087,0x0000006b,0x00000075,0x0004003d,0x00000006,
	0x00000088,0x00000087,0x0007000c,0x00000006,0x00000089,0x00000001,0x00000028,0x00000088,
	0x00000078,0x00050088,0x00000006,0x0000008a,0x00000011,0x00000089,0x0005008e,0x00000024,
	0x0000008b,0x00000086,0x0000008a,0x0003003e,0x00000084,0x0000008b,0x000100fd,0x00010038
};

static const char timeWarpChromaticFragmentProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( std430, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset = 96 ) int ArrayLayer;\n"
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
	0x00000034,0x00030010,0x00000004,0x00000007,0x00030003,0x00000001,0x00000136,0x00070004,
	0x415f4c47,0x655f4252,0x6e61686e,0x5f646563,0x6f79616c,0x00737475,0x00070004,0x455f4c47,
	0x735f5458,0x65646168,0x6f695f72,0x6f6c625f,0x00736b63,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00050005,0x00000009,0x4374756f,0x726f6c6f,0x00000000,0x00040005,0x0000000d,
	0x74786554,0x00657275,0x00050005,0x00000011,0x67617266,0x746e656d,0x00307655,0x00060005,
	0x00000014,0x68737550,0x736e6f43,0x746e6174,0x00000073,0x00060006,0x00000014,0x00000000,
	0x61727241,0x79614c79,0x00007265,0x00030005,0x00000016,0x00006370,0x00050005,0x00000027,
	0x67617266,0x746e656d,0x00317655,0x00050005,0x00000034,0x67617266,0x746e656d,0x00327655,
	0x00030047,0x00000009,0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,
	0x0000000d,0x00000022,0x00000000,0x00040047,0x0000000d,0x00000021,0x00000000,0x00030047,
	0x00000011,0x00000000,0x00040047,0x00000011,0x0000001e,0x00000000,0x00030047,0x00000012,
	0x00000000,0x00040048,0x00000014,0x00000000,0x00000000,0x00050048,0x00000014,0x00000000,
	0x00000023,0x00000060,0x00030047,0x00000014,0x00000002,0x00030047,0x0000001a,0x00000000,
	0x00030047,0x00000027,0x00000000,0x00040047,0x00000027,0x0000001e,0x00000001,0x00030047,
	0x00000028,0x00000000,0x00030047,0x0000002a,0x00000000,0x00030047,0x00000034,0x00000000,
	0x00040047,0x00000034,0x0000001e,0x00000002,0x00030047,0x00000035,0x00000000,0x00030047,
	0x00000037,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,
	0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,
	0x00000003,0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00090019,0x0000000a,
	0x00000006,0x00000001,0x00000000,0x00000001,0x00000000,0x00000001,0x00000000,0x0003001b,
	0x0000000b,0x0000000a,0x00040020,0x0000000c,0x00000000,0x0000000b,0x0004003b,0x0000000c,
	0x0000000d,0x00000000,0x00040017,0x0000000f,0x00000006,0x00000002,0x00040020,0x00000010,
	0x00000001,0x0000000f,0x0004003b,0x00000010,0x00000011,0x00000001,0x00040015,0x00000013,
	0x00000020,0x00000001,0x0003001e,0x00000014,0x00000013,0x00040020,0x00000015,0x00000009,
	0x00000014,0x0004003b,0x00000015,0x00000016,0x00000009,0x0004002b,0x00000013,0x00000017,
	0x00000000,0x00040020,0x00000018,0x00000009,0x00000013,0x00040017,0x0000001c,0x00000006,
	0x00000003,0x00040015,0x00000021,0x00000020,0x00000000,0x0004002b,0x00000021,0x00000022,
	0x00000000,0x00040020,0x00000024,0x00000003,0x00000006,0x0004003b,0x00000010,0x00000027,
	0x00000001,0x0004002b,0x00000021,0x00000030,0x00000001,0x0004003b,0x00000010,0x00000034,
	0x00000001,0x0004002b,0x00000021,0x0000003d,0x00000002,0x0004002b,0x00000006,0x00000040,
	0x3f800000,0x0004002b,0x00000021,0x00000041,0x00000003,0x00050036,0x00000002,0x00000004,
	0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003d,0x0000000b,0x0000000e,0x0000000d,
	0x0004003d,0x0000000f,0x00000012,0x00000011,0x00050041,0x00000018,0x00000019,0x00000016,
	0x00000017,0x0004003d,0x00000013,0x0000001a,0x00000019,0x0004006f,0x00000006,0x0000001b,
	0x0000001a,0x00050051,0x00000006,0x0000001d,0x00000012,0x00000000,0x00050051,0x00000006,
	0x0000001e,0x00000012,0x00000001,0x00060050,0x0000001c,0x0000001f,0x0000001d,0x0000001e,
	0x0000001b,0x00050057,0x00000007,0x00000020,0x0000000e,0x0000001f,0x00050051,0x00000006,
	0x00000023,0x00000020,0x00000000,0x00050041,0x00000024,0x00000025,0x00000009,0x00000022,
	0x0003003e,0x00000025,0x00000023,0x0004003d,0x0000000b,0x00000026,0x0000000d,0x0004003d,
	0x0000000f,0x00000028,0x00000027,0x00050041,0x00000018,0x00000029,0x00000016,0x00000017,
	0x0004003d,0x00000013,0x0000002a,0x00000029,0x0004006f,0x00000006,0x0000002b,0x0000002a,
	0x00050051,0x00000006,0x0000002c,0x00000028,0x00000000,0x00050051,0x00000006,0x0000002d,
	0x00000028,0x00000001,0x00060050,0x0000001c,0x0000002e,0x0000002c,0x0000002d,0x0000002b,
	0x00050057,0x00000007,0x0000002f,0x00000026,0x0000002e,0x00050051,0x00000006,0x00000031,
	0x0000002f,0x00000001,0x00050041,0x00000024,0x00000032,0x00000009,0x00000030,0x0003003e,
	0x00000032,0x00000031,0x0004003d,0x0000000b,0x00000033,0x0000000d,0x0004003d,0x0000000f,
	0x00000035,0x00000034,0x00050041,0x00000018,0x00000036,0x00000016,0x00000017,0x0004003d,
	0x00000013,0x00000037,0x00000036,0x0004006f,0x00000006,0x00000038,0x00000037,0x00050051,
	0x00000006,0x00000039,0x00000035,0x00000000,0x00050051,0x00000006,0x0000003a,0x00000035,
	0x00000001,0x00060050,0x0000001c,0x0000003b,0x00000039,0x0000003a,0x00000038,0x00050057,
	0x00000007,0x0000003c,0x00000033,0x0000003b,0x00050051,0x00000006,0x0000003e,0x0000003c,
	0x00000002,0x00050041,0x00000024,0x0000003f,0x00000009,0x0000003d,0x0003003e,0x0000003f,
	0x0000003e,0x00050041,0x00000024,0x00000042,0x00000009,0x00000041,0x0003003e,0x00000042,
	0x00000040,0x000100fd,0x00010038
};

static void ksTimeWarpGraphics_Create( ksGpuContext * context, ksTimeWarpGraphics * graphics,
									const ksHmdInfo * hmdInfo, ksGpuRenderPass * renderPass )
{
	memset( graphics, 0, sizeof( ksTimeWarpGraphics ) );

	graphics->hmdInfo = *hmdInfo;

	const int vertexCount = ( hmdInfo->eyeTilesHigh + 1 ) * ( hmdInfo->eyeTilesWide + 1 );
	const int indexCount = hmdInfo->eyeTilesHigh * hmdInfo->eyeTilesWide * 6;

	ksGpuTriangleIndexArray indices;
	ksGpuTriangleIndexArray_Alloc( &indices, indexCount, NULL );

	for ( int y = 0; y < hmdInfo->eyeTilesHigh; y++ )
	{
		for ( int x = 0; x < hmdInfo->eyeTilesWide; x++ )
		{
			const int offset = ( y * hmdInfo->eyeTilesWide + x ) * 6;

			indices.indexArray[offset + 0] = (ksGpuTriangleIndex)( ( y + 0 ) * ( hmdInfo->eyeTilesWide + 1 ) + ( x + 0 ) );
			indices.indexArray[offset + 1] = (ksGpuTriangleIndex)( ( y + 1 ) * ( hmdInfo->eyeTilesWide + 1 ) + ( x + 0 ) );
			indices.indexArray[offset + 2] = (ksGpuTriangleIndex)( ( y + 0 ) * ( hmdInfo->eyeTilesWide + 1 ) + ( x + 1 ) );

			indices.indexArray[offset + 3] = (ksGpuTriangleIndex)( ( y + 0 ) * ( hmdInfo->eyeTilesWide + 1 ) + ( x + 1 ) );
			indices.indexArray[offset + 4] = (ksGpuTriangleIndex)( ( y + 1 ) * ( hmdInfo->eyeTilesWide + 1 ) + ( x + 0 ) );
			indices.indexArray[offset + 5] = (ksGpuTriangleIndex)( ( y + 1 ) * ( hmdInfo->eyeTilesWide + 1 ) + ( x + 1 ) );
		}
	}

	ksDefaultVertexAttributeArrays vertexAttribs;
	ksGpuVertexAttributeArrays_Alloc( &vertexAttribs.base,
									DefaultVertexAttributeLayout, vertexCount,
									VERTEX_ATTRIBUTE_FLAG_POSITION |
									VERTEX_ATTRIBUTE_FLAG_UV0 |
									VERTEX_ATTRIBUTE_FLAG_UV1 |
									VERTEX_ATTRIBUTE_FLAG_UV2 );

	const int numMeshCoords = ( hmdInfo->eyeTilesWide + 1 ) * ( hmdInfo->eyeTilesHigh + 1 );
	ksMeshCoord * meshCoordsBasePtr = (ksMeshCoord *) malloc( NUM_EYES * NUM_COLOR_CHANNELS * numMeshCoords * sizeof( ksMeshCoord ) );
	ksMeshCoord * meshCoords[NUM_EYES][NUM_COLOR_CHANNELS] =
	{
		{ meshCoordsBasePtr + 0 * numMeshCoords, meshCoordsBasePtr + 1 * numMeshCoords, meshCoordsBasePtr + 2 * numMeshCoords },
		{ meshCoordsBasePtr + 3 * numMeshCoords, meshCoordsBasePtr + 4 * numMeshCoords, meshCoordsBasePtr + 5 * numMeshCoords }
	};
	BuildDistortionMeshes( meshCoords, hmdInfo );

#if defined( GRAPHICS_API_VULKAN )
	const float flipY = -1.0f;
#else
	const float flipY = 1.0f;
#endif

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		for ( int y = 0; y <= hmdInfo->eyeTilesHigh; y++ )
		{
			for ( int x = 0; x <= hmdInfo->eyeTilesWide; x++ )
			{
				const int index = y * ( hmdInfo->eyeTilesWide + 1 ) + x;
				vertexAttribs.position[index].x = ( -1.0f + eye + ( (float)x / hmdInfo->eyeTilesWide ) );
				vertexAttribs.position[index].y = ( -1.0f + 2.0f * ( ( hmdInfo->eyeTilesHigh - (float)y ) / hmdInfo->eyeTilesHigh ) *
													( (float)( hmdInfo->eyeTilesHigh * hmdInfo->tilePixelsHigh ) / hmdInfo->displayPixelsHigh ) ) * flipY;
				vertexAttribs.position[index].z = 0.0f;
				vertexAttribs.uv0[index].x = meshCoords[eye][0][index].x;
				vertexAttribs.uv0[index].y = meshCoords[eye][0][index].y;
				vertexAttribs.uv1[index].x = meshCoords[eye][1][index].x;
				vertexAttribs.uv1[index].y = meshCoords[eye][1][index].y;
				vertexAttribs.uv2[index].x = meshCoords[eye][2][index].x;
				vertexAttribs.uv2[index].y = meshCoords[eye][2][index].y;
			}
		}

		ksGpuGeometry_Create( context, &graphics->distortionMesh[eye], &vertexAttribs.base, &indices );
	}

	free( meshCoordsBasePtr );
	ksGpuVertexAttributeArrays_Free( &vertexAttribs.base );
	ksGpuTriangleIndexArray_Free( &indices );

	ksGpuGraphicsProgram_Create( context, &graphics->timeWarpSpatialProgram,
								PROGRAM( timeWarpSpatialVertexProgram ), sizeof( PROGRAM( timeWarpSpatialVertexProgram ) ),
								PROGRAM( timeWarpSpatialFragmentProgram ), sizeof( PROGRAM( timeWarpSpatialFragmentProgram ) ),
								timeWarpSpatialGraphicsProgramParms, ARRAY_SIZE( timeWarpSpatialGraphicsProgramParms ),
								graphics->distortionMesh[0].layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_UV1 );
	ksGpuGraphicsProgram_Create( context, &graphics->timeWarpChromaticProgram,
								PROGRAM( timeWarpChromaticVertexProgram ), sizeof( PROGRAM( timeWarpChromaticVertexProgram ) ),
								PROGRAM( timeWarpChromaticFragmentProgram ), sizeof( PROGRAM( timeWarpChromaticFragmentProgram ) ),
								timeWarpChromaticGraphicsProgramParms, ARRAY_SIZE( timeWarpChromaticGraphicsProgramParms ),
								graphics->distortionMesh[0].layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_UV0 |
								VERTEX_ATTRIBUTE_FLAG_UV1 | VERTEX_ATTRIBUTE_FLAG_UV2 );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		ksGpuGraphicsPipelineParms pipelineParms;
		ksGpuGraphicsPipelineParms_Init( &pipelineParms );

		pipelineParms.rop.depthTestEnable = false;
		pipelineParms.rop.depthWriteEnable = false;
		pipelineParms.renderPass = renderPass;
		pipelineParms.program = &graphics->timeWarpSpatialProgram;
		pipelineParms.geometry = &graphics->distortionMesh[eye];

		ksGpuGraphicsPipeline_Create( context, &graphics->timeWarpSpatialPipeline[eye], &pipelineParms );

		pipelineParms.program = &graphics->timeWarpChromaticProgram;
		pipelineParms.geometry = &graphics->distortionMesh[eye];

		ksGpuGraphicsPipeline_Create( context, &graphics->timeWarpChromaticPipeline[eye], &pipelineParms );
	}

	ksGpuTimer_Create( context, &graphics->timeWarpGpuTime );
}

static void ksTimeWarpGraphics_Destroy( ksGpuContext * context, ksTimeWarpGraphics * graphics )
{
	ksGpuTimer_Destroy( context, &graphics->timeWarpGpuTime );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		ksGpuGraphicsPipeline_Destroy( context, &graphics->timeWarpSpatialPipeline[eye] );
		ksGpuGraphicsPipeline_Destroy( context, &graphics->timeWarpChromaticPipeline[eye] );
	}

	ksGpuGraphicsProgram_Destroy( context, &graphics->timeWarpSpatialProgram );
	ksGpuGraphicsProgram_Destroy( context, &graphics->timeWarpChromaticProgram );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		ksGpuGeometry_Destroy( context, &graphics->distortionMesh[eye] );
	}
}

static void ksTimeWarpGraphics_Render( ksGpuCommandBuffer * commandBuffer, ksTimeWarpGraphics * graphics,
									ksGpuFramebuffer * framebuffer, ksGpuRenderPass * renderPass,
									const ksNanoseconds refreshStartTime, const ksNanoseconds refreshEndTime,
									const ksMatrix4x4f * projectionMatrix, const ksMatrix4x4f * viewMatrix,
									ksGpuTexture * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, ksTimeWarpBarGraphs * bargraphs,
									ksNanoseconds cpuTimes[PROFILE_TIME_MAX], ksNanoseconds gpuTimes[PROFILE_TIME_MAX] )
{
	const ksNanoseconds t0 = GetTimeNanoseconds();

	ksMatrix4x4f displayRefreshStartViewMatrix;
	ksMatrix4x4f displayRefreshEndViewMatrix;
	GetHmdViewMatrixForTime( &displayRefreshStartViewMatrix, refreshStartTime );
	GetHmdViewMatrixForTime( &displayRefreshEndViewMatrix, refreshEndTime );

	ksMatrix4x4f timeWarpStartTransform;
	ksMatrix4x4f timeWarpEndTransform;
	CalculateTimeWarpTransform( &timeWarpStartTransform, projectionMatrix, viewMatrix, &displayRefreshStartViewMatrix );
	CalculateTimeWarpTransform( &timeWarpEndTransform, projectionMatrix, viewMatrix, &displayRefreshEndViewMatrix );

	ksMatrix3x4f timeWarpStartTransform3x4;
	ksMatrix3x4f timeWarpEndTransform3x4;
	ksMatrix3x4f_CreateFromMatrix4x4f( &timeWarpStartTransform3x4, &timeWarpStartTransform );
	ksMatrix3x4f_CreateFromMatrix4x4f( &timeWarpEndTransform3x4, &timeWarpEndTransform );

	const ksScreenRect screenRect = ksGpuFramebuffer_GetRect( framebuffer );

	ksGpuCommandBuffer_BeginPrimary( commandBuffer );
	ksGpuCommandBuffer_BeginFramebuffer( commandBuffer, framebuffer, 0, KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );

	ksTimeWarpBarGraphs_UpdateGraphics( commandBuffer, bargraphs );

	ksGpuCommandBuffer_BeginTimer( commandBuffer, &graphics->timeWarpGpuTime );
	ksGpuCommandBuffer_BeginRenderPass( commandBuffer, renderPass, framebuffer, &screenRect );

	ksGpuCommandBuffer_SetViewport( commandBuffer, &screenRect );
	ksGpuCommandBuffer_SetScissor( commandBuffer, &screenRect );

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		ksGpuGraphicsCommand command;
		ksGpuGraphicsCommand_Init( &command );
		ksGpuGraphicsCommand_SetPipeline( &command, correctChromaticAberration ? &graphics->timeWarpChromaticPipeline[eye] : &graphics->timeWarpSpatialPipeline[eye] );
		ksGpuGraphicsCommand_SetParmFloatMatrix3x4( &command, GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM, &timeWarpStartTransform3x4 );
		ksGpuGraphicsCommand_SetParmFloatMatrix3x4( &command, GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM, &timeWarpEndTransform3x4 );
		ksGpuGraphicsCommand_SetParmInt( &command, GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER, &eyeArrayLayer[eye] );
		ksGpuGraphicsCommand_SetParmTextureSampled( &command, GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE, eyeTexture[eye] );

		ksGpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
	}

	const ksNanoseconds t1 = GetTimeNanoseconds();

	ksTimeWarpBarGraphs_RenderGraphics( commandBuffer, bargraphs );

	ksGpuCommandBuffer_EndRenderPass( commandBuffer, renderPass );
	ksGpuCommandBuffer_EndTimer( commandBuffer, &graphics->timeWarpGpuTime );

	ksGpuCommandBuffer_EndFramebuffer( commandBuffer, framebuffer, 0, KS_GPU_TEXTURE_USAGE_PRESENTATION );
	ksGpuCommandBuffer_EndPrimary( commandBuffer );

	ksGpuCommandBuffer_SubmitPrimary( commandBuffer );

	const ksNanoseconds t2 = GetTimeNanoseconds();

	cpuTimes[PROFILE_TIME_TIME_WARP] = t1 - t0;
	cpuTimes[PROFILE_TIME_BAR_GRAPHS] = t2 - t1;
	cpuTimes[PROFILE_TIME_BLIT] = 0;

	const ksNanoseconds barGraphGpuTime = ksTimeWarpBarGraphs_GetGpuNanosecondsGraphics( bargraphs );

	gpuTimes[PROFILE_TIME_TIME_WARP] = ksGpuTimer_GetNanoseconds( &graphics->timeWarpGpuTime ) - barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BAR_GRAPHS] = barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BLIT] = 0;
}

/*
================================================================================================================================

Time warp compute rendering.

ksTimeWarpCompute

static void ksTimeWarpCompute_Create( ksGpuContext * context, ksTimeWarpCompute * compute, const ksHmdInfo * hmdInfo,
									ksGpuRenderPass * renderPass );
static void ksTimeWarpCompute_Destroy( ksGpuContext * context, ksTimeWarpCompute * compute );
static void ksTimeWarpCompute_Render( ksGpuCommandBuffer * commandBuffer, ksTimeWarpCompute * compute,
									ksGpuFramebuffer * framebuffer,
									const ksNanoseconds refreshStartTime, const ksNanoseconds refreshEndTime,
									const ksMatrix4x4f * projectionMatrix, const ksMatrix4x4f * viewMatrix,
									ksGpuTexture * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, ksTimeWarpBarGraphs * bargraphs,
									ksNanoseconds cpuTimes[PROFILE_TIME_MAX], ksNanoseconds gpuTimes[PROFILE_TIME_MAX] );

================================================================================================================================
*/

typedef struct
{
	ksHmdInfo				hmdInfo;
	ksGpuTexture			distortionImage[NUM_EYES][NUM_COLOR_CHANNELS];
	ksGpuTexture			timeWarpImage[NUM_EYES][NUM_COLOR_CHANNELS];
	ksGpuComputeProgram		timeWarpTransformProgram;
	ksGpuComputeProgram		timeWarpSpatialProgram;
	ksGpuComputeProgram		timeWarpChromaticProgram;
	ksGpuComputePipeline	timeWarpTransformPipeline;
	ksGpuComputePipeline	timeWarpSpatialPipeline;
	ksGpuComputePipeline	timeWarpChromaticPipeline;
	ksGpuTimer				timeWarpGpuTime;
} ksTimeWarpCompute;

enum
{
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_DST,
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_SRC,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_DIMENSIONS,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM
};

static const ksGpuProgramParm timeWarpTransformComputeProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,					KS_GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_DST,		"dst",						0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,					KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_SRC,		"src",						1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,		KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_DIMENSIONS,		"dimensions",				96 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE,				"eye",						104 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,	"timeWarpStartTransform",	0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,		"timeWarpEndTransform",		48 }
};

#define TRANSFORM_LOCAL_SIZE_X		8
#define TRANSFORM_LOCAL_SIZE_Y		8

static const char timeWarpTransformComputeProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"\n"
	"layout( local_size_x = " STRINGIFY( TRANSFORM_LOCAL_SIZE_X ) ", local_size_y = " STRINGIFY( TRANSFORM_LOCAL_SIZE_Y ) " ) in;\n"
	"\n"
	"layout( rgba16f, binding = 0 ) uniform writeonly image2D dst;\n"
	"layout( rgba32f, binding = 1 ) uniform readonly image2D src;\n"
	"layout( std430, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =   0 ) highp mat3x4 timeWarpStartTransform;\n"
	"	layout( offset =  48 ) highp mat3x4 timeWarpEndTransform;\n"
	"	layout( offset =  96 ) ivec2 dimensions;\n"
	"	layout( offset = 104 ) int eye;\n"
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
	"	vec3 start = vec4( coords, -1.0f, 1.0f ) * pc.timeWarpStartTransform;\n"
	"	vec3 end = vec4( coords, -1.0f, 1.0f ) * pc.timeWarpEndTransform;\n"
	"	vec3 cur = start + displayFraction * ( end - start );\n"
	"	float rcpZ = 1.0f / cur.z;\n"
	"\n"
	"	imageStore( dst, mesh, vec4( cur.xy * rcpZ, 0.0f, 0.0f ) );\n"
	"}\n";

static const unsigned int timeWarpTransformComputeProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x0000008a,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000005,0x00000004,0x6e69616d,0x00000000,0x0000000d,0x00000030,0x00060010,
	0x00000004,0x00000011,0x00000008,0x00000008,0x00000001,0x00030003,0x00000002,0x000001b8,
	0x00070004,0x415f4c47,0x655f4252,0x6e61686e,0x5f646563,0x6f79616c,0x00737475,0x00070004,
	0x455f4c47,0x735f5458,0x65646168,0x6f695f72,0x6f6c625f,0x00736b63,0x00040005,0x00000004,
	0x6e69616d,0x00000000,0x00040005,0x00000009,0x6873656d,0x00000000,0x00080005,0x0000000d,
	0x475f6c67,0x61626f6c,0x766e496c,0x7461636f,0x496e6f69,0x00000044,0x00060005,0x0000001a,
	0x68737550,0x736e6f43,0x746e6174,0x00000073,0x00090006,0x0000001a,0x00000000,0x656d6974,
	0x70726157,0x72617453,0x61725474,0x6f66736e,0x00006d72,0x00090006,0x0000001a,0x00000001,
	0x656d6974,0x70726157,0x54646e45,0x736e6172,0x6d726f66,0x00000000,0x00060006,0x0000001a,
	0x00000002,0x656d6964,0x6f69736e,0x0000736e,0x00040006,0x0000001a,0x00000003,0x00657965,
	0x00030005,0x0000001c,0x00006370,0x00060005,0x0000002f,0x54657965,0x73656c69,0x65646957,
	0x00000000,0x00070005,0x00000030,0x4e5f6c67,0x6f576d75,0x72476b72,0x7370756f,0x00000000,
	0x00060005,0x00000039,0x54657965,0x73656c69,0x68676948,0x00000000,0x00040005,0x00000041,
	0x726f6f63,0x00007364,0x00030005,0x00000044,0x00637273,0x00060005,0x0000004a,0x70736964,
	0x4679616c,0x74636172,0x006e6f69,0x00040005,0x0000005b,0x72617473,0x00000074,0x00030005,
	0x00000067,0x00646e65,0x00030005,0x0000006f,0x00727563,0x00040005,0x00000077,0x5a706372,
	0x00000000,0x00030005,0x0000007e,0x00747364,0x00040047,0x0000000d,0x0000000b,0x0000001c,
	0x00040048,0x0000001a,0x00000000,0x00000005,0x00050048,0x0000001a,0x00000000,0x00000023,
	0x00000000,0x00050048,0x0000001a,0x00000000,0x00000007,0x00000010,0x00040048,0x0000001a,
	0x00000001,0x00000005,0x00050048,0x0000001a,0x00000001,0x00000023,0x00000030,0x00050048,
	0x0000001a,0x00000001,0x00000007,0x00000010,0x00050048,0x0000001a,0x00000002,0x00000023,
	0x00000060,0x00050048,0x0000001a,0x00000003,0x00000023,0x00000068,0x00030047,0x0000001a,
	0x00000002,0x00040047,0x00000030,0x0000000b,0x00000018,0x00040047,0x00000044,0x00000022,
	0x00000000,0x00040047,0x00000044,0x00000021,0x00000001,0x00030047,0x00000044,0x00000018,
	0x00040047,0x0000007e,0x00000022,0x00000000,0x00040047,0x0000007e,0x00000021,0x00000000,
	0x00030047,0x0000007e,0x00000019,0x00040047,0x00000089,0x0000000b,0x00000019,0x00020013,
	0x00000002,0x00030021,0x00000003,0x00000002,0x00040015,0x00000006,0x00000020,0x00000001,
	0x00040017,0x00000007,0x00000006,0x00000002,0x00040020,0x00000008,0x00000007,0x00000007,
	0x00040015,0x0000000a,0x00000020,0x00000000,0x00040017,0x0000000b,0x0000000a,0x00000003,
	0x00040020,0x0000000c,0x00000001,0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,
	0x00040017,0x0000000e,0x0000000a,0x00000002,0x00020014,0x00000012,0x0004002b,0x0000000a,
	0x00000013,0x00000000,0x00040020,0x00000014,0x00000007,0x00000006,0x00030016,0x00000017,
	0x00000020,0x00040017,0x00000018,0x00000017,0x00000004,0x00040018,0x00000019,0x00000018,
	0x00000003,0x0006001e,0x0000001a,0x00000019,0x00000019,0x00000007,0x00000006,0x00040020,
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
	0x0004002b,0x00000017,0x0000005d,0xbf800000,0x0004002b,0x00000017,0x0000005e,0x3f800000,
	0x0004002b,0x00000006,0x00000062,0x00000000,0x00040020,0x00000063,0x00000009,0x00000019,
	0x0004002b,0x0000000a,0x00000078,0x00000002,0x00090019,0x0000007c,0x00000017,0x00000001,
	0x00000000,0x00000000,0x00000000,0x00000002,0x00000002,0x00040020,0x0000007d,0x00000000,
	0x0000007c,0x0004003b,0x0000007d,0x0000007e,0x00000000,0x0004002b,0x00000017,0x00000085,
	0x00000000,0x0006002c,0x0000000b,0x00000089,0x00000034,0x00000034,0x00000025,0x00050036,
	0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000008,
	0x00000009,0x00000007,0x0004003b,0x00000014,0x0000002f,0x00000007,0x0004003b,0x00000014,
	0x00000039,0x00000007,0x0004003b,0x00000040,0x00000041,0x00000007,0x0004003b,0x00000049,
	0x0000004a,0x00000007,0x0004003b,0x0000005a,0x0000005b,0x00000007,0x0004003b,0x0000005a,
	0x00000067,0x00000007,0x0004003b,0x0000005a,0x0000006f,0x00000007,0x0004003b,0x00000049,
	0x00000077,0x00000007,0x0004003d,0x0000000b,0x0000000f,0x0000000d,0x0007004f,0x0000000e,
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
	0x00000057,0x0003003e,0x0000004a,0x00000058,0x0004003d,0x0000003f,0x0000005c,0x00000041,
	0x00050051,0x00000017,0x0000005f,0x0000005c,0x00000000,0x00050051,0x00000017,0x00000060,
	0x0000005c,0x00000001,0x00070050,0x00000018,0x00000061,0x0000005f,0x00000060,0x0000005d,
	0x0000005e,0x00050041,0x00000063,0x00000064,0x0000001c,0x00000062,0x0004003d,0x00000019,
	0x00000065,0x00000064,0x00050090,0x00000059,0x00000066,0x00000061,0x00000065,0x0003003e,
	0x0000005b,0x00000066,0x0004003d,0x0000003f,0x00000068,0x00000041,0x00050051,0x00000017,
	0x00000069,0x00000068,0x00000000,0x00050051,0x00000017,0x0000006a,0x00000068,0x00000001,
	0x00070050,0x00000018,0x0000006b,0x00000069,0x0000006a,0x0000005d,0x0000005e,0x00050041,
	0x00000063,0x0000006c,0x0000001c,0x00000037,0x0004003d,0x00000019,0x0000006d,0x0000006c,
	0x00050090,0x00000059,0x0000006e,0x0000006b,0x0000006d,0x0003003e,0x00000067,0x0000006e,
	0x0004003d,0x00000059,0x00000070,0x0000005b,0x0004003d,0x00000017,0x00000071,0x0000004a,
	0x0004003d,0x00000059,0x00000072,0x00000067,0x0004003d,0x00000059,0x00000073,0x0000005b,
	0x00050083,0x00000059,0x00000074,0x00000072,0x00000073,0x0005008e,0x00000059,0x00000075,
	0x00000074,0x00000071,0x00050081,0x00000059,0x00000076,0x00000070,0x00000075,0x0003003e,
	0x0000006f,0x00000076,0x00050041,0x00000049,0x00000079,0x0000006f,0x00000078,0x0004003d,
	0x00000017,0x0000007a,0x00000079,0x00050088,0x00000017,0x0000007b,0x0000005e,0x0000007a,
	0x0003003e,0x00000077,0x0000007b,0x0004003d,0x0000007c,0x0000007f,0x0000007e,0x0004003d,
	0x00000007,0x00000080,0x00000009,0x0004003d,0x00000059,0x00000081,0x0000006f,0x0007004f,
	0x0000003f,0x00000082,0x00000081,0x00000081,0x00000000,0x00000001,0x0004003d,0x00000017,
	0x00000083,0x00000077,0x0005008e,0x0000003f,0x00000084,0x00000082,0x00000083,0x00050051,
	0x00000017,0x00000086,0x00000084,0x00000000,0x00050051,0x00000017,0x00000087,0x00000084,
	0x00000001,0x00070050,0x00000018,0x00000088,0x00000086,0x00000087,0x00000085,0x00000085,
	0x00040063,0x0000007f,0x00000080,0x00000088,0x000100fd,0x00010038
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

static const ksGpuProgramParm timeWarpSpatialComputeProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,				KS_GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST,				"dest",				0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE,			"eyeImage",			1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G,		"warpImageG",		2 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE,		"imageScale",		0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS,		"imageBias",		8 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET,	"eyePixelOffset",	16 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER,		"imageLayer",		24 }
};

#define SPATIAL_LOCAL_SIZE_X		8
#define SPATIAL_LOCAL_SIZE_Y		8

static const char timeWarpSpatialComputeProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
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
	"layout( std430, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =  0 ) highp vec2 imageScale;\n"
	"	layout( offset =  8 ) highp vec2 imageBias;\n"
	"	layout( offset = 16 ) ivec2 eyePixelOffset;\n"
	"	layout( offset = 24 ) int imageLayer;\n"
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
	0x00000011,0x00000008,0x00000008,0x00000001,0x00030003,0x00000002,0x000001b8,0x00070004,
	0x415f4c47,0x655f4252,0x6e61686e,0x5f646563,0x6f79616c,0x00737475,0x00070004,0x455f4c47,
	0x735f5458,0x65646168,0x6f695f72,0x6f6c625f,0x00736b63,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00040005,0x00000009,0x656c6974,0x00000000,0x00080005,0x0000000d,0x475f6c67,
	0x61626f6c,0x766e496c,0x7461636f,0x496e6f69,0x00000044,0x00060005,0x00000017,0x68737550,
	0x736e6f43,0x746e6174,0x00000073,0x00060006,0x00000017,0x00000000,0x67616d69,0x61635365,
	0x0000656c,0x00060006,0x00000017,0x00000001,0x67616d69,0x61694265,0x00000073,0x00070006,
	0x00000017,0x00000002,0x50657965,0x6c657869,0x7366664f,0x00007465,0x00060006,0x00000017,
	0x00000003,0x67616d69,0x79614c65,0x00007265,0x00030005,0x00000019,0x00006370,0x00050005,
	0x00000023,0x43657965,0x64726f6f,0x00000073,0x00050005,0x00000027,0x70726177,0x67616d49,
	0x00004765,0x00040005,0x0000002f,0x61626772,0x00000000,0x00050005,0x00000033,0x49657965,
	0x6567616d,0x00000000,0x00040005,0x00000042,0x74736564,0x00000000,0x00040047,0x0000000d,
	0x0000000b,0x0000001c,0x00050048,0x00000017,0x00000000,0x00000023,0x00000000,0x00050048,
	0x00000017,0x00000001,0x00000023,0x00000008,0x00050048,0x00000017,0x00000002,0x00000023,
	0x00000010,0x00050048,0x00000017,0x00000003,0x00000023,0x00000018,0x00030047,0x00000017,
	0x00000002,0x00040047,0x00000027,0x00000022,0x00000000,0x00040047,0x00000027,0x00000021,
	0x00000002,0x00040047,0x00000033,0x00000022,0x00000000,0x00040047,0x00000033,0x00000021,
	0x00000001,0x00040047,0x00000042,0x00000022,0x00000000,0x00040047,0x00000042,0x00000021,
	0x00000000,0x00030047,0x00000042,0x00000019,0x00040047,0x0000004f,0x0000000b,0x00000019,
	0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,
	0x00040017,0x00000007,0x00000006,0x00000002,0x00040020,0x00000008,0x00000007,0x00000007,
	0x00040015,0x0000000a,0x00000020,0x00000000,0x00040017,0x0000000b,0x0000000a,0x00000003,
	0x00040020,0x0000000c,0x00000001,0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,
	0x00040017,0x0000000e,0x0000000a,0x00000002,0x0004002b,0x00000006,0x00000012,0x3f000000,
	0x0005002c,0x00000007,0x00000013,0x00000012,0x00000012,0x00040015,0x00000015,0x00000020,
	0x00000001,0x00040017,0x00000016,0x00000015,0x00000002,0x0006001e,0x00000017,0x00000007,
	0x00000007,0x00000016,0x00000015,0x00040020,0x00000018,0x00000009,0x00000017,0x0004003b,
	0x00000018,0x00000019,0x00000009,0x0004002b,0x00000015,0x0000001a,0x00000000,0x00040020,
	0x0000001b,0x00000009,0x00000007,0x0004002b,0x00000015,0x0000001f,0x00000001,0x00090019,
	0x00000024,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,
	0x0003001b,0x00000025,0x00000024,0x00040020,0x00000026,0x00000000,0x00000025,0x0004003b,
	0x00000026,0x00000027,0x00000000,0x00040017,0x0000002a,0x00000006,0x00000004,0x0004002b,
	0x00000006,0x0000002b,0x00000000,0x00040020,0x0000002e,0x00000007,0x0000002a,0x00090019,
	0x00000030,0x00000006,0x00000001,0x00000000,0x00000001,0x00000000,0x00000001,0x00000000,
	0x0003001b,0x00000031,0x00000030,0x00040020,0x00000032,0x00000000,0x00000031,0x0004003b,
	0x00000032,0x00000033,0x00000000,0x0004002b,0x00000015,0x00000036,0x00000003,0x00040020,
	0x00000037,0x00000009,0x00000015,0x00040017,0x0000003b,0x00000006,0x00000003,0x00090019,
	0x00000040,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,0x00000002,0x00000004,
	0x00040020,0x00000041,0x00000000,0x00000040,0x0004003b,0x00000041,0x00000042,0x00000000,
	0x0004002b,0x00000015,0x00000047,0x00000002,0x00040020,0x00000048,0x00000009,0x00000016,
	0x0004002b,0x0000000a,0x0000004d,0x00000008,0x0004002b,0x0000000a,0x0000004e,0x00000001,
	0x0006002c,0x0000000b,0x0000004f,0x0000004d,0x0000004d,0x0000004e,0x00050036,0x00000002,
	0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000008,0x00000009,
	0x00000007,0x0004003b,0x00000008,0x00000023,0x00000007,0x0004003b,0x0000002e,0x0000002f,
	0x00000007,0x0004003d,0x0000000b,0x0000000f,0x0000000d,0x0007004f,0x0000000e,0x00000010,
	0x0000000f,0x0000000f,0x00000000,0x00000001,0x00040070,0x00000007,0x00000011,0x00000010,
	0x00050081,0x00000007,0x00000014,0x00000011,0x00000013,0x00050041,0x0000001b,0x0000001c,
	0x00000019,0x0000001a,0x0004003d,0x00000007,0x0000001d,0x0000001c,0x00050085,0x00000007,
	0x0000001e,0x00000014,0x0000001d,0x00050041,0x0000001b,0x00000020,0x00000019,0x0000001f,
	0x0004003d,0x00000007,0x00000021,0x00000020,0x00050081,0x00000007,0x00000022,0x0000001e,
	0x00000021,0x0003003e,0x00000009,0x00000022,0x0004003d,0x00000025,0x00000028,0x00000027,
	0x0004003d,0x00000007,0x00000029,0x00000009,0x00070058,0x0000002a,0x0000002c,0x00000028,
	0x00000029,0x00000002,0x0000002b,0x0007004f,0x00000007,0x0000002d,0x0000002c,0x0000002c,
	0x00000000,0x00000001,0x0003003e,0x00000023,0x0000002d,0x0004003d,0x00000031,0x00000034,
	0x00000033,0x0004003d,0x00000007,0x00000035,0x00000023,0x00050041,0x00000037,0x00000038,
	0x00000019,0x00000036,0x0004003d,0x00000015,0x00000039,0x00000038,0x0004006f,0x00000006,
	0x0000003a,0x00000039,0x00050051,0x00000006,0x0000003c,0x00000035,0x00000000,0x00050051,
	0x00000006,0x0000003d,0x00000035,0x00000001,0x00060050,0x0000003b,0x0000003e,0x0000003c,
	0x0000003d,0x0000003a,0x00070058,0x0000002a,0x0000003f,0x00000034,0x0000003e,0x00000002,
	0x0000002b,0x0003003e,0x0000002f,0x0000003f,0x0004003d,0x00000040,0x00000043,0x00000042,
	0x0004003d,0x0000000b,0x00000044,0x0000000d,0x0007004f,0x0000000e,0x00000045,0x00000044,
	0x00000044,0x00000000,0x00000001,0x0004007c,0x00000016,0x00000046,0x00000045,0x00050041,
	0x00000048,0x00000049,0x00000019,0x00000047,0x0004003d,0x00000016,0x0000004a,0x00000049,
	0x00050080,0x00000016,0x0000004b,0x00000046,0x0000004a,0x0004003d,0x0000002a,0x0000004c,
	0x0000002f,0x00040063,0x00000043,0x0000004b,0x0000004c,0x000100fd,0x00010038
};

static const ksGpuProgramParm timeWarpChromaticComputeProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,				KS_GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST,				"dest",				0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE,			"eyeImage",			1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_R,		"warpImageR",		2 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G,		"warpImageG",		3 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_B,		"warpImageB",		4 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE,		"imageScale",		0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS,		"imageBias",		8 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET,	"eyePixelOffset",	16 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER,		"imageLayer",		24 }
};

#define CHROMATIC_LOCAL_SIZE_X		8
#define CHROMATIC_LOCAL_SIZE_Y		8

static const char timeWarpChromaticComputeProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
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
	"layout( std430, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =  0 ) highp vec2 imageScale;\n"
	"	layout( offset =  8 ) highp vec2 imageBias;\n"
	"	layout( offset = 16 ) ivec2 eyePixelOffset;\n"
	"	layout( offset = 24 ) int imageLayer;\n"
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
	0x00000011,0x00000008,0x00000008,0x00000001,0x00030003,0x00000002,0x000001b8,0x00070004,
	0x415f4c47,0x655f4252,0x6e61686e,0x5f646563,0x6f79616c,0x00737475,0x00070004,0x455f4c47,
	0x735f5458,0x65646168,0x6f695f72,0x6f6c625f,0x00736b63,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00040005,0x00000009,0x656c6974,0x00000000,0x00080005,0x0000000d,0x475f6c67,
	0x61626f6c,0x766e496c,0x7461636f,0x496e6f69,0x00000044,0x00060005,0x00000017,0x68737550,
	0x736e6f43,0x746e6174,0x00000073,0x00060006,0x00000017,0x00000000,0x67616d69,0x61635365,
	0x0000656c,0x00060006,0x00000017,0x00000001,0x67616d69,0x61694265,0x00000073,0x00070006,
	0x00000017,0x00000002,0x50657965,0x6c657869,0x7366664f,0x00007465,0x00060006,0x00000017,
	0x00000003,0x67616d69,0x79614c65,0x00007265,0x00030005,0x00000019,0x00006370,0x00050005,
	0x00000023,0x43657965,0x64726f6f,0x00005273,0x00050005,0x00000027,0x70726177,0x67616d49,
	0x00005265,0x00050005,0x0000002e,0x43657965,0x64726f6f,0x00004773,0x00050005,0x0000002f,
	0x70726177,0x67616d49,0x00004765,0x00050005,0x00000034,0x43657965,0x64726f6f,0x00004273,
	0x00050005,0x00000035,0x70726177,0x67616d49,0x00004265,0x00040005,0x0000003b,0x61626772,
	0x00000000,0x00050005,0x0000003f,0x49657965,0x6567616d,0x00000000,0x00040005,0x0000006d,
	0x74736564,0x00000000,0x00040047,0x0000000d,0x0000000b,0x0000001c,0x00050048,0x00000017,
	0x00000000,0x00000023,0x00000000,0x00050048,0x00000017,0x00000001,0x00000023,0x00000008,
	0x00050048,0x00000017,0x00000002,0x00000023,0x00000010,0x00050048,0x00000017,0x00000003,
	0x00000023,0x00000018,0x00030047,0x00000017,0x00000002,0x00040047,0x00000027,0x00000022,
	0x00000000,0x00040047,0x00000027,0x00000021,0x00000002,0x00040047,0x0000002f,0x00000022,
	0x00000000,0x00040047,0x0000002f,0x00000021,0x00000003,0x00040047,0x00000035,0x00000022,
	0x00000000,0x00040047,0x00000035,0x00000021,0x00000004,0x00040047,0x0000003f,0x00000022,
	0x00000000,0x00040047,0x0000003f,0x00000021,0x00000001,0x00040047,0x0000006d,0x00000022,
	0x00000000,0x00040047,0x0000006d,0x00000021,0x00000000,0x00030047,0x0000006d,0x00000019,
	0x00040047,0x00000079,0x0000000b,0x00000019,0x00020013,0x00000002,0x00030021,0x00000003,
	0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000002,
	0x00040020,0x00000008,0x00000007,0x00000007,0x00040015,0x0000000a,0x00000020,0x00000000,
	0x00040017,0x0000000b,0x0000000a,0x00000003,0x00040020,0x0000000c,0x00000001,0x0000000b,
	0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040017,0x0000000e,0x0000000a,0x00000002,
	0x0004002b,0x00000006,0x00000012,0x3f000000,0x0005002c,0x00000007,0x00000013,0x00000012,
	0x00000012,0x00040015,0x00000015,0x00000020,0x00000001,0x00040017,0x00000016,0x00000015,
	0x00000002,0x0006001e,0x00000017,0x00000007,0x00000007,0x00000016,0x00000015,0x00040020,
	0x00000018,0x00000009,0x00000017,0x0004003b,0x00000018,0x00000019,0x00000009,0x0004002b,
	0x00000015,0x0000001a,0x00000000,0x00040020,0x0000001b,0x00000009,0x00000007,0x0004002b,
	0x00000015,0x0000001f,0x00000001,0x00090019,0x00000024,0x00000006,0x00000001,0x00000000,
	0x00000000,0x00000000,0x00000001,0x00000000,0x0003001b,0x00000025,0x00000024,0x00040020,
	0x00000026,0x00000000,0x00000025,0x0004003b,0x00000026,0x00000027,0x00000000,0x00040017,
	0x0000002a,0x00000006,0x00000004,0x0004002b,0x00000006,0x0000002b,0x00000000,0x0004003b,
	0x00000026,0x0000002f,0x00000000,0x0004003b,0x00000026,0x00000035,0x00000000,0x00040020,
	0x0000003a,0x00000007,0x0000002a,0x00090019,0x0000003c,0x00000006,0x00000001,0x00000000,
	0x00000001,0x00000000,0x00000001,0x00000000,0x0003001b,0x0000003d,0x0000003c,0x00040020,
	0x0000003e,0x00000000,0x0000003d,0x0004003b,0x0000003e,0x0000003f,0x00000000,0x0004002b,
	0x00000015,0x00000042,0x00000003,0x00040020,0x00000043,0x00000009,0x00000015,0x00040017,
	0x00000047,0x00000006,0x00000003,0x0004002b,0x0000000a,0x0000004c,0x00000000,0x00040020,
	0x0000004e,0x00000007,0x00000006,0x0004002b,0x0000000a,0x00000059,0x00000001,0x0004002b,
	0x0000000a,0x00000065,0x00000002,0x0004002b,0x00000006,0x00000068,0x3f800000,0x0004002b,
	0x0000000a,0x00000069,0x00000003,0x00090019,0x0000006b,0x00000006,0x00000001,0x00000000,
	0x00000000,0x00000000,0x00000002,0x00000004,0x00040020,0x0000006c,0x00000000,0x0000006b,
	0x0004003b,0x0000006c,0x0000006d,0x00000000,0x0004002b,0x00000015,0x00000072,0x00000002,
	0x00040020,0x00000073,0x00000009,0x00000016,0x0004002b,0x0000000a,0x00000078,0x00000008,
	0x0006002c,0x0000000b,0x00000079,0x00000078,0x00000078,0x00000059,0x00050036,0x00000002,
	0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000008,0x00000009,
	0x00000007,0x0004003b,0x00000008,0x00000023,0x00000007,0x0004003b,0x00000008,0x0000002e,
	0x00000007,0x0004003b,0x00000008,0x00000034,0x00000007,0x0004003b,0x0000003a,0x0000003b,
	0x00000007,0x0004003d,0x0000000b,0x0000000f,0x0000000d,0x0007004f,0x0000000e,0x00000010,
	0x0000000f,0x0000000f,0x00000000,0x00000001,0x00040070,0x00000007,0x00000011,0x00000010,
	0x00050081,0x00000007,0x00000014,0x00000011,0x00000013,0x00050041,0x0000001b,0x0000001c,
	0x00000019,0x0000001a,0x0004003d,0x00000007,0x0000001d,0x0000001c,0x00050085,0x00000007,
	0x0000001e,0x00000014,0x0000001d,0x00050041,0x0000001b,0x00000020,0x00000019,0x0000001f,
	0x0004003d,0x00000007,0x00000021,0x00000020,0x00050081,0x00000007,0x00000022,0x0000001e,
	0x00000021,0x0003003e,0x00000009,0x00000022,0x0004003d,0x00000025,0x00000028,0x00000027,
	0x0004003d,0x00000007,0x00000029,0x00000009,0x00070058,0x0000002a,0x0000002c,0x00000028,
	0x00000029,0x00000002,0x0000002b,0x0007004f,0x00000007,0x0000002d,0x0000002c,0x0000002c,
	0x00000000,0x00000001,0x0003003e,0x00000023,0x0000002d,0x0004003d,0x00000025,0x00000030,
	0x0000002f,0x0004003d,0x00000007,0x00000031,0x00000009,0x00070058,0x0000002a,0x00000032,
	0x00000030,0x00000031,0x00000002,0x0000002b,0x0007004f,0x00000007,0x00000033,0x00000032,
	0x00000032,0x00000000,0x00000001,0x0003003e,0x0000002e,0x00000033,0x0004003d,0x00000025,
	0x00000036,0x00000035,0x0004003d,0x00000007,0x00000037,0x00000009,0x00070058,0x0000002a,
	0x00000038,0x00000036,0x00000037,0x00000002,0x0000002b,0x0007004f,0x00000007,0x00000039,
	0x00000038,0x00000038,0x00000000,0x00000001,0x0003003e,0x00000034,0x00000039,0x0004003d,
	0x0000003d,0x00000040,0x0000003f,0x0004003d,0x00000007,0x00000041,0x00000023,0x00050041,
	0x00000043,0x00000044,0x00000019,0x00000042,0x0004003d,0x00000015,0x00000045,0x00000044,
	0x0004006f,0x00000006,0x00000046,0x00000045,0x00050051,0x00000006,0x00000048,0x00000041,
	0x00000000,0x00050051,0x00000006,0x00000049,0x00000041,0x00000001,0x00060050,0x00000047,
	0x0000004a,0x00000048,0x00000049,0x00000046,0x00070058,0x0000002a,0x0000004b,0x00000040,
	0x0000004a,0x00000002,0x0000002b,0x00050051,0x00000006,0x0000004d,0x0000004b,0x00000000,
	0x00050041,0x0000004e,0x0000004f,0x0000003b,0x0000004c,0x0003003e,0x0000004f,0x0000004d,
	0x0004003d,0x0000003d,0x00000050,0x0000003f,0x0004003d,0x00000007,0x00000051,0x0000002e,
	0x00050041,0x00000043,0x00000052,0x00000019,0x00000042,0x0004003d,0x00000015,0x00000053,
	0x00000052,0x0004006f,0x00000006,0x00000054,0x00000053,0x00050051,0x00000006,0x00000055,
	0x00000051,0x00000000,0x00050051,0x00000006,0x00000056,0x00000051,0x00000001,0x00060050,
	0x00000047,0x00000057,0x00000055,0x00000056,0x00000054,0x00070058,0x0000002a,0x00000058,
	0x00000050,0x00000057,0x00000002,0x0000002b,0x00050051,0x00000006,0x0000005a,0x00000058,
	0x00000001,0x00050041,0x0000004e,0x0000005b,0x0000003b,0x00000059,0x0003003e,0x0000005b,
	0x0000005a,0x0004003d,0x0000003d,0x0000005c,0x0000003f,0x0004003d,0x00000007,0x0000005d,
	0x00000034,0x00050041,0x00000043,0x0000005e,0x00000019,0x00000042,0x0004003d,0x00000015,
	0x0000005f,0x0000005e,0x0004006f,0x00000006,0x00000060,0x0000005f,0x00050051,0x00000006,
	0x00000061,0x0000005d,0x00000000,0x00050051,0x00000006,0x00000062,0x0000005d,0x00000001,
	0x00060050,0x00000047,0x00000063,0x00000061,0x00000062,0x00000060,0x00070058,0x0000002a,
	0x00000064,0x0000005c,0x00000063,0x00000002,0x0000002b,0x00050051,0x00000006,0x00000066,
	0x00000064,0x00000002,0x00050041,0x0000004e,0x00000067,0x0000003b,0x00000065,0x0003003e,
	0x00000067,0x00000066,0x00050041,0x0000004e,0x0000006a,0x0000003b,0x00000069,0x0003003e,
	0x0000006a,0x00000068,0x0004003d,0x0000006b,0x0000006e,0x0000006d,0x0004003d,0x0000000b,
	0x0000006f,0x0000000d,0x0007004f,0x0000000e,0x00000070,0x0000006f,0x0000006f,0x00000000,
	0x00000001,0x0004007c,0x00000016,0x00000071,0x00000070,0x00050041,0x00000073,0x00000074,
	0x00000019,0x00000072,0x0004003d,0x00000016,0x00000075,0x00000074,0x00050080,0x00000016,
	0x00000076,0x00000071,0x00000075,0x0004003d,0x0000002a,0x00000077,0x0000003b,0x00040063,
	0x0000006e,0x00000076,0x00000077,0x000100fd,0x00010038
};

static void ksTimeWarpCompute_Create( ksGpuContext * context, ksTimeWarpCompute * compute,
									const ksHmdInfo * hmdInfo, ksGpuRenderPass * renderPass )
{
	UNUSED_PARM( renderPass );

	memset( compute, 0, sizeof( ksTimeWarpCompute ) );

	compute->hmdInfo = *hmdInfo;

	const int numMeshCoords = ( hmdInfo->eyeTilesHigh + 1 ) * ( hmdInfo->eyeTilesWide + 1 );
	ksMeshCoord * meshCoordsBasePtr = (ksMeshCoord *) malloc( NUM_EYES * NUM_COLOR_CHANNELS * numMeshCoords * sizeof( ksMeshCoord ) );
	ksMeshCoord * meshCoords[NUM_EYES][NUM_COLOR_CHANNELS] =
	{
		{ meshCoordsBasePtr + 0 * numMeshCoords, meshCoordsBasePtr + 1 * numMeshCoords, meshCoordsBasePtr + 2 * numMeshCoords },
		{ meshCoordsBasePtr + 3 * numMeshCoords, meshCoordsBasePtr + 4 * numMeshCoords, meshCoordsBasePtr + 5 * numMeshCoords }
	};
	BuildDistortionMeshes( meshCoords, hmdInfo );

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
			ksGpuTexture_Create2D( context, &compute->distortionImage[eye][channel],
								KS_GPU_TEXTURE_FORMAT_R32G32B32A32_SFLOAT, KS_GPU_SAMPLE_COUNT_1,
								hmdInfo->eyeTilesWide + 1, hmdInfo->eyeTilesHigh + 1, 1, KS_GPU_TEXTURE_USAGE_STORAGE, rgbaFloat, rgbaSize );
			ksGpuTexture_Create2D( context, &compute->timeWarpImage[eye][channel],
								KS_GPU_TEXTURE_FORMAT_R16G16B16A16_SFLOAT, KS_GPU_SAMPLE_COUNT_1,
								hmdInfo->eyeTilesWide + 1, hmdInfo->eyeTilesHigh + 1, 1, KS_GPU_TEXTURE_USAGE_STORAGE | KS_GPU_TEXTURE_USAGE_SAMPLED, NULL, 0 );
		}
	}
	free( rgbaFloat );

	free( meshCoordsBasePtr );

	ksGpuComputeProgram_Create( context, &compute->timeWarpTransformProgram,
								PROGRAM( timeWarpTransformComputeProgram ), sizeof( PROGRAM( timeWarpTransformComputeProgram ) ),
								timeWarpTransformComputeProgramParms, ARRAY_SIZE( timeWarpTransformComputeProgramParms ) );
	ksGpuComputeProgram_Create( context, &compute->timeWarpSpatialProgram,
								PROGRAM( timeWarpSpatialComputeProgram ), sizeof( PROGRAM( timeWarpSpatialComputeProgram ) ),
								timeWarpSpatialComputeProgramParms, ARRAY_SIZE( timeWarpSpatialComputeProgramParms ) );
	ksGpuComputeProgram_Create( context, &compute->timeWarpChromaticProgram,
								PROGRAM( timeWarpChromaticComputeProgram ), sizeof( PROGRAM( timeWarpChromaticComputeProgram ) ),
								timeWarpChromaticComputeProgramParms, ARRAY_SIZE( timeWarpChromaticComputeProgramParms ) );

	ksGpuComputePipeline_Create( context, &compute->timeWarpTransformPipeline, &compute->timeWarpTransformProgram );
	ksGpuComputePipeline_Create( context, &compute->timeWarpSpatialPipeline, &compute->timeWarpSpatialProgram );
	ksGpuComputePipeline_Create( context, &compute->timeWarpChromaticPipeline, &compute->timeWarpChromaticProgram );

	ksGpuTimer_Create( context, &compute->timeWarpGpuTime );
}

static void ksTimeWarpCompute_Destroy( ksGpuContext * context, ksTimeWarpCompute * compute )
{
	ksGpuTimer_Destroy( context, &compute->timeWarpGpuTime );

	ksGpuComputePipeline_Destroy( context, &compute->timeWarpTransformPipeline );
	ksGpuComputePipeline_Destroy( context, &compute->timeWarpSpatialPipeline );
	ksGpuComputePipeline_Destroy( context, &compute->timeWarpChromaticPipeline );

	ksGpuComputeProgram_Destroy( context, &compute->timeWarpTransformProgram );
	ksGpuComputeProgram_Destroy( context, &compute->timeWarpSpatialProgram );
	ksGpuComputeProgram_Destroy( context, &compute->timeWarpChromaticProgram );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			ksGpuTexture_Destroy( context, &compute->distortionImage[eye][channel] );
			ksGpuTexture_Destroy( context, &compute->timeWarpImage[eye][channel] );
		}
	}

	memset( compute, 0, sizeof( ksTimeWarpCompute ) );
}

static void ksTimeWarpCompute_Render( ksGpuCommandBuffer * commandBuffer, ksTimeWarpCompute * compute,
									ksGpuFramebuffer * framebuffer,
									const ksNanoseconds refreshStartTime, const ksNanoseconds refreshEndTime,
									const ksMatrix4x4f * projectionMatrix, const ksMatrix4x4f * viewMatrix,
									ksGpuTexture * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, ksTimeWarpBarGraphs * bargraphs,
									ksNanoseconds cpuTimes[PROFILE_TIME_MAX], ksNanoseconds gpuTimes[PROFILE_TIME_MAX] )
{
	const ksNanoseconds t0 = GetTimeNanoseconds();

	ksMatrix4x4f displayRefreshStartViewMatrix;
	ksMatrix4x4f displayRefreshEndViewMatrix;
	GetHmdViewMatrixForTime( &displayRefreshStartViewMatrix, refreshStartTime );
	GetHmdViewMatrixForTime( &displayRefreshEndViewMatrix, refreshEndTime );

	ksMatrix4x4f timeWarpStartTransform;
	ksMatrix4x4f timeWarpEndTransform;
	CalculateTimeWarpTransform( &timeWarpStartTransform, projectionMatrix, viewMatrix, &displayRefreshStartViewMatrix );
	CalculateTimeWarpTransform( &timeWarpEndTransform, projectionMatrix, viewMatrix, &displayRefreshEndViewMatrix );

	ksMatrix3x4f timeWarpStartTransform3x4;
	ksMatrix3x4f timeWarpEndTransform3x4;
	ksMatrix3x4f_CreateFromMatrix4x4f( &timeWarpStartTransform3x4, &timeWarpStartTransform );
	ksMatrix3x4f_CreateFromMatrix4x4f( &timeWarpEndTransform3x4, &timeWarpEndTransform );

	ksGpuCommandBuffer_BeginPrimary( commandBuffer );
	ksGpuCommandBuffer_BeginFramebuffer( commandBuffer, framebuffer, 0, KS_GPU_TEXTURE_USAGE_STORAGE );

	ksGpuCommandBuffer_BeginTimer( commandBuffer, &compute->timeWarpGpuTime );

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			ksGpuCommandBuffer_ChangeTextureUsage( commandBuffer, &compute->timeWarpImage[eye][channel], KS_GPU_TEXTURE_USAGE_STORAGE );
			ksGpuCommandBuffer_ChangeTextureUsage( commandBuffer, &compute->distortionImage[eye][channel], KS_GPU_TEXTURE_USAGE_STORAGE );
		}
	}

	const ksVector2i dimensions = { compute->hmdInfo.eyeTilesWide + 1, compute->hmdInfo.eyeTilesHigh + 1 };
	const int eyeIndex[NUM_EYES] = { 0, 1 };

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			ksGpuComputeCommand command;
			ksGpuComputeCommand_Init( &command );
			ksGpuComputeCommand_SetPipeline( &command, &compute->timeWarpTransformPipeline );
			ksGpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_DST, &compute->timeWarpImage[eye][channel] );
			ksGpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_SRC, &compute->distortionImage[eye][channel] );
			ksGpuComputeCommand_SetParmFloatMatrix3x4( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM, &timeWarpStartTransform3x4 );
			ksGpuComputeCommand_SetParmFloatMatrix3x4( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM, &timeWarpEndTransform3x4 );
			ksGpuComputeCommand_SetParmIntVector2( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_DIMENSIONS, &dimensions );
			ksGpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE, &eyeIndex[eye] );
			ksGpuComputeCommand_SetDimensions( &command, ( dimensions.x + TRANSFORM_LOCAL_SIZE_X - 1 ) / TRANSFORM_LOCAL_SIZE_X, 
														( dimensions.y + TRANSFORM_LOCAL_SIZE_Y - 1 ) / TRANSFORM_LOCAL_SIZE_Y, 1 );

			ksGpuCommandBuffer_SubmitComputeCommand( commandBuffer, &command );
		}
	}

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			ksGpuCommandBuffer_ChangeTextureUsage( commandBuffer, &compute->timeWarpImage[eye][channel], KS_GPU_TEXTURE_USAGE_SAMPLED );
		}
	}

	const int screenWidth = ksGpuFramebuffer_GetWidth( framebuffer );
	const int screenHeight = ksGpuFramebuffer_GetHeight( framebuffer );
	const int eyePixelsWide = screenWidth / NUM_EYES;
	const int eyePixelsHigh = screenHeight * compute->hmdInfo.eyeTilesHigh * compute->hmdInfo.tilePixelsHigh / compute->hmdInfo.displayPixelsHigh;
	const ksVector2f imageScale =
	{
		(float)compute->hmdInfo.eyeTilesWide / ( compute->hmdInfo.eyeTilesWide + 1 ) / eyePixelsWide,
		(float)compute->hmdInfo.eyeTilesHigh / ( compute->hmdInfo.eyeTilesHigh + 1 ) / eyePixelsHigh
	};
	const ksVector2f imageBias =
	{
		0.5f / ( compute->hmdInfo.eyeTilesWide + 1 ),
		0.5f / ( compute->hmdInfo.eyeTilesHigh + 1 )
	};
	const ksVector2i eyePixelOffset[NUM_EYES] =
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

		ksGpuComputeCommand command;
		ksGpuComputeCommand_Init( &command );
		ksGpuComputeCommand_SetPipeline( &command, correctChromaticAberration ? &compute->timeWarpChromaticPipeline : &compute->timeWarpSpatialPipeline );
		ksGpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST, ksGpuFramebuffer_GetColorTexture( framebuffer ) );
		ksGpuComputeCommand_SetParmTextureSampled( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE, eyeTexture[eye] );
		ksGpuComputeCommand_SetParmTextureSampled( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_R, &compute->timeWarpImage[eye][0] );
		ksGpuComputeCommand_SetParmTextureSampled( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G, &compute->timeWarpImage[eye][1] );
		ksGpuComputeCommand_SetParmTextureSampled( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_B, &compute->timeWarpImage[eye][2] );
		ksGpuComputeCommand_SetParmFloatVector2( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE, &imageScale );
		ksGpuComputeCommand_SetParmFloatVector2( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS, &imageBias );
		ksGpuComputeCommand_SetParmIntVector2( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET, &eyePixelOffset[eye] );
		ksGpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER, &eyeArrayLayer[eye] );
		ksGpuComputeCommand_SetDimensions( &command, screenWidth / ( correctChromaticAberration ? CHROMATIC_LOCAL_SIZE_X : SPATIAL_LOCAL_SIZE_X ) / 2,
													screenHeight / ( correctChromaticAberration ? CHROMATIC_LOCAL_SIZE_Y : SPATIAL_LOCAL_SIZE_Y ), 1 );

		ksGpuCommandBuffer_SubmitComputeCommand( commandBuffer, &command );
	}

	const ksNanoseconds t1 = GetTimeNanoseconds();

	ksTimeWarpBarGraphs_UpdateCompute( commandBuffer, bargraphs );
	ksTimeWarpBarGraphs_RenderCompute( commandBuffer, bargraphs, framebuffer );

	ksGpuCommandBuffer_EndTimer( commandBuffer, &compute->timeWarpGpuTime );

	ksGpuCommandBuffer_EndFramebuffer( commandBuffer, framebuffer, 0, KS_GPU_TEXTURE_USAGE_PRESENTATION );
	ksGpuCommandBuffer_EndPrimary( commandBuffer );

	ksGpuCommandBuffer_SubmitPrimary( commandBuffer );

	const ksNanoseconds t2 = GetTimeNanoseconds();

	cpuTimes[PROFILE_TIME_TIME_WARP] = t1 - t0;
	cpuTimes[PROFILE_TIME_BAR_GRAPHS] = t2 - t1;
	cpuTimes[PROFILE_TIME_BLIT] = 0;

	const ksNanoseconds barGraphGpuTime = ksTimeWarpBarGraphs_GetGpuNanosecondsCompute( bargraphs );

	gpuTimes[PROFILE_TIME_TIME_WARP] = ksGpuTimer_GetNanoseconds( &compute->timeWarpGpuTime ) - barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BAR_GRAPHS] = barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BLIT] = 0;
}

/*
================================================================================================================================

Time warp rendering.

ksTimeWarp
ksTimeWarpImplementation

static void ksTimeWarp_Create( ksTimeWarp * timeWarp, ksGpuWindow * window );
static void ksTimeWarp_Destroy( ksTimeWarp * timeWarp, ksGpuWindow * window );

static void ksTimeWarp_SetBarGraphState( ksTimeWarp * timeWarp, const ksBarGraphState state );
static void ksTimeWarp_CycleBarGraphState( ksTimeWarp * timeWarp );
static void ksTimeWarp_SetImplementation( ksTimeWarp * timeWarp, const ksTimeWarpImplementation implementation );
static void ksTimeWarp_CycleImplementation( ksTimeWarp * timeWarp );
static void ksTimeWarp_SetChromaticAberrationCorrection( ksTimeWarp * timeWarp, const bool set );
static void ksTimeWarp_ToggleChromaticAberrationCorrection( ksTimeWarp * timeWarp );
static void ksTimeWarp_SetMultiView( ksTimeWarp * timeWarp, const bool enabled );

static void ksTimeWarp_SetDisplayResolutionLevel( ksTimeWarp * timeWarp, const int level );
static void ksTimeWarp_SetEyeImageResolutionLevel( ksTimeWarp * timeWarp, const int level );
static void ksTimeWarp_SetEyeImageSamplesLevel( ksTimeWarp * timeWarp, const int level );

static void ksTimeWarp_SetDrawCallLevel( ksTimeWarp * timeWarp, const int level );
static void ksTimeWarp_SetTriangleLevel( ksTimeWarp * timeWarp, const int level );
static void ksTimeWarp_SetFragmentLevel( ksTimeWarp * timeWarp, const int level );

static ksNanoseconds ksTimeWarp_GetPredictedDisplayTime( ksTimeWarp * timeWarp, const int frameIndex );
static void ksTimeWarp_SubmitFrame( ksTimeWarp * timeWarp, const int frameIndex, const ksNanoseconds displayTime,
									const ksMatrix4x4f * viewMatrix, const Matrix4x4_t * projectionMatrix,
									ksGpuTexture * eyeTexture[NUM_EYES], ksGpuFence * eyeCompletionFence[NUM_EYES],
									int eyeArrayLayer[NUM_EYES], ksNanoseconds eyeTexturesCpuTime, ksNanoseconds eyeTexturesGpuTime );
static void ksTimeWarp_Render( ksTimeWarp * timeWarp );

================================================================================================================================
*/

#define AVERAGE_FRAME_RATE_FRAMES		20

typedef enum
{
	TIMEWARP_IMPLEMENTATION_GRAPHICS,
	TIMEWARP_IMPLEMENTATION_COMPUTE,
	TIMEWARP_IMPLEMENTATION_MAX
} ksTimeWarpImplementation;

typedef struct
{
	int							index;
	int							frameIndex;
	ksNanoseconds				displayTime;
	ksMatrix4x4f				viewMatrix;
	ksMatrix4x4f				projectionMatrix;
	ksGpuTexture *				texture[NUM_EYES];
	ksGpuFence *				completionFence[NUM_EYES];
	int							arrayLayer[NUM_EYES];
	ksNanoseconds				cpuTime;
	ksNanoseconds				gpuTime;
} ksEyeTextures;

typedef struct
{
	long long					frameIndex;
	ksNanoseconds				vsyncTime;
	ksNanoseconds				frameTime;
} ksFrameTiming;

typedef struct
{
	ksGpuWindow *				window;
	ksGpuTexture				defaultTexture;
	ksNanoseconds				displayTime;
	ksMatrix4x4f				viewMatrix;
	ksMatrix4x4f				projectionMatrix;
	ksGpuTexture *				eyeTexture[NUM_EYES];
	int							eyeArrayLayer[NUM_EYES];

	ksMutex						newEyeTexturesMutex;
	ksSignal					newEyeTexturesConsumed;
	ksEyeTextures				newEyeTextures;
	int							eyeTexturesPresentIndex;
	int							eyeTexturesConsumedIndex;

	ksFrameTiming				frameTiming;
	ksMutex						frameTimingMutex;
	ksSignal					vsyncSignal;

	float						refreshRate;
	ksNanoseconds				frameCpuTime[AVERAGE_FRAME_RATE_FRAMES];
	int							eyeTexturesFrames[AVERAGE_FRAME_RATE_FRAMES];
	int							timeWarpFrames;
	ksNanoseconds				cpuTimes[PROFILE_TIME_MAX];
	ksNanoseconds				gpuTimes[PROFILE_TIME_MAX];

	ksGpuRenderPass				renderPass;
	ksGpuFramebuffer			framebuffer;
	ksGpuCommandBuffer			commandBuffer;
	bool						correctChromaticAberration;
	ksTimeWarpImplementation	implementation;
	ksTimeWarpGraphics			graphics;
	ksTimeWarpCompute			compute;
	ksTimeWarpBarGraphs			bargraphs;
} ksTimeWarp;

static void ksTimeWarp_Create( ksTimeWarp * timeWarp, ksGpuWindow * window )
{
	timeWarp->window = window;

	ksGpuTexture_CreateDefault( &window->context, &timeWarp->defaultTexture, KS_GPU_TEXTURE_DEFAULT_CIRCLES, 1024, 1024, 0, 2, 1, false, true );
	ksGpuTexture_SetWrapMode( &window->context, &timeWarp->defaultTexture, KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER );

	ksMutex_Create( &timeWarp->newEyeTexturesMutex );
	ksSignal_Create( &timeWarp->newEyeTexturesConsumed, true );
	ksSignal_Raise( &timeWarp->newEyeTexturesConsumed );

	timeWarp->newEyeTextures.index = 0;
	timeWarp->newEyeTextures.displayTime = 0;
	ksMatrix4x4f_CreateIdentity( &timeWarp->newEyeTextures.viewMatrix );
	ksMatrix4x4f_CreateProjectionFov( &timeWarp->newEyeTextures.projectionMatrix, 40.0f, 40.0f, 40.0f, 40.0f, 0.1f, 0.0f );
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		timeWarp->newEyeTextures.texture[eye] = &timeWarp->defaultTexture;
		timeWarp->newEyeTextures.completionFence[eye] = NULL;
		timeWarp->newEyeTextures.arrayLayer[eye] = eye;
	}
	timeWarp->newEyeTextures.cpuTime = 0;
	timeWarp->newEyeTextures.gpuTime = 0;

	timeWarp->displayTime = 0;
	timeWarp->viewMatrix = timeWarp->newEyeTextures.viewMatrix;
	timeWarp->projectionMatrix = timeWarp->newEyeTextures.projectionMatrix;
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		timeWarp->eyeTexture[eye] = timeWarp->newEyeTextures.texture[eye];
		timeWarp->eyeArrayLayer[eye] = timeWarp->newEyeTextures.arrayLayer[eye];
	}

	timeWarp->eyeTexturesPresentIndex = 1;
	timeWarp->eyeTexturesConsumedIndex = 0;

	timeWarp->frameTiming.frameIndex = 0;
	timeWarp->frameTiming.vsyncTime = 0;
	timeWarp->frameTiming.frameTime = 0;
	ksMutex_Create( &timeWarp->frameTimingMutex );
	ksSignal_Create( &timeWarp->vsyncSignal, false );

	timeWarp->refreshRate = window->windowRefreshRate;
	for ( int i = 0; i < AVERAGE_FRAME_RATE_FRAMES; i++ )
	{
		timeWarp->frameCpuTime[i] = 0;
		timeWarp->eyeTexturesFrames[i] = 0;
	}
	timeWarp->timeWarpFrames = 0;

	ksGpuRenderPass_Create( &window->context, &timeWarp->renderPass, window->colorFormat, window->depthFormat,
							KS_GPU_SAMPLE_COUNT_1, KS_GPU_RENDERPASS_TYPE_INLINE,
							KS_GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER );
	ksGpuFramebuffer_CreateFromSwapchain( window, &timeWarp->framebuffer, &timeWarp->renderPass );
	ksGpuCommandBuffer_Create( &window->context, &timeWarp->commandBuffer, KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY, ksGpuFramebuffer_GetBufferCount( &timeWarp->framebuffer ) );

	timeWarp->correctChromaticAberration = false;
	timeWarp->implementation = TIMEWARP_IMPLEMENTATION_GRAPHICS;

	const ksHmdInfo * hmdInfo = GetDefaultHmdInfo( window->windowWidth, window->windowHeight );

	ksTimeWarpGraphics_Create( &window->context, &timeWarp->graphics, hmdInfo, &timeWarp->renderPass );
	ksTimeWarpCompute_Create( &window->context, &timeWarp->compute, hmdInfo, &timeWarp->renderPass );
	ksTimeWarpBarGraphs_Create( &window->context, &timeWarp->bargraphs, &timeWarp->renderPass );

	memset( timeWarp->cpuTimes, 0, sizeof( timeWarp->cpuTimes ) );
	memset( timeWarp->gpuTimes, 0, sizeof( timeWarp->gpuTimes ) );
}

static void ksTimeWarp_Destroy( ksTimeWarp * timeWarp, ksGpuWindow * window )
{
	ksGpuContext_WaitIdle( &window->context );

	ksTimeWarpGraphics_Destroy( &window->context, &timeWarp->graphics );
	ksTimeWarpCompute_Destroy( &window->context, &timeWarp->compute );
	ksTimeWarpBarGraphs_Destroy( &window->context, &timeWarp->bargraphs );

	ksGpuCommandBuffer_Destroy( &window->context, &timeWarp->commandBuffer );
	ksGpuFramebuffer_Destroy( &window->context, &timeWarp->framebuffer );
	ksGpuRenderPass_Destroy( &window->context, &timeWarp->renderPass );

	ksSignal_Destroy( &timeWarp->newEyeTexturesConsumed );
	ksMutex_Destroy( &timeWarp->newEyeTexturesMutex );
	ksMutex_Destroy( &timeWarp->frameTimingMutex );
	ksSignal_Destroy( &timeWarp->vsyncSignal );

	ksGpuTexture_Destroy( &window->context, &timeWarp->defaultTexture );
}

static void ksTimeWarp_SetBarGraphState( ksTimeWarp * timeWarp, const ksBarGraphState state )
{
	timeWarp->bargraphs.barGraphState = state;
}

static void ksTimeWarp_CycleBarGraphState( ksTimeWarp * timeWarp )
{
	timeWarp->bargraphs.barGraphState = (ksBarGraphState)( ( timeWarp->bargraphs.barGraphState + 1 ) % 3 );
}

static void ksTimeWarp_SetImplementation( ksTimeWarp * timeWarp, const ksTimeWarpImplementation implementation )
{
	timeWarp->implementation = implementation;
	const float delta = ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_GRAPHICS ) ? 0.0f : 1.0f;
	ksBarGraph_AddBar( &timeWarp->bargraphs.timeWarpImplementationBarGraph, 0, delta, &colorRed, false );
}

static void ksTimeWarp_CycleImplementation( ksTimeWarp * timeWarp )
{
	timeWarp->implementation = (ksTimeWarpImplementation)( ( timeWarp->implementation + 1 ) % TIMEWARP_IMPLEMENTATION_MAX );
	const float delta = ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_GRAPHICS ) ? 0.0f : 1.0f;
	ksBarGraph_AddBar( &timeWarp->bargraphs.timeWarpImplementationBarGraph, 0, delta, &colorRed, false );
}

static void ksTimeWarp_SetChromaticAberrationCorrection( ksTimeWarp * timeWarp, const bool set )
{
	timeWarp->correctChromaticAberration = set;
	ksBarGraph_AddBar( &timeWarp->bargraphs.correctChromaticAberrationBarGraph, 0, timeWarp->correctChromaticAberration ? 1.0f : 0.0f, &colorRed, false );
}

static void ksTimeWarp_ToggleChromaticAberrationCorrection( ksTimeWarp * timeWarp )
{
	timeWarp->correctChromaticAberration = !timeWarp->correctChromaticAberration;
	ksBarGraph_AddBar( &timeWarp->bargraphs.correctChromaticAberrationBarGraph, 0, timeWarp->correctChromaticAberration ? 1.0f : 0.0f, &colorRed, false );
}

static void ksTimeWarp_SetMultiView( ksTimeWarp * timeWarp, const bool enabled )
{
	ksBarGraph_AddBar( &timeWarp->bargraphs.multiViewBarGraph, 0, enabled ? 1.0f : 0.0f, &colorRed, false );
}

static void ksTimeWarp_SetDisplayResolutionLevel( ksTimeWarp * timeWarp, const int level )
{
	const ksVector4f * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		ksBarGraph_AddBar( &timeWarp->bargraphs.displayResolutionLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void ksTimeWarp_SetEyeImageResolutionLevel( ksTimeWarp * timeWarp, const int level )
{
	const ksVector4f * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		ksBarGraph_AddBar( &timeWarp->bargraphs.eyeImageResolutionLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void ksTimeWarp_SetEyeImageSamplesLevel( ksTimeWarp * timeWarp, const int level )
{
	const ksVector4f * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		ksBarGraph_AddBar( &timeWarp->bargraphs.eyeImageSamplesLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void ksTimeWarp_SetDrawCallLevel( ksTimeWarp * timeWarp, const int level )
{
	const ksVector4f * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		ksBarGraph_AddBar( &timeWarp->bargraphs.sceneDrawCallLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void ksTimeWarp_SetTriangleLevel( ksTimeWarp * timeWarp, const int level )
{
	const ksVector4f * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		ksBarGraph_AddBar( &timeWarp->bargraphs.sceneTriangleLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void ksTimeWarp_SetFragmentLevel( ksTimeWarp * timeWarp, const int level )
{
	const ksVector4f * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		ksBarGraph_AddBar( &timeWarp->bargraphs.sceneFragmentLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static ksNanoseconds ksTimeWarp_GetPredictedDisplayTime( ksTimeWarp * timeWarp, const int frameIndex )
{
	ksMutex_Lock( &timeWarp->frameTimingMutex, true );
	const ksFrameTiming frameTiming = timeWarp->frameTiming;
	ksMutex_Unlock( &timeWarp->frameTimingMutex );

	// The time warp thread is currently released by SwapBuffers shortly after a V-Sync.
	// Where possible, the time warp thread then waits until a short time before the next V-Sync,
	// giving it just enough time to warp the last completed application frame onto the display.
	// The time warp thread then tries to pick up the latest completed application frame and warps
	// the frame onto the display. The application thread is released right after the V-Sync
	// and can start working on a new frame that will be displayed effectively 2 display refresh
	// cycles in the future.

	return frameTiming.vsyncTime + ( frameIndex - frameTiming.frameIndex ) * frameTiming.frameTime;
}

static void ksTimeWarp_SubmitFrame( ksTimeWarp * timeWarp, const int frameIndex, const ksNanoseconds displayTime,
									const ksMatrix4x4f * viewMatrix, const ksMatrix4x4f * projectionMatrix,
									ksGpuTexture * eyeTexture[NUM_EYES], ksGpuFence * eyeCompletionFence[NUM_EYES],
									int eyeArrayLayer[NUM_EYES], ksNanoseconds eyeTexturesCpuTime, ksNanoseconds eyeTexturesGpuTime )
{
	ksEyeTextures newEyeTextures;
	newEyeTextures.index = timeWarp->eyeTexturesPresentIndex++;
	newEyeTextures.frameIndex = frameIndex;
	newEyeTextures.displayTime = displayTime;
	newEyeTextures.viewMatrix = *viewMatrix;
	newEyeTextures.projectionMatrix = *projectionMatrix;
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		newEyeTextures.texture[eye] = eyeTexture[eye];
		newEyeTextures.completionFence[eye] = eyeCompletionFence[eye];
		newEyeTextures.arrayLayer[eye] = eyeArrayLayer[eye];
	}
	newEyeTextures.cpuTime = eyeTexturesCpuTime;
	newEyeTextures.gpuTime = eyeTexturesGpuTime;

	// Wait for the previous eye textures to be consumed before overwriting them.
	ksSignal_Wait( &timeWarp->newEyeTexturesConsumed, SIGNAL_TIMEOUT_INFINITE );

	ksMutex_Lock( &timeWarp->newEyeTexturesMutex, true );
	timeWarp->newEyeTextures = newEyeTextures;
	ksMutex_Unlock( &timeWarp->newEyeTexturesMutex );

	// Wait for at least one V-Sync to pass to avoid piling up frames of latency.
	ksSignal_Wait( &timeWarp->vsyncSignal, SIGNAL_TIMEOUT_INFINITE );

	ksFrameTiming newFrameTiming;
	newFrameTiming.frameIndex = frameIndex;
	newFrameTiming.vsyncTime = ksGpuWindow_GetNextSwapTimeNanoseconds( timeWarp->window );
	newFrameTiming.frameTime = ksGpuWindow_GetFrameTimeNanoseconds( timeWarp->window );

	ksMutex_Lock( &timeWarp->frameTimingMutex, true );
	timeWarp->frameTiming = newFrameTiming;
	ksMutex_Unlock( &timeWarp->frameTimingMutex );
}

static void ksTimeWarp_Render( ksTimeWarp * timeWarp )
{
	const ksNanoseconds nextSwapTime = ksGpuWindow_GetNextSwapTimeNanoseconds( timeWarp->window );
	const ksNanoseconds frameTime = ksGpuWindow_GetFrameTimeNanoseconds( timeWarp->window );

	// Wait until close to the next V-Sync but still far enough away to allow the time warp to complete rendering.
	ksGpuWindow_DelayBeforeSwap( timeWarp->window, frameTime / 2 );

	timeWarp->eyeTexturesFrames[timeWarp->timeWarpFrames % AVERAGE_FRAME_RATE_FRAMES] = 0;

	// Try to pick up the latest eye textures but never block the time warp thread.
	// It is better to display an old set of eye textures than to miss the next V-Sync
	// in case another thread is suspended while holding on to the mutex.
	if ( ksMutex_Lock( &timeWarp->newEyeTexturesMutex, false ) )
	{
		ksEyeTextures newEyeTextures = timeWarp->newEyeTextures;
		ksMutex_Unlock( &timeWarp->newEyeTexturesMutex );

		// If this is a new set of eye textures.
		if ( newEyeTextures.index > timeWarp->eyeTexturesConsumedIndex &&
				// Never display the eye textures before they are meant to be displayed.
				newEyeTextures.displayTime < nextSwapTime + frameTime / 2 &&
					// Make sure both eye textures have completed rendering.
					ksGpuFence_IsSignalled( &timeWarp->window->context, newEyeTextures.completionFence[0] ) &&
						ksGpuFence_IsSignalled( &timeWarp->window->context, newEyeTextures.completionFence[1] ) )
		{
			assert( newEyeTextures.index == timeWarp->eyeTexturesConsumedIndex + 1 );
			timeWarp->eyeTexturesConsumedIndex = newEyeTextures.index;
			timeWarp->displayTime = newEyeTextures.displayTime;
			timeWarp->projectionMatrix = newEyeTextures.projectionMatrix;
			timeWarp->viewMatrix = newEyeTextures.viewMatrix;
			for ( int eye = 0; eye < NUM_EYES; eye++ )
			{
				timeWarp->eyeTexture[eye] = newEyeTextures.texture[eye];
				timeWarp->eyeArrayLayer[eye] = newEyeTextures.arrayLayer[eye];
			}
			timeWarp->cpuTimes[PROFILE_TIME_APPLICATION] = newEyeTextures.cpuTime;
			timeWarp->gpuTimes[PROFILE_TIME_APPLICATION] = newEyeTextures.gpuTime;
			timeWarp->eyeTexturesFrames[timeWarp->timeWarpFrames % AVERAGE_FRAME_RATE_FRAMES] = 1;
			ksSignal_Clear( &timeWarp->vsyncSignal );
			ksSignal_Raise( &timeWarp->newEyeTexturesConsumed );
		}
	}

	// Calculate the eye texture and time warp frame rates.
	float timeWarpFrameRate = timeWarp->refreshRate;
	float eyeTexturesFrameRate = timeWarp->refreshRate;
	{
		ksNanoseconds lastTime = timeWarp->frameCpuTime[timeWarp->timeWarpFrames % AVERAGE_FRAME_RATE_FRAMES];
		ksNanoseconds time = nextSwapTime;
		timeWarp->frameCpuTime[timeWarp->timeWarpFrames % AVERAGE_FRAME_RATE_FRAMES] = time;
		timeWarp->timeWarpFrames++;
		if ( timeWarp->timeWarpFrames > AVERAGE_FRAME_RATE_FRAMES )
		{
			int timeWarpFrames = AVERAGE_FRAME_RATE_FRAMES;
			int eyeTexturesFrames = 0;
			for ( int i = 0; i < AVERAGE_FRAME_RATE_FRAMES; i++ )
			{
				eyeTexturesFrames += timeWarp->eyeTexturesFrames[i];
			}

			timeWarpFrameRate = timeWarpFrames * 1e9f / ( time - lastTime );
			eyeTexturesFrameRate = eyeTexturesFrames * 1e9f / ( time - lastTime );
		}
	}

	// Update the bar graphs if not paused.
	if ( timeWarp->bargraphs.barGraphState == BAR_GRAPH_VISIBLE )
	{
		const ksVector4f * applicationFrameRateColor = ( eyeTexturesFrameRate > timeWarp->refreshRate - 0.5f ) ? &colorPurple : &colorRed;
		const ksVector4f * timeWarpFrameRateColor = ( timeWarpFrameRate > timeWarp->refreshRate - 0.5f ) ? &colorGreen : &colorRed;

		ksBarGraph_AddBar( &timeWarp->bargraphs.applicationFrameRateGraph, 0, eyeTexturesFrameRate / timeWarp->refreshRate, applicationFrameRateColor, true );
		ksBarGraph_AddBar( &timeWarp->bargraphs.timeWarpFrameRateGraph, 0, timeWarpFrameRate / timeWarp->refreshRate, timeWarpFrameRateColor, true );

		for ( int i = 0; i < 2; i++ )
		{
			const ksNanoseconds * times = ( i == 0 ) ? timeWarp->cpuTimes : timeWarp->gpuTimes;
			float barHeights[PROFILE_TIME_MAX];
			float totalBarHeight = 0.0f;
			for ( int p = 0; p < PROFILE_TIME_MAX; p++ )
			{
				barHeights[p] = times[p] * timeWarp->refreshRate * 1e-9f;
				totalBarHeight += barHeights[p];
			}

			const float limit = 0.9f;
			if ( totalBarHeight > limit )
			{
				totalBarHeight = 0.0f;
				for ( int p = 0; p < PROFILE_TIME_MAX; p++ )
				{
					barHeights[p] = ( totalBarHeight + barHeights[p] > limit ) ? ( limit - totalBarHeight ) : barHeights[p];
					totalBarHeight += barHeights[p];
				}
				barHeights[PROFILE_TIME_OVERFLOW] = 1.0f - limit;
			}

			ksBarGraph * barGraph = ( i == 0 ) ? &timeWarp->bargraphs.frameCpuTimeBarGraph : &timeWarp->bargraphs.frameGpuTimeBarGraph;
			for ( int p = 0; p < PROFILE_TIME_MAX; p++ )
			{
				ksBarGraph_AddBar( barGraph, p, barHeights[p], profileTimeBarColors[p], ( p == PROFILE_TIME_MAX - 1 ) );
			}
		}
	}

	ksFrameLog_BeginFrame();

	//assert( timeWarp->displayTime == nextSwapTime );
	const ksNanoseconds refreshStartTime = nextSwapTime;
	const ksNanoseconds refreshEndTime = refreshStartTime /* + display refresh time for an incremental display refresh */;

	if ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_GRAPHICS )
	{
		ksTimeWarpGraphics_Render( &timeWarp->commandBuffer, &timeWarp->graphics, &timeWarp->framebuffer, &timeWarp->renderPass,
								refreshStartTime, refreshEndTime,
								&timeWarp->projectionMatrix, &timeWarp->viewMatrix,
								timeWarp->eyeTexture, timeWarp->eyeArrayLayer, timeWarp->correctChromaticAberration,
								&timeWarp->bargraphs, timeWarp->cpuTimes, timeWarp->gpuTimes );
	}
	else if ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_COMPUTE )
	{
		ksTimeWarpCompute_Render( &timeWarp->commandBuffer, &timeWarp->compute, &timeWarp->framebuffer,
								refreshStartTime, refreshEndTime,
								&timeWarp->projectionMatrix, &timeWarp->viewMatrix,
								timeWarp->eyeTexture, timeWarp->eyeArrayLayer, timeWarp->correctChromaticAberration,
								&timeWarp->bargraphs, timeWarp->cpuTimes, timeWarp->gpuTimes );
	}

	ksFrameLog_EndFrame(	timeWarp->cpuTimes[PROFILE_TIME_TIME_WARP] +
							timeWarp->cpuTimes[PROFILE_TIME_BAR_GRAPHS] +
							timeWarp->cpuTimes[PROFILE_TIME_BLIT],
							timeWarp->gpuTimes[PROFILE_TIME_TIME_WARP] +
							timeWarp->gpuTimes[PROFILE_TIME_BAR_GRAPHS] +
							timeWarp->gpuTimes[PROFILE_TIME_BLIT], KS_GPU_TIMER_FRAMES_DELAYED );

	ksGpuWindow_SwapBuffers( timeWarp->window );

	ksSignal_Raise( &timeWarp->vsyncSignal );
}

#include "scenes/scene_settings.h"
#include "scenes/scene_view_state.h"
#include "scenes/scene_perf.h"
#include "scenes/scene_gltf.h"

/*
================================================================================================================================

Info

================================================================================================================================
*/

static void PrintInfo( const ksGpuWindow * window, const int eyeImageResolutionLevel, const int eyeImageSamplesLevel )
{
	const int resolution = ( eyeImageResolutionLevel >= 0 ) ? eyeResolutionTable[eyeImageResolutionLevel] : 0;
	const int samples = ( eyeImageSamplesLevel >= 0 ) ? eyeSampleCountTable[eyeImageSamplesLevel] : 0;
	char resolutionString[32];
	sprintf( resolutionString, "%4d x %4d - %dx MSAA", resolution, resolution, samples );

	const uint32_t version = window->context.device->physicalDeviceProperties.apiVersion;
	const uint32_t major = VK_VERSION_MAJOR( version );
	const uint32_t minor = VK_VERSION_MINOR( version );
	const uint32_t patch = VK_VERSION_PATCH( version );

	Print( "--------------------------------\n" );
	Print( "OS      : %s\n", GetOSVersion() );
	Print( "CPU     : %s\n", GetCPUVersion() );
	Print( "GPU     : %s\n", window->context.device->physicalDeviceProperties.deviceName );
	Print( "Vulkan  : %d.%d.%d\n", major, minor, patch );
	Print( "Display : %4d x %4d - %1.0f Hz (%s)\n", window->windowWidth, window->windowHeight, window->windowRefreshRate,
												window->windowFullscreen ? "fullscreen" : "windowed" );
	Print( "Eye Img : %s\n", ( resolution >= 0 ) ? resolutionString : "-" );
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
									"glslangValidator -V -x -o %sSPIRV.h %sGLSL.%s\r\n",
									glsl[i].fileName, glsl[i].fileName, glsl[i].extension );
	}

	WriteTextFile( "glsl/spirv_bin.bat", batchFileBin );
	WriteTextFile( "glsl/spirv_hex.bat", batchFileHex );
}

/*
================================================================================================================================

Startup settings.

ksStartupSettings

static int ksStartupSettings_StringToLevel( const char * string, const int maxLevels );
static int ksStartupSettings_StringToRenderMode( const char * string );
static int ksStartupSettings_StringToTimeWarpImplementation( const char * string );

================================================================================================================================
*/

typedef enum
{
	RENDER_MODE_ASYNC_TIME_WARP,
	RENDER_MODE_TIME_WARP,
	RENDER_MODE_SCENE,
	RENDER_MODE_MAX
} ksRenderMode;

typedef struct
{
	const char *				glTF;
	bool						fullscreen;
	bool						simulationPaused;
	bool						headRotationDisabled;
	int							displayResolutionLevel;
	int							eyeImageResolutionLevel;
	int							eyeImageSamplesLevel;
	int							drawCallLevel;
	int							triangleLevel;
	int							fragmentLevel;
	bool						useMultiView;
	bool						correctChromaticAberration;
	bool						hideGraphs;
	ksTimeWarpImplementation	timeWarpImplementation;
	ksRenderMode				renderMode;
	ksNanoseconds				startupTimeNanoseconds;
	ksNanoseconds				noVSyncNanoseconds;
	ksNanoseconds				noLogNanoseconds;
} ksStartupSettings;

static int ksStartupSettings_StringToLevel( const char * string, const int maxLevels )
{
	const int level = atoi( string );
	return ( level >= 0 ) ? ( ( level < maxLevels ) ? level : maxLevels - 1 ) : 0;
}

static int ksStartupSettings_StringToRenderMode( const char * string )
{
	return	( ( strcmp( string, "atw" ) == 0 ) ? RENDER_MODE_ASYNC_TIME_WARP:
			( ( strcmp( string, "tw"  ) == 0 ) ? RENDER_MODE_TIME_WARP :
			RENDER_MODE_SCENE ) );
}

static int ksStartupSettings_StringToTimeWarpImplementation( const char * string )
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
#define WINDOW_RESOLUTION( x, fullscreen )	(x)		// always fullscreen
#else
#define WINDOW_RESOLUTION( x, fullscreen )	( (fullscreen) ? (x) : ROUNDUP( x / 2, 8 ) )
#endif

typedef struct
{
	ksSignal				initialized;
	ksGpuContext *			shareContext;
	ksTimeWarp *			timeWarp;
	ksSceneSettings *		sceneSettings;
	ksGpuWindowInput *		input;

	volatile bool			terminate;
	volatile bool			openFrameLog;
} ksSceneThreadData;

void SceneThread_Render( ksSceneThreadData * threadData )
{
	ksThread_SetAffinity( THREAD_AFFINITY_BIG_CORES );

	ksGpuContext context;
	ksGpuContext_CreateShared( &context, threadData->shareContext, QUEUE_INDEX_SCENE );

	const int resolution = eyeResolutionTable[threadData->sceneSettings->eyeImageResolutionLevel];

	const ksGpuSampleCount sampleCount = eyeSampleCountTable[threadData->sceneSettings->eyeImageSamplesLevel];

	ksGpuRenderPass renderPassSingleView;
	ksGpuRenderPass_Create( &context, &renderPassSingleView, KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, KS_GPU_SURFACE_DEPTH_FORMAT_D24,
							sampleCount, KS_GPU_RENDERPASS_TYPE_INLINE,
							KS_GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER |
							KS_GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER );

	ksGpuRenderPass renderPassMultiView;
	ksGpuRenderPass_Create( &context, &renderPassMultiView, KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, KS_GPU_SURFACE_DEPTH_FORMAT_D24,
							sampleCount, KS_GPU_RENDERPASS_TYPE_SECONDARY_COMMAND_BUFFERS,
							KS_GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER |
							KS_GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER );

	ksGpuFramebuffer framebuffer;
	ksGpuFramebuffer_CreateFromTextureArrays( &context, &framebuffer, &renderPassSingleView,
				resolution, resolution, NUM_EYES, NUM_EYE_BUFFERS, false );

	ksGpuCommandBuffer eyeCommandBuffer[NUM_EYES];
	ksGpuTimer eyeTimer[NUM_EYES];
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		ksGpuCommandBuffer_Create( &context, &eyeCommandBuffer[eye], KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY, NUM_EYE_BUFFERS );
		ksGpuTimer_Create( &context, &eyeTimer[eye] );
	}

	ksGpuCommandBuffer sceneCommandBuffer;
	ksGpuCommandBuffer_Create( &context, &sceneCommandBuffer, KS_GPU_COMMAND_BUFFER_TYPE_SECONDARY_CONTINUE_RENDER_PASS, NUM_EYE_BUFFERS );

	const ksBodyInfo * bodyInfo = GetDefaultBodyInfo();

	ksViewState viewState;
	ksViewState_Init( &viewState, bodyInfo->interpupillaryDistance );

	ksPerfScene perfScene;
	ksGltfScene gltfScene;

	if ( threadData->sceneSettings->glTF == NULL )
	{
		ksPerfScene_Create( &context, &perfScene, threadData->sceneSettings, &renderPassSingleView );
	}
	else
	{
		ksGltfScene_CreateFromFile( &context, &gltfScene, threadData->sceneSettings, &renderPassSingleView );
	}

	ksSignal_Raise( &threadData->initialized );

	for ( int frameIndex = 0; !threadData->terminate; frameIndex++ )
	{
		if ( threadData->openFrameLog )
		{
			threadData->openFrameLog = false;
			ksFrameLog_Open( OUTPUT_PATH "framelog_scene.txt", 10 );
		}

		const ksNanoseconds nextDisplayTime = ksTimeWarp_GetPredictedDisplayTime( threadData->timeWarp, frameIndex );

		if ( threadData->sceneSettings->glTF == NULL )
		{
			ksPerfScene_Simulate( &perfScene, &viewState, nextDisplayTime );
		}
		else
		{
			ksGltfScene_Simulate( &gltfScene, &viewState, threadData->input, nextDisplayTime );
		}

		ksFrameLog_BeginFrame();

		const ksNanoseconds t0 = GetTimeNanoseconds();

		if ( threadData->sceneSettings->useMultiView )
		{
			const ksScreenRect sceneRect = { 0, 0, resolution, resolution };
			ksGpuCommandBuffer_BeginSecondary( &sceneCommandBuffer, &renderPassMultiView, NULL );

			ksGpuCommandBuffer_SetViewport( &sceneCommandBuffer, &sceneRect );
			ksGpuCommandBuffer_SetScissor( &sceneCommandBuffer, &sceneRect );

			if ( threadData->sceneSettings->glTF == NULL )
			{
				ksPerfScene_Render( &sceneCommandBuffer, &perfScene, &viewState );
			}
			else
			{
				ksGltfScene_Render( &sceneCommandBuffer, &gltfScene, &viewState );
			}

			ksGpuCommandBuffer_EndSecondary( &sceneCommandBuffer );
		}

		ksGpuTexture * eyeTexture[NUM_EYES] = { 0 };
		ksGpuFence * eyeCompletionFence[NUM_EYES] = { 0 };
		int eyeArrayLayer[NUM_EYES] = { 0, 1 };

		for ( int eye = 0; eye < NUM_EYES; eye++ )
		{
			const ksScreenRect screenRect = ksGpuFramebuffer_GetRect( &framebuffer );

			ksGpuCommandBuffer_BeginPrimary( &eyeCommandBuffer[eye] );
			ksGpuCommandBuffer_BeginFramebuffer( &eyeCommandBuffer[eye], &framebuffer, eye, KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );

			if ( threadData->sceneSettings->glTF == NULL )
			{
				ksPerfScene_UpdateBuffers( &eyeCommandBuffer[eye], &perfScene, &viewState, eye );
			}
			else
			{
				ksGltfScene_UpdateBuffers( &eyeCommandBuffer[eye], &gltfScene, &viewState, eye );
			}

			ksGpuRenderPass * renderPass = threadData->sceneSettings->useMultiView ? &renderPassMultiView : &renderPassSingleView;

			ksGpuCommandBuffer_BeginTimer( &eyeCommandBuffer[eye], &eyeTimer[eye] );
			ksGpuCommandBuffer_BeginRenderPass( &eyeCommandBuffer[eye], renderPass, &framebuffer, &screenRect );

			if ( threadData->sceneSettings->useMultiView )
			{
				ksGpuCommandBuffer_SubmitSecondary( &sceneCommandBuffer, &eyeCommandBuffer[eye] );
			}
			else
			{
				ksGpuCommandBuffer_SetViewport( &eyeCommandBuffer[eye], &screenRect );
				ksGpuCommandBuffer_SetScissor( &eyeCommandBuffer[eye], &screenRect );
				if ( threadData->sceneSettings->glTF == NULL )
				{
					ksPerfScene_Render( &eyeCommandBuffer[eye], &perfScene, &viewState );
				}
				else
				{
					ksGltfScene_Render( &eyeCommandBuffer[eye], &gltfScene, &viewState );
				}
			}

			ksGpuCommandBuffer_EndRenderPass( &eyeCommandBuffer[eye], renderPass );
			ksGpuCommandBuffer_EndTimer( &eyeCommandBuffer[eye], &eyeTimer[eye] );

			ksGpuCommandBuffer_EndFramebuffer( &eyeCommandBuffer[eye], &framebuffer, eye, KS_GPU_TEXTURE_USAGE_SAMPLED );
			ksGpuCommandBuffer_EndPrimary( &eyeCommandBuffer[eye] );

			eyeTexture[eye] = ksGpuFramebuffer_GetColorTexture( &framebuffer );
			eyeCompletionFence[eye] = ksGpuCommandBuffer_SubmitPrimary( &eyeCommandBuffer[eye] );
		}

		const ksNanoseconds t1 = GetTimeNanoseconds();

		const ksNanoseconds eyeTexturesCpuTime = t1 - t0;
		const ksNanoseconds eyeTexturesGpuTime = ksGpuTimer_GetNanoseconds( &eyeTimer[0] ) + ksGpuTimer_GetNanoseconds( &eyeTimer[1] );

		ksFrameLog_EndFrame( eyeTexturesCpuTime, eyeTexturesGpuTime, KS_GPU_TIMER_FRAMES_DELAYED );

		ksMatrix4x4f projectionMatrix;
		ksMatrix4x4f_CreateProjectionFov( &projectionMatrix, 40.0f, 40.0f, 40.0f, 40.0f, DEFAULT_NEAR_Z, INFINITE_FAR_Z );

		ksTimeWarp_SubmitFrame( threadData->timeWarp, frameIndex, nextDisplayTime,
								&viewState.displayViewMatrix, &projectionMatrix,
								eyeTexture, eyeCompletionFence, eyeArrayLayer,
								eyeTexturesCpuTime, eyeTexturesGpuTime );
	}

	if ( threadData->sceneSettings->glTF == NULL )
	{
		ksPerfScene_Destroy( &context, &perfScene );
	}
	else
	{
		ksGltfScene_Destroy( &context, &gltfScene );
	}

	ksGpuCommandBuffer_Destroy( &context, &sceneCommandBuffer );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		ksGpuTimer_Destroy( &context, &eyeTimer[eye] );
		ksGpuCommandBuffer_Destroy( &context, &eyeCommandBuffer[eye] );
	}

	ksGpuFramebuffer_Destroy( &context, &framebuffer );
	ksGpuRenderPass_Destroy( &context, &renderPassMultiView );
	ksGpuRenderPass_Destroy( &context, &renderPassSingleView );
	ksGpuContext_Destroy( &context );
}

void SceneThread_Create( ksThread * sceneThread, ksSceneThreadData * sceneThreadData,
							ksGpuWindow * window, ksTimeWarp * timeWarp, ksSceneSettings * sceneSettings )
{
	ksSignal_Create( &sceneThreadData->initialized, true );
	sceneThreadData->shareContext = &window->context;
	sceneThreadData->timeWarp = timeWarp;
	sceneThreadData->sceneSettings = sceneSettings;
	sceneThreadData->input = &window->input;
	sceneThreadData->terminate = false;
	sceneThreadData->openFrameLog = false;

	ksThread_Create( sceneThread, "atw:scene", (ksThreadFunction) SceneThread_Render, sceneThreadData );
	ksThread_Signal( sceneThread );
	ksSignal_Wait( &sceneThreadData->initialized, SIGNAL_TIMEOUT_INFINITE );
}

void SceneThread_Destroy( ksThread * sceneThread, ksSceneThreadData * sceneThreadData )
{
	sceneThreadData->terminate = true;
	// The following assumes the time warp thread is blocked when this function is called.
	ksSignal_Raise( &sceneThreadData->timeWarp->newEyeTexturesConsumed );
	ksSignal_Raise( &sceneThreadData->timeWarp->vsyncSignal );
	ksSignal_Destroy( &sceneThreadData->initialized );
	ksThread_Destroy( sceneThread );
}

bool RenderAsyncTimeWarp( ksStartupSettings * startupSettings )
{
	ksThread_SetAffinity( THREAD_AFFINITY_BIG_CORES );
	ksThread_SetRealTimePriority( 1 );

	ksDriverInstance instance;
	ksDriverInstance_Create( &instance );

	const ksGpuQueueInfo queueInfo =
	{
		2,
		KS_GPU_QUEUE_PROPERTY_GRAPHICS | KS_GPU_QUEUE_PROPERTY_COMPUTE,
		{ KS_GPU_QUEUE_PRIORITY_HIGH, KS_GPU_QUEUE_PRIORITY_MEDIUM }
	};

	ksGpuWindow window;
	ksGpuWindow_Create( &window, &instance, &queueInfo, QUEUE_INDEX_TIMEWARP,
						KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, KS_GPU_SURFACE_DEPTH_FORMAT_NONE, KS_GPU_SAMPLE_COUNT_1,
						WINDOW_RESOLUTION( displayResolutionTable[startupSettings->displayResolutionLevel * 2 + 0], startupSettings->fullscreen ),
						WINDOW_RESOLUTION( displayResolutionTable[startupSettings->displayResolutionLevel * 2 + 1], startupSettings->fullscreen ),
						startupSettings->fullscreen );

	int swapInterval = ( startupSettings->noVSyncNanoseconds <= 0 );
	ksGpuWindow_SwapInterval( &window, swapInterval );

	ksTimeWarp timeWarp;
	ksTimeWarp_Create( &timeWarp, &window );
	ksTimeWarp_SetBarGraphState( &timeWarp, startupSettings->hideGraphs ? BAR_GRAPH_HIDDEN : BAR_GRAPH_VISIBLE );
	ksTimeWarp_SetImplementation( &timeWarp, startupSettings->timeWarpImplementation );
	ksTimeWarp_SetChromaticAberrationCorrection( &timeWarp, startupSettings->correctChromaticAberration );
	ksTimeWarp_SetMultiView( &timeWarp, startupSettings->useMultiView );
	ksTimeWarp_SetDisplayResolutionLevel( &timeWarp, startupSettings->displayResolutionLevel );
	ksTimeWarp_SetEyeImageResolutionLevel( &timeWarp, startupSettings->eyeImageResolutionLevel );
	ksTimeWarp_SetEyeImageSamplesLevel( &timeWarp, startupSettings->eyeImageSamplesLevel );
	ksTimeWarp_SetDrawCallLevel( &timeWarp, startupSettings->drawCallLevel );
	ksTimeWarp_SetTriangleLevel( &timeWarp, startupSettings->triangleLevel );
	ksTimeWarp_SetFragmentLevel( &timeWarp, startupSettings->fragmentLevel );

	ksSceneSettings sceneSettings;
	ksSceneSettings_Init( &window.context, &sceneSettings );
	ksSceneSettings_SetGltf( &sceneSettings, startupSettings->glTF );
	ksSceneSettings_SetSimulationPaused( &sceneSettings, startupSettings->simulationPaused );
	ksSceneSettings_SetMultiView( &sceneSettings, startupSettings->useMultiView );
	ksSceneSettings_SetDisplayResolutionLevel( &sceneSettings, startupSettings->displayResolutionLevel );
	ksSceneSettings_SetEyeImageResolutionLevel( &sceneSettings, startupSettings->eyeImageResolutionLevel );
	ksSceneSettings_SetEyeImageSamplesLevel( &sceneSettings, startupSettings->eyeImageSamplesLevel );
	ksSceneSettings_SetDrawCallLevel( &sceneSettings, startupSettings->drawCallLevel );
	ksSceneSettings_SetTriangleLevel( &sceneSettings, startupSettings->triangleLevel );
	ksSceneSettings_SetFragmentLevel( &sceneSettings, startupSettings->fragmentLevel );

	ksThread sceneThread;
	ksSceneThreadData sceneThreadData;
	SceneThread_Create( &sceneThread, &sceneThreadData, &window, &timeWarp, &sceneSettings );

	hmd_headRotationDisabled = startupSettings->headRotationDisabled;

	ksNanoseconds startupTimeNanoseconds = startupSettings->startupTimeNanoseconds;
	ksNanoseconds noVSyncNanoseconds = startupSettings->noVSyncNanoseconds;
	ksNanoseconds noLogNanoseconds = startupSettings->noLogNanoseconds;

	ksThread_SetName( "atw:timewarp" );

	bool exit = false;
	while ( !exit )
	{
		const ksNanoseconds time = GetTimeNanoseconds();

		const ksGpuWindowEvent handleEvent = ksGpuWindow_ProcessEvents( &window );
		if ( handleEvent == KS_GPU_WINDOW_EVENT_ACTIVATED )
		{
			PrintInfo( &window, sceneSettings.eyeImageResolutionLevel, startupSettings->eyeImageSamplesLevel );
		}
		else if ( handleEvent == KS_GPU_WINDOW_EVENT_EXIT )
		{
			exit = true;
			break;
		}

		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_ESCAPE ) )
		{
			ksGpuWindow_Exit( &window );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_Z ) )
		{
			startupSettings->renderMode = (ksRenderMode) ( ( startupSettings->renderMode + 1 ) % RENDER_MODE_MAX );
			break;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_F ) )
		{
			startupSettings->fullscreen = !startupSettings->fullscreen;
			break;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_V ) ||
			( noVSyncNanoseconds > 0 && time - startupTimeNanoseconds > noVSyncNanoseconds ) )
		{
			swapInterval = !swapInterval;
			ksGpuWindow_SwapInterval( &window, swapInterval );
			noVSyncNanoseconds = 0;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_L ) ||
			( noLogNanoseconds > 0 && time - startupTimeNanoseconds > noLogNanoseconds ) )
		{
			ksFrameLog_Open( OUTPUT_PATH "framelog_timewarp.txt", 10 );
			sceneThreadData.openFrameLog = true;
			noLogNanoseconds = 0;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_H ) )
		{
			hmd_headRotationDisabled = !hmd_headRotationDisabled;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_P ) )
		{
			ksSceneSettings_ToggleSimulationPaused( &sceneSettings );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_G ) )
		{
			ksTimeWarp_CycleBarGraphState( &timeWarp );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_R ) )
		{
			ksSceneSettings_CycleDisplayResolutionLevel( &sceneSettings );
			startupSettings->displayResolutionLevel = sceneSettings.displayResolutionLevel;
			break;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_B ) )
		{
			ksSceneSettings_CycleEyeImageResolutionLevel( &sceneSettings );
			startupSettings->eyeImageResolutionLevel = sceneSettings.eyeImageResolutionLevel;
			break;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_S ) )
		{
			ksSceneSettings_CycleEyeImageSamplesLevel( &sceneSettings );
			startupSettings->eyeImageSamplesLevel = sceneSettings.eyeImageSamplesLevel;
			break;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_Q ) )
		{
			ksSceneSettings_CycleDrawCallLevel( &sceneSettings );
			ksTimeWarp_SetDrawCallLevel( &timeWarp, ksSceneSettings_GetDrawCallLevel( &sceneSettings ) );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_W ) )
		{
			ksSceneSettings_CycleTriangleLevel( &sceneSettings );
			ksTimeWarp_SetTriangleLevel( &timeWarp, ksSceneSettings_GetTriangleLevel( &sceneSettings ) );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_E ) )
		{
			ksSceneSettings_CycleFragmentLevel( &sceneSettings );
			ksTimeWarp_SetFragmentLevel( &timeWarp, ksSceneSettings_GetFragmentLevel( &sceneSettings ) );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_I ) )
		{
			ksTimeWarp_CycleImplementation( &timeWarp );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_C ) )
		{
			ksTimeWarp_ToggleChromaticAberrationCorrection( &timeWarp );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_M ) )
		{
			ksSceneSettings_ToggleMultiView( &sceneSettings );
			ksTimeWarp_SetMultiView( &timeWarp, ksSceneSettings_GetMultiView( &sceneSettings ) );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_D ) )
		{
			DumpGLSL();
		}

		if ( window.windowActive )
		{
			ksTimeWarp_Render( &timeWarp );
		}
	}

	ksGpuContext_WaitIdle( &window.context );
	SceneThread_Destroy( &sceneThread, &sceneThreadData );
	ksTimeWarp_Destroy( &timeWarp, &window );
	ksGpuWindow_Destroy( &window );
	ksDriverInstance_Destroy( &instance );

	return exit;
}

/*
================================================================================================================================

Time warp rendering test.

================================================================================================================================
*/

bool RenderTimeWarp( ksStartupSettings * startupSettings )
{
	ksThread_SetAffinity( THREAD_AFFINITY_BIG_CORES );

	ksDriverInstance instance;
	ksDriverInstance_Create( &instance );

	const ksGpuQueueInfo queueInfo =
	{
		1,
		KS_GPU_QUEUE_PROPERTY_GRAPHICS | KS_GPU_QUEUE_PROPERTY_COMPUTE,
		{ KS_GPU_QUEUE_PRIORITY_MEDIUM }
	};

	ksGpuWindow window;
	ksGpuWindow_Create( &window, &instance, &queueInfo, 0,
						KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, KS_GPU_SURFACE_DEPTH_FORMAT_NONE, KS_GPU_SAMPLE_COUNT_1,
						WINDOW_RESOLUTION( displayResolutionTable[startupSettings->displayResolutionLevel * 2 + 0], startupSettings->fullscreen ),
						WINDOW_RESOLUTION( displayResolutionTable[startupSettings->displayResolutionLevel * 2 + 1], startupSettings->fullscreen ),
						startupSettings->fullscreen );

	int swapInterval = ( startupSettings->noVSyncNanoseconds <= 0 );
	ksGpuWindow_SwapInterval( &window, swapInterval );

	ksTimeWarp timeWarp;
	ksTimeWarp_Create( &timeWarp, &window );
	ksTimeWarp_SetBarGraphState( &timeWarp, startupSettings->hideGraphs ? BAR_GRAPH_HIDDEN : BAR_GRAPH_VISIBLE );
	ksTimeWarp_SetImplementation( &timeWarp, startupSettings->timeWarpImplementation );
	ksTimeWarp_SetChromaticAberrationCorrection( &timeWarp, startupSettings->correctChromaticAberration );
	ksTimeWarp_SetDisplayResolutionLevel( &timeWarp, startupSettings->displayResolutionLevel );

	hmd_headRotationDisabled = startupSettings->headRotationDisabled;

	ksNanoseconds startupTimeNanoseconds = startupSettings->startupTimeNanoseconds;
	ksNanoseconds noVSyncNanoseconds = startupSettings->noVSyncNanoseconds;
	ksNanoseconds noLogNanoseconds = startupSettings->noLogNanoseconds;

	ksThread_SetName( "atw:timewarp" );

	bool exit = false;
	while ( !exit )
	{
		const ksNanoseconds time = GetTimeNanoseconds();

		const ksGpuWindowEvent handleEvent = ksGpuWindow_ProcessEvents( &window );
		if ( handleEvent == KS_GPU_WINDOW_EVENT_ACTIVATED )
		{
			PrintInfo( &window, 0, 0 );
		}
		else if ( handleEvent == KS_GPU_WINDOW_EVENT_EXIT )
		{
			exit = true;
		}

		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_ESCAPE ) )
		{
			ksGpuWindow_Exit( &window );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_Z ) )
		{
			startupSettings->renderMode = (ksRenderMode) ( ( startupSettings->renderMode + 1 ) % RENDER_MODE_MAX );
			break;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_F ) )
		{
			startupSettings->fullscreen = !startupSettings->fullscreen;
			break;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_V ) ||
			( noVSyncNanoseconds > 0 && time - startupTimeNanoseconds > noVSyncNanoseconds ) )
		{
			swapInterval = !swapInterval;
			ksGpuWindow_SwapInterval( &window, swapInterval );
			noVSyncNanoseconds = 0;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_L ) ||
			( noLogNanoseconds > 0 && time - startupTimeNanoseconds > noLogNanoseconds ) )
		{
			ksFrameLog_Open( OUTPUT_PATH "framelog_timewarp.txt", 10 );
			noLogNanoseconds = 0;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_H ) )
		{
			hmd_headRotationDisabled = !hmd_headRotationDisabled;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_G ) )
		{
			ksTimeWarp_CycleBarGraphState( &timeWarp );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_I ) )
		{
			ksTimeWarp_CycleImplementation( &timeWarp );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_C ) )
		{
			ksTimeWarp_ToggleChromaticAberrationCorrection( &timeWarp );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_D ) )
		{
			DumpGLSL();
		}

		if ( window.windowActive )
		{
			ksTimeWarp_Render( &timeWarp );
		}
	}

	ksGpuContext_WaitIdle( &window.context );
	ksTimeWarp_Destroy( &timeWarp, &window );
	ksGpuWindow_Destroy( &window );
	ksDriverInstance_Destroy( &instance );

	return exit;
}

/*
================================================================================================================================

Scene rendering test.

================================================================================================================================
*/

bool RenderScene( ksStartupSettings * startupSettings )
{
	ksThread_SetAffinity( THREAD_AFFINITY_BIG_CORES );

	ksDriverInstance instance;
	ksDriverInstance_Create( &instance );

	const ksGpuSampleCount sampleCountTable[] =
	{
		KS_GPU_SAMPLE_COUNT_1,
		KS_GPU_SAMPLE_COUNT_2,
		KS_GPU_SAMPLE_COUNT_4,
		KS_GPU_SAMPLE_COUNT_8
	};
	const ksGpuSampleCount sampleCount = sampleCountTable[startupSettings->eyeImageSamplesLevel];

	const ksGpuQueueInfo queueInfo =
	{
		1,
		KS_GPU_QUEUE_PROPERTY_GRAPHICS,
		{ KS_GPU_QUEUE_PRIORITY_MEDIUM }
	};

	ksGpuWindow window;
	ksGpuWindow_Create( &window, &instance, &queueInfo, 0,
						KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, KS_GPU_SURFACE_DEPTH_FORMAT_D24, sampleCount,
						WINDOW_RESOLUTION( displayResolutionTable[startupSettings->displayResolutionLevel * 2 + 0], startupSettings->fullscreen ),
						WINDOW_RESOLUTION( displayResolutionTable[startupSettings->displayResolutionLevel * 2 + 1], startupSettings->fullscreen ),
						startupSettings->fullscreen );

	int swapInterval = ( startupSettings->noVSyncNanoseconds <= 0 );
	ksGpuWindow_SwapInterval( &window, swapInterval );

	ksGpuRenderPass renderPass;
	ksGpuRenderPass_Create( &window.context, &renderPass, window.colorFormat, window.depthFormat,
							sampleCount, KS_GPU_RENDERPASS_TYPE_INLINE,
							KS_GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER |
							KS_GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER );

	ksGpuFramebuffer framebuffer;
	ksGpuFramebuffer_CreateFromSwapchain( &window, &framebuffer, &renderPass );

	ksGpuCommandBuffer commandBuffer;
	ksGpuCommandBuffer_Create( &window.context, &commandBuffer, KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY, ksGpuFramebuffer_GetBufferCount( &framebuffer ) );

	ksGpuTimer timer;
	ksGpuTimer_Create( &window.context, &timer );

	ksBarGraph frameCpuTimeBarGraph;
	ksBarGraph_CreateVirtualRect( &window.context, &frameCpuTimeBarGraph, &renderPass, &frameCpuTimeBarGraphRect, 64, 1, &colorDarkGrey );

	ksBarGraph frameGpuTimeBarGraph;
	ksBarGraph_CreateVirtualRect( &window.context, &frameGpuTimeBarGraph, &renderPass, &frameGpuTimeBarGraphRect, 64, 1, &colorDarkGrey );

	ksSceneSettings sceneSettings;
	ksSceneSettings_Init( &window.context, &sceneSettings );
	ksSceneSettings_SetSimulationPaused( &sceneSettings, startupSettings->simulationPaused );
	ksSceneSettings_SetDisplayResolutionLevel( &sceneSettings, startupSettings->displayResolutionLevel );
	ksSceneSettings_SetEyeImageResolutionLevel( &sceneSettings, startupSettings->eyeImageResolutionLevel );
	ksSceneSettings_SetEyeImageSamplesLevel( &sceneSettings, startupSettings->eyeImageSamplesLevel );
	ksSceneSettings_SetDrawCallLevel( &sceneSettings, startupSettings->drawCallLevel );
	ksSceneSettings_SetTriangleLevel( &sceneSettings, startupSettings->triangleLevel );
	ksSceneSettings_SetFragmentLevel( &sceneSettings, startupSettings->fragmentLevel );

	ksViewState viewState;
	ksViewState_Init( &viewState, 0.0f );

	ksPerfScene perfScene;
	ksGltfScene gltfScene;

	if ( startupSettings->glTF == NULL )
	{
		ksPerfScene_Create( &window.context, &perfScene, &sceneSettings, &renderPass );
	}
	else
	{
		ksGltfScene_CreateFromFile( &window.context, &gltfScene, &sceneSettings, &renderPass );
	}

	hmd_headRotationDisabled = startupSettings->headRotationDisabled;

	ksNanoseconds startupTimeNanoseconds = startupSettings->startupTimeNanoseconds;
	ksNanoseconds noVSyncNanoseconds = startupSettings->noVSyncNanoseconds;
	ksNanoseconds noLogNanoseconds = startupSettings->noLogNanoseconds;

	ksThread_SetName( "atw:scene" );

	bool exit = false;
	while ( !exit )
	{
		const ksNanoseconds time = GetTimeNanoseconds();

		const ksGpuWindowEvent handleEvent = ksGpuWindow_ProcessEvents( &window );
		if ( handleEvent == KS_GPU_WINDOW_EVENT_ACTIVATED )
		{
			PrintInfo( &window, -1, -1 );
		}
		else if ( handleEvent == KS_GPU_WINDOW_EVENT_EXIT )
		{
			exit = true;
			break;
		}

		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_ESCAPE ) )
		{
			ksGpuWindow_Exit( &window );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_Z ) )
		{
			startupSettings->renderMode = (ksRenderMode) ( ( startupSettings->renderMode + 1 ) % RENDER_MODE_MAX );
			break;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_F ) )
		{
			startupSettings->fullscreen = !startupSettings->fullscreen;
			break;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_V ) ||
			( noVSyncNanoseconds > 0 && time - startupTimeNanoseconds > noVSyncNanoseconds ) )
		{
			swapInterval = !swapInterval;
			ksGpuWindow_SwapInterval( &window, swapInterval );
			noVSyncNanoseconds = 0;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_L ) ||
			( noLogNanoseconds > 0 && time - startupTimeNanoseconds > noLogNanoseconds ) )
		{
			ksFrameLog_Open( OUTPUT_PATH "framelog_scene.txt", 10 );
			noLogNanoseconds = 0;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_H ) )
		{
			hmd_headRotationDisabled = !hmd_headRotationDisabled;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_P ) )
		{
			ksSceneSettings_ToggleSimulationPaused( &sceneSettings );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_R ) )
		{
			ksSceneSettings_CycleDisplayResolutionLevel( &sceneSettings );
			startupSettings->displayResolutionLevel = sceneSettings.displayResolutionLevel;
			break;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_B ) )
		{
			ksSceneSettings_CycleEyeImageResolutionLevel( &sceneSettings );
			startupSettings->eyeImageResolutionLevel = sceneSettings.eyeImageResolutionLevel;
			break;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_S ) )
		{
			ksSceneSettings_CycleEyeImageSamplesLevel( &sceneSettings );
			startupSettings->eyeImageSamplesLevel = sceneSettings.eyeImageSamplesLevel;
			break;
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_Q ) )
		{
			ksSceneSettings_CycleDrawCallLevel( &sceneSettings );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_W ) )
		{
			ksSceneSettings_CycleTriangleLevel( &sceneSettings );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_E ) )
		{
			ksSceneSettings_CycleFragmentLevel( &sceneSettings );
		}
		if ( ksGpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_D ) )
		{
			DumpGLSL();
		}

		if ( window.windowActive )
		{
			const ksNanoseconds nextSwapTime = ksGpuWindow_GetNextSwapTimeNanoseconds( &window );

			if ( startupSettings->glTF == NULL )
			{
				ksPerfScene_Simulate( &perfScene, &viewState, nextSwapTime );
			}
			else
			{
				ksGltfScene_Simulate( &gltfScene, &viewState, &window.input, nextSwapTime );
			}

			ksFrameLog_BeginFrame();

			const ksNanoseconds t0 = GetTimeNanoseconds();

			const ksScreenRect screenRect = ksGpuFramebuffer_GetRect( &framebuffer );

			ksGpuCommandBuffer_BeginPrimary( &commandBuffer );
			ksGpuCommandBuffer_BeginFramebuffer( &commandBuffer, &framebuffer, 0, KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );

			if ( startupSettings->glTF == NULL )
			{
				ksPerfScene_UpdateBuffers( &commandBuffer, &perfScene, &viewState, 0 );
			}
			else
			{
				ksGltfScene_UpdateBuffers( &commandBuffer, &gltfScene, &viewState, 0 );
			}

			ksBarGraph_UpdateGraphics( &commandBuffer, &frameCpuTimeBarGraph );
			ksBarGraph_UpdateGraphics( &commandBuffer, &frameGpuTimeBarGraph );

			ksGpuCommandBuffer_BeginTimer( &commandBuffer, &timer );
			ksGpuCommandBuffer_BeginRenderPass( &commandBuffer, &renderPass, &framebuffer, &screenRect );

			ksGpuCommandBuffer_SetViewport( &commandBuffer, &screenRect );
			ksGpuCommandBuffer_SetScissor( &commandBuffer, &screenRect );

			if ( startupSettings->glTF == NULL )
			{
				ksPerfScene_Render( &commandBuffer, &perfScene, &viewState );
			}
			else
			{
				ksGltfScene_Render( &commandBuffer, &gltfScene, &viewState );
			}

			ksBarGraph_RenderGraphics( &commandBuffer, &frameCpuTimeBarGraph );
			ksBarGraph_RenderGraphics( &commandBuffer, &frameGpuTimeBarGraph );

			ksGpuCommandBuffer_EndRenderPass( &commandBuffer, &renderPass );
			ksGpuCommandBuffer_EndTimer( &commandBuffer, &timer );

			ksGpuCommandBuffer_EndFramebuffer( &commandBuffer, &framebuffer, 0, KS_GPU_TEXTURE_USAGE_PRESENTATION );
			ksGpuCommandBuffer_EndPrimary( &commandBuffer );

			ksGpuCommandBuffer_SubmitPrimary( &commandBuffer );

			const ksNanoseconds t1 = GetTimeNanoseconds();

			const ksNanoseconds sceneCpuTime = t1 - t0;
			const ksNanoseconds sceneGpuTime = ksGpuTimer_GetNanoseconds( &timer );

			ksFrameLog_EndFrame( sceneCpuTime, sceneGpuTime, KS_GPU_TIMER_FRAMES_DELAYED );

			ksBarGraph_AddBar( &frameCpuTimeBarGraph, 0, sceneCpuTime * window.windowRefreshRate * 1e-9f, &colorGreen, true );
			ksBarGraph_AddBar( &frameGpuTimeBarGraph, 0, sceneGpuTime * window.windowRefreshRate * 1e-9f, &colorGreen, true );

			ksGpuWindow_SwapBuffers( &window );
		}
	}

	if ( startupSettings->glTF == NULL )
	{
		ksPerfScene_Destroy( &window.context, &perfScene );
	}
	else
	{
		ksGltfScene_Destroy( &window.context, &gltfScene );
	}

	ksBarGraph_Destroy( &window.context, &frameGpuTimeBarGraph );
	ksBarGraph_Destroy( &window.context, &frameCpuTimeBarGraph );
	ksGpuTimer_Destroy( &window.context, &timer );
	ksGpuCommandBuffer_Destroy( &window.context, &commandBuffer );
	ksGpuFramebuffer_Destroy( &window.context, &framebuffer );
	ksGpuRenderPass_Destroy( &window.context, &renderPass );
	ksGpuWindow_Destroy( &window );
	ksDriverInstance_Destroy( &instance );

	return exit;
}

/*
================================================================================================================================

Startup

================================================================================================================================
*/

static int StartApplication( int argc, char * argv[] )
{
	ksStartupSettings startupSettings;
	memset( &startupSettings, 0, sizeof( startupSettings ) );
	startupSettings.startupTimeNanoseconds = GetTimeNanoseconds();
	
	for ( int i = 1; i < argc; i++ )
	{
		const char * arg = argv[i];
		if ( arg[0] == '-' ) { arg++; }

		if ( strcmp( arg, "a" ) == 0 && i + 0 < argc )	{ startupSettings.glTF = argv[++i]; }
		else if ( strcmp( arg, "f" ) == 0 && i + 0 < argc )	{ startupSettings.fullscreen = true; }
		else if ( strcmp( arg, "v" ) == 0 && i + 1 < argc )	{ startupSettings.noVSyncNanoseconds = (ksNanoseconds)( atof( argv[++i] ) * 1000 * 1000 * 1000 ); }
		else if ( strcmp( arg, "h" ) == 0 && i + 0 < argc )	{ startupSettings.headRotationDisabled = true; }
		else if ( strcmp( arg, "p" ) == 0 && i + 0 < argc )	{ startupSettings.simulationPaused = true; }
		else if ( strcmp( arg, "r" ) == 0 && i + 1 < argc )	{ startupSettings.displayResolutionLevel = ksStartupSettings_StringToLevel( argv[++i], MAX_DISPLAY_RESOLUTION_LEVELS ); }
		else if ( strcmp( arg, "b" ) == 0 && i + 1 < argc )	{ startupSettings.eyeImageResolutionLevel = ksStartupSettings_StringToLevel( argv[++i], MAX_EYE_IMAGE_RESOLUTION_LEVELS ); }
		else if ( strcmp( arg, "s" ) == 0 && i + 1 < argc )	{ startupSettings.eyeImageSamplesLevel = ksStartupSettings_StringToLevel( argv[++i], MAX_EYE_IMAGE_SAMPLES_LEVELS ); }
		else if ( strcmp( arg, "q" ) == 0 && i + 1 < argc )	{ startupSettings.drawCallLevel = ksStartupSettings_StringToLevel( argv[++i], MAX_SCENE_DRAWCALL_LEVELS ); }
		else if ( strcmp( arg, "w" ) == 0 && i + 1 < argc )	{ startupSettings.triangleLevel = ksStartupSettings_StringToLevel( argv[++i], MAX_SCENE_TRIANGLE_LEVELS ); }
		else if ( strcmp( arg, "e" ) == 0 && i + 1 < argc )	{ startupSettings.fragmentLevel = ksStartupSettings_StringToLevel( argv[++i], MAX_SCENE_FRAGMENT_LEVELS ); }
		else if ( strcmp( arg, "m" ) == 0 && i + 0 < argc )	{ startupSettings.useMultiView = ( atoi( argv[++i] ) != 0 ); }
		else if ( strcmp( arg, "c" ) == 0 && i + 1 < argc )	{ startupSettings.correctChromaticAberration = ( atoi( argv[++i] ) != 0 ); }
		else if ( strcmp( arg, "i" ) == 0 && i + 1 < argc )	{ startupSettings.timeWarpImplementation = (ksTimeWarpImplementation)ksStartupSettings_StringToTimeWarpImplementation( argv[++i] ); }
		else if ( strcmp( arg, "z" ) == 0 && i + 1 < argc )	{ startupSettings.renderMode = ksStartupSettings_StringToRenderMode( argv[++i] ); }
		else if ( strcmp( arg, "g" ) == 0 && i + 0 < argc )	{ startupSettings.hideGraphs = true; }
		else if ( strcmp( arg, "l" ) == 0 && i + 1 < argc )	{ startupSettings.noLogNanoseconds = (ksNanoseconds)( atof( argv[++i] ) * 1000 * 1000 * 1000 ); }
		else if ( strcmp( arg, "d" ) == 0 && i + 0 < argc )	{ DumpGLSL(); exit( 0 ); }
		else
		{
			Print( "Unknown option: %s\n"
				   "atw_opengl [options]\n"
				   "options:\n"
				   "   -a <file>   load glTF scene\n"
				   "   -f          start fullscreen\n"
				   "   -v <s>      start with V-Sync disabled for this many seconds\n"
				   "   -h          start with head rotation disabled\n"
				   "   -p          start with the simulation paused\n"
				   "   -r <0-3>    set display resolution level\n"
				   "   -b <0-3>    set eye image resolution level\n"
				   "   -s <0-3>    set multi-sampling level\n"
				   "   -q <0-3>    set per eye draw calls level\n"
				   "   -w <0-3>    set per eye triangles per draw call level\n"
				   "   -e <0-3>    set per eye fragment program complexity level\n"
				   "   -m <0-1>    enable/disable multi-view\n"
				   "   -c <0-1>    enable/disable correction for chromatic aberration\n"
				   "   -i <name>   set time warp implementation: graphics, compute\n"
				   "   -z <name>   set the render mode: atw, tw, scene\n"
				   "   -g          hide graphs\n"
				   "   -l <s>      log 10 frames of OpenGL commands after this many seconds\n"
				   "   -d          dump GLSL to files for conversion to SPIR-V\n",
				   arg );
			return 1;
		}
	}

	//startupSettings.glTF = "models.json";
	//startupSettings.headRotationDisabled = true;
	//startupSettings.simulationPaused = true;
	//startupSettings.eyeImageSamplesLevel = 0;
	//startupSettings.useMultiView = true;
	//startupSettings.correctChromaticAberration = true;
	//startupSettings.renderMode = RENDER_MODE_SCENE;
	//startupSettings.timeWarpImplementation = TIMEWARP_IMPLEMENTATION_COMPUTE;

	Print( "    fullscreen = %d\n",					startupSettings.fullscreen );
	Print( "    noVSyncNanoseconds = %lld\n",		startupSettings.noVSyncNanoseconds );
	Print( "    headRotationDisabled = %d\n",		startupSettings.headRotationDisabled );
	Print( "    simulationPaused = %d\n",			startupSettings.simulationPaused );
	Print( "    displayResolutionLevel = %d\n",		startupSettings.displayResolutionLevel );
	Print( "    eyeImageResolutionLevel = %d\n",	startupSettings.eyeImageResolutionLevel );
	Print( "    eyeImageSamplesLevel = %d\n",		startupSettings.eyeImageSamplesLevel );
	Print( "    drawCallLevel = %d\n",				startupSettings.drawCallLevel );
	Print( "    triangleLevel = %d\n",				startupSettings.triangleLevel );
	Print( "    fragmentLevel = %d\n",				startupSettings.fragmentLevel );
	Print( "    useMultiView = %d\n",				startupSettings.useMultiView );
	Print( "    correctChromaticAberration = %d\n",	startupSettings.correctChromaticAberration );
	Print( "    timeWarpImplementation = %d\n",		startupSettings.timeWarpImplementation );
	Print( "    renderMode = %d\n",					startupSettings.renderMode );
	Print( "    hideGraphs = %d\n",					startupSettings.hideGraphs );
	Print( "    noLogNanoseconds = %lld\n",			startupSettings.noLogNanoseconds );

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

#elif defined( OS_LINUX )

int main( int argc, char * argv[] )
{
	return StartApplication( argc, argv );
}

#elif defined( OS_APPLE_MACOS )

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
		When an application executable is not launched from a bundle, macOS
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

#elif defined( OS_APPLE_IOS )

static int argc_deferred;
static char** argv_deferred;

@interface MyAppDelegate : NSObject <UIApplicationDelegate> {}
@end

@implementation MyAppDelegate

-(BOOL) application: (UIApplication*) application didFinishLaunchingWithOptions: (NSDictionary*) launchOptions {

	CGRect screenRect = UIScreen.mainScreen.bounds;
	myUIView = [[MyUIView alloc] initWithFrame: screenRect];
	UIViewController* myUIVC = [[[MyUIViewController alloc] init] autorelease];
	myUIVC.view = myUIView;

	myUIWindow = [[UIWindow alloc] initWithFrame: screenRect];
	[myUIWindow addSubview: myUIView];
	myUIWindow.rootViewController = myUIVC;
	[myUIWindow makeKeyAndVisible];

	// Delay to allow startup runloop to complete.
	[self performSelector: @selector(startApplication:) withObject: nil afterDelay: 0.25f];

	return YES;
}

-(void) startApplication: (id) argObj {
	autoReleasePool = [[NSAutoreleasePool alloc] init];
	StartApplication( argc_deferred, argv_deferred );
}

@end

int main( int argc, char * argv[] )
{
	argc_deferred = argc;
	argv_deferred = argv;

	return UIApplicationMain( argc, argv, nil, @"MyAppDelegate" );
}

#elif defined( OS_ANDROID )

#define MAX_ARGS		32
#define MAX_ARGS_BUFFER	1024

typedef struct
{
	char	buffer[MAX_ARGS_BUFFER];
	char *	argv[MAX_ARGS];
	int		argc;
} ksAndroidParm;

// adb shell am start -n com.vulkansamples.atw_vulkan/android.app.NativeActivity -a "android.intent.action.MAIN" --es "args" "\"-r tw\""
void GetIntentParms( ksAndroidParm * parms )
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

	ksAndroidParm parms;
	GetIntentParms( &parms );

	StartApplication( parms.argc, parms.argv );
}

#endif

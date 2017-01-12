/*
================================================================================================

Description	:	Asynchronous Time Warp test utility for OpenGL.
Author		:	J.M.P. van Waveren
Date		:	12/21/2014
Language	:	C99
Format		:	Real tabs with the tab size equal to 4 spaces.
Copyright	:	Copyright (c) 2016 Oculus VR, LLC. All Rights reserved.
			:	Portions copyright (c) 2016 The Brenwill Workshop Ltd. All Rights reserved.


LICENSE
=======

Copyright (c) 2016 Oculus VR, LLC.
Portions of macOS, iOS, functionality copyright (c) 2016 The Brenwill Workshop Ltd.

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

This implements the simplest form of time warp transform for OpenGL.
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

	- Context priorities.
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
	-l <s>		log 10 frames of OpenGL commands after this many seconds
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
	[L]		= log 10 frames of OpenGL commands
	[D]		= dump GLSL to files for conversion to SPIR-V
	[Esc]	= exit


IMPLEMENTATION
==============

The code is written in an object-oriented style with a focus on minimizing state
and side effects. The majority of the functions manipulate self-contained objects
without modifying any global state (except for OpenGL state). The types
introduced in this file have no circular dependencies, and there are no forward
declarations.

Even though an object-oriented style is used, the code is written in straight C99 for
maximum portability and readability. To further improve portability and to simplify
compilation, all source code is in a single file without any dependencies on third-
party code or non-standard libraries. The code does not use an OpenGL loading library
like GLEE, GLEW, GL3W, or an OpenGL toolkit like GLUT, FreeGLUT, GLFW, etc. Instead,
the code provides direct access to window and context creation for driver extension work.

The code is written against version 4.3 of the Core Profile OpenGL Specification,
and version 3.1 of the OpenGL ES Specification.

Supported platforms are:

	- Microsoft Windows 7 or later
	- Ubuntu Linux 14.04 or later
	- Apple macOS 10.11 or later
	- Apple iOS 9.0 or later
	- Android 5.0 or later


GRAPHICS API WRAPPER
====================

The code wraps the OpenGL API with a convenient wrapper that takes care of a
lot of the OpenGL intricacies. This wrapper does not expose the full OpenGL API
but can be easily extended to support more features. Some of the current
limitations are:

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
	include\GL\glext.h  -> https://www.opengl.org/registry/api/GL/glext.h
	include\GL\wglext.h -> https://www.opengl.org/registry/api/GL/wglext.h
	"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
	cl /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /arch:SSE2 /Iinclude atw_opengl.c /link user32.lib gdi32.lib Advapi32.lib opengl32.lib

Microsoft Windows: Intel Compiler 14.0
	include\GL\glext.h  -> https://www.opengl.org/registry/api/GL/glext.h
	include\GL\wglext.h -> https://www.opengl.org/registry/api/GL/wglext.h
	"C:\Program Files (x86)\Intel\Composer XE\bin\iclvars.bat" intel64
	icl /Qstd=c99 /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /arch:SSE2 /Iinclude atw_opengl.c /link user32.lib gdi32.lib Advapi32.lib opengl32.lib

Linux: GCC 4.8.2 Xlib:
	sudo apt-get install libx11-dev
	sudo apt-get install libxxf86vm-dev
	sudo apt-get install libxrandr-dev
	sudo apt-get install mesa-common-dev
	sudo apt-get install libgl1-mesa-dev
	gcc -std=c99 -Wall -g -O2 -m64 -o atw_opengl atw_opengl.c -lm -lpthread -lX11 -lXxf86vm -lXrandr -lGL

Linux: GCC 4.8.2 XCB:
	sudo apt-get install libxcb1-dev
	sudo apt-get install libxcb-keysyms1-dev
	sudo apt-get install libxcb-icccm4-dev
	sudo apt-get install mesa-common-dev
	sudo apt-get install libgl1-mesa-dev
	gcc -std=c99 -Wall -g -O2 -o -m64 atw_opengl atw_opengl.c -lm -lpthread -lxcb -lxcb-keysyms -lxcb-randr -lxcb-glx -lxcb-dri2 -lGL

Apple macOS: Apple LLVM 6.0:
	clang -std=c99 -x objective-c -fno-objc-arc -Wall -g -O2 -o atw_opengl atw_opengl.c -framework Cocoa -framework OpenGL

Android for ARM from Windows: NDK Revision 11c - Android 21 - ANT/Gradle
	ANT:
		cd projects/android/ant/atw_opengl
		ndk-build
		ant debug
		adb install -r bin/atw_opengl-debug.apk
	Gradle:
		cd projects/android/gradle/atw_opengl
		gradlew build
		adb install -r build/outputs/apk/atw_opengl-all-debug.apk


KNOWN ISSUES
============

OS     : Apple Mac OS X 10.9.5
GPU    : Geforce GT 750M
DRIVER : NVIDIA 310.40.55b01
-----------------------------------------------
- glGetQueryObjectui64v( query, GL_QUERY_RESULT, &time ) always returns zero for a timer query.
- glFlush() after a glFenceSync() stalls the CPU for many milliseconds.
- Creating a context fails when the share context is current on another thread.

OS     : Android 6.0.1
GPU    : Adreno (TM) 530
DRIVER : OpenGL ES 3.1 V@145.0
-----------------------------------------------
- Enabling OVR_multiview hangs the GPU.


WORK ITEMS
==========

- Implement WGL, GLX and NSOpenGL equivalents of EGL_IMG_context_priority.
- Implement an extension that provides accurate display refresh timing (WGL_NV_delay_before_swap, D3DKMTGetScanLine).
- Implement an OpenGL extension that allows rendering directly to the front buffer.
- Implement an OpenGL extension that allows a compute shader to directly write to the front/back buffer images (WGL_AMDX_drawable_view).
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
	#define OS_LINUX_XLIB		// Xlib + Xlib GLX 1.3
	//#define OS_LINUX_XCB		// XCB + XCB GLX is limited to OpenGL 2.1
	//#define OS_LINUX_XCB_GLX	// XCB + Xlib GLX 1.3
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
		#pragma warning( disable : 4221 )	// nonstandard extension used: 'layers': cannot be initialized using address of automatic variable 'layerProjection'
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

	#define OPENGL_VERSION_MAJOR	4
	#define OPENGL_VERSION_MINOR	3
	#define GLSL_VERSION			"430"
	#define SPIRV_VERSION			"99"
	#define USE_SYNC_OBJECT			0			// 0 = GLsync, 1 = EGLSyncKHR, 2 = storage buffer

	#include <windows.h>
	#include <GL/gl.h>
	#define GL_EXT_color_subtable
	#include <GL/glext.h>
	#include <GL/wglext.h>
	#include <GL/gl_format.h>

	#define GRAPHICS_API_OPENGL		1
	#define OUTPUT_PATH				""

	#define __thread	__declspec( thread )

#elif defined( OS_LINUX )

	#define OPENGL_VERSION_MAJOR	4
	#define OPENGL_VERSION_MINOR	3
	#define GLSL_VERSION			"430"
	#define SPIRV_VERSION			"99"
	#define USE_SYNC_OBJECT			0			// 0 = GLsync, 1 = EGLSyncKHR, 2 = storage buffer

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
	#if defined( OS_LINUX_XLIB )
		#include <X11/Xlib.h>
		#include <X11/Xatom.h>
		#include <X11/extensions/xf86vmode.h>	// for fullscreen video mode
		#include <X11/extensions/Xrandr.h>		// for resolution changes
	#elif defined( OS_LINUX_XCB ) || defined( OS_LINUX_XCB_GLX )
		#include <X11/keysym.h>
		#include <xcb/xcb.h>
		#include <xcb/xcb_keysyms.h>
		#include <xcb/xcb_icccm.h>
		#include <xcb/randr.h>
		#include <xcb/glx.h>
		#include <xcb/dri2.h>
	#endif
	#include <GL/glx.h>
	#include <GL/gl_format.h>

	#define GRAPHICS_API_OPENGL		1
	#define OUTPUT_PATH				""

	// These prototypes are only included when __USE_GNU is defined but that causes other compile errors.
	extern int pthread_setname_np( pthread_t __target_thread, __const char *__name );
	extern int pthread_setaffinity_np( pthread_t thread, size_t cpusetsize, const cpu_set_t * cpuset );

	#pragma GCC diagnostic ignored "-Wunused-function"

#elif defined( OS_APPLE_MACOS )

	// Apple is still at OpenGL 4.1
	#define OPENGL_VERSION_MAJOR	4
	#define OPENGL_VERSION_MINOR	1
	#define GLSL_VERSION			"410"
	#define SPIRV_VERSION			"99"
	#define USE_SYNC_OBJECT			0			// 0 = GLsync, 1 = EGLSyncKHR, 2 = storage buffer

	#include <sys/param.h>
	#include <sys/sysctl.h>
	#include <sys/time.h>
	#include <pthread.h>
	#include <Cocoa/Cocoa.h>
	#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
	#include <OpenGL/OpenGL.h>
	#include <OpenGL/gl3.h>
	#include <OpenGL/gl3ext.h>
	#include <GL/gl_format.h>

	#undef MAX
	#undef MIN

	#define GRAPHICS_API_OPENGL		1
	#define OUTPUT_PATH				""

	// Undocumented CGS and CGL
	typedef void * CGSConnectionID;
	typedef int CGSWindowID;
	typedef int CGSSurfaceID;
    
	CGLError CGLSetSurface( CGLContextObj ctx, CGSConnectionID cid, CGSWindowID wid, CGSSurfaceID sid );
	CGLError CGLGetSurface( CGLContextObj ctx, CGSConnectionID * cid, CGSWindowID * wid, CGSSurfaceID * sid );
	CGLError CGLUpdateContext( CGLContextObj ctx );

	#pragma clang diagnostic ignored "-Wunused-function"
	#pragma clang diagnostic ignored "-Wunused-const-variable"

#elif defined( OS_APPLE_IOS )

// FIXME:

	#define GRAPHICS_API_OPENGL_ES	1

#elif defined( OS_ANDROID )

	#define OPENGL_VERSION_MAJOR	3
	#define OPENGL_VERSION_MINOR	1
	#define GLSL_VERSION			"310 es"
	#define SPIRV_VERSION			"99"
	#define USE_SYNC_OBJECT			1			// 0 = GLsync, 1 = EGLSyncKHR, 2 = storage buffer

	#include <time.h>
	#include <unistd.h>
	#include <dirent.h>							// for opendir/closedir
	#include <pthread.h>
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
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
	#include <GLES2/gl2ext.h>
	#include <GLES3/gl3.h>
	#if OPENGL_VERSION_MAJOR == 3 && OPENGL_VERSION_MINOR == 1
		#include <GLES3/gl31.h>
	#endif
	#include <GLES3/gl3ext.h>
	#include <GL/gl_format.h>

	#define GRAPHICS_API_OPENGL_ES	1
	#define OUTPUT_PATH				"/sdcard/"

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
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>			// for memset
#include <errno.h>			// for EBUSY, ETIMEDOUT etc.
#include <ctype.h>			// for isspace, isdigit

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
#define MAX( x, y )						( ( x > y ) ? ( x ) : ( y ) )
#define MIN( x, y )						( ( x < y ) ? ( x ) : ( y ) )
#define CLAMP( x, min, max )			( ( (x) < (min) ) ? (min) : ( ( (x) > (max) ) ? (max) : (x) ) )
#define STRINGIFY_EXPANDED( a )			#a
#define STRINGIFY( a )					STRINGIFY_EXPANDED(a)

#define APPLICATION_NAME				"OpenGL ATW"
#define WINDOW_TITLE					"Asynchronous Time Warp - OpenGL"

#define PROGRAM( name )					name##GLSL

#define GLSL_EXTENSIONS					"#extension GL_EXT_shader_io_blocks : enable\n"
#define GL_FINISH_SYNC					1

#if defined( OS_ANDROID )
#define ES_HIGHP						"highp"	// GLSL "310 es" requires a precision qualifier on a image2D
#else
#define ES_HIGHP						""		// GLSL "430" disallows a precision qualifier on a image2D
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

	OutputDebugString( buffer );
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

	OutputDebugString( buffer );

	MessageBox( NULL, buffer, "ERROR", MB_OK | MB_ICONINFORMATION );
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
#elif defined( OS_APPLE_MACOS )
	return [NSString stringWithFormat: @"Apple macOS %@", NSProcessInfo.processInfo.operatingSystemVersionString].UTF8String;
#elif defined( OS_APPLE_IOS )
	return [NSString stringWithFormat: @"Apple iOS %@", NSProcessInfo.processInfo.operatingSystemVersionString].UTF8String;
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

typedef uint64_t ksNanoseconds;

static ksNanoseconds GetTimeNanoseconds()
{
#if defined( OS_WINDOWS )
	static ksNanoseconds ticksPerSecond = 0;
	static ksNanoseconds timeBase = 0;

	if ( ticksPerSecond == 0 )
	{
		LARGE_INTEGER li;
		QueryPerformanceFrequency( &li );
		ticksPerSecond = (ksNanoseconds) li.QuadPart;
		QueryPerformanceCounter( &li );
		timeBase = (ksNanoseconds) li.LowPart + 0xFFFFFFFFULL * li.HighPart;
	}

	LARGE_INTEGER li;
	QueryPerformanceCounter( &li );
	ksNanoseconds counter = (ksNanoseconds) li.LowPart + 0xFFFFFFFFULL * li.HighPart;
	return ( counter - timeBase ) * 1000ULL * 1000ULL * 1000ULL / ticksPerSecond;
#elif defined( OS_ANDROID )
	static ksNanoseconds timeBase = 0;

	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts );

	if ( timeBase == 0 )
	{
		timeBase = (ksNanoseconds) ts.tv_sec * 1000ULL * 1000ULL * 1000ULL + ts.tv_nsec;
	}

	return (ksNanoseconds) ts.tv_sec * 1000ULL * 1000ULL * 1000ULL + ts.tv_nsec - timeBase;
#else
	static ksNanoseconds timeBase = 0;

	struct timeval tv;
	gettimeofday( &tv, 0 );

	if ( timeBase == 0 )
	{
		timeBase = (ksNanoseconds) tv.tv_sec * 1000ULL * 1000ULL * 1000ULL + tv.tv_usec * 1000ULL;
	}

	return (ksNanoseconds) tv.tv_sec * 1000ULL * 1000ULL * 1000ULL + tv.tv_usec * 1000ULL - timeBase;
#endif
}

/*
================================================================================================================================

Basic C99-style lexer.

ksTokenType
ksTokenFlags
ksTokenInfo

================================================================================================================================
*/

typedef enum
{
	KS_TOKEN_TYPE_NONE,
	KS_TOKEN_TYPE_NAME,
	KS_TOKEN_TYPE_STRING,
	KS_TOKEN_TYPE_LITERAL,
	KS_TOKEN_TYPE_NUMBER,
	KS_TOKEN_TYPE_PUNCTUATION
} ksTokenType;

typedef enum
{
	KS_TOKEN_FLAG_NONE			= 0,
	KS_TOKEN_FLAG_DECIMAL		= BIT( 0 ),
	KS_TOKEN_FLAG_OCTAL			= BIT( 1 ),
	KS_TOKEN_FLAG_HEXADECIMAL	= BIT( 2 ),
	KS_TOKEN_FLAG_UNSIGNED		= BIT( 3 ),
	KS_TOKEN_FLAG_LONG			= BIT( 4 ),
	KS_TOKEN_FLAG_LONG_LONG		= BIT( 5 ),
	KS_TOKEN_FLAG_FLOAT			= BIT( 6 ),
	KS_TOKEN_FLAG_DOUBLE		= BIT( 7 )
} ksTokenFlags;

typedef struct ksTokenInfo
{
	ksTokenType			type;			// Token type.
	ksTokenFlags		flags;			// Token flags.
	int					linesCrossed;	// Number of lines crossed before the token.
} ksTokenInfo;

// Gets the next C99-style token from a zero-terminated buffer.
// 'buffer' is the base pointer of the buffer and 'ptr' is the current pointer into the buffer.
// A pointer to the next token is returned in 'token' and if 'tokenInfo' is not NULL, then additional information is returned in 'tokenInfo'.
// This is a zero-allocation lexer. A token is returned as a pointer in the original input buffer. As a result:
//  - All tokens are left untouched, including escape sequencies in strings and literals.
//  - Multi-line strings are not automatically merged into a single token.
// Returns a pointer to the first character after the token.
// The length of a token is the returned pointer minus the token pointer stored in 'token'.
static const unsigned char * ksLexer_NextToken( const unsigned char * buffer, const unsigned char * ptr, const unsigned char ** token, ksTokenInfo * tokenInfo )
{
	const unsigned char * start = ptr;
	int linesCrossed = 0;

	// Parse non-tokens.
	while ( ptr[0] != '\0' )
	{
		// Parse white space
		while ( ptr[0] != '\0' && ptr[0] <= ' ' )
		{
			linesCrossed += ( ptr[0] == '\n' );
			ptr++;
		}
		// Parse comment.
		if ( ptr[0] == '/' )
		{
			if ( ptr[1] == '/' )
			{
				ptr += 2;
				while ( ptr[0] != '\0' && ptr[0] != '\n' )
				{
					ptr++;
				}
				continue;
			}
			else if ( ptr[1] == '*' )
			{
				ptr += 2;
				while ( ptr[0] != '\0' && ( ptr[0] != '*' || ptr[1] != '/' ) )
				{
					linesCrossed += ( ptr[0] == '\n' );
					ptr++;
				}
				ptr += 2 * ( ptr[0] != '\0' );
				continue;
			}
		}
		break;
	}

	// Save off pointer to token.
	*token = ptr;

	// Parse name token.
	{
		while (	( ptr[0] >= 'a' && ptr[0] <= 'z' ) ||
				( ptr[0] >= 'A' && ptr[0] <= 'Z' ) ||
				( ptr[0] == '_' ) ||
				( ( ptr[0] >= '0' && ptr[0] <= '9' ) && ptr > *token ) )
		{
			ptr++;
		}
		if ( ptr > *token )
		{
			if ( tokenInfo != NULL )
			{
				tokenInfo->type = KS_TOKEN_TYPE_NAME;
				tokenInfo->flags = KS_TOKEN_FLAG_NONE;
				tokenInfo->linesCrossed = linesCrossed;
			}
			return ptr;
		}
	}

	// Parse string or literal token.
	if ( ptr[0] == '\"' || ptr[0] == '\'' )
	{
		const char firstChar = ptr[0];
		ptr++;
		while ( ptr[0] != '\0' && ptr[0] != firstChar )
		{
			assert( ptr[0] != '\n' );

			if ( ptr[0] == '\\' )
			{
				ptr++;
				if ( ptr[0] == 'x' || ptr[0] == 'X' || ptr[0] == 'u' || ptr[0] == 'U' )
				{
					// Parse hexadecimal or Unicode.
					while ( ( ptr[0] >= '0' && ptr[0] <= '9' ) ||
							( ptr[0] >= 'a' && ptr[0] <= 'f' ) ||
							( ptr[0] >= 'A' && ptr[0] <= 'F' ) )
					{
						ptr++;
					}
				}
				else if ( ptr[0] >= '0' && ptr[0] <= '7' )
				{
					// Parse octal.
					do
					{
						ptr++;
					}
					while ( ptr[0] >= '0' && ptr[0] <= '7' );
				}
				else
				{
					ptr++;
				}
			}
			else
			{
				ptr++;
			}
		}
		ptr++;
		if ( tokenInfo != NULL )
		{
			tokenInfo->type = ( firstChar == '\"' ) ? KS_TOKEN_TYPE_STRING : KS_TOKEN_TYPE_LITERAL;
			tokenInfo->flags = KS_TOKEN_FLAG_NONE;
			tokenInfo->linesCrossed = linesCrossed;
		}
		return ptr;
	}

	// Parse number token.
	{
		// Parse sign.
		if (	( ptr[0] == '+' || ptr[0] == '-' )
				&&
				( ( ptr[1] >= '0' && ptr[1] <= '9' ) || ( ptr[1] == '.' ) )
				&&
				(
					( start == buffer )
					||
					(
						!( start[-1] >= 'a' && start[-1] <= 'z' ) &&
						!( start[-1] >= 'A' && start[-1] <= 'Z' ) &&
						!( start[-1] == '_' || start[-1] == ')' || start[1] == ']' ) &&
						!( start[-1] >= '0' && start[-1] <= '9' )
					)
				)
			)
		{
			ptr++;
		}

		ksTokenFlags flags = KS_TOKEN_FLAG_DECIMAL;

		// Parse octal or hexadecimal.
		if ( ptr[0] == '0' )
		{
			if ( ptr[1] == 'x' || ptr[1] == 'X' )
			{
				ptr += 2;
				while ( ( ptr[0] >= '0' && ptr[0] <= '9' ) ||
						( ptr[0] >= 'a' && ptr[0] <= 'f' ) ||
						( ptr[0] >= 'A' && ptr[0] <= 'F' ) )
				{
					ptr++;
				}
				flags = KS_TOKEN_FLAG_HEXADECIMAL;
			}
			else if ( ptr[1] >= '0' && ptr[1] <= '7' )
			{
				ptr += 2;
				while ( ptr[0] >= '0' && ptr[0] <= '7' )
				{
					ptr++;
				}
				flags = KS_TOKEN_FLAG_OCTAL;
			}
		}

		// Parse decimal integer or floating-point.
		if ( flags == KS_TOKEN_FLAG_DECIMAL )
		{
			for ( bool hasDigit = false; ptr[0] != '\0'; )
			{
				if ( ptr[0] >= '0' && ptr[0] <= '9' )
				{
					ptr++;
					hasDigit = true;
					continue;
				}
				if ( ptr[0] == '.' &&
						( hasDigit || ( ptr[1] >= '0' && ptr[1] <= '9' ) ) )
				{
					ptr++;
					flags |= KS_TOKEN_FLAG_DOUBLE;
					continue;
				}
				if ( hasDigit )
				{
					if (	( ptr[0] == 'e' || ptr[0] == 'E' )
							&&
							(
								( ptr[1] >= '0' && ptr[1] <= '9' )
								||
								(
									( ptr[1] == '+' || ptr[1] == '-' )
									&&
									( ptr[2] >= '0' && ptr[2] <= '9' )
								)
							)
						)
					{
						ptr++;
						if ( ptr[0] == '+' || ptr[0] == '-' )
						{
							ptr++;
						}
						flags |= KS_TOKEN_FLAG_DOUBLE;
						continue;
					}
					if ( ptr[0] == 'f' || ptr[0] == 'F' )
					{
						flags &= ~KS_TOKEN_FLAG_DOUBLE;
						flags |= KS_TOKEN_FLAG_FLOAT;
						ptr++;
						break;
					}
				}
				break;
			}
		}

		// Identify unsigned, long and long long integers.
		if ( ( flags & ( KS_TOKEN_FLAG_FLOAT | KS_TOKEN_FLAG_DOUBLE ) ) == 0 )
		{
			while ( ptr[0] != '\0' )
			{
				if ( ptr[0] == 'u' || ptr[0] == 'U' )
				{
					flags |= KS_TOKEN_FLAG_UNSIGNED;
					ptr++;
				}
				else if ( ptr[0] == 'l' || ptr[0] == 'L' )
				{
					if ( ptr[1] == 'l' || ptr[1] == 'L' )
					{
						flags |= KS_TOKEN_FLAG_LONG_LONG;
						ptr += 2;
					}
					else
					{
						flags |= KS_TOKEN_FLAG_LONG;
						ptr++;
					}
				}
				else
				{
					break;
				}
			}
		}

		if ( ptr > *token )
		{
			if ( tokenInfo != NULL )
			{
				tokenInfo->type = KS_TOKEN_TYPE_NUMBER;
				tokenInfo->flags = flags;
				tokenInfo->linesCrossed = linesCrossed;
			}
			return ptr;
		}
	}

	// Parse punctuation token.
	switch ( ptr[0] )
	{
		// Handle multi-character operators.
		case ':': ptr++; ptr += ( ptr[0] == ':' ); break;										// ::
		case '+': ptr++; ptr += ( ptr[0] == '+' || ptr[0] == '=' ); break;						// ++ +=
		case '-': ptr++; ptr += ( ptr[0] == '-' || ptr[0] == '=' || ptr[0] == '>' ); break;		// -- -= ->
		case '*': ptr++; ptr += ( ptr[0] == '=' ); break;										// *=
		case '/': ptr++; ptr += ( ptr[0] == '=' ); break;										// /=
		case '%': ptr++; ptr += ( ptr[0] == '=' ); break;										// %=
		case '<': ptr++; ptr += ( ptr[0] == '<' ); ptr += ( ptr[0] == '=' ); break;				// << <= <<=
		case '>': ptr++; ptr += ( ptr[0] == '>' ); ptr += ( ptr[0] == '=' ); break;				// >> >= >>=
		case '=': ptr++; ptr += ( ptr[0] == '=' ); break;										// ==
		case '!': ptr++; ptr += ( ptr[0] == '=' ); break;										// !=
		case '^': ptr++; ptr += ( ptr[0] == '=' ); break;										// ^=
		case '&': ptr++; ptr += ( ptr[0] == '=' || ptr[0] == '&' ); break;						// &= &&
		case '|': ptr++; ptr += ( ptr[0] == '=' || ptr[0] == '|' ); break;						// |= ||
		default:
		{
			// Consider any non-character and non-digit a punctuation,
			// including C-operators: ( ) [ ] . ? , and @ # $ ` ;
			if (	ptr[0] > ' ' &&
					!( ptr[0] >= 'a' && ptr[0] <= 'z' ) &&
					!( ptr[0] >= 'A' && ptr[0] <= 'Z' ) &&
					!( ptr[0] == '_' ) &&
					!( ptr[0] >= '0' && ptr[0] <= '9' ) )
			{
				ptr++;
			}
		}
	}
	if ( tokenInfo != NULL )
	{
		tokenInfo->type = ( ptr > *token ) ? KS_TOKEN_TYPE_PUNCTUATION : KS_TOKEN_TYPE_NONE;
		tokenInfo->flags = KS_TOKEN_FLAG_NONE;
		tokenInfo->linesCrossed = linesCrossed;
	}
	return ptr;
}

// Case-sensitive compare the non-zero terminated token to the given zero-terminated value.
bool ksLexer_CaseSensitiveCompareToken( const unsigned char * tokenStart, const unsigned char * tokenEnd, const char * value )
{
	if ( value == NULL )
	{
		return false;
	}
	if ( tokenEnd == NULL )
	{
		return ( strcmp( (const char *)tokenStart, value ) == 0 );
	}
	const size_t length = strlen( value );
	return ( (size_t)( tokenEnd - tokenStart ) == length && strncmp( (const char *)tokenStart, value, length ) == 0 );
}

// Skip up to and including the given token.
// Returns a pointer to the first character after the token.
static const unsigned char * ksLexer_SkipUpToIncludingToken( const unsigned char * buffer, const unsigned char * ptr, const char * token )
{
	while ( ptr[0] != '\0' )
	{
		const unsigned char * t;
		ptr = ksLexer_NextToken( buffer, ptr, &t, NULL );
		if ( ksLexer_CaseSensitiveCompareToken( t, ptr, token ) )
		{
			break;
		}
	}
	return ptr;
}

// Skip up to the end of the line.
// Returns a pointer to the first character after the token after which a line is crossed.
static const unsigned char * ksLexer_SkipUpToEndOfLine( const unsigned char * buffer, const unsigned char * ptr )
{
	while ( ptr[0] != '\0' )
	{
		const unsigned char * token;
		ksTokenInfo info;
		const unsigned char * newPtr = ksLexer_NextToken( buffer, ptr, &token, &info );
		if ( info.linesCrossed > 0 )
		{
			break;
		}
		ptr = newPtr;
	}
	return ptr;
}

// Skip the next curly braced section including any nested curly braced sections.
// The opening curly brace is expected to appear after the 'ptr' in the input 'buffer'.
// Any tokens before the first opening curly brace are skipped as well.
// Returns a pointer to the first character after the closing curly brace.
static const unsigned char * ksLexer_SkipBracedSection( const unsigned char * buffer, const unsigned char * ptr )
{
	int braceDepth = 0;
	while ( ptr[0] != '\0' )
	{
		const unsigned char * token;
		ptr = ksLexer_NextToken( buffer, ptr, &token, NULL );
		if ( token[0] == '{' )
		{
			braceDepth++;
		}
		else if ( token[0] == '}' )
		{
			braceDepth--;
			if ( braceDepth == 0 )
			{
				break;
			}
		}
	}
	return ptr;
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

Mutex for mutual exclusion on shared resources within a single process.

Equivalent to a Windows Critical Section Object which allows recursive access. This mutex cannot be
used for mutual-exclusion synchronization between threads from different processes.

ksMutex

static void ksMutex_Create( ksMutex * mutex );
static void ksMutex_Destroy( ksMutex * mutex );
static bool ksMutex_Lock( ksMutex * mutex, const bool blocking );
static void ksMutex_Unlock( ksMutex * mutex );

================================================================================================================================
*/

typedef struct
{
#if defined( OS_WINDOWS )
	CRITICAL_SECTION	handle;
#else
	pthread_mutex_t		mutex;
#endif
} ksMutex;

static void ksMutex_Create( ksMutex * mutex )
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

static void ksMutex_Destroy( ksMutex * mutex )
{
#if defined( OS_WINDOWS )
	DeleteCriticalSection( &mutex->handle );
#else
	pthread_mutex_destroy( &mutex->mutex );
#endif
}

static bool ksMutex_Lock( ksMutex * mutex, const bool blocking )
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

static void ksMutex_Unlock( ksMutex * mutex )
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

ksSignal

static void ksSignal_Create( ksSignal * signal, const bool autoReset );
static void ksSignal_Destroy( ksSignal * signal );
static bool ksSignal_Wait( ksSignal * signal, const ksNanoseconds timeOutNanoseconds );
static void ksSignal_Raise( ksSignal * signal );
static void ksSignal_Clear( ksSignal * signal );

================================================================================================================================
*/

#define SIGNAL_TIMEOUT_INFINITE		0xFFFFFFFFFFFFFFFFULL

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
} ksSignal;

static void ksSignal_Create( ksSignal * signal, const bool autoReset )
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

static void ksSignal_Destroy( ksSignal * signal )
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
// If 'timeOutNanoseconds' is SIGNAL_TIMEOUT_INFINITE then this will wait indefinitely until the signalled state is reached.
// Returns true if the thread was released because the object entered the signalled state, returns false if the time-out is reached first.
static bool ksSignal_Wait( ksSignal * signal, const ksNanoseconds timeOutNanoseconds )
{
#if defined( OS_WINDOWS )
	DWORD result = WaitForSingleObject( signal->handle, ( timeOutNanoseconds == SIGNAL_TIMEOUT_INFINITE ) ? INFINITE : (DWORD)( timeOutNanoseconds / ( 1000 * 1000 ) ) );
	assert( result == WAIT_OBJECT_0 || ( timeOutNanoseconds != SIGNAL_TIMEOUT_INFINITE && result == WAIT_TIMEOUT ) );
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
		if ( timeOutNanoseconds == SIGNAL_TIMEOUT_INFINITE )
		{
			do
			{
				pthread_cond_wait( &signal->cond, &signal->mutex );
				// Must re-check condition because pthread_cond_wait may spuriously wake up.
			} while ( signal->signaled == false );
		}
		else if ( timeOutNanoseconds > 0 )
		{
			struct timeval tp;
			gettimeofday( &tp, NULL );
			struct timespec ts;
			ts.tv_sec = (time_t)( tp.tv_sec + timeOutNanoseconds / ( 1000 * 1000 * 1000 ) );
			ts.tv_nsec = (long)( tp.tv_usec + ( timeOutNanoseconds % ( 1000 * 1000 * 1000 ) ) );
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
static void ksSignal_Raise( ksSignal * signal )
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
static void ksSignal_Clear( ksSignal * signal )
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

ksThread

static bool ksThread_Create( ksThread * thread, const char * threadName, ksThreadFunction threadFunction, void * threadData );
static void ksThread_Destroy( ksThread * thread );
static void ksThread_Signal( ksThread * thread );
static void ksThread_Join( ksThread * thread );
static void ksThread_Submit( ksThread * thread, ksThreadFunction threadFunction, void * threadData );

static void ksThread_SetName( const char * name );
static void ksThread_SetAffinity( int mask );
static void ksThread_SetRealTimePriority( int priority );

================================================================================================================================
*/

typedef void (*ksThreadFunction)( void * data );

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
	ksThreadFunction	threadFunction;
	void *				threadData;

	void *				stack;
	THREAD_HANDLE		handle;
	ksSignal			workIsDone;
	ksSignal			workIsAvailable;
	ksMutex				workMutex;
	volatile bool		terminate;
} ksThread;

// Note that on Android AttachCurrentThread will reset the thread name.
static void ksThread_SetName( const char * name )
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
#elif defined( OS_LINUX )
	pthread_setname_np( pthread_self(), name );
#elif defined( OS_APPLE )
	pthread_setname_np( name );
#elif defined( OS_ANDROID )
	prctl( PR_SET_NAME, (long)name, 0, 0, 0 );
#endif
}

static void ksThread_SetAffinity( int mask )
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
#elif defined( OS_LINUX )
	if ( mask == THREAD_AFFINITY_BIG_CORES )
	{
		return;
	}
	cpu_set_t set;
	memset( &set, 0, sizeof( cpu_set_t ) );
	for ( int bit = 0; bit < 32; bit++ )
	{
		if ( ( mask & ( 1 << bit ) ) != 0 )
		{
			set.__bits[bit / sizeof( set.__bits[0] )] |= 1 << ( bit & ( sizeof( set.__bits[0] ) - 1 ) );
		}
	}
	const int result = pthread_setaffinity_np( pthread_self(), sizeof( cpu_set_t ), &set );
	if ( result != 0 )
	{
		Print( "Failed to set thread %d affinity.\n", (unsigned int)pthread_self() );
	}
	else
	{
		Print( "Thread %d affinity set to 0x%02X\n", (unsigned int)pthread_self(), mask );
	}
#elif defined( OS_APPLE )
	// macOS and iOS do not export interfaces that identify processors or control thread placement.
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

static void ksThread_SetRealTimePriority( int priority )
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
#elif defined( OS_LINUX ) || defined( OS_APPLE )
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
	ksThread * thread = (ksThread *)data;

	ksThread_SetName( thread->threadName );

	for ( ; ; )
	{
		ksMutex_Lock( &thread->workMutex, true );
		if ( ksSignal_Wait( &thread->workIsAvailable, 0 ) )
		{
			ksMutex_Unlock( &thread->workMutex );
		}
		else
		{
			ksSignal_Raise( &thread->workIsDone );
			ksMutex_Unlock( &thread->workMutex );
			ksSignal_Wait( &thread->workIsAvailable, SIGNAL_TIMEOUT_INFINITE );
		}
		if ( thread->terminate )
		{
			ksSignal_Raise( &thread->workIsDone );
			break;
		}
		thread->threadFunction( thread->threadData );
	}
	return THREAD_RETURN_VALUE;
}

static bool ksThread_Create( ksThread * thread, const char * threadName, ksThreadFunction threadFunction, void * threadData )
{
	strncpy( thread->threadName, threadName, sizeof( thread->threadName ) );
	thread->threadName[sizeof( thread->threadName ) - 1] = '\0';
	thread->threadFunction = threadFunction;
	thread->threadData = threadData;
	thread->stack = NULL;
	ksSignal_Create( &thread->workIsDone, false );
	ksSignal_Create( &thread->workIsAvailable, true );
	ksMutex_Create( &thread->workMutex );
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

	ksSignal_Wait( &thread->workIsDone, SIGNAL_TIMEOUT_INFINITE );
	return true;
}

static void ksThread_Destroy( ksThread * thread )
{
	ksMutex_Lock( &thread->workMutex, true );
	ksSignal_Clear( &thread->workIsDone );
	thread->terminate = true;
	ksSignal_Raise( &thread->workIsAvailable );
	ksMutex_Unlock( &thread->workMutex );
	ksSignal_Wait( &thread->workIsDone, SIGNAL_TIMEOUT_INFINITE );
	ksMutex_Destroy( &thread->workMutex );
	ksSignal_Destroy( &thread->workIsDone );
	ksSignal_Destroy( &thread->workIsAvailable );
#if defined( OS_WINDOWS )
	WaitForSingleObject( thread->handle, INFINITE );
	CloseHandle( thread->handle );
#else
	pthread_join( thread->handle, NULL );
#endif
}

static void ksThread_Signal( ksThread * thread )
{
	ksMutex_Lock( &thread->workMutex, true );
	ksSignal_Clear( &thread->workIsDone );
	ksSignal_Raise( &thread->workIsAvailable );
	ksMutex_Unlock( &thread->workMutex );
}

static void ksThread_Join( ksThread * thread )
{
	ksSignal_Wait( &thread->workIsDone, SIGNAL_TIMEOUT_INFINITE );
}

static void ksThread_Submit( ksThread * thread, ksThreadFunction threadFunction, void * threadData )
{
	ksThread_Join( thread );
	thread->threadFunction = threadFunction;
	thread->threadData = threadData;
	ksThread_Signal( thread );
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

Vectors and matrices. All matrices are column-major.

ksVector2i
ksVector3i
ksVector4i
ksVector2f
ksVector3f
ksVector4f
ksQuatf
ksMatrix2x2f
ksMatrix2x3f
ksMatrix2x4f
ksMatrix3x2f
ksMatrix3x3f
ksMatrix3x4f
ksMatrix4x2f
ksMatrix4x3f
ksMatrix4x4f

static void ksVector3f_Set( ksVector3f * v, const float value );
static void ksVector3f_Add( ksVector3f * result, const ksVector3f * a, const ksVector3f * b );
static void ksVector3f_Sub( ksVector3f * result, const ksVector3f * a, const ksVector3f * b );
static void ksVector3f_Min( ksVector3f * result, const ksVector3f * a, const ksVector3f * b );
static void ksVector3f_Max( ksVector3f * result, const ksVector3f * a, const ksVector3f * b );
static void ksVector3f_Decay( ksVector3f * result, const ksVector3f * a, const float value );
static void ksVector3f_Lerp( ksVector3f * result, const ksVector3f * a, const ksVector3f * b, const float fraction );
static void ksVector3f_Normalize( ksVector3f * v );
static float ksVector3f_Length( const ksVector3f * v );

static void ksQuatf_Lerp( ksQuatf * result, const ksQuatf * a, const ksQuatf * b, const float fraction );

static void ksMatrix3x3f_CreateTransposeFromMatrix4x4f( ksMatrix3x3f * result, const ksMatrix4x4f * src );
static void ksMatrix3x4f_CreateFromMatrix4x4f( ksMatrix3x4f * result, const ksMatrix4x4f * src );

static void ksMatrix4x4f_CreateIdentity( ksMatrix4x4f * result );
static void ksMatrix4x4f_CreateTranslation( ksMatrix4x4f * result, const float x, const float y, const float z );
static void ksMatrix4x4f_CreateRotation( ksMatrix4x4f * result, const float degreesX, const float degreesY, const float degreesZ );
static void ksMatrix4x4f_CreateScale( ksMatrix4x4f * result, const float x, const float y, const float z );
static void ksMatrix4x4f_CreateTranslationRotationScale( ksMatrix4x4f * result, const ksVector3f * translation, const ksQuatf * rotation, const ksVector3f * scale );
static void ksMatrix4x4f_CreateProjection( ksMatrix4x4f * result, const float tanAngleLeft, const float tanAngleRight,
											const float tanAngleUp, float const tanAngleDown, const float nearZ, const float farZ );
static void ksMatrix4x4f_CreateProjectionFov( ksMatrix4x4f * result, const float fovDegreesLeft, const float fovDegreesRight,
											const float fovDegreeUp, const float fovDegreesDown, const float nearZ, const float farZ );
static void ksMatrix4x4f_CreateFromQuaternion( ksMatrix3x4f * result, const ksQuatf * src );
static void ksMatrix4x4f_CreateOffsetScaleForBounds( ksMatrix4x4f * result, const ksMatrix4x4f * matrix, const ksVector3f * mins, const ksVector3f * maxs );

static bool ksMatrix4x4f_IsAffine( const ksMatrix4x4f * matrix, const float epsilon );
static bool ksMatrix4x4f_IsOrthogonal( const ksMatrix4x4f * matrix, const float epsilon );
static bool ksMatrix4x4f_IsOrthonormal( const ksMatrix4x4f * matrix, const float epsilon );
static bool ksMatrix4x4f_IsHomogeneous( const ksMatrix4x4f * matrix, const float epsilon );

static void ksMatrix4x4f_GetTranslation( ksVector3f * result, const ksMatrix4x4f * src );
static void ksMatrix4x4f_GetRotation( ksQuatf * result, const ksMatrix4x4f * src );
static void ksMatrix4x4f_GetScale( ksVector3f * result, const ksMatrix4x4f * src );

static void ksMatrix4x4f_Multiply( ksMatrix4x4f * result, const ksMatrix4x4f * a, const ksMatrix4x4f * b );
static void ksMatrix4x4f_Transpose( ksMatrix4x4f * result, const ksMatrix4x4f * src );
static void ksMatrix4x4f_Invert( ksMatrix4x4f * result, const ksMatrix4x4f * src );
static void ksMatrix4x4f_InvertHomogeneous( ksMatrix4x4f * result, const ksMatrix4x4f * src );

static void ksMatrix4x4f_TransformVector3f( ksVector3f * result, const ksMatrix4x4f * m, const ksVector3f * v );
static void ksMatrix4x4f_TransformVector4f( ksVector4f * result, const ksMatrix4x4f * m, const ksVector4f * v );

static void ksMatrix4x4f_TransformBounds( ksVector3f * resultMins, ksVector3f * resultMaxs, const ksMatrix4x4f * matrix, const ksVector3f * mins, const ksVector3f * maxs );
static bool ksMatrix4x4f_CullBounds( const ksMatrix4x4f * mvp, const ksVector3f * mins, const ksVector3f * maxs );

================================================================================================================================
*/

#define DEFAULT_NEAR_Z		0.015625f		// exact floating point representation
#define INFINITE_FAR_Z		0.0f

// 2D integer vector
typedef struct
{
	int x;
	int y;
} ksVector2i;

// 3D integer vector
typedef struct
{
	int x;
	int y;
	int z;
} ksVector3i;

// 4D integer vector
typedef struct
{
	int x;
	int y;
	int z;
	int w;
} ksVector4i;

// 2D float vector
typedef struct
{
	float x;
	float y;
} ksVector2f;

// 3D float vector
typedef struct
{
	float x;
	float y;
	float z;
} ksVector3f;

// 4D float vector
typedef struct
{
	float x;
	float y;
	float z;
	float w;
} ksVector4f;

// Quaternion
typedef struct
{
	float x;
	float y;
	float z;
	float w;
} ksQuatf;

// Column-major 2x2 matrix
typedef struct
{
	float m[2][2];
} ksMatrix2x2f;

// Column-major 2x3 matrix
typedef struct
{
	float m[2][3];
} ksMatrix2x3f;

// Column-major 2x4 matrix
typedef struct
{
	float m[2][4];
} ksMatrix2x4f;

// Column-major 3x2 matrix
typedef struct
{
	float m[3][2];
} ksMatrix3x2f;

// Column-major 3x3 matrix
typedef struct
{
	float m[3][3];
} ksMatrix3x3f;

// Column-major 3x4 matrix
typedef struct
{
	float m[3][4];
} ksMatrix3x4f;

// Column-major 4x2 matrix
typedef struct
{
	float m[4][2];
} ksMatrix4x2f;

// Column-major 4x3 matrix
typedef struct
{
	float m[4][3];
} ksMatrix4x3f;

// Column-major 4x4 matrix
typedef struct
{
	float m[4][4];
} ksMatrix4x4f;

static const ksVector4f colorRed		= { 1.0f, 0.0f, 0.0f, 1.0f };
static const ksVector4f colorGreen		= { 0.0f, 1.0f, 0.0f, 1.0f };
static const ksVector4f colorBlue		= { 0.0f, 0.0f, 1.0f, 1.0f };
static const ksVector4f colorYellow		= { 1.0f, 1.0f, 0.0f, 1.0f };
static const ksVector4f colorPurple		= { 1.0f, 0.0f, 1.0f, 1.0f };
static const ksVector4f colorCyan		= { 0.0f, 1.0f, 1.0f, 1.0f };
static const ksVector4f colorLightGrey	= { 0.7f, 0.7f, 0.7f, 1.0f };
static const ksVector4f colorDarkGrey	= { 0.3f, 0.3f, 0.3f, 1.0f };

static float RcpSqrt( const float x )
{
	const float SMALLEST_NON_DENORMAL = 1.1754943508222875e-038f;	// ( 1U << 23 )
	const float rcp = ( x >= SMALLEST_NON_DENORMAL ) ? 1.0f / sqrtf( x ) : 1.0f;
	return rcp;
}

static void ksVector3f_Set( ksVector3f * v, const float value )
{
	v->x = value;
	v->y = value;
	v->z = value;
}

static void ksVector3f_Add( ksVector3f * result, const ksVector3f * a, const ksVector3f * b )
{
	result->x = a->x + b->x;
	result->y = a->y + b->y;
	result->z = a->z + b->z;
}

static void ksVector3f_Sub( ksVector3f * result, const ksVector3f * a, const ksVector3f * b )
{
	result->x = a->x - b->x;
	result->y = a->y - b->y;
	result->z = a->z - b->z;
}

static void ksVector3f_Min( ksVector3f * result, const ksVector3f * a, const ksVector3f * b )
{
	result->x = ( a->x < b->x ) ? a->x : b->x;
	result->y = ( a->y < b->y ) ? a->y : b->y;
	result->z = ( a->z < b->z ) ? a->z : b->z;
}

static void ksVector3f_Max( ksVector3f * result, const ksVector3f * a, const ksVector3f * b )
{
	result->x = ( a->x > b->x ) ? a->x : b->x;
	result->y = ( a->y > b->y ) ? a->y : b->y;
	result->z = ( a->z > b->z ) ? a->z : b->z;
}

static void ksVector3f_Decay( ksVector3f * result, const ksVector3f * a, const float value )
{
	result->x = ( fabsf( a->x ) > value ) ? ( ( a->x > 0.0f ) ? ( a->x - value ) : ( a->x + value ) ) : 0.0f;
	result->y = ( fabsf( a->y ) > value ) ? ( ( a->y > 0.0f ) ? ( a->y - value ) : ( a->y + value ) ) : 0.0f;
	result->z = ( fabsf( a->z ) > value ) ? ( ( a->z > 0.0f ) ? ( a->z - value ) : ( a->z + value ) ) : 0.0f;
}

static void ksVector3f_Lerp( ksVector3f * result, const ksVector3f * a, const ksVector3f * b, const float fraction )
{
	result->x = a->x + fraction * ( b->x - a->x );
	result->y = a->y + fraction * ( b->y - a->y );
	result->z = a->z + fraction * ( b->z - a->z );
}

static void ksVector3f_Normalize( ksVector3f * v )
{
	const float lengthRcp = RcpSqrt( v->x * v->x + v->y * v->y + v->z * v->z );
	v->x *= lengthRcp;
	v->y *= lengthRcp;
	v->z *= lengthRcp;
}

static float ksVector3f_Length( const ksVector3f * v )
{
	return sqrtf( v->x * v->x + v->y * v->y + v->z * v->z );
}

static void ksQuatf_Lerp( ksQuatf * result, const ksQuatf * a, const ksQuatf * b, const float fraction )
{
	const float s = a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
	const float fa = 1.0f - fraction;
	const float fb = ( s < 0.0f ) ? -fraction : fraction;
	const float x = a->x * fa + b->x * fb;
	const float y = a->y * fa + b->y * fb;
	const float z = a->z * fa + b->z * fb;
	const float w = a->w * fa + b->w * fb;
	const float lengthRcp = RcpSqrt( x * x + y * y + z * z + w * w );
	result->x = x * lengthRcp;
	result->y = y * lengthRcp;
	result->z = z * lengthRcp;
	result->w = w * lengthRcp;
}

static void ksMatrix3x3f_CreateTransposeFromMatrix4x4f( ksMatrix3x3f * result, const ksMatrix4x4f * src )
{
	result->m[0][0] = src->m[0][0];
	result->m[0][1] = src->m[1][0];
	result->m[0][2] = src->m[2][0];

	result->m[1][0] = src->m[0][1];
	result->m[1][1] = src->m[1][1];
	result->m[1][2] = src->m[2][1];

	result->m[2][0] = src->m[0][2];
	result->m[2][1] = src->m[1][2];
	result->m[2][2] = src->m[2][2];
}

static void ksMatrix3x4f_CreateFromMatrix4x4f( ksMatrix3x4f * result, const ksMatrix4x4f * src )
{
	result->m[0][0] = src->m[0][0];
	result->m[0][1] = src->m[1][0];
	result->m[0][2] = src->m[2][0];
	result->m[0][3] = src->m[3][0];
	result->m[1][0] = src->m[0][1];
	result->m[1][1] = src->m[1][1];
	result->m[1][2] = src->m[2][1];
	result->m[1][3] = src->m[3][1];
	result->m[2][0] = src->m[0][2];
	result->m[2][1] = src->m[1][2];
	result->m[2][2] = src->m[2][2];
	result->m[2][3] = src->m[3][2];
}

// Use left-multiplication to accumulate transformations.
static void ksMatrix4x4f_Multiply( ksMatrix4x4f * result, const ksMatrix4x4f * a, const ksMatrix4x4f * b )
{
	result->m[0][0] = a->m[0][0] * b->m[0][0] + a->m[1][0] * b->m[0][1] + a->m[2][0] * b->m[0][2] + a->m[3][0] * b->m[0][3];
	result->m[0][1] = a->m[0][1] * b->m[0][0] + a->m[1][1] * b->m[0][1] + a->m[2][1] * b->m[0][2] + a->m[3][1] * b->m[0][3];
	result->m[0][2] = a->m[0][2] * b->m[0][0] + a->m[1][2] * b->m[0][1] + a->m[2][2] * b->m[0][2] + a->m[3][2] * b->m[0][3];
	result->m[0][3] = a->m[0][3] * b->m[0][0] + a->m[1][3] * b->m[0][1] + a->m[2][3] * b->m[0][2] + a->m[3][3] * b->m[0][3];

	result->m[1][0] = a->m[0][0] * b->m[1][0] + a->m[1][0] * b->m[1][1] + a->m[2][0] * b->m[1][2] + a->m[3][0] * b->m[1][3];
	result->m[1][1] = a->m[0][1] * b->m[1][0] + a->m[1][1] * b->m[1][1] + a->m[2][1] * b->m[1][2] + a->m[3][1] * b->m[1][3];
	result->m[1][2] = a->m[0][2] * b->m[1][0] + a->m[1][2] * b->m[1][1] + a->m[2][2] * b->m[1][2] + a->m[3][2] * b->m[1][3];
	result->m[1][3] = a->m[0][3] * b->m[1][0] + a->m[1][3] * b->m[1][1] + a->m[2][3] * b->m[1][2] + a->m[3][3] * b->m[1][3];

	result->m[2][0] = a->m[0][0] * b->m[2][0] + a->m[1][0] * b->m[2][1] + a->m[2][0] * b->m[2][2] + a->m[3][0] * b->m[2][3];
	result->m[2][1] = a->m[0][1] * b->m[2][0] + a->m[1][1] * b->m[2][1] + a->m[2][1] * b->m[2][2] + a->m[3][1] * b->m[2][3];
	result->m[2][2] = a->m[0][2] * b->m[2][0] + a->m[1][2] * b->m[2][1] + a->m[2][2] * b->m[2][2] + a->m[3][2] * b->m[2][3];
	result->m[2][3] = a->m[0][3] * b->m[2][0] + a->m[1][3] * b->m[2][1] + a->m[2][3] * b->m[2][2] + a->m[3][3] * b->m[2][3];

	result->m[3][0] = a->m[0][0] * b->m[3][0] + a->m[1][0] * b->m[3][1] + a->m[2][0] * b->m[3][2] + a->m[3][0] * b->m[3][3];
	result->m[3][1] = a->m[0][1] * b->m[3][0] + a->m[1][1] * b->m[3][1] + a->m[2][1] * b->m[3][2] + a->m[3][1] * b->m[3][3];
	result->m[3][2] = a->m[0][2] * b->m[3][0] + a->m[1][2] * b->m[3][1] + a->m[2][2] * b->m[3][2] + a->m[3][2] * b->m[3][3];
	result->m[3][3] = a->m[0][3] * b->m[3][0] + a->m[1][3] * b->m[3][1] + a->m[2][3] * b->m[3][2] + a->m[3][3] * b->m[3][3];
}

// Creates the transpose of the given matrix.
static void ksMatrix4x4f_Transpose( ksMatrix4x4f * result, const ksMatrix4x4f * src )
{
	result->m[0][0] = src->m[0][0];
	result->m[0][1] = src->m[1][0];
	result->m[0][2] = src->m[2][0];
	result->m[0][3] = src->m[3][0];

	result->m[1][0] = src->m[0][1];
	result->m[1][1] = src->m[1][1];
	result->m[1][2] = src->m[2][1];
	result->m[1][3] = src->m[3][1];

	result->m[2][0] = src->m[0][2];
	result->m[2][1] = src->m[1][2];
	result->m[2][2] = src->m[2][2];
	result->m[2][3] = src->m[3][2];

	result->m[3][0] = src->m[0][3];
	result->m[3][1] = src->m[1][3];
	result->m[3][2] = src->m[2][3];
	result->m[3][3] = src->m[3][3];
}

// Returns a 3x3 minor of a 4x4 matrix.
static float ksMatrix4x4f_Minor( const ksMatrix4x4f * matrix, int r0, int r1, int r2, int c0, int c1, int c2 )
{
	return	matrix->m[r0][c0] * ( matrix->m[r1][c1] * matrix->m[r2][c2] - matrix->m[r2][c1] * matrix->m[r1][c2] ) -
			matrix->m[r0][c1] * ( matrix->m[r1][c0] * matrix->m[r2][c2] - matrix->m[r2][c0] * matrix->m[r1][c2] ) +
			matrix->m[r0][c2] * ( matrix->m[r1][c0] * matrix->m[r2][c1] - matrix->m[r2][c0] * matrix->m[r1][c1] );
}
 
// Calculates the inverse of a 4x4 matrix.
static void ksMatrix4x4f_Invert( ksMatrix4x4f * result, const ksMatrix4x4f * src )
{
	const float rcpDet = 1.0f / (	src->m[0][0] * ksMatrix4x4f_Minor( src, 1, 2, 3, 1, 2, 3 ) -
									src->m[0][1] * ksMatrix4x4f_Minor( src, 1, 2, 3, 0, 2, 3 ) +
									src->m[0][2] * ksMatrix4x4f_Minor( src, 1, 2, 3, 0, 1, 3 ) -
									src->m[0][3] * ksMatrix4x4f_Minor( src, 1, 2, 3, 0, 1, 2 ) );

	result->m[0][0] =  ksMatrix4x4f_Minor( src, 1, 2, 3, 1, 2, 3 ) * rcpDet;
	result->m[0][1] = -ksMatrix4x4f_Minor( src, 0, 2, 3, 1, 2, 3 ) * rcpDet;
	result->m[0][2] =  ksMatrix4x4f_Minor( src, 0, 1, 3, 1, 2, 3 ) * rcpDet;
	result->m[0][3] = -ksMatrix4x4f_Minor( src, 0, 1, 2, 1, 2, 3 ) * rcpDet;
	result->m[1][0] = -ksMatrix4x4f_Minor( src, 1, 2, 3, 0, 2, 3 ) * rcpDet;
	result->m[1][1] =  ksMatrix4x4f_Minor( src, 0, 2, 3, 0, 2, 3 ) * rcpDet;
	result->m[1][2] = -ksMatrix4x4f_Minor( src, 0, 1, 3, 0, 2, 3 ) * rcpDet;
	result->m[1][3] =  ksMatrix4x4f_Minor( src, 0, 1, 2, 0, 2, 3 ) * rcpDet;
	result->m[2][0] =  ksMatrix4x4f_Minor( src, 1, 2, 3, 0, 1, 3 ) * rcpDet;
	result->m[2][1] = -ksMatrix4x4f_Minor( src, 0, 2, 3, 0, 1, 3 ) * rcpDet;
	result->m[2][2] =  ksMatrix4x4f_Minor( src, 0, 1, 3, 0, 1, 3 ) * rcpDet;
	result->m[2][3] = -ksMatrix4x4f_Minor( src, 0, 1, 2, 0, 1, 3 ) * rcpDet;
	result->m[3][0] = -ksMatrix4x4f_Minor( src, 1, 2, 3, 0, 1, 2 ) * rcpDet;
	result->m[3][1] =  ksMatrix4x4f_Minor( src, 0, 2, 3, 0, 1, 2 ) * rcpDet;
	result->m[3][2] = -ksMatrix4x4f_Minor( src, 0, 1, 3, 0, 1, 2 ) * rcpDet;
	result->m[3][3] =  ksMatrix4x4f_Minor( src, 0, 1, 2, 0, 1, 2 ) * rcpDet;
}

// Calculates the inverse of a 4x4 homogeneous matrix.
static void ksMatrix4x4f_InvertHomogeneous( ksMatrix4x4f * result, const ksMatrix4x4f * src )
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
static void ksMatrix4x4f_CreateIdentity( ksMatrix4x4f * result )
{
	result->m[0][0] = 1.0f; result->m[0][1] = 0.0f; result->m[0][2] = 0.0f; result->m[0][3] = 0.0f;
	result->m[1][0] = 0.0f; result->m[1][1] = 1.0f; result->m[1][2] = 0.0f; result->m[1][3] = 0.0f;
	result->m[2][0] = 0.0f; result->m[2][1] = 0.0f; result->m[2][2] = 1.0f; result->m[2][3] = 0.0f;
	result->m[3][0] = 0.0f; result->m[3][1] = 0.0f; result->m[3][2] = 0.0f; result->m[3][3] = 1.0f;
}

// Creates a translation matrix.
static void ksMatrix4x4f_CreateTranslation( ksMatrix4x4f * result, const float x, const float y, const float z )
{
	result->m[0][0] = 1.0f; result->m[0][1] = 0.0f; result->m[0][2] = 0.0f; result->m[0][3] = 0.0f;
	result->m[1][0] = 0.0f; result->m[1][1] = 1.0f; result->m[1][2] = 0.0f; result->m[1][3] = 0.0f;
	result->m[2][0] = 0.0f; result->m[2][1] = 0.0f; result->m[2][2] = 1.0f; result->m[2][3] = 0.0f;
	result->m[3][0] =    x; result->m[3][1] =    y; result->m[3][2] =    z; result->m[3][3] = 1.0f;
}

// Creates a rotation matrix.
// If -Z=forward, +Y=up, +X=right, then degreesX=pitch, degreesY=yaw, degreesZ=roll.
static void ksMatrix4x4f_CreateRotation( ksMatrix4x4f * result, const float degreesX, const float degreesY, const float degreesZ )
{
	const float sinX = sinf( degreesX * ( MATH_PI / 180.0f ) );
	const float cosX = cosf( degreesX * ( MATH_PI / 180.0f ) );
	const ksMatrix4x4f rotationX =
	{ {
		{ 1,     0,    0, 0 },
		{ 0,  cosX, sinX, 0 },
		{ 0, -sinX, cosX, 0 },
		{ 0,     0,    0, 1 }
	} };
	const float sinY = sinf( degreesY * ( MATH_PI / 180.0f ) );
	const float cosY = cosf( degreesY * ( MATH_PI / 180.0f ) );
	const ksMatrix4x4f rotationY =
	{ {
		{ cosY, 0, -sinY, 0 },
		{    0, 1,     0, 0 },
		{ sinY, 0,  cosY, 0 },
		{    0, 0,     0, 1 }
	} };
	const float sinZ = sinf( degreesZ * ( MATH_PI / 180.0f ) );
	const float cosZ = cosf( degreesZ * ( MATH_PI / 180.0f ) );
	const ksMatrix4x4f rotationZ =
	{ {
		{  cosZ, sinZ, 0, 0 },
		{ -sinZ, cosZ, 0, 0 },
		{     0,    0, 1, 0 },
		{     0,    0, 0, 1 }
	} };
	ksMatrix4x4f rotationXY;
	ksMatrix4x4f_Multiply( &rotationXY, &rotationY, &rotationX );
	ksMatrix4x4f_Multiply( result, &rotationZ, &rotationXY );
}

// Creates a scale matrix.
static void ksMatrix4x4f_CreateScale( ksMatrix4x4f * result, const float x, const float y, const float z )
{
	result->m[0][0] =    x; result->m[0][1] = 0.0f; result->m[0][2] = 0.0f; result->m[0][3] = 0.0f;
	result->m[1][0] = 0.0f; result->m[1][1] =    y; result->m[1][2] = 0.0f; result->m[1][3] = 0.0f;
	result->m[2][0] = 0.0f; result->m[2][1] = 0.0f; result->m[2][2] =    z; result->m[2][3] = 0.0f;
	result->m[3][0] = 0.0f; result->m[3][1] = 0.0f; result->m[3][2] = 0.0f; result->m[3][3] = 1.0f;
}

// Creates a matrix from a quaternion.
static void ksMatrix4x4f_CreateFromQuaternion( ksMatrix4x4f * result, const ksQuatf * quat )
{
	const float x2 = quat->x + quat->x;
	const float y2 = quat->y + quat->y;
	const float z2 = quat->z + quat->z;

	const float xx2 = quat->x * x2;
	const float yy2 = quat->y * y2;
	const float zz2 = quat->z * z2;

	const float yz2 = quat->y * z2;
	const float wx2 = quat->w * x2;
	const float xy2 = quat->x * y2;
	const float wz2 = quat->w * z2;
	const float xz2 = quat->x * z2;
	const float wy2 = quat->w * y2;

	result->m[0][0] = 1.0f - yy2 - zz2;
	result->m[0][1] = xy2 + wz2;
	result->m[0][2] = xz2 - wy2;
	result->m[0][3] = 0.0f;

	result->m[1][0] = xy2 - wz2;
	result->m[1][1] = 1.0f - xx2 - zz2;
	result->m[1][2] = yz2 + wx2;
	result->m[1][3] = 0.0f;

	result->m[2][0] = xz2 + wy2;
	result->m[2][1] = yz2 - wx2;
	result->m[2][2] = 1.0f - xx2 - yy2;
	result->m[2][3] = 0.0f;

	result->m[3][0] = 0.0f;
	result->m[3][1] = 0.0f;
	result->m[3][2] = 0.0f;
	result->m[3][3] = 1.0f;
}

// Creates a combined translation(rotation(scale(object))) matrix.
static void ksMatrix4x4f_CreateTranslationRotationScale( ksMatrix4x4f * result, const ksVector3f * translation, const ksQuatf * rotation, const ksVector3f * scale )
{
	ksMatrix4x4f scaleMatrix;
	ksMatrix4x4f_CreateScale( &scaleMatrix, scale->x, scale->y, scale->z );

	ksMatrix4x4f rotationMatrix;
	ksMatrix4x4f_CreateFromQuaternion( &rotationMatrix, rotation );

	ksMatrix4x4f translationMatrix;
	ksMatrix4x4f_CreateTranslation( &translationMatrix, translation->x, translation->y, translation->z );

	ksMatrix4x4f combinedMatrix;
	ksMatrix4x4f_Multiply( &combinedMatrix, &rotationMatrix, &scaleMatrix );
	ksMatrix4x4f_Multiply( result, &translationMatrix, &combinedMatrix );
}

// Creates a projection matrix based on the specified dimensions.
// The projection matrix transforms -Z=forward, +Y=up, +X=right to the appropriate clip space for the graphics API.
// The far plane is placed at infinity if farZ <= nearZ.
// An infinite projection matrix is preferred for rasterization because, except for
// things *right* up against the near plane, it always provides better precision:
//		"Tightening the Precision of Perspective Rendering"
//		Paul Upchurch, Mathieu Desbrun
//		Journal of Graphics Tools, Volume 16, Issue 1, 2012
static void ksMatrix4x4f_CreateProjection( ksMatrix4x4f * result, const float tanAngleLeft, const float tanAngleRight,
											const float tanAngleUp, float const tanAngleDown, const float nearZ, const float farZ )
{
	const float tanAngleWidth = tanAngleRight - tanAngleLeft;

#if GRAPHICS_API_VULKAN == 1
	// Set to tanAngleDown - tanAngleUp for a clip space with positive Y down (Vulkan).
	const float tanAngleHeight = tanAngleDown - tanAngleUp;
#else
	// Set to tanAngleUp - tanAngleDown for a clip space with positive Y up (OpenGL / D3D / Metal).
	const float tanAngleHeight = tanAngleUp - tanAngleDown;
#endif

#if GRAPHICS_API_OPENGL == 1 || GRAPHICS_API_OPENGL_ES == 1
	// Set to nearZ for a [-1,1] Z clip space (OpenGL).
	const float offsetZ = nearZ;
#else
	// Set to zero for a [0,1] Z clip space (Vulkan / D3D / Metal).
	const float offsetZ = 0;
#endif

	if ( farZ <= nearZ )
	{
		// place the far plane at infinity
		result->m[0][0] = 2 / tanAngleWidth;
		result->m[1][0] = 0;
		result->m[2][0] = ( tanAngleRight + tanAngleLeft ) / tanAngleWidth;
		result->m[3][0] = 0;

		result->m[0][1] = 0;
		result->m[1][1] = 2 / tanAngleHeight;
		result->m[2][1] = ( tanAngleUp + tanAngleDown ) / tanAngleHeight;
		result->m[3][1] = 0;

		result->m[0][2] = 0;
		result->m[1][2] = 0;
		result->m[2][2] = -1;
		result->m[3][2] = -( nearZ + offsetZ );

		result->m[0][3] = 0;
		result->m[1][3] = 0;
		result->m[2][3] = -1;
		result->m[3][3] = 0;
	}
	else
	{
		// normal projection
		result->m[0][0] = 2 / tanAngleWidth;
		result->m[1][0] = 0;
		result->m[2][0] = ( tanAngleRight + tanAngleLeft ) / tanAngleWidth;
		result->m[3][0] = 0;

		result->m[0][1] = 0;
		result->m[1][1] = 2 / tanAngleHeight;
		result->m[2][1] = ( tanAngleUp + tanAngleDown ) / tanAngleHeight;
		result->m[3][1] = 0;

		result->m[0][2] = 0;
		result->m[1][2] = 0;
		result->m[2][2] = -( farZ + offsetZ ) / ( farZ - nearZ );
		result->m[3][2] = -( farZ * ( nearZ + offsetZ ) ) / ( farZ - nearZ );

		result->m[0][3] = 0;
		result->m[1][3] = 0;
		result->m[2][3] = -1;
		result->m[3][3] = 0;
	}
}

// Creates a projection matrix based on the specified FOV.
static void ksMatrix4x4f_CreateProjectionFov( ksMatrix4x4f * result, const float fovDegreesLeft, const float fovDegreesRight,
												const float fovDegreesUp, const float fovDegreesDown, const float nearZ, const float farZ )
{
	const float tanLeft = - tanf( fovDegreesLeft * ( MATH_PI / 180.0f ) );
	const float tanRight = tanf( fovDegreesRight * ( MATH_PI / 180.0f ) );

	const float tanDown = - tanf( fovDegreesDown * ( MATH_PI / 180.0f ) );
	const float tanUp = tanf( fovDegreesUp * ( MATH_PI / 180.0f ) );

	ksMatrix4x4f_CreateProjection( result, tanLeft, tanRight, tanUp, tanDown, nearZ, farZ );
}

// Creates a matrix that transforms the -1 to 1 cube to cover the given 'mins' and 'maxs' transformed with the given 'matrix'.
static void ksMatrix4x4f_CreateOffsetScaleForBounds( ksMatrix4x4f * result, const ksMatrix4x4f * matrix, const ksVector3f * mins, const ksVector3f * maxs )
{
	const ksVector3f offset = { ( maxs->x + mins->x ) * 0.5f, ( maxs->y + mins->y ) * 0.5f, ( maxs->z + mins->z ) * 0.5f };
	const ksVector3f scale = { ( maxs->x - mins->x ) * 0.5f, ( maxs->y - mins->y ) * 0.5f, ( maxs->z - mins->z ) * 0.5f };

	result->m[0][0] = matrix->m[0][0] * scale.x;
	result->m[0][1] = matrix->m[0][1] * scale.x;
	result->m[0][2] = matrix->m[0][2] * scale.x;
	result->m[0][3] = matrix->m[0][3] * scale.x;

	result->m[1][0] = matrix->m[1][0] * scale.y;
	result->m[1][1] = matrix->m[1][1] * scale.y;
	result->m[1][2] = matrix->m[1][2] * scale.y;
	result->m[1][3] = matrix->m[1][3] * scale.y;

	result->m[2][0] = matrix->m[2][0] * scale.z;
	result->m[2][1] = matrix->m[2][1] * scale.z;
	result->m[2][2] = matrix->m[2][2] * scale.z;
	result->m[2][3] = matrix->m[2][3] * scale.z;

	result->m[3][0] = matrix->m[3][0] + matrix->m[0][0] * offset.x + matrix->m[1][0] * offset.y + matrix->m[2][0] * offset.z;
	result->m[3][1] = matrix->m[3][1] + matrix->m[0][1] * offset.x + matrix->m[1][1] * offset.y + matrix->m[2][1] * offset.z;
	result->m[3][2] = matrix->m[3][2] + matrix->m[0][2] * offset.x + matrix->m[1][2] * offset.y + matrix->m[2][2] * offset.z;
	result->m[3][3] = matrix->m[3][3] + matrix->m[0][3] * offset.x + matrix->m[1][3] * offset.y + matrix->m[2][3] * offset.z;
}

// Returns true if the given matrix is affine.
static bool ksMatrix4x4f_IsAffine( const ksMatrix4x4f * matrix, const float epsilon )
{
	return	fabsf( matrix->m[0][3] ) <= epsilon &&
			fabsf( matrix->m[1][3] ) <= epsilon &&
			fabsf( matrix->m[2][3] ) <= epsilon &&
			fabsf( matrix->m[3][3] - 1.0f ) <= epsilon;
}

// Returns true if the given matrix is orthogonal.
static bool ksMatrix4x4f_IsOrthogonal( const ksMatrix4x4f * matrix, const float epsilon )
{
	for ( int i = 0; i < 3; i++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			if ( i != j )
			{
				if ( fabsf( matrix->m[i][0] * matrix->m[j][0] + matrix->m[i][1] * matrix->m[j][1] + matrix->m[i][2] * matrix->m[j][2] ) > epsilon )
				{
					return false;
				}
				if ( fabsf( matrix->m[0][i] * matrix->m[0][j] + matrix->m[1][i] * matrix->m[1][j] + matrix->m[2][i] * matrix->m[2][j] ) > epsilon )
				{
					return false;
				}
			}
		}
	}
	return true;
}

// Returns true if the given matrix is orthonormal.
static bool ksMatrix4x4f_IsOrthonormal( const ksMatrix4x4f * matrix, const float epsilon )
{
	for ( int i = 0; i < 3; i++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			const float kd = ( i == j ) ? 1.0f : 0.0f;	// Kronecker delta
			if ( fabsf( kd - ( matrix->m[i][0] * matrix->m[j][0] + matrix->m[i][1] * matrix->m[j][1] + matrix->m[i][2] * matrix->m[j][2] ) ) > epsilon )
			{
				return false;
			}
			if ( fabsf( kd - ( matrix->m[0][i] * matrix->m[0][j] + matrix->m[1][i] * matrix->m[1][j] + matrix->m[2][i] * matrix->m[2][j] ) ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

// Returns true if the given matrix is homogeneous.
static bool ksMatrix4x4f_IsHomogeneous( const ksMatrix4x4f * matrix, const float epsilon )
{
	return ksMatrix4x4f_IsAffine( matrix, epsilon ) && ksMatrix4x4f_IsOrthonormal( matrix, epsilon );
}

// Get the translation from a combined translation(rotation(scale(object))) matrix.
static void ksMatrix4x4f_GetTranslation( ksVector3f * result, const ksMatrix4x4f * src )
{
	assert( ksMatrix4x4f_IsAffine( src, 1e-4f ) );
	assert( ksMatrix4x4f_IsOrthogonal( src, 1e-4f ) );

	result->x = src->m[3][0];
	result->y = src->m[3][1];
	result->z = src->m[3][2];
}

// Get the rotation from a combined translation(rotation(scale(object))) matrix.
static void ksMatrix4x4f_GetRotation( ksQuatf * result, const ksMatrix4x4f * src )
{
	assert( ksMatrix4x4f_IsAffine( src, 1e-4f ) );
	assert( ksMatrix4x4f_IsOrthogonal( src, 1e-4f ) );

	const float scaleX = RcpSqrt( src->m[0][0] * src->m[0][0] + src->m[0][1] * src->m[0][1] + src->m[0][2] * src->m[0][2] );
	const float scaleY = RcpSqrt( src->m[1][0] * src->m[1][0] + src->m[1][1] * src->m[1][1] + src->m[1][2] * src->m[1][2] );
	const float scaleZ = RcpSqrt( src->m[2][0] * src->m[2][0] + src->m[2][1] * src->m[2][1] + src->m[2][2] * src->m[2][2] );
	const float m[9] =
	{
		src->m[0][0] * scaleX, src->m[0][1] * scaleX, src->m[0][2] * scaleX,
		src->m[1][0] * scaleY, src->m[1][1] * scaleY, src->m[1][2] * scaleY,
		src->m[2][0] * scaleZ, src->m[2][1] * scaleZ, src->m[2][2] * scaleZ
	};
	if ( m[0 * 3 + 0] + m[1 * 3 + 1] + m[2 * 3 + 2] > 0.0f )
	{
		float t = + m[0 * 3 + 0] + m[1 * 3 + 1] + m[2 * 3 + 2] + 1.0f;
		float s = RcpSqrt( t ) * 0.5f;
		result->w = s * t;
		result->z = ( m[0 * 3 + 1] - m[1 * 3 + 0] ) * s;
		result->y = ( m[2 * 3 + 0] - m[0 * 3 + 2] ) * s;
		result->x = ( m[1 * 3 + 2] - m[2 * 3 + 1] ) * s;
	}
	else if ( m[0 * 3 + 0] > m[1 * 3 + 1] && m[0 * 3 + 0] > m[2 * 3 + 2] )
	{
		float t = + m[0 * 3 + 0] - m[1 * 3 + 1] - m[2 * 3 + 2] + 1.0f;
		float s = RcpSqrt( t ) * 0.5f;
		result->x = s * t;
		result->y = ( m[0 * 3 + 1] + m[1 * 3 + 0] ) * s; 
		result->z = ( m[2 * 3 + 0] + m[0 * 3 + 2] ) * s;
		result->w = ( m[1 * 3 + 2] - m[2 * 3 + 1] ) * s;
	}
	else if ( m[1 * 3 + 1] > m[2 * 3 + 2] )
	{
		float t = - m[0 * 3 + 0] + m[1 * 3 + 1] - m[2 * 3 + 2] + 1.0f;
		float s = RcpSqrt( t ) * 0.5f;
		result->y = s * t;
		result->x = ( m[0 * 3 + 1] + m[1 * 3 + 0] ) * s;
		result->w = ( m[2 * 3 + 0] - m[0 * 3 + 2] ) * s;
		result->z = ( m[1 * 3 + 2] + m[2 * 3 + 1] ) * s;
	}
	else
	{
		float t = - m[0 * 3 + 0] - m[1 * 3 + 1] + m[2 * 3 + 2] + 1.0f;
		float s = RcpSqrt( t ) * 0.5f;
		result->z = s * t;
		result->w = ( m[0 * 3 + 1] - m[1 * 3 + 0] ) * s;
		result->x = ( m[2 * 3 + 0] + m[0 * 3 + 2] ) * s;
		result->y = ( m[1 * 3 + 2] + m[2 * 3 + 1] ) * s;
	}
}

// Get the scale from a combined translation(rotation(scale(object))) matrix.
static void ksMatrix4x4f_GetScale( ksVector3f * result, const ksMatrix4x4f * src )
{
	assert( ksMatrix4x4f_IsAffine( src, 1e-4f ) );
	assert( ksMatrix4x4f_IsOrthogonal( src, 1e-4f ) );

	result->x = sqrtf( src->m[0][0] * src->m[0][0] + src->m[0][1] * src->m[0][1] + src->m[0][2] * src->m[0][2] );
	result->y = sqrtf( src->m[1][0] * src->m[1][0] + src->m[1][1] * src->m[1][1] + src->m[1][2] * src->m[1][2] );
	result->z = sqrtf( src->m[2][0] * src->m[2][0] + src->m[2][1] * src->m[2][1] + src->m[2][2] * src->m[2][2] );
}

// Transforms a 3D vector.
static void ksMatrix4x4f_TransformVector3f( ksVector3f * result, const ksMatrix4x4f * m, const ksVector3f * v )
{
	const float w = m->m[0][3] * v->x + m->m[1][3] * v->y + m->m[2][3] * v->z + m->m[3][3];
	const float rcpW = 1.0f / w;
	result->x = ( m->m[0][0] * v->x + m->m[1][0] * v->y + m->m[2][0] * v->z + m->m[3][0] ) * rcpW;
	result->y = ( m->m[0][1] * v->x + m->m[1][1] * v->y + m->m[2][1] * v->z + m->m[3][1] ) * rcpW;
	result->z = ( m->m[0][2] * v->x + m->m[1][2] * v->y + m->m[2][2] * v->z + m->m[3][2] ) * rcpW;
}

// Transforms a 4D vector.
static void ksMatrix4x4f_TransformVector4f( ksVector4f * result, const ksMatrix4x4f * m, const ksVector4f * v )
{
	result->x = m->m[0][0] * v->x + m->m[1][0] * v->y + m->m[2][0] * v->z + m->m[3][0];
	result->y = m->m[0][1] * v->x + m->m[1][1] * v->y + m->m[2][1] * v->z + m->m[3][1];
	result->z = m->m[0][2] * v->x + m->m[1][2] * v->y + m->m[2][2] * v->z + m->m[3][2];
	result->w = m->m[0][3] * v->x + m->m[1][3] * v->y + m->m[2][3] * v->z + m->m[3][3];
}

// Transforms the 'mins' and 'maxs' bounds with the given 'matrix'.
static void ksMatrix4x4f_TransformBounds( ksVector3f * resultMins, ksVector3f * resultMaxs, const ksMatrix4x4f * matrix, const ksVector3f * mins, const ksVector3f * maxs )
{
	assert( ksMatrix4x4f_IsAffine( matrix, 1e-4f ) );

	const ksVector3f center = { ( mins->x + maxs->x ) * 0.5f, ( mins->y + maxs->y ) * 0.5f, ( mins->z + maxs->z ) * 0.5f };
	const ksVector3f extents = { maxs->x - center.x, maxs->y - center.y, maxs->z - center.z };
	const ksVector3f newCenter =
	{
		matrix->m[0][0] * center.x + matrix->m[1][0] * center.y + matrix->m[2][0] * center.z + matrix->m[3][0],
		matrix->m[0][1] * center.x + matrix->m[1][1] * center.y + matrix->m[2][1] * center.z + matrix->m[3][1],
		matrix->m[0][2] * center.x + matrix->m[1][2] * center.y + matrix->m[2][2] * center.z + matrix->m[3][2]
	};
	const ksVector3f newExtents =
	{
		fabsf( extents.x * matrix->m[0][0] ) + fabsf( extents.y * matrix->m[1][0] ) + fabsf( extents.z * matrix->m[2][0] ),
		fabsf( extents.x * matrix->m[0][1] ) + fabsf( extents.y * matrix->m[1][1] ) + fabsf( extents.z * matrix->m[2][1] ),
		fabsf( extents.x * matrix->m[0][2] ) + fabsf( extents.y * matrix->m[1][2] ) + fabsf( extents.z * matrix->m[2][2] )
	};
	ksVector3f_Sub( resultMins, &newCenter, &newExtents );
	ksVector3f_Add( resultMaxs, &newCenter, &newExtents );
}

// Returns true if the 'mins' and 'maxs' bounds is completely off to one side of the projection matrix.
static bool ksMatrix4x4f_CullBounds( const ksMatrix4x4f * mvp, const ksVector3f * mins, const ksVector3f * maxs )
{
	if ( maxs->x <= mins->x && maxs->y <= mins->y && maxs->z <= mins->z )
	{
		return false;
	}

	ksVector4f c[8];
	for ( int i = 0; i < 8; i++ )
	{
		const ksVector4f corner =
		{
			( i & 1 ) ? maxs->x : mins->x,
			( i & 2 ) ? maxs->y : mins->y,
			( i & 4 ) ? maxs->z : mins->z,
			1.0f
		};
		ksMatrix4x4f_TransformVector4f( &c[i], mvp, &corner );
	}

	int i;
	for ( i = 0; i < 8; i++ )
	{
		if ( c[i].x > -c[i].w )
		{
			break;
		}
	}
	if ( i == 8 )
	{
		return true;
	}
	for ( i = 0; i < 8; i++ )
	{
		if ( c[i].x < c[i].w )
		{
			break;
		}
	}
	if ( i == 8 )
	{
		return true;
	}

	for ( i = 0; i < 8; i++ )
	{
		if ( c[i].y > -c[i].w )
		{
			break;
		}
	}
	if ( i == 8 )
	{
		return true;
	}
	for ( i = 0; i < 8; i++ )
	{
		if ( c[i].y < c[i].w )
		{
			break;
		}
	}
	if ( i == 8 )
	{
		return true;
	}
	for ( i = 0; i < 8; i++ )
	{
		if ( c[i].z > -c[i].w )
		{
			break;
		}
	}
	if ( i == 8 )
	{
		return true;
	}
	for ( i = 0; i < 8; i++ )
	{
		if ( c[i].z < c[i].w )
		{
			break;
		}
	}
	if ( i == 8 )
	{
		return true;
	}

	return false;
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

OpenGL error checking.

================================================================================================================================
*/

#if defined( _DEBUG )
	#define GL( func )		func; ksFrameLog_Write( __FILE__, __LINE__, #func ); GlCheckErrors( #func );
#else
	#define GL( func )		func;
#endif

#if defined( _DEBUG )
	#define EGL( func )		ksFrameLog_Write( __FILE__, __LINE__, #func ); if ( func == EGL_FALSE ) { Error( #func " failed: %s", EglErrorString( eglGetError() ) ); }
#else
	#define EGL( func )		if ( func == EGL_FALSE ) { Error( #func " failed: %s", EglErrorString( eglGetError() ) ); }
#endif

#if defined( OS_ANDROID )
static const char * EglErrorString( const EGLint error )
{
	switch ( error )
	{
		case EGL_SUCCESS:				return "EGL_SUCCESS";
		case EGL_NOT_INITIALIZED:		return "EGL_NOT_INITIALIZED";
		case EGL_BAD_ACCESS:			return "EGL_BAD_ACCESS";
		case EGL_BAD_ALLOC:				return "EGL_BAD_ALLOC";
		case EGL_BAD_ATTRIBUTE:			return "EGL_BAD_ATTRIBUTE";
		case EGL_BAD_CONTEXT:			return "EGL_BAD_CONTEXT";
		case EGL_BAD_CONFIG:			return "EGL_BAD_CONFIG";
		case EGL_BAD_CURRENT_SURFACE:	return "EGL_BAD_CURRENT_SURFACE";
		case EGL_BAD_DISPLAY:			return "EGL_BAD_DISPLAY";
		case EGL_BAD_SURFACE:			return "EGL_BAD_SURFACE";
		case EGL_BAD_MATCH:				return "EGL_BAD_MATCH";
		case EGL_BAD_PARAMETER:			return "EGL_BAD_PARAMETER";
		case EGL_BAD_NATIVE_PIXMAP:		return "EGL_BAD_NATIVE_PIXMAP";
		case EGL_BAD_NATIVE_WINDOW:		return "EGL_BAD_NATIVE_WINDOW";
		case EGL_CONTEXT_LOST:			return "EGL_CONTEXT_LOST";
		default:						return "unknown";
	}
}
#endif

static const char * GlErrorString( GLenum error )
{
	switch ( error )
	{
		case GL_NO_ERROR:						return "GL_NO_ERROR";
		case GL_INVALID_ENUM:					return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:					return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:				return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION:	return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY:					return "GL_OUT_OF_MEMORY";
#if !defined( OS_APPLE_MACOS ) && !defined( OS_ANDROID )
		case GL_STACK_UNDERFLOW:				return "GL_STACK_UNDERFLOW";
		case GL_STACK_OVERFLOW:					return "GL_STACK_OVERFLOW";
#endif
		default: return "unknown";
	}
}

static const char * GlFramebufferStatusString( GLenum status )
{
	switch ( status )
	{
		case GL_FRAMEBUFFER_UNDEFINED:						return "GL_FRAMEBUFFER_UNDEFINED";
		case GL_FRAMEBUFFER_UNSUPPORTED:					return "GL_FRAMEBUFFER_UNSUPPORTED";
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:			return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
#if !defined( OS_ANDROID )
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:			return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:			return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:		return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
#endif
		default: return "unknown";
	}
}

static void GlCheckErrors( const char * function )
{
	for ( int i = 0; i < 10; i++ )
	{
		const GLenum error = glGetError();
		if ( error == GL_NO_ERROR )
		{
			break;
		}
		Error( "GL error: %s: %s", function, GlErrorString( error ) );
	}
}

/*
================================================================================================================================

OpenGL extensions.

================================================================================================================================
*/

typedef struct
{
	bool timer_query;						// GL_ARB_timer_query, GL_EXT_disjoint_timer_query
	bool texture_clamp_to_border;			// GL_EXT_texture_border_clamp, GL_OES_texture_border_clamp
	bool buffer_storage;					// GL_ARB_buffer_storage
	bool multi_sampled_storage;				// GL_ARB_texture_storage_multisample
	bool multi_view;						// GL_OVR_multiview, GL_OVR_multiview2
	bool multi_sampled_resolve;				// GL_EXT_multisampled_render_to_texture
	bool multi_view_multi_sampled_resolve;	// GL_OVR_multiview_multisampled_render_to_texture

	int texture_clamp_to_border_id;
} ksOpenGLExtensions;

ksOpenGLExtensions glExtensions;

/*
================================
Multi-view support
================================
*/

#if !defined( GL_OVR_multiview )
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_NUM_VIEWS_OVR			0x9630
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_BASE_VIEW_INDEX_OVR	0x9632
#define GL_MAX_VIEWS_OVR										0x9631

typedef void (* PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews);
#endif

/*
================================
Multi-sampling support
================================
*/

#if !defined( GL_EXT_framebuffer_multisample )
typedef void (* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
#endif

#if !defined( GL_EXT_multisampled_render_to_texture )
typedef void (* PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);
#endif

#if !defined( GL_OVR_multiview_multisampled_render_to_texture )
typedef void (* PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLsizei samples, GLint baseViewIndex, GLsizei numViews);
#endif

/*
================================
Get proc address / extensions
================================
*/

#if defined( OS_WINDOWS )
PROC GetExtension( const char * functionName )
{
	return wglGetProcAddress( functionName );
}
#elif defined( OS_APPLE )
void ( *GetExtension( const char * functionName ) )()
{
	return NULL;
}
#elif defined( OS_LINUX )
void ( *GetExtension( const char * functionName ) )()
{
	return glXGetProcAddress( (const GLubyte *)functionName );
}
#elif defined( OS_ANDROID )
void ( *GetExtension( const char * functionName ) )()
{
	return eglGetProcAddress( functionName );
}
#endif

GLint glGetInteger( GLenum pname )
{
	GLint i;
	GL( glGetIntegerv( pname, &i ) );
	return i;
}

static bool GlCheckExtension( const char * extension )
{
#if defined( OS_WINDOWS ) || defined( OS_LINUX )
	PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC) GetExtension( "glGetStringi" );
#endif
	GL( const GLint numExtensions = glGetInteger( GL_NUM_EXTENSIONS ) );
	for ( int i = 0; i < numExtensions; i++ )
	{
		GL( const GLubyte * string = glGetStringi( GL_EXTENSIONS, i ) );
		if ( strcmp( (const char *)string, extension ) == 0 )
		{
			return true;
		}
	}
	return false;
}

#if defined( OS_WINDOWS ) || defined( OS_LINUX )

PFNGLGENFRAMEBUFFERSPROC							glGenFramebuffers;
PFNGLDELETEFRAMEBUFFERSPROC							glDeleteFramebuffers;
PFNGLBINDFRAMEBUFFERPROC							glBindFramebuffer;
PFNGLBLITFRAMEBUFFERPROC							glBlitFramebuffer;
PFNGLGENRENDERBUFFERSPROC							glGenRenderbuffers;
PFNGLDELETERENDERBUFFERSPROC						glDeleteRenderbuffers;
PFNGLBINDRENDERBUFFERPROC							glBindRenderbuffer;
PFNGLISRENDERBUFFERPROC								glIsRenderbuffer;
PFNGLRENDERBUFFERSTORAGEPROC						glRenderbufferStorage;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC				glRenderbufferStorageMultisample;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC			glRenderbufferStorageMultisampleEXT;
PFNGLFRAMEBUFFERRENDERBUFFERPROC					glFramebufferRenderbuffer;
PFNGLFRAMEBUFFERTEXTURE2DPROC						glFramebufferTexture2D;
PFNGLFRAMEBUFFERTEXTURELAYERPROC					glFramebufferTextureLayer;
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC			glFramebufferTexture2DMultisampleEXT;
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC				glFramebufferTextureMultiviewOVR;
PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC	glFramebufferTextureMultisampleMultiviewOVR;
PFNGLCHECKFRAMEBUFFERSTATUSPROC						glCheckFramebufferStatus;

PFNGLGENBUFFERSPROC									glGenBuffers;
PFNGLDELETEBUFFERSPROC								glDeleteBuffers;
PFNGLBINDBUFFERPROC									glBindBuffer;
PFNGLBINDBUFFERBASEPROC								glBindBufferBase;
PFNGLBUFFERDATAPROC									glBufferData;
PFNGLBUFFERSTORAGEPROC								glBufferStorage;
PFNGLMAPBUFFERPROC									glMapBuffer;
PFNGLMAPBUFFERRANGEPROC								glMapBufferRange;
PFNGLUNMAPBUFFERPROC								glUnmapBuffer;

PFNGLGENVERTEXARRAYSPROC							glGenVertexArrays;
PFNGLDELETEVERTEXARRAYSPROC							glDeleteVertexArrays;
PFNGLBINDVERTEXARRAYPROC							glBindVertexArray;
PFNGLVERTEXATTRIBPOINTERPROC						glVertexAttribPointer;
PFNGLVERTEXATTRIBDIVISORPROC						glVertexAttribDivisor;
PFNGLDISABLEVERTEXATTRIBARRAYPROC					glDisableVertexAttribArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC					glEnableVertexAttribArray;

#if defined( OS_WINDOWS )
PFNGLACTIVETEXTUREPROC								glActiveTexture;
PFNGLTEXIMAGE3DPROC									glTexImage3D;
PFNGLCOMPRESSEDTEXIMAGE2DPROC						glCompressedTexImage2D;
PFNGLCOMPRESSEDTEXIMAGE3DPROC						glCompressedTexImage3D;
PFNGLTEXSUBIMAGE3DPROC								glTexSubImage3D;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC					glCompressedTexSubImage2D;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC					glCompressedTexSubImage3D;
#endif
PFNGLTEXSTORAGE2DPROC								glTexStorage2D;
PFNGLTEXSTORAGE3DPROC								glTexStorage3D;
PFNGLTEXIMAGE2DMULTISAMPLEPROC						glTexImage2DMultisample;
PFNGLTEXIMAGE3DMULTISAMPLEPROC						glTexImage3DMultisample;
PFNGLTEXSTORAGE2DMULTISAMPLEPROC					glTexStorage2DMultisample;
PFNGLTEXSTORAGE3DMULTISAMPLEPROC					glTexStorage3DMultisample;
PFNGLGENERATEMIPMAPPROC								glGenerateMipmap;
PFNGLBINDIMAGETEXTUREPROC							glBindImageTexture;

PFNGLCREATEPROGRAMPROC								glCreateProgram;
PFNGLDELETEPROGRAMPROC								glDeleteProgram;
PFNGLCREATESHADERPROC								glCreateShader;
PFNGLDELETESHADERPROC								glDeleteShader;
PFNGLSHADERSOURCEPROC								glShaderSource;
PFNGLCOMPILESHADERPROC								glCompileShader;
PFNGLGETSHADERIVPROC								glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC							glGetShaderInfoLog;
PFNGLUSEPROGRAMPROC									glUseProgram;
PFNGLATTACHSHADERPROC								glAttachShader;
PFNGLLINKPROGRAMPROC								glLinkProgram;
PFNGLGETPROGRAMIVPROC								glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC							glGetProgramInfoLog;
PFNGLGETATTRIBLOCATIONPROC							glGetAttribLocation;
PFNGLBINDATTRIBLOCATIONPROC							glBindAttribLocation;
PFNGLGETUNIFORMLOCATIONPROC							glGetUniformLocation;
PFNGLGETUNIFORMBLOCKINDEXPROC						glGetUniformBlockIndex;
PFNGLGETPROGRAMRESOURCEINDEXPROC					glGetProgramResourceIndex;
PFNGLUNIFORMBLOCKBINDINGPROC						glUniformBlockBinding;
PFNGLSHADERSTORAGEBLOCKBINDINGPROC					glShaderStorageBlockBinding;
PFNGLPROGRAMUNIFORM1IPROC							glProgramUniform1i;
PFNGLUNIFORM1IPROC									glUniform1i;
PFNGLUNIFORM1IVPROC									glUniform1iv;
PFNGLUNIFORM2IVPROC									glUniform2iv;
PFNGLUNIFORM3IVPROC									glUniform3iv;
PFNGLUNIFORM4IVPROC									glUniform4iv;
PFNGLUNIFORM1FPROC									glUniform1f;
PFNGLUNIFORM1FVPROC									glUniform1fv;
PFNGLUNIFORM2FVPROC									glUniform2fv;
PFNGLUNIFORM3FVPROC									glUniform3fv;
PFNGLUNIFORM4FVPROC									glUniform4fv;
PFNGLUNIFORMMATRIX2FVPROC							glUniformMatrix2fv;
PFNGLUNIFORMMATRIX2X3FVPROC							glUniformMatrix2x3fv;
PFNGLUNIFORMMATRIX2X4FVPROC							glUniformMatrix2x4fv;
PFNGLUNIFORMMATRIX3X2FVPROC							glUniformMatrix3x2fv;
PFNGLUNIFORMMATRIX3FVPROC							glUniformMatrix3fv;
PFNGLUNIFORMMATRIX3X4FVPROC							glUniformMatrix3x4fv;
PFNGLUNIFORMMATRIX4X2FVPROC							glUniformMatrix4x2fv;
PFNGLUNIFORMMATRIX4X3FVPROC							glUniformMatrix4x3fv;
PFNGLUNIFORMMATRIX4FVPROC							glUniformMatrix4fv;

PFNGLDRAWELEMENTSINSTANCEDPROC						glDrawElementsInstanced;
PFNGLDISPATCHCOMPUTEPROC							glDispatchCompute;
PFNGLMEMORYBARRIERPROC								glMemoryBarrier;

PFNGLGENQUERIESPROC									glGenQueries;
PFNGLDELETEQUERIESPROC								glDeleteQueries;
PFNGLISQUERYPROC									glIsQuery;
PFNGLBEGINQUERYPROC									glBeginQuery;
PFNGLENDQUERYPROC									glEndQuery;
PFNGLQUERYCOUNTERPROC								glQueryCounter;
PFNGLGETQUERYIVPROC									glGetQueryiv;
PFNGLGETQUERYOBJECTIVPROC							glGetQueryObjectiv;
PFNGLGETQUERYOBJECTUIVPROC							glGetQueryObjectuiv;
PFNGLGETQUERYOBJECTI64VPROC							glGetQueryObjecti64v;
PFNGLGETQUERYOBJECTUI64VPROC						glGetQueryObjectui64v;

PFNGLFENCESYNCPROC									glFenceSync;
PFNGLCLIENTWAITSYNCPROC								glClientWaitSync;
PFNGLDELETESYNCPROC									glDeleteSync;
PFNGLISSYNCPROC										glIsSync;

PFNGLBLENDFUNCSEPARATEPROC							glBlendFuncSeparate;
PFNGLBLENDEQUATIONSEPARATEPROC						glBlendEquationSeparate;

#if defined( OS_WINDOWS )
PFNGLBLENDCOLORPROC									glBlendColor;
PFNWGLCHOOSEPIXELFORMATARBPROC						wglChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC					wglCreateContextAttribsARB;
PFNWGLSWAPINTERVALEXTPROC							wglSwapIntervalEXT;
PFNWGLDELAYBEFORESWAPNVPROC							wglDelayBeforeSwapNV;
#elif defined( OS_LINUX )
PFNGLXCREATECONTEXTATTRIBSARBPROC					glXCreateContextAttribsARB;
PFNGLXSWAPINTERVALEXTPROC							glXSwapIntervalEXT;
PFNGLXDELAYBEFORESWAPNVPROC							glXDelayBeforeSwapNV;
#endif

static void GlInitExtensions()
{
	glGenFramebuffers							= (PFNGLGENFRAMEBUFFERSPROC)							GetExtension( "glGenFramebuffers" );
	glDeleteFramebuffers						= (PFNGLDELETEFRAMEBUFFERSPROC)							GetExtension( "glDeleteFramebuffers" );
	glBindFramebuffer							= (PFNGLBINDFRAMEBUFFERPROC)							GetExtension( "glBindFramebuffer" );
	glBlitFramebuffer							= (PFNGLBLITFRAMEBUFFERPROC)							GetExtension( "glBlitFramebuffer" );
	glGenRenderbuffers							= (PFNGLGENRENDERBUFFERSPROC)							GetExtension( "glGenRenderbuffers" );
	glDeleteRenderbuffers						= (PFNGLDELETERENDERBUFFERSPROC)						GetExtension( "glDeleteRenderbuffers" );
	glBindRenderbuffer							= (PFNGLBINDRENDERBUFFERPROC)							GetExtension( "glBindRenderbuffer" );
	glIsRenderbuffer							= (PFNGLISRENDERBUFFERPROC)								GetExtension( "glIsRenderbuffer" );
	glRenderbufferStorage						= (PFNGLRENDERBUFFERSTORAGEPROC)						GetExtension( "glRenderbufferStorage" );
	glRenderbufferStorageMultisample			= (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)				GetExtension( "glRenderbufferStorageMultisample" );
	glRenderbufferStorageMultisampleEXT			= (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)			GetExtension( "glRenderbufferStorageMultisampleEXT" );
	glFramebufferRenderbuffer					= (PFNGLFRAMEBUFFERRENDERBUFFERPROC)					GetExtension( "glFramebufferRenderbuffer" );
	glFramebufferTexture2D						= (PFNGLFRAMEBUFFERTEXTURE2DPROC)						GetExtension( "glFramebufferTexture2D" );
	glFramebufferTextureLayer					= (PFNGLFRAMEBUFFERTEXTURELAYERPROC)					GetExtension( "glFramebufferTextureLayer" );
	glFramebufferTexture2DMultisampleEXT		= (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)			GetExtension( "glFramebufferTexture2DMultisampleEXT" );
	glFramebufferTextureMultiviewOVR			= (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)				GetExtension( "glFramebufferTextureMultiviewOVR" );
	glFramebufferTextureMultisampleMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)	GetExtension( "glFramebufferTextureMultisampleMultiviewOVR" );
	glCheckFramebufferStatus					= (PFNGLCHECKFRAMEBUFFERSTATUSPROC)						GetExtension( "glCheckFramebufferStatus" );

	glGenBuffers								= (PFNGLGENBUFFERSPROC)					GetExtension( "glGenBuffers" );
	glDeleteBuffers								= (PFNGLDELETEBUFFERSPROC)				GetExtension( "glDeleteBuffers" );
	glBindBuffer								= (PFNGLBINDBUFFERPROC)					GetExtension( "glBindBuffer" );
	glBindBufferBase							= (PFNGLBINDBUFFERBASEPROC)				GetExtension( "glBindBufferBase" );
	glBufferData								= (PFNGLBUFFERDATAPROC)					GetExtension( "glBufferData" );
	glBufferStorage								= (PFNGLBUFFERSTORAGEPROC)				GetExtension( "glBufferStorage" );
	glMapBuffer									= (PFNGLMAPBUFFERPROC)					GetExtension( "glMapBuffer" );
	glMapBufferRange							= (PFNGLMAPBUFFERRANGEPROC)				GetExtension( "glMapBufferRange" );
	glUnmapBuffer								= (PFNGLUNMAPBUFFERPROC)				GetExtension( "glUnmapBuffer" );

	glGenVertexArrays							= (PFNGLGENVERTEXARRAYSPROC)			GetExtension( "glGenVertexArrays" );
	glDeleteVertexArrays						= (PFNGLDELETEVERTEXARRAYSPROC)			GetExtension( "glDeleteVertexArrays" );
	glBindVertexArray							= (PFNGLBINDVERTEXARRAYPROC)			GetExtension( "glBindVertexArray" );
	glVertexAttribPointer						= (PFNGLVERTEXATTRIBPOINTERPROC)		GetExtension( "glVertexAttribPointer" );
	glVertexAttribDivisor						= (PFNGLVERTEXATTRIBDIVISORPROC)		GetExtension( "glVertexAttribDivisor" );
	glDisableVertexAttribArray					= (PFNGLDISABLEVERTEXATTRIBARRAYPROC)	GetExtension( "glDisableVertexAttribArray" );
	glEnableVertexAttribArray					= (PFNGLENABLEVERTEXATTRIBARRAYPROC)	GetExtension( "glEnableVertexAttribArray" );

#if defined( OS_WINDOWS )
	glActiveTexture								= (PFNGLACTIVETEXTUREPROC)				GetExtension( "glActiveTexture" );
	glTexImage3D								= (PFNGLTEXIMAGE3DPROC)					GetExtension( "glTexImage3D" );
	glCompressedTexImage2D						= (PFNGLCOMPRESSEDTEXIMAGE2DPROC)		GetExtension( "glCompressedTexImage2D ");
	glCompressedTexImage3D						= (PFNGLCOMPRESSEDTEXIMAGE3DPROC)		GetExtension( "glCompressedTexImage3D ");
	glTexSubImage3D								= (PFNGLTEXSUBIMAGE3DPROC)				GetExtension( "glTexSubImage3D" );
	glCompressedTexSubImage2D					= (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)	GetExtension( "glCompressedTexSubImage2D" );
	glCompressedTexSubImage3D					= (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)	GetExtension( "glCompressedTexSubImage3D" );
#endif
	glTexStorage2D								= (PFNGLTEXSTORAGE2DPROC)				GetExtension( "glTexStorage2D" );
	glTexStorage3D								= (PFNGLTEXSTORAGE3DPROC)				GetExtension( "glTexStorage3D" );
	glTexImage2DMultisample						= (PFNGLTEXIMAGE2DMULTISAMPLEPROC)		GetExtension( "glTexImage2DMultisample" );
	glTexImage3DMultisample						= (PFNGLTEXIMAGE3DMULTISAMPLEPROC)		GetExtension( "glTexImage3DMultisample" );
	glTexStorage2DMultisample					= (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)	GetExtension( "glTexStorage2DMultisample" );
	glTexStorage3DMultisample					= (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)	GetExtension( "glTexStorage3DMultisample" );
	glGenerateMipmap							= (PFNGLGENERATEMIPMAPPROC)				GetExtension( "glGenerateMipmap" );
	glBindImageTexture							= (PFNGLBINDIMAGETEXTUREPROC)			GetExtension( "glBindImageTexture" );

	glCreateProgram								= (PFNGLCREATEPROGRAMPROC)				GetExtension( "glCreateProgram" );
	glDeleteProgram								= (PFNGLDELETEPROGRAMPROC)				GetExtension( "glDeleteProgram" );
	glCreateShader								= (PFNGLCREATESHADERPROC)				GetExtension( "glCreateShader" );
	glDeleteShader								= (PFNGLDELETESHADERPROC)				GetExtension( "glDeleteShader" );
	glShaderSource								= (PFNGLSHADERSOURCEPROC)				GetExtension( "glShaderSource" );
	glCompileShader								= (PFNGLCOMPILESHADERPROC)				GetExtension( "glCompileShader" );
	glGetShaderiv								= (PFNGLGETSHADERIVPROC)				GetExtension( "glGetShaderiv" );
	glGetShaderInfoLog							= (PFNGLGETSHADERINFOLOGPROC)			GetExtension( "glGetShaderInfoLog" );
	glUseProgram								= (PFNGLUSEPROGRAMPROC)					GetExtension( "glUseProgram" );
	glAttachShader								= (PFNGLATTACHSHADERPROC)				GetExtension( "glAttachShader" );
	glLinkProgram								= (PFNGLLINKPROGRAMPROC)				GetExtension( "glLinkProgram" );
	glGetProgramiv								= (PFNGLGETPROGRAMIVPROC)				GetExtension( "glGetProgramiv" );
	glGetProgramInfoLog							= (PFNGLGETPROGRAMINFOLOGPROC)			GetExtension( "glGetProgramInfoLog" );
	glGetAttribLocation							= (PFNGLGETATTRIBLOCATIONPROC)			GetExtension( "glGetAttribLocation" );
	glBindAttribLocation						= (PFNGLBINDATTRIBLOCATIONPROC)			GetExtension( "glBindAttribLocation" );
	glGetUniformLocation						= (PFNGLGETUNIFORMLOCATIONPROC)			GetExtension( "glGetUniformLocation" );
	glGetUniformBlockIndex						= (PFNGLGETUNIFORMBLOCKINDEXPROC)		GetExtension( "glGetUniformBlockIndex" );
	glProgramUniform1i							= (PFNGLPROGRAMUNIFORM1IPROC)			GetExtension( "glProgramUniform1i" );
	glUniform1i									= (PFNGLUNIFORM1IPROC)					GetExtension( "glUniform1i" );
	glUniform1iv								= (PFNGLUNIFORM1IVPROC)					GetExtension( "glUniform1iv" );
	glUniform2iv								= (PFNGLUNIFORM2IVPROC)					GetExtension( "glUniform2iv" );
	glUniform3iv								= (PFNGLUNIFORM3IVPROC)					GetExtension( "glUniform3iv" );
	glUniform4iv								= (PFNGLUNIFORM4IVPROC)					GetExtension( "glUniform4iv" );
	glUniform1f									= (PFNGLUNIFORM1FPROC)					GetExtension( "glUniform1f" );
	glUniform1fv								= (PFNGLUNIFORM1FVPROC)					GetExtension( "glUniform1fv" );
	glUniform2fv								= (PFNGLUNIFORM2FVPROC)					GetExtension( "glUniform2fv" );
	glUniform3fv								= (PFNGLUNIFORM3FVPROC)					GetExtension( "glUniform3fv" );
	glUniform4fv								= (PFNGLUNIFORM4FVPROC)					GetExtension( "glUniform4fv" );
	glUniformMatrix2fv							= (PFNGLUNIFORMMATRIX2FVPROC)			GetExtension( "glUniformMatrix3fv" );
	glUniformMatrix2x3fv						= (PFNGLUNIFORMMATRIX2X3FVPROC)			GetExtension( "glUniformMatrix2x3fv" );
	glUniformMatrix2x4fv						= (PFNGLUNIFORMMATRIX2X4FVPROC)			GetExtension( "glUniformMatrix2x4fv" );
	glUniformMatrix3x2fv						= (PFNGLUNIFORMMATRIX3X2FVPROC)			GetExtension( "glUniformMatrix3x2fv" );
	glUniformMatrix3fv							= (PFNGLUNIFORMMATRIX3FVPROC)			GetExtension( "glUniformMatrix3fv" );
	glUniformMatrix3x4fv						= (PFNGLUNIFORMMATRIX3X4FVPROC)			GetExtension( "glUniformMatrix3x4fv" );
	glUniformMatrix4x2fv						= (PFNGLUNIFORMMATRIX4X2FVPROC)			GetExtension( "glUniformMatrix4x2fv" );
	glUniformMatrix4x3fv						= (PFNGLUNIFORMMATRIX4X3FVPROC)			GetExtension( "glUniformMatrix4x3fv" );
	glUniformMatrix4fv							= (PFNGLUNIFORMMATRIX4FVPROC)			GetExtension( "glUniformMatrix4fv" );
	glGetProgramResourceIndex					= (PFNGLGETPROGRAMRESOURCEINDEXPROC)	GetExtension( "glGetProgramResourceIndex" );
	glUniformBlockBinding						= (PFNGLUNIFORMBLOCKBINDINGPROC)		GetExtension( "glUniformBlockBinding" );
	glShaderStorageBlockBinding					= (PFNGLSHADERSTORAGEBLOCKBINDINGPROC)	GetExtension( "glShaderStorageBlockBinding" );

	glDrawElementsInstanced						= (PFNGLDRAWELEMENTSINSTANCEDPROC)		GetExtension( "glDrawElementsInstanced" );
	glDispatchCompute							= (PFNGLDISPATCHCOMPUTEPROC)			GetExtension( "glDispatchCompute" );
	glMemoryBarrier								= (PFNGLMEMORYBARRIERPROC)				GetExtension( "glMemoryBarrier" );

	glGenQueries								= (PFNGLGENQUERIESPROC)					GetExtension( "glGenQueries" );
	glDeleteQueries								= (PFNGLDELETEQUERIESPROC)				GetExtension( "glDeleteQueries" );
	glIsQuery									= (PFNGLISQUERYPROC)					GetExtension( "glIsQuery" );
	glBeginQuery								= (PFNGLBEGINQUERYPROC)					GetExtension( "glBeginQuery" );
	glEndQuery									= (PFNGLENDQUERYPROC)					GetExtension( "glEndQuery" );
	glQueryCounter								= (PFNGLQUERYCOUNTERPROC)				GetExtension( "glQueryCounter" );
	glGetQueryiv								= (PFNGLGETQUERYIVPROC)					GetExtension( "glGetQueryiv" );
	glGetQueryObjectiv							= (PFNGLGETQUERYOBJECTIVPROC)			GetExtension( "glGetQueryObjectiv" );
	glGetQueryObjectuiv							= (PFNGLGETQUERYOBJECTUIVPROC)			GetExtension( "glGetQueryObjectuiv" );
	glGetQueryObjecti64v						= (PFNGLGETQUERYOBJECTI64VPROC)			GetExtension( "glGetQueryObjecti64v" );
	glGetQueryObjectui64v						= (PFNGLGETQUERYOBJECTUI64VPROC)		GetExtension( "glGetQueryObjectui64v" );

	glFenceSync									= (PFNGLFENCESYNCPROC)					GetExtension( "glFenceSync" );
	glClientWaitSync							= (PFNGLCLIENTWAITSYNCPROC)				GetExtension( "glClientWaitSync" );
	glDeleteSync								= (PFNGLDELETESYNCPROC)					GetExtension( "glDeleteSync" );
	glIsSync									= (PFNGLISSYNCPROC)						GetExtension( "glIsSync" );

	glBlendFuncSeparate							= (PFNGLBLENDFUNCSEPARATEPROC)			GetExtension( "glBlendFuncSeparate" );
	glBlendEquationSeparate						= (PFNGLBLENDEQUATIONSEPARATEPROC)		GetExtension( "glBlendEquationSeparate" );

#if defined( OS_WINDOWS )
	glBlendColor								= (PFNGLBLENDCOLORPROC)					GetExtension( "glBlendColor" );
	wglChoosePixelFormatARB						= (PFNWGLCHOOSEPIXELFORMATARBPROC)		GetExtension( "wglChoosePixelFormatARB" );
	wglCreateContextAttribsARB					= (PFNWGLCREATECONTEXTATTRIBSARBPROC)	GetExtension( "wglCreateContextAttribsARB" );
	wglSwapIntervalEXT							= (PFNWGLSWAPINTERVALEXTPROC)			GetExtension( "wglSwapIntervalEXT" );
	wglDelayBeforeSwapNV						= (PFNWGLDELAYBEFORESWAPNVPROC)			GetExtension( "wglDelayBeforeSwapNV" );
#elif defined( OS_LINUX )
	glXCreateContextAttribsARB					= (PFNGLXCREATECONTEXTATTRIBSARBPROC)	GetExtension( "glXCreateContextAttribsARB" );
	glXSwapIntervalEXT							= (PFNGLXSWAPINTERVALEXTPROC)			GetExtension( "glXSwapIntervalEXT" );
	glXDelayBeforeSwapNV						= (PFNGLXDELAYBEFORESWAPNVPROC)			GetExtension( "glXDelayBeforeSwapNV" );
#endif

	glExtensions.timer_query						= GlCheckExtension( "GL_EXT_timer_query" );
	glExtensions.texture_clamp_to_border			= true; // always available
	glExtensions.buffer_storage						= GlCheckExtension( "GL_EXT_buffer_storage" ) || ( OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR >= 44 );
	glExtensions.multi_sampled_storage				= GlCheckExtension( "GL_ARB_texture_storage_multisample" ) || ( OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR >= 43 );
	glExtensions.multi_view							= GlCheckExtension( "GL_OVR_multiview2" );
	glExtensions.multi_sampled_resolve				= GlCheckExtension( "GL_EXT_multisampled_render_to_texture" );
	glExtensions.multi_view_multi_sampled_resolve	= GlCheckExtension( "GL_OVR_multiview_multisampled_render_to_texture" );

	glExtensions.texture_clamp_to_border_id			= GL_CLAMP_TO_BORDER;
}

#elif defined( OS_APPLE_MACOS )

PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC				glFramebufferTextureMultiviewOVR;
PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC	glFramebufferTextureMultisampleMultiviewOVR;
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC			glFramebufferTexture2DMultisampleEXT;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC			glRenderbufferStorageMultisampleEXT;

static void GlInitExtensions()
{
	glExtensions.timer_query						= GlCheckExtension( "GL_EXT_timer_query" );
	glExtensions.texture_clamp_to_border			= true; // always available
	glExtensions.buffer_storage						= GlCheckExtension( "GL_EXT_buffer_storage" ) || ( OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR >= 44 );
	glExtensions.multi_sampled_storage				= GlCheckExtension( "GL_ARB_texture_storage_multisample" ) || ( OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR >= 43 );
	glExtensions.multi_view							= GlCheckExtension( "GL_OVR_multiview2" );
	glExtensions.multi_sampled_resolve				= GlCheckExtension( "GL_EXT_multisampled_render_to_texture" );
	glExtensions.multi_view_multi_sampled_resolve	= GlCheckExtension( "GL_OVR_multiview_multisampled_render_to_texture" );

	glExtensions.texture_clamp_to_border_id			= GL_CLAMP_TO_BORDER;
}

#elif defined( OS_ANDROID )

// GL_EXT_disjoint_timer_query without _EXT
#if !defined( GL_TIMESTAMP )
#define GL_QUERY_COUNTER_BITS				GL_QUERY_COUNTER_BITS_EXT
#define GL_TIME_ELAPSED						GL_TIME_ELAPSED_EXT
#define GL_TIMESTAMP						GL_TIMESTAMP_EXT
#define GL_GPU_DISJOINT						GL_GPU_DISJOINT_EXT
#endif

// GL_EXT_buffer_storage without _EXT
#if !defined( GL_BUFFER_STORAGE_FLAGS )
#define GL_MAP_READ_BIT						0x0001		// GL_MAP_READ_BIT_EXT
#define GL_MAP_WRITE_BIT					0x0002		// GL_MAP_WRITE_BIT_EXT
#define GL_MAP_PERSISTENT_BIT				0x0040		// GL_MAP_PERSISTENT_BIT_EXT
#define GL_MAP_COHERENT_BIT					0x0080		// GL_MAP_COHERENT_BIT_EXT
#define GL_DYNAMIC_STORAGE_BIT				0x0100		// GL_DYNAMIC_STORAGE_BIT_EXT
#define GL_CLIENT_STORAGE_BIT				0x0200		// GL_CLIENT_STORAGE_BIT_EXT
#define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT	0x00004000	// GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT_EXT
#define GL_BUFFER_IMMUTABLE_STORAGE			0x821F		// GL_BUFFER_IMMUTABLE_STORAGE_EXT
#define GL_BUFFER_STORAGE_FLAGS				0x8220		// GL_BUFFER_STORAGE_FLAGS_EXT
#endif

typedef void (GL_APIENTRY * PFNGLBUFFERSTORAGEEXTPROC) (GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);
typedef void (GL_APIENTRY * PFNGLTEXSTORAGE3DMULTISAMPLEPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);

// EGL_KHR_fence_sync, GL_OES_EGL_sync, VG_KHR_EGL_sync
PFNEGLCREATESYNCKHRPROC								eglCreateSyncKHR;
PFNEGLDESTROYSYNCKHRPROC							eglDestroySyncKHR;
PFNEGLCLIENTWAITSYNCKHRPROC							eglClientWaitSyncKHR;
PFNEGLGETSYNCATTRIBKHRPROC							eglGetSyncAttribKHR;

// GL_EXT_disjoint_timer_query
PFNGLQUERYCOUNTEREXTPROC							glQueryCounter;
PFNGLGETQUERYOBJECTI64VEXTPROC						glGetQueryObjecti64v;
PFNGLGETQUERYOBJECTUI64VEXTPROC						glGetQueryObjectui64v;

// GL_EXT_buffer_storage
PFNGLBUFFERSTORAGEEXTPROC							glBufferStorage;

// GL_OVR_multiview
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC				glFramebufferTextureMultiviewOVR;

// GL_EXT_multisampled_render_to_texture
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC			glRenderbufferStorageMultisampleEXT;
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC			glFramebufferTexture2DMultisampleEXT;

// GL_OVR_multiview_multisampled_render_to_texture
PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC	glFramebufferTextureMultisampleMultiviewOVR;

PFNGLTEXSTORAGE3DMULTISAMPLEPROC					glTexStorage3DMultisample;

#if !defined( EGL_OPENGL_ES3_BIT )
#define EGL_OPENGL_ES3_BIT					0x0040
#endif

// GL_EXT_texture_cube_map_array
#if !defined( GL_TEXTURE_CUBE_MAP_ARRAY )
#define GL_TEXTURE_CUBE_MAP_ARRAY			0x9009
#endif

// GL_EXT_texture_filter_anisotropic
#if !defined( GL_TEXTURE_MAX_ANISOTROPY_EXT )
#define GL_TEXTURE_MAX_ANISOTROPY_EXT		0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT	0x84FF
#endif

// GL_EXT_texture_border_clamp or GL_OES_texture_border_clamp
#if !defined( GL_CLAMP_TO_BORDER )
#define GL_CLAMP_TO_BORDER					0x812D
#endif

// No 1D textures in OpenGL ES.
#if !defined( GL_TEXTURE_1D )
#define GL_TEXTURE_1D						0x0DE0
#endif

// No 1D texture arrays in OpenGL ES.
#if !defined( GL_TEXTURE_1D_ARRAY )
#define GL_TEXTURE_1D_ARRAY					0x8C18
#endif

// No multi-sampled texture arrays in OpenGL ES.
#if !defined( GL_TEXTURE_2D_MULTISAMPLE_ARRAY )
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY		0x9102
#endif

static void GlInitExtensions()
{
	eglCreateSyncKHR								= (PFNEGLCREATESYNCKHRPROC)			GetExtension( "eglCreateSyncKHR" );
	eglDestroySyncKHR								= (PFNEGLDESTROYSYNCKHRPROC)		GetExtension( "eglDestroySyncKHR" );
	eglClientWaitSyncKHR							= (PFNEGLCLIENTWAITSYNCKHRPROC)		GetExtension( "eglClientWaitSyncKHR" );
	eglGetSyncAttribKHR								= (PFNEGLGETSYNCATTRIBKHRPROC)		GetExtension( "eglGetSyncAttribKHR" );

	glQueryCounter									= (PFNGLQUERYCOUNTEREXTPROC)		GetExtension( "glQueryCounterEXT" );
	glGetQueryObjecti64v							= (PFNGLGETQUERYOBJECTI64VEXTPROC)	GetExtension( "glGetQueryObjecti64vEXT" );
	glGetQueryObjectui64v							= (PFNGLGETQUERYOBJECTUI64VEXTPROC)	GetExtension( "glGetQueryObjectui64vEXT" );

	glBufferStorage									= (PFNGLBUFFERSTORAGEEXTPROC)		GetExtension( "glBufferStorageEXT" );

	glRenderbufferStorageMultisampleEXT				= (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)			GetExtension( "glRenderbufferStorageMultisampleEXT" );
	glFramebufferTexture2DMultisampleEXT			= (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)			GetExtension( "glFramebufferTexture2DMultisampleEXT" );
	glFramebufferTextureMultiviewOVR				= (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)				GetExtension( "glFramebufferTextureMultiviewOVR" );
	glFramebufferTextureMultisampleMultiviewOVR		= (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)	GetExtension( "glFramebufferTextureMultisampleMultiviewOVR" );

	glTexStorage3DMultisample						= (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)					GetExtension( "glTexStorage3DMultisample" );
	
	glExtensions.timer_query						= GlCheckExtension( "GL_EXT_disjoint_timer_query" );
	glExtensions.texture_clamp_to_border			= GlCheckExtension( "GL_EXT_texture_border_clamp" ) || GlCheckExtension( "GL_OES_texture_border_clamp" );
	glExtensions.buffer_storage						= GlCheckExtension( "GL_EXT_buffer_storage" );
	glExtensions.multi_view							= GlCheckExtension( "GL_OVR_multiview2" );
	glExtensions.multi_sampled_resolve				= GlCheckExtension( "GL_EXT_multisampled_render_to_texture" );
	glExtensions.multi_view_multi_sampled_resolve	= GlCheckExtension( "GL_OVR_multiview_multisampled_render_to_texture" );

	glExtensions.texture_clamp_to_border_id			=	( GlCheckExtension( "GL_OES_texture_border_clamp" ) ? GL_CLAMP_TO_BORDER :
														( GlCheckExtension( "GL_EXT_texture_border_clamp" ) ? GL_CLAMP_TO_BORDER :
														( GL_CLAMP_TO_EDGE ) ) );
}

#endif

/*
================================
Compute support
================================
*/

#if defined( OS_APPLE_MACOS ) && ( OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR < 43 )

	#define OPENGL_COMPUTE_ENABLED	0

	#define GL_SHADER_STORAGE_BUFFER			0x90D2
	#define GL_COMPUTE_SHADER					0x91B9
	#define GL_UNIFORM_BLOCK					0x92E2
	#define GL_SHADER_STORAGE_BLOCK				0x92E6

	#define GL_TEXTURE_FETCH_BARRIER_BIT		0x00000008
	#define GL_TEXTURE_UPDATE_BARRIER_BIT		0x00000100
	#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT	0x00000020
	#define GL_FRAMEBUFFER_BARRIER_BIT			0x00000400
	#define GL_ALL_BARRIER_BITS					0xFFFFFFFF

	static GLuint glGetProgramResourceIndex( GLuint program, GLenum programInterface, const GLchar *name ) { assert( false ); return 0; }
	static void glShaderStorageBlockBinding( GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding ) { assert( false ); }
	static void glBindImageTexture( GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format ) { assert( false ); }
	static void glDispatchCompute( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z ) { assert( false ); }

	static void glMemoryBarrier( GLbitfield barriers ) { assert( false ); }

	static void glTexStorage2DMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations ) { assert( false ); }
	static void glTexStorage3DMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations ) { assert( false ); }

#elif defined( OS_ANDROID ) && ( OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR < 31 )

	#define OPENGL_COMPUTE_ENABLED	0

	#define GL_SHADER_STORAGE_BUFFER			0x90D2
	#define GL_COMPUTE_SHADER					0x91B9
	#define GL_UNIFORM_BLOCK					0x92E2
	#define GL_SHADER_STORAGE_BLOCK				0x92E6

	#define GL_READ_ONLY						0x88B8
	#define GL_WRITE_ONLY						0x88B9
	#define GL_READ_WRITE						0x88BA

	#define GL_TEXTURE_FETCH_BARRIER_BIT		0x00000008
	#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT	0x00000020
	#define GL_FRAMEBUFFER_BARRIER_BIT			0x00000400
	#define GL_ALL_BARRIER_BITS					0xFFFFFFFF

	static GLuint glGetProgramResourceIndex( GLuint program, GLenum programInterface, const GLchar *name ) { assert( false ); return 0; }
	static void glShaderStorageBlockBinding( GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding ) { assert( false ); }
	static void glBindImageTexture( GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format ) { assert( false ); }
	static void glDispatchCompute( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z ) { assert( false ); }

	static void glProgramUniform1i( GLuint program, GLint location, GLint v0 ) { assert( false ); }
	static void glMemoryBarrier( GLbitfield barriers ) { assert( false ); }

#else

	#define OPENGL_COMPUTE_ENABLED	1

#endif

#if !defined( GL_SR8_EXT )
#define GL_SR8_EXT							0x8FBD
#endif

#if !defined( GL_SRG8_EXT )
#define GL_SRG8_EXT							0x8FBE
#endif

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
	int dummy;
} ksDriverInstance;

static bool ksDriverInstance_Create( ksDriverInstance * instance )
{
	memset( instance, 0, sizeof( ksDriverInstance ) );
	return true;
}

static void ksDriverInstance_Destroy( ksDriverInstance * instance )
{
	memset( instance, 0, sizeof( ksDriverInstance ) );
}

/*
================================================================================================================================

GPU device.

ksGpuQueueProperty
ksGpuQueuePriority
ksGpuQueueInfo
ksGpuDevice

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
	ksDriverInstance *	instance;
	ksGpuQueueInfo		queueInfo;

	size_t				maxPushConstantsSize;
} ksGpuDevice;

static bool ksGpuDevice_Create( ksGpuDevice * device, ksDriverInstance * instance, const ksGpuQueueInfo * queueInfo )
{
	/*
		Use an extensions to select the appropriate device:
		https://www.opengl.org/registry/specs/NV/gpu_affinity.txt
		https://www.opengl.org/registry/specs/AMD/wgl_gpu_association.txt
		https://www.opengl.org/registry/specs/AMD/glx_gpu_association.txt

		On Linux configure each GPU to use a separate X screen and then select
		the X screen to render to.
	*/

	memset( device, 0, sizeof( ksGpuDevice ) );

	device->instance = instance;
	device->queueInfo = *queueInfo;

	device->maxPushConstantsSize = 512;

	return true;
}

static void ksGpuDevice_Destroy( ksGpuDevice * device )
{
	memset( device, 0, sizeof( ksGpuDevice ) );
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
static void ksGpuContext_SetCurrent( ksGpuContext * context );
static void ksGpuContext_UnsetCurrent( ksGpuContext * context );
static bool ksGpuContext_CheckCurrent( ksGpuContext * context );

static bool ksGpuContext_CreateForSurface( ksGpuContext * context, const ksGpuDevice * device, const int queueIndex,
										const ksGpuSurfaceColorFormat colorFormat,
										const ksGpuSurfaceDepthFormat depthFormat,
										const ksGpuSampleCount sampleCount,
										... );

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
	KS_GPU_SAMPLE_COUNT_1		= 1,
	KS_GPU_SAMPLE_COUNT_2		= 2,
	KS_GPU_SAMPLE_COUNT_4		= 4,
	KS_GPU_SAMPLE_COUNT_8		= 8,
	KS_GPU_SAMPLE_COUNT_16		= 16,
	KS_GPU_SAMPLE_COUNT_32		= 32,
	KS_GPU_SAMPLE_COUNT_64		= 64,
} ksGpuSampleCount;

typedef struct
{
	const ksGpuDevice *		device;
#if defined( OS_WINDOWS )
	HDC						hDC;
	HGLRC					hGLRC;
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )
	Display *				xDisplay;
	uint32_t				visualid;
	GLXFBConfig				glxFBConfig;
	GLXDrawable				glxDrawable;
	GLXContext				glxContext;
#elif defined( OS_LINUX_XCB )
	xcb_connection_t *		connection;
	uint32_t				screen_number;
	xcb_glx_fbconfig_t		fbconfigid;
	xcb_visualid_t			visualid;
	xcb_glx_drawable_t		glxDrawable;
	xcb_glx_context_t		glxContext;
	xcb_glx_context_tag_t	glxContextTag;
#elif defined( OS_APPLE_MACOS )
	NSOpenGLContext *		nsContext;
	CGLContextObj			cglContext;
#elif defined( OS_ANDROID )
	EGLDisplay				display;
	EGLConfig				config;
	EGLSurface				tinySurface;
	EGLSurface				mainSurface;
	EGLContext				context;
#endif
} ksGpuContext;

typedef struct
{
	unsigned char	redBits;
	unsigned char	greenBits;
	unsigned char	blueBits;
	unsigned char	alphaBits;
	unsigned char	colorBits;
	unsigned char	depthBits;
} ksGpuSurfaceBits;

ksGpuSurfaceBits ksGpuContext_BitsForSurfaceFormat( const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat )
{
	ksGpuSurfaceBits bits;
	bits.redBits =		( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 ) ? 8 :
						( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 ) ? 8 :
						( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5 ) ? 5 :
						( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5 ) ? 5 :
						8 ) ) ) );
	bits.greenBits =	( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 ) ? 8 :
						( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 ) ? 8 :
						( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5 ) ? 6 :
						( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5 ) ? 6 :
						8 ) ) ) );
	bits.blueBits =		( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 ) ? 8 :
						( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 ) ? 8 :
						( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5 ) ? 5 :
						( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5 ) ? 5 :
						8 ) ) ) );
	bits.alphaBits =	( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 ) ? 8 :
						( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 ) ? 8 :
						( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5 ) ? 0 :
						( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5 ) ? 0 :
						8 ) ) ) );
	bits.colorBits =	bits.redBits + bits.greenBits + bits.blueBits + bits.alphaBits;
	bits.depthBits =	( ( depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_D16 ) ? 16 :
						( ( depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_D24 ) ? 24 :
						0 ) );
	return bits;
}

GLenum ksGpuContext_InternalSurfaceColorFormat( const ksGpuSurfaceColorFormat colorFormat )
{
	return	( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 ) ? GL_RGBA8 :
			( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 ) ? GL_RGBA8 :
			( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5 ) ? GL_RGB565 :
			( ( colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5 ) ? GL_RGB565 :
			GL_RGBA8 ) ) ) );
}

GLenum ksGpuContext_InternalSurfaceDepthFormat( const ksGpuSurfaceDepthFormat depthFormat )
{
	return	( ( depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_D16 ) ? GL_DEPTH_COMPONENT16 :
			( ( depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_D24 ) ? GL_DEPTH_COMPONENT24 :
			GL_DEPTH_COMPONENT24 ) );
}

#if defined( OS_WINDOWS )

static bool ksGpuContext_CreateForSurface( ksGpuContext * context, const ksGpuDevice * device, const int queueIndex,
										const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
										const ksGpuSampleCount sampleCount, HINSTANCE hInstance, HDC hDC )
{
	UNUSED_PARM( queueIndex );

	context->device = device;

	const ksGpuSurfaceBits bits = ksGpuContext_BitsForSurfaceFormat( colorFormat, depthFormat );

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof( PIXELFORMATDESCRIPTOR ),
		1,						// version
		PFD_DRAW_TO_WINDOW |	// must support windowed
		PFD_SUPPORT_OPENGL |	// must support OpenGL
		PFD_DOUBLEBUFFER,		// must support double buffering
		PFD_TYPE_RGBA,			// iPixelType
		bits.colorBits,			// cColorBits
		0, 0,					// cRedBits, cRedShift
		0, 0,					// cGreenBits, cGreenShift
		0, 0,					// cBlueBits, cBlueShift
		0, 0,					// cAlphaBits, cAlphaShift
		0,						// cAccumBits
		0,						// cAccumRedBits
		0,						// cAccumGreenBits
		0,						// cAccumBlueBits
		0,						// cAccumAlphaBits
		bits.depthBits,			// cDepthBits
		0,						// cStencilBits
		0,						// cAuxBuffers
		PFD_MAIN_PLANE,			// iLayerType
		0,						// bReserved
		0,						// dwLayerMask
		0,						// dwVisibleMask
		0						// dwDamageMask
	};

	HWND localWnd = NULL;
	HDC localDC = hDC;

	if ( sampleCount > KS_GPU_SAMPLE_COUNT_1 )
	{
		// A valid OpenGL context is needed to get OpenGL extensions including wglChoosePixelFormatARB
		// and wglCreateContextAttribsARB. A device context with a valid pixel format is needed to create
		// an OpenGL context. However, once a pixel format is set on a device context it is final.
		// Therefore a pixel format is set on the device context of a temporary window to create a context
		// to get the extensions for multi-sampling.
		localWnd = CreateWindow( APPLICATION_NAME, "temp", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL );
		localDC = GetDC( localWnd );
	}

	int pixelFormat = ChoosePixelFormat( localDC, &pfd );
	if ( pixelFormat == 0 )
	{
		Error( "Failed to find a suitable pixel format." );
		return false;
	}

	if ( !SetPixelFormat( localDC, pixelFormat, &pfd ) )
	{
		Error( "Failed to set the pixel format." );
		return false;
	}

	// Now that the pixel format is set, create a temporary context to get the extensions.
	{
		HGLRC hGLRC = wglCreateContext( localDC );
		wglMakeCurrent( localDC, hGLRC );

		GlInitExtensions();

		wglMakeCurrent( NULL, NULL );
		wglDeleteContext( hGLRC );
	}

	if ( sampleCount > KS_GPU_SAMPLE_COUNT_1 )
	{
		// Release the device context and destroy the window that were created to get extensions.
		ReleaseDC( localWnd, localDC );
		DestroyWindow( localWnd );

		int pixelFormatAttribs[] =
		{
			WGL_DRAW_TO_WINDOW_ARB,				GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB,				GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB,				GL_TRUE,
			WGL_PIXEL_TYPE_ARB,					WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB,					bits.colorBits,
			WGL_DEPTH_BITS_ARB,					bits.depthBits,
			WGL_SAMPLE_BUFFERS_ARB,				1,
			WGL_SAMPLES_ARB,					sampleCount,
			0
		};

		unsigned int numPixelFormats = 0;

		if ( !wglChoosePixelFormatARB( hDC, pixelFormatAttribs, NULL, 1, &pixelFormat, &numPixelFormats ) || numPixelFormats == 0 )
		{
			Error( "Failed to find MSAA pixel format." );
			return false;
		}

		memset( &pfd, 0, sizeof( pfd ) );

		if ( !DescribePixelFormat( hDC, pixelFormat, sizeof( PIXELFORMATDESCRIPTOR ), &pfd ) )
		{
			Error( "Failed to describe the pixel format." );
			return false;
		}

		if ( !SetPixelFormat( hDC, pixelFormat, &pfd ) )
		{
			Error( "Failed to set the pixel format." );
			return false;
		}
	}

	int contextAttribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB,	OPENGL_VERSION_MAJOR,
		WGL_CONTEXT_MINOR_VERSION_ARB,	OPENGL_VERSION_MINOR,
		WGL_CONTEXT_PROFILE_MASK_ARB,	WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		WGL_CONTEXT_FLAGS_ARB,			WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	context->hDC = hDC;
	context->hGLRC = wglCreateContextAttribsARB( hDC, NULL, contextAttribs );
	if ( !context->hGLRC )
	{
		Error( "Failed to create GL context." );
		return false;
	}

	return true;
}

#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )

static int glxGetFBConfigAttrib2( Display * dpy, GLXFBConfig config, int attribute )
{
	int value;
	glXGetFBConfigAttrib( dpy, config, attribute, &value );
	return value;
}

static bool ksGpuContext_CreateForSurface( ksGpuContext * context, const ksGpuDevice * device, const int queueIndex,
										const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
										const ksGpuSampleCount sampleCount, Display * xDisplay, int xScreen )
{
	UNUSED_PARM( queueIndex );

	context->device = device;

	GlInitExtensions();

	int glxErrorBase;
	int glxEventBase;
	if ( !glXQueryExtension( xDisplay, &glxErrorBase, &glxEventBase ) )
	{
		Error( "X display does not support the GLX extension." );
		return false;
	}

	int glxVersionMajor;
	int glxVersionMinor;
	if ( !glXQueryVersion( xDisplay, &glxVersionMajor, &glxVersionMinor ) )
	{
		Error( "Unable to retrieve GLX version." );
		return false;
	}

	int fbConfigCount = 0;
	GLXFBConfig * fbConfigs = glXGetFBConfigs( xDisplay, xScreen, &fbConfigCount );
	if ( fbConfigCount == 0 )
	{
		Error( "No valid framebuffer configurations found." );
		return false;
	}

	const ksGpuSurfaceBits bits = ksGpuContext_BitsForSurfaceFormat( colorFormat, depthFormat );

	bool foundFbConfig = false;
	for ( int i = 0; i < fbConfigCount; i++ )
	{
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_FBCONFIG_ID ) == 0 ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_VISUAL_ID ) == 0 ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_DOUBLEBUFFER ) == 0 ) { continue; }
		if ( ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_RENDER_TYPE ) & GLX_RGBA_BIT ) == 0 ) { continue; }
		if ( ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_DRAWABLE_TYPE ) & GLX_WINDOW_BIT ) == 0 ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_RED_SIZE )   != bits.redBits ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_GREEN_SIZE ) != bits.greenBits ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_BLUE_SIZE )  != bits.blueBits ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_ALPHA_SIZE ) != bits.alphaBits ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_DEPTH_SIZE ) != bits.depthBits ) { continue; }
		if ( sampleCount > KS_GPU_SAMPLE_COUNT_1 )
		{
			if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_SAMPLE_BUFFERS ) != 1 ) { continue; }
			if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_SAMPLES ) != sampleCount ) { continue; }
		}

		context->visualid = glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_VISUAL_ID );
		context->glxFBConfig = fbConfigs[i];
		foundFbConfig = true;
		break;
	}

	XFree( fbConfigs );

	if ( !foundFbConfig )
	{
		Error( "Failed to to find desired framebuffer configuration." );
		return false;
	}

	context->xDisplay = xDisplay;

	int attribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB,	OPENGL_VERSION_MAJOR,
		GLX_CONTEXT_MINOR_VERSION_ARB,	OPENGL_VERSION_MINOR,
		GLX_CONTEXT_PROFILE_MASK_ARB,	GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		GLX_CONTEXT_FLAGS_ARB,			GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	context->glxContext = glXCreateContextAttribsARB( xDisplay,					// Display *	dpy
														context->glxFBConfig,	// GLXFBConfig	config
														NULL,					// GLXContext	share_context
														True,					// Bool			direct
														attribs );				// const int *	attrib_list

	if ( context->glxContext == NULL )
	{
		Error( "Unable to create GLX context." );
		return false;
	}

	if ( !glXIsDirect( xDisplay, context->glxContext ) )
	{
		Error( "Unable to create direct rendering context." );
		return false;
	}

	return true;
}

#elif defined( OS_LINUX_XCB )

static uint32_t xcb_glx_get_property( const uint32_t * properties, const uint32_t numProperties, uint32_t propertyName )
{
	for ( int i = 0; i < numProperties; i++ )
	{
		if ( properties[i * 2 + 0] == propertyName )
		{
			return properties[i * 2 + 1];
		}
	}
	return 0;
}

static bool ksGpuContext_CreateForSurface( ksGpuContext * context, const ksGpuDevice * device, const int queueIndex,
										const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
										const ksGpuSampleCount sampleCount, xcb_connection_t * connection, int screen_number )
{
	UNUSED_PARM( queueIndex );

	context->device = device;

	GlInitExtensions();

	xcb_glx_query_version_cookie_t glx_query_version_cookie = xcb_glx_query_version( connection, OPENGL_VERSION_MAJOR, OPENGL_VERSION_MINOR );
	xcb_glx_query_version_reply_t * glx_query_version_reply = xcb_glx_query_version_reply( connection, glx_query_version_cookie, NULL );
	if ( glx_query_version_reply == NULL )
	{
		Error( "Unable to retrieve GLX version." );
		return false;
	}
	free( glx_query_version_reply );

	xcb_glx_get_fb_configs_cookie_t get_fb_configs_cookie = xcb_glx_get_fb_configs( connection, screen_number );
	xcb_glx_get_fb_configs_reply_t * get_fb_configs_reply = xcb_glx_get_fb_configs_reply( connection, get_fb_configs_cookie, NULL );

	if ( get_fb_configs_reply == NULL || get_fb_configs_reply->num_FB_configs == 0 )
	{
		Error( "No valid framebuffer configurations found." );
		return false;
	}

	const ksGpuSurfaceBits bits = ksGpuContext_BitsForSurfaceFormat( colorFormat, depthFormat );

	const uint32_t * fb_configs_properties = xcb_glx_get_fb_configs_property_list( get_fb_configs_reply );
	const uint32_t fb_configs_num_properties = get_fb_configs_reply->num_properties;

	bool foundFbConfig = false;
	for ( int i = 0; i < get_fb_configs_reply->num_FB_configs; i++ )
	{
		const uint32_t * fb_config = fb_configs_properties + i * fb_configs_num_properties * 2;

		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_FBCONFIG_ID ) == 0 ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_VISUAL_ID ) == 0 ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_DOUBLEBUFFER ) == 0 ) { continue; }
		if ( ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_RENDER_TYPE ) & GLX_RGBA_BIT ) == 0 ) { continue; }
		if ( ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_DRAWABLE_TYPE ) & GLX_WINDOW_BIT ) == 0 ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_RED_SIZE )   != bits.redBits ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_GREEN_SIZE ) != bits.greenBits ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_BLUE_SIZE )  != bits.blueBits ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_ALPHA_SIZE ) != bits.alphaBits ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_DEPTH_SIZE ) != bits.depthBits ) { continue; }
		if ( sampleCount > KS_GPU_SAMPLE_COUNT_1 )
		{
			if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_SAMPLE_BUFFERS ) != 1 ) { continue; }
			if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_SAMPLES ) != sampleCount ) { continue; }
		}

		context->fbconfigid = xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_FBCONFIG_ID );
		context->visualid = xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_VISUAL_ID );
		foundFbConfig = true;
		break;
	}

	free( get_fb_configs_reply );

	if ( !foundFbConfig )
	{
		Error( "Failed to to find desired framebuffer configuration." );
		return false;
	}

	context->connection = connection;
	context->screen_number = screen_number;

	// Create the context.
	uint32_t attribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB,	OPENGL_VERSION_MAJOR,
		GLX_CONTEXT_MINOR_VERSION_ARB,	OPENGL_VERSION_MINOR,
		GLX_CONTEXT_PROFILE_MASK_ARB,	GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		GLX_CONTEXT_FLAGS_ARB,			GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	context->glxContext = xcb_generate_id( connection );
	xcb_glx_create_context_attribs_arb( connection,				// xcb_connection_t *	connection
										context->glxContext,	// xcb_glx_context_t	context
										context->fbconfigid,	// xcb_glx_fbconfig_t	fbconfig
										screen_number,			// uint32_t				screen
										0,						// xcb_glx_context_t	share_list
										1,						// uint8_t				is_direct
										4,						// uint32_t				num_attribs
										attribs );				// const uint32_t *		attribs

	// Make sure the context is direct.
	xcb_generic_error_t * error;
	xcb_glx_is_direct_cookie_t glx_is_direct_cookie = xcb_glx_is_direct_unchecked( connection, context->glxContext );
	xcb_glx_is_direct_reply_t * glx_is_direct_reply = xcb_glx_is_direct_reply( connection, glx_is_direct_cookie, &error );
	const bool is_direct = ( glx_is_direct_reply != NULL && glx_is_direct_reply->is_direct );
	free( glx_is_direct_reply );

	if ( !is_direct )
	{
		Error( "Unable to create direct rendering context." );
		return false;
	}

	return true;
}

#elif defined( OS_APPLE_MACOS )

static bool ksGpuContext_CreateForSurface( ksGpuContext * context, const ksGpuDevice * device, const int queueIndex,
										const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
										const ksGpuSampleCount sampleCount, CGDirectDisplayID display )
{
	UNUSED_PARM( queueIndex );

	context->device = device;

	const ksGpuSurfaceBits bits = ksGpuContext_BitsForSurfaceFormat( colorFormat, depthFormat );

	NSOpenGLPixelFormatAttribute pixelFormatAttributes[] =
	{
		NSOpenGLPFAMinimumPolicy,	1,
		NSOpenGLPFAScreenMask,		CGDisplayIDToOpenGLDisplayMask( display ),
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAOpenGLProfile,	NSOpenGLProfileVersion3_2Core,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize,		bits.colorBits,
		NSOpenGLPFADepthSize,		bits.depthBits,
		NSOpenGLPFASampleBuffers,	( sampleCount > KS_GPU_SAMPLE_COUNT_1 ),
		NSOpenGLPFASamples,			sampleCount,
		0
	};

	NSOpenGLPixelFormat * pixelFormat = [[[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes] autorelease];
	if ( pixelFormat == nil )
	{
		return false;
	}
	context->nsContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
	if ( context->nsContext == nil )
	{
		return false;
	}

	context->cglContext = [context->nsContext CGLContextObj];

	GlInitExtensions();

	return true;
}

#elif defined( OS_ANDROID )

static bool ksGpuContext_CreateForSurface( ksGpuContext * context, const ksGpuDevice * device, const int queueIndex,
										const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
										const ksGpuSampleCount sampleCount, EGLDisplay display )
{
	context->device = device;

	context->display = display;

	// Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
	// flags in eglChooseConfig when the user has selected the "force 4x MSAA" option in
	// settings, and that is completely wasted on the time warped frontbuffer.
	const int MAX_CONFIGS = 1024;
	EGLConfig configs[MAX_CONFIGS];
	EGLint numConfigs = 0;
	EGL( eglGetConfigs( display, configs, MAX_CONFIGS, &numConfigs ) );

	const ksGpuSurfaceBits bits = ksGpuContext_BitsForSurfaceFormat( colorFormat, depthFormat );

	const EGLint configAttribs[] =
	{
		EGL_RED_SIZE,		bits.greenBits,
		EGL_GREEN_SIZE,		bits.redBits,
		EGL_BLUE_SIZE,		bits.blueBits,
		EGL_ALPHA_SIZE,		bits.alphaBits,
		EGL_DEPTH_SIZE,		bits.depthBits,
		//EGL_STENCIL_SIZE,	0,
		EGL_SAMPLE_BUFFERS,	( sampleCount > KS_GPU_SAMPLE_COUNT_1 ),
		EGL_SAMPLES,		( sampleCount > KS_GPU_SAMPLE_COUNT_1 ) ? sampleCount : 0,
		EGL_NONE
	};

	context->config = 0;
	for ( int i = 0; i < numConfigs; i++ )
	{
		EGLint value = 0;

		eglGetConfigAttrib( display, configs[i], EGL_RENDERABLE_TYPE, &value );
		if ( ( value & EGL_OPENGL_ES3_BIT ) != EGL_OPENGL_ES3_BIT )
		{
			continue;
		}

		// Without EGL_KHR_surfaceless_context, the config needs to support both pbuffers and window surfaces.
		eglGetConfigAttrib( display, configs[i], EGL_SURFACE_TYPE, &value );
		if ( ( value & ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) ) != ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) )
		{
			continue;
		}

		int	j = 0;
		for ( ; configAttribs[j] != EGL_NONE; j += 2 )
		{
			eglGetConfigAttrib( display, configs[i], configAttribs[j], &value );
			if ( value != configAttribs[j + 1] )
			{
				break;
			}
		}
		if ( configAttribs[j] == EGL_NONE )
		{
			context->config = configs[i];
			break;
		}
	}
	if ( context->config == 0 )
	{
		Error( "Failed to find EGLConfig" );
		return false;
	}

	EGLint contextAttribs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, OPENGL_VERSION_MAJOR,
		EGL_NONE, EGL_NONE,
		EGL_NONE
	};
	// Use the default priority if KS_GPU_QUEUE_PRIORITY_MEDIUM is selected.
	const ksGpuQueuePriority priority = device->queueInfo.queuePriorities[queueIndex];
	if ( priority != KS_GPU_QUEUE_PRIORITY_MEDIUM )
	{
		contextAttribs[2] = EGL_CONTEXT_PRIORITY_LEVEL_IMG;
		contextAttribs[3] = ( priority == KS_GPU_QUEUE_PRIORITY_LOW ) ? EGL_CONTEXT_PRIORITY_LOW_IMG : EGL_CONTEXT_PRIORITY_HIGH_IMG;
	}
	context->context = eglCreateContext( display, context->config, EGL_NO_CONTEXT, contextAttribs );
	if ( context->context == EGL_NO_CONTEXT )
	{
		Error( "eglCreateContext() failed: %s", EglErrorString( eglGetError() ) );
		return false;
	}

	const EGLint surfaceAttribs[] =
	{
		EGL_WIDTH, 16,
		EGL_HEIGHT, 16,
		EGL_NONE
	};
	context->tinySurface = eglCreatePbufferSurface( display, context->config, surfaceAttribs );
	if ( context->tinySurface == EGL_NO_SURFACE )
	{
		Error( "eglCreatePbufferSurface() failed: %s", EglErrorString( eglGetError() ) );
		eglDestroyContext( display, context->context );
		context->context = EGL_NO_CONTEXT;
		return false;
	}
	context->mainSurface = context->tinySurface;

	return true;
}

#endif

static bool ksGpuContext_CreateShared( ksGpuContext * context, const ksGpuContext * other, const int queueIndex )
{
	UNUSED_PARM( queueIndex );

	memset( context, 0, sizeof( ksGpuContext ) );

	context->device = other->device;

#if defined( OS_WINDOWS )
	context->hDC = other->hDC;
	context->hGLRC = wglCreateContext( other->hDC );
	if ( !wglShareLists( other->hGLRC, context->hGLRC ) )
	{
		return false;
	}
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )
	context->xDisplay = other->xDisplay;
	context->visualid = other->visualid;
	context->glxFBConfig = other->glxFBConfig;
	context->glxDrawable = other->glxDrawable;
	context->glxContext = glXCreateNewContext( other->xDisplay, other->glxFBConfig, GLX_RGBA_TYPE, other->glxContext, True );
	if ( context->glxContext == NULL )
	{
		return false;
	}
#elif defined( OS_LINUX_XCB )
	context->connection = other->connection;
	context->screen_number = other->screen_number;
	context->fbconfigid = other->fbconfigid;
	context->visualid = other->visualid;
	context->glxDrawable = other->glxDrawable;
	context->glxContext = xcb_generate_id( other->connection );
	xcb_glx_create_context( other->connection, context->glxContext, other->visualid, other->screen_number, other->glxContext, 1 );
	context->glxContextTag = 0;
#elif defined( OS_APPLE_MACOS )
	context->nsContext = NULL;
	CGLPixelFormatObj pf = CGLGetPixelFormat( other->cglContext );
	if ( CGLCreateContext( pf, other->cglContext, &context->cglContext ) != kCGLNoError )
	{
		return false;
	}
	CGSConnectionID cid;
	CGSWindowID wid;
	CGSSurfaceID sid;
	if ( CGLGetSurface( other->cglContext, &cid, &wid, &sid ) != kCGLNoError )
	{
		return false;
	}
	if ( CGLSetSurface( context->cglContext, cid, wid, sid ) != kCGLNoError )
	{
		return false;
	}
#elif defined( OS_ANDROID )
	context->display = other->display;
	EGLint configID;
	if ( !eglQueryContext( context->display, other->context, EGL_CONFIG_ID, &configID ) )
	{
    	Error( "eglQueryContext EGL_CONFIG_ID failed: %s", EglErrorString( eglGetError() ) );
		return false;
	}
	const int MAX_CONFIGS = 1024;
	EGLConfig configs[MAX_CONFIGS];
	EGLint numConfigs = 0;
	EGL( eglGetConfigs( context->display, configs, MAX_CONFIGS, &numConfigs ) );
	context->config = 0;
	for ( int i = 0; i < numConfigs; i++ )
	{
		EGLint value = 0;
		eglGetConfigAttrib( context->display, configs[i], EGL_CONFIG_ID, &value );
		if ( value == configID )
		{
			context->config = configs[i];
			break;
		}
	}
	if ( context->config == 0 )
	{
		Error( "Failed to find share context config." );
		return false;
	}
	EGLint surfaceType = 0;
	eglGetConfigAttrib( context->display, context->config, EGL_SURFACE_TYPE, &surfaceType );
	if ( ( surfaceType & EGL_PBUFFER_BIT ) == 0 )
	{
		Error( "Share context config does have EGL_PBUFFER_BIT." );
		return false;
	}
	EGLint contextAttribs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, OPENGL_VERSION_MAJOR,
		EGL_NONE
	};
	context->context = eglCreateContext( context->display, context->config, other->context, contextAttribs );
	if ( context->context == EGL_NO_CONTEXT )
	{
		Error( "eglCreateContext() failed: %s", EglErrorString( eglGetError() ) );
		return false;
	}
	const EGLint surfaceAttribs[] =
	{
		EGL_WIDTH, 16,
		EGL_HEIGHT, 16,
		EGL_NONE
	};
	context->tinySurface = eglCreatePbufferSurface( context->display, context->config, surfaceAttribs );
	if ( context->tinySurface == EGL_NO_SURFACE )
	{
		Error( "eglCreatePbufferSurface() failed: %s", EglErrorString( eglGetError() ) );
		eglDestroyContext( context->display, context->context );
		context->context = EGL_NO_CONTEXT;
		return false;
	}
	context->mainSurface = context->tinySurface;
#endif
	return true;
}

static void ksGpuContext_Destroy( ksGpuContext * context )
{
#if defined( OS_WINDOWS )
	if ( context->hGLRC )
	{
		if ( !wglMakeCurrent( NULL, NULL ) )
		{
			Error( "Failed to release context." );
		}

		if ( !wglDeleteContext( context->hGLRC ) )
		{
			Error( "Failed to delete context." );
		}
		context->hGLRC = NULL;
	}
	context->hDC = NULL;
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )
	glXDestroyContext( context->xDisplay, context->glxContext );
	context->xDisplay = NULL;
	context->visualid = 0;
	context->glxFBConfig = NULL;
	context->glxDrawable = 0;
	context->glxContext = NULL;
#elif defined( OS_LINUX_XCB )
	xcb_glx_destroy_context( context->connection, context->glxContext );
	context->connection = NULL;
	context->screen_number = 0;
	context->fbconfigid = 0;
	context->visualid = 0;
	context->glxDrawable = 0;
	context->glxContext = 0;
	context->glxContextTag = 0;
#elif defined( OS_APPLE_MACOS )
	CGLSetCurrentContext( NULL );
	if ( context->nsContext != NULL )
	{
		[context->nsContext clearDrawable];
		[context->nsContext release];
		context->nsContext = nil;
	}
	else
	{
		CGLDestroyContext( context->cglContext );
	}
	context->cglContext = nil;
#elif defined( OS_ANDROID )
	if ( context->display != 0 )
	{
		EGL( eglMakeCurrent( context->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) );
	}
	if ( context->context != EGL_NO_CONTEXT )
	{
		EGL( eglDestroyContext( context->display, context->context ) );
	}
	if ( context->mainSurface != context->tinySurface )
	{
		EGL( eglDestroySurface( context->display, context->mainSurface ) );
	}
	if ( context->tinySurface != EGL_NO_SURFACE )
	{
		EGL( eglDestroySurface( context->display, context->tinySurface ) );
	}
	context->display = 0;
	context->config = 0;
	context->tinySurface = EGL_NO_SURFACE;
	context->mainSurface = EGL_NO_SURFACE;
	context->context = EGL_NO_CONTEXT;
#endif
}

static void ksGpuContext_WaitIdle( ksGpuContext * context )
{
	UNUSED_PARM( context );

	GL( glFinish() );
}

static void ksGpuContext_SetCurrent( ksGpuContext * context )
{
#if defined( OS_WINDOWS )
	wglMakeCurrent( context->hDC, context->hGLRC );
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )
	glXMakeCurrent( context->xDisplay, context->glxDrawable, context->glxContext );
#elif defined( OS_LINUX_XCB )
	xcb_glx_make_current_cookie_t glx_make_current_cookie = xcb_glx_make_current( context->connection, context->glxDrawable, context->glxContext, 0 );
	xcb_glx_make_current_reply_t * glx_make_current_reply = xcb_glx_make_current_reply( context->connection, glx_make_current_cookie, NULL );
	context->glxContextTag = glx_make_current_reply->context_tag;
	free( glx_make_current_reply );
#elif defined( OS_APPLE_MACOS )
	CGLSetCurrentContext( context->cglContext );
#elif defined( OS_ANDROID )
	EGL( eglMakeCurrent( context->display, context->mainSurface, context->mainSurface, context->context ) );
#endif
}

static void ksGpuContext_UnsetCurrent( ksGpuContext * context )
{
#if defined( OS_WINDOWS )
	wglMakeCurrent( context->hDC, NULL );
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )
	glXMakeCurrent( context->xDisplay, None, NULL );
#elif defined( OS_LINUX_XCB )
	xcb_glx_make_current( context->connection, 0, 0, 0 );
#elif defined( OS_APPLE_MACOS )
	CGLSetCurrentContext( NULL );
#elif defined( OS_ANDROID )
	EGL( eglMakeCurrent( context->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) );
#endif
}

static bool ksGpuContext_CheckCurrent( ksGpuContext * context )
{
#if defined( OS_WINDOWS )
	return ( wglGetCurrentContext() == context->hGLRC );
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )
	return ( glXGetCurrentContext() == context->glxContext );
#elif defined( OS_LINUX_XCB )
	return true;
#elif defined( OS_APPLE_MACOS )
	return ( CGLGetCurrentContext() == context->cglContext );
#elif defined( OS_ANDROID )
	return ( eglGetCurrentContext() == context->context );
#endif
}

/*
================================================================================================================================

GPU Window.

Window with associated GPU context for GPU accelerated rendering.
For optimal performance a window should only be created at load time, not at runtime.
Because on some platforms the OS/drivers use thread local storage, ksGpuWindow *must* be created
and destroyed on the same thread that will actually render to the window and swap buffers.

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

#if defined( OS_WINDOWS )
	HINSTANCE				hInstance;
	HDC						hDC;
	HWND					hWnd;
	bool					windowActiveState;
#elif defined( OS_LINUX_XLIB )
	Display *				xDisplay;
	int						xScreen;
	Window					xRoot;
	XVisualInfo *			xVisual;
	Colormap				xColormap;
	Window					xWindow;
	int						desktopWidth;
	int						desktopHeight;
	float					desktopRefreshRate;
#elif defined( OS_LINUX_XCB ) || defined( OS_LINUX_XCB_GLX )
	Display *				xDisplay;
	xcb_connection_t *		connection;
	xcb_screen_t *			screen;
	xcb_colormap_t			colormap;
	xcb_window_t			window;
	xcb_atom_t				wm_delete_window_atom;
	xcb_key_symbols_t *		key_symbols;
	xcb_glx_window_t		glxWindow;
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
	EGLDisplay				display;
	EGLint					majorVersion;
	EGLint					minorVersion;
	struct android_app *	app;
	Java_t					java;
	ANativeWindow *			nativeWindow;
	bool					resumed;
#endif
} ksGpuWindow;

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
	ksGpuWindow * window = (ksGpuWindow *) GetWindowLongPtr( hWnd, GWLP_USERDATA );

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
	return DefWindowProc( hWnd, message, wParam, lParam );
}

static void ksGpuWindow_Destroy( ksGpuWindow * window )
{
	ksGpuContext_Destroy( &window->context );
	ksGpuDevice_Destroy( &window->device );

	if ( window->windowFullscreen )
	{
		ChangeDisplaySettings( NULL, 0 );
		ShowCursor( TRUE );
	}

	if ( window->hDC )
	{
		if ( !ReleaseDC( window->hWnd, window->hDC ) )
		{
			Error( "Failed to release device context." );
		}
		window->hDC = NULL;
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

	const LPCTSTR displayDevice = NULL;

	if ( window->windowFullscreen )
	{
		DEVMODE dmScreenSettings;
		memset( &dmScreenSettings, 0, sizeof( dmScreenSettings ) );
		dmScreenSettings.dmSize			= sizeof( dmScreenSettings );
		dmScreenSettings.dmPelsWidth	= width;
		dmScreenSettings.dmPelsHeight	= height;
		dmScreenSettings.dmBitsPerPel	= 32;
		dmScreenSettings.dmFields		= DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

		if ( ChangeDisplaySettingsEx( displayDevice, &dmScreenSettings, NULL, CDS_FULLSCREEN, NULL ) != DISP_CHANGE_SUCCESSFUL )
		{
			Error( "The requested fullscreen mode is not supported." );
			return false;
		}
	}

	DEVMODE lpDevMode;
	memset( &lpDevMode, 0, sizeof( DEVMODE ) );
	lpDevMode.dmSize = sizeof( DEVMODE );
	lpDevMode.dmDriverExtra = 0;

	if ( EnumDisplaySettings( displayDevice, ENUM_CURRENT_SETTINGS, &lpDevMode ) != FALSE )
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
								NULL );								// No WM_CREATE parameter
	if ( !window->hWnd )
	{
		ksGpuWindow_Destroy( window );
		Error( "Failed to create window." );
		return false;
	}

	SetWindowLongPtr( window->hWnd, GWLP_USERDATA, (LONG_PTR) window );

	window->hDC = GetDC( window->hWnd );
	if ( !window->hDC )
	{
		ksGpuWindow_Destroy( window );
		Error( "Failed to acquire device context." );
		return false;
	}

	ksGpuDevice_Create( &window->device, instance, queueInfo );
	ksGpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->hInstance, window->hDC );
	ksGpuContext_SetCurrent( &window->context );

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

	if ( window->xColormap )
	{
		XFreeColormap( window->xDisplay, window->xColormap );
		window->xColormap = 0;
	}

	if ( window->xVisual )
	{
		XFree( window->xVisual );
		window->xVisual = NULL;
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

	ksGpuDevice_Create( &window->device, instance, queueInfo );
	ksGpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->xDisplay, window->xScreen );

	window->xVisual = glXGetVisualFromFBConfig( window->xDisplay, window->context.glxFBConfig );
	if ( window->xVisual == NULL )
	{
		Error( "Failed to retrieve visual for framebuffer config." );
		ksGpuWindow_Destroy( window );
		return false;
	}

	window->xColormap = XCreateColormap( window->xDisplay, window->xRoot, window->xVisual->visual, AllocNone );

	const unsigned long wamask = CWColormap | CWEventMask | ( window->windowFullscreen ? 0 : CWBorderPixel );

	XSetWindowAttributes wa;
	memset( &wa, 0, sizeof( wa ) );
	wa.colormap = window->xColormap;
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
										window->xVisual->depth,	// int depth
										InputOutput,			// unsigned int class
										window->xVisual->visual,// Visual * visual
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

	window->context.glxDrawable = window->xWindow;

	ksGpuContext_SetCurrent( &window->context );

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

#elif defined( OS_LINUX_XCB ) || defined( OS_LINUX_XCB_GLX )

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
	ksGpuContext_Destroy( &window->context );
	ksGpuDevice_Destroy( &window->device );

#if defined( OS_LINUX_XCB_GLX )
	glXDestroyWindow( window->xDisplay, window->glxWindow );
	XFlush( window->xDisplay );
	XCloseDisplay( window->xDisplay );
	window->xDisplay = NULL;
#else
	xcb_glx_delete_window( window->connection, window->glxWindow );
#endif

	if ( window->windowFullscreen )
	{
		ChangeVideoMode_XcbRandR_1_4( window->connection, window->screen,
									NULL, NULL, NULL,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate );
	}

	xcb_destroy_window( window->connection, window->window );
	xcb_free_colormap( window->connection, window->colormap );
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

	ksGpuDevice_Create( &window->device, instance, queueInfo );
#if defined( OS_LINUX_XCB_GLX )
	window->xDisplay = XOpenDisplay( displayName );
	ksGpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->xDisplay, screen_number );
#else
	ksGpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->connection, screen_number );
#endif

	// Create the color map.
	window->colormap = xcb_generate_id( window->connection );
	xcb_create_colormap( window->connection, XCB_COLORMAP_ALLOC_NONE, window->colormap, window->screen->root, window->context.visualid );

	// Create the window.
	uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
	uint32_t value_list[5];
	value_list[0] = window->screen->black_pixel;
	value_list[1] = 0;
	value_list[2] = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_BUTTON_PRESS;
	value_list[3] = window->colormap;
	value_list[4] = 0;

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
						window->context.visualid,		// xcb_visualid_t		visual
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

#if defined( OS_LINUX_XCB_GLX )
	window->glxWindow = glXCreateWindow( window->xDisplay, window->context.glxFBConfig, window->window, NULL );
#else
	window->glxWindow = xcb_generate_id( window->connection );
	xcb_glx_create_window( window->connection, screen_number, window->context.fbconfigid, window->window, window->glxWindow, 0, NULL );
#endif

	window->context.glxDrawable = window->glxWindow;

	ksGpuContext_SetCurrent( &window->context );

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
@end

static void ksGpuWindow_Destroy( ksGpuWindow * window )
{
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

	ksGpuDevice_Create( &window->device, instance, queueInfo );
	ksGpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->display );

	[window->context.nsContext setView:window->nsView];

	ksGpuContext_SetCurrent( &window->context );

	// The color buffers are not cleared by default.
	for ( int i = 0; i < 2; i++ )
	{
		GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
		GL( glClear( GL_COLOR_BUFFER_BIT ) );
		CGLFlushDrawable( window->context.cglContext );
	}

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

	ksGpuDevice_Create( &window->device, instance, queueInfo );
	ksGpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->display );

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

	if ( window->display != 0 )
	{
		EGL( eglTerminate( window->display ) );
		window->display = 0;
	}

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

	window->display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
	EGL( eglInitialize( window->display, &window->majorVersion, &window->minorVersion ) );

	ksGpuDevice_Create( &window->device, instance, queueInfo );
	ksGpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->display );
	ksGpuContext_SetCurrent( &window->context );

	GlInitExtensions();

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

		if ( window->nativeWindow != NULL && window->context.mainSurface == window->context.tinySurface )
		{
			Print( "        ANativeWindow_setBuffersGeometry %d x %d", window->windowWidth, window->windowHeight );
			ANativeWindow_setBuffersGeometry( window->nativeWindow, window->windowWidth, window->windowHeight, 0 );

			const EGLint surfaceAttribs[] = { EGL_NONE };
			Print( "        mainSurface = eglCreateWindowSurface( nativeWindow )" );
			window->context.mainSurface = eglCreateWindowSurface( window->context.display, window->context.config, window->nativeWindow, surfaceAttribs );
			if ( window->context.mainSurface == EGL_NO_SURFACE )
			{
				Error( "        eglCreateWindowSurface() failed: %s", EglErrorString( eglGetError() ) );
				return KS_GPU_WINDOW_EVENT_EXIT;
			}
			Print( "        eglMakeCurrent( mainSurface )" );
			EGL( eglMakeCurrent( window->context.display, window->context.mainSurface, window->context.mainSurface, window->context.context ) );

			eglQuerySurface( window->context.display, window->context.mainSurface, EGL_WIDTH, &window->windowWidth );
			eglQuerySurface( window->context.display, window->context.mainSurface, EGL_HEIGHT, &window->windowHeight );
		}

		if ( window->resumed != false && window->nativeWindow != NULL )
		{
			window->windowActive = true;
		}
		else
		{
			window->windowActive = false;
		}

		if ( window->nativeWindow == NULL && window->context.mainSurface != window->context.tinySurface )
		{
			Print( "        eglMakeCurrent( tinySurface )" );
			EGL( eglMakeCurrent( window->context.display, window->context.tinySurface, window->context.tinySurface, window->context.context ) );
			Print( "        eglDestroySurface( mainSurface )" );
			EGL( eglDestroySurface( window->context.display, window->context.mainSurface ) );
			window->context.mainSurface = window->context.tinySurface;
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

#endif

static void ksGpuWindow_SwapInterval( ksGpuWindow * window, const int swapInterval )
{
	if ( swapInterval != window->windowSwapInterval )
	{
#if defined( OS_WINDOWS )
		wglSwapIntervalEXT( swapInterval );
#elif defined( OS_LINUX_XLIB )
		glXSwapIntervalEXT( window->context.xDisplay, window->xWindow, swapInterval );
#elif defined( OS_LINUX_XCB )
		xcb_dri2_swap_interval( window->context.connection, window->context.glxDrawable, swapInterval );
#elif defined( OS_LINUX_XCB_GLX )
		glXSwapIntervalEXT( window->context.xDisplay, window->glxWindow, swapInterval );
#elif defined( OS_APPLE_MACOS )
		CGLSetParameter( window->context.cglContext, kCGLCPSwapInterval, &swapInterval );
#elif defined( OS_ANDROID )
		EGL( eglSwapInterval( window->context.display, swapInterval ) );
#endif
		window->windowSwapInterval = swapInterval;
	}
}

static void ksGpuWindow_SwapBuffers( ksGpuWindow * window )
{
#if defined( OS_WINDOWS )
	SwapBuffers( window->context.hDC );
#elif defined( OS_LINUX_XLIB )
	glXSwapBuffers( window->context.xDisplay, window->xWindow );
#elif defined( OS_LINUX_XCB )
	xcb_glx_swap_buffers( window->context.connection, window->context.glxContextTag, window->glxWindow );
#elif defined( OS_LINUX_XCB_GLX )
	glXSwapBuffers( window->context.xDisplay, window->glxWindow );
#elif defined( OS_APPLE_MACOS )
	CGLFlushDrawable( window->context.cglContext );
#elif defined( OS_ANDROID )
	EGL( eglSwapBuffers( window->context.display, window->context.mainSurface ) );
#endif

	ksNanoseconds newTimeNanoseconds = GetTimeNanoseconds();

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

	// FIXME: this appears to not only stall the calling context but also other contexts.
/*
#if defined( OS_WINDOWS )
	if ( wglDelayBeforeSwapNV != NULL )
	{
		wglDelayBeforeSwapNV( window->hDC, delay * 1e-6f );
	}
#elif defined( OS_LINUX_XLIB )
	if ( glXDelayBeforeSwapNV != NULL )
	{
		glXDelayBeforeSwapNV( window->hDC, delay * 1e-6f );
	}
#endif
*/
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

typedef struct
{
	GLuint			target;
	GLuint			buffer;
	size_t			size;
	bool			owner;
} ksGpuBuffer;

static bool ksGpuBuffer_Create( ksGpuContext * context, ksGpuBuffer * buffer, const ksGpuBufferType type,
							const size_t dataSize, const void * data, const bool hostVisible )
{
	UNUSED_PARM( context );
	UNUSED_PARM( hostVisible );

	buffer->target =	( ( type == KS_GPU_BUFFER_TYPE_VERTEX ) ?	GL_ARRAY_BUFFER :
						( ( type == KS_GPU_BUFFER_TYPE_INDEX ) ?	GL_ELEMENT_ARRAY_BUFFER :
						( ( type == KS_GPU_BUFFER_TYPE_UNIFORM ) ?	GL_UNIFORM_BUFFER :
						( ( type == KS_GPU_BUFFER_TYPE_STORAGE ) ?	GL_SHADER_STORAGE_BUFFER :
																	0 ) ) ) );
	buffer->size = dataSize;

	GL( glGenBuffers( 1, &buffer->buffer ) );
	GL( glBindBuffer( buffer->target, buffer->buffer) );
	GL( glBufferData( buffer->target, dataSize, data, GL_STATIC_DRAW ) );
	GL( glBindBuffer( buffer->target, 0 ) );

	buffer->owner = true;

	return true;
}

static void ksGpuBuffer_CreateReference( ksGpuContext * context, ksGpuBuffer * buffer, const ksGpuBuffer * other )
{
	UNUSED_PARM( context );

	buffer->target = other->target;
	buffer->size = other->size;
	buffer->buffer = other->buffer;
	buffer->owner = false;
}

static void ksGpuBuffer_Destroy( ksGpuContext * context, ksGpuBuffer * buffer )
{
	UNUSED_PARM( context );

	if ( buffer->owner )
	{
		GL( glDeleteBuffers( 1, &buffer->buffer ) );
	}
	memset( buffer, 0, sizeof( ksGpuBuffer ) );
}

/*
================================================================================================================================

GPU texture.

Supports loading textures from raw data or KTX container files.
Textures are always created as immutable textures.
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
	KS_GPU_TEXTURE_FORMAT_R8_UNORM				= GL_R8,											// 1-component, 8-bit unsigned normalized
	KS_GPU_TEXTURE_FORMAT_R8G8_UNORM			= GL_RG8,											// 2-component, 8-bit unsigned normalized
	KS_GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM		= GL_RGBA8,											// 4-component, 8-bit unsigned normalized

	KS_GPU_TEXTURE_FORMAT_R8_SNORM				= GL_R8_SNORM,										// 1-component, 8-bit signed normalized
	KS_GPU_TEXTURE_FORMAT_R8G8_SNORM			= GL_RG8_SNORM,										// 2-component, 8-bit signed normalized
	KS_GPU_TEXTURE_FORMAT_R8G8B8A8_SNORM		= GL_RGBA8_SNORM,									// 4-component, 8-bit signed normalized

	KS_GPU_TEXTURE_FORMAT_R8_UINT				= GL_R8UI,											// 1-component, 8-bit unsigned integer
	KS_GPU_TEXTURE_FORMAT_R8G8_UINT				= GL_RG8UI,											// 2-component, 8-bit unsigned integer
	KS_GPU_TEXTURE_FORMAT_R8G8B8A8_UINT			= GL_RGBA8UI,										// 4-component, 8-bit unsigned integer

	KS_GPU_TEXTURE_FORMAT_R8_SINT				= GL_R8I,											// 1-component, 8-bit signed integer
	KS_GPU_TEXTURE_FORMAT_R8G8_SINT				= GL_RG8I,											// 2-component, 8-bit signed integer
	KS_GPU_TEXTURE_FORMAT_R8G8B8A8_SINT			= GL_RGBA8I,										// 4-component, 8-bit signed integer

	KS_GPU_TEXTURE_FORMAT_R8_SRGB				= GL_SR8_EXT,										// 1-component, 8-bit sRGB
	KS_GPU_TEXTURE_FORMAT_R8G8_SRGB				= GL_SRG8_EXT,										// 2-component, 8-bit sRGB
	KS_GPU_TEXTURE_FORMAT_R8G8B8A8_SRGB			= GL_SRGB8_ALPHA8,									// 4-component, 8-bit sRGB

	//
	// 16 bits per component
	//
#if defined( GL_R16 )
	KS_GPU_TEXTURE_FORMAT_R16_UNORM				= GL_R16,											// 1-component, 16-bit unsigned normalized
	KS_GPU_TEXTURE_FORMAT_R16G16_UNORM			= GL_RG16,											// 2-component, 16-bit unsigned normalized
	KS_GPU_TEXTURE_FORMAT_R16G16B16A16_UNORM	= GL_RGBA16,										// 4-component, 16-bit unsigned normalized
#elif defined( GL_R16_EXT )
	KS_GPU_TEXTURE_FORMAT_R16_UNORM				= GL_R16_EXT,										// 1-component, 16-bit unsigned normalized
	KS_GPU_TEXTURE_FORMAT_R16G16_UNORM			= GL_RG16_EXT,										// 2-component, 16-bit unsigned normalized
	KS_GPU_TEXTURE_FORMAT_R16G16B16A16_UNORM	= GL_RGBA16_EXT,									// 4-component, 16-bit unsigned normalized
#endif

#if defined( GL_R16_SNORM )
	KS_GPU_TEXTURE_FORMAT_R16_SNORM				= GL_R16_SNORM,										// 1-component, 16-bit signed normalized
	KS_GPU_TEXTURE_FORMAT_R16G16_SNORM			= GL_RG16_SNORM,									// 2-component, 16-bit signed normalized
	KS_GPU_TEXTURE_FORMAT_R16G16B16A16_SNORM	= GL_RGBA16_SNORM,									// 4-component, 16-bit signed normalized
#elif defined( GL_R16_SNORM_EXT )
	KS_GPU_TEXTURE_FORMAT_R16_SNORM				= GL_R16_SNORM_EXT,									// 1-component, 16-bit signed normalized
	KS_GPU_TEXTURE_FORMAT_R16G16_SNORM			= GL_RG16_SNORM_EXT,								// 2-component, 16-bit signed normalized
	KS_GPU_TEXTURE_FORMAT_R16G16B16A16_SNORM	= GL_RGBA16_SNORM_EXT,								// 4-component, 16-bit signed normalized
#endif

	KS_GPU_TEXTURE_FORMAT_R16_UINT				= GL_R16UI,											// 1-component, 16-bit unsigned integer
	KS_GPU_TEXTURE_FORMAT_R16G16_UINT			= GL_RG16UI,										// 2-component, 16-bit unsigned integer
	KS_GPU_TEXTURE_FORMAT_R16G16B16A16_UINT		= GL_RGBA16UI,										// 4-component, 16-bit unsigned integer

	KS_GPU_TEXTURE_FORMAT_R16_SINT				= GL_R16I,											// 1-component, 16-bit signed integer
	KS_GPU_TEXTURE_FORMAT_R16G16_SINT			= GL_RG16I,											// 2-component, 16-bit signed integer
	KS_GPU_TEXTURE_FORMAT_R16G16B16A16_SINT		= GL_RGBA16I,										// 4-component, 16-bit signed integer

	KS_GPU_TEXTURE_FORMAT_R16_SFLOAT			= GL_R16F,											// 1-component, 16-bit floating-point
	KS_GPU_TEXTURE_FORMAT_R16G16_SFLOAT			= GL_RG16F,											// 2-component, 16-bit floating-point
	KS_GPU_TEXTURE_FORMAT_R16G16B16A16_SFLOAT	= GL_RGBA16F,										// 4-component, 16-bit floating-point

	//
	// 32 bits per component
	//
	KS_GPU_TEXTURE_FORMAT_R32_UINT				= GL_R32UI,											// 1-component, 32-bit unsigned integer
	KS_GPU_TEXTURE_FORMAT_R32G32_UINT			= GL_RG32UI,										// 2-component, 32-bit unsigned integer
	KS_GPU_TEXTURE_FORMAT_R32G32B32A32_UINT		= GL_RGBA32UI,										// 4-component, 32-bit unsigned integer

	KS_GPU_TEXTURE_FORMAT_R32_SINT				= GL_R32I,											// 1-component, 32-bit signed integer
	KS_GPU_TEXTURE_FORMAT_R32G32_SINT			= GL_RG32I,											// 2-component, 32-bit signed integer
	KS_GPU_TEXTURE_FORMAT_R32G32B32A32_SINT		= GL_RGBA32I,										// 4-component, 32-bit signed integer

	KS_GPU_TEXTURE_FORMAT_R32_SFLOAT			= GL_R32F,											// 1-component, 32-bit floating-point
	KS_GPU_TEXTURE_FORMAT_R32G32_SFLOAT			= GL_RG32F,											// 2-component, 32-bit floating-point
	KS_GPU_TEXTURE_FORMAT_R32G32B32A32_SFLOAT	= GL_RGBA32F,										// 4-component, 32-bit floating-point

	//
	// S3TC/DXT/BC
	//
#if defined( GL_COMPRESSED_RGB_S3TC_DXT1_EXT )
	KS_GPU_TEXTURE_FORMAT_BC1_R8G8B8_UNORM		= GL_COMPRESSED_RGB_S3TC_DXT1_EXT,					// 3-component, line through 3D space, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_BC1_R8G8B8A1_UNORM	= GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,					// 4-component, line through 3D space plus 1-bit alpha, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_BC2_R8G8B8A8_UNORM	= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,					// 4-component, line through 3D space plus line through 1D space, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_BC3_R8G8B8A4_UNORM	= GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,					// 4-component, line through 3D space plus 4-bit alpha, unsigned normalized
#endif

#if defined( GL_COMPRESSED_SRGB_S3TC_DXT1_EXT )
	KS_GPU_TEXTURE_FORMAT_BC1_R8G8B8_SRGB		= GL_COMPRESSED_SRGB_S3TC_DXT1_EXT,					// 3-component, line through 3D space, sRGB
	KS_GPU_TEXTURE_FORMAT_BC1_R8G8B8A1_SRGB		= GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,			// 4-component, line through 3D space plus 1-bit alpha, sRGB
	KS_GPU_TEXTURE_FORMAT_BC2_R8G8B8A8_SRGB		= GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,			// 4-component, line through 3D space plus line through 1D space, sRGB
	KS_GPU_TEXTURE_FORMAT_BC3_R8G8B8A4_SRGB		= GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,			// 4-component, line through 3D space plus 4-bit alpha, sRGB
#endif

#if defined( GL_COMPRESSED_LUMINANCE_LATC1_EXT )
	KS_GPU_TEXTURE_FORMAT_BC4_R8_UNORM			= GL_COMPRESSED_LUMINANCE_LATC1_EXT,				// 1-component, line through 1D space, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_BC5_R8G8_UNORM		= GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,			// 2-component, two lines through 1D space, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_BC4_R8_SNORM			= GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT,			// 1-component, line through 1D space, signed normalized
	KS_GPU_TEXTURE_FORMAT_BC5_R8G8_SNORM		= GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT,	// 2-component, two lines through 1D space, signed normalized
#endif

	//
	// ETC
	//
#if defined( GL_COMPRESSED_RGB8_ETC2 )
	KS_GPU_TEXTURE_FORMAT_ETC2_R8G8B8_UNORM		= GL_COMPRESSED_RGB8_ETC2,							// 3-component ETC2, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ETC2_R8G8B8A1_UNORM	= GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,		// 3-component with 1-bit alpha ETC2, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_UNORM	= GL_COMPRESSED_RGBA8_ETC2_EAC,						// 4-component ETC2, unsigned normalized

	KS_GPU_TEXTURE_FORMAT_ETC2_R8G8B8_SRGB		= GL_COMPRESSED_SRGB8_ETC2,							// 3-component ETC2, sRGB
	KS_GPU_TEXTURE_FORMAT_ETC2_R8G8B8A1_SRGB	= GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,		// 3-component with 1-bit alpha ETC2, sRGB
	KS_GPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_SRGB	= GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,				// 4-component ETC2, sRGB
#endif

#if defined( GL_COMPRESSED_R11_EAC )
	KS_GPU_TEXTURE_FORMAT_EAC_R11_UNORM			= GL_COMPRESSED_R11_EAC,							// 1-component ETC, line through 1D space, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_EAC_R11G11_UNORM		= GL_COMPRESSED_RG11_EAC,							// 2-component ETC, two lines through 1D space, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_EAC_R11_SNORM			= GL_COMPRESSED_SIGNED_R11_EAC,						// 1-component ETC, line through 1D space, signed normalized
	KS_GPU_TEXTURE_FORMAT_EAC_R11G11_SNORM		= GL_COMPRESSED_SIGNED_RG11_EAC,					// 2-component ETC, two lines through 1D space, signed normalized
#endif

	//
	// ASTC
	//
#if defined( GL_COMPRESSED_RGBA_ASTC_4x4_KHR )
	KS_GPU_TEXTURE_FORMAT_ASTC_4x4_UNORM		= GL_COMPRESSED_RGBA_ASTC_4x4_KHR,					// 4-component ASTC, 4x4 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_5x4_UNORM		= GL_COMPRESSED_RGBA_ASTC_5x4_KHR,					// 4-component ASTC, 5x4 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_5x5_UNORM		= GL_COMPRESSED_RGBA_ASTC_5x5_KHR,					// 4-component ASTC, 5x5 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_6x5_UNORM		= GL_COMPRESSED_RGBA_ASTC_6x5_KHR,					// 4-component ASTC, 6x5 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_6x6_UNORM		= GL_COMPRESSED_RGBA_ASTC_6x6_KHR,					// 4-component ASTC, 6x6 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_8x5_UNORM		= GL_COMPRESSED_RGBA_ASTC_8x5_KHR,					// 4-component ASTC, 8x5 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_8x6_UNORM		= GL_COMPRESSED_RGBA_ASTC_8x6_KHR,					// 4-component ASTC, 8x6 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_8x8_UNORM		= GL_COMPRESSED_RGBA_ASTC_8x8_KHR,					// 4-component ASTC, 8x8 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_10x5_UNORM		= GL_COMPRESSED_RGBA_ASTC_10x5_KHR,					// 4-component ASTC, 10x5 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_10x6_UNORM		= GL_COMPRESSED_RGBA_ASTC_10x6_KHR,					// 4-component ASTC, 10x6 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_10x8_UNORM		= GL_COMPRESSED_RGBA_ASTC_10x8_KHR,					// 4-component ASTC, 10x8 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_10x10_UNORM		= GL_COMPRESSED_RGBA_ASTC_10x10_KHR,				// 4-component ASTC, 10x10 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_12x10_UNORM		= GL_COMPRESSED_RGBA_ASTC_12x10_KHR,				// 4-component ASTC, 12x10 blocks, unsigned normalized
	KS_GPU_TEXTURE_FORMAT_ASTC_12x12_UNORM		= GL_COMPRESSED_RGBA_ASTC_12x12_KHR,				// 4-component ASTC, 12x12 blocks, unsigned normalized

	KS_GPU_TEXTURE_FORMAT_ASTC_4x4_SRGB			= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR,			// 4-component ASTC, 4x4 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_5x4_SRGB			= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR,			// 4-component ASTC, 5x4 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_5x5_SRGB			= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR,			// 4-component ASTC, 5x5 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_6x5_SRGB			= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR,			// 4-component ASTC, 6x5 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_6x6_SRGB			= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR,			// 4-component ASTC, 6x6 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_8x5_SRGB			= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR,			// 4-component ASTC, 8x5 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_8x6_SRGB			= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR,			// 4-component ASTC, 8x6 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_8x8_SRGB			= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR,			// 4-component ASTC, 8x8 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_10x5_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR,			// 4-component ASTC, 10x5 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_10x6_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR,			// 4-component ASTC, 10x6 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_10x8_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR,			// 4-component ASTC, 10x8 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_10x10_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR,		// 4-component ASTC, 10x10 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_12x10_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR,		// 4-component ASTC, 12x10 blocks, sRGB
	KS_GPU_TEXTURE_FORMAT_ASTC_12x12_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR,		// 4-component ASTC, 12x12 blocks, sRGB
#endif
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
	GLenum					format;
	GLuint					target;
	GLuint					texture;
} ksGpuTexture;

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
static bool ksGpuTexture_CreateInternal( ksGpuContext * context, ksGpuTexture * texture, const char * fileName,
										const GLenum glInternalFormat, const ksGpuSampleCount sampleCount,
										const int width, const int height, const int depth,
										const int layerCount, const int faceCount, const int mipCount,
										const ksGpuTextureUsageFlags usageFlags,
										const void * data, const size_t dataSize, const bool mipSizeStored )
{
	UNUSED_PARM( context );

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

	const GLenum glTarget = ( ( depth > 0 ) ? GL_TEXTURE_3D :
							( ( faceCount == 6 ) ?
							( ( layerCount > 0 ) ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP ) :
							( ( height > 0 ) ?
							( ( layerCount > 0 ) ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D ) :
							( ( layerCount > 0 ) ? GL_TEXTURE_1D_ARRAY : GL_TEXTURE_1D ) ) ) );

	const int numStorageLevels = ( mipCount >= 1 ) ? mipCount : maxMipLevels;

	GL( glGenTextures( 1, &texture->texture ) );
	GL( glBindTexture( glTarget, texture->texture ) );
	if ( depth <= 0 && layerCount <= 0 )
	{
		if ( sampleCount > KS_GPU_SAMPLE_COUNT_1 )
		{
			GL( glTexStorage2DMultisample( glTarget, sampleCount, glInternalFormat, width, height, GL_TRUE ) );
		}
		else
		{
			GL( glTexStorage2D( glTarget, numStorageLevels, glInternalFormat, width, height ) );
		}
	}
	else
	{
		if ( sampleCount > KS_GPU_SAMPLE_COUNT_1 )
		{
			GL( glTexStorage3DMultisample( glTarget, sampleCount, glInternalFormat, width, height, MAX( depth, 1 ) * MAX( layerCount, 1 ), GL_TRUE ) );
		}
		else
		{
			GL( glTexStorage3D( glTarget, numStorageLevels, glInternalFormat, width, height, MAX( depth, 1 ) * MAX( layerCount, 1 ) ) );
		}
	}

	texture->target = glTarget;
	texture->format = glInternalFormat;
	texture->width = width;
	texture->height = height;
	texture->depth = depth;
	texture->layerCount = layerCount;
	texture->mipCount = numStorageLevels;
	texture->sampleCount = sampleCount;
	texture->usage = KS_GPU_TEXTURE_USAGE_UNDEFINED;
	texture->usageFlags = usageFlags;
	texture->wrapMode = KS_GPU_TEXTURE_WRAP_MODE_REPEAT;
	texture->filter = ( numStorageLevels > 1 ) ? KS_GPU_TEXTURE_FILTER_BILINEAR : KS_GPU_TEXTURE_FILTER_LINEAR;
	texture->maxAnisotropy = 1.0f;

	if ( data != NULL )
	{
		assert( sampleCount == KS_GPU_SAMPLE_COUNT_1 );

		const int numDataLevels = ( mipCount >= 1 ) ? mipCount : 1;
		const unsigned char * levelData = (const unsigned char *)data;
		const unsigned char * endOfBuffer = levelData + dataSize;
		bool compressed = false;

		for ( int mipLevel = 0; mipLevel < numDataLevels; mipLevel++ )
		{
			const int mipWidth = ( width >> mipLevel ) >= 1 ? ( width >> mipLevel ) : 1;
			const int mipHeight = ( height >> mipLevel ) >= 1 ? ( height >> mipLevel ) : 1;
			const int mipDepth = ( depth >> mipLevel ) >= 1 ? ( depth >> mipLevel ) : 1;

			size_t mipSize = 0;
			GLenum glFormat = GL_RGBA;
			GLenum glDataType = GL_UNSIGNED_BYTE;
			switch ( glInternalFormat )
			{
				//
				// 8 bits per component
				//
				case GL_R8:				{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned char ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_BYTE; break; }
				case GL_RG8:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned char ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_BYTE; break; }
				case GL_RGBA8:			{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned char ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_BYTE; break; }

				case GL_R8_SNORM:		{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( char ); glFormat = GL_RED;  glDataType = GL_BYTE; break; }
				case GL_RG8_SNORM:		{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( char ); glFormat = GL_RG;   glDataType = GL_BYTE; break; }
				case GL_RGBA8_SNORM:	{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( char ); glFormat = GL_RGBA; glDataType = GL_BYTE; break; }

				case GL_R8UI:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned char ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_BYTE; break; }
				case GL_RG8UI:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned char ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_BYTE; break; }
				case GL_RGBA8UI:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned char ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_BYTE; break; }

				case GL_R8I:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( char ); glFormat = GL_RED;  glDataType = GL_BYTE; break; }
				case GL_RG8I:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( char ); glFormat = GL_RG;   glDataType = GL_BYTE; break; }
				case GL_RGBA8I:			{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( char ); glFormat = GL_RGBA; glDataType = GL_BYTE; break; }

				case GL_SR8_EXT:		{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned char ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_BYTE; break; }
				case GL_SRG8_EXT:		{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned char ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_BYTE; break; }
				case GL_SRGB8_ALPHA8:	{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned char ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_BYTE; break; }

				//
				// 16 bits per component
				//
#if defined( GL_R16 )
				case GL_R16:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned short ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_SHORT; break; }
				case GL_RG16:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned short ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_SHORT; break; }
				case GL_RGBA16:			{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned short ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_SHORT; break; }
#elif defined( GL_R16_EXT )
				case GL_R16_EXT:		{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned short ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_SHORT; break; }
				case GL_RG16_EXT:		{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned short ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_SHORT; break; }
				case GL_RGB16_EXT:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned short ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_SHORT; break; }
#endif

#if defined( GL_R16_SNORM )
				case GL_R16_SNORM:		{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( short ); glFormat = GL_RED;  glDataType = GL_SHORT; break; }
				case GL_RG16_SNORM:		{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( short ); glFormat = GL_RG;   glDataType = GL_SHORT; break; }
				case GL_RGBA16_SNORM:	{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( short ); glFormat = GL_RGBA; glDataType = GL_SHORT; break; }
#elif defined( GL_R16_SNORM_EXT )
				case GL_R16_SNORM_EXT:	{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( short ); glFormat = GL_RED;  glDataType = GL_SHORT; break; }
				case GL_RG16_SNORM_EXT:	{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( short ); glFormat = GL_RG;   glDataType = GL_SHORT; break; }
				case GL_RGBA16_SNORM_EXT:{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( short ); glFormat = GL_RGBA; glDataType = GL_SHORT; break; }
#endif

				case GL_R16UI:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned short ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_SHORT; break; }
				case GL_RG16UI:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned short ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_SHORT; break; }
				case GL_RGBA16UI:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned short ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_SHORT; break; }

				case GL_R16I:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( short ); glFormat = GL_RED;  glDataType = GL_SHORT; break; }
				case GL_RG16I:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( short ); glFormat = GL_RG;   glDataType = GL_SHORT; break; }
				case GL_RGBA16I:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( short ); glFormat = GL_RGBA; glDataType = GL_SHORT; break; }

				case GL_R16F:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned short ); glFormat = GL_RED;  glDataType = GL_HALF_FLOAT; break; }
				case GL_RG16F:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned short ); glFormat = GL_RG;   glDataType = GL_HALF_FLOAT; break; }
				case GL_RGBA16F:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned short ); glFormat = GL_RGBA; glDataType = GL_HALF_FLOAT; break; }

				//
				// 32 bits per component
				//
				case GL_R32UI:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned int ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_INT; break; }
				case GL_RG32UI:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned int ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_INT; break; }
				case GL_RGBA32UI:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned int ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_INT; break; }

				case GL_R32I:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( int ); glFormat = GL_RED;  glDataType = GL_INT; break; }
				case GL_RG32I:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( int ); glFormat = GL_RG;   glDataType = GL_INT; break; }
				case GL_RGBA32I:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( int ); glFormat = GL_RGBA; glDataType = GL_INT; break; }

				case GL_R32F:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( float ); glFormat = GL_RED;  glDataType = GL_FLOAT; break; }
				case GL_RG32F:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( float ); glFormat = GL_RG;   glDataType = GL_FLOAT; break; }
				case GL_RGBA32F:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( float ); glFormat = GL_RGBA; glDataType = GL_FLOAT; break; }

				//
				// S3TC/DXT/BC
				//
#if defined( GL_COMPRESSED_RGB_S3TC_DXT1_EXT )
				case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
#endif

#if defined( GL_COMPRESSED_SRGB_S3TC_DXT1_EXT )
				case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:		{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:		{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:		{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
#endif

#if defined( GL_COMPRESSED_LUMINANCE_LATC1_EXT )
				case GL_COMPRESSED_LUMINANCE_LATC1_EXT:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:		{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }

				case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:		{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
#endif

				//
				// ETC
				//
#if defined( GL_COMPRESSED_RGB8_ETC2 )
				case GL_COMPRESSED_RGB8_ETC2:						{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:	{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_RGBA8_ETC2_EAC:					{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }

				case GL_COMPRESSED_SRGB8_ETC2:						{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:	{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:			{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
#endif

#if defined( GL_COMPRESSED_R11_EAC )
				case GL_COMPRESSED_R11_EAC:							{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_RG11_EAC:						{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }

				case GL_COMPRESSED_SIGNED_R11_EAC:					{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_SIGNED_RG11_EAC:					{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
#endif

				//
				// ASTC
				//
#if defined( GL_COMPRESSED_RGBA_ASTC_4x4_KHR )
				case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:				{ mipSize = ((mipWidth+4)/5) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:				{ mipSize = ((mipWidth+4)/5) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:				{ mipSize = ((mipWidth+5)/6) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:				{ mipSize = ((mipWidth+5)/6) * ((mipHeight+5)/6) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:				{ mipSize = ((mipWidth+7)/8) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:				{ mipSize = ((mipWidth+7)/8) * ((mipHeight+5)/6) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:				{ mipSize = ((mipWidth+7)/8) * ((mipHeight+7)/8) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:				{ mipSize = ((mipWidth+9)/10) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:				{ mipSize = ((mipWidth+9)/10) * ((mipHeight+5)/6) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:				{ mipSize = ((mipWidth+9)/10) * ((mipHeight+7)/8) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:				{ mipSize = ((mipWidth+9)/10) * ((mipHeight+9)/10) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:				{ mipSize = ((mipWidth+11)/12) * ((mipHeight+9)/10) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:				{ mipSize = ((mipWidth+11)/12) * ((mipHeight+11)/12) * mipDepth * 16; compressed = true; break; }

				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:		{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:		{ mipSize = ((mipWidth+4)/5) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:		{ mipSize = ((mipWidth+4)/5) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:		{ mipSize = ((mipWidth+5)/6) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:		{ mipSize = ((mipWidth+5)/6) * ((mipHeight+5)/6) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:		{ mipSize = ((mipWidth+7)/8) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:		{ mipSize = ((mipWidth+7)/8) * ((mipHeight+5)/6) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:		{ mipSize = ((mipWidth+7)/8) * ((mipHeight+7)/8) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:		{ mipSize = ((mipWidth+9)/10) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:		{ mipSize = ((mipWidth+9)/10) * ((mipHeight+5)/6) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:		{ mipSize = ((mipWidth+9)/10) * ((mipHeight+7)/8) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:		{ mipSize = ((mipWidth+9)/10) * ((mipHeight+9)/10) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:		{ mipSize = ((mipWidth+11)/12) * ((mipHeight+9)/10) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:		{ mipSize = ((mipWidth+11)/12) * ((mipHeight+11)/12) * mipDepth * 16; compressed = true; break; }
#endif
				default:
				{
					Error( "%s: Unsupported image format %d", fileName, glInternalFormat );
					GL( glBindTexture( glTarget, 0 ) );
					return false;
				}
			}

			if ( layerCount > 0 )
			{
				mipSize = mipSize * layerCount * faceCount;
			}

			if ( mipSizeStored )
			{
				if ( levelData + 4 > endOfBuffer )
				{
					Error( "%s: Image data exceeds buffer size", fileName );
					GL( glBindTexture( glTarget, 0 ) );
					return false;
				}
				mipSize = (size_t) *(const unsigned int *)levelData;
				levelData += 4;
			}

			if ( depth <= 0 && layerCount <= 0 )
			{
				for ( int face = 0; face < faceCount; face++ )
				{
					if ( mipSize <= 0 || mipSize > (size_t)( endOfBuffer - levelData ) )
					{
						Error( "%s: Mip %mipDepth data exceeds buffer size (%lld > %lld)", fileName, mipLevel, (uint64_t)mipSize, (uint64_t)( endOfBuffer - levelData ) );
						GL( glBindTexture( glTarget, 0 ) );
						return false;
					}

					const GLenum uploadTarget = ( glTarget == GL_TEXTURE_CUBE_MAP ) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_2D;
					if ( compressed )
					{
						GL( glCompressedTexSubImage2D( uploadTarget + face, mipLevel, 0, 0, mipWidth, mipHeight, glInternalFormat, (GLsizei)mipSize, levelData ) );
					}
					else
					{
						GL( glTexSubImage2D( uploadTarget + face, mipLevel, 0, 0, mipWidth, mipHeight, glFormat, glDataType, levelData ) );
					}

					levelData += mipSize;

					if ( mipSizeStored )
					{
						levelData += 3 - ( ( mipSize + 3 ) % 4 );
						if ( levelData > endOfBuffer )
						{
							Error( "%s: Image data exceeds buffer size", fileName );
							GL( glBindTexture( glTarget, 0 ) );
							return false;
						}
					}
				}
			}
			else
			{
				if ( mipSize <= 0 || mipSize > (size_t)( endOfBuffer - levelData ) )
				{
					Error( "%s: Mip %mipDepth data exceeds buffer size (%lld > %lld)", fileName, mipLevel, (uint64_t)mipSize, (uint64_t)( endOfBuffer - levelData ) );
					GL( glBindTexture( glTarget, 0 ) );
					return false;
				}

				if ( compressed )
				{
					GL( glCompressedTexSubImage3D( glTarget, mipLevel, 0, 0, 0, mipWidth, mipHeight, mipDepth * MAX( layerCount, 1 ), glInternalFormat, (GLsizei)mipSize, levelData ) );
				}
				else
				{
					GL( glTexSubImage3D( glTarget, mipLevel, 0, 0, 0, mipWidth, mipHeight, mipDepth * MAX( layerCount, 1 ), glFormat, glDataType, levelData ) );
				}

				levelData += mipSize;

				if ( mipSizeStored )
				{
					levelData += 3 - ( ( mipSize + 3 ) % 4 );
					if ( levelData > endOfBuffer )
					{
						Error( "%s: Image data exceeds buffer size", fileName );
						GL( glBindTexture( glTarget, 0 ) );
						return false;
					}
				}
			}
		}

		if ( mipCount < 1 )
		{
			// Can ony generate mip levels for uncompressed textures.
			assert( compressed == false );

			GL( glGenerateMipmap( glTarget ) );
		}
	}

	GL( glTexParameteri( glTarget, GL_TEXTURE_MIN_FILTER, ( numStorageLevels > 1 ) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR ) );
	GL( glTexParameteri( glTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );

	GL( glBindTexture( glTarget, 0 ) );

	texture->usage = KS_GPU_TEXTURE_USAGE_SAMPLED;

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
	return ksGpuTexture_CreateInternal( context, texture, "data", (GLenum)format, sampleCount, width, height, depth,
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
	return ksGpuTexture_CreateInternal( context, texture, "data", (GLenum)format, sampleCount, width, height, depth,
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
	bool success = ksGpuTexture_CreateInternal( context, texture, "data", GL_RGBA8, KS_GPU_SAMPLE_COUNT_1,
												width, height, depth,
												layerCount, faceCount, mipCount,
												KS_GPU_TEXTURE_USAGE_SAMPLED, data, dataSize, false );

	free( data );

	return success;
}

static bool ksGpuTexture_CreateFromSwapchain( ksGpuContext * context, ksGpuTexture * texture, const ksGpuWindow * window, int index )
{
	UNUSED_PARM( context );
	UNUSED_PARM( index );

	memset( texture, 0, sizeof( ksGpuTexture ) );

	texture->width = window->windowWidth;
	texture->height = window->windowHeight;
	texture->depth = 1;
	texture->layerCount = 1;
	texture->mipCount = 1;
	texture->sampleCount = KS_GPU_SAMPLE_COUNT_1;
	texture->usage = KS_GPU_TEXTURE_USAGE_UNDEFINED;
	texture->wrapMode = KS_GPU_TEXTURE_WRAP_MODE_REPEAT;
	texture->filter = KS_GPU_TEXTURE_FILTER_LINEAR;
	texture->maxAnisotropy = 1.0f;
	texture->format = ksGpuContext_InternalSurfaceColorFormat( window->colorFormat );
	texture->target = 0;
	texture->texture = 0;

	return true;
}

// This KTX loader does not do any conversions. In other words, the format
// stored in the KTX file must be the same as the glInternaltFormat.
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
	const GLenum format = header->glInternalFormat;

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
	UNUSED_PARM( context );

	if ( texture->texture )
	{
		GL( glDeleteTextures( 1, &texture->texture ) );
	}
	memset( texture, 0, sizeof( ksGpuTexture ) );
}

static void ksGpuTexture_SetWrapMode( ksGpuContext * context, ksGpuTexture * texture, const ksGpuTextureWrapMode wrapMode )
{
	UNUSED_PARM( context );

	texture->wrapMode = wrapMode;

	const GLint wrap =  ( ( wrapMode == KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_EDGE ) ? GL_CLAMP_TO_EDGE :
						( ( wrapMode == KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER ) ? glExtensions.texture_clamp_to_border_id :
						( GL_REPEAT ) ) );

	GL( glBindTexture( texture->target, texture->texture ) );
	GL( glTexParameteri( texture->target, GL_TEXTURE_WRAP_S, wrap ) );
	GL( glTexParameteri( texture->target, GL_TEXTURE_WRAP_T, wrap ) );
	GL( glBindTexture( texture->target, 0 ) );
}

static void ksGpuTexture_SetFilter( ksGpuContext * context, ksGpuTexture * texture, const ksGpuTextureFilter filter )
{
	UNUSED_PARM( context );

	texture->filter = filter;

	GL( glBindTexture( texture->target, texture->texture ) );
	if ( filter == KS_GPU_TEXTURE_FILTER_NEAREST )
	{
   		GL( glTexParameteri( texture->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST ) );
		GL( glTexParameteri( texture->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST ) );
	}
	else if ( filter == KS_GPU_TEXTURE_FILTER_LINEAR )
	{
   		GL( glTexParameteri( texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
		GL( glTexParameteri( texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
	}
	else if ( filter == KS_GPU_TEXTURE_FILTER_BILINEAR )
	{
   		GL( glTexParameteri( texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR ) );
		GL( glTexParameteri( texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
	}
	GL( glBindTexture( texture->target, 0 ) );
}

static void ksGpuTexture_SetAniso( ksGpuContext * context, ksGpuTexture * texture, const float maxAniso )
{
	UNUSED_PARM( context );

	texture->maxAnisotropy = maxAniso;

	GL( glBindTexture( texture->target, texture->texture ) );
   	GL( glTexParameterf( texture->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso ) );
	GL( glBindTexture( texture->target, 0 ) );
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
	KS_GPU_ATTRIBUTE_FORMAT_R32_SFLOAT				= ( 1 << 16 ) | GL_FLOAT,
	KS_GPU_ATTRIBUTE_FORMAT_R32G32_SFLOAT			= ( 2 << 16 ) | GL_FLOAT,
	KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT		= ( 3 << 16 ) | GL_FLOAT,
	KS_GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT		= ( 4 << 16 ) | GL_FLOAT
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
} ksGpuRenderPass;

static bool ksGpuRenderPass_Create( ksGpuContext * context, ksGpuRenderPass * renderPass,
									const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
									const ksGpuSampleCount sampleCount, const ksGpuRenderPassType type, const int flags )
{
	UNUSED_PARM( context );

	assert( type == KS_GPU_RENDERPASS_TYPE_INLINE );

	renderPass->type = type;
	renderPass->flags = flags;
	renderPass->colorFormat = colorFormat;
	renderPass->depthFormat = depthFormat;
	renderPass->sampleCount = sampleCount;
	return true;
}

static void ksGpuRenderPass_Destroy( ksGpuContext * context, ksGpuRenderPass * renderPass )
{
	UNUSED_PARM( context );
	UNUSED_PARM( renderPass );
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
	GLuint				renderTexture;
	GLuint				depthBuffer;
	GLuint *			renderBuffers;
	GLuint *			resolveBuffers;
	bool				multiView;
	int					sampleCount;
	int					numFramebuffersPerTexture;
	int					numBuffers;
	int					currentBuffer;
} ksGpuFramebuffer;

typedef enum
{
	MSAA_OFF,
	MSAA_RESOLVE,
	MSAA_BLIT
} ksGpuMsaaMode;

static bool ksGpuFramebuffer_CreateFromSwapchain( ksGpuWindow * window, ksGpuFramebuffer * framebuffer, ksGpuRenderPass * renderPass )
{
	assert( window->sampleCount == renderPass->sampleCount );

	UNUSED_PARM( renderPass );

	memset( framebuffer, 0, sizeof( ksGpuFramebuffer ) );

	static const int NUM_BUFFERS = 1;

	framebuffer->colorTextures = (ksGpuTexture *) malloc( NUM_BUFFERS * sizeof( ksGpuTexture ) );
	framebuffer->renderTexture = 0;
	framebuffer->depthBuffer = 0;
	framebuffer->renderBuffers = (GLuint *) malloc( NUM_BUFFERS * sizeof( GLuint ) );
	framebuffer->resolveBuffers = framebuffer->renderBuffers;
	framebuffer->multiView = false;
	framebuffer->sampleCount = KS_GPU_SAMPLE_COUNT_1;
	framebuffer->numFramebuffersPerTexture = 1;
	framebuffer->numBuffers = NUM_BUFFERS;
	framebuffer->currentBuffer = 0;

	// Create the color textures.
	for ( int bufferIndex = 0; bufferIndex < NUM_BUFFERS; bufferIndex++ )
	{
		assert( renderPass->colorFormat == window->colorFormat );
		assert( renderPass->depthFormat == window->depthFormat );

		ksGpuTexture_CreateFromSwapchain( &window->context, &framebuffer->colorTextures[bufferIndex], window, bufferIndex );

		assert( window->windowWidth == framebuffer->colorTextures[bufferIndex].width );
		assert( window->windowHeight == framebuffer->colorTextures[bufferIndex].height );

		framebuffer->renderBuffers[bufferIndex] = 0;
	}

	return true;
}

static bool ksGpuFramebuffer_CreateFromTextures( ksGpuContext * context, ksGpuFramebuffer * framebuffer, ksGpuRenderPass * renderPass,
												const int width, const int height, const int numBuffers )
{
	UNUSED_PARM( context );

	memset( framebuffer, 0, sizeof( ksGpuFramebuffer ) );

	framebuffer->colorTextures = (ksGpuTexture *) malloc( numBuffers * sizeof( ksGpuTexture ) );
	framebuffer->renderTexture = 0;
	framebuffer->depthBuffer = 0;
	framebuffer->renderBuffers = (GLuint *) malloc( numBuffers * sizeof( GLuint ) );
	framebuffer->resolveBuffers = framebuffer->renderBuffers;
	framebuffer->multiView = false;
	framebuffer->sampleCount = KS_GPU_SAMPLE_COUNT_1;
	framebuffer->numFramebuffersPerTexture = 1;
	framebuffer->numBuffers = numBuffers;
	framebuffer->currentBuffer = 0;

	const ksGpuMsaaMode mode =	( ( renderPass->sampleCount > KS_GPU_SAMPLE_COUNT_1 && glExtensions.multi_sampled_resolve ) ? MSAA_RESOLVE :
								( ( renderPass->sampleCount > KS_GPU_SAMPLE_COUNT_1 ) ? MSAA_BLIT :
									MSAA_OFF ) );

	// Create the color textures.
	const GLenum colorFormat = ksGpuContext_InternalSurfaceColorFormat( renderPass->colorFormat );
	for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
	{
		ksGpuTexture_Create2D( context, &framebuffer->colorTextures[bufferIndex], (ksGpuTextureFormat)colorFormat, KS_GPU_SAMPLE_COUNT_1,
			width, height, 1, KS_GPU_TEXTURE_USAGE_SAMPLED | KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT | KS_GPU_TEXTURE_USAGE_STORAGE, NULL, 0 );
		ksGpuTexture_SetWrapMode( context, &framebuffer->colorTextures[bufferIndex], KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER );
	}

	// Create the depth buffer.
	if ( renderPass->depthFormat != KS_GPU_SURFACE_DEPTH_FORMAT_NONE )
	{
		const GLenum depthFormat = ksGpuContext_InternalSurfaceDepthFormat( renderPass->depthFormat );

		GL( glGenRenderbuffers( 1, &framebuffer->depthBuffer ) );
		GL( glBindRenderbuffer( GL_RENDERBUFFER, framebuffer->depthBuffer ) );
		if ( mode == MSAA_RESOLVE )
		{
			GL( glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER, renderPass->sampleCount, depthFormat, width, height ) );
		}
		else if ( mode == MSAA_BLIT )
		{
			GL( glRenderbufferStorageMultisample( GL_RENDERBUFFER, renderPass->sampleCount, depthFormat, width, height ) );
		}
		else
		{
			GL( glRenderbufferStorage( GL_RENDERBUFFER, depthFormat, width, height ) );
		}
		GL( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );
	}

	// Create the render buffers.
	const int numRenderBuffers = ( mode == MSAA_BLIT ) ? 1 : numBuffers;
	for ( int bufferIndex = 0; bufferIndex < numRenderBuffers; bufferIndex++ )
	{
		GL( glGenFramebuffers( 1, &framebuffer->renderBuffers[bufferIndex] ) );
		GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebuffer->renderBuffers[bufferIndex] ) );
		if ( mode == MSAA_RESOLVE )
		{
			GL( glFramebufferTexture2DMultisampleEXT( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->colorTextures[bufferIndex].texture, 0, renderPass->sampleCount ) );
		}
		else if ( mode == MSAA_BLIT )
		{
			GL( glRenderbufferStorageMultisample( GL_RENDERBUFFER, renderPass->sampleCount, colorFormat, width, height ) );
		}
		else
		{
			GL( glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->colorTextures[bufferIndex].texture, 0 ) );
		}
		if ( renderPass->depthFormat != KS_GPU_SURFACE_DEPTH_FORMAT_NONE )
		{
			GL( glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebuffer->depthBuffer ) );
		}
		GL( glGetIntegerv( GL_SAMPLES, &framebuffer->sampleCount ) );
		GL( GLenum status = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
		GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
		GL( glClear( GL_COLOR_BUFFER_BIT ) );
		GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
		if ( status != GL_FRAMEBUFFER_COMPLETE )
		{
			Error( "Incomplete frame buffer object: %s", GlFramebufferStatusString( status ) );
			return false;
		}
	}

	// Create the resolve buffers.
	if ( mode == MSAA_BLIT )
	{
		framebuffer->resolveBuffers = (GLuint *) malloc( numBuffers * sizeof( GLuint ) );
		for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
		{
			framebuffer->renderBuffers[bufferIndex] = framebuffer->renderBuffers[0];

			GL( glGenFramebuffers( 1, &framebuffer->resolveBuffers[bufferIndex] ) );
			GL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->colorTextures[bufferIndex].texture, 0 ) );
			GL( GLenum status = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
			GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
			if ( status != GL_FRAMEBUFFER_COMPLETE )
			{
				Error( "Incomplete frame buffer object: %s", GlFramebufferStatusString( status ) );
				return false;
			}
		}
	}

	return true;
}

static bool ksGpuFramebuffer_CreateFromTextureArrays( ksGpuContext * context, ksGpuFramebuffer * framebuffer, ksGpuRenderPass * renderPass,
												const int width, const int height, const int numLayers, const int numBuffers, const bool multiview )
{
	UNUSED_PARM( context );

	memset( framebuffer, 0, sizeof( ksGpuFramebuffer ) );

	framebuffer->colorTextures = (ksGpuTexture *) malloc( numBuffers * sizeof( ksGpuTexture ) );
	framebuffer->depthBuffer = 0;
	framebuffer->multiView = multiview;
	framebuffer->sampleCount = KS_GPU_SAMPLE_COUNT_1;
	framebuffer->numFramebuffersPerTexture = ( multiview ? 1 : numLayers );
	framebuffer->renderBuffers = (GLuint *) malloc( numBuffers * framebuffer->numFramebuffersPerTexture * sizeof( GLuint ) );
	framebuffer->resolveBuffers = framebuffer->renderBuffers;
	framebuffer->numBuffers = numBuffers;
	framebuffer->currentBuffer = 0;

	const ksGpuMsaaMode mode =	( ( renderPass->sampleCount > KS_GPU_SAMPLE_COUNT_1 && !multiview && glExtensions.multi_sampled_resolve ) ? MSAA_RESOLVE :
								( ( renderPass->sampleCount > KS_GPU_SAMPLE_COUNT_1 && multiview && glExtensions.multi_view_multi_sampled_resolve ) ? MSAA_RESOLVE :
								( ( renderPass->sampleCount > KS_GPU_SAMPLE_COUNT_1 && glExtensions.multi_sampled_storage ) ? MSAA_BLIT :
									MSAA_OFF ) ) );

	// Create the color textures.
	const GLenum colorFormat = ksGpuContext_InternalSurfaceColorFormat( renderPass->colorFormat );
	for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
	{
		ksGpuTexture_Create2DArray( context, &framebuffer->colorTextures[bufferIndex], (ksGpuTextureFormat)colorFormat, KS_GPU_SAMPLE_COUNT_1,
			width, height, numLayers, 1, KS_GPU_TEXTURE_USAGE_SAMPLED | KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT | KS_GPU_TEXTURE_USAGE_STORAGE, NULL, 0 );
		ksGpuTexture_SetWrapMode( context, &framebuffer->colorTextures[bufferIndex], KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER );
	}

	// Create the render texture.
	if ( mode == MSAA_BLIT )
	{
		GL( glGenTextures( 1, &framebuffer->renderTexture ) );
		GL( glBindTexture( GL_TEXTURE_2D_MULTISAMPLE_ARRAY, framebuffer->renderTexture ) );
		GL( glTexStorage3DMultisample( GL_TEXTURE_2D_MULTISAMPLE_ARRAY, renderPass->sampleCount, colorFormat, width, height, numLayers, GL_TRUE ) );
		GL( glBindTexture( GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0 ) );
	}

	// Create the depth buffer.
	if ( renderPass->depthFormat != KS_GPU_SURFACE_DEPTH_FORMAT_NONE )
	{
		const GLenum depthFormat = ksGpuContext_InternalSurfaceDepthFormat( renderPass->depthFormat );
		const GLenum target = ( mode == MSAA_BLIT ) ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY;

		GL( glGenTextures( 1, &framebuffer->depthBuffer ) );
		GL( glBindTexture( target, framebuffer->depthBuffer ) );
		if ( mode == MSAA_BLIT )
		{
			GL( glTexStorage3DMultisample( target, renderPass->sampleCount, depthFormat, width, height, numLayers, GL_TRUE ) );
		}
		else
		{
			GL( glTexStorage3D( target, 1, depthFormat, width, height, numLayers ) );
		}
		GL( glBindTexture( target, 0 ) );
	}

	// Create the render buffers.
	const int numRenderBuffers = ( mode == MSAA_BLIT ) ? 1 : numBuffers;
	for ( int bufferIndex = 0; bufferIndex < numRenderBuffers; bufferIndex++ )
	{
		for ( int layerIndex = 0; layerIndex < framebuffer->numFramebuffersPerTexture; layerIndex++ )
		{
			GL( glGenFramebuffers( 1, &framebuffer->renderBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] ) );
			GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebuffer->renderBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] ) );
			if ( multiview )
			{
				if ( mode == MSAA_RESOLVE )
				{
					GL( glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							framebuffer->colorTextures[bufferIndex].texture, 0 /* level */, renderPass->sampleCount /* samples */, 0 /* baseViewIndex */, numLayers /* numViews */ ) );
					if ( renderPass->depthFormat != KS_GPU_SURFACE_DEPTH_FORMAT_NONE )
					{
						GL( glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								framebuffer->depthBuffer, 0 /* level */, renderPass->sampleCount /* samples */, 0 /* baseViewIndex */, numLayers /* numViews */ ) );
					}
				}
				else if ( mode == MSAA_BLIT )
				{
					GL( glFramebufferTextureMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							framebuffer->renderTexture, 0 /* level */, 0 /* baseViewIndex */, numLayers /* numViews */ ) );
					if ( renderPass->depthFormat != KS_GPU_SURFACE_DEPTH_FORMAT_NONE )
					{
						GL( glFramebufferTextureMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								framebuffer->depthBuffer, 0 /* level */, 0 /* baseViewIndex */, numLayers /* numViews */ ) );
					}
				}
				else
				{
					GL( glFramebufferTextureMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							framebuffer->colorTextures[bufferIndex].texture, 0 /* level */, 0 /* baseViewIndex */, numLayers /* numViews */ ) );
					if ( renderPass->depthFormat != KS_GPU_SURFACE_DEPTH_FORMAT_NONE )
					{
						GL( glFramebufferTextureMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								framebuffer->depthBuffer, 0 /* level */, 0 /* baseViewIndex */, numLayers /* numViews */ ) );
					}
				}
			}
			else
			{
				if ( mode == MSAA_RESOLVE )
				{
					// Note: using glFramebufferTextureMultisampleMultiviewOVR with a single view because there is no glFramebufferTextureLayerMultisampleEXT
					GL( glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							framebuffer->colorTextures[bufferIndex].texture, 0 /* level */, renderPass->sampleCount /* samples */, layerIndex /* baseViewIndex */, 1 /* numViews */ ) );
					if ( renderPass->depthFormat != KS_GPU_SURFACE_DEPTH_FORMAT_NONE )
					{
						GL( glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								framebuffer->depthBuffer, 0 /* level */, renderPass->sampleCount /* samples */, layerIndex /* baseViewIndex */, 1 /* numViews */ ) );
					}
				}
				else if ( mode == MSAA_BLIT )
				{
					GL( glFramebufferTextureLayer( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							framebuffer->renderTexture, 0 /* level */, layerIndex /* layerIndex */ ) );
					if ( renderPass->depthFormat != KS_GPU_SURFACE_DEPTH_FORMAT_NONE )
					{
						GL( glFramebufferTextureLayer( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								framebuffer->depthBuffer, 0 /* level */, layerIndex /* layerIndex */ ) );
					}
				}
				else
				{
					GL( glFramebufferTextureLayer( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							framebuffer->colorTextures[bufferIndex].texture, 0 /* level */, layerIndex /* layerIndex */ ) );
					if ( renderPass->depthFormat != KS_GPU_SURFACE_DEPTH_FORMAT_NONE )
					{
						GL( glFramebufferTextureLayer( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								framebuffer->depthBuffer, 0 /* level */, layerIndex /* layerIndex */ ) );
					}
				}
			}
			GL( glGetIntegerv( GL_SAMPLES, &framebuffer->sampleCount ) );
			GL( GLenum status = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
			GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
			if ( status != GL_FRAMEBUFFER_COMPLETE )
			{
				Error( "Incomplete frame buffer object: %s", GlFramebufferStatusString( status ) );
				return false;
			}
		}
	}

	// Create the resolve buffers.
	if ( mode == MSAA_BLIT )
	{
		framebuffer->resolveBuffers = (GLuint *) malloc( numBuffers * framebuffer->numFramebuffersPerTexture * sizeof( GLuint ) );
		for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
		{
			for ( int layerIndex = 0; layerIndex < framebuffer->numFramebuffersPerTexture; layerIndex++ )
			{
				framebuffer->renderBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] = framebuffer->renderBuffers[layerIndex];

				GL( glGenFramebuffers( 1, &framebuffer->resolveBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] ) );
				GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebuffer->resolveBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] ) );
				GL( glFramebufferTextureLayer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, framebuffer->colorTextures[bufferIndex].texture, 0 /* level */, layerIndex /* layerIndex */ ) );
				GL( GLenum status = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
				GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
				if ( status != GL_FRAMEBUFFER_COMPLETE )
				{
					Error( "Incomplete frame buffer object: %s", GlFramebufferStatusString( status ) );
					return false;
				}
			}
		}
	}
	return true;
}

static void ksGpuFramebuffer_Destroy( ksGpuContext * context, ksGpuFramebuffer * framebuffer )
{
	for ( int bufferIndex = 0; bufferIndex < framebuffer->numBuffers; bufferIndex++ )
	{
		if ( framebuffer->resolveBuffers != framebuffer->renderBuffers )
		{
			for ( int layerIndex = 0; layerIndex < framebuffer->numFramebuffersPerTexture; layerIndex++ )
			{
				if ( framebuffer->resolveBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] != 0 )
				{
					GL( glDeleteFramebuffers( 1, &framebuffer->resolveBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] ) );
				}
			}
		}
		if ( bufferIndex == 0 || framebuffer->renderBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + 0] != framebuffer->renderBuffers[0] )
		{
			for ( int layerIndex = 0; layerIndex < framebuffer->numFramebuffersPerTexture; layerIndex++ )
			{
				if ( framebuffer->renderBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] != 0 )
				{
					GL( glDeleteFramebuffers( 1, &framebuffer->renderBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] ) );
				}
			}
		}
	}
	if ( framebuffer->depthBuffer != 0 )
	{
		if ( framebuffer->colorTextures[0].layerCount > 0 )
		{
			GL( glDeleteTextures( 1, &framebuffer->depthBuffer ) );
		}
		else
		{
			GL( glDeleteRenderbuffers( 1, &framebuffer->depthBuffer ) );
		}
	}
	if ( framebuffer->renderTexture != 0 )
	{
		if ( framebuffer->colorTextures[0].layerCount > 0 )
		{
			GL( glDeleteTextures( 1, &framebuffer->renderTexture ) );
		}
		else
		{
			GL( glDeleteRenderbuffers( 1, &framebuffer->renderTexture ) );
		}
	}
	for ( int bufferIndex = 0; bufferIndex < framebuffer->numBuffers; bufferIndex++ )
	{
		if ( framebuffer->colorTextures[bufferIndex].texture != 0 )
		{
			ksGpuTexture_Destroy( context, &framebuffer->colorTextures[bufferIndex] );
		}
	}
	if ( framebuffer->resolveBuffers != framebuffer->renderBuffers )
	{
		free( framebuffer->resolveBuffers );
	}
	free( framebuffer->renderBuffers );
	free( framebuffer->colorTextures );

	memset( framebuffer, 0, sizeof( ksGpuFramebuffer ) );
}

static int ksGpuFramebuffer_GetWidth( const ksGpuFramebuffer * framebuffer )
{
	return framebuffer->colorTextures[framebuffer->currentBuffer].width;
}

static int ksGpuFramebuffer_GetHeight( const ksGpuFramebuffer * framebuffer )
{
	return framebuffer->colorTextures[framebuffer->currentBuffer].height;
}

static ksScreenRect ksGpuFramebuffer_GetRect( const ksGpuFramebuffer * framebuffer )
{
	ksScreenRect rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = framebuffer->colorTextures[framebuffer->currentBuffer].width;
	rect.height = framebuffer->colorTextures[framebuffer->currentBuffer].height;
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
										const ksGpuProgramParm * parms, const int numParms,
										const GLuint program );
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
	int							binding;	// OpenGL shader bind points:
											// - texture image unit
											// - image unit
											// - uniform buffer
											// - storage buffer
											// - uniform
											// Note that each bind point uses its own range of binding indices with each range starting at zero.
											// However, each range is unique across all stages of a pipeline.
											// Note that even though multiple targets can be bound to the same texture image unit,
											// the OpenGL spec disallows rendering from multiple targets using a single texture image unit.

} ksGpuProgramParm;

typedef struct
{
	int							numParms;
	const ksGpuProgramParm *	parms;
	int							offsetForIndex[MAX_PROGRAM_PARMS];	// push constant offsets into ksGpuProgramParmState::data based on ksGpuProgramParm::index
	GLint						parmLocations[MAX_PROGRAM_PARMS];	// OpenGL locations
	GLint						parmBindings[MAX_PROGRAM_PARMS];
	GLint						numSampledTextureBindings;
	GLint						numStorageTextureBindings;
	GLint						numUniformBufferBindings;
	GLint						numStorageBufferBindings;
} ksGpuProgramParmLayout;

static bool ksGpuProgramParm_IsOpaqueBinding( const ksGpuProgramParmType type )
{
	return	( ( type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED ) ?	true :
			( ( type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE ) ?	true :
			( ( type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM ) ?		true :
			( ( type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE ) ?		true :
																		false ) ) ) );
}

static int ksGpuProgramParm_GetPushConstantSize( const ksGpuProgramParmType type )
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

static void ksGpuProgramParmLayout_Create( ksGpuContext * context, ksGpuProgramParmLayout * layout,
										const ksGpuProgramParm * parms, const int numParms,
										const GLuint program )
{
	UNUSED_PARM( context );
	assert( numParms <= MAX_PROGRAM_PARMS );

	memset( layout, 0, sizeof( ksGpuProgramParmLayout ) );

	layout->numParms = numParms;
	layout->parms = parms;

	int offset = 0;
	memset( layout->offsetForIndex, -1, sizeof( layout->offsetForIndex ) );

	// Get the texture/buffer/uniform locations and set bindings.
	for ( int i = 0; i < numParms; i++ )
	{
		if ( parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED )
		{
			layout->parmLocations[i] = glGetUniformLocation( program, parms[i].name );
			assert( layout->parmLocations[i] != -1 );
			if ( layout->parmLocations[i] != -1 )
			{
				// set "texture image unit" binding
				layout->parmBindings[i] = layout->numSampledTextureBindings++;
				GL( glProgramUniform1i( program, layout->parmLocations[i], layout->parmBindings[i] ) );
			}
		}
		else if ( parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE )
		{
			layout->parmLocations[i] = glGetUniformLocation( program, parms[i].name );
			assert( layout->parmLocations[i] != -1 );
			if ( layout->parmLocations[i] != -1 )
			{
				// set "image unit" binding
				layout->parmBindings[i] = layout->numStorageTextureBindings++;
#if !defined( OS_ANDROID )
				// OpenGL ES does not support changing the location after linking, so rely on the layout( binding ) being correct.
				GL( glProgramUniform1i( program, layout->parmLocations[i], layout->parmBindings[i] ) );
#endif
			}
		}
		else if ( parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM )
		{
			layout->parmLocations[i] = glGetUniformBlockIndex( program, parms[i].name );
			assert( layout->parmLocations[i] != -1 );
			if ( layout->parmLocations[i] != -1 )
			{
				// set "uniform block" binding
				layout->parmBindings[i] = layout->numUniformBufferBindings++;
				GL( glUniformBlockBinding( program, layout->parmLocations[i], layout->parmBindings[i] ) );
			}
		}
		else if ( parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
		{
			layout->parmLocations[i] = glGetProgramResourceIndex( program, GL_SHADER_STORAGE_BLOCK, parms[i].name );
			assert( layout->parmLocations[i] != -1 );
			if ( layout->parmLocations[i] != -1 )
			{
				// set "shader storage block" binding
				layout->parmBindings[i] = layout->numStorageBufferBindings++;
#if !defined( OS_ANDROID )
				// OpenGL ES does not support glShaderStorageBlockBinding, so rely on the layout( binding ) being correct.
				GL( glShaderStorageBlockBinding( program, layout->parmLocations[i], layout->parmBindings[i] ) );
#endif
			}
		}
		else
		{
			layout->parmLocations[i] = glGetUniformLocation( program, parms[i].name );
			assert( layout->parmLocations[i] != -1 );	// The parm is not actually used in the shader?
			layout->parmBindings[i] = i;

			layout->offsetForIndex[parms[i].index] = offset;
			offset += ksGpuProgramParm_GetPushConstantSize( parms[i].type );
		}
	}

	assert( layout->numSampledTextureBindings <= glGetInteger( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS ) );
#if OPENGL_COMPUTE_ENABLED == 1
	assert( layout->numStorageTextureBindings <= glGetInteger( GL_MAX_IMAGE_UNITS ) );
	assert( layout->numUniformBufferBindings <= glGetInteger( GL_MAX_UNIFORM_BUFFER_BINDINGS ) );
	assert( layout->numStorageBufferBindings <= glGetInteger( GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS ) );
#endif
}

static void ksGpuProgramParmLayout_Destroy( ksGpuContext * context, ksGpuProgramParmLayout * layout )
{
	UNUSED_PARM( context );
	UNUSED_PARM( layout );
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
	GLuint					vertexShader;
	GLuint					fragmentShader;
	GLuint					program;
	ksGpuProgramParmLayout	parmLayout;
	int						vertexAttribsFlags;
	ksStringHash			hash;
} ksGpuGraphicsProgram;

static bool ksGpuGraphicsProgram_Create( ksGpuContext * context, ksGpuGraphicsProgram * program,
										const void * vertexSourceData, const size_t vertexSourceSize,
										const void * fragmentSourceData, const size_t fragmentSourceSize,
										const ksGpuProgramParm * parms, const int numParms,
										const ksGpuVertexAttribute * vertexLayout, const int vertexAttribsFlags )
{
	UNUSED_PARM( vertexSourceSize );
	UNUSED_PARM( fragmentSourceSize );

	program->vertexAttribsFlags = vertexAttribsFlags;

	GLint r;
	GL( program->vertexShader = glCreateShader( GL_VERTEX_SHADER ) );
	GL( glShaderSource( program->vertexShader, 1, (const char **)&vertexSourceData, 0 ) );
	GL( glCompileShader( program->vertexShader ) );
	GL( glGetShaderiv( program->vertexShader, GL_COMPILE_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GLsizei length;
		GL( glGetShaderInfoLog( program->vertexShader, sizeof( msg ), &length, msg ) );
		Error( "%s\nlength=%d\n%s\n", (const char *)vertexSourceData, length, msg );
		return false;
	}

	GL( program->fragmentShader = glCreateShader( GL_FRAGMENT_SHADER ) );
	GL( glShaderSource( program->fragmentShader, 1, (const char **)&fragmentSourceData, 0 ) );
	GL( glCompileShader( program->fragmentShader ) );
	GL( glGetShaderiv( program->fragmentShader, GL_COMPILE_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GLsizei length;
		GL( glGetShaderInfoLog( program->fragmentShader, sizeof( msg ), &length, msg ) );
		Error( "%s\nlength=%d\n%s\n", (const char *)fragmentSourceData, length, msg );
		return false;
	}

	GL( program->program = glCreateProgram() );
	GL( glAttachShader( program->program, program->vertexShader ) );
	GL( glAttachShader( program->program, program->fragmentShader ) );

	// Bind the vertex attribute locations before linking.
	GLuint location = 0;
	for ( int i = 0; vertexLayout[i].attributeFlag != 0; i++ )
	{
		if ( ( vertexLayout[i].attributeFlag & vertexAttribsFlags ) != 0 )
		{
			GL( glBindAttribLocation( program->program, location, vertexLayout[i].name ) );
			location += vertexLayout[i].locationCount;
		}
	}

	GL( glLinkProgram( program->program ) );
	GL( glGetProgramiv( program->program, GL_LINK_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GL( glGetProgramInfoLog( program->program, sizeof( msg ), 0, msg ) );
		Error( "Linking program failed: %s\n", msg );
		return false;
	}

	// Verify the attributes.
	for ( int i = 0; vertexLayout[i].attributeFlag != 0; i++ )
	{
		if ( ( vertexLayout[i].attributeFlag & vertexAttribsFlags ) != 0 )
		{
			assert( glGetAttribLocation( program->program, vertexLayout[i].name ) != -1 );
		}
	}

	ksGpuProgramParmLayout_Create( context, &program->parmLayout, parms, numParms, program->program );

	// Calculate a hash of the vertex and fragment program source.
	ksStringHash_Init( &program->hash );
	ksStringHash_Update( &program->hash, (const char *)vertexSourceData );
	ksStringHash_Update( &program->hash, (const char *)fragmentSourceData );

	return true;
}

static void ksGpuGraphicsProgram_Destroy( ksGpuContext * context, ksGpuGraphicsProgram * program )
{
	ksGpuProgramParmLayout_Destroy( context, &program->parmLayout );
	if ( program->program != 0 )
	{
		GL( glDeleteProgram( program->program ) );
		program->program = 0;
	}
	if ( program->vertexShader != 0 )
	{
		GL( glDeleteShader( program->vertexShader ) );
		program->vertexShader = 0;
	}
	if ( program->fragmentShader != 0 )
	{
		GL( glDeleteShader( program->fragmentShader ) );
		program->fragmentShader = 0;
	}
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
	GLuint					computeShader;
	GLuint					program;
	ksGpuProgramParmLayout	parmLayout;
	ksStringHash			hash;
} ksGpuComputeProgram;

static bool ksGpuComputeProgram_Create( ksGpuContext * context, ksGpuComputeProgram * program,
									const void * computeSourceData, const size_t computeSourceSize,
									const ksGpuProgramParm * parms, const int numParms )
{
	UNUSED_PARM( context );
	UNUSED_PARM( computeSourceSize );

	GLint r;
	GL( program->computeShader = glCreateShader( GL_COMPUTE_SHADER ) );
	GL( glShaderSource( program->computeShader, 1, (const char **)&computeSourceData, 0 ) );
	GL( glCompileShader( program->computeShader ) );
	GL( glGetShaderiv( program->computeShader, GL_COMPILE_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GL( glGetShaderInfoLog( program->computeShader, sizeof( msg ), 0, msg ) );
		Error( "%s\n%s\n", (const char *)computeSourceData, msg );
		return false;
	}

	GL( program->program = glCreateProgram() );
	GL( glAttachShader( program->program, program->computeShader ) );

	GL( glLinkProgram( program->program ) );
	GL( glGetProgramiv( program->program, GL_LINK_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GL( glGetProgramInfoLog( program->program, sizeof( msg ), 0, msg ) );
		Error( "Linking program failed: %s\n", msg );
		return false;
	}

	ksGpuProgramParmLayout_Create( context, &program->parmLayout, parms, numParms, program->program );

	// Calculate a hash of the shader source.
	ksStringHash_Init( &program->hash );
	ksStringHash_Update( &program->hash, (const char *)computeSourceData );

	return true;
}

static void ksGpuComputeProgram_Destroy( ksGpuContext * context, ksGpuComputeProgram * program )
{
	ksGpuProgramParmLayout_Destroy( context, &program->parmLayout );

	if ( program->program != 0 )
	{
		GL( glDeleteProgram( program->program ) );
		program->program = 0;
	}
	if ( program->computeShader != 0 )
	{
		GL( glDeleteShader( program->computeShader ) );
		program->computeShader = 0;
	}
}

/*
================================================================================================================================

GPU graphics pipeline.

A graphics pipeline encapsulates the geometry, program and ROP state that is used to render.
For optimal performance a graphics pipeline should only be created at load time, not at runtime.
Due to the use of a Vertex Array Object (VAO), a graphics pipeline must be created using the same
context that is used to render with the graphics pipeline. The VAO is created here, when both the
geometry and the program are known, to avoid binding vertex attributes that are not used by the
vertex shader, and to avoid binding to a discontinuous set of vertex attribute locations.

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
	KS_GPU_FRONT_FACE_COUNTER_CLOCKWISE				= GL_CCW,
	KS_GPU_FRONT_FACE_CLOCKWISE						= GL_CW
} ksGpuFrontFace;

typedef enum
{
	KS_GPU_CULL_MODE_NONE							= GL_NONE,
	KS_GPU_CULL_MODE_FRONT							= GL_FRONT,
	KS_GPU_CULL_MODE_BACK							= GL_BACK
} ksGpuCullMode;

typedef enum
{
	KS_GPU_COMPARE_OP_NEVER							= GL_NEVER,
	KS_GPU_COMPARE_OP_LESS							= GL_LESS,
	KS_GPU_COMPARE_OP_EQUAL							= GL_EQUAL,
	KS_GPU_COMPARE_OP_LESS_OR_EQUAL					= GL_LEQUAL,
	KS_GPU_COMPARE_OP_GREATER						= GL_GREATER,
	KS_GPU_COMPARE_OP_NOT_EQUAL						= GL_NOTEQUAL,
	KS_GPU_COMPARE_OP_GREATER_OR_EQUAL				= GL_GEQUAL,
	KS_GPU_COMPARE_OP_ALWAYS						= GL_ALWAYS
} ksGpuCompareOp;

typedef enum
{
	KS_GPU_BLEND_OP_ADD								= GL_FUNC_ADD,
	KS_GPU_BLEND_OP_SUBTRACT						= GL_FUNC_SUBTRACT,
	KS_GPU_BLEND_OP_REVERSE_SUBTRACT				= GL_FUNC_REVERSE_SUBTRACT,
	KS_GPU_BLEND_OP_MIN								= GL_MIN,
	KS_GPU_BLEND_OP_MAX								= GL_MAX
} ksGpuBlendOp;

typedef enum
{
	KS_GPU_BLEND_FACTOR_ZERO						= GL_ZERO,
	KS_GPU_BLEND_FACTOR_ONE							= GL_ONE,
	KS_GPU_BLEND_FACTOR_SRC_COLOR					= GL_SRC_COLOR,
	KS_GPU_BLEND_FACTOR_ONE_MINUS_SRC_COLOR			= GL_ONE_MINUS_SRC_COLOR,
	KS_GPU_BLEND_FACTOR_DST_COLOR					= GL_DST_COLOR,
	KS_GPU_BLEND_FACTOR_ONE_MINUS_DST_COLOR			= GL_ONE_MINUS_DST_COLOR,
	KS_GPU_BLEND_FACTOR_SRC_ALPHA					= GL_SRC_ALPHA,
	KS_GPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA			= GL_ONE_MINUS_SRC_ALPHA,
	KS_GPU_BLEND_FACTOR_DST_ALPHA					= GL_DST_ALPHA,
	KS_GPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA			= GL_ONE_MINUS_DST_ALPHA,
	KS_GPU_BLEND_FACTOR_CONSTANT_COLOR				= GL_CONSTANT_COLOR,
	KS_GPU_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR	= GL_ONE_MINUS_CONSTANT_COLOR,
	KS_GPU_BLEND_FACTOR_CONSTANT_ALPHA				= GL_CONSTANT_ALPHA,
	KS_GPU_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA	= GL_ONE_MINUS_CONSTANT_ALPHA,
	KS_GPU_BLEND_FACTOR_SRC_ALPHA_SATURATE			= GL_SRC_ALPHA_SATURATE
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

typedef struct
{
	ksGpuRasterOperations			rop;
	const ksGpuGraphicsProgram *	program;
	const ksGpuGeometry *			geometry;
	GLuint	 						vertexArrayObject;
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
								GLuint * attribLocationCount )
{
	size_t offset = 0;
	for ( int i = 0; vertexLayout[i].attributeFlag != 0; i++ )
	{
		const ksGpuVertexAttribute * v = &vertexLayout[i];
		if ( ( v->attributeFlag & storedAttribsFlags ) != 0 )
		{
			if ( ( v->attributeFlag & usedAttribsFlags ) != 0 )
			{
				const size_t attribLocationSize = v->attributeSize / v->locationCount;
				const size_t attribStride = v->attributeSize;
				for ( int location = 0; location < v->locationCount; location++ )
				{
					GL( glEnableVertexAttribArray( *attribLocationCount + location ) );
					GL( glVertexAttribPointer( *attribLocationCount + location, v->attributeFormat >> 16, v->attributeFormat & 0xFFFF, GL_FALSE,
												(GLsizei)attribStride, (void *)( offset + location * attribLocationSize ) ) );
					GL( glVertexAttribDivisor( *attribLocationCount + location, instance ? 1 : 0 ) );
				}
				*attribLocationCount += v->locationCount;
			}
			offset += numAttribs * v->attributeSize;
		}
	}
}

static bool ksGpuGraphicsPipeline_Create( ksGpuContext * context, ksGpuGraphicsPipeline * pipeline, const ksGpuGraphicsPipelineParms * parms )
{
	UNUSED_PARM( context );

	// Make sure the geometry provides all the attributes needed by the program.
	assert( ( ( parms->geometry->vertexAttribsFlags | parms->geometry->instanceAttribsFlags ) & parms->program->vertexAttribsFlags ) == parms->program->vertexAttribsFlags );

	memset( pipeline, 0, sizeof( ksGpuGraphicsPipeline ) );

	pipeline->rop = parms->rop;
	pipeline->program = parms->program;
	pipeline->geometry = parms->geometry;

	assert( pipeline->vertexArrayObject == 0 );

	GL( glGenVertexArrays( 1, &pipeline->vertexArrayObject ) );
	GL( glBindVertexArray( pipeline->vertexArrayObject ) );

	GLuint attribLocationCount = 0;

	GL( glBindBuffer( parms->geometry->vertexBuffer.target, parms->geometry->vertexBuffer.buffer ) );
	InitVertexAttributes( false, parms->geometry->layout,
							parms->geometry->vertexCount, parms->geometry->vertexAttribsFlags,
							parms->program->vertexAttribsFlags, &attribLocationCount );

	if ( parms->geometry->instanceBuffer.buffer != 0 )
	{
		GL( glBindBuffer( parms->geometry->instanceBuffer.target, parms->geometry->instanceBuffer.buffer ) );
		InitVertexAttributes( true, parms->geometry->layout,
								parms->geometry->instanceCount, parms->geometry->instanceAttribsFlags,
								parms->program->vertexAttribsFlags, &attribLocationCount );
	}

	GL( glBindBuffer( parms->geometry->indexBuffer.target, parms->geometry->indexBuffer.buffer ) );

	GL( glBindVertexArray( 0 ) );

	return true;
}

static void ksGpuGraphicsPipeline_Destroy( ksGpuContext * context, ksGpuGraphicsPipeline * pipeline )
{
	UNUSED_PARM( context );

	if ( pipeline->vertexArrayObject != 0 )
	{
		GL( glDeleteVertexArrays( 1, &pipeline->vertexArrayObject ) );
		pipeline->vertexArrayObject = 0;
	}
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
} ksGpuComputePipeline;

static bool ksGpuComputePipeline_Create( ksGpuContext * context, ksGpuComputePipeline * pipeline, const ksGpuComputeProgram * program )
{
	UNUSED_PARM( context );

	pipeline->program = program;

	return true;
}

static void ksGpuComputePipeline_Destroy( ksGpuContext * context, ksGpuComputePipeline * pipeline )
{
	UNUSED_PARM( context );
	UNUSED_PARM( pipeline );
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
#if USE_SYNC_OBJECT == 0
	GLsync		sync;
#elif USE_SYNC_OBJECT == 1
	EGLDisplay	display;
	EGLSyncKHR	sync;
#elif USE_SYNC_OBJECT == 2
	GLuint		computeShader;
	GLuint		computeProgram;
	GLuint		storageBuffer;
	GLuint *	mappedBuffer;
#else
	#error "invalid USE_SYNC_OBJECT setting"
#endif
} ksGpuFence;

static void ksGpuFence_Create( ksGpuContext * context, ksGpuFence * fence )
{
	UNUSED_PARM( context );

#if USE_SYNC_OBJECT == 0
	fence->sync = 0;
#elif USE_SYNC_OBJECT == 1
	fence->display = 0;
	fence->sync = EGL_NO_SYNC_KHR;
#elif USE_SYNC_OBJECT == 2
	static const char syncComputeProgramGLSL[] =
		"#version " GLSL_VERSION "\n"
		"\n"
		"layout( local_size_x = 1, local_size_y = 1 ) in;\n"
		"\n"
		"layout( std430, binding = 0 ) buffer syncBuffer { int sync[]; };\n"
		"\n"
		"void main()\n"
		"{\n"
		"	sync[0] = 0;\n"
		"}\n";
	const char * source = syncComputeProgramGLSL;
	// Create a compute program to write to a storage buffer.
	GLint r;
	GL( fence->computeShader = glCreateShader( GL_COMPUTE_SHADER ) );
	GL( glShaderSource( fence->computeShader, 1, (const char **)&source, 0 ) );
	GL( glCompileShader( fence->computeShader ) );
	GL( glGetShaderiv( fence->computeShader, GL_COMPILE_STATUS, &r ) );
	assert( r != GL_FALSE );
	GL( fence->computeProgram = glCreateProgram() );
	GL( glAttachShader( fence->computeProgram, fence->computeShader ) );
	GL( glLinkProgram( fence->computeProgram ) );
	GL( glGetProgramiv( fence->computeProgram, GL_LINK_STATUS, &r ) );
	assert( r != GL_FALSE );
	// Create the persistently mapped storage buffer.
	GL( glGenBuffers( 1, &fence->storageBuffer ) );
	GL( glBindBuffer( GL_SHADER_STORAGE_BUFFER, fence->storageBuffer ) );
	GL( glBufferStorage( GL_SHADER_STORAGE_BUFFER, sizeof( GLuint ), NULL, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT ) );
	GL( fence->mappedBuffer = (GLuint *)glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, sizeof( GLuint ), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT ) );
	GL( glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 ) );
#else
	#error "invalid USE_SYNC_OBJECT setting"
#endif
}

static void ksGpuFence_Destroy( ksGpuContext * context, ksGpuFence * fence )
{
	UNUSED_PARM( context );

#if USE_SYNC_OBJECT == 0
	if ( fence->sync != 0 )
	{
		GL( glDeleteSync( fence->sync ) );
		fence->sync = 0;
	}
#elif USE_SYNC_OBJECT == 1
	if ( fence->sync != EGL_NO_SYNC_KHR )
	{
		EGL( eglDestroySyncKHR( fence->display, fence->sync ) );
		fence->display = 0;
		fence->sync = EGL_NO_SYNC_KHR;
	}
#elif USE_SYNC_OBJECT == 2
	GL( glBindBuffer( GL_SHADER_STORAGE_BUFFER, fence->storageBuffer ) );
	GL( glUnmapBuffer( GL_SHADER_STORAGE_BUFFER ) );
	GL( glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 ) );

	GL( glDeleteBuffers( 1, &fence->storageBuffer ) );
	GL( glDeleteProgram( fence->computeProgram ) );
	GL( glDeleteShader( fence->computeShader ) );
#else
	#error "invalid USE_SYNC_OBJECT setting"
#endif
}

static void ksGpuFence_Submit( ksGpuContext * context, ksGpuFence * fence )
{
	UNUSED_PARM( context );

#if USE_SYNC_OBJECT == 0
	// Destroy any old sync object.
	if ( fence->sync != 0 )
	{
		GL( glDeleteSync( fence->sync ) );
		fence->sync = 0;
	}
	// Create and insert a new sync object.
	GL( fence->sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 ) );
	// Force flushing the commands.
	// Note that some drivers will already flush when calling glFenceSync.
	GL( glClientWaitSync( fence->sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0 ) );
#elif USE_SYNC_OBJECT == 1
	// Destroy any old sync object.
	if ( fence->sync != EGL_NO_SYNC_KHR )
	{
		EGL( eglDestroySyncKHR( fence->display, fence->sync ) );
		fence->display = 0;
		fence->sync = EGL_NO_SYNC_KHR;
	}
	// Create and insert a new sync object.
	fence->display = eglGetCurrentDisplay();
	fence->sync = eglCreateSyncKHR( fence->display, EGL_SYNC_FENCE_KHR, NULL );
	if ( fence->sync == EGL_NO_SYNC_KHR )
	{
		Error( "eglCreateSyncKHR() : EGL_NO_SYNC_KHR" );
	}
	// Force flushing the commands.
	// Note that some drivers will already flush when calling eglCreateSyncKHR.
	EGL( eglClientWaitSyncKHR( fence->display, fence->sync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 0 ) );
#elif USE_SYNC_OBJECT == 2
	// Initialize the storage buffer to 1 on the client side.
	fence->mappedBuffer[0] = 1;
	// Use a compute shader to clear the persistently mapped storage buffer.
	// This relies on a compute shader only being executed after all other work has completed.
	GL( glUseProgram( fence->computeProgram ) );
	GL( glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, fence->storageBuffer ) );
	GL( glDispatchCompute( 1, 1, 1 ) );
	GL( glUseProgram( 0 ) );
	// Flush the commands.
	// Note that some drivers may ignore a flush which could result in the storage buffer never being updated.
	GL( glFlush() );
#else
	#error "invalid USE_SYNC_OBJECT setting"
#endif
}

static bool ksGpuFence_IsSignalled( ksGpuContext * context, ksGpuFence * fence )
{
	UNUSED_PARM( context );

	if ( fence == NULL )
	{
		return false;
	}
#if USE_SYNC_OBJECT == 0
	if ( glIsSync( fence->sync ) )
	{
		GL( const GLenum result = glClientWaitSync( fence->sync, 0, 0 ) );
		if ( result == GL_WAIT_FAILED )
		{
			Error( "glClientWaitSync() : GL_WAIT_FAILED" );
		}
		if ( result != GL_TIMEOUT_EXPIRED )
		{
			return true;
		}
	}
#elif USE_SYNC_OBJECT == 1
	if ( fence->sync != EGL_NO_SYNC_KHR )
	{
		const EGLint result = eglClientWaitSyncKHR( fence->display, fence->sync, 0, 0 );
		if ( result == EGL_FALSE )
		{
			Error( "eglClientWaitSyncKHR() : EGL_FALSE" );
		}
		if ( result != EGL_TIMEOUT_EXPIRED_KHR )
		{
			return true;
		}
	}
#elif USE_SYNC_OBJECT == 2
	if ( fence->mappedBuffer[0] == 0 )
	{
		return true;
	}
#else
	#error "invalid USE_SYNC_OBJECT setting"
#endif
	return false;
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
	GLuint			beginQueries[KS_GPU_TIMER_FRAMES_DELAYED];
	GLuint			endQueries[KS_GPU_TIMER_FRAMES_DELAYED];
	int				queryIndex;
	ksNanoseconds	gpuTime;
} ksGpuTimer;

static void ksGpuTimer_Create( ksGpuContext * context, ksGpuTimer * timer )
{
	UNUSED_PARM( context );

	if ( glExtensions.timer_query )
	{
		GL( glGenQueries( KS_GPU_TIMER_FRAMES_DELAYED, timer->beginQueries ) );
		GL( glGenQueries( KS_GPU_TIMER_FRAMES_DELAYED, timer->endQueries ) );
		timer->queryIndex = 0;
		timer->gpuTime = 0;
	}
}

static void ksGpuTimer_Destroy( ksGpuContext * context, ksGpuTimer * timer )
{
	UNUSED_PARM( context );

	if ( glExtensions.timer_query )
	{
		GL( glDeleteQueries( KS_GPU_TIMER_FRAMES_DELAYED, timer->beginQueries ) );
		GL( glDeleteQueries( KS_GPU_TIMER_FRAMES_DELAYED, timer->endQueries ) );
	}
}

static ksNanoseconds ksGpuTimer_GetNanoseconds( ksGpuTimer * timer )
{
	if ( glExtensions.timer_query )
	{
		return timer->gpuTime;
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

static const void * ksGpuProgramParmState_NewPushConstantData( const ksGpuProgramParmLayout * newLayout, const int newParmIndex, const ksGpuProgramParmState * newParmState,
															const ksGpuProgramParmLayout * oldLayout, const int oldParmIndex, const ksGpuProgramParmState * oldParmState,
															const bool force )
{
#if SAVE_PUSH_CONSTANT_STATE == 1
	const ksGpuProgramParm * newParm = &newLayout->parms[newParmIndex];
	const unsigned char * newData = &newParmState->data[newLayout->offsetForIndex[newParm->index]];
	if ( force || oldLayout == NULL || oldParmIndex >= oldLayout->numParms )
	{
		return newData;
	}
	const ksGpuProgramParm * oldParm = &oldLayout->parms[oldParmIndex];
	const unsigned char * oldData = &oldParmState->data[oldLayout->offsetForIndex[oldParm->index]];
	if ( newParm->type != oldParm->type || newLayout->parmBindings[newParmIndex] != oldLayout->parmBindings[oldParmIndex] )
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
	if ( force || oldLayout == NULL || oldParmIndex >= oldLayout->numParms ||
			newLayout->parmBindings[newParmIndex] != oldLayout->parmBindings[oldParmIndex] ||
				newLayout->parms[newParmIndex].type != oldLayout->parms[oldParmIndex].type ||
					newParmState->parms[newLayout->parms[newParmIndex].index] != oldParmState->parms[oldLayout->parms[oldParmIndex].index] )
	{
		return newParmState->parms[newLayout->parms[newParmIndex].index];
	}
	return NULL;
#endif
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

typedef struct
{
	ksGpuCommandBufferType	type;
	int						numBuffers;
	int						currentBuffer;
	ksGpuFence *			fences;
	ksGpuContext *			context;
	ksGpuGraphicsCommand	currentGraphicsState;
	ksGpuComputeCommand		currentComputeState;
	ksGpuFramebuffer *		currentFramebuffer;
	ksGpuRenderPass *		currentRenderPass;
	ksGpuTextureUsage		currentTextureUsage;
} ksGpuCommandBuffer;

static void ksGpuCommandBuffer_Create( ksGpuContext * context, ksGpuCommandBuffer * commandBuffer, const ksGpuCommandBufferType type, const int numBuffers )
{
	assert( type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );

	memset( commandBuffer, 0, sizeof( ksGpuCommandBuffer ) );

	commandBuffer->type = type;
	commandBuffer->numBuffers = numBuffers;
	commandBuffer->currentBuffer = 0;
	commandBuffer->context = context;

	commandBuffer->fences = (ksGpuFence *) malloc( numBuffers * sizeof( ksGpuFence ) );

	for ( int i = 0; i < numBuffers; i++ )
	{
		ksGpuFence_Create( context, &commandBuffer->fences[i] );
	}
}

static void ksGpuCommandBuffer_Destroy( ksGpuContext * context, ksGpuCommandBuffer * commandBuffer )
{
	assert( context == commandBuffer->context );

	for ( int i = 0; i < commandBuffer->numBuffers; i++ )
	{
		ksGpuFence_Destroy( context, &commandBuffer->fences[i] );
	}

	free( commandBuffer->fences );

	memset( commandBuffer, 0, sizeof( ksGpuCommandBuffer ) );
}

void ksGpuCommandBuffer_ChangeRopState( const ksGpuRasterOperations * cmdRop, const ksGpuRasterOperations * stateRop )
{
	// Set front face.
	if ( stateRop == NULL ||
			cmdRop->frontFace != stateRop->frontFace )
	{
		GL( glFrontFace( cmdRop->frontFace ) );
	}
	// Set face culling.
	if ( stateRop == NULL ||
			cmdRop->cullMode != stateRop->cullMode )
	{
		if ( cmdRop->cullMode != KS_GPU_CULL_MODE_NONE )
		{
			GL( glEnable( GL_CULL_FACE ) );
			GL( glCullFace( cmdRop->cullMode ) );
		}
		else
		{
			GL( glDisable( GL_CULL_FACE ) );
		}
	}
	// Enable / disable depth testing.
	if ( stateRop == NULL ||
			cmdRop->depthTestEnable != stateRop->depthTestEnable )
	{
		if ( cmdRop->depthTestEnable ) { GL( glEnable( GL_DEPTH_TEST ) ); } else { GL( glDisable( GL_DEPTH_TEST ) ); }
	}
	// The depth test function is only used when depth testing is enabled.
	if ( stateRop == NULL ||
			cmdRop->depthCompare != stateRop->depthCompare )
	{
		GL( glDepthFunc( cmdRop->depthCompare ) );
	}
	// Depth is only written when depth testing is enabled.
	// Set the depth function to GL_ALWAYS to write depth without actually testing depth.
	if ( stateRop == NULL ||
			cmdRop->depthWriteEnable != stateRop->depthWriteEnable )
	{
		if ( cmdRop->depthWriteEnable ) { GL( glDepthMask( GL_TRUE ) ); } else { GL( glDepthMask( GL_FALSE ) ); }
	}
	// Enable / disable blending.
	if ( stateRop == NULL ||
			cmdRop->blendEnable != stateRop->blendEnable )
	{
		if ( cmdRop->blendEnable ) { GL( glEnable( GL_BLEND ) ); } else { GL( glDisable( GL_BLEND ) ); }
	}
	// Enable / disable writing alpha.
	if ( stateRop == NULL ||
			cmdRop->redWriteEnable != stateRop->redWriteEnable ||
			cmdRop->blueWriteEnable != stateRop->blueWriteEnable ||
			cmdRop->greenWriteEnable != stateRop->greenWriteEnable ||
			cmdRop->alphaWriteEnable != stateRop->alphaWriteEnable )
	{
		GL( glColorMask(	cmdRop->redWriteEnable ? GL_TRUE : GL_FALSE,
							cmdRop->blueWriteEnable ? GL_TRUE : GL_FALSE,
							cmdRop->greenWriteEnable ? GL_TRUE : GL_FALSE,
							cmdRop->alphaWriteEnable ? GL_TRUE : GL_FALSE ) );
	}
	// The blend equation is only used when blending is enabled.
	if ( stateRop == NULL ||
			cmdRop->blendOpColor != stateRop->blendOpColor ||
				cmdRop->blendOpAlpha != stateRop->blendOpAlpha )
	{
		GL( glBlendEquationSeparate( cmdRop->blendOpColor, cmdRop->blendOpAlpha ) );
	}
	// The blend function is only used when blending is enabled.
	if ( stateRop == NULL ||
			cmdRop->blendSrcColor != stateRop->blendSrcColor ||
				cmdRop->blendDstColor != stateRop->blendDstColor ||
					cmdRop->blendSrcAlpha != stateRop->blendSrcAlpha ||
						cmdRop->blendDstAlpha != stateRop->blendDstAlpha )
	{
		GL( glBlendFuncSeparate( cmdRop->blendSrcColor, cmdRop->blendDstColor, cmdRop->blendSrcAlpha, cmdRop->blendDstAlpha ) );
	}
	// The blend color is only used when blending is enabled.
	if ( stateRop == NULL ||
			cmdRop->blendColor.x != stateRop->blendColor.x ||
			cmdRop->blendColor.y != stateRop->blendColor.y ||
			cmdRop->blendColor.z != stateRop->blendColor.z ||
			cmdRop->blendColor.w != stateRop->blendColor.w )
	{
		GL( glBlendColor( cmdRop->blendColor.x, cmdRop->blendColor.y, cmdRop->blendColor.z, cmdRop->blendColor.w ) );
	}
}

static void ksGpuCommandBuffer_BeginPrimary( ksGpuCommandBuffer * commandBuffer )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );

	commandBuffer->currentBuffer = ( commandBuffer->currentBuffer + 1 ) % commandBuffer->numBuffers;

	ksGpuGraphicsCommand_Init( &commandBuffer->currentGraphicsState );
	ksGpuComputeCommand_Init( &commandBuffer->currentComputeState );
	commandBuffer->currentTextureUsage = KS_GPU_TEXTURE_USAGE_UNDEFINED;

	ksGpuGraphicsPipelineParms parms;
	ksGpuGraphicsPipelineParms_Init( &parms );
	ksGpuCommandBuffer_ChangeRopState( &parms.rop, NULL );

	GL( glUseProgram( 0 ) );
	GL( glBindVertexArray( 0 ) );
}

static void ksGpuCommandBuffer_EndPrimary( ksGpuCommandBuffer * commandBuffer )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );

	UNUSED_PARM( commandBuffer );
}

static ksGpuFence * ksGpuCommandBuffer_SubmitPrimary( ksGpuCommandBuffer * commandBuffer )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );

	ksGpuFence * fence = &commandBuffer->fences[commandBuffer->currentBuffer];
	ksGpuFence_Submit( commandBuffer->context, fence );

	return fence;
}

static void ksGpuCommandBuffer_ChangeTextureUsage( ksGpuCommandBuffer * commandBuffer, ksGpuTexture * texture, const ksGpuTextureUsage usage )
{
	assert( ( texture->usageFlags & usage ) != 0 );

	texture->usage = usage;

	if ( usage == commandBuffer->currentTextureUsage )
	{
		return;
	}

	const GLbitfield barriers =	( ( usage == KS_GPU_TEXTURE_USAGE_TRANSFER_SRC ) ?		GL_TEXTURE_UPDATE_BARRIER_BIT :
								( ( usage == KS_GPU_TEXTURE_USAGE_TRANSFER_DST ) ?		GL_TEXTURE_UPDATE_BARRIER_BIT :
								( ( usage == KS_GPU_TEXTURE_USAGE_SAMPLED ) ?			GL_TEXTURE_FETCH_BARRIER_BIT :
								( ( usage == KS_GPU_TEXTURE_USAGE_STORAGE ) ?			GL_SHADER_IMAGE_ACCESS_BARRIER_BIT :
								( ( usage == KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT ) ?	GL_FRAMEBUFFER_BARRIER_BIT :
								( ( usage == KS_GPU_TEXTURE_USAGE_PRESENTATION ) ?		GL_ALL_BARRIER_BITS : GL_ALL_BARRIER_BITS ) ) ) ) ) );

	GL( glMemoryBarrier( barriers ) );

	commandBuffer->currentTextureUsage = usage;
}

static void ksGpuCommandBuffer_BeginFramebuffer( ksGpuCommandBuffer * commandBuffer, ksGpuFramebuffer * framebuffer, const int arrayLayer, const ksGpuTextureUsage usage )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );
	assert( arrayLayer >= 0 && arrayLayer < framebuffer->numFramebuffersPerTexture );

	// Only advance when rendering to the first layer.
	if ( arrayLayer == 0 )
	{
		framebuffer->currentBuffer = ( framebuffer->currentBuffer + 1 ) % framebuffer->numBuffers;
	}

	GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebuffer->renderBuffers[framebuffer->currentBuffer * framebuffer->numFramebuffersPerTexture + arrayLayer] ) );

	if ( framebuffer->colorTextures != NULL )
	{
		framebuffer->colorTextures[framebuffer->currentBuffer].usage = usage;
	}
	commandBuffer->currentFramebuffer = framebuffer;
}

static void ksGpuCommandBuffer_EndFramebuffer( ksGpuCommandBuffer * commandBuffer, ksGpuFramebuffer * framebuffer, const int arrayLayer, const ksGpuTextureUsage usage )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == framebuffer );
	assert( commandBuffer->currentRenderPass == NULL );
	assert( arrayLayer >= 0 && arrayLayer < framebuffer->numFramebuffersPerTexture );

	UNUSED_PARM( framebuffer );
	UNUSED_PARM( arrayLayer );

	// If clamp to border is not available.
	if ( !glExtensions.texture_clamp_to_border )
	{
		// If rendering to a texture.
		if ( framebuffer->renderBuffers[framebuffer->currentBuffer * framebuffer->numFramebuffersPerTexture + arrayLayer] != 0 )
		{
			// Explicitly clear the border texels to black if the texture has clamp-to-border set.
			const ksGpuTexture * texture = &framebuffer->colorTextures[framebuffer->currentBuffer];
			if ( texture->wrapMode == KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER )
			{
				// Clear to fully opaque black.
				GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
				// bottom
				GL( glScissor( 0, 0, texture->width, 1 ) );
				GL( glClear( GL_COLOR_BUFFER_BIT ) );
				// top
				GL( glScissor( 0, texture->height - 1, texture->width, 1 ) );
				GL( glClear( GL_COLOR_BUFFER_BIT ) );
				// left
				GL( glScissor( 0, 0, 1, texture->height ) );
				GL( glClear( GL_COLOR_BUFFER_BIT ) );
				// right
				GL( glScissor( texture->width - 1, 0, 1, texture->height ) );
				GL( glClear( GL_COLOR_BUFFER_BIT ) );
			}
		}
	}

#if defined( OS_ANDROID )
	// If this framebuffer has a depth buffer.
	if ( framebuffer->depthBuffer != 0 )
	{
		// Discard the depth buffer, so a tiler won't need to write it back out to memory.
		const GLenum depthAttachment[1] = { GL_DEPTH_ATTACHMENT };
		GL( glInvalidateFramebuffer( GL_DRAW_FRAMEBUFFER, 1, depthAttachment ) );
	}
#endif

	if ( framebuffer->resolveBuffers != framebuffer->renderBuffers )
	{
		const ksScreenRect rect = ksGpuFramebuffer_GetRect( framebuffer );
		glBindFramebuffer( GL_READ_FRAMEBUFFER, framebuffer->renderBuffers[framebuffer->currentBuffer * framebuffer->numFramebuffersPerTexture + arrayLayer] );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebuffer->resolveBuffers[framebuffer->currentBuffer * framebuffer->numFramebuffersPerTexture + arrayLayer] );
		glBlitFramebuffer(	rect.x, rect.y, rect.width, rect.height,
							rect.x, rect.y, rect.width, rect.height,
							GL_COLOR_BUFFER_BIT, GL_NEAREST );
		glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
	}

	GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );

	if ( framebuffer->colorTextures != NULL )
	{
		framebuffer->colorTextures[framebuffer->currentBuffer].usage = usage;
	}
	commandBuffer->currentFramebuffer = NULL;
}

static void ksGpuCommandBuffer_BeginTimer( ksGpuCommandBuffer * commandBuffer, ksGpuTimer * timer )
{
	UNUSED_PARM( commandBuffer );

	if ( glExtensions.timer_query )
	{
		if ( timer->queryIndex >= KS_GPU_TIMER_FRAMES_DELAYED )
		{
			GLuint64 beginGpuTime = 0;
			GL( glGetQueryObjectui64v( timer->beginQueries[timer->queryIndex % KS_GPU_TIMER_FRAMES_DELAYED], GL_QUERY_RESULT, &beginGpuTime ) );
			GLuint64 endGpuTime = 0;
			GL( glGetQueryObjectui64v( timer->endQueries[timer->queryIndex % KS_GPU_TIMER_FRAMES_DELAYED], GL_QUERY_RESULT, &endGpuTime ) );

			timer->gpuTime = ( endGpuTime - beginGpuTime );
		}

		GL( glQueryCounter( timer->beginQueries[timer->queryIndex % KS_GPU_TIMER_FRAMES_DELAYED], GL_TIMESTAMP ) );
	}
}

static void ksGpuCommandBuffer_EndTimer( ksGpuCommandBuffer * commandBuffer, ksGpuTimer * timer )
{
	UNUSED_PARM( commandBuffer );

	if ( glExtensions.timer_query )
	{
		GL( glQueryCounter( timer->endQueries[timer->queryIndex % KS_GPU_TIMER_FRAMES_DELAYED], GL_TIMESTAMP ) );
		timer->queryIndex++;
	}
}

static void ksGpuCommandBuffer_BeginRenderPass( ksGpuCommandBuffer * commandBuffer, ksGpuRenderPass * renderPass, ksGpuFramebuffer * framebuffer, const ksScreenRect * rect )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentRenderPass == NULL );
	assert( commandBuffer->currentFramebuffer == framebuffer );

	UNUSED_PARM( framebuffer );

	if ( ( renderPass->flags & ( KS_GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER | KS_GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER ) ) != 0 )
	{
		GL( glEnable( GL_SCISSOR_TEST ) );
		GL( glScissor( rect->x, rect->y, rect->width, rect->height ) );
		GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
		GL( glClear(	( ( ( renderPass->flags & KS_GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER ) != 0 ) ? GL_COLOR_BUFFER_BIT : 0 ) |
						( ( ( renderPass->flags & KS_GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER ) != 0 ) ? GL_DEPTH_BUFFER_BIT : 0 ) ) );
	}

	commandBuffer->currentRenderPass = renderPass;
}

static void ksGpuCommandBuffer_EndRenderPass( ksGpuCommandBuffer * commandBuffer, ksGpuRenderPass * renderPass )
{
	assert( commandBuffer->type == KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentRenderPass == renderPass );

	UNUSED_PARM( renderPass );

	commandBuffer->currentRenderPass = NULL;
}

static void ksGpuCommandBuffer_SetViewport( ksGpuCommandBuffer * commandBuffer, const ksScreenRect * rect )
{
	UNUSED_PARM( commandBuffer );

	GL( glViewport( rect->x, rect->y, rect->width, rect->height ) );
}

static void ksGpuCommandBuffer_SetScissor( ksGpuCommandBuffer * commandBuffer, const ksScreenRect * rect )
{
	UNUSED_PARM( commandBuffer );

	GL( glEnable( GL_SCISSOR_TEST ) );
	GL( glScissor( rect->x, rect->y, rect->width, rect->height ) );
}

static void ksGpuCommandBuffer_UpdateProgramParms( const ksGpuProgramParmLayout * newLayout,
												const ksGpuProgramParmLayout * oldLayout,
												const ksGpuProgramParmState * newParmState,
												const ksGpuProgramParmState * oldParmState,
												const bool force )
{
	const ksGpuTexture * oldSampledTextures[MAX_PROGRAM_PARMS] = { 0 };
	const ksGpuTexture * oldStorageTextures[MAX_PROGRAM_PARMS] = { 0 };
	const ksGpuBuffer * oldUniformBuffers[MAX_PROGRAM_PARMS] = { 0 };
	const ksGpuBuffer * oldStorageBuffers[MAX_PROGRAM_PARMS] = { 0 };
	int oldPushConstantParms[MAX_PROGRAM_PARMS] = { 0 };

	if ( oldLayout != NULL )
	{
		// Unbind from the bind points that will no longer be used, and gather
		// the objects that are bound at the bind points that will be used.
		for ( int i = 0; i < oldLayout->numParms; i++ )
		{
			const int index = oldLayout->parms[i].index;
			const int binding = oldLayout->parmBindings[i];

			if ( oldLayout->parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED )
			{
				if ( binding >= newLayout->numSampledTextureBindings )
				{
					const ksGpuTexture * stateTexture = (const ksGpuTexture *)oldParmState->parms[index];
					GL( glActiveTexture( GL_TEXTURE0 + binding ) );
					GL( glBindTexture( stateTexture->target, 0 ) );
				}
				else
				{
					oldSampledTextures[binding] = (const ksGpuTexture *)oldParmState->parms[index];
				}
			}
			else if ( oldLayout->parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE )
			{
				if ( binding >= newLayout->numStorageTextureBindings )
				{
					GL( glBindImageTexture( binding, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8 ) );
				}
				else
				{
					oldStorageTextures[binding] = (const ksGpuTexture *)oldParmState->parms[index];
				}
			}
			else if ( oldLayout->parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM )
			{
				if ( binding >= newLayout->numUniformBufferBindings )
				{
					GL( glBindBufferBase( GL_UNIFORM_BUFFER, binding, 0 ) );
				}
				else
				{
					oldUniformBuffers[binding] = (const ksGpuBuffer *)oldParmState->parms[index];
				}
			}
			else if ( oldLayout->parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
			{
				if ( binding >= newLayout->numStorageBufferBindings )
				{
					GL( glBindBufferBase( GL_SHADER_STORAGE_BUFFER, binding, 0 ) );
				}
				else
				{
					oldStorageBuffers[binding] = (const ksGpuBuffer *)oldParmState->parms[index];
				}
			}
			else
			{
				oldPushConstantParms[binding] = i;
			}
		}
	}

	// Update the bind points.
	for ( int i = 0; i < newLayout->numParms; i++ )
	{
		const int index = newLayout->parms[i].index;
		const int binding = newLayout->parmBindings[i];

		assert( newParmState->parms[index] != NULL );
		if ( newLayout->parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED )
		{
			const ksGpuTexture * texture = (const ksGpuTexture *)newParmState->parms[index];
			assert( texture->usage == KS_GPU_TEXTURE_USAGE_SAMPLED );
			if ( force || texture != oldSampledTextures[binding] )
			{
				GL( glActiveTexture( GL_TEXTURE0 + binding ) );
				GL( glBindTexture( texture->target, texture->texture ) );
			}
		}
		else if ( newLayout->parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE )
		{
			const ksGpuTexture * texture = (const ksGpuTexture *)newParmState->parms[index];
			assert( texture->usage == KS_GPU_TEXTURE_USAGE_STORAGE );
			if ( force || texture != oldStorageTextures[binding] )
			{
				const GLenum access =	( ( newLayout->parms[i].access == KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY ) ?		GL_READ_ONLY :
										( ( newLayout->parms[i].access == KS_GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY ) ?	GL_WRITE_ONLY :
										( ( newLayout->parms[i].access == KS_GPU_PROGRAM_PARM_ACCESS_READ_WRITE ) ?	GL_READ_WRITE :
																													0 ) ) );
				GL( glBindImageTexture( binding, texture->texture, 0, GL_FALSE, 0, access, texture->format ) );
			}
		}
		else if ( newLayout->parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM )
		{
			const ksGpuBuffer * buffer = (const ksGpuBuffer *)newParmState->parms[index];
			assert( buffer->target == GL_UNIFORM_BUFFER );
			if ( force || buffer != oldUniformBuffers[binding] )
			{
				GL( glBindBufferBase( GL_UNIFORM_BUFFER, binding, buffer->buffer ) );
			}
		}
		else if ( newLayout->parms[i].type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
		{
			const ksGpuBuffer * buffer = (const ksGpuBuffer *)newParmState->parms[index];
			assert( buffer->target == GL_SHADER_STORAGE_BUFFER );
			if ( force || buffer != oldStorageBuffers[binding] )
			{
				GL( glBindBufferBase( GL_SHADER_STORAGE_BUFFER, binding, buffer->buffer ) );
			}
		}
		else
		{
			const void * newData = ksGpuProgramParmState_NewPushConstantData( newLayout, i, newParmState, oldLayout, oldPushConstantParms[binding], oldParmState, force );
			if ( newData != NULL )
			{
				const GLint location = newLayout->parmLocations[i];
				switch ( newLayout->parms[i].type )
				{
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT:				GL( glUniform1iv( location, 1, (const GLint *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2:		GL( glUniform2iv( location, 1, (const GLint *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3:		GL( glUniform3iv( location, 1, (const GLint *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4:		GL( glUniform4iv( location, 1, (const GLint *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT:				GL( glUniform1fv( location, 1, (const GLfloat *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2:		GL( glUniform2fv( location, 1, (const GLfloat *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3:		GL( glUniform3fv( location, 1, (const GLfloat *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4:		GL( glUniform4fv( location, 1, (const GLfloat *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2:	GL( glUniformMatrix2fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3:	GL( glUniformMatrix2x3fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4:	GL( glUniformMatrix2x4fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2:	GL( glUniformMatrix3x2fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3:	GL( glUniformMatrix3fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4:	GL( glUniformMatrix3x4fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2:	GL( glUniformMatrix4x2fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3:	GL( glUniformMatrix4x3fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4:	GL( glUniformMatrix4fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					default: assert( false ); break;
				}
			}
		}
	}
}

static void ksGpuCommandBuffer_SubmitGraphicsCommand( ksGpuCommandBuffer * commandBuffer, const ksGpuGraphicsCommand * command )
{
	assert( commandBuffer->currentRenderPass != NULL );

	const ksGpuGraphicsCommand * state = &commandBuffer->currentGraphicsState;

	ksGpuCommandBuffer_ChangeRopState( &command->pipeline->rop, ( state->pipeline != NULL ) ? &state->pipeline->rop : NULL );

	// Compare programs based on a vertex/fragment source code hash value to minimize state changes when
	// the same source code is compiled to programs in different locations.
	const bool differentProgram = ( state->pipeline == NULL || command->pipeline->program->hash != state->pipeline->program->hash );

	if ( differentProgram )
	{
		GL( glUseProgram( command->pipeline->program->program ) );
	}

	ksGpuCommandBuffer_UpdateProgramParms( &command->pipeline->program->parmLayout,
										( state->pipeline != NULL ) ? &state->pipeline->program->parmLayout : NULL,
										&command->parmState, &state->parmState, differentProgram );

	if ( command->pipeline != state->pipeline )
	{
		GL( glBindVertexArray( command->pipeline->vertexArrayObject ) );
	}

	const GLenum indexType = ( sizeof( ksGpuTriangleIndex ) == sizeof( GLuint ) ) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
	if ( command->numInstances > 1 )
	{
		GL( glDrawElementsInstanced( GL_TRIANGLES, command->pipeline->geometry->indexCount, indexType, NULL, command->numInstances ) );
	}
	else
	{
		GL( glDrawElements( GL_TRIANGLES, command->pipeline->geometry->indexCount, indexType, NULL ) );
	}

	commandBuffer->currentGraphicsState = *command;
	commandBuffer->currentTextureUsage = KS_GPU_TEXTURE_USAGE_UNDEFINED;
}

static void ksGpuCommandBuffer_SubmitComputeCommand( ksGpuCommandBuffer * commandBuffer, const ksGpuComputeCommand * command )
{
	assert( commandBuffer->currentRenderPass == NULL );

	const ksGpuComputeCommand * state = &commandBuffer->currentComputeState;

	// Compare programs based on a kernel source code hash value to minimize state changes when
	// the same source code is compiled to programs in different locations.
	const bool differentProgram = ( state->pipeline == NULL || command->pipeline->program->hash != state->pipeline->program->hash );

	if ( differentProgram )
	{
		GL( glUseProgram( command->pipeline->program->program ) );
	}

	ksGpuCommandBuffer_UpdateProgramParms( &command->pipeline->program->parmLayout,
										( state->pipeline != NULL ) ? &state->pipeline->program->parmLayout : NULL,
										&command->parmState, &state->parmState, differentProgram );

	GL( glDispatchCompute( command->x, command->y, command->z ) );

	commandBuffer->currentComputeState = *command;
	commandBuffer->currentTextureUsage = KS_GPU_TEXTURE_USAGE_UNDEFINED;
}

static ksGpuBuffer * ksGpuCommandBuffer_MapBuffer( ksGpuCommandBuffer * commandBuffer, ksGpuBuffer * buffer, void ** data )
{
	UNUSED_PARM( commandBuffer );

	GL( glBindBuffer( buffer->target, buffer->buffer ) );
	GL( *data = glMapBufferRange( buffer->target, 0, buffer->size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT ) );
	GL( glBindBuffer( buffer->target, 0 ) );

	return buffer;
}

static void ksGpuCommandBuffer_UnmapBuffer( ksGpuCommandBuffer * commandBuffer, ksGpuBuffer * buffer, ksGpuBuffer * mappedBuffer, const ksGpuBufferUnmapType type )
{
	UNUSED_PARM( commandBuffer );
	UNUSED_PARM( buffer );

	assert( buffer == mappedBuffer );

	GL( glBindBuffer( mappedBuffer->target, mappedBuffer->buffer ) );
	GL( glUnmapBuffer( mappedBuffer->target ) );
	GL( glBindBuffer( mappedBuffer->target, 0 ) );

	if ( type == KS_GPU_BUFFER_UNMAP_TYPE_COPY_BACK )
	{
		// Can only copy outside a render pass.
		assert( commandBuffer->currentRenderPass == NULL );
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

static void ksGpuCommandBuffer_Blit( ksGpuCommandBuffer * commandBuffer, ksGpuFramebuffer * srcFramebuffer, ksGpuFramebuffer * dstFramebuffer )
{
	UNUSED_PARM( commandBuffer );

	ksGpuTexture * srcTexture = &srcFramebuffer->colorTextures[srcFramebuffer->currentBuffer];
	ksGpuTexture * dstTexture = &dstFramebuffer->colorTextures[dstFramebuffer->currentBuffer];
	assert( srcTexture->width == dstTexture->width && srcTexture->height == dstTexture->height );
	UNUSED_PARM( dstTexture );

	GL( glBindFramebuffer( GL_READ_FRAMEBUFFER, srcFramebuffer->renderBuffers[srcFramebuffer->currentBuffer] ) );
	GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, dstFramebuffer->renderBuffers[dstFramebuffer->currentBuffer] ) );
	GL( glBlitFramebuffer( 0, 0, srcTexture->width, srcTexture->height,
							0, 0, srcTexture->width, srcTexture->height, GL_COLOR_BUFFER_BIT, GL_NEAREST ) );
	GL( glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 ) );
	GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
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
#if OPENGL_COMPUTE_ENABLED == 1
	struct
	{
		ksGpuBuffer				barValueBuffer;
		ksGpuBuffer				barColorBuffer;
		ksVector2i				barGraphOffset;
		ksGpuComputeProgram		program;
		ksGpuComputePipeline	pipeline;
	} compute;
#endif
} ksBarGraph;

static const ksGpuProgramParm barGraphGraphicsProgramParms[] =
{
	{ 0 }
};

static const char barGraphVertexProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"in vec3 vertexPosition;\n"
	"in mat4 vertexTransform;\n"
	"out vec4 fragmentColor;\n"
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

static const char barGraphFragmentProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"in lowp vec4 fragmentColor;\n"
	"out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	outColor = fragmentColor;\n"
	"}\n";

#if OPENGL_COMPUTE_ENABLED == 1

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
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_VALUES,			"barValueBuffer",	0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_COLORS,			"barColorBuffer",	1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BACK_GROUND_COLOR,	"backgroundColor",	0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_GRAPH_OFFSET,		"barGraphOffset",	1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_BARS,				"numBars",			2 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_STACKED,			"numStacked",		3 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_INDEX,			"barIndex",			4 }
};

#define BARGRAPH_LOCAL_SIZE_X	8
#define BARGRAPH_LOCAL_SIZE_Y	8

static const char barGraphComputeProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"\n"
	"layout( local_size_x = " STRINGIFY( BARGRAPH_LOCAL_SIZE_X ) ", local_size_y = " STRINGIFY( BARGRAPH_LOCAL_SIZE_Y ) " ) in;\n"
	"\n"
	"layout( rgba8, binding = 0 ) uniform writeonly " ES_HIGHP " image2D dest;\n"
	"layout( std430, binding = 0 ) buffer barValueBuffer { float barValues[]; };\n"
	"layout( std430, binding = 1 ) buffer barColorBuffer { vec4 barColors[]; };\n"
	"uniform lowp vec4 backgroundColor;\n"
	"uniform ivec2 barGraphOffset;\n"
	"uniform int numBars;\n"
	"uniform int numStacked;\n"
 	"uniform int barIndex;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	ivec2 barGraph = ivec2( gl_GlobalInvocationID.xy );\n"
	"	ivec2 barGraphSize = ivec2( gl_NumWorkGroups.xy * gl_WorkGroupSize.xy );\n"
	"\n"
	"	int index = barGraph.x * numBars / barGraphSize.x;\n"
	"	int barOffset = ( ( barIndex + index ) % numBars ) * numStacked;\n"
	"	float barColorScale = ( ( index & 1 ) != 0 ) ? 0.75f : 1.0f;\n"
	"\n"
	"	vec4 rgba = backgroundColor;\n"
	"	float localY = float( barGraph.y );\n"
	"	float stackedBarValue = 0.0f;\n"
	"	for ( int i = 0; i < numStacked; i++ )\n"
	"	{\n"
	"		stackedBarValue += barValues[barOffset + i];\n"
	"		if ( localY < stackedBarValue * float( barGraphSize.y ) )\n"
	"		{\n"
	"			rgba = barColors[barOffset + i] * barColorScale;\n"
	"			break;\n"
	"		}\n"
	"	}\n"
	"\n"
	"	imageStore( dest, barGraphOffset + barGraph, rgba );\n"
	"}\n";

#endif

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

#if OPENGL_COMPUTE_ENABLED == 1
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
#endif
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

#if OPENGL_COMPUTE_ENABLED == 1
	// compute
	{
		ksGpuComputePipeline_Destroy( context, &barGraph->compute.pipeline );
		ksGpuComputeProgram_Destroy( context, &barGraph->compute.program );
		ksGpuBuffer_Destroy( context, &barGraph->compute.barValueBuffer );
		ksGpuBuffer_Destroy( context, &barGraph->compute.barColorBuffer );
	}
#endif
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
#if OPENGL_COMPUTE_ENABLED == 1
	void * barValues = NULL;
	ksGpuBuffer * mappedBarValueBuffer = ksGpuCommandBuffer_MapBuffer( commandBuffer, &barGraph->compute.barValueBuffer, &barValues );
	memcpy( barValues, barGraph->barValues, barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barValues[0] ) );
	ksGpuCommandBuffer_UnmapBuffer( commandBuffer, &barGraph->compute.barValueBuffer, mappedBarValueBuffer, KS_GPU_BUFFER_UNMAP_TYPE_COPY_BACK );

	void * barColors = NULL;
	ksGpuBuffer * mappedBarColorBuffer = ksGpuCommandBuffer_MapBuffer( commandBuffer, &barGraph->compute.barColorBuffer, &barColors );
	memcpy( barColors, barGraph->barColors, barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barColors[0] ) );
	ksGpuCommandBuffer_UnmapBuffer( commandBuffer, &barGraph->compute.barColorBuffer, mappedBarColorBuffer, KS_GPU_BUFFER_UNMAP_TYPE_COPY_BACK );
#else
	UNUSED_PARM( commandBuffer );
	UNUSED_PARM( barGraph );
#endif
}

static void ksBarGraph_RenderCompute( ksGpuCommandBuffer * commandBuffer, ksBarGraph * barGraph, ksGpuFramebuffer * framebuffer )
{
#if OPENGL_COMPUTE_ENABLED == 1
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
#else
	UNUSED_PARM( commandBuffer );
	UNUSED_PARM( barGraph );
	UNUSED_PARM( framebuffer );
#endif
}

/*
================================================================================================================================

Time warp bar graphs.

ksTimeWarpBarGraphs

static void ksTimeWarpBarGraphs_Create( ksGpuContext * context, ksTimeWarpBarGraphs * bargraphs, ksGpuFramebuffer * framebuffer );
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
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,		KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,	"TimeWarpEndTransform",		1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER,		"ArrayLayer",				2 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT,	KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE,			"Texture",					0 }
};

static const char timeWarpSpatialVertexProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform highp mat3x4 TimeWarpStartTransform;\n"
	"uniform highp mat3x4 TimeWarpEndTransform;\n"
	"in highp vec3 vertexPosition;\n"
	"in highp vec2 vertexUv1;\n"
	"out mediump vec2 fragmentUv1;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"void main( void )\n"
	"{\n"
	"	gl_Position = vec4( vertexPosition, 1.0 );\n"
	"\n"
	"	float displayFraction = vertexPosition.x * 0.5 + 0.5;\n"	// landscape left-to-right
	"\n"
	"	vec3 startUv1 = vec4( vertexUv1, -1, 1 ) * TimeWarpStartTransform;\n"
	"	vec3 endUv1 = vec4( vertexUv1, -1, 1 ) * TimeWarpEndTransform;\n"
	"	vec3 curUv1 = mix( startUv1, endUv1, displayFraction );\n"
	"	fragmentUv1 = curUv1.xy * ( 1.0 / max( curUv1.z, 0.00001 ) );\n"
	"}\n";

static const char timeWarpSpatialFragmentProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform int ArrayLayer;\n"
	"uniform highp sampler2DArray Texture;\n"
	"in mediump vec2 fragmentUv1;\n"
	"out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	outColor = texture( Texture, vec3( fragmentUv1, ArrayLayer ) );\n"
	"}\n";

static const ksGpuProgramParm timeWarpChromaticGraphicsProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,		KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,	"TimeWarpStartTransform",	0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,		KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,	"TimeWarpEndTransform",		1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER,		"ArrayLayer",				2 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT,	KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE,			"Texture",					0 }
};

static const char timeWarpChromaticVertexProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform highp mat3x4 TimeWarpStartTransform;\n"
	"uniform highp mat3x4 TimeWarpEndTransform;\n"
	"in highp vec3 vertexPosition;\n"
	"in highp vec2 vertexUv0;\n"
	"in highp vec2 vertexUv1;\n"
	"in highp vec2 vertexUv2;\n"
	"out mediump vec2 fragmentUv0;\n"
	"out mediump vec2 fragmentUv1;\n"
	"out mediump vec2 fragmentUv2;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"void main( void )\n"
	"{\n"
	"	gl_Position = vec4( vertexPosition, 1.0 );\n"
	"\n"
	"	float displayFraction = vertexPosition.x * 0.5 + 0.5;\n"	// landscape left-to-right
	"\n"
	"	vec3 startUv0 = vec4( vertexUv0, -1, 1 ) * TimeWarpStartTransform;\n"
	"	vec3 startUv1 = vec4( vertexUv1, -1, 1 ) * TimeWarpStartTransform;\n"
	"	vec3 startUv2 = vec4( vertexUv2, -1, 1 ) * TimeWarpStartTransform;\n"
	"\n"
	"	vec3 endUv0 = vec4( vertexUv0, -1, 1 ) * TimeWarpEndTransform;\n"
	"	vec3 endUv1 = vec4( vertexUv1, -1, 1 ) * TimeWarpEndTransform;\n"
	"	vec3 endUv2 = vec4( vertexUv2, -1, 1 ) * TimeWarpEndTransform;\n"
	"\n"
	"	vec3 curUv0 = mix( startUv0, endUv0, displayFraction );\n"
	"	vec3 curUv1 = mix( startUv1, endUv1, displayFraction );\n"
	"	vec3 curUv2 = mix( startUv2, endUv2, displayFraction );\n"
	"\n"
	"	fragmentUv0 = curUv0.xy * ( 1.0 / max( curUv0.z, 0.00001 ) );\n"
	"	fragmentUv1 = curUv1.xy * ( 1.0 / max( curUv1.z, 0.00001 ) );\n"
	"	fragmentUv2 = curUv2.xy * ( 1.0 / max( curUv2.z, 0.00001 ) );\n"
	"}\n";

static const char timeWarpChromaticFragmentProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform int ArrayLayer;\n"
	"uniform highp sampler2DArray Texture;\n"
	"in mediump vec2 fragmentUv0;\n"
	"in mediump vec2 fragmentUv1;\n"
	"in mediump vec2 fragmentUv2;\n"
	"out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	outColor.r = texture( Texture, vec3( fragmentUv0, ArrayLayer ) ).r;\n"
	"	outColor.g = texture( Texture, vec3( fragmentUv1, ArrayLayer ) ).g;\n"
	"	outColor.b = texture( Texture, vec3( fragmentUv2, ArrayLayer ) ).b;\n"
	"	outColor.a = 1.0;\n"
	"}\n";

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

#if GL_FINISH_SYNC == 1
	GL( glFinish() );
#endif
}

/*
================================================================================================================================

Time warp compute rendering.

ksTimeWarpCompute

static void ksTimeWarpCompute_Create( ksGpuContext * context, ksTimeWarpCompute * compute, const ksHmdInfo * hmdInfo,
									ksGpuRenderPass * renderPass, ksGpuWindow * window );
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

#if OPENGL_COMPUTE_ENABLED == 1

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
	ksGpuFramebuffer		framebuffer;
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
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,		KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_DIMENSIONS,		"dimensions",				0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE,				"eye",						1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,	"timeWarpStartTransform",	2 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,		"timeWarpEndTransform",		3 }
};

#define TRANSFORM_LOCAL_SIZE_X		8
#define TRANSFORM_LOCAL_SIZE_Y		8

static const char timeWarpTransformComputeProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"\n"
	"layout( local_size_x = " STRINGIFY( TRANSFORM_LOCAL_SIZE_X ) ", local_size_y = " STRINGIFY( TRANSFORM_LOCAL_SIZE_Y ) " ) in;\n"
	"\n"
	"layout( rgba16f, binding = 0 ) uniform writeonly " ES_HIGHP " image2D dst;\n"
	"layout( rgba32f, binding = 1 ) uniform readonly " ES_HIGHP " image2D src;\n"
	"uniform highp mat3x4 timeWarpStartTransform;\n"
 	"uniform highp mat3x4 timeWarpEndTransform;\n"
	"uniform ivec2 dimensions;\n"
	"uniform int eye;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	ivec2 mesh = ivec2( gl_GlobalInvocationID.xy );\n"
	"	if ( mesh.x >= dimensions.x || mesh.y >= dimensions.y )\n"
	"	{\n"
	"		return;\n"
	"	}\n"
	"	int eyeTilesWide = int( gl_NumWorkGroups.x * gl_WorkGroupSize.x ) - 1;\n"
	"	int eyeTilesHigh = int( gl_NumWorkGroups.y * gl_WorkGroupSize.y ) - 1;\n"
	"\n"
	"	vec2 coords = imageLoad( src, mesh ).xy;\n"
	"\n"
	"	float displayFraction = float( eye * eyeTilesWide + mesh.x ) / ( float( eyeTilesWide ) * 2.0f );\n"		// landscape left-to-right
	"	vec3 start = vec4( coords, -1.0f, 1.0f ) * timeWarpStartTransform;\n"
	"	vec3 end = vec4( coords, -1.0f, 1.0f ) * timeWarpEndTransform;\n"
	"	vec3 cur = start + displayFraction * ( end - start );\n"
	"	float rcpZ = 1.0f / cur.z;\n"
	"\n"
	"	imageStore( dst, mesh, vec4( cur.xy * rcpZ, 0.0f, 0.0f ) );\n"
	"}\n";

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
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE,			"eyeImage",			0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G,		"warpImageG",		1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE,		"imageScale",		0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS,		"imageBias",		1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET,	"eyePixelOffset",	3 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER,		"imageLayer",		2 }
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
	"layout( rgba8, binding = 0 ) uniform writeonly " ES_HIGHP " image2D dest;\n"
	"uniform highp sampler2DArray eyeImage;\n"
	"uniform highp sampler2D warpImageG;\n"
	"uniform highp vec2 imageScale;\n"
	"uniform highp vec2 imageBias;\n"
	"uniform ivec2 eyePixelOffset;\n"
	"uniform int imageLayer;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	vec2 tile = ( vec2( gl_GlobalInvocationID.xy ) + vec2( 0.5f ) ) * imageScale + imageBias;\n"
	"\n"
	"	vec2 eyeCoords = texture( warpImageG, tile ).xy;\n"
	"\n"
	"	vec4 rgba = texture( eyeImage, vec3( eyeCoords, imageLayer ) );\n"
	"\n"
	"	imageStore( dest, ivec2( int( gl_GlobalInvocationID.x ) + eyePixelOffset.x, eyePixelOffset.y - 1 - int( gl_GlobalInvocationID.y ) ), rgba );\n"
	"}\n";

static const ksGpuProgramParm timeWarpChromaticComputeProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,				KS_GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST,				"dest",				0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE,			"eyeImage",			0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_R,		"warpImageR",		1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G,		"warpImageG",		2 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_B,		"warpImageB",		3 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE,		"imageScale",		0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS,		"imageBias",		1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET,	"eyePixelOffset",	3 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_COMPUTE, KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER,		"imageLayer",		2 }
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
	"layout( rgba8, binding = 0 ) uniform writeonly " ES_HIGHP " image2D dest;\n"
	"uniform highp sampler2DArray eyeImage;\n"
	"uniform highp sampler2D warpImageR;\n"
	"uniform highp sampler2D warpImageG;\n"
	"uniform highp sampler2D warpImageB;\n"
	"uniform highp vec2 imageScale;\n"
	"uniform highp vec2 imageBias;\n"
	"uniform ivec2 eyePixelOffset;\n"
	"uniform int imageLayer;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	vec2 tile = ( vec2( gl_GlobalInvocationID.xy ) + vec2( 0.5f ) ) * imageScale + imageBias;\n"
	"\n"
	"	vec2 eyeCoordsR = texture( warpImageR, tile ).xy;\n"
	"	vec2 eyeCoordsG = texture( warpImageG, tile ).xy;\n"
	"	vec2 eyeCoordsB = texture( warpImageB, tile ).xy;\n"
	"\n"
	"	vec4 rgba;\n"
	"	rgba.x = texture( eyeImage, vec3( eyeCoordsR, imageLayer ) ).x;\n"
	"	rgba.y = texture( eyeImage, vec3( eyeCoordsG, imageLayer ) ).y;\n"
	"	rgba.z = texture( eyeImage, vec3( eyeCoordsB, imageLayer ) ).z;\n"
	"	rgba.w = 1.0f;\n"
	"\n"
	"	imageStore( dest, ivec2( int( gl_GlobalInvocationID.x ) + eyePixelOffset.x, eyePixelOffset.y - 1 - int( gl_GlobalInvocationID.y ) ), rgba );\n"
	"}\n";

static void ksTimeWarpCompute_Create( ksGpuContext * context, ksTimeWarpCompute * compute,
									const ksHmdInfo * hmdInfo, ksGpuRenderPass * renderPass, ksGpuWindow * window )
{
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

	ksGpuFramebuffer_CreateFromTextures( context, &compute->framebuffer, renderPass, window->windowWidth, window->windowHeight, 1 );
}

static void ksTimeWarpCompute_Destroy( ksGpuContext * context, ksTimeWarpCompute * compute )
{
	ksGpuFramebuffer_Destroy( context, &compute->framebuffer );

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
	ksGpuCommandBuffer_BeginFramebuffer( commandBuffer, &compute->framebuffer, 0, KS_GPU_TEXTURE_USAGE_STORAGE );

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
	ksGpuCommandBuffer_ChangeTextureUsage( commandBuffer, ksGpuFramebuffer_GetColorTexture( &compute->framebuffer ), KS_GPU_TEXTURE_USAGE_STORAGE );

	const int screenWidth = ksGpuFramebuffer_GetWidth( &compute->framebuffer );
	const int screenHeight = ksGpuFramebuffer_GetHeight( &compute->framebuffer );
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
		ksGpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST, ksGpuFramebuffer_GetColorTexture( &compute->framebuffer ) );
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
	ksTimeWarpBarGraphs_RenderCompute( commandBuffer, bargraphs, &compute->framebuffer );

	const ksNanoseconds t2 = GetTimeNanoseconds();

	ksGpuCommandBuffer_Blit( commandBuffer, &compute->framebuffer, framebuffer );

	ksGpuCommandBuffer_EndTimer( commandBuffer, &compute->timeWarpGpuTime );

	ksGpuCommandBuffer_EndFramebuffer( commandBuffer, &compute->framebuffer, 0, KS_GPU_TEXTURE_USAGE_PRESENTATION );
	ksGpuCommandBuffer_EndPrimary( commandBuffer );

	ksGpuCommandBuffer_SubmitPrimary( commandBuffer );

	const ksNanoseconds t3 = GetTimeNanoseconds();

	cpuTimes[PROFILE_TIME_TIME_WARP] = t1 - t0;
	cpuTimes[PROFILE_TIME_BAR_GRAPHS] = t2 - t1;
	cpuTimes[PROFILE_TIME_BLIT] = t3 - t2;

	const ksNanoseconds barGraphGpuTime = ksTimeWarpBarGraphs_GetGpuNanosecondsCompute( bargraphs );

	gpuTimes[PROFILE_TIME_TIME_WARP] = ksGpuTimer_GetNanoseconds( &compute->timeWarpGpuTime ) - barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BAR_GRAPHS] = barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BLIT] = 0;

#if GL_FINISH_SYNC == 1
	GL( glFinish() );
#endif
}

#else

typedef struct
{
	int		empty;
} ksTimeWarpCompute;

static void ksTimeWarpCompute_Create( ksGpuContext * context, ksTimeWarpCompute * compute,
									const ksHmdInfo * hmdInfo, ksGpuRenderPass * renderPass, ksGpuWindow * window )
{
	UNUSED_PARM( context );
	UNUSED_PARM( compute );
	UNUSED_PARM( hmdInfo );
	UNUSED_PARM( renderPass );
	UNUSED_PARM( window );
}

static void ksTimeWarpCompute_Destroy( ksGpuContext * context, ksTimeWarpCompute * compute )
{
	UNUSED_PARM( context );
	UNUSED_PARM( compute );
}

static void ksTimeWarpCompute_Render( ksGpuCommandBuffer * commandBuffer, ksTimeWarpCompute * compute,
									ksGpuFramebuffer * framebuffer,
									const ksNanoseconds refreshStartTime, const ksNanoseconds refreshEndTime,
									const ksMatrix4x4f * projectionMatrix, const ksMatrix4x4f * viewMatrix,
									ksGpuTexture * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, ksTimeWarpBarGraphs * bargraphs,
									ksNanoseconds cpuTimes[PROFILE_TIME_MAX], ksNanoseconds gpuTimes[PROFILE_TIME_MAX] )
{
	UNUSED_PARM( commandBuffer );
	UNUSED_PARM( compute );
	UNUSED_PARM( framebuffer );
	UNUSED_PARM( refreshStartTime );
	UNUSED_PARM( refreshEndTime );
	UNUSED_PARM( viewMatrix );
	UNUSED_PARM( eyeTexture );
	UNUSED_PARM( eyeArrayLayer );
	UNUSED_PARM( correctChromaticAberration );
	UNUSED_PARM( bargraphs );
	UNUSED_PARM( cpuTimes );
	UNUSED_PARM( gpuTimes );
}

#endif

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
	ksTimeWarpCompute_Create( &window->context, &timeWarp->compute, hmdInfo, &timeWarp->renderPass, window );
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
#if OPENGL_COMPUTE_ENABLED == 0
	if ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_COMPUTE )
	{
		timeWarp->implementation = (ksTimeWarpImplementation)( ( timeWarp->implementation + 1 ) % TIMEWARP_IMPLEMENTATION_MAX );
	}
#endif
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

/*
================================================================================================================================

ksViewState

static void ksViewState_Init( ksViewState * viewState, const float interpupillaryDistance );
static void ksViewState_HandleInput( ksViewState * viewState, ksGpuWindowInput * input, const ksNanoseconds time );
static void ksViewState_HandleHmd( ksViewState * viewState, const ksNanoseconds time );

================================================================================================================================
*/

typedef struct ksViewState
{
	float						interpupillaryDistance;
	ksVector3f					viewTranslationalVelocity;
	ksVector3f					viewRotationalVelocity;
	ksVector3f					viewTranslation;
	ksVector3f					viewRotation;
	ksMatrix4x4f				displayViewMatrix;						// Display view matrix.
	ksMatrix4x4f				viewMatrix[NUM_EYES];					// Per eye view matrix.
	ksMatrix4x4f				projectionMatrix[NUM_EYES];				// Per eye projection matrix.
	ksMatrix4x4f				viewInverseMatrix[NUM_EYES];			// Per eye inverse view matrix.
	ksMatrix4x4f				projectionInverseMatrix[NUM_EYES];		// Per eye inverse projection matrix.
	ksMatrix4x4f				combinedViewProjectionMatrix;			// Combined matrix containing all views for culling.
} ksViewState;

static void ksViewState_DerivedData( ksViewState * viewState, const ksMatrix4x4f * centerViewMatrix )
{
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		ksMatrix4x4f_Invert( &viewState->viewInverseMatrix[eye], &viewState->viewMatrix[eye] );
		ksMatrix4x4f_Invert( &viewState->projectionInverseMatrix[eye], &viewState->projectionMatrix[eye] );
	}

	// Derive a combined view and projection matrix that encapsulates both views.
	ksMatrix4x4f combinedProjectionMatrix;
	combinedProjectionMatrix = viewState->projectionMatrix[0];
	combinedProjectionMatrix.m[0][0] = viewState->projectionMatrix[0].m[0][0] / ( fabsf( viewState->projectionMatrix[0].m[2][0] ) + 1.0f );
	combinedProjectionMatrix.m[2][0] = 0.0f;
 
	ksMatrix4x4f moveBackMatrix;
	ksMatrix4x4f_CreateTranslation( &moveBackMatrix, 0.0f, 0.0f, -0.5f * viewState->interpupillaryDistance * combinedProjectionMatrix.m[0][0] );

	ksMatrix4x4f combinedViewMatrix;
	ksMatrix4x4f_Multiply( &combinedViewMatrix, &moveBackMatrix, centerViewMatrix );

	ksMatrix4x4f_Multiply( &viewState->combinedViewProjectionMatrix, &combinedProjectionMatrix, &combinedViewMatrix );
}

static void ksViewState_Init( ksViewState * viewState, const float interpupillaryDistance )
{
	viewState->interpupillaryDistance = interpupillaryDistance;
	viewState->viewTranslationalVelocity.x = 0.0f;
	viewState->viewTranslationalVelocity.y = 0.0f;
	viewState->viewTranslationalVelocity.z = 0.0f;
	viewState->viewRotationalVelocity.x = 0.0f;
	viewState->viewRotationalVelocity.y = 0.0f;
	viewState->viewRotationalVelocity.z = 0.0f;
	viewState->viewTranslation.x = 0.0f;
	viewState->viewTranslation.y = 1.5f;
	viewState->viewTranslation.z = 0.25f;
	viewState->viewRotation.x = 0.0f;
	viewState->viewRotation.y = 0.0f;
	viewState->viewRotation.z = 0.0f;

	ksMatrix4x4f_CreateIdentity( &viewState->displayViewMatrix );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		ksMatrix4x4f_CreateIdentity( &viewState->viewMatrix[eye] );
		ksMatrix4x4f_CreateProjectionFov( &viewState->projectionMatrix[eye], 45.0f, 45.0f, 30.0f, 30.0f, DEFAULT_NEAR_Z, INFINITE_FAR_Z );

		ksMatrix4x4f_Invert( &viewState->viewInverseMatrix[eye], &viewState->viewMatrix[eye] );
		ksMatrix4x4f_Invert( &viewState->projectionInverseMatrix[eye], &viewState->projectionMatrix[eye] );
	}

	ksMatrix4x4f centerViewMatrix;
	ksMatrix4x4f_CreateIdentity( &centerViewMatrix );

	ksViewState_DerivedData( viewState, &centerViewMatrix );
}

static void ksViewState_HandleInput( ksViewState * viewState, ksGpuWindowInput * input, const ksNanoseconds time )
{
	static const float TRANSLATION_UNITS_PER_TAP		= 0.005f;
	static const float TRANSLATION_UNITS_DECAY			= 0.0025f;
	static const float ROTATION_DEGREES_PER_TAP			= 0.25f;
	static const float ROTATION_DEGREES_DECAY			= 0.125f;
	static const ksVector3f minTranslationalVelocity	= { -0.05f, -0.05f, -0.05f};
	static const ksVector3f maxTranslationalVelocity	= { 0.05f, 0.05f, 0.05f };
	static const ksVector3f minRotationalVelocity		= { -2.0f, -2.0f, -2.0f };
	static const ksVector3f maxRotationalVelocity		= { 2.0f, 2.0f, 2.0f };

	GetHmdViewMatrixForTime( &viewState->displayViewMatrix, time );

	ksVector3f translationDelta = { 0.0f, 0.0f, 0.0f };
	ksVector3f rotationDelta = { 0.0f, 0.0f, 0.0f };

	// NOTE: don't consume but only 'check' the keyboard state in case the input is maintained on another thread.
	if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_SHIFT_LEFT ) )
	{
		if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_UP ) )			{ rotationDelta.x -= ROTATION_DEGREES_PER_TAP; }
		else if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_DOWN ) )		{ rotationDelta.x += ROTATION_DEGREES_PER_TAP; }
		else if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_LEFT ) )		{ rotationDelta.y += ROTATION_DEGREES_PER_TAP; }
		else if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_RIGHT ) )	{ rotationDelta.y -= ROTATION_DEGREES_PER_TAP; }
	}
	else if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CTRL_LEFT ) )
	{
		if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_UP ) )			{ translationDelta.y += TRANSLATION_UNITS_PER_TAP; }
		else if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_DOWN ) )		{ translationDelta.y -= TRANSLATION_UNITS_PER_TAP; }
		else if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_LEFT ) )		{ translationDelta.x -= TRANSLATION_UNITS_PER_TAP; }
		else if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_RIGHT ) )	{ translationDelta.x += TRANSLATION_UNITS_PER_TAP; }
	}
	else
	{
		if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_UP ) )			{ translationDelta.z -= TRANSLATION_UNITS_PER_TAP; }
		else if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_DOWN ) )		{ translationDelta.z += TRANSLATION_UNITS_PER_TAP; }
		else if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_LEFT ) )		{ rotationDelta.y += ROTATION_DEGREES_PER_TAP; }
		else if ( ksGpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_RIGHT ) )	{ rotationDelta.y -= ROTATION_DEGREES_PER_TAP; }
	}

	ksVector3f_Decay( &viewState->viewTranslationalVelocity, &viewState->viewTranslationalVelocity, TRANSLATION_UNITS_DECAY );
	ksVector3f_Decay( &viewState->viewRotationalVelocity, &viewState->viewRotationalVelocity, ROTATION_DEGREES_DECAY );

	ksVector3f_Add( &viewState->viewTranslationalVelocity, &viewState->viewTranslationalVelocity, &translationDelta );
	ksVector3f_Add( &viewState->viewRotationalVelocity, &viewState->viewRotationalVelocity, &rotationDelta );

	ksVector3f_Max( &viewState->viewTranslationalVelocity, &viewState->viewTranslationalVelocity, &minTranslationalVelocity );
	ksVector3f_Min( &viewState->viewTranslationalVelocity, &viewState->viewTranslationalVelocity, &maxTranslationalVelocity );

	ksVector3f_Max( &viewState->viewRotationalVelocity, &viewState->viewRotationalVelocity, &minRotationalVelocity );
	ksVector3f_Min( &viewState->viewRotationalVelocity, &viewState->viewRotationalVelocity, &maxRotationalVelocity );

	ksVector3f_Add( &viewState->viewRotation, &viewState->viewRotation, &viewState->viewRotationalVelocity );

	ksMatrix4x4f yawRotation;
	ksMatrix4x4f_CreateRotation( &yawRotation, 0.0f, viewState->viewRotation.y, 0.0f );

	ksVector3f rotatedTranslationalVelocity;
	ksMatrix4x4f_TransformVector3f( &rotatedTranslationalVelocity, &yawRotation, &viewState->viewTranslationalVelocity );

	ksVector3f_Add( &viewState->viewTranslation, &viewState->viewTranslation, &rotatedTranslationalVelocity );

	ksMatrix4x4f viewRotation;
	ksMatrix4x4f_CreateRotation( &viewRotation, viewState->viewRotation.x, viewState->viewRotation.y, viewState->viewRotation.z );

	ksMatrix4x4f viewRotationTranspose;
	ksMatrix4x4f_Transpose( &viewRotationTranspose, &viewRotation );

	ksMatrix4x4f viewTranslation;
	ksMatrix4x4f_CreateTranslation( &viewTranslation, -viewState->viewTranslation.x, -viewState->viewTranslation.y, -viewState->viewTranslation.z );

	ksMatrix4x4f inputViewMatrix;
	ksMatrix4x4f_Multiply( &inputViewMatrix, &viewRotationTranspose, &viewTranslation );

	ksMatrix4x4f centerViewMatrix;
	ksMatrix4x4f_Multiply( &centerViewMatrix, &viewState->displayViewMatrix, &inputViewMatrix );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		ksMatrix4x4f eyeOffsetMatrix;
		ksMatrix4x4f_CreateTranslation( &eyeOffsetMatrix, ( eye ? -0.5f : 0.5f ) * viewState->interpupillaryDistance, 0.0f, 0.0f );

		ksMatrix4x4f_Multiply( &viewState->viewMatrix[eye], &eyeOffsetMatrix, &centerViewMatrix );
		ksMatrix4x4f_CreateProjectionFov( &viewState->projectionMatrix[eye], 45.0f, 45.0f, 30.0f, 30.0f, DEFAULT_NEAR_Z, INFINITE_FAR_Z );
	}

	ksViewState_DerivedData( viewState, &centerViewMatrix );
}

static void ksViewState_HandleHmd( ksViewState * viewState, const ksNanoseconds time )
{
	GetHmdViewMatrixForTime( &viewState->displayViewMatrix, time );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		ksMatrix4x4f eyeOffsetMatrix;
		ksMatrix4x4f_CreateTranslation( &eyeOffsetMatrix, ( eye ? -0.5f : 0.5f ) * viewState->interpupillaryDistance, 0.0f, 0.0f );

		ksMatrix4x4f_Multiply( &viewState->viewMatrix[eye], &eyeOffsetMatrix, &viewState->displayViewMatrix );
		ksMatrix4x4f_CreateProjectionFov( &viewState->projectionMatrix[eye], 45.0f, 45.0f, 36.0f, 36.0f, DEFAULT_NEAR_Z, INFINITE_FAR_Z );
	}

	ksViewState_DerivedData( viewState, &viewState->displayViewMatrix );
}

/*
================================================================================================================================

Scene settings.

ksSceneSettings

static void ksSceneSettings_ToggleSimulationPaused( ksSceneSettings * settings );
static void ksSceneSettings_ToggleMultiView( ksSceneSettings * settings );
static void ksSceneSettings_SetSimulationPaused( ksSceneSettings * settings, const bool set );
static void ksSceneSettings_SetMultiView( ksSceneSettings * settings, const bool set );
static bool ksSceneSettings_GetSimulationPaused( ksSceneSettings * settings );
static bool ksSceneSettings_GetMultiView( ksSceneSettings * settings );

static void ksSceneSettings_CycleDisplayResolutionLevel( ksSceneSettings * settings );
static void ksSceneSettings_CycleEyeImageResolutionLevel( ksSceneSettings * settings );
static void ksSceneSettings_CycleEyeImageSamplesLevel( ksSceneSettings * settings );

static void ksSceneSettings_CycleDrawCallLevel( ksSceneSettings * settings );
static void ksSceneSettings_CycleTriangleLevel( ksSceneSettings * settings );
static void ksSceneSettings_CycleFragmentLevel( ksSceneSettings * settings );

static void ksSceneSettings_SetDisplayResolutionLevel( ksSceneSettings * settings, const int level );
static void ksSceneSettings_SetEyeImageResolutionLevel( ksSceneSettings * settings, const int level );
static void ksSceneSettings_SetEyeImageSamplesLevel( ksSceneSettings * settings, const int level );

static void ksSceneSettings_SetDrawCallLevel( ksSceneSettings * settings, const int level );
static void ksSceneSettings_SetTriangleLevel( ksSceneSettings * settings, const int level );
static void ksSceneSettings_SetFragmentLevel( ksSceneSettings * settings, const int level );

static int ksSceneSettings_GetDisplayResolutionLevel( const ksSceneSettings * settings );
static int ksSceneSettings_GetEyeImageResolutionLevel( const ksSceneSettings * settings );
static int ksSceneSettings_GetEyeImageSamplesLevel( const ksSceneSettings * settings );

static int ksSceneSettings_GetDrawCallLevel( const ksSceneSettings * settings );
static int ksSceneSettings_GetTriangleLevel( const ksSceneSettings * settings );
static int ksSceneSettings_GetFragmentLevel( const ksSceneSettings * settings );

================================================================================================================================
*/

#define MAX_DISPLAY_RESOLUTION_LEVELS		4
#define MAX_EYE_IMAGE_RESOLUTION_LEVELS		4
#define MAX_EYE_IMAGE_SAMPLES_LEVELS		4

#define MAX_SCENE_DRAWCALL_LEVELS			4
#define MAX_SCENE_TRIANGLE_LEVELS			4
#define MAX_SCENE_FRAGMENT_LEVELS			4

static const int displayResolutionTable[] =
{
	1920, 1080,
	2560, 1440,
	3840, 2160,
	7680, 4320
};

static const int eyeResolutionTable[] =
{
	1024,
	1536,
	2048,
	4096
};

static const ksGpuSampleCount eyeSampleCountTable[] =
{
	KS_GPU_SAMPLE_COUNT_1,
	KS_GPU_SAMPLE_COUNT_2,
	KS_GPU_SAMPLE_COUNT_4,
	KS_GPU_SAMPLE_COUNT_8
};

typedef struct
{
	const char *	glTF;
	bool			simulationPaused;
	bool			useMultiView;
	int				displayResolutionLevel;
	int				eyeImageResolutionLevel;
	int				eyeImageSamplesLevel;
	int				drawCallLevel;
	int				triangleLevel;
	int				fragmentLevel;
	int				maxDisplayResolutionLevels;
	int				maxEyeImageResolutionLevels;
	int				maxEyeImageSamplesLevels;
} ksSceneSettings;

static void ksSceneSettings_Init( ksGpuContext * context, ksSceneSettings * settings )
{
	const int maxEyeImageSamplesLevels = IntegerLog2( glGetInteger( GL_MAX_SAMPLES ) + 1 );

	settings->glTF = NULL;
	settings->simulationPaused = false;
	settings->useMultiView = false;
	settings->displayResolutionLevel = 0;
	settings->eyeImageResolutionLevel = 0;
	settings->eyeImageSamplesLevel = 0;
	settings->drawCallLevel = 0;
	settings->triangleLevel = 0;
	settings->fragmentLevel = 0;
	settings->maxDisplayResolutionLevels =	( !ksGpuWindow_SupportedResolution( displayResolutionTable[1 * 2 + 0], displayResolutionTable[1 * 2 + 1] ) ? 1 :
											( !ksGpuWindow_SupportedResolution( displayResolutionTable[2 * 2 + 0], displayResolutionTable[2 * 2 + 1] ) ? 2 :
											( !ksGpuWindow_SupportedResolution( displayResolutionTable[3 * 2 + 0], displayResolutionTable[3 * 2 + 1] ) ? 3 : 4 ) ) );
	settings->maxEyeImageResolutionLevels = MAX_EYE_IMAGE_RESOLUTION_LEVELS;
	settings->maxEyeImageSamplesLevels = ( maxEyeImageSamplesLevels < MAX_EYE_IMAGE_SAMPLES_LEVELS ) ? maxEyeImageSamplesLevels : MAX_EYE_IMAGE_SAMPLES_LEVELS;

	UNUSED_PARM( context );
}

static void CycleLevel( int * x, const int max ) { (*x) = ( (*x) + 1 ) % max; }

static void ksSceneSettings_SetGltf( ksSceneSettings * settings, const char * jsonName ) { settings->glTF = jsonName; }

static void ksSceneSettings_ToggleSimulationPaused( ksSceneSettings * settings ) { settings->simulationPaused = !settings->simulationPaused; }
static void ksSceneSettings_ToggleMultiView( ksSceneSettings * settings ) { settings->useMultiView = !settings->useMultiView; }

static void ksSceneSettings_SetSimulationPaused( ksSceneSettings * settings, const bool set ) { settings->simulationPaused = set; }
static void ksSceneSettings_SetMultiView( ksSceneSettings * settings, const bool set ) { settings->useMultiView = set; }

static bool ksSceneSettings_GetSimulationPaused( ksSceneSettings * settings ) { return settings->simulationPaused; }
static bool ksSceneSettings_GetMultiView( ksSceneSettings * settings ) { return settings->useMultiView; }

static void ksSceneSettings_CycleDisplayResolutionLevel( ksSceneSettings * settings ) { CycleLevel( &settings->displayResolutionLevel, settings->maxDisplayResolutionLevels ); }
static void ksSceneSettings_CycleEyeImageResolutionLevel( ksSceneSettings * settings ) { CycleLevel( &settings->eyeImageResolutionLevel, settings->maxEyeImageResolutionLevels ); }
static void ksSceneSettings_CycleEyeImageSamplesLevel( ksSceneSettings * settings ) { CycleLevel( &settings->eyeImageSamplesLevel, settings->maxEyeImageSamplesLevels ); }

static void ksSceneSettings_CycleDrawCallLevel( ksSceneSettings * settings ) { CycleLevel( &settings->drawCallLevel, MAX_SCENE_DRAWCALL_LEVELS ); }
static void ksSceneSettings_CycleTriangleLevel( ksSceneSettings * settings ) { CycleLevel( &settings->triangleLevel, MAX_SCENE_TRIANGLE_LEVELS ); }
static void ksSceneSettings_CycleFragmentLevel( ksSceneSettings * settings ) { CycleLevel( &settings->fragmentLevel, MAX_SCENE_FRAGMENT_LEVELS ); }

static void ksSceneSettings_SetDisplayResolutionLevel( ksSceneSettings * settings, const int level ) { settings->displayResolutionLevel = ( level < settings->maxDisplayResolutionLevels ) ? level : settings->maxDisplayResolutionLevels; }
static void ksSceneSettings_SetEyeImageResolutionLevel( ksSceneSettings * settings, const int level ) { settings->eyeImageResolutionLevel = ( level < settings->maxEyeImageResolutionLevels ) ? level : settings->maxEyeImageResolutionLevels; }
static void ksSceneSettings_SetEyeImageSamplesLevel( ksSceneSettings * settings, const int level ) { settings->eyeImageSamplesLevel = ( level < settings->maxEyeImageSamplesLevels ) ? level : settings->maxEyeImageSamplesLevels; }

static void ksSceneSettings_SetDrawCallLevel( ksSceneSettings * settings, const int level ) { settings->drawCallLevel = level; }
static void ksSceneSettings_SetTriangleLevel( ksSceneSettings * settings, const int level ) { settings->triangleLevel = level; }
static void ksSceneSettings_SetFragmentLevel( ksSceneSettings * settings, const int level ) { settings->fragmentLevel = level; }

static int ksSceneSettings_GetDisplayResolutionLevel( const ksSceneSettings * settings ) { return settings->eyeImageResolutionLevel; }
static int ksSceneSettings_GetEyeImageResolutionLevel( const ksSceneSettings * settings ) { return settings->eyeImageResolutionLevel; }
static int ksSceneSettings_GetEyeImageSamplesLevel( const ksSceneSettings * settings ) { return settings->eyeImageSamplesLevel; }

static int ksSceneSettings_GetDrawCallLevel( const ksSceneSettings * settings ) { return settings->drawCallLevel; }
static int ksSceneSettings_GetTriangleLevel( const ksSceneSettings * settings ) { return settings->triangleLevel; }
static int ksSceneSettings_GetFragmentLevel( const ksSceneSettings * settings ) { return settings->fragmentLevel; }

/*
================================================================================================================================

Performance scene rendering.

ksPerfScene

static void ksPerfScene_Create( ksGpuContext * context, ksPerfScene * scene, ksSceneSettings * settings, ksGpuRenderPass * renderPass );
static void ksPerfScene_Destroy( ksGpuContext * context, ksPerfScene * scene );

static void ksPerfScene_Simulate( ksPerfScene * scene, ksViewState * viewState, const ksNanoseconds time );
static void ksPerfScene_UpdateBuffers( ksGpuCommandBuffer * commandBuffer, ksPerfScene * scene, const ksViewState * viewState, const int eye );
static void ksPerfScene_Render( ksGpuCommandBuffer * commandBuffer, ksPerfScene * scene, const ksViewState * viewState );

================================================================================================================================
*/

typedef struct
{
	// assets
	ksGpuGeometry			geometry[MAX_SCENE_TRIANGLE_LEVELS];
	ksGpuGraphicsProgram	program[MAX_SCENE_FRAGMENT_LEVELS];
	ksGpuGraphicsPipeline	pipelines[MAX_SCENE_TRIANGLE_LEVELS][MAX_SCENE_FRAGMENT_LEVELS];
	ksGpuBuffer				sceneMatrices;
	ksGpuTexture			diffuseTexture;
	ksGpuTexture			specularTexture;
	ksGpuTexture			normalTexture;
	ksSceneSettings			settings;
	ksSceneSettings *		newSettings;
	// simulation state
	float					bigRotationX;
	float					bigRotationY;
	float					smallRotationX;
	float					smallRotationY;
	ksMatrix4x4f *			modelMatrix;
} ksPerfScene;

enum
{
	PROGRAM_UNIFORM_MODEL_MATRIX,
	PROGRAM_UNIFORM_SCENE_MATRICES,
	PROGRAM_TEXTURE_0,
	PROGRAM_TEXTURE_1,
	PROGRAM_TEXTURE_2
};

static ksGpuProgramParm flatShadedProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_UNIFORM_MODEL_MATRIX,		"ModelMatrix",		0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_UNIFORM_SCENE_MATRICES,		"SceneMatrices",	0 }
};

static const char flatShadedVertexProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform mat4 ModelMatrix;\n"
	"uniform SceneMatrices\n"
	"{\n"
	"	mat4 ViewMatrix;\n"
	"	mat4 ProjectionMatrix;\n"
	"} ub;\n"
	"in vec3 vertexPosition;\n"
	"in vec3 vertexNormal;\n"
	"out vec3 fragmentEyeDir;\n"
	"out vec3 fragmentNormal;\n"
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
	"	vec4 vertexWorldPos = ModelMatrix * vec4( vertexPosition, 1.0 );\n"
	"	vec3 eyeWorldPos = transposeMultiply3x3( ub.ViewMatrix, -vec3( ub.ViewMatrix[3] ) );\n"
	"	gl_Position = ub.ProjectionMatrix * ( ub.ViewMatrix * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( ModelMatrix, vertexNormal );\n"
	"}\n";

static const char flatShadedMultiViewVertexProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"#define NUM_VIEWS 2\n"
	"#define VIEW_ID gl_ViewID_OVR\n"
	"#extension GL_OVR_multiview2 : require\n"
	"layout( num_views = NUM_VIEWS ) in;\n"
	"\n"
	"uniform mat4 ModelMatrix;\n"
	"uniform SceneMatrices\n"
	"{\n"
	"	mat4 ViewMatrix[NUM_VIEWS];\n"
	"	mat4 ProjectionMatrix[NUM_VIEWS];\n"
	"} ub;\n"
	"in vec3 vertexPosition;\n"
	"in vec3 vertexNormal;\n"
	"out vec3 fragmentEyeDir;\n"
	"out vec3 fragmentNormal;\n"
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
	"	vec4 vertexWorldPos = ModelMatrix * vec4( vertexPosition, 1.0 );\n"
	"	vec3 eyeWorldPos = transposeMultiply3x3( ub.ViewMatrix[VIEW_ID], -vec3( ub.ViewMatrix[VIEW_ID][3] ) );\n"
	"	gl_Position = ub.ProjectionMatrix[VIEW_ID] * ( ub.ViewMatrix[VIEW_ID] * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( ModelMatrix, vertexNormal );\n"
	"}\n";

static const char flatShadedFragmentProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"in lowp vec3 fragmentEyeDir;\n"
	"in lowp vec3 fragmentNormal;\n"
	"out lowp vec4 outColor;\n"
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

static ksGpuProgramParm normalMappedProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,		KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_UNIFORM_MODEL_MATRIX,		"ModelMatrix",		0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,		KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_UNIFORM_SCENE_MATRICES,		"SceneMatrices",	0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT,	KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_TEXTURE_0,					"Texture0",			0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT,	KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_TEXTURE_1,					"Texture1",			1 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT,	KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_TEXTURE_2,					"Texture2",			2 }
};

static const char normalMappedVertexProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform mat4 ModelMatrix;\n"
	"uniform SceneMatrices\n"
	"{\n"
	"	mat4 ViewMatrix;\n"
	"	mat4 ProjectionMatrix;\n"
	"} ub;\n"
	"in vec3 vertexPosition;\n"
	"in vec3 vertexNormal;\n"
	"in vec3 vertexTangent;\n"
	"in vec3 vertexBinormal;\n"
	"in vec2 vertexUv0;\n"
	"out vec3 fragmentEyeDir;\n"
	"out vec3 fragmentNormal;\n"
	"out vec3 fragmentTangent;\n"
	"out vec3 fragmentBinormal;\n"
	"out vec2 fragmentUv0;\n"
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
	"	vec4 vertexWorldPos = ModelMatrix * vec4( vertexPosition, 1.0 );\n"
	"	vec3 eyeWorldPos = transposeMultiply3x3( ub.ViewMatrix, -vec3( ub.ViewMatrix[3] ) );\n"
	"	gl_Position = ub.ProjectionMatrix * ( ub.ViewMatrix * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( ModelMatrix, vertexNormal );\n"
	"	fragmentTangent = multiply3x3( ModelMatrix, vertexTangent );\n"
	"	fragmentBinormal = multiply3x3( ModelMatrix, vertexBinormal );\n"
	"	fragmentUv0 = vertexUv0;\n"
	"}\n";

static const char normalMappedMultiViewVertexProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"#define NUM_VIEWS 2\n"
	"#define VIEW_ID gl_ViewID_OVR\n"
	"#extension GL_OVR_multiview2 : require\n"
	"layout( num_views = NUM_VIEWS ) in;\n"
	"\n"
	"uniform mat4 ModelMatrix;\n"
	"uniform SceneMatrices\n"
	"{\n"
	"	mat4 ViewMatrix[NUM_VIEWS];\n"
	"	mat4 ProjectionMatrix[NUM_VIEWS];\n"
	"} ub;\n"
	"in vec3 vertexPosition;\n"
	"in vec3 vertexNormal;\n"
	"in vec3 vertexTangent;\n"
	"in vec3 vertexBinormal;\n"
	"in vec2 vertexUv0;\n"
	"out vec3 fragmentEyeDir;\n"
	"out vec3 fragmentNormal;\n"
	"out vec3 fragmentTangent;\n"
	"out vec3 fragmentBinormal;\n"
	"out vec2 fragmentUv0;\n"
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
	"	vec4 vertexWorldPos = ModelMatrix * vec4( vertexPosition, 1.0 );\n"
	"	vec3 eyeWorldPos = transposeMultiply3x3( ub.ViewMatrix[VIEW_ID], -vec3( ub.ViewMatrix[VIEW_ID][3] ) );\n"
	"	gl_Position = ub.ProjectionMatrix[VIEW_ID] * ( ub.ViewMatrix[VIEW_ID] * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( ModelMatrix, vertexNormal );\n"
	"	fragmentTangent = multiply3x3( ModelMatrix, vertexTangent );\n"
	"	fragmentBinormal = multiply3x3( ModelMatrix, vertexBinormal );\n"
	"	fragmentUv0 = vertexUv0;\n"
	"}\n";

static const char normalMapped100LightsFragmentProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform sampler2D Texture0;\n"
	"uniform sampler2D Texture1;\n"
	"uniform sampler2D Texture2;\n"
	"in lowp vec3 fragmentEyeDir;\n"
	"in lowp vec3 fragmentNormal;\n"
	"in lowp vec3 fragmentTangent;\n"
	"in lowp vec3 fragmentBinormal;\n"
	"in lowp vec2 fragmentUv0;\n"
	"out lowp vec4 outColor;\n"
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

static const char normalMapped1000LightsFragmentProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform sampler2D Texture0;\n"
	"uniform sampler2D Texture1;\n"
	"uniform sampler2D Texture2;\n"
	"in lowp vec3 fragmentEyeDir;\n"
	"in lowp vec3 fragmentNormal;\n"
	"in lowp vec3 fragmentTangent;\n"
	"in lowp vec3 fragmentBinormal;\n"
	"in lowp vec2 fragmentUv0;\n"
	"out lowp vec4 outColor;\n"
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

static const char normalMapped2000LightsFragmentProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform sampler2D Texture0;\n"
	"uniform sampler2D Texture1;\n"
	"uniform sampler2D Texture2;\n"
	"in lowp vec3 fragmentEyeDir;\n"
	"in lowp vec3 fragmentNormal;\n"
	"in lowp vec3 fragmentTangent;\n"
	"in lowp vec3 fragmentBinormal;\n"
	"in lowp vec2 fragmentUv0;\n"
	"out lowp vec4 outColor;\n"
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

static void ksPerfScene_Create( ksGpuContext * context, ksPerfScene * scene, ksSceneSettings * settings, ksGpuRenderPass * renderPass )
{
	memset( scene, 0, sizeof( ksPerfScene ) );

	ksGpuGeometry_CreateCube( context, &scene->geometry[0], 0.0f, 0.5f );			// 12 triangles
	ksGpuGeometry_CreateTorus( context, &scene->geometry[1], 8, 0.0f, 1.0f );		// 128 triangles
	ksGpuGeometry_CreateTorus( context, &scene->geometry[2], 16, 0.0f, 1.0f );	// 512 triangles
	ksGpuGeometry_CreateTorus( context, &scene->geometry[3], 32, 0.0f, 1.0f );	// 2048 triangles

	ksGpuGraphicsProgram_Create( context, &scene->program[0],
								settings->useMultiView ? PROGRAM( flatShadedMultiViewVertexProgram ) : PROGRAM( flatShadedVertexProgram ),
								settings->useMultiView ? sizeof( PROGRAM( flatShadedMultiViewVertexProgram ) ) : sizeof( PROGRAM( flatShadedVertexProgram ) ),
								PROGRAM( flatShadedFragmentProgram ),
								sizeof( PROGRAM( flatShadedFragmentProgram ) ),
								flatShadedProgramParms, ARRAY_SIZE( flatShadedProgramParms ),
								scene->geometry[0].layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL );
	ksGpuGraphicsProgram_Create( context, &scene->program[1],
								settings->useMultiView ? PROGRAM( normalMappedMultiViewVertexProgram ) : PROGRAM( normalMappedVertexProgram ),
								settings->useMultiView ? sizeof( PROGRAM( normalMappedMultiViewVertexProgram ) ) : sizeof( PROGRAM( normalMappedVertexProgram ) ),
								PROGRAM( normalMapped100LightsFragmentProgram ),
								sizeof( PROGRAM( normalMapped100LightsFragmentProgram ) ),
								normalMappedProgramParms, ARRAY_SIZE( normalMappedProgramParms ),
								scene->geometry[0].layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL |
								VERTEX_ATTRIBUTE_FLAG_TANGENT | VERTEX_ATTRIBUTE_FLAG_BINORMAL |
								VERTEX_ATTRIBUTE_FLAG_UV0 );
	ksGpuGraphicsProgram_Create( context, &scene->program[2],
								settings->useMultiView ? PROGRAM( normalMappedMultiViewVertexProgram ) : PROGRAM( normalMappedVertexProgram ),
								settings->useMultiView ? sizeof( PROGRAM( normalMappedMultiViewVertexProgram ) ) : sizeof( PROGRAM( normalMappedVertexProgram ) ),
								PROGRAM( normalMapped1000LightsFragmentProgram ),
								sizeof( PROGRAM( normalMapped1000LightsFragmentProgram ) ),
								normalMappedProgramParms, ARRAY_SIZE( normalMappedProgramParms ),
								scene->geometry[0].layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL |
								VERTEX_ATTRIBUTE_FLAG_TANGENT | VERTEX_ATTRIBUTE_FLAG_BINORMAL |
								VERTEX_ATTRIBUTE_FLAG_UV0 );
	ksGpuGraphicsProgram_Create( context, &scene->program[3],
								settings->useMultiView ? PROGRAM( normalMappedMultiViewVertexProgram ) : PROGRAM( normalMappedVertexProgram ),
								settings->useMultiView ? sizeof( PROGRAM( normalMappedMultiViewVertexProgram ) ) : sizeof( PROGRAM( normalMappedVertexProgram ) ),
								PROGRAM( normalMapped2000LightsFragmentProgram ),
								sizeof( PROGRAM( normalMapped2000LightsFragmentProgram ) ),
								normalMappedProgramParms, ARRAY_SIZE( normalMappedProgramParms ),
								scene->geometry[0].layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL |
								VERTEX_ATTRIBUTE_FLAG_TANGENT | VERTEX_ATTRIBUTE_FLAG_BINORMAL |
								VERTEX_ATTRIBUTE_FLAG_UV0 );

	for ( int i = 0; i < MAX_SCENE_TRIANGLE_LEVELS; i++ )
	{
		for ( int j = 0; j < MAX_SCENE_FRAGMENT_LEVELS; j++ )
		{
			ksGpuGraphicsPipelineParms pipelineParms;
			ksGpuGraphicsPipelineParms_Init( &pipelineParms );

			pipelineParms.renderPass = renderPass;
			pipelineParms.program = &scene->program[j];
			pipelineParms.geometry = &scene->geometry[i];

			ksGpuGraphicsPipeline_Create( context, &scene->pipelines[i][j], &pipelineParms );
		}
	}

	ksGpuBuffer_Create( context, &scene->sceneMatrices, KS_GPU_BUFFER_TYPE_UNIFORM, ( settings->useMultiView ? 4 : 2 ) * sizeof( ksMatrix4x4f ), NULL, false );

	ksGpuTexture_CreateDefault( context, &scene->diffuseTexture, KS_GPU_TEXTURE_DEFAULT_CHECKERBOARD, 256, 256, 0, 0, 1, true, false );
	ksGpuTexture_CreateDefault( context, &scene->specularTexture, KS_GPU_TEXTURE_DEFAULT_CHECKERBOARD, 256, 256, 0, 0, 1, true, false );
	ksGpuTexture_CreateDefault( context, &scene->normalTexture, KS_GPU_TEXTURE_DEFAULT_PYRAMIDS, 256, 256, 0, 0, 1, true, false );

	scene->settings = *settings;
	scene->newSettings = settings;

	const int maxDimension = 2 * ( 1 << ( MAX_SCENE_DRAWCALL_LEVELS - 1 ) );

	scene->bigRotationX = 0.0f;
	scene->bigRotationY = 0.0f;
	scene->smallRotationX = 0.0f;
	scene->smallRotationY = 0.0f;

	scene->modelMatrix = (ksMatrix4x4f *) AllocAlignedMemory( maxDimension * maxDimension * maxDimension * sizeof( ksMatrix4x4f ), sizeof( ksMatrix4x4f ) );
}

static void ksPerfScene_Destroy( ksGpuContext * context, ksPerfScene * scene )
{
	ksGpuContext_WaitIdle( context );

	for ( int i = 0; i < MAX_SCENE_TRIANGLE_LEVELS; i++ )
	{
		for ( int j = 0; j < MAX_SCENE_FRAGMENT_LEVELS; j++ )
		{
			ksGpuGraphicsPipeline_Destroy( context, &scene->pipelines[i][j] );
		}
	}

	for ( int i = 0; i < MAX_SCENE_TRIANGLE_LEVELS; i++ )
	{
		ksGpuGeometry_Destroy( context, &scene->geometry[i] );
	}

	for ( int i = 0; i < MAX_SCENE_FRAGMENT_LEVELS; i++ )
	{
		ksGpuGraphicsProgram_Destroy( context, &scene->program[i] );
	}

	ksGpuBuffer_Destroy( context, &scene->sceneMatrices );

	ksGpuTexture_Destroy( context, &scene->diffuseTexture );
	ksGpuTexture_Destroy( context, &scene->specularTexture );
	ksGpuTexture_Destroy( context, &scene->normalTexture );

	FreeAlignedMemory( scene->modelMatrix );
	scene->modelMatrix = NULL;
}

static void ksPerfScene_Simulate( ksPerfScene * scene, ksViewState * viewState, const ksNanoseconds time )
{
	// Must recreate the scene if multi-view is enabled/disabled.
	assert( scene->settings.useMultiView == scene->newSettings->useMultiView );
	scene->settings = *scene->newSettings;

	ksViewState_HandleHmd( viewState, time );

	if ( !scene->settings.simulationPaused )
	{
		// FIXME: use double?
		const float offset = time * ( MATH_PI / 1000.0f / 1000.0f / 1000.0f );
		scene->bigRotationX = 20.0f * offset;
		scene->bigRotationY = 10.0f * offset;
		scene->smallRotationX = -60.0f * offset;
		scene->smallRotationY = -40.0f * offset;
	}
}

static void ksPerfScene_UpdateBuffers( ksGpuCommandBuffer * commandBuffer, ksPerfScene * scene, const ksViewState * viewState, const int eye )
{
	ksMatrix4x4f * sceneMatrices = NULL;
	ksGpuBuffer * sceneMatricesBuffer = ksGpuCommandBuffer_MapBuffer( commandBuffer, &scene->sceneMatrices, (void **)&sceneMatrices );
	const int count = ( eye == 2 ) ? 2 : 1;
	memcpy( sceneMatrices + 0 * count, &viewState->viewMatrix[eye], count * sizeof( ksMatrix4x4f ) );
	memcpy( sceneMatrices + 1 * count, &viewState->projectionMatrix[eye], count * sizeof( ksMatrix4x4f ) );
	ksGpuCommandBuffer_UnmapBuffer( commandBuffer, &scene->sceneMatrices, sceneMatricesBuffer, KS_GPU_BUFFER_UNMAP_TYPE_COPY_BACK );
}

static void ksPerfScene_Render( ksGpuCommandBuffer * commandBuffer, ksPerfScene * scene, const ksViewState * viewState )
{
	UNUSED_PARM( viewState );

	const int dimension = 2 * ( 1 << scene->settings.drawCallLevel );
	const float cubeOffset = ( dimension - 1.0f ) * 0.5f;
	const float cubeScale = 2.0f;

	ksMatrix4x4f bigRotationMatrix;
	ksMatrix4x4f_CreateRotation( &bigRotationMatrix, scene->bigRotationX, scene->bigRotationY, 0.0f );

	ksMatrix4x4f bigTranslationMatrix;
	ksMatrix4x4f_CreateTranslation( &bigTranslationMatrix, 0.0f, 0.0f, - 2.5f * dimension );

	ksMatrix4x4f bigTransformMatrix;
	ksMatrix4x4f_Multiply( &bigTransformMatrix, &bigTranslationMatrix, &bigRotationMatrix );

	ksMatrix4x4f smallRotationMatrix;
	ksMatrix4x4f_CreateRotation( &smallRotationMatrix, scene->smallRotationX, scene->smallRotationY, 0.0f );

	ksGpuGraphicsCommand command;
	ksGpuGraphicsCommand_Init( &command );
	ksGpuGraphicsCommand_SetPipeline( &command, &scene->pipelines[scene->settings.triangleLevel][scene->settings.fragmentLevel] );
	ksGpuGraphicsCommand_SetParmBufferUniform( &command, PROGRAM_UNIFORM_SCENE_MATRICES, &scene->sceneMatrices );
	ksGpuGraphicsCommand_SetParmTextureSampled( &command, PROGRAM_TEXTURE_0, ( scene->settings.fragmentLevel >= 1 ) ? &scene->diffuseTexture : NULL );
	ksGpuGraphicsCommand_SetParmTextureSampled( &command, PROGRAM_TEXTURE_1, ( scene->settings.fragmentLevel >= 1 ) ? &scene->specularTexture : NULL );
	ksGpuGraphicsCommand_SetParmTextureSampled( &command, PROGRAM_TEXTURE_2, ( scene->settings.fragmentLevel >= 1 ) ? &scene->normalTexture : NULL );

	for ( int x = 0; x < dimension; x++ )
	{
		for ( int y = 0; y < dimension; y++ )
		{
			for ( int z = 0; z < dimension; z++ )
			{
				ksMatrix4x4f smallTranslationMatrix;
				ksMatrix4x4f_CreateTranslation( &smallTranslationMatrix, cubeScale * ( x - cubeOffset ), cubeScale * ( y - cubeOffset ), cubeScale * ( z - cubeOffset ) );

				ksMatrix4x4f smallTransformMatrix;
				ksMatrix4x4f_Multiply( &smallTransformMatrix, &smallTranslationMatrix, &smallRotationMatrix );

				ksMatrix4x4f * modelMatrix = &scene->modelMatrix[( x * dimension + y ) * dimension + z];
				ksMatrix4x4f_Multiply( modelMatrix, &bigTransformMatrix, &smallTransformMatrix );

				ksGpuGraphicsCommand_SetParmFloatMatrix4x4( &command, PROGRAM_UNIFORM_MODEL_MATRIX, modelMatrix );

				ksGpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
			}
		}
	}
}

/*
================================================================================================================================

glTF scene rendering.

This implementation supports the following extensions:

 - KHR_binary_glTF
 - KHR_skin_culling
 - KHR_image_versions
 - KHR_technique_uniform_stages
 - KHR_technique_uniform_binding_opengl
 - KHR_technique_uniform_binding_vulkan
 - KHR_technique_uniform_binding_d3d
 - KHR_technique_uniform_binding_metal
 - KHR_glsl_shader_versions
 - KHR_spirv_shader_versions
 - KHR_hlsl_shader_versions
 - KHR_metalsl_shader_versions
 - KHR_glsl_joint_buffer
 - KHR_glsl_view_projection_buffer
 - KHR_glsl_multi_view
 - KHR_glsl_layout_opengl
 - KHR_glsl_layout_vulkan

This implementation only supports KTX images.

ksGltfScene

static bool ksGltfScene_CreateFromFile( ksGpuContext * context, ksGltfScene * scene, ksSceneSettings * settings, ksGpuRenderPass * renderPass );
static void ksGltfScene_Destroy( ksGpuContext * context, ksGltfScene * scene );

static void ksGltfScene_SetSubScene( ksGltfScene * scene, const char * subSceneName );
static void ksGltfScene_SetSubTreeVisible( ksGltfScene * scene, const char * subTreeName, const bool visible );
static void ksGltfScene_SetAnimationEnabled( ksGltfScene * scene, const char * animationName, const bool enabled );
static void ksGltfScene_SetNodeTranslation( ksGltfScene * scene, const char * nodeName, const ksVector3f * translation );
static void ksGltfScene_SetNodeRotation( ksGltfScene * scene, const char * nodeName, const ksQuatf * rotation );
static void ksGltfScene_SetNodeScale( ksGltfScene * scene, const char * nodeName, const ksVector3f * scale );

static void ksGltfScene_Simulate( ksGltfScene * scene, ksViewState * viewState, ksGpuWindowInput * input, const ksNanoseconds time );
static void ksGltfScene_UpdateBuffers( ksGpuCommandBuffer * commandBuffer, ksGltfScene * scene, const ksViewState * viewState, const int eye );
static void ksGltfScene_Render( ksGpuCommandBuffer * commandBuffer, const ksGltfScene * scene, const ksViewState * viewState );

glTF 1.0 is not perfect.

Incorrect usage of JSON:
	- As opposed to using arrays with elements with a name property,
	  glTF uses objects with arbitrarily named members. Normally JSON
	  objects are well-defined with well-defined member names.
	  As a result, a JSON parser is needed that cannot just lookup
	  object members by name, but can also iterate over the object
	  members as if they are array elements. Not all JSON parsers
	  support this.
There are no JSON ordering requirements:
	- There are no requirements for how the JSON data is ordered. There
	  are obvious benefits from ordering the JSON data, such that all
	  references are backwards. In other words, any part of the JSON can
	  only reference previously defined parts. This would not only make
	  it trivial to use a SAX-style JSON parser but would also make it
	  trivial to parse the JSON in-place manually. This can be easily up
	  to two times faster than the fastest DOM-style parser.
	- There is no required ordering of the node hierarchy. If the node
	  hierarchy is stored depth or breath first per sub-tree (or in reverse)
	  then it is trivial to linearly walk the hierarchy and transform the
	  nodes from local space to global space. Recursively walking the
	  hierarchy means random memory access which is not efficient. An application
	  can sort the nodes at load time but that is just another step that
	  could be avoided.
There are way too many indirections for no obvious good reasons:
	- Why are there accessors to bufferViews?
	  Why not just have buffers and bufferViews and be done with it?
	- Why do vertex attributes lookup into a parameter table?
	- Why do uniforms lookup into the same parameter table?
	- Animations are similarly convoluted where channels lookup
	  into samplers which lookup into parameters. What is the benefit
	  of two indirections when the data could all be stored per channel?
	- Why do skins reference the jointName property of a node instead of
	  just referencing node names directly?
Redundant or unnecessary data:
	- Skins store a bindShapeMatrix for no good reason.
	- Why have the option to store a node.matrix instead of always storing
	  node.translation, node.rotation and node.scale.
The naming of things is sometimes awkward:
	- In normal graphics terminology meshes are connected pieces of geometry
	  and primitives are triangles (or lines or points). However, in glTF,
	  meshes are models and primitives are surfaces.
	- The stage of a shader is called "shader.type". Why isn't it called
	  "shader.stage"?
There are also obvious things missing:
	- There is no requirement that a buffer holds only one type of data.
	  A buffer may mix vertex attribute arrays, index arrays, animation
	  data and other data. In other words, you cannot just create one
	  graphics API buffer and reference it. You could instead create
	  graphics API buffers based on bufferViews, but there are typically
	  separate bufferViews for every separate piece of data that is used.
	  The bufferView.target member is also not a required member so a
	  bufferView may still not say anything about what kind of data it holds.
	- glTF 1.0 is limited to GLSL 1.00 which means there is no support for
	  uniforms buffers. For animations this means that the number of joints
	  per surface is very limited and updating a uniform array is also far
	  less efficient than updating a uniform buffer on modern hardware.
	- glTF is designed specifically for OpenGL making it hard to load glTF
	  with other graphics APIs.
	- No support for rendering the same geometry with different shaders.
	- Cannot play the same animation on different skeletons.
	- No control over playing different animations on the same skeleton.
	  However, an application could choose to enable/disable animations.
	- All animations time-lines are based on a global time. However, an
	  application can re-base the animation times and choose to play
	  animations at different times.
	- No support for fixed rate animations. Animations always use a time-line
	  with a time for each key frame, possibly with a variable delta
	  time in between key frames. To avoid a statefull animations system
	  (which would be a terrible idea) an application has to look up the
	  surrounding key frames from the time-line every frame. This can be
	  sped up by using a binary search but for long animations with many
	  hundreds of key frames this is still not cheap because it effectively
	  results in random memory access. To make matters worse there may be
	  a separate "animation" for every channel of a joint (scale, translation,
	  rotation) with a different time-line. Some models actually store a
	  different time-line per joint or channel, even though the actual
	  time-lines are the same. As a result, an expensive lookup may be
	  needed for every joint or even every channel, every frame. This is
	  particularly silly considering many models store a time-line that
	  is effectively fixed rate. Significant data preprocessing at load
	  time would be required to identify all the cases that can be optimized.
	- No animation compression. All animation data is stored as plain
	  floats. Even a simple factor 2.2 reduction would be trivial by storing
	  16-bit scales, localized 16-bit offsets, and the largest three
	  normalized quaternion components stored with 16-bits per component.
	  The variable rate time-line can be used for compression but having to
	  do a time-line lookup per channel would be horribly inefficient and
	  each time-line takes up a fair amount of memory as well with one float
	  per channel per key frame. Fixed rate animations would not only allow
	  for fast key frame lookups, but also trivial compression by omitting
	  key frames that can be derived with interpolation. Key frames from
	  a fixed rate animation can be trivially omitted using a bit mask.
	- No support for culling animated models (or meshes).

This glTF implementation overcomes some of these issues.
	- A fast DOM-style JSON parser is used to allow parsing randomly
	  ordered JSON data. This JSON parser also allows iterating over
	  object members as if they are array elements.
	- Fast single allocation hash tables are used to provide access to
	  all the different glTF objects.
	- Shaders are automatically converted to a newer GLSL version and
	  joint uniform arrays are automatically converted to joint uniform
	  buffers.
	- Where possible geometry buffers are shared between model surfaces
	  but creating one graphics API buffer for all vertex attributes and
	  one for all indices would still be much more efficient.
	- Animation channels are merged per joint to avoid a separate time-line
	  lookup per channel per joint.
	- Animation time-lines are often shared between animations. Therefore
	  animation time-lines are stored and evaluated saperately and shared
	  between animations. For variable rate time-lines this can significantly
	  reduce the number of searches through the time-lines.
	- Time-lines that are fixed-rate are identified at load time. A fixed-rate
	  time-line allows for very fast direct indexing of key frames based on
	  the current time.
	- The bindShapeMatrix is folded into the inverseBindMatrices at load
	  time to avoid another run-time matrix multiplication.
	- This implementation supports culling of animated models using the
	  KHR_skin_culling glTF extension.
	- The nodes are sorted to allow a simple linear walk to transform
	  nodes from local space to global space.

================================================================================================================================
*/

#include <utils/json.h>
#include <utils/base64.h>

static ksGpuProgramParm unitCubeFlatShadeProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	0,	"ModelMatrix",		0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	1,	"ViewMatrix",		0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	2,	"ProjectionMatrix",	0 }
};

static const char unitCubeFlatShadeVertexProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform mat4 ModelMatrix;\n"
	"uniform mat4 ViewMatrix;\n"
	"uniform mat4 ProjectionMatrix;\n"
	"in vec3 vertexPosition;\n"
	"in vec3 vertexNormal;\n"
	"out vec3 fragmentEyeDir;\n"
	"out vec3 fragmentNormal;\n"
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
	"	vec4 vertexWorldPos = ModelMatrix * vec4( vertexPosition, 1.0 );\n"
	"	vec3 eyeWorldPos = transposeMultiply3x3( ViewMatrix, -vec3( ViewMatrix[3] ) );\n"
	"	gl_Position = ProjectionMatrix * ( ViewMatrix * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( ModelMatrix, vertexNormal );\n"
	"}\n";

static const char unitCubeFlatShadeFragmentProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"in lowp vec3 fragmentEyeDir;\n"
	"in lowp vec3 fragmentNormal;\n"
	"out lowp vec4 outColor;\n"
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

#define GLTF_JSON_VERSION				"1.0"
#define GLTF_BINARY_MAGIC				( ( 'g' << 0 ) | ( 'l' << 8 ) | ( 'T' << 16 ) | ( 'F' << 24 ) )
#define GLTF_BINARY_VERSION				1
#define GLTF_BINARY_CONTENT_FORMAT		0

#define URI_SCHEME_APPLICATION_BINARY			"data:application/binary,"
#define URI_SCHEME_APPLICATION_BINARY_LENGTH	24

typedef struct ksGltfBinaryHeader
{
	uint32_t					magic;
	uint32_t					version;
	uint32_t					length;
	uint32_t					contentLength;
	uint32_t					contentFormat;
} ksGltfBinaryHeader;

typedef struct ksGltfBuffer
{
	char *						name;
	char *						type;
	size_t						byteLength;
	unsigned char *				bufferData;
} ksGltfBuffer;

typedef struct ksGltfBufferView
{
	char *						name;
	const ksGltfBuffer *		buffer;
	size_t						byteOffset;
	size_t						byteLength;
	int							target;
} ksGltfBufferView;

typedef struct ksGltfAccessor
{
	char *						name;
	char *						type;
	const ksGltfBufferView *	bufferView;
	size_t						byteOffset;
	size_t						byteStride;
	int							componentType;
	int							count;
	int							intMin[16];
	int							intMax[16];
	float						floatMin[16];
	float						floatMax[16];
} ksGltfAccessor;

typedef struct ksGltfImageVersion
{
	char *						container;			// jpg, png, bmp, gif, KTX
	int							glInternalFormat;
	char *						uri;
} ksGltfImageVersion;

typedef struct ksGltfImage
{
	char *						name;
	ksGltfImageVersion *		versions;
	int							versionCount;
} ksGltfImage;

typedef struct ksGltfSampler
{
	char *						name;
	int							magFilter;	// default GL_LINEAR
	int							minFilter;	// default GL_NEAREST_MIPMAP_LINEAR
	int							wrapS;		// default GL_REPEAT
	int							wrapT;		// default GL_REPEAT
} ksGltfSampler;

typedef struct ksGltfTexture
{
	char *						name;
	const ksGltfImage *			image;
	const ksGltfSampler *		sampler;
	ksGpuTexture				texture;
} ksGltfTexture;

typedef struct ksGltfShaderVersion
{
	char *						api;
	char *						version;
	char *						uri;
} ksGltfShaderVersion;

typedef enum
{
	GLTF_SHADER_TYPE_GLSL,
	GLTF_SHADER_TYPE_SPIRV,
	GLTF_SHADER_TYPE_HLSL,
	GLTF_SHADER_TYPE_METALSL,
	GLTF_SHADER_TYPE_MAX
} ksGltfShaderType;

static const char * shaderVersionExtensions[] =
{
	"KHR_glsl_shader_versions",
	"KHR_spirv_shader_versions",
	"KHR_hlsl_shader_versions",
	"KHR_metalsl_shader_versions"
};

typedef struct ksGltfShader
{
	char *						name;
	int							stage;	// GL_VERTEX_SHADER, GL_FRAGMENT_SHADER
	ksGltfShaderVersion *		shaders[GLTF_SHADER_TYPE_MAX];
	int							shaderCount[GLTF_SHADER_TYPE_MAX];
} ksGltfShader;

typedef struct ksGltfProgram
{
	char *						name;
	unsigned char *				vertexSource;
	unsigned char *				fragmentSource;
	size_t						vertexSourceSize;
	size_t						fragmentSourceSize;
} ksGltfProgram;

typedef enum
{
	GLTF_UNIFORM_SEMANTIC_NONE,
	GLTF_UNIFORM_SEMANTIC_DEFAULT_VALUE,
	GLTF_UNIFORM_SEMANTIC_LOCAL,
	GLTF_UNIFORM_SEMANTIC_VIEW,
	GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE,
	GLTF_UNIFORM_SEMANTIC_PROJECTION,
	GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE,
	GLTF_UNIFORM_SEMANTIC_MODEL,
	GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE,
	GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE_TRANSPOSE,
	GLTF_UNIFORM_SEMANTIC_MODEL_VIEW,
	GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE,
	GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE_TRANSPOSE,
	GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION,
	GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION_INVERSE,
	GLTF_UNIFORM_SEMANTIC_VIEWPORT,
	GLTF_UNIFORM_SEMANTIC_JOINT_ARRAY,
	// new semantic values
	GLTF_UNIFORM_SEMANTIC_JOINT_BUFFER,
	GLTF_UNIFORM_SEMANTIC_VIEW_PROJECTION_BUFFER,
	GLTF_UNIFORM_SEMANTIC_VIEW_PROJECTION_MULTI_VIEW_BUFFER,
	GLTF_UNIFORM_SEMANTIC_MAX
} ksGltfUniformSemantic;

static struct
{
	const char *			name;
	ksGltfUniformSemantic	semantic;
}
gltfUniformSemanticNames[] =
{
	{ "",								GLTF_UNIFORM_SEMANTIC_NONE },
	{ "",								GLTF_UNIFORM_SEMANTIC_DEFAULT_VALUE },
	{ "LOCAL",							GLTF_UNIFORM_SEMANTIC_LOCAL },
	{ "VIEW",							GLTF_UNIFORM_SEMANTIC_VIEW },
	{ "VIEWINVERSE",					GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE },
	{ "PROJECTION",						GLTF_UNIFORM_SEMANTIC_PROJECTION },
	{ "PROJECTIONINVERSE",				GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE },
	{ "MODEL",							GLTF_UNIFORM_SEMANTIC_MODEL },
	{ "MODELINVERSE",					GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE },
	{ "MODELINVERSETRANSPOSE",			GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE_TRANSPOSE },
	{ "MODELVIEW",						GLTF_UNIFORM_SEMANTIC_MODEL_VIEW },
	{ "MODELVIEWINVERSE",				GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE },
	{ "MODELVIEWINVERSETRANSPOSE",		GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE_TRANSPOSE },
	{ "MODELVIEWPROJECTION",			GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION },
	{ "MODELVIEWPROJECTIONINVERSE",		GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION_INVERSE },
	{ "VIEWPORT",						GLTF_UNIFORM_SEMANTIC_VIEWPORT },
	{ "JOINTMATRIX",					GLTF_UNIFORM_SEMANTIC_JOINT_ARRAY },
	// new semantic values
	{ "JOINTBUFFER",					GLTF_UNIFORM_SEMANTIC_JOINT_BUFFER },						// KHR_glsl_joint_buffer
	{ "VIEWPROJECTIONBUFFER",			GLTF_UNIFORM_SEMANTIC_VIEW_PROJECTION_BUFFER },				// KHR_glsl_view_projection_buffer
	{ "VIEWPROJECTIONMULTIVIEWBUFFER",	GLTF_UNIFORM_SEMANTIC_VIEW_PROJECTION_MULTI_VIEW_BUFFER },	// KHR_glsl_multi_view
	{ NULL,								0 }
};

typedef struct ksGltfUniformValue
{
	ksGltfTexture *				texture;
	int							intValue[16];
	float						floatValue[16];
} ksGltfUniformValue;

typedef struct ksGltfUniform
{
	char *						name;
	ksGltfUniformSemantic		semantic;
	ksGpuProgramParmType		type;
	int							index;
	ksGltfUniformValue			defaultValue;
} ksGltfUniform;

typedef struct ksGltfVertexAttribute
{
	char *						name;
	ksGpuAttributeFormat		format;
	int							attributeFlag;
	int							location;
} ksGltfVertexAttribute;

typedef struct ksGltfTechnique
{
	char *						name;
	ksGpuGraphicsProgram		program;
	ksGpuProgramParm *			parms;
	ksGltfUniform *				uniforms;
	int							uniformCount;
	ksGltfVertexAttribute *		attributes;
	int							attributeCount;
	ksGpuVertexAttribute *		vertexAttributeLayout;
	int							vertexAttribsFlags;
	ksGpuRasterOperations		rop;
} ksGltfTechnique;

typedef struct ksGltfMaterialValue
{
	ksGltfUniform *				uniform;
	ksGltfUniformValue			value;
} ksGltfMaterialValue;

typedef struct ksGltfMaterial
{
	char *						name;
	const ksGltfTechnique *		technique;
	ksGltfMaterialValue *		values;
	int							valueCount;
} ksGltfMaterial;

typedef struct ksGltfGeometryAccessors
{
	const ksGltfAccessor *		position;
	const ksGltfAccessor *		normal;
	const ksGltfAccessor *		tangent;
	const ksGltfAccessor *		binormal;
	const ksGltfAccessor *		color;
	const ksGltfAccessor *		uv0;
	const ksGltfAccessor *		uv1;
	const ksGltfAccessor *		uv2;
	const ksGltfAccessor *		jointIndices;
	const ksGltfAccessor *		jointWeights;
	const ksGltfAccessor *		indices;
} ksGltfGeometryAccessors;

typedef struct ksGltfSurface
{
	const ksGltfMaterial *		material;		// material used to render this surface
	ksGpuGeometry				geometry;		// surface geometry
	ksGpuGraphicsPipeline		pipeline;		// rendering pipeline for this surface
	ksVector3f					mins;			// minimums of the surface geometry excluding animations
	ksVector3f					maxs;			// maximums of the surface geometry excluding animations
} ksGltfSurface;

typedef struct ksGltfModel
{
	char *						name;
	ksGltfSurface *				surfaces;
	int							surfaceCount;
	ksVector3f					mins;			// minimums of the surface geometry excluding animations
	ksVector3f					maxs;			// maximums of the surface geometry excluding animations
} ksGltfModel;

typedef struct ksGltfTimeLine
{
	float						duration;		// in seconds
	float						rcpStep;		// in seconds
	float *						sampleTimes;	// in seconds
	int							sampleCount;
} ksGltfTimeLine;

typedef struct ksGltfAnimationChannel
{
	char *						nodeName;
	struct ksGltfNode *			node;
	ksQuatf *					rotation;
	ksVector3f *				translation;
	ksVector3f *				scale;
} ksGltfAnimationChannel;

typedef struct ksGltfAnimation
{
	char *						name;
	ksGltfTimeLine *			timeLine;
	ksGltfAnimationChannel *	channels;
	int							channelCount;
} ksGltfAnimation;

typedef struct ksGltfJoint
{
	char *						name;
	struct ksGltfNode *			node;
} ksGltfJoint;

typedef struct ksGltfSkin
{
	char *						name;
	struct ksGltfNode *			parentNode;
	ksMatrix4x4f *				inverseBindMatrices;
	ksVector3f *				jointGeometryMins;		// joint local space minimums of the geometry influenced by each joint
	ksVector3f *				jointGeometryMaxs;		// joint local space maximums of the geometry influenced by each joint
	ksGltfJoint *				joints;					// joints of this skin
	int							jointCount;				// number of joints
	ksGpuBuffer					jointBuffer;			// buffer with joint matrices
	ksVector3f					mins;					// minimums of the complete skin geometry (modified at run-time)
	ksVector3f					maxs;					// maximums of the complete skin geometry (modified at run-time)
	bool						culled;					// true if the skin is culled (modified at run-time)
} ksGltfSkin;

typedef enum
{
	GLTF_CAMERA_TYPE_PERSPECTIVE,
	GLTF_CAMERA_TYPE_ORTHOGRAPHIC
} ksGltfCameraType;

typedef struct ksGltfCamera
{
	char *						name;
	ksGltfCameraType			type;
	struct
	{
		float					aspectRatio;
		float					fovDegreesX;
		float					fovDegreesY;
		float					nearZ;
		float					farZ;
	}							perspective;
	struct
	{
		float					magX;
		float					magY;
		float					nearZ;
		float					farZ;
	}							orthographic;
} ksGltfCamera;

typedef struct ksGltfNode
{
	char *						name;
	char *						jointName;
	ksQuatf						rotation;
	ksVector3f					translation;
	ksVector3f					scale;
	int							subTreeNodeCount;	// this node plus the number of direct or indirect decendants
	struct ksGltfNode **		children;
	char **						childNames;
	int							childCount;
	struct ksGltfNode *			parent;
	struct ksGltfCamera *		camera;
	struct ksGltfSkin *			skin;
	struct ksGltfModel **		models;
	int							modelCount;
} ksGltfNode;

typedef struct ksGltfSubTree
{
	char *						name;
	ksGltfNode **				nodes;
	int							nodeCount;
	ksGltfTimeLine **			timeLines;
	int							timeLineCount;
	ksGltfAnimation **			animations;
	int							animationCount;
} ksGltfSubTree;

typedef struct ksGltfSubScene
{
	char *						name;
	ksGltfSubTree **			subTrees;
	int							subTreeCount;
} ksGltfSubScene;

typedef struct ksGltfTimeLineFrameState
{
	int							frame;
	float						fraction;
} ksGltfTimeLineFrameState;

typedef struct ksGltfSkinCullingState
{
	ksVector3f					mins;				// minimums of the complete skin geometry
	ksVector3f					maxs;				// maximums of the complete skin geometry
	bool						culled;				// true if the skin is culled
} ksGltfSkinCullingState;

typedef struct ksGltfNodeState
{
	struct ksGltfNodeState *	parent;
	ksQuatf						rotation;
	ksVector3f					translation;
	ksVector3f					scale;
	ksMatrix4x4f				localTransform;
	ksMatrix4x4f				globalTransform;
} ksGltfNodeState;

typedef struct ksGltfSubTreeState
{
	bool						visible;
} ksGltfSubTreeState;

typedef struct ksGltfState
{
	ksGltfSubScene *			currentSubScene;
	ksGltfTimeLineFrameState *	timeLineFrameState;
	ksGltfSkinCullingState *	skinCullingState;
	ksGltfNodeState *			nodeState;
	ksGltfSubTreeState *		subTreeState;
} ksGltfState;

typedef struct ksGltfScene
{
	ksGltfBuffer *				buffers;
	int *						bufferNameHash;
	int							bufferCount;
	ksGltfBufferView *			bufferViews;
	int *						bufferViewNameHash;
	int							bufferViewCount;
	ksGltfAccessor *			accessors;
	int *						accessorNameHash;
	int							accessorCount;
	ksGltfImage *				images;
	int *						imageNameHash;
	int							imageCount;
	ksGltfSampler *				samplers;
	int *						samplerNameHash;
	int							samplerCount;
	ksGltfTexture *				textures;
	int *						textureNameHash;
	int							textureCount;
	ksGltfShader *				shaders;
	int *						shaderNameHash;
	int							shaderCount;
	ksGltfProgram *				programs;
	int *						programNameHash;
	int							programCount;
	ksGltfTechnique *			techniques;
	int *						techniqueNameHash;
	int							techniqueCount;
	ksGltfMaterial *			materials;
	int *						materialNameHash;
	int							materialCount;
	ksGltfSkin *				skins;
	int *						skinNameHash;
	int							skinCount;
	ksGltfModel *				models;
	int *						modelNameHash;
	int							modelCount;
	ksGltfTimeLine *			timeLines;
	int *						timeLineNameHash;
	int							timeLineCount;
	ksGltfAnimation *			animations;
	int *						animationNameHash;
	int							animationCount;
	ksGltfCamera *				cameras;
	int *						cameraNameHash;
	int							cameraCount;
	ksGltfNode *				nodes;
	int *						nodeNameHash;
	int *						nodeJointNameHash;
	int							nodeCount;
	ksGltfSubTree *				subTrees;
	int *						subTreeNameHash;
	int							subTreeCount;
	ksGltfSubScene *			subScenes;
	int *						subSceneNameHash;
	int							subSceneCount;

	ksGltfState					state;

	ksGpuBuffer					viewProjectionBuffer;
	ksGpuBuffer					defaultJointBuffer;
	ksGpuGeometry				unitCubeGeometry;
	ksGpuGraphicsProgram		unitCubeFlatShadeProgram;
	ksGpuGraphicsPipeline		unitCubePipeline;
} ksGltfScene;

#define HASH_TABLE_SIZE		256

static unsigned int StringHash( const char * string )
{
	ksStringHash hash;
	ksStringHash_Init( &hash );
	ksStringHash_Update( &hash, string );
	return ( hash & ( HASH_TABLE_SIZE - 1 ) );
}

#define GLTF_HASH( type, typeCapitalized, name, nameCapitalized ) \
	static void ksGltf_Create##typeCapitalized##nameCapitalized##Hash( ksGltfScene * scene ) \
	{ \
		scene->type##nameCapitalized##Hash = (int *) malloc( ( HASH_TABLE_SIZE + scene->type##Count ) * sizeof( scene->type##nameCapitalized##Hash[0] ) ); \
		memset( scene->type##nameCapitalized##Hash, -1, ( HASH_TABLE_SIZE + scene->type##Count ) * sizeof( scene->type##nameCapitalized##Hash[0] ) ); \
		for ( int i = 0; i < scene->type##Count; i++ ) \
		{ \
			const unsigned int hash = StringHash( scene->type##s[i].name ); \
			scene->type##nameCapitalized##Hash[HASH_TABLE_SIZE + i] = scene->type##nameCapitalized##Hash[hash]; \
			scene->type##nameCapitalized##Hash[hash] = i; \
		} \
	} \
	\
	static ksGltf##typeCapitalized * ksGltf_Get##typeCapitalized##By##nameCapitalized( const ksGltfScene * scene, const char * name ) \
	{ \
		const unsigned int hash = StringHash( name ); \
		for ( int i = scene->type##nameCapitalized##Hash[hash]; i >= 0; i = scene->type##nameCapitalized##Hash[HASH_TABLE_SIZE + i] ) \
		{ \
			if ( strcmp( scene->type##s[i].name, name ) == 0 ) \
			{ \
				return &scene->type##s[i]; \
			} \
		} \
		return NULL; \
	}

GLTF_HASH( buffer,		Buffer,		name,		Name );
GLTF_HASH( bufferView,	BufferView,	name,		Name );
GLTF_HASH( accessor,	Accessor,	name,		Name );
GLTF_HASH( image,		Image,		name,		Name );
GLTF_HASH( sampler,		Sampler,	name,		Name );
GLTF_HASH( texture,		Texture,	name,		Name );
GLTF_HASH( shader,		Shader,		name,		Name );
GLTF_HASH( program,		Program,	name,		Name );
GLTF_HASH( technique,	Technique,	name,		Name );
GLTF_HASH( material,	Material,	name,		Name );
GLTF_HASH( skin,		Skin,		name,		Name );
GLTF_HASH( model,		Model,		name,		Name );
GLTF_HASH( animation,	Animation,	name,		Name );
GLTF_HASH( camera,		Camera,		name,		Name );
GLTF_HASH( node,		Node,		name,		Name );
GLTF_HASH( node,		Node,		jointName,	JointName );
GLTF_HASH( subTree,		SubTree,	name,		Name );
GLTF_HASH( subScene,	SubScene,	name,		Name );

static ksGltfAccessor * ksGltf_GetAccessorByNameAndType( const ksGltfScene * scene, const char * name, const char * type, const int componentType )
{
	ksGltfAccessor * accessor = ksGltf_GetAccessorByName( scene, name );
	if ( accessor != NULL &&
			accessor->componentType == componentType &&
				strcmp( accessor->type, type ) == 0 )
	{
		return accessor;
	}
	return NULL;
}

static void * ksGltf_GetBufferData( const ksGltfAccessor * accessor )
{
	if ( accessor != NULL )
	{
		return accessor->bufferView->buffer->bufferData + accessor->bufferView->byteOffset + accessor->byteOffset;
	}
	return NULL;
}

static char * ksGltf_strdup( const char * str )
{
	char * out = (char *)malloc( strlen( str ) + 1 );
	strcpy( out, str );
	return out;
}

static unsigned char * ksGltf_ReadFile( const char * fileName, size_t * outSizeInBytes )
{
	if ( outSizeInBytes != NULL )
	{
		*outSizeInBytes = 0;
	}
	FILE * file = fopen( fileName, "rb" );
	if ( file == NULL )
	{
		return NULL;
	}
	const size_t maxSizeInBytes = ( outSizeInBytes != NULL && *outSizeInBytes > 0 ) ? *outSizeInBytes : SIZE_MAX;
	fseek( file, 0L, SEEK_END );
	size_t bufferSize = (size_t) ftell( file );
	bufferSize = MIN( bufferSize, maxSizeInBytes );
	fseek( file, 0L, SEEK_SET );
	unsigned char * buffer = (unsigned char *) malloc( bufferSize + 1 );
	if ( fread( buffer, 1, bufferSize, file ) != bufferSize )
	{
		fclose( file );
		free( buffer );
		return NULL;
	}
	buffer[bufferSize] = '\0';
	fclose( file );
	if ( outSizeInBytes != NULL )
	{
		*outSizeInBytes = bufferSize;
	}
	return buffer;
}

static unsigned char * ksGltf_ReadPlainText( const char * uri, size_t * outSizeInBytes )
{
	const size_t maxSizeInBytes = ( outSizeInBytes != NULL && *outSizeInBytes > 0 ) ? *outSizeInBytes : SIZE_MAX;
	const size_t length = MIN( strlen( uri ), maxSizeInBytes );
	if ( outSizeInBytes != NULL )
	{
		*outSizeInBytes = length;
	}
	char * out = (char *)malloc( length + 1 );
	strcpy( out, uri );
	return (unsigned char *)out;
}

static unsigned char * ksGltf_ReadBase64( const char * base64, size_t * outSizeInBytes )
{
	const size_t maxSizeInBytes = ( outSizeInBytes != NULL && *outSizeInBytes > 0 ) ? *outSizeInBytes : SIZE_MAX;
	size_t base64SizeInBytes = strlen( base64 );
	size_t dataSizeInBytes = Base64_DecodeSizeInBytes( base64, base64SizeInBytes );
	dataSizeInBytes = MIN( dataSizeInBytes, maxSizeInBytes );
	unsigned char * buffer = (unsigned char *)malloc( dataSizeInBytes );
	Base64_Decode( buffer, base64, base64SizeInBytes, dataSizeInBytes );
	if ( outSizeInBytes != NULL )
	{
		*outSizeInBytes = dataSizeInBytes;
	}
	return buffer;
}

static unsigned char * ksGltf_ReadBinaryBuffer( const unsigned char * binaryBuffer, const char * uri, size_t * outSizeInBytes )
{
	const size_t maxSizeInBytes = ( outSizeInBytes != NULL && *outSizeInBytes > 0 ) ? *outSizeInBytes : SIZE_MAX;
	char * endptr = NULL;
	size_t byteOffset = (size_t) strtol( uri, &endptr, 16 );
	size_t byteLength = (size_t) strtol( endptr + 1, &endptr, 16 );
	byteLength = MIN( byteLength, maxSizeInBytes );
	unsigned char * data = (unsigned char *) malloc( byteLength + 1 );
	memcpy( data, binaryBuffer + byteOffset, byteLength );
	data[byteLength] = '\0';
	if ( outSizeInBytes != NULL )
	{
		*outSizeInBytes = (int)byteLength;
	}
	return data;
}

// if outSizeInBytes != NULL and *outSizeInBytes > 0 then only *outSizeInBytes bytes will be read.
static unsigned char * ksGltf_ReadUri( const unsigned char * binaryBuffer, const char * uri, size_t * outSizeInBytes )
{
	if ( strncmp( uri, "data:", 5 ) == 0 )
	{
		// Plain text.
		if ( strncmp( uri, "data:text/plain,", 16 ) == 0 )
		{
			return ksGltf_ReadPlainText( uri + 16, outSizeInBytes );
		}
		// Base64 text shader.
		else if ( strncmp( uri, "data:text/plain;base64,", 23 ) == 0 )
		{
			return ksGltf_ReadBase64( uri + 23, outSizeInBytes );
		}
		// Base64 binary buffer.
		else if ( strncmp( uri, "data:application/octet-stream;base64,", 37 ) == 0 )
		{
			return ksGltf_ReadBase64( uri + 37, outSizeInBytes );
		}
		// Base64 JPG, PNG, BMP, GIF, KTX image.
		else if (	strncmp( uri, "data:image/jpg;base64,", 22 ) == 0 ||
					strncmp( uri, "data:image/png;base64,", 22 ) == 0 ||
					strncmp( uri, "data:image/bmp;base64,", 22 ) == 0 ||
					strncmp( uri, "data:image/gif;base64,", 22 ) == 0 ||
					strncmp( uri, "data:image/ktx;base64,", 22 ) == 0 )
		{
			return ksGltf_ReadBase64( uri + 22, outSizeInBytes );
		}
		// bufferView
		else if ( strncmp( uri, URI_SCHEME_APPLICATION_BINARY, URI_SCHEME_APPLICATION_BINARY_LENGTH ) == 0 )
		{
			return ksGltf_ReadBinaryBuffer( binaryBuffer, uri + URI_SCHEME_APPLICATION_BINARY_LENGTH, outSizeInBytes );
		}
	}
	return ksGltf_ReadFile( uri, outSizeInBytes );
}

static char * ksGltf_ParseUri( const ksGltfScene * scene, const Json_t * json, const char * uriName )
{
	const Json_t * jsonUri = Json_GetMemberByName( json, uriName );
	if ( jsonUri == NULL )
	{
		return ksGltf_strdup( "" );
	}
	const Json_t * extensions = Json_GetMemberByName( json, "extensions" );
	if ( extensions != NULL )
	{
		const char * bufferViewName = Json_GetString( Json_GetMemberByName( Json_GetMemberByName( extensions, "KHR_binary_glTF" ), "bufferView" ), "" );
		if ( bufferViewName[0] != '\0' )
		{
			const ksGltfBufferView * bufferView = ksGltf_GetBufferViewByName( scene, bufferViewName );
			if ( bufferView != NULL )
			{
				char * uri = (char *) malloc( URI_SCHEME_APPLICATION_BINARY_LENGTH + 10 + 1 + 10 + 1 );
				sprintf( uri, "%s0x%X,0x%X", URI_SCHEME_APPLICATION_BINARY, (uint32_t)bufferView->byteOffset, (uint32_t)bufferView->byteLength );
				return uri;
			}
		}
	}
	return ksGltf_strdup( Json_GetString( jsonUri, "" ) );
}

const char * ksGltf_GetImageContainerFromUri( const char * uri )
{
	if ( strncmp( uri, "data:image/", 11 ) == 0 )
	{
		if ( strncmp( uri + 11, "jpg;", 4 ) == 0 ) { return "jpg"; }
		if ( strncmp( uri + 11, "png;", 4 ) == 0 ) { return "png"; }
		if ( strncmp( uri + 11, "bmp;", 4 ) == 0 ) { return "bmp"; }
		if ( strncmp( uri + 11, "gif;", 4 ) == 0 ) { return "gif"; }
		if ( strncmp( uri + 11, "ktx;", 4 ) == 0 ) { return "ktx"; }
	}
	return "";
}

const int ksGltf_GetImageInternalFormatFromUri( const unsigned char * binaryBuffer, const char * uri )
{
	int glInternalFormat = GL_RGB8;
	if ( strncmp( uri, "data:image/", 11 ) == 0 )
	{
		if ( strncmp( uri + 11, "jpg;", 4 ) == 0 )
		{
			glInternalFormat = GL_RGB8;
		}
		else if ( strncmp( uri + 11, "png;", 4 ) == 0 )
		{
			size_t outSizeInBytes = 16;
			unsigned char * data = ksGltf_ReadUri( binaryBuffer, uri, &outSizeInBytes );
			glInternalFormat = ( data[9] == 4 || data[9] == 6 ) ? GL_RGBA8 : GL_RGB8;
			free( data );
		}
		else if ( strncmp( uri + 11, "bmp;", 4 ) == 0 )
		{
			size_t outSizeInBytes = 32;
			unsigned char * data = ksGltf_ReadUri( binaryBuffer, uri, &outSizeInBytes );
			glInternalFormat = ( ( data[28] | ( data[29] << 8 ) ) == 32 ) ? GL_RGBA8 : GL_RGB8;
			free( data );
		}
		else if ( strncmp( uri + 11, "gif;", 4 ) == 0 )
		{
			size_t outSizeInBytes = 1024;
			unsigned char * data = ksGltf_ReadUri( binaryBuffer, uri, &outSizeInBytes );
			const size_t colorTableSize = ( data[6 + 4] >> 7 ) * 3 * ( 1 << ( ( ( data[6 + 4] >> 4 ) & 7 ) + 1 ) );
			if ( data[6 + 7 + colorTableSize + 0] == 0x21 && data[6 + 7 + colorTableSize + 1] == 0xF9 )
			{
				glInternalFormat = ( data[6 + 7 + colorTableSize + 3] >> 7 ) ? GL_RGBA8 : GL_RGB8;
			}
			free( data );
		}
		else if ( strncmp( uri + 11, "ktx;", 4 ) == 0 )
		{
			size_t outSizeInBytes = 48;
			unsigned char * data = ksGltf_ReadUri( binaryBuffer, uri, &outSizeInBytes );
			glInternalFormat = ( data[28] | ( data[29] << 8 ) || ( data[30] << 16 ) | ( data[31] << 24 ) );
			free( data );
		}
	}
	return glInternalFormat;
}

typedef enum
{
	GLTF_COMPRESSED_IMAGE_DXT			= BIT( 0 ),
	GLTF_COMPRESSED_IMAGE_DXT_SRGB		= BIT( 1 ),
	GLTF_COMPRESSED_IMAGE_ETC2			= BIT( 2 ),
	GLTF_COMPRESSED_IMAGE_ETC2_SRGB		= BIT( 3 ),
	GLTF_COMPRESSED_IMAGE_ASTC			= BIT( 4 ),
	GLTF_COMPRESSED_IMAGE_ASTC_SRGB		= BIT( 5 )
} ksGltfCompressedImageFlags;

static const char * ksGltf_FindImageUri( const ksGltfImage * image, const char * containers[], const ksGltfCompressedImageFlags flags )
{
	for ( int i = 0; i < image->versionCount; i++ )
	{
		bool found = false;
		for ( int j = 0; containers[j] != NULL; j++ )
		{
			if ( strcmp( image->versions[i].container, containers[j] ) == 0 )
			{
				found = true;
				break;
			}
		}
		if ( !found )
		{
			continue;
		}

		if ( ( flags & GLTF_COMPRESSED_IMAGE_DXT ) == 0 )
		{
			if (	( image->versions[i].glInternalFormat >= GL_COMPRESSED_RGB_S3TC_DXT1_EXT &&
					image->versions[i].glInternalFormat <= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ) ||
					( image->versions[i].glInternalFormat >= GL_COMPRESSED_LUMINANCE_LATC1_EXT &&
					image->versions[i].glInternalFormat <= GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT ) )
			{
				continue;
			}
		}

		if ( ( flags & GLTF_COMPRESSED_IMAGE_DXT_SRGB ) == 0 )
		{
			if (	( image->versions[i].glInternalFormat >= GL_COMPRESSED_SRGB_S3TC_DXT1_EXT &&
					image->versions[i].glInternalFormat <= GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT ) )
			{
				continue;
			}
		}

		if ( ( flags & GLTF_COMPRESSED_IMAGE_ETC2 ) == 0 )
		{
			if (	image->versions[i].glInternalFormat == GL_COMPRESSED_RGB8_ETC2 ||
					image->versions[i].glInternalFormat == GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 ||
					image->versions[i].glInternalFormat == GL_COMPRESSED_RGBA8_ETC2_EAC ||
					image->versions[i].glInternalFormat == GL_COMPRESSED_R11_EAC ||
					image->versions[i].glInternalFormat == GL_COMPRESSED_SIGNED_RG11_EAC )
			{
				continue;
			}
		}

		if ( ( flags & GLTF_COMPRESSED_IMAGE_ETC2_SRGB ) == 0 )
		{
			if (	image->versions[i].glInternalFormat == GL_COMPRESSED_SRGB8_ETC2 ||
					image->versions[i].glInternalFormat == GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 ||
					image->versions[i].glInternalFormat == GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC )
			{
				continue;
			}
		}

		if ( ( flags & GLTF_COMPRESSED_IMAGE_ASTC ) == 0 )
		{
			if (	image->versions[i].glInternalFormat >= GL_COMPRESSED_RGBA_ASTC_4x4_KHR &&
					image->versions[i].glInternalFormat >= GL_COMPRESSED_RGBA_ASTC_12x12_KHR )
			{
				continue;
			}
		}

		if ( ( flags & GLTF_COMPRESSED_IMAGE_ASTC ) == 0 )
		{
			if (	image->versions[i].glInternalFormat >= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR &&
					image->versions[i].glInternalFormat >= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR )
			{
				continue;
			}
		}

		return image->versions[i].uri;
	}

	return "";
}

static const char * ksGltf_FindShaderUri( const ksGltfShader * shader, const ksGltfShaderType type, const char * apiString, const char * maxVersionString )
{
	const int maxVersion = atoi( maxVersionString );
	const char * bestUri = NULL;
	int bestVersion = 0;
	for ( int i = 0; i < shader->shaderCount[type]; i++ )
	{
		if ( strcmp( shader->shaders[type][i].api, apiString ) == 0 )
		{
			const int version = atoi( shader->shaders[type][i].version );
			if ( version <= maxVersion && version > bestVersion )
			{
				bestVersion = version;
				bestUri = shader->shaders[type][i].uri;
			}
		}
	}
	return bestUri;
}

static void ksGltf_ParseIntArray( int * elements, const int count, const Json_t * arrayNode )
{
	int i = 0;
	for ( ; i < Json_GetMemberCount( arrayNode ) && i < count; i++ )
	{
		elements[i] = Json_GetInt32( Json_GetMemberByIndex( arrayNode, i ), 0 );
	}
	for ( ; i < count; i++ )
	{
		elements[i] = 0;
	}
}

static void ksGltf_ParseFloatArray( float * elements, const int count, const Json_t * arrayNode )
{
	int i = 0;
	for ( ; i < Json_GetMemberCount( arrayNode ) && i < count; i++ )
	{
		elements[i] = Json_GetFloat( Json_GetMemberByIndex( arrayNode, i ), 0.0f );
	}
	for ( ; i < count; i++ )
	{
		elements[i] = 0.0f;
	}
}

static void ksGltf_ParseUniformValue( ksGltfUniformValue * value, const Json_t * json, const ksGpuProgramParmType type, const ksGltfScene * scene )
{
	switch ( type )
	{
		case KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED:					value->texture = ksGltf_GetTextureByName( scene, Json_GetString( json, "" ) ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT:				value->intValue[0] = Json_GetInt32( json, 0 ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2:		ksGltf_ParseIntArray( value->intValue, 16, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3:		ksGltf_ParseIntArray( value->intValue, 16, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4:		ksGltf_ParseIntArray( value->intValue, 16, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT:				value->floatValue[0] = Json_GetFloat( json, 0.0f ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2:		ksGltf_ParseFloatArray( value->floatValue, 2, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3:		ksGltf_ParseFloatArray( value->floatValue, 3, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4:		ksGltf_ParseFloatArray( value->floatValue, 4, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2:	ksGltf_ParseFloatArray( value->floatValue, 2*2, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3:	ksGltf_ParseFloatArray( value->floatValue, 2*3, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4:	ksGltf_ParseFloatArray( value->floatValue, 2*4, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2:	ksGltf_ParseFloatArray( value->floatValue, 3*2, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3:	ksGltf_ParseFloatArray( value->floatValue, 3*3, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4:	ksGltf_ParseFloatArray( value->floatValue, 3*4, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2:	ksGltf_ParseFloatArray( value->floatValue, 4*2, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3:	ksGltf_ParseFloatArray( value->floatValue, 4*3, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4:	ksGltf_ParseFloatArray( value->floatValue, 4*4, json ); break;
		default: break;
	}
}

static ksGpuTextureFilter ksGltf_GetTextureFilter( const int filter )
{
	switch ( filter )
	{
		case GL_NEAREST:					return KS_GPU_TEXTURE_FILTER_NEAREST;
		case GL_LINEAR:						return KS_GPU_TEXTURE_FILTER_LINEAR;
		case GL_NEAREST_MIPMAP_NEAREST:		return KS_GPU_TEXTURE_FILTER_NEAREST;
		case GL_LINEAR_MIPMAP_NEAREST:		return KS_GPU_TEXTURE_FILTER_NEAREST;
		case GL_NEAREST_MIPMAP_LINEAR:		return KS_GPU_TEXTURE_FILTER_BILINEAR;
		case GL_LINEAR_MIPMAP_LINEAR:		return KS_GPU_TEXTURE_FILTER_BILINEAR;
		default:							return KS_GPU_TEXTURE_FILTER_BILINEAR;
	}
}

static ksGpuTextureWrapMode ksGltf_GetTextureWrapMode( const int wrapMode )
{
	switch ( wrapMode )
	{
		case GL_REPEAT:				return KS_GPU_TEXTURE_WRAP_MODE_REPEAT;
		case GL_CLAMP_TO_EDGE:		return KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_EDGE;
		case GL_CLAMP_TO_BORDER:	return KS_GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER;
		default:					return KS_GPU_TEXTURE_WRAP_MODE_REPEAT;
	}
}

static ksGpuProgramStageFlags ksGltf_GetProgramStageFlag( const int stage )
{
	switch ( stage )
	{
		case GL_VERTEX_SHADER:		return KS_GPU_PROGRAM_STAGE_FLAG_VERTEX;
		case GL_FRAGMENT_SHADER:	return KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT;
		default:					return KS_GPU_PROGRAM_STAGE_FLAG_VERTEX;
	}
}

static ksGpuFrontFace ksGltf_GetFrontFace( const int face )
{
	switch ( face )
	{
		case GL_CCW:	return KS_GPU_FRONT_FACE_COUNTER_CLOCKWISE;
		case GL_CW:		return KS_GPU_FRONT_FACE_CLOCKWISE;
		default:		return KS_GPU_FRONT_FACE_COUNTER_CLOCKWISE;
	}
}

static ksGpuCullMode ksGltf_GetCullMode( const int mode )
{
	switch ( mode )
	{
		case GL_NONE:	return KS_GPU_CULL_MODE_NONE;
		case GL_FRONT:	return KS_GPU_CULL_MODE_FRONT;
		case GL_BACK:	return KS_GPU_CULL_MODE_BACK;
		default:		return KS_GPU_CULL_MODE_BACK;
	}
}

static ksGpuCompareOp ksGltf_GetCompareOp( const int op )
{
	switch ( op )
	{
		case GL_NEVER:		return KS_GPU_COMPARE_OP_NEVER;
		case GL_LESS:		return KS_GPU_COMPARE_OP_LESS;
		case GL_EQUAL:		return KS_GPU_COMPARE_OP_EQUAL;
		case GL_LEQUAL:		return KS_GPU_COMPARE_OP_LESS_OR_EQUAL;
		case GL_GREATER:	return KS_GPU_COMPARE_OP_GREATER;
		case GL_NOTEQUAL:	return KS_GPU_COMPARE_OP_NOT_EQUAL;
		case GL_GEQUAL:		return KS_GPU_COMPARE_OP_GREATER_OR_EQUAL;
		case GL_ALWAYS:		return KS_GPU_COMPARE_OP_ALWAYS;
		default:			return KS_GPU_COMPARE_OP_LESS;
	}
}

static ksGpuBlendOp ksGltf_GetBlendOp( const int op )
{
	switch( op )
	{
		case GL_FUNC_ADD:				return KS_GPU_BLEND_OP_ADD;
		case GL_FUNC_SUBTRACT:			return KS_GPU_BLEND_OP_SUBTRACT;
		case GL_FUNC_REVERSE_SUBTRACT:	return KS_GPU_BLEND_OP_REVERSE_SUBTRACT;
		case GL_MIN:					return KS_GPU_BLEND_OP_MIN;
		case GL_MAX:					return KS_GPU_BLEND_OP_MAX;
		default:						return KS_GPU_BLEND_OP_ADD;
	}
}

static ksGpuBlendFactor ksGltf_GetBlendFactor( const int factor )
{
	switch ( factor )
	{
		case GL_ZERO:						return KS_GPU_BLEND_FACTOR_ZERO;
		case GL_ONE:						return KS_GPU_BLEND_FACTOR_ONE;
		case GL_SRC_COLOR:					return KS_GPU_BLEND_FACTOR_SRC_COLOR;
		case GL_ONE_MINUS_SRC_COLOR:		return KS_GPU_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case GL_DST_COLOR:					return KS_GPU_BLEND_FACTOR_DST_COLOR;
		case GL_ONE_MINUS_DST_COLOR:		return KS_GPU_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case GL_SRC_ALPHA:					return KS_GPU_BLEND_FACTOR_SRC_ALPHA;
		case GL_ONE_MINUS_SRC_ALPHA:		return KS_GPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case GL_DST_ALPHA:					return KS_GPU_BLEND_FACTOR_DST_ALPHA;
		case GL_ONE_MINUS_DST_ALPHA:		return KS_GPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case GL_CONSTANT_COLOR:				return KS_GPU_BLEND_FACTOR_CONSTANT_COLOR;
		case GL_ONE_MINUS_CONSTANT_COLOR:	return KS_GPU_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case GL_CONSTANT_ALPHA:				return KS_GPU_BLEND_FACTOR_CONSTANT_ALPHA;
		case GL_ONE_MINUS_CONSTANT_ALPHA:	return KS_GPU_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
		case GL_SRC_ALPHA_SATURATE:			return KS_GPU_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		default:							return KS_GPU_BLEND_FACTOR_ZERO;
	}
}

static int ksGltf_GetVertexAttributeLocation( const ksGltfTechnique * technique, const unsigned char * nameStart, const unsigned char * nameEnd )
{
	for ( int i = 0; i < technique->attributeCount; i++ )
	{
		if ( ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, technique->attributes[i].name ) )
		{
			return technique->attributes[i].location;
		}
	}
	assert( false );
	return 0;
}

static int ksGltf_GetUniformBinding( const ksGltfTechnique * technique, const unsigned char * nameStart, const unsigned char * nameEnd )
{
	for ( int i = 0; i < technique->uniformCount; i++ )
	{
		if ( ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, technique->parms[i].name ) )
		{
			return technique->parms[i].binding;
		}
	}
	assert( false );
	return 0;
}

static void ksGltf_SetUniformStageFlag( const ksGltfTechnique * technique, const unsigned char * nameStart, const unsigned char * nameEnd, const ksGpuProgramStageFlags flag )
{
	for ( int i = 0; i < technique->uniformCount; i++ )
	{
		if ( ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, technique->parms[i].name ) )
		{
			technique->parms[i].stageFlags |= flag;
			return;
		}
	}
	assert( false );
}

#define JOINT_UNIFORM_BUFFER_NAME							"jointUniformBuffer"
#define VIEW_PROJECTION_UNIFORM_BUFFER_NAME					"viewProjectionUniformBuffer"
#define VIEW_PROJECTION_MULTI_VIEW_UNIFORM_BUFFER_NAME		"viewProjectionMultiViewUniformBuffer"

typedef enum
{
	KS_GLSL_CONVERSION_NONE							= 0,
	KS_GLSL_CONVERSION_FLAG_JOINT_BUFFER			= BIT( 0 ),	// KHR_glsl_joint_buffer
	KS_GLSL_CONVERSION_FLAG_VIEW_PROJECTION_BUFFER	= BIT( 1 ),	// KHR_glsl_view_projection_buffer
	KS_GLSL_CONVERSION_FLAG_MULTI_VIEW				= BIT( 2 ),	// KHR_glsl_multi_view
	KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL			= BIT( 3 ),	// KHR_glsl_layout_opengl
	KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN			= BIT( 4 )	// KHR_glsl_layout_vulkan
} ksGlslConversionFlags;

typedef struct ksGltfInOutParm
{
	const unsigned char *	nameStart;
	const unsigned char *	nameEnd;
	size_t					nameLength;
} ksGltfInOutParm;

// Convert a GLSL 1.0 ES glTF shader to a newer (at least 1.3) GLSL version primarily for uniform buffer support.
// Assumes the GLSL 1.0 ES glTF shader does not use any extensions.
// Currently assumes the GLSL 1.0 ES glTF shader does not use any preprocessing.
unsigned char * ksGltf_ConvertShaderGLSL( const unsigned char * source, size_t * sourceSize, const ksGpuProgramStageFlags stage,
											const int conversion,
											const ksGltfTechnique * technique,
											const char * existingSemanticUniforms[],
											const char * newSemanticUniforms[],
											ksGltfInOutParm inOutParms[], int * inOutParmCount )
{
	const bool multiview = ( conversion & KS_GLSL_CONVERSION_FLAG_MULTI_VIEW ) != 0;

	// GLSL version.
	const char * versionString =
					"#version " GLSL_VERSION "\n";
	const size_t versionStringLength =
					strlen( versionString );

	// Default precision.
	const char * precisionString =
					"precision highp float;\n"
					"precision highp int;\n";
	const size_t precisionStringLength =
					strlen( precisionString );

	// Per vertex extension.
	const char * perVertexExtensionString =
					"#extension GL_EXT_shader_io_blocks : enable\n";
	const size_t perVertexExtensionStringLength =
					strlen( perVertexExtensionString );

	// Enhanced layouts extension.
	const char * layoutExtensionString =
					"#extension GL_ARB_enhanced_layouts : enable\n";
	const size_t layoutExtensionStringLength =
					strlen( layoutExtensionString );

	// KHR_glsl_joint_buffer
	const char * jointUniformSemanticString =
					JOINT_UNIFORM_BUFFER_NAME;
	const size_t jointUniformSemanticStringLength =
					strlen( jointUniformSemanticString );
	const char * jointUniformBufferString =
					"uniform %s\n"
					"{\n"
					"	%smat4 %s[256];\n"
					"};\n";
	const size_t jointUniformBufferStringLength =
					strlen( jointUniformBufferString );

	// KHR_glsl_view_projection_buffer
	const char * viewProjectionUniformSemanticString =
					multiview ? VIEW_PROJECTION_MULTI_VIEW_UNIFORM_BUFFER_NAME : VIEW_PROJECTION_UNIFORM_BUFFER_NAME;
	const size_t viewProjectionUniformSemanticStringLength =
					strlen( viewProjectionUniformSemanticString );
	const char * viewProjectionUniformBufferString =
					"uniform %s\n"
					"{\n"
					"	%smat4 %s%s;\n"	// viewMatrix
					"	%smat4 %s%s;\n"	// viewInverseMatrix
					"	%smat4 %s%s;\n"	// projectionMatrix
					"	%smat4 %s%s;\n"	// projectionInverseMatrix
					"};\n";
	const size_t viewProjectionUniformBufferStringLength =
					strlen( viewProjectionUniformBufferString );

	// KHR_glsl_multi_view
	const char * multiviewString = 
					"#define NUM_VIEWS 2\n"
					"#define VIEW_ID gl_ViewID_OVR\n"
					"#extension GL_OVR_multiview2 : require\n"
					"layout( num_views = NUM_VIEWS ) in;\n";
	const size_t multiviewStringLength =
					strlen( multiviewString );
	const char * multiviewArraySizeString =
					multiview ? "[NUM_VIEWS]" : "";
	const char * multiviewArrayIndexString =
					multiview ? "[VIEW_ID]" : "";

	// push constants
	const char * pushConstantStartString =
					"layout( std430, push_constant ) uniform pushConstants\n"
					"{\n";
	const size_t pushConstantStartStringLength =
					strlen( pushConstantStartString );
	const char * pushConstantEndString =
					"} pc;\n";
	const size_t pushConstantEndStringLength =
					strlen( pushConstantEndString );
	const char * pushConstantInstanceName =
					( ( conversion & KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) != 0 ) ? "pc." : "";

	// Vertex and fragment out parameters.
	const char * perVertexString =
					"out gl_PerVertex { vec4 gl_Position; };\n";
	const size_t perVertexStringLength =
					strlen( perVertexString );
	const char * fragColorString =
					"out vec4 fragColor;\n";
	const size_t fragColorStringLength =
					strlen( fragColorString );

	unsigned char * newSource = (unsigned char *) malloc( *sourceSize * 2 +
						versionStringLength +
						precisionStringLength +
						perVertexExtensionStringLength +
						layoutExtensionStringLength +
						jointUniformSemanticStringLength +
						jointUniformBufferStringLength +
						viewProjectionUniformSemanticStringLength +
						viewProjectionUniformBufferStringLength +
						multiviewStringLength +
						pushConstantStartStringLength +
						pushConstantEndStringLength +
						perVertexStringLength +
						fragColorStringLength );
	unsigned char * out = newSource;
	const unsigned char * ptr = source;

	int glslVersion = 100;

	// Get any existing version string.
	{
		const unsigned char * numberSignToken;
		const unsigned char * endOfNumberSignToken = ksLexer_NextToken( source, source, &numberSignToken, NULL );
		if ( ksLexer_CaseSensitiveCompareToken( numberSignToken, endOfNumberSignToken, "#" ) )
		{
			const unsigned char * versionToken;
			const unsigned char * endOfVersionToken = ksLexer_NextToken( source, endOfNumberSignToken, &versionToken, NULL );
			if ( ksLexer_CaseSensitiveCompareToken( versionToken, endOfVersionToken, "version" ) )
			{
				const unsigned char * numberToken;
				ksLexer_NextToken( source, endOfVersionToken, &numberToken, NULL );
				glslVersion = atoi( (const char *)numberToken );
				ptr = ksLexer_SkipUpToEndOfLine( source, endOfVersionToken );
			}
		}
	}

	// Add a new version string.
	out = (unsigned char *)memcpy( out, versionString, versionStringLength ) + versionStringLength;

	// Add a new precision string.
	out = (unsigned char *)memcpy( out, precisionString, precisionStringLength ) + precisionStringLength;

	// Add GL_EXT_shader_io_blocks.
	if ( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX )
	{
		out = (unsigned char *)memcpy( out, perVertexExtensionString, perVertexExtensionStringLength ) + perVertexExtensionStringLength;
	}

	// Add GL_ARB_enhanced_layouts.
	if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL | KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) ) != 0 )
	{
		out = (unsigned char *)memcpy( out, layoutExtensionString, layoutExtensionStringLength ) + layoutExtensionStringLength;
	}

	if ( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX )
	{
		// Optionally add multi-view support.
		if ( ( conversion & KS_GLSL_CONVERSION_FLAG_MULTI_VIEW ) != 0 )
		{
			out = (unsigned char *)memcpy( out, multiviewString, multiviewStringLength ) + multiviewStringLength;
		}

		// Optionally add a joint uniform buffer.
		if ( ( conversion & KS_GLSL_CONVERSION_FLAG_JOINT_BUFFER ) != 0 )
		{
			if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL | KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) ) != 0 )
			{
				const int binding = ksGltf_GetUniformBinding( technique, (const unsigned char *)jointUniformSemanticString, NULL );
				out += sprintf( (char *)out, "layout( std140, binding = %d ) ", binding );
				out += sprintf( (char *)out, jointUniformBufferString, jointUniformSemanticString,
										"layout( offset = 0 ) ", existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_JOINT_ARRAY] );
			}
			else
			{
				out += sprintf( (char *)out, jointUniformBufferString, jointUniformSemanticString,
										"", existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_JOINT_ARRAY] );
			}
			ksGltf_SetUniformStageFlag( technique, (const unsigned char *)jointUniformSemanticString, NULL, stage );
		}

		// Optionally add a view-projection or multi-view uniform buffer.
		if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_VIEW_PROJECTION_BUFFER | KS_GLSL_CONVERSION_FLAG_MULTI_VIEW ) ) != 0 )
		{
			if (	newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW] != NULL ||
					newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE] != NULL ||
					newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION] != NULL ||
					newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE] != NULL )
			{
				if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL | KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) ) != 0 )
				{
					const int binding = ksGltf_GetUniformBinding( technique, (const unsigned char *)viewProjectionUniformSemanticString, NULL );
					out += sprintf( (char *)out, "layout( std140, binding = %d ) ", binding );
					out += sprintf( (char *)out, viewProjectionUniformBufferString, viewProjectionUniformSemanticString,
								multiview ? "layout( offset =   0 ) " : "layout( offset =   0 ) ", newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW], multiviewArraySizeString,
								multiview ? "layout( offset = 128 ) " : "layout( offset =  64 ) ", newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE], multiviewArraySizeString,
 								multiview ? "layout( offset = 256 ) " : "layout( offset = 128 ) ", newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION], multiviewArraySizeString,
								multiview ? "layout( offset = 384 ) " : "layout( offset = 192 ) ", newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE], multiviewArraySizeString );
				}
				else
				{
					out += sprintf( (char *)out, viewProjectionUniformBufferString, viewProjectionUniformSemanticString,
											"", newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW], multiviewArraySizeString,
											"", newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE], multiviewArraySizeString,
 											"", newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION], multiviewArraySizeString,
											"", newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE], multiviewArraySizeString );
				}
				ksGltf_SetUniformStageFlag( technique, (const unsigned char *)viewProjectionUniformSemanticString, NULL, stage );
			}

			// Optionally add 'MODEL' and 'MODELINVERSE' uniforms.
			if ( ( conversion & KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) == 0 )
			{
				if ( newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL] != NULL && existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL] == NULL )
				{
					if ( ( conversion & KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL ) != 0 )
					{
						const int binding = ksGltf_GetUniformBinding( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL], NULL );
						out += sprintf( (char *)out, "layout( location = %d ) ", binding );
					}
					out += sprintf( (char *)out, "uniform mat4 %s;\n", newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL] );

					ksGltf_SetUniformStageFlag( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL], NULL, stage );
				}
				if ( newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] != NULL && existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] == NULL )
				{
					if ( ( conversion & KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL ) != 0 )
					{
						const int binding = ksGltf_GetUniformBinding( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE], NULL );
						out += sprintf( (char *)out, "layout( location = %d ) ", binding );
					}
					out += sprintf( (char *)out, "uniform mat4 %s;\n", newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] );

					ksGltf_SetUniformStageFlag( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE], NULL, stage );
				}
			}
		}
	}

	// Optionally add a push constant block.
	if ( ( conversion & KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) != 0 )
	{
		out = (unsigned char *)memcpy( out, pushConstantStartString, pushConstantStartStringLength ) + pushConstantStartStringLength;

		int offset = 0;
		for ( int i = 0; i < technique->uniformCount; i++ )
		{
			const int size = ksGpuProgramParm_GetPushConstantSize( technique->parms[i].type );
			if ( size > 0 )
			{
				const char * type = ksGpuProgramParm_GetPushConstantGlslType( technique->parms[i].type );
				out += sprintf( (char *)out, "\tlayout( offset = %3d ) %s %s;\n", offset, type, technique->parms[i].name );
				offset += size;
				// For now make all push constants visible to both the vertex and fragment shader so we don't have to select the ones actually used in each.
				ksGltf_SetUniformStageFlag( technique, (const unsigned char *)technique->parms[i].name, NULL, KS_GPU_PROGRAM_STAGE_FLAG_VERTEX | KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT );
			}
		}

		out = (unsigned char *)memcpy( out, pushConstantEndString, pushConstantEndStringLength ) + pushConstantEndStringLength;
	}

	// Add vertex and fragment shader out parameters.
	if ( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX )
	{
		// Add gl_PerVertex string.
		out = (unsigned char *)memcpy( out, perVertexString, perVertexStringLength ) + perVertexStringLength;
	}
	else if ( stage == KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT )
	{
		// gl_FragColor was deprecated in GLSL 1.3 (OpenGL 3.0, OpenGL ES 3.0)
		if ( glslVersion < 130 )
		{
			if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL | KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) ) != 0 )
			{
				out += sprintf( (char *)out, "layout( location = %d ) ", 0 );
			}
			out = (unsigned char *)memcpy( out, fragColorString, fragColorStringLength ) + fragColorStringLength;
		}
	}

	int addSpace = 0;
	int addTabs = 0;
	bool newLine = true;
	while ( ptr[0] != '\0' )
	{
		const unsigned char * token;
		ksTokenInfo tokenInfo;
		ptr = ksLexer_NextToken( source, ptr, &token, &tokenInfo );

		if ( tokenInfo.type == KS_TOKEN_TYPE_NONE )
		{
			continue;
		}

		if ( tokenInfo.type == KS_TOKEN_TYPE_PUNCTUATION )
		{
			if ( ksLexer_CaseSensitiveCompareToken( token, ptr, ";" ) )
			{
				out = (unsigned char *)memcpy( out, ";\n", 2 ) + 2;
				addSpace = 0;
				newLine = true;
				continue;
			}
			if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "{" ) )
			{
				out = (unsigned char *)memcpy( out, "\n{\n", 3 ) + 3;
				addTabs++;
				addSpace = 0;
				newLine = true;
				continue;
			}
			if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "}" ) )
			{
				out = (unsigned char *)memcpy( out, "}\n", 2 ) + 2;
				addTabs--;
				addSpace = 0;
				newLine = true;
				continue;
			}
			if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "." ) )
			{
				out = (unsigned char *)memcpy( out, ".", 1 ) + 1;
				addSpace = 0;
				newLine = false;
				continue;
			}
			if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "," ) )
			{
				out = (unsigned char *)memcpy( out, ",", 1 ) + 1;
				addSpace = 1;
				newLine = false;
				continue;
			}
			if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "[" ) )
			{
				out = (unsigned char *)memcpy( out, "[", 1 ) + 1;
				addSpace = 0;
				newLine = false;
				continue;
			}
			if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "]" ) )
			{
				out = (unsigned char *)memcpy( out, "]", 1 ) + 1;
				addSpace = 0;
				newLine = false;
				continue;
			}
		}

		// Strip any existing precision specifiers.
		if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "precision" ) )
		{
			ptr = ksLexer_SkipUpToIncludingToken( source, ptr, ";" );
			continue;
		}

		// Insert tabs/spaces.
		static const char tabTable[] = { "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" };
		if ( newLine )
		{
			out = (unsigned char *)memcpy( out, tabTable, MIN( addTabs, 16 ) ) + MIN( addTabs, 16 );
		}
		else if ( addSpace )
		{
			out = (unsigned char *)memcpy( out, " ", 1 ) + 1;
		}
		addSpace = 1;
		newLine = false;

		if ( tokenInfo.type == KS_TOKEN_TYPE_NAME )
		{
			// Convert the vertex and fragment shader in-out parameters.
			if ( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX )
			{
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "attribute" ) )
				{
					if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL | KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) ) != 0 )
					{
						const unsigned char * typeStart;
						const unsigned char * typeEnd = ksLexer_NextToken( source, ptr, &typeStart, NULL );
						const unsigned char * nameStart;
						const unsigned char * nameEnd = ksLexer_NextToken( source, typeEnd, &nameStart, NULL );
						out += sprintf( (char *)out, "layout( location = %d ) ", ksGltf_GetVertexAttributeLocation( technique, nameStart, nameEnd ) );
					}
					out = (unsigned char *)memcpy( out, "in", 2 ) + 2;
					continue;
				}
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "varying" ) )
				{
					if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL | KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) ) != 0 )
					{
						const unsigned char * typeStart;
						const unsigned char * typeEnd = ksLexer_NextToken( source, ptr, &typeStart, NULL );
						const unsigned char * nameStart;
						const unsigned char * nameEnd = ksLexer_NextToken( source, typeEnd, &nameStart, NULL );
						out += sprintf( (char *)out, "layout( location = %d ) ", *inOutParmCount );
						inOutParms[(*inOutParmCount)].nameStart = nameStart;
						inOutParms[(*inOutParmCount)].nameEnd = nameEnd;
						inOutParms[(*inOutParmCount)].nameLength = nameEnd - nameStart;
						(*inOutParmCount)++;
					}
					out = (unsigned char *)memcpy( out, "out", 3 ) + 3;
					continue;
				}
			}
			else if ( stage == KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT )
			{
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "varying" ) )
				{
					if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL | KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) ) != 0 )
					{
						const unsigned char * typeStart;
						const unsigned char * typeEnd = ksLexer_NextToken( source, ptr, &typeStart, NULL );
						const unsigned char * nameStart;
						const unsigned char * nameEnd = ksLexer_NextToken( source, typeEnd, &nameStart, NULL );
						const size_t nameLength = nameEnd - nameStart;
						int location = -1;
						for ( int i = 0; i < *inOutParmCount; i++ )
						{
							if ( inOutParms[i].nameLength == nameLength &&
									strncmp( (const char *)inOutParms[i].nameStart, (const char *)nameStart, nameLength ) == 0 )
							{
								location = i;
								break;
							}
						}
						assert( location >= 0 );
						out += sprintf( (char *)out, "layout( location = %d ) ", location );
					}
					out = (unsigned char *)memcpy( out, "in", 2 ) + 2;
					continue;
				}
			}

			// Strip uniforms that are no longer used, set stage flags and optionally add layout qualifiers.
			if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "uniform" ) )
			{
				const unsigned char * typeStart;
				const unsigned char * typeEnd = ksLexer_NextToken( source, ptr, &typeStart, NULL );
				const unsigned char * nameStart;
				const unsigned char * nameEnd = ksLexer_NextToken( source, typeEnd, &nameStart, NULL );

				// Strip uniforms that are no longer used.
				if ( ksLexer_CaseSensitiveCompareToken( typeStart, typeEnd, "mat3" ) )
				{
					if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_VIEW_PROJECTION_BUFFER | KS_GLSL_CONVERSION_FLAG_MULTI_VIEW ) ) != 0 )
					{
						// Strip uniforms that are replaced by the view and projection matrices from the uniform buffer.
						if (	ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE_TRANSPOSE] ) ||
								ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE_TRANSPOSE] ) )
						{
							assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
							ptr = ksLexer_SkipUpToIncludingToken( source, nameEnd, ";" );
							addSpace = 0;
							continue;
						}
					}
				}
				else if ( ksLexer_CaseSensitiveCompareToken( typeStart, typeEnd, "mat4" ) )
				{
					if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_VIEW_PROJECTION_BUFFER | KS_GLSL_CONVERSION_FLAG_MULTI_VIEW ) ) != 0 )
					{
						// Strip uniforms that are replaced by the view and projection matrices from the uniform buffer.
						if (	ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW] ) ||
								ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE] ) ||
								ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION] ) ||
								ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE] ) ||
								ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW] ) ||
								ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE] ) ||
								ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION] ) )
						{
							assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
							ptr = ksLexer_SkipUpToIncludingToken( source, nameEnd, ";" );
							addSpace = 0;
							continue;
						}
					}

					if ( ( conversion & KS_GLSL_CONVERSION_FLAG_JOINT_BUFFER ) != 0 )
					{
						// Strip the joint uniform array.
						if ( ksLexer_CaseSensitiveCompareToken( nameStart, nameEnd, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_JOINT_ARRAY] ) )
						{
							assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
							ptr = ksLexer_SkipUpToIncludingToken( source, nameEnd, ";" );
							addSpace = 0;
							continue;
						}
					}
				}

				ksGltf_SetUniformStageFlag( technique, nameStart, nameEnd, stage );

				// Optionally add layout qualifiers.
				if ( ( conversion & KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL ) != 0 )
				{
					out += sprintf( (char *)out, "layout( location = %d ) ", ksGltf_GetUniformBinding( technique, nameStart, nameEnd ) );
				}
				else if ( ( conversion & KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) != 0 )
				{
					if ( ksLexer_CaseSensitiveCompareToken( typeStart, typeEnd, "sampler2D" ) ||
						ksLexer_CaseSensitiveCompareToken( typeStart, typeEnd, "samplerCube" ) )
					{
						out += sprintf( (char *)out, "layout( location = %d ) ", ksGltf_GetUniformBinding( technique, nameStart, nameEnd ) );
					}
					else
					{
						// Push constants are declared in the push constant block.
						ptr = ksLexer_SkipUpToIncludingToken( source, nameEnd, ";" );
						addSpace = 0;
						continue;
					}
				}

				out = (unsigned char *)memcpy( out, token, ptr - token ) + ( ptr - token );
				continue;
			}

			// Optionally replace uniform usage.
			if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_VIEW_PROJECTION_BUFFER | KS_GLSL_CONVERSION_FLAG_MULTI_VIEW ) ) != 0 )
			{
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE_TRANSPOSE] ) )
				{
					assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
					out += sprintf( (char *)out, "transpose( mat3( %s%s ) )",
							pushConstantInstanceName, newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] );
					ksGltf_SetUniformStageFlag( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE], NULL, stage );
					continue;
				}
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW] ) )
				{
					assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
					out += sprintf( (char *)out, "%s%s",
							newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW], multiviewArrayIndexString );
					continue;
				}
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE] ) )
				{
					assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
					out += sprintf( (char *)out, "%s%s",
							newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE], multiviewArrayIndexString );
					continue;
				}
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION] ) )
				{
					assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
					out += sprintf( (char *)out, "%s%s",
							newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION], multiviewArrayIndexString );
					continue;
				}
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE] ) )
				{
					assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
					out += sprintf( (char *)out, "%s%s",
							newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE], multiviewArrayIndexString );
					continue;
				}
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW] ) )
				{
					assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
					out += sprintf( (char *)out, "%s%s * %s%s",
							newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW], multiviewArrayIndexString,
							pushConstantInstanceName, newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL] );
					ksGltf_SetUniformStageFlag( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL], NULL, stage );
					continue;
				}
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE] ) )
				{
					assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
					out += sprintf( (char *)out, "%s%s * %s%s",
							newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE], multiviewArrayIndexString,
							pushConstantInstanceName, newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] );
					ksGltf_SetUniformStageFlag( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE], NULL, stage );
					continue;
				}
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE_TRANSPOSE] ) )
				{
					assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
					out += sprintf( (char *)out, "transpose( mat3( %s%s ) ) * transpose( mat3( %s%s ) )",
							newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE], multiviewArrayIndexString,
							pushConstantInstanceName, newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] );
					ksGltf_SetUniformStageFlag( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE], NULL, stage );
					continue;
				}
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION] ) )
				{
					assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
					out += sprintf( (char *)out, "%s%s * %s%s * %s%s",
							newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION], multiviewArrayIndexString,
							newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW], multiviewArrayIndexString,
							pushConstantInstanceName, newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL] );
					ksGltf_SetUniformStageFlag( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL], NULL, stage );
					continue;
				}
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, existingSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION_INVERSE] ) )
				{
					assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
					out += sprintf( (char *)out, "%s%s * %s%s * %s%s",
							newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE], multiviewArrayIndexString,
							newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE], multiviewArrayIndexString,
							pushConstantInstanceName, newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] );
					ksGltf_SetUniformStageFlag( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE], NULL, stage );
					continue;
				}
			}

			// Pre-append the push constant block instance name to push constants names.
			if ( ( conversion & KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) != 0 )
			{
				bool found = false;
				for ( int i = 0; i < technique->uniformCount; i++ )
				{
					const int size = ksGpuProgramParm_GetPushConstantSize( technique->parms[i].type );
					if ( size > 0 )
					{
						if ( ksLexer_CaseSensitiveCompareToken( token, ptr, technique->parms[i].name ) )
						{
							out += sprintf( (char *)out, "%s%s", pushConstantInstanceName, technique->parms[i].name );
							found = true;
							break;
						}
					}
				}
				if ( found )
				{
					ksGltf_SetUniformStageFlag( technique, token, ptr, stage );
					continue;
				}
			}

			// Replace gl_FragColor.
			if ( stage == KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT )
			{
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "gl_FragColor" ) )
				{
					out = (unsigned char *)memcpy( out, "fragColor", 9 ) + 9;
					continue;
				}
			}

			if (	ksLexer_CaseSensitiveCompareToken( token, ptr, "texture1D" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "texture2D" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "texture3D" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "textureCube" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "shadow1D" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "shadow2D" ) )
			{
				out = (unsigned char *)memcpy( out, "texture", 7 ) + 7;
				continue;
			}

			if (	ksLexer_CaseSensitiveCompareToken( token, ptr, "texture1DProj" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "texture2DProj" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "texture3DProj" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "shadow1DProj" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "shadow2DProj" ) )
			{
				out = (unsigned char *)memcpy( out, "textureProj", 11 ) + 11;
				continue;
			}

			if (	ksLexer_CaseSensitiveCompareToken( token, ptr, "texture1DLod" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "texture2DLod" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "texture3DLod" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "textureCubeLod" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "shadow1DLod" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "shadow2DLod" ) )
			{
				out = (unsigned char *)memcpy( out, "textureLod", 10 ) + 10;
				continue;
			}

			if (	ksLexer_CaseSensitiveCompareToken( token, ptr, "texture1DProjLod" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "texture2DProjLod" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "texture3DProjLod" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "shadow1DProjLod" ) ||
					ksLexer_CaseSensitiveCompareToken( token, ptr, "shadow2DProjLod" ) )
			{
				out = (unsigned char *)memcpy( out, "textureProjLod", 14 ) + 14;
				continue;
			}
		}

		out = (unsigned char *)memcpy( out, token, ptr - token ) + ( ptr - token );
	}

	*out++ = '\0';
	*sourceSize = ( out - newSource );

	return newSource;
}

void ksGltf_CreateTechniqueProgram( ksGpuContext * context, ksGltfTechnique * technique, const ksGltfProgram * program,
								const int conversion, const char * semanticUniforms[] )
{
	if ( conversion != KS_GLSL_CONVERSION_NONE )
	{
		const char * newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MAX] = { 0 };

		// Update / replace technique uniforms.
		{
			// At most three new uniforms are added.
			ksGpuProgramParm * newParms = (ksGpuProgramParm *) calloc( technique->uniformCount + 3, sizeof( ksGpuProgramParm ) );
			ksGltfUniform * newUniforms = (ksGltfUniform *) calloc( technique->uniformCount + 3, sizeof( ksGltfUniform ) );
			int newUniformCount = 0;

			if ( ( conversion & KS_GLSL_CONVERSION_FLAG_JOINT_BUFFER ) != 0 )
			{
				// Optionally add a joint uniform buffer.
				if ( semanticUniforms[GLTF_UNIFORM_SEMANTIC_JOINT_ARRAY] != NULL )
				{
					newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_JOINT_BUFFER] = JOINT_UNIFORM_BUFFER_NAME;

					newParms[newUniformCount].stageFlags = 0;	// Set when converting the shader.
					newParms[newUniformCount].type = KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM;
					newParms[newUniformCount].access = KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY;
					newParms[newUniformCount].index = newUniformCount;
					newParms[newUniformCount].name = ksGltf_strdup( JOINT_UNIFORM_BUFFER_NAME );
					newParms[newUniformCount].binding = 0;	// Set when adding layout qualitifiers.

					newUniforms[newUniformCount].name = ksGltf_strdup( JOINT_UNIFORM_BUFFER_NAME );
					newUniforms[newUniformCount].semantic = GLTF_UNIFORM_SEMANTIC_JOINT_BUFFER;
					newUniforms[newUniformCount].type = KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM;
					newUniforms[newUniformCount].index = newUniformCount;

					newUniformCount++;
				}
			}

			if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_VIEW_PROJECTION_BUFFER | KS_GLSL_CONVERSION_FLAG_MULTI_VIEW ) ) != 0 )
			{
				const bool multiview = ( conversion & KS_GLSL_CONVERSION_FLAG_MULTI_VIEW ) != 0;

				// Optionally add a view-projection or multi-view uniform buffer.
				if (	semanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE_TRANSPOSE] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION_INVERSE] != NULL )
				{
					newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW]					=	semanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW] != NULL ?
																						semanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW] : "u_viewMatrix";
					newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE]			=	semanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE] != NULL ?
																						semanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE] : "u_viewInverseMatrix";
					newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION]			=	semanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION] != NULL ?
																						semanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION] : "u_projectionMatrix";
					newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE]	=	semanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE] != NULL ?
																						semanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE] : "u_projectionInverseMatrix";

					newParms[newUniformCount].stageFlags = 0;	// Set when converting the shader.
					newParms[newUniformCount].type = KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM;
					newParms[newUniformCount].access = KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY;
					newParms[newUniformCount].index = newUniformCount;
					newParms[newUniformCount].name = ksGltf_strdup( multiview ? VIEW_PROJECTION_MULTI_VIEW_UNIFORM_BUFFER_NAME : VIEW_PROJECTION_UNIFORM_BUFFER_NAME );
					newParms[newUniformCount].binding = 0;	// Set when adding layout qualitifiers.

					newUniforms[newUniformCount].name = ksGltf_strdup( multiview ? VIEW_PROJECTION_MULTI_VIEW_UNIFORM_BUFFER_NAME : VIEW_PROJECTION_UNIFORM_BUFFER_NAME );
					newUniforms[newUniformCount].semantic = multiview ? GLTF_UNIFORM_SEMANTIC_VIEW_PROJECTION_MULTI_VIEW_BUFFER : GLTF_UNIFORM_SEMANTIC_VIEW_PROJECTION_BUFFER;
					newUniforms[newUniformCount].type = KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM;
					newUniforms[newUniformCount].index = newUniformCount;

					newUniformCount++;
				}

				// Optionally add a model matrix uniform.
				if (	semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION] != NULL )
				{
					newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL]	=	semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL] != NULL ?
																			semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL] : "u_modelMatrix";
					if ( semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL] == NULL )
					{
						newParms[newUniformCount].stageFlags = 0;	// Set when converting the shader.
						newParms[newUniformCount].type = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4;
						newParms[newUniformCount].access = KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY;
						newParms[newUniformCount].index = newUniformCount;
						newParms[newUniformCount].name = ksGltf_strdup( newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL] );
						newParms[newUniformCount].binding = 0;	// Set when adding layout qualitifiers.

						newUniforms[newUniformCount].name = ksGltf_strdup( newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL] );
						newUniforms[newUniformCount].semantic = GLTF_UNIFORM_SEMANTIC_MODEL;
						newUniforms[newUniformCount].type = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4;
						newUniforms[newUniformCount].index = newUniformCount;

						newUniformCount++;
					}
				}

				// Optionally add an inverse model matrix uniform.
				if (	semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE_TRANSPOSE] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE_TRANSPOSE] != NULL ||
						semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION_INVERSE] != NULL )
				{
					newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE]	=	semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] != NULL ?
																					semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] : "u_modelInverseMatrix";
					if ( semanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] == NULL )
					{
						newParms[newUniformCount].stageFlags = 0;	// Set when converting the shader.
						newParms[newUniformCount].type = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4;
						newParms[newUniformCount].access = KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY;
						newParms[newUniformCount].index = newUniformCount;
						newParms[newUniformCount].name = ksGltf_strdup( newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] );
						newParms[newUniformCount].binding = 0;	// Set when adding layout qualitifiers.

						newUniforms[newUniformCount].name = ksGltf_strdup( newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE] );
						newUniforms[newUniformCount].semantic = GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE;
						newUniforms[newUniformCount].type = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4;
						newUniforms[newUniformCount].index = newUniformCount;

						newUniformCount++;
					}
				}
			}

			// Maintain any uniforms that are still used after the conversion to uniform buffers.
			for ( int uniformIndex = 0; uniformIndex < technique->uniformCount; uniformIndex++ )
			{
				if ( ( conversion & KS_GLSL_CONVERSION_FLAG_JOINT_BUFFER ) != 0 )
				{
					if ( technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_JOINT_ARRAY )
					{
						free( (void *)technique->parms[uniformIndex].name );
						free( technique->uniforms[uniformIndex].name );
						continue;
					}
				}
				if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_VIEW_PROJECTION_BUFFER | KS_GLSL_CONVERSION_FLAG_MULTI_VIEW ) ) != 0 )
				{
					if (	technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE_TRANSPOSE ||
							technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_VIEW ||
							technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE ||
							technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_PROJECTION ||
							technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE ||
							technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_MODEL_VIEW ||
							technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE ||
							technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE_TRANSPOSE ||
							technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION ||
							technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION_INVERSE )
					{
						free( (void *)technique->parms[uniformIndex].name );
						free( technique->uniforms[uniformIndex].name );
						continue;
					}
				}

				newParms[newUniformCount].stageFlags = 0;	// Set when converting the shader.
				newParms[newUniformCount].type = technique->parms[uniformIndex].type;
				newParms[newUniformCount].access = technique->parms[uniformIndex].access;
				newParms[newUniformCount].index = newUniformCount;
				newParms[newUniformCount].name = technique->parms[uniformIndex].name;
				newParms[newUniformCount].binding = 0;	// Set when adding layout qualitifiers.

				newUniforms[newUniformCount].name = technique->uniforms[uniformIndex].name;
				newUniforms[newUniformCount].semantic = technique->uniforms[uniformIndex].semantic;
				newUniforms[newUniformCount].type = technique->uniforms[uniformIndex].type;
				newUniforms[newUniformCount].index = newUniformCount;

				newUniformCount++;
			}

			if ( ( conversion & KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL ) != 0 )
			{
				// Set OpenGL layout bindings / locations.
				int numSampledTextureBindings = 0;
				int numStorageTextureBindings = 0;
				int numUniformBufferBindings = 0;
				int numStorageBufferBindings = 0;
				int numUniformLocations = 0;

				for ( int uniformIndex = 0; uniformIndex < newUniformCount; uniformIndex++ )
				{
					if ( newParms[uniformIndex].type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED )
					{
						newParms[uniformIndex].binding = numSampledTextureBindings++;
					}
					else if ( newParms[uniformIndex].type == KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE )
					{
						newParms[uniformIndex].binding = numStorageTextureBindings++;
					}
					else if ( newParms[uniformIndex].type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM )
					{
						newParms[uniformIndex].binding = numUniformBufferBindings++;
					}
					else if ( newParms[uniformIndex].type == KS_GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
					{
						newParms[uniformIndex].binding = numStorageBufferBindings++;
					}
					else
					{
						newParms[uniformIndex].binding = numUniformLocations++;
					}
				}
			}
			else if ( ( conversion & KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) != 0 )
			{
				// Set Vulkan layout bindings / push constant offsets.
				int numOpaqueBindings = 0;
				int pushConstantOffset = 0;

				for ( int uniformIndex = 0; uniformIndex < newUniformCount; uniformIndex++ )
				{
					if ( ksGpuProgramParm_IsOpaqueBinding( newParms[uniformIndex].type ) )
					{
						newParms[uniformIndex].binding = numOpaqueBindings++;
					}
					else
					{
						newParms[uniformIndex].binding = pushConstantOffset;
						pushConstantOffset += ksGpuProgramParm_GetPushConstantSize( newParms[uniformIndex].type );
					}
				}
			}

			free( technique->parms );
			free( technique->uniforms );

			technique->parms = newParms;
			technique->uniforms = newUniforms;
			technique->uniformCount = newUniformCount;
		}

		ksGltfInOutParm inOutParms[16];
		int inOutParmCount = 0;

		size_t vertexSourceSize = program->vertexSourceSize;
		size_t fragmentSourceSize = program->fragmentSourceSize;

		unsigned char * vertexSource = ksGltf_ConvertShaderGLSL( program->vertexSource, &vertexSourceSize, KS_GPU_PROGRAM_STAGE_FLAG_VERTEX, conversion,
																	technique, semanticUniforms, newSemanticUniforms, inOutParms, &inOutParmCount );
		unsigned char * fragmentSource = ksGltf_ConvertShaderGLSL( program->fragmentSource, &fragmentSourceSize, KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT, conversion,
																	technique, semanticUniforms, newSemanticUniforms, inOutParms, &inOutParmCount );

		for ( int uniformIndex = 0; uniformIndex < technique->uniformCount; uniformIndex++ )
		{
			assert( technique->parms[uniformIndex].stageFlags != 0 );
		}

		ksGpuGraphicsProgram_Create( context, &technique->program,
									vertexSource, vertexSourceSize,
									fragmentSource, fragmentSourceSize,
									technique->parms, technique->uniformCount,
									technique->vertexAttributeLayout, technique->vertexAttribsFlags );

		free( vertexSource );
		free( fragmentSource );
	}
	else
	{
		ksGpuGraphicsProgram_Create( context, &technique->program,
									program->vertexSource, program->vertexSourceSize,
									program->fragmentSource, program->fragmentSourceSize,
									technique->parms, technique->uniformCount,
									technique->vertexAttributeLayout, technique->vertexAttribsFlags );
	}
}

// Sort the nodes such that parents come before their children and every sub-tree is a contiguous sequence of nodes.
// Note that the node graph must be acyclic and no node may be a direct or indirect descendant of more than one node.
static void ksGltf_SortNodes( ksGltfNode * nodes, const int nodeCount )
{
	ksGltfNode * nodeStack = (ksGltfNode *) malloc( nodeCount * sizeof( ksGltfNode ) );
	int stackSize = 0;
	int stackOffset = 0;
	for ( int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++ )
	{
		bool foundParent = false;
		for ( int nodeSearchIndex = 0; nodeSearchIndex < nodeCount; nodeSearchIndex++ )
		{
			for ( int childIndex = 0; childIndex < nodes[nodeSearchIndex].childCount; childIndex++ )
			{
				if ( strcmp( nodes[nodeSearchIndex].childNames[childIndex], nodes[nodeIndex].name ) == 0 )
				{
					foundParent = true;
					break;
				}
			}
		}
		if ( !foundParent )
		{
			const int subTreeStartOffset = stackSize;
			nodeStack[stackSize++] = nodes[nodeIndex];
			while( stackOffset < stackSize )
			{
				const ksGltfNode * node = &nodeStack[stackOffset++];
				for ( int childIndex = 0; childIndex < node->childCount; childIndex++ )
				{
					for ( int nodeSearchIndex = 0; nodeSearchIndex < nodeCount; nodeSearchIndex++ )
					{
						if ( strcmp( node->childNames[childIndex], nodes[nodeSearchIndex].name ) == 0 )
						{
							assert( stackSize < nodeCount );
							nodeStack[stackSize++] = nodes[nodeSearchIndex];
							break;
						}
					}
				}
			}
			for ( int updateNodeIndex = subTreeStartOffset; updateNodeIndex < stackSize; updateNodeIndex++ )
			{
				nodeStack[updateNodeIndex].subTreeNodeCount = stackSize - updateNodeIndex;
			}
		}
	}
	assert( stackSize == nodeCount );
	memcpy( nodes, nodeStack, nodeCount * sizeof( nodes[0] ) );
	free( nodeStack );
}

static bool ksGltfScene_CreateFromFile( ksGpuContext * context, ksGltfScene * scene, ksSceneSettings * settings, ksGpuRenderPass * renderPass )
{
	const ksNanoseconds t0 = GetTimeNanoseconds();

	memset( scene, 0, sizeof( ksGltfScene ) );

	// Based on a GL_MAX_UNIFORM_BLOCK_SIZE of 16384 on the ARM Mali.
	const int MAX_JOINTS = 16384 / sizeof( ksMatrix4x4f );

	Json_t * rootNode = Json_Create();

	//
	// Load either the glTF .json or .glb
	//

	unsigned char * binaryBuffer = NULL;
	size_t binaryBufferLength = 0;

	const char * fileName = settings->glTF;
	const size_t fileNameLength = strlen( fileName );
	if ( fileNameLength > 4 && strcmp( &fileName[fileNameLength - 4], ".glb" ) == 0 )
	{
		FILE * binaryFile = fopen( fileName, "rb" );
		if ( binaryFile == NULL )
		{
			Json_Destroy( rootNode );
			Error( "Failed to open %s", fileName );
			return false;
		}

		ksGltfBinaryHeader header;
		if ( fread( &header, 1, sizeof( header ), binaryFile ) != sizeof( header ) )
		{
			fclose( binaryFile );
			Json_Destroy( rootNode );
			Error( "Failed to read glTF binary header %s", fileName );
			return false;
		}

		if ( header.magic != GLTF_BINARY_MAGIC || header.version != GLTF_BINARY_VERSION || header.contentFormat != GLTF_BINARY_CONTENT_FORMAT )
		{
			fclose( binaryFile );
			Json_Destroy( rootNode );
			Error( "Invalid glTF binary header %s", fileName );
			return false;
		}

		char * content = (char *) malloc( header.contentLength + 1 );
		if ( fread( content, 1, header.contentLength, binaryFile ) != header.contentLength )
		{
			free( content );
			fclose( binaryFile );
			Json_Destroy( rootNode );
			Error( "Failed to read binary glTF content %s", fileName );
			return false;
		}
		content[header.contentLength] = '\0';	// make sure the buffer is zero terminated

		const char * errorString = "";
		if ( !Json_ReadFromBuffer( rootNode, content, &errorString ) )
		{
			free( content );
			fclose( binaryFile );
			Json_Destroy( rootNode );
			Error( "Failed to load %s (%s)", fileName, errorString );
			return false;
		}

		free( content );

		assert( ( ( sizeof( header ) + header.contentLength ) & 3 ) == 0 );

		binaryBufferLength = header.length - header.contentLength - sizeof( header );
		binaryBuffer = (unsigned char *) malloc( binaryBufferLength );
		if ( fread( binaryBuffer, 1, binaryBufferLength, binaryFile ) != binaryBufferLength )
		{
			free( binaryBuffer );
			fclose( binaryFile );
			Json_Destroy( rootNode );
			Error( "Failed to read binary glTF content %s", fileName );
			return false;
		}

		fclose( binaryFile );
	}
	else
	{
		const char * errorString = "";
		if ( !Json_ReadFromFile( rootNode, fileName, &errorString ) )
		{
			Json_Destroy( rootNode );
			Error( "Failed to load %s (%s)", fileName, errorString );
			return false;
		}
	}

	//
	// Check the glTF JSON version.
	//

	const Json_t * asset = Json_GetMemberByName( rootNode, "asset" );
	const char * version = Json_GetString( Json_GetMemberByName( asset, "version" ), "1.0" );
	if ( strcmp( version, GLTF_JSON_VERSION ) != 0 )
	{
		Json_Destroy( rootNode );
		Error( "glTF version is %s instead of %s", version, GLTF_JSON_VERSION );
		return false;
	}

	//
	// glTF buffers
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * buffers = Json_GetMemberByName( rootNode, "buffers" );
		scene->bufferCount = Json_GetMemberCount( buffers );
		scene->buffers = (ksGltfBuffer *) calloc( scene->bufferCount, sizeof( ksGltfBuffer ) );
		for ( int bufferIndex = 0; bufferIndex < scene->bufferCount; bufferIndex++ )
		{
			const Json_t * buffer = Json_GetMemberByIndex( buffers, bufferIndex );
			scene->buffers[bufferIndex].name = ksGltf_strdup( Json_GetMemberName( buffer ) );
			scene->buffers[bufferIndex].byteLength = (size_t) Json_GetUint64( Json_GetMemberByName( buffer, "byteLength" ), 0 );
			scene->buffers[bufferIndex].type = ksGltf_strdup( Json_GetString( Json_GetMemberByName( buffer, "type" ), "" ) );
			if ( strcmp( scene->buffers[bufferIndex].name, "binary_glTF" ) == 0 )
			{
				assert( scene->buffers[bufferIndex].byteLength == binaryBufferLength );
				scene->buffers[bufferIndex].bufferData = binaryBuffer;
			}
			else
			{
				scene->buffers[bufferIndex].bufferData = ksGltf_ReadUri( binaryBuffer, Json_GetString( Json_GetMemberByName( buffer, "uri" ), "" ), NULL );
			}
			assert( scene->buffers[bufferIndex].name[0] != '\0' );
			assert( scene->buffers[bufferIndex].byteLength != 0 );
			assert( scene->buffers[bufferIndex].bufferData != NULL );
		}
		ksGltf_CreateBufferNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load buffers\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF bufferViews
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * bufferViews = Json_GetMemberByName( rootNode, "bufferViews" );
		scene->bufferViewCount = Json_GetMemberCount( bufferViews );
		scene->bufferViews = (ksGltfBufferView *) calloc( scene->bufferViewCount, sizeof( ksGltfBufferView ) );
		for ( int bufferViewIndex = 0; bufferViewIndex < scene->bufferViewCount; bufferViewIndex++ )
		{
			const Json_t * view = Json_GetMemberByIndex( bufferViews, bufferViewIndex );
			scene->bufferViews[bufferViewIndex].name = ksGltf_strdup( Json_GetMemberName( view ) );
			scene->bufferViews[bufferViewIndex].buffer = ksGltf_GetBufferByName( scene, Json_GetString( Json_GetMemberByName( view, "buffer" ), "" ) );
			scene->bufferViews[bufferViewIndex].byteOffset = (size_t) Json_GetUint64( Json_GetMemberByName( view, "byteOffset" ), 0 );
			scene->bufferViews[bufferViewIndex].byteLength = (size_t) Json_GetUint64( Json_GetMemberByName( view, "byteLength" ), 0 );
			scene->bufferViews[bufferViewIndex].target = Json_GetUint16( Json_GetMemberByName( view, "target" ), 0 );
			assert( scene->bufferViews[bufferViewIndex].name[0] != '\0' );
			assert( scene->bufferViews[bufferViewIndex].buffer != NULL );
			assert( scene->bufferViews[bufferViewIndex].byteLength != 0 );
			assert( scene->bufferViews[bufferViewIndex].byteOffset +
					scene->bufferViews[bufferViewIndex].byteLength <=
					scene->bufferViews[bufferViewIndex].buffer->byteLength );
		}
		ksGltf_CreateBufferViewNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load buffer views\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF accessors
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * accessors = Json_GetMemberByName( rootNode, "accessors" );
		scene->accessorCount = Json_GetMemberCount( accessors );
		scene->accessors = (ksGltfAccessor *) calloc( scene->accessorCount, sizeof( ksGltfAccessor ) );
		for ( int accessorIndex = 0; accessorIndex < scene->accessorCount; accessorIndex++ )
		{
			const Json_t * access = Json_GetMemberByIndex( accessors, accessorIndex );
			scene->accessors[accessorIndex].name = ksGltf_strdup( Json_GetMemberName( access ) );
			scene->accessors[accessorIndex].bufferView = ksGltf_GetBufferViewByName( scene, Json_GetString( Json_GetMemberByName( access, "bufferView" ), "" ) );
			scene->accessors[accessorIndex].byteOffset = (size_t) Json_GetUint64( Json_GetMemberByName( access, "byteOffset" ), 0 );
			scene->accessors[accessorIndex].byteStride = (size_t) Json_GetUint64( Json_GetMemberByName( access, "byteStride" ), 0 );
			scene->accessors[accessorIndex].componentType = Json_GetUint16( Json_GetMemberByName( access, "componentType" ), 0 );
			scene->accessors[accessorIndex].count = Json_GetInt32( Json_GetMemberByName( access, "count" ), 0 );
			scene->accessors[accessorIndex].type =  ksGltf_strdup( Json_GetString( Json_GetMemberByName( access, "type" ), "" ) );
			const Json_t * min = Json_GetMemberByName( access, "min" );
			const Json_t * max = Json_GetMemberByName( access, "max" );
			if ( min != NULL && max != NULL )
			{
				int componentCount = 0;
				if ( strcmp( scene->accessors[accessorIndex].type, "SCALAR" ) == 0 ) { componentCount = 1; }
				else if ( strcmp( scene->accessors[accessorIndex].type, "VEC2" ) == 0 ) { componentCount = 2; }
				else if ( strcmp( scene->accessors[accessorIndex].type, "VEC3" ) == 0 ) { componentCount = 3; }
				else if ( strcmp( scene->accessors[accessorIndex].type, "VEC4" ) == 0 ) { componentCount = 4; }
				else if ( strcmp( scene->accessors[accessorIndex].type, "MAT2" ) == 0 ) { componentCount = 4; }
				else if ( strcmp( scene->accessors[accessorIndex].type, "MAT3" ) == 0 ) { componentCount = 9; }
				else if ( strcmp( scene->accessors[accessorIndex].type, "MAT4" ) == 0 ) { componentCount = 16; }

				switch ( scene->accessors[accessorIndex].componentType )
				{
					case GL_BYTE:
					case GL_UNSIGNED_BYTE:
					case GL_SHORT:
					case GL_UNSIGNED_SHORT:
						ksGltf_ParseIntArray( scene->accessors[accessorIndex].intMin, componentCount, min );
						ksGltf_ParseIntArray( scene->accessors[accessorIndex].intMax, componentCount, max );
						break;
					case GL_FLOAT:
						ksGltf_ParseFloatArray( scene->accessors[accessorIndex].floatMin, componentCount, min );
						ksGltf_ParseFloatArray( scene->accessors[accessorIndex].floatMax, componentCount, max );
						break;
				}
			}
			assert( scene->accessors[accessorIndex].name[0] != '\0' );
			assert( scene->accessors[accessorIndex].bufferView != NULL );
			assert( scene->accessors[accessorIndex].componentType != 0 );
			assert( scene->accessors[accessorIndex].count != 0 );
			assert( scene->accessors[accessorIndex].type[0] != '\0' );
			assert( scene->accessors[accessorIndex].byteOffset +
					scene->accessors[accessorIndex].count *
					scene->accessors[accessorIndex].byteStride <=
					scene->accessors[accessorIndex].bufferView->byteLength );
		}
		ksGltf_CreateAccessorNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load accessors\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF images
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * images = Json_GetMemberByName( rootNode, "images" );
		scene->imageCount = Json_GetMemberCount( images );
		scene->images = (ksGltfImage *) calloc( scene->imageCount, sizeof( ksGltfImage ) );
		for ( int imageIndex = 0; imageIndex < scene->imageCount; imageIndex++ )
		{
			const Json_t * image = Json_GetMemberByIndex( images, imageIndex );
			scene->images[imageIndex].name = ksGltf_strdup( Json_GetMemberName( image ) );
			char * baseUri = ksGltf_ParseUri( scene, image, "uri" );

			assert( scene->images[imageIndex].name[0] != '\0' );
			assert( baseUri != '\0' );

			const Json_t * extensions = Json_GetMemberByName( image, "extensions" );
			if ( extensions != NULL )
			{
				const Json_t * KHR_image_versions = Json_GetMemberByName( extensions, "KHR_image_versions" );
				if ( KHR_image_versions != NULL )
				{
					const Json_t * versions = Json_GetMemberByName( KHR_image_versions, "versions" );
					const int versionCount = Json_GetMemberCount( versions );
					scene->images[imageIndex].versions = (ksGltfImageVersion *) calloc( versionCount + 1, sizeof( ksGltfImageVersion ) );
					scene->images[imageIndex].versionCount = versionCount + 1;
					for ( int versionIndex = 0; versionIndex < versionCount; versionIndex++ )
					{
						const Json_t * v = Json_GetMemberByIndex( versions, versionIndex );
						scene->images[imageIndex].versions[versionIndex].container = ksGltf_strdup( Json_GetString( Json_GetMemberByName( v, "container" ), "" ) );
						scene->images[imageIndex].versions[versionIndex].glInternalFormat = Json_GetUint32( Json_GetMemberByName( v, "glInternalFormat" ), 0 );
						scene->images[imageIndex].versions[versionIndex].uri = ksGltf_strdup( Json_GetString( Json_GetMemberByName( v, "uri" ), "" ) );
					}
				}
			}
			if ( scene->images[imageIndex].versions == NULL )
			{
				scene->images[imageIndex].versions = (ksGltfImageVersion *) calloc( 1, sizeof( ksGltfImageVersion ) );
				scene->images[imageIndex].versionCount = 1;
			}
			const int count = scene->images[imageIndex].versionCount;
			const char * container = ksGltf_GetImageContainerFromUri( baseUri );
			scene->images[imageIndex].versions[count - 1].container = ksGltf_strdup( container );
			scene->images[imageIndex].versions[count - 1].glInternalFormat = ksGltf_GetImageInternalFormatFromUri( binaryBuffer, baseUri );
			scene->images[imageIndex].versions[count - 1].uri = baseUri;
		}
		ksGltf_CreateImageNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load images\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF samplers
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * samplers = Json_GetMemberByName( rootNode, "samplers" );
		scene->samplerCount = Json_GetMemberCount( samplers );
		scene->samplers = (ksGltfSampler *) calloc( scene->samplerCount, sizeof( ksGltfSampler ) );
		for ( int samplerIndex = 0; samplerIndex < scene->samplerCount; samplerIndex++ )
		{
			const Json_t * sampler = Json_GetMemberByIndex( samplers, samplerIndex );
			scene->samplers[samplerIndex].name = ksGltf_strdup( Json_GetMemberName( sampler ) );
			scene->samplers[samplerIndex].magFilter = Json_GetUint16( Json_GetMemberByName( sampler, "magFilter" ), GL_LINEAR );
			scene->samplers[samplerIndex].minFilter = Json_GetUint16( Json_GetMemberByName( sampler, "minFilter" ), GL_NEAREST_MIPMAP_LINEAR );
			scene->samplers[samplerIndex].wrapS = Json_GetUint16( Json_GetMemberByName( sampler, "wrapS" ), GL_REPEAT );
			scene->samplers[samplerIndex].wrapT = Json_GetUint16( Json_GetMemberByName( sampler, "wrapT" ), GL_REPEAT );
			assert( scene->samplers[samplerIndex].name[0] != '\0' );
		}
		ksGltf_CreateSamplerNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load samplers\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF textures
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * textures = Json_GetMemberByName( rootNode, "textures" );
		scene->textureCount = Json_GetMemberCount( textures );
		scene->textures = (ksGltfTexture *) calloc( scene->textureCount, sizeof( ksGltfTexture ) );
		for ( int textureIndex = 0; textureIndex < scene->textureCount; textureIndex++ )
		{
			const Json_t * texture = Json_GetMemberByIndex( textures, textureIndex );
			scene->textures[textureIndex].name = ksGltf_strdup( Json_GetMemberName( texture ) );
			scene->textures[textureIndex].image = ksGltf_GetImageByName( scene, Json_GetString( Json_GetMemberByName( texture, "source" ), "" ) );
			scene->textures[textureIndex].sampler = ksGltf_GetSamplerByName( scene, Json_GetString( Json_GetMemberByName( texture, "sampler" ), "" ) );

			assert( scene->textures[textureIndex].name[0] != '\0' );
			assert( scene->textures[textureIndex].image != NULL );
			//assert( scene->textures[textureIndex].sampler != NULL );

			const char * containers[] = { "ktx", NULL };
#if defined( OS_WINDOWS ) || defined( OS_LINUX ) || defined( OS_MACOS )
			const int flags = GLTF_COMPRESSED_IMAGE_DXT | GLTF_COMPRESSED_IMAGE_DXT_SRGB;
#else
			const int flags = GLTF_COMPRESSED_IMAGE_ETC2 | GLTF_COMPRESSED_IMAGE_ETC2_SRGB |
								GLTF_COMPRESSED_IMAGE_ASTC | GLTF_COMPRESSED_IMAGE_ASTC_SRGB;
#endif
			const char * uri = ksGltf_FindImageUri( scene->textures[textureIndex].image, containers, flags );

			// The "format", "internalFormat", "target" and "type" are automatically derived from the KTX file.
			size_t dataSizeInBytes = 0;
			unsigned char * data = ksGltf_ReadUri( binaryBuffer, uri, &dataSizeInBytes );
			ksGpuTexture_CreateFromKTX( context, &scene->textures[textureIndex].texture, scene->textures[textureIndex].name, data, dataSizeInBytes );
			free( data );
		}
		ksGltf_CreateTextureNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load textures\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF shaders
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const int defaultGlslShaderCount = 3;

		const Json_t * shaders = Json_GetMemberByName( rootNode, "shaders" );
		scene->shaderCount = Json_GetMemberCount( shaders );
		scene->shaders = (ksGltfShader *) calloc( scene->shaderCount, sizeof( ksGltfShader ) );
		for ( int shaderIndex = 0; shaderIndex < scene->shaderCount; shaderIndex++ )
		{
			const Json_t * shader = Json_GetMemberByIndex( shaders, shaderIndex );
			scene->shaders[shaderIndex].name = ksGltf_strdup( Json_GetMemberName( shader ) );
			scene->shaders[shaderIndex].stage = Json_GetUint16( Json_GetMemberByName( shader, "type" ), 0 );

			assert( scene->shaders[shaderIndex].name[0] != '\0' );
			assert( scene->shaders[shaderIndex].stage != 0 );

			const Json_t * extensions = Json_GetMemberByName( shader, "extensions" );
			if ( extensions != NULL )
			{
				for ( int shaderType = 0; shaderType < GLTF_SHADER_TYPE_MAX; shaderType++ )
				{
					const Json_t * shader_versions = Json_GetMemberByName( extensions, shaderVersionExtensions[shaderType] );
					if ( shader_versions != NULL )
					{
						const int count = Json_GetMemberCount( shader_versions );
						const int extra = ( shaderType == GLTF_SHADER_TYPE_GLSL ) * defaultGlslShaderCount;
						scene->shaders[shaderIndex].shaders[shaderType] = (ksGltfShaderVersion *) calloc( count + extra, sizeof( ksGltfShaderVersion ) );
						scene->shaders[shaderIndex].shaderCount[shaderType] = count + extra;
						for ( int index = 0; index < count; index++ )
						{
							const Json_t * glslShader = Json_GetMemberByIndex( shader_versions, index );
							scene->shaders[shaderIndex].shaders[shaderType][index].api = ksGltf_strdup( Json_GetString( Json_GetMemberByName( glslShader, "api" ), "" ) );
							scene->shaders[shaderIndex].shaders[shaderType][index].version = ksGltf_strdup( Json_GetString( Json_GetMemberByName( glslShader, "version" ), "" ) );
							scene->shaders[shaderIndex].shaders[shaderType][index].uri = ksGltf_ParseUri( scene, glslShader, "uri" );
						}
					}
				}
			}

			if ( scene->shaders[shaderIndex].shaders[GLTF_SHADER_TYPE_GLSL] == NULL )
			{
				scene->shaders[shaderIndex].shaders[GLTF_SHADER_TYPE_GLSL] = (ksGltfShaderVersion *) calloc( defaultGlslShaderCount, sizeof( ksGltfShaderVersion ) );
				scene->shaders[shaderIndex].shaderCount[GLTF_SHADER_TYPE_GLSL] = defaultGlslShaderCount;
			}
			const int count = scene->shaders[shaderIndex].shaderCount[GLTF_SHADER_TYPE_GLSL];
			scene->shaders[shaderIndex].shaders[GLTF_SHADER_TYPE_GLSL][count - 3].api = ksGltf_strdup( "opengl" );
			scene->shaders[shaderIndex].shaders[GLTF_SHADER_TYPE_GLSL][count - 3].version = ksGltf_strdup( "100" );
			scene->shaders[shaderIndex].shaders[GLTF_SHADER_TYPE_GLSL][count - 3].uri = ksGltf_ParseUri( scene, shader, "uri" );
			scene->shaders[shaderIndex].shaders[GLTF_SHADER_TYPE_GLSL][count - 2].api = ksGltf_strdup( "opengles" );
			scene->shaders[shaderIndex].shaders[GLTF_SHADER_TYPE_GLSL][count - 2].version = ksGltf_strdup( "100 es" );
			scene->shaders[shaderIndex].shaders[GLTF_SHADER_TYPE_GLSL][count - 2].uri = ksGltf_ParseUri( scene, shader, "uri" );
			scene->shaders[shaderIndex].shaders[GLTF_SHADER_TYPE_GLSL][count - 1].api = ksGltf_strdup( "vulkan" );
			scene->shaders[shaderIndex].shaders[GLTF_SHADER_TYPE_GLSL][count - 1].version = ksGltf_strdup( "100 es" );
			scene->shaders[shaderIndex].shaders[GLTF_SHADER_TYPE_GLSL][count - 1].uri = ksGltf_ParseUri( scene, shader, "uri" );

#if GRAPHICS_API_OPENGL == 1
			assert( ksGltf_FindShaderUri( &scene->shaders[shaderIndex], GLTF_SHADER_TYPE_SPIRV, "opengl", SPIRV_VERSION ) != NULL ||
					ksGltf_FindShaderUri( &scene->shaders[shaderIndex], GLTF_SHADER_TYPE_GLSL, "opengl", GLSL_VERSION ) != NULL );
#elif GRAPHICS_API_OPENGL_ES == 1
			assert( ksGltf_FindShaderUri( &scene->shaders[shaderIndex], GLTF_SHADER_TYPE_SPIRV, "opengles", SPIRV_VERSION ) != NULL ||
					ksGltf_FindShaderUri( &scene->shaders[shaderIndex], GLTF_SHADER_TYPE_GLSL, "opengles", GLSL_VERSION ) != NULL );
#elif GRAPHICS_API_VULKAN == 1
			assert( ksGltf_FindShaderUri( &scene->shaders[shaderIndex], GLTF_SHADER_TYPE_SPIRV, "vulkan", SPIRV_VERSION ) != NULL ||
					ksGltf_FindShaderUri( &scene->shaders[shaderIndex], GLTF_SHADER_TYPE_GLSL, "vulkan", GLSL_VERSION ) != NULL );
#elif GRAPHICS_API_D3D == 1
			assert( ksGltf_FindShaderUri( &scene->shaders[shaderIndex], GLTF_SHADER_TYPE_HLSL, "d3d", HLSL_VERSION ) != NULL );
#elif GRAPHICS_API_METAL == 1
			assert( ksGltf_FindShaderUri( &scene->shaders[shaderIndex], GLTF_SHADER_TYPE_METALSL, "metal", METALSL_VERSION ) != NULL );
#endif
		}
		ksGltf_CreateShaderNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load shaders\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF programs
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * programs = Json_GetMemberByName( rootNode, "programs" );
		scene->programCount = Json_GetMemberCount( programs );
		scene->programs = (ksGltfProgram *) calloc( scene->programCount, sizeof( ksGltfProgram ) );
		for ( int programIndex = 0; programIndex < scene->programCount; programIndex++ )
		{
			const Json_t * program = Json_GetMemberByIndex( programs, programIndex );
			const char * vertexShaderName = Json_GetString( Json_GetMemberByName( program, "vertexShader" ), "" );
			const char * fragmentShaderName = Json_GetString( Json_GetMemberByName( program, "fragmentShader" ), "" );
			const ksGltfShader * vertexShader = ksGltf_GetShaderByName( scene, vertexShaderName );
			const ksGltfShader * fragmentShader = ksGltf_GetShaderByName( scene, fragmentShaderName );

			assert( vertexShader != NULL );
			assert( fragmentShader != NULL );

			scene->programs[programIndex].name = ksGltf_strdup( Json_GetMemberName( program ) );
			assert( scene->programs[programIndex].name[0] != '\0' );

#if GRAPHICS_API_OPENGL == 1
			const char * vertexShaderUri = ksGltf_FindShaderUri( vertexShader, GLTF_SHADER_TYPE_SPIRV, "opengl", SPIRV_VERSION );
			const char * fragmentShaderUri = ksGltf_FindShaderUri( fragmentShader, GLTF_SHADER_TYPE_SPIRV, "opengl", SPIRV_VERSION );
			if ( vertexShaderUri == NULL || fragmentShaderUri == NULL )
			{
				vertexShaderUri = ksGltf_FindShaderUri( vertexShader, GLTF_SHADER_TYPE_GLSL, "opengl", GLSL_VERSION );
				fragmentShaderUri = ksGltf_FindShaderUri( fragmentShader, GLTF_SHADER_TYPE_GLSL, "opengl", GLSL_VERSION );
			}
#elif GRAPHICS_API_OPENGL_ES == 1
			const char * vertexShaderUri = ksGltf_FindShaderUri( vertexShader, GLTF_SHADER_TYPE_SPIRV, "opengles", SPIRV_VERSION );
			const char * fragmentShaderUri = ksGltf_FindShaderUri( fragmentShader, GLTF_SHADER_TYPE_SPIRV, "opengles", SPIRV_VERSION );
			if ( vertexShaderUri == NULL || fragmentShaderUri == NULL )
			{
				vertexShaderUri = ksGltf_FindShaderUri( vertexShader, GLTF_SHADER_TYPE_GLSL, "opengles", GLSL_VERSION );
				fragmentShaderUri = ksGltf_FindShaderUri( fragmentShader, GLTF_SHADER_TYPE_GLSL, "opengles", GLSL_VERSION );
			}
#elif GRAPHICS_API_VULKAN == 1
			const char * vertexShaderUri = ksGltf_FindShaderUri( vertexShader, GLTF_SHADER_TYPE_SPIRV, "vulkan", SPIRV_VERSION );
			const char * fragmentShaderUri = ksGltf_FindShaderUri( fragmentShader, GLTF_SHADER_TYPE_SPIRV, "vulkan", SPIRV_VERSION );
			if ( vertexShaderUri == NULL || fragmentShaderUri == NULL )
			{
				vertexShaderUri = ksGltf_FindShaderUri( vertexShader, GLTF_SHADER_TYPE_GLSL, "vulkan", GLSL_VERSION );
				fragmentShaderUri = ksGltf_FindShaderUri( fragmentShader, GLTF_SHADER_TYPE_GLSL, "vulkan", GLSL_VERSION );
			}
#elif GRAPHICS_API_D3D == 1
			const char * vertexShaderUri = ksGltf_FindShaderUri( vertexShader, GLTF_SHADER_TYPE_HLSL, "d3d", HLSL_VERSION );
			const char * fragmentShaderUri = ksGltf_FindShaderUri( fragmentShader, GLTF_SHADER_TYPE_HLSL, "d3d", HLSL_VERSION );
#elif GRAPHICS_API_METAL == 1
			const char * vertexShaderUri = ksGltf_FindShaderUri( vertexShader, GLTF_SHADER_TYPE_METALSL, "metal", METALSL_VERSION );
			const char * fragmentShaderUri = ksGltf_FindShaderUri( fragmentShader, GLTF_SHADER_TYPE_METALSL, "metal", METALSL_VERSION );
#endif
			scene->programs[programIndex].vertexSource = ksGltf_ReadUri( binaryBuffer, vertexShaderUri, &scene->programs[programIndex].vertexSourceSize );
			scene->programs[programIndex].fragmentSource = ksGltf_ReadUri( binaryBuffer, fragmentShaderUri, &scene->programs[programIndex].fragmentSourceSize );

			assert( scene->programs[programIndex].vertexSource[0] != '\0' );
			assert( scene->programs[programIndex].fragmentSource[0] != '\0' );
		}
		ksGltf_CreateProgramNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load programs\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF techniques
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * techniques = Json_GetMemberByName( rootNode, "techniques" );
		scene->techniqueCount = Json_GetMemberCount( techniques );
		scene->techniques = (ksGltfTechnique *) calloc( scene->techniqueCount, sizeof( ksGltfTechnique ) );
		for ( int techniqueIndex = 0; techniqueIndex < scene->techniqueCount; techniqueIndex++ )
		{
			const Json_t * technique = Json_GetMemberByIndex( techniques, techniqueIndex );
			scene->techniques[techniqueIndex].name = ksGltf_strdup( Json_GetMemberName( technique ) );

			assert( scene->techniques[techniqueIndex].name[0] != '\0' );

			const Json_t * parameters = Json_GetMemberByName( technique, "parameters" );

#if GRAPHICS_API_OPENGL == 1 || GRAPHICS_API_OPENGL_ES == 1
			int conversion = KS_GLSL_CONVERSION_FLAG_JOINT_BUFFER | KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL |
								( settings->useMultiView ? KS_GLSL_CONVERSION_FLAG_MULTI_VIEW : KS_GLSL_CONVERSION_FLAG_VIEW_PROJECTION_BUFFER );
#elif GRAPHICS_API_VULKAN == 1
			int conversion = KS_GLSL_CONVERSION_FLAG_JOINT_BUFFER | KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN |
								( settings->useMultiView ? KS_GLSL_CONVERSION_FLAG_MULTI_VIEW : KS_GLSL_CONVERSION_FLAG_VIEW_PROJECTION_BUFFER );
#else
			int conversion = KS_GLSL_CONVERSION_NONE;
#endif

			//
			// Parse Vertex Attributes.
			//

			scene->techniques[techniqueIndex].vertexAttributeLayout = (ksGpuVertexAttribute *) malloc( sizeof( DefaultVertexAttributeLayout ) );
			memcpy( scene->techniques[techniqueIndex].vertexAttributeLayout, DefaultVertexAttributeLayout, sizeof( DefaultVertexAttributeLayout ) );

			int vertexAttribsFlags = 0;
			const Json_t * attributes = Json_GetMemberByName( technique, "attributes" );
			const int attributeCount = Json_GetMemberCount( attributes );
			scene->techniques[techniqueIndex].attributeCount = attributeCount;
			scene->techniques[techniqueIndex].attributes = (ksGltfVertexAttribute *) calloc( attributeCount, sizeof( ksGltfVertexAttribute ) );
			for ( int j = 0; j < attributeCount; j++ )
			{
				const Json_t * attrib = Json_GetMemberByIndex( attributes, j );
				const char * attribName = Json_GetMemberName( attrib );
				const char * parmName = Json_GetString( attrib, "" );
				// Check for default shader.
				if ( parmName[0] == '\0' )
				{
					assert( attributeCount == 1 );
					vertexAttribsFlags |= VERTEX_ATTRIBUTE_FLAG_POSITION;
					scene->techniques[techniqueIndex].attributes[j].name = ksGltf_strdup( attribName );
					scene->techniques[techniqueIndex].attributes[j].format = KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT;
					scene->techniques[techniqueIndex].vertexAttributeLayout[0].name = scene->techniques[techniqueIndex].attributes[j].name;
					break;
				}

				const Json_t * parameter = Json_GetMemberByName( parameters, parmName );
				const char * semantic = Json_GetString( Json_GetMemberByName( parameter, "semantic" ), "" );
				const int type = Json_GetUint16( Json_GetMemberByName( parameter, "type" ), 0 );

				int attributeFlag = 0;
				if ( strcmp( semantic, "POSITION" ) == 0 )			{ assert( type == GL_FLOAT_VEC3 ); attributeFlag |= VERTEX_ATTRIBUTE_FLAG_POSITION; }
				else if ( strcmp( semantic, "NORMAL" ) == 0 )		{ assert( type == GL_FLOAT_VEC3 ); attributeFlag |= VERTEX_ATTRIBUTE_FLAG_NORMAL; }
				else if ( strcmp( semantic, "TANGENT" ) == 0 )		{ assert( type == GL_FLOAT_VEC3 ); attributeFlag |= VERTEX_ATTRIBUTE_FLAG_TANGENT; }
				else if ( strcmp( semantic, "BINORMAL" ) == 0 )		{ assert( type == GL_FLOAT_VEC3 ); attributeFlag |= VERTEX_ATTRIBUTE_FLAG_BINORMAL; }
				else if ( strcmp( semantic, "COLOR" ) == 0 )		{ assert( type == GL_FLOAT_VEC4 ); attributeFlag |= VERTEX_ATTRIBUTE_FLAG_COLOR; }
				else if ( strcmp( semantic, "TEXCOORD_0" ) == 0 )	{ assert( type == GL_FLOAT_VEC2 ); attributeFlag |= VERTEX_ATTRIBUTE_FLAG_UV0; }
				else if ( strcmp( semantic, "TEXCOORD_1" ) == 0 )	{ assert( type == GL_FLOAT_VEC2 ); attributeFlag |= VERTEX_ATTRIBUTE_FLAG_UV1; }
				else if ( strcmp( semantic, "TEXCOORD_2" ) == 0 )	{ assert( type == GL_FLOAT_VEC2 ); attributeFlag |= VERTEX_ATTRIBUTE_FLAG_UV2; }
				else if ( strcmp( semantic, "JOINT" ) == 0 )		{ assert( type == GL_FLOAT_VEC4 ); attributeFlag |= VERTEX_ATTRIBUTE_FLAG_JOINT_INDICES; }
				else if ( strcmp( semantic, "WEIGHT" ) == 0 )		{ assert( type == GL_FLOAT_VEC4 ); attributeFlag |= VERTEX_ATTRIBUTE_FLAG_JOINT_WEIGHTS; }

				vertexAttribsFlags |= attributeFlag;

				ksGpuAttributeFormat format = KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT;
				if ( type == GL_FLOAT )				{ format = KS_GPU_ATTRIBUTE_FORMAT_R32_SFLOAT; }
				else if ( type == GL_FLOAT_VEC2 )	{ format = KS_GPU_ATTRIBUTE_FORMAT_R32G32_SFLOAT; }
				else if ( type == GL_FLOAT_VEC3 )	{ format = KS_GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT; }
				else if ( type == GL_FLOAT_VEC4 )	{ format = KS_GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT; }

				scene->techniques[techniqueIndex].attributes[j].name = ksGltf_strdup( attribName );
				scene->techniques[techniqueIndex].attributes[j].format = format;
				scene->techniques[techniqueIndex].attributes[j].attributeFlag = attributeFlag;

				// Change the layout attribute name.
				for ( int attribIndex = 0; scene->techniques[techniqueIndex].vertexAttributeLayout[attribIndex].attributeFlag != 0; attribIndex++ )
				{
					ksGpuVertexAttribute * v = &scene->techniques[techniqueIndex].vertexAttributeLayout[attribIndex];
					if ( ( v->attributeFlag & attributeFlag ) != 0 )
					{
						v->name = scene->techniques[techniqueIndex].attributes[j].name;
						break;
					}
				}
			}

			// Get the attribute locations.
			for ( int j = 0; j < attributeCount; j++ )
			{
				const int attributeFlag = scene->techniques[techniqueIndex].attributes[j].attributeFlag;
				int location = 0;
				for ( int bit = 1; bit < attributeFlag; bit <<= 1 )
				{
					if ( ( vertexAttribsFlags & bit ) != 0 )
					{
						location++;
					}
				}
				scene->techniques[techniqueIndex].attributes[j].location = location;
			}

			// Must have at least positions.
			assert( ( vertexAttribsFlags & VERTEX_ATTRIBUTE_FLAG_POSITION ) != 0 );
			scene->techniques[techniqueIndex].vertexAttribsFlags = vertexAttribsFlags;

			//
			// Parse Uniforms.
			//

			const char * semanticUniforms[GLTF_UNIFORM_SEMANTIC_MAX] = { 0 };

			const Json_t * uniforms = Json_GetMemberByName( technique, "uniforms" );
			const int uniformCount = Json_GetMemberCount( uniforms );
			scene->techniques[techniqueIndex].parms = (ksGpuProgramParm *) calloc( uniformCount, sizeof( ksGpuProgramParm ) );
			scene->techniques[techniqueIndex].uniformCount = uniformCount;
			scene->techniques[techniqueIndex].uniforms = (ksGltfUniform *) calloc( uniformCount, sizeof( ksGltfUniform ) );
			memset( scene->techniques[techniqueIndex].uniforms, 0, uniformCount * sizeof( ksGltfUniform ) );
			for ( int uniformIndex = 0; uniformIndex < uniformCount; uniformIndex++ )
			{
				const Json_t * uniform = Json_GetMemberByIndex( uniforms, uniformIndex );
				const char * uniformName = Json_GetMemberName( uniform );
				const char * parmName = Json_GetString( uniform, "" );

				const Json_t * parameter = Json_GetMemberByName( parameters, parmName );
				const char * semanticName = Json_GetString( Json_GetMemberByName( parameter, "semantic" ), "" );
				const int type = Json_GetUint16( Json_GetMemberByName( parameter, "type" ), 0 );
				const int count = Json_GetUint32( Json_GetMemberByName( parameter, "count" ), 0 );
				const char * node = Json_GetString( Json_GetMemberByName( parameter, "node" ), "" );
				int stageFlags = 0;
				int binding = 0;

				const Json_t * extensions = Json_GetMemberByName( parameter, "extensions" );
				if ( extensions != NULL )
				{
					const Json_t * KHR_technique_uniform_stages = Json_GetMemberByName( extensions, "KHR_technique_uniform_stages" );
					if ( KHR_technique_uniform_stages != NULL )
					{
						const Json_t * stageArray = Json_GetMemberByName( KHR_technique_uniform_stages, "stages" );
						const int stageCount = Json_GetMemberCount( stageArray );
						for ( int stateIndex = 0; stateIndex < stageCount; stateIndex++ )
						{
							stageFlags |= ksGltf_GetProgramStageFlag( Json_GetUint16( Json_GetMemberByIndex( stageArray, stateIndex ), 0 ) );
						}
					}

#if GRAPHICS_API_OPENGL == 1 || GRAPHICS_API_OPENGL_ES == 1
					const Json_t * KHR_technique_uniform_binding_opengl = Json_GetMemberByName( extensions, "KHR_technique_uniform_binding_opengl" );
					if ( KHR_technique_uniform_binding_opengl != NULL )
					{
						binding = Json_GetUint32( Json_GetMemberByName( parameter, "binding" ), 0 );
					}
#elif GRAPHICS_API_VULKAN == 1
					const Json_t * KHR_technique_uniform_binding_vulkan = Json_GetMemberByName( extensions, "KHR_technique_uniform_binding_vulkan" );
					if ( KHR_technique_uniform_binding_vulkan != NULL )
					{
						binding = Json_GetUint32( Json_GetMemberByName( parameter, "binding" ), 0 );
					}
#elif GRAPHICS_API_D3D == 1
					const Json_t * KHR_technique_uniform_binding_d3d = Json_GetMemberByName( extensions, "KHR_technique_uniform_binding_d3d" );
					if ( KHR_technique_uniform_binding_d3d != NULL )
					{
						binding = Json_GetUint32( Json_GetMemberByName( parameter, "binding" ), 0 );
					}
#elif GRAPHICS_API_METAL == 1
					const Json_t * KHR_technique_uniform_binding_metal = Json_GetMemberByName( extensions, "KHR_technique_uniform_binding_metal" );
					if ( KHR_technique_uniform_binding_metal != NULL )
					{
						binding = Json_GetUint32( Json_GetMemberByName( parameter, "binding" ), 0 );
					}
#endif
				}

				ksGpuProgramParmType parmType = KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED;
				switch ( type )
				{
					case GL_SAMPLER_2D:				parmType = KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED; break;
					case GL_SAMPLER_3D:				parmType = KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED; break;
					case GL_SAMPLER_CUBE:			parmType = KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED; break;
					case GL_INT:					parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT; break;
					case GL_INT_VEC2:				parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2; break;
					case GL_INT_VEC3:				parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3; break;
					case GL_INT_VEC4:				parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4; break;
					case GL_FLOAT:					parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT; break;
					case GL_FLOAT_VEC2:				parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2; break;
					case GL_FLOAT_VEC3:				parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3; break;
					case GL_FLOAT_VEC4:				parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4; break;
					case GL_FLOAT_MAT2:				parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2; break;
					case GL_FLOAT_MAT2x3:			parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3; break;
					case GL_FLOAT_MAT2x4:			parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4; break;
					case GL_FLOAT_MAT3x2:			parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2; break;
					case GL_FLOAT_MAT3:				parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3; break;
					case GL_FLOAT_MAT3x4:			parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4; break;
					case GL_FLOAT_MAT4x2:			parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2; break;
					case GL_FLOAT_MAT4x3:			parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3; break;
					case GL_FLOAT_MAT4:				parmType = KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4; break;
					default:						assert( false ); break;
				}

				ksGltfUniformSemantic semantic = GLTF_UNIFORM_SEMANTIC_NONE;	// default to the material setting the uniform
				for ( int s = 0; s < sizeof( gltfUniformSemanticNames ) / sizeof( gltfUniformSemanticNames[0] ); s++ )
				{
					if ( strcmp( gltfUniformSemanticNames[s].name, semanticName ) == 0 )
					{
						semantic = gltfUniformSemanticNames[s].semantic;
						semanticUniforms[semantic] = uniformName;
						break;
					}
				}

				// General uniform arrays are not supported, should be using uniform buffers instead.
				assert( semantic == GLTF_UNIFORM_SEMANTIC_JOINT_ARRAY || count == 0 );
				UNUSED_PARM( count );

				// Node uniforms are not supported.
				assert( node[0] == '\0' );
				UNUSED_PARM( node );

				scene->techniques[techniqueIndex].parms[uniformIndex].stageFlags = ( stageFlags != 0 ) ? stageFlags : KS_GPU_PROGRAM_STAGE_FLAG_VERTEX;
				scene->techniques[techniqueIndex].parms[uniformIndex].type = parmType;
				scene->techniques[techniqueIndex].parms[uniformIndex].access = KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY;	// assume all parms are read-only
				scene->techniques[techniqueIndex].parms[uniformIndex].index = uniformIndex;
				scene->techniques[techniqueIndex].parms[uniformIndex].name = ksGltf_strdup( uniformName );
				scene->techniques[techniqueIndex].parms[uniformIndex].binding = binding;

				scene->techniques[techniqueIndex].uniforms[uniformIndex].name = ksGltf_strdup( parmName );
				scene->techniques[techniqueIndex].uniforms[uniformIndex].semantic = semantic;
				scene->techniques[techniqueIndex].uniforms[uniformIndex].type = parmType;
				scene->techniques[techniqueIndex].uniforms[uniformIndex].index = uniformIndex;

				const Json_t * value = Json_GetMemberByName( parameter, "value" );
				if ( value != NULL )
				{
					ksGltfUniform * techniqueUniform = &scene->techniques[techniqueIndex].uniforms[uniformIndex];
					techniqueUniform->semantic = GLTF_UNIFORM_SEMANTIC_DEFAULT_VALUE;
					ksGltf_ParseUniformValue( &techniqueUniform->defaultValue, value, techniqueUniform->type, scene );
				}
			}

			scene->techniques[techniqueIndex].rop.blendEnable = false;
			scene->techniques[techniqueIndex].rop.redWriteEnable = true;
			scene->techniques[techniqueIndex].rop.blueWriteEnable = true;
			scene->techniques[techniqueIndex].rop.greenWriteEnable = true;
			scene->techniques[techniqueIndex].rop.alphaWriteEnable = true;
			scene->techniques[techniqueIndex].rop.depthTestEnable = false;
			scene->techniques[techniqueIndex].rop.depthWriteEnable = true;
			scene->techniques[techniqueIndex].rop.frontFace = KS_GPU_FRONT_FACE_COUNTER_CLOCKWISE;
			scene->techniques[techniqueIndex].rop.cullMode = KS_GPU_CULL_MODE_NONE;
			scene->techniques[techniqueIndex].rop.depthCompare = KS_GPU_COMPARE_OP_LESS;
			scene->techniques[techniqueIndex].rop.blendColor.x = 0.0f;
			scene->techniques[techniqueIndex].rop.blendColor.y = 0.0f;
			scene->techniques[techniqueIndex].rop.blendColor.z = 0.0f;
			scene->techniques[techniqueIndex].rop.blendColor.w = 0.0f;
			scene->techniques[techniqueIndex].rop.blendOpColor = KS_GPU_BLEND_OP_ADD;
			scene->techniques[techniqueIndex].rop.blendSrcColor = KS_GPU_BLEND_FACTOR_ONE;
			scene->techniques[techniqueIndex].rop.blendDstColor = KS_GPU_BLEND_FACTOR_ZERO;
			scene->techniques[techniqueIndex].rop.blendOpAlpha = KS_GPU_BLEND_OP_ADD;
			scene->techniques[techniqueIndex].rop.blendSrcAlpha = KS_GPU_BLEND_FACTOR_ONE;
			scene->techniques[techniqueIndex].rop.blendDstAlpha = KS_GPU_BLEND_FACTOR_ZERO;

			const Json_t * states = Json_GetMemberByName( technique, "states" );
			const Json_t * enable = Json_GetMemberByName( states, "enable" );
			const int enableCount = Json_GetMemberCount( enable );
			for ( int enableIndex = 0; enableIndex < enableCount; enableIndex++ )
			{
				const int enableState = Json_GetUint16( Json_GetMemberByIndex( enable, enableIndex ), 0 );
				switch ( enableState )
				{
					case GL_BLEND:
						scene->techniques[techniqueIndex].rop.blendEnable = true;
						scene->techniques[techniqueIndex].rop.blendOpColor = KS_GPU_BLEND_OP_ADD;
						scene->techniques[techniqueIndex].rop.blendSrcColor = KS_GPU_BLEND_FACTOR_SRC_ALPHA;
						scene->techniques[techniqueIndex].rop.blendDstColor = KS_GPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
						break;
					case GL_DEPTH_TEST:
						scene->techniques[techniqueIndex].rop.depthTestEnable = true;
						break;
					case GL_CULL_FACE:
						scene->techniques[techniqueIndex].rop.cullMode = KS_GPU_CULL_MODE_BACK;
						break;
					case GL_POLYGON_OFFSET_FILL:
						assert( false );
						break;
					case GL_SAMPLE_ALPHA_TO_COVERAGE:
						assert( false );
						break;
					case GL_SCISSOR_TEST:
						assert( false );
						break;
				}
			}

			const Json_t * functions = Json_GetMemberByName( states, "functions" );
			const int functionCount = Json_GetMemberCount( functions );
			for ( int functionIndex = 0; functionIndex < functionCount; functionIndex++ )
			{
				const Json_t * func = Json_GetMemberByIndex( functions, functionIndex );
				const char * funcName = Json_GetMemberName( func );
				if ( strcmp( funcName, "blendColor" ) == 0 )
				{
					// [float:red, float:blue, float:green, float:alpha]
					scene->techniques[techniqueIndex].rop.blendColor.x = Json_GetFloat( Json_GetMemberByIndex( func, 0 ), 0.0f );
					scene->techniques[techniqueIndex].rop.blendColor.y = Json_GetFloat( Json_GetMemberByIndex( func, 1 ), 0.0f );
					scene->techniques[techniqueIndex].rop.blendColor.z = Json_GetFloat( Json_GetMemberByIndex( func, 2 ), 0.0f );
					scene->techniques[techniqueIndex].rop.blendColor.w = Json_GetFloat( Json_GetMemberByIndex( func, 3 ), 0.0f );
				}
				else if ( strcmp( funcName, "blendEquationSeparate" ) == 0 )
				{
					// [GLenum:GL_FUNC_* (rgb), GLenum:GL_FUNC_* (alpha)]
					scene->techniques[techniqueIndex].rop.blendOpColor = ksGltf_GetBlendOp( Json_GetUint16( Json_GetMemberByIndex( func, 0 ), 0 ) );
					scene->techniques[techniqueIndex].rop.blendOpAlpha = ksGltf_GetBlendOp( Json_GetUint16( Json_GetMemberByIndex( func, 1 ), 0 ) );
				}
				else if ( strcmp( funcName, "blendFuncSeparate" ) == 0 )
				{
					// [GLenum:GL_ONE (srcRGB), GLenum:GL_ZERO (dstRGB), GLenum:GL_ONE (srcAlpha), GLenum:GL_ZERO (dstAlpha)]
					scene->techniques[techniqueIndex].rop.blendSrcColor = ksGltf_GetBlendFactor( Json_GetUint16( Json_GetMemberByIndex( func, 0 ), 0 ) );
					scene->techniques[techniqueIndex].rop.blendDstColor = ksGltf_GetBlendFactor( Json_GetUint16( Json_GetMemberByIndex( func, 1 ), 0 ) );
					scene->techniques[techniqueIndex].rop.blendSrcAlpha = ksGltf_GetBlendFactor( Json_GetUint16( Json_GetMemberByIndex( func, 2 ), 0 ) );
					scene->techniques[techniqueIndex].rop.blendDstAlpha = ksGltf_GetBlendFactor( Json_GetUint16( Json_GetMemberByIndex( func, 3 ), 0 ) );
				}
				else if ( strcmp( funcName, "colorMask" ) == 0 )
				{
					// [bool:red, bool:green, bool:blue, bool:alpha]
					scene->techniques[techniqueIndex].rop.redWriteEnable = Json_GetBool( Json_GetMemberByIndex( func, 0 ), false );
					scene->techniques[techniqueIndex].rop.blueWriteEnable = Json_GetBool( Json_GetMemberByIndex( func, 1 ), false );
					scene->techniques[techniqueIndex].rop.greenWriteEnable = Json_GetBool( Json_GetMemberByIndex( func, 2 ), false );
					scene->techniques[techniqueIndex].rop.alphaWriteEnable = Json_GetBool( Json_GetMemberByIndex( func, 3 ), false );
				}
				else if ( strcmp( funcName, "cullFace" ) == 0 )
				{
					// [GLenum:GL_BACK,GL_FRONT]
					scene->techniques[techniqueIndex].rop.cullMode = ksGltf_GetCullMode( Json_GetUint16( Json_GetMemberByIndex( func, 0 ), 0 ) );
				}
				else if ( strcmp( funcName, "depthFunc" ) == 0 )
				{
					// [GLenum:GL_LESS,GL_LEQUAL,GL_GREATER]
					scene->techniques[techniqueIndex].rop.depthCompare = ksGltf_GetCompareOp( Json_GetUint16( Json_GetMemberByIndex( func, 0 ), 0 ) );
				}
				else if ( strcmp( funcName, "depthMask" ) == 0 )
				{
					// [bool:mask]
					scene->techniques[techniqueIndex].rop.depthWriteEnable = Json_GetBool( Json_GetMemberByIndex( func, 0 ), false );
				}
				else if ( strcmp( funcName, "frontFace" ) == 0 )
				{
					// [Glenum:GL_CCW,GL_CW]
					scene->techniques[techniqueIndex].rop.frontFace = ksGltf_GetFrontFace( Json_GetUint16( Json_GetMemberByIndex( func, 0 ), 0 ) );
				}
				else if ( strcmp( funcName, "lineWidth" ) == 0 )
				{
					// [float:width]
					assert( false );
				}
				else if ( strcmp( funcName, "polygonOffset" ) == 0 )
				{
					// [float:factor, float:units]
					assert( false );
				}
				else if ( strcmp( funcName, "depthRange" ) == 0 )
				{
					// [float:znear, float:zfar]
					assert( false );
				}
				else if ( strcmp( funcName, "scissor" ) == 0 )
				{
					// [int:x, int:y, int:width, int:height]
					assert( false );
				}
			}

			ksGltfProgram * program = ksGltf_GetProgramByName( scene, Json_GetString( Json_GetMemberByName( technique, "program" ), "" ) );
			assert( program != NULL );

			ksGltf_CreateTechniqueProgram( context, &scene->techniques[techniqueIndex], program, conversion, semanticUniforms );

			size_t totalPushConstantBytes = 0;
			for ( int uniformIndex = 0; uniformIndex < scene->techniques[techniqueIndex].uniformCount; uniformIndex++ )
			{
				totalPushConstantBytes += ksGpuProgramParm_GetPushConstantSize( scene->techniques[techniqueIndex].parms[uniformIndex].type );
			}
			assert( totalPushConstantBytes <= context->device->maxPushConstantsSize );

		}
		ksGltf_CreateTechniqueNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load techniques\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF materials
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * materials = Json_GetMemberByName( rootNode, "materials" );
		scene->materialCount = Json_GetMemberCount( materials );
		scene->materials = (ksGltfMaterial *) calloc( scene->materialCount, sizeof( ksGltfMaterial ) );
		for ( int materialIndex = 0; materialIndex < scene->materialCount; materialIndex++ )
		{
			const Json_t * material = Json_GetMemberByIndex( materials, materialIndex );
			scene->materials[materialIndex].name = ksGltf_strdup( Json_GetMemberName( material ) );
			assert( scene->materials[materialIndex].name[0] != '\0' );

			const ksGltfTechnique * technique = ksGltf_GetTechniqueByName( scene, Json_GetString( Json_GetMemberByName( material, "technique" ), "" ) );
			if ( settings->useMultiView )
			{
				const Json_t * extensions = Json_GetMemberByName( material, "extensions" );
				if ( extensions != NULL )
				{
					const Json_t * KHR_glsl_multi_view = Json_GetMemberByName( extensions, "KHR_glsl_multi_view" );
					if ( KHR_glsl_multi_view != NULL )
					{
						const ksGltfTechnique * multiViewTechnique = ksGltf_GetTechniqueByName( scene, Json_GetString( Json_GetMemberByName( KHR_glsl_multi_view, "technique" ), "" ) );
						assert( multiViewTechnique != NULL );
						technique = multiViewTechnique;
					}
				}
			}
			scene->materials[materialIndex].technique = technique;
			assert( scene->materials[materialIndex].technique != NULL );

			const Json_t * values = Json_GetMemberByName( material, "values" );
			scene->materials[materialIndex].valueCount = Json_GetMemberCount( values );
			scene->materials[materialIndex].values = (ksGltfMaterialValue *) calloc( scene->materials[materialIndex].valueCount, sizeof( ksGltfMaterialValue ) );
			for ( int valueIndex = 0; valueIndex < scene->materials[materialIndex].valueCount; valueIndex++ )
			{
				const Json_t * value = Json_GetMemberByIndex( values, valueIndex );
				const char * valueName = Json_GetMemberName( value );
				ksGltfUniform * uniform = NULL;
				for ( int uniformIndex = 0; uniformIndex < technique->uniformCount; uniformIndex++ )
				{
					if ( strcmp( technique->uniforms[uniformIndex].name, valueName ) == 0 )
					{
						uniform = &technique->uniforms[uniformIndex];
						break;
					}
				}
				if ( uniform == NULL )
				{
					assert( false );
					continue;
				}
				assert( uniform->semantic == GLTF_UNIFORM_SEMANTIC_NONE || uniform->semantic == GLTF_UNIFORM_SEMANTIC_DEFAULT_VALUE );
				scene->materials[materialIndex].values[valueIndex].uniform = uniform;
				ksGltf_ParseUniformValue( &scene->materials[materialIndex].values[valueIndex].value, value, uniform->type, scene );
			}
			// Make sure that the material sets any uniforms that do not have a special semantic or a default value.
			for ( int uniformIndex = 0; uniformIndex < technique->uniformCount; uniformIndex++ )
			{
				if ( technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_NONE )
				{
					bool found = false;
					for ( int valueIndex = 0; valueIndex < scene->materials[materialIndex].valueCount; valueIndex++ )
					{
						if ( scene->materials[materialIndex].values[valueIndex].uniform == &technique->uniforms[uniformIndex] )
						{
							found = true;
							break;
						}
					}
					assert( found );
					UNUSED_PARM( found );
				}
			}
		}
		ksGltf_CreateMaterialNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load materials\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF meshes
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * models = Json_GetMemberByName( rootNode, "meshes" );
		scene->modelCount = Json_GetMemberCount( models );
		scene->models = (ksGltfModel *) calloc( scene->modelCount, sizeof( ksGltfModel ) );
		ksGltfGeometryAccessors ** accessors = (ksGltfGeometryAccessors **) calloc( scene->modelCount, sizeof( ksGltfGeometryAccessors * ) );
		for ( int modelIndex = 0; modelIndex < scene->modelCount; modelIndex++ )
		{
			const Json_t * model = Json_GetMemberByIndex( models, modelIndex );
			scene->models[modelIndex].name = ksGltf_strdup( Json_GetMemberName( model ) );

			ksVector3f_Set( &scene->models[modelIndex].mins, FLT_MAX );
			ksVector3f_Set( &scene->models[modelIndex].maxs, -FLT_MAX );

			assert( scene->models[modelIndex].name[0] != '\0' );

			const Json_t * primitives = Json_GetMemberByName( model, "primitives" );
			scene->models[modelIndex].surfaceCount = Json_GetMemberCount( primitives );
			scene->models[modelIndex].surfaces = (ksGltfSurface *) calloc( scene->models[modelIndex].surfaceCount, sizeof( ksGltfSurface ) );
			accessors[modelIndex] = (ksGltfGeometryAccessors *) calloc( scene->models[modelIndex].surfaceCount, sizeof( ksGltfGeometryAccessors ) );
			for ( int surfaceIndex = 0; surfaceIndex < scene->models[modelIndex].surfaceCount; surfaceIndex++ )
			{
				ksGltfSurface * surface = &scene->models[modelIndex].surfaces[surfaceIndex];
				const Json_t * primitive = Json_GetMemberByIndex( primitives, surfaceIndex );
				const Json_t * attributes = Json_GetMemberByName( primitive, "attributes" );

				const char * positionAccessorName		= Json_GetString( Json_GetMemberByName( attributes, "POSITION" ), "" );
				const char * normalAccessorName			= Json_GetString( Json_GetMemberByName( attributes, "NORMAL" ), "" );
				const char * tangentAccessorName		= Json_GetString( Json_GetMemberByName( attributes, "TANGENT" ), "" );
				const char * binormalAccessorName		= Json_GetString( Json_GetMemberByName( attributes, "BINORMAL" ), "" );
				const char * colorAccessorName			= Json_GetString( Json_GetMemberByName( attributes, "COLOR" ), "" );
				const char * uv0AccessorName			= Json_GetString( Json_GetMemberByName( attributes, "TEXCOORD_0" ), "" );
				const char * uv1AccessorName			= Json_GetString( Json_GetMemberByName( attributes, "TEXCOORD_1" ), "" );
				const char * uv2AccessorName			= Json_GetString( Json_GetMemberByName( attributes, "TEXCOORD_2" ), "" );
				const char * jointIndicesAccessorName	= Json_GetString( Json_GetMemberByName( attributes, "JOINT" ), "" );
				const char * jointWeightsAccessorName	= Json_GetString( Json_GetMemberByName( attributes, "WEIGHT" ), "" );
				const char * indicesAccessorName		= Json_GetString( Json_GetMemberByName( primitive, "indices" ), "" );

				surface->material = ksGltf_GetMaterialByName( scene, Json_GetString( Json_GetMemberByName( primitive, "material" ), "" ) );
				assert( surface->material != NULL );

				ksGltfGeometryAccessors * surfaceAccessors = &accessors[modelIndex][surfaceIndex];
				surfaceAccessors->position		= ksGltf_GetAccessorByNameAndType( scene, positionAccessorName,		"VEC3",		GL_FLOAT );
				surfaceAccessors->normal		= ksGltf_GetAccessorByNameAndType( scene, normalAccessorName,		"VEC3",		GL_FLOAT );
				surfaceAccessors->tangent		= ksGltf_GetAccessorByNameAndType( scene, tangentAccessorName,		"VEC3",		GL_FLOAT );
				surfaceAccessors->binormal		= ksGltf_GetAccessorByNameAndType( scene, binormalAccessorName,		"VEC3",		GL_FLOAT );
				surfaceAccessors->color			= ksGltf_GetAccessorByNameAndType( scene, colorAccessorName,		"VEC4",		GL_FLOAT );
				surfaceAccessors->uv0			= ksGltf_GetAccessorByNameAndType( scene, uv0AccessorName,			"VEC2",		GL_FLOAT );
				surfaceAccessors->uv1			= ksGltf_GetAccessorByNameAndType( scene, uv1AccessorName,			"VEC2",		GL_FLOAT );
				surfaceAccessors->uv2			= ksGltf_GetAccessorByNameAndType( scene, uv2AccessorName,			"VEC2",		GL_FLOAT );
				surfaceAccessors->jointIndices	= ksGltf_GetAccessorByNameAndType( scene, jointIndicesAccessorName,	"VEC4",		GL_FLOAT );
				surfaceAccessors->jointWeights	= ksGltf_GetAccessorByNameAndType( scene, jointWeightsAccessorName,	"VEC4",		GL_FLOAT );
				surfaceAccessors->indices		= ksGltf_GetAccessorByNameAndType( scene, indicesAccessorName,		"SCALAR",	GL_UNSIGNED_SHORT );

				if ( surfaceAccessors->position == NULL || surfaceAccessors->indices == NULL )
				{
					assert( false );
					continue;
				}

				surface->mins.x = surfaceAccessors->position->floatMin[0];
				surface->mins.y = surfaceAccessors->position->floatMin[1];
				surface->mins.z = surfaceAccessors->position->floatMin[2];
				surface->maxs.x = surfaceAccessors->position->floatMax[0];
				surface->maxs.y = surfaceAccessors->position->floatMax[1];
				surface->maxs.z = surfaceAccessors->position->floatMax[2];

				assert( surfaceAccessors->normal		== NULL || surfaceAccessors->normal->count			== surfaceAccessors->position->count );
				assert( surfaceAccessors->tangent		== NULL || surfaceAccessors->tangent->count			== surfaceAccessors->position->count );
				assert( surfaceAccessors->binormal		== NULL || surfaceAccessors->binormal->count		== surfaceAccessors->position->count );
				assert( surfaceAccessors->color			== NULL || surfaceAccessors->color->count			== surfaceAccessors->position->count );
				assert( surfaceAccessors->uv0			== NULL || surfaceAccessors->uv0->count				== surfaceAccessors->position->count );
				assert( surfaceAccessors->uv1			== NULL || surfaceAccessors->uv1->count				== surfaceAccessors->position->count );
				assert( surfaceAccessors->uv2			== NULL || surfaceAccessors->uv2->count				== surfaceAccessors->position->count );
				assert( surfaceAccessors->jointIndices	== NULL || surfaceAccessors->jointIndices->count	== surfaceAccessors->position->count );
				assert( surfaceAccessors->jointWeights	== NULL || surfaceAccessors->jointWeights->count	== surfaceAccessors->position->count );

				ksDefaultVertexAttributeArrays attribs;
				memset( &attribs, 0, sizeof( attribs ) );

				for ( int i = 0; i <= modelIndex; i++ )
				{
					const int surfaceCount = ( i == modelIndex ) ? surfaceIndex : scene->models[i].surfaceCount;
					for ( int j = 0; j < surfaceCount; j++ )
					{
						const ksGltfGeometryAccessors * otherAccessors = &accessors[i][j];
						if (	( surfaceAccessors->position		== NULL || surfaceAccessors->position		== otherAccessors->position		) &&
								( surfaceAccessors->normal			== NULL || surfaceAccessors->normal			== otherAccessors->normal		) &&
								( surfaceAccessors->tangent			== NULL || surfaceAccessors->tangent		== otherAccessors->tangent		) &&
								( surfaceAccessors->binormal		== NULL || surfaceAccessors->binormal		== otherAccessors->binormal		) &&
								( surfaceAccessors->color			== NULL || surfaceAccessors->color			== otherAccessors->color		) &&
								( surfaceAccessors->uv0				== NULL || surfaceAccessors->uv0			== otherAccessors->uv0			) &&
								( surfaceAccessors->uv1				== NULL || surfaceAccessors->uv1			== otherAccessors->uv1			) &&
								( surfaceAccessors->uv2				== NULL || surfaceAccessors->uv2			== otherAccessors->uv2			) &&
								( surfaceAccessors->jointIndices	== NULL || surfaceAccessors->jointIndices	== otherAccessors->jointIndices	) &&
								( surfaceAccessors->jointWeights	== NULL || surfaceAccessors->jointWeights	== otherAccessors->jointWeights	)
							)
						{
							ksGpuVertexAttributeArrays_CreateFromBuffer( &attribs.base,
											scene->models[i].surfaces[j].geometry.layout,
											scene->models[i].surfaces[j].geometry.vertexCount,
											scene->models[i].surfaces[j].geometry.vertexAttribsFlags,
											&scene->models[i].surfaces[j].geometry.vertexBuffer );
							i = modelIndex;
							break;
						}
					}
				}

				if ( attribs.base.buffer == NULL )
				{
					const int attribsFlags = ( surfaceAccessors->position != NULL		? VERTEX_ATTRIBUTE_FLAG_POSITION : 0 ) |
											( surfaceAccessors->normal != NULL			? VERTEX_ATTRIBUTE_FLAG_NORMAL : 0 ) |
											( surfaceAccessors->tangent != NULL			? VERTEX_ATTRIBUTE_FLAG_TANGENT : 0 ) |
											( surfaceAccessors->binormal != NULL		? VERTEX_ATTRIBUTE_FLAG_BINORMAL : 0 ) |
											( surfaceAccessors->color != NULL			? VERTEX_ATTRIBUTE_FLAG_COLOR : 0 ) |
											( surfaceAccessors->uv0 != NULL				? VERTEX_ATTRIBUTE_FLAG_UV0 : 0 ) |
											( surfaceAccessors->uv1 != NULL				? VERTEX_ATTRIBUTE_FLAG_UV1 : 0 ) |
											( surfaceAccessors->uv2 != NULL				? VERTEX_ATTRIBUTE_FLAG_UV2 : 0 ) |
											( surfaceAccessors->jointIndices != NULL	? VERTEX_ATTRIBUTE_FLAG_JOINT_INDICES : 0 ) |
											( surfaceAccessors->jointWeights != NULL	? VERTEX_ATTRIBUTE_FLAG_JOINT_WEIGHTS : 0 );

					ksGpuVertexAttributeArrays_Alloc( &attribs.base, surface->material->technique->vertexAttributeLayout, surfaceAccessors->position->count, attribsFlags );

					if ( surfaceAccessors->position != NULL )		memcpy( attribs.position,		ksGltf_GetBufferData( surfaceAccessors->position ),		surfaceAccessors->position->count		* sizeof( attribs.position[0] ) );
					if ( surfaceAccessors->normal != NULL )			memcpy( attribs.normal,			ksGltf_GetBufferData( surfaceAccessors->normal ),		surfaceAccessors->normal->count			* sizeof( attribs.normal[0] ) );
					if ( surfaceAccessors->tangent != NULL )		memcpy( attribs.tangent,		ksGltf_GetBufferData( surfaceAccessors->tangent ),		surfaceAccessors->tangent->count		* sizeof( attribs.tangent[0] ) );
					if ( surfaceAccessors->binormal != NULL )		memcpy( attribs.binormal,		ksGltf_GetBufferData( surfaceAccessors->binormal ),		surfaceAccessors->binormal->count		* sizeof( attribs.binormal[0] ) );
					if ( surfaceAccessors->color != NULL )			memcpy( attribs.color,			ksGltf_GetBufferData( surfaceAccessors->color ),		surfaceAccessors->color->count			* sizeof( attribs.color[0] ) );
					if ( surfaceAccessors->uv0 != NULL )			memcpy( attribs.uv0,			ksGltf_GetBufferData( surfaceAccessors->uv0 ),			surfaceAccessors->uv0->count			* sizeof( attribs.uv0[0] ) );
					if ( surfaceAccessors->uv1 != NULL )			memcpy( attribs.uv1,			ksGltf_GetBufferData( surfaceAccessors->uv1 ),			surfaceAccessors->uv1->count			* sizeof( attribs.uv1[0] ) );
					if ( surfaceAccessors->uv2 != NULL )			memcpy( attribs.uv2,			ksGltf_GetBufferData( surfaceAccessors->uv2 ),			surfaceAccessors->uv2->count			* sizeof( attribs.uv2[0] ) );
					if ( surfaceAccessors->jointIndices != NULL )	memcpy( attribs.jointIndices,	ksGltf_GetBufferData( surfaceAccessors->jointIndices ),	surfaceAccessors->jointIndices->count	* sizeof( attribs.jointIndices[0] ) );
					if ( surfaceAccessors->jointWeights != NULL )	memcpy( attribs.jointWeights,	ksGltf_GetBufferData( surfaceAccessors->jointWeights ),	surfaceAccessors->jointWeights->count	* sizeof( attribs.jointWeights[0] ) );
				}

				ksGpuTriangleIndexArray indices;
				memset( &indices, 0, sizeof( indices ) );

				for ( int i = 0; i <= modelIndex; i++ )
				{
					const int surfaceCount = ( i == modelIndex ) ? surfaceIndex : scene->models[i].surfaceCount;
					for ( int j = 0; j < surfaceCount; j++ )
					{
						const ksGltfGeometryAccessors * otherAccessors = &accessors[i][j];
						if ( surfaceAccessors->indices == otherAccessors->indices )
						{
							ksGpuTriangleIndexArray_CreateFromBuffer( &indices, surfaceAccessors->indices->count,
																	&scene->models[i].surfaces[j].geometry.indexBuffer );
							i = modelIndex;
							break;
						}
					}
				}

				if ( indices.buffer == NULL )
				{
					ksGpuTriangleIndexArray_Alloc( &indices, surfaceAccessors->indices->count, (ksGpuTriangleIndex *)ksGltf_GetBufferData( surfaceAccessors->indices ) );
				}

				ksGpuGeometry_Create( context, &surface->geometry, &attribs.base, &indices );

				ksGpuVertexAttributeArrays_Free( &attribs.base );
				ksGpuTriangleIndexArray_Free( &indices );

				ksGpuGraphicsPipelineParms pipelineParms;
				ksGpuGraphicsPipelineParms_Init( &pipelineParms );

				pipelineParms.renderPass = renderPass;
				pipelineParms.program = &surface->material->technique->program;
				pipelineParms.geometry = &surface->geometry;
				pipelineParms.rop = surface->material->technique->rop;

				ksGpuGraphicsPipeline_Create( context, &surface->pipeline, &pipelineParms );

				ksVector3f_Min( &scene->models[modelIndex].mins, &scene->models[modelIndex].mins, &surface->mins );
				ksVector3f_Max( &scene->models[modelIndex].maxs, &scene->models[modelIndex].maxs, &surface->maxs );
			}
		}

		// Free the accessors.
		for ( int modelIndex = 0; modelIndex < scene->modelCount; modelIndex++ )
		{
			free( accessors[modelIndex] );
		}
		free( accessors );

		ksGltf_CreateModelNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load models\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF animations
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * animations = Json_GetMemberByName( rootNode, "animations" );
		scene->animationCount = Json_GetMemberCount( animations );
		scene->animations = (ksGltfAnimation *) calloc( scene->animationCount, sizeof( ksGltfAnimation ) );
		scene->timeLineCount = 0;	// May not need all because they are often shared.
		scene->timeLines = (ksGltfTimeLine *) calloc( scene->animationCount, sizeof( ksGltfTimeLine ) );
		for ( int animationIndex = 0; animationIndex < scene->animationCount; animationIndex++ )
		{
			const Json_t * animation = Json_GetMemberByIndex( animations, animationIndex );
			scene->animations[animationIndex].name = ksGltf_strdup( Json_GetMemberName( animation ) );

			const Json_t * parameters = Json_GetMemberByName( animation, "parameters" );
			const Json_t * samplers = Json_GetMemberByName( animation, "samplers" );

			// This assumes there is only a single time-line per animation.
			const char * timeAccessorName = Json_GetString( Json_GetMemberByName( parameters, "TIME" ), "" );
			const ksGltfAccessor * timeAccessor = ksGltf_GetAccessorByNameAndType( scene, timeAccessorName, "SCALAR", GL_FLOAT );

			if ( timeAccessor == NULL || timeAccessor->count <= 0 )
			{
				assert( false );
				continue;
			}

			const int sampleCount = timeAccessor->count;
			float * sampleTimes = (float *)ksGltf_GetBufferData( timeAccessor );

			assert( sampleCount >= 2 );
			assert( sampleTimes != NULL );

			// Animation time lines are often shared so check if this one already exists.
			for ( int timeLineIndex = 0; timeLineIndex < scene->timeLineCount; timeLineIndex++ )
			{
				if ( sampleCount == scene->timeLines[timeLineIndex].sampleCount &&
						sampleTimes == scene->timeLines[timeLineIndex].sampleTimes )
				{
					scene->animations[animationIndex].timeLine = &scene->timeLines[timeLineIndex];
					break;
				}
			}
			if ( scene->animations[animationIndex].timeLine == NULL )
			{
				// Create a new time line.
				ksGltfTimeLine * timeLine = &scene->timeLines[scene->timeLineCount++];
				timeLine->sampleCount = sampleCount;
				timeLine->sampleTimes = sampleTimes;

				const float step = ( timeLine->sampleTimes[timeLine->sampleCount - 1] - timeLine->sampleTimes[0] ) / timeLine->sampleCount;
				timeLine->duration = timeLine->sampleTimes[timeLine->sampleCount - 1] - timeLine->sampleTimes[0];
				timeLine->rcpStep = 1.0f / step;
				for ( int keyFrameIndex = 0; keyFrameIndex < timeLine->sampleCount; keyFrameIndex++ )
				{
					const float delta = timeLine->sampleTimes[keyFrameIndex] - keyFrameIndex * step;
					// Check if the time is more than 0.1 milliseconds from a fixed-rate time-line.
					if ( fabs( delta ) > 1e-4f )
					{
						timeLine->rcpStep = 0.0f;
						break;
					}
				}

				scene->animations[animationIndex].timeLine = timeLine;
			}

			const Json_t * channels = Json_GetMemberByName( animation, "channels" );
			scene->animations[animationIndex].channelCount = Json_GetMemberCount( channels );
			scene->animations[animationIndex].channels = (ksGltfAnimationChannel *) calloc( scene->animations[animationIndex].channelCount, sizeof( ksGltfAnimationChannel ) );
			int newChannelCount = 0;
			for ( int channelIndex = 0; channelIndex < scene->animations[animationIndex].channelCount; channelIndex++ )
			{
				const Json_t * channel = Json_GetMemberByIndex( channels, channelIndex );
				const char * samplerName = Json_GetString( Json_GetMemberByName( channel, "sampler" ), "" );
				const Json_t * sampler = Json_GetMemberByName( samplers, samplerName );
				const char * inputName = Json_GetString( Json_GetMemberByName( sampler, "input" ), "" );
				const char * interpolation = Json_GetString( Json_GetMemberByName( sampler, "interpolation" ), "" );
				const char * outputName = Json_GetString( Json_GetMemberByName( sampler, "output" ), "" );
				const char * accessorName = Json_GetString( Json_GetMemberByName( parameters, outputName ), "" );

				assert( strcmp( inputName, "TIME" ) == 0 );
				assert( strcmp( interpolation, "LINEAR" ) == 0 );
				assert( outputName[0] != '\0' );
				assert( accessorName[0] != '\0' );

				UNUSED_PARM( inputName );
				UNUSED_PARM( interpolation );

				const Json_t * target = Json_GetMemberByName( channel, "target" );
				const char * nodeName = Json_GetString( Json_GetMemberByName( target, "id" ), "" );
				const char * pathName = Json_GetString( Json_GetMemberByName( target, "path" ), "" );

				ksVector3f * translation = NULL;
				ksQuatf * rotation = NULL;
				ksVector3f * scale = NULL;

				if ( strcmp( pathName, "translation" ) == 0 )
				{
					const ksGltfAccessor * accessor	= ksGltf_GetAccessorByNameAndType( scene, accessorName, "VEC3", GL_FLOAT );
					assert( accessor != NULL );
					translation = (ksVector3f *) ksGltf_GetBufferData( accessor );
				}
				else if ( strcmp( pathName, "rotation" ) == 0 )
				{
					const ksGltfAccessor * accessor	= ksGltf_GetAccessorByNameAndType( scene, accessorName, "VEC4", GL_FLOAT );
					assert( accessor != NULL );
					rotation = (ksQuatf *) ksGltf_GetBufferData( accessor );
				}
				else if ( strcmp( pathName, "scale" ) == 0 )
				{
					const ksGltfAccessor * accessor	= ksGltf_GetAccessorByNameAndType( scene, accessorName, "VEC3", GL_FLOAT );
					assert( accessor != NULL );
					scale = (ksVector3f *) ksGltf_GetBufferData( accessor );
				}

				// Try to merge this channel with a previous channel for the same node.
				for ( int k = 0; k < newChannelCount; k++ )
				{
					if ( strcmp( nodeName, scene->animations[animationIndex].channels[k].nodeName ) == 0 )
					{
						if ( translation != NULL )
						{
							scene->animations[animationIndex].channels[k].translation = translation;
							translation = NULL;
						}
						if ( rotation != NULL )
						{
							scene->animations[animationIndex].channels[k].rotation = rotation;
							rotation = NULL;
						}
						if ( scale != NULL )
						{
							scene->animations[animationIndex].channels[k].scale = scale;
							scale = NULL;
						}
						break;
					}
				}

				// Only store the channel if it was not merged.
				if ( translation != NULL || rotation != NULL || scale != NULL )
				{
					scene->animations[animationIndex].channels[newChannelCount].nodeName = ksGltf_strdup( nodeName );
					scene->animations[animationIndex].channels[newChannelCount].node = NULL; // linked up once the nodes are loaded
					scene->animations[animationIndex].channels[newChannelCount].translation = translation;
					scene->animations[animationIndex].channels[newChannelCount].rotation = rotation;
					scene->animations[animationIndex].channels[newChannelCount].scale = scale;
					newChannelCount++;
				}
			}
			scene->animations[animationIndex].channelCount = newChannelCount;
		}
		ksGltf_CreateAnimationNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load animations\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF skins
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * skins = Json_GetMemberByName( rootNode, "skins" );
		scene->skinCount = Json_GetMemberCount( skins );
		scene->skins = (ksGltfSkin *) calloc( scene->skinCount, sizeof( ksGltfSkin ) );
		for ( int skinIndex = 0; skinIndex < scene->skinCount; skinIndex++ )
		{
			const Json_t * skin = Json_GetMemberByIndex( skins, skinIndex );
			scene->skins[skinIndex].name = ksGltf_strdup( Json_GetMemberName( skin ) );
			ksMatrix4x4f bindShapeMatrix;
			ksGltf_ParseFloatArray( bindShapeMatrix.m[0], 16, Json_GetMemberByName( skin, "bindShapeMatrix" ) );

			const char * bindAccessorName = Json_GetString( Json_GetMemberByName( skin, "inverseBindMatrices" ), "" );
			const ksGltfAccessor * bindAccess = ksGltf_GetAccessorByNameAndType( scene, bindAccessorName, "MAT4", GL_FLOAT );
			scene->skins[skinIndex].inverseBindMatrices = ksGltf_GetBufferData( bindAccess );

			assert( scene->skins[skinIndex].name[0] != '\0' );
			assert( scene->skins[skinIndex].inverseBindMatrices != NULL );

			scene->skins[skinIndex].parentNode = NULL;	// linked up once the nodes are loaded

			const Json_t * jointNames = Json_GetMemberByName( skin, "jointNames" );
			scene->skins[skinIndex].jointCount = Json_GetMemberCount( jointNames );
			scene->skins[skinIndex].joints = (ksGltfJoint *) calloc( scene->skins[skinIndex].jointCount, sizeof( ksGltfJoint ) );
			assert( scene->skins[skinIndex].jointCount <= MAX_JOINTS );
			for ( int jointIndex = 0; jointIndex < scene->skins[skinIndex].jointCount; jointIndex++ )
			{
				ksMatrix4x4f inverseBindMatrix;
				ksMatrix4x4f_Multiply( &inverseBindMatrix, &scene->skins[skinIndex].inverseBindMatrices[jointIndex], &bindShapeMatrix );
				scene->skins[skinIndex].inverseBindMatrices[jointIndex] = inverseBindMatrix;

				scene->skins[skinIndex].joints[jointIndex].name = ksGltf_strdup( Json_GetString( Json_GetMemberByIndex( jointNames, jointIndex ), "" ) );
				scene->skins[skinIndex].joints[jointIndex].node = NULL; // linked up once the nodes are loaded
			}
			assert( bindAccess->count == scene->skins[skinIndex].jointCount );

			ksGpuBuffer_Create( context, &scene->skins[skinIndex].jointBuffer, KS_GPU_BUFFER_TYPE_UNIFORM, scene->skins[skinIndex].jointCount * sizeof( ksMatrix4x4f ), NULL, false );

			const Json_t * extensions = Json_GetMemberByName( skin, "extensions" );
			if ( extensions != NULL )
			{
				const Json_t * KHR_skin_culling = Json_GetMemberByName( extensions, "KHR_skin_culling" );
				if ( KHR_skin_culling != NULL )
				{
					const char * minsAccessorName = Json_GetString( Json_GetMemberByName( KHR_skin_culling, "jointGeometryMins" ), "" );
					const ksGltfAccessor * minsAccessor = ksGltf_GetAccessorByNameAndType( scene, minsAccessorName, "VEC3", GL_FLOAT );
					scene->skins[skinIndex].jointGeometryMins = ksGltf_GetBufferData( minsAccessor );

					const char * maxsAccessorName = Json_GetString( Json_GetMemberByName( KHR_skin_culling, "jointGeometryMaxs" ), "" );
					const ksGltfAccessor * maxsAccessor = ksGltf_GetAccessorByNameAndType( scene, maxsAccessorName, "VEC3", GL_FLOAT );
					scene->skins[skinIndex].jointGeometryMaxs = ksGltf_GetBufferData( maxsAccessor );
				}
			}
		}
		ksGltf_CreateSkinNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load skins\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF cameras
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * cameras = Json_GetMemberByName( rootNode, "cameras" );
		scene->cameraCount = Json_GetMemberCount( cameras );
		scene->cameras = (ksGltfCamera *) calloc( scene->cameraCount, sizeof( ksGltfCamera ) );
		for ( int cameraIndex = 0; cameraIndex < scene->cameraCount; cameraIndex++ )
		{
			const Json_t * camera = Json_GetMemberByIndex( cameras, cameraIndex );
			const char * type = Json_GetString( Json_GetMemberByName( camera, "type" ), "" );
			scene->cameras[cameraIndex].name = ksGltf_strdup( Json_GetMemberName( camera ) );
			if ( strcmp( type, "perspective" ) == 0 )
			{
				const Json_t * perspective = Json_GetMemberByName( camera, "perspective" );
				const float aspectRatio = Json_GetFloat( Json_GetMemberByName( perspective, "aspectRatio" ), 0.0f );
				const float yfov = Json_GetFloat( Json_GetMemberByName( perspective, "yfov" ), 0.0f );
				scene->cameras[cameraIndex].type = GLTF_CAMERA_TYPE_PERSPECTIVE;
				scene->cameras[cameraIndex].perspective.fovDegreesX = ( 180.0f / MATH_PI ) * 2.0f * atanf( tanf( yfov * 0.5f ) * aspectRatio );
				scene->cameras[cameraIndex].perspective.fovDegreesY = ( 180.0f / MATH_PI ) * yfov;
				scene->cameras[cameraIndex].perspective.nearZ = Json_GetFloat( Json_GetMemberByName( perspective, "znear" ), 0.0f );
				scene->cameras[cameraIndex].perspective.farZ = Json_GetFloat( Json_GetMemberByName( perspective, "zfar" ), 0.0f );
				assert( scene->cameras[cameraIndex].perspective.fovDegreesX > 0.0f );
				assert( scene->cameras[cameraIndex].perspective.fovDegreesY > 0.0f );
				assert( scene->cameras[cameraIndex].perspective.nearZ > 0.0f );
			}
			else
			{
				const Json_t * orthographic = Json_GetMemberByName( camera, "orthographic" );
				scene->cameras[cameraIndex].type = GLTF_CAMERA_TYPE_ORTHOGRAPHIC;
				scene->cameras[cameraIndex].orthographic.magX = Json_GetFloat( Json_GetMemberByName( orthographic, "xmag" ), 0.0f );
				scene->cameras[cameraIndex].orthographic.magY = Json_GetFloat( Json_GetMemberByName( orthographic, "ymag" ), 0.0f );
				scene->cameras[cameraIndex].orthographic.nearZ = Json_GetFloat( Json_GetMemberByName( orthographic, "znear" ), 0.0f );
				scene->cameras[cameraIndex].orthographic.farZ = Json_GetFloat( Json_GetMemberByName( orthographic, "zfar" ), 0.0f );
				assert( scene->cameras[cameraIndex].orthographic.magX > 0.0f );
				assert( scene->cameras[cameraIndex].orthographic.magY > 0.0f );
				assert( scene->cameras[cameraIndex].orthographic.nearZ > 0.0f );
			}
		}
		ksGltf_CreateCameraNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load cameras\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// glTF nodes
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const Json_t * nodes = Json_GetMemberByName( rootNode, "nodes" );
		scene->nodeCount = Json_GetMemberCount( nodes );
		scene->nodes = (ksGltfNode *) calloc( scene->nodeCount, sizeof( ksGltfNode ) );
		for ( int nodeIndex = 0; nodeIndex < scene->nodeCount; nodeIndex++ )
		{
			const Json_t * node = Json_GetMemberByIndex( nodes, nodeIndex );
			scene->nodes[nodeIndex].name = ksGltf_strdup( Json_GetMemberName( node ) );
			scene->nodes[nodeIndex].jointName = ksGltf_strdup( Json_GetString( Json_GetMemberByName( node, "jointName" ), "" ) );
			const Json_t * matrix = Json_GetMemberByName( node, "matrix" );
			if ( Json_IsArray( matrix ) )
			{
				ksMatrix4x4f localTransform;
				ksGltf_ParseFloatArray( localTransform.m[0], 16, matrix );
				ksMatrix4x4f_GetTranslation( &scene->nodes[nodeIndex].translation, &localTransform );
				ksMatrix4x4f_GetRotation( &scene->nodes[nodeIndex].rotation, &localTransform );
				ksMatrix4x4f_GetScale( &scene->nodes[nodeIndex].scale, &localTransform );
			}
			else
			{
				ksGltf_ParseFloatArray( &scene->nodes[nodeIndex].rotation.x, 4, Json_GetMemberByName( node, "rotation" ) );
				ksGltf_ParseFloatArray( &scene->nodes[nodeIndex].scale.x, 3, Json_GetMemberByName( node, "scale" ) );
				ksGltf_ParseFloatArray( &scene->nodes[nodeIndex].translation.x, 3, Json_GetMemberByName( node, "translation" ) );
			}

			assert( scene->nodes[nodeIndex].name[0] != '\0' );

			const Json_t * children = Json_GetMemberByName( node, "children" );
			scene->nodes[nodeIndex].childCount = Json_GetMemberCount( children );
			scene->nodes[nodeIndex].childNames = (char **) calloc( scene->nodes[nodeIndex].childCount, sizeof( char * ) );
			for ( int c = 0; c < scene->nodes[nodeIndex].childCount; c++ )
			{
				scene->nodes[nodeIndex].childNames[c] = ksGltf_strdup( Json_GetString( Json_GetMemberByIndex( children, c ), "" ) );
			}
			scene->nodes[nodeIndex].camera = ksGltf_GetCameraByName( scene, Json_GetString( Json_GetMemberByName( node, "camera" ), "" ) );
			scene->nodes[nodeIndex].skin = ksGltf_GetSkinByName( scene, Json_GetString( Json_GetMemberByName( node, "skin" ), "" ) );
			const Json_t * meshes = Json_GetMemberByName( node, "meshes" );
			scene->nodes[nodeIndex].modelCount = Json_GetMemberCount( meshes );
			scene->nodes[nodeIndex].models = (ksGltfModel **) calloc( scene->nodes[nodeIndex].modelCount, sizeof( ksGltfModel ** ) );
			for ( int m = 0; m < scene->nodes[nodeIndex].modelCount; m++ )
			{
				scene->nodes[nodeIndex].models[m] = ksGltf_GetModelByName( scene, Json_GetString( Json_GetMemberByIndex( meshes, m ), "" ) );
				assert( scene->nodes[nodeIndex].models[m] != NULL );
			}
		}
		ksGltf_SortNodes( scene->nodes, scene->nodeCount );
		ksGltf_CreateNodeNameHash( scene );
		ksGltf_CreateNodeJointNameHash( scene );

		const ksNanoseconds endTime = GetTimeNanoseconds();
		Print( "%1.3f seconds to load nodes\n", ( endTime - startTime ) * 1e-9f );
	}

	//
	// Assign node pointers now that the nodes are sorted and the hash is setup.
	//
	{
		// Get the node children and parents.
		for ( int nodeIndex = 0; nodeIndex < scene->nodeCount; nodeIndex++ )
		{
			ksGltfNode * node = &scene->nodes[nodeIndex];
			node->children = (ksGltfNode **) calloc( node->childCount, sizeof( ksGltfNode * ) );
			for ( int childIndex = 0; childIndex < node->childCount; childIndex++ )
			{
				node->children[childIndex] = ksGltf_GetNodeByName( scene, node->childNames[childIndex] );
				assert( node->children[childIndex] != NULL );
				node->children[childIndex]->parent = node;
			}
		}
		// Get the animated nodes.
		for ( int animationIndex = 0; animationIndex < scene->animationCount; animationIndex++ )
		{
			for ( int channelIndex = 0; channelIndex < scene->animations[animationIndex].channelCount; channelIndex++ )
			{
				scene->animations[animationIndex].channels[channelIndex].node = ksGltf_GetNodeByName( scene, scene->animations[animationIndex].channels[channelIndex].nodeName );
				assert( scene->animations[animationIndex].channels[channelIndex].node != NULL );
			}
		}
		// Get the skin joint nodes.
		for ( int skinIndex = 0; skinIndex < scene->skinCount; skinIndex++ )
		{
			for ( int jointIndex = 0; jointIndex < scene->skins[skinIndex].jointCount; jointIndex++ )
			{
				scene->skins[skinIndex].joints[jointIndex].node = ksGltf_GetNodeByJointName( scene, scene->skins[skinIndex].joints[jointIndex].name );
				assert( scene->skins[skinIndex].joints[jointIndex].node != NULL );
			}
			// Find the parent of the root node of the skin.
			ksGltfNode * root = NULL;
			for ( int jointIndex = 0; jointIndex < scene->skins[skinIndex].jointCount && root == NULL; jointIndex++ )
			{
				root = scene->skins[skinIndex].joints[jointIndex].node;
				for ( int k = 0; k < scene->skins[skinIndex].jointCount; k++ )
				{
					if ( root->parent == scene->skins[skinIndex].joints[k].node )
					{
						root = NULL;
						break;
					}
				}
			}
			scene->skins[skinIndex].parentNode = root->parent;
		}
	}

	//
	// glTF sub-scenes
	//
	{
		const Json_t * subScenes = Json_GetMemberByName( rootNode, "scenes" );
		scene->subTreeCount = 0;
		scene->subTrees = (ksGltfSubTree *) calloc( scene->nodeCount, sizeof( ksGltfSubTree ) );
		scene->subSceneCount = Json_GetMemberCount( subScenes );
		scene->subScenes = (ksGltfSubScene *) calloc( scene->subSceneCount, sizeof( ksGltfSubScene ) );
		for ( int subSceneIndex = 0; subSceneIndex < scene->subSceneCount; subSceneIndex++ )
		{
			const Json_t * subScene = Json_GetMemberByIndex( subScenes, subSceneIndex );
			scene->subScenes[subSceneIndex].name = ksGltf_strdup( Json_GetMemberName( subScene ) );

			const Json_t * nodes = Json_GetMemberByName( subScene, "nodes" );
			scene->subScenes[subSceneIndex].subTreeCount = Json_GetMemberCount( nodes );
			scene->subScenes[subSceneIndex].subTrees = (ksGltfSubTree **) calloc( scene->subScenes[subSceneIndex].subTreeCount, sizeof( ksGltfSubTree * ) );

			for ( int subTreeIndex = 0; subTreeIndex < scene->subScenes[subSceneIndex].subTreeCount; subTreeIndex++ )
			{
				const char * nodeName = Json_GetString( Json_GetMemberByIndex( nodes, subTreeIndex ), "" );

				scene->subScenes[subSceneIndex].subTrees[subTreeIndex] = NULL;
				for ( int i = 0; i < scene->subTreeCount; i++ )
				{
					if ( strcmp( scene->subTrees[i].name, nodeName ) == 0 )
					{
						scene->subScenes[subSceneIndex].subTrees[subTreeIndex] = &scene->subTrees[i];
						break;
					}
				}

				if ( scene->subScenes[subSceneIndex].subTrees[subTreeIndex] == NULL )
				{
					ksGltfSubTree * subTree = &scene->subTrees[scene->subTreeCount++];
					subTree->name = ksGltf_strdup( nodeName );

					ksGltfNode * subTreeRootNode = ksGltf_GetNodeByName( scene, nodeName );
					assert( subTreeRootNode != NULL );

					subTree->nodes = (ksGltfNode **) calloc( subTreeRootNode->subTreeNodeCount, sizeof( ksGltfNode * ) );
					subTree->nodeCount = subTreeRootNode->subTreeNodeCount;
					for ( int nodeIndex = 0; nodeIndex < subTreeRootNode->subTreeNodeCount; nodeIndex++ )
					{
						// Note that the nodes of one subtree should be laid out consecutively in memory after sorting the nodes.
						subTree->nodes[nodeIndex] = subTreeRootNode + nodeIndex;
					}
					subTree->timeLines = (ksGltfTimeLine **) calloc( scene->timeLineCount, sizeof( ksGltfTimeLine * ) );
					subTree->timeLineCount = 0;
					subTree->animations = (ksGltfAnimation **) calloc( scene->animationCount, sizeof( ksGltfAnimation * ) );
					subTree->animationCount = 0;
					for ( int animationIndex = 0; animationIndex < scene->animationCount; animationIndex++ )
					{
						bool include = false;
						ksGltfAnimation * animation = &scene->animations[animationIndex];
						for ( int channelIndex = 0; animation->channelCount; channelIndex++ )
						{
							if ( animation->channels[channelIndex].node >= subTreeRootNode &&
									animation->channels[channelIndex].node < subTreeRootNode + subTreeRootNode->subTreeNodeCount )
							{
								include = true;
								break;
							}
						}
						if ( include )
						{
							for ( int i = 0; i < subTree->animationCount; i++ )
							{
								if ( subTree->animations[i] == animation )
								{
									include = false;
									break;
								}
							}
							if ( include )
							{
								subTree->animations[subTree->animationCount++] = animation;
								for ( int i = 0; i < subTree->timeLineCount; i++ )
								{
									if ( subTree->timeLines[i] == animation->timeLine )
									{
										include = false;
										break;
									}
								}
								if ( include )
								{
									subTree->timeLines[subTree->timeLineCount++] = animation->timeLine;
								}
							}
						}
					}

					scene->subScenes[subSceneIndex].subTrees[subTreeIndex] = subTree;
				}
			}
		}
		ksGltf_CreateSubTreeNameHash( scene );
		ksGltf_CreateSubSceneNameHash( scene );
	}

	//
	// glTF default scene
	//

	const char * defaultSceneName = Json_GetString( Json_GetMemberByName( rootNode, "scene" ), "" );
	scene->state.currentSubScene = ksGltf_GetSubSceneByName( scene, defaultSceneName );
	assert( scene->state.currentSubScene != NULL );

	Json_Destroy( rootNode );

	// Allocate run-time state memory.
	scene->state.timeLineFrameState = (ksGltfTimeLineFrameState *) calloc( scene->timeLineCount, sizeof( ksGltfTimeLineFrameState ) );
	scene->state.skinCullingState = (ksGltfSkinCullingState *) calloc( scene->skinCount, sizeof( ksGltfSkinCullingState ) );
	for ( int skinIndex = 0; skinIndex < scene->skinCount; skinIndex++ )
	{
		ksGltfSkinCullingState * skinCullingState = &scene->state.skinCullingState[skinIndex];
		ksVector3f_Set( &skinCullingState->mins, FLT_MAX );
		ksVector3f_Set( &skinCullingState->maxs, -FLT_MAX );
		skinCullingState->culled = false;
	}
	scene->state.nodeState = (ksGltfNodeState *) calloc( scene->nodeCount, sizeof( ksGltfNodeState ) );
	for ( int nodeIndex = 0; nodeIndex < scene->nodeCount; nodeIndex++ )
	{
		const ksGltfNode * node = &scene->nodes[nodeIndex];
		ksGltfNodeState * nodeState = &scene->state.nodeState[nodeIndex];
		nodeState->parent = ( node->parent != NULL ) ? &scene->state.nodeState[(int)( node->parent - scene->nodes )] : NULL;
		nodeState->translation = node->translation;
		nodeState->rotation = node->rotation;
		nodeState->scale = node->scale;
		ksMatrix4x4f_CreateIdentity( &nodeState->localTransform );
		ksMatrix4x4f_CreateIdentity( &nodeState->globalTransform );
	}
	scene->state.subTreeState = (ksGltfSubTreeState *) calloc( scene->subTreeCount, sizeof( ksGltfSubTreeState ) );
	for ( int subTreeIndex = 0; subTreeIndex < scene->subTreeCount; subTreeIndex++ )
	{
		scene->state.subTreeState[subTreeIndex].visible = true;
	}

	// Create view projection uniform buffer.
	{
		ksGpuBuffer_Create( context, &scene->viewProjectionBuffer, KS_GPU_BUFFER_TYPE_UNIFORM, 4 * sizeof( ksMatrix4x4f ), NULL, false );
	}

	// Create a default joint uniform buffer.
	{
		ksMatrix4x4f * data = malloc( MAX_JOINTS * sizeof( ksMatrix4x4f ) );
		for ( int jointIndex = 0; jointIndex < MAX_JOINTS; jointIndex++ )
		{
			ksMatrix4x4f_CreateIdentity( &data[jointIndex] );
		}
		ksGpuBuffer_Create( context, &scene->defaultJointBuffer, KS_GPU_BUFFER_TYPE_UNIFORM, MAX_JOINTS * sizeof( ksMatrix4x4f ), data, false );
		free( data );
	}

	// Create unit cube.
	{
		ksGpuGeometry_CreateCube( context, &scene->unitCubeGeometry, 0.0f, 1.0f );
		ksGpuGraphicsProgram_Create( context, &scene->unitCubeFlatShadeProgram,
									PROGRAM( unitCubeFlatShadeVertexProgram ), sizeof( PROGRAM( unitCubeFlatShadeVertexProgram ) ),
									PROGRAM( unitCubeFlatShadeFragmentProgram ), sizeof( PROGRAM( unitCubeFlatShadeFragmentProgram ) ),
									unitCubeFlatShadeProgramParms, ARRAY_SIZE( unitCubeFlatShadeProgramParms ),
									scene->unitCubeGeometry.layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL );

		ksGpuGraphicsPipelineParms pipelineParms;
		ksGpuGraphicsPipelineParms_Init( &pipelineParms );

		pipelineParms.renderPass = renderPass;
		pipelineParms.program = &scene->unitCubeFlatShadeProgram;
		pipelineParms.geometry = &scene->unitCubeGeometry;

		ksGpuGraphicsPipeline_Create( context, &scene->unitCubePipeline, &pipelineParms );
	}

	const ksNanoseconds t1 = GetTimeNanoseconds();

	Print( "%1.3f seconds to load %s\n", ( t1 - t0 ) * 1e-9f, fileName );

	return true;
}

static void ksGltfScene_Destroy( ksGpuContext * context, ksGltfScene * scene )
{
	ksGpuContext_WaitIdle( context );

	{
		free( scene->state.timeLineFrameState );
		free( scene->state.skinCullingState );
		free( scene->state.nodeState );
		free( scene->state.subTreeState );
	}
	{
		for ( int bufferIndex = 0; bufferIndex < scene->bufferCount; bufferIndex++ )
		{
			free( scene->buffers[bufferIndex].name );
			free( scene->buffers[bufferIndex].type );
			free( scene->buffers[bufferIndex].bufferData );
		}
		free( scene->buffers );
		free( scene->bufferNameHash );
	}
	{
		for ( int bufferViewIndex = 0; bufferViewIndex < scene->bufferViewCount; bufferViewIndex++ )
		{
			free( scene->bufferViews[bufferViewIndex].name );
		}
		free( scene->bufferViews );
		free( scene->bufferViewNameHash );
	}
	{
		for ( int accessorIndex = 0; accessorIndex < scene->accessorCount; accessorIndex++ )
		{
			free( scene->accessors[accessorIndex].name );
			free( scene->accessors[accessorIndex].type );
		}
		free( scene->accessors );
		free( scene->accessorNameHash );
	}
	{
		for ( int imageIndex = 0; imageIndex < scene->imageCount; imageIndex++ )
		{
			free( scene->images[imageIndex].name );
			for ( int versionIndex = 0; versionIndex < scene->images[imageIndex].versionCount; versionIndex++ )
			{
				free( scene->images[imageIndex].versions[versionIndex].container );
				free( scene->images[imageIndex].versions[versionIndex].uri );
			}
		}
		free( scene->images );
		free( scene->imageNameHash );
	}
	{
		for ( int textureIndex = 0; textureIndex < scene->textureCount; textureIndex++ )
		{
			free( scene->textures[textureIndex].name );
			ksGpuTexture_Destroy( context, &scene->textures[textureIndex].texture );
		}
		free( scene->textures );
		free( scene->textureNameHash );
	}
	{
		for ( int shaderIndex = 0; shaderIndex < scene->shaderCount; shaderIndex++ )
		{
			free( scene->shaders[shaderIndex].name );
			for ( int shaderType = 0; shaderType < GLTF_SHADER_TYPE_MAX; shaderType++ )
			{
				for ( int index = 0; index < scene->shaders[shaderIndex].shaderCount[shaderType]; index++ )
				{
					free( scene->shaders[shaderIndex].shaders[shaderType][index].api );
					free( scene->shaders[shaderIndex].shaders[shaderType][index].version );
					free( scene->shaders[shaderIndex].shaders[shaderType][index].uri );
				}
				free( scene->shaders[shaderIndex].shaders[shaderType] );
			}
		}
		free( scene->shaders );
		free( scene->shaderNameHash );
	}
	{
		for ( int programIndex = 0; programIndex < scene->programCount; programIndex++ )
		{
			free( scene->programs[programIndex].name );
			free( scene->programs[programIndex].vertexSource );
			free( scene->programs[programIndex].fragmentSource );
		}
		free( scene->programs );
		free( scene->programNameHash );
	}
	{
		for ( int techniqueIndex = 0; techniqueIndex < scene->techniqueCount; techniqueIndex++ )
		{
			for ( int uniformIndex = 0; uniformIndex < scene->techniques[techniqueIndex].uniformCount; uniformIndex++ )
			{
				free( (void *)scene->techniques[techniqueIndex].parms[uniformIndex].name );
				free( scene->techniques[techniqueIndex].uniforms[uniformIndex].name );
			}
			for ( int attributeIndex = 0; attributeIndex < scene->techniques[techniqueIndex].attributeCount; attributeIndex++ )
			{
				free( scene->techniques[techniqueIndex].attributes[attributeIndex].name );
			}
			free( scene->techniques[techniqueIndex].name );
			free( scene->techniques[techniqueIndex].parms );
			free( scene->techniques[techniqueIndex].uniforms );
			free( scene->techniques[techniqueIndex].attributes );
			free( scene->techniques[techniqueIndex].vertexAttributeLayout );
			ksGpuGraphicsProgram_Destroy( context, &scene->techniques[techniqueIndex].program );
		}
		free( scene->techniques );
		free( scene->techniqueNameHash );
	}
	{
		for ( int materialIndex = 0; materialIndex < scene->materialCount; materialIndex++ )
		{
			free( scene->materials[materialIndex].name );
			free( scene->materials[materialIndex].values );
		}
		free( scene->materials );
		free( scene->materialNameHash );
	}
	{
		for ( int modelIndex = 0; modelIndex < scene->modelCount; modelIndex++ )
		{
			for ( int surfaceIndex = 0; surfaceIndex < scene->models[modelIndex].surfaceCount; surfaceIndex++ )
			{
				ksGpuGeometry_Destroy( context, &scene->models[modelIndex].surfaces[surfaceIndex].geometry );
				ksGpuGraphicsPipeline_Destroy( context, &scene->models[modelIndex].surfaces[surfaceIndex].pipeline );
			}
			free( scene->models[modelIndex].name );
			free( scene->models[modelIndex].surfaces );
		}
		free( scene->models );
		free( scene->modelNameHash );
	}
	{
		free( scene->timeLines );
		free( scene->timeLineNameHash );
	}
	{
		for ( int animationIndex = 0; animationIndex < scene->animationCount; animationIndex++ )
		{
			for ( int channelIndex = 0; channelIndex < scene->animations[animationIndex].channelCount; channelIndex++ )
			{
				free( scene->animations[animationIndex].channels[channelIndex].nodeName );
			}
			free( scene->animations[animationIndex].name );
			free( scene->animations[animationIndex].channels );
		}
		free( scene->animations );
		free( scene->animationNameHash );
	}
	{
		for ( int skinIndex = 0; skinIndex < scene->skinCount; skinIndex++ )
		{
			for ( int jointIndex = 0; jointIndex < scene->skins[skinIndex].jointCount; jointIndex++ )
			{
				free( scene->skins[skinIndex].joints[jointIndex].name );
			}
			free( scene->skins[skinIndex].name );
			free( scene->skins[skinIndex].joints );
			ksGpuBuffer_Destroy( context, &scene->skins[skinIndex].jointBuffer );
		}
		free( scene->skins );
		free( scene->skinNameHash );
	}
	{
		for ( int cameraIndex = 0; cameraIndex < scene->cameraCount; cameraIndex++ )
		{
			free( scene->cameras[cameraIndex].name );
		}
		free( scene->cameras );
		free( scene->cameraNameHash );
	}
	{
		for ( int nodeIndex = 0; nodeIndex < scene->nodeCount; nodeIndex++ )
		{
			for ( int childIndex = 0; childIndex < scene->nodes[nodeIndex].childCount; childIndex++ )
			{
				free( scene->nodes[nodeIndex].childNames[childIndex] );
			}
			free( scene->nodes[nodeIndex].name );
			free( scene->nodes[nodeIndex].jointName );
			free( scene->nodes[nodeIndex].children );
			free( scene->nodes[nodeIndex].childNames );
			free( scene->nodes[nodeIndex].models );
		}
		free( scene->nodes );
		free( scene->nodeNameHash );
		free( scene->nodeJointNameHash );
	}
	{
		for ( int subTreeIndex = 0; subTreeIndex < scene->subTreeCount; subTreeIndex++ )
		{
			free( scene->subTrees[subTreeIndex].nodes );
			free( scene->subTrees[subTreeIndex].timeLines );
			free( scene->subTrees[subTreeIndex].animations );
		}
		free( scene->subTrees );
	}
	{
		for ( int subSceneIndex = 0; subSceneIndex < scene->subSceneCount; subSceneIndex++ )
		{
			free( scene->subScenes[subSceneIndex].subTrees );
		}
		free( scene->subScenes );
		free( scene->subSceneNameHash );
	}

	ksGpuBuffer_Destroy( context, &scene->viewProjectionBuffer );
	ksGpuBuffer_Destroy( context, &scene->defaultJointBuffer );
	ksGpuGraphicsPipeline_Destroy( context, &scene->unitCubePipeline );
	ksGpuGraphicsProgram_Destroy( context, &scene->unitCubeFlatShadeProgram );
	ksGpuGeometry_Destroy( context, &scene->unitCubeGeometry );

	memset( scene, 0, sizeof( ksGltfScene ) );
}

static void ksGltfScene_SetSubScene( ksGltfScene * scene, const char * subSceneName )
{
	ksGltfSubScene * subScene = ksGltf_GetSubSceneByName( scene, subSceneName );
	assert( subScene != NULL );
	if ( subScene != NULL )
	{
		scene->state.currentSubScene = subScene;
	}
}

static void ksGltfScene_SetSubTreeVisible( ksGltfScene * scene, const char * subTreeName, const bool visible )
{
	ksGltfSubTree * subTree = ksGltf_GetSubTreeByName( scene, subTreeName );
	assert( subTree != NULL );
	if ( subTree != NULL )
	{
		scene->state.subTreeState[(int)( subTree - scene->subTrees )].visible = visible;
	}
}

static void ksGltfScene_SetAnimationEnabled( ksGltfScene * scene, const char * animationName, const bool enabled )
{
	ksGltfAnimation * animation = ksGltf_GetAnimationByName( scene, animationName );
	assert( animation != NULL );
	if ( animation != NULL )
	{
		UNUSED_PARM( enabled );
	}
}

static void ksGltfScene_SetNodeTranslation( ksGltfScene * scene, const char * nodeName, const ksVector3f * translation )
{
	ksGltfNode * node = ksGltf_GetNodeByName( scene, nodeName );
	assert( node != NULL );
	if ( node != NULL )
	{
		scene->state.nodeState[(int)( node - scene->nodes )].translation = *translation;
	}
}

static void ksGltfScene_SetNodeRotation( ksGltfScene * scene, const char * nodeName, const ksQuatf * rotation )
{
	ksGltfNode * node = ksGltf_GetNodeByName( scene, nodeName );
	assert( node != NULL );
	if ( node != NULL )
	{
		scene->state.nodeState[(int)( node - scene->nodes )].rotation = *rotation;
	}
}

static void ksGltfScene_SetNodeScale( ksGltfScene * scene, const char * nodeName, const ksVector3f * scale )
{
	ksGltfNode * node = ksGltf_GetNodeByName( scene, nodeName );
	assert( node != NULL );
	if ( node != NULL )
	{
		scene->state.nodeState[(int)( node - scene->nodes )].scale = *scale;
	}
}

static void ksGltfScene_Simulate( ksGltfScene * scene, ksViewState * viewState, ksGpuWindowInput * input, const ksNanoseconds time )
{
	const ksGltfNode * cameraNode = NULL;

	// Go through all current sub-trees.
	for ( int subTreeIndex = 0; subTreeIndex < scene->state.currentSubScene->subTreeCount; subTreeIndex++ )
	{
		ksGltfSubTree * subTree = scene->state.currentSubScene->subTrees[subTreeIndex];
		if ( !scene->state.subTreeState[(int)( subTree - scene->subTrees )].visible )
		{
			continue;
		}

		// Get the current frame index and frame fraction for each time line.
		for ( int timeLineIndex = 0; timeLineIndex < subTree->timeLineCount; timeLineIndex++ )
		{
			ksGltfTimeLine * timeLine = subTree->timeLines[timeLineIndex];
			const float timeInSeconds = fmodf( time * 1e-9f, timeLine->duration );
			int frame = 0;
			if ( timeLine->rcpStep != 0.0f )
			{
				// Use direct lookup if this is a fixed rate animation.
				frame = (int)( timeInSeconds * timeLine->rcpStep );
			}
			else
			{
				// Use a binary search to find the key frame.
				for ( int sampleCount = timeLine->sampleCount; sampleCount > 1; sampleCount >>= 1 )
				{
					const int mid = sampleCount >> 1;
					if ( timeInSeconds >= timeLine->sampleTimes[frame + mid] )
					{
						frame += mid;
						sampleCount = ( sampleCount - mid ) * 2;
					}
				}
			}
			assert( timeInSeconds >= timeLine->sampleTimes[frame] && timeInSeconds < timeLine->sampleTimes[frame + 1] );
			scene->state.timeLineFrameState[timeLineIndex].frame = frame;
			scene->state.timeLineFrameState[timeLineIndex].fraction = ( timeInSeconds - timeLine->sampleTimes[frame] ) / ( timeLine->sampleTimes[frame + 1] - timeLine->sampleTimes[frame] );
		}

		// Apply animations to the nodes in the hierarchy.
		for ( int animIndex = 0; animIndex < subTree->animationCount; animIndex++ )
		{
			const ksGltfAnimation * animation = subTree->animations[animIndex];

			const int timeLineIndex = (int)( animation->timeLine - scene->timeLines );
			const int frame = scene->state.timeLineFrameState[timeLineIndex].frame;
			const float fraction = scene->state.timeLineFrameState[timeLineIndex].fraction;

			for ( int channelIndex = 0; channelIndex < animation->channelCount; channelIndex++ )
			{
				const ksGltfAnimationChannel * channel = &animation->channels[channelIndex];
				ksGltfNodeState * nodeState = &scene->state.nodeState[(int)( channel->node - scene->nodes )];
				if ( channel->translation != NULL )
				{
					ksVector3f_Lerp( &nodeState->translation, &channel->translation[frame], &channel->translation[frame + 1], fraction );
				}
				if ( channel->rotation != NULL )
				{
					ksQuatf_Lerp( &nodeState->rotation, &channel->rotation[frame], &channel->rotation[frame + 1], fraction );
				}
				if ( channel->scale != NULL )
				{
					ksVector3f_Lerp( &nodeState->scale, &channel->scale[frame], &channel->scale[frame + 1], fraction );
				}
			}
		}

		// Transform the node hierarchy into global space.
		for ( int nodeIndex = 0; nodeIndex < scene->nodeCount; nodeIndex++ )
		{
			ksGltfNodeState * nodeState = &scene->state.nodeState[(int)( subTree->nodes[nodeIndex] - scene->nodes )];

			ksMatrix4x4f_CreateTranslationRotationScale( &nodeState->localTransform, &nodeState->translation, &nodeState->rotation, &nodeState->scale );
			if ( nodeState->parent != NULL )
			{
				assert( nodeState->parent < nodeState );
				ksMatrix4x4f_Multiply( &nodeState->globalTransform, &nodeState->parent->globalTransform, &nodeState->localTransform );
			}
			else
			{
				nodeState->globalTransform = nodeState->localTransform;
			}
		}

		// Find a camera if no camera has been found yet.
		if ( cameraNode == NULL )
		{
			for ( int nodeIndex = 0; nodeIndex < subTree->nodeCount; nodeIndex++ )
			{
				ksGltfNode * node = subTree->nodes[nodeIndex];
				if ( node->camera != NULL )
				{
					cameraNode = node;
					break;
				}
			}
		}
	}

	// Use the camera if there is one, otherwise use input to move the view point.
	if ( cameraNode != NULL )
	{
		GetHmdViewMatrixForTime( &viewState->displayViewMatrix, time );

		ksMatrix4x4f cameraViewMatrix;
		ksMatrix4x4f_Invert( &cameraViewMatrix, &scene->state.nodeState[(int)( cameraNode - scene->nodes )].globalTransform );

		ksMatrix4x4f centerViewMatrix;
		ksMatrix4x4f_Multiply( &centerViewMatrix, &viewState->displayViewMatrix, &cameraViewMatrix );

		for ( int eye = 0; eye < NUM_EYES; eye++ )
		{
			ksMatrix4x4f eyeOffsetMatrix;
			ksMatrix4x4f_CreateTranslation( &eyeOffsetMatrix, ( eye ? -0.5f : 0.5f ) * viewState->interpupillaryDistance, 0.0f, 0.0f );

			ksMatrix4x4f_Multiply( &viewState->viewMatrix[eye], &eyeOffsetMatrix, &centerViewMatrix );
			ksMatrix4x4f_CreateProjectionFov( &viewState->projectionMatrix[eye],
											cameraNode->camera->perspective.fovDegreesX * 0.5f,
											cameraNode->camera->perspective.fovDegreesX * 0.5f,
											cameraNode->camera->perspective.fovDegreesY * 0.5f,
											cameraNode->camera->perspective.fovDegreesY * 0.5f,
											cameraNode->camera->perspective.nearZ, cameraNode->camera->perspective.farZ );

			ksViewState_DerivedData( viewState, &centerViewMatrix );
		}
	}
	else if ( input != NULL )
	{
		ksViewState_HandleInput( viewState, input, time );
	}
	else
	{
		ksViewState_HandleHmd( viewState, time );
	}
}

static void ksGltfScene_UpdateBuffers( ksGpuCommandBuffer * commandBuffer, ksGltfScene * scene, const ksViewState * viewState, const int eye )
{
	// Update the view projection uniform buffer
	ksMatrix4x4f * matrices;
	ksGpuBuffer * mappedViewProjectionBuffer = ksGpuCommandBuffer_MapBuffer( commandBuffer, &scene->viewProjectionBuffer, (void **)&matrices );
	const int count = ( eye == 2 ) ? 2 : 1;
	memcpy( matrices + 0 * count, &viewState->viewMatrix[eye], count * sizeof( ksMatrix4x4f ) );
	memcpy( matrices + 1 * count, &viewState->viewInverseMatrix[eye], count * sizeof( ksMatrix4x4f ) );
	memcpy( matrices + 2 * count, &viewState->projectionMatrix[eye], count * sizeof( ksMatrix4x4f ) );
	memcpy( matrices + 3 * count, &viewState->projectionInverseMatrix[eye], count * sizeof( ksMatrix4x4f ) );
	ksGpuCommandBuffer_UnmapBuffer( commandBuffer, &scene->viewProjectionBuffer, mappedViewProjectionBuffer, KS_GPU_BUFFER_UNMAP_TYPE_COPY_BACK );

	// Cull skins and update any joint uniform buffers of skins that are not culled.
	for ( int subTreeIndex = 0; subTreeIndex < scene->state.currentSubScene->subTreeCount; subTreeIndex++ )
	{
		ksGltfSubTree * subTree = scene->state.currentSubScene->subTrees[subTreeIndex];
		if ( !scene->state.subTreeState[(int)( subTree - scene->subTrees )].visible )
		{
			continue;
		}

		for ( int nodeIndex = 0; nodeIndex < subTree->nodeCount; nodeIndex++ )
		{
			ksGltfNode * node = subTree->nodes[nodeIndex];
			ksGltfSkin * skin = node->skin;
			if ( skin == NULL )
			{
				continue;
			}
	
			const ksGltfNodeState * parentNodeState = &scene->state.nodeState[(int)( skin->parentNode - scene->nodes )];

			// Exclude the transform of the whole skeleton because that transform will be
			// passed down the vertex shader as the model matrix.
			ksMatrix4x4f inverseGlobalSkeletonTransfom;
			ksMatrix4x4f_Invert( &inverseGlobalSkeletonTransfom, &parentNodeState->globalTransform );

			// Calculate the skin bounds.
			if ( skin->jointGeometryMins != NULL && skin->jointGeometryMaxs != NULL )
			{
				ksGltfSkinCullingState * skinCullingState = &scene->state.skinCullingState[(int)( skin - scene->skins )];

				for ( int jointIndex = 0; jointIndex < skin->jointCount; jointIndex++ )
				{
					const ksGltfNodeState * jointNodeState = &scene->state.nodeState[(int)( skin->joints[jointIndex].node - scene->nodes )];

					ksMatrix4x4f localJointTransform;
					ksMatrix4x4f_Multiply( &localJointTransform, &inverseGlobalSkeletonTransfom, &jointNodeState->globalTransform );

					ksVector3f jointMins;
					ksVector3f jointMaxs;
					ksMatrix4x4f_TransformBounds( &jointMins, &jointMaxs, &localJointTransform, &skin->jointGeometryMins[jointIndex], &skin->jointGeometryMaxs[jointIndex] );
					ksVector3f_Min( &skinCullingState->mins, &skin->mins, &jointMins );
					ksVector3f_Max( &skinCullingState->maxs, &skin->maxs, &jointMaxs );
				}

				// Do not update the joint buffer if the skin bounds are culled.
				ksMatrix4x4f modelViewProjectionCullMatrix;
				ksMatrix4x4f_Multiply( &modelViewProjectionCullMatrix, &viewState->combinedViewProjectionMatrix, &parentNodeState->globalTransform );

				skinCullingState->culled = ksMatrix4x4f_CullBounds( &modelViewProjectionCullMatrix, &skin->mins, &skin->maxs );
				if ( skinCullingState->culled )
				{
					continue;
				}
			}

			// Update the skin joint buffer.
			ksMatrix4x4f * joints = NULL;
			ksGpuBuffer * mappedJointBuffer = ksGpuCommandBuffer_MapBuffer( commandBuffer, &skin->jointBuffer, (void **)&joints );

			for ( int jointIndex = 0; jointIndex < skin->jointCount; jointIndex++ )
			{
				const ksGltfNodeState * jointNodeState = &scene->state.nodeState[(int)( skin->joints[jointIndex].node - scene->nodes )];

				ksMatrix4x4f localJointTransform;
				ksMatrix4x4f_Multiply( &localJointTransform, &inverseGlobalSkeletonTransfom, &jointNodeState->globalTransform );
				ksMatrix4x4f_Multiply( &joints[jointIndex], &localJointTransform, &skin->inverseBindMatrices[jointIndex] );
			}

			ksGpuCommandBuffer_UnmapBuffer( commandBuffer, &skin->jointBuffer, mappedJointBuffer, KS_GPU_BUFFER_UNMAP_TYPE_COPY_BACK );
		}
	}
}

static void ksGltfScene_SetUniformValue( ksGpuGraphicsCommand * command, const ksGltfUniform * uniform, const ksGltfUniformValue * value )
{
	switch ( uniform->type )
	{
		case KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED:					ksGpuGraphicsCommand_SetParmTextureSampled( command, uniform->index, &value->texture->texture ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT:				ksGpuGraphicsCommand_SetParmInt( command, uniform->index, value->intValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2:		ksGpuGraphicsCommand_SetParmIntVector2( command, uniform->index, (const ksVector2i *)value->intValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3:		ksGpuGraphicsCommand_SetParmIntVector3( command, uniform->index, (const ksVector3i *)value->intValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4:		ksGpuGraphicsCommand_SetParmIntVector4( command, uniform->index, (const ksVector4i *)value->intValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT:				ksGpuGraphicsCommand_SetParmFloat( command, uniform->index, value->floatValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2:		ksGpuGraphicsCommand_SetParmFloatVector2( command, uniform->index, (const ksVector2f *)value->floatValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3:		ksGpuGraphicsCommand_SetParmFloatVector3( command, uniform->index, (const ksVector3f *)value->floatValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4:		ksGpuGraphicsCommand_SetParmFloatVector4( command, uniform->index, (const ksVector4f *)value->floatValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2:	ksGpuGraphicsCommand_SetParmFloatMatrix2x2( command, uniform->index, (const ksMatrix2x2f *)value->floatValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3:	ksGpuGraphicsCommand_SetParmFloatMatrix2x3( command, uniform->index, (const ksMatrix2x3f *)value->floatValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4:	ksGpuGraphicsCommand_SetParmFloatMatrix2x4( command, uniform->index, (const ksMatrix2x4f *)value->floatValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2:	ksGpuGraphicsCommand_SetParmFloatMatrix3x2( command, uniform->index, (const ksMatrix3x2f *)value->floatValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3:	ksGpuGraphicsCommand_SetParmFloatMatrix3x3( command, uniform->index, (const ksMatrix3x3f *)value->floatValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4:	ksGpuGraphicsCommand_SetParmFloatMatrix3x4( command, uniform->index, (const ksMatrix3x4f *)value->floatValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2:	ksGpuGraphicsCommand_SetParmFloatMatrix4x2( command, uniform->index, (const ksMatrix4x2f *)value->floatValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3:	ksGpuGraphicsCommand_SetParmFloatMatrix4x3( command, uniform->index, (const ksMatrix4x3f *)value->floatValue ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4:	ksGpuGraphicsCommand_SetParmFloatMatrix4x4( command, uniform->index, (const ksMatrix4x4f *)value->floatValue ); break;
		default: break;
	}
}

static void ksGltfScene_Render( ksGpuCommandBuffer * commandBuffer, const ksGltfScene * scene, const ksViewState * viewState )
{
	ksVector4f viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.z = 1.0f;
	viewport.w = 1.0f;

	for ( int subTreeIndex = 0; subTreeIndex < scene->state.currentSubScene->subTreeCount; subTreeIndex++ )
	{
		ksGltfSubTree * subTree = scene->state.currentSubScene->subTrees[subTreeIndex];
		if ( !scene->state.subTreeState[(int)( subTree - scene->subTrees )].visible )
		{
			continue;
		}

		for ( int nodeIndex = 0; nodeIndex < subTree->nodeCount; nodeIndex++ )
		{
			ksGltfNode * node = subTree->nodes[nodeIndex];
			if ( node->modelCount == 0 )
			{
				continue;
			}

			const ksGltfSkin * skin = node->skin;
			const ksGltfNode * parentNode = ( skin != NULL ) ? skin->parentNode : node;
			const int parentNodeIndex = (int)( parentNode - scene->nodes );

			ksMatrix4x4f localMatrix = scene->state.nodeState[parentNodeIndex].localTransform;
			ksMatrix4x4f modelMatrix = scene->state.nodeState[parentNodeIndex].globalTransform;
			ksMatrix4x4f modelInverseMatrix;
			ksMatrix4x4f_Invert( &modelInverseMatrix, &modelMatrix );

			if ( skin != NULL )
			{
				const ksGltfSkinCullingState * skinCullingState = &scene->state.skinCullingState[(int)( skin - scene->skins )];

				bool showSkinBounds = false;
				if ( showSkinBounds )
				{
					ksMatrix4x4f unitCubeMatrix;
					ksMatrix4x4f_CreateOffsetScaleForBounds( &unitCubeMatrix, &modelMatrix, &skinCullingState->mins, &skinCullingState->maxs );

					ksGpuGraphicsCommand command;
					ksGpuGraphicsCommand_Init( &command );
					ksGpuGraphicsCommand_SetPipeline( &command, &scene->unitCubePipeline );
					ksGpuGraphicsCommand_SetParmFloatMatrix4x4( &command, 0, &unitCubeMatrix );
					ksGpuGraphicsCommand_SetParmFloatMatrix4x4( &command, 1, &viewState->viewMatrix[0] );		// FIXME: use uniform buffer
					ksGpuGraphicsCommand_SetParmFloatMatrix4x4( &command, 2, &viewState->projectionMatrix[0] );

					ksGpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
				}

				if ( skinCullingState->culled )
				{
					continue;
				}
			}

			const ksGpuBuffer * jointBuffer = ( skin != NULL ) ? &skin->jointBuffer : &scene->defaultJointBuffer;

			ksMatrix4x4f modelViewProjectionCullMatrix;
			ksMatrix4x4f_Multiply( &modelViewProjectionCullMatrix, &viewState->combinedViewProjectionMatrix, &modelMatrix );

			for ( int modelIndex = 0; modelIndex < node->modelCount; modelIndex++ )
			{
				const ksGltfModel * model = node->models[modelIndex];

				if ( skin == NULL && ksMatrix4x4f_CullBounds( &modelViewProjectionCullMatrix, &model->mins, &model->maxs ) )
				{
					continue;
				}

				for ( int surfaceIndex = 0; surfaceIndex < model->surfaceCount; surfaceIndex++ )
				{
					const ksGltfSurface * surface = &model->surfaces[surfaceIndex];

					if ( skin == NULL && model->surfaceCount > 1 && ksMatrix4x4f_CullBounds( &modelViewProjectionCullMatrix, &surface->mins, &surface->maxs ) )
					{
						continue;
					}

					ksGpuGraphicsCommand command;
					ksGpuGraphicsCommand_Init( &command );
					ksGpuGraphicsCommand_SetPipeline( &command, &surface->pipeline );

					const ksGltfTechnique * technique = surface->material->technique;
					for ( int uniformIndex = 0; uniformIndex < technique->uniformCount; uniformIndex++ )
					{
						const ksGltfUniform * uniform = &technique->uniforms[uniformIndex];
						switch ( uniform->semantic )
						{
							case GLTF_UNIFORM_SEMANTIC_DEFAULT_VALUE:						ksGltfScene_SetUniformValue( &command, uniform, &uniform->defaultValue ); break;
							case GLTF_UNIFORM_SEMANTIC_VIEW:								assert( false ); break;	// replaced by KHR_glsl_view_projection_buffer
							case GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE:						assert( false ); break;	// replaced by KHR_glsl_view_projection_buffer
							case GLTF_UNIFORM_SEMANTIC_PROJECTION:							assert( false ); break;	// replaced by KHR_glsl_view_projection_buffer
							case GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE:					assert( false ); break;	// replaced by KHR_glsl_view_projection_buffer
							case GLTF_UNIFORM_SEMANTIC_LOCAL:								ksGpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &localMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_MODEL:								ksGpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &modelMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE:						ksGpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &modelInverseMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE_TRANSPOSE:				assert( false ); break;	// replaced by KHR_glsl_view_projection_buffer
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW:							assert( false ); break;	// replaced by KHR_glsl_view_projection_buffer
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE:					assert( false ); break;	// replaced by KHR_glsl_view_projection_buffer
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE_TRANSPOSE:		assert( false ); break;	// replaced by KHR_glsl_view_projection_buffer
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION:				assert( false ); break;	// replaced by KHR_glsl_view_projection_buffer
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION_INVERSE:		assert( false ); break;	// replaced by KHR_glsl_view_projection_buffer
							case GLTF_UNIFORM_SEMANTIC_VIEWPORT:							ksGpuGraphicsCommand_SetParmFloatVector4( &command, uniform->index, &viewport ); break;
							case GLTF_UNIFORM_SEMANTIC_JOINT_ARRAY:							assert( false ); break;	// replaced by KHR_glsl_joint_buffer
							case GLTF_UNIFORM_SEMANTIC_JOINT_BUFFER:						ksGpuGraphicsCommand_SetParmBufferUniform( &command, uniform->index, jointBuffer ); break;
							case GLTF_UNIFORM_SEMANTIC_VIEW_PROJECTION_BUFFER:				ksGpuGraphicsCommand_SetParmBufferUniform( &command, uniform->index, &scene->viewProjectionBuffer ); break;
							case GLTF_UNIFORM_SEMANTIC_VIEW_PROJECTION_MULTI_VIEW_BUFFER:	ksGpuGraphicsCommand_SetParmBufferUniform( &command, uniform->index, &scene->viewProjectionBuffer ); break;
							default: break;
						}
					}

					for ( int valueIndex = 0; valueIndex < surface->material->valueCount; valueIndex++ )
					{
						const ksGltfMaterialValue * value = &surface->material->values[valueIndex];
						if ( value->uniform != NULL )
						{
							ksGltfScene_SetUniformValue( &command, value->uniform, &value->value );
						}
					}

					ksGpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
				}
			}
		}
	}
}

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

	Print( "--------------------------------\n" );
	Print( "OS      : %s\n", GetOSVersion() );
	Print( "CPU     : %s\n", GetCPUVersion() );
	Print( "GPU     : %s\n", glGetString( GL_RENDERER ) );
	Print( "OpenGL  : %s\n", glGetString( GL_VERSION ) );
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
		{ "flatShadedMultiViewVertexProgram",		"vert",	flatShadedMultiViewVertexProgramGLSL },
		{ "flatShadedFragmentProgram",				"frag",	flatShadedFragmentProgramGLSL },
		{ "normalMappedVertexProgram",				"vert",	normalMappedVertexProgramGLSL },
		{ "normalMappedMultiViewVertexProgram",		"vert",	normalMappedMultiViewVertexProgramGLSL },
		{ "normalMapped100LightsFragmentProgram",	"frag",	normalMapped100LightsFragmentProgramGLSL },
		{ "normalMapped1000LightsFragmentProgram",	"frag",	normalMapped1000LightsFragmentProgramGLSL },
		{ "normalMapped2000LightsFragmentProgram",	"frag",	normalMapped2000LightsFragmentProgramGLSL },

#if OPENGL_COMPUTE_ENABLED == 1
		{ "barGraphComputeProgram",					"comp", barGraphComputeProgramGLSL },
		{ "timeWarpTransformComputeProgram",		"comp", timeWarpTransformComputeProgramGLSL },
		{ "timeWarpSpatialComputeProgram",			"comp", timeWarpSpatialComputeProgramGLSL },
		{ "timeWarpChromaticComputeProgram",		"comp", timeWarpChromaticComputeProgramGLSL },
#endif
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
									"glslangValidator -G -o %sSPIRV.spv %sGLSL.%s\r\n",
									glsl[i].fileName, glsl[i].fileName, glsl[i].extension );
		batchFileHexLength += sprintf( batchFileHex + batchFileHexLength,
									"glslangValidator -G -x -o %sSPIRV.h %sGLSL.%s\r\n",
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
	ksGpuContext_SetCurrent( &context );

	const int resolution = eyeResolutionTable[threadData->sceneSettings->eyeImageResolutionLevel];

	const ksGpuSampleCount sampleCount = eyeSampleCountTable[threadData->sceneSettings->eyeImageSamplesLevel];

	ksGpuRenderPass renderPass;
	ksGpuRenderPass_Create( &context, &renderPass, KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, KS_GPU_SURFACE_DEPTH_FORMAT_D24,
							sampleCount, KS_GPU_RENDERPASS_TYPE_INLINE,
							KS_GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER |
							KS_GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER );

	ksGpuFramebuffer framebuffer;
	ksGpuFramebuffer_CreateFromTextureArrays( &context, &framebuffer, &renderPass,
				resolution, resolution, NUM_EYES, NUM_EYE_BUFFERS, threadData->sceneSettings->useMultiView );

	const int numPasses = threadData->sceneSettings->useMultiView ? 1 : NUM_EYES;

	ksGpuCommandBuffer eyeCommandBuffer[NUM_EYES];
	ksGpuTimer eyeTimer[NUM_EYES];

	for ( int eye = 0; eye < numPasses; eye++ )
	{
		ksGpuCommandBuffer_Create( &context, &eyeCommandBuffer[eye], KS_GPU_COMMAND_BUFFER_TYPE_PRIMARY, NUM_EYE_BUFFERS );
		ksGpuTimer_Create( &context, &eyeTimer[eye] );
	}

	const ksBodyInfo * bodyInfo = GetDefaultBodyInfo();

	ksViewState viewState;
	ksViewState_Init( &viewState, bodyInfo->interpupillaryDistance );

	ksPerfScene perfScene;
	ksGltfScene gltfScene;

	if ( threadData->sceneSettings->glTF == NULL )
	{
		ksPerfScene_Create( &context, &perfScene, threadData->sceneSettings, &renderPass );
	}
	else
	{
		ksGltfScene_CreateFromFile( &context, &gltfScene, threadData->sceneSettings, &renderPass );
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

		ksGpuTexture * eyeTexture[NUM_EYES] = { 0 };
		ksGpuFence * eyeCompletionFence[NUM_EYES] = { 0 };
		int eyeArrayLayer[NUM_EYES] = { 0, 1 };

		for ( int eye = 0; eye < numPasses; eye++ )
		{
			const ksScreenRect screenRect = ksGpuFramebuffer_GetRect( &framebuffer );

			ksGpuCommandBuffer_BeginPrimary( &eyeCommandBuffer[eye] );
			ksGpuCommandBuffer_BeginFramebuffer( &eyeCommandBuffer[eye], &framebuffer, eye, KS_GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );

			if ( threadData->sceneSettings->glTF == NULL )
			{
				ksPerfScene_UpdateBuffers( &eyeCommandBuffer[eye], &perfScene, &viewState, ( numPasses == 1 ) ? 2 : eye );
			}
			else
			{
				ksGltfScene_UpdateBuffers( &eyeCommandBuffer[eye], &gltfScene, &viewState, ( numPasses == 1 ) ? 2 : eye );
			}

			ksGpuCommandBuffer_BeginTimer( &eyeCommandBuffer[eye], &eyeTimer[eye] );
			ksGpuCommandBuffer_BeginRenderPass( &eyeCommandBuffer[eye], &renderPass, &framebuffer, &screenRect );

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

			ksGpuCommandBuffer_EndRenderPass( &eyeCommandBuffer[eye], &renderPass );
			ksGpuCommandBuffer_EndTimer( &eyeCommandBuffer[eye], &eyeTimer[eye] );

			ksGpuCommandBuffer_EndFramebuffer( &eyeCommandBuffer[eye], &framebuffer, eye, KS_GPU_TEXTURE_USAGE_SAMPLED );
			ksGpuCommandBuffer_EndPrimary( &eyeCommandBuffer[eye] );

			eyeTexture[eye] = ksGpuFramebuffer_GetColorTexture( &framebuffer );
			eyeCompletionFence[eye] = ksGpuCommandBuffer_SubmitPrimary( &eyeCommandBuffer[eye] );
		}

		if ( threadData->sceneSettings->useMultiView )
		{
			eyeTexture[1] = eyeTexture[0];
			eyeCompletionFence[1] = eyeCompletionFence[0];
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

	for ( int eye = 0; eye < numPasses; eye++ )
	{
		ksGpuTimer_Destroy( &context, &eyeTimer[eye] );
		ksGpuCommandBuffer_Destroy( &context, &eyeCommandBuffer[eye] );
	}

	ksGpuFramebuffer_Destroy( &context, &framebuffer );
	ksGpuRenderPass_Destroy( &context, &renderPass );
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

	// On MacOS context creation fails if the share context is current on another thread.
	ksGpuContext_UnsetCurrent( &window->context );

	ksThread_Create( sceneThread, "atw:scene", (ksThreadFunction) SceneThread_Render, sceneThreadData );
	ksThread_Signal( sceneThread );
	ksSignal_Wait( &sceneThreadData->initialized, SIGNAL_TIMEOUT_INFINITE );

	ksGpuContext_SetCurrent( &window->context );
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
			if ( glExtensions.multi_view )
			{
				ksSceneSettings_ToggleMultiView( &sceneSettings );
				break;
			}
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
	ksSceneSettings_SetGltf( &sceneSettings, startupSettings->glTF );
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

// adb shell am start -n com.vulkansamples.atw_opengl/android.app.NativeActivity -a "android.intent.action.MAIN" --es "args" "\"-r tw\""
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

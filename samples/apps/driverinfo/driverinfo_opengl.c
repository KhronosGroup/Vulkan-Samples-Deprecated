/*
================================================================================================

Description	:	OpenGL driver information.
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

This sample displays OpenGL driver information.


IMPLEMENTATION
==============

The code is written against version 4.3 of the Core Profile OpenGL Specification,
and version 3.1 of the OpenGL ES Specification.

Supported platforms are:

	- Microsoft Windows 7 or later
	- Apple Mac OS X 10.9 or later
	- Ubuntu Linux 14.04 or later
	- Android 5.0 or later


COMMAND-LINE COMPILATION
========================

Microsoft Windows: Visual Studio 2013 Compiler:
	"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
	cl /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /arch:SSE2 /Iinclude driverinfo_opengl.c /link user32.lib gdi32.lib Advapi32.lib opengl32.lib

Microsoft Windows: Intel Compiler 14.0
	"C:\Program Files (x86)\Intel\Composer XE\bin\iclvars.bat" intel64
	icl /Qstd=c99 /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /arch:SSE2 /Iinclude driverinfo_opengl.c /link user32.lib gdi32.lib Advapi32.lib opengl32.lib

Apple Mac OS X: Apple LLVM 6.0:
	clang -std=c99 -x objective-c -fno-objc-arc -Wall -g -O2 -o driverinfo_opengl driverinfo_opengl.c -framework Cocoa -framework OpenGL

Linux: GCC 4.8.2 Xlib:
	sudo apt-get install libx11-dev
	sudo apt-get install libxxf86vm-dev
	sudo apt-get install libxrandr-dev
	sudo apt-get install mesa-common-dev
	sudo apt-get install libgl1-mesa-dev
	gcc -std=c99 -Wall -g -O2 -m64 -o driverinfo_opengl driverinfo_opengl.c -lm -lX11 -lXxf86vm -lXrandr -lGL

Linux: GCC 4.8.2 XCB:
	sudo apt-get install libxcb1-dev
	sudo apt-get install libxcb-keysyms1-dev
	sudo apt-get install libxcb-icccm4-dev
	sudo apt-get install mesa-common-dev
	sudo apt-get install libgl1-mesa-dev
	gcc -std=c99 -Wall -g -O2 -m64 -o driverinfo_opengl driverinfo_opengl.c -lm -lxcb -lxcb-keysyms -lxcb-randr -lxcb-glx -lxcb-dri2 -lGL

Android for ARM from Windows: NDK Revision 11c - Android 21 - ANT/Gradle
	ANT:
		cd projects/android/ant/driverinfo_opengl
		ndk-build
		ant debug
		adb install -r bin/driverinfo_opengl-debug.apk
	Gradle:
		cd projects/android/gradle/driverinfo_opengl
		gradlew build
		adb install -r build/outputs/apk/driverinfo_opengl-all-debug.apk


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
	#define OS_LINUX_XLIB		// Xlib + Xlib GLX 1.3
	//#define OS_LINUX_XCB		// XCB + XCB GLX is limited to OpenGL 2.1
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

	#define OPENGL_VERSION_MAJOR	4
	#define OPENGL_VERSION_MINOR	3

	#include <conio.h>			// for _getch()
	#include <windows.h>
	#include <GL/gl.h>
	#define GL_EXT_color_subtable
	#include <GL/glext.h>
	#include <GL/wglext.h>

#elif defined( OS_MAC )

	// Apple is still at OpenGL 4.1
	#define OPENGL_VERSION_MAJOR	4
	#define OPENGL_VERSION_MINOR	1

	#include <sys/param.h>
	#include <sys/sysctl.h>
	#include <sys/time.h>
	#include <pthread.h>
	#include <Cocoa/Cocoa.h>
	#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
	#include <OpenGL/OpenGL.h>
	#include <OpenGL/gl3.h>
	#include <OpenGL/gl3ext.h>

	#pragma clang diagnostic ignored "-Wunused-function"
	#pragma clang diagnostic ignored "-Wunused-const-variable"

#elif defined( OS_LINUX )

	#define OPENGL_VERSION_MAJOR	4
	#define OPENGL_VERSION_MINOR	3

	#include <sys/time.h>
	#define __USE_UNIX98						// for pthread_mutexattr_settype
	#include <pthread.h>
	#include <malloc.h>							// for memalign
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
		#include <xcb/glx.h>
		#include <xcb/dri2.h>
	#endif
	#include <GL/glx.h>

	#pragma GCC diagnostic ignored "-Wunused-function"

#elif defined( OS_ANDROID )

	#define OPENGL_VERSION_MAJOR	3
	#define OPENGL_VERSION_MINOR	1

	#include <time.h>
	#include <malloc.h>							// for memalign
	#include <dlfcn.h>							// for dlopen
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

/*
================================
Common defines
================================
*/

#define UNUSED_PARM( x )				{ (void)(x); }
#define ARRAY_SIZE( a )					( sizeof( (a) ) / sizeof( (a)[0] ) )
#define BIT( x )						( 1 << (x) )
#define ROUNDUP( x, granularity )		( ( (x) + (granularity) - 1 ) & ~( (granularity) - 1 ) )
#define CLAMP( x, min, max )			( ( (x) < (min) ) ? (min) : ( ( (x) > (max) ) ? (max) : (x) ) )
#define STRINGIFY_EXPANDED( a )			#a
#define STRINGIFY( a )					STRINGIFY_EXPANDED(a)
#define ENUM_STRING_CASE( e )			case (e): return #e

#define APPLICATION_NAME				"DriverInfo"
#define WINDOW_TITLE					"DriverInfo"

#if !defined( GL_SR8_EXT )
	#define GL_SR8_EXT					0x8FBD
#endif
#if !defined( GL_SRG8_EXT )
	#define GL_SRG8_EXT					0x8FBE
#endif
#if !defined( EGL_OPENGL_ES3_BIT )
	#define EGL_OPENGL_ES3_BIT			0x0040
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

	OutputDebugString( buffer );
	printf( buffer );
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

/*
================================================================================================================================

Console

================================================================================================================================
*/

static void Console_Resize( const short numLines, const short numColumns )
{
	UNUSED_PARM( numLines );
	UNUSED_PARM( numColumns );

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
#endif
}

/*
================================================================================================================================

OpenGL error checking.

================================================================================================================================
*/

#if defined( _DEBUG )
	#define GL( func )		func; GlCheckErrors( #func );
#else
	#define GL( func )		func;
#endif

#if defined( _DEBUG )
	#define EGL( func )		if ( func == EGL_FALSE ) { Error( #func " failed: %s", EglErrorString( eglGetError() ) ); }
#else
	#define EGL( func )		if ( func == EGL_FALSE ) { Error( #func " failed: %s", EglErrorString( eglGetError() ) ); }
#endif

#if defined( OS_ANDROID )
static const char * EglErrorString( const EGLint error )
{
	switch ( error )
	{
		ENUM_STRING_CASE( EGL_SUCCESS );
		ENUM_STRING_CASE( EGL_NOT_INITIALIZED );
		ENUM_STRING_CASE( EGL_BAD_ACCESS );
		ENUM_STRING_CASE( EGL_BAD_ALLOC );
		ENUM_STRING_CASE( EGL_BAD_ATTRIBUTE );
		ENUM_STRING_CASE( EGL_BAD_CONTEXT );
		ENUM_STRING_CASE( EGL_BAD_CONFIG );
		ENUM_STRING_CASE( EGL_BAD_CURRENT_SURFACE );
		ENUM_STRING_CASE( EGL_BAD_DISPLAY );
		ENUM_STRING_CASE( EGL_BAD_SURFACE );
		ENUM_STRING_CASE( EGL_BAD_MATCH );
		ENUM_STRING_CASE( EGL_BAD_PARAMETER );
		ENUM_STRING_CASE( EGL_BAD_NATIVE_PIXMAP );
		ENUM_STRING_CASE( EGL_BAD_NATIVE_WINDOW );
		ENUM_STRING_CASE( EGL_CONTEXT_LOST );
		default: return "unknown";
	}
}
#endif

static const char * GlErrorString( GLenum error )
{
	switch ( error )
	{
		ENUM_STRING_CASE( GL_NO_ERROR );
		ENUM_STRING_CASE( GL_INVALID_ENUM );
		ENUM_STRING_CASE( GL_INVALID_VALUE );
		ENUM_STRING_CASE( GL_INVALID_OPERATION );
		ENUM_STRING_CASE( GL_INVALID_FRAMEBUFFER_OPERATION );
		ENUM_STRING_CASE( GL_OUT_OF_MEMORY );
#if !defined( OS_MAC ) && !defined( OS_ANDROID )
		ENUM_STRING_CASE( GL_STACK_UNDERFLOW );
		ENUM_STRING_CASE( GL_STACK_OVERFLOW );
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

#if defined( OS_WINDOWS )
PROC GetExtension( const char * functionName )
{
	return wglGetProcAddress( functionName );
}
#elif defined( OS_LINUX )
void ( *GetExtension( const char * functionName ) )()
{
	return glXGetProcAddress( (const GLubyte *)functionName );
}
#endif

/*
================================================================================================================================

GPU context.

================================================================================================================================
*/

typedef struct
{
#if defined( OS_WINDOWS )
	HINSTANCE				hInstance;
	HWND					hWnd;
	HDC						hDC;
	HGLRC					hGLRC;
#elif defined( OS_MAC )
	CGDirectDisplayID		display;
	NSOpenGLContext *		nsContext;
	CGLContextObj			cglContext;
#elif defined( OS_LINUX_XLIB )
	Display *				display;
	int						screen;
	uint32_t				visualid;
	GLXFBConfig				glxFBConfig;
	GLXDrawable				glxDrawable;
	GLXContext				glxContext;
#elif defined( OS_LINUX_XCB )
	Display *				display;
	uint32_t				screen;
	xcb_connection_t *		connection;
	xcb_glx_fbconfig_t		fbconfigid;
	xcb_visualid_t			visualid;
	xcb_glx_drawable_t		glxDrawable;
	xcb_glx_context_t		glxContext;
	xcb_glx_context_tag_t	glxContextTag;
#elif defined( OS_ANDROID )
	EGLDisplay				display;
	EGLConfig				config;
	EGLSurface				tinySurface;
	EGLSurface				mainSurface;
	EGLContext				context;
#endif
} GpuContext_t;

static void GpuContext_Destroy( GpuContext_t * context )
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
	if ( context->hDC )
	{
		if ( !ReleaseDC( context->hWnd, context->hDC ) )
		{
			Error( "Failed to release device context." );
		}
		context->hDC = NULL;
	}

	if ( context->hWnd )
	{
		if ( !DestroyWindow( context->hWnd ) )
		{
			Error( "Failed to destroy the context." );
		}
		context->hWnd = NULL;
	}

	if ( context->hInstance )
	{
		if ( !UnregisterClass( APPLICATION_NAME, context->hInstance ) )
		{
			Error( "Failed to unregister context class." );
		}
		context->hInstance = NULL;
	}
	context->hDC = NULL;
#elif defined( OS_MAC )
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
#elif defined( OS_LINUX_XLIB )
	glXDestroyContext( context->display, context->glxContext );
	XCloseDisplay( context->display );
	context->display = NULL;
	context->visualid = 0;
	context->glxFBConfig = NULL;
	context->glxDrawable = 0;
	context->glxContext = NULL;
#elif defined( OS_LINUX_XCB )
	XCloseDisplay( context->display );
	xcb_glx_destroy_context( context->connection, context->glxContext );
	context->connection = NULL;
	context->screen = 0;
	context->fbconfigid = 0;
	context->visualid = 0;
	context->glxDrawable = 0;
	context->glxContext = 0;
	context->glxContextTag = 0;
#elif defined( OS_ANDROID )
	if ( context->display != 0 )
	{
		EGL( eglMakeCurrent( context->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) );
	}
	if ( context->context != EGL_NO_CONTEXT )
	{
		EGL( eglDestroyContext( context->display, context->context ) );
		context->context = EGL_NO_CONTEXT;
	}
	if ( context->mainSurface != context->tinySurface )
	{
		EGL( eglDestroySurface( context->display, context->mainSurface ) );
		context->mainSurface = EGL_NO_SURFACE;
	}
	if ( context->tinySurface != EGL_NO_SURFACE )
	{
		EGL( eglDestroySurface( context->display, context->tinySurface ) );
		context->tinySurface = EGL_NO_SURFACE;
	}
	if ( context->display != 0 )
	{
		EGL( eglTerminate( context->display ) );
		context->display = 0;
	}
	context->config = EGL_NO_CONTEXT;
#endif
}

#if defined( OS_WINDOWS )

LRESULT APIENTRY WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	GpuContext_t * context = (GpuContext_t *) GetWindowLongPtr( hWnd, GWLP_USERDATA );
	UNUSED_PARM( context );

	switch ( message )
	{
		case WM_SIZE:
		{
			return 0;
		}
		case WM_ACTIVATE:
		{
			return 0;
		}
		case WM_CLOSE:
		{
			PostQuitMessage( 0 );
			return 0;
		}
	}
	return DefWindowProc( hWnd, message, wParam, lParam );
}

static bool GpuContext_Create( GpuContext_t * context )
{
	memset( context, 0, sizeof( GpuContext_t ) );

	context->hInstance = GetModuleHandle( NULL );

	WNDCLASS wc;
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc		= (WNDPROC) WndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= context->hInstance;
	wc.hIcon			= LoadIcon( NULL, IDI_WINLOGO );
	wc.hCursor			= LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground	= NULL;
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= APPLICATION_NAME;

	if ( !RegisterClass( &wc ) )
	{
		Error( "Failed to register context class." );
		return false;
	}

	// Fixed size context.
	const DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	const DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	context->hWnd = CreateWindowEx( dwExStyle,		// Extended style for the context
								APPLICATION_NAME,	// Class name
								WINDOW_TITLE,		// Window title
								dwStyle |			// Defined context style
								WS_CLIPSIBLINGS |	// Required context style
								WS_CLIPCHILDREN,	// Required context style
								CW_USEDEFAULT,		// Window X position
								CW_USEDEFAULT,		// Window Y position
								0,					// Window width
								0,					// Window height
								NULL,				// No parent context
								NULL,				// No menu
								context->hInstance,	// Instance
								NULL );
	if ( !context->hWnd )
	{
		GpuContext_Destroy( context );
		Error( "Failed to create context." );
		return false;
	}

	SetWindowLongPtr( context->hWnd, GWLP_USERDATA, (LONG_PTR) context );

	context->hDC = GetDC( context->hWnd );
	if ( !context->hDC )
	{
		GpuContext_Destroy( context );
		Error( "Failed to acquire device context." );
		return false;
	}

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof( PIXELFORMATDESCRIPTOR ),
		1,						// version
		PFD_DRAW_TO_WINDOW |	// must support windowed
		PFD_SUPPORT_OPENGL |	// must support OpenGL
		PFD_DOUBLEBUFFER,		// must support double buffering
		PFD_TYPE_RGBA,			// iPixelType
		32,						// cColorBits
		0, 0,					// cRedBits, cRedShift
		0, 0,					// cGreenBits, cGreenShift
		0, 0,					// cBlueBits, cBlueShift
		0, 0,					// cAlphaBits, cAlphaShift
		0,						// cAccumBits
		0,						// cAccumRedBits
		0,						// cAccumGreenBits
		0,						// cAccumBlueBits
		0,						// cAccumAlphaBits
		0,						// cDepthBits
		0,						// cStencilBits
		0,						// cAuxBuffers
		PFD_MAIN_PLANE,			// iLayerType
		0,						// bReserved
		0,						// dwLayerMask
		0,						// dwVisibleMask
		0						// dwDamageMask
	};

	int pixelFormat = ChoosePixelFormat( context->hDC, &pfd );
	if ( pixelFormat == 0 )
	{
		Error( "Failed to find a suitable PixelFormat." );
		return false;
	}

	if ( !SetPixelFormat( context->hDC, pixelFormat, &pfd ) )
	{
		Error( "Failed to set the PixelFormat." );
		return false;
	}

	// Now that the pixel format is set, create a temporary context to get the extensions.
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
	{
		HGLRC hGLRC = wglCreateContext( context->hDC );
		wglMakeCurrent( context->hDC, hGLRC );

		wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) GetExtension( "wglCreateContextAttribsARB" );

		wglMakeCurrent( NULL, NULL );
		wglDeleteContext( hGLRC );
	}

	int attribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB,	OPENGL_VERSION_MAJOR,
		WGL_CONTEXT_MINOR_VERSION_ARB,	OPENGL_VERSION_MINOR,
		WGL_CONTEXT_PROFILE_MASK_ARB,	WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		WGL_CONTEXT_FLAGS_ARB,			WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	context->hGLRC = wglCreateContextAttribsARB( context->hDC, NULL, attribs );
	if ( !context->hGLRC )
	{
		Error( "Failed to create GL context." );
		return false;
	}

	return true;
}

#elif defined( OS_MAC )

static bool GpuContext_Create( GpuContext_t * context )
{
	memset( context, 0, sizeof( GpuContext_t ) );

	CGDirectDisplayID displays[32];
	CGDisplayCount displayCount = 0;
	CGDisplayErr err = CGGetActiveDisplayList( 32, displays, &displayCount );
	if ( err != CGDisplayNoErr )
	{
		return false;
	}

	// Use the main display.
	context->display = displays[0];

	NSOpenGLPixelFormatAttribute pixelFormatAttributes[] =
	{
		NSOpenGLPFAMinimumPolicy,	1,
		NSOpenGLPFAScreenMask,		CGDisplayIDToOpenGLDisplayMask( context->display ),
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAOpenGLProfile,	NSOpenGLProfileVersion3_2Core,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize,		32,
		NSOpenGLPFADepthSize,		0,
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

	return true;
}

#elif defined( OS_LINUX_XLIB )

static int glxGetFBConfigAttrib2( Display * dpy, GLXFBConfig config, int attribute )
{
	int value;
	glXGetFBConfigAttrib( dpy, config, attribute, &value );
	return value;
}

static bool GpuContext_Create( GpuContext_t * context )
{
	memset( context, 0, sizeof( GpuContext_t ) );

	const char * displayName = NULL;
	context->display = XOpenDisplay( displayName );
	if ( !context->display )
	{
		Error( "Unable to open X Display." );
		return false;
	}
	context->screen = XDefaultScreen( context->display );

	int glxErrorBase;
	int glxEventBase;
	if ( !glXQueryExtension( context->display, &glxErrorBase, &glxEventBase ) )
	{
		Error( "X display does not support the GLX extension." );
		return false;
	}

	int glxVersionMajor;
	int glxVersionMinor;
	if ( !glXQueryVersion( context->display, &glxVersionMajor, &glxVersionMinor ) )
	{
		Error( "Unable to retrieve GLX version." );
		return false;
	}

	int fbConfigCount = 0;
	GLXFBConfig * fbConfigs = glXGetFBConfigs( context->display, context->screen, &fbConfigCount );
	if ( fbConfigCount == 0 )
	{
		Error( "No valid framebuffer configurations found." );
		return false;
	}

	bool foundFbConfig = false;
	for ( int i = 0; i < fbConfigCount; i++ )
	{
		if ( glxGetFBConfigAttrib2( context->display, fbConfigs[i], GLX_FBCONFIG_ID ) == 0 ) { continue; }
		if ( glxGetFBConfigAttrib2( context->display, fbConfigs[i], GLX_VISUAL_ID ) == 0 ) { continue; }
		if ( glxGetFBConfigAttrib2( context->display, fbConfigs[i], GLX_DOUBLEBUFFER ) == 0 ) { continue; }
		if ( ( glxGetFBConfigAttrib2( context->display, fbConfigs[i], GLX_RENDER_TYPE ) & GLX_RGBA_BIT ) == 0 ) { continue; }
		if ( ( glxGetFBConfigAttrib2( context->display, fbConfigs[i], GLX_DRAWABLE_TYPE ) & GLX_WINDOW_BIT ) == 0 ) { continue; }
		if ( glxGetFBConfigAttrib2( context->display, fbConfigs[i], GLX_RED_SIZE )   != 8 ) { continue; }
		if ( glxGetFBConfigAttrib2( context->display, fbConfigs[i], GLX_GREEN_SIZE ) != 8 ) { continue; }
		if ( glxGetFBConfigAttrib2( context->display, fbConfigs[i], GLX_BLUE_SIZE )  != 8 ) { continue; }
		if ( glxGetFBConfigAttrib2( context->display, fbConfigs[i], GLX_ALPHA_SIZE ) != 8 ) { continue; }
		if ( glxGetFBConfigAttrib2( context->display, fbConfigs[i], GLX_DEPTH_SIZE ) != 0 ) { continue; }

		context->visualid = glxGetFBConfigAttrib2( context->display, fbConfigs[i], GLX_VISUAL_ID );
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

	PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC) GetExtension( "glXCreateContextAttribsARB" );

	int attribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB,	OPENGL_VERSION_MAJOR,
		GLX_CONTEXT_MINOR_VERSION_ARB,	OPENGL_VERSION_MINOR,
		GLX_CONTEXT_PROFILE_MASK_ARB,	GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		GLX_CONTEXT_FLAGS_ARB,			GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	context->glxContext = glXCreateContextAttribsARB( context->display,			// Display *	dpy
														context->glxFBConfig,	// GLXFBConfig	config
														NULL,					// GLXContext	share_context
														True,					// Bool			direct
														attribs );				// const int *	attrib_list

	if ( context->glxContext == NULL )
	{
		Error( "Unable to create GLX context." );
		return false;
	}

	if ( !glXIsDirect( context->display, context->glxContext ) )
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

static bool GpuContext_Create( GpuContext_t * context )
{
	memset( context, 0, sizeof( GpuContext_t ) );

	const char * displayName = NULL;
	context->display = XOpenDisplay( displayName );
	if ( !context->display )
	{
		Error( "Unable to open X Display." );
		return false;
	}
	context->screen = 0;
	context->connection = xcb_connect( displayName, &context->screen );
	if ( xcb_connection_has_error( context->connection ) )
	{
		Error( "Failed to open XCB connection." );
		return false;
	}

	xcb_glx_query_version_cookie_t glx_query_version_cookie = xcb_glx_query_version( context->connection, OPENGL_VERSION_MAJOR, OPENGL_VERSION_MINOR );
	xcb_glx_query_version_reply_t * glx_query_version_reply = xcb_glx_query_version_reply( context->connection, glx_query_version_cookie, NULL );
	if ( glx_query_version_reply == NULL )
	{
		Error( "Unable to retrieve GLX version." );
		return false;
	}
	free( glx_query_version_reply );

	xcb_glx_get_fb_configs_cookie_t get_fb_configs_cookie = xcb_glx_get_fb_configs( context->connection, context->screen );
	xcb_glx_get_fb_configs_reply_t * get_fb_configs_reply = xcb_glx_get_fb_configs_reply( context->connection, get_fb_configs_cookie, NULL );

	if ( get_fb_configs_reply == NULL || get_fb_configs_reply->num_FB_configs == 0 )
	{
		Error( "No valid framebuffer configurations found." );
		return false;
	}

	const GpuSurfaceBits_t bits = GpuContext_BitsForSurfaceFormat( colorFormat, depthFormat );

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

	// Create the context.
	uint32_t attribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB,	OPENGL_VERSION_MAJOR,
		GLX_CONTEXT_MINOR_VERSION_ARB,	OPENGL_VERSION_MINOR,
		GLX_CONTEXT_PROFILE_MASK_ARB,	GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		GLX_CONTEXT_FLAGS_ARB,			GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	context->glxContext = xcb_generate_id( context->connection );
	xcb_glx_create_context_attribs_arb( context->connection,	// xcb_connection_t *	connection
										context->glxContext,	// xcb_glx_context_t	context
										context->fbconfigid,	// xcb_glx_fbconfig_t	fbconfig
										context->screen,		// uint32_t				screen
										0,						// xcb_glx_context_t	share_list
										1,						// uint8_t				is_direct
										4,						// uint32_t				num_attribs
										attribs );				// const uint32_t *		attribs

	// Make sure the context is direct.
	xcb_generic_error_t * error;
	xcb_glx_is_direct_cookie_t glx_is_direct_cookie = xcb_glx_is_direct_unchecked( context->connection, context->glxContext );
	xcb_glx_is_direct_reply_t * glx_is_direct_reply = xcb_glx_is_direct_reply( context->connection, glx_is_direct_cookie, &error );
	const bool is_direct = ( glx_is_direct_reply != NULL && glx_is_direct_reply->is_direct );
	free( glx_is_direct_reply );

	if ( !is_direct )
	{
		Error( "Unable to create direct rendering context." );
		return false;
	}

	return true;
}

#elif defined( OS_ANDROID )

static bool GpuContext_Create( GpuContext_t * context )
{
	memset( context, 0, sizeof( GpuContext_t ) );

	EGLint majorVersion = OPENGL_VERSION_MAJOR;
	EGLint minorVersion = OPENGL_VERSION_MINOR;

	context->display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
	EGL( eglInitialize( context->display, &majorVersion, &minorVersion ) );

	const int MAX_CONFIGS = 1024;
	EGLConfig configs[MAX_CONFIGS];
	EGLint numConfigs = 0;
	EGL( eglGetConfigs( context->display, configs, MAX_CONFIGS, &numConfigs ) );

	const EGLint configAttribs[] =
	{
		EGL_RED_SIZE,		8,
		EGL_GREEN_SIZE,		8,
		EGL_BLUE_SIZE,		8,
		EGL_ALPHA_SIZE,		8,
		EGL_DEPTH_SIZE,		0,
		//EGL_STENCIL_SIZE,	0,
		EGL_SAMPLES,		0,
		EGL_NONE
	};

	context->config = 0;
	for ( int i = 0; i < numConfigs; i++ )
	{
		EGLint value = 0;

		eglGetConfigAttrib( context->display, configs[i], EGL_RENDERABLE_TYPE, &value );
		if ( ( value & EGL_OPENGL_ES3_BIT ) != EGL_OPENGL_ES3_BIT )
		{
			continue;
		}

		// Without EGL_KHR_surfaceless_context, the config needs to support both pbuffers and window surfaces.
		eglGetConfigAttrib( context->display, configs[i], EGL_SURFACE_TYPE, &value );
		if ( ( value & ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) ) != ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) )
		{
			continue;
		}

		int	j = 0;
		for ( ; configAttribs[j] != EGL_NONE; j += 2 )
		{
			eglGetConfigAttrib( context->display, configs[i], configAttribs[j], &value );
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
		return false;
	}

	EGLint contextAttribs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, OPENGL_VERSION_MAJOR,
		EGL_NONE, EGL_NONE,
		EGL_NONE
	};
	context->context = eglCreateContext( context->display, context->config, EGL_NO_CONTEXT, contextAttribs );
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

	return true;
}

#endif

static void GpuContext_SetCurrent( GpuContext_t * context )
{
#if defined( OS_WINDOWS )
	wglMakeCurrent( context->hDC, context->hGLRC );
#elif defined( OS_MAC )
	CGLSetCurrentContext( context->cglContext );
#elif defined( OS_LINUX_XLIB )
	glXMakeCurrent( context->display, context->glxDrawable, context->glxContext );
#elif defined( OS_LINUX_XCB )
	xcb_glx_make_current_cookie_t glx_make_current_cookie = xcb_glx_make_current( context->connection, context->glxDrawable, context->glxContext, 0 );
	xcb_glx_make_current_reply_t * glx_make_current_reply = xcb_glx_make_current_reply( context->connection, glx_make_current_cookie, NULL );
	context->glxContextTag = glx_make_current_reply->context_tag;
	free( glx_make_current_reply );
#elif defined( OS_ANDROID )
	EGL( eglMakeCurrent( context->display, context->mainSurface, context->mainSurface, context->context ) );
#endif
}

static void GpuContext_UnsetCurrent( GpuContext_t * context )
{
#if defined( OS_WINDOWS )
	wglMakeCurrent( context->hDC, NULL );
#elif defined( OS_MAC )
	CGLSetCurrentContext( NULL );
#elif defined( OS_LINUX_XLIB )
	glXMakeCurrent( context->display, None, NULL );
#elif defined( OS_LINUX_XCB )
	xcb_glx_make_current( context->connection, 0, 0, 0 );
#elif defined( OS_ANDROID )
	EGL( eglMakeCurrent( context->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) );
#endif
}

static bool GpuContext_CheckCurrent( GpuContext_t * context )
{
#if defined( OS_WINDOWS )
	return ( wglGetCurrentContext() == context->hGLRC );
#elif defined( OS_MAC )
	return ( CGLGetCurrentContext() == context->cglContext );
#elif defined( OS_LINUX_XLIB )
	return ( glXGetCurrentContext() == context->glxContext );
#elif defined( OS_LINUX_XCB )
	return true;
#elif defined( OS_ANDROID )
	return ( eglGetCurrentContext() == context->context );
#endif
}

/*
================================================================================================================================

Print Driver Info

================================================================================================================================
*/

#define FORMAT_ENUM_STRING( e, c, d )		{ e, #e, c, d }

static struct
{
	GLenum			value;
	const char *	string;
	bool			compressed;
	const char *	description;
}
formats[] =
{
	//
	// 8 bits per component
	//
	FORMAT_ENUM_STRING( GL_R8,												false, "1-component, 8-bit unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_RG8,												false, "2-component, 8-bit unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_RGBA8,											false, "4-component, 8-bit unsigned normalized" ),

	FORMAT_ENUM_STRING( GL_R8_SNORM,										false, "1-component, 8-bit signed normalized" ),
	FORMAT_ENUM_STRING( GL_RG8_SNORM,										false, "2-component, 8-bit signed normalized" ),
	FORMAT_ENUM_STRING( GL_RGBA8_SNORM,										false, "4-component, 8-bit signed normalized" ),

	FORMAT_ENUM_STRING( GL_R8UI,											false, "1-component, 8-bit unsigned integer" ),
	FORMAT_ENUM_STRING( GL_RG8UI,											false, "2-component, 8-bit unsigned integer" ),
	FORMAT_ENUM_STRING( GL_RGBA8UI,											false, "4-component, 8-bit unsigned integer" ),

	FORMAT_ENUM_STRING( GL_R8I,												false, "1-component, 8-bit signed integer" ),
	FORMAT_ENUM_STRING( GL_RG8I,											false, "2-component, 8-bit signed integer" ),
	FORMAT_ENUM_STRING( GL_RGBA8I,											false, "4-component, 8-bit signed integer" ),

	FORMAT_ENUM_STRING( GL_SR8_EXT,											false, "1-component, 8-bit sRGB" ),
	FORMAT_ENUM_STRING( GL_SRG8_EXT,										false, "2-component, 8-bit sRGB" ),
	FORMAT_ENUM_STRING( GL_SRGB8_ALPHA8,									false, "4-component, 8-bit sRGB" ),

	//
	// 16 bits per component
	//
#if defined( GL_R16 )
	FORMAT_ENUM_STRING( GL_R16,												false, "1-component, 16-bit unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_RG16,											false, "2-component, 16-bit unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_RGBA16,											false, "4-component, 16-bit unsigned normalized" ),
#elif defined( GL_R16_EXT )
	FORMAT_ENUM_STRING( GL_R16_EXT,											false, "1-component, 16-bit unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_RG16_EXT,										false, "2-component, 16-bit unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_RGB16_EXT,										false, "4-component, 16-bit unsigned normalized" ),
#endif

#if defined( GL_R16_SNORM )
	FORMAT_ENUM_STRING( GL_R16_SNORM,										false, "1-component, 16-bit signed normalized" ),
	FORMAT_ENUM_STRING( GL_RG16_SNORM,										false, "2-component, 16-bit signed normalized" ),
	FORMAT_ENUM_STRING( GL_RGBA16_SNORM,									false, "4-component, 16-bit signed normalized" ),
#elif defined( GL_R16_SNORM_EXT )
	FORMAT_ENUM_STRING( GL_R16_SNORM_EXT,									false, "1-component, 16-bit signed normalized" ),
	FORMAT_ENUM_STRING( GL_RG16_SNORM_EXT,									false, "2-component, 16-bit signed normalized" ),
	FORMAT_ENUM_STRING( GL_RGB16_SNORM_EXT,									false, "4-component, 16-bit signed normalized" ),
#endif

	FORMAT_ENUM_STRING( GL_R16UI,											false, "1-component, 16-bit unsigned integer" ),
	FORMAT_ENUM_STRING( GL_RG16UI,											false, "2-component, 16-bit unsigned integer" ),
	FORMAT_ENUM_STRING( GL_RGBA16UI,										false, "4-component, 16-bit unsigned integer" ),

	FORMAT_ENUM_STRING( GL_R16I,											false, "1-component, 16-bit signed integer" ),
	FORMAT_ENUM_STRING( GL_RG16I,											false, "2-component, 16-bit signed integer" ),
	FORMAT_ENUM_STRING( GL_RGBA16I,											false, "4-component, 16-bit signed integer" ),

	FORMAT_ENUM_STRING( GL_R16F,											false, "1-component, 16-bit floating-point" ),
	FORMAT_ENUM_STRING( GL_RG16F,											false, "2-component, 16-bit floating-point" ),
	FORMAT_ENUM_STRING( GL_RGBA16F,											false, "4-component, 16-bit floating-point" ),

	//
	// 32 bits per component
	//
	FORMAT_ENUM_STRING( GL_R32UI,											false, "1-component, 32-bit unsigned integer" ),
	FORMAT_ENUM_STRING( GL_RG32UI,											false, "2-component, 32-bit unsigned integer" ),
	FORMAT_ENUM_STRING( GL_RGBA32UI,										false, "4-component, 32-bit unsigned integer" ),

	FORMAT_ENUM_STRING( GL_R32I,											false, "1-component, 32-bit signed integer" ),
	FORMAT_ENUM_STRING( GL_RG32I,											false, "2-component, 32-bit signed integer" ),
	FORMAT_ENUM_STRING( GL_RGBA32I,											false, "4-component, 32-bit signed integer" ),

	FORMAT_ENUM_STRING( GL_R32F,											false, "1-component, 32-bit floating-point" ),
	FORMAT_ENUM_STRING( GL_RG32F,											false, "2-component, 32-bit floating-point" ),
	FORMAT_ENUM_STRING( GL_RGBA32F,											false, "4-component, 32-bit floating-point" ),

	//
	// Odd bits per component
	//
#if defined( GL_R3_G3_B2 )
	FORMAT_ENUM_STRING( GL_R3_G3_B2,										false, "3-component 3:3:2,       unsigned normalized" ),
#endif
#if defined( GL_RGB4 )
	FORMAT_ENUM_STRING( GL_RGB4,											false, "3-component 4:4:4,       unsigned normalized" ),
#endif
#if defined( GL_RGB5 )
	FORMAT_ENUM_STRING( GL_RGB5,											false, "3-component 5:5:5,       unsigned normalized" ),
#endif
#if defined( GL_RGB565 )
	FORMAT_ENUM_STRING( GL_RGB565,											false, "3-component 5:6:5,       unsigned normalized" ),
#endif
#if defined( GL_RGB10 )
	FORMAT_ENUM_STRING( GL_RGB10,											false, "3-component 10:10:10,    unsigned normalized" ),
#endif
#if defined( GL_RGB12 )
	FORMAT_ENUM_STRING( GL_RGB12,											false, "3-component 12:12:12,    unsigned normalized" ),
#endif
#if defined( GL_RGBA2 )
	FORMAT_ENUM_STRING( GL_RGBA2,											false, "4-component 2:2:2:2,     unsigned normalized" ),
#endif
#if defined( GL_RGBA4 )
	FORMAT_ENUM_STRING( GL_RGBA4,											false, "4-component 4:4:4:4,     unsigned normalized" ),
#endif
#if defined( GL_RGBA12 )
	FORMAT_ENUM_STRING( GL_RGBA12,											false, "4-component 12:12:12:12, unsigned normalized" ),
#endif
#if defined( GL_RGB5_A1 )
	FORMAT_ENUM_STRING( GL_RGB5_A1,											false, "4-component 5:5:5:1,     unsigned normalized" ),
#endif
#if defined( GL_RGB10_A2 )
	FORMAT_ENUM_STRING( GL_RGB10_A2,										false, "4-component 10:10:10:2,  unsigned normalized" ),
#endif
#if defined( GL_RGB10_A2UI )
	FORMAT_ENUM_STRING( GL_RGB10_A2UI,										false, "4-component 10:10:10:2,  unsigned integer" ),
#endif
#if defined( GL_R11F_G11F_B10F )
	FORMAT_ENUM_STRING( GL_R11F_G11F_B10F,									false, "3-component 11:11:10,    floating-point" ),
#endif
#if defined( GL_RGB9_E5 )
	FORMAT_ENUM_STRING( GL_RGB9_E5,											false, "3-component/exp 9:9:9/5, floating-point" ),
#endif

	//
	// Compressed formats
	//
#if defined( GL_PALETTE4_RGB8_OES )
	FORMAT_ENUM_STRING( GL_PALETTE4_RGB8_OES,								true, "3-component 8:8:8,   4-bit palette, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_PALETTE4_RGBA8_OES,								true, "4-component 8:8:8:8, 4-bit palette, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_PALETTE4_R5_G6_B5_OES,							true, "3-component 5:6:5,   4-bit palette, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_PALETTE4_RGBA4_OES,								true, "4-component 4:4:4:4, 4-bit palette, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_PALETTE4_RGB5_A1_OES,							true, "4-component 5:5:5:1, 4-bit palette, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_PALETTE8_RGB8_OES,								true, "3-component 8:8:8,   8-bit palette, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_PALETTE8_RGBA8_OES,								true, "4-component 8:8:8:8, 8-bit palette, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_PALETTE8_R5_G6_B5_OES,							true, "3-component 5:6:5,   8-bit palette, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_PALETTE8_RGBA4_OES,								true, "4-component 4:4:4:4, 8-bit palette, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_PALETTE8_RGB5_A1_OES,							true, "4-component 5:5:5:1, 8-bit palette, unsigned normalized" ),
#endif

#if defined( GL_COMPRESSED_RED )
	FORMAT_ENUM_STRING( GL_COMPRESSED_RED,									true, "1-component, generic, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RG,									true, "2-component, generic, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGB,									true, "3-component, generic, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA,									true, "4-component, generic, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB,									true, "3-component, generic, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB_ALPHA,							true, "4-component, generic, sRGB" ),
#endif

#if defined( GL_COMPRESSED_RED_RGTC1 )
	FORMAT_ENUM_STRING( GL_COMPRESSED_RED_RGTC1,							true, "1-component, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SIGNED_RED_RGTC1,						true, "1-component, signed normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RG_RGTC2,								true, "2-component, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SIGNED_RG_RGTC2,						true, "2-component, signed normalized" ),
#endif

#if defined( GL_COMPRESSED_RGBA_BPTC_UNORM )
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_BPTC_UNORM,						true, "4-component, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,				true, "4-component, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,				true, "3-component, signed floating-point" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,				true, "3-component, unsigned floating-point" ),
#endif

#if defined( GL_COMPRESSED_RGB_S3TC_DXT1_EXT )
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGB_S3TC_DXT1_EXT,					true, "line through 3D space, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,					true, "line through 3D space plus 1-bit alpha, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,					true, "line through 3D space plus line through 1D space, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,					true, "line through 3D space plus 4-bit alpha, unsigned normalized" ),
#endif

#if defined( GL_COMPRESSED_SRGB_S3TC_DXT1_EXT )
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB_S3TC_DXT1_EXT,					true, "line through 3D space, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,				true, "line through 3D space plus 1-bit alpha, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,				true, "line through 3D space plus line through 1D space, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,				true, "line through 3D space plus 4-bit alpha, sRGB" ),
#endif

#if defined( GL_COMPRESSED_LUMINANCE_LATC1_EXT )
	FORMAT_ENUM_STRING( GL_COMPRESSED_LUMINANCE_LATC1_EXT,					true, "line through 1D space, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,			true, "line through 2D space, unsigned normalized" ),

	FORMAT_ENUM_STRING( GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT,			true, "line through 1D space, signed normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT,		true, "line through 2D space, signed normalized" ),
#endif

#if defined( GL_ATC_RGB_AMD )
	FORMAT_ENUM_STRING( GL_ATC_RGB_AMD,										true, "3-component, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_ATC_RGBA_EXPLICIT_ALPHA_AMD,						true, "4-component, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD,					true, "4-component, unsigned normalized" ),
#endif

#if defined( GL_ETC1_RGB8_OES )
	FORMAT_ENUM_STRING( GL_ETC1_RGB8_OES,									true, "3-component ETC1, unsigned normalized" ),
#endif

#if defined( GL_COMPRESSED_RGB8_ETC2 )
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGB8_ETC2,							true, "3-component ETC2, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,		true, "4-component with 1-bit alpha ETC2, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA8_ETC2_EAC,						true, "4-component ETC2, unsigned normalized" ),

	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ETC2,							true, "3-component ETC2, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,		true, "4-component with 1-bit alpha ETC2, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,				true, "4-component ETC2, sRGB" ),
#endif

#if defined( GL_COMPRESSED_R11_EAC )
	FORMAT_ENUM_STRING( GL_COMPRESSED_R11_EAC,								true, "1-component ETC, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SIGNED_R11_EAC,						true, "1-component ETC, signed normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RG11_EAC,								true, "2-component ETC, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SIGNED_RG11_EAC,						true, "2-component ETC, signed normalized" ),
#endif

#if defined( GL_COMPRESSED_RGBA_ASTC_4x4_KHR )
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_4x4_KHR,					true, "4-component ASTC, 4x4 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_5x4_KHR,					true, "4-component ASTC, 5x4 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_5x5_KHR,					true, "4-component ASTC, 5x5 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_6x5_KHR,					true, "4-component ASTC, 6x5 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_6x6_KHR,					true, "4-component ASTC, 6x6 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_8x5_KHR,					true, "4-component ASTC, 8x5 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_8x6_KHR,					true, "4-component ASTC, 8x6 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_8x8_KHR,					true, "4-component ASTC, 8x8 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_10x5_KHR,					true, "4-component ASTC, 10x5 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_10x6_KHR,					true, "4-component ASTC, 10x6 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_10x8_KHR,					true, "4-component ASTC, 10x8 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_10x10_KHR,					true, "4-component ASTC, 10x10 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_12x10_KHR,					true, "4-component ASTC, 12x10 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_12x12_KHR,					true, "4-component ASTC, 12x12 blocks, unsigned normalized" ),

	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR,			true, "4-component ASTC, 4x4 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR,			true, "4-component ASTC, 5x4 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR,			true, "4-component ASTC, 5x5 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR,			true, "4-component ASTC, 6x5 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR,			true, "4-component ASTC, 6x6 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR,			true, "4-component ASTC, 8x5 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR,			true, "4-component ASTC, 8x6 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR,			true, "4-component ASTC, 8x8 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR,			true, "4-component ASTC, 10x5 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR,			true, "4-component ASTC, 10x6 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR,			true, "4-component ASTC, 10x8 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR,			true, "4-component ASTC, 10x10 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR,			true, "4-component ASTC, 12x10 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR,			true, "4-component ASTC, 12x12 blocks, sRGB" ),
#endif

#if defined( GL_COMPRESSED_RGBA_ASTC_3x3x3_OES )
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_3x3x3_OES,					true, "4-component ASTC, 3x3x3 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_4x3x3_OES,					true, "4-component ASTC, 4x3x3 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_4x4x3_OES,					true, "4-component ASTC, 4x4x3 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_4x4x4_OES,					true, "4-component ASTC, 4x4x4 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_5x4x4_OES,					true, "4-component ASTC, 5x4x4 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_5x5x4_OES,					true, "4-component ASTC, 5x5x4 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_5x5x5_OES,					true, "4-component ASTC, 5x5x5 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_6x5x5_OES,					true, "4-component ASTC, 6x5x5 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_6x6x5_OES,					true, "4-component ASTC, 6x6x5 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_RGBA_ASTC_6x6x6_OES,					true, "4-component ASTC, 6x6x6 blocks, unsigned normalized" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES,			true, "4-component ASTC, 3x3x3 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES,			true, "4-component ASTC, 4x3x3 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES,			true, "4-component ASTC, 4x4x3 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES,			true, "4-component ASTC, 4x4x4 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES,			true, "4-component ASTC, 5x4x4 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES,			true, "4-component ASTC, 5x5x4 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES,			true, "4-component ASTC, 5x5x5 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES,			true, "4-component ASTC, 6x5x5 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES,			true, "4-component ASTC, 6x6x5 blocks, sRGB" ),
	FORMAT_ENUM_STRING( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES,			true, "4-component ASTC, 6x6x6 blocks, sRGB" ),
#endif
};

static const char * GetTextureEnumString( GLenum e )
{
	for ( int i = 0; i < ARRAY_SIZE( formats ); i++ )
	{
		if ( formats[i].value == e )
		{
			return formats[i].string;
		}
	}
	static char hex[32];
	sprintf( hex, "0x%4X", e );
	return hex;
}

#define LIMIT_ENUM_STRING( e, c, d )		{ e, #e, c, d }

static struct
{
	GLenum			value;
	const char *	string;
	GLint			count;
	const char *	description;
}
limits[] =
{
	//
	// Vertex Shaders
	//
#if defined( GL_MAX_VERTEX_UNIFORM_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_VERTEX_UNIFORM_COMPONENTS,					1, "" ),
#endif
#if defined( GL_MAX_VERTEX_UNIFORM_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_VERTEX_UNIFORM_BLOCKS,						1, "" ),
#endif
#if defined( GL_MAX_VERTEX_UNIFORM_VECTORS )
	LIMIT_ENUM_STRING( GL_MAX_VERTEX_UNIFORM_VECTORS,						1, "" ),
#endif
#if defined( GL_MAX_VERTEX_ATTRIBS )
	LIMIT_ENUM_STRING( GL_MAX_VERTEX_ATTRIBS,								1, "" ),
#endif
#if defined( GL_MAX_VERTEX_OUTPUT_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_VERTEX_OUTPUT_COMPONENTS,						1, "" ),
#endif
#if defined( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS )
	LIMIT_ENUM_STRING( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,					1, "" ),
#endif
#if defined( GL_MAX_VERTEX_IMAGE_UNIFORMS )
	LIMIT_ENUM_STRING( GL_MAX_VERTEX_IMAGE_UNIFORMS,						1, "" ),
#endif
#if defined( GL_MAX_VERTEX_ATOMIC_COUNTERS )
	LIMIT_ENUM_STRING( GL_MAX_VERTEX_ATOMIC_COUNTERS,						1, "" ),
#endif
#if defined( GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS )
	LIMIT_ENUM_STRING( GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS,				1, "" ),
#endif
#if defined( GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,					1, "" ),
#endif

	//
	// Tesselation Control Shaders
	//
#if defined( GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS,				1, "" ),
#endif
#if defined( GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS,					1, "" ),
#endif
#if defined( GL_MAX_TESS_CONTROL_INPUT_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_CONTROL_INPUT_COMPONENTS,				1, "" ),
#endif
#if defined( GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS,				1, "" ),
#endif
#if defined( GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS,				1, "" ),
#endif
#if defined( GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS,					1, "" ),
#endif
#if defined( GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS,					1, "" ),
#endif
#if defined( GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS,			1, "" ),
#endif
#if defined( GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS,			1, "" ),
#endif
#if defined( GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS,			1, "" ),
#endif

	//
	// Tesselation Evaluation Shaders
	//
#if defined( GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS,			1, "" ),
#endif
#if defined( GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS,				1, "" ),
#endif
#if defined( GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS,				1, "" ),
#endif
#if defined( GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS,			1, "" ),
#endif
#if defined( GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS,			1, "" ),
#endif
#if defined( GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS,				1, "" ),
#endif
#if defined( GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS,				1, "" ),
#endif
#if defined( GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS,		1, "" ),
#endif
#if defined( GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS,		1, "" ),
#endif

	//
	// Geometry Shaders
	//
#if defined( GL_MAX_GEOMETRY_UNIFORM_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_GEOMETRY_UNIFORM_COMPONENTS,					1, "" ),
#endif
#if defined( GL_MAX_GEOMETRY_UNIFORM_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_GEOMETRY_UNIFORM_BLOCKS,						1, "" ),
#endif
#if defined( GL_MAX_GEOMETRY_INPUT_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_GEOMETRY_INPUT_COMPONENTS,					1, "" ),
#endif
#if defined( GL_MAX_GEOMETRY_OUTPUT_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_GEOMETRY_OUTPUT_COMPONENTS,					1, "" ),
#endif
#if defined( GL_MAX_GEOMETRY_OUTPUT_VERTICES )
	LIMIT_ENUM_STRING( GL_MAX_GEOMETRY_OUTPUT_VERTICES,						1, "" ),
#endif
#if defined( GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS )
	LIMIT_ENUM_STRING( GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS,					1, "" ),
#endif
#if defined( GL_MAX_GEOMETRY_IMAGE_UNIFORMS )
	LIMIT_ENUM_STRING( GL_MAX_GEOMETRY_IMAGE_UNIFORMS,						1, "" ),
#endif
#if defined( GL_MAX_GEOMETRY_ATOMIC_COUNTERS )
	LIMIT_ENUM_STRING( GL_MAX_GEOMETRY_ATOMIC_COUNTERS,						1, "" ),
#endif
#if defined( GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS )
	LIMIT_ENUM_STRING( GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS,				1, "" ),
#endif
#if defined( GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS,				1, "" ),
#endif
#if defined( GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS,				1, "" ),
#endif

	//
	// Fragment Shaders
	//
#if defined( GL_MAX_FRAGMENT_UNIFORM_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,					1, "" ),
#endif
#if defined( GL_MAX_FRAGMENT_UNIFORM_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_FRAGMENT_UNIFORM_BLOCKS,						1, "" ),
#endif
#if defined( GL_MAX_FRAGMENT_UNIFORM_VECTORS )
	LIMIT_ENUM_STRING( GL_MAX_FRAGMENT_UNIFORM_VECTORS,						1, "" ),
#endif
#if defined( GL_MAX_FRAGMENT_INPUT_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_FRAGMENT_INPUT_COMPONENTS,					1, "" ),
#endif
#if defined( GL_MAX_FRAGMENT_IMAGE_UNIFORMS )
	LIMIT_ENUM_STRING( GL_MAX_FRAGMENT_IMAGE_UNIFORMS,						1, "" ),
#endif
#if defined( GL_MAX_FRAGMENT_ATOMIC_COUNTERS )
	LIMIT_ENUM_STRING( GL_MAX_FRAGMENT_ATOMIC_COUNTERS,						1, "" ),
#endif
#if defined( GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS )
	LIMIT_ENUM_STRING( GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS,				1, "" ),
#endif
#if defined( GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,				1, "" ),
#endif

	//
	// Compute Shaders
	//
#if defined( GL_MAX_COMPUTE_UNIFORM_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_COMPUTE_UNIFORM_COMPONENTS,					1, "" ),
#endif
#if defined( GL_MAX_COMPUTE_UNIFORM_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_COMPUTE_UNIFORM_BLOCKS,						1, "" ),
#endif
#if defined( GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS )
	LIMIT_ENUM_STRING( GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS,					1, "" ),
#endif
#if defined( GL_MAX_COMPUTE_IMAGE_UNIFORMS )
	LIMIT_ENUM_STRING( GL_MAX_COMPUTE_IMAGE_UNIFORMS,						1, "" ),
#endif
#if defined( GL_MAX_COMPUTE_ATOMIC_COUNTERS )
	LIMIT_ENUM_STRING( GL_MAX_COMPUTE_ATOMIC_COUNTERS,						1, "" ),
#endif
#if defined( GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS )
	LIMIT_ENUM_STRING( GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS,				1, "" ),
#endif
#if defined( GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,				1, "" ),
#endif

	//
	// General Shaders
	//
#if defined( GL_MAX_TEXTURE_UNITS )
	LIMIT_ENUM_STRING( GL_MAX_TEXTURE_UNITS,								1, "" ),
#endif
#if defined( GL_MAX_IMAGE_UNITS )
	LIMIT_ENUM_STRING( GL_MAX_IMAGE_UNITS,									1, "" ),
#endif
#if defined( GL_MAX_TEXTURE_IMAGE_UNITS )
	LIMIT_ENUM_STRING( GL_MAX_TEXTURE_IMAGE_UNITS,							1, "" ),
#endif
#if defined( GL_MAX_UNIFORM_BUFFER_BINDINGS )
	LIMIT_ENUM_STRING( GL_MAX_UNIFORM_BUFFER_BINDINGS,						1, "" ),
#endif
#if defined( GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS )
	LIMIT_ENUM_STRING( GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,				1, "" ),
#endif
#if defined( GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS )
	LIMIT_ENUM_STRING( GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS,				1, "" ),
#endif
#if defined( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS )
	LIMIT_ENUM_STRING( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,					1, "" ),
#endif
#if defined( GL_MAX_COMBINED_UNIFORM_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_COMBINED_UNIFORM_BLOCKS,						1, "" ),
#endif
#if defined( GL_MAX_COMBINED_ATOMIC_COUNTERS )
	LIMIT_ENUM_STRING( GL_MAX_COMBINED_ATOMIC_COUNTERS,						1, "" ),
#endif
#if defined( GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS )
	LIMIT_ENUM_STRING( GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS,				1, "" ),
#endif
#if defined( GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS )
	LIMIT_ENUM_STRING( GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS,				1, "" ),
#endif
#if defined( GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES )
	LIMIT_ENUM_STRING( GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES,				1, "" ),
#endif
#if defined( GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS )
	LIMIT_ENUM_STRING( GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS,			1, "" ),
#endif
#if defined( GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS,		1, "" ),
#endif
#if defined( GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS )
	LIMIT_ENUM_STRING( GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS,	1, "" ),
#endif
#if defined( GL_MAX_TRANSFORM_FEEDBACK_BUFFERS )
	LIMIT_ENUM_STRING( GL_MAX_TRANSFORM_FEEDBACK_BUFFERS,					1, "" ),
#endif

	//
	// Textures
	//
#if defined( GL_MAX_TEXTURE_COORDS )
	LIMIT_ENUM_STRING( GL_MAX_TEXTURE_COORDS,								1, "" ),
#endif
#if defined( GL_MAX_TEXTURE_LOD_BIAS )
	LIMIT_ENUM_STRING( GL_MAX_TEXTURE_LOD_BIAS,								1, "" ),
#endif
#if defined( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT )
	LIMIT_ENUM_STRING( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,					1, "" ),
#endif
#if defined( GL_MAX_TEXTURE_SIZE )
	LIMIT_ENUM_STRING( GL_MAX_TEXTURE_SIZE,									1, "" ),
#endif
#if defined( GL_MAX_CUBE_MAP_TEXTURE_SIZE )
	LIMIT_ENUM_STRING( GL_MAX_CUBE_MAP_TEXTURE_SIZE,						1, "" ),
#endif
#if defined( GL_MAX_RECTANGLE_TEXTURE_SIZE )
	LIMIT_ENUM_STRING( GL_MAX_RECTANGLE_TEXTURE_SIZE,						1, "" ),
#endif
#if defined( GL_MAX_3D_TEXTURE_SIZE )
	LIMIT_ENUM_STRING( GL_MAX_3D_TEXTURE_SIZE,								1, "" ),
#endif
#if defined( GL_MAX_ARRAY_TEXTURE_LAYERS )
	LIMIT_ENUM_STRING( GL_MAX_ARRAY_TEXTURE_LAYERS,							1, "" ),
#endif

	//
	// Misc
	//
#if defined( GL_MAX_CLIP_PLANES )
	LIMIT_ENUM_STRING( GL_MAX_CLIP_PLANES,									1, "maximum number of clip planes" ),
#endif
#if defined( GL_MAX_COLOR_ATTACHMENTS )
	LIMIT_ENUM_STRING( GL_MAX_COLOR_ATTACHMENTS,							1, "maximum number of framebuffer color attachments" ),
#endif
#if defined( GL_MAX_DRAW_BUFFERS )
	LIMIT_ENUM_STRING( GL_MAX_DRAW_BUFFERS,									1, "maximum number of draw buffers" ),
#endif
#if defined( GL_MAX_RENDERBUFFER_SIZE )
	LIMIT_ENUM_STRING( GL_MAX_RENDERBUFFER_SIZE,							1, "maximum width and height of a renderbuffer" ),
#endif
#if defined( GL_MAX_VIEWPORT_DIMS )
	LIMIT_ENUM_STRING( GL_MAX_VIEWPORT_DIMS,								1, "maximum width and height of a viewport" ),
#endif
#if defined( GL_MAX_SAMPLES )
	LIMIT_ENUM_STRING( GL_MAX_SAMPLES,										1, "maximum number of samples for multisampling" ),
#endif
#if defined( GL_MAX_VARYING_VECTORS )
	LIMIT_ENUM_STRING( GL_MAX_VARYING_VECTORS,								1, "maximum number 4-element float vectors for varying variables" ),
#endif
#if defined( GL_MAX_EVAL_ORDER )
	LIMIT_ENUM_STRING( GL_MAX_EVAL_ORDER,									1, "maximum order of an evaluator" ),
#endif
#if defined( GL_AUX_BUFFERS )
	LIMIT_ENUM_STRING( GL_AUX_BUFFERS,										1, "maximum number of auxiliary color buffers" ),
#endif
#if defined( GL_SUBPIXEL_BITS )
	LIMIT_ENUM_STRING( GL_SUBPIXEL_BITS,									1, "estimate of the number of bits of subpixel resolution" ),
#endif
#if defined( GL_MAX_ELEMENTS_INDICES )
	LIMIT_ENUM_STRING( GL_MAX_ELEMENTS_INDICES,								1, "maximum number of indices to glDrawRangeElements" ),
#endif
#if defined( GL_MAX_ELEMENTS_VERTICES )
	LIMIT_ENUM_STRING( GL_MAX_ELEMENTS_VERTICES,							1, "maximum number of vertices to glDrawRangeElements" ),
#endif
#if defined( GL_MIN_PROGRAM_TEXEL_OFFSET )
	LIMIT_ENUM_STRING( GL_MIN_PROGRAM_TEXEL_OFFSET,							1, "minimum offset for a texture lookup with offset" ),
#endif
#if defined( GL_MAX_PROGRAM_TEXEL_OFFSET )
	LIMIT_ENUM_STRING( GL_MAX_PROGRAM_TEXEL_OFFSET,							1, "maximum offset for a texture lookup with offset" ),
#endif

	//
	// Points & Lines
	//
#if defined( GL_ALIASED_LINE_WIDTH_RANGE )
	LIMIT_ENUM_STRING( GL_ALIASED_LINE_WIDTH_RANGE,							2, "" ),
#endif
#if defined( GL_ALIASED_POINT_SIZE_RANGE )
	LIMIT_ENUM_STRING( GL_ALIASED_POINT_SIZE_RANGE,							2, "" ),
#endif
#if defined( GL_SMOOTH_LINE_WIDTH_GRANULARITY )
	LIMIT_ENUM_STRING( GL_SMOOTH_LINE_WIDTH_GRANULARITY,					1, "" ),
#endif
#if defined( GL_SMOOTH_LINE_WIDTH_RANGE )
	LIMIT_ENUM_STRING( GL_SMOOTH_LINE_WIDTH_RANGE,							2, "" ),
#endif
#if defined( GL_SMOOTH_POINT_SIZE_GRANULARITY )
	LIMIT_ENUM_STRING( GL_SMOOTH_POINT_SIZE_GRANULARITY,					1, "" ),
#endif
#if defined( GL_SMOOTH_POINT_SIZE_RANGE )
	LIMIT_ENUM_STRING( GL_SMOOTH_POINT_SIZE_RANGE,							2, "" ),
#endif
};

static GLint glGetInteger( GLenum pname )
{
	GLint i;
	glGetIntegerv( pname, &i );
	return i;
}

#define COLUMN_WIDTH	"50"

int main( int argc, char * argv[] )
{
	argc = argc;
	argv = argv;

	Console_Resize( 4096, 120 );

	GpuContext_t context;
	GpuContext_Create( &context );
	GpuContext_SetCurrent( &context );

	Print( "--------------------------------\n" );
	Print( "%-"COLUMN_WIDTH"s: %s\n", "OS", GetOSVersion() );
	Print( "%-"COLUMN_WIDTH"s: %s\n", "CPU", GetCPUVersion() );
	Print( "%-"COLUMN_WIDTH"s: %s\n", "GPU", glGetString( GL_RENDERER ) );
	Print( "%-"COLUMN_WIDTH"s: %s\n", "Vendor", glGetString( GL_VENDOR ) );
	Print( "%-"COLUMN_WIDTH"s: %s\n", "OpenGL", glGetString( GL_VERSION ) );
	Print( "%-"COLUMN_WIDTH"s: %s\n", "GLSL", glGetString( GL_SHADING_LANGUAGE_VERSION ) );

	// Extension strings
	{
#if defined( OS_WINDOWS ) || defined( OS_LINUX )
		PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC) GetExtension( "glGetStringi" );
#endif
		GL( const GLint numExtensions = glGetInteger( GL_NUM_EXTENSIONS ) );
		for ( int i = 0; i < numExtensions; i++ )
		{
			GL( const GLubyte * string = glGetStringi( GL_EXTENSIONS, i ) );
			Print( "%-"COLUMN_WIDTH"s%c %s\n", ( i == 0 ? "Extensions" : "" ), ( i == 0 ? ':' : ' ' ), string );
		}
	}

	// WGL, GLX, EGL extension strings
	{
#if defined( OS_WINDOWS )
		PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC) GetExtension( "wglGetExtensionsStringARB" );
		const char * string = ( wglGetExtensionsStringARB != NULL ) ? wglGetExtensionsStringARB( context.hDC ) : "";
		#define WSI_TYPE "WGL"
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB )
		PFNGLXQUERYEXTENSIONSSTRINGPROC glXQueryExtensionsString = (PFNGLXQUERYEXTENSIONSSTRINGPROC) GetExtension( "glXQueryExtensionsString" );
		const char * string = ( glXQueryExtensionsString != NULL ) ? glXQueryExtensionsString( context.display, context.screen ) : "";
		#define WSI_TYPE "GLX"
#elif defined( OS_ANDROID )
		const char * string = eglQueryString( context.display, EGL_EXTENSIONS );
		#define WSI_TYPE "EGL"
#endif
		for ( int i = 0; string[0] != '\0'; i++ )
		{
			while ( string[0] == ' ' ) { string++; }
			int index = 0;
			while ( string[index] > ' ' ) { index++; }
			if ( index <= 0 )
			{
				break;
			}
			char buffer[128];
			const int length = ( index < sizeof( buffer ) ) ? index : sizeof( buffer ) - 1;
			strncpy( buffer, string, length );
			buffer[length] = '\0';
			Print( "%-"COLUMN_WIDTH"s%c %s\n", ( i == 0 ? WSI_TYPE " Extensions" : "" ), ( i == 0 ? ':' : ' ' ), buffer );
			string += index;
		}
	}

	// Supported texture formats
	{
		for ( int i = 0; i < ARRAY_SIZE( formats ); i++ )
		{
			if ( formats[i].compressed == false )
			{
				Print( "%-"COLUMN_WIDTH"s%c %s\n", ( i == 0 ? "Texture Formats" : "" ), ( i == 0 ? ':' : ' ' ), formats[i].string );
			}
		}
	}

	// Supported compressed texture formats
	{
		const GLint numCompressedTextures = glGetInteger( GL_NUM_COMPRESSED_TEXTURE_FORMATS );
		GLint * compressedTextures = (GLint *) malloc( numCompressedTextures * sizeof( compressedTextures[0] ) );
		GL( glGetIntegerv( GL_COMPRESSED_TEXTURE_FORMATS, compressedTextures ) );
		for ( int i = 0; i < numCompressedTextures; i++ )
		{
			const char * string = GetTextureEnumString( (GLenum)compressedTextures[i] );
			Print( "%-"COLUMN_WIDTH"s%c %s\n", ( i == 0 ? "Compressed Texture Formats" : "" ), ( i == 0 ? ':' : ' ' ), string );
		}
		free( compressedTextures );
	}

	// Limits
	for ( int i = 0; i < ARRAY_SIZE( limits ); i++ )
	{
		GLint values[2] = { 0 };
		GL( glGetIntegerv( limits[i].value, values ) );
		if ( limits[i].count == 1 )
		{
			Print( "%-"COLUMN_WIDTH"s: %-12d  %s\n", limits[i].string, values[0], limits[i].description );
		}
		else
		{
			Print( "%-"COLUMN_WIDTH"s: [%4d, %4d]  %s\n", limits[i].string, values[0], values[1], limits[i].description );
		}
	}

	Print( "--------------------------------\n" );

	GpuContext_Destroy( &context );

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

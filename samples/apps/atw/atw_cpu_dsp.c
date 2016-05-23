/*
================================================================================================

Description	:	CPU and DSP Time Warp
Author		:	J.M.P. van Waveren
Date		:	04/01/2014
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

This implements the simplest form of time warp transform to run on a CPU or DSP.
This transform corrects for optical aberration of the optics used in a virtual
reality headset, and only rotates the stereoscopic images based on the very latest
head orientation to reduce the motion-to-photon delay (or end-to-end latency).

There are 5 implementations that use different texture filters and/or source data
layouts. Only the last implementation also corrects for chromatic aberration.

        | sampling  | source data | chromatic aberration correction
    ----|-----------|-------------|--------------------------------
    1.  | nearest   | packed RGBA | no
    2.  | linear    | packed RGBA | no
    3.  | bilinear  | packed RGBA | no
    4.  | bilinear  | planar RGB  | no
    5.  | bilinear  | planar RGB  | yes


IMPLEMENTATION
==============

The code is written in straight C99 for maximum portability and readability.
To further improve portability and to simplify compilation, all source code
is in a single file without any dependencies on third-party code or non-standard
libraries.

Supported platforms are:

	- Microsoft Windows 7 or later
	- Apple Mac OS X 10.9 or later
	- Ubuntu Linux 14.04 or later
	- Android 5.0 or later


OPTIMIZATION
============

Each implementation has been optimized for the following SIMD instruction sets:

	- x86 Streaming SIMD Extensions 2 (SSE2)
	- x86 Streaming SIMD Extensions 4 (SSE4)
	- x86 Advanced Vector Extensions 2 (AVX2)
	- ARM NEON
	- Hexagon QDSP6 V50

To test the x86 SIMD extensions exclusively, make sure to also set the appropriate
compiler option: /arch:SSE2, /arch:SSE4.1, /arc:AVX, /arch:CORE-AVX2


COMMAND-LINE COMPILATION
========================

Microsoft Windows: Visual Studio 2013 Compiler:
	"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
	cl /Zc:forScope /Wall /WX /MD /GS /Gy /O2 /Oi /arch:AVX atw_cpu_dsp.c

Microsoft Windows: Intel Compiler 14.0
	"C:\Program Files (x86)\Intel\Composer XE\bin\iclvars.bat" intel64
	icl /Qstd=c99 /Zc:forScope /Wall /WX /MD /GS /Gy /O2 /Oi /arch:CORE-AVX2 atw_cpu_dsp.c

Apple Mac OS: Apple LLVM 6.0:
	clang -std=c99 -march=native -Wall -g -O2 -m64 -o atw_cpu_dsp atw_cpu_dsp.c

Linux: GCC 4.8.2:
	gcc -std=c99 -march=native -Wall -g -O2 -m64 -o atw_cpu_dsp atw_cpu_dsp.c -lm

Android for ARM from Windows: NDK Revision 11c - Android 21
	set path=%path%;%ANDROID_NDK_HOME%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin\
	arm-linux-androideabi-gcc -std=c99 -Wall -g -O2 -march=armv7-a -mfloat-abi=softfp -mfpu=neon -o atw_cpu_dsp atw_cpu_dsp.c -lm -llog --sysroot=%ANDROID_NDK_HOME%/platforms/android-21/arch-arm
	adb root
	adb remount
	adb shell mkdir -p /sdcard/atw/images/
	adb shell rm /sdcard/atw/images/ *
	adb push atw_cpu_dsp /data/local/
	adb shell chmod 777 /data/local/atw_cpu_dsp
	adb shell /data/local/atw_cpu_dsp
	adb pull /sdcard/atw/images/ .


VERSION HISTORY
===============

1.0		Initial version.

================================================================================================
*/

#if defined( WIN32 ) || defined( _WIN32 ) || defined( WIN64 ) || defined( _WIN64 )
	#define OS_WINDOWS
#elif defined( __ANDROID__ )
	#define OS_ANDROID
#elif defined( __hexagon__ ) || defined( __qdsp6__ )
	#define OS_HEXAGON
#elif defined( __APPLE__ )
	#define OS_MAC
#elif defined( __linux__ )
	#define OS_LINUX
#else
	#error "unknown platform"
#endif

#if defined( OS_WINDOWS ) || defined( OS_MAC ) || defined( OS_LINUX )

/*
================================
Windows, MacOS, Linux x86 or x64
================================
*/

#if !defined( _CRT_SECURE_NO_WARNINGS )
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef _MSC_VER
#pragma warning( disable : 4204 )	// nonstandard extension used : non-constant aggregate initializer
#pragma warning( disable : 4255 )	// '<name>' : no function prototype given: converting '()' to '(void)'
#pragma warning( disable : 4668 )	// '__cplusplus' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning( disable : 4711 )	// function '<name>' selected for automatic inline expansion
#pragma warning( disable : 4820 )	// '<name>' : 'X' bytes padding added after data member '<member>'
#endif

#ifdef __ICL
#pragma warning( disable : 2415 )	// variable X of static storage duration was declared but never referenced
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-const-variable"
#pragma clang diagnostic ignored "-Wself-assign"
#endif

#include <stdio.h>					// for printf()
#include <math.h>					// for tanf()
#if defined( OS_WINDOWS )
#include <intrin.h>					// for SSE intrinsics
#else
#include <x86intrin.h>				// for SSE intrinsics
#endif

#if 0	// using the hardware prefetcher works well
#define CACHE_LINE_SIZE				64
#define PrefetchLinear( a, b )		for ( int o = 0; o < (b); o += CACHE_LINE_SIZE ) \
									{ \
										_mm_prefetch( (const char *) (a) + (o), _MM_HINT_NTA ); \
									}
#define PrefetchBox( a, w, h, s )	for ( int r = 0; r < (h); r++ ) \
									{ \
										for ( int o = 0; o < (w); o += CACHE_LINE_SIZE ) \
										{ \
											_mm_prefetch( (const char *) (a) + r * (s) + (o), _MM_HINT_NTA ); \
										} \
									}
#define ZeroCacheLinear( a, b )		do {} while( (a) && (b) && 0 )
#define ZeroCacheBox( a, w, h, s )	do {} while( (a) && (w) && (h) && (s) && 0 )
#define FlushCacheLinear( a, b )	do {} while( (a) && (b) && 0 )
#define FlushCacheBox( a, w, h, s )	do {} while( (a) && (w) && (h) && (s) && 0 )
#endif

#define INLINE						__inline
#define LOG(...)					printf( __VA_ARGS__ )

// To test one of these exclusively make sure to also set the appropriate compiler option: /arch:SSE2, /arch:SSE4.1, /arch:CORE-AVX2
#define __USE_SSE2__
#define __USE_SSE4__				// SSE4 is only needed for _mm_extract_epi32() and _mm_insert_epi32(), otherwise SSSE3 would suffice
#define __USE_AVX2__

#elif defined( OS_ANDROID )

/*
================================
Android ARM
================================
*/

#include <android/log.h>			// for __android_log_print()
#include <math.h>					// for tanf()
#include <arm_neon.h>				// for NEON

#if 1	// manual prefetching improves the performance
#define CACHE_LINE_SIZE				64
#define PrefetchLinear( a, b )		for ( int o = 0; o < (b); o += CACHE_LINE_SIZE ) \
									{ \
										__builtin_prefetch( (const char *) (a) + (o) ); \
									}
#define PrefetchBox( a, w, h, s )	for ( int r = 0; r < (h); r++ ) \
									{ \
										for ( int o = 0; o < (w); o += CACHE_LINE_SIZE ) \
										{ \
											__builtin_prefetch( (const char *) (a) + r * (s) + (o) ); \
										} \
									}
#define ZeroCacheLinear( a, b )		do {} while( (a) && (b) && 0 )
#define ZeroCacheBox( a, w, h, s )	do {} while( (a) && (w) && (h) && (s) && 0 )
#define FlushCacheLinear( a, b )	do {} while( (a) && (b) && 0 )
#define FlushCacheBox( a, w, h, s )	do {} while( (a) && (w) && (h) && (s) && 0 )
#endif

#define INLINE						inline
#define LOG(...)					printf( __VA_ARGS__ )
#define OUTPUT						"/sdcard/atw/images/"

//#undef __ARM_NEON__

#elif defined( OS_HEXAGON )

/*
================================
Hexagon QDSP6
================================
*/

#include <math.h>					// for tanf()
#include "HAP_farf.h"
#include "hexagon_types.h"
#include "hexagon_protos.h"

#if 1	// manual prefetching significantly improves the performance
#include "dsp_cache.h"
#define CACHE_LINE_SIZE				32
#define PrefetchLinear( a, b )		dspcache_l2fetch_linear( (a), (b) )
#define PrefetchBox( a, w, h, s )	dspcache_l2fetch_box( (a), (w), (h), (s) )
#define ZeroCacheLinear( a, b )		dspcache_dczeroa_linear( (a), (b) )
#define ZeroCacheBox( a, w, h, s )	dspcache_dczeroa_box( (a), (w), (h), (s) )
#define FlushCacheLinear( a, b )	dspcache_flush_invalidate_linear( (a), (b) )
#define FlushCacheBox( a, w, h, s )	dspcache_flush_invalidate_box( (a), (w), (h), (s) )
#endif

#define INLINE						inline
#define LOG(...)					FARF( DEBUG_MSG, __VA_ARGS__ )
#define FARF_DEBUG_MSG				1
#define FARF_DEBUG_MSG_LEVEL		HAP_LEVEL_HIGH

#if !defined( __HEXAGON_V50__ )
#define __HEXAGON_V50__				1	// Snapdragon 805 (8x84)
#endif
#if !defined( __HEXAGON_V56__ )
#define __HEXAGON_V56__				1	// Snapdragon 810 (8x94)
#endif
#if !defined( __HEXAGON_V60__ )
#define __HEXAGON_V60__				1	// Snapdragon 820 (8x96)
#endif

typedef unsigned char Byte;
typedef unsigned int Word32;
typedef unsigned long long Word64;

inline Word32 Q6_R_extract_Pl( Word64 Rss ) { return (Word32)( Rss ); }
inline Word32 Q6_R_extract_Ph( Word64 Rss ) { return (Word32)( Rss >> 32 ); }
inline Word64 Q6_P_memb_fifo_PRI( Word64 Rss, const Byte * Rt, Word32 s11 ) { return ( Rss >> 8 ) | ( (Word64)Rt[s11] << 56 ); }

#else	// assume generic x86 or x64 platform

/*
================================
Generic x86 or x64
================================
*/

#include <stdio.h>					// for printf()
#include <math.h>					// for tanf()
#include <intrin.h>					// for SSE intrinsics

#if 0	// using the hardware prefetcher works well
#define CACHE_LINE_SIZE				64
#define PrefetchLinear( a, b )		for ( int o = 0; o < (b); o += CACHE_LINE_SIZE ) \
									{ \
										_mm_prefetch( (const char *) (a) + (o), _MM_HINT_NTA ); \
									}
#define PrefetchBox( a, w, h, s )	for ( int r = 0; r < (h); r++ ) \
									{ \
										for ( int o = 0; o < (w); o += CACHE_LINE_SIZE ) \
										{ \
											_mm_prefetch( (const char *) (a) + r * (s) + (o), _MM_HINT_NTA ); \
										} \
									}
#define ZeroCacheLinear( a, b )		do {} while( (a) && (b) && 0 )
#define ZeroCacheBox( a, w, h, s )	do {} while( (a) && (w) && (h) && (s) && 0 )
#define FlushCacheLinear( a, b )	do {} while( (a) && (b) && 0 )
#define FlushCacheBox( a, w, h, s )	do {} while( (a) && (w) && (h) && (s) && 0 )
#endif

#define INLINE						inline
#define LOG(...)					printf( __VA_ARGS__ )

#define __USE_SSE2__
#define __USE_SSE4__				// SSE4 is only needed for _mm_extract_epi32() and _mm_insert_epi32(), otherwise SSSE3 would suffice
#define __USE_AVX2__

#endif

/*
================================
SSE and AVX vector constants
================================
*/

#define _S16( a )						( (a) >> 0 ) & 0xFF, ( (a) >> 8 ) & 0xFF
#define _S32( a )						( (a) >> 0 ) & 0xFF, ( (a) >> 8 ) & 0xFF, ( (a) >> 16 ) & 0xFF, ( (a) >> 24 ) & 0xFF

#define _C( x, s )						( (long long)x << s )
#define _C8( a, b, c, d, e, f, g, h )	( _C( a, 0 ) | _C( b, 8 ) | _C( c, 16 ) | _C( d, 24 ) | _C( e, 32 ) | _C( f, 40 ) | _C( g, 48 ) | _C( h, 56 ) )
#define _C16( a, b, c, d )				( _C( a, 0 ) | _C( b, 16 ) | _C( c, 32 ) | _C( d, 48 ) )
#define _C32( a, b )					( _C( a, 0 ) | _C( b, 32 ) )

#if defined( __USE_SSE2__ ) || defined( __USE_SSE4__ )

#if defined( _MSC_VER )
// MSVC: static initialization of __m128i as 16x 8-bit integers
// typedef union __declspec(intrin_type) __declspec(align(16)) __m128i
// {
//     __int8           m128i_i8[16];
//     __int16          m128i_i16[8];
//     __int32          m128i_i32[4];    
//     __int64          m128i_i64[2];
//     unsigned __int8  m128i_u8[16];
//     unsigned __int16 m128i_u16[8];
//     unsigned __int32 m128i_u32[4];
//     unsigned __int64 m128i_u64[2];
// } __m128i;
#define _MM_SET1_EPI8( x )												{ x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x }
#define _MM_SET_EPI8( p, o, n, m, l, k, j, i, h, g, f, e, d, c, b, a )	{ a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p }
#define _MM_SET1_EPI16( x )												{ _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ) }
#define _MM_SET_EPI16( h, g, f, e, d, c, b, a )							{ _S16( a ), _S16( b ), _S16( c ), _S16( d ), _S16( e ), _S16( f ), _S16( g ), _S16( h ) }
#define _MM_SET1_EPI32( x )												{ _S32( x ), _S32( x ), _S32( x ), _S32( x ) }
#define _MM_SET_EPI32( d, c, b, a )										{ _S32( a ), _S32( b ), _S32( c ), _S32( d ) }
#else
// GCC/Clang/LLVM: static initialization of __m128i as 2x 64-bit integers
// typedef long long __m128i __attribute__ ((__vector_size__ (16), __may_alias__));
#define _MM_SET1_EPI8( x )												{ _C8( x, x, x, x, x, x, x, x ), _C8( x, x, x, x, x, x, x, x ) }
#define _MM_SET_EPI8( p, o, n, m, l, k, j, i, h, g, f, e, d, c, b, a )	{ _C8( a, b, c, d, e, f, g, h ), _C8( i, j, k, l, m, n, o, p ) }
#define _MM_SET1_EPI16( x )												{ _C16( x, x, x, x ), _C16( x, x, x, x ) }
#define _MM_SET_EPI16( h, g, f, e, d, c, b, a )							{ _C16( a, b, c, d ), _C16( e, f, g, h ) }
#define _MM_SET1_EPI32( x )												{ _C32( x, x ), _C32( x, x ) }
#define _MM_SET_EPI32( d, c, b, a )										{ _C32( a, b ), _C32( c, d ) }
#endif

static const __m128i vector_uint8_0				= _MM_SET1_EPI8( 0 );
static const __m128i vector_uint8_127			= _MM_SET1_EPI8( 127 );
static const __m128i vector_uint8_255			= _MM_SET1_EPI8( -1 );
static const __m128i vector_uint8_unpack_hilo	= _MM_SET_EPI8( 15, 11, 14, 10, 13, 9, 12, 8, 7, 3, 6, 2, 5, 1, 4, 0 );

static const __m128i vector_int16_1				= _MM_SET1_EPI16( 1 );
static const __m128i vector_int16_127			= _MM_SET1_EPI16( 127 );
static const __m128i vector_int16_128			= _MM_SET1_EPI16( 128 );
static const __m128i vector_int16_255			= _MM_SET1_EPI16( 255 );
static const __m128i vector_int16_unpack_hilo	= _MM_SET_EPI8( 15, 14, 7, 6, 13, 12, 5, 4, 11, 10, 3, 2, 9, 8, 1, 0 );
static const __m128i vector_int16_01234567		= _MM_SET_EPI16( 7, 6, 5, 4, 3, 2, 1, 0 );

static const __m128i vector_int32_1				= _MM_SET1_EPI32( 1 );
static const __m128i vector_int32_127			= _MM_SET1_EPI32( 127 );
static const __m128i vector_int32_255			= _MM_SET1_EPI32( 255 );

#define _mm_loadh_epi64( x, address )			_mm_castps_si128( _mm_loadh_pi( _mm_castsi128_ps( x ), (__m64 *)(address) ) )
#define _mm_pack_epi32( a, b )					_mm_packs_epi32( _mm_srai_epi32( _mm_slli_epi32( a, 16 ), 16 ), _mm_srai_epi32( _mm_slli_epi32( b, 16 ), 16 ) )

#endif

#if defined( __USE_AVX2__ )

#if defined( _MSC_VER )
// MSVC: static initialization of __m256i as 32x 8-bit integers
// typedef union  __declspec(intrin_type) __declspec(align(32)) __m256i
// {
//     __int8           m256i_i8[32];
//     __int16          m256i_i16[16];
//     __int32          m256i_i32[8];
//     __int64          m256i_i64[4];
//     unsigned __int8  m256i_u8[32];
//     unsigned __int16 m256i_u16[16];
//     unsigned __int32 m256i_u32[8];
//     unsigned __int64 m256i_u64[4];
// } __m256i;
#define _MM256_SET1_EPI8( x )									{ x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x }
#define _MM256_SET_EPI8(	p1, o1, n1, m1, l1, k1, j1, i1, \
							h1, g1, f1, e1, d1, c1, b1, a1, \
							p0, o0, n0, m0, l0, k0, j0, i0, \
							h0, g0, f0, e0, d0, c0, b0, a0 )	{ a0, b0, c0, d0, e0, f0, g0, h0, i0, j0, k0, l0, m0, n0, o0, p0, a1, b1, c1, d1, e1, f1, g1, h1, i1, j1, k1, l1, m1, n1, o1, p1 }
#define _MM256_SET1_EPI16( x )									{ _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ), _S16( x ) }
#define _MM256_SET_EPI16(	p, o, n, m, l, k, j, i, \
							h, g, f, e, d, c, b, a )			{ _S16( a ), _S16( b ), _S16( c ), _S16( d ), _S16( e ), _S16( f ), _S16( g ), _S16( h ), _S16( i ), _S16( j ), _S16( k ), _S16( l ), _S16( m ), _S16( n ), _S16( o ), _S16( p ) }
#define _MM256_SET1_EPI32( x )									{ _S32( x ), _S32( x ), _S32( x ), _S32( x ), _S32( x ), _S32( x ), _S32( x ), _S32( x ) }
#define _MM256_SET_EPI32( h, g, f, e, d, c, b, a )				{ _S32( a ), _S32( b ), _S32( c ), _S32( d ), _S32( c ), _S32( d ), _S32( e ), _S32( f ) }
#else
// GCC/Clang/LLVM: static initialization of __m256i as 4x 64-bit integers
// typedef long long __m256i __attribute__ ((__vector_size__ (32), __may_alias__));
#define _MM256_SET1_EPI8( x )									{ _C8( x, x, x, x, x, x, x, x ), _C8( x, x, x, x, x, x, x, x ), _C8( x, x, x, x, x, x, x, x ), _C8( x, x, x, x, x, x, x, x ) }
#define _MM256_SET_EPI8(	p1, o1, n1, m1, l1, k1, j1, i1, \
							h1, g1, f1, e1, d1, c1, b1, a1, \
							p0, o0, n0, m0, l0, k0, j0, i0, \
							h0, g0, f0, e0, d0, c0, b0, a0 )	{ _C8( a0, b0, c0, d0, e0, f0, g0, h0 ), _C8( i0, j0, k0, l0, m0, n0, o0, p0 ), _C8( a1, b1, c1, d1, e1, f1, g1, h1 ), _C8( i1, j1, k1, l1, m1, n1, o1, p1 ) }
#define _MM256_SET1_EPI16( x )									{ _C16( x, x, x, x ), _C16( x, x, x, x ), _C16( x, x, x, x ), _C16( x, x, x, x ) }
#define _MM256_SET_EPI16(	p, o, n, m, l, k, j, i, \
							h, g, f, e, d, c, b, a )			{ _C16( a, b, c, d ), _C16( e, f, g, h ), _C16( i, j, k, l ), _C16( m, n, o, p ) }
#define _MM256_SET1_EPI32( x )									{ _C32( x, x ), _C32( x, x ), _C32( x, x ), _C32( x, x ) }
#define _MM256_SET_EPI32( h, g, f, e, d, c, b, a )				{ _C32( a, b ), _C32( c, d ), _C32( e, f ), _C32( g, h ) }
#endif

static const __m256i vector256_uint8_unpack_hilo		= _MM256_SET_EPI8( 15, 11, 14, 10, 13, 9, 12, 8, 7, 3, 6, 2, 5, 1, 4, 0, 15, 11, 14, 10, 13, 9, 12, 8, 7, 3, 6, 2, 5, 1, 4, 0 );

static const __m256i vector256_int16_1					= _MM256_SET1_EPI16( 1 );
static const __m256i vector256_int16_127				= _MM256_SET1_EPI16( 127 );
static const __m256i vector256_int16_128				= _MM256_SET1_EPI16( 128 );
static const __m256i vector256_int16_255				= _MM256_SET1_EPI16( 255 );
static const __m256i vector256_int16_unpack_hilo		= _MM256_SET_EPI8( 15, 14, 7, 6, 13, 12, 5, 4, 11, 10, 3, 2, 9, 8, 1, 0, 15, 14, 7, 6, 13, 12, 5, 4, 11, 10, 3, 2, 9, 8, 1, 0 );
static const __m256i vector256_int16_012389AB4567CDEF	= _MM256_SET_EPI16( 15, 14, 13, 12, 7, 6, 5, 4, 11, 10, 9, 8, 3, 2, 1, 0 );

static const __m256i vector256_int32_1					= _MM256_SET1_EPI32( 1 );
static const __m256i vector256_int32_01010101			= _MM256_SET_EPI32( 1, 0, 1, 0, 1, 0, 1, 0 );
static const __m256i vector256_int32_127				= _MM256_SET1_EPI32( 127 );

#define _mm256_pack_epi32( a, b )						_mm256_packs_epi32( _mm256_srai_epi32( _mm256_slli_epi32( a, 16 ), 16 ), _mm256_srai_epi32( _mm256_slli_epi32( b, 16 ), 16 ) )

#endif

/*
================================
Default to no cache management
================================
*/

#if !defined( CACHE_LINE_SIZE )
#define CACHE_LINE_SIZE				64
#define PrefetchLinear( a, b )		do {} while( (a) && (b) && 0 )
#define PrefetchBox( a, w, h, s )	do {} while( (a) && (w) && (h) && (s) && 0 )
#define ZeroCacheLinear( a, b )		do {} while( (a) && (b) && 0 )
#define ZeroCacheBox( a, w, h, s )	do {} while( (a) && (w) && (h) && (s) && 0 )
#define FlushCacheLinear( a, b )	do {} while( (a) && (b) && 0 )
#define FlushCacheBox( a, w, h, s )	do {} while( (a) && (w) && (h) && (s) && 0 )
#endif

#if !defined( OUTPUT )
#define OUTPUT ""
#endif

#define ARRAY_SIZE( a )				( sizeof( (a) ) / sizeof( (a)[0] ) )

/*
================================
Fast integer operations
================================
*/

#if defined( OS_HEXAGON )
INLINE int MinInt( const int x, const int y ) { return Q6_R_min_RR( x, y ); }
INLINE int MaxInt( const int x, const int y ) { return Q6_R_max_RR( x, y ); }
INLINE int MinInt4( const int x, const int y, const int z, const int w ) { return Q6_R_min_RR( Q6_R_min_RR( x, y ), Q6_R_min_RR( z, w ) ); }
INLINE int MaxInt4( const int x, const int y, const int z, const int w ) { return Q6_R_max_RR( Q6_R_max_RR( x, y ), Q6_R_max_RR( z, w ) ); }
INLINE int AbsInt( const int x ) { return Q6_R_abs_R( x ); }
INLINE int ClampInt( const int x, const int min, const int max ) { return Q6_R_min_RR( Q6_R_max_RR( min, x ), max ); }
#else
INLINE int MinInt( const int x, const int y ) { return y + ( ( x - y ) & ( ( x - y ) >> ( sizeof( int ) * 8 - 1 ) ) ); }
INLINE int MaxInt( const int x, const int y ) { return x - ( ( x - y ) & ( ( x - y ) >> ( sizeof( int ) * 8 - 1 ) ) ); }
INLINE int MinInt4( const int x, const int y, const int z, const int w ) { return MinInt( MinInt( x, y ), MinInt( z, w ) ); }
INLINE int MaxInt4( const int x, const int y, const int z, const int w ) { return MaxInt( MaxInt( x, y ), MaxInt( z, w ) ); }
INLINE int AbsInt( const int x ) { const int mask = x >> ( sizeof( int ) * 8 - 1 ); return ( x + mask ) ^ mask; }
INLINE int ClampInt( const int x, const int min, const int max ) { return min + ( ( AbsInt( x - min ) - AbsInt( x - max ) + max - min ) >> 1 ); }
#endif

/*
================================================================================================
32x32 Warp
================================================================================================
*/

typedef struct
{
	float x;
	float y;
} MeshCoord_t;

void Warp32x32_SampleNearestPackedRGB(
		const unsigned char * const	src,
		const int					srcPitchInTexels,
		const int					srcTexelsWide,
		const int					srcTexelsHigh,
		unsigned char * const		dest,
		const int					destPitchInPixels,
		const MeshCoord_t *			meshCoords,
		const int					meshStride )
{
	// The source texture needs to be sampled with the texture coordinate at the center of each destination pixel.
	// In other words, the texture coordinates are offset by 1/64 of the texture space spanned by the 32x32 destination quad.

	const int L32 = 5;	// log2( 32 )
	const int SCP = 16;	// scan-conversion precision
	const int STP = 8;	// sub-texel precision

	//ZeroCacheBox( dest, 32 * 4, 32, destPitchInPixels * 4 );

	// Clamping the corners may distort quads that sample close to the edges, but that should not be noticable because these quads
	// are close to the far peripheral vision, where the human eye is weak when it comes to distinguishing color and shape.
	int clampedCorners[4][2];
	for ( int i = 0; i < 4; i++ )
	{
		clampedCorners[i][0] = ClampInt( (int)( meshCoords[( i >> 1 ) * meshStride + ( i & 1 )].x * ( srcTexelsWide << SCP ) ), 0, ( srcTexelsWide - 1 ) << SCP );
		clampedCorners[i][1] = ClampInt( (int)( meshCoords[( i >> 1 ) * meshStride + ( i & 1 )].y * ( srcTexelsHigh << SCP ) ), 0, ( srcTexelsHigh - 1 ) << SCP );
	}

	// calculate the axis-aligned bounding box of source texture space that may be sampled
	const int minSrcX = ( MinInt4( clampedCorners[0][0], clampedCorners[1][0], clampedCorners[2][0], clampedCorners[3][0] ) >> SCP ) + 0;
	const int maxSrcX = ( MaxInt4( clampedCorners[0][0], clampedCorners[1][0], clampedCorners[2][0], clampedCorners[3][0] ) >> SCP ) + 1;
	const int minSrcY = ( MinInt4( clampedCorners[0][1], clampedCorners[1][1], clampedCorners[2][1], clampedCorners[3][1] ) >> SCP ) + 0;
	const int maxSrcY = ( MaxInt4( clampedCorners[0][1], clampedCorners[1][1], clampedCorners[2][1], clampedCorners[3][1] ) >> SCP ) + 1;

	// prefetch all source texture data that is possibly sampled
	PrefetchBox( src + ( minSrcY * srcPitchInTexels + minSrcX ) * 4, ( maxSrcX - minSrcX ) * 4, ( maxSrcY - minSrcY ), srcPitchInTexels * 4 );

	// vertical deltas in 16.16 fixed point
	const int scanLeftDeltaX  = ( clampedCorners[2][0] - clampedCorners[0][0] ) >> L32;
	const int scanLeftDeltaY  = ( clampedCorners[2][1] - clampedCorners[0][1] ) >> L32;
	const int scanRightDeltaX = ( clampedCorners[3][0] - clampedCorners[1][0] ) >> L32;
	const int scanRightDeltaY = ( clampedCorners[3][1] - clampedCorners[1][1] ) >> L32;

	// scan-line texture coordinates in 16.16 fixed point with half-pixel vertical offset
	int scanLeftSrcX  = clampedCorners[0][0] + ( ( clampedCorners[2][0] - clampedCorners[0][0] ) >> ( L32 + 1 ) );
	int scanLeftSrcY  = clampedCorners[0][1] + ( ( clampedCorners[2][1] - clampedCorners[0][1] ) >> ( L32 + 1 ) );
	int scanRightSrcX = clampedCorners[1][0] + ( ( clampedCorners[3][0] - clampedCorners[1][0] ) >> ( L32 + 1 ) );
	int scanRightSrcY = clampedCorners[1][1] + ( ( clampedCorners[3][1] - clampedCorners[1][1] ) >> ( L32 + 1 ) );

	for ( int y = 0; y < 32; y++ )
	{
		// scan-line texture coordinates in 16.16 fixed point with half-pixel horizontal offset
		const int srcX16 = scanLeftSrcX + ( ( scanRightSrcX - scanLeftSrcX ) >> ( L32 + 1 ) );
		const int srcY16 = scanLeftSrcY + ( ( scanRightSrcY - scanLeftSrcY ) >> ( L32 + 1 ) );

		// horizontal deltas in 16.16 fixed point
		const int deltaX16 = ( scanRightSrcX - scanLeftSrcX ) >> L32;
		const int deltaY16 = ( scanRightSrcY - scanLeftSrcY ) >> L32;

		// get the sign of the deltas
		const int deltaSignX = ( deltaX16 >> 31 );
		const int deltaSignY = ( deltaY16 >> 31 );

		// reduce the deltas to 16.8 fixed-point (may be negative sign extended)
		const int deltaX8 = ( ( ( ( deltaX16 ^ deltaSignX ) - deltaSignX ) >> ( SCP - STP ) ) ^ deltaSignX ) - deltaSignX;
		const int deltaY8 = ( ( ( ( deltaY16 ^ deltaSignY ) - deltaSignY ) >> ( SCP - STP ) ) ^ deltaSignY ) - deltaSignY;

		// reduce the source coordinates to 16.8 fixed-point
		const int srcX8 = srcX16 >> ( SCP - STP );
		const int srcY8 = srcY16 >> ( SCP - STP );

		// get the top-left corner of the bounding box of the texture space sampled by this scan-line
		const int srcBoundsTopLeftX = MinInt( scanLeftSrcX, scanRightSrcX ) >> SCP;
		const int srcBoundsTopLeftY = MinInt( scanLeftSrcY, scanRightSrcY ) >> SCP;

		// localize the source pointer and source coordinates to allow using 8.8 fixed point
		const unsigned int * const localSrc = (const unsigned int *)src + ( srcBoundsTopLeftY * srcPitchInTexels + srcBoundsTopLeftX );
		int localSrcX8 = srcX8 - ( srcBoundsTopLeftX << STP );
		int localSrcY8 = srcY8 - ( srcBoundsTopLeftY << STP );

		unsigned int * destRow = (unsigned int *)dest + y * destPitchInPixels;

#if defined( __USE_AVX2__ )

		__m256i sx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcX8 ) );
		__m256i sy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcY8 ) );
		__m256i dx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaX8 ) );
		__m256i dy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaY8 ) );
		__m256i pitch = _mm256_unpacklo_epi16( _mm256_broadcastw_epi16( _mm_cvtsi32_si128( srcPitchInTexels ) ), vector256_int16_1 );

		sx = _mm256_add_epi16( sx, _mm256_mullo_epi16( dx, vector256_int16_012389AB4567CDEF ) );
		sy = _mm256_add_epi16( sy, _mm256_mullo_epi16( dy, vector256_int16_012389AB4567CDEF ) );
		dx = _mm256_slli_epi16( dx, 4 );
		dy = _mm256_slli_epi16( dy, 4 );

		for ( int x = 0; x < 32; x += 16 )
		{
			__m256i ax = _mm256_srai_epi16( sx, STP );
			__m256i ay = _mm256_srai_epi16( sy, STP );
			__m256i of0 = _mm256_madd_epi16( _mm256_unpacklo_epi16( ay, ax ), pitch );
			__m256i of1 = _mm256_madd_epi16( _mm256_unpackhi_epi16( ay, ax ), pitch );

#if 1
			__m256i d0 = _mm256_i32gather_epi32( (const int *)localSrc, of0, 4 );
			__m256i d1 = _mm256_i32gather_epi32( (const int *)localSrc, of1, 4 );
#else
			__m128i o0 = _mm256_extracti128_si256( of0, 0 );
			__m128i o1 = _mm256_extracti128_si256( of0, 1 );
			__m128i o2 = _mm256_extracti128_si256( of1, 0 );
			__m128i o3 = _mm256_extracti128_si256( of1, 1 );

			const unsigned int a0 = _mm_extract_epi32( o0, 0 );
			const unsigned int a1 = _mm_extract_epi32( o0, 1 );
			const unsigned int a2 = _mm_extract_epi32( o0, 2 );
			const unsigned int a3 = _mm_extract_epi32( o0, 3 );
			const unsigned int a4 = _mm_extract_epi32( o1, 0 );
			const unsigned int a5 = _mm_extract_epi32( o1, 1 );
			const unsigned int a6 = _mm_extract_epi32( o1, 2 );
			const unsigned int a7 = _mm_extract_epi32( o1, 3 );
			const unsigned int a8 = _mm_extract_epi32( o2, 0 );
			const unsigned int a9 = _mm_extract_epi32( o2, 1 );
			const unsigned int aA = _mm_extract_epi32( o2, 2 );
			const unsigned int aB = _mm_extract_epi32( o2, 3 );
			const unsigned int aC = _mm_extract_epi32( o3, 0 );
			const unsigned int aD = _mm_extract_epi32( o3, 1 );
			const unsigned int aE = _mm_extract_epi32( o3, 2 );
			const unsigned int aF = _mm_extract_epi32( o3, 3 );

			__m128i d0 = _mm_cvtsi32_si128( localSrc[a0] );
			d0 = _mm_insert_epi32( d0, localSrc[a1], 1 );
			d0 = _mm_insert_epi32( d0, localSrc[a2], 2 );
			d0 = _mm_insert_epi32( d0, localSrc[a3], 3 );
			__m128i d1 = _mm_cvtsi32_si128( localSrc[a4] );
			d1 = _mm_insert_epi32( d1, localSrc[a5], 1 );
			d1 = _mm_insert_epi32( d1, localSrc[a6], 2 );
			d1 = _mm_insert_epi32( d1, localSrc[a7], 3 );
			__m128i d2 = _mm_cvtsi32_si128( localSrc[a8] );
			d2 = _mm_insert_epi32( d2, localSrc[a9], 1 );
			d2 = _mm_insert_epi32( d2, localSrc[aA], 2 );
			d2 = _mm_insert_epi32( d2, localSrc[aB], 3 );
			__m128i d3 = _mm_cvtsi32_si128( localSrc[aC] );
			d3 = _mm_insert_epi32( d3, localSrc[aD], 1 );
			d3 = _mm_insert_epi32( d3, localSrc[aE], 2 );
			d3 = _mm_insert_epi32( d3, localSrc[aF], 3 );

			d0 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), d0, 0 ), d1, 1 );
			d1 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), d2, 0 ), d3, 1 );
#endif

			_mm256_stream_si256( (__m256i *)( destRow + x + 0 ), d0 );
			_mm256_stream_si256( (__m256i *)( destRow + x + 8 ), d1 );

			sx = _mm256_add_epi16( sx, dx );
			sy = _mm256_add_epi16( sy, dy );
		}

#elif defined( __USE_SSE4__ )

		__m128i sx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcX8 ), 0 ), 0 );
		__m128i sy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcY8 ), 0 ), 0 );
		__m128i dx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaX8 ), 0 ), 0 );
		__m128i dy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaY8 ), 0 ), 0 );
		__m128i pitch = _mm_unpacklo_epi16( _mm_shufflelo_epi16( _mm_cvtsi32_si128( srcPitchInTexels ), 0 ), vector_int16_1 );

		sx = _mm_add_epi16( sx, _mm_mullo_epi16( dx, vector_int16_01234567 ) );
		sy = _mm_add_epi16( sy, _mm_mullo_epi16( dy, vector_int16_01234567 ) );
		dx = _mm_slli_epi16( dx, 3 );
		dy = _mm_slli_epi16( dy, 3 );

		for ( int x = 0; x < 32; x += 8 )
		{
			__m128i ax = _mm_srai_epi16( sx, STP );
			__m128i ay = _mm_srai_epi16( sy, STP );
			__m128i of0 = _mm_madd_epi16( _mm_unpacklo_epi16( ay, ax ), pitch );
			__m128i of1 = _mm_madd_epi16( _mm_unpackhi_epi16( ay, ax ), pitch );

			const unsigned int a0 = _mm_extract_epi32( of0, 0 );
			const unsigned int a1 = _mm_extract_epi32( of0, 1 );
			const unsigned int a2 = _mm_extract_epi32( of0, 2 );
			const unsigned int a3 = _mm_extract_epi32( of0, 3 );
			const unsigned int a4 = _mm_extract_epi32( of1, 0 );
			const unsigned int a5 = _mm_extract_epi32( of1, 1 );
			const unsigned int a6 = _mm_extract_epi32( of1, 2 );
			const unsigned int a7 = _mm_extract_epi32( of1, 3 );

			__m128i d0 = _mm_cvtsi32_si128( localSrc[a0] );
			d0 = _mm_insert_epi32( d0, localSrc[a1], 1 );
			d0 = _mm_insert_epi32( d0, localSrc[a2], 2 );
			d0 = _mm_insert_epi32( d0, localSrc[a3], 3 );
			__m128i d1 = _mm_cvtsi32_si128( localSrc[a4] );
			d1 = _mm_insert_epi32( d1, localSrc[a5], 1 );
			d1 = _mm_insert_epi32( d1, localSrc[a6], 2 );
			d1 = _mm_insert_epi32( d1, localSrc[a7], 3 );

			_mm_stream_si128( (__m128i *)( destRow + x + 0 ), d0 );
			_mm_stream_si128( (__m128i *)( destRow + x + 4 ), d1 );

			sx = _mm_add_epi16( sx, dx );
			sy = _mm_add_epi16( sy, dy );
		}

#elif defined( __USE_SSE2__ )

		__m128i sx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcX8 ), 0 ), 0 );
		__m128i sy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcY8 ), 0 ), 0 );
		__m128i dx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaX8 ), 0 ), 0 );
		__m128i dy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaY8 ), 0 ), 0 );
		__m128i pitch = _mm_unpacklo_epi16( _mm_shufflelo_epi16( _mm_cvtsi32_si128( srcPitchInTexels ), 0 ), vector_int16_1 );

		sx = _mm_add_epi16( sx, _mm_mullo_epi16( dx, vector_int16_01234567 ) );
		sy = _mm_add_epi16( sy, _mm_mullo_epi16( dy, vector_int16_01234567 ) );
		dx = _mm_slli_epi16( dx, 3 );
		dy = _mm_slli_epi16( dy, 3 );

		for ( int x = 0; x < 32; x += 8 )
		{
			__m128i ax = _mm_srai_epi16( sx, STP );
			__m128i ay = _mm_srai_epi16( sy, STP );
			__m128i of0 = _mm_madd_epi16( _mm_unpacklo_epi16( ay, ax ), pitch );
			__m128i of1 = _mm_madd_epi16( _mm_unpackhi_epi16( ay, ax ), pitch );

			const unsigned int a0 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 0 ) );
			const unsigned int a1 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 1 ) );
			const unsigned int a2 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 2 ) );
			const unsigned int a3 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 3 ) );
			const unsigned int a4 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 0 ) );
			const unsigned int a5 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 1 ) );
			const unsigned int a6 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 2 ) );
			const unsigned int a7 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 3 ) );

			destRow[x + 0] = localSrc[a0];
			destRow[x + 1] = localSrc[a1];
			destRow[x + 2] = localSrc[a2];
			destRow[x + 3] = localSrc[a3];
			destRow[x + 4] = localSrc[a4];
			destRow[x + 5] = localSrc[a5];
			destRow[x + 6] = localSrc[a6];
			destRow[x + 7] = localSrc[a7];

			sx = _mm_add_epi16( sx, dx );
			sy = _mm_add_epi16( sy, dy );
		}

#elif defined( __ARM_NEON__ )		// increased throughput

		__asm__ volatile(
			"	mov			r2, #4									\n\t"	// pixel pitch
			"	mov			r3, #16									\n\t"
			"	add			r4, %[d], #128							\n\t"	// &destRow[32]

			"	uxth		%[sx], %[sx]							\n\t"
			"	orr			%[sx], %[sx], %[sy], lsl #16			\n\t"
			"	uxth		%[dx], %[dx]							\n\t"
			"	orr			%[dx], %[dx], %[dy], lsl #16			\n\t"

			".LOOP_NEAREST_PACKED_RGB_1:							\n\t"
			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld1.u32	{d0[0]}, [r0], r2						\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld1.u32	{d0[1]}, [r0], r2						\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld1.u32	{d1[0]}, [r0], r2						\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld1.u32	{d1[1]}, [r0], r2						\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld1.u32	{d2[0]}, [r0], r2						\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld1.u32	{d2[1]}, [r0], r2						\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld1.u32	{d3[0]}, [r0], r2						\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld1.u32	{d3[1]}, [r0], r2						\n\t"

			"	vst1.u64	{d0, d1}, [%[d]], r3					\n\t"
			"	vst1.u64	{d2, d3}, [%[d]], r3					\n\t"

			"	cmp			%[d], r4								\n\t"	// destRow == ( dest + y * destPitchInPixels + 32 )
			"	bne			.LOOP_NEAREST_PACKED_RGB_1				\n\t"
			:
			:	[sx] "r" (localSrcX8),
				[sy] "r" (localSrcY8),
				[dx] "r" (deltaX8),
				[dy] "r" (deltaY8),
				[s] "r" (localSrc),
				[d] "r" (destRow),
				[p] "r" (srcPitchInTexels)
			:	"r0", "r1", "r2", "r3", "r4",
				"q0", "q1",
				"memory"
		);

#elif defined( __ARM_NEON__ )		// compressed computation

		__asm__ volatile(
			"	uxth		%[sx], %[sx]					\n\t"
			"	orr			%[sx], %[sx], %[sy], lsl #16	\n\t"
			"	uxth		%[dx], %[dx]					\n\t"
			"	orr			%[dx], %[dx], %[dy], lsl #16	\n\t"
			"	mov			r2, #32							\n\t"
			".LOOP_NEAREST_PACKED_RGB_2:					\n\t"
			"	sxtb		r0, %[sx], ror #8				\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24				\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0				\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	sadd16		%[sx], %[sx], %[dx]				\n\t"	// (srcX, srcY) += (deltaX, deltaY)
			"	ldr			r0, [%[s], r0, lsl #2]			\n\t"	// temp = localSrc[( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) * 4];
			"	str			r0, [%[d]], #4					\n\t"	// destRow[0] = temp; destRow++;
			"	subs		r2, r2, #1						\n\t"
			"	bne			.LOOP_NEAREST_PACKED_RGB_2		\n\t"
			:
			:	[sx] "r" (localSrcX8),
				[sy] "r" (localSrcY8),
				[dx] "r" (deltaX8),
				[dy] "r" (deltaY8),
				[s] "r" (localSrc),
				[d] "r" (destRow),
				[p] "r" (srcPitchInTexels)
			:	"r0", "r1", "r2", "r3", "r4",
				"memory"
		);

#elif defined( __HEXAGON_V50__ )	// 8 pixels per iteration appears to be optimal

		Word32 dxy1 = Q6_R_combine_RlRl( deltaY8, deltaX8 );
		Word64 dxy2 = Q6_P_vaslh_PI( Q6_P_combine_RR( dxy1, dxy1 ), 1 );
		Word64 dxy8 = Q6_P_vaslh_PI( dxy2, 2 );

		Word32 sxy00 = Q6_R_combine_RlRl( localSrcY8, localSrcX8 );
		Word64 sxy01 = Q6_P_combine_RR( Q6_R_vaddh_RR( sxy00, dxy1 ), sxy00 );
		Word64 sxy23 = Q6_P_vaddh_PP( sxy01, dxy2 );
		Word64 sxy45 = Q6_P_vaddh_PP( sxy23, dxy2 );
		Word64 sxy67 = Q6_P_vaddh_PP( sxy45, dxy2 );
	
		Word32 pch1 = Q6_R_combine_RlRl( srcPitchInTexels, Q6_R_equals_I( 1 ) );
		Word64 pch2 = Q6_P_combine_RR( pch1, pch1 );

		for ( int x = 0; x < 32; x += 8 )
		{
			Word64 offset0 = Q6_P_vdmpy_PP_sat( Q6_P_vasrh_PI( sxy01, STP ), pch2 );
			Word64 offset1 = Q6_P_vdmpy_PP_sat( Q6_P_vasrh_PI( sxy23, STP ), pch2 );
			Word64 offset2 = Q6_P_vdmpy_PP_sat( Q6_P_vasrh_PI( sxy45, STP ), pch2 );
			Word64 offset3 = Q6_P_vdmpy_PP_sat( Q6_P_vasrh_PI( sxy67, STP ), pch2 );

			*(Word64 *)&destRow[x + 0] = Q6_P_combine_RR( localSrc[Q6_R_extract_Ph( offset0 )], localSrc[Q6_R_extract_Pl( offset0 )] );
			*(Word64 *)&destRow[x + 2] = Q6_P_combine_RR( localSrc[Q6_R_extract_Ph( offset1 )], localSrc[Q6_R_extract_Pl( offset1 )] );
			*(Word64 *)&destRow[x + 4] = Q6_P_combine_RR( localSrc[Q6_R_extract_Ph( offset2 )], localSrc[Q6_R_extract_Pl( offset2 )] );
			*(Word64 *)&destRow[x + 6] = Q6_P_combine_RR( localSrc[Q6_R_extract_Ph( offset3 )], localSrc[Q6_R_extract_Pl( offset3 )] );

			sxy01 = Q6_P_vaddh_PP( sxy01, dxy8 );
			sxy23 = Q6_P_vaddh_PP( sxy23, dxy8 );
			sxy45 = Q6_P_vaddh_PP( sxy45, dxy8 );
			sxy67 = Q6_P_vaddh_PP( sxy67, dxy8 );
		}

#else

		for ( int x = 0; x < 32; x++ )
		{
			const int sampleX = localSrcX8 >> STP;
			const int sampleY = localSrcY8 >> STP;

			destRow[x] = *( localSrc + sampleY * srcPitchInTexels + sampleX );

			localSrcX8 += deltaX8;
			localSrcY8 += deltaY8;
		}

#endif

		scanLeftSrcX  += scanLeftDeltaX;
		scanLeftSrcY  += scanLeftDeltaY;
		scanRightSrcX += scanRightDeltaX;
		scanRightSrcY += scanRightDeltaY;
	}

	//FlushCacheBox( dest, 32 * 4, 32, destPitchInPixels * 4 );
}

void Warp32x32_SampleLinearPackedRGB(
		const unsigned char * const	src,
		const int					srcPitchInTexels,
		const int					srcTexelsWide,
		const int					srcTexelsHigh,
		unsigned char * const		dest,
		const int					destPitchInPixels,
		const MeshCoord_t *			meshCoords,
		const int					meshStride )
{
	// The source texture needs to be sampled with the texture coordinate at the center of each destination pixel.
	// In other words, the texture coordinates are offset by 1/64 of the texture space spanned by the 32x32 destination quad.

	const int L32 = 5;	// log2( 32 )
	const int SCP = 16;	// scan-conversion precision
#if defined( __USE_SSE4__ ) || defined( __USE_AVX2__ )
	const int STP = 7;	// sub-texel precision
#else
	const int STP = 8;	// sub-texel precision
#endif

	//ZeroCacheBox( dest, 32 * 4, 32, destPitchInPixels * 4 );

	// Clamping the corners may distort quads that sample close to the edges, but that should not be noticable because these quads
	// are close to the far peripheral vision, where the human eye is weak when it comes to distinguishing color and shape.
	int clampedCorners[4][2];
	for ( int i = 0; i < 4; i++ )
	{
		clampedCorners[i][0] = ClampInt( (int)( meshCoords[( i >> 1 ) * meshStride + ( i & 1 )].x * ( srcTexelsWide << SCP ) ), 0, ( srcTexelsWide - 2 ) << SCP );
		clampedCorners[i][1] = ClampInt( (int)( meshCoords[( i >> 1 ) * meshStride + ( i & 1 )].y * ( srcTexelsHigh << SCP ) ), 0, ( srcTexelsHigh - 1 ) << SCP );
	}

	// calculate the axis-aligned bounding box of source texture space that may be sampled
	const int minSrcX = ( MinInt4( clampedCorners[0][0], clampedCorners[1][0], clampedCorners[2][0], clampedCorners[3][0] ) >> SCP ) + 0;
	const int maxSrcX = ( MaxInt4( clampedCorners[0][0], clampedCorners[1][0], clampedCorners[2][0], clampedCorners[3][0] ) >> SCP ) + 1;
	const int minSrcY = ( MinInt4( clampedCorners[0][1], clampedCorners[1][1], clampedCorners[2][1], clampedCorners[3][1] ) >> SCP ) + 0;
	const int maxSrcY = ( MaxInt4( clampedCorners[0][1], clampedCorners[1][1], clampedCorners[2][1], clampedCorners[3][1] ) >> SCP ) + 1;

	// prefetch all source texture data that is possibly sampled
	PrefetchBox( src + ( minSrcY * srcPitchInTexels + minSrcX ) * 4, ( maxSrcX - minSrcX ) * 4, ( maxSrcY - minSrcY ), srcPitchInTexels * 4 );

	// vertical deltas in 16.16 fixed point
	const int scanLeftDeltaX  = ( clampedCorners[2][0] - clampedCorners[0][0] ) >> L32;
	const int scanLeftDeltaY  = ( clampedCorners[2][1] - clampedCorners[0][1] ) >> L32;
	const int scanRightDeltaX = ( clampedCorners[3][0] - clampedCorners[1][0] ) >> L32;
	const int scanRightDeltaY = ( clampedCorners[3][1] - clampedCorners[1][1] ) >> L32;

	// scan-line texture coordinates in 16.16 fixed point with half-pixel vertical offset
	int scanLeftSrcX  = clampedCorners[0][0] + ( ( clampedCorners[2][0] - clampedCorners[0][0] ) >> ( L32 + 1 ) );
	int scanLeftSrcY  = clampedCorners[0][1] + ( ( clampedCorners[2][1] - clampedCorners[0][1] ) >> ( L32 + 1 ) );
	int scanRightSrcX = clampedCorners[1][0] + ( ( clampedCorners[3][0] - clampedCorners[1][0] ) >> ( L32 + 1 ) );
	int scanRightSrcY = clampedCorners[1][1] + ( ( clampedCorners[3][1] - clampedCorners[1][1] ) >> ( L32 + 1 ) );

	for ( int y = 0; y < 32; y++ )
	{
		// scan-line texture coordinates in 16.16 fixed point with half-pixel horizontal offset
		const int srcX16 = scanLeftSrcX + ( ( scanRightSrcX - scanLeftSrcX ) >> ( L32 + 1 ) );
		const int srcY16 = scanLeftSrcY + ( ( scanRightSrcY - scanLeftSrcY ) >> ( L32 + 1 ) );

		// horizontal deltas in 16.16 fixed point
		const int deltaX16 = ( scanRightSrcX - scanLeftSrcX ) >> L32;
		const int deltaY16 = ( scanRightSrcY - scanLeftSrcY ) >> L32;

		// get the sign of the deltas
		const int deltaSignX = ( deltaX16 >> 31 );
		const int deltaSignY = ( deltaY16 >> 31 );

		// reduce the deltas to 16.8 fixed-point (may be negative sign extended)
		const int deltaX8 = ( ( ( ( deltaX16 ^ deltaSignX ) - deltaSignX ) >> ( SCP - STP ) ) ^ deltaSignX ) - deltaSignX;
		const int deltaY8 = ( ( ( ( deltaY16 ^ deltaSignY ) - deltaSignY ) >> ( SCP - STP ) ) ^ deltaSignY ) - deltaSignY;

		// reduce the source coordinates to 16.8 fixed-point
		const int srcX8 = srcX16 >> ( SCP - STP );
		const int srcY8 = srcY16 >> ( SCP - STP );

		// get the top-left corner of the bounding box of the texture space sampled by this scan-line
		const int srcBoundsTopLeftX = MinInt( scanLeftSrcX, scanRightSrcX ) >> SCP;
		const int srcBoundsTopLeftY = MinInt( scanLeftSrcY, scanRightSrcY ) >> SCP;

		// localize the source pointer and source coordinates to allow using 8.8 fixed point
		const unsigned int * const localSrc = (const unsigned int *)src + ( srcBoundsTopLeftY * srcPitchInTexels + srcBoundsTopLeftX );
		int localSrcX8 = srcX8 - ( srcBoundsTopLeftX << STP );
		int localSrcY8 = srcY8 - ( srcBoundsTopLeftY << STP );

		unsigned int * destRow = (unsigned int *)dest + y * destPitchInPixels;

#if defined( __USE_AVX2__ )

		// This version uses VPMADDUBSW which unfortunately multiplies an unsigned byte with a *signed* byte.
		// As a result, any fraction over 127 will be interpreted as a negative fraction. To keep the
		// fractions positive, the sub-texel precision is reduced to just 7 bits and there is a 1/128 loss
		// in brightness when interpolating horizontally because the fraction 1.0 (128) cannot be used.

		__m256i sx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcX8 ) );
		__m256i sy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcY8 ) );
		__m256i dx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaX8 ) );
		__m256i dy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaY8 ) );
		__m256i pitch = _mm256_unpacklo_epi16( _mm256_broadcastw_epi16( _mm_cvtsi32_si128( srcPitchInTexels ) ), vector256_int16_1 );

		sx = _mm256_add_epi16( sx, _mm256_mullo_epi16( dx, vector256_int16_012389AB4567CDEF ) );
		sy = _mm256_add_epi16( sy, _mm256_mullo_epi16( dy, vector256_int16_012389AB4567CDEF ) );
		dx = _mm256_slli_epi16( dx, 4 );
		dy = _mm256_slli_epi16( dy, 4 );

		for ( int x = 0; x < 32; x += 16 )
		{
			__m256i ax = _mm256_srai_epi16( sx, STP );
			__m256i ay = _mm256_srai_epi16( sy, STP );
			__m256i of0 = _mm256_madd_epi16( _mm256_unpacklo_epi16( ay, ax ), pitch );
			__m256i of1 = _mm256_madd_epi16( _mm256_unpackhi_epi16( ay, ax ), pitch );

#if 1
			__m256i o0 = _mm256_add_epi32( _mm256_unpacklo_epi32( of0, of0 ), vector256_int32_01010101 );
			__m256i o1 = _mm256_add_epi32( _mm256_unpackhi_epi32( of0, of0 ), vector256_int32_01010101 );
			__m256i o2 = _mm256_add_epi32( _mm256_unpacklo_epi32( of1, of1 ), vector256_int32_01010101 );
			__m256i o3 = _mm256_add_epi32( _mm256_unpackhi_epi32( of1, of1 ), vector256_int32_01010101 );

			__m256i r0 = _mm256_i32gather_epi32( (const int *)localSrc, o0, 4 );
			__m256i r1 = _mm256_i32gather_epi32( (const int *)localSrc, o1, 4 );
			__m256i r2 = _mm256_i32gather_epi32( (const int *)localSrc, o2, 4 );
			__m256i r3 = _mm256_i32gather_epi32( (const int *)localSrc, o3, 4 );
#else
			__m128i o0 = _mm256_extracti128_si256( of0, 0 );
			__m128i o1 = _mm256_extracti128_si256( of0, 1 );
			__m128i o2 = _mm256_extracti128_si256( of1, 0 );
			__m128i o3 = _mm256_extracti128_si256( of1, 1 );

			const unsigned int a0 = _mm_extract_epi32( o0, 0 );
			const unsigned int a1 = _mm_extract_epi32( o0, 1 );
			const unsigned int a2 = _mm_extract_epi32( o0, 2 );
			const unsigned int a3 = _mm_extract_epi32( o0, 3 );
			const unsigned int a4 = _mm_extract_epi32( o1, 0 );
			const unsigned int a5 = _mm_extract_epi32( o1, 1 );
			const unsigned int a6 = _mm_extract_epi32( o1, 2 );
			const unsigned int a7 = _mm_extract_epi32( o1, 3 );
			const unsigned int a8 = _mm_extract_epi32( o2, 0 );
			const unsigned int a9 = _mm_extract_epi32( o2, 1 );
			const unsigned int aA = _mm_extract_epi32( o2, 2 );
			const unsigned int aB = _mm_extract_epi32( o2, 3 );
			const unsigned int aC = _mm_extract_epi32( o3, 0 );
			const unsigned int aD = _mm_extract_epi32( o3, 1 );
			const unsigned int aE = _mm_extract_epi32( o3, 2 );
			const unsigned int aF = _mm_extract_epi32( o3, 3 );

			__m128i t0 = _mm_loadh_epi64( _mm_loadl_epi64( (const __m128i *)(localSrc + a0) ), (const __m128i *)(localSrc + a1) );
			__m128i t1 = _mm_loadh_epi64( _mm_loadl_epi64( (const __m128i *)(localSrc + a2) ), (const __m128i *)(localSrc + a3) );
			__m128i t2 = _mm_loadh_epi64( _mm_loadl_epi64( (const __m128i *)(localSrc + a4) ), (const __m128i *)(localSrc + a5) );
			__m128i t3 = _mm_loadh_epi64( _mm_loadl_epi64( (const __m128i *)(localSrc + a6) ), (const __m128i *)(localSrc + a7) );
			__m128i t4 = _mm_loadh_epi64( _mm_loadl_epi64( (const __m128i *)(localSrc + a8) ), (const __m128i *)(localSrc + a9) );
			__m128i t5 = _mm_loadh_epi64( _mm_loadl_epi64( (const __m128i *)(localSrc + aA) ), (const __m128i *)(localSrc + aB) );
			__m128i t6 = _mm_loadh_epi64( _mm_loadl_epi64( (const __m128i *)(localSrc + aC) ), (const __m128i *)(localSrc + aD) );
			__m128i t7 = _mm_loadh_epi64( _mm_loadl_epi64( (const __m128i *)(localSrc + aE) ), (const __m128i *)(localSrc + aF) );

			__m256i r0 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), t0, 0 ), t2, 1 );
			__m256i r1 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), t1, 0 ), t3, 1 );
			__m256i r2 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), t4, 0 ), t6, 1 );
			__m256i r3 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), t5, 0 ), t7, 1 );
#endif

			r0 = _mm256_shuffle_epi8( r0, vector256_uint8_unpack_hilo );
			r1 = _mm256_shuffle_epi8( r1, vector256_uint8_unpack_hilo );
			r2 = _mm256_shuffle_epi8( r2, vector256_uint8_unpack_hilo );
			r3 = _mm256_shuffle_epi8( r3, vector256_uint8_unpack_hilo );

			__m256i fx = _mm256_and_si256( sx, vector256_int16_127 );
			__m256i fxb = _mm256_packus_epi16( fx, fx );
			__m256i fxw = _mm256_unpacklo_epi8( fxb, fxb );
			__m256i fxl = _mm256_xor_si256( _mm256_unpacklo_epi16( fxw, fxw ), vector256_int16_127 );
			__m256i fxh = _mm256_xor_si256( _mm256_unpackhi_epi16( fxw, fxw ), vector256_int16_127 );

			r0 = _mm256_srai_epi16( _mm256_maddubs_epi16( r0, _mm256_shuffle_epi32( fxl, _MM_SHUFFLE( 1, 1, 0, 0 ) ) ), STP );
			r1 = _mm256_srai_epi16( _mm256_maddubs_epi16( r1, _mm256_shuffle_epi32( fxl, _MM_SHUFFLE( 3, 3, 2, 2 ) ) ), STP );
			r2 = _mm256_srai_epi16( _mm256_maddubs_epi16( r2, _mm256_shuffle_epi32( fxh, _MM_SHUFFLE( 1, 1, 0, 0 ) ) ), STP );
			r3 = _mm256_srai_epi16( _mm256_maddubs_epi16( r3, _mm256_shuffle_epi32( fxh, _MM_SHUFFLE( 3, 3, 2, 2 ) ) ), STP );

			r0 = _mm256_packus_epi16( r0, r1 );
			r2 = _mm256_packus_epi16( r2, r3 );
 
			_mm256_stream_si256( (__m256i *)( destRow + x + 0 ), r0 );
			_mm256_stream_si256( (__m256i *)( destRow + x + 8 ), r2 );

			sx = _mm256_add_epi16( sx, dx );
			sy = _mm256_add_epi16( sy, dy );
		}

#elif defined( __USE_SSE4__ )

		// This version uses PMADDUBSW which unfortunately multiplies an unsigned byte with a *signed* byte.
		// As a result, any fraction over 127 will be interpreted as a negative fraction. To keep the
		// fractions positive, the sub-texel precision is reduced to just 7 bits and there is a 1/128 loss
		// in brightness when interpolating horizontally because the fraction 1.0 (128) cannot be used.

		__m128i sx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcX8 ), 0 ), 0 );
		__m128i sy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcY8 ), 0 ), 0 );
		__m128i dx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaX8 ), 0 ), 0 );
		__m128i dy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaY8 ), 0 ), 0 );
		__m128i pitch = _mm_unpacklo_epi16( _mm_shufflelo_epi16( _mm_cvtsi32_si128( srcPitchInTexels ), 0 ), vector_int16_1 );

		sx = _mm_add_epi16( sx, _mm_mullo_epi16( dx, vector_int16_01234567 ) );
		sy = _mm_add_epi16( sy, _mm_mullo_epi16( dy, vector_int16_01234567 ) );
		dx = _mm_slli_epi16( dx, 3 );
		dy = _mm_slli_epi16( dy, 3 );

		for ( int x = 0; x < 32; x += 8 )
		{
			__m128i ax = _mm_srai_epi16( sx, STP );
			__m128i ay = _mm_srai_epi16( sy, STP );
			__m128i of0 = _mm_madd_epi16( _mm_unpacklo_epi16( ay, ax ), pitch );
			__m128i of1 = _mm_madd_epi16( _mm_unpackhi_epi16( ay, ax ), pitch );

			const unsigned int a0 = _mm_extract_epi32( of0, 0 );
			const unsigned int a1 = _mm_extract_epi32( of0, 1 );
			const unsigned int a2 = _mm_extract_epi32( of0, 2 );
			const unsigned int a3 = _mm_extract_epi32( of0, 3 );
			const unsigned int a4 = _mm_extract_epi32( of1, 0 );
			const unsigned int a5 = _mm_extract_epi32( of1, 1 );
			const unsigned int a6 = _mm_extract_epi32( of1, 2 );
			const unsigned int a7 = _mm_extract_epi32( of1, 3 );

			__m128i r0 = _mm_loadh_epi64( _mm_loadl_epi64( (const __m128i *)(localSrc + a0) ), (const __m128i *)(localSrc + a1) );
			__m128i r1 = _mm_loadh_epi64( _mm_loadl_epi64( (const __m128i *)(localSrc + a2) ), (const __m128i *)(localSrc + a3) );
			__m128i r2 = _mm_loadh_epi64( _mm_loadl_epi64( (const __m128i *)(localSrc + a4) ), (const __m128i *)(localSrc + a5) );
			__m128i r3 = _mm_loadh_epi64( _mm_loadl_epi64( (const __m128i *)(localSrc + a6) ), (const __m128i *)(localSrc + a7) );

			r0 = _mm_shuffle_epi8( r0, vector_uint8_unpack_hilo );
			r1 = _mm_shuffle_epi8( r1, vector_uint8_unpack_hilo );
			r2 = _mm_shuffle_epi8( r2, vector_uint8_unpack_hilo );
			r3 = _mm_shuffle_epi8( r3, vector_uint8_unpack_hilo );

			__m128i fx = _mm_and_si128( sx, vector_int16_127 );
			__m128i fxb = _mm_packus_epi16( fx, fx );
			__m128i fxw = _mm_unpacklo_epi8( fxb, fxb );
			__m128i fxl = _mm_xor_si128( _mm_unpacklo_epi16( fxw, fxw ), vector_int16_127 );
			__m128i fxh = _mm_xor_si128( _mm_unpackhi_epi16( fxw, fxw ), vector_int16_127 );

			r0 = _mm_srai_epi16( _mm_maddubs_epi16( r0, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 1, 1, 0, 0 ) ) ), STP );
			r1 = _mm_srai_epi16( _mm_maddubs_epi16( r1, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 3, 3, 2, 2 ) ) ), STP );
			r2 = _mm_srai_epi16( _mm_maddubs_epi16( r2, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 1, 1, 0, 0 ) ) ), STP );
			r3 = _mm_srai_epi16( _mm_maddubs_epi16( r3, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 3, 3, 2, 2 ) ) ), STP );

			r0 = _mm_packus_epi16( r0, r1 );
			r2 = _mm_packus_epi16( r2, r3 );
 
			_mm_stream_si128( (__m128i *)( destRow + x + 0 ), r0 );
			_mm_stream_si128( (__m128i *)( destRow + x + 4 ), r2 );

			sx = _mm_add_epi16( sx, dx );
			sy = _mm_add_epi16( sy, dy );
		}

#elif defined( __USE_SSE2__ )

		__m128i sx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcX8 ), 0 ), 0 );
		__m128i sy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcY8 ), 0 ), 0 );
		__m128i dx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaX8 ), 0 ), 0 );
		__m128i dy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaY8 ), 0 ), 0 );
		__m128i pitch = _mm_unpacklo_epi16( _mm_shufflelo_epi16( _mm_cvtsi32_si128( srcPitchInTexels ), 0 ), vector_int16_1 );

		sx = _mm_add_epi16( sx, _mm_mullo_epi16( dx, vector_int16_01234567 ) );
		sy = _mm_add_epi16( sy, _mm_mullo_epi16( dy, vector_int16_01234567 ) );
		dx = _mm_slli_epi16( dx, 3 );
		dy = _mm_slli_epi16( dy, 3 );

		for ( int x = 0; x < 32; x += 8 )
		{
			__m128i ax = _mm_srai_epi16( sx, STP );
			__m128i ay = _mm_srai_epi16( sy, STP );
			__m128i of0 = _mm_madd_epi16( _mm_unpacklo_epi16( ay, ax ), pitch );
			__m128i of1 = _mm_madd_epi16( _mm_unpackhi_epi16( ay, ax ), pitch );

			const unsigned int a0 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 0 ) );
			const unsigned int a1 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 1 ) );
			const unsigned int a2 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 2 ) );
			const unsigned int a3 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 3 ) );
			const unsigned int a4 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 0 ) );
			const unsigned int a5 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 1 ) );
			const unsigned int a6 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 2 ) );
			const unsigned int a7 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 3 ) );

			__m128i r0 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a0) ), vector_uint8_0 );
			__m128i r1 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a1) ), vector_uint8_0 );
			__m128i r2 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a2) ), vector_uint8_0 );
			__m128i r3 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a3) ), vector_uint8_0 );
			__m128i r4 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a4) ), vector_uint8_0 );
			__m128i r5 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a5) ), vector_uint8_0 );
			__m128i r6 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a6) ), vector_uint8_0 );
			__m128i r7 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a7) ), vector_uint8_0 );

			r0 = _mm_unpacklo_epi16( r0, _mm_shuffle_epi32( r0, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r1 = _mm_unpacklo_epi16( r1, _mm_shuffle_epi32( r1, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r2 = _mm_unpacklo_epi16( r2, _mm_shuffle_epi32( r2, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r3 = _mm_unpacklo_epi16( r3, _mm_shuffle_epi32( r3, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r4 = _mm_unpacklo_epi16( r4, _mm_shuffle_epi32( r4, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r5 = _mm_unpacklo_epi16( r5, _mm_shuffle_epi32( r5, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r6 = _mm_unpacklo_epi16( r6, _mm_shuffle_epi32( r6, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r7 = _mm_unpacklo_epi16( r7, _mm_shuffle_epi32( r7, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );

			__m128i fx = _mm_and_si128( sx, vector_int16_255 );
			__m128i fxl = _mm_add_epi16( _mm_xor_si128( _mm_unpacklo_epi16( fx, fx ), vector_int32_255 ), vector_int32_1 );
			__m128i fxh = _mm_add_epi16( _mm_xor_si128( _mm_unpackhi_epi16( fx, fx ), vector_int32_255 ), vector_int32_1 );

			r0 = _mm_srai_epi32( _mm_madd_epi16( r0, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), STP );
			r1 = _mm_srai_epi32( _mm_madd_epi16( r1, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), STP );
			r2 = _mm_srai_epi32( _mm_madd_epi16( r2, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), STP );
			r3 = _mm_srai_epi32( _mm_madd_epi16( r3, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), STP );
			r4 = _mm_srai_epi32( _mm_madd_epi16( r4, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), STP );
			r5 = _mm_srai_epi32( _mm_madd_epi16( r5, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), STP );
			r6 = _mm_srai_epi32( _mm_madd_epi16( r6, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), STP );
			r7 = _mm_srai_epi32( _mm_madd_epi16( r7, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), STP );

			r0 = _mm_packs_epi32( r0, r1 );
			r2 = _mm_packs_epi32( r2, r3 );
			r4 = _mm_packs_epi32( r4, r5 );
			r6 = _mm_packs_epi32( r6, r7 );

			r0 = _mm_packus_epi16( r0, r2 );
			r4 = _mm_packus_epi16( r4, r6 );

			_mm_stream_si128( (__m128i *)( destRow + x + 0 ), r0 );
			_mm_stream_si128( (__m128i *)( destRow + x + 4 ), r4 );

			sx = _mm_add_epi16( sx, dx );
			sy = _mm_add_epi16( sy, dy );
		}

#elif defined( __ARM_NEON__ )		// increased throughput

		__asm__ volatile(
			"	mov			r2, #4									\n\t"	// pixel pitch

			"	movw		r0, #0x0100								\n\t"	// r0 = 0x00000100
			"	movt		r0, #0x0302								\n\t"	// r0 = 0x03020100
			"	movw		r1, #0x0504								\n\t"	// r1 = 0x00000504
			"	movt		r1, #0x0706								\n\t"	// r1 = 0x07060504
			"	vmov.u32	d8[0], r0								\n\t"	// d8 = 0x0706050403020100
			"	vmov.u32	d8[1], r1								\n\t"	// d8 = 0x0706050403020100
			"	vmov.u32	d9, d8									\n\t"	// d9 = 0x0706050403020100

			"	vdup.u8		d1, %[sx]								\n\t"	//      srcX,      srcX,      srcX,      srcX,      srcX,      srcX,      srcX,      srcX
			"	vmov.u8		d0, #0xFF								\n\t"	//      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF
			"	veor.u8		d0, d0, d1								\n\t"	//   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1
			"	vdup.u8		d3, %[dx]								\n\t"	//    deltaX,    deltaX,    deltaX,    deltaX,    deltaX,    deltaX,    deltaX,    deltaX
			"	vmov.u8		d2, #0x00								\n\t"	//      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00
			"	vsub.u8		d2, d2, d3								\n\t"	//   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX
			"	vmla.u8		q0, q1, q4								\n\t"
			"	vshl.u8		q1, q1, #3								\n\t"

			"	uxth		%[sx], %[sx]							\n\t"
			"	orr			%[sx], %[sx], %[sy], lsl #16			\n\t"
			"	uxth		%[dx], %[dx]							\n\t"
			"	orr			%[dx], %[dx], %[dy], lsl #16			\n\t"

			"	add			r3, %[d], #128							\n\t"	// &destRow[32]

			".LOOP_LINEAR_PACKED_RGB_1:								\n\t"
			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[0],d15[0],d16[0]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[0],d18[0],d19[0]}, [r0], r2		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[1],d15[1],d16[1]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[1],d18[1],d19[1]}, [r0], r2		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[2],d15[2],d16[2]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[2],d18[2],d19[2]}, [r0], r2		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[3],d15[3],d16[3]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[3],d18[3],d19[3]}, [r0], r2		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[4],d15[4],d16[4]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[4],d18[4],d19[4]}, [r0], r2		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[5],d15[5],d16[5]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[5],d18[5],d19[5]}, [r0], r2		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[6],d15[6],d16[6]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[6],d18[6],d19[6]}, [r0], r2		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[7],d15[7],d16[7]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[7],d18[7],d19[7]}, [r0], r2		\n\t"

			"	vmull.u8	 q4, d14, d0							\n\t"	// weight left  red   with (fracX^-1)
			"	vmull.u8	 q5, d15, d0							\n\t"	// weight left  green with (fracX^-1)
			"	vmull.u8	 q6, d16, d0							\n\t"	// weight left  blue  with (fracX^-1)

			"	vmlal.u8	 q4, d17, d1							\n\t"	// weight right red   with (fracX) and add to top-left  red
			"	vmlal.u8	 q5, d18, d1							\n\t"	// weight right green with (fracX) and add to top-left  green
			"	vmlal.u8	 q6, d19, d1							\n\t"	// weight right blue  with (fracX) and add to top-left  blue

			"	vqrshrn.u16	 d8,  q4, #8							\n\t"	// reduce to 8 bits of precision and half the register size
			"	vqrshrn.u16	 d9,  q5, #8							\n\t"	// reduce to 8 bits of precision and half the register size
			"	vqrshrn.u16	d10,  q6, #8							\n\t"	// reduce to 8 bits of precision and half the register size
			"	vmov.u64	d11, 0									\n\t"

			"	vst4.u8		{d8[0],d9[0],d10[0],d11[0]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[1],d9[1],d10[1],d11[1]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[2],d9[2],d10[2],d11[2]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[3],d9[3],d10[3],d11[3]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[4],d9[4],d10[4],d11[4]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[5],d9[5],d10[5],d11[5]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[6],d9[6],d10[6],d11[6]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[7],d9[7],d10[7],d11[7]}, [%[d]], r2	\n\t"

			"	vadd.u8		q0, q0, q1								\n\t"	// update the bilinear weights (fracX^-1), (fracX)

			"	cmp			%[d], r3								\n\t"	// destRow == ( dest + y * destPitchInPixels + 32 )
			"	bne			.LOOP_LINEAR_PACKED_RGB_1				\n\t"
			:
			:	[sx] "r" (localSrcX8),
				[sy] "r" (localSrcY8),
				[dx] "r" (deltaX8),
				[dy] "r" (deltaY8),
				[s] "r" (localSrc),
				[d] "r" (destRow),
				[p] "r" (srcPitchInTexels)
			:	"r0", "r1", "r2", "r3", "r4",
				"q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12",
				"memory"
		);

#elif defined( __ARM_NEON__ )		// compressed computation

		__asm__ volatile(
			"	mov			r3, #4							\n\t"	// destination pointer increment per iteration

			"	vmov.u64	d2, #0xFFFFFFFF					\n\t"	//      0xFF,      0xFF,      0xFF,      0xFF,    0x00,    0x00,    0x00,    0x00
			"	vdup.u8		d0, %[sx]						\n\t"	//      srcX,      srcX,      srcX,      srcX,    srcX,    srcX,    srcX,    srcX
			"	veor.u8		d0, d0, d2						\n\t"	//   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,    srcX,    srcX,    srcX,    srcX
			"	vdup.u8		d1, %[dx]						\n\t"	//    deltaX,    deltaX,    deltaX,    deltaX,  deltaX,  deltaX,  deltaX,  deltaX
			"	veor.u8		d1, d1, d2						\n\t"	// deltaX^-1, deltaX^-1, deltaX^-1, deltaX^-1,  deltaX,  deltaX,  deltaX,  deltaX
			"	vsub.u8		d1, d1, d2						\n\t"	//   -deltaX,   -deltaX,   -deltaX,   -deltaX,  deltaX,  deltaX,  deltaX,  deltaX

			"	uxth		%[sx], %[sx]					\n\t"
			"	orr			%[sx], %[sx], %[sy], lsl #16	\n\t"
			"	uxth		%[dx], %[dx]					\n\t"
			"	orr			%[dx], %[dx], %[dy], lsl #16	\n\t"

			"	mov			r2, #32							\n\t"

			".LOOP_LINEAR_PACKED_RGB_2:						\n\t"
			"	sxtb		r0, %[sx], ror #8				\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24				\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0				\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2			\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]				\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld1.u8		d2, [r0]						\n\t"	// localSrc[( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4][0-7];
			"	vmull.u8	q1, d2, d0						\n\t"	// weight pixels with (fracX^-1) and (fracX)
			"	vadd.u16	d2, d2, d3						\n\t"	// add the two pixels
			"	vqrshrn.u16	d2, q1, #8						\n\t"	// reduce to 8 bits of precision and half the register size
			"	vadd.u8		d0, d0, d1						\n\t"	// update the bilinear weights
			"	vst1.u32	d2[0], [%[d]], r3				\n\r"	// store destination pixel, destRow++;

			"	subs		r2, r2, #1						\n\t"
			"	bne			.LOOP_LINEAR_PACKED_RGB_2		\n\t"
			:
			:	[sx] "r" (localSrcX8),
				[sy] "r" (localSrcY8),
				[dx] "r" (deltaX8),
				[dy] "r" (deltaY8),
				[s] "r" (localSrc),
				[d] "r" (destRow),
				[p] "r" (srcPitchInTexels)
			:	"r0", "r1", "r2", "r3", "r4",
				"d0", "d1", "d2", "d3",
				"q0", "q1",
				"memory"
		);

#elif defined( __HEXAGON_V50__ )	// 2 pixels per iteration appears to be optimal

		Word32 dxy1 = Q6_R_combine_RlRl( deltaY8, deltaX8 );
		Word64 dxy2 = Q6_P_vaslh_PI( Q6_P_combine_RR( dxy1, dxy1 ), 1 );

		Word32 sxy00 = Q6_R_combine_RlRl( localSrcY8, localSrcX8 );
		Word64 sxy01 = Q6_P_combine_RR( Q6_R_vaddh_RR( sxy00, dxy1 ), sxy00 );
	
		Word32 pch0 = Q6_R_combine_RlRl( srcPitchInTexels, Q6_R_equals_I( 1 ) );
		Word64 pch2 = Q6_P_combine_RR( pch0, pch0 );

		Word64 dfx2 = Q6_P_shuffeh_PP( dxy2, dxy2 );
		Word64 dfrx2 = Q6_P_shuffeb_PP( dfx2, dfx2 );
		Word64 dflx2 = Q6_P_vsubb_PP( Q6_P_combine_II( 0, 0 ), dfrx2 );

		Word64 cfx01 = Q6_P_shuffeh_PP( sxy01, sxy01 );
		Word64 cfrx01 = Q6_P_shuffeb_PP( cfx01, cfx01 );
		Word64 cflx01 = Q6_P_xor_PP( Q6_P_combine_II( -1, -1 ), cfrx01 );

		for ( int x = 0; x < 32; x += 2 )
		{
			Word64 offset0 = Q6_P_vdmpy_PP_sat( Q6_P_vasrh_PI( sxy01, STP ), pch2 );

			Word64 r0 = Q6_P_vmpybu_RR( localSrc[Q6_R_extract_Pl( offset0 ) + 0], Q6_R_extract_Pl( cflx01 ) );
			Word64 r1 = Q6_P_vmpybu_RR( localSrc[Q6_R_extract_Ph( offset0 ) + 0], Q6_R_extract_Ph( cflx01 ) );

			r0 = Q6_P_vmpybuacc_RR( r0, localSrc[Q6_R_extract_Pl( offset0 ) + 1], Q6_R_extract_Pl( cfrx01 ) );
			r1 = Q6_P_vmpybuacc_RR( r1, localSrc[Q6_R_extract_Ph( offset0 ) + 1], Q6_R_extract_Ph( cfrx01 ) );

			*(Word64 *)&destRow[x + 0] = Q6_P_combine_RR( Q6_R_vtrunohb_P( r1 ), Q6_R_vtrunohb_P( r0 ) );

			sxy01 = Q6_P_vaddh_PP( sxy01, dxy2 );

			cflx01 = Q6_P_vaddub_PP( cflx01, dflx2 );
			cfrx01 = Q6_P_vaddub_PP( cfrx01, dfrx2 );
		}

#else

		for ( int x = 0; x < 32; x++ )
		{
			const int sampleX = localSrcX8 >> STP;
			const int sampleY = localSrcY8 >> STP;

			const unsigned int * texel = localSrc + sampleY * srcPitchInTexels + sampleX;

			const unsigned int s0 = *( texel + 0 );
			const unsigned int s1 = *( texel + 1 );

			int r0 = ( s0 >> 0 ) & 0xFF;
			int r1 = ( s1 >> 0 ) & 0xFF;

			int g0 = ( s0 >> 8 ) & 0xFF;
			int g1 = ( s1 >> 8 ) & 0xFF;

			int b0 = ( s0 >> 16 ) & 0xFF;
			int b1 = ( s1 >> 16 ) & 0xFF;

			const int fracX1 = localSrcX8 & ( ( 1 << STP ) - 1 );
			const int fracX0 = ( 1 << STP ) - fracX1;

			r0 = fracX0 * r0 + fracX1 * r1;
			g0 = fracX0 * g0 + fracX1 * g1;
			b0 = fracX0 * b0 + fracX1 * b1;

			*destRow++ =	( ( r0 & 0x0000FF00 ) >> 8 ) |
							( ( g0 & 0x0000FF00 ) >> 0 ) |
							( ( b0 & 0x0000FF00 ) << 8 );

			localSrcX8 += deltaX8;
			localSrcY8 += deltaY8;
		}

#endif

		scanLeftSrcX  += scanLeftDeltaX;
		scanLeftSrcY  += scanLeftDeltaY;
		scanRightSrcX += scanRightDeltaX;
		scanRightSrcY += scanRightDeltaY;
	}

	//FlushCacheBox( dest, 32 * 4, 32, destPitchInPixels * 4 );
}

void Warp32x32_SampleBilinearPackedRGB(
		const unsigned char * const	src,
		const int					srcPitchInTexels,
		const int					srcTexelsWide,
		const int					srcTexelsHigh,
		unsigned char * const		dest,
		const int					destPitchInPixels,
		const MeshCoord_t *			meshCoords,
		const int					meshStride )
{
	// The source texture needs to be sampled with the texture coordinate at the center of each destination pixel.
	// In other words, the texture coordinates are offset by 1/64 of the texture space spanned by the 32x32 destination quad.

	const int L32 = 5;	// log2( 32 )
	const int SCP = 16;	// scan-conversion precision
#if defined( __USE_SSE4__ ) || defined( __USE_AVX2__ )
	const int STP = 7;	// sub-texel precision
#else
	const int STP = 8;	// sub-texel precision
#endif

	//ZeroCacheBox( dest, 32 * 4, 32, destPitchInPixels * 4 );

	// Clamping the corners may distort quads that sample close to the edges, but that should not be noticable because these quads
	// are close to the far peripheral vision, where the human eye is weak when it comes to distinguishing color and shape.
	int clampedCorners[4][2];
	for ( int i = 0; i < 4; i++ )
	{
		clampedCorners[i][0] = ClampInt( (int)( meshCoords[( i >> 1 ) * meshStride + ( i & 1 )].x * ( srcTexelsWide << SCP ) ), 0, ( srcTexelsWide - 2 ) << SCP );
		clampedCorners[i][1] = ClampInt( (int)( meshCoords[( i >> 1 ) * meshStride + ( i & 1 )].y * ( srcTexelsHigh << SCP ) ), 0, ( srcTexelsHigh - 2 ) << SCP );
	}

	// calculate the axis-aligned bounding box of source texture space that may be sampled
	const int minSrcX = ( MinInt4( clampedCorners[0][0], clampedCorners[1][0], clampedCorners[2][0], clampedCorners[3][0] ) >> SCP ) + 0;
	const int maxSrcX = ( MaxInt4( clampedCorners[0][0], clampedCorners[1][0], clampedCorners[2][0], clampedCorners[3][0] ) >> SCP ) + 1;
	const int minSrcY = ( MinInt4( clampedCorners[0][1], clampedCorners[1][1], clampedCorners[2][1], clampedCorners[3][1] ) >> SCP ) + 0;
	const int maxSrcY = ( MaxInt4( clampedCorners[0][1], clampedCorners[1][1], clampedCorners[2][1], clampedCorners[3][1] ) >> SCP ) + 1;

	// prefetch all source texture data that is possibly sampled
	PrefetchBox( src + ( minSrcY * srcPitchInTexels + minSrcX ) * 4, ( maxSrcX - minSrcX ) * 4, ( maxSrcY - minSrcY ), srcPitchInTexels * 4 );

	// vertical deltas in 16.16 fixed point
	const int scanLeftDeltaX  = ( clampedCorners[2][0] - clampedCorners[0][0] ) >> L32;
	const int scanLeftDeltaY  = ( clampedCorners[2][1] - clampedCorners[0][1] ) >> L32;
	const int scanRightDeltaX = ( clampedCorners[3][0] - clampedCorners[1][0] ) >> L32;
	const int scanRightDeltaY = ( clampedCorners[3][1] - clampedCorners[1][1] ) >> L32;

	// scan-line texture coordinates in 16.16 fixed point with half-pixel vertical offset
	int scanLeftSrcX  = clampedCorners[0][0] + ( ( clampedCorners[2][0] - clampedCorners[0][0] ) >> ( L32 + 1 ) );
	int scanLeftSrcY  = clampedCorners[0][1] + ( ( clampedCorners[2][1] - clampedCorners[0][1] ) >> ( L32 + 1 ) );
	int scanRightSrcX = clampedCorners[1][0] + ( ( clampedCorners[3][0] - clampedCorners[1][0] ) >> ( L32 + 1 ) );
	int scanRightSrcY = clampedCorners[1][1] + ( ( clampedCorners[3][1] - clampedCorners[1][1] ) >> ( L32 + 1 ) );

	for ( int y = 0; y < 32; y++ )
	{
		// scan-line texture coordinates in 16.16 fixed point with half-pixel horizontal offset
		const int srcX16 = scanLeftSrcX + ( ( scanRightSrcX - scanLeftSrcX ) >> ( L32 + 1 ) );
		const int srcY16 = scanLeftSrcY + ( ( scanRightSrcY - scanLeftSrcY ) >> ( L32 + 1 ) );

		// horizontal deltas in 16.16 fixed point
		const int deltaX16 = ( scanRightSrcX - scanLeftSrcX ) >> L32;
		const int deltaY16 = ( scanRightSrcY - scanLeftSrcY ) >> L32;

		// get the sign of the deltas
		const int deltaSignX = ( deltaX16 >> 31 );
		const int deltaSignY = ( deltaY16 >> 31 );

		// reduce the deltas to 16.8 fixed-point (may be negative sign extended)
		const int deltaX8 = ( ( ( ( deltaX16 ^ deltaSignX ) - deltaSignX ) >> ( SCP - STP ) ) ^ deltaSignX ) - deltaSignX;
		const int deltaY8 = ( ( ( ( deltaY16 ^ deltaSignY ) - deltaSignY ) >> ( SCP - STP ) ) ^ deltaSignY ) - deltaSignY;

		// reduce the source coordinates to 16.8 fixed-point
		const int srcX8 = srcX16 >> ( SCP - STP );
		const int srcY8 = srcY16 >> ( SCP - STP );

		// get the top-left corner of the bounding box of the texture space sampled by this scan-line
		const int srcBoundsTopLeftX = MinInt( scanLeftSrcX, scanRightSrcX ) >> SCP;
		const int srcBoundsTopLeftY = MinInt( scanLeftSrcY, scanRightSrcY ) >> SCP;

		// localize the source pointer and source coordinates to allow using 8.8 fixed point
		const unsigned int * const localSrc = (const unsigned int *)src + ( srcBoundsTopLeftY * srcPitchInTexels + srcBoundsTopLeftX );
		int localSrcX8 = srcX8 - ( srcBoundsTopLeftX << STP );
		int localSrcY8 = srcY8 - ( srcBoundsTopLeftY << STP );

		unsigned int * destRow = (unsigned int *)dest + y * destPitchInPixels;

#if defined( __USE_AVX2__ )

		// This version uses VPMADDUBSW which unfortunately multiplies an unsigned byte with a *signed* byte.
		// As a result, any fraction over 127 will be interpreted as a negative fraction. To keep the
		// fractions positive, the sub-texel precision is reduced to just 7 bits and there is a 1/128 loss
		// in brightness when interpolating vertically because the fraction 1.0 (128) cannot be used.

		__m256i sx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcX8 ) );
		__m256i sy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcY8 ) );
		__m256i dx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaX8 ) );
		__m256i dy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaY8 ) );
		__m256i pitch16 = _mm256_unpacklo_epi16( _mm256_broadcastw_epi16( _mm_cvtsi32_si128( srcPitchInTexels ) ), vector256_int16_1 );
#if 0
		__m256i pitch32 = _mm256_broadcastd_epi32( _mm_cvtsi32_si128( srcPitchInTexels ) );
#endif

		sx = _mm256_add_epi16( sx, _mm256_mullo_epi16( dx, vector256_int16_012389AB4567CDEF ) );
		sy = _mm256_add_epi16( sy, _mm256_mullo_epi16( dy, vector256_int16_012389AB4567CDEF ) );
		dx = _mm256_slli_epi16( dx, 4 );
		dy = _mm256_slli_epi16( dy, 4 );

		for ( int x = 0; x < 32; x += 16 )
		{
			__m256i ax = _mm256_srai_epi16( sx, STP );
			__m256i ay = _mm256_srai_epi16( sy, STP );
			__m256i of0 = _mm256_madd_epi16( _mm256_unpacklo_epi16( ay, ax ), pitch16 );
			__m256i of1 = _mm256_madd_epi16( _mm256_unpackhi_epi16( ay, ax ), pitch16 );

#if 0	// using a gather appears to be slower on an Intel Core i7-4960HQ
			__m256i o0 = _mm256_add_epi32( _mm256_unpacklo_epi32( of0, of0 ), vector256_int32_01010101 );
			__m256i o1 = _mm256_add_epi32( _mm256_unpackhi_epi32( of0, of0 ), vector256_int32_01010101 );
			__m256i o2 = _mm256_add_epi32( _mm256_unpacklo_epi32( of1, of1 ), vector256_int32_01010101 );
			__m256i o3 = _mm256_add_epi32( _mm256_unpackhi_epi32( of1, of1 ), vector256_int32_01010101 );
			__m256i o4 = _mm256_add_epi32( o0, pitch32 );
			__m256i o5 = _mm256_add_epi32( o1, pitch32 );
			__m256i o6 = _mm256_add_epi32( o2, pitch32 );
			__m256i o7 = _mm256_add_epi32( o3, pitch32 );

			__m256i t0 = _mm256_i32gather_epi32( (const int *)localSrc, o0, 4 );
			__m256i t1 = _mm256_i32gather_epi32( (const int *)localSrc, o1, 4 );
			__m256i t2 = _mm256_i32gather_epi32( (const int *)localSrc, o2, 4 );
			__m256i t3 = _mm256_i32gather_epi32( (const int *)localSrc, o3, 4 );
			__m256i t4 = _mm256_i32gather_epi32( (const int *)localSrc, o4, 4 );
			__m256i t5 = _mm256_i32gather_epi32( (const int *)localSrc, o5, 4 );
			__m256i t6 = _mm256_i32gather_epi32( (const int *)localSrc, o6, 4 );
			__m256i t7 = _mm256_i32gather_epi32( (const int *)localSrc, o7, 4 );

			__m256i r0 = _mm256_unpacklo_epi8( t0, t4 );
			__m256i r1 = _mm256_unpackhi_epi8( t0, t4 );
			__m256i r2 = _mm256_unpacklo_epi8( t1, t5 );
			__m256i r3 = _mm256_unpackhi_epi8( t1, t5 );
			__m256i r4 = _mm256_unpacklo_epi8( t2, t6 );
			__m256i r5 = _mm256_unpackhi_epi8( t2, t6 );
			__m256i r6 = _mm256_unpacklo_epi8( t3, t7 );
			__m256i r7 = _mm256_unpackhi_epi8( t3, t7 );
#else
			__m128i o0 = _mm256_extracti128_si256( of0, 0 );
			__m128i o1 = _mm256_extracti128_si256( of0, 1 );
			__m128i o2 = _mm256_extracti128_si256( of1, 0 );
			__m128i o3 = _mm256_extracti128_si256( of1, 1 );

			const unsigned int a0 = _mm_extract_epi32( o0, 0 );
			const unsigned int a1 = _mm_extract_epi32( o0, 1 );
			const unsigned int a2 = _mm_extract_epi32( o0, 2 );
			const unsigned int a3 = _mm_extract_epi32( o0, 3 );
			const unsigned int a4 = _mm_extract_epi32( o1, 0 );
			const unsigned int a5 = _mm_extract_epi32( o1, 1 );
			const unsigned int a6 = _mm_extract_epi32( o1, 2 );
			const unsigned int a7 = _mm_extract_epi32( o1, 3 );
			const unsigned int a8 = _mm_extract_epi32( o2, 0 );
			const unsigned int a9 = _mm_extract_epi32( o2, 1 );
			const unsigned int aA = _mm_extract_epi32( o2, 2 );
			const unsigned int aB = _mm_extract_epi32( o2, 3 );
			const unsigned int aC = _mm_extract_epi32( o3, 0 );
			const unsigned int aD = _mm_extract_epi32( o3, 1 );
			const unsigned int aE = _mm_extract_epi32( o3, 2 );
			const unsigned int aF = _mm_extract_epi32( o3, 3 );

			__m128i t0 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a0) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a0) ) );
			__m128i t1 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a1) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a1) ) );
			__m128i t2 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a2) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a2) ) );
			__m128i t3 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a3) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a3) ) );
			__m128i t4 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a4) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a4) ) );
			__m128i t5 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a5) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a5) ) );
			__m128i t6 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a6) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a6) ) );
			__m128i t7 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a7) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a7) ) );
			__m128i t8 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a8) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a8) ) );
			__m128i t9 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a9) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a9) ) );
			__m128i tA = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + aA) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + aA) ) );
			__m128i tB = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + aB) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + aB) ) );
			__m128i tC = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + aC) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + aC) ) );
			__m128i tD = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + aD) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + aD) ) );
			__m128i tE = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + aE) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + aE) ) );
			__m128i tF = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + aF) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + aF) ) );

			__m256i r0 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), t0, 0 ), t4, 1 );
			__m256i r1 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), t1, 0 ), t5, 1 );
			__m256i r2 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), t2, 0 ), t6, 1 );
			__m256i r3 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), t3, 0 ), t7, 1 );
			__m256i r4 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), t8, 0 ), tC, 1 );
			__m256i r5 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), t9, 0 ), tD, 1 );
			__m256i r6 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), tA, 0 ), tE, 1 );
			__m256i r7 = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), tB, 0 ), tF, 1 );
#endif

			__m256i fy = _mm256_and_si256( sy, vector256_int16_127 );
			__m256i fyb = _mm256_packus_epi16( fy, fy );
			__m256i fyw = _mm256_unpacklo_epi8( fyb, fyb );
			__m256i fyl = _mm256_xor_si256( _mm256_unpacklo_epi16( fyw, fyw ), vector256_int16_127 );
			__m256i fyh = _mm256_xor_si256( _mm256_unpackhi_epi16( fyw, fyw ), vector256_int16_127 );

			r0 = _mm256_shuffle_epi8( _mm256_maddubs_epi16( r0, _mm256_shuffle_epi32( fyl, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), vector256_int16_unpack_hilo );
			r1 = _mm256_shuffle_epi8( _mm256_maddubs_epi16( r1, _mm256_shuffle_epi32( fyl, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), vector256_int16_unpack_hilo );
			r2 = _mm256_shuffle_epi8( _mm256_maddubs_epi16( r2, _mm256_shuffle_epi32( fyl, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), vector256_int16_unpack_hilo );
			r3 = _mm256_shuffle_epi8( _mm256_maddubs_epi16( r3, _mm256_shuffle_epi32( fyl, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), vector256_int16_unpack_hilo );
			r4 = _mm256_shuffle_epi8( _mm256_maddubs_epi16( r4, _mm256_shuffle_epi32( fyh, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), vector256_int16_unpack_hilo );
			r5 = _mm256_shuffle_epi8( _mm256_maddubs_epi16( r5, _mm256_shuffle_epi32( fyh, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), vector256_int16_unpack_hilo );
			r6 = _mm256_shuffle_epi8( _mm256_maddubs_epi16( r6, _mm256_shuffle_epi32( fyh, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), vector256_int16_unpack_hilo );
			r7 = _mm256_shuffle_epi8( _mm256_maddubs_epi16( r7, _mm256_shuffle_epi32( fyh, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), vector256_int16_unpack_hilo );

			__m256i fx = _mm256_and_si256( sx, vector256_int16_127 );
			__m256i fxl = _mm256_add_epi16( _mm256_xor_si256( _mm256_unpacklo_epi16( fx, fx ), vector256_int32_127 ), vector256_int32_1 );
			__m256i fxh = _mm256_add_epi16( _mm256_xor_si256( _mm256_unpackhi_epi16( fx, fx ), vector256_int32_127 ), vector256_int32_1 );

			r0 = _mm256_srli_epi32( _mm256_madd_epi16( r0, _mm256_shuffle_epi32( fxl, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), 2*STP );
			r1 = _mm256_srli_epi32( _mm256_madd_epi16( r1, _mm256_shuffle_epi32( fxl, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), 2*STP );
			r2 = _mm256_srli_epi32( _mm256_madd_epi16( r2, _mm256_shuffle_epi32( fxl, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), 2*STP );
			r3 = _mm256_srli_epi32( _mm256_madd_epi16( r3, _mm256_shuffle_epi32( fxl, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), 2*STP );
			r4 = _mm256_srli_epi32( _mm256_madd_epi16( r4, _mm256_shuffle_epi32( fxh, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), 2*STP );
			r5 = _mm256_srli_epi32( _mm256_madd_epi16( r5, _mm256_shuffle_epi32( fxh, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), 2*STP );
			r6 = _mm256_srli_epi32( _mm256_madd_epi16( r6, _mm256_shuffle_epi32( fxh, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), 2*STP );
			r7 = _mm256_srli_epi32( _mm256_madd_epi16( r7, _mm256_shuffle_epi32( fxh, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), 2*STP );

			r0 = _mm256_packs_epi32( r0, r1 );
			r2 = _mm256_packs_epi32( r2, r3 );
			r4 = _mm256_packs_epi32( r4, r5 );
			r6 = _mm256_packs_epi32( r6, r7 );

			r0 = _mm256_packus_epi16( r0, r2 );
			r4 = _mm256_packus_epi16( r4, r6 );

			_mm256_stream_si256( (__m256i *)( destRow + x + 0 ), r0 );
			_mm256_stream_si256( (__m256i *)( destRow + x + 8 ), r4 );

			sx = _mm256_add_epi16( sx, dx );
			sy = _mm256_add_epi16( sy, dy );
		}

#elif defined( __USE_SSE4__ )

		// This version uses PMADDUBSW which unfortunately multiplies an unsigned byte with a *signed* byte.
		// As a result, any fraction over 127 will be interpreted as a negative fraction. To keep the
		// fractions positive, the sub-texel precision is reduced to just 7 bits and there is a 1/128 loss
		// in brightness when interpolating vertically because the fraction 1.0 (128) cannot be used.

		__m128i sx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcX8 ), 0 ), 0 );
		__m128i sy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcY8 ), 0 ), 0 );
		__m128i dx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaX8 ), 0 ), 0 );
		__m128i dy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaY8 ), 0 ), 0 );
		__m128i pitch = _mm_unpacklo_epi16( _mm_shufflelo_epi16( _mm_cvtsi32_si128( srcPitchInTexels ), 0 ), vector_int16_1 );

		sx = _mm_add_epi16( sx, _mm_mullo_epi16( dx, vector_int16_01234567 ) );
		sy = _mm_add_epi16( sy, _mm_mullo_epi16( dy, vector_int16_01234567 ) );
		dx = _mm_slli_epi16( dx, 3 );
		dy = _mm_slli_epi16( dy, 3 );

		for ( int x = 0; x < 32; x += 8 )
		{
			__m128i ax = _mm_srai_epi16( sx, STP );
			__m128i ay = _mm_srai_epi16( sy, STP );
			__m128i of0 = _mm_madd_epi16( _mm_unpacklo_epi16( ay, ax ), pitch );
			__m128i of1 = _mm_madd_epi16( _mm_unpackhi_epi16( ay, ax ), pitch );

			const unsigned int a0 = _mm_extract_epi32( of0, 0 );
			const unsigned int a1 = _mm_extract_epi32( of0, 1 );
			const unsigned int a2 = _mm_extract_epi32( of0, 2 );
			const unsigned int a3 = _mm_extract_epi32( of0, 3 );
			const unsigned int a4 = _mm_extract_epi32( of1, 0 );
			const unsigned int a5 = _mm_extract_epi32( of1, 1 );
			const unsigned int a6 = _mm_extract_epi32( of1, 2 );
			const unsigned int a7 = _mm_extract_epi32( of1, 3 );

			__m128i r0 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a0) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a0) ) );
			__m128i r1 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a1) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a1) ) );
			__m128i r2 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a2) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a2) ) );
			__m128i r3 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a3) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a3) ) );
			__m128i r4 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a4) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a4) ) );
			__m128i r5 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a5) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a5) ) );
			__m128i r6 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a6) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a6) ) );
			__m128i r7 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a7) ), _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a7) ) );

			__m128i fy = _mm_and_si128( sy, vector_int16_127 );
			__m128i fyb = _mm_packus_epi16( fy, fy );
			__m128i fyw = _mm_unpacklo_epi8( fyb, fyb );
			__m128i fyl = _mm_xor_si128( _mm_unpacklo_epi16( fyw, fyw ), vector_int16_127 );
			__m128i fyh = _mm_xor_si128( _mm_unpackhi_epi16( fyw, fyw ), vector_int16_127 );

			r0 = _mm_shuffle_epi8( _mm_maddubs_epi16( r0, _mm_shuffle_epi32( fyl, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), vector_int16_unpack_hilo );
			r1 = _mm_shuffle_epi8( _mm_maddubs_epi16( r1, _mm_shuffle_epi32( fyl, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), vector_int16_unpack_hilo );
			r2 = _mm_shuffle_epi8( _mm_maddubs_epi16( r2, _mm_shuffle_epi32( fyl, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), vector_int16_unpack_hilo );
			r3 = _mm_shuffle_epi8( _mm_maddubs_epi16( r3, _mm_shuffle_epi32( fyl, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), vector_int16_unpack_hilo );
			r4 = _mm_shuffle_epi8( _mm_maddubs_epi16( r4, _mm_shuffle_epi32( fyh, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), vector_int16_unpack_hilo );
			r5 = _mm_shuffle_epi8( _mm_maddubs_epi16( r5, _mm_shuffle_epi32( fyh, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), vector_int16_unpack_hilo );
			r6 = _mm_shuffle_epi8( _mm_maddubs_epi16( r6, _mm_shuffle_epi32( fyh, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), vector_int16_unpack_hilo );
			r7 = _mm_shuffle_epi8( _mm_maddubs_epi16( r7, _mm_shuffle_epi32( fyh, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), vector_int16_unpack_hilo );

			__m128i fx = _mm_and_si128( sx, vector_int16_127 );
			__m128i fxl = _mm_add_epi16( _mm_xor_si128( _mm_unpacklo_epi16( fx, fx ), vector_int32_127 ), vector_int32_1 );
			__m128i fxh = _mm_add_epi16( _mm_xor_si128( _mm_unpackhi_epi16( fx, fx ), vector_int32_127 ), vector_int32_1 );

			r0 = _mm_srli_epi32( _mm_madd_epi16( r0, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), 2*STP );
			r1 = _mm_srli_epi32( _mm_madd_epi16( r1, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), 2*STP );
			r2 = _mm_srli_epi32( _mm_madd_epi16( r2, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), 2*STP );
			r3 = _mm_srli_epi32( _mm_madd_epi16( r3, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), 2*STP );
			r4 = _mm_srli_epi32( _mm_madd_epi16( r4, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), 2*STP );
			r5 = _mm_srli_epi32( _mm_madd_epi16( r5, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), 2*STP );
			r6 = _mm_srli_epi32( _mm_madd_epi16( r6, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), 2*STP );
			r7 = _mm_srli_epi32( _mm_madd_epi16( r7, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), 2*STP );

			r0 = _mm_packs_epi32( r0, r1 );
			r2 = _mm_packs_epi32( r2, r3 );
			r4 = _mm_packs_epi32( r4, r5 );
			r6 = _mm_packs_epi32( r6, r7 );

			r0 = _mm_packus_epi16( r0, r2 );
			r4 = _mm_packus_epi16( r4, r6 );

			_mm_stream_si128( (__m128i *)( destRow + x + 0 ), r0 );
			_mm_stream_si128( (__m128i *)( destRow + x + 4 ), r4 );

			sx = _mm_add_epi16( sx, dx );
			sy = _mm_add_epi16( sy, dy );
		}

#elif defined( __USE_SSE2__ )

		__m128i sx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcX8 ), 0 ), 0 );
		__m128i sy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcY8 ), 0 ), 0 );
		__m128i dx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaX8 ), 0 ), 0 );
		__m128i dy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaY8 ), 0 ), 0 );
		__m128i pitch = _mm_unpacklo_epi16( _mm_shufflelo_epi16( _mm_cvtsi32_si128( srcPitchInTexels ), 0 ), vector_int16_1 );

		sx = _mm_add_epi16( sx, _mm_mullo_epi16( dx, vector_int16_01234567 ) );
		sy = _mm_add_epi16( sy, _mm_mullo_epi16( dy, vector_int16_01234567 ) );
		dx = _mm_slli_epi16( dx, 3 );
		dy = _mm_slli_epi16( dy, 3 );

		for ( int x = 0; x < 32; x += 8 )
		{
			__m128i ax = _mm_srai_epi16( sx, STP );
			__m128i ay = _mm_srai_epi16( sy, STP );
			__m128i of0 = _mm_madd_epi16( _mm_unpacklo_epi16( ay, ax ), pitch );
			__m128i of1 = _mm_madd_epi16( _mm_unpackhi_epi16( ay, ax ), pitch );

			const unsigned int a0 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 0 ) );
			const unsigned int a1 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 1 ) );
			const unsigned int a2 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 2 ) );
			const unsigned int a3 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 3 ) );
			const unsigned int a4 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 0 ) );
			const unsigned int a5 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 1 ) );
			const unsigned int a6 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 2 ) );
			const unsigned int a7 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 3 ) );

			__m128i r0 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a0) ), vector_uint8_0 );
			__m128i r1 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a1) ), vector_uint8_0 );
			__m128i r2 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a2) ), vector_uint8_0 );
			__m128i r3 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a3) ), vector_uint8_0 );
			__m128i r4 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a4) ), vector_uint8_0 );
			__m128i r5 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a5) ), vector_uint8_0 );
			__m128i r6 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a6) ), vector_uint8_0 );
			__m128i r7 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + a7) ), vector_uint8_0 );

			__m128i s0 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a0) ), vector_uint8_0 );
			__m128i s1 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a1) ), vector_uint8_0 );
			__m128i s2 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a2) ), vector_uint8_0 );
			__m128i s3 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a3) ), vector_uint8_0 );
			__m128i s4 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a4) ), vector_uint8_0 );
			__m128i s5 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a5) ), vector_uint8_0 );
			__m128i s6 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a6) ), vector_uint8_0 );
			__m128i s7 = _mm_unpacklo_epi8( _mm_loadl_epi64( (const __m128i *)(localSrc + srcPitchInTexels + a7) ), vector_uint8_0 );

			__m128i fy = _mm_and_si128( sy, vector_int16_255 );
			__m128i fyl = _mm_unpacklo_epi16( fy, fy );
			__m128i fyh = _mm_unpackhi_epi16( fy, fy );

			r0 = _mm_add_epi16( r0, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( s0, r0 ), _mm_shuffle_epi32( fyl, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), STP ) );
			r1 = _mm_add_epi16( r1, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( s1, r1 ), _mm_shuffle_epi32( fyl, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), STP ) );
			r2 = _mm_add_epi16( r2, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( s2, r2 ), _mm_shuffle_epi32( fyl, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), STP ) );
			r3 = _mm_add_epi16( r3, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( s3, r3 ), _mm_shuffle_epi32( fyl, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), STP ) );
			r4 = _mm_add_epi16( r4, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( s4, r4 ), _mm_shuffle_epi32( fyh, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), STP ) );
			r5 = _mm_add_epi16( r5, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( s5, r5 ), _mm_shuffle_epi32( fyh, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), STP ) );
			r6 = _mm_add_epi16( r6, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( s6, r6 ), _mm_shuffle_epi32( fyh, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), STP ) );
			r7 = _mm_add_epi16( r7, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( s7, r7 ), _mm_shuffle_epi32( fyh, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), STP ) );

			__m128i fx = _mm_and_si128( sx, vector_int16_255 );
			__m128i fxl = _mm_add_epi16( _mm_xor_si128( _mm_unpacklo_epi16( fx, fx ), vector_int32_255 ), vector_int32_1 );
			__m128i fxh = _mm_add_epi16( _mm_xor_si128( _mm_unpackhi_epi16( fx, fx ), vector_int32_255 ), vector_int32_1 );

			r0 = _mm_unpacklo_epi16( r0, _mm_shuffle_epi32( r0, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r1 = _mm_unpacklo_epi16( r1, _mm_shuffle_epi32( r1, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r2 = _mm_unpacklo_epi16( r2, _mm_shuffle_epi32( r2, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r3 = _mm_unpacklo_epi16( r3, _mm_shuffle_epi32( r3, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r4 = _mm_unpacklo_epi16( r4, _mm_shuffle_epi32( r4, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r5 = _mm_unpacklo_epi16( r5, _mm_shuffle_epi32( r5, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r6 = _mm_unpacklo_epi16( r6, _mm_shuffle_epi32( r6, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			r7 = _mm_unpacklo_epi16( r7, _mm_shuffle_epi32( r7, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );

			r0 = _mm_srai_epi32( _mm_madd_epi16( r0, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), STP );
			r1 = _mm_srai_epi32( _mm_madd_epi16( r1, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), STP );
			r2 = _mm_srai_epi32( _mm_madd_epi16( r2, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), STP );
			r3 = _mm_srai_epi32( _mm_madd_epi16( r3, _mm_shuffle_epi32( fxl, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), STP );
			r4 = _mm_srai_epi32( _mm_madd_epi16( r4, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ), STP );
			r5 = _mm_srai_epi32( _mm_madd_epi16( r5, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ), STP );
			r6 = _mm_srai_epi32( _mm_madd_epi16( r6, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), STP );
			r7 = _mm_srai_epi32( _mm_madd_epi16( r7, _mm_shuffle_epi32( fxh, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ), STP );

			r0 = _mm_packs_epi32( r0, r1 );
			r2 = _mm_packs_epi32( r2, r3 );
			r4 = _mm_packs_epi32( r4, r5 );
			r6 = _mm_packs_epi32( r6, r7 );

			r0 = _mm_packus_epi16( r0, r2 );
			r4 = _mm_packus_epi16( r4, r6 );

			_mm_stream_si128( (__m128i *)( destRow + x + 0 ), r0 );
			_mm_stream_si128( (__m128i *)( destRow + x + 4 ), r4 );

			sx = _mm_add_epi32( sx, dx );
			sy = _mm_add_epi32( sy, dy );
		}

#elif defined( __ARM_NEON__ )		// increased throughput

		__asm__ volatile(
			"	mov			r2, #4									\n\t"	// pixel pitch
			"	lsl			r3, %[p], #2							\n\t"	// srcPitchInTexels * 4
			"	sub			r3, r3, #4								\n\t"	// srcPitchInTexels * 4 - 4

			"	movw		r0, #0x0100								\n\t"	// r0 = 0x00000100
			"	movt		r0, #0x0302								\n\t"	// r0 = 0x03020100
			"	movw		r1, #0x0504								\n\t"	// r1 = 0x00000504
			"	movt		r1, #0x0706								\n\t"	// r1 = 0x07060504
			"	vmov.u32	d8[0], r0								\n\t"	// d8 = 0x0706050403020100
			"	vmov.u32	d8[1], r1								\n\t"	// d8 = 0x0706050403020100
			"	vmov.u32	d9, d8									\n\t"	// d9 = 0x0706050403020100

			"	vdup.u8		d1, %[sx]								\n\t"	//      srcX,      srcX,      srcX,      srcX,      srcX,      srcX,      srcX,      srcX
			"	vmov.u8		d0, #0xFF								\n\t"	//      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF
			"	veor.u8		d0, d0, d1								\n\t"	//   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1
			"	vdup.u8		d3, %[dx]								\n\t"	//    deltaX,    deltaX,    deltaX,    deltaX,    deltaX,    deltaX,    deltaX,    deltaX
			"	vmov.u8		d2, #0x00								\n\t"	//      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00
			"	vsub.u8		d2, d2, d3								\n\t"	//   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX
			"	vmla.u8		q0, q1, q4								\n\t"
			"	vshl.u8		q1, q1, #3								\n\t"

			"	vdup.u8		d5, %[sy]								\n\t"	//      srcY,      srcY,      srcY,      srcY,      srcY,      srcY,      srcY,      srcY
			"	vmov.u8		d4, #0xFF								\n\t"	//      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF
			"	veor.u8		d4, d4, d5								\n\t"	//   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1
			"	vdup.u8		d7, %[dy]								\n\t"	//    deltaY,    deltaY,    deltaY,    deltaY,    deltaY,    deltaY,    deltaY,    deltaY
			"	vmov.u8		d6, #0x00								\n\t"	//      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00
			"	vsub.u8		d6, d6, d7								\n\t"	//   -deltaY,   -deltaY,   -deltaY,   -deltaY,   -deltaY,   -deltaY,   -deltaY,   -deltaY
			"	vmla.u8		q2, q3, q4								\n\t"
			"	vshl.u8		q3, q3, #3								\n\t"

			"	uxth		%[sx], %[sx]							\n\t"
			"	orr			%[sx], %[sx], %[sy], lsl #16			\n\t"
			"	uxth		%[dx], %[dx]							\n\t"
			"	orr			%[dx], %[dx], %[dy], lsl #16			\n\t"

			"	add			%[sy], %[d], #128						\n\t"	// &destRow[32]

			".LOOP_BILINEAR_PACKED_RGB_1:							\n\t"
			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[0],d15[0],d16[0]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[0],d18[0],d19[0]}, [r0], r3		\n\t"
			"	vld3.u8		{d20[0],d21[0],d22[0]}, [r0], r2		\n\t"
			"	vld3.u8		{d23[0],d24[0],d25[0]}, [r0], r3		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[1],d15[1],d16[1]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[1],d18[1],d19[1]}, [r0], r3		\n\t"
			"	vld3.u8		{d20[1],d21[1],d22[1]}, [r0], r2		\n\t"
			"	vld3.u8		{d23[1],d24[1],d25[1]}, [r0], r3		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[2],d15[2],d16[2]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[2],d18[2],d19[2]}, [r0], r3		\n\t"
			"	vld3.u8		{d20[2],d21[2],d22[2]}, [r0], r2		\n\t"
			"	vld3.u8		{d23[2],d24[2],d25[2]}, [r0], r3		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[3],d15[3],d16[3]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[3],d18[3],d19[3]}, [r0], r3		\n\t"
			"	vld3.u8		{d20[3],d21[3],d22[3]}, [r0], r2		\n\t"
			"	vld3.u8		{d23[3],d24[3],d25[3]}, [r0], r3		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[4],d15[4],d16[4]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[4],d18[4],d19[4]}, [r0], r3		\n\t"
			"	vld3.u8		{d20[4],d21[4],d22[4]}, [r0], r2		\n\t"
			"	vld3.u8		{d23[4],d24[4],d25[4]}, [r0], r3		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[5],d15[5],d16[5]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[5],d18[5],d19[5]}, [r0], r3		\n\t"
			"	vld3.u8		{d20[5],d21[5],d22[5]}, [r0], r2		\n\t"
			"	vld3.u8		{d23[5],d24[5],d25[5]}, [r0], r3		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[6],d15[6],d16[6]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[6],d18[6],d19[6]}, [r0], r3		\n\t"
			"	vld3.u8		{d20[6],d21[6],d22[6]}, [r0], r2		\n\t"
			"	vld3.u8		{d23[6],d24[6],d25[6]}, [r0], r3		\n\t"

			"	sxtb		r0, %[sx], ror #8						\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24						\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0						\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2					\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]						\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld3.u8		{d14[7],d15[7],d16[7]}, [r0], r2		\n\t"
			"	vld3.u8		{d17[7],d18[7],d19[7]}, [r0], r3		\n\t"
			"	vld3.u8		{d20[7],d21[7],d22[7]}, [r0], r2		\n\t"
			"	vld3.u8		{d23[7],d24[7],d25[7]}, [r0], r3		\n\t"

			"	vmull.u8	 q4, d14, d4							\n\t"	// weight top-left  red   with (fracY^-1)
			"	vmull.u8	 q5, d15, d4							\n\t"	// weight top-left  green with (fracY^-1)
			"	vmull.u8	 q6, d16, d4							\n\t"	// weight top-left  blue  with (fracY^-1)
			"	vmull.u8	 q7, d17, d4							\n\t"	// weight top-right red   with (fracY^-1)
			"	vmull.u8	 q8, d18, d4							\n\t"	// weight top-right green with (fracY^-1)
			"	vmull.u8	 q9, d19, d4							\n\t"	// weight top-right blue  with (fracY^-1)

			"	vmlal.u8	 q4, d20, d5							\n\t"	// weight bottom-left  red   with (fracY) and add to top-left  red   
			"	vmlal.u8	 q5, d21, d5							\n\t"	// weight bottom-left  green with (fracY) and add to top-left  green 
			"	vmlal.u8	 q6, d22, d5							\n\t"	// weight bottom-left  blue  with (fracY) and add to top-left  blue  
			"	vmlal.u8	 q7, d23, d5							\n\t"	// weight bottom-right red   with (fracY) and add to top-right red   
			"	vmlal.u8	 q8, d24, d5							\n\t"	// weight bottom-right green with (fracY) and add to top-right green 
			"	vmlal.u8	 q9, d25, d5							\n\t"	// weight bottom-right blue  with (fracY) and add to top-right blue  

			"	vqrshrn.u16	 d8,  q4, #8							\n\t"	// reduce left  red   to 8 bits of precision and half the register size
			"	vqrshrn.u16 d10,  q5, #8							\n\t"	// reduce left  green to 8 bits of precision and half the register size
			"	vqrshrn.u16 d12,  q6, #8							\n\t"	// reduce left  blue  to 8 bits of precision and half the register size
			"	vqrshrn.u16	d14,  q7, #8							\n\t"	// reduce right red   to 8 bits of precision and half the register size
			"	vqrshrn.u16	d16,  q8, #8							\n\t"	// reduce right green to 8 bits of precision and half the register size
			"	vqrshrn.u16	d18,  q9, #8							\n\t"	// reduce right blue  to 8 bits of precision and half the register size

			"	vmull.u8	 q4,  d8, d0							\n\t"	// weight left red   with (fracX^-1)
			"	vmull.u8	 q5, d10, d0							\n\t"	// weight left green with (fracX^-1)
			"	vmull.u8	 q6, d12, d0							\n\t"	// weight left blue  with (fracX^-1)

			"	vmlal.u8	 q4, d14, d1							\n\t"	// weight right red   with (fracX) and add to left red
			"	vmlal.u8	 q5, d16, d1							\n\t"	// weight right green with (fracX) and add to left green
			"	vmlal.u8	 q6, d18, d1							\n\t"	// weight right blue  with (fracX) and add to left blue

			"	vqrshrn.u16	 d8,  q4, #8							\n\t"	// reduce to 8 bits of precision and half the register size
			"	vqrshrn.u16	 d9,  q5, #8							\n\t"	// reduce to 8 bits of precision and half the register size
			"	vqrshrn.u16	d10,  q6, #8							\n\t"	// reduce to 8 bits of precision and half the register size
			"	vmov.u64	d11, 0									\n\t"

			"	vst4.u8		{d8[0],d9[0],d10[0],d11[0]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[1],d9[1],d10[1],d11[1]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[2],d9[2],d10[2],d11[2]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[3],d9[3],d10[3],d11[3]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[4],d9[4],d10[4],d11[4]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[5],d9[5],d10[5],d11[5]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[6],d9[6],d10[6],d11[6]}, [%[d]], r2	\n\t"
			"	vst4.u8		{d8[7],d9[7],d10[7],d11[7]}, [%[d]], r2	\n\t"

			"	vadd.u8		q0, q0, q1								\n\t"	// update the bilinear weights (fracX^-1), (fracX)
			"	vadd.u8		q2, q2, q3								\n\t"	// update the bilinear weights (fracY^-1), (fracY)

			"	cmp			%[d], %[sy]								\n\t"	// destRow == ( dest + y * destPitchInPixels + 32 )
			"	bne			.LOOP_BILINEAR_PACKED_RGB_1				\n\t"
			:
			:	[sx] "r" (localSrcX8),
				[sy] "r" (localSrcY8),
				[dx] "r" (deltaX8),
				[dy] "r" (deltaY8),
				[s] "r" (localSrc),
				[d] "r" (destRow),
				[p] "r" (srcPitchInTexels)
			:	"r0", "r1", "r2", "r3", "r4",
				"q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12",
				"memory"
		);

#elif defined( __ARM_NEON__ )	// compressed computation

		__asm__ volatile(
			"	lsl			r2, %[p], #2						\n\t"	// srcPitchInTexels * 4
			"	mov			r3, #4								\n\t"	// destination pointer increment per iteration

			"	vdup.u8		d1, %[sx]							\n\t"	//      srcX,      srcX,      srcX,      srcX,      srcX,      srcX,      srcX,      srcX
			"	vmov.u8		d0, #0xFF							\n\t"	//      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF
			"	veor.u8		d0, d0, d1							\n\t"	//   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1
			"	vdup.u8		d3, %[dx]							\n\t"	//    deltaX,    deltaX,    deltaX,    deltaX,    deltaX,    deltaX,    deltaX,    deltaX
			"	vmov.u8		d2, #0x00							\n\t"	//      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00
			"	vsub.u8		d2, d2, d3							\n\t"	//   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX

			"	vmov.u16	q4, #0x00FF							\n\t"
			"	vand.u8		q0, q0, q4							\n\t"
			"	vand.u8		q1, q1, q4							\n\t"

			"	vdup.u8		d5, %[sy]							\n\t"	//      srcY,      srcY,      srcY,      srcY,      srcY,      srcY,      srcY,      srcY
			"	vmov.u8		d4, #0xFF							\n\t"	//      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF
			"	veor.u8		d4, d4, d5							\n\t"	//   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1
			"	vdup.u8		d7, %[dy]							\n\t"	//    deltaY,    deltaY,    deltaY,    deltaY,    deltaY,    deltaY,    deltaY,    deltaY
			"	vmov.u8		d6, #0x00							\n\t"	//      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00
			"	vsub.u8		d6, d6, d7							\n\t"	//   -deltaY,   -deltaY,   -deltaY,   -deltaY,   -deltaY,   -deltaY,   -deltaY,   -deltaY

			"	uxth		%[sx], %[sx]						\n\t"
			"	orr			%[sx], %[sx], %[sy], lsl #16		\n\t"
			"	uxth		%[dx], %[dx]						\n\t"
			"	orr			%[dx], %[dx], %[dy], lsl #16		\n\t"

			"	mov			r4, #32								\n\t"

			".LOOP_BILINEAR_PACKED_RGB_2:						\n\t"
			"	sxtb		r0, %[sx], ror #8					\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24					\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0					\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	add			r0, %[s], r0, lsl #2				\n\t"	// localSrc + ( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4;
			"	sadd16		%[sx], %[sx], %[dx]					\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	vld1.u8		d8, [r0], r2						\n\t"	// localSrc[( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4][2];
			"	vmull.u8	q4, d8, d4							\n\t"	// weight top two pixels with (fracY^-1)
			"	vld1.u8		d10, [r0]							\n\t"	// localSrc[( ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 ) ) * 4][2];
			"	vmlal.u8	q4, d10, d5							\n\t"	// weight bottom two pixels with (fracY) and add to top two pixels
			"	vshr.u16	q4, q4, #8							\n\t"	// reduce to 8 bits of precision
			"	vadd.u8		q2, q2, q3							\n\t"	// update the bilinear weights
			"	vmul.u16	d8, d8, d0							\n\t"	// weight left with (fracX^-1)
			"	vmla.u16	d8, d9, d1							\n\t"	// weight right with (fracX) and add to left
			"	vqrshrn.u16	d8, q4, #8							\n\t"	// reduce to 8 bits of precision and half the register size
			"	vadd.u8		q0, q0, q1							\n\t"	// update the bilinear weights
			"	vst1.u32	d8[0], [%[d]], r3					\n\r"	// store destination pixel, destRow++;

			"	subs		r4, r4, #1							\n\t"
			"	bne			.LOOP_BILINEAR_PACKED_RGB_2			\n\t"
			:
			:	[sx] "r" (localSrcX8),
				[sy] "r" (localSrcY8),
				[dx] "r" (deltaX8),
				[dy] "r" (deltaY8),
				[s] "r" (localSrc),
				[d] "r" (destRow),
				[p] "r" (srcPitchInTexels)
			:	"r0", "r1", "r2", "r3", "r4",
				"q0", "q1", "q2", "q3", "q4", "q5",
				"memory"
		);

#elif defined( __HEXAGON_V50__ )	// transposed, 2 pixels per iteration appears to be optimal

		Word32 dxy0 = Q6_R_combine_RlRl( deltaY8, deltaX8 );
		Word64 dxy2 = Q6_P_vaslh_PI( Q6_P_combine_RR( dxy0, dxy0 ), 1 );		// dx2, dy2, dx2, dy2

		Word32 sxy0 = Q6_R_combine_RlRl( localSrcY8, localSrcX8 );				// sx0, sy0
		Word64 sxy2 = Q6_P_combine_RR( Q6_R_vaddh_RR( sxy0, dxy0 ), sxy0 );		// sx0, sy0, sx1, sy1

		Word32 pch0 = Q6_R_combine_RlRl( srcPitchInTexels, Q6_R_equals_I( 1 ) );
		Word64 pch2 = Q6_P_combine_RR( pch0, pch0 );
		Word64 pchr = Q6_P_combine_RR( srcPitchInTexels, srcPitchInTexels );

		Word64 cfrX = Q6_P_shuffeh_PP( sxy2, sxy2 );		// sx0, sx0, sx1, sx1
		Word64 cfrY = Q6_P_shuffoh_PP( sxy2, sxy2 );		// sy0, sy0, sy1, sy1

		Word64 dfrX = Q6_P_shuffeh_PP( dxy2, dxy2 );		// dx2, dx2, dx2, dx2
		Word64 dfrY = Q6_P_shuffoh_PP( dxy2, dxy2 );		// dy2, dy2, dy2, dy2

		cfrX = Q6_P_shuffeb_PP( cfrX, cfrX );				// fx0, fx0, fx0, fx0, fx1, fx1, fx1, fx1
		cfrY = Q6_P_shuffeb_PP( cfrY, cfrY );				// fy0, fy0, fy0, fy0, fy1, fy1, fy1, fy1

		dfrX = Q6_P_shuffeb_PP( dfrX, dfrX );				// dx2, dx2, dx2, dx2, dx2, dx2, dx2, dx2
		dfrY = Q6_P_shuffeb_PP( dfrY, dfrY );				// dy2, dy2, dy2, dy2, dy2, dy2, dy2, dy2

		Word64 zero = Q6_P_combine_II(  0,  0 );			// 0x0000000000000000
		Word64 none = Q6_P_combine_II( -1, -1 );			// 0xFFFFFFFFFFFFFFFF

		Word64 xfrX = Q6_P_shuffeh_PP( zero, none );		// 0x0000FFFF0000FFFF
		Word64 xfrY = Q6_P_shuffeb_PP( zero, none );		// 0x00FF00FF00FF00FF

		cfrX = Q6_P_xor_PP( cfrX, xfrX );					// 1-fx0, 1-fx0,   fx0, fx0, 1-fx1, 1-fx1,   fx1, fx1
		cfrY = Q6_P_xor_PP( cfrY, xfrY );					// 1-fy0,   fy0, 1-fy0, fy0, 1-fy1,   fy1, 1-fy1, fy1

		dfrX = Q6_P_vsubb_PP( Q6_P_xor_PP( dfrX, xfrX ), xfrX );
		dfrY = Q6_P_vsubb_PP( Q6_P_xor_PP( dfrY, xfrY ), xfrY );

		for ( int x = 0; x < 32; x += 2 )
		{
			Word64 offset0 = Q6_P_vdmpy_PP_sat( Q6_P_vasrh_PI( sxy2, STP ), pch2 );
			Word64 offset1 = Q6_P_vaddw_PP( offset0, pchr );

			Word64 s0 = Q6_P_packhl_RR( localSrc[Q6_R_extract_Pl( offset0 ) + 1], localSrc[Q6_R_extract_Pl( offset0 ) + 0] );	// r0, g0, r1, g1, b0, a0, b1, a1
			Word64 s1 = Q6_P_packhl_RR( localSrc[Q6_R_extract_Pl( offset1 ) + 1], localSrc[Q6_R_extract_Pl( offset1 ) + 0] );	// r2, g2, r3, g3, b2, a2, b3, a3
			Word64 s2 = Q6_P_packhl_RR( localSrc[Q6_R_extract_Ph( offset0 ) + 1], localSrc[Q6_R_extract_Ph( offset0 ) + 0] );	// r0, g0, r1, g1, b0, a0, b1, a1
			Word64 s3 = Q6_P_packhl_RR( localSrc[Q6_R_extract_Ph( offset1 ) + 1], localSrc[Q6_R_extract_Ph( offset1 ) + 0] );	// r2, g2, r3, g3, b2, a2, b3, a3

			Word64 t0 = Q6_P_shuffeb_PP( s1, s0 );	// r0, r2, r1, r3, b0, b2, b1, b3
			Word64 t1 = Q6_P_shuffob_PP( s1, s0 );	// g0, g2, g1, g3, a0, a2, a1, a3
			Word64 t2 = Q6_P_shuffeb_PP( s3, s2 );	// r0, r2, r1, r3, b0, b2, b1, b3
			Word64 t3 = Q6_P_shuffob_PP( s3, s2 );	// g0, g2, g1, g3, a0, a2, a1, a3

			Word32 m0 = Q6_R_vtrunohb_P( Q6_P_vmpybu_RR( Q6_R_extract_Pl( cfrX ), Q6_R_extract_Pl( cfrY ) ) );
			Word32 m1 = Q6_R_vtrunohb_P( Q6_P_vmpybu_RR( Q6_R_extract_Ph( cfrX ), Q6_R_extract_Ph( cfrY ) ) );

			Word64 f0 = Q6_P_combine_RR( m0, m0 );
			Word64 f1 = Q6_P_combine_RR( m1, m1 );

			t0 = Q6_P_vrmpybu_PP( t0, f0 );			// r, b
			t1 = Q6_P_vrmpybu_PP( t1, f0 );			// g, a
			t2 = Q6_P_vrmpybu_PP( t2, f1 );			// r, b
			t3 = Q6_P_vrmpybu_PP( t3, f1 );			// g, a

			t0 = Q6_P_shuffeh_PP( t1, t0 );			// r, g, b, a
			t2 = Q6_P_shuffeh_PP( t3, t2 );			// r, g, b, a

			*(Word64 *)&destRow[x] = Q6_P_combine_RR( Q6_R_vtrunohb_P( t2 ), Q6_R_vtrunohb_P( t0 ) );

			sxy2 = Q6_P_vaddh_PP( sxy2, dxy2 );

			cfrX = Q6_P_vaddub_PP( cfrX, dfrX );
			cfrY = Q6_P_vaddub_PP( cfrY, dfrY );
		}

#else

		for ( int x = 0; x < 32; x++ )
		{
			const int sampleX = localSrcX8 >> STP;
			const int sampleY = localSrcY8 >> STP;

			const unsigned int * texel = localSrc + sampleY * srcPitchInTexels + sampleX;

			const unsigned int s0 = *( texel + 0 );
			const unsigned int s1 = *( texel + 1 );
			const unsigned int s2 = *( texel + srcPitchInTexels + 0 );
			const unsigned int s3 = *( texel + srcPitchInTexels + 1 );

			int r0 = ( s0 >> 0 ) & 0xFF;
			int r1 = ( s1 >> 0 ) & 0xFF;
			int r2 = ( s2 >> 0 ) & 0xFF;
			int r3 = ( s3 >> 0 ) & 0xFF;

			int g0 = ( s0 >> 8 ) & 0xFF;
			int g1 = ( s1 >> 8 ) & 0xFF;
			int g2 = ( s2 >> 8 ) & 0xFF;
			int g3 = ( s3 >> 8 ) & 0xFF;

			int b0 = ( s0 >> 16 ) & 0xFF;
			int b1 = ( s1 >> 16 ) & 0xFF;
			int b2 = ( s2 >> 16 ) & 0xFF;
			int b3 = ( s3 >> 16 ) & 0xFF;

			const int fracX1 = localSrcX8 & ( ( 1 << STP ) - 1 );
			const int fracX0 = ( 1 << STP ) - fracX1;
			const int fracY1 = localSrcY8 & ( ( 1 << STP ) - 1 );
			const int fracY0 = ( 1 << STP ) - fracY1;

			r0 = fracX0 * r0 + fracX1 * r1;
			r2 = fracX0 * r2 + fracX1 * r3;

			g0 = fracX0 * g0 + fracX1 * g1;
			g2 = fracX0 * g2 + fracX1 * g3;

			b0 = fracX0 * b0 + fracX1 * b1;
			b2 = fracX0 * b2 + fracX1 * b3;

			r0 = fracY0 * r0 + fracY1 * r2;
			g0 = fracY0 * g0 + fracY1 * g2;
			b0 = fracY0 * b0 + fracY1 * b2;

			*destRow++ =	( ( r0 & 0x00FF0000 ) >> 16 ) |
							( ( g0 & 0x00FF0000 ) >>  8 ) |
							( ( b0 & 0x00FF0000 ) >>  0 );

			localSrcX8 += deltaX8;
			localSrcY8 += deltaY8;
		}

#endif

		scanLeftSrcX  += scanLeftDeltaX;
		scanLeftSrcY  += scanLeftDeltaY;
		scanRightSrcX += scanRightDeltaX;
		scanRightSrcY += scanRightDeltaY;
	}

	//FlushCacheBox( dest, 32 * 4, 32, destPitchInPixels * 4 );
}

void Warp32x32_SampleBilinearPlanarRGB( 
		const unsigned char * const	srcRed,
		const unsigned char * const	srcGreen,
		const unsigned char * const	srcBlue,
		const int					srcPitchInTexels,
		const int					srcTexelsWide,
		const int					srcTexelsHigh,
		unsigned char * const		dest,
		const int					destPitchInPixels,
		const MeshCoord_t *			meshCoords,
		const int					meshStride )
{
	// The source texture needs to be sampled with the texture coordinate at the center of each destination pixel.
	// In other words, the texture coordinates are offset by 1/64 of the texture space spanned by the 32x32 destination quad.

	const int L32 = 5;	// log2( 32 )
	const int SCP = 16;	// scan-conversion precision
#if defined( __USE_SSE4__ ) || defined( __USE_AVX2__ )
	const int STP = 7;	// sub-texel precision
#else
	const int STP = 8;	// sub-texel precision
#endif

	//ZeroCacheBox( dest, 32 * 4, 32, destPitchInPixels * 4 );

	// Clamping the corners may distort quads that sample close to the edges, but that should not be noticable because these quads
	// are close to the far peripheral vision, where the human eye is weak when it comes to distinguishing color and shape.
	int clampedCorners[4][2];
	for ( int i = 0; i < 4; i++ )
	{
		clampedCorners[i][0] = ClampInt( (int)( meshCoords[( i >> 1 ) * meshStride + ( i & 1 )].x * ( srcTexelsWide << SCP ) ), 0, ( srcTexelsWide - 2 ) << SCP );
		clampedCorners[i][1] = ClampInt( (int)( meshCoords[( i >> 1 ) * meshStride + ( i & 1 )].y * ( srcTexelsHigh << SCP ) ), 0, ( srcTexelsHigh - 2 ) << SCP );
	}

	// calculate the axis-aligned bounding box of source texture space that may be sampled
	const int minSrcX = ( MinInt4( clampedCorners[0][0], clampedCorners[1][0], clampedCorners[2][0], clampedCorners[3][0] ) >> SCP ) + 0;
	const int maxSrcX = ( MaxInt4( clampedCorners[0][0], clampedCorners[1][0], clampedCorners[2][0], clampedCorners[3][0] ) >> SCP ) + 1;
	const int minSrcY = ( MinInt4( clampedCorners[0][1], clampedCorners[1][1], clampedCorners[2][1], clampedCorners[3][1] ) >> SCP ) + 0;
	const int maxSrcY = ( MaxInt4( clampedCorners[0][1], clampedCorners[1][1], clampedCorners[2][1], clampedCorners[3][1] ) >> SCP ) + 1;

	// prefetch all source texture data that is possibly sampled
	PrefetchBox( srcRed + ( minSrcY * srcPitchInTexels + minSrcX ), ( maxSrcX - minSrcX ), ( maxSrcY - minSrcY ), srcPitchInTexels );

	// vertical deltas in 16.16 fixed point
	const int scanLeftDeltaX  = ( clampedCorners[2][0] - clampedCorners[0][0] ) >> L32;
	const int scanLeftDeltaY  = ( clampedCorners[2][1] - clampedCorners[0][1] ) >> L32;
	const int scanRightDeltaX = ( clampedCorners[3][0] - clampedCorners[1][0] ) >> L32;
	const int scanRightDeltaY = ( clampedCorners[3][1] - clampedCorners[1][1] ) >> L32;

	// scan-line texture coordinates in 16.16 fixed point with half-pixel vertical offset
	int scanLeftSrcX  = clampedCorners[0][0] + ( ( clampedCorners[2][0] - clampedCorners[0][0] ) >> ( L32 + 1 ) );
	int scanLeftSrcY  = clampedCorners[0][1] + ( ( clampedCorners[2][1] - clampedCorners[0][1] ) >> ( L32 + 1 ) );
	int scanRightSrcX = clampedCorners[1][0] + ( ( clampedCorners[3][0] - clampedCorners[1][0] ) >> ( L32 + 1 ) );
	int scanRightSrcY = clampedCorners[1][1] + ( ( clampedCorners[3][1] - clampedCorners[1][1] ) >> ( L32 + 1 ) );

	for ( int y = 0; y < 32; y++ )
	{
		if ( y == 1 ) { PrefetchBox( srcGreen + ( minSrcY * srcPitchInTexels + minSrcX ), ( maxSrcX - minSrcX ), ( maxSrcY - minSrcY ), srcPitchInTexels ); }
		else if ( y == 2 ) { PrefetchBox( srcBlue + ( minSrcY * srcPitchInTexels + minSrcX ), ( maxSrcX - minSrcX ), ( maxSrcY - minSrcY ), srcPitchInTexels ); }

		// scan-line texture coordinates in 16.16 fixed point with half-pixel horizontal offset
		const int srcX16 = scanLeftSrcX + ( ( scanRightSrcX - scanLeftSrcX ) >> ( L32 + 1 ) );
		const int srcY16 = scanLeftSrcY + ( ( scanRightSrcY - scanLeftSrcY ) >> ( L32 + 1 ) );

		// horizontal deltas in 16.16 fixed point
		const int deltaX16 = ( scanRightSrcX - scanLeftSrcX ) >> L32;
		const int deltaY16 = ( scanRightSrcY - scanLeftSrcY ) >> L32;

		// get the sign of the deltas
		const int deltaSignX = ( deltaX16 >> 31 );
		const int deltaSignY = ( deltaY16 >> 31 );

		// reduce the deltas to 16.8 fixed-point (may be negative sign extended)
		const int deltaX8 = ( ( ( ( deltaX16 ^ deltaSignX ) - deltaSignX ) >> ( SCP - STP ) ) ^ deltaSignX ) - deltaSignX;
		const int deltaY8 = ( ( ( ( deltaY16 ^ deltaSignY ) - deltaSignY ) >> ( SCP - STP ) ) ^ deltaSignY ) - deltaSignY;

		// reduce the source coordinates to 16.8 fixed-point
		const int srcX8 = srcX16 >> ( SCP - STP );
		const int srcY8 = srcY16 >> ( SCP - STP );

		// get the top-left corner of the bounding box of the texture space sampled by this scan-line
		const int srcBoundsTopLeftX = MinInt( scanLeftSrcX, scanRightSrcX ) >> SCP;
		const int srcBoundsTopLeftY = MinInt( scanLeftSrcY, scanRightSrcY ) >> SCP;

		// localize the source pointer and source coordinates to allow using 8.8 fixed point
		const unsigned char * const localSrcRed = srcRed + ( srcBoundsTopLeftY * srcPitchInTexels + srcBoundsTopLeftX );
		const unsigned char * const localSrcGreen = srcGreen + ( srcBoundsTopLeftY * srcPitchInTexels + srcBoundsTopLeftX );
		const unsigned char * const localSrcBlue = srcBlue + ( srcBoundsTopLeftY * srcPitchInTexels + srcBoundsTopLeftX );
		int localSrcX8 = srcX8 - ( srcBoundsTopLeftX << STP );
		int localSrcY8 = srcY8 - ( srcBoundsTopLeftY << STP );

		unsigned int * destRow = (unsigned int *)dest + y * destPitchInPixels;

#if defined( __USE_AVX2__ )

		// This version uses VPMADDUBSW which unfortunately multiplies an unsigned byte with a *signed* byte.
		// As a result, any fraction over 127 will be interpreted as a negative fraction. To keep the
		// fractions positive, the sub-texel precision is reduced to just 7 bits and there is a 1/128 loss
		// in brightness when interpolating horizontally because the fraction 1.0 (128) cannot be used.

		__m256i sx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcX8 ) );
		__m256i sy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcY8 ) );
		__m256i dx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaX8 ) );
		__m256i dy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaY8 ) );
		__m256i pitch16 = _mm256_unpacklo_epi16( _mm256_broadcastw_epi16( _mm_cvtsi32_si128( srcPitchInTexels ) ), vector256_int16_1 );
		__m256i pitch32 = _mm256_broadcastd_epi32( _mm_cvtsi32_si128( srcPitchInTexels ) );

		sx = _mm256_add_epi16( sx, _mm256_mullo_epi16( dx, vector256_int16_012389AB4567CDEF ) );
		sy = _mm256_add_epi16( sy, _mm256_mullo_epi16( dy, vector256_int16_012389AB4567CDEF ) );
		dx = _mm256_slli_epi16( dx, 4 );
		dy = _mm256_slli_epi16( dy, 4 );

		for ( int x = 0; x < 32; x += 16 )
		{
			__m256i ax = _mm256_srai_epi16( sx, STP );
			__m256i ay = _mm256_srai_epi16( sy, STP );
			__m256i of0 = _mm256_madd_epi16( _mm256_unpacklo_epi16( ay, ax ), pitch16 );
			__m256i of1 = _mm256_madd_epi16( _mm256_unpackhi_epi16( ay, ax ), pitch16 );

#if 1
			__m256i of2 = _mm256_add_epi32( of0, pitch32 );
			__m256i of3 = _mm256_add_epi32( of1, pitch32 );

			__m256i rtl = _mm256_i32gather_epi32( (const int *)localSrcRed, of0, 1 );
			__m256i rth = _mm256_i32gather_epi32( (const int *)localSrcRed, of1, 1 );
			__m256i rbl = _mm256_i32gather_epi32( (const int *)localSrcRed, of2, 1 );
			__m256i rbh = _mm256_i32gather_epi32( (const int *)localSrcRed, of3, 1 );

			__m256i gtl = _mm256_i32gather_epi32( (const int *)localSrcGreen, of0, 1 );
			__m256i gth = _mm256_i32gather_epi32( (const int *)localSrcGreen, of1, 1 );
			__m256i gbl = _mm256_i32gather_epi32( (const int *)localSrcGreen, of2, 1 );
			__m256i gbh = _mm256_i32gather_epi32( (const int *)localSrcGreen, of3, 1 );

			__m256i btl = _mm256_i32gather_epi32( (const int *)localSrcBlue, of0, 1 );
			__m256i bth = _mm256_i32gather_epi32( (const int *)localSrcBlue, of1, 1 );
			__m256i bbl = _mm256_i32gather_epi32( (const int *)localSrcBlue, of2, 1 );
			__m256i bbh = _mm256_i32gather_epi32( (const int *)localSrcBlue, of3, 1 );

			__m256i rt = _mm256_pack_epi32( rtl, rth );
			__m256i rb = _mm256_pack_epi32( rbl, rbh );
			__m256i gt = _mm256_pack_epi32( gtl, gth );
			__m256i gb = _mm256_pack_epi32( gbl, gbh );
			__m256i bt = _mm256_pack_epi32( btl, bth );
			__m256i bb = _mm256_pack_epi32( bbl, bbh );
#else
			__m128i o0 = _mm256_extracti128_si256( of0, 0 );
			__m128i o1 = _mm256_extracti128_si256( of0, 1 );
			__m128i o2 = _mm256_extracti128_si256( of1, 0 );
			__m128i o3 = _mm256_extracti128_si256( of1, 1 );

			const unsigned int a0 = _mm_extract_epi32( o0, 0 );
			const unsigned int a1 = _mm_extract_epi32( o0, 1 );
			const unsigned int a2 = _mm_extract_epi32( o0, 2 );
			const unsigned int a3 = _mm_extract_epi32( o0, 3 );
			const unsigned int a4 = _mm_extract_epi32( o1, 0 );
			const unsigned int a5 = _mm_extract_epi32( o1, 1 );
			const unsigned int a6 = _mm_extract_epi32( o1, 2 );
			const unsigned int a7 = _mm_extract_epi32( o1, 3 );
			const unsigned int a8 = _mm_extract_epi32( o2, 0 );
			const unsigned int a9 = _mm_extract_epi32( o2, 1 );
			const unsigned int aA = _mm_extract_epi32( o2, 2 );
			const unsigned int aB = _mm_extract_epi32( o2, 3 );
			const unsigned int aC = _mm_extract_epi32( o3, 0 );
			const unsigned int aD = _mm_extract_epi32( o3, 1 );
			const unsigned int aE = _mm_extract_epi32( o3, 2 );
			const unsigned int aF = _mm_extract_epi32( o3, 3 );

			__m128i rtl = _mm_setzero_si128();
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[a0], 0 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[a1], 1 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[a2], 2 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[a3], 3 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[a8], 4 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[a9], 5 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[aA], 6 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[aB], 7 );

			__m128i rth = _mm_setzero_si128();
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[a4], 0 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[a5], 1 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[a6], 2 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[a7], 3 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[aC], 4 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[aD], 5 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[aE], 6 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[aF], 7 );

			__m128i rbl = _mm_setzero_si128();
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a0], 0 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a1], 1 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a2], 2 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a3], 3 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a8], 4 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a9], 5 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + aA], 6 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + aB], 7 );

			__m128i rbh = _mm_setzero_si128();
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a4], 0 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a5], 1 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a6], 2 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a7], 3 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + aC], 4 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + aD], 5 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + aE], 6 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + aF], 7 );

			__m128i gtl = _mm_setzero_si128();
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[a0], 0 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[a1], 1 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[a2], 2 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[a3], 3 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[a8], 4 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[a9], 5 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[aA], 6 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[aB], 7 );

			__m128i gth = _mm_setzero_si128();
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[a4], 0 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[a5], 1 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[a6], 2 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[a7], 3 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[aC], 4 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[aD], 5 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[aE], 6 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[aF], 7 );

			__m128i gbl = _mm_setzero_si128();
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a0], 0 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a1], 1 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a2], 2 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a3], 3 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a8], 4 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a9], 5 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + aA], 6 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + aB], 7 );

			__m128i gbh = _mm_setzero_si128();
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a4], 0 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a5], 1 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a6], 2 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a7], 3 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + aC], 4 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + aD], 5 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + aE], 6 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + aF], 7 );

			__m128i btl = _mm_setzero_si128();
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[a0], 0 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[a1], 1 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[a2], 2 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[a3], 3 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[a8], 4 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[a9], 5 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[aA], 6 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[aB], 7 );

			__m128i bth = _mm_setzero_si128();
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[a4], 0 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[a5], 1 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[a6], 2 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[a7], 3 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[aC], 4 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[aD], 5 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[aE], 6 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[aF], 7 );

			__m128i bbl = _mm_setzero_si128();
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a0], 0 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a1], 1 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a2], 2 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a3], 3 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a8], 4 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a9], 5 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + aA], 6 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + aB], 7 );

			__m128i bbh = _mm_setzero_si128();
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a4], 0 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a5], 1 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a6], 2 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a7], 3 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + aC], 4 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + aD], 5 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + aE], 6 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + aF], 7 );

			__m256i rt = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), rtl, 0 ), rth, 1 );
			__m256i rb = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), rbl, 0 ), rbh, 1 );
			__m256i gt = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), gtl, 0 ), gth, 1 );
			__m256i gb = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), gbl, 0 ), gbh, 1 );
			__m256i bt = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), btl, 0 ), bth, 1 );
			__m256i bb = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), bbl, 0 ), bbh, 1 );
#endif

			__m256i fx = _mm256_and_si256( sx, vector256_int16_127 );
			__m256i fxb = _mm256_packus_epi16( fx, fx );
			__m256i fxw = _mm256_xor_si256( _mm256_unpacklo_epi8( fxb, fxb ), vector256_int16_127 );

			rt = _mm256_srli_epi16( _mm256_maddubs_epi16( rt, fxw ), STP );
			gt = _mm256_srli_epi16( _mm256_maddubs_epi16( gt, fxw ), STP );
			bt = _mm256_srli_epi16( _mm256_maddubs_epi16( bt, fxw ), STP );
			rb = _mm256_srli_epi16( _mm256_maddubs_epi16( rb, fxw ), STP );
			gb = _mm256_srli_epi16( _mm256_maddubs_epi16( gb, fxw ), STP );
			bb = _mm256_srli_epi16( _mm256_maddubs_epi16( bb, fxw ), STP );

			__m256i fy = _mm256_and_si256( sy, vector256_int16_127 );

			rt = _mm256_add_epi16( rt, _mm256_srai_epi16( _mm256_mullo_epi16( _mm256_sub_epi16( rb, rt ), fy ), STP ) );
			gt = _mm256_add_epi16( gt, _mm256_srai_epi16( _mm256_mullo_epi16( _mm256_sub_epi16( gb, gt ), fy ), STP ) );
			bt = _mm256_add_epi16( bt, _mm256_srai_epi16( _mm256_mullo_epi16( _mm256_sub_epi16( bb, bt ), fy ), STP ) );

			rt = _mm256_packus_epi16( rt, rt );
			gt = _mm256_packus_epi16( gt, gt );
			bt = _mm256_packus_epi16( bt, bt );
			__m256i at = _mm256_setzero_si256();

			__m256i s0 = _mm256_unpacklo_epi8( rt, gt );		// r0, g0, r1, g1, r2, g2, r3, g3, r4, g4, r5, g5, r6, g6, r7, g7
			__m256i s1 = _mm256_unpacklo_epi8( bt, at );		// b0, a0, b1, a1, b2, a2, b3, a3, b4, a4, b5, a5, b6, a6, b7, a7
			__m256i s2 = _mm256_unpacklo_epi16( s0, s1 );		// r0, g0, b0, a0, r1, g1, b1, a1, r2, g2, b2, a2, r3, g3, b3, a3
			__m256i s3 = _mm256_unpackhi_epi16( s0, s1 );		// r4, g4, b4, a4, r5, g5, b5, a5, r6, g6, b6, a6, r7, g7, b7, a7

			_mm256_stream_si256( (__m256i *)( destRow + x + 0 ), s2 );
			_mm256_stream_si256( (__m256i *)( destRow + x + 8 ), s3 );

			sx = _mm256_add_epi16( sx, dx );
			sy = _mm256_add_epi16( sy, dy );
		}

#elif defined( __USE_SSE4__ )

		// This version uses PMADDUBSW which unfortunately multiplies an unsigned byte with a *signed* byte.
		// As a result, any fraction over 127 will be interpreted as a negative fraction. To keep the
		// fractions positive, the sub-texel precision is reduced to just 7 bits and there is a 1/128 loss
		// in brightness when interpolating horizontally because the fraction 1.0 (128) cannot be used.

		__m128i sx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcX8 ), 0 ), 0 );
		__m128i sy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcY8 ), 0 ), 0 );
		__m128i dx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaX8 ), 0 ), 0 );
		__m128i dy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaY8 ), 0 ), 0 );
		__m128i pitch = _mm_unpacklo_epi16( _mm_shufflelo_epi16( _mm_cvtsi32_si128( srcPitchInTexels ), 0 ), vector_int16_1 );

		sx = _mm_add_epi16( sx, _mm_mullo_epi16( dx, vector_int16_01234567 ) );
		sy = _mm_add_epi16( sy, _mm_mullo_epi16( dy, vector_int16_01234567 ) );
		dx = _mm_slli_epi16( dx, 3 );
		dy = _mm_slli_epi16( dy, 3 );

		for ( int x = 0; x < 32; x += 8 )
		{
			__m128i ax = _mm_srai_epi16( sx, STP );
			__m128i ay = _mm_srai_epi16( sy, STP );
			__m128i of0 = _mm_madd_epi16( _mm_unpacklo_epi16( ay, ax ), pitch );
			__m128i of1 = _mm_madd_epi16( _mm_unpackhi_epi16( ay, ax ), pitch );

			const unsigned int a0 = _mm_extract_epi32( of0, 0 );
			const unsigned int a1 = _mm_extract_epi32( of0, 1 );
			const unsigned int a2 = _mm_extract_epi32( of0, 2 );
			const unsigned int a3 = _mm_extract_epi32( of0, 3 );
			const unsigned int a4 = _mm_extract_epi32( of1, 0 );
			const unsigned int a5 = _mm_extract_epi32( of1, 1 );
			const unsigned int a6 = _mm_extract_epi32( of1, 2 );
			const unsigned int a7 = _mm_extract_epi32( of1, 3 );

			__m128i rt = _mm_setzero_si128();
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a0], 0 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a1], 1 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a2], 2 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a3], 3 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a4], 4 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a5], 5 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a6], 6 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a7], 7 );

			__m128i rb = _mm_setzero_si128();
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a0], 0 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a1], 1 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a2], 2 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a3], 3 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a4], 4 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a5], 5 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a6], 6 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a7], 7 );

			__m128i gt = _mm_setzero_si128();
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a0], 0 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a1], 1 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a2], 2 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a3], 3 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a4], 4 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a5], 5 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a6], 6 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a7], 7 );

			__m128i gb = _mm_setzero_si128();
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a0], 0 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a1], 1 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a2], 2 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a3], 3 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a4], 4 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a5], 5 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a6], 6 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a7], 7 );

			__m128i bt = _mm_setzero_si128();
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a0], 0 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a1], 1 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a2], 2 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a3], 3 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a4], 4 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a5], 5 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a6], 6 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a7], 7 );

			__m128i bb = _mm_setzero_si128();
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a0], 0 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a1], 1 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a2], 2 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a3], 3 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a4], 4 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a5], 5 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a6], 6 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a7], 7 );

			__m128i fx = _mm_and_si128( sx, vector_int16_127 );
			__m128i fxb = _mm_packus_epi16( fx, fx );
			__m128i fxw = _mm_xor_si128( _mm_unpacklo_epi8( fxb, fxb ), vector_int16_127 );

			rt = _mm_srli_epi16( _mm_maddubs_epi16( rt, fxw ), STP );
			gt = _mm_srli_epi16( _mm_maddubs_epi16( gt, fxw ), STP );
			bt = _mm_srli_epi16( _mm_maddubs_epi16( bt, fxw ), STP );
			rb = _mm_srli_epi16( _mm_maddubs_epi16( rb, fxw ), STP );
			gb = _mm_srli_epi16( _mm_maddubs_epi16( gb, fxw ), STP );
			bb = _mm_srli_epi16( _mm_maddubs_epi16( bb, fxw ), STP );

			__m128i fy = _mm_and_si128( sy, vector_int16_127 );

			rt = _mm_add_epi16( rt, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( rb, rt ), fy ), STP ) );
			gt = _mm_add_epi16( gt, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( gb, gt ), fy ), STP ) );
			bt = _mm_add_epi16( bt, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( bb, bt ), fy ), STP ) );

			rt = _mm_packus_epi16( rt, rt );
			gt = _mm_packus_epi16( gt, gt );
			bt = _mm_packus_epi16( bt, bt );
			__m128i at = _mm_setzero_si128();

			__m128i s0 = _mm_unpacklo_epi8( rt, gt );		// r0, g0, r1, g1, r2, g2, r3, g3, r4, g4, r5, g5, r6, g6, r7, g7
			__m128i s1 = _mm_unpacklo_epi8( bt, at );		// b0, a0, b1, a1, b2, a2, b3, a3, b4, a4, b5, a5, b6, a6, b7, a7
			__m128i s2 = _mm_unpacklo_epi16( s0, s1 );		// r0, g0, b0, a0, r1, g1, b1, a1, r2, g2, b2, a2, r3, g3, b3, a3
			__m128i s3 = _mm_unpackhi_epi16( s0, s1 );		// r4, g4, b4, a4, r5, g5, b5, a5, r6, g6, b6, a6, r7, g7, b7, a7

			_mm_stream_si128( (__m128i *)( destRow + x + 0 ), s2 );
			_mm_stream_si128( (__m128i *)( destRow + x + 4 ), s3 );

			sx = _mm_add_epi16( sx, dx );
			sy = _mm_add_epi16( sy, dy );
		}

#elif defined( __USE_SSE2__ )

		__m128i sx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcX8 ), 0 ), 0 );
		__m128i sy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcY8 ), 0 ), 0 );
		__m128i dx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaX8 ), 0 ), 0 );
		__m128i dy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaY8 ), 0 ), 0 );
		__m128i pitch = _mm_unpacklo_epi16( _mm_shufflelo_epi16( _mm_cvtsi32_si128( srcPitchInTexels ), 0 ), vector_int16_1 );

		sx = _mm_add_epi16( sx, _mm_mullo_epi16( dx, vector_int16_01234567 ) );
		sy = _mm_add_epi16( sy, _mm_mullo_epi16( dy, vector_int16_01234567 ) );
		dx = _mm_slli_epi16( dx, 3 );
		dy = _mm_slli_epi16( dy, 3 );

		for ( int x = 0; x < 32; x += 8 )
		{
			__m128i ax = _mm_srai_epi16( sx, STP );
			__m128i ay = _mm_srai_epi16( sy, STP );
			__m128i of0 = _mm_madd_epi16( _mm_unpacklo_epi16( ay, ax ), pitch );
			__m128i of1 = _mm_madd_epi16( _mm_unpackhi_epi16( ay, ax ), pitch );

			const unsigned int a0 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 0 ) );
			const unsigned int a1 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 1 ) );
			const unsigned int a2 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 2 ) );
			const unsigned int a3 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of0, 3 ) );
			const unsigned int a4 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 0 ) );
			const unsigned int a5 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 1 ) );
			const unsigned int a6 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 2 ) );
			const unsigned int a7 = _mm_cvtsi128_si32( _mm_shuffle_epi32( of1, 3 ) );

			__m128i rt = _mm_setzero_si128();
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a0], 0 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a1], 1 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a2], 2 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a3], 3 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a4], 4 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a5], 5 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a6], 6 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[a7], 7 );

			__m128i rb = _mm_setzero_si128();
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a0], 0 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a1], 1 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a2], 2 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a3], 3 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a4], 4 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a5], 5 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a6], 6 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + a7], 7 );

			__m128i gt = _mm_setzero_si128();
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a0], 0 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a1], 1 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a2], 2 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a3], 3 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a4], 4 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a5], 5 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a6], 6 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[a7], 7 );

			__m128i gb = _mm_setzero_si128();
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a0], 0 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a1], 1 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a2], 2 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a3], 3 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a4], 4 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a5], 5 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a6], 6 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + a7], 7 );

			__m128i bt = _mm_setzero_si128();
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a0], 0 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a1], 1 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a2], 2 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a3], 3 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a4], 4 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a5], 5 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a6], 6 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[a7], 7 );

			__m128i bb = _mm_setzero_si128();
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a0], 0 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a1], 1 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a2], 2 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a3], 3 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a4], 4 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a5], 5 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a6], 6 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + a7], 7 );

			__m128i fx = _mm_and_si128( sx, vector_int16_255 );
			__m128i fxl = _mm_add_epi32( _mm_xor_si128( _mm_unpacklo_epi16( fx, fx ), vector_int32_255 ), vector_int32_1 );
			__m128i fxh = _mm_add_epi32( _mm_xor_si128( _mm_unpackhi_epi16( fx, fx ), vector_int32_255 ), vector_int32_1 );

			__m128i rtl = _mm_srli_epi16( _mm_madd_epi16( _mm_unpacklo_epi8( rt, vector_uint8_0 ), fxl ), STP );
			__m128i gtl = _mm_srli_epi16( _mm_madd_epi16( _mm_unpacklo_epi8( gt, vector_uint8_0 ), fxl ), STP );
			__m128i btl = _mm_srli_epi16( _mm_madd_epi16( _mm_unpacklo_epi8( bt, vector_uint8_0 ), fxl ), STP );
			__m128i rth = _mm_srli_epi16( _mm_madd_epi16( _mm_unpackhi_epi8( rt, vector_uint8_0 ), fxh ), STP );
			__m128i gth = _mm_srli_epi16( _mm_madd_epi16( _mm_unpackhi_epi8( gt, vector_uint8_0 ), fxh ), STP );
			__m128i bth = _mm_srli_epi16( _mm_madd_epi16( _mm_unpackhi_epi8( bt, vector_uint8_0 ), fxh ), STP );

			__m128i rbl = _mm_srli_epi16( _mm_madd_epi16( _mm_unpacklo_epi8( rb, vector_uint8_0 ), fxl ), STP );
			__m128i gbl = _mm_srli_epi16( _mm_madd_epi16( _mm_unpacklo_epi8( gb, vector_uint8_0 ), fxl ), STP );
			__m128i bbl = _mm_srli_epi16( _mm_madd_epi16( _mm_unpacklo_epi8( bb, vector_uint8_0 ), fxl ), STP );
			__m128i rbh = _mm_srli_epi16( _mm_madd_epi16( _mm_unpackhi_epi8( rb, vector_uint8_0 ), fxh ), STP );
			__m128i gbh = _mm_srli_epi16( _mm_madd_epi16( _mm_unpackhi_epi8( gb, vector_uint8_0 ), fxh ), STP );
			__m128i bbh = _mm_srli_epi16( _mm_madd_epi16( _mm_unpackhi_epi8( bb, vector_uint8_0 ), fxh ), STP );

			rt = _mm_packs_epi32( rtl, rth );
			gt = _mm_packs_epi32( gtl, gth );
			bt = _mm_packs_epi32( btl, bth );
			rb = _mm_packs_epi32( rbl, rbh );
			gb = _mm_packs_epi32( gbl, gbh );
			bb = _mm_packs_epi32( bbl, bbh );

			__m128i fy = _mm_and_si128( sy, vector_int16_255 );

			rt = _mm_add_epi16( rt, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( rb, rt ), fy ), STP ) );
			gt = _mm_add_epi16( gt, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( gb, gt ), fy ), STP ) );
			bt = _mm_add_epi16( bt, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( bb, bt ), fy ), STP ) );

			rt = _mm_packus_epi16( rt, rt );
			gt = _mm_packus_epi16( gt, gt );
			bt = _mm_packus_epi16( bt, bt );
			__m128i at = _mm_setzero_si128();

			__m128i s0 = _mm_unpacklo_epi8( rt, gt );		// r0, g0, r1, g1, r2, g2, r3, g3, r4, g4, r5, g5, r6, g6, r7, g7
			__m128i s1 = _mm_unpacklo_epi8( bt, at );		// b0, a0, b1, a1, b2, a2, b3, a3, b4, a4, b5, a5, b6, a6, b7, a7
			__m128i s2 = _mm_unpacklo_epi16( s0, s1 );		// r0, g0, b0, a0, r1, g1, b1, a1, r2, g2, b2, a2, r3, g3, b3, a3
			__m128i s3 = _mm_unpackhi_epi16( s0, s1 );		// r4, g4, b4, a4, r5, g5, b5, a5, r6, g6, b6, a6, r7, g7, b7, a7

			_mm_stream_si128( ( __m128i * )( destRow + x + 0 ), s2 );
			_mm_stream_si128( ( __m128i * )( destRow + x + 4 ), s3 );

			sx = _mm_add_epi16( sx, dx );
			sy = _mm_add_epi16( sy, dy );
		}

#elif defined( __ARM_NEON__ )		// increased throughput

		__asm__ volatile(
			"	movw		r0, #0x0100									\n\t"	// r0 = 0x00000100
			"	movt		r0, #0x0302									\n\t"	// r0 = 0x03020100
			"	movw		r1, #0x0504									\n\t"	// r1 = 0x00000504
			"	movt		r1, #0x0706									\n\t"	// r1 = 0x07060504
			"	vmov.u32	d8[0], r0									\n\t"	// d8 = 0x0706050403020100
			"	vmov.u32	d8[1], r1									\n\t"	// d8 = 0x0706050403020100
			"	vmov.u32	d9, d8										\n\t"	// d9 = 0x0706050403020100

			"	vdup.u8		d1, %[sx]									\n\t"	//      srcX,      srcX,      srcX,      srcX,      srcX,      srcX,      srcX,      srcX
			"	vmov.u8		d0, #0xFF									\n\t"	//      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF
			"	veor.u8		d0, d0, d1									\n\t"	//   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1,   srcX^-1
			"	vdup.u8		d3, %[dx]									\n\t"	//    deltaX,    deltaX,    deltaX,    deltaX,    deltaX,    deltaX,    deltaX,    deltaX
			"	vmov.u8		d2, #0x00									\n\t"	//      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00
			"	vsub.u8		d2, d2, d3									\n\t"	//   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX,   -deltaX
			"	vmla.u8		q0, q1, q4									\n\t"
			"	vshl.u8		q1, q1, #3									\n\t"

			"	vdup.u8		d5, %[sy]									\n\t"	//      srcY,      srcY,      srcY,      srcY,      srcY,      srcY,      srcY,      srcY
			"	vmov.u8		d4, #0xFF									\n\t"	//      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF,      0xFF
			"	veor.u8		d4, d4, d5									\n\t"	//   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1,   srcY^-1
			"	vdup.u8		d7, %[dy]									\n\t"	//    deltaY,    deltaY,    deltaY,    deltaY,    deltaY,    deltaY,    deltaY,    deltaY
			"	vmov.u8		d6, #0x00									\n\t"	//      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00,      0x00
			"	vsub.u8		d6, d6, d7									\n\t"	//   -deltaY,   -deltaY,   -deltaY,   -deltaY,   -deltaY,   -deltaY,   -deltaY,   -deltaY
			"	vmla.u8		q2, q3, q4									\n\t"
			"	vshl.u8		q3, q3, #3									\n\t"

			"	uxth		%[sx], %[sx]								\n\t"
			"	orr			%[sx], %[sx], %[sy], lsl #16				\n\t"
			"	uxth		%[dx], %[dx]								\n\t"
			"	orr			%[dx], %[dx], %[dy], lsl #16				\n\t"

			"	add			%[sy], %[d], #128							\n\t"	// &destRow[32]
			"	mov			%[dy], #4									\n\t"	// dest pixel pitch

			".LOOP_BILINEAR_PLANAR_RGB:									\n\t"
			"	sxtb		r0, %[sx], ror #8							\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24							\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	sadd16		%[sx], %[sx], %[dx]							\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d14[0],d15[0]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d20[0],d21[0]}, [r1], %[p]					\n\t"
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d16[0],d17[0]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[0],d23[0]}, [r1], %[p]					\n\t"
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d18[0],d19[0]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d24[0],d25[0]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sx], ror #8							\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24							\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	sadd16		%[sx], %[sx], %[dx]							\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d14[1],d15[1]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d20[1],d21[1]}, [r1], %[p]					\n\t"
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d16[1],d17[1]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[1],d23[1]}, [r1], %[p]					\n\t"
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d18[1],d19[1]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d24[1],d25[1]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sx], ror #8							\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24							\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	sadd16		%[sx], %[sx], %[dx]							\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d14[2],d15[2]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d20[2],d21[2]}, [r1], %[p]					\n\t"
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d16[2],d17[2]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[2],d23[2]}, [r1], %[p]					\n\t"
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d18[2],d19[2]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d24[2],d25[2]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sx], ror #8							\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24							\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	sadd16		%[sx], %[sx], %[dx]							\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d14[3],d15[3]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d20[3],d21[3]}, [r1], %[p]					\n\t"
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d16[3],d17[3]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[3],d23[3]}, [r1], %[p]					\n\t"
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d18[3],d19[3]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d24[3],d25[3]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sx], ror #8							\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24							\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	sadd16		%[sx], %[sx], %[dx]							\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d14[4],d15[4]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d20[4],d21[4]}, [r1], %[p]					\n\t"
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d16[4],d17[4]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[4],d23[4]}, [r1], %[p]					\n\t"
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d18[4],d19[4]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d24[4],d25[4]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sx], ror #8							\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24							\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	sadd16		%[sx], %[sx], %[dx]							\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d14[5],d15[5]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d20[5],d21[5]}, [r1], %[p]					\n\t"
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d16[5],d17[5]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[5],d23[5]}, [r1], %[p]					\n\t"
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d18[5],d19[5]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d24[5],d25[5]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sx], ror #8							\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24							\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	sadd16		%[sx], %[sx], %[dx]							\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d14[6],d15[6]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d20[6],d21[6]}, [r1], %[p]					\n\t"
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d16[6],d17[6]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[6],d23[6]}, [r1], %[p]					\n\t"
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d18[6],d19[6]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d24[6],d25[6]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sx], ror #8							\n\t"	// srcX >> 8;
			"	sxtb		r1, %[sx], ror #24							\n\t"	// srcY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	sadd16		%[sx], %[sx], %[dx]							\n\t"	// (srcX, srcY) += (deltaX, deltaY)

			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d14[7],d15[7]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d20[7],d21[7]}, [r1], %[p]					\n\t"
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d16[7],d17[7]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[7],d23[7]}, [r1], %[p]					\n\t"
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcY >> 8 ) * srcPitchInTexels + ( srcX >> 8 );
			"	vld2.u8		{d18[7],d19[7]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d24[7],d25[7]}, [r1], %[p]					\n\t"

			"	vmull.u8	 q4, d14, d4								\n\t"	// weight top-left  red   with (fracY^-1)
			"	vmull.u8	 q5, d16, d4								\n\t"	// weight top-left  green with (fracY^-1)
			"	vmull.u8	 q6, d18, d4								\n\t"	// weight top-left  blue  with (fracY^-1)
			"	vmull.u8	 q7, d15, d4								\n\t"	// weight top-right red   with (fracY^-1)
			"	vmull.u8	 q8, d17, d4								\n\t"	// weight top-right green with (fracY^-1)
			"	vmull.u8	 q9, d19, d4								\n\t"	// weight top-right blue  with (fracY^-1)

			"	vmlal.u8	 q4, d20, d5								\n\t"	// weight bottom-left  red   with (fracY) and add to top-left  red   
			"	vmlal.u8	 q5, d22, d5								\n\t"	// weight bottom-left  green with (fracY) and add to top-left  green 
			"	vmlal.u8	 q6, d24, d5								\n\t"	// weight bottom-left  blue  with (fracY) and add to top-left  blue  
			"	vmlal.u8	 q7, d21, d5								\n\t"	// weight bottom-right red   with (fracY) and add to top-right red   
			"	vmlal.u8	 q8, d23, d5								\n\t"	// weight bottom-right green with (fracY) and add to top-right green 
			"	vmlal.u8	 q9, d25, d5								\n\t"	// weight bottom-right blue  with (fracY) and add to top-right blue  

			"	vqrshrn.u16	 d8,  q4, #8								\n\t"	// reduce left  red   to 8 bits of precision and half the register size
			"	vqrshrn.u16 d10,  q5, #8								\n\t"	// reduce left  green to 8 bits of precision and half the register size
			"	vqrshrn.u16 d12,  q6, #8								\n\t"	// reduce left  blue  to 8 bits of precision and half the register size
			"	vqrshrn.u16	d14,  q7, #8								\n\t"	// reduce right red   to 8 bits of precision and half the register size
			"	vqrshrn.u16	d16,  q8, #8								\n\t"	// reduce right green to 8 bits of precision and half the register size
			"	vqrshrn.u16	d18,  q9, #8								\n\t"	// reduce right blue  to 8 bits of precision and half the register size

			"	vmull.u8	 q4,  d8, d0								\n\t"	// weight left red   with (fracX^-1)
			"	vmull.u8	 q5, d10, d0								\n\t"	// weight left green with (fracX^-1)
			"	vmull.u8	 q6, d12, d0								\n\t"	// weight left blue  with (fracX^-1)

			"	vmlal.u8	 q4, d14, d1								\n\t"	// weight right red   with (fracX) and add to left red
			"	vmlal.u8	 q5, d16, d1								\n\t"	// weight right green with (fracX) and add to left green
			"	vmlal.u8	 q6, d18, d1								\n\t"	// weight right blue  with (fracX) and add to left blue

			"	vqrshrn.u16	 d8,  q4, #8								\n\t"	// reduce to 8 bits of precision and half the register size
			"	vqrshrn.u16	 d9,  q5, #8								\n\t"	// reduce to 8 bits of precision and half the register size
			"	vqrshrn.u16	d10,  q6, #8								\n\t"	// reduce to 8 bits of precision and half the register size
			"	vmov.u64	d11, 0										\n\t"

			"	vst4.u8		{d8[0],d9[0],d10[0],d11[0]}, [%[d]], %[dy]	\n\t"
			"	vst4.u8		{d8[1],d9[1],d10[1],d11[1]}, [%[d]], %[dy]	\n\t"
			"	vst4.u8		{d8[2],d9[2],d10[2],d11[2]}, [%[d]], %[dy]	\n\t"
			"	vst4.u8		{d8[3],d9[3],d10[3],d11[3]}, [%[d]], %[dy]	\n\t"
			"	vst4.u8		{d8[4],d9[4],d10[4],d11[4]}, [%[d]], %[dy]	\n\t"
			"	vst4.u8		{d8[5],d9[5],d10[5],d11[5]}, [%[d]], %[dy]	\n\t"
			"	vst4.u8		{d8[6],d9[6],d10[6],d11[6]}, [%[d]], %[dy]	\n\t"
			"	vst4.u8		{d8[7],d9[7],d10[7],d11[7]}, [%[d]], %[dy]	\n\t"

			"	vadd.u8		q0, q0, q1									\n\t"	// update the bilinear weights (fracX^-1), (fracX)
			"	vadd.u8		q2, q2, q3									\n\t"	// update the bilinear weights (fracY^-1), (fracY)

			"	cmp			%[d], %[sy]									\n\t"	// destRow == ( dest + y * destPitchInPixels + 32 )
			"	bne			.LOOP_BILINEAR_PLANAR_RGB					\n\t"
			:
			:	[sx] "r" (localSrcX8),
				[sy] "r" (localSrcY8),
				[dx] "r" (deltaX8),
				[dy] "r" (deltaY8),
				[sr] "r" (localSrcRed),
				[sg] "r" (localSrcGreen),
				[sb] "r" (localSrcBlue),
				[d] "r" (destRow),
				[p] "r" (srcPitchInTexels)
			:	"r0", "r1", "r2",
				"q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12",
				"memory"
		);

#elif defined( __HEXAGON_V50__ )	// 2 pixels per iteration appears to be optimal

		Word32 dxy0 = Q6_R_combine_RlRl( deltaY8, deltaX8 );
		Word64 dxy2 = Q6_P_vaslh_PI( Q6_P_combine_RR( dxy0, dxy0 ), 1 );		// dx2, dy2, dx2, dy2

		Word32 sxy0 = Q6_R_combine_RlRl( localSrcY8, localSrcX8 );				// sx0, sy0
		Word64 sxy2 = Q6_P_combine_RR( Q6_R_vaddh_RR( sxy0, dxy0 ), sxy0 );		// sx0, sy0, sx1, sy1

		Word32 pch0 = Q6_R_combine_RlRl( srcPitchInTexels, Q6_R_equals_I( 1 ) );
		Word64 pch2 = Q6_P_combine_RR( pch0, pch0 );
		Word64 pchr = Q6_P_combine_RR( srcPitchInTexels, srcPitchInTexels );

		Word64 cfrX = Q6_P_shuffeh_PP( sxy2, sxy2 );		// sx0, sx0, sx1, sx1
		Word64 cfrY = Q6_P_shuffoh_PP( sxy2, sxy2 );		// sy0, sy0, sy1, sy1

		Word64 dfrX = Q6_P_shuffeh_PP( dxy2, dxy2 );		// dx2, dx2, dx2, dx2
		Word64 dfrY = Q6_P_shuffoh_PP( dxy2, dxy2 );		// dy2, dy2, dy2, dy2

		cfrX = Q6_P_shuffeb_PP( cfrX, cfrX );				// fx0, fx0, fx0, fx0, fx1, fx1, fx1, fx1
		cfrY = Q6_P_shuffeb_PP( cfrY, cfrY );				// fy0, fy0, fy0, fy0, fy1, fy1, fy1, fy1

		dfrX = Q6_P_shuffeb_PP( dfrX, dfrX );				// dx2, dx2, dx2, dx2, dx2, dx2, dx2, dx2
		dfrY = Q6_P_shuffeb_PP( dfrY, dfrY );				// dy2, dy2, dy2, dy2, dy2, dy2, dy2, dy2

		Word64 zero = Q6_P_combine_II(  0,  0 );			// 0x0000000000000000
		Word64 none = Q6_P_combine_II( -1, -1 );			// 0xFFFFFFFFFFFFFFFF

		Word64 xfrX = Q6_P_shuffeh_PP( zero, none );		// 0x0000FFFF0000FFFF
		Word64 xfrY = Q6_P_shuffeb_PP( zero, none );		// 0x00FF00FF00FF00FF

		cfrX = Q6_P_xor_PP( cfrX, xfrX );					// 1-fx0, 1-fx0,   fx0, fx0, 1-fx1, 1-fx1,   fx1, fx1
		cfrY = Q6_P_xor_PP( cfrY, xfrY );					// 1-fy0,   fy0, 1-fy0, fy0, 1-fy1,   fy1, 1-fy1, fy1

		dfrX = Q6_P_vsubb_PP( Q6_P_xor_PP( dfrX, xfrX ), xfrX );
		dfrY = Q6_P_vsubb_PP( Q6_P_xor_PP( dfrY, xfrY ), xfrY );

		for ( int x = 0; x < 32; x += 2 )
		{
			Word64 offset0 = Q6_P_vdmpy_PP_sat( Q6_P_vasrh_PI( sxy2, STP ), pch2 );

			Word32 r0 = localSrcRed[  Q6_R_extract_Pl( offset0 ) + 0];	// r0
			Word32 g0 = localSrcGreen[Q6_R_extract_Pl( offset0 ) + 0];	// g0
			Word32 b0 = localSrcBlue[ Q6_R_extract_Pl( offset0 ) + 0];	// b0

			Word32 r1 = localSrcRed[  Q6_R_extract_Ph( offset0 ) + 0];	// r0
			Word32 g1 = localSrcGreen[Q6_R_extract_Ph( offset0 ) + 0];	// g0
			Word32 b1 = localSrcBlue[ Q6_R_extract_Ph( offset0 ) + 0];	// b0

			r0 = Q6_R_insert_RII( r0, localSrcRed[  Q6_R_extract_Pl( offset0 ) + 1], 8, 16 );	// r0, r2, r1
			g0 = Q6_R_insert_RII( g0, localSrcGreen[Q6_R_extract_Pl( offset0 ) + 1], 8, 16 );	// g0, g2, g1
			b0 = Q6_R_insert_RII( b0, localSrcBlue[ Q6_R_extract_Pl( offset0 ) + 1], 8, 16 );	// b0, b2, b1

			r1 = Q6_R_insert_RII( r1, localSrcRed[  Q6_R_extract_Ph( offset0 ) + 1], 8, 16 );	// r0, r2, r1
			g1 = Q6_R_insert_RII( g1, localSrcGreen[Q6_R_extract_Ph( offset0 ) + 1], 8, 16 );	// g0, g2, b1
			b1 = Q6_R_insert_RII( b1, localSrcBlue[ Q6_R_extract_Ph( offset0 ) + 1], 8, 16 );	// b0, b2, g1

			Word64 offset1 = Q6_P_vaddw_PP( offset0, pchr );

			r0 = Q6_R_insert_RII( r0, localSrcRed[  Q6_R_extract_Pl( offset1 ) + 0], 8, 8 );	// r0, r2
			g0 = Q6_R_insert_RII( g0, localSrcGreen[Q6_R_extract_Pl( offset1 ) + 0], 8, 8 );	// g0, g2
			b0 = Q6_R_insert_RII( b0, localSrcBlue[ Q6_R_extract_Pl( offset1 ) + 0], 8, 8 );	// b0, b2

			r1 = Q6_R_insert_RII( r1, localSrcRed[  Q6_R_extract_Ph( offset1 ) + 0], 8, 8 );	// r0, r2
			g1 = Q6_R_insert_RII( g1, localSrcGreen[Q6_R_extract_Ph( offset1 ) + 0], 8, 8 );	// g0, g2
			b1 = Q6_R_insert_RII( b1, localSrcBlue[ Q6_R_extract_Ph( offset1 ) + 0], 8, 8 );	// b0, b2

			r0 = Q6_R_insert_RII( r0, localSrcRed[  Q6_R_extract_Pl( offset1 ) + 1], 8, 24 );	// r0, r2, r1, r3
			g0 = Q6_R_insert_RII( g0, localSrcGreen[Q6_R_extract_Pl( offset1 ) + 1], 8, 24 );	// g0, g2, g1, g3
			b0 = Q6_R_insert_RII( b0, localSrcBlue[ Q6_R_extract_Pl( offset1 ) + 1], 8, 24 );	// b0, b2, b1, b3

			r1 = Q6_R_insert_RII( r1, localSrcRed[  Q6_R_extract_Ph( offset1 ) + 1], 8, 24 );	// r0, r2, r1, r3
			g1 = Q6_R_insert_RII( g1, localSrcGreen[Q6_R_extract_Ph( offset1 ) + 1], 8, 24 );	// g0, g2, b1, b3
			b1 = Q6_R_insert_RII( b1, localSrcBlue[ Q6_R_extract_Ph( offset1 ) + 1], 8, 24 );	// b0, b2, g1, g3

			Word64 t0 = Q6_P_combine_RR( b0, r0 );
			Word64 t1 = Q6_P_combine_RR(  0, g0 );
			Word64 t2 = Q6_P_combine_RR( b1, r1 );
			Word64 t3 = Q6_P_combine_RR(  0, g1 );

			Word32 m0 = Q6_R_vtrunohb_P( Q6_P_vmpybu_RR( Q6_R_extract_Pl( cfrX ), Q6_R_extract_Pl( cfrY ) ) );
			Word32 m1 = Q6_R_vtrunohb_P( Q6_P_vmpybu_RR( Q6_R_extract_Ph( cfrX ), Q6_R_extract_Ph( cfrY ) ) );

			Word64 f0 = Q6_P_combine_RR( m0, m0 );
			Word64 f1 = Q6_P_combine_RR( m1, m1 );

			t0 = Q6_P_vrmpybu_PP( t0, f0 );			// r, b
			t1 = Q6_P_vrmpybu_PP( t1, f0 );			// g, a
			t2 = Q6_P_vrmpybu_PP( t2, f1 );			// r, b
			t3 = Q6_P_vrmpybu_PP( t3, f1 );			// g, a

			t0 = Q6_P_shuffeh_PP( t1, t0 );			// r, g, b, a
			t2 = Q6_P_shuffeh_PP( t3, t2 );			// r, g, b, a

			*(Word64 *)&destRow[x] = Q6_P_combine_RR( Q6_R_vtrunohb_P( t2 ), Q6_R_vtrunohb_P( t0 ) );

			sxy2 = Q6_P_vaddh_PP( sxy2, dxy2 );

			cfrX = Q6_P_vaddub_PP( cfrX, dfrX );
			cfrY = Q6_P_vaddub_PP( cfrY, dfrY );
		}

#else

		for ( int x = 0; x < 32; x++ )
		{
			const int sampleX = localSrcX8 >> STP;
			const int sampleY = localSrcY8 >> STP;

			const unsigned char * texelR = localSrcRed + sampleY * srcPitchInTexels + sampleX;
			const unsigned char * texelG = localSrcGreen + sampleY * srcPitchInTexels + sampleX;
			const unsigned char * texelB = localSrcBlue + sampleY * srcPitchInTexels + sampleX;

			int r0 = texelR[0];
			int r1 = texelR[1];
			int r2 = texelR[srcPitchInTexels + 0];
			int r3 = texelR[srcPitchInTexels + 1];

			int g0 = texelG[0];
			int g1 = texelG[1];
			int g2 = texelG[srcPitchInTexels + 0];
			int g3 = texelG[srcPitchInTexels + 1];

			int b0 = texelB[0];
			int b1 = texelB[1];
			int b2 = texelB[srcPitchInTexels + 0];
			int b3 = texelB[srcPitchInTexels + 1];

			const int fracX1 = localSrcX8 & ( ( 1 << STP ) - 1 );
			const int fracX0 = ( 1 << STP ) - fracX1;
			const int fracY1 = localSrcY8 & ( ( 1 << STP ) - 1 );
			const int fracY0 = ( 1 << STP ) - fracY1;

			r0 = fracX0 * r0 + fracX1 * r1;
			r2 = fracX0 * r2 + fracX1 * r3;

			g0 = fracX0 * g0 + fracX1 * g1;
			g2 = fracX0 * g2 + fracX1 * g3;

			b0 = fracX0 * b0 + fracX1 * b1;
			b2 = fracX0 * b2 + fracX1 * b3;

			r0 = fracY0 * r0 + fracY1 * r2;
			g0 = fracY0 * g0 + fracY1 * g2;
			b0 = fracY0 * b0 + fracY1 * b2;

			*destRow++ =	( ( r0 & 0x00FF0000 ) >> 16 ) |
							( ( g0 & 0x00FF0000 ) >>  8 ) |
							( ( b0 & 0x00FF0000 ) >>  0 );

			localSrcX8 += deltaX8;
			localSrcY8 += deltaY8;
		}

#endif

		scanLeftSrcX  += scanLeftDeltaX;
		scanLeftSrcY  += scanLeftDeltaY;
		scanRightSrcX += scanRightDeltaX;
		scanRightSrcY += scanRightDeltaY;
	}

	//FlushCacheBox( dest, 32 * 4, 32, destPitchInPixels * 4 );
}

void Warp32x32_SampleChromaticBilinearPlanarRGB(
		const unsigned char * const	srcRed,
		const unsigned char * const	srcGreen,
		const unsigned char * const	srcBlue,
		const int					srcPitchInTexels,
		const int					srcTexelsWide,
		const int					srcTexelsHigh,
		unsigned char * const		dest,
		const int					destPitchInPixels,
		const MeshCoord_t *			meshCoordsRed,
		const MeshCoord_t *			meshCoordsGreen,
		const MeshCoord_t *			meshCoordsBlue,
		const int					meshStride )
{
	// The source texture needs to be sampled with the texture coordinate at the center of each destination pixel.
	// In other words, the texture coordinates are offset by 1/64 of the texture space spanned by the 32x32 destination quad.

	const int L32 = 5;	// log2( 32 )
	const int SCP = 16;	// scan-conversion precision
#if defined( __USE_SSE4__ ) || defined( __USE_AVX2__ )
	const int STP = 7;	// sub-texel precision
#else
	const int STP = 8;	// sub-texel precision
#endif

	//ZeroCacheBox( dest, 32 * 4, 32, destPitchInPixels * 4 );

	// Clamping the corners may distort quads that sample close to the edges, but that should not be noticable because these quads
	// are close to the far peripheral vision, where the human eye is weak when it comes to distinguishing color and shape.
	int clampedCornersRed[4][2];
	int clampedCornersGreen[4][2];
	int clampedCornersBlue[4][2];
	for ( int i = 0; i < 4; i++ )
	{
		clampedCornersRed[i][0] = ClampInt( (int)( meshCoordsRed[( i >> 1 ) * meshStride + ( i & 1 )].x * ( srcTexelsWide << SCP ) ), 0, ( srcTexelsWide - 2 ) << SCP );
		clampedCornersRed[i][1] = ClampInt( (int)( meshCoordsRed[( i >> 1 ) * meshStride + ( i & 1 )].y * ( srcTexelsHigh << SCP ) ), 0, ( srcTexelsHigh - 2 ) << SCP );
		clampedCornersGreen[i][0] = ClampInt( (int)( meshCoordsGreen[( i >> 1 ) * meshStride + ( i & 1 )].x * ( srcTexelsWide << SCP ) ), 0, ( srcTexelsWide - 2 ) << SCP );
		clampedCornersGreen[i][1] = ClampInt( (int)( meshCoordsGreen[( i >> 1 ) * meshStride + ( i & 1 )].y * ( srcTexelsHigh << SCP ) ), 0, ( srcTexelsHigh - 2 ) << SCP );
		clampedCornersBlue[i][0] = ClampInt( (int)( meshCoordsBlue[( i >> 1 ) * meshStride + ( i & 1 )].x * ( srcTexelsWide << SCP ) ), 0, ( srcTexelsWide - 2 ) << SCP );
		clampedCornersBlue[i][1] = ClampInt( (int)( meshCoordsBlue[( i >> 1 ) * meshStride + ( i & 1 )].y * ( srcTexelsHigh << SCP ) ), 0, ( srcTexelsHigh - 2 ) << SCP );
	}

	// calculate the axis-aligned bounding box of source texture space that may be sampled
	const int minSrcRedX = ( MinInt4( clampedCornersRed[0][0], clampedCornersRed[1][0], clampedCornersRed[2][0], clampedCornersRed[3][0] ) >> SCP ) + 0;
	const int maxSrcRedX = ( MaxInt4( clampedCornersRed[0][0], clampedCornersRed[1][0], clampedCornersRed[2][0], clampedCornersRed[3][0] ) >> SCP ) + 1;
	const int minSrcRedY = ( MinInt4( clampedCornersRed[0][1], clampedCornersRed[1][1], clampedCornersRed[2][1], clampedCornersRed[3][1] ) >> SCP ) + 0;
	const int maxSrcRedY = ( MaxInt4( clampedCornersRed[0][1], clampedCornersRed[1][1], clampedCornersRed[2][1], clampedCornersRed[3][1] ) >> SCP ) + 1;

	const int minSrcGreenX = ( MinInt4( clampedCornersGreen[0][0], clampedCornersGreen[1][0], clampedCornersGreen[2][0], clampedCornersGreen[3][0] ) >> SCP ) + 0;
	const int maxSrcGreenX = ( MaxInt4( clampedCornersGreen[0][0], clampedCornersGreen[1][0], clampedCornersGreen[2][0], clampedCornersGreen[3][0] ) >> SCP ) + 1;
	const int minSrcGreenY = ( MinInt4( clampedCornersGreen[0][1], clampedCornersGreen[1][1], clampedCornersGreen[2][1], clampedCornersGreen[3][1] ) >> SCP ) + 0;
	const int maxSrcGreenY = ( MaxInt4( clampedCornersGreen[0][1], clampedCornersGreen[1][1], clampedCornersGreen[2][1], clampedCornersGreen[3][1] ) >> SCP ) + 1;

	const int minSrcBlueX = ( MinInt4( clampedCornersBlue[0][0], clampedCornersBlue[1][0], clampedCornersBlue[2][0], clampedCornersBlue[3][0] ) >> SCP ) + 0;
	const int maxSrcBlueX = ( MaxInt4( clampedCornersBlue[0][0], clampedCornersBlue[1][0], clampedCornersBlue[2][0], clampedCornersBlue[3][0] ) >> SCP ) + 1;
	const int minSrcBlueY = ( MinInt4( clampedCornersBlue[0][1], clampedCornersBlue[1][1], clampedCornersBlue[2][1], clampedCornersBlue[3][1] ) >> SCP ) + 0;
	const int maxSrcBlueY = ( MaxInt4( clampedCornersBlue[0][1], clampedCornersBlue[1][1], clampedCornersBlue[2][1], clampedCornersBlue[3][1] ) >> SCP ) + 1;

	// prefetch all source texture data that is possibly sampled
	PrefetchBox( srcRed + ( minSrcRedY * srcPitchInTexels + minSrcRedX ), ( maxSrcRedX - minSrcRedX ), ( maxSrcRedY - minSrcRedY ), srcPitchInTexels );

	// vertical deltas in 16.16 fixed point
	const int scanLeftDeltaRedX  = ( clampedCornersRed[2][0] - clampedCornersRed[0][0] ) >> L32;
	const int scanLeftDeltaRedY  = ( clampedCornersRed[2][1] - clampedCornersRed[0][1] ) >> L32;
	const int scanRightDeltaRedX = ( clampedCornersRed[3][0] - clampedCornersRed[1][0] ) >> L32;
	const int scanRightDeltaRedY = ( clampedCornersRed[3][1] - clampedCornersRed[1][1] ) >> L32;

	const int scanLeftDeltaGreenX  = ( clampedCornersGreen[2][0] - clampedCornersGreen[0][0] ) >> L32;
	const int scanLeftDeltaGreenY  = ( clampedCornersGreen[2][1] - clampedCornersGreen[0][1] ) >> L32;
	const int scanRightDeltaGreenX = ( clampedCornersGreen[3][0] - clampedCornersGreen[1][0] ) >> L32;
	const int scanRightDeltaGreenY = ( clampedCornersGreen[3][1] - clampedCornersGreen[1][1] ) >> L32;

	const int scanLeftDeltaBlueX  = ( clampedCornersBlue[2][0] - clampedCornersBlue[0][0] ) >> L32;
	const int scanLeftDeltaBlueY  = ( clampedCornersBlue[2][1] - clampedCornersBlue[0][1] ) >> L32;
	const int scanRightDeltaBlueX = ( clampedCornersBlue[3][0] - clampedCornersBlue[1][0] ) >> L32;
	const int scanRightDeltaBlueY = ( clampedCornersBlue[3][1] - clampedCornersBlue[1][1] ) >> L32;

	// scan-line texture coordinates in 16.16 fixed point with half-pixel vertical offset
	int scanLeftSrcRedX  = clampedCornersRed[0][0] + ( ( clampedCornersRed[2][0] - clampedCornersRed[0][0] ) >> ( L32 + 1 ) );
	int scanLeftSrcRedY  = clampedCornersRed[0][1] + ( ( clampedCornersRed[2][1] - clampedCornersRed[0][1] ) >> ( L32 + 1 ) );
	int scanRightSrcRedX = clampedCornersRed[1][0] + ( ( clampedCornersRed[3][0] - clampedCornersRed[1][0] ) >> ( L32 + 1 ) );
	int scanRightSrcRedY = clampedCornersRed[1][1] + ( ( clampedCornersRed[3][1] - clampedCornersRed[1][1] ) >> ( L32 + 1 ) );

	int scanLeftSrcGreenX  = clampedCornersGreen[0][0] + ( ( clampedCornersGreen[2][0] - clampedCornersGreen[0][0] ) >> ( L32 + 1 ) );
	int scanLeftSrcGreenY  = clampedCornersGreen[0][1] + ( ( clampedCornersGreen[2][1] - clampedCornersGreen[0][1] ) >> ( L32 + 1 ) );
	int scanRightSrcGreenX = clampedCornersGreen[1][0] + ( ( clampedCornersGreen[3][0] - clampedCornersGreen[1][0] ) >> ( L32 + 1 ) );
	int scanRightSrcGreenY = clampedCornersGreen[1][1] + ( ( clampedCornersGreen[3][1] - clampedCornersGreen[1][1] ) >> ( L32 + 1 ) );

	int scanLeftSrcBlueX  = clampedCornersBlue[0][0] + ( ( clampedCornersBlue[2][0] - clampedCornersBlue[0][0] ) >> ( L32 + 1 ) );
	int scanLeftSrcBlueY  = clampedCornersBlue[0][1] + ( ( clampedCornersBlue[2][1] - clampedCornersBlue[0][1] ) >> ( L32 + 1 ) );
	int scanRightSrcBlueX = clampedCornersBlue[1][0] + ( ( clampedCornersBlue[3][0] - clampedCornersBlue[1][0] ) >> ( L32 + 1 ) );
	int scanRightSrcBlueY = clampedCornersBlue[1][1] + ( ( clampedCornersBlue[3][1] - clampedCornersBlue[1][1] ) >> ( L32 + 1 ) );

	for ( int y = 0; y < 32; y++ )
	{
		if ( y == 1 ) { PrefetchBox( srcGreen + ( minSrcGreenY * srcPitchInTexels + minSrcGreenX ), ( maxSrcGreenX - minSrcGreenX ), ( maxSrcGreenY - minSrcGreenY ), srcPitchInTexels ); }
		else if ( y == 2 ) { PrefetchBox( srcBlue + ( minSrcBlueY * srcPitchInTexels + minSrcBlueX ), ( maxSrcBlueX - minSrcBlueX ), ( maxSrcBlueY - minSrcBlueY ), srcPitchInTexels ); }

		// scan-line texture coordinates in 16.16 fixed point with half-pixel horizontal offset
		const int srcRedX16 = scanLeftSrcRedX + ( ( scanRightSrcRedX - scanLeftSrcRedX ) >> ( L32 + 1 ) );
		const int srcRedY16 = scanLeftSrcRedY + ( ( scanRightSrcRedY - scanLeftSrcRedY ) >> ( L32 + 1 ) );
		const int srcGreenX16 = scanLeftSrcGreenX + ( ( scanRightSrcGreenX - scanLeftSrcGreenX ) >> ( L32 + 1 ) );
		const int srcGreenY16 = scanLeftSrcGreenY + ( ( scanRightSrcGreenY - scanLeftSrcGreenY ) >> ( L32 + 1 ) );
		const int srcBlueX16 = scanLeftSrcBlueX + ( ( scanRightSrcBlueX - scanLeftSrcBlueX ) >> ( L32 + 1 ) );
		const int srcBlueY16 = scanLeftSrcBlueY + ( ( scanRightSrcBlueY - scanLeftSrcBlueY ) >> ( L32 + 1 ) );

		// horizontal deltas in 16.16 fixed point
		const int deltaRedX16 = ( scanRightSrcRedX - scanLeftSrcRedX ) >> L32;
		const int deltaRedY16 = ( scanRightSrcRedY - scanLeftSrcRedY ) >> L32;
		const int deltaGreenX16 = ( scanRightSrcGreenX - scanLeftSrcGreenX ) >> L32;
		const int deltaGreenY16 = ( scanRightSrcGreenY - scanLeftSrcGreenY ) >> L32;
		const int deltaBlueX16 = ( scanRightSrcBlueX - scanLeftSrcBlueX ) >> L32;
		const int deltaBlueY16 = ( scanRightSrcBlueY - scanLeftSrcBlueY ) >> L32;

		// get the sign of the deltas
		const int deltaSignRedX = ( deltaRedX16 >> 31 );
		const int deltaSignRedY = ( deltaRedY16 >> 31 );
		const int deltaSignGreenX = ( deltaGreenX16 >> 31 );
		const int deltaSignGreenY = ( deltaGreenY16 >> 31 );
		const int deltaSignBlueX = ( deltaBlueX16 >> 31 );
		const int deltaSignBlueY = ( deltaBlueY16 >> 31 );

		// reduce the deltas to 16.8 fixed-point (may be negative sign extended)
		const int deltaRedX8 = ( ( ( ( deltaRedX16 ^ deltaSignRedX ) - deltaSignRedX ) >> ( SCP - STP ) ) ^ deltaSignRedX ) - deltaSignRedX;
		const int deltaRedY8 = ( ( ( ( deltaRedY16 ^ deltaSignRedY ) - deltaSignRedY ) >> ( SCP - STP ) ) ^ deltaSignRedY ) - deltaSignRedY;
		const int deltaGreenX8 = ( ( ( ( deltaGreenX16 ^ deltaSignGreenX ) - deltaSignGreenX ) >> ( SCP - STP ) ) ^ deltaSignGreenX ) - deltaSignGreenX;
		const int deltaGreenY8 = ( ( ( ( deltaGreenY16 ^ deltaSignGreenY ) - deltaSignGreenY ) >> ( SCP - STP ) ) ^ deltaSignGreenY ) - deltaSignGreenY;
		const int deltaBlueX8 = ( ( ( ( deltaBlueX16 ^ deltaSignBlueX ) - deltaSignBlueX ) >> ( SCP - STP ) ) ^ deltaSignBlueX ) - deltaSignBlueX;
		const int deltaBlueY8 = ( ( ( ( deltaBlueY16 ^ deltaSignBlueY ) - deltaSignBlueY ) >> ( SCP - STP ) ) ^ deltaSignBlueY ) - deltaSignBlueY;

		// reduce the source coordinates to 16.8 fixed-point
		const int srcRedX8 = srcRedX16 >> ( SCP - STP );
		const int srcRedY8 = srcRedY16 >> ( SCP - STP );
		const int srcGreenX8 = srcGreenX16 >> ( SCP - STP );
		const int srcGreenY8 = srcGreenY16 >> ( SCP - STP );
		const int srcBlueX8 = srcBlueX16 >> ( SCP - STP );
		const int srcBlueY8 = srcBlueY16 >> ( SCP - STP );

		// get the top-left corner of the bounding box of the texture space sampled by this scan-line
		const int srcBoundsTopLeftRedX = MinInt( scanLeftSrcRedX, scanRightSrcRedX ) >> SCP;
		const int srcBoundsTopLeftRedY = MinInt( scanLeftSrcRedY, scanRightSrcRedY ) >> SCP;
		const int srcBoundsTopLeftGreenX = MinInt( scanLeftSrcGreenX, scanRightSrcGreenX ) >> SCP;
		const int srcBoundsTopLeftGreenY = MinInt( scanLeftSrcGreenY, scanRightSrcGreenY ) >> SCP;
		const int srcBoundsTopLeftBlueX = MinInt( scanLeftSrcBlueX, scanRightSrcBlueX ) >> SCP;
		const int srcBoundsTopLeftBlueY = MinInt( scanLeftSrcBlueY, scanRightSrcBlueY ) >> SCP;

		// localize the source pointer and source coordinates to allow using 8.8 fixed point
		const unsigned char * const localSrcRed = srcRed + ( srcBoundsTopLeftRedY * srcPitchInTexels + srcBoundsTopLeftRedX );
		const unsigned char * const localSrcGreen = srcGreen + ( srcBoundsTopLeftGreenY * srcPitchInTexels + srcBoundsTopLeftGreenX );
		const unsigned char * const localSrcBlue = srcBlue + ( srcBoundsTopLeftBlueY * srcPitchInTexels + srcBoundsTopLeftBlueX );

		int localSrcRedX8 = srcRedX8 - ( srcBoundsTopLeftRedX << STP );
		int localSrcRedY8 = srcRedY8 - ( srcBoundsTopLeftRedY << STP );
		int localSrcGreenX8 = srcGreenX8 - ( srcBoundsTopLeftGreenX << STP );
		int localSrcGreenY8 = srcGreenY8 - ( srcBoundsTopLeftGreenY << STP );
		int localSrcBlueX8 = srcBlueX8 - ( srcBoundsTopLeftBlueX << STP );
		int localSrcBlueY8 = srcBlueY8 - ( srcBoundsTopLeftBlueY << STP );

		unsigned int * destRow = (unsigned int *)dest + y * destPitchInPixels;

#if defined( __USE_AVX2__ )

		// This version uses VPMADDUBSW which unfortunately multiplies an unsigned byte with a *signed* byte.
		// As a result, any fraction over 127 will be interpreted as a negative fraction. To keep the
		// fractions positive, the sub-texel precision is reduced to just 7 bits and there is a 1/128 loss
		// in brightness when interpolating horizontally because the fraction 1.0 (128) cannot be used.

		__m256i rsx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcRedX8 ) );
		__m256i rsy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcRedY8 ) );
		__m256i rdx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaRedX8 ) );
		__m256i rdy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaRedY8 ) );

		__m256i gsx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcGreenX8 ) );
		__m256i gsy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcGreenY8 ) );
		__m256i gdx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaGreenX8 ) );
		__m256i gdy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaGreenY8 ) );

		__m256i bsx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcBlueX8 ) );
		__m256i bsy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( localSrcBlueY8 ) );
		__m256i bdx = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaBlueX8 ) );
		__m256i bdy = _mm256_broadcastw_epi16( _mm_cvtsi32_si128( deltaBlueY8 ) );

		rsx = _mm256_add_epi16( rsx, _mm256_mullo_epi16( rdx, vector256_int16_012389AB4567CDEF ) );
		rsy = _mm256_add_epi16( rsy, _mm256_mullo_epi16( rdy, vector256_int16_012389AB4567CDEF ) );
		rdx = _mm256_slli_epi16( rdx, 4 );
		rdy = _mm256_slli_epi16( rdy, 4 );

		gsx = _mm256_add_epi16( gsx, _mm256_mullo_epi16( gdx, vector256_int16_012389AB4567CDEF ) );
		gsy = _mm256_add_epi16( gsy, _mm256_mullo_epi16( gdy, vector256_int16_012389AB4567CDEF ) );
		gdx = _mm256_slli_epi16( gdx, 4 );
		gdy = _mm256_slli_epi16( gdy, 4 );

		bsx = _mm256_add_epi16( bsx, _mm256_mullo_epi16( bdx, vector256_int16_012389AB4567CDEF ) );
		bsy = _mm256_add_epi16( bsy, _mm256_mullo_epi16( bdy, vector256_int16_012389AB4567CDEF ) );
		bdx = _mm256_slli_epi16( bdx, 4 );
		bdy = _mm256_slli_epi16( bdy, 4 );

		__m256i pitch16 = _mm256_unpacklo_epi16( _mm256_broadcastw_epi16( _mm_cvtsi32_si128( srcPitchInTexels ) ), vector256_int16_1 );
		__m256i pitch32 = _mm256_broadcastd_epi32( _mm_cvtsi32_si128( srcPitchInTexels ) );

		for ( int x = 0; x < 32; x += 16 )
		{
#if 1
			__m256i rax = _mm256_srai_epi16( rsx, STP );
			__m256i ray = _mm256_srai_epi16( rsy, STP );

			__m256i rof0 = _mm256_madd_epi16( _mm256_unpacklo_epi16( ray, rax ), pitch16 );
			__m256i rof1 = _mm256_madd_epi16( _mm256_unpackhi_epi16( ray, rax ), pitch16 );
			__m256i rof2 = _mm256_add_epi32( rof0, pitch32 );
			__m256i rof3 = _mm256_add_epi32( rof1, pitch32 );

			__m256i rtl = _mm256_i32gather_epi32( (const int *)localSrcRed, rof0, 1 );
			__m256i rth = _mm256_i32gather_epi32( (const int *)localSrcRed, rof1, 1 );
			__m256i rbl = _mm256_i32gather_epi32( (const int *)localSrcRed, rof2, 1 );
			__m256i rbh = _mm256_i32gather_epi32( (const int *)localSrcRed, rof3, 1 );

			__m256i gax = _mm256_srai_epi16( gsx, STP );
			__m256i gay = _mm256_srai_epi16( gsy, STP );

			__m256i gof0 = _mm256_madd_epi16( _mm256_unpacklo_epi16( gay, gax ), pitch16 );
			__m256i gof1 = _mm256_madd_epi16( _mm256_unpackhi_epi16( gay, gax ), pitch16 );
			__m256i gof2 = _mm256_add_epi32( gof0, pitch32 );
			__m256i gof3 = _mm256_add_epi32( gof1, pitch32 );

			__m256i gtl = _mm256_i32gather_epi32( (const int *)localSrcGreen, gof0, 1 );
			__m256i gth = _mm256_i32gather_epi32( (const int *)localSrcGreen, gof1, 1 );
			__m256i gbl = _mm256_i32gather_epi32( (const int *)localSrcGreen, gof2, 1 );
			__m256i gbh = _mm256_i32gather_epi32( (const int *)localSrcGreen, gof3, 1 );

			__m256i bax = _mm256_srai_epi16( bsx, STP );
			__m256i bay = _mm256_srai_epi16( bsy, STP );

			__m256i bof0 = _mm256_madd_epi16( _mm256_unpacklo_epi16( bay, bax ), pitch16 );
			__m256i bof1 = _mm256_madd_epi16( _mm256_unpackhi_epi16( bay, bax ), pitch16 );
			__m256i bof2 = _mm256_add_epi32( bof0, pitch32 );
			__m256i bof3 = _mm256_add_epi32( bof1, pitch32 );

			__m256i btl = _mm256_i32gather_epi32( (const int *)localSrcBlue, bof0, 1 );
			__m256i bth = _mm256_i32gather_epi32( (const int *)localSrcBlue, bof1, 1 );
			__m256i bbl = _mm256_i32gather_epi32( (const int *)localSrcBlue, bof2, 1 );
			__m256i bbh = _mm256_i32gather_epi32( (const int *)localSrcBlue, bof3, 1 );

			__m256i rt = _mm256_pack_epi32( rtl, rth );
			__m256i rb = _mm256_pack_epi32( rbl, rbh );
			__m256i gt = _mm256_pack_epi32( gtl, gth );
			__m256i gb = _mm256_pack_epi32( gbl, gbh );
			__m256i bt = _mm256_pack_epi32( btl, bth );
			__m256i bb = _mm256_pack_epi32( bbl, bbh );
#else
			__m256i rax = _mm256_srai_epi16( rsx, STP );
			__m256i ray = _mm256_srai_epi16( rsy, STP );

			__m256i rof0 = _mm256_madd_epi16( _mm256_unpacklo_epi16( ray, rax ), pitch16 );
			__m256i rof1 = _mm256_madd_epi16( _mm256_unpackhi_epi16( ray, rax ), pitch16 );

			__m128i ro0 = _mm256_extracti128_si256( rof0, 0 );
			__m128i ro1 = _mm256_extracti128_si256( rof0, 1 );
			__m128i ro2 = _mm256_extracti128_si256( rof1, 0 );
			__m128i ro3 = _mm256_extracti128_si256( rof1, 1 );

			const unsigned int ra0 = _mm_extract_epi32( ro0, 0 );
			const unsigned int ra1 = _mm_extract_epi32( ro0, 1 );
			const unsigned int ra2 = _mm_extract_epi32( ro0, 2 );
			const unsigned int ra3 = _mm_extract_epi32( ro0, 3 );
			const unsigned int ra4 = _mm_extract_epi32( ro1, 0 );
			const unsigned int ra5 = _mm_extract_epi32( ro1, 1 );
			const unsigned int ra6 = _mm_extract_epi32( ro1, 2 );
			const unsigned int ra7 = _mm_extract_epi32( ro1, 3 );
			const unsigned int ra8 = _mm_extract_epi32( ro2, 0 );
			const unsigned int ra9 = _mm_extract_epi32( ro2, 1 );
			const unsigned int raA = _mm_extract_epi32( ro2, 2 );
			const unsigned int raB = _mm_extract_epi32( ro2, 3 );
			const unsigned int raC = _mm_extract_epi32( ro3, 0 );
			const unsigned int raD = _mm_extract_epi32( ro3, 1 );
			const unsigned int raE = _mm_extract_epi32( ro3, 2 );
			const unsigned int raF = _mm_extract_epi32( ro3, 3 );

			__m128i rtl = _mm_setzero_si128();
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[ra0], 0 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[ra1], 1 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[ra2], 2 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[ra3], 3 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[ra8], 4 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[ra9], 5 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[raA], 6 );
			rtl = _mm_insert_epi16( rtl, *(const unsigned short *)&localSrcRed[raB], 7 );

			__m128i rth = _mm_setzero_si128();
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[ra4], 0 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[ra5], 1 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[ra6], 2 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[ra7], 3 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[raC], 4 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[raD], 5 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[raE], 6 );
			rth = _mm_insert_epi16( rth, *(const unsigned short *)&localSrcRed[raF], 7 );

			__m128i rbl = _mm_setzero_si128();
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra0], 0 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra1], 1 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra2], 2 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra3], 3 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra8], 4 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra9], 5 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + raA], 6 );
			rbl = _mm_insert_epi16( rbl, *(const unsigned short *)&localSrcRed[srcPitchInTexels + raB], 7 );

			__m128i rbh = _mm_setzero_si128();
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra4], 0 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra5], 1 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra6], 2 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra7], 3 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + raC], 4 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + raD], 5 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + raE], 6 );
			rbh = _mm_insert_epi16( rbh, *(const unsigned short *)&localSrcRed[srcPitchInTexels + raF], 7 );

			__m256i gax = _mm256_srai_epi16( gsx, STP );
			__m256i gay = _mm256_srai_epi16( gsy, STP );

			__m256i gof0 = _mm256_madd_epi16( _mm256_unpacklo_epi16( gay, gax ), pitch16 );
			__m256i gof1 = _mm256_madd_epi16( _mm256_unpackhi_epi16( gay, gax ), pitch16 );

			__m128i go0 = _mm256_extracti128_si256( gof0, 0 );
			__m128i go1 = _mm256_extracti128_si256( gof0, 1 );
			__m128i go2 = _mm256_extracti128_si256( gof1, 0 );
			__m128i go3 = _mm256_extracti128_si256( gof1, 1 );

			const unsigned int ga0 = _mm_extract_epi32( go0, 0 );
			const unsigned int ga1 = _mm_extract_epi32( go0, 1 );
			const unsigned int ga2 = _mm_extract_epi32( go0, 2 );
			const unsigned int ga3 = _mm_extract_epi32( go0, 3 );
			const unsigned int ga4 = _mm_extract_epi32( go1, 0 );
			const unsigned int ga5 = _mm_extract_epi32( go1, 1 );
			const unsigned int ga6 = _mm_extract_epi32( go1, 2 );
			const unsigned int ga7 = _mm_extract_epi32( go1, 3 );
			const unsigned int ga8 = _mm_extract_epi32( go2, 0 );
			const unsigned int ga9 = _mm_extract_epi32( go2, 1 );
			const unsigned int gaA = _mm_extract_epi32( go2, 2 );
			const unsigned int gaB = _mm_extract_epi32( go2, 3 );
			const unsigned int gaC = _mm_extract_epi32( go3, 0 );
			const unsigned int gaD = _mm_extract_epi32( go3, 1 );
			const unsigned int gaE = _mm_extract_epi32( go3, 2 );
			const unsigned int gaF = _mm_extract_epi32( go3, 3 );

			__m128i gtl = _mm_setzero_si128();
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[ga0], 0 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[ga1], 1 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[ga2], 2 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[ga3], 3 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[ga8], 4 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[ga9], 5 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[gaA], 6 );
			gtl = _mm_insert_epi16( gtl, *(const unsigned short *)&localSrcGreen[gaB], 7 );

			__m128i gth = _mm_setzero_si128();
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[ga4], 0 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[ga5], 1 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[ga6], 2 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[ga7], 3 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[gaC], 4 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[gaD], 5 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[gaE], 6 );
			gth = _mm_insert_epi16( gth, *(const unsigned short *)&localSrcGreen[gaF], 7 );

			__m128i gbl = _mm_setzero_si128();
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga0], 0 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga1], 1 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga2], 2 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga3], 3 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga8], 4 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga9], 5 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + gaA], 6 );
			gbl = _mm_insert_epi16( gbl, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + gaB], 7 );

			__m128i gbh = _mm_setzero_si128();
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga4], 0 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga5], 1 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga6], 2 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga7], 3 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + gaC], 4 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + gaD], 5 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + gaE], 6 );
			gbh = _mm_insert_epi16( gbh, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + gaF], 7 );

			__m256i bax = _mm256_srai_epi16( bsx, STP );
			__m256i bay = _mm256_srai_epi16( bsy, STP );

			__m256i bof0 = _mm256_madd_epi16( _mm256_unpacklo_epi16( bay, bax ), pitch16 );
			__m256i bof1 = _mm256_madd_epi16( _mm256_unpackhi_epi16( bay, bax ), pitch16 );

			__m128i bo0 = _mm256_extracti128_si256( bof0, 0 );
			__m128i bo1 = _mm256_extracti128_si256( bof0, 1 );
			__m128i bo2 = _mm256_extracti128_si256( bof1, 0 );
			__m128i bo3 = _mm256_extracti128_si256( bof1, 1 );

			const unsigned int ba0 = _mm_extract_epi32( bo0, 0 );
			const unsigned int ba1 = _mm_extract_epi32( bo0, 1 );
			const unsigned int ba2 = _mm_extract_epi32( bo0, 2 );
			const unsigned int ba3 = _mm_extract_epi32( bo0, 3 );
			const unsigned int ba4 = _mm_extract_epi32( bo1, 0 );
			const unsigned int ba5 = _mm_extract_epi32( bo1, 1 );
			const unsigned int ba6 = _mm_extract_epi32( bo1, 2 );
			const unsigned int ba7 = _mm_extract_epi32( bo1, 3 );
			const unsigned int ba8 = _mm_extract_epi32( bo2, 0 );
			const unsigned int ba9 = _mm_extract_epi32( bo2, 1 );
			const unsigned int baA = _mm_extract_epi32( bo2, 2 );
			const unsigned int baB = _mm_extract_epi32( bo2, 3 );
			const unsigned int baC = _mm_extract_epi32( bo3, 0 );
			const unsigned int baD = _mm_extract_epi32( bo3, 1 );
			const unsigned int baE = _mm_extract_epi32( bo3, 2 );
			const unsigned int baF = _mm_extract_epi32( bo3, 3 );

			__m128i btl = _mm_setzero_si128();
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[ba0], 0 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[ba1], 1 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[ba2], 2 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[ba3], 3 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[ba8], 4 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[ba9], 5 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[baA], 6 );
			btl = _mm_insert_epi16( btl, *(const unsigned short *)&localSrcBlue[baB], 7 );

			__m128i bth = _mm_setzero_si128();
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[ba4], 0 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[ba5], 1 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[ba6], 2 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[ba7], 3 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[baC], 4 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[baD], 5 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[baE], 6 );
			bth = _mm_insert_epi16( bth, *(const unsigned short *)&localSrcBlue[baF], 7 );

			__m128i bbl = _mm_setzero_si128();
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba0], 0 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba1], 1 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba2], 2 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba3], 3 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba8], 4 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba9], 5 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + baA], 6 );
			bbl = _mm_insert_epi16( bbl, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + baB], 7 );

			__m128i bbh = _mm_setzero_si128();
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba4], 0 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba5], 1 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba6], 2 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba7], 3 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + baC], 4 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + baD], 5 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + baE], 6 );
			bbh = _mm_insert_epi16( bbh, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + baF], 7 );

			__m256i rt = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), rtl, 0 ), rth, 1 );
			__m256i rb = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), rbl, 0 ), rbh, 1 );
			__m256i gt = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), gtl, 0 ), gth, 1 );
			__m256i gb = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), gbl, 0 ), gbh, 1 );
			__m256i bt = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), btl, 0 ), bth, 1 );
			__m256i bb = _mm256_inserti128_si256( _mm256_inserti128_si256( _mm256_setzero_si256(), bbl, 0 ), bbh, 1 );
#endif

			__m256i rfx = _mm256_and_si256( rsx, vector256_int16_127 );
			__m256i gfx = _mm256_and_si256( gsx, vector256_int16_127 );
			__m256i bfx = _mm256_and_si256( bsx, vector256_int16_127 );

			__m256i rfxb = _mm256_packus_epi16( rfx, rfx );
			__m256i gfxb = _mm256_packus_epi16( gfx, gfx );
			__m256i bfxb = _mm256_packus_epi16( bfx, bfx );

			__m256i rfxw = _mm256_xor_si256( _mm256_unpacklo_epi8( rfxb, rfxb ), vector256_int16_127 );
			__m256i gfxw = _mm256_xor_si256( _mm256_unpacklo_epi8( gfxb, gfxb ), vector256_int16_127 );
			__m256i bfxw = _mm256_xor_si256( _mm256_unpacklo_epi8( bfxb, bfxb ), vector256_int16_127 );

			rt = _mm256_srli_epi16( _mm256_maddubs_epi16( rt, rfxw ), STP );
			gt = _mm256_srli_epi16( _mm256_maddubs_epi16( gt, gfxw ), STP );
			bt = _mm256_srli_epi16( _mm256_maddubs_epi16( bt, bfxw ), STP );
			rb = _mm256_srli_epi16( _mm256_maddubs_epi16( rb, rfxw ), STP );
			gb = _mm256_srli_epi16( _mm256_maddubs_epi16( gb, gfxw ), STP );
			bb = _mm256_srli_epi16( _mm256_maddubs_epi16( bb, bfxw ), STP );

			__m256i rfy = _mm256_and_si256( rsy, vector256_int16_127 );
			__m256i gfy = _mm256_and_si256( gsy, vector256_int16_127 );
			__m256i bfy = _mm256_and_si256( bsy, vector256_int16_127 );

			rt = _mm256_add_epi16( rt, _mm256_srai_epi16( _mm256_mullo_epi16( _mm256_sub_epi16( rb, rt ), rfy ), STP ) );
			gt = _mm256_add_epi16( gt, _mm256_srai_epi16( _mm256_mullo_epi16( _mm256_sub_epi16( gb, gt ), gfy ), STP ) );
			bt = _mm256_add_epi16( bt, _mm256_srai_epi16( _mm256_mullo_epi16( _mm256_sub_epi16( bb, bt ), bfy ), STP ) );

			rt = _mm256_packus_epi16( rt, rt );
			gt = _mm256_packus_epi16( gt, gt );
			bt = _mm256_packus_epi16( bt, bt );
			__m256i at = _mm256_setzero_si256();

			__m256i s0 = _mm256_unpacklo_epi8( rt, gt );		// r0, g0, r1, g1, r2, g2, r3, g3, r4, g4, r5, g5, r6, g6, r7, g7
			__m256i s1 = _mm256_unpacklo_epi8( bt, at );		// b0, a0, b1, a1, b2, a2, b3, a3, b4, a4, b5, a5, b6, a6, b7, a7
			__m256i s2 = _mm256_unpacklo_epi16( s0, s1 );		// r0, g0, b0, a0, r1, g1, b1, a1, r2, g2, b2, a2, r3, g3, b3, a3
			__m256i s3 = _mm256_unpackhi_epi16( s0, s1 );		// r4, g4, b4, a4, r5, g5, b5, a5, r6, g6, b6, a6, r7, g7, b7, a7

			_mm256_stream_si256( (__m256i *)( destRow + x + 0 ), s2 );
			_mm256_stream_si256( (__m256i *)( destRow + x + 8 ), s3 );

			rsx = _mm256_add_epi16( rsx, rdx );
			gsx = _mm256_add_epi16( gsx, gdx );
			bsx = _mm256_add_epi16( bsx, bdx );

			rsy = _mm256_add_epi16( rsy, rdy );
			gsy = _mm256_add_epi16( gsy, gdy );
			bsy = _mm256_add_epi16( bsy, bdy );
		}

#elif defined( __USE_SSE4__ )

		// This version uses PMADDUBSW which unfortunately multiplies an unsigned byte with a *signed* byte.
		// As a result, any fraction over 127 will be interpreted as a negative fraction. To keep the
		// fractions positive, the sub-texel precision is reduced to just 7 bits and there is a 1/128 loss
		// in brightness when interpolating horizontally because the fraction 1.0 (128) cannot be used.

		__m128i rsx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcRedX8 ), 0 ), 0 );
		__m128i rsy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcRedY8 ), 0 ), 0 );
		__m128i rdx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaRedX8 ), 0 ), 0 );
		__m128i rdy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaRedY8 ), 0 ), 0 );

		__m128i gsx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcGreenX8 ), 0 ), 0 );
		__m128i gsy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcGreenY8 ), 0 ), 0 );
		__m128i gdx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaGreenX8 ), 0 ), 0 );
		__m128i gdy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaGreenY8 ), 0 ), 0 );

		__m128i bsx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcBlueX8 ), 0 ), 0 );
		__m128i bsy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcBlueY8 ), 0 ), 0 );
		__m128i bdx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaBlueX8 ), 0 ), 0 );
		__m128i bdy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaBlueY8 ), 0 ), 0 );

		rsx = _mm_add_epi16( rsx, _mm_mullo_epi16( rdx, vector_int16_01234567 ) );
		rsy = _mm_add_epi16( rsy, _mm_mullo_epi16( rdy, vector_int16_01234567 ) );
		rdx = _mm_slli_epi16( rdx, 3 );
		rdy = _mm_slli_epi16( rdy, 3 );

		gsx = _mm_add_epi16( gsx, _mm_mullo_epi16( gdx, vector_int16_01234567 ) );
		gsy = _mm_add_epi16( gsy, _mm_mullo_epi16( gdy, vector_int16_01234567 ) );
		gdx = _mm_slli_epi16( gdx, 3 );
		gdy = _mm_slli_epi16( gdy, 3 );

		bsx = _mm_add_epi16( bsx, _mm_mullo_epi16( bdx, vector_int16_01234567 ) );
		bsy = _mm_add_epi16( bsy, _mm_mullo_epi16( bdy, vector_int16_01234567 ) );
		bdx = _mm_slli_epi16( bdx, 3 );
		bdy = _mm_slli_epi16( bdy, 3 );

		__m128i pitch = _mm_unpacklo_epi16( _mm_shufflelo_epi16( _mm_cvtsi32_si128( srcPitchInTexels ), 0 ), vector_int16_1 );

		for ( int x = 0; x < 32; x += 8 )
		{
			__m128i rax = _mm_srai_epi16( rsx, STP );
			__m128i ray = _mm_srai_epi16( rsy, STP );
			__m128i gax = _mm_srai_epi16( gsx, STP );
			__m128i gay = _mm_srai_epi16( gsy, STP );
			__m128i bax = _mm_srai_epi16( bsx, STP );
			__m128i bay = _mm_srai_epi16( bsy, STP );

			__m128i rof0 = _mm_madd_epi16( _mm_unpacklo_epi16( ray, rax ), pitch );
			__m128i rof1 = _mm_madd_epi16( _mm_unpackhi_epi16( ray, rax ), pitch );
			__m128i gof0 = _mm_madd_epi16( _mm_unpacklo_epi16( gay, gax ), pitch );
			__m128i gof1 = _mm_madd_epi16( _mm_unpackhi_epi16( gay, gax ), pitch );
			__m128i bof0 = _mm_madd_epi16( _mm_unpacklo_epi16( bay, bax ), pitch );
			__m128i bof1 = _mm_madd_epi16( _mm_unpackhi_epi16( bay, bax ), pitch );

			const unsigned int ra0 = _mm_extract_epi32( rof0, 0 );
			const unsigned int ra1 = _mm_extract_epi32( rof0, 1 );
			const unsigned int ra2 = _mm_extract_epi32( rof0, 2 );
			const unsigned int ra3 = _mm_extract_epi32( rof0, 3 );
			const unsigned int ra4 = _mm_extract_epi32( rof1, 0 );
			const unsigned int ra5 = _mm_extract_epi32( rof1, 1 );
			const unsigned int ra6 = _mm_extract_epi32( rof1, 2 );
			const unsigned int ra7 = _mm_extract_epi32( rof1, 3 );

			__m128i rt = _mm_setzero_si128();
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra0], 0 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra1], 1 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra2], 2 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra3], 3 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra4], 4 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra5], 5 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra6], 6 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra7], 7 );

			__m128i rb = _mm_setzero_si128();
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra0], 0 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra1], 1 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra2], 2 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra3], 3 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra4], 4 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra5], 5 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra6], 6 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra7], 7 );

			const unsigned int ga0 = _mm_extract_epi32( gof0, 0 );
			const unsigned int ga1 = _mm_extract_epi32( gof0, 1 );
			const unsigned int ga2 = _mm_extract_epi32( gof0, 2 );
			const unsigned int ga3 = _mm_extract_epi32( gof0, 3 );
			const unsigned int ga4 = _mm_extract_epi32( gof1, 0 );
			const unsigned int ga5 = _mm_extract_epi32( gof1, 1 );
			const unsigned int ga6 = _mm_extract_epi32( gof1, 2 );
			const unsigned int ga7 = _mm_extract_epi32( gof1, 3 );

			__m128i gt = _mm_setzero_si128();
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga0], 0 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga1], 1 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga2], 2 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga3], 3 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga4], 4 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga5], 5 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga6], 6 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga7], 7 );

			__m128i gb = _mm_setzero_si128();
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga0], 0 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga1], 1 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga2], 2 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga3], 3 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga4], 4 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga5], 5 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga6], 6 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga7], 7 );

			const unsigned int ba0 = _mm_extract_epi32( bof0, 0 );
			const unsigned int ba1 = _mm_extract_epi32( bof0, 1 );
			const unsigned int ba2 = _mm_extract_epi32( bof0, 2 );
			const unsigned int ba3 = _mm_extract_epi32( bof0, 3 );
			const unsigned int ba4 = _mm_extract_epi32( bof1, 0 );
			const unsigned int ba5 = _mm_extract_epi32( bof1, 1 );
			const unsigned int ba6 = _mm_extract_epi32( bof1, 2 );
			const unsigned int ba7 = _mm_extract_epi32( bof1, 3 );

			__m128i bt = _mm_setzero_si128();
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba0], 0 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba1], 1 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba2], 2 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba3], 3 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba4], 4 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba5], 5 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba6], 6 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba7], 7 );

			__m128i bb = _mm_setzero_si128();
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba0], 0 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba1], 1 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba2], 2 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba3], 3 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba4], 4 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba5], 5 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba6], 6 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba7], 7 );

			__m128i rfx = _mm_and_si128( rsx, vector_int16_127 );
			__m128i gfx = _mm_and_si128( gsx, vector_int16_127 );
			__m128i bfx = _mm_and_si128( bsx, vector_int16_127 );

			__m128i rfxb = _mm_packus_epi16( rfx, rfx );
			__m128i gfxb = _mm_packus_epi16( gfx, gfx );
			__m128i bfxb = _mm_packus_epi16( bfx, bfx );

			__m128i rfxw = _mm_xor_si128( _mm_unpacklo_epi8( rfxb, rfxb ), vector_int16_127 );
			__m128i gfxw = _mm_xor_si128( _mm_unpacklo_epi8( gfxb, gfxb ), vector_int16_127 );
			__m128i bfxw = _mm_xor_si128( _mm_unpacklo_epi8( bfxb, bfxb ), vector_int16_127 );

			rt = _mm_srli_epi16( _mm_maddubs_epi16( rt, rfxw ), STP );
			gt = _mm_srli_epi16( _mm_maddubs_epi16( gt, gfxw ), STP );
			bt = _mm_srli_epi16( _mm_maddubs_epi16( bt, bfxw ), STP );
			rb = _mm_srli_epi16( _mm_maddubs_epi16( rb, rfxw ), STP );
			gb = _mm_srli_epi16( _mm_maddubs_epi16( gb, gfxw ), STP );
			bb = _mm_srli_epi16( _mm_maddubs_epi16( bb, bfxw ), STP );

			__m128i rfy = _mm_and_si128( rsy, vector_int16_127 );
			__m128i gfy = _mm_and_si128( gsy, vector_int16_127 );
			__m128i bfy = _mm_and_si128( bsy, vector_int16_127 );

			rt = _mm_add_epi16( rt, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( rb, rt ), rfy ), STP ) );
			gt = _mm_add_epi16( gt, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( gb, gt ), gfy ), STP ) );
			bt = _mm_add_epi16( bt, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( bb, bt ), bfy ), STP ) );

			rt = _mm_packus_epi16( rt, rt );
			gt = _mm_packus_epi16( gt, gt );
			bt = _mm_packus_epi16( bt, bt );
			__m128i at = _mm_setzero_si128();

			__m128i s0 = _mm_unpacklo_epi8( rt, gt );		// r0, g0, r1, g1, r2, g2, r3, g3, r4, g4, r5, g5, r6, g6, r7, g7
			__m128i s1 = _mm_unpacklo_epi8( bt, at );		// b0, a0, b1, a1, b2, a2, b3, a3, b4, a4, b5, a5, b6, a6, b7, a7
			__m128i s2 = _mm_unpacklo_epi16( s0, s1 );		// r0, g0, b0, a0, r1, g1, b1, a1, r2, g2, b2, a2, r3, g3, b3, a3
			__m128i s3 = _mm_unpackhi_epi16( s0, s1 );		// r4, g4, b4, a4, r5, g5, b5, a5, r6, g6, b6, a6, r7, g7, b7, a7

			_mm_stream_si128( (__m128i *)( destRow + x + 0 ), s2 );
			_mm_stream_si128( (__m128i *)( destRow + x + 4 ), s3 );

			rsx = _mm_add_epi16( rsx, rdx );
			gsx = _mm_add_epi16( gsx, gdx );
			bsx = _mm_add_epi16( bsx, bdx );

			rsy = _mm_add_epi16( rsy, rdy );
			gsy = _mm_add_epi16( gsy, gdy );
			bsy = _mm_add_epi16( bsy, bdy );
		}

#elif defined( __USE_SSE2__ )

		__m128i rsx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcRedX8 ), 0 ), 0 );
		__m128i rsy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcRedY8 ), 0 ), 0 );
		__m128i rdx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaRedX8 ), 0 ), 0 );
		__m128i rdy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaRedY8 ), 0 ), 0 );

		__m128i gsx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcGreenX8 ), 0 ), 0 );
		__m128i gsy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcGreenY8 ), 0 ), 0 );
		__m128i gdx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaGreenX8 ), 0 ), 0 );
		__m128i gdy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaGreenY8 ), 0 ), 0 );

		__m128i bsx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcBlueX8 ), 0 ), 0 );
		__m128i bsy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( localSrcBlueY8 ), 0 ), 0 );
		__m128i bdx = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaBlueX8 ), 0 ), 0 );
		__m128i bdy = _mm_shuffle_epi32( _mm_shufflelo_epi16( _mm_cvtsi32_si128( deltaBlueY8 ), 0 ), 0 );

		rsx = _mm_add_epi16( rsx, _mm_mullo_epi16( rdx, vector_int16_01234567 ) );
		rsy = _mm_add_epi16( rsy, _mm_mullo_epi16( rdy, vector_int16_01234567 ) );
		rdx = _mm_slli_epi16( rdx, 3 );
		rdy = _mm_slli_epi16( rdy, 3 );

		gsx = _mm_add_epi16( gsx, _mm_mullo_epi16( gdx, vector_int16_01234567 ) );
		gsy = _mm_add_epi16( gsy, _mm_mullo_epi16( gdy, vector_int16_01234567 ) );
		gdx = _mm_slli_epi16( gdx, 3 );
		gdy = _mm_slli_epi16( gdy, 3 );

		bsx = _mm_add_epi16( bsx, _mm_mullo_epi16( bdx, vector_int16_01234567 ) );
		bsy = _mm_add_epi16( bsy, _mm_mullo_epi16( bdy, vector_int16_01234567 ) );
		bdx = _mm_slli_epi16( bdx, 3 );
		bdy = _mm_slli_epi16( bdy, 3 );

		__m128i pitch = _mm_unpacklo_epi16( _mm_shufflelo_epi16( _mm_cvtsi32_si128( srcPitchInTexels ), 0 ), vector_int16_1 );

		for ( int x = 0; x < 32; x += 8 )
		{
			__m128i rax = _mm_srai_epi16( rsx, STP );
			__m128i ray = _mm_srai_epi16( rsy, STP );
			__m128i gax = _mm_srai_epi16( gsx, STP );
			__m128i gay = _mm_srai_epi16( gsy, STP );
			__m128i bax = _mm_srai_epi16( bsx, STP );
			__m128i bay = _mm_srai_epi16( bsy, STP );

			__m128i rof0 = _mm_madd_epi16( _mm_unpacklo_epi16( ray, rax ), pitch );
			__m128i rof1 = _mm_madd_epi16( _mm_unpackhi_epi16( ray, rax ), pitch );
			__m128i gof0 = _mm_madd_epi16( _mm_unpacklo_epi16( gay, gax ), pitch );
			__m128i gof1 = _mm_madd_epi16( _mm_unpackhi_epi16( gay, gax ), pitch );
			__m128i bof0 = _mm_madd_epi16( _mm_unpacklo_epi16( bay, bax ), pitch );
			__m128i bof1 = _mm_madd_epi16( _mm_unpackhi_epi16( bay, bax ), pitch );

			const unsigned int ra0 = _mm_cvtsi128_si32( _mm_shuffle_epi32( rof0, 0 ) );
			const unsigned int ra1 = _mm_cvtsi128_si32( _mm_shuffle_epi32( rof0, 1 ) );
			const unsigned int ra2 = _mm_cvtsi128_si32( _mm_shuffle_epi32( rof0, 2 ) );
			const unsigned int ra3 = _mm_cvtsi128_si32( _mm_shuffle_epi32( rof0, 3 ) );
			const unsigned int ra4 = _mm_cvtsi128_si32( _mm_shuffle_epi32( rof1, 0 ) );
			const unsigned int ra5 = _mm_cvtsi128_si32( _mm_shuffle_epi32( rof1, 1 ) );
			const unsigned int ra6 = _mm_cvtsi128_si32( _mm_shuffle_epi32( rof1, 2 ) );
			const unsigned int ra7 = _mm_cvtsi128_si32( _mm_shuffle_epi32( rof1, 3 ) );

			__m128i rt = _mm_setzero_si128();
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra0], 0 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra1], 1 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra2], 2 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra3], 3 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra4], 4 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra5], 5 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra6], 6 );
			rt = _mm_insert_epi16( rt, *(const unsigned short *)&localSrcRed[ra7], 7 );

			__m128i rb = _mm_setzero_si128();
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra0], 0 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra1], 1 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra2], 2 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra3], 3 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra4], 4 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra5], 5 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra6], 6 );
			rb = _mm_insert_epi16( rb, *(const unsigned short *)&localSrcRed[srcPitchInTexels + ra7], 7 );

			const unsigned int ga0 = _mm_cvtsi128_si32( _mm_shuffle_epi32( gof0, 0 ) );
			const unsigned int ga1 = _mm_cvtsi128_si32( _mm_shuffle_epi32( gof0, 1 ) );
			const unsigned int ga2 = _mm_cvtsi128_si32( _mm_shuffle_epi32( gof0, 2 ) );
			const unsigned int ga3 = _mm_cvtsi128_si32( _mm_shuffle_epi32( gof0, 3 ) );
			const unsigned int ga4 = _mm_cvtsi128_si32( _mm_shuffle_epi32( gof1, 0 ) );
			const unsigned int ga5 = _mm_cvtsi128_si32( _mm_shuffle_epi32( gof1, 1 ) );
			const unsigned int ga6 = _mm_cvtsi128_si32( _mm_shuffle_epi32( gof1, 2 ) );
			const unsigned int ga7 = _mm_cvtsi128_si32( _mm_shuffle_epi32( gof1, 3 ) );

			__m128i gt = _mm_setzero_si128();
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga0], 0 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga1], 1 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga2], 2 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga3], 3 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga4], 4 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga5], 5 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga6], 6 );
			gt = _mm_insert_epi16( gt, *(const unsigned short *)&localSrcGreen[ga7], 7 );

			__m128i gb = _mm_setzero_si128();
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga0], 0 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga1], 1 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga2], 2 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga3], 3 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga4], 4 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga5], 5 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga6], 6 );
			gb = _mm_insert_epi16( gb, *(const unsigned short *)&localSrcGreen[srcPitchInTexels + ga7], 7 );

			const unsigned int ba0 = _mm_cvtsi128_si32( _mm_shuffle_epi32( bof0, 0 ) );
			const unsigned int ba1 = _mm_cvtsi128_si32( _mm_shuffle_epi32( bof0, 1 ) );
			const unsigned int ba2 = _mm_cvtsi128_si32( _mm_shuffle_epi32( bof0, 2 ) );
			const unsigned int ba3 = _mm_cvtsi128_si32( _mm_shuffle_epi32( bof0, 3 ) );
			const unsigned int ba4 = _mm_cvtsi128_si32( _mm_shuffle_epi32( bof1, 0 ) );
			const unsigned int ba5 = _mm_cvtsi128_si32( _mm_shuffle_epi32( bof1, 1 ) );
			const unsigned int ba6 = _mm_cvtsi128_si32( _mm_shuffle_epi32( bof1, 2 ) );
			const unsigned int ba7 = _mm_cvtsi128_si32( _mm_shuffle_epi32( bof1, 3 ) );

			__m128i bt = _mm_setzero_si128();
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba0], 0 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba1], 1 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba2], 2 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba3], 3 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba4], 4 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba5], 5 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba6], 6 );
			bt = _mm_insert_epi16( bt, *(const unsigned short *)&localSrcBlue[ba7], 7 );

			__m128i bb = _mm_setzero_si128();
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba0], 0 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba1], 1 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba2], 2 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba3], 3 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba4], 4 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba5], 5 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba6], 6 );
			bb = _mm_insert_epi16( bb, *(const unsigned short *)&localSrcBlue[srcPitchInTexels + ba7], 7 );

			__m128i rfx = _mm_and_si128( rsx, vector_int16_255 );
			__m128i gfx = _mm_and_si128( gsx, vector_int16_255 );
			__m128i bfx = _mm_and_si128( bsx, vector_int16_255 );

			__m128i rfxl = _mm_add_epi32( _mm_xor_si128( _mm_unpacklo_epi16( rfx, rfx ), vector_int32_255 ), vector_int32_1 );
			__m128i gfxl = _mm_add_epi32( _mm_xor_si128( _mm_unpacklo_epi16( gfx, gfx ), vector_int32_255 ), vector_int32_1 );
			__m128i bfxl = _mm_add_epi32( _mm_xor_si128( _mm_unpacklo_epi16( bfx, bfx ), vector_int32_255 ), vector_int32_1 );
			__m128i rfxh = _mm_add_epi32( _mm_xor_si128( _mm_unpackhi_epi16( rfx, rfx ), vector_int32_255 ), vector_int32_1 );
			__m128i gfxh = _mm_add_epi32( _mm_xor_si128( _mm_unpackhi_epi16( gfx, gfx ), vector_int32_255 ), vector_int32_1 );
			__m128i bfxh = _mm_add_epi32( _mm_xor_si128( _mm_unpackhi_epi16( bfx, bfx ), vector_int32_255 ), vector_int32_1 );

			__m128i rtl = _mm_srli_epi16( _mm_madd_epi16( _mm_unpacklo_epi8( rt, vector_uint8_0 ), rfxl ), STP );
			__m128i gtl = _mm_srli_epi16( _mm_madd_epi16( _mm_unpacklo_epi8( gt, vector_uint8_0 ), gfxl ), STP );
			__m128i btl = _mm_srli_epi16( _mm_madd_epi16( _mm_unpacklo_epi8( bt, vector_uint8_0 ), bfxl ), STP );
			__m128i rth = _mm_srli_epi16( _mm_madd_epi16( _mm_unpackhi_epi8( rt, vector_uint8_0 ), rfxh ), STP );
			__m128i gth = _mm_srli_epi16( _mm_madd_epi16( _mm_unpackhi_epi8( gt, vector_uint8_0 ), gfxh ), STP );
			__m128i bth = _mm_srli_epi16( _mm_madd_epi16( _mm_unpackhi_epi8( bt, vector_uint8_0 ), bfxh ), STP );

			__m128i rbl = _mm_srli_epi16( _mm_madd_epi16( _mm_unpacklo_epi8( rb, vector_uint8_0 ), rfxl ), STP );
			__m128i gbl = _mm_srli_epi16( _mm_madd_epi16( _mm_unpacklo_epi8( gb, vector_uint8_0 ), gfxl ), STP );
			__m128i bbl = _mm_srli_epi16( _mm_madd_epi16( _mm_unpacklo_epi8( bb, vector_uint8_0 ), bfxl ), STP );
			__m128i rbh = _mm_srli_epi16( _mm_madd_epi16( _mm_unpackhi_epi8( rb, vector_uint8_0 ), rfxh ), STP );
			__m128i gbh = _mm_srli_epi16( _mm_madd_epi16( _mm_unpackhi_epi8( gb, vector_uint8_0 ), gfxh ), STP );
			__m128i bbh = _mm_srli_epi16( _mm_madd_epi16( _mm_unpackhi_epi8( bb, vector_uint8_0 ), bfxh ), STP );
			
			rt = _mm_packs_epi32( rtl, rth );
			gt = _mm_packs_epi32( gtl, gth );
			bt = _mm_packs_epi32( btl, bth );
			rb = _mm_packs_epi32( rbl, rbh );
			gb = _mm_packs_epi32( gbl, gbh );
			bb = _mm_packs_epi32( bbl, bbh );

			__m128i rfy = _mm_and_si128( rsy, vector_int16_255 );
			__m128i gfy = _mm_and_si128( gsy, vector_int16_255 );
			__m128i bfy = _mm_and_si128( bsy, vector_int16_255 );

			rt = _mm_add_epi16( rt, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( rb, rt ), rfy ), STP ) );
			gt = _mm_add_epi16( gt, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( gb, gt ), gfy ), STP ) );
			bt = _mm_add_epi16( bt, _mm_srai_epi16( _mm_mullo_epi16( _mm_sub_epi16( bb, bt ), bfy ), STP ) );

			rt = _mm_packus_epi16( rt, rt );
			gt = _mm_packus_epi16( gt, gt );
			bt = _mm_packus_epi16( bt, bt );
			__m128i at = _mm_setzero_si128();

			__m128i s0 = _mm_unpacklo_epi8( rt, gt );		// r0, g0, r1, g1, r2, g2, r3, g3, r4, g4, r5, g5, r6, g6, r7, g7
			__m128i s1 = _mm_unpacklo_epi8( bt, at );		// b0, a0, b1, a1, b2, a2, b3, a3, b4, a4, b5, a5, b6, a6, b7, a7
			__m128i s2 = _mm_unpacklo_epi16( s0, s1 );		// r0, g0, b0, a0, r1, g1, b1, a1, r2, g2, b2, a2, r3, g3, b3, a3
			__m128i s3 = _mm_unpackhi_epi16( s0, s1 );		// r4, g4, b4, a4, r5, g5, b5, a5, r6, g6, b6, a6, r7, g7, b7, a7

			_mm_stream_si128( (__m128i *)(destRow + x + 0), s2 );
			_mm_stream_si128( (__m128i *)(destRow + x + 4), s3 );

			rsx = _mm_add_epi16( rsx, rdx );
			gsx = _mm_add_epi16( gsx, gdx );
			bsx = _mm_add_epi16( bsx, bdx );

			rsy = _mm_add_epi16( rsy, rdy );
			gsy = _mm_add_epi16( gsy, gdy );
			bsy = _mm_add_epi16( bsy, bdy );
		}

#elif defined( __ARM_NEON__ )		// increased throughput

		int deltaRedXY = ( deltaRedY8 << 16 ) | ( deltaRedX8 & 0xFFFF );
		int deltaGreenXY = ( deltaGreenY8 << 16 ) | ( deltaGreenX8 & 0xFFFF );
		int deltaBlueXY = ( deltaBlueY8 << 16 ) | ( deltaBlueX8 & 0xFFFF );

		int localSrcRedXY = ( localSrcRedY8 << 16 ) | ( localSrcRedX8 & 0xFFFF );
		int localSrcGreenXY = ( localSrcGreenY8 << 16 ) | ( localSrcGreenX8 & 0xFFFF );
		int localSrcBlueXY = ( localSrcBlueY8 << 16 ) | ( localSrcBlueX8 & 0xFFFF );

		__asm__ volatile(
			"	movw		r0, #0x0100									\n\t"	// r0 = 0x00000100
			"	movt		r0, #0x0302									\n\t"	// r0 = 0x03020100
			"	movw		r1, #0x0504									\n\t"	// r1 = 0x00000504
			"	movt		r1, #0x0706									\n\t"	// r1 = 0x07060504
			"	vmov.u32	d24[0], r0									\n\t"	// d24 = 0x0706050403020100
			"	vmov.u32	d24[1], r1									\n\t"	// d24 = 0x0706050403020100
			"	vmov.u32	d25, d24									\n\t"	// d25 = 0x0706050403020100

			"	vdup.u8		d1, %[srxy]									\n\t"	//    srcRedX,    srcRedX,    srcRedX,    srcRedX,    srcRedX,    srcRedX,    srcRedX,    srcRedX
			"	vmov.u8		d0, #0xFF									\n\t"	//       0xFF,       0xFF,       0xFF,       0xFF,       0xFF,       0xFF,       0xFF,       0xFF
			"	veor.u8		d0, d0, d1									\n\t"	// srcRedX^-1, srcRedX^-1, srcRedX^-1, srcRedX^-1, srcRedX^-1, srcRedX^-1, srcRedX^-1, srcRedX^-1
			"	vdup.u8		d13, %[drxy]								\n\t"	//  deltaRedX,  deltaRedX,  deltaRedX,  deltaRedX,  deltaRedX,  deltaRedX,  deltaRedX,  deltaRedX
			"	vmov.u8		d12, #0x00									\n\t"	//       0x00,       0x00,       0x00,       0x00,       0x00,       0x00,       0x00,       0x00
			"	vsub.u8		d12, d12, d13								\n\t"	// -deltaRedX, -deltaRedX, -deltaRedX, -deltaRedX, -deltaRedX, -deltaRedX, -deltaRedX, -deltaRedX
			"	vmla.u8		q0, q6, q12									\n\t"	// fracRedX
			"	vshl.u8		q6, q6, #3									\n\t"	// incRedX

			"	lsr			r0, %[srxy], #16							\n\t"
			"	lsr			r1, %[drxy], #16							\n\t"

			"	vdup.u8		d3, r0										\n\t"	//    srcRedY,    srcRedY,    srcRedY,    srcRedY,    srcRedY,    srcRedY,    srcRedY,    srcRedY
			"	vmov.u8		d2, #0xFF									\n\t"	//       0xFF,       0xFF,       0xFF,       0xFF,       0xFF,       0xFF,       0xFF,       0xFF
			"	veor.u8		d2, d2, d3									\n\t"	// srcRedY^-1, srcRedY^-1, srcRedY^-1, srcRedY^-1, srcRedY^-1, srcRedY^-1, srcRedY^-1, srcRedY^-1
			"	vdup.u8		d15, r1										\n\t"	//  deltaRedY,  deltaRedY,  deltaRedY,  deltaRedY,  deltaRedY,  deltaRedY,  deltaRedY,  deltaRedY
			"	vmov.u8		d14, #0x00									\n\t"	//       0x00,       0x00,       0x00,       0x00,       0x00,       0x00,       0x00,       0x00
			"	vsub.u8		d14, d14, d15								\n\t"	// -deltaRedY, -deltaRedY, -deltaRedY, -deltaRedY, -deltaRedY, -deltaRedY, -deltaRedY, -deltaRedY
			"	vmla.u8		q1, q7, q12									\n\t"	// fracRedY
			"	vshl.u8		q7, q7, #3									\n\t"	// incRedY

			"	vdup.u8		d5, %[sgxy]									\n\t"	//    srcGreenX,    srcGreenX,    srcGreenX,    srcGreenX,    srcGreenX,    srcGreenX,    srcGreenX,    srcGreenX
			"	vmov.u8		d4, #0xFF									\n\t"	//         0xFF,         0xFF,         0xFF,         0xFF,         0xFF,         0xFF,         0xFF,         0xFF
			"	veor.u8		d4, d4, d5									\n\t"	// srcGreenX^-1, srcGreenX^-1, srcGreenX^-1, srcGreenX^-1, srcGreenX^-1, srcGreenX^-1, srcGreenX^-1, srcGreenX^-1
			"	vdup.u8		d17, %[dgxy]								\n\t"	//  deltaGreenX,  deltaGreenX,  deltaGreenX,  deltaGreenX,  deltaGreenX,  deltaGreenX,  deltaGreenX,  deltaGreenX
			"	vmov.u8		d16, #0x00									\n\t"	//         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00
			"	vsub.u8		d16, d16, d17								\n\t"	// -deltaGreenX, -deltaGreenX, -deltaGreenX, -deltaGreenX, -deltaGreenX, -deltaGreenX, -deltaGreenX, -deltaGreenX
			"	vmla.u8		q2, q8, q12									\n\t"	// fracGreenX
			"	vshl.u8		q8, q8, #3									\n\t"	// incGreenX

			"	lsr			r0, %[sgxy], #16							\n\t"
			"	lsr			r1, %[dgxy], #16							\n\t"

			"	vdup.u8		d7, r0										\n\t"	//    srcGreenY,    srcGreenY,    srcGreenY,    srcGreenY,    srcGreenY,    srcGreenY,    srcGreenY,    srcGreenY
			"	vmov.u8		d6, #0xFF									\n\t"	//         0xFF,         0xFF,         0xFF,         0xFF,         0xFF,         0xFF,         0xFF,         0xFF
			"	veor.u8		d6, d6, d7									\n\t"	// srcGreenY^-1, srcGreenY^-1, srcGreenY^-1, srcGreenY^-1, srcGreenY^-1, srcGreenY^-1, srcGreenY^-1, srcGreenY^-1
			"	vdup.u8		d19, r1										\n\t"	//  deltaGreenY,  deltaGreenY,  deltaGreenY,  deltaGreenY,  deltaGreenY,  deltaGreenY,  deltaGreenY,  deltaGreenY
			"	vmov.u8		d18, #0x00									\n\t"	//         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00
			"	vsub.u8		d18, d18, d19								\n\t"	// -deltaGreenY, -deltaGreenY, -deltaGreenY, -deltaGreenY, -deltaGreenY, -deltaGreenY, -deltaGreenY, -deltaGreenY
			"	vmla.u8		q3, q9, q12									\n\t"	// fracGreenY
			"	vshl.u8		q9, q9, #3									\n\t"	// incGreenY

			"	vdup.u8		d9, %[sbxy]									\n\t"	//    srcBlueX,    srcBlueX,    srcBlueX,    srcBlueX,    srcBlueX,    srcBlueX,    srcBlueX,    srcBlueX
			"	vmov.u8		d8, #0xFF									\n\t"	//        0xFF,        0xFF,        0xFF,        0xFF,        0xFF,        0xFF,        0xFF,        0xFF
			"	veor.u8		d8, d8, d9									\n\t"	// srcBlueX^-1, srcBlueX^-1, srcBlueX^-1, srcBlueX^-1, srcBlueX^-1, srcBlueX^-1, srcBlueX^-1, srcBlueX^-1
			"	vdup.u8		d21, %[dbxy]								\n\t"	//  deltaBlueX,  deltaBlueX,  deltaBlueX,  deltaBlueX,  deltaBlueX,  deltaBlueX,  deltaBlueX,  deltaBlueX
			"	vmov.u8		d20, #0x00									\n\t"	//        0x00,        0x00,        0x00,        0x00,        0x00,        0x00,        0x00,        0x00
			"	vsub.u8		d20, d20, d21								\n\t"	// -deltaBlueX, -deltaBlueX, -deltaBlueX, -deltaBlueX, -deltaBlueX, -deltaBlueX, -deltaBlueX, -deltaBlueX
			"	vmla.u8		q4, q10, q12								\n\t"	// fracBlueX
			"	vshl.u8		q10, q10, #3								\n\t"	// incBlueX

			"	lsr			r0, %[sbxy], #16							\n\t"
			"	lsr			r1, %[dbxy], #16							\n\t"

			"	vdup.u8		d11, r0										\n\t"	//    srcBlueY,    srcBlueY,    srcBlueY,    srcBlueY,    srcBlueY,    srcBlueY,    srcBlueY,    srcBlueY
			"	vmov.u8		d10, #0xFF									\n\t"	//        0xFF,        0xFF,        0xFF,        0xFF,        0xFF,        0xFF,        0xFF,        0xFF
			"	veor.u8		d10, d10, d11								\n\t"	// srcBlueY^-1, srcBlueY^-1, srcBlueY^-1, srcBlueY^-1, srcBlueY^-1, srcBlueY^-1, srcBlueY^-1, srcBlueY^-1
			"	vdup.u8		d23, r1										\n\t"	//  deltaBlueY,  deltaBlueY,  deltaBlueY,  deltaBlueY,  deltaBlueY,  deltaBlueY,  deltaBlueY,  deltaBlueY
			"	vmov.u8		d22, #0x00									\n\t"	//        0x00,        0x00,        0x00,        0x00,        0x00,        0x00,        0x00,        0x00
			"	vsub.u8		d22, d22, d23								\n\t"	// -deltaBlueY, -deltaBlueY, -deltaBlueY, -deltaBlueY, -deltaBlueY, -deltaBlueY, -deltaBlueY, -deltaBlueY
			"	vmla.u8		 q5, q11, q12								\n\t"	// fracBlueY
			"	vshl.u8		q11, q11, #3								\n\t"	// incBlueY

			"	sub			sp, sp, #64									\n\t"	// reserve stack space

			"	vstm		sp, {q9-q11]								\n\t"	// store q9-q11 on the stack

			"	add			r1, %[d], #128								\n\t"	// destRow + 32
			"	str			r1, [sp, #48]								\n\t"	// store end pointer on the stack

			".LOOP_CHROMATIC_PLANAR_RGB:								\n\t"
			"	sxtb		r0, %[srxy], ror #8							\n\t"	// srcRedX >> 8;
			"	sxtb		r1, %[srxy], ror #24						\n\t"	// srcRedY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );
			"	sadd16		%[srxy], %[srxy], %[drxy]					\n\t"	// (srcRedX, srcRedY) += (deltaRedX, deltaRedY)
			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );

			"	vld2.u8		{d20[0],d21[0]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[0],d23[0]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sgxy], ror #8							\n\t"	// srcGreenX >> 8;
			"	sxtb		r1, %[sgxy], ror #24						\n\t"	// srcGreenY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );
			"	sadd16		%[sgxy], %[sgxy], %[dgxy]					\n\t"	// (srcGreenX, srcGreenY) += (deltaGreenX, deltaGreenY)
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );

			"	vld2.u8		{d24[0],d25[0]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d26[0],d27[0]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sbxy], ror #8							\n\t"	// srcBlueX >> 8;
			"	sxtb		r1, %[sbxy], ror #24						\n\t"	// srcBlueY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );
			"	sadd16		%[sbxy], %[sbxy], %[dbxy]					\n\t"	// (srcBlueX, srcBlueY) += (deltaBlueX, deltaBlueY)
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );

			"	vld2.u8		{d28[0],d29[0]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d30[0],d31[0]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[srxy], ror #8							\n\t"	// srcRedX >> 8;
			"	sxtb		r1, %[srxy], ror #24						\n\t"	// srcRedY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );
			"	sadd16		%[srxy], %[srxy], %[drxy]					\n\t"	// (srcRedX, srcRedY) += (deltaRedX, deltaRedY)
			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );

			"	vld2.u8		{d20[1],d21[1]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[1],d23[1]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sgxy], ror #8							\n\t"	// srcGreenX >> 8;
			"	sxtb		r1, %[sgxy], ror #24						\n\t"	// srcGreenY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );
			"	sadd16		%[sgxy], %[sgxy], %[dgxy]					\n\t"	// (srcGreenX, srcGreenY) += (deltaGreenX, deltaGreenY)
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );

			"	vld2.u8		{d24[1],d25[1]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d26[1],d27[1]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sbxy], ror #8							\n\t"	// srcBlueX >> 8;
			"	sxtb		r1, %[sbxy], ror #24						\n\t"	// srcBlueY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );
			"	sadd16		%[sbxy], %[sbxy], %[dbxy]					\n\t"	// (srcBlueX, srcBlueY) += (deltaBlueX, deltaBlueY)
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );

			"	vld2.u8		{d28[1],d29[1]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d30[1],d31[1]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[srxy], ror #8							\n\t"	// srcRedX >> 8;
			"	sxtb		r1, %[srxy], ror #24						\n\t"	// srcRedY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );
			"	sadd16		%[srxy], %[srxy], %[drxy]					\n\t"	// (srcRedX, srcRedY) += (deltaRedX, deltaRedY)
			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );

			"	vld2.u8		{d20[2],d21[2]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[2],d23[2]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sgxy], ror #8							\n\t"	// srcGreenX >> 8;
			"	sxtb		r1, %[sgxy], ror #24						\n\t"	// srcGreenY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );
			"	sadd16		%[sgxy], %[sgxy], %[dgxy]					\n\t"	// (srcGreenX, srcGreenY) += (deltaGreenX, deltaGreenY)
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );

			"	vld2.u8		{d24[2],d25[2]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d26[2],d27[2]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sbxy], ror #8							\n\t"	// srcBlueX >> 8;
			"	sxtb		r1, %[sbxy], ror #24						\n\t"	// srcBlueY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );
			"	sadd16		%[sbxy], %[sbxy], %[dbxy]					\n\t"	// (srcBlueX, srcBlueY) += (deltaBlueX, deltaBlueY)
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );

			"	vld2.u8		{d28[2],d29[2]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d30[2],d31[2]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[srxy], ror #8							\n\t"	// srcRedX >> 8;
			"	sxtb		r1, %[srxy], ror #24						\n\t"	// srcRedY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );
			"	sadd16		%[srxy], %[srxy], %[drxy]					\n\t"	// (srcRedX, srcRedY) += (deltaRedX, deltaRedY)
			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );

			"	vld2.u8		{d20[3],d21[3]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[3],d23[3]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sgxy], ror #8							\n\t"	// srcGreenX >> 8;
			"	sxtb		r1, %[sgxy], ror #24						\n\t"	// srcGreenY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );
			"	sadd16		%[sgxy], %[sgxy], %[dgxy]					\n\t"	// (srcGreenX, srcGreenY) += (deltaGreenX, deltaGreenY)
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );

			"	vld2.u8		{d24[3],d25[3]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d26[3],d27[3]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sbxy], ror #8							\n\t"	// srcBlueX >> 8;
			"	sxtb		r1, %[sbxy], ror #24						\n\t"	// srcBlueY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );
			"	sadd16		%[sbxy], %[sbxy], %[dbxy]					\n\t"	// (srcBlueX, srcBlueY) += (deltaBlueX, deltaBlueY)
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );

			"	vld2.u8		{d28[3],d29[3]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d30[3],d31[3]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[srxy], ror #8							\n\t"	// srcRedX >> 8;
			"	sxtb		r1, %[srxy], ror #24						\n\t"	// srcRedY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );
			"	sadd16		%[srxy], %[srxy], %[drxy]					\n\t"	// (srcRedX, srcRedY) += (deltaRedX, deltaRedY)
			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );

			"	vld2.u8		{d20[4],d21[4]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[4],d23[4]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sgxy], ror #8							\n\t"	// srcGreenX >> 8;
			"	sxtb		r1, %[sgxy], ror #24						\n\t"	// srcGreenY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );
			"	sadd16		%[sgxy], %[sgxy], %[dgxy]					\n\t"	// (srcGreenX, srcGreenY) += (deltaGreenX, deltaGreenY)
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );

			"	vld2.u8		{d24[4],d25[4]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d26[4],d27[4]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sbxy], ror #8							\n\t"	// srcBlueX >> 8;
			"	sxtb		r1, %[sbxy], ror #24						\n\t"	// srcBlueY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );
			"	sadd16		%[sbxy], %[sbxy], %[dbxy]					\n\t"	// (srcBlueX, srcBlueY) += (deltaBlueX, deltaBlueY)
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );

			"	vld2.u8		{d28[4],d29[4]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d30[4],d31[4]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[srxy], ror #8							\n\t"	// srcRedX >> 8;
			"	sxtb		r1, %[srxy], ror #24						\n\t"	// srcRedY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );
			"	sadd16		%[srxy], %[srxy], %[drxy]					\n\t"	// (srcRedX, srcRedY) += (deltaRedX, deltaRedY)
			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );

			"	vld2.u8		{d20[5],d21[5]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[5],d23[5]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sgxy], ror #8							\n\t"	// srcGreenX >> 8;
			"	sxtb		r1, %[sgxy], ror #24						\n\t"	// srcGreenY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );
			"	sadd16		%[sgxy], %[sgxy], %[dgxy]					\n\t"	// (srcGreenX, srcGreenY) += (deltaGreenX, deltaGreenY)
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );

			"	vld2.u8		{d24[5],d25[5]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d26[5],d27[5]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sbxy], ror #8							\n\t"	// srcBlueX >> 8;
			"	sxtb		r1, %[sbxy], ror #24						\n\t"	// srcBlueY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );
			"	sadd16		%[sbxy], %[sbxy], %[dbxy]					\n\t"	// (srcBlueX, srcBlueY) += (deltaBlueX, deltaBlueY)
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );

			"	vld2.u8		{d28[5],d29[5]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d30[5],d31[5]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[srxy], ror #8							\n\t"	// srcRedX >> 8;
			"	sxtb		r1, %[srxy], ror #24						\n\t"	// srcRedY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );
			"	sadd16		%[srxy], %[srxy], %[drxy]					\n\t"	// (srcRedX, srcRedY) += (deltaRedX, deltaRedY)
			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );

			"	vld2.u8		{d20[6],d21[6]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[6],d23[6]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sgxy], ror #8							\n\t"	// srcGreenX >> 8;
			"	sxtb		r1, %[sgxy], ror #24						\n\t"	// srcGreenY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );
			"	sadd16		%[sgxy], %[sgxy], %[dgxy]					\n\t"	// (srcGreenX, srcGreenY) += (deltaGreenX, deltaGreenY)
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );

			"	vld2.u8		{d24[6],d25[6]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d26[6],d27[6]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sbxy], ror #8							\n\t"	// srcBlueX >> 8;
			"	sxtb		r1, %[sbxy], ror #24						\n\t"	// srcBlueY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );
			"	sadd16		%[sbxy], %[sbxy], %[dbxy]					\n\t"	// (srcBlueX, srcBlueY) += (deltaBlueX, deltaBlueY)
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );

			"	vld2.u8		{d28[6],d29[6]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d30[6],d31[6]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[srxy], ror #8							\n\t"	// srcRedX >> 8;
			"	sxtb		r1, %[srxy], ror #24						\n\t"	// srcRedY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );
			"	sadd16		%[srxy], %[srxy], %[drxy]					\n\t"	// (srcRedX, srcRedY) += (deltaRedX, deltaRedY)
			"	add			r1, %[sr], r0								\n\t"	// localSrcRed + ( srcRedY >> 8 ) * srcPitchInTexels + ( srcRedX >> 8 );

			"	vld2.u8		{d20[7],d21[7]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d22[7],d23[7]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sgxy], ror #8							\n\t"	// srcGreenX >> 8;
			"	sxtb		r1, %[sgxy], ror #24						\n\t"	// srcGreenY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );
			"	sadd16		%[sgxy], %[sgxy], %[dgxy]					\n\t"	// (srcGreenX, srcGreenY) += (deltaGreenX, deltaGreenY)
			"	add			r1, %[sg], r0								\n\t"	// localSrcGreen + ( srcGreenY >> 8 ) * srcPitchInTexels + ( srcGreenX >> 8 );

			"	vld2.u8		{d24[7],d25[7]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d26[7],d27[7]}, [r1], %[p]					\n\t"

			"	sxtb		r0, %[sbxy], ror #8							\n\t"	// srcBlueX >> 8;
			"	sxtb		r1, %[sbxy], ror #24						\n\t"	// srcBlueY >> 8;
			"	mla			r0, r1, %[p], r0							\n\t"	// ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );
			"	sadd16		%[sbxy], %[sbxy], %[dbxy]					\n\t"	// (srcBlueX, srcBlueY) += (deltaBlueX, deltaBlueY)
			"	add			r1, %[sb], r0								\n\t"	// localSrcBlue + ( srcBlueY >> 8 ) * srcPitchInTexels + ( srcBlueX >> 8 );

			"	vld2.u8		{d28[7],d29[7]}, [r1], %[p]					\n\t"
			"	vld2.u8		{d30[7],d31[7]}, [r1], %[p]					\n\t"

			"	mov			r0, #4										\n\t"
			"	ldr			r1, [sp, #48]								\n\t"

			"	vmull.u8	 q9, d20, d2								\n\t"	// weight top-left  red   with (fracRedY^-1)
			"	vmull.u8	q10, d21, d2								\n\t"	// weight top-right red   with (fracRedY^-1)
			"	vmlal.u8	 q9, d22, d3								\n\t"	// weight bottom-left  red   with (fracRedY)   and add to top-left  red
			"	vmlal.u8	q10, d23, d3								\n\t"	// weight bottom-right red   with (fracRedY)   and add to top-right red

			"	vmull.u8	q11, d24, d6								\n\t"	// weight top-left  green with (fracGreenY^-1)
			"	vmull.u8	q12, d25, d6								\n\t"	// weight top-right green with (fracGreenY^-1)
			"	vmlal.u8	q11, d26, d7								\n\t"	// weight bottom-left  green with (fracGreenY) and add to top-left  green
			"	vmlal.u8	q12, d27, d7								\n\t"	// weight bottom-right green with (fracGreenY) and add to top-right green

			"	vmull.u8	q13, d28, d10								\n\t"	// weight top-left  blue  with (fracBlueY^-1)
			"	vmull.u8	q14, d29, d10								\n\t"	// weight top-right blue  with (fracBlueY^-1)
			"	vmlal.u8	q13, d30, d11								\n\t"	// weight bottom-left  blue  with (fracBlueY)  and add to top-left  blue
			"	vmlal.u8	q14, d31, d11								\n\t"	// weight bottom-right blue  with (fracBlueY)  and add to top-right blue

			"	vqrshrn.u16	d18,  q9, #8								\n\t"	// reduce left  red   to 8 bits of precision and half the register size
			"	vqrshrn.u16 d22, q11, #8								\n\t"	// reduce left  green to 8 bits of precision and half the register size
			"	vqrshrn.u16 d26, q13, #8								\n\t"	// reduce left  blue  to 8 bits of precision and half the register size
			"	vqrshrn.u16	d20, q10, #8								\n\t"	// reduce right red   to 8 bits of precision and half the register size
			"	vqrshrn.u16	d24, q12, #8								\n\t"	// reduce right green to 8 bits of precision and half the register size
			"	vqrshrn.u16	d28, q14, #8								\n\t"	// reduce right blue  to 8 bits of precision and half the register size

			"	vmull.u8	 q9, d18, d0								\n\t"	// weight left red   with (fracRedX^-1)
			"	vmull.u8	q11, d22, d4								\n\t"	// weight left green with (fracGreenX^-1)
			"	vmull.u8	q13, d26, d8								\n\t"	// weight left blue  with (fracBlueX^-1)

			"	vmlal.u8	 q9, d20, d1								\n\t"	// weight right red   with (fracRedX)   and add to left red
			"	vmlal.u8	q11, d24, d5								\n\t"	// weight right green with (fracGreenX) and add to left green
			"	vmlal.u8	q13, d28, d9								\n\t"	// weight right blue  with (fracBlueX)  and add to left blue

			"	vqrshrn.u16	d28,  q9, #8								\n\t"	// reduce to 8 bits of precision and half the register size
			"	vqrshrn.u16	d29, q11, #8								\n\t"	// reduce to 8 bits of precision and half the register size
			"	vqrshrn.u16	d30, q13, #8								\n\t"	// reduce to 8 bits of precision and half the register size
			"	vmov.u64	d31, 0										\n\t"

			"	vst4.u8		{d28[0],d29[0],d30[0],d31[0]}, [%[d]], r0	\n\t"
			"	vst4.u8		{d28[1],d29[1],d30[1],d31[1]}, [%[d]], r0	\n\t"
			"	vst4.u8		{d28[2],d29[2],d30[2],d31[2]}, [%[d]], r0	\n\t"
			"	vst4.u8		{d28[3],d29[3],d30[3],d31[3]}, [%[d]], r0	\n\t"
			"	vst4.u8		{d28[4],d29[4],d30[4],d31[4]}, [%[d]], r0	\n\t"
			"	vst4.u8		{d28[5],d29[5],d30[5],d31[5]}, [%[d]], r0	\n\t"
			"	vst4.u8		{d28[6],d29[6],d30[6],d31[6]}, [%[d]], r0	\n\t"
			"	vst4.u8		{d28[7],d29[7],d30[7],d31[7]}, [%[d]], r0	\n\t"

			"	vldm		sp, {q9-q11]								\n\t"	// load q9-q11 from the stack

			"	vadd.u8		q0, q0,  q6									\n\t"	// update the bilinear weights (fracRedX^-1), (fracRedX)
			"	vadd.u8		q1, q1,  q7									\n\t"	// update the bilinear weights (fracRedY^-1), (fracRedY)
			"	vadd.u8		q2, q2,  q8									\n\t"	// update the bilinear weights (fracGreenX^-1), (fracGreenX)
			"	vadd.u8		q3, q3,  q9									\n\t"	// update the bilinear weights (fracGreenY^-1), (fracGreenY)
			"	vadd.u8		q4, q4, q10									\n\t"	// update the bilinear weights (fracBlueX^-1), (fracBlueX)
			"	vadd.u8		q5, q5, q11									\n\t"	// update the bilinear weights (fracBlueY^-1), (fracBlueY)

			"	cmp			%[d], r1									\n\t"
			"	bne			.LOOP_CHROMATIC_PLANAR_RGB					\n\t"

			"	add			sp, sp, #64									\n\t"	// release stack space
			:
			:	[srxy] "r" (localSrcRedXY),
				[sgxy] "r" (localSrcGreenXY),
				[sbxy] "r" (localSrcBlueXY),
				[drxy] "r" (deltaRedXY),
				[dgxy] "r" (deltaGreenXY),
				[dbxy] "r" (deltaBlueXY),
				[sr] "r" (localSrcRed),
				[sg] "r" (localSrcGreen),
				[sb] "r" (localSrcBlue),
				[d] "r" (destRow),
				[p] "r" (srcPitchInTexels)
			:	"r0", "r1",
				"q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15",
				"memory"
		);

#elif defined( __HEXAGON_V50__ )	// 2 pixels per iteration appears to be optimal

		Word32 drxy0 = Q6_R_combine_RlRl( deltaRedY8, deltaRedX8 );
		Word32 dgxy0 = Q6_R_combine_RlRl( deltaGreenY8, deltaGreenX8 );
		Word32 dbxy0 = Q6_R_combine_RlRl( deltaBlueY8, deltaBlueX8 );

		Word64 drxy2 = Q6_P_vaslh_PI( Q6_P_combine_RR( drxy0, drxy0 ), 1 );			// dx2, dy2, dx2, dy2
		Word64 dgxy2 = Q6_P_vaslh_PI( Q6_P_combine_RR( dgxy0, dgxy0 ), 1 );			// dx2, dy2, dx2, dy2
		Word64 dbxy2 = Q6_P_vaslh_PI( Q6_P_combine_RR( dbxy0, dbxy0 ), 1 );			// dx2, dy2, dx2, dy2

		Word32 srxy0 = Q6_R_combine_RlRl( localSrcRedY8, localSrcRedX8 );			// sx0, sy0
		Word32 sgxy0 = Q6_R_combine_RlRl( localSrcGreenY8, localSrcGreenX8 );		// sx0, sy0
		Word32 sbxy0 = Q6_R_combine_RlRl( localSrcBlueY8, localSrcBlueX8 );			// sx0, sy0

		Word64 srxy2 = Q6_P_combine_RR( Q6_R_vaddh_RR( srxy0, drxy0 ), srxy0 );		// sx0, sy0, sx1, sy1
		Word64 sgxy2 = Q6_P_combine_RR( Q6_R_vaddh_RR( sgxy0, dgxy0 ), sgxy0 );		// sx0, sy0, sx1, sy1
		Word64 sbxy2 = Q6_P_combine_RR( Q6_R_vaddh_RR( sbxy0, dbxy0 ), sbxy0 );		// sx0, sy0, sx1, sy1

		Word32 pch0 = Q6_R_combine_RlRl( srcPitchInTexels, Q6_R_equals_I( 1 ) );
		Word64 pch2 = Q6_P_combine_RR( pch0, pch0 );
		Word64 pchr = Q6_P_combine_RR( srcPitchInTexels, srcPitchInTexels );

		Word64 crfrX = Q6_P_shuffeh_PP( srxy2, srxy2 );		// sx0, sx0, sx1, sx1
		Word64 cgfrX = Q6_P_shuffeh_PP( sgxy2, sgxy2 );		// sx0, sx0, sx1, sx1
		Word64 cbfrX = Q6_P_shuffeh_PP( sbxy2, sbxy2 );		// sx0, sx0, sx1, sx1

		Word64 crfrY = Q6_P_shuffoh_PP( srxy2, srxy2 );		// sy0, sy0, sy1, sy1
		Word64 cgfrY = Q6_P_shuffoh_PP( sgxy2, sgxy2 );		// sy0, sy0, sy1, sy1
		Word64 cbfrY = Q6_P_shuffoh_PP( sbxy2, sbxy2 );		// sy0, sy0, sy1, sy1

		Word64 drfrX = Q6_P_shuffeh_PP( drxy2, drxy2 );		// dx2, dx2, dx2, dx2
		Word64 dgfrX = Q6_P_shuffeh_PP( dgxy2, dgxy2 );		// dx2, dx2, dx2, dx2
		Word64 dbfrX = Q6_P_shuffeh_PP( dbxy2, dbxy2 );		// dx2, dx2, dx2, dx2

		Word64 drfrY = Q6_P_shuffoh_PP( drxy2, drxy2 );		// dy2, dy2, dy2, dy2
		Word64 dgfrY = Q6_P_shuffoh_PP( dgxy2, dgxy2 );		// dy2, dy2, dy2, dy2
		Word64 dbfrY = Q6_P_shuffoh_PP( dbxy2, dbxy2 );		// dy2, dy2, dy2, dy2

		crfrX = Q6_P_shuffeb_PP( crfrX, crfrX );			// fx0, fx0, fx0, fx0, fx1, fx1, fx1, fx1
		cgfrX = Q6_P_shuffeb_PP( cgfrX, cgfrX );			// fx0, fx0, fx0, fx0, fx1, fx1, fx1, fx1
		cbfrX = Q6_P_shuffeb_PP( cbfrX, cbfrX );			// fx0, fx0, fx0, fx0, fx1, fx1, fx1, fx1

		crfrY = Q6_P_shuffeb_PP( crfrY, crfrY );			// fy0, fy0, fy0, fy0, fy1, fy1, fy1, fy1
		cgfrY = Q6_P_shuffeb_PP( cgfrY, cgfrY );			// fy0, fy0, fy0, fy0, fy1, fy1, fy1, fy1
		cbfrY = Q6_P_shuffeb_PP( cbfrY, cbfrY );			// fy0, fy0, fy0, fy0, fy1, fy1, fy1, fy1

		drfrX = Q6_P_shuffeb_PP( drfrX, drfrX );			// dx2, dx2, dx2, dx2, dx2, dx2, dx2, dx2
		dgfrX = Q6_P_shuffeb_PP( dgfrX, dgfrX );			// dx2, dx2, dx2, dx2, dx2, dx2, dx2, dx2
		dbfrX = Q6_P_shuffeb_PP( dbfrX, dbfrX );			// dx2, dx2, dx2, dx2, dx2, dx2, dx2, dx2

		drfrY = Q6_P_shuffeb_PP( drfrY, drfrY );			// dy2, dy2, dy2, dy2, dy2, dy2, dy2, dy2
		dgfrY = Q6_P_shuffeb_PP( dgfrY, dgfrY );			// dy2, dy2, dy2, dy2, dy2, dy2, dy2, dy2
		dbfrY = Q6_P_shuffeb_PP( dbfrY, dbfrY );			// dy2, dy2, dy2, dy2, dy2, dy2, dy2, dy2

		Word64 zero = Q6_P_combine_II(  0,  0 );			// 0x0000000000000000
		Word64 none = Q6_P_combine_II( -1, -1 );			// 0xFFFFFFFFFFFFFFFF

		Word64 xfrX = Q6_P_shuffeh_PP( zero, none );		// 0x0000FFFF0000FFFF
		Word64 xfrY = Q6_P_shuffeb_PP( zero, none );		// 0x00FF00FF00FF00FF

		crfrX = Q6_P_xor_PP( crfrX, xfrX );					// 1-fx0, 1-fx0,   fx0, fx0, 1-fx1, 1-fx1,   fx1, fx1
		cgfrX = Q6_P_xor_PP( cgfrX, xfrX );					// 1-fx0, 1-fx0,   fx0, fx0, 1-fx1, 1-fx1,   fx1, fx1
		cbfrX = Q6_P_xor_PP( cbfrX, xfrX );					// 1-fx0, 1-fx0,   fx0, fx0, 1-fx1, 1-fx1,   fx1, fx1

		crfrY = Q6_P_xor_PP( crfrY, xfrY );					// 1-fy0,   fy0, 1-fy0, fy0, 1-fy1,   fy1, 1-fy1, fy1
		cgfrY = Q6_P_xor_PP( cgfrY, xfrY );					// 1-fy0,   fy0, 1-fy0, fy0, 1-fy1,   fy1, 1-fy1, fy1
		cbfrY = Q6_P_xor_PP( cbfrY, xfrY );					// 1-fy0,   fy0, 1-fy0, fy0, 1-fy1,   fy1, 1-fy1, fy1

		drfrX = Q6_P_vsubb_PP( Q6_P_xor_PP( drfrX, xfrX ), xfrX );
		dgfrX = Q6_P_vsubb_PP( Q6_P_xor_PP( dgfrX, xfrX ), xfrX );
		dbfrX = Q6_P_vsubb_PP( Q6_P_xor_PP( dbfrX, xfrX ), xfrX );

		drfrY = Q6_P_vsubb_PP( Q6_P_xor_PP( drfrY, xfrY ), xfrY );
		dgfrY = Q6_P_vsubb_PP( Q6_P_xor_PP( dgfrY, xfrY ), xfrY );
		dbfrY = Q6_P_vsubb_PP( Q6_P_xor_PP( dbfrY, xfrY ), xfrY );

		for ( int x = 0; x < 32; x += 2 )
		{
			Word64 offsetRed0 = Q6_P_vdmpy_PP_sat( Q6_P_vasrh_PI( srxy2, STP ), pch2 );
			Word64 offsetGreen0 = Q6_P_vdmpy_PP_sat( Q6_P_vasrh_PI( sgxy2, STP ), pch2 );
			Word64 offsetBlue0 = Q6_P_vdmpy_PP_sat( Q6_P_vasrh_PI( sbxy2, STP ), pch2 );

			Word32 r0 = localSrcRed[  Q6_R_extract_Pl( offsetRed0   ) + 0];	// r0
			Word32 g0 = localSrcGreen[Q6_R_extract_Pl( offsetGreen0 ) + 0];	// g0
			Word32 b0 = localSrcBlue[ Q6_R_extract_Pl( offsetBlue0  ) + 0];	// b0

			Word32 r1 = localSrcRed[  Q6_R_extract_Ph( offsetRed0   ) + 0];	// r0
			Word32 g1 = localSrcGreen[Q6_R_extract_Ph( offsetGreen0 ) + 0];	// g0
			Word32 b1 = localSrcBlue[ Q6_R_extract_Ph( offsetBlue0  ) + 0];	// b0

			r0 = Q6_R_insert_RII( r0, localSrcRed[  Q6_R_extract_Pl( offsetRed0   ) + 1], 8, 16 );	// r0, r2, r1
			g0 = Q6_R_insert_RII( g0, localSrcGreen[Q6_R_extract_Pl( offsetGreen0 ) + 1], 8, 16 );	// g0, g2, g1
			b0 = Q6_R_insert_RII( b0, localSrcBlue[ Q6_R_extract_Pl( offsetBlue0  ) + 1], 8, 16 );	// b0, b2, b1

			r1 = Q6_R_insert_RII( r1, localSrcRed[  Q6_R_extract_Ph( offsetRed0   ) + 1], 8, 16 );	// r0, r2, r1
			g1 = Q6_R_insert_RII( g1, localSrcGreen[Q6_R_extract_Ph( offsetGreen0 ) + 1], 8, 16 );	// g0, g2, b1
			b1 = Q6_R_insert_RII( b1, localSrcBlue[ Q6_R_extract_Ph( offsetBlue0  ) + 1], 8, 16 );	// b0, b2, g1

			Word64 offsetRed1 = Q6_P_vaddw_PP( offsetRed0, pchr );
			Word64 offsetGreen1 = Q6_P_vaddw_PP( offsetGreen0, pchr );
			Word64 offsetBlue1 = Q6_P_vaddw_PP( offsetBlue0, pchr );

			r0 = Q6_R_insert_RII( r0, localSrcRed[  Q6_R_extract_Pl( offsetRed1   ) + 0], 8, 8 );	// r0, r2
			g0 = Q6_R_insert_RII( g0, localSrcGreen[Q6_R_extract_Pl( offsetGreen1 ) + 0], 8, 8 );	// g0, g2
			b0 = Q6_R_insert_RII( b0, localSrcBlue[ Q6_R_extract_Pl( offsetBlue1  ) + 0], 8, 8 );	// b0, b2

			r1 = Q6_R_insert_RII( r1, localSrcRed[  Q6_R_extract_Ph( offsetRed1   ) + 0], 8, 8 );	// r0, r2
			g1 = Q6_R_insert_RII( g1, localSrcGreen[Q6_R_extract_Ph( offsetGreen1 ) + 0], 8, 8 );	// g0, g2
			b1 = Q6_R_insert_RII( b1, localSrcBlue[ Q6_R_extract_Ph( offsetBlue1  ) + 0], 8, 8 );	// b0, b2

			r0 = Q6_R_insert_RII( r0, localSrcRed[  Q6_R_extract_Pl( offsetRed1   ) + 1], 8, 24 );	// r0, r2, r1, r3
			g0 = Q6_R_insert_RII( g0, localSrcGreen[Q6_R_extract_Pl( offsetGreen1 ) + 1], 8, 24 );	// g0, g2, g1, g3
			b0 = Q6_R_insert_RII( b0, localSrcBlue[ Q6_R_extract_Pl( offsetBlue1  ) + 1], 8, 24 );	// b0, b2, b1, b3

			r1 = Q6_R_insert_RII( r1, localSrcRed[  Q6_R_extract_Ph( offsetRed1   ) + 1], 8, 24 );	// r0, r2, r1, r3
			g1 = Q6_R_insert_RII( g1, localSrcGreen[Q6_R_extract_Ph( offsetGreen1 ) + 1], 8, 24 );	// g0, g2, b1, b3
			b1 = Q6_R_insert_RII( b1, localSrcBlue[ Q6_R_extract_Ph( offsetBlue1  ) + 1], 8, 24 );	// b0, b2, g1, g3

			Word64 t0 = Q6_P_combine_RR( b0, r0 );
			Word64 t1 = Q6_P_combine_RR( g0, g0 );
			Word64 t2 = Q6_P_combine_RR( b1, r1 );
			Word64 t3 = Q6_P_combine_RR( g1, g1 );

			Word32 mr0 = Q6_R_vtrunohb_P( Q6_P_vmpybu_RR( Q6_R_extract_Pl( crfrX ), Q6_R_extract_Pl( crfrY ) ) );
			Word32 mg0 = Q6_R_vtrunohb_P( Q6_P_vmpybu_RR( Q6_R_extract_Pl( cgfrX ), Q6_R_extract_Pl( cgfrY ) ) );
			Word32 mb0 = Q6_R_vtrunohb_P( Q6_P_vmpybu_RR( Q6_R_extract_Pl( cbfrX ), Q6_R_extract_Pl( cbfrY ) ) );

			Word32 mr1 = Q6_R_vtrunohb_P( Q6_P_vmpybu_RR( Q6_R_extract_Ph( crfrX ), Q6_R_extract_Ph( crfrY ) ) );
			Word32 mg1 = Q6_R_vtrunohb_P( Q6_P_vmpybu_RR( Q6_R_extract_Ph( cgfrX ), Q6_R_extract_Ph( cgfrY ) ) );
			Word32 mb1 = Q6_R_vtrunohb_P( Q6_P_vmpybu_RR( Q6_R_extract_Ph( cbfrX ), Q6_R_extract_Ph( cbfrY ) ) );

			Word64 f0 = Q6_P_combine_RR( mb0, mr0 );
			Word64 f1 = Q6_P_combine_RR( mg0, mg0 );
			Word64 f2 = Q6_P_combine_RR( mb1, mr1 );
			Word64 f3 = Q6_P_combine_RR( mg1, mg1 );

			t0 = Q6_P_vrmpybu_PP( t0, f0 );			// r, b
			t1 = Q6_P_vrmpybu_PP( t1, f1 );			// g, a
			t2 = Q6_P_vrmpybu_PP( t2, f2 );			// r, b
			t3 = Q6_P_vrmpybu_PP( t3, f3 );			// g, a

			t0 = Q6_P_shuffeh_PP( t1, t0 );			// r, g, b, a
			t2 = Q6_P_shuffeh_PP( t3, t2 );			// r, g, b, a

			*(Word64 *)&destRow[x] = Q6_P_combine_RR( Q6_R_vtrunohb_P( t2 ), Q6_R_vtrunohb_P( t0 ) );

			srxy2 = Q6_P_vaddh_PP( srxy2, drxy2 );
			sgxy2 = Q6_P_vaddh_PP( sgxy2, dgxy2 );
			sbxy2 = Q6_P_vaddh_PP( sbxy2, dbxy2 );

			crfrX = Q6_P_vaddub_PP( crfrX, drfrX );
			cgfrX = Q6_P_vaddub_PP( cgfrX, dgfrX );
			cbfrX = Q6_P_vaddub_PP( cbfrX, dbfrX );

			crfrY = Q6_P_vaddub_PP( crfrY, drfrY );
			cgfrY = Q6_P_vaddub_PP( cgfrY, dgfrY );
			cbfrY = Q6_P_vaddub_PP( cbfrY, dbfrY );
		}

#else

		for ( int x = 0; x < 32; x++ )
		{
			const int sampleRedX = localSrcRedX8 >> STP;
			const int sampleRedY = localSrcRedY8 >> STP;
			const int sampleGreenX = localSrcGreenX8 >> STP;
			const int sampleGreenY = localSrcGreenY8 >> STP;
			const int sampleBlueX = localSrcBlueX8 >> STP;
			const int sampleBlueY = localSrcBlueY8 >> STP;

			const unsigned char * texelR = localSrcRed + sampleRedY * srcPitchInTexels + sampleRedX;
			const unsigned char * texelG = localSrcGreen + sampleGreenY * srcPitchInTexels + sampleGreenX;
			const unsigned char * texelB = localSrcBlue + sampleBlueY * srcPitchInTexels + sampleBlueX;

			int r0 = texelR[0];
			int r1 = texelR[1];
			int r2 = texelR[srcPitchInTexels + 0];
			int r3 = texelR[srcPitchInTexels + 1];

			int g0 = texelG[0];
			int g1 = texelG[1];
			int g2 = texelG[srcPitchInTexels + 0];
			int g3 = texelG[srcPitchInTexels + 1];

			int b0 = texelB[0];
			int b1 = texelB[1];
			int b2 = texelB[srcPitchInTexels + 0];
			int b3 = texelB[srcPitchInTexels + 1];

			const int fracRedX1 = localSrcRedX8 & ( ( 1 << STP ) - 1 );
			const int fracRedX0 = ( 1 << STP ) - fracRedX1;
			const int fracGreenX1 = localSrcGreenX8 & ( ( 1 << STP ) - 1 );
			const int fracGreenX0 = ( 1 << STP ) - fracGreenX1;
			const int fracBlueX1 = localSrcBlueX8 & ( ( 1 << STP ) - 1 );
			const int fracBlueX0 = ( 1 << STP ) - fracBlueX1;

			const int fracRedY1 = localSrcRedY8 & ( ( 1 << STP ) - 1 );
			const int fracRedY0 = ( 1 << STP ) - fracRedY1;
			const int fracGreenY1 = localSrcGreenY8 & ( ( 1 << STP ) - 1 );
			const int fracGreenY0 = ( 1 << STP ) - fracGreenY1;
			const int fracBlueY1 = localSrcBlueY8 & ( ( 1 << STP ) - 1 );
			const int fracBlueY0 = ( 1 << STP ) - fracBlueY1;

			r0 = fracRedX0 * r0 + fracRedX1 * r1;
			r2 = fracRedX0 * r2 + fracRedX1 * r3;

			g0 = fracGreenX0 * g0 + fracGreenX1 * g1;
			g2 = fracGreenX0 * g2 + fracGreenX1 * g3;

			b0 = fracBlueX0 * b0 + fracBlueX1 * b1;
			b2 = fracBlueX0 * b2 + fracBlueX1 * b3;

			r0 = fracRedY0 * r0 + fracRedY1 * r2;
			g0 = fracGreenY0 * g0 + fracGreenY1 * g2;
			b0 = fracBlueY0 * b0 + fracBlueY1 * b2;

			*destRow++ =	( ( r0 & 0x00FF0000 ) >> 16 ) |
							( ( g0 & 0x00FF0000 ) >>  8 ) |
							( ( b0 & 0x00FF0000 ) >>  0 );

			localSrcRedX8 += deltaRedX8;
			localSrcRedY8 += deltaRedY8;
			localSrcGreenX8 += deltaGreenX8;
			localSrcGreenY8 += deltaGreenY8;
			localSrcBlueX8 += deltaBlueX8;
			localSrcBlueY8 += deltaBlueY8;
		}

#endif

		scanLeftSrcRedX   += scanLeftDeltaRedX;
		scanLeftSrcRedY   += scanLeftDeltaRedY;
		scanLeftSrcGreenX += scanLeftDeltaGreenX;
		scanLeftSrcGreenY += scanLeftDeltaGreenY;
		scanLeftSrcBlueX  += scanLeftDeltaBlueX;
		scanLeftSrcBlueY  += scanLeftDeltaBlueY;

		scanRightSrcRedX   += scanRightDeltaRedX;
		scanRightSrcRedY   += scanRightDeltaRedY;
		scanRightSrcGreenX += scanRightDeltaGreenX;
		scanRightSrcGreenY += scanRightDeltaGreenY;
		scanRightSrcBlueX  += scanRightDeltaBlueX;
		scanRightSrcBlueY  += scanRightDeltaBlueY;
	}

	//FlushCacheBox( dest, 32 * 4, 32, destPitchInPixels * 4 );
}

/*
================================================================================================
4x4 Matrix
================================================================================================
*/

#define MATH_PI				3.14159265358979323846f

// Row-major 4x4 matrix
typedef struct
{
	float m[4][4];
} Matrix4x4f_t;

// Creates an identity matrix.
void Matrix4x4f_CreateIdentity( Matrix4x4f_t * matrix )
{
	matrix->m[0][0] = 1.0f; matrix->m[0][1] = 0.0f; matrix->m[0][2] = 0.0f; matrix->m[0][3] = 0.0f;
	matrix->m[1][0] = 0.0f; matrix->m[1][1] = 1.0f; matrix->m[1][2] = 0.0f; matrix->m[1][3] = 0.0f;
	matrix->m[2][0] = 0.0f; matrix->m[2][1] = 0.0f; matrix->m[2][2] = 1.0f; matrix->m[2][3] = 0.0f;
	matrix->m[3][0] = 0.0f; matrix->m[3][1] = 0.0f; matrix->m[3][2] = 0.0f; matrix->m[3][3] = 1.0f;
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
		matrix->m[0][1] = 0;
		matrix->m[0][2] = ( maxX + minX ) / width;
		matrix->m[0][3] = 0;

		matrix->m[1][0] = 0;
		matrix->m[1][1] = 2 * nearZ / height;
		matrix->m[1][2] = ( maxY + minY ) / height;
		matrix->m[1][3] = 0;

		matrix->m[2][0] = 0;
		matrix->m[2][1] = 0;
		matrix->m[2][2] = -1;
		matrix->m[2][3] = -( nearZ + offsetZ );

		matrix->m[3][0] = 0;
		matrix->m[3][1] = 0;
		matrix->m[3][2] = -1;
		matrix->m[3][3] = 0;
	}
	else
	{
		// normal projection
		matrix->m[0][0] = 2 * nearZ / width;
		matrix->m[0][1] = 0;
		matrix->m[0][2] = ( maxX + minX ) / width;
		matrix->m[0][3] = 0;

		matrix->m[1][0] = 0;
		matrix->m[1][1] = 2 * nearZ / height;
		matrix->m[1][2] = ( maxY + minY ) / height;
		matrix->m[1][3] = 0;

		matrix->m[2][0] = 0;
		matrix->m[2][1] = 0;
		matrix->m[2][2] = -( farZ + offsetZ ) / ( farZ - nearZ );
		matrix->m[2][3] = -( farZ * ( nearZ + offsetZ ) ) / ( farZ - nearZ );

		matrix->m[3][0] = 0;
		matrix->m[3][1] = 0;
		matrix->m[3][2] = -1;
		matrix->m[3][3] = 0;
	}
}

// Creates a projection matrix based on the specified FOV.
void Matrix4x4f_CreateProjectionFov( Matrix4x4f_t * matrix, const float fovDegreesX, const float fovDegreesY,
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

// Use left-multiplication to accumulate transformations.
void Matrix4x4f_Multiply( Matrix4x4f_t * result, const Matrix4x4f_t * a, const Matrix4x4f_t * b )
{
	result->m[0][0] = a->m[0][0] * b->m[0][0] + a->m[0][1] * b->m[1][0] + a->m[0][2] * b->m[2][0] + a->m[0][3] * b->m[3][0];
	result->m[0][1] = a->m[0][0] * b->m[0][1] + a->m[0][1] * b->m[1][1] + a->m[0][2] * b->m[2][1] + a->m[0][3] * b->m[3][1];
	result->m[0][2] = a->m[0][0] * b->m[0][2] + a->m[0][1] * b->m[1][2] + a->m[0][2] * b->m[2][2] + a->m[0][3] * b->m[3][2];
	result->m[0][3] = a->m[0][0] * b->m[0][3] + a->m[0][1] * b->m[1][3] + a->m[0][2] * b->m[2][3] + a->m[0][3] * b->m[3][3];

	result->m[1][0] = a->m[1][0] * b->m[0][0] + a->m[1][1] * b->m[1][0] + a->m[1][2] * b->m[2][0] + a->m[1][3] * b->m[3][0];
	result->m[1][1] = a->m[1][0] * b->m[0][1] + a->m[1][1] * b->m[1][1] + a->m[1][2] * b->m[2][1] + a->m[1][3] * b->m[3][1];
	result->m[1][2] = a->m[1][0] * b->m[0][2] + a->m[1][1] * b->m[1][2] + a->m[1][2] * b->m[2][2] + a->m[1][3] * b->m[3][2];
	result->m[1][3] = a->m[1][0] * b->m[0][3] + a->m[1][1] * b->m[1][3] + a->m[1][2] * b->m[2][3] + a->m[1][3] * b->m[3][3];

	result->m[2][0] = a->m[2][0] * b->m[0][0] + a->m[2][1] * b->m[1][0] + a->m[2][2] * b->m[2][0] + a->m[2][3] * b->m[3][0];
	result->m[2][1] = a->m[2][0] * b->m[0][1] + a->m[2][1] * b->m[1][1] + a->m[2][2] * b->m[2][1] + a->m[2][3] * b->m[3][1];
	result->m[2][2] = a->m[2][0] * b->m[0][2] + a->m[2][1] * b->m[1][2] + a->m[2][2] * b->m[2][2] + a->m[2][3] * b->m[3][2];
	result->m[2][3] = a->m[2][0] * b->m[0][3] + a->m[2][1] * b->m[1][3] + a->m[2][2] * b->m[2][3] + a->m[2][3] * b->m[3][3];

	result->m[3][0] = a->m[3][0] * b->m[0][0] + a->m[3][1] * b->m[1][0] + a->m[3][2] * b->m[2][0] + a->m[3][3] * b->m[3][0];
	result->m[3][1] = a->m[3][0] * b->m[0][1] + a->m[3][1] * b->m[1][1] + a->m[3][2] * b->m[2][1] + a->m[3][3] * b->m[3][1];
	result->m[3][2] = a->m[3][0] * b->m[0][2] + a->m[3][1] * b->m[1][2] + a->m[3][2] * b->m[2][2] + a->m[3][3] * b->m[3][2];
	result->m[3][3] = a->m[3][0] * b->m[0][3] + a->m[3][1] * b->m[1][3] + a->m[3][2] * b->m[2][3] + a->m[3][3] * b->m[3][3];
}

// Returns a 3x3 minor of a 4x4 matrix.
float Matrix4x4f_Minor( const Matrix4x4f_t * src, int r0, int r1, int r2, int c0, int c1, int c2 )
{
	return	src->m[r0][c0] * ( src->m[r1][c1] * src->m[r2][c2] - src->m[r2][c1] * src->m[r1][c2] ) -
			src->m[r0][c1] * ( src->m[r1][c0] * src->m[r2][c2] - src->m[r2][c0] * src->m[r1][c2] ) +
			src->m[r0][c2] * ( src->m[r1][c0] * src->m[r2][c1] - src->m[r2][c0] * src->m[r1][c1] );
}
 
// Calculates the inverse of an arbitrary 4x4 matrix.
void Matrix4x4f_Invert( Matrix4x4f_t * result, const Matrix4x4f_t * src )
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

// Calculates the inverse of a homogeneous matrix.
void Matrix4x4f_InvertHomogeneous( Matrix4x4f_t * result, const Matrix4x4f_t * src )
{
	result->m[0][0] = src->m[0][0];
	result->m[1][0] = src->m[0][1];
	result->m[2][0] = src->m[0][2];
	result->m[3][0] = 0.0f;
	result->m[0][1] = src->m[1][0];
	result->m[1][1] = src->m[1][1];
	result->m[2][1] = src->m[1][2];
	result->m[3][1] = 0.0f;
	result->m[0][2] = src->m[2][0];
	result->m[1][2] = src->m[2][1];
	result->m[2][2] = src->m[2][2];
	result->m[3][2] = 0.0f;
	result->m[0][3] = -( src->m[0][0] * src->m[0][3] + src->m[1][0] * src->m[1][3] + src->m[2][0] * src->m[2][3] );
	result->m[1][3] = -( src->m[0][1] * src->m[0][3] + src->m[1][1] * src->m[1][3] + src->m[2][1] * src->m[2][3] );
	result->m[2][3] = -( src->m[0][2] * src->m[0][3] + src->m[1][2] * src->m[1][3] + src->m[2][2] * src->m[2][3] );
	result->m[3][3] = 1.0f;
}

/*
================================================================================================
Time Warp
================================================================================================
*/

// Calculate a 4x4 time warp transformation matrix.
void CalculateTimeWarpTransform( Matrix4x4f_t * transform, const Matrix4x4f_t * renderProjectionMatrix,
								const Matrix4x4f_t * renderViewMatrix, const Matrix4x4f_t * newViewMatrix )
{
	// Convert the projection matrix from [-1, 1] space to [0, 1] space.
	const Matrix4x4f_t texCoordProjection =
	{ {
		{ 0.5f * renderProjectionMatrix->m[0][0], 0.0f, 0.5f * renderProjectionMatrix->m[0][2] - 0.5f, 0.0f },
		{ 0.0f, 0.5f * renderProjectionMatrix->m[1][1], 0.5f * renderProjectionMatrix->m[1][2] - 0.5f, 0.0f },
		{ 0.0f, 0.0f, -1.0f, 0.0f },
		{ 0.0f, 0.0f,  0.0f, 1.0f }
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
	inverseDeltaViewMatrix.m[0][3] = 0.0f;
	inverseDeltaViewMatrix.m[1][3] = 0.0f;
	inverseDeltaViewMatrix.m[2][3] = 0.0f;

	// Accumulate the transforms.
	Matrix4x4f_Multiply( transform, &texCoordProjection, &inverseDeltaViewMatrix );
}

// Transforms the 2D coordinates by interpreting them as 3D homogeneous coordinates with Z = -1 and W = 1.
void TransformCoords( float result[3], const Matrix4x4f_t * transform, const float coords[2] )
{
	result[0] = transform->m[0][0] * coords[0] + transform->m[0][1] * coords[1] - transform->m[0][2] + transform->m[0][3];
	result[1] = transform->m[1][0] * coords[0] + transform->m[1][1] * coords[1] - transform->m[1][2] + transform->m[1][3];
	result[2] = transform->m[2][0] * coords[0] + transform->m[2][1] * coords[1] - transform->m[2][2] + transform->m[2][3];
}

// Interpolate between two 3D vectors.
void InterpolateCoords( float result[3], const float start[3], const float end[3], const float fraction )
{
	result[0] = start[0] + fraction * ( end[0] - start[0] );
	result[1] = start[1] + fraction * ( end[1] - start[1] );
	result[2] = start[2] + fraction * ( end[2] - start[2] );
}

// Transform the given coordinates with the given time warp matrices.
// displayRefreshFraction       = The fraction along the display refresh.
// displayRefreshStartTransform = The time warp transform at the start of the display refresh.
// displayRefreshEndTransform   = The time warp transform at the end of the display refresh.
void TimeWarpCoords( float result[2], const float coords[2], const float displayRefreshFraction,
						const Matrix4x4f_t * displayRefreshStartTransform, const Matrix4x4f_t * displayRefreshEndTransform )
{
	float start[3];
	float end[3];
	TransformCoords( start, displayRefreshStartTransform, coords );
	TransformCoords( end, displayRefreshEndTransform, coords );

	float current[3];
	InterpolateCoords( current, start, end, displayRefreshFraction );

	const float rcpZ = 1.0f / current[2];
	result[0] = current[0] * rcpZ;
	result[1] = current[1] * rcpZ;
}

void TimeWarp_SampleNearestPackedRGB(
		const unsigned char *	src,				// source texture with 32 bits per texel
		const int				srcPitchInTexels,	// in texels
		const int				srcTexelsWide,		// in texels
		const int				srcTexelsHigh,		// in texels
		unsigned char *			dest,				// destination buffer with 32 bits per pixel
		const int				destPitchInPixels,	// in pixels: 1080, 1440, etc.
		const int				destTilesWide,		// tiles are implicitly 32 x 32 pixels
		const int				destTilesHigh,
		const int				destEye,
		const MeshCoord_t *		distortionMesh,		// [(destTilesWide+1)*(destTilesHigh+1)]
		MeshCoord_t *			tempMeshCoords,
		const Matrix4x4f_t *	timeWarpStartTransform,
		const Matrix4x4f_t *	timeWarpEndTransform )
{
	// Time warp transform the distortion mesh.
	for ( int y = 0; y <= destTilesHigh; y++ )
	{
		for ( int x = 0; x <= destTilesWide; x++ )
		{
			const int index = y * ( destTilesWide + 1 ) + x;
			const float displayFraction = ( (float)destEye * destTilesWide + x ) / ( destTilesWide * 2.0f );	// landscape left-to-right
			TimeWarpCoords( &tempMeshCoords[index].x, &distortionMesh[index].x, displayFraction, timeWarpStartTransform, timeWarpEndTransform );
		}
	}

	// Warp the individual tiles.
	for ( int y = 0; y < destTilesHigh; y++ )
	{
		for ( int x = 0; x < destTilesWide; x++ )
		{
			const MeshCoord_t * quadCoords = tempMeshCoords + ( y * ( destTilesWide + 1 ) + x );
			unsigned char * tileDest = dest + ( y * destPitchInPixels + x ) * 32 * 4;

			Warp32x32_SampleNearestPackedRGB( src, srcPitchInTexels, srcTexelsWide, srcTexelsHigh,
												tileDest, destPitchInPixels,
												quadCoords, ( destTilesWide + 1 ) );
		}
	}
}

void TimeWarp_SampleLinearPackedRGB(
		const unsigned char *	src,				// source texture with 32 bits per texel
		const int				srcPitchInTexels,	// in texels
		const int				srcTexelsWide,		// in texels
		const int				srcTexelsHigh,		// in texels
		unsigned char *			dest,				// destination buffer with 32 bits per pixel
		const int				destPitchInPixels,	// in pixels: 1080, 1440, etc.
		const int				destTilesWide,		// tiles are implicitly 32 x 32 pixels
		const int				destTilesHigh,
		const int				destEye,
		const MeshCoord_t *		distortionMesh,		// [(destTilesWide+1)*(destTilesHigh+1)]
		MeshCoord_t *			tempMeshCoords,
		const Matrix4x4f_t *	timeWarpStartTransform,
		const Matrix4x4f_t *	timeWarpEndTransform )
{
	// Time warp transform the distortion mesh.
	for ( int y = 0; y <= destTilesHigh; y++ )
	{
		for ( int x = 0; x <= destTilesWide; x++ )
		{
			const int index = y * ( destTilesWide + 1 ) + x;
			const float displayFraction = ( (float)destEye * destTilesWide + x ) / ( destTilesWide * 2.0f );	// landscape left-to-right
			TimeWarpCoords( &tempMeshCoords[index].x, &distortionMesh[index].x, displayFraction, timeWarpStartTransform, timeWarpEndTransform );
		}
	}

	// Warp the individual tiles.
	for ( int y = 0; y < destTilesHigh; y++ )
	{
		for ( int x = 0; x < destTilesWide; x++ )
		{
			const MeshCoord_t * quadCoords = tempMeshCoords + ( y * ( destTilesWide + 1 ) + x );
			unsigned char * tileDest = dest + ( y * destPitchInPixels + x ) * 32 * 4;

			Warp32x32_SampleLinearPackedRGB( src, srcPitchInTexels, srcTexelsWide, srcTexelsHigh,
												tileDest, destPitchInPixels,
												quadCoords, ( destTilesWide + 1 ) );
		}
	}
}

void TimeWarp_SampleBilinearPackedRGB(
		const unsigned char *	src,				// source texture with 32 bits per texel
		const int				srcPitchInTexels,	// in texels
		const int				srcTexelsWide,		// in texels
		const int				srcTexelsHigh,		// in texels
		unsigned char *			dest,				// destination buffer with 32 bits per pixel
		const int				destPitchInPixels,	// in pixels: 1080, 1440, etc.
		const int				destTilesWide,		// tiles are implicitly 32 x 32 pixels
		const int				destTilesHigh,
		const int				destEye,
		const MeshCoord_t *		distortionMesh,		// [(destTilesWide+1)*(destTilesHigh+1)]
		MeshCoord_t *			tempMeshCoords,
		const Matrix4x4f_t *	timeWarpStartTransform,
		const Matrix4x4f_t *	timeWarpEndTransform )
{
	// Time warp transform the distortion mesh.
	for ( int y = 0; y <= destTilesHigh; y++ )
	{
		for ( int x = 0; x <= destTilesWide; x++ )
		{
			const int index = y * ( destTilesWide + 1 ) + x;
			const float displayFraction = ( (float)destEye * destTilesWide + x ) / ( destTilesWide * 2.0f );	// landscape left-to-right
			TimeWarpCoords( &tempMeshCoords[index].x, &distortionMesh[index].x, displayFraction, timeWarpStartTransform, timeWarpEndTransform );
		}
	}

	// Warp the individual tiles.
	for ( int y = 0; y < destTilesHigh; y++ )
	{
		for ( int x = 0; x < destTilesWide; x++ )
		{
			const MeshCoord_t * quadCoords = tempMeshCoords + ( y * ( destTilesWide + 1 ) + x );
			unsigned char * tileDest = dest + ( y * destPitchInPixels + x ) * 32 * 4;

			Warp32x32_SampleBilinearPackedRGB( src, srcPitchInTexels, srcTexelsWide, srcTexelsHigh,
												tileDest, destPitchInPixels,
												quadCoords, ( destTilesWide + 1 ) );
		}
	}
}

void TimeWarp_SampleBilinearPlanarRGB(
		const unsigned char *	srcRed,				// source texture with 8 bits per texel
		const unsigned char *	srcGreen,			// source texture with 8 bits per texel
		const unsigned char *	srcBlue,			// source texture with 8 bits per texel
		const int				srcPitchInTexels,	// in texels
		const int				srcTexelsWide,		// in texels
		const int				srcTexelsHigh,		// in texels
		unsigned char *			dest,				// destination buffer with 32 bits per pixel
		const int				destPitchInPixels,	// in pixels: 1080, 1440, etc.
		const int				destTilesWide,		// tiles are implicitly 32 x 32 pixels
		const int				destTilesHigh,
		const int				destEye,
		const MeshCoord_t *		distortionMesh,		// [(destTilesWide+1)*(destTilesHigh+1)]
		MeshCoord_t *			tempMeshCoords,
		const Matrix4x4f_t *	timeWarpStartTransform,
		const Matrix4x4f_t *	timeWarpEndTransform )
{
	// Time warp transform the distortion mesh.
	for ( int y = 0; y <= destTilesHigh; y++ )
	{
		for ( int x = 0; x <= destTilesWide; x++ )
		{
			const int index = y * ( destTilesWide + 1 ) + x;
			const float displayFraction = ( (float)destEye * destTilesWide + x ) / ( destTilesWide * 2.0f );	// landscape left-to-right
			TimeWarpCoords( &tempMeshCoords[index].x, &distortionMesh[index].x, displayFraction, timeWarpStartTransform, timeWarpEndTransform );
		}
	}

	// Warp the individual tiles.
	for ( int y = 0; y < destTilesHigh; y++ )
	{
		for ( int x = 0; x < destTilesWide; x++ )
		{
			const MeshCoord_t * quadCoords = tempMeshCoords + ( y * ( destTilesWide + 1 ) + x );
			unsigned char * tileDest = dest + ( y * destPitchInPixels + x ) * 32 * 4;

			Warp32x32_SampleBilinearPlanarRGB( srcRed, srcGreen, srcBlue, srcPitchInTexels, srcTexelsWide, srcTexelsHigh,
												tileDest, destPitchInPixels,
												quadCoords, ( destTilesWide + 1 ) );
		}
	}
}

void TimeWarp_SampleChromaticBilinearPlanarRGB(
		const unsigned char *	srcRed,				// source texture with 8 bits per texel
		const unsigned char *	srcGreen,			// source texture with 8 bits per texel
		const unsigned char *	srcBlue,			// source texture with 8 bits per texel
		const int				srcPitchInTexels,	// in texels
		const int				srcTexelsWide,		// in texels
		const int				srcTexelsHigh,		// in texels
		unsigned char *			dest,				// destination buffer with 32 bits per pixel
		const int				destPitchInPixels,	// in pixels: 1080, 1440, etc.
		const int				destTilesWide,		// tiles are implicitly 32 x 32 pixels
		const int				destTilesHigh,
		const int				destEye,
		const MeshCoord_t *		distortionMeshRed,	// [(destTilesWide+1)*(destTilesHigh+1)]
		const MeshCoord_t *		distortionMeshGreen,
		const MeshCoord_t *		distortionMeshBlue,
		MeshCoord_t *			tempMeshCoordsRed,
		MeshCoord_t *			tempMeshCoordsGreen,
		MeshCoord_t *			tempMeshCoordsBlue,
		const Matrix4x4f_t *	timeWarpStartTransform,
		const Matrix4x4f_t *	timeWarpEndTransform )
{
	// Time warp transform the distortion mesh.
	for ( int y = 0; y <= destTilesHigh; y++ )
	{
		for ( int x = 0; x <= destTilesWide; x++ )
		{
			const int index = y * ( destTilesWide + 1 ) + x;
			const float displayFraction = ( (float)destEye * destTilesWide + x ) / ( destTilesWide * 2.0f );	// landscape left-to-right
			TimeWarpCoords( &tempMeshCoordsRed[index].x, &distortionMeshRed[index].x, displayFraction, timeWarpStartTransform, timeWarpEndTransform );
			TimeWarpCoords( &tempMeshCoordsGreen[index].x, &distortionMeshGreen[index].x, displayFraction, timeWarpStartTransform, timeWarpEndTransform );
			TimeWarpCoords( &tempMeshCoordsBlue[index].x, &distortionMeshBlue[index].x, displayFraction, timeWarpStartTransform, timeWarpEndTransform );
		}
	}

	// Warp the individual tiles.
	for ( int y = 0; y < destTilesHigh; y++ )
	{
		for ( int x = 0; x < destTilesWide; x++ )
		{
			const MeshCoord_t * quadCoordsRed = tempMeshCoordsRed + ( y * ( destTilesWide + 1 ) + x );
			const MeshCoord_t * quadCoordsGreen = tempMeshCoordsGreen + ( y * ( destTilesWide + 1 ) + x );
			const MeshCoord_t * quadCoordsBlue = tempMeshCoordsBlue + ( y * ( destTilesWide + 1 ) + x );
			unsigned char * tileDest = dest + ( y * destPitchInPixels + x ) * 32 * 4;

			Warp32x32_SampleChromaticBilinearPlanarRGB( srcRed, srcGreen, srcBlue, srcPitchInTexels, srcTexelsWide, srcTexelsHigh,
														tileDest, destPitchInPixels,
														quadCoordsRed, quadCoordsGreen, quadCoordsBlue, ( destTilesWide + 1 ) );
		}
	}
}

#if !defined( OS_HEXAGON )

/*
================================================================================================
Distortion meshes
================================================================================================
*/

#define DISPLAY_PIXELS_WIDE		1920
#define DISPLAY_PIXELS_HIGH		1080

#define NUM_EYES				2
#define NUM_COLOR_CHANNELS		3

#define TILE_PIXELS_WIDE		32
#define TILE_PIXELS_HIGH		32

#define EYE_TILES_WIDE			( DISPLAY_PIXELS_WIDE / TILE_PIXELS_WIDE / NUM_EYES )	// 30*32*2 = 1920
#define EYE_TILES_HIGH			( DISPLAY_PIXELS_HIGH / TILE_PIXELS_HIGH )				// 33*32   = 1056 leaving 24 pixels untouched

static float MaxFloat( float x, float y ) { return ( x > y ) ? x : y; }
static float MinFloat( float x, float y ) { return ( x < y ) ? x : y; }

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
} hmdInfo_t;

const hmdInfo_t * DefaultHmdInfo()
{
	static hmdInfo_t hmdInfo;
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

void BuildDistortionMeshes( MeshCoord_t * meshCoords[NUM_EYES][NUM_COLOR_CHANNELS], const int eyeTilesWide, const int eyeTilesHigh, const hmdInfo_t * hmdInfo )
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
================================================================================================
Test Code
================================================================================================
*/

#if defined( OS_WINDOWS )

#include <stdio.h>			// for fopen()
#include <string.h>			// for memcpy()
#include <conio.h>			// for getch()
#include <malloc.h>			// for _aligned_malloc()
#include <Windows.h>		// for VirtualAlloc(), VirtualFree()

#define USE_DDK		0		// use the Driver Development Kit (DDK)

#elif defined( OS_MAC )

#include <stdio.h>			// for fopen()
#include <string.h>			// for memcpy()
#include <conio.h>			// for getch()
#include <sys/time.h>		// for gettimeofday()

#elif defined( OS_ANDROID )

#include <malloc.h>			// for memalign()
#include <stdio.h>			// for fopen()
#include <stdlib.h>			// for abs()
#include <string.h>			// for memcpy()
#include <time.h>			// for clock_gettime()
#include <ctype.h>			// for isspace() and isdigit()
#include <dlfcn.h>			// for dlopen
#include <fcntl.h>			// for O_RDONLY
#include <sched.h>			// for SCHED_FIFO
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "linux/types.h"
#include "linux/ion.h"

#define USE_ION_MEMORY		1

#else

#include <malloc.h>			// for memalign()
#include <stdio.h>			// for fopen()
#include <stdlib.h>			// for abs()
#include <string.h>			// for memcpy()
#include <sys/time.h>		// for gettimeofday()

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

#if USE_DDK == 1

typedef LARGE_INTEGER PHYSICAL_ADDRESS;

typedef enum _MEMORY_CACHING_TYPE
{ 
	MmNonCached               = 0,
	MmCached                  = 1,
	MmWriteCombined           = 2,
	MmHardwareCoherentCached  = 3,
	MmNonCachedUnordered      = 4,
	MmUSWCCached              = 5,
	MmMaximumCacheType        = 6
} MEMORY_CACHING_TYPE;

PVOID MmAllocateContiguousMemorySpecifyCache(
	_In_ SIZE_T NumberOfBytes,
	_In_ PHYSICAL_ADDRESS LowestAcceptableAddress,
	_In_ PHYSICAL_ADDRESS HighestAcceptableAddress,
	_In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple,
	_In_ MEMORY_CACHING_TYPE CacheType
);

VOID MmFreeContiguousMemory( _In_ PVOID BaseAddress );

#endif // USE_DDK

#if USE_ION_MEMORY == 1

struct mmap_info
{
	struct mmap_info *	next;
	void *				ptr;
	size_t				size;
	int32_t				fd;
	struct ion_fd_data	data;
};

static struct mmap_info * ion_memory;

// Register memory buffers that live on contiguous physical memory and do
// not need to be copied to contiguous physical memory for use by the DSP.
extern void remote_register_buf( void * buf, int size, int fd );
#pragma weak remote_register_buf

static void dsp_register_buf( void * buf, int size, int fd )
{
	if ( remote_register_buf != NULL )
	{
		remote_register_buf( buf, size, fd );
	}
}

#endif	// USE_ION_MEMORY

typedef enum
{
	MEMORY_CACHED,
	MEMORY_WRITE_COMBINED
} CachingType_t;

// Allocates page aligned contiguous physical memory. Memory pages are typically 4kB.
static void * AllocContiguousPhysicalMemory( size_t size, CachingType_t type )
{
#if defined( OS_WINDOWS ) && USE_DDK == 1
	const PHYSICAL_ADDRESS min = { 0x00000000, 0x00000000 };
	const PHYSICAL_ADDRESS max = { 0xFFFFFFFF, 0xFFFFFFFF };
	const PHYSICAL_ADDRESS zero = { 0, 0 };
	const MEMORY_CACHING_TYPE cache = ( type == MEMORY_WRITE_COMBINED ? MmWriteCombined : MmCached );
	return MmAllocateContiguousMemorySpecifyCache( size, min, max, zero, cache );
#elif defined( OS_WINDOWS )
	// NOTE: VirtualAlloc does not allocate contiguous physical memory
	// but at least provides controll over the caching behavior.
	const DWORD flProtect = PAGE_READWRITE | ( type == MEMORY_WRITE_COMBINED ? PAGE_WRITECOMBINE : 0 );
	return VirtualAlloc( NULL, size, MEM_COMMIT | MEM_RESERVE, flProtect );
#elif defined( OS_ANDROID ) && USE_ION_MEMORY == 1
	// NOTE: this implementation is not thread safe due to the use of an unproteced linked list.
	// The Android implementation of ion_heap_map_kernel() in ion_heap.c sets
	// pgprot_t pgprot = pgprot_writecombine(PAGE_KERNEL); when the ION_FLAG_CACHED
	// flag is not specified. However, dependent on whether write-combining is
	// available on the architecture the memory will either be write-combined or
	// non-cacheable. The ARMv7+ based processors used here have write-combining.
	const unsigned int flags = ( type == MEMORY_WRITE_COMBINED ? 0 : ION_FLAG_CACHED );
	const unsigned int align = 4096;

	struct mmap_info * m = (struct mmap_info *)malloc( sizeof( struct mmap_info ) );
	m->next = NULL;
	m->ptr = NULL;
	m->size = ( size + align - 1 ) & ~( align - 1 );
	m->data.handle = 0;
	m->data.fd = 0;
	m->fd = open( "/dev/ion", O_RDONLY );
	if ( m->fd < 0 )
	{
		LOG( "Failed to open /dev/ion\n" );
		return NULL;
	}

	static const uint32_t heapids[] = { 0, 25 };
	for ( int i = 0; i < ARRAY_SIZE( heapids ); i++ )
	{
		struct ion_allocation_data alloc;
		alloc.len = m->size;
		alloc.align = align;
		alloc.heap_id_mask = 1 << heapids[i];
		alloc.flags = flags;

		if ( ioctl( m->fd, ION_IOC_ALLOC, &alloc ) >= 0 )
		{
			//LOG( "Using heapid %d\n", heapids[i] );
			m->data.handle = alloc.handle;
			break;
		}
	}
	if ( m->data.handle == 0 )
	{
		LOG( "Failed to allocate ION memory\n" );
		close( m->fd );
		free( m );
		return NULL;
	}

	if ( ioctl( m->fd, ION_IOC_MAP, &m->data ) < 0 )
	{
		LOG( "Failed to map ION memory\n" );
		ioctl( m->fd, ION_IOC_FREE, &m->data );
		close( m->fd );
		free( m );
		return NULL;
	}

	m->ptr = (void *)mmap( NULL, m->size, PROT_READ|PROT_WRITE, MAP_SHARED, m->data.fd, (off_t)0 );
	if ( m->ptr == MAP_FAILED )
	{
		LOG( "Failed to create virtual mapping\n" );
		ioctl( m->fd, ION_IOC_FREE, &m->data );
		close( m->data.fd );
		close( m->fd );
		free( m );
		return NULL;
	}

	m->next = ion_memory;
	ion_memory = m;

	dsp_register_buf( m->ptr, m->size, m->data.fd );

	return m->ptr;
#else
	// Fall back to regular memory.
	return AllocAlignedMemory( size, 4096 );
#endif
}

static void FreeContiguousPhysicalMemory( void * ptr, size_t size )
{
#if defined( OS_WINDOWS ) && USE_DDK == 1
	size = size;
	MmFreeContiguousMemory( ptr );
#elif defined( OS_WINDOWS )
	VirtualFree( ptr, size, MEM_RELEASE );
#elif defined( OS_ANDROID ) && USE_ION_MEMORY == 1
	size = size;
	for ( struct mmap_info ** m = &ion_memory; (*m) != NULL; m = &((*m)->next) )
	{
		if ( (*m)->ptr == ptr )
		{
			munmap( (*m)->ptr, (*m)->size );
			ioctl( (*m)->fd, ION_IOC_FREE, &(*m)->data );
			close( (*m)->data.fd );
			close( (*m)->fd );
			free( (*m) );
			(*m) = (*m)->next;
			break;
		}
	}
#else
	size = size;
	FreeAlignedMemory( ptr );
#endif
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
		timeBase = (Microseconds_t) li.LowPart + 0xFFFFFFFFLL * li.HighPart;
	}

	LARGE_INTEGER li;
	QueryPerformanceCounter( &li );
	Microseconds_t counter = (Microseconds_t) li.LowPart + 0xFFFFFFFFLL * li.HighPart;
	return ( counter - timeBase ) * 1000000LL / ticksPerSecond;
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
		timeBase = (Microseconds_t) tv.tv_sec * 1000 * 1000;
	}

	return (Microseconds_t) tv.tv_sec * 1000 * 1000 + tv.tv_usec - timeBase;
#endif
}

/*
================================================================================================================================

Threading.

================================================================================================================================
*/

#define THREAD_AFFINITY_BIG_CORES		-1

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
		LOG( "Failed to set thread %p affinity: %s(%d)\n", thread, buffer, error );
	}
	else
	{
		LOG( "Thread %p affinity set to 0x%02X\n", thread, mask );
	}
#elif defined( OS_MAC )
	// OS X does not export interfaces that identify processors or control thread placement.
	// Explicit thread to processor binding is not supported.
	mask = mask;
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
			for ( int j = 0; j < sizeof( files ) / sizeof( files[0] ); j++ )
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
		LOG( "Error sched_setaffinity(%d): thread=(%d) mask=0x%X err=%s(%d)\n", __NR_sched_setaffinity, pid, mask, strerror( err ), err );
	}
	else
	{
		LOG( "Thread %d affinity 0x%02X\n", pid, mask );
	}
#else
	mask = mask;
#endif
}

static void Thread_SetRealTimePriority( int priority )
{
#if defined( OS_WINDOWS )
	priority = priority;
	HANDLE process = GetCurrentProcess();
	if( !SetPriorityClass( process, REALTIME_PRIORITY_CLASS ) )
	{
		char buffer[1024];
		DWORD error = GetLastError();
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), buffer, sizeof( buffer ), NULL );
		LOG( "Failed to set process %p priority class: %s(%d)\n", process, buffer, error );
	}
	else
	{
		LOG( "Process %p priority class set to real-time.\n", process );
	}
	HANDLE thread = GetCurrentThread();
	if ( !SetThreadPriority( thread, THREAD_PRIORITY_TIME_CRITICAL ) )
	{
		char buffer[1024];
		DWORD error = GetLastError();
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), buffer, sizeof( buffer ), NULL );
		LOG( "Failed to set thread %p priority: %s(%d)\n", thread, buffer, error );
	}
	else
	{
		LOG( "Thread %p priority set to critical.\n", thread );
	}
#elif defined( OS_MAC )
	struct sched_param sp;
	memset( &sp, 0, sizeof( struct sched_param ) );
	sp.sched_priority = priority;
	if ( pthread_setschedparam( pthread_self(), SCHED_FIFO, &sp ) == -1 )
	{
		LOG( "Failed to change thread %d priority.\n", gettid() );
	}
	else
	{
		Print( "Thread %d set to SCHED_FIFO, priority=%d\n", gettid(), priority );
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
		LOG( "Error sched_setattr(%d): thread=%d err=%s(%d)\n", __NR_sched_setattr, pid, strerror( err ), err );
	}
	else
	{
		LOG( "Thread %d set to SCHED_FIFO, priority=%d\n", pid, priority );
	}
#else
	UNUSED_PARM( priority );
#endif
}

/*
================================================================================================================================

Test code.

================================================================================================================================
*/

void CreateTestPattern( unsigned char * rgba, const int width, const int height )
{
	const unsigned char colors[4][4] =
	{
		{ 0xFF, 0x00, 0x00, 0xFF },
		{ 0x00, 0xFF, 0x00, 0xFF },
		{ 0x00, 0x00, 0xFF, 0xFF },
		{ 0xFF, 0xFF, 0x00, 0xFF }
	};
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

			for ( int c = 0; c < 3; c++ )
			{
				rgba[( y * width + x ) * 4 + c] = (unsigned char)( ( colors[index][c] * scale ) >> 5 );
			}
			rgba[( y * width + x ) * 4 + 3] = 0;
		}
	}

	unsigned int * intPtr = (unsigned int *)rgba;
	const int borderWidth = 32;

	for ( int i = 0; i < borderWidth; i++ )
	{
		for ( int j = 0; j < width; j++ )
		{
			intPtr[( ( i              ) * width + j )] = 0;
			intPtr[( ( height - 1 - i ) * width + j )] = 0;
		}
	}

	for ( int i = 0; i < height; i++ )
	{
		for ( int j = 0; j < borderWidth; j++ )
		{
			intPtr[( i * width + ( j             ) )] = 0;
			intPtr[( i * width + ( width - 1 - j ) )] = 0;
		}
	}
}

void WriteTGA( const char * fileName, const unsigned char * rgba, const int width, const int height )
{
	enum
	{
		TGA_IMAGETYPE_COLORMAP			= 1,
		TGA_IMAGETYPE_BGR				= 2,
		TGA_IMAGETYPE_GRAYSCALE			= 3,
		TGA_IMAGETYPE_RLE_COLORMAP		= 9,
		TGA_IMAGETYPE_RLE_BGR			= 10,
		TGA_IMAGETYPE_RLE_GRAYSCALE		= 11,
	};
	enum
	{
		TGA_ATTRIBUTE_ABITS				= 0x0F,
		TGA_ATTRIBUTE_FLIP_HORIZONTAL	= 0x10,
		TGA_ATTRIBUTE_FLIP_VERTICAL		= 0x20
	};

	unsigned char header[18] = { 0 };
	header[ 0] = 0;								// id_length
	header[ 1] = 0;								// colormap_type
	header[ 2] = TGA_IMAGETYPE_BGR;				// image_type
	header[ 3] = 0;								// colormap_index
	header[ 4] = 0;								// "
	header[ 5] = 0;								// colormap_length
	header[ 6] = 0;								// "
	header[ 7] = 0;								// colormap_size
	header[ 8] = 0;								// x_origin
	header[ 9] = 0;								// "
	header[10] = 0;								// y_origin
	header[11] = 0;								// "
	header[12] = ( ( width  >> 0 ) & 0xFF );	// width
	header[13] = ( ( width  >> 8 ) & 0xFF );	// "
	header[14] = ( ( height >> 0 ) & 0xFF );	// height
	header[15] = ( ( height >> 8 ) & 0xFF );	// "
	header[16] = 4 * 8;							// pixel_size
	header[17] = TGA_ATTRIBUTE_FLIP_VERTICAL;	// attributes

	FILE * fp = fopen( fileName, "wb" );
	if ( fp == NULL )
	{
		LOG( "Failed to open %s\n", fileName );
		return;
	}

	if ( fwrite( &header, sizeof( header ), 1, fp ) != 1 ) {
		LOG( "Failed to write TGA header to %s\n", fileName );
		fclose( fp );
		return;
	}

	const int totalPixels = width * height;
	for ( int i = 0; i < totalPixels; i += 1024 )
	{
		// First linearly copy to a temporary buffer before swizzling to BGRA in
		// case the source data lives on write-combined memory.
		unsigned char buffer[1024 * 4];
		const int numPixels = ( totalPixels - i < 1024 ) ? totalPixels - i : 1024;
		memcpy( buffer, rgba + i * 4, numPixels * 4 );
		for ( int j = 0; j < numPixels; j++ )
		{
			unsigned char c = buffer[j * 4 + 0];
			buffer[j * 4 + 0] = buffer[j * 4 + 2];
			buffer[j * 4 + 2] = c;
		}
		if ( fwrite( buffer, numPixels * 4, 1, fp ) != 1 )
		{
			LOG( "Failed to write TGA data to %s\n", fileName );
			fclose( fp );
			return;
		}
	}

	fclose( fp );
}

void TestTimeWarp()
{
	int srcTexelsWide = 1024;
	int srcTexelsHigh = 1024;
	int srcPitchInTexels = srcTexelsWide;
	unsigned char * src = (unsigned char *)AllocAlignedMemory( srcTexelsWide * srcTexelsHigh * 4 * sizeof( unsigned char ), 128 );

	CreateTestPattern( src, srcTexelsWide, srcTexelsHigh );

	const size_t packedSize = srcTexelsWide * srcTexelsHigh * 4 * sizeof( unsigned char );
	unsigned char * packedRGB = (unsigned char *)AllocContiguousPhysicalMemory( packedSize, MEMORY_CACHED );
	unsigned char * planarR = packedRGB + 0 * srcTexelsWide * srcTexelsHigh;
	unsigned char * planarG = packedRGB + 1 * srcTexelsWide * srcTexelsHigh;
	unsigned char * planarB = packedRGB + 2 * srcTexelsWide * srcTexelsHigh;

	const size_t numMeshCoords = ( EYE_TILES_WIDE + 1 ) * ( EYE_TILES_HIGH + 1 );
	const size_t meshSize = ( NUM_EYES + 1 ) * NUM_COLOR_CHANNELS * numMeshCoords * sizeof( MeshCoord_t );
	MeshCoord_t * meshCoordsBasePtr = (MeshCoord_t *)AllocContiguousPhysicalMemory( meshSize, MEMORY_CACHED );
	MeshCoord_t * meshCoords[NUM_EYES][NUM_COLOR_CHANNELS] =
	{
		{ meshCoordsBasePtr + 0 * numMeshCoords, meshCoordsBasePtr + 1 * numMeshCoords, meshCoordsBasePtr + 2 * numMeshCoords },
		{ meshCoordsBasePtr + 3 * numMeshCoords, meshCoordsBasePtr + 4 * numMeshCoords, meshCoordsBasePtr + 5 * numMeshCoords }
	};
	MeshCoord_t * tempMeshCoords[NUM_COLOR_CHANNELS] =
	{
		meshCoordsBasePtr + 6 * numMeshCoords,
		meshCoordsBasePtr + 7 * numMeshCoords,
		meshCoordsBasePtr + 8 * numMeshCoords
	};

	BuildDistortionMeshes( meshCoords, EYE_TILES_WIDE, EYE_TILES_HIGH, DefaultHmdInfo() );

	const int dstSize = DISPLAY_PIXELS_WIDE * DISPLAY_PIXELS_HIGH * 4 * sizeof( unsigned char );
	unsigned char * dst = (unsigned char *) AllocContiguousPhysicalMemory( dstSize, MEMORY_WRITE_COMBINED );

	Matrix4x4f_t renderProjectionMatrix;
	Matrix4x4f_CreateProjectionFov( &renderProjectionMatrix, 80.0f, 80.0f, 0.0f, 0.0f, 0.1f, 0.0f );

	Matrix4x4f_t renderViewMatrix;
	Matrix4x4f_CreateIdentity( &renderViewMatrix );

	Matrix4x4f_t displayRefreshStartViewMatrix;
	Matrix4x4f_t displayRefreshEndViewMatrix;
	Matrix4x4f_CreateIdentity( &displayRefreshStartViewMatrix );
	Matrix4x4f_CreateIdentity( &displayRefreshEndViewMatrix );

	Matrix4x4f_t timeWarpStartTransform;
	Matrix4x4f_t timeWarpEndTransform;
	CalculateTimeWarpTransform( &timeWarpStartTransform, &renderProjectionMatrix, &renderViewMatrix, &displayRefreshStartViewMatrix );
	CalculateTimeWarpTransform( &timeWarpEndTransform, &renderProjectionMatrix, &renderViewMatrix, &displayRefreshEndViewMatrix );

	for ( int sampling = 0; sampling < 5; sampling++ )
	{
		if ( sampling >= 0 && sampling <= 2 )
		{
			for ( int i = 0; i < srcTexelsWide * srcTexelsHigh; i++ )
			{
				const unsigned char r = src[i * 4 + 0];
				const unsigned char g = src[i * 4 + 1];
				const unsigned char b = src[i * 4 + 2];
				const unsigned char a = src[i * 4 + 3];

				packedRGB[i * 4 + 0] = r;
				packedRGB[i * 4 + 1] = g;
				packedRGB[i * 4 + 2] = b;
				packedRGB[i * 4 + 3] = a;
			}
		}
		else if ( sampling >= 3 && sampling <= 4 )
		{
			for ( int i = 0; i < srcTexelsWide * srcTexelsHigh; i++ )
			{
				const unsigned char r = src[i * 4 + 0];
				const unsigned char g = src[i * 4 + 1];
				const unsigned char b = src[i * 4 + 2];

				planarR[i] = r;
				planarG[i] = g;
				planarB[i] = b;
			}
		}
		memset( dst, 0, dstSize );

		Microseconds_t bestTime = 0xFFFFFFFFFFFFFFFF;

		for ( int i = 0; i < 25; i++ )
		{
			Microseconds_t start = GetTimeMicroseconds();

			for ( int eye = 0; eye < NUM_EYES; eye++ )
			{
				unsigned char * eyeDest = dst + eye * ( DISPLAY_PIXELS_WIDE / NUM_EYES ) * 4;

				if ( sampling == 0 )
				{
					TimeWarp_SampleNearestPackedRGB( packedRGB, srcPitchInTexels, srcTexelsWide, srcTexelsHigh,
													eyeDest, DISPLAY_PIXELS_WIDE, EYE_TILES_WIDE, EYE_TILES_HIGH, eye,
													meshCoords[eye][1], tempMeshCoords[1],
													&timeWarpStartTransform, &timeWarpEndTransform );
				}
				else if ( sampling == 1 )
				{
					TimeWarp_SampleLinearPackedRGB( packedRGB, srcPitchInTexels, srcTexelsWide, srcTexelsHigh,
													eyeDest, DISPLAY_PIXELS_WIDE, EYE_TILES_WIDE, EYE_TILES_HIGH, eye,
													meshCoords[eye][1], tempMeshCoords[1],
													&timeWarpStartTransform, &timeWarpEndTransform );
				}
				else if ( sampling == 2 )
				{
					TimeWarp_SampleBilinearPackedRGB( packedRGB, srcPitchInTexels, srcTexelsWide, srcTexelsHigh,
													eyeDest, DISPLAY_PIXELS_WIDE, EYE_TILES_WIDE, EYE_TILES_HIGH, eye,
													meshCoords[eye][1], tempMeshCoords[1],
													&timeWarpStartTransform, &timeWarpEndTransform );
				}
				else if ( sampling == 3 )
				{
					TimeWarp_SampleBilinearPlanarRGB( planarR, planarG, planarB,
													srcPitchInTexels, srcTexelsWide, srcTexelsHigh,
													eyeDest, DISPLAY_PIXELS_WIDE, EYE_TILES_WIDE, EYE_TILES_HIGH, eye,
													meshCoords[eye][1], tempMeshCoords[1],
													&timeWarpStartTransform, &timeWarpEndTransform );
				}
				else if ( sampling == 4 )
				{
					TimeWarp_SampleChromaticBilinearPlanarRGB( planarR, planarG, planarB,
													srcPitchInTexels, srcTexelsWide, srcTexelsHigh,
													eyeDest, DISPLAY_PIXELS_WIDE, EYE_TILES_WIDE, EYE_TILES_HIGH, eye,
													meshCoords[eye][0], meshCoords[eye][1], meshCoords[eye][2],
													tempMeshCoords[0], tempMeshCoords[1], tempMeshCoords[2],
													&timeWarpStartTransform, &timeWarpEndTransform );
				}
			}

			Microseconds_t end = GetTimeMicroseconds();

			if ( end - start < bestTime )
			{
				bestTime = end - start;
			}
		}

		const char * string = "";
		switch ( sampling )
		{
			case 0: string = "nearest-packed-RGBA"; break;
			case 1: string = "linear-packed-RGBA"; break;
			case 2: string = "bilinear-packed-RGBA"; break;
			case 3: string = "bilinear-planar-RGB"; break;
			case 4: string = "chromatic-planar-RGB"; break;
		}

		LOG( "%22s = %5.1f milliseconds (%1.0f Mpixels/sec)\n",
				string,
				bestTime / 1000.0f,
				2.0f * EYE_TILES_WIDE * EYE_TILES_HIGH * 32 * 32 / bestTime );

		char fileName[1024];
		sprintf( fileName, OUTPUT "warped-%d-%s.tga", sampling, string );
		WriteTGA( fileName, dst, DISPLAY_PIXELS_WIDE, DISPLAY_PIXELS_HIGH );
	}

	FreeContiguousPhysicalMemory( dst, dstSize );
	FreeContiguousPhysicalMemory( packedRGB, packedSize );
	FreeContiguousPhysicalMemory( meshCoordsBasePtr, meshSize );
	FreeAlignedMemory( src );
}

int main( int argc, char * argv[] )
{
	argc = argc;
	argv = argv;

	LOG( "--------------------------------\n" );
	LOG( "OS     : %s\n", GetOSVersion() );
	LOG( "CPU    : %s\n", GetCPUVersion() );
	LOG( "Mode   : %dx%d\n", DISPLAY_PIXELS_WIDE, DISPLAY_PIXELS_HIGH );
	LOG( "--------------------------------\n" );

	Thread_SetAffinity( THREAD_AFFINITY_BIG_CORES );
	Thread_SetRealTimePriority( 1 );

	LOG( "--------------------------------\n" );

	TestTimeWarp();

	LOG( "--------------------------------\n" );

#if !defined( OS_ANDROID )
	LOG( "Press any key to continue.\n" );
	_getch();
#endif
}

#endif	// !defined( OS_HEXAGON )

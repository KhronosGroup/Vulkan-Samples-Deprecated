/*
================================================================================================

Description	:	Scene settings.
Author		:	J.M.P. van Waveren
Date		:	01/12/2017
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


INTERFACE
=========

ksSceneSettings

static void ksSceneSettings_Init( ksGpuContext * context, ksSceneSettings * settings );
static void ksSceneSettings_SetGltf( ksSceneSettings * settings, const char * fileName );
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

================================================================================================
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

	ksGpuLimits limits;
	ksGpuContext_GetLimits( context, &limits );

	const int maxEyeImageSamplesLevels = IntegerLog2( limits.maxSamples + 1 );
	settings->maxEyeImageSamplesLevels = ( maxEyeImageSamplesLevels < MAX_EYE_IMAGE_SAMPLES_LEVELS ) ? maxEyeImageSamplesLevels : MAX_EYE_IMAGE_SAMPLES_LEVELS;
}

static void CycleLevel( int * x, const int max ) { (*x) = ( (*x) + 1 ) % max; }

static void ksSceneSettings_SetGltf( ksSceneSettings * settings, const char * fileName ) { settings->glTF = fileName; }

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

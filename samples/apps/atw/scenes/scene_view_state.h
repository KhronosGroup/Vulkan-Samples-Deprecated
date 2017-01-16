/*
================================================================================================

Description	:	Scene view state.
Author		:	J.M.P. van Waveren
Date		:	01/12/2017
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


INTERFACE
=========

ksViewState

static void ksViewState_Init( ksViewState * viewState, const float interpupillaryDistance );
static void ksViewState_HandleInput( ksViewState * viewState, ksGpuWindowInput * input, const ksNanoseconds time );
static void ksViewState_HandleHmd( ksViewState * viewState, const ksNanoseconds time );

================================================================================================
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

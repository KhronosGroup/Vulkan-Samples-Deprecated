/*
================================================================================================

Description	:	glTF scene rendering.
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


DESCRIPTION
===========

Set one of the following defines to one before including this header file to
load a glTF scene for a particular graphics API.

#define GRAPHICS_API_OPENGL		0
#define GRAPHICS_API_OPENGL_ES	0
#define GRAPHICS_API_VULKAN		0
#define GRAPHICS_API_D3D		0
#define GRAPHICS_API_METAL		0

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

glTF 1.0 is not perfect.

Incorrect usage of JSON:
	- As opposed to using arrays with elements with a name property,
	  glTF uses objects with arbitrarily named members. Normally JSON
	  objects are well-defined with well-defined member names.
	  As a result, a JSON parser is needed that cannot just lookup
	  object members by name, but can also iterate over the object
	  members as if they are array elements. Not all JSON parsers
	  support this. JSON parsers that do support this behavior are
	  typically not optimized for objects with many members.
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
	- The pipeline stage of a shader is called "shader.type". Why isn't it
	  called "shader.stage"?
There are also obvious things missing:
	- There is no requirement that a buffer holds only one type of data.
	  A buffer may mix vertex attribute arrays, index arrays, animation
	  data and other data. In other words, you cannot just create one
	  graphics API buffer and reference it. You could instead create
	  graphics API buffers based on bufferViews, but there are typically
	  separate bufferViews for every separate piece of data that is used.
	  The bufferView.target member is also not a required member so a
	  bufferView may still not say anything about what kind of data it holds.
	- glTF 1.0 is limited to GLSL 1.00 ES which means there is no support for
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
	- Time-lines are evaluated separately so they can be shared by different
	  animations.
	- The bindShapeMatrix is folded into the inverseBindMatrices at load
	  time to avoid another run-time matrix multiplication.
	- This implementation supports culling of animated models using the
	  KHR_skin_culling glTF extension.
	- The nodes are sorted to allow a simple linear walk to transform
	  nodes from local space to global space.


INTERFACE
=========

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

================================================================================================================================
*/

#include <utils/json.h>
#include <utils/base64.h>
#include <utils/lexer.h>

#if GRAPHICS_API_OPENGL == 0 && GRAPHICS_API_OPENGLES == 0

#define GL_BYTE							0x1400
#define GL_UNSIGNED_BYTE				0x1401
#define GL_SHORT						0x1402
#define GL_UNSIGNED_SHORT				0x1403

#define GL_BOOL							0x8B56
#define GL_BOOL_VEC2					0x8B57
#define GL_BOOL_VEC3					0x8B58
#define GL_BOOL_VEC4					0x8B59
#define GL_INT							0x1404
#define GL_INT_VEC2						0x8B53
#define GL_INT_VEC3						0x8B54
#define GL_INT_VEC4						0x8B55
#define GL_FLOAT						0x1406
#define GL_FLOAT_VEC2					0x8B50
#define GL_FLOAT_VEC3					0x8B51
#define GL_FLOAT_VEC4					0x8B52
#define GL_FLOAT_MAT2					0x8B5A
#define GL_FLOAT_MAT2x3					0x8B65
#define GL_FLOAT_MAT2x4					0x8B66
#define GL_FLOAT_MAT3x2					0x8B67
#define GL_FLOAT_MAT3					0x8B5B
#define GL_FLOAT_MAT3x4					0x8B68
#define GL_FLOAT_MAT4x2					0x8B69
#define GL_FLOAT_MAT4x3					0x8B6A
#define GL_FLOAT_MAT4					0x8B5C
#define GL_SAMPLER_2D					0x8B5E
#define GL_SAMPLER_3D					0x8B5F
#define GL_SAMPLER_CUBE					0x8B60

#define GL_TEXTURE_1D					0x0DE0
#define GL_TEXTURE_2D					0x0DE1
#define GL_TEXTURE_3D					0x806F
#define GL_TEXTURE_CUBE_MAP				0x8513
#define GL_TEXTURE_1D_ARRAY				0x8C18
#define GL_TEXTURE_2D_ARRAY				0x8C1A
#define GL_TEXTURE_CUBE_MAP_ARRAY		0x9009

#define GL_NEAREST						0x2600
#define GL_LINEAR						0x2601
#define GL_NEAREST_MIPMAP_NEAREST		0x2700
#define GL_LINEAR_MIPMAP_NEAREST		0x2701
#define GL_NEAREST_MIPMAP_LINEAR		0x2702
#define GL_LINEAR_MIPMAP_LINEAR			0x2703

#define GL_REPEAT						0x2901
#define GL_CLAMP_TO_EDGE				0x812F
#define GL_CLAMP_TO_BORDER				0x812D

#define GL_VERTEX_SHADER				0x8B31
#define GL_FRAGMENT_SHADER				0x8B30

#define GL_BLEND						0x0BE2
#define GL_DEPTH_TEST					0x0B71
#define GL_DEPTH_WRITEMASK				0x0B72
#define GL_CULL_FACE					0x0B44
#define GL_POLYGON_OFFSET_FILL			0x8037
#define GL_SAMPLE_ALPHA_TO_COVERAGE		0x809E
#define GL_SCISSOR_TEST					0x0C11

#define GL_CW							0x0900
#define GL_CCW							0x0901

#define GL_NONE							0
#define GL_FRONT						0x0404
#define GL_BACK							0x0405

#define GL_NEVER						0x0200
#define GL_LESS							0x0201
#define GL_EQUAL						0x0202
#define GL_LEQUAL						0x0203
#define GL_GREATER						0x0204
#define GL_NOTEQUAL						0x0205
#define GL_GEQUAL						0x0206
#define GL_ALWAYS						0x0207

#define GL_FUNC_ADD						0x8006
#define GL_FUNC_SUBTRACT				0x800A
#define GL_FUNC_REVERSE_SUBTRACT		0x800B
#define GL_MIN							0x8007
#define GL_MAX							0x8008

#define GL_ZERO							0
#define GL_ONE							1
#define GL_SRC_COLOR					0x0300
#define GL_ONE_MINUS_SRC_COLOR			0x0301
#define GL_DST_COLOR					0x0306
#define GL_ONE_MINUS_DST_COLOR			0x0307
#define GL_SRC_ALPHA					0x0302
#define GL_ONE_MINUS_SRC_ALPHA			0x0303
#define GL_DST_ALPHA					0x0304
#define GL_ONE_MINUS_DST_ALPHA			0x0305
#define GL_CONSTANT_COLOR				0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR		0x8002
#define GL_CONSTANT_ALPHA				0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA		0x8004
#define GL_SRC_ALPHA_SATURATE			0x0308

#endif

#if GRAPHICS_API_OPENGL == 1 || GRAPHICS_API_OPENGLES == 1

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

#elif GRAPHICS_API_VULKAN == 1

static ksGpuProgramParm unitCubeFlatShadeProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	0,	"ModelMatrix",		  0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	1,	"ViewMatrix",		 64 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	2,	"ProjectionMatrix",	128 }
};

static const char unitCubeFlatShadeVertexProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
	GLSL_EXTENSIONS
	"layout( location = 0 ) in vec3 vertexPosition;\n"
	"layout( location = 1 ) in vec3 vertexNormal;\n"
	"layout( std430, push_constant ) uniform PushConstants\n"
	"{\n"
	"	layout( offset =   0 ) mat4 ModelMatrix;\n"
	"	layout( offset =  64 ) mat4 ViewMatrix;\n"
	"	layout( offset = 128 ) mat4 ProjectionMatrix;\n"
	"} pc;\n"
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
	"	vec3 eyeWorldPos = transposeMultiply3x3( pc.ViewMatrix, -vec3( pc.ViewMatrix[3] ) );\n"
	"	gl_Position = pc.ProjectionMatrix * ( pc.ViewMatrix * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( pc.ModelMatrix, vertexNormal );\n"
	"}\n";

static const char unitCubeFlatShadeFragmentProgramGLSL[] =
	"#version " GLSL_VERSION "\n"
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

static const unsigned int unitCubeFlatShadeVertexProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x000000c7,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x00000093,0x000000ac,0x000000b7,
	0x000000bf,0x000000c0,0x00030003,0x00000001,0x00000136,0x00070004,0x415f4c47,0x655f4252,
	0x6e61686e,0x5f646563,0x6f79616c,0x00737475,0x00070004,0x455f4c47,0x735f5458,0x65646168,
	0x6f695f72,0x6f6c625f,0x00736b63,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00080005,
	0x0000000f,0x746c756d,0x796c7069,0x28337833,0x3434666d,0x3366763b,0x0000003b,0x00030005,
	0x0000000d,0x0000006d,0x00030005,0x0000000e,0x00000076,0x000a0005,0x00000013,0x6e617274,
	0x736f7073,0x6c754d65,0x6c706974,0x33783379,0x34666d28,0x66763b34,0x00003b33,0x00030005,
	0x00000011,0x0000006d,0x00030005,0x00000012,0x00000076,0x00060005,0x0000008b,0x74726576,
	0x6f577865,0x50646c72,0x0000736f,0x00060005,0x0000008c,0x68737550,0x736e6f43,0x746e6174,
	0x00000073,0x00060006,0x0000008c,0x00000000,0x65646f4d,0x74614d6c,0x00786972,0x00060006,
	0x0000008c,0x00000001,0x77656956,0x7274614d,0x00007869,0x00080006,0x0000008c,0x00000002,
	0x6a6f7250,0x69746365,0x614d6e6f,0x78697274,0x00000000,0x00030005,0x0000008e,0x00006370,
	0x00060005,0x00000093,0x74726576,0x6f507865,0x69746973,0x00006e6f,0x00050005,0x0000009b,
	0x57657965,0x646c726f,0x00736f50,0x00040005,0x000000a5,0x61726170,0x0000006d,0x00040005,
	0x000000a8,0x61726170,0x0000006d,0x00060005,0x000000aa,0x505f6c67,0x65567265,0x78657472,
	0x00000000,0x00060006,0x000000aa,0x00000000,0x505f6c67,0x7469736f,0x006e6f69,0x00030005,
	0x000000ac,0x00000000,0x00060005,0x000000b7,0x67617266,0x746e656d,0x44657945,0x00007269,
	0x00060005,0x000000bf,0x67617266,0x746e656d,0x6d726f4e,0x00006c61,0x00060005,0x000000c0,
	0x74726576,0x6f4e7865,0x6c616d72,0x00000000,0x00040005,0x000000c1,0x61726170,0x0000006d,
	0x00040005,0x000000c4,0x61726170,0x0000006d,0x00040048,0x0000008c,0x00000000,0x00000005,
	0x00050048,0x0000008c,0x00000000,0x00000023,0x00000000,0x00050048,0x0000008c,0x00000000,
	0x00000007,0x00000010,0x00040048,0x0000008c,0x00000001,0x00000005,0x00050048,0x0000008c,
	0x00000001,0x00000023,0x00000040,0x00050048,0x0000008c,0x00000001,0x00000007,0x00000010,
	0x00040048,0x0000008c,0x00000002,0x00000005,0x00050048,0x0000008c,0x00000002,0x00000023,
	0x00000080,0x00050048,0x0000008c,0x00000002,0x00000007,0x00000010,0x00030047,0x0000008c,
	0x00000002,0x00040047,0x00000093,0x0000001e,0x00000000,0x00050048,0x000000aa,0x00000000,
	0x0000000b,0x00000000,0x00030047,0x000000aa,0x00000002,0x00040047,0x000000b7,0x0000001e,
	0x00000000,0x00040047,0x000000bf,0x0000001e,0x00000001,0x00040047,0x000000c0,0x0000001e,
	0x00000001,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
	0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040018,0x00000008,0x00000007,
	0x00000004,0x00040020,0x00000009,0x00000007,0x00000008,0x00040017,0x0000000a,0x00000006,
	0x00000003,0x00040020,0x0000000b,0x00000007,0x0000000a,0x00050021,0x0000000c,0x0000000a,
	0x00000009,0x0000000b,0x00040015,0x00000015,0x00000020,0x00000001,0x0004002b,0x00000015,
	0x00000016,0x00000000,0x00040015,0x00000017,0x00000020,0x00000000,0x0004002b,0x00000017,
	0x00000018,0x00000000,0x00040020,0x00000019,0x00000007,0x00000006,0x0004002b,0x00000015,
	0x0000001f,0x00000001,0x0004002b,0x00000017,0x00000022,0x00000001,0x0004002b,0x00000015,
	0x00000027,0x00000002,0x0004002b,0x00000017,0x0000002a,0x00000002,0x00040020,0x0000008a,
	0x00000007,0x00000007,0x0005001e,0x0000008c,0x00000008,0x00000008,0x00000008,0x00040020,
	0x0000008d,0x00000009,0x0000008c,0x0004003b,0x0000008d,0x0000008e,0x00000009,0x00040020,
	0x0000008f,0x00000009,0x00000008,0x00040020,0x00000092,0x00000001,0x0000000a,0x0004003b,
	0x00000092,0x00000093,0x00000001,0x0004002b,0x00000006,0x00000095,0x3f800000,0x0004002b,
	0x00000015,0x0000009c,0x00000003,0x00040020,0x0000009d,0x00000009,0x00000007,0x0003001e,
	0x000000aa,0x00000007,0x00040020,0x000000ab,0x00000003,0x000000aa,0x0004003b,0x000000ab,
	0x000000ac,0x00000003,0x00040020,0x000000b4,0x00000003,0x00000007,0x00040020,0x000000b6,
	0x00000003,0x0000000a,0x0004003b,0x000000b6,0x000000b7,0x00000003,0x0004003b,0x000000b6,
	0x000000bf,0x00000003,0x0004003b,0x00000092,0x000000c0,0x00000001,0x00050036,0x00000002,
	0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x0000008a,0x0000008b,
	0x00000007,0x0004003b,0x0000000b,0x0000009b,0x00000007,0x0004003b,0x00000009,0x000000a5,
	0x00000007,0x0004003b,0x0000000b,0x000000a8,0x00000007,0x0004003b,0x00000009,0x000000c1,
	0x00000007,0x0004003b,0x0000000b,0x000000c4,0x00000007,0x00050041,0x0000008f,0x00000090,
	0x0000008e,0x00000016,0x0004003d,0x00000008,0x00000091,0x00000090,0x0004003d,0x0000000a,
	0x00000094,0x00000093,0x00050051,0x00000006,0x00000096,0x00000094,0x00000000,0x00050051,
	0x00000006,0x00000097,0x00000094,0x00000001,0x00050051,0x00000006,0x00000098,0x00000094,
	0x00000002,0x00070050,0x00000007,0x00000099,0x00000096,0x00000097,0x00000098,0x00000095,
	0x00050091,0x00000007,0x0000009a,0x00000091,0x00000099,0x0003003e,0x0000008b,0x0000009a,
	0x00060041,0x0000009d,0x0000009e,0x0000008e,0x0000001f,0x0000009c,0x0004003d,0x00000007,
	0x0000009f,0x0000009e,0x00050051,0x00000006,0x000000a0,0x0000009f,0x00000000,0x00050051,
	0x00000006,0x000000a1,0x0000009f,0x00000001,0x00050051,0x00000006,0x000000a2,0x0000009f,
	0x00000002,0x00060050,0x0000000a,0x000000a3,0x000000a0,0x000000a1,0x000000a2,0x0004007f,
	0x0000000a,0x000000a4,0x000000a3,0x00050041,0x0000008f,0x000000a6,0x0000008e,0x0000001f,
	0x0004003d,0x00000008,0x000000a7,0x000000a6,0x0003003e,0x000000a5,0x000000a7,0x0003003e,
	0x000000a8,0x000000a4,0x00060039,0x0000000a,0x000000a9,0x00000013,0x000000a5,0x000000a8,
	0x0003003e,0x0000009b,0x000000a9,0x00050041,0x0000008f,0x000000ad,0x0000008e,0x00000027,
	0x0004003d,0x00000008,0x000000ae,0x000000ad,0x00050041,0x0000008f,0x000000af,0x0000008e,
	0x0000001f,0x0004003d,0x00000008,0x000000b0,0x000000af,0x0004003d,0x00000007,0x000000b1,
	0x0000008b,0x00050091,0x00000007,0x000000b2,0x000000b0,0x000000b1,0x00050091,0x00000007,
	0x000000b3,0x000000ae,0x000000b2,0x00050041,0x000000b4,0x000000b5,0x000000ac,0x00000016,
	0x0003003e,0x000000b5,0x000000b3,0x0004003d,0x0000000a,0x000000b8,0x0000009b,0x0004003d,
	0x00000007,0x000000b9,0x0000008b,0x00050051,0x00000006,0x000000ba,0x000000b9,0x00000000,
	0x00050051,0x00000006,0x000000bb,0x000000b9,0x00000001,0x00050051,0x00000006,0x000000bc,
	0x000000b9,0x00000002,0x00060050,0x0000000a,0x000000bd,0x000000ba,0x000000bb,0x000000bc,
	0x00050083,0x0000000a,0x000000be,0x000000b8,0x000000bd,0x0003003e,0x000000b7,0x000000be,
	0x00050041,0x0000008f,0x000000c2,0x0000008e,0x00000016,0x0004003d,0x00000008,0x000000c3,
	0x000000c2,0x0003003e,0x000000c1,0x000000c3,0x0004003d,0x0000000a,0x000000c5,0x000000c0,
	0x0003003e,0x000000c4,0x000000c5,0x00060039,0x0000000a,0x000000c6,0x0000000f,0x000000c1,
	0x000000c4,0x0003003e,0x000000bf,0x000000c6,0x000100fd,0x00010038,0x00050036,0x0000000a,
	0x0000000f,0x00000000,0x0000000c,0x00030037,0x00000009,0x0000000d,0x00030037,0x0000000b,
	0x0000000e,0x000200f8,0x00000010,0x00060041,0x00000019,0x0000001a,0x0000000d,0x00000016,
	0x00000018,0x0004003d,0x00000006,0x0000001b,0x0000001a,0x00050041,0x00000019,0x0000001c,
	0x0000000e,0x00000018,0x0004003d,0x00000006,0x0000001d,0x0000001c,0x00050085,0x00000006,
	0x0000001e,0x0000001b,0x0000001d,0x00060041,0x00000019,0x00000020,0x0000000d,0x0000001f,
	0x00000018,0x0004003d,0x00000006,0x00000021,0x00000020,0x00050041,0x00000019,0x00000023,
	0x0000000e,0x00000022,0x0004003d,0x00000006,0x00000024,0x00000023,0x00050085,0x00000006,
	0x00000025,0x00000021,0x00000024,0x00050081,0x00000006,0x00000026,0x0000001e,0x00000025,
	0x00060041,0x00000019,0x00000028,0x0000000d,0x00000027,0x00000018,0x0004003d,0x00000006,
	0x00000029,0x00000028,0x00050041,0x00000019,0x0000002b,0x0000000e,0x0000002a,0x0004003d,
	0x00000006,0x0000002c,0x0000002b,0x00050085,0x00000006,0x0000002d,0x00000029,0x0000002c,
	0x00050081,0x00000006,0x0000002e,0x00000026,0x0000002d,0x00060041,0x00000019,0x0000002f,
	0x0000000d,0x00000016,0x00000022,0x0004003d,0x00000006,0x00000030,0x0000002f,0x00050041,
	0x00000019,0x00000031,0x0000000e,0x00000018,0x0004003d,0x00000006,0x00000032,0x00000031,
	0x00050085,0x00000006,0x00000033,0x00000030,0x00000032,0x00060041,0x00000019,0x00000034,
	0x0000000d,0x0000001f,0x00000022,0x0004003d,0x00000006,0x00000035,0x00000034,0x00050041,
	0x00000019,0x00000036,0x0000000e,0x00000022,0x0004003d,0x00000006,0x00000037,0x00000036,
	0x00050085,0x00000006,0x00000038,0x00000035,0x00000037,0x00050081,0x00000006,0x00000039,
	0x00000033,0x00000038,0x00060041,0x00000019,0x0000003a,0x0000000d,0x00000027,0x00000022,
	0x0004003d,0x00000006,0x0000003b,0x0000003a,0x00050041,0x00000019,0x0000003c,0x0000000e,
	0x0000002a,0x0004003d,0x00000006,0x0000003d,0x0000003c,0x00050085,0x00000006,0x0000003e,
	0x0000003b,0x0000003d,0x00050081,0x00000006,0x0000003f,0x00000039,0x0000003e,0x00060041,
	0x00000019,0x00000040,0x0000000d,0x00000016,0x0000002a,0x0004003d,0x00000006,0x00000041,
	0x00000040,0x00050041,0x00000019,0x00000042,0x0000000e,0x00000018,0x0004003d,0x00000006,
	0x00000043,0x00000042,0x00050085,0x00000006,0x00000044,0x00000041,0x00000043,0x00060041,
	0x00000019,0x00000045,0x0000000d,0x0000001f,0x0000002a,0x0004003d,0x00000006,0x00000046,
	0x00000045,0x00050041,0x00000019,0x00000047,0x0000000e,0x00000022,0x0004003d,0x00000006,
	0x00000048,0x00000047,0x00050085,0x00000006,0x00000049,0x00000046,0x00000048,0x00050081,
	0x00000006,0x0000004a,0x00000044,0x00000049,0x00060041,0x00000019,0x0000004b,0x0000000d,
	0x00000027,0x0000002a,0x0004003d,0x00000006,0x0000004c,0x0000004b,0x00050041,0x00000019,
	0x0000004d,0x0000000e,0x0000002a,0x0004003d,0x00000006,0x0000004e,0x0000004d,0x00050085,
	0x00000006,0x0000004f,0x0000004c,0x0000004e,0x00050081,0x00000006,0x00000050,0x0000004a,
	0x0000004f,0x00060050,0x0000000a,0x00000051,0x0000002e,0x0000003f,0x00000050,0x000200fe,
	0x00000051,0x00010038,0x00050036,0x0000000a,0x00000013,0x00000000,0x0000000c,0x00030037,
	0x00000009,0x00000011,0x00030037,0x0000000b,0x00000012,0x000200f8,0x00000014,0x00060041,
	0x00000019,0x00000054,0x00000011,0x00000016,0x00000018,0x0004003d,0x00000006,0x00000055,
	0x00000054,0x00050041,0x00000019,0x00000056,0x00000012,0x00000018,0x0004003d,0x00000006,
	0x00000057,0x00000056,0x00050085,0x00000006,0x00000058,0x00000055,0x00000057,0x00060041,
	0x00000019,0x00000059,0x00000011,0x00000016,0x00000022,0x0004003d,0x00000006,0x0000005a,
	0x00000059,0x00050041,0x00000019,0x0000005b,0x00000012,0x00000022,0x0004003d,0x00000006,
	0x0000005c,0x0000005b,0x00050085,0x00000006,0x0000005d,0x0000005a,0x0000005c,0x00050081,
	0x00000006,0x0000005e,0x00000058,0x0000005d,0x00060041,0x00000019,0x0000005f,0x00000011,
	0x00000016,0x0000002a,0x0004003d,0x00000006,0x00000060,0x0000005f,0x00050041,0x00000019,
	0x00000061,0x00000012,0x0000002a,0x0004003d,0x00000006,0x00000062,0x00000061,0x00050085,
	0x00000006,0x00000063,0x00000060,0x00000062,0x00050081,0x00000006,0x00000064,0x0000005e,
	0x00000063,0x00060041,0x00000019,0x00000065,0x00000011,0x0000001f,0x00000018,0x0004003d,
	0x00000006,0x00000066,0x00000065,0x00050041,0x00000019,0x00000067,0x00000012,0x00000018,
	0x0004003d,0x00000006,0x00000068,0x00000067,0x00050085,0x00000006,0x00000069,0x00000066,
	0x00000068,0x00060041,0x00000019,0x0000006a,0x00000011,0x0000001f,0x00000022,0x0004003d,
	0x00000006,0x0000006b,0x0000006a,0x00050041,0x00000019,0x0000006c,0x00000012,0x00000022,
	0x0004003d,0x00000006,0x0000006d,0x0000006c,0x00050085,0x00000006,0x0000006e,0x0000006b,
	0x0000006d,0x00050081,0x00000006,0x0000006f,0x00000069,0x0000006e,0x00060041,0x00000019,
	0x00000070,0x00000011,0x0000001f,0x0000002a,0x0004003d,0x00000006,0x00000071,0x00000070,
	0x00050041,0x00000019,0x00000072,0x00000012,0x0000002a,0x0004003d,0x00000006,0x00000073,
	0x00000072,0x00050085,0x00000006,0x00000074,0x00000071,0x00000073,0x00050081,0x00000006,
	0x00000075,0x0000006f,0x00000074,0x00060041,0x00000019,0x00000076,0x00000011,0x00000027,
	0x00000018,0x0004003d,0x00000006,0x00000077,0x00000076,0x00050041,0x00000019,0x00000078,
	0x00000012,0x00000018,0x0004003d,0x00000006,0x00000079,0x00000078,0x00050085,0x00000006,
	0x0000007a,0x00000077,0x00000079,0x00060041,0x00000019,0x0000007b,0x00000011,0x00000027,
	0x00000022,0x0004003d,0x00000006,0x0000007c,0x0000007b,0x00050041,0x00000019,0x0000007d,
	0x00000012,0x00000022,0x0004003d,0x00000006,0x0000007e,0x0000007d,0x00050085,0x00000006,
	0x0000007f,0x0000007c,0x0000007e,0x00050081,0x00000006,0x00000080,0x0000007a,0x0000007f,
	0x00060041,0x00000019,0x00000081,0x00000011,0x00000027,0x0000002a,0x0004003d,0x00000006,
	0x00000082,0x00000081,0x00050041,0x00000019,0x00000083,0x00000012,0x0000002a,0x0004003d,
	0x00000006,0x00000084,0x00000083,0x00050085,0x00000006,0x00000085,0x00000082,0x00000084,
	0x00050081,0x00000006,0x00000086,0x00000080,0x00000085,0x00060050,0x0000000a,0x00000087,
	0x00000064,0x00000075,0x00000086,0x000200fe,0x00000087,0x00010038
};

static const unsigned int unitCubeFlatShadeFragmentProgramSPIRV[] =
{
	// SPIRV99.947 15-Feb-2016
	0x07230203,0x00010000,0x00080001,0x0000004a,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0008000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000015,0x00000019,0x00000040,
	0x00030010,0x00000004,0x00000007,0x00030003,0x00000001,0x00000136,0x00070004,0x415f4c47,
	0x655f4252,0x6e61686e,0x5f646563,0x6f79616c,0x00737475,0x00070004,0x455f4c47,0x735f5458,
	0x65646168,0x6f695f72,0x6f6c625f,0x00736b63,0x00040005,0x00000004,0x6e69616d,0x00000000,
	0x00050005,0x00000009,0x66666964,0x4d657375,0x00007061,0x00050005,0x0000000d,0x63657073,
	0x72616c75,0x0070614d,0x00060005,0x00000011,0x63657073,0x72616c75,0x65776f50,0x00000072,
	0x00040005,0x00000013,0x44657965,0x00007269,0x00060005,0x00000015,0x67617266,0x746e656d,
	0x44657945,0x00007269,0x00040005,0x00000018,0x6d726f6e,0x00006c61,0x00060005,0x00000019,
	0x67617266,0x746e656d,0x6d726f4e,0x00006c61,0x00050005,0x0000001c,0x6867696c,0x72694474,
	0x00000000,0x00060005,0x00000020,0x6867696c,0x66655274,0x7463656c,0x006e6f69,0x00060005,
	0x0000002b,0x6867696c,0x66694474,0x65737566,0x00000000,0x00060005,0x00000035,0x6867696c,
	0x65705374,0x616c7563,0x00000072,0x00050005,0x00000040,0x4374756f,0x726f6c6f,0x00000000,
	0x00030047,0x00000009,0x00000000,0x00030047,0x0000000d,0x00000000,0x00030047,0x00000011,
	0x00000000,0x00030047,0x00000013,0x00000000,0x00030047,0x00000015,0x00000000,0x00040047,
	0x00000015,0x0000001e,0x00000000,0x00030047,0x00000016,0x00000000,0x00030047,0x00000017,
	0x00000000,0x00030047,0x00000018,0x00000000,0x00030047,0x00000019,0x00000000,0x00040047,
	0x00000019,0x0000001e,0x00000001,0x00030047,0x0000001a,0x00000000,0x00030047,0x0000001b,
	0x00000000,0x00030047,0x0000001c,0x00000000,0x00030047,0x00000020,0x00000000,0x00030047,
	0x00000022,0x00000000,0x00030047,0x00000023,0x00000000,0x00030047,0x00000024,0x00000000,
	0x00030047,0x00000025,0x00000000,0x00030047,0x00000026,0x00000000,0x00030047,0x00000027,
	0x00000000,0x00030047,0x00000028,0x00000000,0x00030047,0x00000029,0x00000000,0x00030047,
	0x0000002a,0x00000000,0x00030047,0x0000002b,0x00000000,0x00030047,0x0000002c,0x00000000,
	0x00030047,0x0000002d,0x00000000,0x00030047,0x0000002e,0x00000000,0x00030047,0x0000002f,
	0x00000000,0x00030047,0x00000031,0x00000000,0x00030047,0x00000032,0x00000000,0x00030047,
	0x00000033,0x00000000,0x00030047,0x00000034,0x00000000,0x00030047,0x00000035,0x00000000,
	0x00030047,0x00000036,0x00000000,0x00030047,0x00000037,0x00000000,0x00030047,0x00000038,
	0x00000000,0x00030047,0x00000039,0x00000000,0x00030047,0x0000003a,0x00000000,0x00030047,
	0x0000003b,0x00000000,0x00030047,0x0000003c,0x00000000,0x00030047,0x0000003d,0x00000000,
	0x00030047,0x00000040,0x00000000,0x00040047,0x00000040,0x0000001e,0x00000000,0x00030047,
	0x00000041,0x00000000,0x00030047,0x00000042,0x00000000,0x00030047,0x00000043,0x00000000,
	0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,
	0x00040017,0x00000007,0x00000006,0x00000003,0x00040020,0x00000008,0x00000007,0x00000007,
	0x0004002b,0x00000006,0x0000000a,0x3e4ccccd,0x0004002b,0x00000006,0x0000000b,0x3f800000,
	0x0006002c,0x00000007,0x0000000c,0x0000000a,0x0000000a,0x0000000b,0x0004002b,0x00000006,
	0x0000000e,0x3f000000,0x0006002c,0x00000007,0x0000000f,0x0000000e,0x0000000e,0x0000000e,
	0x00040020,0x00000010,0x00000007,0x00000006,0x0004002b,0x00000006,0x00000012,0x41200000,
	0x00040020,0x00000014,0x00000001,0x00000007,0x0004003b,0x00000014,0x00000015,0x00000001,
	0x0004003b,0x00000014,0x00000019,0x00000001,0x0004002b,0x00000006,0x0000001d,0xbf13cd3a,
	0x0004002b,0x00000006,0x0000001e,0x3f13cd3a,0x0006002c,0x00000007,0x0000001f,0x0000001d,
	0x0000001e,0x0000001e,0x0004002b,0x00000006,0x00000021,0x40000000,0x0004002b,0x00000006,
	0x00000030,0x00000000,0x00040017,0x0000003e,0x00000006,0x00000004,0x00040020,0x0000003f,
	0x00000003,0x0000003e,0x0004003b,0x0000003f,0x00000040,0x00000003,0x00040015,0x00000046,
	0x00000020,0x00000000,0x0004002b,0x00000046,0x00000047,0x00000003,0x00040020,0x00000048,
	0x00000003,0x00000006,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
	0x00000005,0x0004003b,0x00000008,0x00000009,0x00000007,0x0004003b,0x00000008,0x0000000d,
	0x00000007,0x0004003b,0x00000010,0x00000011,0x00000007,0x0004003b,0x00000008,0x00000013,
	0x00000007,0x0004003b,0x00000008,0x00000018,0x00000007,0x0004003b,0x00000008,0x0000001c,
	0x00000007,0x0004003b,0x00000008,0x00000020,0x00000007,0x0004003b,0x00000008,0x0000002b,
	0x00000007,0x0004003b,0x00000008,0x00000035,0x00000007,0x0003003e,0x00000009,0x0000000c,
	0x0003003e,0x0000000d,0x0000000f,0x0003003e,0x00000011,0x00000012,0x0004003d,0x00000007,
	0x00000016,0x00000015,0x0006000c,0x00000007,0x00000017,0x00000001,0x00000045,0x00000016,
	0x0003003e,0x00000013,0x00000017,0x0004003d,0x00000007,0x0000001a,0x00000019,0x0006000c,
	0x00000007,0x0000001b,0x00000001,0x00000045,0x0000001a,0x0003003e,0x00000018,0x0000001b,
	0x0003003e,0x0000001c,0x0000001f,0x0004003d,0x00000007,0x00000022,0x0000001c,0x0004003d,
	0x00000007,0x00000023,0x00000018,0x00050094,0x00000006,0x00000024,0x00000022,0x00000023,
	0x00050085,0x00000006,0x00000025,0x00000021,0x00000024,0x0004003d,0x00000007,0x00000026,
	0x00000018,0x0005008e,0x00000007,0x00000027,0x00000026,0x00000025,0x0004003d,0x00000007,
	0x00000028,0x0000001c,0x00050083,0x00000007,0x00000029,0x00000027,0x00000028,0x0006000c,
	0x00000007,0x0000002a,0x00000001,0x00000045,0x00000029,0x0003003e,0x00000020,0x0000002a,
	0x0004003d,0x00000007,0x0000002c,0x00000009,0x0004003d,0x00000007,0x0000002d,0x00000018,
	0x0004003d,0x00000007,0x0000002e,0x0000001c,0x00050094,0x00000006,0x0000002f,0x0000002d,
	0x0000002e,0x0007000c,0x00000006,0x00000031,0x00000001,0x00000028,0x0000002f,0x00000030,
	0x00050085,0x00000006,0x00000032,0x00000031,0x0000000e,0x00050081,0x00000006,0x00000033,
	0x00000032,0x0000000e,0x0005008e,0x00000007,0x00000034,0x0000002c,0x00000033,0x0003003e,
	0x0000002b,0x00000034,0x0004003d,0x00000007,0x00000036,0x0000000d,0x0004003d,0x00000007,
	0x00000037,0x00000020,0x0004003d,0x00000007,0x00000038,0x00000013,0x00050094,0x00000006,
	0x00000039,0x00000037,0x00000038,0x0007000c,0x00000006,0x0000003a,0x00000001,0x00000028,
	0x00000039,0x00000030,0x0004003d,0x00000006,0x0000003b,0x00000011,0x0007000c,0x00000006,
	0x0000003c,0x00000001,0x0000001a,0x0000003a,0x0000003b,0x0005008e,0x00000007,0x0000003d,
	0x00000036,0x0000003c,0x0003003e,0x00000035,0x0000003d,0x0004003d,0x00000007,0x00000041,
	0x0000002b,0x0004003d,0x00000007,0x00000042,0x00000035,0x00050081,0x00000007,0x00000043,
	0x00000041,0x00000042,0x0004003d,0x0000003e,0x00000044,0x00000040,0x0009004f,0x0000003e,
	0x00000045,0x00000044,0x00000043,0x00000004,0x00000005,0x00000006,0x00000003,0x0003003e,
	0x00000040,0x00000045,0x00050041,0x00000048,0x00000049,0x00000040,0x00000047,0x0003003e,
	0x00000049,0x0000000b,0x000100fd,0x00010038
};

#elif GRAPHICS_API_D3D == 1

static ksGpuProgramParm unitCubeFlatShadeProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	0,	"ModelMatrix",		0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	1,	"ViewMatrix",		0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	2,	"ProjectionMatrix",	0 }
};

static const char unitCubeFlatShadeVertexProgramHLSL[] =
	"";

static const char unitCubeFlatShadeFragmentProgramHLSL[] =
	"";

#elif GRAPHICS_API_METAL == 1

static ksGpuProgramParm unitCubeFlatShadeProgramParms[] =
{
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	0,	"ModelMatrix",		0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	1,	"ViewMatrix",		0 },
	{ KS_GPU_PROGRAM_STAGE_FLAG_VERTEX,	KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	2,	"ProjectionMatrix",	0 }
};

static const char unitCubeFlatShadeVertexProgramMetalSL[] =
	"";

static const char unitCubeFlatShadeFragmentProgramMetalSL[] =
	"";


#endif

#define GLTF_JSON_VERSION_10			"1.0"
#define GLTF_JSON_VERSION_101			"1.0.1"
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
	char *						nodeName;
	struct ksGltfNode *			node;
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
	char * out = (char *)malloc( length + 1 );
	strncpy( out, uri, length );
	out[length] = '\0';
	if ( outSizeInBytes != NULL )
	{
		*outSizeInBytes = length;
	}
	return (unsigned char *)out;
}

static unsigned char * ksGltf_ReadBase64( const char * base64, size_t * outSizeInBytes )
{
	const size_t maxSizeInBytes = ( outSizeInBytes != NULL && *outSizeInBytes > 0 ) ? *outSizeInBytes : SIZE_MAX;
	size_t base64SizeInBytes = strlen( base64 );
	size_t dataSizeInBytes = ksBase64_DecodeSizeInBytes( base64, base64SizeInBytes );
	dataSizeInBytes = MIN( dataSizeInBytes, maxSizeInBytes );
	unsigned char * buffer = (unsigned char *)malloc( dataSizeInBytes );
	ksBase64_Decode( buffer, base64, base64SizeInBytes, dataSizeInBytes );
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

static char * ksGltf_ParseUri( const ksGltfScene * scene, const ksJson * json, const char * uriName )
{
	const ksJson * jsonUri = ksJson_GetMemberByName( json, uriName );
	if ( jsonUri == NULL )
	{
		return ksGltf_strdup( "" );
	}
	const ksJson * extensions = ksJson_GetMemberByName( json, "extensions" );
	if ( extensions != NULL )
	{
		const char * bufferViewName = ksJson_GetString( ksJson_GetMemberByName( ksJson_GetMemberByName( extensions, "KHR_binary_glTF" ), "bufferView" ), "" );
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
	return ksGltf_strdup( ksJson_GetString( jsonUri, "" ) );
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

static void ksGltf_ParseIntArray( int * elements, const int count, const ksJson * arrayNode )
{
	int i = 0;
	for ( ; i < ksJson_GetMemberCount( arrayNode ) && i < count; i++ )
	{
		elements[i] = ksJson_GetInt32( ksJson_GetMemberByIndex( arrayNode, i ), 0 );
	}
	for ( ; i < count; i++ )
	{
		elements[i] = 0;
	}
}

static void ksGltf_ParseFloatArray( float * elements, const int count, const ksJson * arrayNode )
{
	int i = 0;
	for ( ; i < ksJson_GetMemberCount( arrayNode ) && i < count; i++ )
	{
		elements[i] = ksJson_GetFloat( ksJson_GetMemberByIndex( arrayNode, i ), 0.0f );
	}
	for ( ; i < count; i++ )
	{
		elements[i] = 0.0f;
	}
}

static void ksGltf_ParseUniformValue( ksGltfUniformValue * value, const ksJson * json, const ksGpuProgramParmType type, const ksGltfScene * scene )
{
	switch ( type )
	{
		case KS_GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED:					value->texture = ksGltf_GetTextureByName( scene, ksJson_GetString( json, "" ) ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT:				value->intValue[0] = ksJson_GetInt32( json, 0 ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2:		ksGltf_ParseIntArray( value->intValue, 16, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3:		ksGltf_ParseIntArray( value->intValue, 16, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4:		ksGltf_ParseIntArray( value->intValue, 16, json ); break;
		case KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT:				value->floatValue[0] = ksJson_GetFloat( json, 0.0f ); break;
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

#if GRAPHICS_API_OPENGL == 1 || GRAPHICS_API_OPENGLES == 1 || GRAPHICS_API_VULKAN == 1

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
			if ( newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_JOINT_BUFFER] != NULL )
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

	static const char tabTable[] = { "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" };

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
			if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "{" ) )
			{
				out = (unsigned char *)memcpy( out, "\n", 1 ) + 1;
				out = (unsigned char *)memcpy( out, tabTable, MIN( addTabs, 16 ) ) + MIN( addTabs, 16 );
				out = (unsigned char *)memcpy( out, "{\n", 2 ) + 2;
				addTabs++;
				addSpace = 0;
				newLine = true;
				continue;
			}
			if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "}" ) )
			{
				addTabs--;
				addSpace = 0;
				newLine = true;
				out = (unsigned char *)memcpy( out, tabTable, MIN( addTabs, 16 ) ) + MIN( addTabs, 16 );
				out = (unsigned char *)memcpy( out, "}\n", 2 ) + 2;
				continue;
			}
			if ( ksLexer_CaseSensitiveCompareToken( token, ptr, ";" ) )
			{
				out = (unsigned char *)memcpy( out, ";\n", 2 ) + 2;
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

		// Insert tabs/spaces.
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
			// Strip any existing precision specifiers.
			if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "precision" ) )
			{
				ptr = ksLexer_SkipUpToIncludingToken( source, ptr, ";" );
				addSpace = 0;
				continue;
			}

			// Convert the vertex and fragment shader in-out parameters.
			if ( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX )
			{
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "attribute" ) )
				{
					const unsigned char * typeStart;
					const unsigned char * typeEnd = ksLexer_NextToken( source, ptr, &typeStart, NULL );
					const unsigned char * nameStart;
					const unsigned char * nameEnd = ksLexer_NextToken( source, typeEnd, &nameStart, NULL );
					if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL | KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) ) != 0 )
					{
						out += sprintf( (char *)out, "layout( location = %d ) ", ksGltf_GetVertexAttributeLocation( technique, nameStart, nameEnd ) );
					}
					out = (unsigned char *)memcpy( out, "in", 2 ) + 2;
					out = (unsigned char *)memcpy( out, " ", 1 ) + 1;
					out = (unsigned char *)memcpy( out, typeStart, typeEnd - typeStart ) + ( typeEnd - typeStart );
					out = (unsigned char *)memcpy( out, " ", 1 ) + 1;
					out = (unsigned char *)memcpy( out, nameStart, nameEnd - nameStart ) + ( nameEnd - nameStart );
					ptr = nameEnd;
					continue;
				}
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "varying" ) )
				{
					const unsigned char * typeStart;
					const unsigned char * typeEnd = ksLexer_NextToken( source, ptr, &typeStart, NULL );
					const unsigned char * nameStart;
					const unsigned char * nameEnd = ksLexer_NextToken( source, typeEnd, &nameStart, NULL );
					if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL | KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) ) != 0 )
					{
						out += sprintf( (char *)out, "layout( location = %d ) ", *inOutParmCount );
						inOutParms[(*inOutParmCount)].nameStart = nameStart;
						inOutParms[(*inOutParmCount)].nameEnd = nameEnd;
						inOutParms[(*inOutParmCount)].nameLength = nameEnd - nameStart;
						(*inOutParmCount)++;
					}
					out = (unsigned char *)memcpy( out, "out", 3 ) + 3;
					out = (unsigned char *)memcpy( out, " ", 1 ) + 1;
					out = (unsigned char *)memcpy( out, typeStart, typeEnd - typeStart ) + ( typeEnd - typeStart );
					out = (unsigned char *)memcpy( out, " ", 1 ) + 1;
					out = (unsigned char *)memcpy( out, nameStart, nameEnd - nameStart ) + ( nameEnd - nameStart );
					ptr = nameEnd;
					continue;
				}
			}
			else if ( stage == KS_GPU_PROGRAM_STAGE_FLAG_FRAGMENT )
			{
				if ( ksLexer_CaseSensitiveCompareToken( token, ptr, "varying" ) )
				{
					const unsigned char * typeStart;
					const unsigned char * typeEnd = ksLexer_NextToken( source, ptr, &typeStart, NULL );
					const unsigned char * nameStart;
					const unsigned char * nameEnd = ksLexer_NextToken( source, typeEnd, &nameStart, NULL );
					if ( ( conversion & ( KS_GLSL_CONVERSION_FLAG_LAYOUT_OPENGL | KS_GLSL_CONVERSION_FLAG_LAYOUT_VULKAN ) ) != 0 )
					{
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
					out = (unsigned char *)memcpy( out, " ", 1 ) + 1;
					out = (unsigned char *)memcpy( out, typeStart, typeEnd - typeStart ) + ( typeEnd - typeStart );
					out = (unsigned char *)memcpy( out, " ", 1 ) + 1;
					out = (unsigned char *)memcpy( out, nameStart, nameEnd - nameStart ) + ( nameEnd - nameStart );
					ptr = nameEnd;
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
				out = (unsigned char *)memcpy( out, " ", 1 ) + 1;
				out = (unsigned char *)memcpy( out, typeStart, typeEnd - typeStart ) + ( typeEnd - typeStart );
				out = (unsigned char *)memcpy( out, " ", 1 ) + 1;
				out = (unsigned char *)memcpy( out, nameStart, nameEnd - nameStart ) + ( nameEnd - nameStart );
				ptr = nameEnd;
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

				// Pre-multiplied node transform with semantic transform.
				bool found = false;
				for ( int i = 0; i < technique->uniformCount; i++ )
				{
					if ( technique->uniforms[i].nodeName != NULL && technique->uniforms[i].semantic != GLTF_UNIFORM_SEMANTIC_NONE &&
							ksLexer_CaseSensitiveCompareToken( token, ptr, technique->parms[i].name ) )
					{
						assert( stage == KS_GPU_PROGRAM_STAGE_FLAG_VERTEX );
						switch ( technique->uniforms[i].semantic )
						{
							case GLTF_UNIFORM_SEMANTIC_VIEW:
								out += sprintf( (char *)out, "%s%s * %s%s",
									newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW], multiviewArrayIndexString,
									pushConstantInstanceName, technique->parms[i].name );
								found = true;
								break;
							case GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE:
								out += sprintf( (char *)out, "%s%s * %s%s",
										newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE], multiviewArrayIndexString,
										pushConstantInstanceName, technique->parms[i].name );
								found = true;
								break;
							case GLTF_UNIFORM_SEMANTIC_PROJECTION:
								out += sprintf( (char *)out, "%s%s * %s%s",
										newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION], multiviewArrayIndexString,
										pushConstantInstanceName, technique->parms[i].name );
								found = true;
								break;
							case GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE:
								out += sprintf( (char *)out, "%s%s * %s%s",
										newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE], multiviewArrayIndexString,
										pushConstantInstanceName, technique->parms[i].name );
								found = true;
								break;
							case GLTF_UNIFORM_SEMANTIC_MODEL:
								out += sprintf( (char *)out, "%s * %s%s",
										newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL],
										pushConstantInstanceName, technique->parms[i].name );
								found = true;
								break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE:
								out += sprintf( (char *)out, "%s * %s%s",
										newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE],
										pushConstantInstanceName, technique->parms[i].name );
								found = true;
								break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW:
								out += sprintf( (char *)out, "%s%s * %s%s * %s%s",
										newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW], multiviewArrayIndexString,
										pushConstantInstanceName, newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL],
										pushConstantInstanceName, technique->parms[i].name );
								ksGltf_SetUniformStageFlag( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL], NULL, stage );
								found = true;
								break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE:
								out += sprintf( (char *)out, "%s%s * %s%s * %s%s",
										newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE], multiviewArrayIndexString,
										pushConstantInstanceName, newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE],
										pushConstantInstanceName, technique->parms[i].name );
								ksGltf_SetUniformStageFlag( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE], NULL, stage );
								found = true;
								break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION:
								out += sprintf( (char *)out, "%s%s * %s%s * %s%s * %s%s",
										newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION], multiviewArrayIndexString,
										newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW], multiviewArrayIndexString,
										pushConstantInstanceName, newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL],
										pushConstantInstanceName, technique->parms[i].name );
								ksGltf_SetUniformStageFlag( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL], NULL, stage );
								found = true;
								break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION_INVERSE:
								out += sprintf( (char *)out, "%s%s * %s%s * %s%s * %s%s",
										newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE], multiviewArrayIndexString,
										newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE], multiviewArrayIndexString,
										pushConstantInstanceName, newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE],
										pushConstantInstanceName, technique->parms[i].name );
								ksGltf_SetUniformStageFlag( technique, (const unsigned char *)newSemanticUniforms[GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE], NULL, stage );
								found = true;
								break;
							default:
								out += sprintf( (char *)out, "%s%s", pushConstantInstanceName, technique->parms[i].name );
								found = true;
								break;
						}
						ksGltf_SetUniformStageFlag( technique, (const unsigned char *)technique->parms[i].name, NULL, stage );
						break;
					}
				}
				if ( found )
				{
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

#endif

void ksGltf_CreateTechniqueProgram( ksGpuContext * context, ksGltfTechnique * technique, const ksGltfProgram * program,
								const int conversion, const char * semanticUniforms[] )
{
#if GRAPHICS_API_OPENGL == 1 || GRAPHICS_API_OPENGLES == 1 || GRAPHICS_API_VULKAN == 1
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
						if ( technique->uniforms[uniformIndex].nodeName == NULL )
						{
							free( (void *)technique->parms[uniformIndex].name );
							free( technique->uniforms[uniformIndex].name );
							continue;
						}
					}
				}

				newParms[newUniformCount].stageFlags = 0;	// Set when converting the shader.
				newParms[newUniformCount].type = technique->parms[uniformIndex].type;
				newParms[newUniformCount].access = technique->parms[uniformIndex].access;
				newParms[newUniformCount].index = newUniformCount;
				newParms[newUniformCount].name = technique->parms[uniformIndex].name;
				newParms[newUniformCount].binding = 0;	// Set when adding layout qualitifiers.

				memcpy( &newUniforms[newUniformCount], &technique->uniforms[uniformIndex], sizeof( ksGltfUniform ) );
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
#else
	UNUSED_PARM( conversion );
	UNUSED_PARM( semanticUniforms );
#endif
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

#if defined( _MSC_VER )
#define stricmp _stricmp
#endif

static bool ksGltfScene_CreateFromFile( ksGpuContext * context, ksGltfScene * scene, ksSceneSettings * settings, ksGpuRenderPass * renderPass )
{
	const ksNanoseconds t0 = GetTimeNanoseconds();

	memset( scene, 0, sizeof( ksGltfScene ) );

	// Based on a GL_MAX_UNIFORM_BLOCK_SIZE of 16384 on the ARM Mali.
	const int MAX_JOINTS = 16384 / sizeof( ksMatrix4x4f );

	ksJson * rootNode = ksJson_Create();

	//
	// Load either the glTF .json or .glb
	//

	unsigned char * binaryBuffer = NULL;
	size_t binaryBufferLength = 0;

	const char * fileName = settings->glTF;
	const size_t fileNameLength = strlen( fileName );
	if ( fileNameLength > 4 && stricmp( &fileName[fileNameLength - 4], ".glb" ) == 0 )
	{
		FILE * binaryFile = fopen( fileName, "rb" );
		if ( binaryFile == NULL )
		{
			ksJson_Destroy( rootNode );
			Error( "Failed to open %s", fileName );
			return false;
		}

		ksGltfBinaryHeader header;
		if ( fread( &header, 1, sizeof( header ), binaryFile ) != sizeof( header ) )
		{
			fclose( binaryFile );
			ksJson_Destroy( rootNode );
			Error( "Failed to read glTF binary header %s", fileName );
			return false;
		}

		if ( header.magic != GLTF_BINARY_MAGIC || header.version != GLTF_BINARY_VERSION || header.contentFormat != GLTF_BINARY_CONTENT_FORMAT )
		{
			fclose( binaryFile );
			ksJson_Destroy( rootNode );
			Error( "Invalid glTF binary header %s", fileName );
			return false;
		}

		char * content = (char *) malloc( header.contentLength + 1 );
		if ( fread( content, 1, header.contentLength, binaryFile ) != header.contentLength )
		{
			free( content );
			fclose( binaryFile );
			ksJson_Destroy( rootNode );
			Error( "Failed to read binary glTF content %s", fileName );
			return false;
		}
		content[header.contentLength] = '\0';	// make sure the buffer is zero terminated

		const char * errorString = "";
		if ( !ksJson_ReadFromBuffer( rootNode, content, &errorString ) )
		{
			free( content );
			fclose( binaryFile );
			ksJson_Destroy( rootNode );
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
			ksJson_Destroy( rootNode );
			Error( "Failed to read binary glTF content %s", fileName );
			return false;
		}

		fclose( binaryFile );
	}
	else
	{
		const char * errorString = "";
		if ( !ksJson_ReadFromFile( rootNode, fileName, &errorString ) )
		{
			ksJson_Destroy( rootNode );
			Error( "Failed to load %s (%s)", fileName, errorString );
			return false;
		}
	}

	//
	// Check the glTF JSON version.
	//

	const ksJson * asset = ksJson_GetMemberByName( rootNode, "asset" );
	const char * version = ksJson_GetString( ksJson_GetMemberByName( asset, "version" ), "1.0" );
	if ( strcmp( version, GLTF_JSON_VERSION_10 ) != 0 && strcmp( version, GLTF_JSON_VERSION_101 ) != 0 )
	{
		Error( "glTF version is %s instead of %s", version, GLTF_JSON_VERSION_10 );
		ksJson_Destroy( rootNode );
		return false;
	}

	//
	// glTF buffers
	//
	{
		const ksNanoseconds startTime = GetTimeNanoseconds();

		const ksJson * buffers = ksJson_GetMemberByName( rootNode, "buffers" );
		scene->bufferCount = ksJson_GetMemberCount( buffers );
		scene->buffers = (ksGltfBuffer *) calloc( scene->bufferCount, sizeof( ksGltfBuffer ) );
		for ( int bufferIndex = 0; bufferIndex < scene->bufferCount; bufferIndex++ )
		{
			const ksJson * buffer = ksJson_GetMemberByIndex( buffers, bufferIndex );
			scene->buffers[bufferIndex].name = ksGltf_strdup( ksJson_GetMemberName( buffer ) );
			scene->buffers[bufferIndex].byteLength = (size_t) ksJson_GetUint64( ksJson_GetMemberByName( buffer, "byteLength" ), 0 );
			scene->buffers[bufferIndex].type = ksGltf_strdup( ksJson_GetString( ksJson_GetMemberByName( buffer, "type" ), "" ) );
			if ( strcmp( scene->buffers[bufferIndex].name, "binary_glTF" ) == 0 )
			{
				assert( scene->buffers[bufferIndex].byteLength == binaryBufferLength );
				scene->buffers[bufferIndex].bufferData = binaryBuffer;
			}
			else
			{
				scene->buffers[bufferIndex].bufferData = ksGltf_ReadUri( binaryBuffer, ksJson_GetString( ksJson_GetMemberByName( buffer, "uri" ), "" ), NULL );
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

		const ksJson * bufferViews = ksJson_GetMemberByName( rootNode, "bufferViews" );
		scene->bufferViewCount = ksJson_GetMemberCount( bufferViews );
		scene->bufferViews = (ksGltfBufferView *) calloc( scene->bufferViewCount, sizeof( ksGltfBufferView ) );
		for ( int bufferViewIndex = 0; bufferViewIndex < scene->bufferViewCount; bufferViewIndex++ )
		{
			const ksJson * view = ksJson_GetMemberByIndex( bufferViews, bufferViewIndex );
			scene->bufferViews[bufferViewIndex].name = ksGltf_strdup( ksJson_GetMemberName( view ) );
			scene->bufferViews[bufferViewIndex].buffer = ksGltf_GetBufferByName( scene, ksJson_GetString( ksJson_GetMemberByName( view, "buffer" ), "" ) );
			scene->bufferViews[bufferViewIndex].byteOffset = (size_t) ksJson_GetUint64( ksJson_GetMemberByName( view, "byteOffset" ), 0 );
			scene->bufferViews[bufferViewIndex].byteLength = (size_t) ksJson_GetUint64( ksJson_GetMemberByName( view, "byteLength" ), 0 );
			scene->bufferViews[bufferViewIndex].target = ksJson_GetUint16( ksJson_GetMemberByName( view, "target" ), 0 );
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

		const ksJson * accessors = ksJson_GetMemberByName( rootNode, "accessors" );
		scene->accessorCount = ksJson_GetMemberCount( accessors );
		scene->accessors = (ksGltfAccessor *) calloc( scene->accessorCount, sizeof( ksGltfAccessor ) );
		for ( int accessorIndex = 0; accessorIndex < scene->accessorCount; accessorIndex++ )
		{
			const ksJson * access = ksJson_GetMemberByIndex( accessors, accessorIndex );
			scene->accessors[accessorIndex].name = ksGltf_strdup( ksJson_GetMemberName( access ) );
			scene->accessors[accessorIndex].bufferView = ksGltf_GetBufferViewByName( scene, ksJson_GetString( ksJson_GetMemberByName( access, "bufferView" ), "" ) );
			scene->accessors[accessorIndex].byteOffset = (size_t) ksJson_GetUint64( ksJson_GetMemberByName( access, "byteOffset" ), 0 );
			scene->accessors[accessorIndex].byteStride = (size_t) ksJson_GetUint64( ksJson_GetMemberByName( access, "byteStride" ), 0 );
			scene->accessors[accessorIndex].componentType = ksJson_GetUint16( ksJson_GetMemberByName( access, "componentType" ), 0 );
			scene->accessors[accessorIndex].count = ksJson_GetInt32( ksJson_GetMemberByName( access, "count" ), 0 );
			scene->accessors[accessorIndex].type =  ksGltf_strdup( ksJson_GetString( ksJson_GetMemberByName( access, "type" ), "" ) );
			const ksJson * min = ksJson_GetMemberByName( access, "min" );
			const ksJson * max = ksJson_GetMemberByName( access, "max" );
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

		const ksJson * images = ksJson_GetMemberByName( rootNode, "images" );
		scene->imageCount = ksJson_GetMemberCount( images );
		scene->images = (ksGltfImage *) calloc( scene->imageCount, sizeof( ksGltfImage ) );
		for ( int imageIndex = 0; imageIndex < scene->imageCount; imageIndex++ )
		{
			const ksJson * image = ksJson_GetMemberByIndex( images, imageIndex );
			scene->images[imageIndex].name = ksGltf_strdup( ksJson_GetMemberName( image ) );
			char * baseUri = ksGltf_ParseUri( scene, image, "uri" );

			assert( scene->images[imageIndex].name[0] != '\0' );
			assert( baseUri != '\0' );

			const ksJson * extensions = ksJson_GetMemberByName( image, "extensions" );
			if ( extensions != NULL )
			{
				const ksJson * KHR_image_versions = ksJson_GetMemberByName( extensions, "KHR_image_versions" );
				if ( KHR_image_versions != NULL )
				{
					const ksJson * versions = ksJson_GetMemberByName( KHR_image_versions, "versions" );
					const int versionCount = ksJson_GetMemberCount( versions );
					scene->images[imageIndex].versions = (ksGltfImageVersion *) calloc( versionCount + 1, sizeof( ksGltfImageVersion ) );
					scene->images[imageIndex].versionCount = versionCount + 1;
					for ( int versionIndex = 0; versionIndex < versionCount; versionIndex++ )
					{
						const ksJson * v = ksJson_GetMemberByIndex( versions, versionIndex );
						scene->images[imageIndex].versions[versionIndex].container = ksGltf_strdup( ksJson_GetString( ksJson_GetMemberByName( v, "container" ), "" ) );
						scene->images[imageIndex].versions[versionIndex].glInternalFormat = ksJson_GetUint32( ksJson_GetMemberByName( v, "glInternalFormat" ), 0 );
						scene->images[imageIndex].versions[versionIndex].uri = ksGltf_strdup( ksJson_GetString( ksJson_GetMemberByName( v, "uri" ), "" ) );
					}
				}
			}
			if ( scene->images[imageIndex].versions == NULL )
			{
				scene->images[imageIndex].versions = (ksGltfImageVersion *) calloc( 1, sizeof( ksGltfImageVersion ) );
				scene->images[imageIndex].versionCount = 1;
			}
			const int count = scene->images[imageIndex].versionCount;
			scene->images[imageIndex].versions[count - 1].container = ksGltf_strdup( ksGltf_GetImageContainerFromUri( baseUri ) );
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

		const ksJson * samplers = ksJson_GetMemberByName( rootNode, "samplers" );
		scene->samplerCount = ksJson_GetMemberCount( samplers );
		scene->samplers = (ksGltfSampler *) calloc( scene->samplerCount, sizeof( ksGltfSampler ) );
		for ( int samplerIndex = 0; samplerIndex < scene->samplerCount; samplerIndex++ )
		{
			const ksJson * sampler = ksJson_GetMemberByIndex( samplers, samplerIndex );
			scene->samplers[samplerIndex].name = ksGltf_strdup( ksJson_GetMemberName( sampler ) );
			scene->samplers[samplerIndex].magFilter = ksJson_GetUint16( ksJson_GetMemberByName( sampler, "magFilter" ), GL_LINEAR );
			scene->samplers[samplerIndex].minFilter = ksJson_GetUint16( ksJson_GetMemberByName( sampler, "minFilter" ), GL_NEAREST_MIPMAP_LINEAR );
			scene->samplers[samplerIndex].wrapS = ksJson_GetUint16( ksJson_GetMemberByName( sampler, "wrapS" ), GL_REPEAT );
			scene->samplers[samplerIndex].wrapT = ksJson_GetUint16( ksJson_GetMemberByName( sampler, "wrapT" ), GL_REPEAT );
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

		const ksJson * textures = ksJson_GetMemberByName( rootNode, "textures" );
		scene->textureCount = ksJson_GetMemberCount( textures );
		scene->textures = (ksGltfTexture *) calloc( scene->textureCount, sizeof( ksGltfTexture ) );
		for ( int textureIndex = 0; textureIndex < scene->textureCount; textureIndex++ )
		{
			const ksJson * texture = ksJson_GetMemberByIndex( textures, textureIndex );
			scene->textures[textureIndex].name = ksGltf_strdup( ksJson_GetMemberName( texture ) );
			scene->textures[textureIndex].image = ksGltf_GetImageByName( scene, ksJson_GetString( ksJson_GetMemberByName( texture, "source" ), "" ) );
			scene->textures[textureIndex].sampler = ksGltf_GetSamplerByName( scene, ksJson_GetString( ksJson_GetMemberByName( texture, "sampler" ), "" ) );

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
			assert( uri != NULL );

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

		const ksJson * shaders = ksJson_GetMemberByName( rootNode, "shaders" );
		scene->shaderCount = ksJson_GetMemberCount( shaders );
		scene->shaders = (ksGltfShader *) calloc( scene->shaderCount, sizeof( ksGltfShader ) );
		for ( int shaderIndex = 0; shaderIndex < scene->shaderCount; shaderIndex++ )
		{
			const ksJson * shader = ksJson_GetMemberByIndex( shaders, shaderIndex );
			scene->shaders[shaderIndex].name = ksGltf_strdup( ksJson_GetMemberName( shader ) );
			scene->shaders[shaderIndex].stage = ksJson_GetUint16( ksJson_GetMemberByName( shader, "type" ), 0 );

			assert( scene->shaders[shaderIndex].name[0] != '\0' );
			assert( scene->shaders[shaderIndex].stage != 0 );

			const ksJson * extensions = ksJson_GetMemberByName( shader, "extensions" );
			if ( extensions != NULL )
			{
				for ( int shaderType = 0; shaderType < GLTF_SHADER_TYPE_MAX; shaderType++ )
				{
					const ksJson * shader_versions = ksJson_GetMemberByName( extensions, shaderVersionExtensions[shaderType] );
					if ( shader_versions != NULL )
					{
						const int count = ksJson_GetMemberCount( shader_versions );
						const int extra = ( shaderType == GLTF_SHADER_TYPE_GLSL ) * defaultGlslShaderCount;
						scene->shaders[shaderIndex].shaders[shaderType] = (ksGltfShaderVersion *) calloc( count + extra, sizeof( ksGltfShaderVersion ) );
						scene->shaders[shaderIndex].shaderCount[shaderType] = count + extra;
						for ( int index = 0; index < count; index++ )
						{
							const ksJson * glslShader = ksJson_GetMemberByIndex( shader_versions, index );
							scene->shaders[shaderIndex].shaders[shaderType][index].api = ksGltf_strdup( ksJson_GetString( ksJson_GetMemberByName( glslShader, "api" ), "" ) );
							scene->shaders[shaderIndex].shaders[shaderType][index].version = ksGltf_strdup( ksJson_GetString( ksJson_GetMemberByName( glslShader, "version" ), "" ) );
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

		const ksJson * programs = ksJson_GetMemberByName( rootNode, "programs" );
		scene->programCount = ksJson_GetMemberCount( programs );
		scene->programs = (ksGltfProgram *) calloc( scene->programCount, sizeof( ksGltfProgram ) );
		for ( int programIndex = 0; programIndex < scene->programCount; programIndex++ )
		{
			const ksJson * program = ksJson_GetMemberByIndex( programs, programIndex );
			const char * vertexShaderName = ksJson_GetString( ksJson_GetMemberByName( program, "vertexShader" ), "" );
			const char * fragmentShaderName = ksJson_GetString( ksJson_GetMemberByName( program, "fragmentShader" ), "" );
			const ksGltfShader * vertexShader = ksGltf_GetShaderByName( scene, vertexShaderName );
			const ksGltfShader * fragmentShader = ksGltf_GetShaderByName( scene, fragmentShaderName );

			assert( vertexShader != NULL );
			assert( fragmentShader != NULL );

			scene->programs[programIndex].name = ksGltf_strdup( ksJson_GetMemberName( program ) );
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

		const ksJson * techniques = ksJson_GetMemberByName( rootNode, "techniques" );
		scene->techniqueCount = ksJson_GetMemberCount( techniques );
		scene->techniques = (ksGltfTechnique *) calloc( scene->techniqueCount, sizeof( ksGltfTechnique ) );
		for ( int techniqueIndex = 0; techniqueIndex < scene->techniqueCount; techniqueIndex++ )
		{
			const ksJson * technique = ksJson_GetMemberByIndex( techniques, techniqueIndex );
			scene->techniques[techniqueIndex].name = ksGltf_strdup( ksJson_GetMemberName( technique ) );

			assert( scene->techniques[techniqueIndex].name[0] != '\0' );

			const ksJson * parameters = ksJson_GetMemberByName( technique, "parameters" );

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
			const ksJson * attributes = ksJson_GetMemberByName( technique, "attributes" );
			const int attributeCount = ksJson_GetMemberCount( attributes );
			scene->techniques[techniqueIndex].attributeCount = attributeCount;
			scene->techniques[techniqueIndex].attributes = (ksGltfVertexAttribute *) calloc( attributeCount, sizeof( ksGltfVertexAttribute ) );
			for ( int j = 0; j < attributeCount; j++ )
			{
				const ksJson * attrib = ksJson_GetMemberByIndex( attributes, j );
				const char * attribName = ksJson_GetMemberName( attrib );
				const char * parmName = ksJson_GetString( attrib, "" );
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

				const ksJson * parameter = ksJson_GetMemberByName( parameters, parmName );
				const char * semantic = ksJson_GetString( ksJson_GetMemberByName( parameter, "semantic" ), "" );
				const int type = ksJson_GetUint16( ksJson_GetMemberByName( parameter, "type" ), 0 );

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

			const ksJson * uniforms = ksJson_GetMemberByName( technique, "uniforms" );
			const int uniformCount = ksJson_GetMemberCount( uniforms );
			scene->techniques[techniqueIndex].parms = (ksGpuProgramParm *) calloc( uniformCount, sizeof( ksGpuProgramParm ) );
			scene->techniques[techniqueIndex].uniformCount = uniformCount;
			scene->techniques[techniqueIndex].uniforms = (ksGltfUniform *) calloc( uniformCount, sizeof( ksGltfUniform ) );
			memset( scene->techniques[techniqueIndex].uniforms, 0, uniformCount * sizeof( ksGltfUniform ) );
			for ( int uniformIndex = 0; uniformIndex < uniformCount; uniformIndex++ )
			{
				const ksJson * uniform = ksJson_GetMemberByIndex( uniforms, uniformIndex );
				const char * uniformName = ksJson_GetMemberName( uniform );
				const char * parmName = ksJson_GetString( uniform, "" );

				const ksJson * parameter = ksJson_GetMemberByName( parameters, parmName );
				const char * semanticName = ksJson_GetString( ksJson_GetMemberByName( parameter, "semantic" ), "" );
				const int type = ksJson_GetUint16( ksJson_GetMemberByName( parameter, "type" ), 0 );
				const int count = ksJson_GetUint32( ksJson_GetMemberByName( parameter, "count" ), 0 );
				const char * node = ksJson_GetString( ksJson_GetMemberByName( parameter, "node" ), "" );
				int stageFlags = 0;
				int binding = 0;

				const ksJson * extensions = ksJson_GetMemberByName( parameter, "extensions" );
				if ( extensions != NULL )
				{
					const ksJson * KHR_technique_uniform_stages = ksJson_GetMemberByName( extensions, "KHR_technique_uniform_stages" );
					if ( KHR_technique_uniform_stages != NULL )
					{
						const ksJson * stageArray = ksJson_GetMemberByName( KHR_technique_uniform_stages, "stages" );
						const int stageCount = ksJson_GetMemberCount( stageArray );
						for ( int stateIndex = 0; stateIndex < stageCount; stateIndex++ )
						{
							stageFlags |= ksGltf_GetProgramStageFlag( ksJson_GetUint16( ksJson_GetMemberByIndex( stageArray, stateIndex ), 0 ) );
						}
					}

#if GRAPHICS_API_OPENGL == 1 || GRAPHICS_API_OPENGL_ES == 1
					const ksJson * KHR_technique_uniform_binding_opengl = ksJson_GetMemberByName( extensions, "KHR_technique_uniform_binding_opengl" );
					if ( KHR_technique_uniform_binding_opengl != NULL )
					{
						binding = ksJson_GetUint32( ksJson_GetMemberByName( parameter, "binding" ), 0 );
					}
#elif GRAPHICS_API_VULKAN == 1
					const ksJson * KHR_technique_uniform_binding_vulkan = ksJson_GetMemberByName( extensions, "KHR_technique_uniform_binding_vulkan" );
					if ( KHR_technique_uniform_binding_vulkan != NULL )
					{
						binding = ksJson_GetUint32( ksJson_GetMemberByName( parameter, "binding" ), 0 );
					}
#elif GRAPHICS_API_D3D == 1
					const ksJson * KHR_technique_uniform_binding_d3d = ksJson_GetMemberByName( extensions, "KHR_technique_uniform_binding_d3d" );
					if ( KHR_technique_uniform_binding_d3d != NULL )
					{
						binding = ksJson_GetUint32( ksJson_GetMemberByName( parameter, "binding" ), 0 );
					}
#elif GRAPHICS_API_METAL == 1
					const ksJson * KHR_technique_uniform_binding_metal = ksJson_GetMemberByName( extensions, "KHR_technique_uniform_binding_metal" );
					if ( KHR_technique_uniform_binding_metal != NULL )
					{
						binding = ksJson_GetUint32( ksJson_GetMemberByName( parameter, "binding" ), 0 );
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

				if ( node[0] != '\0' )
				{
					assert( parmType == KS_GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4 );
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

				scene->techniques[techniqueIndex].parms[uniformIndex].stageFlags = ( stageFlags != 0 ) ? stageFlags : KS_GPU_PROGRAM_STAGE_FLAG_VERTEX;
				scene->techniques[techniqueIndex].parms[uniformIndex].type = parmType;
				scene->techniques[techniqueIndex].parms[uniformIndex].access = KS_GPU_PROGRAM_PARM_ACCESS_READ_ONLY;	// assume all parms are read-only
				scene->techniques[techniqueIndex].parms[uniformIndex].index = uniformIndex;
				scene->techniques[techniqueIndex].parms[uniformIndex].name = ksGltf_strdup( uniformName );
				scene->techniques[techniqueIndex].parms[uniformIndex].binding = binding;

				scene->techniques[techniqueIndex].uniforms[uniformIndex].name = ksGltf_strdup( parmName );
				scene->techniques[techniqueIndex].uniforms[uniformIndex].semantic = semantic;
				scene->techniques[techniqueIndex].uniforms[uniformIndex].nodeName = ( node[0] != '\0' ) ? ksGltf_strdup( node ) : NULL;
				scene->techniques[techniqueIndex].uniforms[uniformIndex].node = NULL;	// linked up later
				scene->techniques[techniqueIndex].uniforms[uniformIndex].type = parmType;
				scene->techniques[techniqueIndex].uniforms[uniformIndex].index = uniformIndex;

				const ksJson * value = ksJson_GetMemberByName( parameter, "value" );
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

			const ksJson * states = ksJson_GetMemberByName( technique, "states" );
			const ksJson * enable = ksJson_GetMemberByName( states, "enable" );
			const int enableCount = ksJson_GetMemberCount( enable );
			for ( int enableIndex = 0; enableIndex < enableCount; enableIndex++ )
			{
				const int enableState = ksJson_GetUint16( ksJson_GetMemberByIndex( enable, enableIndex ), 0 );
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

			const ksJson * functions = ksJson_GetMemberByName( states, "functions" );
			const int functionCount = ksJson_GetMemberCount( functions );
			for ( int functionIndex = 0; functionIndex < functionCount; functionIndex++ )
			{
				const ksJson * func = ksJson_GetMemberByIndex( functions, functionIndex );
				const char * funcName = ksJson_GetMemberName( func );
				if ( strcmp( funcName, "blendColor" ) == 0 )
				{
					// [float:red, float:blue, float:green, float:alpha]
					scene->techniques[techniqueIndex].rop.blendColor.x = ksJson_GetFloat( ksJson_GetMemberByIndex( func, 0 ), 0.0f );
					scene->techniques[techniqueIndex].rop.blendColor.y = ksJson_GetFloat( ksJson_GetMemberByIndex( func, 1 ), 0.0f );
					scene->techniques[techniqueIndex].rop.blendColor.z = ksJson_GetFloat( ksJson_GetMemberByIndex( func, 2 ), 0.0f );
					scene->techniques[techniqueIndex].rop.blendColor.w = ksJson_GetFloat( ksJson_GetMemberByIndex( func, 3 ), 0.0f );
				}
				else if ( strcmp( funcName, "blendEquationSeparate" ) == 0 )
				{
					// [GLenum:GL_FUNC_* (rgb), GLenum:GL_FUNC_* (alpha)]
					scene->techniques[techniqueIndex].rop.blendOpColor = ksGltf_GetBlendOp( ksJson_GetUint16( ksJson_GetMemberByIndex( func, 0 ), 0 ) );
					scene->techniques[techniqueIndex].rop.blendOpAlpha = ksGltf_GetBlendOp( ksJson_GetUint16( ksJson_GetMemberByIndex( func, 1 ), 0 ) );
				}
				else if ( strcmp( funcName, "blendFuncSeparate" ) == 0 )
				{
					// [GLenum:GL_ONE (srcRGB), GLenum:GL_ZERO (dstRGB), GLenum:GL_ONE (srcAlpha), GLenum:GL_ZERO (dstAlpha)]
					scene->techniques[techniqueIndex].rop.blendSrcColor = ksGltf_GetBlendFactor( ksJson_GetUint16( ksJson_GetMemberByIndex( func, 0 ), 0 ) );
					scene->techniques[techniqueIndex].rop.blendDstColor = ksGltf_GetBlendFactor( ksJson_GetUint16( ksJson_GetMemberByIndex( func, 1 ), 0 ) );
					scene->techniques[techniqueIndex].rop.blendSrcAlpha = ksGltf_GetBlendFactor( ksJson_GetUint16( ksJson_GetMemberByIndex( func, 2 ), 0 ) );
					scene->techniques[techniqueIndex].rop.blendDstAlpha = ksGltf_GetBlendFactor( ksJson_GetUint16( ksJson_GetMemberByIndex( func, 3 ), 0 ) );
				}
				else if ( strcmp( funcName, "colorMask" ) == 0 )
				{
					// [bool:red, bool:green, bool:blue, bool:alpha]
					scene->techniques[techniqueIndex].rop.redWriteEnable = ksJson_GetBool( ksJson_GetMemberByIndex( func, 0 ), false );
					scene->techniques[techniqueIndex].rop.blueWriteEnable = ksJson_GetBool( ksJson_GetMemberByIndex( func, 1 ), false );
					scene->techniques[techniqueIndex].rop.greenWriteEnable = ksJson_GetBool( ksJson_GetMemberByIndex( func, 2 ), false );
					scene->techniques[techniqueIndex].rop.alphaWriteEnable = ksJson_GetBool( ksJson_GetMemberByIndex( func, 3 ), false );
				}
				else if ( strcmp( funcName, "cullFace" ) == 0 )
				{
					// [GLenum:GL_BACK,GL_FRONT]
					scene->techniques[techniqueIndex].rop.cullMode = ksGltf_GetCullMode( ksJson_GetUint16( ksJson_GetMemberByIndex( func, 0 ), 0 ) );
				}
				else if ( strcmp( funcName, "depthFunc" ) == 0 )
				{
					// [GLenum:GL_LESS,GL_LEQUAL,GL_GREATER]
					scene->techniques[techniqueIndex].rop.depthCompare = ksGltf_GetCompareOp( ksJson_GetUint16( ksJson_GetMemberByIndex( func, 0 ), 0 ) );
				}
				else if ( strcmp( funcName, "depthMask" ) == 0 )
				{
					// [bool:mask]
					scene->techniques[techniqueIndex].rop.depthWriteEnable = ksJson_GetBool( ksJson_GetMemberByIndex( func, 0 ), false );
				}
				else if ( strcmp( funcName, "frontFace" ) == 0 )
				{
					// [Glenum:GL_CCW,GL_CW]
					scene->techniques[techniqueIndex].rop.frontFace = ksGltf_GetFrontFace( ksJson_GetUint16( ksJson_GetMemberByIndex( func, 0 ), 0 ) );
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

			ksGltfProgram * program = ksGltf_GetProgramByName( scene, ksJson_GetString( ksJson_GetMemberByName( technique, "program" ), "" ) );
			assert( program != NULL );

			ksGltf_CreateTechniqueProgram( context, &scene->techniques[techniqueIndex], program, conversion, semanticUniforms );

			size_t totalPushConstantBytes = 0;
			for ( int uniformIndex = 0; uniformIndex < scene->techniques[techniqueIndex].uniformCount; uniformIndex++ )
			{
				totalPushConstantBytes += ksGpuProgramParm_GetPushConstantSize( scene->techniques[techniqueIndex].parms[uniformIndex].type );
			}
			ksGpuLimits limits;
			ksGpuContext_GetLimits( context, &limits );
			assert( totalPushConstantBytes <= limits.maxPushConstantsSize );

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

		const ksJson * materials = ksJson_GetMemberByName( rootNode, "materials" );
		scene->materialCount = ksJson_GetMemberCount( materials );
		scene->materials = (ksGltfMaterial *) calloc( scene->materialCount, sizeof( ksGltfMaterial ) );
		for ( int materialIndex = 0; materialIndex < scene->materialCount; materialIndex++ )
		{
			const ksJson * material = ksJson_GetMemberByIndex( materials, materialIndex );
			scene->materials[materialIndex].name = ksGltf_strdup( ksJson_GetMemberName( material ) );
			assert( scene->materials[materialIndex].name[0] != '\0' );

			const ksGltfTechnique * technique = ksGltf_GetTechniqueByName( scene, ksJson_GetString( ksJson_GetMemberByName( material, "technique" ), "" ) );
			if ( settings->useMultiView )
			{
				const ksJson * extensions = ksJson_GetMemberByName( material, "extensions" );
				if ( extensions != NULL )
				{
					const ksJson * KHR_glsl_multi_view = ksJson_GetMemberByName( extensions, "KHR_glsl_multi_view" );
					if ( KHR_glsl_multi_view != NULL )
					{
						const ksGltfTechnique * multiViewTechnique = ksGltf_GetTechniqueByName( scene, ksJson_GetString( ksJson_GetMemberByName( KHR_glsl_multi_view, "technique" ), "" ) );
						assert( multiViewTechnique != NULL );
						technique = multiViewTechnique;
					}
				}
			}
			scene->materials[materialIndex].technique = technique;
			assert( scene->materials[materialIndex].technique != NULL );

			const ksJson * values = ksJson_GetMemberByName( material, "values" );
			scene->materials[materialIndex].valueCount = ksJson_GetMemberCount( values );
			scene->materials[materialIndex].values = (ksGltfMaterialValue *) calloc( scene->materials[materialIndex].valueCount, sizeof( ksGltfMaterialValue ) );
			for ( int valueIndex = 0; valueIndex < scene->materials[materialIndex].valueCount; valueIndex++ )
			{
				const ksJson * value = ksJson_GetMemberByIndex( values, valueIndex );
				const char * valueName = ksJson_GetMemberName( value );
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

		const ksJson * models = ksJson_GetMemberByName( rootNode, "meshes" );
		scene->modelCount = ksJson_GetMemberCount( models );
		scene->models = (ksGltfModel *) calloc( scene->modelCount, sizeof( ksGltfModel ) );
		ksGltfGeometryAccessors ** accessors = (ksGltfGeometryAccessors **) calloc( scene->modelCount, sizeof( ksGltfGeometryAccessors * ) );
		for ( int modelIndex = 0; modelIndex < scene->modelCount; modelIndex++ )
		{
			const ksJson * model = ksJson_GetMemberByIndex( models, modelIndex );
			scene->models[modelIndex].name = ksGltf_strdup( ksJson_GetMemberName( model ) );

			ksVector3f_Set( &scene->models[modelIndex].mins, FLT_MAX );
			ksVector3f_Set( &scene->models[modelIndex].maxs, -FLT_MAX );

			assert( scene->models[modelIndex].name[0] != '\0' );

			const ksJson * primitives = ksJson_GetMemberByName( model, "primitives" );
			scene->models[modelIndex].surfaceCount = ksJson_GetMemberCount( primitives );
			scene->models[modelIndex].surfaces = (ksGltfSurface *) calloc( scene->models[modelIndex].surfaceCount, sizeof( ksGltfSurface ) );
			accessors[modelIndex] = (ksGltfGeometryAccessors *) calloc( scene->models[modelIndex].surfaceCount, sizeof( ksGltfGeometryAccessors ) );
			for ( int surfaceIndex = 0; surfaceIndex < scene->models[modelIndex].surfaceCount; surfaceIndex++ )
			{
				ksGltfSurface * surface = &scene->models[modelIndex].surfaces[surfaceIndex];
				const ksJson * primitive = ksJson_GetMemberByIndex( primitives, surfaceIndex );
				const ksJson * attributes = ksJson_GetMemberByName( primitive, "attributes" );

				const char * positionAccessorName		= ksJson_GetString( ksJson_GetMemberByName( attributes, "POSITION" ), "" );
				const char * normalAccessorName			= ksJson_GetString( ksJson_GetMemberByName( attributes, "NORMAL" ), "" );
				const char * tangentAccessorName		= ksJson_GetString( ksJson_GetMemberByName( attributes, "TANGENT" ), "" );
				const char * binormalAccessorName		= ksJson_GetString( ksJson_GetMemberByName( attributes, "BINORMAL" ), "" );
				const char * colorAccessorName			= ksJson_GetString( ksJson_GetMemberByName( attributes, "COLOR" ), "" );
				const char * uv0AccessorName			= ksJson_GetString( ksJson_GetMemberByName( attributes, "TEXCOORD_0" ), "" );
				const char * uv1AccessorName			= ksJson_GetString( ksJson_GetMemberByName( attributes, "TEXCOORD_1" ), "" );
				const char * uv2AccessorName			= ksJson_GetString( ksJson_GetMemberByName( attributes, "TEXCOORD_2" ), "" );
				const char * jointIndicesAccessorName	= ksJson_GetString( ksJson_GetMemberByName( attributes, "JOINT" ), "" );
				const char * jointWeightsAccessorName	= ksJson_GetString( ksJson_GetMemberByName( attributes, "WEIGHT" ), "" );
				const char * indicesAccessorName		= ksJson_GetString( ksJson_GetMemberByName( primitive, "indices" ), "" );

				surface->material = ksGltf_GetMaterialByName( scene, ksJson_GetString( ksJson_GetMemberByName( primitive, "material" ), "" ) );
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

		const ksJson * animations = ksJson_GetMemberByName( rootNode, "animations" );
		scene->animationCount = ksJson_GetMemberCount( animations );
		scene->animations = (ksGltfAnimation *) calloc( scene->animationCount, sizeof( ksGltfAnimation ) );
		scene->timeLineCount = 0;	// May not need all because they are often shared.
		scene->timeLines = (ksGltfTimeLine *) calloc( scene->animationCount, sizeof( ksGltfTimeLine ) );
		for ( int animationIndex = 0; animationIndex < scene->animationCount; animationIndex++ )
		{
			const ksJson * animation = ksJson_GetMemberByIndex( animations, animationIndex );
			scene->animations[animationIndex].name = ksGltf_strdup( ksJson_GetMemberName( animation ) );

			const ksJson * parameters = ksJson_GetMemberByName( animation, "parameters" );
			const ksJson * samplers = ksJson_GetMemberByName( animation, "samplers" );

			// This assumes there is only a single time-line per animation.
			const char * timeAccessorName = ksJson_GetString( ksJson_GetMemberByName( parameters, "TIME" ), "" );
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

			const ksJson * channels = ksJson_GetMemberByName( animation, "channels" );
			scene->animations[animationIndex].channelCount = ksJson_GetMemberCount( channels );
			scene->animations[animationIndex].channels = (ksGltfAnimationChannel *) calloc( scene->animations[animationIndex].channelCount, sizeof( ksGltfAnimationChannel ) );
			int newChannelCount = 0;
			for ( int channelIndex = 0; channelIndex < scene->animations[animationIndex].channelCount; channelIndex++ )
			{
				const ksJson * channel = ksJson_GetMemberByIndex( channels, channelIndex );
				const char * samplerName = ksJson_GetString( ksJson_GetMemberByName( channel, "sampler" ), "" );
				const ksJson * sampler = ksJson_GetMemberByName( samplers, samplerName );
				const char * inputName = ksJson_GetString( ksJson_GetMemberByName( sampler, "input" ), "" );
				const char * interpolation = ksJson_GetString( ksJson_GetMemberByName( sampler, "interpolation" ), "" );
				const char * outputName = ksJson_GetString( ksJson_GetMemberByName( sampler, "output" ), "" );
				const char * accessorName = ksJson_GetString( ksJson_GetMemberByName( parameters, outputName ), "" );

				assert( strcmp( inputName, "TIME" ) == 0 );
				assert( strcmp( interpolation, "LINEAR" ) == 0 );
				assert( outputName[0] != '\0' );
				assert( accessorName[0] != '\0' );

				UNUSED_PARM( inputName );
				UNUSED_PARM( interpolation );

				const ksJson * target = ksJson_GetMemberByName( channel, "target" );
				const char * nodeName = ksJson_GetString( ksJson_GetMemberByName( target, "id" ), "" );
				const char * pathName = ksJson_GetString( ksJson_GetMemberByName( target, "path" ), "" );

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

		const ksJson * skins = ksJson_GetMemberByName( rootNode, "skins" );
		scene->skinCount = ksJson_GetMemberCount( skins );
		scene->skins = (ksGltfSkin *) calloc( scene->skinCount, sizeof( ksGltfSkin ) );
		for ( int skinIndex = 0; skinIndex < scene->skinCount; skinIndex++ )
		{
			const ksJson * skin = ksJson_GetMemberByIndex( skins, skinIndex );
			scene->skins[skinIndex].name = ksGltf_strdup( ksJson_GetMemberName( skin ) );
			ksMatrix4x4f bindShapeMatrix;
			ksGltf_ParseFloatArray( bindShapeMatrix.m[0], 16, ksJson_GetMemberByName( skin, "bindShapeMatrix" ) );

			const char * bindAccessorName = ksJson_GetString( ksJson_GetMemberByName( skin, "inverseBindMatrices" ), "" );
			const ksGltfAccessor * bindAccess = ksGltf_GetAccessorByNameAndType( scene, bindAccessorName, "MAT4", GL_FLOAT );
			scene->skins[skinIndex].inverseBindMatrices = ksGltf_GetBufferData( bindAccess );

			assert( scene->skins[skinIndex].name[0] != '\0' );
			assert( scene->skins[skinIndex].inverseBindMatrices != NULL );

			scene->skins[skinIndex].parentNode = NULL;	// linked up once the nodes are loaded

			const ksJson * jointNames = ksJson_GetMemberByName( skin, "jointNames" );
			scene->skins[skinIndex].jointCount = ksJson_GetMemberCount( jointNames );
			scene->skins[skinIndex].joints = (ksGltfJoint *) calloc( scene->skins[skinIndex].jointCount, sizeof( ksGltfJoint ) );
			assert( scene->skins[skinIndex].jointCount <= MAX_JOINTS );
			for ( int jointIndex = 0; jointIndex < scene->skins[skinIndex].jointCount; jointIndex++ )
			{
				ksMatrix4x4f inverseBindMatrix;
				ksMatrix4x4f_Multiply( &inverseBindMatrix, &scene->skins[skinIndex].inverseBindMatrices[jointIndex], &bindShapeMatrix );
				scene->skins[skinIndex].inverseBindMatrices[jointIndex] = inverseBindMatrix;

				scene->skins[skinIndex].joints[jointIndex].name = ksGltf_strdup( ksJson_GetString( ksJson_GetMemberByIndex( jointNames, jointIndex ), "" ) );
				scene->skins[skinIndex].joints[jointIndex].node = NULL; // linked up once the nodes are loaded
			}
			assert( bindAccess->count == scene->skins[skinIndex].jointCount );

			ksGpuBuffer_Create( context, &scene->skins[skinIndex].jointBuffer, KS_GPU_BUFFER_TYPE_UNIFORM, scene->skins[skinIndex].jointCount * sizeof( ksMatrix4x4f ), NULL, false );

			const ksJson * extensions = ksJson_GetMemberByName( skin, "extensions" );
			if ( extensions != NULL )
			{
				const ksJson * KHR_skin_culling = ksJson_GetMemberByName( extensions, "KHR_skin_culling" );
				if ( KHR_skin_culling != NULL )
				{
					const char * minsAccessorName = ksJson_GetString( ksJson_GetMemberByName( KHR_skin_culling, "jointGeometryMins" ), "" );
					const ksGltfAccessor * minsAccessor = ksGltf_GetAccessorByNameAndType( scene, minsAccessorName, "VEC3", GL_FLOAT );
					scene->skins[skinIndex].jointGeometryMins = ksGltf_GetBufferData( minsAccessor );

					const char * maxsAccessorName = ksJson_GetString( ksJson_GetMemberByName( KHR_skin_culling, "jointGeometryMaxs" ), "" );
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

		const ksJson * cameras = ksJson_GetMemberByName( rootNode, "cameras" );
		scene->cameraCount = ksJson_GetMemberCount( cameras );
		scene->cameras = (ksGltfCamera *) calloc( scene->cameraCount, sizeof( ksGltfCamera ) );
		for ( int cameraIndex = 0; cameraIndex < scene->cameraCount; cameraIndex++ )
		{
			const ksJson * camera = ksJson_GetMemberByIndex( cameras, cameraIndex );
			const char * type = ksJson_GetString( ksJson_GetMemberByName( camera, "type" ), "" );
			scene->cameras[cameraIndex].name = ksGltf_strdup( ksJson_GetMemberName( camera ) );
			if ( strcmp( type, "perspective" ) == 0 )
			{
				const ksJson * perspective = ksJson_GetMemberByName( camera, "perspective" );
				const float aspectRatio = ksJson_GetFloat( ksJson_GetMemberByName( perspective, "aspectRatio" ), 0.0f );
				const float yfov = ksJson_GetFloat( ksJson_GetMemberByName( perspective, "yfov" ), 0.0f );
				scene->cameras[cameraIndex].type = GLTF_CAMERA_TYPE_PERSPECTIVE;
				scene->cameras[cameraIndex].perspective.fovDegreesX = ( 180.0f / MATH_PI ) * 2.0f * atanf( tanf( yfov * 0.5f ) * aspectRatio );
				scene->cameras[cameraIndex].perspective.fovDegreesY = ( 180.0f / MATH_PI ) * yfov;
				scene->cameras[cameraIndex].perspective.nearZ = ksJson_GetFloat( ksJson_GetMemberByName( perspective, "znear" ), 0.0f );
				scene->cameras[cameraIndex].perspective.farZ = ksJson_GetFloat( ksJson_GetMemberByName( perspective, "zfar" ), 0.0f );
				assert( scene->cameras[cameraIndex].perspective.fovDegreesX > 0.0f );
				assert( scene->cameras[cameraIndex].perspective.fovDegreesY > 0.0f );
				assert( scene->cameras[cameraIndex].perspective.nearZ > 0.0f );
			}
			else
			{
				const ksJson * orthographic = ksJson_GetMemberByName( camera, "orthographic" );
				scene->cameras[cameraIndex].type = GLTF_CAMERA_TYPE_ORTHOGRAPHIC;
				scene->cameras[cameraIndex].orthographic.magX = ksJson_GetFloat( ksJson_GetMemberByName( orthographic, "xmag" ), 0.0f );
				scene->cameras[cameraIndex].orthographic.magY = ksJson_GetFloat( ksJson_GetMemberByName( orthographic, "ymag" ), 0.0f );
				scene->cameras[cameraIndex].orthographic.nearZ = ksJson_GetFloat( ksJson_GetMemberByName( orthographic, "znear" ), 0.0f );
				scene->cameras[cameraIndex].orthographic.farZ = ksJson_GetFloat( ksJson_GetMemberByName( orthographic, "zfar" ), 0.0f );
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

		const ksJson * nodes = ksJson_GetMemberByName( rootNode, "nodes" );
		scene->nodeCount = ksJson_GetMemberCount( nodes );
		scene->nodes = (ksGltfNode *) calloc( scene->nodeCount, sizeof( ksGltfNode ) );
		for ( int nodeIndex = 0; nodeIndex < scene->nodeCount; nodeIndex++ )
		{
			const ksJson * node = ksJson_GetMemberByIndex( nodes, nodeIndex );
			scene->nodes[nodeIndex].name = ksGltf_strdup( ksJson_GetMemberName( node ) );
			scene->nodes[nodeIndex].jointName = ksGltf_strdup( ksJson_GetString( ksJson_GetMemberByName( node, "jointName" ), "" ) );
			const ksJson * matrix = ksJson_GetMemberByName( node, "matrix" );
			if ( ksJson_IsArray( matrix ) )
			{
				ksMatrix4x4f localTransform;
				ksGltf_ParseFloatArray( localTransform.m[0], 16, matrix );
				ksMatrix4x4f_GetTranslation( &scene->nodes[nodeIndex].translation, &localTransform );
				ksMatrix4x4f_GetRotation( &scene->nodes[nodeIndex].rotation, &localTransform );
				ksMatrix4x4f_GetScale( &scene->nodes[nodeIndex].scale, &localTransform );
			}
			else
			{
				ksGltf_ParseFloatArray( &scene->nodes[nodeIndex].rotation.x, 4, ksJson_GetMemberByName( node, "rotation" ) );
				ksGltf_ParseFloatArray( &scene->nodes[nodeIndex].scale.x, 3, ksJson_GetMemberByName( node, "scale" ) );
				ksGltf_ParseFloatArray( &scene->nodes[nodeIndex].translation.x, 3, ksJson_GetMemberByName( node, "translation" ) );
			}

			assert( scene->nodes[nodeIndex].name[0] != '\0' );

			const ksJson * children = ksJson_GetMemberByName( node, "children" );
			scene->nodes[nodeIndex].childCount = ksJson_GetMemberCount( children );
			scene->nodes[nodeIndex].childNames = (char **) calloc( scene->nodes[nodeIndex].childCount, sizeof( char * ) );
			for ( int c = 0; c < scene->nodes[nodeIndex].childCount; c++ )
			{
				scene->nodes[nodeIndex].childNames[c] = ksGltf_strdup( ksJson_GetString( ksJson_GetMemberByIndex( children, c ), "" ) );
			}
			scene->nodes[nodeIndex].camera = ksGltf_GetCameraByName( scene, ksJson_GetString( ksJson_GetMemberByName( node, "camera" ), "" ) );
			scene->nodes[nodeIndex].skin = ksGltf_GetSkinByName( scene, ksJson_GetString( ksJson_GetMemberByName( node, "skin" ), "" ) );
			const ksJson * meshes = ksJson_GetMemberByName( node, "meshes" );
			scene->nodes[nodeIndex].modelCount = ksJson_GetMemberCount( meshes );
			scene->nodes[nodeIndex].models = (ksGltfModel **) calloc( scene->nodes[nodeIndex].modelCount, sizeof( ksGltfModel ** ) );
			for ( int m = 0; m < scene->nodes[nodeIndex].modelCount; m++ )
			{
				scene->nodes[nodeIndex].models[m] = ksGltf_GetModelByName( scene, ksJson_GetString( ksJson_GetMemberByIndex( meshes, m ), "" ) );
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
		// Get the uniform nodes of techniques.
		for ( int techniqueIndex = 0; techniqueIndex < scene->techniqueCount; techniqueIndex++ )
		{
			for ( int uniformIndex = 0; uniformIndex < scene->techniques[techniqueIndex].uniformCount; uniformIndex++ )
			{
				ksGltfUniform * uniform = &scene->techniques[techniqueIndex].uniforms[uniformIndex];
				if ( uniform->nodeName != NULL )
				{
					uniform->node = ksGltf_GetNodeByName( scene, uniform->nodeName );
					assert( uniform->node != NULL );
				}
			}
		}
		// Get the animated nodes.
		for ( int animationIndex = 0; animationIndex < scene->animationCount; animationIndex++ )
		{
			for ( int channelIndex = 0; channelIndex < scene->animations[animationIndex].channelCount; channelIndex++ )
			{
				ksGltfAnimationChannel * channel = &scene->animations[animationIndex].channels[channelIndex];
				channel->node = ksGltf_GetNodeByName( scene, channel->nodeName );
				assert( channel->node != NULL );
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
		const ksJson * subScenes = ksJson_GetMemberByName( rootNode, "scenes" );
		scene->subTreeCount = 0;
		scene->subTrees = (ksGltfSubTree *) calloc( scene->nodeCount, sizeof( ksGltfSubTree ) );
		scene->subSceneCount = ksJson_GetMemberCount( subScenes );
		scene->subScenes = (ksGltfSubScene *) calloc( scene->subSceneCount, sizeof( ksGltfSubScene ) );
		for ( int subSceneIndex = 0; subSceneIndex < scene->subSceneCount; subSceneIndex++ )
		{
			const ksJson * subScene = ksJson_GetMemberByIndex( subScenes, subSceneIndex );
			scene->subScenes[subSceneIndex].name = ksGltf_strdup( ksJson_GetMemberName( subScene ) );

			const ksJson * nodes = ksJson_GetMemberByName( subScene, "nodes" );
			scene->subScenes[subSceneIndex].subTreeCount = ksJson_GetMemberCount( nodes );
			scene->subScenes[subSceneIndex].subTrees = (ksGltfSubTree **) calloc( scene->subScenes[subSceneIndex].subTreeCount, sizeof( ksGltfSubTree * ) );

			for ( int subTreeIndex = 0; subTreeIndex < scene->subScenes[subSceneIndex].subTreeCount; subTreeIndex++ )
			{
				const char * nodeName = ksJson_GetString( ksJson_GetMemberByIndex( nodes, subTreeIndex ), "" );

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

	const char * defaultSceneName = ksJson_GetString( ksJson_GetMemberByName( rootNode, "scene" ), "" );
	scene->state.currentSubScene = ksGltf_GetSubSceneByName( scene, defaultSceneName );
	assert( scene->state.currentSubScene != NULL );

	ksJson_Destroy( rootNode );

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
				free( scene->techniques[techniqueIndex].uniforms[uniformIndex].nodeName );
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
		for ( int nodeIndex = 0; nodeIndex < subTree->nodeCount; nodeIndex++ )
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
						if ( uniform->node != NULL )
						{
							const ksMatrix4x4f * matrix = &scene->state.nodeState[(int)( uniform->node - scene->nodes )].globalTransform;
							ksGpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, matrix );
						}
						else
						{
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

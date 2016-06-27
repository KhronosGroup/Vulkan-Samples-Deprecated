include_directories(
        "${CMAKE_SOURCE_DIR}/../Vulkan-LoaderAndValidationLayers/include"
        "${CMAKE_SOURCE_DIR}/../Vulkan-LoaderAndValidationLayers/build/layers"
        "${CMAKE_SOURCE_DIR}/../glslang"
        "$ENV{VK_SDK_PATH}/Include"
        "$ENV{VK_SDK_PATH}/Source/layers"
        )

add_library(VkLayer_glsl_shader SHARED glsl_shader.cpp VkLayer_glsl_shader.json)
set_target_properties( VkLayer_glsl_shader PROPERTIES CXX_FLAGS "" )
set_target_properties( VkLayer_glsl_shader PROPERTIES FOLDER layers )

if ( WIN32 )
    macro( link_glslang_lib lib )
        add_library( "${lib}_target" STATIC IMPORTED )
        set_target_properties( "${lib}_target" PROPERTIES
			IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/external/libs/glslang/Release/${lib}.lib"
			IMPORTED_LOCATION_DEBUG "${CMAKE_BINARY_DIR}/external/libs/glslang/Debug/${lib}.lib" )
        target_link_libraries( VkLayer_glsl_shader "${lib}_target" )
    endmacro()

    target_compile_options( VkLayer_glsl_shader PRIVATE /arch:AVX /Zc:wchar_t /Zc:forScope /Wall /WX )
    set_target_properties( VkLayer_glsl_shader PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/projects/windows/VkLayer_glsl_shader.def" )
    link_glslang_lib( glslang )
	link_glslang_lib( OSDependent )
    link_glslang_lib( OGLCompiler )
    link_glslang_lib( HLSL )
    link_glslang_lib( SPIRV )
    add_custom_command( OUTPUT VkLayer_glsl_shader.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/projects/windows/VkLayer_glsl_shader.json $<CONFIGURATION>/VkLayer_glsl_shader.json
        VERBATIM )
elseif( APPLE )
    macro( link_glslang_lib lib )
        find_library( ${lib}_LIB NAMES ${lib} HINTS "${CMAKE_BINARY_DIR}/external/libs/glslang/" )
        find_library( ${lib}_DLIB NAMES ${lib} HINTS "${CMAKE_BINARY_DIR}/external/libs/glslang/" )
        target_link_libraries( VkLayer_glsl_shader ${lib} )
    endmacro()

	target_compile_options( VkLayer_glsl_shader PRIVATE -std=c++11 -Wall -Wno-unused-function )
    set_target_properties( VkLayer_glsl_shader PROPERTIES LINK_FLAGS "-Bsymbolic" )
    link_glslang_lib( glslang )
	link_glslang_lib( OSDependent )
    link_glslang_lib( OGLCompiler )
    link_glslang_lib( HLSL )
    link_glslang_lib( SPIRV )
    add_custom_command( OUTPUT VkLayer_glsl_shader.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/projects/linux/VkLayer_glsl_shader.json VkLayer_glsl_shader.json
        VERBATIM )
else()
    macro( link_glslang_lib lib )
        find_library( ${lib}_LIB NAMES ${lib} HINTS "${CMAKE_BINARY_DIR}/external/libs/glslang/" )
        find_library( ${lib}_DLIB NAMES ${lib} HINTS "${CMAKE_BINARY_DIR}/external/libs/glslang/" )
        target_link_libraries( VkLayer_glsl_shader ${lib} )
    endmacro()

	target_compile_options( VkLayer_glsl_shader PRIVATE -std=c++11 -Wall -Wno-unused-function )
    set_target_properties( VkLayer_glsl_shader PROPERTIES LINK_FLAGS "-Bsymbolic" )
    link_glslang_lib( glslang )
	link_glslang_lib( OSDependent )
    link_glslang_lib( OGLCompiler )
    link_glslang_lib( HLSL )
    link_glslang_lib( SPIRV )
    add_custom_command( OUTPUT VkLayer_glsl_shader.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/projects/linux/VkLayer_glsl_shader.json VkLayer_glsl_shader.json
        VERBATIM )
endif()

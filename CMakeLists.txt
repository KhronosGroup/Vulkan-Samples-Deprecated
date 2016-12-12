cmake_minimum_required( VERSION 2.8 FATAL_ERROR )
cmake_policy( VERSION 2.8 )

project( VULKAN_SAMPLES )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )
set_property( GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "" )
set( SUPPORT_X ON CACHE BOOL "Compile with support for Xlib and XCB" )
set( SUPPORT_DISPLAY_SURFACE OFF CACHE BOOL "Compile with support for VkDisplaySurface" )


# The MAJOR number of the Vulkan version.
# Used in naming vulkan-<major>.dll.
set( MAJOR "1" )

include_directories(
        "external/include"
        "../Vulkan-LoaderAndValidationLayers/include"
        "../Vulkan-LoaderAndValidationLayers/build/generated/include"
        "$ENV{VK_SDK_PATH}/Include" )

if( WIN32 )
    STRING( REGEX REPLACE "/" "\\\\" BASE_DIR "${CMAKE_SOURCE_DIR}" )
    add_custom_target( vk_dispatch_table_helper
        COMMAND IF NOT exist ${BASE_DIR}\\..\\Vulkan-LoaderAndValidationLayers\\build\\generated\\include (mkdir ${BASE_DIR}\\..\\Vulkan-LoaderAndValidationLayers\\build\\generated\\include)
        COMMAND python ${BASE_DIR}\\..\\Vulkan-LoaderAndValidationLayers\\scripts\\lvl_genvk.py -registry ${BASE_DIR}\\..\\Vulkan-LoaderAndValidationLayers\\scripts\\vk.xml vk_dispatch_table_helper.h -o ${BASE_DIR}\\..\\Vulkan-LoaderAndValidationLayers\\build\\generated\\include
        )
elseif( APPLE )
    set( BASE_DIR "${CMAKE_SOURCE_DIR}" )
    add_custom_target( vk_dispatch_table_helper
        COMMAND mkdir -p ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/build/generated/include
        COMMAND python ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/scripts/lvl_genvk.py -registry ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/scripts/vk.xml vk_dispatch_table_helper.h -o ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/build/generated/include
        )
else()
    set( BASE_DIR "${CMAKE_SOURCE_DIR}" )
    add_custom_target( vk_dispatch_table_helper
        COMMAND mkdir -p ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/build/generated/include
        COMMAND python ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/scripts/lvl_genvk.py -registry ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/scripts/vk.xml vk_dispatch_table_helper.h -o ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/build/generated/include
        )

	#Sanity check as we don't support compiling with more than one presentation/input path.
	if( SUPPORT_X AND SUPPORT_DISPLAY_SURFACE)
		message( FATAL_ERROR "SUPPORT_X and SUPPORT_DISPLAY_SURFACE cannot be enabled at the same time" )
	endif()

	if( SUPPORT_X )
		add_definitions( -DSUPPORT_X )
		set( XLIB_LIBRARIES X11 Xxf86vm Xrandr )
		set( XCB_LIBRARIES xcb xcb-keysyms xcb-randr )
	endif()
endif()

#This is outside the platform-specific blocks because in theory it is platform-neutral.
if( SUPPORT_DISPLAY_SURFACE )
	add_definitions( -DOS_NEUTRAL_DISPLAY_SURFACE )
endif()

add_subdirectory( external/include )
add_subdirectory( external/libs/glslang )
add_subdirectory( samples )

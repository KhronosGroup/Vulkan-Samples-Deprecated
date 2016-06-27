cmake_minimum_required( VERSION 2.8 FATAL_ERROR )
cmake_policy( VERSION 2.8 )

project( VULKAN_SAMPLES )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )
set_property( GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "" )

# The MAJOR number of the Vulkan version.
# Used in naming vulkan-<major>.dll.
set( MAJOR "1" )

include_directories(
		"external/include"
		"../Vulkan-LoaderAndValidationLayers/include"
		"$ENV{VK_SDK_PATH}/Include" )

if( NOT WIN32 )
    set( XLIB_LIBRARIES X11 Xxf86vm Xrandr )
    set( XCB_LIBRARIES xcb xcb-keysyms xcb-randr )
endif()

add_subdirectory( external/include/GL )
add_subdirectory( external/libs/glslang )
add_subdirectory( samples )

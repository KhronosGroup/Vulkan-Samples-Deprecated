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

if( WIN32 )
    STRING( REGEX REPLACE "/" "\\\\" BASE_DIR "${CMAKE_SOURCE_DIR}" )
    add_custom_target( vk_dispatch_table_helper
        COMMAND IF NOT exist ${BASE_DIR}\\..\\Vulkan-LoaderAndValidationLayers\\build\\layers (mkdir ${BASE_DIR}\\..\\Vulkan-LoaderAndValidationLayers\\build\\layers)
        COMMAND python ${BASE_DIR}\\..\\Vulkan-LoaderAndValidationLayers\\scripts\\vk-generate.py AllPlatforms dispatch-table-ops layer > ${BASE_DIR}\\..\\Vulkan-LoaderAndValidationLayers\\build\\layers\\vk_dispatch_table_helper.h
        )
elseif( APPLE )
    set( BASE_DIR "${CMAKE_SOURCE_DIR}" )
    add_custom_target( vk_dispatch_table_helper
        COMMAND mkdir -p ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/build/layers
        COMMAND python ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/scripts/vk-generate.py AllPlatforms dispatch-table-ops layer > ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/build/layers/vk_dispatch_table_helper.h
        )
else()
    set( BASE_DIR "${CMAKE_SOURCE_DIR}" )
    add_custom_target( vk_dispatch_table_helper
        COMMAND mkdir -p ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/build/layers
        COMMAND python ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/scripts/vk-generate.py AllPlatforms dispatch-table-ops layer > ${BASE_DIR}/../Vulkan-LoaderAndValidationLayers/build/layers/vk_dispatch_table_helper.h
        )

    set( XLIB_LIBRARIES X11 Xxf86vm Xrandr )
    set( XCB_LIBRARIES xcb xcb-keysyms xcb-randr )
endif()

add_subdirectory( external/libs/glslang )
add_subdirectory( samples )

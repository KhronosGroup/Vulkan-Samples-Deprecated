include_directories(
        "${CMAKE_SOURCE_DIR}/../Vulkan-LoaderAndValidationLayers/include"
        "${CMAKE_SOURCE_DIR}/../Vulkan-LoaderAndValidationLayers/build/layers"
        "$ENV{VK_SDK_PATH}/Include"
        "$ENV{VK_SDK_PATH}/Source/layers"
        )

add_library( VkLayer_queue_muxer SHARED queue_muxer.c VkLayer_queue_muxer.json )
add_dependencies( VkLayer_queue_muxer vk_dispatch_table_helper )
set_target_properties( VkLayer_queue_muxer PROPERTIES CXX_FLAGS "" )
set_target_properties( VkLayer_queue_muxer PROPERTIES FOLDER layers )

if ( WIN32 )
	target_compile_options( VkLayer_queue_muxer PRIVATE /arch:AVX /Zc:wchar_t /Zc:forScope /Wall /WX )
    set_target_properties( VkLayer_queue_muxer PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/projects/windows/VkLayer_queue_muxer.def" )
    add_custom_command( OUTPUT VkLayer_queue_muxer.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/projects/windows/VkLayer_queue_muxer.json $<CONFIGURATION>/VkLayer_queue_muxer.json
        VERBATIM )
elseif( APPLE )
	target_compile_options( VkLayer_queue_muxer PRIVATE -std=c99 -Wall -Wno-unused-function )
    set_target_properties( VkLayer_queue_muxer PROPERTIES LINK_FLAGS "-Bsymbolic" )
    add_custom_command( OUTPUT VkLayer_queue_muxer.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/projects/linux/VkLayer_queue_muxer.json VkLayer_queue_muxer.json
        VERBATIM )
else()
	target_compile_options( VkLayer_queue_muxer PRIVATE -std=c99 -Wall -Wno-unused-function )
    set_target_properties( VkLayer_queue_muxer PROPERTIES LINK_FLAGS "-Bsymbolic" )
    add_custom_command( OUTPUT VkLayer_queue_muxer.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/projects/linux/VkLayer_queue_muxer.json VkLayer_queue_muxer.json
        VERBATIM )
endif()

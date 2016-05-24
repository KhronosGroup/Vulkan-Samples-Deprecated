cmake_minimum_required(VERSION 2.8.11)

message(STATUS "Cxxflags: ${CMAKE_CXX_FLAGS}")

include_directories(
        "${CMAKE_SOURCE_DIR}/../Vulkan-LoaderAndValidationLayers/layers"
        "${CMAKE_SOURCE_DIR}/../Vulkan-LoaderAndValidationLayers/loader"
        "$ENV{VK_SDK_PATH}/Source/layers"
        "$ENV{VK_SDK_PATH}/Source/loader"
        )

add_library(VkLayer_queue_muxer SHARED queue_muxer.cpp VkLayer_queue_muxer.json)
set_target_properties(VkLayer_queue_muxer PROPERTIES CXX_FLAGS "")

if (WIN32)
    set_target_properties(VkLayer_queue_muxer PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/projects/windows/VkLayer_queue_muxer.def")
    add_custom_command(OUTPUT VkLayer_queue_muxer.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/projects/windows/VkLayer_queue_muxer.json $<CONFIGURATION>/VkLayer_queue_muxer.json
        VERBATIM)
else()
    set_target_properties(VkLayer_queue_muxer PROPERTIES LINK_FLAGS "-Wl,-Bsymbolic,-Wno-unused-function")
    add_custom_command(OUTPUT VkLayer_queue_muxer.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/projects/linux/VkLayer_queue_muxer.json VkLayer_queue_muxer.json
        VERBATIM)
endif()

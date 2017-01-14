
set( INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/include" )

set( SOURCES
    ${INCLUDE_DIR}/include.c )
set( GL_HEADERS
    ${INCLUDE_DIR}/GL/gl_format.h
    ${INCLUDE_DIR}/GL/glext.h
    ${INCLUDE_DIR}/GL/wglext.h )
set( UTILS_HEADERS
    ${INCLUDE_DIR}/utils/base64.h
    ${INCLUDE_DIR}/utils/json.h
    ${INCLUDE_DIR}/utils/lexer.h )
set( VULKAN_HEADERS
    ${INCLUDE_DIR}/vulkan/vk_format.h )
source_group( Dummy FILES ${SOURCES} )
source_group( GL FILES ${GL_HEADERS} )
source_group( utils FILES ${UTILS_HEADERS} )
source_group( vulkan FILES ${VULKAN_HEADERS} )
add_library( include STATIC ${SOURCES} ${GL_HEADERS} ${UTILS_HEADERS} ${VULKAN_HEADERS} )
set_property( TARGET include PROPERTY FOLDER external/include )


#
# atw_cpu_dsp
#
if( WIN32 )
    add_executable( atw_cpu_dsp atw_cpu_dsp.c )
    target_compile_options( atw_cpu_dsp PRIVATE /arch:AVX /Zc:wchar_t /Zc:forScope /Wall /WX )
	set_target_properties( atw_cpu_dsp PROPERTIES FOLDER apps )
elseif( APPLE )
    find_library( COCOA_LIBRARY Cocoa )
    mark_as_advanced( COCOA_LIBRARY )
    add_executable( atw_cpu_dsp atw_cpu_dsp.c )
    target_compile_options( atw_cpu_dsp PRIVATE -std=c99 -x objective-c -fno-objc-arc -march=core-avx2 -Wall -Wno-unused-function -Wno-unused-const-variable )
	set_target_properties( atw_cpu_dsp PROPERTIES FOLDER apps )
    target_link_libraries( atw_cpu_dsp m pthread ${COCOA_LIBRARY} )
else()
    add_executable( atw_cpu_dsp atw_cpu_dsp.c )
    target_compile_options( atw_cpu_dsp PRIVATE -std=c99 -march=core-avx2 -Wall -Wno-unused-function -Wno-unused-const-variable )
	set_target_properties( atw_cpu_dsp PROPERTIES FOLDER apps )
    target_link_libraries( atw_cpu_dsp m pthread )
endif()

#
# atw_opengl
#
if( WIN32 )
    add_executable( atw_opengl WIN32 atw_opengl.c scenes/scene_settings.h scenes/scene_view_state.h scenes/scene_perf.h scenes/scene_gltf.h )
    target_compile_options( atw_opengl PRIVATE /Zc:wchar_t /Zc:forScope /Wall /WX )
	set_target_properties( atw_opengl PROPERTIES FOLDER apps )
    target_link_libraries( atw_opengl opengl32 )
elseif( APPLE )
    find_library( COCOA_LIBRARY Cocoa )
    find_library( OPENGL_LIBRARY OpenGL )
    mark_as_advanced( COCOA_LIBRARY OPENGL_LIBRARY )
    add_executable( atw_opengl atw_opengl.c scenes/scene_settings.h scenes/scene_view_state.h scenes/scene_perf.h scenes/scene_gltf.h )
    target_compile_options( atw_opengl PRIVATE -std=c99 -x objective-c -fno-objc-arc -Wall -Wno-unused-function -Wno-unused-const-variable )
	set_target_properties( atw_opengl PROPERTIES FOLDER apps )
    target_link_libraries( atw_opengl ${COCOA_LIBRARY} ${OPENGL_LIBRARY} )
else()
    add_executable( atw_opengl atw_opengl.c scenes/scene_settings.h scenes/scene_view_state.h scenes/scene_perf.h scenes/scene_gltf.h )
    target_compile_options( atw_opengl PRIVATE -std=c99 -Wall -Wno-unused-function -Wno-unused-const-variable )
	set_target_properties( atw_opengl PROPERTIES FOLDER apps )
    target_link_libraries( atw_opengl m pthread GL ${XLIB_LIBRARIES} ${XCB_LIBRARIES} )
endif()

#
# atw_vulkan
#
if( WIN32 )
    add_executable( atw_vulkan WIN32 atw_vulkan.c scenes/scene_settings.h scenes/scene_view_state.h scenes/scene_perf.h scenes/scene_gltf.h )
    target_compile_options( atw_vulkan PRIVATE /Zc:wchar_t /Zc:forScope /Wall /WX )
	set_target_properties( atw_vulkan PROPERTIES FOLDER apps )
    target_link_libraries( atw_vulkan )
elseif( APPLE )
    find_library( COCOA_LIBRARY Cocoa )
    mark_as_advanced( COCOA_LIBRARY )
    add_executable( atw_vulkan atw_vulkan.c scenes/scene_settings.h scenes/scene_view_state.h scenes/scene_perf.h scenes/scene_gltf.h )
    target_compile_options( atw_vulkan PRIVATE -std=c99 -x objective-c -fno-objc-arc -Wall -Wno-unused-function -Wno-unused-const-variable )
	set_target_properties( atw_vulkan PROPERTIES FOLDER apps )
    target_link_libraries( atw_vulkan ${COCOA_LIBRARY} )
else()
    add_executable( atw_vulkan atw_vulkan.c scenes/scene_settings.h scenes/scene_view_state.h scenes/scene_perf.h scenes/scene_gltf.h )
    target_compile_options( atw_vulkan PRIVATE -std=c99 -Wall -Wno-unused-function -Wno-unused-const-variable )
	set_target_properties( atw_vulkan PROPERTIES FOLDER apps )
    target_link_libraries( atw_vulkan m pthread dl )

	#It's possible to build a non-X Linux version of the vulkan sample, so only conditionally link against xcb and xlib
	if( SUPPORT_X )
		target_link_libraries( atw_vulkan ${XLIB_LIBRARIES} )
		target_link_libraries( atw_vulkan ${XCB_LIBRARIES} )
	endif()

endif()

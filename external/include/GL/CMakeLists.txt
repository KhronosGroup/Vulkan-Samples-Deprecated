add_library( GL STATIC glext.h wglext.h )
set_target_properties( GL PROPERTIES LINKER_LANGUAGE CXX )
set_target_properties( GL PROPERTIES FOLDER external/include )

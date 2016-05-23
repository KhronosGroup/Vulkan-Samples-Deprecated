LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE			:= atw_opengl
LOCAL_SRC_FILES			:= ../../../../../atw_opengl.c
LOCAL_CFLAGS			:= -std=c99 -O3 -mfpu=neon -Wall
LOCAL_LDLIBS			:= -llog -landroid -lEGL -lGLESv3
LOCAL_STATIC_LIBRARIES	:= android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)

LOCAL_PATH := $(call my-dir)

# base of the Vulkan-Samples repository
BASE_DIR := ../../../../../../../..

include $(CLEAR_VARS)

LOCAL_MODULE			:= atw_vulkan
LOCAL_C_INCLUDES		:= $(BASE_DIR)/Vulkan-Samples/external/include \
                           $(BASE_DIR)/Vulkan-LoaderAndValidationLayers/include \
                           $(VK_SDK_PATH)/Include
LOCAL_SRC_FILES			:= ../../../../../atw_vulkan.c
LOCAL_CFLAGS			:= -std=c99 -O3 -Wall
LOCAL_LDLIBS			:= -llog -landroid -ldl
LOCAL_STATIC_LIBRARIES	:= android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)

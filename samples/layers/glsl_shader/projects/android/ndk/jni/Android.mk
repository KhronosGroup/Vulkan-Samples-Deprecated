LOCAL_PATH := $(abspath $(call my-dir))

include ../../../../../../external/libs/glslang/ndk/jni/Android.mk

# base of the Vulkan-Samples repository
BASE_DIR := ../../../../../../..

# VkLayer_glsl_shader
include $(CLEAR_VARS)
LOCAL_MODULE			:= VkLayer_glsl_shader
LOCAL_C_INCLUDES		:= \
							$(BASE_DIR)/Vulkan-Samples/external/include \
							$(BASE_DIR)/Vulkan-LoaderAndValidationLayers/include \
							$(BASE_DIR)/Vulkan-LoaderAndValidationLayers/build/layers \
							$(BASE_DIR)/glslang \
							$(VK_SDK_PATH)/Include \
							$(VK_SDK_PATH)/Source/layers
LOCAL_SRC_FILES			:= $(BASE_DIR)/samples/layers/glsl_shader/glsl_shader.cpp
LOCAL_CXXFLAGS			:= -std=c++11 -fno-exceptions -fno-rtti -Wall -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_LDLIBS			:= -llog
LOCAL_STATIC_LIBRARIES	:= glslang
include $(BUILD_SHARED_LIBRARY)

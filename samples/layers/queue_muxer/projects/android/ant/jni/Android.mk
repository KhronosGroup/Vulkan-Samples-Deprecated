LOCAL_PATH := $(abspath $(call my-dir))

# base of the Vulkan-Samples repository
BASE_DIR := ../../../../../../..

include $(CLEAR_VARS)

LOCAL_MODULE := VkLayer_queue_muxer
LOCAL_C_INCLUDES := $(BASE_DIR)/Vulkan-LoaderAndValidationLayers/include \
                    $(BASE_DIR)/Vulkan-LoaderAndValidationLayers/build/layers \
                    $(BASE_DIR)/Vulkan-LoaderAndValidationLayers/layers \
                    $(BASE_DIR)/Vulkan-LoaderAndValidationLayers/loader \
                    $(VK_SDK_PATH)/Include \
                    $(VK_SDK_PATH)/Source/layers \
                    $(VK_SDK_PATH)/Source/loader
LOCAL_SRC_FILES  := $(BASE_DIR)/samples/layers/queue_muxer/queue_muxer.cpp
LOCAL_CPPFLAGS   := -std=c++11 -Wall -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_LDLIBS     := -llog

include $(BUILD_SHARED_LIBRARY)

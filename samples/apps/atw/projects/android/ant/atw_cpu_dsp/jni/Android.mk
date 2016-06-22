LOCAL_PATH := $(call my-dir)

# base of the Vulkan-Samples repository
BASE_DIR := ../../../../../../../..

include $(CLEAR_VARS)

LOCAL_MODULE			:= atw_cpu_dsp
LOCAL_C_INCLUDES		:= $(BASE_DIR)/external/include
LOCAL_SRC_FILES			:= ../../../../../atw_cpu_dsp.c
LOCAL_CFLAGS			:= -std=c99 -O3 -mfpu=neon -Wall -Wno-unused-function
LOCAL_LDLIBS			:= -L$(SYSROOT)/usr/lib -llog

include $(BUILD_EXECUTABLE)

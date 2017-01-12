#!/bin/bash

mkdir -p ../Vulkan-LoaderAndValidationLayers/build/generated/include
python3 ../Vulkan-LoaderAndValidationLayers/scripts/lvl_genvk.py -registry ../Vulkan-LoaderAndValidationLayers/scripts/vk.xml vk_dispatch_table_helper.h -o ../Vulkan-LoaderAndValidationLayers/build/generated/include/

${ANDROID_NDK_HOME}/ndk-build -C samples/apps/atw/projects/android/ndk/atw_cpu_dsp/ NDK_LIBS_OUT=../../../../../../../build/android/ndk/apps/atw_cpu_dsp/libs NDK_OUT=../../../../../../../build/android/ndk/apps/atw_cpu_dsp/obj
${ANDROID_NDK_HOME}/ndk-build -C samples/apps/atw/projects/android/ndk/atw_opengl/ NDK_LIBS_OUT=../../../../../../../build/android/ndk/apps/atw_opengl/libs NDK_OUT=../../../../../../../build/android/ndk/apps/atw_opengl/obj
${ANDROID_NDK_HOME}/ndk-build -C samples/apps/atw/projects/android/ndk/atw_vulkan/ NDK_LIBS_OUT=../../../../../../../build/android/ndk/apps/atw_vulkan/libs NDK_OUT=../../../../../../../build/android/ndk/apps/atw_vulkan/obj
${ANDROID_NDK_HOME}/ndk-build -C samples/apps/driverinfo/projects/android/ndk/driverinfo_opengl/ NDK_LIBS_OUT=../../../../../../../build/android/ndk/apps/driverinfo_opengl/libs NDK_OUT=../../../../../../../build/android/ndk/apps/driverinfo_opengl/obj
${ANDROID_NDK_HOME}/ndk-build -C samples/apps/driverinfo/projects/android/ndk/driverinfo_vulkan/ NDK_LIBS_OUT=../../../../../../../build/android/ndk/apps/driverinfo_vulkan/libs NDK_OUT=../../../../../../../build/android/ndk/apps/driverinfo_vulkan/obj
${ANDROID_NDK_HOME}/ndk-build -C samples/layers/glsl_shader/projects/android/ndk/ NDK_LIBS_OUT=../../../../../../build/android/ndk/layers/glsl_shader/libs NDK_OUT=../../../../../../build/android/ndk/layers/glsl_shader/obj
${ANDROID_NDK_HOME}/ndk-build -C samples/layers/queue_muxer/projects/android/ndk/ NDK_LIBS_OUT=../../../../../../build/android/ndk/layers/queue_muxer/libs NDK_OUT=../../../../../../build/android/ndk/layers/queue_muxer/obj

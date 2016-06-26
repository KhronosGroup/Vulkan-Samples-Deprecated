#!/bin/bash

set BUILD_DIR="../../../../../../build/android/ndk/layers/queue_muxer/"

ndk-build NDK_LIBS_OUT=${BUILD_DIR}libs NDK_OUT=${BUILD_DIR}obj

adb push ${BUILD_DIR}libs/armeabi-v7a/libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer-armeabi-v7a.so
adb push ${BUILD_DIR}libs/arm64-v8a/libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer-arm64-v8a.so
adb push ${BUILD_DIR}libs/x86/libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer-x86.so
adb push ${BUILD_DIR}libs/x86_64/libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer-x86_64.so

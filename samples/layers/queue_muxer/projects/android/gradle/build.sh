#!/bin/bash

set BUILD_DIR="../../../../../../build/android/gradle/layers/queue_muxer/"

gradlew --project-cache-dir ${BUILD_DIR}.gradle build

cp /Y ${BUILD_DIR}build/outputs/native/debug/arm7/lib/* ${BUILD_DIR}../../apps/atw_vulkan/build/intermediates/binaries/debug/arm7/lib/
cp /Y ${BUILD_DIR}build/outputs/native/release/arm7/lib/* ${BUILD_DIR}../../apps/atw_vulkan/build/intermediates/binaries/release/arm7/lib/

adb push ${BUILD_DIR}build/outputs/native/debug/arm7/lib/armeabi-v7a/libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer.so
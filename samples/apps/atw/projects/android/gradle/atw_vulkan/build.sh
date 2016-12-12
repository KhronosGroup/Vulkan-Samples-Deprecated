#!/bin/bash

set BUILD_DIR="../../../../../../../build/android/gradle/apps/atw_vulkan/"
set MUXER_DIR="../../../../../../../build/android/gradle/layers/queue_muxer/"
set GLSL_DIR="../../../../../../../build/android/gradle/layers/glsl_shader/"

cp /Y ${MUXER_DIR}outputs/native/debug/all/lib/* ${BUILD_DIR}intermediates/binaries/debug/all/lib/
cp /Y ${MUXER_DIR}outputs/native/release/all/lib/* ${BUILD_DIR}intermediates/binaries/release/all/lib/
cp /Y ${GLSL_DIR}outputs/native/debug/all/lib/* ${BUILD_DIR}intermediates/binaries/debug/all/lib/
cp /Y ${GLSL_DIR}outputs/native/release/all/lib/* ${BUILD_DIR}intermediates/binaries/release/all/lib/
gradlew --project-cache-dir ${BUILD_DIR}.gradle build
jar -tf ${BUILD_DIR}outputs/apk/atw_vulkan-all-debug.apk
adb install -r ${BUILD_DIR}outputs/apk/atw_vulkan-all-debug.apk
adb shell am start -n com.vulkansamples.atw_vulkan/android.app.NativeActivity

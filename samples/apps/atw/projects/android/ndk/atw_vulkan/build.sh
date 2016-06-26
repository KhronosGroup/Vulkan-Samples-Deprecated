#!/bin/bash

set BUILD_DIR="../../../../../../../build/android/ndk/apps/atw_vulkan/"
set MUXER_DIR="../../../../../../../build/android/ndk/layers/queue_muxer/"
set GLSL_DIR="../../../../../../../build/android/ndk/layers/glsl_shader/"

ndk-build NDK_LIBS_OUT=${BUILD_DIR}libs NDK_OUT=${BUILD_DIR}obj
cp AndroidManifest.xml ${BUILD_DIR}AndroidManifest.xml
cp build.xml ${BUILD_DIR}build.xml
cp project.properties ${BUILD_DIR}project.properties
cp /Y ${MUXER_DIR}libs/* ${BUILD_DIR}libs/
cp /Y ${GLSL_DIR}libs/* ${BUILD_DIR}libs/
ant -q debug -Dbasedir=${BUILD_DIR}
jar -tf ${BUILD_DIR}bin/atw_vulkan-debug.apk
adb install -r ${BUILD_DIR}bin/atw_vulkan-debug.apk
adb shell am start -n com.vulkansamples.atw_vulkan/android.app.NativeActivity -a "android.intent.action.MAIN"

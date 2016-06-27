#!/bin/bash

set BUILD_DIR="../../../../../../../build/android/ndk/apps/driverinfo_opengl/"

ndk-build NDK_LIBS_OUT=${BUILD_DIR}libs NDK_OUT=${BUILD_DIR}obj
copy AndroidManifest.xml ${BUILD_DIR}AndroidManifest.xml
copy build.xml ${BUILD_DIR}build.xml
copy project.properties ${BUILD_DIR}project.properties
ant -q debug -Dbasedir=${BUILD_DIR}
jar -tf ${BUILD_DIR}bin/driverinfo_opengl-debug.apk
adb install -r ${BUILD_DIR}bin/driverinfo_opengl-debug.apk
adb shell am start -n com.vulkansamples.driverinfo_opengl/android.app.NativeActivity -a "android.intent.action.MAIN"

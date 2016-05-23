#!/bin/bash

set BUILD_DIR="../../../../../../../build/android/gradle/apps/atw_opengl/"

gradlew --project-cache-dir ${BUILD_DIR}.gradle build
jar -tf ${BUILD_DIR}build/outputs/apk/atw_opengl-arm7-debug.apk
adb install -r ${BUILD_DIR}build/outputs/apk/atw_opengl-arm7-debug.apk
adb shell am start -n com.vulkansamples.atw_opengl/android.app.NativeActivity

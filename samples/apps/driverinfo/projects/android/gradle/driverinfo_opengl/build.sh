#!/bin/bash

set BUILD_DIR="../../../../../../../build/android/gradle/apps/driverinfo_opengl/"

gradlew --project-cache-dir ${BUILD_DIR}.gradle build
jar -tf ${BUILD_DIR}build/outputs/apk/driverinfo_opengl-all-debug.apk
adb install -r ${BUILD_DIR}build/outputs/apk/driverinfo_opengl-all-debug.apk
adb shell am start -n com.vulkansamples.driverinfo_opengl/android.app.NativeActivity

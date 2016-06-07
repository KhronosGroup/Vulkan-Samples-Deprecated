#!/bin/bash

set BUILD_DIR="../../../../../../../build/android/gradle/apps/atw_vulkan/"

gradlew --project-cache-dir ${BUILD_DIR}.gradle build
jar -tf ${BUILD_DIR}build/outputs/apk/atw_vulkan-all-debug.apk
adb install -r ${BUILD_DIR}build/outputs/apk/atw_vulkan-all-debug.apk
adb shell am start -n com.vulkansamples.atw_vulkan/android.app.NativeActivity

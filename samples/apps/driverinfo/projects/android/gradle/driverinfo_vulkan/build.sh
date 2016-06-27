#!/bin/bash

set BUILD_DIR="../../../../../../../build/android/gradle/apps/driverinfo_vulkan/"

gradlew --project-cache-dir ${BUILD_DIR}.gradle build
jar -tf ${BUILD_DIR}build/outputs/apk/driverinfo_vulkan-all-debug.apk
adb install -r ${BUILD_DIR}build/outputs/apk/driverinfo_vulkan-all-debug.apk
adb shell am start -n com.vulkansamples.driverinfo_vulkan/android.app.NativeActivity

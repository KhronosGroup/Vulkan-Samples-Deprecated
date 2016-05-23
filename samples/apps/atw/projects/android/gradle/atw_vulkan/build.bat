@echo off

set BUILD_DIR="../../../../../../../build/android/gradle/apps/atw_vulkan/"

call gradlew --project-cache-dir %BUILD_DIR%.gradle build
call jar -tf %BUILD_DIR%build\outputs\apk\atw_vulkan-arm7-debug.apk
call adb install -r %BUILD_DIR%build/outputs/apk/atw_vulkan-arm7-debug.apk
call adb shell am start -n com.vulkansamples.atw_vulkan/android.app.NativeActivity

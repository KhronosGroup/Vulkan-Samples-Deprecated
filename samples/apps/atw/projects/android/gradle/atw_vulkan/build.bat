@echo off

set BUILD_DIR="../../../../../../../build/android/gradle/apps/atw_vulkan/"
set MUXER_DIR="../../../../../../../build/android/gradle/layers/queue_muxer/"

call xcopy /Y /S %MUXER_DIR%build\outputs\native\debug\all\lib\* %BUILD_DIR%build\intermediates\binaries\debug\all\lib\
call xcopy /Y /S %MUXER_DIR%build\outputs\native\release\all\lib\* %BUILD_DIR%build\intermediates\binaries\release\all\lib\
call gradlew --project-cache-dir %BUILD_DIR%.gradle build
call jar -tf %BUILD_DIR%build\outputs\apk\atw_vulkan-all-debug.apk
call adb install -r %BUILD_DIR%build/outputs/apk/atw_vulkan-all-debug.apk
call adb shell am start -n com.vulkansamples.atw_vulkan/android.app.NativeActivity

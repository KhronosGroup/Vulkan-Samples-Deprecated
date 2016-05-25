@echo off

set BUILD_DIR="../../../../../../build/android/gradle/layers/queue_muxer/"

call gradlew --project-cache-dir %BUILD_DIR%.gradle build

xcopy /Y /S %BUILD_DIR%build\outputs\native\debug\all\lib\* %BUILD_DIR%..\..\apps\atw_vulkan\build\intermediates\binaries\debug\all\lib\
xcopy /Y /S %BUILD_DIR%build\outputs\native\release\all\lib\* %BUILD_DIR%..\..\apps\atw_vulkan\build\intermediates\binaries\release\all\lib\

adb push %BUILD_DIR%build\outputs\native\debug\all\lib\armeabi-v7a\libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer-armeabi-v7a.so
adb push %BUILD_DIR%build\outputs\native\debug\all\lib\arm64-v8a\libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer-arm64-v8a.so
adb push %BUILD_DIR%build\outputs\native\debug\all\lib\x86\libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer-x86.so
adb push %BUILD_DIR%build\outputs\native\debug\all\lib\x86_64\libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer-x86_64.so

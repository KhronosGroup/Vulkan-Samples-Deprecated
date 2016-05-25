@echo off

set BUILD_DIR="../../../../../../build/android/ant/layers/queue_muxer/"

call ndk-build NDK_LIBS_OUT=%BUILD_DIR%libs NDK_OUT=%BUILD_DIR%obj

xcopy /Y /S %BUILD_DIR%libs\* %BUILD_DIR%..\..\apps\atw_vulkan\libs\
xcopy /Y /S %BUILD_DIR%libs\* %BUILD_DIR%..\..\apps\atw_vulkan\libs\

adb push %BUILD_DIR%libs\armeabi-v7a\libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer-armeabi-v7a.so
adb push %BUILD_DIR%libs\arm64-v8a\libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer-arm64-v8a.so
adb push %BUILD_DIR%libs\x86\libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer-x86.so
adb push %BUILD_DIR%libs\x86_64\libVkLayer_queue_muxer.so /data/local/debug/vulkan/libVkLayer_queue_muxer-x86_64.so

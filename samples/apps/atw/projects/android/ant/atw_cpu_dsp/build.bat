@echo off

set BUILD_DIR="../../../../../../../build/android/ant/apps/atw_cpu_dsp/"

call ndk-build NDK_LIBS_OUT=%BUILD_DIR%libs NDK_OUT=%BUILD_DIR%obj
call adb push %BUILD_DIR%libs/armeabi-v7a/atw_cpu_dsp /data/local
call adb shell chmod 777 /data/local/atw_cpu_dsp
call adb shell mkdir -p /sdcard/atw/images/
call adb shell rm /sdcard/atw/images/*
call adb shell /data/local/atw_cpu_dsp
call adb pull /sdcard/atw/images/ %BUILD_DIR%images/

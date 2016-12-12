@echo off

set BUILD_DIR="../../../../../../../build/android/gradle/apps/atw_cpu_dsp/"

call gradlew --project-cache-dir %BUILD_DIR%.gradle build
call adb push %BUILD_DIR%libs/armeabi-v7a/atw_cpu_dsp /data/local
call adb shell chmod 777 /data/local/atw_cpu_dsp
call adb shell mkdir -p /sdcard/atw/images/
call adb shell rm /sdcard/atw/images/*
call adb shell /data/local/atw_cpu_dsp
call adb pull /sdcard/atw/images/ %BUILD_DIR%images/

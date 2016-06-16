@echo off

set BUILD_DIR="../../../../../../../build/android/gradle/apps/driverinfo_opengl/"

call gradlew --project-cache-dir %BUILD_DIR%.gradle build
call jar -tf %BUILD_DIR%build\outputs\apk\driverinfo_opengl-all-debug.apk
call adb install -r %BUILD_DIR%build/outputs/apk/driverinfo_opengl-all-debug.apk
call adb shell am start -n com.vulkansamples.driverinfo_opengl/android.app.NativeActivity

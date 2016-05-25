@echo off

set BUILD_DIR="../../../../../../../build/android/ant/apps/atw_vulkan/"

call ndk-build NDK_LIBS_OUT=%BUILD_DIR%libs NDK_OUT=%BUILD_DIR%obj
call copy AndroidManifest.xml %BUILD_DIR%AndroidManifest.xml
call copy build.xml %BUILD_DIR%build.xml
call copy project.properties %BUILD_DIR%project.properties
call ant -q debug -Dbasedir=%BUILD_DIR%
call jar -tf %BUILD_DIR%bin/atw_vulkan-debug.apk
call adb install -r %BUILD_DIR%bin/atw_vulkan-debug.apk
call adb shell am start -n com.vulkansamples.atw_vulkan/android.app.NativeActivity -a "android.intent.action.MAIN"

@echo ------------------------------------------------
@echo The Hexagon SDK must be installed to C:\Qualcomm\Hexagon_SDK\3.0\
@echo Run:
@echo    C:\Qualcomm\Hexagon_SDK\3.0\setup_sdk_env.cmd
@echo Do *NOT* run:
@echo    C:\Qualcomm\Hexagon_SDK\3.0\tools\scripts\push_adsp.cmd.
@echo Make sure to place a TestSig on the device using:
@echo    C:\Qualcomm\Hexagon_SDK\3.0\scripts\testsig.py
@echo Enable full root access with:
@echo    adb root
@echo    adb wait-for-device
@echo    adb remount
@echo Make sure that the adsprpcd driver is running using:
@echo    adb shell "ps | grep adsprpcd"
@echo If the adsprpcd driver is not running, then start it in a new command prompt:
@echo    adb shell chmod 777 /system/bin/adsprpcd
@echo    adb shell /system/bin/adsprpcd
@echo Use adb logcat to check for ION or RPC failures:
@echo    adb logcat -s adsprpc
@echo In debug builds use adb logcat for DSP debug messages:
@echo    adb logcat -s adspmsgd
@echo ------------------------------------------------

@rem run as root
@adb root
@adb wait-for-device
@adb remount

@if "%1" == "debug" (
	make tree V_toolv72=1 V_ARCH=v60 V=hexagon_Debug_dynamic
	make tree V_toolv72=1 V_ARCH=v60 V=android_Debug

	@rem python %HEXAGON_SDK_ROOT%\tools\elfsigner\elfsigner.py -i hexagon_Debug_dynamic/libTimeWarpInterface_skel.so -o hexagon_Debug_dynamic/signed/

	adb shell mkdir -p /system/lib/rfsa/adsp
	adb push android_Debug/ship/TimeWarp /data/local
	adb shell chmod 777 /data/local/TimeWarp
	adb push hexagon_Debug_dynamic/libTimeWarpInterface_skel.so /system/lib/rfsa/adsp
) else (
	make tree V_toolv72=1 V_ARCH=v60 V=hexagon_Release_dynamic
	make tree V_toolv72=1 V_ARCH=v60 V=android_Release

	@rem python %HEXAGON_SDK_ROOT%\tools\elfsigner\elfsigner.py -i hexagon_Release_dynamic/libTimeWarpInterface_skel.so -o hexagon_Release_dynamic/signed/

	adb shell mkdir -p /system/lib/rfsa/adsp
	adb push android_Release/ship/TimeWarp /data/local
	adb shell chmod 777 /data/local/TimeWarp
	adb push hexagon_Release_dynamic/libTimeWarpInterface_skel.so /system/lib/rfsa/adsp
)

@rem automatically start adsprpcd
@adb shell chmod 777 /system/bin/adsprpcd
@start adb shell /system/bin/adsprpcd

@echo on

adb shell mkdir -p /sdcard/atw/images/
adb shell rm /sdcard/atw/images/*
adb shell /data/local/TimeWarp
adb pull /sdcard/atw/images/ images/

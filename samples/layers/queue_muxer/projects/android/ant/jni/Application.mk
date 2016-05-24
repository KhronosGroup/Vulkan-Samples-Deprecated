# This needs to be defined to get the right header directories for egl / etc
APP_PLATFORM 	:= android-21

# This needs to be defined to avoid compile errors like:
# Error: selected processor does not support ARM mode `ldrex r0,[r3]'
APP_ABI 		:= armeabi-v7a

# Statically link the GNU STL.
APP_STL			:= gnustl_static

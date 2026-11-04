#!/bin/sh

# test program ${1} on the Android emulator or device of adb: filename.sh needs a Unix shell and the locale variables,
# Android has neither, its default locale is UTF-8 and only setlocale() gives an ASCII locale (no nl_langinfo() before API level 26)

device_path="/data/local/tmp/zenlib_testsuite"
status=0

adb shell "rm -rf ${device_path} && mkdir ${device_path}" || exit 1
adb push "${1}" "${device_path}/zen_filename" || exit 1

api="$(adb shell getprop ro.build.version.sdk | tr -d '\r')"
modes="0 1"
if [ "${api}" -ge 26 ] ; then
    modes="0 1 C"
fi

for mode in ${modes} ; do
    directory="${device_path}/setlocale_${mode}"
    if adb shell "mkdir ${directory} && ${device_path}/zen_filename ${mode} ${directory}" ; then
        echo "OK: filename/android/setlocale=${mode}"
    else
        echo "NOK: filename/android/setlocale=${mode}"
        adb logcat -d -s 'DEBUG:*' | tail -n 40 # backtrace of a crash
        status=1
    fi
done

exit ${status}

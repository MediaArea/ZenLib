#!/usr/bin/env bash

script_path="${PWD}/test"
. "${script_path}/helpers.sh"

test="longpath"

# file and directory names longer than MAX_PATH (260 characters with the null character), the limit of the functions of Windows when the
# process does not support long paths (ZenLib adds then the extended-length prefix, not in the names it gives): file, directory listing,
# relative path, temporary file, network path (Windows, administrative share of the drive of the temporary directory if available)

directory="${temp_path}/longpath"
mkdir "${directory}" || fatal "internal" "mkdir command failed"
network_root=
if test -n "${WINDOWS}" && [[ "${directory}" == [A-Za-z]:/* ]] ; then
    network_root="//localhost/${directory:0:1}\$"
    if ! [ -d "${network_root}${directory:2}" ] ; then
        echo "SKIP: ${test}, network path, ${network_root} not available" >&${fd}
        network_root=
    fi
fi
run_command unset "${script_path}/zen_longpath" "${directory}" ${network_root:+"${network_root}"}
report "default"

exit ${status}

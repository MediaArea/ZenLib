#!/usr/bin/env bash

script_path="${PWD}/test"
. "${script_path}/helpers.sh"

test="filename"

# file names with non-ASCII characters (conversions, File and Dir functions, directory listing) with an ASCII locale,
# a UTF-8 locale, a Latin-1 locale and an EUC-JP locale, by a program which calls setlocale() or not

for config in ${locale_configs} ; do
    for set_locale in 0 1 ; do
        directory="${temp_path}/${config}_${set_locale}"
        mkdir "${directory}" || fatal "internal" "mkdir command failed"
        run_command "${config}" "${script_path}/zen_filename" "${set_locale}" "${directory}"
        report "${config}/setlocale=${set_locale}"
    done
done

exit ${status}

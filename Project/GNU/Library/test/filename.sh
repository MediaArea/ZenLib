#!/usr/bin/env bash

script_path="${PWD}/test"
. "${script_path}/helpers.sh"

test="filename"

# file names with non-ASCII characters (conversions, File and Dir functions, directory listing) with an ASCII locale,
# a UTF-8 locale, a Latin-1 locale and an EUC-JP locale, by a program which calls setlocale() or not

# also with other encodings of locales if available, as some systems (e.g. macOS, FreeBSD) have a wchar_t which is not
# Unicode with them, single byte or multibyte
if test -z "${WINDOWS}" ; then
    for spec in "fr_FR ISO-8859-15 fr_FR.ISO8859-15 fr_FR.ISO-8859-15 fr_FR.iso885915" \
                "ru_RU KOI8-R ru_RU.KOI8-R ru_RU.koi8r" \
                "ru_RU CP1251 ru_RU.CP1251 ru_RU.cp1251" \
                "ja_JP SHIFT_JIS ja_JP.SJIS ja_JP.Shift_JIS ja_JP.sjis" \
                "zh_CN GBK zh_CN.GBK zh_CN.gbk" \
                "zh_TW BIG5 zh_TW.Big5 zh_TW.BIG5 zh_TW.big5" \
                "ko_KR EUC-KR ko_KR.eucKR ko_KR.EUC-KR ko_KR.euckr" ; do
        if find_locale other_locale ${spec} ; then
            locale_configs="${locale_configs} ${other_locale}"
        fi
    done
fi

for config in ${locale_configs} ; do
    for set_locale in 0 1 ; do
        directory="${temp_path}/${config}_${set_locale}"
        mkdir "${directory}" || fatal "internal" "mkdir command failed"
        run_command "${config}" "${script_path}/zen_filename" "${set_locale}" "${directory}"
        report "${config}/setlocale=${set_locale}"
    done
done

exit ${status}

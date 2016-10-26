# Upgrade the version number of ZenLib

function btask.UpgradeVersion.run () {
    
    if [ $(b.opt.get_opt --source-path) ]; then
        ZL_source=$(sanitize_arg $(b.opt.get_opt --source-path))
    else
        if [ $(b.opt.get_opt --repo-url) ]; then
            RepoURL=$(sanitize_arg $(b.opt.get_opt --repo-url))
        else
            RepoURL="git://github.com/MediaArea/ZenLib/"
        fi
        getSource /tmp $RepoURL
        ZL_source=/tmp/ZenLib
    fi

    echo
    echo "Passage for version with dots..."
    index=0
    ZL_files[((index++))]="Project/GNU/libzen.dsc"
    ZL_files[((index++))]="Project/GNU/libzen.spec"
    ZL_files[((index++))]="Project/Solaris/mkpkg"
    ZL_files[((index++))]="debian/changelog"
    ZL_files[((index++))]="debian/control"
    ZL_files[((index++))]="Project/GNU/Library/configure.ac"

    # Replace old version by new version
    for ZL_file in ${ZL_files[@]}
    do
        echo "${ZL_source}/${ZL_file}"
        updateFile $Version_old_dot $Version_new "${ZL_source}/${ZL_file}"
    done

    echo
    echo "Replace major/minor/patch in ${ZL_source}/Project/CMake/CMakeLists.txt"
    updateFile "set(ZenLib_MAJOR_VERSION \"$Version_old_major\")" \
        "set(ZenLib_MAJOR_VERSION \"$Version_new_major\")" \
        "${ZL_source}/Project/CMake/CMakeLists.txt"
    updateFile "set(ZenLib_MINOR_VERSION \"$Version_old_minor\")" \
        "set(ZenLib_MINOR_VERSION \"$Version_new_minor\")" \
        "${ZL_source}/Project/CMake/CMakeLists.txt"
    updateFile "set(ZenLib_PATCH_VERSION \"$Version_old_patch\")" \
        "set(ZenLib_PATCH_VERSION \"$Version_new_patch\")" \
        "${ZL_source}/Project/CMake/CMakeLists.txt"

    unset -v ZL_files index ZL_source
}

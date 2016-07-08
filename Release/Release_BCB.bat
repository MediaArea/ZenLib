@echo off

rem --- Search binaries ---
set BPATH=
if exist "%~dp0\..\..\..\MediaArea-Utils-Binaries" set BPATH="%~dp0\..\..\..\MediaArea-Utils-Binaries"
if exist "%~dp0\..\..\MediaArea-Utils-Binaries" set BPATH="%~dp0\..\..\MediaArea-Utils-Binaries"
if "%BPATH%"=="" (
    echo "ERROR: binaries path not found"
    exit /b 1
)

rem --- Clean up ---
if exist ZenLib_BCB.7z del ZenLib_BCB.7z
rmdir ZenLib_BCB /S /Q
mkdir ZenLib_BCB


rem --- Copying : Include ---
xcopy ..\Source\ZenLib\*.h ZenLib_BCB\Include\ZenLib\

rem --- Copying : Documentation ---
mkdir Doc
pushd ..\Source\Doc
%BPATH%\Windows\Doxygen\doxygen
popd
mkdir ZenLib_BCB\Doc\
xcopy Doc\*.*  ZenLib_BCB\Doc\
rmdir Doc /S /Q
xcopy ..\Source\Doc\*.html ZenLib_BCB\ /S

rem --- Copying : Library ---
copy BCB\Library\ZenLib.lib ZenLib_BCB\

rem --- Copying : Information files ---
copy ..\*.txt ZenLib_BCB\


rem --- Compressing Archive ---
pushd ZenLib_BCB
%BPATH%\Windows\7-Zip\7z a -r -t7z -mx9 ..\ZenLib_BCB.7z *
popd

rem --- Clean up ---
rmdir ZenLib_BCB /S /Q

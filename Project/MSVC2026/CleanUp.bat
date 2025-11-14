@echo off

rem MS Visual Studio specific ---
if exist *.opensdf del *.opensdf
if exist *.sdf     del *.sdf

pushd Example
if exist Debug   rmdir Debug /Q /S
if exist Release rmdir Release /Q /S
if exist Win32   rmdir Win32 /Q /S
if exist x64     rmdir x64 /Q /S
if exist *.ncb   del *.ncb
if exist *.user  del *.user
if exist *.suo   del *.suo /AH
popd

pushd Library
if exist Debug   rmdir Debug /Q /S
if exist Release rmdir Release /Q /S
if exist Win32   rmdir Win32 /Q /S
if exist x64     rmdir x64 /Q /S
if exist *.ncb   del *.ncb
if exist *.user  del *.user
if exist *.suo   del *.suo /AH
popd

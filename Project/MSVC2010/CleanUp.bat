@rem echo off

@rem MS Visual Studio specific ---
del *.opensdf *.sdf
cd Example
rmdir Win32 /Q /S
rmdir x64/Q /S
del *.ncb *.user *.filters
del *.suo /AH
cd ..
cd Library
rmdir Win32 /Q /S
rmdir x64/Q /S
del *.ncb *.user *.filters
del *.suo /AH
cd ..

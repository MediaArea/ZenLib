@rem echo off

@rem Borland Developper Studio ---
cd ..\Project\BCB
call CleanUp
cd ..\..\Release

@rem Code::Blocks ---
cd ..\Project\CodeBlocks
call CleanUp
cd ..\..\Release

@rem GCC ---
cd ..\Project\GCC
Ccall leanUp
cd ..\..\Release

@rem MS Visual Studio ---
cd ..\Project\MSVC
call CleanUp
cd ..\..\Release
cd ..\Project\MSVC2005
call CleanUp
cd ..\..\Release

@rem Release ---
del *.zip *.gz *.bz2 *.7z /Q /S

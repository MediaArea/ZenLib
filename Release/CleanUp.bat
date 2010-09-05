@rem echo off
@rem --- General ---
cd ..
del *.~* *.obj *.o *.tds *.dcu *.ddp *.opt *.ncb *.suo *.ilk *.idb *.pdb *.pch *.plg *.aps *.user *.win *.layout *.local *.depend *.identcache *.tgs *.tgw *.sdf /Q /S
del *.~* *.obj *.o *.tds *.dcu *.ddp *.opt *.ncb *.suo *.ilk *.idb *.pdb *.pch *.plg *.aps *.user *.win *.layout *.local *.depend  *.identcache *.tgs *.tgw *.sdf /AH /Q /S
cd Release

@rem Borland Developper Studio ---
cd ..\Project\BCB
call CleanUp
cd ..\..\Release

@rem Code::Blocks ---
cd ..\Project\CodeBlocks
call CleanUp
cd ..\..\Release

@rem MS Visual Studio ---
cd ..\Project\MSVC2010
call CleanUp
cd ..\..\Release
cd ..\Project\MSVC2008
call CleanUp
cd ..\..\Release
cd ..\Project\MSVC2005
call CleanUp
cd ..\..\Release

@rem Release ---
del *.zip *.gz *.bz2 *.7z /Q /S

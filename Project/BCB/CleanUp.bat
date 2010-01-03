@rem echo off

@rem Borland Developper Studio specific ---
cd Example
del *.local
cd Debug_Build
del *.* /Q
cd ..
cd Release_Build
del *.* /Q
cd ..
cd ..
cd Library
del *.local
cd Debug_Build
del *.* /Q
cd ..
cd Release_Build
del *.* /Q
cd ..
cd ..

@rem Borland __history files ---
rmdir Example\__history /Q /S
rmdir Library\__history/Q /S
rmdir ..\..\Source\ZenLib\__history/Q /S
rmdir ..\..\Source\ZenLib\Base64\__history/Q /S
rmdir ..\..\Source\ZenLib\Format\__history/Q /S
rmdir ..\..\Source\ZenLib\Format\Html\__history/Q /S
rmdir ..\..\Source\ZenLib\Format\Http\__history/Q /S
rmdir ..\..\Source\ZenLib\HTTP_Client\__history/Q /S

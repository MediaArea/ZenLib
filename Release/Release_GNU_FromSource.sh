#! /bin/sh

#-----------------------------------------------------------------------
# Clean up
test -e ZenLib_GNU_FromSource.tar     && rm    ZenLib_GNU_FromSource.tar
test -e ZenLib_GNU_FromSource.tar.bz2 && rm    ZenLib_GNU_FromSource.tar.bz2
test -d ZenLib_GNU_FromSource         && rm -r ZenLib_GNU_FromSource
mkdir ZenLib_GNU_FromSource


#-----------------------------------------------------------------------
# Preparing : Project
cd ../Project/GNU/Library
chmod u+x autogen
./autogen
cd ../../../Release

#-----------------------------------------------------------------------
# Copying : License files
cp ../License.txt ZenLib_GNU_FromSource/

#-----------------------------------------------------------------------
# Copying : History
cp ../History.txt ZenLib_GNU_FromSource/

#-----------------------------------------------------------------------
# Copying : Information file
cp ../ReadMe.txt ZenLib_GNU_FromSource/

#-----------------------------------------------------------------------
# Copying : Sources
mkdir -p ZenLib_GNU_FromSource/Source/ZenLib
cp -r ../Source/ZenLib/*.h ZenLib_GNU_FromSource/Source/ZenLib/
cp -r ../Source/ZenLib/*.cpp ZenLib_GNU_FromSource/Source/ZenLib/
mkdir ZenLib_GNU_FromSource/Source/ZenLib/HTTP_Client
cp -r ../Source/ZenLib/HTTP_Client/*.h ZenLib_GNU_FromSource/Source/ZenLib/HTTP_Client
cp -r ../Source/ZenLib/HTTP_Client/*.c ZenLib_GNU_FromSource/Source/ZenLib/HTTP_Client
mkdir ZenLib_GNU_FromSource/Source/ZenLib/Base64
cp -r ../Source/ZenLib/Base64/*.h ZenLib_GNU_FromSource/Source/ZenLib/Base64
mkdir ZenLib_GNU_FromSource/Source/ZenLib/Format
mkdir ZenLib_GNU_FromSource/Source/ZenLib/Format/Html
cp -r ../Source/ZenLib/Format/Html/*.h ZenLib_GNU_FromSource/Source/ZenLib/Format/Html
cp -r ../Source/ZenLib/Format/Html/*.cpp ZenLib_GNU_FromSource/Source/ZenLib/Format/Html
mkdir ZenLib_GNU_FromSource/Source/ZenLib/Format/Http
cp -r ../Source/ZenLib/Format/Http/*.h ZenLib_GNU_FromSource/Source/ZenLib/Format/Http
cp -r ../Source/ZenLib/Format/Http/*.cpp ZenLib_GNU_FromSource/Source/ZenLib/Format/Http
mkdir ZenLib_GNU_FromSource/Source/ZenLib/TinyXml
cp -r ../Source/ZenLib/TinyXml/*.h ZenLib_GNU_FromSource/Source/ZenLib/TinyXml
cp -r ../Source/ZenLib/TinyXml/*.cpp ZenLib_GNU_FromSource/Source/ZenLib/TinyXml

#-----------------------------------------------------------------------
# Copying : Projects
mkdir -p ZenLib_GNU_FromSource/Project/GNU/Library
cp ../Project/GNU/Library/aclocal.m4 ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/autogen ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/config.guess ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/config.sub ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/configure ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/configure.ac ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/depcomp ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/install-sh ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/ltmain.sh ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/Makefile.am ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/Makefile.in ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/missing ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/wxwin.m4 ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/libzen.pc.in ZenLib_GNU_FromSource/Project/GNU/Library/
cp ../Project/GNU/Library/libzen-config.in ZenLib_GNU_FromSource/Project/GNU/Library/


#-----------------------------------------------------------------------
# Compressing Archive
tar jchf ZenLib_GNU_FromSource.tar.bz2 ZenLib_GNU_FromSource/*

#-----------------------------------------------------------------------
# Clean up
rm -r ZenLib_GNU_FromSource

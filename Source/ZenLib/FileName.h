/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// File name related functions
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef ZenLib_FileNameH
#define ZenLib_FileNameH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "ZenLib/Ztring.h"
//---------------------------------------------------------------------------

namespace ZenLib
{

//***************************************************************************
/// @brief File name manipulation
//***************************************************************************

class FileName : public ZenLib::Ztring
{
public :
    //Constructor/Destructor
    FileName ()                                                                 : Ztring(){}
    FileName (const tstring& str)                                               : Ztring(str){}
    FileName (const tstring& str, size_type pos, size_type n=npos)              : Ztring(str, pos, n){}
    FileName (const Char* s, size_type n)                                       : Ztring(s, n){}
    FileName (const Char* s)                                                    : Ztring(s){}
    FileName (size_type n, char c)                                              : Ztring(n, c){}
    
    //Read/Write
    ZenLib::Ztring  Path_Get             () const;
    ZenLib::Ztring  Name_Get             () const;
    ZenLib::Ztring  Extension_Get        () const;
    ZenLib::Ztring& Path_Set             (const Ztring &Path);
    ZenLib::Ztring& Name_Set             (const Ztring &Name);
    ZenLib::Ztring& Extension_Set        (const Ztring &Extension);

    //Names related to this one, separators are "/", also "\" on Windows
    //Only FolderName_Get() and Absolute_Get() read the current folder (a system call), for a name which does not have the needed folders
    //The current folder is the one to which the system applies ".." (on Windows with its junctions and directory links)
    //On Windows without Unicode, the names they build from the current folder, and the names from the root built by Absolute_Get(), are UTF-8 text
    ZenLib::Ztring  FullName_Get         () const;                              ///< Name with its extension, without its folders (and without the drive of "C:name" on Windows)
    size_type       FullName_RFind       (Char ToFind) const;                   ///< Position in the whole name of the last ToFind of the full name (not in a multibyte character), npos if there is none
    ZenLib::Ztring  FolderName_Get       (size_t Levels_Up=0) const;            ///< Name of the folder of the file, or of the folder Levels_Up folders above, with "." and ".." applied, from the current folder if it is not in the name (e.g. "name", "../name"); empty above the root or the host of an URL
    ZenLib::Ztring  FolderPath_Get       (size_t Levels_Up=0) const;            ///< Path of the same folder, in the form of this name ("." for "name"), without path separator at the end except for a root
    ZenLib::Ztring  FromFolder_Get       (const Ztring &Name, size_t Levels_Up=0) const; ///< Name of a file given from the same folder (e.g. "../dir/name"), in the form of this name; empty if this name is empty
    ZenLib::Ztring  Root_Get             () const;                              ///< Root of the name with its path separator ("/", on Windows also "C:\", "\", "\\server\share\", "\\?\C:\" or "\\?\UNC\server\share\"), or the drive of "C:name" on Windows; empty for a name from the current folder or an URL
    ZenLib::Ztring  Absolute_Get         () const;                              ///< Name from the root, with "." and ".." applied, without empty folders, from the current folder if it is not from the root (also "\name" and "C:name" on Windows); this name if it is empty or an URL, or if the current folder is unknown
    ZenLib::Ztring  ExtendedLength_Get   () const;                              ///< Extended-length name of Windows ("\\?\"), in which the system does not apply "." and "..", with them applied; this name if it has another form

    //Helpers
    static ZenLib::Ztring Path_Get              (const Ztring &File_Name)       {return ((FileName&)File_Name).Path_Get();}
    static ZenLib::Ztring Name_Get              (const Ztring &File_Name)       {return ((FileName&)File_Name).Name_Get();}
    static ZenLib::Ztring Extension_Get         (const Ztring &File_Name)       {return ((FileName&)File_Name).Extension_Get();}
    static ZenLib::Ztring FullName_Get          (const Ztring &File_Name)       {return ((FileName&)File_Name).FullName_Get();}
    static size_type      FullName_RFind        (const Ztring &File_Name, Char ToFind) {return ((FileName&)File_Name).FullName_RFind(ToFind);}
    static ZenLib::Ztring FolderName_Get        (const Ztring &File_Name, size_t Levels_Up=0) {return ((FileName&)File_Name).FolderName_Get(Levels_Up);}
    static ZenLib::Ztring FolderPath_Get        (const Ztring &File_Name, size_t Levels_Up=0) {return ((FileName&)File_Name).FolderPath_Get(Levels_Up);}
    static ZenLib::Ztring FromFolder_Get        (const Ztring &File_Name, const Ztring &Name, size_t Levels_Up=0) {return ((FileName&)File_Name).FromFolder_Get(Name, Levels_Up);}
    static ZenLib::Ztring Root_Get              (const Ztring &File_Name)       {return ((FileName&)File_Name).Root_Get();}
    static ZenLib::Ztring Absolute_Get          (const Ztring &File_Name)       {return ((FileName&)File_Name).Absolute_Get();}
    static ZenLib::Ztring ExtendedLength_Get    (const Ztring &File_Name)       {return ((FileName&)File_Name).ExtendedLength_Get();}
    static ZenLib::Ztring TempFileName_Create   (const Ztring &Prefix);
};

//Platform differences (no more used by ZenLib, which accepts "/" and also "\" on Windows)
extern const Char* FileName_PathSeparator;

} //NameSpace

//---------------------------------------------------------------------------
#ifdef __BORLANDC__
    #pragma warn .8027
#endif
//---------------------------------------------------------------------------

#endif

/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "ZenLib/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "ZenLib/Conf_Internal.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "ZenLib/FileName.h"
#include "ZenLib/Dir.h"
#include "ZenLib/File.h"
#ifdef ZENLIB_USEWX
    #include <wx/filename.h>
#else //ZENLIB_USEWX
    #ifdef ZENLIB_STANDARD
        #undef WINDOWS
    #endif
    #ifdef WINDOWS
        #undef __TEXT
        #include <windows.h>
    #endif
#endif //ZENLIB_USEWX
#include <vector>
//---------------------------------------------------------------------------
#if defined(WINDOWS) && !defined(UNICODE) && !defined(ZENLIB_USEWX)
    #define ZENLIB_FILENAME_CODEPAGE_BYTES //A string which is not UTF-8 text is the bytes of the code page (see Ztring::To_Unicode())
#endif
#undef ZENLIB_USEWX

namespace ZenLib
{

#if defined(ZENLIB_FILENAME_CODEPAGE_BYTES)
//---------------------------------------------------------------------------
//In Ztring.cpp
ZENLIB_INTERNAL bool IsCodePageBytes(const char* S, size_t Size);
#endif

//---------------------------------------------------------------------------
//In OS_Utils.cpp
ZENLIB_INTERNAL Ztring CurrentFolder_Get(const Ztring &Drive);

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
// Name in which ASCII characters can be searched, at the same positions
// Without Unicode on Windows, in the bytes of the code page given as they are (see Ztring::To_Unicode()), the second byte of a character of
// a multibyte code page (e.g. "\" in cp932) is not a character, so it is replaced, UTF-8 text has no such byte
#if defined(WINDOWS) && !defined(UNICODE)
static Ztring Characters_Get(const Ztring &Name)
{
    Ztring ToReturn(Name);
    #if defined(ZENLIB_FILENAME_CODEPAGE_BYTES)
        if (!IsCodePageBytes(Name.c_str(), Name.size()))
            return ToReturn;
    #endif
    for (size_t Pos=0; Pos<ToReturn.size(); Pos++)
        if (IsDBCSLeadByte((BYTE)ToReturn[Pos]) && ++Pos<ToReturn.size())
            ToReturn[Pos]='\0'; //Second byte
    return ToReturn;
}
#else
static const Ztring& Characters_Get(const Ztring &Name)
{
    return Name;
}
#endif

//---------------------------------------------------------------------------
// Path separators: "/", also "\" on Windows
#if defined(WINDOWS)
    static const Char* const PathSeparators=__T("\\/");
#else
    static const Char* const PathSeparators=__T("/");
#endif

//---------------------------------------------------------------------------
// Position of the last path separator
static size_t PathSeparator_RFind(const Ztring &Name)
{
    return Characters_Get(Name).find_last_of(PathSeparators);
}

//---------------------------------------------------------------------------
// Size of the drive of a name ("C:" of "C:\name" or "C:name" on Windows)
static size_t Drive_Size_Get(const Ztring &Name)
{
    #if defined(WINDOWS)
        if (Name.size()>=2 && Name[1]==__T(':') && Name[0]!=__T('\\') && Name[0]!=__T('/'))
            return 2;
    #else
        (void)Name;
    #endif
    return 0;
}

//---------------------------------------------------------------------------
// Size of the folders of a name with the last path separator, of the drive for "C:name" on Windows
static size_t Folders_Size_Get(const Ztring &Name)
{
    size_t Pos=PathSeparator_RFind(Name);
    return Pos==Ztring::npos?Drive_Size_Get(Name):(Pos+1);
}

//---------------------------------------------------------------------------
// "scheme://..."
static bool IsUrl(const Ztring &Name)
{
    size_t Pos=Name.find(__T("://"));
    return Pos!=Ztring::npos && Pos>=2 && Name.find_first_of(PathSeparators)==Pos+1;
}

//---------------------------------------------------------------------------
// Folders of a name
struct ZENLIB_INTERNAL name_folders
{
    std::vector<size_t> Separators; //Positions of the path separators
    size_t              Root_Count; //Count of path separators of the root: 1 for "/" (also "\" and "C:\" on Windows), 4 for "\\server\share\" or "\\?\C:\", 6 for "\\?\UNC\server\share\", 2 for "scheme://" of an URL (its host is its first folder), 0 if the name is not from the root
    size_t              Drive_Size;

    explicit name_folders(const Ztring &Name)
        : Root_Count(0)
        , Drive_Size(Drive_Size_Get(Name))
    {
        const Ztring& Characters=Characters_Get(Name);
        for (size_t Pos=Characters.find_first_of(PathSeparators); Pos!=Ztring::npos; Pos=Characters.find_first_of(PathSeparators, Pos+1))
            Separators.push_back(Pos);
        if (!Separators.empty() && Separators[0]==Drive_Size)
        {
            Root_Count=1;
            #if defined(WINDOWS)
                if (!Drive_Size && Separators.size()>=2 && Separators[1]==1)
                    Root_Count=Name.compare(0, 8, __T("\\\\?\\UNC\\"))?4:6;
            #endif
        }
        else if (IsUrl(Name))
            Root_Count=2;
    }

    //Size of the root with its last path separator, all the name if the root is not complete (e.g. "\\server")
    size_t Root_Size(const Ztring &Name) const
    {
        return Root_Count>Separators.size()?Name.size():(Separators[Root_Count-1]+1);
    }

    //Folder before a path separator
    size_t Folder_Begin(size_t Pos) const
    {
        return Pos?(Separators[Pos-1]+1):Drive_Size;
    }
    Ztring Folder(const Ztring &Name, size_t Pos) const
    {
        size_t Begin=Folder_Begin(Pos);
        return Ztring(Name, Begin, Separators[Pos]-Begin);
    }
    bool Folder_Is(const Ztring &Name, size_t Pos, const Char* Text) const
    {
        size_t Begin=Folder_Begin(Pos);
        return !Name.compare(Begin, Separators[Pos]-Begin, Text);
    }
};

//---------------------------------------------------------------------------
// Name from the root, from the current folder (or its root for "\name" on Windows) if needed, empty if the current folder is unknown
static Ztring FromRoot_Get(const Ztring &File_Name)
{
    #if defined(WINDOWS) && !defined(UNICODE)
        Ztring Name=Ztring().From_Unicode(File_Name.To_Unicode()); //UTF-8 text, as the current folder
    #else
        const Ztring& Name=File_Name;
    #endif
    name_folders Folders(Name);
    #if defined(WINDOWS)
        if (Folders.Root_Count>(Folders.Drive_Size?(size_t)0:(size_t)1))
            return Name;
    #else
        if (Folders.Root_Count)
            return Name;
    #endif

    Ztring ToReturn=CurrentFolder_Get(Ztring(Name, 0, Folders.Drive_Size));
    name_folders ToReturn_Folders(ToReturn);
    if (!ToReturn_Folders.Root_Count)
        return Ztring();
    if (Folders.Root_Count)
        ToReturn.resize(ToReturn_Folders.Root_Size(ToReturn));
    ToReturn+=PathSeparator;
    ToReturn.append(Name, Folders.Drive_Size, Ztring::npos);
    return ToReturn;
}

//---------------------------------------------------------------------------
// Name from the root with "." and ".." applied, without empty folders (and without path separator at the end, except a root)
static Ztring Folders_Apply(const Ztring &Name)
{
    name_folders Folders(Name);
    size_t Begin=Folders.Root_Size(Name);
    Ztring ToReturn(Name, 0, Begin);
    for (size_t Pos=0; Pos<Folders.Root_Count && Pos<Folders.Separators.size(); Pos++)
        ToReturn[Folders.Separators[Pos]]=PathSeparator;
    Folders.Separators.push_back(Name.size());
    std::vector<size_t> Sizes; //Size of ToReturn before each folder
    for (size_t Pos=Folders.Root_Count; Pos<Folders.Separators.size(); Pos++)
    {
        size_t Size=Folders.Separators[Pos]-Begin;
        if (Size==2 && !Name.compare(Begin, Size, __T("..")))
        {
            if (!Sizes.empty())
            {
                ToReturn.resize(Sizes.back());
                Sizes.pop_back();
            }
        }
        else if (Size && (Size!=1 || Name[Begin]!=__T('.')))
        {
            Sizes.push_back(ToReturn.size());
            if (Sizes.size()>1)
                ToReturn+=PathSeparator;
            ToReturn.append(Name, Begin, Size);
        }
        Begin=Folders.Separators[Pos]+1;
    }
    return ToReturn;
}

//***************************************************************************
// Read/Write
//***************************************************************************

//---------------------------------------------------------------------------
Ztring FileName::Path_Get() const
{
    #ifdef ZENLIB_USEWX
        wxFileName FN(c_str());
        return FN.GetPath().c_str();
    #else //ZENLIB_USEWX
        //Path limit
        size_t Pos_Path=PathSeparator_RFind(*this);
        if (Pos_Path==Ztring::npos)
            return Ztring(); //Not found
        else
            return Ztring(*this, 0, Pos_Path);
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
Ztring& FileName::Path_Set(const Ztring &Path)
{
    #ifdef ZENLIB_USEWX
        wxFileName FN(c_str());
        FN.SetPath(Path.c_str());
        assign (FN.GetFullPath().c_str());
    #else //ZENLIB_USEWX
        #ifdef WINDOWS
            //Path limit
            size_t Pos_Path=PathSeparator_RFind(*this);
            if (Pos_Path==Ztring::npos)
            {
                insert(0, 1, __T('\\')); //Not found
                Pos_Path=0;
            }
            replace(0, Pos_Path, Path, 0, Ztring::npos);
        #else
            //Not supported
            (void)Path;
        #endif
    #endif //ZENLIB_USEWX
    return *this;
}

//---------------------------------------------------------------------------
Ztring FileName::Name_Get() const
{
    #ifdef ZENLIB_USEWX
        wxFileName FN(c_str());
        if (FN==FN.GetName()) //Bug of WxWidgets? if C:\\dir\\(no name), name is C:\\dir\\(no name)
            return Ztring();
        return FN.GetName().c_str();
    #else //ZENLIB_USEWX
        size_t Pos_Path=PathSeparator_RFind(*this); //Path limit
        if (Pos_Path==Ztring::npos)
            Pos_Path=0; //Not found
        else
            Pos_Path++; //Path separator size
        //Extension limit
        size_t Pos_Ext=rfind(__T('.'));
        if (Pos_Ext==Ztring::npos || Pos_Ext<Pos_Path)
            Pos_Ext=size(); //Not found
        return Ztring(*this, Pos_Path, Pos_Ext-Pos_Path);
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
Ztring& FileName::Name_Set(const Ztring &Name)
{
    #ifdef ZENLIB_USEWX
        wxFileName FN(c_str());
        if (FN==FN.GetName()) //Bug of WxWidgets? if C:\\dir\\(no name), name is C:\\dir\\(no name)
            FN.SetPath(c_str());
        FN.SetName(Name.c_str());
        assign ((FN.GetFullPath()+FN.GetPathSeparator()/*FileName_PathSeparator*/+FN.GetFullName()).c_str());
    #else //ZENLIB_USEWX
        #ifdef WINDOWS
            //Path limit
            size_t Pos_Path=PathSeparator_RFind(*this);
            if (Pos_Path==Ztring::npos)
                Pos_Path=0; //Not found
            //Extension limit
            size_t Pos_Ext=rfind(__T('.'));
            if (Pos_Ext==Ztring::npos || Pos_Ext<Pos_Path)
                Pos_Ext=size(); //Not found
            replace(Pos_Path+1, Pos_Ext-Pos_Path-1, Name, 0, Ztring::npos);
        #else
            //Not supported
            (void)Name;
        #endif
    #endif //ZENLIB_USEWX
    return *this;
}

//---------------------------------------------------------------------------
Ztring FileName::Extension_Get() const
{
    #ifdef ZENLIB_USEWX
        wxFileName FN(c_str());
        return FN.GetExt().c_str();
    #else //ZENLIB_USEWX
        //Path limit
        size_t Pos_Path=PathSeparator_RFind(*this);
        if (Pos_Path==Ztring::npos)
            Pos_Path=0; //Not found
        //Extension limit
        size_t Pos_Ext=rfind(__T('.'));
        if (Pos_Ext==Ztring::npos || Pos_Ext<Pos_Path)
            return Ztring(); //Not found
        else
            return Ztring(*this, Pos_Ext+1, size()-Pos_Ext-1);
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
Ztring& FileName::Extension_Set(const Ztring &Extension)
{
    #ifdef ZENLIB_USEWX
        wxFileName FN(c_str());
        FN.SetExt(Extension.c_str());
        assign (FN.GetFullPath().c_str());
    #else //ZENLIB_USEWX
        #ifdef WINDOWS
            //Path limit
            size_t Pos_Path=PathSeparator_RFind(*this);
            if (Pos_Path==Ztring::npos)
                Pos_Path=0; //Not found
            //Extension limit
            size_t Pos_Ext=rfind(__T('.'));
            if (Pos_Ext==Ztring::npos || Pos_Ext<Pos_Path)
            {
                append(1, __T('.')); //Not found
                Pos_Ext=size()-1;
            }
            replace(Pos_Ext+1, size()-Pos_Ext-1, Extension, 0, Ztring::npos);
        #else
            //Not supported
            (void)Extension;
        #endif
    #endif //ZENLIB_USEWX
    return *this;
}

//***************************************************************************
// Related names
//***************************************************************************

//---------------------------------------------------------------------------
Ztring FileName::FullName_Get() const
{
    return Ztring(*this, Folders_Size_Get(*this));
}

//---------------------------------------------------------------------------
FileName::size_type FileName::FullName_RFind(Char ToFind) const
{
    size_t ToReturn=Characters_Get(*this).rfind(ToFind);
    return ToReturn!=npos && ToReturn>=Folders_Size_Get(*this)?ToReturn:npos;
}

//---------------------------------------------------------------------------
Ztring FileName::FolderName_Get(size_t Levels_Up) const
{
    name_folders Folders(*this);
    size_t Levels=Levels_Up;
    size_t Parents=0;
    for (size_t Pos=Folders.Separators.size(); Pos>Folders.Root_Count; Pos--)
    {
        Ztring Folder=Folders.Folder(*this, Pos-1);
        if (Folder.empty() || Folder==__T("."))
            continue;
        if (Folder==__T(".."))
            Parents++;
        else if (Parents)
            Parents--;
        else if (Levels)
            Levels--;
        else
            return Folder;
    }
    if (empty() || Folders.Root_Count)
        return Ztring();
    return FolderName_Get(FromRoot_Get(*this), Levels_Up);
}

//---------------------------------------------------------------------------
Ztring FileName::FolderPath_Get(size_t Levels_Up) const
{
    Ztring ToReturn=FromFolder_Get(Ztring(), Levels_Up);
    if (ToReturn.empty() && !empty())
        return __T(".");
    name_folders Folders(ToReturn);
    while (Folders.Separators.size()>Folders.Root_Count && Folders.Separators.back()+1==ToReturn.size())
    {
        ToReturn.resize(Folders.Separators.back());
        Folders.Separators.pop_back();
    }
    return ToReturn;
}

//---------------------------------------------------------------------------
Ztring FileName::FromFolder_Get(const Ztring &Name, size_t Levels_Up) const
{
    if (empty())
        return Ztring();
    name_folders Folders(*this);
    while (Levels_Up && Folders.Separators.size()>Folders.Root_Count)
    {
        size_t Pos=Folders.Separators.size()-1;
        if (Folders.Folder_Is(*this, Pos, __T("..")))
            break;
        if (!Folders.Folder_Is(*this, Pos, __T("")) && !Folders.Folder_Is(*this, Pos, __T(".")))
            Levels_Up--;
        Folders.Separators.pop_back();
    }
    Ztring ToReturn(*this, 0, Folders.Separators.empty()?Folders.Drive_Size:(Folders.Separators.back()+1));
    for (; Levels_Up; Levels_Up--)
    {
        ToReturn+=__T("..");
        ToReturn+=PathSeparator;
    }
    ToReturn+=Name;
    return ExtendedLength_Get(ToReturn);
}

//---------------------------------------------------------------------------
Ztring FileName::Root_Get() const
{
    if (IsUrl(*this))
        return Ztring();
    name_folders Folders(*this);
    return Ztring(*this, 0, Folders.Root_Count?Folders.Root_Size(*this):Folders.Drive_Size);
}

//---------------------------------------------------------------------------
Ztring FileName::Absolute_Get() const
{
    if (empty() || IsUrl(*this))
        return *this;
    Ztring ToReturn=FromRoot_Get(*this);
    if (ToReturn.empty())
        return *this;
    return Folders_Apply(ToReturn);
}

//---------------------------------------------------------------------------
Ztring FileName::ExtendedLength_Get() const
{
    #if defined(WINDOWS)
        if (!compare(0, 4, __T("\\\\?\\")))
        {
            Ztring ToReturn=Folders_Apply(*this);
            if (Folders_Size_Get(*this)==size() && Folders_Size_Get(ToReturn)!=ToReturn.size())
                ToReturn+=PathSeparator; //A folder given with its path separator
            return ToReturn;
        }
    #endif
    return *this;
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
Ztring FileName::TempFileName_Create(const Ztring &Prefix)
{
    #ifdef ZENLIB_USEWX
        return wxFileName::CreateTempFileName(Prefix.c_str()).c_str();
    #else //ZENLIB_USEWX
        #ifdef WINDOWS
            //Wide functions, also without Unicode (UTF-8 text), for the characters which are not in the code page
            DWORD Path_Size=GetTempPathW(0, NULL);
            std::wstring Path(Path_Size, L'\0');
            if (!Path_Size || !(Path_Size=GetTempPathW(Path_Size, &Path[0])) || Path_Size>=Path.size())
                return Ztring(); //Problem while getting a temp path
            Path.resize(Path_Size);

            #ifndef WINDOWS_GAMES
                //Name as GetTempFileNameW() creates it, without its limit of the length of the directory: up to the first 3 characters of
                //the prefix, a hexadecimal number from the time (the next one if this name exists) and ".tmp", the file is created
                Ztring Name_Start=Ztring().From_Unicode(Path+Prefix.To_Unicode().substr(0, 3));
                int16u Unique=(int16u)GetTickCount();
                for (int32u Try=0; Try<0xFFFF; Try++, Unique++)
                {
                    if (!Unique)
                        Unique++;
                    Ztring Name=Name_Start+Ztring().From_Number(Unique, 16)+__T(".tmp");
                    File F;
                    if (F.Create(Name, false))
                        return Name;
                    if (!File::Exists(Name) && !Dir::Exists(Name))
                        break; //Problem while creating the file
                }
                return Ztring(); //Problem while getting a file name
            #else // #ifdef WINDOWS_GAMES
                return __T("D:\\xxx.txt");
            #endif // #ifdef WINDOWS_GAMES
        #else
            (void)Prefix;
            return __T("C:\\xxx.txt");
        #endif
    #endif //ZENLIB_USEWX
}

//***************************************************************************
// Platform differences
//***************************************************************************

//End of line
#ifdef __WINDOWS__
    const Char* FileName_PathSeparator=__T("\\");
#endif
#if defined(UNIX) || defined(MACOS) || defined(MACOSX)
    const Char* FileName_PathSeparator=__T("/");
#endif

} //namespace

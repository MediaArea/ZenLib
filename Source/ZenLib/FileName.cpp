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

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
// Position of the last path separator: "/", also "\" on Windows
// Without Unicode on Windows, in the bytes of the code page given as they are (see Ztring::To_Unicode()), the second byte of a character of
// a multibyte code page (e.g. "\" in cp932) is not a separator, UTF-8 text has no such byte
static size_t PathSeparator_RFind(const Ztring &Name)
{
    #if defined(WINDOWS) && !defined(UNICODE)
        #if defined(ZENLIB_FILENAME_CODEPAGE_BYTES)
            if (!IsCodePageBytes(Name.c_str(), Name.size()))
                return Name.find_last_of(__T("\\/"));
        #endif
        size_t ToReturn=Ztring::npos;
        for (size_t Pos=0; Pos<Name.size(); Pos++)
        {
            if (Name[Pos]=='\\' || Name[Pos]=='/')
                ToReturn=Pos;
            else if (IsDBCSLeadByte((BYTE)Name[Pos]))
                Pos++; //Second byte
        }
        return ToReturn;
    #elif defined(WINDOWS)
        return Name.find_last_of(__T("\\/"));
    #else
        return Name.rfind(__T('/'));
    #endif
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
            wchar_t Path[MAX_PATH+1];
            if (!GetTempPathW(MAX_PATH, Path))
                return Ztring(); //Problem while getting a temp path

            #ifndef WINDOWS_GAMES
                wchar_t FileName[MAX_PATH+1];
                if (!GetTempFileNameW(Path, Prefix.To_Unicode().c_str(), 0, FileName))
                    return Ztring(); //Problem while getting a file name

                return Ztring().From_Unicode(FileName);
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

/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef ZenOS_UtilsH
#define ZenOS_UtilsH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "ZenLib/Ztring.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#ifdef WINDOWS_UWP
#ifndef _HIDE_GLOBAL_ASYNC_STATUS
#define _HIDE_GLOBAL_ASYNC_STATUS //Else AsyncInfo.h declares a global Error (and Started, Completed, Canceled), ambiguous with ZenLib::Error with "using namespace ZenLib"
#endif
#include <wrl.h>
#include <windows.foundation.h>
#include <windows.storage.h>
#endif
//---------------------------------------------------------------------------

namespace ZenLib
{

//***************************************************************************
// OS Information
//***************************************************************************

//---------------------------------------------------------------------------
bool IsWin9X ();
// Execute
//***************************************************************************

void Shell_Execute(const Ztring &ToExecute);

//***************************************************************************
// File names
//***************************************************************************

// Names for the narrow functions of the C library or of the system (e.g. fopen(), the arguments of main()), in the encoding of the locale
// (the code page on Windows), or UTF-8 where the file names are UTF-8 (e.g. macOS). Without Unicode, the strings of ZenLib, also the names
// it gives (e.g. Dir::GetAllFileNames()), are UTF-8 text: a name for these functions, or to show in the encoding of the locale, must be
// converted with FileName_To_Native() or Ztring::To_Local(). FileName_From_Native(NULL) is an empty name, as Ztring::From_Local(NULL).
std::string FileName_To_Native(const Ztring &File_Name);
Ztring FileName_From_Native(const char* File_Name);

#if defined(WINDOWS) && !defined(WINDOWS_UWP) && !defined(ZENLIB_USEWX)
// Name for the wide functions of Windows (e.g. CreateFileW()), also without Unicode (UTF-8 text). If the process does not support long
// paths (Windows before 10 version 1607, LongPathsEnabled not set in the registry, or no longPathAware in the manifest of the program, e.g.
// a program using the library) and the name is too long for these functions (MAX_PATH characters including the null character, 12 less
// to create a directory), the full path with the extended-length prefix ("\\?\", "\\?\UNC\" for a network path), else the name as it is.
// The names given by ZenLib (e.g. Dir::GetAllFileNames()) are built from the names given to it, so with this prefix only if given with it.
std::wstring FileName_To_Wide(const Ztring &File_Name);
#endif

#if defined(WINDOWS) && !defined(WINDOWS_UWP) && !defined(UNICODE) && !defined(ZENLIB_USEWX)
// Without Unicode on Windows, name of an existing file or directory for the wide functions, as File and Dir open it: UTF-8 text, or the
// bytes of the code page given as they are (see Ztring::To_Unicode()), also when these bytes are valid UTF-8 but only the name in the code
// page exists. Ztring::From_Unicode() gives this name as UTF-8 text, as FileName_From_Native(FileName_To_Native()) on the other systems.
// This name is without the extended-length prefix of FileName_To_Wide().
std::wstring FileName_To_Wide_Existing(const Ztring &File_Name);
#endif

//***************************************************************************
// Directorues
//***************************************************************************

Ztring OpenFolder_Show(void* Handle, const Ztring &Title, const Ztring &Caption);

//***************************************************************************
// WinRT Helpers
//***************************************************************************
#ifdef WINDOWS_UWP
template <typename T>
inline HRESULT Await(const Microsoft::WRL::ComPtr<T> &Operation)
{
    if (!Operation)
        return E_POINTER;

    Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IAsyncInfo> Info;
    HRESULT Result=Operation.As(&Info);
    if (FAILED(Result))
        return Result;

    ABI::Windows::Foundation::AsyncStatus Status;
    while (SUCCEEDED(Result=Info->get_Status(&Status)) && Status==ABI::Windows::Foundation::AsyncStatus::Started)
        Sleep(0); //Yield

    if (FAILED(Result) || Status!=ABI::Windows::Foundation::AsyncStatus::Completed) {
        HRESULT Error_Code;
        Result=Info->get_ErrorCode(&Error_Code);
        if (FAILED(Result))
            return Result;
        return Error_Code;
    }

    return Result;
}

// Name for the WinRT functions, e.g. Get_File(WinRT_Name(File_Name).Ref(), File): in wide characters, also without Unicode (the UTF-8
// text converted, see Ztring::To_Unicode()). The reference given by Ref() is valid while this object exists, e.g. until the end of the
// expression which creates it
class WinRT_Name
{
public:
    explicit WinRT_Name(const Ztring &Name) : Wide(Name.To_Unicode()) {}
    Microsoft::WRL::Wrappers::HStringReference Ref() const {return Microsoft::WRL::Wrappers::HStringReference(Wide.c_str(), (unsigned int)Wide.size());}

private:
    const std::wstring Wide;
};

HRESULT Add_Item_To_FUA(Microsoft::WRL::Wrappers::HStringReference Path, Microsoft::WRL::ComPtr<ABI::Windows::Storage::IStorageItem> &Item);
HRESULT Get_File(Microsoft::WRL::Wrappers::HStringReference File_Name,  Microsoft::WRL::ComPtr<ABI::Windows::Storage::IStorageFile> &File);
HRESULT Get_Folder(Microsoft::WRL::Wrappers::HStringReference Folder_Name,  Microsoft::WRL::ComPtr<ABI::Windows::Storage::IStorageFolder> &Folder);
#endif

} //namespace ZenLib
#endif

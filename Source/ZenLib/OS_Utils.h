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
// converted with FileName_To_Native() or Ztring::To_Local()
std::string FileName_To_Native(const Ztring &File_Name);
Ztring FileName_From_Native(const char* File_Name);

#if defined(WINDOWS) && !defined(WINDOWS_UWP) && !defined(UNICODE) && !defined(ZENLIB_USEWX)
// Without Unicode on Windows, name of an existing file or directory for the wide functions, as File and Dir open it: UTF-8 text, or the
// bytes of the code page given as they are (see Ztring::To_Unicode()), also when these bytes are valid UTF-8 but only the name in the code
// page exists. Ztring::From_Unicode() gives this name as UTF-8 text, as FileName_From_Native(FileName_To_Native()) on the other systems.
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

HRESULT Add_Item_To_FUA(Microsoft::WRL::Wrappers::HStringReference Path, Microsoft::WRL::ComPtr<ABI::Windows::Storage::IStorageItem> &Item);
HRESULT Get_File(Microsoft::WRL::Wrappers::HStringReference File_Name,  Microsoft::WRL::ComPtr<ABI::Windows::Storage::IStorageFile> &File);
HRESULT Get_Folder(Microsoft::WRL::Wrappers::HStringReference Folder_Name,  Microsoft::WRL::ComPtr<ABI::Windows::Storage::IStorageFolder> &Folder);
#endif

} //namespace ZenLib
#endif

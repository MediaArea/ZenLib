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
#ifdef ZENLIB_USEWX
#else //ZENLIB_USEWX
    #ifdef WINDOWS
        #undef __TEXT
        #include <windows.h>
        #ifdef WINDOWS_UWP
            #include <wrl.h>
            #include <windows.foundation.h>
            #include <windows.storage.h>
            #include <windows.storage.accesscache.h>
            #include <windows.storage.streams.h>
            #include <windows.security.cryptography.h>
            #include <windows.security.cryptography.core.h>
            using namespace Microsoft::WRL;
            using namespace Microsoft::WRL::Wrappers;
            using namespace ABI::Windows::Foundation;
            using namespace ABI::Windows::Foundation::Collections;
            using namespace ABI::Windows::Storage;
            using namespace ABI::Windows::Storage::AccessCache;
            using namespace ABI::Windows::Storage::Streams;
            using namespace ABI::Windows::Security::Cryptography;
            using namespace ABI::Windows::Security::Cryptography::Core;
        #else
            #include <shlobj.h>
            #include <shellapi.h>
        #endif
    #endif
#endif //ZENLIB_USEWX
#include "ZenLib/OS_Utils.h"
#if !defined(UNICODE) && !defined(ZENLIB_USEWX) && !defined(WINDOWS) && defined(__APPLE__)
    #include <cstring>
    #include <iconv.h>
    #include <langinfo.h>
    #include <vector>
#endif
//---------------------------------------------------------------------------

namespace ZenLib
{

//***************************************************************************
// OS info
//***************************************************************************

//---------------------------------------------------------------------------
bool IsWin9X ()
{
    return false; //Hardcoded value because we don't support Win9x anymore
}

//***************************************************************************
// WinRT Helpers
//***************************************************************************

#if defined(WINDOWS) && defined(WINDOWS_UWP)
//---------------------------------------------------------------------------
HRESULT Add_Item_To_FUA(HStringReference Path, ComPtr<IStorageItem> &Item)
{
    ComPtr<IBuffer> Path_Buffer;
    ComPtr<IBuffer> Hash_Buffer;
    ComPtr<ICryptographicBufferStatics> Cryptographic_Buffer_Statics;
    ComPtr<IHashAlgorithmProviderStatics> Hash_Provider_Statics;
    ComPtr<IHashAlgorithmProvider> Hash_Provider;
    UINT32 Buffer_Lenght = 0;
    UINT32 Hash_Lenght = 0;
    HString Path_Hash;

    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer).Get(), &Cryptographic_Buffer_Statics)) &&
        Cryptographic_Buffer_Statics &&
        SUCCEEDED(Cryptographic_Buffer_Statics->ConvertStringToBinary(Path.Get(), BinaryStringEncoding_Utf16LE, &Path_Buffer)) &&
        Path_Buffer &&
        SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Core_HashAlgorithmProvider).Get(), &Hash_Provider_Statics)) &&
        Hash_Provider_Statics &&
        SUCCEEDED(Hash_Provider_Statics->OpenAlgorithm(HString::MakeReference(L"SHA256").Get(), &Hash_Provider)) &&
        Hash_Provider &&
        SUCCEEDED(Hash_Provider->HashData(*Path_Buffer.GetAddressOf(), &Hash_Buffer)) &&
        Hash_Buffer &&
        SUCCEEDED(Hash_Buffer->get_Length(&Buffer_Lenght)) &&
        Buffer_Lenght &&
        SUCCEEDED(Hash_Provider->get_HashLength(&Hash_Lenght)) &&
        Hash_Lenght &&
        Buffer_Lenght == Hash_Lenght &&
        SUCCEEDED(Cryptographic_Buffer_Statics->EncodeToBase64String(*Hash_Buffer.GetAddressOf(), Path_Hash.GetAddressOf())))
    {
        ComPtr<IStorageApplicationPermissionsStatics> Permissions_Manager;
        ComPtr<IStorageItemAccessList> FutureAccess_List;
        if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_AccessCache_StorageApplicationPermissions).Get(), &Permissions_Manager)) &&
            Permissions_Manager &&
            SUCCEEDED(Permissions_Manager->get_FutureAccessList(&FutureAccess_List)) &&
            FutureAccess_List)
            return FutureAccess_List->AddOrReplace(Path_Hash.Get(), *Item.GetAddressOf(), HString::MakeReference(L"").Get());
    }

    return E_FAIL;
}

//---------------------------------------------------------------------------
HRESULT Get_File(HStringReference File_Name, ComPtr<IStorageFile> &File)
{
    //Try to access file directly
    ComPtr<IStorageFileStatics> Storage;
    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_StorageFile).Get(), &Storage)) && Storage)
    {
        ComPtr<IAsyncOperation<StorageFile*> > Async_GetFile;
        if (SUCCEEDED(Storage->GetFileFromPathAsync(File_Name.Get(), &Async_GetFile)) &&
            SUCCEEDED(Await(Async_GetFile)) &&
            SUCCEEDED(Async_GetFile->GetResults(&File)) &&
            File)
            return S_OK;
    }

    //Try to access file by sha256 hashed path in future access list
    ComPtr<IBuffer> Path_Buffer;
    ComPtr<IBuffer> Hash_Buffer;
    ComPtr<ICryptographicBufferStatics> Cryptographic_Buffer_Statics;
    ComPtr<IHashAlgorithmProviderStatics> Hash_Provider_Statics;
    ComPtr<IHashAlgorithmProvider> Hash_Provider;
    UINT32 Buffer_Lenght=0;
    UINT32 Hash_Lenght=0;
    HString Path_Hash;

    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer).Get(), &Cryptographic_Buffer_Statics)) &&
        Cryptographic_Buffer_Statics &&
        SUCCEEDED(Cryptographic_Buffer_Statics->ConvertStringToBinary(File_Name.Get(), BinaryStringEncoding_Utf16LE, &Path_Buffer)) &&
        Path_Buffer &&
        SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Core_HashAlgorithmProvider).Get(), &Hash_Provider_Statics)) &&
        Hash_Provider_Statics &&
        SUCCEEDED(Hash_Provider_Statics->OpenAlgorithm(HString::MakeReference(L"SHA256").Get(), &Hash_Provider)) &&
        Hash_Provider &&
        SUCCEEDED(Hash_Provider->HashData(*Path_Buffer.GetAddressOf(), &Hash_Buffer)) &&
        Hash_Buffer &&
        SUCCEEDED(Hash_Buffer->get_Length(&Buffer_Lenght)) &&
        Buffer_Lenght &&
        SUCCEEDED(Hash_Provider->get_HashLength(&Hash_Lenght)) &&
        Hash_Lenght &&
        Buffer_Lenght == Hash_Lenght &&
        SUCCEEDED(Cryptographic_Buffer_Statics->EncodeToBase64String(*Hash_Buffer.GetAddressOf(), Path_Hash.GetAddressOf())))
    {
        ComPtr<IStorageApplicationPermissionsStatics> Permissions_Manager;
        if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_AccessCache_StorageApplicationPermissions).Get(), &Permissions_Manager)) && Permissions_Manager)
        {
            ComPtr<IStorageItemAccessList> FutureAccess_List;
            ComPtr<IAsyncOperation<StorageFile*> > Async_GetFile_FutureAccess;
            if (SUCCEEDED(Permissions_Manager->get_FutureAccessList(&FutureAccess_List)) &&
                FutureAccess_List &&
                SUCCEEDED(FutureAccess_List->GetFileAsync(Path_Hash.Get(), &Async_GetFile_FutureAccess)) &&
                Await(Async_GetFile_FutureAccess) &&
                SUCCEEDED(Async_GetFile_FutureAccess->GetResults(&File)) &&
                File)
                return S_OK;
        }
    }

    return E_FAIL;
}

//---------------------------------------------------------------------------
HRESULT Get_Folder(HStringReference Folder_Name, ComPtr<IStorageFolder> &Folder)
{
    //Try to access folder directly
    ComPtr<IStorageFolderStatics> Storage;
    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_StorageFolder).Get(), &Storage)) && Storage)
    {
        ComPtr<IAsyncOperation<StorageFolder*> > Async_GetFolder;
        if (SUCCEEDED(Storage->GetFolderFromPathAsync(Folder_Name.Get(), &Async_GetFolder)) &&
            SUCCEEDED(Await(Async_GetFolder)) &&
            SUCCEEDED(Async_GetFolder->GetResults(&Folder)) &&
            Folder)
            return S_OK;
    }

    //Try to access folder by sha256 hashed path in future access list
    ComPtr<IBuffer> Path_Buffer;
    ComPtr<IBuffer> Hash_Buffer;
    ComPtr<ICryptographicBufferStatics> Cryptographic_Buffer_Statics;
    ComPtr<IHashAlgorithmProviderStatics> Hash_Provider_Statics;
    ComPtr<IHashAlgorithmProvider> Hash_Provider;
    UINT32 Buffer_Lenght = 0;
    UINT32 Hash_Lenght = 0;
    HString Path_Hash;

    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer).Get(), &Cryptographic_Buffer_Statics)) &&
        Cryptographic_Buffer_Statics &&
        SUCCEEDED(Cryptographic_Buffer_Statics->ConvertStringToBinary(Folder_Name.Get(), BinaryStringEncoding_Utf16LE, &Path_Buffer)) &&
        Path_Buffer &&
        SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Core_HashAlgorithmProvider).Get(), &Hash_Provider_Statics)) &&
        Hash_Provider_Statics &&
        SUCCEEDED(Hash_Provider_Statics->OpenAlgorithm(HString::MakeReference(L"SHA256").Get(), &Hash_Provider)) &&
        Hash_Provider &&
        SUCCEEDED(Hash_Provider->HashData(*Path_Buffer.GetAddressOf(), &Hash_Buffer)) &&
        Hash_Buffer &&
        SUCCEEDED(Hash_Buffer->get_Length(&Buffer_Lenght)) &&
        Buffer_Lenght &&
        SUCCEEDED(Hash_Provider->get_HashLength(&Hash_Lenght)) &&
        Hash_Lenght &&
        Buffer_Lenght == Hash_Lenght &&
        SUCCEEDED(Cryptographic_Buffer_Statics->EncodeToBase64String(*Hash_Buffer.GetAddressOf(), Path_Hash.GetAddressOf())))
    {
        ComPtr<IStorageApplicationPermissionsStatics> Permissions_Manager;
        if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_AccessCache_StorageApplicationPermissions).Get(), &Permissions_Manager)) && Permissions_Manager)
        {
            ComPtr<IStorageItemAccessList> FutureAccess_List;
            ComPtr<IAsyncOperation<StorageFolder*> > Async_GetFolder_FutureAccess;
            if (SUCCEEDED(Permissions_Manager->get_FutureAccessList(&FutureAccess_List)) &&
                FutureAccess_List &&
                SUCCEEDED(FutureAccess_List->GetFolderAsync(Folder_Name.Get(), &Async_GetFolder_FutureAccess)) &&
                Await(Async_GetFolder_FutureAccess) &&
                SUCCEEDED(Async_GetFolder_FutureAccess->GetResults(&Folder)) &&
                Folder)
                return S_OK;
        }
    }

    return E_FAIL;
}
#endif

//***************************************************************************
// Shell
//***************************************************************************

void Shell_Execute(const Ztring &ToExecute)
{
    #ifdef ZENLIB_USEWX
    #else //ZENLIB_USEWX
        #if defined(WINDOWS) && !defined(WINDOWS_UWP) && !defined(WINDOWS_GAMES)
            ShellExecute(NULL, __T("open"), ToExecute.c_str(), NULL, NULL, 0);
        #else
            //Not supported
            (void)ToExecute;
        #endif
    #endif //ZENLIB_USEWX
}

//***************************************************************************
// File names
//***************************************************************************

//---------------------------------------------------------------------------
//File names are UTF-8 on macOS and Android, whatever the locale. On other
//Unix-like systems, they use the locale encoding, which is UTF-8 when the
//locale is ASCII (see Ztring::To_Local()).
//Without Unicode, strings are the bytes of the locale encoding: on macOS,
//where file names are UTF-8, they are converted with iconv when the locale
//is neither ASCII nor UTF-8 (Android has no other locale).
#if defined(UNICODE) && !defined(ZENLIB_USEWX) && !defined(WINDOWS) && (defined(__APPLE__) || defined(__ANDROID__))
    #define ZENLIB_FILENAME_UTF8
#endif
#if !defined(UNICODE) && !defined(ZENLIB_USEWX) && !defined(WINDOWS) && defined(__APPLE__)
    #define ZENLIB_FILENAME_UTF8_NARROW
#endif

#ifdef ZENLIB_FILENAME_UTF8_NARROW
//---------------------------------------------------------------------------
// Encoding of the locale if it is neither ASCII nor UTF-8, else NULL
static const char* FileName_Locale_Other()
{
    const char* CodeSet=nl_langinfo(CODESET);
    if (!CodeSet || !*CodeSet
     || !strcmp(CodeSet, "US-ASCII") || !strcmp(CodeSet, "ASCII") || !strcmp(CodeSet, "ANSI_X3.4-1968") || !strcmp(CodeSet, "646")
     || !strcmp(CodeSet, "UTF-8") || !strcmp(CodeSet, "utf8"))
        return NULL;
    return CodeSet;
}

//---------------------------------------------------------------------------
// Valid UTF-8 (no overlong sequence, no surrogate)
static bool FileName_IsUTF8(const char* S)
{
    for (const unsigned char* P=(const unsigned char*)S; *P;)
    {
        unsigned long Code;
        int More;
        if (*P<0x80)
            { Code=*P; More=0; }
        else if ((*P&0xE0)==0xC0)
            { Code=*P&0x1F; More=1; }
        else if ((*P&0xF0)==0xE0)
            { Code=*P&0x0F; More=2; }
        else if ((*P&0xF8)==0xF0)
            { Code=*P&0x07; More=3; }
        else
            return false;
        P++;
        for (int i=0; i<More; i++, P++)
        {
            if ((*P&0xC0)!=0x80)
                return false;
            Code=(Code<<6)|(*P&0x3F);
        }
        if ((More==1 && Code<0x80) || (More==2 && Code<0x800) || (More==3 && Code<0x10000) || Code>0x10FFFF || (Code>=0xD800 && Code<0xE000))
            return false;
    }
    return true;
}

//---------------------------------------------------------------------------
// Name of an encoding of the locales for iconv, which does not know all of them, e.g. "eucJP" with macOS 14 and more
static std::string FileName_Iconv_Name(const char* CodeSet)
{
    static const char* const Names[][2]=
    {
        {"eucJP", "EUC-JP"},
        {"eucKR", "EUC-KR"},
        {"eucCN", "EUC-CN"},
        {"eucTW", "EUC-TW"},
        {"SJIS", "SHIFT_JIS"},
        {"Big5HKSCS", "BIG5-HKSCS"},
    };
    for (size_t i=0; i<sizeof(Names)/sizeof(*Names); i++)
        if (!strcmp(CodeSet, Names[i][0]))
            return Names[i][1];
    if (!strncmp(CodeSet, "ISO8859-", 8))
        return std::string("ISO-8859-")+(CodeSet+8);
    return CodeSet;
}

//---------------------------------------------------------------------------
static iconv_t FileName_Iconv_Open(const char* To, const char* From)
{
    iconv_t Converter=iconv_open(To, From);
    if (Converter==(iconv_t)-1)
        Converter=iconv_open(FileName_Iconv_Name(To).c_str(), FileName_Iconv_Name(From).c_str());
    return Converter;
}

//---------------------------------------------------------------------------
// Conversion with iconv (all the bytes, without transliteration), empty if a character can't be converted
static std::string FileName_Convert(const char* From, const char* To, const char* S)
{
    iconv_t Converter=FileName_Iconv_Open(To, From);
    if (Converter==(iconv_t)-1)
        return std::string();
    size_t In_Size=strlen(S);
    std::vector<char> Out(In_Size*4+16); //UTF-8 and the locale encodings use at most 4 bytes per character, and shift sequences
    char* In=(char*)S;
    char* Out_Pos=&Out[0];
    size_t Out_Size=Out.size();
    bool OK=iconv(Converter, &In, &In_Size, &Out_Pos, &Out_Size)!=(size_t)-1 && iconv(Converter, NULL, NULL, &Out_Pos, &Out_Size)!=(size_t)-1;
    iconv_close(Converter);
    if (!OK || In_Size)
        return std::string();
    return std::string(&Out[0], Out_Pos-&Out[0]);
}
#endif //ZENLIB_FILENAME_UTF8_NARROW

//---------------------------------------------------------------------------
std::string FileName_To_Native(const Ztring &File_Name)
{
    #ifdef ZENLIB_FILENAME_UTF8
        for (size_t Pos=0; Pos<File_Name.size(); Pos++)
            if (File_Name[Pos]>=0xDC80 && File_Name[Pos]<=0xDCFF)
                return File_Name.To_Local(); //Bytes of a name which is not valid UTF-8 (see Ztring::To_Local())
        return File_Name.To_UTF8();
    #elif defined(ZENLIB_FILENAME_UTF8_NARROW)
        //UTF-8 kept (e.g. a path from the Finder), else converted from the locale encoding (e.g. typed in a terminal using it)
        const char* CodeSet=FileName_Locale_Other();
        if (!CodeSet || FileName_IsUTF8(File_Name.c_str()))
            return File_Name;
        std::string ToReturn=FileName_Convert(CodeSet, "UTF-8", File_Name.c_str());
        return ToReturn.empty()?std::string(File_Name):ToReturn;
    #else
        return File_Name.To_Local();
    #endif
}

//---------------------------------------------------------------------------
Ztring FileName_From_Native(const char* File_Name)
{
    #ifdef ZENLIB_FILENAME_UTF8
        Ztring ToReturn;
        ToReturn.From_UTF8(File_Name);
        if (ToReturn.empty() || FileName_To_Native(ToReturn)!=File_Name)
            ToReturn.From_Local(File_Name); //Not UTF-8, e.g. typed in a terminal using the locale encoding
        return ToReturn;
    #elif defined(ZENLIB_FILENAME_UTF8_NARROW)
        //In the locale encoding if it can encode it and gives back the same name, else the UTF-8 bytes
        const char* CodeSet=FileName_Locale_Other();
        if (!CodeSet)
            return Ztring(File_Name);
        std::string ToReturn=FileName_Convert("UTF-8", CodeSet, File_Name);
        if (ToReturn.empty() || FileName_To_Native(ToReturn)!=File_Name)
            return Ztring(File_Name);
        return Ztring(ToReturn);
    #else
        return Ztring().From_Local(File_Name);
    #endif
}

//***************************************************************************
// Directories
//***************************************************************************

//---------------------------------------------------------------------------
// Select directory code
// Extracted from TBffolder by Torsten Johann (t-johann@gmx.de)

Ztring Directory_Select_Caption;

#if defined(WINDOWS) && !defined(WINDOWS_UWP) && !defined(WINDOWS_GAMES)
    #ifdef UNICODE
        char    InitDirA[MAX_PATH];
        wchar_t InitDir [MAX_PATH];

        int __stdcall ShowOpenFolder_CallbackProc (HWND hwnd, UINT uMsg, LPARAM, LPARAM)
        {
            if (uMsg==BFFM_INITIALIZED)
            {
                SetWindowText(hwnd, Directory_Select_Caption.c_str());    // Caption
                SendMessage  (hwnd, BFFM_ENABLEOK, 0, TRUE);
                SendMessage  (hwnd, BFFM_SETSELECTION, true, (LPARAM)&InitDir);
            }
            return 0;
        }

        Ztring OpenFolder_Show(void* Handle, const Ztring &Title, const Ztring &Caption)
        {
            //Caption
            Directory_Select_Caption=Caption;

            //Values
            LPMALLOC        Malloc;
            LPSHELLFOLDER   ShellFolder;
            BROWSEINFO      BrowseInfo;
            LPITEMIDLIST    ItemIdList;

            //Initializing the SHBrowseForFolder function
            if (SHGetMalloc(&Malloc)!=NOERROR)
                return Ztring();
            if (SHGetDesktopFolder(&ShellFolder)!=NOERROR)
                return Ztring();
            ZeroMemory(&BrowseInfo, sizeof(BROWSEINFOW));
            BrowseInfo.ulFlags+=BIF_RETURNONLYFSDIRS;
            BrowseInfo.hwndOwner=(HWND)Handle;
            BrowseInfo.pszDisplayName=InitDir;
            BrowseInfo.lpszTitle=Title.c_str();
            BrowseInfo.lpfn=ShowOpenFolder_CallbackProc;

            //Displaying
            ItemIdList=SHBrowseForFolder(&BrowseInfo);

            //Releasing
            ShellFolder->Release();
            if (ItemIdList!=NULL)
            {
                SHGetPathFromIDList(ItemIdList, InitDir);
                Malloc->Free(ItemIdList);
                Malloc->Release();

                //The value
                return InitDir;
            }
            else
                return Ztring();
        }

    #else
        char InitDirA[MAX_PATH];

        int __stdcall ShowOpenFolder_CallbackProc (HWND hwnd, UINT uMsg, LPARAM, LPARAM)
        {
            if (uMsg==BFFM_INITIALIZED)
            {
                SetWindowText (hwnd, Directory_Select_Caption.c_str());    // Caption
                SendMessage   (hwnd, BFFM_ENABLEOK, 0, TRUE);
                SendMessage   (hwnd, BFFM_SETSELECTION, true, (LPARAM)&InitDirA);
            }
            return 0;
        }

        Ztring OpenFolder_Show(void* Handle, const Ztring &Title, const Ztring &Caption)
        {
            //Caption
            Directory_Select_Caption=Caption;

            //Values
            LPMALLOC        Malloc;
            LPSHELLFOLDER   ShellFolder;
            BROWSEINFO      BrowseInfo;
            LPITEMIDLIST    ItemIdList;

            //Initializing the SHBrowseForFolder function
            if (SHGetMalloc(&Malloc)!=NOERROR)
                return Ztring();
            if (SHGetDesktopFolder(&ShellFolder)!=NOERROR)
                return Ztring();
            ZeroMemory(&BrowseInfo, sizeof(BROWSEINFO));
            BrowseInfo.ulFlags+=BIF_RETURNONLYFSDIRS;
            BrowseInfo.hwndOwner=(HWND)Handle;
            BrowseInfo.pszDisplayName=InitDirA;
            BrowseInfo.lpszTitle=Title.c_str();
            BrowseInfo.lpfn=ShowOpenFolder_CallbackProc;

            //Displaying
            ItemIdList=SHBrowseForFolder(&BrowseInfo);

            //Releasing
            ShellFolder->Release();
            if (ItemIdList!=NULL)
            {
                SHGetPathFromIDList(ItemIdList, InitDirA);
                Malloc->Free(ItemIdList);
                Malloc->Release();

                //The value
                return InitDirA;
            }
            else
                return Ztring();
        }
    #endif //UNICODE
#endif //WINDOWS

} //namespace ZenLib

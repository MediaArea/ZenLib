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
//File names are UTF-8 whatever the locale on macOS and Android, also on other systems if ZENLIB_FILENAME_UTF8 is defined (e.g. tests)
#if !defined(ZENLIB_USEWX) && !defined(WINDOWS) && !defined(WSTRING_MISSING)
    #if defined(__APPLE__) || defined(__ANDROID__) || defined(ZENLIB_FILENAME_UTF8)
        #define ZENLIB_FILENAME_UTF8_SYSTEM
        #if !defined(__ANDROID__)
            #define ZENLIB_FILENAME_EXISTING //Volumes which keep the bytes of names created with another encoding, e.g. NFS or AFS
        #endif
    #elif !defined(UNICODE)
        #define ZENLIB_FILENAME_EXISTING //Without Unicode, also the bytes of the string as they are given (see FileName_To_Native())
    #endif
    #if defined(ZENLIB_FILENAME_EXISTING)
        #include <algorithm>
        #include <cstring>
        #include <sys/stat.h>
        #include <vector>
    #endif
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
            ShellExecuteW(NULL, L"open", ToExecute.To_Unicode().c_str(), NULL, NULL, 0); //Also without Unicode (UTF-8 text)
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
//File names are UTF-8 on macOS and Android, whatever the locale, and the
//bytes which are not valid UTF-8 are kept (see Ztring::From_Local()). On other
//Unix-like systems, they use the encoding of the locale, which is UTF-8 when
//the locale is ASCII, and the bytes which are not valid in it are kept.
//Without Unicode, the names are UTF-8 text as the other strings, converted
//the same way (with the bytes kept as Ztring::To_Unicode() decodes them).
#if defined(ZENLIB_FILENAME_UTF8_SYSTEM)
//---------------------------------------------------------------------------
//In Ztring.cpp
ZENLIB_INTERNAL const char* Local_Other();
ZENLIB_INTERNAL void From_UTF8_KeepBytes(std::wstring& ToReturn, const char* S);
ZENLIB_INTERNAL std::string To_UTF8_KeepBytes(const std::wstring& S);

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
#endif

#if defined(ZENLIB_FILENAME_EXISTING)
#if !defined(ZENLIB_FILENAME_UTF8_SYSTEM)
//---------------------------------------------------------------------------
//In Ztring.cpp
ZENLIB_INTERNAL const char* Local_Other();
#endif

//---------------------------------------------------------------------------
static bool FileName_IsASCII(const std::string& S)
{
    for (size_t Pos=0; Pos<S.size(); Pos++)
        if ((unsigned char)S[Pos]>=0x80)
            return false;
    return true;
}

//---------------------------------------------------------------------------
//APFS and HFS+ accept only UTF-8 names, but network volumes (e.g. NFS, AFS,
//SMB) keep the bytes of names created with another encoding. With a locale
//which is neither ASCII nor UTF-8, a path, or a part of it, may exist only
//with the bytes of this encoding, e.g. typed in a terminal using it: the first
//name is used if it exists, else for each part of the path the first name
//which exists, else the part of the first name (e.g. a new file)
//Without Unicode on the other systems, the bytes of the string as they are
//given are the other name (see FileName_To_Native())
static bool FileName_Exists(const std::string& Name)
{
    struct stat Stat;
    return !lstat(Name.c_str(), &Stat);
}

static std::string FileName_Existing(std::vector<std::string> Names)
{
    //Other names with the same parts (the separator is ASCII)
    for (size_t i=1; i<Names.size(); i++)
        if (Names[i]==Names[0] || std::count(Names[i].begin(), Names[i].end(), '/')!=std::count(Names[0].begin(), Names[0].end(), '/'))
            Names.erase(Names.begin()+i--);
    if (Names.size()==1 || FileName_Exists(Names[0]))
        return Names[0];

    std::vector<size_t> Starts(Names.size(), 0);
    std::string ToReturn;
    for (;;)
    {
        std::vector<std::string> Parts(Names.size());
        for (size_t i=0; i<Names.size(); i++)
        {
            size_t End=Names[i].find('/', Starts[i]);
            Parts[i]=Names[i].substr(Starts[i], End==std::string::npos?std::string::npos:End-Starts[i]);
            Starts[i]=End==std::string::npos?End:End+1;
        }
        size_t Chosen=0;
        if (!FileName_Exists(ToReturn+Parts[0]))
            for (size_t i=1; i<Parts.size(); i++)
                if (Parts[i]!=Parts[0] && FileName_Exists(ToReturn+Parts[i]))
                {
                    Chosen=i;
                    break;
                }
        ToReturn+=Parts[Chosen];
        if (Starts[0]==std::string::npos)
            return ToReturn;
        ToReturn+='/';
    }
}
#endif //ZENLIB_FILENAME_EXISTING

//---------------------------------------------------------------------------
std::string FileName_To_Native(const Ztring &File_Name)
{
    #if defined(ZENLIB_FILENAME_UTF8_SYSTEM)
        #ifdef UNICODE
            const std::wstring& Name=File_Name;
        #else
            std::wstring Name=File_Name.To_Unicode();
        #endif
        std::string ToReturn=To_UTF8_KeepBytes(Name);
        #if defined(ZENLIB_FILENAME_EXISTING)
            if (!FileName_IsASCII(ToReturn) && Local_Other())
            {
                //Also in the encoding of the locale, the name or its UTF-8 bytes (e.g. typed in a terminal using it, see FileName_From_Native())
                std::vector<std::string> Names;
                Names.push_back(ToReturn);
                #ifdef UNICODE
                    Names.push_back(To_UTF8_KeepBytes(Ztring().From_Local(ToReturn.c_str())));
                #else
                    Names.push_back(To_UTF8_KeepBytes(Ztring().From_Local(ToReturn.c_str()).To_Unicode()));
                #endif
                Names.push_back(File_Name.To_Local());
                ToReturn=FileName_Existing(Names);
            }
        #endif
        return ToReturn;
    #elif defined(ZENLIB_FILENAME_EXISTING)
        //Also the bytes of the string as they are given, if only this name exists: bytes of the encoding of the locale put in a Ztring without
        //From_Local() which are also valid UTF-8, so read as UTF-8 (the other bytes are kept, they give the same bytes back)
        std::string ToReturn=File_Name.To_Local();
        if (ToReturn!=File_Name && !FileName_IsASCII(File_Name) && Local_Other())
        {
            std::vector<std::string> Names;
            Names.push_back(ToReturn);
            Names.push_back(File_Name);
            ToReturn=FileName_Existing(Names);
        }
        return ToReturn;
    #else
        return File_Name.To_Local();
    #endif
}

//---------------------------------------------------------------------------
Ztring FileName_From_Native(const char* File_Name)
{
    #if defined(ZENLIB_FILENAME_UTF8_SYSTEM)
        //UTF-8, else with a locale which is neither ASCII nor UTF-8 its encoding (e.g. typed in a terminal using it), bytes which are not valid kept
        if (!FileName_IsUTF8(File_Name) && Local_Other())
            return Ztring().From_Local(File_Name);
        std::wstring ToReturn;
        From_UTF8_KeepBytes(ToReturn, File_Name);
        #ifdef UNICODE
            return ToReturn;
        #else
            return Ztring().From_Unicode(ToReturn);
        #endif
    #else
        return Ztring().From_Local(File_Name);
    #endif
}

#if defined(WINDOWS) && !defined(WINDOWS_UWP) && !defined(UNICODE) && !defined(ZENLIB_USEWX)
//---------------------------------------------------------------------------
//In Ztring.cpp
ZENLIB_INTERNAL bool IsCodePageBytes(const char* S, size_t Size);

//---------------------------------------------------------------------------
//Name of an existing file or directory for the wide functions, without Unicode: UTF-8 text, or the bytes of the code page given as they are
//(see Ztring::To_Unicode()), also when these bytes are valid UTF-8 but only the name in the code page exists
ZENLIB_INTERNAL std::wstring FileName_To_Wide_Existing(const Ztring &File_Name)
{
    std::wstring Name=File_Name.To_Unicode();
    size_t ASCII_Size=0;
    while (ASCII_Size<File_Name.size() && !((int8u)File_Name[ASCII_Size]&0x80))
        ASCII_Size++;
    if (ASCII_Size==File_Name.size() || IsCodePageBytes(File_Name.c_str(), File_Name.size()) || GetFileAttributesW(Name.c_str())!=INVALID_FILE_ATTRIBUTES)
        return Name;
    std::wstring Name_CodePage=Ztring().From_Local(File_Name.c_str()).To_Unicode();
    if (Name_CodePage!=Name && GetFileAttributesW(Name_CodePage.c_str())!=INVALID_FILE_ATTRIBUTES)
        return Name_CodePage;
    return Name;
}
#endif

//***************************************************************************
// Directories
//***************************************************************************

//---------------------------------------------------------------------------
// Select directory code
// Extracted from TBffolder by Torsten Johann (t-johann@gmx.de)

Ztring Directory_Select_Caption;

#if defined(WINDOWS) && !defined(WINDOWS_UWP) && !defined(WINDOWS_GAMES)
    //Wide functions, also without Unicode (UTF-8 text), for the characters which are not in the code page
    wchar_t InitDir[MAX_PATH];

    int __stdcall ShowOpenFolder_CallbackProc (HWND hwnd, UINT uMsg, LPARAM, LPARAM)
    {
        if (uMsg==BFFM_INITIALIZED)
        {
            SetWindowTextW(hwnd, Directory_Select_Caption.To_Unicode().c_str());    // Caption
            SendMessageW  (hwnd, BFFM_ENABLEOK, 0, TRUE);
            SendMessageW  (hwnd, BFFM_SETSELECTIONW, true, (LPARAM)&InitDir);
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
        BROWSEINFOW     BrowseInfo;
        LPITEMIDLIST    ItemIdList;
        std::wstring    Title_Wide=Title.To_Unicode();

        //Initializing the SHBrowseForFolder function
        if (SHGetMalloc(&Malloc)!=NOERROR)
            return Ztring();
        if (SHGetDesktopFolder(&ShellFolder)!=NOERROR)
            return Ztring();
        ZeroMemory(&BrowseInfo, sizeof(BROWSEINFOW));
        BrowseInfo.ulFlags+=BIF_RETURNONLYFSDIRS;
        BrowseInfo.hwndOwner=(HWND)Handle;
        BrowseInfo.pszDisplayName=InitDir;
        BrowseInfo.lpszTitle=Title_Wide.c_str();
        BrowseInfo.lpfn=ShowOpenFolder_CallbackProc;

        //Displaying
        ItemIdList=SHBrowseForFolderW(&BrowseInfo);

        //Releasing
        ShellFolder->Release();
        if (ItemIdList!=NULL)
        {
            SHGetPathFromIDListW(ItemIdList, InitDir);
            Malloc->Free(ItemIdList);
            Malloc->Release();

            //The value
            return Ztring().From_Unicode(InitDir);
        }
        else
            return Ztring();
    }
#endif //WINDOWS

} //namespace ZenLib

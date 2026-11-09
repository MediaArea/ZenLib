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
    #include <wx/file.h>
    #include <wx/filename.h>
    #include <wx/utils.h>
    #include <wx/dir.h>
#else //ZENLIB_USEWX
    #ifdef ZENLIB_STANDARD
        #undef WINDOWS
    #endif
    #ifdef WINDOWS
        #undef __TEXT
        #include <windows.h>
        #ifdef WINDOWS_UWP
            #include <wrl.h>
            #include <windows.foundation.h>
            #include <windows.storage.h>
            #include <windows.storage.accesscache.h>
            using namespace Microsoft::WRL;
            using namespace Microsoft::WRL::Wrappers;
            using namespace ABI::Windows::Foundation;
            using namespace ABI::Windows::Foundation::Collections;
            using namespace ABI::Windows::Storage;
        #endif
    #else
        #include <sys/stat.h>
        #include <sys/types.h>
        #undef __TEXT //dirent include windows.h on Windows/Borland
        #include <dirent.h>
        #include <glob.h>
    #endif
#endif //ZENLIB_USEWX
#include "ZenLib/Dir.h"
#include "ZenLib/File.h"
#include "ZenLib/FileName.h"
#include "ZenLib/OS_Utils.h"
//---------------------------------------------------------------------------

namespace ZenLib
{

//---------------------------------------------------------------------------
//Names for the wide functions of Windows, also without Unicode (UTF-8 text)
#if defined(WINDOWS) && !defined(WINDOWS_UWP) && !defined(ZENLIB_USEWX)
    #ifdef UNICODE
        static inline const std::wstring& FileName_To_Wide(const Ztring &File_Name) {return File_Name;}
        static inline const std::wstring& FileName_To_Wide_Existing(const Ztring &File_Name) {return File_Name;}
        static inline Ztring FileName_From_Wide(const wchar_t* File_Name) {return File_Name;}
    #else //UNICODE
        static inline std::wstring FileName_To_Wide(const Ztring &File_Name) {return File_Name.To_Unicode();}
        static inline Ztring FileName_From_Wide(const wchar_t* File_Name) {return Ztring().From_Unicode(File_Name);}
        ZENLIB_INTERNAL bool IsCodePageBytes(const char* S, size_t Size); //In Ztring.cpp
    #endif //UNICODE
#endif

//---------------------------------------------------------------------------
// Pattern of the content of a directory, "\" and "/" are path separators on Windows
// Without Unicode, in the bytes of the code page given as they are (see Ztring::To_Unicode()), the second byte of a character of a multibyte
// code page (e.g. "\" in cp932) is not a separator, UTF-8 text has no such byte
#if defined(WINDOWS) && !defined(WINDOWS_UWP)
static Ztring Dir_Pattern(const Ztring &Dir_Name)
{
    #if !defined(UNICODE)
        #if !defined(ZENLIB_USEWX)
            bool CodePageBytes=IsCodePageBytes(Dir_Name.c_str(), Dir_Name.size());
        #else //ZENLIB_USEWX
            bool CodePageBytes=true;
        #endif //ZENLIB_USEWX
    #endif
    bool EndsWithSeparator=false;
    for (size_t Pos=0; Pos<Dir_Name.size(); Pos++)
    {
        EndsWithSeparator=Dir_Name[Pos]==__T('\\') || Dir_Name[Pos]==__T('/');
        #if !defined(UNICODE)
            if (CodePageBytes && IsDBCSLeadByte((BYTE)Dir_Name[Pos]))
                Pos++; //Second byte
        #endif
    }
    return Dir_Name+(EndsWithSeparator?__T("*"):__T("\\*"));
}
#endif

//---------------------------------------------------------------------------
// Debug
#ifdef ZENLIB_DEBUG
    #include <stdio.h>
    #include <windows.h>
    namespace ZenLib_Dir_Debug
    {
        FILE* F;
        std::string Debug;
        SYSTEMTIME st_In;

        void Debug_Open(bool Out)
        {
            F=fopen("C:\\Temp\\ZenLib_Debug.txt", "a+t");
            Debug.clear();
            SYSTEMTIME st;
            GetLocalTime( &st );

            char Duration[100];
            if (Out)
            {
                FILETIME ft_In;
                if (SystemTimeToFileTime(&st_In, &ft_In))
                {
                    FILETIME ft_Out;
                    if (SystemTimeToFileTime(&st, &ft_Out))
                    {
                        ULARGE_INTEGER UI_In;
                        UI_In.HighPart=ft_In.dwHighDateTime;
                        UI_In.LowPart=ft_In.dwLowDateTime;

                        ULARGE_INTEGER UI_Out;
                        UI_Out.HighPart=ft_Out.dwHighDateTime;
                        UI_Out.LowPart=ft_Out.dwLowDateTime;

                        ULARGE_INTEGER UI_Diff;
                        UI_Diff.QuadPart=UI_Out.QuadPart-UI_In.QuadPart;

                        FILETIME ft_Diff;
                        ft_Diff.dwHighDateTime=UI_Diff.HighPart;
                        ft_Diff.dwLowDateTime=UI_Diff.LowPart;

                        SYSTEMTIME st_Diff;
                        if (FileTimeToSystemTime(&ft_Diff, &st_Diff))
                        {
                            sprintf(Duration, "%02hd:%02hd:%02hd.%03hd", st_Diff.wHour, st_Diff.wMinute, st_Diff.wSecond, st_Diff.wMilliseconds);
                        }
                        else
                            strcpy(Duration, "            ");
                    }
                    else
                        strcpy(Duration, "            ");

                }
                else
                    strcpy(Duration, "            ");
            }
            else
            {
                st_In=st;
                strcpy(Duration, "            ");
            }

            fprintf(F,"                                       %02hd:%02hd:%02hd.%03hd %s", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, Duration);
        }

        void Debug_Close()
        {
            Debug += "\r\n";
            fwrite(Debug.c_str(), Debug.size(), 1, F); \
            fclose(F);
        }
    }
    using namespace ZenLib_Dir_Debug;

    #define ZENLIB_DEBUG1(_NAME,_TOAPPEND) \
        Debug_Open(false); \
        Debug+=", ";Debug+=_NAME; \
        _TOAPPEND; \
        Debug_Close();

    #define ZENLIB_DEBUG2(_NAME,_TOAPPEND) \
        Debug_Open(true); \
        Debug+=", ";Debug+=_NAME; \
        _TOAPPEND; \
        Debug_Close();
#else // ZENLIB_DEBUG
    #define ZENLIB_DEBUG1(_NAME,_TOAPPEND)
    #define ZENLIB_DEBUG2(_NAME,_TOAPPEND)
#endif // ZENLIB_DEBUG

//***************************************************************************
// Constructor/Destructor
//***************************************************************************


//***************************************************************************
// Open/Close
//***************************************************************************

ZtringList Dir::GetAllFileNames(const Ztring &Dir_Name_, dirlist_t Options)
{
    ZENLIB_DEBUG1(     "Dir GetAllFileNames",
                        Debug+=", Dir_Name="; Debug+=Ztring(Dir_Name_).To_UTF8(); Debug+=", Options="; Debug +=Ztring::ToZtring(Options).To_UTF8())

    ZtringList ToReturn;
    Ztring Dir_Name=Dir_Name_;

    #ifdef ZENLIB_USEWX
        int Flags=wxDIR_FILES | wxDIR_DIRS;

        //Search for files
        wxArrayString Liste;
        wxFileName FullPath; FullPath=Dir_Name.c_str();
        //-File
        if (FullPath.FileExists())
        {
            FullPath.Normalize();
            Liste.Add(FullPath.GetFullPath());
        }
        //-Directory
        else if (FullPath.DirExists())
        {
            FullPath.Normalize();
            wxDir::GetAllFiles(FullPath.GetFullPath(), &Liste, Ztring(), Flags);
        }
        //-WildCards
        else
        {
            wxString FileName=FullPath.GetFullName();
            FullPath.SetFullName(Ztring()); //Supress filename
            FullPath.Normalize();
            if (FullPath.DirExists())
                wxDir::GetAllFiles(FullPath.GetPath(), &Liste, FileName, Flags);
        }

        //Compatible array
        ToReturn.reserve(Liste.GetCount());
        for (size_t Pos=0; Pos<Liste.GetCount(); Pos++)
            ToReturn.push_back(Liste[Pos].c_str());
    #else //ZENLIB_USEWX
        #ifdef WINDOWS
            #ifdef WINDOWS_UWP
                //Ensure all slashs are converted to backslashs (WinRT file API don't like slashs)
                Dir_Name.FindAndReplace(__T("/"), __T("\\"));

                ComPtr<IStorageFolder> Folder;
                if (FAILED(Get_Folder(HStringReference(Dir_Name.c_str(), (unsigned int)Dir_Name.size()), Folder)))
                    return ToReturn;

                ComPtr<IAsyncOperation<IVectorView<IStorageItem*>* > > Async_GetItems;
                ComPtr<IVectorView<IStorageItem*> > Items;
                if (FAILED(Folder->GetItemsAsyncOverloadDefaultStartAndCount(&Async_GetItems)) ||
                    FAILED(Await(Async_GetItems)) ||
                    FAILED(Async_GetItems->GetResults(&Items)) ||
                    !Items)
                    return ToReturn;

                unsigned int Size;
                if (FAILED(Items->get_Size(&Size)))
                    return ToReturn;

                for (unsigned int Pos=0; Pos<Size; ++Pos)
                {
                    ComPtr<IStorageItem> Item;
                    if (FAILED(Items->GetAt(Pos, &Item)) || !Item)
                        continue;

                    HString File_Name_HString;
                    if (FAILED(Item->get_Name(File_Name_HString.GetAddressOf())))
                        continue;

                    Ztring File_Name(WindowsGetStringRawBuffer(File_Name_HString.Get(), NULL));

                    if (File_Name.empty() || (!(Options&Include_Hidden) && File_Name[0]==__T('.')))
                        continue;

                    tstring File_Name_Complete = Dir_Name + __T("\\") + File_Name;

                    boolean IsFile = FALSE;
                    boolean IsFolder = FALSE;
                    Item->IsOfType(StorageItemTypes_File, &IsFile);
                    Item->IsOfType(StorageItemTypes_Folder, &IsFolder);

                    if ((Options&Include_Files) && IsFile && ((Options&Include_Hidden) || File_Name[0] != __T('.')))
                    {
                        Add_Item_To_FUA(HStringReference(File_Name_Complete.c_str(), (unsigned int)File_Name_Complete.length()), Item);
                        ToReturn.push_back(File_Name_Complete);
                    }
                    else if ((Options&Include_Dirs) && IsFolder)
                    {
                        Add_Item_To_FUA(HStringReference(File_Name_Complete.c_str(), (unsigned int)File_Name_Complete.length()), Item);
                        ToReturn.push_back(File_Name_Complete);
                    }

                    if (IsFolder && Options&Parse_SubDirs)
                        ToReturn+=GetAllFileNames(File_Name_Complete, Options);
                }

                return ToReturn;
            #else
                #if !defined(UNICODE)
                    //UTF-8 text as the names of the list, also from the bytes of the code page given as they are (see Ztring::To_Unicode())
                    Dir_Name=FileName_From_Wide(FileName_To_Wide_Existing(Dir_Name).c_str());
                #endif

                //Is a dir?
                if (Exists(Dir_Name))
                    Dir_Name=Dir_Pattern(Dir_Name);

                //Path
                Ztring Path=FileName::Path_Get(Dir_Name);
                if (Path.empty())
                {
                    DWORD Path_Size=GetFullPathNameW(FileName_To_Wide(Dir_Name).c_str(), 0, NULL, NULL);
                    wchar_t* PathTemp=new wchar_t[Path_Size+1];
                    if (GetFullPathNameW(FileName_To_Wide(Dir_Name).c_str(), Path_Size+1, PathTemp, NULL))
                        Path=FileName::Path_Get(FileName_From_Wide(PathTemp));
                    delete [] PathTemp;
                }

                WIN32_FIND_DATAW FindFileDataW;
                HANDLE hFind=FindFirstFileW(FileName_To_Wide(Dir_Name).c_str(), &FindFileDataW);

                if (hFind==INVALID_HANDLE_VALUE)
                {
                    ZENLIB_DEBUG2(   "Dir GetAllFileNames",
                                        Debug+=", returns with files count="; Debug +=Ztring::ToZtring(ToReturn.size()).To_UTF8())

                    return ZtringList();
                }

                BOOL ReturnValue;
                do
                {
                    Ztring File_Name=FileName_From_Wide(FindFileDataW.cFileName);
                    if (File_Name!=__T(".") && File_Name!=__T("..")) //Avoid . an ..
                    {
                        if (!(Options&Include_Hidden) && ((!File_Name.empty() && File_Name[0]==__T('.')) ||
                            (FindFileDataW.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN)
                        ))
                        {
                            ReturnValue=FindNextFileW(hFind, &FindFileDataW);
                            continue;
                        }

                        Ztring File_Name_Complete=Path+__T("\\")+File_Name;
                        if (Exists(File_Name_Complete))
                        {
                            if (Options&Include_Dirs)
                                ToReturn.push_back(File_Name_Complete); //A dir
                            if (Options&Parse_SubDirs)
                                ToReturn+=GetAllFileNames(File_Name_Complete, Options); //A SubDir
                        }
                        else if (Options&Include_Files)
                            ToReturn.push_back(File_Name_Complete); //A file
                    }
                    ReturnValue=FindNextFileW(hFind, &FindFileDataW);
                }
                while (ReturnValue);

                FindClose(hFind);
            #endif
        #else //WINDOWS
            #if !defined(UNICODE)
                //UTF-8 text as the names of the list, also from the bytes of the string given as they are (see FileName_To_Native())
                Dir_Name=FileName_From_Native(FileName_To_Native(Dir_Name).c_str());
            #endif

            //A file?
            if (File::Exists(Dir_Name))
            {
               ToReturn.push_back(Dir_Name); //TODO
               return ToReturn;
            }

            //A dir?
            if (!Dir::Exists(Dir_Name))
                return ToReturn; //Does not exist

            //open
            std::string Dir_Name_Native=FileName_To_Native(Dir_Name);
            DIR* Dir=opendir(Dir_Name_Native.c_str());
            if (Dir)
            {
                //This is a dir
                //Normalizing dir (the / at the end)
                if (Dir_Name.empty() || Dir_Name[Dir_Name.size()-1]!=__T('/'))
                {
                    Dir_Name+=__T('/');
                    Dir_Name_Native+='/';
                }

                struct dirent *DirEnt;
                while((DirEnt=readdir(Dir))!=NULL)
                {
                    //A file, with a name which gives the same bytes back with the directory (on macOS, they may depend on the existing files)
                    Ztring File_Name=FileName_From_Native(DirEnt->d_name);
                    if (File_Name.empty() || FileName_To_Native(Dir_Name+File_Name)!=Dir_Name_Native+DirEnt->d_name)
                        continue; //Can't be decoded to a name which can be opened (e.g. an overlong UTF-8 sequence decoded to "/")
                    if (!(Options&Include_Hidden) && File_Name[0]==__T('.'))
                        continue;

                    if (File_Name!=__T(".") && File_Name!=__T("..")) //Avoid . an ..
                    {
                        Ztring File_Name_Complete=Dir_Name+File_Name;
                        if (Exists(File_Name_Complete))
                        {
                            if (Options&Include_Dirs)
                                ToReturn.push_back(File_Name_Complete); //A dir
                            if (Options&Parse_SubDirs)
                                ToReturn+=GetAllFileNames(File_Name_Complete, Options); //A SubDir
                        }
                        else if (Options&Include_Files)
                            ToReturn.push_back(File_Name_Complete); //A file
                    }
                }

                //Close it
                closedir(Dir);
            }
            #if !defined(__ANDROID_API__) || __ANDROID_API__ >= 28
            else
            {
                glob_t globbuf;
                if (glob(FileName_To_Native(Dir_Name).c_str(), GLOB_NOSORT, NULL, &globbuf)==0)
                {
                    for (size_t Pos=0; Pos<globbuf.gl_pathc; Pos++)
                    {
                        Ztring File_Name=FileName_From_Native(globbuf.gl_pathv[Pos]);
                        if (!File_Name.empty() && FileName_To_Native(File_Name)==globbuf.gl_pathv[Pos])
                            ToReturn.push_back(File_Name);
                    }
                    globfree(&globbuf);
                }
            }
            #endif
        #endif
    #endif //ZENLIB_USEWX

    ZENLIB_DEBUG2(   "Dir GetAllFileNames",
                        Debug+=", files count="; Debug +=Ztring::ToZtring(ToReturn.size()).To_UTF8())

    return ToReturn;
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
bool Dir::Exists(const Ztring &File_Name)
{
    ZENLIB_DEBUG1(     "Dir Exists",
                        Debug+=", Dir_Name="; Debug+=Ztring(File_Name).To_UTF8();)

    #ifdef ZENLIB_USEWX
        wxFileName FN(File_Name.c_str());
        return FN.DirExists();
    #else //ZENLIB_USEWX
       #ifdef WINDOWS
           #ifdef WINDOWS_UWP
                //Ensure all slashs are converted to backslashs (WinRT file API don't like slashs)
                Ztring File_Name_=File_Name;
                File_Name_.FindAndReplace(__T("/"), __T("\\"));

                //Try to access folder directly
                ComPtr<IStorageFolder> Folder;
                if (SUCCEEDED(Get_Folder(HStringReference(File_Name_.c_str(), (unsigned int)File_Name_.size()), Folder)))
                    return true;

                //Try directory access methods
                tstring Parent=FileName::Path_Get(File_Name_);
                tstring Dir=FileName::Name_Get(File_Name_);

                ComPtr<IStorageFolder> Parent_Folder;
                if (FAILED(Get_Folder(HStringReference(Parent.c_str(), (unsigned int)Parent.size()), Parent_Folder)))
                    return false;

                //IStorageFolder don't provide TryGetItemAsync
                ComPtr<IStorageFolder2> Parent_Folder2;
                if (FAILED(Parent_Folder->QueryInterface(IID_PPV_ARGS(&Parent_Folder2))) || !Parent_Folder2)
                    return false;

                ComPtr<IAsyncOperation<IStorageItem*> > Async_GetItem;
                ComPtr<IStorageItem> Item;
                ComPtr<IStorageFolder> Child;
                if (SUCCEEDED(Parent_Folder2->TryGetItemAsync(HStringReference(Dir.c_str(), (unsigned int)Dir.size()).Get(), &Async_GetItem)) &&
                    SUCCEEDED(Await(Async_GetItem)) &&
                    SUCCEEDED(Async_GetItem->GetResults(&Item)) &&
                    Item &&
                    SUCCEEDED(Item.As(&Child)))
                    return true;

                return false;
            #else
                DWORD FileAttributes=GetFileAttributesW(FileName_To_Wide_Existing(File_Name).c_str());

                ZENLIB_DEBUG2(   "Dir Exists",
                                    Debug+=", returns "; Debug +=Ztring::ToZtring(((FileAttributes!=INVALID_FILE_ATTRIBUTES) && (FileAttributes&FILE_ATTRIBUTE_DIRECTORY))?1:0).To_UTF8())

                return ((FileAttributes!=INVALID_FILE_ATTRIBUTES) && (FileAttributes&FILE_ATTRIBUTE_DIRECTORY));
            #endif
        #else //WINDOWS
            struct stat buffer;
            int         status;
            status=stat(FileName_To_Native(File_Name).c_str(), &buffer);
            return status==0 && S_ISDIR(buffer.st_mode);
        #endif
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
bool Dir::Create(const Ztring &File_Name)
{
    Ztring Parent=FileName::Path_Get(File_Name);

    if (Parent.length() && !Exists(Parent))
    {
        if (!Create(Parent))
            return false;
    }

  #ifdef ZENLIB_USEWX
        return wxFileName::Mkdir(File_Name.c_str());
    #else //ZENLIB_USEWX
        #ifdef WINDOWS
            #ifdef WINDOWS_UWP
                CreationCollisionOption Collision_Option=CreationCollisionOption_FailIfExists;

                //Ensure all slashs are converted to backslashs (WinRT file API don't like slashs)
                Parent.FindAndReplace(__T("/"), __T("\\"));

                //Split folder and parent
                Ztring Dir=FileName::Name_Get(File_Name);

                //Split folder and parent
                size_t Last_Separator=File_Name.rfind(__T("\\"));

                //Open parent folder
                ComPtr<IStorageFolder> Folder;
                if (FAILED(Get_Folder(HStringReference(Parent.c_str(), (unsigned int)Parent.size()), Folder)))
                    return false;

                //Create folder
                ComPtr<IAsyncOperation<StorageFolder*> > Async_Create;
                ComPtr<IStorageFolder> New;
                if (FAILED(Folder->CreateFolderAsync(HStringReference(Dir.c_str(), (unsigned int)Dir.size()).Get(), Collision_Option, &Async_Create)) ||
                    FAILED(Await(Async_Create)) ||
                    FAILED(Async_Create->GetResults(&New)) ||
                    !New)
                    return false;

                return true;
            #else
                return CreateDirectoryW(FileName_To_Wide(File_Name).c_str(), NULL)!=0;
            #endif
        #else //WINDOWS
            return mkdir(FileName_To_Native(File_Name).c_str(), 0700)==0;
        #endif //WINDOWS
    #endif //ZENLIB_USEWX
}

//***************************************************************************
// GetAllFileNames
//***************************************************************************

#if defined WINDOWS && !defined WINDOWS_UWP
class GetAllFileNames_Private
{
public:
    Ztring Dir_Name;
    Ztring Path;
    Dir::dirlist_t Options;
    HANDLE hFind;
    WIN32_FIND_DATAW FindFileDataW{};

    GetAllFileNames_Private()
        : Options(), hFind(INVALID_HANDLE_VALUE)
    {
    }
};

//---------------------------------------------------------------------------
GetAllFileNames::GetAllFileNames()
    : p(NULL)
{
}

//---------------------------------------------------------------------------
GetAllFileNames::~GetAllFileNames()
{
    Close();
}

//---------------------------------------------------------------------------
void GetAllFileNames::Start  (const Ztring &Dir_Name_, Dir::dirlist_t Options_)
{
    ZENLIB_DEBUG1(     "GetAllFileNames Start",
                        Debug+=", Dir_Name="; Debug+=Ztring(Dir_Name_).To_UTF8(); Debug+=", Options="; Debug +=Ztring::ToZtring(Options_).To_UTF8())

    delete p; p=new GetAllFileNames_Private;
    p->Dir_Name=Dir_Name_;
    p->Options=Options_;

    #ifdef WINDOWS
        //Is a dir?
        if (Dir::Exists(p->Dir_Name))
            p->Dir_Name=Dir_Pattern(p->Dir_Name);

        //Path
        p->Path=FileName::Path_Get(p->Dir_Name);
        if (p->Path.empty())
        {
            DWORD Path_Size=GetFullPathNameW(FileName_To_Wide(p->Dir_Name).c_str(), 0, NULL, NULL);
            wchar_t* PathTemp=new wchar_t[Path_Size+1];
            if (GetFullPathNameW(FileName_To_Wide(p->Dir_Name).c_str(), Path_Size+1, PathTemp, NULL))
                p->Path=FileName::Path_Get(FileName_From_Wide(PathTemp));
            delete [] PathTemp;
        }
    #else //WINDOWS
    #endif

    ZENLIB_DEBUG2(   "GetAllFileNames Start",
                        )
}

bool GetAllFileNames::Next (Ztring& Name)
{
    if (!p)
        return false;

    ZENLIB_DEBUG1(     "GetAllFileNames Next",
                        Debug+=",  Dir_Name="; Debug+=Ztring(p->Dir_Name).To_UTF8())

    #ifdef WINDOWS
        for (;;)
        {
            if (p->hFind==INVALID_HANDLE_VALUE)
            {
                p->hFind=FindFirstFileW(FileName_To_Wide(p->Dir_Name).c_str(), &p->FindFileDataW);

                if (p->hFind==INVALID_HANDLE_VALUE)
                    break;
            }
            else
            {
                BOOL ReturnValue;
                ReturnValue=FindNextFileW(p->hFind, &p->FindFileDataW);
                if (!ReturnValue)
                    break;
            }

            Ztring File_Name=FileName_From_Wide(p->FindFileDataW.cFileName);
            if (File_Name!=__T(".") && File_Name!=__T("..")) //Avoid . an ..
            {
                bool IsOk=false;
                if (p->FindFileDataW.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (p->Options&Dir::Include_Dirs)
                        IsOk=true; //A dir
                }
                else if ((p->Options&Dir::Include_Files) && ((p->Options&Dir::Include_Hidden) || (!File_Name.empty() && File_Name[0]!=__T('.'))))
                    IsOk=true; //A file
                if (IsOk)
                {
                    Name=p->Path+__T("\\")+File_Name;

                    ZENLIB_DEBUG2(   "GetAllFileNames Next",
                                        Debug+=", File_Name="; Debug +=Name.To_UTF8())

                    return true;
                }
            }
        }
    #else //WINDOWS
    #endif

    Close();
    return false;
}

void GetAllFileNames::Close ()
{
    if (!p)
        return;

    ZENLIB_DEBUG1(     "GetAllFileNames Close",
                        Debug+=", Dir_Name="; Debug+=Ztring(p->Dir_Name).To_UTF8())

    FindClose(p->hFind); p->hFind=INVALID_HANDLE_VALUE;
    delete p; p=NULL;

    ZENLIB_DEBUG2(   "GetAllFileNames Close",
                        )
}
#endif //WINDOWS && !WINDOWS_UWP

//***************************************************************************
//
//***************************************************************************

} //namespace

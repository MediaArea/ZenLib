/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// File and directory names longer than MAX_PATH, the limit of the functions of Windows when the process does not support long paths (then
// ZenLib adds the extended-length prefix), for test/longpath.sh
// Usage: zen_longpath directory [network path of the root of the drive of directory, e.g. //localhost/C$ for C:/...]
// LONGPATHS_EXPECTED=enabled|disabled: support of long paths by the process expected on Windows (not checked if not set)
// Prints "NOK: check, message" lines or "OK: count checks", exit code 0 (OK) or 1 (NOK)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "ZenLib/Ztring.h"
#include "ZenLib/ZtringList.h"
#include "ZenLib/File.h"
#include "ZenLib/Dir.h"
#include "ZenLib/FileName.h"
#include "ZenLib/OS_Utils.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(_WIN32)
    #include <direct.h>
    #include <windows.h>
#else
    #include <unistd.h>
#endif
using namespace ZenLib;

//---------------------------------------------------------------------------
static size_t Checks=0;
static int Status=0;

static void Check(bool OK, const std::string& Name, const std::string& Message)
{
    Checks++;
    if (!OK)
    {
        printf("NOK: %s, %s\n", Name.c_str(), Message.c_str());
        Status=1;
    }
}

// Names of the test are ASCII
static Ztring Z(const std::string& Value)
{
    return Ztring().From_UTF8(Value);
}

static std::string S(const Ztring& Value)
{
    return Value.To_UTF8();
}

// Name with "/" as separator, for the comparisons (ZenLib gives "\" on Windows)
static std::string Slashes(const std::string& Value)
{
    std::string ToReturn(Value);
    #if defined(_WIN32)
        std::replace(ToReturn.begin(), ToReturn.end(), '\\', '/');
    #endif
    return ToReturn;
}

// Names of a listing with "/" as separator, sorted
static std::string Listed(const ZtringList& List)
{
    std::vector<std::string> Names;
    for (size_t Pos=0; Pos<List.size(); Pos++)
        Names.push_back(Slashes(S(List[Pos])));
    std::sort(Names.begin(), Names.end());
    std::string ToReturn;
    for (size_t Pos=0; Pos<Names.size(); Pos++)
        ToReturn+=(Pos?" | ":"")+Names[Pos];
    return ToReturn;
}

static bool Has_Prefix(const ZtringList& List)
{
    for (size_t Pos=0; Pos<List.size(); Pos++)
        if (S(List[Pos]).find('?')!=std::string::npos)
            return true;
    return false;
}

// File system functions independent from ZenLib, with the extended-length prefix on Windows (full paths)
#if defined(_WIN32)
static std::wstring Raw_Name(const std::string& Path)
{
    std::wstring ToReturn(L"\\\\?\\");
    size_t Start=0;
    if (Path.size()>=2 && (Path[0]=='/' || Path[0]=='\\') && (Path[1]=='/' || Path[1]=='\\'))
    {
        ToReturn+=L"UNC\\";
        Start=2;
    }
    for (size_t Pos=Start; Pos<Path.size(); Pos++)
        ToReturn+=Path[Pos]=='/'?L'\\':(wchar_t)(unsigned char)Path[Pos];
    return ToReturn;
}

static bool Exists_Raw(const std::string& Path, bool IsDirectory=false)
{
    DWORD Attributes=GetFileAttributesW(Raw_Name(Path).c_str());
    return Attributes!=INVALID_FILE_ATTRIBUTES && ((Attributes&FILE_ATTRIBUTE_DIRECTORY)!=0)==IsDirectory;
}

static long long Size_Raw(const std::string& Path)
{
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (!GetFileAttributesExW(Raw_Name(Path).c_str(), GetFileExInfoStandard, &Data))
        return -1;
    return (((long long)Data.nFileSizeHigh)<<32)|Data.nFileSizeLow;
}

static bool Create_Raw_Directory(const std::string& Path)
{
    return CreateDirectoryW(Raw_Name(Path).c_str(), NULL)!=0;
}

static bool Create_Raw(const std::string& Path)
{
    HANDLE F=CreateFileW(Raw_Name(Path).c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    if (F==INVALID_HANDLE_VALUE)
        return false;
    DWORD Written=0;
    bool OK=WriteFile(F, "0123456789", 10, &Written, NULL) && Written==10;
    return CloseHandle(F) && OK;
}

static bool Change_Directory(const std::string& Path)
{
    return SetCurrentDirectoryW(Raw_Name(Path).c_str()+4)!=0; // Short directory
}
#else //defined(_WIN32)
static bool Exists_Raw(const std::string& Path, bool IsDirectory=false)
{
    struct stat Stat;
    if (stat(Path.c_str(), &Stat))
        return false;
    return IsDirectory?S_ISDIR(Stat.st_mode):S_ISREG(Stat.st_mode);
}

static long long Size_Raw(const std::string& Path)
{
    struct stat Stat;
    if (stat(Path.c_str(), &Stat))
        return -1;
    return (long long)Stat.st_size;
}

static bool Create_Raw_Directory(const std::string& Path)
{
    return !mkdir(Path.c_str(), 0700);
}

static bool Create_Raw(const std::string& Path)
{
    FILE* F=fopen(Path.c_str(), "wb");
    if (!F)
        return false;
    bool Written=fwrite("0123456789", 1, 10, F)==10;
    return !fclose(F) && Written;
}

static bool Change_Directory(const std::string& Path)
{
    return !chdir(Path.c_str());
}
#endif //defined(_WIN32)

// Content of a file with ZenLib
static bool Read_10(const std::string& Path)
{
    File F;
    if (!F.Open(Z(Path)) || F.Size_Get()!=10)
        return false;
    char Buffer[11]={};
    return F.Read((int8u*)Buffer, 10)==10 && !strcmp(Buffer, "0123456789");
}

//---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (argc<2)
    {
        fprintf(stderr, "Usage: zen_longpath directory [network path of the root of the drive of directory]\n");
        return 2;
    }
    setvbuf(stdout, NULL, _IONBF, 0); // Lines already written if the program is stopped
    std::string Directory(argv[1]);
    std::string Network_Root(argc>=3?argv[2]:"");

    // Support of long paths by the process (Windows 10 version 1607 and more, LongPathsEnabled in the registry and longPathAware in the
    // manifest of the program), else the prefix is needed
    #if defined(_WIN32)
        typedef BOOLEAN (WINAPI *RtlAreLongPathsEnabled_Type)();
        RtlAreLongPathsEnabled_Type RtlAreLongPathsEnabled=(RtlAreLongPathsEnabled_Type)(void(*)())GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlAreLongPathsEnabled");
        bool LongPaths=RtlAreLongPathsEnabled && RtlAreLongPathsEnabled();
        printf("OK: long paths of the process: %s\n", LongPaths?"enabled":"disabled");
        char Expected[16];
        DWORD Expected_Size=GetEnvironmentVariableA("LONGPATHS_EXPECTED", Expected, sizeof(Expected));
        if (Expected_Size && Expected_Size<sizeof(Expected))
            Check(!strcmp(Expected, LongPaths?"enabled":"disabled"), "support of long paths by the process", std::string("not ")+Expected+" (registry or manifest not taken into account)");
    #endif

    // Directory tree longer than MAX_PATH also from the directory, each part shorter than the limit of a name (255)
    std::string Relative_Directory;
    for (int i=1; i<=6; i++)
        Relative_Directory+=(i==1?"":"/")+std::string("directory")+(char)('0'+i)+std::string(40, 'x');
    std::string Long_Directory=Directory+"/"+Relative_Directory;
    std::string Long_File=Long_Directory+"/file.txt";
    std::string Inner_File=Long_Directory+"/sub/inner.txt";
    std::string First_Directory=Directory+"/directory1"+std::string(40, 'x');
    printf("OK: name of %u characters\n", (unsigned)Long_File.size());

    // Directory and file created, opened and found
    Check(Dir::Create(Z(Long_Directory)) && Exists_Raw(Long_Directory, true), "Dir::Create()", "not created");
    Check(Dir::Exists(Z(Long_Directory)), "Dir::Exists()", "not found");
    {
        File F;
        Check(F.Create(Z(Long_File)) && F.Write((const int8u*)"0123456789", 10)==10, "File::Create()", "not created");
    }
    Check(Size_Raw(Long_File)==10, "File::Create()", "wrong size");
    Check(File::Exists(Z(Long_File)), "File::Exists()", "not found");
    Check(!Dir::Exists(Z(Long_File)), "Dir::Exists() of a file", "found");
    Check(Read_10(Long_File), "File::Open()", "not opened or wrong content");
    Check(File::Size_Get(Z(Long_File))==10, "File::Size_Get()", "wrong size");
    Check(!File::Modified_Get(Z(Long_File)).empty(), "File::Modified_Get()", "empty");
    if (!Create_Raw_Directory(Long_Directory+"/sub") || !Create_Raw(Inner_File))
        Check(false, "internal", "file not created");
    Check(Read_10(Long_Directory+"/sub/../file.txt"), "File::Open() with \"..\"", "not opened");
    #if defined(_WIN32)
        std::string Long_File_Backslashes(Long_File);
        std::replace(Long_File_Backslashes.begin(), Long_File_Backslashes.end(), '/', '\\');
        Check(Read_10(Long_File_Backslashes), "File::Open() with \"\\\" as separator", "not opened");
        Check(File::Exists(Z(Long_File+". ")), "File::Exists() with trailing dot and space", "not found");
    #endif

    // Directory listing, through the directories longer than MAX_PATH, with the names as given
    std::string Expected_List=Long_File+" | "+Inner_File;
    ZtringList List=Dir::GetAllFileNames(Z(First_Directory));
    Check(Listed(List)==Expected_List && !Has_Prefix(List), "Dir::GetAllFileNames()", "listed "+Listed(List));
    List=Dir::GetAllFileNames(Z(Long_Directory), (Dir::dirlist_t)(Dir::Include_Files|Dir::Include_Dirs));
    Check(Listed(List)==Long_File+" | "+Long_Directory+"/sub" && !Has_Prefix(List), "Dir::GetAllFileNames() of a long directory", "listed "+Listed(List));
    #if defined(_WIN32)
    {
        GetAllFileNames Names;
        Names.Start(Z(Long_Directory), (Dir::dirlist_t)(Dir::Include_Files|Dir::Include_Dirs));
        ZtringList List_Next;
        Ztring Name;
        while (Names.Next(Name))
            List_Next.push_back(Name);
        Check(Listed(List_Next)==Long_File+" | "+Long_Directory+"/sub" && !Has_Prefix(List_Next), "GetAllFileNames::Next()", "listed "+Listed(List_Next));
    }
    #endif

    // Copy (Windows only), move, delete
    std::string Copy_File=Long_Directory+"/copy.txt";
    std::string Moved_File=Long_Directory+"/moved.txt";
    #if defined(_WIN32)
        Check(File::Copy(Z(Long_File), Z(Copy_File)) && Size_Raw(Copy_File)==10, "File::Copy()", "not copied");
    #else
        if (!Create_Raw(Copy_File))
            Check(false, "internal", "file not created");
    #endif
    Check(File::Move(Z(Copy_File), Z(Moved_File)) && Size_Raw(Moved_File)==10 && !Exists_Raw(Copy_File), "File::Move()", "not moved");
    Check(File::Delete(Z(Moved_File)) && !Exists_Raw(Moved_File), "File::Delete()", "not deleted");

    // Relative path longer than MAX_PATH, from a short current directory
    if (!Change_Directory(Directory))
        Check(false, "internal", "current directory not changed");
    std::string Relative_File=Relative_Directory+"/file.txt";
    Check(File::Exists(Z(Relative_File)), "File::Exists() of a relative path", "not found");
    Check(Read_10(Relative_File), "File::Open() of a relative path", "not opened");
    Check(Dir::Exists(Z(Relative_Directory)), "Dir::Exists() of a relative path", "not found");
    List=Dir::GetAllFileNames(Z(Relative_Directory.substr(0, Relative_Directory.find('/'))));
    Check(Listed(List)==Relative_File+" | "+Relative_Directory+"/sub/inner.txt" && !Has_Prefix(List), "Dir::GetAllFileNames() of a relative path", "listed "+Listed(List));
    {
        File F;
        Check(F.Create(Z(Relative_Directory+"/relative.txt")) && F.Write((const int8u*)"0123456789", 10)==10, "File::Create() of a relative path", "not created");
    }
    Check(Size_Raw(Long_Directory+"/relative.txt")==10, "File::Create() of a relative path", "not created");
    Check(Dir::Create(Z(Relative_Directory+"/relative_dir")) && Exists_Raw(Long_Directory+"/relative_dir", true), "Dir::Create() of a relative path", "not created");

    #if defined(_WIN32)
        // Names for the wide functions: prefix only if needed
        std::wstring Wide_Long_File=Raw_Name(Long_File).substr(4);
        Check(FileName_To_Wide(Z(Directory))==Ztring(Z(Directory)).To_Unicode(), "FileName_To_Wide() of a short name", "changed");
        std::wstring Wide=FileName_To_Wide(Z(Long_File));
        Check(Wide==(LongPaths?Z(Long_File).To_Unicode():L"\\\\?\\"+Wide_Long_File), "FileName_To_Wide() of a long name", LongPaths?"changed":"no prefix");
        Wide=FileName_To_Wide(Z(Relative_File));
        Check(Wide==(LongPaths?Z(Relative_File).To_Unicode():L"\\\\?\\"+Wide_Long_File), "FileName_To_Wide() of a long relative name", LongPaths?"changed":"no prefix");
        Check(FileName_To_Wide(Z("\\\\?\\"+Long_File))==Z("\\\\?\\"+Long_File).To_Unicode(), "FileName_To_Wide() of a name with the prefix", "changed");
        Check(FileName_To_Wide(Z("//server/share/"+Relative_File))==(LongPaths?Z("//server/share/"+Relative_File).To_Unicode():L"\\\\?\\UNC\\server\\share\\"+Raw_Name("C:/"+Relative_File).substr(7)), "FileName_To_Wide() of a long network name", LongPaths?"changed":"no prefix");
        #if !defined(UNICODE)
            Check(FileName_To_Wide_Existing(Z(Long_File))==Z(Long_File).To_Unicode(), "FileName_To_Wide_Existing() of a long name", "not the name as given");
        #endif

        // Temporary file in a directory longer than the limit of GetTempFileNameW() (MAX_PATH-14), GetTempPathW() accepts up to MAX_PATH
        std::string Temp_Directory=Directory+"/"+std::string(Directory.size()<200?249-Directory.size()-1:1, 't');
        if (!Create_Raw_Directory(Temp_Directory))
            Check(false, "internal", "directory not created");
        std::string Temp_Directory_Backslashes(Temp_Directory);
        std::replace(Temp_Directory_Backslashes.begin(), Temp_Directory_Backslashes.end(), '/', '\\');
        DWORD TMP_Size=GetEnvironmentVariableW(L"TMP", NULL, 0);
        std::wstring TMP(TMP_Size, L'\0');
        TMP.resize(GetEnvironmentVariableW(L"TMP", &TMP[0], TMP_Size));
        SetEnvironmentVariableW(L"TMP", Z(Temp_Directory_Backslashes).To_Unicode().c_str());
        Ztring Temp_File=FileName::TempFileName_Create(Z("zen_test"));
        SetEnvironmentVariableW(L"TMP", TMP_Size?TMP.c_str():NULL);
        Check(S(Temp_File).find(Temp_Directory_Backslashes+"\\zen")==0 && Size_Raw(S(Temp_File))==0, "FileName::TempFileName_Create() in a directory of 249 characters", "created "+S(Temp_File));
    #endif

    // Network path (Windows)
    if (!Network_Root.empty())
    {
        size_t Colon=Long_File.find(':');
        std::string Network_Directory=Network_Root+Long_Directory.substr(Colon+1);
        std::string Network_File=Network_Directory+"/file.txt";
        Check(File::Exists(Z(Network_File)), "File::Exists() of a network path", "not found");
        Check(Read_10(Network_File), "File::Open() of a network path", "not opened");
        Check(Dir::Exists(Z(Network_Directory)), "Dir::Exists() of a network path", "not found");
        List=Dir::GetAllFileNames(Z(Network_Directory));
        Check(Listed(List)==Network_File+" | "+Network_Directory+"/relative.txt | "+Network_Directory+"/sub/inner.txt" && !Has_Prefix(List), "Dir::GetAllFileNames() of a network path", "listed "+Listed(List));
        {
            File F;
            Check(F.Create(Z(Network_Directory+"/network.txt")) && F.Write((const int8u*)"0123456789", 10)==10, "File::Create() of a network path", "not created");
        }
        Check(Size_Raw(Long_Directory+"/network.txt")==10, "File::Create() of a network path", "not created");
    }

    printf("OK: %u checks\n", (unsigned)Checks);
    return Status;
}

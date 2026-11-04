/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// File names with non-ASCII characters with the locale of the environment, for test/filename.sh
// Usage: zen_filename setlocale(0|1|C) directory
// (0: no call, 1: locale of the environment, C: C locale, the way to get an ASCII locale on Android, which ignores the environment)
// Prints "NOK: check, message" lines or "OK: count checks", exit code 0 (OK) or 1 (NOK)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "ZenLib/Ztring.h"
#include "ZenLib/File.h"
#include "ZenLib/Dir.h"
#include "ZenLib/OS_Utils.h"
#include <clocale>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(_WIN32)
    #include <direct.h>
    #include <windows.h>
#else
    #include <langinfo.h>
    #include <locale.h>
    #include <unistd.h>
    #if defined(__APPLE__)
        #include <xlocale.h>
    #endif
#endif
using namespace ZenLib;

//---------------------------------------------------------------------------
// UTF-8 names, as given to the wide API by e.g. pymediainfo: Latin-1 range, CJK, outside of the BMP
static const char* const Names[]=
{
    "caf\xC3\xA9",
    "\xC3\x85snan \xC3\xA9 \xE6\xB5\x8B\xE8\xAF\x95",
    "\xF0\x9F\x8E\xB5",
};
static const size_t Names_Size=sizeof(Names)/sizeof(*Names);

// First name in Latin-1 or cp1252
static const char* const Name_Latin1="caf\xE9";

// Names which are not valid UTF-8: Latin-1, EUC-JP, overlong sequences of "/" and ".." (they can't be created on macOS and Windows)
static const char* const Names_NotUTF8[]=
{
    "caf\xE9.bin",
    "\xA5\xC6\xA5\xB9\xA5\xC8.bin",
    "\xC0\xAF",
    "a\xC0\257b.bin",
    "\xC0\xAE\xC0\xAE",
};
static const size_t Names_NotUTF8_Size=sizeof(Names_NotUTF8)/sizeof(*Names_NotUTF8);

// Other strings which are not valid UTF-8, for the conversions: truncated characters, UTF-16 surrogates, after U+10FFFF, not UTF-8 bytes, in valid text
static const char* const Strings_NotUTF8[]=
{
    "caf\xC3",
    "\xE6\xB5",
    "\xF0\x9F\x8E",
    "\xED\xB3\xA9",
    "\xED\xA0\x80",
    "\xF4\x90\x80\x80",
    "\xF5\x80\x80\x80",
    "\x80",
    "\xFE\xFF",
    "caf\xC3\xA9\xE9\xC3\xA9",
};
static const size_t Strings_NotUTF8_Size=sizeof(Strings_NotUTF8)/sizeof(*Strings_NotUTF8);

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

// Bytes which are not printable ASCII as \xHH, for the messages
static std::string Printable(const std::string& Value)
{
    std::string ToReturn;
    for (size_t Pos=0; Pos<Value.size(); Pos++)
    {
        unsigned char Char=(unsigned char)Value[Pos];
        if (Char<0x20 || Char>=0x7F)
        {
            char Hex[5];
            snprintf(Hex, sizeof(Hex), "\\x%02X", Char);
            ToReturn+=Hex;
        }
        else
            ToReturn+=(char)Char;
    }
    return ToReturn;
}

static Ztring From_UTF8(const std::string& Value)
{
    Ztring ToReturn;
    ToReturn.From_UTF8(Value.c_str());
    return ToReturn;
}

// With an ASCII locale, a string which is not valid UTF-8 must give the same bytes back
static void Check_Bytes(const std::string& Value)
{
    Ztring Wide;
    Wide.From_Local(Value.c_str());
    Check(!Wide.empty() && Wide.To_Local()==Value, "From_Local("+Printable(Value)+").To_Local()", "result is "+Printable(Wide.To_Local()));
    std::string Native=FileName_To_Native(FileName_From_Native(Value.c_str()));
    Check(Native==Value, "FileName_To_Native(FileName_From_Native("+Printable(Value)+"))", "result is "+Printable(Native));
}

// File system functions with the bytes of the names (UTF-8 on Windows with Unicode), independent from ZenLib and from the locale
#if defined(_WIN32) && defined(UNICODE)
static std::wstring To_Wide(const std::string& Path)
{
    std::wstring ToReturn;
    int Size=MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Path.c_str(), -1, NULL, 0);
    if (Size>1)
    {
        ToReturn.resize(Size);
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Path.c_str(), -1, &ToReturn[0], Size);
        ToReturn.resize(Size-1);
    }
    return ToReturn;
}

static bool Exists_Raw(const std::string& Path, bool IsDirectory=false)
{
    struct _stat64 Stat;
    if (_wstat64(To_Wide(Path).c_str(), &Stat))
        return false;
    return (Stat.st_mode&(IsDirectory?_S_IFDIR:_S_IFREG))!=0;
}

static FILE* Open_Raw(const std::string& Path)
{
    FILE* F=NULL;
    return _wfopen_s(&F, To_Wide(Path).c_str(), L"wb")?NULL:F;
}

static bool Create_Raw_Directory(const std::string& Path)
{
    return !_wmkdir(To_Wide(Path).c_str());
}
#elif defined(_WIN32)
static bool Exists_Raw(const std::string& Path, bool IsDirectory=false)
{
    struct _stat64 Stat;
    if (_stat64(Path.c_str(), &Stat))
        return false;
    return (Stat.st_mode&(IsDirectory?_S_IFDIR:_S_IFREG))!=0;
}

static FILE* Open_Raw(const std::string& Path)
{
    FILE* F=NULL;
    return fopen_s(&F, Path.c_str(), "wb")?NULL:F;
}

static bool Create_Raw_Directory(const std::string& Path)
{
    return !_mkdir(Path.c_str());
}
#else //defined(_WIN32)
static bool Exists_Raw(const std::string& Path, bool IsDirectory=false)
{
    struct stat Stat;
    if (stat(Path.c_str(), &Stat))
        return false;
    return IsDirectory?S_ISDIR(Stat.st_mode):S_ISREG(Stat.st_mode);
}

static FILE* Open_Raw(const std::string& Path)
{
    return fopen(Path.c_str(), "wb");
}

static bool Create_Raw_Directory(const std::string& Path)
{
    return !mkdir(Path.c_str(), 0700);
}
#endif //defined(_WIN32)

static bool Create_Raw(const std::string& Path, size_t Size=10)
{
    FILE* F=Open_Raw(Path);
    if (!F)
        return false;
    bool Written=fwrite("0123456789", 1, Size, F)==Size;
    return !fclose(F) && Written;
}

// Name of a directory listing as the bytes of the name for the file system functions above
static std::string Listed_Path(const Ztring& Path)
{
    #if defined(_WIN32)
        #if defined(UNICODE)
            std::string ToReturn=Path.To_UTF8();
        #else
            std::string ToReturn=Path.To_Local();
        #endif
        for (size_t Pos=0; Pos<ToReturn.size(); Pos++)
            if (ToReturn[Pos]=='\\')
                ToReturn[Pos]='/';
        return ToReturn;
    #else
        return FileName_To_Native(Path);
    #endif
}

//---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (argc<3)
    {
        fprintf(stderr, "Usage: zen_filename setlocale(0|1) directory\n");
        return 2;
    }
    setvbuf(stdout, NULL, _IONBF, 0); // Lines already written if the program is stopped, e.g. endless directory listing
    bool SetLocale=!strcmp(argv[1], "1");
    if (SetLocale)
        setlocale(LC_ALL, "");
    else if (!strcmp(argv[1], "C"))
        setlocale(LC_ALL, "C");
    std::string Directory(argv[2]);

    // Encoding of the locale: ASCII (converted to UTF-8 by ZenLib), UTF-8, or another one (e.g. Latin-1, only a part of the names can be converted)
    #if defined(_WIN32)
        bool IsASCII=false;
        bool IsUnicode=false; // Code page of the system for the narrow strings, with or without setlocale()
        bool IsLatin1=GetACP()==1252;
        bool FileNames_IsUnicode=true; // Wide functions of the system, narrow file names use the code page
        bool FileNames_Narrow_IsUnicode=false;
    #else
        #if !defined(__ANDROID_API__) || __ANDROID_API__ >= 26
            const char* CodeSet=nl_langinfo(CODESET);
        #else
            const char* CodeSet="UTF-8"; // Default of Android, no nl_langinfo() before API level 26
        #endif
        bool IsASCII=!strcmp(CodeSet, "ANSI_X3.4-1968") || !strcmp(CodeSet, "ASCII") || !strcmp(CodeSet, "US-ASCII") || !strcmp(CodeSet, "646");
        bool IsUnicode=IsASCII || !strcmp(CodeSet, "UTF-8") || !strcmp(CodeSet, "utf8");
        bool IsLatin1=!strcmp(CodeSet, "ISO-8859-1") || !strcmp(CodeSet, "ISO8859-1");
        #if defined(__APPLE__) || defined(__ANDROID__)
            bool FileNames_IsUnicode=true; // UTF-8 file names whatever the locale
        #else
            bool FileNames_IsUnicode=IsUnicode;
        #endif
        bool FileNames_Narrow_IsUnicode=FileNames_IsUnicode;
    #endif

    // Text in other encodings, with a character outside of the BMP for UTF-16: UTF-8 whatever the locale, not the low byte of each value, not lost
    {
        static const unsigned char UTF16BE[]={0x00, 0x63, 0x00, 0xE9, 0x6D, 0x4B, 0xD8, 0x3C, 0xDF, 0xB5, 0x00, 0x00};
        static const unsigned char UTF16LE[]={0x63, 0x00, 0xE9, 0x00, 0x4B, 0x6D, 0x3C, 0xD8, 0xB5, 0xDF, 0x00, 0x00};
        std::string Expected_Text("c\xC3\xA9\xE6\xB5\x8B\xF0\x9F\x8E\xB5");
        std::string Text=Ztring().From_UTF16BE((const char*)UTF16BE).To_UTF8();
        Check(Text==Expected_Text, "From_UTF16BE()", "result is "+Printable(Text));
        Text=Ztring().From_UTF16LE((const char*)UTF16LE).To_UTF8();
        Check(Text==Expected_Text, "From_UTF16LE()", "result is "+Printable(Text));
        Text=Ztring().From_ISO_8859_1(Name_Latin1).To_UTF8();
        Check(Text==Names[0], "From_ISO_8859_1("+Printable(Name_Latin1)+")", "result is "+Printable(Text));
        Text=Ztring().From_CC4(0xA9746F6F).To_UTF8();
        Check(Text=="\xC2\xA9too", "From_CC4(0xA9746F6F)", "result is "+Printable(Text));

        // Text which is not valid UTF-8: detected, it is the way to try another encoding
        Text=From_UTF8(Name_Latin1).To_UTF8();
        Check(Text.empty(), "From_UTF8("+Printable(Name_Latin1)+")", "result is "+Printable(Text));

        // Result of To_UTF8() which can be decoded with From_UTF8(), with a string from the local encoding (it is used for documents)
        Ztring Local;
        Local.From_Local(Name_Latin1);
        Text=Local.To_UTF8();
        Check(Local.empty() || !From_UTF8(Text).empty(), "From_UTF8(From_Local("+Printable(Name_Latin1)+").To_UTF8())", "result is "+Printable(Text));
    }

    // Without Unicode, strings are the bytes of the names, no conversion whatever the locale: same results as with the bytes kept with an ASCII locale
    #if !defined(UNICODE)
        IsASCII=true;
        IsUnicode=true;
        IsLatin1=false;
        FileNames_IsUnicode=true;
        FileNames_Narrow_IsUnicode=true;
    #endif

    // Conversions with the locale: UTF-8 with an ASCII locale, not "?" (Linux) or Latin-1 bytes (macOS, FreeBSD)
    if (IsUnicode)
        for (size_t i=0; i<Names_Size; i++)
        {
            std::string Name(Names[i]);
            std::string Local=From_UTF8(Name).To_Local();
            Check(Local==Name, "To_Local("+Printable(Name)+")", "result is "+Printable(Local));
            Ztring Wide;
            Wide.From_Local(Name.c_str());
            Check(Wide==From_UTF8(Name), "From_Local("+Printable(Name)+")", "result is "+Printable(Wide.To_UTF8()));
        }

    // Conversions with another locale or the code page of the system: its encoding
    if (IsLatin1)
    {
        std::string Local=From_UTF8(Names[0]).To_Local();
        Check(Local==Name_Latin1, "To_Local("+Printable(Names[0])+")", "result is "+Printable(Local));
        Ztring Wide;
        Wide.From_Local(Name_Latin1);
        Check(Wide==From_UTF8(Names[0]), "From_Local("+Printable(Name_Latin1)+")", "result is "+Printable(Wide.To_UTF8()));
    }

    // Strings which are not valid UTF-8 with an ASCII locale: same bytes back, not an empty string, not the UTF-8 string of the same characters,
    // and the valid part is decoded
    if (IsASCII)
    {
        for (size_t i=0; i<Names_NotUTF8_Size; i++)
            Check_Bytes(Names_NotUTF8[i]);
        for (size_t i=0; i<Strings_NotUTF8_Size; i++)
            Check_Bytes(Strings_NotUTF8[i]);
        Check_Bytes(Name_Latin1);
        Ztring Wide;
        Wide.From_Local(Name_Latin1);
        Check(Wide!=From_UTF8(Names[0]), "From_Local("+Printable(Name_Latin1)+")", "same string as the UTF-8 name");
        Wide.From_Local("caf\xC3\xA9\xE9");
        Check(Wide.find(From_UTF8(Names[0]))==0 && Wide.size()==From_UTF8(Names[0]).size()+1, "From_Local(valid text and a byte which is not valid)", "result is "+Printable(Wide.To_Local()));
    }

    // File names given to the system or got from it
    if (FileNames_Narrow_IsUnicode)
        for (size_t i=0; i<Names_Size; i++)
        {
            std::string Name(Names[i]);
            std::string Native=FileName_To_Native(From_UTF8(Name));
            Check(Native==Name, "FileName_To_Native("+Printable(Name)+")", "result is "+Printable(Native));
            Ztring Wide=FileName_From_Native(Name.c_str());
            Check(Wide==From_UTF8(Name), "FileName_From_Native("+Printable(Name)+")", "result is "+Printable(Wide.To_UTF8()));
        }
    else if (IsLatin1)
    {
        std::string Native=FileName_To_Native(From_UTF8(Names[0]));
        Check(Native==Name_Latin1, "FileName_To_Native("+Printable(Names[0])+")", "result is "+Printable(Native));
    }
    if (IsLatin1) // Not valid UTF-8, so the locale encoding also where file names are UTF-8, e.g. typed in a terminal on macOS
    {
        Ztring Wide=FileName_From_Native(Name_Latin1);
        Check(Wide==From_UTF8(Names[0]), "FileName_From_Native("+Printable(Name_Latin1)+")", "result is "+Printable(Wide.To_UTF8()));
    }

    #if !defined(_WIN32)
        // ASCII locale for the thread only, then back to the locale of the program: the encoding is the one of the current locale
        if (SetLocale && !IsUnicode)
        {
            locale_t Locale_C=newlocale(LC_ALL_MASK, "C", (locale_t)0);
            if (!Locale_C)
                Check(false, "internal", "newlocale failed");
            else
            {
                locale_t Locale_Program=uselocale(Locale_C);
                Ztring Wide;
                Wide.From_Local(Names[0]);
                std::string Local=From_UTF8(Names[0]).To_Local();
                uselocale(Locale_Program);
                freelocale(Locale_C);
                Check(Wide==From_UTF8(Names[0]) && Local==Names[0], "ASCII locale of the thread", "results are "+Printable(Wide.To_UTF8())+" and "+Printable(Local));
                Wide.From_Local(Names[0]);
                Check(Wide!=From_UTF8(Names[0]), "locale of the program after the locale of the thread", "UTF-8 is still used");
            }
        }
    #endif //!defined(_WIN32)

    // File and Dir functions with the names of the wide API, the files must have the UTF-8 names
    if (FileNames_IsUnicode)
        for (size_t i=0; i<Names_Size; i++)
        {
            std::string Path=Directory+"/file_"+Names[i];
            std::string Path_Moved=Path+"_moved";
            std::string Path_Directory=Directory+"/directory_"+Names[i];
            std::string Shown=Printable(Names[i]);

            File F;
            bool Created=F.Create(From_UTF8(Path));
            if (Created)
            {
                Created=F.Write((const int8u*)"0123456789", 10)==10;
                F.Close();
            }
            Check(Created && Exists_Raw(Path), "File::Create("+Shown+")", Created?"file created with another name":"file not created");
            Check(File::Exists(From_UTF8(Path)), "File::Exists("+Shown+")", "file not found");
            Check(File::Size_Get(From_UTF8(Path))==10, "File::Size_Get("+Shown+")", "wrong size");
            bool Opened=F.Open(From_UTF8(Path));
            Check(Opened && !F.Modified_Get().empty(), "File::Open("+Shown+")", Opened?"no modification date":"file not opened");
            F.Close();
            Check(File::Move(From_UTF8(Path), From_UTF8(Path_Moved)) && Exists_Raw(Path_Moved) && !Exists_Raw(Path), "File::Move("+Shown+")", "file not moved");
            Check(File::Delete(From_UTF8(Path_Moved)) && !Exists_Raw(Path_Moved), "File::Delete("+Shown+")", "file not deleted");
            Check(Dir::Create(From_UTF8(Path_Directory)) && Exists_Raw(Path_Directory, true) && Dir::Exists(From_UTF8(Path_Directory)), "Dir::Create("+Shown+")", "directory not created with this name");
        }
    else if (IsLatin1)
    {
        // The files must have the name in the encoding of the locale
        std::string Path=Directory+"/file_"+Name_Latin1;
        File F;
        Check(F.Create(From_UTF8(Directory+"/file_"+Names[0])) && Exists_Raw(Path), "File::Create("+Printable(Names[0])+")", "file not created with this name");
    }

    // Names decoded with From_Local(), as some programs decode their command line arguments (e.g. MediaConch, dvrescue): not encoded twice
    if (IsUnicode)
        for (size_t i=0; i<Names_Size; i++)
        {
            std::string Path=Directory+"/argument_"+Names[i];
            if (!Create_Raw(Path))
            {
                Check(false, "internal", "can't create "+Printable(Path));
                continue;
            }
            Ztring Path_Local;
            Path_Local.From_Local(Path.c_str());
            File F;
            Check(File::Exists(Path_Local) && F.Open(Path_Local), "File::Open(From_Local("+Printable(Names[i])+"))", "file not found");
        }
    else if (IsLatin1)
    {
        std::string Path=Directory+"/argument_"+(FileNames_IsUnicode?Names[0]:Name_Latin1);
        if (!Create_Raw(Path))
            Check(false, "internal", "can't create "+Printable(Path));
        else
        {
            Ztring Path_Local;
            Path_Local.From_Local((Directory+"/argument_"+Name_Latin1).c_str());
            File F;
            Check(File::Exists(Path_Local) && F.Open(Path_Local), "File::Open(From_Local("+Printable(Name_Latin1)+"))", "file not found");
        }
    }

    // Name which is not valid UTF-8 with an ASCII locale, next to the UTF-8 name of the same characters: the file with this name is used
    // (these names can't be created on macOS)
    if (IsASCII)
    {
        std::string Path=Directory+"/twin_"+Name_Latin1;
        std::string Path_UTF8=Directory+"/twin_"+Names[0];
        if (Create_Raw(Path) && Create_Raw(Path_UTF8, 4))
        {
            Ztring Path_Local;
            Path_Local.From_Local(Path.c_str());
            std::string Shown=Printable(Name_Latin1);
            File F;
            Check(File::Exists(Path_Local) && File::Size_Get(Path_Local)==10 && F.Open(Path_Local) && F.Size_Get()==10, "File::Open("+Shown+")", "file not found or file with the UTF-8 name");
            F.Close();
            Check(File::Delete(Path_Local) && !Exists_Raw(Path) && Exists_Raw(Path_UTF8), "File::Delete("+Shown+")", "file not deleted or file with the UTF-8 name deleted");
        }
    }

    // Directory listing with names which can't be decoded, or not to a name which can be opened: it must end, with existing files only
    std::string List_Path=Directory+"/list";
    std::string SubDirectory=std::string("sub_")+Names[0];
    std::set<std::string> Expected; // Names relative to List_Path
    bool Created=Create_Raw_Directory(List_Path) && Create_Raw_Directory(List_Path+"/"+SubDirectory);
    for (size_t i=0; i<Names_Size; i++)
    {
        std::string Name=std::string(Names[i])+".bin";
        Created=Create_Raw(List_Path+"/"+Name) && Created;
        Expected.insert(Name);
    }
    Created=Create_Raw(List_Path+"/ascii.bin") && Create_Raw(List_Path+"/"+SubDirectory+"/inner.bin") && Created;
    Expected.insert("ascii.bin");
    Expected.insert(SubDirectory+"/inner.bin");
    std::set<std::string> Expected_All=Expected; // With the names which are not valid UTF-8
    size_t NotUTF8_Count=0;
    for (size_t i=0; i<Names_NotUTF8_Size; i++)
        if (Create_Raw(List_Path+"/"+Names_NotUTF8[i])) // Not on macOS and Windows
        {
            NotUTF8_Count++;
            Expected_All.insert(Names_NotUTF8[i]);
        }
    std::string SubDirectory_NotUTF8=std::string("sub_")+Name_Latin1;
    if (Create_Raw_Directory(List_Path+"/"+SubDirectory_NotUTF8) && Create_Raw(List_Path+"/"+SubDirectory_NotUTF8+"/inner.bin"))
    {
        NotUTF8_Count++;
        Expected_All.insert(SubDirectory_NotUTF8+"/inner.bin");
    }
    if (!Created)
        Check(false, "internal", "can't create the files in "+Printable(List_Path));
    else
    {
        ZtringList List=Dir::GetAllFileNames(From_UTF8(List_Path), (Dir::dirlist_t)(Dir::Include_Files|Dir::Parse_SubDirs));
        std::set<std::string> Listed;
        std::string Listed_Shown, NotExisting;
        for (size_t i=0; i<List.size(); i++)
        {
            std::string Path=Listed_Path(List[i]);
            if (Path.compare(0, List_Path.size()+1, List_Path+"/") || !Exists_Raw(Path))
                NotExisting+=" "+Printable(Path);
            else
            {
                Listed.insert(Path.substr(List_Path.size()+1));
                Listed_Shown+=" "+Printable(Path.substr(List_Path.size()+1));
            }
        }
        Check(NotExisting.empty(), "Dir::GetAllFileNames, existing files", "not existing:"+NotExisting);
        if (IsASCII)
            Check(Listed==Expected_All, "Dir::GetAllFileNames, all names", "listed:"+Listed_Shown);
        else if (FileNames_IsUnicode)
            Check(Listed==Expected, "Dir::GetAllFileNames, valid UTF-8 names", "listed:"+Listed_Shown);
        else if (IsLatin1)
            Check(List.size()==Expected.size()+NotUTF8_Count, "Dir::GetAllFileNames, all names", "listed:"+Listed_Shown); // Each byte is a character
        else
            Check(Listed.count("ascii.bin")==1, "Dir::GetAllFileNames, names which can be decoded", "listed:"+Listed_Shown); // e.g. EUC-JP
    }

    #if !defined(_WIN32) && (!defined(__ANDROID_API__) || __ANDROID_API__ >= 28)
        // Directory which can't be read, for the part of the directory listing based on glob() (not for root, and not with an
        // ASCII locale on Android, its glob() does not accept non-ASCII bytes)
        #if defined(__ANDROID__)
            bool Glob_IsUnicode=!IsASCII;
        #else
            bool Glob_IsUnicode=true;
        #endif
        if (FileNames_IsUnicode && Glob_IsUnicode && geteuid())
        {
            std::string Path=Directory+"/unreadable_"+Names[0];
            if (!Create_Raw_Directory(Path) || chmod(Path.c_str(), 0))
                Check(false, "internal", "can't create "+Printable(Path));
            else
            {
                ZtringList List=Dir::GetAllFileNames(From_UTF8(Path));
                chmod(Path.c_str(), 0700);
                Check(List.size()==1 && List[0]==From_UTF8(Path), "Dir::GetAllFileNames, directory which can't be read", List.empty()?"nothing listed":"listed "+Printable(List[0].To_UTF8()));
            }
        }
    #endif //!defined(_WIN32) && (!defined(__ANDROID_API__) || __ANDROID_API__ >= 28)

    if (!Status)
        printf("OK: %u checks\n", (unsigned)Checks);
    return Status;
}

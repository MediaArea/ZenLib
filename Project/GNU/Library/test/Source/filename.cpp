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
#include "ZenLib/FileName.h"
#include "ZenLib/OS_Utils.h"
#include "ZenLib/ZtringListListF.h"
#include <cctype>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <map>
#include <set>
#include <string>
#include <vector>
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

// Other strings for the multibyte encodings of locales: first byte without the next one (EUC, Shift_JIS, GBK, Big5, GB18030), second byte
// which is not valid in some of them, parts of a 4-byte character of GB18030, bytes not used
static const char* const Strings_Multibyte[]=
{
    "\xA5",
    "a\xA5",
    "\xA5\x41",
    "\xA5\xC6\xA5",
    "\x8E",
    "\x8F\xA1",
    "\x81\x30",
    "\x81\x30\x81",
    "\x83\x5C",
    "\x80\xFF",
};
static const size_t Strings_Multibyte_Size=sizeof(Strings_Multibyte)/sizeof(*Strings_Multibyte);

// Text in the encodings of locales which are neither ASCII nor UTF-8 (names of the encoding in upper case without "-" and "_"), with
// characters which are not the same value in wchar_t with some systems (e.g. macOS and FreeBSD), and a character outside of the BMP
// (and for EUC-JP a character whose bytes are also valid UTF-8, of another character)
struct encoding_text
{
    const char* Names;
    const char* Text;
    const char* Local;
    const char* Text_ValidUTF8;
    const char* Local_ValidUTF8;
};
static const encoding_text Encoding_Texts[]=
{
    {" ISO88591 CP1252 ",       "caf\xC3\xA9",                                          "caf\xE9",                              NULL, NULL},
    {" ISO885915 ",             "\xE2\x82\xAC caf\xC3\xA9",                             "\xA4 caf\xE9",                         NULL, NULL},
    {" KOI8R ",                 "\xD0\xA2\xD0\xB5\xD1\x81\xD1\x82",                     "\xF4\xC5\xD3\xD4",                     NULL, NULL},
    {" CP1251 ",                "\xD0\xA2\xD0\xB5\xD1\x81\xD1\x82",                     "\xD2\xE5\xF1\xF2",                     NULL, NULL},
    {" EUCJP ",                 "\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88\xE8\xA1\xA8",     "\xA5\xC6\xA5\xB9\xA5\xC8\xC9\xBD",     "\xE8\xA1\xA8", "\xC9\xBD"},
    {" SJIS SHIFTJIS CP932 ",   "\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88\xE8\xA1\xA8",     "\x83\x65\x83\x58\x83\x67\x95\x5C",     NULL, NULL},
    {" GB18030 ",               "\xE6\xB5\x8B\xE8\xAF\x95\xF0\x9F\x8E\xB5",             "\xB2\xE2\xCA\xD4\x94\x39\xC1\x39",     NULL, NULL},
    {" GBK CP936 ",             "\xE6\xB5\x8B\xE8\xAF\x95",                             "\xB2\xE2\xCA\xD4",                     NULL, NULL},
    {" BIG5 CP950 ",            "\xE6\xB8\xAC\xE8\xA9\xA6",                             "\xB4\xFA\xB8\xD5",                     NULL, NULL},
    {" EUCKR CP949 ",           "\xED\x85\x8C\xEC\x8A\xA4\xED\x8A\xB8",                 "\xC5\xD7\xBD\xBA\xC6\xAE",             NULL, NULL},
};
static const size_t Encoding_Texts_Size=sizeof(Encoding_Texts)/sizeof(*Encoding_Texts);

// Text of the encoding of the locale or of the code page, NULL if there is none
static const encoding_text* Encoding_Text(std::string Name)
{
    std::string Key=" ";
    for (size_t Pos=0; Pos<Name.size(); Pos++)
        if (Name[Pos]!='-' && Name[Pos]!='_')
            Key+=(char)toupper((unsigned char)Name[Pos]);
    Key+=' ';
    for (size_t i=0; i<Encoding_Texts_Size; i++)
        if (strstr(Encoding_Texts[i].Names, Key.c_str()))
            return Encoding_Texts+i;
    return NULL;
}

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

// Encoding of the locale by the C library or of the code page by the system, false if a character can't be encoded
// (Unix: only where wchar_t is Unicode with all locales)
#if defined(_WIN32) || defined(__STDC_ISO_10646__)
    #define HAVE_ENCODE_REFERENCE

// Other names for the multibyte encodings: Japanese (EUC-JP, cp932 where the second byte of this character is "\"), also in GB18030
static const char* const Names_Multibyte[]=
{
    "\xE8\xA1\xA8",
    "\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88",
};
static const size_t Names_Multibyte_Size=sizeof(Names_Multibyte)/sizeof(*Names_Multibyte);

static bool Reference_Encode(const std::wstring& Value, std::string& ToReturn)
{
    #if defined(_WIN32)
        BOOL UsedDefaultChar=FALSE;
        int Size=WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, Value.c_str(), -1, NULL, 0, NULL, &UsedDefaultChar);
        if (Size<=0 || UsedDefaultChar)
            return false;
        std::vector<char> Buffer(Size);
        WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, Value.c_str(), -1, &Buffer[0], Size, NULL, NULL);
        ToReturn.assign(&Buffer[0], Size-1);
    #else
        size_t Size=wcstombs(NULL, Value.c_str(), 0);
        if (Size==(size_t)-1)
            return false;
        std::vector<char> Buffer(Size+1);
        wcstombs(&Buffer[0], Value.c_str(), Size+1);
        ToReturn.assign(&Buffer[0], Size);
    #endif
    return true;
}
#endif

// Unicode of a valid UTF-8 text, independent from ZenLib (UTF-16 where wchar_t is 16-bit), where it is used
#if !defined(UNICODE) || defined(HAVE_ENCODE_REFERENCE)
static std::wstring UTF8_To_Wide(const std::string& Value)
{
    std::wstring ToReturn;
    for (size_t Pos=0; Pos<Value.size();)
    {
        unsigned char Char=(unsigned char)Value[Pos++];
        unsigned long Code=Char;
        int More=Char>=0xF0?3:(Char>=0xE0?2:(Char>=0xC0?1:0));
        if (More)
            Code&=0x3F>>More;
        for (int i=0; i<More && Pos<Value.size(); i++)
            Code=(Code<<6)|(Value[Pos++]&0x3F);
        if (sizeof(wchar_t)==2 && Code>=0x10000)
        {
            ToReturn+=(wchar_t)(0xD800|((Code-0x10000)>>10));
            ToReturn+=(wchar_t)(0xDC00|((Code-0x10000)&0x3FF));
        }
        else
            ToReturn+=(wchar_t)Code;
    }
    return ToReturn;
}
#endif

// A string which is not valid in the encoding of the locale (UTF-8 with an ASCII locale) must give the same bytes back, and valid UTF-8,
// also as a file name if the names are not converted from the locale encoding to UTF-8 (e.g. on macOS)
static void Check_Bytes(const std::string& Value, bool FileNames)
{
    Ztring Wide;
    Wide.From_Local(Value.c_str());
    Check(!Wide.empty() && Wide.To_Local()==Value, "From_Local("+Printable(Value)+").To_Local()", "result is "+Printable(Wide.To_Local()));
    std::string UTF8=Wide.To_UTF8();
    Check(!From_UTF8(UTF8).empty(), "From_Local("+Printable(Value)+").To_UTF8()", "not valid UTF-8: "+Printable(UTF8));
    #if !defined(UNICODE)
        std::wstring Unicode=Ztring(Value).To_Unicode();
        Check(Ztring().From_Unicode(Unicode).To_Unicode()==Unicode, "To_Unicode(From_Unicode(To_Unicode("+Printable(Value)+")))", "not the same values");
    #endif
    if (!FileNames)
        return;
    std::string Native=FileName_To_Native(FileName_From_Native(Value.c_str()));
    Check(Native==Value, "FileName_To_Native(FileName_From_Native("+Printable(Value)+"))", "result is "+Printable(Native));
}

// File system functions with the bytes of the names (UTF-8 on Windows), independent from ZenLib and from the locale
#if defined(_WIN32)
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
        std::string ToReturn=Path.To_UTF8();
        for (size_t Pos=0; Pos<ToReturn.size(); Pos++)
            if (ToReturn[Pos]=='\\')
                ToReturn[Pos]='/';
        return ToReturn;
    #else
        return FileName_To_Native(Path);
    #endif
}

// Name typed in a terminal, as a command line argument decoded by MediaInfo
static Ztring Typed(const std::string& Path)
{
    return FileName_From_Native(Path.c_str());
}

// File of a volume which keeps the bytes of the names: name of the file system, size, name typed in a terminal
struct volume_file
{
    std::string Raw;
    size_t      Size;
    std::string Typed;
};

static void Volume_Add(std::vector<volume_file>& Files, const std::string& Raw, size_t Size, const std::string& Typed)
{
    volume_file File_Temp;
    File_Temp.Raw=Raw;
    File_Temp.Size=Size;
    File_Temp.Typed=Typed;
    Files.push_back(File_Temp);
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
        // Code page of the system, or of the program (activeCodePage of its manifest), for the narrow strings, with or without setlocale()
        printf("OK: code page %u\n", (unsigned)GetACP());
        bool IsASCII=false;
        bool IsUnicode=GetACP()==CP_UTF8;
        bool IsLatin1=GetACP()==1252;
        bool FileNames_IsUnicode=true; // Wide functions of the system, narrow file names use the code page
        bool FileNames_Narrow_IsUnicode=IsUnicode;
        char CodePage[16];
        snprintf(CodePage, sizeof(CodePage), "CP%u", (unsigned)GetACP());
        std::string CodeSet(CodePage);
    #else
        #if !defined(__ANDROID_API__) || __ANDROID_API__ >= 26
            std::string CodeSet(nl_langinfo(CODESET));
        #else
            std::string CodeSet("UTF-8"); // Default of Android, no nl_langinfo() before API level 26
        #endif
        bool IsASCII=CodeSet=="ANSI_X3.4-1968" || CodeSet=="ASCII" || CodeSet=="US-ASCII" || CodeSet=="646";
        bool IsUnicode=IsASCII || CodeSet=="UTF-8" || CodeSet=="utf8";
        bool IsLatin1=CodeSet=="ISO-8859-1" || CodeSet=="ISO8859-1";
        #if defined(__APPLE__) || defined(__ANDROID__) || defined(ZENLIB_FILENAME_UTF8)
            bool FileNames_IsUnicode=true; // UTF-8 file names whatever the locale
        #else
            bool FileNames_IsUnicode=IsUnicode;
        #endif
        bool FileNames_Narrow_IsUnicode=FileNames_IsUnicode;
    #endif
    printf("OK: encoding %s\n", CodeSet.c_str());
    const encoding_text* Encoding=IsUnicode?NULL:Encoding_Text(CodeSet);

    // Names of the file system converted from and to a locale which is neither ASCII nor UTF-8 (e.g. macOS, the names of the file system are
    // UTF-8, also without Unicode), so the bytes of a name may depend on the existing files (e.g. network volumes, see FileName_To_Native())
    #if defined(__APPLE__) || defined(ZENLIB_FILENAME_UTF8)
        bool Names_AreConverted=!IsUnicode;
    #else
        bool Names_AreConverted=false;
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

        // UTF-16 surrogates encoded in UTF-8 (e.g. a byte kept in a std::string made from a Ztring, or CESU-8): accepted as with Unicode, a
        // surrogate which is not in a pair kept, a pair as one character
        std::wstring Surrogate=Ztring().From_UTF8("a\xED\xB3\xA9" "b").To_Unicode();
        Check(Surrogate.size()==3 && Surrogate[1]==0xDCE9, "From_UTF8(UTF-16 surrogate which is not in a pair)", "wrong result");
        Text=Ztring().From_UTF8("\xED\xA0\xBC\xED\xBE\xB5").To_UTF8();
        Check(Text=="\xF0\x9F\x8E\xB5", "From_UTF8(UTF-16 surrogate pair)", "result is "+Printable(Text));
        std::wstring Pair=Ztring().From_UTF8("\xED\xA0\xBC\xED\xBE\xB5").To_Unicode();
        Check(Pair.size()==2 && Pair[0]==0xD83C && Pair[1]==0xDFB5, "From_UTF8(UTF-16 surrogate pair).To_Unicode()", "wrong result");

        // Other sequences which are not valid UTF-8 but accepted as with Unicode: an overlong sequence is the character, a value after
        // U+10FFFF is U+FFFD; other bytes are not accepted
        Text=Ztring().From_UTF8("\xC1\x81" "\xE0\x81\x81" "\xF0\x80\x81\x81" "\xF4\x90\x80\x80" "b").To_UTF8();
        Check(Text=="AAA\xEF\xBF\xBD" "b", "From_UTF8(overlong sequences and a value after U+10FFFF)", "result is "+Printable(Text));
        Text=Ztring().From_UTF8("a\xF8\x88\x80\x80\x80").To_UTF8();
        Check(Text.empty(), "From_UTF8(5 bytes)", "result is "+Printable(Text));

        // Result of To_UTF8() which can be decoded with From_UTF8(), with a string from the local encoding (it is used for documents)
        Ztring Local;
        Local.From_Local(Name_Latin1);
        Text=Local.To_UTF8();
        Check(Local.empty() || !From_UTF8(Text).empty(), "From_UTF8(From_Local("+Printable(Name_Latin1)+").To_UTF8())", "result is "+Printable(Text));

        // Values which are not characters, e.g. a byte kept by From_Local() or the first part of a UTF-16 surrogate pair alone: U+FFFD
        #if defined(UNICODE)
            Ztring Values;
            Values+=(wchar_t)0xD83C; // Surrogate pair
            Values+=(wchar_t)0xDFB5;
            Values+=__T('a');
            Values+=(wchar_t)0xDCE9;
            Values+=(wchar_t)0xD800;
            Values+=__T('b');
            Values+=(wchar_t)0xD83C; // At the end of the string
            Text=Values.To_UTF8();
            Check(Text=="\xF0\x9F\x8E\xB5" "a\xEF\xBF\xBD\xEF\xBF\xBD" "b\xEF\xBF\xBD", "To_UTF8(values which are not characters)", "result is "+Printable(Text));
        #endif
    }

    // MakeLowerCase() and MakeUpperCase(): the characters as with Unicode (towlower() and towupper() of the locale), not the bytes of the UTF-8
    // text without Unicode (e.g. with a Latin-1 locale, the bytes of other characters were changed)
    {
        static const wchar_t Text[]={0xC9, L'T', 0xC9, L' ', L'c', L'a', L'f', 0xE9, L' ', 0x6D4B, L' ', 0x0130, L'I', 0};
        std::wstring Lower(Text), Upper(Text);
        for (size_t i=0; i<Lower.size(); i++)
        {
            Lower[i]=(wchar_t)towlower(Lower[i]);
            Upper[i]=(wchar_t)towupper(Upper[i]);
        }
        Ztring Value;
        Value.From_Unicode(Text);
        std::wstring Result=Ztring(Value).MakeLowerCase().To_Unicode();
        Check(Result==Lower, "MakeLowerCase()", "result is "+Printable(Ztring().From_Unicode(Result).To_UTF8()));
        Result=Ztring(Value).MakeUpperCase().To_Unicode();
        Check(Result==Upper, "MakeUpperCase()", "result is "+Printable(Ztring().From_Unicode(Result).To_UTF8()));
    }

    // Without Unicode, the UTF-8 text and Unicode with To_Unicode() and From_Unicode()
    {
        #if !defined(UNICODE)
            for (size_t i=0; i<Names_Size; i++)
            {
                std::wstring Wide=UTF8_To_Wide(Names[i]);
                Check(Ztring(Names[i]).To_Unicode()==Wide, "To_Unicode("+Printable(Names[i])+")", "wrong result");
                std::string UTF8=Ztring().From_Unicode(Wide);
                Check(UTF8==Names[i], "From_Unicode("+Printable(Names[i])+")", "result is "+Printable(UTF8));
            }

            // Bytes which are not valid UTF-8 kept as U+DC80 to U+DCFF, encoded by From_Unicode() as the other UTF-16 surrogates which are not in a
            // pair, as 3 bytes as in WTF-8 (not other characters, e.g. bytes kept which are valid UTF-8 together), and replaced by To_UTF8()
            #if !defined(_WIN32) // Windows: bytes of the code page, see below
                std::wstring Kept=Ztring("caf\xC3\xA9\xE9").To_Unicode();
                Check(Kept.size()==5 && Kept[3]==0xE9 && Kept[4]==0xDCE9, "To_Unicode(valid text and a byte which is not valid)", "wrong result");
            #endif
            std::wstring Bytes;
            Bytes+=(wchar_t)0xDCC3;
            Bytes+=(wchar_t)0xDC85;
            Check(Ztring().From_Unicode(Bytes).To_Unicode()==Bytes, "To_Unicode(From_Unicode(bytes kept which are valid UTF-8 together))", "not the same values");
            std::wstring Values;
            Values+=(wchar_t)0xD83C; // Surrogate pair
            Values+=(wchar_t)0xDFB5;
            Values+=L'a';
            Values+=(wchar_t)0xDCE9;
            Values+=(wchar_t)0xD800;
            Values+=L'b';
            Values+=(wchar_t)0xD83C; // At the end of the string
            std::string Text=Ztring().From_Unicode(Values);
            Check(Text=="\xF0\x9F\x8E\xB5" "a\xED\xB3\xA9\xED\xA0\x80" "b\xED\xA0\xBC", "From_Unicode(values which are not characters)", "result is "+Printable(Text));
            Text=Ztring(Text).To_UTF8();
            Check(Text=="\xF0\x9F\x8E\xB5" "a\xEF\xBF\xBD\xEF\xBF\xBD" "b\xEF\xBF\xBD", "To_UTF8(From_Unicode(values which are not characters))", "result is "+Printable(Text));
        #endif
    }

    // Without Unicode on Windows, the bytes of the code page given as they are (e.g. the arguments of main()) are not UTF-8 text: decoded with
    // the code page as the narrow functions of Windows do, also the names of the files (not the bytes which are also UTF-8 text, read as UTF-8)
    #if defined(_WIN32) && !defined(UNICODE)
    {
        bool Directory_ASCII=true; // The name with the directory must be bytes of the code page
        for (size_t Pos=0; Pos<Directory.size(); Pos++)
            if ((unsigned char)Directory[Pos]>=0x80)
                Directory_ASCII=false;
        std::string CodePage_Directory=Directory+"/codepage";
        bool Files=Directory_ASCII && Create_Raw_Directory(CodePage_Directory);
        for (size_t i=0; i<Names_Size+Names_Multibyte_Size; i++)
        {
            std::string Name(i<Names_Size?Names[i]:Names_Multibyte[i-Names_Size]);
            std::wstring Wide=UTF8_To_Wide(Name);
            std::string Bytes;
            if (!Reference_Encode(Wide, Bytes) || (Bytes!=Name && !From_UTF8(Bytes).empty()))
                continue; // Not in the code page, or also UTF-8 text
            std::string Shown=Printable(Bytes);
            Ztring Value(Bytes);
            Check(Value.To_Unicode()==Wide, "To_Unicode("+Shown+"), bytes of the code page", "wrong result");
            Check(Value.To_UTF8()==Name, "To_UTF8("+Shown+"), bytes of the code page", "result is "+Printable(Value.To_UTF8()));
            Check(Value.To_Local()==Bytes, "To_Local("+Shown+"), bytes of the code page", "result is "+Printable(Value.To_Local()));
            if (!Files)
                continue;
            if (!Create_Raw(CodePage_Directory+"/"+Name))
            {
                Check(false, "internal", "file not created");
                continue;
            }
            Ztring Path(CodePage_Directory+"/"+Bytes);
            Check(File::Exists(Path), "File::Exists("+Shown+"), bytes of the code page", "not found");
            File F;
            Check(F.Open(Path) && F.Size_Get()==10, "File::Open("+Shown+"), bytes of the code page", "not opened");
        }
    }
    #endif

    // Files of the next checks in their own directory
    std::string Names_Directory=Directory+"/names";
    if (!Create_Raw_Directory(Names_Directory))
        Check(false, "internal", "directory not created");

    // Path and name of a file in a directory whose name is not ASCII, and listing of this directory given with a separator at the end (without
    // Unicode on Windows, the second byte of a character of a multibyte code page, e.g. "\" in cp932, is only in the bytes of the code page)
    {
        std::vector<std::string> Directory_Names(Names, Names+Names_Size);
        #if defined(HAVE_ENCODE_REFERENCE)
            Directory_Names.insert(Directory_Names.end(), Names_Multibyte, Names_Multibyte+Names_Multibyte_Size);
        #endif
        for (size_t i=0; i<Directory_Names.size(); i++)
        {
            std::string Path_Directory=Names_Directory+"/path_"+Directory_Names[i];
            std::string Shown=Printable(Path_Directory+"/file.txt");
            Ztring Path=From_UTF8(Path_Directory+"/file.txt");
            Check(FileName::Path_Get(Path)==From_UTF8(Path_Directory), "FileName::Path_Get("+Shown+")", "result is "+Printable(FileName::Path_Get(Path).To_UTF8()));
            Check(FileName::Name_Get(Path)==From_UTF8("file"), "FileName::Name_Get("+Shown+")", "result is "+Printable(FileName::Name_Get(Path).To_UTF8()));
            #if defined(_WIN32)
                std::string Path_Directory_Raw=Path_Directory; // UTF-8 for the wide functions
            #else
                std::string Path_Directory_Raw=FileName_To_Native(From_UTF8(Path_Directory));
                if (FileName_From_Native(Path_Directory_Raw.c_str())!=From_UTF8(Path_Directory))
                    continue; // Not in the encoding of the locale
            #endif
            if (!Create_Raw_Directory(Path_Directory_Raw) || !Create_Raw(Path_Directory_Raw+"/file.txt"))
            {
                Check(false, "internal", "file not created");
                continue;
            }
            ZtringList List=Dir::GetAllFileNames(From_UTF8(Path_Directory+"/"));
            Check(List.size()==1 && Listed_Path(List[0])==Path_Directory_Raw+"/file.txt" && File::Exists(List[0]), "Dir::GetAllFileNames("+Printable(Path_Directory)+"/)", List.empty()?"nothing listed":"listed "+Printable(List[0].To_UTF8()));
        }
    }

    // Without Unicode on Windows, the same with the bytes of the code page given as they are, listed as UTF-8 text
    #if defined(_WIN32) && !defined(UNICODE)
    {
        bool Directory_ASCII=true; // The name with the directory must be bytes of the code page
        for (size_t Pos=0; Pos<Names_Directory.size(); Pos++)
            if ((unsigned char)Names_Directory[Pos]>=0x80)
                Directory_ASCII=false;
        for (size_t i=0; Directory_ASCII && i<Names_Size+Names_Multibyte_Size; i++)
        {
            std::string Name(i<Names_Size?Names[i]:Names_Multibyte[i-Names_Size]);
            std::string Bytes;
            if (!Reference_Encode(UTF8_To_Wide(Name), Bytes) || !From_UTF8(Bytes).empty())
                continue; // Not in the code page, or also UTF-8 text
            std::string Shown=Printable(Names_Directory+"/raw_"+Bytes+"/file.txt");
            Ztring Path(Names_Directory+"/raw_"+Bytes+"/file.txt");
            Check(FileName::Path_Get(Path)==Ztring(Names_Directory+"/raw_"+Bytes), "FileName::Path_Get("+Shown+"), bytes of the code page", "result is "+Printable(FileName::Path_Get(Path)));
            Check(FileName::Name_Get(Path)==Ztring("file"), "FileName::Name_Get("+Shown+"), bytes of the code page", "result is "+Printable(FileName::Name_Get(Path)));
            if (!Create_Raw_Directory(Names_Directory+"/raw_"+Name) || !Create_Raw(Names_Directory+"/raw_"+Name+"/file.txt"))
            {
                Check(false, "internal", "file not created");
                continue;
            }
            ZtringList List=Dir::GetAllFileNames(Ztring(Names_Directory+"/raw_"+Bytes));
            Check(List.size()==1 && Listed_Path(List[0])==Names_Directory+"/raw_"+Name+"/file.txt" && File::Exists(List[0]), "Dir::GetAllFileNames("+Printable(Names_Directory+"/raw_"+Bytes)+"), bytes of the code page", List.empty()?"nothing listed":"listed "+Printable(List[0].To_UTF8()));
        }
    }
    #endif

    // Without Unicode, bytes of the encoding of the locale (the code page on Windows) given as they are which are also valid UTF-8, so read as
    // UTF-8: the existing name is used, the name in this encoding if only it exists, e.g. the bytes C3 A9 are U+00C3 U+00A9 in Windows-1252 and
    // Latin-1 and U+00E9 in UTF-8, also ideographs of GBK and GB18030, UHC, Big5, EUC-JP, and cp932 (with a half-width katakana)
    #if !defined(UNICODE) && defined(HAVE_ENCODE_REFERENCE)
    {
        static const wchar_t* const Candidates[]=
        {
            L"\x00C3\x00A9",
            L"\x8059",
            L"\x4F3D",
            L"\x7652",
            L"\x81D3",
            L"\x71FF\xFF61",
        };
        bool Directory_ASCII=true; // The name with the directory must be bytes of the encoding
        for (size_t Pos=0; Pos<Names_Directory.size(); Pos++)
            if ((unsigned char)Names_Directory[Pos]>=0x80)
                Directory_ASCII=false;
        for (size_t i=0; Directory_ASCII && i<sizeof(Candidates)/sizeof(*Candidates); i++)
        {
            std::wstring Wide(Candidates[i]);
            std::string Bytes;
            std::string Name_UTF8=Ztring().From_Unicode(Wide);
            if (!Reference_Encode(Wide, Bytes) || From_UTF8(Bytes).empty() || Bytes==Name_UTF8)
                continue; // Not in this encoding, not valid UTF-8, or UTF-8
            std::string Shown=Printable(Bytes);
            #if defined(_WIN32)
                std::string Created=Name_UTF8; // UTF-8 for the wide functions
            #else
                std::string Created=Bytes;
            #endif
            if (!Create_Raw(Names_Directory+"/lookalike_"+Created) || !Create_Raw_Directory(Names_Directory+"/lookalike_dir_"+Created) || !Create_Raw(Names_Directory+"/lookalike_dir_"+Created+"/file.txt"))
            {
                Check(false, "internal", "file not created");
                continue;
            }
            Ztring Path(Names_Directory+"/lookalike_"+Bytes);
            Check(File::Exists(Path), "File::Exists("+Shown+"), bytes of the encoding which are also UTF-8", "not found");
            File F;
            Check(F.Open(Path) && F.Size_Get()==10, "File::Open("+Shown+"), bytes of the encoding which are also UTF-8", "not opened");
            Check(Dir::Exists(Ztring(Names_Directory+"/lookalike_dir_"+Bytes)), "Dir::Exists("+Shown+"), bytes of the encoding which are also UTF-8", "not found");
            ZtringList List=Dir::GetAllFileNames(Ztring(Names_Directory+"/lookalike_dir_"+Bytes));
            #if defined(__APPLE__) || defined(__ANDROID__) || defined(ZENLIB_FILENAME_UTF8)
                const std::string& Listed=Bytes; // UTF-8 names of the file system
            #else
                const std::string& Listed=Name_UTF8;
            #endif
            Check(List.size()==1 && List[0].find(Listed)!=std::string::npos && File::Exists(List[0]), "Dir::GetAllFileNames("+Shown+"), bytes of the encoding which are also UTF-8", List.empty()?"nothing listed":"listed "+Printable(List[0]));
        }
    }
    #endif

    // Conversions with another locale or code page: its encoding, also where wchar_t is not Unicode with it (e.g. macOS and FreeBSD, with iconv),
    // without the internal state of the C library (with some systems, e.g. GB18030 on FreeBSD and macOS, it kept the last bytes of a string which
    // is not valid in the encoding, so all the next strings were not converted)
    if (!IsUnicode)
    {
        if (!Encoding)
            Check(false, "internal", "no test text for the encoding "+CodeSet);
        else
            for (int Pass=0; Pass<2; Pass++)
            {
                Ztring Text=From_UTF8(Encoding->Text);
                std::string Local=Text.To_Local();
                Check(Local==Encoding->Local, "To_Local("+Printable(Encoding->Text)+") with "+CodeSet, "result is "+Printable(Local));
                Ztring Wide;
                Wide.From_Local(Encoding->Local);
                Check(Wide==Text, "From_Local("+Printable(Encoding->Local)+") with "+CodeSet, "result is "+Printable(Wide.To_UTF8()));

                // Then a byte which is not valid in the multibyte encodings: the text before it is decoded character per character
                std::string Local_Byte=Encoding->Local+std::string("\xFF");
                Wide.From_Local(Local_Byte.c_str());
                Check(Wide.find(Text)==0 && Wide.To_Unicode().size()==Text.To_Unicode().size()+1 && Wide.To_Local()==Local_Byte, "From_Local("+Printable(Local_Byte)+") with "+CodeSet, "result is "+Printable(Wide.To_UTF8()));
                Wide.From_Local(Encoding->Local+std::string("\xA5\xC6\xA5")); // Then the end of a character missing
            }
    }

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

    // Conversions and file names with another locale or code page (e.g. EUC-JP, GB18030 which encodes all characters, cp932): same
    // bytes as the C library or the system for the names they can encode, and the files must have these names (not on Windows, UTF-16 names)
    #if defined(HAVE_ENCODE_REFERENCE)
        if (!IsUnicode)
            for (size_t i=0; i<Names_Size+Names_Multibyte_Size; i++)
            {
                std::string Name(i<Names_Size?Names[i]:Names_Multibyte[i-Names_Size]);
                std::string Expected;
                if (!Reference_Encode(UTF8_To_Wide(Name), Expected))
                    continue; // Not in this encoding
                std::string Shown=Printable(Name);
                std::string Local=From_UTF8(Name).To_Local();
                Check(Local==Expected, "To_Local("+Shown+") with the locale", "result is "+Printable(Local)+" instead of "+Printable(Expected));
                Ztring Wide;
                Wide.From_Local(Expected.c_str());
                Check(Wide==From_UTF8(Name), "From_Local("+Printable(Expected)+")", "result is "+Printable(Wide.To_UTF8()));
                #if !defined(_WIN32) && !defined(__APPLE__) && !defined(__ANDROID__) && !defined(ZENLIB_FILENAME_UTF8)
                    std::string Path=Directory+"/locale_"+Expected;
                    File F;
                    Check(F.Create(From_UTF8(Directory+"/locale_"+Name)) && Exists_Raw(Path), "File::Create("+Shown+") with the locale", "file not created with the name in the encoding of the locale");
                    F.Close();
                    Check(FileName_From_Native(Path.c_str())==From_UTF8(Directory+"/locale_"+Name), "FileName_From_Native("+Printable(Expected)+")", "wrong name");
                #endif
            }
    #endif

    // Path separators: FileName_PathSeparator, also "/" and "\" on Windows
    {
        #if defined(_WIN32)
            static const char* const Paths[]={"C:\\dir\\name.ext", "C:/dir/name.ext", "C:\\dir/name.ext", "C:/dir\\name.ext"};
            static const char* const Paths_Path[]={"C:\\dir", "C:/dir", "C:\\dir", "C:/dir"};
        #else
            static const char* const Paths[]={"/dir/name.ext"};
            static const char* const Paths_Path[]={"/dir"};
        #endif
        for (size_t i=0; i<sizeof(Paths)/sizeof(*Paths); i++)
        {
            Ztring Path; Path.From_UTF8(Paths[i]);
            Ztring Path_Path; Path_Path.From_UTF8(Paths_Path[i]);
            Check(FileName::Name_Get(Path)==__T("name") && FileName::Extension_Get(Path)==__T("ext") && FileName::Path_Get(Path)==Path_Path, std::string("FileName functions(")+Paths[i]+")", "results are "+FileName::Path_Get(Path).To_UTF8()+", "+FileName::Name_Get(Path).To_UTF8()+", "+FileName::Extension_Get(Path).To_UTF8());
        }
    }

    // ASCII text is the same with all locales
    {
        Ztring Wide;
        Wide.From_Local("ascii.bin");
        Check(Wide==__T("ascii.bin") && Wide.To_Local()=="ascii.bin", "From_Local(ascii.bin)", "result is "+Printable(Wide.To_UTF8())+", back "+Printable(Wide.To_Local()));
        Check(FileName_To_Native(FileName_From_Native("ascii.bin"))=="ascii.bin", "FileName_To_Native(FileName_From_Native(ascii.bin))", "result is "+Printable(FileName_To_Native(FileName_From_Native("ascii.bin"))));
    }

    // Strings which are not valid in the encoding of the locale (UTF-8 with an ASCII locale), with all the locales: same bytes back, not an empty
    // string, not the UTF-8 string of the same characters, and the valid part is decoded, as Python with "surrogateescape"; also as file names,
    // except if they are converted from a locale which is neither ASCII nor UTF-8 (the bytes may depend on the existing files)
    #if !defined(_WIN32)
    {
        for (size_t i=0; i<Names_NotUTF8_Size; i++)
            Check_Bytes(Names_NotUTF8[i], !Names_AreConverted);
        for (size_t i=0; i<Strings_NotUTF8_Size; i++)
            Check_Bytes(Strings_NotUTF8[i], !Names_AreConverted);
        for (size_t i=0; i<Strings_Multibyte_Size; i++)
            Check_Bytes(Strings_Multibyte[i], !Names_AreConverted);
        Check_Bytes(Name_Latin1, !Names_AreConverted);
        if (IsUnicode)
        {
            Ztring Wide;
            Wide.From_Local(Name_Latin1);
            Check(Wide!=From_UTF8(Names[0]), "From_Local("+Printable(Name_Latin1)+")", "same string as the UTF-8 name");
            Wide.From_Local("caf\xC3\xA9\xE9");
            Check(Wide.find(From_UTF8(Names[0]))==0 && Wide.To_Unicode().size()==From_UTF8(Names[0]).To_Unicode().size()+1, "From_Local(valid text and a byte which is not valid)", "result is "+Printable(Wide.To_Local()));
        }

        // Name of the wide API with a byte kept, e.g. from Python (os.fsdecode() keeps the bytes as U+DC80 to U+DCFF with all the locales)
        std::wstring Wide_Kept(L"caf");
        Wide_Kept+=(wchar_t)0xDCE9;
        std::string Native=FileName_To_Native(Ztring().From_Unicode(Wide_Kept));
        Check(Native==Name_Latin1, "FileName_To_Native(caf\\uDCE9)", "result is "+Printable(Native));
    }
    #endif

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

    // Text files of ZtringListListF (e.g. configuration, translations) are written in UTF-8: read as UTF-8 with all the locales, also without
    // Unicode, else (not valid UTF-8) in the encoding of the locale
    for (int Pass=0; Pass<2; Pass++)
    {
        std::string Content=Pass?Name_Latin1:Names[1];
        std::string Path=Directory+(Pass?"/text_local.csv":"/text_utf8.csv");
        FILE* F=Open_Raw(Path);
        bool Written=F && fwrite(Content.c_str(), 1, Content.size(), F)==Content.size();
        if (F && fclose(F))
            Written=false;
        if (!Written)
        {
            Check(false, "internal", "file not created");
            continue;
        }
        ZtringListListF List;
        List.Load(From_UTF8(Path));
        Ztring Value=List.Read(0, 0);
        Ztring Expected=Pass?Ztring().From_Local(Name_Latin1):From_UTF8(Names[1]);
        Check(Value==Expected, "ZtringListListF::Load("+Printable(Content)+")", "result is "+Printable(Value.To_UTF8()));
    }

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

    // Name which is not valid UTF-8, next to the UTF-8 name of the same characters: the file with this name is used, with all the locales
    // (these names can't be created on macOS, the UTF-8 name is used if names are converted from a locale which is neither ASCII nor UTF-8)
    if (!Names_AreConverted)
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
    std::set<std::string> Expected_All=Expected; // With the names which are not valid UTF-8 (not where names are converted, see the volume below)
    size_t NotUTF8_Count=0;
    for (size_t i=0; i<Names_NotUTF8_Size; i++)
        if (!Names_AreConverted && Create_Raw(List_Path+"/"+Names_NotUTF8[i])) // Not on macOS and Windows
        {
            NotUTF8_Count++;
            Expected_All.insert(Names_NotUTF8[i]);
        }
    std::string SubDirectory_NotUTF8=std::string("sub_")+Name_Latin1;
    if (!Names_AreConverted && Create_Raw_Directory(List_Path+"/"+SubDirectory_NotUTF8) && Create_Raw(List_Path+"/"+SubDirectory_NotUTF8+"/inner.bin"))
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
        #if defined(_WIN32)
            if (IsASCII)
                Check(Listed==Expected_All, "Dir::GetAllFileNames, all names", "listed:"+Listed_Shown);
            else if (FileNames_IsUnicode)
                Check(Listed==Expected, "Dir::GetAllFileNames, valid UTF-8 names", "listed:"+Listed_Shown);
            else if (IsLatin1)
                Check(List.size()==Expected.size()+NotUTF8_Count, "Dir::GetAllFileNames, all names", "listed:"+Listed_Shown); // Each byte is a character
            else
                Check(Listed.count("ascii.bin")==1, "Dir::GetAllFileNames, names which can be decoded", "listed:"+Listed_Shown); // e.g. cp932 without Unicode
        #else
            Check(Listed==Expected_All, "Dir::GetAllFileNames, all names", "listed:"+Listed_Shown); // With all the locales, the names which are not valid kept
            (void)NotUTF8_Count;
        #endif

        // Same names with a path separator at the end of the directory name, without a second separator
        ZtringList List_Separator=Dir::GetAllFileNames(From_UTF8(List_Path+"/"), (Dir::dirlist_t)(Dir::Include_Files|Dir::Parse_SubDirs));
        std::set<std::string> Listed_Separator;
        for (size_t i=0; i<List_Separator.size(); i++)
        {
            std::string Path=Listed_Path(List_Separator[i]);
            if (!Path.compare(0, List_Path.size()+1, List_Path+"/") && Path.compare(List_Path.size()+1, 1, "/"))
                Listed_Separator.insert(Path.substr(List_Path.size()+1));
        }
        Check(Listed_Separator==Listed, "Dir::GetAllFileNames, directory name with a path separator at the end", "different names or a second separator");
    }

    // Network volumes (e.g. NFS, AFS) keep the bytes of names created by other systems with another encoding, and a locale which is neither ASCII
    // nor UTF-8 is used where names are converted to UTF-8 (e.g. macOS): names typed in this encoding are found with the UTF-8 name or with the
    // bytes of this encoding, for each part of the path, new files get the UTF-8 name, and all the listed names give their file (only where
    // such names can be created, e.g. with ZENLIB_FILENAME_UTF8 on Linux, not on macOS, but also with the names which are valid UTF-8)
    if (Names_AreConverted && Encoding)
    {
        std::string Local(Encoding->Local), UTF8(Encoding->Text);
        std::string Volume=Directory+"/volume";
        std::vector<volume_file> Files;
        // (not the UTF-8 name and the name in the other encoding of the same characters in the same directory, the UTF-8 one is used)
        Volume_Add(Files, Volume+"/utf8_"+UTF8, 1, Volume+"/utf8_"+Local); // e.g. created on macOS
        Volume_Add(Files, Volume+"/local_"+Local, 2, Volume+"/local_"+Local); // e.g. created on Linux with this encoding
        Volume_Add(Files, Volume+"/dir1_"+Local+"/utf8_"+UTF8, 3, Volume+"/dir1_"+Local+"/utf8_"+Local);
        Volume_Add(Files, Volume+"/dir2_"+UTF8+"/local_"+Local, 4, Volume+"/dir2_"+Local+"/local_"+Local);
        if (Encoding->Text_ValidUTF8) // Name in this encoding which is also valid UTF-8, of other characters
            Volume_Add(Files, Volume+"/valid_"+Encoding->Text_ValidUTF8, 5, Volume+"/valid_"+Encoding->Local_ValidUTF8);
        if (!Create_Raw_Directory(Volume))
            Check(false, "internal", "can't create "+Printable(Volume));
        else
        {
            Create_Raw_Directory(Volume+"/dir1_"+Local);
            Create_Raw_Directory(Volume+"/dir2_"+UTF8);
            std::map<std::string, size_t> Created; // Size of the files
            for (size_t i=0; i<Files.size(); i++)
            {
                if (!Create_Raw(Files[i].Raw, Files[i].Size))
                    continue; // Not valid UTF-8, on macOS
                Created[Files[i].Raw]=Files[i].Size;
                std::string Shown=Printable(Files[i].Typed.substr(Volume.size()+1));
                File F;
                Check(File::Size_Get(Typed(Files[i].Typed))==Files[i].Size && F.Open(Typed(Files[i].Typed)) && F.Size_Get()==Files[i].Size, "File::Open("+Shown+") on a volume with names in another encoding", "file not found or another file");
            }

            // New file in a directory which exists with the bytes of this encoding: the UTF-8 name in this directory
            if (Exists_Raw(Volume+"/dir1_"+Local, true))
            {
                File F;
                bool IsCreated=F.Create(Typed(Volume+"/dir1_"+Local+"/new_"+Local));
                F.Close();
                Check(IsCreated && Exists_Raw(Volume+"/dir1_"+Local+"/new_"+UTF8), "File::Create(new file) in a directory with a name in another encoding", "not created with the UTF-8 name in this directory");
                if (IsCreated)
                    Created[Volume+"/dir1_"+Local+"/new_"+UTF8]=0;
            }

            // Listing: each name gives its file
            ZtringList List=Dir::GetAllFileNames(From_UTF8(Volume), (Dir::dirlist_t)(Dir::Include_Files|Dir::Parse_SubDirs));
            std::string Listed_Shown;
            bool IsOK=List.size()==Created.size();
            for (size_t i=0; i<List.size(); i++)
            {
                std::string Native=FileName_To_Native(List[i]);
                Listed_Shown+=" "+Printable(Native);
                std::map<std::string, size_t>::iterator Item=Created.find(Native);
                if (Item==Created.end() || File::Size_Get(List[i])!=Item->second)
                    IsOK=false;
            }
            Check(IsOK, "Dir::GetAllFileNames, volume with names in another encoding", "listed:"+Listed_Shown);
        }
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
                Check(List.size()==1 && FileName_To_Native(List[0])==Path, "Dir::GetAllFileNames, directory which can't be read", List.empty()?"nothing listed":"listed "+Printable(List[0].To_UTF8()));
            }
        }
    #endif //!defined(_WIN32) && (!defined(__ANDROID_API__) || __ANDROID_API__ >= 28)

    if (!Status)
        printf("OK: %u checks\n", (unsigned)Checks);
    return Status;
}

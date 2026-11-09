/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// More methods for std::(w)string
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
    #include <wx/strconv.h>
    #include <wx/datetime.h>
#else //ZENLIB_USEWX
    #ifdef ZENLIB_STANDARD
        #undef WINDOWS
    #endif
    #ifdef WINDOWS
        #undef __TEXT
        #include <windows.h>
        #ifndef WINDOWS_GAMES
            #include <tchar.h>
        #endif
    #elif !defined(__ANDROID_API__) || __ANDROID_API__ >= 26
        #include <langinfo.h>
    #endif
#endif //ZENLIB_USEWX
#ifdef __MINGW32__
    #include <windows.h>
#endif //__MINGW32__
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <cstring>
#include <ctime>
#include <cwchar>
#include "ZenLib/OS_Utils.h"
#include "ZenLib/File.h"
using namespace std;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//Text in the encoding of the locale on Unix-like systems (see From_Local()):
//bytes kept with Unicode, and iconv of the system where wchar_t is not Unicode
//with some locales (e.g. macOS, FreeBSD) or if ZENLIB_ICONV is defined (e.g.
//tests), also for the file names of macOS without Unicode (see OS_Utils.cpp)
#if !defined(ZENLIB_USEWX) && !defined(WINDOWS)
    #if defined(_UNICODE)
        #define ZENLIB_LOCAL_KEEPBYTES
        #include <climits>
    #endif
    #if (defined(_UNICODE) && (defined(ZENLIB_ICONV) || !(defined(__STDC_ISO_10646__) || defined(__GLIBC__) || defined(__linux__) || defined(__EMSCRIPTEN__)))) \
     || (!defined(_UNICODE) && (defined(__APPLE__) || defined(ZENLIB_FILENAME_UTF8)))
        #define ZENLIB_LOCAL_ICONV
        #include <cerrno>
        #include <dlfcn.h>
        #include <string>
    #endif
#endif
//---------------------------------------------------------------------------

//MSVC2005 : "deprecated" warning (replacement functions are not in MinGW32 or Borland!)
#if defined(_MSC_VER) && _MSC_VER >= 1400
    #pragma warning(disable : 4996)
#endif

//---------------------------------------------------------------------------

namespace ZenLib
{

int16u Ztring_ISO_8859_2[96]=
{
    0x00A0,
    0x0104,
    0x02D8,
    0x0141,
    0x00A4,
    0x013D,
    0x015A,
    0x00A7,
    0x00A8,
    0x0160,
    0x015E,
    0x0164,
    0x0179,
    0x00AD,
    0x017D,
    0x017B,
    0x00B0,
    0x0105,
    0x02DB,
    0x0142,
    0x00B4,
    0x013E,
    0x015B,
    0x02C7,
    0x00B8,
    0x0161,
    0x015F,
    0x0165,
    0x017A,
    0x02DD,
    0x017E,
    0x017C,
    0x0154,
    0x00C1,
    0x00C2,
    0x0102,
    0x00C4,
    0x0139,
    0x0106,
    0x00C7,
    0x010C,
    0x00C9,
    0x0118,
    0x00CB,
    0x011A,
    0x00CD,
    0x00CE,
    0x010E,
    0x0110,
    0x0143,
    0x0147,
    0x00D3,
    0x00D4,
    0x0150,
    0x00D6,
    0x00D7,
    0x0158,
    0x016E,
    0x00DA,
    0x0170,
    0x00DC,
    0x00DD,
    0x0162,
    0x00DF,
    0x0155,
    0x00E1,
    0x00E2,
    0x0103,
    0x00E4,
    0x013A,
    0x0107,
    0x00E7,
    0x010D,
    0x00E9,
    0x0119,
    0x00EB,
    0x011B,
    0x00ED,
    0x00EE,
    0x010F,
    0x0111,
    0x0144,
    0x0148,
    0x00F3,
    0x00F4,
    0x0151,
    0x00F6,
    0x00F7,
    0x0159,
    0x016F,
    0x00FA,
    0x0171,
    0x00FC,
    0x00FD,
    0x0163,
    0x02D9,
};

//---------------------------------------------------------------------------
Ztring EmptyZtring;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(STREAM_MISSING)
    #if defined (_UNICODE)
        #if defined (MACOS) || defined (MACOSX)
            #define _tnprintf swprintf
        #else
            #define _tnprintf snwprintf
        #endif
    #else
        #define _tnprintf snprintf
    #endif
#else
    typedef basic_stringstream<Char>  tStringStream;
    typedef basic_istringstream<Char> tiStringStream;
    typedef basic_ostringstream<Char> toStringStream;
#endif
//---------------------------------------------------------------------------

//***************************************************************************
// Operators
//***************************************************************************

Char &Ztring::operator() (size_type Pos)
{
    if (Pos>size())
        resize(Pos);
    return operator[] (Pos);
}

//***************************************************************************
// Assign
//***************************************************************************

bool Ztring::Assign_FromFile (const Ztring &FileName)
{
    File F;
    if (!F.Open(FileName))
        return false;
    int64u F_Size=F.Size_Get();
    if (F_Size>((size_t)-1)-1)
        return false;

    //Creating buffer
    int8u* Buffer=new int8u[(size_t)F_Size+1];
    size_t Buffer_Offset=0;

    //Reading the file
    while(Buffer_Offset<F_Size)
    {
        size_t BytesRead=F.Read(Buffer+Buffer_Offset, (size_t)F_Size-Buffer_Offset);
        if (BytesRead==0)
            break; //Read is finished
        Buffer_Offset+=BytesRead;
    }
    if (Buffer_Offset<F_Size)
    {
        delete[] Buffer;
        return false;
    }
    Buffer[Buffer_Offset]='\0';

    //Filling
    assign((const Char*)Buffer);
    delete[] Buffer;

    return true;
}

//***************************************************************************
// Conversions
//***************************************************************************

#if !defined(ZENLIB_USEWX) && (!defined(WINDOWS) || !defined(_UNICODE))
static size_t UTF8_Char_Size(const int8u* Z);

//A surrogate pair of UTF-16 is one character where wchar_t is 32-bit
static void UTF16_Append(std::wstring& Wide, int16u Value)
{
    if (sizeof(wchar_t)==4 && (Value&0xFC00)==0xDC00 && !Wide.empty() && (Wide[Wide.size()-1]&0xFC00)==0xD800)
        Wide[Wide.size()-1]=(wchar_t)(0x10000+((Wide[Wide.size()-1]&0x3FF)<<10)+(Value&0x3FF));
    else
        Wide+=(wchar_t)Value;
}
#endif

Ztring& Ztring::From_Unicode (const wchar_t S)
{
    #ifdef _UNICODE
        append(1, S);
    #else
        From_Unicode(&S, 1);
    #endif
    return *this;
}

Ztring& Ztring::From_Unicode (const wchar_t* S)
{
    if (S==NULL)
        return *this;

    #ifdef _UNICODE
        assign(S);
    #else
        #ifdef ZENLIB_USEWX
            size_type OK=wxConvCurrent->WC2MB(NULL, S, 0);
            if (OK!=0 && OK!=Error)
                assign(wxConvCurrent->cWC2MB(S));
        #else //ZENLIB_USEWX
            #ifdef WINDOWS
                int Size=WideCharToMultiByte(CP_UTF8, 0, S, -1, NULL, 0, NULL, NULL);
                if (Size!=0)
                {
                    char* AnsiString=new char[Size+1];
                    WideCharToMultiByte(CP_UTF8, 0, S, -1, AnsiString, Size, NULL, NULL);
                    AnsiString[Size]='\0';
                    assign (AnsiString);
                    delete[] AnsiString;
                }
                else
                    clear();
            #else //WINDOWS
                //UTF-8, as From_UTF8() and on Windows, whatever the locale
                clear();
                for (; *S; S++)
                {
                    int32u Value=(int32u)*S;
                    if (Value<0x80)
                        append(1, (char)Value);
                    else if (Value<0x800)
                    {
                        append(1, (char)(0xC0|(Value>>6)));
                        append(1, (char)(0x80|(Value&0x3F)));
                    }
                    else if (Value<0x10000)
                    {
                        append(1, (char)(0xE0|(Value>>12)));
                        append(1, (char)(0x80|((Value>>6)&0x3F)));
                        append(1, (char)(0x80|(Value&0x3F)));
                    }
                    else
                    {
                        append(1, (char)(0xF0|(Value>>18)));
                        append(1, (char)(0x80|((Value>>12)&0x3F)));
                        append(1, (char)(0x80|((Value>>6)&0x3F)));
                        append(1, (char)(0x80|(Value&0x3F)));
                    }
                }
            #endif
        #endif //ZENLIB_USEWX
    #endif
    return *this;
}

Ztring& Ztring::From_Unicode (const wchar_t *S, size_type Start, size_type Length)
{
    if (S==NULL)
        return *this;

    if (Length==Error)
        Length=wcslen(S+Start);
    wchar_t* Temp=new wchar_t[Length+1];
    wcsncpy (Temp, S+Start, Length);
    Temp[Length]=__T('\0');

    From_Unicode(Temp);
    delete[] Temp; //Temp=NULL;
    return *this;
}

Ztring& Ztring::From_UTF8 (const char* S)
{
    if (S==NULL)
        return *this;

    #ifdef ZENLIB_USEWX
        size_type OK=wxConvUTF8.MB2WC(NULL, S, 0);
        if (OK!=0 && OK!=Error)
            #ifdef _UNICODE
                assign(wxConvUTF8.cMB2WC(S).data());
            #else
                assign(wxConvCurrent->cWC2MB(wxConvUTF8.cMB2WC(S)));
            #endif
    #else //ZENLIB_USEWX
        #ifdef _UNICODE
            // Don't use MultiByteToWideChar(), some characters are not well decoded
            clear();
            const int8u* Z=(const int8u*)S;
            while (*Z) //0 is end
            {
                //1 byte
                if (*Z<0x80)
                {
                    operator += ((wchar_t)(*Z));
                    Z++;
                }
                //2 bytes
                else if ((*Z&0xE0)==0xC0)
                {
                    if ((*(Z+1)&0xC0)==0x80)
                    {
                        operator += ((((wchar_t)(*Z&0x1F))<<6)|(*(Z+1)&0x3F));
                        Z+=2;
                    }
                    else
                    {
                        clear();
                        return *this; //Bad character
                    }
                }
                //3 bytes
                else if ((*Z&0xF0)==0xE0)
                {
                    if ((*(Z+1)&0xC0)==0x80 && (*(Z+2)&0xC0)==0x80)
                    {
                        operator += ((((wchar_t)(*Z&0x0F))<<12)|((*(Z+1)&0x3F)<<6)|(*(Z+2)&0x3F));
                        Z+=3;
                    }
                    else
                    {
                        clear();
                        return *this; //Bad character
                    }
                }
                //4 bytes
                else if ((*Z&0xF8)==0xF0)
                {
                    if ((*(Z+1)&0xC0)==0x80 && (*(Z+2)&0xC0)==0x80 && (*(Z+3)&0xC0)==0x80)
                    {
                        #if defined(_MSC_VER)
                            #pragma warning(push)
                            #pragma warning(disable:4127)
                        #endif //defined(_MSC_VER)
                        if (sizeof(wchar_t) == 2)
                        #if defined(_MSC_VER)
                            #pragma warning(pop)
                        #endif //defined(_MSC_VER)
                        {
                            int32u Value = ((((int32u)(*Z&0x0F))<<18)|((*(Z+1)&0x3F)<<12)|((*(Z+2)&0x3F)<<6)|(*(Z+3)&0x3F));
                            operator += (0xD800|((Value>>10)-0x40));
                            operator += (0xDC00| (Value&0x3FF));
                        }
                        else
                            operator += ((((wchar_t)(*Z&0x0F))<<18)|((*(Z+1)&0x3F)<<12)|((*(Z+2)&0x3F)<<6)|(*(Z+3)&0x3F));
                        Z+=4;
                    }
                    else
                    {
                        clear();
                        return *this; //Bad character
                    }
                }
                else
                {
                    clear();
                    return *this; //Bad character
                }
            }
        #else
            //Text is UTF-8 without Unicode, it must be valid as it is the way to detect another encoding
            for (const int8u* Z=(const int8u*)S; *Z; )
            {
                size_t Size=UTF8_Char_Size(Z);
                if (!Size)
                {
                    clear();
                    return *this; //Bad character
                }
                Z+=Size;
            }
            assign(S);
        #endif
    #endif //ZENLIB_USEWX
    return *this;
}

Ztring& Ztring::From_UTF8 (const char* S, size_type Start, size_type Length)
{
    if (S==NULL)
        return *this;

    if (Length==Error)
        Length=strlen(S+Start);
    char* Temp=new char[Length+1];
    strncpy (Temp, S+Start, Length);
    Temp[Length]='\0';

    From_UTF8(Temp);
    delete[] Temp; //Temp=NULL;
    return *this;
}

Ztring& Ztring::From_UTF16 (const char* S)
{
    if (S==NULL)
        return *this;

         if ((unsigned char)S[0]==(unsigned char)0xFF && (unsigned char)S[1]==(unsigned char)0xFE)
        return From_UTF16LE(S+2);
    else if ((unsigned char)S[0]==(unsigned char)0xFE && (unsigned char)S[1]==(unsigned char)0xFF)
        return From_UTF16BE(S+2);
    else if ((unsigned char)S[0]==(unsigned char)0x00 && (unsigned char)S[1]==(unsigned char)0x00)
    {
        clear(); //No begin!
        return *this;
    }
    else
        return From_UTF16LE(S); //Not sure, default
}

Ztring& Ztring::From_UTF16 (const char* S, size_type Start, size_type Length)
{
    if (S==NULL)
        return *this;

    if (Length<2)
        return *this;

         if ((unsigned char)S[0]==(unsigned char)0xFF && (unsigned char)S[1]==(unsigned char)0xFE)
        return From_UTF16LE(S+2, Start, Length-2);
    else if ((unsigned char)S[0]==(unsigned char)0xFE && (unsigned char)S[1]==(unsigned char)0xFF)
        return From_UTF16BE(S+2, Start, Length-2);
    else if ((unsigned char)S[0]==(unsigned char)0x00 && (unsigned char)S[1]==(unsigned char)0x00)
    {
        clear(); //No begin!
        return *this;
    }
    else
        return From_UTF16LE(S, Start, Length); //Not sure, default
}

Ztring& Ztring::From_UTF16BE (const char* S)
{
    if (S==NULL)
        return *this;

    #ifdef ZENLIB_USEWX
        //clear(); return *this;
        wxMBConvUTF16BE wxConvUTF16BE;
        size_type OK=wxConvUTF16BE.MB2WC(NULL, S, 0);
        if (OK!=0 && OK!=Error)
            #ifdef _UNICODE
                assign(wxConvUTF16BE.cMB2WC(S).data());
            #else
                assign(wxConvCurrent->cWC2MB(wxConvUTF16BE.cMB2WC(S)));
            #endif
    #else //ZENLIB_USEWX
        #if defined(WINDOWS) && defined(_UNICODE)
            clear();
            const wchar_t* SW=(const wchar_t*)S;
            size_t Pos=0;
            while (SW[Pos]!=__T('\0'))
            {
                Char Temp=(Char)(((SW[Pos]&0xFF00)>>8)+((SW[Pos]&0x00FF)<<8));
                append(1, Temp);
                Pos++;
            }
        #else //defined(WINDOWS) && defined(_UNICODE)
            std::wstring Wide;
            while (S[0]!=0 || S[1]!=0)
            {
                UTF16_Append(Wide, BigEndian2int16u(S));
                S+=2;
            }
            From_Unicode(Wide.c_str());
        #endif
    #endif //ZENLIB_USEWX
    return *this;
}

Ztring& Ztring::From_UTF16BE (const char* S, size_type Start, size_type Length)
{
    if (S==NULL)
        return *this;

    if (Length==Error)
    {
        Length=0;
        while(S[Length]!=0x0000)
            Length++;
    }
    else
        Length&=(size_t)-2; //odd number

    char* Temp=new char[Length+2];
    memcpy (Temp, S+Start, Length);
    Temp[Length+0]=0x00;
    Temp[Length+1]=0x00;
    reserve(Length);
    From_UTF16BE(Temp);
    delete[] Temp; //Temp=NULL;
    return *this;
}

Ztring& Ztring::From_UTF16LE (const char* S)
{
    if (S==NULL)
        return *this;

    #ifdef ZENLIB_USEWX
        //clear(); return *this;
        wxMBConvUTF16LE wxConvUTF16LE;
        size_type OK=wxConvUTF16LE.MB2WC(NULL, S, 0);
        if (OK!=0 && OK!=Error)
            #ifdef _UNICODE
                assign(wxConvUTF16LE.cMB2WC(S).data());
            #else
                assign(wxConvCurrent->cWC2MB(wxConvUTF16LE.cMB2WC(S)));
            #endif
    #else //ZENLIB_USEWX
        #if defined(WINDOWS) && defined(_UNICODE)
            const wchar_t* SW=(const wchar_t*)S;
            assign(SW);
        #else //defined(WINDOWS) && defined(_UNICODE)
            std::wstring Wide;
            while (S[0]!=0 || S[1]!=0)
            {
                UTF16_Append(Wide, LittleEndian2int16u(S));
                S+=2;
            }
            From_Unicode(Wide.c_str());
        #endif
    #endif //ZENLIB_USEWX
    return *this;
}

Ztring& Ztring::From_UTF16LE (const char* S, size_type Start, size_type Length)
{
    if (S==NULL)
        return *this;

    if (Length==Error)
    {
        Length=0;
        while(S[Length]!=0x0000)
            Length+=2;
    }
    else
        Length&=(size_t)-2; //odd number

    char* Temp=new char[Length+2];
    memcpy (Temp, S+Start, Length);
    Temp[Length+0]=0x00;
    Temp[Length+1]=0x00;
    From_UTF16LE(Temp);
    delete[] Temp; //Temp=NULL;
    return *this;
}

//---------------------------------------------------------------------------
//Text in the encoding of the locale, with Unicode on Unix-like systems:
//An ASCII locale such as "C", which programs get when they don't call
//setlocale() or when the environment sets no locale, can't encode non-ASCII
//characters, or converts them byte per byte, so UTF-8 is used instead, as
//Python does in its UTF-8 Mode. With all locales, bytes which are not valid in
//the encoding, e.g. of a Latin-1 file name with a UTF-8 locale, are kept as
//U+DC80 to U+DCFF, which are not characters, so a name gives the same bytes
//back, as Python does with "surrogateescape".
//Where wchar_t is not Unicode with some locales (e.g. EUC-JP, GB18030 or
//ISO-8859-15 on macOS and FreeBSD), iconv of the system is used instead of the
//C library, it is loaded at run time and only for a string which is not ASCII
//with such a locale; if it is not available, the C library is used.
#if defined(ZENLIB_LOCAL_KEEPBYTES) || defined(ZENLIB_LOCAL_ICONV)
//Encoding of the locale if it is neither ASCII nor UTF-8, else NULL, also used by OS_Utils.cpp
//(Android has no other encoding, and no nl_langinfo() before API level 26)
const char* Local_Other()
{
    #if defined(__ANDROID__)
        return NULL;
    #else
        const char* CodeSet=nl_langinfo(CODESET);
        if (!CodeSet || !*CodeSet)
            return NULL;
        static const char* const Names[]={"ANSI_X3.4-1968", "ASCII", "US-ASCII", "646", "UTF-8", "UTF8", "utf8", "utf-8"};
        for (size_t Pos=0; Pos<sizeof(Names)/sizeof(*Names); Pos++)
            if (!strcmp(CodeSet, Names[Pos]))
                return NULL;
        return CodeSet;
    #endif
}
#endif

#if !defined(ZENLIB_USEWX) && (!defined(WINDOWS) || !defined(_UNICODE))
//Count of bytes of the valid UTF-8 character at Z, 0 if there is none
static size_t UTF8_Char_Size(const int8u* Z)
{
    if (*Z<0x80)
        return 1;

    size_t Size;
    int8u Second_Min=0x80, Second_Max=0xBF;
    if (*Z>=0xC2 && *Z<=0xDF)
        Size=2;
    else if (*Z>=0xE0 && *Z<=0xEF)
    {
        Size=3;
        if (*Z==0xE0)
            Second_Min=0xA0; //Overlong
        else if (*Z==0xED)
            Second_Max=0x9F; //UTF-16 surrogate
    }
    else if (*Z>=0xF0 && *Z<=0xF4)
    {
        Size=4;
        if (*Z==0xF0)
            Second_Min=0x90; //Overlong
        else if (*Z==0xF4)
            Second_Max=0x8F; //After U+10FFFF
    }
    else
        return 0;

    if (Z[1]<Second_Min || Z[1]>Second_Max)
        return 0;
    for (size_t Pos=2; Pos<Size; Pos++)
        if ((Z[Pos]&0xC0)!=0x80)
            return 0;
    return Size;
}
#endif

#if defined(ZENLIB_LOCAL_KEEPBYTES)
static bool IsASCII(const char* S, size_t Size)
{
    for (size_t Pos=0; Pos<Size; Pos++)
        if ((int8u)S[Pos]>=0x80)
            return false;
    return true;
}

static bool IsASCII(const Ztring& S)
{
    for (size_t Pos=0; Pos<S.size(); Pos++)
        if ((int32u)S[Pos]>=0x80)
            return false;
    return true;
}

//Byte kept as U+DC80 to U+DCFF, not the second part of a UTF-16 surrogate pair
static bool IsKeptByte(const Ztring& S, size_t Pos)
{
    return S[Pos]>=0xDC80 && S[Pos]<=0xDCFF && (!Pos || (S[Pos-1]&0xFC00)!=0xD800);
}

//UTF-8 with the bytes which are not valid kept, also used by OS_Utils.cpp
Ztring& From_UTF8_KeepBytes(Ztring& ToReturn, const char* S)
{
    ToReturn.clear();
    const char* Valid=S;
    while (*S)
    {
        size_t Size=UTF8_Char_Size((const int8u*)S);
        if (Size)
        {
            S+=Size;
            continue;
        }
        ToReturn+=Ztring().From_UTF8(Valid, 0, S-Valid);
        ToReturn+=(wchar_t)(0xDC00+*(const int8u*)S);
        Valid=++S;
    }
    ToReturn+=Ztring().From_UTF8(Valid);
    return ToReturn;
}

std::string To_UTF8_KeepBytes(const Ztring& S)
{
    std::string ToReturn;
    size_t Valid=0;
    for (size_t Pos=0; Pos<S.size(); Pos++)
        if (IsKeptByte(S, Pos))
        {
            ToReturn+=Ztring(S, Valid, Pos-Valid).To_UTF8();
            ToReturn+=(char)(S[Pos]-0xDC00);
            Valid=Pos+1;
        }
    ToReturn+=Ztring(S, Valid).To_UTF8();
    return ToReturn;
}

//Encoding of the locale with the C library, character per character: bytes which are not valid, or which don't give the same bytes
//back (e.g. a character with two encodings, the name could not be opened), kept
//Conversion state of each call: the one of the C library is undefined after a string which is not valid, and some implementations
//then keep its last bytes (e.g. GB18030 on FreeBSD and macOS), so the next strings were not converted
static void From_Local_C(Ztring& ToReturn, const char* S, size_t Size)
{
    ToReturn.clear();
    mbstate_t State;
    memset(&State, 0, sizeof(State));
    for (size_t Pos=0; Pos<Size;)
    {
        if ((int8u)S[Pos]<0x80)
        {
            ToReturn+=(wchar_t)S[Pos];
            Pos++;
            continue;
        }
        wchar_t Char;
        size_t Char_Size=mbrtowc(&Char, S+Pos, Size-Pos, &State);
        if (Char_Size && Char_Size<=Size-Pos && (Char<0xD800 || Char>0xDFFF))
        {
            char Back[MB_LEN_MAX];
            mbstate_t Back_State;
            memset(&Back_State, 0, sizeof(Back_State));
            if (wcrtomb(Back, Char, &Back_State)==Char_Size && !memcmp(Back, S+Pos, Char_Size))
            {
                ToReturn+=Char;
                Pos+=Char_Size;
                continue;
            }
        }
        ToReturn+=(wchar_t)(0xDC00+(int8u)S[Pos]); //Then the initial state
        Pos++;
        memset(&State, 0, sizeof(State));
    }
}

//Encoding of the locale with the C library: the bytes kept by From_Local() back, "?" for the characters which can't be encoded
static void To_Local_C(const Ztring& S, std::string& ToReturn)
{
    ToReturn.clear();
    mbstate_t State;
    memset(&State, 0, sizeof(State));
    char Buffer[MB_LEN_MAX];
    for (size_t Pos=0; Pos<S.size(); Pos++)
    {
        if ((int32u)S[Pos]<0x80)
            ToReturn+=(char)S[Pos];
        else if (IsKeptByte(S, Pos))
            ToReturn+=(char)(S[Pos]-0xDC00);
        else
        {
            size_t Buffer_Size=wcrtomb(Buffer, S[Pos], &State);
            if (Buffer_Size==(size_t)-1)
            {
                ToReturn+='?';
                memset(&State, 0, sizeof(State)); //Undefined after an error
            }
            else
                ToReturn.append(Buffer, Buffer_Size);
        }
    }
}
#endif //ZENLIB_LOCAL_KEEPBYTES

#if defined(ZENLIB_LOCAL_ICONV)
//Functions of iconv, found by name at run time: in the C library (e.g. FreeBSD, NetBSD, Solaris), libiconv of the system (macOS) or
//GNU libiconv, whose names have a "lib" prefix; iconv_t is a pointer with all of them
struct local_iconv
{
    void*   (*Open)(const char* ToCode, const char* FromCode);
    size_t  (*Convert)(void* Converter, char** In, size_t* In_Size, char** Out, size_t* Out_Size);
    int     (*Close)(void* Converter);
};

static bool Local_Iconv_Load(local_iconv& Iconv)
{
    static const char* const Libraries[]=
    {
        NULL, //Already in the program
        #if defined(__APPLE__)
            "/usr/lib/libiconv.2.dylib",
        #endif
        "libiconv.so.2",
        "libiconv.so",
    };
    for (size_t Library_Pos=0; Library_Pos<sizeof(Libraries)/sizeof(*Libraries); Library_Pos++)
    {
        void* Library=Libraries[Library_Pos]?dlopen(Libraries[Library_Pos], RTLD_LAZY|RTLD_LOCAL):RTLD_DEFAULT; //RTLD_DEFAULT is NULL with glibc
        if (Libraries[Library_Pos] && !Library)
            continue;
        for (int Prefix=0; Prefix<2; Prefix++)
        {
            Iconv.Open=(void* (*)(const char*, const char*))dlsym(Library, Prefix?"libiconv_open":"iconv_open");
            Iconv.Convert=(size_t (*)(void*, char**, size_t*, char**, size_t*))dlsym(Library, Prefix?"libiconv":"iconv");
            Iconv.Close=(int (*)(void*))dlsym(Library, Prefix?"libiconv_close":"iconv_close");
            if (Iconv.Open && Iconv.Convert && Iconv.Close)
                return true; //The library stays loaded
        }
        if (Libraries[Library_Pos])
            dlclose(Library);
    }
    return false;
}

//iconv, NULL if it is not available (loaded once, thread-safe since C++11)
static const local_iconv* Local_Iconv()
{
    static local_iconv Iconv;
    static const bool IsLoaded=Local_Iconv_Load(Iconv);
    return IsLoaded?&Iconv:NULL;
}

//Converter from the encoding of the locale to UTF-8 or the reverse, also with another name of the encoding if iconv does not know
//the one of the locale (e.g. "eucJP" with macOS 14 and more), NULL if not possible
static void* Local_Iconv_Open(const local_iconv& Iconv, const char* CodeSet, bool ToUTF8)
{
    void* Converter=ToUTF8?Iconv.Open("UTF-8", CodeSet):Iconv.Open(CodeSet, "UTF-8");
    if (Converter!=(void*)-1)
        return Converter;
    static const char* const Names[][2]=
    {
        {"eucJP",     "EUC-JP"},
        {"eucKR",     "EUC-KR"},
        {"eucCN",     "EUC-CN"},
        {"eucTW",     "EUC-TW"},
        {"SJIS",      "SHIFT_JIS"},
        {"Big5HKSCS", "BIG5-HKSCS"},
    };
    std::string Name;
    for (size_t Pos=0; Pos<sizeof(Names)/sizeof(*Names); Pos++)
        if (!strcmp(CodeSet, Names[Pos][0]))
            Name=Names[Pos][1];
    if (Name.empty() && !strncmp(CodeSet, "ISO8859-", 8))
        Name=std::string("ISO-8859-")+(CodeSet+8);
    if (Name.empty())
        return NULL;
    Converter=ToUTF8?Iconv.Open("UTF-8", Name.c_str()):Iconv.Open(Name.c_str(), "UTF-8");
    return Converter==(void*)-1?NULL:Converter;
}

//Conversion of all the bytes with a converter, from its initial state, false if a character can't be converted or is converted to
//another one (some implementations replace the characters which can't be converted)
static bool Local_Iconv_Run(const local_iconv& Iconv, void* Converter, const char* S, size_t Size, std::string& ToReturn)
{
    ToReturn.clear();
    Iconv.Convert(Converter, NULL, NULL, NULL, NULL);
    char* In=(char*)S;
    size_t In_Size=Size;
    for (;;)
    {
        char Out[256];
        char* Out_Pos=Out;
        size_t Out_Size=sizeof(Out);
        bool End=!In_Size; //Then the end of a shift sequence
        size_t Result=End?Iconv.Convert(Converter, NULL, NULL, &Out_Pos, &Out_Size):Iconv.Convert(Converter, &In, &In_Size, &Out_Pos, &Out_Size);
        int Error=errno;
        ToReturn.append(Out, Out_Pos-Out);
        if (Result==(size_t)-1)
        {
            if (Error!=E2BIG)
                return false;
        }
        else if (Result)
            return false;
        else if (End)
            return true;
    }
}

//GB18030 encodes the characters after U+FFFF with 4 bytes from their value, iconv of some systems (e.g. macOS) does not know them
static bool GB18030_Supplementary_Decode(const int8u* S, size_t Size, int32u& Value)
{
    if (Size<4 || S[0]<0x90 || S[0]>0xE3 || S[1]<0x30 || S[1]>0x39 || S[2]<0x81 || S[2]>0xFE || S[3]<0x30 || S[3]>0x39)
        return false;
    Value=0x10000+(((S[0]-0x90)*10+(S[1]-0x30))*126+(S[2]-0x81))*10+(S[3]-0x30);
    return Value<=0x10FFFF;
}

static void GB18030_Supplementary_Encode(int32u Value, std::string& ToReturn)
{
    char Bytes[4];
    Value-=0x10000;
    Bytes[3]=(char)(0x30+Value%10);
    Value/=10;
    Bytes[2]=(char)(0x81+Value%126);
    Value/=126;
    Bytes[1]=(char)(0x30+Value%10);
    Bytes[0]=(char)(0x90+Value/10);
    ToReturn.append(Bytes, 4);
}

//Conversion of all the bytes from or to GB18030, the characters after U+FFFF without iconv and the other parts with it, false if a
//character can't be converted
static bool Local_Iconv_Run_GB18030(const local_iconv& Iconv, void* Converter, const char* S, size_t Size, std::string& ToReturn, bool ToUTF8)
{
    ToReturn.clear();
    std::string Part;
    size_t Start=0;
    for (size_t Pos=0; Pos<Size;)
    {
        //Length of the character (a second byte from "0" to "9" is in a 4-byte character of GB18030), and its value if it is after U+FFFF
        const int8u* Char=(const int8u*)S+Pos;
        size_t Char_Size;
        int32u Value=0;
        if (ToUTF8)
        {
            Char_Size=*Char<0x80?1:((Pos+1<Size && Char[1]>=0x30 && Char[1]<=0x39)?4:2);
            if (Char_Size==4 && !GB18030_Supplementary_Decode(Char, Size-Pos, Value))
                Value=0;
        }
        else
        {
            Char_Size=UTF8_Char_Size(Char);
            if (Char_Size==4)
                Value=((Char[0]&0x07)<<18)|((Char[1]&0x3F)<<12)|((Char[2]&0x3F)<<6)|(Char[3]&0x3F);
        }
        if (!Char_Size || Char_Size>Size-Pos)
            return false;
        if (Value)
        {
            if (Pos>Start && !Local_Iconv_Run(Iconv, Converter, S+Start, Pos-Start, Part))
                return false;
            if (Pos>Start)
                ToReturn+=Part;
            if (ToUTF8)
            {
                ToReturn+=(char)(0xF0|(Value>>18));
                ToReturn+=(char)(0x80|((Value>>12)&0x3F));
                ToReturn+=(char)(0x80|((Value>>6)&0x3F));
                ToReturn+=(char)(0x80|(Value&0x3F));
            }
            else
                GB18030_Supplementary_Encode(Value, ToReturn);
            Start=Pos+Char_Size;
        }
        Pos+=Char_Size;
    }
    if (Size>Start)
    {
        if (!Local_Iconv_Run(Iconv, Converter, S+Start, Size-Start, Part))
            return false;
        ToReturn+=Part;
    }
    return true;
}

//Conversion of all the bytes from the encoding of the locale to UTF-8 or the reverse, false if iconv or the encoding is not available
//or if a character can't be converted, also used by OS_Utils.cpp
bool Local_Iconv_Convert(const char* S, size_t Size, std::string& ToReturn, bool ToUTF8, const char* CodeSet)
{
    const local_iconv* Iconv=Local_Iconv();
    void* Converter=Iconv?Local_Iconv_Open(*Iconv, CodeSet, ToUTF8):NULL;
    if (!Converter)
        return false;
    bool IsConverted=Local_Iconv_Run(*Iconv, Converter, S, Size, ToReturn);
    if (!IsConverted && !strcmp(CodeSet, "GB18030"))
        IsConverted=Local_Iconv_Run_GB18030(*Iconv, Converter, S, Size, ToReturn, ToUTF8);
    Iconv->Close(Converter);
    return IsConverted;
}
#endif //ZENLIB_LOCAL_ICONV

#if defined(ZENLIB_LOCAL_KEEPBYTES) && defined(ZENLIB_LOCAL_ICONV)
//Character after U+FFFF, as a UTF-16 surrogate pair where wchar_t is 16-bit
static void Append_Supplementary(Ztring& ToReturn, int32u Value)
{
    if (sizeof(wchar_t)==2)
    {
        ToReturn+=(wchar_t)(0xD800+((Value-0x10000)>>10));
        ToReturn+=(wchar_t)(0xDC00+((Value-0x10000)&0x3FF));
    }
    else
        ToReturn+=(wchar_t)Value;
}

//Encoding of the locale with iconv, false if it is not available: all the text at once if it gives the same bytes back, else character
//per character with the length of the character from the C library (its wchar_t may not be Unicode), bytes which are not valid or
//which don't give the same bytes back (e.g. a character with two encodings) kept
static bool From_Local_Iconv(Ztring& ToReturn, const char* S, size_t Size, const char* CodeSet)
{
    const local_iconv* Iconv=Local_Iconv();
    void* Decoder=Iconv?Local_Iconv_Open(*Iconv, CodeSet, true):NULL;
    void* Encoder=Decoder?Local_Iconv_Open(*Iconv, CodeSet, false):NULL;
    if (!Encoder)
    {
        if (Decoder)
            Iconv->Close(Decoder);
        return false;
    }

    std::string UTF8, Back;
    if (Local_Iconv_Run(*Iconv, Decoder, S, Size, UTF8) && Local_Iconv_Run(*Iconv, Encoder, UTF8.data(), UTF8.size(), Back) && Back.size()==Size && !memcmp(Back.data(), S, Size))
        ToReturn.From_UTF8(UTF8.c_str());
    else
    {
        ToReturn.clear();
        bool IsGB18030=!strcmp(CodeSet, "GB18030");
        mbstate_t State;
        memset(&State, 0, sizeof(State));
        for (size_t Pos=0; Pos<Size;)
        {
            if ((int8u)S[Pos]<0x80)
            {
                ToReturn+=(wchar_t)S[Pos];
                Pos++;
                continue;
            }
            int32u Value;
            if (IsGB18030 && GB18030_Supplementary_Decode((const int8u*)S+Pos, Size-Pos, Value))
            {
                Append_Supplementary(ToReturn, Value);
                Pos+=4;
                continue;
            }
            size_t Char_Size=mbrlen(S+Pos, Size-Pos, &State);
            if (Char_Size && Char_Size<=Size-Pos
             && Local_Iconv_Run(*Iconv, Decoder, S+Pos, Char_Size, UTF8) && !UTF8.empty()
             && Local_Iconv_Run(*Iconv, Encoder, UTF8.data(), UTF8.size(), Back) && Back.size()==Char_Size && !memcmp(Back.data(), S+Pos, Char_Size))
            {
                ToReturn+=Ztring().From_UTF8(UTF8.c_str());
                Pos+=Char_Size;
                continue;
            }
            ToReturn+=(wchar_t)(0xDC00+(int8u)S[Pos]); //Then the initial state
            Pos++;
            memset(&State, 0, sizeof(State));
        }
    }

    Iconv->Close(Encoder);
    Iconv->Close(Decoder);
    return true;
}

//Encoding of the locale with iconv, false if it is not available: the bytes kept by From_Local() back, "?" for the characters which
//can't be encoded
static bool To_Local_Iconv(const Ztring& S, std::string& ToReturn, const char* CodeSet)
{
    const local_iconv* Iconv=Local_Iconv();
    void* Encoder=Iconv?Local_Iconv_Open(*Iconv, CodeSet, false):NULL;
    if (!Encoder)
        return false;

    ToReturn.clear();
    bool IsGB18030=!strcmp(CodeSet, "GB18030");
    size_t Start=0;
    for (size_t Pos=0; Pos<=S.size(); Pos++)
    {
        bool IsByte=Pos<S.size() && IsKeptByte(S, Pos);
        if (Pos<S.size() && !IsByte)
            continue;
        if (Pos>Start)
        {
            //All the characters at once, else character per character
            std::string UTF8=Ztring(S, Start, Pos-Start).To_UTF8(), Local;
            if (Local_Iconv_Run(*Iconv, Encoder, UTF8.data(), UTF8.size(), Local))
                ToReturn+=Local;
            else
                for (size_t UTF8_Pos=0; UTF8_Pos<UTF8.size();)
                {
                    const int8u* Char=(const int8u*)UTF8.c_str()+UTF8_Pos;
                    size_t Char_Size=UTF8_Char_Size(Char);
                    if (!Char_Size)
                        Char_Size=1; //Not with To_UTF8()
                    if (Local_Iconv_Run(*Iconv, Encoder, UTF8.data()+UTF8_Pos, Char_Size, Local))
                        ToReturn+=Local;
                    else if (IsGB18030 && Char_Size==4)
                        GB18030_Supplementary_Encode(((Char[0]&0x07)<<18)|((Char[1]&0x3F)<<12)|((Char[2]&0x3F)<<6)|(Char[3]&0x3F), ToReturn);
                    else
                        ToReturn+='?';
                    UTF8_Pos+=Char_Size;
                }
        }
        if (IsByte)
            ToReturn+=(char)(S[Pos]-0xDC00);
        Start=Pos+1;
    }

    Iconv->Close(Encoder);
    return true;
}
#endif //defined(ZENLIB_LOCAL_KEEPBYTES) && defined(ZENLIB_LOCAL_ICONV)

Ztring& Ztring::From_Local (const char* S)
{
    if (S==NULL)
        return *this;

    #ifdef _UNICODE
        #ifdef ZENLIB_USEWX
            size_type OK=wxConvCurrent->MB2WC(NULL, S, 0);
            if (OK!=0 && OK!=Error)
                assign(wxConvCurrent->cMB2WC(S).data());
        #else //ZENLIB_USEWX
            #ifdef WINDOWS
                int Size=MultiByteToWideChar(CP_ACP, 0, S, -1, NULL, 0);
                if (Size!=0)
                {
                    wchar_t* WideString=new wchar_t[Size+1];
                    MultiByteToWideChar(CP_ACP, 0, S, -1, WideString, Size);
                    WideString[Size]=L'\0';
                    assign (WideString);
                    delete[] WideString; //WideString=NULL;
                }
                else
                    clear();
            #else //WINDOWS
                //ASCII: the same with all encodings, without the C library or iconv
                size_t Size=strlen(S);
                if (IsASCII(S, Size))
                {
                    assign(S, S+Size);
                    return *this;
                }

                const char* CodeSet=Local_Other();
                if (!CodeSet)
                    return From_UTF8_KeepBytes(*this, S); //ASCII (UTF-8 instead) or UTF-8
                #if defined(ZENLIB_LOCAL_ICONV)
                    if (From_Local_Iconv(*this, S, Size, CodeSet))
                        return *this;
                #endif
                From_Local_C(*this, S, Size);
            #endif
        #endif //ZENLIB_USEWX
    #else
        assign(S);
    #endif
    return *this;
}

Ztring& Ztring::From_Local (const char* S, size_type Start, size_type Length)
{
    if (S==NULL)
        return *this;

    if (Length==Error)
        Length=strlen(S+Start);
    #ifdef _UNICODE
        char* Temp=new char[Length+1];
        strncpy (Temp, S+Start, Length);
        Temp[Length]='\0';
        From_Local(Temp);
        delete[] Temp; //Temp=NULL;
    #else
        assign(S+Start, Length);
        if (find(__T('\0'))!=std::string::npos)
            resize(find(__T('\0')));
    #endif
    return *this;
}

Ztring& Ztring::From_ISO_8859_1(const char* S)
{
    size_t Length = strlen(S);
    wchar_t* Temp = new wchar_t[Length +1];

    for (size_t Pos=0; Pos<Length+1; Pos++)
        Temp[Pos]=(wchar_t)((int8u)S[Pos]);

    From_Unicode(Temp);
    delete[] Temp;
    return *this;
}

Ztring& Ztring::From_ISO_8859_1(const char* S, size_type Start, size_type Length)
{
    if (S==NULL)
        return *this;

    if (Length==Error)
        Length=strlen(S+Start);

    char* Temp = new char[Length+1];
    strncpy(Temp, S +Start, Length);
    Temp[Length] = '\0';
    From_ISO_8859_1(Temp);
    delete[] Temp;

    return *this;
}

Ztring& Ztring::From_ISO_8859_2(const char* S)
{
    size_t Length = strlen(S);
    wchar_t* Temp = new wchar_t[Length +1];

    for (size_t Pos=0; Pos<Length+1; Pos++)
    {
        if ((int8u)S[Pos]>=0xA0)
            Temp[Pos]=(wchar_t)Ztring_ISO_8859_2[((int8u)S[Pos])-0xA0];
        else
            Temp[Pos]=(wchar_t)((int8u)S[Pos]);
    }

    From_Unicode(Temp);
    delete[] Temp;
    return *this;
}

Ztring& Ztring::From_ISO_8859_2(const char* S, size_type Start, size_type Length)
{
    if (S==NULL)
        return *this;

    if (Length==Error)
        Length=strlen(S+Start);

    char* Temp = new char[Length+1];
    strncpy(Temp, S +Start, Length);
    Temp[Length] = '\0';
    From_ISO_8859_2(Temp);
    delete[] Temp;

    return *this;
}

Ztring& Ztring::From_GUID (const int128u S)
{
    Ztring S1;
    S1.From_CC1((int8u) ((S.hi&0x000000FF00000000LL)>>32)); append(S1);
    S1.From_CC1((int8u) ((S.hi&0x0000FF0000000000LL)>>40)); append(S1);
    S1.From_CC1((int8u) ((S.hi&0x00FF000000000000LL)>>48)); append(S1);
    S1.From_CC1((int8u) ((S.hi&0xFF00000000000000LL)>>56)); append(S1); append(__T("-"));
    S1.From_CC1((int8u) ((S.hi&0x0000000000FF0000LL)>>16)); append(S1);
    S1.From_CC1((int8u) ((S.hi&0x00000000FF000000LL)>>24)); append(S1); append(__T("-"));
    S1.From_CC1((int8u) ( S.hi&0x00000000000000FFLL     )); append(S1);
    S1.From_CC1((int8u) ((S.hi&0x000000000000FF00LL)>> 8)); append(S1); append(__T("-"));
    S1.From_CC2((int16u)((S.lo&0xFFFF000000000000LL)>>48)); append(S1); append(__T("-"));
    S1.From_CC2((int16u)((S.lo&0x0000FFFF00000000LL)>>32)); append(S1);
    S1.From_CC2((int16u)((S.lo&0x00000000FFFF0000LL)>>16)); append(S1);
    S1.From_CC2((int16u)( S.lo&0x000000000000FFFFLL     )); append(S1);

    return *this;
}

Ztring& Ztring::From_UUID (const int128u S)
{
    Ztring S1;
    S1.From_CC2((int16u)((S.hi&0xFFFF000000000000LL)>>48)); assign(S1);
    S1.From_CC2((int16u)((S.hi&0x0000FFFF00000000LL)>>32)); append(S1); append(__T("-"));
    S1.From_CC2((int16u)((S.hi&0x00000000FFFF0000LL)>>16)); append(S1); append(__T("-"));
    S1.From_CC2((int16u)( S.hi&0x000000000000FFFFLL     )); append(S1); append(__T("-"));
    S1.From_CC2((int16u)((S.lo&0xFFFF000000000000LL)>>48)); append(S1); append(__T("-"));
    S1.From_CC2((int16u)((S.lo&0x0000FFFF00000000LL)>>32)); append(S1);
    S1.From_CC2((int16u)((S.lo&0x00000000FFFF0000LL)>>16)); append(S1);
    S1.From_CC2((int16u)( S.lo&0x000000000000FFFFLL     )); append(S1);

    return *this;
}

Ztring& Ztring::From_CC4 (const int32u S)
{
    clear();
    for (int8s i=(4-1)*8; i>=0; i-=8)
    {
        int32u Value=(S&(0xFF<<i))>>i;
        if (Value<0x20)
        {
            if (!i || (i!=24  && !(S&(0xFFFFFFFF>>(32-i)))))
                return *this; // Trailing 0 are fine

            // Not valid, using 0x as fallback
            clear();
            append(__T("0x"));
            append(Ztring().From_CC1((int8u)((S&0xFF000000)>>24)));
            append(Ztring().From_CC1((int8u)((S&0x00FF0000)>>16)));
            append(Ztring().From_CC1((int8u)((S&0x0000FF00)>> 8)));
            append(Ztring().From_CC1((int8u)((S&0x000000FF)    )));
            return *this;
        }
        append(Ztring().From_Unicode((wchar_t)Value)); //Not 1 byte without Unicode
    }
    return *this;
}

Ztring& Ztring::From_CC3 (const int32u S)
{
    clear();
    for (int8s i=(3-1)*8; i>=0; i-=8)
    {
        int32u Value=(S&(0xFF<<i))>>i;
        if (Value<0x20)
        {
            if (!i || (i!=16  && !(S&(0xFFFFFF>>(24-i)))))
                return *this; // Trailing 0 are fine

            // Not valid, using 0x as fallback
            clear();
            append(__T("0x"));
            append(Ztring().From_CC1((int8u)((S&0x00FF0000)>>16)));
            append(Ztring().From_CC1((int8u)((S&0x0000FF00)>> 8)));
            append(Ztring().From_CC1((int8u)((S&0x000000FF)    )));
            return *this;
        }
        append(Ztring().From_Unicode((wchar_t)Value)); //Not 1 byte without Unicode
    }
    return *this;
}

Ztring& Ztring::From_CC2 (const int16u S)
{
    clear();
    Ztring Pos1; Pos1.From_Number(S, 16);
    resize(4-Pos1.size(), __T('0'));
    append(Pos1);
    MakeUpperCase();

    return *this;
}

Ztring& Ztring::From_CC1 (const int8u S)
{
    clear();
    Ztring Pos1; Pos1.From_Number(S, 16);
    resize(2-Pos1.size(), __T('0'));
    append(Pos1);
    MakeUpperCase();

    return *this;
}

//---------------------------------------------------------------------------
#if !defined(STREAM_MISSING)
// Integer to text without stream, same result as a stream with setbase(Radix) then MakeUpperCase():
// hexadecimal for 16, octal for 8, else decimal
static void Number_To_Text(Ztring& ToReturn, int64u Value, int8u Radix, bool Negative=false)
{
    Char Temp[24]; //Enough for 64-bit in octal and for decimal with sign
    Char* const End=Temp+24;
    Char* Pos=End;
    switch (Radix)
    {
        case 16:
            do
            {
                *--Pos=__T("0123456789ABCDEF")[Value&0xF];
                Value>>=4;
            }
            while (Value);
            break;
        case 8:
            do
            {
                *--Pos=(Char)(__T('0')+(Value&0x7));
                Value>>=3;
            }
            while (Value);
            break;
        default:
            do
            {
                *--Pos=(Char)(__T('0')+(Value%10));
                Value/=10;
            }
            while (Value);
            if (Negative)
                *--Pos=__T('-');
    }
    ToReturn.assign(Pos, End-Pos);
}

// Signed integer, Value_Unsigned is the value as the unsigned type of the same size, used by streams in hexadecimal and octal
static void Number_To_Text(Ztring& ToReturn, int64s Value, int64u Value_Unsigned, int8u Radix)
{
    if (Radix==16 || Radix==8)
        Number_To_Text(ToReturn, Value_Unsigned, Radix);
    else if (Value<0)
        Number_To_Text(ToReturn, ((int64u)0)-(int64u)Value, 10, true);
    else
        Number_To_Text(ToReturn, (int64u)Value, 10);
}
#endif //!defined(STREAM_MISSING)

Ztring& Ztring::From_Number (const int8s I, int8u Radix)
{
    #if defined(STREAM_MISSING)
        if (Radix==0)
        {
            clear();
            return *this;
        }
        Char* C1=new Char[33];
        #ifdef __MINGW32__
            _itot (I, C1, Radix);
        #else
            _tnprintf(C1, 32, Radix==10?__T("%d"):(Radix==16?__T("%x"):(Radix==8?__T("%o"):__T(""))), I);
        #endif
        assign (C1);
        delete[] C1; //C1=NULL;
    #else
        #ifdef UNICODE
            Number_To_Text(*this, (int64s)I, (int64u)(unsigned int)(int)I, Radix); //Given as int to the stream
        #else //UNICODE
            Number_To_Text(*this, (int64u)(size_t)I, Radix); //Given as size_t to the stream
        #endif //UNICODE
        return *this;
    #endif
    MakeUpperCase();
    return *this;
}

Ztring& Ztring::From_Number (const int8u I, int8u Radix)
{
    #if defined(STREAM_MISSING)
        if (Radix==0)
        {
            clear();
            return *this;
        }
        Char* C1=new Char[33];
        #ifdef __MINGW32__
            _ultot (I, C1, Radix);
        #else
            _tnprintf(C1, 32, Radix==10?__T("%d"):(Radix==16?__T("%x"):(Radix==8?__T("%o"):__T(""))), I);
        #endif
        assign (C1);
        delete[] C1; //C1=NULL;
    #else
        if (Radix==2)
        {
            clear();
            for (int8u Pos=0; Pos<8; Pos++)
            {
                if (I<(((int8u)1)<<Pos))
                    break;
                insert(0, 1, (I&(((int8u)1)<<Pos))?__T('1'):__T('0'));
            }
        }
        else
        {
            Number_To_Text(*this, (int64u)I, Radix);
            return *this;
        }
    #endif
    MakeUpperCase();
    return *this;
}

Ztring& Ztring::From_Number (const int16s I, int8u Radix)
{
    #if defined(STREAM_MISSING)
        if (Radix==0)
        {
            clear();
            return *this;
        }
        Char* C1=new Char[33];
        #ifdef __MINGW32__
            _itot (I, C1, Radix);
        #else
            _tnprintf(C1, 32, Radix==10?__T("%d"):(Radix==16?__T("%x"):(Radix==8?__T("%o"):__T(""))), I);
        #endif
        assign (C1);
        delete[] C1; //C1=NULL;
    #else
        Number_To_Text(*this, (int64s)I, (int64u)(int16u)I, Radix);
        return *this;
    #endif
    MakeUpperCase();
    return *this;
}

Ztring& Ztring::From_Number (const int16u I, int8u Radix)
{
    #if defined(STREAM_MISSING)
        if (Radix==0)
        {
            clear();
            return *this;
        }
        Char* C1=new Char[33];
        #ifdef __MINGW32__
            _ultot (I, C1, Radix);
        #else
            _tnprintf(C1, 32, Radix==10?__T("%u"):(Radix==16?__T("%x"):(Radix==8?__T("%o"):__T(""))), I);
        #endif
        assign (C1);
        delete[] C1; //C1=NULL;
    #else
        if (Radix==2)
        {
            clear();
            for (int8u Pos=0; Pos<16; Pos++)
            {
                if (I<(((int16u)1)<<Pos))
                    break;
                insert(0, 1, (I&(((int16u)1)<<Pos))?__T('1'):__T('0'));
            }
        }
        else
        {
            Number_To_Text(*this, (int64u)I, Radix);
            return *this;
        }
    #endif
    MakeUpperCase();
    return *this;
}

Ztring& Ztring::From_Number (const int32s I, int8u Radix)
{
    #if defined(STREAM_MISSING)
        if (Radix==0)
        {
            clear();
            return *this;
        }
        Char* C1=new Char[33];
        #ifdef __MINGW32__
            _itot (I, C1, Radix);
        #else
            _tnprintf(C1, 32, Radix==10?__T("%ld"):(Radix==16?__T("%lx"):(Radix==8?__T("%lo"):__T(""))), I);
        #endif
        assign (C1);
        delete[] C1; //C1=NULL;
    #else
        Number_To_Text(*this, (int64s)I, (int64u)(int32u)I, Radix);
        return *this;
    #endif
    MakeUpperCase();
    return *this;
}

Ztring& Ztring::From_Number (const int32u I, int8u Radix)
{
    #if defined(STREAM_MISSING)
        if (Radix==0)
        {
            clear();
            return *this;
        }
        Char* C1=new Char[33];
        #ifdef __MINGW32__
            _ultot (I, C1, Radix);
        #else
            _tnprintf(C1, 32, Radix==10?__T("%lu"):(Radix==16?__T("%lx"):(Radix==8?__T("%lo"):__T(""))), I);
        #endif
        assign (C1);
        delete[] C1; //C1=NULL;
    #else
        if (Radix==2)
        {
            clear();
            for (int8u Pos=0; Pos<32; Pos++)
            {
                if (I<(((int32u)1)<<Pos))
                    break;
                insert(0, 1, (I&(((int32u)1)<<Pos))?__T('1'):__T('0'));
            }
        }
        else
        {
            Number_To_Text(*this, (int64u)I, Radix);
            return *this;
        }
    #endif
    MakeUpperCase();
    return *this;
}

Ztring& Ztring::From_Number (const int64s I, int8u Radix)
{
    #if defined(STREAM_MISSING)
        if (Radix==0)
        {
            clear();
            return *this;
        }
        Char* C1=new Char[65];
        #ifdef __MINGW32__
            _i64tot (I, C1, Radix);
        #else
            _tnprintf(C1, 64, Radix==10?__T("%lld"):(Radix==16?__T("%llx"):(Radix==8?__T("%llo"):__T(""))), I);
        #endif
        assign (C1);
        delete[] C1; //C1=NULL;
    #else
        Number_To_Text(*this, (int64s)I, (int64u)(int64u)I, Radix);
        return *this;
    #endif
    MakeUpperCase();
    return *this;
}

Ztring& Ztring::From_Number (const int64u I, int8u Radix)
{
    #if defined(STREAM_MISSING)
        if (Radix==0)
        {
            clear();
            return *this;
        }
        Char* C1=new Char[65]; C1[0] = 0;
        #ifdef __MINGW32__
            _ui64tot (I, C1, Radix);
        #else
            _tnprintf(C1, 64, Radix==10?__T("%llu"):(Radix==16?__T("%llx"):(Radix==8?__T("%llo"):__T(""))), I);
        #endif
        assign (C1);
        delete[] C1; //C1=NULL;
    #else
        if (Radix==2)
        {
            clear();
            for (int8u Pos=0; Pos<32; Pos++)
            {
                if (I<(((int64u)1)<<Pos))
                    break;
                insert(0, 1, (I&(((int64u)1)<<Pos))?__T('1'):__T('0'));
            }
        }
        else
        {
            Number_To_Text(*this, (int64u)I, Radix);
            return *this;
        }
    #endif
    MakeUpperCase();
    return *this;
}

Ztring& Ztring::From_Number (const int128u I, int8u Radix)
{
    From_Local(I.toString(Radix));

    return *this;
}

Ztring& Ztring::From_Number (const float32 F, int8u Precision, ztring_t Options)
{
    #if defined(STREAM_MISSING)
        Char C1[100];
        _tnprintf (C1, 99, (Ztring(__T("%."))+Ztring::ToZtring(Precision)+__T("f")).c_str(), F);
        assign(C1);
    #else
        toStringStream Stream;
        Stream << setprecision(Precision) << fixed << F;
        assign(Stream.str());
        #if defined(__BORLANDC__)
            FindAndReplace(__T(","), __T(".")); //Borland C++ Builder 2010+Windows Seven put a comma for istringstream, but does not support comma for ostringstream
        #endif
    #endif

    if ((Options & Ztring_NoZero && size()>0) && find(__T('.'))!=string::npos)
    {
        while (size()>0 && ((*this)[size()-1]==__T('0')))
            resize(size()-1);
        if (size()>0 && (*this)[size()-1]==__T('.'))
            resize(size()-1);
    }

    return *this;
}

Ztring& Ztring::From_Number (const float64 F, int8u Precision, ztring_t Options)
{
    #if defined(STREAM_MISSING)
        Char C1[100];
        _tnprintf (C1, 99, (Ztring(__T("%."))+Ztring::ToZtring(Precision)+__T("f")).c_str(), F);
        assign(C1);
    #else
        toStringStream Stream;
        Stream << setprecision(Precision) << fixed << F;
        assign(Stream.str());
        #if defined(__BORLANDC__)
            FindAndReplace(__T(","), __T(".")); //Borland C++ Builder 2010+Windows Seven put a comma for istringstream, but does not support comma for ostringstream
        #endif
    #endif

    if ((Options & Ztring_NoZero && size()>0) && find(__T('.'))!=string::npos)
    {
        while (size()>0 && ((*this)[size()-1]==__T('0')))
            resize(size()-1);
        if (size()>0 && (*this)[size()-1]==__T('.'))
            resize(size()-1);
    }

    return *this;
}

Ztring& Ztring::From_Number (const float80 F, int8u Precision, ztring_t Options)
{
    #if defined(STREAM_MISSING)
        Char C1[100];
        _tnprintf (C1, 99, (Ztring(__T("%."))+Ztring::ToZtring(Precision)+__T("f")).c_str(), F);
        assign(C1);
    #else
        toStringStream Stream;
        Stream << setprecision(Precision) << fixed << F;
        assign(Stream.str());
        #if defined(__BORLANDC__)
            FindAndReplace(__T(","), __T(".")); //Borland C++ Builder 2010+Windows Seven put a comma for istringstream, but does not support comma for ostringstream
        #endif
    #endif

    if ((Options & Ztring_NoZero && size()>0) && find(__T('.'))!=string::npos)
    {
        while (size()>0 && ((*this)[size()-1]==__T('0')))
            resize(size()-1);
        if (size()>0 && (*this)[size()-1]==__T('.'))
            resize(size()-1);
    }

    return *this;
}

#ifdef SIZE_T_IS_LONG
Ztring& Ztring::From_Number (const size_t I, int8u Radix)
{
    #if defined(STREAM_MISSING)
        Char C1[100];
        _tnprintf(C1, 64, Radix==10?__T("%zu"):(Radix==16?__T("%zx"):(Radix==8?__T("%zo"):__T(""))), I);
        assign(C1);
    #elif defined(__BORLANDC__)
        toStringStream Stream;
        Stream << setbase(Radix) << I;
        assign(Stream.str());
        FindAndReplace(__T(","), __T(".")); //Borland C++ Builder 2010+Windows Seven put a comma for istringstream, but does not support comma for ostringstream
    #else
        Number_To_Text(*this, (int64u)I, Radix);
        return *this;
    #endif
    MakeUpperCase();
    return *this;
}
#endif //SIZE_T_IS_LONG

Ztring& Ztring::From_BCD     (const int8u I)
{
    #if defined(STREAM_MISSING)
        clear();
        append(1, __T('0')+I/0x10);
        append(1, __T('0')+I%0x10);
    #else
        toStringStream Stream;
        Stream << I/0x10;
        Stream << I%0x10;
        assign(Stream.str());
    #endif
    return *this;
}

//---------------------------------------------------------------------------
Ztring& Ztring::Duration_From_Milliseconds (const int64s Value_)
{
    int64s Value=Value_;
    bool Negative=false;
    if (Value<0)
    {
        Value=-Value;
        Negative=true;
    }

    int64u HH=(int8u)(Value/1000/60/60);
    int64u MM=Value/1000/60   -((HH*60));
    int64u Stream=Value/1000      -((HH*60+MM)*60);
    int64u MS=Value           -((HH*60+MM)*60+Stream)*1000;
    Ztring DateT;
    Ztring Date;
    DateT.From_Number(HH); if (DateT.size()<2){DateT=Ztring(__T("0"))+DateT;}
    Date+=DateT;
    Date+=__T(":");
    DateT.From_Number(MM); if (DateT.size()<2){DateT=Ztring(__T("0"))+DateT;}
    Date+=DateT;
    Date+=__T(":");
    DateT.From_Number(Stream); if (DateT.size()<2){DateT=Ztring(__T("0"))+DateT;}
    Date+=DateT;
    Date+=__T(".");
    DateT.From_Number(MS); if (DateT.size()<2){DateT=Ztring(__T("00"))+DateT;} else if (DateT.size()<3){DateT=Ztring(__T("0"))+DateT;}
    Date+=DateT;
    if (Negative)
    {
        assign(__T("-"));
        append(Date);
    }
    else
        assign (Date.c_str());
    return *this;
}

//---------------------------------------------------------------------------
Ztring& Ztring::Duration_From_Milliseconds (const int64u Value)
{
    int64u HH=(int8u)(Value/1000/60/60);
    int64u MM=Value/1000/60   -((HH*60));
    int64u Stream=Value/1000      -((HH*60+MM)*60);
    int64u MS=Value           -((HH*60+MM)*60+Stream)*1000;
    Ztring DateT;
    Ztring Date;
    DateT.From_Number(HH); if (DateT.size()<2){DateT=Ztring(__T("0"))+DateT;}
    Date+=DateT;
    Date+=__T(":");
    DateT.From_Number(MM); if (DateT.size()<2){DateT=Ztring(__T("0"))+DateT;}
    Date+=DateT;
    Date+=__T(":");
    DateT.From_Number(Stream); if (DateT.size()<2){DateT=Ztring(__T("0"))+DateT;}
    Date+=DateT;
    Date+=__T(".");
    DateT.From_Number(MS); if (DateT.size()<2){DateT=Ztring(__T("00"))+DateT;} else if (DateT.size()<3){DateT=Ztring(__T("0"))+DateT;}
    Date+=DateT;
    assign (Date.c_str());
    return *this;
}

Ztring& Ztring::Date_From_Milliseconds_1601 (const int64u Value)
{
    if (Value>=11644473600000LL) //Values <1970 are not supported
    {
        Date_From_Seconds_1970((int32u)((Value-11644473600000LL)/1000));
        append(__T("."));
        Ztring Milliseconds; Milliseconds.From_Number(Value%1000);
        while (Milliseconds.size()<3)
            Milliseconds+=__T('0');
        append(Milliseconds);
    }
    else
        clear(); //Not supported

    return *this;
}

Ztring& Ztring::Date_From_Seconds_1601 (const int64u Value)
{
    return Date_From_Seconds_1970(((int64s)Value)-11644473600LL);
}

Ztring& Ztring::Date_From_Seconds_1900 (const int32u Value)
{
    if (Value>2208988800)
        return Date_From_Seconds_1970(((int64s)Value)-2208988800);
    else
        return Date_From_Seconds_1970(((int64s)Value)+0x100000000LL-2208988800); //Value is considering to loop e.g. NTP value
}

Ztring& Ztring::Date_From_Seconds_1900 (const int64s Value)
{
    return Date_From_Seconds_1970(Value-2208988800);
}

Ztring& Ztring::Date_From_Seconds_1904 (const int32u Value)
{
    return Date_From_Seconds_1970(((int64s)Value)-2082844800);
}

Ztring& Ztring::Date_From_Seconds_1904 (const int64u Value)
{
    return Date_From_Seconds_1970(((int64s)Value)-2082844800);
}

Ztring& Ztring::Date_From_Seconds_1904 (const int64s Value)
{
    return Date_From_Seconds_1970(Value-2082844800);
}

Ztring& Ztring::Date_From_Seconds_1970 (const int32u Value)
{
    return Date_From_Seconds_1970((int64s)Value);
}

Ztring& Ztring::Date_From_Seconds_1970 (const int32s Value)
{
    return Date_From_Seconds_1970((int64s)Value);
}

Ztring& Ztring::Date_From_Seconds_1970 (const int64s Value)
{
    time_t Time=(time_t)Value;
    #if defined(_WIN32)
        #if _CRT_USE_CONFORMING_ANNEX_K_TIME || (__STDC_WANT_LIB_EXT1__ && defined(__STDC_LIB_EXT1__))
        // C11 standard version on MSVC or other compilers
        tm Gmt_Temp;
        tm* Gmt = gmtime_s(&Time, &Gmt_Temp);
        #else
        // MSVC and MinGW-w64 argument order and return value differs from C11 standard
        tm Gmt_Temp;
        errno_t gmtime_s_Result = gmtime_s(&Gmt_Temp, &Time);
        tm* Gmt = gmtime_s_Result ? nullptr : &Gmt_Temp;
        #endif
    #elif defined(HAVE_GMTIME_R)
        // POSIX or C23
        tm Gmt_Temp;
        tm *Gmt = gmtime_r(&Time, &Gmt_Temp);
    #else
        // Fallback: not thread-safe, but prevents compile errors
        #ifdef __GNUC__
        #warning "This version of ZenLib is not thread safe"
        #endif
        tm *Gmt = gmtime(&Time);
    #endif
    if (!Gmt)
    {
        clear();
        return *this;
    }
    Ztring DateT;
    Ztring Date=__T("UTC ");
    Date+=Ztring::ToZtring((Gmt->tm_year+1900));
    Date+=__T("-");
    DateT.From_Number(Gmt->tm_mon+1); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Gmt->tm_mon+1);}
    Date+=DateT;
    Date+=__T("-");
    DateT.From_Number(Gmt->tm_mday); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Gmt->tm_mday);}
    Date+=DateT;
    Date+=__T(" ");
    DateT.From_Number(Gmt->tm_hour); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Gmt->tm_hour);}
    Date+=DateT;
    Date+=__T(":");
    DateT=Ztring::ToZtring(Gmt->tm_min); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Gmt->tm_min);}
    Date+=DateT;
    Date+=__T(":");
    DateT.From_Number(Gmt->tm_sec); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Gmt->tm_sec);}
    Date+=DateT;
    assign (Date.c_str());
    return *this;
}

Ztring& Ztring::Date_From_Seconds_1970_Local (const int32u Value)
{
    time_t Time=(time_t)Value;
    #if defined(_WIN32)
        #if _CRT_USE_CONFORMING_ANNEX_K_TIME || (__STDC_WANT_LIB_EXT1__ && defined(__STDC_LIB_EXT1__))
        // C11 standard version on MSVC or other compilers
        tm Gmt_Temp;
        tm* Gmt = localtime_s(&Time, &Gmt_Temp);
        #else
        // MSVC and MinGW-w64 argument order and return value differs from C11 standard
        tm Gmt_Temp;
        errno_t localtime_s_Result = localtime_s(&Gmt_Temp, &Time);
        tm* Gmt = localtime_s_Result ? nullptr : &Gmt_Temp;
        #endif
    #elif defined(HAVE_GMTIME_R)
        // POSIX or C23
        tm Gmt_Temp;
        tm *Gmt = localtime_r(&Time, &Gmt_Temp);
    #else
        // Fallback: not thread-safe, but prevents compile errors
        #ifdef __GNUC__
        #warning "This version of ZenLib is not thread safe"
        #endif
        tm *Gmt = localtime(&Time);
    #endif
    Ztring DateT;
    Ztring Date;
    if (Gmt)
    {
    Date+=Ztring::ToZtring((Gmt->tm_year+1900));
    Date+=__T("-");
    DateT.From_Number(Gmt->tm_mon+1); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Gmt->tm_mon+1);}
    Date+=DateT;
    Date+=__T("-");
    DateT.From_Number(Gmt->tm_mday); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Gmt->tm_mday);}
    Date+=DateT;
    Date+=__T(" ");
    DateT.From_Number(Gmt->tm_hour); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Gmt->tm_hour);}
    Date+=DateT;
    Date+=__T(":");
    DateT=Ztring::ToZtring(Gmt->tm_min); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Gmt->tm_min);}
    Date+=DateT;
    Date+=__T(":");
    DateT.From_Number(Gmt->tm_sec); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Gmt->tm_sec);}
    Date+=DateT;
    assign (Date.c_str());
    }
    return *this;
}

Ztring& Ztring::Date_From_String (const char* Value, size_t Value_Size)
{
    //Only the year
    if (Value_Size<10)
    {
        From_UTF8(Value, 0, Value_Size);
        return *this;
    }

    #ifdef ZENLIB_USEWX
        Ztring ToReturn=__T("UTC ");
        wxDateTime Date;
        Ztring DateS;
        DateS.From_Local(Value, Value_Size).c_str();
        if (!DateS.empty() && DateS[DateS.size()-1]==__T('\n'))
            DateS.resize(DateS.size()-1);

        //Some strange formating : exactly 24 bytes (or 25 with 0x0A at the end) and Year is at the end
        if (DateS.size()==24 && DateS[23]>=__T('0') && DateS[23]<=__T('9') && DateS[21]>=__T('0') && DateS[21]<=__T('9') && DateS[19]==__T(' '))
            Date.ParseFormat(DateS.c_str(), __T("%a %b %d %H:%M:%S %Y"));
        //ISO date
        else if (DateS.size()==10 && (DateS[4]<__T('0') || DateS[4]>__T('9')) && (DateS[7]<__T('0') || DateS[7]>__T('9')))
        {
            DateS[4]=__T('-');
            DateS[7]=__T('-');
            ToReturn+=DateS;
        }
        //Default
        else
            Date.ParseDateTime(DateS.c_str());

        if (ToReturn.size()<5)
        {
            ToReturn+=Date.FormatISODate();
            ToReturn+=__T(" ");
            ToReturn+=Date.FormatISOTime();
        }
        else
            ToReturn+=DateS;

        assign (ToReturn.c_str());
    #else //ZENLIB_USEWX
        Ztring DateS; DateS.From_UTF8(Value, 0, Value_Size);
        //Unix style formating : exactly 24 bytes (or 25 with 0x0A at the end) and Year is at the end
        if ((DateS.size()==24 || (DateS.size()==25 && DateS[24]==__T('\n'))) && DateS[23]>=__T('0') && DateS[23]<=__T('9') && DateS[21]>=__T('0') && DateS[21]<=__T('9') && DateS[19]==__T(' '))
        {
            clear();
            append(1, DateS[20]);
            append(1, DateS[21]);
            append(1, DateS[22]);
            append(1, DateS[23]);
            append(1, __T('-'));
                 if (DateS[4]==__T('J') && DateS[5]==__T('a') && DateS[6]==__T('n') && DateS[7]==__T(' '))
            {
                append(1, __T('0'));
                append(1, __T('1'));
            }
            else if (DateS[4]==__T('F') && DateS[5]==__T('e') && DateS[6]==__T('b') && DateS[7]==__T(' '))
            {
                append(1, __T('0'));
                append(1, __T('2'));
            }
            else if (DateS[4]==__T('M') && DateS[5]==__T('a') && DateS[6]==__T('r') && DateS[7]==__T(' '))
            {
                append(1, __T('0'));
                append(1, __T('3'));
            }
            else if (DateS[4]==__T('A') && DateS[5]==__T('p') && DateS[6]==__T('r') && DateS[7]==__T(' '))
            {
                append(1, __T('0'));
                append(1, __T('4'));
            }
            else if (DateS[4]==__T('M') && DateS[5]==__T('a') && DateS[6]==__T('y') && DateS[7]==__T(' '))
            {
                append(1, __T('0'));
                append(1, __T('5'));
            }
            else if (DateS[4]==__T('J') && DateS[5]==__T('u') && DateS[6]==__T('n') && DateS[7]==__T(' '))
            {
                append(1, __T('0'));
                append(1, __T('6'));
            }
            else if (DateS[4]==__T('J') && DateS[5]==__T('u') && DateS[6]==__T('l') && DateS[7]==__T(' '))
            {
                append(1, __T('0'));
                append(1, __T('7'));
            }
            else if (DateS[4]==__T('A') && DateS[5]==__T('u') && DateS[6]==__T('g') && DateS[7]==__T(' '))
            {
                append(1, __T('0'));
                append(1, __T('8'));
            }
            else if (DateS[4]==__T('S') && DateS[5]==__T('e') && DateS[6]==__T('p') && DateS[7]==__T(' '))
            {
                append(1, __T('0'));
                append(1, __T('9'));
            }
            else if (DateS[4]==__T('O') && DateS[5]==__T('c') && DateS[6]==__T('t') && DateS[7]==__T(' '))
            {
                append(1, __T('1'));
                append(1, __T('0'));
            }
            else if (DateS[4]==__T('N') && DateS[5]==__T('o') && DateS[6]==__T('v') && DateS[7]==__T(' '))
            {
                append(1, __T('1'));
                append(1, __T('1'));
            }
            else if (DateS[4]==__T('D') && DateS[5]==__T('e') && DateS[6]==__T('c') && DateS[7]==__T(' '))
            {
                append(1, __T('1'));
                append(1, __T('2'));
            }
            else
            {
                assign(DateS);
                return *this;
            }
            append(1, __T('-'));
            append(1, DateS[8]);
            append(1, DateS[9]);
            append(1, __T(' '));
            append(1, DateS[11]);
            append(1, DateS[12]);
            append(1, __T(':'));
            append(1, DateS[14]);
            append(1, DateS[15]);
            append(1, __T(':'));
            append(1, DateS[17]);
            append(1, DateS[18]);
        }
        else if (DateS.size()==20 && DateS[4]==__T('-') && DateS[7]==__T('-') && DateS[10]==__T('T') && DateS[13]==__T(':') && DateS[16]==__T(':') && DateS[19]==__T('Z'))
        {
            DateS.resize(19);
            DateS[10]=__T(' ');
            assign(__T("UTC "));
            append(DateS);
        }
        else if (DateS.size()==23 && DateS[4]==__T('-') && DateS[7]==__T('-') && DateS[10]==__T(' ') && DateS[14]==__T(' ') && DateS[17]==__T(':') && DateS[20]==__T(':'))
        {
            DateS.erase(10, 4);
            //assign(__T("UTC ")); //Is not UTC
            append(DateS);
        }
        else
            From_UTF8(Value, 0, Value_Size); //Not implemented
    #endif //ZENLIB_USEWX
    return *this;
}

Ztring& Ztring::Date_From_Numbers (const int8u Year, const int8u Month, const int8u Day, const int8u Hour, const int8u Minute, const int8u Second)
{
    Ztring DateT;
    Ztring Date=__T("UTC ");
    DateT.From_Number(Year); if (DateT.size()<2){DateT=Ztring(__T("200"))+Ztring::ToZtring(Year);}; if (DateT.size()<3){DateT=Ztring(__T("20"))+Ztring::ToZtring(Year);}
    Date+=DateT;
    Date+=__T("-");
    DateT.From_Number(Month); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Month);}
    Date+=DateT;
    Date+=__T("-");
    DateT.From_Number(Day); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Day);}
    Date+=DateT;
    Date+=__T(" ");
    DateT.From_Number(Hour); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Hour);}
    Date+=DateT;
    Date+=__T(":");
    DateT=Ztring::ToZtring(Minute); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Minute);}
    Date+=DateT;
    Date+=__T(":");
    DateT.From_Number(Second); if (DateT.size()<2){DateT=Ztring(__T("0"))+Ztring::ToZtring(Second);}
    Date+=DateT;
    assign (Date.c_str());
    return *this;
}

#ifndef WSTRING_MISSING
//---------------------------------------------------------------------------
std::wstring Ztring::To_Unicode () const
{
    #ifdef _UNICODE
        return c_str();
    #else //_UNICODE
        #ifdef ZENLIB_USEWX
            return wxConvCurrent->cMB2WC(c_str()).data();
        #else //ZENLIB_USEWX
            return std::wstring(); //Not implemented
        #endif //ZENLIB_USEWX
    #endif //_UNICODE
}
#endif //WSTRING_MISSING

std::string Ztring::To_UTF8 () const
{
    #ifdef _UNICODE
        //Correction thanks to Andrew Jang
        // Don't use WideCharToMultiByte(), some characters are not well converted
        //Always valid UTF-8, as the result may be in a document decoded with From_UTF8(): U+FFFD for the values which are not characters,
        //UTF-16 surrogates which are not in a pair (e.g. the bytes kept by From_Local()) and values after U+10FFFF
        std::string ToReturn;
        ToReturn.reserve(size()); // more efficient

        for (size_t Pos=0; Pos<size() && operator[](Pos); Pos++)
        {
            int32u Value=(int32u)operator[](Pos);
            if (Value<0x80)
            {
                ToReturn+=(char)Value;
                continue;
            }
            if ((Value&0xFC00)==0xD800 && Pos+1<size() && ((int32u)operator[](Pos+1)&0xFC00)==0xDC00)
            {
                Value=0x10000+((Value&0x3FF)<<10)+((int32u)operator[](Pos+1)&0x3FF); //UTF-16 surrogate pair
                Pos++;
            }
            else if ((Value>=0xD800 && Value<=0xDFFF) || Value>0x10FFFF)
                Value=0xFFFD;

            // refer to http://en.wikipedia.org/wiki/UTF-8#Description
            if (Value<0x800)
                ToReturn+=(char)(0xC0|(Value>>6));
            else
            {
                if (Value<0x10000)
                    ToReturn+=(char)(0xE0|(Value>>12));
                else
                {
                    ToReturn+=(char)(0xF0|(Value>>18));
                    ToReturn+=(char)(0x80|((Value>>12)&0x3F));
                }
                ToReturn+=(char)(0x80|((Value>>6)&0x3F));
            }
            ToReturn+=(char)(0x80|(Value&0x3F));
        }

        return ToReturn;
    #else
        #ifdef ZENLIB_USEWX
            return wxConvUTF8.cWC2MB(wxConvCurrent->cMB2WC(c_str())).data();
        #else //ZENLIB_USEWX
            //Text is UTF-8 without Unicode, but not the bytes of a file name: replaced, the result may be in a document decoded with From_UTF8()
            std::string ToReturn;
            for (const int8u* Z=(const int8u*)c_str(); *Z; )
            {
                size_t Size=UTF8_Char_Size(Z);
                if (Size)
                {
                    ToReturn.append((const char*)Z, Size);
                    Z+=Size;
                }
                else
                {
                    ToReturn+="\xEF\xBF\xBD";
                    Z++;
                }
            }
            return ToReturn;
        #endif //ZENLIB_USEWX
    #endif
}

std::string Ztring::To_Local () const
{
    #ifdef _UNICODE
        #ifdef ZENLIB_USEWX
            wxCharBuffer C=wxConvCurrent->cWC2MB(c_str());
            if (C.data())
                return C.data();
            else
                return std::string();
        #else //ZENLIB_USEWX
            #ifdef WINDOWS
                int Size=WideCharToMultiByte(CP_ACP, 0, c_str(), -1, NULL, 0, NULL, NULL);
                if (Size!=0)
                {
                    char* AnsiString=new char[Size+1];
                    WideCharToMultiByte(CP_ACP, 0, c_str(), -1, AnsiString, Size, NULL, NULL);
                    AnsiString[Size]='\0';
                    std::string ToReturn(AnsiString);
                    delete[] AnsiString; //AnsiString=NULL;
                    return ToReturn;
                }
                else
                    return std::string();
            #else //WINDOWS
                //ASCII: the same with all encodings, without the C library or iconv
                if (IsASCII(*this))
                    return std::string(begin(), end());

                const char* CodeSet=Local_Other();
                if (!CodeSet)
                    return To_UTF8_KeepBytes(*this); //ASCII (UTF-8 instead) or UTF-8
                std::string ToReturn;
                #if defined(ZENLIB_LOCAL_ICONV)
                    if (To_Local_Iconv(*this, ToReturn, CodeSet))
                        return ToReturn;
                #endif
                To_Local_C(*this, ToReturn); //All the bytes, there can be a 0 in a character (FreeBSD accepts the end of a string as second byte of an EUC character)
                return ToReturn;
            #endif
        #endif //ZENLIB_USEWX
    #else
        return c_str();
    #endif
}

//---------------------------------------------------------------------------
int128u Ztring::To_UUID () const
{
    if (size()!=36)
        return 0;

    Ztring Temp=*this;

    for (size_t Pos=0; Pos<36; Pos++)
    {
        if ((Temp[Pos]< __T('0') || Temp[Pos]> __T('9'))
         && (Temp[Pos]< __T('A') || Temp[Pos]> __T('F'))
         && (Temp[Pos]< __T('a') || Temp[Pos]> __T('f')))
            return 0;
        if (Temp[Pos]>=__T('A') && Temp[Pos]<=__T('F'))
        {
            Temp[Pos]-=__T('A');
            Temp[Pos]+=__T('9')+1;
        }
        if (Temp[Pos]>=__T('a') && Temp[Pos]<=__T('f'))
        {
            Temp[Pos]-=__T('a');
            Temp[Pos]+=__T('9')+1;
        }

        switch(Pos)
        {
            case  7 :
            case 12 :
            case 17 :
            case 22 :
                        if (at(Pos+1)!=__T('-'))
                            return 0;
                        Pos++; //Skipping dash in the test
        }
    }

    int128u I;
    I.hi=((int64u)((int8u)(Temp[ 0]-'0'))<<60)
       | ((int64u)((int8u)(Temp[ 1]-'0'))<<56)
       | ((int64u)((int8u)(Temp[ 2]-'0'))<<52)
       | ((int64u)((int8u)(Temp[ 3]-'0'))<<48)
       | ((int64u)((int8u)(Temp[ 4]-'0'))<<44)
       | ((int64u)((int8u)(Temp[ 5]-'0'))<<40)
       | ((int64u)((int8u)(Temp[ 6]-'0'))<<36)
       | ((int64u)((int8u)(Temp[ 7]-'0'))<<32)
       | ((int64u)((int8u)(Temp[ 9]-'0'))<<28)
       | ((int64u)((int8u)(Temp[10]-'0'))<<24)
       | ((int64u)((int8u)(Temp[11]-'0'))<<20)
       | ((int64u)((int8u)(Temp[12]-'0'))<<16)
       | ((int64u)((int8u)(Temp[14]-'0'))<<12)
       | ((int64u)((int8u)(Temp[15]-'0'))<< 8)
       | ((int64u)((int8u)(Temp[16]-'0'))<< 4)
       | ((int64u)((int8u)(Temp[17]-'0'))    );
    I.lo=((int64u)((int8u)(Temp[19]-'0'))<<60)
       | ((int64u)((int8u)(Temp[20]-'0'))<<56)
       | ((int64u)((int8u)(Temp[21]-'0'))<<52)
       | ((int64u)((int8u)(Temp[22]-'0'))<<48)
       | ((int64u)((int8u)(Temp[24]-'0'))<<44)
       | ((int64u)((int8u)(Temp[25]-'0'))<<40)
       | ((int64u)((int8u)(Temp[26]-'0'))<<36)
       | ((int64u)((int8u)(Temp[27]-'0'))<<32)
       | ((int64u)((int8u)(Temp[28]-'0'))<<28)
       | ((int64u)((int8u)(Temp[29]-'0'))<<24)
       | ((int64u)((int8u)(Temp[30]-'0'))<<20)
       | ((int64u)((int8u)(Temp[31]-'0'))<<16)
       | ((int64u)((int8u)(Temp[32]-'0'))<<12)
       | ((int64u)((int8u)(Temp[33]-'0'))<< 8)
       | ((int64u)((int8u)(Temp[34]-'0'))<< 4)
       | ((int64u)((int8u)(Temp[35]-'0'))    );

    return I;
}

//---------------------------------------------------------------------------
int32u Ztring::To_CC4 () const
{
    int32u I;
    I =((int32u)((int8u)at(0))<<24)
     | ((int32u)((int8u)at(1))<<16)
     | ((int32u)((int8u)at(2))<< 8)
     | ((int32u)((int8u)at(3))    );

    return I;
}

//---------------------------------------------------------------------------
//Operateur ToInt
int8s Ztring::To_int8s (int8u Radix, ztring_t Options) const
{
    //Integrity
    if (empty())
        return 0;

    //Conversion
    int I;
    #if defined(STREAM_MISSING)
        #ifdef __MINGW32__
            I=_ttoi(c_str());
        #elif defined(UNICODE)
            std::string S=To_UTF8();
            I=atoi(S.c_str());
        #else //UNICODE
            I=atoi(c_str());
        #endif //UNICODE
    #else
        tStringStream Stream(*this);
        Stream >> setbase(Radix) >> I;
        if (Stream.fail())
            return 0;
    #endif

    //Rounded
    if (Options&Ztring_Rounded && find(__T('.'))!=Error)
    {
        float80 F=To_float80();
        F-=I;
        if (F>=0.5f)
            return (int8s)I+1;
    }

    return (int8s)I;
}

//---------------------------------------------------------------------------
//Operateur ToInt
int8u Ztring::To_int8u (int8u Radix, ztring_t Options) const
{
    //Integrity
    if (empty())
        return 0;

    //Conversion
    unsigned int I;
    #if defined(STREAM_MISSING)
        #ifdef __MINGW32__
            I=_ttoi64(c_str()); //TODO : I>0x7FFFFFFF - Replaced by i64 version to support, but not good
        #elif defined(UNICODE)
            std::string S=To_UTF8();
            I=atoi(S.c_str());
        #else //defined(UNICODE)
            I=atoi(c_str());
        #endif //defined(UNICODE)
    #else
        tStringStream Stream(*this);
        Stream >> setbase(Radix) >> I;
        if (Stream.fail())
            return 0;
    #endif

    //Rounded
    if (Options&Ztring_Rounded && find(__T('.'))!=std::string::npos)
    {
        float32 F=To_float32();
        F-=I;
        if (F>=0.5f)
            return (int8u)I+1;
    }

    return (int8u)I;
}

//---------------------------------------------------------------------------
//Operateur ToInt
int16s Ztring::To_int16s (int8u Radix, ztring_t Options) const
{
    //Integrity
    if (empty())
        return 0;

    //Conversion
    int I;
    #if defined(STREAM_MISSING)
        #ifdef __MINGW32__
            I=_ttoi(c_str());
        #elif defined(UNICODE)
            std::string S=To_UTF8();
            I=atoi(S.c_str());
        #else //defined(UNICODE)
            I=atoi(c_str());
        #endif //defined(UNICODE)
    #else
        tStringStream Stream(*this);
        Stream >> setbase(Radix) >> I;
        if (Stream.fail())
            return 0;
    #endif

    //Rounded
    if (Options&Ztring_Rounded && find(__T('.'))!=Error)
    {
        float80 F=To_float80();
        F-=I;
        if (F>=0.5f)
            return (int16s)I+1;
    }

    return (int16s)I;
}

//---------------------------------------------------------------------------
//Operateur ToInt
int16u Ztring::To_int16u (int8u Radix, ztring_t Options) const
{
    //Integrity
    if (empty())
        return 0;

    //Conversion
    unsigned int I;
    #if defined(STREAM_MISSING)
        #ifdef __MINGW32__
            I=_ttoi64(c_str()); //TODO : I>0x7FFFFFFF - Replaced by i64 version to support, but not good
        #elif defined(UNICODE)
            std::string S=To_UTF8();
            I=atoi(S.c_str());
        #else //defined(UNICODE)
            I=atoi(c_str());
        #endif //defined(UNICODE)
    #else
        tStringStream Stream(*this);
        Stream >> setbase(Radix) >> I;
        if (Stream.fail())
            return 0;
    #endif

    //Rounded
    if (Options&Ztring_Rounded && find(__T('.'))!=std::string::npos)
    {
        float32 F=To_float32();
        F-=I;
        if (F>=0.5f)
            return (int16u)I+1;
    }

    return (int16u)I;
}

//---------------------------------------------------------------------------
//Operateur ToInt
int32s Ztring::To_int32s (int8u Radix, ztring_t Options) const
{
    //Integrity
    if (empty())
        return 0;

    //Conversion
    int32s I;
    #if defined(STREAM_MISSING)
        #ifdef __MINGW32__
            I=_ttoi(c_str());
        #elif defined(UNICODE)
            std::string S=To_UTF8();
            I=atol(S.c_str());
        #else //defined(UNICODE)
            I=atol(c_str());
        #endif //defined(UNICODE)
    #else
        tStringStream Stream(*this);
        Stream >> setbase(Radix) >> I;
        if (Stream.fail())
            return 0;
    #endif

    //Rounded
    if (Options&Ztring_Rounded && find(__T('.'))!=Error)
    {
        float80 F=To_float80();
        F-=I;
        if (F>=0.5f)
            return I+1;
    }

    return I;
}

//---------------------------------------------------------------------------
//Operateur ToInt
int32u Ztring::To_int32u (int8u Radix, ztring_t Options) const
{
    //Integrity
    if (empty())
        return 0;

    //Conversion
    int32u I;
    #if defined(STREAM_MISSING)
        #ifdef __MINGW32__
            I=_ttoi64(c_str()); //TODO : I>0x7FFFFFFF - Replaced by i64 version to support, but not good
        #elif defined(UNICODE)
            std::string S=To_UTF8();
            I=atol(S.c_str());
        #else //defined(UNICODE)
            I=atol(c_str());
        #endif //defined(UNICODE)
    #else
        tStringStream Stream(*this);
        Stream >> setbase(Radix) >> I;
        if (Stream.fail())
            return 0;
    #endif

    //Rounded
    if (Options&Ztring_Rounded && find(__T('.'))!=std::string::npos)
    {
        float32 F=To_float32();
        F-=I;
        if (F>=0.5f)
            return I+1;
    }

    return I;
}

//---------------------------------------------------------------------------
//Operateur ToInt
int64s Ztring::To_int64s (int8u Radix, ztring_t Options) const
{
    //Integrity
    if (empty())
        return 0;

    //Conversion
    int64s I;
    #if defined(STREAM_MISSING)
        #ifdef __MINGW32__
            I=_ttoi64(c_str());
        #elif defined(UNICODE)
            std::string S=To_UTF8();
            I=atoll(S.c_str());
        #else //defined(UNICODE)
            I=atoll(c_str());
        #endif //defined(UNICODE)
    #else
        tStringStream Stream(*this);
        Stream >> setbase(Radix) >> I;
        if (Stream.fail())
            return 0;
    #endif

    //Rounded
    if (Options&Ztring_Rounded && find(__T('.'))!=std::string::npos)
    {
        float32 F=To_float32();
        F-=I;
        if (F>0.5f)
            return I+1;
    }

    return I;
}

//---------------------------------------------------------------------------
//Operateur ToInt
int64u Ztring::To_int64u (int8u Radix, ztring_t Options) const
{
    //Integrity
    if (empty())
        return 0;

    //Conversion
    int64u I;
    #if defined(STREAM_MISSING)
        #ifdef __MINGW32__
            I=_ttoi64(c_str()); //TODO : I>0x7FFFFFFFFFFFFFFF
        #elif defined(UNICODE)
            std::string S=To_UTF8();
            I=atoll(S.c_str());
        #else //defined(UNICODE)
            I=atoll(c_str());
        #endif //defined(UNICODE)
    #else
        tStringStream Stream(*this);
        Stream >> setbase(Radix) >> I;
        if (Stream.fail())
            return 0;
    #endif

    //Rounded
    if (Options&Ztring_Rounded && find(__T('.'))!=std::string::npos)
    {
        float32 F=To_float32();
        F-=I;
        if (F>=0.5f)
            return I+1;
    }

    return I;
}

//---------------------------------------------------------------------------
int128u Ztring::To_int128u (int8u, ztring_t) const
{
    if (size()!=32)
        return 0;

    Ztring Temp=*this;

    for (size_t Pos=0; Pos<32; Pos++)
    {
        if ((Temp[Pos]< __T('0') || Temp[Pos]> __T('9'))
         && (Temp[Pos]< __T('A') || Temp[Pos]> __T('F'))
         && (Temp[Pos]< __T('a') || Temp[Pos]> __T('f')))
            return 0;
        if (Temp[Pos]>=__T('A') && Temp[Pos]<=__T('F'))
        {
            Temp[Pos]-=__T('A');
            Temp[Pos]+=__T('9')+1;
        }
        if (Temp[Pos]>=__T('a') && Temp[Pos]<=__T('f'))
        {
            Temp[Pos]-=__T('a');
            Temp[Pos]+=__T('9')+1;
        }
    }

    int128u I;
    I.hi=((int64u)((int8u)(Temp[ 0]-'0'))<<60)
       | ((int64u)((int8u)(Temp[ 1]-'0'))<<56)
       | ((int64u)((int8u)(Temp[ 2]-'0'))<<52)
       | ((int64u)((int8u)(Temp[ 3]-'0'))<<48)
       | ((int64u)((int8u)(Temp[ 4]-'0'))<<44)
       | ((int64u)((int8u)(Temp[ 5]-'0'))<<40)
       | ((int64u)((int8u)(Temp[ 6]-'0'))<<36)
       | ((int64u)((int8u)(Temp[ 7]-'0'))<<32)
       | ((int64u)((int8u)(Temp[ 8]-'0'))<<28)
       | ((int64u)((int8u)(Temp[ 9]-'0'))<<24)
       | ((int64u)((int8u)(Temp[10]-'0'))<<20)
       | ((int64u)((int8u)(Temp[11]-'0'))<<16)
       | ((int64u)((int8u)(Temp[12]-'0'))<<12)
       | ((int64u)((int8u)(Temp[13]-'0'))<< 8)
       | ((int64u)((int8u)(Temp[14]-'0'))<< 4)
       | ((int64u)((int8u)(Temp[15]-'0'))    );
    I.lo=((int64u)((int8u)(Temp[16]-'0'))<<60)
       | ((int64u)((int8u)(Temp[17]-'0'))<<56)
       | ((int64u)((int8u)(Temp[18]-'0'))<<52)
       | ((int64u)((int8u)(Temp[19]-'0'))<<48)
       | ((int64u)((int8u)(Temp[20]-'0'))<<44)
       | ((int64u)((int8u)(Temp[21]-'0'))<<40)
       | ((int64u)((int8u)(Temp[22]-'0'))<<36)
       | ((int64u)((int8u)(Temp[23]-'0'))<<32)
       | ((int64u)((int8u)(Temp[24]-'0'))<<28)
       | ((int64u)((int8u)(Temp[25]-'0'))<<24)
       | ((int64u)((int8u)(Temp[26]-'0'))<<20)
       | ((int64u)((int8u)(Temp[27]-'0'))<<16)
       | ((int64u)((int8u)(Temp[28]-'0'))<<12)
       | ((int64u)((int8u)(Temp[29]-'0'))<< 8)
       | ((int64u)((int8u)(Temp[30]-'0'))<< 4)
       | ((int64u)((int8u)(Temp[31]-'0'))    );

    return I;
}

//---------------------------------------------------------------------------
//Operateur ToFloat
float32 Ztring::To_float32(ztring_t) const
{
    //Integrity
    if (empty())
        return 0;

    //Conversion
    #if defined(STREAM_MISSING)
        #ifdef UNICODE
            return (wcstod(c_str(),NULL));
        #else
            return (strtod(c_str(),NULL));
        #endif
    #else
        float32 F;
        tStringStream Stream(*this);
        Stream >> F;
        if (Stream.fail())
            return 0;

        return F;
    #endif
}

//---------------------------------------------------------------------------
//Operateur ToFloat
float64 Ztring::To_float64(ztring_t) const
{
    //Integrity
    if (empty())
        return 0;

    //Conversion
    #if defined(STREAM_MISSING)
        #ifdef UNICODE
            return (wcstod(c_str(),NULL)); //TODO verify no wcstold
        #else
            return (strtod(c_str(),NULL)); //TODO verify no strtold
        #endif
    #else
        float64 F;
        tStringStream Stream(*this);
        Stream >> F;
        if (Stream.fail())
            return 0;

        return F;
    #endif
}

//---------------------------------------------------------------------------
//Operateur ToFloat
float80 Ztring::To_float80(ztring_t) const
{
    //Integrity
    if (empty())
        return 0;

    //Conversion
    #if defined(STREAM_MISSING)
        #ifdef UNICODE
            return (wcstod(c_str(),NULL)); //TODO verify no wcstold
        #else
            return (strtod(c_str(),NULL)); //TODO verify no strtold
        #endif
    #else
        float80 F;
        tStringStream Stream(*this);
        Stream >> F;
        if (Stream.fail())
            return 0;

        return F;
    #endif
}

//***************************************************************************
// Edition
//***************************************************************************

//---------------------------------------------------------------------------
// Retourne une partie de la chaine
Ztring Ztring::SubString (const tstring &Begin, const tstring &End, size_type Pos, ztring_t Options) const
{
    //Recherche Debut
    size_type I_Debut=find(Begin, Pos);
    if (I_Debut==Error)
        return Ztring();
    I_Debut+=Begin.size();

    //gestion fin NULL
    if (End.empty())
        return substr(I_Debut);

    //Recherche Fin
    size_type I_Fin=find(End, I_Debut);
    if (I_Fin==Error)
    {
        if (Options & Ztring_AddLastItem)
            return substr(I_Debut);
        else
            return Ztring();
    }

    return substr(I_Debut, I_Fin-I_Debut);
}

//---------------------------------------------------------------------------
//FindAndReplace
Ztring::size_type Ztring::FindAndReplace (const ZenLib::tstring &ToFind, const ZenLib::tstring &ReplaceBy, size_type Pos, ZenLib::ztring_t Options)
{
    if (ToFind.empty())
        return 0;

    size_type Count=0;
    size_type Middle=Pos;
    if ((Options&Ztring_Recursive) && ToFind.length()!=ReplaceBy.length())
    {
        //Building a new string in one pass, replacing in place would move the end of the string for each occurrence
        size_type Begin=Pos;
        tstring Result;
        while ((Middle=find(ToFind, Middle))!=npos)
        {
            if (!Count)
            {
                Result.reserve(size());
                Result.append(*this, 0, Begin);
            }
            Result.append(*this, Begin, Middle-Begin);
            Result.append(ReplaceBy);
            Middle+=ToFind.length();
            Begin=Middle;
            Count++;
        }
        if (Count)
        {
            Result.append(*this, Begin, npos);
            assign(Result);
        }
        return Count;
    }

    while (!(Count==1 && !(Options&Ztring_Recursive)) && (Middle=find(ToFind, Middle))!=npos)
    {
        replace(Middle, ToFind.length(), ReplaceBy);
        Middle += ReplaceBy.length();
        Count++;
    }

    return Count;
}

//---------------------------------------------------------------------------
//test if it is a number
bool Ztring::IsNumber() const
{
    if (empty())
        return false;

    bool OK=true;
    size_t Size=size();
    for (size_t Pos=0; Pos<Size; Pos++)
        if (operator[](Pos)<__T('0') || operator[](Pos)>__T('9'))
        {
            OK=false;
            break;
        }
    return OK;
}

//---------------------------------------------------------------------------
//Mise en minuscules
Ztring &Ztring::MakeLowerCase()
{
#if defined(__UNICODE__)
    transform(begin(), end(), begin(), [](wchar_t c) { return towlower(c); });
#else // defined(__UNICODE__)
    transform(begin(), end(), begin(), [](unsigned char c) { return tolower(c); });
#endif // defined(__UNICODE__)
    return *this;
}

//---------------------------------------------------------------------------
// Mise en majuscules
Ztring &Ztring::MakeUpperCase()
{
#if defined(__UNICODE__)
    transform(begin(), end(), begin(), [](wchar_t c) { return towupper(c); });
#else // defined(__UNICODE__)
    transform(begin(), end(), begin(), [](unsigned char c) { return toupper(c); });
#endif // defined(__UNICODE__)
    return *this;
}

//---------------------------------------------------------------------------
// Remove leading whitespaces from a string
Ztring &Ztring::TrimLeft(Char ToTrim)
{
    size_type First=0;
    while (First<size() && operator[](First)==ToTrim)
        First++;
    assign (c_str()+First);
    return *this;
}

//---------------------------------------------------------------------------
// Remove trailing whitespaces from a string
Ztring &Ztring::TrimRight(Char ToTrim)
{
    if (size()==0)
        return *this;

    size_type Last=size()-1;
    while (Last!=(size_type)-1 && operator[](Last)==ToTrim)
        Last--;
    assign (c_str(), Last+1);
    return *this;
}

//---------------------------------------------------------------------------
// Remove leading and trailing whitespaces from a string
Ztring &Ztring::Trim(Char ToTrim)
{
    TrimLeft(ToTrim);
    TrimRight(ToTrim);
    return *this;
}

//---------------------------------------------------------------------------
// Quotes a string
Ztring &Ztring::Quote(Char ToTrim)
{
    assign(tstring(1, ToTrim)+c_str()+ToTrim);
    return *this;
}

//***************************************************************************
// Information
//***************************************************************************

//---------------------------------------------------------------------------
//Count
Ztring::size_type Ztring::Count (const Ztring &ToCount, ztring_t) const
{
    size_type Count=0;
    for (size_type Pos=0; Pos<=size(); Pos++)
        if (find(ToCount, Pos)!=npos)
        {
            Count++;
            Pos+=ToCount.size()-1; //-1 because the loop will add 1
        }
    return Count;
}

//---------------------------------------------------------------------------
//Compare
bool Ztring::Compare (const Ztring &ToCompare, const Ztring &Comparator, ztring_t Options) const
{
    //Integers management
    if (IsNumber() && ToCompare.IsNumber())
    {
        int64s Left=To_int64s();
        int64s Right=ToCompare.To_int64s();
        if (Comparator==__T("==")) return (Left==Right);
        if (Comparator==__T("<"))  return (Left< Right);
        if (Comparator==__T("<=")) return (Left<=Right);
        if (Comparator==__T(">=")) return (Left>=Right);
        if (Comparator==__T(">"))  return (Left> Right);
        if (Comparator==__T("!=")) return (Left!=Right);
        if (Comparator==__T("<>")) return (Left!=Right);
        return false;
    }

    //Case sensitive option
    if (!(Options & Ztring_CaseSensitive))
    {
        //Need to copy strings and make it lowercase
        Ztring Left (c_str());
        Ztring Right (ToCompare.c_str());
        Left.MakeLowerCase();
        Right.MakeLowerCase();

        //string comparasion
        if (Comparator==__T("==")) return (Left==Right);
        if (Comparator==__T("IN")) {if (Left.find(Right)!=string::npos) return true; else return false;}
        if (Comparator==__T("<"))  return (Left< Right);
        if (Comparator==__T("<=")) return (Left<=Right);
        if (Comparator==__T(">=")) return (Left>=Right);
        if (Comparator==__T(">"))  return (Left> Right);
        if (Comparator==__T("!=")) return (Left!=Right);
        if (Comparator==__T("<>")) return (Left!=Right);
        return false;
    }
    else
    {
        //string comparasion
        if (Comparator==__T("==")) return (*this==ToCompare);
        if (Comparator==__T("IN")) {if (this->find(ToCompare)!=string::npos) return true; else return false;}
        if (Comparator==__T("<"))  return (*this< ToCompare);
        if (Comparator==__T("<=")) return (*this<=ToCompare);
        if (Comparator==__T(">=")) return (*this>=ToCompare);
        if (Comparator==__T(">"))  return (*this> ToCompare);
        if (Comparator==__T("!=")) return (*this!=ToCompare);
        if (Comparator==__T("<>")) return (*this!=ToCompare);
        return false;
    }
}

} //namespace

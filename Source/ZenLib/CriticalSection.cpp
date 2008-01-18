// ZenLib::CriticalSection - CriticalSection functions
// Copyright (C) 2007-2008 Jerome Martinez, Zen@MediaArea.net
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#include "ZenLib/Conf_Internal.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifdef ZENLIB_USEWX
    #include <wx/thread.h>
#else //ZENLIB_USEWX
    #ifdef WINDOWS
        #undef __TEXT
        #include <windows.h>
    #endif //WINDOWS
#endif //ZENLIB_USEWX
#include "ZenLib/CriticalSection.h"
//---------------------------------------------------------------------------

namespace ZenLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
CriticalSection::CriticalSection()
{
    #ifdef ZENLIB_USEWX
        CritSect=new wxCriticalSection();
    #else //ZENLIB_USEWX
        #ifdef WINDOWS
            CritSect=new CRITICAL_SECTION;
            InitializeCriticalSection((CRITICAL_SECTION*)CritSect);
        #endif //WINDOWS
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
CriticalSection::~CriticalSection()
{
    #ifdef ZENLIB_USEWX
        delete ((wxCriticalSection*)CritSect); //CritSect=NULL;
    #else //ZENLIB_USEWX
        #ifdef WINDOWS
            DeleteCriticalSection((CRITICAL_SECTION*)CritSect);
            delete ((CRITICAL_SECTION*)CritSect); //CritSect=NULL;
        #endif //WINDOWS
    #endif //ZENLIB_USEWX
}

//***************************************************************************
// Enter/Leave
//***************************************************************************

//---------------------------------------------------------------------------
void CriticalSection::Enter()
{
    #ifdef ZENLIB_USEWX
        ((wxCriticalSection*)CritSect)->Enter();
    #else //ZENLIB_USEWX
        #ifdef WINDOWS
            EnterCriticalSection((CRITICAL_SECTION*)CritSect);
        #endif //WINDOWS
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
void CriticalSection::Leave()
{
    #ifdef ZENLIB_USEWX
        ((wxCriticalSection*)CritSect)->Leave();
    #else //ZENLIB_USEWX
        #ifdef WINDOWS
            LeaveCriticalSection((CRITICAL_SECTION*)CritSect);
        #endif //WINDOWS
    #endif //ZENLIB_USEWX
}

//***************************************************************************
//
//***************************************************************************

} //namespace


// ZenLib::Thread - Thread functions
// Copyright (C) 2007-2009 Jerome Martinez, Zen@MediaArea.net
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
#include "ZenLib/Thread.h"
//---------------------------------------------------------------------------

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ZENLIB_USEWX
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef ZENLIB_USEWX
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#include <wx/thread.h>
//---------------------------------------------------------------------------

namespace ZenLib
{

class ThreadEntry : public wxThread
{
public :
    ThreadEntry(Thread* Th_) : wxThread(wxTHREAD_JOINABLE)
        {Th=Th_;};
    void* Entry() {Th->Entry(); return NULL;}
private :
    Thread* Th;
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Thread::Thread()
{
    ThreadPointer=(void*)new ThreadEntry(this);
    ((ThreadEntry*)ThreadPointer)->Create();
}

//---------------------------------------------------------------------------
Thread::~Thread()
{
    delete (ThreadEntry*)ThreadPointer;
}

//***************************************************************************
// Main Entry
//***************************************************************************

void Thread::Entry()
{
}

//***************************************************************************
// Control
//***************************************************************************

void Thread::Run()
{
    ((ThreadEntry*)ThreadPointer)->Resume();
}

void Thread::Pause()
{
    ((ThreadEntry*)ThreadPointer)->Pause();
}

void Thread::Stop()
{
    ((ThreadEntry*)ThreadPointer)->Delete();
}

bool Thread::IsRunning()
{
    return ((ThreadEntry*)ThreadPointer)->IsRunning();
}

//***************************************************************************
// Communicating
//***************************************************************************

void Thread::Sleep(size_t Millisecond)
{
    ((ThreadEntry*)ThreadPointer)->Sleep((unsigned long)Millisecond);
}

void Thread::Yield()
{
    ((ThreadEntry*)ThreadPointer)->Yield();
}

} //Namespace

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// WINDOWS
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#else //ZENLIB_USEWX
#ifdef WINDOWS
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#undef __TEXT
#include <windows.h>
#undef Yield
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//Config

#ifndef _MT
    #define _MT //Must have this symbol defined to get _beginthread/_endthread declarations
#endif

#ifdef __BORLANDC__
    #if !defined(__MT__)
        #define __MT__ //-tWM in the IDE is not always set
    #endif

    #if !defined(__MFC_COMPAT__)
        #define __MFC_COMPAT__ // Needed to know about _beginthreadex etc..
    #endif
#endif //__BORLANDC__


#if  defined(__VISUALC__) || \
    (defined(__GNUG__) && defined(__MSVCRT__)) || \
     defined(__WATCOMC__) || \
     defined(__MWERKS__)
    //(defined(__BORLANDC__) && (__BORLANDC__ >= 0x500))

    #ifndef __WXWINCE__
        #define USING_BEGINTHREAD //Using _beginthreadex() instead of CreateThread() if possible (better, because of Win32 API has problems with memory leaks in C library)
    #endif
#endif

#ifdef USING_BEGINTHREAD
    #include <process.h>
    typedef unsigned THREAD_RETVAL;     //The return type        of the thread function entry point
    #define THREAD_CALLCONV __stdcall   //The calling convention of the thread function entry point
#else
    // the settings for CreateThread()
    typedef DWORD THREAD_RETVAL;
    #define THREAD_CALLCONV WINAPI
#endif
//---------------------------------------------------------------------------

namespace ZenLib
{

THREAD_RETVAL THREAD_CALLCONV Thread_Start(void *param)
{
    ((Thread*)param)->Entry();
    return 1;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Thread::Thread()
{
    #ifdef USING_BEGINTHREAD
        #ifdef __WATCOMC__
            const unsigned stksize=10240; //Watcom is reported to not like 0 stack size (which means "use default")
        #else
            const unsigned stksize=0; //Default
        #endif //__WATCOMC__

        ThreadPointer=(void*)_beginthreadex (NULL, stksize, Thread_Start, this, CREATE_SUSPENDED, NULL);
    #else
        ThreadPointer=(void*)CreateThread   (NULL,       0, Thread_Start, this, CREATE_SUSPENDED, NULL);
    #endif //USING_BEGINTHREAD
}

//---------------------------------------------------------------------------
Thread::~Thread()
{
    CloseHandle((HANDLE)ThreadPointer);
}

//***************************************************************************
// Main Entry
//***************************************************************************

void Thread::Entry()
{
}

//***************************************************************************
// Control
//***************************************************************************

void Thread::Run()
{
    ResumeThread((HANDLE)ThreadPointer);
}

void Thread::Pause()
{
    SuspendThread((HANDLE)SuspendThread);
}

void Thread::Stop()
{
    SuspendThread((HANDLE)SuspendThread); //Find better
}

bool Thread::IsRunning()
{
    return true; //ToImplement
}

//***************************************************************************
// Communicating
//***************************************************************************

void Thread::Sleep(size_t Millisecond)
{
    ::Sleep((DWORD)Millisecond);
}

void Thread::Yield()
{
    ::Sleep(0);
}

} //Namespace

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// UNIX
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#else //WINDOWS
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

namespace ZenLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Thread::Thread()
{
}

//---------------------------------------------------------------------------
Thread::~Thread()
{
}

//***************************************************************************
// Main Entry
//***************************************************************************

void Thread::Entry()
{
}

//***************************************************************************
// Control
//***************************************************************************

void Thread::Run()
{
}

void Thread::Pause()
{
}

void Thread::Stop()
{
}

bool Thread::IsRunning()
{
    return true; //ToImplement
}

//***************************************************************************
// Communicating
//***************************************************************************

void Thread::Sleep(size_t Millisecond)
{
}

void Thread::Yield()
{
}

} //Namespace

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#endif //WINDOWS
#endif //ZENLIB_USEWX
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


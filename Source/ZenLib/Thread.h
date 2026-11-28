/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Thread functions
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef ZenLib_ThreadH
#define ZenLib_ThreadH
//---------------------------------------------------------------------------
#include "ZenLib/Conf.h"
#include "ZenLib/CriticalSection.h"
#ifdef _WINDOWS
    #undef Yield
#endif
//---------------------------------------------------------------------------

namespace ZenLib
{

//***************************************************************************
/// @brief Thread manipulation
//***************************************************************************

class Thread
{
public :
    //Constructor/Destructor
    Thread  ();
    virtual ~Thread ();

    //Control
    enum returnvalue
    {
        Ok,
        IsNotRunning,
        Incoherent,
        Resource,
    };
    returnvalue Run();
    returnvalue RunAgain();
    returnvalue Pause();
    returnvalue RequestTerminate();
    returnvalue ForceTerminate();
    void        RequestTerminateAndDelete(); //Object deleted at the end of the thread, now if not started or ended

    //Status
    bool        IsRunning();
    bool        IsTerminating();
    bool        IsExited();

    //Configuration
    void        Priority_Set(int8s Priority); //-100 to +100

    //Main Entry
    virtual void Entry();

    //Internal
    returnvalue Internal_Exit(); //Do not use it

protected :

    //Communicating
    void    Sleep(std::size_t Millisecond);
    void    Yield();

private :
    //Internal
    void*   ThreadPointer;

    //The possible states of the thread ("-->" shows all possible transitions from this state)
    enum state
    {
        State_New,              // didn't start execution yet (--> Running)
        State_Running,          // thread is running (--> Paused, Terminating, Terminating_Delete)
        State_Paused,           // thread is temporarily suspended (--> Running, Terminating_Delete)
        State_Terminating,      // thread should terminate a.s.a.p. (--> Terminated, Terminating_Delete)
        State_Terminated,       // thread is terminated
        State_Terminating_Delete, // thread should terminate a.s.a.p., then the object is deleted (--> Terminated)
    };
    state State;
    CriticalSection C;
};

} //NameSpace

#endif

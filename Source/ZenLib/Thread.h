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
//
// Thread functions
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef ZenLib_ThreadH
#define ZenLib_ThreadH
//---------------------------------------------------------------------------
#include "ZenLib/Conf.h"
#undef Yield
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
    ~Thread ();

    //Control
    void    Run();
    void    Pause();
    void    Stop();
    bool    IsRunning();

    //Configuration
    void    Priority_Set(int8s Priority); //-100 to +100

    //Main Entry
    virtual void Entry();

protected :
    //Control
    bool    WantToStop_Get();

    //Communicating
    void    Sleep(size_t Millisecond);
    void    Yield();

private :
    //Internal
    void*   ThreadPointer;
    bool    WantToStop;
};

} //NameSpace

#endif

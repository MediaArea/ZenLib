/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef ZenConf_InternalH
#define ZenConf_InternalH
//---------------------------------------------------------------------------

#include "ZenLib/Conf.h"

//***************************************************************************
// Choice of method
//***************************************************************************

#ifndef ZENLIB_USEWX
    #ifndef WINDOWS
        #define ZENLIB_STANDARD //We select the C/C++ standard as much as possible
    #endif
#endif //ZENLIB_USEWX

//***************************************************************************
// Functions shared by the files of ZenLib, not in its interface
//***************************************************************************

#if defined(__GNUC__) && __GNUC__>=4 && !defined(_WIN32) && !defined(__CYGWIN__)
    #define ZENLIB_INTERNAL __attribute__((visibility("hidden"))) //Not exported by the shared library
#else
    #define ZENLIB_INTERNAL
#endif

#endif

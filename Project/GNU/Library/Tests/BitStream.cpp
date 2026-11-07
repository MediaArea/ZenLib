/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

#include "ZenLib/BitStream_LE.h"

#include <iostream>

using namespace ZenLib;

namespace
{

bool Check(bool Condition, const char* Message)
{
    if (Condition)
        return true;

    std::cerr << Message << '\n';
    return false;
}

bool InvalidReadDoesNotChangePreviousOffset()
{
    const int8u Data[] = { 0x11, 0x22, 0x33 };
    BitStream_LE BitStream(Data, sizeof(Data));

    if (!Check(BitStream.Get(8)==0x11, "Initial read returned an unexpected value"))
        return false;
    if (!Check(BitStream.OffsetBeforeLastCall_Get()==0, "Initial read stored an unexpected previous offset"))
        return false;

    const auto Remain=BitStream.Remain();
    const auto Offset=BitStream.Offset_Get();
    const auto BitOffset=BitStream.BitOffset_Get();
    const auto PreviousOffset=BitStream.OffsetBeforeLastCall_Get();

    if (!Check(BitStream.Get(0)==0, "A zero-bit read returned an unexpected value"))
        return false;
    if (!Check(BitStream.Get(33)==0, "An oversized read returned an unexpected value"))
        return false;
    BitStream.Skip(0);

    if (!Check(BitStream.Remain()==Remain, "An invalid read changed the remaining bit count"))
        return false;
    if (!Check(BitStream.Offset_Get()==Offset, "An invalid read changed the byte offset"))
        return false;
    if (!Check(BitStream.BitOffset_Get()==BitOffset, "An invalid read changed the bit offset"))
        return false;
    if (!Check(BitStream.OffsetBeforeLastCall_Get()==PreviousOffset, "An invalid read changed the previous-call offset"))
        return false;

    if (!Check(BitStream.Get(8)==0x22, "Second valid read returned an unexpected value"))
        return false;
    if (!Check(BitStream.OffsetBeforeLastCall_Get()==1, "A valid read did not update the previous-call offset"))
        return false;

    return true;
}

} // unnamed namespace

int main()
{
    return InvalidReadDoesNotChangePreviousOffset() ? 0 : 1;
}

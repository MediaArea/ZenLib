/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

#include "ZenLib/BitStream_Fast.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <utility>
#include <vector>

using namespace ZenLib;

namespace
{

const int8u Data[] = {
    0x00, 0x01, 0x7F, 0x80, 0xFF, 0x55, 0xAA, 0x12,
    0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x0F,
    0xC3,
};

int Fail(size_t Line, const char* Message)
{
    std::cerr << "Line " << Line << ": " << Message << '\n';
    return 1;
}

#define CHECK(Condition, Message) do { if (!(Condition)) return Fail(__LINE__, Message); } while (0)

int64u ReferenceRead(size_t Offset, size_t Count)
{
    int64u Value=0;
    for (size_t Pos=0; Pos<Count; ++Pos)
    {
        const size_t Bit=Offset+Pos;
        Value=(Value<<1)|((Data[Bit/8]>>(7-Bit%8))&1);
    }
    return Value;
}

int TestReadsAndPeeks()
{
    const size_t BitCount=sizeof(Data)*8;
    for (size_t Offset=0; Offset<BitCount; ++Offset)
    {
        const size_t Max32=std::min<size_t>(32, BitCount-Offset);
        for (size_t Count=1; Count<=Max32; ++Count)
        {
            BitStream_Fast Bits(Data, sizeof(Data));
            Bits.Skip(Offset);
            const int32u Expected=(int32u)ReferenceRead(Offset, Count);
            const size_t Remain=Bits.Remain();
            CHECK(Bits.Peek4((int8u)Count)==Expected, "Peek4 returned an unexpected value");
            CHECK(Bits.Remain()==Remain, "Peek4 changed the remaining bit count");
            CHECK(Bits.Get4((int8u)Count)==Expected, "Get4 returned an unexpected value");
            CHECK(Bits.Remain()==Remain-Count, "Get4 changed the remaining bit count incorrectly");
        }

        const size_t Max64=std::min<size_t>(64, BitCount-Offset);
        for (size_t Count=1; Count<=Max64; ++Count)
        {
            BitStream_Fast Bits(Data, sizeof(Data));
            Bits.Skip(Offset);
            CHECK(Bits.Get8((int8u)Count)==ReferenceRead(Offset, Count), "Get8 returned an unexpected value");
        }
    }
    return 0;
}

int TestNarrowReads()
{
    for (size_t Offset=0; Offset<64; ++Offset)
    {
        for (size_t Count=1; Count<=8; ++Count)
        {
            BitStream_Fast Bits(Data, sizeof(Data));
            Bits.Skip(Offset);
            CHECK(Bits.Get1((int8u)Count)==ReferenceRead(Offset, Count), "Get1 returned an unexpected value");
        }
        for (size_t Count=1; Count<=16; ++Count)
        {
            BitStream_Fast Bits(Data, sizeof(Data));
            Bits.Skip(Offset);
            CHECK(Bits.Get2((int8u)Count)==ReferenceRead(Offset, Count), "Get2 returned an unexpected value");
        }
    }
    return 0;
}

int TestBitsAndAlignment()
{
    BitStream_Fast Bits(Data, sizeof(Data));
    for (size_t Pos=0; Pos<sizeof(Data)*8; ++Pos)
        CHECK(Bits.GetB()==(ReferenceRead(Pos, 1)!=0), "GetB returned an unexpected value");
    CHECK(!Bits.GetB(), "GetB returned data after the buffer end");
    CHECK(Bits.BufferUnderRun(), "GetB did not report a buffer underrun");

    for (size_t Count=1; Count<8; ++Count)
    {
        BitStream_Fast Aligned(Data, sizeof(Data));
        Aligned.Skip(Count);
        Aligned.Byte_Align();
        CHECK(Aligned.Offset_Get()==1, "Byte_Align did not advance to the next byte");
        CHECK(Aligned.BitOffset_Get()==0, "Byte_Align left a non-zero bit offset");
        CHECK(Aligned.Get1(8)==Data[1], "Byte_Align selected the wrong byte");
    }

    BitStream_Fast Cached(Data, sizeof(Data));
    CHECK(Cached.Peek4(32)==ReferenceRead(0, 32), "Peek4 did not prime the cache correctly");
    Cached.Skip(64);
    CHECK(Cached.Get4(32)==ReferenceRead(64, 32), "Skip(64) did not discard a full cache word");
    return 0;
}

int TestBoundariesAndResize()
{
    BitStream_Fast Invalid(Data, sizeof(Data));
    CHECK(Invalid.Get1(9)==0, "Get1 accepted an oversized read");
    CHECK(Invalid.Get2(17)==0, "Get2 accepted an oversized read");
    CHECK(Invalid.Get4(33)==0, "Get4 accepted an oversized read");
    CHECK(Invalid.Remain()==sizeof(Data)*8, "An invalid read changed the remaining bit count");

    BitStream_Fast UnderRun(Data, 1);
    CHECK(UnderRun.Get4(9)==0, "An overread returned data");
    CHECK(UnderRun.BufferUnderRun(), "An overread did not report a buffer underrun");
    CHECK(UnderRun.Remain()==0, "An overread did not exhaust the buffer");
    UnderRun.Resize(8);
    CHECK(!UnderRun.BufferUnderRun(), "Resize did not clear the buffer underrun");
    CHECK(UnderRun.Get1(8)==Data[0], "Resize did not restore readable data");

    BitStream_Fast Resized(Data, sizeof(Data));
    Resized.Resize(16);
    CHECK(Resized.Get2(16)==ReferenceRead(0, 16), "Reading a reduced buffer failed");
    Resized.Resize(sizeof(Data)*8-16);
    CHECK(Resized.Get4(32)==ReferenceRead(16, 32), "Growing a reduced buffer lost cached data");

    BitStream_Fast Attached;
    Attached.Attach(Data, sizeof(Data));
    CHECK(Attached.Get4(32)==ReferenceRead(0, 32), "Attach did not initialize the cache correctly");
    return 0;
}

int TestOptimizedPaths()
{
    for (size_t Offset=0; Offset<sizeof(Data)*8; ++Offset)
    {
        for (size_t Count=1; Count<=64 && Offset+Count<=sizeof(Data)*8; ++Count)
        {
            BitStream_Fast Bits(Data, sizeof(Data));
            Bits.Skip(Offset);
            CHECK(Bits.Get8_Unchecked((int8u)Count)==ReferenceRead(Offset, Count), "Unchecked wide read failed");
            if (Count<=32)
            {
                BitStream_Fast Narrow(Data, sizeof(Data));
                Narrow.Skip(Offset);
                CHECK(Narrow.Get4_Unchecked((int8u)Count)==ReferenceRead(Offset, Count), "Unchecked read failed");
            }
        }
    }

    // Exercise every physical tail length with no padding available to the reader.
    for (size_t Size=1; Size<=sizeof(Data); ++Size)
    {
        const std::vector<int8u> Exact(Data, Data+Size);
        for (size_t Offset=0; Offset<Size*8; ++Offset)
        {
            BitStream_Fast Bits(Exact.data(), Size);
            Bits.Skip(Offset);
            size_t Pos=Offset;
            while (Pos<Size*8)
            {
                const int8u Count=(int8u)std::min<size_t>(64, Size*8-Pos);
                CHECK(Bits.Get8(Count)==ReferenceRead(Pos, Count), "Wide tail read failed");
                Pos+=Count;
            }
            CHECK(Bits.Get8(1)==0 && Bits.BufferUnderRun(), "Wide tail overread was not detected");
        }
    }

    for (size_t Prefix=0; Prefix<8; ++Prefix)
    {
        BitStream_Fast Bits(Data, sizeof(Data));
        Bits.Skip(Prefix);
        Bits.Peek4(32);
        Bits.Resize(16-Prefix);
        CHECK(Bits.Get4((int8u)(17-Prefix))==0 && Bits.BufferUnderRun(), "A hidden cached bit bypassed the size check");
        Bits.Resize(sizeof(Data)*8-Prefix);
        CHECK(Bits.Get8(64)==ReferenceRead(Prefix, 64), "Resize lost hidden cached data");
    }

    for (size_t Count=0; Count<=sizeof(Data)*8; ++Count)
    {
        BitStream_Fast Bits(Data, sizeof(Data));
        Bits.Peek4(32);
        Bits.Skip(Count);
        CHECK(Bits.Remain()==sizeof(Data)*8-Count, "Skip lost the remaining size with a primed cache");
        if (Bits.Remain())
        {
            const int8u Width=(int8u)std::min<size_t>(64, Bits.Remain());
            CHECK(Bits.Get8(Width)==ReferenceRead(Count, Width), "Skip selected wrong data with a primed cache");
        }
    }

    // All 64 cached bits can be hidden while both state flags remain intact.
    for (unsigned Mode=0; Mode<2; ++Mode)
    {
        std::vector<int8u> Padded(sizeof(Data)+8, 0xA5);
        std::copy(Data, Data+sizeof(Data), Padded.begin());
        BitStream_Fast Hidden(Padded.data(), sizeof(Data), Mode?8:0);
        Hidden.Peek4(32);
        Hidden.Resize(0);
        CHECK(!Hidden.GetB() && Hidden.BufferUnderRun(), "A hidden cache exposed data");
        CHECK(!Hidden.GetB() && Hidden.BufferUnderRun(), "Repeated underrun lost the error state");
        Hidden.Resize(sizeof(Data)*8);
        CHECK(!Hidden.BufferUnderRun(), "Resize did not clear the packed error flag");
        CHECK(Hidden.Get8(64)==ReferenceRead(0, 64), "Restoring hidden bits lost data");
        Hidden.Skip(sizeof(Data)*8-64-3);
        CHECK(Hidden.Get4(3)==ReferenceRead(sizeof(Data)*8-3, 3), "Packed flags corrupted the final read");
    }

    BitStream_Fast WideOverread(Data, 5);
    CHECK(WideOverread.Get8(64)==0 && WideOverread.BufferUnderRun(), "Wide overread was not detected");
    // The historical Get8(64) consumed the high 32 bits before the low read failed.
    WideOverread.Resize(8);
    CHECK(WideOverread.Get1(8)==Data[4], "Wide overread changed partial-consumption semantics");

    BitStream_Fast Sticky(Data, sizeof(Data));
    Sticky.Skip(sizeof(Data)*8+1);
    CHECK(Sticky.BufferUnderRun() && Sticky.Remain()==0, "An overrun did not set the error state");
    Sticky.Resize(0);
    CHECK(Sticky.BufferUnderRun(), "Resize(0) cleared a sticky error");
    auto ErrorLocal=Sticky.LocalReader();
    ErrorLocal.Commit();
    CHECK(Sticky.BufferUnderRun(), "A local commit lost the error state");
    Sticky.Resize(sizeof(Data)*8);
    CHECK(!Sticky.BufferUnderRun(), "Growing the buffer did not clear the error state");

    BitStream_Fast Owner(Data, sizeof(Data));
    {
        auto Local=Owner.LocalReader();
        CHECK(Local.Get8(64)==ReferenceRead(0, 64), "Local reader returned wrong data");
        CHECK(Owner.Remain()==sizeof(Data)*8, "Local reader modified its owner before Commit");
    }
    CHECK(Owner.Remain()==sizeof(Data)*8, "Discarding a local reader changed its owner");
    auto Local=Owner.LocalReader();
    CHECK(Local.Get4_Unchecked(17)==ReferenceRead(0, 17), "Local unchecked read failed");
    auto Moved=std::move(Local);
    Local.Commit();
    CHECK(Owner.Remain()==sizeof(Data)*8, "Moved-from local reader committed state");
    Moved.Commit();
    CHECK(Owner.Remain()==sizeof(Data)*8-17, "Commit lost the local position");
    CHECK(Owner.Get8(64)==ReferenceRead(17, 64), "Commit lost the local cache");
    return 0;
}

int TestExtraBytesAvailable()
{
    // Exact allocations make ASan catch any load beyond the declared reserve.
    // Nonzero extra bytes must never affect values or logical limits.
    const size_t ExtraCounts[]={0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 255, 256, 300};
    for (size_t Extra : ExtraCounts)
    for (size_t Size=0; Size<=sizeof(Data); ++Size)
    {
        std::vector<int8u> Padded(Size+Extra, 0xA5);
        std::copy(Data, Data+Size, Padded.begin());
        for (size_t Offset=0; Offset<=Size*8; ++Offset)
        {
            for (size_t Count=1; Count<=64 && Offset+Count<=Size*8; ++Count)
            {
                BitStream_Fast Bits(Padded.data(), Size, Extra);
                Bits.Skip(Offset);
                if (Count<=32)
                    CHECK(Bits.Peek4((int8u)Count)==ReferenceRead(Offset, Count), "Padded peek returned padding bits");
                CHECK(Bits.Get8((int8u)Count)==ReferenceRead(Offset, Count), "Padded wide read failed");
                CHECK(Bits.Remain()==Size*8-Offset-Count, "Padded read changed the logical size incorrectly");

                BitStream_Fast Unchecked(Padded.data(), Size, Extra);
                Unchecked.Skip(Offset);
                CHECK(Unchecked.Get8_Unchecked((int8u)Count)==ReferenceRead(Offset, Count), "Padded unchecked read failed");
            }

            BitStream_Fast Tail(Padded.data(), Size, Extra);
            Tail.Skip(Offset);
            size_t Position=Offset;
            while (Tail.Remain())
            {
                const int8u Count=(int8u)std::min<size_t>(64, Tail.Remain());
                CHECK(Tail.Get8(Count)==ReferenceRead(Position, Count), "Sequential padded read failed");
                Position+=Count;
            }
            CHECK(Tail.GetB()==false && Tail.BufferUnderRun(), "Padding was exposed past the logical end");
        }
    }

    std::vector<int8u> Padded(sizeof(Data)+8, 0x5A);
    std::copy(Data, Data+sizeof(Data), Padded.begin());
    BitStream_Fast Bits(Padded.data(), 1, 8);
    CHECK(Bits.Get1(8)==Data[0], "Padded byte read failed");
    Bits.Resize(sizeof(Data)*8-8);
    CHECK(Bits.Get8(64)==ReferenceRead(8, 64), "Growing a padded buffer lost prefetched bytes");

    Bits.Attach(Padded.data(), sizeof(Data), 8);
    Bits.Peek4(32);
    Bits.Resize(8);
    CHECK(Bits.Get4(9)==0 && Bits.BufferUnderRun(), "Resize exposed padded cached bits");
    Bits.Resize(sizeof(Data)*8);
    CHECK(Bits.Get8(64)==ReferenceRead(0, 64), "Resize did not restore padded cached bits");

    Bits.Attach(Padded.data(), sizeof(Data), 8);
    auto Local=Bits.LocalReader();
    Local.Skip(sizeof(Data)*8-3);
    CHECK(Local.Get4(3)==ReferenceRead(sizeof(Data)*8-3, 3), "Local reader lost the padding mode");
    Local.Commit();
    CHECK(Bits.Remain()==0 && !Bits.BufferUnderRun(), "Local padded commit lost state");

    // Reattaching with the original overload must reset the mode to safe.
    const std::vector<int8u> Exact(Data, Data+1);
    Bits.Attach(Exact.data(), 1);
    CHECK(Bits.Get1(8)==Data[0], "Attach did not reset to safe mode");
    CHECK(!Bits.GetB() && Bits.BufferUnderRun(), "Safe reattachment exposed extra data");
    Bits.Attach(Exact.data(), 1, 0);
    CHECK(Bits.Get1(8)==Data[0], "Explicit safe attachment failed");
    BitStream_Fast Empty(NULL, 0);
    CHECK(!Empty.GetB() && Empty.BufferUnderRun(), "Empty padded buffer was read");
    return 0;
}


// Reference cursor independent of the production cache representation.
struct BitCursorOracle
{
    const int8u* Data;
    size_t Position, Remaining, Initial;
    bool Error;
    BitCursorOracle(const int8u* Data_, size_t Size)
        : Data(Data_), Position(0), Remaining(Size*8), Initial(Size*8), Error(false) {}
    int64u Peek(unsigned Count, unsigned Maximum)
    {
        if (!Count || Count>Maximum) return 0;
        if (Count>Remaining) { Remaining=0; Error=true; return 0; }
        int64u Value=0;
        for (unsigned I=0; I<Count; ++I)
        {
            const size_t Bit=Position+I;
            Value=(Value<<1)|((Data[Bit/8]>>(7-Bit%8))&1);
        }
        return Value;
    }
    int64u Read(unsigned Count, unsigned Maximum)
    {
        const bool Valid=Count && Count<=Maximum && Count<=Remaining;
        const int64u Value=Peek(Count, Maximum);
        if (Valid) { Position+=Count; Remaining-=Count; }
        return Value;
    }
    int64u Wide(unsigned Count)
    {
        if (Count>64) return 0;
        if (Count<=32) return Read(Count, 32);
        const int64u High=Read(Count-32, 32);
        const int64u Low=Read(32, 32);
        return Error?0:(High<<32)|Low;
    }
    void Skip(size_t Count)
    {
        if (Count>Remaining) { Remaining=0; Error=true; return; }
        Position+=Count;
        Remaining-=Count;
    }
    void Resize(size_t Count)
    {
        if (Count>Remaining) Error=false;
        Remaining=Count;
    }
};

// Exercise successive native-word crossings from byte and word cache states.
int TestCacheCrossings()
{
    const size_t Sizes[]={9, 16, 17, 24, 25, 32, 33};
    const size_t Extras[]={0, 3, 16};
    for (size_t Size : Sizes)
    for (size_t Extra : Extras)
    {
        std::vector<int8u> Bytes(Size+Extra, 0xA5);
        for (size_t I=0; I<Size; ++I) Bytes[I]=(int8u)(I*137+19);
        for (unsigned Mode=0; Mode<4; ++Mode)
        for (unsigned Offset=0; Offset<64; ++Offset)
        for (unsigned Width=1; Width<=64 && Offset+Width<=Size*8; ++Width)
        {
            BitStream_Fast Bits(Bytes.data(), Size, Extra);
            BitCursorOracle Oracle(Bytes.data(), Size);
            if (Mode==1)
            {
                for (unsigned I=0; I<Offset; ++I)
                    CHECK(Bits.GetB()==(Oracle.Read(1, 1)!=0), "Byte-cache prefix failed");
            }
            else
            {
                if (Mode==2) Bits.Peek4(32);
                Bits.Skip(Offset);
                Oracle.Skip(Offset);
            }
            if (Mode==3)
            {
                Bits.Peek4(1);
                Bits.Resize(Width);
                Bits.Resize(Oracle.Remaining);
            }
            CHECK(Bits.Get8((int8u)Width)==Oracle.Read(Width, 64), "Native-word crossing failed");
            while (Oracle.Remaining)
            {
                const unsigned Count=(unsigned)std::min<size_t>(64, Oracle.Remaining);
                CHECK(Bits.Get8_Unchecked((int8u)Count)==Oracle.Read(Count, 64), "Successive word crossing failed");
                CHECK(Bits.Remain()==Oracle.Remaining, "Word crossing changed the logical size");
            }
            CHECK(!Bits.BufferUnderRun(), "A valid word crossing set the error flag");
        }
    }
    return 0;
}


unsigned MixedRandom(unsigned& State)
{
    State=State*1664525+1013904223;
    return State>>8;
}

int TestMixedOperations()
{
    const size_t Size=257;
    std::vector<int8u> Bytes(Size+16, 0xA5);
    unsigned Random=198;
    for (size_t I=0; I<Size; ++I) Bytes[I]=(int8u)MixedRandom(Random);
    const size_t ExtraCounts[]={0, 1, 2, 7, 8, 15, 16};
    for (size_t Extra : ExtraCounts)
    {
        BitStream_Fast Bits(Bytes.data(), Size, Extra);
        BitCursorOracle Oracle(Bytes.data(), Size);
        for (unsigned Step=0; Step<20000; ++Step)
        {
            const unsigned Operation=MixedRandom(Random)%17;
            const int8u Width=(int8u)(MixedRandom(Random)%71);
            int64u Expected=0, Actual=0;
            switch (Operation)
            {
                case 0: Expected=Oracle.Read(1, 1); Actual=Bits.GetB(); break;
                case 1: Expected=Oracle.Read(Width, 8); Actual=Bits.Get1(Width); break;
                case 2: Expected=Oracle.Read(Width, 16); Actual=Bits.Get2(Width); break;
                case 3: Expected=Oracle.Read(Width, 32); Actual=Bits.Get4(Width); break;
                case 4: Expected=Oracle.Wide(Width); Actual=Bits.Get8(Width); break;
                case 5: Expected=Oracle.Peek(Width, 8); Actual=Bits.Peek1(Width); break;
                case 6: Expected=Oracle.Peek(Width, 16); Actual=Bits.Peek2(Width); break;
                case 7: Expected=Oracle.Peek(Width, 32); Actual=Bits.Peek4(Width); break;
                case 8: Expected=Oracle.Peek(1, 1); Actual=Bits.PeekB(); break;
                case 9: Expected=Oracle.Peek(Width, 64); Actual=Bits.Peek8(Width); break;
                case 10: Oracle.Skip(Width); Bits.Skip(Width); break;
                case 11: Oracle.Skip(Oracle.Remaining%8); Bits.Byte_Align(); break;
                case 12:
                {
                    // An underrun resets the legacy reader's logical alignment
                    // to zero. Reattach if the physical position is unaligned.
                    if (Oracle.Error && Oracle.Position%8)
                    {
                        Bits.Attach(Bytes.data(), Size, Extra);
                        Oracle=BitCursorOracle(Bytes.data(), Size);
                        break;
                    }
                    const size_t Alignment=Oracle.Remaining%8;
                    const size_t Maximum=Size*8-Oracle.Position;
                    const size_t Count=Alignment+8*(MixedRandom(Random)%((Maximum-Alignment)/8+1));
                    Oracle.Resize(Count);
                    Bits.Resize(Count);
                    break;
                }
                case 13:
                {
                    const size_t Attached=MixedRandom(Random)%(Size+1);
                    Bits.Attach(Bytes.data(), Attached, Extra);
                    Oracle=BitCursorOracle(Bytes.data(), Attached);
                    break;
                }
                case 14:
                case 15:
                    if (Oracle.Remaining)
                    {
                        const unsigned Limit=(unsigned)std::min<size_t>(Operation==14?32:64, Oracle.Remaining);
                        const int8u Count=(int8u)(1+MixedRandom(Random)%Limit);
                        Expected=Oracle.Read(Count, Limit);
                        Actual=Operation==14?Bits.Get4_Unchecked(Count):Bits.Get8_Unchecked(Count);
                    }
                    break;
                case 16:
                {
                    BitCursorOracle Temporary=Oracle;
                    auto Local=Bits.LocalReader();
                    Expected=Temporary.Wide(Width);
                    Actual=Local.Get8(Width);
                    if (MixedRandom(Random)&1) { Local.Commit(); Oracle=Temporary; }
                    break;
                }
            }
            if (Actual!=Expected || Bits.Remain()!=Oracle.Remaining || Bits.BufferUnderRun()!=Oracle.Error)
                std::cerr << "Step=" << Step << " Op=" << Operation << " Width=" << unsigned(Width)
                          << " Position=" << Oracle.Position << " Extra=" << Extra << '\n';
            CHECK(Actual==Expected, "Mixed operations returned unexpected data");
            CHECK(Bits.Remain()==Oracle.Remaining, "Mixed operations lost remaining bits");
            CHECK(Bits.BufferUnderRun()==Oracle.Error, "Mixed operations lost error state");
            CHECK(Bits.Offset_Get()==(Oracle.Initial-Oracle.Remaining)/8, "Mixed operations changed byte offset semantics");
            CHECK(Bits.BitOffset_Get()==Oracle.Remaining%8, "Mixed operations changed bit offset semantics");
        }
    }
    return 0;
}

// Peek8 reads up to 64 bits without consuming them, from any bit offset,
// with exact allocations and with reserves.
int TestPeek8()
{
    const size_t Extras[]={0, 1, 7, 8};
    for (size_t Extra : Extras)
    for (size_t Size=1; Size<=sizeof(Data); ++Size)
    {
        std::vector<int8u> Padded(Size+Extra, 0xA5);
        std::copy(Data, Data+Size, Padded.begin());
        for (size_t Offset=0; Offset<=Size*8; ++Offset)
        {
            for (size_t Count=0; Count<=66; ++Count)
            {
                BitStream_Fast Bits(Padded.data(), Size, Extra);
                Bits.Skip(Offset);
                const size_t Remain=Bits.Remain();
                const int64u Value=Bits.Peek8((int8u)Count);
                if (!Count || Count>64)
                {
                    CHECK(Value==0 && Bits.Remain()==Remain && !Bits.BufferUnderRun(), "Peek8 accepted an invalid size");
                    continue;
                }
                if (Count>Remain)
                {
                    CHECK(Value==0 && !Bits.Remain() && Bits.BufferUnderRun(), "Peek8 overread was not detected");
                    continue;
                }
                CHECK(Value==ReferenceRead(Offset, Count), "Peek8 returned an unexpected value");
                CHECK(Bits.Remain()==Remain && !Bits.BufferUnderRun(), "Peek8 consumed data");
                CHECK(Bits.Get8((int8u)Count)==Value, "Get8 after Peek8 returned another value");
            }
        }
    }

    // Peek8 does not disturb a following sequence of small reads
    BitStream_Fast Bits(Data, sizeof(Data));
    for (size_t Pos=0; Pos+64<=sizeof(Data)*8; Pos+=3)
    {
        CHECK(Bits.Peek8(64)==ReferenceRead(Pos, 64), "Peek8(64) in a sequence failed");
        CHECK(Bits.Get1(3)==ReferenceRead(Pos, 3), "Read after Peek8 failed");
    }
    return 0;
}

//---------------------------------------------------------------------------
// Exp-Golomb (GetUE/GetSE), with at most 31 leading zero bits: a longer or
// truncated code returns 0 and sets the underrun state. Also tiny buffers.
//---------------------------------------------------------------------------

unsigned ExpGolombRandom(unsigned& State)
{
    State=State*1664525+1013904223;
    return State>>8;
}

struct BitWriter
{
    std::vector<int8u> Bytes;
    size_t Bits;
    BitWriter() : Bits(0) {}
    void Put(int64u Value, unsigned Count)
    {
        for (unsigned I=Count; I-->0;)
        {
            if (Bits%8==0)
                Bytes.push_back(0);
            if ((Value>>I)&1)
                Bytes.back()|=(int8u)(0x80>>(Bits%8));
            Bits++;
        }
    }
    void PutUE(int32u Value) // Value <= 0xFFFFFFFE
    {
        const int64u CodeNum=(int64u)Value+1;
        unsigned LeadingZeroBits=0;
        while ((CodeNum>>LeadingZeroBits)>1)
            LeadingZeroBits++;
        Put(0, LeadingZeroBits);
        Put(CodeNum, LeadingZeroBits+1);
    }
    void PutSE(int32s Value) // Value != INT32_MIN
    {
        PutUE(Value>0?(int32u)Value*2-1:(int32u)(-(int64s)Value)*2);
    }
};

// Reference Exp-Golomb decoder: position-based, no cache
struct ExpGolombOracle
{
    const int8u* Data;
    size_t Position, Remaining;
    bool Error;
    ExpGolombOracle(const int8u* Data_, size_t Size) : Data(Data_), Position(0), Remaining(Size*8), Error(false) {}
    bool Bit(size_t Pos) const { return ((Data[Pos/8]>>(7-Pos%8))&1)!=0; }
    int32u UE()
    {
        size_t LeadingZeroBits=0;
        while (LeadingZeroBits<Remaining && !Bit(Position+LeadingZeroBits))
            LeadingZeroBits++;
        if (LeadingZeroBits>31 || LeadingZeroBits*2+1>Remaining)
        {
            Remaining=0;
            Error=true;
            return 0;
        }
        int64u Value=0;
        for (size_t I=0; I<=LeadingZeroBits; I++)
            Value=(Value<<1)|(int64u)Bit(Position+LeadingZeroBits+I);
        Position+=LeadingZeroBits*2+1;
        Remaining-=LeadingZeroBits*2+1;
        return (int32u)(Value-1);
    }
    int64u Read(unsigned Count, unsigned Maximum)
    {
        if (!Count || Count>Maximum) return 0;
        if (Count>Remaining) { Remaining=0; Error=true; return 0; }
        int64u Value=0;
        for (unsigned I=0; I<Count; I++)
            Value=(Value<<1)|(int64u)Bit(Position+I);
        Position+=Count;
        Remaining-=Count;
        return Value;
    }
    void Skip(size_t Count)
    {
        if (Count>Remaining) { Remaining=0; Error=true; return; }
        Position+=Count;
        Remaining-=Count;
    }
};

int32s ToSE(int32u CodeNum)
{
    return (CodeNum&1)?(int32s)((CodeNum>>1)+1):-(int32s)(CodeNum>>1);
}

// Every prefix length (0..33 leading zero bits), every bit offset, exact allocations
int TestExpGolombLengths()
{
    unsigned Random=7;
    const size_t Extras[]={0, 1, 3, 7, 8, 16};
    for (unsigned LeadingZeroBits=0; LeadingZeroBits<=33; LeadingZeroBits++)
    for (unsigned Offset=0; Offset<20; Offset++)
    for (size_t Extra : Extras)
    {
        BitWriter W;
        W.Put(ExpGolombRandom(Random), Offset);
        W.Put(0, LeadingZeroBits);
        W.Put(1, 1);
        const int64u Suffix=((int64u)ExpGolombRandom(Random)<<32)|ExpGolombRandom(Random);
        W.Put(Suffix, LeadingZeroBits);
        const size_t CodeEnd=W.Bits;
        // Every truncation of the stream, from the code end down to the code start
        for (size_t End=Offset; End<=((CodeEnd+7)&~(size_t)7); End+=1)
        {
            if (End%8) continue; // sizes are in bytes
            std::vector<int8u> Exact(W.Bytes.begin(), W.Bytes.begin()+std::min(End/8, W.Bytes.size()));
            Exact.resize(End/8+Extra, 0xFF); // the reserve holds ones: they must never be decoded
            BitStream_Fast Bits(Exact.data(), End/8, Extra);
            ExpGolombOracle O(Exact.data(), End/8);
            Bits.Skip(Offset);
            O.Skip(Offset);
            const int32u Expected=O.UE();
            const int32u Actual=Bits.GetUE();
            CHECK(Actual==Expected, "GetUE returned an unexpected value");
            CHECK(Bits.Remain()==O.Remaining, "GetUE left an unexpected remaining size");
            CHECK(Bits.BufferUnderRun()==O.Error, "GetUE error state is wrong");
            if (O.Error)
                CHECK(Expected==0, "ExpGolombOracle error value");
        }
    }
    return 0;
}

int TestExpGolombSequences()
{
    unsigned Random=1234;
    for (unsigned Round=0; Round<200; Round++)
    {
        BitWriter W;
        std::vector<int64s> Values;
        std::vector<int> Kinds;
        const unsigned Count=1+ExpGolombRandom(Random)%64;
        for (unsigned I=0; I<Count; I++)
        {
            const int Kind=ExpGolombRandom(Random)%4;
            Kinds.push_back(Kind);
            if (Kind==0)
            {
                const unsigned Magnitude=ExpGolombRandom(Random)%33;
                int32u V=Magnitude>=32?0xFFFFFFFEu:(ExpGolombRandom(Random)&((1u<<Magnitude)-1));
                if (Magnitude==32) V=0xFFFFFFFEu - (ExpGolombRandom(Random)%3);
                W.PutUE(V);
                Values.push_back(V);
            }
            else if (Kind==1)
            {
                const unsigned Magnitude=ExpGolombRandom(Random)%32;
                int32s V=(int32s)(ExpGolombRandom(Random)&((1u<<Magnitude)-1));
                if (ExpGolombRandom(Random)&1) V=-V;
                W.PutSE(V);
                Values.push_back(V);
            }
            else if (Kind==2)
            {
                const unsigned Width=1+ExpGolombRandom(Random)%32;
                const int32u V=ExpGolombRandom(Random)&(Width==32?0xFFFFFFFFu:((1u<<Width)-1));
                W.Put(V, Width);
                Values.push_back(((int64s)Width<<32)|V);
            }
            else
            {
                W.Put(ExpGolombRandom(Random)&1, 1);
                Values.push_back((W.Bytes.back()>>(7-(W.Bits-1)%8))&1);
            }
        }
        const size_t Size=W.Bytes.size();
        const size_t Extra=ExpGolombRandom(Random)%10;
        std::vector<int8u> Exact(W.Bytes);
        Exact.resize(Size+Extra, 0xA5);
        BitStream_Fast Bits(Exact.data(), Size, Extra);
        for (unsigned I=0; I<Count; I++)
        {
            switch (Kinds[I])
            {
                case 0: CHECK(Bits.GetUE()==(int32u)Values[I], "Sequential GetUE failed"); break;
                case 1: CHECK(Bits.GetSE()==(int32s)Values[I], "Sequential GetSE failed"); break;
                case 2: CHECK(Bits.Get4((int8u)(Values[I]>>32))==(int32u)Values[I], "Interleaved Get4 failed"); break;
                case 3: CHECK(Bits.GetB()==(Values[I]!=0), "Interleaved GetB failed"); break;
            }
        }
        CHECK(!Bits.BufferUnderRun(), "A valid sequence set the error state");
        CHECK(Bits.Remain()==Size*8-W.Bits, "Sequence consumed an unexpected size");
        // Whatever is left is padding (zeros): a ue(v) there is truncated or too long
        if (Bits.Remain())
        {
            ExpGolombOracle O(Exact.data(), Size);
            O.Skip(W.Bits);
            CHECK(Bits.GetUE()==O.UE(), "Trailing GetUE failed");
            CHECK(Bits.Remain()==O.Remaining && Bits.BufferUnderRun()==O.Error, "Trailing GetUE state failed");
        }
    }
    return 0;
}

int TestSignedExpGolomb()
{
    const int32s Values[]={0, 1, -1, 2, -2, 3, 1000, -1000, 0x7FFFFFFF, -0x7FFFFFFF};
    for (int32s V : Values)
    {
        BitWriter W;
        W.PutSE(V);
        W.Put(1, 1);
        BitStream_Fast Bits(W.Bytes.data(), W.Bytes.size());
        CHECK(Bits.GetSE()==V, "GetSE returned an unexpected value");
        CHECK(Bits.GetB(), "GetSE consumed an unexpected size");
    }
    // Error value
    const int8u Zeros[8]={0, 0, 0, 0, 0, 0, 0, 0};
    BitStream_Fast Bits(Zeros, 8);
    CHECK(Bits.GetSE()==0, "GetSE error value is not 0");
    CHECK(Bits.BufferUnderRun() && !Bits.Remain(), "GetSE error state");
    for (int32u K=0; K<100000; K++)
    {
        const int32s Expected=(K&1)?(int32s)((K+1)/2):-(int32s)(K/2);
        CHECK(ToSE(K)==Expected, "se(v) mapping");
    }
    return 0;
}

// Error then growing Resize(): the position is preserved and data is readable again
int TestErrorRecovery()
{
    BitWriter W;
    W.Put(0x5, 3);
    W.PutUE(1000);
    W.PutUE(3);
    const size_t Size=W.Bytes.size();
    BitStream_Fast Bits(W.Bytes.data(), Size);
    CHECK(Bits.Get1(3)==5, "Prefix read failed");
    Bits.Resize(5);
    CHECK(Bits.GetUE()==0, "Truncated GetUE did not return 0");
    CHECK(Bits.BufferUnderRun() && !Bits.Remain(), "Truncated GetUE error state");
    Bits.Resize(Size*8-3);
    CHECK(!Bits.BufferUnderRun(), "Growing Resize did not clear the error");
    CHECK(Bits.GetUE()==1000, "GetUE after recovery failed");
    CHECK(Bits.GetUE()==3, "Second GetUE after recovery failed");

    // Local reader
    BitStream_Fast Owner(W.Bytes.data(), Size);
    Owner.Skip(3);
    {
        BitStream_Fast_Local Local=Owner.LocalReader();
        CHECK(Local.GetUE()==1000, "Local GetUE failed");
        CHECK(Owner.Remain()==Size*8-3, "Local GetUE changed the owner");
        Local.Commit();
    }
    CHECK(Owner.GetUE()==3, "Committed local GetUE lost the position");
    return 0;
}

// Tiny buffers (0..9 bytes), all operations, with and without reserve, against the oracle
int TestTinyBuffers()
{
    unsigned Random=99;
    for (unsigned Round=0; Round<20000; Round++)
    {
        const size_t Size=ExpGolombRandom(Random)%10;
        const size_t Extra=ExpGolombRandom(Random)%3==0?ExpGolombRandom(Random)%9:0;
        std::vector<int8u> Exact(Size+Extra);
        for (size_t I=0; I<Exact.size(); I++)
            Exact[I]=(int8u)ExpGolombRandom(Random);
        BitStream_Fast Bits(Exact.empty()?NULL:Exact.data(), Size, Extra);
        ExpGolombOracle O(Exact.empty()?NULL:Exact.data(), Size);
        for (unsigned Step=0; Step<12; Step++)
        {
            const unsigned Operation=ExpGolombRandom(Random)%7;
            const unsigned Width=ExpGolombRandom(Random)%34;
            int64u Expected=0, Actual=0;
            switch (Operation)
            {
                case 0: Expected=O.Read(1, 1); Actual=Bits.GetB(); break;
                case 1: Expected=O.Read(Width, 8); Actual=Bits.Get1((int8u)Width); break;
                case 2: Expected=O.Read(Width, 16); Actual=Bits.Get2((int8u)Width); break;
                case 3: Expected=O.Read(Width, 32); Actual=Bits.Get4((int8u)Width); break;
                case 4: O.Skip(Width%9); Bits.Skip(Width%9); break;
                case 5: Expected=O.UE(); Actual=Bits.GetUE(); break;
                case 6:
                {
                    ExpGolombOracle P=O;
                    Expected=P.Read(Width, 32);
                    Actual=Bits.Peek4((int8u)Width);
                    if (P.Error) O=P; // a failing peek is an underrun
                    break;
                }
            }
            CHECK(Actual==Expected, "Tiny buffer operation returned unexpected data");
            CHECK(Bits.Remain()==O.Remaining, "Tiny buffer remaining size");
            CHECK(Bits.BufferUnderRun()==O.Error, "Tiny buffer error state");
        }
    }
    return 0;
}


//---------------------------------------------------------------------------
// A growing Resize() may move the logical end a few bits beyond the attached
// size, here into the reserve: the reader must then still never load beyond
// the attached size plus the reserve.
// The allocation is exact, so an address sanitizer reports any overread.
//---------------------------------------------------------------------------

int TestResizeBeyondEnd()
{
    unsigned Random=4321;
    for (size_t Size=1; Size<=sizeof(Data); ++Size)
    for (size_t Extra=1; Extra<=9; ++Extra)
    for (size_t Consumed=0; Consumed<=Size*8; Consumed+=3)
    for (size_t Beyond=1; Beyond<=7; ++Beyond)
    {
        std::vector<int8u> Exact(Size+Extra);
        for (size_t I=0; I<Exact.size(); I++)
            Exact[I]=I<Size?Data[I]:(int8u)ExpGolombRandom(Random);
        BitStream_Fast Bits(Exact.data(), Size, Extra);
        ExpGolombOracle O(Exact.data(), Size+1); // the oracle sees the byte after the attached size
        Bits.Skip(Consumed);
        O.Skip(Consumed);
        Bits.Resize(Size*8-Consumed+Beyond);
        O.Remaining=Size*8-Consumed+Beyond;
        CHECK(Bits.Remain()==O.Remaining && !Bits.BufferUnderRun(), "Resize beyond the attached size");
        for (unsigned Step=0; Step<8 && O.Remaining; Step++)
        {
            const unsigned Operation=ExpGolombRandom(Random)%6;
            const unsigned Width=1+ExpGolombRandom(Random)%32;
            int64u Expected=0, Actual=0;
            switch (Operation)
            {
                case 0: Expected=O.Read(1, 1); Actual=Bits.GetB(); break;
                case 1: Expected=O.Read(Width%9, 8); Actual=Bits.Get1((int8u)(Width%9)); break;
                case 2: Expected=O.Read(Width, 32); Actual=Bits.Get4((int8u)Width); break;
                case 3:
                {
                    ExpGolombOracle P=O;
                    Expected=P.Read(Width, 32);
                    Actual=Bits.Peek4((int8u)Width);
                    if (P.Error) O=P;
                    break;
                }
                case 4: Expected=O.UE(); Actual=Bits.GetUE(); break;
                case 5: O.Skip(Width%9); Bits.Skip(Width%9); break;
            }
            CHECK(Actual==Expected, "Read after a Resize beyond the attached size returned unexpected data");
            CHECK(Bits.Remain()==O.Remaining && Bits.BufferUnderRun()==O.Error, "State after a Resize beyond the attached size");
        }
    }
    return 0;
}

//---------------------------------------------------------------------------
// Resize() to a count whose alignment differs from Remain() keeps the exact
// position, only the logical end moves. Pattern: a region rounded up to the
// byte grid of the end is skipped to its end, then the end is restored.
// Resuming at the declared end of the region needs a copy of the reader
// taken before the region.
//---------------------------------------------------------------------------

int TestResizeAlignment()
{
    std::vector<int8u> Exact(sizeof(Data)+1, 0x5A); // The logical end may go up to 7 bits into the reserve
    std::copy(Data, Data+sizeof(Data), Exact.begin());
    for (size_t Start=0; Start<=24; Start++)
    for (size_t Region=0; Start+Region<=sizeof(Data)*8; Region+=3)
    {
        const size_t End=sizeof(Data)*8-Start-Region;
        BitStream_Fast Bits(Exact.data(), sizeof(Data), 1);
        Bits.Skip(Start);
        const BitStream_Fast Saved=Bits;
        Bits.Resize(Region+End%8);
        Bits.Skip(Bits.Remain());
        Bits.Resize(End);
        ExpGolombOracle O(Exact.data(), sizeof(Data)+1);
        O.Skip(Start+Region+End%8);
        O.Remaining=End;
        CHECK(Bits.Remain()==End && !Bits.BufferUnderRun(), "Resize with another alignment changed the state");
        const unsigned Width=(unsigned)MIN(End, (size_t)32);
        if (Width)
            CHECK(Bits.Peek4((int8u)Width)==O.Read(Width, 32), "Resize with another alignment moved the position");

        // From the copy, the declared end of the region
        Bits=Saved;
        Bits.Skip(Region);
        CHECK(Bits.Remain()==End, "Copy of the reader lost the logical end");
        if (Width)
            CHECK(Bits.Peek4((int8u)Width)==ReferenceRead(Start+Region, Width), "Copy of the reader lost the position");
    }
    return 0;
}

//---------------------------------------------------------------------------
// Sizes near the size_t limit, with 32-bit and 64-bit words: 2^(N-1) bits or
// more after the word start must not be taken for the end of the buffer, and
// the Exp-Golomb and Resize() computations must not overflow. The reader only
// loads at the position, so a small allocation is enough when only its first
// bytes are read.
//---------------------------------------------------------------------------

int TestHugeSizes()
{
    int8u Bytes[64];
    unsigned Random=2468;
    for (size_t I=0; I<sizeof(Bytes); I++)
        Bytes[I]=(int8u)(ExpGolombRandom(Random)|1); // Exp-Golomb codes of up to 7 leading zero bits
    const size_t Sizes[]={((size_t)1<<(sizeof(size_t)*8-4))+8, (size_t)-1/8};
    for (size_t Size : Sizes)
    {
        BitStream_Fast Bits(Bytes, Size);
        ExpGolombOracle O(Bytes, Size);
        for (unsigned Step=0; Step<200 && O.Position<300; Step++)
        {
            const unsigned Operation=ExpGolombRandom(Random)%6;
            const unsigned Width=1+ExpGolombRandom(Random)%32;
            int64u Expected=0, Actual=0;
            switch (Operation)
            {
                case 0: Expected=O.Read(1, 1); Actual=Bits.GetB(); break;
                case 1: Expected=O.Read(Width%9, 8); Actual=Bits.Get1((int8u)(Width%9)); break;
                case 2: Expected=O.Read(Width, 32); Actual=Bits.Get4((int8u)Width); break;
                case 3: Expected=O.Read(Width, 64); Actual=Bits.Get8((int8u)(Width*2)); break;
                case 4: Expected=O.UE(); Actual=Bits.GetUE(); break;
                case 5: O.Skip(Width); Bits.Skip(Width); break;
            }
            if (Operation==3)
                Expected=(Expected<<Width)|O.Read(Width, 64);
            CHECK(Actual==Expected, "Read with a huge size returned unexpected data");
            CHECK(Bits.Remain()==O.Remaining && !Bits.BufferUnderRun(), "State with a huge size");
        }
    }

    // Exp-Golomb codes too long for Window near the size_t limit: 20 leading zero bits
    // (slow path with 32-bit words) and 36 (more than 31, an error)
    const unsigned LeadingZeroBits[]={20, 36};
    for (unsigned Zeros : LeadingZeroBits)
    for (size_t Skipped=0; Skipped<8; Skipped++)
    {
        int8u Code[24];
        std::memset(Code, 0, sizeof(Code));
        const size_t One=Skipped+Zeros;
        Code[One/8]|=(int8u)(0x80>>(One%8));
        Code[sizeof(Code)-1]=0xFF;
        BitStream_Fast Bits(Code, (size_t)-1/8);
        ExpGolombOracle O(Code, (size_t)-1/8);
        Bits.Skip(Skipped);
        O.Skip(Skipped);
        const int32u Expected=O.UE();
        CHECK(Bits.GetUE()==Expected, "Long Exp-Golomb code near the size_t limit");
        CHECK(Bits.BufferUnderRun()==O.Error && Bits.Remain()==O.Remaining, "Long Exp-Golomb code state near the size_t limit");
    }

    // Resize() to a logical end near the size_t limit
    for (size_t Skipped=0; Skipped<64; Skipped+=7)
    {
        BitStream_Fast Bits(Bytes, sizeof(Bytes));
        Bits.Skip(Skipped);
        Bits.Resize((size_t)-1-Skipped);
        CHECK(Bits.Remain()==(size_t)-1-Skipped && !Bits.BufferUnderRun(), "Resize to a huge size");
        ExpGolombOracle O(Bytes, sizeof(Bytes));
        O.Skip(Skipped);
        CHECK(Bits.Get4(32)==O.Read(32, 32), "Read after a Resize to a huge size");
        const int64u High=O.Read(32, 32);
        const int64u Low=O.Read(32, 32);
        CHECK(Bits.Get8(64)==((High<<32)|Low), "Wide read after a Resize to a huge size");
    }
    return 0;
}

//---------------------------------------------------------------------------
// Refresh() after modifying attached bytes not read yet, as a descrambling in
// place does (Dolby E): the reads then see the new bytes, from the same
// position. The byte holding the position keeps its bits already read.
//---------------------------------------------------------------------------

int TestRefresh()
{
    const size_t Extras[]={0, 1, 7};
    for (size_t Extra : Extras)
    for (size_t Size=0; Size<=sizeof(Data); Size++)
    for (size_t Consumed=0; Consumed<=Size*8; Consumed++)
    {
        std::vector<int8u> Bytes(Size+Extra, 0x3C);
        std::copy(Data, Data+Size, Bytes.begin());
        BitStream_Fast Bits(Bytes.empty()?NULL:Bytes.data(), Size, Extra);
        const size_t Read=MIN(Consumed, (size_t)32);
        if (Read)
            CHECK(Bits.Get4((int8u)Read)==ReferenceRead(0, Read), "Read before Refresh");
        Bits.Skip(Consumed-Read);
        for (size_t I=Consumed/8; I<Size; I++)
        {
            int8u Mask=(int8u)(0xA5^(I*29));
            if (I==Consumed/8)
                Mask&=(int8u)(0xFF>>(Consumed%8));
            Bytes[I]^=Mask;
        }
        Bits.Refresh();
        CHECK(Bits.Remain()==Size*8-Consumed && !Bits.BufferUnderRun(), "Refresh changed the state");
        ExpGolombOracle O(Bytes.empty()?NULL:Bytes.data(), Size);
        O.Skip(Consumed);
        while (O.Remaining)
        {
            const unsigned Count=(unsigned)MIN(O.Remaining, (size_t)32);
            CHECK(Bits.Get4((int8u)Count)==O.Read(Count, 32), "Read after Refresh returned stale data");
        }
        Bits.Refresh();
        CHECK(!Bits.Remain() && !Bits.BufferUnderRun(), "Refresh at the end changed the state");
        CHECK(!Bits.GetB() && Bits.BufferUnderRun(), "Read at the end after Refresh");
        Bits.Refresh();
        CHECK(Bits.BufferUnderRun() && !Bits.Remain(), "Refresh cleared the underrun state");
    }
    return 0;
}

} // unnamed namespace

int main()
{
    if (TestReadsAndPeeks())
        return 1;
    if (TestNarrowReads())
        return 1;
    if (TestBitsAndAlignment())
        return 1;
    if (TestBoundariesAndResize())
        return 1;
    if (TestOptimizedPaths())
        return 1;
    if (TestExtraBytesAvailable())
        return 1;
    if (TestCacheCrossings())
        return 1;
    if (TestPeek8())
        return 1;
    if (TestExpGolombLengths())
        return 1;
    if (TestExpGolombSequences())
        return 1;
    if (TestSignedExpGolomb())
        return 1;
    if (TestErrorRecovery())
        return 1;
    if (TestTinyBuffers())
        return 1;
    if (TestResizeBeyondEnd())
        return 1;
    if (TestResizeAlignment())
        return 1;
    if (TestHugeSizes())
        return 1;
    if (TestRefresh())
        return 1;
    return TestMixedOperations();
}

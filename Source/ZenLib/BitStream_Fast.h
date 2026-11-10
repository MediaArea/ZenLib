/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Read a stream bit per bit
// Can read up to 32 bits at once (64 bits with Get8 and Peek8)
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef ZenBitStream_FastH
#define ZenBitStream_FastH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "ZenLib/Conf.h"
//---------------------------------------------------------------------------

#include <cstddef>
#include <cstring>
#if defined(_MSC_VER)
    #include <stdlib.h>
    #include <intrin.h>
#endif

namespace ZenLib
{

class BitStream_Fast_Local;

#ifndef MIN
    #define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#if defined(_MSC_VER)
    #define ZENLIB_BSF_INLINE   __forceinline
    #define ZENLIB_BSF_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
    #define ZENLIB_BSF_INLINE   inline __attribute__((always_inline))
    #define ZENLIB_BSF_NOINLINE __attribute__((noinline))
#else
    #define ZENLIB_BSF_INLINE   inline
    #define ZENLIB_BSF_NOINLINE
#endif

//***************************************************************************
// BitStream_Fast
//***************************************************************************
//
// Design: the position is the count of remaining bits, Buffer_Size. Window
// holds one native big-endian word of the stream, loaded by Attach(),
// Resize() and Refresh(), then when a read goes past its usable bits. A read
// checks that its bits are in Window, decrements Buffer_Size and extracts
// them with one rotation and a mask: the position is the only state written
// by reads, and Skip() is a subtraction.
//
// Window is rotated at load time so that the bit read when Buffer_Size is B
// is its bit (B-1)%WordBits: after a read of n bits decremented Buffer_Size,
// the bits are the n low bits of Window rotated right by Buffer_Size. Reads
// only need, in addition to Buffer_Size:
// - WindowEnd: Buffer_Size value after the last usable bit of Window, never
//   beyond the logical end. A read of n bits can use Window iff
//   n+WindowEnd<=Buffer_Size, which also implies n<=Remain().
// - The position is never before Window: Window is loaded at or before the
//   position, so the usable bits never wrap around. A position beyond Window
//   (after Skip()) just fails the usability check.
// A reload at byte Position/8 gives at least WordBits-7 usable bits, or all
// the remaining bits when the word is moved back to end at the last readable
// byte. While Buffer_Size is 0, every read fails the usability check.
// Get8(), Get8_Unchecked() and Peek8() parts not in Window, and peeks wider
// than a fresh Window (32-bit words), use their own word load at the position,
// Window unchanged.
//
// Underrun: Remain() becomes 0 and the logical end moves to the position,
// which is preserved for a growing Resize(); BufferUnderRun() thus implies
// Remain()==0.
//
// Memory safety: loads never go before Buffer or beyond the logical end plus
// the declared ExtraBytesAvailable_ reserve, the reserve keeping its Attach()
// time end. Near the end, the word is loaded so that it ends at the last
// readable byte. If fewer bytes than a word are
// readable (logical size plus reserve), they are copied entirely into Window,
// zero padded, and never need another load: WindowEnd is then 0 and Reload()
// is never called.
//
class BitStream_Fast
{
public:
    BitStream_Fast ()                                                           { Attach(NULL, 0); }
    // ExtraBytesAvailable_ is the number of initialized, readable bytes after
    // Buffer_+Size_, excluded from Size_ and Remain(); readable bytes must not
    // be modified while attached, except bytes not read yet followed by a call
    // to Refresh(). Zero requires no extra storage beyond the
    // logical buffer. The reserve is not moved by Resize(): loads stay before
    // Buffer_+Size_+ExtraBytesAvailable_, or before the logical end if a
    // growing Resize() moved it further.
    BitStream_Fast (const int8u* Buffer_, size_t Size_, size_t ExtraBytesAvailable_=0) { Attach(Buffer_, Size_, ExtraBytesAvailable_); }
    ~BitStream_Fast ()                                                          {}

    // Same extra-byte contract as the constructor; omitted means zero.
    ZENLIB_BSF_INLINE void Attach(const int8u* Buffer_, size_t Size_, size_t ExtraBytesAvailable_=0)
    {
        Buffer=Buffer_;
        Buffer_Size=Buffer_Size_Init=BitEnd=Size_*8;
        // A reserve of one word minus one byte is enough for any load
        ExtraBytes=(int8u)MIN(ExtraBytesAvailable_, sizeof(Word)-1);
        BufferUnderRun_=false;
        Fill();
    }

    //***********************************************************************
    // Reads
    //***********************************************************************

    ZENLIB_BSF_INLINE bool GetB()
    {
        if (Buffer_Size<=WindowEnd)
        {
            if (!Buffer_Size)
            {
                UnderRun();
                return false;
            }
            Reload();
        }
        Buffer_Size--;
        return TestBit(Window, Buffer_Size);
    }

    ZENLIB_BSF_INLINE int8u Get1(int8u HowMany)
    {
        return (int8u)Get<8>(HowMany);
    }

    ZENLIB_BSF_INLINE int16u Get2(int8u HowMany)
    {
        return (int16u)Get<16>(HowMany);
    }

    ZENLIB_BSF_INLINE int32u Get4(int8u HowMany)
    {
        return (int32u)Get<32>(HowMany);
    }

    ZENLIB_BSF_INLINE int64u Get8(int8u HowMany)
    {
        if (HowMany<=32)
            return Get<32>(HowMany);
        if (HowMany>64)
            return 0;
        if (HowMany>Buffer_Size)
        {
            // Historical behavior: the high part is consumed if it is available
            if ((size_t)(HowMany-32)<=Buffer_Size)
                Buffer_Size-=HowMany-32;
            UnderRun();
            return 0;
        }
        return Read64(HowMany);
    }

    // Preconditions: 1 <= HowMany <= 32 and Remain() >= HowMany.
    ZENLIB_BSF_INLINE int32u Get4_Unchecked(int8u HowMany)
    {
        return (int32u)Read<32>(HowMany);
    }

    // Preconditions: 1 <= HowMany <= 64 and Remain() >= HowMany.
    ZENLIB_BSF_INLINE int64u Get8_Unchecked(int8u HowMany)
    {
        return Read64(HowMany);
    }

    // Exp-Golomb ue(v), up to 31 leading zero bits (values up to 2^32-2).
    // Returns 0 if the code is longer or truncated; the buffer is then
    // considered as exhausted (Remain() is 0 and BufferUnderRun() is set), so
    // BufferUnderRun() right after the call tells an error from a valid 0.
    ZENLIB_BSF_INLINE int32u GetUE()
    {
        Word Bits;
        unsigned Size;
        if (!PeekUE(Bits, Size))
            return GetUE_Slow();
        Buffer_Size-=Size;
        return (int32u)(Bits>>(WordBits-Size))-1;
    }

    // Exp-Golomb se(v). Returns 0 on error, see GetUE().
    ZENLIB_BSF_INLINE int32s GetSE()
    {
        const int32u CodeNum=GetUE();
        return (int32s)((CodeNum&1)?(CodeNum>>1)+1:0u-(CodeNum>>1));
    }

    // Sticky error state; cleared by Attach() or by growing Resize().
    bool BufferUnderRun() const { return BufferUnderRun_; }

    BitStream_Fast_Local LocalReader();

    ZENLIB_BSF_INLINE void Skip(size_t HowMany)
    {
        if (HowMany>Buffer_Size)
        {
            UnderRun();
            return;
        }
        Buffer_Size-=HowMany;
    }

    ZENLIB_BSF_INLINE bool PeekB()
    {
        if (Buffer_Size<=WindowEnd)
        {
            if (!Buffer_Size)
            {
                UnderRun();
                return false;
            }
            Reload();
        }
        return TestBit(Window, Buffer_Size-1);
    }

    ZENLIB_BSF_INLINE int8u Peek1(int8u HowMany)
    {
        return (int8u)Peek<8>(HowMany);
    }

    ZENLIB_BSF_INLINE int16u Peek2(int8u HowMany)
    {
        return (int16u)Peek<16>(HowMany);
    }

    ZENLIB_BSF_INLINE int32u Peek4(int8u HowMany)
    {
        return (int32u)Peek<32>(HowMany);
    }

    int64u Peek8(int8u HowMany)
    {
        if (HowMany<=32)
            return Peek<32>(HowMany);
        if (HowMany>64)
            return 0;
        if (HowMany>Buffer_Size)
        {
            UnderRun();
            return 0;
        }
        if (HowMany<=WordBits)
            return PeekWide(Buffer_Size, HowMany);
        // 32-bit words: high part, then the 32 bits after it
        const int64u High=PeekWide(Buffer_Size, HowMany-32);
        return (High<<32)|PeekWide(Buffer_Size-(HowMany-32), 32);
    }

    inline size_t Remain() const
    {
        return Buffer_Size;
    }

    inline void Byte_Align()
    {
        Buffer_Size&=~(size_t)7;
    }

    inline size_t Offset_Get() const
    {
        return (Buffer_Size_Init-Buffer_Size)/8;
    }

    inline size_t BitOffset_Get() const
    {
        return Buffer_Size%8;
    }

    inline size_t OffsetBeforeLastCall_Get() const //No more valid
    {
        return Buffer_Size%8;
    }

    // Size_ is the new count of remaining bits: the position is kept, only the logical end
    // moves, to the position plus Size_ bits (this sum must fit in a size_t). Beyond the
    // attached size, the bytes up to the new logical end must be readable, see Attach().
    inline void Resize(size_t Size_)
    {
        if (BufferUnderRun_ && Size_>Buffer_Size)
            BufferUnderRun_=false;
        BitEnd=BitEnd-Buffer_Size+Size_;
        Buffer_Size=Size_;
        Fill();
    }

    // Loads Window again from the attached bytes at the position: needed after
    // modifying bytes not read yet (for example a descrambling in place), as
    // Window may already contain them. The position and the state are kept.
    inline void Refresh()
    {
        Fill();
    }

private:
    typedef size_t Word;
    enum { WordBits=sizeof(Word)*8 };

    //***********************************************************************
    // Word helpers
    //***********************************************************************

    static ZENLIB_BSF_INLINE Word LoadWord(const int8u* Source)
    {
        Word Value;
        std::memcpy(&Value, Source, sizeof(Value));
        #if defined(__BYTE_ORDER__) && __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
            return Value;
        #elif defined(_MSC_VER)
            #if defined(_WIN64)
                return _byteswap_uint64(Value);
            #else
                return _byteswap_ulong(Value);
            #endif
        #elif (defined(__GNUC__) || defined(__clang__)) && defined(__BYTE_ORDER__) && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
            return sizeof(Word)==8?(Word)__builtin_bswap64((int64u)Value):(Word)__builtin_bswap32((int32u)Value);
        #else
            // Unknown byte order
            Word Result=0;
            for (size_t i=0; i<sizeof(Word); i++)
                Result=(Result<<8)|Source[i];
            return Result;
        #endif
    }

    // Big-endian value of Size bytes (Size<sizeof(Word)), left aligned, zero padded
    static ZENLIB_BSF_NOINLINE Word LoadPartial(const int8u* Source, size_t Size)
    {
        Word Value=0;
        for (size_t i=0; i<Size; i++)
            Value|=(Word)Source[i]<<(WordBits-8-8*i);
        return Value;
    }

    static ZENLIB_BSF_INLINE unsigned CountLeadingZeros(Word Value) //Value!=0
    {
        #if defined(_MSC_VER)
            unsigned long Index;
            #if defined(_WIN64)
                _BitScanReverse64(&Index, Value);
            #else
                _BitScanReverse(&Index, Value);
            #endif
            return (unsigned)(WordBits-1-Index);
        #elif defined(__GNUC__) || defined(__clang__)
            return sizeof(Word)==8?(unsigned)__builtin_clzll((unsigned long long)Value):(unsigned)__builtin_clz((unsigned)Value);
        #else
            unsigned Count=0;
            while (!(Value>>(WordBits-1))) { Value<<=1; Count++; }
            return Count;
        #endif
    }

    // Mask of the HowMany low bits, 0 <= HowMany <= WordBits
    static ZENLIB_BSF_INLINE Word Mask(unsigned HowMany)
    {
        // A table avoids a variable shift when HowMany is not a constant.
        // 64-bit entries for any word size: with 32-bit words, only the first 33 are used.
        #define ZENLIB_BSF_MASK4(N) ((int64u)1<<(N))-1, ((int64u)2<<(N))-1, ((int64u)4<<(N))-1, ((int64u)8<<(N))-1
        static const int64u Masks[65]=
        {
            ZENLIB_BSF_MASK4( 0), ZENLIB_BSF_MASK4( 4), ZENLIB_BSF_MASK4( 8), ZENLIB_BSF_MASK4(12),
            ZENLIB_BSF_MASK4(16), ZENLIB_BSF_MASK4(20), ZENLIB_BSF_MASK4(24), ZENLIB_BSF_MASK4(28),
            ZENLIB_BSF_MASK4(32), ZENLIB_BSF_MASK4(36), ZENLIB_BSF_MASK4(40), ZENLIB_BSF_MASK4(44),
            ZENLIB_BSF_MASK4(48), ZENLIB_BSF_MASK4(52), ZENLIB_BSF_MASK4(56), ZENLIB_BSF_MASK4(60),
            (int64u)-1,
        };
        #undef ZENLIB_BSF_MASK4
        return (Word)Masks[HowMany];
    }

    // Value rotated right by Count%WordBits bits
    static ZENLIB_BSF_INLINE Word RotateRight(Word Value, size_t Count)
    {
        #if defined(_MSC_VER) && !defined(__clang__)
            // The instruction masks the count
            #if defined(_WIN64)
                return _rotr64(Value, (int)Count);
            #else
                return _rotr(Value, (int)Count);
            #endif
        #else
            const unsigned Shift=(unsigned)(Count&(WordBits-1));
            return (Value>>Shift)|(Value<<((WordBits-Shift)&(WordBits-1))); // Recognized as a rotation
        #endif
    }

    // Bit Index%WordBits of Value, 0 being the least significant bit
    static ZENLIB_BSF_INLINE bool TestBit(Word Value, size_t Index)
    {
        // Both forms let the instruction mask the index: a rotation with MSVC,
        // a bit test or a shift with GCC and Clang
        #if defined(_MSC_VER) && !defined(__clang__)
            return (RotateRight(Value, Index)&1)!=0;
        #else
            return ((Value>>(Index&(WordBits-1)))&1)!=0;
        #endif
    }

    //***********************************************************************
    // Window management
    //***********************************************************************

    // Loads Window at byte Position/8, or at the last loadable byte offset.
    // Precondition: a full word is readable (LastLoad is valid), which is the case if WindowEnd>0.
    ZENLIB_BSF_INLINE void Reload()
    {
        size_t Load=(BitEnd-Buffer_Size)/8;
        if (Load>LastLoad)
            Load=LastLoad; // Near the end, the word ends at the last readable byte
        // Bits from the word start to the logical end, then Buffer_Size value at the least
        // significant bit of the word (wraps around if the word ends after the logical end).
        // Unsigned comparison: with 32-bit words, Avail may be 2^31 or more.
        const size_t Avail=BitEnd-Load*8;
        const size_t WindowLow=Avail-WordBits;
        Window=RotateRight(LoadWord(Buffer+Load), 0-WindowLow);
        WindowEnd=Avail>WordBits?WindowLow:0;
    }

    // Computes the load limit for the current logical end then loads Window, any buffer size
    ZENLIB_BSF_INLINE void Fill()
    {
        // Readable bytes since Buffer: the logical ones, then the reserve, trusted
        // only up to its end at Attach() time, as a growing Resize() may move the
        // logical end beyond the attached size
        const size_t Logical=BitEnd/8+(BitEnd%8!=0); // No overflow after any Resize()
        const size_t Attached=Buffer_Size_Init/8+ExtraBytes;
        size_t Readable=Logical+ExtraBytes;
        if (Readable>Attached)
            Readable=Logical>Attached?Logical:Attached;
        if (Readable>=sizeof(Word))
        {
            LastLoad=Readable-sizeof(Word);
            if (Buffer_Size)
            {
                Reload();
                return;
            }
        }
        else
        {
            LastLoad=0; // Never used: Window contains the whole buffer
            if (Buffer_Size)
            {
                Window=RotateRight(LoadPartial(Buffer, Readable), WordBits-BitEnd);
                WindowEnd=0;
                return;
            }
        }

        // Nothing to read, nothing to load: every read fails the usability check
        Window=0;
        WindowEnd=0;
    }

    ZENLIB_BSF_INLINE void UnderRun()
    {
        // The logical end moves to the position, which is kept for a growing Resize()
        BitEnd-=Buffer_Size;
        Buffer_Size=0;
        WindowEnd=0;
        BufferUnderRun_=true;
    }

    //***********************************************************************
    // Reads
    //***********************************************************************

    template <int Max>
    ZENLIB_BSF_INLINE Word Get(int8u HowMany)
    {
        // Rejects 0 and HowMany>Max with a single comparison
        if ((unsigned)(HowMany-1)>=(unsigned)Max)
            return 0;
        return Read<Max>(HowMany);
    }

    template <int Max>
    ZENLIB_BSF_INLINE Word Peek(int8u HowMany)
    {
        if ((unsigned)(HowMany-1)>=(unsigned)Max)
            return 0;
        if (HowMany+WindowEnd>Buffer_Size)
        {
            if (HowMany>Buffer_Size)
            {
                UnderRun();
                return 0;
            }
            Reload();
            // A fresh Window has at least WordBits-7 usable bits
            if (Max>WordBits-7 && HowMany+WindowEnd>Buffer_Size)
                return PeekDirect(Buffer_Size, HowMany);
        }
        return RotateRight(Window, Buffer_Size-HowMany)&Mask(HowMany);
    }

    // Preconditions: 1 <= HowMany <= Max <= WordBits (if HowMany>Remain(): underrun)
    template <int Max>
    ZENLIB_BSF_INLINE Word Read(unsigned HowMany)
    {
        if (HowMany+WindowEnd>Buffer_Size)
        {
            if (HowMany>Buffer_Size)
            {
                UnderRun();
                return 0;
            }
            Reload();
            // A fresh Window has at least WordBits-7 usable bits
            if (Max>WordBits-7 && HowMany+WindowEnd>Buffer_Size)
                return Read_Split(HowMany);
        }
        Buffer_Size-=HowMany;
        return RotateRight(Window, Buffer_Size)&Mask(HowMany);
    }

    // Read wider than the usable bits of a fresh Window (32-bit words only): the high
    // part from Window, which is then reloaded at the position of the low part
    ZENLIB_BSF_NOINLINE Word Read_Split(unsigned HowMany)
    {
        const unsigned High_Size=(unsigned)(Buffer_Size-WindowEnd);
        Buffer_Size-=High_Size;
        const Word High=RotateRight(Window, Buffer_Size)&Mask(High_Size);
        Reload();
        const unsigned Low_Size=HowMany-High_Size;
        Buffer_Size-=Low_Size;
        return (High<<Low_Size)|(RotateRight(Window, Buffer_Size)&Mask(Low_Size));
    }

    // Preconditions: 1 <= HowMany <= 64 (if HowMany>Remain(): underrun)
    ZENLIB_BSF_INLINE int64u Read64(unsigned HowMany)
    {
        if (HowMany<=WordBits)
            return ReadWide(HowMany);
        const int64u High=ReadWide(HowMany-32);
        return (High<<32)|ReadWide(32);
    }

    // A wide read would consume most of a reloaded Window: it uses its own load instead.
    // Preconditions: 1 <= HowMany <= WordBits (if HowMany>Remain(): underrun)
    ZENLIB_BSF_INLINE Word ReadWide(unsigned HowMany)
    {
        Word Value;
        if (HowMany+WindowEnd<=Buffer_Size)
            Value=RotateRight(Window, Buffer_Size-HowMany)&Mask(HowMany);
        else if (HowMany<=Buffer_Size)
            Value=PeekDirect(Buffer_Size, HowMany);
        else
        {
            UnderRun();
            return 0;
        }
        Buffer_Size-=HowMany;
        return Value;
    }

    // HowMany bits at the position where Remain() would be Remaining, no state change.
    // Preconditions: 1 <= HowMany <= WordBits and HowMany <= Remaining <= Remain(),
    // so that the position is not before Window.
    ZENLIB_BSF_INLINE Word PeekWide(size_t Remaining, unsigned HowMany) const
    {
        if (HowMany+WindowEnd<=Remaining)
            return RotateRight(Window, Remaining-HowMany)&Mask(HowMany);
        return PeekDirect(Remaining, HowMany);
    }

    // HowMany bits at the position where Remain() would be Remaining, from their own
    // word load, Window unchanged.
    // Preconditions: 1 <= HowMany <= WordBits, HowMany <= Remaining <= Remain() and the
    // bits are not all in Window (so a full word is readable).
    ZENLIB_BSF_INLINE Word PeekDirect(size_t Remaining, unsigned HowMany) const
    {
        const size_t BitPos=BitEnd-Remaining;
        size_t Load=BitPos/8;
        if (Load>LastLoad)
            Load=LastLoad; // The word then contains all the remaining bits
        const unsigned Shift=(unsigned)(BitPos-Load*8);
        Word Value=LoadWord(Buffer+Load)<<Shift;
        if (Shift+HowMany>WordBits)
            Value|=(Word)(Buffer[Load+sizeof(Word)]>>(8-Shift)); // Not clamped, before the logical end
        return Value>>(WordBits-HowMany);
    }

    //***********************************************************************
    // Exp-Golomb
    //***********************************************************************

    // Exp-Golomb code inside Window: bits from the position, left aligned, and code size
    ZENLIB_BSF_INLINE bool PeekUE(Word& Bits, unsigned& Size) const
    {
        if (Buffer_Size<=WindowEnd)
            return false;
        Bits=RotateRight(Window, Buffer_Size); // Next bit at the top, then the usable bits
        if (!Bits)
            return false;
        // The whole code must be in the usable bits, so it is valid even if the
        // leading one was found in the bits wrapped around after them.
        // Size<=WordBits-1, so at most 31 leading zero bits on 64-bit.
        Size=CountLeadingZeros(Bits)*2+1;
        return Size<=Buffer_Size-WindowEnd; // Size+WindowEnd could overflow
    }

    // Exp-Golomb code not inside Window
    ZENLIB_BSF_NOINLINE int32u GetUE_Slow()
    {
        // Reload if bits after Window are readable, else the code is long, truncated or invalid
        Word Bits;
        unsigned Size;
        if (WindowEnd)
        {
            Reload();
            if (PeekUE(Bits, Size))
            {
                Buffer_Size-=Size;
                return (int32u)(Bits>>(WordBits-Size))-1;
            }
        }
        if (Buffer_Size)
        {
            const int64u Result=GetUE_Bytes(Buffer, BitEnd-Buffer_Size, Buffer_Size);
            const size_t Code_Size=(size_t)(Result>>32);
            if (Code_Size)
            {
                Buffer_Size-=Code_Size; // The next read reloads Window if needed
                return (int32u)Result;
            }
        }
        UnderRun();
        return 0;
    }

    // Exp-Golomb code read byte per byte, for codes not fitting in a fresh Window.
    // Returns the code size (0 on error) in the high 32 bits, the value in the low 32 bits.
    static ZENLIB_BSF_NOINLINE int64u GetUE_Bytes(const int8u* Buffer, size_t BitPos, size_t Remain)
    {
        const unsigned Peeked=(unsigned)MIN(Remain, (size_t)32);
        const int32u Prefix=ReadBytes(Buffer, BitPos, Peeked)<<(32-Peeked);
        if (!Prefix)
            return 0; // More than 31 leading zero bits, or truncated
        unsigned LeadingZeroBits=0;
        while (!(Prefix>>(31-LeadingZeroBits)))
            LeadingZeroBits++;
        if (LeadingZeroBits*2+1>Remain)
            return 0; // Truncated
        const int32u Info=LeadingZeroBits?ReadBytes(Buffer, BitPos+LeadingZeroBits+1, LeadingZeroBits):0;
        return ((int64u)(LeadingZeroBits*2+1)<<32)|((((int32u)1<<LeadingZeroBits)|Info)-1);
    }

    // HowMany bits (1 to 32) at BitPos, reading only the bytes containing them
    static int32u ReadBytes(const int8u* Buffer, size_t BitPos, unsigned HowMany)
    {
        const int8u* Source=Buffer+BitPos/8;
        const unsigned Skipped=(unsigned)(BitPos%8);
        const unsigned Bytes=(Skipped+HowMany+7)/8;
        int64u Value=0;
        for (unsigned i=0; i<Bytes; i++)
            Value=(Value<<8)|Source[i];
        return (int32u)((Value>>(Bytes*8-Skipped-HowMany))&(((int64u)1<<HowMany)-1));
    }

    // Hot state, see the class comment
    Word                Window;
    size_t              Buffer_Size;        // Remaining bits
    size_t              WindowEnd;
    // Cold state, used when Window is (re)loaded
    const int8u*        Buffer;
    size_t              BitEnd;             // Logical end, in bits since Buffer
    size_t              LastLoad;           // Last byte offset where a full word can be loaded
    size_t              Buffer_Size_Init;
    int8u               ExtraBytes;
    bool                BufferUnderRun_;
};

// Independent parsing state: changes become visible in the owner only at Commit().
// The owner must outlive the session and must not be modified before Commit().
class BitStream_Fast_Local : public BitStream_Fast
{
public:
    explicit BitStream_Fast_Local(BitStream_Fast& Owner_)
        : BitStream_Fast(Owner_), Owner(&Owner_) {}
    BitStream_Fast_Local(BitStream_Fast_Local&& Other)
        : BitStream_Fast(Other), Owner(Other.Owner) { Other.Owner=NULL; }
    BitStream_Fast_Local(const BitStream_Fast_Local&)=delete;
    BitStream_Fast_Local& operator=(const BitStream_Fast_Local&)=delete;

    // Explicit commit permits speculative parsing; destruction discards changes.
    void Commit()
    {
        if (Owner)
            *Owner=static_cast<const BitStream_Fast&>(*this);
    }

private:
    BitStream_Fast* Owner;
};

inline BitStream_Fast_Local BitStream_Fast::LocalReader()
{
    return BitStream_Fast_Local(*this);
}

#undef ZENLIB_BSF_INLINE
#undef ZENLIB_BSF_NOINLINE

} //namespace ZenLib
#endif

#pragma once

#include <stdint.h>

#define MAX_COLS_16_GAP (8*5)

#define HEX_CHAR_WIDTH(__hcw_v__, __hcw_w__) \
{ \
    uint8_t _hcw_w_ = 0x10; \
    for ( uint8_t _i_ = 0x38; _i_ > 0; _i_-=8 ) \
    { \
        if ( ! ((uint8_t)(__hcw_v__ >> _i_)) ) \
            _hcw_w_ -= 2; \
        else \
            break; \
    } \
    __hcw_w__ = _hcw_w_; \
}

//
// stick to ASCII only for safety and simplicity
//
static inline void putPrintableA(uint8_t c)
{
    putchar(( c >= 0x20 && c <= 0x7E ) ? (CHAR)c : '.');
}

//
// stick to ASCII only for safety and simplicity
//
static inline void putPrintableW(uint16_t wc)
{
    putchar(( wc >= 0x20 && wc <= 0x7E ) ? (CHAR)wc : '.');
}

#ifdef DEBUG_PRINT
#define DPrint(...) \
                printf(__VA_ARGS__);
#define FEnter() \
                printf("[>] %s()\n", __func__);
#define FLeave() \
                printf("[<] %s()\n", __func__);

FORCEINLINE
void DPrintMemCols32(PVOID buffer, SIZE_T size, uint64_t addr)
{
    uint8_t hw = 0x10;
    HEX_CHAR_WIDTH((addr + size), hw);

    uint8_t* p = (uint8_t*)buffer;
    uint64_t full = size & ~(uint64_t)0xF; // bytes in complete 16-byte rows
    uint64_t tail = size & 0xF; // leftover bytes (0..15)

    uint64_t i;
    for ( i = 0; i < full; i += 0x10 )
    {
        uint32_t d0, d1, d2, d3;
        d0 = *(uint32_t*)&p[i];
        d1 = *(uint32_t*)&p[i+ 4];
        d2 = *(uint32_t*)&p[i+ 8];
        d3 = *(uint32_t*)&p[i+ 12];
        //memcpy(&d0, p + i, 4);
        //memcpy(&d1, p + i + 4, 4);
        //memcpy(&d2, p + i + 8, 4);
        //memcpy(&d3, p + i + 12, 4);
        printf("%.*zx  %08x %08x %08x %08x\n",
            hw, (size_t)(addr + i), d0, d1, d2, d3);
    }
    if ( tail )
    {
        printf("%.*zx ", hw, (size_t)(addr + i));
        uint64_t off = 0;
        for ( ; off + 4 <= tail; off += 4 ) // full 4-byte chunks
        {
            uint32_t d;
            d = *(uint32_t*)&p[i + off];
            //memcpy(&d, p + i + off, 4);
            printf(" %08x", d);
        }
        if ( off < tail ) // final 1..3 bytes
        {
            uint32_t d = 0;
            memcpy(&d, p + i + off, (size_t)(tail - off));
            printf(" %08x", d);
        }
        putchar('\n');
    }
}
#else
#define DPrint(...)
#define FEnter()
#define FLeave()
#define DPrintMemCols32(_b_, _s_, _a_)
#endif

#ifdef ERROR_PRINT
#define EPrint(...) \
{ \
                printf("[e] ");\
                printf(__VA_ARGS__); \
}
#else
#define EPrint(...)
#endif

FORCEINLINE
void PrintMemCols8(PVOID _b_, SIZE_T _s_, SIZE_T _a_)
{
    uint64_t _hw_v_ = (uint64_t)_a_ + _s_;
    uint8_t _hw_w_ = 0x10;
    HEX_CHAR_WIDTH(_hw_v_, _hw_w_);
   
    for ( uint64_t _i_ = 0; _i_ < _s_; _i_+=0x10 )
    {
        uint64_t _end_ = (_i_+0x10<_s_) ? (_i_+0x10) : (_s_);
        ULONG _gap_ = (_i_+0x10<=_s_) ? 0 : (ULONG)((0x10+_i_-_s_)*3);
        printf("%.*zx  ", _hw_w_, (((uint64_t)_a_)+_i_));
        
        for ( uint64_t _j_ = _i_, _k_=0; _j_ < _end_; _j_++, _k_++ )
        {
            printf("%02x", ((uint8_t*)_b_)[_j_]);
            printf("%c", (_k_==7?'-':' '));
        }
        for ( ULONG _j_ = 0; _j_ < _gap_; _j_++ )
        {
            printf(" ");
        }
        printf("  ");
        for ( uint64_t _k_ = _i_; _k_ < _end_; _k_++ )
        {
            putPrintableA(((uint8_t*)_b_)[_k_]);
        }
        printf("\n");
    }
}

FORCEINLINE
void PrintMemCols16(PVOID buffer, SIZE_T size, uint64_t addr)
{
    uint8_t hw = 0x10;
    HEX_CHAR_WIDTH((addr + size), hw);

    uint8_t* p = (uint8_t*)buffer;
    uint64_t full = size & ~(uint64_t)0xF; // bytes in complete 16-byte rows
    uint32_t tail = size & 0xF; // leftover bytes (0..15)

    uint64_t i;
    for ( i = 0; i < full; i += 0x10 )
    {
        printf("%.*zx ", hw, (size_t)(addr + i));
        uint16_t w[8] = { 0 };

        for ( uint8_t wi = 0; wi < 8; wi++ )
        {
            w[wi] = *(uint16_t*)&p[i + wi*2];
            // memcpy(&w[wi], p + i + wi*2, 2);
            printf(" %04x", w[wi]);
        }
        printf("  ");
        for ( uint8_t wi = 0; wi < 8; wi++ )
        {
            putPrintableW(w[wi]);
        }
        printf("\n");
    }

    if ( tail )
    {
        printf("%.*zx ", hw, (size_t)(addr + i));
        uint32_t off = 0;
        uint16_t w[8] = { 0 };
        uint8_t wi = 0;
        for ( ; off + 2 <= tail; off += 2, wi++ ) // full 2-byte chunks
        {
            w[wi] = *(uint16_t*)&p[i + off];
            // memcpy(&w[wi], p + i + off, 2);
            printf(" %04x", w[wi]);
        }
        if ( off < tail ) // final 1 byte
        {
            w[wi] = p[i + off];
            //memcpy(&w[wi], p + i + off, (size_t)(tail - off));
            printf(" %04x", w[wi]);
            wi++;
        }
        
        uint32_t aligned_tail = (tail + 1) & ~1u;
        uint32_t gap = 2 + MAX_COLS_16_GAP - ((aligned_tail/2) * 5);
        for ( uint32_t gi = 0; gi < gap; gi++ )
            printf(" ");
        
        off = 0;
        uint8_t w_size = wi;
        for ( wi = 0; wi < w_size; wi++ )
        {
            putPrintableW(w[wi]);
        }
        putchar('\n');
    }
}

FORCEINLINE
void PrintMemCols32(PVOID buffer, SIZE_T size, uint64_t addr)
{
    uint8_t hw = 0x10;
    HEX_CHAR_WIDTH((addr + size), hw);

    uint8_t* p = (uint8_t*)buffer;
    uint64_t full = size & ~(uint64_t)0xF; // bytes in complete 16-byte rows
    uint64_t tail = size & 0xF; // leftover bytes (0..15)

    uint64_t i;
    for ( i = 0; i < full; i += 0x10 )
    {
        uint32_t d0, d1, d2, d3;
        d0 = *(uint32_t*)&p[i];
        d1 = *(uint32_t*)&p[i+ 4];
        d2 = *(uint32_t*)&p[i+ 8];
        d3 = *(uint32_t*)&p[i+ 12];
        //memcpy(&d0, p + i, 4);
        //memcpy(&d1, p + i + 4, 4);
        //memcpy(&d2, p + i + 8, 4);
        //memcpy(&d3, p + i + 12, 4);
        printf("%.*zx  %08x %08x %08x %08x\n",
            hw, (size_t)(addr + i), d0, d1, d2, d3);
    }
    if ( tail )
    {
        printf("%.*zx ", hw, (size_t)(addr + i));
        uint64_t off = 0;
        for ( ; off + 4 <= tail; off += 4 ) // full 4-byte chunks
        {
            uint32_t d;
            d = *(uint32_t*)&p[i + off];
            //memcpy(&d, p + i + off, 4);
            printf(" %08x", d);
        }
        if ( off < tail ) // final 1..3 bytes
        {
            uint32_t d = 0;
            memcpy(&d, p + i + off, (size_t)(tail - off));
            printf(" %08x", d);
        }
        putchar('\n');
    }
}

FORCEINLINE
void PrintMemCols64(PVOID buffer, SIZE_T size, uint64_t addr)
{
    uint8_t hw = 0x10;
    HEX_CHAR_WIDTH((addr + size), hw);

    uint8_t* p = (uint8_t*)buffer;
    uint64_t full = size & ~(uint64_t)0xF; // bytes in complete 16-byte rows
    uint64_t tail = size & 0xF; // leftover bytes (0..15)

    uint64_t i;
    for ( i = 0; i < full; i += 0x10 )
    {
        uint64_t q0, q1;
        q0 = *(uint64_t*)&p[i];
        q1 = *(uint64_t*)&p[i+ 8];
        //memcpy(&q0, p + i, 8);
        //memcpy(&q1, p + i + 8, 8);
        printf("%.*zx  %016llx %016llx\n",
            hw, (size_t)(addr + i), q0, q1);
    }

    if ( tail )
    {
        printf("%.*zx ", hw, (size_t)(addr + i));
        uint64_t off = 0;
        for ( ; off + 8 <= tail; off += 8 ) // full 8-byte chunks
        {
            uint64_t q;
            q = *(uint64_t*)&p[i+ off];
            //memcpy(&q, p + i + off, 8);
            printf(" %016llx", q);
        }
        if ( off < tail ) // final 1..7 bytes
        {
            uint64_t q = 0;
            memcpy(&q, p + i + off, (size_t)(tail - off));
            printf(" %016llx", q);
        }
        putchar('\n');
    }
}

FORCEINLINE
void PrintMemColsBits(PVOID _b_, SIZE_T _s_, uint64_t _o_)
{
    uint64_t _hw_v_ = (SIZE_T)_o_ + (SIZE_T)_s_;
    uint8_t _hw_w_ = 0x10;
    uint8_t _bytes_per_col = 8;
    uint64_t _s_cpy_ = _s_;
    HEX_CHAR_WIDTH(_hw_v_, _hw_w_);
   
    for ( SIZE_T _i_ = 0; _i_ < (SIZE_T)_s_cpy_; _i_+=_bytes_per_col )
    {
        SIZE_T _end_ = (_i_+_bytes_per_col<(SIZE_T)_s_cpy_)?(_i_+_bytes_per_col):((SIZE_T)_s_cpy_);
        printf("%.*zx  ", _hw_w_, (((SIZE_T)_o_)+_i_));
        
        for ( SIZE_T _bi_ = _i_; _bi_ < _end_; _bi_++ )
        {
            uint8_t _n_ = ((uint8_t*)_b_)[_bi_];
            for ( INT _j_ = 7; _j_ >= 0; _j_-- )
            {
                if ( ( (_n_ >> _j_) & 1 ) )
                    putchar('1');
                else
                    putchar('0');
                if ( _bi_ % 4 == 3 && _j_ == 0 && _bi_ < _end_-1)
                {
                    putchar('-');
                }
                else if ( _j_ % 4 == 0 )
                {
                    putchar(' ');
                }
            }
        }
        printf("\n");
    }
}

FORCEINLINE
void PrintMemBytes(PVOID _b_, SIZE_T _s_)
{
    for ( uint64_t _i_ = 0; _i_ < _s_; _i_++ )
    {
        printf("%02x ", ((uint8_t*)_b_)[_i_]);
    }
}

FORCEINLINE
void PrintMemByteStr(PVOID _b_, SIZE_T _s_)
{
    for ( uint64_t _i_ = 0; _i_ < _s_; _i_++ )
    {
        printf("%02x", ((uint8_t*)_b_)[_i_]);
    }
}

FORCEINLINE
void PrintAStr(PVOID b, SIZE_T s)
{
    for ( SIZE_T k = 0; k < s; k++ )
    {
        putPrintableA(((uint8_t*)b)[k]);
    }
    printf("\n");
}

FORCEINLINE
void PrintWStr(PVOID b, SIZE_T s)
{
    SIZE_T n = s / 2;
    for ( SIZE_T k = 0; k < n; k++ )
    {
        putPrintableW(*(uint16_t*)&(((uint8_t*)b)[k * 2]));
    }
    printf("\n");
}



#define STATUS_CASE(__status__) \
    case __status__: return #__status__;
#define STATUS_CASE_DESC(__status__, __desc__) \
    case __status__: return __desc__;

FORCEINLINE
const char* getStatusString(NTSTATUS status)
{
    switch ( status )
    {
        STATUS_CASE(STATUS_NOT_IMPLEMENTED)
        STATUS_CASE(STATUS_INVALID_HANDLE)
        STATUS_CASE(STATUS_INVALID_PARAMETER)
        STATUS_CASE(STATUS_NO_SUCH_DEVICE)
        STATUS_CASE(STATUS_NO_SUCH_FILE)
        STATUS_CASE(STATUS_INVALID_DEVICE_REQUEST)
        STATUS_CASE(STATUS_ACCESS_DENIED)
        STATUS_CASE(STATUS_OBJECT_TYPE_MISMATCH)
        STATUS_CASE(STATUS_OBJECT_NAME_INVALID)
        STATUS_CASE(STATUS_OBJECT_NAME_NOT_FOUND)
        STATUS_CASE(STATUS_OBJECT_NAME_COLLISION)
        STATUS_CASE(STATUS_OBJECT_PATH_NOT_FOUND)
        STATUS_CASE(STATUS_OBJECT_PATH_SYNTAX_BAD)
        STATUS_CASE_DESC(STATUS_ILLEGAL_FUNCTION, "STATUS_ILLEGAL_FUNCTION: The specified handle is not open to the server end of the named pipe.")
        STATUS_CASE(STATUS_NOT_SUPPORTED)
        STATUS_CASE(STATUS_NOT_FOUND)
        STATUS_CASE_DESC(STATUS_DATATYPE_MISALIGNMENT_ERROR, "STATUS_DATATYPE_MISALIGNMENT_ERROR: A data type misalignment error was detected in a load or store instruction.")
        
        STATUS_CASE(STATUS_BUFFER_OVERFLOW)
        
        default:
            return "Unknown status code";
    }
}

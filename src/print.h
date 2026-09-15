#pragma once


#define HEX_CHAR_WIDTH(__hcw_v__, __hcw_w__) \
{ \
    UINT8 _hcw_w_ = 0x10; \
    for ( UINT8 _i_ = 0x38; _i_ > 0; _i_-=8 ) \
    { \
        if ( ! ((UINT8)(__hcw_v__ >> _i_)) ) \
            _hcw_w_ -= 2; \
        else \
            break; \
    } \
    __hcw_w__ = _hcw_w_; \
}

//
// stick to ASCII only for safety and simplicity
//
static inline void putPrintableA(UINT8 c)
{
    putchar(( c >= 0x20 && c <= 0x7E ) ? (CHAR)c : '.');
}

//
// stick to ASCII only for safety and simplicity
//
static inline void putPrintableW(UINT16 wc)
{
    putchar(( wc >= 0x20 && wc <= 0x7E ) ? (CHAR)wc : '.');
}

#ifdef DEBUG_PRINT
#define DPrint(...) \
                printf(__VA_ARGS__);
#define FEnter() \
                printf("[>] %s()\n", __FUNCTION__);
#define FLeave() \
                printf("[<] %s()\n", __FUNCTION__);

#define DPrintMemCols32(b, s, _a_) \
{\
    UINT64 _hw_v_ = (UINT64)_a_ + s; \
    UINT8 _hw_w_ = 0x10; \
    UINT64 _s_cpy_ = s; \
    HEX_CHAR_WIDTH(_hw_v_, _hw_w_); \
    \
    if ( _s_cpy_ % 4 != 0 ) _s_cpy_ = _s_cpy_ - (_s_cpy_ % 4); \
    \
    for ( UINT64 _i_ = 0; _i_ < _s_cpy_; _i_+=0x10 ) \
    { \
        UINT64 _end_ = (_i_+0x10<_s_cpy_)?(_i_+0x10):(_s_cpy_); \
        printf("%.*zx  ", _hw_w_, (((UINT64)_a_)+_i_)); \
         \
        for ( UINT64 _j_ = _i_; _j_ < _end_; _j_+=4 ) \
        { \
            printf("%08x ", *(PUINT32)&(((PUINT8)b)[_j_])); \
        } \
        printf("\n"); \
    } \
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
    UINT64 _hw_v_ = (UINT64)_a_ + _s_;
    UINT8 _hw_w_ = 0x10;
    HEX_CHAR_WIDTH(_hw_v_, _hw_w_);
   
    for ( UINT64 _i_ = 0; _i_ < _s_; _i_+=0x10 )
    {
        UINT64 _end_ = (_i_+0x10<_s_) ? (_i_+0x10) : (_s_);
        ULONG _gap_ = (_i_+0x10<=_s_) ? 0 : (ULONG)((0x10+_i_-_s_)*3);
        printf("%.*zx  ", _hw_w_, (((UINT64)_a_)+_i_));
        
        for ( UINT64 _j_ = _i_, _k_=0; _j_ < _end_; _j_++, _k_++ )
        {
            printf("%02x", ((PUINT8)_b_)[_j_]);
            printf("%c", (_k_==7?'-':' '));
        }
        for ( ULONG _j_ = 0; _j_ < _gap_; _j_++ )
        {
            printf(" ");
        }
        printf("  ");
        for ( UINT64 _k_ = _i_; _k_ < _end_; _k_++ )
        {
            putPrintableA(((PUINT8)_b_)[_k_]);
        }
        printf("\n");
    }
}

FORCEINLINE
void PrintMemCols16(PVOID _b_, SIZE_T _s_, UINT64 _a_)
{
    UINT64 _hw_v_ = (UINT64)_a_ + _s_;
    UINT8 _hw_w_ = 0x10;
    UINT64 _s_cpy_ = _s_;
    HEX_CHAR_WIDTH(_hw_v_, _hw_w_);
   
    if ( _s_cpy_ % 2 != 0 ) _s_cpy_ = _s_cpy_ - 1;
   
    for ( UINT64 _i_ = 0; _i_ < _s_cpy_; _i_+=0x10 )
    {
        UINT64 _end_ = (_i_+0x10<_s_cpy_)?(_i_+0x10):(_s_cpy_);
        ULONG _gap_ = (_i_+0x10<=_s_cpy_) ? 0 : (ULONG)((0x10+_i_-_s_cpy_)/2*5);
        printf("%.*zx  ", _hw_w_, (((UINT64)_a_)+_i_));
        
        for ( UINT64 _j_ = _i_; _j_ < _end_; _j_+=2 )
        {
            printf("%04x ", *(PUINT16)&(((PUINT8)_b_)[_j_]));
        }
        for ( ULONG _j_ = 0; _j_ < _gap_; _j_++ )
        {
            printf(" ");
        }
        printf("  ");
        for ( UINT64 _j_ = _i_; _j_ < _end_; _j_+=2 )
        {
            putPrintableW(*(PUINT16)&(((PUINT8)_b_)[_j_]));
        }
        printf("\n");
    }
}

FORCEINLINE
void PrintMemCols32(PVOID _b_, SIZE_T _s_, UINT64 _a_)
{
    UINT64 _hw_v_ = (UINT64)_a_ + _s_;
    UINT8 _hw_w_ = 0x10;
    UINT64 _s_cpy_ = _s_;
    HEX_CHAR_WIDTH(_hw_v_, _hw_w_);
   
    if ( _s_cpy_ % 4 != 0 ) _s_cpy_ = _s_cpy_ - (_s_cpy_ % 4);
   
    for ( UINT64 _i_ = 0; _i_ < _s_cpy_; _i_+=0x10 )
    {
        UINT64 _end_ = (_i_+0x10<_s_cpy_)?(_i_+0x10):(_s_cpy_);
        printf("%.*zx  ", _hw_w_, (((UINT64)_a_)+_i_));
        
        for ( UINT64 _j_ = _i_; _j_ < _end_; _j_+=4 )
        {
            printf("%08x ", *(PUINT32)&(((PUINT8)_b_)[_j_]));
        }
        printf("\n");
    }
}

FORCEINLINE
void PrintMemCols64(PVOID _b_, SIZE_T _s_, UINT64 _a_)
{
    UINT64 _hw_v_ = (UINT64)_a_ + _s_;
    UINT8 _hw_w_ = 0x10;
    UINT64 _s_cpy_ = _s_;
    HEX_CHAR_WIDTH(_hw_v_, _hw_w_);
   
    if ( _s_cpy_ % 8 != 0 ) _s_cpy_ = _s_cpy_ - (_s_cpy_ % 8);
   
    for ( UINT64 _i_ = 0; _i_ < _s_cpy_; _i_+=0x10 )
    {
        UINT64 _end_ = (_i_+0x10<_s_cpy_)?(_i_+0x10):(_s_cpy_);
        printf("%.*zx  ", _hw_w_, (((UINT64)_a_)+_i_));
        
        for ( UINT64 _j_ = _i_; _j_ < _end_; _j_+=8 )
        {
            printf("%016llx ", *(PUINT64)&(((PUINT8)_b_)[_j_]));
        }
        printf("\n");
    }
}

FORCEINLINE
void PrintMemColsBits(PVOID _b_, SIZE_T _s_, UINT64 _o_)
{
    UINT64 _hw_v_ = (SIZE_T)_o_ + (SIZE_T)_s_;
    UINT8 _hw_w_ = 0x10;
    UINT8 _bytes_per_col = 8;
    UINT64 _s_cpy_ = _s_;
    HEX_CHAR_WIDTH(_hw_v_, _hw_w_);
   
    for ( SIZE_T _i_ = 0; _i_ < (SIZE_T)_s_cpy_; _i_+=_bytes_per_col )
    {
        SIZE_T _end_ = (_i_+_bytes_per_col<(SIZE_T)_s_cpy_)?(_i_+_bytes_per_col):((SIZE_T)_s_cpy_);
        printf("%.*zx  ", _hw_w_, (((SIZE_T)_o_)+_i_));
        
        for ( SIZE_T _bi_ = _i_; _bi_ < _end_; _bi_++ )
        {
            UINT8 _n_ = ((PUINT8)_b_)[_bi_];
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
    for ( UINT64 _i_ = 0; _i_ < _s_; _i_++ )
    {
        printf("%02x ", ((PUINT8)_b_)[_i_]);
    }
    printf("\n");
}

FORCEINLINE
void PrintMemByteStr(PVOID _b_, SIZE_T _s_)
{
    for ( UINT64 _i_ = 0; _i_ < _s_; _i_++ )
    {
        printf("%02x", ((PUINT8)_b_)[_i_]);
    }
    printf("\n");
}

FORCEINLINE
void PrintAStr(PVOID b, SIZE_T s)
{
    for ( SIZE_T k = 0; k < s; k++ )
    {
        putPrintableA(((PUINT8)b)[k]);
    }
    printf("\n");
}

FORCEINLINE
void PrintWStr(PVOID b, SIZE_T s)
{
    SIZE_T n = s / 2;
    for ( SIZE_T k = 0; k < n; k++ )
    {
        putPrintableW(*(PUINT16)&(((PUINT8)b)[k * 2]));
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

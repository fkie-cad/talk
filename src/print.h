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

#define PrintMemCols8(_b_, _s_, _a_) \
{ \
    UINT64 _hw_v_ = (SIZE_T)_a_ + _s_; \
    UINT8 _hw_w_ = 0x10; \
    HEX_CHAR_WIDTH(_hw_v_, _hw_w_); \
    \
    for ( ULONG _i_ = 0; _i_ < _s_; _i_+=0x10 ) \
    { \
        ULONG _end_ = (_i_+0x10<_s_) ? (_i_+0x10) : (_s_); \
        ULONG _gap_ = (_i_+0x10<=_s_) ? 0 : ((0x10+_i_-_s_)*3); \
        printf("%.*zx  ", _hw_w_, (((SIZE_T)_a_)+_i_)); \
         \
        for ( ULONG _j_ = _i_, _k_=0; _j_ < _end_; _j_++, _k_++ ) \
        { \
            printf("%02x", ((PUINT8)_b_)[_j_]); \
            printf("%c", (_k_==7?'-':' ')); \
        } \
        for ( ULONG _j_ = 0; _j_ < _gap_; _j_++ ) \
        { \
            printf(" "); \
        } \
        printf("  "); \
        for ( ULONG _k_ = _i_; _k_ < _end_; _k_++ ) \
        { \
            if ( ((PUINT8)_b_)[_k_] < 0x20 || ((PUINT8)_b_)[_k_] > 0x7E || ((PUINT8)_b_)[_k_] == 0x25 ) \
            { \
                printf("."); \
            }  \
            else \
            { \
                printf("%c", ((PUINT8)_b_)[_k_]); \
            } \
        } \
        printf("\n"); \
    } \
}

#define PrintMemCols16(_b_, _s_, _a_) \
{ \
    UINT64 _hw_v_ = (SIZE_T)_a_ + _s_; \
    UINT8 _hw_w_ = 0x10; \
    HEX_CHAR_WIDTH(_hw_v_, _hw_w_); \
    \
    for ( ULONG _i_ = 0; _i_ < _s_; _i_+=0x10 ) \
    { \
        ULONG _end_ = (_i_+0x10<_s_)?(_i_+0x10):(_s_); \
        ULONG _gap_ = (_i_+0x10<=_s_) ? 0 : ((0x10+_i_-_s_)/2*5); \
        printf("%.*zx  ", _hw_w_, (((SIZE_T)_a_)+_i_)); \
         \
        for ( ULONG _j_ = _i_; _j_ < _end_; _j_+=2 ) \
        { \
            printf("%04x ", *(PUINT16)&(((PUINT8)_b_)[_j_])); \
        } \
        for ( ULONG _j_ = 0; _j_ < _gap_; _j_++ ) \
        { \
            printf(" "); \
        } \
        printf("  "); \
        for ( ULONG _j_ = _i_; _j_ < _end_; _j_+=2 ) \
        { \
            printf("%wc", *(PUINT16)&(((PUINT8)_b_)[_j_])); \
        } \
        printf("\n"); \
    } \
}

#define PrintMemCols32(_b_, _s_, _a_) \
{\
    UINT64 _hw_v_ = (SIZE_T)_a_ + _s_; \
    UINT8 _hw_w_ = 0x10; \
    HEX_CHAR_WIDTH(_hw_v_, _hw_w_); \
    \
    for ( ULONG _i_ = 0; _i_ < _s_; _i_+=0x10 ) \
    { \
        ULONG _end_ = (_i_+0x10<_s_)?(_i_+0x10):(_s_); \
        printf("%.*zx  ", _hw_w_, (((SIZE_T)_a_)+_i_)); \
         \
        for ( ULONG _j_ = _i_; _j_ < _end_; _j_+=4 ) \
        { \
            printf("%08x ", *(PUINT32)&(((PUINT8)_b_)[_j_])); \
        } \
        printf("\n"); \
    } \
}

#define PrintMemCols64(_b_, _s_, _a_) \
{\
    UINT64 _hw_v_ = (SIZE_T)_a_ + _s_; \
    UINT8 _hw_w_ = 0x10; \
    HEX_CHAR_WIDTH(_hw_v_, _hw_w_); \
    \
    for ( ULONG _i_ = 0; _i_ < _s_; _i_+=0x10 ) \
    { \
        ULONG _end_ = (_i_+0x10<_s_)?(_i_+0x10):(_s_); \
        printf("%.*zx  ", _hw_w_, (((SIZE_T)_a_)+_i_)); \
         \
        for ( ULONG _j_ = _i_; _j_ < _end_; _j_+=8 ) \
        { \
            printf("%016llx ", *(PUINT64)&(((PUINT8)_b_)[_j_])); \
        } \
        printf("\n"); \
    } \
}

#define PrintMemBytes(_b_, _s_) \
{ \
    for ( ULONG _i_ = 0; _i_ < _s_; _i_++ ) \
    { \
        printf("%02x ", ((PUINT8)_b_)[_i_]); \
    } \
    printf("\n"); \
}

#define STATUS_CASE(__status__) \
    case __status__: return #__status__;
#define STATUS_CASE_DESC(__status__, __desc__) \
    case __status__: return __desc__;

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
        STATUS_CASE_DESC(STATUS_DATATYPE_MISALIGNMENT_ERROR, "STATUS_DATATYPE_MISALIGNMENT_ERROR: A data type misalignment error was detected in a load or store instruction")
        default:
            return "Unknown status code";
    }
}

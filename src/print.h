#pragma once


#define PrintMemCols(_b_, _s_) \
    for ( ULONG _i_ = 0; _i_ < _s_; _i_+=0x10 ) \
    { \
        ULONG _end_ = (_i_+0x10<_s_)?(_i_+0x10):(_s_); \
        ULONG _gap_ = (_i_+0x10<=_s_) ? 0 : ((0x10+_i_-_s_)*3); \
        printf("%p  ", (((PUINT8)_b_)+_i_)); \
         \
        for ( ULONG _j_ = _i_; _j_ < _end_; _j_++ ) \
        { \
            printf("%02x ", ((PUINT8)_b_)[_j_]); \
        } \
        for ( ULONG _j_ = 0; _j_ < _gap_; _j_++ ) \
        { \
            printf(" "); \
        } \
        printf("  "); \
        for ( ULONG _j_ = _i_; _j_ < _end_; _j_++ ) \
        { \
            if ( ((PUINT8)_b_)[_j_] < 0x20 || ((PUINT8)_b_)[_j_] > 0x7E || ((PUINT8)_b_)[_j_] == 0x25 ) \
            { \
                printf("."); \
            }  \
            else \
            { \
                printf("%c", ((PUINT8)_b_)[_j_]); \
            } \
        } \
        printf("\n"); \
    }

#define PrintMemBytes(_b_, _s_) \
    for ( ULONG _i_ = 0; _i_ < _s_; _i_++ ) \
    { \
        printf("%02x ", ((PUINT8)_b_)[_i_]); \
    } \
    printf("\n"); \


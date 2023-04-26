#ifndef CONVERTER_H
#define CONVERTER_H

#include <windows.h>



int parseUint8(const char* arg, UINT8* value, UINT8 base);
int parseUint64(const char* arg, ULONGLONG* value, UINT8 base);


#define IS_NUM(__char__) \
    ( __char__ >= '0' && __char__ <= '9' )

#define IN_HEX_RANGE(__char__) \
    ( IS_NUM(__char__) || ( __char__ >= 'a' && __char__ <= 'f' )  || ( __char__ >= 'A' && __char__ <= 'F' ) )

/**
 * Parse plain byte string into byte array
 */
INT parsePlainBytes(
    _In_ const char* Raw, 
    _Inout_ PUINT8* Buffer, 
    _Inout_ PULONG Size, 
    _In_ ULONG MaxBytes
)
{
    ULONG i, j;
    SIZE_T arg_ln = strlen(Raw);
    PUINT8 p = NULL;
    BOOL malloced = FALSE;
    ULONG buffer_ln;
    int s = 0;

    UINT8 m1, m2;

    if ( arg_ln > MaxBytes * 2ULL )
    {
        printf("Error: Data too big!\n");
        return ERROR_BUFFER_OVERFLOW;
    }

    if ( arg_ln == 0 )
    {
        printf("Error: Buffer is empty!\n");
        return ERROR_INVALID_PARAMETER;
    }
    if ( arg_ln % 2 != 0 )
    {
        printf("Error: Buffer data is not byte aligned!\n");
        return ERROR_INVALID_PARAMETER;
    }
    
    buffer_ln = (ULONG) (arg_ln / 2);

    if ( *Size && *Buffer && buffer_ln > *Size )
    {
        printf("Error: Provided buffer is too small 0x%x < 0x%x!\n", *Size, buffer_ln);
        return ERROR_INVALID_PARAMETER;
    }

    if ( *Buffer == NULL )
    {
        p = (PUINT8) malloc(buffer_ln);
        if ( p == NULL )
        {
            printf("Error: No memory!\n");
            return GetLastError();
        }
        malloced = TRUE;
    }
    else
    {
        p = *Buffer;
    }

    for ( i = 0, j = 0; i < arg_ln; i += 2, j++ )
    {
        if ( !IN_HEX_RANGE(Raw[i]) || !IN_HEX_RANGE(Raw[i+1]) )
        {
            s = ERROR_INVALID_PARAMETER;
            printf("Error: Byte string not in hex range!\n");
            break;
        }

        if ( Raw[i] <= 0x39 )
            m1 = 0x30;
        else if ( Raw[i] <= 0x46 )
            m1 = 0x37;
        else
            m1 = 0x57;

        if ( Raw[i+1] <= 0x39 )
            m2 = 0x30;
        else if ( Raw[i+1] <= 0x46 )
            m2 = 0x37;
        else
            m2 = 0x57;

        p[j] = ((Raw[i] - m1)<<4) | ((Raw[i+1] - m2) & 0x0F);
    }
    
    if ( s != 0 )
    {
        if ( malloced && p )
            free(p);
    }
    else
    {
        *Size = buffer_ln;
        *Buffer = p;
    }

    return s;
}

int parseUint8(const char* arg, UINT8* value, UINT8 base)
{
    ULONGLONG result;
    int s = parseUint64(arg, &result, base);
    if ( s != 0 ) return s;
    if ( s > (UINT8)-1 )
    {
        fprintf(stderr, "Error: %s could not be converted to a byte: Out of range!\n", arg);
        return 5;
    }

    *value = (UINT8) result;
    return 0;
}

int parseUint64(const char* arg, ULONGLONG* value, UINT8 base)
{
    char* endptr;
    int err_no = 0;
    errno = 0;
    ULONGLONG result;

    if ( base != 10 && base != 16 && base != 0 )
    {
        fprintf(stderr, "Error: Unsupported base %u!\n", base);
        return 1;
    }

    if ( arg[0] ==  '-' )
    {
        fprintf(stderr, "Error: %s could not be converted to a number: is negative!\n", arg);
        return 2;
    }

#if defined(_WIN32)
    result = strtoull(arg, &endptr, base);
#else
    result = strtoul(arg, &endptr, base);
#endif
    err_no = errno;

    if ( endptr == arg )
    {
        fprintf(stderr, "Error: %s could not be converted to a number: Not a number!\n", arg);
        return 3;
    }
    if ( result == (ULONGLONG)-1 && err_no == ERANGE )
    {
        fprintf(stderr, "Error: %s could not be converted to a number: Out of range!\n", arg);
        return 4;
    }

    *value = result;
    return 0;
}

#endif

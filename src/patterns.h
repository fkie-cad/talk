#pragma once

#define MAX_DEF_PATTERN_SIZE (26*26*10*3)



INT genPattern(_Inout_ PVOID Buffer, _In_ ULONG Size);
INT genCustomPattern(_In_ UINT64 PatternStart, _In_ ULONG PatternSize, _Inout_ PVOID Buffer, _In_ ULONG BufferSize);



INT genPattern(_Inout_ PVOID Buffer, _In_ ULONG Size)
{
    //ULONG parts = Size / 3;
    ULONG rest = Size % 3;

    PUINT8 b = (PUINT8)Buffer;
    ULONG j = 0;
    UINT8 f = 0;
    UINT8 s = 0;
    UINT8 t = 0;

    if ( !Size || Size > MAX_DEF_PATTERN_SIZE )
        return ERROR_INVALID_PARAMETER;

    for ( f = 'A'; f <= 'Z'; f++ )
    {
        for ( s = 'a'; s <= 'z'; s++ )
        {
            for ( t = '0'; t <= '9'; t++ )
            {
                if ( j + 3 > Size )
                    goto endAligned;
                
                b[j] = f;
                b[j+1] =  s;
                b[j+2] = t;
                
                j += 3;
            }
        }
    }
endAligned:
    if ( rest >= 1 )
    {
        b[j] = f;
    }
    if ( rest == 2 )
    {
        b[j+1] = s;
    }

    return 0;
}

INT genCustomPattern(_In_ UINT64 PatternStart, _In_ ULONG PatternSize, _Inout_ PVOID Buffer, _In_ ULONG BufferSize)
{
    INT s = 0;
    PUINT8 b = (UINT8*)Buffer;
    UINT64 i = 0;
    UINT64 v = 0;
    UINT64 end = BufferSize - ( BufferSize % PatternSize );
    UINT8 rest = (UINT8)(BufferSize % PatternSize);

    DPrint("PatternStart: 0x%llx\n", PatternStart);
    DPrint("PatternSize: 0x%x\n", PatternSize);
    DPrint("end: 0x%llx\n", end);
    
    v = PatternStart;

    for ( i = 0; i < end; i+=PatternSize )
    {
        DPrint("[%llu] v: 0x%llx\n", i, v);
        switch ( PatternSize )
        {
            case 1:
                b[i] = (UINT8)v;
                break;
            case 2:
                *(PUINT16)&b[i] = ((UINT16)v);
                break;
            case 3:
                b[i] =   (UINT8)((v & 0x0000FFu));
                b[i+1] = (UINT8)((v & 0x00FF00u) >> 0x08u);
                b[i+2] = (UINT8)((v & 0xFF0000u) >> 0x10u);
                break;
            case 4:
                *(PUINT32)&b[i] = ((UINT32)v);
                break;
            case 5:
                b[i] =   (UINT8)((v & 0x00000000FFu));
                b[i+1] = (UINT8)((v & 0x000000FF00ull) >> 0x08u);
                b[i+2] = (UINT8)((v & 0x0000FF0000ull) >> 0x10u);
                b[i+3] = (UINT8)((v & 0x00FF000000ull) >> 0x18u);
                b[i+4] = (UINT8)((v & 0xFF00000000ull) >> 0x20u);
                break;
            case 6:
                b[i] =   (UINT8)((v & 0x0000000000FFu));
                b[i+1] = (UINT8)((v & 0x00000000FF00ull) >> 0x08u);
                b[i+2] = (UINT8)((v & 0x000000FF0000ull) >> 0x10u);
                b[i+3] = (UINT8)((v & 0x0000FF000000ull) >> 0x18u);
                b[i+4] = (UINT8)((v & 0x00FF00000000ull) >> 0x20u);
                b[i+5] = (UINT8)((v & 0xFF0000000000ull) >> 0x28u);
                break;
            case 7:
                b[i] =   (UINT8)((v & 0x000000000000FFu));
                b[i+1] = (UINT8)((v & 0x0000000000FF00ull) >> 0x08u);
                b[i+2] = (UINT8)((v & 0x00000000FF0000ull) >> 0x10u);
                b[i+3] = (UINT8)((v & 0x000000FF000000ull) >> 0x18u);
                b[i+4] = (UINT8)((v & 0x0000FF00000000ull) >> 0x20u);
                b[i+5] = (UINT8)((v & 0x00FF0000000000ull) >> 0x28u);
                b[i+6] = (UINT8)((v & 0xFF000000000000ull) >> 0x30u);
                break;
            case 8:
                *(PUINT64)&b[i] = ((UINT64)v);
                break;
            default:
                EPrint("Not aligned!\n");
                s = ERROR_INVALID_PARAMETER;
                goto clean;
        }

        v = swapUint64(v);
        if ( PatternSize < 8 )
            v = v >> ((8-PatternSize) * 8);
        v++;
        if ( PatternSize < 8 )
            v = v << ((8-PatternSize) * 8);
        v = swapUint64(v);
    }

    for ( i = 0; i < rest; i++ )
    {
        UINT64 mask = 0xFFull << (i*8);
        UINT64 shift = (0x08ull*i);
        b[end+i] = (UINT8)((v & mask) >> shift);
    }

clean:
    return s;
}

#pragma once

UINT32 countHexChars(UINT64 Value)
{
    UINT32 shift = 0x3c;
    UINT32 zc = 0;

    if ( !Value )
        return 1;

    while ( (((Value >> shift)&0xF) ^ 0xF ) == 0xF )
    {
        zc++;
        shift-=4;
    }

    return 16-zc;
}

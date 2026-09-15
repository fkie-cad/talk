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

FORCEINLINE
SIZE_T getNrOfPages(SIZE_T Size, SIZE_T LastPageBytes)
{
    SIZE_T n = 0;
    if ( Size <= LastPageBytes ) 
    {
        n = 1;
    }
    else
    {
        n = Size - LastPageBytes;
        n = ALIGN_DOWN_TO_PREV_BY(n, PAGE_SIZE);
        n = ( n / PAGE_SIZE ) + 2;
    }
    return n;
}

FORCEINLINE
SIZE_T getBufferOffset(SIZE_T Size, SIZE_T LastPageBytes)
{
    SIZE_T offset = 0;
    if ( Size <= LastPageBytes ) 
    {
        offset = 0;
    }
    else
    {
        offset = ( PAGE_SIZE + LastPageBytes - ( Size % PAGE_SIZE ) ) % PAGE_SIZE;
    }
    return offset;
}

//
// allocate page aligned buffer.
// The buffer reaches max LastPageBytes bytes into a new page.
// This should maximize the amount of memory accessible beyond the buffer not crossing a page border.
//
PVOID allocCMPL(SIZE_T Size, PVOID* Base, SIZE_T LastPageBytes)
{
    FEnter();

    NTSTATUS status = 0;

    HANDLE process = (HANDLE)-1;
    PVOID baseAddress = NULL;
    PVOID bufferAddress = NULL;
    SIZE_T zeroBits = 0;
    SIZE_T regionSize = 0;
    
    SIZE_T nrOfPages = getNrOfPages(Size, LastPageBytes);
    DPrint("nrOfPages: 0x%zx\n", nrOfPages);

    *Base = 0;

    regionSize = nrOfPages * PAGE_SIZE;
    DPrint("regionSize: 0x%zx\n", regionSize);

    status = NtAllocateVirtualMemory(
            process,
            &baseAddress,
            zeroBits,
            &regionSize,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE
        );
    if ( status != 0 )
    {
        EPrint("NtAllocateVirtualMemory failed! (0x%x)\n", status);
        goto clean;
    }
    DPrint("baseAddress : 0x%p\n", baseAddress);
    DPrint("regionSize: 0x%zx\n", regionSize);

    *Base = baseAddress;
    bufferAddress = (PVOID)((SIZE_T)baseAddress + getBufferOffset(Size, LastPageBytes));

clean:

    FLeave();
    return bufferAddress;
}

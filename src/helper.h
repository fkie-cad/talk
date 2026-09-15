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
SIZE_T getNrOfPages(SIZE_T Size, SIZE_T Mpp)
{
    SIZE_T n = 0;
    if ( Size <= Mpp ) 
    {
        n = 1;
    }
    else
    {
        n = Size - Mpp;
        n = ALIGN_DOWN_TO_LAST_BY(n, PAGE_SIZE);
        n = ( n / PAGE_SIZE ) + 2;
    }
    return n;
}

FORCEINLINE
SIZE_T getBufferOffset(SIZE_T Size, SIZE_T Mpp)
{
    SIZE_T offset = 0;
    if ( Size <= Mpp ) 
    {
        offset = 0;
    }
    else
    {
        offset = Size % PAGE_SIZE;
        offset = PAGE_SIZE - offset + Mpp;
    }
    return offset;
}

//
// allocate buffer aligned buffer.
// The buffer reaches max Mpp bytes into a new page.
// This should maximize the amount of memory accessible beyond the buffer not crossing a page border.
//
PVOID allocCMPL(SIZE_T Size, PVOID* Base, SIZE_T Mpp)
{
    FEnter();

    NTSTATUS status = 0;

    HANDLE process = (HANDLE)-1;
    PVOID baseAddress = NULL;
    PVOID bufferAddress = NULL;
    SIZE_T zeroBits = 0;
    SIZE_T regionSize = 0;
    
    SIZE_T nrOfPages = getNrOfPages(Size, Mpp);
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
    bufferAddress = (PVOID)((SIZE_T)baseAddress + getBufferOffset(Size, Mpp));

clean:

    FLeave();
    return bufferAddress;
}

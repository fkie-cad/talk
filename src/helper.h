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

//
// allocate buffer 
//
//PVOID allocCMPL(SIZE_T Size)
//{
//    NTSTATUS status = 0;
//
//    ULONG ret_len;
//
//    HANDLE process = (HANDLE)-1;
//    PVOID baseAddress = NULL;
//    SIZE_T zeroBits = 0;
//    SIZE_T regionSize = 0;
//    SIZE_T protectSize = 0;
//    DWORD oldProtect = 0;
//    PVOID outImagebase = NULL;
//    PVOID nextPageBaseAddress = NULL;
//    
//    printf("Process  : 0x%p\n", process);
//    regionSize = _nr_of_pages * tsbi->PageSize;
//
//    status = NtAllocateVirtualMemory(
//            process,
//            &baseAddress,
//            zeroBits,
//            &regionSize,
//            MEM_COMMIT | MEM_RESERVE,
//            PAGE_READWRITE
//        );
//    if (NT_ERROR(status))
//    {
//        printf("ERROR (0x%lx): NtAllocateVirtualMemory failed", status);
//        return 0;
//    }
//    printf("SUCCESS: memory allocated\n");
//
//    printf("page_size : 0x%lx\n", *_size_of_page);
//    printf("base_addr : 0x%p\n", baseAddress);
//    printf("RegionSize: 0x%zx\n", regionSize);
//
//    nextPageBaseAddress = (PVOID)(((SIZE_T)baseAddress) + page_size);
//    printf("NextPageBaseAddress : 0x%p\n", nextPageBaseAddress);
////    RegionSize = 0x1000;
//    protectSize = 1;
//    printf("ProtectSize         : 0x%zx\n", protectSize);
//
//    outImagebase = (PVOID)(((SIZE_T)baseAddress) + page_size);
//    printf("out_imagebase : 0x%p\n", outImagebase);
//    oldProtect = 0;
//    printf("protectx : 0x%lx\n", oldProtect);
//    printf("Process  : 0x%p\n", process);
//    status = NtProtectVirtualMemory(
//            process, 
//            &(outImagebase), 
//            &protectSize, 
//            PAGE_NOACCESS, 
//            &oldProtect
//        );
//
//    printf("status : 0x%lx\n", status);
//    printf("oldprotectx : 0x%lx\n", oldProtect);
//    printf("PAGE_NOACCESS : 0x%lx\n", PAGE_NOACCESS);
//    printf("ProtectSize : 0x%zx\n", protectSize);
//    if ( !NT_SUCCESS(status) )
////    if (NT_ERROR(s))
//    {
//        printf("ERROR (0x%lx): nt_protect_virtual_memory\n", status);
//        exit(0);
//    }
//    printf("nt_protect_virtual_memory success : 0x%lx\n", status);
//    printf("base_addr : 0x%p\n", baseAddress);
//    printf("NextPageBaseAddress : 0x%p\n", nextPageBaseAddress);
//    printf("RegionSize: 0x%zx\n", regionSize);
//    
//    return baseAddress;
//}

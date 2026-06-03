#pragma once

typedef struct _IOPUT_INT {
    INT Id;
    INT Size;
} 
IOPUT_INT, *PIOPUT_INT,
INPUT_INT, *PINPUT_INT,
OUTPUT_INT, *POUTPUT_INT;

typedef struct _SE {
    PULONG List;
    ULONG Count;
} SE, *PSE;

#define STR_TO_ULONG_CLN(__out__, __val__, __s__) \
{ \
    __try { \
        __out__ = (ULONG)strtoul(__val__, NULL, 0); \
    } \
    __except ( EXCEPTION_EXECUTE_HANDLER ) \
    { \
        __s__ = GetExceptionCode(); \
        printf("[X] Exception parsing input number! (0x%x)\n", __s__); \
        goto clean; \
    } \
}

int parseIOnts(_In_ int argc, _In_ char** argv, _In_ PIOPUT_INT Data, _In_ INT Count, _Inout_ PVOID* Buffer, _Inout_ PULONG BufferSize);
INT parseSe(_In_ INT argc, _In_reads_(argc) CHAR** argv, _Inout_ PSE Se, _In_ PINT Ids, _In_ INT IdsCount);


INT parseStringA(_In_ PCHAR Value, _Out_ PVOID *Buffer, _Out_ PULONG Size)
{
    INT s = 0;

    PVOID buffer = NULL;
    ULONG cch = (ULONG)strlen(Value);
    ULONG cb = cch;
    ULONG size = cb + 1;
    
    *Buffer = NULL;
    *Size = 0;

    buffer = malloc(size);
    if ( !buffer )
    {
        s = ERROR_NO_SYSTEM_RESOURCES;
        EPrint("No memory for data!\n");
        goto clean;
    }
    memcpy(buffer, Value, size);

    *Buffer = buffer;
    *Size = size;

clean:
    return s;
}

INT parseStringU(_In_ PCHAR Value, _Out_ PVOID *Buffer, _Out_ PULONG Size)
{
    INT s = 0;

    PVOID buffer = NULL;
    SIZE_T cch = strlen(Value);
    
    *Buffer = NULL;
    *Size = 0;
    
    if ( !cch || cch > (((ULONG)-1) / 2) - 1 )
        return STATUS_INVALID_PARAMETER;

    ULONG cb = (ULONG)cch * 2;
    ULONG bufferSize = cb + 2;
    

    buffer = malloc(bufferSize);
    if ( !buffer )
    {
        s = ERROR_NO_SYSTEM_RESOURCES;
        EPrint("No memory for data!\n");
        goto clean;
    }
    s = StringCbPrintfW((PWCHAR)buffer, bufferSize, L"%hs", Value);

    *Buffer = buffer;
    *Size = bufferSize;

clean:
    return s;
}

INT parseFile(_In_ PCHAR FilePath, _Out_ PVOID *Buffer, _Out_ PULONG Size)
{
    INT s = 0;
    BOOL b = FALSE;
    
    PWCHAR ntPath = NULL;
    HANDLE file = NULL;

    WCHAR filePathW[MAX_PATH];
    ULONG cch = 0;
    ULONG cb = 0;

    PVOID buffer = NULL;
    ULONG size = 0;

    *Buffer = NULL;
    *Size = 0;

    s = StringCchPrintfW(filePathW, MAX_PATH, L"%hs", FilePath);
    if ( s != 0 )
        goto clean;

    cch = GetFullPathNameW(filePathW, 0, NULL, NULL);
    if ( cch == 0 )
    {
        s = GetLastError();
        EPrint("Getting full path failed! (0x%x)\n", s);
        goto clean;
    }
    cch += 4; // nt prefix
    cb = cch * 2;
    ntPath = (PWCHAR)malloc(cb);
    if ( !ntPath )
    {
        s = ERROR_NO_SYSTEM_RESOURCES;
        EPrint("No memory for path!\n");
        goto clean;
    }
    cch = GetFullPathNameW(filePathW, cch-4, &ntPath[4], NULL);
    if ( cch == 0 )
    {
        s = GetLastError();
        EPrint("Getting full path failed! (0x%x)\n", s);
        goto clean;
    }
    *(PUINT64)ntPath = NT_PATH_PREFIX_W;

    s = openFile(&file, ntPath, FILE_GENERIC_READ, FILE_SHARE_READ);
    if ( s != 0 )
    {
        goto clean;
    }

    LARGE_INTEGER fileSize = {0};
    b = GetFileSizeEx(file, &fileSize);
    if ( !b || !fileSize.QuadPart )
    {
        s = GetLastError();
        EPrint("Getting file size failed! (0x%x)\n", s);
        goto clean;
    }
    if ( fileSize.HighPart )
    {
        s = ERROR_INVALID_PARAMETER;
        EPrint("File too big! (0x%x)\n", s);
        goto clean;
    }

    buffer = malloc(fileSize.LowPart);
    if ( !buffer )
    {
        s = ERROR_NO_SYSTEM_RESOURCES;
        EPrint("No memory for file data!\n");
        goto clean;
    }
    size = fileSize.LowPart;
            
    IO_STATUS_BLOCK iosb;
    RtlZeroMemory(&iosb, sizeof(iosb));
            
    s = NtReadFile(file, NULL, NULL, NULL, &iosb, buffer, size, NULL, NULL);
    if ( s != 0 ) 
    {
        EPrint("Reading file data failed! (0x%x)\n", s);
        goto clean;
    }
    size = (ULONG) iosb.Information;
    
    *Buffer = buffer;
    *Size = size;

clean:
    if ( s != 0 )
        free(buffer);

    if ( ntPath )
        free(ntPath);
    if ( file )
        NtClose(file);

    return s;
}

INT parseRandom(_In_ PCHAR SizeStr, _Out_ PVOID *Buffer, _Out_ PULONG BufferSize)
{
    INT s = 0;
                
    PVOID buffer = NULL;
    ULONG size = 0;

    *Buffer = NULL;
    *BufferSize = 0;

    STR_TO_ULONG_CLN(size, SizeStr, s);
    buffer = malloc(size);
    if ( !buffer )
    {
        s = ERROR_NO_SYSTEM_RESOURCES;
        EPrint("No memory for random data!\n");
        goto clean;
    }

    s = genRand(buffer, size);
    if ( s != 0 )
    {
        EPrint("Generating random failed!\n");
        goto clean;
    }
    
    *Buffer = buffer;
    *BufferSize = size;

clean:
    if ( s != 0 )
        free(buffer);

    return s;
}

INT parsePattern(_In_ PCHAR SizeStr, _Out_ PVOID *Buffer, _Out_ PULONG BufferSize)
{
    INT s = 0;
    
    PVOID buffer = NULL;
    ULONG size = 0;

    *Buffer = NULL;
    *BufferSize = 0;

    STR_TO_ULONG_CLN(size, SizeStr, s);
    buffer = malloc(size);
    if ( !buffer )
    {
        s = ERROR_NO_SYSTEM_RESOURCES;
        EPrint("No memory for pattern data!\n");
        goto clean;
    }

    s = genPattern(buffer, size);
    if ( s != 0 )
    {
        EPrint("Generating pattern failed!\n");
        goto clean;
    }
    
    *Buffer = buffer;
    *BufferSize = size;

clean:
    if ( s != 0 )
        free(buffer);

    return s;
}

INT parseCustomPattern(_In_ PCHAR Pattern, _In_ PCHAR PatternSizeStr, _Out_ PVOID *Buffer, _Out_ PULONG BufferSize)
{
    INT s = 0;
                
    UINT64 patternStart = 0;
    PUINT8 patternStartPtr = (PUINT8)&patternStart;
    ULONG patternCb = 0;

    PVOID buffer = NULL;
    ULONG bufferSize = 0;

    *Buffer = NULL;
    *BufferSize = 0;

    s = parsePlainBytes(Pattern, &patternStartPtr, &patternCb, 8);
    if ( s != 0 )
    {
        goto clean;
    }

    bufferSize = (ULONG)strtoul(PatternSizeStr, NULL, 0);
    if ( !bufferSize )
        return ERROR_INVALID_PARAMETER;

    buffer = malloc(bufferSize);
    if ( !buffer )
    {
        s = ERROR_NO_SYSTEM_RESOURCES;
        EPrint("No memory for pattern data!\n");
        goto clean;
    }

    s = genCustomPattern(patternStart, patternCb, buffer, bufferSize);
    if ( s != 0 )
    {
        EPrint("Generating pattern failed!\n");
        goto clean;
    }
    
    *Buffer = buffer;
    *BufferSize = bufferSize;

clean:
    if ( s != 0 )
        free(buffer);

    return s;
}

FORCEINLINE
INT parseIOBSize(_Inout_ PVOID* Buffer, _In_ ULONG BufferSize, _In_ UINT8 FillValue)
{
    INT s = 0;

    if ( BufferSize > 0 && !(*Buffer) )
    {
        (*Buffer) = malloc(BufferSize);
        if ( !(*Buffer) )
        {
            s = ERROR_NO_SYSTEM_RESOURCES;
            EPrint("Buffer malloc failed! (0x%x)\n", s);
            goto clean;
        }
        memset((*Buffer), FillValue, BufferSize);
    }

clean:
    return s;
}

// parse input ints to an input "struct"
int parseIOnts(
    _In_ int argc, 
    _In_ char** argv, 
    _In_ PIOPUT_INT Data, 
    _In_ INT Count, 
    _Inout_ PVOID* Buffer,
    _Inout_ PULONG BufferSize
)
{
    int s = 0;
    INT i;
                
    PVOID buffer = NULL;
    ULONG size = 0;

    if ( !Count )
        return 0;

    if ( *Buffer )
    {
        printf("[i] Data already set! Skipping!\n");
        return 0;
    }

    // get needed size
    // max is MAX_INTS * 8 = 0x80
    for ( i = 0; i < Count; i++ )
    {
        size += Data[i].Size;
    }

    // allocate buffer
    buffer = malloc(size);
    if ( !buffer )
    {
        s = ERROR_NO_SYSTEM_RESOURCES;
        EPrint("No memory for data! (0x%x)\n", s);
        goto clean;
    }

    // fill buffer
    
    __try
    {
        PUINT8 ptr = buffer;
        for ( i = 0; i < Count; i++ )
        {
            // should not happen
            if ( Data[i].Id >= argc )
            {
                s = ERROR_INVALID_PARAMETER;
                EPrint("Invalid int id! (0x%x)\n", s);
                goto clean;
            }

            switch ( Data[i].Size )
            {
                case 1:
                    *(PUINT8)ptr = (UINT8)strtoul(argv[Data[i].Id], NULL, 0);
                    ptr++;
                    break;
                case 2:
                    *(PUINT16)ptr = (UINT16)strtoul(argv[Data[i].Id], NULL, 0);
                    ptr+=2;
                    break;
                case 4:
                    *(PUINT32)ptr = (UINT32)strtoul(argv[Data[i].Id], NULL, 0);
                    ptr+=4;
                    break;
                case 8:
                    *(PUINT64)ptr = (UINT64)strtoull(argv[Data[i].Id], NULL, 0);
                    ptr+=8;
                    break;
                default:
                    s = ERROR_INVALID_PARAMETER;
                    goto clean;
            }
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        s = GetExceptionCode();
        printf("[X] Exception parsing input number! (0x%x)\n", s);
        goto clean;
    }

    *Buffer = buffer;
    *BufferSize = size;

clean:
    if ( s != 0 )
        free(buffer);

    return s;
}

INT parseSe(
    _In_ INT argc, 
    _In_reads_(argc) CHAR** argv, 
    _Inout_ PSE Se, 
    _In_ PINT Ids, 
    _In_ INT IdsCount
)
{
    INT i;
    PCHAR arg = NULL;
    SIZE_T reqSize = 0;
    ULONG count = 0;
    
    if ( IdsCount == 0 )
        return 0;

    //
    // get required size of flat array

    for ( i = 0; i < IdsCount; i++ )
    {
        if ( Ids[i] >= argc )
        {
            Ids[i] = -1;
            continue;
        }

        arg = argv[Ids[i]];
        
        // sanitization checks
        if ( arg == NULL || arg[0] == 0 || arg[0] == LIN_PARAM_IDENTIFIER || arg[0] == WIN_PARAM_IDENTIFIER )
        {
            Ids[i] = -1;
            continue;
        }
        
        // value checks
        UINT32 val = strtoul(arg, NULL, 0);
        if ( val < SE_MIN_WELL_KNOWN_PRIVILEGE || val > SE_MAX_WELL_KNOWN_PRIVILEGE )
        {
            Ids[i] = -1;
            continue;
        }

        count++;
    }

    if ( count == 0 )
        return -1;
    
    reqSize = count * sizeof(UINT32);
    if ( reqSize > ULONG_MAX )
        return -1;


    //
    // alloc buffer
    
    Se->List = (PULONG)malloc(reqSize);
    if ( !Se->List )
        return ERROR_NO_SYSTEM_RESOURCES;
    RtlZeroMemory(Se->List, reqSize);

    //
    // fill array buffer with strings
    
    ULONG j = 0;
    for ( i = 0; i < IdsCount; i++ )
    {
        if ( Ids[i] == -1)
        {
            continue;
        }
        
        arg = argv[Ids[i]];
        UINT32 val = strtoul(arg, NULL, 0);

        Se->List[j] = val;
        j++;
    }
    Se->Count = j;

    DPrintMemCols32(Se->List, (Se->Count*4), 0);

    return 0;
}

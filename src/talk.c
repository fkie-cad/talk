#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>

#include "nt.h"
#include "warnings.h"
#include "Converter.h"
#include "Args.h"
#include "print.h"
#include "crypto/BRand.h"
#include "privs.h"
#include "helper.h"



#define BIN_NAME "Talk"
#define VERSION "2.1.9"
#define LAST_CHANGED "06.11.2025"


#define PRINT_MODE_NONE         (0x00) // 0000
#define PRINT_MODE_BYTES        (0x01) // 0001
#define PRINT_MODE_BYTE_STR     (0x02) // 0010
#define PRINT_MODE_COLS_8       (0x03) // 0011
#define PRINT_MODE_COLS_16      (0x04) // 0100
#define PRINT_MODE_COLS_32      (0x05) // 0101
#define PRINT_MODE_COLS_64      (0x06) // 0101
#define PRINT_MODE_COLS_BITS    (0x07) // 0111
#define PRINT_MODE_ASCII        (0x08) // 1000
#define PRINT_MODE_UNICODE      (0x09) // 1001

#define PRINT_MODE_MAX  PRINT_MODE_UNICODE 

#define DEFAULT_FILL_VALUE ('A')

#define MAX_DEF_PATTERN_SIZE (26*26*10*3)

#define MAX_SE_COUNT (0x10)

#define MAX_INTS (0x10)
typedef struct _INPUT_INT {
    INT Id;
    INT Size;
} INPUT_INT, *PINPUT_INT;

typedef struct _SE {
    PULONG List;
    ULONG Count;
} SE, *PSE;

typedef struct CmdParams {
    CHAR* DeviceName;
    ULONG InputBufferSize;
    ULONG OutputBufferSize;
    PUINT8 InputBufferData;
    ULONG IOCTL;
    ULONG Sleep;
    ACCESS_MASK DesiredAccess;
    ULONG ShareAccess;
    SE Se;
    struct {
        ULONG Verbose:1;
        ULONG PrintMode:4;
        ULONG Reserved:27;
    } Flags;
    BOOL TestHandle;
    CHAR FillValue;
} CmdParams, * PCmdParams;



INT genPattern(_Inout_ PVOID Buffer, _In_ ULONG Size);
INT genCustomPattern(_In_ UINT64 PatternStart, _In_ ULONG PatternSize, _Inout_ PVOID Buffer, _In_ ULONG BufferSize);

int parseInts(_In_ int argc, _In_ char** argv, _In_ PINPUT_INT ints, _In_ INT intsCount, _Inout_ CmdParams* Params);
INT parseSe(_In_ INT argc, _In_reads_(argc) CHAR** argv, _Inout_ PSE Se, _In_ PINT Ids, _In_ INT IdsCount);

BOOL parseArgs(_In_ INT argc, _In_ CHAR** argv, _Out_ CmdParams* Params);
BOOL checkArgs(_In_ CmdParams* Params);
void printArgs(_In_ PCmdParams Params);

void printUsage();
void printHelp();

int openDevice(
    _Out_ PHANDLE Device, 
    _In_ CHAR* DeviceNameA, 
    _In_ ACCESS_MASK DesiredAccess, 
    _In_ ULONG ShareAccess
);

int generateIoRequest(
    _In_ HANDLE Device, 
    _In_ PCmdParams Params
);

int openFile(
    _Out_ PHANDLE File, 
    _In_ PWCHAR FileName, 
    _In_ ACCESS_MASK DesiredAccess, 
    _In_ ULONG ShareAccess
);



int _cdecl main(int argc, char** argv)
{
    HANDLE device = NULL;
    CmdParams params;
    INT s;

    if ( isAskForHelp(argc, argv) )
    {
        printHelp();
        return 0;
    }

    if ( !parseArgs(argc, argv, &params) )
    {
        printUsage();
        return -2;
    }

    if ( !checkArgs(&params) )
    {
        printUsage();
        return -1;
    }

    if ( params.Flags.Verbose )
        printArgs(&params);



    if ( params.Se.Count)
    {
        s = setPrivileges(params.Se.List, params.Se.Count, TRUE);
        if ( !NT_SUCCESS(s) )
        {
            EPrint("Requested privileges could not be assigned! (0x%x)\n", s);
            params.Se.Count = 0;
            goto clean;
        }
        DPrint("SE privileges assigned.\n");
    }



    s = openDevice(&device, params.DeviceName, params.DesiredAccess, params.ShareAccess);
    if ( s != 0 )
    {
        goto clean;
    }
    else if ( params.TestHandle )
    {
        printf("Device opened successfully: %p\n", device);
        goto clean;
    }
    else
    {
        if ( params.Flags.Verbose )
        {
            printf("device: %p\n", device);
            printf("\n");
        }
    }

    s = generateIoRequest(device, &params);

clean:
    if ( params.Se.Count )
    {
        s = setPrivileges(params.Se.List, params.Se.Count, FALSE);
        if ( !NT_SUCCESS(s) )
        {
            EPrint("Requested privileges could not be resigned! (0x%x)\n", s);
        }
    }
    if ( params.Se.List != NULL )
        free(params.Se.List);
    if ( params.InputBufferData != NULL )
        free(params.InputBufferData);

    if ( device )
        NtClose(device);

    return s;
}

int generateIoRequest(_In_ HANDLE Device, _In_ PCmdParams Params)
{
    ULONG bytesReturned = 0;
    NTSTATUS status = 0;
    IO_STATUS_BLOCK iosb;

    PUINT8 inputBuffer = NULL;
    PUINT8 outputBuffer = NULL;
    
    HANDLE event = NULL;

    if ( Params->InputBufferData )
    {
        inputBuffer = Params->InputBufferData;
    }

    if ( Params->OutputBufferSize > 0 )
    {
        outputBuffer = malloc(Params->OutputBufferSize);
        if ( !outputBuffer )
        {
            status = GetLastError();
            printf("Error (0x%08x): outputBuffer malloc failed\n", status);
            goto clean;
        }

        memset(outputBuffer, 0, Params->OutputBufferSize);
    }

    status = NtCreateEvent(&event,
                           FILE_ALL_ACCESS,
                           0,
                           NotificationEvent,
                           0);
    if ( status != STATUS_SUCCESS )
    {
        EPrint("Create event failed! (0x%08x)\n", status);
        if ( Params->Flags.Verbose )
            printf("    %s\n", getStatusString(status));
        goto clean;
    }

    RtlZeroMemory(&iosb, sizeof(iosb));
    
    printf("Launching I/O Request Packet...\n");

    status = NtDeviceIoControlFile(
                Device,
                event,
                NULL,
                NULL,
                &iosb,
                Params->IOCTL,
                inputBuffer,
                Params->InputBufferSize,
                outputBuffer,
                Params->OutputBufferSize
            );
    
    if ( Params->Flags.Verbose )
    {
        printf("returned\n");
        printf("  status: 0x%x\n",status);
        printf("  iosb.status: 0x%x\n",iosb.Status);
    }
    if ( status == STATUS_PENDING )
    {
        if ( Params->Flags.Verbose )
        {
            printf("Pending\n");
            printf("Waiting for event to signal.\n");
        }
        status = NtWaitForSingleObject(event, 0, 0);
    }

    if ( !NT_SUCCESS(status) )
    {
        EPrint("DeviceIo failed! (0x%08x)\n", status);
        if ( Params->Flags.Verbose )
        {
            printf("    %s\n", getStatusString(status));
            printf("    iosb info: 0x%08x\n", (ULONG)iosb.Information);
        }
        goto clean;
    };

    if ( Params->Sleep )
    {
        if ( Params->Flags.Verbose )
            printf("Sleeping for 0x%x (%u) ms.\n", Params->Sleep, Params->Sleep);

        Sleep(Params->Sleep);
    }

    printf("\n");
    
    bytesReturned = (ULONG)iosb.Information;

    printf("The driver returned 0x%x bytes:\n", bytesReturned);
    printf("-----------------------------");
    UINT32 zc = countHexChars(bytesReturned);
    for ( UINT32 zci = 0; zci < zc; zci++ ) printf("-");
    printf("\n");

    if ( bytesReturned && bytesReturned <= Params->OutputBufferSize )
    {
// warning C6385: Reading invalid data from 'outputBuffer':  the readable size is 'Params->OutputBufferSize' bytes, but '2' bytes may be read ??
#pragma warning ( disable : 6385 )
        switch ( Params->Flags.PrintMode )
        {
            case PRINT_MODE_BYTES:
                PrintMemBytes(outputBuffer, bytesReturned);
                break;
            case PRINT_MODE_BYTE_STR:
                PrintMemByteStr(outputBuffer, bytesReturned);
                break;
            case PRINT_MODE_COLS_16:
                PrintMemCols16(outputBuffer, bytesReturned, 0);
                break;
            case PRINT_MODE_COLS_32:
                PrintMemCols32(outputBuffer, bytesReturned, 0);
                break;
            case PRINT_MODE_COLS_64:
                PrintMemCols64(outputBuffer, bytesReturned, 0);
                break;
            case PRINT_MODE_COLS_BITS:
                PrintMemColsBits(outputBuffer, bytesReturned, 0);
                break;
            case PRINT_MODE_ASCII:
                printf("%.*s\n", bytesReturned, outputBuffer);
                break;
            case PRINT_MODE_UNICODE:
                printf("%.*ws\n", bytesReturned/2, (PWCHAR)outputBuffer);
                break;
            default:
                PrintMemCols8(outputBuffer, bytesReturned, 0);
                break;
        }
#pragma warning ( default : 6385 )
    }
    printf("-----------------------------");
    for ( UINT32 zci = 0; zci < zc; zci++ ) printf("-");
    printf("\n");


clean:
    if ( !Params->InputBufferData && inputBuffer )
        free(inputBuffer);

    if ( outputBuffer )
        free(outputBuffer);

    if ( event )
        NtClose(event);

    return status;
}

#define STR_TO_ULONG(__out__, __val__, __s__) \
{ \
    __try { \
        __out__ = (ULONG)strtoul(__val__, NULL, 0); \
    } \
    __except ( EXCEPTION_EXECUTE_HANDLER ) \
    { \
        __s__ = GetExceptionCode(); \
        printf("[X] Exception parsing input number! (0x%x)\n", __s__); \
        break; \
    } \
}

#define STR_TO_ULONG_X(__out__, __val__, __s__) \
{ \
    __try { \
        __out__ = (ULONG)strtoul(__val__, NULL, 16); \
    } \
    __except ( EXCEPTION_EXECUTE_HANDLER ) \
    { \
        __s__ = GetExceptionCode(); \
        printf("[X] Exception parsing input number! (0x%x)\n", __s__); \
        break; \
    } \
}

#define CONTINUE_IF_ID_SET(__id__, __i__) \
{ \
    if ( __id__ ) \
    { \
        printf("[i] InputData already set! Skipping!\n"); \
        __i__++; \
        continue; \
    } \
}

#define CONTINUE_IF_MAX_INT_COUNT(__cnt__, __i__) \
{ \
    if ( __cnt__ >= MAX_INTS ) \
    { \
        DPrint("[i] Maximum number of input integers reached!\n Skipping!\n"); \
        __i__++; \
        continue; \
    } \
}

BOOL parseArgs(_In_ INT argc, _In_ CHAR** argv, _Out_ CmdParams* Params)
{
    INT start_i = 1;
    INT last_i = argc;
    INT i;
    INT s = 0;
    BOOL b;

    char* arg = NULL;
    char *val1 = NULL;
    char *val2 = NULL;
    
    ULONG cb;
    ULONG cch;

    PWCHAR ntPath = NULL;
    HANDLE file = NULL;

    INT intCount = 0;
    INPUT_INT ints[MAX_INTS] = { 0 };

    INT seId[MAX_SE_COUNT] = {0};
    INT seIdCount = 0;
    
    ZeroMemory(Params, sizeof(CmdParams));
    Params->DesiredAccess = FILE_GENERIC_READ|FILE_GENERIC_WRITE;
    Params->ShareAccess = FILE_SHARE_READ|FILE_SHARE_WRITE;
    Params->FillValue = DEFAULT_FILL_VALUE;

    for ( i = start_i; i < last_i; i++ )
    {
        arg = argv[i];
        val1 = GET_ARG_VALUE(argc, argv, i, 1);

        if ( !arg )
            break;

        if ( IS_1C_ARG(arg, 'n') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No name set!\n");

            Params->DeviceName = val1;
            i++;
        }
        else if ( IS_2C_ARG(arg, 'os') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No output length set!\n");

            STR_TO_ULONG(Params->OutputBufferSize, val1, s);

            i++;
        }
        else if ( IS_1C_ARG(arg, 'c') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No ioctl code set!\n");
  
            STR_TO_ULONG_X(Params->IOCTL, val1, s);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'ix') || IS_2C_ARG(arg, 'ih') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No hex data set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            s = parsePlainBytes(val1, &Params->InputBufferData, &Params->InputBufferSize, MAXUINT32);
            if ( s != 0 )
            {
                break;
            }
            
            i++;
        }
        else if ( IS_2C_ARG(arg, 'ib') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No byte data set!\n");

            //CONTINUE_IF_ID_SET(Params->InputBufferData, i);
            CONTINUE_IF_MAX_INT_COUNT(intCount, i);

            //ints[intCount] = { .Id = i+1, .Size = 1 };
            //ints[intCount] = { i+1, 1 };
            ints[intCount].Id = i+1;
            ints[intCount].Size = 1;
            intCount++;
            
            i++;
        }
        else if ( IS_2C_ARG(arg, 'iw') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No word data set!\n");

            //CONTINUE_IF_ID_SET(Params->InputBufferData, i);
            CONTINUE_IF_MAX_INT_COUNT(intCount, i);
            
            ints[intCount].Id = i+1;
            ints[intCount].Size = 2;
            intCount++;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'id') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No dword data set!\n");
            
            CONTINUE_IF_MAX_INT_COUNT(intCount, i);
            //CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            ints[intCount].Id = i+1;
            ints[intCount].Size = 4;
            intCount++;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'iq') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No qword data set!\n");
            
            CONTINUE_IF_MAX_INT_COUNT(intCount, i);
            //CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            ints[intCount].Id = i+1;
            ints[intCount].Size = 8;
            intCount++;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'ia') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No ascii string data set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            cch = (ULONG)strlen(val1);
            cb = cch;
            Params->InputBufferSize = cb + 1;
            Params->InputBufferData = malloc(Params->InputBufferSize);
            if ( !Params->InputBufferData )
            {
                s = ERROR_NOT_ENOUGH_MEMORY;
                EPrint("No memory for input data!\n");
                Params->InputBufferSize = 0;
                break;
            }
            memcpy(Params->InputBufferData, val1, Params->InputBufferSize);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'iu') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No unicode string data set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            cch = (ULONG)strlen(val1);
            cb = cch * 2;
            Params->InputBufferSize = cb + 2;
            Params->InputBufferData = malloc(Params->InputBufferSize);
            if ( !Params->InputBufferData )
            {
                s = ERROR_NOT_ENOUGH_MEMORY;
                EPrint("No memory for input data!\n");
                Params->InputBufferSize = 0;
                break;
            }
            StringCchPrintfW((PWCHAR)Params->InputBufferData, cch+1, L"%hs", val1);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'if') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No file data set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            WCHAR val1W[MAX_PATH];
            StringCchPrintfW(val1W, MAX_PATH, L"%hs", val1);
            cch = GetFullPathNameW(val1W, 0, NULL, NULL);
            if ( cch == 0 )
            {
                s = GetLastError();
                EPrint("Getting full path failed! (0x%x)\n", s);
                break;
            }
            cch += 4; // nt prefix
            cb = cch * 2;
            ntPath = (PWCHAR)malloc(cb);
            if ( !ntPath )
            {
                s = ERROR_NOT_ENOUGH_MEMORY;
                EPrint("No memory for path!\n");
                break;
            }
            cch = GetFullPathNameW(val1W, cch-4, &ntPath[4], NULL);
            if ( cch == 0 )
            {
                s = GetLastError();
                EPrint("Getting full path failed! (0x%x)\n", s);
                break;
            }
            *(PUINT64)ntPath = NT_PATH_PREFIX_W;

            s = openFile(&file, ntPath, FILE_GENERIC_READ, FILE_SHARE_READ);
            if ( s != 0 )
            {
                break;
            }

            LARGE_INTEGER fileSize = {0};
            b = GetFileSizeEx(file, &fileSize);
            if ( !b || !fileSize.QuadPart )
            {
                s = GetLastError();
                EPrint("Getting file size failed! (0x%x)\n", s);
                break;
            }
            if ( fileSize.QuadPart > MAXUINT32 )
            {
                s = ERROR_INVALID_PARAMETER;
                EPrint("File too big! (0x%x)\n", s);
                break;
            }

            Params->InputBufferData = malloc(fileSize.LowPart);
            if ( !Params->InputBufferData )
            {
                s = ERROR_NOT_ENOUGH_MEMORY;
                EPrint("No memory for input data!\n");
                break;
            }
            Params->InputBufferSize = fileSize.LowPart;
            
            IO_STATUS_BLOCK iosb;
            RtlZeroMemory(&iosb, sizeof(iosb));
            
            s = NtReadFile(file, NULL, NULL, NULL, &iosb, Params->InputBufferData, Params->InputBufferSize, NULL, NULL);
            if ( s != 0 ) 
            {
                EPrint("Reading input data failed! (0x%x)\n", s);
                break;
            }
            if ( NT_SUCCESS(iosb.Status) )
                Params->InputBufferSize = (ULONG) iosb.Information;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'ir') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No random length set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            STR_TO_ULONG(Params->InputBufferSize, val1, s);
            Params->InputBufferData = malloc(Params->InputBufferSize);
            if ( !Params->InputBufferData )
            {
                s = ERROR_NOT_ENOUGH_MEMORY;
                EPrint("No memory for input data!\n");
                Params->InputBufferSize = 0;
                break;
            }

            s = genRand(Params->InputBufferData, Params->InputBufferSize);
            if ( s != 0 )
            {
                EPrint("Generating random failed!\n");
                break;
            }

            i++;
        }
        else if ( IS_2C_ARG(arg, 'ip') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No pattern length set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            STR_TO_ULONG(Params->InputBufferSize, val1, s);
            Params->InputBufferData = malloc(Params->InputBufferSize);
            if ( !Params->InputBufferData )
            {
                s = ERROR_NOT_ENOUGH_MEMORY;
                EPrint("No memory for input data!\n");
                Params->InputBufferSize = 0;
                break;
            }

            s = genPattern(Params->InputBufferData, Params->InputBufferSize);
            if ( s != 0 )
            {
                EPrint("Generating pattern failed!\n");
                break;
            }

            i++;
        }
        else if ( IS_3C_ARG(arg, 'ipc') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No pattern start value set!\n");
            
            val2 = GET_ARG_VALUE(argc, argv, i, 2);
            BREAK_ON_NOT_A_VALUE(val2, s, "[e] No pattern length set!\n");
            
            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            UINT64 patternStart = 0;
            PUINT8 patternStartPtr = (PUINT8)&patternStart;
            ULONG patternSize = 0;
            s = parsePlainBytes(val1, &patternStartPtr, &patternSize, 8);
            if ( s != 0 )
            {
                break;
            }

            STR_TO_ULONG(Params->InputBufferSize, val2, s);
            Params->InputBufferData = malloc(Params->InputBufferSize);
            if ( !Params->InputBufferData )
            {
                s = ERROR_NOT_ENOUGH_MEMORY;
                EPrint("No memory for input data!\n");
                Params->InputBufferSize = 0;
                break;
            }

            s = genCustomPattern(patternStart, patternSize, Params->InputBufferData, Params->InputBufferSize);
            if ( s != 0 )
            {
                EPrint("Generating pattern failed!\n");
                break;
            }

            i++;
            i++;
        }
        else if ( IS_2C_ARG(arg, 'is') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No input length set!\n");
            
            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            STR_TO_ULONG(Params->InputBufferSize, val1, s);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'da') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No desired access flag set!\n");

            STR_TO_ULONG(Params->DesiredAccess, val1, s);

            i++;
        }
        else if ( IS_1C_ARG(arg, 's') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No sleep length set!\n");

            STR_TO_ULONG(Params->Sleep, val1, s);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'sa') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No shareAccess flag set!\n");

            STR_TO_ULONG(Params->ShareAccess, val1, s);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'se') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No SE value set!\n");
            if ( seIdCount >= MAX_SE_COUNT )
            {
                printf("Maximum number of se values reached!");
                continue;
            }

            seId[seIdCount] = i+1;
            seIdCount++;

            i++;
        }
        else if ( IS_1C_ARG(arg, 't') )
        {
            Params->TestHandle = TRUE;
        }
        else if ( IS_2C_ARG(arg, 'pb') )
        {
            Params->Flags.PrintMode = PRINT_MODE_BYTES;
        }
        else if ( IS_3C_ARG(arg, 'pbs') )
        {
            Params->Flags.PrintMode = PRINT_MODE_BYTE_STR;
        }
        else if ( IS_3C_ARG(arg, 'pc8') )
        {
            Params->Flags.PrintMode = PRINT_MODE_COLS_8;
        }
        else if ( IS_4C_ARG(arg, 'pc16') )
        {
            Params->Flags.PrintMode = PRINT_MODE_COLS_16;
        }
        else if ( IS_4C_ARG(arg, 'pc32') )
        {
            Params->Flags.PrintMode = PRINT_MODE_COLS_32;
        }
        else if ( IS_4C_ARG(arg, 'pc64') )
        {
            Params->Flags.PrintMode = PRINT_MODE_COLS_64;
        }
        else if ( IS_3C_ARG(arg, 'pc1') )
        {
            Params->Flags.PrintMode = PRINT_MODE_COLS_BITS;
        }
        else if ( IS_2C_ARG(arg, 'pa') )
        {
            Params->Flags.PrintMode = PRINT_MODE_ASCII;
        }
        else if ( IS_2C_ARG(arg, 'pu') )
        {
            Params->Flags.PrintMode = PRINT_MODE_UNICODE;
        }
        else if ( IS_1C_ARG(arg, 'v') )
        {
            Params->Flags.Verbose = 1;
        }
        else
        {
            printf("[i] Unknown arg type \"%s\"\n", argv[i]);
        }
    }

    if ( s != 0 )
    {
        printf("\n");
        goto clean;
    }
    
    s = parseInts(argc, argv, ints, intCount, Params);
    if ( s != 0 )
    {
        goto clean;
    }
    
    s = parseSe(argc, argv, &Params->Se, seId, seIdCount);
    if ( s != 0 )
    {
        goto clean;
    }

    // fill input with a fill value, if just /is has been set
    if ( Params->InputBufferSize > 0 && !Params->InputBufferData )
    {
        Params->InputBufferData = malloc(Params->InputBufferSize);
        if ( !Params->InputBufferData )
        {
            s = GetLastError();
            printf("Error (0x%08x): inputBuffer malloc failed\n", s);
            goto clean;
        }
        memset(Params->InputBufferData, Params->FillValue, Params->InputBufferSize);
    }

clean:
    if ( ntPath )
        free(ntPath);
    if ( file )
        NtClose(file);

    return s==0;
}

// parse input ints to an input "struct"
int parseInts(_In_ int argc, _In_ char** argv, _In_ PINPUT_INT ints, _In_ INT intsCount, _Inout_ CmdParams* Params)
{
    int s = 0;
    INT i;

    // max is MAX_INTS * 8 = 0x80
    UINT32 neededSize = 0;

    if ( !intsCount )
        return 0;

    if ( Params->InputBufferData )
    {
        printf("[i] InputData already set! Skipping!\n");
        return 0;
    }

    // get needed size
    for ( i = 0; i < intsCount; i++ )
    {
        neededSize += ints[i].Size;
    }

    // allocate buffer
    Params->InputBufferSize = neededSize;
    Params->InputBufferData = malloc(Params->InputBufferSize);
    if ( !Params->InputBufferData )
    {
        s = GetLastError();
        EPrint("inputBuffer malloc failed! (0x%x)\n", s);
        goto clean;
    }

    // fill buffer
    PUINT8 ptr = Params->InputBufferData;
    for ( i = 0; i < intsCount; i++ )
    {
        // should not happen
        if ( ints[i].Id >= argc )
        {
            s = ERROR_INVALID_PARAMETER;
            EPrint("Invalid int id! (0x%x)\n", s);
            goto clean;
        }

        switch ( ints[i].Size )
        {
            case 1:
                *(PUINT8)ptr = (UINT8)strtoul(argv[ints[i].Id], NULL, 0);
                ptr++;
                break;
            case 2:
                *(PUINT16)ptr = (UINT16)strtoul(argv[ints[i].Id], NULL, 0);
                ptr+=2;
                break;
            case 4:
                *(PUINT32)ptr = (UINT32)strtoul(argv[ints[i].Id], NULL, 0);
                ptr+=4;
                break;
            case 8:
                *(PUINT64)ptr = (UINT64)strtoull(argv[ints[i].Id], NULL, 0);
                ptr+=8;
                break;
            default:
                s = ERROR_INVALID_PARAMETER;
                goto clean;
        }
    }

clean:

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

int openDevice(_Out_ PHANDLE Device, _In_ CHAR* DeviceNameA, _In_ ACCESS_MASK DesiredAccess, _In_ ULONG ShareAccess)
{
    INT s = 0;
    WCHAR deviceNameW[MAX_PATH];
    StringCchPrintfW(deviceNameW, MAX_PATH, L"%hs", DeviceNameA);
    deviceNameW[MAX_PATH-1] = 0;

    *Device = NULL;

    s = openFile(Device, deviceNameW, DesiredAccess, ShareAccess);

    return s;
}

int openFile(
    _Out_ PHANDLE File, 
    _In_ PWCHAR FileName, 
    _In_ ACCESS_MASK DesiredAccess, 
    _In_ ULONG ShareAccess
)
{
    NTSTATUS status = 0;
    OBJECT_ATTRIBUTES objAttr = { 0 };
    UNICODE_STRING fileNameUS;
    IO_STATUS_BLOCK iostatusblock = { 0 };

    RtlInitUnicodeString(&fileNameUS, FileName);
    objAttr.Length = sizeof(objAttr);
    objAttr.ObjectName = &fileNameUS;
    
    *File = NULL;

    status = NtCreateFile(
                File,
                DesiredAccess,
                &objAttr,
                &iostatusblock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                ShareAccess,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT,
                NULL, 
                0
            );
    if ( !NT_SUCCESS(status) )
    {
        EPrint("Open file failed! (0x%x) [%s].\n", status, getStatusString(status));
        return status;
    }

    return 0;
}

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
                break;
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

    return s;
}

BOOL checkArgs(_In_ CmdParams* Params)
{
    INT s = 0;
    if ( Params->DeviceName == NULL )
    {
        EPrint("No device name given!\n");
        s = -1;
    }
    
    if ( Params->Flags.PrintMode == PRINT_MODE_NONE
        || Params->Flags.PrintMode > PRINT_MODE_MAX )
    {
        Params->Flags.PrintMode = PRINT_MODE_COLS_8;
    }

    if ( s != 0 )
        printf("\n");

    return s == 0;
}

void printArgs(_In_ PCmdParams Params)
{
    printf("Params:\n");
    printf(" - DeviceName: %s\n", Params->DeviceName);
    printf(" - IOCTL: 0x%x\n", Params->IOCTL);
    printf(" - InputBufferSize: 0x%x\n", Params->InputBufferSize);
    if ( Params->InputBufferData )
    {
        printf(" - InputBufferData:\n");
        PrintMemBytes(Params->InputBufferData, Params->InputBufferSize);
    }
    printf(" - OutputBufferSize: 0x%x\n", Params->OutputBufferSize);
    printf(" - Sleep: 0x%x\n", Params->Sleep);
    printf(" - TestHandle: %d\n", Params->TestHandle);
    printf(" - DesiredAccess: 0x%x\n", Params->DesiredAccess);
    printf(" - ShareAccess: 0x%x\n", Params->ShareAccess);
    printf(" - FillValue: 0x%x\n", Params->FillValue);
    printf("\n");
}

void printVersion()
{
    printf("%s\n", BIN_NAME);
    printf("Version: %s\n", VERSION);
    printf("Last changed: %s\n", LAST_CHANGED);
    printf("Compiled: %s %s\n", __DATE__, __TIME__);
}
void printUsage()
{
    printf("Usage: %s "
           "/n <DeviceName> "
           "[/c <ioctl>] "
           "[/os <size>] "
           "[/is|/ir|/ip <size>] "
           "[/ipc <pattern> <size>] "
           "[/i(x|b|w|d|q|a|u) <data>] "
           "[/if <file>] "
           "[/s <sleep>] "
           "[/da <flags>] "
           "[/sa <flags>] "
           "[/se <priv>] "
           "[/t] "
           "[/v] "
           "[/pb|pbs|pc8|pc16|pc32|pc64|pc1|pa|pu] "
           "[/h]"
           "\n",
        BIN_NAME);
}

void printHelp()
{
    printVersion();
    printf("\n");
    printUsage();
    printf("\n");
    printf("Options:\n");
    printf(" - /n DeviceName to call. I.e. \"\\Device\\Beep\"\n");
    printf(" - /c The desired IOCTL in hex.\n");
    printf(" - /os Size of OutputBuffer.\n");
    printf(" - Input Data:\n");
    printf("    * /ix <Data> as hex byte string.\n");
    printf("    * /ib <Data> as byte.\n");
    printf("    * /iw <Data> as word (uint16).\n");
    printf("    * /id <Data> as dword (uint32).\n");
    printf("    * /iq <Data> as qword (uint64).\n");
    printf("    * /ia <Data> as ascii text.\n");
    printf("    * /iu <Data> as unicode (utf-16) text.\n");
    printf("    * /if Input data is read as binary data from the file <path>.\n");
    printf("    * /ir Input data will be filled with <size> random bytes.\n");
    printf("    * /ip Input data will be filled with <size> default pattern bytes (Aa0Aa1...).\n");
    printf("    * /ipc Input data will be filled with <size> custom pattern bytes, starting from <pattern>, incremented by 1.\n");
    printf("    * /is Input data will be filled with <size> 'A's.\n");
    printf(" - /s Duration of a possible sleep after device io.\n");
    printf(" - /t Just test the device for accessibility. Don't send data.\n");
    printf(" - /da DesiredAccess flags to open the device. Defaults to FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE = 0x%x.\n", (FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE));
    printf(" - /sa ShareAccess flags to open the device. Defaults to FILE_SHARE_READ|FILE_SHARE_WRITE = 0x%x.\n", (FILE_SHARE_READ|FILE_SHARE_WRITE));
    printf(" - /se Additional SE_XXX privilege (if run as admin). Can be set multiple (0x%x) times for multiple privileges.\n", MAX_SE_COUNT);
    printf(" - Printing style for output buffer:\n");
    printf("    * /pb Print in plain space separated bytes.\n");
    printf("    * /pbs Print in plain byte string.\n");
    printf("    * /pc8 Print in cols of Address | bytes | ascii chars.\n");
    printf("    * /pc16 Print in cols of Address | words | utf-16 chars.\n");
    printf("    * /pc32 Print in cols of Address | dwords.\n");
    printf("    * /pc64 Print in cols of Address | qwords.\n");
    printf("    * /pc1 Print in cols of Address | bits.\n");
    printf("    * /pa Print as ascii string.\n");
    printf("    * /pu Print as unicode (utf-16) string.\n");
    printf(" - /v More verbose output.\n");
    printf("\n");
    printf("Example:\n");
    printf("$ Talk.exe /n \\Device\\Beep /c 0x10000 /id 0x202 /id 0x83e /s 0x083e\n");
    printf("$ Talk.exe /n \\Device\\HarddiskVolume1 /c 0x4d0008 /os 0x100 /pu\n");
}

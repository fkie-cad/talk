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



#define BIN_NAME "Talk"
#define VERSION "2.1.1"
#define LAST_CHANGED "07.11.2023"


#define PRINT_MODE_BYTES    (0x01) // 001
#define PRINT_MODE_COLS_8   (0x02) // 010
#define PRINT_MODE_COLS_16  (0x03) // 011
#define PRINT_MODE_COLS_32  (0x04) // 100
#define PRINT_MODE_COLS_64  (0x05) // 101

#define PRINT_MODE_MAX  PRINT_MODE_COLS_64 

#define DEFAULT_FILL_VALUE ('A')


typedef struct CmdParams {
    CHAR* DeviceName;
    ULONG InputBufferSize;
    ULONG OutputBufferSize;
    PUINT8 InputBufferData;
    ULONG IOCTL;
    ULONG Sleep;
    ACCESS_MASK DesiredAccess;
    ULONG ShareAccess;
    struct {
        ULONG Verbose:1;
        ULONG PrintMode:3;
        ULONG Reserved:28;
    } Flags;
    BOOL TestHandle;
    CHAR FillValue;
} CmdParams, * PCmdParams;



BOOL parseArgs(INT argc, CHAR** argv, CmdParams* Params);
BOOL checkArgs(CmdParams* Params);
void printArgs(PCmdParams Params);

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
    
    ZeroMemory(&params, sizeof(CmdParams));
    params.DesiredAccess = FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE;
    params.ShareAccess = FILE_SHARE_READ|FILE_SHARE_WRITE;
    params.FillValue = DEFAULT_FILL_VALUE;

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


    s = openDevice(&device, params.DeviceName, params.DesiredAccess, params.ShareAccess);
    if ( s != 0 )
    {
        goto clean;
    }
    else if ( params.TestHandle )
    {
        printf("Device opened succcessfully: %p\n", device);
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
    
    HANDLE event;

    if ( Params->InputBufferSize > 0 && !Params->InputBufferData )
    {
        inputBuffer = malloc(Params->InputBufferSize);
        if ( !inputBuffer )
        {
            status = GetLastError();
            printf("Error (0x%08x): inputBuffer malloc failed\n", status);
            goto clean;
        }
        memset(inputBuffer, Params->FillValue, Params->InputBufferSize);
    }
    else if ( Params->InputBufferData )
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

    printf("Launching I/O Request Packet...\n");

    if ( inputBuffer && Params->Flags.Verbose )
    {
        printf(" - inputBuffer:\n");
        PrintMemCols8(inputBuffer, Params->InputBufferSize, 0);
        printf("\n");
    }

    status = NtCreateEvent(&event,
                           FILE_ALL_ACCESS,
                           0,
                           0,
                           0);
    if ( status != STATUS_SUCCESS )
    {
        printf("ERROR (0x%08x): Create event failed (%s).\n", status, getStatusString(status));
        goto clean;
    }

    RtlZeroMemory(&iosb, sizeof(iosb));

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

    if ( status == STATUS_PENDING )
    {
        status = NtWaitForSingleObject(event, 0, 0);
    }

    if ( !NT_SUCCESS(status) )
    {
        printf("ERROR (0x%08x): DeviceIo failed!\n"
               " %s.\n", 
               status, getStatusString(status));
        printf(" iosb info: 0x%08x\n", (ULONG)iosb.Information);
        goto clean;
    };

    if ( Params->Sleep )
    {
        if ( Params->Flags.Verbose )
            printf("Sleeping for 0x%x (%u) ms.\n", Params->Sleep, Params->Sleep);

        Sleep(Params->Sleep);
    }

    printf("\n");
    printf("Answer received.\n----------------\n");
    bytesReturned = (ULONG)iosb.Information;
    printf("The driver returned 0x%x bytes:\n", bytesReturned);
    if ( bytesReturned && bytesReturned <= Params->OutputBufferSize )
    {
// warning C6385: Reading invalid data from 'outputBuffer':  the readable size is 'Params->OutputBufferSize' bytes, but '2' bytes may be read ??
#pragma warning ( disable : 6385 )
        switch ( Params->Flags.PrintMode )
        {
            case PRINT_MODE_BYTES:
                PrintMemBytes(outputBuffer, bytesReturned);
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
            default:
                PrintMemCols8(outputBuffer, bytesReturned, 0);
                break;
        }
#pragma warning ( default : 6385 )
    }
    printf("\n----------------\n");


clean:
    if ( !Params->InputBufferData && inputBuffer )
        free(inputBuffer);

    if ( outputBuffer )
        free(outputBuffer);

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
        printf("[X] Exception parsing number! (0x%x)\n", __s__); \
        break; \
    } \
} \

#define PARSE_ID_NUMBER(__buffer__, __bufferSize__, __size__, __status__, __type__, __value__) \
{ \
    __buffer__ = malloc(__size__); \
    if ( !__buffer__ ) \
    { \
        __status__ = ERROR_NOT_ENOUGH_MEMORY; \
        printf("ERROR: No memory for input data!\n"); \
        break; \
    } \
     \
    __try { \
        *(__type__*)__buffer__ = (__type__)strtoull(__value__, NULL, 0); \
    } \
    __except ( EXCEPTION_EXECUTE_HANDLER ) \
    { \
        __status__ = GetExceptionCode(); \
        printf("[X] Exception parsing number! (0x%x)\n", (__status__)); \
        break; \
    } \
    __bufferSize__ = __size__; \
}

#define CONTINUE_IF_ID_SET(__id__, __i__) \
{ \
    if ( __id__ ) \
    { \
        printf("INFO: InputData already set! Skipping!\n"); \
        __i__++; \
        continue; \
    } \
}

BOOL parseArgs(INT argc, CHAR** argv, CmdParams* Params)
{
    INT start_i = 1;
    INT last_i = argc;
    INT i;
    INT s = 0;
    BOOL b;

    char* arg = NULL;
    char *val1 = NULL;
    
    ULONG cb;
    ULONG cch;

    PWCHAR ntPath = NULL;
    HANDLE file = NULL;

    for ( i = start_i; i < last_i; i++ )
    {
        arg = argv[i];
        val1 = GET_ARG_VALUE(argc, argv, i, 1);

        if ( !arg )
            break;

        if ( IS_1C_ARG(arg, 'n') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No name set!\n");

            Params->DeviceName = val1;
            i++;
        }
        else if ( IS_2C_ARG(arg, 'is') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No input length set!\n");

            if ( Params->InputBufferData )
            {
                printf("INFO: InputData size already set! Skipping!\n");
                i++;
                continue;
            }

            STR_TO_ULONG(Params->InputBufferSize, val1, s);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'os') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No output length set!\n");

            STR_TO_ULONG(Params->OutputBufferSize, val1, s);

            i++;
        }
        else if ( IS_1C_ARG(arg, 'c') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No ioctl code set!\n");
  
            STR_TO_ULONG(Params->IOCTL, val1, s);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'ix') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No hex data set!\n");

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
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No byte data set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);
    
            PARSE_ID_NUMBER(Params->InputBufferData, Params->InputBufferSize, 1, s, UINT8, val1);
            
            i++;
        }
        else if ( IS_2C_ARG(arg, 'iw') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No word data set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            PARSE_ID_NUMBER(Params->InputBufferData, Params->InputBufferSize, 2, s, UINT16, val1);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'id') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No dword data set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            PARSE_ID_NUMBER(Params->InputBufferData, Params->InputBufferSize, 4, s, UINT32, val1);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'iq') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No qword data set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            PARSE_ID_NUMBER(Params->InputBufferData, Params->InputBufferSize, 8, s, UINT64, val1);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'ia') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No ascii string data set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            cch = (ULONG)strlen(val1);
            cb = cch;
            Params->InputBufferSize = cb + 1;
            Params->InputBufferData = malloc(Params->InputBufferSize);
            if ( !Params->InputBufferData )
            {
                s = ERROR_NOT_ENOUGH_MEMORY;
                printf("ERROR: No memory for input data!\n");
                Params->InputBufferSize = 0;
                break;
            }
            memcpy(Params->InputBufferData, val1, Params->InputBufferSize);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'iu') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No unicode string data set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            cch = (ULONG)strlen(val1);
            cb = cch * 2;
            Params->InputBufferSize = cb + 2;
            Params->InputBufferData = malloc(Params->InputBufferSize);
            if ( !Params->InputBufferData )
            {
                s = ERROR_NOT_ENOUGH_MEMORY;
                printf("ERROR: No memory for input data!\n");
                Params->InputBufferSize = 0;
                break;
            }
            StringCchPrintfW((PWCHAR)Params->InputBufferData, cch+1, L"%hs", val1);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'if') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No file data set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            WCHAR val1W[MAX_PATH];
            StringCchPrintfW(val1W, MAX_PATH, L"%hs", val1);
            cch = GetFullPathNameW(val1W, 0, NULL, NULL);
            if ( cch == 0 )
            {
                s = GetLastError();
                printf("ERROR: Getting full path failed! (0x%x)\n", s);
                break;
            }
            cch += 4; // nt prefix
            cb = cch * 2;
            ntPath = (PWCHAR)malloc(cb);
            if ( !ntPath )
            {
                s = ERROR_NOT_ENOUGH_MEMORY;
                printf("ERROR: No memory for path!\n");
                break;
            }
            cch = GetFullPathNameW(val1W, cch-4, &ntPath[4], NULL);
            if ( cch == 0 )
            {
                s = GetLastError();
                printf("ERROR: Getting full path failed! (0x%x)\n", s);
                break;
            }
            *(PUINT64)ntPath = NT_PATH_PREFIX_W;

            s = openFile(&file, ntPath, FILE_GENERIC_READ|SYNCHRONIZE, FILE_SHARE_READ);
            if ( s != 0 )
            {
                break;
            }

            LARGE_INTEGER fileSize = {0};
            b = GetFileSizeEx(file, &fileSize);
            if ( !b || !fileSize.QuadPart )
            {
                s = GetLastError();
                printf("ERROR: Getting file size failed! (0x%x)\n", s);
                break;
            }
            if ( fileSize.QuadPart > MAXUINT32 )
            {
                s = ERROR_INVALID_PARAMETER;
                printf("ERROR: File too big! (0x%x)\n", s);
                break;
            }

            Params->InputBufferData = malloc(fileSize.LowPart);
            if ( !Params->InputBufferData )
            {
                s = ERROR_NOT_ENOUGH_MEMORY;
                printf("ERROR: No memory for input data!\n");
                break;
            }
            Params->InputBufferSize = fileSize.LowPart;
            
            IO_STATUS_BLOCK iosb;
            RtlZeroMemory(&iosb, sizeof(iosb));
            
            s = NtReadFile(file, NULL, NULL, NULL, &iosb, Params->InputBufferData, Params->InputBufferSize, NULL, NULL);
            if ( s != 0 ) 
            {
                printf("ERROR: Reading input data failed! (0x%x)\n", s);
                break;
            }
            if ( NT_SUCCESS(iosb.Status) )
                Params->InputBufferSize = (ULONG) iosb.Information;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'da') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No desired access flag set!\n");

            STR_TO_ULONG(Params->DesiredAccess, val1, s);

            i++;
        }
        else if ( IS_1C_ARG(arg, 's') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No sleep length set!\n");

            STR_TO_ULONG(Params->Sleep, val1, s);

            i++;
        }
        else if ( IS_2C_ARG(arg, 'sa') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No shareAccess flag set!\n");

            STR_TO_ULONG(Params->ShareAccess, val1, s);

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
        else if ( IS_1C_ARG(arg, 'v') )
        {
            Params->Flags.Verbose = 1;
        }
        else
        {
            printf("INFO: Unknown arg type \"%s\"\n", argv[i]);
        }
    }

    if ( s != 0 )
    {
        printf("\n");
    }

//clean:
    if ( ntPath )
        free(ntPath);
    if ( file )
        NtClose(file);

    return s==0;
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
        printf("ERROR: Open file failed! (0x%x) [%s].\n", status, getStatusString(status));
        return status;
    }

    return 0;
}

BOOL checkArgs(CmdParams* Params)
{
    INT s = 0;
    if ( Params->DeviceName == NULL )
    {
        printf("ERROR: No device name given!\n");
        s = -1;
    }
    
    if ( Params->Flags.PrintMode == 0 || Params->Flags.PrintMode > PRINT_MODE_MAX )
        Params->Flags.PrintMode = PRINT_MODE_COLS_8;

    if ( s != 0 )
        printf("\n");

    return s == 0;
}

void printArgs(PCmdParams Params)
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
    printf("Usage: %s /n <DeviceName> [/c <ioctl>] [/is <size>] [/os <size>] [/i(x|b|w|d|q|a|u|f) <data>] [/s <sleep>] [/da <flags>] [/sa <flags>] [/t] [/h]\n",
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
    printf(" - /c The desired IOCTL.\n");
    printf(" - /is Size of InputBuffer, if not filled with the data options. Will be filled with 'A's\n");
    printf(" - /os Size of OutputBuffer.\n");
    printf(" - Input Data:\n");
    printf("    * /ix Data as hex byte string.\n");
    printf("    * /ib Data as byte.\n");
    printf("    * /ib Data as word (uint16).\n");
    printf("    * /id Data as dword (uint32).\n");
    printf("    * /iq Data as qword (uint64).\n");
    printf("    * /ia Data as ascii text.\n");
    printf("    * /iu Data as unicode (utf-16) text.\n");
    printf("    * /if Data from binary file.\n");
    printf(" - /s Duration of a possible sleep after device io\n");
    printf(" - /t Just test the device for accessibility. Don't send data.\n");
    printf(" - /da DesiredAccess flags to open the device. Defaults to FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE = 0x%x.\n", (FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE));
    printf(" - /sa ShareAccess flags to open the device. Defaults to FILE_SHARE_READ|FILE_SHARE_WRITE = 0x%x.\n", (FILE_SHARE_READ|FILE_SHARE_WRITE));
    printf(" - Printing style for output buffer:\n");
    printf("    * /pb Print in plain bytes.\n");
    printf("    * /pc8 Print in cols of Address | bytes | ascii chars.\n");
    printf("    * /pc16 Print in cols of Address | words | utf-16 chars.\n");
    printf("    * /pc32 Print in cols of Address | dwords.\n");
    printf("    * /pc64 Print in cols of Address | qwords.\n");
    printf("\n");
    printf("Example:\n");
    printf("$ Talk.exe /n \\Device\\Beep /c 0x10000 /ix 020200003e080000 /s 0x083e\n");
}

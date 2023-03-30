#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_BUFFER_LN (0x100)

#include "ntstuff.h"
#include "warnings.h"
#include "Converter.h"
#include "Args.h"
#include "print.h"



#define VERSION "2.0.6"
#define LAST_CHANGED "30.03.2023"


#define FLAG_VERBOSE    (0x01)
#define FLAG_PRINT_B    (0x02)
#define FLAG_PRINT_W    (0x04)
#define FLAG_PRINT_DW   (0x08)
#define FLAG_PRINT_QW   (0x10)
#define FLAG_PRINT_COLS (0x20)


typedef struct CmdParams {
    CHAR* DeviceName;
    ULONG InputBufferSize;
    ULONG OutputBufferSize;
    BYTE* InputBufferData;
    ULONG IOCTL;
    ULONG Sleep;
    ACCESS_MASK DesiredAccess;
    ULONG ShareAccess;
    ULONG Flags;
    BOOL TestHandle;
} CmdParams, * PCmdParams;



BOOL parseArgs(INT argc, CHAR** argv, CmdParams* Params);
BOOL checkArgs(CmdParams* Params);
void printArgs(PCmdParams Params);

void printUsage();
void printHelp();

int openDevice(PHANDLE Device, CHAR* DeviceNameA, ACCESS_MASK DesiredAccess, ULONG ShareAccess);
int generateIoRequest(HANDLE Device, PCmdParams Params);



int _cdecl main(int argc, char** argv)
{
    HANDLE device = NULL;
    CmdParams params;
    BOOL s;


    if ( isAskForHelp(argc, argv) )
    {
        printHelp();
        return 0;
    }
    
    ZeroMemory(&params, sizeof(CmdParams));
    params.DesiredAccess = FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE;
    params.ShareAccess = FILE_SHARE_READ|FILE_SHARE_WRITE;

    if ( !parseArgs(argc, argv, &params) )
    {
        return -2;
    }

    if ( !checkArgs(&params) )
    {
        printUsage();
        return -1;
    }

    
    printArgs(&params);


    s = openDevice(&device, params.DeviceName, params.DesiredAccess, params.ShareAccess);
    if ( s != 0 )
        return -1;
    if ( params.TestHandle )
        goto clean;

    generateIoRequest(device, &params);

clean:
    if ( params.InputBufferData != NULL )
        free(params.InputBufferData);

    if ( device )
        NtClose(device);

    return 0;
}

int openDevice(PHANDLE Device, CHAR* DeviceNameA, ACCESS_MASK DesiredAccess, ULONG ShareAccess)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objAttr = { 0 };
    UNICODE_STRING deviceNameUS;
    IO_STATUS_BLOCK iostatusblock;

    WCHAR deviceNameW[MAX_PATH];
    swprintf(deviceNameW, MAX_PATH, L"%hs", DeviceNameA);
    deviceNameW[MAX_PATH-1] = 0;

    RtlInitUnicodeString(&deviceNameUS, deviceNameW);
    objAttr.Length = sizeof( objAttr );
    objAttr.ObjectName = &deviceNameUS;
    
    *Device = NULL;

    status = NtCreateFile(
                Device,
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
        printf("Error (0x%08x): Open device failed: %s.\n", status, getStatusString(status));
        return -1;
    }

    printf("device: %p\n", *Device);
    printf("\n");

    return 0;
}

int generateIoRequest(HANDLE Device, PCmdParams Params)
{
    ULONG bytesReturned = 0;
    SIZE_T i = 0;
    NTSTATUS status = 0;
    IO_STATUS_BLOCK iosb;

    BYTE* inputBuffer = NULL;
    BYTE* outputBuffer = NULL;
    
    HANDLE event;

    if ( Params->InputBufferSize > 0 )
    {
        inputBuffer = malloc(Params->InputBufferSize);
        if ( !inputBuffer )
        {
            status = GetLastError();
            printf("Error (0x%08x): inputBuffer calloc failed\n", status);
            goto clean;
        }
    }
    if ( Params->OutputBufferSize > 0 )
    {
        outputBuffer = malloc(Params->OutputBufferSize);
        if ( !outputBuffer )
        {
            status = GetLastError();
            printf("Error (0x%08x): outputBuffer calloc failed\n", status);
            goto clean;
        }
    }


    printf("Launching I/O Request Packet...\n");
    
    if ( Params->InputBufferData )
        memcpy(inputBuffer, Params->InputBufferData, Params->InputBufferSize);
    else
        memset(inputBuffer, 'A', Params->InputBufferSize); // Initialize with 'A'.
    memset(outputBuffer, 0, Params->OutputBufferSize); // Potential stuff returned by driver goes here.
    
    if ( inputBuffer )
    {
        printf(" - inputBuffer: ");
        for ( i = 0; i < Params->InputBufferSize; i++ ) printf("%02x ", inputBuffer[i]);
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

    status = NtDeviceIoControlFile(Device, 
                                   event, 
                                   NULL, 
                                   NULL, 
                                   &iosb, 
                                   Params->IOCTL, 
                                   inputBuffer, 
                                   Params->InputBufferSize, 
                                   outputBuffer,
                                   Params->OutputBufferSize);
    
    if ( status == STATUS_PENDING )
    {
        status = NtWaitForSingleObject(event, 0, 0);
    }

    if ( !NT_SUCCESS(status) )
    {
        fprintf(stderr,"ERROR (0x%08x): Sorry, the driver is present but does not want to answer (%s).\n", status, getStatusString(status));
        fprintf(stderr," iosb info: 0x%08x\n", (ULONG) iosb.Information);
        goto clean;
    };

    if ( Params->Sleep )
	    Sleep(Params->Sleep);
    
    printf("\n");
    printf("Answer received.\n----------------\n");
    bytesReturned = (ULONG) iosb.Information;
    printf("The driver returned 0x%x bytes:\n", bytesReturned);
    if ( bytesReturned && bytesReturned <= Params->OutputBufferSize )
    {
// warning C6385: Reading invalid data from 'outputBuffer':  the readable size is 'Params->OutputBufferSize' bytes, but '2' bytes may be read ??
#pragma warning ( disable : 6385 )
        if ( Params->Flags & FLAG_PRINT_B )
        {
            PrintMemBytes(outputBuffer, bytesReturned);
        }
        else if ( Params->Flags & FLAG_PRINT_COLS )
        {
            PrintMemCols(outputBuffer, bytesReturned);
        }
#pragma warning ( default : 6385 )
    }
    printf("\n----------------\n");



clean:
    if ( inputBuffer )
        free(inputBuffer);

    if ( outputBuffer )
        free(outputBuffer);

    return status;
}



BOOL parseArgs(INT argc, CHAR** argv, CmdParams* Params)
{
    INT start_i = 1;
    INT last_i = argc;
    INT i;
    BOOL error = FALSE;
    INT s = 0;

    char* arg;
    char *val0;

    // defaults

    for ( i = start_i; i < last_i; i++ )
    {
        arg = argv[i];
        val0 = ( i < argc - 1 ) ? argv[i+1] : NULL;

        if ( !arg )
            break;

        if ( IS_1C_ARG(arg, 'n') )
        {
            if ( NOT_A_VALUE(val0) )
            {
                printf("ERROR: No name set!\n");
                s = -1;
                break;
            }

            Params->DeviceName = val0;
            i++;
        }
        else if ( IS_1C_ARG(arg, 'i') )
        {
            if ( NOT_A_VALUE(val0) )
            {
                printf("ERROR: No input length set!\n");
                s = -1;
                break;
            }
            __try {
                Params->InputBufferSize = (ULONG)strtoul(argv[i + 1], NULL, 0);
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                printf("[X] Exception in %s [%s line %d]\n", __FUNCTION__, __FILE__, __LINE__);
                error = TRUE;
                break;
            }
            i++;
        }
        else if ( IS_1C_ARG(arg, 'o') )
        {
            if ( NOT_A_VALUE(val0) )
            {
                printf("ERROR: No output length set!\n");
                s = -1;
                break;
            }
            __try {
                Params->OutputBufferSize = (ULONG)strtoul(argv[i + 1], NULL, 0);
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                printf("[X] Exception in %s [%s line %d]\n", __FUNCTION__, __FILE__, __LINE__);
                error = TRUE;
                break;
            }

            i++;
        }
        else if ( IS_1C_ARG(arg, 'c') )
        {
            if ( NOT_A_VALUE(val0) )
            {
                printf("ERROR: No ioctl code set!\n");
                s = -1;
                break;
            }
            __try {
                Params->IOCTL = (ULONG)strtoul(argv[i + 1], NULL, 0);
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                printf("[X] Exception in %s [%s line %d]\n", __FUNCTION__, __FILE__, __LINE__);
                error = TRUE;
                break;
            }

            i++;
        }
        else if ( IS_1C_ARG(arg, 'd') )
        {
            if ( NOT_A_VALUE(val0) )
            {
                printf("ERROR: No byte data set!\n");
                s = -1;
                break;
            }
            if ( parsePlainBytes(argv[i+1], &Params->InputBufferData, Params->InputBufferSize) == 0 )
            {
                error = TRUE;
                break;
            }
                    
            i++;
        }
        else if ( IS_2C_ARG(arg, 'da') )
        {
            if ( NOT_A_VALUE(val0) )
            {
                printf("ERROR: desired access flag set!\n");
                s = -1;
                break;
            }
            __try {
                Params->DesiredAccess = (ULONG)strtoul(argv[i + 1], NULL, 0);
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                printf("[X] Exception in %s [%s line %d]\n", __FUNCTION__, __FILE__, __LINE__);
                error = TRUE;
                break;
            }

            i++;
        }
        else if ( IS_1C_ARG(arg, 's') )
        {
            if ( NOT_A_VALUE(val0) )
            {
                printf("ERROR: No sleep length set!\n");
                s = -1;
                break;
            }
            __try {
                Params->Sleep = (ULONG)strtoul(argv[i + 1], NULL, 0);
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                printf("[X] Exception in %s [%s line %d]\n", __FUNCTION__, __FILE__, __LINE__);
                error = TRUE;
                break;
            }

            i++;
        }
        else if ( IS_2C_ARG(arg, 'sa') )
        {
            if ( NOT_A_VALUE(val0) )
            {
                printf("ERROR: No shareAccess flag set!\n");
                s = -1;
                break;
            }
            __try {
                Params->ShareAccess = (ULONG)strtoul(argv[i + 1], NULL, 0);
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                printf("[X] Exception in %s [%s line %d]\n", __FUNCTION__, __FILE__, __LINE__);
                error = TRUE;
                break;
            }

            i++;
        }
        else if ( IS_1C_ARG(arg, 't') )
        {
            Params->TestHandle = TRUE;
        }
        else if ( IS_2C_ARG(arg, 'pb') )
        {
            Params->Flags |= FLAG_PRINT_B;
        }
        else if ( IS_2C_ARG(arg, 'pc') )
        {
            Params->Flags |= FLAG_PRINT_COLS;
        }
        //else if ( IS_2C_ARG(arg, 'pw') )
        //{
        //    Params->Flags |= FLAG_PRINT_W;
        //}
        //else if ( IS_2C_ARG(arg, 'pd') )
        //{
        //    Params->Flags |= FLAG_PRINT_DW;
        //}
        //else if ( IS_2C_ARG(arg, 'pq') )
        //{
        //    Params->Flags |= FLAG_PRINT_QW;
        //}
        else
        {
            printf("INFO: Unknown arg type \"%s\"\n", argv[i]);
        }
    }

    return !error;
}

BOOL checkArgs(CmdParams* Params)
{
    INT s = 0;
    if ( Params->DeviceName == NULL )
    {
        printf("ERROR: No device name given!\n");
        s = -1;
    }
    
    ULONG f = Params->Flags&(FLAG_PRINT_B|FLAG_PRINT_W|FLAG_PRINT_DW|FLAG_PRINT_QW|FLAG_PRINT_COLS);
    if ( ( (f & (f-1)) != 0 ) )
    {
        printf("ERROR: No valid printing mode!\n");
        s = -1;
    }
    if ( f == 0 )
        Params->Flags |= FLAG_PRINT_COLS;

    return s == 0;
}

void printArgs(PCmdParams Params)
{
    SIZE_T i;

    printf("Params:\n");
    printf(" - DeviceName: %s\n", Params->DeviceName);
    printf(" - InputBufferSize: 0x%x\n", Params->InputBufferSize);
    printf(" - OutputBufferSize: 0x%x\n", Params->OutputBufferSize);
    printf(" - IOCTL: 0x%x\n", Params->IOCTL);
    if ( Params->InputBufferData )
    {
        printf(" - InputBufferData: ");
        for ( i = 0; i < Params->InputBufferSize; i++ ) printf("%02x ", Params->InputBufferData[i]);
        printf("\n");
    }
    printf(" - Sleep: 0x%x\n", Params->Sleep);
    printf(" - TestHandle: %d\n", Params->TestHandle);
    printf("\n");
}

void printUsage()
{
    printf("Usage: Talk.exe /n <DeviceName> [/c <ioctl>] [/i <InputBufferSize>] [/o <OutputBufferSize>] [/d <aabbcc>] [/s <SleepDuration>] [/da <Flags>] [/sa <Flags>] [/t] [/h]\n");
    printf("Version: %s\n", VERSION);
    printf("Last changed: %s\n", LAST_CHANGED);
    printf("Compiled: %s %s\n", __DATE__, __TIME__);
}

void printHelp()
{
    printUsage();
    printf("\n");
    printf("Options:\n");
    printf(" - /n DeviceName to call. I.e. \"\\Device\\Beep\"\n");
    printf(" - /c The desired IOCTL.\n");
    printf(" - /i InputBufferSize possible size of InputBuffer.\n");
    printf(" - /o OutputBufferSize possible size of OutputBuffer.\n");
    printf(" - /d InputBuffer data in hex.\n");
    printf(" - /s Duration of sleep after call\n");
    printf(" - /t Just test the device for accessibility. Don't send data.\n");
    printf(" - /da DesiredAccess flags to open the device. Defaults to FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE = 0x%x.\n", (FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE));
    printf(" - /sa ShareAccess flags to open the device. Defaults to FILE_SHARE_READ|FILE_SHARE_WRITE = 0x%x.\n", (FILE_SHARE_READ|FILE_SHARE_WRITE));
    printf(" - /p* Printig style for output buffer.\n");
    printf("    * /pb Print in plain bytes.\n");
    printf("    * /pc Print in cols of Address | bytes | ascii.\n");
}

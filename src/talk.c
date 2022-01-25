#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_BUFFER_LN (0x100)

#include "ntstuff.h"
#include "Converter.h"
#include "Args.h"



#define VERSION "2.0.4"
#define LAST_CHANGED "11.01.2022"



typedef struct CmdParams {
    CHAR* DeviceName;
    ULONG InputBufferSize;
    ULONG OutputBufferSize;
    BYTE* InputBufferData;
    ULONG IOCTL;
    ULONG Sleep;
    BOOL TestHandle;
    ACCESS_MASK DesiredAccess;
    ULONG ShareAccess;
} CmdParams, * PCmdParams;



BOOL parseArgs(INT argc, CHAR** argv, CmdParams* params);
BOOL checkArgs(CmdParams* params);
void printArgs(PCmdParams params);

void printUsage();
void printHelp();

int openDevice(PHANDLE device, CHAR* name, ACCESS_MASK DesiredAccess, ULONG ShareAccess);
int generateIoRequest(HANDLE device, PCmdParams params);



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
        return 1;
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

int openDevice(PHANDLE device, CHAR* name, ACCESS_MASK DesiredAccess, ULONG ShareAccess)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objAttr = { 0 };
    UNICODE_STRING devname;
    IO_STATUS_BLOCK iostatusblock;

    WCHAR dev_name[MAX_PATH];
    swprintf(dev_name, MAX_PATH, L"\\Device\\%hs", name);
    dev_name[MAX_PATH-1] = 0;

    RtlInitUnicodeString(&devname, dev_name);
    objAttr.Length = sizeof( objAttr );
    objAttr.ObjectName = &devname;

    //status = NtCreateFile(device, 
    //                    FILE_GENERIC_READ|FILE_GENERIC_READ|SYNCHRONIZE, 
    //                    &objAttr, 
    //                    &iostatusblock,
    //                    0, 
    //                    FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);
    
    status = NtCreateFile(
                device,
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

    printf("device: %p\n", *device);
    printf("\n");

    return 0;
}

int generateIoRequest(HANDLE device, PCmdParams params)
{
    ULONG bytesReturned = 0;
    SIZE_T i = 0;
    INT s = 0;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;

    BYTE* InputBuffer = NULL;
    BYTE* OutputBuffer = NULL;
    
    HANDLE event;

    if ( params->InputBufferSize > 0 )
    {
        InputBuffer = malloc(params->InputBufferSize * 1);
        if ( !InputBuffer )
        {
            printf("Error (0x%08x): InputBuffer calloc failed\n", GetLastError());
            s = -2;
            goto clean;
        }
    }
    if ( params->OutputBufferSize > 0 )
    {
        OutputBuffer = malloc(params->OutputBufferSize * 1);
        if ( !OutputBuffer )
        {
            printf("Error (0x%08x): OutputBuffer calloc failed\n", GetLastError());
            s = -2;
            goto clean;
        }
    }


    printf("Launching I/O Request Packet...\n");
    
    if ( params->InputBufferData )
        memcpy(InputBuffer, params->InputBufferData, params->InputBufferSize);
    else
        memset(InputBuffer, 'A', params->InputBufferSize); // Initialize with 'A'.
    memset(OutputBuffer, 0, params->OutputBufferSize); // Potential stuff returned by driver goes here.
    
    if ( InputBuffer )
    {
        printf(" - InputBuffer: ");
        for ( i = 0; i < params->InputBufferSize; i++ ) printf("%02x ", InputBuffer[i]);
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
        s = -1;
        goto clean;
    }

    RtlZeroMemory(&iosb, sizeof(iosb));

    status = NtDeviceIoControlFile(device, 
                                   event, 
                                   NULL, 
                                   NULL, 
                                   &iosb, 
                                   params->IOCTL, 
                                   InputBuffer, 
                                   params->InputBufferSize, 
                                   OutputBuffer,
                                   params->OutputBufferSize);
    
    if ( status == STATUS_PENDING )
    {
        status = NtWaitForSingleObject(event, 0, 0);
    }

    if ( !NT_SUCCESS(status) )
    {
        fprintf(stderr,"ERROR (0x%08x): Sorry, the driver is present but does not want to answer (%s).\n", status, getStatusString(status));
        fprintf(stderr," iosb info: 0x%08x\n", (ULONG) iosb.Information);
        s = -3;
        goto clean;
    };

    if ( params->Sleep )
	    Sleep(params->Sleep);
    
    printf("\n");
    printf("Answer received.\n----------------\n");
    bytesReturned = (ULONG) iosb.Information;
    printf("The driver returned %d bytes:\n",bytesReturned);
    if ( bytesReturned )
    {
        for (i=0;i<bytesReturned;i++)
        {
            printf("%02x ",OutputBuffer[i]);
        }
        printf("\n");
    }
    printf("\n----------------\n");



clean:
    if ( InputBuffer )
        free(InputBuffer);

    if ( OutputBuffer )
        free(OutputBuffer);

    return s;
}

BOOL parseArgs(INT argc, CHAR** argv, CmdParams* params)
{
    INT start_i = 0;
    INT last_i = argc;
    INT i;
    BOOL error = FALSE;

    // defaults

    for ( i = start_i; i < last_i; i++ )
    {
        if ( argv[i][0] != PARAM_IDENTIFIER )
             continue;

        if ( isArgOfType(argv[i], "n") )
        {
            if ( hasValue("n", i, last_i) )
            {
                params->DeviceName = argv[i+1];
                i++;
            }
        }
        else if ( isArgOfType(argv[i], "i") )
        {
            if ( hasValue("i", i, last_i) )
            {
                __try {
                    params->InputBufferSize = (ULONG)strtoul(argv[i + 1], NULL, 0);
                }
                __except ( EXCEPTION_EXECUTE_HANDLER )
                {
                    printf("[X] Exception in %s [%s line %d]\n", __FUNCTION__, __FILE__, __LINE__);
                    error = TRUE;
                    break;
                }
                i++;
            }
        }
        else if ( isArgOfType(argv[i], "o") )
        {
            if ( hasValue("o", i, last_i) )
            {
                __try {
                    params->OutputBufferSize = (ULONG)strtoul(argv[i + 1], NULL, 0);
                }
                __except ( EXCEPTION_EXECUTE_HANDLER )
                {
                    printf("[X] Exception in %s [%s line %d]\n", __FUNCTION__, __FILE__, __LINE__);
                    error = TRUE;
                    break;
                }

                i++;
            }
        }
        else if ( isArgOfType(argv[i], "c") )
        {
            if ( hasValue("c", i, last_i) )
            {
                __try {
                    params->IOCTL = (ULONG)strtoul(argv[i + 1], NULL, 0);
                }
                __except ( EXCEPTION_EXECUTE_HANDLER )
                {
                    printf("[X] Exception in %s [%s line %d]\n", __FUNCTION__, __FILE__, __LINE__);
                    error = TRUE;
                    break;
                }

                i++;
            }
        }
        else if ( isArgOfType(argv[i], "d") )
        {
            if ( hasValue("d", i, last_i) )
            {
                if ( parsePlainBytes(argv[i+1], &params->InputBufferData, params->InputBufferSize) == 0 )
                {
                    error = TRUE;
                    break;
                }
                    
                i++;
            }
        }
        else if ( isArgOfType(argv[i], "da") )
        {
            if ( hasValue("da", i, last_i) )
            {
                __try {
                    params->DesiredAccess = (ULONG)strtoul(argv[i + 1], NULL, 0);
                }
                __except ( EXCEPTION_EXECUTE_HANDLER )
                {
                    printf("[X] Exception in %s [%s line %d]\n", __FUNCTION__, __FILE__, __LINE__);
                    error = TRUE;
                    break;
                }

                i++;
            }
        }
        else if ( isArgOfType(argv[i], "s") )
        {
            if ( hasValue("s", i, last_i) )
            {
                __try {
                    params->Sleep = (ULONG)strtoul(argv[i + 1], NULL, 0);
                }
                __except ( EXCEPTION_EXECUTE_HANDLER )
                {
                    printf("[X] Exception in %s [%s line %d]\n", __FUNCTION__, __FILE__, __LINE__);
                    error = TRUE;
                    break;
                }

                i++;
            }
        }
        else if ( isArgOfType(argv[i], "sa") )
        {
            if ( hasValue("sa", i, last_i) )
            {
                __try {
                    params->ShareAccess = (ULONG)strtoul(argv[i + 1], NULL, 0);
                }
                __except ( EXCEPTION_EXECUTE_HANDLER )
                {
                    printf("[X] Exception in %s [%s line %d]\n", __FUNCTION__, __FILE__, __LINE__);
                    error = TRUE;
                    break;
                }

                i++;
            }
        }
        else if ( isArgOfType(argv[i], "t") )
        {
            params->TestHandle = TRUE;
        }
        else
        {
            printf("INFO: Unknown arg type \"%s\"\n", argv[i]);
        }
    }

    return !error;
}

BOOL checkArgs(CmdParams* params)
{
    if ( params->DeviceName == NULL )
        return FALSE;
    
    return TRUE;
}

void printArgs(PCmdParams params)
{
    SIZE_T i;

    printf("params:\n");
    printf(" - DeviceName: %s\n", params->DeviceName);
    printf(" - InputBufferSize: 0x%x\n", params->InputBufferSize);
    printf(" - OutputBufferSize: 0x%x\n", params->OutputBufferSize);
    printf(" - IOCTL: 0x%x\n", params->IOCTL);
    if ( params->InputBufferData )
    {
        printf(" - InputBufferData: ");
        for ( i = 0; i < params->InputBufferSize; i++ ) printf("%02x ", params->InputBufferData[i]);
        printf("\n");
    }
    printf(" - Sleep: 0x%x\n", params->Sleep);
    printf(" - TestHandle: %u\n", params->TestHandle);
    printf("\n");
}

int isCallForHelp(char * arg1)
{
    return arg1[0] == '/' && (arg1[1] == 'h' || arg1[1] == '?' ) && arg1[2] == 0;
}

void printUsage()
{
    printf("Usage: Talk.exe /n DeviceName [/c ioctl] [/i InputBufferSize] [/o OutputBufferSize] [/d aabbcc] [/s SleepDuration] [/t] [/h]\n");
    printf("Version: %s\n", VERSION);
    printf("Last changed: %s\n", LAST_CHANGED);
    printf("Compiled: %s %s\n", __DATE__, __TIME__);
}

void printHelp()
{
    printUsage();
    printf("\n");
    printf("Options:\n");
    printf(" - /n DeviceName to call.\n");
    printf(" - /c The desired IOCTL.\n");
    printf(" - /i InputBufferSize possible size of InputBuffer.\n");
    printf(" - /o OutputBufferSize possible size of OutputBuffer.\n");
    printf(" - /d InputBuffer data in hex.\n");
    printf(" - /s Duration of sleep after call\n");
    printf(" - /t Just test the device for accessibility. Don't send data.\n");
    printf(" - /sa DesiredAccess flags to open the device. Defaults to FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE = 0x%x.\n", (FILE_GENERIC_READ|FILE_GENERIC_WRITE|SYNCHRONIZE));
    printf(" - /sa ShareAccess flags to open the device. Defaults to FILE_SHARE_READ|FILE_SHARE_WRITE = 0x%x.\n", (FILE_SHARE_READ|FILE_SHARE_WRITE));
}

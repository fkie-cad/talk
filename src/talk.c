#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <stdlib.h>

#include "talk.h"


#define PARAM_IDENTIFIER '/'
#define MAX_BUFFER_LN (0x100)

#define VERSION "2.0.1"
#define LAST_CHANGED "04.02.2021"



typedef struct CmdParams {
    CHAR* DeviceName;
    ULONG InputBufferSize;
    ULONG OutputBufferSize;
    BYTE* InputBufferData;
    ULONG IOCTL;
    ULONG Sleep;
    BOOL TestHandle;
} CmdParams, * PCmdParams;



BOOL parseArgs(INT argc, CHAR** argv, CmdParams* params);
BOOL checkArgs(CmdParams* params);
BOOL isAskForHelp(INT argc, CHAR** argv);
BOOL isArgOfType(const CHAR* arg, const CHAR* type);
BOOL hasValue(const char* type, int i, int end_i);

void printUsage();
void printHelp();

int openDevice(PHANDLE device, CHAR* name);
int generateIoRequest(HANDLE device, PCmdParams params);
void printStatus(NTSTATUS status);

ULONG parsePlainBytes(const char* raw, BYTE** buffer, ULONG size);
int parseUint8(const char* arg, BYTE* value, BYTE base);
int parseUint64(const char* arg, ULONGLONG* value, BYTE base);

int isCallForHelp(char * arg1)
{
    return arg1[0] == '/' && (arg1[1] == 'h' || arg1[1] == '?' ) && arg1[2] == 0;
}

void printUsage()
{
    printf("Usage: Talk.exe /n DeviceName [/c ioctl] [/i InputBufferSize] [/o OutputBufferSize] [/d aabbcc] [/s SleepDuration] [/t] [/h]\n");
    printf("Version: %s\n", VERSION);
    printf("Last changed: %s\n", LAST_CHANGED);
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
}

int _cdecl main(int argc, char** argv)
{
    HANDLE device = INVALID_HANDLE_VALUE;
    CmdParams params;
    ULONG i;
    BOOL s;

    if ( isAskForHelp(argc, argv) )
    {
        printHelp();
        return 0;
    }

    if ( !parseArgs(argc, argv, &params) )
    {
        return -2;
    }

    if ( !checkArgs(&params) )
    {
        printUsage();
        return 1;
    }

    
    printf("params:\n");
    printf(" - DeviceName: %s\n", params.DeviceName);
    printf(" - InputBufferSize: 0x%x\n", params.InputBufferSize);
    printf(" - OutputBufferSize: 0x%x\n", params.OutputBufferSize);
    printf(" - IOCTL: 0x%x\n", params.IOCTL);
    if ( params.InputBufferData )
    {
        printf(" - InputBufferData: ");
        for ( i = 0; i < params.InputBufferSize; i++ ) printf("%02x ", params.InputBufferData[i]);
        printf("\n");
    }
    printf(" - Sleep: 0x%x\n", params.Sleep);
    printf(" - TestHandle: %u\n", params.TestHandle);
    printf("\n");

    s = openDevice(&device, params.DeviceName);
    if ( s != 0 )
        return -1;
    if ( params.TestHandle )
        goto clean;

    generateIoRequest(device, &params);

clean:
    if ( params.InputBufferData != NULL )
        free(params.InputBufferData);

    CloseHandle(device);

    return 0;
}

int openDevice(PHANDLE device, CHAR* name)
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

    status = NtOpenFile(device, 
                        FILE_GENERIC_READ, 
                        &objAttr, 
                        &iostatusblock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, 
                        3);

    if ( !NT_SUCCESS(status) )
    {
        printf("Error (0x%08x): nobody here.\n", status);
        printStatus(status);
        return -1;
    }

    printf("device: 0x%p\n", *device);
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
        printf("ERROR (0x%08x): Create event failed.\n", status);
        printStatus(status);
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
        fprintf(stderr,"ERROR (0x%08x): Sorry, the driver is present but does not want to answer.\n", status);
        printStatus(status);
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

void printStatus(NTSTATUS status)
{
    if (status == STATUS_ACCESS_DENIED)
        printf("STATUS_ACCESS_DENIED.(\n");
    else if (status == STATUS_INVALID_PARAMETER)
        printf("STATUS_INVALID_PARAMETER.\n");
    else if (status == STATUS_NO_SUCH_FILE)
        printf("STATUS_NO_SUCH_FILE.\n");
    else if (status == STATUS_INVALID_DEVICE_REQUEST)
        printf("STATUS_INVALID_DEVICE_REQUEST: The specified request is not a valid operation for the target device.\n");
    else if (status == STATUS_ILLEGAL_FUNCTION)
        printf("STATUS_ILLEGAL_FUNCTION: kernel driver is irritated.\n");
    else if (status == STATUS_INVALID_HANDLE)
        printf("STATUS_INVALID_HANDLE.\n");
    else if (status == STATUS_DATATYPE_MISALIGNMENT_ERROR)
        printf("STATUS_DATATYPE_MISALIGNMENT_ERROR: A data type misalignment error was detected in a load or store instruction.\n");
    else if (status == STATUS_NOT_SUPPORTED)
        printf("STATUS_NOT_SUPPORTED: The request is not supported.\n");
    else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        printf("STATUS_OBJECT_NAME_NOT_FOUND.\n");
}

BOOL parseArgs(INT argc, CHAR** argv, CmdParams* params)
{
    INT start_i = 0;
    INT last_i = argc;
    INT i;
    BOOL error = FALSE;

    // defaults
    memset(params, 0, sizeof(CmdParams));

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

BOOL isAskForHelp(INT argc, CHAR** argv)
{
    int i = 1;
    if ( argc < i+1 )
        return FALSE;

    return isArgOfType(argv[i], "h") || isArgOfType(argv[i], "?");
}

BOOL isArgOfType(const CHAR* arg, const CHAR* type)
{
    size_t type_ln;

    type_ln = strlen(type);

    return arg[0] == PARAM_IDENTIFIER && 
        strnlen(&arg[1], 10) == type_ln && 
        strncmp(&arg[1], type, type_ln) == 0;
}

BOOL checkArgs(CmdParams* params)
{
    if ( params->DeviceName == NULL )
        return FALSE;
    
    return TRUE;
}

BOOL hasValue(const char* type, int i, int last_i)
{
    if ( i >= last_i )
    {
        printf("INFO: Arg \"%c%s\" has no value! Skipped!\n", PARAM_IDENTIFIER, type);
        return FALSE;
    }

    return TRUE;
}

ULONG parsePlainBytes(const char* raw, BYTE** buffer, ULONG size)
{
	ULONG i, j;
	SIZE_T arg_ln = strnlen(raw, MAX_BUFFER_LN);
	BYTE* p;
	char byte[3] = {0};
	ULONG buffer_ln;
	int s = 0;
    
	buffer_ln = (ULONG) (arg_ln / 2);

	if ( arg_ln % 2 != 0 || arg_ln == 0 )
	{
		printf("Error: Buffer data is not byte aligned!\n");
		return 0;
	}

	if ( buffer_ln != size )
	{
		printf("Error: Buffer data has the wrong size %zu != %u!\n", arg_ln, size);
		return 0;
	}

	p = (BYTE*) malloc(arg_ln/2);
	if ( p == NULL )
	{
		printf("ERROR (0x%08x): Allocating memory failed!\n", GetLastError());
		return 0;
	}

	for ( i = 0, j = 0; i < arg_ln; i += 2 )
	{
		byte[0] = raw[i];
		byte[1] = raw[i + 1];

		 s = parseUint8(byte, &p[j++], 16);
		 if ( s != 0 )
		 {
		 	free(p);
		 	return 0;
		 }
	}
    
	*buffer = p;
	return buffer_ln;
}

int parseUint8(const char* arg, BYTE* value, BYTE base)
{
	ULONGLONG result;
	int s = parseUint64(arg, &result, base);
	if ( s != 0 ) return s;
	if ( s > (BYTE)-1 )
	{
		fprintf(stderr, "Error: %s could not be converted to a byte: Out of range!\n", arg);
		return 5;
	}

	*value = (BYTE) result;
	return 0;
}

int parseUint64(const char* arg, ULONGLONG* value, BYTE base)
{
	char* endptr;
	int err_no = 0;
	errno = 0;
	ULONGLONG result;

	if ( base != 10 && base != 16 && base != 0 )
	{
		fprintf(stderr, "Error: Unsupported base %u!\n", base);
		return 1;
	}

	if ( arg[0] ==  '-' )
	{
		fprintf(stderr, "Error: %s could not be converted to a number: is negative!\n", arg);
		return 2;
	}

#if defined(_WIN32)
	result = strtoull(arg, &endptr, base);
#else
	result = strtoul(arg, &endptr, base);
#endif
	err_no = errno;

	if ( endptr == arg )
	{
		fprintf(stderr, "Error: %s could not be converted to a number: Not a number!\n", arg);
		return 3;
	}
	if ( result == (ULONGLONG)-1 && err_no == ERANGE )
	{
		fprintf(stderr, "Error: %s could not be converted to a number: Out of range!\n", arg);
		return 4;
	}

	*value = result;
	return 0;
}
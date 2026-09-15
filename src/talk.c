#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>

// maximal size of to overlap into another page
#define MAX_PAGE_POPULATION (0x10)

#include "nt.h"
#include "print.h"
#include "Args.h"
#include "warnings.h"
#include "Converter.h"
#include "crypto/BRand.h"
#include "privs.h"
#include "helper.h"

#include "fileio.h"
#include "patterns.h"
#include "argParse.h"



#define BIN_NAME "Talk"
#define VERSION "2.2.7"
#define LAST_CHANGED "15.09.2026"


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

#define DEFAULT_DA (FILE_GENERIC_READ|FILE_GENERIC_WRITE)
#define DEFAULT_IB_FILL_BYTE (0x41)
#define DEFAULT_OB_FILL_BYTE (0x0)

#define MAX_SE_COUNT (0x10)

#define MAX_INTS (0x10)
#define MAX_ONTS (0x10)



typedef struct CmdParams {
    CHAR* DeviceName;
    ULONG InputBufferSize;
    ULONG OutputBufferSize;
    PUINT8 InputBufferData;
    PUINT8 OutputBufferData;
    PUINT8 OutputBufferBase;
    ULONG IoCtl;
    ULONG Sleep;
    ACCESS_MASK DesiredAccess;
    ULONG ShareAccess;
    SE Se;
    struct {
        ULONG Verbose:1;
        ULONG PrintMode:4;
        ULONG ForceOutBufferPrint:1;
        ULONG OverFlowAlignedBuffer:1;
        ULONG Reserved:25;
    } Flags;
    BOOL TestHandle;
    UINT8 InputBufferFillByte;
    UINT8 OutputBufferFillByte;
} CmdParams, * PCmdParams;




INT parseArgs(_In_ INT argc, _In_ CHAR** argv, _Out_ CmdParams* Params);
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
    
    s = parseArgs(argc, argv, &params);
    if ( s != 0 )
    {
        printUsage();
        goto clean;
    }
    
    if ( !checkArgs(&params) )
    {
        printUsage();
        s = ERROR_INVALID_PARAMETER;
        goto clean;
    }
    
    if ( params.Flags.Verbose )
        printArgs(&params);
    
    
    
    if ( params.Se.Count)
    {
        s = setPrivileges(params.Se.List, params.Se.Count, TRUE);
        if ( s != 0 )
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
        NTSTATUS s2 = setPrivileges(params.Se.List, params.Se.Count, FALSE);
        if ( s2 != 0 )
        {
            EPrint("Requested privileges could not be resigned! (0x%x)\n", s2);
        }
    }
    if ( params.Se.List )
        free(params.Se.List);
    if ( params.InputBufferData )
        free(params.InputBufferData);
    if ( params.OutputBufferBase )
    {
        SIZE_T size = 0;
        NtFreeVirtualMemory((HANDLE)-1, &params.OutputBufferBase, &size, MEM_RELEASE);
    }
    else if ( params.OutputBufferData )
    {
        free(params.OutputBufferData);
    }
    
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
    
    if ( Params->OutputBufferData )
    {
        outputBuffer = Params->OutputBufferData;
    }

    status = NtCreateEvent(
                &event,
                FILE_ALL_ACCESS,
                0,
                NotificationEvent,
                0
            );
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
                Params->IoCtl,
                inputBuffer,
                Params->InputBufferSize,
                outputBuffer,
                Params->OutputBufferSize
            );
    
    if ( Params->Flags.Verbose )
    {
        printf("returned\n");
        printf("  status: 0x%x\n", status);
        printf("  iosb.status: 0x%x\n", iosb.Status);
    }
    if ( status == STATUS_PENDING )
    {
        if ( Params->Flags.Verbose )
        {
            printf("Pending\n");
            printf("Waiting for event to signal.\n");
        }
        status = NtWaitForSingleObject(event, 0, 0);
        if ( status == 0 )
            status = iosb.Status;
    }
    
    if ( status != 0 )
    {
        if ( NT_WARNING(status) )
        {
            printf("[w] DeviceIo failed! (0x%08x)\n", status);
        }
        else
        {
            EPrint("DeviceIo failed! (0x%08x)\n", status);
        }
        if ( Params->Flags.Verbose )
        {
            printf("    %s\n", getStatusString(status));
            printf("    iosb info: 0x%08x\n", (ULONG)iosb.Information);
        }

        // skip output buffer printing in error case, if not forced to print
        // the output buffer of warnings should be printed anyway
        if ( NT_ERROR(status) && !Params->Flags.ForceOutBufferPrint )
            goto clean;
    }

    if ( Params->Sleep )
    {
        if ( Params->Flags.Verbose )
            printf("Sleeping for 0x%x (%u) ms.\n", Params->Sleep, Params->Sleep);
        
        Sleep(Params->Sleep);
    }
    
    printf("\n");
    
    bytesReturned = (ULONG)iosb.Information;
    printf("The driver returned 0x%x bytes:\n", bytesReturned);

    if ( bytesReturned > Params->OutputBufferSize )
    {
        EPrint("Output buffer to small, adjust its size with the /os parameter.");
        goto clean;
    }

    if ( !outputBuffer )
    {
        DPrint("No output buffer given!\n");
        goto clean;
    }
    
    SIZE_T toPrint = 0;
    UINT32 method = METHOD_FROM_CTL_CODE(Params->IoCtl);
    // use output buffer size, because iosb.Information is not reliable or not filled at all.
    if ( method == METHOD_NEITHER || method == METHOD_OUT_DIRECT )
    {
        toPrint = Params->OutputBufferSize;
    }
    else
    {
        // for buffered io, iosb.Information is the only possible buffer size
        toPrint = bytesReturned;
    }
    
    if ( toPrint )
    {
        printf("-----------------------------");
        UINT32 zc = countHexChars(bytesReturned);
        for ( UINT32 zci = 0; zci < zc; zci++ ) printf("-");
        printf("\n");

// warning C6385: Reading invalid data from 'outputBuffer': the readable size is 'Params->OutputBufferSize' bytes, but '2' bytes may be read ??
DISABLE_WARNING ( 6385 )
        switch ( Params->Flags.PrintMode )
        {
            case PRINT_MODE_BYTES:
                PrintMemBytes(outputBuffer, bytesReturned);
                printf("\n");
                break;
            case PRINT_MODE_BYTE_STR:
                PrintMemByteStr(outputBuffer, bytesReturned);
                printf("\n");
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
                PrintAStr(outputBuffer, bytesReturned);
                break;
            case PRINT_MODE_UNICODE:
                PrintWStr(outputBuffer, bytesReturned);
                break;
            default:
                PrintMemCols8(outputBuffer, bytesReturned, 0);
                break;
        }
DEFAULT_WARNING ( 6385 )
        printf("-----------------------------");
        for ( UINT32 zci = 0; zci < zc; zci++ ) printf("-");
        printf("\n");
    }


clean:
    if ( event )
        NtClose(event);

    return status;
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

#define CONTINUE_IF_OD_SET(__od__, __i__) \
{ \
    if ( __od__ ) \
    { \
        printf("[i] OutputData already set! Skipping!\n"); \
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

#define CONTINUE_IF_MAX_ONT_COUNT(__cnt__, __i__) \
{ \
    if ( __cnt__ >= MAX_ONTS ) \
    { \
        DPrint("[i] Maximum number of output integers reached!\n Skipping!\n"); \
        __i__++; \
        continue; \
    } \
}

INT parseArgs(_In_ INT argc, _In_ CHAR** argv, _Out_ CmdParams* Params)
{
    INT start_i = 1;
    INT last_i = argc;
    INT i;
    INT s = 0;

    char* arg = NULL;
    char *val1 = NULL;
    char *val2 = NULL;
    
    INT intCount = 0;
    INPUT_INT ints[MAX_INTS] = { 0 };

    INT ontCount = 0;
    INPUT_INT onts[MAX_ONTS] = { 0 };

    INT seId[MAX_SE_COUNT] = {0};
    INT seIdCount = 0;
    
    ZeroMemory(Params, sizeof(CmdParams));
    Params->DesiredAccess = DEFAULT_DA;
    Params->ShareAccess = FILE_SHARE_READ|FILE_SHARE_WRITE;
    Params->InputBufferFillByte = DEFAULT_IB_FILL_BYTE;
    Params->OutputBufferFillByte = DEFAULT_OB_FILL_BYTE;

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
        else if ( IS_1C_ARG(arg, 'c') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No ioctl code set!\n");
  
            s = parseUint32(val1, &Params->IoCtl, 0x10);
            if ( s != 0 )
                break;

            i++;
        }
        //
        // input buffer filling
        //
        else if ( IS_2C_ARG(arg, 'ix') || IS_2C_ARG(arg, 'ih') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No hex given!\n");

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
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No byte given!\n");

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
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No word given!\n");

            //CONTINUE_IF_ID_SET(Params->InputBufferData, i);
            CONTINUE_IF_MAX_INT_COUNT(intCount, i);
            
            ints[intCount].Id = i+1;
            ints[intCount].Size = 2;
            intCount++;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'id') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No dword given!\n");
            
            CONTINUE_IF_MAX_INT_COUNT(intCount, i);
            //CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            ints[intCount].Id = i+1;
            ints[intCount].Size = 4;
            intCount++;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'iq') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No qword given!\n");
            
            CONTINUE_IF_MAX_INT_COUNT(intCount, i);
            //CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            ints[intCount].Id = i+1;
            ints[intCount].Size = 8;
            intCount++;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'ia') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No ASCII string given!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);
            
            s = parseStringA(val1, &Params->InputBufferData, &Params->InputBufferSize);
            if ( s != 0 )
                break;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'iu') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No unicode string given!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);
            
            s = parseStringU(val1, &Params->InputBufferData, &Params->InputBufferSize);
            if ( s != 0 )
                break;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'if') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No file given!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            s = parseFile(val1, &Params->InputBufferData, &Params->InputBufferSize);
            if ( s != 0 )
                break;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'ir') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No random length set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);
            
            s = parseRandom(val1, &Params->InputBufferData, &Params->InputBufferSize);
            if ( s != 0 )
                break;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'ip') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No pattern length set!\n");

            CONTINUE_IF_ID_SET(Params->InputBufferData, i);
            
            s = parsePattern(val1, &Params->InputBufferData, &Params->InputBufferSize);
            if ( s != 0 )
                break;

            i++;
        }
        else if ( IS_3C_ARG(arg, 'ipc') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No pattern start value set!\n");
            
            val2 = GET_ARG_VALUE(argc, argv, i, 2);
            BREAK_ON_NOT_A_VALUE(val2, s, "[e] No pattern length set!\n");
            
            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            s = parseCustomPattern(val1, val2, &Params->InputBufferData, &Params->InputBufferSize);
            if ( s != 0 )
                break;

            i += 2;
        }
        else if ( IS_2C_ARG(arg, 'is') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No input length set!\n");
            
            CONTINUE_IF_ID_SET(Params->InputBufferData, i);

            s = parseUint32(val1, &Params->InputBufferSize, 0);
            if ( s != 0 )
                break;

            i++;
        }
        //
        // output buffer filling
        //
        else if ( IS_2C_ARG(arg, 'ox') || IS_2C_ARG(arg, 'oh') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No hex given!\n");

            CONTINUE_IF_OD_SET(Params->OutputBufferData, i);

            s = parsePlainBytes(val1, &Params->OutputBufferData, &Params->OutputBufferSize, MAXUINT32);
            if ( s != 0 )
            {
                break;
            }
            
            i++;
        }
        else if ( IS_2C_ARG(arg, 'ob') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No byte given!\n");

            //CONTINUE_IF_OD_SET(Params->OutputBufferData, i);
            CONTINUE_IF_MAX_ONT_COUNT(ontCount, i);

            onts[ontCount].Id = i+1;
            onts[ontCount].Size = 1;
            ontCount++;
            
            i++;
        }
        else if ( IS_2C_ARG(arg, 'ow') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No word given!\n");

            //CONTINUE_IF_OD_SET(Params->OutputBufferData, i);
            CONTINUE_IF_MAX_ONT_COUNT(ontCount, i);
            
            onts[ontCount].Id = i+1;
            onts[ontCount].Size = 2;
            ontCount++;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'od') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No dword given!\n");
            
            CONTINUE_IF_MAX_ONT_COUNT(ontCount, i);
            //CONTINUE_IF_OD_SET(Params->OutputBufferData, i);

            onts[ontCount].Id = i+1;
            onts[ontCount].Size = 4;
            ontCount++;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'oq') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No qword given!\n");
            
            CONTINUE_IF_MAX_ONT_COUNT(ontCount, i);
            //CONTINUE_IF_OD_SET(Params->OutputBufferData, i);

            onts[ontCount].Id = i+1;
            onts[ontCount].Size = 8;
            ontCount++;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'oa') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No ASCII string given!\n");

            CONTINUE_IF_OD_SET(Params->OutputBufferData, i);

            s = parseStringA(val1, &Params->OutputBufferData, &Params->OutputBufferSize);
            if ( s != 0 )
                break;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'ou') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No unicode string given!\n");

            CONTINUE_IF_OD_SET(Params->OutputBufferData, i);
            
            s = parseStringU(val1, &Params->OutputBufferData, &Params->OutputBufferSize);
            if ( s != 0 )
                break;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'of') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No file given!\n");

            CONTINUE_IF_OD_SET(Params->OutputBufferData, i);

            s = parseFile(val1, &Params->OutputBufferData, &Params->OutputBufferSize);
            if ( s != 0 )
                break;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'or') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No random length set!\n");

            CONTINUE_IF_OD_SET(Params->OutputBufferData, i);
            
            s = parseRandom(val1, &Params->OutputBufferData, &Params->OutputBufferSize);
            if ( s != 0 )
                break;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'op') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No pattern length set!\n");

            CONTINUE_IF_OD_SET(Params->OutputBufferData, i);
            
            s = parsePattern(val1, &Params->OutputBufferData, &Params->OutputBufferSize);
            if ( s != 0 )
                break;
            i++;
        }
        else if ( IS_3C_ARG(arg, 'opc') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No pattern start value set!\n");
            
            val2 = GET_ARG_VALUE(argc, argv, i, 2);
            BREAK_ON_NOT_A_VALUE(val2, s, "[e] No pattern length set!\n");
            
            CONTINUE_IF_OD_SET(Params->OutputBufferData, i);
            
            s = parseCustomPattern(val1, val2, &Params->OutputBufferData, &Params->OutputBufferSize);
            if ( s != 0 )
                break;

            i += 2;
        }
        else if ( IS_2C_ARG(arg, 'os') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No output length set!\n");

            CONTINUE_IF_OD_SET(Params->OutputBufferData, i);

            s = parseUint32(val1, &Params->OutputBufferSize, 0);
            if ( s != 0 )
                break;

            i++;
        }
        //
        // other stuff
        //
        else if ( IS_2C_ARG(arg, 'da') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No desired access flag set!\n");

            s = parseUint32(val1, &Params->DesiredAccess, 0);
            if ( s != 0 )
                break;

            i++;
        }
        else if ( IS_1C_ARG(arg, 's') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No sleep length set!\n");

            s = parseUint32(val1, &Params->Sleep, 0);
            if ( s != 0 )
                break;

            i++;
        }
        else if ( IS_2C_ARG(arg, 'sa') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No shareAccess flag set!\n");

            s = parseUint32(val1, &Params->ShareAccess, 0);
            if ( s != 0 )
                break;

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
        else if ( IS_4C_ARG(arg, 'fobp') )
        {
            Params->Flags.ForceOutBufferPrint = 1;
        }
        else if ( IS_4C_ARG(arg, 'ofao') )
        {
            Params->Flags.OverFlowAlignedBuffer = 1;
        }
        else if ( IS_4C_ARG(arg, 'ibfb') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No fill byte set!\n");

            s = parseUint8(val1, &Params->InputBufferFillByte, 0x10);

            i++;
        }
        else if ( IS_4C_ARG(arg, 'obfb') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "[e] No fill byte set!\n");

            s = parseUint8(val1, &Params->OutputBufferFillByte, 0x10);
            if ( s != 0 )
                break;

            i++;
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
    
    s = parseIOnts(argc, argv, ints, intCount, &Params->InputBufferData, &Params->InputBufferSize);
    if ( s != 0 )
    {
        goto clean;
    }
    
    s = parseIOnts(argc, argv, onts, ontCount, &Params->OutputBufferData, &Params->OutputBufferSize);
    if ( s != 0 )
    {
        goto clean;
    }
    
    s = parseSe(argc, argv, &Params->Se, seId, seIdCount);
    if ( s != 0 )
    {
        goto clean;
    }

    // fill input with a fill byte, if just /is has been set
    s = parseIOBSize(&Params->InputBufferData, Params->InputBufferSize, Params->InputBufferFillByte);
    if ( s != 0 )
        goto clean;

    // fill output with a fill byte, if just /os has been set
    s = parseIOBSize(&Params->OutputBufferData, Params->OutputBufferSize, Params->OutputBufferFillByte);
    if ( s != 0 )
        goto clean;

    // if flagged
    // relocate output buffer to special page alignment for overflow kindness
    if ( Params->Flags.OverFlowAlignedBuffer && Params->OutputBufferSize > 0 && Params->OutputBufferData )
    {
        PVOID base = NULL;
        PVOID buffer = NULL;
        DPrint("realigning buffer: %p\n", Params->OutputBufferData);
        buffer = allocCMPL(Params->OutputBufferSize, &base, MAX_PAGE_POPULATION);
        if ( !buffer )
        {
            EPrint("realigning buffer failed!\n");
            goto clean;
        }
        DPrint("  base: %p\n", base);
        DPrint("  buffer: %p\n", buffer);
        free(Params->OutputBufferData);
        Params->OutputBufferData = buffer;
        Params->OutputBufferBase = base;
    }

clean:

    return s;
}

int openDevice(_Out_ PHANDLE Device, _In_ CHAR* DeviceNameA, _In_ ACCESS_MASK DesiredAccess, _In_ ULONG ShareAccess)
{
    INT s = 0;
    PWCHAR deviceNameW = NULL;
    SIZE_T deviceNameWCb = strlen(DeviceNameA) * 2;

    *Device = NULL;

    deviceNameW = (PWCHAR)malloc(deviceNameWCb + 2);
    if ( !deviceNameW )
        return STATUS_INSUFFICIENT_RESOURCES;
    
    s = StringCbPrintfW(deviceNameW, deviceNameWCb+2, L"%hs", DeviceNameA);
    if ( s != 0 )
        goto clean;


    s = openFile(Device, deviceNameW, DesiredAccess, ShareAccess);

clean:
    if ( deviceNameW )
        free(deviceNameW);

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
    printf(" - IOCTL: 0x%x\n", Params->IoCtl);
    printf(" - InputBufferSize: 0x%x\n", Params->InputBufferSize);
    if ( Params->InputBufferData )
    {
        printf(" - InputBufferData:\n");
        SIZE_T printSize = min(Params->InputBufferSize, 0x100);
        PrintMemBytes(Params->InputBufferData, printSize);
        if ( printSize < Params->InputBufferSize)
            printf("[...]");
        printf("\n");
    }
    printf(" - OutputBufferSize: 0x%x\n", Params->OutputBufferSize);
    if ( Params->OutputBufferData )
    {
        printf(" - OutputBufferData:\n");
        SIZE_T printSize = min(Params->OutputBufferSize, 0x100);
        PrintMemBytes(Params->OutputBufferData, printSize);
        if ( printSize < Params->OutputBufferSize)
            printf("[...]");
        printf("\n");
    }
    printf(" - Sleep: 0x%x\n", Params->Sleep);
    printf(" - TestHandle: %d\n", Params->TestHandle);
    printf(" - DesiredAccess: 0x%x\n", Params->DesiredAccess);
    printf(" - ShareAccess: 0x%x\n", Params->ShareAccess);
    printf(" - InputBufferFillByte: 0x%x\n", Params->InputBufferFillByte);
    printf(" - OutputBufferFillByte: 0x%x\n", Params->OutputBufferFillByte);
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
           "[/is|/ir|/ip|/os|/or|/op <size>] "
           "[/ipc|/opc <pattern> <size>] "
           "[/i|o(x|b|w|d|q|a|u) <data>] "
           "[/if|/of <file>] "
           "[/s <sleep>] "
           "[/da <flags>] "
           "[/sa <flags>] "
           "[/se <priv>] "
           "[/fobp] "
           "[/ofao] "
           "[/ibfb <value>] "
           "[/obfb <value>] "
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
    printf(" - Input Data:\n");
    printf("    (The integer types are chainable.)\n");
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
    printf("    * /is Input data will be filled with <size> 0x41 or another fill byte (/ibfb).\n");
    printf(" - Output Data:\n");
    printf("    (Sometimes the output buffer might need to be filled as well.)\n");
    printf("    (The integer types are chainable.)\n");
    printf("    * /os Size of OutputBuffer to be filled with <size> 0 or another fill byte (/obfb).\n");
    printf("    * /ox <Data> as hex byte string.\n");
    printf("    * /ob <Data> as byte.\n");
    printf("    * /ow <Data> as word (uint16).\n");
    printf("    * /od <Data> as dword (uint32).\n");
    printf("    * /oq <Data> as qword (uint64).\n");
    printf("    * /oa <Data> as ascii text.\n");
    printf("    * /ou <Data> as unicode (utf-16) text.\n");
    printf("    * /of Input data is read as binary data from the file <path>.\n");
    printf("    * /or Input data will be filled with <size> random bytes.\n");
    printf("    * /op Input data will be filled with <size> default pattern bytes (Aa0Aa1...).\n");
    printf("    * /opc Input data will be filled with <size> custom pattern bytes, starting from <pattern>, incremented by 1.\n");
    printf(" - /s Duration of a possible sleep after device io.\n");
    printf(" - /t Just test the device for accessibility. Don't send data.\n");
    printf(" - /da DesiredAccess flags to open the device. Defaults to FILE_GENERIC_READ|FILE_GENERIC_WRITE = 0x%x.\n", DEFAULT_DA);
    printf(" - /sa ShareAccess flags to open the device. Defaults to FILE_SHARE_READ|FILE_SHARE_WRITE = 0x%x.\n", (FILE_SHARE_READ|FILE_SHARE_WRITE));
    printf(" - /se Additional SE_XXX privilege (if run as admin). Can be set multiple (0x%x) times for multiple privileges.\n", MAX_SE_COUNT);
    printf(" - /fobp Force printing of the output buffer, even in an error case.\n");
    printf(" - /ofao Aligns output buffer to maximal reach 0x%x bytes into a page.\n", MAX_PAGE_POPULATION);
    printf(" - /ibfb Fill byte value for the input buffer. Default 0x%x.\n", DEFAULT_IB_FILL_BYTE);
    printf(" - /obfb Fill byte value for the output buffer. Default 0x%x.\n", DEFAULT_OB_FILL_BYTE);
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
    printf("$ Talk.exe /n \\Device\\Beep /c 0x10000 /ix 020200003e080000 /s 0x083e\n");
    printf("$ Talk.exe /n \\Device\\Beep /c 0x10000 /id 0x202 /id 0x83e /s 0x083e\n");
    printf("$ Talk.exe /n \\Device\\HarddiskVolume1 /c 0x4d0008 /os 0x100 /pu\n");
}

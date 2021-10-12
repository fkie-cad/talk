#ifndef NT_STUFF_H
#define NT_STUFF_H

#define STATUS_SUCCESS                       ((NTSTATUS)0x00000000L)
#define STATUS_NO_SUCH_FILE                  ((NTSTATUS)0xC000000FL)   
#define STATUS_INVALID_DEVICE_REQUEST        ((NTSTATUS)0xC0000010L)   
#define STATUS_ACCESS_DENIED                 ((NTSTATUS)0xC0000022L)   
#define STATUS_OBJECT_NAME_INVALID           ((NTSTATUS)0xC0000033L)   
#define STATUS_OBJECT_NAME_NOT_FOUND         ((NTSTATUS)0xC0000034L)   
#define STATUS_OBJECT_PATH_NOT_FOUND         ((NTSTATUS)0xC000003AL)   
#define STATUS_ILLEGAL_FUNCTION              ((NTSTATUS)0xC00000AFL)   
#define STATUS_NOT_SUPPORTED                 ((NTSTATUS)0xC00000BBL)   
#define STATUS_DATATYPE_MISALIGNMENT_ERROR   ((NTSTATUS)0xC00002C5L)    




typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE, *PEVENT_TYPE;

NTSYSAPI NTSTATUS NtCreateEvent(
    PHANDLE            EventHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    EVENT_TYPE         EventType,
    BOOLEAN            InitialState
);

const char* getStatusString(NTSTATUS status)
{
    if (status == STATUS_ACCESS_DENIED)
        return "STATUS_ACCESS_DENIED";
    else if (status == STATUS_INVALID_PARAMETER)
        return "STATUS_INVALID_PARAMETER";
    else if (status == STATUS_NO_SUCH_FILE)
        return "STATUS_NO_SUCH_FILE";
    else if (status == STATUS_INVALID_DEVICE_REQUEST)
        return "STATUS_INVALID_DEVICE_REQUEST: The specified request is not a valid operation for the target device";
    else if (status == STATUS_ILLEGAL_FUNCTION)
        return "STATUS_ILLEGAL_FUNCTION: kernel driver is irritated";
    else if (status == STATUS_INVALID_HANDLE)
        return "STATUS_INVALID_HANDLE";
    else if (status == STATUS_DATATYPE_MISALIGNMENT_ERROR)
        return "STATUS_DATATYPE_MISALIGNMENT_ERROR: A data type misalignment error was detected in a load or store instruction";
    else if (status == STATUS_NOT_SUPPORTED)
        return "STATUS_NOT_SUPPORTED: The request is not supported";
    else if (status == STATUS_OBJECT_NAME_INVALID)
        return "STATUS_OBJECT_NAME_INVALID";
    else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        return "STATUS_OBJECT_NAME_NOT_FOUND";
    else if (status == STATUS_OBJECT_PATH_NOT_FOUND)
        return "STATUS_OBJECT_PATH_NOT_FOUND";

    return "unknown";
}

#endif

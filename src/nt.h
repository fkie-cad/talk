#ifndef NT_STUFF_H
#define NT_STUFF_H

#define STATUS_SUCCESS                       ((NTSTATUS)0x00000000L)
#define STATUS_NOT_IMPLEMENTED               ((NTSTATUS)0xC0000002L)
#define STATUS_NO_SUCH_DEVICE                ((NTSTATUS)0xC000000EL)
#define STATUS_NO_SUCH_FILE                  ((NTSTATUS)0xC000000FL)
#define STATUS_INVALID_DEVICE_REQUEST        ((NTSTATUS)0xC0000010L)
#define STATUS_ACCESS_DENIED                 ((NTSTATUS)0xC0000022L)
#define STATUS_OBJECT_NAME_INVALID           ((NTSTATUS)0xC0000033L)
#define STATUS_OBJECT_NAME_NOT_FOUND         ((NTSTATUS)0xC0000034L)
#define STATUS_OBJECT_NAME_COLLISION         ((NTSTATUS)0xC0000035L)
#define STATUS_OBJECT_PATH_NOT_FOUND         ((NTSTATUS)0xC000003AL)
#define STATUS_OBJECT_PATH_SYNTAX_BAD        ((NTSTATUS)0xC000003BL)
#define STATUS_ILLEGAL_FUNCTION              ((NTSTATUS)0xC00000AFL)
#define STATUS_NOT_SUPPORTED                 ((NTSTATUS)0xC00000BBL)
#define STATUS_NOT_FOUND                     ((NTSTATUS)0xC0000225L)
#define STATUS_DATATYPE_MISALIGNMENT_ERROR   ((NTSTATUS)0xC00002C5L)


#define NT_PATH_PREFIX_W (0x005c003f003f005c)


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

NTSYSAPI 
NTSTATUS
NtReadFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _In_opt_ PULONG Key
);

#endif

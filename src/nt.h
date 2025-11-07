#pragma once

#define STATUS_SUCCESS                       ((NTSTATUS)0x00000000L)
#define STATUS_NOT_IMPLEMENTED               ((NTSTATUS)0xC0000002L)
#define STATUS_NO_SUCH_DEVICE                ((NTSTATUS)0xC000000EL)
#define STATUS_NO_SUCH_FILE                  ((NTSTATUS)0xC000000FL)
#define STATUS_INVALID_DEVICE_REQUEST        ((NTSTATUS)0xC0000010L)
#define STATUS_ACCESS_DENIED                 ((NTSTATUS)0xC0000022L)
#define STATUS_OBJECT_TYPE_MISMATCH          ((NTSTATUS)0xC0000024L)
#define STATUS_OBJECT_NAME_INVALID           ((NTSTATUS)0xC0000033L)
#define STATUS_OBJECT_NAME_NOT_FOUND         ((NTSTATUS)0xC0000034L)
#define STATUS_OBJECT_NAME_COLLISION         ((NTSTATUS)0xC0000035L)
#define STATUS_OBJECT_PATH_NOT_FOUND         ((NTSTATUS)0xC000003AL)
#define STATUS_OBJECT_PATH_SYNTAX_BAD        ((NTSTATUS)0xC000003BL)
#define STATUS_PRIVILEGE_NOT_HELD            ((NTSTATUS)0xC0000061L)
#define STATUS_ILLEGAL_FUNCTION              ((NTSTATUS)0xC00000AFL)
#define STATUS_NOT_SUPPORTED                 ((NTSTATUS)0xC00000BBL)
#define STATUS_NOT_ALL_ASSIGNED              ((NTSTATUS)0x00000106L)
#define STATUS_NOT_FOUND                     ((NTSTATUS)0xC0000225L)
#define STATUS_DATATYPE_MISALIGNMENT_ERROR   ((NTSTATUS)0xC00002C5L)


#define NT_PATH_PREFIX_W (0x005c003f003f005c)

#define SE_MIN_WELL_KNOWN_PRIVILEGE         (2L)
#define SE_CREATE_TOKEN_PRIVILEGE           (2L)
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE     (3L)
#define SE_LOCK_MEMORY_PRIVILEGE            (4L)
#define SE_INCREASE_QUOTA_PRIVILEGE         (5L)


#define SE_MACHINE_ACCOUNT_PRIVILEGE        (6L)
#define SE_TCB_PRIVILEGE                    (7L)
#define SE_SECURITY_PRIVILEGE               (8L)
#define SE_TAKE_OWNERSHIP_PRIVILEGE         (9L)
#define SE_LOAD_DRIVER_PRIVILEGE            (10L)
#define SE_SYSTEM_PROFILE_PRIVILEGE         (11L)
#define SE_SYSTEMTIME_PRIVILEGE             (12L)
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE    (13L)
#define SE_INC_BASE_PRIORITY_PRIVILEGE      (14L)
#define SE_CREATE_PAGEFILE_PRIVILEGE        (15L)
#define SE_CREATE_PERMANENT_PRIVILEGE       (16L)
#define SE_BACKUP_PRIVILEGE                 (17L)
#define SE_RESTORE_PRIVILEGE                (18L)
#define SE_SHUTDOWN_PRIVILEGE               (19L)
#define SE_DEBUG_PRIVILEGE                  (20L)
#define SE_AUDIT_PRIVILEGE                  (21L)
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE     (22L)
#define SE_CHANGE_NOTIFY_PRIVILEGE          (23L)
#define SE_REMOTE_SHUTDOWN_PRIVILEGE        (24L)
#define SE_UNDOCK_PRIVILEGE                 (25L)
#define SE_SYNC_AGENT_PRIVILEGE             (26L)
#define SE_ENABLE_DELEGATION_PRIVILEGE      (27L)
#define SE_MANAGE_VOLUME_PRIVILEGE          (28L)
#define SE_IMPERSONATE_PRIVILEGE            (29L)
#define SE_CREATE_GLOBAL_PRIVILEGE          (30L)
#define SE_TRUSTED_CREDMAN_ACCESS_PRIVILEGE (31L)
#define SE_RELABEL_PRIVILEGE                (32L)
#define SE_INC_WORKING_SET_PRIVILEGE        (33L)
#define SE_TIME_ZONE_PRIVILEGE              (34L)
#define SE_CREATE_SYMBOLIC_LINK_PRIVILEGE   (35L)
#define SE_DELEGATE_SESSION_USER_IMPERSONATE_PRIVILEGE   (36L)
#define SE_MAX_WELL_KNOWN_PRIVILEGE         (SE_DELEGATE_SESSION_USER_IMPERSONATE_PRIVILEGE)


#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)


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

NTSYSAPI
NTSTATUS
NTAPI
NtAdjustPrivilegesToken(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN DisableAllPrivileges,
    _In_opt_ PTOKEN_PRIVILEGES NewState,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_to_opt_(BufferLength, *ReturnLength) PTOKEN_PRIVILEGES PreviousState,
    _Out_opt_ PULONG ReturnLength);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle);

FORCEINLINE 
LUID 
RtlConvertUlongToLuid(
    _In_ ULONG Ulong
)
{
    LUID tempLuid;

    tempLuid.LowPart = Ulong;
    tempLuid.HighPart = 0;

    return tempLuid;
}
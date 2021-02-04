#ifndef TALK_H
#define TALK_H

#define STATUS_SUCCESS                       ((NTSTATUS)0x00000000L)
#define STATUS_NO_SUCH_FILE                  ((NTSTATUS)0xC000000FL)   
#define STATUS_INVALID_DEVICE_REQUEST        ((NTSTATUS)0xC0000010L)   
#define STATUS_ACCESS_DENIED                 ((NTSTATUS)0xC0000022L)   
#define STATUS_OBJECT_NAME_NOT_FOUND         ((NTSTATUS)0xC0000034L)   
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

#endif

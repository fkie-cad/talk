#pragma

NTSTATUS setPrivileges(
    _In_ ULONG *Privileges,
    _In_ ULONG PrivilegesCount,
    _In_ INT Enable
)
{
    ULONG retLength;
    NTSTATUS status = 0;
    HANDLE token = NULL;
    LUID luid;

    status = NtOpenProcessToken(
        NtCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &token
    );
    if ( !NT_SUCCESS(status) )
    {
        EPrint("NtOpenProcessToken failed! (0x%x)\n", status);
        return status;
    }

    ULONG tokenPrivsSize = (ULONG)(sizeof(TOKEN_PRIVILEGES) + (PrivilegesCount-1) * sizeof(LUID_AND_ATTRIBUTES));
    TOKEN_PRIVILEGES* tokenPrivs = (PTOKEN_PRIVILEGES)malloc(tokenPrivsSize);
    if ( !tokenPrivs )
        return STATUS_NO_MEMORY;
    
    tokenPrivs->PrivilegeCount = PrivilegesCount;
    for ( ULONG i = 0; i < PrivilegesCount; i++ )
    {
        luid = RtlConvertUlongToLuid(Privileges[i]);

        tokenPrivs->Privileges[i].Luid = luid;
        tokenPrivs->Privileges[i].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;
    }

    status = NtAdjustPrivilegesToken(token,
        FALSE,
        tokenPrivs,
        tokenPrivsSize,
        NULL,
        &retLength
    );
    if ( status == STATUS_NOT_ALL_ASSIGNED )
    {
        EPrint("NtAdjustPrivilegesToken failed! (0x%x)\n", status);
        status = STATUS_PRIVILEGE_NOT_HELD;
    }

    if ( token )
        NtClose(token);
    if ( tokenPrivs )
        free(tokenPrivs);

    return status;
}

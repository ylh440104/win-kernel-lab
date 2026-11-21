static ULONG64 LxCopyUserBytes(ULONG64 Address, PVOID Buffer, SIZE_T Length, BOOLEAN ToUser)
{
    LX_APC_STATE apcState;
    BOOLEAN attached = FALSE;
    ULONG64 result = 0;
    ULONG64 ceiling = (ULONG64)(ULONG_PTR)MmHighestUserAddress;
    if (Length == 0)
    {
        return 0;
    }
    if (Address == 0 || Address > ceiling || Length - 1 > ceiling - Address)
    {
        return (ULONG64)(-14);
    }
    __try
    {
        if (g_PicoProcessObject != NULL && PsGetCurrentProcess() != g_PicoProcessObject)
        {
            KeStackAttachProcess(g_PicoProcessObject, (PVOID)&apcState);
            attached = TRUE;
        }
        __try
        {
            if (ToUser)
            {
                ProbeForWrite((PVOID)(ULONG_PTR)Address, Length, 1);
                RtlCopyMemory((PVOID)(ULONG_PTR)Address, Buffer, Length);
            }
            else
            {
                ProbeForRead((PVOID)(ULONG_PTR)Address, Length, 1);
                RtlCopyMemory(Buffer, (PVOID)(ULONG_PTR)Address, Length);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            result = (ULONG64)(-14);
        }
    }
    __finally
    {
        if (attached)
        {
            KeUnstackDetachProcess((PVOID)&apcState);
        }
    }
    return result;
}

static ULONG64 LxCopyUserPathExact(ULONG64 Address, CHAR *Path, ULONG Capacity)
{
    LX_APC_STATE apcState;
    BOOLEAN attached = FALSE;
    ULONG64 result = (ULONG64)(-36);
    ULONG64 ceiling = (ULONG64)(ULONG_PTR)MmHighestUserAddress;
    ULONG index;
    if (Address == 0 || Address > ceiling || Capacity < 2)
    {
        return (ULONG64)(-14);
    }
    Path[0] = 0;
    __try
    {
        if (g_PicoProcessObject != NULL && PsGetCurrentProcess() != g_PicoProcessObject)
        {
            KeStackAttachProcess(g_PicoProcessObject, (PVOID)&apcState);
            attached = TRUE;
        }
        __try
        {
            for (index = 0; index < Capacity; index++)
            {
                CHAR value;
                if (index > ceiling - Address)
                {
                    result = (ULONG64)(-14);
                    break;
                }
                value = *(volatile const CHAR *)(ULONG_PTR)(Address + index);
                Path[index] = value;
                if (value == 0)
                {
                    result = 0;
                    break;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            result = (ULONG64)(-14);
        }
    }
    __finally
    {
        if (attached)
        {
            KeUnstackDetachProcess((PVOID)&apcState);
        }
    }
    Path[Capacity - 1] = 0;
    return result;
}

static ULONG64 LxResolveAtPath(LONG DirFd, const CHAR *Path, CHAR *Resolved, ULONG Capacity)
{
    const CHAR *directory = "/";
    ULONG prefix = 0;
    ULONG index = 0;
    if (Path[0] == 0)
    {
        return (ULONG64)(-2);
    }
    if (Path[0] != '/')
    {
        if (DirFd != -100)
        {
            if (DirFd < 0 || DirFd >= LX_MAX_FD || !g_FdTable[DirFd].Used)
            {
                return (ULONG64)(-9);
            }
            if (!g_FdTable[DirFd].IsDirectory)
            {
                return (ULONG64)(-20);
            }
            directory = g_FdTable[DirFd].LinuxPath;
        }
        while (directory[prefix] != 0 && prefix + 1 < Capacity)
        {
            Resolved[prefix] = directory[prefix];
            prefix++;
        }
        if (prefix != 0 && Resolved[prefix - 1] != '/')
        {
            if (prefix + 1 >= Capacity)
            {
                return (ULONG64)(-36);
            }
            Resolved[prefix++] = '/';
        }
    }
    while (Path[index] != 0)
    {
        if (prefix + index + 1 >= Capacity)
        {
            return (ULONG64)(-36);
        }
        Resolved[prefix + index] = Path[index];
        index++;
    }
    Resolved[prefix + index] = 0;
    return 0;
}

static ULONG64 LxSysOpenDecoded(ULONG64 Context, BOOLEAN At)
{
    LX_SYSCALL_REGISTERS registers;
    CHAR path[260];
    CHAR resolved[260];
    ULONG64 result;
    if (!NT_SUCCESS(LxDecodeSyscallRegisters(Context, &registers)))
    {
        return (ULONG64)(-14);
    }
    result = LxCopyUserPathExact(registers.Args[At ? 1 : 0], path, sizeof(path));
    if ((LONG64)result < 0)
    {
        return result;
    }
    result = LxResolveAtPath(At ? (LONG)registers.Args[0] : -100, path, resolved, sizeof(resolved));
    if ((LONG64)result < 0)
    {
        return result;
    }
    return LxOpenFile(resolved, (ULONG)registers.Args[At ? 2 : 1],
                      (ULONG)registers.Args[At ? 3 : 2]);
}

static ULONG64 LxSysReadDecoded(ULONG64 Context, BOOLEAN Positioned)
{
    LX_SYSCALL_REGISTERS registers;
    ULONG fd;
    ULONG64 userBuffer;
    ULONG64 count;
    ULONG64 total = 0;
    ULONG64 result = 0;
    LONG64 position;
    PUCHAR buffer;
    NTSTATUS status;
    if (!NT_SUCCESS(LxDecodeSyscallRegisters(Context, &registers)))
    {
        return (ULONG64)(-14);
    }
    fd = (ULONG)registers.Args[0];
    userBuffer = registers.Args[1];
    count = registers.Args[2] > 0x7FFFF000ull ? 0x7FFFF000ull : registers.Args[2];
    if (fd >= LX_MAX_FD || !g_FdTable[fd].Used || g_FdTable[fd].Handle == NULL ||
        (g_FdTable[fd].OpenFlags & LX_O_ACCMODE) == 1)
    {
        return (ULONG64)(-9);
    }
    if (g_FdTable[fd].IsDirectory)
    {
        return (ULONG64)(-21);
    }
    position = Positioned ? (LONG64)registers.Args[3] : (LONG64)g_FdTable[fd].Offset;
    if (position < 0 || count > 0x7FFFFFFFFFFFFFFFull - (ULONG64)position)
    {
        return (ULONG64)(-22);
    }
    if (count == 0)
    {
        return 0;
    }
    if (userBuffer == 0 || userBuffer > (ULONG64)(ULONG_PTR)MmHighestUserAddress ||
        count - 1 > (ULONG64)(ULONG_PTR)MmHighestUserAddress - userBuffer)
    {
        return (ULONG64)(-14);
    }
    buffer = (PUCHAR)ExAllocatePool2(POOL_FLAG_NON_PAGED, 65536, 'rIxL');
    if (buffer == NULL)
    {
        return (ULONG64)(-12);
    }
    g_ReadCount++;
    while (total < count)
    {
        IO_STATUS_BLOCK io;
        LARGE_INTEGER offset;
        ULONG bytes = (ULONG)((count - total > 65536) ? 65536 : count - total);
        offset.QuadPart = position + (LONG64)total;
        RtlZeroMemory(&io, sizeof(io));
        status = ZwReadFile(g_FdTable[fd].Handle, NULL, NULL, NULL, &io,
                            buffer, bytes, &offset, NULL);
        g_LastReadStatus = (ULONG)status;
        if (status == STATUS_END_OF_FILE)
        {
            break;
        }
        if (!NT_SUCCESS(status))
        {
            result = LxNtStatusToErrno(status);
            break;
        }
        if (io.Information > bytes)
        {
            result = (ULONG64)(-5);
            break;
        }
        result = LxCopyUserBytes(userBuffer + total, buffer, (SIZE_T)io.Information, TRUE);
        if ((LONG64)result < 0)
        {
            break;
        }
        total += io.Information;
        if (io.Information < bytes)
        {
            break;
        }
    }
    ExFreePoolWithTag(buffer, 'rIxL');
    if (!Positioned)
    {
        g_FdTable[fd].Offset += total;
    }
    return total != 0 ? total : result;
}

static ULONG64 LxSysStatDecoded(ULONG64 Context, ULONG Kind)
{
    LX_SYSCALL_REGISTERS registers;
    LX_LINUX_STAT info;
    CHAR path[260];
    CHAR resolved[260];
    WCHAR ntBuffer[300];
    UNICODE_STRING ntPath;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK io;
    HANDLE handle = NULL;
    BOOLEAN owned = FALSE;
    ULONG flags = 0;
    LONG dirfd = -100;
    ULONG64 output;
    ULONG64 result;
    NTSTATUS status;
    C_ASSERT(sizeof(LX_LINUX_STAT) == 144);
    if (!NT_SUCCESS(LxDecodeSyscallRegisters(Context, &registers)))
    {
        return (ULONG64)(-14);
    }
    g_StatCount++;
    if (Kind == LX_SYS_NEWFSTATAT)
    {
        g_NfaCount++;
        dirfd = (LONG)registers.Args[0];
        flags = (ULONG)registers.Args[3];
        output = registers.Args[2];
        g_NfaDirFd = registers.Args[0];
        g_NfaPath = registers.Args[1];
        g_NfaStat = output;
        g_NfaFlags = flags;
        if ((flags & ~(0x100u | 0x800u | 0x1000u)) != 0)
        {
            return (ULONG64)(-22);
        }
    }
    else
    {
        output = registers.Args[1];
    }
    if (Kind == LX_SYS_FSTAT)
    {
        ULONG fd = (ULONG)registers.Args[0];
        if (fd >= LX_MAX_FD || !g_FdTable[fd].Used || g_FdTable[fd].Handle == NULL)
        {
            return (ULONG64)(-9);
        }
        handle = g_FdTable[fd].Handle;
    }
    else
    {
        result = LxCopyUserPathExact(registers.Args[Kind == LX_SYS_NEWFSTATAT ? 1 : 0], path, sizeof(path));
        if ((LONG64)result < 0)
        {
            return result;
        }
        g_NfaLen = LxKernelPathLength(path);
        RtlZeroMemory(g_LastStatPath, sizeof(g_LastStatPath));
        RtlCopyMemory(g_LastStatPath, path,
                      g_NfaLen < sizeof(g_LastStatPath) - 1 ? (SIZE_T)g_NfaLen : sizeof(g_LastStatPath) - 1);
        if (path[0] == 0 && (flags & 0x1000u) != 0 && dirfd != -100)
        {
            if (dirfd < 0 || dirfd >= LX_MAX_FD || !g_FdTable[dirfd].Used ||
                g_FdTable[dirfd].Handle == NULL)
            {
                return (ULONG64)(-9);
            }
            handle = g_FdTable[dirfd].Handle;
        }
        else
        {
            if (path[0] == 0 && (flags & 0x1000u) != 0)
            {
                path[0] = '/';
                path[1] = 0;
            }
            result = LxResolveAtPath(dirfd, path, resolved, sizeof(resolved));
            if ((LONG64)result < 0)
            {
                return result;
            }
            if (LxBuildNtPath(resolved, ntBuffer, RTL_NUMBER_OF(ntBuffer)) == 0)
            {
                return (ULONG64)(-36);
            }
            RtlInitUnicodeString(&ntPath, ntBuffer);
            InitializeObjectAttributes(&attributes, &ntPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
            status = ZwCreateFile(&handle, FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                  &attributes, &io, NULL, 0,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT |
                                  ((flags & 0x100u) != 0 ? FILE_OPEN_REPARSE_POINT : 0), NULL, 0);
            if (!NT_SUCCESS(status))
            {
                g_LastStatStatus = (ULONG)status;
                return LxNtStatusToErrno(status);
            }
            owned = TRUE;
        }
    }
    result = LxFillStat(handle, &info);
    if (owned)
    {
        ZwClose(handle);
    }
    if ((LONG64)result < 0)
    {
        g_LastStatStatus = result;
        return result;
    }
    result = LxCopyUserBytes(output, &info, sizeof(info), TRUE);
    g_LastStatStatus = result == 0 ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION;
    if (result == 0)
    {
        g_LastStatSize = info.Size;
        g_LastStatMode = info.Mode;
    }
    return result;
}

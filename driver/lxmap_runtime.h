typedef struct _LX_FILE_VIEW
{
    ULONG64 Base;
    ULONG64 Size;
    LONG Fd;
    BOOLEAN Valid;
} LX_FILE_VIEW;

#define LX_MAX_FILE_VIEWS 16u

static LX_FILE_VIEW g_FileViews[LX_MAX_FILE_VIEWS];
static ULONG g_FileViewCount = 0;

static ULONG64 LxCopyFileIntoView(HANDLE FileHandle, ULONG64 TargetBase, ULONG64 FileOffset, ULONG64 Length);
static BOOLEAN LxRegionAvailable(PVOID Base, SIZE_T Length);

static BOOLEAN LxFindFileView(LONG Fd, ULONG64 Address, ULONG64 Length, ULONG64 *ViewBase)
{
    ULONG index;
    if (Length == 0)
    {
        return FALSE;
    }
    for (index = 0; index < g_FileViewCount; index++)
    {
        const LX_FILE_VIEW *view = &g_FileViews[index];
        if (!view->Valid || view->Fd != Fd || view->Base == 0)
        {
            continue;
        }
        if (Address < view->Base || Address - view->Base > view->Size)
        {
            continue;
        }
        if (Address - view->Base > view->Size - Length)
        {
            continue;
        }
        if (ViewBase != NULL)
        {
            *ViewBase = view->Base;
        }
        return TRUE;
    }
    return FALSE;
}

static VOID LxRecordFileView(ULONG64 Base, ULONG64 Size, LONG Fd)
{
    ULONG slot;
    if (g_FileViewCount < LX_MAX_FILE_VIEWS)
    {
        slot = g_FileViewCount;
        g_FileViewCount++;
    }
    else
    {
        slot = LX_MAX_FILE_VIEWS - 1;
    }
    g_FileViews[slot].Base = Base;
    g_FileViews[slot].Size = Size;
    g_FileViews[slot].Fd = Fd;
    g_FileViews[slot].Valid = TRUE;
}

static ULONG64 LxSysMmapFile(ULONG64 Context)
{
    LX_SYSCALL_REGISTERS registers;
    LX_APC_STATE apcState;
    BOOLEAN attached = FALSE;
    ULONG64 address;
    ULONG64 length;
    ULONG64 protection;
    ULONG64 flags;
    ULONG64 offset;
    LONG fd;
    ULONG64 viewBase = 0;
    IO_STATUS_BLOCK io;
    FILE_STANDARD_INFORMATION fileInfo;
    NTSTATUS status;
    PVOID base;
    SIZE_T viewSize;

    status = LxDecodeSyscallRegisters(Context, &registers);
    if (!NT_SUCCESS(status))
    {
        return (ULONG64)(-14);
    }
    address = registers.Args[0];
    length = registers.Args[1];
    protection = registers.Args[2];
    flags = registers.Args[3];
    fd = (LONG)registers.Args[4];
    offset = registers.Args[5];
    g_MmapDebug[0] = address;
    g_MmapDebug[1] = length;
    g_MmapDebug[2] = protection;
    g_MmapDebug[3] = flags;
    g_MmapDebug[4] = (ULONG64)(ULONG)fd;
    g_MmapDebug[5] = offset;
    g_MmapDebug[6] = 0xAAu;
    g_MmapDebug[7] = 0xAAu;
    g_MmapDebug[10] = (ULONG64)g_FileViewCount;
    g_MmapDebug[11] = 0;
    if (length == 0 || length > 0x40000000ull || (offset & 0xFFFull) != 0)
    {
        g_MmapDebug[11] = 0xF1u;
        return (ULONG64)(-22);
    }
    length = (length + 0xFFFull) & ~0xFFFull;
    if (fd < 0 || fd >= LX_MAX_FD || !g_FdTable[fd].Used || g_FdTable[fd].Handle == NULL)
    {
        g_MmapDebug[11] = 0xF2u;
        return (ULONG64)(-9);
    }
    if (g_FdTable[fd].IsDirectory)
    {
        g_MmapDebug[11] = 0xF3u;
        return (ULONG64)(-21);
    }
    if ((flags & 0x10u) != 0)
    {
        if (address == 0 || (address & 0xFFFull) != 0)
        {
            g_MmapDebug[11] = 0xF4u;
            return (ULONG64)(-22);
        }
        if (LxFindFileView(fd, address, length, &viewBase))
        {
            g_MmapDebug[8] = viewBase;
            g_MmapDebug[11] = 0xC1u;
            g_MmapCount++;
            g_LastMmapAddress = address;
            g_LastMmapStatus = 0;
            return address;
        }
    }
    RtlZeroMemory(&fileInfo, sizeof(fileInfo));
    status = ZwQueryInformationFile(g_FdTable[fd].Handle, &io, &fileInfo, sizeof(fileInfo),
                                    FileStandardInformation);
    if (!NT_SUCCESS(status))
    {
        g_MmapDebug[11] = 0xF5u;
        return LxNtStatusToErrno(status);
    }
    g_MmapDebug[9] = (ULONG64)fileInfo.EndOfFile.QuadPart;
    if ((flags & 0x10u) != 0)
    {
        PVOID probe = (PVOID)(ULONG_PTR)address;
        if (!LxRegionAvailable(probe, (SIZE_T)length))
        {
            g_MmapDebug[11] = 0xF6u;
            return (ULONG64)(-12);
        }
    }
    base = (flags & 0x10u) != 0 ? (PVOID)(ULONG_PTR)address : NULL;
    viewSize = (SIZE_T)length;
    if (g_PicoProcessObject != NULL && PsGetCurrentProcess() != g_PicoProcessObject)
    {
        KeStackAttachProcess(g_PicoProcessObject, (PVOID)&apcState);
        attached = TRUE;
    }
    status = ZwAllocateVirtualMemory((HANDLE)g_LastPicoHandle, &base, 0, &viewSize,
                                     MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    g_MmapDebug[6] = (ULONG64)status | 0x300000000ull;
    if (attached)
    {
        KeUnstackDetachProcess((PVOID)&apcState);
    }
    if (!NT_SUCCESS(status) || viewSize < length)
    {
        if (NT_SUCCESS(status))
        {
            SIZE_T freeSize = viewSize;
            PVOID freeBase = base;
            ZwFreeVirtualMemory((HANDLE)g_LastPicoHandle, &freeBase, &freeSize, MEM_RELEASE);
        }
        g_MmapDebug[11] = 0xF7u;
        return (ULONG64)(-12);
    }
    viewBase = (ULONG64)(ULONG_PTR)base;
    g_MmapDebug[8] = viewBase;
    g_MmapDebug[7] = (ULONG64)viewSize;
    {
        ULONG64 copied = LxCopyFileIntoView(g_FdTable[fd].Handle, viewBase, offset, length);
        if ((LONG64)copied < 0)
        {
            g_MmapDebug[11] = 0xF8u;
            return copied;
        }
        g_MmapDebug[9] = copied;
    }
    LxRecordFileView(viewBase, length, fd);
    g_MmapDebug[10] = (ULONG64)g_FileViewCount;
    g_MmapDebug[11] = 0xC0u;
    g_MmapCount++;
    g_LastMmapAddress = viewBase;
    g_LastMmapStatus = 0;
    return viewBase;
}

static ULONG64 LxCopyFileIntoView(HANDLE FileHandle, ULONG64 TargetBase, ULONG64 FileOffset, ULONG64 Length)
{
    UCHAR *staging;
    ULONG64 done = 0;
    ULONG64 stagingSize = 0x10000ull;
    NTSTATUS status;
    if (Length == 0)
    {
        return 0;
    }
    staging = (UCHAR *)ExAllocatePool2(POOL_FLAG_NON_PAGED, (SIZE_T)stagingSize, 'mFxL');
    if (staging == NULL)
    {
        return (ULONG64)(-12);
    }
    while (done < Length)
    {
        IO_STATUS_BLOCK io;
        LARGE_INTEGER readOffset;
        ULONG chunk = (ULONG)((Length - done > stagingSize) ? stagingSize : Length - done);
        ULONG copyIndex;
        RtlZeroMemory(&io, sizeof(io));
        RtlZeroMemory(staging, (SIZE_T)chunk);
        readOffset.QuadPart = (LONGLONG)(FileOffset + done);
        status = ZwReadFile(FileHandle, NULL, NULL, NULL, &io, staging, chunk, &readOffset, NULL);
        if (!NT_SUCCESS(status) || io.Information == 0)
        {
            g_MmapDebug[11] = 0xE1u;
            break;
        }
        if (io.Information > chunk)
        {
            io.Information = chunk;
        }
        for (copyIndex = 0; copyIndex < io.Information; copyIndex += 8)
        {
            ULONG64 address = TargetBase + done + copyIndex;
            ULONG copySize = 8;
            if (copyIndex + copySize > io.Information)
            {
                copySize = io.Information - copyIndex;
            }
            if ((LONG64)LxCopyUserBytes(address, staging + copyIndex, (SIZE_T)copySize, TRUE) < 0)
            {
                g_MmapDebug[11] = 0xE2u;
                g_MmapDebug[8] = address;
                ExFreePoolWithTag(staging, 'mFxL');
                return (ULONG64)(-14);
            }
        }
        done += io.Information;
        if (io.Information < chunk)
        {
            break;
        }
    }
    ExFreePoolWithTag(staging, 'mFxL');
    return done;
}

static BOOLEAN LxRegionAvailable(PVOID Base, SIZE_T Length)
{
    MEMORY_BASIC_INFORMATION info;
    SIZE_T returned = 0;
    ULONG64 cursor = (ULONG64)(ULONG_PTR)Base;
    ULONG64 end = cursor + Length;
    LX_APC_STATE apcState;
    BOOLEAN attached = FALSE;
    NTSTATUS status;
    if (g_LastPicoHandle == 0)
    {
        return FALSE;
    }
    if (g_PicoProcessObject != NULL && PsGetCurrentProcess() != g_PicoProcessObject)
    {
        KeStackAttachProcess(g_PicoProcessObject, (PVOID)&apcState);
        attached = TRUE;
    }
    while (cursor < end)
    {
        RtlZeroMemory(&info, sizeof(info));
        status = ZwQueryVirtualMemory(NtCurrentProcess(), (PVOID)(ULONG_PTR)cursor,
                                      MemoryBasicInformation, &info, sizeof(info), &returned);
        if (!NT_SUCCESS(status))
        {
            if (attached)
            {
                KeUnstackDetachProcess((PVOID)&apcState);
            }
            return FALSE;
        }
        if (info.State == MEM_COMMIT || info.State == MEM_RESERVE)
        {
            if (attached)
            {
                KeUnstackDetachProcess((PVOID)&apcState);
            }
            return FALSE;
        }
        cursor = (ULONG64)(ULONG_PTR)info.BaseAddress + (ULONG64)info.RegionSize;
        if (cursor <= (ULONG64)(ULONG_PTR)info.BaseAddress)
        {
            if (attached)
            {
                KeUnstackDetachProcess((PVOID)&apcState);
            }
            return FALSE;
        }
    }
    if (attached)
    {
        KeUnstackDetachProcess((PVOID)&apcState);
    }
    return TRUE;
}

static ULONG64 LxSysMunmapDecoded(ULONG64 Context)
{
    LX_SYSCALL_REGISTERS registers;
    ULONG64 address;
    ULONG64 length;
    ULONG index;
    if (!NT_SUCCESS(LxDecodeSyscallRegisters(Context, &registers)))
    {
        return (ULONG64)(-14);
    }
    address = registers.Args[0];
    length = registers.Args[1];
    if ((address & 0xFFFull) != 0)
    {
        return (ULONG64)(-22);
    }
    length = (length + 0xFFFull) & ~0xFFFull;
    if (length == 0)
    {
        return (ULONG64)(-22);
    }
    for (index = 0; index < g_FileViewCount; index++)
    {
        LX_FILE_VIEW *view = &g_FileViews[index];
        if (!view->Valid)
        {
            continue;
        }
        if (address >= view->Base && address - view->Base < view->Size &&
            view->Base + view->Size <= address + length)
        {
            view->Valid = FALSE;
        }
    }
    g_MmapDebug[11] = 0xB0u;
    return 0;
}
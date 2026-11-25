typedef struct _LX_MAP_VIEW
{
    ULONG64 Base;
    ULONG64 Size;
    ULONG64 FileOffset;
    LONG Fd;
    BOOLEAN FileBacked;
    BOOLEAN Valid;
} LX_MAP_VIEW;

#define LX_MAX_MAP_VIEWS 32u

static LX_MAP_VIEW g_MapViews[LX_MAX_MAP_VIEWS];
static ULONG g_MapViewCount = 0;

static BOOLEAN LxFindOwnedRegion(ULONG64 Address, ULONG64 Length, ULONG64 *RegionBase, ULONG64 *RegionSize)
{
    ULONG index;
    if (Length == 0)
    {
        return FALSE;
    }
    for (index = 0; index < g_MapViewCount; index++)
    {
        const LX_MAP_VIEW *view = &g_MapViews[index];
        if (!view->Valid || view->Base == 0 || view->Size == 0)
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
        if (RegionBase != NULL)
        {
            *RegionBase = view->Base;
        }
        if (RegionSize != NULL)
        {
            *RegionSize = view->Size;
        }
        return TRUE;
    }
    return FALSE;
}

static VOID LxRecordMapView(ULONG64 Base, ULONG64 Size, ULONG64 FileOffset, LONG Fd, BOOLEAN FileBacked)
{
    ULONG slot;
    if (g_MapViewCount < LX_MAX_MAP_VIEWS)
    {
        slot = g_MapViewCount;
        g_MapViewCount++;
    }
    else
    {
        slot = LX_MAX_MAP_VIEWS - 1;
    }
    g_MapViews[slot].Base = Base;
    g_MapViews[slot].Size = Size;
    g_MapViews[slot].FileOffset = FileOffset;
    g_MapViews[slot].Fd = Fd;
    g_MapViews[slot].FileBacked = FileBacked;
    g_MapViews[slot].Valid = TRUE;
}

static ULONG64 LxWriteByteToUser(ULONG64 Address, UCHAR Value)
{
    __try
    {
        *(volatile UCHAR *)(ULONG_PTR)Address = Value;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return (ULONG64)(-14);
    }
    return 0;
}

static ULONG64 LxProbeCommittedPages(ULONG64 Address, ULONG64 Length)
{
    ULONG64 pages = 0;
    ULONG64 offset;
    if (Length < 4096)
    {
        Length = 4096;
    }
    for (offset = 0; offset < Length; offset += 4096)
    {
        ULONG64 result = LxWriteByteToUser(Address + offset, 0);
        if ((LONG64)result < 0)
        {
            break;
        }
        pages++;
    }
    return pages;
}

static ULONG64 LxLoadFileBytes(HANDLE FileHandle, ULONG64 TargetBase, ULONG64 FileOffset, ULONG64 Length)
{
    UCHAR *staging;
    ULONG64 done = 0;
    ULONG64 stagingSize = 0x40000ull;
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
            g_MmapDebug[6] = (ULONG64)status | 0x10000ull;
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
            __try
            {
                RtlCopyMemory((PVOID)(ULONG_PTR)address, staging + copyIndex, copySize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                g_MmapDebug[6] = 0xF1u;
                g_MmapDebug[8] = done + copyIndex;
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
    g_MmapDebug[9] = done;
    return done;
}

static ULONG64 LxCommitRegion(ULONG64 Address, ULONG64 Length)
{
    PVOID base = (PVOID)(ULONG_PTR)Address;
    SIZE_T size = (SIZE_T)Length;
    NTSTATUS status;
    status = ZwAllocateVirtualMemory((HANDLE)g_LastPicoHandle, &base, 0, &size,
                                     MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    g_MmapDebug[7] = (ULONG64)status;
    if (!NT_SUCCESS(status) || size < Length)
    {
        if (NT_SUCCESS(status))
        {
            ZwFreeVirtualMemory((HANDLE)g_LastPicoHandle, &base, &size, MEM_RELEASE);
        }
        return 0;
    }
    return (ULONG64)(ULONG_PTR)base;
}

static ULONG64 LxZeroUserRange(ULONG64 Address, ULONG64 Length)
{
    ULONG64 done = 0;
    while (done < Length)
    {
        ULONG64 chunk = Length - done > 0x10000ull ? 0x10000ull : Length - done;
        __try
        {
            RtlZeroMemory((PVOID)(ULONG_PTR)(Address + done), (SIZE_T)chunk);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return (ULONG64)(-14);
        }
        done += chunk;
    }
    return 0;
}

static ULONG64 LxSysMmapDecoded(ULONG64 Context)
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
    ULONG64 regionBase = 0;
    ULONG64 regionSize = 0;
    ULONG64 mappedBase;
    ULONG64 probePages;
    BOOLEAN anonymous;
    BOOLEAN fixed;
    NTSTATUS status = LxDecodeSyscallRegisters(Context, &registers);
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
    if (length == 0 || length > 0x40000000ull || (offset & 0xFFFull) != 0)
    {
        return (ULONG64)(-22);
    }
    length = (length + 0xFFFull) & ~0xFFFull;
    fixed = (flags & 0x10u) != 0 ? TRUE : FALSE;
    anonymous = (flags & 0x20u) != 0 ? TRUE : FALSE;
    if (fixed && (address == 0 || (address & 0xFFFull) != 0))
    {
        return (ULONG64)(-22);
    }
    if (!fixed)
    {
        address = 0;
    }
    if (!anonymous && (fd < 0 || fd >= LX_MAX_FD || !g_FdTable[fd].Used || g_FdTable[fd].Handle == NULL))
    {
        return (ULONG64)(-9);
    }
    if (!anonymous && g_FdTable[fd].IsDirectory)
    {
        return (ULONG64)(-21);
    }
    g_MmapDebug[0] = address;
    g_MmapDebug[1] = length;
    g_MmapDebug[2] = protection;
    g_MmapDebug[3] = flags;
    g_MmapDebug[4] = anonymous ? 0xFFFFFFFEull : (ULONG64)(ULONG)fd;
    g_MmapDebug[5] = offset;
    g_MmapDebug[6] = 0xCCu;
    g_MmapDebug[7] = 0xCCu;
    g_MmapDebug[8] = 0;
    g_MmapDebug[9] = 0;
    g_MmapDebug[10] = (ULONG64)g_MapViewCount;
    if (g_PicoProcessObject == NULL || g_LastPicoHandle == 0)
    {
        return (ULONG64)(-12);
    }
    __try
    {
        KeStackAttachProcess(g_PicoProcessObject, (PVOID)&apcState);
        attached = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        attached = FALSE;
    }
    if (fixed && LxFindOwnedRegion(address, length, &regionBase, &regionSize))
    {
        ULONG64 result = 0;
        g_MmapDebug[6] = regionBase;
        g_MmapDebug[7] = 0xDDu;
        if (anonymous)
        {
            result = LxZeroUserRange(address, length);
        }
        else
        {
            ULONG64 copied = LxLoadFileBytes(g_FdTable[fd].Handle, address, offset, length);
            if ((LONG64)copied < 0)
            {
                result = copied;
            }
        }
        if (result == 0)
        {
            LxRecordMapView(address, length, anonymous ? 0 : offset, anonymous ? -1 : fd, !anonymous);
            g_MmapCount++;
            g_LastMmapAddress = address;
            g_LastMmapStatus = 0;
        }
        g_MmapDebug[10] = (ULONG64)g_MapViewCount;
        if (attached)
        {
            KeUnstackDetachProcess((PVOID)&apcState);
        }
        return result == 0 ? address : result;
    }
    mappedBase = LxCommitRegion(address, length);
    if (mappedBase == 0)
    {
        g_MmapDebug[6] = 0xEEu;
        if (attached)
        {
            KeUnstackDetachProcess((PVOID)&apcState);
        }
        return (ULONG64)(-12);
    }
    g_MmapDebug[6] = mappedBase;
    probePages = LxProbeCommittedPages(mappedBase, length);
    g_MmapDebug[8] = probePages;
    g_MmapDebug[11] = length / 4096;
    if (!anonymous)
    {
        ULONG64 copied = LxLoadFileBytes(g_FdTable[fd].Handle, mappedBase, offset, length);
        if ((LONG64)copied < 0)
        {
            if (attached)
            {
                KeUnstackDetachProcess((PVOID)&apcState);
            }
            return copied;
        }
    }
    LxRecordMapView(mappedBase, length, anonymous ? 0 : offset, anonymous ? -1 : fd, !anonymous);
    g_MmapDebug[10] = (ULONG64)g_MapViewCount;
    g_MmapCount++;
    g_LastMmapAddress = mappedBase;
    g_LastMmapStatus = 0;
    if (attached)
    {
        KeUnstackDetachProcess((PVOID)&apcState);
    }
    return mappedBase;
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
    for (index = 0; index < g_MapViewCount; index++)
    {
        LX_MAP_VIEW *view = &g_MapViews[index];
        if (!view->Valid)
        {
            continue;
        }
        if (address >= view->Base && address - view->Base <= view->Size &&
            address + length >= view->Base && view->Base + view->Size >= address + length)
        {
            view->Valid = FALSE;
            g_MmapDebug[10] = (ULONG64)g_MapViewCount;
            return 0;
        }
    }
    return 0;
}

static ULONG64 LxSysAccessDecoded(ULONG64 Context)
{
    LX_SYSCALL_REGISTERS registers;
    CHAR path[260];
    CHAR resolved[260];
    WCHAR ntBuffer[300];
    NTSTATUS status;
    ULONG64 result;
    if (!NT_SUCCESS(LxDecodeSyscallRegisters(Context, &registers)))
    {
        return (ULONG64)(-14);
    }
    result = LxCopyUserPathExact(registers.Args[0], path, sizeof(path));
    if ((LONG64)result < 0)
    {
        return result;
    }
    result = LxResolveAtPath(-100, path, resolved, sizeof(resolved));
    if ((LONG64)result < 0)
    {
        return result;
    }
    if (LxBuildNtPath(resolved, ntBuffer, RTL_NUMBER_OF(ntBuffer)) == 0)
    {
        return (ULONG64)(-36);
    }
    status = LxQueryExists(ntBuffer);
    if (!NT_SUCCESS(status))
    {
        return LxNtStatusToErrno(status);
    }
    return 0;
}
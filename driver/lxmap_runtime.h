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
static ULONG64 g_FileMmapFail[4];

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
        ULONG64 delta;
        if (!view->Valid || view->Fd != Fd || view->Base == 0)
        {
            continue;
        }
        if (Address < view->Base)
        {
            continue;
        }
        delta = Address - view->Base;
        if (delta > view->Size)
        {
            continue;
        }
        if (delta > view->Size - Length)
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
        if (status == STATUS_END_OF_FILE)
        {
            break;
        }
        if (!NT_SUCCESS(status) || io.Information == 0)
        {
            g_FileMmapFail[0] = 0xE1u;
            g_FileMmapFail[1] = (ULONG64)status;
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
                g_FileMmapFail[0] = 0xE2u;
                g_FileMmapFail[1] = address;
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

typedef struct _LX_ELF_PHDR_VIEW
{
    ULONG Type;
    ULONG Flags;
    ULONG64 Offset;
    ULONG64 Vaddr;
    ULONG64 Filesz;
    ULONG64 Memsz;
} LX_ELF_PHDR_VIEW;

static ULONG64 LxMapFileSegments(HANDLE FileHandle, ULONG64 FileSize, ULONG64 ViewBase,
                                 ULONG64 ViewSpan, ULONG64 WholeSize)
{
    UCHAR header[64];
    ULONG64 phoff;
    ULONG phnum;
    ULONG phentsize;
    UCHAR *phdrs;
    ULONG index;
    IO_STATUS_BLOCK io;
    LARGE_INTEGER offset;
    ULONG64 copiedTotal = 0;

    RtlZeroMemory(header, sizeof(header));
    RtlZeroMemory(&io, sizeof(io));
    offset.QuadPart = 0;
    if (!NT_SUCCESS(ZwReadFile(FileHandle, NULL, NULL, NULL, &io, header, sizeof(header), &offset, NULL)) ||
        io.Information < 64)
    {
        return 0;
    }
    if (header[0] != 0x7F || header[1] != 'E' || header[2] != 'L' || header[3] != 'F')
    {
        return 0;
    }
    phoff = *(ULONG64 *)(header + 0x20);
    phentsize = *(USHORT *)(header + 0x36);
    phnum = *(USHORT *)(header + 0x38);
    if (phentsize < 56 || phnum == 0 || phnum > 64 || phoff > FileSize)
    {
        return 0;
    }
    if (phoff + (ULONG64)phnum * phentsize > FileSize)
    {
        return 0;
    }
    phdrs = (UCHAR *)ExAllocatePool2(POOL_FLAG_NON_PAGED, (SIZE_T)(phnum * phentsize), 'pExL');
    if (phdrs == NULL)
    {
        return 0;
    }
    RtlZeroMemory(phdrs, (SIZE_T)(phnum * phentsize));
    RtlZeroMemory(&io, sizeof(io));
    offset.QuadPart = (LONGLONG)phoff;
    if (!NT_SUCCESS(ZwReadFile(FileHandle, NULL, NULL, NULL, &io, phdrs, (ULONG)(phnum * phentsize),
                              &offset, NULL)) ||
        io.Information < phnum * phentsize)
    {
        ExFreePoolWithTag(phdrs, 'pExL');
        return 0;
    }
    for (index = 0; index < phnum; index++)
    {
        UCHAR *ph = phdrs + index * phentsize;
        ULONG type = *(ULONG *)(ph + 0);
        ULONG64 pOffset;
        ULONG64 pVaddr;
        ULONG64 pFilesz;
        ULONG64 limit;
        ULONG64 copied;
        if (type != 1)
        {
            continue;
        }
        pOffset = *(ULONG64 *)(ph + 0x08);
        pVaddr = *(ULONG64 *)(ph + 0x10);
        pFilesz = *(ULONG64 *)(ph + 0x20);
        if (pFilesz == 0 || pOffset >= FileSize)
        {
            continue;
        }
        if (pOffset + pFilesz > FileSize)
        {
            pFilesz = FileSize - pOffset;
        }
        if (pVaddr >= WholeSize)
        {
            continue;
        }
        limit = WholeSize - pVaddr;
        if (pFilesz > limit)
        {
            pFilesz = limit;
        }
        if (pFilesz == 0)
        {
            continue;
        }
        copied = LxCopyFileIntoView(FileHandle, ViewBase + pVaddr, pOffset, pFilesz);
        if ((LONG64)copied < 0)
        {
            ExFreePoolWithTag(phdrs, 'pExL');
            return (ULONG64)(-14);
        }
        copiedTotal += copied;
    }
    ExFreePoolWithTag(phdrs, 'pExL');
    UNREFERENCED_PARAMETER(ViewSpan);
    return copiedTotal;
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
    ULONG64 wholeSize;
    ULONG64 requestBase = 0;
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
        if (g_FileMmapFail[0] == 0)
        {
            g_FileMmapFail[0] = 0xF1u;
            g_FileMmapFail[1] = length;
        }
        g_MmapDebug[11] = 0xF1u;
        return (ULONG64)(-22);
    }
    length = (length + 0xFFFull) & ~0xFFFull;
    if (fd < 0 || fd >= LX_MAX_FD || !g_FdTable[fd].Used || g_FdTable[fd].Handle == NULL)
    {
        if (g_FileMmapFail[0] == 0)
        {
            g_FileMmapFail[0] = 0xF2u;
            g_FileMmapFail[1] = (ULONG64)(ULONG)fd;
        }
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
        requestBase = address;
        if (offset != 0 && requestBase >= offset)
        {
            requestBase = address - offset;
        }
        if (LxFindFileView(fd, requestBase, length, &viewBase))
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
    wholeSize = ((ULONG64)fileInfo.EndOfFile.QuadPart + 0xFFFull) & ~0xFFFull;
    if (wholeSize < offset + length)
    {
        wholeSize = offset + length;
    }
    base = requestBase != 0 ? (PVOID)(ULONG_PTR)requestBase : NULL;
    viewSize = (SIZE_T)wholeSize;
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
    if (!NT_SUCCESS(status) || viewSize < wholeSize)
    {
        if (g_FileMmapFail[0] == 0)
        {
            g_FileMmapFail[0] = (ULONG64)status | 0x100000000ull;
            g_FileMmapFail[1] = requestBase;
            g_FileMmapFail[2] = wholeSize;
            g_FileMmapFail[3] = flags;
        }
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
        ULONG64 copied = LxMapFileSegments(g_FdTable[fd].Handle,
                                          (ULONG64)fileInfo.EndOfFile.QuadPart,
                                          viewBase, wholeSize, wholeSize);
        if ((LONG64)copied < 0)
        {
            g_MmapDebug[11] = 0xF8u;
            return copied;
        }
        if (copied == 0)
        {
            ULONG64 flat = LxCopyFileIntoView(g_FdTable[fd].Handle, viewBase, 0, wholeSize);
            if ((LONG64)flat < 0)
            {
                g_MmapDebug[11] = 0xF9u;
                return flat;
            }
            copied = flat;
        }
        g_MmapDebug[9] = copied;
    }
    LxRecordFileView(viewBase, wholeSize, fd);
    g_MmapDebug[10] = (ULONG64)g_FileViewCount;
    g_MmapDebug[11] = 0xC0u;
    g_MmapCount++;
    g_LastMmapAddress = viewBase + offset;
    g_LastMmapStatus = 0;
    return viewBase + offset;
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
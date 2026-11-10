#include <ntddk.h>
#include <intrin.h>

typedef struct _LX_APC_STATE
{
    UCHAR Reserved[0x40];
} LX_APC_STATE;

typedef struct _LX_FILE_INTERNAL_INFORMATION
{
    LARGE_INTEGER IndexNumber;
} LX_FILE_INTERNAL_INFORMATION;

typedef struct _LX_FILE_BOTH_DIR_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    WCHAR FileName[1];
} LX_FILE_BOTH_DIR_INFORMATION;

#define LX_DEVICE_NAME L"\\Device\\Lxrun"
#define LX_SYMLINK_NAME L"\\DosDevices\\Lxrun"

#define LX_IOCTL_QUERY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define LX_IOCTL_REGISTER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define LX_IOCTL_CREATE_PICO CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define LX_IOCTL_LOAD_ELF CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define LX_IOCTL_CREATE_THREAD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LX_ELF_MAX_SEGMENTS 16

typedef struct _LX_ELF_SEG
{
    ULONG64 Vaddr;
    ULONG64 FileOffset;
    ULONG64 FileSize;
    ULONG64 MemSize;
    ULONG Flags;
} LX_ELF_SEG;

typedef struct _LX_ELF_LOAD
{
    ULONG64 ImageSize;
    ULONG64 MinVaddr;
    ULONG64 EntryPoint;
    ULONG SegmentCount;
    ULONG DataSize;
    ULONG Reserved;
    ULONG64 PhdrAddress;
    ULONG PhdrCount;
    ULONG PhdrEntrySize;
} LX_ELF_LOAD;

#define LX_PICO_INFO_SIZE 0x58
#define LX_PICO_ROUTINES_SIZE 0x60

#define LX_RVA_PSP_PICO_REG_DISABLED 0xC262DBu
#define LX_FLAG_RIP_DISP 0x29E5C0u
#define LX_ROUTINES_RIP_DISP 0x29E5D1u

#define LX_LDR_DLL_BASE 0x30
#define LX_LDR_BASE_NAME 0x58

typedef struct _LX_QUERY_RESULT
{
    ULONG Magic;
    ULONG HasExport;
    ULONG64 ExportAddr;
    ULONG RegisterStatus;
    ULONG DeviceOk;
    ULONG64 NtosBase;
    ULONG OldFlag;
    ULONG RegisterStatus2;
    ULONG Pad0;
    ULONG64 FlagAddr;
    UCHAR FlagNear[16];
    ULONG SizeOfImage;
    ULONG FoundNameLen;
    UCHAR FoundName[16];
    ULONG64 EntryRaw[12];
    ULONG AltFlag4;
    UCHAR AltFlagNear[8];
    UCHAR ThunkBytes[16];
    ULONG64 LockFnAddr;
    ULONG64 UnlockFnAddr;
    UCHAR LockedBytes[8];
    UCHAR CreateBytes[32];
    UCHAR FullBytes[256];
    ULONG64 RoutinesAddr;
    UCHAR RoutinesBytes[96];
    ULONG64 CreateFnAddr;
    UCHAR CreateFnBytes[32];
    ULONG64 PicoHandle;
    ULONG PicoStatus;
    ULONG ParentStatus;
    ULONG ElfStatus;
    ULONG ElfMapStatus;
    ULONG64 ElfBase;
    ULONG64 ElfCopied;
    ULONG64 ElfViewSize;
    ULONG ElfSegStatus[LX_ELF_MAX_SEGMENTS];
    ULONG ThreadStatus;
    ULONG StackMapStatus;
    ULONG64 StackBase;
    ULONG64 ThreadHandle;
    ULONG64 SyscallCount;
    ULONG64 LastSyscallNo;
    ULONG64 SyscallHandler;
    ULONG ResumeStatus;
    ULONG PrevSuspendCount;
    ULONG64 ThreadExitCount;
    ULONG64 ProcessExitCount;
    ULONG64 ExceptionCount;
    ULONG64 ProcessTerminateCount;
    ULONG64 ContextDump[64];
    ULONG64 ArgsDump[8];
    ULONG64 SyscallHistory[32];
    ULONG SyscallHistoryCount;
    ULONG PoolStatus;
    ULONG64 PoolBase;
    ULONG64 BrkCurrent;
    ULONG64 MmapCurrent;
    UCHAR WriteBuffer[512];
    ULONG WriteLength;
    ULONG WriteCount;
    ULONG ExitCalled;
    ULONG Pad1;
    ULONG64 ExitCode;
    ULONG64 LastSyscallResult;
    ULONG StackSetupStatus;
    ULONG Pad2;
    ULONG64 ExceptionDump[8];
    ULONG64 FsBaseAfter;
    ULONG ArchPrctlCalls;
    ULONG64 WriteDebug[8];
    ULONG WriteStatus;
    ULONG64 WriteContext[64];
    ULONG WriteFd;
    ULONG64 ContextRdx;
    ULONG GetContextStatus;
    ULONG OpenCount;
    ULONG LastOpenFd;
    ULONG64 LastOpenStatus;
    CHAR LastOpenPath[128];
    ULONG64 ReadCount;
    ULONG64 LastReadStatus;
    CHAR LastNtPath[260];
    ULONG SelfTestSystemRoot;
    ULONG SelfTestGlobal;
    ULONG SelfTestDosDevices;
    ULONG64 ReadDebug[8];
    ULONG GetDentsCount;
    ULONG64 GetDentsStatus;
    ULONG64 GetDentsBytes;
    CHAR LastDirectoryPath[260];
    ULONG64 StatCount;
    ULONG64 LastStatStatus;
    LONG64 LastStatSize;
    ULONG LastStatMode;
    CHAR LastStatPath[128];
    ULONG64 StatDebug[8];
} LX_QUERY_RESULT;

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD LxrunUnload;

static PDEVICE_OBJECT g_DeviceObject = NULL;
static PVOID g_PsRegisterPicoProvider = NULL;
static ULONG64 g_NtosBase = 0;
static ULONG g_NtosFoundNameLen = 0;
static UCHAR g_NtosFoundName[16];
static ULONG64 g_NtosEntryRaw[12];
static ULONG g_RegisterStatus = 0xFFFFFFFFu;
static ULONG g_RegisterStatus2 = 0xFFFFFFFFu;
static ULONG g_OldFlag = 0xFFFFFFFFu;
static ULONG64 g_FlagAddr = 0;
static ULONG64 g_CreatePicoFn = 0;
static ULONG64 g_LastPicoHandle = 0;
static ULONG g_LastPicoStatus = 0xFFFFFFFFu;
static ULONG g_ParentHandleStatus = 0xFFFFFFFFu;
static ULONG64 g_ElfBase = 0;
static ULONG g_ElfStatus = 0xFFFFFFFFu;
static ULONG g_ElfMapStatus = 0xFFFFFFFFu;
static ULONG g_ElfSegStatus[LX_ELF_MAX_SEGMENTS];
static ULONG64 g_ElfCopiedTotal = 0;
static ULONG64 g_ElfViewSize = 0;
static volatile ULONG64 g_SyscallCount = 0;
static volatile ULONG64 g_LastContext = 0;
static volatile ULONG64 g_LastSyscallNo = 0;
static volatile ULONG64 g_SyscallHandlerAddr = 0;
static ULONG64 g_PicoContext = 0;
static ULONG64 g_ElfEntry = 0;
static LX_ELF_LOAD g_ElfLoad;
static ULONG64 g_CreatePicoThreadFn = 0;
static ULONG64 g_StackBase = 0;
static ULONG g_ThreadStatus = 0xFFFFFFFFu;
static ULONG g_StackMapStatus = 0xFFFFFFFFu;
static ULONG64 g_ThreadHandle = 0;
static ULONG64 g_ResumeThreadFn = 0;
static ULONG g_ResumeStatus = 0xFFFFFFFFu;
static ULONG g_PrevSuspendCount = 0xFFFFFFFFu;
static volatile ULONG64 g_ThreadExitCount = 0;
static volatile ULONG64 g_ProcessExitCount = 0;
static volatile ULONG64 g_ExceptionCount = 0;
static volatile ULONG64 g_ProcessTerminateCount = 0;
static ULONG64 g_ContextDump[64];
static ULONG64 g_ArgsDump[8];
static ULONG64 g_SyscallHistory[32];
static ULONG g_SyscallHistoryCount = 0;

#define LX_POOL_SIZE 0x01000000ull
#define LX_PARAM_BASE 0x140

static ULONG g_PoolStatus = 0xFFFFFFFFu;
static ULONG64 g_PoolBase = 0;
static ULONG64 g_BrkCurrent = 0;
static ULONG64 g_MmapCurrent = 0;
static UCHAR g_WriteBuffer[512];
static ULONG g_WriteLength = 0;
static ULONG g_WriteCount = 0;
static ULONG64 g_ExitCode = 0;
static ULONG g_ExitCalled = 0;
static ULONG64 g_LastSyscallResult = 0;
static ULONG64 g_SetDescriptorBaseFn = 0;
static ULONG64 g_FsBaseAfter = 0;
static ULONG g_ArchPrctlCalls = 0;
static ULONG64 g_WriteDebug[8];
static ULONG g_WriteStatus = 0xFFFFFFFFu;
static ULONG g_WriteFd = 0xFFFFFFFFu;
static ULONG64 g_GetContextFn = 0;
static ULONG64 g_ContextRdx = 0;
static ULONG g_GetContextStatus = 0xFFFFFFFFu;

#define LX_SYS_READ 0
#define LX_SYS_WRITE 1
#define LX_SYS_MMAP 9
#define LX_SYS_BRK 12
#define LX_SYS_EXIT 60
#define LX_SYS_ARCH_PRCTL 158
#define LX_SYS_SET_TID_ADDRESS 218
#define LX_SYS_EXIT_GROUP 231
#define LX_SYS_OPEN 2
#define LX_SYS_CLOSE 3
#define LX_SYS_STAT 4
#define LX_SYS_FSTAT 5
#define LX_SYS_LSEEK 8
#define LX_SYS_IOCTL 16
#define LX_SYS_ACCESS 33
#define LX_SYS_OPENAT 257

#define LX_MAX_FD 64

typedef struct _LX_FD_ENTRY
{
    HANDLE Handle;
    ULONG OpenFlags;
    ULONG64 Offset;
    ULONG64 DirectoryCursor;
    CHAR LinuxPath[260];
    BOOLEAN Used;
    BOOLEAN IsDirectory;
} LX_FD_ENTRY;

typedef struct _LX_LINUX_DIRENT64
{
    ULONG64 Inode;
    LONG64 Offset;
    USHORT RecordLength;
    UCHAR Type;
    CHAR Name[1];
} LX_LINUX_DIRENT64;

#define LX_SYS_NEWFSTATAT 262
#define LX_SYS_GETDENTS64 217
#define LX_SYS_STATX 332
#define LX_SYS_AT_FDCWD 0xFFFFFF9Cu
#define LX_O_ACCMODE 3u
#define LX_O_CREAT 0x40u
#define LX_O_EXCL 0x80u
#define LX_O_TRUNC 0x200u
#define LX_O_DIRECTORY 0x10000u
#define LX_AT_FDCWD ((ULONG64)0xFFFFFFFFFFFFFF9Cull)

static LX_FD_ENTRY g_FdTable[LX_MAX_FD];
static ULONG g_OpenCount = 0;
static ULONG64 g_LastOpenStatus = 0xFFFFFFFFu;
static CHAR g_LastOpenPath[128];
static ULONG g_LastOpenFd = 0xFFFFFFFFu;
static ULONG64 g_ReadCount = 0;
static ULONG64 g_LastReadStatus = 0xFFFFFFFFu;
static CHAR g_LastNtPath[260];
static ULONG g_SelfTestSystemRoot = 0xFFFFFFFFu;
static ULONG g_SelfTestGlobal = 0xFFFFFFFFu;
static ULONG g_SelfTestDosDevices = 0xFFFFFFFFu;
static ULONG64 g_ReadDebug[8];
static ULONG64 g_StatDebug[8];
static ULONG g_GetDentsCount = 0;
static ULONG64 g_GetDentsStatus = 0xFFFFFFFFu;
static ULONG64 g_GetDentsBytes = 0;
static CHAR g_LastDirectoryPath[260];
static ULONG64 g_StatCount = 0;
static ULONG64 g_LastStatStatus = 0xFFFFFFFFu;
static LONG64 g_LastStatSize = 0;
static ULONG g_LastStatMode = 0;
static CHAR g_LastStatPath[128];

static ULONG64 g_WriteContext[64];
static ULONG g_WriteContextDone = 0;

typedef struct _LX_LINUX_STAT
{
    ULONG64 Device;
    ULONG64 Inode;
    ULONG64 LinkCount;
    ULONG Mode;
    ULONG UserId;
    ULONG GroupId;
    ULONG Pad0;
    ULONG64 DeviceType;
    LONG64 Size;
    LONG64 BlockSize;
    LONG64 Blocks;
    LONG64 AccessSeconds;
    ULONG64 AccessNanoseconds;
    LONG64 ModifySeconds;
    ULONG64 ModifyNanoseconds;
    LONG64 ChangeSeconds;
    ULONG64 ChangeNanoseconds;
    ULONG64 Reserved[3];
} LX_LINUX_STAT;

static LONG64 LxWindowsTimeToUnix(LARGE_INTEGER TimeValue)
{
    const LONGLONG EpochDelta = 116444736000000000LL;
    LONGLONG value = TimeValue.QuadPart;

    if (value < EpochDelta)
    {
        return 0;
    }

    return (value - EpochDelta) / 10000000LL;
}

static ULONG64 LxNtStatusToErrno(NTSTATUS Status)
{
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND || Status == STATUS_OBJECT_PATH_NOT_FOUND)
    {
        return (ULONG64)(-2);
    }
    if (Status == STATUS_ACCESS_DENIED)
    {
        return (ULONG64)(-13);
    }
    if (Status == STATUS_INVALID_HANDLE)
    {
        return (ULONG64)(-9);
    }
    if (Status == STATUS_SHARING_VIOLATION)
    {
        return (ULONG64)(-16);
    }
    if (Status == STATUS_DISK_FULL)
    {
        return (ULONG64)(-28);
    }
    if (Status == STATUS_NO_MEMORY)
    {
        return (ULONG64)(-12);
    }
    if (Status == STATUS_INVALID_PARAMETER || Status == STATUS_BUFFER_TOO_SMALL)
    {
        return (ULONG64)(-22);
    }
    if (Status == STATUS_NOT_A_DIRECTORY)
    {
        return (ULONG64)(-20);
    }
    if (Status == STATUS_FILE_IS_A_DIRECTORY)
    {
        return (ULONG64)(-21);
    }
    if (Status == STATUS_OBJECT_NAME_COLLISION)
    {
        return (ULONG64)(-17);
    }

    return (ULONG64)(-5);
}
static ULONG LxCopyLinuxPath(ULONG64 UserPath, CHAR *KernelPath, ULONG Capacity)
{
    ULONG i;

    if (UserPath == 0 || KernelPath == NULL || Capacity < 2)
    {
        return 0;
    }

    KernelPath[0] = 0;

    __try
    {
        ProbeForRead((PVOID)(ULONG_PTR)UserPath, 1, 1);

        for (i = 0; i < Capacity - 1 && i < 200; i++)
        {
            CHAR value = ((const CHAR *)(ULONG_PTR)UserPath)[i];

            if (value == 0)
            {
                return i;
            }

            if (value < 0x20 || value > 0x7E)
            {
                KernelPath[0] = 0;
                return 0;
            }

            KernelPath[i] = value;
        }

        KernelPath[0] = 0;
        return 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KernelPath[0] = 0;
        return 0;
    }
}

static ULONG64 LxFillStat(HANDLE Handle, LX_LINUX_STAT *LinuxStat)
{
    FILE_STANDARD_INFORMATION standardInfo;
    FILE_BASIC_INFORMATION basicInfo;
    LX_FILE_INTERNAL_INFORMATION internalInfo;
    IO_STATUS_BLOCK statusBlock;
    NTSTATUS status;
    ULONG attributes;
    LONGLONG accessTime;
    LONGLONG modifyTime;
    LONGLONG changeTime;

    RtlZeroMemory(LinuxStat, sizeof(*LinuxStat));

    status = ZwQueryInformationFile(Handle, &statusBlock, &standardInfo, sizeof(standardInfo), FileStandardInformation);
    if (!NT_SUCCESS(status))
    {
        return LxNtStatusToErrno(status);
    }

    status = ZwQueryInformationFile(Handle, &statusBlock, &basicInfo, sizeof(basicInfo), FileBasicInformation);
    if (!NT_SUCCESS(status))
    {
        return LxNtStatusToErrno(status);
    }

    status = ZwQueryInformationFile(Handle, &statusBlock, &internalInfo, sizeof(internalInfo), FileInternalInformation);
    if (NT_SUCCESS(status))
    {
        LinuxStat->Inode = (ULONG64)internalInfo.IndexNumber.QuadPart;
    }

    attributes = basicInfo.FileAttributes;
    accessTime = basicInfo.LastAccessTime.QuadPart - 116444736000000000LL;
    modifyTime = basicInfo.LastWriteTime.QuadPart - 116444736000000000LL;
    changeTime = basicInfo.ChangeTime.QuadPart - 116444736000000000LL;

    LinuxStat->Device = 1;
    LinuxStat->LinkCount = standardInfo.NumberOfLinks;
    LinuxStat->Mode = standardInfo.Directory ? (0x4000u | 0555u) : (0x8000u | 0444u);
    if ((attributes & FILE_ATTRIBUTE_READONLY) == 0)
    {
        LinuxStat->Mode |= 0222u;
    }
    LinuxStat->Size = standardInfo.EndOfFile.QuadPart;
    LinuxStat->BlockSize = 4096;
    LinuxStat->Blocks = (standardInfo.AllocationSize.QuadPart + 511) / 512;
    LinuxStat->AccessSeconds = LxWindowsTimeToUnix(basicInfo.LastAccessTime);
    LinuxStat->AccessNanoseconds = accessTime >= 0 ? (ULONG64)(accessTime % 10000000LL) * 100 : 0;
    LinuxStat->ModifySeconds = LxWindowsTimeToUnix(basicInfo.LastWriteTime);
    LinuxStat->ModifyNanoseconds = modifyTime >= 0 ? (ULONG64)(modifyTime % 10000000LL) * 100 : 0;
    LinuxStat->ChangeSeconds = LxWindowsTimeToUnix(basicInfo.ChangeTime);
    LinuxStat->ChangeNanoseconds = changeTime >= 0 ? (ULONG64)(changeTime % 10000000LL) * 100 : 0;

    return 0;
}

static ULONG LxBuildNtPath(const char *LinuxPath, WCHAR *OutBuffer, ULONG OutChars)
{
    static const WCHAR PrefixSlash[] = {L'\\', L'?', L'?', L'\\', 0};
    static const WCHAR LxRoot[] = {L'\\', L'S', L'y', L's', L't', L'e', L'm', L'R', L'o', L'o', L't', L'\\', L'l', L'x', L'r', L'o', L'o', L't', 0};
    static const WCHAR DrivePrefix[] = {L'\\', L'S', L'y', L's', L't', L'e', L'm', L'R', L'o', L'o', L't', 0};
    ULONG index = 0;
    ULONG out = 0;
    BOOLEAN isMntC = FALSE;
    ULONG prefixLength;

    if (LinuxPath == NULL || OutBuffer == NULL || OutChars < 8)
    {
        return 0;
    }

    if (LinuxPath[0] == '/' && LinuxPath[1] == 'm' && LinuxPath[2] == 'n' && LinuxPath[3] == 't' &&
        LinuxPath[4] == '/' && LinuxPath[5] == 'c' && (LinuxPath[6] == '/' || LinuxPath[6] == 0))
    {
        isMntC = TRUE;
        index = 6;
        prefixLength = 0;
        while (DrivePrefix[prefixLength] != 0)
        {
            OutBuffer[out++] = DrivePrefix[prefixLength++];
        }
    }
    else
    {
        prefixLength = 0;
        while (LxRoot[prefixLength] != 0)
        {
            OutBuffer[out++] = LxRoot[prefixLength++];
        }
    }

    (void)PrefixSlash;

    while (LinuxPath[index] != 0 && out + 4 < OutChars)
    {
        CHAR ch = LinuxPath[index++];

        if (ch == '/')
        {
            OutBuffer[out++] = L'\\';
        }
        else
        {
            OutBuffer[out++] = (WCHAR)(UCHAR)ch;
        }
    }

    if (out == 0 || (isMntC && out <= 11))
    {
        return 0;
    }

    OutBuffer[out] = 0;
    return out;
}

static ULONG64 LxOpenFile(const char *LinuxPath, ULONG LinuxFlags, ULONG Mode)
{
    WCHAR ntPathBuffer[260];
    UNICODE_STRING ntPath;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK statusBlock;
    NTSTATUS status;
    ULONG pathLength;
    ULONG slot;
    ULONG accessMode = LinuxFlags & LX_O_ACCMODE;
    ACCESS_MASK access;
    ULONG share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    ULONG disposition = FILE_OPEN;
    ULONG copyLength;
    ULONG create = (LinuxFlags & LX_O_CREAT) != 0;
    ULONG truncate = (LinuxFlags & LX_O_TRUNC) != 0;
    ULONG options = FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT;

    UNREFERENCED_PARAMETER(Mode);

    pathLength = LxBuildNtPath(LinuxPath, ntPathBuffer, RTL_NUMBER_OF(ntPathBuffer));
    if (pathLength == 0)
    {
        g_LastOpenStatus = STATUS_INVALID_PARAMETER;
        return (ULONG64)(-22);
    }

    for (copyLength = 0; copyLength < sizeof(g_LastOpenPath) - 1 && LinuxPath[copyLength] != 0; copyLength++)
    {
        g_LastOpenPath[copyLength] = LinuxPath[copyLength];
    }
    g_LastOpenPath[copyLength] = 0;

    if (accessMode == 0)
    {
        access = GENERIC_READ | FILE_READ_ATTRIBUTES | SYNCHRONIZE;
    }
    else if (accessMode == 1)
    {
        access = GENERIC_WRITE | FILE_READ_ATTRIBUTES | SYNCHRONIZE;
    }
    else if (accessMode == 2)
    {
        access = GENERIC_READ | GENERIC_WRITE | FILE_READ_ATTRIBUTES | SYNCHRONIZE;
    }
    else
    {
        g_LastOpenStatus = STATUS_INVALID_PARAMETER;
        return (ULONG64)(-22);
    }

    if (create && (LinuxFlags & LX_O_EXCL) != 0)
    {
        disposition = FILE_CREATE;
    }
    else if (create)
    {
        disposition = FILE_OPEN_IF;
    }
    else if (truncate)
    {
        disposition = FILE_OVERWRITE;
        if (accessMode == 0)
        {
            g_LastOpenStatus = STATUS_ACCESS_DENIED;
            return (ULONG64)(-13);
        }
    }

    if (create && truncate)
    {
        disposition = FILE_OVERWRITE_IF;
    }

    if ((LinuxFlags & LX_O_DIRECTORY) != 0)
    {
        options |= FILE_DIRECTORY_FILE;
    }

    ntPath.Buffer = ntPathBuffer;
    ntPath.Length = (USHORT)(pathLength * sizeof(WCHAR));
    ntPath.MaximumLength = (USHORT)((pathLength + 1) * sizeof(WCHAR));

    {
        ULONG k;

        for (k = 0; k < RTL_NUMBER_OF(g_LastNtPath) - 1 && ntPathBuffer[k] != 0; k++)
        {
            g_LastNtPath[k] = (CHAR)ntPathBuffer[k];
        }
        g_LastNtPath[k] = 0;
    }

    InitializeObjectAttributes(&attributes, &ntPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    for (slot = 3; slot < LX_MAX_FD; slot++)
    {
        if (!g_FdTable[slot].Used)
        {
            break;
        }
    }

    if (slot >= LX_MAX_FD)
    {
        g_LastOpenStatus = STATUS_TOO_MANY_OPENED_FILES;
        return (ULONG64)(-24);
    }

    g_FdTable[slot].Handle = NULL;
    status = ZwCreateFile(&g_FdTable[slot].Handle, access, &attributes, &statusBlock, NULL, FILE_ATTRIBUTE_NORMAL,
                          share, disposition, options, NULL, 0);
    g_OpenCount++;
    g_LastOpenStatus = (ULONG)status;

    if (!NT_SUCCESS(status))
    {
        g_FdTable[slot].Handle = NULL;
        g_FdTable[slot].Used = FALSE;
        g_LastOpenFd = 0xFFFFFFFFu;
        return LxNtStatusToErrno(status);
    }

    {
        FILE_STANDARD_INFORMATION standardInfo;
        NTSTATUS queryStatus = ZwQueryInformationFile(g_FdTable[slot].Handle, &statusBlock, &standardInfo,
                                                      sizeof(standardInfo), FileStandardInformation);
        g_FdTable[slot].IsDirectory = NT_SUCCESS(queryStatus) && standardInfo.Directory;
    }

    g_FdTable[slot].Used = TRUE;
    g_FdTable[slot].OpenFlags = LinuxFlags;
    g_FdTable[slot].Offset = 0;
    g_FdTable[slot].DirectoryCursor = 0;
    RtlZeroMemory(g_FdTable[slot].LinuxPath, sizeof(g_FdTable[slot].LinuxPath));
    {
        ULONG pathCopy = LxKernelPathLength(LinuxPath);

        if (pathCopy > sizeof(g_FdTable[slot].LinuxPath) - 1)
        {
            pathCopy = sizeof(g_FdTable[slot].LinuxPath) - 1;
        }
        RtlCopyMemory(g_FdTable[slot].LinuxPath, LinuxPath, pathCopy);
    }
    g_LastOpenFd = slot;

    return slot;
}

static ULONG64 LxReadFile(ULONG Fd, PVOID UserBuffer, ULONG Count)
{
    IO_STATUS_BLOCK statusBlock;
    NTSTATUS status;
    LARGE_INTEGER offset;

    if (Fd >= LX_MAX_FD || !g_FdTable[Fd].Used || UserBuffer == NULL || Count == 0)
    {
        return (ULONG64)(-9);
    }

    offset.QuadPart = (LONGLONG)g_FdTable[Fd].Offset;

    status = ZwReadFile(g_FdTable[Fd].Handle, NULL, NULL, NULL, &statusBlock, UserBuffer, Count, &offset, NULL);

    g_ReadCount++;
    g_LastReadStatus = (ULONG)status;

    if (status == STATUS_END_OF_FILE)
    {
        return 0;
    }

    if (!NT_SUCCESS(status))
    {
        return (ULONG64)(-(LONG)(status & 0xFFFF));
    }

    g_FdTable[Fd].Offset += statusBlock.Information;

    return statusBlock.Information;
}

static ULONG LxKernelPathLength(const char *LinuxPath)
{
    ULONG length = 0;

    while (length < 259 && LinuxPath[length] != 0)
    {
        length++;
    }

    return length;
}

static ULONG64 LxGetDents64(ULONG Fd, PVOID UserBuffer, ULONG Count)
{
    LX_FILE_BOTH_DIR_INFORMATION *info;
    IO_STATUS_BLOCK statusBlock;
    NTSTATUS status;
    ULONG bufferSize = 0x4000;
    PVOID buffer;
    ULONG written = 0;
    ULONG restart = 0;
    WCHAR ntPathBuffer[260];
    UNICODE_STRING directoryName;
    OBJECT_ATTRIBUTES attributes;
    HANDLE directoryHandle = NULL;
    ULONG pathLength;

    if (Fd >= LX_MAX_FD || !g_FdTable[Fd].Used || UserBuffer == NULL || Count == 0)
    {
        return (ULONG64)(-9);
    }

    g_GetDentsCount++;

    if (!g_FdTable[Fd].IsDirectory)
    {
        g_GetDentsStatus = 0xC0000103u;
        return (ULONG64)(-20);
    }

    pathLength = LxBuildNtPath(g_FdTable[Fd].LinuxPath, ntPathBuffer, RTL_NUMBER_OF(ntPathBuffer));
    if (pathLength == 0)
    {
        g_GetDentsStatus = 0xC0000037u;
        return (ULONG64)(-22);
    }

    directoryName.Buffer = ntPathBuffer;
    directoryName.Length = (USHORT)(pathLength * sizeof(WCHAR));
    directoryName.MaximumLength = (USHORT)((pathLength + 1) * sizeof(WCHAR));

    InitializeObjectAttributes(&attributes, &directoryName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    g_LastDirectoryPath[0] = 0;
    {
        ULONG k;

        for (k = 0; k < RTL_NUMBER_OF(g_LastDirectoryPath) - 1 && ntPathBuffer[k] != 0; k++)
        {
            g_LastDirectoryPath[k] = (CHAR)ntPathBuffer[k];
        }
        g_LastDirectoryPath[k] = 0;
    }

    status = ZwCreateFile(&directoryHandle, FILE_LIST_DIRECTORY | SYNCHRONIZE, &attributes, &statusBlock, NULL,
                          FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE, NULL, 0);
    g_GetDentsStatus = (ULONG)status;

    if (!NT_SUCCESS(status))
    {
        return LxNtStatusToErrno(status);
    }

    buffer = ExAllocatePoolWithTag(NonPagedPool, bufferSize, 'rDxL');
    if (buffer == NULL)
    {
        ZwClose(directoryHandle);
        g_GetDentsStatus = 0xC000009Au;
        return (ULONG64)(-12);
    }

    if (g_FdTable[Fd].DirectoryCursor != 0)
    {
        restart = 1;
    }

    status = ZwQueryDirectoryFile(directoryHandle, NULL, NULL, NULL, &statusBlock, buffer, bufferSize,
                                  FileBothDirectoryInformation, FALSE, NULL, restart);

    if (status == STATUS_NO_MORE_FILES || status == STATUS_END_OF_FILE)
    {
        status = STATUS_NO_MORE_FILES;
    }
    else if (NT_SUCCESS(status))
    {
        ULONG skip = 0;
        ULONG index = 0;

        info = (LX_FILE_BOTH_DIR_INFORMATION *)buffer;

        for (;;)
        {
            index++;

            if (index >= g_FdTable[Fd].DirectoryCursor)
            {
                ULONG nameLength = info->FileNameLength / sizeof(WCHAR);
                ULONG recordLength;
                ULONG m;
                BOOLEAN isDirectory = (info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                LX_LINUX_DIRENT64 *entry = (LX_LINUX_DIRENT64 *)((PUCHAR)(ULONG_PTR)UserBuffer + written);

                recordLength = (ULONG)(19 + nameLength + 1);
                recordLength = (recordLength + 7) & ~7u;

                if (written + recordLength > Count || nameLength > 255)
                {
                    break;
                }

                __try
                {
                    entry->Inode = (ULONG64)((index + 1) * 2);
                    entry->Offset = written + recordLength;
                    entry->RecordLength = (USHORT)recordLength;
                    entry->Type = isDirectory ? 4 : 8;

                    for (m = 0; m < nameLength; m++)
                    {
                        entry->Name[m] = (CHAR)(UCHAR)info->FileName[m];
                    }
                    entry->Name[nameLength] = 0;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    ExFreePoolWithTag(buffer, 'rDxL');
                    ZwClose(directoryHandle);
                    g_GetDentsStatus = 0xC0000005u;
                    return (ULONG64)(-14);
                }

                written += recordLength;
            }

            if (info->NextEntryOffset == 0)
            {
                break;
            }

            info = (LX_FILE_BOTH_DIR_INFORMATION *)((PUCHAR)info + info->NextEntryOffset);
        }

        g_FdTable[Fd].DirectoryCursor = (ULONG64)index;
    }

    ExFreePoolWithTag(buffer, 'rDxL');
    ZwClose(directoryHandle);

    g_GetDentsStatus = (ULONG)status;
    g_GetDentsBytes = written;

    if (written == 0 && !NT_SUCCESS(status))
    {
        return 0;
    }

    return written;
}

static ULONG64 LxCloseFile(ULONG Fd)
{
    if (Fd >= LX_MAX_FD || !g_FdTable[Fd].Used)
    {
        return (ULONG64)(-9);
    }

    ZwClose(g_FdTable[Fd].Handle);
    g_FdTable[Fd].Handle = NULL;
    g_FdTable[Fd].Used = FALSE;

    return 0;
}

static ULONG64 LxHandleSyscall(ULONG64 Number, ULONG64 *Parameters, ULONG64 Context)
{
    ULONG64 arg0 = Parameters[0];
    ULONG64 arg1 = Parameters[1];
    ULONG64 arg2 = Parameters[2];

    switch (Number)
    {
    case LX_SYS_OPEN:
    case LX_SYS_OPENAT:
    {
        ULONG64 pathPointer = Parameters[1];
        ULONG64 openFlags = Parameters[2];
        ULONG64 modeBits = Parameters[3];
        ULONG64 result;

        if (pathPointer == 0)
        {
            return (ULONG64)(-14);
        }

        __try
        {
            ProbeForRead((PVOID)(ULONG_PTR)pathPointer, 1, 1);
            result = LxOpenFile((const char *)(ULONG_PTR)pathPointer, (ULONG)openFlags, (ULONG)modeBits);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            result = (ULONG64)(-14);
        }

        return result;
    }

    case LX_SYS_CLOSE:
        return LxCloseFile((ULONG)Parameters[1]);

    case LX_SYS_STAT:
    {
        CHAR kernelPath[260];
        ULONG64 length = LxCopyLinuxPath(arg0, kernelPath, sizeof(kernelPath));
        ULONG64 status;
        LX_LINUX_STAT linuxStat;
        HANDLE fileHandle = NULL;
        WCHAR ntPathBuffer[260];
        UNICODE_STRING ntPath;
        OBJECT_ATTRIBUTES attributes;
        IO_STATUS_BLOCK statusBlock;
        NTSTATUS ntStatus;
        ULONG pathLength;

        g_StatCount++;
        g_LastStatPath[0] = 0;

        g_StatDebug[0] = Parameters[0];
        g_StatDebug[1] = Parameters[1];
        g_StatDebug[2] = Parameters[2];
        g_StatDebug[3] = Parameters[3];
        g_StatDebug[4] = Parameters[4];
        g_StatDebug[5] = Parameters[5];

        if (length == 0)
        {
            ULONG s;

            for (s = 0; s < 6; s++)
            {
                ULONG probe;

                if (Parameters[s] < 0x400000ull || Parameters[s] >= 0x800000000000ull)
                {
                    continue;
                }

                if (Parameters[s] == arg1)
                {
                    continue;
                }

                probe = LxCopyLinuxPath(Parameters[s], kernelPath, sizeof(kernelPath));

                if (probe > 0 && kernelPath[0] == '/')
                {
                    length = probe;
                    g_StatDebug[6] = Parameters[s];
                    break;
                }
            }
        }
        {
            g_LastStatStatus = 0xFFFFFFFFu;
            return (ULONG64)(-14);
        }

        {
            ULONG k;

            for (k = 0; k < RTL_NUMBER_OF(g_LastStatPath) - 1 && kernelPath[k] != 0; k++)
            {
                g_LastStatPath[k] = kernelPath[k];
            }
            g_LastStatPath[k] = 0;
        }

        pathLength = LxBuildNtPath(kernelPath, ntPathBuffer, RTL_NUMBER_OF(ntPathBuffer));
        if (pathLength == 0)
        {
            g_LastStatStatus = 0xC000000Du;
            return (ULONG64)(-22);
        }

        ntPath.Buffer = ntPathBuffer;
        ntPath.Length = (USHORT)(pathLength * sizeof(WCHAR));
        ntPath.MaximumLength = (USHORT)((pathLength + 1) * sizeof(WCHAR));
        InitializeObjectAttributes(&attributes, &ntPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

        ntStatus = ZwCreateFile(&fileHandle, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attributes, &statusBlock, NULL,
                                FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT, NULL, 0);

        g_LastStatStatus = (ULONG)ntStatus;
        if (!NT_SUCCESS(ntStatus))
        {
            return LxNtStatusToErrno(ntStatus);
        }

        status = LxFillStat(fileHandle, &linuxStat);
        ZwClose(fileHandle);

        if (status != 0)
        {
            return status;
        }

        g_LastStatSize = linuxStat.Size;
        g_LastStatMode = linuxStat.Mode;

        if (arg1 == 0)
        {
            return (ULONG64)(-14);
        }

        __try
        {
            ProbeForWrite((PVOID)(ULONG_PTR)arg1, sizeof(linuxStat), 4);
            RtlCopyMemory((PVOID)(ULONG_PTR)arg1, &linuxStat, sizeof(linuxStat));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return (ULONG64)(-14);
        }

        return 0;
    }

    case LX_SYS_FSTAT:
    {
        ULONG fd = (ULONG)arg0;
        LX_LINUX_STAT linuxStat;
        ULONG64 status;

        g_StatCount++;

        if (fd >= LX_MAX_FD || !g_FdTable[fd].Used)
        {
            g_LastStatStatus = 0xC0000008u;
            return (ULONG64)(-9);
        }

        status = LxFillStat(g_FdTable[fd].Handle, &linuxStat);
        g_LastStatStatus = 0;
        if (status != 0)
        {
            return status;
        }

        g_LastStatSize = linuxStat.Size;
        g_LastStatMode = linuxStat.Mode;

        __try
        {
            ProbeForWrite((PVOID)(ULONG_PTR)arg1, sizeof(linuxStat), 4);
            RtlCopyMemory((PVOID)(ULONG_PTR)arg1, &linuxStat, sizeof(linuxStat));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return (ULONG64)(-14);
        }

        return 0;
    }

    case LX_SYS_GETDENTS64:
    {
        ULONG fd = (ULONG)arg0;
        ULONG count = (ULONG)arg2;

        if (count > 0x4000)
        {
            count = 0x4000;
        }

        return LxGetDents64(fd, (PVOID)(ULONG_PTR)arg1, count);
    }

    case LX_SYS_ACCESS:
        return 0;

    case LX_SYS_LSEEK:
    case LX_SYS_IOCTL:
        return 0;

    case LX_SYS_READ:
    {
        ULONG64 values[6];
        ULONG64 fd = 0;
        ULONG64 buffer = 0;
        ULONG64 count = 0;
        ULONG i;
        BOOLEAN haveBuffer = FALSE;

        for (i = 0; i < 6; i++)
        {
            values[i] = Parameters[i];
        }

        g_ReadDebug[0] = values[0];
        g_ReadDebug[1] = values[1];
        g_ReadDebug[2] = values[2];
        g_ReadDebug[3] = values[3];

        for (i = 0; i < 6; i++)
        {
            if (!haveBuffer &&
                ((values[i] >= 0x400000ull && values[i] < 0x520000ull) ||
                 (values[i] >= 0x7fff00000000ull && values[i] < 0x800000000000ull) ||
                 (values[i] >= 0x100000000ull && values[i] < 0x101000000ull) ||
                 (values[i] >= 0x10000000ull && values[i] < 0x10100000ull)))
            {
                buffer = values[i];
                haveBuffer = TRUE;
            }
        }

        for (i = 0; i < 6; i++)
        {
            if (values[i] >= 3 && values[i] < LX_MAX_FD && fd == 0)
            {
                fd = values[i];
            }
        }

        for (i = 0; i < 6; i++)
        {
            if (values[i] > 0 && values[i] < 0x10000 && values[i] != fd && values[i] != buffer && count == 0)
            {
                count = values[i];
            }
        }

        if (fd == 0)
        {
            fd = g_LastOpenFd;
        }

        if (count == 0)
        {
            count = 16;
        }

        g_ReadDebug[4] = fd;
        g_ReadDebug[5] = buffer;
        g_ReadDebug[6] = count;

        if (buffer == 0)
        {
            return (ULONG64)(-14);
        }

        if (count > 0x10000)
        {
            count = 0x10000;
        }

        return LxReadFile((ULONG)fd, (PVOID)(ULONG_PTR)buffer, (ULONG)count);
    }

    case LX_SYS_WRITE:
    {
        ULONG64 values[6];
        ULONG64 fd = 0;
        ULONG64 buffer = 0;
        ULONG length = 0;
        ULONG i;
        BOOLEAN haveFd = FALSE;
        BOOLEAN haveBuffer = FALSE;

        for (i = 0; i < 6; i++)
        {
            values[i] = Parameters[i];
        }

        for (i = 0; i < 6; i++)
        {
            if (!haveBuffer &&
                ((values[i] >= 0x400000ull && values[i] < 0x520000ull) ||
                 (values[i] >= 0x7fff00000000ull && values[i] < 0x800000000000ull) ||
                 (values[i] >= 0x100000000ull && values[i] < 0x101000000ull)))
            {
                buffer = values[i];
                haveBuffer = TRUE;
            }
        }

        for (i = 0; i < 6; i++)
        {
            if (!haveFd && (values[i] == 1 || values[i] == 2))
            {
                fd = values[i];
                haveFd = TRUE;
            }
        }

        for (i = 0; i < 6; i++)
        {
            if (length == 0 && values[i] > 0 && values[i] < 0x10000 && values[i] != fd)
            {
                length = (ULONG)values[i];
            }
        }

        g_WriteDebug[0] = values[0];
        g_WriteDebug[1] = values[1];
        g_WriteDebug[2] = values[2];
        g_WriteDebug[3] = buffer;
        g_WriteDebug[4] = values[3];
        g_WriteDebug[5] = values[4];
        g_WriteDebug[6] = values[5];
        g_WriteDebug[7] = (ULONG64)length;

        if (haveBuffer && length == 0)
        {
            if (g_GetContextFn != 0)
            {
                UCHAR contextBuffer[0x200];
                NTSTATUS contextStatus;

                RtlZeroMemory(contextBuffer, sizeof(contextBuffer));

                contextStatus = ((NTSTATUS (*)(PVOID, PVOID, ULONG))g_GetContextFn)(
                    KeGetCurrentThread(), contextBuffer, 0x10000Bu);

                g_GetContextStatus = (ULONG)contextStatus;

                if (NT_SUCCESS(contextStatus))
                {
                    g_ContextRdx = *(ULONG64 *)(contextBuffer + 0x88);

                    if (g_ContextRdx > 0 && g_ContextRdx < 0x10000)
                    {
                        length = (ULONG)g_ContextRdx;
                    }
                }
            }

            if (length == 0)
            {
                length = 8;
            }
        }

        if (haveBuffer && length > 0)
        {
            if (length > sizeof(g_WriteBuffer) - 1)
            {
                length = sizeof(g_WriteBuffer) - 1;
            }

            if (g_WriteCount < 64)
            {
                RtlZeroMemory(g_WriteBuffer, sizeof(g_WriteBuffer));

                __try
                {
                    ProbeForRead((PVOID)(ULONG_PTR)buffer, length, 1);
                    RtlCopyMemory(g_WriteBuffer, (PVOID)(ULONG_PTR)buffer, length);
                    g_WriteLength = length;
                    g_WriteCount++;
                    g_WriteFd = (ULONG)fd;
                    g_WriteStatus = 0;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    g_WriteStatus = 0xE0000004u;
                }
            }
        }

        return (ULONG64)length;
    }

    case LX_SYS_BRK:
        if (arg0 == 0)
        {
            return g_BrkCurrent;
        }
        if (arg0 >= g_PoolBase && arg0 <= g_PoolBase + LX_POOL_SIZE / 2)
        {
            g_BrkCurrent = arg0;
            return arg0;
        }
        return g_BrkCurrent;

    case LX_SYS_MMAP:
    {
        ULONG64 length = arg1;
        ULONG64 address;

        if (length == 0)
        {
            return (ULONG64)(-22);
        }

        length = (length + 0xFFF) & ~0xFFFull;
        address = g_MmapCurrent;

        if (address + length > g_PoolBase + LX_POOL_SIZE)
        {
            return (ULONG64)(-12);
        }

        g_MmapCurrent += length;
        return address;
    }

    case LX_SYS_ARCH_PRCTL:
    {
        ULONG64 code = arg0;
        ULONG64 address = arg1;

        if (code != 0x1001 && code != 0x1002 && code != 0x1003)
        {
            code = arg1;
            address = arg0;
        }

        if (code == 0x1002)
        {
            if (g_SetDescriptorBaseFn != 0)
            {
                ((ULONG64(*)(ULONG, ULONG64))g_SetDescriptorBaseFn)(0, address);
            }

            g_ArchPrctlCalls++;

            __try
            {
                g_FsBaseAfter = __readmsr(0xC0000100);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                g_FsBaseAfter = 0xDEAD0001u;
            }

            return 0;
        }

        if (code == 0x1001)
        {
            if (g_SetDescriptorBaseFn != 0)
            {
                ((ULONG64(*)(ULONG, ULONG64))g_SetDescriptorBaseFn)(1, address);
            }

            g_ArchPrctlCalls++;
            return 0;
        }

        return 0;
    }

    case LX_SYS_SET_TID_ADDRESS:
        return 1;

    case LX_SYS_EXIT:
    case LX_SYS_EXIT_GROUP:
        g_ExitCode = arg0;
        g_ExitCalled = 1;
        return 0;

    default:
        return (ULONG64)(-38);
    }
}

static NTSTATUS LxSysDispatch(PVOID Args)
{
    ULONG64 *argument = (ULONG64 *)Args;
    ULONG64 context;
    ULONG i;

    if (argument == NULL)
    {
        return 0;
    }

    if (g_SyscallCount == 0)
    {
        for (i = 0; i < 8; i++)
        {
            g_ArgsDump[i] = argument[i];
        }
    }

    context = argument[0];
    g_SyscallCount++;
    g_LastContext = context;

    if (context != 0)
    {
        ULONG64 *fields = (ULONG64 *)context;
        ULONG64 syscallNumber = fields[0x30 / 8];

        if (g_SyscallCount == 1)
        {
            for (i = 0; i < 64; i++)
            {
                g_ContextDump[i] = fields[i];
            }
        }

        if (g_SyscallHistoryCount < 32)
        {
            g_SyscallHistory[g_SyscallHistoryCount] = syscallNumber;
            g_SyscallHistoryCount++;
        }

        g_LastSyscallNo = syscallNumber;
        fields[0x30 / 8] = 0;

        {
            ULONG64 result = LxHandleSyscall(syscallNumber, &fields[LX_PARAM_BASE / 8], context);

            fields[0x30 / 8] = result;
            g_LastSyscallResult = result;
        }
    }

    return 0;
}

static VOID LxThreadExit(PVOID Args)
{
    UNREFERENCED_PARAMETER(Args);
    g_ThreadExitCount++;
}

static VOID LxProcessExit(PVOID Args)
{
    UNREFERENCED_PARAMETER(Args);
    g_ProcessExitCount++;
}

static ULONG64 g_ExceptionDump[8];
static ULONG g_ExceptionDumpDone = 0;

static NTSTATUS LxDispatchException(PVOID Args)
{
    ULONG64 *argument = (ULONG64 *)Args;
    ULONG i;

    if (Args != NULL && g_ExceptionDumpDone == 0)
    {
        for (i = 0; i < 8; i++)
        {
            g_ExceptionDump[i] = argument[i];
        }
        g_ExceptionDumpDone = 1;
    }

    g_ExceptionCount++;
    return 0;
}

static VOID LxProcessTerminate(PVOID Args)
{
    UNREFERENCED_PARAMETER(Args);
    g_ProcessTerminateCount++;
}

static NTSTATUS LxWalkUserStack(PVOID Args)
{
    UNREFERENCED_PARAMETER(Args);
    return 0;
}

static ULONG64 LxProtectedRanges(PVOID Args)
{
    UNREFERENCED_PARAMETER(Args);
    return 0;
}

typedef struct _LX_PICO_ARGS
{
    ULONG64 ParentHandle;
    ULONG64 TokenHandle;
    ULONG64 PicoContext;
    ULONG64 Flags;
} LX_PICO_ARGS;

static PVOID LxFindKernelExport(PCWSTR Name)
{
    UNICODE_STRING name;

    RtlInitUnicodeString(&name, Name);
    return MmGetSystemRoutineAddress(&name);
}

static ULONG64 LxFindNtoskrnlBase(PDRIVER_OBJECT DriverObject)
{
    PVOID section;
    PLIST_ENTRY head;
    PLIST_ENTRY cur;
    UNICODE_STRING wanted1;
    UNICODE_STRING wanted2;
    ULONG guard;

    if (DriverObject == NULL)
    {
        return 0;
    }

    section = DriverObject->DriverSection;
    if (section == NULL)
    {
        return 0;
    }

    RtlInitUnicodeString(&wanted1, L"ntoskrnl.exe");
    RtlInitUnicodeString(&wanted2, L"ntkrnlmp.exe");

    head = (PLIST_ENTRY)section;
    cur = head->Flink;
    guard = 0;

    while (cur != head && guard < 512)
    {
        UCHAR *entry = (UCHAR *)cur;
        PVOID dllBase = *(PVOID *)(entry + LX_LDR_DLL_BASE);
        UNICODE_STRING *baseName = (UNICODE_STRING *)(entry + LX_LDR_BASE_NAME);

        if (baseName->Buffer != NULL && baseName->Length > 0)
        {
            if (RtlEqualUnicodeString(baseName, &wanted1, TRUE) ||
                RtlEqualUnicodeString(baseName, &wanted2, TRUE))
            {
                ULONG copyLen = baseName->Length / sizeof(WCHAR);

                if (copyLen > 16)
                {
                    copyLen = 16;
                }
                RtlCopyMemory(g_NtosFoundName, baseName->Buffer, copyLen * sizeof(WCHAR));
                g_NtosFoundNameLen = copyLen;

                {
                    ULONG k;

                    for (k = 0; k < 12; k++)
                    {
                        g_NtosEntryRaw[k] = *(ULONG64 *)(entry + k * 8);
                    }
                }

                return (ULONG64)dllBase;
            }
        }

        cur = cur->Flink;
        guard++;
    }

    return 0;
}

static void LxLoadElf(PVOID Buffer, ULONG BufferLength)
{
    LX_ELF_LOAD *load;
    LX_ELF_SEG *segs;
    UCHAR *imageData;
    OBJECT_ATTRIBUTES objectAttributes;
    LARGE_INTEGER maximumSize;
    HANDLE sectionHandle = NULL;
    PVOID processObject = NULL;
    PEPROCESS targetProcess;
    PVOID baseAddress;
    SIZE_T viewSize;
    NTSTATUS status;
    ULONG i;

    g_ElfStatus = 0xFFFFFFFFu;
    g_ElfMapStatus = 0xFFFFFFFFu;
    g_ElfBase = 0;
    g_ElfCopiedTotal = 0;
    g_ElfViewSize = 0;

    for (i = 0; i < LX_ELF_MAX_SEGMENTS; i++)
    {
        g_ElfSegStatus[i] = 0xFFFFFFFFu;
    }

    if (g_LastPicoHandle == 0 || BufferLength < sizeof(LX_ELF_LOAD))
    {
        g_ElfStatus = 0xC0000001u;
        return;
    }

    load = (LX_ELF_LOAD *)Buffer;

    if (load->SegmentCount == 0 || load->SegmentCount > LX_ELF_MAX_SEGMENTS || load->ImageSize == 0)
    {
        g_ElfStatus = 0xC000000Du;
        return;
    }

    segs = (LX_ELF_SEG *)((UCHAR *)Buffer + sizeof(LX_ELF_LOAD));
    imageData = (UCHAR *)Buffer + sizeof(LX_ELF_LOAD) + load->SegmentCount * sizeof(LX_ELF_SEG);

    status = ObReferenceObjectByHandle((HANDLE)g_LastPicoHandle, PROCESS_ALL_ACCESS, *PsProcessType, KernelMode,
                                       &processObject, NULL);
    if (!NT_SUCCESS(status))
    {
        g_ElfStatus = (ULONG)status;
        return;
    }

    targetProcess = (PEPROCESS)processObject;

    maximumSize.QuadPart = (LONGLONG)load->ImageSize;
    InitializeObjectAttributes(&objectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwCreateSection(&sectionHandle, SECTION_ALL_ACCESS, &objectAttributes, &maximumSize,
                             PAGE_EXECUTE_READWRITE, SEC_COMMIT, NULL);
    if (!NT_SUCCESS(status))
    {
        g_ElfStatus = (ULONG)status;
        ObDereferenceObject(targetProcess);
        return;
    }

    baseAddress = (PVOID)(ULONG_PTR)load->MinVaddr;
    viewSize = 0;

    status = ZwMapViewOfSection(sectionHandle, (HANDLE)g_LastPicoHandle, &baseAddress, 0, 0, NULL, &viewSize,
                                ViewUnmap, 0, PAGE_EXECUTE_READWRITE);

    g_ElfMapStatus = (ULONG)status;
    g_ElfBase = (ULONG64)(ULONG_PTR)baseAddress;
    g_ElfViewSize = (ULONG64)viewSize;

    if (!NT_SUCCESS(status))
    {
        g_ElfStatus = (ULONG)status;
        ZwClose(sectionHandle);
        ObDereferenceObject(targetProcess);
        return;
    }

    for (i = 0; i < load->SegmentCount; i++)
    {
        SIZE_T copied = 0;

        if (segs[i].FileSize == 0)
        {
            g_ElfSegStatus[i] = 0;
            continue;
        }

        status = MmCopyVirtualMemory(PsGetCurrentProcess(), imageData + segs[i].FileOffset, targetProcess,
                                     (PVOID)(ULONG_PTR)segs[i].Vaddr, (SIZE_T)segs[i].FileSize, KernelMode, &copied);

        g_ElfSegStatus[i] = (ULONG)status;
        g_ElfCopiedTotal += (ULONG64)copied;

        if (!NT_SUCCESS(status))
        {
            g_ElfStatus = (ULONG)status;
            ZwClose(sectionHandle);
            ObDereferenceObject(targetProcess);
            return;
        }
    }

    g_ElfStatus = 0;
    g_ElfEntry = load->EntryPoint;
    RtlCopyMemory(&g_ElfLoad, load, sizeof(LX_ELF_LOAD));

    ZwClose(sectionHandle);
    ObDereferenceObject(targetProcess);
}

static void LxCreatePicoProcess(void)
{
    LX_PICO_ARGS args;
    HANDLE parentHandle = NULL;
    HANDLE picoHandle = NULL;
    PVOID context;
    NTSTATUS status;

    g_LastPicoStatus = 0xFFFFFFFFu;
    g_LastPicoHandle = 0;
    g_ParentHandleStatus = 0xFFFFFFFFu;

    if (g_CreatePicoFn == 0)
    {
        g_LastPicoStatus = 0xC0000001u;
        return;
    }

    context = ExAllocatePool2(POOL_FLAG_NON_PAGED, 0x100, 'xtcL');
    if (context == NULL)
    {
        g_LastPicoStatus = 0xC000009Au;
        return;
    }
    RtlZeroMemory(context, 0x100);

    status = ObOpenObjectByPointer(PsGetCurrentProcess(), 0, NULL, 0x80, NULL, KernelMode, &parentHandle);
    g_ParentHandleStatus = (ULONG)status;

    if (!NT_SUCCESS(status))
    {
        g_LastPicoStatus = (ULONG)status;
        ExFreePool(context);
        return;
    }

    args.ParentHandle = (ULONG64)parentHandle;
    args.TokenHandle = 0;
    args.PicoContext = (ULONG64)context;
    args.Flags = 1;

    status = ((NTSTATUS (*)(PVOID, PVOID, PVOID))g_CreatePicoFn)(&args, NULL, &picoHandle);
    g_LastPicoStatus = (ULONG)status;
    g_LastPicoHandle = (ULONG64)picoHandle;

    if (NT_SUCCESS(status))
    {
        g_PicoContext = (ULONG64)context;
    }
    else
    {
        ExFreePool(context);
    }

    ZwClose(parentHandle);
}

static void LxMapPool(void)
{
    HANDLE sectionHandle = NULL;
    OBJECT_ATTRIBUTES objectAttributes;
    LARGE_INTEGER maximumSize;
    PVOID baseAddress = (PVOID)0x100000000ull;
    SIZE_T viewSize = 0;
    NTSTATUS status;

    g_PoolStatus = 0xFFFFFFFFu;
    g_PoolBase = 0;

    maximumSize.QuadPart = (LONGLONG)LX_POOL_SIZE;
    InitializeObjectAttributes(&objectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwCreateSection(&sectionHandle, SECTION_ALL_ACCESS, &objectAttributes, &maximumSize,
                             PAGE_READWRITE, SEC_COMMIT, NULL);
    if (!NT_SUCCESS(status))
    {
        g_PoolStatus = (ULONG)status;
        return;
    }

    status = ZwMapViewOfSection(sectionHandle, (HANDLE)g_LastPicoHandle, &baseAddress, 0, 0, NULL, &viewSize,
                                ViewUnmap, 0, PAGE_READWRITE);

    g_PoolStatus = (ULONG)status;
    g_PoolBase = (ULONG64)(ULONG_PTR)baseAddress;

    if (NT_SUCCESS(status))
    {
        g_BrkCurrent = g_PoolBase;
        g_MmapCurrent = g_PoolBase + LX_POOL_SIZE / 2;
    }

    ZwClose(sectionHandle);
}

#define LX_PROCESS_VM_ACCESS 0x0028ul

typedef struct _LX_CONTEXT
{
    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5Home;
    ULONG64 P6Home;
    ULONG ContextFlags;
    ULONG MxCsr;
    USHORT SegCs;
    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegGs;
    USHORT SegSs;
    ULONG EFlags;
    ULONG64 Dr0;
    ULONG64 Dr1;
    ULONG64 Dr2;
    ULONG64 Dr3;
    ULONG64 Dr6;
    ULONG64 Dr7;
    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 Rbx;
    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;
    ULONG64 Rip;
} LX_CONTEXT;

static ULONG g_StackSetupStatus = 0xFFFFFFFFu;

static ULONG64 LxSetupStack(ULONG64 StackTop, const LX_ELF_LOAD *Load)
{
    UCHAR buffer[512];
    ULONG64 *slots = (ULONG64 *)buffer;
    ULONG index = 0;
    ULONG64 nameAddress;
    ULONG64 stackPointer;
    ULONG totalBytes;
    NTSTATUS status;
    PVOID processObject = NULL;
    LX_APC_STATE apcState;
    const char name[] = "busybox";

    g_StackSetupStatus = 0xFFFFFFFFu;

    RtlZeroMemory(buffer, sizeof(buffer));

    nameAddress = StackTop - (ULONG64)sizeof(name);

    slots[index++] = 1;
    slots[index++] = nameAddress;
    slots[index++] = 0;
    slots[index++] = 0;
    slots[index++] = 0;

    slots[index++] = 6;
    slots[index++] = 4096;
    slots[index++] = 3;
    slots[index++] = Load->PhdrAddress;
    slots[index++] = 4;
    slots[index++] = Load->PhdrEntrySize;
    slots[index++] = 5;
    slots[index++] = Load->PhdrCount;
    slots[index++] = 9;
    slots[index++] = Load->EntryPoint;
    slots[index++] = 11;
    slots[index++] = 0;
    slots[index++] = 12;
    slots[index++] = 0;
    slots[index++] = 13;
    slots[index++] = 0;
    slots[index++] = 14;
    slots[index++] = 0;
    slots[index++] = 23;
    slots[index++] = 0;
    slots[index++] = 0;
    slots[index++] = 0;

    totalBytes = index * sizeof(ULONG64);
    stackPointer = (nameAddress - totalBytes) & ~0xFull;

    status = ObReferenceObjectByHandle((HANDLE)g_LastPicoHandle, LX_PROCESS_VM_ACCESS, *PsProcessType, KernelMode,
                                       &processObject, NULL);
    if (!NT_SUCCESS(status))
    {
        g_StackSetupStatus = (ULONG)status;
        return 0;
    }

    KeStackAttachProcess((PEPROCESS)processObject, (PVOID)&apcState);

    __try
    {
        RtlCopyMemory((PVOID)(ULONG_PTR)stackPointer, buffer, totalBytes);
        RtlCopyMemory((PVOID)(ULONG_PTR)nameAddress, name, sizeof(name));
        g_StackSetupStatus = 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        g_StackSetupStatus = 0xE0000003u;
    }

    KeUnstackDetachProcess((PVOID)&apcState);

    ObDereferenceObject(processObject);

    if (g_StackSetupStatus != 0)
    {
        return 0;
    }

    return stackPointer;
}

static void LxCreatePicoThread(void)
{
    ULONG64 threadArgs[0x18];
    HANDLE sectionHandle = NULL;
    OBJECT_ATTRIBUTES objectAttributes;
    LARGE_INTEGER maximumSize;
    PVOID baseAddress;
    SIZE_T viewSize;
    NTSTATUS status;
    HANDLE threadHandle = NULL;
    ULONG64 stackTop;

    g_ThreadStatus = 0xFFFFFFFFu;
    g_StackMapStatus = 0xFFFFFFFFu;
    g_ThreadHandle = 0;

    if (g_LastPicoHandle == 0 || g_CreatePicoThreadFn == 0 || g_ElfEntry == 0 || g_PicoContext == 0)
    {
        g_ThreadStatus = 0xC0000001u;
        return;
    }

    LxMapPool();

    maximumSize.QuadPart = 0x20000;
    InitializeObjectAttributes(&objectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwCreateSection(&sectionHandle, SECTION_ALL_ACCESS, &objectAttributes, &maximumSize,
                             PAGE_READWRITE, SEC_COMMIT, NULL);
    if (!NT_SUCCESS(status))
    {
        g_ThreadStatus = (ULONG)status;
        return;
    }

    baseAddress = (PVOID)0x7FFF00000000ull;
    viewSize = 0;

    status = ZwMapViewOfSection(sectionHandle, (HANDLE)g_LastPicoHandle, &baseAddress, 0, 0, NULL, &viewSize,
                                ViewUnmap, 0, PAGE_READWRITE);

    g_StackMapStatus = (ULONG)status;
    g_StackBase = (ULONG64)(ULONG_PTR)baseAddress;

    if (!NT_SUCCESS(status))
    {
        g_ThreadStatus = (ULONG)status;
        ZwClose(sectionHandle);
        return;
    }

    stackTop = (g_StackBase + 0x20000 - 0x100) & ~0xFull;

    {
        ULONG64 newStackPointer = LxSetupStack(stackTop, &g_ElfLoad);

        if (newStackPointer != 0)
        {
            stackTop = newStackPointer;
        }
    }

    RtlZeroMemory(threadArgs, sizeof(threadArgs));

    threadArgs[0] = g_LastPicoHandle;
    threadArgs[1] = stackTop;
    threadArgs[2] = g_ElfEntry;
    threadArgs[0x17] = g_PicoContext;

    status = ((NTSTATUS (*)(PVOID, PVOID, PVOID))g_CreatePicoThreadFn)(threadArgs, NULL, &threadHandle);

    g_ThreadStatus = (ULONG)status;
    g_ThreadHandle = (ULONG64)threadHandle;

    if (NT_SUCCESS(status) && threadHandle != NULL && g_ResumeThreadFn != 0)
    {
        PVOID threadObject = NULL;
        ULONG previousCount = 0;
        NTSTATUS resumeStatus;

        resumeStatus = ObReferenceObjectByHandle(threadHandle, THREAD_SUSPEND_RESUME, *PsThreadType, KernelMode,
                                                 &threadObject, NULL);

        if (NT_SUCCESS(resumeStatus))
        {
            g_ResumeStatus = (ULONG)((NTSTATUS (*)(PVOID, PULONG))g_ResumeThreadFn)(threadObject, &previousCount);
            g_PrevSuspendCount = previousCount;
            ObDereferenceObject(threadObject);
        }
        else
        {
            g_ResumeStatus = (ULONG)resumeStatus;
        }
    }

    ZwClose(sectionHandle);
}

static void LxSelfTestFileAccess(void)
{
    UNICODE_STRING testPath;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK statusBlock;
    HANDLE handle = NULL;
    NTSTATUS status;

    RtlInitUnicodeString(&testPath, L"\\SystemRoot\\System32\\ntoskrnl.exe");
    InitializeObjectAttributes(&attributes, &testPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = ZwCreateFile(&handle, GENERIC_READ, &attributes, &statusBlock, NULL, FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    g_SelfTestSystemRoot = (ULONG)status;
    if (NT_SUCCESS(status))
    {
        ZwClose(handle);
        handle = NULL;
    }

    RtlInitUnicodeString(&testPath, L"\\GLOBAL??\\C:\\hello.x64");
    InitializeObjectAttributes(&attributes, &testPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = ZwCreateFile(&handle, GENERIC_READ, &attributes, &statusBlock, NULL, FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    g_SelfTestGlobal = (ULONG)status;
    if (NT_SUCCESS(status))
    {
        ZwClose(handle);
        handle = NULL;
    }

    RtlInitUnicodeString(&testPath, L"\\DosDevices\\C:\\hello.x64");
    InitializeObjectAttributes(&attributes, &testPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = ZwCreateFile(&handle, GENERIC_READ, &attributes, &statusBlock, NULL, FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    g_SelfTestDosDevices = (ULONG)status;
    if (NT_SUCCESS(status))
    {
        ZwClose(handle);
    }
}

static NTSTATUS LxrunCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS LxrunDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION stack;
    ULONG code;
    ULONG outLen = 0;
    NTSTATUS status = STATUS_SUCCESS;
    LX_QUERY_RESULT *result;
    ULONG i;

    UNREFERENCED_PARAMETER(DeviceObject);

    stack = IoGetCurrentIrpStackLocation(Irp);
    code = stack->Parameters.DeviceIoControl.IoControlCode;
    result = (LX_QUERY_RESULT *)Irp->AssociatedIrp.SystemBuffer;

    if (code != LX_IOCTL_QUERY && code != LX_IOCTL_REGISTER && code != LX_IOCTL_CREATE_PICO &&
        code != LX_IOCTL_LOAD_ELF && code != LX_IOCTL_CREATE_THREAD)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(LX_QUERY_RESULT))
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        if (code == LX_IOCTL_LOAD_ELF)
        {
            ULONG si;

            LxLoadElf(result, stack->Parameters.DeviceIoControl.InputBufferLength);

            RtlZeroMemory(result, sizeof(LX_QUERY_RESULT));
            result->Magic = 0x4C585255;
            result->ElfStatus = g_ElfStatus;
            result->ElfMapStatus = g_ElfMapStatus;
            result->ElfBase = g_ElfBase;
            result->ElfCopied = g_ElfCopiedTotal;
            result->ElfViewSize = g_ElfViewSize;

            for (si = 0; si < LX_ELF_MAX_SEGMENTS; si++)
            {
                result->ElfSegStatus[si] = g_ElfSegStatus[si];
            }

            outLen = sizeof(LX_QUERY_RESULT);
            status = STATUS_SUCCESS;
        }
        else
        {
            RtlZeroMemory(result, sizeof(LX_QUERY_RESULT));

            result->Magic = 0x4C585255;
        result->DeviceOk = 1;
        result->HasExport = (g_PsRegisterPicoProvider != NULL) ? 1 : 0;
        result->ExportAddr = (ULONG64)g_PsRegisterPicoProvider;

        if (g_PsRegisterPicoProvider != NULL)
        {
            ULONG i;

            __try
            {
                for (i = 0; i < 16; i++)
                {
                    result->ThunkBytes[i] = *(UCHAR *)((UCHAR *)g_PsRegisterPicoProvider + i);
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                result->ThunkBytes[0] = 0xDE;
            }

            __try
            {
                const UCHAR *createAddr = (const UCHAR *)g_PsRegisterPicoProvider + 0x120;

                for (i = 0; i < 32; i++)
                {
                    result->CreateBytes[i] = createAddr[i];
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                result->CreateBytes[0] = 0xDE;
            }

            __try
            {
                const UCHAR *full = (const UCHAR *)g_PsRegisterPicoProvider;

                for (i = 0; i < 256; i++)
                {
                    result->FullBytes[i] = full[i];
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                result->FullBytes[0] = 0xDE;
            }
        }
        result->NtosBase = g_NtosBase;
        result->FoundNameLen = g_NtosFoundNameLen;
        RtlCopyMemory(result->FoundName, g_NtosFoundName, sizeof(result->FoundName));
        RtlCopyMemory(result->EntryRaw, g_NtosEntryRaw, sizeof(result->EntryRaw));

        if (g_NtosBase != 0)
        {
            ULONG i;

            __try
            {
                result->AltFlag4 = *(ULONG *)((UCHAR *)g_NtosBase + 0xCEDD20u);
                for (i = 0; i < 8; i++)
                {
                    result->AltFlagNear[i] = *(UCHAR *)((UCHAR *)g_NtosBase + 0xCEDD20u - 4 + i);
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                result->AltFlag4 = 0xDEAD0003u;
            }
        }

        if (g_NtosBase != 0)
        {
            __try
            {
                result->SizeOfImage = *(ULONG *)((UCHAR *)g_NtosBase + 0x50);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                result->SizeOfImage = 0xDEAD0001u;
            }
        }

        if (code == LX_IOCTL_REGISTER)
        {
            UCHAR info[LX_PICO_INFO_SIZE];
            UCHAR routines[LX_PICO_ROUTINES_SIZE];
            NTSTATUS (*registerProvider)(PVOID, PVOID);
            ULONG64 ntBase;
            volatile UCHAR *flag;

            RtlZeroMemory(info, sizeof(info));
            RtlZeroMemory(routines, sizeof(routines));

            {
                ULONG64 infoSize = LX_PICO_INFO_SIZE;
                ULONG64 routinesSize = LX_PICO_ROUTINES_SIZE;
                ULONG64 sysDispatchHandler = (ULONG64)&LxSysDispatch;
                ULONG64 handlers[6];

                handlers[0] = (ULONG64)&LxThreadExit;
                handlers[1] = (ULONG64)&LxProcessExit;
                handlers[2] = (ULONG64)&LxDispatchException;
                handlers[3] = (ULONG64)&LxProcessTerminate;
                handlers[4] = (ULONG64)&LxWalkUserStack;
                handlers[5] = (ULONG64)&LxProtectedRanges;

                RtlCopyMemory(&info[0], &infoSize, sizeof(infoSize));
                RtlCopyMemory(&routines[0], &routinesSize, sizeof(routinesSize));
                RtlCopyMemory(&info[8], &sysDispatchHandler, sizeof(sysDispatchHandler));
                RtlCopyMemory(&info[0x10], handlers, sizeof(handlers));

                g_SyscallHandlerAddr = sysDispatchHandler;
            }

            registerProvider = (NTSTATUS (*)(PVOID, PVOID))g_PsRegisterPicoProvider;

            result->LockedBytes[6] = routines[0];
            result->OldFlag = 0xBB;
            result->RegisterStatus = (ULONG)registerProvider(info, routines);
            result->LockedBytes[7] = routines[0];

            ntBase = g_NtosBase;
            if (ntBase != 0)
            {
flag = (volatile UCHAR *)((UCHAR *)g_PsRegisterPicoProvider + LX_FLAG_RIP_DISP);
                result->FlagAddr = (ULONG64)flag;
                result->NtosBase = ntBase;
                result->LockFnAddr = 0;
                result->UnlockFnAddr = 0;

                result->OldFlag = 0xCC;
                result->LockedBytes[0] = 0xCC;
                result->LockedBytes[1] = 0xCC;
                result->LockedBytes[2] = 0xCC;

                __try
                {
                    const UCHAR *routinesGlobal = (const UCHAR *)g_PsRegisterPicoProvider + LX_ROUTINES_RIP_DISP;
                    ULONG k;

                    result->RoutinesAddr = (ULONG64)routinesGlobal;
                    for (k = 0; k < 16; k++)
                    {
                        result->RoutinesBytes[k] = routinesGlobal[k];
                    }

                    result->OldFlag = (ULONG)(*flag);
                    *flag = 0;
                    result->LockedBytes[0] = *flag;
                    result->LockedBytes[2] = (UCHAR)(*(PULONG64)&info[0]);
                    result->LockedBytes[3] = (UCHAR)(*(PULONG64)&routines[0]);
                    result->RegisterStatus2 = (ULONG)registerProvider(info, routines);
                    result->LockedBytes[1] = *flag;
                    result->LockedBytes[4] = (UCHAR)(*(PULONG64)&info[0]);
                    result->LockedBytes[5] = (UCHAR)(*(PULONG64)&routines[0]);

                    if (result->RegisterStatus2 == 0)
                    {
                        ULONG64 createFn = *(ULONG64 *)&routines[8];
                        ULONG m;

                        for (m = 0; m < 96; m++)
                        {
                            result->RoutinesBytes[m] = routines[m];
                        }

                        result->CreateFnAddr = createFn;
                        g_CreatePicoFn = createFn;
                        g_CreatePicoThreadFn = *(ULONG64 *)&routines[0x10];
                        g_ResumeThreadFn = *(ULONG64 *)&routines[0x40];
                        g_SetDescriptorBaseFn = *(ULONG64 *)&routines[0x48];
                        g_GetContextFn = *(ULONG64 *)&routines[0x28];
                        if (createFn != 0)
                        {
                            __try
                            {
                                for (m = 0; m < 32; m++)
                                {
                                    result->CreateFnBytes[m] = ((const UCHAR *)createFn)[m];
                                }
                            }
                            __except (EXCEPTION_EXECUTE_HANDLER)
                            {
                                result->CreateFnBytes[0] = 0xDE;
                            }
                        }
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    result->RegisterStatus2 = 0xE0000001u;
                }
            }

            g_RegisterStatus = result->RegisterStatus;
            g_RegisterStatus2 = result->RegisterStatus2;
            g_OldFlag = result->OldFlag;
            g_FlagAddr = result->FlagAddr;

            status = STATUS_SUCCESS;
        }
        else if (code == LX_IOCTL_CREATE_PICO)
        {
            LxCreatePicoProcess();
            result->PicoHandle = g_LastPicoHandle;
            result->PicoStatus = g_LastPicoStatus;
            result->ParentStatus = g_ParentHandleStatus;
        }
        else if (code == LX_IOCTL_CREATE_THREAD)
        {
            LxCreatePicoThread();
            result->ThreadStatus = g_ThreadStatus;
            result->StackMapStatus = g_StackMapStatus;
            result->StackBase = g_StackBase;
            result->ThreadHandle = g_ThreadHandle;
            result->SyscallCount = g_SyscallCount;
            result->LastSyscallNo = g_LastSyscallNo;
            result->SyscallHandler = g_SyscallHandlerAddr;
            result->ResumeStatus = g_ResumeStatus;
            result->PrevSuspendCount = g_PrevSuspendCount;
        }
        else
        {
            result->RegisterStatus = g_RegisterStatus;
            result->RegisterStatus2 = g_RegisterStatus2;
            result->OldFlag = g_OldFlag;
            result->FlagAddr = g_FlagAddr;

            result->ThreadStatus = g_ThreadStatus;
            result->StackBase = g_StackBase;
            result->ThreadHandle = g_ThreadHandle;
            result->SyscallCount = g_SyscallCount;
            result->LastSyscallNo = g_LastSyscallNo;
            result->SyscallHandler = g_SyscallHandlerAddr;
            result->PicoHandle = g_LastPicoHandle;
            result->PicoStatus = g_LastPicoStatus;
            result->ElfStatus = g_ElfStatus;
            result->ElfBase = g_ElfBase;
            result->ElfCopied = g_ElfCopiedTotal;
            result->ThreadExitCount = g_ThreadExitCount;
            result->ProcessExitCount = g_ProcessExitCount;
            result->ExceptionCount = g_ExceptionCount;
            result->ProcessTerminateCount = g_ProcessTerminateCount;

            for (i = 0; i < 64; i++)
            {
                result->ContextDump[i] = g_ContextDump[i];
            }

            for (i = 0; i < 8; i++)
            {
                result->ArgsDump[i] = g_ArgsDump[i];
            }

            for (i = 0; i < 32; i++)
            {
                result->SyscallHistory[i] = g_SyscallHistory[i];
            }

            result->SyscallHistoryCount = g_SyscallHistoryCount;
            result->PoolStatus = g_PoolStatus;
            result->PoolBase = g_PoolBase;
            result->BrkCurrent = g_BrkCurrent;
            result->MmapCurrent = g_MmapCurrent;
            result->WriteLength = g_WriteLength;
            result->WriteCount = g_WriteCount;
            result->ExitCalled = g_ExitCalled;
            result->ExitCode = g_ExitCode;
            result->LastSyscallResult = g_LastSyscallResult;
            result->StackSetupStatus = g_StackSetupStatus;

            for (i = 0; i < 8; i++)
            {
                result->ExceptionDump[i] = g_ExceptionDump[i];
            }

            result->FsBaseAfter = g_FsBaseAfter;
            result->ArchPrctlCalls = g_ArchPrctlCalls;
            result->WriteStatus = g_WriteStatus;
            result->WriteFd = g_WriteFd;
            result->ContextRdx = g_ContextRdx;
            result->GetContextStatus = g_GetContextStatus;
            result->OpenCount = g_OpenCount;
            result->LastOpenFd = g_LastOpenFd;
            result->LastOpenStatus = g_LastOpenStatus;
            result->ReadCount = g_ReadCount;
            result->LastReadStatus = g_LastReadStatus;
            RtlCopyMemory(result->LastOpenPath, g_LastOpenPath, sizeof(result->LastOpenPath));
            RtlCopyMemory(result->LastNtPath, g_LastNtPath, sizeof(result->LastNtPath));
            result->SelfTestSystemRoot = g_SelfTestSystemRoot;
            result->SelfTestGlobal = g_SelfTestGlobal;
            result->SelfTestDosDevices = g_SelfTestDosDevices;

            for (i = 0; i < 8; i++)
            {
                result->ReadDebug[i] = g_ReadDebug[i];
                result->StatDebug[i] = g_StatDebug[i];
            }

            result->GetDentsCount = g_GetDentsCount;
            result->GetDentsStatus = g_GetDentsStatus;
            result->GetDentsBytes = g_GetDentsBytes;
            result->StatCount = g_StatCount;
            result->LastStatStatus = g_LastStatStatus;
            result->LastStatSize = g_LastStatSize;
            result->LastStatMode = g_LastStatMode;
            RtlCopyMemory(result->LastDirectoryPath, g_LastDirectoryPath, sizeof(result->LastDirectoryPath));
            RtlCopyMemory(result->LastStatPath, g_LastStatPath, sizeof(result->LastStatPath));

            for (i = 0; i < 8; i++)
            {
                result->WriteDebug[i] = g_WriteDebug[i];
            }

            for (i = 0; i < 64; i++)
            {
                result->WriteContext[i] = g_WriteContext[i];
            }

            RtlCopyMemory(result->WriteBuffer, g_WriteBuffer, sizeof(result->WriteBuffer));
        }

        outLen = sizeof(LX_QUERY_RESULT);
        }
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = outLen;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

VOID LxrunUnload(PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING symLink;

    UNREFERENCED_PARAMETER(DriverObject);

    RtlInitUnicodeString(&symLink, LX_SYMLINK_NAME);
    IoDeleteSymbolicLink(&symLink);

    if (g_DeviceObject != NULL)
    {
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
    }
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    UNICODE_STRING symLink;
    PDEVICE_OBJECT deviceObject = NULL;

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString(&deviceName, LX_DEVICE_NAME);

    status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN, FALSE, &deviceObject);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    RtlInitUnicodeString(&symLink, LX_SYMLINK_NAME);

    status = IoCreateSymbolicLink(&symLink, &deviceName);
    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(deviceObject);
        return status;
    }

    g_DeviceObject = deviceObject;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = LxrunCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = LxrunCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = LxrunDeviceControl;
    DriverObject->DriverUnload = LxrunUnload;

    g_PsRegisterPicoProvider = LxFindKernelExport(L"PsRegisterPicoProvider");
    g_NtosBase = LxFindNtoskrnlBase(DriverObject);

    LxSelfTestFileAccess();

    return STATUS_SUCCESS;
}
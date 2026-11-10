#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LX_IOCTL_QUERY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define LX_IOCTL_REGISTER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define LX_IOCTL_CREATE_PICO CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define LX_IOCTL_LOAD_ELF CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define LX_IOCTL_CREATE_THREAD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LX_ELF_MAX_SEGMENTS 16

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
} LX_QUERY_RESULT;

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

typedef struct _ELF64_EHDR
{
    UCHAR e_ident[16];
    USHORT e_type;
    USHORT e_machine;
    ULONG e_version;
    ULONG64 e_entry;
    ULONG64 e_phoff;
    ULONG64 e_shoff;
    ULONG e_flags;
    USHORT e_ehsize;
    USHORT e_phentsize;
    USHORT e_phnum;
    USHORT e_shentsize;
    USHORT e_shnum;
    USHORT e_shstrndx;
} ELF64_EHDR;

typedef struct _ELF64_PHDR
{
    ULONG p_type;
    ULONG p_flags;
    ULONG64 p_offset;
    ULONG64 p_vaddr;
    ULONG64 p_paddr;
    ULONG64 p_filesz;
    ULONG64 p_memsz;
    ULONG64 p_align;
} ELF64_PHDR;

static void PrintBytes(const char *Tag, const UCHAR *Bytes, int Count)
{
    int i;

    printf("%s:", Tag);
    for (i = 0; i < Count; i++)
    {
        printf(" %02x", Bytes[i]);
    }
    printf("\n");
}

static void DumpResult(const char *Label, const LX_QUERY_RESULT *Q, DWORD Ret, DWORD Err)
{
    int i;

    printf("%s: ret=%lu err=%lu\n", Label, Ret, Err);
    printf("  magic=0x%08lx reg=0x%08lx reg2=0x%08lx old=0x%lx\n", Q->Magic, Q->RegisterStatus,
           Q->RegisterStatus2, Q->OldFlag);
    printf("  ntos=0x%llx faddr=0x%llx\n", (unsigned long long)Q->NtosBase,
           (unsigned long long)Q->FlagAddr);
    printf("  routinesAddr=0x%llx createFn=0x%llx\n", (unsigned long long)Q->RoutinesAddr,
           (unsigned long long)Q->CreateFnAddr);
    PrintBytes("  routines", Q->RoutinesBytes, 96);
    PrintBytes("  createFn", Q->CreateFnBytes, 32);
    PrintBytes("  locked", Q->LockedBytes, 8);
    printf("  export=");
    for (i = 0; i < 12; i++)
    {
        printf(" %02x", Q->ThunkBytes[i]);
    }
    printf("\n");
}

static int LoadFileToBuffer(const wchar_t *Path, UCHAR **OutData, ULONG *OutSize)
{
    HANDLE file;
    LARGE_INTEGER size;
    DWORD read = 0;
    UCHAR *data;

    file = CreateFileW(Path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        printf("open %ls failed: %lu\n", Path, GetLastError());
        return 0;
    }

    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > (64LL << 20))
    {
        printf("bad size\n");
        CloseHandle(file);
        return 0;
    }

    data = (UCHAR *)malloc((size_t)size.QuadPart);
    if (data == NULL)
    {
        CloseHandle(file);
        return 0;
    }

    if (!ReadFile(file, data, (DWORD)size.QuadPart, &read, NULL) || read != (DWORD)size.QuadPart)
    {
        printf("read failed\n");
        free(data);
        CloseHandle(file);
        return 0;
    }

    CloseHandle(file);
    *OutData = data;
    *OutSize = (ULONG)size.QuadPart;
    return 1;
}

static int SendLoadElf(HANDLE device, const wchar_t *Path)
{
    UCHAR *fileData = NULL;
    ULONG fileSize = 0;
    ELF64_EHDR *ehdr;
    ELF64_PHDR *phdr;
    LX_ELF_LOAD *load;
    LX_ELF_SEG *segs;
    UCHAR *buffer;
    ULONG bufferSize;
    ULONG i;
    ULONG segCount = 0;
    ULONG64 minVaddr = 0xFFFFFFFFFFFFFFFFull;
    ULONG64 maxEnd = 0;
    LX_QUERY_RESULT q;
    DWORD ret = 0;
    BOOL ok;

    if (!LoadFileToBuffer(Path, &fileData, &fileSize))
    {
        return 0;
    }

    if (fileSize < sizeof(ELF64_EHDR))
    {
        printf("file too small\n");
        free(fileData);
        return 0;
    }

    ehdr = (ELF64_EHDR *)fileData;

    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' || ehdr->e_ident[2] != 'L' ||
        ehdr->e_ident[3] != 'F')
    {
        printf("not an ELF file\n");
        free(fileData);
        return 0;
    }

    printf("ELF: type=%u machine=%u entry=0x%llx phoff=0x%llx phnum=%u phentsize=%u\n", ehdr->e_type,
           ehdr->e_machine, (unsigned long long)ehdr->e_entry, (unsigned long long)ehdr->e_phoff,
           ehdr->e_phnum, ehdr->e_phentsize);

    phdr = (ELF64_PHDR *)(fileData + ehdr->e_phoff);

    for (i = 0; i < ehdr->e_phnum && i < 64; i++)
    {
        ELF64_PHDR *p = (ELF64_PHDR *)((UCHAR *)phdr + i * ehdr->e_phentsize);

        if (p->p_type != 1)
        {
            continue;
        }

        segCount++;

        if ((p->p_vaddr & ~0xFFFull) < minVaddr)
        {
            minVaddr = p->p_vaddr & ~0xFFFull;
        }

        if (((p->p_vaddr + p->p_memsz) + 0xFFFull) > maxEnd)
        {
            maxEnd = ((p->p_vaddr + p->p_memsz) + 0xFFFull) & ~0xFFFull;
        }
    }

    if (segCount == 0 || segCount > LX_ELF_MAX_SEGMENTS || maxEnd <= minVaddr)
    {
        printf("bad segments: count=%lu min=0x%llx max=0x%llx\n", segCount,
               (unsigned long long)minVaddr, (unsigned long long)maxEnd);
        free(fileData);
        return 0;
    }

    bufferSize = sizeof(LX_ELF_LOAD) + segCount * sizeof(LX_ELF_SEG) + fileSize;
    buffer = (UCHAR *)malloc(bufferSize);
    if (buffer == NULL)
    {
        free(fileData);
        return 0;
    }

    memset(buffer, 0, bufferSize);

    load = (LX_ELF_LOAD *)buffer;
    load->ImageSize = maxEnd - minVaddr;
    load->MinVaddr = minVaddr;
    load->EntryPoint = ehdr->e_entry;
    load->SegmentCount = segCount;
    load->DataSize = fileSize;
    load->PhdrCount = ehdr->e_phnum;
    load->PhdrEntrySize = ehdr->e_phentsize;

    {
        ULONG64 firstLoadOffset = 0;
        ULONG index2;

        for (index2 = 0; index2 < ehdr->e_phnum && index2 < 64; index2++)
        {
            ELF64_PHDR *p = (ELF64_PHDR *)((UCHAR *)phdr + index2 * ehdr->e_phentsize);

            if (p->p_type == 1)
            {
                firstLoadOffset = p->p_offset;
                break;
            }
        }

        load->PhdrAddress = minVaddr + (ehdr->e_phoff - firstLoadOffset);
    }

    segs = (LX_ELF_SEG *)(buffer + sizeof(LX_ELF_LOAD));

    segCount = 0;
    for (i = 0; i < ehdr->e_phnum && i < 64; i++)
    {
        ELF64_PHDR *p = (ELF64_PHDR *)((UCHAR *)phdr + i * ehdr->e_phentsize);

        if (p->p_type != 1)
        {
            continue;
        }

        segs[segCount].Vaddr = p->p_vaddr;
        segs[segCount].FileOffset = p->p_offset;
        segs[segCount].FileSize = p->p_filesz;
        segs[segCount].MemSize = p->p_memsz;
        segs[segCount].Flags = p->p_flags;

        printf("  LOAD[%lu] vaddr=0x%llx filesz=0x%llx memsz=0x%llx off=0x%llx flags=%lu\n", segCount,
               (unsigned long long)p->p_vaddr, (unsigned long long)p->p_filesz,
               (unsigned long long)p->p_memsz, (unsigned long long)p->p_offset, p->p_flags);
        segCount++;
    }

    memcpy(buffer + sizeof(LX_ELF_LOAD) + load->SegmentCount * sizeof(LX_ELF_SEG), fileData, fileSize);

    printf("sending: imageSize=0x%llx minVaddr=0x%llx segs=%lu dataSize=%lu total=%lu\n",
           (unsigned long long)load->ImageSize, (unsigned long long)load->MinVaddr, load->SegmentCount,
           load->DataSize, bufferSize);

    memset(&q, 0, sizeof(q));
    ok = DeviceIoControl(device, LX_IOCTL_LOAD_ELF, buffer, bufferSize, &q, sizeof(q), &ret, NULL);

    printf("LOAD_ELF: ok=%d ret=%lu\n", ok ? 1 : 0, ret);
    printf("  elfStatus=0x%08lx mapStatus=0x%08lx base=0x%llx copied=%llu viewSize=0x%llx\n", q.ElfStatus,
           q.ElfMapStatus, (unsigned long long)q.ElfBase, (unsigned long long)q.ElfCopied,
           (unsigned long long)q.ElfViewSize);
    for (i = 0; i < 4 && i < LX_ELF_MAX_SEGMENTS; i++)
    {
        printf("  seg[%lu] status=0x%08lx\n", i, q.ElfSegStatus[i]);
    }

    free(buffer);
    free(fileData);
    return q.ElfStatus == 0;
}

int main(void)
{
    HANDLE device;
    LX_QUERY_RESULT q;
    DWORD ret;
    BOOL ok;

    device = CreateFileW(L"\\\\.\\Lxrun", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    if (device == INVALID_HANDLE_VALUE)
    {
        printf("open device failed: %lu\n", GetLastError());
        return 1;
    }
    printf("device opened\n\n");

    memset(&q, 0, sizeof(q));
    ret = 0;
    ok = DeviceIoControl(device, LX_IOCTL_REGISTER, NULL, 0, &q, sizeof(q), &ret, NULL);
    DumpResult("REGISTER", &q, ret, ok ? 0 : GetLastError());

    printf("\n");
    memset(&q, 0, sizeof(q));
    ret = 0;
    ok = DeviceIoControl(device, LX_IOCTL_CREATE_PICO, NULL, 0, &q, sizeof(q), &ret, NULL);
    printf("CREATE_PICO: ok=%d ret=%lu\n", ok ? 1 : 0, ret);
    printf("  picoStatus=0x%08lx parentStatus=0x%08lx picoHandle=0x%llx\n", q.PicoStatus, q.ParentStatus,
           (unsigned long long)q.PicoHandle);

    printf("\n");
    if (q.PicoStatus == 0)
    {
        SendLoadElf(device, L"C:\\Windows\\hello.x64");

        printf("\n");
        memset(&q, 0, sizeof(q));
        ret = 0;
        ok = DeviceIoControl(device, LX_IOCTL_CREATE_THREAD, NULL, 0, &q, sizeof(q), &ret, NULL);
        printf("CREATE_THREAD: ok=%d ret=%lu\n", ok ? 1 : 0, ret);
        printf("  threadStatus=0x%08lx stackMap=0x%08lx stackBase=0x%llx threadHandle=0x%llx\n",
               q.ThreadStatus, q.StackMapStatus, (unsigned long long)q.StackBase,
               (unsigned long long)q.ThreadHandle);
        printf("  syscallCount=%llu lastSyscall=%llu handler=0x%llx\n",
               (unsigned long long)q.SyscallCount, (unsigned long long)q.LastSyscallNo,
               (unsigned long long)q.SyscallHandler);

        Sleep(3000);

        memset(&q, 0, sizeof(q));
        ret = 0;
        ok = DeviceIoControl(device, LX_IOCTL_QUERY, NULL, 0, &q, sizeof(q), &ret, NULL);
        printf("AFTER 3s: syscallCount=%llu lastSyscall=%llu\n",
               (unsigned long long)q.SyscallCount, (unsigned long long)q.LastSyscallNo);
        printf("  resumeStatus=0x%08lx prevSuspend=%lu\n", q.ResumeStatus, q.PrevSuspendCount);
        printf("  pico=0x%llx elfStatus=0x%08lx elfBase=0x%llx copied=%llu\n",
               (unsigned long long)q.PicoHandle, q.ElfStatus, (unsigned long long)q.ElfBase,
               (unsigned long long)q.ElfCopied);
        printf("  threadExit=%llu processExit=%llu exception=%llu term=%llu\n",
               (unsigned long long)q.ThreadExitCount, (unsigned long long)q.ProcessExitCount,
               (unsigned long long)q.ExceptionCount, (unsigned long long)q.ProcessTerminateCount);
        {
            int k;

            printf("  args dump:");
            for (k = 0; k < 8; k++)
            {
                printf(" %llx", (unsigned long long)q.ArgsDump[k]);
            }
            printf("\n  ctx dump (nonzero):\n");
            for (k = 0; k < 64; k++)
            {
                if (q.ContextDump[k] != 0)
                {
                    printf("    +0x%03x = 0x%llx\n", k * 8, (unsigned long long)q.ContextDump[k]);
                }
            }

            printf("  syscalls (%lu):", q.SyscallHistoryCount);
            for (k = 0; k < (int)q.SyscallHistoryCount && k < 32; k++)
            {
                printf(" %llu", (unsigned long long)q.SyscallHistory[k]);
            }
            printf("\n");

            printf("  poolStatus=0x%08lx poolBase=0x%llx brk=0x%llx mmap=0x%llx\n", q.PoolStatus,
                   (unsigned long long)q.PoolBase, (unsigned long long)q.BrkCurrent,
                   (unsigned long long)q.MmapCurrent);
            printf("  writeCount=%lu writeLen=%lu exitCalled=%lu exitCode=%llu lastResult=0x%llx\n",
                   q.WriteCount, q.WriteLength, q.ExitCalled, (unsigned long long)q.ExitCode,
                   (unsigned long long)q.LastSyscallResult);
            printf("  writeFd=%lu\n", q.WriteFd);
            printf("  contextRdx=0x%llx getContextStatus=0x%08lx\n", (unsigned long long)q.ContextRdx,
                   q.GetContextStatus);
            printf("  openCount=%lu lastOpenFd=%lu lastOpenStatus=0x%llx\n", q.OpenCount, q.LastOpenFd,
                   (unsigned long long)q.LastOpenStatus);
            printf("  lastOpenPath=[%.*s]\n", (int)sizeof(q.LastOpenPath), q.LastOpenPath);
            printf("  lastNtPath=[%.*s]\n", (int)sizeof(q.LastNtPath), q.LastNtPath);
            printf("  selfTest: systemRoot=0x%08lx global=0x%08lx dosDevices=0x%08lx\n",
                   q.SelfTestSystemRoot, q.SelfTestGlobal, q.SelfTestDosDevices);
            printf("  readCount=%llu lastReadStatus=0x%llx\n", (unsigned long long)q.ReadCount,
                   (unsigned long long)q.LastReadStatus);
            printf("  stackSetup=0x%08lx\n", q.StackSetupStatus);
            printf("  fsBaseAfter=0x%llx archPrctlCalls=%lu\n", (unsigned long long)q.FsBaseAfter,
                   q.ArchPrctlCalls);
            printf("  writeDebug=%llx %llx %llx %llx %llx %llx %llx len=%llx fd=%lx status=0x%08lx\n",
                   (unsigned long long)q.WriteDebug[0], (unsigned long long)q.WriteDebug[1],
                   (unsigned long long)q.WriteDebug[2], (unsigned long long)q.WriteDebug[3],
                   (unsigned long long)q.WriteDebug[4], (unsigned long long)q.WriteDebug[5],
                   (unsigned long long)q.WriteDebug[6], (unsigned long long)q.WriteDebug[7],
                   q.WriteFd, q.WriteStatus);
            {
                int m;

                printf("  writeCtx (nonzero):\n");
                for (m = 0; m < 64; m++)
                {
                    if (q.WriteContext[m] != 0)
                    {
                        printf("    +0x%03x = 0x%llx\n", m * 8, (unsigned long long)q.WriteContext[m]);
                    }
                }
            }
            {
                int m;

                printf("  exceptionDump:");
                for (m = 0; m < 8; m++)
                {
                    printf(" %llx", (unsigned long long)q.ExceptionDump[m]);
                }
                printf("\n");
            }

            if (q.WriteLength > 0)
            {
                char text[513];

                memcpy(text, q.WriteBuffer, q.WriteLength < 512 ? q.WriteLength : 512);
                text[q.WriteLength < 512 ? q.WriteLength : 512] = 0;
                printf("  WRITE OUTPUT: [%s]\n", text);
            }
        }
    }
    else
    {
        printf("skip ELF load (no pico process)\n");
    }

    CloseHandle(device);
    return 0;
}
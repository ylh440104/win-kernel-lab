#include <windows.h>
#include <stdio.h>
#define NLSTR "\n"
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
    ULONG64 DynInterpVaddr;
    ULONG DynHasInterp;
    ULONG DynHasDynamic;
    ULONG64 DynDynamicVaddr;
    ULONG64 DynDynamicSize;
    ULONG DynDynamicCount;
    ULONG64 DynStrTabVaddr;
    ULONG64 DynSymTabVaddr;
    ULONG64 DynRelaVaddr;
    ULONG64 DynRelaSize;
    ULONG64 DynRelaCount;
    ULONG64 DynJmpRelVaddr;
    ULONG64 DynJmpRelSize;
    ULONG64 DynJmpRelCount;
    ULONG64 DynPltGotVaddr;
    ULONG64 DynInitVaddr;
    ULONG64 DynFiniVaddr;
    ULONG64 DynSonameOffset;
    ULONG DynNeededCount;
    ULONG DynNeededOffsets[8];
    ULONG RelocRelativeCount;
    ULONG RelocJumpSlotCount;
    ULONG RelocGlobDatCount;
    ULONG RelocUnsupportedCount;
    ULONG RelocWriteFailCount;
    ULONG RelocTotalCount;
    ULONG64 RelocFirstOffset;
    ULONG64 RelocFirstAddend;
    ULONG64 RelocFirstWritten;
    ULONG64 RelocFirstReadBack;
    ULONG RelocFirstStatus;
    ULONG SymTotal;
    ULONG SymDumped;
    ULONG SymTruncated;
    ULONG SymStrTabReadable;
    ULONG SymStrTabStatus;
    ULONG64 SymNameFileOffset;
    ULONG SymNameByte0;
    ULONG SymNameByte1;
    ULONG64 SymSymTabVaddr;
    ULONG64 SymStrTabVaddr;
    ULONG SymNameOffsets[512];
    ULONG SymRelocOffsets[512];
    CHAR SymNames[512][32];
    ULONG StubCount;
    ULONG StubFound;
    ULONG StubUnknown;
    ULONG StubWriteFail;
    ULONG64 StubAreaBase;
    ULONG64 StubLastAddress;
    ULONG64 StubLastSyscall;
    ULONG StubLastStatus;
    CHAR StubLastName[48];
    ULONG StubSlotUsed;
    ULONG LibAttempted;
    ULONG LibLoaded;
    ULONG LibFailed;
    ULONG LibLastStatus;
    ULONG LibTotalSymbols;
    ULONG LibTotalRelocs;
    ULONG LibNameResolveFail;
    ULONG LibPathResolveFail;
    ULONG LibUsed[4];
    ULONG LibStatus[4];
    ULONG LibSymCount[4];
    ULONG64 LibFirstSymValue[4];
    ULONG64 LibSymTab[4];
    ULONG64 LibGot12C0[4];
    ULONG64 DiagGlobalSinm;
    ULONG64 DiagMainGot;
    ULONG LibRelaCount[4];
    ULONG LibRelaApplied[4];
    ULONG LibNeededCount[4];
    ULONG64 LibBase[4];
    ULONG64 LibImageSize[4];
    CHAR LibNames[4][48];
    ULONG GlobalCount;
    ULONG GlobalCapacity;
    ULONG GlobalProbeCount;
    ULONG GlobalProbeHit;
    ULONG GlobalCrossResolved;
    ULONG GlobalCrossMiss;
    ULONG64 GlobalLastHitAddress;
    CHAR GlobalLastMissName[48];
    CHAR GlobalLastHitName[48];
    CHAR GlobalFirstNames[8][48];
    ULONG BuiltinWritten;
    ULONG BuiltinTotalBytes;
    ULONG BuiltinFailCount;
    ULONG BuiltinRegistered;
    ULONG BuiltinLastStatus;
    ULONG64 BuiltinAreaBase;
    ULONG64 BuiltinLastAddress;
    CHAR BuiltinLastName[48];
    ULONG64 BrkAddress;
    ULONG BrkOkCount;
    ULONG DrvStructSize;
    ULONG64 Ent0No;
    ULONG64 Ent0P0;
    ULONG64 Ent0P1;
    ULONG64 Ent0P2;
    ULONG64 Ent1No;
    ULONG64 Ent1P0;
    ULONG64 Ent1P1;
    ULONG64 Ent1P2;
    ULONG EntryCount;
    ULONG64 BrkDbg0;
    ULONG64 BrkDbg1;
    ULONG64 BrkDbg2;
    ULONG64 BrkDbg3;
    ULONG64 BrkDbg4;
    ULONG64 BrkDbg5;
    ULONG BrkDumpCount;
    ULONG BrkRejectCount;
    ULONG64 BrkLastBad;
    ULONG BrkInitStatus;
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
    CHAR SlotText[8][48];
    ULONG64 SeekCount;
    ULONG64 LastSeekResult;
    ULONG PicoProcessObjectSet;
    ULONG64 ProbeMagicWritten;
    ULONG64 ProbeMagicReadback;
    ULONG ProbeMagicOk;
    ULONG MmapCount;
    ULONG64 LastMmapAddress;
    ULONG64 LastMmapStatus;
    ULONG64 MmapDebug[8];
    ULONG64 MmapContext[40];
    UCHAR MmapBytes[16];
    ULONG64 TlsBlockBase;
    ULONG64 TlsBlockSize;
    ULONG TlsSetupDone;
    ULONG64 TlsHeadStatus;
    ULONG64 TlsDataStatus;
    ULONG64 TlsHeadAddr;
    ULONG64 TlsCopyAttempted;
    ULONG64 TlsCopySrc;
    ULONG64 TlsCopyLen;
    ULONG64 TlsCopyStatus;
    ULONG64 TlsEntered;
    ULONG64 TlsInStubBase;
    ULONG64 TlsInStats;
    ULONG64 TlsInLibsUsed;
    ULONG64 TlsInLibsMemsz;
    ULONG64 TlsInLibsBase;
    ULONG64 TlsCallMark;
    ULONG64 TlsInTarget;
    ULONG64 FlagAddrUsed;
    ULONG64 FlagFromBase;
    ULONG64 ScanFound;
    ULONG64 ScanFlagDisp;
    ULONG64 ScanRoutinesDisp;
    ULONG64 ScanReached;
    ULONG64 ScanTried;
    ULONG64 RegAtExport;
    ULONG64 RegAtNtos;
    ULONG64 RegAtRegisterEntry;
    ULONG64 RegFnAddr;
    ULONG64 RegPtrAddr;
    ULONG64 RegExportAtRegister;
    ULONG64 RegNtosAtRegister;
    ULONG64 ScanFault;
    ULONG64 FlgAddrFromScan;
    ULONG64 RtnAddrFromScan;
    ULONG LibTlsOffset[4];
    ULONG64 LibTlsMemSize[4];
    ULONG64 LibTlsAllocated[4];
    ULONG LibRelrCount[4];
    ULONG LibRelrApplied[4];
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
    printf("  exportAddress=0x%llx hasExport=%lu", (unsigned long long)Q->ExportAddr, Q->HasExport);
    printf("%s", NLSTR);
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
    printf("  SCAN: found=%llu flagDisp=0x%llx routDisp=0x%llx flagAddr=0x%llx fromBase=%llu",
           (unsigned long long)Q->ScanFound,
           (unsigned long long)Q->ScanFlagDisp,
           (unsigned long long)Q->ScanRoutinesDisp,
           (unsigned long long)Q->FlagAddrUsed,
           (unsigned long long)Q->FlagFromBase);
    printf("  SCAN2: reached=%llu fault=%llu flagAddr2=0x%llx rtnAddr2=0x%llx",
           (unsigned long long)Q->ScanReached,
           (unsigned long long)Q->ScanFault,
           (unsigned long long)Q->FlgAddrFromScan,
           (unsigned long long)Q->RtnAddrFromScan);
    printf("  SCAN2: tried=%llu", (unsigned long long)Q->ScanTried);
    printf("  REGAT: export=0x%llx ntos=0x%llx entry=0x%llx",
           (unsigned long long)Q->RegAtExport,
           (unsigned long long)Q->RegAtNtos,
           (unsigned long long)Q->RegAtRegisterEntry);
    printf("%s", NLSTR);
    printf("  REGFN: fn=0x%llx ptr=0x%llx", (unsigned long long)Q->RegFnAddr, (unsigned long long)Q->RegPtrAddr);
    printf("%s", NLSTR);
    printf("  ATREG: export=0x%llx ntos=0x%llx",
           (unsigned long long)Q->RegExportAtRegister,
           (unsigned long long)Q->RegNtosAtRegister);
    printf("%s", NLSTR);
    printf("  SCAN2: reached=%llu fault=%llu flagAddr2=0x%llx rtnAddr2=0x%llx",
           (unsigned long long)Q->ScanReached,
           (unsigned long long)Q->ScanFault,
           (unsigned long long)Q->FlgAddrFromScan,
           (unsigned long long)Q->RtnAddrFromScan);
    printf("%s", "\n");
    printf("%s", "\n");
    printf("  TLS: entered=%llu inTarget=%llu callMark=0x%llx base=0x%llx size=0x%llx done=%lu",
           (unsigned long long)Q->TlsEntered,
           (unsigned long long)Q->TlsInTarget,
           (unsigned long long)Q->TlsCallMark,
           (unsigned long long)Q->TlsBlockBase,
           (unsigned long long)Q->TlsBlockSize,
           Q->TlsSetupDone);
    printf("%s", "\n");
    printf("  TLS: headAddr=0x%llx headSt=0x%llx dataSt=0x%llx copyTried=%llu copySt=0x%llx",
           (unsigned long long)Q->TlsHeadAddr,
           (unsigned long long)Q->TlsHeadStatus,
           (unsigned long long)Q->TlsDataStatus,
           (unsigned long long)Q->TlsCopyAttempted,
           (unsigned long long)Q->TlsCopyStatus);
    printf("%s", "\n");
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
    printf("  DYNAMIC: hasInterp=%lu interpVaddr=0x%llx\n", q.DynHasInterp,
           (unsigned long long)q.DynInterpVaddr);
    printf("  LOADTLS: entered=%llu callMark=0x%llx base=0x%llx size=0x%llx done=%llu\n",
           (unsigned long long)q.TlsEntered,
           (unsigned long long)q.TlsCallMark,
           (unsigned long long)q.TlsBlockBase,
           (unsigned long long)q.TlsBlockSize,
           q.TlsSetupDone);
    printf("  DYNAMIC: hasDyn=%lu dynVaddr=0x%llx dynSize=0x%llx entries=%lu\n", q.DynHasDynamic,
           (unsigned long long)q.DynDynamicVaddr, (unsigned long long)q.DynDynamicSize,
           q.DynDynamicCount);
    printf("  DYNAMIC: strtab=0x%llx symtab=0x%llx sonameOff=%llu needed=%lu\n",
           (unsigned long long)q.DynStrTabVaddr, (unsigned long long)q.DynSymTabVaddr,
           (unsigned long long)q.DynSonameOffset, q.DynNeededCount);
    printf("  DYNAMIC: rela=0x%llx relaSize=0x%llx relaCount=%llu\n",
           (unsigned long long)q.DynRelaVaddr, (unsigned long long)q.DynRelaSize,
           (unsigned long long)q.DynRelaCount);
    printf("  DYNAMIC: jmprel=0x%llx jmprelSize=0x%llx jmprelCount=%llu plgot=0x%llx\n",
           (unsigned long long)q.DynJmpRelVaddr, (unsigned long long)q.DynJmpRelSize,
           (unsigned long long)q.DynJmpRelCount, (unsigned long long)q.DynPltGotVaddr);
    printf("  RELOC: total=%lu relative=%lu jmpslot=%lu globdat=%lu unsupported=%lu writefail=%lu\n",
           q.RelocTotalCount, q.RelocRelativeCount, q.RelocJumpSlotCount, q.RelocGlobDatCount,
           q.RelocUnsupportedCount, q.RelocWriteFailCount);
    printf("  RELOC: firstOff=0x%llx addend=0x%llx wrote=0x%llx readback=0x%llx status=0x%08lx\n",
           (unsigned long long)q.RelocFirstOffset, (unsigned long long)q.RelocFirstAddend,
           (unsigned long long)q.RelocFirstWritten, (unsigned long long)q.RelocFirstReadBack,
           q.RelocFirstStatus);
    printf("  SYMS: total=%lu dumped=%lu truncated=%lu strtabOk=%lu strtabStatus=0x%08lx\n",
           q.SymTotal, q.SymDumped, q.SymTruncated, q.SymStrTabReadable, q.SymStrTabStatus);
    printf("  SIZE: probe=%lu drv=%lu %s\n",
           (unsigned long)sizeof(LX_QUERY_RESULT), (unsigned long)q.DrvStructSize,
           (sizeof(LX_QUERY_RESULT) == (size_t)q.DrvStructSize) ? "MATCH" : "MISMATCH");
    printf("  ENTRY: n=%lu [0] no=%lu p0=0x%llx p1=0x%llx p2=0x%llx\n",
           q.EntryCount, q.Ent0No, (unsigned long long)q.Ent0P0,
           (unsigned long long)q.Ent0P1, (unsigned long long)q.Ent0P2);
    printf("  ENTRY: [1] no=%lu p0=0x%llx p1=0x%llx p2=0x%llx\n",
           q.Ent1No, (unsigned long long)q.Ent1P0,
           (unsigned long long)q.Ent1P1, (unsigned long long)q.Ent1P2);
    printf("  BRKDUMP: n=%lu p0=0x%llx p1=0x%llx p2=0x%llx\n",
           q.BrkDumpCount, (unsigned long long)q.BrkDbg0,
           (unsigned long long)q.BrkDbg1, (unsigned long long)q.BrkDbg2);
    printf("  BRKDUMP: ctx=0x%llx arg0=0x%llx\n",
           (unsigned long long)q.BrkDbg3, (unsigned long long)q.BrkDbg4);
    printf("  BRKSTAT: ok=%lu reject=%lu lastBad=0x%llx\n",
           q.BrkOkCount, q.BrkRejectCount, (unsigned long long)q.BrkLastBad);
    printf("  STUBMAP: found=%lu slotUsed=%lu area=0x%llx\n",
           q.StubFound, q.StubSlotUsed, (unsigned long long)q.StubAreaBase);
    printf("  BUILTIN: written=%lu bytes=%lu fail=%lu registered=%lu status=0x%08lx\n",
           q.BuiltinWritten, q.BuiltinTotalBytes, q.BuiltinFailCount, q.BuiltinRegistered,
           q.BuiltinLastStatus);
    printf("  BRKVAR: addr=0x%llx initStatus=0x%08lx\n",
           (unsigned long long)q.BrkAddress, q.BrkInitStatus);
    printf("  BUILTIN: area=0x%llx last=%.48s @0x%llx\n",
           (unsigned long long)q.BuiltinAreaBase, q.BuiltinLastName,
           (unsigned long long)q.BuiltinLastAddress);
    printf("  GLOBAL: count=%lu/%lu probe=%lu hit=%lu crossOk=%lu crossMiss=%lu\n",
           q.GlobalCount, q.GlobalCapacity, q.GlobalProbeCount, q.GlobalProbeHit,
           q.GlobalCrossResolved, q.GlobalCrossMiss);
    printf("  GLOBAL: lastHit=%.48s @0x%llx lastMiss=%.48s\n",
           q.GlobalLastHitName, (unsigned long long)q.GlobalLastHitAddress, q.GlobalLastMissName);
    {
        ULONG gi;

        for (gi = 0; gi < q.GlobalCount && gi < 8; gi++)
        {
            printf("  GLOBAL: [%lu] %.48s\n", gi, q.GlobalFirstNames[gi]);
        }
    }
    printf("  LIBS: attempted=%lu loaded=%lu failed=%lu lastStatus=0x%08lx\n",
           q.LibAttempted, q.LibLoaded, q.LibFailed, q.LibLastStatus);
    printf("  LIBS: symbols=%lu relocs=%lu nameFail=%lu pathFail=%lu\n",
           q.LibTotalSymbols, q.LibTotalRelocs, q.LibNameResolveFail, q.LibPathResolveFail);
    {
        ULONG li;

        for (li = 0; li < 4; li++)
        {
            if (q.LibUsed[li] == 0)
            {
                continue;
            }

            printf("  LIBS: [%lu] name=%.48s status=0x%08lx base=0x%llx size=0x%llx\n", li,
                   q.LibNames[li], q.LibStatus[li], (unsigned long long)q.LibBase[li],
                   (unsigned long long)q.LibImageSize[li]);
            printf("  LIBS: [%lu] syms=%lu rela=%lu applied=%lu needed=%lu\n", li,
                   q.LibSymCount[li], q.LibRelaCount[li], q.LibRelaApplied[li], q.LibNeededCount[li]);
            printf("  LIBS: [%lu] symtab=0x%llx firstSymValue=0x%llx\n", li,
                   (unsigned long long)q.LibSymTab[li], (unsigned long long)q.LibFirstSymValue[li]);
            printf("  LIBS: [%lu] got12C0=0x%llx\n", li, (unsigned long long)q.LibGot12C0[li]);
            printf("  LIBS: [%lu] relr=%lu relrApplied=%lu\n", li, q.LibRelrCount[li], q.LibRelrApplied[li]);
            printf("  LIBS: [%lu] tls=off:%lu memsz=%llu alloc=%llu\n", li,
                   q.LibTlsOffset[li], (unsigned long long)q.LibTlsMemSize[li],
                   (unsigned long long)q.LibTlsAllocated[li]);
            printf("  DIAG: globalSinm=0x%llx mainGot1A18=0x%llx\n", (unsigned long long)q.DiagGlobalSinm, (unsigned long long)q.DiagMainGot);
    }
        printf("  TLS: base=0x%llx size=0x%llx done=%lu\n",
               (unsigned long long)q.TlsBlockBase, (unsigned long long)q.TlsBlockSize, q.TlsSetupDone);
        printf("  TLS: headAddr=0x%llx headSt=0x%llx dataSt=0x%llx\n",
               (unsigned long long)q.TlsHeadAddr,
               (unsigned long long)q.TlsHeadStatus,
               (unsigned long long)q.TlsDataStatus);
        printf("  TLS: copyTried=%llu src=0x%llx len=%llu st=0x%llx\n",
               (unsigned long long)q.TlsCopyAttempted,
               (unsigned long long)q.TlsCopySrc,
               (unsigned long long)q.TlsCopyLen,
               (unsigned long long)q.TlsCopyStatus);
        printf("  TLS: entered=%llu stubBase=0x%llx stats=0x%llx used=%llu memsz=%llu libBase=0x%llx\n",
               (unsigned long long)q.TlsEntered,
               (unsigned long long)q.TlsInStubBase,
               (unsigned long long)q.TlsInStats,
               (unsigned long long)q.TlsInLibsUsed,
               (unsigned long long)q.TlsInLibsMemsz,
               (unsigned long long)q.TlsInLibsBase);
        printf("  TLS: callMark=0x%llx\n", (unsigned long long)q.TlsCallMark);
        printf("  TLS: inTarget=%llu\n", (unsigned long long)q.TlsInTarget);
        printf("  SCAN: found=%llu flagDisp=0x%llx routDisp=0x%llx flagAddr=0x%llx fromBase=%llu\n",
               (unsigned long long)q.ScanFound,
               (unsigned long long)q.ScanFlagDisp,
               (unsigned long long)q.ScanRoutinesDisp,
               (unsigned long long)q.FlagAddrUsed,
               (unsigned long long)q.FlagFromBase);
    }
    printf("  STUBS: count=%lu found=%lu unknown=%lu writeFail=%lu area=0x%llx\n",
           q.StubCount, q.StubFound, q.StubUnknown, q.StubWriteFail,
           (unsigned long long)q.StubAreaBase);
    printf("  STUBS: lastAddr=0x%llx syscall=%llu status=0x%08lx name=%.48s\n",
           (unsigned long long)q.StubLastAddress, (unsigned long long)q.StubLastSyscall,
           q.StubLastStatus, q.StubLastName);
    printf("  SYMS: nameFileOff=0x%llx byte0=0x%lx byte1=0x%lx\n",
           (unsigned long long)q.SymNameFileOffset, q.SymNameByte0, q.SymNameByte1);
    printf("  SYMS: symtab=0x%llx strtab=0x%llx\n", (unsigned long long)q.SymSymTabVaddr,
           (unsigned long long)q.SymStrTabVaddr);
    {
        ULONG si;

        for (si = 0; si < q.SymDumped && si < 16; si++)
        {
            printf("  SYMS: [%lu] off=0x%lx nameOff=%lu name=%.48s\n", si,
                   q.SymRelocOffsets[si], q.SymNameOffsets[si], q.SymNames[si]);
        }
    }
    printf("  DYNAMIC: init=0x%llx fini=0x%llx\n", (unsigned long long)q.DynInitVaddr,
           (unsigned long long)q.DynFiniVaddr);
    {
        ULONG ni;

        for (ni = 0; ni < q.DynNeededCount && ni < 8; ni++)
        {
            printf("  DYNAMIC: needed[%lu] strOff=%llu\n", ni, (unsigned long long)q.DynNeededOffsets[ni]);
        }
    }
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
        SendLoadElf(device, L"C:\\Windows\\dy.x64");

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
            printf("  readDebug=%llx %llx %llx %llx -> fd=%llx buf=%llx cnt=%llx\n",
                   (unsigned long long)q.ReadDebug[0], (unsigned long long)q.ReadDebug[1],
                   (unsigned long long)q.ReadDebug[2], (unsigned long long)q.ReadDebug[3],
                   (unsigned long long)q.ReadDebug[4], (unsigned long long)q.ReadDebug[5],
                   (unsigned long long)q.ReadDebug[6]);
printf("  writeLen=%lu\n", q.WriteLength);
            fflush(stdout);
            fprintf(stderr, "  RAWDBG len=%lu bytes:", q.WriteLength);
            {
                ULONG di;

                for (di = 0; di < q.WriteLength && di < 128; di++)
                {
                    unsigned char ch = (unsigned char)q.WriteBuffer[di];

                    if (ch >= 32 && ch < 127)
                    {
                        fprintf(stderr, " %c", ch);
                    }
                    else
                    {
                        fprintf(stderr, " %02x", ch);
                    }
                }

                fprintf(stderr, "\n");
                fflush(stderr);
            }
            {
                unsigned char *raw = (unsigned char *)q.WriteBuffer;
                unsigned long long off = 0;

                fprintf(stderr, "  DIRENT parsed:\n");

                while (off + 19 <= q.WriteLength)
                {
                    unsigned long long ino;
                    unsigned long long doff;
                    unsigned short reclen;
                    unsigned char dtype;
                    char nm[260];
                    int k;

                    memcpy(&ino, raw + off + 0, 8);
                    memcpy(&doff, raw + off + 8, 8);
                    memcpy(&reclen, raw + off + 16, 2);
                    dtype = raw[off + 18];

                    if (reclen == 0 || off + reclen > q.WriteLength)
                    {
                        fprintf(stderr, "    [stop off=%llu reclen=%u]\n", off, reclen);
                        break;
                    }

                    for (k = 0; k < 255 && (unsigned)(off + 19 + k) < q.WriteLength; k++)
                    {
                        nm[k] = (char)raw[off + 19 + k];

                        if (nm[k] == 0)
                        {
                            break;
                        }
                    }

                    nm[k] = 0;
                    fprintf(stderr, "    off=%llu ino=%llu type=%u name=[%s]\n",
                            off, ino, dtype, nm);
                    off += reclen;
                }

                fflush(stderr);
            }
            printf("  picoProcessObjectSet=%lu\n", q.PicoProcessObjectSet);
            printf("  mmapCount=%lu lastMmap=0x%llx mmapStatus=0x%llx\n", q.MmapCount,
                   (unsigned long long)q.LastMmapAddress,
                   (unsigned long long)q.LastMmapStatus);
            printf("  mmapDebug=%llx %llx %llx %llx %llx %llx fdok=%llx handle=%llx\n",
                   (unsigned long long)q.MmapDebug[0], (unsigned long long)q.MmapDebug[1],
                   (unsigned long long)q.MmapDebug[2], (unsigned long long)q.MmapDebug[3],
                   (unsigned long long)q.MmapDebug[4], (unsigned long long)q.MmapDebug[5],
                   (unsigned long long)q.MmapDebug[6], (unsigned long long)q.MmapDebug[7]);
            printf("  mmapBytes:");
            {
                int bi;

                for (bi = 0; bi < 16; bi++)
                {
                    printf(" %02x", q.MmapBytes[bi]);
                }

                printf("\n");
            }
            printf("  magicOk=%lu written=0x%llx readback=0x%llx\n", q.ProbeMagicOk,
                   (unsigned long long)q.ProbeMagicWritten, (unsigned long long)q.ProbeMagicReadback);
            printf("  seekCount=%llu lastSeek=0x%llx\n", (unsigned long long)q.SeekCount,
                   (unsigned long long)q.LastSeekResult);
            printf("  statCount=%llu lastStatStatus=0x%llx size=%lld mode=0%lo path=[%.*s]\n",
                   (unsigned long long)q.StatCount, (unsigned long long)q.LastStatStatus,
                   (long long)q.LastStatSize, q.LastStatMode, (int)sizeof(q.LastStatPath), q.LastStatPath);
            printf("  statDebug=%llx %llx %llx %llx %llx %llx used=%llx\n",
                   (unsigned long long)q.StatDebug[0], (unsigned long long)q.StatDebug[1],
                   (unsigned long long)q.StatDebug[2], (unsigned long long)q.StatDebug[3],
                   (unsigned long long)q.StatDebug[4], (unsigned long long)q.StatDebug[5],
                   (unsigned long long)q.StatDebug[6]);
            {
                int si;
                for (si = 0; si < 8; si++)
                {
                    printf("    slot[%d]=0x%llx text=[%s]\n", si,
                           (unsigned long long)q.StatDebug[si], q.SlotText[si]);
                }
            }
            printf("  getdentsCount=%lu status=0x%llx bytes=%llu dir=[%.*s]\n", q.GetDentsCount,
                   (unsigned long long)q.GetDentsStatus, (unsigned long long)q.GetDentsBytes,
                   (int)sizeof(q.LastDirectoryPath), q.LastDirectoryPath);
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
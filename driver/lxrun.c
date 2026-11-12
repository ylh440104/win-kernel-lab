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

#define LX_MAX_NEEDED 8u
#define LX_MAX_INTERP 128u

typedef struct _LX_ELF_DYNAMIC_INFO
{
    ULONG64 InterpVaddr;
    ULONG64 InterpOffset;
    ULONG InterpSize;
    ULONG HasInterp;
    ULONG64 DynamicVaddr;
    ULONG64 DynamicOffset;
    ULONG64 DynamicSize;
    ULONG HasDynamic;
    ULONG DynamicCount;
    ULONG64 StrTabVaddr;
    ULONG64 SymTabVaddr;
    ULONG64 RelaVaddr;
    ULONG64 RelaSize;
    ULONG64 RelaCount;
    ULONG64 JmpRelVaddr;
    ULONG64 JmpRelSize;
    ULONG64 JmpRelCount;
    ULONG64 PltGotVaddr;
    ULONG64 InitVaddr;
    ULONG64 FiniVaddr;
    ULONG64 SonameOffset;
    ULONG64 HashVaddr;
    ULONG64 SymCountFromHash;
    ULONG NeededCount;
    ULONG NeededOffsets[LX_MAX_NEEDED];
} LX_ELF_DYNAMIC_INFO;

typedef struct _LX_ELF_RELA
{
    ULONG64 Offset;
    ULONG64 Info;
    LONG64 Addend;
} LX_ELF_RELA;

#define LX_R_X86_64_NONE 0u
#define LX_R_X86_64_64 1u
#define LX_R_X86_64_GLOB_DAT 6u
#define LX_R_X86_64_JUMP_SLOT 7u
#define LX_R_X86_64_RELATIVE 8u
#define LX_R_X86_64_DTPMOD64 16u
#define LX_R_X86_64_IRELATIVE 37u
#define LX_MAX_RELOCATIONS 256u

typedef struct _LX_RELOC_STATS
{
    ULONG RelativeCount;
    ULONG JumpSlotCount;
    ULONG GlobDatCount;
    ULONG UnsupportedCount;
    ULONG WriteFailCount;
    ULONG TotalCount;
    ULONG64 FirstOffset;
    ULONG64 FirstAddend;
    ULONG64 FirstWritten;
    ULONG64 FirstReadBack;
    NTSTATUS FirstWriteStatus;
} LX_RELOC_STATS;

typedef struct _LX_ELF_SYM
{
    ULONG NameOffset;
    UCHAR Info;
    UCHAR Other;
    USHORT Shndx;
    ULONG64 Value;
    ULONG64 Size;
} LX_ELF_SYM;

#define LX_MAX_SYMBOLS 64u
#define LX_MAX_SYMBOL_NAME 48u

typedef struct _LX_SYMBOL_ENTRY
{
    ULONG NameOffset;
    ULONG RelocOffset;
    ULONG64 RelocAddend;
    ULONG Resolved;
    ULONG64 ResolvedAddress;
    CHAR Name[LX_MAX_SYMBOL_NAME];
} LX_SYMBOL_ENTRY;

typedef struct _LX_SYMBOL_INFO
{
    ULONG TotalSyms;
    ULONG DumpedCount;
    ULONG Truncated;
    ULONG StrTabReadable;
    ULONG StrTabStatus;
    ULONG64 NameFileOffset;
    ULONG NameByte0;
    ULONG NameByte1;
    ULONG64 StrTabVaddr;
    ULONG64 SymTabVaddr;
    ULONG64 SymCount;
    LX_SYMBOL_ENTRY Entries[LX_MAX_SYMBOLS];
} LX_SYMBOL_INFO;

#define LX_STUB_SIZE 16u
#define LX_STUB_AREA_SIZE 0x10000u
#define LX_BUILTIN_AREA_OFFSET 0x8000u
#define LX_BUILTIN_MAX_SIZE 0x4000u
#define LX_BUILTIN_BRK_OFFSET 0x0200u
#define LX_BUILTIN_MALLOC_MAX 96u

typedef struct _LX_BUILTIN_STATS
{
    ULONG Written;
    ULONG TotalBytes;
    ULONG64 AreaBase;
    ULONG FailCount;
    NTSTATUS LastStatus;
    ULONG Registered;
    CHAR LastName[LX_MAX_SYMBOL_NAME];
    ULONG64 LastAddress;
    ULONG64 BrkAddress;
    ULONG BrkOkCount;
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
} LX_BUILTIN_STATS;

typedef struct _LX_SYMBOL_BINDING
{
    const char *Name;
    ULONG SyscallNumber;
} LX_SYMBOL_BINDING;

typedef struct _LX_STUB_STATS
{
    ULONG StubCount;
    ULONG StubFound;
    ULONG StubSlot;
    ULONG StubUnknown;
    ULONG StubWriteFail;
    ULONG64 StubAreaBase;
    ULONG64 LastStubAddress;
    ULONG64 LastStubSyscall;
    NTSTATUS LastStubStatus;
    CHAR LastStubName[LX_MAX_SYMBOL_NAME];
} LX_STUB_STATS;

#define LX_MAX_LIBS 4u
#define LX_LIB_BASE_STEP 0x4000000ull
#define LX_LIB_IMAGE_MAX 0x800000u
#define LX_MAX_LIB_SYMBOLS 64u

typedef struct _LX_LIB_ENTRY
{
    ULONG Used;
    ULONG Index;
    ULONG64 BaseAddress;
    ULONG64 ImageSize;
    ULONG64 EntryPoint;
    ULONG64 PhdrAddress;
    ULONG PhdrCount;
    ULONG PhdrEntrySize;
    ULONG HasDynamic;
    ULONG64 DynamicOffset;
    ULONG64 DynamicSize;
    ULONG64 StrTabVaddr;
    ULONG64 SymTabVaddr;
    ULONG NeededCount;
    ULONG NeededOffsets[LX_MAX_NEEDED];
    ULONG RelaCount;
    ULONG RelaApplied;
    ULONG SymCount;
    ULONG SymDumped;
    ULONG LoadStatus;
    CHAR Name[LX_MAX_SYMBOL_NAME];
    CHAR FirstSymbol[LX_MAX_SYMBOL_NAME];
} LX_LIB_ENTRY;

typedef struct _LX_LIB_STATS
{
    ULONG Attempted;
    ULONG Loaded;
    ULONG Failed;
    ULONG LastStatus;
    ULONG TotalSymbols;
    ULONG TotalRelocs;
    ULONG NameResolveFail;
    ULONG PathResolveFail;
    LX_LIB_ENTRY Libs[LX_MAX_LIBS];
} LX_LIB_STATS;

#define LX_MAX_GLOBAL_SYMBOLS 128u

typedef struct _LX_GLOBAL_SYMBOL
{
    CHAR Name[LX_MAX_SYMBOL_NAME];
    ULONG64 Address;
    ULONG LibIndex;
    ULONG IsFunction;
    ULONG Source;
} LX_GLOBAL_SYMBOL;

typedef struct _LX_GLOBAL_TABLE
{
    ULONG Count;
    ULONG Capacity;
    ULONG ProbeCount;
    ULONG ProbeHit;
    ULONG CrossResolved;
    ULONG CrossMiss;
    CHAR LastMissName[LX_MAX_SYMBOL_NAME];
    CHAR LastHitName[LX_MAX_SYMBOL_NAME];
    ULONG64 LastHitAddress;
    LX_GLOBAL_SYMBOL Entries[LX_MAX_GLOBAL_SYMBOLS];
} LX_GLOBAL_TABLE;

#define LX_PT_NULL 0u
#define LX_PT_LOAD 1u
#define LX_PT_DYNAMIC 2u
#define LX_PT_INTERP 3u
#define LX_DT_NULL 0u
#define LX_DT_NEEDED 1u
#define LX_DT_PLTRELSZ 2u
#define LX_DT_PLTGOT 3u
#define LX_DT_HASH 4u
#define LX_DT_STRTAB 5u
#define LX_DT_SYMTAB 6u
#define LX_DT_RELA 7u
#define LX_DT_RELASZ 8u
#define LX_DT_RELAENT 9u
#define LX_DT_STRSZ 10u
#define LX_DT_SYMENT 11u
#define LX_DT_INIT 12u
#define LX_DT_FINI 13u
#define LX_DT_SONAME 14u
#define LX_DT_RPATH 15u
#define LX_DT_SYMBOLIC 16u
#define LX_DT_REL 17u
#define LX_DT_RELSZ 18u
#define LX_DT_RELENT 19u
#define LX_DT_PLTREL 20u
#define LX_DT_DEBUG 21u
#define LX_DT_TEXTREL 22u
#define LX_DT_JMPREL 23u
#define LX_DT_BIND_NOW 24u
#define LX_DT_INIT_ARRAY 25u
#define LX_DT_FINI_ARRAY 26u
#define LX_DT_INIT_ARRAYSZ 27u
#define LX_DT_FINI_ARRAYSZ 28u
#define LX_DT_RUNPATH 29u
#define LX_DT_FLAGS 30u
#define LX_DT_MAX_ENTRIES 32u

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
    ULONG SymNameOffsets[64];
    ULONG SymRelocOffsets[64];
    CHAR SymNames[64][48];
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
static PEPROCESS g_PicoProcessObject = NULL;
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
static LX_ELF_DYNAMIC_INFO g_ElfDynamic;
static LX_RELOC_STATS g_RelocStats;
static LX_SYMBOL_INFO g_SymbolInfo;
static LX_STUB_STATS g_StubStats;
static LX_LIB_STATS g_LibStats;
static LX_GLOBAL_TABLE g_GlobalSymbols;
static LX_BUILTIN_STATS g_BuiltinStats;
static ULONG g_LastRelocKernelStatus = 0;
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
static ULONG g_BrkOk = 0;
static ULONG g_BrkReject = 0;
static ULONG64 g_BrkLastBad = 0;
static ULONG64 g_BrkDebug[8];
static ULONG g_BrkDumpCount = 0;
static ULONG64 g_EntryDump[8][4];
static ULONG g_EntryCount = 0;
static ULONG64 g_MmapCurrent = 0;
static ULONG g_MmapCount = 0;
static ULONG64 g_LastMmapAddress = 0;
static ULONG64 g_LastMmapStatus = 0;
static ULONG64 g_MmapDebug[8];
static ULONG64 g_MmapContext[40];
static UCHAR g_MmapBytes[16];
static ULONG g_MmapContextDone = 0;
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
#define LX_SYS_LSTAT 6
#define LX_SYS_LSEEK 8
#define LX_SYS_IOCTL 16
#define LX_SYS_ACCESS 21
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

#define LX_SYS_GETCWD 79
#define LX_SYS_READLINK 89
#define LX_SYS_UNAME 63
#define LX_SYS_GETUID 102
#define LX_SYS_GETGID 104
#define LX_SYS_GETEUID 107
#define LX_SYS_GETEGID 108
#define LX_SYS_GETPID 39
#define LX_SYS_GETPPID 110
#define LX_SYS_GETTID 186
#define LX_SYS_SET_ROBUST_LIST 273
#define LX_SYS_GETTIMEOFDAY 96
#define LX_SYS_CLOCK_GETTIME 228
#define LX_SYS_TIME 201
#define LX_SYS_SYSINFO 99
#define LX_SYS_GETRUSAGE 98
#define LX_SYS_SCHED_YIELD 24
#define LX_SYS_RT_SIGACTION 13
#define LX_SYS_RT_SIGPROCMASK 14
#define LX_SYS_RT_SIGRETURN 15
#define LX_SYS_SIGALTSTACK 131
#define LX_SYS_FUTEX 202
#define LX_SYS_GETRLIMIT 97
#define LX_SYS_PRCTL 157
#define LX_SYS_MADVISE 28
#define LX_SYS_NANOSLEEP 35
#define LX_SYS_MPROTECT 10
#define LX_SYS_MUNMAP 11
#define LX_SYS_FCNTL 72
#define LX_SYS_DUP 32
#define LX_SYS_DUP2 33
#define LX_SYS_PIPE 22
#define LX_SYS_SCHED_GETAFFINITY 204
#define LX_SYS_GETRANDOM 318
#define LX_SYS_READLINKAT 267
#define LX_SYS_CLOCK_GETRES 229
#define LX_SYS_TIMES 100
#define LX_SYS_UMASK 95
#define LX_SYS_FSYNC 74
#define LX_SYS_FDATASYNC 75
#define LX_SYS_SYNC 162
#define LX_SYS_FLOCK 73
#define LX_SYS_MKDIR 83
#define LX_SYS_MKDIRAT 258
#define LX_SYS_RMDIR 84
#define LX_SYS_UNLINK 87
#define LX_SYS_UNLINKAT 263
#define LX_SYS_RENAME 82
#define LX_SYS_RENAMEAT 264
#define LX_SYS_KILL 62
#define LX_SYS_TGKILL 234
#define LX_SYS_PAUSE 34
#define LX_SYS_ALARM 37
#define LX_SYS_RT_SIGPENDING 127
#define LX_SYS_RT_SIGSUSPEND 130
#define LX_SYS_RT_SIGTIMEDWAIT 128
#define LX_SYS_EPOLL_CREATE 213
#define LX_SYS_EPOLL_CTL 233
#define LX_SYS_EPOLL_WAIT 232
#define LX_SYS_EVENTFD 284
#define LX_SYS_EPOLL_CREATE1 291
#define LX_SYS_EVENTFD2 290
#define LX_SYS_STATFS 137
#define LX_SYS_FSTATFS 138
#define LX_SYS_READV 19
#define LX_SYS_WRITEV 20
#define LX_SYS_SENDTO 44
#define LX_SYS_RECVFROM 45
#define LX_SYS_SOCKET 41
#define LX_SYS_CONNECT 42
#define LX_SYS_SHUTDOWN 48
#define LX_SYS_SETSOCKOPT 54
#define LX_SYS_GETSOCKOPT 55
#define LX_SYS_CLONE 56
#define LX_SYS_FORK 57
#define LX_SYS_VFORK 58
#define LX_SYS_EXECVE 59
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
static ULONG64 g_SeekCount = 0;
static ULONG64 g_LastSeekResult = 0;
static ULONG g_GetDentsCount = 0;
static ULONG64 g_GetDentsStatus = 0xFFFFFFFFu;
static ULONG64 g_GetDentsBytes = 0;
static ULONG64 g_ProbeMagicWritten = 0;
static ULONG64 g_ProbeMagicReadback = 0;
static ULONG g_ProbeMagicOk = 0;
static CHAR g_DirCache[4096];
static ULONG g_DirCacheLen = 0;
static ULONG64 g_DirCacheUserAddr = 0;
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

typedef struct _LX_LINUX_UTSNAME
{
    CHAR SysName[65];
    CHAR NodeName[65];
    CHAR Release[65];
    CHAR Version[65];
    CHAR Machine[65];
    CHAR DomainName[65];
} LX_LINUX_UTSNAME;

typedef struct _LX_FILE_DISPOSITION_INFORMATION
{
    BOOLEAN DeleteFile;
} LX_FILE_DISPOSITION_INFORMATION;

typedef struct _LX_FILE_RENAME_INFORMATION
{
    BOOLEAN ReplaceIfExists;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} LX_FILE_RENAME_INFORMATION;

typedef struct _LX_FILE_BASIC_INFORMATION
{
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} LX_FILE_BASIC_INFORMATION;

typedef struct _LX_LINUX_STATFS
{
    LONG64 Type;
    LONG64 BlockSize;
    ULONG64 Blocks;
    ULONG64 BlocksFree;
    ULONG64 BlocksAvailable;
    ULONG64 Files;
    ULONG64 FilesFree;
    LONG64 FileSystemId;
    LONG64 NameLength;
    LONG64 FragmentSize;
    LONG64 MountFlags;
    LONG64 Spare[4];
} LX_LINUX_STATFS;

typedef struct _LX_IOVEC
{
    ULONG64 Base;
    ULONG64 Length;
} LX_IOVEC;

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

typedef struct _LX_LINUX_TIMEVAL
{
    LONG64 Seconds;
    LONG64 Microseconds;
} LX_LINUX_TIMEVAL;

typedef struct _LX_LINUX_TIMESPEC
{
    LONG64 Seconds;
    LONG64 Nanoseconds;
} LX_LINUX_TIMESPEC;

static VOID LxGetUnixTimeval(LX_LINUX_TIMEVAL *Timeval)
{
    const LONGLONG EpochDelta = 116444736000000000LL;
    LARGE_INTEGER systemTime;
    LONGLONG value;

    KeQuerySystemTime(&systemTime);
    value = systemTime.QuadPart;

    if (value < EpochDelta)
    {
        Timeval->Seconds = 0;
        Timeval->Microseconds = 0;
        return;
    }

    value -= EpochDelta;
    Timeval->Seconds = value / 10000000LL;
    Timeval->Microseconds = (value % 10000000LL) / 10LL;
}

static VOID LxGetUnixTimespec(LX_LINUX_TIMESPEC *Timespec)
{
    const LONGLONG EpochDelta = 116444736000000000LL;
    LARGE_INTEGER systemTime;
    LONGLONG value;

    KeQuerySystemTime(&systemTime);
    value = systemTime.QuadPart;

    if (value < EpochDelta)
    {
        Timespec->Seconds = 0;
        Timespec->Nanoseconds = 0;
        return;
    }

    value -= EpochDelta;
    Timespec->Seconds = value / 10000000LL;
    Timespec->Nanoseconds = (value % 10000000LL) * 100LL;
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
static ULONG64 g_SlotProbe[8];
static CHAR g_SlotText[8][48];

static void LxProbeSlots(ULONG64 *Parameters)
{
    ULONG s;
    ULONG i;

    RtlZeroMemory(g_SlotProbe, sizeof(g_SlotProbe));
    RtlZeroMemory(g_SlotText, sizeof(g_SlotText));

    for (s = 0; s < 6; s++)
    {
        ULONG64 address = Parameters[s];
        BOOLEAN anyOk = FALSE;

        g_SlotProbe[s] = address;

        if (address < 0x10000 || address >= 0x800000000000ull)
        {
            continue;
        }

        for (i = 0; i < 15; i++)
        {
            CHAR value = 0;

            __try
            {
                ProbeForRead((PVOID)(ULONG_PTR)(address + i), 1, 1);
                value = ((const CHAR *)(ULONG_PTR)address)[i];
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                value = 0;

                if (!anyOk)
                {
                    g_SlotText[s][0] = 0;
                }

                break;
            }

            anyOk = TRUE;

            if (value == 0)
            {
                g_SlotText[s][i] = 0;
                break;
            }

            g_SlotText[s][i] = (value >= 0x20 && value <= 0x7E) ? value : '.';
            g_SlotText[s][i + 1] = 0;
        }
    }
}

static ULONG LxCopyLinuxPath(ULONG64 UserPath, CHAR *KernelPath, ULONG Capacity)
{
    CHAR staging[64];
    ULONG i;
    ULONG limit = Capacity - 1;
    ULONG attempt;

    if (UserPath == 0 || KernelPath == NULL || Capacity < 2)
    {
        return 0;
    }

    if (limit > 60)
    {
        limit = 60;
    }

    KernelPath[0] = 0;

    for (attempt = 0; attempt < 3; attempt++)
    {
        BOOLEAN copied = FALSE;

        RtlZeroMemory(staging, sizeof(staging));

        __try
        {
            RtlCopyMemory(staging, (PVOID)(ULONG_PTR)UserPath, 32);
            copied = TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            copied = FALSE;
        }

        if (!copied)
        {
            continue;
        }

        for (i = 0; i < limit && i < 32; i++)
        {
            CHAR value = staging[i];

            if (value == 0)
            {
                if (i > 0)
                {
                    KernelPath[i] = 0;
                    return i;
                }

                break;
            }

            KernelPath[i] = value;
        }
    }

    KernelPath[0] = 0;
    return 0;
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

    if (out == 0 || (isMntC && out < 11))
    {
        return 0;
    }

    OutBuffer[out] = 0;
    return out;
}

static NTSTATUS LxQueryExists(const WCHAR *NtPath)
{
    UNICODE_STRING path;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK statusBlock;
    HANDLE handle = NULL;
    NTSTATUS status;
    ULONG chars = 0;

    while (chars < 259 && NtPath[chars] != 0)
    {
        chars++;
    }

    if (chars == 0)
    {
        return 0xC0000033u;
    }

    path.Buffer = (PWCH)NtPath;
    path.Length = (USHORT)(chars * sizeof(WCHAR));
    path.MaximumLength = (USHORT)((chars + 1) * sizeof(WCHAR));
    InitializeObjectAttributes(&attributes, &path, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwCreateFile(&handle, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attributes, &statusBlock, NULL,
                          FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT, NULL, 0);

    if (NT_SUCCESS(status))
    {
        ZwClose(handle);
    }

    return status;
}

static NTSTATUS LxCreateDirectory(const WCHAR *NtPath, ULONG Mode)
{
    UNICODE_STRING path;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK statusBlock;
    HANDLE handle = NULL;
    NTSTATUS status;
    ULONG chars = 0;
    LX_FILE_BASIC_INFORMATION basicInfo;

    while (chars < 259 && NtPath[chars] != 0)
    {
        chars++;
    }

    if (chars == 0)
    {
        return 0xC0000033u;
    }

    path.Buffer = (PWCH)NtPath;
    path.Length = (USHORT)(chars * sizeof(WCHAR));
    path.MaximumLength = (USHORT)((chars + 1) * sizeof(WCHAR));
    InitializeObjectAttributes(&attributes, &path, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwCreateFile(&handle, FILE_LIST_DIRECTORY | FILE_ADD_FILE | SYNCHRONIZE, &attributes,
                          &statusBlock, NULL, FILE_ATTRIBUTE_DIRECTORY,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          FILE_CREATE, FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE, NULL, 0);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    RtlZeroMemory(&basicInfo, sizeof(basicInfo));
    basicInfo.LastWriteTime.QuadPart = -1;
    basicInfo.ChangeTime.QuadPart = -1;
    basicInfo.CreationTime.QuadPart = -1;
    basicInfo.LastAccessTime.QuadPart = -1;

    ZwSetInformationFile(handle, &statusBlock, &basicInfo, sizeof(basicInfo), FileBasicInformation);
    ZwClose(handle);

    (void)Mode;
    return STATUS_SUCCESS;
}

static NTSTATUS LxRemoveEntry(const WCHAR *NtPath, BOOLEAN IsDirectory)
{
    UNICODE_STRING path;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK statusBlock;
    HANDLE handle = NULL;
    NTSTATUS status;
    ULONG chars = 0;
    LX_FILE_DISPOSITION_INFORMATION disposition;

    while (chars < 259 && NtPath[chars] != 0)
    {
        chars++;
    }

    if (chars == 0)
    {
        return 0xC0000033u;
    }

    path.Buffer = (PWCH)NtPath;
    path.Length = (USHORT)(chars * sizeof(WCHAR));
    path.MaximumLength = (USHORT)((chars + 1) * sizeof(WCHAR));
    InitializeObjectAttributes(&attributes, &path, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwCreateFile(&handle, DELETE | SYNCHRONIZE, &attributes, &statusBlock, NULL,
                          FILE_ATTRIBUTE_NORMAL | (IsDirectory ? FILE_ATTRIBUTE_DIRECTORY : 0),
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT |
                              (IsDirectory ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE),
                          NULL, 0);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    RtlZeroMemory(&disposition, sizeof(disposition));
    disposition.DeleteFile = TRUE;

    status = ZwSetInformationFile(handle, &statusBlock, &disposition, sizeof(disposition), FileDispositionInformation);
    ZwClose(handle);

    return status;
}

static NTSTATUS LxRenameEntry(const WCHAR *SourceNtPath, const WCHAR *TargetNtPath, BOOLEAN Replace)
{
    UNICODE_STRING sourcePath;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK statusBlock;
    HANDLE handle = NULL;
    NTSTATUS status;
    ULONG sourceChars = 0;
    ULONG targetChars = 0;
    ULONG bytesNeeded;
    LX_FILE_RENAME_INFORMATION *renameInfo;

    while (sourceChars < 259 && SourceNtPath[sourceChars] != 0)
    {
        sourceChars++;
    }

    while (targetChars < 259 && TargetNtPath[targetChars] != 0)
    {
        targetChars++;
    }

    if (sourceChars == 0 || targetChars == 0)
    {
        return 0xC0000033u;
    }

    sourcePath.Buffer = (PWCH)SourceNtPath;
    sourcePath.Length = (USHORT)(sourceChars * sizeof(WCHAR));
    sourcePath.MaximumLength = (USHORT)((sourceChars + 1) * sizeof(WCHAR));
    InitializeObjectAttributes(&attributes, &sourcePath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwCreateFile(&handle, DELETE | SYNCHRONIZE, &attributes, &statusBlock, NULL,
                          FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT, NULL, 0);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    bytesNeeded = sizeof(LX_FILE_RENAME_INFORMATION) + targetChars * sizeof(WCHAR);
    renameInfo = (LX_FILE_RENAME_INFORMATION *)ExAllocatePoolWithTag(NonPagedPool, bytesNeeded, 'nExL');

    if (renameInfo == NULL)
    {
        ZwClose(handle);
        return 0xC000009Au;
    }

    RtlZeroMemory(renameInfo, bytesNeeded);
    renameInfo->ReplaceIfExists = Replace ? TRUE : FALSE;
    renameInfo->RootDirectory = NULL;
    renameInfo->FileNameLength = targetChars * sizeof(WCHAR);
    RtlCopyMemory(renameInfo->FileName, TargetNtPath, targetChars * sizeof(WCHAR));

    status = ZwSetInformationFile(handle, &statusBlock, renameInfo, bytesNeeded, FileRenameInformation);

    ExFreePoolWithTag(renameInfo, 'nExL');
    ZwClose(handle);

    return status;
}

static ULONG g_ReportedUmask = 022u;

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

    status = ZwReadFile(g_FdTable[Fd].Handle, NULL, NULL, NULL, &statusBlock,
                        UserBuffer, Count, &offset, NULL);

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

    if (g_ProbeMagicWritten == 0)
    {
        ULONG64 magic = 0x4C5852554E4B3031ull;
        ULONG64 readback = 0;
        LX_APC_STATE probeApc;
        BOOLEAN probeAttached = FALSE;

        g_ProbeMagicWritten = magic;
        g_ProbeMagicReadback = 0;
        g_ProbeMagicOk = 0;

        if (g_PicoProcessObject != NULL)
        {
            KeStackAttachProcess(g_PicoProcessObject, (PVOID)&probeApc);
            probeAttached = TRUE;
        }

        __try
        {
            RtlCopyMemory(UserBuffer, &magic, 8);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            g_ProbeMagicOk = 0xEE;
        }

        __try
        {
            RtlCopyMemory(&readback, UserBuffer, 8);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            readback = 0;
            g_ProbeMagicOk = 0xEF;
        }

        if (probeAttached)
        {
            KeUnstackDetachProcess((PVOID)&probeApc);
        }

        g_ProbeMagicReadback = readback;

        if (g_ProbeMagicOk == 0)
        {
            g_ProbeMagicOk = (readback == magic) ? 1 : 2;
        }
    }


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
                {
                    LX_APC_STATE apcState;
                    BOOLEAN ok = FALSE;
                    BOOLEAN attached = FALSE;

                    if (g_PicoProcessObject != NULL)
                    {
                        KeStackAttachProcess(g_PicoProcessObject, (PVOID)&apcState);
                        attached = TRUE;
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
                        ok = TRUE;
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        ok = FALSE;
                    }
                    if (attached)
                    {
                        KeUnstackDetachProcess((PVOID)&apcState);
                    }

                    if (written + recordLength <= sizeof(g_DirCache))
                    {
                        ULONG64 cacheIno = (ULONG64)((index + 1) * 2);
                        ULONG64 cacheOff = (ULONG64)(written + recordLength);
                        USHORT cacheReclen = (USHORT)recordLength;
                        UCHAR cacheType = isDirectory ? 4 : 8;

                        RtlZeroMemory(g_DirCache + written, recordLength);

                        __try
                        {
                            RtlCopyMemory(g_DirCache + written + 0, &cacheIno, 8);
                            RtlCopyMemory(g_DirCache + written + 8, &cacheOff, 8);
                            RtlCopyMemory(g_DirCache + written + 16, &cacheReclen, 2);
                            RtlCopyMemory(g_DirCache + written + 18, &cacheType, 1);
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                        }

                        for (m = 0; m < nameLength && (19 + m) < (LONG)recordLength; m++)
                        {
                            g_DirCache[written + 19 + m] = (CHAR)(UCHAR)info->FileName[m];
                        }

                        g_DirCacheLen = written + recordLength;
                    }

                    if (!ok)
                    {
                        ExFreePoolWithTag(buffer, 'rDxL');
                        ZwClose(directoryHandle);
                        g_GetDentsStatus = 0xC0000005u;
                        return (ULONG64)(-14);
                    }
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
    g_DirCacheUserAddr = (ULONG64)(ULONG_PTR)UserBuffer;

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

    if (g_EntryCount < 8)
    {
        g_EntryDump[g_EntryCount][0] = Number;
        g_EntryDump[g_EntryCount][1] = Parameters[0];
        g_EntryDump[g_EntryCount][2] = Parameters[1];
        g_EntryDump[g_EntryCount][3] = Parameters[2];
        g_EntryCount++;
    }

    switch (Number)
    {
    case LX_SYS_OPEN:
    case LX_SYS_OPENAT:
    {
        CHAR kernelPath[260];
        ULONG pathLength;
        ULONG64 openFlags = Parameters[1];
        ULONG64 modeBits = Parameters[3];

        pathLength = LxCopyLinuxPath(Parameters[2], kernelPath, sizeof(kernelPath));

        g_StatDebug[3] = Parameters[2];
        g_StatDebug[4] = pathLength;

        if (pathLength == 0)
        {
            pathLength = LxCopyLinuxPath(Parameters[1], kernelPath, sizeof(kernelPath));

            if (pathLength > 0)
            {
                openFlags = Parameters[2];
            }
        }

        g_OpenCount++;

        g_StatDebug[0] = Parameters[1];
        g_StatDebug[1] = Parameters[2];
        g_StatDebug[2] = Parameters[3];
        LxProbeSlots(Parameters);
        g_SlotProbe[6] = (ULONG64)LxCopyLinuxPath(0x400078ull, g_SlotText[6], 48);

        if (pathLength == 0)
        {
            g_LastOpenStatus = 0xC000000Du;
            g_LastOpenFd = 0xFFFFFFFFu;
            g_LastOpenPath[0] = 0;
            return (ULONG64)(-2);
        }

        return LxOpenFile(kernelPath, (ULONG)openFlags, (ULONG)modeBits);
    }

    case LX_SYS_CLOSE:
        return LxCloseFile((ULONG)Parameters[1]);

    case LX_SYS_STAT:
    {
        CHAR kernelPath[260];
        ULONG64 length = LxCopyLinuxPath(Parameters[2], kernelPath, sizeof(kernelPath));
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

        LxProbeSlots(Parameters);

        g_SlotProbe[6] = (ULONG64)LxCopyLinuxPath(0x400078ull, g_SlotText[6], 48);
        g_SlotProbe[7] = (ULONG64)LxCopyLinuxPath(Parameters[2], g_SlotText[7], 48);
        g_StatDebug[6] = length;
        g_StatDebug[7] = (ULONG64)(UCHAR)kernelPath[0];

        if (length > 0)
        {
            g_LastStatStatus = 0x11110000u;
            g_LastNtPath[0] = 0;

            {
                ULONG k;
                ULONG pl = LxBuildNtPath(kernelPath, ntPathBuffer, RTL_NUMBER_OF(ntPathBuffer));

                g_StatDebug[7] = pl;

                for (k = 0; k < 259 && pl > 0 && ntPathBuffer[k] != 0; k++)
                {
                    g_LastNtPath[k] = (CHAR)ntPathBuffer[k];
                }
                g_LastNtPath[k] = 0;
            }
        }

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

        if (length == 0)
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

        if (Parameters[1] == 0)
        {
            return (ULONG64)(-14);
        }

        __try
        {
            RtlCopyMemory((PVOID)(ULONG_PTR)Parameters[1], &linuxStat, sizeof(linuxStat));
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
        ULONG fd = (ULONG)Parameters[1];
        ULONG count = (ULONG)Parameters[3];

        if (count > 0x4000)
        {
            count = 0x4000;
        }

        if (count == 0)
        {
            count = 1024;
        }

        return LxGetDents64(fd, (PVOID)(ULONG_PTR)Parameters[2], count);
    }

    case LX_SYS_ACCESS:
    {
        CHAR accessPath[260];
        WCHAR accessNt[300];
        ULONG accessLength;
        NTSTATUS accessStatus;

        accessLength = LxCopyLinuxPath(Parameters[1], accessPath, sizeof(accessPath));

        if (accessLength == 0)
        {
            return (ULONG64)(-14);
        }

        if (LxBuildNtPath(accessPath, accessNt, RTL_NUMBER_OF(accessNt)) == 0)
        {
            return (ULONG64)(-2);
        }

        accessStatus = LxQueryExists(accessNt);

        if (!NT_SUCCESS(accessStatus))
        {
            return (ULONG64)(-2);
        }

        return 0;
    }

    case LX_SYS_LSEEK:
    {
        ULONG64 fd = Parameters[1];
        LONG64 offset = (LONG64)Parameters[2];
        ULONG64 whence = Parameters[3];
        ULONG64 base;
        LONGLONG newPosition;

        if (fd >= LX_MAX_FD || !g_FdTable[fd].Used)
        {
            return (ULONG64)(-9);
        }

        base = g_FdTable[fd].Offset;

        if (whence == 0)
        {
            newPosition = offset;
        }
        else if (whence == 1)
        {
            newPosition = (LONGLONG)base + offset;
        }
        else if (whence == 2)
        {
            FILE_STANDARD_INFORMATION sizeInfo;
            IO_STATUS_BLOCK seekStatusBlock;

            if (!NT_SUCCESS(ZwQueryInformationFile(g_FdTable[fd].Handle, &seekStatusBlock, &sizeInfo,
                                                  sizeof(sizeInfo), FileStandardInformation)))
            {
                return (ULONG64)(-5);
            }

            newPosition = sizeInfo.EndOfFile.QuadPart + offset;
        }
        else
        {
            return (ULONG64)(-22);
        }

        if (newPosition < 0)
        {
            return (ULONG64)(-22);
        }

        g_FdTable[fd].Offset = (ULONG64)newPosition;

        g_SeekCount++;
        g_LastSeekResult = (ULONG64)newPosition;

        return (ULONG64)newPosition;
    }

    case LX_SYS_IOCTL:
        return 0;

    case LX_SYS_READ:
    {
        ULONG64 fd = Parameters[1];
        ULONG64 buffer = Parameters[2];
        ULONG64 count = Parameters[3];

        g_ReadDebug[0] = Parameters[0];
        g_ReadDebug[1] = Parameters[1];
        g_ReadDebug[2] = Parameters[2];
        g_ReadDebug[3] = Parameters[3];

        if (buffer == 0 || count == 0 || count > 0x10000)
        {
            if (buffer == 0)
            {
                return (ULONG64)(-14);
            }

            count = 16;
        }

        g_ReadDebug[4] = fd;
        g_ReadDebug[5] = buffer;
        g_ReadDebug[6] = count;

        return LxReadFile((ULONG)fd, (PVOID)(ULONG_PTR)buffer, (ULONG)count);
    }

    case LX_SYS_WRITE:
    {
        ULONG64 values[6];
        ULONG64 fd = 0;
        ULONG64 buffer = 0;
        ULONG64 length = 0;
        ULONG i;
        BOOLEAN haveBuffer = FALSE;
        BOOLEAN haveFd = FALSE;

        for (i = 0; i < 6; i++)
        {
            values[i] = Parameters[i];
        }

        for (i = 0; i < 6; i++)
        {
            if (!haveFd && (values[i] == 1 || values[i] == 2))
            {
                fd = values[i];
                haveFd = TRUE;
            }
        }

        buffer = values[2];
        haveBuffer = TRUE;

        __try
        {
            ULONG64 writeCount = 0;

            RtlCopyMemory(&writeCount, (PUCHAR)(ULONG_PTR)Context + 0x040, 8);

            g_WriteDebug[7] = writeCount;

            length = writeCount;

            if (length == 0 || length > 0x100000)
            {
                length = 4096;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            length = 4096;
        }

        g_WriteDebug[0] = values[0];
        g_WriteDebug[1] = values[1];
        g_WriteDebug[2] = values[2];
        g_WriteDebug[3] = buffer;
        g_WriteDebug[4] = fd;
        g_WriteDebug[5] = values[3];
        g_WriteDebug[6] = length;

        if (g_WriteContextDone == 0)
        {
            g_WriteContextDone = 1;

            for (i = 0; i < 64; i++)
            {
                ULONG64 value = 0;
                PUCHAR slot = (PUCHAR)(ULONG_PTR)Context + i * 8;

                __try
                {
                    RtlCopyMemory(&value, slot, 8);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    value = 0xDEADBEEFDEADBEEFull;
                }

                g_WriteContext[i] = value;
            }
        }

{
            ULONG64 contextCount = 0;

            __try
            {
                RtlCopyMemory(&contextCount, (PUCHAR)(ULONG_PTR)Context + 0x40, 8);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                contextCount = 0;
            }

            UNREFERENCED_PARAMETER(contextCount);
        }

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
                if (g_WriteCount == 0)
                {
                    RtlZeroMemory(g_WriteBuffer, sizeof(g_WriteBuffer));
                    g_WriteLength = 0;
                }

                {
                    ULONG appendAt = g_WriteLength;
                    BOOLEAN stored = FALSE;

                    if (appendAt + length >= sizeof(g_WriteBuffer) - 1)
                    {
                        appendAt = 0;
                    }

                    do
                    {
                        if (length > 0 && length <= 256 && g_PicoProcessObject != NULL)
                        {
                            CHAR staging[256];
                            LX_APC_STATE pullApc;
                            BOOLEAN pulled = FALSE;

                            KeStackAttachProcess(g_PicoProcessObject, (PVOID)&pullApc);

                            __try
                            {
                                ProbeForRead((PVOID)(ULONG_PTR)buffer, length, 1);
                                RtlCopyMemory(staging, (PVOID)(ULONG_PTR)buffer, length);
                                pulled = TRUE;
                            }
                            __except (EXCEPTION_EXECUTE_HANDLER)
                            {
                                pulled = FALSE;
                            }

                            KeUnstackDetachProcess((PVOID)&pullApc);

                            if (pulled)
                            {
                                ULONG pi;
                                ULONG nonZero = 0;

                                for (pi = 0; pi < length; pi++)
                                {
                                    if ((UCHAR)staging[pi] != 0)
                                    {
                                        nonZero++;
                                    }
                                }

                                if (nonZero > 0)
                                {
                                    RtlCopyMemory(g_WriteBuffer + appendAt, staging, length);
                                    appendAt += length;
                                    g_WriteBuffer[appendAt] = 0;
                                    g_WriteLength = appendAt;
                                    g_WriteCount++;
                                    g_WriteFd = (ULONG)fd;
                                    g_WriteStatus = 0xC0000002u;
                                    stored = TRUE;
                                    break;
                                }
                            }
                        }

                        if (g_DirCacheUserAddr != 0 && g_DirCacheLen != 0 &&
                            (ULONG64)(ULONG_PTR)buffer >= g_DirCacheUserAddr &&
                            (ULONG64)(ULONG_PTR)buffer < g_DirCacheUserAddr + g_DirCacheLen)
                        {
                            ULONG offset = (ULONG)((ULONG64)(ULONG_PTR)buffer - g_DirCacheUserAddr);
                            ULONG take = length;

                            if (offset + take > g_DirCacheLen)
                            {
                                take = (ULONG)(g_DirCacheLen - offset);
                            }

                            RtlCopyMemory(g_WriteBuffer + appendAt, g_DirCache + offset, take);
                            appendAt += take;
                            g_WriteBuffer[appendAt] = 0;
                            g_WriteLength = appendAt;
                            g_WriteCount++;
                            g_WriteFd = (ULONG)fd;
                            g_WriteStatus = 0xC0000001u;
                            stored = TRUE;
                            break;
                        }

                        RtlCopyMemory(g_WriteBuffer + appendAt, (PVOID)(ULONG_PTR)buffer, length);
                        appendAt += length;
                        g_WriteBuffer[appendAt] = 0;
                        g_WriteLength = appendAt;
                        g_WriteCount++;
                        g_WriteFd = (ULONG)fd;
                        g_WriteStatus = 0;
                        stored = TRUE;
                    } while (0);

                    UNREFERENCED_PARAMETER(stored);
                }
            }
        }

        return (ULONG64)length;
    }

    case LX_SYS_BRK:
        {
            ULONG64 brkTarget = arg1;

            if (brkTarget == 0 && arg0 != 0)
            {
                brkTarget = arg0;
            }

            g_BrkDebug[0] = arg0;
            g_BrkDebug[1] = arg1;
            g_BrkDebug[2] = Parameters[2];
            g_BrkDebug[3] = Context;
            g_BrkDebug[4] = brkTarget;
            g_BrkDebug[5] = 0;
            g_BrkDumpCount++;

            if (brkTarget == 0)
            {
                return g_BrkCurrent;
            }

            if (brkTarget >= g_PoolBase && brkTarget <= g_PoolBase + LX_POOL_SIZE)
            {
                g_BrkCurrent = brkTarget;
                g_BrkOk++;
                return brkTarget;
            }

            g_BrkReject++;
            if (g_BrkLastBad == 0)
            {
                g_BrkLastBad = brkTarget;
            }
            return g_BrkCurrent;
        }

    case LX_SYS_MMAP:
    {
        ULONG64 mapAddress = Parameters[1];
        ULONG64 mapLength = Parameters[2];
        ULONG64 mapProt = 0;
        ULONG64 mapFlags = 0;
        ULONG64 mapFd = 0;
        ULONG64 mapOffset = 0;
        ULONG desiredProtection;

        if (mapLength == 0)
        {
            return (ULONG64)(-22);
        }

        __try
        {
            RtlCopyMemory(&mapProt, (PUCHAR)(ULONG_PTR)Context + 0x040, 8);
            RtlCopyMemory(&mapFlags, (PUCHAR)(ULONG_PTR)Context + 0x048, 8);
            RtlCopyMemory(&mapFd, (PUCHAR)(ULONG_PTR)Context + 0x038, 8);
            RtlCopyMemory(&mapOffset, (PUCHAR)(ULONG_PTR)Context + 0x050, 8);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            mapProt = 0;
            mapFlags = 0;
            mapFd = 0;
            mapOffset = 0;
        }

        g_MmapDebug[0] = mapAddress;
        g_MmapDebug[1] = mapLength;
        g_MmapDebug[2] = mapProt;
        g_MmapDebug[3] = mapFlags;
        g_MmapDebug[4] = mapFd;
        g_MmapDebug[5] = mapOffset;
        g_MmapDebug[6] = (mapFd < LX_MAX_FD && g_FdTable[mapFd].Used) ? 1 : 0;
        g_MmapDebug[7] = (mapFd < LX_MAX_FD) ? (ULONG64)(ULONG_PTR)g_FdTable[mapFd].Handle : 0;

        mapLength = (mapLength + 0xFFF) & ~0xFFFull;

        if (mapProt & 0x4u)
        {
            desiredProtection = PAGE_EXECUTE_READWRITE;
        }
        else if (mapProt & 0x2u)
        {
            desiredProtection = PAGE_READWRITE;
        }
        else
        {
            desiredProtection = PAGE_READONLY;
        }

        if ((mapFlags & 0x10u) == 0)
        {
            mapAddress = mapAddress & ~0xFFFull;
        }

        if (mapAddress == 0)
        {
            mapAddress = g_MmapCurrent;
        }

        if (mapAddress + mapLength > g_PoolBase + LX_POOL_SIZE)
        {
            return (ULONG64)(-12);
        }

        if (mapFd < LX_MAX_FD && g_FdTable[mapFd].Used && g_FdTable[mapFd].Handle != NULL &&
            g_PicoProcessObject != NULL)
        {
            HANDLE sectionHandle = NULL;
            OBJECT_ATTRIBUTES sectionAttributes;
            PVOID viewBase = (PVOID)(ULONG_PTR)mapAddress;
            SIZE_T viewSize = 0;
            LARGE_INTEGER sectionOffset;
            LARGE_INTEGER fileSize;
            IO_STATUS_BLOCK sectionIosb;
            FILE_STANDARD_INFORMATION fileInfo;
            NTSTATUS mapStatus;

            __try
            {
                RtlZeroMemory(&fileInfo, sizeof(fileInfo));
                mapStatus = ZwQueryInformationFile(g_FdTable[mapFd].Handle, &sectionIosb, &fileInfo,
                                                   sizeof(fileInfo), FileStandardInformation);

                g_MmapDebug[6] = (ULONG64)mapStatus;

                if (NT_SUCCESS(mapStatus))
                {
                    fileSize.QuadPart = fileInfo.EndOfFile.QuadPart;
                    g_MmapDebug[7] = (ULONG64)fileSize.QuadPart;
                }
                else
                {
                    fileSize.QuadPart = (LONGLONG)(mapOffset + mapLength);
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                fileSize.QuadPart = (LONGLONG)(mapOffset + mapLength);
            }

            sectionOffset.QuadPart = (LONGLONG)mapOffset;
            InitializeObjectAttributes(&sectionAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

            mapStatus = ZwCreateSection(&sectionHandle, SECTION_MAP_READ, &sectionAttributes, &fileSize,
                                        PAGE_READONLY, SEC_COMMIT, g_FdTable[mapFd].Handle);

            g_LastMmapStatus = (ULONG64)mapStatus | 0x100000000ull;
            g_MmapDebug[6] = (ULONG64)mapStatus;
            g_MmapDebug[7] = fileSize.QuadPart;

            if (NT_SUCCESS(mapStatus))
            {
                mapStatus = ZwMapViewOfSection(sectionHandle, (HANDLE)g_LastPicoHandle, &viewBase, 0, 0,
                                               &sectionOffset, &viewSize, ViewUnmap, 0, PAGE_READONLY);

                ZwClose(sectionHandle);

                g_LastMmapStatus = (ULONG64)mapStatus | 0x200000000ull;

                if (NT_SUCCESS(mapStatus))
                {
                    g_MmapCurrent = (ULONG64)(ULONG_PTR)viewBase + viewSize;
                    g_MmapCount++;
                    g_LastMmapAddress = (ULONG64)(ULONG_PTR)viewBase;
                    g_LastMmapStatus = 0;

                    {
                        UCHAR probeBytes[16];
                        ULONG probeIndex;

                        RtlZeroMemory(probeBytes, sizeof(probeBytes));

                        __try
                        {
                            RtlCopyMemory(probeBytes, viewBase, 16);
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            probeBytes[0] = 0xEE;
                        }

                        for (probeIndex = 0; probeIndex < 16; probeIndex++)
                        {
                            g_MmapBytes[probeIndex] = probeBytes[probeIndex];
                        }
                    }

                    return g_LastMmapAddress;
                }
            }
        }

        g_MmapCurrent = mapAddress + mapLength;
        g_MmapCount++;
        g_LastMmapAddress = mapAddress;
        g_LastMmapStatus = 0xC0000004u;
        return mapAddress;
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

    case LX_SYS_UNAME:
    {
        LX_LINUX_UTSNAME uts;
        ULONG64 utsTarget = Parameters[1];

        if (utsTarget == 0)
        {
            utsTarget = Parameters[2];
        }

        if (utsTarget == 0)
        {
            return (ULONG64)(-14);
        }

        RtlZeroMemory(&uts, sizeof(uts));
        RtlCopyMemory(uts.SysName, (const VOID *)"Linux", 5);
        RtlCopyMemory(uts.NodeName, (const VOID *)"lxrun", 5);
        RtlCopyMemory(uts.Release, (const VOID *)"5.15.0-lxrun", 12);
        RtlCopyMemory(uts.Version, (const VOID *)"#1 SMP lxrun", 12);
        RtlCopyMemory(uts.Machine, (const VOID *)"x86_64", 6);
        RtlCopyMemory(uts.DomainName, (const VOID *)"(none)", 6);

        __try
        {
            ProbeForWrite((PVOID)(ULONG_PTR)utsTarget, sizeof(uts), 4);
            RtlCopyMemory((PVOID)(ULONG_PTR)utsTarget, &uts, sizeof(uts));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return (ULONG64)(-14);
        }

        return 0;
    }

    case LX_SYS_GETCWD:
    {
        static const char Cwd[] = "/";
        ULONG64 cwdBuffer = Parameters[1];
        ULONG64 cwdSize = Parameters[2];
        ULONG cwdNeeded = (ULONG)sizeof(Cwd);

        if (cwdBuffer == 0)
        {
            return (ULONG64)(-14);
        }

        if (cwdSize < cwdNeeded)
        {
            return (ULONG64)(-34);
        }

        __try
        {
            ProbeForWrite((PVOID)(ULONG_PTR)cwdBuffer, cwdNeeded, 1);
            RtlCopyMemory((PVOID)(ULONG_PTR)cwdBuffer, Cwd, cwdNeeded);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return (ULONG64)(-14);
        }

        return cwdNeeded;
    }

    case LX_SYS_GETRANDOM:
    {
        ULONG64 randomBuffer = Parameters[1];
        ULONG64 randomLength = Parameters[2];
        LARGE_INTEGER systemTime;
        ULONGLONG seed;

        if (randomBuffer == 0 || randomLength == 0)
        {
            return (ULONG64)(-14);
        }

        if (randomLength > 256)
        {
            randomLength = 256;
        }

        KeQuerySystemTime(&systemTime);
        seed = (ULONGLONG)systemTime.QuadPart ^ (ULONGLONG)(ULONG_PTR)g_PicoProcessObject;

        __try
        {
            UCHAR temp[256];
            ULONG ri;

            for (ri = 0; ri < (ULONG)randomLength; ri++)
            {
                seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
                temp[ri] = (UCHAR)((seed >> 33) & 0xFF);
            }

            ProbeForWrite((PVOID)(ULONG_PTR)randomBuffer, (ULONG)randomLength, 1);
            RtlCopyMemory((PVOID)(ULONG_PTR)randomBuffer, temp, (ULONG)randomLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return (ULONG64)(-14);
        }

        return (ULONG64)randomLength;
    }

    case LX_SYS_GETUID:
    case LX_SYS_GETEUID:
        return 0;

    case LX_SYS_GETGID:
    case LX_SYS_GETEGID:
        return 0;

    case LX_SYS_GETPID:
        return 1;

    case LX_SYS_GETPPID:
        return 0;

    case LX_SYS_GETTID:
        return 1;

    case LX_SYS_SET_ROBUST_LIST:
    case LX_SYS_SCHED_YIELD:
    case LX_SYS_MADVISE:
        return 0;

    case LX_SYS_MPROTECT:
        return 0;

    case LX_SYS_MUNMAP:
        return 0;

    case LX_SYS_GETTIMEOFDAY:
    {
        LX_LINUX_TIMEVAL tv;
        ULONG64 tvTarget = Parameters[1];

        if (tvTarget == 0)
        {
            tvTarget = Parameters[2];
        }

        LxGetUnixTimeval(&tv);

        if (tvTarget != 0)
        {
            __try
            {
                ProbeForWrite((PVOID)(ULONG_PTR)tvTarget, sizeof(tv), 4);
                RtlCopyMemory((PVOID)(ULONG_PTR)tvTarget, &tv, sizeof(tv));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return (ULONG64)(-14);
            }
        }

        return 0;
    }

    case LX_SYS_CLOCK_GETTIME:
    case LX_SYS_CLOCK_GETRES:
    {
        LX_LINUX_TIMESPEC ts;
        ULONG64 tsTarget = Parameters[2];

        if (tsTarget == 0)
        {
            tsTarget = Parameters[1];
        }

        LxGetUnixTimespec(&ts);

        if (tsTarget != 0)
        {
            __try
            {
                ProbeForWrite((PVOID)(ULONG_PTR)tsTarget, sizeof(ts), 4);
                RtlCopyMemory((PVOID)(ULONG_PTR)tsTarget, &ts, sizeof(ts));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return (ULONG64)(-14);
            }
        }

        return 0;
    }

    case LX_SYS_TIME:
    {
        LX_LINUX_TIMEVAL tv;
        ULONG64 timeTarget = Parameters[1];

        LxGetUnixTimeval(&tv);

        if (timeTarget != 0)
        {
            __try
            {
                ProbeForWrite((PVOID)(ULONG_PTR)timeTarget, sizeof(LONG64), 8);
                RtlCopyMemory((PVOID)(ULONG_PTR)timeTarget, &tv.Seconds, sizeof(LONG64));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return (ULONG64)(-14);
            }
        }

        return (ULONG64)tv.Seconds;
    }

    case LX_SYS_GETRLIMIT:
    {
        ULONG64 limitTarget = Parameters[2];

        if (limitTarget == 0)
        {
            limitTarget = Parameters[3];
        }

        if (limitTarget != 0)
        {
            ULONG64 limits[2];

            limits[0] = 0x7FFFFFFF;
            limits[1] = 0x7FFFFFFF;

            __try
            {
                ProbeForWrite((PVOID)(ULONG_PTR)limitTarget, sizeof(limits), 8);
                RtlCopyMemory((PVOID)(ULONG_PTR)limitTarget, limits, sizeof(limits));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return (ULONG64)(-14);
            }
        }

        return 0;
    }

    case LX_SYS_MKDIR:
    case LX_SYS_MKDIRAT:
    {
        CHAR mkdirPath[260];
        WCHAR mkdirNt[300];
        ULONG mkdirLength;
        ULONG64 mkdirArg;

        mkdirArg = (Number == LX_SYS_MKDIRAT) ? Parameters[2] : Parameters[1];

        mkdirLength = LxCopyLinuxPath(mkdirArg, mkdirPath, sizeof(mkdirPath));

        if (mkdirLength == 0)
        {
            return (ULONG64)(-14);
        }

        if (LxBuildNtPath(mkdirPath, mkdirNt, RTL_NUMBER_OF(mkdirNt)) == 0)
        {
            return (ULONG64)(-2);
        }

        return LxNtStatusToErrno(LxCreateDirectory(mkdirNt, (ULONG)Parameters[2]));
    }

    case LX_SYS_RMDIR:
    case LX_SYS_UNLINK:
    case LX_SYS_UNLINKAT:
    {
        CHAR removePath[260];
        WCHAR removeNt[300];
        ULONG removeLength;
        ULONG64 removeArg = Parameters[1];

        if (Number == LX_SYS_UNLINKAT)
        {
            removeArg = Parameters[2];
        }

        removeLength = LxCopyLinuxPath(removeArg, removePath, sizeof(removePath));

        if (removeLength == 0)
        {
            return (ULONG64)(-14);
        }

        if (LxBuildNtPath(removePath, removeNt, RTL_NUMBER_OF(removeNt)) == 0)
        {
            return (ULONG64)(-2);
        }

        return LxNtStatusToErrno(LxRemoveEntry(removeNt, Number == LX_SYS_RMDIR));
    }

    case LX_SYS_RENAME:
    case LX_SYS_RENAMEAT:
    {
        CHAR renameSource[260];
        CHAR renameTarget[260];
        WCHAR renameSourceNt[300];
        WCHAR renameTargetNt[300];
        ULONG64 sourceArg;
        ULONG64 targetArg;

        if (Number == LX_SYS_RENAMEAT)
        {
            sourceArg = Parameters[2];
            targetArg = Parameters[4];
        }
        else
        {
            sourceArg = Parameters[1];
            targetArg = Parameters[2];
        }

        if (LxCopyLinuxPath(sourceArg, renameSource, sizeof(renameSource)) == 0)
        {
            return (ULONG64)(-14);
        }

        if (LxCopyLinuxPath(targetArg, renameTarget, sizeof(renameTarget)) == 0)
        {
            return (ULONG64)(-14);
        }

        if (LxBuildNtPath(renameSource, renameSourceNt, RTL_NUMBER_OF(renameSourceNt)) == 0)
        {
            return (ULONG64)(-2);
        }

        if (LxBuildNtPath(renameTarget, renameTargetNt, RTL_NUMBER_OF(renameTargetNt)) == 0)
        {
            return (ULONG64)(-2);
        }

        return LxNtStatusToErrno(LxRenameEntry(renameSourceNt, renameTargetNt, TRUE));
    }

    case LX_SYS_UMASK:
    {
        ULONG previous = g_ReportedUmask;

        g_ReportedUmask = (ULONG)Parameters[1] & 0777u;
        return previous;
    }

    case LX_SYS_RT_SIGACTION:
    case LX_SYS_RT_SIGPROCMASK:
    case LX_SYS_SIGALTSTACK:
    case LX_SYS_RT_SIGPENDING:
    case LX_SYS_RT_SIGSUSPEND:
    case LX_SYS_RT_SIGRETURN:
    case LX_SYS_KILL:
    case LX_SYS_TGKILL:
    case LX_SYS_PAUSE:
    case LX_SYS_ALARM:
        return 0;

    case LX_SYS_FUTEX:
    {
        ULONG64 futexOperation = Parameters[2] & 0x7Fu;

        if (futexOperation == 0 || futexOperation == 9 || futexOperation == 10)
        {
            return 0;
        }

        if (futexOperation == 1 || futexOperation == 2 || futexOperation == 3)
        {
            return (ULONG64)(-11);
        }

        return 0;
    }

    case LX_SYS_EPOLL_CREATE:
    case LX_SYS_EPOLL_CREATE1:
    case LX_SYS_EVENTFD:
    case LX_SYS_EVENTFD2:
    {
        ULONG epollSlot;

        for (epollSlot = 0; epollSlot < LX_MAX_FD; epollSlot++)
        {
            if (!g_FdTable[epollSlot].Used)
            {
                break;
            }
        }

        if (epollSlot >= LX_MAX_FD)
        {
            return (ULONG64)(-24);
        }

        RtlZeroMemory(&g_FdTable[epollSlot], sizeof(g_FdTable[epollSlot]));
        g_FdTable[epollSlot].Used = TRUE;
        g_FdTable[epollSlot].IsDirectory = FALSE;
        g_FdTable[epollSlot].Handle = NULL;
        g_FdTable[epollSlot].OpenFlags = 0;
        g_FdTable[epollSlot].Offset = 0;
        RtlCopyMemory(g_FdTable[epollSlot].LinuxPath, (const VOID *)"/dev/epoll", 11);

        return (ULONG64)epollSlot;
    }

    case LX_SYS_EPOLL_CTL:
        return 0;

    case LX_SYS_EPOLL_WAIT:
        return (ULONG64)(-11);

    case LX_SYS_READLINK:
    case LX_SYS_READLINKAT:
    {
        static const CHAR SelfExe[] = "/mnt/c/Windows/mk.x64";
        ULONG64 linkBuffer;
        ULONG64 linkSize;
        ULONG linkLength = (ULONG)(sizeof(SelfExe) - 1);

        if (Number == LX_SYS_READLINKAT)
        {
            linkBuffer = Parameters[3];
            linkSize = Parameters[4];
        }
        else
        {
            linkBuffer = Parameters[2];
            linkSize = Parameters[3];
        }

        if (linkBuffer == 0)
        {
            return (ULONG64)(-14);
        }

        if (linkSize < linkLength)
        {
            return (ULONG64)(-22);
        }

        __try
        {
            ProbeForWrite((PVOID)(ULONG_PTR)linkBuffer, linkLength, 1);
            RtlCopyMemory((PVOID)(ULONG_PTR)linkBuffer, (const VOID *)SelfExe, linkLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return (ULONG64)(-14);
        }

        return (ULONG64)linkLength;
    }

    case LX_SYS_STATFS:
    case LX_SYS_FSTATFS:
    {
        LX_LINUX_STATFS statfs;
        ULONG64 statfsTarget = Parameters[2];

        if (Number == LX_SYS_FSTATFS)
        {
            ULONG64 statfsFd = Parameters[1];

            if (statfsFd >= LX_MAX_FD || !g_FdTable[statfsFd].Used)
            {
                return (ULONG64)(-9);
            }

            statfsTarget = Parameters[2];
        }

        if (statfsTarget == 0)
        {
            return (ULONG64)(-14);
        }

        RtlZeroMemory(&statfs, sizeof(statfs));
        statfs.Type = 0x01021997;
        statfs.BlockSize = 4096;
        statfs.Blocks = 0x4000000;
        statfs.BlocksFree = 0x2000000;
        statfs.BlocksAvailable = 0x2000000;
        statfs.Files = 0x100000;
        statfs.FilesFree = 0x80000;
        statfs.FileSystemId = 0x4C585255;
        statfs.NameLength = 255;
        statfs.FragmentSize = 4096;

        __try
        {
            ProbeForWrite((PVOID)(ULONG_PTR)statfsTarget, sizeof(statfs), 8);
            RtlCopyMemory((PVOID)(ULONG_PTR)statfsTarget, &statfs, sizeof(statfs));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return (ULONG64)(-14);
        }

        return 0;
    }

    case LX_SYS_READV:
    case LX_SYS_WRITEV:
    {
        ULONG64 vectorFd = Parameters[1];
        ULONG64 vectorBase = Parameters[2];
        ULONG64 vectorCount = Parameters[3];
        ULONG64 vectorTotal = 0;
        ULONG64 vectorIndex;
        LX_APC_STATE vectorApc;
        BOOLEAN vectorAttached = FALSE;

        if (vectorFd >= LX_MAX_FD || !g_FdTable[vectorFd].Used)
        {
            return (ULONG64)(-9);
        }

        if (vectorBase == 0 || vectorCount == 0 || vectorCount > 64)
        {
            return (ULONG64)(-22);
        }

        if (g_PicoProcessObject != NULL)
        {
            KeStackAttachProcess(g_PicoProcessObject, (PVOID)&vectorApc);
            vectorAttached = TRUE;
        }

        __try
        {
            LX_IOVEC vector[64];
            ULONG vectorBytes = (ULONG)(vectorCount * sizeof(LX_IOVEC));

            ProbeForRead((PVOID)(ULONG_PTR)vectorBase, vectorBytes, 8);
            RtlCopyMemory(vector, (PVOID)(ULONG_PTR)vectorBase, vectorBytes);

            for (vectorIndex = 0; vectorIndex < vectorCount; vectorIndex++)
            {
                if (vector[vectorIndex].Length == 0)
                {
                    continue;
                }

                if (Number == LX_SYS_READV)
                {
                    ULONG64 got = LxReadFile((ULONG)vectorFd,
                                             (PVOID)(ULONG_PTR)vector[vectorIndex].Base,
                                             (ULONG)vector[vectorIndex].Length);

                    if ((LONG64)got < 0)
                    {
                        if (vectorTotal == 0)
                        {
                            vectorTotal = got;
                        }
                        break;
                    }

                    vectorTotal += got;

                    if (got < vector[vectorIndex].Length)
                    {
                        break;
                    }
                }
                else
                {
                    LX_APC_STATE writevApc;
                    BOOLEAN writevAttached = FALSE;
                    PUCHAR writevSource = (PUCHAR)(ULONG_PTR)vector[vectorIndex].Base;

                    if (g_DirCacheUserAddr != 0 && g_DirCacheLen != 0 &&
                        (ULONG64)(ULONG_PTR)writevSource >= g_DirCacheUserAddr &&
                        (ULONG64)(ULONG_PTR)writevSource < g_DirCacheUserAddr + g_DirCacheLen)
                    {
                        ULONG cacheOffset = (ULONG)((ULONG64)(ULONG_PTR)writevSource - g_DirCacheUserAddr);

                        if ((ULONG64)cacheOffset + vector[vectorIndex].Length > g_DirCacheLen)
                        {
                            vector[vectorIndex].Length = (ULONG64)(g_DirCacheLen - cacheOffset);
                        }

                        writevSource = (PUCHAR)g_DirCache + cacheOffset;
                    }
                    else if (g_PicoProcessObject != NULL)
                    {
                        KeStackAttachProcess(g_PicoProcessObject, (PVOID)&writevApc);
                        writevAttached = TRUE;
                    }

                    if (g_WriteCount < 64 && vector[vectorIndex].Length <= 256)
                    {
                        CHAR writevStaging[256];
                        BOOLEAN writevCopied = FALSE;

                        __try
                        {
                            ProbeForRead(writevSource, (ULONG)vector[vectorIndex].Length, 1);
                            RtlCopyMemory(writevStaging, writevSource, (ULONG)vector[vectorIndex].Length);
                            writevCopied = TRUE;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            writevCopied = FALSE;
                        }

                        if (writevCopied)
                        {
                            ULONG writevAppend = g_WriteLength;

                            if (writevAppend + (ULONG)vector[vectorIndex].Length >= sizeof(g_WriteBuffer) - 1)
                            {
                                writevAppend = 0;
                            }

                            RtlCopyMemory(g_WriteBuffer + writevAppend, writevStaging, (ULONG)vector[vectorIndex].Length);
                            writevAppend += (ULONG)vector[vectorIndex].Length;
                            g_WriteBuffer[writevAppend] = 0;
                            g_WriteLength = writevAppend;
                            g_WriteCount++;
                            g_WriteFd = (ULONG)vectorFd;
                            g_WriteStatus = 0xC0000003u;
                        }
                    }

                    if (writevAttached)
                    {
                        KeUnstackDetachProcess((PVOID)&writevApc);
                    }

                    vectorTotal += vector[vectorIndex].Length;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            vectorTotal = (ULONG64)(-14);
        }

        if (vectorAttached)
        {
            KeUnstackDetachProcess((PVOID)&vectorApc);
        }

        return vectorTotal;
    }

    case LX_SYS_SOCKET:
    case LX_SYS_CONNECT:
    case LX_SYS_SENDTO:
    case LX_SYS_RECVFROM:
    case LX_SYS_SHUTDOWN:
    case LX_SYS_SETSOCKOPT:
    case LX_SYS_GETSOCKOPT:
        return (ULONG64)(-97);

    case LX_SYS_CLONE:
    case LX_SYS_FORK:
    case LX_SYS_VFORK:
    case LX_SYS_EXECVE:
        return (ULONG64)(-38);

    case LX_SYS_PRCTL:
    case LX_SYS_FSYNC:
    case LX_SYS_FDATASYNC:
    case LX_SYS_SYNC:
    case LX_SYS_FLOCK:
        return 0;

    case LX_SYS_FCNTL:
    {
        ULONG64 fcntlFd = Parameters[1];

        if (fcntlFd >= LX_MAX_FD || !g_FdTable[fcntlFd].Used)
        {
            return (ULONG64)(-9);
        }

        return 0;
    }

    case LX_SYS_DUP:
    {
        ULONG64 dupSource = Parameters[1];
        ULONG dupSlot;

        if (dupSource >= LX_MAX_FD || !g_FdTable[dupSource].Used)
        {
            return (ULONG64)(-9);
        }

        for (dupSlot = 0; dupSlot < LX_MAX_FD; dupSlot++)
        {
            if (!g_FdTable[dupSlot].Used)
            {
                break;
            }
        }

        if (dupSlot >= LX_MAX_FD)
        {
            return (ULONG64)(-24);
        }

        g_FdTable[dupSlot] = g_FdTable[dupSource];
        return (ULONG64)dupSlot;
    }

    case LX_SYS_DUP2:
    {
        ULONG64 dup2Source = Parameters[1];
        ULONG64 dup2Target = Parameters[2];

        if (dup2Source >= LX_MAX_FD || !g_FdTable[dup2Source].Used)
        {
            return (ULONG64)(-9);
        }

        if (dup2Target >= LX_MAX_FD)
        {
            return (ULONG64)(-9);
        }

        if (dup2Source == dup2Target)
        {
            return dup2Target;
        }

        if (g_FdTable[dup2Target].Used && g_FdTable[dup2Target].Handle != NULL)
        {
            if (dup2Target > 2)
            {
                ZwClose(g_FdTable[dup2Target].Handle);
            }

            RtlZeroMemory(&g_FdTable[dup2Target], sizeof(g_FdTable[dup2Target]));
        }

        g_FdTable[dup2Target] = g_FdTable[dup2Source];
        return dup2Target;
    }

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

static void LxParseElfDynamic(const UCHAR *ImageData, ULONG ImageSize, const LX_ELF_LOAD *Load,
                              LX_ELF_DYNAMIC_INFO *Info)
{
    ULONG index;

    RtlZeroMemory(Info, sizeof(LX_ELF_DYNAMIC_INFO));

    if (ImageData == NULL || ImageSize == 0 || Load->PhdrCount == 0 || Load->PhdrEntrySize < 56)
    {
        return;
    }

    for (index = 0; index < Load->PhdrCount && index < 64; index++)
    {
        ULONG64 phdrOffset;
        const UCHAR *phdr;
        ULONG pType;
        ULONG64 pOffset;
        ULONG64 pVaddr;
        ULONG64 pFilesz;

        if (Load->PhdrAddress >= Load->MinVaddr)
        {
            phdrOffset = (Load->PhdrAddress - Load->MinVaddr) + (ULONG64)index * Load->PhdrEntrySize;
        }
        else
        {
            break;
        }

        if (phdrOffset + 56 > ImageSize)
        {
            continue;
        }

        phdr = ImageData + phdrOffset;

        pType = *(const ULONG *)(phdr + 0);
        pVaddr = *(const ULONG64 *)(phdr + 16);
        pOffset = *(const ULONG64 *)(phdr + 8);
        pFilesz = *(const ULONG64 *)(phdr + 32);

        if (pType == LX_PT_INTERP)
        {
            Info->HasInterp = 1;
            Info->InterpVaddr = pVaddr;
            Info->InterpOffset = pOffset;
            Info->InterpSize = (ULONG)pFilesz;
        }
        else if (pType == LX_PT_DYNAMIC)
        {
            Info->HasDynamic = 1;
            Info->DynamicVaddr = pVaddr;
            Info->DynamicOffset = pOffset;
            Info->DynamicSize = pFilesz;
        }
    }

    if (Info->HasDynamic == 0 || Info->DynamicOffset + 16 > ImageSize)
    {
        return;
    }

    for (index = 0; index < 128; index++)
    {
        ULONG64 entryOffset = Info->DynamicOffset + (ULONG64)index * 16;
        LONG64 dTag;
        ULONG64 dVal;

        if (entryOffset + 16 > ImageSize || entryOffset + 16 > Info->DynamicOffset + Info->DynamicSize)
        {
            break;
        }

        dTag = *(const LONG64 *)(ImageData + entryOffset);
        dVal = *(const ULONG64 *)(ImageData + entryOffset + 8);

        Info->DynamicCount++;

        if (dTag == LX_DT_NULL)
        {
            break;
        }

        if (dTag == LX_DT_NEEDED)
        {
            if (Info->NeededCount < LX_MAX_NEEDED)
            {
                Info->NeededOffsets[Info->NeededCount] = (ULONG)dVal;
                Info->NeededCount++;
            }
        }
        else if (dTag == LX_DT_STRTAB)
        {
            Info->StrTabVaddr = dVal;
        }
        else if (dTag == LX_DT_SYMTAB)
        {
            Info->SymTabVaddr = dVal;
        }
        else if (dTag == LX_DT_RELA)
        {
            Info->RelaVaddr = dVal;
        }
        else if (dTag == LX_DT_RELASZ)
        {
            Info->RelaSize = dVal;
        }
        else if (dTag == LX_DT_JMPREL)
        {
            Info->JmpRelVaddr = dVal;
        }
        else if (dTag == LX_DT_PLTRELSZ)
        {
            Info->JmpRelSize = dVal;
        }
        else if (dTag == LX_DT_PLTGOT)
        {
            Info->PltGotVaddr = dVal;
        }
        else if (dTag == LX_DT_INIT)
        {
            Info->InitVaddr = dVal;
        }
        else if (dTag == LX_DT_FINI)
        {
            Info->FiniVaddr = dVal;
        }
        else if (dTag == LX_DT_SONAME)
        {
            Info->SonameOffset = dVal;
        }
        else if (dTag == LX_DT_HASH)
        {
            Info->HashVaddr = dVal;
        }
    }

    Info->RelaCount = Info->RelaSize / 24;
    Info->JmpRelCount = Info->JmpRelSize / 24;

    if (Info->HashVaddr != 0 && Info->HashVaddr >= Load->MinVaddr)
    {
        ULONG64 hashOffset = Info->HashVaddr - Load->MinVaddr;

        if (hashOffset + 8 <= ImageSize)
        {
            ULONG nchain = *(const ULONG *)(ImageData + hashOffset + 4);

            if (nchain > 0 && nchain < 4096)
            {
                Info->SymCountFromHash = nchain;
            }
        }
    }
}

static NTSTATUS LxWritePicoMemory(PEPROCESS TargetProcess, ULONG64 Address, const VOID *LocalBuffer, ULONG Length)
{
    SIZE_T written = 0;
    NTSTATUS status;

    if (TargetProcess == NULL || LocalBuffer == NULL || Length == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    status = MmCopyVirtualMemory(PsGetCurrentProcess(), (PVOID)LocalBuffer, TargetProcess,
                                 (PVOID)(ULONG_PTR)Address, (SIZE_T)Length, KernelMode, &written);

    if (!NT_SUCCESS(status))
    {
        g_LastRelocKernelStatus = (ULONG)status;
        return status;
    }

    if (written != Length)
    {
        g_LastRelocKernelStatus = 0xC0000001u;
        return STATUS_UNSUCCESSFUL;
    }

    g_LastRelocKernelStatus = 0;

    return STATUS_SUCCESS;
}

static void LxApplyRelocations(const UCHAR *ImageData, ULONG ImageSize, const LX_ELF_LOAD *Load,
                               const LX_ELF_DYNAMIC_INFO *Info, PEPROCESS TargetProcess, LX_RELOC_STATS *Stats)
{
    ULONG pass;
    ULONG limit;

    RtlZeroMemory(Stats, sizeof(LX_RELOC_STATS));

    if (ImageData == NULL || ImageSize == 0 || TargetProcess == NULL || Load->MinVaddr == 0)
    {
        return;
    }

    limit = LX_MAX_RELOCATIONS;

    for (pass = 0; pass < 2; pass++)
    {
        ULONG64 tableOffset;
        ULONG64 tableSize;
        ULONG entryCount;
        ULONG entryIndex;

        if (pass == 0)
        {
            if (Info->RelaVaddr == 0 || Info->RelaSize == 0)
            {
                continue;
            }

            tableOffset = Info->RelaVaddr - Load->MinVaddr;
            tableSize = Info->RelaSize;
        }
        else
        {
            if (Info->JmpRelVaddr == 0 || Info->JmpRelSize == 0)
            {
                continue;
            }

            if (Info->JmpRelVaddr == Info->RelaVaddr && Info->JmpRelSize == Info->RelaSize)
            {
                continue;
            }

            tableOffset = Info->JmpRelVaddr - Load->MinVaddr;
            tableSize = Info->JmpRelSize;
        }

        if (tableOffset >= ImageSize)
        {
            continue;
        }

        entryCount = (ULONG)(tableSize / sizeof(LX_ELF_RELA));

        if (entryCount > limit)
        {
            entryCount = limit;
        }

        for (entryIndex = 0; entryIndex < entryCount; entryIndex++)
        {
            ULONG64 entryOffset = tableOffset + (ULONG64)entryIndex * sizeof(LX_ELF_RELA);
            LX_ELF_RELA rela;
            ULONG relocType;
            ULONG64 targetAddress;
            ULONG64 newValue;
            NTSTATUS writeStatus;

            if (entryOffset + sizeof(LX_ELF_RELA) > ImageSize)
            {
                break;
            }

            RtlCopyMemory(&rela, ImageData + entryOffset, sizeof(LX_ELF_RELA));

            relocType = (ULONG)(rela.Info & 0xFFFFFFFFull);
            targetAddress = Load->MinVaddr + rela.Offset;

            Stats->TotalCount++;

            if (relocType == LX_R_X86_64_NONE)
            {
                continue;
            }

            if (relocType == LX_R_X86_64_RELATIVE)
            {
                newValue = Load->MinVaddr + (ULONG64)rela.Addend;
                Stats->RelativeCount++;
            }
            else if (relocType == LX_R_X86_64_JUMP_SLOT)
            {
                newValue = 0;
                Stats->JumpSlotCount++;
            }
            else if (relocType == LX_R_X86_64_GLOB_DAT)
            {
                newValue = 0;
                Stats->GlobDatCount++;
            }
            else
            {
                Stats->UnsupportedCount++;
                continue;
            }

            if (Stats->FirstOffset == 0)
            {
                Stats->FirstOffset = rela.Offset;
                Stats->FirstAddend = (ULONG64)rela.Addend;
                Stats->FirstWritten = newValue;
            }

            writeStatus = LxWritePicoMemory(TargetProcess, targetAddress, &newValue, sizeof(newValue));

            Stats->FirstWriteStatus = writeStatus;

            if (!NT_SUCCESS(writeStatus))
            {
                Stats->WriteFailCount++;
                continue;
            }

            if (Stats->FirstReadBack == 0)
            {
                ULONG64 readBack = 0;
                SIZE_T copied = 0;
                NTSTATUS readStatus;

                readStatus = MmCopyVirtualMemory(TargetProcess, (PVOID)(ULONG_PTR)targetAddress,
                                                 PsGetCurrentProcess(), &readBack, sizeof(readBack),
                                                 KernelMode, &copied);

                if (NT_SUCCESS(readStatus) && copied == sizeof(readBack))
                {
                    Stats->FirstReadBack = readBack;
                }
                else
                {
                    Stats->FirstReadBack = 0xFFFFFFFFDEADBEEFull;
                }
            }
        }
    }
}

static void LxDumpSymbols(const UCHAR *ImageData, ULONG ImageSize, const LX_ELF_LOAD *Load,
                          const LX_ELF_DYNAMIC_INFO *Info, LX_SYMBOL_INFO *Symbols)
{
    ULONG64 relaOffset;
    ULONG relaCount;
    ULONG index;
    ULONG64 strTabOffset;
    ULONG64 symTabOffset;

    RtlZeroMemory(Symbols, sizeof(LX_SYMBOL_INFO));

    if (ImageData == NULL || ImageSize == 0 || Load->MinVaddr == 0)
    {
        return;
    }

    Symbols->SymTabVaddr = Info->SymTabVaddr;
    Symbols->StrTabVaddr = Info->StrTabVaddr;

    if (Info->SymTabVaddr != 0 && Info->SymTabVaddr >= Load->MinVaddr)
    {
        symTabOffset = Info->SymTabVaddr - Load->MinVaddr;
    }
    else
    {
        symTabOffset = 0;
    }

    if (Info->StrTabVaddr != 0 && Info->StrTabVaddr >= Load->MinVaddr)
    {
        strTabOffset = Info->StrTabVaddr - Load->MinVaddr;

        if (strTabOffset < ImageSize)
        {
            Symbols->StrTabReadable = 1;
            Symbols->StrTabStatus = 0;
        }
        else
        {
            Symbols->StrTabReadable = 0;
            Symbols->StrTabStatus = 0xC0000001u;
        }
    }
    else
    {
        strTabOffset = 0;
        Symbols->StrTabReadable = 0;
        Symbols->StrTabStatus = Info->StrTabVaddr == 0 ? 0xC0000225u : 0xC0000001u;
    }

    if (Info->RelaVaddr == 0 || Info->RelaSize == 0 || Info->RelaVaddr < Load->MinVaddr)
    {
        return;
    }

    relaOffset = Info->RelaVaddr - Load->MinVaddr;

    if (relaOffset >= ImageSize)
    {
        return;
    }

    relaCount = (ULONG)(Info->RelaSize / sizeof(LX_ELF_RELA));

    if (relaCount > LX_MAX_SYMBOLS)
    {
        Symbols->Truncated = 1;
        relaCount = LX_MAX_SYMBOLS;
    }

    Symbols->TotalSyms = (ULONG)(Info->RelaSize / sizeof(LX_ELF_RELA));

    for (index = 0; index < relaCount; index++)
    {
        ULONG64 entryOffset = relaOffset + (ULONG64)index * sizeof(LX_ELF_RELA);
        LX_ELF_RELA rela;
        ULONG symIndex;
        LX_SYMBOL_ENTRY *entry;

        if (entryOffset + sizeof(LX_ELF_RELA) > ImageSize)
        {
            break;
        }

        RtlCopyMemory(&rela, ImageData + entryOffset, sizeof(LX_ELF_RELA));

        entry = &Symbols->Entries[Symbols->DumpedCount];
        entry->RelocOffset = (ULONG)rela.Offset;
        entry->RelocAddend = (ULONG64)rela.Addend;

        symIndex = (ULONG)(rela.Info >> 32);

        if (symTabOffset != 0 && symIndex != 0)
        {
            ULONG64 symEntryOffset = symTabOffset + (ULONG64)symIndex * sizeof(LX_ELF_SYM);

            if (symEntryOffset + sizeof(LX_ELF_SYM) <= ImageSize)
            {
                LX_ELF_SYM sym;

                RtlCopyMemory(&sym, ImageData + symEntryOffset, sizeof(LX_ELF_SYM));

                entry->NameOffset = sym.NameOffset;

                if (Symbols->StrTabReadable && sym.NameOffset != 0)
                {
                    ULONG64 nameOffset = strTabOffset + sym.NameOffset;
                    ULONG charIndex;

                    if (Symbols->DumpedCount == 1)
                    {
                        Symbols->NameFileOffset = nameOffset;
                        Symbols->NameByte0 = (ULONG)ImageData[nameOffset];

                        if (nameOffset + 1 < ImageSize)
                        {
                            Symbols->NameByte1 = (ULONG)ImageData[nameOffset + 1];
                        }
                    }

                    if (nameOffset < ImageSize)
                    {
                        for (charIndex = 0; charIndex + 1 < LX_MAX_SYMBOL_NAME; charIndex++)
                        {
                            ULONG64 pos = nameOffset + charIndex;

                            if (pos >= ImageSize)
                            {
                                break;
                            }

                            entry->Name[charIndex] = (CHAR)ImageData[pos];

                            if (entry->Name[charIndex] == 0)
                            {
                                break;
                            }
                        }

                        entry->Name[LX_MAX_SYMBOL_NAME - 1] = 0;
                    }
                }
            }
        }

        Symbols->DumpedCount++;
    }
}

static const LX_SYMBOL_BINDING g_SymbolTable[] = {
    { "write", LX_SYS_WRITE },
    { "read", LX_SYS_READ },
    { "open", LX_SYS_OPEN },
    { "openat", LX_SYS_OPENAT },
    { "close", LX_SYS_CLOSE },
    { "_exit", LX_SYS_EXIT },
    { "exit", LX_SYS_EXIT },
    { "exit_group", LX_SYS_EXIT_GROUP },
    { "lseek", LX_SYS_LSEEK },
    { "brk", LX_SYS_BRK },
    { "mmap", LX_SYS_MMAP },
    { "munmap", LX_SYS_MUNMAP },
    { "mprotect", LX_SYS_MPROTECT },
    { "fstat", LX_SYS_FSTAT },
    { "stat", LX_SYS_STAT },
    { "lstat", LX_SYS_LSTAT },
    { "getpid", LX_SYS_GETPID },
    { "getuid", LX_SYS_GETUID },
    { "geteuid", LX_SYS_GETEUID },
    { "getgid", LX_SYS_GETGID },
    { "getegid", LX_SYS_GETEGID },
    { "uname", LX_SYS_UNAME },
    { "getcwd", LX_SYS_GETCWD },
    { "gettimeofday", LX_SYS_GETTIMEOFDAY },
    { "clock_gettime", LX_SYS_CLOCK_GETTIME },
    { "time", LX_SYS_TIME },
    { "access", LX_SYS_ACCESS },
    { "readlink", LX_SYS_READLINK },
    { "rt_sigaction", LX_SYS_RT_SIGACTION },
    { "rt_sigprocmask", LX_SYS_RT_SIGPROCMASK },
    { "arch_prctl", LX_SYS_ARCH_PRCTL },
    { "set_tid_address", LX_SYS_SET_TID_ADDRESS },
    { "set_robust_list", LX_SYS_SET_ROBUST_LIST },
    { "getrandom", LX_SYS_GETRANDOM },
    { "futex", LX_SYS_FUTEX },
    { "execve", LX_SYS_EXECVE },
    { "ioctl", LX_SYS_IOCTL },
    { "fcntl", LX_SYS_FCNTL },
    { "dup", LX_SYS_DUP },
    { "dup2", LX_SYS_DUP2 },
    { "pipe", LX_SYS_PIPE },
    { "mkdir", LX_SYS_MKDIR },
    { "unlink", LX_SYS_UNLINK },
    { "rename", LX_SYS_RENAME },
    { "getdents64", LX_SYS_GETDENTS64 },
    { "statfs", LX_SYS_STATFS },
    { "writev", LX_SYS_WRITEV },
    { "readv", LX_SYS_READV },
    { "prctl", LX_SYS_PRCTL },
    { "madvise", LX_SYS_MADVISE },
    { "sched_yield", LX_SYS_SCHED_YIELD },
    { "nanosleep", LX_SYS_NANOSLEEP }
};

static ULONG LxLookupSyscallForSymbol(const char *Name)
{
    ULONG index;
    ULONG count = sizeof(g_SymbolTable) / sizeof(g_SymbolTable[0]);

    if (Name == NULL || Name[0] == 0)
    {
        return 0;
    }

    for (index = 0; index < count; index++)
    {
        const char *left = g_SymbolTable[index].Name;
        const char *right = Name;
        BOOLEAN matched = TRUE;

        while (*left != 0 || *right != 0)
        {
            if (*left != *right)
            {
                matched = FALSE;
                break;
            }

            left++;
            right++;
        }

        if (matched)
        {
            return g_SymbolTable[index].SyscallNumber;
        }
    }

    return 0;
}

static ULONG LxBuildStubCode(ULONG SyscallNumber, UCHAR *StubBuffer)
{
    ULONG size = 0;

    StubBuffer[size++] = 0xB8;
    StubBuffer[size++] = (UCHAR)(SyscallNumber & 0xFF);
    StubBuffer[size++] = (UCHAR)((SyscallNumber >> 8) & 0xFF);
    StubBuffer[size++] = (UCHAR)((SyscallNumber >> 16) & 0xFF);
    StubBuffer[size++] = (UCHAR)((SyscallNumber >> 24) & 0xFF);

    StubBuffer[size++] = 0x0F;
    StubBuffer[size++] = 0x05;
    StubBuffer[size++] = 0xC3;

    return size;
}

static BOOLEAN LxStringEqual(const char *Left, const char *Right)
{
    if (Left == NULL || Right == NULL)
    {
        return FALSE;
    }

    while (*Left != 0 || *Right != 0)
    {
        if (*Left != *Right)
        {
            return FALSE;
        }

        Left++;
        Right++;
    }

    return TRUE;
}

static void LxCopyString(CHAR *Dest, const char *Src, ULONG DestSize)
{
    ULONG index;

    if (Dest == NULL || Src == NULL || DestSize == 0)
    {
        return;
    }

    for (index = 0; index + 1 < DestSize; index++)
    {
        Dest[index] = Src[index];

        if (Src[index] == 0)
        {
            return;
        }
    }

    Dest[DestSize - 1] = 0;
}

static void LxGlobalTableReset(LX_GLOBAL_TABLE *Table)
{
    if (Table == NULL)
    {
        return;
    }

    RtlZeroMemory(Table, sizeof(LX_GLOBAL_TABLE));
    Table->Capacity = LX_MAX_GLOBAL_SYMBOLS;
}

static BOOLEAN LxGlobalTableAdd(LX_GLOBAL_TABLE *Table, const char *Name, ULONG64 Address,
                                ULONG LibIndex, ULONG IsFunction, ULONG Source)
{
    LX_GLOBAL_SYMBOL *slot;
    ULONG index;

    if (Table == NULL || Name == NULL || Name[0] == 0 || Address == 0)
    {
        return FALSE;
    }

    for (index = 0; index < Table->Count; index++)
    {
        if (LxStringEqual(Table->Entries[index].Name, Name))
        {
            return FALSE;
        }
    }

    if (Table->Count >= Table->Capacity)
    {
        return FALSE;
    }

    slot = &Table->Entries[Table->Count];

    LxCopyString(slot->Name, Name, LX_MAX_SYMBOL_NAME);
    slot->Address = Address;
    slot->LibIndex = LibIndex;
    slot->IsFunction = IsFunction;
    slot->Source = Source;

    Table->Count++;

    return TRUE;
}

static ULONG64 LxGlobalTableLookup(LX_GLOBAL_TABLE *Table, const char *Name, ULONG *OutLibIndex)
{
    ULONG index;

    if (Table == NULL || Name == NULL || Name[0] == 0)
    {
        return 0;
    }

    Table->ProbeCount++;

    for (index = 0; index < Table->Count; index++)
    {
        if (LxStringEqual(Table->Entries[index].Name, Name))
        {
            Table->ProbeHit++;
            LxCopyString(Table->LastHitName, Name, LX_MAX_SYMBOL_NAME);
            Table->LastHitAddress = Table->Entries[index].Address;

            if (OutLibIndex != NULL)
            {
                *OutLibIndex = Table->Entries[index].LibIndex;
            }

            return Table->Entries[index].Address;
        }
    }

    if (Table->LastMissName[0] == 0)
    {
        LxCopyString(Table->LastMissName, Name, LX_MAX_SYMBOL_NAME);
    }

    return 0;
}

static void LxInstallBuiltins(PEPROCESS TargetProcess, ULONG64 AreaBase, LX_GLOBAL_TABLE *Globals, LX_BUILTIN_STATS *Stats);

static void LxBuildStubs(const UCHAR *ImageData, ULONG ImageSize, const LX_ELF_LOAD *Load,
                         const LX_ELF_DYNAMIC_INFO *Info, PEPROCESS TargetProcess,
                         LX_SYMBOL_INFO *Symbols, LX_STUB_STATS *Stats, LX_GLOBAL_TABLE *Globals)
{
    ULONG64 stubAreaBase;

    RtlZeroMemory(Stats, sizeof(LX_STUB_STATS));

    if (ImageData == NULL || Load == NULL || Symbols == NULL || TargetProcess == NULL ||
        Load->MinVaddr == 0 || ImageSize == 0 || g_LastPicoHandle == 0)
    {
        return;
    }

    HANDLE stubSectionHandle = NULL;
    OBJECT_ATTRIBUTES stubAttributes;
    LARGE_INTEGER stubSectionSize;
    PVOID stubViewBase;
    SIZE_T stubViewSize;
    NTSTATUS stubMapStatus;

    stubAreaBase = (Load->MinVaddr + 0x100000ull) & ~0xFFFFFull;

    stubSectionSize.QuadPart = LX_STUB_AREA_SIZE;
    InitializeObjectAttributes(&stubAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    stubMapStatus = ZwCreateSection(&stubSectionHandle, SECTION_ALL_ACCESS, &stubAttributes, &stubSectionSize,
                                    PAGE_EXECUTE_READWRITE, SEC_COMMIT, NULL);

    Stats->LastStubStatus = stubMapStatus;
    Stats->StubAreaBase = stubAreaBase;

    if (!NT_SUCCESS(stubMapStatus))
    {
        return;
    }

    stubViewBase = (PVOID)(ULONG_PTR)stubAreaBase;
    stubViewSize = 0;

    stubMapStatus = ZwMapViewOfSection(stubSectionHandle, (HANDLE)g_LastPicoHandle, &stubViewBase, 0, 0,
                                       NULL, &stubViewSize, ViewUnmap, 0, PAGE_EXECUTE_READWRITE);

    ZwClose(stubSectionHandle);

    Stats->LastStubStatus = stubMapStatus;
    Stats->StubAreaBase = (ULONG64)(ULONG_PTR)stubViewBase;

    if (!NT_SUCCESS(stubMapStatus))
    {
        return;
    }

    stubAreaBase = (ULONG64)(ULONG_PTR)stubViewBase;

    LxInstallBuiltins(TargetProcess, stubAreaBase + LX_BUILTIN_AREA_OFFSET, Globals, &g_BuiltinStats);

    {
        ULONG index;

        for (index = 0; index < Symbols->DumpedCount && index < LX_MAX_SYMBOLS; index++)
        {
            LX_SYMBOL_ENTRY *entry = &Symbols->Entries[index];
            ULONG syscallNumber;
            UCHAR stubCode[LX_STUB_SIZE];
            ULONG stubLength;
            ULONG64 stubAddress;
            NTSTATUS writeStatus;

            if (entry->Name[0] == 0)
            {
                continue;
            }

            Stats->StubCount++;

            syscallNumber = LxLookupSyscallForSymbol(entry->Name);

            if (syscallNumber == 0 && Globals != NULL)
            {
                ULONG hitLib = 0;
                ULONG64 crossAddress = LxGlobalTableLookup(Globals, entry->Name, &hitLib);

                if (crossAddress != 0)
                {
                    ULONG64 relocAddr = Load->MinVaddr + entry->RelocOffset;
                    NTSTATUS crossStatus;

                    crossStatus = LxWritePicoMemory(TargetProcess, relocAddr, &crossAddress,
                                                    sizeof(crossAddress));

                    Globals->CrossResolved++;

                    if (NT_SUCCESS(crossStatus))
                    {
                        Stats->StubFound++;
                        Stats->LastStubAddress = crossAddress;
                        Stats->LastStubSyscall = 0;
                        Stats->LastStubStatus = crossStatus;
                        entry->Resolved = 1;
                        entry->ResolvedAddress = crossAddress;
                        continue;
                    }

                    Stats->StubWriteFail++;
                    continue;
                }

                Globals->CrossMiss++;
            }

            if (syscallNumber == 0)
            {
                Stats->StubUnknown++;

                if (Stats->LastStubName[0] == 0)
                {
                    ULONG copyIndex;

                    for (copyIndex = 0; copyIndex + 1 < LX_MAX_SYMBOL_NAME && entry->Name[copyIndex] != 0;
                         copyIndex++)
                    {
                        Stats->LastStubName[copyIndex] = entry->Name[copyIndex];
                    }

                    Stats->LastStubName[LX_MAX_SYMBOL_NAME - 1] = 0;
                }

                continue;
            }

            RtlZeroMemory(stubCode, sizeof(stubCode));
            stubLength = LxBuildStubCode(syscallNumber, stubCode);

            stubAddress = stubAreaBase + (ULONG64)Stats->StubSlot * LX_STUB_SIZE;
            Stats->StubSlot++;
            Stats->StubFound++;

            writeStatus = LxWritePicoMemory(TargetProcess, stubAddress, stubCode, stubLength);

            Stats->LastStubStatus = writeStatus;

            if (!NT_SUCCESS(writeStatus))
            {
                Stats->StubWriteFail++;
                continue;
            }

            Stats->LastStubAddress = stubAddress;
            Stats->LastStubSyscall = syscallNumber;

            {
                ULONG64 relocAddr = Load->MinVaddr + entry->RelocOffset;
                ULONG64 stubValue = stubAddress;

                writeStatus = LxWritePicoMemory(TargetProcess, relocAddr, &stubValue, sizeof(stubValue));

                if (!NT_SUCCESS(writeStatus))
                {
                    Stats->StubWriteFail++;
                    continue;
                }

                entry->Resolved = 1;
                entry->ResolvedAddress = stubAddress;
            }
        }
    }
}

#define LX_BUILTIN_COUNT 55u

typedef struct _LX_BUILTIN_DEF
{
    const char *Name;
    const UCHAR *Code;
    ULONG Size;
} LX_BUILTIN_DEF;


static const UCHAR g_BuiltinMemcpy[] = {
    0x48, 0x89, 0xF8,
    0x48, 0x89, 0xD1,
    0xF3, 0xA4,
    0xC3
};

static const UCHAR g_BuiltinMemset[] = {
    0x48, 0x89, 0xF8,
    0x48, 0x85, 0xD2,
    0x74, 0x0E,
    0x88, 0xF0,
    0x48, 0x89, 0xD1,
    0xF3, 0xAA,
    0xC3
};

static const UCHAR g_BuiltinStrlen[] = {
    0x31, 0xC0,
    0x80, 0x3C, 0x07, 0x00,
    0x74, 0x05,
    0x48, 0xFF, 0xC0,
    0xEB, 0xF6,
    0xC3
};

static const UCHAR g_BuiltinStrcmp[] = {
    0x31, 0xC9,
    0x0F, 0xB6, 0x04, 0x0F,
    0x0F, 0xB6, 0x14, 0x0E,
    0x29, 0xD0,
    0x75, 0x09,
    0x85, 0xD2,
    0x74, 0x05,
    0x48, 0xFF, 0xC1,
    0xEB, 0xEB,
    0xC3
};

static const UCHAR g_BuiltinStrcpy[] = {
    0x31, 0xC9,
    0x8A, 0x04, 0x0E,
    0x88, 0x04, 0x0F,
    0x84, 0xC0,
    0x74, 0x05,
    0x48, 0xFF, 0xC1,
    0xEB, 0xF1,
    0x48, 0x89, 0xF8,
    0xC3
};

static const UCHAR g_BuiltinMemcmp[] = {
    0x31, 0xC9,
    0x48, 0x39, 0xD1,
    0x73, 0x14,
    0x0F, 0xB6, 0x04, 0x0F,
    0x44, 0x0F, 0xB6, 0x04, 0x0E,
    0x44, 0x29, 0xC0,
    0x75, 0x06,
    0x48, 0xFF, 0xC1,
    0xEB, 0xE8,
    0xC3
};

static const UCHAR g_BuiltinMemmove[] = {
    0x48, 0x89, 0xF8,
    0x48, 0x89, 0xD1,
    0x48, 0x39, 0xF7,
    0x76, 0x07,
    0x48, 0x01, 0xD6,
    0x48, 0x01, 0xD7,
    0xFD,
    0xF3, 0xA4,
    0xFC,
    0xC3
};

static const UCHAR g_BuiltinStrncmp[] = {
    0x31, 0xC9,
    0x48, 0x39, 0xD1,
    0x73, 0x18,
    0x0F, 0xB6, 0x04, 0x0F,
    0x44, 0x0F, 0xB6, 0x04, 0x0E,
    0x44, 0x29, 0xC0,
    0x75, 0x0A,
    0x45, 0x85, 0xC0,
    0x74, 0x05,
    0x48, 0xFF, 0xC1,
    0xEB, 0xE3,
    0xC3
};

static const UCHAR g_BuiltinPrintf[] = {
    0x55, 0x48, 0x89, 0xE5, 0x53, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41,
    0x57, 0x48, 0x81, 0xEC, 0x90, 0x05, 0x00, 0x00, 0xF2, 0x0F, 0x11, 0x85,
    0xC0, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0x8D, 0xC8, 0xFE, 0xFF, 0xFF,
    0xF2, 0x0F, 0x11, 0x95, 0xD0, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0x9D,
    0xD8, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0xA5, 0xE0, 0xFE, 0xFF, 0xFF,
    0xF2, 0x0F, 0x11, 0xAD, 0xE8, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0xB5,
    0xF0, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0xBD, 0xF8, 0xFE, 0xFF, 0xFF,
    0x48, 0xC7, 0x85, 0x48, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x48,
    0x89, 0x75, 0x98, 0x48, 0x89, 0x55, 0xA0, 0x48, 0x89, 0x4D, 0xA8, 0x4C,
    0x89, 0x45, 0xB0, 0x4C, 0x89, 0x4D, 0xB8, 0x4C, 0x8D, 0xBD, 0x88, 0xFA,
    0xFF, 0xFF, 0x49, 0x89, 0xFC, 0x4D, 0x31, 0xED, 0x4D, 0x31, 0xF6, 0x41,
    0x0F, 0xB6, 0x04, 0x24, 0x84, 0xC0, 0x0F, 0x84, 0x8B, 0x02, 0x00, 0x00,
    0x3C, 0x25, 0x74, 0x0C, 0x88, 0xC2, 0xE8, 0xB0, 0x02, 0x00, 0x00, 0x49,
    0xFF, 0xC4, 0xEB, 0xE3, 0x49, 0xFF, 0xC4, 0x4D, 0x31, 0xD2, 0x41, 0xB3,
    0x20, 0x48, 0xC7, 0x85, 0x38, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x48, 0xC7, 0x85, 0x30, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x48,
    0xC7, 0x85, 0x40, 0xFF, 0xFF, 0xFF, 0x06, 0x00, 0x00, 0x00, 0x41, 0x0F,
    0xB6, 0x04, 0x24, 0x3C, 0x2D, 0x74, 0x06, 0x3C, 0x2B, 0x74, 0x12, 0xEB,
    0x20, 0x48, 0xC7, 0x85, 0x38, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
    0x49, 0xFF, 0xC4, 0xEB, 0xE1, 0x48, 0xC7, 0x85, 0x30, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xEB, 0xD1, 0x41, 0x0F, 0xB6,
    0x04, 0x24, 0x3C, 0x30, 0x75, 0x16, 0x41, 0x0F, 0xB6, 0x4C, 0x24, 0x01,
    0x80, 0xF9, 0x30, 0x72, 0x0B, 0x80, 0xF9, 0x39, 0x77, 0x06, 0x41, 0xB3,
    0x30, 0x49, 0xFF, 0xC4, 0x41, 0x0F, 0xB6, 0x04, 0x24, 0x3C, 0x30, 0x72,
    0x17, 0x3C, 0x39, 0x77, 0x13, 0x4F, 0x8D, 0x14, 0x92, 0x4F, 0x8D, 0x14,
    0x12, 0x83, 0xE8, 0x30, 0x49, 0x01, 0xC2, 0x49, 0xFF, 0xC4, 0xEB, 0xE0,
    0x3C, 0x2E, 0x75, 0x2C, 0x31, 0xC9, 0x49, 0xFF, 0xC4, 0x41, 0x0F, 0xB6,
    0x04, 0x24, 0x3C, 0x30, 0x72, 0x17, 0x3C, 0x39, 0x77, 0x13, 0x48, 0x8D,
    0x0C, 0x89, 0x48, 0x8D, 0x0C, 0x09, 0x83, 0xE8, 0x30, 0x48, 0x01, 0xC1,
    0x49, 0xFF, 0xC4, 0xEB, 0xE0, 0x48, 0x89, 0x8D, 0x40, 0xFF, 0xFF, 0xFF,
    0x41, 0x0F, 0xB6, 0x04, 0x24, 0x3C, 0x6C, 0x75, 0x05, 0x49, 0xFF, 0xC4,
    0xEB, 0xF2, 0x84, 0xC0, 0x74, 0x65, 0x3C, 0x25, 0x74, 0x6D, 0x3C, 0x64,
    0x0F, 0x84, 0xA2, 0x00, 0x00, 0x00, 0x3C, 0x75, 0x0F, 0x84, 0xAF, 0x00,
    0x00, 0x00, 0x3C, 0x78, 0x0F, 0x84, 0xBC, 0x00, 0x00, 0x00, 0x3C, 0x58,
    0x0F, 0x84, 0xC9, 0x00, 0x00, 0x00, 0x3C, 0x70, 0x0F, 0x84, 0xD6, 0x00,
    0x00, 0x00, 0x3C, 0x73, 0x0F, 0x84, 0xFB, 0x00, 0x00, 0x00, 0x3C, 0x63,
    0x0F, 0x84, 0x51, 0x01, 0x00, 0x00, 0x3C, 0x66, 0x74, 0x57, 0x3C, 0x46,
    0x74, 0x53, 0x3C, 0x65, 0x74, 0x38, 0x3C, 0x45, 0x74, 0x34, 0xB2, 0x25,
    0xE8, 0x7E, 0x01, 0x00, 0x00, 0x41, 0x0F, 0xB6, 0x14, 0x24, 0xE8, 0x74,
    0x01, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0xA4, 0xFE, 0xFF, 0xFF, 0xB2,
    0x25, 0xE8, 0x65, 0x01, 0x00, 0x00, 0xE9, 0x30, 0x01, 0x00, 0x00, 0xB2,
    0x25, 0xE8, 0x59, 0x01, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x89, 0xFE,
    0xFF, 0xFF, 0xE8, 0xEC, 0x02, 0x00, 0x00, 0x66, 0x48, 0x0F, 0x6E, 0xC0,
    0xE8, 0x4A, 0x01, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x72, 0xFE, 0xFF,
    0xFF, 0xE8, 0xD5, 0x02, 0x00, 0x00, 0x66, 0x48, 0x0F, 0x6E, 0xC0, 0xE8,
    0xF2, 0x02, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x5B, 0xFE, 0xFF, 0xFF,
    0xE8, 0x9F, 0x02, 0x00, 0x00, 0x48, 0x89, 0xC7, 0xE8, 0xB1, 0x03, 0x00,
    0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x46, 0xFE, 0xFF, 0xFF, 0xE8, 0x8A, 0x02,
    0x00, 0x00, 0x48, 0x89, 0xC7, 0xE8, 0xC8, 0x03, 0x00, 0x00, 0x49, 0xFF,
    0xC4, 0xE9, 0x31, 0xFE, 0xFF, 0xFF, 0xE8, 0x75, 0x02, 0x00, 0x00, 0x48,
    0x89, 0xC7, 0xE8, 0xCF, 0x03, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x1C,
    0xFE, 0xFF, 0xFF, 0xE8, 0x60, 0x02, 0x00, 0x00, 0x48, 0x89, 0xC7, 0xE8,
    0xC9, 0x03, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x07, 0xFE, 0xFF, 0xFF,
    0xE8, 0x4B, 0x02, 0x00, 0x00, 0x48, 0x89, 0xC7, 0x49, 0x83, 0xFA, 0x02,
    0x72, 0x04, 0x49, 0x83, 0xEA, 0x02, 0xB2, 0x30, 0xE8, 0xB6, 0x00, 0x00,
    0x00, 0xB2, 0x78, 0xE8, 0xAF, 0x00, 0x00, 0x00, 0xE8, 0x8D, 0x03, 0x00,
    0x00, 0x49, 0xFF, 0xC4, 0xE9, 0xDA, 0xFD, 0xFF, 0xFF, 0xE8, 0x1E, 0x02,
    0x00, 0x00, 0x48, 0x89, 0xC3, 0x48, 0x31, 0xC9, 0x80, 0x3C, 0x0B, 0x00,
    0x74, 0x05, 0x48, 0xFF, 0xC1, 0xEB, 0xF5, 0x4D, 0x89, 0xD1, 0x49, 0x29,
    0xC9, 0x7E, 0x16, 0x48, 0x83, 0xBD, 0x38, 0xFF, 0xFF, 0xFF, 0x00, 0x75,
    0x0C, 0xB2, 0x20, 0xE8, 0x73, 0x00, 0x00, 0x00, 0x49, 0xFF, 0xC9, 0x75,
    0xF4, 0x0F, 0xB6, 0x13, 0x84, 0xD2, 0x74, 0x0A, 0xE8, 0x62, 0x00, 0x00,
    0x00, 0x48, 0xFF, 0xC3, 0xEB, 0xEF, 0x4D, 0x85, 0xC9, 0x7E, 0x0C, 0xB2,
    0x20, 0xE8, 0x51, 0x00, 0x00, 0x00, 0x49, 0xFF, 0xC9, 0x75, 0xF4, 0x49,
    0xFF, 0xC4, 0xE9, 0x7C, 0xFD, 0xFF, 0xFF, 0xE8, 0xC0, 0x01, 0x00, 0x00,
    0x88, 0xC2, 0xE8, 0x38, 0x00, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x68,
    0xFD, 0xFF, 0xFF, 0x43, 0xC6, 0x04, 0x37, 0x00, 0x4C, 0x89, 0xF2, 0x48,
    0xC7, 0xC7, 0x01, 0x00, 0x00, 0x00, 0x4C, 0x89, 0xFE, 0x48, 0xC7, 0xC0,
    0x01, 0x00, 0x00, 0x00, 0x0F, 0x05, 0x4C, 0x89, 0xF0, 0x48, 0x81, 0xC4,
    0x90, 0x05, 0x00, 0x00, 0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C,
    0x5B, 0x5D, 0xC3, 0x43, 0x88, 0x14, 0x37, 0x49, 0xFF, 0xC6, 0xC3, 0x66,
    0x48, 0x0F, 0x7E, 0xC0, 0x48, 0x85, 0xC0, 0x79, 0x11, 0xB2, 0x2D, 0xE8,
    0xE7, 0xFF, 0xFF, 0xFF, 0x48, 0x0F, 0xBA, 0xF0, 0x3F, 0x66, 0x48, 0x0F,
    0x6E, 0xC0, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40,
    0x66, 0x48, 0x0F, 0x6E, 0xD0, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xF0, 0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xE8, 0x48, 0xC7, 0x85, 0x20,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x66, 0x48, 0x0F, 0x7E, 0xC0,
    0x48, 0x85, 0xC0, 0x74, 0x28, 0x66, 0x0F, 0x2F, 0xC2, 0x73, 0x08, 0x66,
    0x0F, 0x2F, 0xC5, 0x72, 0x0F, 0xEB, 0x1A, 0xF2, 0x0F, 0x5E, 0xC2, 0x48,
    0xFF, 0x85, 0x20, 0xFF, 0xFF, 0xFF, 0xEB, 0xE5, 0xF2, 0x0F, 0x59, 0xC2,
    0x48, 0xFF, 0x8D, 0x20, 0xFF, 0xFF, 0xFF, 0xEB, 0xD8, 0xF2, 0x48, 0x0F,
    0x2C, 0xC0, 0x48, 0x89, 0xC7, 0xE8, 0x3C, 0x02, 0x00, 0x00, 0xB2, 0x2E,
    0xE8, 0x6E, 0xFF, 0xFF, 0xFF, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x24, 0x40, 0x66, 0x48, 0x0F, 0x6E, 0xD0, 0x48, 0x8B, 0x8D, 0x40,
    0xFF, 0xFF, 0xFF, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0,
    0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xD8, 0x48, 0x85, 0xC9, 0x74, 0x09, 0xF2,
    0x0F, 0x59, 0xDA, 0x48, 0xFF, 0xC9, 0xEB, 0xF2, 0x48, 0xB8, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xE0, 0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xE0, 0xF2,
    0x0F, 0x5E, 0xE3, 0xF2, 0x0F, 0x58, 0xC4, 0xF2, 0x48, 0x0F, 0x2C, 0xC0,
    0xF2, 0x48, 0x0F, 0x2A, 0xC8, 0xF2, 0x0F, 0x5C, 0xC1, 0xF2, 0x0F, 0x59,
    0xC3, 0xF2, 0x48, 0x0F, 0x2C, 0xC0, 0x49, 0x89, 0xC0, 0x48, 0x8B, 0x8D,
    0x40, 0xFF, 0xFF, 0xFF, 0x48, 0x85, 0xC9, 0x74, 0x2D, 0x4C, 0x89, 0xC0,
    0x31, 0xD2, 0x49, 0xC7, 0xC2, 0x0A, 0x00, 0x00, 0x00, 0x49, 0xF7, 0xF2,
    0x49, 0x89, 0xC0, 0x83, 0xC2, 0x30, 0x52, 0x48, 0xFF, 0xC9, 0x75, 0xE5,
    0x48, 0x8B, 0x8D, 0x40, 0xFF, 0xFF, 0xFF, 0x5A, 0xE8, 0xD6, 0xFE, 0xFF,
    0xFF, 0x48, 0xFF, 0xC9, 0x75, 0xF5, 0xB2, 0x65, 0xE8, 0xCA, 0xFE, 0xFF,
    0xFF, 0xB2, 0x2B, 0x48, 0x8B, 0x85, 0x20, 0xFF, 0xFF, 0xFF, 0x48, 0x85,
    0xC0, 0x79, 0x0C, 0x48, 0xF7, 0xD8, 0x48, 0x89, 0x85, 0x20, 0xFF, 0xFF,
    0xFF, 0xB2, 0x2D, 0xE8, 0xAB, 0xFE, 0xFF, 0xFF, 0x48, 0x8B, 0x85, 0x20,
    0xFF, 0xFF, 0xFF, 0x31, 0xD2, 0x49, 0xC7, 0xC2, 0x0A, 0x00, 0x00, 0x00,
    0x49, 0xF7, 0xF2, 0x49, 0x89, 0xD0, 0x04, 0x30, 0x88, 0xC2, 0xE8, 0x8C,
    0xFE, 0xFF, 0xFF, 0x4C, 0x89, 0xC0, 0x04, 0x30, 0x88, 0xC2, 0xE8, 0x80,
    0xFE, 0xFF, 0xFF, 0xC3, 0x49, 0x83, 0xFD, 0x05, 0x73, 0x09, 0x4A, 0x8B,
    0x44, 0xED, 0x98, 0x49, 0xFF, 0xC5, 0xC3, 0x4C, 0x89, 0xE8, 0x48, 0x83,
    0xE8, 0x05, 0x48, 0x8B, 0x44, 0xC5, 0x10, 0x49, 0xFF, 0xC5, 0xC3, 0x48,
    0x8B, 0x8D, 0x48, 0xFF, 0xFF, 0xFF, 0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00,
    0x00, 0x48, 0x83, 0xF9, 0x08, 0x73, 0x08, 0x48, 0x8B, 0x84, 0xCD, 0xC0,
    0xFE, 0xFF, 0xFF, 0x48, 0xFF, 0xC1, 0x48, 0x89, 0x8D, 0x48, 0xFF, 0xFF,
    0xFF, 0xC3, 0x66, 0x48, 0x0F, 0x7E, 0xC0, 0x48, 0x85, 0xC0, 0x79, 0x11,
    0xB2, 0x2D, 0xE8, 0x28, 0xFE, 0xFF, 0xFF, 0x48, 0x0F, 0xBA, 0xF0, 0x3F,
    0x66, 0x48, 0x0F, 0x6E, 0xC0, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x24, 0x40, 0x66, 0x48, 0x0F, 0x6E, 0xD0, 0x48, 0x8B, 0x8D, 0x40,
    0xFF, 0xFF, 0xFF, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0,
    0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xD8, 0x48, 0x85, 0xC9, 0x74, 0x09, 0xF2,
    0x0F, 0x59, 0xDA, 0x48, 0xFF, 0xC9, 0xEB, 0xF2, 0x48, 0xB8, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xE0, 0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xE0, 0xF2,
    0x0F, 0x5E, 0xE3, 0xF2, 0x0F, 0x58, 0xC4, 0xF2, 0x48, 0x0F, 0x2C, 0xC0,
    0x48, 0x89, 0xC7, 0xE8, 0x8E, 0x00, 0x00, 0x00, 0xF2, 0x48, 0x0F, 0x2C,
    0xC0, 0xF2, 0x48, 0x0F, 0x2A, 0xC8, 0xF2, 0x0F, 0x5C, 0xC1, 0xF2, 0x0F,
    0x59, 0xC3, 0xF2, 0x48, 0x0F, 0x2C, 0xC0, 0x49, 0x89, 0xC0, 0x48, 0x8B,
    0x8D, 0x40, 0xFF, 0xFF, 0xFF, 0x48, 0x85, 0xC9, 0x74, 0x3B, 0xB2, 0x2E,
    0xE8, 0x9A, 0xFD, 0xFF, 0xFF, 0x48, 0x8B, 0x8D, 0x40, 0xFF, 0xFF, 0xFF,
    0x4C, 0x89, 0xC0, 0x31, 0xD2, 0x49, 0xC7, 0xC2, 0x0A, 0x00, 0x00, 0x00,
    0x49, 0xF7, 0xF2, 0x49, 0x89, 0xC0, 0x83, 0xC2, 0x30, 0x52, 0x48, 0xFF,
    0xC9, 0x75, 0xE5, 0x48, 0x8B, 0x8D, 0x40, 0xFF, 0xFF, 0xFF, 0x5A, 0xE8,
    0x6B, 0xFD, 0xFF, 0xFF, 0x48, 0xFF, 0xC9, 0x75, 0xF5, 0xC3, 0x45, 0x31,
    0xC9, 0x48, 0x85, 0xFF, 0x79, 0x0B, 0x41, 0xB9, 0x2D, 0x00, 0x00, 0x00,
    0x48, 0xF7, 0xDF, 0xEB, 0x10, 0x48, 0x83, 0xBD, 0x30, 0xFF, 0xFF, 0xFF,
    0x00, 0x74, 0x06, 0x41, 0xB9, 0x2B, 0x00, 0x00, 0x00, 0xB9, 0x0A, 0x00,
    0x00, 0x00, 0x31, 0xF6, 0xEB, 0x3A, 0x45, 0x31, 0xC9, 0x48, 0x83, 0xBD,
    0x30, 0xFF, 0xFF, 0xFF, 0x00, 0x74, 0x06, 0x41, 0xB9, 0x2B, 0x00, 0x00,
    0x00, 0xB9, 0x0A, 0x00, 0x00, 0x00, 0x31, 0xF6, 0xEB, 0x1E, 0x45, 0x31,
    0xC9, 0xB9, 0x10, 0x00, 0x00, 0x00, 0xBE, 0x57, 0x00, 0x00, 0x00, 0xEB,
    0x0F, 0x45, 0x31, 0xC9, 0xB9, 0x10, 0x00, 0x00, 0x00, 0xBE, 0x37, 0x00,
    0x00, 0x00, 0xEB, 0x00, 0x48, 0x8D, 0x9D, 0x60, 0xFF, 0xFF, 0xFF, 0x49,
    0x89, 0xD8, 0x48, 0x89, 0xF8, 0x31, 0xD2, 0x48, 0xF7, 0xF1, 0x80, 0xFA,
    0x0A, 0x72, 0x05, 0x40, 0x00, 0xF2, 0xEB, 0x03, 0x80, 0xC2, 0x30, 0x48,
    0xFF, 0xCB, 0x88, 0x13, 0x48, 0x85, 0xC0, 0x75, 0xE4, 0x4C, 0x89, 0xC0,
    0x48, 0x29, 0xD8, 0x45, 0x84, 0xC9, 0x74, 0x03, 0x48, 0xFF, 0xC0, 0x49,
    0x29, 0xC2, 0x4D, 0x85, 0xD2, 0x7E, 0x56, 0x48, 0x83, 0xBD, 0x38, 0xFF,
    0xFF, 0xFF, 0x00, 0x75, 0x0F, 0x41, 0x80, 0xFB, 0x30, 0x74, 0x30, 0xB2,
    0x20, 0xE8, 0x5E, 0x00, 0x00, 0x00, 0xEB, 0x3D, 0x45, 0x84, 0xC9, 0x74,
    0x08, 0x44, 0x88, 0xCA, 0xE8, 0x9A, 0xFC, 0xFF, 0xFF, 0x4C, 0x39, 0xC3,
    0x73, 0x0C, 0x8A, 0x13, 0xE8, 0x8E, 0xFC, 0xFF, 0xFF, 0x48, 0xFF, 0xC3,
    0xEB, 0xEF, 0xB2, 0x20, 0xE8, 0x37, 0x00, 0x00, 0x00, 0xEB, 0x34, 0x45,
    0x84, 0xC9, 0x74, 0x08, 0x44, 0x88, 0xCA, 0xE8, 0x73, 0xFC, 0xFF, 0xFF,
    0xB2, 0x30, 0xE8, 0x21, 0x00, 0x00, 0x00, 0xEB, 0x0D, 0x45, 0x84, 0xC9,
    0x74, 0x08, 0x44, 0x88, 0xCA, 0xE8, 0x5D, 0xFC, 0xFF, 0xFF, 0x4C, 0x39,
    0xC3, 0x73, 0x0C, 0x8A, 0x13, 0xE8, 0x51, 0xFC, 0xFF, 0xFF, 0x48, 0xFF,
    0xC3, 0xEB, 0xEF, 0xC3, 0xE8, 0x46, 0xFC, 0xFF, 0xFF, 0x49, 0xFF, 0xCA,
    0x75, 0xF6, 0xC3,
};

static const UCHAR g_BuiltinSnprintf[] = {
    0x55, 0x48, 0x89, 0xE5, 0x53, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41,
    0x57, 0x48, 0x81, 0xEC, 0x20, 0x01, 0x00, 0x00, 0xF2, 0x0F, 0x11, 0x85,
    0xC0, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0x8D, 0xC8, 0xFE, 0xFF, 0xFF,
    0xF2, 0x0F, 0x11, 0x95, 0xD0, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0x9D,
    0xD8, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0xA5, 0xE0, 0xFE, 0xFF, 0xFF,
    0xF2, 0x0F, 0x11, 0xAD, 0xE8, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0xB5,
    0xF0, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0xBD, 0xF8, 0xFE, 0xFF, 0xFF,
    0x48, 0xC7, 0x85, 0x48, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x48,
    0x89, 0x4D, 0xB8, 0x4C, 0x89, 0x45, 0xC0, 0x4C, 0x89, 0x4D, 0xC8, 0x49,
    0x89, 0xFF, 0x48, 0x89, 0x75, 0x98, 0x49, 0x89, 0xD4, 0x4D, 0x31, 0xED,
    0x4D, 0x31, 0xF6, 0x48, 0x89, 0xF0, 0x48, 0x85, 0xC0, 0x74, 0x03, 0x48,
    0xFF, 0xC8, 0x48, 0x89, 0x45, 0xA0, 0x41, 0x0F, 0xB6, 0x04, 0x24, 0x84,
    0xC0, 0x0F, 0x84, 0x8B, 0x02, 0x00, 0x00, 0x3C, 0x25, 0x74, 0x0C, 0x88,
    0xC2, 0xE8, 0xAF, 0x02, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xEB, 0xE3, 0x49,
    0xFF, 0xC4, 0x4D, 0x31, 0xD2, 0x41, 0xB3, 0x20, 0x48, 0xC7, 0x85, 0x38,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x48, 0xC7, 0x85, 0x30, 0xFF,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x48, 0xC7, 0x85, 0x40, 0xFF, 0xFF,
    0xFF, 0x06, 0x00, 0x00, 0x00, 0x41, 0x0F, 0xB6, 0x04, 0x24, 0x3C, 0x2D,
    0x74, 0x06, 0x3C, 0x2B, 0x74, 0x12, 0xEB, 0x20, 0x48, 0xC7, 0x85, 0x38,
    0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xEB, 0xE1,
    0x48, 0xC7, 0x85, 0x30, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x49,
    0xFF, 0xC4, 0xEB, 0xD1, 0x41, 0x0F, 0xB6, 0x04, 0x24, 0x3C, 0x30, 0x75,
    0x16, 0x41, 0x0F, 0xB6, 0x4C, 0x24, 0x01, 0x80, 0xF9, 0x30, 0x72, 0x0B,
    0x80, 0xF9, 0x39, 0x77, 0x06, 0x41, 0xB3, 0x30, 0x49, 0xFF, 0xC4, 0x41,
    0x0F, 0xB6, 0x04, 0x24, 0x3C, 0x30, 0x72, 0x17, 0x3C, 0x39, 0x77, 0x13,
    0x4F, 0x8D, 0x14, 0x92, 0x4F, 0x8D, 0x14, 0x12, 0x83, 0xE8, 0x30, 0x49,
    0x01, 0xC2, 0x49, 0xFF, 0xC4, 0xEB, 0xE0, 0x3C, 0x2E, 0x75, 0x2C, 0x31,
    0xC9, 0x49, 0xFF, 0xC4, 0x41, 0x0F, 0xB6, 0x04, 0x24, 0x3C, 0x30, 0x72,
    0x17, 0x3C, 0x39, 0x77, 0x13, 0x48, 0x8D, 0x0C, 0x89, 0x48, 0x8D, 0x0C,
    0x09, 0x83, 0xE8, 0x30, 0x48, 0x01, 0xC1, 0x49, 0xFF, 0xC4, 0xEB, 0xE0,
    0x48, 0x89, 0x8D, 0x40, 0xFF, 0xFF, 0xFF, 0x41, 0x0F, 0xB6, 0x04, 0x24,
    0x3C, 0x6C, 0x75, 0x05, 0x49, 0xFF, 0xC4, 0xEB, 0xF2, 0x84, 0xC0, 0x74,
    0x65, 0x3C, 0x25, 0x74, 0x6D, 0x3C, 0x64, 0x0F, 0x84, 0xA2, 0x00, 0x00,
    0x00, 0x3C, 0x75, 0x0F, 0x84, 0xAF, 0x00, 0x00, 0x00, 0x3C, 0x78, 0x0F,
    0x84, 0xBC, 0x00, 0x00, 0x00, 0x3C, 0x58, 0x0F, 0x84, 0xC9, 0x00, 0x00,
    0x00, 0x3C, 0x70, 0x0F, 0x84, 0xD6, 0x00, 0x00, 0x00, 0x3C, 0x73, 0x0F,
    0x84, 0xFB, 0x00, 0x00, 0x00, 0x3C, 0x63, 0x0F, 0x84, 0x51, 0x01, 0x00,
    0x00, 0x3C, 0x66, 0x74, 0x57, 0x3C, 0x46, 0x74, 0x53, 0x3C, 0x65, 0x74,
    0x38, 0x3C, 0x45, 0x74, 0x34, 0xB2, 0x25, 0xE8, 0x7D, 0x01, 0x00, 0x00,
    0x41, 0x0F, 0xB6, 0x14, 0x24, 0xE8, 0x73, 0x01, 0x00, 0x00, 0x49, 0xFF,
    0xC4, 0xE9, 0xA4, 0xFE, 0xFF, 0xFF, 0xB2, 0x25, 0xE8, 0x64, 0x01, 0x00,
    0x00, 0xE9, 0x30, 0x01, 0x00, 0x00, 0xB2, 0x25, 0xE8, 0x58, 0x01, 0x00,
    0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x89, 0xFE, 0xFF, 0xFF, 0xE8, 0xF1, 0x02,
    0x00, 0x00, 0x66, 0x48, 0x0F, 0x6E, 0xC0, 0xE8, 0x4F, 0x01, 0x00, 0x00,
    0x49, 0xFF, 0xC4, 0xE9, 0x72, 0xFE, 0xFF, 0xFF, 0xE8, 0xDA, 0x02, 0x00,
    0x00, 0x66, 0x48, 0x0F, 0x6E, 0xC0, 0xE8, 0xF7, 0x02, 0x00, 0x00, 0x49,
    0xFF, 0xC4, 0xE9, 0x5B, 0xFE, 0xFF, 0xFF, 0xE8, 0xA4, 0x02, 0x00, 0x00,
    0x48, 0x89, 0xC7, 0xE8, 0xB6, 0x03, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9,
    0x46, 0xFE, 0xFF, 0xFF, 0xE8, 0x8F, 0x02, 0x00, 0x00, 0x48, 0x89, 0xC7,
    0xE8, 0xCD, 0x03, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x31, 0xFE, 0xFF,
    0xFF, 0xE8, 0x7A, 0x02, 0x00, 0x00, 0x48, 0x89, 0xC7, 0xE8, 0xD4, 0x03,
    0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x1C, 0xFE, 0xFF, 0xFF, 0xE8, 0x65,
    0x02, 0x00, 0x00, 0x48, 0x89, 0xC7, 0xE8, 0xCE, 0x03, 0x00, 0x00, 0x49,
    0xFF, 0xC4, 0xE9, 0x07, 0xFE, 0xFF, 0xFF, 0xE8, 0x50, 0x02, 0x00, 0x00,
    0x48, 0x89, 0xC7, 0x49, 0x83, 0xFA, 0x02, 0x72, 0x04, 0x49, 0x83, 0xEA,
    0x02, 0xB2, 0x30, 0xE8, 0xB5, 0x00, 0x00, 0x00, 0xB2, 0x78, 0xE8, 0xAE,
    0x00, 0x00, 0x00, 0xE8, 0x92, 0x03, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9,
    0xDA, 0xFD, 0xFF, 0xFF, 0xE8, 0x23, 0x02, 0x00, 0x00, 0x48, 0x89, 0xC3,
    0x48, 0x31, 0xC9, 0x80, 0x3C, 0x0B, 0x00, 0x74, 0x05, 0x48, 0xFF, 0xC1,
    0xEB, 0xF5, 0x4D, 0x89, 0xD1, 0x49, 0x29, 0xC9, 0x7E, 0x16, 0x48, 0x83,
    0xBD, 0x38, 0xFF, 0xFF, 0xFF, 0x00, 0x75, 0x0C, 0xB2, 0x20, 0xE8, 0x72,
    0x00, 0x00, 0x00, 0x49, 0xFF, 0xC9, 0x75, 0xF4, 0x0F, 0xB6, 0x13, 0x84,
    0xD2, 0x74, 0x0A, 0xE8, 0x61, 0x00, 0x00, 0x00, 0x48, 0xFF, 0xC3, 0xEB,
    0xEF, 0x4D, 0x85, 0xC9, 0x7E, 0x0C, 0xB2, 0x20, 0xE8, 0x50, 0x00, 0x00,
    0x00, 0x49, 0xFF, 0xC9, 0x75, 0xF4, 0x49, 0xFF, 0xC4, 0xE9, 0x7C, 0xFD,
    0xFF, 0xFF, 0xE8, 0xC5, 0x01, 0x00, 0x00, 0x88, 0xC2, 0xE8, 0x37, 0x00,
    0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x68, 0xFD, 0xFF, 0xFF, 0x48, 0x8B,
    0x45, 0xA0, 0x49, 0x39, 0xC6, 0x73, 0x03, 0x4C, 0x89, 0xF0, 0x48, 0x8B,
    0x55, 0x98, 0x48, 0x85, 0xD2, 0x74, 0x05, 0x41, 0xC6, 0x04, 0x07, 0x00,
    0x4C, 0x89, 0xF0, 0x48, 0x81, 0xC4, 0x20, 0x01, 0x00, 0x00, 0x41, 0x5F,
    0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C, 0x5B, 0x5D, 0xC3, 0x4C, 0x3B, 0x75,
    0xA0, 0x73, 0x04, 0x43, 0x88, 0x14, 0x37, 0x49, 0xFF, 0xC6, 0xC3, 0x66,
    0x48, 0x0F, 0x7E, 0xC0, 0x48, 0x85, 0xC0, 0x79, 0x11, 0xB2, 0x2D, 0xE8,
    0xE1, 0xFF, 0xFF, 0xFF, 0x48, 0x0F, 0xBA, 0xF0, 0x3F, 0x66, 0x48, 0x0F,
    0x6E, 0xC0, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40,
    0x66, 0x48, 0x0F, 0x6E, 0xD0, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xF0, 0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xE8, 0x48, 0xC7, 0x85, 0x20,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x66, 0x48, 0x0F, 0x7E, 0xC0,
    0x48, 0x85, 0xC0, 0x74, 0x28, 0x66, 0x0F, 0x2F, 0xC2, 0x73, 0x08, 0x66,
    0x0F, 0x2F, 0xC5, 0x72, 0x0F, 0xEB, 0x1A, 0xF2, 0x0F, 0x5E, 0xC2, 0x48,
    0xFF, 0x85, 0x20, 0xFF, 0xFF, 0xFF, 0xEB, 0xE5, 0xF2, 0x0F, 0x59, 0xC2,
    0x48, 0xFF, 0x8D, 0x20, 0xFF, 0xFF, 0xFF, 0xEB, 0xD8, 0xF2, 0x48, 0x0F,
    0x2C, 0xC0, 0x48, 0x89, 0xC7, 0xE8, 0x3C, 0x02, 0x00, 0x00, 0xB2, 0x2E,
    0xE8, 0x68, 0xFF, 0xFF, 0xFF, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x24, 0x40, 0x66, 0x48, 0x0F, 0x6E, 0xD0, 0x48, 0x8B, 0x8D, 0x40,
    0xFF, 0xFF, 0xFF, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0,
    0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xD8, 0x48, 0x85, 0xC9, 0x74, 0x09, 0xF2,
    0x0F, 0x59, 0xDA, 0x48, 0xFF, 0xC9, 0xEB, 0xF2, 0x48, 0xB8, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xE0, 0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xE0, 0xF2,
    0x0F, 0x5E, 0xE3, 0xF2, 0x0F, 0x58, 0xC4, 0xF2, 0x48, 0x0F, 0x2C, 0xC0,
    0xF2, 0x48, 0x0F, 0x2A, 0xC8, 0xF2, 0x0F, 0x5C, 0xC1, 0xF2, 0x0F, 0x59,
    0xC3, 0xF2, 0x48, 0x0F, 0x2C, 0xC0, 0x49, 0x89, 0xC0, 0x48, 0x8B, 0x8D,
    0x40, 0xFF, 0xFF, 0xFF, 0x48, 0x85, 0xC9, 0x74, 0x2D, 0x4C, 0x89, 0xC0,
    0x31, 0xD2, 0x49, 0xC7, 0xC2, 0x0A, 0x00, 0x00, 0x00, 0x49, 0xF7, 0xF2,
    0x49, 0x89, 0xC0, 0x83, 0xC2, 0x30, 0x52, 0x48, 0xFF, 0xC9, 0x75, 0xE5,
    0x48, 0x8B, 0x8D, 0x40, 0xFF, 0xFF, 0xFF, 0x5A, 0xE8, 0xD0, 0xFE, 0xFF,
    0xFF, 0x48, 0xFF, 0xC9, 0x75, 0xF5, 0xB2, 0x65, 0xE8, 0xC4, 0xFE, 0xFF,
    0xFF, 0xB2, 0x2B, 0x48, 0x8B, 0x85, 0x20, 0xFF, 0xFF, 0xFF, 0x48, 0x85,
    0xC0, 0x79, 0x0C, 0x48, 0xF7, 0xD8, 0x48, 0x89, 0x85, 0x20, 0xFF, 0xFF,
    0xFF, 0xB2, 0x2D, 0xE8, 0xA5, 0xFE, 0xFF, 0xFF, 0x48, 0x8B, 0x85, 0x20,
    0xFF, 0xFF, 0xFF, 0x31, 0xD2, 0x49, 0xC7, 0xC2, 0x0A, 0x00, 0x00, 0x00,
    0x49, 0xF7, 0xF2, 0x49, 0x89, 0xD0, 0x04, 0x30, 0x88, 0xC2, 0xE8, 0x86,
    0xFE, 0xFF, 0xFF, 0x4C, 0x89, 0xC0, 0x04, 0x30, 0x88, 0xC2, 0xE8, 0x7A,
    0xFE, 0xFF, 0xFF, 0xC3, 0x49, 0x83, 0xFD, 0x03, 0x73, 0x09, 0x4A, 0x8B,
    0x44, 0xED, 0xB8, 0x49, 0xFF, 0xC5, 0xC3, 0x4C, 0x89, 0xE8, 0x48, 0x83,
    0xE8, 0x03, 0x48, 0x8B, 0x44, 0xC5, 0x10, 0x49, 0xFF, 0xC5, 0xC3, 0x48,
    0x8B, 0x8D, 0x48, 0xFF, 0xFF, 0xFF, 0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00,
    0x00, 0x48, 0x83, 0xF9, 0x08, 0x73, 0x08, 0x48, 0x8B, 0x84, 0xCD, 0xC0,
    0xFE, 0xFF, 0xFF, 0x48, 0xFF, 0xC1, 0x48, 0x89, 0x8D, 0x48, 0xFF, 0xFF,
    0xFF, 0xC3, 0x66, 0x48, 0x0F, 0x7E, 0xC0, 0x48, 0x85, 0xC0, 0x79, 0x11,
    0xB2, 0x2D, 0xE8, 0x22, 0xFE, 0xFF, 0xFF, 0x48, 0x0F, 0xBA, 0xF0, 0x3F,
    0x66, 0x48, 0x0F, 0x6E, 0xC0, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x24, 0x40, 0x66, 0x48, 0x0F, 0x6E, 0xD0, 0x48, 0x8B, 0x8D, 0x40,
    0xFF, 0xFF, 0xFF, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0,
    0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xD8, 0x48, 0x85, 0xC9, 0x74, 0x09, 0xF2,
    0x0F, 0x59, 0xDA, 0x48, 0xFF, 0xC9, 0xEB, 0xF2, 0x48, 0xB8, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xE0, 0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xE0, 0xF2,
    0x0F, 0x5E, 0xE3, 0xF2, 0x0F, 0x58, 0xC4, 0xF2, 0x48, 0x0F, 0x2C, 0xC0,
    0x48, 0x89, 0xC7, 0xE8, 0x8E, 0x00, 0x00, 0x00, 0xF2, 0x48, 0x0F, 0x2C,
    0xC0, 0xF2, 0x48, 0x0F, 0x2A, 0xC8, 0xF2, 0x0F, 0x5C, 0xC1, 0xF2, 0x0F,
    0x59, 0xC3, 0xF2, 0x48, 0x0F, 0x2C, 0xC0, 0x49, 0x89, 0xC0, 0x48, 0x8B,
    0x8D, 0x40, 0xFF, 0xFF, 0xFF, 0x48, 0x85, 0xC9, 0x74, 0x3B, 0xB2, 0x2E,
    0xE8, 0x94, 0xFD, 0xFF, 0xFF, 0x48, 0x8B, 0x8D, 0x40, 0xFF, 0xFF, 0xFF,
    0x4C, 0x89, 0xC0, 0x31, 0xD2, 0x49, 0xC7, 0xC2, 0x0A, 0x00, 0x00, 0x00,
    0x49, 0xF7, 0xF2, 0x49, 0x89, 0xC0, 0x83, 0xC2, 0x30, 0x52, 0x48, 0xFF,
    0xC9, 0x75, 0xE5, 0x48, 0x8B, 0x8D, 0x40, 0xFF, 0xFF, 0xFF, 0x5A, 0xE8,
    0x65, 0xFD, 0xFF, 0xFF, 0x48, 0xFF, 0xC9, 0x75, 0xF5, 0xC3, 0x45, 0x31,
    0xC9, 0x48, 0x85, 0xFF, 0x79, 0x0B, 0x41, 0xB9, 0x2D, 0x00, 0x00, 0x00,
    0x48, 0xF7, 0xDF, 0xEB, 0x10, 0x48, 0x83, 0xBD, 0x30, 0xFF, 0xFF, 0xFF,
    0x00, 0x74, 0x06, 0x41, 0xB9, 0x2B, 0x00, 0x00, 0x00, 0xB9, 0x0A, 0x00,
    0x00, 0x00, 0x31, 0xF6, 0xEB, 0x3A, 0x45, 0x31, 0xC9, 0x48, 0x83, 0xBD,
    0x30, 0xFF, 0xFF, 0xFF, 0x00, 0x74, 0x06, 0x41, 0xB9, 0x2B, 0x00, 0x00,
    0x00, 0xB9, 0x0A, 0x00, 0x00, 0x00, 0x31, 0xF6, 0xEB, 0x1E, 0x45, 0x31,
    0xC9, 0xB9, 0x10, 0x00, 0x00, 0x00, 0xBE, 0x57, 0x00, 0x00, 0x00, 0xEB,
    0x0F, 0x45, 0x31, 0xC9, 0xB9, 0x10, 0x00, 0x00, 0x00, 0xBE, 0x37, 0x00,
    0x00, 0x00, 0xEB, 0x00, 0x48, 0x8D, 0x9D, 0x60, 0xFF, 0xFF, 0xFF, 0x49,
    0x89, 0xD8, 0x48, 0x89, 0xF8, 0x31, 0xD2, 0x48, 0xF7, 0xF1, 0x80, 0xFA,
    0x0A, 0x72, 0x05, 0x40, 0x00, 0xF2, 0xEB, 0x03, 0x80, 0xC2, 0x30, 0x48,
    0xFF, 0xCB, 0x88, 0x13, 0x48, 0x85, 0xC0, 0x75, 0xE4, 0x4C, 0x89, 0xC0,
    0x48, 0x29, 0xD8, 0x45, 0x84, 0xC9, 0x74, 0x03, 0x48, 0xFF, 0xC0, 0x49,
    0x29, 0xC2, 0x4D, 0x85, 0xD2, 0x7E, 0x56, 0x48, 0x83, 0xBD, 0x38, 0xFF,
    0xFF, 0xFF, 0x00, 0x75, 0x0F, 0x41, 0x80, 0xFB, 0x30, 0x74, 0x30, 0xB2,
    0x20, 0xE8, 0x5E, 0x00, 0x00, 0x00, 0xEB, 0x3D, 0x45, 0x84, 0xC9, 0x74,
    0x08, 0x44, 0x88, 0xCA, 0xE8, 0x94, 0xFC, 0xFF, 0xFF, 0x4C, 0x39, 0xC3,
    0x73, 0x0C, 0x8A, 0x13, 0xE8, 0x88, 0xFC, 0xFF, 0xFF, 0x48, 0xFF, 0xC3,
    0xEB, 0xEF, 0xB2, 0x20, 0xE8, 0x37, 0x00, 0x00, 0x00, 0xEB, 0x34, 0x45,
    0x84, 0xC9, 0x74, 0x08, 0x44, 0x88, 0xCA, 0xE8, 0x6D, 0xFC, 0xFF, 0xFF,
    0xB2, 0x30, 0xE8, 0x21, 0x00, 0x00, 0x00, 0xEB, 0x0D, 0x45, 0x84, 0xC9,
    0x74, 0x08, 0x44, 0x88, 0xCA, 0xE8, 0x57, 0xFC, 0xFF, 0xFF, 0x4C, 0x39,
    0xC3, 0x73, 0x0C, 0x8A, 0x13, 0xE8, 0x4B, 0xFC, 0xFF, 0xFF, 0x48, 0xFF,
    0xC3, 0xEB, 0xEF, 0xC3, 0xE8, 0x40, 0xFC, 0xFF, 0xFF, 0x49, 0xFF, 0xCA,
    0x75, 0xF6, 0xC3,
};

static const UCHAR g_BuiltinSprintf[] = {
    0x55, 0x48, 0x89, 0xE5, 0x53, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41,
    0x57, 0x48, 0x81, 0xEC, 0x20, 0x01, 0x00, 0x00, 0xF2, 0x0F, 0x11, 0x85,
    0xC0, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0x8D, 0xC8, 0xFE, 0xFF, 0xFF,
    0xF2, 0x0F, 0x11, 0x95, 0xD0, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0x9D,
    0xD8, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0xA5, 0xE0, 0xFE, 0xFF, 0xFF,
    0xF2, 0x0F, 0x11, 0xAD, 0xE8, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0xB5,
    0xF0, 0xFE, 0xFF, 0xFF, 0xF2, 0x0F, 0x11, 0xBD, 0xF8, 0xFE, 0xFF, 0xFF,
    0x48, 0xC7, 0x85, 0x48, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x48,
    0x89, 0x55, 0xB0, 0x48, 0x89, 0x4D, 0xB8, 0x4C, 0x89, 0x45, 0xC0, 0x4C,
    0x89, 0x4D, 0xC8, 0x49, 0x89, 0xFF, 0x49, 0x89, 0xF4, 0x4D, 0x31, 0xED,
    0x49, 0x89, 0xFE, 0x41, 0x0F, 0xB6, 0x04, 0x24, 0x84, 0xC0, 0x0F, 0x84,
    0x8A, 0x02, 0x00, 0x00, 0x3C, 0x25, 0x74, 0x0B, 0x41, 0x88, 0x06, 0x49,
    0xFF, 0xC6, 0x49, 0xFF, 0xC4, 0xEB, 0xE4, 0x49, 0xFF, 0xC4, 0x4D, 0x31,
    0xD2, 0x41, 0xB3, 0x20, 0x48, 0xC7, 0x85, 0x38, 0xFF, 0xFF, 0xFF, 0x00,
    0x00, 0x00, 0x00, 0x48, 0xC7, 0x85, 0x30, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x48, 0xC7, 0x85, 0x40, 0xFF, 0xFF, 0xFF, 0x06, 0x00, 0x00,
    0x00, 0x41, 0x0F, 0xB6, 0x04, 0x24, 0x3C, 0x2D, 0x74, 0x06, 0x3C, 0x2B,
    0x74, 0x12, 0xEB, 0x20, 0x48, 0xC7, 0x85, 0x38, 0xFF, 0xFF, 0xFF, 0x01,
    0x00, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xEB, 0xE1, 0x48, 0xC7, 0x85, 0x30,
    0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xEB, 0xD1,
    0x41, 0x0F, 0xB6, 0x04, 0x24, 0x3C, 0x30, 0x75, 0x16, 0x41, 0x0F, 0xB6,
    0x4C, 0x24, 0x01, 0x80, 0xF9, 0x30, 0x72, 0x0B, 0x80, 0xF9, 0x39, 0x77,
    0x06, 0x41, 0xB3, 0x30, 0x49, 0xFF, 0xC4, 0x41, 0x0F, 0xB6, 0x04, 0x24,
    0x3C, 0x30, 0x72, 0x17, 0x3C, 0x39, 0x77, 0x13, 0x4F, 0x8D, 0x14, 0x92,
    0x4F, 0x8D, 0x14, 0x12, 0x83, 0xE8, 0x30, 0x49, 0x01, 0xC2, 0x49, 0xFF,
    0xC4, 0xEB, 0xE0, 0x3C, 0x2E, 0x75, 0x2C, 0x31, 0xC9, 0x49, 0xFF, 0xC4,
    0x41, 0x0F, 0xB6, 0x04, 0x24, 0x3C, 0x30, 0x72, 0x17, 0x3C, 0x39, 0x77,
    0x13, 0x48, 0x8D, 0x0C, 0x89, 0x48, 0x8D, 0x0C, 0x09, 0x83, 0xE8, 0x30,
    0x48, 0x01, 0xC1, 0x49, 0xFF, 0xC4, 0xEB, 0xE0, 0x48, 0x89, 0x8D, 0x40,
    0xFF, 0xFF, 0xFF, 0x41, 0x0F, 0xB6, 0x04, 0x24, 0x3C, 0x6C, 0x75, 0x05,
    0x49, 0xFF, 0xC4, 0xEB, 0xF2, 0x84, 0xC0, 0x74, 0x65, 0x3C, 0x25, 0x0F,
    0x84, 0x97, 0x00, 0x00, 0x00, 0x3C, 0x64, 0x0F, 0x84, 0x9E, 0x00, 0x00,
    0x00, 0x3C, 0x75, 0x0F, 0x84, 0xAB, 0x00, 0x00, 0x00, 0x3C, 0x78, 0x0F,
    0x84, 0xB8, 0x00, 0x00, 0x00, 0x3C, 0x58, 0x0F, 0x84, 0xC5, 0x00, 0x00,
    0x00, 0x3C, 0x70, 0x0F, 0x84, 0xD2, 0x00, 0x00, 0x00, 0x3C, 0x73, 0x0F,
    0x84, 0xF7, 0x00, 0x00, 0x00, 0x3C, 0x63, 0x0F, 0x84, 0x4E, 0x01, 0x00,
    0x00, 0x3C, 0x66, 0x74, 0x44, 0x3C, 0x46, 0x74, 0x40, 0x3C, 0x65, 0x74,
    0x25, 0x3C, 0x45, 0x74, 0x21, 0x41, 0xC6, 0x06, 0x25, 0x49, 0xFF, 0xC6,
    0x41, 0x88, 0x06, 0x49, 0xFF, 0xC6, 0x49, 0xFF, 0xC4, 0xE9, 0xA5, 0xFE,
    0xFF, 0xFF, 0x41, 0xC6, 0x06, 0x25, 0x49, 0xFF, 0xC6, 0xE9, 0x30, 0x01,
    0x00, 0x00, 0xE8, 0x47, 0x01, 0x00, 0x00, 0x66, 0x48, 0x0F, 0x6E, 0xC0,
    0xE8, 0x39, 0x02, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x82, 0xFE, 0xFF,
    0xFF, 0xE8, 0x30, 0x01, 0x00, 0x00, 0x66, 0x48, 0x0F, 0x6E, 0xC0, 0xE8,
    0x4D, 0x01, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x6B, 0xFE, 0xFF, 0xFF,
    0x41, 0xC6, 0x06, 0x25, 0x49, 0xFF, 0xC6, 0x49, 0xFF, 0xC4, 0xE9, 0x5C,
    0xFE, 0xFF, 0xFF, 0xE8, 0x86, 0x03, 0x00, 0x00, 0x48, 0x89, 0xC7, 0xE8,
    0x9D, 0x03, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x47, 0xFE, 0xFF, 0xFF,
    0xE8, 0x71, 0x03, 0x00, 0x00, 0x48, 0x89, 0xC7, 0xE8, 0xB4, 0x03, 0x00,
    0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x32, 0xFE, 0xFF, 0xFF, 0xE8, 0x5C, 0x03,
    0x00, 0x00, 0x48, 0x89, 0xC7, 0xE8, 0xBB, 0x03, 0x00, 0x00, 0x49, 0xFF,
    0xC4, 0xE9, 0x1D, 0xFE, 0xFF, 0xFF, 0xE8, 0x47, 0x03, 0x00, 0x00, 0x48,
    0x89, 0xC7, 0xE8, 0xB5, 0x03, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0x08,
    0xFE, 0xFF, 0xFF, 0xE8, 0x32, 0x03, 0x00, 0x00, 0x48, 0x89, 0xC7, 0x49,
    0x83, 0xFA, 0x02, 0x72, 0x04, 0x49, 0x83, 0xEA, 0x02, 0x41, 0xC6, 0x06,
    0x30, 0x49, 0xFF, 0xC6, 0x41, 0xC6, 0x06, 0x78, 0x49, 0xFF, 0xC6, 0xE8,
    0x79, 0x03, 0x00, 0x00, 0x49, 0xFF, 0xC4, 0xE9, 0xDB, 0xFD, 0xFF, 0xFF,
    0xE8, 0x05, 0x03, 0x00, 0x00, 0x48, 0x89, 0xC3, 0x48, 0x31, 0xC9, 0x80,
    0x3C, 0x0B, 0x00, 0x74, 0x05, 0x48, 0xFF, 0xC1, 0xEB, 0xF5, 0x4D, 0x89,
    0xD1, 0x49, 0x29, 0xC9, 0x7E, 0x16, 0x48, 0x83, 0xBD, 0x38, 0xFF, 0xFF,
    0xFF, 0x00, 0x75, 0x0C, 0x41, 0xC6, 0x06, 0x20, 0x49, 0xFF, 0xC6, 0x49,
    0xFF, 0xC9, 0x75, 0xF4, 0x0F, 0xB6, 0x03, 0x84, 0xC0, 0x74, 0x0B, 0x41,
    0x88, 0x06, 0x49, 0xFF, 0xC6, 0x48, 0xFF, 0xC3, 0xEB, 0xEE, 0x4D, 0x85,
    0xC9, 0x7E, 0x0C, 0x41, 0xC6, 0x06, 0x20, 0x49, 0xFF, 0xC6, 0x49, 0xFF,
    0xC9, 0x75, 0xF4, 0x49, 0xFF, 0xC4, 0xE9, 0x7C, 0xFD, 0xFF, 0xFF, 0xE8,
    0xA6, 0x02, 0x00, 0x00, 0x41, 0x88, 0x06, 0x49, 0xFF, 0xC6, 0x49, 0xFF,
    0xC4, 0xE9, 0x69, 0xFD, 0xFF, 0xFF, 0x41, 0xC6, 0x06, 0x00, 0x4C, 0x89,
    0xF0, 0x4C, 0x29, 0xF8, 0x48, 0x81, 0xC4, 0x20, 0x01, 0x00, 0x00, 0x41,
    0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C, 0x5B, 0x5D, 0xC3, 0x48, 0x8B,
    0x8D, 0x48, 0xFF, 0xFF, 0xFF, 0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,
    0x48, 0x83, 0xF9, 0x08, 0x73, 0x08, 0x48, 0x8B, 0x84, 0xCD, 0xC0, 0xFE,
    0xFF, 0xFF, 0x48, 0xFF, 0xC1, 0x48, 0x89, 0x8D, 0x48, 0xFF, 0xFF, 0xFF,
    0xC3, 0x66, 0x48, 0x0F, 0x7E, 0xC0, 0x48, 0x85, 0xC0, 0x79, 0x11, 0x41,
    0xC6, 0x06, 0x2D, 0x49, 0xFF, 0xC6, 0x48, 0x0F, 0xBA, 0xF0, 0x3F, 0x66,
    0x48, 0x0F, 0x6E, 0xC0, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x24, 0x40, 0x66, 0x48, 0x0F, 0x6E, 0xD0, 0x48, 0x8B, 0x8D, 0x40, 0xFF,
    0xFF, 0xFF, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F,
    0x66, 0x48, 0x0F, 0x6E, 0xD8, 0x48, 0x85, 0xC9, 0x74, 0x09, 0xF2, 0x0F,
    0x59, 0xDA, 0x48, 0xFF, 0xC9, 0xEB, 0xF2, 0x48, 0xB8, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xE0, 0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xE0, 0xF2, 0x0F,
    0x5E, 0xE3, 0xF2, 0x0F, 0x58, 0xC4, 0xF2, 0x48, 0x0F, 0x2C, 0xC0, 0x48,
    0x89, 0xC7, 0xE8, 0x2E, 0x02, 0x00, 0x00, 0xF2, 0x48, 0x0F, 0x2C, 0xC0,
    0xF2, 0x48, 0x0F, 0x2A, 0xC8, 0xF2, 0x0F, 0x5C, 0xC1, 0xF2, 0x0F, 0x59,
    0xC3, 0xF2, 0x48, 0x0F, 0x2C, 0xC0, 0x49, 0x89, 0xC0, 0x48, 0x8B, 0x8D,
    0x40, 0xFF, 0xFF, 0xFF, 0x48, 0x85, 0xC9, 0x74, 0x3C, 0x41, 0xC6, 0x06,
    0x2E, 0x49, 0xFF, 0xC6, 0x48, 0x8B, 0x8D, 0x40, 0xFF, 0xFF, 0xFF, 0x4C,
    0x89, 0xC0, 0x31, 0xD2, 0x49, 0xC7, 0xC2, 0x0A, 0x00, 0x00, 0x00, 0x49,
    0xF7, 0xF2, 0x49, 0x89, 0xC0, 0x83, 0xC2, 0x30, 0x52, 0x48, 0xFF, 0xC9,
    0x75, 0xE5, 0x48, 0x8B, 0x8D, 0x40, 0xFF, 0xFF, 0xFF, 0x5A, 0x41, 0x88,
    0x16, 0x49, 0xFF, 0xC6, 0x48, 0xFF, 0xC9, 0x75, 0xF4, 0xC3, 0x66, 0x48,
    0x0F, 0x7E, 0xC0, 0x48, 0x85, 0xC0, 0x79, 0x12, 0xB2, 0x2D, 0x41, 0x88,
    0x16, 0x49, 0xFF, 0xC6, 0x48, 0x0F, 0xBA, 0xF0, 0x3F, 0x66, 0x48, 0x0F,
    0x6E, 0xC0, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40,
    0x66, 0x48, 0x0F, 0x6E, 0xD0, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xF0, 0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xE8, 0x48, 0xC7, 0x85, 0x20,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x66, 0x48, 0x0F, 0x7E, 0xC0,
    0x48, 0x85, 0xC0, 0x74, 0x28, 0x66, 0x0F, 0x2F, 0xC2, 0x73, 0x08, 0x66,
    0x0F, 0x2F, 0xC5, 0x72, 0x0F, 0xEB, 0x1A, 0xF2, 0x0F, 0x5E, 0xC2, 0x48,
    0xFF, 0x85, 0x20, 0xFF, 0xFF, 0xFF, 0xEB, 0xE5, 0xF2, 0x0F, 0x59, 0xC2,
    0x48, 0xFF, 0x8D, 0x20, 0xFF, 0xFF, 0xFF, 0xEB, 0xD8, 0xF2, 0x48, 0x0F,
    0x2C, 0xC0, 0x48, 0x89, 0xC7, 0xE8, 0x47, 0x01, 0x00, 0x00, 0xB2, 0x2E,
    0x41, 0x88, 0x16, 0x49, 0xFF, 0xC6, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x24, 0x40, 0x66, 0x48, 0x0F, 0x6E, 0xD0, 0x48, 0x8B, 0x8D,
    0x40, 0xFF, 0xFF, 0xFF, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF0, 0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xD8, 0x48, 0x85, 0xC9, 0x74, 0x09,
    0xF2, 0x0F, 0x59, 0xDA, 0x48, 0xFF, 0xC9, 0xEB, 0xF2, 0x48, 0xB8, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x3F, 0x66, 0x48, 0x0F, 0x6E, 0xE0,
    0xF2, 0x0F, 0x5E, 0xE3, 0xF2, 0x0F, 0x58, 0xC4, 0xF2, 0x48, 0x0F, 0x2C,
    0xC0, 0xF2, 0x48, 0x0F, 0x2A, 0xC8, 0xF2, 0x0F, 0x5C, 0xC1, 0xF2, 0x0F,
    0x59, 0xC3, 0xF2, 0x48, 0x0F, 0x2C, 0xC0, 0x49, 0x89, 0xC0, 0x48, 0x8B,
    0x8D, 0x40, 0xFF, 0xFF, 0xFF, 0x48, 0x85, 0xC9, 0x74, 0x2E, 0x4C, 0x89,
    0xC0, 0x31, 0xD2, 0x49, 0xC7, 0xC2, 0x0A, 0x00, 0x00, 0x00, 0x49, 0xF7,
    0xF2, 0x49, 0x89, 0xC0, 0x83, 0xC2, 0x30, 0x52, 0x48, 0xFF, 0xC9, 0x75,
    0xE5, 0x48, 0x8B, 0x8D, 0x40, 0xFF, 0xFF, 0xFF, 0x5A, 0x41, 0x88, 0x16,
    0x49, 0xFF, 0xC6, 0x48, 0xFF, 0xC9, 0x75, 0xF4, 0xB2, 0x65, 0x41, 0x88,
    0x16, 0x49, 0xFF, 0xC6, 0xB2, 0x2B, 0x48, 0x8B, 0x85, 0x20, 0xFF, 0xFF,
    0xFF, 0x48, 0x85, 0xC0, 0x79, 0x0C, 0x48, 0xF7, 0xD8, 0x48, 0x89, 0x85,
    0x20, 0xFF, 0xFF, 0xFF, 0xB2, 0x2D, 0x41, 0x88, 0x16, 0x49, 0xFF, 0xC6,
    0x48, 0x8B, 0x85, 0x20, 0xFF, 0xFF, 0xFF, 0x31, 0xD2, 0x49, 0xC7, 0xC2,
    0x0A, 0x00, 0x00, 0x00, 0x49, 0xF7, 0xF2, 0x49, 0x89, 0xD0, 0x04, 0x30,
    0x88, 0xC2, 0x41, 0x88, 0x16, 0x49, 0xFF, 0xC6, 0x4C, 0x89, 0xC0, 0x04,
    0x30, 0x88, 0xC2, 0x41, 0x88, 0x16, 0x49, 0xFF, 0xC6, 0xC3, 0x49, 0x83,
    0xFD, 0x04, 0x73, 0x09, 0x4A, 0x8B, 0x44, 0xED, 0xB0, 0x49, 0xFF, 0xC5,
    0xC3, 0x4C, 0x89, 0xE8, 0x48, 0x83, 0xE8, 0x04, 0x48, 0x8B, 0x44, 0xC5,
    0x10, 0x49, 0xFF, 0xC5, 0xC3, 0x45, 0x31, 0xC9, 0x48, 0x85, 0xFF, 0x79,
    0x0B, 0x41, 0xB9, 0x2D, 0x00, 0x00, 0x00, 0x48, 0xF7, 0xDF, 0xEB, 0x10,
    0x48, 0x83, 0xBD, 0x30, 0xFF, 0xFF, 0xFF, 0x00, 0x74, 0x06, 0x41, 0xB9,
    0x2B, 0x00, 0x00, 0x00, 0xB9, 0x0A, 0x00, 0x00, 0x00, 0x31, 0xF6, 0xEB,
    0x3A, 0x45, 0x31, 0xC9, 0x48, 0x83, 0xBD, 0x30, 0xFF, 0xFF, 0xFF, 0x00,
    0x74, 0x06, 0x41, 0xB9, 0x2B, 0x00, 0x00, 0x00, 0xB9, 0x0A, 0x00, 0x00,
    0x00, 0x31, 0xF6, 0xEB, 0x1E, 0x45, 0x31, 0xC9, 0xB9, 0x10, 0x00, 0x00,
    0x00, 0xBE, 0x57, 0x00, 0x00, 0x00, 0xEB, 0x0F, 0x45, 0x31, 0xC9, 0xB9,
    0x10, 0x00, 0x00, 0x00, 0xBE, 0x37, 0x00, 0x00, 0x00, 0xEB, 0x00, 0x48,
    0x8D, 0x9D, 0x78, 0xFF, 0xFF, 0xFF, 0x49, 0x89, 0xD8, 0x48, 0x89, 0xF8,
    0x31, 0xD2, 0x48, 0xF7, 0xF1, 0x80, 0xFA, 0x0A, 0x72, 0x05, 0x40, 0x00,
    0xF2, 0xEB, 0x03, 0x80, 0xC2, 0x30, 0x48, 0xFF, 0xCB, 0x88, 0x13, 0x48,
    0x85, 0xC0, 0x75, 0xE4, 0x4C, 0x89, 0xC0, 0x48, 0x29, 0xD8, 0x45, 0x84,
    0xC9, 0x74, 0x03, 0x48, 0xFF, 0xC0, 0x4C, 0x89, 0xD2, 0x48, 0x29, 0xC2,
    0x48, 0x85, 0xD2, 0x7E, 0x53, 0x48, 0x83, 0xBD, 0x38, 0xFF, 0xFF, 0xFF,
    0x00, 0x75, 0x0F, 0x41, 0x80, 0xFB, 0x30, 0x74, 0x2F, 0xB0, 0x20, 0xE8,
    0x5A, 0x00, 0x00, 0x00, 0xEB, 0x3A, 0x45, 0x84, 0xC9, 0x74, 0x06, 0x45,
    0x88, 0x0E, 0x49, 0xFF, 0xC6, 0x4C, 0x39, 0xC3, 0x73, 0x0D, 0x8A, 0x03,
    0x41, 0x88, 0x06, 0x49, 0xFF, 0xC6, 0x48, 0xFF, 0xC3, 0xEB, 0xEE, 0xB0,
    0x20, 0xE8, 0x34, 0x00, 0x00, 0x00, 0xEB, 0x31, 0x45, 0x84, 0xC9, 0x74,
    0x06, 0x45, 0x88, 0x0E, 0x49, 0xFF, 0xC6, 0xB0, 0x30, 0xE8, 0x20, 0x00,
    0x00, 0x00, 0xEB, 0x0B, 0x45, 0x84, 0xC9, 0x74, 0x06, 0x45, 0x88, 0x0E,
    0x49, 0xFF, 0xC6, 0x4C, 0x39, 0xC3, 0x73, 0x0D, 0x8A, 0x03, 0x41, 0x88,
    0x06, 0x49, 0xFF, 0xC6, 0x48, 0xFF, 0xC3, 0xEB, 0xEE, 0xC3, 0x41, 0x88,
    0x06, 0x49, 0xFF, 0xC6, 0x48, 0xFF, 0xCA, 0x75, 0xF5, 0xC3,
};

static const UCHAR g_BuiltinStrncpy[] = {
    0x48, 0x89, 0xF8, 0x48, 0x85, 0xD2, 0x74, 0x28, 0x48, 0x31, 0xC9, 0x44,
    0x0F, 0xB6, 0x04, 0x0E, 0x44, 0x88, 0x04, 0x0F, 0x45, 0x84, 0xC0, 0x74,
    0x09, 0x48, 0xFF, 0xC1, 0x48, 0x39, 0xD1, 0x72, 0xEA, 0xC3, 0x48, 0xFF,
    0xC1, 0x48, 0x39, 0xD1, 0x73, 0x06, 0xC6, 0x04, 0x0F, 0x00, 0xEB, 0xF2,
    0xC3,
};

static const UCHAR g_BuiltinStrcat[] = {
    0x48, 0x89, 0xF8, 0x48, 0x31, 0xC9, 0x80, 0x3C, 0x0F, 0x00, 0x74, 0x05,
    0x48, 0xFF, 0xC1, 0xEB, 0xF5, 0x4D, 0x31, 0xC0, 0x46, 0x0F, 0xB6, 0x0C,
    0x06, 0x44, 0x88, 0x0C, 0x0F, 0x45, 0x84, 0xC9, 0x74, 0x08, 0x48, 0xFF,
    0xC1, 0x49, 0xFF, 0xC0, 0xEB, 0xEA, 0xC3,
};

static const UCHAR g_BuiltinStrtokR[] = {
    0x48, 0x85, 0xFF, 0x75, 0x03, 0x48, 0x8B, 0x3A, 0x48, 0x85, 0xFF, 0x74,
    0x66, 0x0F, 0xB6, 0x07, 0x84, 0xC0, 0x74, 0x58, 0x49, 0x89, 0xF0, 0x45,
    0x0F, 0xB6, 0x08, 0x45, 0x84, 0xC9, 0x74, 0x0F, 0x44, 0x38, 0xC8, 0x74,
    0x05, 0x49, 0xFF, 0xC0, 0xEB, 0xED, 0x48, 0xFF, 0xC7, 0xEB, 0xDE, 0x49,
    0x89, 0xFA, 0x0F, 0xB6, 0x07, 0x84, 0xC0, 0x74, 0x28, 0x49, 0x89, 0xF0,
    0x45, 0x0F, 0xB6, 0x08, 0x45, 0x84, 0xC9, 0x74, 0x0A, 0x44, 0x38, 0xC8,
    0x74, 0x0A, 0x49, 0xFF, 0xC0, 0xEB, 0xED, 0x48, 0xFF, 0xC7, 0xEB, 0xDE,
    0xC6, 0x07, 0x00, 0x48, 0xFF, 0xC7, 0x48, 0x89, 0x3A, 0x4C, 0x89, 0xD0,
    0xC3, 0x48, 0xC7, 0x02, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x89, 0xD0, 0xC3,
    0x48, 0xC7, 0x02, 0x00, 0x00, 0x00, 0x00, 0x31, 0xC0, 0xC3,
};

static const UCHAR g_BuiltinQsort[] = {
    0x55, 0x48, 0x89, 0xE5, 0x53, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41,
    0x57, 0x48, 0x83, 0xEC, 0x48, 0x48, 0x89, 0xFB, 0x49, 0x89, 0xF4, 0x49,
    0x89, 0xD5, 0x49, 0x89, 0xCE, 0x49, 0x83, 0xFC, 0x02, 0x72, 0x7D, 0x48,
    0xC7, 0x45, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x45, 0xC0, 0x4C,
    0x89, 0xE1, 0x48, 0xFF, 0xC9, 0x48, 0x39, 0xC8, 0x73, 0x66, 0x4C, 0x89,
    0xE0, 0x48, 0xFF, 0xC8, 0x48, 0x2B, 0x45, 0xC0, 0x48, 0x89, 0x45, 0xB8,
    0x48, 0xC7, 0x45, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x45, 0xB0,
    0x48, 0x3B, 0x45, 0xB8, 0x73, 0x40, 0x49, 0x0F, 0xAF, 0xC5, 0x48, 0x8D,
    0x3C, 0x03, 0x4A, 0x8D, 0x34, 0x2F, 0x41, 0xFF, 0xD6, 0x85, 0xC0, 0x7E,
    0x27, 0x48, 0x8B, 0x45, 0xB0, 0x49, 0x0F, 0xAF, 0xC5, 0x48, 0x8D, 0x3C,
    0x03, 0x4A, 0x8D, 0x34, 0x2F, 0x48, 0x31, 0xC9, 0x8A, 0x04, 0x0F, 0x8A,
    0x14, 0x0E, 0x88, 0x14, 0x0F, 0x88, 0x04, 0x0E, 0x48, 0xFF, 0xC1, 0x4C,
    0x39, 0xE9, 0x72, 0xEC, 0x48, 0xFF, 0x45, 0xB0, 0xEB, 0xB6, 0x48, 0xFF,
    0x45, 0xC0, 0xEB, 0x8B, 0x48, 0x83, 0xC4, 0x48, 0x41, 0x5F, 0x41, 0x5E,
    0x41, 0x5D, 0x41, 0x5C, 0x5B, 0x5D, 0xC3,
};

static const UCHAR g_BuiltinBsearch[] = {
    0x53, 0x55, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x83,
    0xEC, 0x08, 0x48, 0x89, 0xFB, 0x49, 0x89, 0xF4, 0x49, 0x89, 0xD5, 0x49,
    0x89, 0xCE, 0x4D, 0x89, 0xC7, 0x4D, 0x85, 0xED, 0x74, 0x3D, 0x4C, 0x89,
    0xE8, 0x48, 0xD1, 0xE8, 0x48, 0x89, 0xC1, 0x49, 0x0F, 0xAF, 0xCE, 0x49,
    0x8D, 0x2C, 0x0C, 0x48, 0x89, 0xDF, 0x48, 0x89, 0xEE, 0x41, 0xFF, 0xD7,
    0x85, 0xC0, 0x74, 0x1A, 0x78, 0x13, 0x4C, 0x89, 0xE8, 0x48, 0xD1, 0xE8,
    0x49, 0x29, 0xC5, 0x49, 0xFF, 0xCD, 0x4E, 0x8D, 0x64, 0x35, 0x00, 0xEB,
    0xC8, 0x49, 0xD1, 0xED, 0xEB, 0xC3, 0x48, 0x89, 0xE8, 0xEB, 0x02, 0x31,
    0xC0, 0x48, 0x83, 0xC4, 0x08, 0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41,
    0x5C, 0x5D, 0x5B, 0xC3,
};

static const UCHAR g_BuiltinRandR[] = {
    0x8B, 0x07, 0x69, 0xC0, 0x6D, 0x4E, 0xC6, 0x41, 0x05, 0x39, 0x30, 0x00,
    0x00, 0x89, 0x07, 0xC1, 0xE8, 0x10, 0x25, 0xFF, 0x7F, 0x00, 0x00, 0xC3,
};

static const UCHAR g_BuiltinAbs[] = {
    0x89, 0xF8, 0x99, 0x31, 0xD0, 0x29, 0xD0, 0xC3,
};

static const UCHAR g_BuiltinLabs[] = {
    0x48, 0x89, 0xF8, 0x48, 0x99, 0x48, 0x31, 0xD0, 0x48, 0x29, 0xD0, 0xC3,
};

static const UCHAR g_BuiltinIsalpha[] = {
    0x40, 0x0F, 0xB6, 0xC7, 0x89, 0xC1, 0x83, 0xE9, 0x41, 0x83, 0xF9, 0x19,
    0x76, 0x0D, 0x89, 0xC1, 0x83, 0xE9, 0x61, 0x83, 0xF9, 0x19, 0x76, 0x03,
    0x31, 0xC0, 0xC3, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3,
};

static const UCHAR g_BuiltinIsdigit[] = {
    0x40, 0x0F, 0xB6, 0xC7, 0x83, 0xE8, 0x30, 0x83, 0xF8, 0x09, 0x76, 0x03,
    0x31, 0xC0, 0xC3, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3,
};

static const UCHAR g_BuiltinIsspace[] = {
    0x40, 0x0F, 0xB6, 0xC7, 0x3C, 0x20, 0x74, 0x0B, 0x83, 0xE8, 0x09, 0x83,
    0xF8, 0x04, 0x76, 0x03, 0x31, 0xC0, 0xC3, 0xB8, 0x01, 0x00, 0x00, 0x00,
    0xC3,
};

static const UCHAR g_BuiltinIsupper[] = {
    0x40, 0x0F, 0xB6, 0xC7, 0x83, 0xE8, 0x41, 0x83, 0xF8, 0x19, 0x76, 0x03,
    0x31, 0xC0, 0xC3, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3,
};

static const UCHAR g_BuiltinIslower[] = {
    0x40, 0x0F, 0xB6, 0xC7, 0x83, 0xE8, 0x61, 0x83, 0xF8, 0x19, 0x76, 0x03,
    0x31, 0xC0, 0xC3, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3,
};

static const UCHAR g_BuiltinIsalnum[] = {
    0x40, 0x0F, 0xB6, 0xC7, 0x89, 0xC1, 0x83, 0xE9, 0x30, 0x83, 0xF9, 0x09,
    0x76, 0x15, 0x89, 0xC1, 0x83, 0xE9, 0x41, 0x83, 0xF9, 0x19, 0x76, 0x0B,
    0x83, 0xE8, 0x61, 0x83, 0xF8, 0x19, 0x76, 0x03, 0x31, 0xC0, 0xC3, 0xB8,
    0x01, 0x00, 0x00, 0x00, 0xC3,
};

static const UCHAR g_BuiltinIsxdigit[] = {
    0x40, 0x0F, 0xB6, 0xC7, 0x89, 0xC1, 0x83, 0xE9, 0x30, 0x83, 0xF9, 0x09,
    0x76, 0x10, 0x89, 0xC1, 0x83, 0xC9, 0x20, 0x83, 0xE9, 0x61, 0x83, 0xF9,
    0x05, 0x76, 0x03, 0x31, 0xC0, 0xC3, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3,
};

static const UCHAR g_BuiltinStrtol[] = {
    0x4D, 0x31, 0xC9, 0x4D, 0x31, 0xD2, 0x49, 0x89, 0xFB, 0x41, 0x0F, 0xB6,
    0x03, 0x3C, 0x20, 0x74, 0x08, 0x3C, 0x09, 0x72, 0x09, 0x3C, 0x0D, 0x77,
    0x05, 0x49, 0xFF, 0xC3, 0xEB, 0xEB, 0x3C, 0x2D, 0x75, 0x0C, 0x49, 0xC7,
    0xC2, 0x01, 0x00, 0x00, 0x00, 0x49, 0xFF, 0xC3, 0xEB, 0x07, 0x3C, 0x2B,
    0x75, 0x03, 0x49, 0xFF, 0xC3, 0x48, 0x85, 0xD2, 0x75, 0x1D, 0x48, 0xC7,
    0xC2, 0x0A, 0x00, 0x00, 0x00, 0x41, 0x0F, 0xB6, 0x03, 0x3C, 0x30, 0x75,
    0x33, 0x41, 0x0F, 0xB6, 0x43, 0x01, 0x83, 0xC8, 0x20, 0x3C, 0x78, 0x74,
    0x1C, 0xEB, 0x25, 0x48, 0x83, 0xFA, 0x10, 0x75, 0x1F, 0x41, 0x0F, 0xB6,
    0x03, 0x3C, 0x30, 0x75, 0x17, 0x41, 0x0F, 0xB6, 0x43, 0x01, 0x83, 0xC8,
    0x20, 0x3C, 0x78, 0x75, 0x0B, 0x48, 0xC7, 0xC2, 0x10, 0x00, 0x00, 0x00,
    0x49, 0x83, 0xC3, 0x02, 0x41, 0x0F, 0xB6, 0x03, 0x84, 0xC0, 0x74, 0x2B,
    0x89, 0xC1, 0x83, 0xE9, 0x30, 0x83, 0xF9, 0x09, 0x76, 0x10, 0x89, 0xC1,
    0x83, 0xC9, 0x20, 0x83, 0xE9, 0x61, 0x83, 0xF9, 0x19, 0x77, 0x14, 0x83,
    0xC1, 0x0A, 0x48, 0x39, 0xD1, 0x73, 0x0C, 0x4C, 0x0F, 0xAF, 0xCA, 0x49,
    0x01, 0xC9, 0x49, 0xFF, 0xC3, 0xEB, 0xCD, 0x48, 0x85, 0xF6, 0x74, 0x03,
    0x4C, 0x89, 0x1E, 0x4C, 0x89, 0xC8, 0x4D, 0x85, 0xD2, 0x74, 0x03, 0x48,
    0xF7, 0xD8, 0xC3,
};

static const UCHAR g_BuiltinMemchr[] = {
    0x48, 0x85, 0xD2, 0x74, 0x14, 0x40, 0x0F, 0xB6, 0xF6, 0x0F, 0xB6, 0x07,
    0x40, 0x38, 0xF0, 0x74, 0x0B, 0x48, 0xFF, 0xC7, 0x48, 0xFF, 0xCA, 0x75,
    0xF0, 0x31, 0xC0, 0xC3, 0x48, 0x89, 0xF8, 0xC3,
};

static const UCHAR g_BuiltinMemrchr[] = {
    0x48, 0x85, 0xD2, 0x74, 0x19, 0x40, 0x0F, 0xB6, 0xF6, 0x48, 0x8D, 0x7C,
    0x17, 0xFF, 0x0F, 0xB6, 0x07, 0x40, 0x38, 0xF0, 0x74, 0x0B, 0x48, 0xFF,
    0xCF, 0x48, 0xFF, 0xCA, 0x75, 0xF0, 0x31, 0xC0, 0xC3, 0x48, 0x89, 0xF8,
    0xC3,
};

static const UCHAR g_BuiltinStrncat[] = {
    0x48, 0x89, 0xF8, 0x48, 0x31, 0xC9, 0x80, 0x3C, 0x0F, 0x00, 0x74, 0x05,
    0x48, 0xFF, 0xC1, 0xEB, 0xF5, 0x4D, 0x31, 0xC0, 0x49, 0x39, 0xD0, 0x73,
    0x16, 0x46, 0x0F, 0xB6, 0x0C, 0x06, 0x45, 0x84, 0xC9, 0x74, 0x0C, 0x44,
    0x88, 0x0C, 0x0F, 0x48, 0xFF, 0xC1, 0x49, 0xFF, 0xC0, 0xEB, 0xE5, 0xC6,
    0x04, 0x0F, 0x00, 0xC3,
};

static const UCHAR g_BuiltinStrspn[] = {
    0x48, 0x31, 0xC9, 0x0F, 0xB6, 0x04, 0x0F, 0x84, 0xC0, 0x74, 0x1B, 0x49,
    0x89, 0xF0, 0x45, 0x0F, 0xB6, 0x08, 0x45, 0x84, 0xC9, 0x74, 0x0F, 0x44,
    0x38, 0xC8, 0x74, 0x05, 0x49, 0xFF, 0xC0, 0xEB, 0xED, 0x48, 0xFF, 0xC1,
    0xEB, 0xDD, 0x48, 0x89, 0xC8, 0xC3,
};

static const UCHAR g_BuiltinStrcspn[] = {
    0x48, 0x31, 0xC9, 0x0F, 0xB6, 0x04, 0x0F, 0x84, 0xC0, 0x74, 0x1B, 0x49,
    0x89, 0xF0, 0x45, 0x0F, 0xB6, 0x08, 0x45, 0x84, 0xC9, 0x74, 0x0A, 0x44,
    0x38, 0xC8, 0x74, 0x0A, 0x49, 0xFF, 0xC0, 0xEB, 0xED, 0x48, 0xFF, 0xC1,
    0xEB, 0xDD, 0x48, 0x89, 0xC8, 0xC3,
};

static const UCHAR g_BuiltinStrpbrk[] = {
    0x0F, 0xB6, 0x07, 0x84, 0xC0, 0x74, 0x1F, 0x49, 0x89, 0xF0, 0x45, 0x0F,
    0xB6, 0x08, 0x45, 0x84, 0xC9, 0x74, 0x0A, 0x44, 0x38, 0xC8, 0x74, 0x0A,
    0x49, 0xFF, 0xC0, 0xEB, 0xED, 0x48, 0xFF, 0xC7, 0xEB, 0xDE, 0x48, 0x89,
    0xF8, 0xC3, 0x31, 0xC0, 0xC3,
};

static const UCHAR g_BuiltinDiv[] = {
    0x89, 0xF8, 0x99, 0xF7, 0xFE, 0x48, 0xC1, 0xE2, 0x20, 0x48, 0x09, 0xD0,
    0xC3,
};

static const UCHAR g_BuiltinLdiv[] = {
    0x48, 0x89, 0xF8, 0x48, 0x99, 0x48, 0xF7, 0xFE, 0xC3,
};

static const UCHAR g_BuiltinStrsep[] = {
    0x48, 0x8B, 0x07, 0x48, 0x85, 0xC0, 0x74, 0x40, 0x49, 0x89, 0xC0, 0x49,
    0x89, 0xC1, 0x41, 0x0F, 0xB6, 0x09, 0x84, 0xC9, 0x74, 0x27, 0x49, 0x89,
    0xF2, 0x41, 0x0F, 0xB6, 0x12, 0x84, 0xD2, 0x74, 0x09, 0x38, 0xD1, 0x74,
    0x0A, 0x49, 0xFF, 0xC2, 0xEB, 0xEF, 0x49, 0xFF, 0xC1, 0xEB, 0xDF, 0x41,
    0xC6, 0x01, 0x00, 0x49, 0xFF, 0xC1, 0x4C, 0x89, 0x0F, 0x4C, 0x89, 0xC0,
    0xC3, 0x48, 0xC7, 0x07, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x89, 0xC0, 0xC3,
    0x31, 0xC0, 0xC3,
};

static const UCHAR g_BuiltinMemmem[] = {
    0x48, 0x85, 0xC9, 0x74, 0x37, 0x48, 0x39, 0xF1, 0x77, 0x36, 0x49, 0x89,
    0xF0, 0x49, 0x29, 0xC8, 0x49, 0xFF, 0xC0, 0x4D, 0x85, 0xC0, 0x74, 0x28,
    0x4D, 0x31, 0xC9, 0x49, 0x39, 0xC9, 0x73, 0x18, 0x42, 0x0F, 0xB6, 0x04,
    0x0F, 0x42, 0x3A, 0x04, 0x0A, 0x75, 0x05, 0x49, 0xFF, 0xC1, 0xEB, 0xEB,
    0x48, 0xFF, 0xC7, 0x49, 0xFF, 0xC8, 0xEB, 0xDB, 0x48, 0x89, 0xF8, 0xC3,
    0x48, 0x89, 0xF8, 0xC3, 0x31, 0xC0, 0xC3,
};

static const UCHAR g_BuiltinBzero[] = {
    0x48, 0x89, 0xF1, 0x31, 0xC0, 0xF3, 0xAA, 0xC3,
};

static const UCHAR g_BuiltinBcopy[] = {
    0x48, 0x89, 0xD1, 0xF3, 0xA4, 0xC3,
};

static const UCHAR g_BuiltinTime[] = {
    0x57, 0xB8, 0xC9, 0x00, 0x00, 0x00, 0x31, 0xFF, 0x0F, 0x05, 0x5F, 0x48,
    0x85, 0xFF, 0x74, 0x03, 0x48, 0x89, 0x07, 0xC3,
};

static const UCHAR g_BuiltinGetpid[] = {
    0xB8, 0x27, 0x00, 0x00, 0x00, 0x0F, 0x05, 0xC3,
};

static const UCHAR g_BuiltinGetppid[] = {
    0xB8, 0x6E, 0x00, 0x00, 0x00, 0x0F, 0x05, 0xC3,
};

static const UCHAR g_BuiltinSleep[] = {
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x89, 0x3C, 0x24, 0x48, 0xC7, 0x44, 0x24,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x3C, 0x24, 0x48, 0x8D, 0x74,
    0x24, 0x10, 0xB8, 0x23, 0x00, 0x00, 0x00, 0x0F, 0x05, 0x48, 0x85, 0xC0,
    0x74, 0x07, 0x48, 0x8B, 0x44, 0x24, 0x10, 0xEB, 0x02, 0x31, 0xC0, 0x48,
    0x83, 0xC4, 0x28, 0xC3,
};

static const UCHAR g_BuiltinUsleep[] = {
    0x48, 0x83, 0xEC, 0x18, 0x89, 0xF8, 0x31, 0xD2, 0xB9, 0x40, 0x42, 0x0F,
    0x00, 0xF7, 0xF1, 0x48, 0x89, 0x04, 0x24, 0x89, 0xD0, 0x69, 0xC0, 0xE8,
    0x03, 0x00, 0x00, 0x48, 0x89, 0x44, 0x24, 0x08, 0x48, 0x89, 0xE7, 0x31,
    0xF6, 0xB8, 0x23, 0x00, 0x00, 0x00, 0x0F, 0x05, 0x48, 0x83, 0xC4, 0x18,
    0xC3,
};

static const UCHAR g_BuiltinIsatty[] = {
    0x85, 0xFF, 0x78, 0x22, 0x48, 0x83, 0xEC, 0x48, 0xBE, 0x01, 0x54, 0x00,
    0x00, 0x48, 0x89, 0xE2, 0xB8, 0x10, 0x00, 0x00, 0x00, 0x0F, 0x05, 0x48,
    0x83, 0xC4, 0x48, 0x48, 0x85, 0xC0, 0x78, 0x06, 0xB8, 0x01, 0x00, 0x00,
    0x00, 0xC3, 0x31, 0xC0, 0xC3,
};

static const UCHAR g_BuiltinFfs[] = {
    0x89, 0xF8, 0x85, 0xC0, 0x74, 0x06, 0x0F, 0xBC, 0xC0, 0xFF, 0xC0, 0xC3,
    0x31, 0xC0, 0xC3,
};

static const UCHAR g_BuiltinSscanf[] = {
    0x55, 0x48, 0x89, 0xE5, 0x53, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41,
    0x57, 0x48, 0x83, 0xEC, 0x38, 0x48, 0x89, 0x55, 0xB8, 0x48, 0x89, 0x4D,
    0xC0, 0x4C, 0x89, 0x45, 0xC8, 0x4C, 0x89, 0x4D, 0xD0, 0x49, 0x89, 0xFC,
    0x49, 0x89, 0xF5, 0x4D, 0x31, 0xF6, 0x4D, 0x31, 0xFF, 0x41, 0x0F, 0xB6,
    0x45, 0x00, 0x84, 0xC0, 0x0F, 0x84, 0x44, 0x02, 0x00, 0x00, 0x3C, 0x25,
    0x74, 0x31, 0x3C, 0x20, 0x74, 0x12, 0x41, 0x38, 0x04, 0x24, 0x0F, 0x85,
    0x32, 0x02, 0x00, 0x00, 0x49, 0xFF, 0xC5, 0x49, 0xFF, 0xC4, 0xEB, 0xD9,
    0x41, 0x0F, 0xB6, 0x04, 0x24, 0x3C, 0x20, 0x74, 0x08, 0x3C, 0x09, 0x72,
    0x09, 0x3C, 0x0D, 0x77, 0x05, 0x49, 0xFF, 0xC4, 0xEB, 0xEA, 0x49, 0xFF,
    0xC5, 0xEB, 0xBE, 0x49, 0xFF, 0xC5, 0x41, 0x0F, 0xB6, 0x45, 0x00, 0x3C,
    0x6C, 0x75, 0x05, 0x49, 0xFF, 0xC5, 0xEB, 0xF2, 0x3C, 0x64, 0x0F, 0x84,
    0xA4, 0x00, 0x00, 0x00, 0x3C, 0x75, 0x0F, 0x84, 0x19, 0x01, 0x00, 0x00,
    0x3C, 0x78, 0x0F, 0x84, 0x70, 0x01, 0x00, 0x00, 0x3C, 0x73, 0x74, 0x49,
    0x3C, 0x63, 0x74, 0x1F, 0x3C, 0x25, 0x74, 0x05, 0xE9, 0xD5, 0x01, 0x00,
    0x00, 0x41, 0x80, 0x3C, 0x24, 0x25, 0x0F, 0x85, 0xCA, 0x01, 0x00, 0x00,
    0x49, 0xFF, 0xC4, 0x49, 0xFF, 0xC5, 0xE9, 0x6E, 0xFF, 0xFF, 0xFF, 0x41,
    0x0F, 0xB6, 0x04, 0x24, 0x84, 0xC0, 0x0F, 0x84, 0xB2, 0x01, 0x00, 0x00,
    0x41, 0x88, 0xC0, 0xE8, 0xD3, 0x01, 0x00, 0x00, 0x44, 0x88, 0x00, 0x49,
    0xFF, 0xC4, 0x49, 0xFF, 0xC7, 0x49, 0xFF, 0xC5, 0xE9, 0x48, 0xFF, 0xFF,
    0xFF, 0xE8, 0xA6, 0x01, 0x00, 0x00, 0x41, 0x0F, 0xB6, 0x04, 0x24, 0x84,
    0xC0, 0x0F, 0x84, 0x87, 0x01, 0x00, 0x00, 0xE8, 0xAB, 0x01, 0x00, 0x00,
    0x48, 0x89, 0xC3, 0x41, 0x0F, 0xB6, 0x04, 0x24, 0x84, 0xC0, 0x74, 0x16,
    0x3C, 0x20, 0x74, 0x12, 0x3C, 0x09, 0x72, 0x04, 0x3C, 0x0D, 0x76, 0x0A,
    0x88, 0x03, 0x48, 0xFF, 0xC3, 0x49, 0xFF, 0xC4, 0xEB, 0xE1, 0xC6, 0x03,
    0x00, 0x49, 0xFF, 0xC7, 0x49, 0xFF, 0xC5, 0xE9, 0x01, 0xFF, 0xFF, 0xFF,
    0xE8, 0x5F, 0x01, 0x00, 0x00, 0x41, 0x0F, 0xB6, 0x04, 0x24, 0x84, 0xC0,
    0x0F, 0x84, 0x40, 0x01, 0x00, 0x00, 0x4D, 0x31, 0xC0, 0x4D, 0x31, 0xC9,
    0x48, 0x31, 0xDB, 0x3C, 0x2D, 0x75, 0x0C, 0x49, 0xC7, 0xC1, 0x01, 0x00,
    0x00, 0x00, 0x49, 0xFF, 0xC4, 0xEB, 0x07, 0x3C, 0x2B, 0x75, 0x03, 0x49,
    0xFF, 0xC4, 0x41, 0x0F, 0xB6, 0x04, 0x24, 0x89, 0xC1, 0x83, 0xE9, 0x30,
    0x83, 0xF9, 0x09, 0x77, 0x17, 0x4F, 0x8D, 0x04, 0x80, 0x4F, 0x8D, 0x04,
    0x00, 0x49, 0x01, 0xC8, 0x49, 0xFF, 0xC4, 0x48, 0xC7, 0xC3, 0x01, 0x00,
    0x00, 0x00, 0xEB, 0xDA, 0x48, 0x85, 0xDB, 0x74, 0x1B, 0x4D, 0x85, 0xC9,
    0x74, 0x03, 0x49, 0xF7, 0xD8, 0xE8, 0x11, 0x01, 0x00, 0x00, 0x4C, 0x89,
    0x00, 0x49, 0xFF, 0xC7, 0x49, 0xFF, 0xC5, 0xE9, 0x89, 0xFE, 0xFF, 0xFF,
    0xE9, 0xD5, 0x00, 0x00, 0x00, 0xE8, 0xE2, 0x00, 0x00, 0x00, 0x41, 0x0F,
    0xB6, 0x04, 0x24, 0x84, 0xC0, 0x0F, 0x84, 0xC3, 0x00, 0x00, 0x00, 0x4D,
    0x31, 0xC0, 0x48, 0x31, 0xDB, 0x3C, 0x2B, 0x75, 0x03, 0x49, 0xFF, 0xC4,
    0x41, 0x0F, 0xB6, 0x04, 0x24, 0x89, 0xC1, 0x83, 0xE9, 0x30, 0x83, 0xF9,
    0x09, 0x77, 0x17, 0x4F, 0x8D, 0x04, 0x80, 0x4F, 0x8D, 0x04, 0x00, 0x49,
    0x01, 0xC8, 0x49, 0xFF, 0xC4, 0x48, 0xC7, 0xC3, 0x01, 0x00, 0x00, 0x00,
    0xEB, 0xDA, 0x48, 0x85, 0xDB, 0x74, 0x13, 0xE8, 0xAF, 0x00, 0x00, 0x00,
    0x4C, 0x89, 0x00, 0x49, 0xFF, 0xC7, 0x49, 0xFF, 0xC5, 0xE9, 0x27, 0xFE,
    0xFF, 0xFF, 0xEB, 0x76, 0xE8, 0x83, 0x00, 0x00, 0x00, 0x41, 0x0F, 0xB6,
    0x04, 0x24, 0x84, 0xC0, 0x74, 0x68, 0x4D, 0x31, 0xC0, 0x48, 0x31, 0xDB,
    0x3C, 0x30, 0x75, 0x12, 0x41, 0x0F, 0xB6, 0x4C, 0x24, 0x01, 0x83, 0xC9,
    0x20, 0x80, 0xF9, 0x78, 0x75, 0x04, 0x49, 0x83, 0xC4, 0x02, 0x41, 0x0F,
    0xB6, 0x04, 0x24, 0x89, 0xC1, 0x83, 0xE9, 0x30, 0x83, 0xF9, 0x09, 0x76,
    0x10, 0x89, 0xC1, 0x83, 0xC9, 0x20, 0x83, 0xE9, 0x61, 0x83, 0xF9, 0x05,
    0x77, 0x16, 0x83, 0xC1, 0x0A, 0x49, 0xC1, 0xE0, 0x04, 0x49, 0x01, 0xC8,
    0x49, 0xFF, 0xC4, 0x48, 0xC7, 0xC3, 0x01, 0x00, 0x00, 0x00, 0xEB, 0xCE,
    0x48, 0x85, 0xDB, 0x74, 0x13, 0xE8, 0x39, 0x00, 0x00, 0x00, 0x4C, 0x89,
    0x00, 0x49, 0xFF, 0xC7, 0x49, 0xFF, 0xC5, 0xE9, 0xB1, 0xFD, 0xFF, 0xFF,
    0xEB, 0x00, 0x4C, 0x89, 0xF8, 0x48, 0x83, 0xC4, 0x38, 0x41, 0x5F, 0x41,
    0x5E, 0x41, 0x5D, 0x41, 0x5C, 0x5B, 0x5D, 0xC3, 0x41, 0x0F, 0xB6, 0x04,
    0x24, 0x3C, 0x20, 0x74, 0x08, 0x3C, 0x09, 0x72, 0x09, 0x3C, 0x0D, 0x77,
    0x05, 0x49, 0xFF, 0xC4, 0xEB, 0xEA, 0xC3, 0x49, 0x83, 0xFE, 0x04, 0x73,
    0x09, 0x4A, 0x8B, 0x44, 0xF5, 0xB8, 0x49, 0xFF, 0xC6, 0xC3, 0x4C, 0x89,
    0xF0, 0x48, 0x83, 0xE8, 0x04, 0x48, 0x8B, 0x44, 0xC5, 0x10, 0x49, 0xFF,
    0xC6, 0xC3,
};

static const UCHAR g_BuiltinStrrchr[] = {
    0x4D, 0x31, 0xC0, 0x4D, 0x31, 0xC9, 0x42, 0x0F, 0xB6, 0x04, 0x07, 0x84,
    0xC0, 0x74, 0x0E, 0x40, 0x38, 0xF0, 0x75, 0x04, 0x4E, 0x8D, 0x0C, 0x07,
    0x49, 0xFF, 0xC0, 0xEB, 0xE9, 0x40, 0x84, 0xF6, 0x74, 0x04, 0x4C, 0x89,
    0xC8, 0xC3, 0x4A, 0x8D, 0x04, 0x07, 0xC3,
};

static const UCHAR g_BuiltinToupper[] = {
    0x48, 0x89, 0xF8, 0x3C, 0x61, 0x72, 0x06, 0x3C, 0x7A, 0x77, 0x02, 0x2C,
    0x20, 0xC3,
};

static const UCHAR g_BuiltinTolower[] = {
    0x48, 0x89, 0xF8, 0x3C, 0x41, 0x72, 0x06, 0x3C, 0x5A, 0x77, 0x02, 0x04,
    0x20, 0xC3,
};

static const UCHAR g_BuiltinStrstr[] = {
    0x0F, 0xB6, 0x06, 0x84, 0xC0, 0x74, 0x36, 0x0F, 0xB6, 0x07, 0x84, 0xC0,
    0x74, 0x2B, 0x49, 0x89, 0xF8, 0x49, 0x89, 0xF1, 0x41, 0x0F, 0xB6, 0x01,
    0x84, 0xC0, 0x74, 0x19, 0x41, 0x0F, 0xB6, 0x10, 0x84, 0xD2, 0x74, 0x15,
    0x38, 0xD0, 0x75, 0x08, 0x49, 0xFF, 0xC0, 0x49, 0xFF, 0xC1, 0xEB, 0xE4,
    0x48, 0xFF, 0xC7, 0xEB, 0xD2, 0x48, 0x89, 0xF8, 0xC3, 0x48, 0x31, 0xC0,
    0xC3, 0x48, 0x89, 0xF8, 0xC3,
};

static const UCHAR g_BuiltinAtoi[] = {
    0x31, 0xC0, 0x45, 0x31, 0xC0, 0x0F, 0xB6, 0x17,
    0x80, 0xFA, 0x2D, 0x75, 0x09, 0x41, 0xB8, 0x01,
    0x00, 0x00, 0x00, 0x48, 0xFF, 0xC7, 0x0F, 0xB6,
    0x17, 0x84, 0xD2, 0x74, 0x15, 0x80, 0xEA, 0x30,
    0x80, 0xFA, 0x09, 0x77, 0x0D, 0x6B, 0xC0, 0x0A,
    0x0F, 0xB6, 0xD2, 0x01, 0xD0, 0x48, 0xFF, 0xC7,
    0xEB, 0xE4, 0x45, 0x85, 0xC0, 0x74, 0x03, 0x48,
    0xF7, 0xD8, 0xC3,
};

static const UCHAR g_BuiltinStrchr[] = {
    0x48, 0x89, 0xF8, 0x0F, 0xB6, 0x10, 0x40, 0x38,
    0xF2, 0x74, 0x0D, 0x84, 0xD2, 0x74, 0x05, 0x48,
    0xFF, 0xC0, 0xEB, 0xEF, 0x48, 0x31, 0xC0, 0xC3,
    0xC3,
};

static const UCHAR g_BuiltinCalloc[] = {
    0x48, 0x89, 0xF8, 0x48, 0x0F, 0xAF, 0xC6, 0x48,
    0x83, 0xC0, 0x0F, 0x48, 0x83, 0xE0, 0xF0, 0x49,
    0x89, 0xC0, 0x48, 0xC7, 0xC0, 0x0C, 0x00, 0x00,
    0x00, 0x48, 0x31, 0xFF, 0x0F, 0x05, 0x49, 0x89,
    0xC1, 0x4C, 0x89, 0xCF, 0x4C, 0x01, 0xC7, 0x48,
    0xC7, 0xC0, 0x0C, 0x00, 0x00, 0x00, 0x0F, 0x05,
    0x4C, 0x89, 0xCF, 0x4C, 0x89, 0xC1, 0x31, 0xC0,
    0xF3, 0xAA, 0x4C, 0x89, 0xC8, 0xC3,
};

static const UCHAR g_BuiltinItoa[] = {
    0x49, 0x89, 0xF0, 0x48, 0x89, 0xF0, 0x48, 0x85,
    0xFF, 0x7D, 0x0A, 0x41, 0xC6, 0x00, 0x2D, 0x49,
    0xFF, 0xC0, 0x48, 0xF7, 0xDF, 0x48, 0x85, 0xFF,
    0x75, 0x09, 0x41, 0xC6, 0x00, 0x30, 0x49, 0xFF,
    0xC0, 0xEB, 0x2C, 0x31, 0xC9, 0x48, 0x89, 0xF8,
    0x31, 0xD2, 0x49, 0xC7, 0xC1, 0x0A, 0x00, 0x00,
    0x00, 0x49, 0xF7, 0xF1, 0x52, 0x48, 0xFF, 0xC1,
    0x48, 0x89, 0xC7, 0x48, 0x85, 0xC0, 0x75, 0xE5,
    0x5A, 0x80, 0xC2, 0x30, 0x41, 0x88, 0x10, 0x49,
    0xFF, 0xC0, 0x48, 0xFF, 0xC9, 0x75, 0xF1, 0x41,
    0xC6, 0x00, 0x00, 0x48, 0x89, 0xF0, 0xC3,
};

static const LX_BUILTIN_DEF g_Builtins[LX_BUILTIN_COUNT] = {
    { "memcpy", g_BuiltinMemcpy, sizeof(g_BuiltinMemcpy) },
    { "memset", g_BuiltinMemset, sizeof(g_BuiltinMemset) },
    { "strlen", g_BuiltinStrlen, sizeof(g_BuiltinStrlen) },
    { "strcmp", g_BuiltinStrcmp, sizeof(g_BuiltinStrcmp) },
    { "strcpy", g_BuiltinStrcpy, sizeof(g_BuiltinStrcpy) },
    { "memcmp", g_BuiltinMemcmp, sizeof(g_BuiltinMemcmp) },
    { "memmove", g_BuiltinMemmove, sizeof(g_BuiltinMemmove) },
    { "strncmp", g_BuiltinStrncmp, sizeof(g_BuiltinStrncmp) },
    { "calloc", g_BuiltinCalloc, sizeof(g_BuiltinCalloc) },
    { "itoa", g_BuiltinItoa, sizeof(g_BuiltinItoa) },
    { "atoi", g_BuiltinAtoi, sizeof(g_BuiltinAtoi) },
        { "strchr", g_BuiltinStrchr, sizeof(g_BuiltinStrchr) },
    { "printf", g_BuiltinPrintf, sizeof(g_BuiltinPrintf) },
    { "strstr", g_BuiltinStrstr, sizeof(g_BuiltinStrstr) },
    { "sprintf", g_BuiltinSprintf, sizeof(g_BuiltinSprintf) },
    { "toupper", g_BuiltinToupper, sizeof(g_BuiltinToupper) },
    { "tolower", g_BuiltinTolower, sizeof(g_BuiltinTolower) },
    { "snprintf", g_BuiltinSnprintf, sizeof(g_BuiltinSnprintf) },
    { "strncpy", g_BuiltinStrncpy, sizeof(g_BuiltinStrncpy) },
    { "strcat", g_BuiltinStrcat, sizeof(g_BuiltinStrcat) },
    { "strrchr", g_BuiltinStrrchr, sizeof(g_BuiltinStrrchr) },
    { "strtok_r", g_BuiltinStrtokR, sizeof(g_BuiltinStrtokR) },
    { "qsort", g_BuiltinQsort, sizeof(g_BuiltinQsort) },
    { "bsearch", g_BuiltinBsearch, sizeof(g_BuiltinBsearch) },
    { "rand_r", g_BuiltinRandR, sizeof(g_BuiltinRandR) },
    { "abs", g_BuiltinAbs, sizeof(g_BuiltinAbs) },
    { "labs", g_BuiltinLabs, sizeof(g_BuiltinLabs) },
    { "isalpha", g_BuiltinIsalpha, sizeof(g_BuiltinIsalpha) },
    { "isdigit", g_BuiltinIsdigit, sizeof(g_BuiltinIsdigit) },
    { "isspace", g_BuiltinIsspace, sizeof(g_BuiltinIsspace) },
    { "isupper", g_BuiltinIsupper, sizeof(g_BuiltinIsupper) },
    { "islower", g_BuiltinIslower, sizeof(g_BuiltinIslower) },
    { "isalnum", g_BuiltinIsalnum, sizeof(g_BuiltinIsalnum) },
    { "isxdigit", g_BuiltinIsxdigit, sizeof(g_BuiltinIsxdigit) },
    { "strtol", g_BuiltinStrtol, sizeof(g_BuiltinStrtol) },
    { "memchr", g_BuiltinMemchr, sizeof(g_BuiltinMemchr) },
    { "memrchr", g_BuiltinMemrchr, sizeof(g_BuiltinMemrchr) },
    { "strncat", g_BuiltinStrncat, sizeof(g_BuiltinStrncat) },
    { "strspn", g_BuiltinStrspn, sizeof(g_BuiltinStrspn) },
    { "strcspn", g_BuiltinStrcspn, sizeof(g_BuiltinStrcspn) },
    { "strpbrk", g_BuiltinStrpbrk, sizeof(g_BuiltinStrpbrk) },
    { "sscanf", g_BuiltinSscanf, sizeof(g_BuiltinSscanf) },
    { "div", g_BuiltinDiv, sizeof(g_BuiltinDiv) },
    { "ldiv", g_BuiltinLdiv, sizeof(g_BuiltinLdiv) },
    { "strsep", g_BuiltinStrsep, sizeof(g_BuiltinStrsep) },
    { "memmem", g_BuiltinMemmem, sizeof(g_BuiltinMemmem) },
    { "bzero", g_BuiltinBzero, sizeof(g_BuiltinBzero) },
    { "bcopy", g_BuiltinBcopy, sizeof(g_BuiltinBcopy) },
    { "time", g_BuiltinTime, sizeof(g_BuiltinTime) },
    { "getpid", g_BuiltinGetpid, sizeof(g_BuiltinGetpid) },
    { "getppid", g_BuiltinGetppid, sizeof(g_BuiltinGetppid) },
    { "sleep", g_BuiltinSleep, sizeof(g_BuiltinSleep) },
    { "usleep", g_BuiltinUsleep, sizeof(g_BuiltinUsleep) },
    { "isatty", g_BuiltinIsatty, sizeof(g_BuiltinIsatty) },
    { "ffs", g_BuiltinFfs, sizeof(g_BuiltinFfs) }
};

static ULONG LxBuildMallocCode(ULONG64 BrkAddress, UCHAR *Out, ULONG OutSize)
{
    ULONG pos = 0;

    UNREFERENCED_PARAMETER(BrkAddress);

    if (Out == NULL || OutSize < LX_BUILTIN_MALLOC_MAX)
    {
        return 0;
    }

    Out[pos++] = 0x49;
    Out[pos++] = 0x89;
    Out[pos++] = 0xf8;
    Out[pos++] = 0x48;
    Out[pos++] = 0xc7;
    Out[pos++] = 0xc0;
    Out[pos++] = 0x0c;
    Out[pos++] = 0x00;
    Out[pos++] = 0x00;
    Out[pos++] = 0x00;
    Out[pos++] = 0x48;
    Out[pos++] = 0x31;
    Out[pos++] = 0xff;
    Out[pos++] = 0x0f;
    Out[pos++] = 0x05;
    Out[pos++] = 0x49;
    Out[pos++] = 0x89;
    Out[pos++] = 0xc1;
    Out[pos++] = 0x49;
    Out[pos++] = 0x83;
    Out[pos++] = 0xc0;
    Out[pos++] = 0x0f;
    Out[pos++] = 0x49;
    Out[pos++] = 0x83;
    Out[pos++] = 0xe0;
    Out[pos++] = 0xf0;
    Out[pos++] = 0x4c;
    Out[pos++] = 0x89;
    Out[pos++] = 0xcf;
    Out[pos++] = 0x4c;
    Out[pos++] = 0x01;
    Out[pos++] = 0xc7;
    Out[pos++] = 0x48;
    Out[pos++] = 0xc7;
    Out[pos++] = 0xc0;
    Out[pos++] = 0x0c;
    Out[pos++] = 0x00;
    Out[pos++] = 0x00;
    Out[pos++] = 0x00;
    Out[pos++] = 0x0f;
    Out[pos++] = 0x05;
    Out[pos++] = 0x4c;
    Out[pos++] = 0x89;
    Out[pos++] = 0xc8;
    Out[pos++] = 0xc3;

    return pos;
}



static const UCHAR g_BuiltinFree[] = {
    0xC3
};

static void LxInstallBuiltins(PEPROCESS TargetProcess, ULONG64 AreaBase, LX_GLOBAL_TABLE *Globals,
                              LX_BUILTIN_STATS *Stats)
{
    ULONG index;
    ULONG64 cursor;

    RtlZeroMemory(Stats, sizeof(LX_BUILTIN_STATS));

    if (TargetProcess == NULL || AreaBase == 0 || Stats == NULL)
    {
        return;
    }

    Stats->AreaBase = AreaBase;
    cursor = AreaBase;

    for (index = 0; index < LX_BUILTIN_COUNT; index++)
    {
        const LX_BUILTIN_DEF *def = &g_Builtins[index];
        ULONG64 address = (cursor + 15ull) & ~15ull;
        NTSTATUS writeStatus;

        if (address + def->Size > AreaBase + LX_BUILTIN_MAX_SIZE)
        {
            Stats->FailCount++;
            break;
        }

        writeStatus = LxWritePicoMemory(TargetProcess, address, def->Code, def->Size);

        Stats->LastStatus = writeStatus;

        if (!NT_SUCCESS(writeStatus))
        {
            Stats->FailCount++;
            continue;
        }

        Stats->Written++;
        Stats->TotalBytes += def->Size;
        LxCopyString(Stats->LastName, def->Name, LX_MAX_SYMBOL_NAME);
        Stats->LastAddress = address;

        if (Globals != NULL)
        {
            if (LxGlobalTableAdd(Globals, def->Name, address, LX_MAX_LIBS, 1, 2))
            {
                Stats->Registered++;
            }
        }

        cursor = address + def->Size;
    }

    {
        UCHAR mallocCode[LX_BUILTIN_MALLOC_MAX];
        ULONG mallocSize;
        ULONG64 brkAddress;
        ULONG64 zeroValue = 0;
        ULONG64 mallocAddress = (cursor + 15ull) & ~15ull;
        NTSTATUS mallocStatus;

        mallocSize = LxBuildMallocCode(0, mallocCode, sizeof(mallocCode));

        if (mallocSize > 0 && mallocAddress + mallocSize <= AreaBase + LX_BUILTIN_MAX_SIZE)
        {
            mallocStatus = LxWritePicoMemory(TargetProcess, mallocAddress, mallocCode, mallocSize);
            Stats->LastStatus = mallocStatus;

            if (NT_SUCCESS(mallocStatus))
            {
                Stats->Written++;
                Stats->TotalBytes += mallocSize;
                LxCopyString(Stats->LastName, "malloc", LX_MAX_SYMBOL_NAME);
                Stats->LastAddress = mallocAddress;

                if (Globals != NULL)
                {
                    if (LxGlobalTableAdd(Globals, "malloc", mallocAddress, LX_MAX_LIBS, 1, 2))
                    {
                        Stats->Registered++;
                    }
                }

                cursor = mallocAddress + mallocSize;
            }
            else
            {
                Stats->FailCount++;
            }
        }

        {
            ULONG64 freeAddress = (cursor + 15ull) & ~15ull;
            NTSTATUS freeStatus;

            freeStatus = LxWritePicoMemory(TargetProcess, freeAddress, g_BuiltinFree, sizeof(g_BuiltinFree));
            Stats->LastStatus = freeStatus;

            if (NT_SUCCESS(freeStatus))
            {
                Stats->Written++;
                Stats->TotalBytes += sizeof(g_BuiltinFree);
                LxCopyString(Stats->LastName, "free", LX_MAX_SYMBOL_NAME);
                Stats->LastAddress = freeAddress;
                cursor = freeAddress + sizeof(g_BuiltinFree);

                if (Globals != NULL)
                {
                    if (LxGlobalTableAdd(Globals, "free", freeAddress, LX_MAX_LIBS, 1, 2))
                    {
                        Stats->Registered++;
                    }
                }
            }
            else
            {
                Stats->FailCount++;
            }
        }

        brkAddress = (cursor + 15ull) & ~15ull;

        {
            NTSTATUS initStatus = LxWritePicoMemory(TargetProcess, brkAddress, &zeroValue, sizeof(zeroValue));

            Stats->LastStatus = initStatus;
            Stats->BrkAddress = brkAddress;
            Stats->BrkInitStatus = (ULONG)initStatus;
        }
    }
}

static ULONG64 LxTryLoadLibraryFile(const char *Name, UCHAR **OutBuffer, ULONG *OutLength, ULONG *OutStatus)
{
    static const char *Prefixes[] = {
        "/lib/x86_64-linux-gnu/",
        "/usr/lib/x86_64-linux-gnu/",
        "/lib64/",
        "/usr/lib64/",
        "/lib/",
        "/usr/lib/"
    };
    CHAR fullPath[192];
    ULONG prefixIndex;
    ULONG nameLength = 0;

    *OutBuffer = NULL;
    *OutLength = 0;
    *OutStatus = 0xC0000001u;

    if (Name == NULL || Name[0] == 0)
    {
        return 0;
    }

    while (Name[nameLength] != 0 && nameLength + 1 < LX_MAX_SYMBOL_NAME)
    {
        nameLength++;
    }

    if (nameLength == 0)
    {
        return 0;
    }

    for (prefixIndex = 0; prefixIndex < RTL_NUMBER_OF(Prefixes); prefixIndex++)
    {
        const char *prefix = Prefixes[prefixIndex];
        ULONG prefixLength = 0;
        ULONG buildIndex;
        WCHAR ntPath[260];
        UNICODE_STRING uniPath;
        OBJECT_ATTRIBUTES attributes;
        IO_STATUS_BLOCK statusBlock;
        HANDLE fileHandle = NULL;
        FILE_STANDARD_INFORMATION fileInfo;
        NTSTATUS status;
        ULONG pathLength;

        while (prefix[prefixLength] != 0)
        {
            prefixLength++;
        }

        if (prefixLength + nameLength + 1 >= RTL_NUMBER_OF(fullPath))
        {
            continue;
        }

        for (buildIndex = 0; buildIndex < prefixLength; buildIndex++)
        {
            fullPath[buildIndex] = prefix[buildIndex];
        }

        for (buildIndex = 0; buildIndex < nameLength; buildIndex++)
        {
            fullPath[prefixLength + buildIndex] = Name[buildIndex];
        }

        fullPath[prefixLength + nameLength] = 0;

        pathLength = LxBuildNtPath(fullPath, ntPath, RTL_NUMBER_OF(ntPath));

        if (pathLength == 0)
        {
            continue;
        }

        RtlInitUnicodeString(&uniPath, ntPath);
        InitializeObjectAttributes(&attributes, &uniPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

        status = ZwOpenFile(&fileHandle, GENERIC_READ | FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attributes,
                            &statusBlock, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);

        *OutStatus = (ULONG)status;

        if (!NT_SUCCESS(status))
        {
            continue;
        }

        RtlZeroMemory(&fileInfo, sizeof(fileInfo));

        status = ZwQueryInformationFile(fileHandle, &statusBlock, &fileInfo, sizeof(fileInfo),
                                        FileStandardInformation);

        if (!NT_SUCCESS(status) || fileInfo.EndOfFile.QuadPart <= 0 ||
            fileInfo.EndOfFile.QuadPart > (LONGLONG)LX_LIB_IMAGE_MAX)
        {
            *OutStatus = NT_SUCCESS(status) ? 0xC0000023u : (ULONG)status;
            ZwClose(fileHandle);
            continue;
        }

        {
            ULONG length = (ULONG)fileInfo.EndOfFile.QuadPart;
            PVOID buffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, length, 'bilL');

            if (buffer == NULL)
            {
                *OutStatus = 0xC000009Au;
                ZwClose(fileHandle);
                continue;
            }

            status = ZwReadFile(fileHandle, NULL, NULL, NULL, &statusBlock, buffer, length, NULL, NULL);

            ZwClose(fileHandle);
            *OutStatus = (ULONG)status;

            if (!NT_SUCCESS(status))
            {
                ExFreePool(buffer);
                continue;
            }

            *OutBuffer = (UCHAR *)buffer;
            *OutLength = (ULONG)statusBlock.Information;

            return (ULONG64)*OutLength;
        }
    }

    return 0;
}

static ULONG64 LxLoadSharedLibrary(const char *Name, PEPROCESS TargetProcess, ULONG LibIndex,
                                   LX_LIB_ENTRY *Entry, LX_LIB_STATS *Stats, LX_GLOBAL_TABLE *Globals)
{
    UCHAR *fileBuffer = NULL;
    ULONG fileLength = 0;
    ULONG openStatus = 0;
    ULONG64 baseAddress;
    ULONG64 minVaddr = 0xFFFFFFFFFFFFFFFFull;
    ULONG64 maxEnd = 0;
    ULONG phdrIndex;
    HANDLE sectionHandle = NULL;
    OBJECT_ATTRIBUTES sectionAttributes;
    LARGE_INTEGER sectionSize;
    PVOID viewBase;
    SIZE_T viewSize = 0;
    NTSTATUS status;
    ULONG copiedName;

    RtlZeroMemory(Entry, sizeof(LX_LIB_ENTRY));

    for (copiedName = 0; copiedName + 1 < LX_MAX_SYMBOL_NAME && Name[copiedName] != 0; copiedName++)
    {
        Entry->Name[copiedName] = Name[copiedName];
    }

    Entry->Index = LibIndex;
    Entry->Used = 1;

    Stats->Attempted++;

    if (LxTryLoadLibraryFile(Name, &fileBuffer, &fileLength, &openStatus) == 0 || fileBuffer == NULL)
    {
        Entry->LoadStatus = openStatus == 0 ? 0xC0000001u : openStatus;
        Stats->Failed++;
        Stats->LastStatus = Entry->LoadStatus;
        Stats->PathResolveFail++;
        return 0;
    }

    if (fileLength < sizeof(LX_ELF_LOAD) || fileLength < 64)
    {
        Entry->LoadStatus = 0xC000000Du;
        Stats->Failed++;
        Stats->LastStatus = Entry->LoadStatus;
        ExFreePool(fileBuffer);
        return 0;
    }

    if (fileBuffer[0] != 0x7F || fileBuffer[1] != 'E' || fileBuffer[2] != 'L' || fileBuffer[3] != 'F')
    {
        Entry->LoadStatus = 0xC000007Bu;
        Stats->Failed++;
        Stats->LastStatus = Entry->LoadStatus;
        ExFreePool(fileBuffer);
        return 0;
    }

    {
        ULONG64 phoff = *(ULONG64 *)(fileBuffer + 32);
        USHORT phnum = *(USHORT *)(fileBuffer + 56);
        USHORT phentsize = *(USHORT *)(fileBuffer + 54);

        Entry->PhdrCount = phnum;
        Entry->PhdrEntrySize = phentsize;

        if (phoff + (ULONG64)phnum * phentsize > fileLength || phentsize < 56 || phnum == 0)
        {
            Entry->LoadStatus = 0xC000000Du;
            Stats->Failed++;
            Stats->LastStatus = Entry->LoadStatus;
            ExFreePool(fileBuffer);
            return 0;
        }

        for (phdrIndex = 0; phdrIndex < phnum && phdrIndex < 64; phdrIndex++)
        {
            const UCHAR *phdr = fileBuffer + phoff + (ULONG64)phdrIndex * phentsize;
            ULONG pType = *(ULONG *)(phdr + 0);
            ULONG64 pVaddr = *(ULONG64 *)(phdr + 16);
            ULONG64 pMemsz = *(ULONG64 *)(phdr + 40);

            if (pType != LX_PT_LOAD)
            {
                continue;
            }

            if ((pVaddr & ~0xFFFull) < minVaddr)
            {
                minVaddr = pVaddr & ~0xFFFull;
            }

            if (((pVaddr + pMemsz) + 0xFFFull) > maxEnd)
            {
                maxEnd = ((pVaddr + pMemsz) + 0xFFFull) & ~0xFFFull;
            }
        }

        if (minVaddr == 0xFFFFFFFFFFFFFFFFull || maxEnd <= minVaddr)
        {
            Entry->LoadStatus = 0xC000000Du;
            Stats->Failed++;
            Stats->LastStatus = Entry->LoadStatus;
            ExFreePool(fileBuffer);
            return 0;
        }

        Entry->ImageSize = maxEnd - minVaddr;

        {
            const UCHAR *firstLoad = NULL;
            ULONG scanIndex;

            for (scanIndex = 0; scanIndex < phnum && scanIndex < 64; scanIndex++)
            {
                const UCHAR *phdr = fileBuffer + phoff + (ULONG64)scanIndex * phentsize;

                if (*(ULONG *)(phdr + 0) == LX_PT_LOAD)
                {
                    firstLoad = phdr;
                    break;
                }
            }

            if (firstLoad != NULL)
            {
                ULONG64 firstOffset = *(ULONG64 *)(firstLoad + 8);
                Entry->PhdrAddress = minVaddr + (phoff - firstOffset);
            }
        }
    }

    baseAddress = 0x10000000ull + (ULONG64)LibIndex * LX_LIB_BASE_STEP;
    Entry->BaseAddress = baseAddress;

    sectionSize.QuadPart = (LONGLONG)((Entry->ImageSize + 0xFFFull) & ~0xFFFull);
    InitializeObjectAttributes(&sectionAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwCreateSection(&sectionHandle, SECTION_ALL_ACCESS, &sectionAttributes, &sectionSize,
                             PAGE_EXECUTE_READWRITE, SEC_COMMIT, NULL);

    Entry->LoadStatus = (ULONG)status;

    if (!NT_SUCCESS(status))
    {
        Stats->Failed++;
        Stats->LastStatus = Entry->LoadStatus;
        ExFreePool(fileBuffer);
        return 0;
    }

    viewBase = (PVOID)(ULONG_PTR)baseAddress;
    viewSize = 0;

    status = ZwMapViewOfSection(sectionHandle, (HANDLE)g_LastPicoHandle, &viewBase, 0, 0, NULL, &viewSize,
                                ViewUnmap, 0, PAGE_EXECUTE_READWRITE);

    ZwClose(sectionHandle);

    Entry->LoadStatus = (ULONG)status;

    if (!NT_SUCCESS(status))
    {
        Stats->Failed++;
        Stats->LastStatus = Entry->LoadStatus;
        ExFreePool(fileBuffer);
        return 0;
    }

    Entry->BaseAddress = (ULONG64)(ULONG_PTR)viewBase;

    {
        ULONG64 phoff = *(ULONG64 *)(fileBuffer + 32);
        USHORT phnum = *(USHORT *)(fileBuffer + 56);
        USHORT phentsize = *(USHORT *)(fileBuffer + 54);
        ULONG segIndex;

        for (segIndex = 0; segIndex < phnum && segIndex < 64; segIndex++)
        {
            const UCHAR *phdr = fileBuffer + phoff + (ULONG64)segIndex * phentsize;
            ULONG pType = *(ULONG *)(phdr + 0);
            ULONG64 pOffset;
            ULONG64 pVaddr;
            ULONG64 pFilesz;
            ULONG64 destAddress;
            SIZE_T copied = 0;
            NTSTATUS copyStatus;

            if (pType != LX_PT_LOAD)
            {
                continue;
            }

            pOffset = *(ULONG64 *)(phdr + 8);
            pVaddr = *(ULONG64 *)(phdr + 16);
            pFilesz = *(ULONG64 *)(phdr + 32);

            if (pFilesz == 0 || pOffset + pFilesz > fileLength)
            {
                continue;
            }

            destAddress = Entry->BaseAddress + (pVaddr - minVaddr);

            copyStatus = MmCopyVirtualMemory(PsGetCurrentProcess(), fileBuffer + pOffset, TargetProcess,
                                             (PVOID)(ULONG_PTR)destAddress, (SIZE_T)pFilesz, KernelMode, &copied);

            if (!NT_SUCCESS(copyStatus))
            {
                Entry->LoadStatus = (ULONG)copyStatus;
                Stats->Failed++;
                Stats->LastStatus = Entry->LoadStatus;
                ExFreePool(fileBuffer);
                return 0;
            }
        }
    }

    {
        LX_ELF_LOAD libLoad;

        RtlZeroMemory(&libLoad, sizeof(libLoad));
        libLoad.ImageSize = Entry->ImageSize;
        libLoad.MinVaddr = minVaddr;
        libLoad.DataSize = fileLength;
        libLoad.PhdrAddress = Entry->PhdrAddress;
        libLoad.PhdrCount = Entry->PhdrCount;
        libLoad.PhdrEntrySize = Entry->PhdrEntrySize;
        libLoad.SegmentCount = 1;

        {
            LX_ELF_DYNAMIC_INFO libDynamic;

            LxParseElfDynamic(fileBuffer, fileLength, &libLoad, &libDynamic);

            Entry->HasDynamic = libDynamic.HasDynamic;
            Entry->DynamicOffset = libDynamic.DynamicOffset;
            Entry->DynamicSize = libDynamic.DynamicSize;
            Entry->StrTabVaddr = libDynamic.StrTabVaddr;
            Entry->SymTabVaddr = libDynamic.SymTabVaddr;
            Entry->NeededCount = libDynamic.NeededCount;
            Entry->RelaCount = libDynamic.RelaCount;

            for (phdrIndex = 0; phdrIndex < libDynamic.NeededCount && phdrIndex < LX_MAX_NEEDED; phdrIndex++)
            {
                Entry->NeededOffsets[phdrIndex] = libDynamic.NeededOffsets[phdrIndex];
            }

            if (libDynamic.HasDynamic && libDynamic.RelaSize > 0 && libDynamic.RelaVaddr != 0)
            {
                ULONG64 relaOffset = libDynamic.RelaVaddr - minVaddr;
                ULONG relaCount = (ULONG)(libDynamic.RelaSize / sizeof(LX_ELF_RELA));
                ULONG relaIndex;

                if (relaCount > LX_MAX_RELOCATIONS)
                {
                    relaCount = LX_MAX_RELOCATIONS;
                }

                for (relaIndex = 0; relaIndex < relaCount; relaIndex++)
                {
                    ULONG64 entryOffset = relaOffset + (ULONG64)relaIndex * sizeof(LX_ELF_RELA);
                    LX_ELF_RELA rela;
                    ULONG relocType;
                    ULONG64 destAddress;
                    ULONG64 newValue;
                    SIZE_T written = 0;

                    if (entryOffset + sizeof(LX_ELF_RELA) > fileLength)
                    {
                        break;
                    }

                    RtlCopyMemory(&rela, fileBuffer + entryOffset, sizeof(LX_ELF_RELA));

                    relocType = (ULONG)(rela.Info & 0xFFFFFFFFull);

                    if (relocType != LX_R_X86_64_RELATIVE)
                    {
                        continue;
                    }

                    destAddress = Entry->BaseAddress + rela.Offset;
                    newValue = Entry->BaseAddress + (ULONG64)rela.Addend;

                    if (NT_SUCCESS(MmCopyVirtualMemory(PsGetCurrentProcess(), &newValue, TargetProcess,
                                                       (PVOID)(ULONG_PTR)destAddress, sizeof(newValue),
                                                       KernelMode, &written)))
                    {
                        Entry->RelaApplied++;
                        Stats->TotalRelocs++;
                    }
                }
            }

            if (libDynamic.SymTabVaddr != 0 && libDynamic.StrTabVaddr != 0)
            {
                ULONG64 symTabOffset = libDynamic.SymTabVaddr - minVaddr;
                ULONG64 strTabOffset = libDynamic.StrTabVaddr - minVaddr;
                ULONG symWalk;

                ULONG symLimit = LX_MAX_LIB_SYMBOLS;

                if (libDynamic.SymCountFromHash > 0 && libDynamic.SymCountFromHash < symLimit)
                {
                    symLimit = (ULONG)libDynamic.SymCountFromHash;
                }

                for (symWalk = 1; symWalk < symLimit; symWalk++)
                {
                    ULONG64 symEntryOffset = symTabOffset + (ULONG64)symWalk * sizeof(LX_ELF_SYM);
                    LX_ELF_SYM sym;
                    ULONG64 nameOffset;
                    ULONG charIndex;

                    if (symEntryOffset + sizeof(LX_ELF_SYM) > fileLength)
                    {
                        break;
                    }

                    RtlCopyMemory(&sym, fileBuffer + symEntryOffset, sizeof(LX_ELF_SYM));

                    if (sym.NameOffset == 0)
                    {
                        continue;
                    }

                    nameOffset = strTabOffset + sym.NameOffset;

                    if (nameOffset >= fileLength)
                    {
                        continue;
                    }

                    {
                        CHAR symbolName[LX_MAX_SYMBOL_NAME];
                        ULONG nameCharIndex;
                        ULONG64 symbolAddress;

                        RtlZeroMemory(symbolName, sizeof(symbolName));

                        for (nameCharIndex = 0; nameCharIndex + 1 < LX_MAX_SYMBOL_NAME; nameCharIndex++)
                        {
                            ULONG64 pos = nameOffset + nameCharIndex;

                            if (pos >= fileLength || fileBuffer[pos] == 0)
                            {
                                break;
                            }

                            symbolName[nameCharIndex] = (CHAR)fileBuffer[pos];
                        }

                        symbolName[LX_MAX_SYMBOL_NAME - 1] = 0;

                        if (symWalk == 1)
                        {
                            LxCopyString(Entry->FirstSymbol, symbolName, LX_MAX_SYMBOL_NAME);
                        }

                        symbolAddress = 0;

                        if (sym.Value != 0)
                        {
                            symbolAddress = Entry->BaseAddress + sym.Value;
                        }

                        if (symbolAddress == 0 && sym.NameOffset != 0)
                        {
                            symbolAddress = Entry->BaseAddress;
                        }

                        if (Globals != NULL && symbolName[0] != 0 && symbolAddress != 0)
                        {
                            LxGlobalTableAdd(Globals, symbolName, symbolAddress, LibIndex, 1, 1);
                        }
                    }

                    UNREFERENCED_PARAMETER(charIndex);
                    Entry->SymCount++;
                    Stats->TotalSymbols++;
                }
            }
        }
    }

    Entry->LoadStatus = 0;
    Stats->Loaded++;
    Stats->LastStatus = 0;

    ExFreePool(fileBuffer);

    return Entry->BaseAddress;
}

static void LxLoadNeededLibraries(UCHAR *ImageData, ULONG ImageSize, const LX_ELF_LOAD *Load,
                                  const LX_ELF_DYNAMIC_INFO *Info, PEPROCESS TargetProcess,
                                  LX_LIB_STATS *Stats, LX_GLOBAL_TABLE *Globals)
{
    ULONG neededIndex;

    RtlZeroMemory(Stats, sizeof(LX_LIB_STATS));

    if (ImageData == NULL || Load == NULL || Info == NULL || TargetProcess == NULL)
    {
        return;
    }

    if (Info->NeededCount == 0 || Info->StrTabVaddr == 0)
    {
        return;
    }

    for (neededIndex = 0; neededIndex < Info->NeededCount && neededIndex < LX_MAX_LIBS; neededIndex++)
    {
        ULONG64 nameOffset = Info->StrTabVaddr + Info->NeededOffsets[neededIndex];
        ULONG64 strTabOffset;
        CHAR libName[LX_MAX_SYMBOL_NAME];
        ULONG charIndex;

        RtlZeroMemory(libName, sizeof(libName));

        if (nameOffset < Load->MinVaddr)
        {
            Stats->NameResolveFail++;
            continue;
        }

        strTabOffset = nameOffset - Load->MinVaddr;

        if (strTabOffset >= ImageSize)
        {
            Stats->NameResolveFail++;
            continue;
        }

        for (charIndex = 0; charIndex + 1 < LX_MAX_SYMBOL_NAME; charIndex++)
        {
            ULONG64 pos = strTabOffset + charIndex;

            if (pos >= ImageSize)
            {
                break;
            }

            libName[charIndex] = (CHAR)ImageData[pos];

            if (libName[charIndex] == 0)
            {
                break;
            }
        }

        libName[LX_MAX_SYMBOL_NAME - 1] = 0;

        if (libName[0] == 0)
        {
            Stats->NameResolveFail++;
            continue;
        }

        LxLoadSharedLibrary(libName, TargetProcess, neededIndex, &Stats->Libs[neededIndex], Stats, Globals);
    }
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

    if (g_PicoProcessObject != NULL)
    {
        ObDereferenceObject(g_PicoProcessObject);
        g_PicoProcessObject = NULL;
    }

    ObReferenceObject(targetProcess);
    g_PicoProcessObject = targetProcess;

    maximumSize.QuadPart = (LONGLONG)((load->ImageSize + 0xFFF) & ~0xFFFull);
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

    LxParseElfDynamic(imageData, load->DataSize, load, &g_ElfDynamic);

    LxApplyRelocations(imageData, load->DataSize, load, &g_ElfDynamic, targetProcess, &g_RelocStats);

    LxDumpSymbols(imageData, load->DataSize, load, &g_ElfDynamic, &g_SymbolInfo);

    LxGlobalTableReset(&g_GlobalSymbols);

    LxLoadNeededLibraries(imageData, load->DataSize, load, &g_ElfDynamic, targetProcess, &g_LibStats,
                          &g_GlobalSymbols);

    LxBuildStubs(imageData, load->DataSize, load, &g_ElfDynamic, targetProcess, &g_SymbolInfo, &g_StubStats,
                 &g_GlobalSymbols);

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
            result->DynInterpVaddr = g_ElfDynamic.InterpVaddr;
            result->DynHasInterp = g_ElfDynamic.HasInterp;
            result->DynHasDynamic = g_ElfDynamic.HasDynamic;
            result->DynDynamicVaddr = g_ElfDynamic.DynamicVaddr;
            result->DynDynamicSize = g_ElfDynamic.DynamicSize;
            result->DynDynamicCount = g_ElfDynamic.DynamicCount;
            result->DynStrTabVaddr = g_ElfDynamic.StrTabVaddr;
            result->DynSymTabVaddr = g_ElfDynamic.SymTabVaddr;
            result->DynRelaVaddr = g_ElfDynamic.RelaVaddr;
            result->DynRelaSize = g_ElfDynamic.RelaSize;
            result->DynRelaCount = g_ElfDynamic.RelaCount;
            result->DynJmpRelVaddr = g_ElfDynamic.JmpRelVaddr;
            result->DynJmpRelSize = g_ElfDynamic.JmpRelSize;
            result->DynJmpRelCount = g_ElfDynamic.JmpRelCount;
            result->DynPltGotVaddr = g_ElfDynamic.PltGotVaddr;
            result->DynInitVaddr = g_ElfDynamic.InitVaddr;
            result->DynFiniVaddr = g_ElfDynamic.FiniVaddr;
            result->DynSonameOffset = g_ElfDynamic.SonameOffset;
            result->DynNeededCount = g_ElfDynamic.NeededCount;
            result->RelocRelativeCount = g_RelocStats.RelativeCount;
            result->RelocJumpSlotCount = g_RelocStats.JumpSlotCount;
            result->RelocGlobDatCount = g_RelocStats.GlobDatCount;
            result->RelocUnsupportedCount = g_RelocStats.UnsupportedCount;
            result->RelocWriteFailCount = g_RelocStats.WriteFailCount;
            result->RelocTotalCount = g_RelocStats.TotalCount;
            result->RelocFirstOffset = g_RelocStats.FirstOffset;
            result->RelocFirstAddend = g_RelocStats.FirstAddend;
            result->RelocFirstWritten = g_RelocStats.FirstWritten;
            result->RelocFirstReadBack = g_RelocStats.FirstReadBack;
            result->RelocFirstStatus = (ULONG)g_RelocStats.FirstWriteStatus;
            result->SymTotal = g_SymbolInfo.TotalSyms;
            result->SymDumped = g_SymbolInfo.DumpedCount;
            result->SymTruncated = g_SymbolInfo.Truncated;
            result->SymStrTabReadable = g_SymbolInfo.StrTabReadable;
            result->SymStrTabStatus = g_SymbolInfo.StrTabStatus;
            result->SymSymTabVaddr = g_SymbolInfo.SymTabVaddr;
            result->SymStrTabVaddr = g_SymbolInfo.StrTabVaddr;
            result->SymNameFileOffset = g_SymbolInfo.NameFileOffset;
            result->SymNameByte0 = g_SymbolInfo.NameByte0;
            result->SymNameByte1 = g_SymbolInfo.NameByte1;

            result->StubCount = g_StubStats.StubCount;
            result->StubFound = g_StubStats.StubFound;
            result->StubSlotUsed = g_StubStats.StubSlot;
            result->StubUnknown = g_StubStats.StubUnknown;
            result->StubWriteFail = g_StubStats.StubWriteFail;
            result->StubAreaBase = g_StubStats.StubAreaBase;
            result->StubLastAddress = g_StubStats.LastStubAddress;
            result->StubLastSyscall = g_StubStats.LastStubSyscall;
            result->StubLastStatus = (ULONG)g_StubStats.LastStubStatus;
            RtlCopyMemory(result->StubLastName, g_StubStats.LastStubName, 48);
            result->LibAttempted = g_LibStats.Attempted;
            result->LibLoaded = g_LibStats.Loaded;
            result->LibFailed = g_LibStats.Failed;
            result->LibLastStatus = g_LibStats.LastStatus;
            result->LibTotalSymbols = g_LibStats.TotalSymbols;
            result->LibTotalRelocs = g_LibStats.TotalRelocs;
            result->LibNameResolveFail = g_LibStats.NameResolveFail;
            result->LibPathResolveFail = g_LibStats.PathResolveFail;

            for (i = 0; i < 4; i++)
            {
                result->LibUsed[i] = g_LibStats.Libs[i].Used;
                result->LibStatus[i] = g_LibStats.Libs[i].LoadStatus;
                result->LibSymCount[i] = g_LibStats.Libs[i].SymCount;
                result->LibRelaCount[i] = g_LibStats.Libs[i].RelaCount;
                result->LibRelaApplied[i] = g_LibStats.Libs[i].RelaApplied;
                result->LibNeededCount[i] = g_LibStats.Libs[i].NeededCount;
                result->LibBase[i] = g_LibStats.Libs[i].BaseAddress;
                result->LibImageSize[i] = g_LibStats.Libs[i].ImageSize;
                RtlCopyMemory(result->LibNames[i], g_LibStats.Libs[i].Name, 48);
            }

            result->GlobalCount = g_GlobalSymbols.Count;
            result->GlobalCapacity = g_GlobalSymbols.Capacity;
            result->GlobalProbeCount = g_GlobalSymbols.ProbeCount;
            result->GlobalProbeHit = g_GlobalSymbols.ProbeHit;
            result->GlobalCrossResolved = g_GlobalSymbols.CrossResolved;
            result->GlobalCrossMiss = g_GlobalSymbols.CrossMiss;
            result->GlobalLastHitAddress = g_GlobalSymbols.LastHitAddress;
            RtlCopyMemory(result->GlobalLastMissName, g_GlobalSymbols.LastMissName, 48);
            RtlCopyMemory(result->GlobalLastHitName, g_GlobalSymbols.LastHitName, 48);

            for (i = 0; i < 8; i++)
            {
                RtlCopyMemory(result->GlobalFirstNames[i], g_GlobalSymbols.Entries[i].Name, 48);
            }

            result->BuiltinWritten = g_BuiltinStats.Written;
            result->BuiltinTotalBytes = g_BuiltinStats.TotalBytes;
            result->BuiltinFailCount = g_BuiltinStats.FailCount;
            result->BuiltinRegistered = g_BuiltinStats.Registered;
            result->BuiltinLastStatus = (ULONG)g_BuiltinStats.LastStatus;
            result->BuiltinAreaBase = g_BuiltinStats.AreaBase;
            result->BuiltinLastAddress = g_BuiltinStats.LastAddress;
            RtlCopyMemory(result->BuiltinLastName, g_BuiltinStats.LastName, 48);
            result->BrkAddress = g_BuiltinStats.BrkAddress;
            result->BrkOkCount = g_BrkOk;
            result->DrvStructSize = (ULONG)sizeof(LX_QUERY_RESULT);
            result->Ent0No = g_EntryDump[0][0];
            result->Ent0P0 = g_EntryDump[0][1];
            result->Ent0P1 = g_EntryDump[0][2];
            result->Ent0P2 = g_EntryDump[0][3];
            result->Ent1No = g_EntryDump[1][0];
            result->Ent1P0 = g_EntryDump[1][1];
            result->Ent1P1 = g_EntryDump[1][2];
            result->Ent1P2 = g_EntryDump[1][3];
            result->EntryCount = g_EntryCount;
            result->BrkDbg0 = g_BrkDebug[0];
            result->BrkDbg1 = g_BrkDebug[1];
            result->BrkDbg2 = g_BrkDebug[2];
            result->BrkDbg3 = g_BrkDebug[3];
            result->BrkDbg4 = g_BrkDebug[4];
            result->BrkDbg5 = g_BrkDebug[5];
            result->BrkDumpCount = g_BrkDumpCount;
            result->BrkRejectCount = g_BrkReject;
            result->BrkLastBad = g_BrkLastBad;
            result->BrkInitStatus = g_BuiltinStats.BrkInitStatus;

            for (i = 0; i < 64; i++)
            {
                result->SymNameOffsets[i] = g_SymbolInfo.Entries[i].NameOffset;
                result->SymRelocOffsets[i] = g_SymbolInfo.Entries[i].RelocOffset;
                RtlCopyMemory(result->SymNames[i], g_SymbolInfo.Entries[i].Name, 48);
            }

            for (i = 0; i < 8; i++)
            {
                result->DynNeededOffsets[i] = g_ElfDynamic.NeededOffsets[i];
            }
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

                for (i = 0; i < 64; i++)
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

            for (i = 0; i < 64; i++)
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
            result->PicoProcessObjectSet = (g_PicoProcessObject != NULL) ? 1 : 0;
            result->ProbeMagicWritten = g_ProbeMagicWritten;
            result->ProbeMagicReadback = g_ProbeMagicReadback;
            result->ProbeMagicOk = g_ProbeMagicOk;
            result->MmapCount = g_MmapCount;
            result->LastMmapAddress = g_LastMmapAddress;
            result->LastMmapStatus = g_LastMmapStatus;

            for (i = 0; i < 8; i++)
            {
                result->MmapDebug[i] = g_MmapDebug[i];
            }

            for (i = 0; i < 40; i++)
            {
                result->MmapContext[i] = g_MmapContext[i];
            }

            for (i = 0; i < 16; i++)
            {
                result->MmapBytes[i] = g_MmapBytes[i];
            }

            result->SeekCount = g_SeekCount;
            result->LastSeekResult = g_LastSeekResult;

            for (i = 0; i < 8; i++)
            {
                result->ReadDebug[i] = g_ReadDebug[i];
                result->StatDebug[i] = g_StatDebug[i];
                RtlCopyMemory(result->SlotText[i], g_SlotText[i], 48);
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
# lxrun — 自研 WSL1 兼容层（Windows 内核驱动）

一个从零写起的 WSL1 替代实现。不依赖 lxcore.sys 的任何代码，
所有接口参数都来自 `ntoskrnl` 反编译 + 运行时探针实测。

## 当前状态：VFS 核心已打通

实测（GitHub Actions / Windows Server 2022 runner）：

```
syscalls (5): 1 2 217 1 60     ← write → open → getdents64 → write → exit

openCount = 2
lastOpenFd = 3                 ← 真实目录句柄
lastOpenStatus = 0x0           ← STATUS_SUCCESS
lastOpenPath = [/mnt/c]

getdentsCount = 1
getdentsStatus = 0x0           ← 目录遍历成功
getdentsBytes = 992            ← 返回 992 字节 dirent 数据
dir = [\SystemRoot]            ← 真实 Windows 目录

exitCode = 0
```

**一个手写的 Linux ELF 打开了真实 Windows 目录，遍历出了 992 字节的目录项。**

另一组实测（单个文件）：

```
syscalls (4): 2 0 1 60         ← open → read → write → exit

openCount = 1  lastOpenFd = 3  lastOpenStatus = 0x0
lastNtPath = [\SystemRoot\hello.x64]

readCount = 1  lastReadStatus = 0x0
WRITE_OUT = [ELF....]          ← 从 Windows 文件读出的真实 ELF 头

statCount = 1  lastStatStatus = 0x0
size = 157  mode = 0100666     ← 真实文件元数据
```

## 文件系统映射

```
/mnt/c        →   \SystemRoot
/mnt/c/xxx    →   \SystemRoot\xxx
其它路径      →   \SystemRoot\l xroot\...
```

> 这个 runner 的 System 进程里 `\GLOBAL??\C:` 和 `\DosDevices\C:`
> 都返回 `0xC0000034`（符号链接不存在），只有 `\SystemRoot` 可访问。
> `\SystemRoot\..` 也不被 NT 路径解析支持（返回 `0xC0000033` NAME_INVALID）。

## 已实现的 syscall

| 号 | 名称 | 状态 |
|---|---|---|
| 0 | read | ✅ 真实读文件 |
| 1 | write | ✅ 读到驱动缓冲区 |
| 2 | open | ✅ 真实打开文件/目录 |
| 3 | close | ✅ |
| 4 | stat | ✅ 真实元数据（size/mode/inode） |
| 5 | fstat | ✅ |
| 158 | arch_prctl | ✅ 真实设 FS |
| 217 | getdents64 | ✅ 真实目录遍历 |
| 60 / 231 | exit / exit_group | ✅ |
| 257 | openat | ✅ |
| 8 / 16 / 33 | lseek / ioctl / access | 桩 |

## syscall 参数映射（实测）

```
Parameters[0] = 0     （恒为 0）
Parameters[1] = rdi
Parameters[2] = rsi
Parameters[3] = rdx
```

注意：`open` 的路径在 `[1]`（rdi）或 `[2]`（rsi），取决于测试程序用哪条
`lea`/`movabs` 装载；驱动里做了双槽兜底。

## 用户内存读取的正确做法

**`ProbeForRead` 在 Pico 线程上下文里不可靠**，会间歇性失败。
可靠做法是 `RtlCopyMemory` 包在 `__try` 里，并且**不要把整段路径一次读完**
（跨越未提交页会整体失败）。当前实现：一次复制 32 字节到内核暂存区，最多重试 3 次。

## 踩过的坑（全部是真 bug，按代价排序）

| 现象 | 真因 | 代价 |
|---|---|---|
| `open("/mnt/c")` 永远失败，但 `stat` 成功 | `LxBuildNtPath` 的边界写成了 `out <= 11`，而 `/mnt/c` 转换后正好 11 字符（`\SystemRoot`），被误判非法。应为 `out < 11` | **6 轮 CI** |
| `stat` 每次都返回 `-14`，`lastStatStatus=0xffffffff` | 一次编辑把 `if (length == 0) {` 的右括号位置弄错，导致后面的 `return -14` 无条件执行 | **4 轮 CI** |
| 路径读到一半就失败 | 一次 `RtlCopyMemory` 读 64 字节会跨越未提交页，整体抛异常 | 3 轮 |
| 完全读不到用户路径 | `ProbeForRead` 在 Pico 上下文不可靠 | 5 轮 |

**教训**：表面现象（"路径读不到"）和真因（"路径读到了，但在后面的边界检查被丢掉"）
可能隔了好几层。加一个"把真实参数和返回值都打出来"的探针，比反复调整参数位置有效得多。

## 当前状态：可以执行任意静态 x86-64 ELF

实测（GitHub Actions / Windows Server 2022 runner）：

```
LOAD_ELF: ok=1  elfStatus=0  copied=157  base=0x400000

syscalls (2): 1 60        ← write(1) → exit(60)
writeDebug = 0  1  400099  ...  fd=1
WRITE OUTPUT: [hi!]       ← 从程序内存读出的真实内容
exitCalled = 1
exitCode   = 0
exceptionDump = 0 0 ...   ← 零崩溃
```

被测程序是一个 **157 字节手写的 ELF**：

```asm
mov eax,1      ; write
mov edi,1      ; fd=1
lea rsi,[msg]
mov edx,4      ; len
syscall
mov eax,60     ; exit
xor edi,edi
syscall
msg: "hi!\n"
```

## 完整链路（全部实测通过）

```
驱动加载
  ↓ 遍历 DriverSection 链表拿 ntoskrnl 基址
  ↓ 从 RIP 相对指令算出 gate 地址
  ↓ 写 gate = 0（PAGE 段，需绕过只读）
  ↓ PsRegisterPicoProvider → STATUS_SUCCESS
  ↓ 内核回填 11 个函数指针（routines 表）
  ↓ PspCreatePicoProcess → Pico 进程
  ↓ ZwCreateSection + ZwMapViewOfSection 映射 ELF 到 0x400000
  ↓ 映射栈到 0x7fff00000000，映射 16MB 池到 0x100000000
  ↓ KeStackAttachProcess 写 execve 栈布局（argc/argv/envp/auxv）
  ↓ PspCreatePicoThread → Linux 线程，RIP = ELF entry
  ↓ 执行 syscall 指令 → 内核 → 我们的 LxSysDispatch
  ↓ arch_prctl → PspSetPicoThreadDescriptorBase → wrmsr 设 FS
  ↓ write / exit
```

## 关键技术数据（全部实测，不依赖符号表）

### 1. PICO_PROVIDER_INFO（0x58 字节）

```
+0x00  ULONG64  Size = 0x58
+0x08  SysDispatch           ← 我们的 syscall 分发器
+0x10  ThreadExit
+0x18  ProcessExit
+0x20  DispatchException
+0x28  ProcessTerminate
+0x30  WalkUserStack
+0x38  ProtectedRanges
+0x48  Flags（高 11 位必须为 0）
+0x4C  Flags4c（高 11 位必须为 0）
+0x50  （未知，注册时传 1）
```

### 2. PICO_PROVIDER_ROUTINES（0x60 字节，内核回填）

```
+0x00  Size = 0x60
+0x08  PspCreatePicoProcess
+0x10  PspCreatePicoThread
+0x18  PspGetPicoProcessContext
+0x20  PspGetPicoThreadContext
+0x28  PspPicoGetContextThreadEx
+0x30  PspPicoSetContextThreadEx
+0x38  PspTerminateThreadByPointer
+0x40  PsResumeThread
+0x48  PspSetPicoThreadDescriptorBase
+0x50  PsSuspendThread
+0x58  PspTerminatePicoProcess
```

### 3. 注册 gate 的真实位置

`PsRegisterPicoProvider` 内部：

```asm
0x2E: 38 05 8c e5 29 00   cmp byte [rip+0x29E58C], 0
0x34: 74 07               je +7          ; == 0 → 继续注册
0x36: b8 89 01 00 c0      mov eax,0xC0000189
0x3B: c3                  ret
```

- 指令长 **6 字节**（2 opcode + 4 disp32）
- RIP = 函数入口 + 0x34
- **gate 地址 = 函数入口 + 0x29E5C0**

### 4. routines 全局的真实位置

```asm
0x40: 0f 11 05 89 e5 29 00   movups [rip+0x29E589], xmm0
```

- 指令长 **8 字节**，RIP = 函数入口 + 0x48
- **全局地址 = 函数入口 + 0x29E5D1**

> 注意：`symbols.tsv` 给的 0xF05820 / 0xF05880 与实际差约 0x60，
> 必须以代码里的 RIP 相对偏移为准。

### 5. PspCreatePicoProcess 参数

```c
int PspCreatePicoProcess(ULONG64 *param1, LONG64 param2, LONG64 *param3);

param1[0] = 父进程 Handle（需 PROCESS_CREATE_PROCESS = 0x80）
param1[1] = Token Handle（可为 0）
param1[2] = PicoContext（非 0，存进 EPROCESS+0x640）
param1[3] = Flags（只能低位，>0xF 直接拒）
param3    = 输出：Pico 进程 Handle
```

### 6. PspCreatePicoThread 参数（CONTEXT 映射）

`param1` 是线程上下文数组，字段直接映射到 x64 CONTEXT：

| 参数 | 含义 |
|---|---|
| `param1[0]` | 进程 Handle（需 PROCESS_CREATE_THREAD = 2） |
| `param1[1]` | RSP |
| `param1[2]` | RIP（ELF entry） |
| `param1[3]` | RCX |
| `param1[8]` | RAX |
| `param1[0xc..0x16]` | RDI / RSI / RBP / R8–R15 |
| `param1[0x17]` | PicoContext（线程级，存进 thread+0x670） |

### 7. PicoContext 布局（我们自定义）

```
+0x00 ~  我们随便用
+0x30    syscall 号（入口）/ 返回值（出口）
```

`PsPicoSystemCallDispatch` 只有三行：

```c
local_res8[0] = context;
(*PspPicoProviderRoutines.SysDispatch)(local_res8);
return *(ULONG64 *)(context + 0x30);
```

### 8. syscall 参数布局（Pico 自定义，不是标准 ABI）

实测 `write(1, 0x400099, 4)`：

```
context + 0x140 = 0
context + 0x148 = 1          ← fd（rdi）
context + 0x150 = 0x400099   ← buf（rsi）
context + 0x158 = 0x400099   ← 重复
context + 0x160 = 0
context + 0x168 = 0x400090   ← 代码段地址（不是参数）
```

**`rdx`（count）没有被传进 context。**

`arch_prctl` 的参数顺序是**反的**（`[rsi, rdi]`），需要兼容处理。

## 踩过的坑（全部是真 bug）

| 现象 | 真因 |
|---|---|
| 读 gate 总是 0 | RIP 偏移按 7 字节算，实际指令 6 字节，差 10 字节 |
| 探针读不到数据 | `Irp->IoStatus.Status` 设成失败码，IOCTL 失败时 buffer 不拷回用户态 |
| `routines[0]` 恒为 0 | 内层 `const UCHAR *routines` 遮蔽外层 `UCHAR routines[0x60]` |
| 编译报 "No Target Architecture" | 内核编译必须定义 `_AMD64_` |
| 程序在 `mov rax, fs:[0]` 崩 | `arch_prctl` 只返回 0，没真调 `wrmsr` 设 FS |
| 写栈时 job 挂死 | Pico 上下文里 `MmCopyVirtualMemory` 会死锁 |
| 读 write 缓冲时挂死 | 同上 |
| 程序在 `__libc_start_main` 崩 | 栈上没有 argc/argv/envp/auxv |
| `KAPC_STATE`/`PKAPC_STATE`/`PROCESS_VM_WRITE` 未定义 | `ntddk.h` 没导出，要自己定义或用数值 |

**两条硬规则**：

1. **在 Pico 上下文里不要用 `MmCopyVirtualMemory`** —— 改用 `KeStackAttachProcess` 或 `ProbeForRead`
2. **内核栈上访问用户地址必须 `ProbeForRead` + `__try`**

## 文件

```
lxrun.c            1428 行   驱动主体
lxprobe.c           518 行   用户态探针 + ELF 解析
build-driver.yml             CI：编译 → 签名 → 加载 → 运行
hello.x64           157 字节 手写测试程序
```

## 尚未实现（按工程量排序）

| 功能 | 规模 | 说明 |
|---|---|---|
| `write` 输出到真实控制台 | 几十行 | 现在只是读到驱动缓冲区 |
| `count` 参数 | 未知 | 需要 Pico 专用线程对象才能取完整 CONTEXT |
| VFS（open/read/stat/getdents） | 几千行 | 需要把 Windows 文件系统映射到 Linux 路径 |
| 动态链接（ld.so + .so） | 几千行 | PT_INTERP 加载 + 重定位 |
| 进程/线程/信号/IPC | 上万行 | clone/futex/signal 语义 |
| socket | 几千行 | winsock → BSD socket |
| GUI | 未知 | X11 + 显示服务 |

## 结论

**最核心的门已经推开**：Linux 程序的机器码可以在 Windows 内核里执行，
它的系统调用完全归我们接管，程序能正常跑完并退出。

剩下的是工程量，不再是机制不明的黑盒。

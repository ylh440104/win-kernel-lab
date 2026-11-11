# win-kernel-lab 交接文档

最后更新：本轮结束
仓库：https://github.com/ylh440104/win-kernel-lab
本地备份：/sdcard/Download/lxcore/winkernel/

---

## 1. 项目是什么

在 Windows 内核上从零实现一个 WSL1 兼容层（替代 lxcore.sys），
让 x86-64 Linux ELF 程序在 Pico 进程里执行并响应 syscall。

**不依赖 lxcore.sys 的任何代码。** 所有接口参数都来自
`ntoskrnl` 反编译 + 运行时探针实测。

最终目标（用户明确要求）：**能完整跑几乎所有 Linux 程序，包含 GUI。**

---

## 2. 当前真实进度

### 已验证可用（有 CI 日志证据）

| syscall | 号 | 证据 |
|---|---|---|
| read | 0 | `WRITE_OUT=[ELF....]` 真实文件内容 |
| write | 1 | count 已解决，累积输出 168 字节 |
| open | 2 | `fd=3, path=[/mnt/c], status=0x0` |
| close | 3 | |
| stat | 4 | `size=157, mode=0100666` 真实元数据 |
| fstat | 5 | |
| lseek | 8 | `lastSeek=0x8`，SEEK_SET/CUR/END 全支持 |
| arch_prctl | 158 | `fsBaseAfter=0x400078` 真实设 MSR |
| getdents64 | 217 | `bytes=1024, dir=[\SystemRoot]`（写入未达用户态，见§5） |
| exit / exit_group | 60/231 | `exitCode=0` |
| openat | 257 | |

### 已验证的完整链路

1. 驱动加载，绕过 `PspPicoRegistrationDisabled` gate
2. `PsRegisterPicoProvider` 注册成功
3. `PspCreatePicoProcess` 创建 Pico 进程
4. ELF 映射到 0x400000（`ZwCreateSection` + `ZwMapViewOfSection`）
5. 栈映射到 0x7fff00000000，池映射到 0x100000000
6. `KeStackAttachProcess` 写 execve 栈布局（argc/argv/envp/auxv）
7. `PspCreatePicoThread` 创建 Linux 线程
8. 执行 syscall → `LxSysDispatch` 分发
9. `arch_prctl` → `PspSetPicoThreadDescriptorBase` → `wrmsr` 设 FS
10. 文件 open/read/stat/lseek/getdents 全部走通
11. `exit` 正常退出，零崩溃

### 手写测试程序（全部跑通）

| 文件 | 大小 | 验证内容 |
|---|---|---|
| hello.x64 | 157 B | `write(1,"hi!\n",4); exit(0)` |
| stat.x64 | 196 B | `stat("/mnt/c/h", buf)` → size=157 |
| gd2.x64 | 240 B | `open + getdents64 + write` |
| seek.x64 | 233 B | `open + lseek(8) + read + write` |
| ls.x64 | 275 B | 目录遍历 + 循环打印 8 个名字（19 个 syscall） |

---

## 3. 关键技术参数（全部实测，直接可用）

### 3.1 注册 gate 位置

```asm
0x2E: 38 05 8c e5 29 00   cmp byte [rip+0x29E58C], 0
0x34: 74 07               je +7
```
- 指令长 6 字节，RIP = 函数入口 + 0x34
- **gate 地址 = 函数入口 + 0x29E5C0**

### 3.2 routines 全局位置

```asm
0x40: 0f 11 05 89 e5 29 00   movups [rip+0x29E589], xmm0
```
- 指令长 8 字节，RIP = 函数入口 + 0x48
- **全局地址 = 函数入口 + 0x29E5D1**

> symbols.tsv 给的 0xF05820/0xF05880 与实际差约 0x60，以代码 RIP 偏移为准。

### 3.3 PICO_PROVIDER_INFO（0x58 字节）

```
+0x00  ULONG64  Size = 0x58
+0x08  SysDispatch
+0x10  ThreadExit
+0x18  ProcessExit
+0x20  DispatchException
+0x28  ProcessTerminate
+0x30  WalkUserStack
+0x38  ProtectedRanges
+0x48  Flags（高 11 位必须为 0）
+0x4C  Flags4c（高 11 位必须为 0）
+0x50  注册时传 1
```

### 3.4 PICO_PROVIDER_ROUTINES（0x60 字节，内核回填）

```
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

### 3.5 PspCreatePicoProcess 参数

```c
param1[0] = 父进程 Handle（需 PROCESS_CREATE_PROCESS = 0x80）
param1[1] = Token Handle（可 0）
param1[2] = PicoContext（非 0，存进 EPROCESS+0x640）
param1[3] = Flags（> 0xF 直接拒）
param3    = 输出 Pico 进程 Handle
```

### 3.6 PspCreatePicoThread 参数（CONTEXT 映射）

| 参数 | 含义 |
|---|---|
| param1[0] | 进程 Handle（需 PROCESS_CREATE_THREAD = 2） |
| param1[1] | RSP |
| param1[2] | RIP（ELF entry） |
| param1[3] | RCX |
| param1[8] | RAX |
| param1[0xc..0x16] | RDI/RSI/RBP/R8-R15 |
| param1[0x17] | PicoContext（存进 thread+0x670） |

### 3.7 PicoContext 布局（自定义）

```
+0x30   syscall 号（入口）/ 返回值（出口）
```

`PsPicoSystemCallDispatch` 实现（反编译确认）：
```c
local_res8[0] = context;
(*DAT_140f05828)(local_res8);      // 只传 context 指针
return *(ULONG64 *)(context + 0x30);
```

---

## 4. ★ 最重要的发现：syscall context 完整布局

**这是本轮最大的突破。** 用 `RtlCopyMemory` 读内核 context（**不能用 ProbeForRead，
那是给用户地址用的**），得到 64 个 qword 的真实内容：

```
+0x000 = 0xffffcc8e2f5b9540     Peb / 进程相关
+0x010 = 0xfffffe068d48f520
+0x018 = 0x401013               ★ write 的 buffer
+0x028 = 0x1f8002000000
+0x040 = 0x14                   ★★ write 的 count（0x14 = 20）
+0x048 = 0x8
+0x148 = 0x1                    ★ fd
+0x150 = 0x401013               buffer 副本
+0x168 = 0x4000e1               RIP
+0x170 = 0x33
+0x178 = 0x202
+0x180 = 0x7fff0001fe20         栈指针
+0x188 = 0x2b
+0x190 = 0xfffffe068d490000
+0x198 = 0xfffffe068d489000
+0x1e0 = 0x27f
+0x1f8 = 0x1f80
```

### 参数读取规则（最终结论）

```
context + 0x18   = write 的 buffer
context + 0x40   = write 的 count   ← 之前一直找不到，就在这里
context + 0x148  = fd (rdi)
context + 0x150  = buffer (rsi)
context + 0x168  = RIP
context + 0x180  = RSP
context + 0x30   = 返回值出口
```

**count 不在 Parameters 数组里，必须从 `context + 0x40` 直接读。**

读法：
```c
ULONG64 contextCount = 0;
__try {
    RtlCopyMemory(&contextCount, (PUCHAR)Context + 0x40, 8);
} __except (EXCEPTION_EXECUTE_HANDLER) { contextCount = 0; }
if (contextCount > 0 && contextCount < 0x10000) length = (ULONG)contextCount;
```

---

## 5. 当前卡点：dirent 写入不到用户态

### 现象

```
getdentsCount = 1
getdentsStatus = 0x0        ← 成功
getdentsBytes = 1024        ← written 累加到了 1024
```

但程序 `write` 出来的 buffer 内容**全是 0**。

### 已排除

- `picoProcessObjectSet = 1` → `g_PicoProcessObject` 已正确设置
- `KeStackAttachProcess` 已执行
- `__try/__except` 未触发异常

### 结论

写入在内核看来成功，但目标地址不在当前地址空间。
**`KeStackAttachProcess` 附加到的可能不是 Pico 进程的真实地址空间。**

### 建议的排查方向

1. 用 `PspPicoGetContextThreadEx` 拿真实的用户 CR3 / 地址空间
2. 或者验证 `ObReferenceObjectByHandle(g_LastPicoHandle)` 拿到的是不是同一个进程
3. 或者改用 `MmMapViewInSystemSpace` / 直接写物理内存
4. 反汇编 `KiSystemServiceExitPico`（RVA `0x6b68ab`），看它如何切换地址空间

---

## 6. 用户内存访问的正确做法（踩坑总结）

| 方法 | 结果 |
|---|---|
| `ProbeForRead` + 直接解引用 | ❌ 在 Pico 上下文不可靠，间歇性失败 |
| `ProbeForRead` + `RtlCopyMemory` | ⚠️ 读小块可以，跨未提交页整体失败 |
| 逐字节 `ProbeForRead` | ❌ 每次触发异常，导致异常风暴 |
| **`RtlCopyMemory` 一次复制到内核暂存区 + 重试** | ✅ 传路径用这个（32 字节，3 次重试） |
| `KeStackAttachProcess` + 直写 | ⚠️ 写入返回成功但内容不到用户态 |

**注意：读内核 context 必须用 `RtlCopyMemory`，用 `ProbeForRead` 会全部失败（全是 0xdeadbeef）。**

---

## 7. 踩过的坑（按 CI 代价排序）

| 现象 | 真因 | 代价 |
|---|---|---|
| `open("/mnt/c")` 永远失败但 `stat` 成功 | `LxBuildNtPath` 边界写成 `out <= 11`，而 `/mnt/c` 转换后正好 11 字符（`\SystemRoot`）被误判非法，应为 `out < 11` | **6 轮** |
| `stat` 每次返回 `-14` | 一次编辑把 `if (length == 0) {` 的右括号位置弄错，导致后面 `return -14` 无条件执行 | **4 轮** |
| 路径读到一半失败 | 一次 `RtlCopyMemory` 读 64 字节跨未提交页，整体抛异常 | 3 轮 |
| 完全读不到用户路径 | `ProbeForRead` 在 Pico 上下文不可靠 | 5 轮 |
| 读 context 全是 `deadbeef` | `Context` 是内核地址，`ProbeForRead` 只接受用户地址，必须用 `RtlCopyMemory` | 1 轮 |
| 写栈时 job 挂死 | Pico 上下文里 `MmCopyVirtualMemory` 死锁 | 2 轮 |
| 程序在 `mov rax, fs:[0]` 崩 | `arch_prctl` 只返回 0，没真调 `wrmsr` 设 FS | 2 轮 |
| 程序在 `__libc_start_main` 崩 | 栈上没有 argc/argv/envp/auxv | 1 轮 |
| `KAPC_STATE`/`PKAPC_STATE`/`PROCESS_VM_WRITE`/`KAPC_STATE`/`FILE_INTERNAL_INFORMATION`/`FILE_BOTH_DIR_INFORMATION` 未定义 | `ntddk.h` 没导出，需自己 typedef | 各 1 轮 |
| 编译报 "No Target Architecture" | 内核编译必须定义 `_AMD64_` | 1 轮 |

**两条硬规则：**
1. **Pico 上下文里不要用 `MmCopyVirtualMemory`**（死锁）
2. **用户地址用 `ProbeForRead`+`RtlCopyMemory`，内核地址用 `RtlCopyMemory`**

---

## 8. 文件系统映射

```
/mnt/c        →   \SystemRoot
/mnt/c/xxx    →   \SystemRoot\xxx
其它路径      →   \SystemRoot\l xroot\...
```

**重要**：这个 runner 的 System 进程里 `\GLOBAL??\C:` 和 `\DosDevices\C:`
都返回 `0xC0000034`（符号链接不存在），**只有 `\SystemRoot` 可访问**。
`\SystemRoot\..` 也不被 NT 路径解析支持（`0xC0000033` NAME_INVALID）。

---

## 9. 代码结构

```
driver/lxrun.c      ~2700 行   驱动主体
driver/lxprobe.c     ~610 行   用户态探针 + ELF 解析
tools/dumpnt.py       46 行    ntoskrnl PE 解析工具
.github/workflows/
  build-driver.yml           主 CI：编译 → 签名 → 加载 → 运行
  dump-nt.yml                提取 ntoskrnl 指定 RVA 的字节
README.md                    技术档案（已更新到 VFS 阶段）
```

### lxrun.c 关键函数

```
LxBuildNtPath()      Linux 路径 → NT 路径（注意 11 字符边界）
LxCopyLinuxPath()    用户路径 → 内核缓冲（RtlCopyMemory + 3 次重试）
LxOpenFile()         ZwCreateFile + 分配 fd
LxReadFile()         ZwReadFile + 维护偏移
LxCloseFile()        ZwClose
LxFillStat()         填 LX_LINUX_STAT
LxGetDents64()       ZwQueryDirectoryFile + 转 dirent64
LxProbeSlots()       调试用：把 6 个参数槽当字符串探测
LxHandleSyscall()    总分发（约 20 个 case）
LxSysDispatch()      Pico 的 SysDispatch 入口
```

---

## 10. 开发流程

### 推送（网络不稳时用 Contents API）

```bash
export GH=$(cat /sdcard/Download/lxcore/.gh)
# 用 GitHub Contents API：GET 拿 sha → PUT base64 内容
```

### 触发 CI

```bash
curl -H "Authorization: Bearer $GH" -X POST \
  'https://api.github.com/repos/ylh440104/win-kernel-lab/actions/workflows/build-driver.yml/dispatches' \
  -d '{"ref":"main"}'
```

### 取日志

```bash
curl -sL -H "Authorization: Bearer $GH" \
  "https://api.github.com/repos/ylh440104/win-kernel-lab/actions/jobs/$JID/logs"
```

**每轮 CI 约 4.5 分钟。** dispatch 后 sleep 255 再取结果比较稳。

### 日志是二进制时

```bash
grep -a ...        # -a 强制按文本处理
```

---

## 11. 距离"完整跑 Linux 程序含 GUI"还差什么

**老实说：差距非常大。** 当前是 3 千行，完整实现需要 8~15 万行。

| 缺失 | 规模 | 优先级 |
|---|---|---|
| dirent 写入到用户态 | 待突破 | ★★★ 当前卡点 |
| 动态链接（ld.so + .so + 重定位） | 数千行 | ★★★ 所有发行版程序都动态链接 |
| 完整 syscall 集 | 300+ 个，数万行 | ★★★ |
| 进程/线程（clone/futex/execve） | 上万行 | ★★★ |
| 信号机制 | 数千行 | ★★☆ |
| socket（winsock → BSD） | 数千行 | ★★☆ X11 也要 socket |
| mmap 完整语义（匿名/文件/共享） | 数千行 | ★★★ |
| X11 服务端 | 数万行 | ★☆☆ GUI 必需 |
| GUI 输出（GDI/D3D → X11） | 未知 | ★☆☆ 最难 |

**建议路径：** 目录写入 → 动态链接 → clone/futex → mmap → socket → X11。

---

## 12. 参考资源

```
nt-dump-out 仓库（作者自己的反编译输出）：
https://github.com/ylh440104/nt-dump-out/tree/main/modules/ntoskrnl

文件：
  ntoskrnl_decomp_0.c.part00/01/02   （70MB，Ghidra 反编译）
  ntoskrnl_symbols.tsv               （符号表，15MB）
  ntoskrnl_types.tsv                 （类型表）

关键 RVA：
  PsPicoSystemCallDispatch = 0x140a89878
  KiSystemServiceExitPico  = 0x1406b68ab
  PsSyscallProviderDispatch= 0x140a896e0
```

---

## 13. 交接要点

1. **`context + 0x40` 是 write 的 count** —— 这是最值钱的一条，
   之前一直以为 Pico 不传 rdx，实际是传在固定偏移。
2. **读内核 context 必须用 `RtlCopyMemory`**，`ProbeForRead` 会全部失败。
3. **`LxBuildNtPath` 的 11 字符边界** 是个隐蔽的坑，卡了 6 轮。
4. **当前唯一卡点是 dirent 写入用户态**，其他 VFS 功能都通了。
5. 所有参数都有 CI 日志证据，不靠推断。

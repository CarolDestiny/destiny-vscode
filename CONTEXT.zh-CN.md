# Destiny Core 领域词汇

本文是项目在 `code/core` 设计和实现时使用的中文术语基线。`core` 位于 `basicType` 之上，提供错误、日志、诊断、内存和线程能力；项目不使用 C++ 异常。

## 基础身份

**TypeCode（类型编码）**：
由两个 `uint32_t` 组成的 64 位统一身份。高 32 位是 category（类别），低 32 位是 value（类别内值）。文件类型、记录类型、区段类型、错误码和未来数据类型都使用它。

_避免：为每个模块发明一套互不兼容的编码。_

**errorCode（错误码）**：
TypeCode 在错误领域的语义。错误码由 `error` 模块定义为编译期常量；它不是单独的编码类型。

_避免：异常码、错误字符串、运行时动态注册错误码。_

**LogIndex（日志序号）**：
进程内从 `0` 开始、在 `write()` 时原子分配的 `uint32_t` 逻辑序号。磁盘固定写成 8 字节，低 32 位有效。空洞表示日志丢失，不是诊断序号。

**Destiny 文件身份**：
由文件头中的 magic 和 fileTypeCode 组成。`log.destiny`、`index.destiny`、`slow.destiny` 是可读文件名，不是唯一身份本身。

## 错误和状态

**错误（Error）**：
C 风格的失败值、错误码和诊断状态。错误发生后系统进入慢车道以维持结构，不抛出 C++ 异常。

_避免：异常系统、throw、catch。_

**错误等级查询**：
由 `error` 的独立查询头文件提供隐藏函数，把 TypeCode 解析为 `destiny::detail` 内部的 `uint8_t` 等级 `0..4`；查询结果使用有符号值，未知码返回 `-1`。error 只报告未知码，提升为 warning 由 log 执行；外部不能直接设置日志等级。

**慢车道（Slow lane）**：
正常队列、内存或持久化路径失败后，为维持结构完整而采取的降级路径。日志慢车道加锁写 `slow.destiny`，不等待有界队列。

**早期 writer（Early writer）**：
core 未完全启动时提供的最小日志写入器。它与 slow writer 行为相同，同样加锁写 `slow.destiny`，不创建 `early.destiny`。

**先记录、后处理**：
任何错误先完成日志持久化，再向 error 事件入口投递；错误维护线程不能在记录落盘前执行恢复动作。

## 线程和诊断

**Destiny 线程（Destiny thread）**：
由 thread 模块创建、跟踪和汇合，并拥有编译期固定、名称固定槽位的线程。

_避免：让外部任意声明新的 thread-local 槽位。_

**线程类别（Thread class）**：
线程的语义角色：`heavy`（重型）、`normal`（普通）和 `light`（轻型）。第一版不把物理核心 id 暴露给调用方；heavy 只表达性能优先语义。

**固定槽位（Fixed slot）**：
编译期确定数量和名字的线程上下文位置。它是项目对高访问延迟 `thread_local` 的替代方案；thread 对外提供固定名字，其他模块可以把槽位中的 `void*` 解释为自己的上下文类型。

**diagnostic（诊断）**：
独立的上下文采集模块，发行版收集必要信息，调试构建收集更多信息。它依赖 thread 槽位和 memory 契约，不依赖 log、不创建线程。

**diagnostic_context（诊断上下文槽位）**：
由 thread 提供给 diagnostic 使用的固定槽位。Log 构造时读取它并复制诊断字符串。

## 内存

**分配句柄（Allocation handle）**：
memory 返回的类型，除地址外隐藏 owner、generation、大小、对齐、池 id 和释放路径。

**分配模式（Allocation mode）**：
一次分配的生命周期契约：`owner_local`（所有者本地）、`transferable`（可转移）或 `shared`（共享）。默认协议是申请、释放和访问由同一线程完成。

**预申请（Preallocation）**：
工作线程提前请求维护线程准备内存的操作。返回外部不可见 token；槽位不足返回内部 id 为 `-1` 的无效 token，兑现未准备好时降级为普通申请。

**维护 worker（Maintenance worker）**：
memory 独立申请的 `light` 线程，负责预申请、空闲回收、泄露检查和所有权账本维护。它在 memory 关闭前完成排空。

## 日志对象和协议

**Log（日志对象）**：
`destiny::Log` 的一次可移动 builder value。无参构造时自动采集 diagnostic；调用方通过 `setErrorCode(uint64_t)` 和 `append(const std::string&)` 填充数据，最后调用 `write()`。它不拥有队列、线程或文件，不支持复制和链式调用。

**日志运行时（Log runtime）**：
log 模块内部的队列、一个 light worker、writer、index 和 recovery。它接收 Log 的独立快照并决定正常或慢车道。

**正常队列（Normal queue）**：
非阻塞 MPMC 队列。生产者只调用 `try_push`；满时立即进入 slow writer。v0 固定一个 light 消费者。

**日志记录（Log record）**：
落盘的结构化事件，包含 DestinyRecordCode、recordLength、LogIndex、错误发生时间和一个 diagnostic/user 区段。

**Log 区段协议（提交格式）**：
当前 Log 数据在提交时使用的区段格式：

```text
[sectionType:uint64][sectionLength:uint64]
[diagnosticLen:uint64][userLen:uint64]
[diagnostic bytes][user bytes]
```

`sectionLength = 16 + diagnosticLen + userLen`。diagnostic 和 user 都按长度复制；v0 不解析 user、不验证 UTF-8、不使用 JSON。

**记录协议（Record protocol）**：

```text
[DestinyRecordCode:uint64][recordLength:uint64]
[LogIndex:uint64][errorTime:uint64]
[Log section]
```

`recordLength` 是其后所有字节的长度，不包含前 16 字节记录头。

**索引项（Index slot）**：
`index.destiny` 中固定 8 个 `uint64_t` 的 64 字节项：LogIndex、errorTime、level、logOffset、recordLength 和三个保留槽位。它是由文件头和固定大小识别的布局例外，不再包一层额外的 TypeCode/长度头。只索引 `log.destiny`，不索引 `slow.destiny`。

**恢复（Recovery）**：
独立模块负责检查文件头、长度、尾部完整性、index 对齐和从 `log.destiny` 重建 index。它不负责错误等级解析或恢复策略。

## 接缝和生命周期

**契约接缝（Contract seam）**：
core 内部的最小 non-owning port，用于反转模块依赖，例如 AllocatorPort、ExecutorPort、ThreadContextPort、ErrorSinkPort、ErrorLevelQueryPort 和 FileAppendPort。

**onload/unload**：
core 的统一启动和关闭函数。`core::onload()` 只返回 `bool`；启动失败先写 early/slow 日志并反序回滚。关闭时 log、error、thread 先停止产生工作，memory 最后收口所有权和泄露账本，early/slow writer 最后关文件。

**慢车道 warning**：
不是修改原始 Log 的等级，而是新增一条 warning Log。例如队列满时原始记录和 queue-failure warning 都写入 `slow.destiny`；未知错误码时原始 warning、unknown-code warning 和必要的 queue-failure warning 按顺序处理。

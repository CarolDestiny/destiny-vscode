# `log.hpp` 对外接口

本文冻结 `code/core/log/log.hpp` 面向项目调用方的接口和语义。它描述准备实现的接口，不在本次修改中改动 `log.hpp`。

## 1. 对外只有一条记录类型

`destiny::Log` 表示一次待提交的日志记录。它是可移动、不可复制的 builder（构造器）对象：调用方设置错误码、追加用户文本、显式调用 `write()`；队列、worker、文件、锁、内存池、错误等级查询和恢复均是 log 模块的私有实现。

这条接缝应保持很小。调用方不需要知道记录将进入 `log.destiny`、`slow.destiny` 还是启动期写入器，也不能根据返回值依赖某条内部路由。

```cpp
#pragma once

#include <cstdint>
#include <string>

namespace destiny::core::log {

bool onload() noexcept;
void unload() noexcept;

} // namespace destiny::core::log

namespace destiny {

class Log final {
public:
    Log() noexcept;
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
    Log(Log&&) noexcept;
    Log& operator=(Log&&) noexcept;
    ~Log() noexcept;

    void setErrorCode(std::uint64_t code) noexcept;
    void append(const std::string& text) noexcept;
    void write() noexcept;
};

} // namespace destiny
```

`onload()` 和 `unload()` 保持既有的 `destiny::core::log` 命名；日志记录类型和所有其他公开函数属于 `destiny`。不公开的函数、常量、队列、writer 和 provider 都在 `destiny::detail`。

## 2. 方法契约

| 方法 | 调用方可见语义 | 不承诺的实现细节 |
| --- | --- | --- |
| `Log()` | 创建一条日志对象，自动采集当前 diagnostic 上下文，并设置非 `0,0` 的未知默认 TypeCode | 采集哪些诊断字段、使用哪个线程槽位、内部存储布局 |
| `setErrorCode(uint64_t)` | 设置该记录的 TypeCode 错误码；可多次调用，以最后一次为准 | 日志等级、错误恢复策略、错误码文字 |
| `append(const std::string&)` | 复制 UTF-8 短文本到 user 区段；可多次调用，空文本合法 | JSON 解析、用户文本验证、字段编码、外部字符串生命周期 |
| `write()` | 在调用当刻拍摄记录快照，采集错误时间和 LogIndex，并把快照提交给日志管线 | 队列是否接收、写入哪个文件、worker 是否已完成持久化、错误处理结果 |

接口不提供 `Level`、`WriteResult`、`field`、`message`、`std::source_location` 参数、链式返回值或公开的查询方法。等级是 error 模块对 TypeCode 的解析结果，不由调用方手工设定；自动上下文由 diagnostic 模块采集，不要求每个调用点传入源位置。

## 3. 基本调用方式

```cpp
destiny::Log record;
record.setErrorCode(allocation_failed_code);
record.append("fixed block allocation failed");
record.write();
```

`append()` 没有链式返回值。调用方应保留清晰的逐步构造形式，避免把格式化、内存申请和日志提交隐藏在表达式链中。

TypeCode 在逻辑上由两个 `uint32_t` 组成：高 32 位是类别，低 32 位是类别内编号。`setErrorCode()` 接受已打包的 `uint64_t`，内部再拆成两个 `uint32_t`；调用方不应自行猜测未知默认码的具体值。

## 4. 对象状态、移动和重复操作

一个 `Log` 的通常用途是一条错误记录。它不提供复制，以免两个对象在不知情的情况下复制并写入同一份数据；它可以移动，以便在容器、任务或函数返回值中转移构造中的记录。

Log 不承诺并发修改。一个对象在同一时刻只能由一个线程调用 `setErrorCode()`、`append()` 或 `write()`；跨线程移动后，诊断上下文仍表示最初构造该对象时采集到的信息，而不是接收线程的上下文。

v0 不把构造后的再次修改变成代码级错误。`write()` 必须先复制一个独立快照进入管线，因此之后修改对象不能改写已提交的记录。调用方若再次 `write()`，实现按当时快照处理；不以断言、异常或未定义行为惩罚这种用法。由调用方负责避免无意义的重复日志。

移动后的对象只保证可析构和可重新赋值；调用方不应再依赖其先前内容。

## 5. diagnostic 与 user 的分区

无参构造时，Log 调用独立的 `diagnostic` 模块。diagnostic 通过 thread 的编译期固定 `diagnostic_context` 槽位和 memory 契约收集必要信息：发行版采集最低诊断内容，调试构建可追加更多上下文。

之后 `append()` 的文本只属于 user 区段。两者不使用 `user:`、`debug:` 等可与用户文本冲突的前缀，也不引入 JSON。提交时，Log 将两段字符串格式化成一个明确的区段：

```text
[sectionType:uint64]
[sectionLength:uint64]
[diagnosticLen:uint64]
[userLen:uint64]
[diagnostic bytes]
[user bytes]
```

```text
sectionLength = 16 + diagnosticLen + userLen
```

读取器可以依据两个长度无歧义地分离文本。v0 不验证 UTF-8，也不解析 user 文本；它只复制字节。因此用户数据即使看起来像 diagnostic 标记，也不会被错误解释。

## 6. 存储、分配和启动期

Log 在逻辑上保存 `TypeCode`、diagnostic 字符串和 user 字符串。`append()` 传入的 `std::string` 只在调用期间借用，Log 必须复制内容，不能保存调用方的指针、引用或 `c_str()`。

正常运行时，字符串增长、队列信封和格式化缓冲由 memory 模块的分配契约提供，以便统一管理和提高效率。这个依赖不能破坏启动期记录，因此实现必须有私有的 bootstrap（引导）存储路径：

- memory 已就绪时，使用 memory 的正常分配路径；
- memory 尚未就绪、正在关闭或普通分配失败时，不递归申请日志内存；
- 可保存的数据直接交给加锁的 slow writer，或者用预留的最小缓冲构造一条故障记录。

这是一处受控的启动期例外，不使 `log` 静态依赖 memory 的具体实现。memory 失败时，日志不能为记录失败再次无限申请内存；它进入慢车道。

## 7. 写入时间和 LogIndex

`write()` 发生时立即获得两个记录元数据：

- `LogIndex`：进程内 `uint32_t` 单调序号，从 `0` 开始原子分配；
- `errorTime`：调用 `write()` 时采集的 UTC Unix 秒。

磁盘上的 LogIndex 固定写成 `uint64_t`，低 32 位有效。序号空洞只代表对应日志丢失，不是错误诊断计数。v0 不预期回绕，但回绕不能使进程崩溃；未来解析器负责处理可能变复杂的排序。

在正常路径，worker 只消费已经拥有 LogIndex、errorTime 和独立文本快照的信封。这样队列满后改走慢车道也能使用同一条记录身份和同一错误发生时间。

## 8. 等级解析和未知错误码

日志等级只由 error 模块的隐藏查询函数解析。等级常量是 `destiny::detail` 内的 `uint8_t` 值，查询函数用有符号返回值承载未知哨兵：

```text
0 = trace（追踪）
1 = debug（调试）
2 = info（信息）
3 = warn（警告）
4 = error（错误）
-1 = unknown（未知错误码）
```

外部不能直接读取或设定这些值。日志 worker 在收到正常队列的记录后才查询等级，因此生产者路径不承担查询成本。

未知码属于代码逻辑错误，但处理策略是可恢复的：

1. 将原始 Log 以 warning 等级写入；原始错误码保持不变。
2. 生成一个新的 unknown-code warning，其中复制完整的原始 Log 数据，而不是保存对原对象的引用。
3. 新 warning 先尝试投回正常 MPMC 队列。
4. 若该次投递失败，先把 unknown-code warning 写入 `slow.destiny`，成功后再写入 queue-failure warning。

这样原始错误、未知码诊断和队列故障都有独立记录，且不会形成引用悬挂或无限递归。

## 9. 队列满、slow writer 和 early writer

正常队列是非阻塞 MPMC。`Log::write()` 只调用 `try_push`；失败只表示队列已满，不等待生产者、不重试阻塞 `push`。

队列满时有两个独立动作：

1. 原始 Log 快照直接由 slow writer 加锁写入 `slow.destiny`，内容和错误码不被伪造或覆盖；
2. 日志管线失败本身形成一个新的 warning Log；它携带原始 Log 的独立数据副本，直接写入 `slow.destiny`。

也就是说，队列故障不会修改原始 Log 的等级或错误码；“升级”是新增一条描述日志管线故障的 warning 记录。该 warning 的错误码由 error 模块定义，未来解析器据此得到 warning 等级。

early writer 与 slow writer 使用同一个加锁写入行为和同一个文件 `slow.destiny`。它们不创建 `early.destiny`，也不索引 slow 文件。slow writer 若连最小记录也无法写入，先尝试写入一条最小 error 记录，随后保存无分配的失败状态并进入 `core::unload()`，不得继续假设日志系统可用。

## 10. 落盘格式与索引关系

每个 UTC 日期目录含 `log.destiny`、`index.destiny` 和 `slow.destiny`。文件开头都是：

```text
[magic:uint64]       # 写入“destiny”的固定 64 位 magic
[fileTypeCode:uint64]
```

`fileTypeCode` 的高 32 位标识文件类别，低 32 位区分具体文件；具体数值由 `TypeCode` 定义阶段冻结，Log 接口不暴露这些数值。

正常记录和慢车道记录使用相同格式，其中 `DestinyRecordCode` 是本条记录的 TypeCode；错误日志中承载 errorCode：

```text
[DestinyRecordCode:uint64]
[recordLength:uint64]

[LogIndex:uint64]
[errorTime:uint64]
[sectionType:uint64]
[sectionLength:uint64]
[diagnosticLen:uint64]
[userLen:uint64]
[diagnostic bytes]
[user bytes]
```

`recordLength` 不包含最前面的 16 字节记录头，保护其后的所有字节；它必须与实际后续长度严格一致。`slow.destiny` 只追加该格式，不进入 `index.destiny`。

`index.destiny` 只索引正常 `log.destiny`。每项固定 64 字节，等于 8 个 `uint64_t`：LogIndex、errorTime、level、logOffset、recordLength 和三个保留扩展槽位。index 项由文件头和固定大小识别，是固定布局例外，不再包一层额外的 TypeCode/长度头。index 的职责是为后续按序号、时间和等级查询提供快速信息，不取代数据文件，也不在 v0 提供查询接口。

## 11. 生命周期约束

`destiny::core::log::onload()` 由 core runtime 调用。它先通过 early writer 具备最小落盘能力，随后在 thread、memory、error 契约可用时启动 MPMC 队列、一个 `light` worker、正常 writer、index writer 和 recovery。

关闭时，normal log 先停止接收并排空队列；无法进入队列的记录继续交给 slow writer。日志持久化完成后才投递 error 事件，满足“先记录、后处理”。early/slow writer 是 core 最后关闭的日志路径，因此不应该让调用方持有任何文件或 worker 句柄。

## 12. 实现时的验收点

- `log.hpp` 不 include `<fstream>`、`<mutex>`、`<atomic>`、队列实现或具体 memory/thread 头文件。
- `Log` 构造不需要错误码、等级、源位置或其他调用方上下文参数。
- `append(const std::string&)` 复制文本；源字符串销毁或修改后，已提交记录保持不变。
- `write()` 不阻塞等待正常队列，且每次提交有独立的 LogIndex 和 errorTime 快照。
- 队列满时原始记录与 queue-failure warning 都进入 `slow.destiny`；不会新建 early 文件。
- 未知错误码按 warning 处理，且 secondary warning 包含独立的原始 Log 数据副本。
- 正常日志只由一个 light worker 消费；正常写入完成后才交给 error 处理。
- `slow.destiny` 与 `log.destiny` 格式相同但不索引；UTC 日目录恢复可从 `log.destiny` 重建 index。
- 所有公开调用都不抛出 C++ 异常；内存失败走慢车道或最终 core 卸载路径。

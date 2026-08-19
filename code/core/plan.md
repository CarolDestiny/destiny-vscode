# Core 开工与验收计划

状态：架构决策已冻结，源码尚未按本文实施。

范围：`code/core` 及其内部模块 `error`、`diagnostic`、`log`、`memory`、`thread`。第一验证平台为 Windows；平台接缝覆盖 Windows、Linux、macOS。本文只给出实施顺序、阶段出口、验收证据和备份节点，不在本轮修改 `.hpp`、`.cpp`、`CMakeLists.txt` 或队列实现。

## 1. 不可改变的基线

- `core` 位于 `basicType` 之上；项目禁止 C++ 异常。
- 所有持久化身份使用 `TypeCode`：高 32 位 category，低 32 位 value，磁盘固定为一个 `uint64_t`；记录采用 little-endian，小端字节序。
- `core::onload()` 只返回 `bool`；内部错误先写 early/slow，再反序回滚。
- 公共日志类型是 `destiny::Log`，无参构造、可移动不可复制；公开方法只有 `setErrorCode(uint64_t)`、`append(const std::string&)`、`write()` 和特殊成员。
- 等级由 error 查询 TypeCode 得出，外部不能设置等级；未知码返回 `-1`，由 log 按 warning 处理并生成独立的 unknown-code warning。
- 日志查询声明集中在 error 的独立头文件；v0 只实现等级查询，未来查询接口不扩散到 `destiny::Log`。
- 正常队列是非阻塞 MPMC，固定一个 log `light` worker；`try_push` 失败立即走 slow writer。
- early writer 与 slow writer 使用相同的加锁行为，均写 `slow.destiny`，不创建 early 文件。
- 日期目录使用 UTC；`log.destiny`、`index.destiny`、`slow.destiny` 的文件头和记录格式以架构文档为准。
- `LogIndex` 逻辑类型是从 0 开始的 `uint32_t`，磁盘固定 8 字节，低 32 位有效；index 项是固定布局例外，不加额外项头。
- Log 只复制 diagnostic/user 文本，不验证 UTF-8、不解析 user、不引入 JSON；提交时格式化 Log 区段。
- memory 提供固定块、变长块、Allocation 句柄、预申请和维护 worker；预申请槽位不足返回内部 id 为 `-1` 的无效 token，兑现未准备好时降级普通申请。
- thread 提供 `heavy`、`normal`、`light` 语义类别和编译期固定命名槽位；固定槽位替代高访问延迟的 `thread_local`，第一版不绑定物理核心。
- memory 的所有权在关闭阶段最后收口；early/slow writer 是最后关闭的文件路径。

## 2. 阶段总览

| 阶段 | 目标 | 主要依赖 | 阶段出口 |
| --- | --- | --- | --- |
| P0 | 文档、TypeCode 和契约冻结 | `basicType` | 架构评审通过，编码表无重复 |
| P1 | error 核心与早期文件路径 | TypeCode、平台文件适配器 | core 未完全启动也能加锁写 `slow.destiny` |
| P2 | thread 与固定上下文 | P1、平台线程适配器 | 三类线程和槽位测试通过 |
| P3 | diagnostic | P2、memory 契约草案 | Log 构造可获得发行/调试两种诊断快照 |
| P4 | memory 后端与预申请 | P2、P1 | 固定/变长申请、回收、泄露和 token 测试通过 |
| P5 | Log 纯内存 builder | P1、P3、memory port | 无文件 fake sink 下接口语义稳定 |
| P6 | 正常队列与 log worker | P2、P4、P5 | 非阻塞 MPMC、等级查询和 warning 顺序成立 |
| P7 | 文件格式、index 与 recovery | P1、P6 | 损坏尾部可恢复，index 可重建 |
| P8 | core runtime 集成 | P1-P7 | onload/unload 可回滚、可重复关闭 |
| P9 | 跨平台与压力强化 | P8 | Windows 基线达标，Linux/macOS 接缝可编译 |

每个阶段必须有独立提交、可重放测试和明确的退出证据。阶段之间不把“临时实现”偷偷变成公共接口；如果需要变化，先更新本文和架构文档。

## 3. 分阶段开工顺序

### P0：契约与格式冻结

工作内容：

1. 建立 `TypeCode` 的打包/拆包规则和保留值约束，确认未知默认码不是 `0,0`。
2. 写出内部 port 的最小形状：`AllocatorPort`、`ExecutorPort`、`ThreadContextPort`、`ErrorSinkPort`、`ErrorLevelQueryPort`、`FileAppendPort`。
3. 把文件头、记录头、Log 区段（提交格式）、64 字节 index 项写成字段表和长度断言。
4. 确认不引入 `CoreConfig`，容量和槽位使用宏或编译期常量。

出口证据：TypeCode round-trip（往返）测试、协议长度静态断言、依赖图评审记录。

### P1：error 核心与 early/slow writer

工作内容：

1. 定义错误类别常量、五级等级映射和未知查询返回 `-1` 的函数。
2. 实现 UTC 日期目录发现和三个文件的头检查。
3. 实现加锁追加的 slow writer；early writer 只是一条启动期入口，复用 slow writer，不创建额外文件。
4. 为写入失败保留无分配的最小状态，禁止在日志故障中递归申请内存。

出口证据：core 未启动 memory/thread 时可以写 `slow.destiny`；并发写入不会交错；头部损坏可报告。

### P2：thread、worker 和固定槽位

工作内容：

1. 实现 `destiny::Thread` 的创建、停止接收、排空和 join。
2. 实现 `heavy`、`normal`、`light` 的语义调度和降级链，不实现物理 affinity。
3. 固定编译期槽位数量和名字，提供诊断槽位；外部不能声明新的槽位。
4. 接入未来 CPU 拓扑适配器的只读接缝，不把 P/E 核数量写死到公共接口。

出口证据：每个槽位只能由所属线程访问；heavy 容量不足时降级可观测；停止/排空/join 顺序在测试中可断言。

### P3：diagnostic

工作内容：

1. 新建独立 `diagnostic` 模块，发行版收集最低必要上下文，调试构建追加更多信息。
2. 从 thread 固定槽位读取上下文，从 memory port 获得受控存储；不 include log、不创建 worker。
3. 输出可复制的诊断字符串；不使用 `user:`、`debug:` 前缀，不验证 user 文本。

出口证据：无 Log 运行时时 diagnostic 仍可单测；不同构建模式字段差异明确；外部线程标记为未知而不伪造 Destiny 线程。

### P4：memory

工作内容：

1. 先实现固定大小池，再实现对齐的变长后端，统一返回 Allocation 句柄。
2. 句柄携带 owner、generation、大小、对齐、池 id 和释放路径；拒绝旧句柄和重复释放。
3. 实现 `owner_local`、`transferable`、`shared` 三种显式生命周期契约。
4. 实现预申请 token、编译期固定槽位、维护 light worker、空闲回收、泄露检查和按线程强制回收。
5. 维护线程未及时准备 token 时，工作线程必须无等待降级为普通申请；槽位不足直接返回内部 id `-1`。

出口证据：多线程原子性压力测试、跨线程模式测试、token generation 测试、泄露报告和强制回收保护测试。

### P5：Log 纯内存 builder

工作内容：

1. 按 `log/INTERFACE.zh-CN.md` 实现无参 `destiny::Log`，先不接真实文件。
2. 构造时调用 diagnostic；`setErrorCode()` 拆包并覆盖旧码；`append()` 复制字符串，空文本合法。
3. `write()` 在提交边界生成独立快照，原子分配 LogIndex、记录 UTC 秒，并保持移动语义。
4. 用 fake sink 验证 Log 区段格式化、长度计算、user 原样复制和内存失败降级。

出口证据：builder 单测不依赖文件、线程或真实队列；源字符串销毁后快照仍正确；Log 不抛异常。

### P6：正常队列、等级解析和 warning 顺序

工作内容：

1. 接入非阻塞 MPMC 的 `try_push/try_pop`，禁止生产者调用会等待的 push/pop。
2. 用 thread 申请一个 log `light` worker；不要因为 MPMC 类型而在 v0 增加第二个消费者。
3. worker 消费时查询 error 等级；未知码先写原始 warning，再复制原始 Log 生成 unknown-code warning。
4. queue 满时原始记录直接写 slow，随后按顺序写 queue-failure warning；不修改原始错误码。
5. 错误事件必须在记录持久化后投递，避免 log worker 被错误处理阻塞。

出口证据：队列满时生产者无等待；原始记录、unknown warning、queue-failure warning 的顺序和副本关系可读回验证。

### P7：文件、索引和恢复

工作内容：

1. 将 `day_directory`、`file_format`、`segment_writer`、`index_writer`、`recovery` 分开实现。
2. 正常记录写 `log.destiny`，固定 8 槽位 index 写 `index.destiny`；slow 文件使用同一记录格式但不索引。
3. 检查 magic、fileTypeCode、recordLength、sectionLength、64 字节 index 对齐和 offset 范围。
4. 尾部不完整只截断尾部；index 缺失/损坏时从 `log.destiny` 重建；不解析 user。

出口证据：故障注入测试覆盖半条记录、半个 index 项、错误长度、错误头部、index 重建和 UTC 跨日。

### P8：core runtime 集成

启动顺序：early/slow writer → error → thread → memory → normal log → diagnostic 绑定。任一阶段失败都记录后反序回滚，`onload()` 只返回 `bool`。

关闭顺序：停止 log 接收并排空 → 停止 error 事件接收 → thread 进入静默并停止用户提交 → memory 停止预申请、回收和查漏 → join 维护/用户 worker → 脱离 error → 刷新并关闭 early/slow writer。memory 的所有权账本最后收口，但维护 worker 必须在最终 join 前结束。

出口证据：启动故障矩阵、重复 unload、部分初始化回滚、关闭期间错误写入和线程级强制回收测试。

### P9：跨平台与强化

工作内容：

- Windows x86/x64 Debug/Release：线程、文件、虚拟内存和 UTC 行为基线；
- Linux/macOS：平台适配器编译与最小 smoke test；
- 并发压力：多生产者、队列满、slow writer、LogIndex 空洞；
- 恢复压力：异常终止后的尾部修复、index 重建；
- 内存压力：池耗尽、变长扩容失败、预申请降级、泄露检查；
- 性能基线：以 Windows 实测确定阈值，不在架构阶段虚构数字。

出口证据：平台矩阵、压力报告、故障注入报告和可重复的性能基线。

## 4. 验收矩阵

| 领域 | 必须满足 | 验收证据 |
| --- | --- | --- |
| 依赖 | 模块不 include 彼此具体实现；通过 port 装配 | include 扫描、CMake 目标图、架构评审 |
| 语言约束 | `code/core` 无 `throw`、`catch`、C++ `thread_local` | 静态搜索和编译告警清单 |
| TypeCode | 两个 `uint32_t` 往返无损，磁盘 64 位 | 单元测试、协议样例 |
| 生命周期 | onload 只返回 bool；失败反序回滚；unload 可重复 | 故障注入日志、状态断言 |
| thread | 三类别可运行；槽位编译期固定；heavy 降级确定 | 并发测试、槽位测试、降级事件 |
| diagnostic | 构造时自动采集；独立于 log；发行/调试策略可区分 | fake port 测试、构建矩阵 |
| memory | 固定/变长、句柄、三种模式、预申请和维护 worker | 原子压力、所有权、泄露、强制回收报告 |
| Log 接口 | 无参构造、setErrorCode/append/write、移动不可复制、无链式 | 编译期接口测试、行为单测 |
| Log 快照 | append 复制；空文本合法；write 时有 UTC 秒和 LogIndex | 修改源字符串后的回读断言 |
| 队列 | MPMC 非阻塞；满时不等待；一个 log light worker | 计时/线程测试、队列满注入 |
| 等级 | error 查询等级；未知返回 -1；warning 链路顺序正确 | 错误码表测试、慢车道记录回放 |
| 文件 | UTC 目录、头部、recordLength/sectionLength 严格匹配 | 二进制 golden 文件和边界测试 |
| 索引 | 64 字节项、低 32 位 LogIndex、只索引 log | index 回读、重建测试 |
| 恢复 | 尾部截断、index 重建、slow 不索引 | 损坏样本集和恢复报告 |
| 三平台 | Windows 先通过，Linux/macOS 接缝可编译 | CI 矩阵和平台日志 |

验收不以“程序没有崩溃”作为唯一标准；必须能从 `log.destiny`、`index.destiny` 和 `slow.destiny` 的字节内容证明顺序、长度和恢复结果。

## 5. 测试与故障注入清单

测试替身应在 port 接缝上提供，不把真实文件和 OS 线程带进每个单元测试：

1. `FakeFileAppend`：可在任意字节位置返回失败、短写或 flush 失败。
2. `FakeAllocator`：可控制固定池耗尽、变长扩容失败、预申请延迟和 generation 变化。
3. `FakeExecutor`：可控制 worker 尚未启动、停止接收和 join 顺序。
4. `FakeErrorLevelQuery`：覆盖五级、未知码和查询失败。
5. `FakeClock`：覆盖 UTC 跨秒、跨日和写入期间时间采样。
6. `FakeQueue`：可精确填满并记录生产者是否等待。

每个故障样本都要记录：触发点、原始错误码、是否进入 slow、落盘文件、LogIndex 是否出现空洞、恢复后可读取的最后一条记录。

## 6. Git / GitHub 备份节点

建议分支：`codex/core-architecture`。主分支只接收已经通过阶段出口的提交；不使用 force-push 覆盖备份节点。每个节点都应能独立 checkout、构建文档和运行对应阶段测试。

| 节点 | 建议提交主题 | 建议标签 | 可恢复内容 |
| --- | --- | --- | --- |
| N0 | `文档(core)：冻结中文架构与术语` | `core-architecture-v0` | 本架构、Log 接口、plan、HTML |
| N1 | `契约(core)：冻结 TypeCode 与内部 port` | `core-contract-v0` | 编码规则和依赖接缝 |
| N2 | `日志(core)：加入 early/slow 文件基础` | `core-file-alpha` | UTC 目录、头部、加锁 slow writer |
| N3 | `线程(core)：加入 Destiny 线程上下文` | `core-thread-alpha` | 三类别、固定槽位、join |
| N4 | `诊断(core)：加入 diagnostic 采集` | `core-diagnostic-alpha` | 发行/调试诊断快照 |
| N5 | `内存(core)：加入句柄与预申请` | `core-memory-alpha` | 固定/变长、所有权、维护 worker |
| N6 | `日志(core)：加入 Log builder` | `core-log-builder-alpha` | 纯内存 Log 与区段格式化 |
| N7 | `日志(core)：加入非阻塞队列与 warning` | `core-log-pipeline-beta` | 一个 light worker、slow 路径、错误顺序 |
| N8 | `恢复(core)：加入 index 与文件恢复` | `core-recovery-beta` | index、尾部修复、重建 |
| N9 | `运行时(core)：集成 onload/unload` | `core-runtime-rc1` | 完整生命周期和回滚 |
| N10 | `测试(core)：完成跨平台与压力覆盖` | `core-runtime-v1` | Windows 基线、Linux/macOS 接缝、压力报告 |

备份规则：

- 每个 N 节点通过后立即 push 到 GitHub，并创建轻量 tag；
- 阶段内的实验提交可以存在，但合并前压缩为一个可回退节点；
- 文档、协议、实现和测试不要混成无法独立回退的大提交；
- GitHub PR 描述必须附阶段出口、测试命令、平台和已知未冻结项；
- 任何修改 TypeCode、文件协议、Log 公共接口或关闭顺序的 PR，都必须先更新中文架构文档和本计划；
- 发布候选前保留一个只读备份分支，例如 `backup/core-runtime-rc1`，用于恢复和二分。

首个文档节点可以按下面的命令执行；命令只作为后续开工参考，本轮不创建分支、不提交、不推送：

```powershell
git switch -c codex/core-architecture
git add CONTEXT.zh-CN.md code/core/ARCHITECTURE.zh-CN.md code/core/ARCHITECTURE.html code/core/log/INTERFACE.zh-CN.md code/core/plan.md
git commit -m "docs(core): freeze Chinese architecture"
git tag core-architecture-v0
git push -u origin codex/core-architecture
git push origin core-architecture-v0
```

## 7. 开工前检查表

- [ ] 本文和 `ARCHITECTURE.zh-CN.md` 的 TypeCode、Log、文件格式描述一致。
- [ ] `INTERFACE.zh-CN.md` 没有旧的公开 Level、WriteResult、field 或链式调用方案。
- [ ] 不恢复英文架构文档或英文上下文文件。
- [ ] 确认本轮只改文档，源码改动由后续阶段单独提交。
- [ ] 为 P0 建立 port 测试替身和协议 golden 样本目录。
- [ ] 在 Windows 上记录第一份编译器、标准库和文件系统基线。

# Destiny Core 领域术语

`code/core` 是位于 `basicType` 之上的第二层运行时基础。它提供错误、日志、内存和线程能力，并且项目不使用 C++ 异常。

## 术语

**错误（Error）**：
一种 C 风格的失败值和诊断状态，用于让运行时在故障后保持结构完整并继续运行。
_避免_: 异常、异常系统

**错误码（Error code）**：
由 `uint32_t category` 和类别内 `uint32_t code` 组成的稳定错误标识。
_避免_: 错误字符串、异常码

**早期日志（Early log）**：
完整 core 启动前即可使用的最小日志路径。
_避免_: 启动异常日志器

**慢车道（Slow lane）**：
错误发生后，运行时为维持结构完整而进入的降级工作状态。
_避免_: 阻塞模式、崩溃模式

**Destiny 线程（Destiny thread）**：
由 core 线程管理器创建和跟踪，并拥有固定命名上下文的线程。
_避免_: thread-local 替代物、非托管线程

**线程类别（Thread class）**：
Destiny 线程的语义调度角色：`heavy`（重型）、`normal`（普通）和 `light`（轻型）。
_避免_: P 线程、E 线程、物理核心线程

**分配模式（Allocation mode）**：
绑定到一次内存分配上的生命周期契约：`owner_local`（所有者本地）、`transferable`（可转移）或 `shared`（共享）。
_避免_: 隐式跨线程所有权

**分配句柄（Allocation handle）**：
内存系统返回的类型，除数据地址外还携带访问、所有权和释放所需的元数据。
_避免_: 裸分配指针

**预申请（Preallocation）**：
工作线程提前请求维护线程在本线程槽位中准备内存的操作。
_避免_: 投机 malloc、后台分配

**Destiny 编码（Destiny code）**：
由 category 和 code 组成的 64 位持久化身份，用于标识文件和数据记录。
_避免_: 文件名身份、日志文件键

**日志记录（Log record）**：
包含日志级别、Destiny 编码、运行时上下文和负载的结构化事件。
_避免_: 打印行、控制台消息

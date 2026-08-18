# Destiny Core Context

中文版： [CONTEXT.zh-CN.md](./CONTEXT.zh-CN.md)

`code/core` is the second runtime layer above `basicType`. It provides the project's error, logging, memory, and thread foundations without using C++ exceptions.

## Language

**Error**:
A C-style failure value and diagnostic state used to keep the runtime operating after a fault.
_Avoid_: Exception, exception system

**Error code**:
A stable pair of `uint32_t` values: a category owned by a subsystem and a code meaningful inside that category.
_Avoid_: Error string, exception code

**Early log**:
The minimal logging path available before the complete core runtime is loaded.
_Avoid_: Bootstrap exception logger

**Slow lane**:
The degraded operating mode entered after an error so the runtime can preserve its structural invariants while doing recovery and diagnostic work.
_Avoid_: Blocking mode, emergency crash mode

**Destiny thread**:
A thread created and tracked by the core thread manager, with a fixed named context supplied by the manager.
_Avoid_: Thread-local replacement, unmanaged thread

**Thread class**:
The semantic scheduling role of a destiny thread: `heavy`, `normal`, or `light`.
_Avoid_: P-thread, E-thread, physical-core thread

**Allocation mode**:
The explicit lifetime contract attached to an allocation: owner-local, transferable, or shared.
_Avoid_: Implicit cross-thread ownership

**Allocation handle**:
The typed value returned by the memory system that carries access and release metadata alongside the data address.
_Avoid_: Bare allocation pointer

**Preallocation**:
A request for the memory maintenance worker to prepare a block in a per-thread slot before the worker needs it.
_Avoid_: Speculative malloc, background allocation

**Destiny code**:
The 64-bit on-disk identity formed from a category `uint32_t` and a code `uint32_t`.
_Avoid_: Filename identity, log filename key

**Log record**:
A structured runtime event containing a level, a Destiny code, context, and a payload that can be persisted and later indexed.
_Avoid_: Printed line, console message

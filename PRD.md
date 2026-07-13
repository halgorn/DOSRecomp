# PRD — DOSRecomp

## DOS Executable Static Recompiler for Linux

**Version:** 1.0
**Status:** Active
**License:** MIT
**Language:** C++23
**Target Platform:** Linux x86_64 (primary)

---

# Vision

DOSRecomp is an open-source static binary recompiler that converts MS-DOS executables (.COM and .EXE) into native Linux ELF executables.

Unlike DOSBox or DOSEMU, DOSRecomp does **not emulate the CPU**. It analyzes the original executable, reconstructs its control flow, replaces DOS APIs with native implementations, and emits optimized native binaries.

The objective is software preservation, reverse engineering, performance, and educational research.

---

# Problem

Current solutions execute DOS software through emulation or compatibility layers.

Examples:

* DOSBox
* DOSBox-X
* DOSEMU2

These solutions:

* emulate the DOS environment
* execute interrupts dynamically
* introduce compatibility overhead
* cannot modernize binaries
* do not recover readable program structure

The goal is different.

Instead of running DOS software inside DOS...

Convert DOS software into native Linux applications.

---

# Vision Statement

> "Transform DOS software into first-class Linux applications."

---

# Core Principles

* No CPU emulation.
* No virtual machine.
* No DOS runtime required after recompilation.
* Produce standalone ELF executables.
* Recover high-level program structure whenever possible.
* Modular architecture.
* Designed for research and extensibility.

---

# Goals

* Convert COM executables.
* Convert MZ EXE executables.
* Produce valid ELF binaries.
* Replace DOS interrupts with Linux implementations.
* Reconstruct functions.
* Recover control flow.
* Generate maintainable intermediate code.
* Preserve original behavior.

---

# Non-Goals

Version 1 will NOT support:

* Windows executables
* Win32
* Win16
* OS/2
* DOS extenders
* Protected Mode
* Networking
* Multiplayer
* DRM bypass
* Copy protection circumvention

---

# High-Level Architecture

```text
            DOS Executable
            (.COM / .EXE)
                    │
                    ▼
          Binary Loader
                    │
                    ▼
        Instruction Decoder (8086)
                    │
                    ▼
      Control Flow Analysis
      (basic blocks, branches, calls, loops)
                    │
                    ▼
     High-Level Reconstruction
     (if/else, do/while, function dispatch)
                    │
                    ▼
 DOS API Translation Layer
 (INT 21h, 10h, 13h, 16h → Linux syscalls)
                    │
                    ▼
        C++ Code Generator
     (readable C++23 source emitted to disk)
                    │
                    ▼
        Host C++ Compiler (g++ -O2)
                    │
                    ▼
          Native ELF Binary
```

---

# Project Modules

## 1. Binary Loader

Responsibilities

* Read COM
* Read MZ EXE
* Parse relocation table
* Parse headers
* Load memory image
* Resolve segments

Output

Memory representation.

---

## 2. CPU Decoder

Supported

8086

Later:

* 80186
* 80286
* 80386 (optional)

Responsibilities

* Decode instructions
* Validate operands
* Detect invalid instructions
* Recover instruction boundaries

---

## 3. CFG Builder

Responsibilities

Recover

* Branches
* Loops (via backward-conditional-jump detection)
* Calls (runtime stack-based)
* Functions (via INT 0x21 terminators; full prologue detection: future)

Output

Implicit CFG as a worklist of basic-block start offsets threaded through
the C++ emitter. No explicit CFG data structure.

---

## 4. SSA Builder

*Future work.* Not in current scope. The C++ backend emits statements
directly from the worklist of basic blocks. SSA construction is required
only if/when an LLVM backend is added (see §8).

---

## 5. High-Level Reconstruction

Convert

```asm
CMP AX,0
JE label
```

Into

```cpp
if (ax == 0)
```

Status

* if / else — done
* do / while — done (backward Jcc to block start)
* while — not yet
* for — not yet
* switch — not yet
* function dispatch — runtime CALL/RET done; named function recovery: future

---

## 6. DOS API Translation

Translate interrupts.

Example

INT 21h

↓

Linux syscalls

Example

```text
INT 21h AH=09h

↓

printf()
```

Example

```text
INT 21h AH=3Dh

↓

open()
```

---

## Initial Interrupt Support

INT 21h

* File IO
* Memory
* Exit
* Current directory
* Date
* Time
* Environment

INT 10h

* Text mode
* Cursor
* Video memory

INT 16h

* Keyboard

INT 13h

* Disk access (virtual)

---

## 7. Runtime Library

Provide modern implementations for

* Filesystem — done (open/read/write/close/chdir/getcwd via Linux syscalls)
* Memory — done (alloc/free/resize)
* Console — partial (INT 10h text mode, INT 21h AH=02h/09h/0Ah)
* Keyboard — partial (INT 16h AH=00h)
* Disk — partial (INT 13h AH=00h/02h/03h via in-memory buffer)
* Audio — not yet
* Graphics — not yet (no mode 13h, no pixel ops)
* Mouse — not yet
* Timers — not yet

---

## 8. Code Generation Backends

The primary backend is the C++ emitter. It produces readable C++23
source that is compiled by a host C++ compiler (g++ -O2 -static) into
a native ELF binary.

C++ Backend (primary)

* Readable output suitable for reverse engineering
* Reuses host compiler's optimizer (-O2)
* Single dependency: g++ with C++23 support
* x86_64 Linux only

LLVM Backend (future, for multi-architecture support)

* Generate LLVM IR
* Run LLVM optimization pipeline
* Target x86_64, ARM64, RISC-V

The C++ backend was chosen as the production backend because it
delivers working binaries with 1/N the implementation cost of an LLVM
backend. Multi-architecture support remains a long-term goal.

---

# AI Reconstruction Module (Future)

Use an LLM to infer:

Variable names

Function names

Enums

Structs

Comments

Potential source intent

Example

Assembly

↓

CFG

↓

Decompiler

↓

LLM

↓

Readable C++

---

# Runtime API Mapping

Example

DOS

```asm
INT 21h
```

↓

Runtime

```cpp
dos_open()
```

↓

Linux

```cpp
open()
```

---

# Graphics

Phase 1

Console only.

Phase 2

SDL3

Features

320x200

Mode 13h

Palette

Sprites

Double buffering

---

# Audio

Phase 1

None

Phase 2

PC Speaker

Phase 3

Sound Blaster

Phase 4

AdLib

---

# File System

Virtual DOS drive

Example

```text
C:\GAME

↓

~/dosrecomp/game/
```

Drive mapping

Read-only mode

Write protection

Sandbox

---

# Memory Model

Support

Conventional Memory

640 KB

Future

EMS

XMS

Protected Mode

---

# Optimization Pipeline

Optimization is delegated to the host C++ compiler (g++ -O2). The
emitter produces C++23 source with inline block functions, register
coherence helpers, and high-level control flow that the host compiler
can optimize.

When the LLVM backend is implemented, the pipeline will include:

* Constant folding
* Dead code elimination
* Register propagation
* Function inlining
* Loop simplification
* Peephole optimization
* CFG simplification

---

# CLI

Compile

```bash
dosrecomp program.com
```

Output

```bash
program
```

Specify output

```bash
dosrecomp program.com -o program_linux
```

Verbose

```bash
dosrecomp program.com --verbose
```

Generate readable C++ (default; the compiled ELF is the user-visible
output, but the C++ source is kept for inspection)

```bash
dosrecomp program.com --keep-cpp
```

Future flags (not yet implemented)

* `--emit-llvm` — emit LLVM IR (requires §8 LLVM backend)
* `--emit-cfg` — emit GraphViz CFG dump
* `--emit-dot` — GraphViz output

---

# GUI (Future)

Binary Viewer

CFG Viewer

Disassembler

Decompiler

Hex Viewer

IR Viewer

Function Browser

Memory Viewer

Live execution trace

---

# Plugin System

Allow plugins for

Instruction decoders

Optimizations

API mappings

Custom loaders

Game-specific fixes

Output generators

---

# Testing

Unit Tests

* Instruction decoder
* Binary loader
* CFG / block worklist
* Runtime header (call/ret, segment dispatch)

Integration Tests

* Hand-assembled .com programs (chdir, alloc, do/while, INT 10h)
* Real DOS utilities (future)

Golden Tests

* Compare recompiled output behavior against original

Regression Suite

* Known DOS utilities
* Games (future)

Benchmarks

* Compile time per size class
* Runtime parity vs. DOSBox / native

---

# Performance Targets

Binary loading

<50 ms

Block worklist + C++ emission

<200 ms for small programs (<4 KB)

Small applications (<4 KB)

<1 second end-to-end

Medium applications (<64 KB)

<5 seconds end-to-end

Large applications (<640 KB)

<30 seconds end-to-end

Generated executable startup

<5 ms cold, near-native warm

---

# Coding Standards

Modern C++23

No global mutable state (the runtime header's call stack is a
documented exception for the runtime)

RAII

Modular architecture: each file ≤500 lines

Public APIs documented at declaration

Continuous Integration (future)

Static analysis (clang-tidy, future)

Fuzz testing (decoder, future)

---

# Project Structure

```text
dosrecomp/
 ├── cli/                # main.cpp — argument parsing, dispatch
 ├── loader/             # COM and MZ loaders
 ├── decoder/            # 8086 instruction decoder
 ├── compiler/           # straight-line + branching C++ emitter
 ├── runtime/            # generated-call helper (regs, mem, INT handlers)
 ├── tests/              # CTest suite (33 passing)
 ├── docs/               # architecture.md
 ├── examples/           # (future) recompiled real DOS programs
 └── TODO.md             # progress + gap tracking
```

Removed from PRD: `ssa/`, `ir/`, `llvm_backend/`, `optimizer/`. These
are not in current scope. See §4 and §8.

---

# Stretch Goals

* LLVM IR backend (enables ARM64, RISC-V, WebAssembly)
* Windows PE → Linux ELF recompilation research
* DOS game auto-patching
* Automatic SDL rendering (mode 13h)
* Automatic OpenGL renderer
* Automatic Vulkan renderer
* PC Speaker + Sound Blaster audio
* Source code regeneration (decompiler output)
* Interactive reverse engineering assistant powered by AI
* Native ARM64 output (requires LLVM backend)
* WebAssembly backend (requires LLVM backend)

---

# Success Metrics

* Successfully recompile simple COM applications. — done
* Successfully recompile standard DOS utilities. — in progress
* Successfully execute classic DOS programs as native ELF binaries. — done (24 INT handlers)
* Achieve behavior equivalent to the original executables for supported features. — done
* Produce readable intermediate representations useful for reverse engineering. — done (C++ source kept)
* Maintain a modular architecture that allows new instructions, interrupts, and backends to be added independently. — done

---

# Long-Term Vision

DOSRecomp aims to become the definitive open-source static recompilation framework for DOS software, combining binary analysis, compiler technology, reverse engineering, and optional AI-assisted code reconstruction. Rather than emulating legacy systems, it preserves and modernizes classic software by transforming it into native applications for contemporary platforms.

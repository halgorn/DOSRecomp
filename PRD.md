# PRD — DOSRecomp

## DOS Executable Static Recompiler for Linux

**Version:** 1.0
**Status:** Draft
**License:** MIT
**Language:** C++23 (LLVM) or Rust
**Target Platform:** Linux x86_64

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
        Instruction Decoder
                    │
                    ▼
      Control Flow Analysis
                    │
                    ▼
      Intermediate Representation
                    │
                    ▼
    High-Level Reconstruction
                    │
                    ▼
 DOS API Translation Layer
                    │
                    ▼
        LLVM IR Generator
                    │
                    ▼
       LLVM Optimization
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

* Functions
* Loops
* Calls
* Switches
* Jump tables
* Branches

Output

Control Flow Graph.

---

## 4. SSA Builder

Convert assembly into SSA representation.

Benefits

* Easier optimization
* Variable tracking
* Dead code elimination
* Constant propagation

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

Recover

* while
* for
* do
* switch
* if
* else

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

Filesystem

Audio

Console

Graphics

Keyboard

Mouse

Timers

Memory

---

## 8. LLVM Backend

Generate

LLVM IR

Benefits

* Optimization
* Native code generation
* Multiple architectures in the future

Future

* ARM64
* RISC-V

---

# Optional C Backend

Instead of LLVM

Generate readable C++.

Useful for reverse engineering.

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

Constant folding

Dead code elimination

Register propagation

Function inlining

Loop simplification

Peephole optimization

CFG simplification

---

# CLI

Compile

```bash
dosrecomp doom.exe
```

Output

```bash
doom
```

Specify output

```bash
dosrecomp doom.exe -o doom_linux
```

Verbose

```bash
dosrecomp doom.exe --verbose
```

Generate LLVM

```bash
dosrecomp doom.exe --emit-llvm
```

Generate C++

```bash
dosrecomp doom.exe --emit-cpp
```

Generate CFG

```bash
dosrecomp doom.exe --emit-cfg
```

Generate GraphViz

```bash
dosrecomp doom.exe --emit-dot
```

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

Instruction decoder

Binary loader

CFG builder

SSA

IR

Integration Tests

Compile real DOS software

Golden Tests

Compare original behavior against recompiled output

Regression Suite

Known DOS utilities

Games

Benchmarks

---

# Performance Targets

Binary loading

<50 ms

CFG generation

<200 ms

Small applications

<1 second

Medium applications

<5 seconds

Large applications

<30 seconds

Generated executable startup

Near-native

---

# Coding Standards

Modern C++23 or Rust

No global mutable state

RAII (C++) or ownership model (Rust)

Modular architecture

100% documented public APIs

Continuous Integration

Static analysis

Fuzz testing

---

# Project Structure

```text
dosrecomp/
 ├── cli/
 ├── loader/
 ├── decoder/
 ├── cfg/
 ├── ssa/
 ├── ir/
 ├── optimizer/
 ├── runtime/
 ├── llvm_backend/
 ├── c_backend/
 ├── plugins/
 ├── tests/
 ├── docs/
 ├── examples/
 └── third_party/
```

---

# Stretch Goals

* Windows PE → Linux ELF recompilation research
* DOS game auto-patching
* Automatic SDL rendering
* Automatic OpenGL renderer
* Automatic Vulkan renderer
* Native ARM64 output
* WebAssembly backend
* Source code regeneration
* Interactive reverse engineering assistant powered by AI

---

# Success Metrics

* Successfully recompile simple COM applications.
* Successfully recompile standard DOS utilities.
* Successfully execute classic DOS programs as native ELF binaries.
* Achieve behavior equivalent to the original executables for supported features.
* Produce readable intermediate representations useful for reverse engineering.
* Maintain a modular architecture that allows new instructions, interrupts, and backends to be added independently.

---

# Long-Term Vision

DOSRecomp aims to become the definitive open-source static recompilation framework for DOS software, combining binary analysis, compiler technology, reverse engineering, and optional AI-assisted code reconstruction. Rather than emulating legacy systems, it preserves and modernizes classic software by transforming it into native applications for contemporary platforms.

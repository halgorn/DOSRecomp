# DOSRecomp

> **Static recompilation of DOS executables into native Linux applications.**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](#license)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)]()
[![LLVM](https://img.shields.io/badge/LLVM-Compatible-orange)]()
[![Linux](https://img.shields.io/badge/Linux-x86__64-success)]()
[![CI](https://github.com/halgorn/DOSRecomp/actions/workflows/ci.yml/badge.svg)](https://github.com/halgorn/DOSRecomp/actions/workflows/ci.yml)

## Overview

DOSRecomp is an open-source **static binary recompiler** that transforms MS-DOS executables (`.COM` and `.EXE`) into native Linux ELF executables.

Unlike traditional emulators such as DOSBox, DOSRecomp does **not emulate the CPU**.

Instead, it analyzes the executable, reconstructs its control flow, translates DOS APIs into native Linux equivalents, and generates optimized native binaries.

The final executable runs directly on Linux without requiring DOS, a virtual machine, or CPU emulation.

---

## Why?

Traditional DOS execution works like this:

```text
DOS Program
     │
     ▼
 DOS Emulator
     │
     ▼
 Linux
```

DOSRecomp takes a completely different approach:

```text
DOS Program
     │
     ▼
 Static Recompiler
     │
     ▼
 Native ELF
     │
     ▼
 Linux
```

The objective is software preservation, reverse engineering, performance, and long-term compatibility.

---

## Current status

The straight-line compiler walks the CFG → IR → SSA pipeline and emits native
ELF for the verified subset of DOS programs:

* Word and byte register MOV (immediate, register, `[mem]` load/store)
* Word and byte arithmetic/logic (`ADD`, `SUB`, `AND`, `OR`, `XOR`, `CMP`, `TEST`, `ADC`, `SBB`)
  in immediate and register forms; `INC`/`DEC` word
* `PUSH`/`POP` word with stack tracking in real-mode memory
* `CALL` near with return-address push; `RET` and `RET imm16`
* Conditional branches evaluated statically through the SSA flag chain
  (`JE`, `JNE`, `JB`, `JAE`, `JL`, `JGE`, `JS`, `JNS`, ...)
* `INT 21h` `AH=09h` (`$`-terminated string write), `AH=02h` (write char), `AH=4Ch` (exit)
* `INT 10h` `AH=0Eh` (teletype char), `AH=02h` (cursor positioning as a no-op)

DOS runtime services exist as standalone libraries (`int10`, `int13`,
`int16`, `int21`, `int1a`, conventional memory, virtual drive, virtual file
system) but the recompiled output is a self-contained ELF that talks to the
host through Linux syscalls only — no DOS, no VM, no emulation.

Data-dependent branches, indirect jumps (`jmp [bx]`), and runtime keyboard
input are deferred: they require a full x86_64 codegen layer that is out of
scope for the current subset.

### Build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Compile a supported DOS binary to a native executable:

```bash
./build/dosrecomp program.com -o program_linux
```

Example programs the pipeline accepts today:

```asm
; exit 7
mov ax, 0x4c07
int 21h

; print "ok" then exit 6
mov dx, msg
mov ax, 0x0900          ; AH=09h, AL=0
int 21h
mov ax, 0x4c06
int 21h
msg: db "ok$"

; write 'A' to stderr (file descriptor 2) then exit
mov dl, 'A'
mov ah, 02h
int 21h
mov ax, 0x4c07
int 21h
```

Emit readable C++ for the same verified subset:

```bash
./build/dosrecomp program.com --emit-cpp > program.cpp
```

Emit textual LLVM IR for the same subset (one SSA definition per straight-line
instruction):

```bash
./build/dosrecomp program.com --emit-llvm > program.ll
```

Emit the currently supported direct-control-flow graph as GraphViz DOT:

```bash
./build/dosrecomp program.exe --emit-cfg > program.dot
```

`--emit-dot` is an equivalent explicit GraphViz spelling.

Inspect the target-independent control-flow IR:

```bash
./build/dosrecomp program.exe --emit-ir
```

See [the architecture notes](docs/architecture.md) for the loader contract.

## Features

* Static recompilation
* Native Linux ELF output
* No CPU emulation
* No virtual machine
* No DOS runtime
* COM executable support
* MZ EXE executable support
* DOS interrupt translation
* LLVM backend
* Optional C/C++ code generation
* Control Flow Graph reconstruction
* SSA intermediate representation
* Plugin architecture
* AI-assisted reverse engineering (planned)

---

## Planned Architecture

```text
DOS Executable
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
 DOS API Translation
        │
        ▼
 LLVM Backend
        │
        ▼
 Native ELF Executable
```

---

## Supported Formats

### Input

* `.COM`
* `.EXE` (MZ)

### Output

* ELF x86_64

Future:

* ARM64
* RISC-V
* WebAssembly

---

## Roadmap

### Phase 1

* Binary loader
* COM support
* MZ EXE support
* 8086 decoder
* ELF generation
* Basic CLI

### Phase 2

* INT 21h implementation
* File system support
* Console output
* Keyboard input

### Phase 3

* CFG reconstruction
* SSA
* LLVM backend
* Optimizations

### Phase 4

* VGA Mode 13h
* SDL3 backend
* Mouse
* Audio

### Phase 5

* C/C++ code generation
* Reverse engineering tools
* Graph viewer
* GUI

### Phase 6

* AI-assisted decompilation
* Function reconstruction
* Variable recovery
* Automatic code documentation

---

## Example

Input:

```bash
dosrecomp doom.exe
```

Output:

```bash
doom
```

Run:

```bash
./doom
```

---

## Long-Term Vision

DOSRecomp is not intended to be "another DOS emulator."

The long-term vision is to become a modern recompilation framework capable of preserving legacy DOS software by converting it into native applications for modern operating systems.

The project combines ideas from:

* Static binary recompilation
* Reverse engineering
* Compiler design
* LLVM
* Binary translation
* Software preservation

---

## Contributing

Contributions are welcome.

Areas of interest include:

* Compiler development
* LLVM
* Binary analysis
* Reverse engineering
* Operating systems
* Static analysis
* Retro computing
* Game preservation

---

## License

MIT License.

See the LICENSE file for details.

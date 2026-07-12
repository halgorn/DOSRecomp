# DOSRecomp

> **Static recompilation of DOS executables into native Linux applications.**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](#license)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)]()
[![LLVM](https://img.shields.io/badge/LLVM-Compatible-orange)]()
[![Linux](https://img.shields.io/badge/Linux-x86__64-success)]()

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

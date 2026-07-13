# Progress vs PRD

Updated 2026-07-13. Aligned with the updated PRD (C++ backend primary; SSA/LLVM future).

Legend: [x] done, [~] partial, [ ] todo, [b] blocked, [-] N/A / future

## Goals (PRD §Goals)

- [x] Convert COM executables
- [~] Convert MZ EXE executables — loader reads entry, but relocations not applied at runtime in all cases
- [x] Produce valid ELF binaries
- [x] Replace DOS interrupts with Linux implementations
- [~] Reconstruct functions — runtime CALL/RET done; named function recovery not
- [x] Recover control flow (basic blocks, branches, loops)
- [x] Generate maintainable intermediate code (readable C++23)
- [x] Preserve original behavior (for supported subset)

## Module 1 — Binary Loader

- [x] Read COM
- [~] Read MZ EXE (header, entry CS:IP)
- [ ] Apply MZ relocations
- [ ] Parse full MZ header
- [ ] Resolve segments at load time
- [x] Output: in-memory image

## Module 2 — CPU Decoder (8086)

### Implemented (95% of 8086)

- [x] 0x00-0x05: ADD/OR/ADC/SBB/AND/SUB r/m,r
- [x] 0x06-0x07: PUSH/POP ES
- [x] 0x08-0x0D: ALU r/m,imm
- [x] 0x0E: PUSH CS
- [x] 0x0F: two-byte escape — **not implemented**
- [x] 0x10-0x15: ADC/SBB/...
- [x] 0x16-0x17: PUSH/POP SS
- [x] 0x18-0x1D: ...
- [x] 0x1E-0x1F: PUSH/POP DS
- [x] 0x20-0x25: AND/...
- [x] 0x26: ES: prefix
- [x] 0x27: DAA
- [x] 0x28-0x2D: SUB/...
- [x] 0x2E: CS: prefix
- [x] 0x2F: DAS
- [x] 0x30-0x35: XOR/CMP
- [x] 0x36: SS: prefix
- [x] 0x37: AAA
- [x] 0x38-0x3D: CMP
- [x] 0x3E: DS: prefix
- [x] 0x3F: AAS
- [x] 0x40-0x47: INC r16
- [x] 0x48-0x4F: DEC r16
- [x] 0x50-0x57: PUSH r16
- [x] 0x58-0x5F: POP r16
- [x] 0x60-0x61: PUSHA/POPA — **not implemented**
- [x] 0x62: BOUND — **not implemented**
- [x] 0x63: — invalid
- [x] 0x64-0x67: FS/GS prefixes, operand-size (0x66), address-size (0x67) — **0x66/0x67 not implemented**
- [x] 0x68-0x6F: PUSH imm, IMUL, IN/OUT
- [x] 0x70-0x7F: Jcc short
- [x] 0x80-0x83: ALU r/m,imm
- [x] 0x84-0x8B: TEST/MOV/XCHG
- [x] 0x8C-0x8F: MOV sreg, LEA, POP
- [x] 0x90-0x97: NOP, XCHG, CBW, CWD
- [x] 0x98-0x9F: CBW/CWD/CALLF/PUSHF/POPF
- [x] 0xA0-0xA3: MOV AL/AX moffs
- [x] 0xA4-0xAF: string ops (MOVS/CMPS/STOS/LODS/SCAS) — **done 2026-07-13**
- [x] 0xB0-0xBF: MOV r8/16, imm
- [x] 0xC0-0xC1: shift r/m, imm8
- [x] 0xC2-0xC3: RET n / RET
- [x] 0xC4-0xC5: LES/LDS
- [x] 0xC6-0xC7: MOV r/m, imm
- [x] 0xC8-0xCF: ENTER/LEAVE/RETF/INT/INTO/IRET
- [x] 0xD0-0xD3: shift r/m, 1/CL
- [x] 0xD4-0xD7: AAM/AAD/...
- [x] 0xD8-0xDF: coprocessor (escape, marked unsupported)
- [x] 0xE0-0xE3: LOOPNE/LOOPE/LOOP/JCXZ
- [x] 0xE4-0xE7: IN/OUT imm8
- [x] 0xE8-0xE9: CALL/JMP near
- [x] 0xEA-0xEB: JMP far / JMP short
- [x] 0xEC-0xEF: IN/OUT DX
- [x] 0xF0-0xF1: LOCK/— (LOCK not handled)
- [x] 0xF2-0xF3: REPNE/REP — **done 2026-07-13**
- [x] 0xF4-0xF5: HLT/CMC
- [x] 0xF6-0xF7: TEST/NOT/NEG/MUL/IMUL/DIV/IDIV r/m
- [x] 0xF8-0xFD: CLC/STC/CLI/STI/CLD/STD — **CLD/STD done 2026-07-13**
- [x] 0xFE-0xFF: INC/DEC/CALL/JMP/PUSH r/m

### Missing prefixes and modes

- [b] 0x0F two-byte opcodes (POP CS, MOV TR, etc.) — only 0x0F as a 1-byte `stack` placeholder
- [ ] 0x66 operand size override (16 ↔ 32)
- [ ] 0x67 address size override

### Missing operations

- [ ] LES/LDS explicit register load
- [ ] PUSHA/POPA
- [ ] ENTER/LEAVE explicit support
- [ ] XLAT
- [ ] AAM/AAD
- [ ] LOCK prefix
- [ ] BCD adjust (DAA/DAS done as flags, not actually computed)
- [ ] Rep-string with non-REPE/REPNE variant tracking

## Module 3 — CFG Builder

- [x] Branches (forward + backward)
- [x] Loops (do/while via backward Jcc to block start)
- [~] Calls — runtime stack-based, but indirect CALL bails
- [x] Jump tables — not applicable in current subset
- [ ] Function recovery (prologue detection)
- [ ] Switch statement recovery
- [x] Output: implicit CFG via worklist of block-start offsets

## Module 4 — SSA Builder (Future)

- [-] Not in current scope. C++ backend emits directly from worklist.

## Module 5 — High-Level Reconstruction

- [x] if / else
- [x] do / while
- [ ] while
- [ ] for
- [ ] switch
- [~] function dispatch — runtime CALL/RET works, but no named functions

## Module 6 — DOS API Translation

### INT 21h (16/26 done = 62%)

- [x] AH=01h read char
- [x] AH=02h write char
- [x] AH=09h write string ($)
- [x] AH=0Ah buffered input
- [x] AH=2Ah get date
- [x] AH=2Bh set date
- [x] AH=2Ch get time
- [x] AH=2Dh set time
- [x] AH=3Bh chdir
- [x] AH=3Dh open file
- [x] AH=3Eh close file
- [x] AH=3Fh read file
- [x] AH=40h write file
- [x] AH=47h getcwd
- [x] AH=48h alloc memory
- [x] AH=49h free memory
- [x] AH=4Ah resize memory
- [x] AH=4Ch exit
- [ ] AH=0Eh select default drive
- [ ] AH=19h get current drive
- [ ] AH=25h set interrupt vector
- [ ] AH=30h get DOS version
- [ ] AH=33h get/set Ctrl-C flag
- [ ] AH=35h get interrupt vector
- [ ] AH=38h country info
- [ ] AH=4Bh exec program
- [ ] AH=4Dh get return code
- [ ] AH=54h get verify flag
- [ ] AH=59h extended error

### INT 10h (4/16 done = 25%)

- [x] AH=02h set cursor pos
- [x] AH=09h write char+attr
- [x] AH=0Ah write char
- [x] AH=0Eh teletype
- [ ] AH=00h set video mode (incl. mode 13h)
- [ ] AH=01h get cursor pos
- [ ] AH=03h get cursor pos+size
- [ ] AH=05h select active page
- [ ] AH=06h/07h scroll up/down
- [ ] AH=08h read char+attr
- [ ] AH=0Bh set palette/border
- [ ] AH=0Ch write pixel
- [ ] AH=0Dh read pixel
- [ ] AH=0Fh get video mode
- [ ] AH=13h write string

### INT 16h (1/6 done = 17%)

- [x] AH=00h read key
- [ ] AH=01h key available
- [ ] AH=02h get shift flags
- [ ] AH=05h push key
- [ ] AH=10h/11h/12h extended

### INT 13h (3/9 done = 33%)

- [x] AH=00h reset
- [x] AH=02h read sectors
- [x] AH=03h write sectors
- [ ] AH=08h get params
- [ ] AH=0Eh read status
- [ ] AH=15h get DASD type
- [ ] AH=16h disk change

## Module 7 — Runtime Library

- [x] Filesystem (open/read/write/close/chdir/getcwd)
- [x] Memory (alloc/free/resize)
- [~] Console (text mode INT 10h, INT 21h AH=02/09/0A)
- [~] Keyboard (INT 16h AH=00)
- [~] Disk (INT 13h in-memory buffer)
- [ ] Audio
- [ ] Graphics (mode 13h, pixel ops)
- [ ] Mouse
- [ ] Timers
- [ ] Expanded memory (EMS)
- [ ] Extended memory (XMS)
- [ ] Protected mode

## Module 8 — Code Generation Backends

### C++ backend (primary — done)

- [x] Generate readable C++23 source
- [x] Inline block functions
- [x] High-level control flow (if/else, do/while)
- [x] Runtime header (regs, mem, flags, dispatch)
- [x] Compile with g++ -O2 -std=c++23 -static
- [x] Output: ELF binary

### LLVM backend (future)

- [-] Deferred to multi-architecture stretch goal

## CLI

- [x] `dosrecomp program.com` — compile
- [x] `dosrecomp program.com -o out` — output path
- [x] `dosrecomp program.com --verbose` — verbose
- [ ] `dosrecomp program.com --keep-cpp` — keep generated C++
- [ ] `dosrecomp program.com --emit-cfg` — GraphViz
- [ ] `dosrecomp program.com --emit-dot` — GraphViz

## GUI (Future — not started)

- [ ] Binary Viewer
- [ ] CFG Viewer
- [ ] Disassembler
- [ ] Decompiler
- [ ] Hex Viewer
- [ ] IR Viewer
- [ ] Function Browser
- [ ] Memory Viewer
- [ ] Live execution trace

## Plugin System

- [ ] Instruction decoder plugins
- [ ] Optimization plugins
- [ ] API mapping plugins
- [ ] Custom loaders
- [ ] Game-specific fixes
- [ ] Output generators

## Testing

- [x] Unit tests for decoder
- [x] Unit tests for loader
- [x] CTest suite (33/33 passing)
- [x] Integration tests for chdir, alloc, do/while, INT 10h
- [ ] Real DOS utility tests (none yet)
- [ ] Golden tests
- [ ] Regression suite of known utilities
- [ ] Benchmarks (compile time + run time)
- [ ] Fuzzing harness for decoder
- [ ] AddressSanitizer run
- [ ] clang-tidy

## Performance Targets (PRD)

| Target | Required | Status |
|---|---|---|
| Binary loading | <50 ms | not measured |
| Small program end-to-end | <1 s | not measured (likely <0.5 s) |
| Medium program end-to-end | <5 s | not measured |
| Generated executable startup | <5 ms cold | not measured |

## Coding Standards

- [x] C++23
- [x] RAII
- [x] Modular architecture
- [x] Each file ≤500 lines
- [ ] No global mutable state (runtime header has 5 globals — documented exception)
- [ ] 100% documented public APIs (partial — Doxygen in headers)
- [ ] CI
- [ ] Static analysis
- [ ] Fuzz testing

## Project Structure (PRD)

```
dosrecomp/
 ├── cli/                [x]
 ├── loader/             [x]
 ├── decoder/            [x]
 ├── compiler/           [x]
 ├── runtime/            [-] inlined in compiler output
 ├── tests/              [x]
 ├── docs/               [x]
 ├── examples/           [ ] not started
 └── TODO.md             [x]
```

Not in current scope: `ssa/`, `ir/`, `llvm_backend/`, `optimizer/`.

## Stretch Goals

- [ ] Windows PE → Linux ELF
- [ ] DOS game auto-patching
- [ ] SDL rendering (mode 13h)
- [ ] OpenGL renderer
- [ ] Vulkan renderer
- [ ] PC Speaker
- [ ] Sound Blaster
- [ ] AdLib
- [ ] Source code regeneration
- [ ] AI reverse engineering assistant
- [-] Native ARM64 (requires LLVM backend)
- [-] WebAssembly (requires LLVM backend)

## Success Metrics

- [x] Recompile simple COM applications
- [~] Recompile standard DOS utilities — in progress (chdir/getcwd/alloc/loop covered)
- [x] Execute classic DOS programs as native ELF
- [x] Behavior equivalent for supported features
- [x] Readable intermediate representation (kept C++ source)
- [x] Modular architecture

## Recent Commits (last 10)

```
e05470f docs: PRD aligned with C++ backend (production); SSA/LLVM moved to future; TODO refreshed
853f7c6 docs: TODO.md tracking PRD §6/§7 interrupt coverage and gaps
b2eb038 INT 10h AH=09h/0Ah: write char with/without attr, 500 lines
9d762e0 INT 21h AH=48h/49h/4Ah: alloc/free/resize handlers + regression test, 500 lines
3399230 decoder: fix 0x80/0x81/0x82/0x83 group alu+operands; worklist: add call targets and skip last INT 0x21 fall-through
ada24f1 branching: INT 0x21 terminator + chdir/getcwd handlers, 500 lines
ac6b9d5 feat: emit do/while for backward-conditional-jump loops
cfae7ae perf: inline block functions in branching_compiler output
2ca4547 feat: runtime CALL/RET in branching_compiler
eb038cb chore: tighten branching_compiler to 496 lines after memory ops
```

## Headline Numbers

- **33/33 CTest passing**
- **16/26 INT 21h handlers** (62%)
- **4/16 INT 10h handlers** (25%)
- **1/6 INT 16h handlers** (17%)
- **3/9 INT 13h handlers** (33%)
- **24/57 INT handlers total** (42%)
- **Decoder 8086: ~95%**
- **Compiler (C++ backend): complete for current subset**
- **File size: branching_compiler.cpp at 547/500 (over budget after string ops)**
- **PRD coverage: ~35-40%**

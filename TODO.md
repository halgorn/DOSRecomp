# TODO

Status: tracked against updated PRD (C++ backend primary; LLVM/SSA future).

Legend: [x] done, [~] partial, [ ] todo, [b] blocked

## INT 21h (DOS API)
- [x] AH=01h read char
- [x] AH=02h write char
- [x] AH=09h write string ($)
- [x] AH=0Ah buffered input
- [x] AH=2Ah get date
- [x] AH=2Ch get time
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

## INT 10h (Video)
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

## INT 16h (Keyboard)
- [x] AH=00h read key
- [ ] AH=01h key available
- [ ] AH=02h get shift flags
- [ ] AH=05h push key
- [ ] AH=10h/11h/12h extended

## INT 13h (Disk)
- [x] AH=00h reset
- [x] AH=02h read sectors
- [x] AH=03h write sectors
- [ ] AH=08h get params
- [ ] AH=0Eh read status
- [ ] AH=15h get DASD type
- [ ] AH=16h disk change

## Decoder
- [x] 0x00-0x3F ALU, move, branch
- [x] 0x40-0x7F inc/dec, cond jumps, MOV imm
- [x] 0x80-0x8F ALU r/m,imm + group 1
- [x] 0x90-0xBF XCHG, MOV, MOV imm, LEAVE, RET
- [x] 0xC0-0xCF shifts, RET, INT
- [x] 0xD0-0xDF shifts, AAM, XLAT, ESC, LOOP
- [x] 0xE0-0xEF LOOP, JMP, IN, OUT
- [x] 0xF0-0xFF LOCK, REPNE, REP, HLT, CMC, grp2/3/4/5
- [b] 0x0F two-byte opcodes (POP CS, MOV TR, etc.)
- [ ] 0x66 operand size prefix (32-bit)
- [ ] 0x67 address size prefix
- [ ] LEA/LES/LDS/BOUND
- [ ] XLAT, XCHG mem, atomic XADD
- [ ] BCD adjust: AAM, AAD, DAA, DAS, AAS

## Compiler (C++ backend)
- [x] Straight-line C++ emitter
- [x] Branching C++ emitter (block fusion)
- [x] Runtime CALL/RET (256-deep stack)
- [x] Runtime segment dispatch
- [x] Inline block functions
- [x] High-level if/else from Jcc
- [x] High-level do/while from backward Jcc
- [x] Direct memory access (no MMU)
- [x] Byte/word register coherence
- [x] INT 0x21 as block terminator
- [x] INT 0x21 last-in-program skip
- [~] MZ EXE support — loader reads entry, relocations not applied
- [ ] Segment register tracking (DS:SI, ES:DI as 16:16)
- [ ] Function recovery (prologue detection)
- [ ] Data section recovery
- [ ] String ops (MOVS, STOS, LODS, CMPS, SCAS)
- [ ] I/O port (IN/OUT) — silently dropped today
- [ ] PUSHA/POPA, ENTER, LEAVE
- [ ] High-level while/for/switch
- [ ] REP/REPNE prefix
- [ ] CLI flag: --keep-cpp (write generated C++ to disk)

## Tooling
- [x] dosrecomp CLI (compile .com → elf)
- [x] CTest (33/33 passing)
- [x] Build under 2 seconds
- [ ] CI (GitHub Actions)
- [ ] AddressSanitizer run
- [ ] clang-tidy
- [ ] Fuzzing harness (decoder)
- [ ] benchmark.json (compile + run time)
- [ ] example run script (real .com → recompile → run)

## Docs
- [x] PRD.md (updated 2026-07-13: C++ backend primary)
- [x] docs/architecture.md
- [x] README.md
- [x] TODO.md (this file)
- [ ] examples/ (real DOS .com recompiled)
- [ ] CONTRIBUTING.md
- [ ] CHANGELOG.md
- [ ] API mapping table (INT 21h/10h/16h/13h → runtime helper)

## Future (Multi-arch, requires LLVM backend — §8)
- [ ] SSA construction
- [ ] IR + LLVM IR generator
- [ ] ARM64 backend
- [ ] RISC-V backend
- [ ] WebAssembly backend

## Non-Goals (PRD)
- [ ] Don't emulate CPU — OK
- [ ] No DOS runtime at runtime — OK
- [ ] No VM — OK

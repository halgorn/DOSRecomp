# TODO — Next Actions

Companion to `PROGRESS.md` (status) and `PRD.md` (vision).
Last updated 2026-07-13.

## Now (this session)

- [ ] Trim `branching_compiler.cpp` from 547 to ≤500 lines
  - Compact `emit_string_op` (one-line lambda bodies)
  - Collapse `emit_arithmetic` default case
  - Merge runtime header `out <<` calls
- [ ] Add regression test: `rep movsb` program (hand-assembled nasm)
- [ ] Verify 33/33 ctest still passes after trim
- [ ] Commit as: `string ops + REP prefix + CLD/STD + 500 lines`

## This week

- [ ] 0x66 operand size override (32-bit immediate in 16-bit mode)
- [ ] 0x67 address size override
- [ ] CLI flag `--keep-cpp` (write generated C++ next to ELF)
- [ ] AddressSanitizer run on existing tests
- [ ] First real DOS utility in `examples/` (e.g. `DEBUG.COM` or `CHKDSK.COM`)

## This month

- [ ] MZ EXE relocation application at load time
- [ ] INT 21h AH=19h, 0Eh (drive select) — easy, 1 syscall each
- [ ] INT 10h AH=0Ch/0Dh (write/read pixel) — needed for mode 13h
- [ ] INT 10h AH=00h (set video mode) — needed for any graphics
- [ ] INT 16h AH=01h (key available) — non-blocking poll
- [ ] Decoder: 0x0F two-byte opcodes
- [ ] String: PUSHA/POPA, ENTER, LEAVE

## Next month

- [ ] Function recovery (prologue/epilogue detection, named functions)
- [ ] Switch statement recovery
- [ ] while/for loop detection (compare to entry-of-block, not just back-edge)
- [ ] ELF .symtab generation with original offsets
- [ ] CI on GitHub Actions
- [ ] clang-tidy
- [ ] Fuzzing harness for decoder

## When someone asks

- [ ] Mode 13h (320x200, 256 colors) — needs SDL3 + palette mapping
- [ ] Audio (PC Speaker, Sound Blaster) — needs SDL3 audio
- [ ] Native ARM64 — requires LLVM backend (§8)
- [ ] Native RISC-V — requires LLVM backend
- [ ] WebAssembly — requires LLVM backend

## Decision pending

- [ ] ASM/RPC/Loader for `examples/` (which real .com to start with?)
- [ ] License header (MIT per PRD — but year 2026?)
- [ ] Version bump to 0.2.0? (we have a feature: C++ backend primary)

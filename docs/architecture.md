# Architecture

## Loader

The loader is the trust boundary for executable input. It accepts a byte stream,
detects MZ signatures, and exposes a `program_image` without executing any
instruction. It validates header sizes, declared file sizes, relocation-table
bounds, relocation targets, and the initial MZ entry point.

COM input is a single 64 KiB-or-smaller load module with entry offset `0100h`.
MZ input excludes the header from `program_image::bytes`; relocation addresses,
the initial stack, and the entry point remain relative to the load module.
`program_image::entry_offset()` converts that DOS logical entry address into the
byte-vector index required by the decoder and CFG; COM's conventional `0100h`
origin therefore maps to index zero.

The loader has no dependencies beyond the C++ standard library. Decoder and CFG
modules will consume `program_image`; applying relocation values requires a
chosen load segment and is provided by `binary_loader::apply_relocations`, which
returns a relocated copy of the image.

Current limitations: only COM and real-mode MZ containers are recognized. DOS
extenders, protected mode, and malformed headers are rejected or unsupported.

## Decoder

The decoder is a separate, pure module. Its first CFG-oriented subset recognizes
instruction boundaries for NOP, immediate MOV, INT, near CALL/JMP, short Jcc,
LOOP-family branches, and near RET. Unsupported opcodes are returned as errors,
so control-flow analysis can never continue based on a guessed boundary.
It also determines boundaries for common arithmetic, data-movement, stack, and
immediate ModR/M encodings, including 16-bit addressing displacements.

## CFG

The CFG builder follows reachable direct edges from an explicit load-module
entry offset. It creates basic blocks at control-flow boundaries and reports a
decoder failure or external branch target as an error. Indirect control flow,
switch-table recovery, and function discovery depend on broader operand support.

## Function recovery

Function recovery identifies the program entry plus direct near-call targets.
It records proven call sites and deliberately never promotes a jump target to a
function. Indirect calls and shared-tail recovery remain unsupported pending
complete operand decoding.

Loop recovery identifies direct backward `JMP`, conditional-jump, and `LOOP`
edges, reporting their header and latch offsets. It intentionally reports CFG
facts rather than guessing whether a loop originated as `for`, `while`, or
`do-while` source syntax.

## Backend

The initial ELF backend emits a structurally valid ELF64/x86_64 image with one
loadable executable segment and a Linux `exit` syscall stub. It establishes
checked byte-order and segment-layout primitives; lowering recovered DOS
semantics into this image remains the next backend stage.

## Compiler pipeline

The first vertical compiler slice accepts a loaded image whose entry sequence
is exactly `MOV AX, 4Cxxh; INT 21h`, translates the register definition into
SSA, and emits a native ELF that exits with `xx`. It is intentionally explicit
about this boundary: every other program is rejected rather than compiled with
changed semantics.

## IR

The IR module lowers validated CFG block starts into stable numeric IDs and
explicit `stop`, `jump`, or `branch` terminators. It deliberately contains no
machine-dependent operands; instruction-to-SSA lowering will add typed values
and phi nodes above this verified control-flow foundation. The module depends
only on CFG and rejects dangling or ambiguous successor relationships.

`register_ssa_builder` models the nine 8086 architectural registers as
immutable versions. It introduces entry values, definitions, and phi values at
control-flow joins. Instruction semantics will use this builder to translate
register reads and writes without mutating historical values.

## Semantics

The semantics module is the only layer permitted to assign machine meaning to
decoded bytes. Its initial `MOV r16, imm16` implementation updates register SSA
and preserves the literal immediate. Any other encoding produces a contextual
error; this prevents accidental execution or invented program behavior while
coverage grows.

## Runtime

The runtime is host-state-free at its API boundary. `INT 21h` currently maps
function `02h` (character output), `09h` (bounded `$`-terminated string output),
and `4Ch` (process exit) to observable process state. String pointers and
terminators are checked before use. Filesystem, date/time, memory, and input
services remain separate additions.

`virtual_drive` maps one absolute DOS drive into an explicit host root. It is a
lexical sandbox: other drives, relative paths, and `.`/`..` components are
rejected before a filesystem operation is attempted.

## Optimizer

Constant propagation consumes immutable SSA values and computes facts without
rewriting them. Direct constant definitions are preserved and phi values fold
only when every incoming value is the same known 16-bit constant; malformed or
forward SSA references are rejected.

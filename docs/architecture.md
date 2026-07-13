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
chosen load segment and is intentionally deferred to the runtime/link stage.

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

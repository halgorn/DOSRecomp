# Architecture

## Loader

The loader is the trust boundary for executable input. It accepts a byte stream,
detects MZ signatures, and exposes a `program_image` without executing any
instruction. It validates header sizes, declared file sizes, relocation-table
bounds, relocation targets, and the initial MZ entry point.

COM input is a single 64 KiB-or-smaller load module with entry offset `0100h`.
MZ input excludes the header from `program_image::bytes`; relocation addresses,
the initial stack, and the entry point remain relative to the load module.

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

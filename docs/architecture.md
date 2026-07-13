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
For `MOV r8/r16, immediate` and register/memory ModR/M MOV forms, it
additionally emits typed operands; the semantics layer consumes those decoded
fields rather than re-reading instruction bytes. Memory forms retain their
validated ModR/M encoding plus decoded 8086 base/index registers and signed
displacement for later effective-address lowering.
Accumulator-immediate arithmetic and comparison forms also expose their
accumulator and immediate operands, providing the verified input for flag and
branch semantics.
Short conditional branches preserve their exact 8086 condition (`JE`, `JNE`,
signed and unsigned relations, and flag tests), rather than exposing only a
generic conditional edge. `LOOP`, `LOOPE`, `LOOPNE`, and `JCXZ` likewise
preserve their distinct CX/flag conditions through CFG lowering.
8086 segment, LOCK, and REP prefix combinations are included in instruction
boundaries and are bounded to the architectural 15-byte instruction maximum.
Far-pointer loads and 8087 escape encodings are also bounded and labeled, but
their operand semantics remain deferred to the semantic translation layer.

## CFG

The CFG builder follows reachable direct edges from an explicit load-module
entry offset. It creates basic blocks at control-flow boundaries and reports a
decoder failure or external branch target as an error. Indirect control flow,
switch-table recovery, and function discovery depend on broader operand support.
Block starts are discovered to a fixed point, so a branch into the middle of a
sequential region causes a proper split rather than overlapping blocks.

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

The ELF backend emits structurally valid ELF64/x86_64 images with one loadable
executable segment. Its verified stubs support Linux `exit` and `write` then
`exit`, with embedded payload bytes addressed through RIP-relative code. This
is intentionally a small syscall boundary; general native lowering remains a
later stage.

## Compiler pipeline

The compiler accepts `MOV AX, 4Cxxh; INT 21h` or the equivalent explicit
byte-register sequence using `MOV AH, 4Ch` and `MOV AL, xx` in either order.
It also compiles a COM entry sequence `MOV DX, offset; MOV AH, 09h; INT 21h`
when followed by the word-register exit form. The offset is required to remain
inside the COM image and its string must have a `$` terminator; the generated
ELF writes only the bytes before that terminator and exits with `xx`. NOP
padding around exit instructions is accepted. Every other shape is rejected
rather than changing semantics.

## IR

The IR module lowers validated CFG block starts into stable numeric IDs and
explicit `stop`, `jump`, or `branch` terminators. It deliberately contains no
machine-dependent operands, but preserves exact decoded branch conditions on
conditional terminators; instruction-to-SSA lowering will add typed values
and phi nodes above this verified control-flow foundation. The module depends
only on CFG and rejects dangling or ambiguous successor relationships.

`register_ssa_builder` models the nine 8086 architectural registers as
immutable versions. It introduces entry values, definitions, and phi values at
control-flow joins. It also records explicit add/subtract/bitwise operation
definitions, so instruction semantics can preserve arithmetic meaning rather
than treating every write as an opaque version. Instruction semantics will use this builder to translate
register reads and writes without mutating historical values.

## Semantics

The semantics module is the only layer permitted to assign machine meaning to
decoded instructions. Its initial `MOV r16, imm16` implementation consumes the
typed decoder operands, updates register SSA, and preserves the literal
immediate. It also models `MOV r16, r16` as an SSA definition that depends on
the source version rather than inventing a constant. Any other encoding produces a contextual error; this prevents
accidental execution or invented program behavior while coverage grows.
`ADD`, `SUB`, `AND`, `OR`, and `XOR` accumulator-immediate word forms lower to
explicit SSA operations with an immutable literal input. Carry-dependent and
flag-only operations remain rejected until flags have first-class semantics.

## Runtime

The runtime is host-state-free at its API boundary. `INT 21h` currently maps
function `02h` (character output), `09h` (bounded `$`-terminated string output),
and `4Ch` (process exit) to observable process state. String pointers and
terminators are checked before use. Filesystem, date/time, memory, and input
services remain separate additions.

`INT 10h` currently provides text-mode cursor placement (`AH=02h`) and
teletype output (`AH=0Eh`) through a host-independent terminal state. Video
memory, modes, and palette services remain separate runtime additions.

`mode13_framebuffer` provides the memory model for 320×200 indexed pixels and
a 256-entry palette, with checked coordinate access. SDL or another presenter
can consume this isolated surface in a later backend.

`INT 10h/AH=00h` selects either BIOS text mode `03h` or Mode 13h `13h` in a
separate mode state; unsupported video modes are rejected explicitly.

`pc_speaker` models validated PIT-range tone frequency and active state without
choosing a host audio backend. A future SDL/audio presenter can consume this
state without changing DOS-visible behavior.

`INT 16h` provides deterministic keyboard read (`AH=00h`) and availability
probe (`AH=01h`) over a caller-owned queue, keeping platform input policy out
of translated program semantics.

`bios_timer` provides a caller-driven 18.2 Hz BIOS tick counter with a
deterministic midnight rollover count. `INT 1Ah/AH=00h` returns the current
ticks and consumes that count, so a translated program never needs to read a
host clock to observe BIOS time.

`INT 13h` provides validated `AH=02h` CHS reads from an in-memory virtual disk
mapped as drive `80h`. Geometry, sector ranges, and image bounds are checked
before data is returned.

`conventional_memory` implements the 640 KB DOS pool as a first-fit allocator
in 16-byte paragraphs. It rejects zero or exhausted allocations, validates
release ownership, and coalesces adjacent free blocks.

`INT 21h` exposes that model through `AH=48h` allocation and `AH=49h` release,
returning an allocated segment only after the request is validated.

Date (`AH=2Ah`) and time (`AH=2Ch`) queries consume an injected `dos_clock`, so
host policy supplies the current time while translated behavior remains
deterministic and testable.

`virtual_drive` maps one absolute DOS drive into an explicit host root. It is a
lexical sandbox: other drives, relative paths, and `.`/`..` components are
rejected before a filesystem operation is attempted. Canonical containment is
also checked, so a symlink beneath the root cannot redirect a guest path
outside the drive.

`virtual_file_system` layers DOS-style read/write handles over that drive for
bounded reads, writes, and close operations. It starts handles at five,
rejects invalid or mode-incompatible handles, and never resolves a path outside
the drive.

`current_directory` tracks a guest-visible DOS path only after the matching
virtual-drive directory exists, keeping host paths out of program-visible
state and rejecting traversal attempts.

`int21_directory_dispatcher` maps DOS `3Bh` and `47h` requests onto that
sandboxed state without exposing any host path.

`dos_environment` stores guest-only variables with case-insensitive DOS keys.
It does not inherit host environment variables and validates keys and values
before retaining them.

`int21_file_dispatcher` maps DOS `3Ch`, `3Dh`, `3Eh`, `3Fh`, `40h`, and `42h`
requests directly to those virtual handles. Seek accepts the DOS begin/current/
end origins, a signed 32-bit displacement, and rejects negative or out-of-range
result positions. Path decoding remains a caller responsibility, keeping the
interrupt boundary independent of guest-memory representation.

## Optimizer

Constant propagation consumes immutable SSA values and computes facts without
rewriting them. Direct constant definitions are preserved and phi values fold
only when every incoming value is the same known 16-bit constant; malformed or
forward SSA references are rejected. It also folds known 16-bit add, subtract,
and bitwise SSA operations with defined modular arithmetic.

Dead-value analysis starts at explicit observable roots and marks their SSA
dependencies live. It validates dense IDs and input bounds, producing a
non-mutating liveness mask for a later IR rewrite pass.

## Plugins

The plugin registry defines typed extension points for instruction decoders,
optimizers, DOS API mappings, loaders, and output generators. It manages
validated in-process metadata today; dynamic loading can be layered onto this
stable contract without coupling compiler modules to a loader mechanism.

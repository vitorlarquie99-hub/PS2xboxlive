<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:0fa96cf4cca455e654b44e0e146597fd82757adb6f078710d0b7d46a883fed64 -->

# Shadow of the Colossus Recomp for Xbox Series

An experimental project to recompile the PlayStation 2 version of **Shadow of the Colossus** (`SCUS_974.72`) and eventually run it in an Xbox Series environment. The intended route is a UWP application in Xbox Developer Mode, pending platform and dependency validation.

> **Status: research and early boot investigation.** There is no playable build, Xbox package, or validated console release. The Linux diagnostic runs described in the reports do not establish Xbox compatibility.

## What is a recomp?

A static recompiler translates the game's PS2 R5900 instructions into C++ ahead of time. The C++ is compiled for a host computer and runs with a runtime that supplies PS2 memory, IOP services, graphics, audio, input, and file access. This repository builds on the experimental [PS2Recomp](https://github.com/ran-j/PS2Recomp) toolchain. Translating the executable is only one part of the work: this game also loads XFF modules and relocates instructions at runtime.

The targeted executable on the disc is named `SCUS_974.72`; it is an ELF even though its filename has no `.elf` extension. The investigated copy has SHA-256 `d16ad76ba8cdf87695d94b1316d51c1f248959ec0749814c31bb7a2d55d9a2e1`. Keep game files, memory captures, and generated game code outside this repository.

## Project guide

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:35dd878e6b8fb250f89b9cf9f19deb29dc137f5aed81b6ad12ee2e30743a0d6a -->

- [Guia de continuidade em português](docs/GUIA_CONTINUIDADE.md)
- [Game-specific setup and investigation](shadow_of_the_colossus/README.md)

<!-- /coderabbit-ai-generated:v1 -->
- [Change history](CHANGELOG.md) and [detailed investigation report](docs/reports/2026-11-01-initial-investigation.md)
- [Releases](releases/README.md) — release information and the future download location
- [IOP runtime](ps2xIOP/README.md), [analyzer](ps2xAnalyzer/Readme.md), and [runtime](ps2xRuntime/Readme.md)

## Work remaining

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:d5b0f2324acc40a4e778e864ab12fa482994b8daa38d6caf28ffe6e1d53123d7 -->

Selected XFF relocation fields now use live guest RAM values (LUI, ADDIU, LW, SW and J/JAL); this does not provide arbitrary self-modifying code or movable module layouts. Fresh boot/KERNEL/MANAGER/GAMECORE snapshots were reconstructed and compiled; see the [current reconstruction report](docs/reports/2026-11-01-boot-reconstruction.md). Normal IOP reboot, a reliable game boot, graphics, audio, input, storage, an Xbox-compatible runtime and package, and testing on a real console remain outstanding.

<!-- /coderabbit-ai-generated:v1 -->

## PS2Recomp toolchain reference

<!-- /coderabbit-ai-generated:v1 -->

[![Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white)](https://discord.gg/JQ8mawxUEf)

Also check our [WIKI](https://github.com/ran-j/PS2Recomp/wiki)


This project statically recompiles PS2 ELF binaries into C++ and provides a runtime to execute the generated code.

### Modules

* `ps2xAnalyzer`: scans ELF/functions and writes TOML config (`stubs`, `skip`, instruction patches).
* `ps2xRecomp`: reads TOML + ELF, decodes R5900 instructions, and generates C++ output.
* `ps2xRuntime`: hosts memory, function registration, syscall dispatch, and hardware stubs.
* `ps2xIOP`: R3000A IRX execution, a virtual IOP kernel, and generic HLE fallbacks.

### Features

* Translates MIPS R5900 instructions to C++ code
* PS2-specific MMI and VU0 macro support.
* Single-file or multi-file output.
* Configurable stubs, skips, and instruction patches.
* Instruction-driven syscall handling.

### How It Works
PS2Recomp works by:

* Parsing a PS2 ELF file to extract functions, symbols, and relocations
* Decoding the MIPS R5900 instructions in each function
* Translating those instructions to equivalent C++ code
* Generating a runtime that can execute the recompiled code

The translated code is very literal, with each MIPS instruction mapping to a C++ operation. For example, `addiu $r4, $r4, 0x20` becomes `ctx->r4 = ADD32(ctx->r4, 0X20);`.

### Current Behavior

* `stubs` entries generate wrappers that call known runtime syscall/stub handlers by name.
* `stubs` also supports address bindings with `handler@0xADDRESS` for stripped games (for example `sceCdRead@0x00123456`).
* Address bindings also support generic return handlers for triage: `ret0`, `ret1`, `reta0`.
* Recompiler now tries relocation-symbol auto-binding at callsites (`J/JAL`) before raw address dispatch; when relocation symbol is known (for example `sceCdRead`), it can call runtime handlers without manual address mapping.
* Recompiler discovers additional internal static entry targets and emits `entry_...` wrappers for those addresses.
* For unresolved static `J/JAL` sites, generated code falls back to `runtime->lookupFunction(0x...)`.
* `skip` entries are not recompiled and generate explicit `ps2_stubs::TODO_NAMED(...)` wrappers.
* Recompiled `SYSCALL` now calls `runtime->handleSyscall(...)` with the encoded syscall immediate.
* Runtime syscall dispatch tries encoded syscall ID first, then falls back to `$v1`.

### Requirements

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:03331a7c80d932e461d0b30d18fd9da143d01f684435914c196ec11951e17394 -->

* CMake 3.21+

<!-- /coderabbit-ai-generated:v1 -->
* C++20 compiler (currently tested mainly with MSVC)
* SSE4/AVX host support for some vector paths

### Build

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:983beaf13edfc183b380d8d23e14ea99c466b10b34dbab220dee807d46b8bdf3 -->

<!-- coderabbit-ai-generated-section:v1:e57fabaec1977e9f41aa9bf5c70c580a72acc9642c6dedede0f7a595f3ba3339 -->

<!-- /coderabbit-ai-generated:v1 -->

```bash
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:16e61ff5b18ea993ed1e3d82e7050306e6d64c2100be509921c0e5414a92b1a2 -->

git clone --recurse-submodules https://github.com/vitordeser99-cmd/PS2xbox.git
cd PS2xbox

<!-- /coderabbit-ai-generated:v1 -->
cmake -S . -B out/build
cmake --build out/build --config Debug
```

<!-- /coderabbit-ai-generated:v1 -->

### Usage

Preferred workflow for retail or stripped games:

1. Open the ELF in Ghidra.
2. Run `ps2xRecomp/tools/ghidra/ExportPS2Functions.java`.
3. Use the exported TOML and CSV map.
4. Recompile with the exported TOML:

```bash
./ps2_recomp config.toml
```

Fallback workflow for quick local experiments or ELFs with debug symbol :

```bash
./ps2_analyzer your_game.elf config.toml
```

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:3b5197bced249500687b86a40d10538ed8b441c173e8dc39530cc9483a9dca39 -->

See the [Ghidra integration guide](ps2xAnalyzer/Readme.md#4-ghidra-integration) for instructions.

<!-- /coderabbit-ai-generated:v1 -->

Then build generated output and link with `ps2xRuntime`.

### Configuration

Main fields in `config.toml`:

* `general.input`: source ELF path.
* `general.ghidra_output`: recommended function map CSV exported from Ghidra.
* `general.output`: generated C++ output folder.
* `general.single_file_output`: one combined cpp or one file per function.
* `general.low_memory_mode`: reduce peak output-generation memory by avoiding retained disassembly strings and forcing serial output generation. Generated instruction comments are still emitted; disassembly text is produced while writing each output file instead of being kept in memory.
* `general.output_worker_threads`: number of output-generation workers (clamped to nproc * 2). A positive value uses exactly that many workers. `0` uses `nproc - 1` when at least two hardware threads are available, otherwise serial output generation. `1` forces serial output generation.
* `general.patch_syscalls`: apply configured patches to `SYSCALL` instructions (`false` recommended).
* `general.patch_cop0`: apply configured patches to COP0 instructions.
* `general.patch_cache`: apply configured patches to CACHE instructions.
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:9a155e0c03c269b81db67ebd6d7168f31e2801205d7539a076f0cb4e10dfc0ee -->

* `general.stubs`: names to force as stubs. Also accepts `handler@0xADDRESS` to bind a stripped function address directly to a runtime syscall/stub handler. Includes generic handlers `ret0`, `ret1`, `reta0`.
* `general.recompile_addresses`: aligned guest function starts to recompile instead of selecting automatic or configured stubs, for example `[0x0010A8C0]`. Default empty; explicit `skip` rules still apply.
* `general.live_immediate_addresses`: aligned instruction addresses whose relocation fields must be read from guest RAM. Supports LUI, ADDIU, LW, SW and J/JAL targets; opcode/register fields remain fixed. Default empty. The loader must populate instruction memory before execution. Unsupported opcodes fail generation. See [relocation validation](docs/reports/2026-11-01-boot-reconstruction.md).

<!-- /coderabbit-ai-generated:v1 -->
* `general.skip`: names to force as skipped wrappers.
* `patches.instructions`: raw instruction replacements by address.

Address binding for stripped ELFs:

* Use `handler@0xADDRESS` inside `general.stubs` to map a stripped function start directly to a runtime handler.
* Example: `sceCdRead@0x00123456` binds function start `0x00123456` to `ps2_stubs::sceCdRead(...)`.
* Generic temporary handlers are available: `ret0@0xADDR`, `ret1@0xADDR`, `reta0@0xADDR`.
* Before manual binding, prefer recompilation from a Ghidra-exported TOML/CSV first. The extra boundaries and synthetic entry points are usually more important than manual early triage.
* The address must be the function start in that exact ELF build.
* Addresses are not portable across different games/regions/builds.
* The handler name must exist in runtime call lists (`PS2_SYSCALL_LIST` or `PS2_STUB_LIST`).

Example:

```toml
# stripped function binding by address:
stubs = ["sceCdRead@0x00123456", "SifLoadModule@0x00127890"]
# temporary return handlers:
stubs = ["ret0@0x001D9410", "ret1@0x001D5BC8", "reta0@0x0024B7C0"]
# mixed example:
stubs = ["printf", "sceCdRead@0x00123456", "SifLoadModule@0x00127890"] 
```

### Runtime

To execute the recompiled code.

`ps2xRuntime` currently provides:

* Guest memory model and function dispatch table.
* Some syscall dispatcher with common kernel IDs.
* Basic GS/VU/file/system stubs.
* Foundation to expand and port your game.
* `ps2xIOP` execution of original IRX modules with generic HLE fallbacks.

See [IOP emulation](ps2xIOP/README.md) for module execution and the service boundary.

### Game Override Hooks

Game overrides are runtime-side, build-scoped patch modules.

A game override is C++ code that runs during `loadELF` and can replace EE function bindings by address for one specific game build. IOP RPC/DMA behavior is handled by the `ps2xIOP` emulator and its runtime transport. This is separate from recompilation output and separate from global runtime stubs/syscalls.

API:

* Header: `ps2xRuntime/include/game_overrides.h`
* Register macro: `PS2_REGISTER_GAME_OVERRIDE(name, elfName, entry, crc32, applyFn)`
* Direct bind helper: `ps2_game_overrides::bindAddressHandler(runtime, addr, "handler")`

Use Game Override modules when:

* You need per-game/per-build routing or patches without polluting global behavior.
* You need to bind many addresses, or install custom replacement logic for a specific title.

#### Recommended Iteration Loop

1. Run with minimal config and no aggressive skipping.
2. Fix hard blockers first (`function not found`, syscall TODO, critical IO stubs).
3. Use temporary return stubs only to classify call importance.
4. Promote temporary fixes to real implementations.
5. Move per-game hacks into game overrides keyed by ELF metadata.
6. Re-test from cold boot after each batch.

### Describing Your Game Project

A game project built with PS2Recomp can describe itself in a `.recomp.json` file at the root of its repository. Lists of recomp and decomp projects, such as [recomp.board](https://recomp.fyi), read that file instead of guessing the game, system and status from the README.

> [!NOTE]
> This file is optional: PS2Recomp does not read it and works the same without it. `.recomp.json` and recomp.board are third-party projects; the PS2Recomp developers have no ties to them.

Starter file:

```json
{
  "$schema": "https://recomp.fyi/schema/v1.json",
  "game": "<title as it shipped>",
  "system": "PS2",
  "type": "recomp",
  "toolchain": "PS2Recomp",
  "status": "in-progress",
  "original": { "region": "USA", "serial": "SLUS-20312" }
}
```

* `original` is the release a user must own. On retail discs the ELF is named after the serial (`SLUS_203.12` is `SLUS-20312`), and the prefix gives the region: `SLUS`/`SCUS` USA, `SLES`/`SCES` Europe, `SLPS`/`SLPM`/`SCPS` Japan.
* `status` is one of `exploring`, `in-progress`, `playable`, `released`, `complete`, `paused`. Edit it when the project moves on: a stale status is worse than none.
* Never put a game file, or a link to one, in the file.

Other fields (Wikidata item, target platforms, maintainers, links, what help is wanted) and the JSON Schema are in the [specification](https://recomp.fyi/spec).

### Limitations
 
* Performance is very bad for VU and GS
* Hardware emulation is partial and many paths are stubbed.

###  Acknowledgments

* Inspired by N64Recomp
* Uses ELFIO for ELF parsing
* Uses toml11 for TOML parsing
* Uses fmt for string formatting
* Reference for runtime PCSX2
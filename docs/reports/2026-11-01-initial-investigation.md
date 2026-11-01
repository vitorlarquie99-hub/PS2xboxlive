<!-- coderabbit-ai-generated:v1 -->

# Investigation report — 2026-11-01

## Objective and status

The goal is to run the PS2 release of Shadow of the Colossus on Xbox Series, provisionally through a UWP application in Developer Mode. The repository inherits the experimental PS2Recomp analyzer, recompiler, runtime, and IOP implementation. This report describes development-computer investigations; it is not evidence of a playable game or an Xbox build.

## Disc identification

The supplied full-disc archive was extracted outside the repository after checking archive paths and links; ZIP extraction checked member CRCs. `SYSTEM.CNF` identifies `SCUS_974.72` as the boot file. Although that name does not end in `.elf`, its contents are an ELF executable. The tested ELF SHA-256 is `d16ad76ba8cdf87695d94b1316d51c1f248959ec0749814c31bb7a2d55d9a2e1`. The previously provided boot and XFF files matched the corresponding full-disc files byte for byte. These observations identify the tested inputs, not all releases bearing the same serial.

The disc also contains `STARTUP.XFF`, `KERNEL.XFF`, `MANAGER.XFF`, `GAMECORE.XFF`, `IOPRP300.IMG`, IRX modules, and game data. Disc files, generated game code, and RAM captures remain outside the repository.

## Inspection and relocation work

`shadow_of_the_colossus/inspect_elf.py` checks ELF header details. `audit_xff.py` inventories imports and relocation tables, checks original words, and checks HI16/LO16 grouping. `xff_relocations.py` implements the arithmetic for relocation types 2, 4, 5, and 6 given resolved symbol values. Its arithmetic was compared with a separate C++ reference using synthetic symbol values across the three XFF files. It does not choose addresses or resolve symbols, and it is not integrated into the runtime.

The reports use [Celosia's loader implementation](https://github.com/HenryKun55/celosia/blob/df6b12b69e8c8c1d0012f5ac5b834ddb818bb2c2/game/overrides/decomp_reloc.cpp) as a reference for loader behavior and the [sotc-decomp XFF documentation](https://github.com/TheRealestNwah/sotc-decomp/blob/ac246f0ad20f937ef0b0c4544c96db45ee44e203/docs/xff2.md) for format investigation. Matching a symbol by name alone does not prove runtime resolution.

`extract_boot_symbols.py` extracts candidate absolute symbols from `KERNEL.XFF`. A pinned Celosia function mapping tool estimated additional boot boundaries. `prepare_boot_config.py` checks the boot ELF hash and preserves guest code for three incomplete stub bindings. Function boundaries remain heuristic and need runtime validation.

## Runtime and IOP changes

The runner accepts `PS2X_RUNNER_GENERATED_DIR` to compile generated C++ from a dedicated directory. The diagnostic x86 runner also enables SSE4.1 because generated VU code uses those intrinsics. Neither change creates an Xbox build.

`inspect_ioprp.py` lists an IOPRP ROMDIR and extracts a named module without overwriting files. The original `FILEIO` module was extracted from the supplied IOPRP image for diagnostics. The IOP implementation gained legacy CD-ROM `getstat` and the observed `devctl` command `0x4391`, which returns the existing CD/DVD interrupt event flag. Unknown devices and commands remain unhandled. This does not implement normal IOP reboot from the image.

An opt-in `PS2X_CAPTURE_MISSING_DIR` feature records guest RAM and source/target PCs on the first missing function. It refuses an existing destination, uses the actual allocated RAM size, and leaves the existing missing-function policy intact. Captures contain game data and stay outside the repository.

## Boot experiments

The initial Linux runner using the full disc reached an incomplete `sceFsSemInit` stub. A separate diagnostic configuration enabled original guest code at selected locations. A scratch runtime manually loaded copies of `FILEIO`, `LIBSD`, and `SG2IOPM1`; these changes were confined to the diagnostic runtime and do not represent normal IOP startup.

The diagnostic run captured the loaded `KERNEL.XFF` and rebuilt it with the loader. A subsequent run reached `MANAGER.XFF`; using a captured fixed-address image of that module reached the first indirect call into `GAMECORE.XFF`. These captures used the memory layout of those executions only. The game did not reach a playable state.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:dcfe70e1a0fdc2aeff6dd960cb6036abd46b8fcd11b5bf647baafe8386bb2df7 -->

The combined captured images yielded 26,624 text import relocation sites across the loaded modules. Seven 32-bit words in the manager text differed between the snapshots taken before and after loading GAMECORE. A fixed snapshot cannot model these runtime changes. A diagnostic runner containing all three captured modules compiled on Linux, but two runs timed out after the last logged binding attempt for service `0x11223344`; the cause was not determined.

Additional logging in the scratch runtime showed a later run binding that service with a nonzero server descriptor. The run dispatched into the `GAMECORE.XFF` entry at `0x01344788` and found `NICO.DAT` on the virtual disc. It still timed out and did not produce a playable game. Instrumentation and manual IOP module loading can affect execution; these observations do not establish reliable boot or identify the cause of the earlier stalls.

<!-- /coderabbit-ai-generated:v1 -->

## Validation and limitations

- The Python game-tool suite passed 16 tests; the standalone IOP CTest suite passed all four suites; two missing-target capture tests passed.
- The affected Linux runner compiled. The three-module diagnostic generated 8,832 recompiled functions and 187 stubs, with 666 control-flow warnings and no decode errors. Compilation does not establish correct execution.
- The larger Debug `ps2x_tests` suite aborted at an `EeScheduler::assertExecutor()` assertion. A clean checkout at the same base revision aborted at the same point; the suite remains incomplete.
- Linux diagnostics used a virtual display and had no working host audio device. Xbox packaging, graphics, controller support, performance, and console behavior have not been tested.

## Next work

Implement and verify normal IOP reboot and service initialization, and design dynamic handling for relocated instruction immediates and jump targets without assuming fixed module addresses. Then validate module linking, work through remaining boot and hardware behavior, and port the runtime and dependencies to an Xbox-compatible environment. Build and test a package on an actual Xbox Series console before announcing a release.

See the [game guide](../../shadow_of_the_colossus/README.md), [change history](../../CHANGELOG.md), and [releases index](../../releases/README.md).
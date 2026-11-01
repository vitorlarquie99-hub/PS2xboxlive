<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:70c3e5187d74f33b5381d15fa2468e9f547711a2eb9f5dd8cade2e29a6ce2701 -->

# Shadow of the Colossus: recompilation guide

This is an investigation into an Xbox Series release using the experimental PS2Recomp toolchain. There is no working game build or UWP application yet. Xbox Developer Mode is the intended deployment environment, subject to verification of the graphics, audio, controller, file access, and dependency requirements on a console.

## Identify the game version

Extract the ELF from your own game disc and keep the ELF and disc data outside the repository. From the repository root:

```sh
python3 shadow_of_the_colossus/inspect_elf.py /path/to/SCUS_974.72
```

Record the SHA-256, entry point, region, and disc edition. The tool checks the ELF header only; it cannot prove that the file is Shadow of the Colossus or that the game data is complete. Function maps, patches, and addresses are specific to the exact ELF hash.

## Development tools

Install CMake 3.21 or later and a C++20 compiler. To build the analyzer and recompiler on a development computer:

```sh
cmake -S . -B build/sotc-tools -DPS2X_BUILD_RUNTIME=OFF -DPS2X_BUILD_TEST=OFF -DPS2X_BUILD_STUDIO=OFF
cmake --build build/sotc-tools --config Release --target ps2_analyzer ps2_recomp
```

For a stripped retail ELF, import it into Ghidra and use `ps2xRecomp/tools/ghidra/ExportPS2Functions.java`; see the [analyzer guide](../ps2xAnalyzer/Readme.md). Review its TOML and CSV output before invoking `ps2_recomp`. The native analyzer is another starting point, but its function boundaries are heuristic. Executables may be in configuration subdirectories depending on the CMake generator.

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:94fc7526100dfd47fd9b85477274f04330514fe52886f33e739544ae38e101fa -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:282f0200bafc314c371c5b237864c04bf4814af15970e235e4b34927d75822f3 -->

## Inventory XFF modules

Keep the files outside the repository and inspect imports from all three modules of the same edition:

```sh
python3 shadow_of_the_colossus/audit_xff.py /path/KERNEL.XFF /path/MANAGER.XFF /path/GAMECORE.XFF > audit.json
```

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:a3b9ecccc43bc4a2ae8ecc734ff94366745202025a3fcecab67a07bb100e77fe -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:2f156b32a40331289e6af196f0a6dc49f8f3f5258b8885ddddb74ae972594c4f -->

The report lists name-matched candidate definitions and counts numerical relocation types. A candidate does not prove the loader will use it: load order, absolute symbols, other modules, and relocation rules still require verification. A missing candidate is not proof that an import cannot be resolved.

The audit checks saved original words and that every HI16 sequence is followed by a LO16 in the same table, following the behavior described by [Celosia's loader](https://github.com/HenryKun55/celosia/blob/df6b12b69e8c8c1d0012f5ac5b834ddb818bb2c2/game/overrides/decomp_reloc.cpp). This is a structural check; it does not validate runtime addresses, resolved symbols, or relocation results. It neither applies relocations nor generates executable code.

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:37b46657b372c28b731db9044beea96e8fca02e68efaadd7c291a0aba3b9f0c6 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:8b7952cd567e162ec92834655a962a03445b8a5002100688d18fef932b990399 -->

The experimental `xff_relocations.py` implements types 2, 4, 5, and 6 using original words. It accepts a section, an ordered relocation table, and already resolved symbol values, then returns a copy. It does not determine addresses or resolve imports and is not integrated into the runtime. Matching names alone is insufficient to determine runtime addresses.

The supplied cover is stored as `assets/cover.jpg`. Integrating it requires an Xbox application package.

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:5c6293711a5176eeb56b24c31d8c44c16fa2a190c2797a212cb146cda0628954 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:8a3aa8c216b4bbaa9840903f649205d7843a41cc62321a9a3a83c70a3104c160 -->

The [sotc-decomp XFF documentation and converter](https://github.com/TheRealestNwah/sotc-decomp/blob/ac246f0ad20f937ef0b0c4544c96db45ee44e203/docs/xff2.md) describe conversion to relocatable ELF. Relocatable objects do not contain final runtime addresses; conversion does not replace module linking.

## Build the initial executable on a development computer

After generating C++ for the initial ELF, configure the runtime with the output directory containing `register_functions.cpp`, `ps2_recompiled_functions.h`, and `ps2_recompiled_stubs.h`:

```sh
cmake -S . -B build/sotc-runtime -DPS2X_BUILD_RECOMP=OFF -DPS2X_BUILD_ANALYZER=OFF -DPS2X_BUILD_TEST=OFF -DPS2X_BUILD_STUDIO=OFF -DPS2X_RUNNER_GENERATED_DIR=/path/to/output
cmake --build build/sotc-runtime --config Debug --target ps2EntryRunner
```

Use a dedicated directory for each ELF: all `.cpp` files in it are compiled. The runtime still needs its graphics and FFmpeg dependencies; `-DPS2X_ENABLE_FFMPEG=OFF` uses placeholder video frames for diagnostics. This creates a development computer build, not an Xbox package. Matching disc data and XFF module linking are needed to progress beyond the initial loader.

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:ffca7a02ff7a7beba7bc806c08ba79449423b0f957fe3af560541accf296f435 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:780f6e613987969d2152cbef839cb23f5353b68c167f48910a32941400921988 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:d4c41be22b5af296cb3e3cb04bab649a103c29cd7df3849a2de9c945e1c7a06d -->

## Prepare names for the initial executable

`KERNEL.XFF` contains absolute symbols that can help analyze the boot ELF. Extract them to a local file:

```sh
python3 shadow_of_the_colossus/extract_boot_symbols.py /path/KERNEL.XFF > /path/to/scus-symbols.txt
```

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:bc751ffde62698228e57b3a66a07ce78e5e63c0c600fad34ecab982a01f9f136 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:242eeadec3f3ebd1c802e1f3e7117d3f664b6a09bc19f34117ad9b070880f3bd -->

The list contains candidates, not a complete function map or loaded-module addresses. [Celosia's funcmap_elf.py](https://github.com/HenryKun55/celosia/blob/df6b12b69e8c8c1d0012f5ac5b834ddb818bb2c2/tools/funcmap_elf.py) accepts the list and produces estimated CSV boundaries; it also needs `elfmini.py` from the same project. Verify the map during execution.

With analyzer TOML and the CSV, prepare an experimental configuration for the ELF with SHA-256 `d16ad76ba8cdf87695d94b1316d51c1f248959ec0749814c31bb7a2d55d9a2e1`:

```sh
python3 shadow_of_the_colossus/prepare_boot_config.py --elf /path/SCUS_974.72 --config /path/analyzer.toml --function-map /path/functions.csv --generated-dir /path/output --output-config /path/to/boot.toml
```

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:e50af104f385f4865b8fef8a233f9dfcbc41f5aafbe286af7b1cc901513a64e2 -->

Python 3.11 or later is required. The command checks the ELF hash and refuses to overwrite its output. It removes four incomplete stub bindings and uses `general.recompile_addresses` to retain the original `_sceFsSemInit`, `_sceRpcGetFPacket`, `_sceRpcGetFPacket2` and `sceFsInit` bodies. This also prevents automatic name matching from replacing `sceFsInit` with its placeholder. Existing guest address choices and other analyzer settings are retained. Boot is not complete.

For a diagnostic disc directory, use regular files or hardlinks. The virtual ISO builder intentionally ignores symbolic links, so a symlink-only fixture returns ENOENT even when host file access can follow those links.

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:46b591844090e3a666575e34bb15b6ec110877a30a910e4f93d64400cb563203 -->

## Inspect the IOP reboot image

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:3a7073e8e653b410424d385a91778858c405c72771cbd866fb95c4a82c17a120 -->

To list the ROMDIR of an IOPRP image beginning with `RESET` and `ROMDIR`, or extract a module to a new path outside the repository:

```sh
python3 shadow_of_the_colossus/inspect_ioprp.py /path/IOPRP300.IMG
python3 shadow_of_the_colossus/inspect_ioprp.py /path/IOPRP300.IMG --extract FILEIO --output /path/to/FILEIO.IRX
```

The extractor does not overwrite files.

<!-- /coderabbit-ai-generated:v1 --> <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:187b598682fb4c1e9c7a956e284eb9c370e226a047992e9282d30fc9eb6b65d5 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:a52c2b7eee87343d15c3359f4844a272c3cbc1b4241dd7f4b11972fbdda5808a -->

Extraction does not start the module: IOP reboot and the necessary imports still need runtime support.

## Capture an unrecompiled target

Set `PS2X_CAPTURE_MISSING_DIR` to a **new** directory whose parent exists before starting the runtime. On the first missing target it saves `ram.bin` and `target.txt` there. A capture contains game data and must remain outside the repository. Existing directories are never overwritten. This option does not change execution policy or resolve the missing target; module positions can differ between runs.

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:94e51d21d8429b9c3be4a7f39534c467a7b83b8bad6231db67be7bb9d52e85cb -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:c86818e9b429a83ab2d2e5aff2347ad019599a1721d5836e3b97956d4d7ac7c4 -->

## Full-disc diagnostic status

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:0f0c2fcf2f5bb519e57ea1b454036a737f83dc64fec8ece076252c13819e0a4c -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:b724d807797b0de04ad86f12006deb2e18f12df67ae478ae8e1d3395947ec73a -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:2c85c16c2d74209a379b8d46f6b6457d9d228dbf1d82aee6110824cc346be0fb -->

The supplied `SCUS_974.72` ELF has SHA-256 `d16ad76ba8cdf87695d94b1316d51c1f248959ec0749814c31bb7a2d55d9a2e1`. In an experimental Linux run, the recompiled loader, `KERNEL.XFF`, and `MANAGER.XFF` reached the first jump into `GAMECORE.XFF`; its position was captured. A later instrumented run invoked the recompiled GAMECORE entry and found `NICO.DAT`, then timed out without reaching playable gameplay.

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:381fb6c4973a8591859934169df024ef0c42f044662a8003781405b342473f6d -->

The diagnostics use a runtime that manually starts copies of the original `FILEIO`, `LIBSD`, and `SG2IOPM1` modules. This is not yet the normal IOP boot sequence. Fresh boot/KERNEL/MANAGER/GAMECORE inputs have now been reconstructed from the supplied disc. The combined runner compiles 9,024 mapped functions and uses live reads at 26,624 imported relocation sites. Supported fields are LUI/ADDIU immediates, LW/SW displacements and J/JAL targets; opcode/register fields and host function layouts remain fixed. Captured module addresses are not guaranteed across runs. The [current report](../docs/reports/2026-11-01-boot-reconstruction.md) distinguishes fresh results from historical probes. A playable boot and Xbox execution remain unverified.

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:0b316c125198debaccf88fab0f1c4ac51cd69df37c8c05dbccd80866ea700560 -->

## Next milestones

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->


<!-- /coderabbit-ai-generated:v1 -->
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:a951df40843122bff8b8cd67262be6794ab28bb8a712801bae4242dc9e149c1c -->

1. Validate function boundaries and dynamic relocation handling for the exact ELF and XFF modules.
2. Complete IOP startup and investigate EE, VU, GS, audio, and disc access until a reliable game boot is possible.
3. Adapt and build the runtime and dependencies for UWP, implement controller, graphics, and data access, and test a package on Xbox Series in Developer Mode.

The matching ELF and disc data, plus a Windows UWP development environment and Xbox console, are needed for console validation.

<!-- /coderabbit-ai-generated:v1 -->

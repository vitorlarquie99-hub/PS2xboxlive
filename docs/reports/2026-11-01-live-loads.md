<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:1429d0532dd5dc68ff1e2927b7562b18e74ac3166ae388e2f8d39fc42f038cd8 -->

# Live LW displacement and wider relocation experiment

This report records the earlier implementation. The [boot reconstruction follow-up](2026-11-01-boot-reconstruction.md) extends live relocations to J/JAL and SW and records fresh captures from the supplied disc.

<!-- /coderabbit-ai-generated:v1 -->

## Baseline and implementation

The previous [live immediate implementation](2026-11-01-live-immediates.md) covered LUI and ADDIU, enabled at two confirmed sound initialization sites. Snapshot comparison identified 140 additional LW instructions whose low 16 bits changed after linking. Their opcodes and register fields did not change.

The recompiler now accepts LW in `general.live_immediate_addresses`. It reads the current instruction word from guest memory, sign-extends the low 16 bits as the displacement, adds that displacement to the runtime base register with 32-bit wraparound, and loads through the existing runtime-aware READ32 path. The loaded word retains the original LW contract: sign extension to the guest register's low 64 bits. Register zero handling remains in the existing register-write macro.

The option remains disabled by default. With any configured live sites, inferred memory-address hints and GIF DMA coalescing remain disabled. Opcode/register changes, live stores, direct jump relocations and general self-modifying code remain unsupported. Jump-table inference also still uses captured inputs; this feature does not guarantee arbitrary relocated indirect control flow.

## Expanded diagnostic configuration

A new external configuration selects all 691 instruction sites observed to differ between GAMECORE-entry and worker-sleep captures: 334 LUI, 217 ADDIU, and 140 LW. This is an experimental coverage set for the observed layout, not a complete relocation manifest or a portable game configuration.

Generation processed 9,019 functions: 8,832 recompiled, 187 stubs, no decode failures or reported errors. The existing 666 control-flow warnings remain. Exactly 198 generated files differed from the earlier two-site output. Their prior contents were preserved, only changed files were copied into the active diagnostic build directory, and all active generated files were verified byte-identical to the new output. Game-derived files remain outside the repository.

## Verification

- Recompiler and test target rebuilt successfully.
- All 65 CodeGenerator cases passed, including the new live LW case and existing default/configuration/unsupported-opcode coverage.
- Compiled the exact LW statement from the generated `setDefaultMenuControlButton` routine against production runtime macros. All 16 combinations of signed displacements and loaded-word boundaries passed, including negative displacement and 64-bit sign extension.
- The broader Debug test suite retains its previously reproduced baseline scheduler assertion; these focused passes do not establish a full-suite pass.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:c8c814366d5152c554fb7bc962611f0255506d12fbcb7a8c56998fe4fdc6c2f9 -->

The Linux diagnostic runner rebuilt successfully. A 30-second Xvfb probe entered `GAMECORE.XFF`, located `NICO.DAT`, and preserved the expected 88-byte sound allocation. It timed out with exit 124 without reaching playable gameplay or reporting a new missing function. Later thread snapshots again showed threads 1 and 2 waiting and thread 6 running the loader idle path. This wider immediate coverage did not resolve the observed boot stall; the test does not identify its cause.

The runner still uses scratch-only manual FILEIO/LIBSD/SG2IOPM1 initialization and snapshot-derived function addresses. Neither normal IOP startup nor Xbox compatibility was verified. Direct J/JAL relocation and reliable control flow remain separate requirements.

<!-- /coderabbit-ai-generated:v1 -->

## Remaining work

Normal IOP initialization, comprehensive XFF relocation handling, control flow, reliable game boot, and Xbox packaging remain unfinished. The centered settings overlay remains deferred. No playable or Xbox release is claimed.

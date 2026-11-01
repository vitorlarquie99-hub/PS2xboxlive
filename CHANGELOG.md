<!-- coderabbit-ai-generated:v1 -->

# Change history

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:329bf0c2a69f9c979ac79eb921d8ff22b659141f36c69ef60878155564113493 -->

This log records project work. Dates describe investigation activity, not released versions.

## Reconstructed continuation — 2026-11-01

- Added opt-in guest implementation selection with `general.recompile_addresses`; SOTC boot preparation now retains the real `sceFsInit` path. Default stub and explicit skip behavior are preserved.
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:1eca761c30cebed1faf89771ffeb0c6ab8348fe5ff205ce04bede2c39a3fd399 -->

- Extended live relocation reads to SW and J/JAL with target capture before delay slots and correct link-register ordering.
- Routed unmasked GS CSR events to INTC_GS through the executor IRQ queue, with one handler chain in flight and guest acknowledgement preserved. Added FINISH/SIGNAL scheduler coverage.
- Applied the existing x86 SSE4.1 build requirement to the runtime and its consumers; corrected scheduler initialization in two DMA test fixtures. The updated full C++ suite passed456 cases.

<!-- /coderabbit-ai-generated:v1 -->
- Reconstructed and compiled boot/KERNEL/MANAGER/GAMECORE inputs from fresh captures without requiring old snapshots. Added generator, configuration and runtime execution checks; see the [current report](docs/reports/2026-11-01-boot-reconstruction.md).

<!-- /coderabbit-ai-generated:v1 -->

## Unreleased — 2026-11-01

- Identified the supplied PS2 boot ELF as `SCUS_974.72` and recorded its SHA-256.
- Added game-specific ELF, XFF, relocation, IOPRP, and boot configuration investigation tools.
- Added limited CD-ROM IOP operations needed by the original FILEIO module, plus regression coverage.
- Added an opt-in missing-target RAM capture and support for compiling generated code from a separate directory.
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:aeaefe91e40ef1d4343f41a878c669bd717d7e483d619b49d8671f1cfebd6d20 -->

- Built diagnostic Linux snapshots of the loader and XFF modules; observed entry into `GAMECORE.XFF` and lookup of `NICO.DAT` under a manually prepared diagnostic IOP.

<!-- /coderabbit-ai-generated:v1 -->
- Saved the supplied cover image for a future Xbox application package.
- Added an English project guide, investigation report, and releases index.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:60082aabbcb2a8da9167f2f6860c6c1045bb48671ff638bccef83151776250d2 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:902340362fd06660b612e7b418314a51f627f30b624993adb863ceae999292c7 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:ad53d32508ed2ad1c12d44a27e28454f3961c52f10b5616ba74e4844e02ce711 -->

- Added opt-in live LUI/ADDIU immediate reads to the recompiler, with conservative memory-address and DMA optimization handling, configuration validation, and regression coverage.
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:84c14b42d677c24a642e9279d31e2e7c10ef9148f9a3431599132a55d0d2558e -->

- Extended live immediates to signed LW displacements and generated a diagnostic with 691 observed relocation sites; see the [detailed follow-up](docs/reports/2026-11-01-live-loads.md).
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:0fc91156745a54a02bbcfd744db7f19c60a4f7829678c3b89482151aca776fee -->

- Traced loader recovery to failed IOP sound-buffer allocation; a scratch-only heap partition experiment advanced into graphics initialization. See the [IOP heap investigation](docs/reports/2026-11-01-iop-heap.md).
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:8bd3abc283939d038505c623a9599fe3f9409911b52113b2e0bab27421898247 -->

- Replaced the fixed IOP module/heap partition with shared reservations, freed-block reuse, accurate memory queries and module lifecycle safeguards. Five IOP suites passed; see the [implementation report](docs/reports/2026-11-01-shared-iop-memory.md).
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:58be90f434c9d193a6625171e820d534affbd491c6b75b69a93e5796609d3239 -->

- Extended boot tracing and preserved later RAM captures; distinguished a GAMECORE scheduler yield from module completion. See the [boot continuation investigation](docs/reports/2026-11-01-boot-continuations.md).
- Confirmed sheet initialization resumption, graphics loop progress, and calls to initgcmain/initgcmaingp/initgcchartest; see the [GAMECORE milestone report](docs/reports/2026-11-01-gamecore-init.md).

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

See the [initial report](docs/reports/2026-11-01-initial-investigation.md), [RPC and thread follow-up](docs/reports/2026-11-01-rpc-followup.md), [sound relocation diagnosis](docs/reports/2026-11-01-sound-relocation.md), and [live immediate implementation](docs/reports/2026-11-01-live-immediates.md) for methods, test results, and outstanding work. No playable or Xbox release is claimed.

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->
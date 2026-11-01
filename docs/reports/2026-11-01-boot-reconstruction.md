<!-- coderabbit-ai-generated:v1 -->

# Reconstructed boot and live jump relocations — 2026-11-01

## Changes

`general.recompile_addresses` is an optional array of aligned 32-bit guest function starts. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:8a8cacf6a9dbb8588405bfbe2516c45353c029605bdfc42538f36a2da584cdce -->

At these starts the recompiler retains guest code instead of selecting a configured or automatic runtime stub. Relocation-name call shortcuts targeting these starts also defer to guest dispatch.

<!-- /coderabbit-ai-generated:v1 --> It defaults to empty. Explicit `skip` rules still apply. This preserves the default stub contract for existing configurations.

The SOTC preparation tool now selects four guest implementations, including `sceFsInit@0x0010A8C0`. Simply removing that stub binding was insufficient: the recompiler also recognized the function name automatically. The placeholder returned success without establishing the guest FILEIO connection. No semaphore ABI or completion shortcut was introduced.

The existing live relocation option previously accepted LUI, ADDIU and LW. It now also supports SW and J/JAL. SW uses the live signed 16-bit displacement with the original register fields. J/JAL read the live 26-bit target, combine it with the high nibble of PC+4, capture it before the delay slot, and dispatch through the runtime. JAL writes PC+8 into RA before the delay slot; J preserves RA. Live calls bypass captured import-name shortcuts. Opcode/register changes, arbitrary self-modifying code, conditional-branch relocations and module address changes are not supported by this extension.

The existing conservative policy remains: live sites disable inferred constant memory hints and GIF DMA coalescing for the generated program. Performance is not validated.

## Reconstruction evidence

The original supplied disc and continuation archive remain outside the repository. No old snapshots were required. Original ELF SHA256: `d16ad76ba8cdf87695d94b1316d51c1f248959ec0749814c31bb7a2d55d9a2e1`.

A diagnostic fixture initially used symlinks, which the virtual ISO builder deliberately skips. Consequently FILEIO returned ENOENT. Tracing proved that its SIF callback and iSignalSema completion already worked; the earlier async-failure hypothesis was rejected. Replacing fixture symlinks with hardlinks allowed the original getstat path to succeed.

A fresh bounded Linux run then linked KERNEL.XFF and captured its first entry at `0x001B32A0`. Celosia tools at `df6b12b69e8c8c1d0012f5ac5b834ddb818bb2c2` reconstructed a combined ELF from the original boot executable and captured module sections. The next runner executed KERNEL's version check, loaded MANAGER.XFF and captured its first target at `0x01082ED0`.

The combined boot/KERNEL/MANAGER map has 5,052 functions. MANAGER's 6,941 import relocation sites include 4,091 JAL, 183 J and two SW instructions, which exposed the previous generator limitation. After the extension, generation completed with 4,864 guest functions, 188 stubs, no decode failures or errors, and 510 control-flow warnings. Those warnings remain correctness risks.

## Verification

- 18 Python investigation/preparation tests passed.
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:ae3c7c9725faf2c2477ee3a668aec6e8b990d0e9f15a3a7c4bd561b0be489ae2 -->

- 93 isolated CodeGenerator and PS2Recompiler cases passed, including config validation/roundtrip, default stub behavior, forced guest bodies and relocation callsites, signed live stores, live jump targets and delay-slot/RA ordering.

<!-- /coderabbit-ai-generated:v1 -->
- Exact newly generated synthetic J/JAL/SW functions compiled and executed against the production runtime and macros. Assertions verified changed targets, call returns, J preserving RA, delay-slot self-patching after target capture, and SW displacements 0, 0x7fff, 0x8000 and 0xffff.
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:1ad53deca36635d88d96aefbe70fd5269b87974d383ec17356d036200b0d5fe3 -->

- Linux Debug runtime and combined KERNEL, MANAGER and GAMECORE runners compiled. The combined GAMECORE build processed 9,024 functions (8,836 recompiled, 188 stubs), with 26,624 import relocation sites, no skipped functions or decode failures, 670 control-flow warnings and no generation errors.
- A fresh 120-second GAMECORE probe reached sheet and graphics initialization without another missing-function capture. Timed RAM/thread snapshots at 4, 30 and 90 seconds after GAMECORE entry show the main guest thread waiting in the counted graphics synchronization loop. Its saved counter advanced from 0 to 1 between the later captures. This is forward progress, not proof of a deadlock or a successful game boot. The process was stopped by timeout (124).

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:1e4fd0d1197b9d4653754f80393ec2444165eae92e28cef12f5520d94202d5e2 -->

## Longer startup with aggressive logs disabled

Using the existing `PS2X_ENABLE_AGRESSIVE_LOGS=OFF` option, a fresh 360-second diagnostic run completed all 14 counted graphics synchronization iterations and invoked `initgc`, `initgcmain`, `initgcmaingp` and `initgcchartest`. Further tracing observed `createScriptObj`, the main script callback, language menu resource load/unload, `commonDataLoad`, the boot memory-card check, stage manager preparation, camera setup, `initGameObject`, `nowLoadingScript`, `controllerAlertScript` and `arrowWaterMain`. This advances beyond the earlier initializer-entry-only evidence. Function entries do not prove that every downstream operation succeeded.

A 90-second capture showed a resource worker in `sndBDOpen`; by 180 seconds the sound DMA producer and consumer both reached 24 with completed slot statuses, and the resource worker returned to its idle wait. That sound operation was slow, not an established deadlock. The later stage/script wait remains under investigation.

Screenshots from the actual raylib framebuffer at 60, 180 and 300 seconds show a small dotted loading indicator over a black background. The first two were inspected directly. These are sparse rendered pixels, not a menu, a scene or gameplay. The run timed out with exit 124, without a new missing-target capture. An audio playback device was unavailable.

Comparing every executable-section word in the reconstructed ELF with the 300-second RAM capture found 2,701 changed instructions: 434 in MANAGER and 2,267 in GAMECORE. Every changed site was covered by the live relocation configuration. No unconfigured instruction mutation was observed in boot, KERNEL, MANAGER or GAMECORE at that checkpoint. This does not validate all data relocations, register values or future code changes.

The final forced-guest relocation-call fix regenerated all 9,078 C++/header files byte-identically to the previously compiled GAMECORE output. The reconstructed ELF has no static ELF relocation entries; the new guard is covered by the separate regression test. This equivalence permits retaining the native build result after that compiler change.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:cb5b008805f795278e0243dc4125e1609b72822dc4175e9a87ba1a7dc36de201 -->

## Optimized build and GS interrupt delivery

The existing x86/non-MSVC SSE4.1 option was applied only to the runner. GCC15 optimization exposed a target mismatch when compiling runtime intrinsics. Moving that option to `ps2_runtime PUBLIC` makes the runtime and its public-header consumers use the already documented host requirement. The complete combined GAMECORE runner compiled with `-Og -g1`, aggressive logs disabled and FFmpeg/debug UI disabled. This is a Linux diagnostic build, not a console package.

The broad C++ suite initially stopped at two DMA fixtures that used the scheduler before `reset` established its executor. The fixtures now initialize that executor on the test thread. Production executor assertions and test expectations were preserved. All455 existing cases then passed.

Later captures showed the counter read by `st_WaitGPFinish` at `0x01465648` remaining zero. The game registered `0x01345FB0` as its GP-finish callback in `0x001E116C`; this callback increments that counter. Before this repair, the GS frontend set CSR.SIGNAL/FINISH but the scheduler had no path to deliver these status events to INTC_GS. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:c9f4b977bdff1c59ff4a2727def3c8db87ac3650d8bae07fbbc333d1100e9628 -->

The repair routes the implemented CSR.SIGNAL/FINISH bits0/1 through IMR mask bits8/9 and INTC cause0 to the existing executor invocation queue. Other GS event bits remain outside this repair because their CSR acknowledgement is not implemented.

<!-- /coderabbit-ai-generated:v1 --> Only one handler chain is queued at a time. Guest CSR acknowledgement releases the event; a remaining event is eligible after the chain completes. The public `dispatchIrq` interface and the fixed20-word `GSRegisters` layout are preserved.

The event-mask relationship was cross-checked against the [PCSX2 GS implementation](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS.cpp), retrieved2026-11-01. This is a repair of missing delivery in the existing IRQ model; it does not add full hardware INTC_STAT emulation or SIGNAL FIFO backpressure.

The updated production runtime compiled and all456 C++ cases passed, including a new end-to-end scheduler test using actual GS FINISH and SIGNAL register writes. It covers masked pending events,32/64-bit IMR writes, INTC masking, a handler yielding across dispatches, acknowledgement and repeated events without duplicate queued callbacks. These456 cases include the earlier focused suites; their counts must not be added together.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:78a518195c8a1f0003afd5e66092ce739641af24a4e3057f0993dfd74b2da7fa -->

The final supported-event diagnostic probe confirms native delivery: `0x01345FB0` was called from `0x001C382C`. Its guest GP-finish counter advanced from0 to37 at30 seconds,209 at90 seconds and472 at180 seconds after GAMECORE entry. The previous optimized run held this counter at0 through300 seconds. Further guest entries reached `stagemanager_loadstagecore`, `initstage`, `initlayout`, `createPauseMenuMap` and `bootmccheck`. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:9ad2ca588e03669d89eb1bc0544df4d75e7369f1e0992929dd0028c0c4d99232 -->

This demonstrates that the original graphics-finish wait was released through guest code.

<!-- /coderabbit-ai-generated:v1 -->

The later boot memory-card check remains at state2 (`0x014664BC`), with completion flag `0x011E0158` still0. Local XTest presses of X and Return through the existing keyboard mapping did not establish completion. These are input-delivery probes, not proof of game acceptance or console-controller support. The60/180-second framebuffer contains incomplete dark text-like elements and bright strips on black. No readable menu or rendered world was verified. The180-second loading indicator disappeared, but this alone is not boot success. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:6c5ca2efeb7f6f3443e9cff006fc023fcbabd0b6b0d29b73d6872d4e0660e833 -->

The final240-second probe ended by timeout (124), without a new missing-function capture. Game boot remains incomplete.

<!-- /coderabbit-ai-generated:v1 -->

All of this still uses the diagnostic IOP module injection described below. No guest wait, counter or script state was forced forward.

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:2192f346b2756d1635fb276235fa633f174fe1ec8ca26a10bb528eca0d6ccbe7 -->

## Controller follow-up

Subsequent analysis found that the game uses the original Pad2/DBC path, outside the tested Pad1 HLE input interface. Both observed Pad2 sockets remain open but unsynchronized, with DBC device-status words0. See the [Pad2 investigation](2026-11-01-pad2-investigation.md) and `shadow_of_the_colossus/inspect_boot_state.py` for reproducible capture inspection. The precise missing enumeration/transport operation remains under investigation.

## Limits

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

All disc execution still uses an external diagnostic runner that manually loads original FILEIO, LIBSD and SG2IOPM1 modules. This does not establish normal IOP reboot. Captures and generated sources depend on the observed module layout. Linux tests use desktop OpenGL/raylib, Xvfb, disabled FFmpeg/debug UI, and no verified audio device. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:3d83976b37876e50d771788f08241b3181de5b2f417857768e95f0c7717e1869 -->

A later diagnostic framebuffer capture shows a sparse loading indicator on a black background, as described below. No menu, scene rendering, gameplay, UWP package or Xbox execution has been validated.

<!-- /coderabbit-ai-generated:v1 --> Current validation details and subsequent probe outcomes are kept in the workspace validation record.

<!-- coderabbit-ai-generated:v1 -->

# IOP VBlank callback implementation — 2026-11-01

## Change and contract

Previously, imports8/9 of the IOP `vblank` library returned success without storing or invoking a handler. This continuation implements that missing behavior. Imports4–7 retain their existing scheduling behavior and NTSC period/phase constants.

The API contract was checked against the [PS2SDK service implementation](https://github.com/ps2dev/ps2sdk/blob/master/iop/system/vblank/src/vblank.c), [header](https://github.com/ps2dev/ps2sdk/blob/master/iop/system/vblank/include/vblank.h) and [kernel errors](https://github.com/ps2dev/ps2sdk/blob/master/iop/kernel/include/kerr.h). Downloaded reference files and hashes are retained with the reconstruction artifacts. This is a service implementation reference, not proof of cycle-accurate hardware behavior.

The implementation stores separate start/end handlers with ascending signed priority and registration order for equal priorities. It preserves each registering module's GP and user argument, removes callbacks returning zero, rejects duplicates/missing releases, and reserves14 guest slots (the reference's16 slots minus its two built-in wait handlers). Registration/release during callback delivery returns the illegal-context error. Reset clears callbacks. The scheduler includes deadlines while idle and services due callbacks during interpreted execution when the IOP interrupt-control gate is enabled. Recursive delivery is guarded. Missed fields are coalesced and callback execution uses the existing100,000-instruction guest executor budget; timing and interrupt context are still partial models.

## Validation

- Standalone IOP build succeeded; all five CTest suites passed.
- Import tests cover phases, priority/order, GP/argument, no early delivery, duplicate/release/context/capacity errors, recursion, zero-return removal, coalescing and reset.
- The emulator test executes a synthetic MIPS module that registers a persistent end callback and observes its counter increasing across scheduler runs. Its existing lower-priority-thread VBlank wait check still passes.
- The optimized production runner and diagnostic runner rebuilt successfully.
- All456 existing C++ cases passed after linking unchanged test objects against the new production runtime/IOP archives. This was a fresh execution, not a fresh compilation of those unchanged test objects.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:4adba6c822063de712f681aea7793387eed6ca152e5752cad6c7e85cd66ceed4 -->

The bounded native command `timeout 120s xvfb-run -a /tmp/sotc-diagnostic-build/ps2EntryRunner SCUS_974.72`, run from the diagnostic disc directory, ended with timeout124. All five captures at4/30/60/90/110 seconds were decoded successfully. GP-finish counts were0/35/115/200/258; memory-card states were0/0/2/2/2 with completion0. Both Pad2 sockets stayed open but unsynchronized, with pad state0 and DBC status0. The60s screenshot still contains incomplete dark text/graphics; a readable menu and gameplay are not demonstrated. This probe does not establish frame rate or console performance.

Validation identities are in `workspace-context/sotc-vblank-validation-inputs.json` (65 source, test object, binary, configuration, reference, log and capture identities); results are in `workspace-context/sotc-vblank-results.json`. The unchanged C++ test source/header and object hashes were compared with the preceding compilation record before accepting reuse. `git diff --check` passed.

<!-- /coderabbit-ai-generated:v1 -->

## Remaining game boundary

The game's original DS1O_D imports WaitVblankStart; SIO2D and MC2_D import WaitVblankEnd. A scan of the disc's IRX import tables did not find direct RegisterVblankHandler imports. Consequently this repair is not established as the cause of the Pad2 enumeration failure described in [the Pad2 investigation](2026-11-01-pad2-investigation.md).

SIO2 hardware transfer, device enumeration and DBC notification/DMA remain to be implemented or verified. A connected status word alone is not valid controller emulation. The diagnostic boot still injects the original FILEIO/LIBSD/SG2IOPM1 modules, so it does not validate normal IOP reboot. No completed menu/gameplay, Windows UWP build or Xbox execution is claimed.

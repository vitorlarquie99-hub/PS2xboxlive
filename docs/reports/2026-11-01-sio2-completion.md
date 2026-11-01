<!-- coderabbit-ai-generated:v1 -->

# SIO2 completion repair — 2026-11-01

## Observed blocker

An instrumented75s diagnostic run showed the original SIO2 driver programming command0x00140540, writing pad request bytes01/42/00/00/00 through0x1F808260, then setting control0x1F808268 to0x3BD. No subsequent result-status read occurred. The previous hardware map only stored writes and could not complete that transaction.

The [PS2SDK SIO2MAN transfer routine](https://github.com/ps2dev/ps2sdk/blob/master/iop/sio/sio2man/src/sio2man.c) waits for an interrupt after setting control bit0; its interrupt handler acknowledges the local status and signals the semaphore. The [PCSX2 SIO2 model](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/SIO/Sio2.cpp) identifies IRQ17 and suppresses duplicate completion edges while local status is latched. Its [register definitions](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/SIO/SioTypes.h) provide the empty-bus defaults used here. These sources were inspected as behavioral references; no external implementation was copied.

## Implemented scope

- A start-control write latches SIO2 completion and IOP interrupt17.
- The IOP executor delivers the registered guest interrupt handler, honoring global interrupt control and the interrupt mask, with a recursion guard.
- Local status0x1F808280 supports write-one acknowledgement; zero or unrelated bits do not acknowledge. Byte and halfword writes preserve unrelated status bits.
- An unacknowledged local status suppresses duplicate start edges. Reset clears it.
- Without an attached device protocol, receive-FIFO reads return0xFF and command/port status default to0x1D100/0xF; result registers are read-only.

This intentionally replaces generic register storage for the listed SIO2 addresses. Existing SPU DMA handling and guest VBlank waits are preserved. Completion is immediate in the simplified hardware model; cycle-accurate SIO2 timing, DMA channels11/12, device command processing, controller input, multitap and physical memory cards remain unimplemented here. Empty-bus responses do not establish controller support.

## Verification

All five IOP suites pass, including register latch/acknowledgement/reset tests and <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:a18e95d710060bd617773e2ca7249a4e9328754d8fed1b916d88a61af772d96b -->

a synthetic MIPS module whose actual interrupt handler increments a counter, acknowledges SIO2 and rearms it once, receiving two completions without further duplicates.

<!-- /coderabbit-ai-generated:v1 --> The optimized production runner builds. All456 C++ tests pass after relinking the unchanged test objects against the fresh production runtime/IOP archives. The earlier object hashes were checked before reuse.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:77dc9a0e38b7a90420b6ade3a62852e2f0296cb0e889ee607560ee26e859f09c -->

The first fixed90s probe completed with timeout124 and showed repeated queries/result reads. A final refinement consumes the interrupt edge before invoking the guest handler, preserving a new edge if that handler acknowledges and starts another transfer. The synthetic MIPS test now verifies both completions and the absence of further duplicate delivery. All five IOP suites passed again after this refinement. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:cb7788676b608713f18527e6fd38faacfa999e16c2b2e0da86ee11de48e0cdd6 -->

The final75s probe ended by timeout124. The bounded hardware trace recorded13 start-control writes and7 command-status reads, versus one start and no result reads before the repair. These are trace counts capped by the logging budget, not total transactions or performance measurements. Three captures at4/30/60s showed GP-finish counts0/24/69. Both Pad2 sockets remained open but unsynchronized, with device status0. Memory-card state/completion remained0/0 in these captures; this run does not establish arrival at the later memory-card check. Native menu/gameplay is still unproven.

The tracing hook logs the stored register value before read handling; its FIFO log value0 is not the returned byte. The actual0xFF empty-FIFO response is verified by the production memory test.

Fresh final validation:5/5 IOP suites,456/456 C++ cases, production/diagnostic builds and `git diff --check` passed. Results and72 hashed source/object/binary/configuration/reference/log/capture identities are retained in `workspace-context/sotc-sio2-results.json` and `workspace-context/sotc-sio2-validation-inputs.json`. The previous compilation's C++ test source/header and object identities matched before reuse.

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

The native runner still uses diagnostic FILEIO/LIBSD/SG2IOPM1 injection. Normal IOP reboot, readable menu, gameplay, Windows UWP compilation and Xbox execution remain unproven. This sandbox has no Windows SDK or Xbox hardware.

<!-- coderabbit-ai-generated:v1 -->

# RPC and thread follow-up — 2026-11-01

## Purpose

Investigate the diagnostic stall around `NICO.DAT` lookup. These experiments use the existing Linux snapshot build and manually started FILEIO, LIBSD and SG2IOPM1 modules. They do not validate normal startup, gameplay, or Xbox compatibility.

## Method and observations

Additional logging was added only to the scratch runtime, around SIF RPC requests, transport results, and client completion. For the observed calls to service `0x11223344`, function 0 returned a handled result and the client completion path ran without a callback. This narrows the investigation: those particular calls did not remain pending at the runtime transport boundary. It does not prove that all game requests or results are correct.

A separate run sampled EE scheduler checkpoints and a published thread snapshot. Repeated checkpoint samples appeared at `0x00105a60` and `0x00105ad8`, inside the heuristic boot function `sub_001059D8`. In the sampled state, thread 6 was running with priority 45; the loader thread was sleeping, and threads 2 and 7 were waiting on semaphores 3 and 22. Those semaphores had zero count and one waiter each. Other threads were also sleeping. The snapshot and PC samples were taken at different instants, so they do not establish a single exact execution state.

The captured return addresses map to `loaderLoop`, `isysGroupForFirst`, and `iosSendMsg` among other functions. The next investigation should trace the wakeup or signal producers for the sleeping loader and the two semaphore waits, and verify interrupt delivery. Existing unimplemented EE syscall warnings remain relevant, but no causal connection has been established.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:af3ab6e03a491d845a4f2814ec41502881a47d5c235330a48de1538a7d750485 -->

## Wakeup tracing follow-up

Scratch-only logging showed successful wakeup chains between threads 11, 8, 4, 10 and 9. Thread 11 subsequently slept with return address `0x0115e5a8`, inside the heuristic manager function `manager_sub_0115E4C0`, after calling `iosSleepThread`. A RAM capture was taken at that sleep. The inspected debug-text region contained no printable message at that instant.

These observations do not establish that the waits are erroneous. Interrupt delivery, message producers and periodic `Sg2RemoteTickProc` work still require investigation. The runs timed out without gameplay; no scheduler workaround was added to production code.

## Validation and limits

<!-- /coderabbit-ai-generated:v1 -->

The scratch runtime rebuilt successfully. The RPC probe and scheduler sampling runs were bounded diagnostic executions; they did not reach a playable state. Production runtime behavior was not modified by this follow-up. Previous unit-test results still apply to unchanged repository code. The broad Debug test suite remains incomplete because of the previously reproduced baseline assertion.

See the [initial investigation](2026-11-01-initial-investigation.md) for input identity, earlier boot milestones, and the limitations of fixed-address snapshots.

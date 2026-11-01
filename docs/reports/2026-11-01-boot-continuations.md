<!-- coderabbit-ai-generated:v1 -->

# Boot tracing after graphics setup

## Objective

Following the [shared IOP memory repair](2026-11-01-shared-iop-memory.md), determine whether the remaining boot behavior reflects slow execution, a module exit, additional instruction relocation, or a guest wait. This investigation changes only the external diagnostic runtime.

## Experiments and findings

An extended Linux Debug run was bounded at 90 seconds and timed out (exit 124). Checkpoint/thread tracing produced 200 checkpoint observations, ending at EE cycle 280,773,140 before reaching the logging cap. This is the last observed cycle, not the final simulated duration or a reliable throughput measurement. The log contains 93 GsPutIMR calls; they establish continued activity, not rendered frames or gameplay.

RAM was captured at five million and ten million checkpoint calls and preserved outside the repository. Comparing listed dynamic relocation sites against the first new capture found exactly the same 691 changed instruction sites, all already configured for live reads. No additional unconfigured changed sites were found in that comparison. This does not cover unlisted self-modifying instructions or other layouts.

Thread snapshots again show the service threads sleeping or waiting on semaphores while thread 5 continues running and servicing activity. The snapshots alone do not establish a deadlock.

A separate 15-second run traced module returns and explicit thread exits. Thread 3 exited before GAMECORE was entered. GAMECORE's dispatch returned with PC `0x0115eb60`, inside the initialization loop of `sheetMgrInit`, and RA `0x0115eb44`. The generated loop explicitly returns at a scheduler checkpoint. Therefore this return is evidence of a yielded continuation, not a completed GAMECORE module or a proven abnormal thread exit. MANAGER's earlier return reached its expected caller continuation.

## Validation

Both instrumented diagnostic runners rebuilt successfully. The 90-second and 15-second probes timed out; neither produced confirmed gameplay. A relocation comparison script passed and captures were copied with SHA-256 verification. The previously validated production allocator, IOP suites and generator code were unchanged in this investigation, so their earlier results are reused. The broader Debug test assertion remains outstanding.

Captures contain game data and remain under workspace inputs. The scratch runtime still uses manually injected original IOP modules, snapshot-derived EE addresses and the 691-site immediate configuration. These observations do not validate normal IOP startup, audio, rendering, or Xbox execution.

## Next boot work

Trace continuation through `sheetMgrInit` and its callers, then mark entry into `initgcmain`, `initgcmaingp` and `initgcchartest`. The GAMECORE entry calls those initializers after its earlier initialization call. Distinguish scheduler yields from completion, and measure cycles beyond the trace cap before attributing delay to execution speed. No production scheduler change is justified by these observations yet. There is no playable or Xbox release, and the settings overlay remains deferred.

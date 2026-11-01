<!-- coderabbit-ai-generated:v1 -->

# GAMECORE initialization milestones

## What was traced

Following the [boot continuation investigation](2026-11-01-boot-continuations.md), scratch-only tracing followed scheduler dispatch and return addresses after the checkpoint at `sheetMgrInit`. Additional markers covered the GAMECORE entry's calls to `initgc`, `initgcmain`, `initgcmaingp`, and `initgcchartest`.

The first broad trace was dominated by the new worker thread and interrupt callbacks. A focused trace of thread 6 showed the actual continuation: `sheetMgrInit` resumed at `0x0115ebd0`, returned into manager initialization, and execution subsequently reached `graphInit`. The earlier sheet initialization checkpoint was not the remaining blocker.

## Graphics synchronization and progress

Thread 6 then repeatedly resumed through `iosSyncV` and a manager routine at `0x010912c8`. Inspection of that routine found a counted loop with 14 iterations. Runtime counter observations at `0x01091310` showed s0 progressing from 0 through 12 in a 45-second Debug probe, which timed out before completion.

A longer probe with the same executable observed all counter values 0 through 13, then the following GAMECORE entry callsites:

| Caller | Target | Initializer |
| --- | --- | --- |
| `0x01344790` | `0x01345b70` | `initgc` |
| `0x01344798` | `0x01346458` | `initgcmain` |
| `0x013447a0` | `0x01346100` | `initgcmaingp` |
| `0x013447a8` | `0x013453d0` | `initgcchartest` |

These markers prove that execution passed the earlier sheet and graphics initialization waits and invoked all three later initializers. No scheduler behavior change was needed to get through this path. They do not establish that the initializers completed every downstream task or that gameplay works.

## Verification and limitations

Three instrumentation builds passed. The first 20-second and focused 15-second probes timed out. The 45-second counter probe timed out after reaching s0=12. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:0e9ee2ac9ef226b989bf91cb785ea92dee4b7550b477745a13f1142a642a29ee -->

The 90-second probe timed out (exit 124) after invoking all four listed initializers. It did not produce confirmed gameplay or a new missing-function report. Later snapshots no longer listed thread 6; the trace does not by itself establish the reason for that lifecycle change.

<!-- /coderabbit-ai-generated:v1 -->

A framebuffer observation during the longer probe captured a black X11 root with a cursor, without a visible game image. It does not demonstrate rendering. No browser was used; this was a native Linux diagnostic process running under Xvfb. Production code remains unchanged by this tracing work, so previous unit results are reused.

The diagnostic still uses manual FILEIO/LIBSD/SG2IOPM1 initialization, snapshot-derived EE addresses and the 691-site immediate configuration. Normal IOP startup, rendering/audio, gameplay, Xbox packaging and console validation remain outstanding. The broader Debug test suite retains its previously reproduced scheduler assertion. The settings overlay remains deferred.

## Next investigation boundary

Follow work scheduled by the GAMECORE initializers into script execution and the first scene/resource loads. Separate entry into initialization routines from successful execution of those later jobs. Preserve the observed continuations and synchronization behavior; the counted graphics loop progressing slowly in Debug is not independent evidence of a deadlock.

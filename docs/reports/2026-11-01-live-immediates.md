<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:48885ab79632e6c035fcd4c63fdc246bc843ed68b5048b1e88026c912158970f -->

# Live instruction immediate support

This report records the earlier implementation. The [boot reconstruction follow-up](2026-11-01-boot-reconstruction.md) extends live relocations to J/JAL and SW and records fresh captures from the supplied disc.

<!-- /coderabbit-ai-generated:v1 -->

## Why this change was needed

The [sound relocation investigation](2026-11-01-sound-relocation.md) identified a concrete mismatch between generated C++ and guest code after XFF linking. The captured LUI/ADDIU pair at `0x0115863c` and `0x01158644` described address `0x001b7b60`. The loader later patched those instruction immediates to describe `0x01565cd0`. Generated code retained the captured values and read a kernel instruction as an allocation count.

A manual diagnostic edit established that reading the patched instructions removed that invalid sound allocation. This change moves that limited repair into the repository recompiler, so regenerating C++ no longer requires manually changing the sound routine.

## Configuration

Add this optional array inside the existing `[general]` table of a recompiler TOML file:

```toml
live_immediate_addresses = [0x0115863c, 0x01158644]
```

These are instruction addresses, not the addresses of the data being accessed. They apply to the investigated combined snapshot and are not guaranteed for another module layout. The default is an empty array. Configuration saving preserves the array. Parsing rejects negative, unaligned, out-of-range, and noninteger entries.

For listed LUI instructions, generated C++ reads the instruction word from guest RAM, extracts its low 16 bits, shifts them left 16 bits, and writes the sign-extended 32-bit result. For listed ADDIU instructions, it reads the current low immediate, sign-extends 16 bits, and adds with 32-bit wraparound. Opcode and register fields still come from the input ELF. The loader must populate guest instruction memory before the generated function runs.

Only LUI and ADDIU are supported. A configured instruction with another opcode is rejected when its function is generated. The feature does not execute arbitrary self-modifying instructions, relocate host function addresses, or handle relocated J/JAL targets.

## Avoiding stale inferred addresses

Changing the immediate alone is insufficient when the generator has already inferred a constant address for a later load or store. Whenever this option is nonempty, instruction translation discards inferred memory-address hints for the entire generated program. GIF DMA store coalescing is also disabled because it uses the same captured constants. Ordinary runtime memory operations then use guest registers.

This is deliberately conservative and may reduce performance. With the option empty, the previous inference and DMA optimization behavior is preserved. Future work can track which values depend on live instructions and retain independent optimizations.

## Verification

- Rebuilt the recompiler and test target using the existing Linux Debug environment.
- Ran all 64 CodeGenerator test cases independently of the broader suite's previously reproduced scheduler assertion: all passed. Cases cover configuration roundtrip and rejection, default behavior, live reads, signed ADDIU emission, stale memory hints, unsupported instructions, and DMA optimization handling.
- Regenerated the complete diagnostic snapshot into a separate directory: 9,019 functions processed, 8,832 recompiled, 187 stubs, 666 control-flow warnings, no decode failures or reported errors.
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:a1d160d7772377be4652176642a4b57120357dd89ec8ccfa82bd43e902a856ec -->

- Inspected the generated sound function: both immediate sites read live guest RAM; the count and pointer loads use the resulting guest register instead of the old constant address.
- Compiled the exact emitted immediate and count-load statements against production runtime macros, then executed them against the preserved RAM capture: pointer `0x01565cd0`, count `11`. Additional assertions covered low immediates `0`, `0x7fff`, `0x8000`, and `0xffff` and 64-bit sign extension of the 32-bit result.

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:96a55982d0c0330b9712f493994509be74b14f10acd4ee4d730d0ed7728d4f4b -->

The complete Linux diagnostic runner rebuilt successfully against this fresh generated output. A 30-second Xvfb boot probe entered `GAMECORE.XFF`, found `NICO.DAT`, and logged the expected 88-byte sound allocation. The earlier invalid allocation did not reappear. The probe timed out (exit 124); playable gameplay was not reached. The scratch runner still uses manually injected original IOP modules and fixed snapshot addresses, so this does not validate normal boot or Xbox execution.

A separate comparison of 26,624 listed relocation sites between the GAMECORE-entry and worker-sleep captures found 691 changed instructions: 334 LUI, 217 ADDIU, and 140 LW. Their upper 16 bits were unchanged. The new option is enabled for the two confirmed sound sites only. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:f2df9b9a0599660fc3bde06cffa4308fb1fcfe13a96985a14b37a7b687b8ab4e -->

Live LW displacements were not supported in this experiment; the [subsequent LW implementation and wider test](2026-11-01-live-loads.md) records that follow-up. The comparison alone does not establish the cause of the remaining boot failure.

<!-- /coderabbit-ai-generated:v1 -->

Game files, captured RAM, generated game sources, and diagnostic executables remain outside the repository.

<!-- /coderabbit-ai-generated:v1 -->

## Remaining work

This is the first supported pair of live instruction immediates. General XFF relocation handling, jump targets, normal IOP reboot, graphics/audio, and reliable game boot still require work. There is no validated Xbox package or playable release. The [deferred settings overlay](../settings-overlay-backlog.md) remains outside this boot repair.

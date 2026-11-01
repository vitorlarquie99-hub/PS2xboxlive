<!-- coderabbit-ai-generated:v1 -->

# Sound configuration relocation defect — 2026-11-01

## Confirmed defect

Guest diagnostic logging revealed `_iosMallocAlign` receiving size `0xFDF80000` from `initSoundManager`. The generated function used a captured address of `0x001B7B60`, inside kernel code, as the sound configuration. Its count field therefore read an instruction word rather than configuration data.

Comparing the pre-entry GAMECORE capture with the later worker-sleep capture showed that the loader rewrote MANAGER instructions at `0x0115863C` and `0x01158644`. The later instruction pair addresses `0x01565CD0`. Its count field is 11, which implies an 88-byte allocation. This is direct evidence that compiling those captured instruction immediates as constants is incorrect for this execution.

## Diagnostic repair and result

Only the external generated diagnostic function was changed: read the two instruction immediates from guest RAM at execution time, retain signed ADDIU semantics, and replace the two constant memory reads with reads through the resulting register address. The original generated function was preserved outside the repository.

The Linux diagnostic runner rebuilt successfully. The new run logged an 88-byte allocation at the same sound initialization call. The earlier invalid allocation message did not reappear in the bounded test. The game still did not reach playable gameplay; this repair addresses one confirmed relocation error only.

## Remaining implementation

The repository recompiler still requires general dynamic relocation support, including memory-address inference and relocated jump targets. The diagnostic function edit is outside the repository, depends on this exact ELF and observed instruction locations, and is not an Xbox release or a complete game fix. Normal IOP startup and reliable boot remain outstanding.

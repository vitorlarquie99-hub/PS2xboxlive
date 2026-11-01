<!-- coderabbit-ai-generated:v1 -->

# Pad2 startup investigation — 2026-11-01

## Observed boundary

The game uses the original `scePad2` guest library. Existing `scePad` HLE tests and desktop keyboard event injection do not validate this path. No `ps2_stubs::scePadRead` call exists in the reconstructed generated output.

`padSysTickProc` at0x00104CC0 checks `scePad2GetState(socket)` for state1 before reading input. Its two port records start at0x0013D840 with stride0x140; state is at+0x118 and socket at+0x11C. The Pad2 socket table starts at0x0014B598 with stride0x334; fields+0/+4 indicate open/synchronized state in the guest library.

The previous180s final GS capture has sockets0/1 open, both synchronization fields0, both pad states0. `scePad2GetState` calls0x00115650 to synchronize; that helper calls `sceDbcGetDepNumber` at0x00116030. Before issuing RPC0x80001303, this function copies the DBC device-status table at0x00150D00 and requires the socket's word to equal1. Otherwise it returns-12. Both observed status words are0. This explains a concrete gate before input polling; it does not prove the complete cause of the memory-card dialog wait.

The original SIO2MAN, DBCMAN, SIO2D and DS1O_D modules load in the diagnostic run. DBC RPC service binding succeeds. A loaded service is insufficient to establish a connected device or valid input DMA. Current IOP memory hardware handling contains general register storage and SPU DMA completion, but no SIO2 controller transfer implementation. Device enumeration, DBC notification/DMA and SIO2 behavior require further tracing before choosing a compatible repair. Returning connected status alone would not establish valid input packets.

## Reproducible inspection

Use the original boot ELF with SHA256d16ad76ba8cdf87695d94b1316d51c1f248959ec0749814c31bb7a2d55d9a2e1 and a32MiB EE RAM dump from the documented combined-module layout:

```sh
python3 shadow_of_the_colossus/inspect_boot_state.py /path/SCUS_974.72 /path/ram.bin
```

The tool rejects another ELF or RAM size. It reports raw pad/DBC values, GP-finish count and memory-card state. The ELF hash does not validate the loaded module layout; callers must use captures matching the reconstruction report. Only observed socket indices0/1 are decoded, with null fields for other indices.

Validation against the existing180s capture confirmed GP count472, memory-card state2, completion0 and the two unsynchronized sockets. CLI checks also verified wrong ELF rejection, short RAM rejection and safe handling of an invalid socket index. The runtime/recompiler were unchanged in this continuation; their previous456-case C++ result remains applicable and does not establish Pad2 compatibility.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:f9e911d122008abe38495fb6f19ab237ec3da123789954b16524ee876fe4e273 -->

A fresh120s probe ended by timeout (124). All five captures at4/30/60/90/110 seconds reproduce both pad states0, socket synchronization0 and DBC status0. At90/110 seconds the memory-card state is2 with completion0. GP-finish counts continue advancing. The results reproduce the enumeration gate without establishing the cause of missing device notification.

<!-- /coderabbit-ai-generated:v1 --> <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:a408395870c514c4a5346bf28cea2bf7828651fa00c4a966fad0accd55317976 -->

No guest state or controller packet was overridden. No Xbox or completed game boot is claimed.

The next continuation implemented missing IOP VBlank callback delivery and reran the native probe. The Pad2 gate persists; see [the implementation and validation report](2026-11-01-iop-vblank.md).

<!-- /coderabbit-ai-generated:v1 -->

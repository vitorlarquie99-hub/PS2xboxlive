<!-- coderabbit-ai-generated:v1 -->

# Shared IOP module and heap memory

## Contract change and purpose

The original implementation divided IOP RAM into a module arena below `0x00120000` and an 832 KiB heap above it. The [heap investigation](2026-11-01-iop-heap.md) showed that three sound buffers alone need 1,056 KiB. Lowering a fixed boundary in a scratch experiment advanced boot but reduced the available module arena.

The repository now shares the existing range `0x00010000` through `0x001f0000` between modules and allocations. Physical IOP RAM remains 2 MiB; the low and upper reserved memory outside that range remain excluded. Allocation addresses intentionally change. Modules retain their existing placement preference at low addresses, while ordinary heap allocations search free gaps from high addresses downwards. Fixed-address requests still require free space. A later module that collides with a live allocation is rejected instead of overwriting it.

## Implementation

- All loaded module images receive explicit reservations in the same allocation map used by the heap. The image span is reserved before segment writes; segment/section writes are checked against that span. Range arithmetic rejects overflowing image extents.
- Failed image loads release their reservation and retain the incoming module placement cursor. Successful fixed images advance the cursor so subsequent relocatable images do not overwrite them.
- Guest heap frees cannot release module reservations. Stopping a module releases its reservation after terminating its owned execution and removing imports/RPC servers. Reset clears all reservations.
- Heap allocation searches current gaps, allowing freed blocks to be reused. Bounds, rounding overflow and invalid alignment requests are rejected.
- Maximum contiguous free space and total free space are computed independently. The corresponding sysmem queries now return the appropriate value.

This removes the fixed module/heap split. It does not guarantee that arbitrary module combinations fit in 2 MiB, compact live allocations, or relocate an already loaded module around a collision.

## Verification

All five standalone IOP CTest suites passed. New memory tests cover three simultaneous 352 KiB buffers, reuse of a freed middle block, fixed-address and module collisions, invalid sizes/alignment, reset, failed image rollback, protection of module reservations against guest frees, and reservations above the old module ceiling.

An initial compile failed because a new allocation metadata field was missing; that was corrected. The first complete suite run found a module lifecycle regression: reservations were not released on stop, and fixed-address modules did not advance the placement cursor. Both were corrected; all five suites passed again after final range/cursor safeguards. The existing broader Debug suite's independently reproduced scheduler assertion remains outstanding.

The Linux diagnostic runner rebuilt with the repository allocator, module loader and sysmem implementation, replacing the earlier scratch partition. A 20-second boot probe observed three successful 352 KiB allocations at `0x0018aa00`, `0x00132a00`, and `0x000daa00`, then reached GsGetIMR/GsPutIMR calls. It timed out (exit 124), with no confirmed gameplay or new missing-function report.

## Remaining limitations

The runner still uses scratch-only manual FILEIO/LIBSD/SG2IOPM1 startup, snapshot-derived EE function addresses and the 691-site relocation configuration. This validates the memory repair in that diagnostic environment, not normal IOP reboot, correct rendering, audio output, playable boot or Xbox compatibility. There is no Xbox release. The settings overlay remains deferred while boot work continues.

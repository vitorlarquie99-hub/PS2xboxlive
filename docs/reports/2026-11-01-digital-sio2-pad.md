<!-- coderabbit-ai-generated:v1 -->

# Native digital controller over SIO2 — 2026-11-01

## Implemented path

The previous SIO2 repair completed transactions but returned an empty-bus response. This continuation adds bounded PIO command queues and an actual digital controller poll response. Descriptor transmit/receive lengths and port bits select the packet; a01/42 poll returns FF/41/5A followed by the host's active-low button bytes. Other devices and unsupported commands retain an absent-device response. The16-descriptor queue and9-bit lengths bound buffering. Writing the first descriptor resets transaction buffers; FIFO reads consume data and return FF when empty. Reset discards partial input and output while retaining the host input reader.

The runtime's existing pad backend supplies keyboard/gamepad buttons for port0. Port1 remains disconnected. A new optional IopHost method defaults to disconnected for other hosts; callers must rebuild against the changed C++ interface. No guest pad/DBC state is forced and no button presses are synthesized in the native probe. This changes port0 from absent to a digital device backed by real host input. Analog axes, DualShock configuration commands, pressure, vibration, multitap and SIO2 DMA remain outside this implementation. Digital support alone may be insufficient for this game's original driver.

The behavioral references are the [PCSX2 SIO2 descriptor/FIFO model](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/SIO/Sio2.cpp), [controller protocol implementation](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/SIO/Pad/PadDualshock2.cpp) and existing runtime PS2 button mapping. No external implementation was copied.

## Validation

All five standalone IOP suites passed. New memory checks validate identification, marker, non-neutral button byte order, port isolation, unsupported commands, FIFO exhaustion, reset and8/16/32-bit writes. The optimized production runner built successfully. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:647af308f8d8e9f424fa58bfe6e64f2b14ca64c1265cf982cfdf2691852f3b4b -->

The full C++ suite was freshly compiled because IopHost's virtual interface changed; the scratch diagnostic objects were also rebuilt after clearing their source-only caches to account for changed headers.

<!-- /coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:11b9036a571f0540e716333d3af03502645863072bf9128dd1b362edae46bf58 -->

Fresh compilation of the complete C++ suite passed456/456. A FIFO lane refinement then added tests proving wider reads consume a single byte and unused lanes do not consume data; all5 IOP suites passed again. The production runner rebuilt and the full C++ suite relinked against final archives passed456/456 again. Scratch tracing was rebuilt against final sources. `git diff --check` passed.

The first120s probe ended124 with captures at4/30/60/90/110s: GP counters0/37/118/203/261 and memory-card states0/0/2/2/2, completion0 throughout. The final75s probe ended124: counters0/35/111 at4/30/60s, memory-card states0/0/2. In both probes, Pad2 state, synchronization and DBC status remained0.

The final trace confirms two original port0 polls0x42 reaching the host reader and producing connected command status0x1100. The next observed command is0x43 (DualShock configuration), repeated with an unsupported/absent response. This is the next concrete protocol boundary. It does not prove that implementing0x43 alone will complete boot: later configuration, DMA and DBC notification may also need work. Port1 polls remain absent as intended. No guest state or input was overridden.

Results: `workspace-context/sotc-digital-results.json`; input identities: `workspace-context/sotc-digital-validation-inputs.json`. Diagnostic sources/logs are retained in inputs/sotc/reconstruction/current. No readable menu or gameplay is demonstrated; this is progress in startup compatibility, not a completed boot.

<!-- /coderabbit-ai-generated:v1 -->

Normal IOP reboot is still unproven: the native diagnostic injects original FILEIO/LIBSD/SG2IOPM1. A readable menu and gameplay remain unproven. Windows UWP and Xbox validation are blocked by unavailable SDK/hardware in this Linux sandbox.

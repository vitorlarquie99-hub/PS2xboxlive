<!-- coderabbit-ai-generated:v1 -->

# DualShock configuration sequence — 2026-11-01

## Implemented behavior

The previous implementation stopped at model query0x45. This continuation extends the virtual controller from digital/configuration entry to logical DualShock2 negotiation:

| Commands | Behavior |
| --- | --- |
|40,41,45|Protocol constants, button mask and model/current-mode query|
|46,47,4C|Capability constants with selector-dependent responses|
|44|Digital/analog mode selection|
|4D|Return previous motor mapping and store the new mapping|
|4F|Select digital, analog or pressure packet format from the response mask|
|42,43|Poll the selected format; enter/exit configuration using the prior packet identifier|

Configuration-only queries require configuration mode. Packet identifiers use the mode before any update. Reset restores digital mode and unmapped motors. Unknown commands and absent ports still fail. Existing transmit/receive lengths, FIFO consumption and bounded16-descriptor behavior remain in place.

The runtime supplies RX/RY/LX/LY from its existing input backend through an optional host axes reader; other hosts default to centered axes. This virtual-interface extension requires rebuilding callers. Buttons remain active-low. Twelve pressure channels use0/255 derived from their corresponding buttons, providing quantized pressure rather than physical pressure sensitivity. Motor mapping is emulated logically; physical vibration output is not implemented. Analog-mode hardware toggle buttons and cycle-accurate timing are not claimed. These limits apply despite the protocol model identifier.

Behavioral packet references: [controller implementation](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/SIO/Pad/PadDualshock2.cpp) and [protocol types](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/SIO/Pad/PadTypes.h), retained locally from the prior investigation. No external implementation was copied.

## Checks

All5 IOP suites passed, including exact model/selector response vectors, motor-map round trips, mode transitions, non-neutral axis order and pressure mapping. The production runner built. Because the host virtual interface changed, all C++ test sources were freshly compiled;456/456 passed. Diagnostic source-only caches were cleared before rebuilding scratch objects against the new headers. `git diff --check` passed.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:663c0f1865c07438f38563f166c17e65cf803e59fd8dad77255fd7657df4a1a5 -->

The120s probe ended by timeout124. The original driver accepted45/46/47/4C/44/4D, exited configuration through43 and entered repeated9-byte42 polls. All five captures at4/30/60/90/110 seconds show port0 Pad2 state1, socket sync1 and DBC device status1; port1 remains absent. This removes the observed configuration-sequence blockers and the earlier device-enumeration gate without overwriting EE state.

GP-finish counters were0/32/104/189/245. Memory-card states were0/0/2/2/2 with completion0. XTest x/Return events were delivered/released in the native window both early and after the90s capture; their scripts returned0. The captured pad record remained unchanged, so guest button acceptance and memory-card completion are not established. Pressure-format negotiation was covered by packet tests; the observed native path selected analog9-byte polling.

PASS0: `python3 /tmp/sotc-sequence-results.py` verified the native sequence, regular analog polling and synchronized port0 in all5 RAM dumps. Results and input/source/test object/binary/configuration/reference/log/capture identities are retained in `workspace-context/sotc-sequence-results.json` and `workspace-context/sotc-sequence-validation-inputs.json`. Diagnostic sources and logs are retained under inputs/sotc/reconstruction/current. No connected status or guest readiness value was forced. The remaining startup boundary includes the memory-card wait, input delivery into the game and rendering; completed gameplay is not claimed.

<!-- /coderabbit-ai-generated:v1 -->

Normal IOP reboot remains unproven because the diagnostic injects original FILEIO/LIBSD/SG2IOPM1. The60s screenshot was visually inspected: loading indicator on a black background, no readable menu or gameplay. Windows UWP/Xbox validation remains blocked by the unavailable SDK/hardware in this Linux environment.

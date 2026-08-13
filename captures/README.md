# UART Captures

This directory is intended for raw, hardware-verified Walksnail VRX UART captures.

Please keep captures as close to the original byte stream as possible.

## Suggested metadata

For every capture, include:

- VRX model
- VRX firmware version
- test date
- UART configuration
- test condition
- raw TX bytes
- raw RX bytes
- optional notes

Example:

```text
VRX model:
Firmware:
UART: 115200 8N1

Condition:
VTX powered, stable video link, short range

TX:
FE EF A2 00 08 52 00 00 00 00 00 00 00 00 52 0D 0A

RX:
FE EF ...
```

## Useful test cases

1. VRX powered, VTX off
2. VTX connected at strong signal
3. VTX connected while RF signal is gradually reduced
4. Change Band / Channel / Hop
5. Start and stop recording
6. Compare received and calculated checksum values

## Checksum validation

For every reply, calculate:

```text
sum(payload bytes) & 0xFFFF
```

and compare it to the two checksum-position bytes in the received frame.

Until this is confirmed across real hardware, preserve both values in logs instead of dropping frames with mismatches.

## Naming suggestion

```text
YYYY-MM-DD_model_firmware_test-name.txt
```

Example:

```text
2026-08-14_vrx-fw-unknown_strong-link.txt
```

Raw captures are especially valuable for confirming:

- RSSI representation
- DataRate units
- Latency units
- Connected values
- RX checksum behavior
- additional response types

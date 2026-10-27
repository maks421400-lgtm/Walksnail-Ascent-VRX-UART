# Walksnail Ascent VRX UART Protocol


Open documentation and reference implementation of the **Walksnail VRX UART control and telemetry protocol**.

The goal of this project is to make Walksnail VRX hardware easier to integrate with custom controllers, STM32 projects, ground stations, antenna trackers, test equipment, and other user-built systems.

The primary source for the control protocol is the Chinese serial-command document supplied by the repository maintainer from the developers' firmware package, reviewed together with its English counterpart, **Serial Port Command Software Function Description.docx**. The Chinese file's supplied filename is garbled; it is identified by its opening sentence: 本文档说明使用串口命令方式，实现按键的各项功能.

These documents take precedence over earlier implementation-derived assumptions where their descriptions are clear. They are two versions of the same material, not independent hardware validation. The original files are not currently included in this repository.

The documentation distinguishes source-described behavior, implementation-derived assumptions, and unresolved source inconsistencies. No raw hardware captures were present in `captures/` at the time of this review (2026-10-27).

---

## Features

Currently documented:

* UART configuration
* packet framing
* payload length encoding
* checksum algorithm
* remote-control key commands
* UP / DOWN / LEFT / RIGHT
* ENTER / BACK
* PAIR / UPGRADE / RECORD
* Force 720p60 / Unlock Debug3 mode
* channel control
* Band / Channel / Hop
* VRX status requests
* RSSI1 / RSSI2
* Data Rate
* Latency
* Link state
* STM32-compatible reference implementation

---

# UART configuration

The existing implementation uses the settings below. The supplied serial-command documents do **not** specify baud rate, parity, stop bits, electrical levels, or pinout; these settings still require confirmation for the target hardware and firmware.

```text
115200 baud
8 data bits
No parity
1 stop bit
```

In short:

```text
115200 8N1
```

GPIO/pin assignments depend on your own hardware.

Before connecting a VRX to a microcontroller, verify the electrical UART voltage levels of your particular hardware.

---

# Packet format

Packets use the following structure:

```text
FE EF CMD LEN_H LEN_L PAYLOAD... SUM_H SUM_L 0D 0A
```

Field layout:

```text
+------+------+-----+-------+-------+-------------+-------+-------+------+------+
|  FE  |  EF  | CMD | LEN_H | LEN_L | PAYLOAD ... | SUM_H | SUM_L |  0D  |  0A  |
+------+------+-----+-------+-------+-------------+-------+-------+------+------+
```

The source defines a two-byte payload length and a payload range of 0–65535 bytes. Its packet examples encode length in **big-endian** order. This field capacity is not a guarantee that a device accepts arbitrary payload sizes; the reference library limits payloads to 64 bytes.

Example for a four-byte payload:

```text
00 04
```

Total frame size is:

```text
payload_length + 9
```

---

# Command byte and direction ambiguity

The source describes command values 0x00–0x7F and assigns bit 7 to direction: 0 for host→SoC and 1 for SoC→host. However, the same source explicitly gives **0xA2 for host-issued queries** (`0x51` and `0x52`), which has bit 7 set.

Preserve the documented query packets below. Do not clear bit 7 or derive reply command values solely from the direction description until this inconsistency is resolved by hardware captures or vendor clarification.

---

# Checksum

The source defines the checksum generally as the accumulated sum of payload bytes, without specifying a separate RX algorithm. The two-byte reference implementation uses `sum(payload) & 0xFFFF`. Source TX examples agree with this rule; actual RX behavior remains to be checked against captures.

```c
uint16_t checksum = 0;

for (uint16_t i = 0; i < payloadLength; i++) {
    checksum += payload[i];
}
```

Only the payload is included.

The following bytes are not included in the checksum:

```text
FE EF
CMD
LEN_H LEN_L
0D 0A
```

The checksum is transmitted big-endian:

```text
SUM_H SUM_L
```

Example:

```text
Payload:
40 03 00 00
```

Calculation:

```text
0x40 + 0x03 + 0x00 + 0x00 = 0x0043
```

Checksum bytes:

```text
00 43
```

---

# Generic packet transmitter

```c
void sendVrxPacket(
    uint8_t command,
    const uint8_t *payload,
    uint16_t payloadLength)
{
    uartWriteByte(0xFE);
    uartWriteByte(0xEF);

    uartWriteByte(command);

    uartWriteByte((uint8_t)(payloadLength >> 8));
    uartWriteByte((uint8_t)(payloadLength & 0xFF));

    uint16_t checksum = 0;

    for (uint16_t i = 0; i < payloadLength; i++) {
        uartWriteByte(payload[i]);
        checksum += payload[i];
    }

    uartWriteByte((uint8_t)(checksum >> 8));
    uartWriteByte((uint8_t)(checksum & 0xFF));

    uartWriteByte(0x0D);
    uartWriteByte(0x0A);
}
```

---

# Remote-control commands

Remote-control key events use:

```text
CMD = 0x22
```

with a four-byte payload:

```text
40 KEY PRESS_TYPE 00
```

The source names these fields `cmd_type` (also called `key_type`), `key_num`, `press_type`, and `rsv`. The helpers retain the parameter name `argument` for `press_type`. Only the combinations below are documented; `press_type` is not established as a universal short/long-press flag. The reserved byte is zero in every supplied key example.

Generic key packet:

```text
FE EF 22 00 04 40 KEY PRESS_TYPE 00 SUM_H SUM_L 0D 0A
```

Helper:

```c
void sendVrxKey(uint8_t key, uint8_t argument)
{
    uint8_t payload[4] = {
        0x40,
        key,
        argument,
        0x00
    };

    sendVrxPacket(0x22, payload, sizeof(payload));
}
```

---

# Key codes

| Function |    KEY | PRESS_TYPE |
| -------- | -----: | -----: |
| UP       | `0x00` | `0x00` |
| DOWN     | `0x01` | `0x00` |
| LEFT     | `0x02` | `0x00` |
| RIGHT    | `0x03` | `0x00` |
| ENTER    | `0x04` | `0x00` |
| PAIR     | `0x05` | `0x00` |
| UPGRADE  | `0x05` | `0x01` |
| RECORD   | `0x06` | `0x00` |
| BACK     | `0x07` | `0x00` |
| Force 720p60 | `0x07` | `0x01` |
| Unlock Debug3 mode | `0x09` | `0x00` |

The source explicitly marks key code `0x08` as **invalid**, with no valid `press_type` specified.

UPGRADE is the documented upgrade-key action. The document does not describe firmware transfer, prerequisites, or recovery behavior.

---

# Ready-to-send HEX commands

## UP

```text
FE EF 22 00 04 40 00 00 00 00 40 0D 0A
```

## DOWN

```text
FE EF 22 00 04 40 01 00 00 00 41 0D 0A
```

## LEFT

```text
FE EF 22 00 04 40 02 00 00 00 42 0D 0A
```

## RIGHT

```text
FE EF 22 00 04 40 03 00 00 00 43 0D 0A
```

## ENTER

```text
FE EF 22 00 04 40 04 00 00 00 44 0D 0A
```

## PAIR

```text
FE EF 22 00 04 40 05 00 00 00 45 0D 0A
```

## UPGRADE

```text
FE EF 22 00 04 40 05 01 00 00 46 0D 0A
```

## RECORD

```text
FE EF 22 00 04 40 06 00 00 00 46 0D 0A
```

## BACK

```text
FE EF 22 00 04 40 07 00 00 00 47 0D 0A
```

## Force 720p60

```text
FE EF 22 00 04 40 07 01 00 00 48 0D 0A
```

## Unlock Debug3 mode

```text
FE EF 22 00 04 40 09 00 00 00 49 0D 0A
```

---

# Channel control

Channel selection uses:

```text
CMD = 0x22
```

Payload:

```text
50 BAND CHANNEL HOP 00 00 00 00
```

Example:

```c
void vrxSetChannel(
    uint8_t band,
    uint8_t channel,
    uint8_t hop)
{
    uint8_t payload[8] = {
        0x50,
        band,
        channel,
        hop,
        0x00,
        0x00,
        0x00,
        0x00
    };

    sendVrxPacket(0x22, payload, sizeof(payload));
}
```

The checksum is:

```text
0x50 + BAND + CHANNEL + HOP
```

---

# Band representation

Documented values (other band values are invalid):

```text
0 = Band A
1 = Band B
2 = Band C
```

The source lists channel indexes `0…16`; its examples establish zero-based numbering (CH1 → 0, CH13 → 12). It does not establish that every band supports every index, or reconcile this range with the implementation-derived frequency tables below.

Channels are represented internally as zero-based indexes.

Therefore:

```text
raw channel 0 = displayed channel 1
raw channel 1 = displayed channel 2
...
```

Hop:

```text
0 = Fixed frequency
1 = Frequency hopping
```

---

# Known frequency tables

The following frequency tables were identified in a compatible implementation.

They are absent from the supplied serial-command documents and should be considered implementation data rather than a universal guarantee for every VRX model or firmware version.

## Band A

```text
CH1   4915 MHz
CH2   4945 MHz
CH3   4975 MHz
CH4   5005 MHz
CH5   5035 MHz
CH6   5065 MHz
CH7   5120 MHz
CH8   5160 MHz
CH9   5200 MHz
CH10  5240 MHz
CH11  5275 MHz
CH12  5320 MHz
CH13  5360 MHz
CH14  5400 MHz
CH15  5440 MHz
CH16  5839 MHz
```

## Band B

```text
CH1   5475 MHz
CH2   5510 MHz
CH3   5545 MHz
CH4   5580 MHz
CH5   5620 MHz
CH6   5660 MHz
CH7   5695 MHz
CH8   5740 MHz
CH9   5770 MHz
CH10  5805 MHz
CH11  5878 MHz
CH12  5914 MHz
CH13  5955 MHz
CH14  5985 MHz
CH15  5839 MHz
```

## Band C

```text
CH1   6015 MHz
CH2   6045 MHz
CH3   6075 MHz
CH4   6110 MHz
CH5   6145 MHz
CH6   6180 MHz
CH7   6220 MHz
CH8   6255 MHz
CH9   6290 MHz
CH10  6325 MHz
CH11  6360 MHz
CH12  6395 MHz
CH13  5839 MHz
```

---

# Telemetry queries and unresolved reply layouts

The source specifies eight-byte query payloads under command `0xA2`:

| Operation | Payload |
| --- | --- |
| Read Band / Channel / Hop | `51 00 00 00 00 00 00 00` |
| Read wireless status | `52 00 00 00 00 00 00 00` |

Complete requests:

```text
FE EF A2 00 08 51 00 00 00 00 00 00 00 00 51 0D 0A
FE EF A2 00 08 52 00 00 00 00 00 00 00 00 52 0D 0A
```

The wireless-operation table describes `cmd_type` (one byte) plus `value` (seven bytes), and lists data length 8. It provides query packets but no complete RX examples that establish actual reply lengths or command bytes. In particular, it does **not** establish the four-byte and six-byte reply payloads assumed by the existing library.

## Channel fields described by the source

| Source index | Meaning |
| --- | --- |
| `value[0]` | Band: 0=A, 1=B, 2=C; other values invalid |
| `value[1]` | Channel index: documented range 0…16, subject to device/band validation |
| `value[2]` | Hop: 0=fixed frequency, 1=frequency hopping |

## Wireless-status fields described by the source

Both language versions contain the same incomplete indexing:

| Source index | Meaning |
| --- | --- |
| `value[0]` | RSSI1 |
| `value[1]` | RSSI2 |
| `value[2]` | Not described |
| `value[3]` | Data rate, Mbps |
| `value[4]` | Delay, ms |
| `value[5]` | BB connect; the explanation of value 0 is unfinished |
| `value[6]` | Not described |

These are **value-array indexes**, not full-payload offsets: if the stated `cmd_type + value` structure applies to a reply, `value[0]` would follow `payload[0]`. That conditional mapping is not a verified reply layout. Do not silently remove the missing index, invent a reserved field, or infer an exact RX length.

The source confirms the Mbps and ms units, but not the current decoder's offsets. It does not specify RSSI signedness, scale, or dBm units. Interpreting `0xA6` as signed eight-bit gives −90 mathematically; it does not establish that the device reports −90 dBm. The mapping `Connected: 0=disconnected, 1=connected` remains an implementation assumption because the source's description is incomplete.

## Existing reference decoder limitations

[`src/vrx_protocol.c`](src/vrx_protocol.c) currently accepts:

* `0x51`: exactly four payload bytes, interpreted as `51 BAND CHANNEL HOP`.
* `0x52`: exactly six payload bytes, interpreted as `52 RSSI1 RSSI2 DATARATE LATENCY CONNECTED`, with signed eight-bit RSSI.
* The STM32 example additionally filters incoming frames to command `0xA2`.

These are legacy implementation assumptions, **not source-confirmed wire formats**. The status offsets differ from the literal source table, and replies of other lengths are rejected by the decoder. This documentation update does not change the decoder based on an ambiguous table.

Capture complete raw RX frames, their declared lengths, command bytes, and checksum fields before relying on decoded telemetry. Preserve raw bytes alongside any experimental decoding.

---

# Suggested polling interval

The following intervals are implementation suggestions, not timing requirements from the supplied documents:

```text
Channel state (0x51): every 2000 ms
Link status   (0x52): every 200 ms
```

Band/channel information changes rarely.

RSSI, bitrate, and latency are more dynamic.

---

# STM32 HAL implementation

```c
HAL_StatusTypeDef vrxSendPacket(
    UART_HandleTypeDef *uart,
    uint8_t command,
    const uint8_t *payload,
    uint16_t payloadLength)
{
    uint8_t frame[256];

    if (payloadLength > 247)
        return HAL_ERROR;

    uint16_t pos = 0;
    uint16_t checksum = 0;

    frame[pos++] = 0xFE;
    frame[pos++] = 0xEF;

    frame[pos++] = command;

    frame[pos++] =
        (uint8_t)(payloadLength >> 8);

    frame[pos++] =
        (uint8_t)(payloadLength & 0xFF);

    for (uint16_t i = 0;
         i < payloadLength;
         i++)
    {
        frame[pos++] = payload[i];

        checksum =
            (uint16_t)(checksum + payload[i]);
    }

    frame[pos++] =
        (uint8_t)(checksum >> 8);

    frame[pos++] =
        (uint8_t)(checksum & 0xFF);

    frame[pos++] = 0x0D;
    frame[pos++] = 0x0A;

    return HAL_UART_Transmit(
        uart,
        frame,
        pos,
        100
    );
}
```

Key helper:

```c
HAL_StatusTypeDef vrxSendKey(
    UART_HandleTypeDef *uart,
    uint8_t key,
    uint8_t argument)
{
    uint8_t payload[4] = {
        0x40,
        key,
        argument,
        0x00
    };

    return vrxSendPacket(
        uart,
        0x22,
        payload,
        sizeof(payload)
    );
}
```

Examples:

```c
vrxSendKey(&huart1, 0x00, 0x00); // UP
vrxSendKey(&huart1, 0x01, 0x00); // DOWN
vrxSendKey(&huart1, 0x02, 0x00); // LEFT
vrxSendKey(&huart1, 0x03, 0x00); // RIGHT
vrxSendKey(&huart1, 0x04, 0x00); // ENTER
vrxSendKey(&huart1, 0x05, 0x00); // PAIR
vrxSendKey(&huart1, 0x05, 0x01); // UPGRADE key
vrxSendKey(&huart1, 0x06, 0x00); // RECORD
vrxSendKey(&huart1, 0x07, 0x00); // BACK
vrxSendKey(&huart1, 0x07, 0x01); // Force 720p60
vrxSendKey(&huart1, 0x09, 0x00); // Unlock Debug3 mode
```

---

# Minimal hardware test

After checking that these implementation-derived settings match your device, configure the UART as:

```text
115200 8N1
```

Then send:

```text
FE EF 22 00 04 40 00 00 00 00 40 0D 0A
```

This is the UP command.

Example:

```c
static const uint8_t VRX_UP[] =
{
    0xFE, 0xEF,
    0x22,
    0x00, 0x04,

    0x40,
    0x00,
    0x00,
    0x00,

    0x00, 0x40,

    0x0D, 0x0A
};

HAL_UART_Transmit(
    &huart1,
    (uint8_t *)VRX_UP,
    sizeof(VRX_UP),
    100
);
```

---

# Hardware validation wanted

Raw UART captures from physical VRX hardware would be especially useful.

Please include:

```text
VRX model
VRX firmware version
test condition
raw TX bytes
raw RX bytes
```

Suggested tests:

### No link

```text
VRX powered
VTX powered off
```

### Strong link

```text
VTX close to VRX
stable video
```

### Weakening link

Increase distance or otherwise reduce RF signal safely.

Observe:

```text
RSSI1
RSSI2
DataRate
Latency
Connected
```

### Change channel

Change:

```text
Band
Channel
Hop
```

and capture response type `0x51`.

---

# RX checksum validation

The source defines one payload-sum checksum rule for the frame format. No different RX checksum is documented. Verify that actual firmware replies conform to that rule by comparing raw captures against:

```c
uint16_t vrxChecksum(
    const uint8_t *payload,
    uint16_t length)
{
    uint16_t checksum = 0;

    for (uint16_t i = 0; i < length; i++)
        checksum += payload[i];

    return checksum;
}
```

During initial testing, do not discard telemetry solely because the checksum does not match.

Log both:

```text
received checksum
calculated checksum
```

Keep checksum-mismatched frames as diagnostic evidence, not validated telemetry. This distinguishes source errors, firmware differences, framing problems, and corrupted traffic.

---

# Protocol quick reference

## Remote key

```text
CMD = 22

40 KEY PRESS_TYPE 00
```

## Set channel

```text
CMD = 22

50 BAND CHANNEL HOP 00 00 00 00
```

## Request channel state

```text
CMD = A2

51 00 00 00 00 00 00 00
```

## Channel response

No complete source RX example is available. The library's four-byte layout is provisional; see [Telemetry queries and unresolved reply layouts](#telemetry-queries-and-unresolved-reply-layouts).

---

## Request status

```text
CMD = A2

52 00 00 00 00 00 00 00
```

## Status response

No complete source RX example is available. The library's six-byte layout conflicts with the literal source indexes; see [Wireless-status fields described by the source](#wireless-status-fields-described-by-the-source).

---

# Evidence status

## Described by the supplied serial-command documents

* Frame header, command position, two-byte payload length, two-byte payload sum, and terminator.
* Big-endian length/checksum representation in the supplied packet examples.
* Key combinations including UPGRADE, Force 720p60, Unlock Debug3, and invalid key 0x08.
* Set-channel payload, Band IDs, zero-based channel examples, and fixed/hopping mode values.
* Eight-byte query packets for 0x51 and 0x52.
* Wireless-status field names and Mbps/ms units, with incomplete field indexing.

## Still requiring clarification or hardware evidence

* UART settings, voltage levels, and physical pinout.
* Command bit-7 direction semantics versus documented 0xA2 queries.
* Actual reply command bytes, lengths, status-field offsets, and checksum conformance.
* RSSI signedness/scale and BB-connect value meanings.
* Per-band channel limits and frequency tables.
* Firmware/model differences, command timing, and upgrade-key behavior.

Documentation agreement is not a substitute for a captured device response. The `captures/` directory currently contains collection instructions rather than hardware logs.

---

# Source errata and corrected channel examples

Both language versions have the direction-bit ambiguity and incomplete status table described above. The English version also contains one extra `00` in each of its Band B CH13 and Band C CH3 examples, producing 18-byte packets despite a declared payload length of 8 (which requires 17 total bytes).

Use the Chinese source's internally consistent examples:

```text
Band B CH13, fixed frequency:
FE EF 22 00 08 50 01 0C 00 00 00 00 00 00 5D 0D 0A

Band C CH3, fixed frequency:
FE EF 22 00 08 50 02 02 00 00 00 00 00 00 54 0D 0A
```

All 20 complete Chinese-source packet examples match their declared lengths and payload sums. In the English version, 18 of 20 do; the two exceptions are above. These checks verify packet arithmetic, not device execution.

---

# Official CRSF / SBUS pass-through documentation

**Source scope:** This existing section summarizes a separate pass-through document. That document was not among the two serial-command files reviewed for this update, so its instructions and firmware-version discrepancy were not independently rechecked here.

A separate vendor-provided document describes two officially supported control-signal pass-through modes for the Ascent system:

* CRSF receiver pass-through
* SBUS pass-through

These functions are related to the overall Ascent data path, but they should **not** be confused with the VRX UART control protocol documented in this repository.

The protocol documented in this repository:

```text
FE EF CMD LEN_H LEN_L PAYLOAD... SUM_H SUM_L 0D 0A
```

is used to control and query the VRX itself.

The CRSF and SBUS functions described below transport RC control data through the Ascent system.

## CRSF pass-through

The documented CRSF configuration is:

```text
Radio
  │
  ▼
ELRS receiver
  │
  │ 6-pin connection
  ▼
Ascent VRX
  │
  │ Ascent RF link
  ▼
Ascent GT
  │
  │ 2-pin serial connection
  ▼
Flight controller UART
  │
  └── configured as CRSF receiver
```

The vendor document specifies:

1. Upgrade the Ascent GT and Ascent VRX to a sufficiently recent firmware version.
2. Connect the Ascent GT 2-pin port to a flight-controller serial port.
3. Configure that flight-controller serial port as a CRSF receiver.
4. Connect an ELRS receiver to the Ascent VRX 6-pin port.
5. Bind the radio transmitter and ELRS receiver.
6. Set the transmitter Packet Rate to `50 Hz`.
7. Enable the receiver function for the corresponding serial port in the ground-station / flight-controller configuration.
8. Verify that RC channel data is received by the flight controller.

This demonstrates that the Ascent RF link can transport CRSF receiver/control data from the VRX side to the GT side.

## SBUS pass-through

The documented SBUS configuration is:

```text
Radio SBUS output
       │
       ▼
   Ascent VRX
       │
       │ Ascent RF link
       ▼
 Ascent GT / VTX
       │
       │ SBUS
       ▼
Flight controller
```

The vendor document specifies:

1. Connect the Ascent VRX SBUS port to the radio transmitter's SBUS output.
2. Set the transmitter Internal RF to `Off`.
3. Set External RF to `SBUS`.
4. Configure the flight controller to use an SBUS receiver.
5. Connect the Ascent GT VTX SBUS port to the flight-controller SBUS input.
6. Enable the corresponding receiver interface.
7. Select SBUS and verify that RC control data is received.

## Relationship to the VRX UART protocol

The existence of CRSF and SBUS pass-through provides useful architectural context.

It shows that the Ascent system contains mechanisms for transporting digital control information across the ground-to-air link:

```text
                     ASCENT SYSTEM

       Ground side                         Air side

 ┌──────────────────────┐            ┌──────────────────────┐
 │      Ascent VRX      │            │      Ascent GT       │
 │                      │            │                      │
 │ CRSF / ELRS input ───┼── RF link ─┼──► CRSF / UART → FC │
 │                      │            │                      │
 │ SBUS input ──────────┼── RF link ─┼──► SBUS → FC        │
 │                      │            │                      │
 │ VRX control UART     │            │                      │
 │ FE EF ...            │            │                      │
 └─────────▲────────────┘            └──────────────────────┘
           │
           │
     External MCU
 STM32 / ESP32 / etc.
```

However, there is currently **no evidence that the CRSF/SBUS pass-through interface and the `FE EF` VRX control interface are the same physical or logical protocol**.

In particular, the vendor document does not specify for the CRSF pass-through interface:

* UART baud rate
* electrical voltage levels
* complete 6-pin pinout
* packet framing below CRSF
* whether the VRX internally encapsulates CRSF
* whether the 6-pin interface is shared with the VRX control UART
* any relationship to the `FE EF` command protocol

Therefore these interfaces are intentionally documented separately.

## Firmware-version discrepancy in the source document

The supplied document contains an apparent version-number inconsistency.

The Chinese instructions state:

```text
16.4.2 以上版本
```

which means version `16.4.2` or later.

The English translation in the same document states:

```text
16.2.1 or higher
```

The discrepancy appears in both the CRSF and SBUS sections.

Until this is clarified by the vendor or verified on hardware, this repository does not assume which of those two version numbers is correct.

## Why this matters

The official pass-through documentation independently confirms an important part of the Ascent architecture:

> The Ascent ground/air link is capable of transporting digital RC control data in addition to the video link.

This is consistent with the system architecture observed during interoperability research, but it does **not** by itself validate the reverse-engineered VRX UART frame format, commands, checksum, or telemetry documented elsewhere in this repository.

The control commands are now cross-checked against the supplied serial-command documents; the unresolved reply behavior still requires hardware evidence.

---

# Related Ascent VRX Max interfaces and the separate OTRA protocol

## Scope and evidence

TelaMechanica's [Ascent VRX Max UDP client](https://github.com/TelaMechanica/Caddx_vrx_max_udp_protocol/blob/f5cda0a755ff804bc4332e29a14f01b20a66bbde/README.md) explicitly limits its UDP protocol to **Ascent VRX Max**. The details below describe that third-party implementation and its accompanying documents, rather than a hardware validation performed by this project. Links identify the source revisions reviewed for this addition.

Similar framing does not establish that every Max UDP command is supported by the control UART, by the original Ascent VRX, or by every firmware version. These sources do not establish a shared physical port or pinout. The serial-command documents remain the primary source for the control commands documented above.

## Control and OSD over UDP

The [client implementation](https://github.com/TelaMechanica/Caddx_vrx_max_udp_protocol/blob/f5cda0a755ff804bc4332e29a14f01b20a66bbde/udp_vrx_max_client.py) defines these default device endpoints:

| Connection profile | Device IP | Control UDP port | OSD UDP port |
| --- | --- | --- | --- |
| RJ45 Ethernet | `192.168.1.100` | `9001` | `9200` |
| USB-C network | `192.168.3.102` | `9001` | `9200` |

These are client defaults, not discovery results or universal device settings. USB-C here refers to a network interface, not a USB serial port.

The control client sends one `FE EF … 0D 0A` frame per UDP datagram, using a two-byte big-endian payload length and a two-byte big-endian payload sum. The OSD channel is separate: it receives MSP v1/v2 traffic, including MSP DisplayPort, rather than the VRX control frame format. See the [control/OSD channel description](https://github.com/TelaMechanica/Caddx_vrx_max_udp_protocol/blob/f5cda0a755ff804bc4332e29a14f01b20a66bbde/UserManuals/Description%20of%20Ctrl%20and%20OSD%20UDP%20Channels.md).

For integration, keep the two receive paths separate. The referenced OSD implementation reassembles MSP frames across UDP datagrams before parsing them; UDP datagram boundaries are not assumed to be MSP frame boundaries.

## Additional commands implemented by the Max client

The following are **implementation-derived Max UDP commands**, not additions to this repository's verified UART command set. Their construction is visible in [`build_command_payload()` and `build_set_mode_packet()`](https://github.com/TelaMechanica/Caddx_vrx_max_udp_protocol/blob/f5cda0a755ff804bc4332e29a14f01b20a66bbde/udp_vrx_max_client.py).

| Operation | Outer command | Payload | Implementation notes |
| --- | --- | --- | --- |
| Get power index | `0xA2` | `53 00 00 00 00 00 00 00` | Client interprets the result as a power index and a four-byte bitmap of settable indexes. |
| Set power index | `0x22` | `54 INDEX 00 00 00 00 00 00` | CLI accepts indexes 0–19; this does not establish an index-to-milliwatt mapping or support for every index on a device. |
| Switch passthrough/output mode | `0x23` | `02 MODE 00 00 00` | CLI exposes CRSF (`MODE=2`) and OSD (`MODE=5`). |

Other mode names in the source enum are implementation declarations, not evidence that all modes work on every device. This section does not change the existing UART library or resolve its open reply-format questions.

The Python OSD startup sequence starts both sockets, sends the OSD mode switch on port 9001, waits 500 ms, then sends probe bytes `31 32 33 34` to port 9200. Treat this as the client's startup procedure, not a proven minimum delay or universal handshake.

## Reported behavior and source inconsistencies

The [Max troubleshooting guide](https://github.com/TelaMechanica/Caddx_vrx_max_udp_protocol/blob/f5cda0a755ff804bc4332e29a14f01b20a66bbde/UserManuals/VRX_Troubleshooting_Guide_EN_CN.md) reports that key simulation does not return an ACK and that Force 720p60 restarts the VRX to apply the output setting. Consequently, missing key-command acknowledgments alone should not be treated as evidence of failure in that described setup, and a Force 720p60 action may temporarily interrupt communication. These are source-reported behaviors, not guarantees for every UART device or firmware.

Some accompanying documentation conflicts with the implementation:

* The troubleshooting guide lists `192.169.*` endpoints, whereas the README and Python profiles use `192.168.*`. The table above follows the actual client profiles.
* The [packet summary](https://github.com/TelaMechanica/Caddx_vrx_max_udp_protocol/blob/f5cda0a755ff804bc4332e29a14f01b20a66bbde/UserManuals/UART_Protocol_Packet_Summary.md) contains an UP example with checksum `04 00` and tail `D0 A0`. Those contradict its own framing rules and the client, which use `00 40` and `0D 0A` for that command. Do not copy those malformed examples.
* The channel description mentions a `$MSP` probe in one startup path, while the Python client uses `31 32 33 34`. The startup description above follows the Python implementation.

These inconsistencies are why this section cites concrete implementations and keeps model/transport limitations explicit.

## OTRA is a separate transfer protocol

TelaMechanica also publishes [Ascent VRX rooting research](https://github.com/TelaMechanica/Walksnail-Ascent-FPV-VRX-Rooting-Exploit/blob/488bcf06a28bd6788b6b53e99bdbcc6eee11a9af/README.md). Its [serial-transfer implementation](https://github.com/TelaMechanica/Walksnail-Ascent-FPV-VRX-Rooting-Exploit/blob/488bcf06a28bd6788b6b53e99bdbcc6eee11a9af/exploit.py) uses a different protocol:

| Property | VRX control protocol described here | OTRA implementation in the referenced research |
| --- | --- | --- |
| Start marker | `FE EF` | ASCII `OTRA` |
| Header before payload | 5 bytes | 36 bytes |
| Integrity check | 16-bit payload-byte sum | CRC32 over the header with its CRC field zeroed, plus payload |
| Purpose in the referenced code | Key actions and parameter control/query | File-transfer/update-related sessions used by the rooting research |

The OTRA script configures its serial connection as `115200 8N1`. This establishes a setting used by that script, not an independent confirmation of the physical control UART's settings, voltage, or pinout.

OTRA packets must not be passed to the `FE EF` control parser. The Upgrade key documented earlier is a key action; the existence of this separate transfer implementation does not establish how that key relates to an OTRA session. The research is linked for protocol context only; no rooting or firmware-transfer procedure is required for the control interface described in this repository.

---

# Repository structure

Suggested layout:

```text
walksnail-vrx-uart/
│
├── README.md
├── LICENSE
│
├── src/
│   ├── vrx_protocol.c
│   └── vrx_protocol.h
│
├── examples/
│   ├── stm32_hal/
│   └── arduino/
│
├── docs/
│   └── telemetry.md
│
└── captures/
    └── README.md
```

---

# Contributing

Pull requests and hardware validation are welcome.

Especially valuable:

* raw UART captures
* additional command discovery
* validation on different VRX models
* validation on different firmware versions
* telemetry interpretation
* checksum confirmation
* MCU examples
* logic-analyzer captures

When possible, always include the unmodified raw UART bytes.

---

# Why publish this?

Closed protocols make otherwise useful hardware unnecessarily difficult to integrate into custom systems.

A documented UART interface opens the door to:

* custom FPV controllers
* STM32 projects
* ESP32 projects
* antenna trackers
* ground stations
* telemetry displays
* automated test rigs
* embedded Linux systems
* RP2040 projects
* custom user interfaces

The goal of this repository is not to reproduce proprietary firmware.

The goal is to document an interface so independently developed hardware and software can interoperate with equipment owned by the user.

---

# Credits

This is an independent community interoperability project.

Protocol documentation and reference code were developed through collaborative research, implementation analysis, and testing.

AI-assisted analysis was performed with **ChatGPT by OpenAI**.

Independent hardware verification and additional protocol captures are welcome.

---

# Disclaimer

This project is independent and is not affiliated with, endorsed by, or supported by Walksnail or Caddx.

No proprietary firmware is distributed by this repository.

The information is provided for:

* interoperability
* research
* education
* experimentation
* development with user-owned hardware

Use at your own risk.

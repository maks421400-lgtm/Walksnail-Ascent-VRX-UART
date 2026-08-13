# Walksnail Ascent VRX UART Protocol


# Walksnail VRX UART Protocol

Open documentation and reference implementation of the **Walksnail VRX UART control and telemetry protocol**.

The goal of this project is to make Walksnail VRX hardware easier to integrate with custom controllers, STM32 projects, ground stations, antenna trackers, test equipment, and other user-built systems.

The protocol information below was obtained through independent interoperability research and analysis of compatible implementations.

Where possible, confirmed protocol behavior is separated from details that still require validation on physical hardware.

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
* PAIR / RECORD
* FORCE720 / DEBUG
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

The payload length is encoded as an unsigned 16-bit **big-endian** integer.

Example for a four-byte payload:

```text
00 04
```

Total frame size is:

```text
payload_length + 9
```

---

# Checksum

The checksum used for transmitted commands is a simple unsigned 16-bit sum of the payload bytes.

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
40 KEY ARG 00
```

Generic key packet:

```text
FE EF 22 00 04 40 KEY ARG 00 SUM_H SUM_L 0D 0A
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

| Function |    KEY |    ARG |
| -------- | -----: | -----: |
| UP       | `0x00` | `0x00` |
| DOWN     | `0x01` | `0x00` |
| LEFT     | `0x02` | `0x00` |
| RIGHT    | `0x03` | `0x00` |
| ENTER    | `0x04` | `0x00` |
| PAIR     | `0x05` | `0x00` |
| RECORD   | `0x06` | `0x00` |
| BACK     | `0x07` | `0x00` |
| FORCE720 | `0x07` | `0x01` |
| DEBUG    | `0x09` | `0x00` |

A possible command associated with key code `0x08` has not yet been sufficiently verified and is therefore intentionally omitted.

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

## RECORD

```text
FE EF 22 00 04 40 06 00 00 00 46 0D 0A
```

## BACK

```text
FE EF 22 00 04 40 07 00 00 00 47 0D 0A
```

## FORCE720

```text
FE EF 22 00 04 40 07 01 00 00 48 0D 0A
```

## DEBUG

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

Known values:

```text
0 = Band A
1 = Band B
2 = Band C
```

Channels are represented internally as zero-based indexes.

Therefore:

```text
raw channel 0 = displayed channel 1
raw channel 1 = displayed channel 2
...
```

Hop:

```text
0 = Off
1 = On
```

---

# Known frequency tables

The following frequency tables were identified in a compatible implementation.

They should be considered implementation data rather than a universal guarantee for every VRX model or firmware version.

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

# Telemetry

Two request types are currently documented:

```text
0x51 = Channel / frequency state
0x52 = Link status
```

Requests use:

```text
CMD = 0xA2
```

---

# Request channel state

Payload:

```text
51 00 00 00 00 00 00 00
```

Complete packet:

```text
FE EF A2 00 08 51 00 00 00 00 00 00 00 00 51 0D 0A
```

---

# Channel-state response

The response payload has four bytes:

```text
51 BAND CHANNEL HOP
```

Decoded:

```text
payload[0] = response type = 0x51
payload[1] = Band
payload[2] = Channel
payload[3] = Hop
```

Frame shape:

```text
FE EF A2 00 04
51 BAND CHANNEL HOP
SUM_H SUM_L
0D 0A
```

The reply checksum behavior should be verified against raw hardware captures before strict validation is enabled.

---

# Request link status

Payload:

```text
52 00 00 00 00 00 00 00
```

Complete request:

```text
FE EF A2 00 08 52 00 00 00 00 00 00 00 00 52 0D 0A
```

---

# Link-status response

The response payload contains:

```text
52 RSSI1 RSSI2 DATARATE LATENCY CONNECTED
```

Decoded:

```text
payload[0] = response type = 0x52
payload[1] = RSSI1
payload[2] = RSSI2
payload[3] = DataRate
payload[4] = Latency
payload[5] = Connected
```

Frame:

```text
FE EF A2 00 06
52 RSSI1 RSSI2 DATARATE LATENCY CONNECTED
SUM_H SUM_L
0D 0A
```

---

# Telemetry structure

A convenient representation:

```c
typedef struct
{
    uint8_t band;
    uint8_t channel;
    uint8_t hop;

    int8_t rssi1;
    int8_t rssi2;

    uint8_t dataRate;
    uint8_t latency;
    uint8_t connected;

} VrxTelemetry;
```

Logical layout:

```text
Band
Channel
Hop
RSSI1
RSSI2
DataRate
Latency
Connected
```

---

# RSSI

RSSI bytes can be interpreted as signed 8-bit values.

Example:

```text
Raw byte:
A6
```

As `int8_t`:

```text
-90
```

Example code:

```c
int8_t rssi =
    (int8_t)payload[1];
```

The exact physical interpretation should still be verified with RF testing.

---

# Data Rate

The DataRate byte is exposed directly as an Mbps value by compatible implementations.

Example:

```text
18
```

corresponds to:

```text
18 Mbps
```

---

# Latency

Latency is similarly represented directly as a one-byte millisecond value.

Example:

```text
27
```

corresponds to:

```text
27 ms
```

---

# Connected

The link state is represented as a one-byte value.

Expected interpretation:

```text
0 = disconnected
1 = connected
```

Hardware captures are welcome to confirm whether additional values are ever used.

---

# Telemetry parser

```c
typedef struct
{
    uint8_t band;
    uint8_t channel;
    uint8_t hop;

    int8_t rssi1;
    int8_t rssi2;

    uint8_t dataRate;
    uint8_t latency;
    uint8_t connected;

} VrxTelemetry;


static VrxTelemetry vrxTelemetry;


void vrxProcessPayload(
    const uint8_t *payload,
    uint16_t length)
{
    if (payload == NULL || length == 0)
        return;

    switch (payload[0])
    {
        case 0x51:

            if (length == 4)
            {
                vrxTelemetry.band =
                    payload[1];

                vrxTelemetry.channel =
                    payload[2];

                vrxTelemetry.hop =
                    payload[3];
            }

            break;


        case 0x52:

            if (length == 6)
            {
                vrxTelemetry.rssi1 =
                    (int8_t)payload[1];

                vrxTelemetry.rssi2 =
                    (int8_t)payload[2];

                vrxTelemetry.dataRate =
                    payload[3];

                vrxTelemetry.latency =
                    payload[4];

                vrxTelemetry.connected =
                    payload[5];
            }

            break;
    }
}
```

---

# Suggested polling interval

A practical starting point:

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
vrxSendKey(&huart1, 0x06, 0x00); // RECORD
vrxSendKey(&huart1, 0x07, 0x00); // BACK
vrxSendKey(&huart1, 0x07, 0x01); // FORCE720
vrxSendKey(&huart1, 0x09, 0x00); // DEBUG
```

---

# Minimal hardware test

Configure the UART as:

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

TX checksum behavior is documented.

Before assuming that VRX replies use exactly the same checksum calculation, compare real captures against:

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

This allows the RX algorithm to be determined without losing valid traffic.

---

# Protocol quick reference

## Remote key

```text
CMD = 22

40 KEY ARG 00
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

```text
CMD = A2

51 BAND CHANNEL HOP
```

## Request status

```text
CMD = A2

52 00 00 00 00 00 00 00
```

## Status response

```text
CMD = A2

52 RSSI1 RSSI2 DATARATE LATENCY CONNECTED
```

---

# Confirmed protocol behavior

Currently considered confirmed:

* UART `115200 8N1`
* `FE EF` frame header
* command byte position
* big-endian 16-bit payload length
* TX additive payload checksum
* big-endian TX checksum
* `0D 0A` terminator
* command `0x22`
* key prefix `0x40`
* UP
* DOWN
* LEFT
* RIGHT
* ENTER
* PAIR
* RECORD
* BACK
* FORCE720
* DEBUG
* channel-control prefix `0x50`
* query command `0xA2`
* response/request identifiers `0x51` and `0x52`
* Band
* Channel
* Hop
* RSSI1
* RSSI2
* DataRate
* Latency
* Connected
* Band IDs A/B/C
* zero-based internal channel numbering
* Hop Off/On representation

---

# Still being verified

Open questions:

* checksum algorithm used by VRX replies
* exact physical RSSI interpretation across hardware versions
* additional `Connected` states, if any
* additional undocumented response types
* additional key codes
* differences between VRX firmware versions
* differences between VRX hardware generations
* complete verification of frequency tables

If you have hardware captures, contributions are welcome.

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

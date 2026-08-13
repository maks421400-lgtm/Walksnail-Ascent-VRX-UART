#ifndef VRX_PROTOCOL_H
#define VRX_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VRX_FRAME_HEADER_0       0xFEu
#define VRX_FRAME_HEADER_1       0xEFu
#define VRX_FRAME_TERMINATOR_0   0x0Du
#define VRX_FRAME_TERMINATOR_1   0x0Au

#define VRX_CMD_CONTROL          0x22u
#define VRX_CMD_QUERY            0xA2u

#define VRX_PAYLOAD_KEY          0x40u
#define VRX_PAYLOAD_CHANNEL      0x50u
#define VRX_PAYLOAD_FREQUENCY    0x51u
#define VRX_PAYLOAD_STATUS       0x52u

#define VRX_MAX_PAYLOAD          64u
#define VRX_MAX_FRAME_SIZE       (VRX_MAX_PAYLOAD + 9u)

typedef enum
{
    VRX_OK = 0,
    VRX_ERROR_ARGUMENT = -1,
    VRX_ERROR_TOO_LARGE = -2,
    VRX_ERROR_IO = -3
} VrxResult;

typedef enum
{
    VRX_KEY_UP = 0x00,
    VRX_KEY_DOWN = 0x01,
    VRX_KEY_LEFT = 0x02,
    VRX_KEY_RIGHT = 0x03,
    VRX_KEY_ENTER = 0x04,
    VRX_KEY_PAIR = 0x05,
    VRX_KEY_RECORD = 0x06,
    VRX_KEY_BACK = 0x07,
    VRX_KEY_DEBUG = 0x09
} VrxKeyCode;

typedef struct
{
    uint8_t band;
    uint8_t channel;
    uint8_t hop;

    int8_t rssi1;
    int8_t rssi2;

    uint8_t data_rate_mbps;
    uint8_t latency_ms;
    uint8_t connected;
} VrxTelemetry;

/*
 * User-supplied UART writer.
 *
 * Return 0 on success.
 * Return a non-zero value on failure.
 */
typedef int (*VrxWriteFn)(
    const uint8_t *data,
    size_t length,
    void *user);

/*
 * Optional callback invoked after a valid frame has been assembled.
 *
 * The callback is called after header, length and CR/LF framing checks.
 * If strict RX checksum verification is enabled, it is called only for
 * checksum-valid frames.
 */
typedef void (*VrxFrameFn)(
    uint8_t command,
    const uint8_t *payload,
    uint16_t payload_length,
    uint16_t received_checksum,
    uint16_t calculated_checksum,
    void *user);

typedef enum
{
    VRX_RX_WAIT_FE = 0,
    VRX_RX_WAIT_EF,
    VRX_RX_COMMAND,
    VRX_RX_LENGTH_H,
    VRX_RX_LENGTH_L,
    VRX_RX_PAYLOAD,
    VRX_RX_CHECKSUM_H,
    VRX_RX_CHECKSUM_L,
    VRX_RX_CR,
    VRX_RX_LF
} VrxRxState;

typedef struct
{
    VrxRxState state;

    uint8_t command;

    uint16_t payload_length;
    uint16_t payload_position;

    uint8_t payload[VRX_MAX_PAYLOAD];

    uint16_t received_checksum;

    uint8_t strict_rx_checksum;

    VrxFrameFn frame_callback;
    void *frame_user;
} VrxParser;

/* Core helpers */
uint16_t vrx_checksum(
    const uint8_t *payload,
    uint16_t payload_length);

VrxResult vrx_send_packet(
    VrxWriteFn write_fn,
    void *write_user,
    uint8_t command,
    const uint8_t *payload,
    uint16_t payload_length);

/* Remote-control keys */
VrxResult vrx_send_key(
    VrxWriteFn write_fn,
    void *write_user,
    uint8_t key,
    uint8_t argument);

VrxResult vrx_up(VrxWriteFn write_fn, void *user);
VrxResult vrx_down(VrxWriteFn write_fn, void *user);
VrxResult vrx_left(VrxWriteFn write_fn, void *user);
VrxResult vrx_right(VrxWriteFn write_fn, void *user);
VrxResult vrx_enter(VrxWriteFn write_fn, void *user);
VrxResult vrx_pair(VrxWriteFn write_fn, void *user);
VrxResult vrx_record(VrxWriteFn write_fn, void *user);
VrxResult vrx_back(VrxWriteFn write_fn, void *user);
VrxResult vrx_force720(VrxWriteFn write_fn, void *user);
VrxResult vrx_debug(VrxWriteFn write_fn, void *user);

/* Channel control */
VrxResult vrx_set_channel(
    VrxWriteFn write_fn,
    void *write_user,
    uint8_t band,
    uint8_t channel,
    uint8_t hop);

/* Telemetry requests */
VrxResult vrx_request_frequency(
    VrxWriteFn write_fn,
    void *write_user);

VrxResult vrx_request_status(
    VrxWriteFn write_fn,
    void *write_user);

/* RX parser */
void vrx_parser_init(
    VrxParser *parser,
    VrxFrameFn frame_callback,
    void *frame_user);

void vrx_parser_set_strict_rx_checksum(
    VrxParser *parser,
    uint8_t enabled);

void vrx_parser_reset(VrxParser *parser);

void vrx_parser_feed(
    VrxParser *parser,
    uint8_t byte);

/* Decode known telemetry payloads into VrxTelemetry. */
uint8_t vrx_decode_telemetry(
    const uint8_t *payload,
    uint16_t payload_length,
    VrxTelemetry *telemetry);

#ifdef __cplusplus
}
#endif

#endif /* VRX_PROTOCOL_H */

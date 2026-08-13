#include "vrx_protocol.h"

static VrxResult vrx_write(
    VrxWriteFn write_fn,
    void *write_user,
    const uint8_t *data,
    size_t length)
{
    if (write_fn == NULL || data == NULL || length == 0u)
        return VRX_ERROR_ARGUMENT;

    return (write_fn(data, length, write_user) == 0)
        ? VRX_OK
        : VRX_ERROR_IO;
}

uint16_t vrx_checksum(
    const uint8_t *payload,
    uint16_t payload_length)
{
    uint16_t sum = 0u;
    uint16_t i;

    if (payload == NULL && payload_length != 0u)
        return 0u;

    for (i = 0u; i < payload_length; ++i)
        sum = (uint16_t)(sum + payload[i]);

    return sum;
}

VrxResult vrx_send_packet(
    VrxWriteFn write_fn,
    void *write_user,
    uint8_t command,
    const uint8_t *payload,
    uint16_t payload_length)
{
    uint8_t frame[VRX_MAX_FRAME_SIZE];
    uint16_t checksum;
    size_t pos = 0u;
    uint16_t i;

    if (write_fn == NULL)
        return VRX_ERROR_ARGUMENT;

    if (payload_length > VRX_MAX_PAYLOAD)
        return VRX_ERROR_TOO_LARGE;

    if (payload_length != 0u && payload == NULL)
        return VRX_ERROR_ARGUMENT;

    checksum = vrx_checksum(payload, payload_length);

    frame[pos++] = VRX_FRAME_HEADER_0;
    frame[pos++] = VRX_FRAME_HEADER_1;
    frame[pos++] = command;

    frame[pos++] = (uint8_t)(payload_length >> 8);
    frame[pos++] = (uint8_t)(payload_length & 0xFFu);

    for (i = 0u; i < payload_length; ++i)
        frame[pos++] = payload[i];

    frame[pos++] = (uint8_t)(checksum >> 8);
    frame[pos++] = (uint8_t)(checksum & 0xFFu);

    frame[pos++] = VRX_FRAME_TERMINATOR_0;
    frame[pos++] = VRX_FRAME_TERMINATOR_1;

    return vrx_write(write_fn, write_user, frame, pos);
}

VrxResult vrx_send_key(
    VrxWriteFn write_fn,
    void *write_user,
    uint8_t key,
    uint8_t argument)
{
    const uint8_t payload[4] = {
        VRX_PAYLOAD_KEY,
        key,
        argument,
        0x00u
    };

    return vrx_send_packet(
        write_fn,
        write_user,
        VRX_CMD_CONTROL,
        payload,
        (uint16_t)sizeof(payload));
}

VrxResult vrx_up(VrxWriteFn write_fn, void *user)
{
    return vrx_send_key(write_fn, user, VRX_KEY_UP, 0x00u);
}

VrxResult vrx_down(VrxWriteFn write_fn, void *user)
{
    return vrx_send_key(write_fn, user, VRX_KEY_DOWN, 0x00u);
}

VrxResult vrx_left(VrxWriteFn write_fn, void *user)
{
    return vrx_send_key(write_fn, user, VRX_KEY_LEFT, 0x00u);
}

VrxResult vrx_right(VrxWriteFn write_fn, void *user)
{
    return vrx_send_key(write_fn, user, VRX_KEY_RIGHT, 0x00u);
}

VrxResult vrx_enter(VrxWriteFn write_fn, void *user)
{
    return vrx_send_key(write_fn, user, VRX_KEY_ENTER, 0x00u);
}

VrxResult vrx_pair(VrxWriteFn write_fn, void *user)
{
    return vrx_send_key(write_fn, user, VRX_KEY_PAIR, 0x00u);
}

VrxResult vrx_record(VrxWriteFn write_fn, void *user)
{
    return vrx_send_key(write_fn, user, VRX_KEY_RECORD, 0x00u);
}

VrxResult vrx_back(VrxWriteFn write_fn, void *user)
{
    return vrx_send_key(write_fn, user, VRX_KEY_BACK, 0x00u);
}

VrxResult vrx_force720(VrxWriteFn write_fn, void *user)
{
    return vrx_send_key(write_fn, user, VRX_KEY_BACK, 0x01u);
}

VrxResult vrx_debug(VrxWriteFn write_fn, void *user)
{
    return vrx_send_key(write_fn, user, VRX_KEY_DEBUG, 0x00u);
}

VrxResult vrx_set_channel(
    VrxWriteFn write_fn,
    void *write_user,
    uint8_t band,
    uint8_t channel,
    uint8_t hop)
{
    const uint8_t payload[8] = {
        VRX_PAYLOAD_CHANNEL,
        band,
        channel,
        hop,
        0x00u,
        0x00u,
        0x00u,
        0x00u
    };

    return vrx_send_packet(
        write_fn,
        write_user,
        VRX_CMD_CONTROL,
        payload,
        (uint16_t)sizeof(payload));
}

VrxResult vrx_request_frequency(
    VrxWriteFn write_fn,
    void *write_user)
{
    const uint8_t payload[8] = {
        VRX_PAYLOAD_FREQUENCY,
        0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u
    };

    return vrx_send_packet(
        write_fn,
        write_user,
        VRX_CMD_QUERY,
        payload,
        (uint16_t)sizeof(payload));
}

VrxResult vrx_request_status(
    VrxWriteFn write_fn,
    void *write_user)
{
    const uint8_t payload[8] = {
        VRX_PAYLOAD_STATUS,
        0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u
    };

    return vrx_send_packet(
        write_fn,
        write_user,
        VRX_CMD_QUERY,
        payload,
        (uint16_t)sizeof(payload));
}

void vrx_parser_reset(VrxParser *parser)
{
    if (parser == NULL)
        return;

    parser->state = VRX_RX_WAIT_FE;
    parser->command = 0u;
    parser->payload_length = 0u;
    parser->payload_position = 0u;
    parser->received_checksum = 0u;
}

void vrx_parser_init(
    VrxParser *parser,
    VrxFrameFn frame_callback,
    void *frame_user)
{
    if (parser == NULL)
        return;

    parser->strict_rx_checksum = 0u;
    parser->frame_callback = frame_callback;
    parser->frame_user = frame_user;

    vrx_parser_reset(parser);
}

void vrx_parser_set_strict_rx_checksum(
    VrxParser *parser,
    uint8_t enabled)
{
    if (parser == NULL)
        return;

    parser->strict_rx_checksum = (enabled != 0u) ? 1u : 0u;
}

void vrx_parser_feed(
    VrxParser *parser,
    uint8_t byte)
{
    uint16_t calculated_checksum;

    if (parser == NULL)
        return;

    switch (parser->state)
    {
        case VRX_RX_WAIT_FE:
            if (byte == VRX_FRAME_HEADER_0)
                parser->state = VRX_RX_WAIT_EF;
            break;

        case VRX_RX_WAIT_EF:
            if (byte == VRX_FRAME_HEADER_1)
            {
                parser->state = VRX_RX_COMMAND;
            }
            else if (byte == VRX_FRAME_HEADER_0)
            {
                parser->state = VRX_RX_WAIT_EF;
            }
            else
            {
                parser->state = VRX_RX_WAIT_FE;
            }
            break;

        case VRX_RX_COMMAND:
            parser->command = byte;
            parser->state = VRX_RX_LENGTH_H;
            break;

        case VRX_RX_LENGTH_H:
            parser->payload_length = (uint16_t)byte << 8;
            parser->state = VRX_RX_LENGTH_L;
            break;

        case VRX_RX_LENGTH_L:
            parser->payload_length |= byte;
            parser->payload_position = 0u;

            if (parser->payload_length > VRX_MAX_PAYLOAD)
            {
                vrx_parser_reset(parser);
            }
            else if (parser->payload_length == 0u)
            {
                parser->state = VRX_RX_CHECKSUM_H;
            }
            else
            {
                parser->state = VRX_RX_PAYLOAD;
            }
            break;

        case VRX_RX_PAYLOAD:
            parser->payload[parser->payload_position++] = byte;

            if (parser->payload_position >= parser->payload_length)
                parser->state = VRX_RX_CHECKSUM_H;
            break;

        case VRX_RX_CHECKSUM_H:
            parser->received_checksum = (uint16_t)byte << 8;
            parser->state = VRX_RX_CHECKSUM_L;
            break;

        case VRX_RX_CHECKSUM_L:
            parser->received_checksum |= byte;
            parser->state = VRX_RX_CR;
            break;

        case VRX_RX_CR:
            if (byte == VRX_FRAME_TERMINATOR_0)
                parser->state = VRX_RX_LF;
            else
                vrx_parser_reset(parser);
            break;

        case VRX_RX_LF:
            if (byte == VRX_FRAME_TERMINATOR_1)
            {
                calculated_checksum = vrx_checksum(
                    parser->payload,
                    parser->payload_length);

                if ((!parser->strict_rx_checksum ||
                     calculated_checksum == parser->received_checksum) &&
                    parser->frame_callback != NULL)
                {
                    parser->frame_callback(
                        parser->command,
                        parser->payload,
                        parser->payload_length,
                        parser->received_checksum,
                        calculated_checksum,
                        parser->frame_user);
                }
            }

            vrx_parser_reset(parser);
            break;

        default:
            vrx_parser_reset(parser);
            break;
    }
}

uint8_t vrx_decode_telemetry(
    const uint8_t *payload,
    uint16_t payload_length,
    VrxTelemetry *telemetry)
{
    if (payload == NULL || telemetry == NULL || payload_length == 0u)
        return 0u;

    if (payload[0] == VRX_PAYLOAD_FREQUENCY &&
        payload_length == 4u)
    {
        telemetry->band = payload[1];
        telemetry->channel = payload[2];
        telemetry->hop = payload[3];
        return 1u;
    }

    if (payload[0] == VRX_PAYLOAD_STATUS &&
        payload_length == 6u)
    {
        telemetry->rssi1 = (int8_t)payload[1];
        telemetry->rssi2 = (int8_t)payload[2];
        telemetry->data_rate_mbps = payload[3];
        telemetry->latency_ms = payload[4];
        telemetry->connected = payload[5];
        return 1u;
    }

    return 0u;
}

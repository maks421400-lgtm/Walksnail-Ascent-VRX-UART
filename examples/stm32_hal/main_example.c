/*
 * Minimal STM32 HAL example for walksnail-vrx-uart.
 *
 * UART settings:
 *   115200 baud
 *   8 data bits
 *   no parity
 *   1 stop bit
 *
 * Adjust huart1 to match your project.
 */

#include "main.h"
#include "vrx_protocol.h"

static VrxParser vrx_parser;
static VrxTelemetry vrx_telemetry;

extern UART_HandleTypeDef huart1;

/*
 * Portable library callback -> STM32 HAL.
 * Return 0 on success, non-zero on error.
 */
static int vrx_uart_write(
    const uint8_t *data,
    size_t length,
    void *user)
{
    UART_HandleTypeDef *uart = (UART_HandleTypeDef *)user;

    if (HAL_UART_Transmit(
            uart,
            (uint8_t *)data,
            (uint16_t)length,
            100u) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

static void vrx_frame_received(
    uint8_t command,
    const uint8_t *payload,
    uint16_t payload_length,
    uint16_t received_checksum,
    uint16_t calculated_checksum,
    void *user)
{
    (void)user;

    /*
     * RX checksum behavior still deserves hardware confirmation.
     * During early captures, log both values rather than dropping frames.
     */
    (void)received_checksum;
    (void)calculated_checksum;

    if (command != VRX_CMD_QUERY)
        return;

    (void)vrx_decode_telemetry(
        payload,
        payload_length,
        &vrx_telemetry);
}

void app_vrx_init(void)
{
    vrx_parser_init(
        &vrx_parser,
        vrx_frame_received,
        NULL);

    /*
     * Keep disabled until VRX reply checksum behavior has been
     * confirmed on real hardware captures.
     */
    vrx_parser_set_strict_rx_checksum(
        &vrx_parser,
        0u);
}

/*
 * Feed every received VRX UART byte here.
 *
 * This can be called from an interrupt/DMA consumer or from a normal
 * polling loop depending on the STM32 application.
 */
void app_vrx_rx_byte(uint8_t byte)
{
    vrx_parser_feed(&vrx_parser, byte);
}

void app_vrx_test_commands(void)
{
    /*
     * Confirmed key examples.
     */
    (void)vrx_up(vrx_uart_write, &huart1);

    HAL_Delay(200u);

    (void)vrx_record(vrx_uart_write, &huart1);
}

void app_vrx_poll(void)
{
    static uint32_t last_frequency_ms = 0u;
    static uint32_t last_status_ms = 0u;

    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - last_frequency_ms) >= 2000u)
    {
        (void)vrx_request_frequency(
            vrx_uart_write,
            &huart1);

        last_frequency_ms = now;
    }

    if ((uint32_t)(now - last_status_ms) >= 200u)
    {
        (void)vrx_request_status(
            vrx_uart_write,
            &huart1);

        last_status_ms = now;
    }
}

/*
 * Examples:
 *
 *   vrx_up(vrx_uart_write, &huart1);
 *   vrx_down(vrx_uart_write, &huart1);
 *   vrx_left(vrx_uart_write, &huart1);
 *   vrx_right(vrx_uart_write, &huart1);
 *   vrx_enter(vrx_uart_write, &huart1);
 *   vrx_pair(vrx_uart_write, &huart1);
 *   vrx_record(vrx_uart_write, &huart1);
 *   vrx_back(vrx_uart_write, &huart1);
 *   vrx_force720(vrx_uart_write, &huart1);
 *   vrx_debug(vrx_uart_write, &huart1);
 *
 * Channel example:
 *
 *   vrx_set_channel(
 *       vrx_uart_write,
 *       &huart1,
 *       1,  // Band B
 *       3,  // raw channel 3 = displayed CH4
 *       0   // Hop off
 *   );
 */

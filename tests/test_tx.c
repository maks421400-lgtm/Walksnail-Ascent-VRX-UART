#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "vrx_protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned calls;
static unsigned char frame[128];
static size_t frame_len;
static int capture(const uint8_t *data, size_t length, void *user)
{
    (void)user;
    assert(length <= sizeof(frame));
    memcpy(frame, data, length);
    frame_len = length;
    ++calls;
    return 0;
}
static void expect(const char *hex)
{
    unsigned byte;
    size_t i = 0;
    int consumed;
    while (sscanf(hex, "%x%n", &byte, &consumed) == 1) {
        assert(i < frame_len && frame[i++] == byte);
        hex += consumed;
    }
    assert(i == frame_len);
}
typedef VrxResult (*KeyFn)(VrxWriteFn, void *);
int main(void)
{
    /* Literal packet vectors from the Chinese firmware-package document. */
    static const struct { KeyFn fn; const char *hex; } keys[] = {
        {vrx_up, "FE EF 22 00 04 40 00 00 00 00 40 0D 0A"},
        {vrx_down, "FE EF 22 00 04 40 01 00 00 00 41 0D 0A"},
        {vrx_left, "FE EF 22 00 04 40 02 00 00 00 42 0D 0A"},
        {vrx_right, "FE EF 22 00 04 40 03 00 00 00 43 0D 0A"},
        {vrx_enter, "FE EF 22 00 04 40 04 00 00 00 44 0D 0A"},
        {vrx_pair, "FE EF 22 00 04 40 05 00 00 00 45 0D 0A"},
        {vrx_upgrade, "FE EF 22 00 04 40 05 01 00 00 46 0D 0A"},
        {vrx_record, "FE EF 22 00 04 40 06 00 00 00 46 0D 0A"},
        {vrx_back, "FE EF 22 00 04 40 07 00 00 00 47 0D 0A"},
        {vrx_force720, "FE EF 22 00 04 40 07 01 00 00 48 0D 0A"},
        {vrx_debug, "FE EF 22 00 04 40 09 00 00 00 49 0D 0A"}
    };
    static const struct { uint8_t band, channel, hop; const char *hex; } channels[] = {
        {0,0,0,"FE EF 22 00 08 50 00 00 00 00 00 00 00 00 50 0D 0A"},
        {0,0,1,"FE EF 22 00 08 50 00 00 01 00 00 00 00 00 51 0D 0A"},
        {1,0,1,"FE EF 22 00 08 50 01 00 01 00 00 00 00 00 52 0D 0A"},
        {2,0,1,"FE EF 22 00 08 50 02 00 01 00 00 00 00 00 53 0D 0A"},
        {0,0,0,"FE EF 22 00 08 50 00 00 00 00 00 00 00 00 50 0D 0A"},
        {1,12,0,"FE EF 22 00 08 50 01 0C 00 00 00 00 00 00 5D 0D 0A"},
        {2,2,0,"FE EF 22 00 08 50 02 02 00 00 00 00 00 00 54 0D 0A"}
    };
    size_t i;
    unsigned v, before;
    for(i=0;i<sizeof(keys)/sizeof(keys[0]);++i) {
        before=calls;
        assert(keys[i].fn(capture,NULL)==VRX_OK);
        assert(calls==before+1);
        expect(keys[i].hex);
    }
    for(i=0;i<sizeof(channels)/sizeof(channels[0]);++i) {
        assert(vrx_set_channel(capture,NULL,channels[i].band,channels[i].channel,channels[i].hop)==VRX_OK);
        expect(channels[i].hex);
    }
    assert(vrx_request_frequency(capture,NULL)==VRX_OK);
    expect("FE EF A2 00 08 51 00 00 00 00 00 00 00 00 51 0D 0A");
    assert(vrx_request_status(capture,NULL)==VRX_OK);
    expect("FE EF A2 00 08 52 00 00 00 00 00 00 00 00 52 0D 0A");
    assert(vrx_set_channel(capture,NULL,2,16,1)==VRX_OK);
    expect("FE EF 22 00 08 50 02 10 01 00 00 00 00 00 63 0D 0A");
    before=calls;
    for(v=0;v<256;++v)
        assert(vrx_send_key(capture,NULL,8,(uint8_t)v)==VRX_ERROR_ARGUMENT);
    for(v=3;v<256;++v)
        assert(vrx_set_channel(capture,NULL,(uint8_t)v,0,0)==VRX_ERROR_ARGUMENT);
    for(v=17;v<256;++v)
        assert(vrx_set_channel(capture,NULL,0,(uint8_t)v,0)==VRX_ERROR_ARGUMENT);
    for(v=2;v<256;++v)
        assert(vrx_set_channel(capture,NULL,0,0,(uint8_t)v)==VRX_ERROR_ARGUMENT);
    assert(calls==before);
    assert(vrx_upgrade(NULL,NULL)==VRX_ERROR_ARGUMENT);
    puts("PASS: 20 source packets, channel boundary, invalid parameters produce no writes");
    return 0;
}

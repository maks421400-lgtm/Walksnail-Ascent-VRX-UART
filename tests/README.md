# TX protocol tests

The tests compare emitted packets with all 20 complete examples in the Chinese serial-command document supplied with the firmware package. They also check the channel-index boundary and verify that invalid key 0x08 and out-of-range Band/Channel/Hop values produce no UART writes.

Run from the repository root with a C compiler (assertions must remain enabled):

```sh
cc -std=c99 -Wall -Wextra -Werror -Isrc src/vrx_protocol.c tests/test_tx.c -o test_tx
./test_tx
```

Or from an MSVC Developer Command Prompt:

```bat
cl /nologo /W4 /WX /TC /Isrc src\vrx_protocol.c tests\test_tx.c /Fe:test_tx.exe
test_tx.exe
```

These tests verify TX encoding and argument handling, not device behavior or the unresolved RX telemetry layout.

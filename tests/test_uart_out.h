#include "../midi_uart.h"
#include "test.h"

static uint8_t uart_out_busy = 0;
static uint8_t uart_out_last_byte;

static void uartOutSendByte(uint8_t b)
{
    uart_out_last_byte = b;
    uart_out_busy = 1;
}
static void uartOutStopSend()
{
    uart_out_busy = 0;
}
static MidiRet uartOutIsBusy()
{
    return 0 == uart_out_busy ? MIDI_RET_OK : MIDI_RET_FAIL;
}

static void testUartOut()
{
    TEST_NEW_BLOCK("uart out");

    MidiOutPortContextT test_port_context;
    midiPortInit(&test_port_context);
    MidiOutUartContextT test_uart_context;
    MidiOutUartPortT test_uart_port = {
        .port = &test_port_context,
        .context = &test_uart_context,
        .sendByte = uartOutSendByte,
        .stopSend = uartOutStopSend,
        .isBusy = uartOutIsBusy
    };
    midiOutUartInit(&test_uart_port);

    //////////////////////////////////////////////////////////////////
    // basics
    MidiMessageT stream1[] = {
        { .cin = MIDI_CIN_NOTEOFF, .miditype = MIDI_CIN_NOTEOFF, .byte2 = 0x10, .byte3 = 0x20 },
        { .cin = MIDI_CIN_NOTEOFF, .miditype = MIDI_CIN_NOTEOFF, .byte2 = 0x11, .byte3 = 0x21 },
        { .cin = MIDI_CIN_NOTEON, .miditype = MIDI_CIN_NOTEON, .byte2 = 0x12, .byte3 = 0x22 },
        { .cin = MIDI_CIN_NOTEON, .miditype = MIDI_CIN_NOTEON, .byte2 = 0x13, .byte3 = 0x23 },
        { .cin = MIDI_CIN_NOTEOFF, .miditype = MIDI_CIN_NOTEOFF, .byte2 = 0x14, .byte3 = 0x24 },
        { .cin = MIDI_CIN_POLYKEYPRESS, .miditype = MIDI_CIN_POLYKEYPRESS, .byte2 = 0x15, .byte3 = 0x25 },
        { .cin = MIDI_CIN_POLYKEYPRESS, .miditype = MIDI_CIN_POLYKEYPRESS, .byte2 = 0x16, .byte3 = 0x26 },
        { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 0x17, .byte3 = 0x27 },
        { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 0x18, .byte3 = 0x28 },
        { .cin = MIDI_CIN_PROGRAMCHANGE, .miditype = MIDI_CIN_PROGRAMCHANGE, .byte2 = 0x19, .byte3 = 0x29 },
        { .cin = MIDI_CIN_PROGRAMCHANGE, .miditype = MIDI_CIN_PROGRAMCHANGE, .byte2 = 0x1A, .byte3 = 0x2A },
        { .cin = MIDI_CIN_CHANNELPRESSURE, .miditype = MIDI_CIN_CHANNELPRESSURE, .byte2 = 0x1B, .byte3 = 0x2B },
        { .cin = MIDI_CIN_CHANNELPRESSURE, .miditype = MIDI_CIN_CHANNELPRESSURE, .byte2 = 0x1C, .byte3 = 0x2C },
        { .cin = MIDI_CIN_PITCHBEND, .miditype = MIDI_CIN_PITCHBEND, .byte2 = 0x1E, .byte3 = 0x2E },
        { .cin = MIDI_CIN_PITCHBEND, .miditype = MIDI_CIN_PITCHBEND, .byte2 = 0x1F, .byte3 = 0x2F },
    };
    uint8_t stream1bytes[] = {
        // 0x80, 0x10, 0x20, 0x11, 0x21, 0x90, 0x12, 0x22, 0x13, 0x23, 0x80, 0x14, 0x24, // NOTEOFF
        0x90, 0x10, 0x00, 0x11, 0x00, 0x12, 0x22, 0x13, 0x23, 0x14, 0x00, // NOTEON with 0-velocity
        0xA0, 0x15, 0x25, 0x16, 0x26, 0xB0, 0x17, 0x27, 0x18, 0x28,
        0xC0, 0x19, 0x1A, 0xD0, 0x1B, 0x1C, 0xE0, 0x1E, 0x2E, 0x1F, 0x2F
    };
    for (int i = 0; i < sizeof(stream1) / sizeof(MidiMessageT); i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWriteRaw(&test_port_context, stream1[i]), i);
    }
    for (int i = 0; i < sizeof(stream1bytes); i++) {
        midiOutUartTranmissionCompleteCallback(&test_uart_port);
        TEST_ASSERT_int(stream1bytes[i] == uart_out_last_byte, i);
        // printf("debug: %02X %02X" ENDLINE, i, uart_out_last_byte);
        TEST_ASSERT_int(uart_out_busy, i);
    }
    midiOutUartTranmissionCompleteCallback(&test_uart_port);
    TEST_ASSERT(0 == uart_out_busy);

    //////////////////////////////////////////////////////////////////
    // basics system
    test_clock += (MIDI_CLOCK_RATE * 500 / 1000); // MIDI_RUNNINGSTATUS_HOLD
    MidiMessageT stream2[] = {
        { .cin = MIDI_CIN_PITCHBEND, .miditype = MIDI_CIN_PITCHBEND, .byte2 = 0x1E, .byte3 = 0x2E },
        { .cin = MIDI_CIN_2BYTESYSTEMCOMMON, .byte1 = 0xF1, .byte2 = 0x10, .byte3 = 0x20 },
        { .cin = MIDI_CIN_PITCHBEND, .miditype = MIDI_CIN_PITCHBEND, .byte2 = 0x1F, .byte3 = 0x2F },
        { .cin = MIDI_CIN_2BYTESYSTEMCOMMON, .byte1 = 0xF1, .byte2 = 0x11, .byte3 = 0x21 },
        { .cin = MIDI_CIN_3BYTESYSTEMCOMMON, .byte1 = 0xF2, .byte2 = 0x12, .byte3 = 0x22 },
        { .cin = MIDI_CIN_3BYTESYSTEMCOMMON, .byte1 = 0xF2, .byte2 = 0x13, .byte3 = 0x23 },
        { .cin = MIDI_CIN_PITCHBEND, .miditype = MIDI_CIN_PITCHBEND, .byte2 = 0x1E, .byte3 = 0x2E },
        { .cin = MIDI_CIN_SINGLEBYTE, .byte1 = 0xFA, .byte2 = 0x14, .byte3 = 0x24 },
        { .cin = MIDI_CIN_SINGLEBYTE, .byte1 = 0xFB, .byte2 = 0x15, .byte3 = 0x25 },
        { .cin = MIDI_CIN_PITCHBEND, .miditype = MIDI_CIN_PITCHBEND, .byte2 = 0x1F, .byte3 = 0x2F },
        { .cin = MIDI_CIN_SYSEX3BYTES, .byte1 = 0xF0, .byte2 = 0x16, .byte3 = 0x26 },
        { .cin = MIDI_CIN_SYSEX3BYTES, .byte1 = 0x07, .byte2 = 0x17, .byte3 = 0x27 },
        { .cin = MIDI_CIN_SYSEXEND1, .byte1 = 0xF7, .byte2 = 0x18, .byte3 = 0x28 },
        { .cin = MIDI_CIN_PITCHBEND, .miditype = MIDI_CIN_PITCHBEND, .byte2 = 0x1E, .byte3 = 0x2E },
        { .cin = MIDI_CIN_SYSEXEND1, .byte1 = 0xF7, .byte2 = 0x19, .byte3 = 0x29 },
        { .cin = MIDI_CIN_PITCHBEND, .miditype = MIDI_CIN_PITCHBEND, .byte2 = 0x1F, .byte3 = 0x2F },
        { .cin = MIDI_CIN_SYSEXEND2, .byte1 = 0x0A, .byte2 = 0xF7, .byte3 = 0x2A },
        { .cin = MIDI_CIN_SYSEXEND2, .byte1 = 0x0B, .byte2 = 0xF7, .byte3 = 0x2B },
        { .cin = MIDI_CIN_SYSEXEND3, .byte1 = 0x0C, .byte2 = 0x1C, .byte3 = 0xF7 },
        { .cin = MIDI_CIN_SYSEXEND3, .byte1 = 0x0D, .byte2 = 0x1D, .byte3 = 0xF7 },
    };
    uint8_t stream2bytes[] = {
        0xE0, 0x1E, 0x2E, 0xF1, 0x10, 0xE0, 0x1F, 0x2F,
        0xF1, 0x11, 0xF2, 0x12, 0x22, 0xF2, 0x13, 0x23,
        0xE0, 0x1E, 0x2E, 0xFA, 0xFB, /* 0xE0, */ 0x1F, 0x2F,
        0xF0, 0x16, 0x26, 0x07, 0x17, 0x27, 0xF7, 
        0xE0, 0x1E, 0x2E, 0xF7, 0xE0, 0x1F, 0x2F,
        0x0A, 0xF7, 0x0B, 0xF7,
        0x0C, 0x1C, 0xF7, 0x0D, 0x1D, 0xF7
    };

    for (int i = 0; i < sizeof(stream2) / sizeof(MidiMessageT); i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWriteRaw(&test_port_context, stream2[i]), i);
    }
    for (int i = 0; i < sizeof(stream2bytes); i++) {
        midiOutUartTranmissionCompleteCallback(&test_uart_port);
        TEST_ASSERT_int(stream2bytes[i] == uart_out_last_byte, i);
        // printf("debug: %02X %02X" ENDLINE, i, uart_out_last_byte);
        TEST_ASSERT_int(uart_out_busy, i);
    }
    midiOutUartTranmissionCompleteCallback(&test_uart_port);
    TEST_ASSERT(0 == uart_out_busy);

    // printf("debug: %02X" ENDLINE, uart_out_last_byte);
    // midiOutUartTranmissionCompleteCallback(&test_uart_port);
    // printf("debug: %02X" ENDLINE, uart_out_last_byte);
    // midiOutUartTranmissionCompleteCallback(&test_uart_port);
    // printf("debug: %02X" ENDLINE, uart_out_last_byte);
    // midiOutUartTranmissionCompleteCallback(&test_uart_port);
    // printf("debug: %02X" ENDLINE, uart_out_last_byte);
    // midiOutUartTranmissionCompleteCallback(&test_uart_port);
    // printf("debug: %02X" ENDLINE, uart_out_last_byte);
    // midiOutUartTranmissionCompleteCallback(&test_uart_port);
    // printf("debug: %02X" ENDLINE, uart_out_last_byte);
    // midiOutUartTranmissionCompleteCallback(&test_uart_port);
    // printf("debug: %02X" ENDLINE, uart_out_last_byte);
    // midiOutUartTranmissionCompleteCallback(&test_uart_port);
    // printf("debug: %02X" ENDLINE, uart_out_last_byte);
    // midiOutUartTranmissionCompleteCallback(&test_uart_port);
    // printf("debug: %02X" ENDLINE, uart_out_last_byte);
    // midiOutUartTranmissionCompleteCallback(&test_uart_port);
    // printf("debug: %02X" ENDLINE, uart_out_last_byte);

    // printf("debug: %08X - %08X" ENDLINE, mtr.timestamp, mtr.mes.full_word);
    // printf("debug: %08X" ENDLINE, mr.full_word);
    // MIDI_CIN_RESERVED1 = 0, //     0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.
    // MIDI_CIN_RESERVED2, //         0x1 1, 2 or 3 Cable events. Reserved for future expansion.
    // MIDI_CIN_2BYTESYSTEMCOMMON, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.
    // MIDI_CIN_3BYTESYSTEMCOMMON, // 0x3 3 Three-byte System Common messages like SPP, etc.
    // MIDI_CIN_SYSEX3BYTES, //       0x4 3 SysEx starts or continues
    // MIDI_CIN_SYSEXEND1, //         0x5 1 Single-byte System Common Message or SysEx ends with following single byte.
    // MIDI_CIN_SYSEXEND2, //         0x6 2 SysEx ends with following two bytes.
    // MIDI_CIN_SYSEXEND3, //         0x7 3 SysEx ends with following three bytes.
    // MIDI_CIN_NOTEOFF, //           0x8 3 Note-off
    // MIDI_CIN_NOTEON, //            0x9 3 Note-on
    // MIDI_CIN_POLYKEYPRESS, //      0xA 3 Poly-KeyPress
    // MIDI_CIN_CONTROLCHANGE, //     0xB 3 Control Change
    // MIDI_CIN_PROGRAMCHANGE, //     0xC 2 Program Change
    // MIDI_CIN_CHANNELPRESSURE, //   0xD 2 Channel Pressure
    // MIDI_CIN_PITCHBEND, //         0xE 3 PitchBend Change
    // MIDI_CIN_SINGLEBYTE //         0xF 1 Single Byte
    TEST_TODO("rs timer");
    TEST_TODO("as timer, tap");
}

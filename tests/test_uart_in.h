#include "../midi_uart.h"
#include "test.h"

static void uartSequenceReceive(const uint8_t* seq, const uint32_t len, const uint32_t ts_delta, MidiInUartContextT* const cx)
{
    for (uint32_t i = 0; i < len; i++) {
        test_clock += ts_delta;
        midiInUartByteReceiveCallback(seq[i], cx, 5);
    }
}

static void testUartIn()
{
    TEST_NEW_BLOCK("uart in");
    MidiMessageT mr;
    MidiTsMessageT mtr;
    MidiInUartContextT test_uart_in_context;
    midiInUartInit(&test_uart_in_context);
    midiInit(); // syx unlock

    //////////////////////////////////////////////////////////////////
    // basics
    const uint8_t tseq01[] = {
        0x21, 0x22, 0x23, // 03 --
        0x90, 0x23, 0x34, // 06 0x34239059
        0x35, 0x26, //       08 0x26359059
        0x38, 0x00, //       10 0x00388058
        0x35, 0x33, //       12 0x33359059
        0x80, 0x11, 0x12, // 15 0x12118058
        0x13, 0x00, //       17 0x00138058
        0x15, 0x16, //       19 0x16158058
        0x81, 0x44, 0x30, // 22 0x30448158
        0x45, 0x46, //       24 0x46458158
        0xA2, 0x16, 0x17, // 27 0x1716A25A
        0x18, 0x19, //       29 0x1918A25A
        0x1A, 0x00, //       31 0x001AA25A
        0xB3, 0x2A, 0x2B, // 34 0x2B2AB35B
        0x1E, 0x00, //       36 0x001EB35B
        0x2D, 0x2F, //       38 0x2F2DB35B
        0xCC, 0x2C, //       40 0x002CCC5C ts+1
        0x00, //             41 0x0000CC5C ts+1
        0x2D, //             42 0x002DCC5C ts+1
        0xDA, 0x49, //       44 0x0049DA5D ts+1
        0x48, //             45 0x0048DA5D ts+1
        0x00, //             46 0x0000DA5D ts+1
        0x47, //             47 0x0047DA5D ts+1
        0xEF, 0x78, 0x77, // 50 0x7778EF5E
        0x00, 0x00, //       52 0x0000EF5E
        0x76, 0x75, //       54 0x7576EF5E
        0xF4 // terminate
    };
    const uint32_t tseq01_len = sizeof(tseq01);
    const uint32_t tseq01_mres[] = {
        0x34239059, 0x26359059, 0x00388058, 0x33359059, 0x12118058, 0x00138058, 0x16158058,
        0x30448158, 0x46458158, 0x1716A25A, 0x1918A25A, 0x001AA25A, 0x2B2AB35B, 0x001EB35B,
        0x2F2DB35B, 0x002CCC5C, 0x0000CC5C, 0x002DCC5C, 0x0049DA5D, 0x0048DA5D, 0x0000DA5D,
        0x0047DA5D, 0x7778EF5E, 0x0000EF5E, 0x7576EF5E
    };
    const uint8_t tseq01_ts[] = {
        6, 8, 10, 12, 15, 17, 19, 22, 24, 27, 29, 31, 34, 36, 38, 41, 42, 43, 45, 46, 47, 48, 50, 52, 54
    };
    const uint32_t tseq01_mres_len = sizeof(tseq01_ts);
    TEST_ASSERT(tseq01_mres_len == sizeof(tseq01_mres) / 4);

    test_clock = 0x22;
    uartSequenceReceive(tseq01, tseq01_len, 1, &test_uart_in_context);
    for (int i = 0; i < tseq01_mres_len; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiRead(&mtr), i);
        TEST_ASSERT_int(0x21 + tseq01_ts[i] == mtr.timestamp, i);
        TEST_ASSERT_int(tseq01_mres[i] == mtr.mes.full_word, i);
    }
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));

    //////////////////////////////////////////////////////////////////
    // system
    const uint8_t tseq02[] = {
        0x21, 0x22, 0x23, // --
        0x90, 0x23, 0x34, // 05 0x34239059
        0x35, 0x26, 0x38, // 07 0x26359059
        0xF0, 0x11, 0x22, // 10 0x2211F054 - syx
        0x33, 0x44, 0x55, // 13 0x55443354 - syx
        0x66, 0xF7, //       16 0x00F76656 - syx
        0xF1, 0x23, 0x24, // 18 0x0023F152
        0x25, 0x26, //       21 -
        0xF0, 0x32, 0x33, // 23 0x3332F054 - syx
        0x34, 0x35, //       26 0xF7352457 - syx
        0xF2, 0x45, 0x46, // 28 0x4645F253
        0x47, 0x48, //       31 -
        0xF3, 0x66, 0x67, // 33 0x0066F352
        0x68, 0x69, //       36 -
        0xF4, 0x11, 0x12, // 38 -
        0xF5, 0x11, 0x12, // 41 -
        0xF6, 0x11, 0x12, // 44 0x0000F65F
        0xF0, 0x69, //       47 0xF769F057 - syx

        0x90, 0xF8, 0x44, 0x45, // 49 50 0x0000F85F 0x45449059
        0x90, 0x42, 0xF8, 0x51, // 53 55 0x0000F85F 0x51429059
        0x90, 0x41, 0xF9, 0x50, // 57 59 0x50419059
        0x90, 0x40, 0xFA, 0x4F, // 61 63 0x0000FA5F 0x4F409059
        0x90, 0x3F, 0xFB, 0x4E, // 65 67 0x0000FB5F 0x4E3F9059
        0x90, 0x3E, 0xFC, 0x4D, // 69 71 0x0000FC5F 0x4D3E9059
        0x90, 0x3D, 0xFD, 0x4C, // 73 75 0x4C3D9059
        0x90, 0x3C, 0xFE, 0x4B, // 77 79 0x4B3C9059 - AS
        0x90, 0x3B, 0xFF, 0x4A, // 81 83 0x0000FF5F 0x4A3B9059
        0xF0, 0x3A, 0xF8, 0xF7, // 85 87 0x0000F85F 0xF73AF057 - syx

    };
    const uint8_t tseq02_ts[] = {
        5, 7, 18, 28, 33, 44, 50, 51, 55, 54, 58, 63, 62, 67, 66, 71, 70, 74, 78, 83, 82, 87
    };
    const uint32_t tseq02_len = sizeof(tseq02);
    const uint32_t tseq02_mres[] = {
        0x34239059, 0x26359059, 0x0023F152, 0x4645F253, 0x0066F352, 0x0000F65F,
        0x0000F85F, 0x45449059, 0x0000F85F, 0x51429059, 0x50419059, 0x0000FA5F,
        0x4F409059, 0x0000FB5F, 0x4E3F9059, 0x0000FC5F, 0x4D3E9059, 0x4C3D9059,
        0x4B3C9059, 0x0000FF5F, 0x4A3B9059, 0x0000F85F
    };
    const uint32_t tseq02_mres_syx[] = {
        0x2211F054, 0x55443354, 0x00F76656, 0x3332F054, 0xF7353457, 0xF769F057,
        0xF73AF057
    };
    const uint32_t tseq02_mres_len = sizeof(tseq02_mres) / 4;
    TEST_ASSERT(sizeof(tseq02_ts) == sizeof(tseq02_mres) / 4);

    test_clock = 0x22;
    uartSequenceReceive(tseq02, tseq02_len, 1, &test_uart_in_context);
    for (int i = 0; i < tseq02_mres_len; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiRead(&mtr), i);
        // printf("debug: %d - %08X" ENDLINE, mtr.timestamp - 0x22, mtr.timestamp);
        TEST_ASSERT_int(0x22 + tseq02_ts[i] == mtr.timestamp, i);
        TEST_ASSERT_int(tseq02_mres[i] == mtr.mes.full_word, i);
    }
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    for (int i = 0; i < sizeof(tseq02_mres_syx) / 4; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiSysexRead(&mr), i);
        TEST_ASSERT_int(tseq02_mres_syx[i] == mr.full_word, i);
    }
    TEST_ASSERT(MIDI_RET_FAIL == midiSysexRead(&mr));

    //////////////////////////////////////////////////////////////////
    // #define MIDI_RUNNINGSTATUS_RESET (MIDI_CLOCK_RATE * 5000 / 1000)
    // #define MIDI_RUNNINGSTATUS_HOLD (MIDI_CLOCK_RATE * 500 / 1000)

    // test runningstatus timer
    // it is integrated in midiInUartByteReceiveCallback
    // should ignore data after (MIDI_CLOCK_RATE * 5000 / 1000)
    // test_clock = 0x22;
    // midiInUartInit(&test_uart_in_context);
    midiInUartByteReceiveCallback(0x90, &test_uart_in_context, 5);
    midiInUartByteReceiveCallback(0x55, &test_uart_in_context, 5);
    midiInUartByteReceiveCallback(0x56, &test_uart_in_context, 5);
    test_clock += (MIDI_CLOCK_RATE * 5000 / 1000) - 1;
    midiInUartByteReceiveCallback(0x57, &test_uart_in_context, 5);
    midiInUartByteReceiveCallback(0x58, &test_uart_in_context, 5);
    test_clock += 1;
    midiInUartByteReceiveCallback(0x59, &test_uart_in_context, 5);
    midiInUartByteReceiveCallback(0x5A, &test_uart_in_context, 5);
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mtr));
    TEST_ASSERT(mtr.mes.full_word == 0x56559059);
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mtr));
    TEST_ASSERT(mtr.mes.full_word == 0x58579059);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    // and the same with 2 byte messages
    midiInUartByteReceiveCallback(0xD0, &test_uart_in_context, 5);
    midiInUartByteReceiveCallback(0x25, &test_uart_in_context, 5);
    test_clock += (MIDI_CLOCK_RATE * 5000 / 1000) - 1;
    midiInUartByteReceiveCallback(0x26, &test_uart_in_context, 5);
    test_clock += 1;
    // test_clock = ~test_clock; // TODO: overflow is tolerable, i think
    // test_clock = test_clock + 1;
    midiInUartByteReceiveCallback(0x27, &test_uart_in_context, 5);
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mtr));
    TEST_ASSERT(mtr.mes.full_word == 0x0025D05D);
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mtr));
    TEST_ASSERT(mtr.mes.full_word == 0x0026D05D);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));

    // #define MIDI_ACTIVESENSE_RESET (MIDI_CLOCK_RATE * 330 / 1000)
    // #define MIDI_ACTIVESENSE_SEND (MIDI_CLOCK_RATE * 270 / 1000)
    // tap is useed for active sense reset - should send ANO
    // inactive on init
    midiInUartInit(&test_uart_in_context);
    midiInUartTap(&test_uart_in_context, 8);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    // printf("read: %08X" ENDLINE, mtr.mes.full_word);
    test_clock += (MIDI_CLOCK_RATE * 330 / 1000);
    midiInUartTap(&test_uart_in_context, 8);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    test_clock += 1;
    midiInUartTap(&test_uart_in_context, 8);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    // restore on receive 0xFE, no pass thru for AS
    midiInUartByteReceiveCallback(0xFE, &test_uart_in_context, 5);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    midiInUartTap(&test_uart_in_context, 8);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    test_clock += (MIDI_CLOCK_RATE * 330 / 1000);
    midiInUartTap(&test_uart_in_context, 8);
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(MIDI_RET_OK == midiRead(&mtr));
        TEST_ASSERT(mtr.mes.full_word == (0x007BB08B | (i << 8)));
    }
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    test_clock += 1;
    midiInUartTap(&test_uart_in_context, 8);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    // but what if tap is too slow
    midiInUartByteReceiveCallback(0xFE, &test_uart_in_context, 5);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    midiInUartTap(&test_uart_in_context, 8);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    test_clock += (MIDI_CLOCK_RATE * 330 / 1000);
    test_clock += (MIDI_CLOCK_RATE * 330 / 1000);
    test_clock += (MIDI_CLOCK_RATE * 330 / 1000);
    midiInUartTap(&test_uart_in_context, 8);
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(MIDI_RET_OK == midiRead(&mtr));
        TEST_ASSERT(mtr.mes.full_word == (0x007BB08B | (i << 8)));
    }
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));

    // maintaining AS by other messages
    midiInUartByteReceiveCallback(0xFE, &test_uart_in_context, 5);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    midiInUartTap(&test_uart_in_context, 8);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));

    for (int i = 0; i < 88; i++) {
        test_clock += (MIDI_CLOCK_RATE * 330 / 1000) - 1;
        midiInUartTap(&test_uart_in_context, 8);
        TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
        midiInUartByteReceiveCallback(0xFF, &test_uart_in_context, 5);
        TEST_ASSERT(MIDI_RET_OK == midiRead(&mtr));
        TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    }
    test_clock += (MIDI_CLOCK_RATE * 330 / 1000);
    midiInUartTap(&test_uart_in_context, 8);
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(MIDI_RET_OK == midiRead(&mtr));
        TEST_ASSERT(mtr.mes.full_word == (0x007BB08B | (i << 8)));
    }
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));

    //////////////////////////////////////////////////////////////////
    // integration: sysex lock
    MidiInUartContextT another_input;
    midiInUartInit(&another_input);
    #define CN1 3
    #define CN2 7
    midiInUartByteReceiveCallback(0xF0, &test_uart_in_context, CN1);
    midiInUartByteReceiveCallback(0x10, &test_uart_in_context, CN1);
    midiInUartByteReceiveCallback(0x11, &test_uart_in_context, CN1);
    midiInUartByteReceiveCallback(0x12, &test_uart_in_context, CN1);
    
    midiInUartByteReceiveCallback(0xF0, &another_input, CN2);
    midiInUartByteReceiveCallback(0x20, &another_input, CN2);
    midiInUartByteReceiveCallback(0x21, &another_input, CN2);
    midiInUartByteReceiveCallback(0x22, &another_input, CN2);

    midiInUartByteReceiveCallback(0x13, &test_uart_in_context, CN1);
    midiInUartByteReceiveCallback(0x14, &test_uart_in_context, CN1);
    midiInUartByteReceiveCallback(0x15, &test_uart_in_context, CN1);
    midiInUartByteReceiveCallback(0xF7, &test_uart_in_context, CN1);

    midiInUartByteReceiveCallback(0x23, &another_input, CN2);
    midiInUartByteReceiveCallback(0x24, &another_input, CN2);
    midiInUartByteReceiveCallback(0x25, &another_input, CN2);
    midiInUartByteReceiveCallback(0xF7, &another_input, CN2);

    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mtr));
    // F0 10 11 - 12 13 14 - 15 F7
    TEST_ASSERT(MIDI_RET_OK == midiSysexRead(&mr));
    TEST_ASSERT(mr.full_word == 0x1110F034);
    TEST_ASSERT(MIDI_RET_OK == midiSysexRead(&mr));
    TEST_ASSERT(mr.full_word == 0x14131234);
    TEST_ASSERT(MIDI_RET_OK == midiSysexRead(&mr));
    TEST_ASSERT(mr.full_word == 0x00F71536);
    // printf("read: %08X" ENDLINE, mr.full_word);
    TEST_ASSERT(MIDI_RET_FAIL == midiSysexRead(&mr));

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

    // TEST_TODO("msg decode");
    // TEST_TODO("realtime interruption");
    // TEST_TODO("sysex interruption?");
    // TEST_TODO("system decode, rs");
}

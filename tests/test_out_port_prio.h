#include "../midi_output.h"
#include "test.h"

__attribute__((unused)) static void printBuffer(MidiOutPortContextT* cx)
{
    for (int i = 0; i < MIDI_TX_BUFFER_SIZE; i++) {
        if (i == cx->buf_rp) {
            if (i == cx->buf_wp)
                printf(ANSI_COLOR_CYAN);
            else
                printf(ANSI_COLOR_GREEN);
        } else if (i == cx->buf_wp) {
            printf(ANSI_COLOR_RED);
        } else if (((i - cx->buf_rp) & (MIDI_TX_BUFFER_SIZE - 1)) <= ((cx->buf_wp - cx->buf_rp) & (MIDI_TX_BUFFER_SIZE - 1))) {
            printf(ANSI_COLOR_YELLOW);
        } else {
            printf(ANSI_COLOR_RESET);
        }
        printf("%02X ", cx->buf[i].byte2);
    }
    printf(ANSI_COLOR_RESET ENDLINE);
}

// #define PBUF(cx) printBuffer(&cx)
#define PBUF(...) ;

// rp 8, wp 9, i = 11

static void outPortTests_prio()
{
    MidiOutPortContextT test_port_context;

    TEST_NEW_BLOCK("out port prio");

    // THIS IS COPY FROM THE GUTS. BE CAREFUL!
    uint16_t m_cin_priorities[16] = {
        // values must be from 0 (filtered out) to (buffer size - 2)
        (MIDI_TX_BUFFER_SIZE - 2) * 0 / 1, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.        | not used at all
        (MIDI_TX_BUFFER_SIZE - 2) * 0 / 1, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.                         | not used at all
        (MIDI_TX_BUFFER_SIZE - 2) * 1 / 1, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.                   | 100%
        (MIDI_TX_BUFFER_SIZE - 2) * 1 / 1, // 0x3 3 Three-byte System Common messages like SPP, etc.                             | 100%
        (MIDI_TX_BUFFER_SIZE - 2) * 0 / 1, // 0x4 3 SysEx starts or continues                                                    | --
        (MIDI_TX_BUFFER_SIZE - 2) * 0 / 1, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.  | --
        (MIDI_TX_BUFFER_SIZE - 2) * 0 / 1, // 0x6 2 SysEx ends with following two bytes.                                         | --
        (MIDI_TX_BUFFER_SIZE - 2) * 0 / 1, // 0x7 3 SysEx ends with following three bytes.                                       | --
        (MIDI_TX_BUFFER_SIZE - 2) * 7 / 8, // 0x8 3 Note-off                                                                     | 87%
        (MIDI_TX_BUFFER_SIZE - 2) * 5 / 8, // 0x9 3 Note-on                                                                      | 62% - it must be noticable
        (MIDI_TX_BUFFER_SIZE - 2) * 1 / 2, // 0xA 3 Poly-KeyPress                                                                | 50%
        (MIDI_TX_BUFFER_SIZE - 2) * 7 / 8, // 0xB 3 Control Change                                                               | 87% - NOT USED by defaul
        (MIDI_TX_BUFFER_SIZE - 2) * 2 / 3, // 0xC 2 Program Change                                                               | 67%
        (MIDI_TX_BUFFER_SIZE - 2) * 5 / 9, // 0xD 2 Channel Pressure                                                             | 55%
        (MIDI_TX_BUFFER_SIZE - 2) * 4 / 7, // 0xE 3 PitchBend Change                                                             | 57%
        (MIDI_TX_BUFFER_SIZE - 2) * 1 / 1, // 0xF 1 Single Byte                                                                  | 100% always active
    };
    uint16_t m_cc_priorities[3] = {
        // values must be from 0 (filtered out) to (buffer size - 2)
        (MIDI_TX_BUFFER_SIZE - 2) * 5 / 8, // 62% MSB
        (MIDI_TX_BUFFER_SIZE - 2) * 4 / 7, // 57% regular
        (MIDI_TX_BUFFER_SIZE - 2) * 2 / 3, // 67% special
    };
    printf("prio 2BYTESYSTEMCOMMON: %d" ENDLINE, m_cin_priorities[MIDI_CIN_2BYTESYSTEMCOMMON]);
    printf("prio 3BYTESYSTEMCOMMON: %d" ENDLINE, m_cin_priorities[MIDI_CIN_3BYTESYSTEMCOMMON]);
    printf("prio SINGLEBYTE       : %d" ENDLINE, m_cin_priorities[MIDI_CIN_SINGLEBYTE]);
    printf("prio NOTEOFF          : %d" ENDLINE, m_cin_priorities[MIDI_CIN_NOTEOFF]);
    printf("prio PROGRAMCHANGE    : %d" ENDLINE, m_cin_priorities[MIDI_CIN_PROGRAMCHANGE]);
    printf("prio NOTEON           : %d" ENDLINE, m_cin_priorities[MIDI_CIN_NOTEON]);
    printf("prio PITCHBEND        : %d" ENDLINE, m_cin_priorities[MIDI_CIN_PITCHBEND]);
    printf("prio CHANNELPRESSURE  : %d" ENDLINE, m_cin_priorities[MIDI_CIN_CHANNELPRESSURE]);
    printf("prio POLYKEYPRESS     : %d" ENDLINE, m_cin_priorities[MIDI_CIN_POLYKEYPRESS]);

    printf("prio CONTROLCHANGE    : %d" ENDLINE, m_cin_priorities[MIDI_CIN_CONTROLCHANGE]);
    printf("prio CC MSB           : %d" ENDLINE, m_cc_priorities[0]);
    printf("prio CC regular       : %d" ENDLINE, m_cc_priorities[1]);
    printf("prio CC special       : %d" ENDLINE, m_cc_priorities[2]);

    TEST_ASSERT(MIDI_TX_BUFFER_SIZE == 32);
    TEST_ASSERT(MIDI_TX_SYSEX_BUFFER_SIZE == 16);
    MidiMessageT mm = {
        .cn = MIDI_CN_TEST0,
        .cin = MIDI_CIN_POLYKEYPRESS,
        .byte1 = 0x11,
        .byte2 = 0x22,
        .byte3 = 0x33
    };
    MidiMessageT mr;

    midiPortInit(&test_port_context);
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // test prio
    mm.cin = mm.miditype = MIDI_CIN_POLYKEYPRESS;
    for (int i = 0; i < 40; i++) {
        mm.byte2 = i;
        PBUF(test_port_context);
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    for (int i = 0; i < 40; i++) {
        PBUF(test_port_context);
        if (i < (MIDI_TX_BUFFER_SIZE - 1)) {
            TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
            TEST_ASSERT_int(mr.byte2 == i + 9, i);
        } else {
            TEST_ASSERT_int(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr), i);
        }
    }
    // printf(ENDLINE);
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    mm.cin = mm.miditype = MIDI_CIN_POLYKEYPRESS;
    for (int i = 0; i < 40; i++) {
        mm.byte2 = i;
        PBUF(test_port_context);
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    // now there should be 10..40
    // printf(ENDLINE);

    MidiMessageT stream[] = {
        { .cin = MIDI_CIN_POLYKEYPRESS, .miditype = MIDI_CIN_POLYKEYPRESS, .byte2 = 0x7f, .byte3 = 0x33 },
        { .cin = MIDI_CIN_CHANNELPRESSURE, .miditype = MIDI_CIN_CHANNELPRESSURE, .byte2 = 0x7f, .byte3 = 0x33 },
        { .cin = MIDI_CIN_PITCHBEND, .miditype = MIDI_CIN_PITCHBEND, .byte2 = 0x7f, .byte3 = 0x33 },
        { .cin = MIDI_CIN_NOTEON, .miditype = MIDI_CIN_NOTEON, .byte2 = 0x70, .byte3 = 0x33 },
        { .cin = MIDI_CIN_PROGRAMCHANGE, .miditype = MIDI_CIN_PROGRAMCHANGE, .byte2 = 0xAA, .byte3 = 0x33 },
        { .cin = MIDI_CIN_PROGRAMCHANGE, .miditype = MIDI_CIN_PROGRAMCHANGE, .midichannel = 2, .byte2 = 0xBB, .byte3 = 0x33 },
        { .cin = MIDI_CIN_NOTEOFF, .miditype = MIDI_CIN_NOTEOFF, .byte2 = 0x7f, .byte3 = 0x33 },
        { .cin = MIDI_CIN_NOTEOFF, .miditype = MIDI_CIN_NOTEOFF, .byte2 = 0x7e, .byte3 = 0x33 },
        { .cin = MIDI_CIN_NOTEOFF, .miditype = MIDI_CIN_NOTEOFF, .byte2 = 0x7d, .byte3 = 0x33 },
        { .cin = MIDI_CIN_NOTEOFF, .miditype = MIDI_CIN_NOTEOFF, .byte2 = 0x7c, .byte3 = 0x33 },
    };
    size_t len = sizeof(stream) / sizeof(stream[1]);

    for (int i = 0; i < len; i++) {
        PBUF(test_port_context);
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, stream[i]), i);
    }

    TEST_ASSERT((MIDI_TX_BUFFER_SIZE - 1) == midiPortMaxUtilisation(&test_port_context));
    TEST_ASSERT(0 == test_port_context.messages_optimized);

    for (int i = 0; i < MIDI_TX_BUFFER_SIZE - len - 1; i++) {
        PBUF(test_port_context);
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        TEST_ASSERT_int(40 - MIDI_TX_BUFFER_SIZE + len + 1 + i == mr.byte2, i);
    }
    for (int i = 0; i < len; i++) {
        PBUF(test_port_context);
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        // printf("debug: %08X %08X %d" ENDLINE, stream[i].full_word, mr.full_word, 0);
        TEST_ASSERT_int(stream[i].full_word == mr.full_word, i);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // for each of the priority check that the lowest will not overwrite it

    // dirty hack to test rt branch
    test_port_context.buf[test_port_context.buf_rp].cin = MIDI_CIN_SINGLEBYTE;
    mm.miditype = mm.cin = MIDI_CIN_SINGLEBYTE;
    mm.midichannel = 8;
    for (int i = 0; i < MIDI_TX_BUFFER_SIZE; i++) {
        PBUF(test_port_context);
        mm.byte2 = i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    for (int i = 0; i < len; i++) {
        TEST_ASSERT_int(MIDI_RET_FAIL == midiPortWrite(&test_port_context, stream[i]), i);
    }
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(1 == mr.byte2);
    // printf(ENDLINE);

    // now we have spece for only one message
    for (int i = 0; i < len; i++) {
        PBUF(test_port_context);
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, stream[i]), i);
        if (i - 1 >= 0) {
            PBUF(test_port_context);
            if (stream[i - 1].cin != stream[i].cin)
                TEST_ASSERT_int(MIDI_RET_FAIL == midiPortWrite(&test_port_context, stream[i - 1]), i);
        }
        TEST_ASSERT_int(0 == test_port_context.messages_optimized, i);
    }

    for (int i = 0; i < MIDI_TX_BUFFER_SIZE - 2; i++) {
        PBUF(test_port_context);
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        TEST_ASSERT_int(2 + i == mr.byte2, i);
    }
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(stream[len - 1].full_word == (mr.full_word & 0xFFFFFF0F));
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));
    // printf(ENDLINE);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    MidiMessageT mm_cc_damper = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 64, .byte3 = 64 };
    MidiMessageT mm_cc_modwheel_msb = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 1, .byte3 = 64 };
    MidiMessageT mm_cc_modwheel_lsb = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 0x20 + 1, .byte3 = 64 };
    MidiMessageT mm_cc_data_msb = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 6, .byte3 = 64 };
    MidiMessageT mm_cc_data_lsb = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 0x20 + 6, .byte3 = 64 };
    // MidiMessageT mm_cc_data_inc = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 96, .byte3 = 64 };
    // MidiMessageT mm_cc_data_dec = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 97, .byte3 = 64 };
    // MidiMessageT mm_cc_rpn_lsb = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 98, .byte3 = 64 };
    MidiMessageT mm_cc_rpn_msb = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 99, .byte3 = 64 };
    MidiMessageT mm_cc_nrpn_lsb = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 100, .byte3 = 64 };
    // MidiMessageT mm_cc_nrpn_msb = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 101, .byte3 = 64 };
    MidiMessageT mm_cc_aso = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 120, .byte3 = 64 };
    MidiMessageT mm_cc_ano = { .cin = MIDI_CIN_CONTROLCHANGE, .miditype = MIDI_CIN_CONTROLCHANGE, .byte2 = 123, .byte3 = 64 };
    MidiMessageT mm_pbwh = { .cin = MIDI_CIN_PITCHBEND, .miditype = MIDI_CIN_PITCHBEND, .byte2 = 64, .byte3 = 64 };
    MidiMessageT mm_noteon = { .cin = MIDI_CIN_NOTEON, .miditype = MIDI_CIN_NOTEON, .byte2 = 64, .byte3 = 64 };
    MidiMessageT mm_pc = { .cin = MIDI_CIN_PROGRAMCHANGE, .miditype = MIDI_CIN_PROGRAMCHANGE, .byte2 = 64, .byte3 = 64 };
    MidiMessageT mm_pressure = { .cin = MIDI_CIN_CHANNELPRESSURE, .miditype = MIDI_CIN_CHANNELPRESSURE, .byte2 = 64, .byte3 = 64 };

    mm.miditype = mm.cin = MIDI_CIN_SINGLEBYTE;
    mm.midichannel = 8;
    for (int i = 0; i < MIDI_TX_BUFFER_SIZE - 2; i++) {
        mm.byte2 = i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    PBUF(test_port_context);
    // same prio CC with others - who wins?!
    // cc msb = noteon
    // cc = pitchbend
    // cc special = pc
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm_cc_modwheel_lsb));
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm_pressure));
    TEST_ASSERT(mm_cc_modwheel_lsb.full_word == test_port_context.buf[(test_port_context.buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1)].full_word);
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm_pbwh));
    TEST_ASSERT(mm_pbwh.full_word == test_port_context.buf[(test_port_context.buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1)].full_word);
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm_cc_data_lsb)); // because addr is undefined!!!
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm_cc_modwheel_lsb));
    TEST_ASSERT(mm_cc_modwheel_lsb.full_word == test_port_context.buf[(test_port_context.buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1)].full_word);
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm_cc_modwheel_msb));
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm_cc_modwheel_lsb));
    TEST_ASSERT(mm_cc_modwheel_msb.full_word == test_port_context.buf[(test_port_context.buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1)].full_word);
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm_noteon));
    TEST_ASSERT(mm_noteon.full_word == test_port_context.buf[(test_port_context.buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1)].full_word);
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm_cc_data_msb)); // because addr is undefined!!!
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm_cc_modwheel_msb));
    TEST_ASSERT(mm_cc_modwheel_msb.full_word == test_port_context.buf[(test_port_context.buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1)].full_word);
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm_cc_ano));
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm_cc_modwheel_msb));
    TEST_ASSERT(mm_cc_ano.full_word == test_port_context.buf[(test_port_context.buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1)].full_word);
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm_pc));
    TEST_ASSERT(mm_pc.full_word == test_port_context.buf[(test_port_context.buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1)].full_word);
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm_cc_aso));
    TEST_ASSERT(mm_cc_aso.full_word == test_port_context.buf[(test_port_context.buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1)].full_word);
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm_cc_damper));
    TEST_ASSERT(mm_cc_damper.full_word == test_port_context.buf[(test_port_context.buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1)].full_word);
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm_cc_nrpn_lsb));
    TEST_ASSERT(mm_cc_nrpn_lsb.full_word == test_port_context.buf[(test_port_context.buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1)].full_word);
    PBUF(test_port_context);
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm_cc_rpn_msb));
    TEST_ASSERT(mm_cc_rpn_msb.full_word == test_port_context.buf[(test_port_context.buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1)].full_word);
    PBUF(test_port_context);
}

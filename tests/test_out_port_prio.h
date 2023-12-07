#include "../midi_output.h"
#include "test.h"

static void outPortTests_prio()
{
    MidiOutPortContextT test_port_context;

    MidiOutPortT test_port = {
        .context = &test_port_context,
        .api = 0,
        .name = "TEST",
        .type = MIDI_TYPE_USB,
        .cn = 0x9
    };
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

    midiPortInit(&test_port);
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // test prio
    mm.cin = MIDI_CIN_POLYKEYPRESS;
    for (int i = 0; i < 40; i++) {
        mm.byte2 = i;
        // TEST_ASSERT_int(((i < m_cin_priorities[mm.cin]) ? MIDI_RET_OK : MIDI_RET_FAIL) == midiPortWrite(&test_port, mm), i);
        // TEST_ASSERT_int(((i < (MIDI_TX_BUFFER_SIZE-1)) ? MIDI_RET_OK : MIDI_RET_FAIL) == midiPortWrite(&test_port, mm), i);
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    for (int i = 0; i < 40; i++) {
        // if (i < m_cin_priorities[mm.cin]) {
        if (i < (MIDI_TX_BUFFER_SIZE - 1)) {
            TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port, &mr), i);
            TEST_ASSERT_int(mr.byte2 == i + 9, i);
        } else {
            TEST_ASSERT_int(MIDI_RET_FAIL == midiPortReadNext(&test_port, &mr), i);
        }
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    mm.cin = MIDI_CIN_POLYKEYPRESS;
    for (int i = 0; i < 40; i++) {
        mm.byte2 = i;
        // TEST_ASSERT_int(((i < m_cin_priorities[mm.cin]) ? MIDI_RET_OK : MIDI_RET_FAIL) == midiPortWrite(&test_port, mm), i);
        // TEST_ASSERT_int(((i < (MIDI_TX_BUFFER_SIZE - 1)) ? MIDI_RET_OK : MIDI_RET_FAIL) == midiPortWrite(&test_port, mm), i);
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    // now there should be 10..40
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    printf("debug: %d %d %d" ENDLINE, mr.byte2, mr.cin, 0);

    mm.byte2 = 0x7F;
    mm.cin = MIDI_CIN_CHANNELPRESSURE;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    mm.cin = MIDI_CIN_PITCHBEND;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    mm.cin = MIDI_CIN_NOTEON;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    mm.cin = MIDI_CIN_PROGRAMCHANGE;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    mm.byte2 = 0x22;
    mm.cin = MIDI_CIN_PROGRAMCHANGE;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    mm.cin = MIDI_CIN_NOTEOFF;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    mm.byte2 = 0x23;
    mm.cin = MIDI_CIN_NOTEOFF;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    mm.byte2 = 0x24;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    mm.byte2 = 0x25;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    // utilisation is == lowest prio == polykp
    TEST_ASSERT((MIDI_TX_BUFFER_SIZE - 1) == midiPortMaxUtilisation(&test_port));

    // TODO: same prio CC with others - who wins?!

    // midiPortFlush(&test_port);
    // overflow 2 - again, it is harder, because of filtering!!
    // mm.cin = MIDI_CIN_SINGLEBYTE;
    // mm.cin = MIDI_CIN_3BYTESYSTEMCOMMON;
    // // printf("debug: %d %d %d" ENDLINE, test_port_context.max_utilisation, test_port_context.buf_rp, test_port_context.buf_wp);
    // printf("debug: %d %d %d" ENDLINE, test_port_context.max_utilisation, test_port_context.buf_rp, test_port_context.buf_wp);

    // TEST_ASSERT(3 == midiPortUtilisation(&test_port));
    // TEST_ASSERT(3 == midiPortSysexUtilisation(&test_port));

    // TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port, &mr));
    // TEST_ASSERT(mr.byte2 == 0x77);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    // printf("debug: %d %d" ENDLINE, midiPortUtilisation(&test_port), midiPortSysexUtilisation(&test_port));
    // TEST_ASSERT(4 == midiPortMaxUtilisation(&test_port));
    // printf("debug: %d %d" ENDLINE, test_port_context.sysex_rp, test_port_context.sysex_wp);
    // printf("midiPortMaxSysexUtilisation: %d" ENDLINE, midiPortMaxSysexUtilisation(&test_port));
    // // TEST_ASSERT(0 == midiPortMaxSysexUtilisation(&test_port));

    // printf("debug: %d %d %d" ENDLINE, mr.cn, 0, 0);
    // printf("debug: %d %d %d" ENDLINE, test_port_context.buf_rp, test_port_context.buf_wp, MIDI_TX_BUFFER_SIZE);
    // printf("debug: %d %d %d" ENDLINE, test_port_context.buf_rp, test_port_context.buf_wp, MIDI_TX_BUFFER_SIZE);
    // printf("debug: %d %d %d" ENDLINE, test_port_context.buf_rp, test_port_context.buf_wp, MIDI_TX_BUFFER_SIZE);
    // printf("debug: %d %d %d" ENDLINE, test_port_context.buf_rp, test_port_context.buf_wp, MIDI_TX_BUFFER_SIZE);

    // TEST_TODO("midiPortWrite");
    // TEST_TODO("    - write channel message");
    // TEST_TODO("        - note on with zero?");
    // TEST_TODO("        - repeated events");
    // TEST_TODO("            - noteoff");
    // TEST_TODO("            - noteon");
    // TEST_TODO("            - poly kp");
    // TEST_TODO("            - cc");
    // TEST_TODO("                - nrpn");
    // TEST_TODO("                - channel mode");
    // TEST_TODO("                - multiple cc");
    // TEST_TODO("                - high resolution cc 0x01/0x21");
    // TEST_TODO("            - pc");
    // TEST_TODO("            - channel pressure");
    // TEST_TODO("            - pitchbend");

    // TEST_TODO("        - priority");
    // TEST_TODO("            - noteoff");
    // TEST_TODO("            - noteon");
    // TEST_TODO("            - poly kp");
    // TEST_TODO("            - cc");
    // TEST_TODO("                - nrpn");
    // TEST_TODO("                - channel mode");
    // TEST_TODO("                - damper");
    // TEST_TODO("                - high resolution cc 0x01/0x21");
    // TEST_TODO("            - pc");
    // TEST_TODO("            - channel pressure");
    // TEST_TODO("            - pitchbend");
    // TEST_TODO("    - sysex");
    // TEST_TODO("        - is not interruptable by others");
    // TEST_TODO("        - is not interruptable by others");
    // TEST_TODO("    - realtime");
    // TEST_TODO("        - is always on top");
    // TEST_TODO("        - is always on top");

    // TEST_TODO("        - messages flushed");
    // TEST_TODO("        - messages optimized");
    // sysex lock
}

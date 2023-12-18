#include "../midi_output.h"
#include "test.h"

static void outPortTests_optim()
{
    MidiOutPortContextT test_port_context;

    MidiOutPortT test_port = {
        .context = &test_port_context,
        .api = 0,
        .name = "TEST",
        .type = MIDI_TYPE_USB,
        .cn = 0x9
    };
    midiPortInit(&test_port);
    TEST_NEW_BLOCK("out port optim");

    MidiMessageT mm = { .cin = MIDI_CIN_NOTEON, .miditype = MIDI_CIN_NOTEON, .byte2 = 0, .byte3 = 0 };
    MidiMessageT mr;
    /////////////////////////////////////////////////////////////////////////////////////////////
    // notoff, noteon
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port));
    mm.cin = mm.miditype = MIDI_CIN_NOTEOFF;
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port));
    mm.cin = mm.miditype = MIDI_CIN_NOTEON;
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(1 + 8 == midiPortUtilisation(&test_port));
    mm.cin = mm.miditype = MIDI_CIN_NOTEOFF;
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(1 + 8 == midiPortUtilisation(&test_port));
    mm.cin = mm.miditype = MIDI_CIN_NOTEOFF;
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(0 == mr.byte2);
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.byte2, i);
    }
    for (int i = 0; i < 4; i++) {
        mm.midichannel = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(4 == midiPortUtilisation(&test_port));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.midichannel, i);
        // printf("debug: %d" ENDLINE, mr.byte2);
    }
    // midiPortFlush(&test_port);
    // printf("debug: %d" ENDLINE, test_port_context.buf[test_port_context.buf_rp+1].byte2);

    /////////////////////////////////////////////////////////////////////////////////////////////
    // poly kp
    mm.cin = mm.miditype = MIDI_CIN_POLYKEYPRESS;
    mm.midichannel = 0;
    mm.byte2 = mm.byte3 = 0;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port));
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(1 + 8 == midiPortUtilisation(&test_port));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(0 == mr.byte2);
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.byte2, i);
    }
    for (int i = 0; i < 4; i++) {
        mm.midichannel = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(4 == midiPortUtilisation(&test_port));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.midichannel, i);
        // printf("debug: %d" ENDLINE, mr.byte2);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////
    // pc
    mm.cin = mm.miditype = MIDI_CIN_PROGRAMCHANGE;
    mm.midichannel = 0;
    mm.byte2 = mm.byte3 = 0;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port));
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(0x80 == mr.byte2);
    for (int i = 0; i < 4; i++) {
        mm.midichannel = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(4 == midiPortUtilisation(&test_port));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.midichannel, i);
        // printf("debug: %d" ENDLINE, mr.byte2);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////
    // channel pressure
    mm.cin = mm.miditype = MIDI_CIN_CHANNELPRESSURE;
    mm.midichannel = 0;
    mm.byte2 = mm.byte3 = 0;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port));
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(0x80 == mr.byte2);
    for (int i = 0; i < 4; i++) {
        mm.midichannel = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(4 == midiPortUtilisation(&test_port));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.midichannel, i);
        // printf("debug: %d" ENDLINE, mr.byte2);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////
    // pb
    mm.cin = mm.miditype = MIDI_CIN_PITCHBEND;
    mm.midichannel = 0;
    mm.byte2 = mm.byte3 = 0;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port));
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(0x80 == mr.byte2);
    for (int i = 0; i < 4; i++) {
        mm.midichannel = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
    }
    TEST_ASSERT(4 == midiPortUtilisation(&test_port));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.midichannel, i);
        // printf("debug: %d" ENDLINE, mr.byte2);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////
    // singlebyte, system common messages - no optimisations
    uint8_t mtype[3] = { MIDI_CIN_SINGLEBYTE, MIDI_CIN_2BYTESYSTEMCOMMON, MIDI_CIN_3BYTESYSTEMCOMMON };
    uint8_t mbyte[3] = { 0xF8, 0xF1, 0xF2 };
    for (uint8_t j = 0; j < 3; j++) {
        mm.cin = mtype[j];
        mm.byte1 = mbyte[j];
        mm.byte2 = mm.byte3 = 0;
        for (int i = 0; i < 8; i++) {
            mm.byte3 = 1 << i;
            TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
        }
        TEST_ASSERT(8 == midiPortUtilisation(&test_port));
        for (int i = 0; i < 8; i++) {
            TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port, &mr), i);
            TEST_ASSERT_int((1 << i) == mr.byte3, i);
        }
        for (int i = 0; i < 8; i++) {
            mm.byte2 = 1 << i;
            TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port, mm), i);
        }
        TEST_ASSERT(8 == midiPortUtilisation(&test_port));
        for (int i = 0; i < 8; i++) {
            TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port, &mr), i);
            TEST_ASSERT_int((1 << i) == mr.byte2, i);
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////////////
    // cc, channel mode, nrpn

    TEST_TODO("m_compare_mask");
    TEST_TODO("cc sequence reordering - overwrite or delete+add");
    TEST_TODO("other messages reordering - overwrite or delete+add");
    TEST_TODO("status should not be modified by flushed message");
    TEST_TODO("nrpn addr timer");

    TEST_ASSERT(MIDI_TX_BUFFER_SIZE == 32);
    TEST_ASSERT(MIDI_TX_SYSEX_BUFFER_SIZE == 16);

    // remember, that noteon and noteoff are special case and almost is the one
}

#include "../midi_output.h"
#include "test.h"

typedef enum { // TODO: unused??
    CC_DEFAULT = 0,
    CC____DATH = 0x6,
    CC____DATL = 0x26,
    CC__DAMPER = 0x40,
    CC_____INC = 0x60,
    CC_____DEC = 0x61,
    CC__NRPNAH = 0x62,
    CC__NRPNAL = 0x63,
    CC___RPNAH = 0x64,
    CC___RPNAL = 0x65,
    CC_NTESOFF = 0x78,
    CC_RESETCC = 0x79,
    CC_LOCALON = 0x7A,
    CC_SNDSOFF = 0x7B,
    CC_OMNIOFF = 0x7C,
    CC__OMNION = 0x7D,
    CC__MONOPH = 0x7E,
    CC__POLYPH = 0x7F,
} tmcc_nrpn_e;

static void outPortTests_optim()
{
    MidiOutPortContextT test_port_context;

    midiPortInit(&test_port_context);
    TEST_NEW_BLOCK("out port optim");

    MidiMessageT mm = { .cin = MIDI_CIN_NOTEON, .miditype = MIDI_CIN_NOTEON, .byte2 = 0, .byte3 = 0 };
    MidiMessageT mr;
    /////////////////////////////////////////////////////////////////////////////////////////////
    // interference
    for (int i = 9; i < 0xF; i++) {
        mm.cin = mm.miditype = i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(0xF - 9 == midiPortUtilisation(&test_port_context));
    for (int i = 9; i < 0xF; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        TEST_ASSERT_int(i == mr.miditype, i);
    }
    TEST_ASSERT(0 == midiPortUtilisation(&test_port_context));
    /////////////////////////////////////////////////////////////////////////////////////////////
    // correct scan
    mm.midichannel = 5;
    mm.byte2 = 0x55;
    mm.byte3 = 0xAA;
    for (int i = 9; i < 0xF; i++) {
        mm.cin = mm.miditype = i;
        for (int w = 0; w < MIDI_TX_BUFFER_SIZE; w++) {
            test_port_context.buf[w].full_word = mm.full_word;
        }
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
        TEST_ASSERT(1 == midiPortUtilisation(&test_port_context));
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        TEST_ASSERT_int(i == mr.miditype, i);
        TEST_ASSERT(0 == midiPortUtilisation(&test_port_context));
    }
    /////////////////////////////////////////////////////////////////////////////////////////////
    // notoff, noteon
    mm.cin = mm.miditype = MIDI_CIN_NOTEON;
    mm.midichannel = mm.byte2 = mm.byte3 = 0;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port_context));
    mm.cin = mm.miditype = MIDI_CIN_NOTEOFF;
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port_context));
    mm.cin = mm.miditype = MIDI_CIN_NOTEON;
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 + 8 == midiPortUtilisation(&test_port_context));
    mm.cin = mm.miditype = MIDI_CIN_NOTEOFF;
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 + 8 == midiPortUtilisation(&test_port_context));
    mm.cin = mm.miditype = MIDI_CIN_NOTEOFF;
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(0 == mr.byte2);
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.byte2, i);
    }
    for (int i = 0; i < 4; i++) {
        mm.midichannel = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(4 == midiPortUtilisation(&test_port_context));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.midichannel, i);
        // printf("debug: %d" ENDLINE, mr.byte2);
    }
    // midiPortFlush(&test_port_context);
    // printf("debug: %d" ENDLINE, test_port_context.buf[test_port_context.buf_rp+1].byte2);

    /////////////////////////////////////////////////////////////////////////////////////////////
    // poly kp
    mm.cin = mm.miditype = MIDI_CIN_POLYKEYPRESS;
    mm.midichannel = 0;
    mm.byte2 = mm.byte3 = 0;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port_context));
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 + 8 == midiPortUtilisation(&test_port_context));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(0 == mr.byte2);
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.byte2, i);
    }
    for (int i = 0; i < 4; i++) {
        mm.midichannel = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(4 == midiPortUtilisation(&test_port_context));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.midichannel, i);
        // printf("debug: %d" ENDLINE, mr.byte2);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////
    // pc
    mm.cin = mm.miditype = MIDI_CIN_PROGRAMCHANGE;
    mm.midichannel = 0;
    mm.byte2 = mm.byte3 = 0;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port_context));
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port_context));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(0x80 == mr.byte2);
    for (int i = 0; i < 4; i++) {
        mm.midichannel = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(4 == midiPortUtilisation(&test_port_context));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.midichannel, i);
        // printf("debug: %d" ENDLINE, mr.byte2);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////
    // channel pressure
    mm.cin = mm.miditype = MIDI_CIN_CHANNELPRESSURE;
    mm.midichannel = 0;
    mm.byte2 = mm.byte3 = 0;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port_context));
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port_context));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(0x80 == mr.byte2);
    for (int i = 0; i < 4; i++) {
        mm.midichannel = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(4 == midiPortUtilisation(&test_port_context));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.midichannel, i);
        // printf("debug: %d" ENDLINE, mr.byte2);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////
    // pb
    mm.cin = mm.miditype = MIDI_CIN_PITCHBEND;
    mm.midichannel = 0;
    mm.byte2 = mm.byte3 = 0;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port_context));
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port_context));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(0x80 == mr.byte2);
    for (int i = 0; i < 4; i++) {
        mm.midichannel = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(4 == midiPortUtilisation(&test_port_context));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
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
            TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
        }
        TEST_ASSERT(8 == midiPortUtilisation(&test_port_context));
        for (int i = 0; i < 8; i++) {
            TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
            TEST_ASSERT_int((1 << i) == mr.byte3, i);
        }
        for (int i = 0; i < 8; i++) {
            mm.byte2 = 1 << i;
            TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
        }
        TEST_ASSERT(8 == midiPortUtilisation(&test_port_context));
        for (int i = 0; i < 8; i++) {
            TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
            TEST_ASSERT_int((1 << i) == mr.byte2, i);
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////////////
    // cc, channel mode, nrpn
    mm.cin = mm.miditype = MIDI_CIN_CONTROLCHANGE;
    mm.midichannel = 0;
    mm.byte2 = mm.byte3 = 0;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    for (int i = 0; i < 8; i++) {
        mm.byte3 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 == midiPortUtilisation(&test_port_context));
    for (int i = 0; i < 8; i++) {
        mm.byte2 = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(1 + 8 == midiPortUtilisation(&test_port_context));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(0 == mr.byte2);
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.byte2, i);
    }
    for (int i = 0; i < 4; i++) {
        mm.midichannel = 1 << i;
        TEST_ASSERT_int(MIDI_RET_OK == midiPortWrite(&test_port_context, mm), i);
    }
    TEST_ASSERT(4 == midiPortUtilisation(&test_port_context));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr), i);
        TEST_ASSERT_int((1 << i) == mr.midichannel, i);
        // printf("debug: %d" ENDLINE, mr.byte2);
    }

    //
    mm.cin = mm.miditype = MIDI_CIN_CONTROLCHANGE;
    mm.byte2 = 0x55;
    mm.byte3 = 0xAA;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = 0x66;
    mm.byte3 = 0xAA;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = 0x77;
    mm.byte3 = 0xAA;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = 0x55;
    mm.byte3 = 0x00;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT((0x55 == mr.byte2) && (0 == mr.byte3));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT((0x66 == mr.byte2) && (0xAA == mr.byte3));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT((0x77 == mr.byte2) && (0xAA == mr.byte3));
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));
    // TEST_TODO("cc reordering - overwrite instead of delete+add");

    mm.cin = mm.miditype = MIDI_CIN_NOTEOFF;
    mm.byte2 = 0x55;
    mm.byte3 = 0xAA;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = 0x66;
    mm.byte3 = 0xAA;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = 0x77;
    mm.byte3 = 0xAA;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = 0x55;
    mm.byte3 = 0x00;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT((0x55 == mr.byte2) && (0 == mr.byte3));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT((0x66 == mr.byte2) && (0xAA == mr.byte3));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT((0x77 == mr.byte2) && (0xAA == mr.byte3));
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));
    // TEST_TODO("other messages reordering - overwrite instead of delete+add");

    // rpn - nrpn write
    mm.cin = mm.miditype = MIDI_CIN_CONTROLCHANGE;
    mm.byte2 = CC____DATH;
    mm.byte3 = 0;
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC____DATL;
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC_____INC;
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC_____DEC;
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC___RPNAH;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm)); // rpnah
    mm.byte2 = CC_____DEC;
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC__NRPNAL;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm)); // nrpnal
    mm.byte2 = CC_____DEC;
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC___RPNAL;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm)); // rpnal
    mm.byte2 = CC_____DEC;
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC___RPNAH;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm)); // rpnah
    mm.byte2 = CC_____DEC;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(CC___RPNAH == mr.byte2);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(CC__NRPNAL == mr.byte2);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(CC___RPNAL == mr.byte2);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(CC___RPNAH == mr.byte2);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(CC_____DEC == mr.byte2);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));

    // repeat now
    mm.byte2 = CC___RPNAL;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC_____DEC;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC___RPNAH;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC_____DEC;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(CC_____DEC == mr.byte2);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));
    // 1 sec
    test_clock += TEST_SAMPLE_RATE;
    mm.byte2 = CC___RPNAL;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC_____DEC;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC___RPNAH;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC_____DEC;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(CC_____DEC == mr.byte2);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));
    // 2 sec, combine with cc
    test_clock += TEST_SAMPLE_RATE;
    mm.byte2 = 1; // modwheel
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC___RPNAL;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC_____DEC;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC___RPNAH;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC_____DEC;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = 1; // modwheel
    mm.byte3 = 55;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT((1 == mr.byte2) && (55 == mr.byte3));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(CC___RPNAL == mr.byte2);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(CC_____DEC == mr.byte2);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(CC___RPNAH == mr.byte2);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(CC_____DEC == mr.byte2);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));

    // TEST_TODO("message in the middle, that replacing/flushing is correct");
    mm.byte2 = 1;
    mm.byte3 = 55;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = 2;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = 3;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = 4;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = 5;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = 3;
    mm.byte3 = 33;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT((1 == mr.byte2) && (55 == mr.byte3));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT((2 == mr.byte2) && (55 == mr.byte3));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT((3 == mr.byte2) && (33 == mr.byte3));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT((4 == mr.byte2) && (55 == mr.byte3));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT((5 == mr.byte2) && (55 == mr.byte3));

    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));

    // status reset
    mm.byte2 = CC_____INC;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte2 = CC___RPNAH;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.cin = mm.miditype = MIDI_CIN_SINGLEBYTE;
    for (int i = 0; i < MIDI_TX_BUFFER_SIZE - 1; i++) {
        TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    }
    for (int i = 0; i < MIDI_TX_BUFFER_SIZE - 1; i++) {
        TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
        TEST_ASSERT(MIDI_CIN_SINGLEBYTE == mr.cin);
    }
    mm.cin = mm.miditype = MIDI_CIN_CONTROLCHANGE;
    mm.byte2 = CC_____INC;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm)); // flush does not detect this

    mm.cin = mm.miditype = MIDI_CIN_SINGLEBYTE;
    for (int i = 0; i < MIDI_TX_BUFFER_SIZE - 1; i++) {
        TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    }
    mm.cin = mm.miditype = MIDI_CIN_CONTROLCHANGE;
    // printf("debug: %08X" ENDLINE, midiPortUtilisation(&test_port_context));
    test_clock += TEST_SAMPLE_RATE * 5;
    mm.byte2 = CC___RPNAL;
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm));
    for (int i = 0; i < MIDI_TX_BUFFER_SIZE - 1; i++) {
        TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
        TEST_ASSERT(MIDI_CIN_SINGLEBYTE == mr.cin);
    }

    mm.cin = mm.miditype = MIDI_CIN_CONTROLCHANGE;
    mm.byte2 = CC_____INC;
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, mm));

    TEST_TODO("status should not be modified by flushed message");

    // printf("debug: %08X" ENDLINE, mr.full_word);
    // printf("debug: %08X" ENDLINE, mr.full_word);
    // printf("debug: %08X" ENDLINE, mr.full_word);
    TEST_ASSERT(MIDI_TX_BUFFER_SIZE == 32);
    TEST_ASSERT(MIDI_TX_SYSEX_BUFFER_SIZE == 16);

    // remember, that noteon and noteoff are special case and almost is the one
}

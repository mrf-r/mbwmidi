#include "../midi_output.h"
#include "test.h"

static void outPortTests()
{
    MidiOutPortContextT test_port_context;
    TEST_NEW_BLOCK("out port");

    TEST_ASSERT(MIDI_TX_BUFFER_SIZE == 32);
    TEST_ASSERT(MIDI_TX_SYSEX_BUFFER_SIZE == 16);
    MidiMessageT mm = {
        .cn = MIDI_CN_TEST0,
        .cin = MIDI_CIN_PITCHBEND,
        .byte1 = MIDI_CIN_PITCHBEND << 4,
        .byte2 = 0x33,
        .byte3 = 0x44
    };
    MidiMessageT mm2 = {
        .cn = MIDI_CN_TEST0,
        .cin = MIDI_CIN_POLYKEYPRESS,
        .byte1 = MIDI_CIN_POLYKEYPRESS << 4,
        .byte2 = 0x33,
        .byte3 = 0x44
    };
    MidiMessageT ms = {
        .cn = MIDI_CN_TEST0,
        .cin = MIDI_CIN_SYSEXEND2,
        .byte1 = 0x77, // anyway it does not check that
        .byte2 = 0xF7, // only uart/com will notice wrong bytes
        .byte3 = 0x99
    };
    MidiMessageT ms2 = {
        .cn = MIDI_CN_TEST2,
        .cin = MIDI_CIN_SYSEX3BYTES,
        .byte1 = 0xF0, // anyway it does not check that
        .byte2 = 0x88, // only uart/com will notice wrong bytes
        .byte3 = 0x99
    };
    MidiMessageT mr;

    midiPortInit(&test_port_context);
    // printf("midiPortFreespaceGet: %d/%d" ENDLINE, midiPortFreespaceGet(&test_port_context), MIDI_TX_BUFFER_SIZE);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(MIDI_RET_OK == midiPortWriteRaw(&test_port_context, mm));
    mm.byte3 = 0x45;
    TEST_ASSERT(MIDI_RET_OK == midiPortWriteRaw(&test_port_context, mm));
    mm.byte3 = 0x46;
    TEST_ASSERT(MIDI_RET_OK == midiPortWriteRaw(&test_port_context, mm));

    TEST_ASSERT(3 == midiPortUtilisation(&test_port_context));
    TEST_ASSERT(0 == midiPortSysexUtilisation(&test_port_context));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(MIDI_CN_TEST0 == mr.cn);
    TEST_ASSERT(MIDI_CIN_PITCHBEND == mr.cin);
    TEST_ASSERT(MIDI_CIN_PITCHBEND << 4 == mr.byte1);
    TEST_ASSERT(0x33 == mr.byte2);
    TEST_ASSERT(0x44 == mr.byte3);
    // after being switched to raw mode, we need flush to exit
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, ms));
    ms.byte3 = 0xAA;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, ms));
    TEST_ASSERT(4 == midiPortUtilisation(&test_port_context));
    TEST_ASSERT(0 == midiPortSysexUtilisation(&test_port_context));

    midiPortFlush(&test_port_context);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(0 == midiPortUtilisation(&test_port_context));
    TEST_ASSERT(0 == midiPortSysexUtilisation(&test_port_context));
    // ok, as we are now in full feature mode, let's experience some optimisations
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm2));
    mm.byte3 = 0x23;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    mm.byte3 = 0x25;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm));
    // syx is only one who will act normally no matter what)
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, ms));
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, ms));
    TEST_ASSERT(2 == midiPortUtilisation(&test_port_context));
    TEST_ASSERT(2 == midiPortSysexUtilisation(&test_port_context));

    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(MIDI_CN_TEST0 == mr.cn);
    // printf("debug: %08X" ENDLINE, mr.full_word);
    // printf("debug: %08X" ENDLINE, mm2.full_word);
    // reordering: optimisation set to replace instead of delete+add
    TEST_ASSERT(mm.cin == mr.cin);
    TEST_ASSERT(mm.byte1 == mr.byte1);
    TEST_ASSERT(mm.byte2 == mr.byte2);
    TEST_ASSERT(mm.byte3 == mr.byte3);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(MIDI_CN_TEST0 == mr.cn);
    TEST_ASSERT(mm2.cin == mr.cin);
    TEST_ASSERT(mm2.byte1 == mr.byte1);
    TEST_ASSERT(mm2.byte2 == mr.byte2);
    TEST_ASSERT(mm2.byte3 == mr.byte3);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(MIDI_CN_TEST0 == mr.cn);
    TEST_ASSERT(ms.cin == mr.cin);
    TEST_ASSERT(ms.byte1 == mr.byte1);
    TEST_ASSERT(ms.byte2 == mr.byte2);
    TEST_ASSERT(ms.byte3 == mr.byte3);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(MIDI_CN_TEST0 == mr.cn);
    TEST_ASSERT(ms.cin == mr.cin);
    TEST_ASSERT(ms.byte1 == mr.byte1);
    TEST_ASSERT(ms.byte2 == mr.byte2);
    TEST_ASSERT(ms.byte3 == mr.byte3);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // circular
    for (int i = 0; i < 200; i++) {
        TEST_ASSERT(MIDI_RET_OK == midiPortWriteRaw(&test_port_context, mm));
        TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
        TEST_ASSERT(mr.full_word == mm.full_word);
        // printf("%08X %08X \n", mr.full_word, mm.full_word);
    }
    TEST_ASSERT(4 == midiPortMaxUtilisation(&test_port_context));
    TEST_ASSERT(0 == midiPortMaxUtilisation(&test_port_context));
    TEST_ASSERT(2 == midiPortMaxSysexUtilisation(&test_port_context));
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // overflow
    mm.cin = mm.miditype = MIDI_CIN_NOTEOFF;
    // printf("debug: %d %d %d" ENDLINE, test_port_context.max_utilisation, test_port_context.buf_rp, test_port_context.buf_wp);
    for (int i = 0; i < 40; i++) {
        mm.byte2 = i;
        TEST_ASSERT(((i < 31) ? MIDI_RET_OK : MIDI_RET_FAIL) == midiPortWriteRaw(&test_port_context, mm));
    }
    // printf("debug: %d %d %d" ENDLINE, test_port_context.max_utilisation, test_port_context.buf_rp, test_port_context.buf_wp);
    for (int i = 0; i < 31; i++) {
        TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
        TEST_ASSERT(mr.byte2 == i);
    }
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // sysex lock
    midiPortFlush(&test_port_context);
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, ms2));
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, ms2));
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, ms2));
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, ms));
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port_context, ms));
    TEST_ASSERT(0 == midiPortUtilisation(&test_port_context));
    TEST_ASSERT(3 == midiPortSysexUtilisation(&test_port_context));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(mr.full_word == ms2.full_word);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(mr.full_word == ms2.full_word);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(mr.full_word == ms2.full_word);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    mm2.cin = mm2.miditype = MIDI_CIN_NOTEOFF;
    mm2.byte2 = 1;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm2));
    mm2.byte2 = 2;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm2));
    mm2.byte2 = 3;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, mm2));
    TEST_ASSERT(3 == midiPortUtilisation(&test_port_context));

    // we have not terminated sysex stream!!
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port_context, &mr));

    ms2.cin = MIDI_CIN_SYSEXEND3;
    ms2.byte3 = 0xF7;
    ms2.byte2 = 0x77;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, ms2));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(mr.full_word == ms2.full_word);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(mr.byte2 == 1);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(mr.byte2 == 2);
    // write single sysex, but it will wait till main buffer empty
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port_context, ms2));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(mr.byte2 == 3);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port_context, &mr));
    TEST_ASSERT(mr.full_word == ms2.full_word);

    TEST_TODO("nrpn rpn address undefined test");
    TEST_TODO("atomic was not tested!");

    // TODO!!!
}

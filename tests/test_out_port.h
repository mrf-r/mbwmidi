#include "../midi_output.h"
#include "test.h"

static void outPortTests()
{
    MidiOutPortContextT test_port_context;

    MidiOutPortT test_port = {
        .context = &test_port_context,
        .api = 0,
        .name = "TEST",
        .type = MIDI_TYPE_USB,
        .cn = 0x9
    };

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
        .byte2 = 0x88, // only uart/com will notice wrong bytes
        .byte3 = 0x99
    };
    MidiMessageT ms2 = {
        .cn = MIDI_CN_TEST2,
        .cin = MIDI_CIN_SYSEX3BYTES,
        .byte1 = 0x77, // anyway it does not check that
        .byte2 = 0x88, // only uart/com will notice wrong bytes
        .byte3 = 0x99
    };
    MidiMessageT mr;

    midiPortInit(&test_port);
    // printf("midiPortFreespaceGet: %d/%d" ENDLINE, midiPortFreespaceGet(&test_port), MIDI_TX_BUFFER_SIZE);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(MIDI_RET_OK == midiPortWriteRaw(&test_port, mm));
    mm.byte3 = 0x45;
    TEST_ASSERT(MIDI_RET_OK == midiPortWriteRaw(&test_port, mm));
    mm.byte3 = 0x46;
    TEST_ASSERT(MIDI_RET_OK == midiPortWriteRaw(&test_port, mm));

    TEST_ASSERT(3 == midiPortUtilisation(&test_port));
    TEST_ASSERT(0 == midiPortSysexUtilisation(&test_port));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(0x9 == mr.cn);
    TEST_ASSERT(MIDI_CIN_PITCHBEND == mr.cin);
    TEST_ASSERT(MIDI_CIN_PITCHBEND << 4 == mr.byte1);
    TEST_ASSERT(0x33 == mr.byte2);
    TEST_ASSERT(0x44 == mr.byte3);
    // after being switched to raw mode, we need flush to exit
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, ms));
    ms.byte3 = 0xAA;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, ms));
    TEST_ASSERT(4 == midiPortUtilisation(&test_port));
    TEST_ASSERT(0 == midiPortSysexUtilisation(&test_port));

    midiPortFlush(&test_port);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(0 == midiPortUtilisation(&test_port));
    TEST_ASSERT(0 == midiPortSysexUtilisation(&test_port));
    // ok, as we are now in full feature mode, let's experience some optimisations
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm2));
    mm.byte3 = 0x23;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    mm.byte3 = 0x25;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm));
    // syx is only one who will act normally no matter what)
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, ms));
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, ms));
    TEST_ASSERT(2 == midiPortUtilisation(&test_port));
    TEST_ASSERT(2 == midiPortSysexUtilisation(&test_port));

    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(0x9 == mr.cn);
    TEST_ASSERT(mm2.cin == mr.cin);
    TEST_ASSERT(mm2.byte1 == mr.byte1);
    TEST_ASSERT(mm2.byte2 == mr.byte2);
    TEST_ASSERT(mm2.byte3 == mr.byte3);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(0x9 == mr.cn);
    TEST_ASSERT(mm.cin == mr.cin);
    TEST_ASSERT(mm.byte1 == mr.byte1);
    TEST_ASSERT(mm.byte2 == mr.byte2);
    TEST_ASSERT(mm.byte3 == mr.byte3);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(0x9 == mr.cn);
    TEST_ASSERT(ms.cin == mr.cin);
    TEST_ASSERT(ms.byte1 == mr.byte1);
    TEST_ASSERT(ms.byte2 == mr.byte2);
    TEST_ASSERT(ms.byte3 == mr.byte3);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(0x9 == mr.cn);
    TEST_ASSERT(ms.cin == mr.cin);
    TEST_ASSERT(ms.byte1 == mr.byte1);
    TEST_ASSERT(ms.byte2 == mr.byte2);
    TEST_ASSERT(ms.byte3 == mr.byte3);
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port, &mr));
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // circular
    for (int i = 0; i < 200; i++) {
        TEST_ASSERT(MIDI_RET_OK == midiPortWriteRaw(&test_port, mm));
        TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
        TEST_ASSERT(mr.full_word == ((mm.full_word & 0xFFFFFF0F) | 0x90));
    }
    TEST_ASSERT(4 == midiPortMaxUtilisation(&test_port));
    TEST_ASSERT(0 == midiPortMaxUtilisation(&test_port));
    TEST_ASSERT(2 == midiPortMaxSysexUtilisation(&test_port));
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // overflow
    mm.cin = MIDI_CIN_NOTEOFF;
    // printf("debug: %d %d %d" ENDLINE, test_port_context.max_utilisation, test_port_context.buf_rp, test_port_context.buf_wp);
    for (int i = 0; i < 40; i++) {
        mm.byte2 = i;
        TEST_ASSERT(((i < 31) ? MIDI_RET_OK : MIDI_RET_FAIL) == midiPortWriteRaw(&test_port, mm));
    }
    // printf("debug: %d %d %d" ENDLINE, test_port_context.max_utilisation, test_port_context.buf_rp, test_port_context.buf_wp);
    for (int i = 0; i < 31; i++) {
        TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
        TEST_ASSERT(mr.byte2 == i);
    }
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port, &mr));
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // sysex lock
    midiPortFlush(&test_port);
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, ms2));
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, ms2));
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, ms2));
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port, ms));
    TEST_ASSERT(MIDI_RET_FAIL == midiPortWrite(&test_port, ms));
    TEST_ASSERT(0 == midiPortUtilisation(&test_port));
    TEST_ASSERT(3 == midiPortSysexUtilisation(&test_port));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(mr.full_word == ((ms2.full_word & 0xFFFFFF0F) | 0x90));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(mr.full_word == ((ms2.full_word & 0xFFFFFF0F) | 0x90));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(mr.full_word == ((ms2.full_word & 0xFFFFFF0F) | 0x90));
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port, &mr));
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    mm2.cin = MIDI_CIN_NOTEOFF;
    mm2.byte2 = 1;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm2));
    mm2.byte2 = 2;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm2));
    mm2.byte2 = 3;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, mm2));
    TEST_ASSERT(3 == midiPortUtilisation(&test_port));

    // we have not terminated sysex stream!!
    TEST_ASSERT(MIDI_RET_FAIL == midiPortReadNext(&test_port, &mr));

    ms2.cin = MIDI_CIN_SYSEXEND3;
    ms2.byte2 = 0x77;
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, ms2));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(mr.full_word == ((ms2.full_word & 0xFFFFFF0F) | 0x90));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(mr.byte2 == 1);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(mr.byte2 == 2);
    // write single sysex, but it will wait till main buffer empty
    TEST_ASSERT(MIDI_RET_OK == midiPortWrite(&test_port, ms2));
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(mr.byte2 == 3);
    TEST_ASSERT(MIDI_RET_OK == midiPortReadNext(&test_port, &mr));
    TEST_ASSERT(mr.full_word == ((ms2.full_word & 0xFFFFFF0F) | 0x90));

    TEST_TODO("atomic was not tested!");

    // TODO!!!
}

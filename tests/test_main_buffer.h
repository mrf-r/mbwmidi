#include "../midi.h"
#include "../midi_input.h"
#include "test.h"

static void mainBufferTests()
{
    TEST_NEW_BLOCK("main input buffer");
    midiInit();
    MidiMessageT mm;
    test_clock = 0x11223344;
    mm.cin = MIDI_CIN_PITCHBEND;
    mm.cn = MIDI_CN_TEST1;
    mm.byte1 = mm.cin << 4; // 0th  channel
    mm.byte2 = 0x44;
    mm.byte3 = 0x55;
    TEST_ASSERT(MIDI_RET_OK == midiWrite(mm));
    MidiTsMessageT mts;
    TEST_ASSERT(midiUtilisationGet() == 1);
    TEST_ASSERT(midiSysexUtilisationGet() == 0);
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mts));
    TEST_ASSERT(mts.mes.full_word == mm.full_word);
    TEST_ASSERT(mts.timestamp == test_clock);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mts));

    for (int i = 0; i < 16; i++) {
        mm.byte3 = i;
        TEST_ASSERT(MIDI_RET_OK == midiWrite(mm));
        TEST_ASSERT(midiUtilisationGet() == i + 1);
    }
    for (int i = 0; i < 16; i++) {
        mm.byte3 = i;
        TEST_ASSERT(MIDI_RET_OK == midiRead(&mts));
        TEST_ASSERT(mts.mes.byte3 == i);
        TEST_ASSERT(midiUtilisationGet() == 15 - i);
    }
    // buffer cycling
    for (int i = 0; i < 800; i++) {
        test_clock = i;
        TEST_ASSERT_int(MIDI_RET_OK == midiWrite(mm), i);
        TEST_ASSERT_int(MIDI_RET_OK == midiRead(&mts), i);
        TEST_ASSERT(mts.timestamp == i);
    }
    // overflow
    for (int i = 0; i < 270; i++) {
        TEST_ASSERT_int(midiWrite(mm) == ((i < 255) ? MIDI_RET_OK : MIDI_RET_FAIL), i);
        TEST_ASSERT_int(midiUtilisationGet() == (i < 255 ? i + 1 : 255), i);
    }
    midiInit();
    mm.cin = MIDI_CIN_NOTEON;
    mm.byte1 = mm.cin << 4;
    mm.byte2 = 0x23;
    mm.byte3 = 0x15;
    TEST_ASSERT(MIDI_RET_OK == midiWrite(mm));
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mts));
    TEST_ASSERT(mts.mes.cin == MIDI_CIN_NOTEON);
    // printf("%02X" ENDLINE, mts.mes.cin);
    TEST_ASSERT(mts.mes.byte1 == MIDI_CIN_NOTEON << 4);
    mm.byte3 = 0x0;
    TEST_ASSERT(MIDI_RET_OK == midiWrite(mm));
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mts));
    TEST_ASSERT(mts.mes.cin == MIDI_CIN_NOTEOFF);
    TEST_ASSERT(mts.mes.byte1 == MIDI_CIN_NOTEOFF << 4);

    TEST_ASSERT(MIDI_RET_OK == midiTsWrite(mm, 0x5758595A));
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mts));
    TEST_ASSERT(mts.timestamp == 0x5758595A);

    // midiInit();
    mm.byte3 = 16;
    TEST_ASSERT(MIDI_RET_OK == midiNonSysexWrite(mm));
    mm.byte3 = 17;
    TEST_ASSERT(MIDI_RET_OK == midiNonSysexWrite(mm));
    mm.byte3 = 18;
    TEST_ASSERT(MIDI_RET_OK == midiNonSysexWrite(mm));
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mts));
    TEST_ASSERT(16 == mts.mes.byte3);
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mts));
    TEST_ASSERT(17 == mts.mes.byte3);
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mts));
    TEST_ASSERT(18 == mts.mes.byte3);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mts));

    TEST_ASSERT(MIDI_RET_OK == midiNonSysexWrite(mm));
    midiFlush();
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mts));
    
    TEST_TODO("atomic was not tested!");
    // TODO noteoff
}

#include "../midi.h"
#include "../midi_input.h"
#include "test.h"

static void mainSysexTests()
{
    TEST_NEW_BLOCK("sysex input buffer");
    midiInit();
    MidiMessageT mm;
    MidiMessageT ms;
    MidiMessageT mts;
    MidiTsMessageT mrts;
    test_clock = 0x11223344;
    mm.cin = MIDI_CIN_POLYKEYPRESS;
    mm.cn = MIDI_CN_TEST1;
    mm.byte1 = mm.cin << 4;
    mm.byte2 = 0x11;
    mm.byte3 = 0x12;

    ms.cin = MIDI_CIN_SYSEX3BYTES;
    ms.cn = MIDI_CN_TEST1;
    ms.byte1 = 0xF0; // and what if not??
    ms.byte2 = 0x11;
    ms.byte3 = 0x12;

    TEST_ASSERT(MIDI_RET_OK == midiWrite(ms)); // first syx started
    ms.cn = MIDI_CN_TEST2;
    TEST_ASSERT(MIDI_RET_FAIL == midiWrite(ms)); // syx from different src failed
    ms.cn = MIDI_CN_TEST1;
    ms.cin = MIDI_CIN_SYSEXEND3;
    ms.byte1 = 0x11;
    ms.byte2 = 0x19;
    ms.byte3 = 0xF7; // and what if not??
    TEST_ASSERT(MIDI_RET_OK == midiWrite(ms)); // first syx ended
    // now previous message ended and it is possible receive other sources
    ms.cn = MIDI_CN_TEST2;
    TEST_ASSERT(MIDI_RET_OK == midiWrite(ms)); // syx from different src ok
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mrts));
    TEST_ASSERT(midiSysexUtilisationGet() == 3);
    TEST_ASSERT(MIDI_RET_OK == midiSysexRead(&mts));
    TEST_ASSERT(mts.cin == MIDI_CIN_SYSEX3BYTES);
    TEST_ASSERT(mts.cn == MIDI_CN_TEST1);
    TEST_ASSERT(mts.byte1 == 0xF0);
    TEST_ASSERT(mts.byte2 == 0x11);
    TEST_ASSERT(mts.byte3 == 0x12);
    TEST_ASSERT(midiSysexUtilisationGet() == 2);
    TEST_ASSERT(MIDI_RET_OK == midiSysexRead(&mts));
    TEST_ASSERT(mts.cin == MIDI_CIN_SYSEXEND3);
    TEST_ASSERT(mts.cn == MIDI_CN_TEST1);
    TEST_ASSERT(mts.byte1 == 0x11);
    TEST_ASSERT(mts.byte2 == 0x19);
    TEST_ASSERT(mts.byte3 == 0xF7);
    TEST_ASSERT(midiSysexUtilisationGet() == 1);
    TEST_ASSERT(MIDI_RET_OK == midiSysexRead(&mts));
    TEST_ASSERT(mts.cin == MIDI_CIN_SYSEXEND3);
    TEST_ASSERT(mts.cn == MIDI_CN_TEST2);
    TEST_ASSERT(mts.byte1 == 0x11);
    TEST_ASSERT(mts.byte2 == 0x19);
    TEST_ASSERT(mts.byte3 == 0xF7);
    TEST_ASSERT(midiSysexUtilisationGet() == 0);
    TEST_ASSERT(MIDI_RET_FAIL == midiSysexRead(&mts));

    // overflow
    ms.cin = MIDI_CIN_SYSEX3BYTES;
    for (int i = 0; i < 300; i++) {
        ms.byte2 = i & 0x7F;
        ms.byte3 = (i >> 1) & 0x7F;
        TEST_ASSERT_int(((i < 255) ? MIDI_RET_OK : MIDI_RET_FAIL) == midiWrite(ms), i);
        TEST_ASSERT_int(midiSysexUtilisationGet() == ((i < 255) ? i + 1 : 255), i);
    }
    for (int i = 0; i < 255; i++) {
        TEST_ASSERT_int(MIDI_RET_OK == midiSysexRead(&mts), i);
        TEST_ASSERT_int(midiSysexUtilisationGet() == 254 - i, i);
        // printf("%d - %d" ENDLINE, mts.byte2, mts.byte3);
        TEST_ASSERT_int(mts.byte2 == (i & 0x7F), i);
        TEST_ASSERT_int(mts.byte3 == ((i >> 1) & 0x7F), i);
    }
    TEST_ASSERT(midiSysexUtilisationGet() == 0);
    TEST_ASSERT(MIDI_RET_FAIL == midiSysexRead(&mts));
    
    // buffer cycling
    for (int i = 0; i < 800; i++) {
        ms.byte2 = (i ^ 69) & 0x7F;
        ms.byte3 = i & 0x7F;
        TEST_ASSERT_int(MIDI_RET_OK == midiWrite(ms), i);
        TEST_ASSERT_int(MIDI_RET_OK == midiSysexRead(&mts), i);
        TEST_ASSERT(mts.byte2 == ms.byte2);
        TEST_ASSERT(mts.byte3 == ms.byte3);
    }

    TEST_ASSERT(MIDI_RET_FAIL == midiSysexRead(&mts));
    TEST_ASSERT(MIDI_RET_OK == midiWrite(ms));
    TEST_ASSERT(MIDI_RET_OK == midiWrite(ms));
    midiSysexFlush();
    TEST_ASSERT(MIDI_RET_FAIL == midiSysexRead(&mts));

    TEST_TODO("sysex write while being locked and released leads to corrupted message (only last part of the sequence)");
    TEST_TODO("atomic was not tested!");

    // TODO!!!
}

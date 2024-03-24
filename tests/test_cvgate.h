#include "../midi_cvgate.h"
#include "test.h"
#include <stdlib.h> // abs

#define CVOUT_PROC_CYC(ms)                 \
    for (unsigned cr = 0; cr < ms; cr++) { \
        midiCvTap(&test_cvout);            \
    }

#define CVOUT_PROC_PRINT_CYC(ms)                        \
    for (unsigned cr = 0; cr < ms; cr++) {              \
        midiCvTap(&test_cvout);                         \
        printf("debug %4d: %04X %04X %04X" ENDLINE, cr, \
            test_cvout.out[MIDI_CV_CH_PITCH] >> 1,      \
            test_cvout.out[MIDI_CV_CH_VELO] >> 1,       \
            test_cvout.out[MIDI_CV_CH_VELO_LAST] >> 1); \
    }

#define PRINT_NOTEMEM()                                      \
    printf("note : ");                                       \
    for (unsigned nn = 0; nn < NOTE_HOLD_MAX_MEMORY; nn++) { \
        printf(" %02X", test_cvout.notememory[nn].note);     \
    }                                                        \
    printf(ENDLINE);                                         \
    printf("velo : ");                                       \
    for (unsigned nn = 0; nn < NOTE_HOLD_MAX_MEMORY; nn++) { \
        printf(" %02X", test_cvout.notememory[nn].velocity); \
    }                                                        \
    printf(ENDLINE);

static void testCvgate()
{
    TEST_NEW_BLOCK("cv gate");

    MidiCvOutVoice test_cvout;
    MidiMessageT m;
    m.cn = m.midichannel = 0;
    midiCvInit(&test_cvout);
    CVOUT_PROC_CYC(10)

    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_PITCH]);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_VELO]);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_VELO_LAST]);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_AFTERTOUCH]);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_MODWHEEL]);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_BREATH]);
    // printf("debug: %04X" ENDLINE, test_cvout.out[MIDI_CV_CH_MODWHEEL]);
    // printf("debug: %04X" ENDLINE, test_cvout.out[MIDI_CV_CH_BREATH]);

    // breath
    midiCvTap(&test_cvout);
    m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.byte2 = 0x2;
    m.byte3 = 0x15;
    midiCvHandleMessage(&test_cvout, m);
    CVOUT_PROC_CYC(1)
    TEST_ASSERT((0x15 << 9) == test_cvout.out[MIDI_CV_CH_BREATH]);
    // printf("debug: %04X" ENDLINE, test_cvout.out[MIDI_CV_CH_BREATH]);

    m.byte2 = 0x22;
    m.byte3 = 0x16;
    midiCvHandleMessage(&test_cvout, m);
    CVOUT_PROC_CYC(1)
    TEST_ASSERT(((0x15 << 9) | (0x16 << 2)) == test_cvout.out[MIDI_CV_CH_BREATH]);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_PITCH]);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_VELO]);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_VELO_LAST]);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_AFTERTOUCH]);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_MODWHEEL]);

    // modwh
    midiCvTap(&test_cvout);
    m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.byte2 = 0x1;
    m.byte3 = 0x39;
    midiCvHandleMessage(&test_cvout, m);
    CVOUT_PROC_CYC(1)
    TEST_ASSERT((0x39 << 9) == test_cvout.out[MIDI_CV_CH_MODWHEEL]);
    m.byte2 = 0x21;
    m.byte3 = 0x44;
    midiCvHandleMessage(&test_cvout, m);
    CVOUT_PROC_CYC(1)
    TEST_ASSERT(((0x39 << 9) | (0x44 << 2)) == test_cvout.out[MIDI_CV_CH_MODWHEEL]);

    // AT
    midiCvTap(&test_cvout);
    m.cin = m.miditype = MIDI_CIN_CHANNELPRESSURE;
    m.byte2 = 0x28;
    m.byte3 = 0;
    midiCvHandleMessage(&test_cvout, m);
    CVOUT_PROC_CYC(1)
    TEST_ASSERT((0x28 << 9) == test_cvout.out[MIDI_CV_CH_AFTERTOUCH]);
    m.byte2 = 0x29;
    midiCvHandleMessage(&test_cvout, m);
    CVOUT_PROC_CYC(1)
    TEST_ASSERT((0x29 << 9) == test_cvout.out[MIDI_CV_CH_AFTERTOUCH]);

    MidiMessageT mm[] = {
        { .cin = MIDI_CIN_NOTEON, .miditype = MIDI_CIN_NOTEON, .byte2 = 0x23, .byte3 = 0x33 }, // press 23
        { .cin = MIDI_CIN_NOTEON, .miditype = MIDI_CIN_NOTEON, .byte2 = 0x26, .byte3 = 0x36 }, // press 26
        { .cin = MIDI_CIN_NOTEON, .miditype = MIDI_CIN_NOTEON, .byte2 = 0x29, .byte3 = 0x39 }, // press 29
        { .cin = MIDI_CIN_NOTEON, .miditype = MIDI_CIN_NOTEON, .byte2 = 0x32, .byte3 = 0x42 }, // press 32
        { .cin = MIDI_CIN_NOTEOFF, .miditype = MIDI_CIN_NOTEOFF, .byte2 = 0x26, .byte3 = 0x36 }, // release 26
        { .cin = MIDI_CIN_NOTEOFF, .miditype = MIDI_CIN_NOTEOFF, .byte2 = 0x32, .byte3 = 0x42 }, // release 32
        { .cin = MIDI_CIN_NOTEON, .miditype = MIDI_CIN_NOTEON, .byte2 = 0x35, .byte3 = 0x45 }, // press 35
        { .cin = MIDI_CIN_NOTEOFF, .miditype = MIDI_CIN_NOTEOFF, .byte2 = 0x35, .byte3 = 0x45 }, // release 35
        { .cin = MIDI_CIN_NOTEOFF, .miditype = MIDI_CIN_NOTEOFF, .byte2 = 0x23, .byte3 = 0x33 }, // release 23
        { .cin = MIDI_CIN_NOTEOFF, .miditype = MIDI_CIN_NOTEOFF, .byte2 = 0x29, .byte3 = 0x39 }, // release 29
    };
    int pm[] = { 0x23, 0x26, 0x29, 0x32, 0x32, 0x29, 0x35, 0x29, 0x29, 0x29 };
    int vm[] = { 0x33, 0x36, 0x39, 0x42, 0x42, 0x39, 0x45, 0x39, 0x39, 0x0 };
    int lvm[] = { 0x33, 0x36, 0x39, 0x42, 0x42, 0x42, 0x45, 0x45, 0x45, 0x0 };

    // PRINT_NOTEMEM();
    for (unsigned i = 0; i < sizeof(mm) / sizeof(MidiMessageT); i++) {
        midiCvHandleMessage(&test_cvout, mm[i]);
        CVOUT_PROC_CYC(10)
        TEST_ASSERT(abs((pm[i] << 9) - test_cvout.out[MIDI_CV_CH_PITCH]) < 2);
        TEST_ASSERT(abs((vm[i] << 9) - test_cvout.out[MIDI_CV_CH_VELO]) < 2);
        TEST_ASSERT(abs((lvm[i] << 9) - test_cvout.out[MIDI_CV_CH_VELO_LAST]) < 2);
        // PRINT_NOTEMEM();
        // printf("debug: %04X %04X %04X %04X %d" ENDLINE,
        //     test_cvout.out[MIDI_CV_CH_PITCH] >> 1,
        //     test_cvout.out[MIDI_CV_CH_VELO] >> 1,
        //     test_cvout.out[MIDI_CV_CH_VELO_LAST] >> 1,
        //     test_cvout.pitch_goal, test_cvout.keycount);
    }

#ifdef MIDI_CV_RETRIG_OPTION
    m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.byte2 = 0x7f;
    // test_cvout.gateretrig = 1;
    midiCvHandleMessage(&test_cvout, m); // switch to polyphonic

    for (unsigned i = 0; i < sizeof(mm) / sizeof(MidiMessageT); i++) {
        midiCvHandleMessage(&test_cvout, mm[i]);
        CVOUT_PROC_CYC(1)
        if (i != 0) {
            TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_VELO]);
            TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_VELO_LAST]);
        }
        CVOUT_PROC_CYC(1)
        if (i != 0) {
            TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_VELO]);
            TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_VELO_LAST]);
        }
        CVOUT_PROC_CYC(1)
        if (i < sizeof(mm) / sizeof(MidiMessageT) - 1) {
            TEST_ASSERT(0 != test_cvout.out[MIDI_CV_CH_VELO]);
            TEST_ASSERT(0 != test_cvout.out[MIDI_CV_CH_VELO_LAST]);
        }
        // CVOUT_PROC_PRINT_CYC(10, MIDI_CV_CH_VELO);
        CVOUT_PROC_CYC(10)
        // TEST_ASSERT(abs((pm[i] << 9) - test_cvout.out[MIDI_CV_CH_PITCH]) < 2);
        // TEST_ASSERT(abs((vm[i] << 9) - test_cvout.out[MIDI_CV_CH_VELO]) < 2);
        // TEST_ASSERT(abs((lvm[i] << 9) - test_cvout.out[MIDI_CV_CH_VELO_LAST]) < 2);
        // PRINT_NOTEMEM();
        // printf("debug: %04X %04X %04X %04X %d" ENDLINE,
        //     test_cvout.out[MIDI_CV_CH_PITCH] >> 1,
        //     test_cvout.out[MIDI_CV_CH_VELO] >> 1,
        //     test_cvout.out[MIDI_CV_CH_VELO_LAST] >> 1,
        //     test_cvout.pitch_goal, test_cvout.keycount);
    }

    m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.byte2 = 0x7e;
    midiCvHandleMessage(&test_cvout, m); // switch back to mono
    for (unsigned i = 0; i < sizeof(mm) / sizeof(MidiMessageT); i++) {
        midiCvHandleMessage(&test_cvout, mm[i]);
        CVOUT_PROC_CYC(1)
        if (i < sizeof(mm) / sizeof(MidiMessageT) - 1) {
            TEST_ASSERT_int(0 != test_cvout.out[MIDI_CV_CH_VELO], i);
            TEST_ASSERT_int(0 != test_cvout.out[MIDI_CV_CH_VELO_LAST], i);
        }
        // printf("debug: %04X %04X %04X %04X %d" ENDLINE,
        //     test_cvout.out[MIDI_CV_CH_PITCH] >> 1,
        //     test_cvout.out[MIDI_CV_CH_VELO] >> 1,
        //     test_cvout.out[MIDI_CV_CH_VELO_LAST] >> 1,
        //     test_cvout.pitch_goal, test_cvout.keycount);
        // CVOUT_PROC_PRINT_CYC(10, MIDI_CV_CH_VELO);
        CVOUT_PROC_CYC(10)
    }

    m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.byte2 = 0x7f;
    midiCvHandleMessage(&test_cvout, m); // switch to poly again
    m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.byte2 = 0x40;
    m.byte3 = 0x7F;
    midiCvHandleMessage(&test_cvout, m); // hold pedal
    for (unsigned i = 0; i < sizeof(mm) / sizeof(MidiMessageT); i++) {
        midiCvHandleMessage(&test_cvout, mm[i]);
        CVOUT_PROC_CYC(1)
        if (i < sizeof(mm) / sizeof(MidiMessageT) - 1) {
            TEST_ASSERT_int(0 != test_cvout.out[MIDI_CV_CH_VELO], i);
            TEST_ASSERT_int(0 != test_cvout.out[MIDI_CV_CH_VELO_LAST], i);
        }
        CVOUT_PROC_CYC(10)
    }

    m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.byte2 = 0x40;
    m.byte3 = 0x0;
    midiCvHandleMessage(&test_cvout, m); // hold pedal
    for (unsigned i = 0; i < sizeof(mm) / sizeof(MidiMessageT); i++) {
        midiCvHandleMessage(&test_cvout, mm[i]);
        CVOUT_PROC_CYC(1)
        if (i != 0) {
            TEST_ASSERT_int(0 == test_cvout.out[MIDI_CV_CH_VELO], i);
            TEST_ASSERT_int(0 == test_cvout.out[MIDI_CV_CH_VELO_LAST], i);
        }
        CVOUT_PROC_CYC(10)
    }
#endif // MIDI_CV_RETRIG_OPTION

    // TEST_TODO("pitchwheel");
    m.cin = m.miditype = MIDI_CIN_NOTEON;
    m.byte2 = m.byte3 = 0x40;
    midiCvHandleMessage(&test_cvout, m);
    m.cin = m.miditype = MIDI_CIN_NOTEOFF;
    midiCvHandleMessage(&test_cvout, m);
    CVOUT_PROC_CYC(10)
    TEST_ASSERT(abs((0x40 << 9) - test_cvout.out[MIDI_CV_CH_PITCH]) < 2);
    // printf("debug: %04X" ENDLINE, test_cvout.out[MIDI_CV_CH_PITCH]);
    // default range is octave up - octave dwn
    m.cin = m.miditype = MIDI_CIN_PITCHBEND;
    for (int i = 0; i < 128; i++) {
        m.byte2 = 0;
        m.byte3 = i;
        midiCvHandleMessage(&test_cvout, m);
        CVOUT_PROC_CYC(1)
        int shift = (12 << 9) * (i - 64) / 64;
        TEST_ASSERT_int(abs((0x40 << 9) + shift - test_cvout.out[MIDI_CV_CH_PITCH]) < 2, i);
    }
    // change range to 2 semi
    m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.byte2 = 0x3;
    m.byte3 = 4; // 2
    midiCvHandleMessage(&test_cvout, m);
    m.cin = m.miditype = MIDI_CIN_PITCHBEND;
    for (int i = 0; i < 128; i++) {
        m.byte2 = 0;
        m.byte3 = i;
        midiCvHandleMessage(&test_cvout, m);
        CVOUT_PROC_CYC(1)
        int shift = (2 << 9) * (i - 64) / 64;
        TEST_ASSERT_int(abs((0x40 << 9) + shift - test_cvout.out[MIDI_CV_CH_PITCH]) < 2, i);
    }
    // saturation low
    m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.byte2 = 0x3;
    m.byte3 = 126; // 2
    midiCvHandleMessage(&test_cvout, m);
    m.cin = m.miditype = MIDI_CIN_NOTEON;
    m.byte2 = m.byte3 = 0x20;
    midiCvHandleMessage(&test_cvout, m);
    m.cin = m.miditype = MIDI_CIN_NOTEOFF;
    midiCvHandleMessage(&test_cvout, m);
    m.cin = m.miditype = MIDI_CIN_PITCHBEND;
    for (int i = 0; i < 128; i++) {
        m.byte2 = 0;
        m.byte3 = i;
        midiCvHandleMessage(&test_cvout, m);
        CVOUT_PROC_CYC(1)
        int cv = (0x20 << 9) + (63 << 9) * (i - 64) / 64;
        TEST_ASSERT_int(abs((cv < 0 ? 0 : cv) - test_cvout.out[MIDI_CV_CH_PITCH]) < 2, i);
        // printf("debug: %04X %d" ENDLINE, test_cvout.out[MIDI_CV_CH_PITCH], i);
    }
    m.cin = m.miditype = MIDI_CIN_NOTEON;
    m.byte2 = m.byte3 = 0x60;
    midiCvHandleMessage(&test_cvout, m);
    m.cin = m.miditype = MIDI_CIN_NOTEOFF;
    midiCvHandleMessage(&test_cvout, m);
    m.cin = m.miditype = MIDI_CIN_PITCHBEND;
    for (int i = 0; i < 128; i++) {
        m.byte2 = 0;
        m.byte3 = i;
        midiCvHandleMessage(&test_cvout, m);
        CVOUT_PROC_CYC(1)
        int cv = (0x60 << 9) + (63 << 9) * (i - 64) / 64;
        TEST_ASSERT_int(abs((cv > 0xFFFF ? 0xFFFF : cv) - test_cvout.out[MIDI_CV_CH_PITCH]) < 2, i);
        // printf("debug: %04X %d" ENDLINE, test_cvout.out[MIDI_CV_CH_PITCH], i);
    }
    // lsb
    for (int i = 0; i < 128; i++) {
        m.byte2 = i;
        m.byte3 = 0x40;
        midiCvHandleMessage(&test_cvout, m);
        CVOUT_PROC_CYC(1)
        int cv = (0x60 << 9) + (63 << 9) * i / 128 / 64;
        TEST_ASSERT_int(abs(cv - test_cvout.out[MIDI_CV_CH_PITCH]) < 2, i);
        // printf("debug: %04X %d" ENDLINE, test_cvout.out[MIDI_CV_CH_PITCH], i);
    }
    m.cin = m.miditype = MIDI_CIN_PITCHBEND;
    m.byte2 = 0, m.byte3 = 0x40;
    midiCvHandleMessage(&test_cvout, m);
    // printf("debug: %08X" ENDLINE, test_cvout.pwrange);

    // TEST_TODO("damper");
    CVOUT_PROC_CYC(10);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_VELO]);
    m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.byte2 = 0x40, m.byte3 = 0x7F;
    midiCvHandleMessage(&test_cvout, m);
    CVOUT_PROC_CYC(10);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_VELO]);
    m.cin = m.miditype = MIDI_CIN_NOTEON;
    m.byte2 = 0x40, m.byte3 = 0x40;
    midiCvHandleMessage(&test_cvout, m);
    CVOUT_PROC_CYC(10);
    m.cin = m.miditype = MIDI_CIN_NOTEOFF;
    m.byte2 = 0x40, m.byte3 = 0x40;
    midiCvHandleMessage(&test_cvout, m);
    CVOUT_PROC_CYC(10);
    // value not exact, because 1st NON was with damper - glide
    TEST_ASSERT(abs((0x40 << 9) - test_cvout.out[MIDI_CV_CH_VELO]) < 2);
    m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.byte2 = 0x40, m.byte3 = 0x0;
    midiCvHandleMessage(&test_cvout, m);
    CVOUT_PROC_CYC(1);
    TEST_ASSERT(0 == test_cvout.out[MIDI_CV_CH_VELO]);
    // only base test

    TEST_TODO("glide");
    if (0) { // manual test
        m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
        m.byte2 = 0x7E, m.byte3 = 0x0;
        midiCvHandleMessage(&test_cvout, m);
        m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
        m.byte2 = 0x5, m.byte3 = 0x20;
        midiCvHandleMessage(&test_cvout, m);

        m.cin = m.miditype = MIDI_CIN_NOTEON;
        m.byte2 = 0x20, m.byte3 = 0x20;
        midiCvHandleMessage(&test_cvout, m);
        CVOUT_PROC_PRINT_CYC(10);

        m.cin = m.miditype = MIDI_CIN_NOTEON;
        m.byte2 = 0x40, m.byte3 = 0x40;
        midiCvHandleMessage(&test_cvout, m);

        CVOUT_PROC_PRINT_CYC(100);
        // m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
        // m.byte2 = 0x5, m.byte3 = 0x7F;
        // midiCvHandleMessage(&test_cvout, m);

        m.cin = m.miditype = MIDI_CIN_NOTEOFF;
        m.byte2 = 0x40, m.byte3 = 0x0;
        midiCvHandleMessage(&test_cvout, m);
        CVOUT_PROC_PRINT_CYC(100);

        m.cin = m.miditype = MIDI_CIN_NOTEOFF;
        m.byte2 = 0x0, m.byte3 = 0x0;
        midiCvHandleMessage(&test_cvout, m);
    }

    // printf("debug: %04X" ENDLINE, test_cvout.out[MIDI_CV_CH_VELO]);
    // CVOUT_PROC_PRINT_CYC(10, MIDI_CV_CH_VELO);
}

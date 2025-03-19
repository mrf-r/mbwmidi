#include "../../tools/potproc.h"
#include "../test.h"

// we need to test our fancy filter and pots sources merging and locking logic
// unfortunately, the filter tested only manually so far
int32_t lcg = 678876; // just some feed number
int32_t pot_flt;
#define TEST_FILTER_TAPS 3000

static int16_t testPotOneTap(int16_t adc, unsigned bits)
{
    lcg = lcg * 0x41C64E6D + 0x3039;
    int16_t adc_noise = lcg >> 14;
    adc_noise /= 16394; // incorrect noise, not for edges
    // return potFilterCompensated(&pot_flt, adc + adc_noise, lcg, bits);
    return potFilterCompensated(&pot_flt, adc, lcg, bits);
    // printf("%08X\n", pot_flt); // HUGE OUTPUT
}

static void potFilterTest()
{
    TEST_NEW_BLOCK("Pot Filter Compensated");
    // check that after 5 seconds value good
    unsigned bits = 9;
    int16_t adc = (1 << bits) - 1;
    pot_flt = 0;
    for (unsigned i = 0; i < TEST_FILTER_TAPS; i++) {
        int32_t result = testPotOneTap(adc, bits);
        TEST_ASSERT(result <= 0x3FFF);
        // printf("%08X - %04X\n", pot_flt, result);
    }
    TEST_ASSERT(0x3FFF == testPotOneTap(adc, bits));
    // printf("%04X\n", testPotOneTap(adc, bits));
    adc = 0;
    for (unsigned i = 0; i < TEST_FILTER_TAPS; i++) {
        int32_t result = testPotOneTap(adc, bits);
        TEST_ASSERT(result >= 0);
        // printf("%08X - %04X\n", pot_flt, result);
    }
    TEST_ASSERT(0 == testPotOneTap(adc, bits));

    TEST_TODO("mid-scale tests with various ADC noise levels");
}

static void potLockTest()
{
    TEST_NEW_BLOCK("Pot Locks");
    PotProcData pd;
    potInit(&pd, 0, 0);
    // non - midi ===================================
    int potposition = 0;
    for (; potposition < 100; potposition++) {
        potProcessLocalValue(&pd, potposition);
        TEST_ASSERT(potGetValue(&pd) == potposition);
    }
    // just lock
    potposition = 0;
    potProcessLocalValue(&pd, potposition);
    potLockOnCurrentValue(&pd);
    for (; potposition < 300; potposition += 2) {
        potProcessLocalValue(&pd, potposition);
        if (potposition <= 128) {
            TEST_ASSERT(potGetValue(&pd) == 0);
        } else {
            TEST_ASSERT(potGetValue(&pd) == potposition);
        }
    }
    // lock on far left
    potposition = 0;
    potProcessLocalValue(&pd, potposition);
    potLockOnIncomingValue(&pd, 800);
    for (; potposition < 1000; potposition += 10) {
        potProcessLocalValue(&pd, potposition);
        if (potposition <= 800 - 64) {
            uint16_t result = potGetValue(&pd);
            TEST_ASSERT(result == 800);
            // printf("%d: %d\n", potposition, result);

        } else {
            TEST_ASSERT(potGetValue(&pd) == potposition);
        }
    }
    // lock on far right
    potposition = 2000;
    potProcessLocalValue(&pd, potposition);
    potLockOnIncomingValue(&pd, 800);
    for (; potposition > 100; potposition -= 10) {
        potProcessLocalValue(&pd, potposition);
        if (potposition >= 800 + 64) {
            uint16_t result = potGetValue(&pd);
            TEST_ASSERT(result == 800);
            // printf("%d: %d\n", potposition, result);

        } else {
            TEST_ASSERT(potGetValue(&pd) == potposition);
        }
    }
    // lock on same
    potposition = 2000;
    potProcessLocalValue(&pd, potposition);
    potLockOnIncomingValue(&pd, 2002);
    for (; potposition < 2200; potposition++) {
        potProcessLocalValue(&pd, potposition);
        if (potposition <= 2000 + 128) {
            uint16_t result = potGetValue(&pd);
            TEST_ASSERT(result == 2002);
            // printf("%d: %d\n", potposition, result);
        } else {
            TEST_ASSERT(potGetValue(&pd) == potposition);
        }
    }
    // =========================================================
    // lsb follow logic
    potposition = 0x2035;
    potProcessLocalValue(&pd, potposition);
    potReceiveLsb(&pd, 0x46);
    TEST_ASSERT(0x2046 == potGetValue(&pd));
    potReceiveLsb(&pd, 0x24);
    TEST_ASSERT(0x2024 == potGetValue(&pd));
    potReceiveMsb(&pd, 0x42);
    TEST_ASSERT(0x2100 == potGetValue(&pd));
    potReceiveMsb(&pd, 0x40);
    TEST_ASSERT(0x207F == potGetValue(&pd));
    // =========================================================
    // midi send
    MidiTsMessageT mt;
    MidiMessageT m = { 0 };
    m.cn = MIDI_CN_TEST0;
    // m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.cin = MIDI_CIN_CONTROLCHANGE;
    m.miditype = MIDI_CIN_CONTROLCHANGE;
    m.byte2 = 0x10;
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mt));
    // printf("%08X\n", m.full_word);
    potInit(&pd, 0x1F4, 2);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mt));
    potProcessLocalValueWithMidiSend(&pd, m, 0x1F5);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mt));
    potProcessLocalValueWithMidiSend(&pd, m, 0x1f6);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mt));
    potProcessLocalValueWithMidiSend(&pd, m, 0x1f7);
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mt));
    // printf("%08X\n", mt.mes.full_word);
    TEST_ASSERT(mt.mes.full_word == 0x7730B00B);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mt));
    potProcessLocalValueWithMidiSend(&pd, m, 0x1f8);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mt));
    potProcessLocalValueWithMidiSend(&pd, m, 0x1f9);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mt));
    potProcessLocalValueWithMidiSend(&pd, m, 0x1fA);
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mt));
    // printf("%08X\n", mt.mes.full_word);
    TEST_ASSERT(mt.mes.full_word == 0x7A30B00B);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mt));
    potProcessLocalValueWithMidiSend(&pd, m, 0x1355);
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mt));
    printf("%08X\n", mt.mes.full_word);
    TEST_ASSERT(mt.mes.full_word == 0x2610B00B);
    TEST_ASSERT(MIDI_RET_OK == midiRead(&mt));
    // printf("%08X\n", mt.mes.full_word);
    TEST_ASSERT(mt.mes.full_word == 0x5530B00B);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mt));
    potMidiThresholdSet(&pd, 127);
    potProcessLocalValueWithMidiSend(&pd, m, 0x13AA);
    TEST_ASSERT(MIDI_RET_FAIL == midiRead(&mt));
}
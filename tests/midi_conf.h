#ifndef _MIDI_CONF_H
#define _MIDI_CONF_H

// TESTING CONFIG

#include <stdint.h>
#include <assert.h>
#define MIDI_ASSERT assert

#define MIDI_ATOMIC_START()
#define MIDI_ATOMIC_END()

#define TEST_SAMPLE_RATE 1

extern volatile uint32_t test_clock;

#define MIDI_GET_CLOCK() (test_clock)
#define MIDI_GET_CLOCK_RATE (TEST_SAMPLE_RATE)


// maximum 16 cables can be defined !!!!
typedef enum {
    MIDI_CN_TEST0 = 0,
    MIDI_CN_TEST1,
    MIDI_CN_TEST2,
    MIDI_CN_TEST3,
    MIDI_CN_TOTAL
} MidiCnEn;


#endif // _MIDI_CONF_H

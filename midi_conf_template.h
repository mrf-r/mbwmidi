#ifndef _MIDI_CONF_H
#define _MIDI_CONF_H

// TEMPLATE

#include <stdint.h>
#include <assert.h>
#define MIDI_ASSERT assert

#define SAMPLE_RATE 48000
// #define CONTROL_RATE 1500

extern volatile uint32_t counter_sr;
// extern volatile uint32_t counter_cr;

// TODO: use this
#define MIDI_GET_CLOCK() (counter_sr)
#define MIDI_CLOCK_RATE (SAMPLE_RATE)


#endif // _MIDI_CONF_H

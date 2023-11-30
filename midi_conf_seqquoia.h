#ifndef _MIDI_CONF_H
#define _MIDI_CONF_H

// SEQUOIA CONFIG

#include <stdint.h>
#include <assert.h>

#define MIDI_ASSERT assert
#define MIDI_DEBUG DEBUG // TODO do we need it????

#define SAMPLE_RATE 48000
#define CONTROL_RATE 1500

extern volatile uint32_t counter_cr;
extern volatile uint32_t counter_sr;

// TODO: use this
#define MIDI_GET_CLOCK() (counter_sr)
#define MIDI_GET_CLOCK_RATE() (SAMPLE_RATE)


#endif // _MIDI_CONF_H
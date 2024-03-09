#ifndef _MIDI_CONF_H
#define _MIDI_CONF_H

// TEMPLATE

#include <stdint.h>
#include <assert.h>
#define MIDI_ASSERT assert

#define SAMPLE_RATE 48000
#define CONTROL_RATE 1500 // for CV-GATE converter only

extern volatile uint32_t counter_sr;
// extern volatile uint32_t counter_cr;

// TODO: use this
#define MIDI_GET_CLOCK() (counter_sr)
#define MIDI_CLOCK_RATE (SAMPLE_RATE)

extern volatile unsigned atomic_level;
static inline void _atomicStart()
{
    MIDI_ASSERT(atomic_level < 2);
    // TODO: should we get current irq state ?
    __disable_irq();
    atomic_level++;
}
static inline void _atomicEnd()
{
    MIDI_ASSERT(atomic_level);
    atomic_level--;
    if (0 == atomic_level) {
        __enable_irq();
    }
}

#define MIDI_ATOMIC_START _atomicStart
#define MIDI_ATOMIC_END _atomicEnd

// #define MIDI_UART_NOTEOFF_VELOCITY_MATTERS // it will break Running Status more often

#endif // _MIDI_CONF_H

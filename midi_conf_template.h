/*
Copyright (C) 2024 Eugene Chernyh (mrf-r)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _MIDI_CONF_H
#define _MIDI_CONF_H

// TEMPLATE

#include <stdint.h>
#include <assert.h>
#define MIDI_ASSERT assert

#define SAMPLE_RATE 48000

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

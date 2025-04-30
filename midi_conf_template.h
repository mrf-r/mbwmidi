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

typedef enum {
    MIDI_CN_LOCALPANEL = 0,
    MIDI_CN_USB_DEVICE,
    MIDI_CN_USB_HOST,
    MIDI_CN_UART1,
    MIDI_CN_UART2,
    MIDI_CN_UART3,
    MIDI_CN_TOTAL,
} MidiCnEn;

#if MIDI_CN_TOTAL > 16
#error "too much input IDs"
#endif

#define SAMPLE_RATE 48000

extern volatile uint32_t counter_sr;

#define MIDI_GET_CLOCK() (counter_sr)
#define MIDI_CLOCK_RATE (SAMPLE_RATE)

extern volatile unsigned midi_atomic_level;

// since the midi library also assumes operation inside an interrupt, it is
// necessary to choose one of the ways to ensure atomicity. The library itself
// has reenterancy, so it is necessary to monitor the depth

// !!! proposed, was not tested on hw !!!

// good, but you must understand how this mechanism works and keep all your
// interrupt priorities under control
static inline void _midiAtomicStartBasepri()
{
    MIDI_ASSERT((midi_atomic_level == 0) ? (0 == __get_BASEPRI()) : (BSP_IRQPRIO_MIDI == __get_BASEPRI()));
    MIDI_ASSERT(midi_atomic_level < 2);
    // UART MIDI probably should have higher prio than USB and anything else, that uses midi
    __set_BASEPRI(BSP_IRQPRIO_MIDI);
    midi_atomic_level++;
}
static inline void _midiAtomicEndBasepri()
{
    MIDI_ASSERT(midi_atomic_level);
    midi_atomic_level--;
    if (0 == midi_atomic_level) {
        __set_BASEPRI(0);
    }
}

// a bit slower, but ok
static inline void _midiAtomicStartCustomIrq()
{
    // MIDI_ASSERT((midi_atomic_level == 0) ? (1 == NVIC_GetEnableIRQ(USART1_IRQn)) : (0 == NVIC_GetEnableIRQ(USART1_IRQn)));
    MIDI_ASSERT(midi_atomic_level < 2);
    // it is necessary to disable all interrupts that use midi
    NVIC_DisableIRQ(USART1_IRQn);
    NVIC_DisableIRQ(OTG_FS_IRQn);
    // NVIC_DisableIRQ(I2C1_EV_IRQn);
    midi_atomic_level++;
}
static inline void _midiAtomicEndCustomIrq()
{
    MIDI_ASSERT(midi_atomic_level);
    midi_atomic_level--;
    if (0 == midi_atomic_level) {
        NVIC_EnableIRQ(USART1_IRQn);
        NVIC_EnableIRQ(OTG_FS_IRQn);
    }
}

// not cool, because it disables all interrupts. can be used in systems where
// it is not critical, but if there are other interrupts with strict response
// requirements, it is better to use other methods
static inline void _midiAtomicStartSimple()
{
    MIDI_ASSERT((midi_atomic_level == 0) ? (0 == __get_PRIMASK()) : (1 == __get_PRIMASK()));
    MIDI_ASSERT(midi_atomic_level < 2);
    __disable_irq();
    midi_atomic_level++;
}
static inline void _midiAtomicEndSimple()
{
    MIDI_ASSERT(midi_atomic_level);
    midi_atomic_level--;
    if (0 == midi_atomic_level) {
        __enable_irq();
    }
}

#define MIDI_ATOMIC_START _midiAtomicStart
#define MIDI_ATOMIC_END _midiAtomicEnd

#endif // _MIDI_CONF_H

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

#ifndef _POTPROC_H
#define _POTPROC_H

#include <stdint.h>
#include "midi_input.h"

typedef enum {
    POT_STATE_NORMAL = 0,
    POT_STATE_LOCK_INSIDE,
    POT_STATE_LOCK_HIGH,
    POT_STATE_LOCK_LOW,
} PotStateEn;

#ifndef POT_LOCK_THRSH
#define POT_LOCK_THRSH (128)
#endif

typedef struct {
    uint16_t current; // midi 14 bit
    uint16_t locked; // midi 14 bit
    uint8_t state; // PotStateEn
    uint8_t threshold;
} PotProcData;

// adc is right aligned
static inline uint16_t potFilter(int32_t* const filter, const uint16_t adc, const uint32_t lcg, const unsigned ADC_BITS)
{
    int32_t knob = (adc << (30 - ADC_BITS)) + (lcg >> (2 + ADC_BITS));
    int32_t f = *filter;
    int32_t delta = knob - f; // delta is +/-30 bit
    delta = delta / 32768; // + (delta < 0 ? -1 : 1); // now 15 bit
    int32_t cf = delta < 0 ? -delta : delta;
    f += delta * cf;
    *filter = f;
    return f >> 16;
}

// value is 14 bit midi resolution, right aligned
static inline void potLockFetch(PotProcData* const pd, const uint16_t value)
{
    MIDI_ASSERT(value < 0x4000);
    uint16_t current = pd->current;
    int delta = current - value;
    int d = delta < 0 ? -delta : delta;
    if (d > POT_LOCK_THRSH) {
        pd->locked = value;
        if (value < current) {
            pd->state = POT_STATE_LOCK_LOW;
        } else {
            pd->state = POT_STATE_LOCK_HIGH;
        }
    } else {
        pd->locked = current;
        pd->state = POT_STATE_LOCK_INSIDE;
    }
}

static inline void potLockJump(PotProcData* const pd)
{
    pd->locked = pd->current;
    pd->state = POT_STATE_LOCK_INSIDE;
}

static inline uint16_t parameterReceiveMsb(const uint16_t old_value, const uint8_t msb)
{
    MIDI_ASSERT(msb < 0x80);
    uint16_t new_value = msb << 7;
    if (new_value > old_value) {
        return new_value;
    } else if (new_value < (old_value & 0x3F80)) {
        return new_value | 0x7F;
    } else {
        return old_value;
    }
}

static inline uint16_t parameterReceiveLsb(const uint16_t old_value, const uint8_t lsb)
{
    MIDI_ASSERT(lsb < 0x80);
    return (old_value & 0x3F80) | lsb;
}


static inline void potThresholdSet(PotProcData* const pd, uint8_t const thr)
{
    MIDI_ASSERT(thr < 128);
    pd->threshold = thr;
}

static inline void potProcess(PotProcData* const pd, MidiMessageT m, uint16_t const value)
{
    int16_t current = value;
    pd->current = value;
    int16_t locked = pd->locked;
    int delta = current - locked;
    int d = delta < 0 ? -delta : delta;
    if (POT_STATE_NORMAL == pd->state) {
        if (d > pd->threshold) {
            int16_t bitdiff = current ^ locked;
            if (bitdiff >> 7) {
                m.byte3 = current >> 7;
                midiNonSysexWrite(m);
            }
            if (bitdiff & 0x7F) {
                m.byte2 += 0x20;
                m.byte3 = current & 0x7F;
                midiNonSysexWrite(m);
            }
            pd->locked = current;
        }
    } else if (
        ((POT_STATE_LOCK_INSIDE == pd->state) && (d > POT_LOCK_THRSH))
        || ((POT_STATE_LOCK_LOW == pd->state) && ((current < locked) || (d < POT_LOCK_THRSH / 2)))
        || ((POT_STATE_LOCK_HIGH == pd->state) && ((current > locked) || (d < POT_LOCK_THRSH / 2)))) {
        m.byte3 = current >> 7;
        midiNonSysexWrite(m);
        m.byte2 += 0x20;
        m.byte3 = current & 0x7F;
        midiNonSysexWrite(m);
        pd->state = POT_STATE_NORMAL;
        pd->locked = current;
    }
}

#endif // _POTPROC_H
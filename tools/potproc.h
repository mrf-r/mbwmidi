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

#ifndef POT_FILTER_FACTOR_EXP
#define POT_FILTER_FACTOR_EXP (15)
#endif

#ifndef POT_FILTER_FACTOR_LIN
#define POT_FILTER_FACTOR_LIN (1)
#endif

// all values are 14bit
typedef struct {
    uint16_t pot_position;
    uint16_t lock_n_midi_compare; // and also midi
    uint8_t state;
    uint8_t midi_threshold;
    uint16_t output;
} PotProcData;

// adc is right aligned
static inline uint16_t potFilter(int32_t* const filter, const uint16_t adc, const uint32_t lcg, const unsigned ADC_BITS)
{
    int32_t knob = (adc << (30 - ADC_BITS)) + (lcg >> (2 + ADC_BITS));
    int32_t f = *filter;
    int32_t delta = knob - f; // delta is +/-30 bit
    delta = delta / (1 << POT_FILTER_FACTOR_EXP);
    int32_t cf = delta < 0 ? -delta : delta;
    f += delta * cf / POT_FILTER_FACTOR_LIN;
    *filter = f;
    return f / 65536;
}

// less "absolute", more practical. To fill the scale edges
static inline uint16_t potFilterCompensated(int32_t* const filter, const uint16_t adc, const uint32_t lcg, const unsigned ADC_BITS)
{
    int32_t knob = adc << (30 - ADC_BITS);
    knob += adc << (30 - ADC_BITS - ADC_BITS); // fill the lowest bits
    knob += (adc << (16 - ADC_BITS)) - 0x8000; // make edges more clear
    // probably 2048 can be adjusted
    int32_t noise = (int32_t)lcg / 2048;
    knob += noise;

    int32_t f = *filter;
    int32_t delta = knob - f;
    delta = delta / (1 << POT_FILTER_FACTOR_EXP);
    int32_t cf = delta < 0 ? -delta : delta;
    f += delta * cf / POT_FILTER_FACTOR_LIN;
    *filter = f;
    f /= 65536;
    if (f < 0) {
        f = 0;
    } else if (f > 0x3FFF) {
        f = 0x3FFF;
    }
    return f;
}

static inline void potMidiThresholdSet(PotProcData* const pd, uint8_t const thr)
{
    MIDI_ASSERT(thr < 128);
    pd->midi_threshold = thr;
}

static inline void potInit(PotProcData* const pd, uint16_t value14b, uint16_t midi_threshold)
{
    pd->state = POT_STATE_NORMAL;
    pd->pot_position = pd->lock_n_midi_compare = pd->output = value14b;
    potMidiThresholdSet(pd, midi_threshold);
}

static inline uint16_t potGetValue(PotProcData* const pd)
{
    return pd->output;
}

static inline uint16_t potGetPhysicalPosition(PotProcData* const pd)
{
    return pd->pot_position;
}

static inline void potLockOnIncomingValue(PotProcData* const pd, const uint16_t value14b)
{
    MIDI_ASSERT(value14b < 0x4000);
    uint16_t pot_position = pd->pot_position;
    int delta = pot_position - value14b;
    int d = delta < 0 ? -delta : delta;
    if (d > POT_LOCK_THRSH) {
        if (value14b < pot_position) {
            pd->state = POT_STATE_LOCK_LOW;
        } else {
            pd->state = POT_STATE_LOCK_HIGH;
        }
        pd->lock_n_midi_compare = value14b;
    } else {
        pd->state = POT_STATE_LOCK_INSIDE;
        pd->lock_n_midi_compare = pot_position;
    }
    pd->output = value14b;
}

static inline void potLockOnCurrentValue(PotProcData* const pd)
{
    pd->state = POT_STATE_LOCK_INSIDE;
    pd->lock_n_midi_compare = pd->pot_position;
}

static inline void potProcessLocalValueWithMidiSend(PotProcData* const pd, MidiMessageT m, uint16_t const value14b)
{
    int16_t pot_position = value14b;
    pd->pot_position = value14b;
    int16_t lock_n_midi_compare = pd->lock_n_midi_compare;
    int delta = pot_position - lock_n_midi_compare;
    int d = delta < 0 ? -delta : delta;
    if (POT_STATE_NORMAL == pd->state) {
        if (d > pd->midi_threshold) {
            int16_t bitdiff = pot_position ^ lock_n_midi_compare;
            if (bitdiff >> 7) {
                m.byte3 = pot_position >> 7;
                midiNonSysexWrite(m);
            }
            if (bitdiff & 0x7F) {
                m.byte2 += 0x20;
                m.byte3 = pot_position & 0x7F;
                midiNonSysexWrite(m);
            }
            pd->lock_n_midi_compare = pot_position;
        }
        pd->output = pot_position;
    } else if (
        ((POT_STATE_LOCK_INSIDE == pd->state) && (d > POT_LOCK_THRSH))
        || ((POT_STATE_LOCK_LOW == pd->state) && ((pot_position < lock_n_midi_compare) || (d < POT_LOCK_THRSH / 2)))
        || ((POT_STATE_LOCK_HIGH == pd->state) && ((pot_position > lock_n_midi_compare) || (d < POT_LOCK_THRSH / 2)))) {
        pd->state = POT_STATE_NORMAL;
        m.byte3 = pot_position >> 7;
        midiNonSysexWrite(m);
        m.byte2 += 0x20;
        m.byte3 = pot_position & 0x7F;
        midiNonSysexWrite(m);
        pd->lock_n_midi_compare = pot_position;
        pd->output = pot_position;
    }
}

static inline void potProcessLocalValue(PotProcData* const pd, uint16_t const value14b)
{
    int16_t pot_position = value14b;
    pd->pot_position = value14b;
    int16_t lock_n_midi_compare = pd->lock_n_midi_compare;
    int delta = pot_position - lock_n_midi_compare;
    int d = delta < 0 ? -delta : delta;
    if (POT_STATE_NORMAL == pd->state) {
        pd->output = pot_position;
    } else if (
        ((POT_STATE_LOCK_INSIDE == pd->state) && (d > POT_LOCK_THRSH))
        || ((POT_STATE_LOCK_LOW == pd->state) && ((pot_position < lock_n_midi_compare) || (d < POT_LOCK_THRSH / 2)))
        || ((POT_STATE_LOCK_HIGH == pd->state) && ((pot_position > lock_n_midi_compare) || (d < POT_LOCK_THRSH / 2)))) {
        pd->state = POT_STATE_NORMAL;
        pd->output = pot_position;
    }
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

static inline void potReceiveMsb(PotProcData* const pd, const uint8_t msb)
{
    uint16_t pot_position = potGetValue(pd);
    uint16_t new = parameterReceiveMsb(pot_position, msb);
    potLockOnIncomingValue(pd, new);
}

static inline void potReceiveLsb(PotProcData* const pd, const uint8_t lsb)
{
    uint16_t pot_position = potGetValue(pd);
    uint16_t new = parameterReceiveLsb(pot_position, lsb);
    potLockOnIncomingValue(pd, new);
}

#endif // _POTPROC_H
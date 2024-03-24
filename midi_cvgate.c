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

#include "midi_cvgate.h"
#include "midi_cc.h"

#ifdef MIDI_CV_RETRIG_OPTION
#ifndef RETRIG_TIME_CR_CYCLES
#define RETRIG_TIME_CR_CYCLES 2
#endif
#endif // MIDI_CV_RETRIG_OPTION

// TODO: separate int math?
// fast approximation of exp2(x/4096)
static inline uint16_t exp2_fast(uint16_t x)
{
    uint32_t e = x / 4096;
    uint32_t s = x & (4096 - 1);
    e = 1 << e;
    s = s * e / 4096;
    e = e + s;
    return (uint16_t)e;
}

// left aligned
// 0 - shortest
// 65535 - longest
// the actual length will depend on the call rate, so please, make it constant and around 1500-3000 Hz
static inline void midiCvSetGlide(MidiCvOutVoice* v, const uint16_t pv)
{
    /*
    scaling coefficients calculation for frequency or time dial
    1 - define overall range (range = max_value / min_value (~10000))
    2 - find k (k = np.log2(range)/128*4096)
    3 - find b (b = np.log2(min_value))
    that's it!
    */
    const uint32_t k = 430 * 128; // limits max time
    uint32_t arg = 65535 - k * pv / 65536;
    // setting w is basically the SR scaling: w = freq / SR
    // here we can just shift b
    v->glidew = exp2_fast(arg);
}

// TODO: midiCvGetGlideTimeMs(CONTROL_RATE)

void midiCvInit(MidiCvOutVoice* v)
{
    v->channel = 0;
    v->pwrange = 12 << 9; // octave
    v->damperstate = 0;
    v->glidew = 65535;
    v->keycount = 0;
    v->pitch_goal = 0;
    v->pitch_slide = 0;
    v->pwshift = 0;
    v->velo_last = 0;
    v->velo_goal = 0;
    v->velo_slide = 0;
    for (unsigned i = 0; i < MIDI_CV_CH_TOTAL; i++) {
        v->out[i] = 0;
    }
#ifdef MIDI_CV_RETRIG_OPTION
    v->gateretrig = 0;
    v->retrigrequest = 0;
#endif
}
// this must be called on every CR
void midiCvTap(MidiCvOutVoice* v)
{
    v->pitch_slide += (v->pitch_goal - v->pitch_slide) / 65536 * v->glidew;
    v->velo_slide += (v->velo_goal - v->velo_slide) / 65536 * v->glidew;
    int32_t pitch = v->pitch_slide / 32768 + v->pwshift;
    if (pitch < 0) {
        pitch = 0;
    } else if (pitch > 65535) {
        pitch = 65535;
    }
    v->out[MIDI_CV_CH_PITCH] = pitch;
#ifdef MIDI_CV_RETRIG_OPTION
    if (v->retrigrequest) {
        v->retrigrequest--;
        v->out[MIDI_CV_CH_VELO] = 0;
        v->out[MIDI_CV_CH_VELO_LAST] = 0;
    } else {
        v->out[MIDI_CV_CH_VELO_LAST] = v->velo_last << 9;
        v->out[MIDI_CV_CH_VELO] = v->velo_slide / 32768;
    }
#else
    v->out[MIDI_CV_CH_VELO_LAST] = v->velo_last << 9;
    v->out[MIDI_CV_CH_VELO] = v->velo_slide / 32768;
#endif // MIDI_CV_RETRIG_OPTION
}

static void mCv_cc(MidiMessageT m, MidiCvOutVoice* v)
{
    switch (m.byte2) {
    case MIDI_CC_01_MODWHEEL_MSB:
        v->out[MIDI_CV_CH_MODWHEEL] &= 0x01FF;
        v->out[MIDI_CV_CH_MODWHEEL] |= m.byte3 << 9;
        break;
    case MIDI_CC_21_MODWHEEL_LSB:
        v->out[MIDI_CV_CH_MODWHEEL] &= 0xFE00;
        v->out[MIDI_CV_CH_MODWHEEL] |= m.byte3 << 2;
        break;
    case MIDI_CC_02_BREATH_MSB:
        v->out[MIDI_CV_CH_BREATH] &= 0x01FF;
        v->out[MIDI_CV_CH_BREATH] |= m.byte3 << 9;
        break;
    case MIDI_CC_22_BREATH_LSB:
        v->out[MIDI_CV_CH_BREATH] &= 0xFE00;
        v->out[MIDI_CV_CH_BREATH] |= m.byte3 << 2;
        break;
    case MIDI_CC_03_UNDEF_MSB:
        // let it be pitch bend range in semitones, but 0..63
        v->pwrange = (m.byte3 >> 1) << 9;
        break;
    case MIDI_CC_05_PORTATIME_MSB:
        v->portamento &= 0x01FF;
        v->portamento |= m.byte3 << 9;
        midiCvSetGlide(v, v->portamento);
        break;
    case MIDI_CC_25_PORTATIME_LSB:
        v->portamento &= 0xFE00;
        v->portamento |= m.byte3 << 2;
        midiCvSetGlide(v, v->portamento);
        break;
    case MIDI_CC_40_DAMPERPEDAL_SW: // damper
        if (0 != m.byte3) {
            v->damperstate = 1;
        } else {
            v->damperstate = 0;
            if (v->keycount == 0) {
                v->velo_last = 0;
                v->velo_goal = 0;
                v->velo_slide = 0;
            }
        }
        break;
    case MIDI_CC_78_ALLSOUNDOFF_ND:
    case MIDI_CC_7B_ALLNOTESOFF_ND:
        v->damperstate = 0;
        v->keycount = 0;
        v->velo_last = 0;
        v->velo_goal = 0;
        v->velo_slide = 0;
        break;
#ifdef MIDI_CV_RETRIG_OPTION
    case MIDI_CC_7E_MONOPHONIC_ND:
        v->gateretrig = 0;
        break;
    case MIDI_CC_7F_POLYPHONIC_ND:
        v->gateretrig = 1;
        break;
#endif // MIDI_CV_RETRIG_OPTION
    default:
        break;
    }
}

static void mCv_na(MidiMessageT m, MidiCvOutVoice* v)
{
    (void)m;
    (void)v;
}
static void mCv_pb(MidiMessageT m, MidiCvOutVoice* v)
{
    int16_t value = (m.byte2 | (m.byte3 << 7)) - 0x2000;
    v->pwshift = (int32_t)v->pwrange * value / 0x2000;
}
static void mCv_at(MidiMessageT m, MidiCvOutVoice* v)
{
    v->out[MIDI_CV_CH_AFTERTOUCH] = m.byte2 << 9;
}

static void mCv_non(MidiMessageT m, MidiCvOutVoice* v)
{
    uint8_t note = m.byte2;
    uint8_t velo = m.byte3;
    if (v->keycount < NOTE_HOLD_MAX_MEMORY) {
        v->keycount++;
    }
    // scan from bottom to top
    for (int i = v->keycount; i > 0; i--) {
        // shift the whole used part of buffer down (from the priority pov)
        v->notememory[i] = v->notememory[i - 1];
    }
    // and put new note at the top
    v->notememory[0].note = note;
    v->notememory[0].velocity = velo;
    v->pitch_goal = note << 24;
    v->velo_goal = velo << 24;
    v->velo_last = velo;

    if (v->damperstate == 0) {
        if (v->keycount == 1) {
            v->pitch_slide = note << 24;
            v->velo_slide = velo << 24;
        }
#ifdef MIDI_CV_RETRIG_OPTION
        else if (v->gateretrig) {
            v->pitch_slide = note << 24;
            v->velo_slide = velo << 24;
            v->retrigrequest = RETRIG_TIME_CR_CYCLES;
        }
#endif // MIDI_CV_RETRIG_OPTION
    }
}
static void mCv_noff(MidiMessageT m, MidiCvOutVoice* v)
{
    uint8_t note = m.byte2;
    // scan note buffer
    for (int i = 0; i < v->keycount; i++) {
        // if we found note, release it...
        if (v->notememory[i].note == note) {
            v->keycount--; // decrement
            for (; i < v->keycount; i++) {
                // shift other notes
                v->notememory[i] = v->notememory[i + 1];
            }
            // retrig
            if (v->keycount) {
                v->pitch_goal = v->notememory[0].note << 24;
                v->velo_goal = v->notememory[0].velocity << 24;
#ifdef MIDI_CV_RETRIG_OPTION
                if ((v->damperstate == 0) && (v->gateretrig)) {
                    v->pitch_slide = v->notememory[0].note << 24;
                    v->velo_slide = v->notememory[0].velocity << 24;
                    v->retrigrequest = RETRIG_TIME_CR_CYCLES;
                }
#endif // MIDI_CV_RETRIG_OPTION
            } else {
                if (v->damperstate == 0) {
                    v->velo_last = 0;
                    v->velo_goal = 0;
                    v->velo_slide = 0;
                }
            }
        }
    }
}

static void (*const midi_tocv[16])(MidiMessageT m, MidiCvOutVoice* v) = {
    mCv_na, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.
    mCv_na, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.
    mCv_na, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.
    mCv_na, // 0x3 3 Three-byte System Common messages like SPP, etc.
    mCv_na, // 0x4 3 SysEx starts or continues
    mCv_na, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.
    mCv_na, // 0x6 2 SysEx ends with following two bytes.
    mCv_na, // 0x7 3 SysEx ends with following three bytes.
    mCv_noff, // 0x8 3 Note-off
    mCv_non, // 0x9 3 Note-on
    mCv_na, // 0xA 3 Poly-KeyPress
    mCv_cc, // 0xB 3 Control Change
    mCv_na, // 0xC 2 Program Change
    mCv_at, // 0xD 2 Channel Pressure
    mCv_pb, // 0xE 3 PitchBend Change
    mCv_na // 0xF 1 Single Byte
};

void midiCvHandleMessage(MidiCvOutVoice* v, MidiMessageT m)
{
    if (m.midichannel == v->channel)
        midi_tocv[m.cin](m, v);
}
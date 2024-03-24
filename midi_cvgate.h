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

#ifndef _MIDI_CVGATE_H
#define _MIDI_CVGATE_H

#include "midi.h"

// virtual monophonic cvgate processor with multiple cv outputs
// it's up to you how you will map these outputs to physical cv
// gate here is also a cv. so you can use either velocity or
// something simple like (0!=channel_gate)?pin_gate=1:pin_gate=0;
// scaling and calibration is also outside of the scope

// parameters can be set over midi
// received messages will not only be converted to signals, but also configure the glide time, retrig, etc..
// please, take a look into mCv_cc() internal function for available functionality

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NOTE_HOLD_MAX_MEMORY
#define NOTE_HOLD_MAX_MEMORY 10
#endif

typedef enum {
    MIDI_CV_CH_PITCH = 0,
    MIDI_CV_CH_VELO,
    MIDI_CV_CH_VELO_LAST,
    MIDI_CV_CH_AFTERTOUCH,
    MIDI_CV_CH_MODWHEEL,
    MIDI_CV_CH_BREATH,
    MIDI_CV_CH_TOTAL
} MidiCvChannelEn;

typedef struct
{
    uint8_t channel; // 1..16 - channel

    uint16_t portamento;
    int32_t glidew; // 1..65535
    int32_t pitch_goal;
    int32_t pitch_slide;
    int32_t velo_goal;
    int32_t velo_slide;

    int16_t pwrange;
    int32_t pwshift;
#ifdef MIDI_CV_RETRIG_OPTION
    // enable or disable retrig - short gate off on each note (even legato)
    uint8_t gateretrig;
    uint8_t retrigrequest;
#endif // MIDI_CV_RETRIG_OPTION
    uint8_t damperstate;
    uint8_t velo_last;

    uint8_t keycount;
    struct {
        uint8_t note;
        uint8_t velocity;
    } notememory[NOTE_HOLD_MAX_MEMORY];

    uint16_t out[MIDI_CV_CH_TOTAL];
} MidiCvOutVoice;

void midiCvInit(MidiCvOutVoice* v);
void midiCvTap(MidiCvOutVoice* v);
void midiCvHandleMessage(MidiCvOutVoice* v, MidiMessageT m);

#ifdef __cplusplus
}
#endif

#endif // _MIDI_CVGATE_H
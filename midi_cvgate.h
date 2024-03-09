#ifndef _MIDI_CVGATE_H
#define _MIDI_CVGATE_H

#include "midi.h"

// virtual monophonic cvgate processor with multiple cv outputs
// it's up to you how you will map these outputs to physical cv
// gate here is also a cv. so you can use either velocity or
// something simple like (0!=channel_gate)?pin_gate=1:pin_gate=0;
// scaling and calibration is also outside of the scope

// parameters can be set over midi
// so received messages not only converted to signals, but also configure module
// please, take a look into mCv_cc() internal function

// TODO: make polyphonic?? - probably we need a separate voice manager

// #define MIDI_CV_RETRIG_OPTION 0
#define NOTE_HOLD_MAX_MEMORY 10

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
// void midiCvSetGlide(MidiCvOutVoice* v, const uint16_t pv); // use midi stream for that
void midiCvHandleMessage(MidiCvOutVoice* v, MidiMessageT m);

#endif // _MIDI_CVGATE_H
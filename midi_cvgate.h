#ifndef _MIDI_CVGATE_H
#define _MIDI_CVGATE_H

#include "midi.h"

#define NOTE_HOLD_MAX_MEMORY 10

typedef enum {
    MIDI_CVGVB_CH_PITCH = 0,
    MIDI_CVGVB_CH_VELO,
    MIDI_CVGVB_CH_VELO_LAST,
    MIDI_CVGVB_CH_AFTERTOUCH,
    MIDI_CVGVB_CH_MODWHEEL,
    MIDI_CVGVB_CH_BREATH,
    MIDI_CVGVB_CH_TOTAL
} midi_cvout_vb_channels_en;

// voiceblock is a virtual monophonic cvgate processor
// use it's out values for physical cv outputs
typedef struct
{
    uint8_t channel; //1..16 - channel
    uint8_t gateretrig; //0..1             CC 126/127?
    uint16_t glidecoef; //0..65536   smooth always active in all modes, switches to legato in notes // CC 5 ??
    int8_t pw_range; //semitones -24..24   zero - 0x40

    //state
    int32_t pitch_goal;
    int32_t pitch_slide;
    int32_t pwshift;
    int32_t velo_goal;
    int32_t velo_slide;

    uint8_t retrigrequest;
    uint8_t damperstate;
    uint8_t velo_last;

    uint8_t keycount;
    struct {
        uint8_t note;
        uint8_t velocity;
    } notememory[NOTE_HOLD_MAX_MEMORY];

    uint16_t out[MIDI_CVGVB_CH_TOTAL];
} midi_cvout_voiceblock_t;

// it's up to end user to fill all these fields
typedef struct {
    uint16_t scale;
    uint16_t offset_tresh;
    uint8_t vb_mode; // select output
    midi_cvout_voiceblock_t* vb;
} midi_cvout_ram_t;

typedef const struct midi_cvapi {
    void (*set_value)(uint16_t cv);
    midi_cvout_ram_t* cvr;
} midi_cvout_t;

void midi_voiceblock_init(midi_cvout_voiceblock_t* vb);
void midi_cr_cvgprocess(midi_cvout_voiceblock_t* vb);
void midi_cv_transmit(midi_cvout_voiceblock_t* vb, MidiMessageT m);

#endif // _MIDI_CVGATE_H
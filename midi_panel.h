#ifndef _MIDI_PANEL_H
#define _MIDI_PANEL_H

#include "midi.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// control panel extension

/*
 *    ! EVERYTHING IS LEFT ALIGNED !
 * from panel
 * CTRL_CIN_POT                    CC MSB [0..1F] val [0..7F] LSB [20..3F] val [0..7F] - up to 32
 * CTRL_CIN_ENCODER                CC [40..5F] val [40..0x3F] (-64 to 63) - up to 32
 * CTRL_CIN_BUTTON_PUSH            NoteOn, value - velocity, 0x00 - release (NoteOff as well)
 * CTRL_CIN_BUTTON_PRESSURE        PolyKp, value - pressure - up to 128
 *
 * to panel
 * CTRL_CIN_LED_ON                 NoteOn value - color/intensity/palette code. value 0 or NoteOff = OFF
 * CTRL_CIN_PALETTE                PolyKp value - code
 * CTRL_CIN_POT_SWITCH             CC MSB [0..1F] val [0..7F] - switch patch (same functionality) - wait for cross
 * CTRL_CIN_POT_LOCK               MonoAT - switch mode (different functionality) - wait for active
 * CTRL_CIN_ENC_LOCK               MonoAT val ignored - wait for active
 *
 * CTRL_CIN_LED_VALUE              value - set color for future pattern update
 * CTRL_CIN_LED_PATTERN            key - least 4 bits, value - most 4 bits
 * fist time this command received by the panel, it enters full panel update mode and exits when
 * full panel updated or when any other command received (included ack request)
 * better always write led value before update to prevent situation when panell needs to be updated
 * just after previous update.
 *
 * CTRL_CIN_POT_LOCK               cc - number, value - value for potentiometer, that needs to be achieved
 * pot will not send any values until this value is set.
 *
 */

/*
 *
 * F8 - both ways, but it's on the lower lever. (no load into buffers) just connection indicator.
 * in case there are few panels, i don't know. just don't create stupid overcomplicated systems
 * F8 - always first
 *
 *
 *
 * from panel:
 * buttons = notes = noteon/noteoff - push/release
 * potentiometers = cc pairs 0x0:0x20 MSB:LSB MSB can be omitted
 * encoder - poly keypress.
 *
 *
 *
 * to panel:
 * total brightness
 *
 * leds = notes. use velocity for intencity and midi channels for colors (ch 0 - int 1-r 2-g 3-b) or palette
 * potentiometers can be locked by sending lock value with cc pair. lock moment is LSB reception
 * encoders have no parameters
 *
 * display: sysex
 * F0h, 7Fh, 7Fh, 7Fh, 02h, dn, F7 - select display number
 * F0h, 7Fh, 7Fh, 7Fh, 03h, x, y, F7 - cursor set pposition (char/block position on graphic lcd)
 * F0h, 7Fh, 7Fh, 7Fh, 04h, text..., F7 - output
 * F0h, 7Fh, 7Fh, 7Fh, 05h, start_char, data....., F7 - char glyph edit
 * F0h, 7Fh, 7Fh, 7Fh, 06h, ........... something

 *
 *
 */

// from panel
#define CTRL_CIN_ENCODER MIDI_CIN_POLYKEYPRESS
#define CTRL_CIN_POT MIDI_CIN_CONTROLCHANGE
#define CTRL_CIN_BUTTON_PUSH MIDI_CIN_NOTEON
#define CTRL_CIN_BUTTON_RELEASE MIDI_CIN_NOTEOFF

// to panel
#define CTRL_CIN_LED_ON MIDI_CIN_NOTEON
#define CTRL_CIN_LED_OFF MIDI_CIN_NOTEOFF
// pattern ored with value
#define CTRL_CIN_LED_PATTERN MIDI_CIN_POLYKEYPRESS
// value for pattern update
#define CTRL_CIN_LED_VALUE MIDI_CIN_CHANNELPRESSURE
#define CTRL_CIN_POT_LOCK MIDI_CIN_CONTROLCHANGE

#define CTRL_CIN_CUSTOMCODE MIDI_CIN_PROGRAMCHANGE
#define CTRL_CIN_14BITVECTOR MIDI_CIN_PITCHBEND

typedef enum {
    CTRL_BUT_ENC = 0,
    CTRL_BUT_ENC2,
    CTRL_BUT_YES,
    CTRL_BUT_NO,
    CTRL_BUT_F1,
    CTRL_BUT_F2,
    CTRL_BUT_F3,
    CTRL_BUT_F4,

    CTRL_BUT_PLAYSTART,
    CTRL_BUT_PLAYCONT,
    CTRL_BUT_STOP,
    CTRL_BUT_RECORD,
    CTRL_BUT_OVERDUB,
    CTRL_BUT_TAP,
    CTRL_BUT_SHIFT,
    CTRL_BUT_SALAD,

    CTRL_BUT_S01,
    CTRL_BUT_S02,
    CTRL_BUT_S03,
    CTRL_BUT_S04,
    CTRL_BUT_S05,
    CTRL_BUT_S06,
    CTRL_BUT_S07,
    CTRL_BUT_S08,

    CTRL_BUT_S09,
    CTRL_BUT_S10,
    CTRL_BUT_S11,
    CTRL_BUT_S12,
    CTRL_BUT_S13,
    CTRL_BUT_S14,
    CTRL_BUT_S15,
    CTRL_BUT_S16,

    CTRL_BUT_S17,
    CTRL_BUT_S18,
    CTRL_BUT_S19,
    CTRL_BUT_S20,
    CTRL_BUT_S21,
    CTRL_BUT_S22,
    CTRL_BUT_S23,
    CTRL_BUT_S24,

    CTRL_BUT_S25,
    CTRL_BUT_S26,
    CTRL_BUT_S27,
    CTRL_BUT_S28,
    CTRL_BUT_S29,
    CTRL_BUT_S30,
    CTRL_BUT_S31,
    CTRL_BUT_S32,
    CTRL_BUT_TOTAL
} buttons_en;

#define CTRL_BUT_RIGHT CTRL_BUT_YES
#define CTRL_BUT_LEFT CTRL_BUT_NO

typedef enum {
    CTRL_ENC_M1 = 0,
    CTRL_ENC_M2,

    CTRL_ENC_X1,
    CTRL_ENC_X2,
    CTRL_ENC_X3,
    CTRL_ENC_X4,
    CTRL_ENC_X5,
    CTRL_ENC_X6,
    CTRL_ENC_X7,
    CTRL_ENC_X8,
    CTRL_ENC_TOTAL
} encoders_en;

typedef enum {
    CTRL_POT_X1 = 8,
    CTRL_POT_X2,
    CTRL_POT_X3,
    CTRL_POT_X4,
    CTRL_POT_X5,
    CTRL_POT_X6,
    CTRL_POT_X7,
    CTRL_POT_X8,
    CTRL_POT_TOTAL
} pots_en;





#endif // _MIDI_PANEL_H
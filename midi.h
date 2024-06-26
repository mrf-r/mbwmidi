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

#ifndef _MIDI_H
#define _MIDI_H

#include "midi_conf.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned MidiRet;

#define MIDI_RET_OK 0x1
#define MIDI_RET_FAIL 0x0

typedef enum {
    MIDI_CIN_RESERVED1 = 0x0, //         0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.
    MIDI_CIN_RESERVED2 = 0x1, //         0x1 1, 2 or 3 Cable events. Reserved for future expansion.
    MIDI_CIN_2BYTESYSTEMCOMMON = 0x2, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.
    MIDI_CIN_3BYTESYSTEMCOMMON = 0x3, // 0x3 3 Three-byte System Common messages like SPP, etc.
    MIDI_CIN_SYSEX3BYTES = 0x4, //       0x4 3 SysEx starts or continues
    MIDI_CIN_SYSEXEND1 = 0x5, //         0x5 1 Single-byte System Common Message or SysEx ends with following single byte.
    MIDI_CIN_SYSEXEND2 = 0x6, //         0x6 2 SysEx ends with following two bytes.
    MIDI_CIN_SYSEXEND3 = 0x7, //         0x7 3 SysEx ends with following three bytes.
    MIDI_CIN_NOTEOFF = 0x8, //           0x8 3 Note-off
    MIDI_CIN_NOTEON = 0x9, //            0x9 3 Note-on
    MIDI_CIN_POLYKEYPRESS = 0xA, //      0xA 3 Poly-KeyPress
    MIDI_CIN_CONTROLCHANGE = 0xB, //     0xB 3 Control Change
    MIDI_CIN_PROGRAMCHANGE = 0xC, //     0xC 2 Program Change
    MIDI_CIN_CHANNELPRESSURE = 0xD, //   0xD 2 Channel Pressure
    MIDI_CIN_PITCHBEND = 0xE, //         0xE 3 PitchBend Change
    MIDI_CIN_SINGLEBYTE = 0xF //         0xF 1 Single Byte
} MidiCinEn;

// THIS IS STANDART USB MIDI 1.0 MESSAGE
typedef union {
    uint32_t full_word;
    struct {
        union {
            struct {
                uint8_t cin : 4;
                uint8_t cn : 4;
            };
            uint8_t cn_cin;
        };
        union {
            struct {
                uint8_t midichannel : 4;
                uint8_t miditype : 4;
            };
            uint8_t byte1;
        };
        uint8_t byte2;
        uint8_t byte3;
    };
} MidiMessageT;

#ifdef __cplusplus
}
#endif

#endif // _MIDI_H

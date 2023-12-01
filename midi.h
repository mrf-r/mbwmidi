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
    MIDI_CIN_RESERVED1 = 0, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.        | not used at all
    MIDI_CIN_RESERVED2, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.                         | not used at all
    MIDI_CIN_2BYTESYSTEMCOMMON, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.                   | systemonly
    MIDI_CIN_3BYTESYSTEMCOMMON, // 0x3 3 Three-byte System Common messages like SPP, etc.                             | systemonly
    MIDI_CIN_SYSEX3BYTES, // 0x4 3 SysEx starts or continues                                                    | maybe seq, or systemonly
    MIDI_CIN_SYSEXEND1, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.  | maybe seq, or systemonly
    MIDI_CIN_SYSEXEND2, // 0x6 2 SysEx ends with following two bytes.                                         | maybe seq, or systemonly
    MIDI_CIN_SYSEXEND3, // 0x7 3 SysEx ends with following three bytes.                                       | maybe seq, or systemonly
    MIDI_CIN_NOTEOFF, // 0x8 3 Note-off                                                                     | seq
    MIDI_CIN_NOTEON, // 0x9 3 Note-on                                                                      | seq
    MIDI_CIN_POLYKEYPRESS, // 0xA 3 Poly-KeyPress                                                                | seq
    MIDI_CIN_CONTROLCHANGE, // 0xB 3 Control Change                                                               | seq
    MIDI_CIN_PROGRAMCHANGE, // 0xC 2 Program Change                                                               | seq
    MIDI_CIN_CHANNELPRESSURE, // 0xD 2 Channel Pressure                                                             | seq
    MIDI_CIN_PITCHBEND, // 0xE 3 PitchBend Change                                                             | seq
    MIDI_CIN_SINGLEBYTE // 0xF 1 Single Byte                                                                  | systemonly
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

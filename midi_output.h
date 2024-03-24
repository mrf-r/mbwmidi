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

#ifndef _MIDI_OUTPUT_H
#define _MIDI_OUTPUT_H

#include "midi.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// unsigned power of 2 only!
#ifndef MIDI_TX_BUFFER_SIZE
#define MIDI_TX_BUFFER_SIZE (32)
#endif

#ifndef MIDI_TX_SYSEX_BUFFER_SIZE
#define MIDI_TX_SYSEX_BUFFER_SIZE (16)
#endif

typedef struct {
    // main buffer for messages
    MidiMessageT buf[MIDI_TX_BUFFER_SIZE];
    uint16_t buf_wp;
    uint16_t buf_rp;
    MidiMessageT sysex_buf[MIDI_TX_SYSEX_BUFFER_SIZE];
    uint16_t sysex_wp;
    uint16_t sysex_rp;
    uint8_t status;
    uint8_t sysex_cn; // current sysex source cable
    // we assume that nrpn bulks are always atomic
    // so there is no need to save the source cn
    // uint8_t nrpn_cn;
    uint8_t nrpn_lsb;
    uint8_t nrpn_msb;
    uint32_t nrpn_lsb_time;
    uint32_t nrpn_msb_time;
    // diagnostic stuff. TODO: add conditionals
    uint16_t max_utilisation;
    uint16_t max_syx_utilisation;
    uint32_t messages_flushed;
    uint32_t messages_optimized;
} MidiOutPortContextT;

void midiPortInit(MidiOutPortContextT* cx);
void midiPortFlush(MidiOutPortContextT* cx);

MidiRet midiPortWriteRaw(MidiOutPortContextT* cx, MidiMessageT m);
MidiRet midiPortWrite(MidiOutPortContextT* cx, MidiMessageT m);
MidiRet midiPortReadNext(MidiOutPortContextT* cx, MidiMessageT* m);

uint16_t midiPortFreespaceGet(MidiOutPortContextT* cx);
uint16_t midiPortUtilisation(MidiOutPortContextT* cx);
uint16_t midiPortSysexUtilisation(MidiOutPortContextT* cx);
uint16_t midiPortMaxUtilisation(MidiOutPortContextT* cx);
uint16_t midiPortMaxSysexUtilisation(MidiOutPortContextT* cx);

#ifdef __cplusplus
}
#endif

#endif // _MIDI_OUTPUT_H
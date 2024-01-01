#ifndef _MIDI_INPUT_H
#define _MIDI_INPUT_H

#include "midi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    MidiMessageT mes;
    uint32_t timestamp;
} MidiTsMessageT;

void midiInit(void);
// main buffer write
MidiRet midiWrite(MidiMessageT m);
MidiRet midiTsWrite(MidiMessageT m, uint32_t timestamp);
MidiRet midiNonSysexWrite(MidiMessageT m);
uint16_t midiUtilisationGet(void);
uint16_t midiSysexUtilisationGet(void);

// main buffer read
MidiRet midiRead(MidiTsMessageT* m);
MidiRet midiSysexRead(MidiMessageT* m);
void midiFlush(void);
void midiSysexFlush(void);

#ifdef __cplusplus
}
#endif

#endif // _MIDI_INPUT_H

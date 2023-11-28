#ifndef _MIDI_HW_H
#define _MIDI_HW_H

#include "midi.h"

extern MidiOutPortT u1midi_out;
extern MidiOutPortT u3midi_out;
extern MidiOutPortT usbmidi_out;
extern MidiOutPortT panelmidi_out;

// start
void midi_start(void);
void cvgate_start(void);

// cr loop
void midi_update(void); //to be called in CR
void cvgate_update(void);

// event
void cvgate_send(MidiMessageT m);


#endif // _MIDI_HW_H
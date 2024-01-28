#ifndef _EVENTLINK_H
#define _EVENTLINK_H

#include "midi.h"
#include "cellmem.h"

typedef struct
{
    uint16_t next;
    uint16_t time;
    MidiMessageT message;
} MidiEvent;

// after event_id = cmAlloc(...)
static inline void eventLink(MidiEvent* array, uint16_t chain_id, uint16_t event_id)
{
    MIDI_ASSERT(chain_id);
    MIDI_ASSERT(event_id);
    MIDI_ASSERT(event_id != chain_id);

    if (array[chain_id].next) {
        // time - based link
        uint16_t event_time = array[event_id].time;

        uint16_t prev = chain_id;
        uint16_t next = array[prev].next;
        while (next) {
            uint16_t time = array[next].time;
            if (time > event_time) {
                array[event_id].next = next;
                array[prev].next = event_id;
                return;
            }
            prev = next;
            next = array[prev].next;
        }
        array[event_id].next = 0;
        array[prev].next = event_id;
    } else {
        // simple link to the end
        array[event_id].next = 0;
        array[chain_id].next = event_id;
    }
}

// before cmFree(bmp, event_id)
static inline void eventUnlink(MidiEvent* array, uint16_t chain_id, uint16_t event_id)
{
    MIDI_ASSERT(chain_id);
    MIDI_ASSERT(event_id);
    MIDI_ASSERT(event_id != chain_id);
    // find previous event
    uint16_t prev_id = chain_id;
    do {
        uint16_t next = array[prev_id].next;
        if (event_id == next) {
            array[prev_id].next = array[event_id].next;
            return;
        }
        prev_id = next;

    } while (prev_id);
    // chain does not contain event
    MIDI_ASSERT(0);
}

#endif // _EVENTLINK_H
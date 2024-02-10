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

// typedef const struct {
//     MidiOutPortContextT* context;
//     // TODO: why api is needed???
//     // const void* api; // defined by type
//     const char* name;
//     // uint8_t type;
//     uint8_t cn;
// } MidiOutPortT;

// typedef enum {
//     MIDI_TYPE_USB = 0,
//     MIDI_TYPE_UART, // normal midi protocol
//     MIDI_TYPE_COM, // also same as midi, but with different physics and speeds
//     MIDI_TYPE_IIC, // same as usb, polling?
//     MIDI_TYPE_TOTAL
// } MidiOutPortTypeEn;

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
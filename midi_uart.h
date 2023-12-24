#ifndef _MIDI_UART_H
#define _MIDI_UART_H

#include "midi_input.h"
#include "midi_output.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: this should be pointed thru MidiOutUartApiT
typedef struct {
    union {
        uint32_t full;
        uint8_t byte[4];
    }message;
    uint8_t message_pos;
    uint8_t message_len;
    uint32_t rs_alive_timer; // running status alive timer
    uint32_t activesense_timer;
} MidiOutUartContextT;

typedef const struct {
    MidiOutUartContextT* context;
    void (*sendbyte)(uint8_t b); // send byte and enable irq
    void (*stop_send)(void); // disable irq
    MidiRet (*is_busy)(void); // read irq status
} MidiOutUartApiT;

// and this is completely different story
typedef struct _MidiInUartContextT {
    void (*data_handler)(uint8_t d, struct _MidiInUartContextT* input);
    uint32_t activesense_timer;
    uint32_t runningstatus_timer;
    MidiTsMessageT message;
} MidiInUartContextT;

void midiInUartInit(MidiInPortT* p);
void midiInUartByteReceiveCallback(uint8_t byte, MidiInPortT* p);
void midiInUartTap(MidiInPortT* p);

void midiOutUartInit(MidiOutPortT* p);
void midiOutUartTranmissionCompleteCallback(MidiOutPortT* p); // transmit next
void midiOutUartTap(MidiOutPortT* p);

#ifdef __cplusplus
}
#endif

#endif // _MIDI_UART_H
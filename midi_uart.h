#ifndef _MIDI_UART_H
#define _MIDI_UART_H

#include "midi.h"

// TODO: this should be pointed thru MidiOutUartApiT
typedef struct {
    union {
        uint32_t message_full;
        uint8_t message_bytes[4];
    };
    uint16_t rs_alive_timer; // running status alive timer
    uint16_t activesense_timer;
    uint8_t laststatus;
    uint8_t tx_pos;
} MidiOutUartContextT;

typedef const struct {
    MidiOutUartContextT* context;
    void (*start_send)(void);
    void (*stop_send)(void);
    uint8_t (*is_busy)(void);
    void (*sendbyte)(uint8_t b);
} MidiOutUartApiT;

// and this is completely different story
typedef struct _MidiInUartContextT {
    void (*data_handler)(uint8_t d, struct _MidiInUartContextT* input);
    uint16_t activesense_timer;
    uint16_t runningstatus_timer;
    MidiTsMessageT message;
} MidiInUartContextT;

void midiInUartInit(MidiInPortT* p);
void midiInUartByteReceiveCallback(uint8_t byte, MidiInPortT* p);
void midiInUartTap(MidiInPortT* p);

void midiOutUartInit(MidiOutPortT* p);
void midiOutUartTranmissionCompleteCallback(MidiOutPortT* p); // transmit next
void midiOutUartTap(MidiOutPortT* p);

#endif // _MIDI_UART_H
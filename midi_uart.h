#ifndef _MIDI_UART_H
#define _MIDI_UART_H

#include "midi_input.h"
#include "midi_output.h"

#ifdef __cplusplus
extern "C" {
#endif

// input
typedef struct _MidiInUartContextT {
    void (*data_handler)(uint8_t d, struct _MidiInUartContextT* input);
    uint32_t activesense_timer;
    uint32_t runningstatus_timer;
    MidiTsMessageT message;
} MidiInUartContextT;

void midiInUartInit(MidiInUartContextT* cx);
void midiInUartByteReceiveCallback(uint8_t byte, MidiInUartContextT* cx, const uint8_t cn);
void midiInUartTap(MidiInUartContextT* cx, const uint8_t cn);

// output
typedef struct {
    union {
        uint32_t full;
        uint8_t byte[4];
    } message;
    uint8_t message_pos;
    uint8_t message_len;
    uint32_t rs_alive_timer; // running status alive timer
    uint32_t activesense_timer;
} MidiOutUartContextT;

typedef const struct {
    MidiOutPortContextT* port;
    MidiOutUartContextT* context;
    void (*sendByte)(uint8_t b); // send byte and enable irq
    void (*stopSend)(void); // disable irq
    MidiRet (*isBusy)(void); // read irq status
} MidiOutUartPortT;

void midiOutUartInit(MidiOutUartPortT* p);
void midiOutUartTranmissionCompleteCallback(MidiOutUartPortT* p); // transmit next
void midiOutUartTap(MidiOutUartPortT* p);

#ifdef __cplusplus
}
#endif

#endif // _MIDI_UART_H
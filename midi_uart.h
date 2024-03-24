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
    MidiRet (*isBusy)(void); // read irq status FAIL==free
} MidiOutUartPortT;

void midiOutUartInit(MidiOutUartPortT* p);
void midiOutUartTranmissionCompleteCallback(MidiOutUartPortT* p); // transmit next
void midiOutUartTap(MidiOutUartPortT* p);

#ifdef __cplusplus
}
#endif

#endif // _MIDI_UART_H
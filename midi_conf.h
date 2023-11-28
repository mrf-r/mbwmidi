#ifndef _MIDI_CONF_H
#define _MIDI_CONF_H

// ORCA CONFIG

#include <stdint.h>
#include <assert.h>
#define MIDI_ASSERT assert
#define MIDI_DEBUG DEBUG // TODO do we need it????

#define MIDI_ATOMIC_START()
#define MIDI_ATOMIC_END()

#define SAMPLE_RATE 1

extern volatile uint32_t counter_sr;

#define MIDI_GET_CLOCK() (counter_sr)
#define MIDI_GET_CLOCK_RATE() (SAMPLE_RATE)


// maximum 16 cables can be defined !!!!
typedef enum {
    MIDI_CN_CONTROLPANEL = 0, // special cable for control events from buttons, encoders to leds, displays
    MIDI_CN_LOCAL, // local midi events
    MIDI_CN_UART1,
    MIDI_CN_UART2,

    MIDI_CN_UART3,
    MIDI_CN_UART4,
    MIDI_CN_USBHOST1,
    MIDI_CN_USBHOST2,

    MIDI_CN_USBDEVICE1,
    MIDI_CN_USBDEVICE2,
    MIDI_CN_CVGATE1,
    MIDI_CN_CVGATE2,

    MIDI_CN_CVGATE3,
    MIDI_CN_CVGATE4,
    MIDI_CN_TOTAL
} MidiCnEn;


#endif // _MIDI_CONF_H

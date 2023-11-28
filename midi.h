#ifndef _MIDI_H
#define _MIDI_H

#include "midi_conf.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned MidiRet;

#define MIDI_RET_OK 0x1
#define MIDI_RET_FAIL 0x0

typedef enum {
    MIDI_CIN_RESERVED1 = 0,     // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.        | not used at all
    MIDI_CIN_RESERVED2,         // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.                         | not used at all
    MIDI_CIN_2BYTESYSTEMCOMMON, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.                   | systemonly
    MIDI_CIN_3BYTESYSTEMCOMMON, // 0x3 3 Three-byte System Common messages like SPP, etc.                             | systemonly
    MIDI_CIN_SYSEX3BYTES,       // 0x4 3 SysEx starts or continues                                                    | maybe seq, or systemonly
    MIDI_CIN_SYSEXEND1,         // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.  | maybe seq, or systemonly
    MIDI_CIN_SYSEXEND2,         // 0x6 2 SysEx ends with following two bytes.                                         | maybe seq, or systemonly
    MIDI_CIN_SYSEXEND3,         // 0x7 3 SysEx ends with following three bytes.                                       | maybe seq, or systemonly
    MIDI_CIN_NOTEOFF,           // 0x8 3 Note-off                                                                     | seq
    MIDI_CIN_NOTEON,            // 0x9 3 Note-on                                                                      | seq
    MIDI_CIN_POLYKEYPRESS,      // 0xA 3 Poly-KeyPress                                                                | seq
    MIDI_CIN_CONTROLCHANGE,     // 0xB 3 Control Change                                                               | seq
    MIDI_CIN_PROGRAMCHANGE,     // 0xC 2 Program Change                                                               | seq
    MIDI_CIN_CHANNELPRESSURE,   // 0xD 2 Channel Pressure                                                             | seq
    MIDI_CIN_PITCHBEND,         // 0xE 3 PitchBend Change                                                             | seq
    MIDI_CIN_SINGLEBYTE         // 0xF 1 Single Byte                                                                  | systemonly
} MidiCinEn;

// THIS IS STANDART USB MIDI 1.0 MESSAGE
typedef union {
    uint32_t full_word;
    struct {
        union {
            struct {
                uint8_t cin : 4;
                uint8_t cn : 4;
            };
            uint8_t cn_cin;
        };
        union {
            struct {
                uint8_t midichannel : 4;
                uint8_t miditype : 4;
            };
            uint8_t byte1;
        };
        uint8_t byte2;
        uint8_t byte3;
    };
} MidiMessageT;

typedef struct {
    MidiMessageT mes;
    uint32_t timestamp;
} MidiTsMessageT;

// unsigned power of 2 only!
#define MIDI_RX_BUFFER_SIZE (256UL)
#define MIDI_RX_SYSEX_BUFFER_SIZE (256UL)
#define MIDI_TX_BUFFER_SIZE (32UL)
#define MIDI_TX_SYSEX_BUFFER_SIZE (16UL)

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
#ifdef MIDI_DEBUG
    uint16_t max_utilization;
    uint16_t max_syx_utilization;
    uint32_t max_time;
    uint32_t messages_flushed;
    uint32_t messages_optimized;
#endif
} MidiOutPortContextT;

typedef enum {
    MIDI_TYPE_USB = 0,
    MIDI_TYPE_UART, // normal midi protocol
    MIDI_TYPE_COM, // also same as midi, but with different physics and speeds
    MIDI_TYPE_IIC, // same as usb, polling?
    MIDI_TYPE_TOTAL
} MidiOutPortTypeEn; // TODO: type of what??

typedef const struct midi_portout {
    MidiOutPortContextT* context; // common portout block
    const void* api;
    const char* name;
    uint8_t type;
    uint8_t cn;
} MidiOutPortT;

typedef const struct {
    void* context;
    const void* api;
    const char* name;
    uint8_t type;
    uint8_t cn; // unique cn
} MidiInPortT;

void midiInit(void); // TODO: receivers and transmitters here???

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

// out ports
void midiPortInit(MidiOutPortT* p);
uint16_t midiPortFreespaceGet(MidiOutPortT* p);
uint16_t midiPortUtilisation(MidiOutPortT* p);
uint16_t midiPortSysexUtilisation(MidiOutPortT* p);
MidiRet midiPortWriteRaw(MidiOutPortT* p, MidiMessageT m);
void midiPortFlush(MidiOutPortT* p);

MidiRet midiPortWrite(MidiOutPortT* p, MidiMessageT m);





#ifdef __cplusplus
}
#endif

#endif /* _MIDI_H */

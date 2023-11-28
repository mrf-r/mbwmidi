
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  #   #    ###    ####    #####               ///////////////////////////////////////////////////////////////////////////////////////////////
//  #   #   #   #   #   #     #                 ///////////////////////////////////////////////////////////////////////////////////////////////
//  #   #   #####   ####      #                 ////////////////////////////////////////////////////////////////////////////////////////////////
//  #   #   #   #   #  #      #                 ///////////////////////////////////////////////////////////////////////////////////////////////
//   ###    #   #   #   #     #                 ///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "midi_uart.h"

static inline MidiRet mBufferWrite(MidiTsMessageT m)
{
    // TODO: copy from main, delete me
    return 0;
}
static inline MidiRet mSysexWrite(MidiMessageT m)
{
    // TODO: copy from main, delete me
    return 0;
}
static inline MidiRet mSysexUnlock(uint8_t cn)
{
    // TODO: copy from main, delete me
    return 0;
}
static inline MidiRet mSysexLock(uint8_t cn)
{
    // TODO: copy from main, delete me
    return 0;
}
static inline uint8_t midi_port_read_next(MidiOutPortT* p, MidiMessageT* m)
{
    // TODO: copy from main, delete me
    return 0;
}
static inline uint8_t midi_port_check_rt(MidiOutPortT* p, MidiMessageT* m)
{
    // TODO: copy from main, delete me
    return 0;
}


// TODO: remade everything for single counter!!!
// in 3000Hz ticks
#define MIDI_RX_RUNNING_STATUS_RESET_TICKS 1 // ((uint16_t)1000 * CONTROL_RATE / 1000)
#define MIDI_RX_AS_RESET_TICKS 1 // ((uint16_t)330 * CONTROL_RATE / 1000)
#define MIDI_TX_RUNNING_STATUS_HOLD_TICKS 1 // ((uint16_t)200 * CONTROL_RATE / 1000)
#define MIDI_TX_AS_SEND_TICKS 1 // ((uint16_t)270 * CONTROL_RATE / 1000)
// 270mS - send 330mS - connection lost

static void midi_sh0syx(uint8_t byte, MidiInUartContextT* input);
static void midi_sh1mtc(uint8_t byte, MidiInUartContextT* input);
static void midi_sh2spp(uint8_t byte, MidiInUartContextT* input);
static void midi_sh3ss(uint8_t byte, MidiInUartContextT* input);
static void midi_sh6tune(uint8_t byte, MidiInUartContextT* input);
static void midi_sh7syxend(uint8_t byte, MidiInUartContextT* input);
static void midi_sh_noa(uint8_t byte, MidiInUartContextT* input);
static void midi_sh_rt(uint8_t byte, MidiInUartContextT* input);
static void midi_shEsense(uint8_t byte, MidiInUartContextT* input);

static void midi_dh_noa(uint8_t byte, MidiInUartContextT* input);
static void midi_dh_3bcm2(uint8_t byte, MidiInUartContextT* input);
static void midi_dh_3bcm3(uint8_t byte, MidiInUartContextT* input);
static void midi_dh_2bcm2(uint8_t byte, MidiInUartContextT* input);
static void midi_dh_syx2(uint8_t byte, MidiInUartContextT* input);
static void midi_dh_syx3(uint8_t byte, MidiInUartContextT* input);
static void midi_dh_syx1(uint8_t byte, MidiInUartContextT* input);
static void midi_dh_3bsm2(uint8_t byte, MidiInUartContextT* input);
static void midi_dh_3bsm3(uint8_t byte, MidiInUartContextT* input);
static void midi_dh_2bsm2(uint8_t byte, MidiInUartContextT* input);

// init uart input handling structures
void midiInUartInit(MidiInPortT* p)
{
    MidiInUartContextT* input = (MidiInUartContextT*)p->context;
    input->activesense_timer = 0;
    input->runningstatus_timer = 0;
    input->data_handler = midi_dh_noa;
    input->message.mes.cn = p->cn;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MIDI 2 USB
static void (*const midi_channel_datahandler[7])(uint8_t byte, MidiInUartContextT* input) = {
    midi_dh_3bcm2, // note off                     | note number       | velocity
    midi_dh_3bcm2, // note on                      | note number       | velocity
    midi_dh_3bcm2, // polyphonic key pressure      | note number       | pressure
    midi_dh_3bcm2, // control change               | controller number | value
    midi_dh_2bcm2, // program change               | program number    | --
    midi_dh_2bcm2, // aftertouch                   | pressure          | --
    midi_dh_3bcm2 // pitch wheel change           | LSB               | MSB
};

static void (*const midi_system_statushandler[16])(uint8_t byte, MidiInUartContextT* input) = {
    midi_sh0syx, // start sysex reception        | ID                | ...
    midi_sh1mtc, // midi time code               | timecode          | --
    midi_sh2spp, // song position pointer        | LSB               | MSB
    midi_sh3ss, // select song                  | song number       | --
    midi_sh_noa, //
    midi_sh_noa, //
    midi_sh6tune, // tune request                 | --                | --
    midi_sh7syxend, // end sysex reception          | --                | --
    midi_sh_rt, // main sync
    midi_sh_noa, // don't know what it used for
    midi_sh_rt, // start
    midi_sh_rt, // continue
    midi_sh_rt, // stop
    midi_sh_noa, //
    midi_shEsense, // 270mS delay after last message, 330mS - connection error
    midi_sh_rt // reset
};

// terminate previous sysex from status
static inline void mtsfs(MidiInUartContextT* input)
{
    if (input->data_handler == midi_dh_syx2) {
        input->message.mes.cin = MIDI_CIN_SYSEXEND2;
        input->message.mes.byte2 = 0xF7;
        input->message.mes.byte3 = 0;
        mSysexWrite(input->message.mes);
        mSysexUnlock(input->message.mes.cn);
    } else {
        if (input->data_handler == midi_dh_syx3) {
            input->message.mes.cin = MIDI_CIN_SYSEXEND3;
            input->message.mes.byte3 = 0xF7;
            mSysexWrite(input->message.mes);
            mSysexUnlock(input->message.mes.cn);
        } else {
            if (input->data_handler == midi_dh_syx1) {
                input->message.mes.cin = MIDI_CIN_SYSEXEND1;
                input->message.mes.byte1 = 0xF7;
                input->message.mes.byte2 = 0;
                input->message.mes.byte3 = 0;
                mSysexWrite(input->message.mes);
                mSysexUnlock(input->message.mes.cn);
            }
        }
    }
}

// uart byte receive callback
void midiInUartByteReceiveCallback(uint8_t byte, MidiInPortT* p)
{
    MidiInUartContextT* input = (MidiInUartContextT*)p->context;
    MIDI_ATOMIC_START();
    if (input->activesense_timer) {
        input->activesense_timer = MIDI_RX_AS_RESET_TICKS;
    }
    if (byte & 0x80) {
        // status
        uint8_t mtype = byte >> 4;
        input->message.timestamp = MIDI_GET_CLOCK();
        input->message.mes.cn = p->cn;
        if (mtype == 0xF) {
            if (byte < 0xF8) {
                mtsfs(input);
            }
            midi_system_statushandler[byte & 0xF](byte, input);
        } else {
            mtsfs(input);
            input->runningstatus_timer = MIDI_RX_RUNNING_STATUS_RESET_TICKS;
            input->message.mes.cin = mtype;
            input->message.mes.byte1 = byte;
            input->message.mes.byte3 = 0;
            input->data_handler = midi_channel_datahandler[mtype & 0x7];
        }
    } else {
        // databyte
        input->data_handler(byte, input);
    }
    MIDI_ATOMIC_END();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MIDI 2 USB
static void midi_sh0syx(uint8_t byte, MidiInUartContextT* input)
{
    // TODO: ability to ignore sysex messages
    // if (global_syx_cn == port->cn)
    if (mSysexLock(input->message.mes.cn)) {
        input->message.mes.cin = MIDI_CIN_SYSEX3BYTES;
        input->message.mes.byte1 = byte;
        // input->message.mes.byte2 = 0;
        // input->message.mes.byte3 = 0;
        input->data_handler = midi_dh_syx2;
    } else {
        input->data_handler = midi_dh_noa;
    }
}
static void midi_sh1mtc(uint8_t byte, MidiInUartContextT* input)
{
    input->message.mes.cin = MIDI_CIN_2BYTESYSTEMCOMMON;
    input->message.mes.byte1 = byte;
    input->message.mes.byte3 = 0;
    input->data_handler = midi_dh_2bsm2;
}
static void midi_sh2spp(uint8_t byte, MidiInUartContextT* input)
{
    input->message.mes.cin = MIDI_CIN_3BYTESYSTEMCOMMON;
    input->message.mes.byte1 = byte;
    input->data_handler = midi_dh_3bsm2;
}
static void midi_sh3ss(uint8_t byte, MidiInUartContextT* input)
{
    input->message.mes.cin = MIDI_CIN_2BYTESYSTEMCOMMON;
    input->message.mes.byte1 = byte;
    input->message.mes.byte3 = 0;
    input->data_handler = midi_dh_2bsm2;
}
static void midi_sh6tune(uint8_t byte, MidiInUartContextT* input)
{
    input->message.mes.cin = MIDI_CIN_SINGLEBYTE;
    input->message.mes.byte1 = byte;
    input->message.mes.byte2 = 0;
    input->message.mes.byte3 = 0;
    mBufferWrite(input->message);
    input->data_handler = midi_dh_noa;
}
static void midi_sh7syxend(uint8_t byte, MidiInUartContextT* input)
{
    if (input->data_handler == midi_dh_syx2) {
        input->message.mes.cin = MIDI_CIN_SYSEXEND2;
        input->message.mes.byte2 = byte;
        input->message.mes.byte3 = 0;
    } else {
        if (input->data_handler == midi_dh_syx3) {
            input->message.mes.cin = MIDI_CIN_SYSEXEND3;
            input->message.mes.byte3 = byte;
        } else {
            input->message.mes.cin = MIDI_CIN_SYSEXEND1;
            input->message.mes.byte1 = byte;
            input->message.mes.byte2 = 0;
            input->message.mes.byte3 = 0;
        }
    }
    mSysexWrite(input->message.mes);
    mSysexUnlock(input->message.mes.cn);
    input->data_handler = midi_dh_noa;
}
static void midi_sh_noa(uint8_t byte, MidiInUartContextT* input)
{
#ifndef MIDI_NO_PROTOCOL_FILTER
    (void)byte;
    (void)input;
    input->data_handler = midi_dh_noa;
#else
    input->message.mes.cin = MIDI_CIN_SINGLEBYTE;
    input->message.mes.byte1 = byte;
    input->message.mes.byte2 = 0;
    input->message.mes.byte3 = 0;
    mBufferWrite(input->message);
#endif
}
static void midi_sh_rt(uint8_t byte, MidiInUartContextT* input)
{
    input->message.mes.cin = MIDI_CIN_SINGLEBYTE;
    input->message.mes.byte1 = byte;
    input->message.mes.byte2 = 0;
    input->message.mes.byte3 = 0;
    mBufferWrite(input->message);
}
static void midi_shEsense(uint8_t byte, MidiInUartContextT* input)
{
#ifndef MIDI_NO_PROTOCOL_FILTER
    input->activesense_timer = MIDI_RX_RUNNING_STATUS_RESET_TICKS;
#else
    input->message.mes.cin = MIDI_CIN_SINGLEBYTE;
    input->message.mes.byte1 = byte;
    input->message.mes.byte2 = 0;
    input->message.mes.byte3 = 0;
    mBufferWrite(input->message);
#endif
    (void)byte;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void midi_dh_noa(uint8_t byte, MidiInUartContextT* input)
{
#ifndef MIDI_NO_PROTOCOL_FILTER
    (void)byte;
    (void)input;
#else
    input->message.timestamp = MIDI_GET_CLOCK();
    input->message.mes.cin = MIDI_CIN_SINGLEBYTE;
    input->message.mes.byte1 = byte;
    input->message.mes.byte2 = 0;
    input->message.mes.byte3 = 0;
    mBufferWrite(input->message);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// channel messages with running status
static void midi_dh_3bcm2(uint8_t byte, MidiInUartContextT* input)
{
    input->message.timestamp = MIDI_GET_CLOCK();
    input->message.mes.byte2 = byte;
    input->data_handler = midi_dh_3bcm3;
}
static void midi_dh_3bcm3(uint8_t byte, MidiInUartContextT* input)
{
    input->message.mes.byte3 = byte;
    input->data_handler = midi_dh_3bcm2;
    mBufferWrite(input->message);
}
static void midi_dh_2bcm2(uint8_t byte, MidiInUartContextT* input)
{
    input->message.timestamp = MIDI_GET_CLOCK();
    input->message.mes.byte2 = byte;
    mBufferWrite(input->message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sysex bulk messages
static void midi_dh_syx2(uint8_t byte, MidiInUartContextT* input)
{
    input->message.mes.byte2 = byte;
    input->data_handler = midi_dh_syx3;
}

static void midi_dh_syx3(uint8_t byte, MidiInUartContextT* input)
{
    input->message.mes.byte3 = byte;
    mSysexWrite(input->message.mes);
    input->data_handler = midi_dh_syx1;
}

static void midi_dh_syx1(uint8_t byte, MidiInUartContextT* input)
{
    input->message.timestamp = MIDI_GET_CLOCK();
    input->message.mes.byte1 = byte;
    // input->message.mes.byte2 = 0;
    // input->message.mes.byte3 = 0;
    input->data_handler = midi_dh_syx2;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// system
static void midi_dh_3bsm2(uint8_t byte, MidiInUartContextT* input)
{
    input->message.mes.byte2 = byte;
    input->data_handler = midi_dh_3bsm3;
}

static void midi_dh_3bsm3(uint8_t byte, MidiInUartContextT* input)
{
    input->message.mes.byte3 = byte;
    input->data_handler = midi_dh_noa;
    mBufferWrite(input->message);
}

static void midi_dh_2bsm2(uint8_t byte, MidiInUartContextT* input)
{
    input->message.mes.byte2 = byte;
    input->data_handler = midi_dh_noa;
    mBufferWrite(input->message);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// system override

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// uart port handling

// uart input handling daemon
// must be called with strict timings
void midiInUartTap(MidiInPortT* p)
{
    MidiInUartContextT* input = (MidiInUartContextT*)p->context;
    MIDI_ATOMIC_START();
    if (input->activesense_timer) {
        input->activesense_timer--;
        if (input->activesense_timer == 0) {
            uint8_t i;
            MidiTsMessageT mes;
            mes.mes.cin = MIDI_CIN_CONTROLCHANGE;
            mes.mes.cn = p->cn;
            mes.mes.byte2 = 0x7B; // ALL NOTES OFF
            mes.mes.byte3 = 0;
            mes.timestamp = MIDI_GET_CLOCK();
            for (i = 0; i < 0xF; i++) {
                mes.mes.byte1 = (uint8_t)(0xB0 + i);
                mBufferWrite(mes);
            }
            // also reset running status
            input->data_handler = midi_dh_noa;
            input->runningstatus_timer = 0;
        }
    } else if (input->runningstatus_timer) {
        input->runningstatus_timer--;
        if (input->runningstatus_timer == 0) {
            input->data_handler = midi_dh_noa;
        }
    }
    MIDI_ATOMIC_END();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// out

// init uart out structures
void midiOutUartInit(MidiOutPortT* p)
{
    MidiOutUartApiT* uap = (MidiOutUartApiT*)p->api;
    MidiOutUartContextT* u = uap->context;
    u->activesense_timer = MIDI_TX_AS_SEND_TICKS;
    u->laststatus = 0;
    u->rs_alive_timer = 0;
    u->tx_pos = 4;
    midiPortInit(p);
}

typedef struct
{
    uint8_t shift;
    uint8_t runningstatus;
} mptbms_t;

static const mptbms_t mptbms[16] = {
    { (4 - 0), 0 }, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.        | not used at all
    { (4 - 0), 0 }, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.                         | not used at all
    { (4 - 2), 0 }, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.                   | systemonly
    { (4 - 3), 0 }, // 0x3 3 Three-byte System Common messages like SPP, etc.                             | systemonly
    { (4 - 3), 0 }, // 0x4 3 SysEx starts or continues                                                    | maybe seq, or systemonly
    { (4 - 1), 0 }, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.  | maybe seq, or systemonly
    { (4 - 2), 0 }, // 0x6 2 SysEx ends with following two bytes.                                         | maybe seq, or systemonly
    { (4 - 3), 0 }, // 0x7 3 SysEx ends with following three bytes.                                       | maybe seq, or systemonly
    { (4 - 3), 1 }, // 0x8 3 Note-off                                                                     | seq
    { (4 - 3), 1 }, // 0x9 3 Note-on                                                                      | seq
    { (4 - 3), 1 }, // 0xA 3 Poly-KeyPress                                                                | seq
    { (4 - 3), 1 }, // 0xB 3 Control Change                                                               | seq
    { (4 - 2), 1 }, // 0xC 2 Program Change                                                               | seq
    { (4 - 2), 1 }, // 0xD 2 Channel Pressure                                                             | seq
    { (4 - 3), 1 }, // 0xE 3 PitchBend Change                                                             | seq
    { (4 - 1), 0 }, // 0xF 1 Single Byte                                                                  | systemonly
};

// uart callback for byte transmit
void midiOutUartTranmissionCompleteCallback(MidiOutPortT* p)
{
    MidiOutUartApiT* uap = (MidiOutUartApiT*)p->api;
    MidiOutUartContextT* ur = uap->context;
    MidiMessageT m;
    MIDI_ATOMIC_START();
    if (ur->tx_pos == 4) {
    nextmessage:
        if (midi_port_read_next(p, &m)) {
            mptbms_t params = mptbms[m.cin];
            ur->tx_pos = params.shift;
            ur->message_full = m.full_word << (8 * (params.shift - 1));
            if (params.runningstatus) {
                if (m.cin == MIDI_CIN_NOTEOFF) {
                    ur->message_bytes[1] |= 0x90;
                    ur->message_bytes[3] = 0x00;
                    // noteoff velocity isn't possible
                }
                if (ur->message_bytes[ur->tx_pos] == ur->laststatus) {
                    ur->tx_pos++;
                } else {
                    ur->laststatus = m.byte1;
                    ur->rs_alive_timer = MIDI_TX_RUNNING_STATUS_HOLD_TICKS;
                }
            }
            // send byte
            if (ur->tx_pos < 4) {
                uap->sendbyte(ur->message_bytes[ur->tx_pos++]);
                ur->activesense_timer = MIDI_TX_AS_SEND_TICKS;
                uap->start_send();
            } 
            // else                DEBUG_PRINTF("uartout: prohibited message type\n");
            // NOW NOTHING IS SEND TO PORT, WE MUST REENTER IRQ ROUTINE
            goto nextmessage;
        } else // port empty
            uap->stop_send();
    } else {
        if (midi_port_check_rt(p, &m)) {
            // insert rt message
            uap->sendbyte(m.byte1);
            ur->activesense_timer = MIDI_TX_AS_SEND_TICKS;
        } else {
            // send next byte of message
            uap->sendbyte(ur->message_bytes[ur->tx_pos++]);
            ur->activesense_timer = MIDI_TX_AS_SEND_TICKS;
        }
    }
    MIDI_ATOMIC_END();
}

// TODO : remove that doubling
#define STATUS_OUTPUT_SYX_MODE 0x04
#warning "bad code here"

// uart output handling daemon
// must be called with strict timings
void midiOutUartTap(MidiOutPortT* p)
{
    MidiOutPortContextT* pr = p->context;
    MidiOutUartApiT* uap = (MidiOutUartApiT*)p->api;
    MidiOutUartContextT* ur = uap->context;
    MIDI_ATOMIC_START();
    if (!(uap->is_busy())) {
        ur->activesense_timer--;
        if (ur->activesense_timer == 0) {
            if (pr->status & STATUS_OUTPUT_SYX_MODE) {
                // DEBUG_PRINTF("probably sysex timeout\n");
                // exit sysex mode and start sending main buffer
                pr->status &= ~STATUS_OUTPUT_SYX_MODE;
                midiOutUartTranmissionCompleteCallback(p);
                // TODO: WARNING
                // atomic state cleared by midiOutUartTranmissionCompleteCallback()
            } else {
                uap->sendbyte(0xFE);
                ur->activesense_timer = MIDI_TX_AS_SEND_TICKS;
            }
        }
    }
    if (ur->rs_alive_timer) {
        ur->rs_alive_timer--;
        if (ur->rs_alive_timer == 0)
            ur->laststatus = 0;
    }
    MIDI_ATOMIC_END();
}

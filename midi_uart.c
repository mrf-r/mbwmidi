#include "midi_uart.h"
#include "midi_input.h"
#include "midi_internals.h"

#define MIDI_RUNNINGSTATUS_RESET (MIDI_CLOCK_RATE * 5000 / 1000)
#define MIDI_RUNNINGSTATUS_HOLD (MIDI_CLOCK_RATE * 500 / 1000)
#define MIDI_ACTIVESENSE_RESET (MIDI_CLOCK_RATE * 330 / 1000)
#define MIDI_ACTIVESENSE_SEND (MIDI_CLOCK_RATE * 270 / 1000)
// 270mS - send 330mS - connection lost

static void mSh_0syx(uint8_t byte, MidiInUartContextT* cx);
static void mSh_1mtc(uint8_t byte, MidiInUartContextT* cx);
static void mSh_2spp(uint8_t byte, MidiInUartContextT* cx);
static void mSh_3ss(uint8_t byte, MidiInUartContextT* cx);
static void mSh_6tune(uint8_t byte, MidiInUartContextT* cx);
static void mSh_7sxend(uint8_t byte, MidiInUartContextT* cx);
static void mSh_noa(uint8_t byte, MidiInUartContextT* cx);
static void mSh_rtna(uint8_t byte, MidiInUartContextT* cx);
static void mSh_rt(uint8_t byte, MidiInUartContextT* cx);
static void mSh_Esense(uint8_t byte, MidiInUartContextT* cx);

static void mDh_noa(uint8_t byte, MidiInUartContextT* cx);
static void mDh_3bCont2(uint8_t byte, MidiInUartContextT* cx);
static void mDh_3bCont3(uint8_t byte, MidiInUartContextT* cx);
static void mDh_2bCont2(uint8_t byte, MidiInUartContextT* cx);
static void mDh_syx2(uint8_t byte, MidiInUartContextT* cx);
static void mDh_syx3(uint8_t byte, MidiInUartContextT* cx);
static void mDh_syx1(uint8_t byte, MidiInUartContextT* cx);
static void mDh_3bSingle2(uint8_t byte, MidiInUartContextT* cx);
static void mDh_3bSingle3(uint8_t byte, MidiInUartContextT* cx);
static void mDh_2bSingle2(uint8_t byte, MidiInUartContextT* cx);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MIDI 2 USB
static void (*const midi_channel_datahandler[7])(uint8_t byte, MidiInUartContextT* cx) = {
    mDh_3bCont2, // note off                     | note number       | velocity
    mDh_3bCont2, // note on                      | note number       | velocity
    mDh_3bCont2, // polyphonic key pressure      | note number       | pressure
    mDh_3bCont2, // control change               | controller number | value
    mDh_2bCont2, // program change               | program number    | --
    mDh_2bCont2, // aftertouch                   | pressure          | --
    mDh_3bCont2 // pitch wheel change           | LSB               | MSB
};

static void (*const midi_system_statushandler[16])(uint8_t byte, MidiInUartContextT* cx) = {
    mSh_0syx, // start sysex reception        | ID                | ...
    mSh_1mtc, // midi time code               | timecode          | --
    mSh_2spp, // song position pointer        | LSB               | MSB
    mSh_3ss, //  select song                  | song number       | --
    mSh_noa, //  -
    mSh_noa, //  -
    mSh_6tune, // tune request                 | --                | --
    mSh_7sxend, // end sysex reception          | --                | --
    mSh_rt, //   main sync
    mSh_rtna, // -
    mSh_rt, //   start
    mSh_rt, //   continue
    mSh_rt, //   stop
    mSh_rtna, // -
    mSh_Esense, // 270mS delay after last message, 330mS - connection error
    mSh_rt // reset
};

static inline void mTerminateSysex(MidiInUartContextT* cx)
{
    if (cx->data_handler == mDh_syx2) {
        cx->message.mes.cin = MIDI_CIN_SYSEXEND2;
        cx->message.mes.byte2 = 0xF7;
        cx->message.mes.byte3 = 0;
        midiTsWrite(cx->message.mes, cx->message.timestamp);
    } else {
        if (cx->data_handler == mDh_syx3) {
            cx->message.mes.cin = MIDI_CIN_SYSEXEND3;
            cx->message.mes.byte3 = 0xF7;
            midiTsWrite(cx->message.mes, cx->message.timestamp);
        } else {
            if (cx->data_handler == mDh_syx1) {
                cx->message.mes.cin = MIDI_CIN_SYSEXEND1;
                cx->message.mes.byte1 = 0xF7;
                cx->message.mes.byte2 = 0;
                cx->message.mes.byte3 = 0;
                midiTsWrite(cx->message.mes, cx->message.timestamp);
            }
        }
    }
    cx->data_handler = mDh_noa; // TODO: side effect
}

// COUNTER_IS_INACTIVE - (time - counter) < 0
// COUNTER_IS_ACTIVE - (time - counter) > 0
// COUNTER_IS_FAILED - (time - counter) > border
// COUNTER_DEACTIVATE - counter = time + 0x80000000
// COUNTER_ACTIVATE - counter = time

// init uart input handling structures
void midiInUartInit(MidiInUartContextT* cx)
{
    uint32_t t = MIDI_GET_CLOCK();
    cx->activesense_timer = t + 0x80000000;
    cx->runningstatus_timer = t; // doesn't matter when data_handler is noa
    cx->data_handler = mDh_noa;
}

// uart input handling daemon
void midiInUartTap(MidiInUartContextT* cx, const uint8_t cn)
{
    MIDI_ATOMIC_START();
    uint32_t t = MIDI_GET_CLOCK();
    int32_t tdelta = t - cx->activesense_timer;
    if (tdelta < 0) {
        // inactive - just shift the value
        cx->activesense_timer = t + 0x80000000;
    } else if (tdelta < MIDI_ACTIVESENSE_RESET) {
        // was active and ok - update value
        cx->activesense_timer = t;
    } else {
        // was active and failed
        cx->data_handler = mDh_noa;
        cx->activesense_timer = t + 0x80000000;
        MidiMessageT m;
        m.cin = m.miditype = MIDI_CIN_CONTROLCHANGE;
        m.cn = cn;
        m.byte2 = 0x7B; // ALL NOTES OFF
        m.byte3 = 0;
        for (int i = 0; i < 16; i++) {
            m.midichannel = i;
            midiWrite(m);
        }
        // also reset running status
        cx->data_handler = mDh_noa;
    }
    MIDI_ATOMIC_END();
}

// called either from IRQ or from the same thread as Tap
void midiInUartByteReceiveCallback(uint8_t byte, MidiInUartContextT* cx, const uint8_t cn)
{
    MIDI_ASSERT(cx->data_handler);
    uint32_t t = MIDI_GET_CLOCK();
    if ((int32_t)(t - cx->activesense_timer) > 0) {
        cx->activesense_timer = t;
    }
    if (byte & 0x80) {
        uint8_t mtype = byte >> 4;
        cx->message.mes.cn = cn;
        if (mtype == 0xF) {
            if (byte < 0xF8) {
                mTerminateSysex(cx);
                cx->message.timestamp = t;
            }
            midi_system_statushandler[byte & 0xF](byte, cx);
        } else {
            mTerminateSysex(cx);
            cx->message.timestamp = t;
            cx->runningstatus_timer = t;
            cx->message.mes.cin = mtype;
            cx->message.mes.cn = cn;
            cx->message.mes.byte1 = byte;
            cx->message.mes.byte3 = 0;
            cx->data_handler = midi_channel_datahandler[mtype & 0x7];
        }
    } else {
        cx->data_handler(byte, cx);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Status

static void mSh_0syx(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.mes.cin = MIDI_CIN_SYSEX3BYTES;
    cx->message.mes.byte1 = byte;
    cx->data_handler = mDh_syx2;
}
static void mSh_1mtc(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.mes.cin = MIDI_CIN_2BYTESYSTEMCOMMON;
    cx->message.mes.byte1 = byte;
    cx->message.mes.byte3 = 0;
    cx->data_handler = mDh_2bSingle2;
}
static void mSh_2spp(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.mes.cin = MIDI_CIN_3BYTESYSTEMCOMMON;
    cx->message.mes.byte1 = byte;
    cx->data_handler = mDh_3bSingle2;
}
static void mSh_3ss(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.mes.cin = MIDI_CIN_2BYTESYSTEMCOMMON;
    cx->message.mes.byte1 = byte;
    cx->message.mes.byte3 = 0;
    cx->data_handler = mDh_2bSingle2;
}
static void mSh_6tune(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.mes.cin = MIDI_CIN_SINGLEBYTE;
    cx->message.mes.byte1 = byte;
    cx->message.mes.byte2 = 0;
    cx->message.mes.byte3 = 0;
    midiTsWrite(cx->message.mes, cx->message.timestamp);
    cx->data_handler = mDh_noa;
}
static void mSh_7sxend(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.timestamp = MIDI_GET_CLOCK();
    mTerminateSysex(cx);
}
static void mSh_noa(uint8_t byte, MidiInUartContextT* cx)
{
    (void)byte;
    (void)cx;
    // TODO: sysex termination timeout??
    mTerminateSysex(cx);
}
static void mSh_rtna(uint8_t byte, MidiInUartContextT* cx)
{
    // filter out unimplemented
    (void)byte;
    (void)cx;
}
static void mSh_rt(uint8_t byte, MidiInUartContextT* cx)
{
    // create local to keep channel message
    MidiMessageT m;
    m.cin = MIDI_CIN_SINGLEBYTE;
    m.cn = cx->message.mes.cn;
    m.byte1 = byte;
    m.byte2 = m.byte3 = 0;
    midiTsWrite(m, MIDI_GET_CLOCK());
}
static void mSh_Esense(uint8_t byte, MidiInUartContextT* cx)
{
    cx->activesense_timer = MIDI_GET_CLOCK();
    (void)byte;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void mDh_noa(uint8_t byte, MidiInUartContextT* cx)
{
#ifndef MIDI_NO_PROTOCOL_FILTER
    (void)byte;
    (void)cx;
#else
    cx->message.timestamp = MIDI_GET_CLOCK();
    cx->message.mes.cin = MIDI_CIN_SINGLEBYTE;
    cx->message.mes.byte1 = byte;
    cx->message.mes.byte2 = 0;
    cx->message.mes.byte3 = 0;
    midiTsWrite(cx->message.mes, cx->message.timestamp);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// channel messages with running status
static void mDh_3bCont2(uint8_t byte, MidiInUartContextT* cx)
{
    uint32_t t = MIDI_GET_CLOCK();
    if ((int32_t)(t - cx->runningstatus_timer) < MIDI_RUNNINGSTATUS_RESET) {
        cx->message.timestamp = t;
        cx->message.mes.byte2 = byte;
        cx->data_handler = mDh_3bCont3;
    } else {
        cx->data_handler = mDh_noa;
    }
}
static void mDh_3bCont3(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.mes.byte3 = byte;
    cx->data_handler = mDh_3bCont2;
    midiTsWrite(cx->message.mes, cx->message.timestamp);
}
static void mDh_2bCont2(uint8_t byte, MidiInUartContextT* cx)
{
    uint32_t t = MIDI_GET_CLOCK();
    if ((int32_t)(t - cx->runningstatus_timer) < MIDI_RUNNINGSTATUS_RESET) {
        cx->message.timestamp = t;
        cx->message.mes.byte2 = byte;
        midiTsWrite(cx->message.mes, cx->message.timestamp);
    } else {
        cx->data_handler = mDh_noa;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sysex bulk messages
static void mDh_syx2(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.mes.byte2 = byte;
    cx->data_handler = mDh_syx3;
}
static void mDh_syx3(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.mes.byte3 = byte;
    if (MIDI_RET_OK == midiTsWrite(cx->message.mes, cx->message.timestamp)) {
        cx->data_handler = mDh_syx1;
    } else {
        cx->data_handler = mDh_noa;
    }
}
static void mDh_syx1(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.timestamp = MIDI_GET_CLOCK();
    cx->message.mes.byte1 = byte;
    cx->data_handler = mDh_syx2;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// system
static void mDh_3bSingle2(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.mes.byte2 = byte;
    cx->data_handler = mDh_3bSingle3;
}
static void mDh_3bSingle3(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.mes.byte3 = byte;
    cx->data_handler = mDh_noa;
    midiTsWrite(cx->message.mes, cx->message.timestamp);
}
static void mDh_2bSingle2(uint8_t byte, MidiInUartContextT* cx)
{
    cx->message.mes.byte2 = byte;
    cx->data_handler = mDh_noa;
    midiTsWrite(cx->message.mes, cx->message.timestamp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// out

// init uart out structures
void midiOutUartInit(MidiOutUartPortT* p)
{
    MidiOutUartContextT* cx = p->context;
    MIDI_ASSERT(cx);
    uint32_t t = MIDI_GET_CLOCK();
    cx->activesense_timer = t;
    cx->message.byte[1] = 0xFF;
    cx->rs_alive_timer = t - MIDI_RUNNINGSTATUS_HOLD;
    cx->message_len = cx->message_pos = 0;
    midiPortInit(p->port);
}

typedef struct
{
    uint8_t shift;
    uint8_t runningstatus;
} mptbms_t;

static const uint8_t cin_len[16] = {
    0, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.        | not used at all
    0, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.                         | not used at all
    2 + 1, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.                   | systemonly
    3 + 1, // 0x3 3 Three-byte System Common messages like SPP, etc.                             | systemonly
    3 + 1, // 0x4 3 SysEx starts or continues                                                    | maybe seq, or systemonly
    1 + 1, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.  | maybe seq, or systemonly
    2 + 1, // 0x6 2 SysEx ends with following two bytes.                                         | maybe seq, or systemonly
    3 + 1, // 0x7 3 SysEx ends with following three bytes.                                       | maybe seq, or systemonly
    3 + 1, // 0x8 3 Note-off                                                                     | seq
    3 + 1, // 0x9 3 Note-on                                                                      | seq
    3 + 1, // 0xA 3 Poly-KeyPress                                                                | seq
    3 + 1, // 0xB 3 Control Change                                                               | seq
    2 + 1, // 0xC 2 Program Change                                                               | seq
    2 + 1, // 0xD 2 Channel Pressure                                                             | seq
    3 + 1, // 0xE 3 PitchBend Change                                                             | seq
    1 + 1, // 0xF 1 Single Byte                                                                  | systemonly
};

// uart callback for byte transmit
void midiOutUartTranmissionCompleteCallback(MidiOutUartPortT* p)
{
    MidiOutUartContextT* cx = p->context;
    MIDI_ATOMIC_START();
    uint32_t t = MIDI_GET_CLOCK();

    if (cx->message_pos < cx->message_len) {
        p->sendByte(cx->message.byte[cx->message_pos++]);
        // update AS if it is enabled
        cx->activesense_timer = t;
    } else {
        // start new message
        MidiMessageT m;
        if (MIDI_RET_OK == midiPortReadNext(p->port, &m)) {
            if ( /* (MIDI_CIN_SINGLEBYTE == m.cin) && */ (m.byte1 > 0xF7)) {
                // realtime
                p->sendByte(m.byte1);
            } else {
#ifndef MIDI_UART_NOTEOFF_VELOCITY_MATTERS
                // can not be performed in midiPort,
                // as NOTEOFF and NOTEON priorities are different
                // it should be optimized here
                if (MIDI_CIN_NOTEOFF == m.cin) {
                    m.byte3 = 0;
                    m.full_word |= 0x00001001;
                }
#endif
                const uint32_t cinrs = 0x7F00;
                if (((cinrs >> m.cin) & 0x1) && (m.byte1 == cx->message.byte[1]) && ((int32_t)(cx->rs_alive_timer - t) > 0)) {
                    // running status is OK, send data
                    p->sendByte(m.byte2);
                    cx->message_pos = 3;
                } else {
                    // send status or first byte
                    p->sendByte(m.byte1);
                    cx->message_pos = 2;
                    cx->rs_alive_timer = t + MIDI_RUNNINGSTATUS_HOLD;
                }
                cx->activesense_timer = t;
                cx->message.full = m.full_word;
                cx->message_len = cin_len[m.cin];
                MIDI_ASSERT(cx->message_pos <= cx->message_len);
            }
        } else {
            // port empty
            p->stopSend();
        }
    }
    MIDI_ATOMIC_END();
}

// can be used to force start transmission?
void midiOutUartTap(MidiOutUartPortT* p)
{
    // MidiOutPortContextT* pcx = p->context;
    MidiOutUartContextT* ucx = p->context;
    MIDI_ATOMIC_START();
    uint32_t t = MIDI_GET_CLOCK();
    if (!(p->isBusy())) {
        // if transmission is inactive
        if ((int32_t)(t - ucx->activesense_timer) > 0) {
            // if AS is active
            if ((int32_t)(t - ucx->activesense_timer) > MIDI_ACTIVESENSE_SEND) {
                ucx->activesense_timer = t;
                p->sendByte(0xFE);
                // TODO: syx timeout should be handled in port level ????
                // or uart tap should be part of port tap
                // if (pcx->status & STATUS_OUTPUT_SYX_MODE) {
                //     // DEBUG_PRINTF("probably sysex timeout\n");
                //     // exit sysex mode and start sending main buffer
                //     pcx->status &= ~STATUS_OUTPUT_SYX_MODE;
            }
            midiOutUartTranmissionCompleteCallback(p);
        } else {
            ucx->activesense_timer = t + 0x80000000;
        }
    }
    MIDI_ATOMIC_END();
}

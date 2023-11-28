

#include "midi.h"

#define SYSEX_CN_UNLOCK 0xFF

#define STATUS_SYXLOCK 0x01
#define STATUS_OPTIMIZATION_DISABLED 0x02
#define STATUS_OUTPUT_SYX_MODE 0x04e
#define STATUS_NRPNUNDEF 0xC0
#define STATUS_NRPAHFAIL 0x40
#define STATUS_NRPALFAIL 0x80

// #define CINBMP_RESERVED1 (1 << 0)
// #define CINBMP_RESERVED2 (1 << 1)
#define CINBMP_2BYTESYSTEMCOMMON (1 << 2)
#define CINBMP_3BYTESYSTEMCOMMON (1 << 3)
#define CINBMP_SYSEX3BYTES (1 << 4)
#define CINBMP_SYSEXEND1 (1 << 5)
#define CINBMP_SYSEXEND2 (1 << 6)
#define CINBMP_SYSEXEND3 (1 << 7)
#define CINBMP_NOTEOFF (1 << 8)
#define CINBMP_NOTEON (1 << 9)
#define CINBMP_POLYKEYPRESS (1 << 10)
#define CINBMP_CONTROLCHANGE (1 << 11)
#define CINBMP_PROGRAMCHANGE (1 << 12)
#define CINBMP_CHANNELPRESSURE (1 << 13)
#define CINBMP_PITCHBEND (1 << 14)
#define CINBMP_SINGLEBYTE (1 << 15)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MIDI COMMON PORT OUTPUT
// buffer analyze and message reduction

static const uint32_t m_compare_mask[16] = {
    // RIGHT TO LEFT: cin-cn, status_byte, data_bytes
    0xFFFFFFFF, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.        | --
    0xFFFFFFFF, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.                         | --
    0x0000FF00, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.                   |
    0x0000FF00, // 0x3 3 Three-byte System Common messages like SPP, etc.                             |
    0xFFFFFF00, // 0x4 3 SysEx starts or continues                                                    | -- //sysex must not go through that method
    0xFFFFFF00, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.  | --
    0xFFFFFF00, // 0x6 2 SysEx ends with following two bytes.                                         | --
    0xFFFFFF00, // 0x7 3 SysEx ends with following three bytes.                                       | --
    0x00FFEF00, // 0x8 3 Note-off                                                                     |
    0x00FFEF00, // 0x9 3 Note-on                                                                      |
    0x00FFFF00, // 0xA 3 Poly-KeyPress                                                                |
    0x00FFFF00, // 0xB 3 Control Change                                                               | used in CC handler
    0x0000FF00, // 0xC 2 Program Change                                                               | --
    0x0000FF00, // 0xD 2 Channel Pressure                                                             | --
    0x0000FF00, // 0xE 3 PitchBend Change                                                             |
    0xFFFFFF00, // 0xF 1 Single Byte                                                                  | --
};
static const uint16_t m_cin_priorities[16] = {
    // values must be from 0 (filtered out) to (buffer size - 2)
    (MIDI_TX_BUFFER_SIZE - 2) * 0 / 1, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.        | not used at all
    (MIDI_TX_BUFFER_SIZE - 2) * 0 / 1, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.                         | not used at all
    (MIDI_TX_BUFFER_SIZE - 2) * 1 / 1, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.                   | 100%
    (MIDI_TX_BUFFER_SIZE - 2) * 1 / 1, // 0x3 3 Three-byte System Common messages like SPP, etc.                             | 100%
    (MIDI_TX_BUFFER_SIZE - 2) * 0 / 1, // 0x4 3 SysEx starts or continues                                                    | --
    (MIDI_TX_BUFFER_SIZE - 2) * 0 / 1, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.  | --
    (MIDI_TX_BUFFER_SIZE - 2) * 0 / 1, // 0x6 2 SysEx ends with following two bytes.                                         | --
    (MIDI_TX_BUFFER_SIZE - 2) * 0 / 1, // 0x7 3 SysEx ends with following three bytes.                                       | --
    (MIDI_TX_BUFFER_SIZE - 2) * 7 / 8, // 0x8 3 Note-off                                                                     | 87%
    (MIDI_TX_BUFFER_SIZE - 2) * 5 / 8, // 0x9 3 Note-on                                                                      | 62% - it must be noticable
    (MIDI_TX_BUFFER_SIZE - 2) * 1 / 2, // 0xA 3 Poly-KeyPress                                                                | 50%
    (MIDI_TX_BUFFER_SIZE - 2) * 7 / 8, // 0xB 3 Control Change                                                               | 87% - NOT USED by defaul
    (MIDI_TX_BUFFER_SIZE - 2) * 2 / 3, // 0xC 2 Program Change                                                               | 67%
    (MIDI_TX_BUFFER_SIZE - 2) * 5 / 9, // 0xD 2 Channel Pressure                                                             | 55%
    (MIDI_TX_BUFFER_SIZE - 2) * 4 / 7, // 0xE 3 PitchBend Change                                                             | 57%
    (MIDI_TX_BUFFER_SIZE - 2) * 1 / 1, // 0xF 1 Single Byte                                                                  | 100% always active
};
static const uint16_t m_cc_priorities[3] = {
    // values must be from 0 (filtered out) to (buffer size - 2)
    (MIDI_TX_BUFFER_SIZE - 2) * 4 / 7, // 57% regular
    (MIDI_TX_BUFFER_SIZE - 2) * 2 / 3, // 67% special
};

static uint32_t m_cc_prio_bmp[4] = {
    0x00000000,
    0x00000000,
    0x00000001, // damper
    0xFF00003C, // channel modes and rpn addr
};

typedef enum { // TODO: unused??
    CC_DEFAULT = 0,
    CC____DATH = 0x6,
    CC____DATL = 0x26,
    CC__DAMPER = 0x40,
    CC_____INC = 0x60,
    CC_____DEC = 0x61,
    CC__NRPNAH = 0x62,
    CC__NRPNAL = 0x63,
    CC___RPNAH = 0x64,
    CC___RPNAL = 0x65,
    CC_NTESOFF = 0x78,
    CC_RESETCC = 0x79,
    CC_LOCALON = 0x7A,
    CC_SNDSOFF = 0x7B,
    CC_OMNIOFF = 0x7C,
    CC__OMNION = 0x7D,
    CC__MONOPH = 0x7E,
    CC__POLYPH = 0x7F,
} tmcc_nrpn_e;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// main buffer input - can also be called from isr
static volatile MidiTsMessageT m_buffer[MIDI_RX_BUFFER_SIZE];
static volatile uint16_t m_wp;
static volatile uint16_t m_rp;
static volatile MidiMessageT m_sysex_buffer[MIDI_RX_SYSEX_BUFFER_SIZE];
static volatile uint16_t m_sysex_wp;
static volatile uint16_t m_sysex_rp;
static volatile uint8_t m_sysex_cn;
#ifdef MIDI_DEBUG
static volatile uint16_t midi_rx_maxload; // TODO: on write ?
static volatile uint16_t midi_rxsyx_maxload; // TODO on write ?
#endif

uint16_t midiUtilisationGet()
{
    return (m_wp - m_rp) & (MIDI_RX_BUFFER_SIZE - 1);
}

uint16_t midiSysexUtilisationGet()
{
    return (m_sysex_wp - m_sysex_rp) & (MIDI_RX_SYSEX_BUFFER_SIZE - 1);
}

static inline MidiRet mBufferWrite(MidiTsMessageT m)
{
    // convert zero velocity noteon to noteoff
    if ((m.mes.full_word & 0xFF00F00F) == 0x00009009) {
        m.mes.full_word &= 0xFFFFEFFE;
    }
    uint16_t wp = (m_wp + 1) & (MIDI_RX_BUFFER_SIZE - 1);
    if (wp != m_rp) {
        m_buffer[m_wp] = m;
        m_wp = wp;
        return MIDI_RET_OK;
    } else {
        return MIDI_RET_FAIL;
    }
}

static inline MidiRet mSysexWrite(MidiMessageT m)
{
    uint16_t wp = (m_sysex_wp + 1) & (MIDI_RX_SYSEX_BUFFER_SIZE - 1);
    if (wp != m_sysex_rp) {
        m_sysex_buffer[m_sysex_wp] = m;
        m_sysex_wp = wp;
        const uint16_t t_syxe = CINBMP_SYSEXEND1 | CINBMP_SYSEXEND2 | CINBMP_SYSEXEND3;
        if ((t_syxe >> m.cin) & 1) {
            m_sysex_cn = SYSEX_CN_UNLOCK;
        }
        return MIDI_RET_OK;
    } else {
        return MIDI_RET_FAIL;
    }
}

static inline MidiRet mSysexLock(uint8_t cn)
{
    if ((m_sysex_cn == SYSEX_CN_UNLOCK) || (m_sysex_cn == cn)) {
        m_sysex_cn = cn;
        return MIDI_RET_OK;
    }
    return MIDI_RET_FAIL;
}

void midiFlush()
{
    MIDI_ATOMIC_START();
    m_rp = 0;
    m_wp = 0;
    MIDI_ATOMIC_END();
}

// free main receive sysex buffer
// ! function doesn't unlock buffer !
void midiSysexFlush()
{
    MIDI_ATOMIC_START();
    m_sysex_rp = 0;
    m_sysex_wp = 0;
    m_sysex_cn = SYSEX_CN_UNLOCK;
    MIDI_ATOMIC_END();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// main buffer processing by main process

// initialize main receive buffers
void midiInit()
{
    m_rp = 0;
    m_wp = 0;
    m_sysex_rp = 0;
    m_sysex_wp = 0;
    m_sysex_cn = SYSEX_CN_UNLOCK;
#ifdef MIDI_DEBUG
    midi_rx_maxload = 0;
    midi_rxsyx_maxload = 0;
#endif
}

// read one timestamped channel message from receive buffers
// returns amount of messages before read
MidiRet midiRead(MidiTsMessageT* m)
{
    if (m_rp != m_wp) {
        *m = m_buffer[m_rp];
        m_rp = (m_rp + 1) & (MIDI_RX_BUFFER_SIZE - 1);
        return MIDI_RET_OK;
    }
    return MIDI_RET_FAIL;
}

// read one sysex message from receive buffers
// returns amount of messages before read
MidiRet midiSysexRead(MidiMessageT* m)
{
    if (m_sysex_rp != m_sysex_wp) {
        *m = m_sysex_buffer[m_sysex_rp];
        m_sysex_rp = (m_sysex_rp + 1) & (MIDI_RX_SYSEX_BUFFER_SIZE - 1);
        return MIDI_RET_OK;
    }
    return MIDI_RET_FAIL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USB INPUT HANDLER
/*
const uint8_t musbmt[16] =
{
                0, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.        | not used at all
                0, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.                         | not used at all
                0, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.                   | systemonly
                0, // 0x3 3 Three-byte System Common messages like SPP, etc.                             | systemonly
                1, // 0x4 3 SysEx starts or continues                                                    | maybe seq, or systemonly
                1, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.  | maybe seq, or systemonly
                1, // 0x6 2 SysEx ends with following two bytes.                                         | maybe seq, or systemonly
                1, // 0x7 3 SysEx ends with following three bytes.                                       | maybe seq, or systemonly
                0, // 0x8 3 Note-off                                                                     | seq
                0, // 0x9 3 Note-on                                                                      | seq
                0, // 0xA 3 Poly-KeyPress                                                                | seq
                0, // 0xB 3 Control Change                                                               | seq
                0, // 0xC 2 Program Change                                                               | seq
                0, // 0xD 2 Channel Pressure                                                             | seq
                0, // 0xE 3 PitchBend Change                                                             | seq
                0, // 0xF 1 Single Byte                                                                  | systemonly
};
const uint8_t mprce[16] =
{
                0, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.        | not used at all
                0, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.                         | not used at all
                0, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.                   | systemonly
                0, // 0x3 3 Three-byte System Common messages like SPP, etc.                             | systemonly
                0, // 0x4 3 SysEx starts or continues                                                    | maybe seq, or systemonly
                1, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.  | maybe seq, or systemonly
                1, // 0x6 2 SysEx ends with following two bytes.                                         | maybe seq, or systemonly
                1, // 0x7 3 SysEx ends with following three bytes.                                       | maybe seq, or systemonly
                0, // 0x8 3 Note-off                                                                     | seq
                0, // 0x9 3 Note-on                                                                      | seq
                0, // 0xA 3 Poly-KeyPress                                                                | seq
                0, // 0xB 3 Control Change                                                               | seq
                0, // 0xC 2 Program Change                                                               | seq
                0, // 0xD 2 Channel Pressure                                                             | seq
                0, // 0xE 3 PitchBend Change                                                             | seq
                0, // 0xF 1 Single Byte                                                                  | systemonly
};
*/

MidiRet midiTsWrite(MidiMessageT m, uint32_t timestamp)
{
    MidiRet ret = MIDI_RET_OK;
    const uint16_t t_syxs = CINBMP_SYSEX3BYTES | CINBMP_SYSEXEND1 | CINBMP_SYSEXEND2 | CINBMP_SYSEXEND3;
    MIDI_ATOMIC_START();
    if ((t_syxs >> m.cin) & 1) {
        // sysex start
        if (MIDI_RET_OK == mSysexLock(m.cn)) {
            ret = mSysexWrite(m);
        } else {
            ret = MIDI_RET_FAIL;
        }
    } else {
        MidiTsMessageT mrx;
        mrx.mes = m;
        mrx.timestamp = timestamp;
        ret = mBufferWrite(mrx);
    }
    MIDI_ATOMIC_END();
    return ret;
}

MidiRet midiWrite(MidiMessageT m)
{
    return midiTsWrite(m, MIDI_GET_CLOCK());
}

MidiRet midiNonSysexWrite(MidiMessageT m)
{
    MidiRet ret;
    MidiTsMessageT mrx;
    mrx.mes = m;
    MIDI_ATOMIC_START();
    mrx.timestamp = MIDI_GET_CLOCK();
    ret = mBufferWrite(mrx);
    MIDI_ATOMIC_END();
    return ret;
}

// typedef enum {
//     TMCC_DEFAULT = 0, // this order is extremely critical
//     TMCC___RPNAH,
//     TMCC___RPNAL,
//     TMCC__NRPNAH, // please, do not touch it
//     TMCC__NRPNAL,
//     TMCC_____INC,
//     TMCC_____DEC,
//     TMCC____DATH,
//     TMCC____DATL,
//     TMCC__DAMPER,
// } tmcc_nrpn_e;

// static const uint8_t __tmcc[128] = {
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC____DATH, TMCC_DEFAULT, //   0  0x00
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, //   8  0x08
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, //  16  0x10
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, //  24  0x18

//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC____DATL, TMCC_DEFAULT, //  32  0x20
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, //  40  0x28
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, //  48  0x30
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, //  56  0x38

//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, //  64  0x40
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, //  72  0x48
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, //  80  0x50
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, //  88  0x58

//     TMCC_____INC, TMCC_____DEC, TMCC__NRPNAL, TMCC__NRPNAH, TMCC___RPNAL, TMCC___RPNAH, TMCC_DEFAULT, TMCC_DEFAULT, //  96  0x60
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, // 104  0x68
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, // 112  0x70
//     TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT, TMCC_DEFAULT // 120  0x78
// };

static MidiRet mPortOptWr________na(MidiOutPortContextT* p, MidiMessageT m);
static MidiRet mPortOptWr_SyxStaCon(MidiOutPortContextT* p, MidiMessageT m);
static MidiRet mPortOptWr____SyxEnd(MidiOutPortContextT* p, MidiMessageT m);
static MidiRet mPortOptWr___Regular(MidiOutPortContextT* p, MidiMessageT m);
static MidiRet mPortOptWr____Single(MidiOutPortContextT* p, MidiMessageT m);
static MidiRet mPortOptWr________CC(MidiOutPortContextT* p, MidiMessageT m);

static MidiRet (*const mPortOptWr[16])(MidiOutPortContextT* p, MidiMessageT m) = {
    mPortOptWr________na, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.
    mPortOptWr________na, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.
    mPortOptWr____Single, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.
    mPortOptWr____Single, // 0x3 3 Three-byte System Common messages like SPP, etc.
    mPortOptWr_SyxStaCon, // 0x4 3 SysEx starts or continues
    mPortOptWr____SyxEnd, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.
    mPortOptWr____SyxEnd, // 0x6 2 SysEx ends with following two bytes.
    mPortOptWr____SyxEnd, // 0x7 3 SysEx ends with following three bytes.
    mPortOptWr___Regular, // 0x9 3 Note-on
    mPortOptWr___Regular, // 0xA 3 Poly-KeyPress
    mPortOptWr___Regular, // 0x8 3 Note-off
    mPortOptWr________CC, // 0xB 3 Control Change
    mPortOptWr___Regular, // 0xC 2 Program Change
    mPortOptWr___Regular, // 0xD 2 Channel Pressure
    mPortOptWr___Regular, // 0xE 3 PitchBend Change
    mPortOptWr____Single, // 0xF 1 Single Byte
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// init output port structure
void midiPortInit(MidiOutPortT* p)
{
    MidiOutPortContextT* cx = (MidiOutPortContextT*)p->context;
    MIDI_ATOMIC_START();
    cx->buf_rp = 0;
    cx->buf_wp = 0;
    cx->status = STATUS_NRPNUNDEF;
    cx->sysex_rp = 0;
    cx->sysex_wp = 0;
#ifdef MIDI_DEBUG
    cx->max_syx_utilization = 0;
    cx->max_utilization = 0;
    cx->max_time = 0;
    cx->messages_flushed = 0;
    cx->messages_optimized = 0;
#endif
    MIDI_ATOMIC_END();
}

uint16_t midiPortUtilisation(MidiOutPortT* p)
{
    MidiOutPortContextT* cx = p->context;
    return (cx->buf_wp - cx->buf_rp) & (MIDI_TX_BUFFER_SIZE - 1);
}
uint16_t midiPortSysexUtilisation(MidiOutPortT* p)
{
    MidiOutPortContextT* cx = p->context;
    return (cx->sysex_wp - cx->sysex_rp) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1);
}

// get available space in output port
// of channel and sysex buffers lowest value will be returned
uint16_t midiPortFreespaceGet(MidiOutPortT* p)
{
    uint16_t ret;
    MIDI_ATOMIC_START();
    MidiOutPortContextT* cx = p->context;
    uint16_t mainbu = (cx->buf_rp - cx->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
    uint16_t syxbu = (cx->sysex_rp - cx->sysex_wp - 1) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1);
    ret = syxbu < mainbu ? syxbu : mainbu;
    MIDI_ATOMIC_END();
    return ret;
}

// write message to output port with protocol logic optimizations
// returns port available space (depending of message type)
MidiRet midiPortWrite(MidiOutPortT* p, MidiMessageT m)
{
    uint16_t ret;
    MidiOutPortContextT* cx = p->context;
    if (cx->status & STATUS_OPTIMIZATION_DISABLED)
        return midiPortWriteRaw(p, m);

    MIDI_ATOMIC_START();
    ret = mPortOptWr[m.cin](cx, m);

    // // start transmission if not already started
    // if (p->type == MIDI_TYPE_UART) {
    //     MidiOutUartApiT* uap = (MidiOutUartApiT*)p->api;
    //     if (!(uap->is_busy())) {
    //         midiOutUartTranmissionCompleteCallback(p);
    //     }
    // }
    MIDI_ATOMIC_END();
    return ret;
}
static inline MidiRet mPortWrite(MidiOutPortContextT* p, MidiMessageT m);

// "midi cable mode" without messages reorganization and
// protocol logic optimization. both channel and sysex
// messages can be send without any rectrictions
// returns amount of space available
// call port flush to exit from this mode
MidiRet midiPortWriteRaw(MidiOutPortT* p, MidiMessageT m)
{
    MidiRet ret;
    MidiOutPortContextT* cx = p->context;
    MIDI_ATOMIC_START();
    cx->status |= STATUS_OPTIMIZATION_DISABLED;
    ret = mPortWrite(cx, m);
    MIDI_ATOMIC_END();
    return ret;
}

// free port, also reset nonoptimize mode
// and unlocks sysex
void midiPortFlush(MidiOutPortT* p)
{
    MidiOutPortContextT* cx = p->context;
    MIDI_ATOMIC_START();
    cx->buf_rp = cx->buf_wp;
    cx->sysex_rp = cx->sysex_wp;
    cx->status = STATUS_NRPNUNDEF;
    MIDI_ATOMIC_END();
}

static MidiRet mPortOptWr________na(MidiOutPortContextT* p, MidiMessageT m)
{
    MIDI_ASSERT(0);
    (void)p;
    (void)m;
    return MIDI_RET_FAIL;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// inside atomic
static inline MidiRet mPortWrite(MidiOutPortContextT* p, MidiMessageT m) //
{
    uint16_t wp = (p->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
    if (wp != p->buf_rp) {
        p->buf[p->buf_wp] = m;
        p->buf_wp = wp;
        return MIDI_RET_OK;
    } else {
        MIDI_ASSERT(0);
        return MIDI_RET_FAIL;
    }
}

// flush certain position from port buffer
static inline void mPortFlushMessage(MidiOutPortContextT* p, uint16_t pos)
{
    MIDI_ASSERT(p);
    MIDI_ASSERT(pos < MIDI_TX_BUFFER_SIZE);
    // rp <= pos < wp
    MIDI_ASSERT(((pos - p->buf_rp) & MIDI_TX_BUFFER_SIZE) < ((p->buf_wp - p->buf_rp) & MIDI_TX_BUFFER_SIZE));
#ifdef MIDI_DEBUG
    p->messages_flushed++;
#endif
    // shift pointer
    p->buf_wp = (p->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
    // flush (write pointer already shifted)
    for (uint16_t i = pos; i != p->buf_wp; i = ((i + 1) & (MIDI_TX_BUFFER_SIZE - 1)))
        p->buf[i] = p->buf[(i + 1) & (MIDI_TX_BUFFER_SIZE - 1)];
}

// write and check rpn address
static inline MidiRet mPortWriteCC(MidiOutPortContextT* p, MidiMessageT m)
{
    // add rpn lost flag in case of filled buffer
    const uint32_t nrpn_al_bmp3 = 0x00000014;
    const uint32_t nrpn_ah_bmp3 = 0x00000028;
    uint8_t cc_pos = m.byte2 >> 5;
    uint8_t cc_msk = m.byte2 & 0x31;

    uint16_t wp = (p->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
    if (wp != p->buf_rp) {
        if (cc_pos == 3) {
            if ((nrpn_al_bmp3 >> cc_msk) & 0x1)
                p->status &= ~STATUS_NRPALFAIL;
            else if ((nrpn_ah_bmp3 >> cc_msk) & 0x1)
                p->status &= ~STATUS_NRPAHFAIL;
        }
        p->buf[p->buf_wp] = m;
        p->buf_wp = wp;
        return MIDI_RET_OK;
    } else {
        if (cc_pos == 3) {
            if ((nrpn_al_bmp3 >> cc_msk) & 0x1)
                p->status |= STATUS_NRPALFAIL;
            else if ((nrpn_ah_bmp3 >> cc_msk) & 0x1)
                p->status |= STATUS_NRPAHFAIL;
        }
#ifdef MIDI_DEBUG
        p->messages_flushed++;
#endif
        return MIDI_RET_FAIL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
optimization types
1 - replace the same message that has not yet been sent with a new value (update)
    - old message removed
    - new message added in the end (we can have situation where controller will not be sent at all)
    TODO: proper way is to update message on that exact position
    - CC:
        - NRPN addresses will not be replaced
        - NRPN data will be checked only up to the last address
2 - priority filtering - the available buffer size is dependent of message type
    - on every write check lowest priority and flush it if utilisation is higher than allowable for that priority
    - if there are multiple, then flush the oldest one
    - CC:
        - NRPN addresses and channel mode messages has higher priority
        - NRPN data will only be replaced until the last address (not perfect..)
        - in case of NRPN address flush, set the UNDEF flag
*/

static MidiRet mPortOptWr________CC(MidiOutPortContextT* p, MidiMessageT m)
{
    uint8_t cc_pos = m.byte2 >> 5;
    uint8_t cc_msk = m.byte2 & 0x31;
    // if this is nrpn data then check it's address validity
    // if either AL or AH was flushed, this data is useless
    const uint32_t nrpn_dat_bmp[4] = {
        0x00000040, // dat msb
        0x00000040, // dat lsb
        0x00000000, //
        0x00000003, // inc dec
    };

    // TODO: the next is repeated filtering, but it will not work if there are few messages
    if ((nrpn_dat_bmp[cc_pos] >> cc_msk) & 0x1) {
        // if this is NRPN DATA
        // check that address is correct
        if (p->status & STATUS_NRPNUNDEF) {
#ifdef MIDI_DEBUG
            p->messages_flushed++;
#endif
            return MIDI_RET_FAIL;
        }

        // scan buffer only up to the last address set
        for (uint16_t i = (p->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
             i != ((p->buf_rp - 1) & (MIDI_TX_BUFFER_SIZE - 1));
             i = (i - 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
            // check that current position is not an address, then we can replace
            cc_pos = p->buf[i].byte2 >> 5;
            cc_msk = p->buf[i].byte2 & 0x31;
            const uint32_t nrpn_addr_bmp3 = 0x0000003C;
            if ((cc_pos == 3) && ((nrpn_addr_bmp3 >> cc_msk) & 1))
                break;
            //
            // all others
        }
        /////////////////////////////////////////////////////
        /////////////////////////////////////////////////////
        /////////////////////////////////////////////////////
        /////////////////////////////////////////////////////
    } else {
        // this CC is just a regular or channel mode
        // scan the whole buffer from end to start
        for (uint16_t i = (p->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
             i != ((p->buf_rp - 1) & (MIDI_TX_BUFFER_SIZE - 1));
             i = (i - 1) & (MIDI_TX_BUFFER_SIZE - 1)) {

            // optimize repeated events
            const uint32_t compare_mask = m_compare_mask[MIDI_CIN_CONTROLCHANGE];
            if (((p->buf[i].full_word ^ m.full_word) & compare_mask) == 0) {
#ifdef MIDI_DEBUG
                p->messages_optimized++;
#endif
                mPortFlushMessage(p, i); // TODO: set NRPN UNDEF
                mPortWriteCC(p, m);
                return MIDI_RET_OK;
            }
            // else check next message
        }
        // there was no repeated events
    }
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    /////////////////////////////////////////////

    uint16_t lowest_priority = m_cin_priorities[MIDI_CIN_SINGLEBYTE];
    uint16_t lowest_priority_pos = p->buf_wp; // lowest priority position
    uint16_t allowed_prio; // buffer load change priority

    // scan from last to first
    for (uint16_t i = (p->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
         i != ((p->buf_rp - 1) & (MIDI_TX_BUFFER_SIZE - 1));
         i = (i - 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
        // check that event is not address

        // check repeat events
        if (((p->buf[i].full_word ^ m.full_word) & m_compare_mask[m.cin]) == 0) {

            // optimize only if it is not nrpn/rpn
            // TODO: buffer may contain few RPN sequences
            // need to check address - data correlation to effectively optimize
            const uint32_t nrpn_addr_bmp3 = 0x0000003C;

            if (1) {
                // if (tmcc[m.byte2] == TMCC_DEFAULT) {
                // cut previous and add actual data to end
#ifdef MIDI_DEBUG
                p->messages_optimized++;
#endif
                mPortFlushMessage(p, i);
                mPortWriteCC(p, m);
                return (p->buf_rp - p->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
            }
        }
        // find lowest priority
        uint8_t priority = m_cin_priorities[p->buf[i].cin];
        if (priority < lowest_priority) {
            lowest_priority = priority;
            lowest_priority_pos = i;
        }
    }
    // check buffer utilization
    // if there is no place for us, then replace lower one or flush ourself
    allowed_prio = (p->buf_rp - p->buf_wp) & (MIDI_TX_BUFFER_SIZE - 1);
    if (lowest_priority < allowed_prio) {
        // cc must have maximum priority, same as realtime
        mPortFlushMessage(p, lowest_priority_pos);
    }
    // cc's got top priority, so write anyway
    mPortWriteCC(p, m);
    return (p->buf_rp - p->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
}

static MidiRet mPortOptWr___Regular(MidiOutPortContextT* p, MidiMessageT m)
{
    uint16_t lowest_priority = m_cin_priorities[MIDI_CIN_SINGLEBYTE];
    uint16_t lowest_priority_pos = p->buf_wp; // lowest priority position
    uint16_t new_priority = m_cin_priorities[m.cin];

    // scan from last to first
    for (uint16_t i = (p->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
         i != ((p->buf_rp - 1) & (MIDI_TX_BUFFER_SIZE - 1));
         i = (i - 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
        // check if the same entity got new value before previous was sent
        if (((p->buf[i].full_word ^ m.full_word) & m_compare_mask[m.cin]) == 0) {
            // shift buffer part and write message to the end
#ifdef MIDI_DEBUG
            p->messages_optimized++;
#endif
            mPortFlushMessage(p, i);
            return mPortWrite(p, m);
        }
        // find lowest priority
        uint16_t priority = m_cin_priorities[p->buf[i].cin];
        if (priority < lowest_priority) {
            lowest_priority = priority;
            lowest_priority_pos = i;
        }
    }
    // check buffer free space
    uint16_t allowed_prio = (p->buf_rp - p->buf_wp) & (MIDI_TX_BUFFER_SIZE - 1);
    // flush lower priority messages on buffer fill
    MidiRet ret;
    if (new_priority < allowed_prio) {
        if (new_priority >= lowest_priority) {
            mPortFlushMessage(p, lowest_priority_pos);
            ret = mPortWrite(p, m);
        } else {
            // flush self
            ret = MIDI_RET_FAIL;
#ifdef MIDI_DEBUG
            p->messages_flushed++;
#endif
        }
    } else {
        // new_priority >= allowed_prio
        if (lowest_priority < allowed_prio) {
            mPortFlushMessage(p, lowest_priority_pos);
        }
        ret = mPortWrite(p, m);
    }
    return ret;
}

// TODO: the problem is that realtime could not interrupt message on serial midi
static MidiRet mPortOptWr____Single(MidiOutPortContextT* p, MidiMessageT m)
{
    uint16_t lowest_priority = m_cin_priorities[MIDI_CIN_SINGLEBYTE];
    uint16_t lowest_priority_pos = p->buf_wp;
    uint16_t allowed_prio = (p->buf_rp - p->buf_wp) & (MIDI_TX_BUFFER_SIZE - 1);
    uint16_t last_rt_pos = p->buf_rp;
    // we can shift real time messages close to read pointer,
    // but it will probably screw protocol too much
    // scan from first to last
    for (uint16_t i = p->buf_rp; i != p->buf_wp; i = (i + 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
        const unsigned t_rt = CINBMP_SINGLEBYTE | CINBMP_2BYTESYSTEMCOMMON | CINBMP_3BYTESYSTEMCOMMON;
        if ((t_rt >> p->buf[i].cin) & 1) {
            last_rt_pos = i;
        }
        uint8_t priority = m_cin_priorities[p->buf[i].cin];
        // find the last one lowest priority
        if (priority <= lowest_priority) {
            lowest_priority = priority;
            lowest_priority_pos = i;
        }
    }
    if (lowest_priority <= allowed_prio) {
        mPortFlushMessage(p, lowest_priority_pos);
    }
    // add rt to the buffer start (rp), but after the other RT messages
    uint16_t rp = (p->buf_rp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
    if (p->buf_wp != rp) {
        p->buf_rp = rp;
        for (uint16_t i = rp; i != last_rt_pos; i = (i + 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
            p->buf[i] = p->buf[(i + 1) & (MIDI_TX_BUFFER_SIZE - 1)];
        }
        p->buf[last_rt_pos] = m;
        return MIDI_RET_OK;
    } else {
#ifdef MIDI_DEBUG
        p->messages_flushed++;
#endif
        return MIDI_RET_FAIL;
    }
}

static MidiRet mPortOptWr_SyxStaCon(MidiOutPortContextT* p, MidiMessageT m)
{
    if ((p->status & STATUS_SYXLOCK) && (p->sysex_cn != m.cn)) {
        // transmission is ongoing, and it's not our business
        return MIDI_RET_FAIL;
    } else {
        uint16_t wp = (p->sysex_wp + 1) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1);
        if (wp != p->sysex_rp) {
            // lock port for our messages only
            p->status |= STATUS_SYXLOCK;
            p->sysex_cn = m.cn;
            p->sysex_buf[p->sysex_wp] = m;
            p->sysex_wp = wp;
            return MIDI_RET_OK;
        } else {
            MIDI_ASSERT(0);
            return MIDI_RET_FAIL;
        }
    }
}

static MidiRet mPortOptWr____SyxEnd(MidiOutPortContextT* p, MidiMessageT m)
{
    if ((p->status & STATUS_SYXLOCK) && (p->sysex_cn != m.cn)) {
        return MIDI_RET_FAIL;
    } else {
        // TODO: in case of overflow it will NOT unlock buffer
        uint16_t wp = (p->sysex_wp + 1) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1);
        if (wp != p->sysex_rp) {
            p->sysex_buf[p->sysex_wp] = m;
            p->sysex_wp = wp;
            p->status &= ~(STATUS_SYXLOCK);
            return MIDI_RET_OK;
        } else {
            MIDI_ASSERT(0);
            return MIDI_RET_FAIL;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// these are called from interrupts
static inline uint8_t midi_port_check_rt(MidiOutPortT* p, MidiMessageT* m)
{
    MidiOutPortContextT* cx = p->context;
    // TODO critical stuff???
    if (cx->status & (STATUS_OPTIMIZATION_DISABLED | STATUS_OUTPUT_SYX_MODE))
        return 0;
    else if (cx->buf_rp != cx->buf_wp) {
        *m = cx->buf[cx->buf_rp];
        if (m->cin == MIDI_CIN_SINGLEBYTE) {
            cx->buf_rp = (cx->buf_rp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
            m->cn = p->cn;
            return 1;
        }
    }
    return 0;
}

// irq
static inline uint8_t midi_port_read_next(MidiOutPortT* p, MidiMessageT* m)
{
    MidiOutPortContextT* cx = p->context;

    if (cx->status & STATUS_OUTPUT_SYX_MODE) {
        if (cx->sysex_rp != cx->sysex_wp) {
            *m = cx->sysex_buf[cx->sysex_rp];
            cx->sysex_rp = (cx->sysex_rp + 1) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1);
            // CIN SYX END
            if ((1 << m->cin) & 0x0700)
                cx->status &= ~STATUS_OUTPUT_SYX_MODE;
            m->cn = p->cn; // TODO: why??
            return 1;
        } else {
            // wait for sysex buffer filling
            return 0;
        }
    }
    // sysex starts only if main buffer is empty
    if (cx->buf_rp != cx->buf_wp) {
        *m = cx->buf[cx->buf_rp];
        cx->buf_rp = (cx->buf_rp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
        m->cn = p->cn;
        return 1;
    } else {
        if (cx->sysex_rp != cx->sysex_wp) {
            *m = cx->sysex_buf[cx->sysex_rp];
            cx->sysex_rp = (cx->sysex_rp + 1) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1);
            // CIN SYX CONT
            if ((1 << m->cin) & 0x0800)
                cx->status |= STATUS_OUTPUT_SYX_MODE;
            m->cn = p->cn;
            return 1;
        } else
            return 0;
    }
}

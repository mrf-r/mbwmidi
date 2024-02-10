#include "midi_output.h"
#include "midi_internals.h"

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

#define MIDI_TX_NRPN_LIFETIME_TICKS ((uint32_t)(MIDI_CLOCK_RATE * 2)) // 2 seconds probably ok

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
    0x0000FF00, // 0xF 1 Single Byte                                                                  | --
};

static MidiRet mOptWr________na(MidiOutPortContextT* cx, MidiMessageT m);
static MidiRet mOptWr_SyxStaCon(MidiOutPortContextT* cx, MidiMessageT m);
static MidiRet mOptWr____SyxEnd(MidiOutPortContextT* cx, MidiMessageT m);
static MidiRet mOptWr___Regular(MidiOutPortContextT* cx, MidiMessageT m);
static MidiRet mOptWr____Single(MidiOutPortContextT* cx, MidiMessageT m);
static MidiRet mOptWr________CC(MidiOutPortContextT* cx, MidiMessageT m);

static MidiRet (*const mPortOptWr[16])(MidiOutPortContextT* cx, MidiMessageT m) = {
    mOptWr________na, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.
    mOptWr________na, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.
    mOptWr____Single, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.
    mOptWr____Single, // 0x3 3 Three-byte System Common messages like SPP, etc.
    mOptWr_SyxStaCon, // 0x4 3 SysEx starts or continues
    mOptWr____SyxEnd, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.
    mOptWr____SyxEnd, // 0x6 2 SysEx ends with following two bytes.
    mOptWr____SyxEnd, // 0x7 3 SysEx ends with following three bytes.
    mOptWr___Regular, // 0x9 3 Note-on
    mOptWr___Regular, // 0xA 3 Poly-KeyPress
    mOptWr___Regular, // 0x8 3 Note-off
    mOptWr________CC, // 0xB 3 Control Change
    mOptWr___Regular, // 0xC 2 Program Change
    mOptWr___Regular, // 0xD 2 Channel Pressure
    mOptWr___Regular, // 0xE 3 PitchBend Change
    mOptWr____Single, // 0xF 1 Single Byte
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint16_t midiPortMaxUtilisation(MidiOutPortContextT* cx)
{
    MIDI_ATOMIC_START();
    uint16_t u = cx->max_utilisation;
    cx->max_utilisation = 0;
    MIDI_ATOMIC_END();
    return u;
}

uint16_t midiPortMaxSysexUtilisation(MidiOutPortContextT* cx)
{
    MIDI_ATOMIC_START();
    uint16_t u = cx->max_syx_utilisation;
    cx->max_syx_utilisation = 0;
    MIDI_ATOMIC_END();
    return u;
}

// init output port structure
void midiPortInit(MidiOutPortContextT* cx)
{
    MIDI_ATOMIC_START();
    cx->buf_rp = 0;
    cx->buf_wp = 0;
    cx->status = STATUS_NRPN_UNDEF_BMP;
    cx->sysex_rp = 0;
    cx->sysex_wp = 0;
    cx->nrpn_lsb = 0xFF;
    cx->nrpn_msb = 0xFF;

    cx->max_syx_utilisation = 0;
    cx->max_utilisation = 0;
    cx->messages_flushed = 0;
    cx->messages_optimized = 0;
    MIDI_ATOMIC_END();
}

uint16_t midiPortUtilisation(MidiOutPortContextT* cx)
{
    return (cx->buf_wp - cx->buf_rp) & (MIDI_TX_BUFFER_SIZE - 1);
}
uint16_t midiPortSysexUtilisation(MidiOutPortContextT* cx)
{
    return (cx->sysex_wp - cx->sysex_rp) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1);
}

// get available space in output port
// of channel and sysex buffers lowest value will be returned
uint16_t midiPortFreespaceGet(MidiOutPortContextT* cx)
{
    uint16_t ret;
    MIDI_ATOMIC_START();
    uint16_t mainbu = (cx->buf_rp - cx->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
    uint16_t syxbu = (cx->sysex_rp - cx->sysex_wp - 1) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1);
    ret = syxbu < mainbu ? syxbu : mainbu;
    MIDI_ATOMIC_END();
    return ret;
}

static inline void mUtilisationUpdate(MidiOutPortContextT* cx)
{
    uint16_t um = ((cx->buf_wp - cx->buf_rp) & (MIDI_TX_BUFFER_SIZE - 1));
    if (um > cx->max_utilisation) {
        cx->max_utilisation = um;
    }
    uint16_t us = ((cx->sysex_wp - cx->sysex_rp) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1));
    if (us > cx->max_syx_utilisation) {
        cx->max_syx_utilisation = us;
    }
}

unsigned midiMessageCheck(MidiMessageT m)
{
    if (m.cin >= 0x8) {
        return m.cin == m.miditype;
    } else {
        switch (m.cin) {
        case MIDI_CIN_2BYTESYSTEMCOMMON:
            return (0xF1 == m.byte1) || (0xF3 == m.byte1); // MTC or Song select
        case MIDI_CIN_3BYTESYSTEMCOMMON:
            return 0xF2 == m.byte1; // SPP
        case MIDI_CIN_SYSEX3BYTES:
            return 1;
        case MIDI_CIN_SYSEXEND1:
            return 0xF7 == m.byte1;
        case MIDI_CIN_SYSEXEND2:
            return 0xF7 == m.byte2;
        case MIDI_CIN_SYSEXEND3:
            return 0xF7 == m.byte3;
        case MIDI_CIN_SINGLEBYTE:
            return 0xF == m.miditype;
        default:
        case MIDI_CIN_RESERVED1:
        case MIDI_CIN_RESERVED2:
            return 0;
        }
    }
}

// write message to output port with protocol logic optimizations
MidiRet midiPortWrite(MidiOutPortContextT* cx, MidiMessageT m)
{
    MIDI_ASSERT(midiMessageCheck(m));
    uint16_t ret;
    if (cx->status & STATUS_OPTIMIZATION_DISABLED)
        return midiPortWriteRaw(cx, m);

    MIDI_ATOMIC_START();
    ret = mPortOptWr[m.cin](cx, m);
    mUtilisationUpdate(cx);
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

// "midi cable mode" without messages reorganization and
// protocol logic optimization. both channel and sysex
// call port flush to exit from this mode
MidiRet midiPortWriteRaw(MidiOutPortContextT* cx, MidiMessageT m)
{
    MIDI_ASSERT(midiMessageCheck(m));
    MIDI_ASSERT(m.cin >= 0x8 ? m.cin == m.miditype : 1);
    MidiRet ret = MIDI_RET_FAIL;
    MIDI_ATOMIC_START();
    cx->status |= STATUS_OPTIMIZATION_DISABLED;
    uint16_t wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
    if (wp != cx->buf_rp) {
        cx->buf[cx->buf_wp] = m;
        cx->buf_wp = wp;
        ret = MIDI_RET_OK;
    }
    mUtilisationUpdate(cx);
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

// free port, also reset nonoptimize mode
// and unlocks sysex
void midiPortFlush(MidiOutPortContextT* cx)
{
    MIDI_ATOMIC_START();
    cx->buf_rp = cx->buf_wp;
    cx->sysex_rp = cx->sysex_wp;
    cx->sysex_cn = SYSEX_CN_UNLOCK;
    cx->status = STATUS_NRPN_UNDEF_BMP;
    cx->nrpn_lsb = 0xFF;
    cx->nrpn_msb = 0xFF;
    MIDI_ATOMIC_END();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
    (MIDI_TX_BUFFER_SIZE - 2) * 5 / 8, // 62% MSB
    (MIDI_TX_BUFFER_SIZE - 2) * 4 / 7, // 57% regular
    (MIDI_TX_BUFFER_SIZE - 2) * 2 / 3, // 67% special
    // CC LSB - not critical, it is possible to flush them without problems
    // CC MSB - by the time LSB is already flushed, so nothing wrong
    // nrpn/rpn data - possible to flush the message without errors - same as CC
    // nrpn/rpn address - by the time data is already flushed, so no errors
    // channel mode - atomic
};

static uint32_t m_cc_prio_bmp[4] = {
    0x00000000,
    0x00000000,
    0x00000001, // damper
    0xFF00003C, // channel modes and rpn addr
};

static inline uint16_t mGetMessagePrio(MidiMessageT m)
{
    uint16_t priority = 0;
    if (m.cin == MIDI_CIN_CONTROLCHANGE) {
        uint8_t cc_pos = m.byte2 >> 5;
        uint8_t cc_msk = m.byte2 & 0x1F;
        if (cc_pos == 0) {
            priority = m_cc_priorities[0];
        } else {
            uint8_t is_high = (m_cc_prio_bmp[cc_pos] >> cc_msk) & 0x1;
            priority = m_cc_priorities[1 + is_high];
        }
    } else {
        priority = m_cin_priorities[m.cin];
    }
    return priority;
}

// flush certain position from port buffer
static inline void mFlushMessage(MidiOutPortContextT* cx, uint16_t pos)
{
    MIDI_ASSERT(cx);
    MIDI_ASSERT(pos < MIDI_TX_BUFFER_SIZE);
    // rp <= pos < wp
    MIDI_ASSERT(((pos - cx->buf_rp) & (MIDI_TX_BUFFER_SIZE - 1)) <= ((cx->buf_wp - cx->buf_rp) & (MIDI_TX_BUFFER_SIZE - 1)));
    cx->messages_flushed++;
    // scan and shift form pos to rp
    for (uint16_t i = pos; i != cx->buf_rp; i = (i - 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
        cx->buf[i] = cx->buf[(i - 1) & (MIDI_TX_BUFFER_SIZE - 1)];
    }
    cx->buf_rp = (cx->buf_rp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
    // // shift from pos to wp
    // cx->buf_wp = (cx->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
    // uint16_t next;
    // for (uint16_t i = pos; i != cx->buf_wp; i = next) {
    //     next = (i + 1) & (MIDI_TX_BUFFER_SIZE - 1);
    //     cx->buf[i] = cx->buf[next];
    // }
}

// return MIDI_RET_OK if there is available space in buffer
static inline MidiRet mCheckBufferSpace(MidiOutPortContextT* cx, uint16_t new_prio)
{
    // priorities are not related to the buffer size and can be arbitrary numbers.
    // they use MIDI_TX_BUFFER_SIZE as a reference for historical reasons
    MidiRet ret = MIDI_RET_OK;
    uint16_t utilisation = ((cx->buf_wp - cx->buf_rp) & (MIDI_TX_BUFFER_SIZE - 1));
    if ((MIDI_TX_BUFFER_SIZE - 1) <= utilisation) {
        uint16_t lowest_prio = 0xFFFF;
        uint16_t lowest_prio_pos;
        for (uint16_t i = cx->buf_rp; i != cx->buf_wp; i = (i + 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
            uint16_t prio = mGetMessagePrio(cx->buf[i]);
            if (prio < lowest_prio) { // oldest will be flushed
                lowest_prio = prio;
                lowest_prio_pos = i;
            }
        }
        if (lowest_prio <= new_prio) { // oldest will be flushed
            mFlushMessage(cx, lowest_prio_pos);
        } else {
            cx->messages_flushed++;
            ret = MIDI_RET_FAIL;
        }
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static MidiRet mOptWr________na(MidiOutPortContextT* cx, MidiMessageT m)
{
    MIDI_ASSERT(0);
    (void)cx;
    (void)m;
    return MIDI_RET_FAIL;
}

// CC is the most complex one
// it uses nrpn special filtering, repeated messages filtering and priority filtering

static inline MidiRet mowccNrpnData(MidiOutPortContextT* cx, MidiMessageT m)
{
    // if this is nrpn data then check it's address validity
    // if either AL or AH was flushed, this data is useless
    if (cx->status & STATUS_NRPN_UNDEF_BMP) {
        cx->messages_flushed++;
        return MIDI_RET_FAIL;
    }
    // scan buffer only up to the last address set, do not replace data of different addresses
    for (uint16_t i = (cx->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
         i != ((cx->buf_rp - 1) & (MIDI_TX_BUFFER_SIZE - 1));
         i = (i - 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
        // check that current position is not an address, then we can replace
        uint8_t cc_pos = cx->buf[i].byte2 >> 5;
        uint8_t cc_msk = cx->buf[i].byte2 & 0x1F;
        const uint32_t nrpn_addr_bmp3 = 0x0000003C;
        if ((cc_pos == 3) && ((nrpn_addr_bmp3 >> cc_msk) & 1)) {
            break;
        } else {
            const uint32_t compare_mask = m_compare_mask[MIDI_CIN_CONTROLCHANGE];
            if (((cx->buf[i].full_word ^ m.full_word) & compare_mask) == 0) {
                cx->messages_optimized++;
                // mFlushMessage(cx, i);
                // cx->buf[cx->buf_wp] = m;
                // cx->buf_wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
                cx->buf[i] = m; // let's simplify!
                return MIDI_RET_OK;
            }
        }
    }
    // filter by priority
    MidiRet ret = MIDI_RET_FAIL;
    if (MIDI_RET_OK == mCheckBufferSpace(cx, mGetMessagePrio(m))) {
        cx->buf[cx->buf_wp] = m;
        cx->buf_wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
        ret = MIDI_RET_OK;
    } else {
        cx->messages_flushed++;
    }
    return ret;
}

static inline MidiRet mowccNrpnAddress(MidiOutPortContextT* cx, MidiMessageT m, uint8_t cc_msk)
{
    // handle RPN-NRPN switch
    if (cx->status & STATUS_NOT_RPN) {
        if (m.byte2 >= 100) {
            cx->status |= STATUS_NRPN_UNDEF_BMP;
            cx->status &= ~STATUS_NOT_RPN;
            cx->nrpn_lsb = 0xFF;
            cx->nrpn_msb = 0xFF;
        }
    } else {
        if (m.byte2 < 100) {
            cx->status |= STATUS_NRPN_UNDEF_BMP | STATUS_NOT_RPN;
            cx->nrpn_lsb = 0xFF;
            cx->nrpn_msb = 0xFF;
        }
    }
    MidiRet ret = MIDI_RET_FAIL;
    const uint16_t prio = m_cc_priorities[2];
    if (MIDI_RET_OK == mCheckBufferSpace(cx, prio)) {
        uint32_t t = MIDI_GET_CLOCK();
        const uint32_t nrpn_al_bmp3 = 0x00000014;
        const uint32_t nrpn_ah_bmp3 = 0x00000028;
        if ((nrpn_al_bmp3 >> cc_msk) & 0x1) {
            if ((m.byte3 == cx->nrpn_lsb) && ((int32_t)(cx->nrpn_lsb_time - t) > 0)) {
                // same address was transmitted recently
                return MIDI_RET_OK;
            } else {
                // different address or timeout
                cx->nrpn_lsb_time = t + MIDI_TX_NRPN_LIFETIME_TICKS;
                cx->nrpn_lsb = m.byte3;
                cx->status &= ~STATUS_NRPNAL_UNDEF;
            }
        } else if ((nrpn_ah_bmp3 >> cc_msk) & 0x1) {
            if ((m.byte3 == cx->nrpn_msb) && ((int32_t)(cx->nrpn_msb_time - t) > 0)) {
                // same address was transmitted recently
                return MIDI_RET_OK;
            } else {
                // different address or timeout
                cx->nrpn_msb_time = t + MIDI_TX_NRPN_LIFETIME_TICKS;
                cx->nrpn_msb = m.byte3;
                cx->status &= ~STATUS_NRPNAH_UNDEF;
            }
        }
        cx->buf[cx->buf_wp] = m;
        cx->buf_wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
        ret = MIDI_RET_OK;
    } else {
        // buffer is full
        const uint32_t nrpn_al_bmp3 = 0x00000014;
        const uint32_t nrpn_ah_bmp3 = 0x00000028;
        if ((nrpn_al_bmp3 >> cc_msk) & 0x1) {
            cx->status |= STATUS_NRPNAL_UNDEF;
        } else if ((nrpn_ah_bmp3 >> cc_msk) & 0x1) {
            cx->status |= STATUS_NRPNAH_UNDEF;
        }
        cx->messages_flushed++;
    }
    return ret;
}

static inline MidiRet mowccRegularCC(MidiOutPortContextT* cx, MidiMessageT m)
{
    for (uint16_t i = (cx->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
         i != ((cx->buf_rp - 1) & (MIDI_TX_BUFFER_SIZE - 1));
         i = (i - 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
        // optimize repeated events
        const uint32_t compare_mask = m_compare_mask[MIDI_CIN_CONTROLCHANGE];
        if (((cx->buf[i].full_word ^ m.full_word) & compare_mask) == 0) {
            cx->messages_optimized++;
            // mFlushMessage(cx, i); // should we keep order?
            // cx->buf[cx->buf_wp] = m;
            // cx->buf_wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
            cx->buf[i] = m; // let's simplify!
            return MIDI_RET_OK;
        }
    }
    // filter by priority
    MidiRet ret = MIDI_RET_FAIL;
    if (MIDI_RET_OK == mCheckBufferSpace(cx, mGetMessagePrio(m))) {
        cx->buf[cx->buf_wp] = m;
        cx->buf_wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
        ret = MIDI_RET_OK;
    } else {
        cx->messages_flushed++;
    }
    return ret;
}

static MidiRet mOptWr________CC(MidiOutPortContextT* cx, MidiMessageT m)
{
    const uint32_t nrpn_dat_bmp[4] = {
        0x00000040, // dat msb
        0x00000040, // dat lsb
        0x00000000, //
        0x00000003, // inc dec
    };
    const uint32_t nrpn_addr_bmp3 = 0x0000003C;
    uint8_t cc_pos = m.byte2 >> 5;
    uint8_t cc_msk = m.byte2 & 0x1F;
    if ((nrpn_dat_bmp[cc_pos] >> cc_msk) & 0x1) {
        return mowccNrpnData(cx, m);
    } else if ((cc_pos == 3) && (nrpn_addr_bmp3 >> cc_msk) & 0x1) {
        return mowccNrpnAddress(cx, m, cc_msk);
    } else {
        return mowccRegularCC(cx, m);
    }
}

// Regular is a little bit harder
// it uses messages optimisation and priority filtering
static MidiRet mOptWr___Regular(MidiOutPortContextT* cx, MidiMessageT m)
{
    // find similar messages and delete them
    // scan from last to first
    for (uint16_t i = (cx->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
         i != ((cx->buf_rp - 1) & (MIDI_TX_BUFFER_SIZE - 1));
         i = (i - 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
        // check if the same entity got new value before previous was sent
        if (0 == ((cx->buf[i].full_word ^ m.full_word) & m_compare_mask[m.cin])) {
            // shift buffer part and write message to the end
            cx->messages_optimized++;
            // mFlushMessage(cx, i);
            // cx->buf[cx->buf_wp] = m;
            // cx->buf_wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
            cx->buf[i] = m;
            return MIDI_RET_OK;
        }
    }
    // filter by priority
    MidiRet ret = MIDI_RET_FAIL;
    if (MIDI_RET_OK == mCheckBufferSpace(cx, mGetMessagePrio(m))) {
        cx->buf[cx->buf_wp] = m;
        cx->buf_wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
        ret = MIDI_RET_OK;
    } else {
        cx->messages_flushed++;
    }
    return ret;
}

// Single is the simplest one
// the only optimisation it uses is priority filtering
static MidiRet mOptWr____Single(MidiOutPortContextT* cx, MidiMessageT m)
{
    MidiRet ret = MIDI_RET_FAIL;
    if (MIDI_RET_OK == mCheckBufferSpace(cx, m_cin_priorities[MIDI_CIN_SINGLEBYTE])) {
        cx->buf_rp = (cx->buf_rp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
        uint16_t pos = cx->buf_rp;
        while (pos != cx->buf_wp) {
            uint16_t next = (pos + 1) & (MIDI_TX_BUFFER_SIZE - 1);
            const unsigned t_rt = CINBMP_SINGLEBYTE | CINBMP_2BYTESYSTEMCOMMON | CINBMP_3BYTESYSTEMCOMMON;
            if ((next != cx->buf_wp) && ((t_rt >> cx->buf[next].cin) & 1)) {
                // same kind of message, shift it, so we can insert after
                cx->buf[pos] = cx->buf[next];
            } else {
                cx->buf[pos] = m;
                ret = MIDI_RET_OK;
                break;
            }
            pos = next;
        }
    } else {
        // this is dead code
        cx->messages_flushed++;
    }
    return ret;
}

static MidiRet mOptWr_SyxStaCon(MidiOutPortContextT* cx, MidiMessageT m)
{
    MidiRet ret = MIDI_RET_FAIL;
    if ((SYSEX_CN_UNLOCK == cx->sysex_cn) || (cx->sysex_cn == m.cn)) {
        uint16_t wp = (cx->sysex_wp + 1) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1);
        if (wp != cx->sysex_rp) {
            cx->sysex_cn = m.cn;
            cx->sysex_buf[cx->sysex_wp] = m;
            cx->sysex_wp = wp;
            ret = MIDI_RET_OK;
        }
    }
    return ret;
}

static MidiRet mOptWr____SyxEnd(MidiOutPortContextT* cx, MidiMessageT m)
{
    MidiRet ret = MIDI_RET_FAIL;
    if ((SYSEX_CN_UNLOCK == cx->sysex_cn) || (cx->sysex_cn == m.cn)) {
        uint16_t wp = (cx->sysex_wp + 1) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1);
        if (wp != cx->sysex_rp) {
            cx->sysex_buf[cx->sysex_wp] = m;
            cx->sysex_wp = wp;
            ret = MIDI_RET_OK;
        }
        cx->sysex_cn = SYSEX_CN_UNLOCK;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// MidiRet midi_port_check_rt(MidiOutPortContextT* cx, MidiMessageT* m)
// {
//     MidiRet ret = MIDI_RET_FAIL;
//     MidiOutPortContextT* cx = p->context;
//     // TODO critical stuff???
//     if ((0 == (cx->status & (STATUS_OPTIMIZATION_DISABLED | STATUS_OUTPUT_SYX_MODE))) && (cx->buf_rp != cx->buf_wp)) {
//         *m = cx->buf[cx->buf_rp];
//         if (m->cin == MIDI_CIN_SINGLEBYTE) {
//             cx->buf_rp = (cx->buf_rp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
//             m->cn = p->cn;
//             ret = MIDI_RET_OK;
//         }
//     }
//     return ret;
// }

MidiRet midiPortReadNext(MidiOutPortContextT* cx, MidiMessageT* m)
{
    MidiRet ret = MIDI_RET_FAIL;
    MIDI_ATOMIC_START();
    if (cx->status & STATUS_OUTPUT_SYX_MODE) {
        if (cx->sysex_rp != cx->sysex_wp) {
            *m = cx->sysex_buf[cx->sysex_rp];
            cx->sysex_rp = (cx->sysex_rp + 1) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1);
            // CIN SYX END
            const uint16_t t_syxe = CINBMP_SYSEXEND1 | CINBMP_SYSEXEND2 | CINBMP_SYSEXEND3;
            if ((t_syxe >> m->cin) & 0x1) {
                cx->status &= ~STATUS_OUTPUT_SYX_MODE;
            }
            ret = MIDI_RET_OK;
        }
        // else wait for sysex buffer filling
    } else {
        // sysex starts only if main buffer is empty - probably not the best policy
        if (cx->buf_rp != cx->buf_wp) {
            *m = cx->buf[cx->buf_rp];
            cx->buf_rp = (cx->buf_rp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
            ret = MIDI_RET_OK;
        } else {
            if (cx->sysex_rp != cx->sysex_wp) {
                *m = cx->sysex_buf[cx->sysex_rp];
                cx->sysex_rp = (cx->sysex_rp + 1) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1);
                // CIN SYX CONT
                const uint16_t t_syxs = CINBMP_SYSEX3BYTES;
                if ((t_syxs >> m->cin) & 0x1) {
                    cx->status |= STATUS_OUTPUT_SYX_MODE;
                }
                ret = MIDI_RET_OK;
            }
        }
    }
    // m->cn = p->cn;
    MIDI_ATOMIC_END();
    return ret;
}

#if (MIDI_TX_BUFFER_SIZE & (MIDI_TX_BUFFER_SIZE - 1)) != 0
#error "output buffer: please, use only power of 2"
#endif

#if (MIDI_TX_SYSEX_BUFFER_SIZE & (MIDI_TX_SYSEX_BUFFER_SIZE - 1)) != 0
#error "output sysex buffer: please, use only power of 2"
#endif

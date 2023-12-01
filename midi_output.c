#include "midi_output.h"
#include "midi_internals.h"

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
uint16_t midiPortMaxUtilisation(MidiOutPortT* p)
{
    MidiOutPortContextT* cx = p->context;
    MIDI_ATOMIC_START();
    uint16_t u = cx->max_utilisation;
    cx->max_utilisation = 0;
    MIDI_ATOMIC_END();
    return u;
}

uint16_t midiPortMaxSysexUtilisation(MidiOutPortT* p)
{
    MidiOutPortContextT* cx = p->context;
    MIDI_ATOMIC_START();
    uint16_t u = cx->max_syx_utilisation;
    cx->max_syx_utilisation = 0;
    MIDI_ATOMIC_END();
    return u;
}

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

    cx->max_syx_utilisation = 0;
    cx->max_utilisation = 0;
    cx->messages_flushed = 0;
    cx->messages_optimized = 0;
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

static inline void mUtilisationUpdate(MidiOutPortContextT* cx, MidiMessageT m)
{
    const uint16_t t_syxs = CINBMP_SYSEX3BYTES | CINBMP_SYSEXEND1 | CINBMP_SYSEXEND2 | CINBMP_SYSEXEND3;
    if ((t_syxs >> m.cin) & 1) {
        uint16_t utilisation = ((cx->sysex_wp - cx->sysex_rp) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1)) + 1;
        if (utilisation > cx->max_syx_utilisation) {
            cx->max_syx_utilisation = utilisation;
        }
    } else {
        uint16_t utilisation = ((cx->buf_wp - cx->buf_rp) & (MIDI_TX_BUFFER_SIZE - 1)) + 1;
        if (utilisation > cx->max_utilisation) {
            cx->max_utilisation = utilisation;
        }
    }
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
    mUtilisationUpdate(cx, m);
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
// messages can be send without any rectrictions
// returns amount of space available
// call port flush to exit from this mode
MidiRet midiPortWriteRaw(MidiOutPortT* p, MidiMessageT m)
{
    MidiRet ret = MIDI_RET_FAIL;
    MidiOutPortContextT* cx = p->context;
    MIDI_ATOMIC_START();
    cx->status |= STATUS_OPTIMIZATION_DISABLED;
    uint16_t wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
    if (wp != cx->buf_rp) {
        cx->buf[cx->buf_wp] = m;
        cx->buf_wp = wp;
        ret = MIDI_RET_OK;
    }
    mUtilisationUpdate(cx, m);

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
void midiPortFlush(MidiOutPortT* p)
{
    MidiOutPortContextT* cx = p->context;
    MIDI_ATOMIC_START();
    cx->buf_rp = cx->buf_wp;
    cx->sysex_rp = cx->sysex_wp;
    cx->sysex_cn = SYSEX_CN_UNLOCK;
    cx->status = STATUS_NRPNUNDEF;
    MIDI_ATOMIC_END();
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
        uint8_t cc_msk = m.byte2 & 0x31;
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
    MIDI_ASSERT(((pos - cx->buf_rp) & MIDI_TX_BUFFER_SIZE) < ((cx->buf_wp - cx->buf_rp) & MIDI_TX_BUFFER_SIZE));
    cx->messages_flushed++;

    // scan and shift form pos to rp
    for (uint16_t i = pos; i != cx->buf_rp; i = (i - 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
        cx->buf[i] = cx->buf[(i - 1) & (MIDI_TX_BUFFER_SIZE - 1)];
    }
    cx->buf_rp = (cx->buf_rp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
}

// return MIDI_RET_OK if there is available space in buffer
static inline MidiRet mFlushLowestPrio(MidiOutPortContextT* cx, uint16_t new_prio)
{
    MidiRet ret = MIDI_RET_FAIL;
    uint16_t lowest_prio = new_prio;
    uint16_t lowest_prio_pos;

    for (uint16_t i = cx->buf_rp; i != cx->buf_wp; i = (i + 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
        uint16_t prio = mGetMessagePrio(cx->buf[i]);
        if (prio < lowest_prio) {
            lowest_prio = prio;
            lowest_prio_pos = i;
            ret = MIDI_RET_OK;
        }
    }

    uint16_t utilisation = ((cx->buf_wp - cx->buf_rp) & (MIDI_TX_BUFFER_SIZE - 1)) + 1;
    if (lowest_prio < utilisation) {
        if (MIDI_RET_OK == ret) {
            mFlushMessage(cx, lowest_prio_pos);
        }
        return ret;
    } else {
        return MIDI_RET_OK;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static MidiRet mOptWr________na(MidiOutPortContextT* cx, MidiMessageT m)
{
    MIDI_ASSERT(0);
    (void)cx;
    (void)m;
    return MIDI_RET_FAIL;
}

// CC is the most complex one
// it uses rpn filtering, repeated messages filtering and priority filtering
static MidiRet mOptWr________CC(MidiOutPortContextT* cx, MidiMessageT m)
{
    // rpn filtering
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
    const uint32_t nrpn_addr_bmp3 = 0x0000003C;
    const uint32_t nrpn_al_bmp3 = 0x00000014;
    const uint32_t nrpn_ah_bmp3 = 0x00000028;
    const uint32_t compare_mask = m_compare_mask[MIDI_CIN_CONTROLCHANGE];
    uint8_t status_addr = 0;
    // set proper address status flags
    if (cc_pos == 3) {
        if ((nrpn_al_bmp3 >> cc_msk) & 0x1) {
            status_addr = STATUS_NRPALFAIL;
        } else if ((nrpn_ah_bmp3 >> cc_msk) & 0x1) {
            status_addr = STATUS_NRPAHFAIL;
        }
    }
    if ((nrpn_dat_bmp[cc_pos] >> cc_msk) & 0x1) {
        // if this is NRPN DATA
        // check that address is correct
        if (cx->status & STATUS_NRPNUNDEF) {
            cx->messages_flushed++;
            return MIDI_RET_FAIL;
        }
        // scan buffer only up to the last address set, do not replace data of different addresses
        for (uint16_t i = (cx->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
             i != ((cx->buf_rp - 1) & (MIDI_TX_BUFFER_SIZE - 1));
             i = (i - 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
            // check that current position is not an address, then we can replace
            cc_pos = cx->buf[i].byte2 >> 5;
            cc_msk = cx->buf[i].byte2 & 0x31;
            if ((cc_pos == 3) && ((nrpn_addr_bmp3 >> cc_msk) & 1)) {
                break;
            } else {
                if (((cx->buf[i].full_word ^ m.full_word) & compare_mask) == 0) {
                    cx->messages_optimized++;
                    mFlushMessage(cx, i);
                    cx->buf[cx->buf_wp] = m;
                    cx->buf_wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
                    return MIDI_RET_OK;
                }
            }
        }
    } else {
        //  this is NRPN address, channel mode, or just a regular CC
        //  scan the whole buffer from end to start
        for (uint16_t i = (cx->buf_wp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
             i != ((cx->buf_rp - 1) & (MIDI_TX_BUFFER_SIZE - 1));
             i = (i - 1) & (MIDI_TX_BUFFER_SIZE - 1)) {

            // optimize repeated events, in case they are not nrpn addresses
            const uint32_t compare_mask = m_compare_mask[MIDI_CIN_CONTROLCHANGE];
            if (((cx->buf[i].full_word ^ m.full_word) & compare_mask) == 0) {
                if (status_addr) {
                    // in case this is address, delete new message, and set addr fail
                    cx->status |= status_addr;
                    cx->messages_flushed++;
                    return MIDI_RET_FAIL;
                } else {
                    // this is not an NRPN address
                    cx->messages_optimized++;
                    // mFlushMessage(cx, i); // should we keep order?
                    // cx->buf[cx->buf_wp] = m;
                    // cx->buf_wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
                    cx->buf[i] = m; // let's simplify!
                    return MIDI_RET_OK;
                }
            }
            // else check next message
        }
        // there was no repeated events
    }

    // 3rd stage - prio filtering
    MidiRet ret = MIDI_RET_FAIL;
    if (MIDI_RET_OK == mFlushLowestPrio(cx, mGetMessagePrio(m))) {
        cx->buf[cx->buf_wp] = m;
        cx->buf_wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
        cx->status &= ~status_addr;
        ret = MIDI_RET_OK;
    } else {
        cx->status |= status_addr;
        cx->messages_flushed++;
    }
    return ret;
}

// next one if Regular - a little bit harder
// it uses similar messages filtering and priority filtering
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
            mFlushMessage(cx, i);
            cx->buf[cx->buf_wp] = m;
            cx->buf_wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
            return MIDI_RET_OK;
        }
    }

    // filter by priority
    MidiRet ret = MIDI_RET_FAIL;
    if (MIDI_RET_OK == mFlushLowestPrio(cx, mGetMessagePrio(m))) {
        cx->buf[cx->buf_wp] = m;
        cx->buf_wp = (cx->buf_wp + 1) & (MIDI_TX_BUFFER_SIZE - 1);
        ret = MIDI_RET_OK;
    } else {
        cx->messages_flushed++;
    }
    return ret;
}

// first of all is a Single - the simplest one
// the only optimisation it uses is priority filtering
static MidiRet mOptWr____Single(MidiOutPortContextT* cx, MidiMessageT m)
{
    MidiRet ret = MIDI_RET_FAIL;
    if (MIDI_RET_OK == mFlushLowestPrio(cx, m_cin_priorities[MIDI_CIN_SINGLEBYTE])) {
        for (uint16_t i = cx->buf_rp; i != cx->buf_wp; i = (i + 1) & (MIDI_TX_BUFFER_SIZE - 1)) {
            const unsigned t_rt = CINBMP_SINGLEBYTE | CINBMP_2BYTESYSTEMCOMMON | CINBMP_3BYTESYSTEMCOMMON;
            if ((t_rt >> cx->buf[i].cin) & 1) {
                // same kind of message, shift it, se we can insert after
                cx->buf[(i - 1) & (MIDI_TX_BUFFER_SIZE - 1)] = cx->buf[i];
            } else {
                cx->buf[(i - 1) & (MIDI_TX_BUFFER_SIZE - 1)] = m;
                ret = MIDI_RET_OK;
                break;
            }
        }
        cx->buf_rp = (cx->buf_rp - 1) & (MIDI_TX_BUFFER_SIZE - 1);
    } else {
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

            uint16_t utilisation = ((cx->sysex_wp - cx->sysex_rp) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1)) + 1;
            if (utilisation > cx->max_syx_utilisation) {
                cx->max_syx_utilisation = utilisation;
            }

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

            uint16_t utilisation = ((cx->sysex_wp - cx->sysex_rp) & (MIDI_TX_SYSEX_BUFFER_SIZE - 1)) + 1;
            if (utilisation > cx->max_syx_utilisation) {
                cx->max_syx_utilisation = utilisation;
            }

            ret = MIDI_RET_OK;
        }
        cx->sysex_cn = SYSEX_CN_UNLOCK;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// MidiRet midi_port_check_rt(MidiOutPortT* p, MidiMessageT* m)
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

MidiRet midiPortReadNext(MidiOutPortT* p, MidiMessageT* m)
{
    MidiRet ret = MIDI_RET_FAIL;
    MidiOutPortContextT* cx = p->context;
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
                return MIDI_RET_OK;
            }
        }
    }
    m->cn = p->cn; // this is ok
    MIDI_ATOMIC_END();
    return ret;
}

#if (MIDI_TX_BUFFER_SIZE & (MIDI_TX_BUFFER_SIZE - 1)) != 0
#error "output buffer: please, use only power of 2"
#endif

#if (MIDI_TX_SYSEX_BUFFER_SIZE & (MIDI_TX_SYSEX_BUFFER_SIZE - 1)) != 0
#error "output sysex buffer: please, use only power of 2"
#endif 

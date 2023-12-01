

#include "midi_input.h"
#include "midi_internals.h"

// unsigned power of 2 only!
#define MIDI_RX_BUFFER_SIZE (256)
#define MIDI_RX_SYSEX_BUFFER_SIZE (256)


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

#if (MIDI_RX_BUFFER_SIZE & (MIDI_RX_BUFFER_SIZE - 1)) != 0
#error "input buffer: please, use only power of 2"
#endif 

#if (MIDI_RX_SYSEX_BUFFER_SIZE & (MIDI_RX_SYSEX_BUFFER_SIZE - 1)) != 0
#error "input sysex buffer: please, use only power of 2"
#endif 

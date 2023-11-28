#include "midi_cvgate.h"
#include "math.h"
// #include "cmsis_gcc.h"

uint16_t fast_exp2_16bit(uint16_t arg)
{
    uint32_t e = arg / 4096;
    uint32_t s = arg & (4096 - 1);
    uint32_t comp = s > 2048 ? 4096 - s : s;
    comp = 2048 - comp;
    comp = comp * comp / 2048;
    comp = 2048 - comp;
    e = 1 << e;
    s = s * e / 4096;
    comp = comp * e / 23808;
    e = e + s - comp;
    return (uint16_t)e;
}

uint16_t fast_exp2_32bit(uint32_t arg)
{
    uint32_t e = arg / 0x08000000;
    uint32_t s = arg & (0x08000000 - 1);
    uint32_t comp = s > 0x04000000 ? 0x08000000 - s : s;
    comp = 0x04000000 - comp;
    comp = (uint64_t)comp * comp / 0x04000000;
    comp = 0x04000000 - comp;
    e = 1 << e;
    s = (uint64_t)s * e / 0x08000000;
    comp = ((uint64_t)comp * e) >> 16;
    comp = comp / 0x2e80;
    e = e + s - comp;
    return (uint16_t)e;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   ####   #   #    ####    ###    #####   #####//////////////////////////////////////////////////////////////////////////////////////////////
//  #       #   #   #       #   #     #     #   ///////////////////////////////////////////////////////////////////////////////////////////////
//  #       #   #   #  ##   #####     #     ### ////////////////////////////////////////////////////////////////////////////////////////////////
//  #        # #    #   #   #   #     #     #   ///////////////////////////////////////////////////////////////////////////////////////////////
//   ####     #      ####   #   #     #     #####//////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float exp2f(float v)
{
    return v;
    // TODO: replace this!!!
}

void midi_cv_set_glide(midi_cvout_voiceblock_t* vb, uint16_t val)
{
    // from 2 to 16384
    float v = (float)val / (65536.0f + 4682.0f) * 14.0f + 14.0f * 4682.0f;
    v = exp2f(v);
    vb->glidecoef = (uint16_t)v;
}

void midi_voiceblock_init(midi_cvout_voiceblock_t* vb)
{
    vb->damperstate = 0;
    vb->keycount = 0;
    vb->velo_goal = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 3kHz audio interrupt

#ifndef __USAT
#define __USAT(value, bit)          \
    do {                            \
        if (value > (1 << bit)) {   \
            value = (1 << bit) - 1; \
        }                           \
    } while (0)
#endif

void midi_cr_cvgprocess(midi_cvout_voiceblock_t* vb)
{
    // TODO: multiply
    vb->pitch_slide += (vb->pitch_goal + vb->glidecoef - vb->pitch_slide) / vb->glidecoef;
    vb->velo_slide += (vb->velo_goal + vb->glidecoef - vb->velo_slide) / vb->glidecoef;
    if (vb->retrigrequest) {
        vb->retrigrequest--;
        vb->out[MIDI_CVGVB_CH_VELO] = 0;
        vb->out[MIDI_CVGVB_CH_VELO_LAST] = 0;
    } else {
        vb->out[MIDI_CVGVB_CH_VELO] = vb->velo_slide >> 15;
        vb->out[MIDI_CVGVB_CH_VELO_LAST] = vb->velo_last << 9;
    }
    int32_t pitch = vb->pitch_slide + vb->pwshift;
    if (pitch > 0)
        __USAT(pitch, 30);
    else
        pitch = 0;
    vb->out[MIDI_CVGVB_CH_PITCH] = vb->pitch_slide >> 14;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sequencer output
static void mcc_default(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    (void)mes;
    (void)vb;
}
static void mcc_dat_mw_h(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    vb->out[MIDI_CVGVB_CH_MODWHEEL] &= 0x01FF;
    vb->out[MIDI_CVGVB_CH_MODWHEEL] |= mes.byte3 << 9;
}
static void mcc_dat_mw_l(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    vb->out[MIDI_CVGVB_CH_MODWHEEL] &= 0xFE00;
    vb->out[MIDI_CVGVB_CH_MODWHEEL] |= mes.byte3 << 2;
}
static void mcc_dat_damperpedal(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    if (mes.byte3 > 0x40) {
        vb->damperstate = 1;
    } else {
        vb->damperstate = 0;
        if (vb->keycount == 0) {
            vb->velo_goal = 0;
        }
    }
}
static void mcc_dat_breath_h(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    vb->out[MIDI_CVGVB_CH_BREATH] &= 0x01FF;
    vb->out[MIDI_CVGVB_CH_BREATH] |= mes.byte3 << 9;
}
static void mcc_dat_breath_l(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    vb->out[MIDI_CVGVB_CH_BREATH] &= 0xFE00;
    vb->out[MIDI_CVGVB_CH_BREATH] |= mes.byte3 << 2;
}
static void mcc_panic(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    midi_voiceblock_init(vb);
    (void)mes;
}

typedef enum {
    MCC__DEFAULT = 0,
    MCC_MODWHL_H,
    MCC_MODWHL_L,
    MCC_DMPRPEDL,
    MCC_ALLSNDOF,
    MCC_ALLNTSOF,
    MCC_BREATH_H,
    MCC_BREATH_L,
    MCC_TOTAL
} midi_noncc_en;
static void (*const ccn[MCC_TOTAL])(MidiMessageT mes, midi_cvout_voiceblock_t* vb) = {
    mcc_default, // MCC__DEFAULT
    mcc_dat_mw_h, // MCC_MODWHL_H
    mcc_dat_mw_l, // MCC_MODWHL_L
    mcc_dat_damperpedal, // MCC_DMPRPEDL
    mcc_panic, // MCC_ALLSNDOF
    mcc_panic, // MCC_ALLNTSOF
    mcc_dat_breath_h,
    mcc_dat_breath_l,
};
static const uint8_t cc_table[128] = {
    MCC__DEFAULT, MCC_MODWHL_H, MCC_BREATH_H, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //   0  0x00
    MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //   8  0x08
    MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //  16  0x10
    MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //  24  0x18
    MCC__DEFAULT, MCC_MODWHL_L, MCC_BREATH_L, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //  32  0x20
    MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //  40  0x28
    MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //  48  0x30
    MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //  56  0x38
    MCC_DMPRPEDL, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //  64  0x40
    MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //  72  0x48
    MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //  80  0x50
    MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //  88  0x58
    MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, //  96  0x60
    MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, // 104  0x68
    MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, // 112  0x70
    MCC_ALLSNDOF, MCC__DEFAULT, MCC__DEFAULT, MCC_ALLNTSOF, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, MCC__DEFAULT, // 120  0x78
};

static void mtcv_na(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    (void)mes;
    (void)vb;
}
static void mtcv_pb(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    int16_t value = (mes.byte2 | (mes.byte3 << 7)) - 0x2000;
    vb->pwshift = vb->pw_range * value * 0x400;
}
static void mtcv_at(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    vb->out[MIDI_CVGVB_CH_AFTERTOUCH] = mes.byte2 << 9;
}
static void mtcv_cc(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    ccn[cc_table[mes.byte2]](mes, vb);
}

static void mtcv_non(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    uint8_t i;
    uint8_t note = mes.byte2;
    uint8_t velo = mes.byte3;
    if (vb->keycount < NOTE_HOLD_MAX_MEMORY) {
        vb->keycount++;
    }
    for (i = vb->keycount; i > 0; i--) // scan from bottom to top
    {
        vb->notememory[i] = vb->notememory[i - 1]; // shift the whole used part of buffer down (from the priority pov)
    }
    vb->notememory[0].note = note; // and put new note at the top
    vb->notememory[0].velocity = velo;
    vb->pitch_goal = note << 23;
    vb->velo_goal = velo << 24;
    if ((vb->gateretrig) || ((vb->keycount == 1) && (vb->damperstate == 0))) {
        vb->pitch_slide = note << 23;
        vb->velo_slide = velo << 24;
        vb->retrigrequest = 2;
    }
}
static void mtcv_noff(MidiMessageT mes, midi_cvout_voiceblock_t* vb)
{
    uint8_t i;
    uint8_t note = mes.byte2;
    for (i = 0; i < vb->keycount; i++) // scan note buffer
    {
        if (vb->notememory[i].note == note) // if we found note, release it...
        {
            vb->keycount--; // decrement
            for (; i < vb->keycount; i++) {
                vb->notememory[i] = vb->notememory[i + 1]; // shift other notes
            }
            // i == notes holded, exit
            if (i == 0) {
                // retrig
                if (vb->keycount) {
                    if (vb->gateretrig) {
                        vb->pitch_slide = vb->notememory[0].note << 23;
                        vb->velo_slide = vb->notememory[0].velocity << 24;
                        vb->retrigrequest = 2;
                    }
                    vb->pitch_goal = vb->notememory[0].note << 23;
                    vb->velo_goal = vb->notememory[0].velocity << 24;
                } else {
                    if (vb->damperstate == 0) {
                        vb->velo_goal = 0;
                    }
                }
            }
        }
    }
}
static void (*const midi_tocvg[16])(MidiMessageT mes, midi_cvout_voiceblock_t* vb) = {
    mtcv_na, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.        | not used at all
    mtcv_na, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.                         | not used at all
    mtcv_na, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.                   | systemonly
    mtcv_na, // 0x3 3 Three-byte System Common messages like SPP, etc.                             | systemonly
    mtcv_na, // 0x4 3 SysEx starts or continues                                                    | maybe seq, or systemonly
    mtcv_na, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.  | maybe seq, or systemonly
    mtcv_na, // 0x6 2 SysEx ends with following two bytes.                                         | maybe seq, or systemonly
    mtcv_na, // 0x7 3 SysEx ends with following three bytes.                                       | maybe seq, or systemonly
    mtcv_noff, // 0x8 3 Note-off                                                                     | seq
    mtcv_non, // 0x9 3 Note-on                                                                      | seq
    mtcv_na, // 0xA 3 Poly-KeyPress                                                                | seq
    mtcv_cc, // 0xB 3 Control Change                                                               | seq
    mtcv_na, // 0xC 2 Program Change                                                               | seq
    mtcv_at, // 0xD 2 Channel Pressure                                                             | seq
    mtcv_pb, // 0xE 3 PitchBend Change                                                             | seq
    mtcv_na // 0xF 1 Single Byte                                                                  | systemonly
};

void midi_cv_transmit(midi_cvout_voiceblock_t* vb, MidiMessageT m)
{
    if (m.cn == vb->channel)
        midi_tocvg[m.cin](m, vb);
}
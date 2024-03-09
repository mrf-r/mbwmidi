#include "midi_cvgate.h"
#include "math.h"
// #include "cmsis_gcc.h"

#ifdef MIDI_CV_RETRIG_OPTION
#ifndef RETRIG_TIME_CR_CYCLES
#define RETRIG_TIME_CR_CYCLES 2
#endif
#endif // MIDI_CV_RETRIG_OPTION

// fast approximation of exp2(x/4096)
static inline uint16_t exp2_fast(uint16_t x)
{
    uint32_t e = x / 4096;
    uint32_t s = x & (4096 - 1);
    e = 1 << e;
    s = s * e / 4096;
    e = e + s;
    return (uint16_t)e;
}

// left aligned
// 0 - shortest
// 65535 - longest
static inline void midiCvSetGlide(MidiCvOutVoice* v, const uint16_t pv)
{
    /*
    scaling coefficients calculation for frequency or time dial
    1 - define overall range (range = max_value / min_value (~10000))
    2 - find k (k = np.log2(range)/128*4096)
    3 - find b (b = np.log2(min_value))
    that's it!
    */
    const uint32_t k = 430 * 128; // limits max time
    uint32_t arg = 65535 - k * pv / 65536;
    // setting w is basically the SR scaling: w = freq / SR
    // here we can just shift b
    v->glidew = exp2_fast(arg);
}

void midiCvInit(MidiCvOutVoice* v)
{
    v->pwrange = 12 << 9;
    v->damperstate = 0;
    v->keycount = 0;
    v->velo_last = 0;
    v->velo_goal = 0;
    v->velo_slide = 0;
}
// this must be called on every CR
void midiCvTap(MidiCvOutVoice* v)
{
    v->pitch_slide += (v->pitch_goal - v->pitch_slide) / 65536 * v->glidew;
    v->velo_slide += (v->velo_goal - v->velo_slide) / 65536 * v->glidew;
    int32_t pitch = v->pitch_slide / 32768 + v->pwshift;
    if (pitch < 0) {
        pitch = 0;
    } else if (pitch > 65535) {
        pitch = 65535;
    }
    v->out[MIDI_CV_CH_PITCH] = pitch;
#ifdef MIDI_CV_RETRIG_OPTION
    if (v->retrigrequest) {
        v->retrigrequest--;
        v->out[MIDI_CV_CH_VELO] = 0;
        v->out[MIDI_CV_CH_VELO_LAST] = 0;
    } else {
        v->out[MIDI_CV_CH_VELO_LAST] = v->velo_last << 9;
        v->out[MIDI_CV_CH_VELO] = v->velo_slide / 32768;
    }
#else
    v->out[MIDI_CV_CH_VELO_LAST] = v->velo_last << 9;
    v->out[MIDI_CV_CH_VELO] = v->velo_slide / 32768;
#endif // MIDI_CV_RETRIG_OPTION
}

static void mCv_cc(MidiMessageT m, MidiCvOutVoice* v)
{
    switch (m.byte2) {
    case 0x1: // modwheel
        v->out[MIDI_CV_CH_MODWHEEL] &= 0x01FF;
        v->out[MIDI_CV_CH_MODWHEEL] |= m.byte3 << 9;
        break;
    case 0x21:
        v->out[MIDI_CV_CH_MODWHEEL] &= 0xFE00;
        v->out[MIDI_CV_CH_MODWHEEL] |= m.byte3 << 2;
        break;
    case 0x2: // breath
        v->out[MIDI_CV_CH_BREATH] &= 0x01FF;
        v->out[MIDI_CV_CH_BREATH] |= m.byte3 << 9;
        break;
    case 0x22:
        v->out[MIDI_CV_CH_BREATH] &= 0xFE00;
        v->out[MIDI_CV_CH_BREATH] |= m.byte3 << 2;
        break;
    case 0x3: // undefined
        // let it be pitch bend range in semitones
        v->pwrange = (m.byte3 >> 1) << 9;
        break;
    case 0x5: // portamento
        v->portamento &= 0x01FF;
        v->portamento |= m.byte3 << 9;
        midiCvSetGlide(v, v->portamento);
        break;
    case 0x25:
        v->portamento &= 0xFE00;
        v->portamento |= m.byte3 << 2;
        midiCvSetGlide(v, v->portamento);
        break;
    case 0x40: // damper
        if (m.byte3 > 0x40) {
            v->damperstate = 1;
        } else {
            v->damperstate = 0;
            if (v->keycount == 0) {
                v->velo_last = 0;
                v->velo_goal = 0;
                v->velo_slide = 0;
            }
        }
        break;
    case 0x70: // ASO
    case 0x73: // ANO
        v->damperstate = 0;
        v->keycount = 0;
        v->velo_last = 0;
        v->velo_goal = 0;
        v->velo_slide = 0;
        break;
#ifdef MIDI_CV_RETRIG_OPTION
    case 0x7E: // mono
        v->gateretrig = 0;
        break;
    case 0x7F: // poly
        v->gateretrig = 1;
        break;
#endif // MIDI_CV_RETRIG_OPTION
    default:
        break;
    }
}

static void mCv_na(MidiMessageT m, MidiCvOutVoice* v)
{
    (void)m;
    (void)v;
}
static void mCv_pb(MidiMessageT m, MidiCvOutVoice* v)
{
    int16_t value = (m.byte2 | (m.byte3 << 7)) - 0x2000;
    v->pwshift = (int32_t)v->pwrange * value / 0x2000;
}
static void mCv_at(MidiMessageT m, MidiCvOutVoice* v)
{
    v->out[MIDI_CV_CH_AFTERTOUCH] = m.byte2 << 9;
}

static void mCv_non(MidiMessageT m, MidiCvOutVoice* v)
{
    uint8_t note = m.byte2;
    uint8_t velo = m.byte3;
    if (v->keycount < NOTE_HOLD_MAX_MEMORY) {
        v->keycount++;
    }
    // scan from bottom to top
    for (int i = v->keycount; i > 0; i--) {
        // shift the whole used part of buffer down (from the priority pov)
        v->notememory[i] = v->notememory[i - 1];
    }
    // and put new note at the top
    v->notememory[0].note = note;
    v->notememory[0].velocity = velo;
    v->pitch_goal = note << 24;
    v->velo_goal = velo << 24;
    v->velo_last = velo;
#ifdef MIDI_CV_RETRIG_OPTION
    if ((v->gateretrig) || ((v->keycount == 1) && (v->damperstate == 0))) {
        v->pitch_slide = note << 24;
        v->velo_slide = velo << 24;
        v->retrigrequest = RETRIG_TIME_CR_CYCLES;
    }
#endif // MIDI_CV_RETRIG_OPTION
}
static void mCv_noff(MidiMessageT m, MidiCvOutVoice* v)
{
    uint8_t note = m.byte2;
    // scan note buffer
    for (int i = 0; i < v->keycount; i++) {
        // if we found note, release it...
        if (v->notememory[i].note == note) {
            v->keycount--; // decrement
            for (; i < v->keycount; i++) {
                // shift other notes
                v->notememory[i] = v->notememory[i + 1];
            }
            // i == notes holded, exit
            if (i == 0) {
                // retrig
                if (v->keycount) {
                    v->pitch_goal = v->notememory[0].note << 24;
                    v->velo_goal = v->notememory[0].velocity << 24;
#ifdef MIDI_CV_RETRIG_OPTION
                    if (v->gateretrig) {
                        v->pitch_slide = v->notememory[0].note << 24;
                        v->velo_slide = v->notememory[0].velocity << 24;
                        v->retrigrequest = RETRIG_TIME_CR_CYCLES;
                    }
#endif // MIDI_CV_RETRIG_OPTION
                } else {
                    if (v->damperstate == 0) {
                        v->velo_last = 0;
                        v->velo_goal = 0;
                        v->velo_slide = 0;
                    }
                }
            }
        }
    }
}

static void (*const midi_tocv[16])(MidiMessageT m, MidiCvOutVoice* v) = {
    mCv_na, // 0x0 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions.        | not used at all
    mCv_na, // 0x1 1, 2 or 3 Cable events. Reserved for future expansion.                         | not used at all
    mCv_na, // 0x2 2 Two-byte System Common messages like MTC, SongSelect, etc.                   | systemonly
    mCv_na, // 0x3 3 Three-byte System Common messages like SPP, etc.                             | systemonly
    mCv_na, // 0x4 3 SysEx starts or continues                                                    | maybe seq, or systemonly
    mCv_na, // 0x5 1 Single-byte System Common Message or SysEx ends with following single byte.  | maybe seq, or systemonly
    mCv_na, // 0x6 2 SysEx ends with following two bytes.                                         | maybe seq, or systemonly
    mCv_na, // 0x7 3 SysEx ends with following three bytes.                                       | maybe seq, or systemonly
    mCv_noff, // 0x8 3 Note-off                                                                     | seq
    mCv_non, // 0x9 3 Note-on                                                                      | seq
    mCv_na, // 0xA 3 Poly-KeyPress                                                                | seq
    mCv_cc, // 0xB 3 Control Change                                                               | seq
    mCv_na, // 0xC 2 Program Change                                                               | seq
    mCv_at, // 0xD 2 Channel Pressure                                                             | seq
    mCv_pb, // 0xE 3 PitchBend Change                                                             | seq
    mCv_na // 0xF 1 Single Byte                                                                  | systemonly
};

void midiCvHandleMessage(MidiCvOutVoice* v, MidiMessageT m)
{
    if (m.cn == v->channel)
        midi_tocv[m.cin](m, v);
}
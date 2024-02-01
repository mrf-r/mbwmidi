#ifndef _CELLMEM_H
#define _CELLMEM_H

// this is limited to 65536 cells
// max: 65535 cells, bmpsize: 2048 (8192 B), array with 64bit cells: 512kB, total: 520 kB
// 64k:  8191 cells, bmpsize:  256 (1024 B), array with 64bit cells:  64kB, total:  65 kB
//  8k:  1023 cells, bmpsize:   32 ( 128 B), array with 64bit cells:   8kB, total:  ~9 kB
// max bitmap size: 8kB (2048 32bit words)
// max data size for 64bit cell: 512kB

#include <stdint.h>
#include "midi_conf.h" // MIDI_ASSERT, __CLZ

// TODO: put this method in some global space
static inline unsigned _clz_sw(uint32_t x)
{
    static const uint8_t lut[] = {
        32, 31, 30, 30, 29, 29, 29, 29, 28, 28, 28, 28, 28, 28, 28, 28
    };
    unsigned n;
    if (x < (1 << 16)) {
        if (x < (1 << 8)) {
            if (x < (1 << 4)) {
                n = 0;
            } else {
                n = 4;
            }
        } else {
            if (x < (1 << 12)) {
                n = 8;
            } else {
                n = 12;
            }
        }
    } else {
        if (x < (1 << 24)) {
            if (x < (1 << 20)) {
                n = 16;
            } else {
                n = 20;
            }
        } else {
            if (x < (1 << 28)) {
                n = 24;
            } else {
                n = 28;
            }
        }
    }
    return lut[x >> n] - n;
}

#ifndef __CLZ
// #define __CLZ (uint8_t) __builtin_clz // undefined when arg == 0
#define __CLZ (uint8_t) _clz_sw
#endif

// working array usage will be reduced to the nearest multiple of 32
#define CM_BMPSIZE(cells) ((unsigned)(cells / 32))

// 1st: define array length: | #define ELEMENTS 192
// 2nd: create bitmap:       | uint32_t cm_bitmap[CM_BMPSIZE(ELEMENTS)];
// 3rd: create array:        | MyElement my_elements[ELEMENTS];
// 4th: init bitmap:         | cmInit(cm_bitmap, CM_BMPSIZE(ELEMENTS));
// 5th: use:                 | position = cmAlloc(cm_bitmap, CM_BMPSIZE(ELEMENTS));
//                           | if (position) { my_elements[position]; } else { Error(); }
//                           | cmFree(cm_bitmap, position);

static inline void cmInit(uint32_t* const bmp, const unsigned bmpsize)
{
    for (int i = 0; i < bmpsize; i++) {
        bmp[i] = 0xFFFFFFFF;
    }
    bmp[0] = 0x7FFFFFFF; // first element always 0
}

// get the position of the new element in the array according to the bitmap
// returns 0 if there is no free space left
static inline unsigned cmAlloc(uint32_t* const bmp, const unsigned bmpsize)
{
    MIDI_ASSERT((bmp[0] & 0x80000000) == 0); // 1st cell is never empty
    static unsigned first_try_bmp_pos = 0;
    unsigned bmp_pos = first_try_bmp_pos;
    unsigned ret_cell = 0;
    do {
        uint8_t cell = __CLZ(bmp[bmp_pos]);
        if (cell < 32) {
            bmp[bmp_pos] &= ~(1 << (31 - cell)); // allocate position
            ret_cell = cell + 32 * bmp_pos;
        }
        bmp_pos++;
        if (bmp_pos == bmpsize)
            bmp_pos = 0;

    } while ((0 == ret_cell) && (bmp_pos != first_try_bmp_pos));
    first_try_bmp_pos = bmp_pos;
    return ret_cell;
}

static inline void cmFree(uint32_t* const bmp, unsigned pos)
{
    MIDI_ASSERT(pos);
    MIDI_ASSERT((bmp[pos / 32] & (1 << (31 - (pos & 31)))) == 0);
    bmp[pos / 32] |= 1 << (31 - (pos & 31));
}

static inline unsigned cmCountFreeCells(uint32_t* const bmp, const unsigned bmpsize)
{
    unsigned utilisation = 0;
    for (unsigned i = 0; i < bmpsize; i++) {
        for (unsigned ii = 0; ii < 32; ii++) {
            if (bmp[i] & (1 << ii)) {
                utilisation++;
            }
        }
    }
    return utilisation;
}
#endif // _CELLMEM_H
/*
Copyright (C) 2024 Eugene Chernyh (mrf-r)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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
    static const uint8_t lut[256] = {
        32, 31, 30, 30, 29, 29, 29, 29, 28, 28, 28, 28, 28, 28, 28, 28, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 25, 25, 25, 25, 25, 25, 25, 25,
        25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
        25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
        25, 25, 25, 25, 25, 25, 25, 25, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24
    };
    unsigned n;
    if (x < (1 << 16)) {
        if (x < (1 << 8)) {
            n = 0;
        } else {
            n = 8;
        }
    } else {
        if (x < (1 << 24)) {
            n = 16;
        } else {
            n = 24;
        }
    }
    return lut[x >> n] - n;
}
static inline unsigned _clz_gcc(uint32_t x)
{
    if (x)
        return __builtin_clz(x);
    else
        return 32;
}

#ifndef __CLZ
// #define __CLZ (uint8_t) _clz_gcc
#define __CLZ (uint8_t) _clz_sw
#endif

// working array usage will be reduced to the nearest multiple of 32
#define CM_BMPSIZE(cells) ((unsigned)(cells / 32))

// define array length: | #define ELEMENTS 192
// create bitmap:       | uint32_t cm_bitmap[CM_BMPSIZE(ELEMENTS)];
// create prev_pos var: | unsigned cm_prev_pos;
// create array:        | MyElement my_elements[ELEMENTS];
// init bitmap:         | cmInit(cm_bitmap, &cm_prev_pos, CM_BMPSIZE(ELEMENTS));
// use:                 | index = cmAlloc(cm_bitmap, &cm_prev_pos, CM_BMPSIZE(ELEMENTS));
//                      | if (index) { my_elements[index]; } else { Error(); }
//                      | cmFree(cm_bitmap, index);

// prev_pos is used keep the previous bitmap position
static inline void cmInit(uint32_t* const bmp, unsigned* const prev_pos, const unsigned bmpsize)
{
    MIDI_ASSERT(prev_pos);
    for (int i = 0; i < bmpsize; i++) {
        bmp[i] = 0xFFFFFFFF;
    }
    bmp[0] = 0x7FFFFFFF; // first element always 0
    *prev_pos = 0;
}

// get the index of the new element in the array according to the bitmap
// prev_pos is just a helper variable.
// returns 0 if there is no free space left
static inline unsigned cmAlloc(uint32_t* const bmp, unsigned* const prev_pos, const unsigned bmpsize)
{
    MIDI_ASSERT((bmp[0] & 0x80000000) == 0); // 1st cell is never empty
    MIDI_ASSERT(prev_pos);
    MIDI_ASSERT(*prev_pos < bmpsize);
    unsigned bmp_pos = *prev_pos;
    unsigned ret_cell = 0;
    do {
        unsigned cell = __CLZ(bmp[bmp_pos]);
        if (cell < 32) {
            bmp[bmp_pos] &= ~(1 << (31 - cell)); // allocate index
            ret_cell = cell + 32 * bmp_pos;
        }
        bmp_pos++;
        if (bmp_pos == bmpsize)
            bmp_pos = 0;

    } while ((0 == ret_cell) && (bmp_pos != *prev_pos));
    *prev_pos = bmp_pos;
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
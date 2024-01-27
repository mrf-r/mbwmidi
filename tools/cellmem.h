#ifndef _CELLMEM_H
#define _CELLMEM_H

// this is limited to 65536 cells
// max: 65535 cells, bmpsize: 2048 (8192 B), array with 64bit cells: 512kB, total: 520 kB
// 64k:  8192 cells, bmpsize:  256 (1024 B), array with 64bit cells:  64kB, total:  65 kB
//  8k:  1024 cells, bmpsize:   32 ( 128 B), array with 64bit cells:   8kB, total:  ~9 kB
// max bitmap size: 8kB (2048 32bit words)
// max data size for 64bit cell: 512kB

#include <stdint.h>
#include "midi_conf.h" // MIDI_ASSERT

#ifndef __CLZ
#define __CLZ (uint8_t) __builtin_clz
#endif

#define CM_BMPSIZE(cells) ((cells + 31) / 32)

// 1st: define array length: | #define ELEMENTS 192
// 2nd: create bitmap:       | uint32_t cm_bitmap[CM_BMPSIZE(ELEMENTS)];
// 3rd: create array:        | MyElement my_elements[ELEMENTS];
// 4th: init bitmap:         | cmInit(cm_bitmap, CM_BMPSIZE(ELEMENTS));
// 5th: use:                 | position = cmAlloc(cm_bitmap, CM_BMPSIZE(ELEMENTS));
//                           | if (position) { my_elements[position]; } else { Error(); }
//                           | cmFree(cm_bitmap, position);

static inline void cmInit(uint32_t* bmp, size_t bmpsize)
{
    for (int i = 0; i < bmpsize; i++) {
        bmp[i] = 0xFFFFFFFF;
    }
    bmp[0] = 0x7FFFFFFF; // first element always 0
}

// get the position of the new element in the array according to the bitmap
// returns 0 if there is no free space left
static inline uint16_t cmAlloc(uint32_t* bmp, size_t bmpsize)
{
    MIDI_ASSERT((bmp[0] & 0x80000000) == 0); // 1st is never empty
    for (int i = 0; i < bmpsize; i++) {
        uint8_t cpos = __CLZ(bmp[i]);
        if (cpos < 32) {
            bmp[i] &= ~(1 << (31 - cpos)); // allocate position
            uint16_t pos = cpos + 32 * i;
            return pos;
        }
    }
    return 0;
}

static inline void cmFree(uint32_t* bmp, uint16_t pos)
{
    MIDI_ASSERT(pos);
    bmp[pos / 32] |= 1 << (31 - (pos & 31));
}

#endif // _CELLMEM_H
#include "../../tools/cellmem.h"
#include "../test.h"

#define SEQUENCER_EVENTS 156
uint32_t cm_bitmap[CM_BMPSIZE(SEQUENCER_EVENTS)];
unsigned cm_prev_pos;

#if 0
#define CMVIEW(pos)                                                      \
    printf("%5d: ", pos);                                                \
    for (int cmpos = 0; cmpos < CM_BMPSIZE(SEQUENCER_EVENTS); cmpos++) { \
        printf("%08X", cm_bitmap[cmpos]);                                \
    }                                                                    \
    printf("\n");
#else
#define CMVIEW(...)
#endif

#define TEST_SIZE 200
unsigned positions[TEST_SIZE];

static void cemtests()
{
    TEST_NEW_BLOCK("cellmem");
    printf("events: %d, bmpsize: %d\n", SEQUENCER_EVENTS, (int)sizeof(cm_bitmap));
    CMVIEW(0);
    cmInit(cm_bitmap, &cm_prev_pos, CM_BMPSIZE(SEQUENCER_EVENTS));
    // printf("%5d\n", cmCountFreeCells(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    TEST_ASSERT(cmCountFreeCells(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)) == (CM_BMPSIZE(SEQUENCER_EVENTS) * 32 - 1));
    for (int i = 0; i < TEST_SIZE; i++) {
        positions[i] = cmAlloc(cm_bitmap, &cm_prev_pos, CM_BMPSIZE(SEQUENCER_EVENTS));
        CMVIEW(positions[i]);
        TEST_ASSERT_int(i < (CM_BMPSIZE(SEQUENCER_EVENTS) * 32 - 1) ? positions[i] : !positions[i], i);
    }
    // free
    // printf("%5d\n", cmCountFreeCells(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    TEST_ASSERT(cmCountFreeCells(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)) == 0);
    for (int i = 0; i < 50; i++) {
        cmFree(cm_bitmap, positions[i]);
        CMVIEW(positions[i]);
    }
    // printf("%5d\n", cmCountFreeCells(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    TEST_ASSERT(cmCountFreeCells(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)) == 50);
    // free
    for (int i = 0; i < 50; i++) {
        positions[i] = cmAlloc(cm_bitmap, &cm_prev_pos, CM_BMPSIZE(SEQUENCER_EVENTS));
        CMVIEW(positions[i]);
        TEST_ASSERT_int(positions[i], i);
    }

    // TEST_ASSERT()
    TEST_TODO("test is incomplete without CMVIEW and manual analysis");
}

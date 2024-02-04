#include "../../tools/cellmem.h"
#include "../test.h"

#define SEQUENCER_EVENTS 156
uint32_t cm_bitmap[CM_BMPSIZE(SEQUENCER_EVENTS)];
unsigned cm_prev_pos;

#define CMVIEW                                                           \
    for (int cmpos = 0; cmpos < CM_BMPSIZE(SEQUENCER_EVENTS); cmpos++) { \
        printf("%08X", cm_bitmap[cmpos]);                                \
    }                                                                    \
    printf("\n");

#define TEST_SIZE 200
unsigned positions[TEST_SIZE];

static void cemtests()
{
    TEST_NEW_BLOCK("cellmem");
    printf("events: %d, bmpsize: %d\n", SEQUENCER_EVENTS, (int)sizeof(cm_bitmap));
    CMVIEW
    cmInit(cm_bitmap, &cm_prev_pos, CM_BMPSIZE(SEQUENCER_EVENTS));
    // printf("%5d\n", cmCountFreeCells(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    TEST_ASSERT(cmCountFreeCells(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)) == (CM_BMPSIZE(SEQUENCER_EVENTS) * 32 - 1));
    for (int i = 0; i < TEST_SIZE; i++) {
        positions[i] = cmAlloc(cm_bitmap, &cm_prev_pos, CM_BMPSIZE(SEQUENCER_EVENTS));
        printf("%5d: ", positions[i]);
        CMVIEW
        TEST_ASSERT_int(i < (CM_BMPSIZE(SEQUENCER_EVENTS) * 32 - 1) ? positions[i] : !positions[i], i);
    }
    // free
    // printf("%5d\n", cmCountFreeCells(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    TEST_ASSERT(cmCountFreeCells(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)) == 0);
    for (int i = 0; i < 50; i++) {
        cmFree(cm_bitmap, positions[i]);
        printf("%5d: ", positions[i]);
        CMVIEW
    }
    // printf("%5d\n", cmCountFreeCells(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    TEST_ASSERT(cmCountFreeCells(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)) == 50);
    // free
    for (int i = 0; i < 50; i++) {
        positions[i] = cmAlloc(cm_bitmap, &cm_prev_pos, CM_BMPSIZE(SEQUENCER_EVENTS));
        printf("%5d: ", positions[i]);
        CMVIEW
        TEST_ASSERT_int(positions[i], i);
    }

    // TEST_ASSERT()
    TEST_TODO("improve this test");
}

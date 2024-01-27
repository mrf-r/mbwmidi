#include "../../tools/cellmem.h"
#include "../test.h"

#define SEQUENCER_EVENTS 156
uint32_t cm_bitmap[CM_BMPSIZE(SEQUENCER_EVENTS)];

#define CMVIEW                                                           \
    for (int cmpos = 0; cmpos < CM_BMPSIZE(SEQUENCER_EVENTS); cmpos++) { \
        printf("%08X ", cm_bitmap[cmpos]);                               \
    }                                                                    \
    printf("\n");

static void cemtests()
{
    TEST_NEW_BLOCK("cellmem");
    printf("events: %d, bmpsize: %d\n", SEQUENCER_EVENTS, (int)sizeof(cm_bitmap));
    CMVIEW
    cmInit(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS));
    CMVIEW
    printf("cmAlloc: %d\n", cmAlloc(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    CMVIEW
    printf("cmAlloc: %d\n", cmAlloc(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    CMVIEW
    printf("cmAlloc: %d\n", cmAlloc(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    CMVIEW
    printf("cmAlloc: %d\n", cmAlloc(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    CMVIEW
    printf("cmAlloc: %d\n", cmAlloc(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    CMVIEW
    printf("cmFree 1\n");
    cmFree(cm_bitmap, 1);
    CMVIEW
    printf("cmAlloc: %d\n", cmAlloc(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    CMVIEW
    printf("cmFree 2\n");
    cmFree(cm_bitmap, 2);
    CMVIEW
    printf("cmFree 4\n");
    cmFree(cm_bitmap, 4);
    CMVIEW
    printf("cmAlloc: %d\n", cmAlloc(cm_bitmap, CM_BMPSIZE(SEQUENCER_EVENTS)));
    CMVIEW
    TEST_TODO("so far visual control only))");

    // TEST_ASSERT()
}

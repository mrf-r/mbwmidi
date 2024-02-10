#include "../midi_uart.h"
#include "test.h"

static void outPortTests_optim()
{
    TEST_NEW_BLOCK("uart out");

    MidiOutPortContextT test_port_context;

    midiPortInit(&test_port_context);


    TEST_TODO("as timer");
    TEST_TODO("rs timer");
    TEST_TODO("msg types length");
    TEST_TODO("syx?");

}

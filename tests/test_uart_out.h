#include "../midi_uart.h"
#include "test.h"

static void outPortTests_optim()
{
    TEST_NEW_BLOCK("uart out");

    MidiOutPortContextT test_port_context;

    MidiOutPortT test_port = {
        .context = &test_port_context,
        .api = 0,
        .name = "TEST",
        .type = MIDI_TYPE_USB,
        .cn = 0x9
    };
    midiPortInit(&test_port);


    TEST_TODO("as timer");
    TEST_TODO("rs timer");
    TEST_TODO("msg types length");
    TEST_TODO("syx?");

}

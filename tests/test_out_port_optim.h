#include "../midi.h"
#include "../midi_output.h"
#include "test.h"

static void outPortTests_optim()
{
    MidiOutPortContextT test_port_context;

    MidiOutPortT test_port = {
        .context = &test_port_context,
        .api = 0,
        .name = "TEST",
        .type = MIDI_TYPE_USB,
        .cn = 0x9
    };
    TEST_NEW_BLOCK("out port optim");

    TEST_ASSERT(MIDI_TX_BUFFER_SIZE == 32);
    TEST_ASSERT(MIDI_TX_SYSEX_BUFFER_SIZE == 16);
    MidiMessageT mm = {
        .cn = MIDI_CN_TEST0,
        .cin = MIDI_CIN_PITCHBEND,
        .byte1 = MIDI_CIN_PITCHBEND << 4,
        .byte2 = 0x33,
        .byte3 = 0x44
    };
    MidiMessageT mm2 = {
        .cn = MIDI_CN_TEST0,
        .cin = MIDI_CIN_POLYKEYPRESS,
        .byte1 = MIDI_CIN_POLYKEYPRESS << 4,
        .byte2 = 0x33,
        .byte3 = 0x44
    };
    MidiMessageT ms = {
        .cn = MIDI_CN_TEST0,
        .cin = MIDI_CIN_SYSEXEND2,
        .byte1 = 0x77, // anyway it does not check that
        .byte2 = 0x88, // only uart/com will notice wrong bytes
        .byte3 = 0x99
    };
    MidiMessageT ms2 = {
        .cn = MIDI_CN_TEST2,
        .cin = MIDI_CIN_SYSEX3BYTES,
        .byte1 = 0x77, // anyway it does not check that
        .byte2 = 0x88, // only uart/com will notice wrong bytes
        .byte3 = 0x99
    };
    MidiMessageT mr;

    midiPortInit(&test_port);
}

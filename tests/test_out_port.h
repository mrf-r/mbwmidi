#include "../midi.h"
#include "test.h"

extern volatile uint32_t counter_sr;

static void outPortTests()
{
    TEST_NEW_BLOCK("out port");

    // - thread safety, atomic
    // - raw write
    // - optimized write
    //     - sysex start end
    //         - from multiple sysex sources
    //         - simultaneous with main port
    //     - regular mesages (non-CC)
    //         - flushing is working properly
    //             - same message is replaced
    //             - lowest optimized on correct levels
    //             - oldest one is optimized
    //             - [circular write more than read and check flushing]
    //             - for every kind of messages
    //     - CC
    //         - same as regular for non nrpn, channel
    //         - priority of channel is higher
    //         - nrpn keeps the sense under extreme cases
    //             - ??
    TEST_TODO("");
    TEST_TODO("atomic?");

    // TODO!!!
}


#include "../midi.h"
#include "test.h"

TEST_GLOBAL_VARIABLES

volatile uint32_t test_clock;

#include "test_main_buffer.h"
#include "test_main_sysex.h"
#include "test_out_port.h"

int main()
{
    TEST_ASSERT_str(1, "test OK");
    TEST_ASSERT_str(0, "test that failures are visible");
    tests_result_not_passed = 0; // reset, because previous failure was intentional
    mainBufferTests();
    mainSysexTests();
    outPortTests();
    TEST_RESULTS();
    return tests_result_not_passed;
}
//
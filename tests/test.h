#ifndef _TEST_H
#define _TEST_H

// kiss test :-*

#ifndef TESTS_SHOW_PASSED
#define TESTS_SHOW_PASSED 0
#endif

#include <stdio.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define ENDLINE ANSI_COLOR_RESET "\r\n"

#define STRINGIFY0(v) #v
#define STRINGIFY(v) STRINGIFY0(v)

extern int tests_total;
extern int tests_passed;
extern int tests_failed;
extern int tests_result_not_passed;

#define TEST_GLOBAL_VARIABLES \
    int tests_total = 0;                 \
    int tests_passed = 0;                \
    int tests_failed = 0;                \
    int tests_result_not_passed = 0;

#define TEST_ASSERT(value)                                                           \
    do {                                                                             \
        tests_total++;                                                               \
        if (value) {                                                                 \
            tests_passed++;                                                          \
            if (TESTS_SHOW_PASSED)                                                   \
                printf(ANSI_COLOR_GREEN "PASSED: " STRINGIFY0(value) " - " ENDLINE); \
        } else {                                                                     \
            tests_failed++;                                                          \
            tests_result_not_passed |= 1;                                            \
            printf(ANSI_COLOR_RED "FAILED: " STRINGIFY0(value) " - " /*ENDLINE*/);   \
            printf(" - %s:%d" ENDLINE, __FILE__, __LINE__);                          \
        }                                                                            \
    } while (0)

#define TEST_ASSERT_str(value, str)                                                      \
    do {                                                                                 \
        tests_total++;                                                                   \
        if (value) {                                                                     \
            tests_passed++;                                                              \
            if (TESTS_SHOW_PASSED)                                                       \
                printf(ANSI_COLOR_GREEN "PASSED: " STRINGIFY0(value) " - " str ENDLINE); \
        } else {                                                                         \
            tests_failed++;                                                              \
            tests_result_not_passed |= 1;                                                \
            printf(ANSI_COLOR_RED "FAILED: " STRINGIFY0(value) " - " str /*ENDLINE*/);   \
            printf(" - %s:%d" ENDLINE, __FILE__, __LINE__);                              \
        }                                                                                \
    } while (0)

#define TEST_ASSERT_int(value, i)                                                         \
    do {                                                                                  \
        tests_total++;                                                                    \
        if (value) {                                                                      \
            tests_passed++;                                                               \
            if (TESTS_SHOW_PASSED)                                                        \
                printf(ANSI_COLOR_GREEN "PASSED: " STRINGIFY0(value) " - %d" ENDLINE, i); \
        } else {                                                                          \
            tests_failed++;                                                               \
            tests_result_not_passed |= 1;                                                 \
            printf(ANSI_COLOR_RED "FAILED: " STRINGIFY0(value) " - %d" /*ENDLINE*/, i);   \
            printf(" - %s:%d" ENDLINE, __FILE__, __LINE__);                               \
        }                                                                                 \
    } while (0)

#define TEST_TODO(str)                                  \
    do {                                                \
        printf(ANSI_COLOR_YELLOW "TODO: " str ENDLINE); \
    } while (0)

#define TEST_RESULTS()                                                                                \
    do {                                                                                              \
        printf("total: %d, passed: %d, failed: %d" ENDLINE, tests_total, tests_passed, tests_failed); \
        tests_total = tests_failed = tests_passed = 0;                                                \
    } while (0)

#define TEST_NEW_BLOCK(str)                                                                           \
    do {                                                                                              \
        printf("total: %d, passed: %d, failed: %d" ENDLINE, tests_total, tests_passed, tests_failed); \
        tests_total = tests_failed = tests_passed = 0;                                                \
        printf(ANSI_COLOR_CYAN " - " str " - " ENDLINE);                                              \
    } while (0)

#endif // _TEST_H
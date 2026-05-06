#ifndef DUMBPAD_TEST_H
#define DUMBPAD_TEST_H

#include <stdio.h>

#define TEST_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "Assertion failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
            return 1; \
        } \
    } while (0)

#endif

#include "test.h"

#include "../src/line_endings.h"

#include <stdlib.h>
#include <string.h>

int
test_line_endings(void)
{
    {
        DumbpadLineEndingInfo info = dumbpad_detect_line_endings(L"one\r\ntwo\r\nthree");

        TEST_ASSERT(info.detected_mode == DUMBPAD_LINE_ENDINGS_CRLF);
        TEST_ASSERT(info.preferred_mode == DUMBPAD_LINE_ENDINGS_CRLF);
        TEST_ASSERT(info.crlf_count == 2);
    }

    {
        DumbpadLineEndingInfo info = dumbpad_detect_line_endings(L"one\ntwo\nthree");

        TEST_ASSERT(info.detected_mode == DUMBPAD_LINE_ENDINGS_LF);
        TEST_ASSERT(info.preferred_mode == DUMBPAD_LINE_ENDINGS_LF);
        TEST_ASSERT(info.lf_count == 2);
    }

    {
        DumbpadLineEndingInfo info = dumbpad_detect_line_endings(L"one\rtwo\rthree");

        TEST_ASSERT(info.detected_mode == DUMBPAD_LINE_ENDINGS_CR);
        TEST_ASSERT(info.preferred_mode == DUMBPAD_LINE_ENDINGS_CR);
        TEST_ASSERT(info.cr_count == 2);
    }

    {
        DumbpadLineEndingInfo info = dumbpad_detect_line_endings(L"one\r\ntwo\nthree\rfour");

        TEST_ASSERT(info.detected_mode == DUMBPAD_LINE_ENDINGS_MIXED);
        TEST_ASSERT(info.preferred_mode == DUMBPAD_LINE_ENDINGS_CRLF);
        TEST_ASSERT(info.crlf_count == 1);
        TEST_ASSERT(info.lf_count == 1);
        TEST_ASSERT(info.cr_count == 1);
    }

    {
        wchar_t *normalized = dumbpad_normalize_line_endings(L"one\r\ntwo\nthree\rfour", DUMBPAD_LINE_ENDINGS_LF);

        TEST_ASSERT(normalized != NULL);
        TEST_ASSERT(wcscmp(normalized, L"one\ntwo\nthree\nfour") == 0);
        free(normalized);
    }

    {
        wchar_t *normalized = dumbpad_normalize_line_endings(L"one\ntwo\rthree", DUMBPAD_LINE_ENDINGS_CRLF);

        TEST_ASSERT(normalized != NULL);
        TEST_ASSERT(wcscmp(normalized, L"one\r\ntwo\r\nthree") == 0);
        free(normalized);
    }

    return 0;
}

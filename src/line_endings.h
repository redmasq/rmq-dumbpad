#ifndef DUMBPAD_LINE_ENDINGS_H
#define DUMBPAD_LINE_ENDINGS_H

#include <stddef.h>
#include <wchar.h>
#include <wchar.h>

typedef enum DumbpadLineEndingMode {
    DUMBPAD_LINE_ENDINGS_NONE = 0,
    DUMBPAD_LINE_ENDINGS_CRLF = 1,
    DUMBPAD_LINE_ENDINGS_LF = 2,
    DUMBPAD_LINE_ENDINGS_CR = 3,
    DUMBPAD_LINE_ENDINGS_MIXED = 4
} DumbpadLineEndingMode;

typedef struct DumbpadLineEndingInfo {
    DumbpadLineEndingMode detected_mode;
    DumbpadLineEndingMode preferred_mode;
    size_t crlf_count;
    size_t lf_count;
    size_t cr_count;
} DumbpadLineEndingInfo;

DumbpadLineEndingInfo dumbpad_detect_line_endings(const wchar_t *text);
wchar_t *dumbpad_normalize_line_endings(const wchar_t *text, DumbpadLineEndingMode target_mode);
const char *dumbpad_line_ending_name(DumbpadLineEndingMode mode);

#endif

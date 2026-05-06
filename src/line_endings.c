#include "line_endings.h"

#include <stdlib.h>
#include <string.h>

static DumbpadLineEndingMode
dominant_mode(size_t crlf_count, size_t lf_count, size_t cr_count)
{
    if (crlf_count >= lf_count && crlf_count >= cr_count && crlf_count > 0) {
        return DUMBPAD_LINE_ENDINGS_CRLF;
    }
    if (lf_count >= cr_count && lf_count > 0) {
        return DUMBPAD_LINE_ENDINGS_LF;
    }
    if (cr_count > 0) {
        return DUMBPAD_LINE_ENDINGS_CR;
    }
    return DUMBPAD_LINE_ENDINGS_CRLF;
}

DumbpadLineEndingInfo
dumbpad_detect_line_endings(const wchar_t *text)
{
    DumbpadLineEndingInfo info;
    size_t i;
    int kinds = 0;

    memset(&info, 0, sizeof(info));

    for (i = 0; text[i] != L'\0'; ++i) {
        if (text[i] == L'\r') {
            if (text[i + 1] == L'\n') {
                info.crlf_count++;
                i++;
            } else {
                info.cr_count++;
            }
        } else if (text[i] == L'\n') {
            info.lf_count++;
        }
    }

    if (info.crlf_count > 0) {
        kinds++;
    }
    if (info.lf_count > 0) {
        kinds++;
    }
    if (info.cr_count > 0) {
        kinds++;
    }

    if (kinds == 0) {
        info.detected_mode = DUMBPAD_LINE_ENDINGS_NONE;
        info.preferred_mode = DUMBPAD_LINE_ENDINGS_CRLF;
    } else if (kinds == 1) {
        info.detected_mode = dominant_mode(info.crlf_count, info.lf_count, info.cr_count);
        info.preferred_mode = info.detected_mode;
    } else {
        info.detected_mode = DUMBPAD_LINE_ENDINGS_MIXED;
        info.preferred_mode = dominant_mode(info.crlf_count, info.lf_count, info.cr_count);
    }

    return info;
}

wchar_t *
dumbpad_normalize_line_endings(const wchar_t *text, DumbpadLineEndingMode target_mode)
{
    size_t src_len = wcslen(text);
    size_t extra = 0;
    size_t i;
    wchar_t *out;
    size_t j = 0;

    if (target_mode != DUMBPAD_LINE_ENDINGS_CRLF &&
        target_mode != DUMBPAD_LINE_ENDINGS_LF &&
        target_mode != DUMBPAD_LINE_ENDINGS_CR) {
        return NULL;
    }

    for (i = 0; i < src_len; ++i) {
        if (text[i] == L'\r') {
            if (text[i + 1] == L'\n') {
                if (target_mode == DUMBPAD_LINE_ENDINGS_CRLF) {
                    extra += 1;
                }
                i++;
            } else if (target_mode == DUMBPAD_LINE_ENDINGS_CRLF) {
                extra += 1;
            }
        } else if (text[i] == L'\n' && target_mode == DUMBPAD_LINE_ENDINGS_CRLF) {
            extra += 1;
        }
    }

    out = (wchar_t *)malloc((src_len + extra + 1) * sizeof(wchar_t));
    if (!out) {
        return NULL;
    }

    for (i = 0; i < src_len; ++i) {
        if (text[i] == L'\r') {
            if (text[i + 1] == L'\n') {
                if (target_mode == DUMBPAD_LINE_ENDINGS_CRLF) {
                    out[j++] = L'\r';
                    out[j++] = L'\n';
                } else if (target_mode == DUMBPAD_LINE_ENDINGS_LF) {
                    out[j++] = L'\n';
                } else {
                    out[j++] = L'\r';
                }
                i++;
                continue;
            }

            if (target_mode == DUMBPAD_LINE_ENDINGS_CRLF) {
                out[j++] = L'\r';
                out[j++] = L'\n';
            } else if (target_mode == DUMBPAD_LINE_ENDINGS_LF) {
                out[j++] = L'\n';
            } else {
                out[j++] = L'\r';
            }
            continue;
        }

        if (text[i] == L'\n') {
            if (target_mode == DUMBPAD_LINE_ENDINGS_CRLF) {
                out[j++] = L'\r';
                out[j++] = L'\n';
            } else if (target_mode == DUMBPAD_LINE_ENDINGS_LF) {
                out[j++] = L'\n';
            } else {
                out[j++] = L'\r';
            }
            continue;
        }

        out[j++] = text[i];
    }

    out[j] = L'\0';
    return out;
}

const char *
dumbpad_line_ending_name(DumbpadLineEndingMode mode)
{
    switch (mode) {
    case DUMBPAD_LINE_ENDINGS_NONE:
        return "none";
    case DUMBPAD_LINE_ENDINGS_CRLF:
        return "CRLF";
    case DUMBPAD_LINE_ENDINGS_LF:
        return "LF";
    case DUMBPAD_LINE_ENDINGS_CR:
        return "CR";
    case DUMBPAD_LINE_ENDINGS_MIXED:
        return "mixed";
    }
    return "unknown";
}

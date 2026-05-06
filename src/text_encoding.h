#ifndef DUMBPAD_TEXT_ENCODING_H
#define DUMBPAD_TEXT_ENCODING_H

#include <stddef.h>

typedef enum DumbpadTextEncoding {
    DUMBPAD_TEXT_ENCODING_ASCII,
    DUMBPAD_TEXT_ENCODING_UTF8,
    DUMBPAD_TEXT_ENCODING_UTF16_LE,
    DUMBPAD_TEXT_ENCODING_UTF16_BE,
    DUMBPAD_TEXT_ENCODING_ANSI
} DumbpadTextEncoding;

typedef struct DumbpadTextDetectResult {
    DumbpadTextEncoding encoding;
    int has_bom;
    int has_invalid_sequences;
} DumbpadTextDetectResult;

DumbpadTextDetectResult dumbpad_detect_text_encoding(const unsigned char *data, size_t size);
const char *dumbpad_text_encoding_name(DumbpadTextEncoding encoding);

#endif

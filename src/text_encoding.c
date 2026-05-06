#include "text_encoding.h"

static int
is_ascii(const unsigned char *data, size_t size)
{
    size_t i;

    for (i = 0; i < size; ++i) {
        if (data[i] == 0 || data[i] > 0x7F) {
            return 0;
        }
    }
    return 1;
}

static int
is_valid_utf8(const unsigned char *data, size_t size)
{
    size_t i = 0;

    while (i < size) {
        unsigned char c = data[i];
        size_t need = 0;
        unsigned char min_second = 0x80;
        unsigned char max_second = 0xBF;

        if (c <= 0x7F) {
            i++;
            continue;
        }
        if (c >= 0xC2 && c <= 0xDF) {
            need = 1;
        } else if (c == 0xE0) {
            need = 2;
            min_second = 0xA0;
        } else if (c >= 0xE1 && c <= 0xEC) {
            need = 2;
        } else if (c == 0xED) {
            need = 2;
            max_second = 0x9F;
        } else if (c >= 0xEE && c <= 0xEF) {
            need = 2;
        } else if (c == 0xF0) {
            need = 3;
            min_second = 0x90;
        } else if (c >= 0xF1 && c <= 0xF3) {
            need = 3;
        } else if (c == 0xF4) {
            need = 3;
            max_second = 0x8F;
        } else {
            return 0;
        }

        if (i + need >= size) {
            return 0;
        }
        if (data[i + 1] < min_second || data[i + 1] > max_second) {
            return 0;
        }

        for (size_t j = 2; j <= need; ++j) {
            if (data[i + j] < 0x80 || data[i + j] > 0xBF) {
                return 0;
            }
        }

        i += need + 1;
    }

    return 1;
}

static unsigned short
read_u16(const unsigned char *data, size_t offset, int little_endian)
{
    if (little_endian) {
        return (unsigned short)(data[offset] | ((unsigned short)data[offset + 1] << 8));
    }
    return (unsigned short)(((unsigned short)data[offset] << 8) | data[offset + 1]);
}

static int
has_invalid_utf16_sequences(const unsigned char *data, size_t size, int little_endian)
{
    size_t i;

    if ((size % 2) != 0) {
        return 1;
    }

    for (i = 0; i + 1 < size; i += 2) {
        unsigned short unit = read_u16(data, i, little_endian);

        if (unit >= 0xD800 && unit <= 0xDBFF) {
            if (i + 3 >= size) {
                return 1;
            }
            i += 2;
            unit = read_u16(data, i, little_endian);
            if (unit < 0xDC00 || unit > 0xDFFF) {
                return 1;
            }
        } else if (unit >= 0xDC00 && unit <= 0xDFFF) {
            return 1;
        }
    }

    return 0;
}

static int
looks_like_utf16(const unsigned char *data, size_t size, int little_endian)
{
    size_t i;
    size_t pairs = 0;
    size_t printable = 0;
    size_t zeros_in_expected_lane = 0;
    size_t zeros_in_unexpected_lane = 0;

    if (size < 4 || (size % 2) != 0) {
        return 0;
    }

    for (i = 0; i + 1 < size; i += 2) {
        unsigned char first = data[i];
        unsigned char second = data[i + 1];
        unsigned short unit = read_u16(data, i, little_endian);

        pairs++;
        if (little_endian) {
            if (second == 0) {
                zeros_in_expected_lane++;
            }
            if (first == 0) {
                zeros_in_unexpected_lane++;
            }
        } else {
            if (first == 0) {
                zeros_in_expected_lane++;
            }
            if (second == 0) {
                zeros_in_unexpected_lane++;
            }
        }

        if (unit == 0x0009 || unit == 0x000A || unit == 0x000D ||
            (unit >= 0x0020 && unit <= 0x007E)) {
            printable++;
        }
    }

    return pairs >= 2 &&
        zeros_in_expected_lane * 2 >= pairs &&
        zeros_in_unexpected_lane * 4 <= pairs &&
        printable * 2 >= pairs &&
        !has_invalid_utf16_sequences(data, size, little_endian);
}

DumbpadTextDetectResult
dumbpad_detect_text_encoding(const unsigned char *data, size_t size)
{
    DumbpadTextDetectResult result;

    result.encoding = DUMBPAD_TEXT_ENCODING_ANSI;
    result.has_bom = 0;
    result.has_invalid_sequences = 0;

    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        result.encoding = DUMBPAD_TEXT_ENCODING_UTF8;
        result.has_bom = 1;
        result.has_invalid_sequences = !is_valid_utf8(data + 3, size - 3);
        return result;
    }

    if (size >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
        result.encoding = DUMBPAD_TEXT_ENCODING_UTF16_LE;
        result.has_bom = 1;
        result.has_invalid_sequences = has_invalid_utf16_sequences(data + 2, size - 2, 1);
        return result;
    }

    if (size >= 2 && data[0] == 0xFE && data[1] == 0xFF) {
        result.encoding = DUMBPAD_TEXT_ENCODING_UTF16_BE;
        result.has_bom = 1;
        result.has_invalid_sequences = has_invalid_utf16_sequences(data + 2, size - 2, 0);
        return result;
    }

    if (is_ascii(data, size)) {
        result.encoding = DUMBPAD_TEXT_ENCODING_ASCII;
        return result;
    }

    if (looks_like_utf16(data, size, 1)) {
        result.encoding = DUMBPAD_TEXT_ENCODING_UTF16_LE;
        return result;
    }

    if (looks_like_utf16(data, size, 0)) {
        result.encoding = DUMBPAD_TEXT_ENCODING_UTF16_BE;
        return result;
    }

    if (is_valid_utf8(data, size)) {
        result.encoding = DUMBPAD_TEXT_ENCODING_UTF8;
    }

    return result;
}

const char *
dumbpad_text_encoding_name(DumbpadTextEncoding encoding)
{
    switch (encoding) {
    case DUMBPAD_TEXT_ENCODING_ASCII:
        return "ASCII";
    case DUMBPAD_TEXT_ENCODING_UTF8:
        return "UTF-8";
    case DUMBPAD_TEXT_ENCODING_UTF16_LE:
        return "UTF-16 LE";
    case DUMBPAD_TEXT_ENCODING_UTF16_BE:
        return "UTF-16 BE";
    case DUMBPAD_TEXT_ENCODING_ANSI:
        return "ANSI";
    }
    return "Unknown";
}

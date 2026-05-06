#include "test.h"

#include "../src/text_encoding.h"

int
test_text_encoding(void)
{
    {
        const unsigned char ascii_text[] = {'h', 'e', 'l', 'l', 'o'};
        DumbpadTextDetectResult result = dumbpad_detect_text_encoding(ascii_text, sizeof(ascii_text));

        TEST_ASSERT(result.encoding == DUMBPAD_TEXT_ENCODING_ASCII);
        TEST_ASSERT(result.has_bom == 0);
        TEST_ASSERT(result.has_invalid_sequences == 0);
    }

    {
        const unsigned char bom_utf8[] = {0xEF, 0xBB, 0xBF, 'h', 'i'};
        DumbpadTextDetectResult result = dumbpad_detect_text_encoding(bom_utf8, sizeof(bom_utf8));

        TEST_ASSERT(result.encoding == DUMBPAD_TEXT_ENCODING_UTF8);
        TEST_ASSERT(result.has_bom == 1);
        TEST_ASSERT(result.has_invalid_sequences == 0);
    }

    {
        const unsigned char plain_utf8[] = {'c', 'a', 'f', 0xC3, 0xA9};
        DumbpadTextDetectResult result = dumbpad_detect_text_encoding(plain_utf8, sizeof(plain_utf8));

        TEST_ASSERT(result.encoding == DUMBPAD_TEXT_ENCODING_UTF8);
        TEST_ASSERT(result.has_bom == 0);
        TEST_ASSERT(result.has_invalid_sequences == 0);
    }

    {
        const unsigned char utf16le_bom[] = {0xFF, 0xFE, 'A', 0x00, 'B', 0x00};
        DumbpadTextDetectResult result = dumbpad_detect_text_encoding(utf16le_bom, sizeof(utf16le_bom));

        TEST_ASSERT(result.encoding == DUMBPAD_TEXT_ENCODING_UTF16_LE);
        TEST_ASSERT(result.has_bom == 1);
        TEST_ASSERT(result.has_invalid_sequences == 0);
    }

    {
        const unsigned char utf16be_bom[] = {0xFE, 0xFF, 0x00, 'A', 0x00, 'B'};
        DumbpadTextDetectResult result = dumbpad_detect_text_encoding(utf16be_bom, sizeof(utf16be_bom));

        TEST_ASSERT(result.encoding == DUMBPAD_TEXT_ENCODING_UTF16_BE);
        TEST_ASSERT(result.has_bom == 1);
        TEST_ASSERT(result.has_invalid_sequences == 0);
    }

    {
        const unsigned char utf16le_no_bom[] = {'A', 0x00, 'B', 0x00, 'C', 0x00};
        DumbpadTextDetectResult result = dumbpad_detect_text_encoding(utf16le_no_bom, sizeof(utf16le_no_bom));

        TEST_ASSERT(result.encoding == DUMBPAD_TEXT_ENCODING_UTF16_LE);
    }

    {
        const unsigned char invalid_utf8[] = {0xC3, 0x28};
        DumbpadTextDetectResult result = dumbpad_detect_text_encoding(invalid_utf8, sizeof(invalid_utf8));

        TEST_ASSERT(result.encoding == DUMBPAD_TEXT_ENCODING_ANSI);
        TEST_ASSERT(result.has_bom == 0);
    }

    {
        const unsigned char invalid_utf16le_bom[] = {0xFF, 0xFE, 0x00, 0xD8};
        DumbpadTextDetectResult result = dumbpad_detect_text_encoding(invalid_utf16le_bom, sizeof(invalid_utf16le_bom));

        TEST_ASSERT(result.encoding == DUMBPAD_TEXT_ENCODING_UTF16_LE);
        TEST_ASSERT(result.has_invalid_sequences == 1);
    }

    return 0;
}

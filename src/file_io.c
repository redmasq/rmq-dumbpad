#include "file_io.h"

#include <stdlib.h>
#include <string.h>

static BOOL
write_file_bytes(const WCHAR *path, const unsigned char *bytes, size_t size)
{
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written = 0;

    if (file == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    if (size > 0 && !WriteFile(file, bytes, (DWORD)size, &written, NULL)) {
        CloseHandle(file);
        return FALSE;
    }
    CloseHandle(file);
    return TRUE;
}

static BOOL
append_wchar(WCHAR **buffer, size_t *length, size_t *capacity, WCHAR value)
{
    WCHAR *grown;

    if (*length + 1 >= *capacity) {
        size_t new_capacity = *capacity ? (*capacity * 2) : 256;
        if (new_capacity <= *length + 1) {
            new_capacity = *length + 2;
        }
        grown = (WCHAR *)realloc(*buffer, new_capacity * sizeof(WCHAR));
        if (!grown) {
            return FALSE;
        }
        *buffer = grown;
        *capacity = new_capacity;
    }
    (*buffer)[*length] = value;
    (*length)++;
    return TRUE;
}

static BOOL
append_codepoint_utf16(WCHAR **buffer, size_t *length, size_t *capacity, unsigned int codepoint)
{
    if (codepoint <= 0xFFFF) {
        return append_wchar(buffer, length, capacity, (WCHAR)codepoint);
    }

    codepoint -= 0x10000;
    if (!append_wchar(buffer, length, capacity, (WCHAR)(0xD800 + (codepoint >> 10)))) {
        return FALSE;
    }
    return append_wchar(buffer, length, capacity, (WCHAR)(0xDC00 + (codepoint & 0x3FF)));
}

static BOOL
decode_utf8_lossy(const unsigned char *data, size_t size, WCHAR **out_text, BOOL *had_errors)
{
    WCHAR *result = NULL;
    size_t length = 0;
    size_t capacity = 0;
    size_t i = 0;

    *had_errors = FALSE;
    while (i < size) {
        unsigned char c = data[i];
        unsigned int codepoint = 0;
        size_t need = 0;
        unsigned char min_second = 0x80;
        unsigned char max_second = 0xBF;
        size_t j;
        BOOL invalid = FALSE;

        if (c <= 0x7F) {
            codepoint = c;
        } else if (c >= 0xC2 && c <= 0xDF) {
            need = 1;
            codepoint = (unsigned int)(c & 0x1F);
        } else if (c == 0xE0) {
            need = 2;
            min_second = 0xA0;
            codepoint = (unsigned int)(c & 0x0F);
        } else if (c >= 0xE1 && c <= 0xEC) {
            need = 2;
            codepoint = (unsigned int)(c & 0x0F);
        } else if (c == 0xED) {
            need = 2;
            max_second = 0x9F;
            codepoint = (unsigned int)(c & 0x0F);
        } else if (c >= 0xEE && c <= 0xEF) {
            need = 2;
            codepoint = (unsigned int)(c & 0x0F);
        } else if (c == 0xF0) {
            need = 3;
            min_second = 0x90;
            codepoint = (unsigned int)(c & 0x07);
        } else if (c >= 0xF1 && c <= 0xF3) {
            need = 3;
            codepoint = (unsigned int)(c & 0x07);
        } else if (c == 0xF4) {
            need = 3;
            max_second = 0x8F;
            codepoint = (unsigned int)(c & 0x07);
        } else {
            invalid = TRUE;
        }

        if (!invalid && need > 0) {
            if (i + need >= size) {
                invalid = TRUE;
            } else if (data[i + 1] < min_second || data[i + 1] > max_second) {
                invalid = TRUE;
            } else {
                for (j = 1; j <= need; ++j) {
                    if (j > 1 && (data[i + j] < 0x80 || data[i + j] > 0xBF)) {
                        invalid = TRUE;
                        break;
                    }
                    codepoint = (codepoint << 6) | (unsigned int)(data[i + j] & 0x3F);
                }
            }
        }

        if (invalid) {
            *had_errors = TRUE;
            if (!append_wchar(&result, &length, &capacity, 0xFFFD)) {
                free(result);
                return FALSE;
            }
            i++;
            continue;
        }

        if (!append_codepoint_utf16(&result, &length, &capacity, codepoint)) {
            free(result);
            return FALSE;
        }
        i += need + 1;
    }

    if (!append_wchar(&result, &length, &capacity, L'\0')) {
        free(result);
        return FALSE;
    }

    *out_text = result;
    return TRUE;
}

static BOOL
decode_utf16_lossy(const unsigned char *data, size_t size, int little_endian, WCHAR **out_text, BOOL *had_errors)
{
    WCHAR *result = NULL;
    size_t length = 0;
    size_t capacity = 0;
    size_t i = 0;

    *had_errors = FALSE;
    if ((size % 2) != 0) {
        *had_errors = TRUE;
    }

    while (i + 1 < size) {
        unsigned short unit;

        if (little_endian) {
            unit = (unsigned short)(data[i] | ((unsigned short)data[i + 1] << 8));
        } else {
            unit = (unsigned short)(((unsigned short)data[i] << 8) | data[i + 1]);
        }
        i += 2;

        if (unit >= 0xD800 && unit <= 0xDBFF) {
            unsigned short next_unit;

            if (i + 1 >= size) {
                *had_errors = TRUE;
                if (!append_wchar(&result, &length, &capacity, 0xFFFD)) {
                    free(result);
                    return FALSE;
                }
                break;
            }
            if (little_endian) {
                next_unit = (unsigned short)(data[i] | ((unsigned short)data[i + 1] << 8));
            } else {
                next_unit = (unsigned short)(((unsigned short)data[i] << 8) | data[i + 1]);
            }
            if (next_unit < 0xDC00 || next_unit > 0xDFFF) {
                *had_errors = TRUE;
                if (!append_wchar(&result, &length, &capacity, 0xFFFD)) {
                    free(result);
                    return FALSE;
                }
                continue;
            }
            if (!append_wchar(&result, &length, &capacity, (WCHAR)unit) ||
                !append_wchar(&result, &length, &capacity, (WCHAR)next_unit)) {
                free(result);
                return FALSE;
            }
            i += 2;
            continue;
        }

        if (unit >= 0xDC00 && unit <= 0xDFFF) {
            *had_errors = TRUE;
            if (!append_wchar(&result, &length, &capacity, 0xFFFD)) {
                free(result);
                return FALSE;
            }
            continue;
        }

        if (!append_wchar(&result, &length, &capacity, (WCHAR)unit)) {
            free(result);
            return FALSE;
        }
    }

    if (!append_wchar(&result, &length, &capacity, L'\0')) {
        free(result);
        return FALSE;
    }

    *out_text = result;
    return TRUE;
}

BOOL
dumbpad_save_text_file(
    const WCHAR *path,
    const WCHAR *text,
    DumbpadTextEncoding encoding,
    DumbpadLineEndingMode line_endings)
{
    WCHAR *normalized = dumbpad_normalize_line_endings(text, line_endings);
    const WCHAR *source;
    size_t length;

    if (!normalized) {
        return FALSE;
    }
    source = normalized;
    length = wcslen(source);

    switch (encoding) {
    case DUMBPAD_TEXT_ENCODING_ASCII:
        {
            unsigned char *bytes = (unsigned char *)malloc(length ? length : 1);
            size_t i;
            BOOL ok;

            if (!bytes) {
                free(normalized);
                return FALSE;
            }
            for (i = 0; i < length; ++i) {
                if (source[i] > 0x7F) {
                    free(bytes);
                    free(normalized);
                    return FALSE;
                }
                bytes[i] = (unsigned char)source[i];
            }
            ok = write_file_bytes(path, bytes, length);
            free(bytes);
            free(normalized);
            return ok;
        }
    case DUMBPAD_TEXT_ENCODING_UTF8:
        {
            int bytes_needed = WideCharToMultiByte(CP_UTF8, 0, source, -1, NULL, 0, NULL, NULL);
            char *utf8;
            unsigned char *bytes;
            BOOL ok;

            if (bytes_needed <= 0) {
                free(normalized);
                return FALSE;
            }
            utf8 = (char *)malloc((size_t)bytes_needed);
            bytes = (unsigned char *)malloc((size_t)bytes_needed + 3);
            if (!utf8 || !bytes) {
                free(utf8);
                free(bytes);
                free(normalized);
                return FALSE;
            }
            if (!WideCharToMultiByte(CP_UTF8, 0, source, -1, utf8, bytes_needed, NULL, NULL)) {
                free(utf8);
                free(bytes);
                free(normalized);
                return FALSE;
            }
            bytes[0] = 0xEF;
            bytes[1] = 0xBB;
            bytes[2] = 0xBF;
            memcpy(bytes + 3, utf8, (size_t)bytes_needed - 1);
            ok = write_file_bytes(path, bytes, (size_t)bytes_needed + 2);
            free(utf8);
            free(bytes);
            free(normalized);
            return ok;
        }
    case DUMBPAD_TEXT_ENCODING_UTF16_LE:
        {
            unsigned char *bytes = (unsigned char *)malloc((length * 2) + 2);
            size_t i;
            BOOL ok;

            if (!bytes) {
                free(normalized);
                return FALSE;
            }
            bytes[0] = 0xFF;
            bytes[1] = 0xFE;
            for (i = 0; i < length; ++i) {
                bytes[2 + (i * 2)] = (unsigned char)(source[i] & 0xFF);
                bytes[3 + (i * 2)] = (unsigned char)((source[i] >> 8) & 0xFF);
            }
            ok = write_file_bytes(path, bytes, (length * 2) + 2);
            free(bytes);
            free(normalized);
            return ok;
        }
    case DUMBPAD_TEXT_ENCODING_UTF16_BE:
        {
            unsigned char *bytes = (unsigned char *)malloc((length * 2) + 2);
            size_t i;
            BOOL ok;

            if (!bytes) {
                free(normalized);
                return FALSE;
            }
            bytes[0] = 0xFE;
            bytes[1] = 0xFF;
            for (i = 0; i < length; ++i) {
                bytes[2 + (i * 2)] = (unsigned char)((source[i] >> 8) & 0xFF);
                bytes[3 + (i * 2)] = (unsigned char)(source[i] & 0xFF);
            }
            ok = write_file_bytes(path, bytes, (length * 2) + 2);
            free(bytes);
            free(normalized);
            return ok;
        }
    case DUMBPAD_TEXT_ENCODING_ANSI:
        {
            BOOL used_default = FALSE;
            int bytes_needed = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, source, -1, NULL, 0, NULL, &used_default);
            char *ansi;
            BOOL ok;

            if (bytes_needed <= 0 || used_default) {
                free(normalized);
                return FALSE;
            }
            ansi = (char *)malloc((size_t)bytes_needed);
            if (!ansi) {
                free(normalized);
                return FALSE;
            }
            if (!WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, source, -1, ansi, bytes_needed, NULL, &used_default) || used_default) {
                free(ansi);
                free(normalized);
                return FALSE;
            }
            ok = write_file_bytes(path, (const unsigned char *)ansi, (size_t)bytes_needed - 1);
            free(ansi);
            free(normalized);
            return ok;
        }
    }

    free(normalized);
    return FALSE;
}

BOOL
dumbpad_load_file_into_edit(HWND edit, const WCHAR *path, DumbpadFileLoadResult *result)
{
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    DumbpadTextDetectResult detected;
    const unsigned char *payload;
    size_t payload_size;
    WCHAR *wide = NULL;
    BOOL had_errors = FALSE;

    if (file == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    {
        LARGE_INTEGER size;
        DWORD byte_count;
        char *bytes;
        DWORD read = 0;

        if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 || size.QuadPart > 64 * 1024 * 1024) {
            CloseHandle(file);
            return FALSE;
        }

        byte_count = (DWORD)size.QuadPart;
        bytes = (char *)malloc((size_t)byte_count + 1);
        if (!bytes) {
            CloseHandle(file);
            return FALSE;
        }

        if (!ReadFile(file, bytes, byte_count, &read, NULL)) {
            free(bytes);
            CloseHandle(file);
            return FALSE;
        }
        bytes[read] = '\0';
        CloseHandle(file);

        detected = dumbpad_detect_text_encoding((const unsigned char *)bytes, (size_t)read);
        payload = (const unsigned char *)bytes;
        payload_size = (size_t)read;

        if (detected.encoding == DUMBPAD_TEXT_ENCODING_UTF8 && detected.has_bom && payload_size >= 3) {
            payload += 3;
            payload_size -= 3;
        } else if ((detected.encoding == DUMBPAD_TEXT_ENCODING_UTF16_LE ||
                    detected.encoding == DUMBPAD_TEXT_ENCODING_UTF16_BE) &&
                   detected.has_bom && payload_size >= 2) {
            payload += 2;
            payload_size -= 2;
        }

        switch (detected.encoding) {
        case DUMBPAD_TEXT_ENCODING_ASCII:
        case DUMBPAD_TEXT_ENCODING_ANSI:
            {
                UINT codepage = detected.encoding == DUMBPAD_TEXT_ENCODING_ASCII ? 20127u : CP_ACP;
                int needed = MultiByteToWideChar(codepage, 0, (const char *)payload, (int)payload_size, NULL, 0);

                if (needed <= 0) {
                    free(bytes);
                    return FALSE;
                }
                wide = (WCHAR *)calloc((size_t)needed + 1, sizeof(WCHAR));
                if (!wide) {
                    free(bytes);
                    return FALSE;
                }
                if (!MultiByteToWideChar(codepage, 0, (const char *)payload, (int)payload_size, wide, needed)) {
                    free(wide);
                    free(bytes);
                    return FALSE;
                }
            }
            break;
        case DUMBPAD_TEXT_ENCODING_UTF8:
            if (!decode_utf8_lossy(payload, payload_size, &wide, &had_errors)) {
                free(bytes);
                return FALSE;
            }
            break;
        case DUMBPAD_TEXT_ENCODING_UTF16_LE:
            if (!decode_utf16_lossy(payload, payload_size, 1, &wide, &had_errors)) {
                free(bytes);
                return FALSE;
            }
            break;
        case DUMBPAD_TEXT_ENCODING_UTF16_BE:
            if (!decode_utf16_lossy(payload, payload_size, 0, &wide, &had_errors)) {
                free(bytes);
                return FALSE;
            }
            break;
        }

        free(bytes);
    }

    SetWindowTextW(edit, wide);
    if (result) {
        result->encoding = detected.encoding;
        result->has_invalid_unicode = detected.has_invalid_sequences || had_errors;
        result->line_endings = dumbpad_detect_line_endings(wide);
    }
    free(wide);
    return TRUE;
}

#ifndef DUMBPAD_FILE_IO_H
#define DUMBPAD_FILE_IO_H

#include <windows.h>

#include "line_endings.h"
#include "text_encoding.h"

typedef struct DumbpadFileLoadResult {
    DumbpadTextEncoding encoding;
    BOOL has_invalid_unicode;
    DumbpadLineEndingInfo line_endings;
} DumbpadFileLoadResult;

BOOL dumbpad_load_file_into_edit(HWND edit, const WCHAR *path, DumbpadFileLoadResult *result);
BOOL dumbpad_save_text_file(
    const WCHAR *path,
    const WCHAR *text,
    DumbpadTextEncoding encoding,
    DumbpadLineEndingMode line_endings);

#endif

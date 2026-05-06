#ifndef DUMBPAD_SETTINGS_H
#define DUMBPAD_SETTINGS_H

#include <windows.h>

#include "app_types.h"

typedef struct DumbpadSettings {
    ThemeMode theme;
    int font_height;
    WCHAR font_face[LF_FACESIZE];
} DumbpadSettings;

BOOL dumbpad_load_settings(DumbpadSettings *settings, BOOL *backend_available);
BOOL dumbpad_save_settings(const DumbpadSettings *settings, BOOL *backend_available);

#endif

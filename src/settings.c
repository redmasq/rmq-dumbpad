#include "settings.h"

#include <string.h>

typedef LSTATUS (WINAPI *RegCreateKeyExWProc)(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, const LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
typedef LSTATUS (WINAPI *RegSetValueExWProc)(HKEY, LPCWSTR, DWORD, DWORD, const BYTE *, DWORD);
typedef LSTATUS (WINAPI *RegQueryValueExWProc)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef LSTATUS (WINAPI *RegCloseKeyProc)(HKEY);

typedef struct DumbpadRegistryApi {
    RegCreateKeyExWProc reg_create_key_ex_w;
    RegSetValueExWProc reg_set_value_ex_w;
    RegQueryValueExWProc reg_query_value_ex_w;
    RegCloseKeyProc reg_close_key;
} DumbpadRegistryApi;

static int
load_registry_api(HMODULE advapi, DumbpadRegistryApi *api)
{
    union {
        FARPROC raw;
        RegCreateKeyExWProc reg_create_key_ex_w;
        RegSetValueExWProc reg_set_value_ex_w;
        RegQueryValueExWProc reg_query_value_ex_w;
        RegCloseKeyProc reg_close_key;
    } conv;

    conv.raw = GetProcAddress(advapi, "RegCreateKeyExW");
    api->reg_create_key_ex_w = conv.reg_create_key_ex_w;
    conv.raw = GetProcAddress(advapi, "RegSetValueExW");
    api->reg_set_value_ex_w = conv.reg_set_value_ex_w;
    conv.raw = GetProcAddress(advapi, "RegQueryValueExW");
    api->reg_query_value_ex_w = conv.reg_query_value_ex_w;
    conv.raw = GetProcAddress(advapi, "RegCloseKey");
    api->reg_close_key = conv.reg_close_key;

    return api->reg_create_key_ex_w &&
        api->reg_set_value_ex_w &&
        api->reg_query_value_ex_w &&
        api->reg_close_key;
}

BOOL
dumbpad_save_settings(const DumbpadSettings *settings, BOOL *backend_available)
{
    HMODULE advapi = LoadLibraryW(L"advapi32.dll");
    DumbpadRegistryApi api;
    HKEY key;
    DWORD disposition;
    DWORD theme;
    DWORD font_height;
    size_t bytes;

    if (!advapi) {
        if (backend_available) {
            *backend_available = FALSE;
        }
        return FALSE;
    }

    if (!load_registry_api(advapi, &api)) {
        FreeLibrary(advapi);
        if (backend_available) {
            *backend_available = FALSE;
        }
        return FALSE;
    }

    if (api.reg_create_key_ex_w(HKEY_CURRENT_USER, L"Software\\dumbpad", 0, NULL, 0, KEY_WRITE, NULL, &key, &disposition) != ERROR_SUCCESS) {
        FreeLibrary(advapi);
        if (backend_available) {
            *backend_available = TRUE;
        }
        return FALSE;
    }

    theme = (DWORD)settings->theme;
    font_height = (DWORD)settings->font_height;
    bytes = (wcslen(settings->font_face) + 1) * sizeof(WCHAR);

    api.reg_set_value_ex_w(key, L"Theme", 0, REG_DWORD, (const BYTE *)&theme, sizeof(theme));
    api.reg_set_value_ex_w(key, L"FontHeight", 0, REG_DWORD, (const BYTE *)&font_height, sizeof(font_height));
    api.reg_set_value_ex_w(key, L"FontFace", 0, REG_SZ, (const BYTE *)settings->font_face, (DWORD)bytes);
    api.reg_close_key(key);
    FreeLibrary(advapi);

    if (backend_available) {
        *backend_available = TRUE;
    }
    return TRUE;
}

BOOL
dumbpad_load_settings(DumbpadSettings *settings, BOOL *backend_available)
{
    HMODULE advapi = LoadLibraryW(L"advapi32.dll");
    DumbpadRegistryApi api;
    HKEY key;
    DWORD disposition;
    DWORD type;
    DWORD size;
    DWORD theme;
    DWORD font_height;

    if (!advapi) {
        if (backend_available) {
            *backend_available = FALSE;
        }
        return FALSE;
    }

    if (!load_registry_api(advapi, &api)) {
        FreeLibrary(advapi);
        if (backend_available) {
            *backend_available = FALSE;
        }
        return FALSE;
    }

    if (api.reg_create_key_ex_w(HKEY_CURRENT_USER, L"Software\\dumbpad", 0, NULL, 0, KEY_READ, NULL, &key, &disposition) != ERROR_SUCCESS) {
        FreeLibrary(advapi);
        if (backend_available) {
            *backend_available = TRUE;
        }
        return FALSE;
    }

    size = sizeof(theme);
    if (api.reg_query_value_ex_w(key, L"Theme", NULL, &type, (LPBYTE)&theme, &size) == ERROR_SUCCESS && type == REG_DWORD) {
        if (theme <= THEME_DARK) {
            settings->theme = (ThemeMode)theme;
        }
    }

    size = sizeof(font_height);
    if (api.reg_query_value_ex_w(key, L"FontHeight", NULL, &type, (LPBYTE)&font_height, &size) == ERROR_SUCCESS && type == REG_DWORD) {
        settings->font_height = (int)font_height;
    }

    size = sizeof(settings->font_face);
    api.reg_query_value_ex_w(key, L"FontFace", NULL, &type, (LPBYTE)settings->font_face, &size);

    api.reg_close_key(key);
    FreeLibrary(advapi);

    if (backend_available) {
        *backend_available = TRUE;
    }
    return TRUE;
}

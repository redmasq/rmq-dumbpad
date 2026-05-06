#include <windows.h>
#include <commdlg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "app_types.h"
#include "file_io.h"
#include "line_endings.h"
#include "settings.h"

#define APP_CLASS_NAME L"DumbpadMainWindow"
#define APP_TITLE L"dumbpad"
#define IDM_FILE_NEW 100
#define IDM_FILE_OPEN 101
#define IDM_FILE_SAVE 102
#define IDM_FILE_SAVE_AS 103
#define IDM_FILE_EXIT 104

#define IDM_EDIT_CUT 200
#define IDM_EDIT_COPY 201
#define IDM_EDIT_PASTE 202
#define IDM_EDIT_DELETE 203
#define IDM_EDIT_SELECT_ALL 204
#define IDM_EDIT_FIND 205
#define IDM_EDIT_REPLACE 206
#define IDM_EDIT_GOTO 207

#define IDM_VIEW_FONT 300
#define IDM_VIEW_THEME_SYSTEM 301
#define IDM_VIEW_THEME_LIGHT 302
#define IDM_VIEW_THEME_DARK 303

#define IDM_OPTIONS_SAVE_SETTINGS 400

#define ID_EDIT_CONTROL 1000
#define ID_GOTO_EDIT 1100

typedef struct AppState {
    HWND window;
    HWND edit;
    HINSTANCE instance;
    WCHAR file_path[MAX_PATH];
    HFONT font;
    int font_height;
    WCHAR font_face[LF_FACESIZE];
    BOOL modified;
    ThemeMode theme;
    BOOL advapi_available;
    UINT find_msg;
    HWND find_dialog;
    FINDREPLACEW find_replace;
    WCHAR find_text[128];
    WCHAR replace_text[128];
    long pending_goto_line;
    DumbpadTextEncoding current_encoding;
    BOOL has_invalid_unicode;
    DumbpadLineEndingMode current_line_endings;
    DumbpadLineEndingMode detected_line_endings;
} AppState;

static AppState g_app;

static const COLORREF g_light_bg = RGB(255, 255, 255);
static const COLORREF g_light_fg = RGB(0, 0, 0);
static const COLORREF g_dark_bg = RGB(30, 30, 30);
static const COLORREF g_dark_fg = RGB(230, 230, 230);

static HBRUSH
theme_brush(void)
{
    static HBRUSH light = NULL;
    static HBRUSH dark = NULL;

    if (!light) {
        light = CreateSolidBrush(g_light_bg);
    }
    if (!dark) {
        dark = CreateSolidBrush(g_dark_bg);
    }
    return g_app.theme == THEME_DARK ? dark : light;
}

static void
set_title(void)
{
    WCHAR title[MAX_PATH + 96];
    WCHAR encoding[24];
    WCHAR line_endings[16];
    const WCHAR *name = g_app.file_path[0] ? g_app.file_path : L"(untitled)";
    MultiByteToWideChar(CP_ACP, 0, dumbpad_text_encoding_name(g_app.current_encoding), -1,
        encoding, (int)(sizeof(encoding) / sizeof(encoding[0])));
    MultiByteToWideChar(CP_ACP, 0, dumbpad_line_ending_name(g_app.current_line_endings), -1,
        line_endings, (int)(sizeof(line_endings) / sizeof(line_endings[0])));
    swprintf(title, sizeof(title) / sizeof(title[0]), L"%ls%ls [%ls/%ls]%ls%ls - %ls",
        g_app.modified ? L"*" : L"",
        name,
        encoding,
        line_endings,
        g_app.detected_line_endings == DUMBPAD_LINE_ENDINGS_MIXED ? L" [mixed line endings]" : L"",
        g_app.has_invalid_unicode ? L" [invalid unicode]" : L"",
        APP_TITLE);
    SetWindowTextW(g_app.window, title);
}

static void
mark_modified(BOOL modified)
{
    g_app.modified = modified;
    set_title();
}

static size_t
edit_text_length(void)
{
    return (size_t)GetWindowTextLengthW(g_app.edit);
}

static WCHAR *
alloc_edit_text(void)
{
    size_t len = edit_text_length();
    WCHAR *buf = (WCHAR *)calloc(len + 1, sizeof(WCHAR));
    if (!buf) {
        return NULL;
    }
    GetWindowTextW(g_app.edit, buf, (int)(len + 1));
    return buf;
}

static BOOL
save_current_file(const WCHAR *path)
{
    WCHAR *text = alloc_edit_text();
    BOOL ok;

    if (!text) {
        return FALSE;
    }
    ok = dumbpad_save_text_file(path, text, g_app.current_encoding, g_app.current_line_endings);
    free(text);
    if (ok) {
        wcsncpy(g_app.file_path, path, MAX_PATH - 1);
        g_app.file_path[MAX_PATH - 1] = L'\0';
        g_app.has_invalid_unicode = FALSE;
        g_app.detected_line_endings = g_app.current_line_endings;
        mark_modified(FALSE);
    }
    return ok;
}

static void
apply_font(void)
{
    LOGFONTW lf;

    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = g_app.font_height;
    lf.lfWeight = FW_NORMAL;
    wcsncpy(lf.lfFaceName, g_app.font_face, LF_FACESIZE - 1);

    if (g_app.font) {
        DeleteObject(g_app.font);
    }
    g_app.font = CreateFontIndirectW(&lf);
    SendMessageW(g_app.edit, WM_SETFONT, (WPARAM)g_app.font, TRUE);
}

static void
set_default_font(void)
{
    HDC dc = GetDC(g_app.window);
    int dpi = GetDeviceCaps(dc, LOGPIXELSY);
    ReleaseDC(g_app.window, dc);

    g_app.font_height = -MulDiv(16, dpi, 72);
    wcsncpy(g_app.font_face, L"Consolas", LF_FACESIZE - 1);
    apply_font();
}

static void
apply_theme(void)
{
    InvalidateRect(g_app.edit, NULL, TRUE);
    InvalidateRect(g_app.window, NULL, TRUE);
}

static BOOL
save_settings(void)
{
    DumbpadSettings settings;

    settings.theme = g_app.theme;
    settings.font_height = g_app.font_height;
    wcsncpy(settings.font_face, g_app.font_face, LF_FACESIZE - 1);
    settings.font_face[LF_FACESIZE - 1] = L'\0';

    return dumbpad_save_settings(&settings, &g_app.advapi_available);
}

static void
load_settings(void)
{
    DumbpadSettings settings;

    settings.theme = g_app.theme;
    settings.font_height = g_app.font_height;
    wcsncpy(settings.font_face, g_app.font_face, LF_FACESIZE - 1);
    settings.font_face[LF_FACESIZE - 1] = L'\0';

    if (dumbpad_load_settings(&settings, &g_app.advapi_available)) {
        g_app.theme = settings.theme;
        g_app.font_height = settings.font_height;
        wcsncpy(g_app.font_face, settings.font_face, LF_FACESIZE - 1);
        g_app.font_face[LF_FACESIZE - 1] = L'\0';
    }
}

static BOOL
confirm_discard_changes(void)
{
    if (!g_app.modified) {
        return TRUE;
    }
    return MessageBoxW(g_app.window, L"Discard unsaved changes?", APP_TITLE, MB_ICONWARNING | MB_OKCANCEL) == IDOK;
}

static void
do_open(void)
{
    OPENFILENAMEW ofn;
    WCHAR path[MAX_PATH] = L"";

    if (!confirm_discard_changes()) {
        return;
    }

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_app.window;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Text Files\0*.txt;*.log;*.md\0All Files\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&ofn)) {
        DumbpadFileLoadResult load_result;

        if (!dumbpad_load_file_into_edit(g_app.edit, path, &load_result)) {
            MessageBoxW(g_app.window, L"Open failed.", APP_TITLE, MB_ICONERROR | MB_OK);
        } else {
            wcsncpy(g_app.file_path, path, MAX_PATH - 1);
            g_app.file_path[MAX_PATH - 1] = L'\0';
            g_app.current_encoding = load_result.encoding;
            g_app.has_invalid_unicode = load_result.has_invalid_unicode;
            g_app.current_line_endings = load_result.line_endings.preferred_mode;
            g_app.detected_line_endings = load_result.line_endings.detected_mode;
            mark_modified(FALSE);
            if (g_app.has_invalid_unicode) {
                MessageBoxW(g_app.window,
                    L"The file contained invalid Unicode sequences. They were loaded with replacement characters.",
                    APP_TITLE,
                    MB_OK | MB_ICONWARNING);
            }
        }
    }
}

static BOOL
do_save_as(void)
{
    OPENFILENAMEW ofn;
    WCHAR path[MAX_PATH];

    wcsncpy(path, g_app.file_path, MAX_PATH - 1);
    path[MAX_PATH - 1] = L'\0';

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_app.window;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"UTF-8 Text\0*.txt\0All Files\0*.*\0";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameW(&ofn)) {
        return FALSE;
    }

    if (!save_current_file(path)) {
        MessageBoxW(g_app.window, L"Save failed.", APP_TITLE, MB_ICONERROR | MB_OK);
        return FALSE;
    }
    return TRUE;
}

static BOOL
do_save(void)
{
    if (g_app.file_path[0]) {
        return save_current_file(g_app.file_path);
    }
    return do_save_as();
}

static void
choose_font(void)
{
    CHOOSEFONTW cf;
    LOGFONTW lf;

    ZeroMemory(&cf, sizeof(cf));
    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = g_app.font_height;
    wcsncpy(lf.lfFaceName, g_app.font_face, LF_FACESIZE - 1);

    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = g_app.window;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS;
    cf.rgbColors = g_app.theme == THEME_DARK ? g_dark_fg : g_light_fg;

    if (ChooseFontW(&cf)) {
        g_app.font_height = lf.lfHeight;
        wcsncpy(g_app.font_face, lf.lfFaceName, LF_FACESIZE - 1);
        apply_font();
        save_settings();
    }
}

static void
open_find_dialog(BOOL replace)
{
    ZeroMemory(&g_app.find_replace, sizeof(g_app.find_replace));
    g_app.find_replace.lStructSize = sizeof(g_app.find_replace);
    g_app.find_replace.hwndOwner = g_app.window;
    g_app.find_replace.lpstrFindWhat = g_app.find_text;
    g_app.find_replace.lpstrReplaceWith = g_app.replace_text;
    g_app.find_replace.wFindWhatLen = sizeof(g_app.find_text) / sizeof(g_app.find_text[0]);
    g_app.find_replace.wReplaceWithLen = sizeof(g_app.replace_text) / sizeof(g_app.replace_text[0]);
    g_app.find_replace.Flags = FR_DOWN;

    g_app.find_dialog = replace ? ReplaceTextW(&g_app.find_replace) : FindTextW(&g_app.find_replace);
}

static void
select_found_text(int start, int length)
{
    SendMessageW(g_app.edit, EM_SETSEL, (WPARAM)start, (LPARAM)(start + length));
    SendMessageW(g_app.edit, EM_SCROLLCARET, 0, 0);
    SetFocus(g_app.edit);
}

static void
do_find_next(BOOL reverse)
{
    WCHAR *text = alloc_edit_text();
    DWORD sel_start = 0;
    DWORD sel_end = 0;
    WCHAR *found = NULL;

    if (!text || !g_app.find_text[0]) {
        free(text);
        return;
    }

    SendMessageW(g_app.edit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    if (reverse) {
        WCHAR saved = text[sel_start];
        text[sel_start] = L'\0';
        found = wcsstr(text, g_app.find_text);
        text[sel_start] = saved;
        if (found) {
            WCHAR *next = found;
            while (next) {
                WCHAR *candidate = wcsstr(next + 1, g_app.find_text);
                if (!candidate) {
                    break;
                }
                found = candidate;
                next = candidate;
            }
        }
    } else {
        found = wcsstr(text + sel_end, g_app.find_text);
    }

    if (found) {
        select_found_text((int)(found - text), (int)wcslen(g_app.find_text));
    } else {
        MessageBoxW(g_app.window, L"No further match found.", APP_TITLE, MB_OK | MB_ICONINFORMATION);
    }
    free(text);
}

static void
do_replace_current(void)
{
    DWORD sel_start = 0;
    DWORD sel_end = 0;
    WCHAR *text;
    WCHAR *selected;
    size_t len;

    SendMessageW(g_app.edit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    if (sel_end <= sel_start) {
        do_find_next(FALSE);
        return;
    }

    text = alloc_edit_text();
    if (!text) {
        return;
    }

    len = (size_t)(sel_end - sel_start);
    selected = (WCHAR *)calloc(len + 1, sizeof(WCHAR));
    if (!selected) {
        free(text);
        return;
    }

    wmemcpy(selected, text + sel_start, len);
    if (wcscmp(selected, g_app.find_text) == 0) {
        SendMessageW(g_app.edit, EM_REPLACESEL, TRUE, (LPARAM)g_app.replace_text);
        mark_modified(TRUE);
    }
    free(text);
    free(selected);
    do_find_next(FALSE);
}

static INT_PTR CALLBACK
goto_dialog_proc(HWND dialog, UINT msg, WPARAM wparam, LPARAM lparam)
{
    (void)lparam;
    switch (msg) {
    case WM_INITDIALOG:
        SetDlgItemTextW(dialog, ID_GOTO_EDIT, L"1");
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDOK:
            {
                WCHAR buf[32];
                HWND edit = GetDlgItem(dialog, ID_GOTO_EDIT);

                g_app.pending_goto_line = 0;
                if (edit) {
                    GetWindowTextW(edit, buf, 32);
                    g_app.pending_goto_line = wcstol(buf, NULL, 10);
                }
                EndDialog(dialog, IDOK);
                return TRUE;
            }
        case IDCANCEL:
            EndDialog(dialog, LOWORD(wparam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void
do_goto_line(void)
{
    /*
     * This compact in-memory dialog template avoids introducing a resource
     * toolchain before the project has real UI content.
     */
    struct {
        DLGTEMPLATE dlg;
        WORD menu;
        WORD cls;
        WCHAR title[8];
        WORD point_size;
        WCHAR font[14];
        WORD align1;
        DLGITEMTEMPLATE label;
        WORD label_cls[2];
        WCHAR label_title[6];
        WORD align2;
        DLGITEMTEMPLATE edit;
        WORD edit_cls[2];
        WORD edit_title;
        WORD edit_extra;
        WORD align3;
        DLGITEMTEMPLATE ok;
        WORD ok_cls[2];
        WCHAR ok_title[3];
        WORD ok_extra;
        WORD align4;
        DLGITEMTEMPLATE cancel;
        WORD cancel_cls[2];
        WCHAR cancel_title[7];
        WORD cancel_extra;
    } tmpl = {
        .dlg = {WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION | DS_SETFONT, 0, 4, 10, 10, 160, 60},
        .menu = 0, .cls = 0, .title = L"Go To", .point_size = 8, .font = L"MS Shell Dlg",
        .align1 = 0,
        .label = {WS_CHILD | WS_VISIBLE, 0, 8, 10, 32, 12, (WORD)-1},
        .label_cls = {0xFFFF, 0x0082}, .label_title = L"Line:",
        .align2 = 0,
        .edit = {WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 44, 8, 60, 14, ID_GOTO_EDIT},
        .edit_cls = {0xFFFF, 0x0081}, .edit_title = 0, .edit_extra = 0,
        .align3 = 0,
        .ok = {WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 32, 30, 40, 14, IDOK},
        .ok_cls = {0xFFFF, 0x0080}, .ok_title = L"OK", .ok_extra = 0,
        .align4 = 0,
        .cancel = {WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 78, 30, 50, 14, IDCANCEL},
        .cancel_cls = {0xFFFF, 0x0080}, .cancel_title = L"Cancel", .cancel_extra = 0
    };
    INT_PTR result;

    g_app.pending_goto_line = 0;
    result = DialogBoxIndirectParamW(g_app.instance, &tmpl.dlg, g_app.window, goto_dialog_proc, 0);
    if (result == IDOK) {
        long line = g_app.pending_goto_line;

        if (line > 0) {
            LRESULT index = SendMessageW(g_app.edit, EM_LINEINDEX, (WPARAM)(line - 1), 0);
            if (index >= 0) {
                select_found_text((int)index, 0);
            }
        }
    }
}

static HMENU
build_menu(void)
{
    HMENU bar = CreateMenu();
    HMENU file = CreatePopupMenu();
    HMENU edit = CreatePopupMenu();
    HMENU view = CreatePopupMenu();
    HMENU options = CreatePopupMenu();

    AppendMenuW(file, MF_STRING, IDM_FILE_NEW, L"&New");
    AppendMenuW(file, MF_STRING, IDM_FILE_OPEN, L"&Open...");
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE, L"&Save");
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE_AS, L"Save &As...");
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
    AppendMenuW(file, MF_STRING, IDM_FILE_EXIT, L"E&xit");

    AppendMenuW(edit, MF_STRING, IDM_EDIT_CUT, L"Cu&t");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_COPY, L"&Copy");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_PASTE, L"&Paste");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_DELETE, L"&Delete");
    AppendMenuW(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(edit, MF_STRING, IDM_EDIT_FIND, L"&Find...");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_REPLACE, L"&Replace...");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_GOTO, L"&Go To...");
    AppendMenuW(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(edit, MF_STRING, IDM_EDIT_SELECT_ALL, L"Select &All");

    AppendMenuW(view, MF_STRING, IDM_VIEW_FONT, L"&Font...");
    AppendMenuW(view, MF_SEPARATOR, 0, NULL);
    AppendMenuW(view, MF_STRING, IDM_VIEW_THEME_SYSTEM, L"Theme: &System");
    AppendMenuW(view, MF_STRING, IDM_VIEW_THEME_LIGHT, L"Theme: &Light");
    AppendMenuW(view, MF_STRING, IDM_VIEW_THEME_DARK, L"Theme: &Dark");

    AppendMenuW(options, MF_STRING, IDM_OPTIONS_SAVE_SETTINGS, L"&Save Settings Now");

    AppendMenuW(bar, MF_POPUP, (UINT_PTR)file, L"&File");
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)edit, L"&Edit");
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)view, L"&View");
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)options, L"&Options");
    return bar;
}

static void
update_menu_state(void)
{
    HMENU menu = GetMenu(g_app.window);
    CheckMenuRadioItem(menu, IDM_VIEW_THEME_SYSTEM, IDM_VIEW_THEME_DARK,
        IDM_VIEW_THEME_SYSTEM + g_app.theme, MF_BYCOMMAND);
    EnableMenuItem(menu, IDM_OPTIONS_SAVE_SETTINGS, MF_BYCOMMAND | (g_app.advapi_available ? MF_ENABLED : MF_GRAYED));
}

static LRESULT CALLBACK
window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == g_app.find_msg) {
        LPFINDREPLACEW fr = (LPFINDREPLACEW)lparam;
        if (fr->Flags & FR_DIALOGTERM) {
            g_app.find_dialog = NULL;
            return 0;
        }
        if (fr->Flags & FR_FINDNEXT) {
            do_find_next((fr->Flags & FR_DOWN) == 0);
            return 0;
        }
        if (fr->Flags & FR_REPLACE) {
            do_replace_current();
            return 0;
        }
        if (fr->Flags & FR_REPLACEALL) {
            while (1) {
                WCHAR *text = alloc_edit_text();
                WCHAR *found;
                if (!text) {
                    break;
                }
                found = wcsstr(text, g_app.find_text);
                free(text);
                if (!found) {
                    break;
                }
                SendMessageW(g_app.edit, EM_SETSEL, 0, -1);
                do_find_next(FALSE);
                do_replace_current();
            }
            return 0;
        }
    }

    switch (msg) {
    case WM_CREATE:
        g_app.window = hwnd;
        g_app.edit = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
            0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)ID_EDIT_CONTROL, g_app.instance, NULL);
        if (!g_app.edit) {
            return -1;
        }
        load_settings();
        if (!g_app.font_face[0]) {
            set_default_font();
        } else {
            apply_font();
        }
        apply_theme();
        set_title();
        update_menu_state();
        return 0;
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
        if ((HWND)lparam == g_app.edit && g_app.theme != THEME_SYSTEM) {
            HDC dc = (HDC)wparam;
            SetBkColor(dc, g_app.theme == THEME_DARK ? g_dark_bg : g_light_bg);
            SetTextColor(dc, g_app.theme == THEME_DARK ? g_dark_fg : g_light_fg);
            return (LRESULT)theme_brush();
        }
        break;
    case WM_SIZE:
        MoveWindow(g_app.edit, 0, 0, LOWORD(lparam), HIWORD(lparam), TRUE);
        return 0;
    case WM_COMMAND:
        if ((HWND)lparam == g_app.edit && HIWORD(wparam) == EN_CHANGE) {
            if (!g_app.modified) {
                mark_modified(TRUE);
            }
        }
        switch (LOWORD(wparam)) {
        case IDM_FILE_NEW:
            if (confirm_discard_changes()) {
                SetWindowTextW(g_app.edit, L"");
                g_app.file_path[0] = L'\0';
                g_app.current_encoding = DUMBPAD_TEXT_ENCODING_UTF8;
                g_app.has_invalid_unicode = FALSE;
                g_app.current_line_endings = DUMBPAD_LINE_ENDINGS_CRLF;
                g_app.detected_line_endings = DUMBPAD_LINE_ENDINGS_NONE;
                mark_modified(FALSE);
            }
            return 0;
        case IDM_FILE_OPEN:
            do_open();
            return 0;
        case IDM_FILE_SAVE:
            do_save();
            return 0;
        case IDM_FILE_SAVE_AS:
            do_save_as();
            return 0;
        case IDM_FILE_EXIT:
            SendMessageW(hwnd, WM_CLOSE, 0, 0);
            return 0;
        case IDM_EDIT_CUT:
            SendMessageW(g_app.edit, WM_CUT, 0, 0);
            return 0;
        case IDM_EDIT_COPY:
            SendMessageW(g_app.edit, WM_COPY, 0, 0);
            return 0;
        case IDM_EDIT_PASTE:
            SendMessageW(g_app.edit, WM_PASTE, 0, 0);
            return 0;
        case IDM_EDIT_DELETE:
            SendMessageW(g_app.edit, WM_CLEAR, 0, 0);
            return 0;
        case IDM_EDIT_SELECT_ALL:
            SendMessageW(g_app.edit, EM_SETSEL, 0, -1);
            return 0;
        case IDM_EDIT_FIND:
            open_find_dialog(FALSE);
            return 0;
        case IDM_EDIT_REPLACE:
            open_find_dialog(TRUE);
            return 0;
        case IDM_EDIT_GOTO:
            do_goto_line();
            return 0;
        case IDM_VIEW_FONT:
            choose_font();
            return 0;
        case IDM_VIEW_THEME_SYSTEM:
            g_app.theme = THEME_SYSTEM;
            apply_theme();
            update_menu_state();
            save_settings();
            return 0;
        case IDM_VIEW_THEME_LIGHT:
            g_app.theme = THEME_LIGHT;
            apply_theme();
            update_menu_state();
            save_settings();
            return 0;
        case IDM_VIEW_THEME_DARK:
            g_app.theme = THEME_DARK;
            apply_theme();
            update_menu_state();
            save_settings();
            return 0;
        case IDM_OPTIONS_SAVE_SETTINGS:
            save_settings();
            update_menu_state();
            return 0;
        }
        break;
    case WM_CLOSE:
        if (!confirm_discard_changes()) {
            return 0;
        }
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (g_app.font) {
            DeleteObject(g_app.font);
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI
wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR cmd_line, int show_cmd)
{
    WNDCLASSW wc;
    MSG msg;

    (void)prev_instance;
    (void)cmd_line;

    ZeroMemory(&g_app, sizeof(g_app));
    g_app.instance = instance;
    g_app.theme = THEME_SYSTEM;
    g_app.find_msg = RegisterWindowMessageW(FINDMSGSTRING);
    g_app.advapi_available = TRUE;
    g_app.pending_goto_line = 0;
    g_app.current_encoding = DUMBPAD_TEXT_ENCODING_UTF8;
    g_app.has_invalid_unicode = FALSE;
    g_app.current_line_endings = DUMBPAD_LINE_ENDINGS_CRLF;
    g_app.detected_line_endings = DUMBPAD_LINE_ENDINGS_NONE;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(NULL, IDC_IBEAM);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = APP_CLASS_NAME;

    if (!RegisterClassW(&wc)) {
        return 1;
    }

    g_app.window = CreateWindowExW(
        0, APP_CLASS_NAME, APP_TITLE,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
        NULL, build_menu(), instance, NULL);
    if (!g_app.window) {
        return 1;
    }

    ShowWindow(g_app.window, show_cmd);
    UpdateWindow(g_app.window);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!g_app.find_dialog || !IsDialogMessageW(g_app.find_dialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return (int)msg.wParam;
}

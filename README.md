# dumbpad

`dumbpad` is a small Win32 text editor scaffold intended to stay close to classic Notepad behavior while keeping a very tight dependency surface:

- `kernel32.dll`
- `user32.dll`
- `gdi32.dll`
- `comdlg32.dll`
- optional dynamic load of `advapi32.dll` for settings persistence

Current scaffold features:

- native Win32 window and multiline `EDIT` control
- `Open`, `Save`, `Save As`
- font picker with size support
- common dialog based `Find`, `Replace`, `Go To`
- theme overrides: system, light, dark
- optional registry settings persistence via runtime-loaded `advapi32.dll`
- build targets for both `win32` and `win64`

Planned milestones:

1. Encoding-aware load/save for ASCII, UTF-8, UTF-16 LE/BE with and without BOM
2. Explicit line ending mode conversion (`CRLF`, `LF`, `CR`)
3. Raw edit mode with control-character insertion dialog
4. Clipboard special formats UI
5. Pattern search extensions (`glob`, limited regex-like engine)
6. Binary / hex view mode

Build:

```bash
make win32
make win64
```

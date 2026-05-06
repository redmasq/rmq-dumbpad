# dumbpad Notes

This file captures project-wide engineering notes that should shape implementation choices as the editor grows.

## Core Principles

- Keep the runtime dependency surface intentionally small:
  - Required: `kernel32.dll`, `user32.dll`, `gdi32.dll`, `comdlg32.dll`
  - Optional at runtime: `advapi32.dll`
- Prefer straightforward Win32 code over magic, but do not let that collapse the codebase into one giant file.
- Build a stable core that can support text mode, raw mode, and later binary/hex mode without rewriting everything.

## Separation Of Concerns

This needs to be an explicit project rule, not an afterthought.

Desired boundaries:

- `platform/win32`:
  - Window creation
  - Message loop
  - Menus
  - accelerators / keybinding dispatch
  - Dialog plumbing
  - Clipboard integration
  - File dialogs
  - Font selection
  - Theme application
- `document`:
  - In-memory representation of the current file
  - Dirty state
  - Current encoding
  - Current line ending mode
  - Current insert/overwrite mode
  - Current view mode (`text`, `raw`, later `hex`)
  - File metadata snapshot for external-change detection
  - Anchor positions for cross-mode selection
  - policy for display and insertion of control characters
- `large_file`:
  - Chunked read path for files that should not be loaded fully into memory
  - Active chunk/window state
  - File offset mapping
  - Lazy line or view indexing where needed
  - Cross-chunk copy support
- `io`:
  - Load/save logic
  - BOM detection
  - Encoding conversion
  - Line-ending normalization and emission
  - Binary-safe file access
  - invalid Unicode detection and repair strategy
- `search`:
  - Plain text search
  - Replace
  - Glob-style matcher
  - Optional limited regex-style engine
  - Byte-pattern matcher for hex mode
- `settings`:
  - Runtime detection of optional settings backend
  - Registry persistence
  - Graceful disablement when unavailable
- `help`:
  - Optional help backend detection and binding
  - Help menu command routing
  - Content packaging and indexed lookup strategy
- `commands`:
  - User-intent operations such as open, save, find-next, replace, go-to, toggle-theme
  - reload-from-disk
  - toggle word wrap
  - toggle insert/overwrite mode
  - respond to external file change warnings
  - set selection anchor start/end
  - copy anchored range
  - normalize unicode
  - open help topics
  - insert selected control characters
  - Keep command logic separate from raw `WM_COMMAND` plumbing

What to avoid:

- UI code directly owning file-format logic
- Accelerator/keybinding behavior hard-coded in scattered message branches without a clear command map
- Search logic embedded inside dialog callbacks
- Encoding decisions scattered across menu handlers
- Future hex-mode logic mixed into the text editor widget code
- File monitoring decisions embedded directly in menu handlers or paint/update code
- Large-file chunk management hidden inside ordinary small-file text editing logic
- Optional help-system behavior hard-wired into core editor startup assumptions

Short version:

- Win32 layer should translate UI events into commands
- Core modules should implement behavior
- Shared state should live in well-defined data structures, not in ad hoc globals

## Recommended Near-Term Refactor

The current single-file scaffold was fine to get the project moving, but it should not stay that way for long.

Suggested first split:

- `src/main.c`
  - process entrypoint only
- `src/app.c`, `src/app.h`
  - app lifecycle and top-level state
- `src/win32_ui.c`, `src/win32_ui.h`
  - window procedure, menus, dialogs, edit-control integration
- `src/document.c`, `src/document.h`
  - document state and editor-facing operations
- `src/file_io.c`, `src/file_io.h`
  - encoding detection and file load/save
- `src/settings.c`, `src/settings.h`
  - optional `advapi32` integration
- `src/search.c`, `src/search.h`
  - text and pattern matching

This will make the future testing story much better.

## Unit Testing Plan

Unit testing is feasible for a meaningful subset of this project, even though the UI itself is Win32-heavy.

### Good Candidates For Unit Tests

- Encoding detection
  - UTF-8 BOM
  - UTF-16 LE/BE BOM
  - BOM-less UTF-8 validation
  - ASCII fallback behavior
- Invalid Unicode handling
  - malformed UTF-8
  - malformed UTF-16 surrogate usage
  - replacement or preservation policy depending on mode
  - supplementary-plane round-trip behavior
- Decode/encode transforms
  - Unicode text to UTF-8
  - UTF-8 to internal text representation
  - UTF-16 LE/BE read and write
- Line ending logic
  - detect `CRLF`, `LF`, `CR`
  - normalize mixed endings
  - emit chosen save mode correctly
- Search engines
  - plain forward/backward search
  - replace-one / replace-all behavior
  - glob matching semantics for `*`, `?`, `%{min,max}`, and escapes
  - byte-pattern matching for future hex mode
- Position parsing
  - parse `line`
  - parse `line:col`
  - parse raw offsets
- External change detection logic
  - compare stored file size and timestamp against current metadata
  - distinguish unchanged, changed, and unavailable states
- Insert/overwrite editing helpers
  - overwrite semantics on simple text buffers once editing logic is factored out of the UI
- Anchor and range logic
  - normalize start/end ordering
  - represent ranges across chunks or offsets
  - clamp or validate invalid positions
- Large-file chunk mapping helpers
  - map absolute offsets to chunk-local positions
  - compute next/previous chunk windows
  - copy a range spanning multiple chunks
- Allocation and bounds handling
  - truncated inputs
  - oversized lengths
  - invalid ranges
  - graceful behavior when allocation fails in testable core helpers
- Settings serialization logic
  - parse/apply settings structs
  - backend-available vs backend-unavailable behavior

### Poor Candidates For Pure Unit Tests

- Window creation
- Message dispatch details
- Common dialog invocation
- Native clipboard interaction
- GDI drawing behavior

Those are better handled by careful manual testing and by isolating logic away from the Win32 calls.

## Testing Strategy

### 1. Pure C Unit Tests

Goal:
- Test core logic without needing a GUI or Windows runtime

Approach:
- Keep parsers, matchers, encoding helpers, and document transforms free of direct Win32 dependencies where possible
- Keep anchor/range logic and large-file chunk math free of direct Win32 dependencies
- Build small test executables for host Linux with native `gcc` in addition to MinGW builds where practical
- Prefer table-driven tests for encoding, line endings, and pattern matching
- Include explicit test vectors for malformed Unicode and supplementary-plane characters

### 2. Cross-Compiled Smoke Tests

Goal:
- Confirm the Windows targets still compile and link cleanly

Approach:
- Keep `make win32` and `make win64` as baseline build checks
- Add a future `make check-build` convenience target that runs both

### 3. Manual UI Test Checklists

Goal:
- Cover behavior that is difficult to unit test in a pure-C Win32 app

Checklist areas:
- open/save dialogs
- font selection
- dark/light/system overrides
- find/replace dialogs
- go-to behavior
- clipboard operations
- settings persistence available/unavailable

## Test Harness Direction

Keep it simple.

Recommended shape:

- `tests/`
  - `test_main.c`
  - `test_encoding.c`
  - `test_line_endings.c`
  - `test_search.c`
  - `test_position.c`
- tiny assertion helpers in `tests/test.h`
- no heavy external framework unless it clearly pays for itself

Minimal style:

- return nonzero on failure
- print readable diagnostics
- keep each test module focused on one subsystem
- include edge-case coverage for allocation failure and bounds-sensitive logic wherever feasible

## Milestone Policy

Before adding large user-visible features, prefer this order:

1. make the core compile cleanly
2. split core logic out of the UI
3. add unit tests for the extracted logic
4. expand features on top of the tested core

That order should keep the project from becoming difficult to change once raw mode and hex mode arrive.

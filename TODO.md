# dumbpad TODO

This file tracks pseudo-tickets for the project in a simple, implementation-first format.

## Development Order Plan

The feature list is intentionally ambitious, so development order matters a lot.

Working rule:
- stabilize the plain text editor first
- extract reusable core logic second
- add advanced modes only after the core is strong enough to support them cleanly

Recommended milestone flow:

1. Build-clean scaffold and early refactor
2. Encoding and line-ending foundation
3. Core editor usability features
4. Search and navigation improvements
5. Selection model and cross-mode groundwork
6. Advanced modes: raw, large-file, binary/hex

### Phase 1: Foundation

Primary focus:
- `Ticket 001 / DUMB-1: Build-Clean Win32 Scaffold`
- early separation of concerns work around `file_io`, `settings`, and shared core types

Exit criteria:
- both Windows targets build cleanly
- host-side unit tests exist for extracted pure logic
- `main.c` is no longer the only place where behavior lives

### Phase 2: Text File Core

Primary focus:
- `Ticket 002: Encoding-Aware Load/Save`
- `Ticket 003 / DUMB-3: Line Ending Modes`
- baseline work from `Ticket 019 / DUMB-19: Invalid Unicode Handling And Normalization`
- test coverage pressure from `Ticket 018 / DUMB-18: Allocation Failure And Bounds Testing`

Why next:
- these define the behavior of the document itself
- many later features depend on encoding and line-ending correctness

Exit criteria:
- text files open/save reliably across the supported encodings
- line-ending handling is explicit and testable
- invalid Unicode is detected and surfaced in a basic, non-destructive way

### Phase 3: Core Editor UX

Primary focus:
- `Ticket 005: Theme And Font Polish`
- `Ticket 006 / DUMB-6: Word Wrap`
- `Ticket 007 / DUMB-7: Insert / Overwrite Editing`
- `Ticket 010 / DUMB-10: Settings Persistence Fallback`
- `Ticket 009 / DUMB-9: Reload From Disk`
- basic passive-mode support from `Ticket 008 / DUMB-8: File Access Mode And External Change Detection`

Why here:
- these features improve the normal editor path without forcing a new document model
- they make the app useful early while keeping complexity controlled

Exit criteria:
- the plain text editor feels coherent and dependable for ordinary use

### Phase 4: Search And Navigation

Primary focus:
- `Ticket 004: Find / Replace Engine`
- `Ticket 011 / DUMB-12: Go To And Positioning`

Why here:
- search and navigation should build on the finalized text/line-ending model
- this is also a good stage to continue extracting pure logic for tests

Exit criteria:
- normal text find/replace is solid
- line, line+column, and offset navigation design is settled

### Phase 5: Shared Selection And Metadata Infrastructure

Primary focus:
- `Ticket 017: Anchor-Based Selection`
- deeper work from `Ticket 008 / DUMB-8: File Access Mode And External Change Detection`

Why here:
- anchor-based selection can become the shared range model across normal, raw, large-file, and hex modes
- file metadata tracking becomes increasingly important once reloading and alternate modes exist

Exit criteria:
- selection/range logic is no longer tied only to the stock edit control
- external-change tracking has reusable core logic

### Phase 6: Advanced Modes

Primary focus:
- `Ticket 015: Large File Read Mode`
- `Ticket 012 / DUMB-11: Raw Edit Mode`
- `Ticket 014 / DUMB-14: Binary / Hex Mode`
- `Ticket 016 / DUMB-16: Optional Large File Edit Mode`

Why deferred:
- these are the most architecture-sensitive features
- they are likely to need their own view models or rendering assumptions
- they should reuse the tested core instead of dictating it too early

Exit criteria:
- each advanced mode is added intentionally, with explicit feature limits and UI distinctions

### Priority Summary

Must-have before advanced modes:
- `001 / DUMB-1`, `002 / DUMB-2`, `003 / DUMB-3`, `004 / DUMB-4`, `008 / DUMB-8`, `009 / DUMB-9`, `010 / DUMB-10`, `011 / DUMB-12`, `019 / DUMB-19`

High-value plain-editor features:
- `005 / DUMB-5`, `006 / DUMB-6`, `007 / DUMB-7`

Cross-mode infrastructure:
- `017 / DUMB-17`

Defer until architecture is ready:
- `012 / DUMB-11`, `014 / DUMB-14`, `015 / DUMB-15`, `016 / DUMB-16`

Always-on quality work:
- `018 / DUMB-18`
- `019 / DUMB-19`

Distribution and packaging:
- `020 / DUMB-20`
- `021 / DUMB-21`
- `022 / DUMB-22`
- `023 / DUMB-23`

### Important Constraint

Do not let large-file, raw, or hex requirements distort the small-file text editor too early.

The normal text editor path should become stable first, then shared abstractions can be widened to support the more specialized modes.

## Ticket 001 / DUMB-1: Build-Clean Win32 Scaffold

Status: In progress

Goals:
- Make the current source compile for both `i686-w64-mingw32-gcc` and `x86_64-w64-mingw32-gcc`
- Remove scaffold-level API mistakes against the stock Win32 `EDIT` control
- Keep the DLL surface limited to `kernel32`, `user32`, `gdi32`, `comdlg32`, plus optional runtime-loaded `advapi32`

Acceptance:
- `make win32` succeeds
- `make win64` succeeds
- Resulting binaries launch a main window with the editor control and menus

## Ticket 002 / DUMB-2: Encoding-Aware Load/Save

Status: Pending

Goals:
- Support ASCII, UTF-8, UTF-16 LE, UTF-16 BE
- Detect BOM when present
- Support BOM and no-BOM variants where feasible
- Preserve or explicitly choose encoding on save

Acceptance:
- Open/save round-trips basic files in each supported encoding
- UI reports or remembers current encoding choice

## Ticket 003 / DUMB-3: Line Ending Modes

Status: Pending

Goals:
- Track and convert `CRLF`, `LF`, and `CR`
- Allow user selection for save mode
- Detect mixed line endings and handle them predictably

Acceptance:
- Save can force each line-ending mode
- Open detects the dominant or exact mode and exposes it in UI state

## Ticket 004 / DUMB-4: Find / Replace Engine

Status: Pending

Goals:
- Stabilize plain text search and replace
- Add whole-word and case-sensitive options
- Add optional glob-style matching with `*`, `?`, `%{min,max}`, and escapes
- Add an optional extended mode for escaped matching:
  - newlines, including any newline or specific newline styles
  - tabs and other common escaped controls
  - character escapes such as `\x01`, `\007`, `\d13`, and similar forms
- Allow extended mode to be combined with glob mode where practical
- Evaluate whether a small regex-like engine is practical under current constraints

Acceptance:
- Standard search and replace works reliably in both directions
- Pattern mode has documented semantics and passing test cases

## Ticket 005 / DUMB-5: Theme And Font Polish

Status: Pending

Goals:
- Finish light/dark/system override behavior
- Apply colors consistently to editor and surrounding chrome where possible
- Improve font persistence and startup defaults

Acceptance:
- Theme override behaves consistently after relaunch
- Font family and size survive restart when settings are available

## Ticket 006 / DUMB-6: Word Wrap

Status: Pending

Goals:
- Add a word wrap toggle for text editing mode
- Ensure horizontal scrolling behavior changes correctly with wrap on/off
- Decide how wrap interacts with `Go To`, line numbering assumptions, and search navigation
- Persist the wrap preference when settings are available

Acceptance:
- User can toggle word wrap from the UI
- Wrapped and unwrapped modes both remain usable for navigation and editing
- Wrap preference is restored on restart when settings are available

## Ticket 007 / DUMB-7: Insert / Overwrite Editing

Status: Pending

Goals:
- Support toggling between insert mode and overwrite mode via the `Insert` key
- Expose the current editing mode clearly in the UI
- Define overwrite behavior carefully for selections, line endings, and multi-byte encodings
- Persist the preferred startup mode when settings are available if that proves useful

Acceptance:
- Pressing `Insert` toggles editing mode reliably
- Text entry behaves correctly in both modes
- The current mode is visible enough that it is not surprising to the user

## Ticket 008 / DUMB-8: File Access Mode And External Change Detection

Status: Pending

Goals:
- Add a configurable choice between lock-file mode and passive mode
- In lock-file mode, prefer stronger write coordination where feasible with the current DLL constraints
- In passive mode, track whether the open file changed on disk since load/save
- Use at least last-modified time and file size as the baseline change detection mechanism
- Evaluate whether a polling timer is sufficient or whether a Win32 notification API fits within the allowed DLL set

Acceptance:
- User can choose the file access mode
- Passive mode can detect likely on-disk changes after open/save
- The app warns before overwriting or continuing with stale in-memory content

## Ticket 009 / DUMB-9: Reload From Disk

Status: Pending

Goals:
- Add a `Reload from Disk` menu command
- Handle both clean and dirty-buffer cases safely
- Integrate with external-change detection so the user can recover when the file changed outside the editor

Acceptance:
- User can reload the current file from disk from the menu
- Dirty buffers require confirmation before reload
- Reload updates the editor content and file metadata correctly

## Ticket 010 / DUMB-10: Settings Persistence Fallback

Status: Pending

Goals:
- Keep dynamic `advapi32.dll` registry persistence
- If unavailable, degrade gracefully
- Decide whether `.ini` fallback is desirable under the DLL constraints

Acceptance:
- Missing `advapi32.dll` disables settings features without crashing
- UI clearly reflects availability

## Ticket 011 / DUMB-12: Go To And Positioning

Status: Pending

Goals:
- Support `Ctrl+G`
- Support jump to line, line+column, and raw offset
- Validate positions and report invalid input cleanly

Acceptance:
- User can jump accurately using the requested forms

## Ticket 012 / DUMB-11: Raw Edit Mode

Status: Pending

Goals:
- Add a raw mode that does not reinterpret control bytes as text structure
- Provide a dialog to insert control characters such as `CR`, `LF`, and `NUL`
- Decide whether raw mode is implemented in the text view or a separate buffer/view

Acceptance:
- Raw mode preserves control characters exactly
- Insert-control dialog works for core control values

## Ticket 013 / DUMB-13: Clipboard Special

Status: Pending

Goals:
- Add `Copy Special` and `Paste Special`
- Inspect and expose clipboard formats
- Allow transforming between selected supported formats

Acceptance:
- User can choose from at least a few meaningful clipboard formats

## Ticket 014 / DUMB-14: Binary / Hex Mode

Status: Pending

Goals:
- Add an alternate binary viewer/editor mode
- Show offset plus 8 or 16 values in hex, decimal, or octal
- Show corresponding character lane
- Add byte-sequence search with wildcard support

Acceptance:
- Open-as-binary works on arbitrary files
- Byte search handles exact and wildcard patterns

## Ticket 015 / DUMB-15: Large File Read Mode

Status: Pending

Goals:
- Add a smart large-file mode that does not require loading the entire file into memory
- Require passive file-access mode for this feature
- Keep only the active chunk or working set in memory
- Support navigation by offset and practical position tracking within the file
- Support copy and paste operations from the current chunk where those operations make sense for the active mode
- Support external-change detection using file metadata

Acceptance:
- Large files can be opened without fully loading them into memory
- User can navigate and copy data from the active chunk
- The app clearly indicates that this is a chunked passive-mode workflow

## Ticket 016 / DUMB-16: Optional Large File Edit Mode

Status: Pending

Goals:
- Evaluate whether chunked editing for large files is worth the complexity
- Keep this separate from large-file read mode so the basic feature can ship sooner
- Define safe writeback semantics before implementation
- Document feature limits compared with normal in-memory editing

Acceptance:
- Either a safe editing design is approved and implemented, or the feature remains intentionally unsupported
- The user experience makes the distinction between read mode and editable mode clear

## Ticket 017 / DUMB-17: Anchor-Based Selection

Status: Pending

Goals:
- Add selection anchors that work in all modes
- Allow setting start and end anchors from the context menu
- When both anchors are set, select or represent the full range between them
- In large-file mode, allow anchors to exist outside the current chunk
- Support copying the anchored range to the clipboard even when the full range is not currently visible

Acceptance:
- User can set start and end anchors from the UI
- Anchored selections behave predictably in normal text mode, raw mode, and large-file mode
- Large-file anchored copy works across chunk boundaries

## Ticket 018 / DUMB-18: Allocation Failure And Bounds Testing

Status: Pending

Goals:
- Add targeted tests for allocation-failure paths in core logic where feasible
- Add bounds-checking tests for text decoding, range handling, chunk mapping, and file metadata helpers
- Prefer designs that make error handling testable without needing the full Win32 UI
- Identify hot spots where helper functions should reject oversized, truncated, or malformed inputs safely

Acceptance:
- Core modules have explicit tests for representative allocation-failure behavior where practical
- Bounds-sensitive helpers have table-driven tests for edge cases and invalid inputs
- New core logic is expected to handle allocation and bounds failures deliberately rather than implicitly

## Ticket 019 / DUMB-19: Invalid Unicode Handling And Normalization

Status: Pending

Goals:
- Detect invalid Unicode sequences during load, display, search, and save workflows
- Make handling mode-aware:
  - normal text mode
  - raw mode
  - large-file read mode
  - future binary/hex mode where relevant
- Support correct handling of non-Latin scripts and supplementary Unicode planes such as emoji
- Define baseline behavior first:
  - indicate that invalid Unicode exists in the current content
  - add a `Normalize Unicode` menu command or equivalent repair flow
- Preserve the option for later advanced behavior:
  - highlight invalid sequences in views that can support it
  - preserve invalid byte ranges intentionally when the user chooses a non-destructive mode
  - provide ranged or selective normalization rather than only whole-buffer normalization

Acceptance:
- The app can detect and report invalid Unicode content without crashing or silently corrupting data
- Valid Unicode text, including supplementary-plane characters, round-trips correctly in supported encodings
- Baseline UI exists for user awareness and normalization action
- Later visual highlighting and intent-preserving repair paths remain architecturally possible

## Ticket 020 / DUMB-20: Portable Build And Optional Windows Installer

Status: Pending

Goals:
- Keep the primary distribution model as a stand-alone portable `.exe`
- Add an optional Windows installer path for users who want Start Menu integration
- Evaluate a simple installer implementation using either Nullsoft Scriptable Install System or Inno Setup
- Support basic installer tasks:
  - install to a chosen directory
  - create Start Menu shortcuts
  - optional desktop shortcut
  - uninstall entry
- Keep portable mode first-class so the app remains easy to ship and use without installation

Acceptance:
- A portable standalone build remains available
- An optional installer can place the app in the Start Menu and provide uninstall support
- Packaging steps are documented clearly enough to reproduce release artifacts

## Ticket 021 / DUMB-21: Optional Help System Integration

Status: Pending

Goals:
- Add a basic Help menu and help viewer workflow
- Prefer optional runtime binding with graceful failure rather than a hard dependency
- Evaluate the most appropriate Windows help/display path for this project, which may include `mshtml` or another lightweight system-supported approach
- Support a simple indexed help format that is practical to author and ship, with HTML being an acceptable baseline
- Keep the fallback behavior simple if the optional help backend is unavailable

Acceptance:
- The app exposes a Help menu entry
- Help content can be viewed through the chosen optional backend when available
- Missing optional help components fail gracefully without breaking the editor
- Help content format and packaging approach are documented well enough for future updates

## Ticket 022 / DUMB-22: Menu Keybindings And Accelerators

Status: Pending

Goals:
- Provide good keyboard coverage for the bulk of the menu options
- Expose keybindings clearly in menu labels and accelerator handling
- Keep shortcuts predictable and consistent with common Windows editor conventions where practical
- Make sure keybindings still make sense across normal text mode, raw mode, and later specialized modes

Acceptance:
- Most important menu actions have keyboard shortcuts
- Menu text and accelerator handling stay in sync
- Shortcut behavior is documented and testable enough to avoid accidental regressions

## Ticket 023 / DUMB-23: Display And Insert Lower ASCII Control Characters

Status: Pending

Goals:
- Support optional display treatment for lower ASCII control characters such as `BEL`, `BS`, `HT`, `LF`, `VT`, `FF`, `CR`, and related values
- Add a deliberate insertion workflow for those control characters
- Decide mode-aware behavior:
  - normal text mode
  - raw mode
  - large-file and future hex-oriented modes where applicable
- Ensure the user can distinguish between visual representation and underlying stored data

Acceptance:
- The app can insert selected lower ASCII control characters intentionally
- Display behavior for those characters is defined and consistent for the active mode
- The chosen representation does not silently corrupt or mislead about stored content

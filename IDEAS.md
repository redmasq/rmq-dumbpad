# dumbpad ideas

This file tracks possible future tickets and design ideas that are not yet committed to the main roadmap.

## Session Dirty-State Tracking Cleanup

Clarify one source of truth for modified state, reload prompts, save prompts, and external-change warnings.

## Undo/Redo Strategy And Limits

Define whether stock `EDIT` behavior is enough for early phases, and document limits before advanced modes complicate it.

## Central Error Reporting And User Messages

Create a small shared path for status-bar text, message boxes, and recoverable error reporting so failures feel consistent.

## Document Metadata Model

Extract a clean model for encoding, line endings, path, file timestamps, access mode, dirty state, and future mode flags.

## Command Routing Layer

Add a thin command layer between menu and accelerator handlers and editor behavior so commands stop living directly in window procedures.

## Test Harness For Pure Core Modules

Formalize a lightweight host-side test structure for decoding, line splitting, bounds checks, search helpers, and file metadata logic.

## Status Bar And Document State Surface

Expose encoding, line endings, insert or overwrite state, dirty state, and access mode clearly in the UI.

## Search Result Semantics And Wrap Behavior

Define exact behavior for wraparound search, zero-length matches, replace-next, and replace-all edge cases before the search engine grows.

## Line Index / Position Mapping Helpers

Build reusable helpers for offset-to-line, line-to-offset, and column mapping so `Go To`, search, wrap, and selections share one model.

## New File / Untitled Document Workflow

Define untitled buffers, default encoding and line endings, title-bar labeling, and save or save-as transitions.

## Safe Save Workflow

Consider temp-file plus replace semantics, backup-file options, and how save failure should preserve user data.

## Open File Mode Decision UX

When opening suspicious or binary-ish data, define how the app decides between text, raw, large-file, or future hex workflows.

## Resource String Organization And Localization Boundaries

Follow on from `DUMB-24`: define which strings belong in resources, how IDs are grouped, and where optional localization intentionally stops.

## Memory Growth Policy For Large Text Buffers

Document and test allocation growth rules so large edits and big files do not rely on ad hoc `realloc` behavior.

## Crash-Resistant Startup And Shutdown Paths

Harden startup failure, partial initialization, and shutdown cleanup so resource, file, or control failures do not leave inconsistent state.

## Mode Capability Matrix

Write down what normal, raw, large-file, and hex modes are each allowed to do. This helps avoid accidental feature creep and inconsistent UX.

## Clipboard Unicode And Binary Boundary Rules

Define exactly how copy and paste behaves across encodings, embedded `NUL` bytes, control characters, and future raw or hex modes.

## Optional Logging / Debug Diagnostics

Add debug-only tracing or assertions that help development without changing release dependency goals.

## File Path And Long Path Handling

Explicitly handle long paths, unusual path characters, relative versus absolute reopening, and common Win32 path edge cases.

## Help Text / Discoverability For Non-Obvious Modes

Before raw or hex features ship, add lightweight discoverability so users understand what a mode changes and what its limits are.

# ADR-0017: Command Palette Design

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

Astra includes a command palette (Cmd/Ctrl+K) that lets users search and execute commands by typing. The command palette must surface both standard Chrome commands (new tab, reload, zoom, history, etc.) and Astra-specific commands (toggle sidebar, new workspace, toggle split view, add to favorites, etc.).

The architectural question is how the command palette relates to Chrome's existing command system. Options range from building a completely separate command registry to fully integrating with `BrowserCommandController`.

Key requirements:
- Fuzzy search over all commands with display names and descriptions.
- Keyboard shortcut display alongside each command.
- Mixed Chrome + Astra command results in a single unified list.
- Fast filtering as the user types.
- Extensible: new Astra commands should be easy to add.

## Decision

The command palette is a **Views-based UI with its own search model** that indexes both Chrome and Astra command IDs. Execution dispatches through the existing command infrastructure (`BrowserCommandController` for Chrome commands, `AstraCommandDelegate` for Astra commands).

**Command model (AstraCommandPaletteModel):**
- Maintains a flat list of `AstraCommandPaletteItem` entries, each with: `command_id`, `display_name`, `description`, `shortcut`, `is_astra`.
- The list includes both curated Chrome commands (IDC_NEW_TAB, IDC_RELOAD, etc.) and all Astra commands (kAstraCommand* range).
- Supports case-insensitive substring search with relevance scoring (prefix match > name substring > description substring).
- Maximum 20 results per query.
- Is purely a search/display index -- does not execute commands, does not track enabled state.

**Execution flow:**
1. User selects a command from the palette.
2. The palette calls `chrome::ExecuteCommand(browser, command_id)` or equivalent.
3. `BrowserCommandController` handles the command if it is a standard Chrome ID.
4. For Astra-range IDs (60000+), a patch point in `BrowserCommandController` delegates to `AstraCommandDelegate`.
5. The command palette itself does not know or care which system handles execution.

**UI layer (AstraCommandPaletteView / AstraCommandPaletteBubble):**
- A `BubbleDialogDelegateView` subclass anchored to the toolbar or location bar.
- Contains a text field for input and a list of result items.
- Queries `AstraCommandPaletteModel` on each keystroke.
- Queries command enabled state from `BrowserCommandController` + `AstraCommandDelegate` for result dimming.
- Standard Views keyboard navigation (up/down arrows, Enter, Escape).

**Command ID unification:**
- Chrome commands use `IDC_*` constants (0-59999), defined in `chrome/app/chrome_command_ids.h`.
- Astra commands use `kAstraCommand*` constants (60000+), defined in `astra/browser/astra_command_delegate.h`.
- The palette treats both as integers in a single namespace.
- `AstraCommandDelegate::IsAstraCommand(command_id)` distinguishes the two ranges.

## Consequences

Positive:

- Unified UX: one palette searches both Chrome and Astra commands.
- Reuses existing command execution infrastructure -- no dual dispatch logic.
- Search model is independent of execution, making it easy to test and evolve.
- Views-based UI stays in the browser process, no IPC overhead.
- Command IDs are stable identifiers that work across menus, keybindings, and palette.

Negative:

- The initial command list is curated (hardcoded) rather than dynamically enumerated from Chrome's command table. A curated list may miss some Chrome commands.
- Command enabled/disabled state must be queried from two systems (`BrowserCommandController` for Chrome, `AstraCommandDelegate` for Astra).
- Keyboard shortcuts are hardcoded in the palette model rather than read from the accelerator table. A future patch to expose the accelerator table would improve accuracy.
- Adding a new Astra command requires updates in three places: the delegate, the palette model, and the accelerator table.

Neutral:

- The palette does not include WebUI or extension commands by default. These can be added later as additional command sources.

## Alternatives Considered

### WebUI command palette
Build the command palette as a WebUI page with HTML/CSS/JS.

- Rejected: WebUI adds IPC overhead for every keystroke, requires a bridge to browser commands, and cannot easily query all browser commands. A Views palette is faster, simpler, and consistent with Chromium desktop architecture for browser chrome.

### Extending Chrome's command system to include search
Add search capabilities directly to `BrowserCommandController`.

- Rejected: `BrowserCommandController` is designed for dispatch, not search. Adding search logic to it would mix concerns. A separate search model that reads command IDs is cleaner.

### Chrome action system
Use Chromium's newer `ui::Action` system for commands and search.

- Considered: The actions system is evolving but is not yet the primary command dispatch path. We track it and may migrate in the future, but for now we align with `BrowserCommandController` where Chrome already routes.

## References

- **Chromium subsystems reused:** `BrowserCommandController`, `chrome::ExecuteCommand`, `views::BubbleDialogDelegateView`, `views::Textfield`
- **Astra components:** `AstraCommandPaletteModel`, `AstraCommandPaletteView`, `AstraCommandPaletteBubble`, `AstraCommandDelegate`
- **Patch points:** Command forwarding (patch 0003 / `chrome/browser/ui/browser_command_controller.cc`), accelerator table (patch 0007 / `chrome/browser/ui/views/accelerator_table.cc`)

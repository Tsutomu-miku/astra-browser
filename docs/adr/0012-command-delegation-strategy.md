# ADR-0012: Command Delegation Strategy

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

Astra has two categories of commands:

1. **Standard browser commands** -- new tab, new window, close tab, reload,
   back, forward, zoom, find, DevTools, history, downloads, settings,
   extensions, passwords, and so on. These are the hundreds of commands already
   defined in Chrome's `chrome/app/chrome_command_ids.h` and dispatched by
   `BrowserCommandController`.

2. **Astra-specific commands** -- toggle sidebar, new workspace, next/previous
   workspace, open glance, toggle split view, add to favorites, move to
   workspace, command palette, and similar product-specific actions. These have
   no equivalent in Chrome.

The architecture must handle both sets consistently. The command palette,
keybindings, menu system, and programmatic invocation should all route through
the same system.

## Decision

Standard browser commands use Chrome's `BrowserCommandController` unchanged.
Astra-specific commands (ID range 60000+) go through `AstraCommandDelegate`.
A single patch point in `BrowserCommandController` forwards unknown command IDs
to `AstraCommandDelegate`.

**Command ID allocation:**

- Standard Chrome commands: `IDC_*` (ID 0-59999), from
  `chrome/app/chrome_command_ids.h`. Reused as-is.
- Astra commands: `kAstraCommand*` (ID 60000+), from
  `astra/browser/astra_command_delegate.h`. No overlap with Chrome IDs.

**Dispatch flow:**

1. Menu items, keybindings, and the command palette reference command IDs
   regardless of origin.
2. `BrowserCommandController::ExecuteCommand()` is the entry point.
3. If the command ID is a standard Chrome command, Chrome handles it normally.
4. If the command ID falls in the Astra range (60000+), the patch point
   delegates to `AstraCommandDelegate::ExecuteCommand(browser, command_id)`.
5. `AstraCommandDelegate` resolves the command and mutates the appropriate
   Astra service (`AstraWorkspaceService`, `AstraTabFeatures`, UI controllers).

**Key bindings and menus:**

- Standard keybindings are defined by Chrome's accelerator tables.
- Astra keybindings are registered through a small patch to the accelerator
  table or via a `views::FocusManager` handler on `AstraBrowserView`.
- The command palette searches both sets uniformly by command ID.

**Query APIs:**

- `AstraCommandDelegate::IsAstraCommand(command_id)` returns true for the Astra
  ID range, so callers can distinguish the two sets without listing them.

## Consequences

Positive:

- All standard Chrome commands work exactly as in Chrome. No reimplementation,
  no drift.
- Consistent command surface: menus, keybindings, command palette, and
  programmatic callers all use the same ID namespace.
- The patch point is minimal: a single `if` statement in
  `BrowserCommandController` (or a virtual override hook if one already exists).
- Commands are discoverable by ID, enabling a unified command palette.
- Extensions that use the commands API continue to work against standard
  command IDs.

Negative:

- The 60000+ ID range must be maintained across Chromium rebases. If Chrome
  ever adds commands in that range, a collision check and rebase is needed.
  This is low risk; Chrome uses the `IDC_*` enum and adds new entries at the
  end well below this range.
- Command enable/disable state for Astra commands must be managed separately
  from `BrowserCommandController`'s built-in command update mechanism. A
  companion `IsCommandEnabled` check on `AstraCommandDelegate` handles this.

## Alternatives Considered

### Custom Astra command system
Build a separate command registry and dispatcher for all commands, wrapping
Chrome commands as handlers.

- Rejected: Duplicates the entire Chrome command infrastructure. Menu wiring,
  accelerator tables, and command update logic already exist in Chromium and
  are well-tested. Wrapping them adds indirection without benefit.

### Chrome action system
Use `ui/base/models/action.h` / `actions` feature, which Chromium is evolving
toward.

- Considered: The actions system is newer and may become the standard.
  However, `BrowserCommandController` remains the primary dispatch path for
  desktop Chrome. We will track the actions system and migrate if it becomes
  the standard, but for now we go where Chrome already routes.

### WebUI command palette
Build the command palette as a WebUI page with its own command registry.

- Rejected: WebUI command palettes are common but add IPC overhead, can't
  easily reach all browser commands, and create a parallel command model. A
  Views-based command palette that queries both Chrome and Astra command IDs
  stays in the browser process and has direct access to `Browser`.

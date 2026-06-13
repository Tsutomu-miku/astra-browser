# 0003 — Forward Astra command IDs

**Patch ID:** 0003
**File:** `chrome/browser/ui/browser_command_controller.cc`
(and optionally `chrome/browser/ui/browser_command_controller.h`)
**Size estimate:** ~10 lines
**Status:** planned
**Astra component:** `astra/browser/astra_command_delegate.h`

## Context

Chromium's command system is built around `BrowserCommandController` and
`CommandUpdater`. Every browser commands (new tab, back, reload, etc.) are
identified by integer command IDs (see `chrome/app/chrome_command_ids.h`)
and dispatched through a central controller.

Astra defines product-specific commands that have no equivalent in Chrome
(e.g., "toggle sidebar", "switch workspace", "toggle split view").
Rather than building a parallel command system, Astra extends Chrome's
`CommandUpdater` / `BrowserCommandController` infrastructure with
additional command IDs in the 60000+ range.

This patch teaches the existing command controller to forward unknown
Astra-range command IDs to `AstraCommandDelegate`.

This cannot be done from `//astra` alone because the command dispatch
happens inside `BrowserCommandController`, which is a Chromium-owned class.
A small patch adds a range check and forward.

## Change

### Before

```cpp
// Include at the top of the file.
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/app/chrome_command_ids.h"
// ... other includes ...
```

```cpp
// Inside BrowserCommandController::ExecuteCommandWithDisposition()
// or CommandUpdater::ExecuteCommand().
void BrowserCommandController::ExecuteCommandWithDisposition(
    int command_id,
    WindowOpenDisposition disposition) {
  switch (command_id) {
    case IDC_NEW_TAB:
      // ... handle new tab ...
      break;
    // ... many more Chrome commands ...
    default:
      NOTREACHED() << "Unknown command: " << command_id;
      break;
  }
}
```

(The exact structure depends on the Chromium revision — it may use a
`CommandUpdater` class with a command map, or a switch statement in
`BrowserCommandController`.)

### After

Include at the top of the file, guarded by the build flag:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
#include "astra/browser/astra_command_delegate.h"
#include "astra/browser/astra_command_ids.h"
#endif
```

Inside the command execution method, after Chrome's built-in command
handling but before the default/unknown case:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Forward Astra-range command IDs to AstraCommandDelegate.
  // Astra commands (60000+) are product-specific commands that
  // have no equivalent in Chrome's built-in command set.
  if (command_id >= astra::kAstraCommandFirst &&
      command_id <= astra::kAstraCommandLast) {
    astra::AstraCommandDelegate::GetForBrowser(browser_)->ExecuteCommand(
        command_id, disposition);
    return;
  }
#endif
```

For command registration (enabling/disabling commands), add a similar
guard in the `CommandUpdater` update function:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Let AstraCommandDelegate manages enabled/disabled state for
  // Astra-range command IDs.
  if (command_id >= astra::kAstraCommandFirst &&
      command_id <= astra::kAstraCommandLast) {
    astra::AstraCommandDelegate::GetForBrowser(browser_)->UpdateCommand(
        command_id, this);
    return;
  }
#endif
```

## Rationale

**Why extend Chrome's command system?**
- It is the standard command dispatch mechanism in Chromium.
- All UI surfaces (menus, keyboard shortcuts, context menus) all
  use the same command IDs.
- Extending the existing system means Astra commands work everywhere
  automatically — no need to build separate dispatch paths.

**Why a range check instead of registering each command?**
- The command IDs are allocated in a contiguous range (60000-60999).
- A single range check is simpler and faster than registering
  individual commands one by one.
- Adding a new Astra command only requires adding it to
  `astra_command_ids.h` and `AstraCommandDelegate` — no
  Chromium patch needed.

**Why AstraCommandDelegate is a BrowserUserData?**
- Yes, recommended pattern: `AstraCommandDelegate` is attached to each
  `Browser` via `BrowserUserData`, accessible via
  `GetForBrowser(browser_)`.
- This is the same pattern used by other Browser-scoped helpers in Chrome.
- Each browser window gets its own delegate instance.

**What `//astra` code does this delegate to?**
- `astra/browser/astra_command_delegate.h` — executes Astra commands.
- `astra/browser/astra_command_ids.h` — defines Astra command ID constants.
- `astra/app/astra_accelerator_table.h` — maps shortcuts to command IDs.

## Build Flag

- **Gate:** `BUILDFLAG(IS_ASTRA_BRANDED)**
- **Build flag defined in:** `astra/build/astra_buildflags.h`
- **Default:** off (Astra commands not compiled in)
- All Astra includes and code are behind this flag.

## Accelerator integration — end-to-end flow

Astra commands are reachable via keyboard shortcuts through Chrome's
accelerator system. The full flow:

1. **Key press** — User presses a key combination (e.g. `Ctrl/Cmd+\`).

2. **FocusManager matches accelerator** — `views::FocusManager` looks up
   the key in Chrome's accelerator table (which now includes Astra
   entries via patch 0007).

3. **AcceleratorTarget dispatches** — `BrowserView` (which implements
   `views::AcceleratorTarget`) receives the accelerator and calls
   `browser_command_controller_->ExecuteCommand(command_id)`.

4. **Command forwarding** — `BrowserCommandController::ExecuteCommand()`
   checks if the command ID is in the Astra range (60000+) via this
   patch (0003). If so, it forwards to `AstraCommandDelegate`.

5. **Astra handles the command** — `AstraCommandDelegate` routes the
   command to the appropriate Astra service or UI observer.

**Why two patches?**
The accelerator table patch (0007) only adds key-to-command mappings.
The command forwarding patch (0003) adds dispatch logic for Astra-range
command IDs. Together they enable keyboard-driven Astra features, but they
are separable:
- Without 0007: Astra commands still work via menus and command palette.
- Without 0003: Astra accelerators are registered but do nothing.

## Alternatives Considered

1. **Build a parallel command system** — Create our own command
   infrastructure in `//astra`. Rejected: would need to integrate with menus,
   shortcuts, command palette separately. Much more work and less
   consistent with Chrome.

2. **Register each Astra commands individually** — Add each Astra command
   with `CommandUpdater` at runtime. Possible but more complex
   and requires more patch surface area. A single range check is simpler.

3. **Use extension commands API** — Treat Astra features as extensions.
   Rejected: Astra is not an extension; it's a product layer.

4. **Override in a custom command controller** — Subclass or replace
   BrowserCommandController. Rejected: too invasive, high rebase cost.

## Risks & Rebase Concerns

- **Low-to-medium risk.** The command controller code is relatively
  stable, but the exact structure (switch statement vs. command map)
  can change between Chromium versions.
- **Mitigation:** The patch concept is simple — check if a command
  is in the Astra range and forward it. Even if the surrounding code
  changes, the patch is easy to adapt.
- **Command ID range collision:** If Chrome ever adds commands in the
  60000+ range, we'd have a conflict. Mitigation: The range is
  well above Chrome's current max (~40000), and we can adjust if
  needed (see patch 0008 for range reservation).
- **Graceful degradation:** If the patch fails to apply, Astra
  commands won't execute. The browser still works — only Astra
  features are unavailable. No crash, no data loss.

## Related

- ADR: `docs/adr/0012-command-delegation-strategy.md`
- Related patches: 0007 (accelerator table), 0008 (command ID range)
- Astra source: `astra/browser/astra_command_delegate.h`
- Astra source: `astra/browser/astra_command_ids.h`
- Astra source: `astra/app/astra_accelerator_table.h`

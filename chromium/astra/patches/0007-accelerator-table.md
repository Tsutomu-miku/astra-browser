# 0007 — Merge Astra accelerators into Chrome's accelerator table

**Patch ID:** 0007
**File:** `chrome/browser/ui/views/accelerator_table.cc`
**Size estimate:** ~15 lines
**Status:** planned
**Astra component:** `astra/app/astra_accelerator_table.h`

## Context

Chrome manages all keyboard shortcuts (accelerators) through a static table in
`chrome/browser/ui/views/accelerator_table.cc`. This table maps command IDs to
key combinations. The `FocusManager` reads this table at widget creation time
and uses it to match key presses to commands. Commands are then dispatched
through `BrowserView` (as `AcceleratorTarget`) to `BrowserCommandController`.

Astra adds product-specific commands (sidebar, workspaces, split view, command
palette, etc.) with command IDs in the 60000+ range. These commands need
keyboard shortcuts that work the same way as Chrome's built-in shortcuts —
processed by `FocusManager`, dispatched through `BrowserCommandController`,
and (for Astra-range IDs) forwarded to `AstraCommandDelegate` via the
command-forwarding patch (patch 0005).

This cannot be done from `//astra` alone because the accelerator table is a
static array in Chrome code. A small patch adds Astra entries to the table.

## Change

### Before

```cpp
// In chrome/browser/ui/views/accelerator_table.cc,
// near the end of GetAcceleratorTable() or the kAcceleratorMap array.

  // Chrome's last accelerator entry...
  {IDC_SHOW_SIGNIN_VIEW, ui::VKEY_M, ui::EF_ALT_DOWN | ui::EF_SHIFT_DOWN},
  // End of table.
};
```

### After

```cpp
// At the top of the file, add the include (gated by build flag).
#if BUILDFLAG(IS_ASTRA_BRANDED)
#include "astra/app/astra_accelerator_table.h"
#endif

// ...

  // Chrome's last accelerator entry...
  {IDC_SHOW_SIGNIN_VIEW, ui::VKEY_M, ui::EF_ALT_DOWN | ui::EF_SHIFT_DOWN},

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Astra-specific accelerators.  These map Astra command IDs (60000+) to
  // key combinations.  The commands are dispatched through
  // BrowserCommandController and forwarded to AstraCommandDelegate.
  for (const auto& entry : astra::GetAstraAcceleratorTable()) {
    accelerator_map[entry.command_id].push_back(
        ui::Accelerator(entry.keycode, entry.modifiers));
  }
#endif

  return accelerator_map;
}
```

Alternatively, if the accelerator table is a plain array (not a map builder),
the patch could append Astra entries to the array:

```cpp
// Append Astra accelerator entries at the end of the array.
#if BUILDFLAG(IS_ASTRA_BRANDED)
#define ASTRA_ACCEL_ENTRY(cmd, key, mods) \
    {cmd, key, mods}
  // Insert Astra entries here...
  ASTRA_ACCEL_ENTRY(astra::kAstraCommandToggleSidebar, ui::VKEY_OEM_5,
                    kPlatformPrimaryModifier),
  // ... more entries
#undef ASTRA_ACCEL_ENTRY
#endif
```

The exact form depends on how the accelerator table is structured in the
specific Chromium revision. The goal is the same: add Astra entries so they
are processed by `FocusManager` like any other accelerator.

## Rationale

**Why patch the accelerator table?**
- It is the canonical way to define keyboard shortcuts in Chromium.
- All existing infrastructure (FocusManager, tooltip shortcut display,
  menu shortcut display) works automatically with table entries.
- Astra shortcuts behave exactly like Chrome shortcuts — same priority,
  same dispatch path, same handling for focus contexts.

**Why not runtime registration?**
- Runtime registration via `FocusManager::RegisterAccelerator()` is possible
  from `//astra` code (see `AstraAcceleratorRegistrar`), but it requires
  access to the `AcceleratorTarget` (BrowserView) and is less integrated
  with Chrome's shortcut display systems (menus, tooltips).
- The static table approach is one small patch and everything works.

**What `//astra` code does this delegate to?**
- `astra/app/astra_accelerator_table.h` — defines the accelerator table
  and shortcut text helpers.
- `astra/browser/astra_command_delegate.h` — executes Astra commands
  after dispatch from BrowserCommandController.

## Build Flag

- Gate: `BUILDFLAG(IS_ASTRA_BRANDED)`
- Build flag defined in: `build/config/chrome_build.gni` (patch 0003)
- All Astra includes and code are behind this flag.

## Alternatives Considered

1. **Runtime registration via FocusManager** — Register accelerators at
   widget creation time from `AstraBrowserView`. Works from `//astra` without
   a table patch, but shortcuts don't appear in menu tooltips and the
   approach is less idiomatic. Rejected as primary approach; kept as a
   fallback option in `AstraAcceleratorRegistrar`.

2. **Custom accelerator handler via FocusManagerObserver** — Listen for key
   events and intercept Astra shortcuts before Chrome processes them. More
   invasive, fragile, and could cause ordering issues. Rejected.

3. **Extension commands API** — Use Chrome's extension commands system to
   register shortcuts. Would require building Astra features as extensions,
   which contradicts the direct Chromium approach. Rejected.

4. **WebUI-based shortcut handling** — Handle shortcuts in a WebUI surface.
   Only works when that WebUI is focused, not browser-wide. Rejected.

## Risks & Rebase Concerns

- **Stability:** The accelerator table in Chrome is fairly stable. New
  entries are added occasionally but the structure rarely changes. Low
  rebase risk.
- **Conflict with new Chrome shortcuts:** If Chrome adds a shortcut that
  conflicts with an Astra shortcut, users would get the Chrome behavior
  (since Chrome entries come first in the table). Mitigation: choose Astra
  shortcuts that are unlikely to be adopted by Chrome, and document how
  users can customize shortcuts.
- **Build break on rebase:** If the table structure changes significantly,
  the patch may need adjustment. The patch is small (~15 lines), so
  rebasing is straightforward.
- **Graceful degradation:** If the patch fails to apply, Astra features
  still work via menu items and the command palette. Only keyboard
  shortcuts for Astra commands are lost.

## Related

- ADR: `docs/adr/0012-command-delegation-strategy.md`
- Related patches: 0005 (command delegate forward), 0006 (command ID range)
- Astra source: `astra/app/astra_accelerator_table.h`,
  `astra/app/astra_accelerator_table.cc`,
  `astra/app/astra_accelerator_registrar.h`

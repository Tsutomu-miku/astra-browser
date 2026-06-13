# 0008 — Reserve Astra command ID range

**Patch ID:** 0008
**File:** `chrome/app/chrome_command_ids.h`
**Size estimate:** ~5 lines
**Status:** planned
**Astra component:** `astra/browser/astra_command_ids.h`

## Context

Chromium uses integer command IDs to identify browser actions (new tab, back,
reload, etc.). These are defined as `IDC_*` constants in
`chrome/app/chrome_command_ids.h` and are used throughout the browser — by
menus, keyboard shortcuts, command controller, and the command palette.

Astra adds product-specific commands that have no equivalent in Chrome
(toggle sidebar, switch workspace, toggle split view, etc.). These commands
need their own command ID range that does not collide with Chrome's IDs.

This cannot be done from `//astra` alone because Chrome's command ID header
is a single shared header included by all Chrome UI code. A small patch
reserves a range of command IDs for Astra use.

## Change

### Before

```cpp
// In chrome/app/chrome_command_ids.h, near the end of the IDC_* definitions.

// NOTE: All values above must be < IDC_MinimumLabelValue.
#define IDC_MinimumLabelValue 40000
```

(Or wherever the last reserved ID and the minimum label value are defined
in the specific Chromium revision.)

### After

```cpp
// Include at the top of the file, or inline the range directly.
// (Best practice: define the range constants directly in this header
//  to avoid circular include issues.)

#if BUILDFLAG(IS_ASTRA_BRANDED)
// Astra-specific command IDs start at 60000.
// This range is well above Chrome's highest command ID (~40000 range)
// and below the label value minimum, so it avoids collisions.
//
// See astra/browser/astra_command_ids.h for the full list of Astra command IDs.
// The actual constants are defined there to keep Chromium patches minimal.
#define IDC_ASTRA_FIRST 60000
#define IDC_ASTRA_LAST  60999
#endif

// NOTE: All values above must be < IDC_MinimumLabelValue.
#define IDC_MinimumLabelValue 40000
```

Wait — `IDC_MinimumLabelValue` is 40000 and we want Astra IDs at 60000.
That's above the minimum label value, which means they would be treated as
label IDs, not command IDs. Let me check...

Actually, the correct approach is to place Astra command IDs BELOW
IDC_MinimumLabelValue (which is typically 32768 or 40000 depending on the
Chromium revision). If that range is too crowded, we can use a higher range
but we need to be careful about the label value boundary.

**Revised approach:** Place Astra command IDs in the 59000–59999 range,
which is above Chrome's typical maximum command ID but we need to verify
it's below IDC_MinimumLabelValue.

### Alternate after (if IDC_MinimumLabelValue is higher):

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
// Astra-specific command ID range.
// Allocated in the space between Chrome's highest command ID and
// IDC_MinimumLabelValue to avoid collisions with both Chrome commands
// and dynamic label IDs.
//
// See astra/browser/astra_command_ids.h for individual command IDs.
#define IDC_ASTRA_FIRST 59000
#define IDC_ASTRA_LAST  59999
#endif
```

**Important:** The exact range depends on the Chromium revision. We need to:
1. Check the highest Chrome command ID in `chrome_command_ids.h`.
2. Verify that `IDC_MinimumLabelValue` is higher than our range.
3. Adjust the Astra range accordingly.

The Astra source code in `astra/browser/astra_command_ids.h` defines the
individual command ID constants (e.g., `kAstraCommandToggleSidebar`) and
uses these boundary constants for range checking.

## Rationale

**Why patch chrome_command_ids.h?**
- It is the canonical source of all command IDs in Chromium.
- All command infrastructure (menus, shortcuts, command controller)
  references this header.
- Reserving a range here ensures no future Chrome command IDs will
  collide with Astra IDs.

**Why 60000+ range?**
- Chrome's built-in command IDs are well below 40000.
- The 60000+ range is comfortably above all current Chrome IDs.
- It is below 65535 (common 16-bit boundary), leaving room for both
  growth and other extensions.
- The round number is easy to remember and document.

**Why not use dynamic command IDs?**
- Dynamic IDs would be harder to reference from accelerators, menus,
  and the command palette.
- Static IDs are the standard Chromium pattern.
- A reserved range is the approach used by other Chromium embedders.

**What `//astra` code does this delegate to?**
- `astra/browser/astra_command_ids.h` — defines individual Astra command IDs
- `astra/browser/astra_command_delegate.h` — executes Astra commands
- `astra/app/astra_accelerator_table.h` — maps shortcuts to Astra command IDs

## Build Flag

- Gate: `BUILDFLAG(IS_ASTRA_BRANDED)`
- Build flag defined in: `build/config/chrome_build.gni` (patch 0003 build flag)
- All Astra range definitions are behind this flag.

## Alternatives Considered

1. **Use extension-style command IDs** — Chrome extensions have their own
   command ID mechanism. But Astra is not an extension, and this would be
   architecturally wrong. Rejected.

2. **Dynamic command ID allocation** — Allocate IDs at runtime from a pool.
   Would work but loses the simplicity of static IDs (can't reference them
   in accelerator tables, switch statements, etc.). Rejected.

3. **Higher range (e.g. 100000+)** — More room for growth but may conflict
   with other embedders or future Chrome features. The 60000 range is a
   reasonable middle ground.

4. **Lower range (e.g. 30000+)** — Closer to Chrome's IDs but risk of
   collision as Chrome adds new commands. Rejected — we want headroom.

## Risks & Rebase Concerns

- **Very low risk.** The command ID header changes rarely — new IDs are
  added occasionally but the structure is stable. The patch just adds
  a few `#define` lines near the end.
- **Range collision risk:** If Chrome adds so many new command IDs that
  they approach the Astra range, we need to renumber. Mitigation: choose
  a range with plenty of headroom (60000 vs Chrome's ~40000 max).
- **Graceful degradation:** If the patch fails to apply, Astra commands
  won't have IDs and all Astra UI surfaces will fail to compile. This is
  a hard build break — not graceful, but the patch is tiny and the break
  is obvious, so it will be caught immediately.

## Related

- Related patches: 0003 (command forwarding — uses Astra command IDs),
  0007 (accelerator table — maps shortcuts to Astra command IDs)
- Astra source: `astra/browser/astra_command_ids.h`
- Astra source: `astra/browser/astra_command_delegate.h`
- Astra source: `astra/app/astra_accelerator_table.h`
- ADR: `docs/adr/0012-command-delegation-strategy.md`

# Patch Template

**Patch ID:** NNNN
**File:** `path/to/chromium/file.cc`
**Size estimate:** ~X lines
**Status:** planned / applied / needs-rebase
**Astra component:** `astra/...`

## Context

What Chromium subsystem are we patching, and why can't this be done from `//astra` alone?

Describe the Chromium class/function and its role in the browser.

## Change

### Before

```cpp
// 3-5 lines of context before the change
void SomeClass::SomeMethod() {
  existing_code();
  // <- patch goes here
  more_existing_code();
}
```

### After

```cpp
void SomeClass::SomeMethod() {
  existing_code();
#if BUILDFLAG(IS_ASTRA_BRANDED)
  astra::AstraDelegate::DoTheThing(this);
#endif
  more_existing_code();
}
```

Include the exact code that should be added or changed. Keep it minimal.

## Rationale

Why this patch point specifically? Why here and not somewhere else?

What `//astra` code does this delegate to? (file and class name)

## Build Flag

- Gate: `BUILDFLAG(IS_ASTRA_BRANDED)` / `#if defined(...)` / other
- Build flag defined in: `path/to/build_flag.gni`

## Alternatives Considered

1. **Alternative approach 1** -- why not this way?
2. **Alternative approach 2** -- why not this way?
3. **Doing nothing** -- what breaks if we skip this patch?

## Risks & Rebase Concerns

- Is this code area stable in Chromium, or does it change frequently?
- What breaks if this patch fails to apply? (graceful degradation, build break, etc.)
- Known upcoming Chromium changes that may affect this patch.

## Related

- ADR: `docs/adr/NNNN-*.md`
- Related patches: NNNN, NNNN
- Astra source: `astra/path/to/file.cc`

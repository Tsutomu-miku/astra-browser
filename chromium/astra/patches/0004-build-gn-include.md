# 0004 — Include //astra in the build graph

**Patch ID:** 0004
**File:** `chrome/browser/BUILD.gn`
(and `chrome/browser/ui/views/BUILD.gn`, `build/config/chrome_build.gni`)
**Size estimate:** ~10 lines total across 3 files
**Status:** planned
**Astra component:** `astra/BUILD.gn` (root), `astra/build/buildflags.gni`

## Context

`//astra` is a top-level source tree that sits alongside `//chrome`,
`//content`, `//components`, etc. in the Chromium source tree. It needs
to be pulled into the build dependency graph so that `gn gen` compiles
Astra code and Chromium source files can include Astra headers.

Without this patch, `gn gen` will never compile Astra code, even if
Chromium source files include Astra headers (those includes would fail
because the headers are not in the include path without the dep).

This cannot be done from `//astra` alone — a high-level Chromium target
must depend on the Astra targets to pull them into the build graph.

## Change

### 1. chrome/browser/BUILD.gn — main browser target

Add the Astra dependency to the main browser target. This is where
`AstraBrowserMainExtraParts` and `AstraCommandDelegate` are referenced
from Chromium code.

#### Before

```gn
source_set("browser") {
  # ... sources ...

  deps = [
    ":browser_ui",
    "//base",
    "//chrome/common",
    # ... many more deps ...
  ]
}
```

#### After

```gn
source_set("browser") {
  # ... sources ...

  deps = [
    ":browser_ui",
    "//base",
    "//chrome/common",
    # ... many more deps ...
  ]

  # Astra product-layer code.
  if (is_astra_branded) {
    deps += [ "//astra:astra_browser" ]
  }
}
```

### 2. chrome/browser/ui/views/BUILD.gn — views target

Add the Astra views dependency so that `BrowserView` can include
`AstraBrowserView`.

#### Before

```gn
source_set("views") {
  # ... sources ...

  deps = [
    # ... deps ...
  ]
}
```

#### After

```gn
source_set("views") {
  # ... sources ...

  deps = [
    # ... deps ...
  ]

  if (is_astra_branded) {
    deps += [ "//astra/ui/views" ]
  }
}
```

### 3. build/config/chrome_build.gni — GN arg declaration

Declare the `is_astra_branded` build flag so it can be set via GN args.

#### Before

```gn
declare_args() {
  is_chrome_branded = false
  is_chromecast = false
  # ... other args ...
}
```

#### After

```gn
declare_args() {
  # When true, builds the Astra product layer on top of Chromium.
  # This enables Astra-specific UI surfaces, branding, and services.
  # Set via `gn gen --args="is_astra_branded=true"`.
  is_astra_branded = false

  is_chrome_branded = false
  is_chromecast = false
  # ... other args ...
}
```

Note: This build flag is also referenced in patch 0005 (branding).
It is listed here because it is part of the build system integration.

## Rationale

**Why add deps to chrome/browser?**
- It is the highest-level browser target that links into the final
  `chrome` binary.
- Most Chromium patch points for Astra are in `chrome/browser/`.
- Adding the dep here ensures Astra code is compiled and linked.

**Why use `is_astra_branded` as a GN arg?**
- Follows the same pattern as `is_chrome_branded`.
- Can be set from the command line: `gn gen --args="is_astra_branded=true"`.
- Defaults to false, so plain Chromium builds are unaffected.
- The `is_astra_branded` variable controls whether `//astra` targets
  are compiled and whether `BUILDFLAG(IS_ASTRA_BRANDED)` is true.

**Why visibility restrictions on Astra targets?**
- `//astra` targets use `visibility` to restrict who can depend on them.
- Only `//chrome/*` and `//astra/*` should have direct access.
- This prevents accidental dependencies from other Chromium layers.

**What about `//components` layer dependencies?**
- Patches that touch `//components` (like the omnibox provider, patch 0011)
  will need their own dep additions in the appropriate component BUILD.gn.
- Those are documented in their respective patch files.

## Build Flag

- **Gate:** `is_astra_branded` GN arg (compile-time)
- **Defined in:** `build/config/chrome_build.gni` (patched)
- **Default:** `false` (plain Chromium build)
- When false, `//astra` is not compiled and all `BUILDFLAG(IS_ASTRA_BRANDED)`
  guarded code is compiled out.

## Alternatives Considered

1. **Add to chrome/BUILD.gn instead of chrome/browser/BUILD.gn**
   — Higher-level target, but the pattern in Chromium is to add
   deps at the layer where they are used. Since most Astra code
   is used from chrome/browser, adding it there is more idiomatic.

2. **Use a .gni import instead of a direct dep**
   — Could import `//astra/astra.gni` from chrome/browser/BUILD.gn.
   More flexible but adds indirection. A direct dep is simpler.

3. **Create an overlay instead of patches**
   — Use GN's `import_dir` or overlay mechanism to add Astra code
   without patching Chromium BUILD.gn files. Considered, but GN
   doesn't have a standard overlay mechanism, and patching is the
   established pattern for Chromium forks.

4. **Build Astra as a separate shared library**
   — Compile Astra as a .so/.dylib/.dll and load it at runtime.
   Rejected: More complex build setup, performance overhead, and
   loses static linking optimizations.

## Risks & Rebase Concerns

- **Very low risk.** BUILD.gn files change frequently, but adding a
  dep is trivial to rebase. The patch is ~3 lines per file.
- **Dep order:** Astra deps should be added at the end of the deps list
  or in a logical location. Exact position doesn't matter for correctness
  but should be consistent.
- **GN arg ordering:** The `is_astra_branded` arg should be declared
  near other branding/build args for consistency.
- **Graceful degradation:** If the patch fails to apply, `gn gen`
  will succeed but Astra code won't be compiled. This is a build-time
  issue, not a runtime issue — it will be caught immediately.

## Related

- Related patches: 0005 (branding — also adds build flag config),
  0001 (browser main extra parts — uses //astra/app dep)
- Astra source: `astra/BUILD.gn` (root aggregate target)
- Astra source: `astra/build/buildflags.gni`
- Astra source: `astra/build/astra_buildflags.h`

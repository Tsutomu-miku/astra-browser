# Chromium Patch Points

Keep the patch queue small. Astra code should live in `//astra`; Chromium files
should only register or delegate to it.

Expected first patches:

- Register `astra::AstraBrowserMainExtraParts` from Chrome browser main parts.
- Add an Astra branding/build flag.
- Install `astra::AstraBrowserView` after `BrowserView` construction.
- Forward Astra-only command ids to `astra::AstraCommandDelegate`.

Do not patch Chromium services to duplicate downloads, permissions, history,
passwords, extensions, or DevTools. Prefer existing Chrome services and prefs.

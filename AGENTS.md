# Agent Instructions

This repository is now a direct Chromium refactor. Follow these rules before
writing code.

## Non-Negotiable Direction

- Do not build new architecture on Electron.
- Do not build on CEF.
- Do not add a CMake browser target.
- Do not create an external native shell around Chromium.
- Use Chromium's `chrome`, `content`, `components`, and `ui/views` framework.

## Active Work Area

Use `chromium/astra/` for new direct Chromium code.

Allowed active directories:

- `chromium/astra/app`: startup hooks and Chromium patch helpers.
- `chromium/astra/browser`: Astra-only product metadata/services.
- `chromium/astra/ui/views`: Views UI additions.
- `chromium/astra/patches`: patch queue notes.
- `docs`: active direct Chromium docs and ADRs.
- `scripts`: bootstrap/build/check automation.

Legacy `src/` is migration reference only. Do not add new architecture there.

## Reuse Chromium

Never reimplement these in Astra:

- Profile management.
- WebContents ownership.
- TabStripModel.
- NavigationController.
- History.
- Downloads.
- Permissions.
- Password Manager.
- Autofill.
- Safe Browsing.
- Extensions.
- DevTools.
- WebUI.
- Update.
- Policy.

If a task touches one of these, first identify the Chromium component and patch
point. Astra code should adapt or project Chromium state, not replace it.

## Astra May Own

- Spaces/workspace metadata.
- Sidebar projection and classification.
- Favorite folder membership as tab metadata.
- Split/Glance presentation metadata.
- Astra-only commands.
- Astra-specific Views surfaces.

## Code Shape Rules

- Prefix classes with `Astra`.
- Prefix source files with `astra_`.
- Use Chromium C++ style and ownership patterns.
- Use `ProfileKeyedService` for profile-scoped metadata.
- Use `content::WebContentsUserData` for tab-scoped metadata.
- UI code must not be a product state truth source.
- Chromium patches must be tiny and delegate into `//astra`.
- TODOs must use `TODO(astra):` and name the Chromium owner or patch point.

## Before Handoff

Run:

```bash
pnpm check:architecture
git diff --check
```

If Chromium is checked out locally, also run the relevant GN/Ninja build or test.

Summaries must mention:

- Chromium subsystem reused.
- Astra metadata/UI changed.
- Patch point used.
- Checks run.

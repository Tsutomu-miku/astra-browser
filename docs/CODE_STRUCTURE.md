# Code Structure

## Active Directories

```text
chromium/
  README.md
    Explains how the overlay maps into a Chromium checkout.

  astra/
    BUILD.gn
      Single entry for Astra direct Chromium build targets.

    app/
      Startup and patch helpers:
      astra_browser_main_extra_parts.*
      astra_content_browser_client.*
      astra_main_delegate.*

    browser/
      Product semantics that Chromium does not own:
      astra_workspace_service.*
      astra_tab_features.*
      astra_command_delegate.*

    ui/views/
      Chromium Views UI additions:
      astra_browser_view.*
      sidebar/astra_sidebar_view.*

    patches/
      Patch-point notes. Prefer small text notes here over sprawling source
      forks.

docs/
  Active direct Chromium docs, ADRs, standards, and roadmap.

docs/legacy/electron-prototype/
  Historical prototype docs only.

scripts/
  chromium-bootstrap.sh
  build-chromium.sh
  check-architecture.mjs

src/
  Legacy Electron prototype. No new direct Chromium architecture here.
```

## Placement Rules

- Put Chromium startup hooks in `chromium/astra/app`.
- Put Profile-keyed product metadata in `chromium/astra/browser`.
- Put per-tab product metadata on `content::WebContentsUserData`.
- Put UI widgets in `chromium/astra/ui/views`.
- Put Chromium source patch notes in `chromium/astra/patches`.
- Put durable decisions in `docs/adr`.
- Put agent-facing constraints in `AGENTS.md` and
  `docs/ENGINEERING_STANDARDS.md`.

## Naming Rules

- Prefix Astra C++ classes with `Astra`.
- Prefix files with `astra_`.
- Use Chromium naming and ownership conventions in C++.
- Use Chromium TODO style: `TODO(astra): ...`.
- Never use TODOs as vague placeholders. Name the Chromium component or patch
  point that should own the future work.

## File Size Rules

- Keep Astra overlay files small enough to review in one sitting.
- Split by Chromium ownership boundary, not by arbitrary helper extraction.
- If a file starts mirroring a Chromium subsystem, stop and route to the
  Chromium subsystem instead.

## Review Checklist

Before merging a change, answer:

- Does this reuse Chromium's existing service instead of replacing it?
- Is the state truth source in Chromium or in an Astra product metadata object?
- Is UI code only rendering or delegating commands?
- Is the Chromium patch point minimal?
- Can another agent identify the owner directory from the file path alone?

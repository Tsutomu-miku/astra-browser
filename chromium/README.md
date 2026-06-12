# Astra Direct Chromium Overlay

This directory contains the Astra overlay intended to be copied into a Chromium
checkout as `chromium/src/astra`.

It is not a CEF project and it is not built by CMake. The target build system is
Chromium GN/Ninja.

```bash
./scripts/chromium-bootstrap.sh
./scripts/build-chromium.sh Debug
```

Current status: architecture skeleton only. The Chromium patch points listed in
`docs/CHROMIUM_DIRECT_REFACTOR_PLAN.md` still need to be applied by follow-up
agents inside the Chromium checkout.

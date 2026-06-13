# ADR-0033: Agent-Based Development Workflow

- Status: Accepted
- Date: 2026-06-13
- Deciders: Architecture

## Context

The Astra browser project is developed using AI agents as the primary
coding force. Multiple agents work in parallel on different parts of
the codebase. To maintain architectural consistency and prevent drift,
we need a structured approach to agent-based development.

Key challenges:

- **Parallelism.** Multiple agents work simultaneously on different
  directories. Without clear boundaries, agents may step on each other.
- **Consistency.** All agents must follow the same architectural rules
  and coding conventions. Without guardrails, each agent might make
  different architectural decisions.
- **Ownership.** Each part of the codebase needs a clear owner so that
  agents know who to coordinate with and who is responsible for quality.
- **Review.** Changes need to be checked against architecture rules
  before they are merged.

This ADR documents the directory-ownership agent model used for parallel
development on the Astra project.

## Decision

Astra uses a **directory-ownership agent model** for parallel development.
Each directory or architectural layer has a designated agent (or agent
role) that owns it. Agents work within their owned directories and follow
shared architecture guardrails.

### Directory Ownership Model

The codebase is divided into ownership domains, each with a primary owner:

| Domain | Directory | Owner Role | Responsibility |
|--------|-----------|------------|----------------|
| Architecture docs | `docs/adr/`, `docs/ARCHITECTURE.md` | Architecture Docs Agent | ADRs, architecture overview, design docs |
| App layer | `chromium/astra/app/` | App Layer Agent | Startup hooks, browser main extra parts, content browser client, accelerator registration |
| Browser layer | `chromium/astra/browser/` | Browser Layer Agent | ProfileKeyedServices, WebContentsUserData, command delegate, helper services |
| Common layer | `chromium/astra/common/` | Common Layer Agent | Shared types, enums, constants, lightweight utilities |
| UI Views | `chromium/astra/ui/views/` | UI Views Agent | Sidebar, split view, glance, command palette, workspace overview, all Views surfaces |
| Color system | `chromium/astra/ui/color/` | Color System Agent | Color IDs, color mixer, accent color derivation, theme integration |
| DevTools | `chromium/astra/ui/views/devtools/` | DevTools Agent | DevTools toolbar, workspace panel, integration coordinator |
| Accessibility | `chromium/astra/ui/accessibility/` | Accessibility Agent | AX utilities, keyboard navigation, focus management |
| Patches | `chromium/astra/patches/` | Patch Strategy Agent | Patch queue documentation, patch review, rebase strategy |
| Build config | `chromium/astra/build/`, `chromium/astra/BUILD.gn` | Build Agent | GN build targets, buildflags, dependency graph |
| Scripts | `scripts/` | DevOps / Build Agent | Bootstrap scripts, build scripts, architecture checks |
| Legacy | `src/`, `docs/legacy/` | (read-only) | Migration reference only, no new development |

### Agent Operating Rules

Every agent must follow these rules:

1. **Stay in your lane.** An agent primarily edits files within its
   owned directory. Cross-domain changes require coordination with the
   owning agent.

2. **Follow the architecture.** All agents must adhere to the decisions
   in `AGENTS.md`, `docs/ENGINEERING_STANDARDS.md`, and accepted ADRs.
   If a change would violate an ADR, the agent must propose a new ADR
   or ADR amendment instead of proceeding.

3. **Reuse Chromium.** Agents must never reimplement what Chromium
   already provides. Before building something new, check if a Chromium
   subsystem already exists for it.

4. **Patch lightly.** Chromium patches must be minimal, build-flag
   gated, and delegate to `//astra` immediately. No product logic in
   patched files.

5. **Use the naming conventions.** `Astra` class prefix, `astra_` file
   prefix, `TODO(astra):` with Chromium owner / patch point.

6. **No truth in UI.** UI-layer agents must never introduce state that
   should live in the browser layer. UI projects state from services,
   it does not own it.

7. **Write tests.** Each new feature or component should have
   corresponding unit tests (or browser tests where appropriate).

8. **Document patch points.** Any change that requires a Chromium patch
   must include a patch detail file in `chromium/astra/patches/`.

### Coordination Mechanisms

Agents coordinate through:

- **ADR process.** Architecture-level decisions are made through ADRs,
  which all agents read and follow. ADRs are the source of truth for
  architectural decisions.
- **Common layer.** Types and constants shared across layers go in
  `astra/common/`. Changes to common types affect all layers and need
  broader review.
- **Patch queue.** Chromium patch points are documented in
  `chromium/astra/patches/` and reviewed by the patch strategy agent.
  All agents propose patches through this channel.
- **Architecture checks.** `pnpm check:architecture` runs automated
  checks on the codebase to catch architecture violations early.
- **`AGENTS.md`** at the repo root contains non-negotiable rules that
  all agents must read before starting work.

### Architecture Guardrails

Automated and manual guardrails prevent architecture drift:

1. **`pnpm check:architecture`** — automated script that checks:
   - No Electron code in active directories.
   - No CEF or CMake browser targets.
   - No parallel services that duplicate Chromium.
   - No state ownership in UI layers.
   - Patch files follow conventions.

2. **ADR requirement** — any change that would alter the architecture
   requires an ADR before implementation. This includes new layers,
   new Chromium subsystem replacements, and new patch categories.

3. **Code review** — all changes are reviewed for architecture
   compliance before merge. Reviewers check against the rules in
   `AGENTS.md` and the ADRs.

4. **`git diff --check`** — whitespace and basic quality check.

### New Feature Workflow

When a new feature needs to be built:

1. **Identify owners.** Determine which directories the feature touches
   and which agents own those directories.
2. **Write ADR if needed.** If the feature introduces new architecture,
   write an ADR first.
3. **Implement in parallel.** Each owning agent implements its part of
   the feature within its directory.
4. **Coordinate interfaces.** Agents agree on interfaces between layers
   (types in common/, service APIs, observer interfaces).
5. **Integration.** Agents integrate their changes and test end-to-end.
6. **Architecture check.** Run `pnpm check:architecture` and verify
   all guardrails pass.
7. **Update docs.** Update ADRs, architecture overview, and patch
   points documentation as needed.

## Consequences

Positive:

- **Parallel development.** Multiple agents can work simultaneously on
  different parts of the codebase without conflict.
- **Deep expertise.** Each agent specializes in its domain, leading to
  higher-quality code in each layer.
- **Architectural consistency.** Shared guardrails (ADRs, `AGENTS.md`,
  architecture checks) ensure all agents follow the same rules.
- **Clear accountability.** Each directory has an owner who is
  responsible for quality, tests, and documentation.
- **Scalability.** Adding more agents is straightforward — assign a
  new directory or subdirectory with clear boundaries.
- **Reduced coordination overhead.** Agents don't need to coordinate
  on every change — only on cross-layer interfaces.

Negative:

- **Coordination cost at boundaries.** Changes that touch multiple
  layers require coordination between multiple agents. The common layer
  acts as a boundary interface, but changes there affect everyone.
- **Risk of silos.** If agents only look at their own directory, they
  may miss opportunities for cross-layer optimization or consistency.
- **ADR overhead.** Every architectural decision needs an ADR, which
  adds process overhead. This is acceptable because bad architecture
  decisions are expensive to fix later.
- **Dependency on good ADRs.** The system relies on ADRs being clear
  and complete. If an ADR is ambiguous, agents may interpret it
  differently.

Neutral:

- The model is optimized for AI agent workflows, but human developers
  follow the same rules. The directory ownership model works for both
  human and AI contributors.
- The number of agent roles can grow or shrink as needed. Small
  features may be handled by a single agent across multiple directories.

## Alternatives Considered

### Single-agent workflow

One agent works on the entire codebase, handling all layers.

- Rejected: Slow. Parallelism is a major advantage of agent-based
  development. A single agent would be a bottleneck. Also, no single
  agent can have deep expertise in all layers (Chromium internals,
  Views UI, build system, DevTools, etc.).

### Feature-based ownership

Each agent owns a feature end-to-end (e.g., "split view agent" owns
both the browser service and the UI views).

- Rejected: Creates silos and inconsistent architecture. If each
  feature agent implements its own service pattern and UI patterns,
  the codebase becomes inconsistent. Layer-based ownership ensures
  all services follow the same pattern and all UI follows the same
  conventions. It also aligns with Chromium's own structure (chrome/browser,
  ui/views, etc.).

### No ownership model — free-for-all

All agents can edit any file, with architecture review at PR time.

- Rejected: Chaos. Without clear ownership, agents duplicate work,
  step on each other's changes, and make inconsistent decisions.
  Architecture review becomes a bottleneck because every PR needs
  full architecture review.

### Central architecture team + feature teams

A central architecture team defines patterns, and feature teams
implement within those patterns.

- Considered: This is similar to the current model but with a stronger
  distinction between "architecture" and "feature" work. In practice,
  the architecture docs agent and the layer agents work together —
  layer agents propose changes, and the architecture agent documents
  decisions in ADRs. The distinction is already present but informal.

## References

- **Project instructions:** `AGENTS.md` (root-level agent guardrails)
- **Engineering standards:** `docs/ENGINEERING_STANDARDS.md`
- **Architecture docs:** `docs/ARCHITECTURE.md`, `docs/adr/`
- **Patch strategy:** ADR-0021 (Direct Chromium Patch Strategy)
- **Code structure:** `docs/CODE_STRUCTURE.md`
- **Check scripts:** `scripts/check-architecture.mjs`
- **Related ADRs:** ADR-0009 (Direct Chromium Architecture),
  ADR-0021 (Direct Chromium Patch Strategy)

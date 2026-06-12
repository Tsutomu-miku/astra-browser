/* eslint-disable max-lines -- Legacy Electron prototype migration reference; direct Chromium work must not expand this file. */
import type { BrowserState, SplitLayout, SplitTab, Workspace } from "../browser/types";
import { createId } from "../browser/factory";
import { getActiveWorkspace } from "../browser/selectors";

/**
 * Arc-style split view: each split is a first-class entity (SplitTab) that
 * lives alongside regular tabs in the sidebar. A split contains exactly two
 * child tabs (primary + secondary) and appears as one sidebar row.
 *
 * Split state lives on Workspace so each workspace has independent split state.
 */

export const MAX_SPLIT_VIEW_TABS = 2;

/** Create a new split tab from two existing tabs. */
export function createSplitTab(
  workspace: Workspace,
  primaryTabId: string,
  secondaryTabId: string,
  layout: SplitLayout = "horizontal",
  side: "left" | "right" = "right"
): SplitTab {
  const split: SplitTab = {
    id: createId(),
    primaryTabId,
    secondaryTabId,
    layout,
    side
  };
  workspace.splitTabs = [...workspace.splitTabs, split];
  syncLegacySplitStateFromActive(workspace);
  return split;
}

/** Remove a split (both child tabs become regular standalone tabs). */
export function removeSplitTab(workspace: Workspace, splitId: string): void {
  const wasActive = workspace.activeSplitId === splitId;
  workspace.splitTabs = workspace.splitTabs.filter((s) => s.id !== splitId);
  if (wasActive) {
    workspace.activeSplitId = null;
    workspace.splitMode = false;
    workspace.ancillaryTabIds = [];
    workspace.activeAncillaryTabId = null;
    // Keep primary tab as the active one
  }
}

/** Get a split by id. */
export function getSplitTab(workspace: Pick<Workspace, "splitTabs">, splitId: string): SplitTab | undefined {
  return workspace.splitTabs.find((s) => s.id === splitId);
}

/** Find which split (if any) contains a given tab. */
export function getSplitForTab(workspace: Pick<Workspace, "splitTabs">, tabId: string): SplitTab | undefined {
  return workspace.splitTabs.find(
    (s) => s.primaryTabId === tabId || s.secondaryTabId === tabId
  );
}

/** Check if a tab is inside any split. */
export function isTabInSplit(workspace: Pick<Workspace, "splitTabs">, tabId: string): boolean {
  return workspace.splitTabs.some(
    (s) => s.primaryTabId === tabId || s.secondaryTabId === tabId
  );
}

/** Get all tab IDs that are inside any split. */
export function getSplitChildTabIds(workspace: Pick<Workspace, "splitTabs">): string[] {
  const ids = new Set<string>();
  for (const s of workspace.splitTabs) {
    ids.add(s.primaryTabId);
    ids.add(s.secondaryTabId);
  }
  return Array.from(ids);
}

/** Activate a split view (shows both panes). */
export function activateSplitTab(workspace: Workspace, splitId: string): void {
  const split = getSplitTab(workspace, splitId);
  if (!split) return;
  workspace.activeSplitId = splitId;
  workspace.activeTabId = split.primaryTabId;
  syncLegacySplitStateFromActive(workspace);
}

/** Deactivate split view (go back to single tab). */
export function deactivateSplit(workspace: Workspace): void {
  workspace.activeSplitId = null;
  workspace.splitMode = false;
  workspace.ancillaryTabIds = [];
  workspace.activeAncillaryTabId = null;
}

/** Swap primary and secondary panes within a split. */
export function swapSplitPanes(workspace: Workspace, splitId: string): void {
  const split = getSplitTab(workspace, splitId);
  if (!split) return;
  const primary = split.primaryTabId;
  split.primaryTabId = split.secondaryTabId;
  split.secondaryTabId = primary;
  // Update activeTabId to point to the new primary
  if (workspace.activeSplitId === splitId) {
    workspace.activeTabId = split.primaryTabId;
  }
  syncLegacySplitStateFromActive(workspace);
}

/** Set which side the secondary pane appears on. */
export function setSplitSide(workspace: Workspace, splitId: string, side: "left" | "right"): void {
  const split = getSplitTab(workspace, splitId);
  if (!split) return;
  split.side = side;
  syncLegacySplitStateFromActive(workspace);
}

/** Toggle split side. */
export function toggleSplitSide(workspace: Workspace, splitId: string): void {
  const split = getSplitTab(workspace, splitId);
  if (!split) return;
  split.side = split.side === "left" ? "right" : "left";
  syncLegacySplitStateFromActive(workspace);
}

/** Replace the secondary tab in a split. */
export function setSplitSecondaryTab(workspace: Workspace, splitId: string, tabId: string): void {
  const split = getSplitTab(workspace, splitId);
  if (!split) return;
  if (split.primaryTabId === tabId) return; // can't be both
  split.secondaryTabId = tabId;
  syncLegacySplitStateFromActive(workspace);
}

/** Remove a tab from its split. If it was the primary, secondary becomes primary. */
export function removeTabFromSplit(workspace: Workspace, tabId: string): void {
  const split = getSplitForTab(workspace, tabId);
  if (!split) return;

  if (split.primaryTabId === tabId) {
    // Primary removed → secondary becomes primary, split stays open if there's...
    // wait, a split needs 2 tabs. If we remove primary, the split becomes just secondary?
    // No — if you remove one tab from a split, the split is destroyed.
    removeSplitTab(workspace, split.id);
    workspace.activeTabId = split.secondaryTabId;
  } else if (split.secondaryTabId === tabId) {
    // Secondary removed → split destroyed, primary remains
    const wasActive = workspace.activeSplitId === split.id;
    const primaryId = split.primaryTabId;
    removeSplitTab(workspace, split.id);
    if (wasActive) {
      workspace.activeTabId = primaryId;
    }
  }
}

/** Clean up stale split references (when tabs are deleted). */
export function pruneSplitTabs(workspace: Workspace): void {
  const validIds = new Set(workspace.tabs.map((t) => t.id));
  const toRemove: string[] = [];
  for (const split of workspace.splitTabs) {
    if (!validIds.has(split.primaryTabId) || !validIds.has(split.secondaryTabId)) {
      toRemove.push(split.id);
    }
  }
  for (const id of toRemove) {
    removeSplitTab(workspace, id);
  }
  // Ensure activeSplitId still valid
  if (workspace.activeSplitId && !getSplitTab(workspace, workspace.activeSplitId)) {
    workspace.activeSplitId = null;
    workspace.splitMode = false;
    workspace.ancillaryTabIds = [];
    workspace.activeAncillaryTabId = null;
  }
}

/**
 * Sync legacy split fields (splitMode, ancillaryTabIds, activeAncillaryTabId, splitSide)
 * from the active split entity. This keeps old code working during migration.
 *
 * Only writes to legacy fields when there IS an active split entity.
 * When there is no active split, legacy fields are left alone (they may be
 * controlled by grid layout mode or other features).
 */
function syncLegacySplitStateFromActive(workspace: Workspace): void {
  if (!workspace.activeSplitId) return;
  const split = getSplitTab(workspace, workspace.activeSplitId);
  if (!split) return;
  workspace.splitMode = true;
  workspace.ancillaryTabIds = [split.secondaryTabId];
  workspace.activeAncillaryTabId = split.secondaryTabId;
  workspace.splitSide = split.side;
  workspace.splitLayout = split.layout;
}

/** Sync state-level legacy fields from active workspace. */
export function syncLegacyStateSplitFields(state: BrowserState): void {
  const ws = getActiveWorkspace(state);
  state.splitMode = ws.splitMode;
  state.splitTabIds = ws.ancillaryTabIds;
  state.splitTabId = ws.activeAncillaryTabId;
}

// ===== Migration: old ancillaryTabIds model → new splitTabs model =====

export function migrateSplitsToEntityModel(workspace: Workspace): void {
  // If there are already splitTabs, skip migration
  if (workspace.splitTabs && workspace.splitTabs.length > 0) return;

  // Ensure splitTabs exists
  if (!workspace.splitTabs) workspace.splitTabs = [];

  // Migrate from old model: if splitMode is on and we have exactly one ancillary tab,
  // create one split tab with activeTabId as primary and activeAncillaryTabId as secondary.
  // Grid mode (multiple ancillary tabs) is not migrated — it's a separate feature.
  if (
    workspace.splitMode &&
    workspace.activeAncillaryTabId &&
    workspace.activeTabId &&
    workspace.ancillaryTabIds.length === 1
  ) {
    const secondaryId = workspace.activeAncillaryTabId;
    if (secondaryId !== workspace.activeTabId) {
      const split: SplitTab = {
        id: createId(),
        primaryTabId: workspace.activeTabId,
        secondaryTabId: secondaryId,
        layout: workspace.splitLayout || "horizontal",
        side: workspace.splitSide || "right"
      };
      workspace.splitTabs.push(split);
      workspace.activeSplitId = split.id;
    }
  }

  // Ensure activeSplitId is set
  if (!workspace.activeSplitId) workspace.activeSplitId = null;
}

// ===== Legacy compatibility shims =====
// These exist so old code that uses state-level split fields keeps working.
// New code should use the splitTabs entity model directly.

export function getSplitTabIds(state: BrowserState): string[] {
  const workspace = getActiveWorkspace(state);
  if (workspace.activeSplitId) {
    const split = getSplitTab(workspace, workspace.activeSplitId);
    if (split) return [split.secondaryTabId];
  }
  // Fall back to workspace-level legacy fields (grid mode)
  if (workspace.splitMode && workspace.ancillaryTabIds.length > 0) {
    return workspace.ancillaryTabIds.filter(Boolean);
  }
  // Final fallback to state-level legacy fields
  if (state.splitMode) {
    const ids = Array.isArray(state.splitTabIds) ? state.splitTabIds : [];
    return ids.length > 0 ? ids : state.splitTabId ? [state.splitTabId] : [];
  }
  return [];
}

/** @deprecated Use splitTabs entity model instead. */
export function setSplitTabIds(state: BrowserState, ids: string[]): void {
  const workspace = getActiveWorkspace(state);
  if (ids.length === 0) {
    // Close the active split
    if (workspace.activeSplitId) {
      deactivateSplit(workspace);
    }
  } else {
    // This is a legacy path — we don't have a primary tab specified.
    // Just create a split with activeTabId as primary.
    const primaryId = workspace.activeTabId;
    const secondaryId = ids[0];
    if (primaryId && secondaryId && primaryId !== secondaryId) {
      const existing = getSplitForTab(workspace, primaryId);
      if (existing) {
        setSplitSecondaryTab(workspace, existing.id, secondaryId);
        activateSplitTab(workspace, existing.id);
      } else {
        const split = createSplitTab(workspace, primaryId, secondaryId);
        activateSplitTab(workspace, split.id);
      }
    }
  }
  state.splitMode = ids.length > 0;
  state.splitTabIds = ids;
  state.splitTabId = ids[0] ?? null;
}

/** @deprecated Use splitTabs entity model instead. */
export function clearSplitView(state: BrowserState): void {
  const workspace = getActiveWorkspace(state);
  deactivateSplit(workspace);
  state.splitMode = false;
  state.splitTabIds = [];
  state.splitTabId = null;
}

/** @deprecated Use pruneSplitTabs instead. */
export function pruneSplitTabIds(_state: BrowserState, workspace: Workspace): void {
  pruneSplitTabs(workspace);
}

// ===== Old function signatures preserved for backward compat =====
// These operate on the "active split" (like the old splitMode model)
// and are kept so existing callers don't break.

export function isSplitModeActive(state: BrowserState): boolean {
  return getActiveWorkspace(state).splitMode;
}

export function isSplitModeActiveFromWorkspace(workspace: Workspace): boolean {
  return workspace.splitMode;
}

export function getAncillaryTabIds(workspace: Pick<Workspace, "ancillaryTabIds">): string[] {
  return workspace.ancillaryTabIds.filter(Boolean);
}

export function getActiveAncillaryTabId(
  workspace: Pick<Workspace, "activeAncillaryTabId" | "ancillaryTabIds">
): string | null {
  if (!workspace.activeAncillaryTabId) return null;
  if (!workspace.ancillaryTabIds.includes(workspace.activeAncillaryTabId)) return null;
  return workspace.activeAncillaryTabId;
}

/** Open a tab in the secondary pane — wraps active split or creates one. */
export function openTabInAncillaryPane(state: BrowserState, tabId: string): void {
  const workspace = getActiveWorkspace(state);
  if (!workspace.activeTabId) {
    if (workspace.tabs.length === 0) return;
    workspace.activeTabId = workspace.tabs[0].id;
  }
  if (workspace.activeTabId === tabId) return;

  if (workspace.activeSplitId) {
    // Active split exists → replace secondary
    setSplitSecondaryTab(workspace, workspace.activeSplitId, tabId);
  } else {
    // No active split → create a new one from active tab + target tab
    const split = createSplitTab(
      workspace,
      workspace.activeTabId,
      tabId,
      workspace.splitLayout,
      workspace.splitSide
    );
    activateSplitTab(workspace, split.id);
  }
  syncLegacyStateSplitFields(state);
}

/** Close the active split / ancillary pane. */
export function closeSplitPane(state: BrowserState): void {
  const workspace = getActiveWorkspace(state);
  deactivateSplit(workspace);
  syncLegacyStateSplitFields(state);
}

/** Remove a specific tab from the active split. */
export function removeTabFromAncillaryPane(state: BrowserState, tabId: string): void {
  const workspace = getActiveWorkspace(state);
  if (!workspace.activeSplitId) return;
  const split = getSplitTab(workspace, workspace.activeSplitId);
  if (!split) return;
  if (split.secondaryTabId === tabId) {
    deactivateSplit(workspace);
  }
  syncLegacyStateSplitFields(state);
}

/** Switch secondary / active ancillary tab. Works with both entity model and grid mode. */
export function selectAncillaryTab(state: BrowserState, tabId: string): void {
  const workspace = getActiveWorkspace(state);
  if (workspace.activeSplitId) {
    // Entity model: set the secondary tab of the active split
    setSplitSecondaryTab(workspace, workspace.activeSplitId, tabId);
  } else if (workspace.splitMode) {
    // Grid mode: just set the active ancillary tab
    if (workspace.ancillaryTabIds.includes(tabId)) {
      workspace.activeAncillaryTabId = tabId;
    }
  }
  syncLegacyStateSplitFields(state);
}

/** Swap panes of the active split. Works with both entity model and grid mode. */
export function swapSplitPanesFromState(state: BrowserState): void {
  const workspace = getActiveWorkspace(state);
  if (workspace.activeSplitId) {
    // Entity model: swap primary/secondary of the active split
    swapSplitPanes(workspace, workspace.activeSplitId);
  } else if (workspace.splitMode && workspace.activeTabId && workspace.activeAncillaryTabId) {
    // Grid mode: swap active tab with active ancillary tab
    const prevActive = workspace.activeTabId;
    const newActive = workspace.activeAncillaryTabId;
    workspace.activeTabId = newActive;
    workspace.activeAncillaryTabId = prevActive;
    // Update ancillaryTabIds array: remove new active, add old active
    workspace.ancillaryTabIds = workspace.ancillaryTabIds
      .filter((id) => id !== newActive)
      .concat([prevActive]);
  }
  syncLegacyStateSplitFields(state);
}

export function toggleSplitSideFromState(state: BrowserState): void {
  const workspace = getActiveWorkspace(state);
  if (!workspace.activeSplitId) {
    workspace.splitSide = workspace.splitSide === "left" ? "right" : "left";
    return;
  }
  toggleSplitSide(workspace, workspace.activeSplitId);
}

export function setSplitSideFromState(state: BrowserState, side: "left" | "right"): void {
  const workspace = getActiveWorkspace(state);
  if (!workspace.activeSplitId) {
    workspace.splitSide = side;
    return;
  }
  setSplitSide(workspace, workspace.activeSplitId, side);
}

export function pruneAncillaryTabIds(workspace: Workspace): void {
  pruneSplitTabs(workspace);
}

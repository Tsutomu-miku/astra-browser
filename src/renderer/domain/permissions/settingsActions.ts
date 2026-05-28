import { BrowserState, SitePermissionRule } from "../browser";
import { clearSitePermission, clearSitePermissionsForOrigin, upsertSitePermission } from "./sitePermissions";
import { updateBrowserState } from "../browser/updateState";

export function updateSettings(state: BrowserState, patch: Partial<BrowserState["settings"]>): BrowserState {
  return updateBrowserState(state, (draft) => {
    Object.assign(draft.settings, patch);
    draft.settings.memorySaverIdleMinutes = normalizeMemorySaverIdleMinutes(draft.settings.memorySaverIdleMinutes);
  });
}

function normalizeMemorySaverIdleMinutes(value: unknown): number {
  const minutes = Number(value);
  if (!Number.isFinite(minutes)) return 30;
  return Math.min(240, Math.max(1, Math.round(minutes)));
}

export function setSitePermission(
  state: BrowserState,
  rule: Pick<SitePermissionRule, "profileId" | "origin" | "permission" | "decision">
): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.sitePermissions = upsertSitePermission(
      draft.sitePermissions,
      rule.profileId,
      rule.origin,
      rule.permission,
      rule.decision
    );
  });
}

export function clearSitePermissionRule(state: BrowserState, profileId: string, origin: string, permission: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.sitePermissions = clearSitePermission(draft.sitePermissions, profileId, origin, permission);
  });
}

export function clearSitePermissionRulesForOrigin(state: BrowserState, profileId: string, origin: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.sitePermissions = clearSitePermissionsForOrigin(draft.sitePermissions, profileId, origin);
  });
}

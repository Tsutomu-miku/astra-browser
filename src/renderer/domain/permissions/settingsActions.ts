import type { BrowserState, SitePermissionRule } from "../browser";
import {
  clearAllPerOriginZoom,
  clearPerOriginZoom as clearZoomForOrigin,
  normalizePerOriginZoom,
  upsertPerOriginZoom
} from "../browser/perOriginZoom";
import { clearSitePermission, clearSitePermissionsForOrigin, upsertSitePermission } from "./sitePermissions";
import { updateBrowserState } from "../browser/updateState";

export function updateSettings(state: BrowserState, patch: Partial<BrowserState["settings"]>): BrowserState {
  return updateBrowserState(state, (draft) => {
    Object.assign(draft.settings, patch);
    draft.settings.memorySaverIdleMinutes = normalizeMemorySaverIdleMinutes(draft.settings.memorySaverIdleMinutes);
    draft.settings.defaultZoomFactor = normalizeDefaultZoomFactor(draft.settings.defaultZoomFactor);
    draft.settings.perOriginZoom = normalizePerOriginZoom(draft.settings.perOriginZoom);
    if (!isIncognitoMode(draft.settings.incognito)) {
      draft.settings.incognito = "disabled";
    }
  });
}

export function setPerOriginZoom(
  state: BrowserState,
  origin: string,
  zoomFactor: number
): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.settings.perOriginZoom = upsertPerOriginZoom(draft.settings.perOriginZoom ?? [], origin, zoomFactor);
  });
}

export function clearPerOriginZoom(state: BrowserState, origin: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.settings.perOriginZoom = clearZoomForOrigin(draft.settings.perOriginZoom ?? [], origin);
  });
}

export function clearAllPerOriginZoomSettings(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.settings.perOriginZoom = clearAllPerOriginZoom();
  });
}

export function setIncognitoMode(
  state: BrowserState,
  mode: BrowserState["settings"]["incognito"]
): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.settings.incognito = isIncognitoMode(mode) ? mode : "disabled";
  });
}

function normalizeMemorySaverIdleMinutes(value: unknown): number {
  const minutes = Number(value);
  if (!Number.isFinite(minutes)) return 30;
  return Math.min(240, Math.max(1, Math.round(minutes)));
}

function normalizeDefaultZoomFactor(value: unknown): number {
  const factor = Number(value);
  if (!Number.isFinite(factor)) return 1;
  // 与 zoom.ts 常量对齐（不直接 import 避免循环依赖）
  return Math.min(3, Math.max(0.25, Math.round(factor * 100) / 100));
}

function isIncognitoMode(
  value: unknown
): value is BrowserState["settings"]["incognito"] {
  return value === "disabled" || value === "in-memory";
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

import type {
  SitePermissionDecision,
  SitePermissionRule
} from "../browser/types";

export const COMMON_SITE_PERMISSIONS = [
  "media",
  "geolocation",
  "notifications",
  "midiSysex",
  "pointerLock"
];

const PERMISSION_LABELS: Record<string, string> = {
  geolocation: "Location",
  media: "Camera and microphone",
  midiSysex: "MIDI devices",
  notifications: "Notifications",
  pointerLock: "Pointer lock"
};

export function getOriginFromUrl(value: string | undefined): string | null {
  try {
    const url = new URL(value ?? "");
    return ["http:", "https:"].includes(url.protocol) ? url.origin : null;
  } catch {
    return null;
  }
}

export function getPermissionLabel(permission: string): string {
  return PERMISSION_LABELS[permission] ?? humanizePermission(permission);
}

export function getPermissionRule(
  rules: SitePermissionRule[],
  profileId: string,
  origin: string | null,
  permission: string
): SitePermissionRule | undefined {
  return rules.find((rule) => rule.profileId === profileId && rule.origin === origin && rule.permission === permission);
}

export function upsertSitePermission(
  rules: SitePermissionRule[],
  profileId: string,
  origin: string,
  permission: string,
  decision: SitePermissionDecision
): SitePermissionRule[] {
  const next = rules.filter((rule) => rule.profileId !== profileId || rule.origin !== origin || rule.permission !== permission);
  next.unshift({ profileId, origin, permission, decision, updatedAt: Date.now() });
  return next.slice(0, 200);
}

export function clearSitePermission(
  rules: SitePermissionRule[],
  profileId: string,
  origin: string,
  permission: string
): SitePermissionRule[] {
  return rules.filter((rule) => rule.profileId !== profileId || rule.origin !== origin || rule.permission !== permission);
}

export function clearSitePermissionsForOrigin(
  rules: SitePermissionRule[],
  profileId: string,
  origin: string
): SitePermissionRule[] {
  return rules.filter((rule) => rule.profileId !== profileId || rule.origin !== origin);
}

export function normalizeSitePermissions(
  rules: Array<Partial<SitePermissionRule> | null> | undefined
): SitePermissionRule[] {
  if (!Array.isArray(rules)) return [];

  const deduped = new Map<string, SitePermissionRule>();
  for (const rule of rules) {
    const origin = getOriginFromUrl(rule?.origin);
    const permission = String(rule?.permission ?? "").trim();
    if (!origin || !permission || !isDecision(rule?.decision)) continue;
    const profileId = String(rule.profileId ?? "default").trim() || "default";
    deduped.set(`${profileId}:${origin}:${permission}`, {
      profileId,
      origin,
      permission,
      decision: rule.decision,
      updatedAt: Number(rule.updatedAt) || Date.now()
    });
  }
  return [...deduped.values()].sort((a, b) => b.updatedAt - a.updatedAt).slice(0, 200);
}

function isDecision(value: unknown): value is SitePermissionDecision {
  return value === "allow" || value === "block";
}

function humanizePermission(permission: string): string {
  return permission
    .replace(/([a-z])([A-Z])/g, "$1 $2")
    .replace(/^./, (letter) => letter.toUpperCase());
}

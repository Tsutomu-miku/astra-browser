import type {
  AddressEntry,
  AutofillDatabase,
  BrowserState,
  PasswordEntry,
  PaymentMethodEntry,
  ReaderSettings,
  SitePermissionRule,
  TranslationSettings
} from "../browser";
import {
  clearAllPerOriginZoom,
  clearPerOriginZoom as clearZoomForOrigin,
  normalizePerOriginZoom,
  upsertPerOriginZoom
} from "../browser/perOriginZoom";
import { clearSitePermission, clearSitePermissionsForOrigin, upsertSitePermission } from "./sitePermissions";
import { updateBrowserState } from "../browser/updateState";
import { createId } from "../browser/factory";

export function updateSettings(state: BrowserState, patch: Partial<BrowserState["settings"]>): BrowserState {
  return updateBrowserState(state, (draft) => {
    Object.assign(draft.settings, patch);
    draft.settings.memorySaverIdleMinutes = normalizeMemorySaverIdleMinutes(draft.settings.memorySaverIdleMinutes);
    draft.settings.defaultZoomFactor = normalizeDefaultZoomFactor(draft.settings.defaultZoomFactor);
    draft.settings.perOriginZoom = normalizePerOriginZoom(draft.settings.perOriginZoom);
    if (!isIncognitoMode(draft.settings.incognito)) draft.settings.incognito = "disabled";
    draft.settings.autofill = normalizeAutofillDatabase(draft.settings.autofill);
    draft.settings.reader = normalizeReaderSettings(draft.settings.reader);
    draft.settings.translation = normalizeTranslationSettings(draft.settings.translation);
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

/* ======= Autofill（密码 / 地址 / 支付） actions ======= */

function normalizeAutofillDatabase(value: unknown): AutofillDatabase {
  const def: AutofillDatabase = { passwords: [], addresses: [], paymentMethods: [] };
  if (!value || typeof value !== "object") return def;
  const v = value as Partial<AutofillDatabase>;
  return {
    passwords: Array.isArray(v.passwords)
      ? v.passwords.filter((p): p is PasswordEntry => Boolean(p && p.origin && p.username && p.encryptedPassword))
      : [],
    addresses: Array.isArray(v.addresses)
      ? v.addresses.filter((a): a is AddressEntry => Boolean(a && a.recipient && a.address1 && a.city))
      : [],
    paymentMethods: Array.isArray(v.paymentMethods)
      ? v.paymentMethods.filter((p): p is PaymentMethodEntry => Boolean(p && p.cardLastFour && p.cardholderName))
      : []
  };
}

export function upsertPassword(state: BrowserState, entry: PasswordEntry): BrowserState {
  return updateBrowserState(state, (draft) => {
    const existingIndex = draft.settings.autofill.passwords.findIndex((p) => p.id === entry.id || (p.origin === entry.origin && p.username === entry.username));
    const normalized = { ...entry, updatedAt: Date.now() };
    if (existingIndex >= 0) {
      draft.settings.autofill.passwords[existingIndex] = {
        ...draft.settings.autofill.passwords[existingIndex],
        ...normalized,
        createdAt: draft.settings.autofill.passwords[existingIndex].createdAt
      };
    } else {
      draft.settings.autofill.passwords.push({ ...normalized, id: normalized.id || createId(), createdAt: normalized.createdAt || Date.now() });
    }
  });
}

export function removePassword(state: BrowserState, id: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.settings.autofill.passwords = draft.settings.autofill.passwords.filter((p) => p.id !== id);
  });
}

export function touchPasswordUsed(state: BrowserState, id: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const found = draft.settings.autofill.passwords.find((p) => p.id === id);
    if (found) found.usedAt = Date.now();
  });
}

export function upsertAddress(state: BrowserState, entry: AddressEntry): BrowserState {
  return updateBrowserState(state, (draft) => {
    const idx = draft.settings.autofill.addresses.findIndex((a) => a.id === entry.id);
    if (idx >= 0) draft.settings.autofill.addresses[idx] = entry;
    else draft.settings.autofill.addresses.push({ ...entry, id: entry.id || createId(), createdAt: entry.createdAt || Date.now() });
  });
}

export function removeAddress(state: BrowserState, id: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.settings.autofill.addresses = draft.settings.autofill.addresses.filter((a) => a.id !== id);
  });
}

export function upsertPaymentMethod(state: BrowserState, entry: PaymentMethodEntry): BrowserState {
  return updateBrowserState(state, (draft) => {
    const idx = draft.settings.autofill.paymentMethods.findIndex((p) => p.id === entry.id);
    if (idx >= 0) draft.settings.autofill.paymentMethods[idx] = { ...entry, updatedAt: Date.now() };
    else draft.settings.autofill.paymentMethods.push({ ...entry, id: entry.id || createId(), createdAt: entry.createdAt || Date.now(), updatedAt: entry.updatedAt || Date.now() });
  });
}

export function removePaymentMethod(state: BrowserState, id: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.settings.autofill.paymentMethods = draft.settings.autofill.paymentMethods.filter((p) => p.id !== id);
  });
}

/* ======= Reader & Translation actions ======= */

function normalizeReaderSettings(value: unknown): ReaderSettings {
  const def: ReaderSettings = {
    enabled: false,
    theme: "light",
    fontSize: 16,
    fontFamily: "serif",
    lineHeight: 1.6,
    contentWidth: 70
  };
  if (!value || typeof value !== "object") return def;
  const v = value as Partial<ReaderSettings>;
  const theme = v.theme === "sepia" || v.theme === "dark" ? v.theme : "light";
  const fontFamily = v.fontFamily === "sans" || v.fontFamily === "mono" ? v.fontFamily : "serif";
  return {
    enabled: Boolean(v.enabled ?? def.enabled),
    theme,
    fontSize: clamp(Number(v.fontSize), 10, 36, def.fontSize),
    fontFamily,
    lineHeight: clamp(Number(v.lineHeight), 1, 2.4, def.lineHeight),
    contentWidth: clamp(Number(v.contentWidth), 30, 120, def.contentWidth)
  };
}

function normalizeTranslationSettings(value: unknown): TranslationSettings {
  const def: TranslationSettings = {
    provider: "google",
    autoTranslate: false,
    preferredTarget: "zh-CN",
    skipOrigins: []
  };
  if (!value || typeof value !== "object") return def;
  const v = value as Partial<TranslationSettings>;
  const provider = v.provider === "libretranslate" || v.provider === "disabled" ? v.provider : "google";
  return {
    provider,
    autoTranslate: Boolean(v.autoTranslate),
    preferredTarget: String(v.preferredTarget || def.preferredTarget),
    skipOrigins: Array.isArray(v.skipOrigins) ? v.skipOrigins.filter((s) => typeof s === "string") : []
  };
}

export function updateReaderSettings(state: BrowserState, patch: Partial<ReaderSettings>): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.settings.reader = normalizeReaderSettings({ ...draft.settings.reader, ...patch });
  });
}

export function updateTranslationSettings(state: BrowserState, patch: Partial<TranslationSettings>): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.settings.translation = normalizeTranslationSettings({ ...draft.settings.translation, ...patch });
  });
}

function clamp(value: number, min: number, max: number, fallback: number): number {
  const n = Number(value);
  if (!Number.isFinite(n)) return fallback;
  return Math.min(max, Math.max(min, n));
}

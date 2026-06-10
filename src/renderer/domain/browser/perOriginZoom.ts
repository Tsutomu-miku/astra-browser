import type { PerOriginZoomRule } from "./types";
import { normalizeZoomFactor } from "./zoom";
import { getOriginFromUrl } from "../permissions/sitePermissions";

const MAX_ORIGIN_RULES = 200;

export function getZoomForOrigin(
  rules: PerOriginZoomRule[] | undefined,
  origin: string | null
): number | undefined {
  if (!origin) return undefined;
  return rules?.find((rule) => rule.origin === origin)?.zoomFactor;
}

export function getZoomForUrl(
  rules: PerOriginZoomRule[] | undefined,
  url: string | undefined
): number | undefined {
  return getZoomForOrigin(rules, getOriginFromUrl(url));
}

export function upsertPerOriginZoom(
  rules: PerOriginZoomRule[],
  origin: string,
  zoomFactor: number
): PerOriginZoomRule[] {
  const cleanOrigin = getOriginFromUrl(origin) ?? origin;
  if (!cleanOrigin) return rules;
  const factor = normalizeZoomFactor(zoomFactor);
  const next = rules.filter((rule) => rule.origin !== cleanOrigin);
  next.unshift({ origin: cleanOrigin, zoomFactor: factor, updatedAt: Date.now() });
  return next.slice(0, MAX_ORIGIN_RULES);
}

export function clearPerOriginZoom(rules: PerOriginZoomRule[], origin: string): PerOriginZoomRule[] {
  const cleanOrigin = getOriginFromUrl(origin) ?? origin;
  if (!cleanOrigin) return rules;
  return rules.filter((rule) => rule.origin !== cleanOrigin);
}

export function clearAllPerOriginZoom(): PerOriginZoomRule[] {
  return [];
}

export function normalizePerOriginZoom(
  rules: Array<Partial<PerOriginZoomRule> | null> | undefined
): PerOriginZoomRule[] {
  if (!Array.isArray(rules)) return [];
  const deduped = new Map<string, PerOriginZoomRule>();
  for (const rule of rules) {
    const origin = getOriginFromUrl(rule?.origin ?? (typeof rule?.origin === "string" ? rule.origin : undefined));
    if (!origin) continue;
    const factor = normalizeZoomFactor(rule?.zoomFactor);
    deduped.set(origin, {
      origin,
      zoomFactor: factor,
      updatedAt: Number(rule?.updatedAt) || Date.now()
    });
  }
  return [...deduped.values()].sort((a, b) => b.updatedAt - a.updatedAt).slice(0, MAX_ORIGIN_RULES);
}

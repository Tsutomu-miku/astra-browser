import type { SitePermissionRule } from "../../../domain/browser-core";
import type { UrlSecurity } from "../../../domain/browser/urlIdentity";

export interface SitePermissionSummary {
  allowedCount: number;
  blockedCount: number;
  label: string;
  totalCount: number;
}

export function getSitePermissionSummary(
  rules: SitePermissionRule[],
  profileId: string,
  origin: string | null
): SitePermissionSummary {
  const scopedRules = origin
    ? rules.filter((rule) => rule.profileId === profileId && rule.origin === origin)
    : [];
  const allowedCount = scopedRules.filter((rule) => rule.decision === "allow").length;
  const blockedCount = scopedRules.filter((rule) => rule.decision === "block").length;
  const totalCount = scopedRules.length;

  return {
    allowedCount,
    blockedCount,
    label: getSummaryLabel(allowedCount, blockedCount),
    totalCount
  };
}

export function getSecurityDescription(security: UrlSecurity): string {
  if (security === "secure") return "Connection uses HTTPS.";
  if (security === "insecure") return "Connection is not encrypted.";
  if (security === "internal") return "Astra internal page.";
  return "Security is unavailable for this entry.";
}

function getSummaryLabel(allowedCount: number, blockedCount: number): string {
  const totalCount = allowedCount + blockedCount;
  if (totalCount === 0) return "No custom permissions";

  const parts = [
    allowedCount > 0 ? `${allowedCount} allowed` : "",
    blockedCount > 0 ? `${blockedCount} blocked` : ""
  ].filter(Boolean);

  return parts.join(", ");
}

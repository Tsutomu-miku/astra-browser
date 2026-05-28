import type { SitePermissionRule } from "../../../../domain/browser";
import type { SitePermissionDecision } from "../../../../domain/browser/types";
import {
  COMMON_SITE_PERMISSIONS,
  getPermissionLabel,
  getPermissionRule
} from "../../../../domain/permissions/sitePermissions";
import { PermissionRow } from "./PermissionRow";

export function PermissionList({
  origin,
  profileId,
  rules,
  onClear,
  onSet
}: {
  origin: string;
  profileId: string;
  rules: SitePermissionRule[];
  onClear: (permission: string) => void;
  onSet: (permission: string, decision: SitePermissionDecision) => void;
}) {
  return (
    <section className="permission-list" aria-label="Site permissions">
      {COMMON_SITE_PERMISSIONS.map((permission) => (
        <PermissionRow
          key={permission}
          decision={getPermissionRule(rules, profileId, origin, permission)?.decision}
          label={getPermissionLabel(permission)}
          onClear={() => onClear(permission)}
          onSet={(decision) => onSet(permission, decision)}
        />
      ))}
    </section>
  );
}

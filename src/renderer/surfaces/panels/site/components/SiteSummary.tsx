import type { ReactNode } from "react";
import { FiCopy, FiShield, FiSliders, FiUser } from "react-icons/fi";

import type { UrlSecurity } from "../../../../domain/browser/urlIdentity";
import { getSecurityDescription, type SitePermissionSummary } from "../../model/siteInfoState";

export function SiteSummary({
  identityLabel,
  origin,
  permissionSummary,
  profileName,
  security,
  url,
  onCopyOrigin,
  onResetPermissions
}: {
  identityLabel: string;
  origin: string | null;
  permissionSummary: SitePermissionSummary;
  profileName: string;
  security: UrlSecurity;
  url: string;
  onCopyOrigin: () => void;
  onResetPermissions: () => void;
}) {
  return (
    <section className="site-summary">
      <span className={`site-security is-${security}`}>{identityLabel}</span>
      <span className="site-origin">{origin ?? url}</span>
      <div className="site-summary-actions">
        {origin && (
          <button className="site-summary-action" type="button" onClick={onCopyOrigin}>
            <FiCopy />
            <span>Copy origin</span>
          </button>
        )}
      </div>
      <div className="site-summary-grid">
        <SummaryItem icon={<FiShield />} label="Security" value={getSecurityDescription(security)} />
        <SummaryItem icon={<FiUser />} label="Profile" value={`${profileName} profile`} />
        <SummaryItem icon={<FiSliders />} label="Permissions" value={permissionSummary.label} />
      </div>
      {origin && permissionSummary.totalCount > 0 && (
        <button
          className="site-clear-permissions"
          type="button"
          onClick={onResetPermissions}
        >
          Reset permissions for this site
        </button>
      )}
    </section>
  );
}

function SummaryItem({
  icon,
  label,
  value
}: {
  icon: ReactNode;
  label: string;
  value: string;
}) {
  return (
    <article className="site-summary-item">
      <span className="site-summary-icon">{icon}</span>
      <span className="site-summary-copy">
        <span>{label}</span>
        <strong>{value}</strong>
      </span>
    </article>
  );
}

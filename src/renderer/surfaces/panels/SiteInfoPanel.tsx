import type { ReactNode } from "react";
import { FiShield, FiSliders, FiUser, FiX } from "react-icons/fi";

import { getUrlIdentity } from "../../domain/browser/urlIdentity";
import {
  COMMON_SITE_PERMISSIONS,
  getOriginFromUrl,
  getPermissionLabel,
  getPermissionRule
} from "../../domain/permissions/sitePermissions";
import type { SitePermissionDecision } from "../../domain/browser/types";
import type { BrowserController } from "../../app/controller/types";
import { getSecurityDescription, getSitePermissionSummary } from "./model/siteInfoState";

export function SiteInfoPanel({ controller }: { controller: BrowserController }) {
  const { actions, activeTab, activeWorkspace, setPanel, state } = controller;
  const identity = getUrlIdentity(activeTab.url);
  const origin = getOriginFromUrl(activeTab.url);
  const permissionSummary = getSitePermissionSummary(state.sitePermissions, activeWorkspace.profileId, origin);

  return (
    <aside className="site-panel">
      <header className="panel-header">
        <h2>Site</h2>
        <button className="icon-button" title="Close site info" type="button" onClick={() => setPanel(null)}><FiX /></button>
      </header>
      <section className="site-summary">
        <span className={`site-security is-${identity.security}`}>{identity.label}</span>
        <span className="site-origin">{origin ?? activeTab.url}</span>
        <div className="site-summary-grid">
          <SummaryItem icon={<FiShield />} label="Security" value={getSecurityDescription(identity.security)} />
          <SummaryItem icon={<FiUser />} label="Profile" value={`${activeWorkspace.profileName} profile`} />
          <SummaryItem icon={<FiSliders />} label="Permissions" value={permissionSummary.label} />
        </div>
      </section>
      {origin ? (
        <section className="permission-list" aria-label="Site permissions">
          {COMMON_SITE_PERMISSIONS.map((permission) => (
            <PermissionRow
              key={permission}
              decision={getPermissionRule(state.sitePermissions, activeWorkspace.profileId, origin, permission)?.decision}
              label={getPermissionLabel(permission)}
              onClear={() => actions.clearSitePermission(activeWorkspace.profileId, origin, permission)}
              onSet={(decision) => actions.setSitePermission(activeWorkspace.profileId, origin, permission, decision)}
            />
          ))}
        </section>
      ) : (
        <p className="empty-state">Permissions are available for http and https pages.</p>
      )}
    </aside>
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

function PermissionRow({
  decision,
  label,
  onClear,
  onSet
}: {
  decision?: SitePermissionDecision;
  label: string;
  onClear: () => void;
  onSet: (decision: SitePermissionDecision) => void;
}) {
  return (
    <article className="permission-row">
      <span className="permission-name">{label}</span>
      <div className="permission-choice">
        <button type="button" aria-pressed={!decision} onClick={onClear}>Ask</button>
        <button type="button" aria-pressed={decision === "allow"} onClick={() => onSet("allow")}>Allow</button>
        <button type="button" aria-pressed={decision === "block"} onClick={() => onSet("block")}>Block</button>
      </div>
    </article>
  );
}

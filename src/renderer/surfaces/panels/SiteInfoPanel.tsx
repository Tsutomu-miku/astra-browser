import { FiX } from "react-icons/fi";

import { getUrlIdentity } from "../../domain/browser/urlIdentity";
import { getOriginFromUrl } from "../../domain/permissions/sitePermissions";
import type { BrowserController } from "../../app/controller/types";
import { getSitePermissionSummary } from "./model/siteInfoState";
import { PermissionList } from "./site/components/PermissionList";
import { SiteSummary } from "./site/components/SiteSummary";

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
      <SiteSummary
        identityLabel={identity.label}
        origin={origin}
        permissionSummary={permissionSummary}
        profileName={activeWorkspace.profileName}
        security={identity.security}
        url={activeTab.url}
        onCopyOrigin={() => {
          if (origin) actions.copyText(origin);
        }}
        onResetPermissions={() => {
          if (origin) actions.clearSitePermissionsForOrigin(activeWorkspace.profileId, origin);
        }}
      />
      {origin ? (
        <PermissionList
          origin={origin}
          profileId={activeWorkspace.profileId}
          rules={state.sitePermissions}
          onClear={(permission) => actions.clearSitePermission(activeWorkspace.profileId, origin, permission)}
          onSet={(permission, decision) => actions.setSitePermission(activeWorkspace.profileId, origin, permission, decision)}
        />
      ) : (
        <p className="empty-state">Permissions are available for http and https pages.</p>
      )}
    </aside>
  );
}

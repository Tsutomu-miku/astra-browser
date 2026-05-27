import { getPermissionLabel } from "../../domain/sitePermissions";
import type { BrowserController } from "../../app/controller/types";

export function PermissionPrompt({ controller }: { controller: BrowserController }) {
  const { actions, permissionRequest } = controller;
  if (!permissionRequest) return null;

  return (
    <aside className="permission-prompt" role="dialog" aria-label="Site permission request">
      <div className="permission-copy">
        <span className="permission-origin">{permissionRequest.origin}</span>
        <span className="permission-message">
          Wants to use {getPermissionLabel(permissionRequest.permission).toLowerCase()}
        </span>
      </div>
      <div className="permission-actions">
        <button className="toolbar-button" type="button" onClick={() => actions.resolvePermissionRequest("block")}>
          Block
        </button>
        <button className="toolbar-button primary-action" type="button" onClick={() => actions.resolvePermissionRequest("allow")}>
          Allow
        </button>
      </div>
    </aside>
  );
}

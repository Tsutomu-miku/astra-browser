import { useState } from "react";
import { FiEye, FiEyeOff, FiLock, FiShield } from "react-icons/fi";

import type { BrowserController } from "../../app/controller/types";
import {
  getPermissionIcon,
  getPermissionLabel
} from "../../domain/permissions/sitePermissions";

export function PermissionPrompt({ controller }: { controller: BrowserController }) {
  const { actions, permissionRequest } = controller;
  const [remember, setRemember] = useState(true);
  if (!permissionRequest) return null;

  const label = getPermissionLabel(permissionRequest.permission);
  const icon = getPermissionIcon(permissionRequest.permission);

  return (
    <aside className="permission-prompt permission-prompt-card" role="dialog" aria-label={`${label} permission request`}>
      <div className="permission-copy">
        <span className="permission-origin">
          <FiShield aria-hidden /> {permissionRequest.origin}
        </span>
        <span className="permission-message">
          <span className="permission-icon" aria-hidden>{icon}</span>
          Wants to use <strong>{label.toLowerCase()}</strong>
        </span>
        <label className="permission-remember">
          <input
            type="checkbox"
            checked={remember}
            onChange={(event) => setRemember(event.target.checked)}
          />
          Remember this decision for this site
        </label>
      </div>
      <div className="permission-actions permission-actions-col">
        <button
          className="toolbar-button"
          type="button"
          onClick={() => {
            window.astraShell?.resolvePermissionRequest(permissionRequest.id, false);
            actions.resolvePermissionRequest("block");
          }}
        >
          <FiEyeOff aria-hidden /> Block
        </button>
        <button
          className="toolbar-button primary-action"
          type="button"
          onClick={() => {
            window.astraShell?.resolvePermissionRequest(permissionRequest.id, true);
            actions.resolvePermissionRequest("allow");
          }}
        >
          <FiEye aria-hidden /> Allow
        </button>
      </div>
    </aside>
  );
}

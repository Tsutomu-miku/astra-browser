import { FiLock, FiUser } from "react-icons/fi";

import type { BrowserController } from "../../app/controller/types";

export function SavePasswordPrompt({ controller }: { controller: BrowserController }) {
  const { actions, passwordSavePrompt } = controller;
  if (!passwordSavePrompt) return null;

  return (
    <aside className="permission-prompt save-password-prompt" role="dialog" aria-label="Save password">
      <div className="permission-copy">
        <span className="permission-origin"><FiLock aria-hidden /> {"  "}{passwordSavePrompt.origin}</span>
        <span className="permission-message">
          Save password for <FiUser aria-hidden /> <strong>{passwordSavePrompt.username}</strong>
        </span>
      </div>
      <div className="permission-actions">
        <button
          className="toolbar-button"
          type="button"
          onClick={() => {
            void window.astraShell?.resolvePasswordSave?.(passwordSavePrompt.id, false);
            actions.rejectPasswordSavePrompt(passwordSavePrompt.id);
          }}
        >
          Never
        </button>
        <button
          className="toolbar-button primary-action"
          type="button"
          onClick={() => {
            void window.astraShell?.resolvePasswordSave?.(passwordSavePrompt.id, true);
            void actions.acceptPasswordSavePrompt(passwordSavePrompt.id);
          }}
        >
          Save
        </button>
      </div>
    </aside>
  );
}

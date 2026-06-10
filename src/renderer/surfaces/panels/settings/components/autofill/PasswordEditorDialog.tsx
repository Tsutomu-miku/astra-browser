import { useEffect, useState } from "react";
import { FiEye, FiEyeOff, FiLock, FiX } from "react-icons/fi";

import type { PasswordEntry } from "../../../../../domain/browser";
import { Field } from "../shared/SettingsUIPrimitives";

export interface PasswordEditorDialogProps {
  entry: PasswordEntry;
  onClose: () => void;
  onSave: (next: {
    origin: string;
    username: string;
    notes?: string;
    plaintextPassword?: string;
  }) => Promise<string | null | void>;
}

export function PasswordEditorDialog({ entry, onClose, onSave }: PasswordEditorDialogProps) {
  const [origin, setOrigin] = useState(entry.origin || "https://");
  const [username, setUsername] = useState(entry.username || "");
  const [notes, setNotes] = useState(entry.notes || "");
  const [plaintextPassword, setPlaintextPassword] = useState("");
  const [showPassword, setShowPassword] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [busy, setBusy] = useState(false);

  useEffect(() => {
    const onKey = (event: KeyboardEvent) => {
      if (event.key === "Escape") onClose();
    };
    document.addEventListener("keydown", onKey);
    return () => document.removeEventListener("keydown", onKey);
  }, [onClose]);

  const isNew = !entry.id;

  const submit = async () => {
    if (!origin || !username || (isNew && !plaintextPassword)) {
      setError("Origin, username, and (for new entries) password are required.");
      return;
    }
    setBusy(true);
    setError(null);
    try {
      const message = await onSave({ origin, username, notes, plaintextPassword: plaintextPassword || undefined });
      if (typeof message === "string") setError(message);
    } catch (err) {
      setError(err instanceof Error ? err.message : String(err));
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="password-editor-backdrop" role="presentation" onClick={onClose}>
      <div
        className="password-editor-dialog"
        role="dialog"
        aria-modal="true"
        aria-label={isNew ? "Add password" : "Edit password"}
        onClick={(event) => event.stopPropagation()}
      >
        <header className="password-editor-header">
          <h3><FiLock aria-hidden /> {isNew ? "Add password" : "Edit password"}</h3>
          <button className="icon-button" type="button" aria-label="Close dialog" onClick={onClose}>
            <FiX />
          </button>
        </header>
        <div className="password-editor-body">
          <Field label="Origin / URL">
            <input
              type="url"
              value={origin}
              onChange={(event) => setOrigin(event.target.value)}
              placeholder="https://example.com"
              autoFocus
            />
          </Field>
          <Field label="Username / email">
            <input
              type="text"
              value={username}
              onChange={(event) => setUsername(event.target.value)}
              placeholder="you@example.com"
            />
          </Field>
          <Field
            label={isNew ? "Password" : "Password (leave blank to keep current)"}
            hint={isNew ? "AES-256-GCM encrypted with PBKDF2-derived key." : undefined}
          >
            <div className="field-inline">
              <input
                type={showPassword ? "text" : "password"}
                value={plaintextPassword}
                onChange={(event) => setPlaintextPassword(event.target.value)}
                placeholder={isNew ? "Enter password" : "•••••••• (unchanged)"}
                autoComplete="new-password"
              />
              <button
                className="icon-button"
                type="button"
                aria-label={showPassword ? "Hide password" : "Show password"}
                title={showPassword ? "Hide password" : "Show password"}
                onClick={() => setShowPassword((prev) => !prev)}
              >
                {showPassword ? <FiEyeOff /> : <FiEye />}
              </button>
            </div>
          </Field>
          <Field label="Notes">
            <textarea
              rows={3}
              value={notes}
              onChange={(event) => setNotes(event.target.value)}
              placeholder="Optional public notes (not encrypted)"
            />
          </Field>
          {error ? <p className="muted password-editor-error">{error}</p> : null}
        </div>
        <footer className="password-editor-footer">
          <button className="toolbar-button" type="button" onClick={onClose} disabled={busy}>
            Cancel
          </button>
          <button className="toolbar-button primary-action" type="button" onClick={submit} disabled={busy}>
            {busy ? "Saving…" : (isNew ? "Add" : "Save")}
          </button>
        </footer>
      </div>
    </div>
  );
}

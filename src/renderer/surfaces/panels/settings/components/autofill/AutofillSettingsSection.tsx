import { useMemo, useState } from "react";
import { FiCopy, FiEye, FiEyeOff, FiLock, FiSearch, FiUnlock } from "react-icons/fi";

import type {
  AutofillDatabase,
  PasswordEntry
} from "../../../../../domain/browser";
import { passwordMatchesOrigin } from "../../../../../domain/browser/passwordVault";
import {
  DangerButton,
  Empty,
  GroupHeader,
  NormalButton,
  Row,
  SectionHeader
} from "../shared/SettingsUIPrimitives";

export interface AutofillPanelProps {
  autofill: AutofillDatabase;
  onAddPassword: () => void;
  onEditPassword: (entry: PasswordEntry) => void;
  onRevealPassword: (id: string) => Promise<string | null>;
  onRemovePassword: (id: string) => void;
  passwordVaultUnlocked: boolean;
  onUnlockVault: (passphrase?: string) => Promise<void>;
  onLockVault: () => void;
  passwordSearchQuery: string;
  setPasswordSearchQuery: (q: string) => void;
  onAddAddress: () => void;
  onRemoveAddress: (id: string) => void;
  onAddPaymentMethod: () => void;
  onRemovePaymentMethod: (id: string) => void;
}

function usePasswordRevealer(reveal: (id: string) => Promise<string | null>) {
  const [revealed, setRevealed] = useState<Map<string, string>>(new Map());
  const show = async (id: string) => {
    if (revealed.has(id)) {
      const next = new Map(revealed);
      next.delete(id);
      setRevealed(next);
      return;
    }
    const plaintext = await reveal(id);
    if (plaintext == null) return;
    const next = new Map(revealed);
    next.set(id, plaintext);
    setRevealed(next);
  };
  const copy = async (id: string) => {
    const plaintext = revealed.get(id) ?? await reveal(id);
    if (plaintext) void navigator.clipboard?.writeText(plaintext);
  };
  return { revealed, show, copy };
}

export function AutofillSettingsSection(props: AutofillPanelProps) {
  const {
    autofill,
    onAddPassword,
    onEditPassword,
    onRevealPassword,
    onRemovePassword,
    passwordVaultUnlocked,
    onUnlockVault,
    onLockVault,
    passwordSearchQuery,
    setPasswordSearchQuery,
    onAddAddress,
    onRemoveAddress,
    onAddPaymentMethod,
    onRemovePaymentMethod
  } = props;
  const { revealed, show, copy } = usePasswordRevealer(onRevealPassword);

  const filteredPasswords = useMemo(() => {
    const q = passwordSearchQuery.trim().toLowerCase();
    const list = autofill.passwords;
    if (!q) return list;
    return list.filter((p) =>
      p.origin.toLowerCase().includes(q) ||
      p.username.toLowerCase().includes(q) ||
      (p.notes ?? "").toLowerCase().includes(q) ||
      passwordMatchesOrigin(p, q)
    );
  }, [autofill.passwords, passwordSearchQuery]);

  return (
    <section className="settings-pane" aria-label="Autofill and passwords">
      <SectionHeader
        title="Autofill and passwords"
        description="密码、地址、支付方式的本地加密库。密码通过 PBKDF2 (600k rounds) + AES-256-GCM 加密。"
      />
      <div className="field field-group" aria-label="Vault status">
        <div className="vault-status-row">
          <span>
            {passwordVaultUnlocked ? (
              <><FiUnlock aria-hidden /> Vault unlocked</>
            ) : (
              <><FiLock aria-hidden /> Vault locked</>
            )}
          </span>
          <div className="row-actions">
            {passwordVaultUnlocked ? (
              <DangerButton onClick={onLockVault} ariaLabel="Lock password vault">Lock vault</DangerButton>
            ) : (
              <NormalButton onClick={() => { void onUnlockVault(); }}>Unlock vault</NormalButton>
            )}
          </div>
        </div>
      </div>
      <div className="field field-group" aria-label="Saved passwords">
        <GroupHeader
          title={`Saved passwords (${autofill.passwords.length})`}
          action={<NormalButton onClick={onAddPassword}>Add password</NormalButton>}
        >
          {autofill.passwords.length > 0 ? (
            <label className="password-search field-inline">
              <FiSearch aria-hidden />
              <input
                type="search"
                placeholder="Search saved passwords…"
                value={passwordSearchQuery}
                onChange={(event) => setPasswordSearchQuery(event.target.value)}
              />
            </label>
          ) : null}
        </GroupHeader>
        {autofill.passwords.length === 0 ? (
          <Empty text="尚无已保存的密码。在站点登录表单保存后会出现在这里。" />
        ) : filteredPasswords.length === 0 ? (
          <Empty text={`No passwords match “${passwordSearchQuery}”.`} />
        ) : (
          <ul className="autofill-list" role="list">
            {filteredPasswords.map((entry) => {
              const plaintext = revealed.get(entry.id);
              return (
                <Row
                  key={entry.id}
                  primary={<code>{entry.origin}</code>}
                  secondary={
                    <>
                      <strong>{entry.username}</strong>
                      <span className="password-inline-reveal">
                        <button
                          className="icon-button"
                          type="button"
                          aria-label={plaintext ? "Hide password" : "Reveal password"}
                          title={plaintext ? "Hide password" : "Reveal password"}
                          onClick={() => { void show(entry.id); }}
                        >
                          {plaintext ? <FiEyeOff /> : <FiEye />}
                        </button>
                        <code className="password-inline-text">
                          {plaintext ? plaintext : "••••••••"}
                        </code>
                        <button
                          className="icon-button"
                          type="button"
                          aria-label="Copy password to clipboard"
                          title="Copy password"
                          onClick={() => { void copy(entry.id); }}
                        >
                          <FiCopy />
                        </button>
                      </span>
                      {entry.usedAt ? <small>Last used {new Date(entry.usedAt).toLocaleString()}</small> : null}
                    </>
                  }
                  actions={
                    <>
                      <NormalButton onClick={() => onEditPassword(entry)}>Edit</NormalButton>
                      <DangerButton onClick={() => onRemovePassword(entry.id)}>Delete</DangerButton>
                    </>
                  }
                />
              );
            })}
          </ul>
        )}
      </div>
      <div className="field field-group" aria-label="Saved addresses">
        <GroupHeader
          title={`Saved addresses (${autofill.addresses.length})`}
          action={<NormalButton onClick={onAddAddress}>Add address</NormalButton>}
        />
        {autofill.addresses.length === 0 ? (
          <Empty text="尚无地址。在结账流程中保存后会出现在这里。" />
        ) : (
          <ul className="autofill-list">
            {autofill.addresses.map((a) => (
              <li key={a.id}>
                <div className="autofill-main">
                  <strong>{a.label || a.recipient}</strong>
                  <span>{a.recipient} — {a.address1}, {a.city}{a.postalCode ? ` ${a.postalCode}` : ""}</span>
                </div>
                <DangerButton onClick={() => onRemoveAddress(a.id)}>Delete</DangerButton>
              </li>
            ))}
          </ul>
        )}
      </div>
      <div className="field field-group" aria-label="Payment methods">
        <GroupHeader
          title={`Payment methods (${autofill.paymentMethods.length})`}
          action={<NormalButton onClick={onAddPaymentMethod}>Add payment method</NormalButton>}
        />
        {autofill.paymentMethods.length === 0 ? (
          <Empty text="按 PCI 要求：仅保存卡号后 4 位和过期日，完整卡号经 AES-256-GCM 加密。" />
        ) : (
          <ul className="autofill-list">
            {autofill.paymentMethods.map((p) => (
              <li key={p.id}>
                <div className="autofill-main">
                  <strong>{p.label || p.cardholderName}</strong>
                  <span>•••• {p.cardLastFour}{p.expiryMonth ? ` · ${String(p.expiryMonth).padStart(2, "0")}/${p.expiryYear}` : ""}</span>
                </div>
                <DangerButton onClick={() => onRemovePaymentMethod(p.id)}>Delete</DangerButton>
              </li>
            ))}
          </ul>
        )}
      </div>
    </section>
  );
}

import type {
  AutofillDatabase,
  PasswordEntry
} from "../../../../../domain/browser";
import {
  DangerButton,
  Empty,
  GroupHeader,
  NormalButton,
  SectionHeader
} from "../shared/SettingsUIPrimitives";

export interface AutofillPanelProps {
  autofill: AutofillDatabase;
  onAddPassword: () => void;
  onEditPassword: (entry: PasswordEntry) => void;
  onRemovePassword: (id: string) => void;
  onAddAddress: () => void;
  onRemoveAddress: (id: string) => void;
  onAddPaymentMethod: () => void;
  onRemovePaymentMethod: (id: string) => void;
}

export function AutofillSettingsSection(props: AutofillPanelProps) {
  const {
    autofill,
    onAddPassword,
    onEditPassword,
    onRemovePassword,
    onAddAddress,
    onRemoveAddress,
    onAddPaymentMethod,
    onRemovePaymentMethod
  } = props;
  return (
    <section className="settings-pane" aria-label="Autofill and passwords">
      <SectionHeader
        title="Autofill and passwords"
        description="密码、地址、支付方式的本地加密库。密码通过 PBKDF2 (600k rounds) + AES-256-GCM 加密。"
      />
      <div className="field field-group" aria-label="Saved passwords">
        <GroupHeader
          title={`Saved passwords (${autofill.passwords.length})`}
          action={<NormalButton onClick={onAddPassword}>Add password</NormalButton>}
        />
        {autofill.passwords.length === 0 ? (
          <Empty text="尚无已保存的密码。在站点登录表单保存后会出现在这里。" />
        ) : (
          <ul className="autofill-list">
            {autofill.passwords.map((entry) => (
              <li key={entry.id}>
                <div className="autofill-main">
                  <code>{entry.origin}</code>
                  <strong>{entry.username}</strong>
                  {entry.usedAt ? <small>Last used {new Date(entry.usedAt).toLocaleString()}</small> : null}
                </div>
                <div className="row-actions">
                  <NormalButton onClick={() => onEditPassword(entry)}>Edit</NormalButton>
                  <DangerButton onClick={() => onRemovePassword(entry.id)}>Delete</DangerButton>
                </div>
              </li>
            ))}
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

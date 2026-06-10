/**
 * Shared UI primitives for the 11 interactive settings panels (legacy + M1).
 *
 * Kept small so each panel stays well under the 300-line max-lines rule.
 */

export function Field({ label, children, hint }: { label: string; children: React.ReactNode; hint?: string }) {
  return (
    <label className="field">
      <span>{label}</span>
      {children}
      {hint ? <small>{hint}</small> : null}
    </label>
  );
}

export function SectionHeader({ title, description }: { title: string; description?: string }) {
  return (
    <header className="settings-section-header">
      <h3>{title}</h3>
      {description ? <p className="muted">{description}</p> : null}
    </header>
  );
}

export function GroupHeader({
  title,
  action
}: {
  title: string;
  action?: React.ReactNode;
}) {
  return (
    <div className="field-group-header">
      <span>{title}</span>
      {action}
    </div>
  );
}

export function Empty({ text }: { text: string }) {
  return <p className="muted">{text}</p>;
}

export function Pill({ kind, children }: { kind: "allow" | "block"; children: React.ReactNode }) {
  return <span className={`pill ${kind}`}>{children}</span>;
}

export function Row({
  primary,
  secondary,
  pill,
  actions
}: {
  primary: React.ReactNode;
  secondary?: React.ReactNode;
  pill?: { kind: "allow" | "block"; text: string };
  actions: React.ReactNode;
}) {
  return (
    <li>
      <div className="autofill-main">
        {typeof primary === "string" ? <code>{primary}</code> : primary}
        {typeof secondary === "string" ? <strong>{secondary}</strong> : secondary}
        {pill ? <Pill kind={pill.kind}>{pill.text}</Pill> : null}
      </div>
      <div className="row-actions">{actions}</div>
    </li>
  );
}

export function DangerButton({ children, onClick, ariaLabel }: {
  children: React.ReactNode;
  onClick?: () => void;
  ariaLabel?: string;
}) {
  return (
    <button aria-label={ariaLabel} className="danger" type="button" onClick={onClick}>
      {children}
    </button>
  );
}

export function NormalButton({ children, onClick, ariaLabel }: {
  children: React.ReactNode;
  onClick?: () => void;
  ariaLabel?: string;
}) {
  return (
    <button aria-label={ariaLabel} type="button" onClick={onClick}>
      {children}
    </button>
  );
}

/**
 * Shared UI primitives for the 11 interactive settings panels (legacy + M1).
 *
 * Kept small so each panel stays well under the 300-line max-lines rule.
 */
import type { ReactNode } from "react";
import React from "react";

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
  action,
  children
}: {
  title: string;
  action?: React.ReactNode;
  children?: React.ReactNode;
}) {
  return (
    <div>
      <div className="field-group-header">
        <span>{title}</span>
        {action}
      </div>
      {children}
    </div>
  );
}

export function Empty({ text }: { text: string }) {
  return <p className="muted">{text}</p>;
}

export function Pill({ kind, children, text }: {
  kind: "allow" | "block";
  children?: ReactNode;
  text?: string;
}) {
  const content: ReactNode = children ?? text ?? "";
  return <span className={`pill ${kind}`}>{content}</span>;
}

export function Row({
  primary,
  secondary,
  pill,
  actions,
  avatarColor
}: {
  primary: ReactNode;
  secondary?: ReactNode;
  pill?: ReactNode | { kind: "allow" | "block"; text: string };
  actions: ReactNode;
  avatarColor?: string;
}) {
  const pillNode: ReactNode =
    pill && typeof pill === "object" && "kind" in pill
      ? React.createElement(Pill, { kind: (pill as { kind: "allow" | "block"; text?: string }).kind,
          text: (pill as { kind: "allow" | "block"; text?: string }).text })
      : pill;
  return (
    <li>
      {avatarColor ? (
        <div
        aria-hidden
        className="avatar-dot"
        style={{ background: avatarColor, borderRadius: "50%", width: 20, height: 20, display: "inline-block", marginRight: 8 }}
      />
      ) : null}
      <div className="autofill-main">
        {typeof primary === "string" ? <code>{primary}</code> : primary}
        {typeof secondary === "string" ? <strong>{secondary}</strong> : secondary}
        {pillNode}
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

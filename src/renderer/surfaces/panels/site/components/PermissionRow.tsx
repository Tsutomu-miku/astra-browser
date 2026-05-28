import type { SitePermissionDecision } from "../../../../domain/browser/types";

export function PermissionRow({
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

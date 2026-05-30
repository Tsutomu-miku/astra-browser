import { FiColumns, FiEye } from "react-icons/fi";

export interface SidebarModifierActionHint {
  id: "preview" | "split";
  label?: string;
  modifier?: string;
}

const DEFAULT_MODIFIER_ACTION_HINTS: SidebarModifierActionHint[] = [
  { id: "preview", modifier: "Alt", label: "Preview" },
  { id: "split", modifier: "Shift", label: "Split" }
];

export function SidebarModifierActionHints({
  ariaHidden = false,
  ariaLabel,
  className,
  hintClassName,
  hints = DEFAULT_MODIFIER_ACTION_HINTS
}: {
  ariaHidden?: boolean;
  ariaLabel?: string;
  className: string;
  hintClassName: string;
  hints?: SidebarModifierActionHint[];
}) {
  const label = ariaHidden ? undefined : ariaLabel ?? getModifierActionHintsLabel(hints);

  return (
    <span className={className} aria-hidden={ariaHidden || undefined} aria-label={label}>
      {hints.map((hint) => (
        <span className={`${hintClassName} is-${hint.id}`} data-action-hint={hint.id} key={hint.id} aria-hidden="true">
          {hint.id === "preview" ? <FiEye /> : <FiColumns />}
        </span>
      ))}
    </span>
  );
}

export function getModifierActionHintsLabel(hints: SidebarModifierActionHint[]) {
  return hints.map((hint) => `${hint.modifier} ${hint.label}`).join(", ");
}

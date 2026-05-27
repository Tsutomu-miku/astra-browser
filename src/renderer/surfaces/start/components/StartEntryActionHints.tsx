import { getStartOpenActionHints } from "../startOpenIntent";

export function StartEntryActionHints() {
  const actionHints = getStartOpenActionHints();

  return (
    <span className="start-entry-action-hints" aria-label={actionHints.map((hint) => `${hint.modifier} ${hint.label}`).join(", ")}>
      {actionHints.map((hint) => (
        <span className={`start-entry-action-hint is-${hint.id}`} key={hint.id}>
          <kbd>{hint.modifier}</kbd>
          <span>{hint.label}</span>
        </span>
      ))}
    </span>
  );
}

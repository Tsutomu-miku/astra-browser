import type { Command } from "./commandTypes";

export interface CommandRunModifiers {
  altKey: boolean;
  shiftKey: boolean;
}

export interface CommandActionHint {
  id: "preview" | "split";
  label: string;
  modifier: string;
}

export function getCommandRunner(command: Command, modifiers: CommandRunModifiers): () => void {
  if (modifiers.altKey && command.runPreview) {
    return command.runPreview;
  }

  if (modifiers.shiftKey && command.runInSplit) {
    return command.runInSplit;
  }

  return command.run;
}

export function getCommandActionHints(command: Command): CommandActionHint[] {
  return [
    command.runPreview ? { id: "preview", label: "Preview", modifier: "Alt" } : null,
    command.runInSplit ? { id: "split", label: "Split", modifier: "Shift" } : null
  ].filter((hint): hint is CommandActionHint => Boolean(hint));
}

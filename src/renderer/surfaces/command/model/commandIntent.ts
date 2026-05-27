import type { Command } from "./commandTypes";

export interface CommandRunModifiers {
  altKey: boolean;
  shiftKey: boolean;
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

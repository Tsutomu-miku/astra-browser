import type { Command } from "./commandTypes";

export type CommandPresentationIcon =
  | "chrome"
  | "closed"
  | "content"
  | "history"
  | "memory"
  | "page"
  | "search"
  | "space"
  | "split"
  | "tab";

export interface CommandPresentation {
  icon: CommandPresentationIcon;
  label: string;
}

export function getCommandPresentation(command: Command): CommandPresentation {
  const title = command.title.toLowerCase();
  const subtitle = command.subtitle.toLowerCase();

  if (subtitle.startsWith("active tab")) return { icon: "tab", label: "Active" };
  if (subtitle.startsWith("sleeping tab")) return { icon: "memory", label: "Sleeping" };
  if (subtitle.startsWith("open tab")) return { icon: "tab", label: "Tab" };
  if (title === "new tab") return { icon: "tab", label: "Tab" };
  if (subtitle.startsWith("recently closed") || title.startsWith("reopen")) return { icon: "closed", label: "Closed" };
  if (subtitle.startsWith("history")) return { icon: "history", label: "History" };
  if (subtitle.startsWith("essential")) return { icon: "content", label: "Essential" };
  if (subtitle.startsWith("favorite")) return { icon: "content", label: "Favorite" };
  if (title.includes("split") || subtitle.includes("split")) return { icon: "split", label: "Split" };
  if (title.includes("workspace") || subtitle.includes("workspace") || title.includes("space")) return { icon: "space", label: "Space" };
  if (title.includes("memory saver") || title.includes("sleep")) return { icon: "memory", label: "Memory" };
  if (title.startsWith("search ") || subtitle.includes("search")) return { icon: "search", label: "Search" };
  if (title.startsWith("open ") || subtitle.includes("address")) return { icon: "page", label: "Page" };
  if (title.includes("sidebar") || title.includes("compact") || title.includes("toolbar")) return { icon: "chrome", label: "Chrome" };

  return { icon: "page", label: "Page" };
}

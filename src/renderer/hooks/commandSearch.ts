import type { Command } from "./commandTypes";

export function getVisibleCommands(
  commands: Command[],
  query: string,
  openQuery: (query: string) => void
): Command[] {
  const normalizedQuery = query.trim().toLowerCase();
  const filteredCommands = normalizedQuery
    ? commands.filter((command) => commandMatches(command, normalizedQuery))
    : commands;

  if (!normalizedQuery) {
    return filteredCommands;
  }

  const queryCommand = {
    title: getQueryTitle(query),
    subtitle: isLikelyUrl(query) ? "Open address" : "Search with selected engine",
    run: () => openQuery(query.trim())
  };

  if (isLikelyUrl(query) || !hasStrongCommandMatch(filteredCommands, normalizedQuery)) {
    return [queryCommand, ...filteredCommands];
  }

  return [...filteredCommands, queryCommand];
}

function commandMatches(command: Command, normalizedQuery: string): boolean {
  return `${command.title} ${command.subtitle}`.toLowerCase().includes(normalizedQuery);
}

function getQueryTitle(query: string): string {
  const trimmed = query.trim();
  return isLikelyUrl(trimmed) ? `Open ${trimmed}` : `Search ${trimmed}`;
}

function hasStrongCommandMatch(commands: Command[], normalizedQuery: string): boolean {
  return commands.some((command) => command.title.toLowerCase() === normalizedQuery);
}

function isLikelyUrl(query: string): boolean {
  const trimmed = query.trim();
  return trimmed.includes("://") || /^[^\s]+\.[^\s]+$/.test(trimmed);
}

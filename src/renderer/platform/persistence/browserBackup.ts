import { normalizeState, type BrowserState, type PartialBrowserState } from "../../domain/browser-core";

const BACKUP_VERSION = 1;

interface BrowserStateBackup {
  app: "astra-browser";
  exportedAt: string;
  state: PartialBrowserState;
  version: number;
}

export function createBrowserStateBackup(state: BrowserState): string {
  return JSON.stringify({
    app: "astra-browser",
    exportedAt: new Date().toISOString(),
    state,
    version: BACKUP_VERSION
  } satisfies BrowserStateBackup, null, 2);
}

export function parseBrowserStateBackup(value: string): BrowserState {
  const parsed = JSON.parse(value) as PartialBrowserState | Partial<BrowserStateBackup>;
  if (isBrowserStateBackup(parsed)) {
    return normalizeState(parsed.state);
  }

  return normalizeState(parsed as PartialBrowserState);
}

function isBrowserStateBackup(value: PartialBrowserState | Partial<BrowserStateBackup>): value is BrowserStateBackup {
  return typeof value === "object" && value !== null && "state" in value && "version" in value;
}

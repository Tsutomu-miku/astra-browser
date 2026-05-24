import { INTERNAL_NEW_TAB_URL } from "./browser-constants";

export function isInternalNewTabUrl(url: string | undefined): boolean {
  return normalizeInternalUrl(url) === INTERNAL_NEW_TAB_URL;
}

export function isInternalPageUrl(url: string | undefined): boolean {
  return isInternalNewTabUrl(url);
}

function normalizeInternalUrl(url: string | undefined): string {
  try {
    return new URL(url ?? "").href.replace(/\/$/, "");
  } catch {
    return "";
  }
}

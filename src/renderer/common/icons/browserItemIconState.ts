import { getHostInitial, isInternalPageUrl } from "../../domain/browser";

export interface BrowserItemIconState {
  kind: "file" | "internal" | "unknown" | "web";
  label: string;
  text: string | null;
}

export function getBrowserItemIconState(url: string): BrowserItemIconState {
  if (isInternalPageUrl(url)) {
    return {
      kind: "internal",
      label: "Astra internal page",
      text: null
    };
  }

  try {
    const parsed = new URL(url);

    if (parsed.protocol === "file:") {
      return {
        kind: "file",
        label: "Local file",
        text: null
      };
    }

    if (parsed.protocol === "http:" || parsed.protocol === "https:") {
      const host = parsed.hostname.replace(/^www\./, "");
      return {
        kind: "web",
        label: host || "Website",
        text: getHostInitial(url)
      };
    }
  } catch {
    return {
      kind: "unknown",
      label: "Unknown page",
      text: "?"
    };
  }

  return {
    kind: "unknown",
    label: "Browser page",
    text: getHostInitial(url)
  };
}

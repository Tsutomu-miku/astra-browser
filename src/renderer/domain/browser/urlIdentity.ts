export type UrlSecurity = "secure" | "insecure" | "internal" | "unknown";

export interface UrlIdentity {
  host: string;
  label: string;
  security: UrlSecurity;
}

export function getUrlIdentity(url: string): UrlIdentity {
  try {
    const parsed = new URL(url);
    if (parsed.protocol === "https:") {
      return {
        host: parsed.hostname,
        label: parsed.hostname.replace(/^www\./, ""),
        security: "secure"
      };
    }

    if (parsed.protocol === "http:") {
      return {
        host: parsed.hostname,
        label: parsed.hostname.replace(/^www\./, ""),
        security: "insecure"
      };
    }

    return {
      host: parsed.protocol.replace(":", ""),
      label: parsed.protocol.replace(":", ""),
      security: "internal"
    };
  } catch {
    return {
      host: "",
      label: "Search",
      security: "unknown"
    };
  }
}

export function getSecurityGlyph(security: UrlSecurity): string {
  const glyphs: Record<UrlSecurity, string> = {
    insecure: "!",
    internal: "•",
    secure: "✓",
    unknown: "?"
  };

  return glyphs[security];
}

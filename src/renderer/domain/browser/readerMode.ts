/**
 * 阅读模式 MVP（PRD §3.9 V-3）。
 *
 * MVP 不依赖真实 Readability.js：走简化版正文提取——
 * 以"字符数最多且包含 <p> 最多的 <article>/<main>/<div>"作为候选节点，
 * 剥离 script/style/iframe/nav/footer，输出 sanitized HTML。
 * 真实替换在 M2 引入 @mozilla/readability 前保持独立。
 */

import type { ReaderTheme } from "./types";

export type { ReaderTheme };
export interface ReaderContent {
  title: string;
  byline?: string;
  excerpt?: string;
  html: string;
  wordCount: number;
  /** 提取用时（ms） */
  extractedAt: number;
  /** 是否来自 Readability 完整实现 — MVP 为 false */
  fullQuality: boolean;
}

const BLOCKLIST_TAGS = new Set([
  "SCRIPT", "STYLE", "NOSCRIPT", "IFRAME", "NAV", "FOOTER", "ASIDE", "HEADER",
  "FORM", "BUTTON", "INPUT", "SELECT", "TEXTAREA", "SVG", "CANVAS"
]);
const SCORING_SELECTORS = ["article", "main", "[role=main]", "section", "div"];

function scoreNode(el: Element): number {
  if (!el) return 0;
  const text = el.textContent || "";
  const pCount = el.querySelectorAll("p").length;
  const linkDensity = (() => {
    const links = el.querySelectorAll("a");
    const linkText = Array.from(links).map((a) => a.textContent || "").join("").length;
    return text.length ? linkText / text.length : 0;
  })();
  const paragraphs = Math.max(1, pCount);
  return text.length + paragraphs * 40 - linkDensity * text.length * 2;
}

export function extractReaderContent(source: string | Document): ReaderContent {
  const started = Date.now();
  let doc: Document;
  if (typeof source === "string") {
    const parser = new DOMParser();
    doc = parser.parseFromString(source, "text/html");
  } else {
    doc = source;
  }

  // Clean up <script>/<style> first
  for (const tag of BLOCKLIST_TAGS) {
    doc.querySelectorAll(tag).forEach((el) => el.remove());
  }

  let best: Element | null = null;
  let bestScore = 0;
  for (const sel of SCORING_SELECTORS) {
    doc.querySelectorAll(sel).forEach((el) => {
      const s = scoreNode(el);
      if (s > bestScore) {
        bestScore = s;
        best = el;
      }
    });
  }
  if (!best || bestScore < 400) {
    best = doc.body || doc.documentElement;
  }

  // Remove ad-like / nav-like nested nodes from candidate
  best.querySelectorAll(BLOCKLIST_TAGS.size ? [...BLOCKLIST_TAGS].join(",") : "").forEach((el) => el.remove());
  best.querySelectorAll<HTMLElement>("[class*='ad'],[class*='sidebar'],[class*='nav'],[id*='ad'],[id*='sidebar']").forEach((el) => {
    if (!(el === best)) el.remove();
  });

  const html = best.innerHTML.trim() || "<p>（无法提取正文）</p>";
  const title = doc.querySelector("h1")?.textContent?.trim() || doc.title || "Untitled";
  const byline = doc.querySelector<HTMLMetaElement>("meta[name='author']")?.content || doc.querySelector('[rel="author"]')?.textContent?.trim() || undefined;
  const plain = best.textContent || "";
  const wordCount = plain.trim().split(/\s+/).filter(Boolean).length;
  const excerpt = plain.trim().slice(0, 200);

  return {
    title,
    byline,
    excerpt,
    html,
    wordCount,
    extractedAt: started,
    fullQuality: false
  };
}

const THEME_STYLES: Record<ReaderTheme, string> = {
  light: "color:#222;background:#fafafa;",
  sepia: "color:#5b4636;background:#f4ecd8;",
  dark: "color:#e6e6e6;background:#1c1c1c;"
};

export function applyReaderStyles(params: {
  content: ReaderContent;
  fontSize: number;
  fontFamily: string;
  lineHeight: number;
  contentWidth: number;
  theme: ReaderTheme;
}): string {
  const { content, fontSize, fontFamily, lineHeight, contentWidth, theme } = params;
  const font = fontFamily === "sans" ? "ui-sans-serif, system-ui, -apple-system, sans-serif"
    : fontFamily === "mono" ? "ui-monospace, Menlo, Consolas, monospace"
    : "Georgia, 'Source Serif Pro', 'Noto Serif SC', serif";
  const style = `
body { ${THEME_STYLES[theme]} font-family:${font}; font-size:${fontSize}px; line-height:${lineHeight}; max-width:${contentWidth}ch; margin:0 auto; padding:2rem 1.2rem; }
h1 { font-size:1.9em; line-height:1.25; margin-bottom:1rem; }
h2 { font-size:1.5em; margin-top:1.6em; }
h3 { font-size:1.25em; margin-top:1.3em; }
p { margin:1em 0; }
img { max-width:100%; height:auto; border-radius:6px; }
figure { margin:1.2em 0; }
code { font-family:ui-monospace, Menlo, Consolas, monospace; background:rgba(127,127,127,0.14); padding:0.1em 0.35em; border-radius:4px; }
blockquote { border-left:3px solid currentColor; opacity:0.85; padding:0.2em 1em; margin:1.2em 0; }
a { color:inherit; text-decoration:underline; }
.byline { opacity:0.7; font-size:0.9em; margin-bottom:1.6em; }
`.trim();
  const bylineHtml = content.byline ? `<div class="byline">${content.byline}</div>` : "";
  return `<!doctype html><html><head><meta charset="utf-8"><title>${content.title}</title><style>${style}</style></head><body><h1>${content.title}</h1>${bylineHtml}${content.html}</body></html>`;
}

/**
 * 页面级翻译 MVP（PRD §3.9 V-12）。
 *
 * MVP 策略：
 *  - provider = "google"：走 translate.google.com/translate?sl=auto&tl=target&u=<url>
 *    把当前 URL 在一个新 tab（或 split view）打开 Google 官方重写结果页。
 *  - provider = "libretranslate"：保留接口，M2 填真实后端。
 *  - provider = "disabled"：完全不启用。
 *
 * 这比嵌入 Chromium translate 模块成本低得多，也完全规避了 API Key。
 */

import type { BrowserState, TranslationProvider, TranslationSettings } from "./types";
import { normalizeOrigin as passwordOrigin } from "./passwordVault";

export { type TranslationProvider, type TranslationSettings };

const GOOGLE_TRANSLATE_BASE = "https://translate.google.com/translate";

export function buildTranslateUrl(opts: {
  provider: TranslationProvider;
  url: string;
  sourceLang?: string;
  targetLang: string;
}): string | null {
  const { provider, url, sourceLang = "auto", targetLang } = opts;
  if (!url || !targetLang) return null;
  if (provider === "google") {
    const u = new URL(GOOGLE_TRANSLATE_BASE);
    u.searchParams.set("sl", sourceLang);
    u.searchParams.set("tl", targetLang);
    u.searchParams.set("u", url);
    return u.toString();
  }
  if (provider === "libretranslate") {
    // 预留接口：MVP 返回 undefined 让 UI 提示"尚未配置 LibreTranslate 后端"
    return null;
  }
  return null;
}

export function detectLanguage(text: string): string {
  // 超轻量启发式：CJK 字符比例 → "zh"、日文假名 → "ja"、韩文 → "ko"；默认 "en"
  const t = String(text || "").slice(0, 500);
  if (!t) return "en";
  const cjk = (t.match(/[\u4e00-\u9fff]/g) || []).length;
  const kana = (t.match(/[\u3040-\u30ff]/g) || []).length;
  const hangul = (t.match(/[\uac00-\ud7af]/g) || []).length;
  if (hangul > cjk && hangul > kana) return "ko";
  if (kana > cjk) return "ja";
  if (cjk > 40) return "zh";
  return "en";
}

export function shouldOfferTranslateForUrl(state: Pick<BrowserState, "settings">, url: string): boolean {
  const t = state.settings.translation;
  if (!t || t.provider === "disabled") return false;
  if (t.skipOrigins.includes(passwordOrigin(url))) return false;
  return true;
}

export interface TranslationPanelStatus {
  enabled: boolean;
  provider: TranslationProvider;
  auto: boolean;
  target: string;
  suggestedUrl: string | null;
  skipped: boolean;
}

export function getTranslationPanelStatus(
  state: Pick<BrowserState, "settings">,
  url: string,
  pageText = ""
): TranslationPanelStatus {
  const t = state.settings.translation;
  const provider = t.provider;
  const target = t.preferredTarget || "zh-CN";
  const skipped = t.skipOrigins.includes(passwordOrigin(url));
  const source = pageText ? detectLanguage(pageText) : "auto";
  const suggested = provider === "google" ? buildTranslateUrl({ provider, url, sourceLang: source, targetLang: target }) : null;
  return {
    enabled: provider !== "disabled",
    provider,
    auto: t.autoTranslate && !skipped,
    target,
    suggestedUrl: suggested,
    skipped
  };
}

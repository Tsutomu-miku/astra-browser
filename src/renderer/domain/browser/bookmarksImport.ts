/**
 * 书签导入（PRD §3.5 D-10）。
 *
 * 仅支持 Netscape bookmark HTML 格式（Chrome / Edge / Firefox / Safari
 * 导出的 HTML 都满足这个规范）。M1 MVP：解析后写入 Essentials +
 * workspace.favorites，不做 UI 化的分桶预览。
 */

import { createFavorite, createId } from "./factory";
import type { BookmarksImportBatch, Favorite } from "./types";
import { normalizeAddress } from "./navigation";

export type { BookmarksImportBatch };
export interface ImportedBookmarkNode {
  id: string;
  title: string;
  url: string;
  addDate?: number;
  folderPath: string[];
}

export interface ImportedBookmarkFolder {
  name: string;
  children: Array<ImportedBookmarkNode | ImportedBookmarkFolder>;
}

const BOOKMARKS_DOCTYPE = /^<!DOCTYPE\s+NETSCAPE-Bookmark-file-1/i;
const DL_RE = /<DL([^>]*)?>([\s\S]*?)<\/DL>/gi;
const DT_RE = /<DT>([\s\S]*?)(?=<\/?DT>|<DL|$)/gi;
const H3_RE = /<H3([^>]*)?>([^<]*)<\/H3>/i;
const A_RE = /<A([^>]*)?>([^<]*)<\/A>/i;

function parseAttrs(attrsRaw: string): Record<string, string> {
  const out: Record<string, string> = {};
  const re = /\s([a-z0-9_-]+)=("[^"]*"|'[^']*'|\S+)/gi;
  let m: RegExpExecArray | null;
  while ((m = re.exec(attrsRaw)) !== null) {
    let v = m[2];
    if ((v.startsWith('"') && v.endsWith('"')) || (v.startsWith("'") && v.endsWith("'"))) v = v.slice(1, -1);
    out[m[1].toLowerCase()] = v;
  }
  return out;
}

function parseDlBlock(inner: string, path: string[] = []): Array<ImportedBookmarkNode | ImportedBookmarkFolder> {
  const items: Array<ImportedBookmarkNode | ImportedBookmarkFolder> = [];
  const text = inner;
  // 剥离子 <DL> 段，记录位置以便对应到 H3
  const subBlocks: { start: number; end: number; inner: string; afterH3: string | null }[] = [];
  let safe = text;
  DL_RE.lastIndex = 0;
  let m: RegExpExecArray | null;
  while ((m = DL_RE.exec(text)) !== null) {
    // m.index 为起点；向前查找最近的 <H3>...</H3>
    const prev = text.slice(0, m.index);
    const lastH3 = prev.match(/<H3[^>]*>([^<]*)<\/H3>\s*$/i);
    subBlocks.push({
      start: m.index,
      end: m.index + m[0].length,
      inner: m[2],
      afterH3: lastH3 ? lastH3[1].trim() : null
    });
    safe = safe.slice(0, m.index) + " ".repeat(m[0].length) + safe.slice(m.index + m[0].length);
  }
  // 在 safe 文本里迭代 <DT>
  DT_RE.lastIndex = 0;
  let dt: RegExpExecArray | null;
  while ((dt = DT_RE.exec(safe)) !== null) {
    const body = dt[1];
    const h3Match = body.match(H3_RE);
    const aMatch = body.match(A_RE);
    if (h3Match) {
      const title = h3Match[2].trim();
      const attrs = parseAttrs(h3Match[1] || "");
      const childrenStart = dt.index + dt[1].length;
      const sub = subBlocks.find((block) => block.start >= childrenStart && block.afterH3 === title);
      const folder: ImportedBookmarkFolder = {
        name: title || attrs.folder || "Folder",
        children: sub ? parseDlBlock(sub.inner, [...path, title || "Folder"]) : []
      };
      items.push(folder);
    } else if (aMatch) {
      const attrs = parseAttrs(aMatch[1] || "");
      const addDate = attrs.add_date ? Number(attrs.add_date) * 1000 : undefined;
      const node: ImportedBookmarkNode = {
        id: createId(),
        title: aMatch[2].trim() || attrs.href || "Untitled",
        url: normalizeAddress(attrs.href || ""),
        addDate,
        folderPath: [...path]
      };
      items.push(node);
    }
  }
  return items;
}

export function parseBookmarksHtml(html: string): ImportedBookmarkFolder {
  const trimmed = String(html || "").trim();
  if (!trimmed) return { name: "Bookmarks", children: [] };
  if (!BOOKMARKS_DOCTYPE.test(trimmed)) {
    // 仍尝试解析
  }
  const topMatch = trimmed.match(DL_RE);
  if (!topMatch) return { name: "Bookmarks", children: [] };
  const firstInner = DL_RE.exec(topMatch[0])?.[2] ?? "";
  DL_RE.lastIndex = 0;
  const outer = DL_RE.exec(trimmed)?.[2] ?? firstInner;
  return { name: "Bookmarks", children: parseDlBlock(outer) };
}

function flatten(root: ImportedBookmarkFolder): ImportedBookmarkNode[] {
  const out: ImportedBookmarkNode[] = [];
  const walk = (node: ImportedBookmarkNode | ImportedBookmarkFolder) => {
    if ("url" in node) out.push(node);
    else for (const child of node.children) walk(child);
  };
  for (const child of root.children) walk(child);
  return out;
}

export interface ImportBookmarksOptions {
  source: BookmarksImportBatch["source"];
  /** 把 bookmarks bar 的项直接写进 Essentials（默认 true） */
  mergeBarToEssentials?: boolean;
  /** 最大导入数量（Electron MVP 默认 2000，避免内存问题） */
  maxCount?: number;
}

export interface ImportBookmarksResult {
  batch: BookmarksImportBatch;
  essentials: Favorite[];
  /** 其它书签按 folderPath 分组，供调用方写入对应 workspace 的 favorites */
  favoritesByFolder: Record<string, Favorite[]>;
}

export function importBookmarksFromHtml(html: string, opts: ImportBookmarksOptions): ImportBookmarksResult {
  const root = parseBookmarksHtml(html);
  const flat = flatten(root).slice(0, opts.maxCount ?? 2000);
  const essentials: Favorite[] = [];
  const favoritesByFolder: Record<string, Favorite[]> = {};
  const mergeBar = opts.mergeBarToEssentials !== false;
  for (const node of flat) {
    const fav = createFavorite(node.title, node.url);
    const topPath = node.folderPath[0] || "";
    const isBookmarksBar = /(bookmarks ?bar|收藏夹栏|书签栏)/i.test(topPath);
    if (isBookmarksBar && mergeBar) {
      essentials.push(fav);
      continue;
    }
    const key = node.folderPath.join(" / ") || "Other Bookmarks";
    if (!favoritesByFolder[key]) favoritesByFolder[key] = [];
    favoritesByFolder[key].push(fav);
  }
  const batch: BookmarksImportBatch = {
    id: createId(),
    source: opts.source,
    importedAt: Date.now(),
    importedEssentials: essentials.map((e) => e.id),
    importedFavorites: Object.values(favoritesByFolder).flat().map((e) => e.id)
  };
  return { batch, essentials, favoritesByFolder };
}

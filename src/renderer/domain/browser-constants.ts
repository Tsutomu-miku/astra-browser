import type { SearchEngine, SearchEngineKey } from "./browser-types";

export const DEFAULT_URL = "https://www.google.com";

export const SEARCH_ENGINES: Record<SearchEngineKey, SearchEngine> = {
  google: {
    name: "Google",
    url: "https://www.google.com/search?q="
  },
  duckduckgo: {
    name: "DuckDuckGo",
    url: "https://duckduckgo.com/?q="
  },
  bing: {
    name: "Bing",
    url: "https://www.bing.com/search?q="
  }
};

export const WORKSPACE_ACCENTS = [
  "#7dd3fc",
  "#f0abfc",
  "#86efac",
  "#fda4af",
  "#fde68a",
  "#c4b5fd"
];

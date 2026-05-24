export type WebviewAction = "goBack" | "goForward" | "reload";

export type WebviewElement = HTMLElement & {
  canGoBack?: () => boolean;
  canGoForward?: () => boolean;
  findInPage?: (text: string, options?: { findNext?: boolean; forward?: boolean }) => void;
  goBack?: () => void;
  goForward?: () => void;
  reload?: () => void;
  loadURL?: (url: string) => void;
  setAudioMuted?: (muted: boolean) => void;
  setZoomFactor?: (factor: number) => void;
  stopFindInPage?: (action: "clearSelection" | "keepSelection" | "activateSelection") => void;
};

export type WebviewAction = "goBack" | "goForward" | "reload" | "reloadIgnoringCache" | "stop";

export type WebviewElement = HTMLElement & {
  canGoBack?: () => boolean;
  canGoForward?: () => boolean;
  executeJavaScript?: (code: string) => Promise<unknown>;
  findInPage?: (text: string, options?: { findNext?: boolean; forward?: boolean }) => void;
  getZoomFactor?: () => Promise<number>;
  getWebContentsId?: () => number;
  goBack?: () => void;
  goForward?: () => void;
  reload?: () => void;
  reloadIgnoringCache?: () => void;
  stop?: () => void;
  loadURL?: (url: string) => void;
  print?: () => void;
  setAudioMuted?: (muted: boolean) => void;
  setZoomFactor?: (factor: number) => void;
  stopFindInPage?: (action: "clearSelection" | "keepSelection" | "activateSelection") => void;
};

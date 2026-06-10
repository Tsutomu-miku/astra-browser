import type { DownloadEntry } from "../../../../domain/browser";

export interface DownloadActionsState {
  canOpen: boolean;
  canShowInFolder: boolean;
  canPause: boolean;
  canResume: boolean;
  canCancel: boolean;
  canRetry: boolean;
  canRemove: boolean;
  progress: number;
  isPaused: boolean;
  isTerminal: boolean;
}

export function getDownloadActionsState(download: DownloadEntry): DownloadActionsState {
  const isCompleteWithPath = download.state === "completed" && Boolean(download.savePath);
  const isProgressing = download.state === "progressing";
  const isPaused = download.state === "paused";
  const isInterrupted = download.state === "interrupted";
  const isCancelled = download.state === "cancelled";
  const isTerminal = isCompleteWithPath || isCancelled || isInterrupted;

  return {
    canOpen: isCompleteWithPath,
    canShowInFolder: isCompleteWithPath,
    canPause: isProgressing && Boolean(download.canPause),
    canResume: isPaused,
    canCancel: isProgressing || isPaused,
    canRetry: isInterrupted,
    canRemove: isTerminal,
    progress: getDownloadProgress(download),
    isPaused,
    isTerminal
  };
}

export function getDownloadProgress(download: DownloadEntry): number {
  if (download.state === "completed") return 100;
  if (download.state === "paused" || !download.totalBytes) return 0;
  return Math.round((download.receivedBytes / download.totalBytes) * 100);
}

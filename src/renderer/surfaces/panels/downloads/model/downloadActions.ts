import type { DownloadEntry } from "../../../../domain/browser";

export interface DownloadActionsState {
  canOpen: boolean;
  canShowInFolder: boolean;
  progress: number;
}

export function getDownloadActionsState(download: DownloadEntry): DownloadActionsState {
  const isCompleteWithPath = download.state === "completed" && Boolean(download.savePath);
  return {
    canOpen: isCompleteWithPath,
    canShowInFolder: isCompleteWithPath,
    progress: getDownloadProgress(download)
  };
}

export function getDownloadProgress(download: DownloadEntry): number {
  if (download.state === "completed") return 100;
  if (!download.totalBytes) return 0;
  return Math.round((download.receivedBytes / download.totalBytes) * 100);
}

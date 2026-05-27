import type { DownloadEntry } from "../../../domain/browser-core";

export interface DownloadActionsState {
  canOpen: boolean;
  canShowInFolder: boolean;
  progress: number;
}

export function getDownloadActionsState(download: DownloadEntry): DownloadActionsState {
  const isCompletedWithPath = download.state === "completed" && Boolean(download.savePath);

  return {
    canOpen: isCompletedWithPath,
    canShowInFolder: isCompletedWithPath,
    progress: getDownloadProgress(download)
  };
}

export function getDownloadProgress(download: DownloadEntry): number {
  if (!download.totalBytes) return download.state === "completed" ? 100 : 0;
  return Math.round((download.receivedBytes / download.totalBytes) * 100);
}

import { formatBytes, type DownloadEntry } from "../../../../domain/browser";

export function getDownloadMeta(download: DownloadEntry, progress: number): string {
  const size = download.totalBytes ? formatBytes(download.totalBytes) : "Unknown size";
  if (download.state === "completed") return `${size} · Completed`;
  if (download.state === "interrupted") return `${size} · Interrupted`;
  if (download.state === "cancelled") return `${size} · Cancelled`;
  if (download.state === "paused") return `${size} · Paused · ${progress}%`;
  return `${size} · ${progress}%`;
}

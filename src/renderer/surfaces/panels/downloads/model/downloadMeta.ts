import { formatBytes, type DownloadEntry } from "../../../../domain/browser";

export function getDownloadMeta(download: DownloadEntry, progress: number): string {
  const size = download.totalBytes ? formatBytes(download.totalBytes) : "Unknown size";
  if (download.state === "completed") return `${size} · Completed`;
  if (download.state === "interrupted") return `${size} · Interrupted`;
  if (download.state === "cancelled") return `${size} · Cancelled`;
  return `${size} · ${progress}%`;
}

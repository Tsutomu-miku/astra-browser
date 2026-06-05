import { FiCpu, FiRefreshCw } from "react-icons/fi";

import { formatBytes, type MemoryUsageBreakdown } from "../../../../common/memory/memoryUsage";
import type { MemoryUsageStatus } from "../../../../app/controller/useMemoryUsage";
import type { MemorySaverState } from "../../../../common/memory/memorySaverState";

interface Props {
  breakdown: MemoryUsageBreakdown;
  error: string | null;
  history: number[];
  memorySaver: MemorySaverState;
  onRefresh: () => void;
  status: MemoryUsageStatus;
}

export function MemoryUsagePanel({ breakdown, error, history, memorySaver, onRefresh, status }: Props) {
  const total = Math.max(breakdown.totalBytes, 1);
  const browserShare = (breakdown.browserBytes / total) * 100;
  const webviewShare = (breakdown.webviewBytes / total) * 100;
  const largest = breakdown.perWorkspace.reduce(
    (winner, next) => (next.totalBytes > winner.totalBytes ? next : winner),
    breakdown.perWorkspace[0]
  );
  const peakHistory = history.length > 0 ? Math.max(...history) : total;

  return (
    <section className="settings-section memory-usage-section" aria-label="Memory usage">
      <div className="section-copy">
        <span className="memory-usage-title">
          <FiCpu /> Memory usage
        </span>
        <span>
          {formatBytes(breakdown.totalBytes)} total · {memorySaver.mountedWebviews} live tabs · {memorySaver.sleepingTabs} sleeping
          {status === "ready" && breakdown.processSnapshot ? ` · sampled ${formatTimestamp(breakdown.estimatedAt)}` : ""}
        </span>
      </div>
      <button
        className="icon-button"
        type="button"
        title="Refresh memory snapshot"
        onClick={onRefresh}
        disabled={status === "loading"}
      >
        <FiRefreshCw />
      </button>

      <div className="memory-usage-total" aria-label="Total memory allocation">
        <div className="memory-usage-track">
          <div
            className="memory-usage-segment memory-usage-browser"
            style={{ width: `${browserShare}%` }}
            title={`Browser shell · ${formatBytes(breakdown.browserBytes)}`}
          />
          <div
            className="memory-usage-segment memory-usage-webviews"
            style={{ width: `${webviewShare}%` }}
            title={`Tabs & webviews · ${formatBytes(breakdown.webviewBytes)}`}
          />
        </div>
        <div className="memory-usage-legend">
          <span>
            <span className="memory-swatch memory-swatch-browser" />
            Browser shell <strong>{formatBytes(breakdown.browserBytes)}</strong>
          </span>
          <span>
            <span className="memory-swatch memory-swatch-webviews" />
            Tabs & webviews <strong>{formatBytes(breakdown.webviewBytes)}</strong>
          </span>
        </div>
      </div>

      {history.length > 1 && (
        <div className="memory-usage-sparkline" aria-label="Memory usage trend">
          <svg viewBox={`0 0 ${history.length} 40`} preserveAspectRatio="none" width="100%" height="48">
            <polyline
              fill="none"
              stroke="currentColor"
              strokeWidth="1.5"
              strokeLinejoin="round"
              strokeLinecap="round"
              points={history
                .map((value, index) => {
                  const height = peakHistory > 0 ? (value / peakHistory) * 38 : 0;
                  return `${index + 0.5},${39 - height}`;
                })
                .join(" ")}
            />
          </svg>
          <span className="memory-usage-sparkline-label">
            Peak <strong>{formatBytes(peakHistory)}</strong>
          </span>
        </div>
      )}

      <ul className="memory-workspace-list" aria-label="Memory per Space">
        {breakdown.perWorkspace.map((row) => {
          const share = total > 0 ? (row.totalBytes / total) * 100 : 0;
          const topTabs = [...row.tabs]
            .sort((a, b) => b.estimatedBytes - a.estimatedBytes)
            .slice(0, 3);
          return (
            <li key={row.workspace.id} className="memory-workspace-row">
              <div className="memory-workspace-head">
                <span
                  className="memory-workspace-dot"
                  style={{ background: row.workspace.accent }}
                  aria-hidden="true"
                />
                <span className="memory-workspace-name">{row.workspace.name}</span>
                <span className="memory-workspace-size">
                  <strong>{formatBytes(row.totalBytes)}</strong>
                  <span>{row.tabs.length} tabs · {formatBytes(row.cacheBytes)} cache</span>
                </span>
              </div>
              <div className="memory-workspace-bar" aria-hidden="true">
                <div
                  className="memory-workspace-fill"
                  style={{
                    background: row.workspace.accent,
                    width: `${Math.max(4, share)}%`
                  }}
                />
              </div>
              {topTabs.length > 0 && (
                <ul className="memory-tab-list" aria-label={`Top tabs in ${row.workspace.name}`}>
                  {topTabs.map((tab) => (
                    <li key={tab.tab.id} className="memory-tab-row">
                      <span className={`memory-tab-state memory-tab-state-${tab.tab.isSleeping ? "sleeping" : "live"}`} />
                      <span className="memory-tab-title">{truncate(tab.tab.title || tab.tab.url, 48)}</span>
                      <span className="memory-tab-size">{formatBytes(tab.estimatedBytes)}</span>
                    </li>
                  ))}
                </ul>
              )}
            </li>
          );
        })}
      </ul>

      {error && <p className="memory-usage-error">{error}</p>}
      {!error && status === "ready" && largest && largest.cacheBytes > 0 && (
        <p className="memory-usage-hint">
          Largest footprint: <strong>{largest.workspace.name}</strong> uses{" "}
          <strong>{formatBytes(largest.totalBytes)}</strong> across {largest.tabs.length} tabs. Use the Memory Saver above to
          release sleeping tabs.
        </p>
      )}
    </section>
  );
}

function formatTimestamp(epochMs: number): string {
  const diff = Math.max(0, Date.now() - epochMs);
  if (diff < 2000) return "just now";
  if (diff < 60_000) return `${Math.floor(diff / 1000)}s ago`;
  return `${Math.floor(diff / 60_000)}m ago`;
}

function truncate(value: string, max: number): string {
  return value.length > max ? `${value.slice(0, max - 1)}…` : value;
}

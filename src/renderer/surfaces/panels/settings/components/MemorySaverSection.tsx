import type { MemorySaverState } from "../../../../common/memory/memorySaverState";

export function MemorySaverSection({
  memorySaver,
  onSleepInactiveTabs
}: {
  memorySaver: MemorySaverState;
  onSleepInactiveTabs: () => void;
}) {
  return (
    <section className="settings-section is-stacked memory-saver-section" aria-label="Memory saver">
      <div className="section-copy">
        <span>Memory Saver</span>
        <span>{memorySaver.summary}</span>
      </div>
      <button
        className="toolbar-button"
        type="button"
        disabled={memorySaver.reclaimableTabs === 0}
        onClick={onSleepInactiveTabs}
      >
        Sleep inactive tabs
      </button>
      <div className="memory-saver-metrics" aria-label="Memory saver tab counts">
        <span><strong>{memorySaver.mountedWebviews}</strong> mounted</span>
        <span><strong>{memorySaver.reclaimableTabs}</strong> releasable</span>
        <span><strong>{memorySaver.sleepingTabs}</strong> sleeping</span>
        <span><strong>{memorySaver.protectedTabs}</strong> protected</span>
      </div>
    </section>
  );
}

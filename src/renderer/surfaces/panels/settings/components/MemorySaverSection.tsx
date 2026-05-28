import { memorySaverIdleMinuteOptions } from "../../../../common/memory/memorySaverSettings";
import type { MemorySaverState } from "../../../../common/memory/memorySaverState";

export function MemorySaverSection({
  memorySaver,
  onUpdateMemorySaver,
  onSleepInactiveTabs
}: {
  memorySaver: MemorySaverState;
  onUpdateMemorySaver: (patch: { memorySaverEnabled?: boolean; memorySaverIdleMinutes?: number }) => void;
  onSleepInactiveTabs: () => void;
}) {
  return (
    <section className="settings-section is-stacked memory-saver-section" aria-label="Memory saver">
      <div className="section-copy">
        <span>Memory Saver</span>
        <span>{memorySaver.sleepEnabled ? `Auto sleep after ${memorySaver.sleepAfterMinutes} min · ${memorySaver.summary}` : `Manual only · ${memorySaver.summary}`}</span>
      </div>
      <button
        className="toolbar-button"
        type="button"
        disabled={memorySaver.reclaimableTabs === 0}
        onClick={onSleepInactiveTabs}
      >
        Sleep inactive tabs
      </button>
      <div className="memory-saver-controls">
        <label className="memory-saver-toggle">
          <input
            type="checkbox"
            checked={memorySaver.sleepEnabled}
            onChange={(event) => onUpdateMemorySaver({ memorySaverEnabled: event.target.checked })}
          />
          <span>Automatically sleep inactive tabs</span>
        </label>
        <label className="memory-saver-delay">
          <span>After</span>
          <select
            value={memorySaver.sleepAfterMinutes}
            disabled={!memorySaver.sleepEnabled}
            onChange={(event) => onUpdateMemorySaver({ memorySaverIdleMinutes: Number(event.target.value) })}
          >
            {memorySaverIdleMinuteOptions.map((minutes) => (
              <option key={minutes} value={minutes}>{minutes} min</option>
            ))}
          </select>
        </label>
      </div>
      <div className="memory-saver-metrics" aria-label="Memory saver tab counts">
        <span><strong>{memorySaver.mountedWebviews}</strong> mounted</span>
        <span><strong>{memorySaver.reclaimableTabs}</strong> releasable</span>
        <span><strong>{memorySaver.sleepingTabs}</strong> sleeping</span>
        <span><strong>{memorySaver.protectedTabs}</strong> protected</span>
      </div>
    </section>
  );
}

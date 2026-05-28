import { normalizeAddress, type SearchEngineKey, type Workspace } from "../../../../domain/browser";

export function SpaceSettingsSection({
  onUpdateWorkspace,
  searchEngine,
  workspace
}: {
  onUpdateWorkspace: (workspace: Partial<Workspace>) => void;
  searchEngine: SearchEngineKey;
  workspace: Workspace;
}) {
  return (
    <section className="settings-pane" aria-label="Space settings">
      <label className="field">
        <span>Workspace name</span>
        <input value={workspace.name} onChange={(event) => onUpdateWorkspace({ name: event.target.value })} />
      </label>
      <label className="field">
        <span>Workspace accent</span>
        <input type="color" value={workspace.accent} onChange={(event) => onUpdateWorkspace({ accent: event.target.value })} />
      </label>
      <label className="field">
        <span>Workspace homepage</span>
        <input
          autoComplete="off"
          spellCheck={false}
          value={workspace.homepage}
          onChange={(event) => onUpdateWorkspace({ homepage: event.target.value })}
          onBlur={(event) => onUpdateWorkspace({ homepage: normalizeAddress(event.target.value, searchEngine) })}
        />
      </label>
      <label className="field">
        <span>Workspace profile</span>
        <input value={workspace.profileName} onChange={(event) => onUpdateWorkspace({ profileName: event.target.value })} />
      </label>
    </section>
  );
}

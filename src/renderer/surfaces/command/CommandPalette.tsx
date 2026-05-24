import { KeyboardEvent } from "react";

import type { BrowserController } from "../../hooks/types";
import { getVisibleCommands } from "../../hooks/commandSearch";

export function CommandPalette({ controller }: { controller: BrowserController }) {
  const { actions, commandQuery, commands, setCommandOpen, setCommandQuery } = controller;
  const visibleCommands = getVisibleCommands(commands, commandQuery, (query) =>
    actions.openUrlInActiveWorkspace(query, query)
  );

  function onKeyDown(event: KeyboardEvent<HTMLInputElement>) {
    if (event.key === "Enter" && visibleCommands[0]) {
      event.preventDefault();
      setCommandOpen(false);
      visibleCommands[0].run();
    }
  }

  return (
    <section className="command-palette" onClick={(event) => event.target === event.currentTarget && setCommandOpen(false)}>
      <div className="command-dialog" role="dialog" aria-modal="true" aria-label="Command palette">
        <header className="command-header">
          <input
            autoFocus
            autoComplete="off"
            spellCheck={false}
            aria-label="Command"
            placeholder="Command or history"
            value={commandQuery}
            onChange={(event) => setCommandQuery(event.target.value)}
            onKeyDown={onKeyDown}
          />
          <button className="icon-button" title="Close command palette" type="button" onClick={() => setCommandOpen(false)}>×</button>
        </header>
        <div className="command-list">
          {visibleCommands.slice(0, 12).map((command) => (
            <button
              className="command-item"
              key={`${command.title}-${command.subtitle}`}
              type="button"
              onClick={() => {
                setCommandOpen(false);
                command.run();
              }}
            >
              <span className="command-title">{command.title}</span>
              <span className="command-subtitle">{command.subtitle}</span>
            </button>
          ))}
        </div>
      </div>
    </section>
  );
}

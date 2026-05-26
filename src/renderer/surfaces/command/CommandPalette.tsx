import { KeyboardEvent, useEffect, useMemo, useRef, useState } from "react";
import { FiX } from "react-icons/fi";

import { isListNavigationKey } from "../../common/navigation/listNavigation";
import type { BrowserController } from "../../hooks/types";
import { getVisibleCommands } from "../../hooks/commandSearch";
import {
  clampCommandIndex,
  getNextCommandIndex
} from "../../hooks/commandPaletteSelection";

export function CommandPalette({ controller }: { controller: BrowserController }) {
  const { actions, commandQuery, commands, setCommandOpen, setCommandQuery } = controller;
  const visibleCommands = getVisibleCommands(commands, commandQuery, (query) =>
    actions.openUrlInActiveWorkspace(query, query)
  );
  const displayedCommands = useMemo(() => visibleCommands.slice(0, 12), [visibleCommands]);
  const [activeCommandIndex, setActiveCommandIndex] = useState(0);
  const activeIndex = clampCommandIndex(activeCommandIndex, displayedCommands.length);
  const itemRefs = useRef<Array<HTMLButtonElement | null>>([]);

  useEffect(() => {
    setActiveCommandIndex(0);
  }, [commandQuery]);

  useEffect(() => {
    itemRefs.current[activeIndex]?.scrollIntoView({ block: "nearest" });
  }, [activeIndex, displayedCommands.length]);

  function onKeyDown(event: KeyboardEvent<HTMLInputElement>) {
    if (isListNavigationKey(event.key)) {
      event.preventDefault();
      const key = event.key;
      setActiveCommandIndex((index) => getNextCommandIndex(index, displayedCommands.length, key));
    } else if (event.key === "Enter" && displayedCommands[activeIndex]) {
      event.preventDefault();
      setCommandOpen(false);
      displayedCommands[activeIndex].run();
    } else if (event.key === "Escape") {
      event.preventDefault();
      setCommandOpen(false);
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
            aria-activedescendant={displayedCommands.length > 0 ? `command-option-${activeIndex}` : undefined}
          />
          <button className="icon-button" title="Close command palette" type="button" onClick={() => setCommandOpen(false)}><FiX /></button>
        </header>
        <div className="command-list" role="listbox" aria-label="Commands">
          {displayedCommands.map((command, index) => (
            <button
              className="command-item"
              id={`command-option-${index}`}
              key={`${command.title}-${command.subtitle}`}
              type="button"
              role="option"
              aria-selected={index === activeIndex}
              ref={(element) => {
                itemRefs.current[index] = element;
              }}
              onClick={() => {
                setCommandOpen(false);
                command.run();
              }}
              onMouseEnter={() => setActiveCommandIndex(index)}
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

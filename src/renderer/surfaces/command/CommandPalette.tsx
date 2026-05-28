import { KeyboardEvent, useEffect, useMemo, useRef, useState } from "react";
import { FiX } from "react-icons/fi";

import { isListNavigationKey } from "../../common/navigation/listNavigation";
import type { BrowserController } from "../../app/controller/types";
import { getCommandActionHints, getCommandRunner } from "./model/commandIntent";
import { getVisibleCommands } from "./model/commandSearch";
import {
  clampCommandIndex,
  getNextCommandIndex
} from "./model/commandPaletteSelection";

export function CommandPalette({ controller }: { controller: BrowserController }) {
  const { actions, commandQuery, commands, setCommandOpen, setCommandQuery } = controller;
  const visibleCommands = getVisibleCommands(commands, commandQuery, {
    open: (query) => actions.openUrlInActiveWorkspace(query, query),
    openInSplit: (query) => actions.openUrlInSplit(query, query)
  });
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
      getCommandRunner(displayedCommands[activeIndex], {
        altKey: event.altKey,
        shiftKey: event.shiftKey
      })();
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
          {displayedCommands.map((command, index) => {
            const actionHints = getCommandActionHints(command);
            const hasCommandMeta = Boolean(command.shortcut) || actionHints.length > 0;
            return (
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
                onClick={(event) => {
                  setCommandOpen(false);
                  getCommandRunner(command, {
                    altKey: event.altKey,
                    shiftKey: event.shiftKey
                  })();
                }}
                onMouseEnter={() => setActiveCommandIndex(index)}
              >
                <span className="command-copy">
                  <span className="command-title">{command.title}</span>
                  <span className="command-subtitle">{command.subtitle}</span>
                </span>
                {hasCommandMeta && (
                  <span className="command-meta">
                    {command.shortcut && (
                      <kbd className="command-shortcut" aria-label={`Shortcut ${command.shortcut}`}>
                        {command.shortcut}
                      </kbd>
                    )}
                    {actionHints.length > 0 && (
                      <span
                        className="command-action-hints"
                        aria-label={actionHints.map((hint) => `${hint.modifier} ${hint.label}`).join(", ")}
                      >
                        {actionHints.map((hint) => (
                          <span className={`command-action-hint is-${hint.id}`} key={hint.id}>
                            <kbd>{hint.modifier}</kbd>
                            <span>{hint.label}</span>
                          </span>
                        ))}
                      </span>
                    )}
                  </span>
                )}
              </button>
            );
          })}
        </div>
      </div>
    </section>
  );
}

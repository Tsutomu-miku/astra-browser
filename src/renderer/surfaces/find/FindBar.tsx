import { KeyboardEvent } from "react";
import { FiChevronDown, FiChevronUp, FiX } from "react-icons/fi";

import type { BrowserController } from "../../hooks/types";

export function FindBar({ controller }: { controller: BrowserController }) {
  const { actions, findQuery } = controller;

  function close() {
    actions.closeFind();
  }

  function search(query: string, forward = true) {
    actions.findInPage(query, forward);
  }

  function onKeyDown(event: KeyboardEvent<HTMLInputElement>) {
    if (event.key === "Enter") {
      event.preventDefault();
      search(findQuery, !event.shiftKey);
    }

    if (event.key === "Escape") {
      event.preventDefault();
      close();
    }
  }

  return (
    <aside className="find-bar" aria-label="Find in page">
      <input
        autoFocus
        value={findQuery}
        placeholder="Find in page"
        onChange={(event) => search(event.target.value)}
        onKeyDown={onKeyDown}
      />
      <button className="icon-button" title="Previous match" type="button" onClick={() => search(findQuery, false)}><FiChevronUp /></button>
      <button className="icon-button" title="Next match" type="button" onClick={() => search(findQuery, true)}><FiChevronDown /></button>
      <button className="icon-button" title="Close find" type="button" onClick={close}><FiX /></button>
    </aside>
  );
}

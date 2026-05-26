import {
  useCallback,
  useEffect,
  useMemo,
  useState,
  type FormEvent,
  type KeyboardEvent,
  type MouseEvent
} from "react";

import { isListNavigationKey } from "../common/navigation/listNavigation";
import type { BrowserController } from "./types";
import { getOmniboxAction } from "./omniboxActions";
import { buildOmniboxSuggestions, type OmniboxSuggestion } from "./omniboxSuggestions";
import { clampOmniboxIndex, getNextOmniboxIndex } from "./omniboxSelection";

type OmniboxControllerInput = Pick<
  BrowserController,
  "actions" | "addressValue" | "setAddressValue" | "state"
>;

export function useOmniboxController({
  actions,
  addressValue,
  setAddressValue,
  state
}: OmniboxControllerInput) {
  const [suggestionsOpen, setSuggestionsOpen] = useState(false);
  const [activeSuggestionIndex, setActiveSuggestionIndex] = useState(0);
  const suggestions = useMemo(() => buildOmniboxSuggestions(state, addressValue), [addressValue, state]);
  const activeIndex = clampOmniboxIndex(activeSuggestionIndex, suggestions.length);

  useEffect(() => {
    setActiveSuggestionIndex((index) => clampOmniboxIndex(index, suggestions.length));
  }, [suggestions.length]);

  const runSuggestion = useCallback((suggestion: OmniboxSuggestion | undefined, openInSplit = false) => {
    const action = getOmniboxAction(suggestion, addressValue, openInSplit);

    switch (action.type) {
      case "selectTab":
        actions.selectTab(action.tabId);
        break;
      case "openTabInSplit":
        actions.openTabInSplit(action.tabId);
        break;
      case "openUrlInSplit":
        actions.openUrlInSplit(action.url, action.title);
        break;
      case "navigateActiveTab":
        actions.navigateActiveTab(action.value);
        break;
    }

    setSuggestionsOpen(false);
  }, [actions, addressValue]);

  const submitAddress = useCallback((event: FormEvent) => {
    event.preventDefault();
    runSuggestion(suggestionsOpen ? suggestions[activeIndex] : undefined);
  }, [activeIndex, runSuggestion, suggestions, suggestionsOpen]);

  const onAddressKeyDown = useCallback((event: KeyboardEvent<HTMLInputElement>) => {
    if (!suggestionsOpen && isListNavigationKey(event.key)) {
      setSuggestionsOpen(true);
    }

    if (isListNavigationKey(event.key)) {
      event.preventDefault();
      const key = event.key;
      setActiveSuggestionIndex((index) => getNextOmniboxIndex(index, suggestions.length, key));
    } else if (event.key === "Enter") {
      event.preventDefault();
      runSuggestion(suggestionsOpen ? suggestions[activeIndex] : undefined, event.altKey);
    } else if (event.key === "Escape") {
      setSuggestionsOpen(false);
    }
  }, [activeIndex, runSuggestion, suggestions, suggestionsOpen]);

  const onSuggestionPointerDown = useCallback((event: MouseEvent, suggestion: OmniboxSuggestion) => {
    event.preventDefault();
    runSuggestion(suggestion, event.altKey);
  }, [runSuggestion]);

  const updateAddressValue = useCallback((value: string) => {
    setAddressValue(value);
    setSuggestionsOpen(true);
    setActiveSuggestionIndex(0);
  }, [setAddressValue]);

  return {
    activeIndex,
    onAddressKeyDown,
    onSuggestionPointerDown,
    setActiveSuggestionIndex,
    setSuggestionsOpen,
    suggestions,
    suggestionsOpen,
    submitAddress,
    updateAddressValue
  };
}

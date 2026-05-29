import {
  useCallback,
  useEffect,
  useMemo,
  useState,
  type FormEvent,
  type KeyboardEvent,
  type MouseEvent
} from "react";

import { isListNavigationKey } from "../../common/navigation/listNavigation";
import type { BrowserController } from "./types";
import { getOmniboxAction } from "../../common/omnibox/omniboxActions";
import {
  buildOmniboxSuggestions,
  getOmniboxInlineCompletion,
  type OmniboxSuggestion
} from "../../common/omnibox/omniboxSuggestions";
import { clampOmniboxIndex, getNextOmniboxIndex } from "../../common/omnibox/omniboxSelection";

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
  const inlineCompletion = useMemo(() => (
    getOmniboxInlineCompletion(suggestions, addressValue)
  ), [addressValue, suggestions]);
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
      case "openUrlInActiveWorkspace":
        actions.openUrlInActiveWorkspace(action.url, action.title);
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

  const acceptInlineCompletion = useCallback(() => {
    if (!inlineCompletion) return false;
    setAddressValue(inlineCompletion.value);
    setSuggestionsOpen(true);
    const nextIndex = suggestions.findIndex((suggestion) => suggestion.id === inlineCompletion.suggestionId);
    setActiveSuggestionIndex(nextIndex >= 0 ? nextIndex : 0);
    return true;
  }, [inlineCompletion, setAddressValue, suggestions]);

  const onAddressKeyDown = useCallback((event: KeyboardEvent<HTMLInputElement>) => {
    const shouldAcceptCompletion = (
      event.key === "Tab" ||
      (
        event.key === "ArrowRight" &&
        event.currentTarget.selectionStart === event.currentTarget.value.length &&
        event.currentTarget.selectionEnd === event.currentTarget.value.length
      )
    );

    if (shouldAcceptCompletion && acceptInlineCompletion()) {
      event.preventDefault();
      return;
    }

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
  }, [acceptInlineCompletion, activeIndex, runSuggestion, suggestions, suggestionsOpen]);

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
    completionSuffix: inlineCompletion?.suffix ?? "",
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

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
import { getOmniboxAction, type OmniboxActionModifiers } from "../../common/omnibox/omniboxActions";
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
  const [acceptedCompletionSuggestionId, setAcceptedCompletionSuggestionId] = useState<string | null>(null);
  const suggestions = useMemo(() => buildOmniboxSuggestions(state, addressValue), [addressValue, state]);
  const inlineCompletion = useMemo(() => (
    getOmniboxInlineCompletion(suggestions, addressValue)
  ), [addressValue, suggestions]);
  const activeIndex = clampOmniboxIndex(activeSuggestionIndex, suggestions.length);
  const acceptedCompletionSuggestion = acceptedCompletionSuggestionId
    ? suggestions.find((suggestion) => suggestion.id === acceptedCompletionSuggestionId)
    : undefined;
  const activeSuggestion = suggestionsOpen
    ? acceptedCompletionSuggestion ?? suggestions[activeIndex]
    : undefined;

  useEffect(() => {
    setActiveSuggestionIndex((index) => clampOmniboxIndex(index, suggestions.length));
  }, [suggestions.length]);

  useEffect(() => {
    if (!acceptedCompletionSuggestionId) return;
    if (suggestions.some((suggestion) => suggestion.id === acceptedCompletionSuggestionId)) return;
    setAcceptedCompletionSuggestionId(null);
  }, [acceptedCompletionSuggestionId, suggestions]);

  const runSuggestion = useCallback((suggestion: OmniboxSuggestion | undefined, modifiers: OmniboxActionModifiers = {}) => {
    const action = getOmniboxAction(suggestion, addressValue, modifiers);

    switch (action.type) {
      case "openGlance":
        actions.openGlance(action.url, action.title);
        break;
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

    setAcceptedCompletionSuggestionId(null);
    setSuggestionsOpen(false);
  }, [actions, addressValue]);

  const submitAddress = useCallback((event: FormEvent) => {
    event.preventDefault();
    runSuggestion(activeSuggestion);
  }, [activeSuggestion, runSuggestion]);

  const acceptInlineCompletion = useCallback(() => {
    if (!inlineCompletion) return false;
    setAddressValue(inlineCompletion.value);
    setSuggestionsOpen(true);
    setAcceptedCompletionSuggestionId(inlineCompletion.suggestionId);
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
      setAcceptedCompletionSuggestionId(null);
    }

    if (isListNavigationKey(event.key)) {
      event.preventDefault();
      const key = event.key;
      setAcceptedCompletionSuggestionId(null);
      setActiveSuggestionIndex((index) => getNextOmniboxIndex(index, suggestions.length, key));
    } else if (event.key === "Enter") {
      event.preventDefault();
      runSuggestion(activeSuggestion, {
        altKey: event.altKey,
        shiftKey: event.shiftKey
      });
    } else if (event.key === "Escape") {
      setAcceptedCompletionSuggestionId(null);
      setSuggestionsOpen(false);
    }
  }, [acceptInlineCompletion, activeSuggestion, runSuggestion, suggestions.length, suggestionsOpen]);

  const onSuggestionPointerDown = useCallback((event: MouseEvent, suggestion: OmniboxSuggestion) => {
    event.preventDefault();
    runSuggestion(suggestion, {
      altKey: event.altKey,
      shiftKey: event.shiftKey
    });
  }, [runSuggestion]);

  const updateAddressValue = useCallback((value: string) => {
    setAddressValue(value);
    setSuggestionsOpen(true);
    setAcceptedCompletionSuggestionId(null);
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

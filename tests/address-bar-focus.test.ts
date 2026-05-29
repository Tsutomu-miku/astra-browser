import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { afterEach, describe, expect, it, vi } from "vitest";

import { useAddressBarFocus } from "../src/renderer/app/controller/useAddressBarFocus";
import type { BrowserStore } from "../src/renderer/stores/browserStoreTypes";

describe("address bar focus", () => {
  afterEach(() => {
    vi.restoreAllMocks();
    document.body.replaceChildren();
  });

  it("focuses and selects the topbar address input outside compact mode", () => {
    const store = createStore(false);
    const topbarInput = appendInput("addressInput");
    const sidebarInput = appendInput("sidebarAddressInput");
    const root = renderHarness(store);

    clickFocusButton();

    expect(store.setCommandOpen).toHaveBeenCalledWith(false);
    expect(document.activeElement).toBe(topbarInput);
    expect(topbarInput.select).toHaveBeenCalled();
    expect(sidebarInput.select).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("reveals and focuses the sidebar address input in compact mode", () => {
    vi.spyOn(window, "requestAnimationFrame").mockImplementation((callback) => {
      callback(0);
      return 1;
    });
    const store = createStore(true);
    const revealCompactSidebar = vi.fn();
    const topbarInput = appendInput("addressInput");
    const sidebarInput = appendInput("sidebarAddressInput");
    const root = renderHarness(store, revealCompactSidebar);

    clickFocusButton();

    expect(store.setCommandOpen).toHaveBeenCalledWith(false);
    expect(revealCompactSidebar).toHaveBeenCalledOnce();
    expect(document.activeElement).toBe(sidebarInput);
    expect(sidebarInput.select).toHaveBeenCalled();
    expect(topbarInput.select).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("falls back to the topbar input when compact sidebar address input is unavailable", () => {
    const store = createStore(true);
    const revealCompactSidebar = vi.fn();
    const topbarInput = appendInput("addressInput");
    const root = renderHarness(store, revealCompactSidebar);

    clickFocusButton();

    expect(revealCompactSidebar).not.toHaveBeenCalled();
    expect(document.activeElement).toBe(topbarInput);
    expect(topbarInput.select).toHaveBeenCalled();

    act(() => root.unmount());
  });
});

function renderHarness(store: BrowserStore, revealCompactSidebar?: () => void) {
  const container = document.createElement("div");
  document.body.append(container);
  const root = createRoot(container);

  act(() => {
    root.render(createElement(FocusHarness, { revealCompactSidebar, store }));
  });

  return root;
}

function FocusHarness({
  revealCompactSidebar,
  store
}: {
  revealCompactSidebar?: () => void;
  store: BrowserStore;
}) {
  const focusAddressBar = useAddressBarFocus(store, revealCompactSidebar);

  return createElement("button", {
    "data-focus-address": true,
    onClick: focusAddressBar,
    type: "button"
  }, "Focus");
}

function clickFocusButton() {
  const button = document.querySelector<HTMLButtonElement>("[data-focus-address]")!;

  act(() => {
    button.dispatchEvent(new MouseEvent("click", { bubbles: true }));
  });
}

function appendInput(id: string) {
  const input = document.createElement("input");
  input.id = id;
  vi.spyOn(input, "select").mockImplementation(() => undefined);
  document.body.append(input);
  return input;
}

function createStore(compactMode: boolean) {
  return {
    compactMode,
    setCommandOpen: vi.fn()
  } as unknown as BrowserStore;
}

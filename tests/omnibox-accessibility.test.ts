import { createElement, useState } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { createDefaultState, createFavorite, type BrowserState } from "../src/renderer/domain/browser";
import { getActiveTab, getActiveWorkspace } from "../src/renderer/domain/browser/selectors";
import { Topbar } from "../src/renderer/surfaces/topbar/Topbar";

describe("omnibox accessibility", () => {
  it("exposes topbar suggestions as a combobox-backed listbox", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(Topbar, {
        controller: createController({ addressValue: "github", compactMode: false })
      }));
    });

    const input = container.querySelector<HTMLInputElement>("#addressInput")!;
    expect(input.getAttribute("role")).toBe("combobox");
    expect(input.getAttribute("aria-autocomplete")).toBe("both");
    expect(input.getAttribute("aria-controls")).toBe("address-suggestions");
    expect(input.getAttribute("aria-expanded")).toBe("false");

    act(() => {
      input.dispatchEvent(new FocusEvent("focusin", { bubbles: true }));
    });

    const listbox = container.querySelector<HTMLElement>("#address-suggestions")!;
    expect(input.getAttribute("aria-expanded")).toBe("true");
    expect(listbox.getAttribute("role")).toBe("listbox");
    expect(container.querySelectorAll('[role="option"]').length).toBeGreaterThan(0);
    const option = container.querySelector('[role="option"]');
    expect(option?.getAttribute("aria-selected")).toBe("true");
    expect(option?.hasAttribute("title")).toBe(false);
    expect(option?.querySelector(".omnibox-action-hints")?.getAttribute("aria-label")).toBe("Alt Preview, Shift Split");
    expect(option?.querySelector(".omnibox-action-hint")?.getAttribute("data-action-hint")).toBe("preview");
    expect(option?.querySelectorAll(".omnibox-action-hint")).toHaveLength(2);
    expect(option?.querySelector("kbd")).toBeNull();
    expect(input.getAttribute("aria-activedescendant")).toBe("address-suggestion-0");

    act(() => root.unmount());
    container.remove();
  });

  it("shows and accepts topbar inline completion", () => {
    const setAddressValue = vi.fn();
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(Topbar, {
        controller: createController({ addressValue: "git", compactMode: false, setAddressValue })
      }));
    });

    expect(container.querySelector(".address-autocomplete-suffix")?.textContent).toBe("hub.com");

    const input = container.querySelector<HTMLInputElement>("#addressInput")!;
    input.setSelectionRange(3, 3);
    act(() => {
      input.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "Tab"
      }));
    });

    expect(setAddressValue).toHaveBeenCalledWith("github.com");

    act(() => root.unmount());
    container.remove();
  });

  it("runs the accepted title completion as its suggestion instead of searching the title", () => {
    const actions = createActions();
    const state = createDefaultState();
    getActiveWorkspace(state).favorites.unshift(createFavorite("Linear Planning", "https://linear.example/"));
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(OmniboxHarness, {
        actions,
        initialAddressValue: "lin",
        state
      }));
    });

    const input = container.querySelector<HTMLInputElement>("#addressInput")!;
    input.setSelectionRange(3, 3);
    act(() => {
      input.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "Tab"
      }));
    });
    expect(container.querySelector<HTMLInputElement>("#addressInput")?.value).toBe("Linear Planning");

    act(() => {
      container.querySelector<HTMLInputElement>("#addressInput")?.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "Enter"
      }));
    });

    expect(actions.openUrlInActiveWorkspace).toHaveBeenCalledWith("https://linear.example/", "Linear Planning");
    expect(actions.navigateActiveTab).not.toHaveBeenCalledWith("Linear Planning");

    act(() => root.unmount());
    container.remove();
  });

});

function OmniboxHarness({
  actions,
  initialAddressValue,
  state
}: {
  actions: BrowserController["actions"];
  initialAddressValue: string;
  state: BrowserState;
}) {
  const [addressValue, setAddressValue] = useState(initialAddressValue);

  return createElement(Topbar, {
    controller: createController({
      actions,
      addressValue,
      compactMode: false,
      setAddressValue,
      state
    })
  });
}

function createController({
  actions = createActions(),
  addressValue,
  compactMode,
  setAddressValue = vi.fn(),
  state = createDefaultState()
}: {
  actions?: BrowserController["actions"];
  addressValue: string;
  compactMode: boolean;
  setAddressValue?: BrowserController["setAddressValue"];
  state?: BrowserState;
}): BrowserController {
  const activeWorkspace = getActiveWorkspace(state);
  const activeTab = getActiveTab(activeWorkspace);

  return {
    actions,
    activeTab,
    activeWorkspace,
    addressValue,
    compactMode,
    floatingToolbarOpen: false,
    setAddressValue,
    setPanel: vi.fn(),
    state
  } as unknown as BrowserController;
}

function createActions() {
  return {
    closeActiveTab: vi.fn(),
    navigateActiveTab: vi.fn(),
    openGlance: vi.fn(),
    openTabInSplit: vi.fn(),
    openUrlInActiveWorkspace: vi.fn(),
    openUrlInSplit: vi.fn(),
    resetActiveTabZoom: vi.fn(),
    runWebviewAction: vi.fn(),
    selectTab: vi.fn(),
    setSplitLayout: vi.fn(),
    toggleActiveTabEssential: vi.fn(),
    toggleActiveTabFavorite: vi.fn(),
    toggleActiveTabMuted: vi.fn(),
    toggleActiveTabPinned: vi.fn(),
    toggleFloatingToolbar: vi.fn(),
    zoomIn: vi.fn(),
    zoomOut: vi.fn()
  } as unknown as BrowserController["actions"];
}

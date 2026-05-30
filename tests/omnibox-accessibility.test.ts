import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { createDefaultState } from "../src/renderer/domain/browser";
import { getActiveTab, getActiveWorkspace } from "../src/renderer/domain/browser/selectors";
import { SidebarAddress } from "../src/renderer/surfaces/sidebar/components/chrome/SidebarAddress";
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
    expect(container.querySelector('[role="option"]')?.getAttribute("aria-selected")).toBe("true");
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

  it("exposes compact sidebar address suggestions with matching combobox semantics", () => {
    const actions = createActions();
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarAddress, {
        controller: createController({ actions, addressValue: "github", compactMode: true })
      }));
    });

    const input = container.querySelector<HTMLInputElement>("#sidebarAddressInput")!;
    expect(input.getAttribute("role")).toBe("combobox");
    expect(input.getAttribute("aria-autocomplete")).toBe("both");
    expect(input.getAttribute("aria-controls")).toBe("sidebar-address-suggestions");
    expect(input.getAttribute("aria-expanded")).toBe("false");

    act(() => {
      input.dispatchEvent(new FocusEvent("focusin", { bubbles: true }));
    });

    expect(input.getAttribute("aria-expanded")).toBe("true");
    expect(container.querySelector("#sidebar-address-suggestions")?.getAttribute("role")).toBe("listbox");
    const option = container.querySelector('[role="option"]');
    expect(option?.id).toBe("sidebar-address-suggestion-0");
    expect(option?.hasAttribute("title")).toBe(false);
    expect(option?.querySelector(".omnibox-action-hints")?.getAttribute("aria-label")).toBe("Alt Split");
    expect(option?.querySelector(".omnibox-action-hint")?.getAttribute("data-action-hint")).toBe("split");
    expect(option?.querySelector("kbd")).toBeNull();

    act(() => {
      input.dispatchEvent(new KeyboardEvent("keydown", {
        altKey: true,
        bubbles: true,
        key: "Enter"
      }));
    });

    expect(actions.openUrlInSplit).toHaveBeenCalled();

    act(() => root.unmount());
    container.remove();
  });
});

function createController({
  actions = createActions(),
  addressValue,
  compactMode,
  setAddressValue = vi.fn()
}: {
  actions?: BrowserController["actions"];
  addressValue: string;
  compactMode: boolean;
  setAddressValue?: BrowserController["setAddressValue"];
}): BrowserController {
  const state = createDefaultState();
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

import type { WebviewAction } from "../../../types/browser-ui";

export interface ReloadButtonState {
  action: WebviewAction;
  label: string;
}

export function getReloadButtonState(isLoading: boolean): ReloadButtonState {
  return isLoading
    ? { action: "stop", label: "Stop loading" }
    : { action: "reload", label: "Reload" };
}

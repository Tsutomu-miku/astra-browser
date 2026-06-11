import { forwardRef, useRef, type ForwardedRef } from "react";

import { useBrowserStore } from "../../../stores/browserStore";
import type { WebviewElement } from "../../../types/browser-ui";

export const GlanceWebview = forwardRef<WebviewElement, {
  partition: string;
  url: string;
}>(({ partition, url }, ref) => {
  const localRef = useRef<WebviewElement | null>(null);
  const autofillBridgePath = useBrowserStore((s) => s.autofillBridgePath);

  return (
    <webview
      key={url}
      ref={(webview) => {
        localRef.current = webview;
        setForwardedRef(ref, webview);
      }}
      allowpopups
      className="glance-webview"
      src={url}
      partition={partition}
      preload={autofillBridgePath || undefined}
    />
  );
});

GlanceWebview.displayName = "GlanceWebview";

function setForwardedRef(ref: ForwardedRef<WebviewElement>, value: WebviewElement | null) {
  if (typeof ref === "function") {
    ref(value);
  } else if (ref) {
    ref.current = value;
  }
}

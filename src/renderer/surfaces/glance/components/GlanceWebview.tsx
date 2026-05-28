import { forwardRef, useEffect, useRef, type ForwardedRef } from "react";

import type { WebviewElement } from "../../../types/browser-ui";

export const GlanceWebview = forwardRef<WebviewElement, {
  partition: string;
  url: string;
}>(({ partition, url }, ref) => {
  const localRef = useRef<WebviewElement | null>(null);

  useEffect(() => {
    localRef.current?.setAttribute("allowpopups", "true");
  }, []);

  return (
    <webview
      key={url}
      ref={(webview) => {
        localRef.current = webview;
        setForwardedRef(ref, webview);
      }}
      className="glance-webview"
      src={url}
      partition={partition}
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

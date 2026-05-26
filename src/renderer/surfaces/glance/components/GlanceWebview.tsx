import { forwardRef } from "react";

import type { WebviewElement } from "../../../types/browser-ui";

export const GlanceWebview = forwardRef<WebviewElement, {
  partition: string;
  url: string;
}>(({ partition, url }, ref) => (
  <webview
    key={url}
    ref={ref}
    className="glance-webview"
    src={url}
    partition={partition}
    allowpopups
  />
));

GlanceWebview.displayName = "GlanceWebview";

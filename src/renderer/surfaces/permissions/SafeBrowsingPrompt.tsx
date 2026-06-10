import type { SafeBrowsingCheckResult } from "../../types/electron";

interface SafeBrowsingAlertProps {
  alert: NonNullable<{
    url: string;
    reason?: string;
    severity?: "low" | "medium" | "high";
    proceedCallback?: () => void;
  }>;
  onProceed: () => void;
  onGoBack: () => void;
}

/**
 * Safe Browsing 拦截面板（M2.3 K-6 / D-3）。
 *
 * 当 main 端 safe-browsing:check-navigation 返回 block 时，browserStore
 * 会把 alert 写入 state.safeBrowsingAlert，App 根组件根据该字段渲染此面板。
 */
export function SafeBrowsingPrompt({ alert, onProceed, onGoBack }: SafeBrowsingAlertProps) {
  const severityLabel =
    alert.severity === "high" ? "Dangerous" :
    alert.severity === "medium" ? "Suspicious" : "Caution";
  const severityClass = `safe-browsing severity-${alert.severity ?? "medium"}`;
  const details = describeReason(alert.reason ?? "unknown");

  return (
    <div className="permission-prompt" role="alertdialog" aria-label="Safe Browsing warning">
      <div className={severityClass}>
        <header>
          <h2>{severityLabel}</h2>
          <p className="muted">{details.title}</p>
        </header>
        <div className="permission-targets">
          <label className="field">
            <span>Site</span>
            <code>{alert.url}</code>
          </label>
          <label className="field">
            <span>Threat</span>
            <code>{alert.reason}</code>
          </label>
        </div>
        <p className="muted">{details.body}</p>
        <div className="permission-actions">
          <button className="danger" type="button" onClick={onGoBack}>
            Back to safety
          </button>
          <button className="normal" type="button" onClick={() => { alert.proceedCallback?.(); onProceed(); }}>
            Proceed anyway (one-time)
          </button>
        </div>
      </div>
    </div>
  );
}

function describeReason(reason: SafeBrowsingCheckResult["reason"]): { title: string; body: string } {
  switch (reason) {
    case "blacklisted-domain":
    case "blacklisted-suffix":
      return {
        title: "This site is on our unsafe list.",
        body: "It has been flagged for distributing malware, phishing, or unwanted software. You should not enter any credentials or download files here."
      };
    case "remote-lookup":
      return {
        title: "An external safe-browsing service flagged this site.",
        body: "A known malicious pattern was detected. Unless you are absolutely certain this is a false positive, close this page immediately."
      };
    case "double-extension":
      return {
        title: "This file has a suspicious double extension.",
        body: "Files like `invoice.pdf.exe` try to disguise their real type. Opening it could install malware."
      };
    case "dangerous-extension":
      return {
        title: "This file type can run code on your device.",
        body: "Executable / script files from untrusted sources should never be opened unless you initiated the download intentionally."
      };
    default:
      return {
        title: "This page or file was flagged as unsafe.",
        body: "Proceed only if you trust the source and understand the risks."
      };
  }
}

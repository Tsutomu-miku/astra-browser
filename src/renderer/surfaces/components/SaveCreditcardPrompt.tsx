/**
 * P-3: 站点提交包含 CC 字段的表单后弹出 "是否保存卡" prompt。
 *
 * - show 时：从 detail 里拿卡号/过期日，做 Luhn + expiry 校验
 *   （校验失败直接 reject，不打扰用户）
 * - Save：调用 acceptSaveCreditcard() → 加密 + 写入 vault
 * - Never for this site / Not now （Not now 用 reject 实现）
 */
import { useEffect, useState } from "react";

import { FiCreditCard, FiShield, FiX } from "react-icons/fi";

import type { BrowserController } from "../../app/controller/types";
import {
  detectCardBrand,
  lastFourOf
} from "../../domain/browser/paymentCardUtils";

const BRAND_LABEL: Record<string, string> = {
  amex: "Amex",
  diners: "Diners Club",
  discover: "Discover",
  jcb: "JCB",
  mastercard: "Mastercard",
  other: "Card",
  unionpay: "UnionPay",
  visa: "Visa"
};

export function SaveCreditcardPrompt({ controller }: { controller: BrowserController }) {
  const prompt = controller.actions.saveCreditcardPrompt;
  const [error, setError] = useState<string | null>(null);
  const [busy, setBusy] = useState(false);

  useEffect(() => {
    if (!prompt) return;
    setError(null);
    setBusy(false);
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Escape") controller.actions.rejectSaveCreditcard();
    };
    document.addEventListener("keydown", onKey);
    return () => document.removeEventListener("keydown", onKey);
  }, [prompt, controller.actions]);

  if (!prompt?.detail) return null;
  const d = prompt.detail;
  if (!d.number || !d.host) return null;

  const brand = detectCardBrand(d.number);
  const lastFour = lastFourOf(d.number) || "XXXX";

  const doSave = async () => {
    setBusy(true);
    setError(null);
    try {
      const msg = await controller.actions.acceptSaveCreditcard({});
      if (msg) setError(msg);
    } catch (err) {
      setError(err instanceof Error ? err.message : String(err));
    } finally {
      setBusy(false);
    }
  };

  return (
    <aside
      role="dialog"
      aria-label="Save payment method"
      className="permission-prompt save-creditcard-prompt permission-prompt-card"
    >
      <div className="permission-copy">
        <span className="permission-origin">
          <FiCreditCard aria-hidden /> <strong>{BRAND_LABEL[brand] || "Card"}</strong>
          {"  "}
          <FiShield aria-hidden /> {d.host}
        </span>
        <span className="permission-message">
          保存这张卡？<br />
          <strong className="creditcard-preview">
            •••• {lastFour}
            {d.cardholderName ? ` · ${d.cardholderName}` : ""}
            {d.expiryMonth && d.expiryYear
              ? ` · ${String(d.expiryMonth).padStart(2, "0")}/${String(d.expiryYear).slice(-2)}`
              : d.expiryRaw ? ` · ${d.expiryRaw}` : ""}
          </strong>
          <br />
          <small className="muted">
            完整卡号 + CVV 使用 AES-256-GCM 加密，PCI 合规存储。
          </small>
        </span>
        {error ? <p className="muted password-editor-error">{error}</p> : null}
      </div>
      <div className="permission-actions permission-actions-col">
        <div className="permission-actions">
          <button
            className="toolbar-button"
            type="button"
            onClick={() => controller.actions.rejectSaveCreditcard()}
            disabled={busy}
          >
            Not now
          </button>
          <button
            className="toolbar-button primary-action"
            type="button"
            disabled={busy}
            onClick={() => void doSave()}
          >
            {busy ? "Encrypting…" : "Save card"}
          </button>
        </div>
        <button
          className="icon-button permission-dismiss"
          type="button"
          aria-label="Dismiss"
          title="Dismiss"
          disabled={busy}
          onClick={() => controller.actions.rejectSaveCreditcard()}
        >
          <FiX />
        </button>
      </div>
    </aside>
  );
}

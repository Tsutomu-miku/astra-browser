/* eslint-disable max-lines */
/**
 * P-3 支付方式编辑对话框：完整卡信息录入 + BIN 识别 + Luhn 校验 + AES-256-GCM 加密。
 *
 * 表单字段：
 *   - Cardholder name（持卡人姓名）
 *   - Card number（完整卡号 / 13-19 位，输入时按品牌规则自动格式化）
 *   - Expiry (MM / YYYY)
 *   - CVV / CVC（3-4 位，按品牌校验长度）
 *   - Label（可选，默认 "持卡人 ••XXXX"）
 *
 * 保存逻辑：
 *   1) 校验 pan (Luhn + 品牌 + 长度)、cvc 长度、expiry 有效性
 *   2) vault 解锁 → 加密 {pan, csc, brand} → encryptedCardDetails
 *   3) 提取 cardLastFour + expiryMonth/expiryYear + cardholderName + label
 *   4) 调用 onSave(PaymentMethodEntry)，由调用方写入 store/settings
 */
import { useEffect, useMemo, useState } from "react";
import { FiCreditCard, FiEye, FiEyeOff, FiLock, FiShield, FiX } from "react-icons/fi";

import type { PaymentMethodEntry } from "../../../../../domain/browser";
import {
  detectCardBrand,
  encryptCardDetails,
  expectedCscLength,
  formatCardNumber,
  isValidExpiry,
  isValidPan,
  lastFourOf,
  type CardBrand
} from "../../../../../domain/browser/paymentCardUtils";
import { createId } from "../../../../../domain/browser/factory";
import { Field } from "../shared/SettingsUIPrimitives";

const BRAND_LABEL: Record<CardBrand, string> = {
  amex: "American Express",
  diners: "Diners Club",
  discover: "Discover",
  jcb: "JCB",
  mastercard: "Mastercard",
  other: "Card",
  unionpay: "UnionPay",
  visa: "Visa"
};

export interface PaymentMethodEditorDialogProps {
  entry: Partial<PaymentMethodEntry> | null;
  onClose: () => void;
  /**
   * 保存回调，返回值：
   *   - 成功：void / null / 空 string
   *   - 失败：string 错误信息
   */
  onSave: (entry: PaymentMethodEntry) => Promise<string | null | void>;
  /**
   * 必须先解锁 vault，否则 PAN 无法加密。
   * 如果 locked，对话框展示 "先解锁 vault" 提示。
   */
  vaultUnlocked: boolean;
  onRequestUnlock: () => Promise<void> | void;
}

export function PaymentMethodEditorDialog({
  entry,
  onClose,
  onSave,
  vaultUnlocked,
  onRequestUnlock
}: PaymentMethodEditorDialogProps) {
  const isNew = !entry?.id;

  const [cardholderName, setCardholderName] = useState(entry?.cardholderName ?? "");
  const [rawPan, setRawPan] = useState(""); // 用户原始输入（含空格）
  const [expiryMm, setExpiryMm] = useState<string>(
    entry?.expiryMonth ? String(entry.expiryMonth).padStart(2, "0") : ""
  );
  const [expiryYy, setExpiryYy] = useState<string>(
    entry?.expiryYear != null ? String(entry.expiryYear).slice(-4) : ""
  );
  const [csc, setCsc] = useState("");
  const [showCsc, setShowCsc] = useState(false);
  const [label, setLabel] = useState(entry?.label ?? "");
  const [error, setError] = useState<string | null>(null);
  const [busy, setBusy] = useState(false);
  const [localUnlockBusy, setLocalUnlockBusy] = useState(false);

  useEffect(() => {
    const onKey = (event: KeyboardEvent) => {
      if (event.key === "Escape") onClose();
    };
    document.addEventListener("keydown", onKey);
    return () => document.removeEventListener("keydown", onKey);
  }, [onClose]);

  const brand = useMemo(() => detectCardBrand(rawPan), [rawPan]);
  const formattedPan = useMemo(() => formatCardNumber(rawPan, brand), [rawPan, brand]);
  const panCheck = useMemo(() => {
    if (!rawPan) return { valid: false, reason: "empty" as const };
    return isValidPan(rawPan);
  }, [rawPan]);
  const cscLen = expectedCscLength(brand);
  const cscOk = csc.length === 0 || csc.length === cscLen;

  const mmNum = expiryMm ? Number(expiryMm) : undefined;
  const yyNum = expiryYy ? (expiryYy.length === 2 ? 2000 + Number(expiryYy) : Number(expiryYy)) : undefined;
  const expOk = expiryMm === "" && expiryYy === "" ? true : isValidExpiry(mmNum, yyNum);

  const cardholderOk = cardholderName.trim().length > 0;

  /** 用户在 pan input 输入时：仅保留数字，重新格式化后展示，但不存到 state。 */
  const onPanInput = (e: React.ChangeEvent<HTMLInputElement>) => {
    const digits = e.target.value.replace(/\D/g, "");
    setRawPan(digits.slice(0, 19));
  };
  const onMmInput = (e: React.ChangeEvent<HTMLInputElement>) => {
    const d = e.target.value.replace(/\D/g, "").slice(0, 2);
    setExpiryMm(d);
  };
  const onYyInput = (e: React.ChangeEvent<HTMLInputElement>) => {
    const d = e.target.value.replace(/\D/g, "").slice(0, 4);
    setExpiryYy(d);
  };
  const onCscInput = (e: React.ChangeEvent<HTMLInputElement>) => {
    setCsc(e.target.value.replace(/\D/g, "").slice(0, 4));
  };

  const submit = async () => {
    setError(null);
    if (!vaultUnlocked) {
      setError("请先解锁密码库（vault）后再保存完整卡号。");
      return;
    }
    if (!cardholderOk) { setError("请填写持卡人姓名。"); return; }
    if (!panCheck.valid) {
      const reason = panCheck.reason === "luhn-failed"
        ? "卡号校验位不通过（Luhn check failed）。"
        : panCheck.reason === "empty"
          ? "请填写卡号。"
          : `${BRAND_LABEL[brand] ?? "该卡"} 卡号长度不正确：${panCheck.reason}.`;
      setError(reason);
      return;
    }
    if (!cscOk) { setError(`CVV/CVC 应为 ${cscLen} 位。`); return; }
    if (!expOk) { setError("有效期无效（应为未过期的未来月份，格式 MM / YYYY）。"); return; }

    const pan = rawPan.replace(/\D/g, "");
    const entryId = entry?.id || createId();
    const lastFour = lastFourOf(pan);
    setBusy(true);
    try {
      const encrypted = await encryptCardDetails({
        pan,
        csc: csc || undefined,
        brand
      });
      const finalLabel = label.trim() || `${cardholderName.trim()} ••${lastFour}`;
      const saved: PaymentMethodEntry = {
        id: entryId,
        label: finalLabel,
        cardholderName: cardholderName.trim(),
        cardLastFour: lastFour,
        encryptedCardDetails: encrypted,
        expiryMonth: mmNum,
        expiryYear: yyNum,
        createdAt: entry?.createdAt ?? Date.now(),
        updatedAt: Date.now()
      };
      const msg = await onSave(saved);
      if (typeof msg === "string") setError(msg);
    } catch (err) {
      setError(err instanceof Error ? err.message : String(err));
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="password-editor-backdrop" role="presentation" onClick={onClose}>
      <div
        className="password-editor-dialog"
        role="dialog"
        aria-modal="true"
        aria-label={isNew ? "Add payment method" : "Edit payment method"}
        onClick={(event) => event.stopPropagation()}
      >
        <header className="password-editor-header">
          <h3><FiCreditCard aria-hidden /> {isNew ? "Add payment method" : "Edit payment method"}</h3>
          <button className="icon-button" type="button" aria-label="Close dialog" onClick={onClose}>
            <FiX />
          </button>
        </header>
        <div className="password-editor-body">
          {!vaultUnlocked && (
            <div className="vault-locked-hint">
              <FiLock aria-hidden />
              <div>
                <strong>密码库已锁定。</strong>
                <span>PCI 合规要求：完整卡号与 CVV 必须使用 vault 密钥加密，保存前请先解锁。</span>
              </div>
              <button
                className="toolbar-button primary-action"
                type="button"
                onClick={async () => {
                  setLocalUnlockBusy(true);
                  try { await onRequestUnlock(); } finally { setLocalUnlockBusy(false); }
                }}
                disabled={localUnlockBusy}
              >
                {localUnlockBusy ? "Unlocking…" : "Unlock vault"}
              </button>
            </div>
          )}
          <Field label="Cardholder name">
            <input
              type="text"
              value={cardholderName}
              onChange={(e) => setCardholderName(e.target.value)}
              placeholder="JOHN DOE"
              autoFocus
              autoComplete="cc-name"
            />
          </Field>
          <Field
            label={`Card number · ${BRAND_LABEL[brand] || "Unknown"}`}
            hint="Luhn + brand-length validation; encrypted with vault AES-256-GCM."
          >
            <input
              type="text"
              inputMode="numeric"
              value={formattedPan}
              onChange={onPanInput}
              placeholder={brand === "amex" ? "3782 822463 10005" : "4242 4242 4242 4242"}
              autoComplete="cc-number"
              aria-invalid={rawPan.length > 0 && !panCheck.valid}
            />
          </Field>
          <div className="field-inline">
            <Field label="MM / YYYY">
              <div className="field-inline">
                <input
                  type="text"
                  inputMode="numeric"
                  value={expiryMm}
                  onChange={onMmInput}
                  placeholder="MM"
                  autoComplete="cc-exp-month"
                  aria-invalid={!expOk}
                />
                <span className="muted" aria-hidden>/</span>
                <input
                  type="text"
                  inputMode="numeric"
                  value={expiryYy}
                  onChange={onYyInput}
                  placeholder="YYYY"
                  autoComplete="cc-exp-year"
                  aria-invalid={!expOk}
                />
              </div>
            </Field>
            <Field label={`CVV/CVC (${cscLen} digits)`}>
              <div className="field-inline">
                <input
                  type={showCsc ? "text" : "password"}
                  inputMode="numeric"
                  value={csc}
                  onChange={onCscInput}
                  placeholder={"•".repeat(cscLen)}
                  autoComplete="cc-csc"
                  aria-invalid={!cscOk && csc.length > 0}
                />
                <button
                  className="icon-button"
                  type="button"
                  aria-label={showCsc ? "Hide CVV" : "Show CVV"}
                  title={showCsc ? "Hide CVV" : "Show CVV"}
                  onClick={() => setShowCsc((p) => !p)}
                >
                  {showCsc ? <FiEyeOff /> : <FiEye />}
                </button>
              </div>
            </Field>
          </div>
          <Field label="Label (optional)" hint={`默认 "${cardholderName.trim() || "Card"} ••${lastFourOf(rawPan) || "XXXX"}"`}>
            <input
              type="text"
              value={label}
              onChange={(e) => setLabel(e.target.value)}
              placeholder="My Visa / Work Amex / Backup card"
            />
          </Field>
          {error ? <p className="muted password-editor-error">{error}</p> : null}
        </div>
        <footer className="password-editor-footer">
          <button className="toolbar-button" type="button" onClick={onClose} disabled={busy}>
            Cancel
          </button>
          <button
            className="toolbar-button primary-action"
            type="button"
            onClick={submit}
            disabled={busy || localUnlockBusy}
          >
            {busy ? "Encrypting & saving…" : (isNew ? "Add card" : "Save changes")}
          </button>
        </footer>
      </div>
    </div>
  );
}

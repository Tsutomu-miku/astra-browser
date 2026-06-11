/**
 * P-3 PaymentRequest: 当站点触发 new PaymentRequest().show() 且 supportedMethods 包含
 * "basic-card" 时，渲染侧弹出卡片选择器（代替原生 Payment Sheet，保持 UI 一致）。
 *
 *  - 顶部显示站点 + 订单金额（若 PaymentRequest.total 可用）
 *  - 卡片列表复用 autofill 的 matches 列表 UI（• •••• LastFour · MM/YY）
 *  - "Use native payment sheet" = cancel，走 Chromium 原生 UI
 */
import { useEffect, useRef } from "react";
import {
  FiCreditCard,
  FiShield,
  FiX
} from "react-icons/fi";

import type { BrowserController } from "../../app/controller/types";
import { lastFourOf } from "../../domain/browser/paymentCardUtils";

export function PaymentRequestPicker({ controller }: { controller: BrowserController }) {
  const prompt = controller.actions.paymentRequestPrompt;
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!prompt) return;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Escape") controller.actions.rejectPaymentRequest();
    };
    document.addEventListener("keydown", onKey);
    return () => document.removeEventListener("keydown", onKey);
  }, [prompt, controller.actions]);

  if (!prompt) return null;

  const host = prompt.detail.host;
  const total = prompt.detail.total;
  const amountText = total ? `${total.amount.currency} ${total.amount.value}` : null;
  const totalLabel = total?.label ? total.label : "Amount";

  return (
    <div
      className="payment-request-backdrop"
      role="presentation"
      onClick={() => controller.actions.rejectPaymentRequest()}
    >
      <aside
        ref={ref}
        role="dialog"
        aria-modal="true"
        aria-label="Select a card to pay"
        className="payment-request"
        onClick={(e) => e.stopPropagation()}
      >
        <header className="payment-request__header">
          <div className="payment-request__header-title">
            <FiShield aria-hidden />
            <div>
              <strong>{host}</strong>
              <span>要求选择支付方式</span>
            </div>
          </div>
          <button
            className="icon-button"
            type="button"
            aria-label="Cancel"
            title="Cancel"
            onClick={() => controller.actions.rejectPaymentRequest()}
          >
            <FiX />
          </button>
        </header>

        {amountText && (
          <div className="payment-request__amount">
            <span>{totalLabel}</span>
            <strong>{amountText}</strong>
          </div>
        )}

        <ul className="payment-request__list" aria-label="Saved cards">
          {prompt.candidateCards.length === 0 && (
            <li className="payment-request__empty">
              该 vault 中没有已保存的卡。您可以选择：
              <ul>
                <li>使用原生 Payment Sheet 添加新卡；</li>
                <li>或之后在 Settings → Autofill 中手动录入。</li>
              </ul>
            </li>
          )}
          {prompt.candidateCards.map((c) => {
            const lastFour = c.cardLastFour || lastFourOf("");
            return (
              <li key={c.id}>
                <button
                  className="payment-request__card"
                  type="button"
                  onClick={() => void controller.actions.acceptPaymentRequestCard(c.id)}
                >
                  <span className="payment-request__card-icon">
                    <FiCreditCard />
                  </span>
                  <span className="payment-request__card-body">
                    <strong>{c.label}</strong>
                    <small>
                      {c.cardholderName || "Card"}
                      {lastFour ? ` • •••• ${lastFour}` : ""}
                      {c.subtitle ? ` · ${c.subtitle.split("·").slice(1).join("·").trim() || ""}` : ""}
                    </small>
                  </span>
                  <span className="payment-request__card-cta">Pay</span>
                </button>
              </li>
            );
          })}
        </ul>

        <footer className="payment-request__footer">
          <button
            className="toolbar-button"
            type="button"
            onClick={() => controller.actions.rejectPaymentRequest()}
          >
            Use browser&apos;s native payment sheet
          </button>
        </footer>
      </aside>
    </div>
  );
}

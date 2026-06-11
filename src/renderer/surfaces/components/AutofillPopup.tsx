/**
 * P-2 地址 / 信用卡自动填充浮动弹窗。
 *
 * 由 autofill:field-focus IPC 打开，附着在 topbar 下方（精确坐标来自
 * webview DOM 比较困难，这里使用"跟 active webview 顶部对齐"的简化定位）。
 *
 * 展示：
 *   - 头部 pill：显示当前聚焦字段的 label + host 名
 *   - 匹配列表：地址库/卡库中能命中至少一个 form 字段的条目
 *   - 底部动作：保存当前表单为新地址（当 focusedBucket=address 时显示）
 */
import { useEffect, useRef } from "react";

import {
  FiCreditCard,
  FiMapPin,
  FiPlus,
  FiSave,
  FiX
} from "react-icons/fi";
import type { BrowserController } from "../../app/controller/types";

export function AutofillPopup({ controller }: { controller: BrowserController }) {
  const prompt = controller.actions.autofillPrompt;
  const ref = useRef<HTMLDivElement>(null);

  // 点击外部 / Esc 关闭
  useEffect(() => {
    if (!prompt) return;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Escape") controller.actions.hideAutofillPopup();
    };
    const onClick = (e: MouseEvent) => {
      if (!ref.current) return;
      if (!ref.current.contains(e.target as Node)) {
        controller.actions.hideAutofillPopup();
      }
    };
    document.addEventListener("keydown", onKey);
    setTimeout(() => document.addEventListener("mousedown", onClick), 0);
    return () => {
      document.removeEventListener("keydown", onKey);
      document.removeEventListener("mousedown", onClick);
    };
  }, [prompt, controller.actions]);

  if (!prompt) return null;

  const bucket = prompt.detail.focusedBucket;
  const BucketIcon = bucket === "creditcard" ? FiCreditCard : FiMapPin;
  const matches = prompt.matches;

  return (
    <div
      ref={ref}
      className="autofill-popup"
      role="dialog"
      aria-label={`Autofill ${bucket} suggestions`}
    >
      <header className="autofill-popup__header">
        <span className="autofill-popup__pill">
          <BucketIcon aria-hidden />
          <span>{prompt.detail.focusedLabel} · {prompt.detail.host}</span>
        </span>
        <button
          className="icon-button"
          type="button"
          title="Dismiss"
          aria-label="Dismiss autofill"
          onClick={() => controller.actions.hideAutofillPopup()}
        >
          <FiX />
        </button>
      </header>

      <ul className="autofill-popup__list">
        {matches.length === 0 && (
          <li className="autofill-popup__empty">
            没有匹配的{bucket === "creditcard" ? "支付方式" : "地址"}。
          </li>
        )}
        {matches.map((m) => (
          <li key={m.id}>
            <button
              type="button"
              className="autofill-popup__item"
              onClick={() => void controller.actions.acceptAutofillMatch(m.id)}
            >
              <span className="autofill-popup__item-icon">
                {bucket === "creditcard" ? <FiCreditCard /> : <FiMapPin />}
              </span>
              <span className="autofill-popup__item-body">
                <strong>{m.label}</strong>
                {m.subtitle && <small>{m.subtitle}</small>}
              </span>
            </button>
          </li>
        ))}
      </ul>

      {prompt.offerSaveAddress && (
        <footer className="autofill-popup__footer">
          <button
            type="button"
            className="autofill-popup__save"
            onClick={() => {
              // 简化 MVP：从 prompt.fields 反推当前 form 里已有的值做 seed，
              // 然后走 Settings 保存流程。这里仅 open Settings + autofill section
              // 并自动 upsert 一个空壳（用户随后补全）。
              const seeded = buildAddressFromFieldHints(prompt.detail.fields, prompt.detail.host);
              controller.actions.saveCurrentFormAsAddress(seeded);
              if (typeof controller.setPanel === "function") controller.setPanel("settings");
            }}
          >
            <FiPlus /> <FiSave /> <span>保存当前表单为新地址</span>
          </button>
        </footer>
      )}
    </div>
  );
}

/**
 * 当前 form 中所有 field type 的值并不知道（webview DOM 值没传回主进程）。
 * 这里只是生成一个占位 entry，用户打开设置后再填具体字段。
 * 未来扩展：让 shim 在 field-focus 时把当前值也 encode 进 detail。
 */
function buildAddressFromFieldHints(
  fields: Array<{ type: string; label: string }>,
  host: string
): Partial<import("../../domain/browser").AddressEntry> {
  const now = Date.now();
  return {
    label: `Saved from ${host || "web"} (${new Date(now).toLocaleDateString()})`,
    recipient: "",
    address1: "",
    city: "",
    // 写入注释型字段，方便用户之后识别
    createdAt: now,
    // 把当前 form 能识别的 field type 记在 notes-like 字段里（address2）
    address2: `Fields: ${fields.map((f) => f.label || f.type).join(", ")}`
  };
}

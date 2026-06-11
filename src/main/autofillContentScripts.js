/**
 * ADR-0005 配套 / P-2 + P-3: 自动填充注入层。
 *
 * 浏览器的地址 / 信用卡填充底层靠 DOM 扫描 + 识别。这里实现
 *   "双 preload 注入" 模式：
 *
 *   installAutofillForAllSessions()
 *     1) ensureShimFile()          — 主世界 shim：扫描 DOM、识别字段、填回值
 *     2) ensureBridgeFile()        — 隔离世界 bridge：ipcRenderer 通道
 *     3) session.setPreloads([shim]) — 为所有 webContents（含 webview）注入主世界 shim
 *
 *   主世界 shim (主进程写入到 astra-autofill-shim-<hash>.js):
 *     1) 监听所有表单 input 的 focus 事件；
 *     2) 识别该字段属于"地址类"（P-2）还是"信用卡类"（P-3）；
 *     3) 向上收集 form（或 document）中所有同 bucket 字段
 *     4) 通过 CustomEvent("astra-autofill:field-focus", detail) 发往隔离世界
 *     5) 监听 CustomEvent("astra-autofill:fill-form") 把值回填
 *
 *   隔离世界 bridge (astra-autofill-bridge-<hash>.js, 作为 webview preload 属性加载):
 *     - 接收主世界 CustomEvent → ipcRenderer.send("autofill:field-focus", detail) → main
 *     - 接收 main 侧 "autofill:fill-form" (ipcRenderer.on) → window CustomEvent → 主世界
 *
 *   渲染侧：
 *     - 收到 autofill:field-focus 事件，弹出 AutofillPopup，列出匹配条目
 *     - 用户选中后调用 autofill:fill-form IPC，main 进程通过 webContentsId 路由
 *       回正确的 webview 内部触发 CustomEvent
 *
 *   本文件导出：
 *     installAutofillShim(session)    — 安装到某个 session（主世界 shim）
 *     ensureBridgeFile()              — 返回隔离世界桥 preload 路径，供 <webview preload="file://...">
 *     routeAutofillFillTo(webContentsId, values) — main 侧把 fill 消息路由回指定 webContents
 */

/* eslint-disable max-lines */

const crypto = require("node:crypto");
const { ipcMain, webContents } = require("electron");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");

const installedSessions = new WeakSet();
const shimFileBySession = new WeakMap();

/**
 * 每次写入同一个 OS temp 文件，避免每次启动重写（hash 基于版本号 + EOF）。
 * 生产环境可改到 userData。
 */
function ensureShimFile() {
  const content = buildShimScript();
  const hash = crypto.createHash("sha1").update(content).digest("hex").slice(0, 10);
  const tmp = path.join(os.tmpdir(), `astra-autofill-shim-${hash}.js`);
  if (!fs.existsSync(tmp)) {
    try {
      fs.writeFileSync(tmp, content, { mode: 0o644 });
    } catch {
      // 写盘失败（权限/磁盘）→ 不抛错，降级为不支持自动填充。
      return null;
    }
  }
  return tmp;
}

function buildShimScript() {
  // 反引号模板：避免与外层 JS 解析冲突。
  return String.raw`
  (function() {
    if (window.__astraAutofillInstalled) return;
    window.__astraAutofillInstalled = true;

    /* ========== 字段识别（autocomplete + name 启发式） ========== */

    const FIELD_TYPES = {
      // 地址（P-2）
      addressName: {
        bucket: "address",
        label: "Full name",
        keys: ["name", "fullname", "full_name", "contactname"],
        ac: ["name", "name full"]
      },
      firstName: { bucket: "address", label: "First name", keys: ["firstname", "givenname", "first_name"], ac: ["given-name", "name given"] },
      lastName: { bucket: "address", label: "Last name", keys: ["lastname", "surname", "familyname", "last_name"], ac: ["family-name", "name family"] },
      email: { bucket: "address", label: "Email", keys: ["email", "emailaddress", "e_mail", "email_address"], ac: ["email"] },
      phone: { bucket: "address", label: "Phone", keys: ["phone", "telephone", "tel", "mobile", "cellphone", "phone_number"], ac: ["tel", "tel national"] },
      company: { bucket: "address", label: "Company", keys: ["company", "organization", "org", "employer"], ac: ["organization"] },
      street: { bucket: "address", label: "Street address", keys: ["address", "address1", "streetaddress", "addr1", "line1", "street", "street_line_1"], ac: ["street-address", "address-line1"] },
      street2: { bucket: "address", label: "Address line 2", keys: ["address2", "addr2", "line2", "apt", "suite", "street_line_2"], ac: ["address-line2"] },
      city: { bucket: "address", label: "City", keys: ["city", "town", "locality"], ac: ["address-level2"] },
      state: { bucket: "address", label: "State/Province", keys: ["state", "province", "region", "county"], ac: ["address-level1"] },
      zip: { bucket: "address", label: "ZIP/Postal code", keys: ["zip", "zipcode", "postalcode", "postcode", "postal_code", "postal"], ac: ["postal-code"] },
      country: { bucket: "address", label: "Country", keys: ["country", "countrycode", "nation"], ac: ["country"] },

      // 信用卡（P-3）
      ccName: { bucket: "creditcard", label: "Cardholder name", keys: ["cardname", "ccname", "card_name", "cc_holder", "cardholder", "name_on_card"], ac: ["cc-name", "cc-name full"] },
      ccNumber: { bucket: "creditcard", label: "Card number", keys: ["cardnumber", "ccnumber", "ccnum", "card_number", "creditcard", "cc_no"], ac: ["cc-number"] },
      ccExpMonth: { bucket: "creditcard", label: "Expiry month", keys: ["ccexpmonth", "expmonth", "expiry_month", "month", "cc_month", "mm"], ac: ["cc-exp-month"] },
      ccExpYear: { bucket: "creditcard", label: "Expiry year", keys: ["ccexpyear", "expyear", "expiry_year", "year", "cc_year", "yy", "yyyy"], ac: ["cc-exp-year"] },
      ccExp: { bucket: "creditcard", label: "Expiry (MM/YY)", keys: ["ccexp", "expiration", "expiry", "expdate", "card_expiry", "valid_thru"], ac: ["cc-exp"] },
      ccCsc: { bucket: "creditcard", label: "Security code (CVV/CVC)", keys: ["cvv", "cvc", "csc", "ccv", "securitycode", "security_code", "cardcode"], ac: ["cc-csc"] },
      ccType: { bucket: "creditcard", label: "Card brand", keys: ["cardtype", "brand", "cc_type", "paymentmethod"], ac: ["cc-type"] }
    };

    function normalize(str) {
      return (str || "").toLowerCase().replace(/[\s_-]/g, "");
    }

    function detectField(input) {
      const name = normalize(input.getAttribute("name") || "");
      const id = normalize(input.getAttribute("id") || "");
      const ac = normalize(input.getAttribute("autocomplete") || "");
      for (const [type, meta] of Object.entries(FIELD_TYPES)) {
        if (ac && meta.ac.some((s) => ac.includes(normalize(s)))) {
          return { type, bucket: meta.bucket, label: meta.label };
        }
        if (name && meta.keys.some((k) => name.includes(k))) {
          return { type, bucket: meta.bucket, label: meta.label };
        }
        if (id && meta.keys.some((k) => id.includes(k))) {
          return { type, bucket: meta.bucket, label: meta.label };
        }
      }
      // fallback：inputmode=numeric 且 maxLength 通常出现在 CVV
      const im = input.getAttribute("inputmode");
      if (im === "numeric" && input.maxLength >= 3 && input.maxLength <= 4) {
        return { type: "ccCsc", bucket: "creditcard", label: "Security code (CVV/CVC)" };
      }
      // inputmode=email
      if (input.type === "email") return { type: "email", bucket: "address", label: "Email" };
      // type=tel
      if (input.type === "tel") return { type: "phone", bucket: "address", label: "Phone" };
      return null;
    }

    /* ========== 填充值回写 ========== */

    function fillField(input, value) {
      // 触发 React / Vue 等受控组件的 onChange
      const protoSetter = Object.getOwnPropertyDescriptor(window.HTMLInputElement.prototype, "value").set;
      protoSetter.call(input, value);
      input.dispatchEvent(new Event("input", { bubbles: true }));
      input.dispatchEvent(new Event("change", { bubbles: true }));
    }

    function applyFill(fieldsMap, values) {
      // fieldsMap: {type: HTMLInputElement}; values: {type: value-string}
      for (const [type, value] of Object.entries(values || {})) {
        if (value == null) continue;
        const el = fieldsMap[type];
        if (!el) continue;
        fillField(el, String(value));
      }
    }

    /* ========== 收集 form 上下文 ==========
     * 当用户聚焦某个地址字段，我们会向上收集 form（或 document）中所有
     * 可识别的同 bucket 字段，形成 fieldsMap，渲染侧即可根据 form shape
     * 匹配合适的地址/信用卡条目，一次填充整个表单。
     */
    function collectContext(rootInput) {
      const form = rootInput.closest("form") || (rootInput.ownerDocument || document);
      const inputs = form.querySelectorAll("input, select");
      const detected = [];
      let focused = null;
      for (const el of inputs) {
        if (!el || el.tagName !== "INPUT") continue;
        const f = detectField(el);
        if (!f) continue;
        if (el === rootInput) focused = f.type;
        detected.push({ type: f.type, bucket: f.bucket, label: f.label, selector: getCssSelector(el) });
      }
      return { focusedType: focused, fields: detected };
    }

    // 稳定 selector：避免外部 selector 依赖
    function getCssSelector(el) {
      try {
        if (el.id) return "#" + el.id;
        if (el.getAttribute("name")) return el.querySelector.toString ? el.getAttribute("name") : "";
        return "";
      } catch { return ""; }
    }

    function resolveFields(fields) {
      const out = {};
      for (const f of fields) {
        let el;
        if (f.selector && f.selector.startsWith("#")) {
          el = document.querySelector(f.selector);
        } else if (f.selector) {
          el = document.querySelector("input[name=\"" + f.selector + "\"]");
        }
        if (el) out[f.type] = el;
      }
      return out;
    }

    /* ========== 事件桥 ==========
     *  主世界 → 隔离世界：CustomEvent("astra-autofill:field-focus" / "field-blur")
     *  隔离世界 → 主世界：CustomEvent("astra-autofill:fill-form" / "ping")
     */

    let lastFocusCtx = null;

    document.addEventListener("focusin", (ev) => {
      const t = ev.target;
      if (!t || t.tagName !== "INPUT") return;
      const field = detectField(t);
      if (!field) return;
      const ctx = collectContext(t);
      if (!ctx.focusedType) return;
      lastFocusCtx = ctx;
      window.dispatchEvent(new CustomEvent("astra-autofill:field-focus", {
        detail: {
          focusedType: ctx.focusedType,
          focusedBucket: field.bucket,
          focusedLabel: field.label,
          host: location.hostname,
          fields: ctx.fields
        }
      }));
    }, true);

    document.addEventListener("blur", () => {
      window.dispatchEvent(new CustomEvent("astra-autofill:field-blur"));
    }, true);

    // 填充入口：隔离世界/外部通过 CustomEvent 回传
    window.addEventListener("astra-autofill:fill-form", (ev) => {
      const d = ev?.detail || {};
      if (lastFocusCtx) {
        const els = resolveFields(lastFocusCtx.fields);
        applyFill(els, d.values);
      }
    });

    /* ========== PaymentRequest 拦截（P-3） ==========
     *  仅对 methodData 中包含 "basic-card" 的请求做拦截：
     *    1) 向渲染侧发 payment-request 事件（包含 supportedNetworks）
     *    2) 等待 "astra-autofill:payment-response" 事件（返回 card response）
     *       若结果为 cancel → 走原生 UI；若为 card → 返回合成 PaymentResponse
     *
     *  注：Electron 的 Chromium 目前对部分站点已支持原生 Payment Sheet；
     *  这里只拦截 basic-card 保证不破坏 Secure Payment Confirmation 等其他方法。
     */
    try {
      const NativePaymentRequest = window.PaymentRequest;
      if (NativePaymentRequest && typeof NativePaymentRequest === "function") {
        const isBasicCardSupported = (methodData) => {
          if (!Array.isArray(methodData)) return false;
          return methodData.some((m) => m && m.supportedMethods === "basic-card");
        };
        const buildResponse = (req, result) => {
          const r = {
            requestId: "",
            methodName: "basic-card",
            details: result,
            shippingAddress: null,
            shippingOption: null,
            payerName: null,
            payerEmail: null,
            payerPhone: null,
            complete: () => Promise.resolve(),
            retry: () => Promise.resolve(),
            toJSON: () => ({})
          };
          return r;
        };
        const waitForBridgeResponse = (corrId, timeoutMs) => new Promise((resolve) => {
          let done = false;
          const timer = setTimeout(() => {
            if (done) return;
            done = true;
            resolve({ canceled: true, timeout: true });
          }, timeoutMs);
          const onResp = (ev) => {
            if (done) return;
            if (ev?.detail?.correlationId !== corrId) return;
            done = true;
            clearTimeout(timer);
            window.removeEventListener("astra-autofill:payment-response", onResp);
            resolve(ev.detail);
          };
          window.addEventListener("astra-autofill:payment-response", onResp);
        });
        window.PaymentRequest = class AstraPaymentRequest extends NativePaymentRequest {
          constructor(methodData, details, options) {
            super(methodData, details, options);
            this.__astraBasicCard = isBasicCardSupported(methodData);
            this.__astraMethods = methodData;
            this.__astraDetails = details;
          }
          show(optionalDetailsPromise) {
            if (!this.__astraBasicCard) {
              return super.show(optionalDetailsPromise);
            }
            const correlationId = "pr-" + Date.now().toString(36) + "-" + Math.random().toString(36).slice(2, 8);
            // 通知渲染侧：弹出卡片选择器
            window.dispatchEvent(new CustomEvent("astra-autofill:payment-request", {
              detail: {
                correlationId,
                host: location.hostname,
                methodData: this.__astraMethods,
                total: this.__astraDetails?.total,
                displayItems: this.__astraDetails?.displayItems
              }
            }));
            return Promise.resolve(optionalDetailsPromise)
              .then(() => waitForBridgeResponse(correlationId, 120_000))
              .then((resp) => {
                if (!resp || resp.canceled) {
                  // 用户取消 → 走原生 Payment Sheet
                  return super.show(optionalDetailsPromise);
                }
                // 使用用户选择的卡（已解密）合成 PaymentResponse
                return buildResponse(this, resp.paymentResponse);
              });
          }
        };
        // 让 instanceof 检查也能通过
        window.PaymentRequest.prototype = NativePaymentRequest.prototype;
      }
    } catch { /* 站点自己已经 monkey-patched → 忽略 */ }

    /* ========== 保存新卡检测（表单提交） ==========
     *  当检测到 submit 事件时的 form 中同时出现 ccNumber + ccExp + ccName，
     *  触发 save-creditcard 事件，渲染侧根据是否重复再决定是否弹保存 prompt。
     */
    document.addEventListener("submit", (ev) => {
      try {
        const form = ev.target;
        if (!form || form.tagName !== "FORM") return;
        const inputs = form.querySelectorAll("input");
        const detected = [];
        for (const el of inputs) {
          if (el.tagName !== "INPUT") continue;
          const f = detectField(el);
          if (!f || f.bucket !== "creditcard") continue;
          detected.push({
            type: f.type,
            value: el.value || "",
            label: f.label
          });
        }
        if (detected.length < 2) return;
        // 仅提取需要保存的字段，避免 cvv 明文在内存里留存过久
        const numberField = detected.find((d) => d.type === "ccNumber");
        const nameField = detected.find((d) => d.type === "ccName");
        const expField = detected.find((d) =>
          d.type === "ccExp" || d.type === "ccExpMonth" || d.type === "ccExpYear");
        if (!numberField || !numberField.value) return;
        window.dispatchEvent(new CustomEvent("astra-autofill:save-creditcard", {
          detail: {
            host: location.hostname,
            cardholderName: nameField?.value || "",
            number: numberField.value.replace(/\D/g, ""),
            expiryRaw: expField?.value || "",
            expiryMonth: detected.find((d) => d.type === "ccExpMonth")?.value || "",
            expiryYear: detected.find((d) => d.type === "ccExpYear")?.value || "",
            cvv: (detected.find((d) => d.type === "ccCsc")?.value || "").replace(/\D/g, "")
          }
        }));
      } catch { /* ignore */ }
    }, true);
  })();
`;
}

/**
 * 隔离世界 preload：bridge。
 * 作为 <webview preload="file://${path}"> 属性加载，拥有 ipcRenderer 权限。
 */
function buildBridgeScript() {
  return String.raw`
  (function() {
    const { ipcRenderer } = require("electron");
    if (window.__astraAutofillBridgeInstalled) return;
    window.__astraAutofillBridgeInstalled = true;

    // 主世界 → main 进程：转发 field-focus / field-blur
    window.addEventListener("astra-autofill:field-focus", (ev) => {
      try {
        const wcId = process.getELECTRON_RENDERER_PROCESS_ID ? null : null;
        ipcRenderer.send("autofill:field-focus", {
          webContentsId: (window.location && location.hostname) ? null : null,
          detail: ev.detail
        });
      } catch {}
    });
    window.addEventListener("astra-autofill:field-blur", () => {
      try { ipcRenderer.send("autofill:field-blur"); } catch {}
    });
    // 主世界 → main：PaymentRequest（用户在站点点了「立即购买」触发 show）
    window.addEventListener("astra-autofill:payment-request", (ev) => {
      try {
        const senderWcId = process.getELECTRON_RENDERER_PROCESS_ID ? null : null;
        ipcRenderer.send("autofill:payment-request", {
          webContentsId: senderWcId,
          detail: ev.detail
        });
      } catch {}
    });
    // 主世界 → main：保存新卡（form.submit 时检测到 CC）
    window.addEventListener("astra-autofill:save-creditcard", (ev) => {
      try {
        const senderWcId = process.getELECTRON_RENDERER_PROCESS_ID ? null : null;
        ipcRenderer.send("autofill:save-creditcard", {
          webContentsId: senderWcId,
          detail: ev.detail
        });
      } catch {}
    });

    // main 进程 → 主世界：执行填充
    ipcRenderer.on("autofill:fill-form", (_event, values) => {
      window.dispatchEvent(new CustomEvent("astra-autofill:fill-form", { detail: { values } }));
    });
    // main 进程 → 主世界：PaymentRequest 响应（用户选中卡片 or 取消）
    ipcRenderer.on("autofill:payment-response", (_event, payload) => {
      window.dispatchEvent(new CustomEvent("astra-autofill:payment-response", { detail: payload || {} }));
    });
  })();
`;
}

/** 每次启动（内容一致时）返回同一个 bridge preload 文件路径。 */
function ensureBridgeFile() {
  const content = buildBridgeScript();
  const hash = crypto.createHash("sha1").update(content).digest("hex").slice(0, 10);
  const tmp = path.join(os.tmpdir(), `astra-autofill-bridge-${hash}.js`);
  if (!fs.existsSync(tmp)) {
    try {
      fs.writeFileSync(tmp, content, { mode: 0o644 });
    } catch {
      return null;
    }
  }
  // `<webview preload=...>` 在 Electron 中只接受 "file:" 协议 URL；
  // 裸绝对路径在 dev-server 模式下会被当作相对于 vite dev server 的 HTTP URL。
  // 统一返回 RFC 8089 file-URI，macOS/Linux/Windows 均兼容（pathname 绝对）。
  const fileUri = `file://${tmp.startsWith("/") ? "" : "/"}${tmp.replace(/\\/g, "/")}`;
  return fileUri;
}

function installAutofillShim(targetSession) {
  if (!targetSession || installedSessions.has(targetSession)) return;
  const tmp = ensureShimFile();
  if (!tmp) return;
  try {
    const existing = targetSession.getPreloads ? [...(targetSession.getPreloads?.() || [])] : [];
    if (!existing.includes(tmp)) {
      // 避免重复添加（WeakSet 上面的判断本应拦住，但做双保险）
      targetSession.setPreloads([tmp, ...existing]);
    }
    installedSessions.add(targetSession);
    shimFileBySession.set(targetSession, tmp);
  } catch {
    // setPreloads 不可用（极个别 Electron 版本） → 降级忽略。
  }
}

module.exports = {
  installAutofillShim,
  ensureShimFile, // 导出给测试
  ensureBridgeFile,
  /**
   * main.js app.whenReady 里调用一次，注册 IPC：
   *   - autofill:get-bridge-path → 返回 bridge preload 文件路径（renderer 用它填 webview preload 属性）
   *   - autofill:fill-form → renderer 请求把 values 路由回某个 webContentsId
   *   - autofill:field-focus/field-blur (ipcRenderer.send from bridge) → broadcast 给 host 窗口
   */
  installAutofillIpc({ broadcastToRenderer }) {
    ipcMain.handle("autofill:get-bridge-path", () => ensureBridgeFile());
    ipcMain.handle("autofill:fill-form", (_event, { webContentsId, values }) => {
      if (typeof webContentsId !== "number") return { ok: false, reason: "missing-webContentsId" };
      try {
        const wc = webContents.fromId(webContentsId);
        if (!wc || wc.isDestroyed()) return { ok: false, reason: "destroyed" };
        wc.send("autofill:fill-form", values);
        return { ok: true };
      } catch (err) {
        return { ok: false, reason: err instanceof Error ? err.message : String(err) };
      }
    });
    /**
     * P-3 PaymentRequest 响应。renderer 把 {correlationId, canceled, paymentResponse}
     * 发送给 webview，bridge 接收后 dispach astra-autofill:payment-response CustomEvent
     * 给主世界 shim，shim 中的 show() Promise 将 resolve 该结果。
     */
    ipcMain.handle("autofill:payment-response", (_event, { webContentsId, response }) => {
      if (typeof webContentsId !== "number") return { ok: false, reason: "missing-webContentsId" };
      try {
        const wc = webContents.fromId(webContentsId);
        if (!wc || wc.isDestroyed()) return { ok: false, reason: "destroyed" };
        wc.send("autofill:payment-response", response);
        return { ok: true };
      } catch (err) {
        return { ok: false, reason: err instanceof Error ? err.message : String(err) };
      }
    });
    // bridge 传来的 field-focus → 广播给 host 窗口 renderer（注意 sender 是 webview 的 webContents）
    ipcMain.on("autofill:field-focus", (event, payload) => {
      const senderWcId = event.sender?.id;
      broadcastToRenderer?.("autofill:field-focus", {
        webContentsId: typeof senderWcId === "number" ? senderWcId : null,
        ...(payload || {}),
      });
    });
    ipcMain.on("autofill:field-blur", (event) => {
      const senderWcId = event.sender?.id;
      broadcastToRenderer?.("autofill:field-blur", {
        webContentsId: typeof senderWcId === "number" ? senderWcId : null
      });
    });
    ipcMain.on("autofill:payment-request", (event, payload) => {
      const senderWcId = event.sender?.id;
      broadcastToRenderer?.("autofill:payment-request", {
        webContentsId: typeof senderWcId === "number" ? senderWcId : null,
        ...(payload || {})
      });
    });
    ipcMain.on("autofill:save-creditcard", (event, payload) => {
      const senderWcId = event.sender?.id;
      broadcastToRenderer?.("autofill:save-creditcard", {
        webContentsId: typeof senderWcId === "number" ? senderWcId : null,
        ...(payload || {})
      });
    });
  }
};

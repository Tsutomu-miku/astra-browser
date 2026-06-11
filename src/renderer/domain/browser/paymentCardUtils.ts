/**
 * P-3 支付方式工具：BIN 品牌识别 + Luhn 校验 + 加解密封装。
 *
 * BIN/IIN 范围整理自公开资料；MVP 覆盖 6 大主流品牌（Visa/MC/Amex/
 * Discover/JCB/UnionPay）。未识别时返回 "other"，UI 回退到通用卡图标。
 *
 * 加密：复用 passwordVault 的 AES-256-GCM in-memory vault key。
 * encryptedCardDetails 格式（JSON 字符串被 encryptSecret 加密后）：
 *   { pan: string, csc: string, brand: string }
 *  - pan：完整卡号（去除空格/非数字）
 *  - csc：CVV/CVC（3-4 位，可选）
 *  - brand：BIN 识别出的卡品牌（用于 UI 提示）
 */

import {
  decryptSecret,
  encryptSecret,
  isVaultUnlocked
} from "./passwordVault";

export type CardBrand =
  | "visa"
  | "mastercard"
  | "amex"
  | "discover"
  | "jcb"
  | "unionpay"
  | "diners"
  | "other";

export interface CardDetails {
  pan: string;
  csc?: string;
  brand: CardBrand;
  expiryMonth?: number;
  expiryYear?: number;
  cardholderName?: string;
  cardLastFour: string;
}

/** IIN 范围表（按长度降序匹配，避免 Amex 与其他品牌冲突）。 */
const IIN_RULES: Array<{ brand: CardBrand; prefixes: string[] }> = [
  // Diners Club: 300-305, 3095, 36, 38, 39
  { brand: "diners", prefixes: ["300", "301", "302", "303", "304", "305", "3095", "36", "38", "39"] },
  // Amex: 34 / 37
  { brand: "amex", prefixes: ["34", "37"] },
  // JCB: 3528-3589
  { brand: "jcb", prefixes: generateNumericPrefixes(3528, 3589) },
  // UnionPay: 62 / 81
  { brand: "unionpay", prefixes: ["62", "81"] },
  // Discover: 6011, 622126-622925, 644-649, 65
  {
    brand: "discover",
    prefixes: ["6011", "65", ...generateNumericPrefixes(644, 649), ...generateNumericPrefixes(622126, 622925)]
  },
  // Mastercard: 51-55 / 2221-2720
  { brand: "mastercard", prefixes: [...generateNumericPrefixes(51, 55), ...generateNumericPrefixes(2221, 2720)] },
  // Visa: 4
  { brand: "visa", prefixes: ["4"] }
];

function generateNumericPrefixes(start: number, end: number): string[] {
  const out: string[] = [];
  for (let i = start; i <= end; i++) out.push(String(i));
  return out;
}

/** 按 IIN 前缀长度降序，优先匹配更具体的前缀（避免 4 先于 4xxx 命中）。 */
const SORTED_RULES: Array<{ brand: CardBrand; prefix: string }> = IIN_RULES
  .flatMap((r) => r.prefixes.map((p) => ({ brand: r.brand, prefix: p })))
  .sort((a, b) => b.prefix.length - a.prefix.length);

export function detectCardBrand(rawPan: string): CardBrand {
  const pan = (rawPan || "").replace(/\D/g, "");
  if (!pan) return "other";
  for (const rule of SORTED_RULES) {
    if (pan.startsWith(rule.prefix)) return rule.brand;
  }
  return "other";
}

/** Luhn 算法校验（所有主流品牌的校验位算法一致）。 */
export function luhnCheck(rawPan: string): boolean {
  const pan = (rawPan || "").replace(/\D/g, "");
  if (pan.length < 12 || pan.length > 19) return false;
  let sum = 0;
  let alt = false;
  for (let i = pan.length - 1; i >= 0; i--) {
    let d = Number(pan[i]);
    if (alt) {
      d *= 2;
      if (d > 9) d -= 9;
    }
    sum += d;
    alt = !alt;
  }
  return sum % 10 === 0;
}

/** 品牌对应的典型长度范围（校验用）。 */
export function expectedPanLength(brand: CardBrand): number[] {
  switch (brand) {
    case "visa":
      return [13, 16, 19];
    case "mastercard":
      return [16];
    case "amex":
      return [15];
    case "discover":
      return [16, 19];
    case "jcb":
      return [15, 16, 17, 18, 19];
    case "unionpay":
      return [16, 17, 18, 19];
    case "diners":
      return [14, 16, 19];
    default:
      return [13, 14, 15, 16, 17, 18, 19];
  }
}

export function isValidPan(rawPan: string): { valid: boolean; brand: CardBrand; reason?: string } {
  const pan = (rawPan || "").replace(/\D/g, "");
  if (!pan) return { valid: false, brand: "other", reason: "empty" };
  const brand = detectCardBrand(pan);
  const lengths = expectedPanLength(brand);
  if (!lengths.includes(pan.length)) {
    return { valid: false, brand, reason: `length: expected ${lengths.join("/")}, got ${pan.length}` };
  }
  if (!luhnCheck(pan)) return { valid: false, brand, reason: "luhn-failed" };
  return { valid: true, brand };
}

export function formatCardNumber(rawPan: string, brand = detectCardBrand(rawPan)): string {
  const pan = (rawPan || "").replace(/\D/g, "");
  // Amex: 4-6-5（XXXXXX XXXXXX XXXXX）
  if (brand === "amex") {
    return [pan.slice(0, 4), pan.slice(4, 10), pan.slice(10, 15)].filter(Boolean).join(" ");
  }
  // 其他：每 4 位
  return pan.match(/.{1,4}/g)?.join(" ") ?? pan;
}

export function lastFourOf(rawPan: string): string {
  const pan = (rawPan || "").replace(/\D/g, "");
  if (pan.length < 4) return pan;
  return pan.slice(-4);
}

/** CVV/CVC 长度：Amex=4，其他=3。 */
export function expectedCscLength(brand: CardBrand): number {
  return brand === "amex" ? 4 : 3;
}

export function isValidExpiry(mm: number | undefined, yy: number | undefined): boolean {
  if (mm == null || yy == null || Number.isNaN(mm) || Number.isNaN(yy)) return false;
  if (mm < 1 || mm > 12) return false;
  const year = yy < 100 ? 2000 + yy : yy;
  const now = new Date();
  const maxYear = now.getFullYear() + 30;
  if (year < now.getFullYear() || year > maxYear) return false;
  if (year === now.getFullYear() && mm < now.getMonth() + 1) return false;
  return true;
}

/* ===== 加解密：JSON 对象 → vault encryptSecret ===== */

export interface EncryptedCardPayload {
  pan: string;
  csc?: string;
  brand: CardBrand;
}

export async function encryptCardDetails(payload: EncryptedCardPayload): Promise<string> {
  if (!isVaultUnlocked()) throw new Error("vault-not-unlocked");
  return encryptSecret(JSON.stringify(payload));
}

export async function decryptCardDetails(encrypted: string): Promise<EncryptedCardPayload | null> {
  if (!encrypted || !isVaultUnlocked()) return null;
  try {
    const json = await decryptSecret(encrypted);
    const obj = JSON.parse(json) as EncryptedCardPayload;
    if (obj && typeof obj.pan === "string") return obj;
    return null;
  } catch {
    return null;
  }
}

/** 用于 PaymentRequest 回调，把 CardDetails 拼成 PRD 的 BasicCardResponse。 */
export function toBasicCardResponse(details: CardDetails): {
  cardholderName: string;
  cardNumber: string;
  expiryMonth: string;
  expiryYear: string;
  cardSecurityCode: string;
  billingAddress?: any;
} {
  return {
    cardholderName: details.cardholderName || "",
    cardNumber: details.pan || "",
    expiryMonth: String(details.expiryMonth ?? "").padStart(2, "0"),
    expiryYear: String(details.expiryYear ?? ""),
    cardSecurityCode: details.csc || ""
  };
}

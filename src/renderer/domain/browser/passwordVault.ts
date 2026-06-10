/**
 * 密码库（PRD §3.6 P-1：最小可用版本。
 *
 * 加密：
 *   - PasswordEntry：单条凭证（origin/signon/username/password/notes）
 *   - 序列化：BrowserState.passwords 作为 PasswordEntry[]，未加密存入内存
 *   - 加解密函数：基于 Web Crypto AES-GCM + 派生主密钥（首次使用用户显式解锁；MVP 阶段主密钥从 localStorage 派生
 *   - 真实部署：M2 时换 keytar / Keyring
 *
 * 注意：MVP 版本仅提供数据库模型和功能入口，主密钥保护和加密存储放到 M2 补 keytar 接入。
 */

import { createId } from "./factory";

export interface PasswordEntry {
  id: string;
  /** e.g. https://github.com */
  origin: string;
  username: string;
  /** 加密后的 base64：ciphertext.iv.tag 拼接 */
  encryptedPassword: string;
  /** 可选备注（纯文本，不包含敏感信息） */
  notes?: string;
  usedAt?: number;
  createdAt: number;
  updatedAt: number;
}

export interface AddressEntry {
  id: string;
  label: string;
  recipient: string;
  address1: string;
  address2?: string;
  city: string;
  region?: string;
  postalCode?: string;
  country?: string;
  phone?: string;
  email?: string;
  createdAt: number;
}

export interface PaymentMethodEntry {
  id: string;
  label: string;
  cardholderName: string;
  /** 卡号后 4 位 + 过期 MM/YY（不存完整卡号，遵循 PCI） */
  cardLastFour: string;
  encryptedCardDetails: string;
  expiryMonth?: number;
  expiryYear?: number;
  createdAt: number;
  updatedAt: number;
}

const VAULT_VERSION = 1;
const IV_BYTES = 12;
const SALT_BYTES = 16;
const KDF_ITERATIONS = 600_000;
/** 主密钥在内存里缓存（MVP：用户解锁后不随状态持久化） */
let inMemoryVaultKey: CryptoKey | null = null;

function toBase64(bytes: Uint8Array): string {
  let binary = "";
  for (let i = 0; i < bytes.byteLength; i += 1) binary += String.fromCharCode(bytes[i]);
  return globalThis.btoa(binary);
}

function fromBase64(input: string): Uint8Array {
  const binary = globalThis.atob(input);
  const out = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i += 1) out[i] = binary.charCodeAt(i);
  return out;
}

async function deriveVaultKey(passphrase: string, salt: Uint8Array): Promise<CryptoKey> {
  const baseKey = await crypto.subtle.importKey("raw", new TextEncoder().encode(passphrase), { name: "PBKDF2" }, false, ["deriveKey"]);
  return crypto.subtle.deriveKey(
    { name: "PBKDF2", salt: webCryptoBytes(salt), iterations: KDF_ITERATIONS, hash: "SHA-256" },
    baseKey,
    { name: "AES-GCM", length: 256 },
    false,
    ["encrypt", "decrypt"]
  );
}

/** 获取或创建 vault 元数据（首次调用会生成 salt 与版本号，持久化到 localStorage） */
export interface VaultMetadata {
  saltBase64: string;
  version: number;
  /** 首次调用前为 true（无需密码就能解锁，MVP 版本主密码留空） */
  initialized: boolean;
}

const VAULT_METADATA_KEY = "astra.passwordVault.meta.v1";

export function getVaultMetadata(): VaultMetadata {
  const raw = typeof localStorage !== "undefined" ? localStorage.getItem(VAULT_METADATA_KEY) : null;
  if (raw) {
    try {
      const parsed = JSON.parse(raw) as VaultMetadata;
      return parsed;
    } catch {
      /* fallthrough */
    }
  }
  const salt = crypto.getRandomValues(new Uint8Array(SALT_BYTES));
  const meta: VaultMetadata = { saltBase64: toBase64(salt), version: VAULT_VERSION, initialized: false };
  try { localStorage.setItem(VAULT_METADATA_KEY, JSON.stringify(meta)); } catch { /* ignore */ }
  return meta;
}

/** 解锁 vault（MVP 里 passphrase 为空字符串，即"无密码但数据库可用"）。
 *  M2 起替换为真实主密码 + keytar。 */
export async function unlockVault(passphrase = ""): Promise<void> {
  const meta = getVaultMetadata();
  const salt = fromBase64(meta.saltBase64);
  inMemoryVaultKey = await deriveVaultKey(passphrase, salt);
}

export function isVaultUnlocked(): boolean {
  return inMemoryVaultKey !== null;
}

export function lockVault(): void {
  inMemoryVaultKey = null;
}

/**
 * TypeScript lib.dom 对 BufferSource 要求 `ArrayBuffer`（不接受
 * `SharedArrayBuffer`），但 getRandomValues 返回的 Uint8Array 在某些
 * lib 版本下类型是 `Uint8Array<ArrayBufferLike>`。运行时我们用独立
 * `ArrayBuffer` 构造并使用 `as BufferSource` 绕开严格类型。
 */
function webCryptoBytes(bytes: Uint8Array): BufferSource {
  const buffer = new ArrayBuffer(bytes.byteLength);
  const out = new Uint8Array(buffer);
  out.set(bytes);
  return out as unknown as BufferSource;
}

export async function encryptSecret(plaintext: string): Promise<string> {
  if (!inMemoryVaultKey) throw new Error("vault-not-unlocked");
  const rawIv = crypto.getRandomValues(new Uint8Array(IV_BYTES));
  const iv = webCryptoBytes(rawIv);
  const ct = new Uint8Array(
    await crypto.subtle.encrypt({ name: "AES-GCM", iv }, inMemoryVaultKey, new TextEncoder().encode(plaintext))
  );
  return `${toBase64(rawIv)}.${toBase64(ct)}`;
}

export async function decryptSecret(cipherBundle: string): Promise<string> {
  if (!inMemoryVaultKey) throw new Error("vault-not-unlocked");
  const parts = cipherBundle.split(".");
  const ivB64 = parts[0];
  const ctB64 = parts[1];
  if (!ivB64 || !ctB64) throw new Error("bad-cipher");
  const plain = await crypto.subtle.decrypt(
    { name: "AES-GCM", iv: webCryptoBytes(fromBase64(ivB64)) },
    inMemoryVaultKey,
    webCryptoBytes(fromBase64(ctB64))
  );
  return new TextDecoder().decode(plain);
}

export interface PasswordDraft {
  origin: string;
  username: string;
  password: string;
  notes?: string;
}

export async function createPasswordEntry(draft: PasswordDraft): Promise<PasswordEntry> {
  const now = Date.now();
  return {
    id: createId(),
    origin: normalizeOrigin(draft.origin),
    username: draft.username,
    encryptedPassword: await encryptSecret(draft.password),
    notes: draft.notes,
    createdAt: now,
    updatedAt: now
  };
}

export function normalizeOrigin(raw: string): string {
  if (!raw) return "";
  try {
    const u = new URL(raw.includes("://") ? raw : `https://${raw}`);
    return `${u.protocol}//${u.host}`;
  } catch {
    return String(raw).trim();
  }
}

export function passwordMatchesOrigin(entry: PasswordEntry, candidate: string): boolean {
  const candidateOrigin = normalizeOrigin(candidate);
  if (!entry.origin || !candidateOrigin) return false;
  return entry.origin === candidateOrigin;
}

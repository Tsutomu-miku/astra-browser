import { describe, expect, it, vi } from "vitest";

import type { PasswordEntry } from "../src/renderer/domain/browser";
import {
  createPasswordEntry,
  isVaultUnlocked,
  normalizeOrigin,
  passwordMatchesOrigin,
  unlockVault
} from "../src/renderer/domain/browser/passwordVault";
import {
  removePassword,
  touchPasswordUsed,
  upsertPassword
} from "../src/renderer/domain/permissions/settingsActions";
import { createDefaultState } from "../src/renderer/domain/browser/factory";

function makeEntry(patch: Partial<PasswordEntry> = {}): PasswordEntry {
  return {
    id: "p",
    origin: "https://github.com",
    username: "alice",
    encryptedPassword: "AAAA.bbbb",
    createdAt: 1,
    updatedAt: 1,
    ...patch
  };
}

describe("password vault: origin normalize", () => {
  it("strips paths and ports where relevant", () => {
    expect(normalizeOrigin("https://github.com/login/oauth")).toBe("https://github.com");
    expect(normalizeOrigin("http://example.com:8080/app")).toBe("http://example.com:8080");
    expect(normalizeOrigin("example.com")).toBe("https://example.com");
    expect(normalizeOrigin("")).toBe("");
  });

  it("matches origins strictly by normalized origin", () => {
    const entry = makeEntry({ origin: "https://github.com" });
    expect(passwordMatchesOrigin(entry, "https://github.com/login")).toBe(true);
    expect(passwordMatchesOrigin(entry, "https://github.com:443/x")).toBe(true);
    expect(passwordMatchesOrigin(entry, "http://github.com/login")).toBe(false);
    expect(passwordMatchesOrigin(entry, "https://evil-github.com")).toBe(false);
  });
});

describe("password vault: createPasswordEntry encrypts", () => {
  it("produces normalized origin + encrypted ciphertext", async () => {
    await unlockVault("");
    expect(isVaultUnlocked()).toBe(true);
    const created = await createPasswordEntry({
      origin: "https://example.com/signin",
      username: "bob",
      password: "correct horse battery staple",
      notes: "work account"
    });
    expect(created.origin).toBe("https://example.com");
    expect(created.username).toBe("bob");
    expect(created.notes).toBe("work account");
    expect(created.encryptedPassword).toMatch(/^[A-Za-z0-9+/=]+\.[A-Za-z0-9+/=]+$/);
    expect(created.createdAt).toBeGreaterThan(0);
    expect(created.id).toBeTruthy();
  });
});

describe("settingsActions: upsert / remove / touch", () => {
  it("upsertPassword inserts on unique (origin+username) and updates existing", () => {
    const s0 = createDefaultState();
    const s1 = upsertPassword(s0, makeEntry({ id: "", origin: "https://a.test", username: "u1", createdAt: 10 }));
    const pwdList1 = s1.settings.autofill.passwords;
    expect(pwdList1).toHaveLength(1);
    expect(pwdList1[0].username).toBe("u1");
    expect(pwdList1[0].id).toBeTruthy();

    const s2 = upsertPassword(s1, makeEntry({ id: "", origin: "https://a.test", username: "u1", encryptedPassword: "NEW.ENCRYPTED" }));
    expect(s2.settings.autofill.passwords).toHaveLength(1);
    expect(s2.settings.autofill.passwords[0].encryptedPassword).toBe("NEW.ENCRYPTED");
  });

  it("removePassword filters by id", () => {
    const s = upsertPassword(createDefaultState(), makeEntry({ id: "aaa" }));
    expect(removePassword(s, "aaa").settings.autofill.passwords).toHaveLength(0);
    expect(removePassword(s, "unknown").settings.autofill.passwords).toHaveLength(1);
  });

  it("touchPasswordUsed stamps usedAt", () => {
    const now = Date.now();
    vi.useFakeTimers().setSystemTime(now);
    const s1 = upsertPassword(createDefaultState(), makeEntry({ id: "x" }));
    const s2 = touchPasswordUsed(s1, "x");
    expect(s2.settings.autofill.passwords[0].usedAt).toBe(now);
    vi.useRealTimers();
  });
});

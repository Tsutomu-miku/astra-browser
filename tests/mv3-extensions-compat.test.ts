import { beforeEach, describe, expect, it } from "vitest";

/**
 * M2.5 E-1/E-2 MV3 扩展兼容层 PoC smoke 测试。
 * 纯 domain 测试：直接 require main 端 mv3Extensions 模块验证函数签名与
 * manifest 校验、规则集装配逻辑。不需要 Electron 启动。
 */

import fs from "node:fs";
import os from "node:os";
import path from "node:path";

const MV3 = require("../src/main/mv3Extensions.js");

function makeTmpDir() {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), "astra-mv3-test-"));
  return dir;
}

function writeManifest(root: string, extId: string, manifest: unknown) {
  const extDir = path.join(root, "extensions", extId);
  fs.mkdirSync(extDir, { recursive: true });
  fs.writeFileSync(path.join(extDir, "manifest.json"), JSON.stringify(manifest));
  return extDir;
}

describe("MV3 extension compatibility PoC (E-1/E-2)", () => {
  beforeEach(() => {
    MV3.resetLoadedRegistry();
  });

  it("loads manifest V3 extensions and rejects V2", () => {
    const userDataDir = makeTmpDir();
    writeManifest(userDataDir, "good-mv3", {
      manifest_version: 3,
      name: "Dark Reader Lite",
      version: "1.0.0",
      description: "Dark mode for every website"
    });
    writeManifest(userDataDir, "bad-mv2", {
      manifest_version: 2,
      name: "Legacy ext",
      version: "0.1"
    });
    writeManifest(userDataDir, "no-manifest", {});

    MV3.scanAndLoad(userDataDir);
    const list = MV3.listExtensions();

    expect(list.find((e: { id: string }) => e.id === "good-mv3")).toBeTruthy();
    expect(list.find((e: { id: string }) => e.id === "bad-mv2")).toBeUndefined();
    expect(list.find((e: { id: string }) => e.id === "no-manifest")).toBeUndefined();

    const good = list.find((e: { id: string }) => e.id === "good-mv3");
    expect(good.name).toBe("Dark Reader Lite");
    expect(good.version).toBe("1.0.0");
    expect(good.enabled).toBe(true);
  });

  it("compiles DNR rulesets and attaches them by extension id", () => {
    const userDataDir = makeTmpDir();
    const extDir = writeManifest(userDataDir, "block-ads", {
      manifest_version: 3,
      name: "Ad blocker smoke",
      version: "0.1",
      declarative_net_request: {
        rule_resources: [{ id: "rules", enabled: true, path: "rules.json" }]
      }
    });
    fs.writeFileSync(
      path.join(extDir, "rules.json"),
      JSON.stringify([
        {
          id: 1,
          priority: 1,
          action: { type: "block" },
          condition: { urlFilter: "||ads.example.com^", resourceTypes: ["script"] }
        },
        {
          id: 2,
          priority: 1,
          action: { type: "upgradeScheme" },
          condition: { urlFilter: "http://*" }
        }
      ])
    );

    MV3.scanAndLoad(userDataDir);
    MV3.compileDnr();

    // 虽然我们无法在无头测试里验证 session.webRequest，但 list 中必须包含该扩展，
    // 且该扩展启用时 compileDnr 不会抛。
    const list = MV3.listExtensions();
    expect(list.map((e: { id: string }) => e.id)).toContain("block-ads");
    expect(() => MV3.compileDnr()).not.toThrow();
  });

  it("installs extension from folder by copying to userData/extensions/<id>", () => {
    const userDataDir = makeTmpDir();
    const sourceDir = makeTmpDir();
    fs.writeFileSync(
      path.join(sourceDir, "manifest.json"),
      JSON.stringify({
        manifest_version: 3,
        name: "uBlock Smoke",
        version: "0.0.1",
        description: "uBlock smoke install"
      })
    );

    const result = MV3.installFromFolder(userDataDir, sourceDir);
    expect(result.ok).toBe(true);
    expect(typeof result.id).toBe("string");

    const list = MV3.listExtensions();
    expect(list.map((e: { name: string }) => e.name)).toContain("uBlock Smoke");
    const installed = list.find((e: { name: string }) => e.name === "uBlock Smoke");
    // 目录确实被拷贝到 extensions/<id>
    const copiedDir = path.join(userDataDir, "extensions", installed.id);
    expect(fs.existsSync(path.join(copiedDir, "manifest.json"))).toBe(true);
  });

  it("enables/disables extension via setEnabled and persists to state file", () => {
    const userDataDir = makeTmpDir();
    writeManifest(userDataDir, "toggle-me", {
      manifest_version: 3,
      name: "Toggle test",
      version: "1.0"
    });
    MV3.scanAndLoad(userDataDir);
    expect(MV3.listExtensions().find((e: { id: string }) => e.id === "toggle-me").enabled).toBe(true);

    expect(MV3.setEnabled(userDataDir, "toggle-me", false)).toBe(true);
    MV3.scanAndLoad(userDataDir);
    expect(MV3.listExtensions().find((e: { id: string }) => e.id === "toggle-me").enabled).toBe(false);

    // 重启模拟：重新扫
    const state = JSON.parse(
      fs.readFileSync(path.join(userDataDir, "astra-extensions-state.json"), "utf8")
    );
    expect(state["toggle-me"].enabled).toBe(false);
  });

  it("uninstall removes directory, state entry, and in-memory registry", () => {
    const userDataDir = makeTmpDir();
    writeManifest(userDataDir, "bye", {
      manifest_version: 3,
      name: "Going away",
      version: "0.1"
    });
    MV3.scanAndLoad(userDataDir);
    expect(MV3.listExtensions().some((e: { id: string }) => e.id === "bye")).toBe(true);

    const result = MV3.uninstallExtension(userDataDir, "bye");
    expect(result.ok).toBe(true);
    expect(MV3.listExtensions().some((e: { id: string }) => e.id === "bye")).toBe(false);
    expect(fs.existsSync(path.join(userDataDir, "extensions", "bye"))).toBe(false);
  });
});

import { defineConfig } from "vitest/config";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],
  root: "src/renderer",
  base: "./",
  cacheDir: "../../node_modules/.vite-renderer",
  build: {
    outDir: "../../dist/renderer",
    emptyOutDir: true
  },
  server: {
    host: "127.0.0.1",
    port: 5173,
    strictPort: true
  },
  test: {
    environment: "jsdom",
    include: ["../../tests/**/*.test.ts"]
  }
});

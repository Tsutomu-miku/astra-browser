import js from "@eslint/js";
import tseslint from "typescript-eslint";

export default tseslint.config(
  {
    ignores: [
      "build",
      "chromium/src",
      "chromium/out",
      "dist",
      "node_modules",
      "release",
      "scripts",
      "third_party"
    ]
  },
  js.configs.recommended,
  ...tseslint.configs.recommended,
  {
    files: ["**/*.{ts,tsx,js,mjs}"],
    rules: {
      // Enforce 300 lines per source file as a hard architectural guard.
      // CSS is not checked by ESLint and is exempt by nature.
      // Files that legitimately cannot be split should add:
      //   /* eslint-disable max-lines */
      // at the top, with a comment explaining why a split would be
      // architecturally unreasonable.
      "max-lines": ["error", { max: 300, skipBlankLines: false, skipComments: false }],
      // The rest of the recommended rulesets are turned off here so the lint
      // command only enforces what the project actually cares about.
      "no-unused-vars": "off",
      "no-undef": "off",
      "@typescript-eslint/no-unused-vars": "off",
      "@typescript-eslint/no-explicit-any": "off",
      "@typescript-eslint/no-unused-expressions": "off",
      "@typescript-eslint/no-empty-object-type": "off",
      "@typescript-eslint/no-require-imports": "off",
      "@typescript-eslint/no-this-alias": "off"
    }
  },
  {
    files: ["tests/**/*.{ts,tsx,js}"],
    rules: {
      "max-lines": "off"
    }
  }
);

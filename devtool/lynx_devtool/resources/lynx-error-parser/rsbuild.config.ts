import { defineConfig } from '@rsbuild/core';

export default defineConfig({
  tools: {
    htmlPlugin: false,
  },
  output: {
    filename: {
      js: 'lynx-error-parser.js',
    },
  },
});

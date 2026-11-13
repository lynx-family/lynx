import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    name: 'lynx-core',
    include: ['test/**/*.test.ts'],
  },
});

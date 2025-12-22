import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    name: 'type-element-api',
    include: ['test/**/*.test-d.ts', 'test/**/*.test.js'],
    typecheck: {
      enabled: true,
      include: ['test/**/*.test-d.ts'],
      tsconfig: './tsconfig.json',
    },
  },
});

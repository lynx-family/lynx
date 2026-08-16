import { defineConfig } from '@lynx-js/rspeedy';
import { pluginReactLynx } from '@lynx-js/react-rsbuild-plugin';

const entry = process.env.GRID_LANES_FIXTURE_ENTRY;
if (!entry) {
  throw new Error('GRID_LANES_FIXTURE_ENTRY is required');
}

export default defineConfig({
  source: {
    entry,
  },
  output: {
    filename: 'main.lynx.bundle',
  },
  plugins: [pluginReactLynx()],
});

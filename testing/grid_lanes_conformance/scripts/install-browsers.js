const path = require('path');
const { spawnSync } = require('child_process');

const { configurePlaywrightHostPlatform } = require('./host-platform');

const environment = configurePlaywrightHostPlatform({ ...process.env });

const packagePath = require.resolve('playwright/package.json');
const cliPath = path.join(path.dirname(packagePath), 'cli.js');
const result = spawnSync(
  process.execPath,
  [cliPath, 'install', '--with-deps', 'chromium', 'webkit'],
  {
    env: environment,
    stdio: 'inherit',
  }
);
process.exit(result.status === null ? 1 : result.status);

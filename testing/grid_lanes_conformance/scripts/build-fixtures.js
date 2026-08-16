const { spawnSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const { PACKAGE_ROOT, readFixtures } = require('./fixtures');

const configPath = path.join(PACKAGE_ROOT, 'lynx.config.mjs');
const rspeedyPackagePath = require.resolve('@lynx-js/rspeedy/package.json');
const rspeedyPath = path.join(
  path.dirname(rspeedyPackagePath),
  'bin',
  'rspeedy.js'
);
for (const fixture of readFixtures()) {
  const result = spawnSync(
    process.execPath,
    [rspeedyPath, 'build', '--config', configPath],
    {
      cwd: fixture.directory,
      env: {
        ...process.env,
        GRID_LANES_FIXTURE_ENTRY: path.join(fixture.directory, 'index.tsx'),
      },
      encoding: 'utf8',
    }
  );
  if (result.status !== 0) {
    process.stdout.write(result.stdout || '');
    process.stderr.write(result.stderr || '');
    process.exit(result.status || 1);
  }
  if (!fs.existsSync(fixture.bundlePath)) {
    throw new Error(`fixture did not produce ${fixture.bundlePath}`);
  }
  console.log(`built ${fixture.name}`);
}

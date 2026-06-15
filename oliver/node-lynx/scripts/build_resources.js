const childProcess = require('child_process');
const fs = require('fs');
const path = require('path');

const packageRoot = path.resolve(__dirname, '..');
const jsLibrariesRoot = path.resolve(packageRoot, '..', '..', 'js_libraries');
const lynxCoreRoot = path.join(jsLibrariesRoot, 'lynx-core');
const runtimeSharedRoot = path.join(jsLibrariesRoot, 'lynx-runtime-shared');
const pnpmCommand = process.platform === 'win32' ? 'pnpm.cmd' : 'pnpm';

function parseOutputDir(argv) {
  let outputDir = path.join(packageRoot, 'resources');
  for (let index = 0; index < argv.length; index++) {
    const arg = argv[index];
    if (arg === '--output') {
      const value = argv[index + 1];
      if (!value || value.startsWith('-')) {
        throw new Error('missing value for --output');
      }
      outputDir = path.resolve(process.cwd(), value);
      index++;
    } else if (arg.startsWith('--output=')) {
      const value = arg.slice('--output='.length);
      if (!value) {
        throw new Error('missing value for --output');
      }
      outputDir = path.resolve(process.cwd(), value);
    } else {
      throw new Error(`unknown argument: ${arg}`);
    }
  }
  return outputDir;
}

const outputDir = parseOutputDir(process.argv.slice(2));

childProcess.execFileSync(pnpmCommand, ['run', 'build'], {
  cwd: runtimeSharedRoot,
  stdio: 'inherit',
});

childProcess.execFileSync(pnpmCommand, ['run', 'build:android'], {
  cwd: lynxCoreRoot,
  stdio: 'inherit',
});

fs.mkdirSync(outputDir, { recursive: true });
for (const fileName of ['lynx_core.js', 'lynx_core_dev.js']) {
  fs.copyFileSync(
    path.join(lynxCoreRoot, 'output', fileName),
    path.join(outputDir, fileName)
  );
}

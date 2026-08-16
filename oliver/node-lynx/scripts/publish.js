#!/usr/bin/env node
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const childProcess = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');

const packageRoot = path.resolve(__dirname, '..');
const rootPackageJsonPath = path.join(packageRoot, 'package.json');
const platformPackages = [
  {
    id: 'darwin-arm64',
    dir: path.join(packageRoot, 'platform', 'darwin-arm64'),
    addon: path.join(packageRoot, 'platform', 'darwin-arm64', 'node_lynx.node'),
  },
  {
    id: 'linux-x64',
    dir: path.join(packageRoot, 'platform', 'linux-x64'),
    addon: path.join(packageRoot, 'platform', 'linux-x64', 'node_lynx.node'),
  },
];

function parseArgs(argv) {
  const options = {
    access: 'public',
    dryRun: false,
    packDir: '',
    provenance: true,
    skipBuild: false,
    tag: 'latest',
  };

  for (let index = 0; index < argv.length; index++) {
    const arg = argv[index];
    if (arg === '--dry-run') {
      options.dryRun = true;
    } else if (arg === '--no-provenance') {
      options.provenance = false;
    } else if (arg === '--skip-build') {
      options.skipBuild = true;
    } else if (arg === '--tag' || arg === '--access' || arg === '--pack-dir') {
      const value = argv[index + 1];
      if (!value || value.startsWith('-')) {
        throw new Error(`missing value for ${arg}`);
      }
      if (arg === '--tag') {
        options.tag = value;
      } else if (arg === '--access') {
        options.access = value;
      } else {
        options.packDir = value;
      }
      index++;
    } else if (arg.startsWith('--tag=')) {
      options.tag = arg.slice('--tag='.length);
    } else if (arg.startsWith('--access=')) {
      options.access = arg.slice('--access='.length);
    } else if (arg.startsWith('--pack-dir=')) {
      options.packDir = arg.slice('--pack-dir='.length);
    } else {
      throw new Error(`unknown argument: ${arg}`);
    }
  }

  if (!options.tag) {
    throw new Error('tag must not be empty');
  }

  return options;
}

function run(command, args, options = {}) {
  const result = childProcess.spawnSync(command, args, {
    cwd: options.cwd || packageRoot,
    encoding: 'utf8',
    env: process.env,
    stdio: options.capture ? ['ignore', 'pipe', 'inherit'] : 'inherit',
  });
  if (result.error) {
    throw result.error;
  }
  if (result.status !== 0) {
    throw new Error(
      `${command} ${args.join(' ')} failed with ${result.status}`
    );
  }
  return options.capture ? result.stdout.trim() : '';
}

function readPackageJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function validatePackageVersions(rootManifest, platformManifests) {
  const mismatches = platformManifests.filter(
    ({ manifest }) => manifest.version !== rootManifest.version
  );
  if (mismatches.length === 0) {
    return;
  }

  const versions = [
    `${rootManifest.name}@${rootManifest.version}`,
    ...platformManifests.map(
      ({ manifest }) => `${manifest.name}@${manifest.version}`
    ),
  ];
  throw new Error(
    `node-lynx package versions must match: ${versions.join(', ')}`
  );
}

function packPackage(cwd, packDir) {
  const output = run(
    'npm',
    ['pack', '--ignore-scripts', '--json', '--pack-destination', packDir],
    { cwd, capture: true }
  );
  const packResult = JSON.parse(output);
  if (!Array.isArray(packResult) || packResult.length !== 1) {
    throw new Error(`unexpected npm pack output for ${cwd}`);
  }
  return path.join(packDir, packResult[0].filename);
}

function listTarball(tarball) {
  return run('tar', ['-tzf', tarball], { capture: true }).split(/\r?\n/);
}

function readTarballFile(tarball, fileName) {
  return run('tar', ['-xOf', tarball, fileName], { capture: true });
}

function requireTarballEntry(tarball, entries, entry) {
  if (!entries.includes(`package/${entry}`)) {
    throw new Error(`${path.basename(tarball)} is missing ${entry}`);
  }
}

function publishTarball(tarball, options) {
  const publishArgs = [
    'publish',
    tarball,
    '--access',
    options.access,
    '--tag',
    options.tag,
  ];
  if (options.provenance) {
    publishArgs.push('--provenance');
  }

  if (options.dryRun) {
    console.log(`dry-run: npm ${publishArgs.join(' ')}`);
    return;
  }

  run('npm', publishArgs);
}

function main() {
  const options = parseArgs(process.argv.slice(2));
  const packDir = path.resolve(
    options.packDir ||
      fs.mkdtempSync(path.join(os.tmpdir(), 'node-lynx-publish-'))
  );
  fs.mkdirSync(packDir, { recursive: true });

  const rootPackageJson = fs.readFileSync(rootPackageJsonPath, 'utf8');
  const rootManifest = JSON.parse(rootPackageJson);
  const platformManifests = platformPackages.map((platformPackage) => ({
    ...platformPackage,
    manifest: readPackageJson(path.join(platformPackage.dir, 'package.json')),
  }));
  validatePackageVersions(rootManifest, platformManifests);

  for (const platformPackage of platformPackages) {
    if (!fs.existsSync(platformPackage.addon)) {
      throw new Error(`missing native addon: ${platformPackage.addon}`);
    }
  }

  if (!options.skipBuild) {
    run('pnpm', ['run', 'build:resources']);
    run('pnpm', ['exec', 'tsc']);
  }

  rootManifest.optionalDependencies = Object.fromEntries(
    platformManifests.map((platformPackage) => [
      platformPackage.manifest.name,
      platformPackage.manifest.version,
    ])
  );

  let restored = false;
  const restoreRootManifest = () => {
    if (!restored) {
      fs.writeFileSync(rootPackageJsonPath, rootPackageJson);
      restored = true;
    }
  };
  process.once('SIGINT', () => {
    restoreRootManifest();
    process.exit(130);
  });
  process.once('SIGTERM', () => {
    restoreRootManifest();
    process.exit(143);
  });

  try {
    fs.writeFileSync(
      rootPackageJsonPath,
      `${JSON.stringify(rootManifest, null, 2)}\n`
    );

    const platformTarballs = platformManifests.map((platformPackage) => {
      const tarball = packPackage(platformPackage.dir, packDir);
      const entries = listTarball(tarball);
      requireTarballEntry(tarball, entries, 'node_lynx.node');
      return tarball;
    });

    const rootTarball = packPackage(packageRoot, packDir);
    const rootEntries = listTarball(rootTarball);
    requireTarballEntry(rootTarball, rootEntries, 'dist/index.js');
    requireTarballEntry(rootTarball, rootEntries, 'resources/lynx_core.js');
    requireTarballEntry(rootTarball, rootEntries, 'skills/node-lynx/SKILL.md');

    const packedRootManifest = JSON.parse(
      readTarballFile(rootTarball, 'package/package.json')
    );
    for (const platformPackage of platformManifests) {
      const dependencyVersion =
        packedRootManifest.optionalDependencies[platformPackage.manifest.name];
      if (dependencyVersion !== platformPackage.manifest.version) {
        throw new Error(
          `${platformPackage.manifest.name} dependency is ${dependencyVersion}, expected ${platformPackage.manifest.version}`
        );
      }
    }
    for (const dependencyVersion of Object.values(
      packedRootManifest.optionalDependencies
    )) {
      if (String(dependencyVersion).startsWith('workspace:')) {
        throw new Error('root package tarball contains a workspace dependency');
      }
    }

    for (const tarball of platformTarballs) {
      publishTarball(tarball, options);
    }
    publishTarball(rootTarball, options);

    console.log(`node-lynx packages prepared in ${packDir}`);
  } finally {
    restoreRootManifest();
  }
}

if (require.main === module) {
  main();
}

module.exports = { validatePackageVersions };

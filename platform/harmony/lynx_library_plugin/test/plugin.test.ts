// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

import { afterEach, describe, expect, test } from 'vitest';

import {
  generateRegistrySource,
  harmonyLynxLibraryPlugin,
  prepareHarmonyAutolink,
  scanHarmonyLibraries,
} from '../src/index.js';
import type { HvigorTaskLike } from '../src/types.js';

const tempDirs: string[] = [];

afterEach(() => {
  for (const dir of tempDirs.splice(0)) {
    fs.rmSync(dir, { recursive: true, force: true });
  }
});

describe('Harmony Lynx library scanner', () => {
  test('scans hoisted scoped symlink packages and parses JSON5', () => {
    const workspace = makeTempDir();
    const project = path.join(workspace, 'apps', 'explorer');
    fs.mkdirSync(project, { recursive: true });
    const packageRoot = path.join(workspace, 'packages', 'demo-library');
    createLibrary(packageRoot, {
      npmName: '@scope/demo-library',
      ohPackageName: '@scope/demo_library',
      moduleName: 'demo_library',
      json5: true,
    });
    const scopeDir = path.join(workspace, 'node_modules', '@scope');
    fs.mkdirSync(scopeDir, { recursive: true });
    fs.symlinkSync(packageRoot, path.join(scopeDir, 'demo-library'), 'dir');

    const libraries = scanHarmonyLibraries(project);

    expect(libraries).toHaveLength(1);
    expect(libraries[0]).toMatchObject({
      npmName: '@scope/demo-library',
      ohPackageName: '@scope/demo_library',
      entry: 'Index.ets',
      moduleName: 'demo_library',
    });
    expect(libraries[0]?.harmonyDir).toBe(
      fs.realpathSync(path.join(packageRoot, 'harmony'))
    );
  });

  test('rejects a sourceDir symlink that escapes the npm package', () => {
    const workspace = makeTempDir();
    const project = path.join(workspace, 'app');
    const packageRoot = path.join(workspace, 'node_modules', 'bad-library');
    const outside = path.join(workspace, 'outside-harmony');
    fs.mkdirSync(project, { recursive: true });
    createHarmonyPackage(outside, '@scope/outside', 'outside');
    fs.mkdirSync(packageRoot, { recursive: true });
    write(packageRoot, 'package.json', '{"name":"bad-library"}');
    write(
      packageRoot,
      'lynx.lib.json',
      '{"platforms":{"harmony":{"sourceDir":"harmony"}}}'
    );
    fs.symlinkSync(outside, path.join(packageRoot, 'harmony'), 'dir');

    expect(() => scanHarmonyLibraries(project)).toThrow(
      'resolves outside npm package'
    );
  });

  test('rejects duplicate Harmony package names', () => {
    const workspace = makeTempDir();
    const project = path.join(workspace, 'app');
    fs.mkdirSync(project, { recursive: true });
    createLibrary(path.join(workspace, 'node_modules', 'library-a'), {
      npmName: 'library-a',
      ohPackageName: '@scope/shared',
      moduleName: 'library_a',
    });
    createLibrary(path.join(workspace, 'node_modules', 'library-b'), {
      npmName: 'library-b',
      ohPackageName: '@scope/shared',
      moduleName: 'library_b',
    });

    expect(() => scanHarmonyLibraries(project)).toThrow(
      "Duplicate Harmony package '@scope/shared'"
    );
  });

  test('rejects duplicate Harmony module names', () => {
    const workspace = makeTempDir();
    const project = path.join(workspace, 'app');
    fs.mkdirSync(project, { recursive: true });
    createLibrary(path.join(workspace, 'node_modules', 'library-a'), {
      npmName: 'library-a',
      ohPackageName: '@scope/library_a',
      moduleName: 'shared_module',
    });
    createLibrary(path.join(workspace, 'node_modules', 'library-b'), {
      npmName: 'library-b',
      ohPackageName: '@scope/library_b',
      moduleName: 'shared_module',
    });

    expect(() => scanHarmonyLibraries(project)).toThrow(
      "Duplicate Harmony module 'shared_module'"
    );
  });
});

describe('Harmony Lynx library generator', () => {
  test('generates stable imports and exposes only setupGlobal', () => {
    const source = generateRegistrySource([
      libraryInfo('z-library', '@scope/z_library', 'z_library'),
      libraryInfo('a-library', '@scope/a_library', 'a_library'),
    ]);

    expect(source.indexOf("from '@scope/a_library'")).toBeLessThan(
      source.indexOf("from '@scope/z_library'")
    );
    expect(source).toContain("packageName: 'a-library'");
    expect(source.match(/export /g)).toEqual(['export ']);
    expect(source).toContain('export function setupGlobal(): void');
  });

  test('prepares a deterministic Registry HAR and project wiring', () => {
    const project = makeTempDir();
    createEntryModule(project, 'entry');
    createLibrary(
      path.join(project, 'node_modules', '@scope', 'demo-library'),
      {
        npmName: '@scope/demo-library',
        ohPackageName: '@scope/demo_library',
        moduleName: 'demo_library',
      }
    );

    const first = prepareHarmonyAutolink({ projectRoot: project });
    const firstSource = read(first.outputDir, 'src/main/ets/Index.ets');
    const second = prepareHarmonyAutolink({ projectRoot: project });
    const buildProfile = JSON.parse(read(project, 'build-profile.json5')) as {
      modules: Array<{ name: string }>;
    };
    const consumerPackage = JSON.parse(
      read(project, 'entry/oh-package.json5')
    ) as {
      dependencies: Record<string, string>;
    };

    expect(second.libraries.map((library) => library.npmName)).toEqual([
      '@scope/demo-library',
    ]);
    expect(read(second.outputDir, 'src/main/ets/Index.ets')).toBe(firstSource);
    expect(read(second.outputDir, 'Index.ets')).toBe(
      "export { setupGlobal } from './src/main/ets/Index';\n"
    );
    expect(buildProfile.modules.map((module) => module.name)).toEqual([
      'entry',
      'demo_library',
      'lynx_library_registry',
    ]);
    expect(consumerPackage.dependencies['@lynx/lynx_library_registry']).toBe(
      'file:../.lynx/autolink/lynx_library_registry'
    );
  });

  test('rejects a Harmony source path mapped under another module name', () => {
    const project = makeTempDir();
    createEntryModule(project, 'entry');
    createLibrary(path.join(project, 'node_modules', 'demo-library'), {
      npmName: 'demo-library',
      ohPackageName: '@scope/demo_library',
      moduleName: 'demo_library',
    });
    const buildProfile = JSON.parse(read(project, 'build-profile.json5')) as {
      modules: Array<Record<string, unknown>>;
    };
    buildProfile.modules.push({
      name: 'wrong_name',
      srcPath: './node_modules/demo-library/harmony',
    });
    write(project, 'build-profile.json5', JSON.stringify(buildProfile));

    expect(() => prepareHarmonyAutolink({ projectRoot: project })).toThrow(
      'is already mapped as wrong_name; cannot map demo_library'
    );
  });

  test('registers generation before assembleHap', () => {
    const project = makeTempDir();
    createEntryModule(project, 'entry');
    let registeredTask: HvigorTaskLike | undefined;
    const plugin = harmonyLynxLibraryPlugin({ projectRoot: project });

    plugin.apply({
      registerTask(task) {
        registeredTask = task;
      },
    });

    expect(registeredTask?.name).toBe('generateLynxLibraryRegistry');
    expect(registeredTask?.postDependencies).toEqual(['assembleHap']);
    expect(
      fs.existsSync(
        path.join(
          project,
          '.lynx/autolink/lynx_library_registry/src/main/ets/Index.ets'
        )
      )
    ).toBe(true);
  });
});

interface LibraryFixtureOptions {
  npmName: string;
  ohPackageName: string;
  moduleName: string;
  json5?: boolean;
}

function createLibrary(root: string, options: LibraryFixtureOptions): void {
  write(root, 'package.json', JSON.stringify({ name: options.npmName }));
  write(root, 'lynx.lib.json', JSON.stringify({ platforms: { harmony: {} } }));
  createHarmonyPackage(
    path.join(root, 'harmony'),
    options.ohPackageName,
    options.moduleName,
    options.json5
  );
}

function createHarmonyPackage(
  root: string,
  ohPackageName: string,
  moduleName: string,
  json5 = false
): void {
  const ohPackage = json5
    ? `{
  // JSON5 comments and trailing commas are valid Harmony metadata.
  name: '${ohPackageName}',
  main: 'Index.ets',
}`
    : JSON.stringify({ name: ohPackageName, main: 'Index.ets' });
  write(root, 'oh-package.json5', ohPackage);
  write(root, 'Index.ets', 'export class LynxLibraryProviderImpl {}\n');
  write(
    root,
    'src/main/module.json5',
    JSON.stringify({
      module: {
        name: moduleName,
        type: 'har',
      },
    })
  );
}

function createEntryModule(projectRoot: string, moduleDir: string): void {
  write(
    projectRoot,
    'build-profile.json5',
    JSON.stringify({
      modules: [
        {
          name: 'entry',
          srcPath: `./${moduleDir}`,
          targets: [{ name: 'default', applyToProducts: ['default'] }],
        },
      ],
    })
  );
  write(
    projectRoot,
    `${moduleDir}/oh-package.json5`,
    JSON.stringify({
      name: 'entry',
      dependencies: {},
    })
  );
  write(
    projectRoot,
    `${moduleDir}/src/main/module.json5`,
    JSON.stringify({
      module: {
        name: 'entry',
        type: 'entry',
      },
    })
  );
}

function libraryInfo(
  npmName: string,
  ohPackageName: string,
  moduleName: string
) {
  return {
    npmName,
    packageDir: `/packages/${npmName}`,
    manifestPath: `/packages/${npmName}/lynx.lib.json`,
    harmonyDir: `/packages/${npmName}/harmony`,
    sourceDir: 'harmony',
    ohPackageName,
    entry: 'Index.ets',
    moduleName,
  };
}

function makeTempDir(): string {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'lynx-harmony-autolink-'));
  tempDirs.push(dir);
  return dir;
}

function write(root: string, relativePath: string, content: string): void {
  const filePath = path.join(root, relativePath);
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
  fs.writeFileSync(filePath, content);
}

function read(root: string, relativePath: string): string {
  return fs.readFileSync(path.join(root, relativePath), 'utf8');
}

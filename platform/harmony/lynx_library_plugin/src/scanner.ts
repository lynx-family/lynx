// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import fs from 'node:fs';
import path from 'node:path';

import JSON5 from 'json5';

import type { HarmonyLibraryInfo } from './types.js';

interface LynxManifest {
  platforms?: {
    harmony?: {
      sourceDir?: unknown;
    };
  };
}

interface NpmPackageManifest {
  name?: unknown;
}

interface OhPackageManifest {
  name?: unknown;
  main?: unknown;
}

interface HarmonyModuleManifest {
  module?: {
    name?: unknown;
    type?: unknown;
  };
}

export function scanHarmonyLibraries(startDir: string): HarmonyLibraryInfo[] {
  const manifests = new Map<string, string>();
  for (const nodeModulesDir of findNodeModulesDirs(path.resolve(startDir))) {
    for (const manifestPath of findManifestFiles(nodeModulesDir)) {
      const realManifestPath = fs.realpathSync(manifestPath);
      manifests.set(realManifestPath, realManifestPath);
    }
  }

  const librariesByNpmName = new Map<string, HarmonyLibraryInfo>();
  for (const manifestPath of manifests.values()) {
    const library = parseHarmonyLibrary(manifestPath);
    if (library != null && !librariesByNpmName.has(library.npmName)) {
      librariesByNpmName.set(library.npmName, library);
    }
  }
  const libraries = Array.from(
    librariesByNpmName.values()
  ).sort((left, right) => left.npmName.localeCompare(right.npmName));

  assertUnique(
    libraries,
    'Harmony package',
    (library) => library.ohPackageName
  );
  assertUnique(libraries, 'Harmony module', (library) => library.moduleName);
  return libraries;
}

export function parseJson5File<T>(filePath: string): T {
  try {
    return JSON5.parse(fs.readFileSync(filePath, 'utf8')) as T;
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    throw new Error(`Failed to parse ${filePath}: ${message}`);
  }
}

function findNodeModulesDirs(startDir: string): string[] {
  const result: string[] = [];
  let current = startDir;
  while (true) {
    const nodeModulesDir = path.join(current, 'node_modules');
    if (isDirectory(nodeModulesDir)) {
      result.push(fs.realpathSync(nodeModulesDir));
    }
    const parent = path.dirname(current);
    if (parent === current) {
      break;
    }
    current = parent;
  }
  return result;
}

function findManifestFiles(nodeModulesDir: string): string[] {
  const manifests: string[] = [];
  const entries = fs
    .readdirSync(nodeModulesDir, { withFileTypes: true })
    .sort((left, right) => left.name.localeCompare(right.name));
  for (const entry of entries) {
    if (entry.name.startsWith('.')) {
      continue;
    }
    const entryPath = path.join(nodeModulesDir, entry.name);
    if (entry.name.startsWith('@') && isDirectory(entryPath)) {
      const scopedEntries = fs
        .readdirSync(entryPath, { withFileTypes: true })
        .sort((left, right) => left.name.localeCompare(right.name));
      for (const scopedEntry of scopedEntries) {
        if (scopedEntry.name.startsWith('.')) {
          continue;
        }
        addManifest(manifests, path.join(entryPath, scopedEntry.name));
      }
    } else {
      addManifest(manifests, entryPath);
    }
  }
  return manifests;
}

function addManifest(manifests: string[], packageDir: string): void {
  if (!isDirectory(packageDir)) {
    return;
  }
  const manifestPath = path.join(packageDir, 'lynx.lib.json');
  if (fs.existsSync(manifestPath)) {
    manifests.push(manifestPath);
  }
}

function parseHarmonyLibrary(
  manifestPath: string
): HarmonyLibraryInfo | undefined {
  const manifest = parseJson5File<LynxManifest>(manifestPath);
  const harmony = manifest.platforms?.harmony;
  if (harmony == null) {
    return undefined;
  }
  if (typeof harmony !== 'object') {
    throw new Error(`Invalid platforms.harmony in ${manifestPath}`);
  }

  const packageDir = fs.realpathSync(path.dirname(manifestPath));
  const sourceDir =
    harmony.sourceDir == null
      ? 'harmony'
      : requireString(
          harmony.sourceDir,
          `platforms.harmony.sourceDir in ${manifestPath}`
        );
  const harmonyPath = path.resolve(packageDir, sourceDir);
  if (!isPathInside(packageDir, harmonyPath)) {
    throw new Error(
      `Harmony sourceDir '${sourceDir}' escapes npm package ${packageDir}`
    );
  }
  if (!isDirectory(harmonyPath)) {
    throw new Error(
      `Harmony sourceDir '${sourceDir}' does not exist for ${manifestPath}`
    );
  }

  const harmonyDir = fs.realpathSync(harmonyPath);
  if (!isPathInside(packageDir, harmonyDir)) {
    throw new Error(
      `Harmony sourceDir '${sourceDir}' resolves outside npm package ${packageDir}`
    );
  }

  const packageManifest = parseJson5File<NpmPackageManifest>(
    path.join(packageDir, 'package.json')
  );
  const npmName = requireString(
    packageManifest.name,
    `name in ${path.join(packageDir, 'package.json')}`
  );
  const ohPackagePath = path.join(harmonyDir, 'oh-package.json5');
  if (!fs.existsSync(ohPackagePath)) {
    throw new Error(`Missing oh-package.json5 in ${harmonyDir}`);
  }
  const ohPackage = parseJson5File<OhPackageManifest>(ohPackagePath);
  const ohPackageName = requireString(
    ohPackage.name,
    `name in ${ohPackagePath}`
  );
  const entry =
    ohPackage.main == null || ohPackage.main === ''
      ? 'Index.ets'
      : requireString(ohPackage.main, `main in ${ohPackagePath}`);
  const entryPath = path.resolve(harmonyDir, entry);
  if (!isPathInside(harmonyDir, entryPath) || !isFile(entryPath)) {
    throw new Error(
      `Harmony entry '${entry}' is missing or outside ${harmonyDir}`
    );
  }

  const moduleManifestPath = path.join(harmonyDir, 'src/main/module.json5');
  const moduleManifest = parseJson5File<HarmonyModuleManifest>(
    moduleManifestPath
  );
  const moduleName = requireString(
    moduleManifest.module?.name,
    `module.name in ${moduleManifestPath}`
  );
  if (moduleManifest.module?.type !== 'har') {
    throw new Error(
      `Harmony module ${moduleName} in ${moduleManifestPath} must have type 'har'`
    );
  }

  return {
    npmName,
    packageDir,
    manifestPath,
    harmonyDir,
    sourceDir,
    ohPackageName,
    entry,
    moduleName,
  };
}

function assertUnique(
  libraries: HarmonyLibraryInfo[],
  kind: string,
  key: (library: HarmonyLibraryInfo) => string
): void {
  const owners = new Map<string, string>();
  for (const library of libraries) {
    const value = key(library);
    const owner = owners.get(value);
    if (owner != null) {
      throw new Error(
        `Duplicate ${kind} '${value}' in ${owner} and ${library.manifestPath}`
      );
    }
    owners.set(value, library.manifestPath);
  }
}

function requireString(value: unknown, context: string): string {
  if (typeof value !== 'string' || value.trim().length === 0) {
    throw new Error(`Missing or invalid ${context}`);
  }
  return value.trim();
}

function isDirectory(filePath: string): boolean {
  try {
    return fs.statSync(filePath).isDirectory();
  } catch {
    return false;
  }
}

function isFile(filePath: string): boolean {
  try {
    return fs.statSync(filePath).isFile();
  } catch {
    return false;
  }
}

function isPathInside(parentDir: string, childPath: string): boolean {
  const relative = path.relative(parentDir, childPath);
  return (
    relative === '' ||
    (!relative.startsWith(`..${path.sep}`) &&
      relative !== '..' &&
      !path.isAbsolute(relative))
  );
}

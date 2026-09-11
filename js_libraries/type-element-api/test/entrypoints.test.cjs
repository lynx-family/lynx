// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const assert = require('assert').strict;
const fs = require('fs');
const os = require('os');
const path = require('path');
const ts = require('typescript');

const packageRoot = path.resolve(__dirname, '..');
const fixturesRoot = path.join(__dirname, 'fixtures');
const entrypoints = ['stable', 'experimental', 'internal', 'root'];

const compilerOptions = {
  noEmit: true,
  skipLibCheck: false,
  strict: true,
  target: ts.ScriptTarget.ES2017,
  types: [],
};

function formatDiagnostics(diagnostics) {
  const host = {
    getCanonicalFileName: (fileName) => fileName,
    getCurrentDirectory: () => packageRoot,
    getNewLine: () => ts.sys.newLine,
  };
  return ts.formatDiagnosticsWithColorAndContext(diagnostics, host);
}

function canonicalPath(fileName) {
  const realPath = ts.sys.realpath ? ts.sys.realpath(fileName) : fileName;
  return path.normalize(realPath);
}

function copyDirectory(source, destination) {
  fs.mkdirSync(destination, { recursive: true });
  for (const entry of fs.readdirSync(source, { withFileTypes: true })) {
    const sourcePath = path.join(source, entry.name);
    const destinationPath = path.join(destination, entry.name);
    if (entry.isDirectory()) {
      copyDirectory(sourcePath, destinationPath);
    } else {
      fs.copyFileSync(sourcePath, destinationPath);
    }
  }
}

function removeDirectory(directory) {
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    const entryPath = path.join(directory, entry.name);
    if (entry.isDirectory()) {
      removeDirectory(entryPath);
    } else {
      fs.unlinkSync(entryPath);
    }
  }
  fs.rmdirSync(directory);
}

function checkProgram(name, rootName, options, expectedEntry) {
  // Each entry point gets its own Program so global declarations from a wider
  // API level cannot make a narrower fixture pass accidentally.
  const program = ts.createProgram({
    rootNames: [rootName],
    options: { ...compilerOptions, ...options },
  });
  const diagnostics = ts.getPreEmitDiagnostics(program);
  assert.equal(diagnostics.length, 0, `${name} failed:\n${formatDiagnostics(diagnostics)}`);

  const normalizedEntry = canonicalPath(expectedEntry);
  assert.ok(
    program.getSourceFiles().some((sourceFile) => canonicalPath(sourceFile.fileName) === normalizedEntry),
    `${name} did not resolve to ${expectedEntry}`
  );
}

function checkNodeNextSelfReferences() {
  for (const entrypoint of entrypoints) {
    const declaration = entrypoint === 'root' ? 'index' : entrypoint;
    checkProgram(
      `NodeNext self-reference (${entrypoint})`,
      path.join(fixturesRoot, `${entrypoint}.test.ts`),
      {
        module: ts.ModuleKind.NodeNext,
        moduleResolution: ts.ModuleResolutionKind.NodeNext,
      },
      path.join(packageRoot, 'types', `${declaration}.d.ts`)
    );
  }
}

function checkLegacyNodeResolution() {
  const temporaryRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'type-element-api-'));
  try {
    const installedPackage = path.join(temporaryRoot, 'node_modules', '@lynx-js', 'type-element-api');
    fs.mkdirSync(installedPackage, { recursive: true });
    fs.copyFileSync(path.join(packageRoot, 'package.json'), path.join(installedPackage, 'package.json'));
    copyDirectory(path.join(packageRoot, 'types'), path.join(installedPackage, 'types'));

    for (const entrypoint of entrypoints) {
      const fixture = path.join(temporaryRoot, `${entrypoint}.test.ts`);
      const declaration = entrypoint === 'root' ? 'index' : entrypoint;
      fs.copyFileSync(path.join(fixturesRoot, `${entrypoint}.test.ts`), fixture);
      checkProgram(
        `Node resolution (${entrypoint})`,
        fixture,
        {
          module: ts.ModuleKind.ESNext,
          moduleResolution: ts.ModuleResolutionKind.Node10,
        },
        path.join(installedPackage, 'types', `${declaration}.d.ts`)
      );
    }
  } finally {
    removeDirectory(temporaryRoot);
  }
}

checkNodeNextSelfReferences();
checkLegacyNodeResolution();
console.log('Element API entry-point isolation checks passed.');

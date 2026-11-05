// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const path = require('path');

const lynxRoot = path.resolve(__dirname, '..', '..');
const ts = require(path.join(
  lynxRoot,
  'js_libraries',
  'type-element-api',
  'node_modules',
  'typescript'
));
const elementTypingPath = path.join(
  lynxRoot,
  'js_libraries',
  'types',
  'types',
  'common',
  'element',
  'element.d.ts'
);

const dedicatedProperties = new Set(['id', 'class', 'className', 'style']);
const typingOnlyProperties = new Set(['__lynx_timing_flag']);
const eventPropertyPattern = /^(?:main-thread:)?(?:capture-)?(?:global-)?(?:bind|catch)/;

function main() {
  const program = ts.createProgram([elementTypingPath], {
    moduleResolution: ts.ModuleResolutionKind.Node10,
    noEmit: true,
    skipLibCheck: true,
    target: ts.ScriptTarget.ES2020,
  });
  const diagnostics = ts.getPreEmitDiagnostics(program);
  if (diagnostics.length > 0) {
    const host = {
      getCanonicalFileName: (fileName) => fileName,
      getCurrentDirectory: () => lynxRoot,
      getNewLine: () => '\n',
    };
    throw new Error(ts.formatDiagnosticsWithColorAndContext(diagnostics, host));
  }

  const checker = program.getTypeChecker();
  const sourceFile = program.getSourceFile(elementTypingPath);
  const intrinsicElements = sourceFile.statements.find(
    (statement) =>
      ts.isInterfaceDeclaration(statement) &&
      statement.name.text === 'IntrinsicElements'
  );
  if (!intrinsicElements) {
    throw new Error(`IntrinsicElements not found in ${elementTypingPath}`);
  }

  const attributes = new Set();
  const elementsType = checker.getTypeAtLocation(intrinsicElements);
  for (const element of checker.getPropertiesOfType(elementsType)) {
    const declaration =
      element.valueDeclaration || (element.declarations || [])[0];
    if (!declaration) continue;
    const propsType = checker.getTypeOfSymbolAtLocation(element, declaration);
    for (const property of checker.getPropertiesOfType(propsType)) {
      const name = property.getName();
      if (
        !dedicatedProperties.has(name) &&
        !typingOnlyProperties.has(name) &&
        !eventPropertyPattern.test(name)
      ) {
        attributes.add(name);
      }
    }
  }

  process.stdout.write(
    `${JSON.stringify(
      {
        schema_version: 1,
        source: path.relative(lynxRoot, elementTypingPath),
        attributes: [...attributes].sort(),
      },
      null,
      2
    )}\n`
  );
}

main();

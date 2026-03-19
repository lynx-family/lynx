// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { Project } from 'ts-morph';
import { parseSourceFile } from './api-parser';
import { BaseObject } from './metadata-def';
import * as fs from 'fs';

interface MetadataObject {
  path: string;
  exports: string[];
}

function main() {
  const args = process.argv.slice(2);
  const fileIndex = args.indexOf('--file');

  if (fileIndex !== -1 && fileIndex + 1 < args.length) {
    const metadatObjectList: MetadataObject[] = JSON.parse(args[fileIndex + 1]);
    const project = new Project();
    const allMetadata: BaseObject[] = [];

    metadatObjectList.forEach((metadataObject) => {
      let fileContent: string;
      try {
        fileContent = fs.readFileSync(metadataObject.path, 'utf-8');
      } catch (e) {
        console.error(`Skipping unreadable file: ${metadataObject.path}`);
        return;
      }
      const modifiedContent = fileContent.replace(/\bstruct\b/g, 'class');

      const sourceFile = project.createSourceFile(
        metadataObject.path,
        modifiedContent,
        { overwrite: true }
      );

      const metadata = parseSourceFile(sourceFile, metadataObject.exports);
      allMetadata.push(...metadata);
    });

    console.log(JSON.stringify(allMetadata, null, 2));
  } else {
    console.error('Usage: ts-node cli.ts --file <file1>,<file2>,...');
    return;
  }
}

main();

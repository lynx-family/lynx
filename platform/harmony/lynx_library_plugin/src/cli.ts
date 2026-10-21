#!/usr/bin/env node

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import path from 'node:path';

import { prepareHarmonyAutolink } from './generator.js';
import type { PrepareHarmonyAutolinkOptions } from './types.js';

function parseArgs(argv: string[]): PrepareHarmonyAutolinkOptions {
  const options: PrepareHarmonyAutolinkOptions = {};
  for (let index = 0; index < argv.length; index += 1) {
    const arg = argv[index];
    if (arg === '--help' || arg === '-h') {
      printHelp();
      process.exit(0);
    }
    const value = argv[index + 1];
    switch (arg) {
      case '--project-root':
      case '-p':
        options.projectRoot = path.resolve(requiredValue(arg, value));
        index += 1;
        break;
      case '--consumer-module':
        options.consumerModule = requiredValue(arg, value);
        index += 1;
        break;
      case '--output-dir':
        options.outputDir = requiredValue(arg, value);
        index += 1;
        break;
      case '--lynx-dependency':
        options.lynxDependency = requiredValue(arg, value);
        index += 1;
        break;
      default:
        if (arg != null) {
          throw new Error(`Unknown argument: ${arg}`);
        }
    }
  }
  return options;
}

function requiredValue(name: string, value: string | undefined): string {
  if (value == null) {
    throw new Error(`${name} requires a value`);
  }
  return value;
}

function printHelp(): void {
  console.log(`Usage: lynx-harmony-autolink [options]

Generate and wire the Harmony global Autolink registry before ohpm install.

Options:
  --project-root, -p <path>  Harmony project root
  --consumer-module <path>  HAP module path relative to the project root
  --output-dir <path>       Generated Registry HAR path
  --lynx-dependency <spec>  @lynx/lynx OHPM dependency specifier`);
}

try {
  const result = prepareHarmonyAutolink(parseArgs(process.argv.slice(2)));
  console.log(
    `Generated Harmony Autolink registry for ${result.libraries.length} libraries`
  );
  console.log(`Registry HAR: ${result.outputDir}`);
} catch (error) {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
}

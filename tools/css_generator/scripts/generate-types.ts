import * as fs from 'node:fs';
import * as path from 'node:path';

import { validateCSSDefine } from './validate';

// Define CSSDefine interface for internal use
interface CSSDefine {
  name: string;
  type: string;
  values?: Array<{ value: string; version: string; desc?: string }>;
  default_value: string;
  keywords?: string[];
  is_shorthand: boolean;
}

function toCamelCase(str: string): string {
  return str.replace(/-([a-z])/g, (g) => g[1].toUpperCase());
}

function generateTypeDefinition(property: CSSDefine): string {
  const name = property.name;
  if (!name) return '';
  const camelName = toCamelCase(name);

  // Handle numeric types
  if (property.type === 'number' || property.type === 'integer') {
    return `${camelName}?: number;`;
  }

  // Handle enum types from values array
  if (property.values && Array.isArray(property.values)) {
    const values = property.values
      .map((v) => {
        if (typeof v === 'object' && v !== null && 'value' in v) {
          return `'${v.value}'`;
        }
        return `'${v}'`;
      })
      .join(' | ');
    return `${camelName}?: ${values};`;
  }

  // Handle properties with keywords
  if (property.keywords && Array.isArray(property.keywords)) {
    const keywords = property.keywords.map((k) => `'${k}'`).join(' | ');
    return `${camelName}?: ${keywords} | (string & {});`;
  }

  // Default to string type
  return `${camelName}?: string;`;
}

function generateShorthandsOrLonghands(
  groups: { [key: string]: CSSDefine[] },
  isShorthand: boolean
) {
  return Object.entries(groups)
    .map(([category, props]) => {
      const types = props
        .filter((p) => p.is_shorthand === isShorthand)
        .map((p) => {
          const camelName = toCamelCase(p.name);
          return JSON.stringify(camelName);
        })
        .join(' | ');
      return `\n  // ${category}\n  ${types}`;
    })
    .map((t) => t.trimEnd())
    .join(' |');
}

// Group properties by category for better organization
function groupProperties(
  properties: CSSDefine[]
): { [key: string]: CSSDefine[] } {
  const groups: { [key: string]: CSSDefine[] } = {
    layout: [],
    typography: [],
    visual: [],
    animation: [],
    other: [],
  };

  for (const prop of properties) {
    const name = prop.name?.toLowerCase() || '';
    if (
      name.includes('position') ||
      name.includes('display') ||
      name.includes('flex') ||
      name.includes('grid') ||
      name.includes('margin') ||
      name.includes('padding')
    ) {
      groups.layout.push(prop);
    } else if (
      name.includes('font') ||
      name.includes('text') ||
      name.includes('line')
    ) {
      groups.typography.push(prop);
    } else if (
      name.includes('color') ||
      name.includes('background') ||
      name.includes('border')
    ) {
      groups.visual.push(prop);
    } else if (name.includes('animation') || name.includes('transition')) {
      groups.animation.push(prop);
    } else {
      groups.other.push(prop);
    }
  }

  return groups;
}

function generateTypeDefinitions(): string {
  const cssDefinesDir = path.join(__dirname, '..', 'css_defines');
  const files = fs
    .readdirSync(cssDefinesDir)
    .filter((f: string) => f.endsWith('.json'));

  const cssDefines = files.map((file: string) => {
    const filePath = path.join(cssDefinesDir, file);
    const content = fs.readFileSync(filePath, 'utf-8');

    // Validate the file
    const validation = validateCSSDefine(content, filePath);
    if (!validation.valid) {
      throw new Error(
        `Invalid CSS define file: ${filePath}\n${JSON.stringify(
          validation.errors,
          null,
          2
        )}`
      );
    }

    return JSON.parse(content) as CSSDefine;
  });

  const groups = groupProperties(cssDefines);
  const typeDefinitions = Object.entries(groups)
    .map(([category, props]) => {
      const types = props
        .map(generateTypeDefinition)
        .filter(Boolean)
        .join('\n  ');
      return `  // ${category}\n  ${types}`;
    })
    .join('\n\n');

  const shorthandDefinitions = generateShorthandsOrLonghands(groups, true);
  const longhandsDefinitions = generateShorthandsOrLonghands(groups, false);

  return `// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * This file is auto-generated from CSS define files in the css_defines directory.
 * Each property's type is determined by:
 * 1. For enum types: Uses the values array from the CSS define file
 * 2. For properties with keywords: Uses the keywords array as enum values, with (string & {}) for open-ended types
 * 3. For other types: Uses string type
 */

export type CSSProperties = {
${typeDefinitions}
};

export type Shorthands = ${shorthandDefinitions};
export type Longhands = ${longhandsDefinitions};

export type CSSPropertiesWithShorthands = Pick<CSSProperties, Shorthands>;
export type CSSPropertiesWithLonghands = Pick<CSSProperties, Longhands>;
`;
}

// Create dist directory if it doesn't exist
const distDir = path.join(__dirname, '..', 'dist');
if (!fs.existsSync(distDir)) {
  fs.mkdirSync(distDir);
}

// Generate and write the type definitions
const typeDefinitions = generateTypeDefinitions();
fs.writeFileSync(path.join(distDir, 'csstype.d.ts'), typeDefinitions);

console.log('Type definitions generated successfully!');

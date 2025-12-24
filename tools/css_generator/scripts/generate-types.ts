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

/**
 * Determines if a property is Lynx-specific (not in standard CSS)
 */
function isLynxSpecific(propertyName: string): boolean {
  const lynxPrefixes = ['linear', 'relative', '-x-', 'X'];
  const lynxSpecificProps = [
    'adaptFontSize',
    'layoutAnimation',
    'implicitAnimation',
    'enterTransitionName',
    'exitTransitionName',
    'pauseTransitionName',
    'resumeTransitionName',
  ];
  
  return (
    lynxPrefixes.some(prefix => propertyName.startsWith(prefix)) ||
    lynxSpecificProps.some(prop => propertyName.includes(prop))
  );
}

function generateTypeDefinition(property: CSSDefine, mode: 'strict' | 'loose' = 'loose'): string {
  const name = property.name;
  if (!name) return '';
  const camelName = toCamelCase(name);
  const isLynxProp = isLynxSpecific(name);

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
    
    // For loose mode and non-Lynx-specific properties, allow broader types
    if (mode === 'loose' && !isLynxProp) {
      return `${camelName}?: ${values} | (string & {});`;
    }
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
  
  // Generate both strict and loose type definitions
  const looseTypeDefinitions = Object.entries(groups)
    .map(([category, props]) => {
      const types = props
        .map(p => generateTypeDefinition(p, 'loose'))
        .filter(Boolean)
        .join('\n    ');
      return `    // ${category}\n    ${types}`;
    })
    .join('\n\n');

  const strictTypeDefinitions = Object.entries(groups)
    .map(([category, props]) => {
      const types = props
        .map(p => generateTypeDefinition(p, 'strict'))
        .filter(Boolean)
        .join('\n    ');
      return `    // ${category}\n    ${types}`;
    })
    .join('\n\n');

  const shorthandDefinitions = generateShorthandsOrLonghands(groups, true);
  const longhandsDefinitions = generateShorthandsOrLonghands(groups, false);

  return `// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * This file is auto-generated from CSS define files in the css_defines directory.
 * 
 * Type System Design:
 * ===================
 * This module provides a gradual type strengthening system for inline styles:
 * 
 * 1. CSSProperties (default, loose mode):
 *    - Lynx-specific properties have strict types for better autocomplete
 *    - Standard CSS properties accept both specific values AND loose string types
 *    - Backward compatible with existing code
 *    - Recommended for most use cases
 * 
 * 2. StrictCSSProperties (strict mode):
 *    - All properties have strict enum types
 *    - Better type safety but may require code changes
 *    - Use when you want maximum type checking
 * 
 * 3. Compatibility with csstype:
 *    - CSSProperties can be used alongside CSS.Properties from 'csstype'
 *    - Uses the Modify helper type to override specific properties
 * 
 * Migration Path:
 * ===============
 * - Start with CSSProperties (loose mode) for backward compatibility
 * - Gradually migrate to StrictCSSProperties as your codebase matures
 * - Use utility types (Shorthands, Longhands) for specific use cases
 */

import type * as CSS from 'csstype';

export type Modify<T, R> = Omit<T, keyof R> & R;

/**
 * Lynx-specific CSS properties defined strictly.
 * These properties are not part of standard CSS.
 */
type LynxSpecificProperties = {
${Object.entries(groups)
    .map(([category, props]) => {
      const lynxProps = props.filter(p => isLynxSpecific(p.name));
      if (lynxProps.length === 0) return '';
      const types = lynxProps
        .map(p => generateTypeDefinition(p, 'strict'))
        .filter(Boolean)
        .join('\n    ');
      return `    // ${category}\n    ${types}`;
    })
    .filter(Boolean)
    .join('\n\n')}
};

/**
 * Override types for standard CSS properties that Lynx implements differently.
 * Uses loose typing to allow both Lynx-specific values and standard CSS values.
 */
type LynxCSSOverrides = {
${Object.entries(groups)
    .map(([category, props]) => {
      const nonLynxProps = props.filter(p => !isLynxSpecific(p.name));
      if (nonLynxProps.length === 0) return '';
      const types = nonLynxProps
        .map(p => generateTypeDefinition(p, 'loose'))
        .filter(Boolean)
        .join('\n    ');
      return `    // ${category}\n    ${types}`;
    })
    .filter(Boolean)
    .join('\n\n')}
};

/**
 * Default CSSProperties type with loose typing for backward compatibility.
 * Recommended for most use cases.
 * 
 * This type:
 * - Inherits from csstype's CSS.Properties for standard CSS support
 * - Overrides specific properties with Lynx implementations (loose mode)
 * - Adds Lynx-specific properties
 */
export type CSSProperties = Modify<
  CSS.Properties<string | number>,
  LynxCSSOverrides
> & LynxSpecificProperties;

/**
 * Strict CSSProperties type for maximum type safety.
 * Use when you want strict enum checking for all properties.
 * 
 * This type:
 * - Uses strict enum types for all properties
 * - Better autocomplete and type checking
 * - May require code changes for existing projects
 */
export type StrictCSSProperties = {
${strictTypeDefinitions}
};

export type Shorthands = ${shorthandDefinitions};
export type Longhands = ${longhandsDefinitions};

// Since \`Shorthands\` and \`Longhands\` are auto generated, there may be properties
// such as \`gridColumnSpan\` is not manually defined in \`CSSProperties\` yet.
// Use \`& keyof CSSProperties\` to ensure only the defined keys are included to avoid type error.
export type CSSPropertiesWithShorthands = Pick<CSSProperties, Shorthands & keyof CSSProperties>;
export type CSSPropertiesWithLonghands = Pick<CSSProperties, Longhands & keyof CSSProperties>;
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

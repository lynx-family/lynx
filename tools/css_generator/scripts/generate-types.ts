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
 * Get the TypeScript type string for a property.
 * @param property - The CSS property definition
 * @param mode - 'strict' for LynxCSSProperties (enum-only), 'loose' for CSSProperties overrides
 */
function getPropertyType(property: CSSDefine, mode: 'strict' | 'loose'): string | null {
  // Handle numeric types
  if (property.type === 'number' || property.type === 'integer') {
    return 'number';
  }

  // Handle enum types from values array
  if (property.values && Array.isArray(property.values) && property.values.length > 0) {
    const values = property.values
      .map((v) => {
        if (typeof v === 'object' && v !== null && 'value' in v) {
          return `'${v.value}'`;
        }
        return `'${v}'`;
      })
      .join(' | ');
    return values;
  }

  // Handle properties with keywords
  if (property.keywords && Array.isArray(property.keywords) && property.keywords.length > 0) {
    const keywords = property.keywords.map((k) => `'${k}'`).join(' | ');
    return `${keywords} | (string & {})`;
  }

  // For strict mode, default to string
  // For loose mode, return null to skip (use csstype's definition)
  if (mode === 'strict') {
    return 'string';
  }
  return null;
}

function generateTypeDefinition(property: CSSDefine): string {
  const name = property.name;
  if (!name) return '';
  const camelName = toCamelCase(name);

  const type = getPropertyType(property, 'strict');
  return `${camelName}?: ${type};`;
}

/**
 * Generate override entries for the Modify type (loose mode).
 * Only generates overrides for properties that have specific Lynx types.
 */
function generateOverrideEntry(property: CSSDefine): string | null {
  const name = property.name;
  if (!name) return null;
  const camelName = toCamelCase(name);

  const type = getPropertyType(property, 'loose');
  if (!type) return null;

  return `    ${camelName}?: ${type};`;
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

  // Generate strict type definitions (LynxCSSProperties)
  const strictTypeDefinitions = Object.entries(groups)
    .map(([category, props]) => {
      const types = props
        .map(generateTypeDefinition)
        .filter(Boolean)
        .join('\n  ');
      return `  // ${category}\n  ${types}`;
    })
    .join('\n\n');

  // Generate override entries for loose mode (CSSProperties)
  // Only include properties that have Lynx-specific types (enum values or keywords)
  const overrideEntries = Object.entries(groups)
    .map(([category, props]) => {
      const entries = props
        .map(generateOverrideEntry)
        .filter((entry): entry is string => entry !== null);
      if (entries.length === 0) return null;
      return `    // ${category}\n${entries.join('\n')}`;
    })
    .filter((entry): entry is string => entry !== null)
    .join('\n\n');

  const shorthandDefinitions = generateShorthandsOrLonghands(groups, true);
  const longhandsDefinitions = generateShorthandsOrLonghands(groups, false);

  return `// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * This file is auto-generated from CSS define files in the css_defines directory.
 *
 * ## Gradual Type Strictness
 *
 * This module exports two CSS property types with different strictness levels:
 *
 * ### CSSProperties (Default - Loose)
 * - Extends \`CSS.Properties\` from the \`csstype\` package for full web compatibility
 * - Overrides specific properties with Lynx-specific types where applicable
 * - Use this for maximum compatibility with existing CSS patterns
 * - Allows standard CSS values that may not be supported by Lynx runtime
 *
 * ### LynxCSSProperties (Strict)
 * - Contains ONLY properties defined in Lynx's css_defines
 * - Provides strict typing based on Lynx's actual supported values
 * - Use this when you want maximum type safety and only Lynx-supported properties
 * - Will cause compile errors for unsupported CSS properties
 *
 * ## Migration Path
 * 1. Start with \`CSSProperties\` for compatibility
 * 2. Gradually migrate to \`LynxCSSProperties\` for stricter type checking
 * 3. Use \`LynxCSSProperties\` in new code for best type safety
 */

import * as CSS from 'csstype';

// =============================================================================
// Utility Types
// =============================================================================

/**
 * Utility type to modify an existing type by overriding specific properties.
 * Used to extend csstype's Properties with Lynx-specific types.
 */
export type Modify<T, R> = Omit<T, keyof R> & R;

// =============================================================================
// LynxCSSProperties (Strict Mode)
// =============================================================================

/**
 * Strict CSS properties type generated from Lynx's css_defines.
 * Only includes properties that are explicitly supported by the Lynx runtime.
 * Use this type for maximum type safety when you want to ensure only
 * Lynx-supported CSS properties and values are used.
 *
 * @example
 * const style: LynxCSSProperties = {
 *   display: 'flex',      // ✓ Only Lynx-supported values
 *   flexDirection: 'row', // ✓ Strictly typed
 *   // display: 'inline', // ✗ Error: 'inline' not supported in Lynx
 * };
 */
export type LynxCSSProperties = {
${strictTypeDefinitions}
};

// =============================================================================
// CSSProperties (Loose Mode - Default)
// =============================================================================

/**
 * Lynx-specific property overrides for CSSProperties.
 * These override the loose csstype definitions with Lynx-specific types
 * while maintaining compatibility with standard CSS.
 */
type LynxCSSOverrides = {
${overrideEntries}
};

/**
 * CSS properties type that extends standard web CSS with Lynx-specific overrides.
 * This is the recommended default type for most use cases as it provides:
 * - Full compatibility with standard CSS properties from csstype
 * - Lynx-specific type refinements for properties that have limited values in Lynx
 * - Smooth migration path for web developers
 *
 * @example
 * const style: CSSProperties = {
 *   display: 'flex',           // ✓ Lynx-specific type
 *   color: 'red',              // ✓ Falls back to csstype
 *   backgroundColor: '#fff',   // ✓ Falls back to csstype
 * };
 */
export type CSSProperties = Modify<CSS.Properties<string | number>, LynxCSSOverrides>;

// =============================================================================
// Shorthand and Longhand Property Types
// =============================================================================

export type Shorthands = ${shorthandDefinitions};
export type Longhands = ${longhandsDefinitions};

// Since \`Shorthands\` and \`Longhands\` are auto generated, there may be properties
// such as \`gridColumnSpan\` is not manually defined in \`CSSProperties\` yet.
// Use \`& keyof CSSProperties\` to ensure only the defined keys are included to avoid type error.
export type CSSPropertiesWithShorthands = Pick<CSSProperties, Shorthands & keyof CSSProperties>;
export type CSSPropertiesWithLonghands = Pick<CSSProperties, Longhands & keyof CSSProperties>;

// Strict versions for LynxCSSProperties
export type LynxCSSPropertiesWithShorthands = Pick<LynxCSSProperties, Shorthands & keyof LynxCSSProperties>;
export type LynxCSSPropertiesWithLonghands = Pick<LynxCSSProperties, Longhands & keyof LynxCSSProperties>;
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

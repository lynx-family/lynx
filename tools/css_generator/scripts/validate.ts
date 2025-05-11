import type { ErrorObject } from 'ajv';
import Ajv from 'ajv';
import AjvErrors from 'ajv-errors';
import addFormats from 'ajv-formats';
import * as fs from 'node:fs';
import * as path from 'node:path';

interface ValidationResult {
  valid: boolean;
  errors?: ErrorObject[];
  file?: string;
}

interface ValidationSummary {
  total: number;
  valid: number;
  invalid: number;
  errors: ValidationResult[];
}

// Load and parse the schema
const schemaPath = path.join(
  __dirname,
  '..',
  'css_define_json_schema',
  'css_define_with_doc.schema.json'
);
const schema = JSON.parse(fs.readFileSync(schemaPath, 'utf-8'));

// Initialize Ajv
const ajv = new Ajv({ allErrors: true });
addFormats(ajv);
AjvErrors(ajv);
ajv.addKeyword('tsEnumNames');
ajv.addKeyword('tsName');
ajv.addKeyword('tsType');
const validate = ajv.compile(schema);

export function validateCSSDefine(
  content: string,
  filePath: string
): ValidationResult {
  try {
    const define = JSON.parse(content);
    const valid = validate(define);

    return {
      valid,
      errors: validate.errors || undefined,
      file: filePath,
    };
  } catch (error) {
    // Create a properly typed error object
    const parseError: ErrorObject = {
      keyword: 'parse',
      message: error instanceof Error ? error.message : 'Unknown error',
      params: {},
      schemaPath: '',
      instancePath: '',
      schema: undefined,
      parentSchema: undefined,
    };

    return {
      valid: false,
      errors: [parseError],
      file: filePath,
    };
  }
}

export function validateAllCSSDefines(): ValidationSummary {
  const cssDefinesDir = path.join(__dirname, '..', 'css_defines');
  const files = fs
    .readdirSync(cssDefinesDir)
    .filter((f: string) => f.endsWith('.json'));

  const results: ValidationResult[] = files.map((file: string) => {
    const filePath = path.join(cssDefinesDir, file);
    const content = fs.readFileSync(filePath, 'utf-8');
    return validateCSSDefine(content, file);
  });

  const summary: ValidationSummary = {
    total: results.length,
    valid: results.filter((r) => r.valid).length,
    invalid: results.filter((r) => !r.valid).length,
    errors: results.filter((r) => !r.valid),
  };

  return summary;
}

// If this file is run directly
if (require.main === module) {
  const summary = validateAllCSSDefines();

  console.log('\nCSS Define Validation Results:');
  console.log('=============================');
  console.log(`Total files: ${summary.total}`);
  console.log(`Valid files: ${summary.valid}`);
  console.log(`Invalid files: ${summary.invalid}`);

  if (summary.errors.length > 0) {
    console.log('\nValidation Errors:');
    console.log('=================');
    for (const result of summary.errors) {
      console.log(`\nFile: ${result.file}`);
      if (result.errors) {
        for (const error of result.errors) {
          console.log(`  - ${error.message}`);
          if (error.params) {
            console.log(`    Params: ${JSON.stringify(error.params)}`);
          }
        }
      }
    }
    process.exit(1);
  } else {
    console.log('\nAll files are valid!');
  }
}

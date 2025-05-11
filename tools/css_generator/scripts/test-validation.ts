import * as fs from 'node:fs';
import * as path from 'node:path';
import { validateCSSDefine } from './validate';

const testDir = path.join(__dirname, '..', 'test', 'css_defines');
const testFiles = fs.readdirSync(testDir).filter((f) => f.endsWith('.json'));

console.log('Running validation tests...\n');

let passed = 0;
let failed = 0;

for (const file of testFiles) {
  const filePath = path.join(testDir, file);
  const content = fs.readFileSync(filePath, 'utf-8');
  const result = validateCSSDefine(content, filePath);

  console.log(`Testing ${file}:`);
  console.log('Expected: Invalid');
  console.log(`Actual: ${result.valid ? 'Valid' : 'Invalid'}`);

  if (!result.valid) {
    console.log('Errors:');
    if (result.errors) {
      for (const error of result.errors) {
        console.log(`  - ${error.message}`);
        if (error.params) {
          console.log(`    Params: ${JSON.stringify(error.params)}`);
        }
      }
    }
    console.log('✓ Test passed\n');
    passed++;
  } else {
    console.log('✗ Test failed - Expected invalid file to be caught\n');
    failed++;
  }
}

console.log('Test Summary:');
console.log('=============');
console.log(`Total tests: ${testFiles.length}`);
console.log(`Passed: ${passed}`);
console.log(`Failed: ${failed}`);

if (failed > 0) {
  process.exit(1);
}

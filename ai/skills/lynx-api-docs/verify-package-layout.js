'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const packageRoot = __dirname;
const skillRoot = path.join(packageRoot, 'skills', 'using-lynx-api-docs');
const packageJson = readJson(path.join(packageRoot, 'package.json'));
const { generateIndex } = require('./generate-index.js');

assert.strictEqual(packageJson.name, '@lynx-js/lynx-api-docs');
assert.strictEqual(packageJson.license, 'Apache-2.0');
assert.deepStrictEqual(packageJson.publishConfig, {
  registry: 'https://registry.npmjs.org',
  access: 'public',
});
assert.deepStrictEqual(packageJson.repository, {
  type: 'git',
  url: 'https://github.com/lynx-family/lynx.git',
  directory: 'ai/skills/lynx-api-docs',
});

for (const relativePath of [
  'cli.js',
  'CHANGELOG.md',
  'evaluate.js',
  'evaluations/cases.jsonl',
  'generate-index.js',
  'LICENSE',
  'README.md',
  'skills/using-lynx-api-docs/INDEX.md',
  'skills/using-lynx-api-docs/SKILL.md',
  'skills/using-lynx-api-docs/topics.jsonl',
]) {
  assert.ok(fs.existsSync(path.join(packageRoot, relativePath)), `Missing ${relativePath}`);
}

assert.ok(!fs.existsSync(path.join(skillRoot, 'config')), 'Unexpected non-public config docs');
assert.ok(!fs.existsSync(path.join(skillRoot, 'AGENTS.md')), 'Unexpected redundant skill entry');
assert.ok(
  !fs.readdirSync(path.join(skillRoot, 'elements')).some((name) => /^x-.*\.md$/.test(name)),
  'Unexpected non-public element docs'
);

generateIndex(skillRoot, true);
assert.ok(packageJson.files.includes('generate-index.js'));
assert.strictEqual(packageJson.scripts['generate:index'], 'node ./generate-index.js');

for (const filePath of listFiles(packageRoot)) {
  if (!/\.(?:js|json|jsonl|md|yml|yaml)$/.test(filePath)) {
    continue;
  }

  const content = fs.readFileSync(filePath, 'utf8');
  const packageNames = content.match(/@[A-Za-z0-9_.-]+\/lynx-api-docs/g) || [];
  assert.ok(
    packageNames.every((name) => name === '@lynx-js/lynx-api-docs'),
    `${path.relative(packageRoot, filePath)} contains non-public package branding`
  );
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function listFiles(rootDir) {
  const files = [];
  for (const entry of fs.readdirSync(rootDir, { withFileTypes: true })) {
    if (entry.name === 'node_modules') {
      continue;
    }

    const fullPath = path.join(rootDir, entry.name);
    if (entry.isDirectory()) {
      files.push(...listFiles(fullPath));
    } else if (entry.isFile()) {
      files.push(fullPath);
    }
  }
  return files;
}

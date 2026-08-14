'use strict';

const fs = require('fs');
const path = require('path');

const META_DOCS = new Set([
  'INDEX.md',
  'SKILL.md',
]);
const TOPIC_FIELDS = new Set([
  'canonical_for',
  'group',
  'id',
  'keywords',
  'modes',
  'path',
  'platforms',
  'summary',
  'title',
]);

function readTopics(filePath) {
  const lines = fs.readFileSync(filePath, 'utf8').split(/\r?\n/);
  const topics = [];

  for (let index = 0; index < lines.length; index += 1) {
    const line = lines[index].trim();
    if (!line) {
      continue;
    }

    try {
      topics.push(JSON.parse(line));
    } catch (error) {
      throw new Error(`${filePath}:${index + 1}: ${error.message}`);
    }
  }

  return topics;
}

function validateTopics(topics, skillRoot) {
  const ids = new Set();
  const paths = new Set();
  const canonicalOwners = new Map();

  for (const topic of topics) {
    if (!topic || typeof topic !== 'object' || Array.isArray(topic)) {
      throw new Error(`Each topic must be a JSON object: ${JSON.stringify(topic)}`);
    }

    const unknownFields = Object.keys(topic).filter((field) => !TOPIC_FIELDS.has(field));
    if (unknownFields.length > 0) {
      throw new Error(
        `Topic "${topic.id || '<unknown>'}" has unsupported fields: ${unknownFields.join(', ')}`
      );
    }

    for (const field of ['id', 'group', 'title', 'path', 'summary']) {
      if (typeof topic[field] !== 'string' || !topic[field].trim()) {
        throw new Error(`Topic is missing a non-empty "${field}": ${JSON.stringify(topic)}`);
      }
    }

    for (const field of ['keywords', 'platforms', 'modes', 'canonical_for']) {
      if (
        !Array.isArray(topic[field]) ||
        topic[field].length === 0 ||
        topic[field].some((value) => typeof value !== 'string' || !value.trim())
      ) {
        throw new Error(
          `Topic "${topic.id}" must define a non-empty string array for "${field}".`
        );
      }
    }

    if (ids.has(topic.id)) {
      throw new Error(`Duplicate topic id: ${topic.id}`);
    }
    ids.add(topic.id);

    const portablePath = topic.path.replace(/[\\]+/g, '/');
    const normalizedPath = path.posix.normalize(portablePath);
    if (
      path.posix.isAbsolute(portablePath) ||
      path.win32.isAbsolute(portablePath) ||
      /^[A-Za-z]:/.test(portablePath) ||
      normalizedPath === '..' ||
      normalizedPath.startsWith('../')
    ) {
      throw new Error(`Topic "${topic.id}" has an unsafe path: ${topic.path}`);
    }
    if (paths.has(normalizedPath)) {
      throw new Error(`Multiple topics point to the same document: ${normalizedPath}`);
    }
    paths.add(normalizedPath);

    const targetPath = path.join(skillRoot, normalizedPath);
    if (!fs.existsSync(targetPath) || !fs.statSync(targetPath).isFile()) {
      throw new Error(`Topic "${topic.id}" points to a missing document: ${normalizedPath}`);
    }

    for (const canonicalTerm of topic.canonical_for) {
      const key = canonicalTerm.toLowerCase();
      if (canonicalOwners.has(key)) {
        throw new Error(
          `Canonical term "${canonicalTerm}" is owned by both "${canonicalOwners.get(key)}" and "${topic.id}".`
        );
      }
      canonicalOwners.set(key, topic.id);
    }
  }

  const unlistedDocs = listFiles(skillRoot)
    .filter((filePath) => filePath.endsWith('.md'))
    .map((filePath) => path.relative(skillRoot, filePath).replace(/[\\]+/g, '/'))
    .filter((relativePath) => !META_DOCS.has(relativePath))
    .filter((relativePath) => !paths.has(relativePath));

  if (unlistedDocs.length > 0) {
    throw new Error(`Reference documents missing from topics.jsonl: ${unlistedDocs.join(', ')}`);
  }
}

function renderIndex(topics) {
  const groups = new Map();
  for (const topic of topics) {
    if (!groups.has(topic.group)) {
      groups.set(topic.group, []);
    }
    groups.get(topic.group).push(topic);
  }

  const lines = [
    '# Lynx API Docs Index',
    '',
    'Use this file only to choose the smallest relevant reference. The installed package version defines the documentation baseline. Search [topics.jsonl](./topics.jsonl) by an exact property, element, symptom, or task keyword.',
    '',
    'Do not load every file in a group. Start with one canonical document and follow its related links only when the task requires more context.',
    '',
  ];

  for (const [group, entries] of groups) {
    lines.push(`## ${group}`, '');
    lines.push('| Topic | Read when | Platforms |');
    lines.push('| --- | --- | --- |');
    for (const topic of entries) {
      lines.push(
        `| [${escapeTable(topic.title)}](./${topic.path}) | ${escapeTable(topic.summary)} | ${topic.platforms.join(', ')} |`
      );
    }
    lines.push('');
  }

  lines.push(
    '## Deterministic lookup',
    '',
    'Search `topics.jsonl` before searching prose. Each `canonical_for` term has exactly one owner, and every reference Markdown file is represented by one topic entry.',
    ''
  );

  return `${lines.join('\n').trimEnd()}\n`;
}

function generateIndex(skillRoot, checkOnly) {
  const topicsPath = path.join(skillRoot, 'topics.jsonl');
  const indexPath = path.join(skillRoot, 'INDEX.md');
  const topics = readTopics(topicsPath);
  validateTopics(topics, skillRoot);
  const expected = renderIndex(topics);

  if (checkOnly) {
    const current = fs.existsSync(indexPath) ? fs.readFileSync(indexPath, 'utf8') : '';
    if (current !== expected) {
      throw new Error('INDEX.md is stale. Run node generate-index.js.');
    }
    return;
  }

  fs.writeFileSync(indexPath, expected, 'utf8');
}

function escapeTable(value) {
  return value.replace(/[|]/g, '\\|').replace(/\s+/g, ' ').trim();
}

function listFiles(rootDir) {
  const files = [];
  for (const entry of fs.readdirSync(rootDir, { withFileTypes: true })) {
    const fullPath = path.join(rootDir, entry.name);
    if (entry.isDirectory()) {
      files.push(...listFiles(fullPath));
    } else if (entry.isFile()) {
      files.push(fullPath);
    }
  }
  return files;
}

if (require.main === module) {
  try {
    const args = process.argv.slice(2);
    if (args.some((arg) => arg !== '--check')) {
      throw new Error(`Unknown argument: ${args.find((arg) => arg !== '--check')}`);
    }
    const skillRoot = path.join(__dirname, 'skills', 'using-lynx-api-docs');
    generateIndex(skillRoot, args.includes('--check'));
  } catch (error) {
    console.error(`Error: ${error.message}`);
    process.exitCode = 1;
  }
}

module.exports = {
  generateIndex,
  readTopics,
  renderIndex,
  validateTopics,
};

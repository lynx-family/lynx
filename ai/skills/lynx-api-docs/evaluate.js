'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const { readTopics, validateTopics } = require('./generate-index');

const packageRoot = __dirname;
const skillRoot = path.join(packageRoot, 'skills', 'using-lynx-api-docs');
const casesPath = path.join(packageRoot, 'evaluations', 'cases.jsonl');

main();

function main() {
  const topics = readTopics(path.join(skillRoot, 'topics.jsonl'));
  const cases = readTopics(casesPath);
  const topicsById = new Map(topics.map((topic) => [topic.id, topic]));
  const caseIds = new Set();

  validateTopics(topics, skillRoot);
  assert.ok(cases.length > 0, 'At least one evaluation case is required.');

  for (const evalCase of cases) {
    evaluateCase(evalCase, topicsById);
    assert.ok(!caseIds.has(evalCase.id), `Duplicate evaluation case: ${evalCase.id}`);
    caseIds.add(evalCase.id);
    console.log(`ok - ${evalCase.id}`);
  }
}

function evaluateCase(evalCase, topicsById) {
  assert.ok(
    evalCase && typeof evalCase === 'object' && !Array.isArray(evalCase),
    `Evaluation case must be a JSON object: ${JSON.stringify(evalCase)}`
  );

  for (const field of ['id', 'prompt', 'expected_topic']) {
    assert.strictEqual(
      typeof evalCase[field],
      'string',
      `Evaluation case must define a string "${field}": ${JSON.stringify(evalCase)}`
    );
    assert.ok(evalCase[field].trim(), `Evaluation case has an empty "${field}".`);
  }

  for (const field of ['query_terms', 'required_doc_terms']) {
    assert.ok(
      isNonEmptyStringArray(evalCase[field]),
      `Evaluation case "${evalCase.id}" must define "${field}".`
    );
  }
  assert.ok(
    evalCase.forbidden_doc_terms === undefined ||
      isNonEmptyStringArray(evalCase.forbidden_doc_terms),
    `Evaluation case "${evalCase.id}" has invalid "forbidden_doc_terms".`
  );

  const topic = topicsById.get(evalCase.expected_topic);
  assert.ok(topic, `Unknown expected topic in "${evalCase.id}": ${evalCase.expected_topic}`);

  const routingText = normalize([
    topic.id,
    topic.title,
    topic.summary,
    ...topic.keywords,
    ...topic.canonical_for,
  ].join('\n'));
  for (const term of evalCase.query_terms) {
    assert.ok(
      routingText.includes(normalize(term)),
      `Topic "${topic.id}" does not expose routing term "${term}" for "${evalCase.id}".`
    );
  }

  const doc = normalize(
    fs.readFileSync(path.join(skillRoot, topic.path), 'utf8')
  );
  for (const term of evalCase.required_doc_terms) {
    assert.ok(
      doc.includes(normalize(term)),
      `Document "${topic.path}" is missing required claim "${term}" for "${evalCase.id}".`
    );
  }
  for (const term of evalCase.forbidden_doc_terms || []) {
    assert.ok(
      !doc.includes(normalize(term)),
      `Document "${topic.path}" retains forbidden claim "${term}" for "${evalCase.id}".`
    );
  }
}

function isNonEmptyStringArray(value) {
  return (
    Array.isArray(value) &&
    value.length > 0 &&
    value.every((entry) => typeof entry === 'string' && entry.trim())
  );
}

function normalize(value) {
  return value.toLowerCase().replace(/\s+/g, ' ').trim();
}

const BOX_FIELDS = ['content', 'padding', 'border', 'margin'];
const EPSILON = 0.01;

function roundToLayoutAccuracy(value) {
  const rounded = Math.round((Number(value) + Number.EPSILON) * 100) / 100;
  return Object.is(rounded, -0) || Math.abs(rounded) < Number.EPSILON
    ? 0
    : rounded;
}

function normalizeBoxModel(model) {
  if (!model) {
    return null;
  }
  if (!Array.isArray(model.content) || model.content.length !== 8) {
    throw new Error('invalid content quad');
  }
  const contentWidth = Math.hypot(
    model.content[2] - model.content[0],
    model.content[3] - model.content[1]
  );
  const contentHeight = Math.hypot(
    model.content[4] - model.content[2],
    model.content[5] - model.content[3]
  );
  const normalized = {
    width: roundToLayoutAccuracy(contentWidth),
    height: roundToLayoutAccuracy(contentHeight),
  };
  for (const field of BOX_FIELDS) {
    if (!Array.isArray(model[field]) || model[field].length !== 8) {
      throw new Error(`invalid ${field} quad`);
    }
    normalized[field] = model[field].map(roundToLayoutAccuracy);
  }
  return normalized;
}

function compareGeometry(expected, actual, epsilon = EPSILON) {
  const differences = [];
  const expectedTags = Object.keys(expected).sort();
  const actualTags = Object.keys(actual).sort();
  for (const tag of expectedTags) {
    if (!actual[tag]) {
      differences.push({
        tag,
        field: 'node',
        expected: 'present',
        actual: 'missing',
      });
      continue;
    }
    for (const field of ['width', 'height']) {
      compareValue(
        differences,
        tag,
        field,
        expected[tag][field],
        actual[tag][field],
        epsilon
      );
    }
    for (const field of BOX_FIELDS) {
      for (let index = 0; index < 8; index += 1) {
        compareValue(
          differences,
          tag,
          `${field}[${index}]`,
          expected[tag][field][index],
          actual[tag][field][index],
          epsilon
        );
      }
    }
  }
  for (const tag of actualTags) {
    if (!expected[tag]) {
      differences.push({
        tag,
        field: 'node',
        expected: 'missing',
        actual: 'present',
      });
    }
  }
  return differences;
}

function compareValue(differences, tag, field, expected, actual, epsilon) {
  if (
    !Number.isFinite(expected) ||
    !Number.isFinite(actual) ||
    Math.abs(expected - actual) > epsilon
  ) {
    differences.push({ tag, field, expected, actual });
  }
}

function formatDifference(difference) {
  return `node #${difference.tag}: ${difference.field} ${difference.actual} vs ${difference.expected}`;
}

module.exports = {
  BOX_FIELDS,
  EPSILON,
  compareGeometry,
  formatDifference,
  normalizeBoxModel,
  roundToLayoutAccuracy,
};

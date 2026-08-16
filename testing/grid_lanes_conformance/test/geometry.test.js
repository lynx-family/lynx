const assert = require('assert');
const test = require('node:test');

const {
  compareGeometry,
  normalizeBoxModel,
  roundToLayoutAccuracy,
} = require('../scripts/geometry');

const quad = [0, 0, 10, 0, 10, 20, 0, 20];

test('roundToLayoutAccuracy matches replay policy', () => {
  assert.strictEqual(roundToLayoutAccuracy(4.44444), 4.44);
  assert.strictEqual(roundToLayoutAccuracy(-0), 0);
  assert.strictEqual(Object.is(roundToLayoutAccuracy(-0.001), -0), false);
});

test('normalizeBoxModel rounds every coordinate', () => {
  const model = normalizeBoxModel({
    width: 99,
    height: 99,
    content: [0, 0, 10.005, 0, 10.005, 20, 0, 20],
    padding: quad,
    border: quad,
    margin: quad,
  });
  assert.strictEqual(model.width, 10.01);
  assert.strictEqual(model.height, 20);
  assert.deepStrictEqual(model.content, [0, 0, 10.01, 0, 10.01, 20, 0, 20]);
});

test('compareGeometry reports actionable tag and field', () => {
  const expected = {
    card3: normalizeBoxModel({
      width: 10,
      height: 20,
      content: quad,
      padding: quad,
      border: quad,
      margin: quad,
    }),
  };
  const actual = JSON.parse(JSON.stringify(expected));
  actual.card3.content[1] = 3.5;
  assert.deepStrictEqual(compareGeometry(expected, actual), [
    {
      tag: 'card3',
      field: 'content[1]',
      expected: 0,
      actual: 3.5,
    },
  ]);
});

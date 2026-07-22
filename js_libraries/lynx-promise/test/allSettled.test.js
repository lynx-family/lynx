// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

'use strict';
/* eslint-disable */

// Standalone test runner for Promise.allSettled (no external test framework).
// Run with: node test/allSettled.test.js

var getPromise = require('../src/index').getPromise;

// Build the polyfilled Promise the same way the runtime wires it up.
var Promise = getPromise({
  setTimeout: setTimeout,
  clearTimeout: clearTimeout,
  onUnhandled: function() {
    /* swallow unhandled-rejection reporting during tests */
  },
});

var passed = 0;
var failed = 0;
var pending = 0;

function assert(cond, message) {
  if (cond) {
    passed++;
  } else {
    failed++;
    // eslint-disable-next-line no-console
    console.error('  ✗ FAIL: ' + message);
  }
}

function deepEqual(a, b) {
  return JSON.stringify(a) === JSON.stringify(b);
}

// Register an async test. Each test returns a Promise that resolves when its
// assertions are done.
var testQueue = [];
function test(name, fn) {
  pending++;
  testQueue.push(function() {
    return new Promise(function(resolve) {
      var before = failed;
      Promise.resolve()
        .then(fn)
        .then(
          function() {
            pending--;
            if (failed === before) {
              // eslint-disable-next-line no-console
              console.log('  ✓ ' + name);
            } else {
              // eslint-disable-next-line no-console
              console.log('  ✗ ' + name);
            }
            resolve();
          },
          function(err) {
            pending--;
            failed++;
            // eslint-disable-next-line no-console
            console.error('  ✗ ' + name + ' (threw): ' + (err && err.stack || err));
            resolve();
          }
        );
    });
  });
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

test('exists as a static function', function() {
  assert(typeof Promise.allSettled === 'function', 'Promise.allSettled should be a function');
});

test('empty iterable resolves with an empty array', function() {
  return Promise.allSettled([]).then(function(results) {
    assert(Array.isArray(results), 'result should be an array');
    assert(results.length === 0, 'result should be empty');
  });
});

test('all fulfilled', function() {
  return Promise.allSettled([
    Promise.resolve(1),
    Promise.resolve(2),
    Promise.resolve(3),
  ]).then(function(results) {
    assert(
      deepEqual(results, [
        { status: 'fulfilled', value: 1 },
        { status: 'fulfilled', value: 2 },
        { status: 'fulfilled', value: 3 },
      ]),
      'all fulfilled results mismatch: ' + JSON.stringify(results)
    );
  });
});

test('all rejected', function() {
  return Promise.allSettled([
    Promise.reject('e1'),
    Promise.reject('e2'),
  ]).then(function(results) {
    assert(
      deepEqual(results, [
        { status: 'rejected', reason: 'e1' },
        { status: 'rejected', reason: 'e2' },
      ]),
      'all rejected results mismatch: ' + JSON.stringify(results)
    );
  });
});

test('mixed fulfilled and rejected', function() {
  return Promise.allSettled([
    Promise.resolve('ok'),
    Promise.reject('bad'),
    Promise.resolve(42),
  ]).then(function(results) {
    assert(
      deepEqual(results, [
        { status: 'fulfilled', value: 'ok' },
        { status: 'rejected', reason: 'bad' },
        { status: 'fulfilled', value: 42 },
      ]),
      'mixed results mismatch: ' + JSON.stringify(results)
    );
  });
});

test('non-promise values are treated as fulfilled', function() {
  return Promise.allSettled([1, 'two', null, undefined, false]).then(function(results) {
    assert(
      deepEqual(results, [
        { status: 'fulfilled', value: 1 },
        { status: 'fulfilled', value: 'two' },
        { status: 'fulfilled', value: null },
        { status: 'fulfilled', value: undefined },
        { status: 'fulfilled', value: false },
      ]),
      'non-promise results mismatch: ' + JSON.stringify(results)
    );
  });
});

test('preserves order regardless of settle timing', function() {
  var slow = new Promise(function(resolve) {
    setTimeout(function() {
      resolve('slow');
    }, 30);
  });
  var fast = new Promise(function(resolve) {
    setTimeout(function() {
      resolve('fast');
    }, 5);
  });
  return Promise.allSettled([slow, fast]).then(function(results) {
    assert(
      deepEqual(results, [
        { status: 'fulfilled', value: 'slow' },
        { status: 'fulfilled', value: 'fast' },
      ]),
      'order not preserved: ' + JSON.stringify(results)
    );
  });
});

test('supports thenable', function() {
  var thenable = {
    then: function(onFulfilled) {
      onFulfilled('thenable-value');
    },
  };
  return Promise.allSettled([thenable]).then(function(results) {
    assert(
      deepEqual(results, [{ status: 'fulfilled', value: 'thenable-value' }]),
      'thenable result mismatch: ' + JSON.stringify(results)
    );
  });
});

test('never rejects even if every input rejects', function() {
  var rejectedSeen = false;
  return Promise.allSettled([Promise.reject('x'), Promise.reject('y')])
    .then(
      function() {
        assert(true, 'resolved as expected');
      },
      function() {
        rejectedSeen = true;
      }
    )
    .then(function() {
      assert(!rejectedSeen, 'allSettled must never reject');
    });
});

test('a throwing iterator rejects the returned promise instead of throwing', function() {
  var badIterable = {};
  badIterable[Symbol.iterator] = function() {
    throw new Error('boom');
  };

  var thrown = false;
  var promise;
  try {
    // Per spec, an input whose iterator throws must surface on the returned
    // promise, not synchronously at the call site.
    promise = Promise.allSettled(badIterable);
  } catch (e) {
    thrown = true;
  }

  assert(!thrown, 'allSettled must not throw synchronously for a bad iterable');
  assert(promise && typeof promise.then === 'function', 'allSettled must return a promise');

  var rejectedReason = null;
  return promise
    .then(
      function() {
        assert(false, 'promise should reject for a throwing iterator');
      },
      function(reason) {
        rejectedReason = reason;
      }
    )
    .then(function() {
      assert(
        rejectedReason instanceof Error && rejectedReason.message === 'boom',
        'rejection reason should be the error thrown by the iterator'
      );
    });
});

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

// eslint-disable-next-line no-console
console.log('Running Promise.allSettled tests...');

(function runNext() {
  if (testQueue.length === 0) {
    // Allow any trailing microtasks to flush before reporting.
    setTimeout(function() {
      // eslint-disable-next-line no-console
      console.log('\n' + passed + ' assertion(s) passed, ' + failed + ' failed.');
      if (failed > 0) {
        process.exit(1);
      }
    }, 50);
    return;
  }
  var next = testQueue.shift();
  next().then(runNext);
})();

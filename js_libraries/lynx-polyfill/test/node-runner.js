// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/* eslint-disable */
require('./tests');
let { tests } = global;
let result = {};

for (let key in tests) {
  let test = tests[key];
  try {
    if (typeof test === 'function') {
      result[key] = Boolean(test());
    } else
      result[key] = test.reduce(function(accumulator, $test) {
        return accumulator && Boolean($test());
      }, true);
  } catch (error) {
    result[key] = false;
  }
}

// eslint-disable-next-line no-console
console.log(JSON.stringify(result, null, '  '));

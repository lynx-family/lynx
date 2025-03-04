// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

'use strict';
/* eslint-disable */
var table = document.getElementById('table');
for (var key in window.tests) {
  var test = window.tests[key];
  var result = true;
  try {
    if (typeof test === 'function') {
      result = Boolean(test());
    } else {
      for (var i = 0; i < test.length; i++) result = result && Boolean(test[i].call(undefined));
    }
  } catch (error) {
    result = false;
  }
  var tr = document.createElement('tr');
  tr.className = result;
  var td1 = document.createElement('td');
  td1.innerHTML = key;
  tr.appendChild(td1);
  var td2 = document.createElement('td');
  td2.innerHTML = result ? 'not required' : 'required';
  tr.appendChild(td2);
  table.appendChild(tr);
}

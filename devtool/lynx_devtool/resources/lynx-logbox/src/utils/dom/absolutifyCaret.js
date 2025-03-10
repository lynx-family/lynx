/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

function removeNextBr(parent, component) {
  while (component != null && component.tagName.toLowerCase() !== 'br') {
    component = component.nextElementSibling;
  }
  if (component != null) {
    parent.removeChild(component);
  }
}

function absolutifyCaret(component) {
  const ccn = component.childNodes;
  for (let index = 0; index < ccn.length; ++index) {
    const c = ccn[index];
    if (c.tagName.toLowerCase() !== 'span') {
      continue;
    }
    const _text = c.innerText;
    if (_text == null) {
      continue;
    }
    const text = _text.replace(/\s/g, '');
    if (text !== '|^') {
      continue;
    }

    c.style.position = 'absolute';

    removeNextBr(component, c);
  }
}

export { absolutifyCaret };

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root, useState } from '@lynx-js/react';

import './index.css';

export function InputInsertText() {
  const [inputAValue, setInputAValue] = useState('');
  const [inputBValue, setInputBValue] = useState('');
  const [inputCValue, setInputCValue] = useState('');
  const [inputCount, setInputCount] = useState(0);
  const [lastInput, setLastInput] = useState('none');

  const handleInputA = (event: { detail: { value: string } }) => {
    setInputAValue(event.detail.value);
    setInputCount((count) => count + 1);
    setLastInput('input-a');
  };

  const handleInputB = (event: { detail: { value: string } }) => {
    setInputBValue(event.detail.value);
    setInputCount((count) => count + 1);
    setLastInput('input-b');
  };

  const handleInputC = (event: { detail: { value: string } }) => {
    setInputCValue(event.detail.value);
    setInputCount((count) => count + 1);
    setLastInput('input-c');
  };

  return (
    <view className="root">
      <view className="field">
        <text className="label">Input A</text>
        <input
          className="input-box"
          lynx-test-tag="insert-text-input-a"
          bindinput={handleInputA}
          placeholder="Waiting for DevTool text"
          show-soft-input-on-focus={false}
          text-color="#1f2933"
        />
        <text className="value-label">Input A value</text>
        <text className="value" lynx-test-tag="insert-text-value-a">
          {inputAValue}
        </text>
      </view>
      <view className="field">
        <text className="label">Input B</text>
        <input
          className="input-box"
          lynx-test-tag="insert-text-input-b"
          bindinput={handleInputB}
          placeholder="Waiting for DevTool text"
          show-soft-input-on-focus={false}
          text-color="#1f2933"
        />
        <text className="value-label">Input B value</text>
        <text className="value" lynx-test-tag="insert-text-value-b">
          {inputBValue}
        </text>
      </view>
      <view className="field">
        <text className="label">Input C</text>
        <input
          className="input-box"
          lynx-test-tag="insert-text-input-c"
          bindinput={handleInputC}
          placeholder="Waiting for DevTool text"
          show-soft-input-on-focus={false}
          text-color="#1f2933"
        />
        <text className="value-label">Input C value</text>
        <text className="value" lynx-test-tag="insert-text-value-c">
          {inputCValue}
        </text>
      </view>
      <view className="field">
        <text className="value-label">Input event count</text>
        <text className="value" lynx-test-tag="insert-text-input-count">
          {inputCount}
        </text>
        <text className="value-label">Last input target</text>
        <text className="value" lynx-test-tag="insert-text-last-input">
          {lastInput}
        </text>
      </view>
    </view>
  );
}

root.render(<InputInsertText />);

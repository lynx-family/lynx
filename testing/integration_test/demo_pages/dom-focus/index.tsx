// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root, useState } from '@lynx-js/react';

import './index.css';

export function DomFocus() {
  const [focusedInput, setFocusedInput] = useState('none');
  const [blurredInput, setBlurredInput] = useState('none');

  return (
    <scroll-view className="root" lynx-test-tag="root" scroll-y>
      <view className="section">
        <text className="label">Focused input</text>
        <text className="value" lynx-test-tag="focus-state">
          {focusedInput}
        </text>
      </view>
      <view className="section">
        <text className="label">Blurred input</text>
        <text className="value" lynx-test-tag="blur-state">
          {blurredInput}
        </text>
      </view>
      <view className="field">
        <text className="label">Input A</text>
        <input
          className="input"
          lynx-test-tag="focus-input-a"
          placeholder="input-a"
          show-soft-input-on-focus={false}
          bindfocus={() => {
            setFocusedInput('input-a');
          }}
          bindblur={() => {
            setBlurredInput('input-a');
          }}
        />
      </view>
      <view className="field">
        <text className="label">Input B</text>
        <input
          className="input"
          lynx-test-tag="focus-input-b"
          placeholder="input-b"
          show-soft-input-on-focus={false}
          bindfocus={() => {
            setFocusedInput('input-b');
          }}
          bindblur={() => {
            setBlurredInput('input-b');
          }}
        />
      </view>
    </scroll-view>
  );
}

root.render(<DomFocus />);

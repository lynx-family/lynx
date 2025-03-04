// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { KeyframeEffect as IKeyframeEffect } from '@lynx-js/types';
import Element from '../element';

export class KeyframeEffect implements IKeyframeEffect {
  public readonly target: Element;
  public readonly keyframes: Array<Record<string, any>>;
  public readonly options: Record<string, any>;

  constructor(
    target: Element,
    keyframes: Array<Record<string, any>>,
    options: Record<string, any>
  ) {
    this.target = target;
    this.keyframes = keyframes;
    this.options = options;
  }
}

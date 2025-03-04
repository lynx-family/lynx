// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { Animation as IAnimation } from '@lynx-js/types';
import { KeyframeEffect } from './effect';

export const enum AnimationOperation {
  START = 0,
  PLAY,
  PAUSE,
  CANCEL,
  FINISH,
}

export class Animation implements IAnimation {
  static count: number = 0;
  public readonly effect: KeyframeEffect;
  public readonly id: string;

  constructor(effect: KeyframeEffect) {
    this.effect = effect;
    this.id = '__lynx-inner-js-animation-' + Animation.count++;
  }

  cancel(): void {
    this.effect.target.cancelAnimate(this);
  }

  pause(): void {
    this.effect.target.pauseAnimate(this);
  }

  play(): void {
    this.effect.target.playAnimate(this);
  }
}

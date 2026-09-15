// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { ListAnimationStartEvent, ListAnimationEndEvent, ListAnimationCancelEvent, ListAnimationUpdateEvent } from '../../types';

declare const animationEnabled: boolean;
const handleListAnimationStart = (event: ListAnimationStartEvent) => {
  assertType<number>(event.detail.transactionId);
};
const handleListAnimationEnd = (event: ListAnimationEndEvent) => {};
const handleListAnimationCancel = (event: ListAnimationCancelEvent) => {};
const handleListAnimationUpdate = (event: ListAnimationUpdateEvent) => {
  assertType<number>(event.detail.progress);
};

<list
  experimental-use-new-update-animation={true}
  experimental-new-update-animation={{
    enable: animationEnabled,
    stages: [
      { type: 'remove', duration: 1000 },
      [
        { type: 'move', duration: 800 },
        { type: 'add', duration: 1200 },
      ],
    ],
  }}
  bindlistanimationstart={handleListAnimationStart}
  bindlistanimationend={handleListAnimationEnd}
  bindlistanimationcancel={handleListAnimationCancel}
  bindlistanimationupdate={handleListAnimationUpdate}
/>;

<list
  experimental-use-new-update-animation={true}
  experimental-new-update-animation={{
    enable: animationEnabled,
    stages: [
      { animations: ['remove'], durations: 1000 },
      { animations: ['move', 'add'], durations: [800, 1200] },
    ],
  }}
/>;

// @ts-expect-error implementation selection requires a boolean
<list experimental-use-new-update-animation="true" />;
// @ts-expect-error start handlers receive no progress field
<list bindlistanimationstart={handleListAnimationUpdate} />;

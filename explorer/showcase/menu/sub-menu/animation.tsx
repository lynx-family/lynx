// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root } from '@lynx-js/react';
import { ItemProps } from '@components/menu-item';
import { Menu, theme } from '@components/menu';

const ITEMS: ItemProps[] = [
  {
    title: 'Animate',
    description: 'An example shows how to use animate method',
    url: `file://lynx?local://showcase/animation/animate.lynx.bundle`,
  },
  {
    title: 'Keyframe Animation',
    description: 'An example shows how to use keyframes CSS at-rule',
    url:
      'file://lynx?local://showcase/animation/keyframe_animation.lynx.bundle',
  },
  {
    title: 'Keyframe Rotate',
    description: 'An example shows how to use keyframes rotate',
    url: 'file://lynx?local://showcase/animation/keyframe_rotate.lynx.bundle',
  },
  {
    title: 'Keyframe Spring',
    description: 'An example shows how to use keyframes spring animation',
    url: 'file://lynx?local://showcase/animation/keyframe_spring.lynx.bundle',
  },
  {
    title: 'Toggle Transitions',
    description: 'An example shows how to toggle animation',
    url:
      'file://lynx?local://showcase/animation/toggle_transition_demo.lynx.bundle',
  },
  {
    title: 'Transitions Animation',
    description: 'An example shows how to use CSS transitions',
    url:
      'file://lynx?local://showcase/animation/transition_animation.lynx.bundle',
  },
];

root.render(<Menu items={ITEMS} />);

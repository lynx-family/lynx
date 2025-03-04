// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root } from '@lynx-js/react';
import { ItemProps } from '@components/menu-item';
import { Menu } from '@components/menu';

const ITEMS: ItemProps[] = [
  {
    title: 'Direction',
    description: 'An example shows how to set direction',
    url: 'file://lynx?local://showcase/layout/direction.lynx.bundle',
  },
  {
    title: 'Flex grow',
    description:
      'An example shows how different flex-grow values assigned to flex items in a flex container',
    url: 'file://lynx?local://showcase/layout/flex_grow.lynx.bundle',
  },
  {
    title: 'Flex shrink',
    description:
      'An example shows how flex items shrink based on their flex-shrink values',
    url: 'file://lynx?local://showcase/layout/flex_shrink.lynx.bundle',
  },
  {
    title: 'Grid',
    description:
      'An example shows how elements are arranged in rows and columns within a grid container.',
    url: 'file://lynx?local://showcase/layout/grid.lynx.bundle',
  },
  {
    title: 'Linear',
    description: 'An example shows a linear direction container',
    url: 'file://lynx?local://showcase/layout/linear.lynx.bundle',
  },
  {
    title: 'Relative',
    description:
      'An example shows how an element is positioned relative to its normal position',
    url: 'file://lynx?local://showcase/layout/relative.lynx.bundle',
  },
  {
    title: 'Sizing',
    description:
      'An example shows how different sizing properties control the dimensions of node',
    url: 'file://lynx?local://showcase/layout/sizing.lynx.bundle',
  },
];

root.render(<Menu items={ITEMS} />);

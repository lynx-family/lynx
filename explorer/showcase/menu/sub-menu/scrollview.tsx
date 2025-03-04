// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root } from '@lynx-js/react';
import { ItemProps, MenuItem } from '@components/menu-item';
import { Menu } from '@components/menu';

const ITEMS: ItemProps[] = [
  {
    title: 'Base',
    description: 'An example shows how to use scroll-view component',
    url: 'file://lynx?local://showcase/scroll-view/base.lynx.bundle',
  },
  {
    title: 'Event',
    description: 'An example shows how to use event in scroll-view component',
    url: 'file://lynx?local://showcase/scroll-view/event.lynx.bundle',
  },
  {
    title: 'Sticky',
    description:
      'An example shows how to use sticky property in scroll-view component',
    url: 'file://lynx?local://showcase/scroll-view/sticky.lynx.bundle',
  },
  {
    title: 'Vertical',
    description:
      'An example shows a scroll-view component scroll in vertical direction',
    url: 'file://lynx?local://showcase/scroll-view/vertical.lynx.bundle',
  },
];

root.render(<Menu items={ITEMS} />);

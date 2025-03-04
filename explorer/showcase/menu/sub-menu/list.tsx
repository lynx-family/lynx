// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root } from '@lynx-js/react';
import { ItemProps, MenuItem } from '@components/menu-item';
import { Menu } from '@components/menu';

const ITEMS: ItemProps[] = [
  {
    title: 'Base',
    description: 'An example shows how to use list component',
    url: 'file://lynx?local://showcase/list/base.lynx.bundle',
  },
  {
    title: 'Flow',
    description:
      'An example shows how to set flow layout type for list component',
    url: 'file://lynx?local://showcase/list/flow.lynx.bundle',
  },
  {
    title: 'ItemSnap',
    description:
      'An example shows how to set item-snap property for list component',
    url: 'file://lynx?local://showcase/list/itemSnap.lynx.bundle',
  },
  {
    title: 'LoadMore',
    description:
      'An example shows how to implement the load more nodes feature',
    url: 'file://lynx?local://showcase/list/loadMore.lynx.bundle',
  },
  {
    title: 'Sticky',
    description: 'An example shows the effect of sticky in list component',
    url: 'file://lynx?local://showcase/list/sticky.lynx.bundle',
  },
  {
    title: 'Waterfall',
    description:
      'An example shows how to use list component with waterfall type',
    url: 'file://lynx?local://showcase/list/waterfall.lynx.bundle',
  },
];

root.render(<Menu items={ITEMS} />);

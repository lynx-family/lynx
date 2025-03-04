// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root } from '@lynx-js/react';
import { ItemProps, MenuItem } from '@components/menu-item';
import { Menu } from '@components/menu';

const ITEMS: ItemProps[] = [
  {
    title: 'Custom font',
    description: 'An example shows how to custom font in text component',
    url: 'file://lynx?local://showcase/text/custom_font.lynx.bundle',
  },
  {
    title: 'Text event',
    description: 'An example shows how to use text event',
    url: 'file://lynx?local://showcase/text/text_event.lynx.bundle',
  },
  {
    title: 'Text inline image',
    description: 'An example shows how to inline image in text component',
    url: 'file://lynx?local://showcase/text/inline_image.lynx.bundle',
  },
  {
    title: 'Text inline text',
    description: 'An example shows how to inline text in text component',
    url: 'file://lynx?local://showcase/text/inline_text.lynx.bundle',
  },
  {
    title: 'Text inline truncation',
    description: 'An example shows how to use inline-truncation component',
    url: 'file://lynx?local://showcase/text/inline_truncation.lynx.bundle',
  },
  {
    title: 'Text inline view',
    description: 'An example shows how to use inline-view component',
    url: 'file://lynx?local://showcase/text/inline_view.lynx.bundle',
  },
  {
    title: 'Text layout',
    description: 'An example shows how to set layout for text component',
    url: 'file://lynx?local://showcase/text/text_layout.lynx.bundle',
  },
  {
    title: 'Text style',
    description:
      'An example shows how to set CSS properties for text component',
    url: 'file://lynx?local://showcase/text/text_style.lynx.bundle',
  },
  {
    title: 'Shadow and stroke',
    description:
      'An example shows how to set shadow and stroke property for text component',
    url: 'file://lynx?local://showcase/text/shadow_and_stroke.lynx.bundle',
  },
];

root.render(<Menu items={ITEMS} />);

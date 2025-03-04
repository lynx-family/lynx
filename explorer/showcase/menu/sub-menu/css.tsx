// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root } from '@lynx-js/react';
import { ItemProps, MenuItem } from '@components/menu-item';
import { Menu } from '@components/menu';

const ITEMS: ItemProps[] = [
  {
    title: 'Background',
    description: 'An example shows how to use background CSS property',
    url: 'file://lynx?local://showcase/css/bg.lynx.bundle',
  },
  {
    title: 'Background gradient',
    description: 'An example shows how to use background gradient CSS function',
    url: 'file://lynx?local://showcase/css/bg_gradient.lynx.bundle',
  },
  {
    title: 'Background image',
    description: 'An example shows how to use background-image CSS property',
    url: 'file://lynx?local://showcase/css/bg_image.lynx.bundle',
  },
  {
    title: 'Background radial gradient',
    description: 'An example shows how to use radial-gradient CSS function',
    url: 'file://lynx?local://showcase/css/bg_radial.lynx.bundle',
  },
  {
    title: 'Border',
    description: 'An example shows how to use border CSS property',
    url: 'file://lynx?local://showcase/css/border.lynx.bundle',
  },
  {
    title: 'Border background shadow',
    description:
      'An example shows how to use box background shadow CSS property',
    url:
      'file://lynx?local://showcase/css/border_background_shadow.lynx.bundle',
  },
  {
    title: 'Border radius',
    description: 'An example shows how to use border-radius CSS property',
    url: 'file://lynx?local://showcase/css/border_radius.lynx.bundle',
  },
  {
    title: 'Box shadow',
    description: 'An example shows how to use box-shadow CSS property',
    url: 'file://lynx?local://showcase/css/box_shadow.lynx.bundle',
  },
  {
    title: 'Cascade',
    description: 'An example shows how to use CSS Cascade',
    url: 'file://lynx?local://showcase/css/cascade_guide.lynx.bundle',
  },
  {
    title: 'Class',
    description: 'An example shows how to use class in CSS',
    url: 'file://lynx?local://showcase/css/class_guide.lynx.bundle',
  },
  {
    title: 'ClipPath',
    description: 'An example shows how to use clip-path',
    url: 'file://lynx?local://showcase/css/clip_path_super_ellipse.lynx.bundle',
  },
  {
    title: 'Filter',
    description: 'An example shows how to use filter CSS property',
    url: 'file://lynx?local://showcase/css/filter.lynx.bundle',
  },
  {
    title: 'Flexible box layout ',
    description: 'An example shows how to use CSS flexible box layout module',
    url: 'file://lynx?local://showcase/css/flex_layout.lynx.bundle',
  },
  {
    title: 'Grid layout',
    description: 'An example shows how to use CSS grid layout module',
    url: 'file://lynx?local://showcase/css/grid_layout.lynx.bundle',
  },
  {
    title: 'Linear layout',
    description: 'An example shows how to use CSS linear layout module',
    url: 'file://lynx?local://showcase/css/linear_layout.lynx.bundle',
  },
  {
    title: 'Mask Image',
    description: 'An example shows how to use mask-image CSS property',
    url:
      'file://lynx?local://showcase/css/mask_image_circle_gradient.lynx.bundle',
  },
  {
    title: 'Relative layout',
    description: 'An example shows how to use CSS relative layout module',
    url: 'file://lynx?local://showcase/css/relative_layout.lynx.bundle',
  },
  {
    title: 'Variables',
    description: 'An example shows how to use variables property',
    url: 'file://lynx?local://showcase/css/css_variable_theme.lynx.bundle',
  },
];

root.render(<Menu items={ITEMS} />);

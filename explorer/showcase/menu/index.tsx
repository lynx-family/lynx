// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root } from '@lynx-js/react';
import { ItemProps } from '@components/menu-item';
import { Menu } from '@components/menu';

import AnimationIcon from '@assets/images/animation.png?inline';
import AnimationIconDark from '@assets/images/animation-dark.png?inline';
import CSSIcon from '@assets/images/css.png?inline';
import CSSIconDark from '@assets/images/css-dark.png?inline';
import ImageIcon from '@assets/images/image.png?inline';
import ImageIconDark from '@assets/images/image-dark.png?inline';
import LazyBundleIcon from '@assets/images/lazy-bundle.png?inline';
import ScrollViewIcon from '@assets/images/scroll-view.png?inline';
import ScrollViewIconDark from '@assets/images/scroll-view-dark.png?inline';
import TextIcon from '@assets/images/text.png?inline';
import TextIconDark from '@assets/images/text-dark.png?inline';
import ViewIcon from '@assets/images/view.png?inline';
import ViewIconDark from '@assets/images/view-dark.png?inline';

const ITEMS: ItemProps[] = [
  {
    title: 'Animation',
    description: 'Some examples show how to use CSS Animation',
    url: 'file://lynx?local://showcase/menu/animation.lynx.bundle',
    icon: {
      dark: AnimationIconDark,
      light: AnimationIcon,
    },
  },
  {
    title: 'CSS',
    description: 'Some examples show how to use different CSS properties',
    url: 'file://lynx?local://showcase/menu/css.lynx.bundle',
    icon: {
      dark: CSSIconDark,
      light: CSSIcon,
    },
  },
  {
    title: 'Fetch API',
    description: 'An example shows how fetch API work in ReactLynx',
    url: 'file://lynx?local://showcase/fetch/main.lynx.bundle',
  },
  {
    title: 'Image',
    description: 'An example for image component.',
    url: 'file://lynx?local://showcase/image/main.lynx.bundle',
    icon: {
      dark: ImageIconDark,
      light: ImageIcon,
    },
  },
  {
    title: 'Layout',
    description: 'Some examples show the layout related styling',
    url: 'file://lynx?local://showcase/menu/layout.lynx.bundle',
  },
  {
    title: 'Lazy Bundle',
    description: 'An example shows how to use lazy loading component',
    url: 'file://lynx?local://showcase/lazy-bundle/main.lynx.bundle',
    icon: {
      light: LazyBundleIcon,
    },
  },
  {
    title: 'List',
    description: 'Some examples show how to use list component',
    url: 'file://lynx?local://showcase/menu/list.lynx.bundle',
  },
  {
    title: 'ScrollView',
    description: 'Some examples show how to use scrollable container',
    url: 'file://lynx?local://showcase/menu/scrollview.lynx.bundle',
    icon: {
      dark: ScrollViewIconDark,
      light: ScrollViewIcon,
    },
  },
  {
    title: 'Text',
    description: 'Some examples show how to use text and inline-text',
    url: 'file://lynx?local://showcase/menu/text.lynx.bundle',
    icon: {
      dark: TextIconDark,
      light: TextIcon,
    },
  },
  {
    title: 'View',
    description: 'An example shows how to use view',
    url: 'file://lynx?local://showcase/view/main.lynx.bundle',
    icon: {
      dark: ViewIconDark,
      light: ViewIcon,
    },
  },
];

root.render(<Menu items={ITEMS} />);

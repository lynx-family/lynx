// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';
import { NoProps } from '../props';
import { ComponentProps } from './component';
import { FilterImageProps } from './filter-image';
import { ImageProps } from './image';
import { ListItemProps, ListProps, ListRowProps, ListUIMethods } from './list';
import { PageProps } from './page';
import { ScrollViewProps, ScrollViewUIMethods } from './scroll-view';
import { TextProps } from './text';
import { ViewProps } from './view';

export interface UIMethods {
  'list': ListUIMethods;
  'scroll-view': ScrollViewUIMethods;
}

// add also to global.JSX.IntrinsicElements
export interface IntrinsicElements {
  'component': ComponentProps;
  'filter-image': FilterImageProps;
  'image': ImageProps;
  'inline-image': ImageProps;
  'inline-text': TextProps;
  'inline-truncation': NoProps;
  'list': ListProps;
  'list-item': ListItemProps;
  'list-row': ListRowProps;
  'page': PageProps;
  'scroll-view': ScrollViewProps;
  'text': TextProps;
  'view': ViewProps;
  'raw-text': StandardProps & { text: number | string };
}

declare module 'react' {
  namespace JSX {
    // Should copy from above IntrinsicElements
    interface IntrinsicElements {
      'component': ComponentProps;
      'filter-image': FilterImageProps;
      'image': ImageProps;
      'inline-image': ImageProps;
      'inline-text': TextProps;
      'inline-truncation': NoProps;
      'list': ListProps;
      'list-item': ListItemProps;
      'list-row': ListRowProps;
      'page': PageProps;
      'scroll-view': ScrollViewProps;
      'text': TextProps;
      'view': ViewProps;
      'raw-text': StandardProps & { text: number | string };
    }
  }
}

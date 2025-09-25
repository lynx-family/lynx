// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType, describe, it } from 'vitest';
import { ListItemProps, StandardProps } from '../../../types';

declare const listItemProps: ListItemProps;

describe('ListItemProps type test', () => {
  it('should extend StandardProps', () => {
    assertType<StandardProps>(listItemProps);
  });

  it('should have recyclable property', () => {
    assertType<ListItemProps>({
      'item-key': 'test',
      recyclable: true,
    });
    assertType<ListItemProps>({
      'item-key': 'test',
      recyclable: false,
    });
    assertType<ListItemProps>({
      'item-key': 'test',
      recyclable: undefined,
    });
    assertType<ListItemProps>({
      'item-key': 'test',
    });
    assertType<ListItemProps>({
      'item-key': 'test',
      // @ts-expect-error
      recyclable: 'foo',
    });
    assertType<ListItemProps>({
      'item-key': 'test',
      // @ts-expect-error
      recyclable: 0,
    });
  });
});

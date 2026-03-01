// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { ItemProps, MenuItem } from '@components/menu-item';
import { useTheme } from '@explorer/lib';
import './index.scss';

interface MenuProps {
  items: ItemProps[];
}

export function Menu(menu: MenuProps) {
  const { withTheme } = useTheme();
  const { items } = menu;
  return (
    <view clip-radius="true" className={withTheme('page')}>
      <scroll-view scroll-y clip-radius="true" className="list">
        {items.map((item: ItemProps) => {
          return <MenuItem {...item} />;
        })}
      </scroll-view>
    </view>
  );
}

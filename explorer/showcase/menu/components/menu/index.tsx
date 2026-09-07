// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { ItemProps, MenuItem } from '@components/menu-item';
import { useSafeArea, useTheme } from '@explorer/lib';
import './index.scss';

interface MenuProps {
  items: ItemProps[];
}

export function Menu(menu: MenuProps) {
  const { withTheme } = useTheme();
  const safeArea = useSafeArea();
  const { items } = menu;
  const horizontalSafeArea = safeArea.left + safeArea.right;
  const bottomPadding = 30 + safeArea.bottom;
  return (
    <view clip-radius="true" className={withTheme('page')}>
      <scroll-view
        scroll-y
        clip-radius="true"
        className="list"
        style={{
          marginLeft: `${safeArea.left}px`,
          marginRight: `${safeArea.right}px`,
          width: `calc(100% - ${horizontalSafeArea}px)`,
          paddingBottom: `${bottomPadding}px`,
        }}
      >
        {items.map((item: ItemProps) => {
          return <MenuItem {...item} />;
        })}
      </scroll-view>
    </view>
  );
}

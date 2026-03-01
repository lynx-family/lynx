// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import './index.scss';
import { useTheme, useSafeArea } from '@explorer/lib';

import homeIconDark from '@assets/images/home-dark.png?inline';
import selectedHomeIconDark from '@assets/images/home-selected-dark.png?inline';
import selectedHomeIcon from '@assets/images/home-selected.png?inline';
import homeIcon from '@assets/images/home.png?inline';
import settingsIconDark from '@assets/images/settings-dark.png?inline';
import selectedSettingsIconDark from '@assets/images/settings-selected-dark.png?inline';
import selectedSettingsIcon from '@assets/images/settings-selected.png?inline';
import settingsIcon from '@assets/images/settings.png?inline';

interface NavigatorProps {
  activePage: 'home' | 'settings';
  onNavigate: (page: 'home' | 'settings') => void;
}

type IconName = 'home' | 'settings';

export default function Navigator(props: NavigatorProps) {
  const { resolved, withTheme } = useTheme();
  const safeArea = useSafeArea();

  const icons = {
    home: {
      selected: { dark: selectedHomeIconDark, light: selectedHomeIcon },
      unselected: { dark: homeIconDark, light: homeIcon },
    },
    settings: {
      selected: { dark: selectedSettingsIconDark, light: selectedSettingsIcon },
      unselected: { dark: settingsIconDark, light: settingsIcon },
    },
  } as const;

  const getIcon = (name: IconName, selected: boolean) =>
    icons[name][selected ? 'selected' : 'unselected'][resolved];

  return (
    <view
      clip-radius="true"
      className={withTheme('navigator')}
      style={{ paddingBottom: `${safeArea.bottom}px` }}
    >
      <view
        className="button"
        bindtap={() => props.onNavigate('home')}
        accessibility-element={true}
        accessibility-label="Show Home Page"
        accessibility-traits="button"
      >
        <image
          src={getIcon('home', props.activePage === 'home')}
          className="icon"
        />
      </view>
      <view
        className="button"
        bindtap={() => props.onNavigate('settings')}
        accessibility-element={true}
        accessibility-label="Show Settings Page"
        accessibility-traits="button"
      >
        <image
          src={getIcon('settings', props.activePage === 'settings')}
          className="icon"
        />
      </view>
    </view>
  );
}

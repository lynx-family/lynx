// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import './index.scss';

import homeIconDark from '@assets/images/home-dark.png?inline';
import selectedHomeIconDark from '@assets/images/home-selected-dark.png?inline';
import selectedHomeIcon from '@assets/images/home-selected.png?inline';
import homeIcon from '@assets/images/home.png?inline';
import settingsIconDark from '@assets/images/settings-dark.png?inline';
import selectedSettingsIconDark from '@assets/images/settings-selected-dark.png?inline';
import selectedSettingsIcon from '@assets/images/settings-selected.png?inline';
import settingsIcon from '@assets/images/settings.png?inline';

interface NavigatorProps {
  showHomePage: boolean;
  showSettingsPage: boolean;
  currentTheme: string;
  withTheme: (className: string) => string;
  openHomePage: () => void;
  openSettingsPage: () => void;
}

type IconName = 'home' | 'settings';
type ThemeType = 'Dark' | 'Light';

export default function Navigator(props: NavigatorProps) {
  const icons = {
    home: {
      selected: {
        Dark: selectedHomeIconDark,
        Light: selectedHomeIcon,
      },
      unselected: {
        Dark: homeIconDark,
        Light: homeIcon,
      },
    },
    settings: {
      selected: {
        Dark: selectedSettingsIconDark,
        Light: selectedSettingsIcon,
      },
      unselected: {
        Dark: settingsIconDark,
        Light: settingsIcon,
      },
    },
  } as const;

  const getIcon = (name: IconName, selected: boolean) => {
    const { currentTheme } = props;
    if (currentTheme !== 'Auto') {
      return icons[name][selected ? 'selected' : 'unselected'][
        currentTheme as ThemeType
      ];
    }
    return icons[name][selected ? 'selected' : 'unselected'][
      lynx.__globalProps.theme as ThemeType
    ];
  };

  return (
    <view clip-radius="true" className={props.withTheme('navigator')}>
      <view
        className="button"
        bindtap={props.openHomePage}
        accessibility-element={true}
        accessibility-label="Show Home Page"
        accessibility-traits="button"
      >
        <image src={getIcon('home', props.showHomePage)} className="icon" />
      </view>
      <view
        className="button"
        bindtap={props.openSettingsPage}
        accessibility-element={true}
        accessibility-label="Show Settings Page"
        accessibility-traits="button"
      >
        <image
          src={getIcon('settings', props.showSettingsPage)}
          className="icon"
        />
      </view>
    </view>
  );
}

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { Component } from '@lynx-js/react';
import './index.scss';

import homeIcon from '@assets/images/home.png?inline';
import homeIconDark from '@assets/images/home-dark.png?inline';
import selectedHomeIcon from '@assets/images/home-selected.png?inline';
import selectedHomeIconDark from '@assets/images/home-selected-dark.png?inline';
import settingsIcon from '@assets/images/settings.png?inline';
import settingsIconDark from '@assets/images/settings-dark.png?inline';
import selectedSettingsIcon from '@assets/images/settings-selected.png?inline';
import selectedSettingsIconDark from '@assets/images/settings-selected-dark.png?inline';

interface NavigatorProps {
  showHomePage: boolean;
  showSettingsPage: boolean;
  currentTheme: string;
  withTheme: (className: string) => string;
  openHomePage: () => void;
  openSettingsPage: () => void;
}

export default class Navigator extends Component<NavigatorProps, unknown> {
  icons = {
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
  };

  constructor(props: any) {
    super(props);
  }

  icon(name: string, selected: boolean) {
    const { currentTheme } = this.props;
    if (currentTheme !== 'Auto') {
      return this.icons[name][selected ? 'selected' : 'unselected'][
        currentTheme
      ];
    }
    return this.icons[name][selected ? 'selected' : 'unselected'][
      lynx.__globalProps.theme
    ];
  }

  render() {
    return (
      <view clip-radius="true" className={this.props.withTheme('navigator')}>
        <view className="button" bindtap={this.props.openHomePage}>
          <image
            src={this.icon('home', this.props.showHomePage)}
            className="icon"
          />
        </view>
        <view className="button" bindtap={this.props.openSettingsPage}>
          <image
            src={this.icon('settings', this.props.showSettingsPage)}
            className="icon"
          />
        </view>
      </view>
    );
  }
}

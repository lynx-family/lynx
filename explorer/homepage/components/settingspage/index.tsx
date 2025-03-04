// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { Component } from '@lynx-js/react';
import './index.scss';

import ForwardIcon from '@assets/images/forward.png?inline';
import ForwardDarkIcon from '@assets/images/forward-dark.png?inline';
import AutoDarkIcon from '@assets/images/auto-dark.png?inline';
import AutoLightIcon from '@assets/images/auto.png?inline';
import DarkDarkIcon from '@assets/images/dark-dark.png?inline';
import DarkLightIcon from '@assets/images/dark.png?inline';
import LightDarkIcon from '@assets/images/light-dark.png?inline';
import LightLightIcon from '@assets/images/light.png?inline';

interface SettingsPageProps {
  showPage: boolean;

  themes: string[];
  currentTheme: string;
  setTheme: (theme: string) => void;
  withTheme: (className: string) => string;
  withNotchScreen: (className: string) => string;
}

export default class SettingsPage extends Component<
  SettingsPageProps,
  unknown
> {
  icons = {
    Auto: {
      Dark: AutoDarkIcon,
      Light: AutoLightIcon,
    },
    Dark: {
      Dark: DarkDarkIcon,
      Light: DarkLightIcon,
    },
    Light: {
      Dark: LightDarkIcon,
      Light: LightLightIcon,
    },
    Forward: {
      Dark: ForwardDarkIcon,
      Light: ForwardIcon,
    },
  };

  constructor(props: any) {
    super(props);
  }

  openDevtoolSwitchPage = () => {
    NativeModules.ExplorerModule.openDevtoolSwitchPage();
  };

  icon(name: string) {
    const { currentTheme } = this.props;
    if (currentTheme !== 'Auto') {
      return this.icons[name][currentTheme];
    }
    return this.icons[name][lynx.__globalProps.theme];
  }

  render() {
    const { showPage } = this.props;
    const { themes, currentTheme, withTheme, withNotchScreen } = this.props;

    if (!showPage) {
      return <></>;
    }

    return (
      <view clip-radius="true" className={withTheme('page')}>
        <view className={withNotchScreen('page-header')}>
          <text className={withTheme('title')}>Settings</text>
        </view>

        <view style="margin: 0px 5% 0px 5%; height: 5%">
          <text className={withTheme('sub-title')}>Theme</text>
        </view>
        <view className={withTheme('theme')}>
          {themes.map((theme) => {
            return (
              <view
                className="option-item"
                bindtap={() => this.props.setTheme(theme)}
              >
                <image src={this.icon(theme)} className="option-icon" />
                <text className={withTheme('text')}>{theme}</text>
                <view
                  className={
                    currentTheme == theme
                      ? withTheme('radio-button-container-active')
                      : withTheme('radio-button-container-inactive')
                  }
                >
                  {currentTheme == theme ? (
                    <view className={withTheme('radio-button-active')} />
                  ) : (
                    <view className={withTheme('radio-button')} />
                  )}
                </view>
              </view>
            );
          })}
        </view>

        <view style="margin: 3% 5% 0px 5%; height: 5%">
          <text className={withTheme('sub-title')}>DevTool</text>
        </view>
        <view
          className={withTheme('devtool')}
          bindtap={this.openDevtoolSwitchPage}
        >
          <text className={withTheme('text')}>Lynx DevTool Switches</text>
          <view style="margin: auto 5% auto auto; justify-content: center">
            <image src={this.icon('Forward')} className="forward-icon" />
          </view>
        </view>
      </view>
    );
  }
}
